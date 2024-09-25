//===- SemaSYCL.cpp - Semantic Analysis for SYCL constructs ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This implements Semantic Analysis for SYCL constructs.
//===----------------------------------------------------------------------===//

#include "TreeTransform.h"
#include "clang/Sema/SemaSYCL.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/StmtSYCL.h"
#include "clang/AST/SYCLKernelInfo.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Sema/Attr.h"
#include "clang/Sema/ParsedAttr.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaDiagnostic.h"

using namespace clang;

// -----------------------------------------------------------------------------
// SYCL device specific diagnostics implementation
// -----------------------------------------------------------------------------

SemaSYCL::SemaSYCL(Sema &S) : SemaBase(S) {}

Sema::SemaDiagnosticBuilder SemaSYCL::DiagIfDeviceCode(SourceLocation Loc,
                                                       unsigned DiagID) {
  assert(getLangOpts().SYCLIsDevice &&
         "Should only be called during SYCL compilation");
  FunctionDecl *FD = dyn_cast<FunctionDecl>(SemaRef.getCurLexicalContext());
  SemaDiagnosticBuilder::Kind DiagKind = [this, FD] {
    if (!FD)
      return SemaDiagnosticBuilder::K_Nop;
    if (SemaRef.getEmissionStatus(FD) == Sema::FunctionEmissionStatus::Emitted)
      return SemaDiagnosticBuilder::K_ImmediateWithCallStack;
    return SemaDiagnosticBuilder::K_Deferred;
  }();
  return SemaDiagnosticBuilder(DiagKind, Loc, DiagID, FD, SemaRef);
}

static bool isZeroSizedArray(SemaSYCL &S, QualType Ty) {
  if (const auto *CAT = S.getASTContext().getAsConstantArrayType(Ty))
    return CAT->isZeroSize();
  return false;
}

void SemaSYCL::deepTypeCheckForDevice(SourceLocation UsedAt,
                                      llvm::DenseSet<QualType> Visited,
                                      ValueDecl *DeclToCheck) {
  assert(getLangOpts().SYCLIsDevice &&
         "Should only be called during SYCL compilation");
  // Emit notes only for the first discovered declaration of unsupported type
  // to avoid mess of notes. This flag is to track that error already happened.
  bool NeedToEmitNotes = true;

  auto Check = [&](QualType TypeToCheck, const ValueDecl *D) {
    bool ErrorFound = false;
    if (isZeroSizedArray(*this, TypeToCheck)) {
      DiagIfDeviceCode(UsedAt, diag::err_typecheck_zero_array_size) << 1;
      ErrorFound = true;
    }
    // Checks for other types can also be done here.
    if (ErrorFound) {
      if (NeedToEmitNotes) {
        if (auto *FD = dyn_cast<FieldDecl>(D))
          DiagIfDeviceCode(FD->getLocation(),
                           diag::note_illegal_field_declared_here)
              << FD->getType()->isPointerType() << FD->getType();
        else
          DiagIfDeviceCode(D->getLocation(), diag::note_declared_at);
      }
    }

    return ErrorFound;
  };

  // In case we have a Record used do the DFS for a bad field.
  SmallVector<const ValueDecl *, 4> StackForRecursion;
  StackForRecursion.push_back(DeclToCheck);

  // While doing DFS save how we get there to emit a nice set of notes.
  SmallVector<const FieldDecl *, 4> History;
  History.push_back(nullptr);

  do {
    const ValueDecl *Next = StackForRecursion.pop_back_val();
    if (!Next) {
      assert(!History.empty());
      // Found a marker, we have gone up a level.
      History.pop_back();
      continue;
    }
    QualType NextTy = Next->getType();

    if (!Visited.insert(NextTy).second)
      continue;

    auto EmitHistory = [&]() {
      // The first element is always nullptr.
      for (uint64_t Index = 1; Index < History.size(); ++Index) {
        DiagIfDeviceCode(History[Index]->getLocation(),
                         diag::note_within_field_of_type)
            << History[Index]->getType();
      }
    };

    if (Check(NextTy, Next)) {
      if (NeedToEmitNotes)
        EmitHistory();
      NeedToEmitNotes = false;
    }

    // In case pointer/array/reference type is met get pointee type, then
    // proceed with that type.
    while (NextTy->isAnyPointerType() || NextTy->isArrayType() ||
           NextTy->isReferenceType()) {
      if (NextTy->isArrayType())
        NextTy = QualType{NextTy->getArrayElementTypeNoTypeQual(), 0};
      else
        NextTy = NextTy->getPointeeType();
      if (Check(NextTy, Next)) {
        if (NeedToEmitNotes)
          EmitHistory();
        NeedToEmitNotes = false;
      }
    }

    if (const auto *RecDecl = NextTy->getAsRecordDecl()) {
      if (auto *NextFD = dyn_cast<FieldDecl>(Next))
        History.push_back(NextFD);
      // When nullptr is discovered, this means we've gone back up a level, so
      // the history should be cleaned.
      StackForRecursion.push_back(nullptr);
      llvm::copy(RecDecl->fields(), std::back_inserter(StackForRecursion));
    }
  } while (!StackForRecursion.empty());
}

ExprResult SemaSYCL::BuildUniqueStableNameExpr(SourceLocation OpLoc,
                                               SourceLocation LParen,
                                               SourceLocation RParen,
                                               TypeSourceInfo *TSI) {
  return SYCLUniqueStableNameExpr::Create(getASTContext(), OpLoc, LParen,
                                          RParen, TSI);
}

ExprResult SemaSYCL::ActOnUniqueStableNameExpr(SourceLocation OpLoc,
                                               SourceLocation LParen,
                                               SourceLocation RParen,
                                               ParsedType ParsedTy) {
  TypeSourceInfo *TSI = nullptr;
  QualType Ty = SemaRef.GetTypeFromParser(ParsedTy, &TSI);

  if (Ty.isNull())
    return ExprError();
  if (!TSI)
    TSI = getASTContext().getTrivialTypeSourceInfo(Ty, LParen);

  return BuildUniqueStableNameExpr(OpLoc, LParen, RParen, TSI);
}

void SemaSYCL::handleKernelAttr(Decl *D, const ParsedAttr &AL) {
  // The 'sycl_kernel' attribute applies only to function templates.
  const auto *FD = cast<FunctionDecl>(D);
  const FunctionTemplateDecl *FT = FD->getDescribedFunctionTemplate();
  assert(FT && "Function template is expected");

  // Function template must have at least two template parameters.
  const TemplateParameterList *TL = FT->getTemplateParameters();
  if (TL->size() < 2) {
    Diag(FT->getLocation(), diag::warn_sycl_kernel_num_of_template_params);
    return;
  }

  // Template parameters must be typenames.
  for (unsigned I = 0; I < 2; ++I) {
    const NamedDecl *TParam = TL->getParam(I);
    if (isa<NonTypeTemplateParmDecl>(TParam)) {
      Diag(FT->getLocation(),
           diag::warn_sycl_kernel_invalid_template_param_type);
      return;
    }
  }

  // Function must have at least one argument.
  if (getFunctionOrMethodNumParams(D) != 1) {
    Diag(FT->getLocation(), diag::warn_sycl_kernel_num_of_function_params);
    return;
  }

  // Function must return void.
  QualType RetTy = getFunctionOrMethodResultType(D);
  if (!RetTy->isVoidType()) {
    Diag(FT->getLocation(), diag::warn_sycl_kernel_return_type);
    return;
  }

  handleSimpleAttribute<SYCLKernelAttr>(*this, D, AL);
}

void SemaSYCL::handleKernelEntryPointAttr(Decl *D, const ParsedAttr &AL) {
  ParsedType PT = AL.getTypeArg();
  TypeSourceInfo *TSI = nullptr;
  (void)SemaRef.GetTypeFromParser(PT, &TSI);
  assert(TSI && "no type source info for attribute argument");

  FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
  assert(FD && "Not a function decl");

  bool hasError = false;
  if (auto *MD = dyn_cast<CXXMethodDecl>(D)) {
    if (!MD->isStatic()) {
      Diag(AL.getLoc(), diag::err_sycl_entry_point_invalid)
          << /*non-static member function*/ 0;
      hasError = true;
    }
  }
  if (FD->isVariadic()) {
    Diag(AL.getLoc(), diag::err_sycl_entry_point_invalid)
        << /*variadic function*/ 1;
    hasError = true;
  }
  if (FD->isConsteval()) {
    Diag(AL.getLoc(), diag::err_sycl_entry_point_invalid)
        << /*consteval function*/ 5;
    hasError = true;
  } else if (FD->isConstexpr()) {
    Diag(AL.getLoc(), diag::err_sycl_entry_point_invalid)
        << /*constexpr function*/ 4;
    hasError = true;
  }
  QualType Ret = FD->getReturnType();
  if (!Ret->isDependentType() && !Ret->isVoidType()) {
    Diag(AL.getLoc(), diag::err_sycl_entry_point_return_type);
    hasError = true;
  }

  if (!hasError)
    D->addAttr(::new (SemaRef.Context)
                   SYCLKernelEntryPointAttr(SemaRef.Context, AL, TSI));
}

namespace {

// The body of a function declared with the [[sycl_kernel_entry_point]]
// attribute is cloned and transformed to substitute references to the original
// function parameters with references to replacement variables that stand in
// for SYCL kernel parameters or local variables that reconstitute a decomposed
// SYCL kernel argument.
class OutlinedFunctionDeclBodyInstantiator
    : public TreeTransform<OutlinedFunctionDeclBodyInstantiator> {
public:
  using ParmDeclMap = llvm::DenseMap<ParmVarDecl*, VarDecl*>;

  OutlinedFunctionDeclBodyInstantiator(Sema &S, ParmDeclMap &M)
      : TreeTransform<OutlinedFunctionDeclBodyInstantiator>(S),
        SemaRef(S), MapRef(M) {}

  // A new set of AST nodes is always required.
  bool AlwaysRebuild() {
    return true;
  }

  // Transform ParmVarDecl references to the supplied replacement variables.
  ExprResult TransformDeclRefExpr(DeclRefExpr *DRE) {
    const ParmVarDecl *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl());
    if (PVD) {
      ParmDeclMap::iterator I = MapRef.find(PVD);
      if (I != MapRef.end()) {
        VarDecl *VD = I->second;
        assert(SemaRef.getASTContext().hasSameUnqualifiedType(PVD->getType(),
                                                              VD->getType()));
        assert(!VD->getType().isMoreQualifiedThan(PVD->getType()));
        VD->setIsUsed();
        // The replacement DeclRefExpr
        return DeclRefExpr::Create(
            SemaRef.getASTContext(), DRE->getQualifierLoc(),
            DRE->getTemplateKeywordLoc(), VD, false, DRE->getNameInfo(),
            DRE->getType(), DRE->getValueKind());
      }
    }
    return DRE;
  }

private:
  Sema &SemaRef;
  ParmDeclMap &MapRef;
};

void DiagnoseSYCLEntryPoint(Sema &S, FunctionDecl *FD,
                            const PartialDiagnosticAt &Diag) {
  if (S.inTemplateInstantiation()) {
    // We are synthesizing decls but we may not commit to use at all.
    // Delay diagnostics until we know it is needed.
    auto &SuppressedDiags = S.SYCL().EntryPointSuppressedDiagnostics[FD];
    SuppressedDiags.push_back(Diag);
    if (S.getDiagnostics().getDiagnosticLevel(
            Diag.second.getDiagID(), Diag.first) >= DiagnosticsEngine::Warning)
      S.PrintInstantiationStack([&](const PartialDiagnosticAt &PD) {
        SuppressedDiags.push_back(PD);
      });
  } else {
    S.Diag(Diag.first, Diag.second);
  }
}

} // unnamed namespace

void SemaSYCL::CheckSYCLEntryPointFunctionDecl(FunctionDecl *FD) {
  // Ensure that all attributes present on the declaration are consistent
  // and warn about any redundant ones.
  const SYCLKernelEntryPointAttr *SKEPAttr = nullptr;
  for (auto SAI = FD->specific_attr_begin<SYCLKernelEntryPointAttr>();
       SAI != FD->specific_attr_end<SYCLKernelEntryPointAttr>(); ++SAI) {
    if (!SKEPAttr) {
      SKEPAttr = *SAI;
      continue;
    }
    if (!getASTContext().hasSameType(SAI->getKernelName(),
                                     SKEPAttr->getKernelName())) {
      DiagnoseSYCLEntryPoint(
          SemaRef, FD,
          {SAI->getLocation(),
           PDiag(diag::err_sycl_entry_point_invalid_redeclaration)
               << SAI->getKernelName() << SKEPAttr->getKernelName()});
      DiagnoseSYCLEntryPoint(
          SemaRef, FD,
          {SKEPAttr->getLocation(), PDiag(diag::note_previous_attribute)});
    } else {
      DiagnoseSYCLEntryPoint(
          SemaRef, FD,
          {SAI->getLocation(),
           PDiag(diag::warn_sycl_entry_point_redundant_declaration)});
      DiagnoseSYCLEntryPoint(
          SemaRef, FD,
          {SKEPAttr->getLocation(), PDiag(diag::note_previous_attribute)});
    }
  }
  assert(SKEPAttr && "Missing sycl_kernel_entry_point attribute");

  // Ensure the kernel name type is a class type.
  if (!SKEPAttr->getKernelName()->isDependentType() &&
      !SKEPAttr->getKernelName()->isStructureOrClassType()) {
    DiagnoseSYCLEntryPoint(
        SemaRef, FD,
        {SKEPAttr->getLocation(), PDiag(diag::err_sycl_kernel_name_type)});
  }

  // Ensure that an attribute present on the previous declaration
  // matches the one on this declaration.
  FunctionDecl *PrevFD = FD->getPreviousDecl();
  if (PrevFD && !PrevFD->isInvalidDecl()) {
    const auto *PrevSKEPAttr = PrevFD->getAttr<SYCLKernelEntryPointAttr>();
    if (PrevSKEPAttr) {
      if (!getASTContext().hasSameType(SKEPAttr->getKernelName(),
                                       PrevSKEPAttr->getKernelName())) {
        DiagnoseSYCLEntryPoint(
            SemaRef, FD,
            {SKEPAttr->getLocation(),
             PDiag(diag::err_sycl_entry_point_invalid_redeclaration)
                 << SKEPAttr->getKernelName()
                 << PrevSKEPAttr->getKernelName()});
        DiagnoseSYCLEntryPoint(SemaRef, FD,
                               {PrevSKEPAttr->getLocation(),
                                PDiag(diag::note_previous_decl) << PrevFD});
      }
    }
  }

  if (FD->isNoReturn()) {
    DiagnoseSYCLEntryPoint(
        SemaRef, FD,
        {SKEPAttr->getLocation(), PDiag(diag::err_sycl_entry_point_invalid)
                                      << /*noreturn function*/ 6});
  }

  if (!FD->getReturnType()->isVoidType()) {
    DiagnoseSYCLEntryPoint(SemaRef, FD,
                           {SKEPAttr->getLocation(),
                            PDiag(diag::err_sycl_entry_point_return_type)});
  }

  if (!FD->isInvalidDecl() && !FD->isDependentContext()) {
    const SYCLKernelInfo *SKI =
        getASTContext().findSYCLKernelInfo(SKEPAttr->getKernelName());
    if (SKI) {
      if (!declaresSameEntity(FD, SKI->GetKernelEntryPointDecl())) {
        DiagnoseSYCLEntryPoint(
            SemaRef, FD,
            {FD->getLocation(), PDiag(diag::err_sycl_kernel_name_conflict)
                                    << SKEPAttr->getKernelName()});
        DiagnoseSYCLEntryPoint(SemaRef, FD,
                               {SKI->GetKernelEntryPointDecl()->getLocation(),
                                PDiag(diag::note_previous_declaration)});
        for (const PartialDiagnosticAt &PD :
             SYCLKernelEntryContextNotes.at(SKI->GetKernelEntryPointDecl())) {
          DiagnoseSYCLEntryPoint(SemaRef, FD, PD);
        }
      }
    } else {
      // Note: In order to not interfere with SFINAE, we delay the diagnostic of
      // conflicting names to when we act on the attribute.
      ContextNotes &Notes = SYCLKernelEntryContextNotes[FD];
      SemaRef.PrintInstantiationStack(
          [&](const PartialDiagnosticAt &PD) { Notes.push_back(PD); });
      getASTContext().registerSYCLEntryPointFunction(FD);
    }
  }
}

StmtResult SemaSYCL::BuildSYCLKernelCallStmt(FunctionDecl *FD, Stmt *Body) {
  assert(!FD->isInvalidDecl());
  assert(!FD->isDependentContext());
  assert(FD->hasPrototype());

  const auto *SKEPAttr = FD->getAttr<SYCLKernelEntryPointAttr>();
  assert(SKEPAttr && "Missing sycl_kernel_entry_point attribute");

  // Ensure that the kernel name was previously registered and that the
  // stored declaration matches.
  const SYCLKernelInfo &SKI =
        getASTContext().getSYCLKernelInfo(SKEPAttr->getKernelName());
  if (!declaresSameEntity(SKI.GetKernelEntryPointDecl(), FD)) {
    FD->setInvalidDecl();
    return {/*Invalid=*/true};
  }

  using ParmDeclMap = OutlinedFunctionDeclBodyInstantiator::ParmDeclMap;
  ParmDeclMap ParmMap;

  assert(SemaRef.CurContext == FD);
  OutlinedFunctionDecl *OFD =
      OutlinedFunctionDecl::Create(getASTContext(), FD, FD->getNumParams());
  unsigned i = 0;
  for (ParmVarDecl *PVD : FD->parameters()) {
    ImplicitParamDecl *IPD =
        ImplicitParamDecl::Create(getASTContext(), OFD, SourceLocation(),
                                  PVD->getIdentifier(), PVD->getType(),
                                  ImplicitParamKind::Other);
    OFD->setParam(i, IPD);
    ParmMap[PVD] = IPD;
    ++i;
  }

  OutlinedFunctionDeclBodyInstantiator OFDBodyInstantiator(SemaRef, ParmMap);
  Stmt *OFDBody = OFDBodyInstantiator.TransformStmt(Body).get();
  OFD->setBody(OFDBody);
  OFD->setNothrow();
  Stmt *NewBody = new (getASTContext()) SYCLKernelCallStmt(Body, OFD);

  return NewBody;
}

bool SemaSYCL::EmitDelayedKernelEntryPointDiagnostics(Decl *FD) {
  auto Pos = EntryPointSuppressedDiagnostics.find(FD);
  if (Pos != EntryPointSuppressedDiagnostics.end()) {
    for (const PartialDiagnosticAt &Suppressed : Pos->second) {
      DiagnosticBuilder Builder(SemaRef.Diags.Report(
          Suppressed.first, Suppressed.second.getDiagID()));
      Suppressed.second.Emit(Builder);
    }
    Pos->second.clear();
    return true;
  }
  return false;
}
