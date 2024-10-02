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
  D->addAttr(::new (SemaRef.Context) SYCLKernelEntryPointAttr(SemaRef.Context,
                                                              AL, TSI));
}

namespace {

// Some diagnostics are produced by detecting a condition and then constructing
// notes on a stack while unwinding.
using PartialDiagStack = SmallVector<PartialDiagnosticAt, 1>;

SourceLocation SourceLocationForType(QualType QT) {
  SourceLocation Loc;
  const Type *T = QT->getUnqualifiedDesugaredType();
  if (const TagType *TT = dyn_cast<TagType>(T))
    Loc = TT->getDecl()->getLocation();
  else if (const ObjCInterfaceType *ObjCIT = dyn_cast<ObjCInterfaceType>(T))
    Loc = ObjCIT->getDecl()->getLocation();
  return Loc;
}

bool CheckDeclIsNamed(Sema &S, TagDecl *TD, PartialDiagStack &DiagNotes) {
  if (TD->getIdentifier() == nullptr) {
    if (TD->getTypedefNameForAnonDecl() == nullptr) {
      DiagNotes.push_back(
        { TD->getLocation(),
          S.PDiag(diag::note_sycl_kernel_name_not_forward_declarable) << TD
                  << 1 /* it is an unnamed type */ });
    } else {
      DiagNotes.push_back(
        { TD->getLocation(),
          S.PDiag(diag::note_sycl_kernel_name_not_forward_declarable) << TD
                  << 2 /* it is an alias of an unnamed type */ });
    }
    return true;
  }
  return false;
}

bool CheckDeclarableAtNamespaceScope(Sema &S, NamedDecl *ND,
                                     PartialDiagStack &DiagNotes) {
  // From SYCL 2020 section 5.2, "Naming of kernels":
  // - The kernel name must be forward declarable at namespace scope (including
  //   global namespace scope) and may not be forward declared other than at
  //   namespace scope. If it isn't forward declared but is specified as a
  //   template argument in a kernel invoking interface, as described in
  //   Section 4.9.4.2, then it may not conflict with a name in any enclosing
  //   namespace scope.
  // That section also includes the following note:
  // - The requirement that a kernel name be forward declarable makes some
  //   types for kernel names illegal, such as anything declared in the std
  //   namespace (adding a declaration to namespace std leads to undefined
  //   behavior).
  // The note and its allegation that kernel name types cannot be composed from
  // members of the std namespace is disputed as reported at
  // https://github.com/KhronosGroup/SYCL-Docs/issues/629.

  assert(ND->getIdentifier() &&
         "Declaration must have its own name, not a name for linkage purposes");

  if (!isa<TranslationUnitDecl, NamespaceDecl, LinkageSpecDecl>(
           ND->getDeclContext())) {
    DiagNotes.push_back(
      { ND->getLocation(),
        S.PDiag(diag::note_sycl_kernel_name_not_forward_declarable) << ND
                << 4 /* it is not declared at global or namespace scope */ });
    return true;
  }

  for (const DeclContext *DeclCtx = ND->getDeclContext();
       !isa<TranslationUnitDecl>(DeclCtx); DeclCtx = DeclCtx->getParent()) {
    if (const auto *NSD = dyn_cast<NamespaceDecl>(DeclCtx)) {
      if (NSD->isStdNamespace()) {
        DiagNotes.push_back(
          { ND->getLocation(),
            S.PDiag(diag::note_sycl_kernel_name_not_forward_declarable) << ND
                    << 5 /* it is declared within the 'std' namespace */ });
        return true;
      }
    }
  }

  return false;
}

// SYCL 2020 section 5.2, "Naming of kernels", requires that types used as
// kernel names be forward declarable. This requirement is present to support
// implementations that separately compile source code for the host target vs
// device targets and where the host compiler is not SYCL aware. Such
// implementations generate an "integration header" during device compilation
// that is then pre-included during host compilation in order to make SYCL
// kernel information available to the SYCL library. Since this header is
// pre-included with kernel information indexed by kernel name type, it is
// necessary that kernel name types be forward declarable. Since Clang is SYCL
// aware and does not require nor use an integration header, it has no need to
// require that kernel name types be forward declarable. However, diagnostics
// are issued for types that are not forward declarable to identify portability
// concerns.
bool CheckForwardDeclarableHelper(Sema &S, const Type *T,
                                  PartialDiagStack &DiagNotes);
bool CheckForwardDeclarableHelper(Sema &S, TagDecl *TD,
                                  PartialDiagStack &DiagNotes);
bool CheckForwardDeclarableHelper(Sema &S, EnumDecl *ED,
                                  PartialDiagStack &DiagNotes);
bool CheckForwardDeclarableHelper(Sema &S, RecordDecl *RD,
                                  PartialDiagStack &DiagNotes);

bool CheckForwardDeclarable(Sema &S, QualType QT, PartialDiagStack &DiagNotes) {
  // Qualifiers don't affect forward declarability.
  const Type *T = QT.getTypePtr();
  return CheckForwardDeclarableHelper(S, T, DiagNotes);
}

bool CheckForwardDeclarableHelper(Sema &S, const Type *T,
                                  PartialDiagStack &DiagNotes) {
  // Sugared types don't affect forward declarability.
  T = T->getUnqualifiedDesugaredType();
  if (auto *TT = T->getAs<TagType>()) {
    TagDecl *TD = TT->getDecl();
    return CheckForwardDeclarableHelper(S, TD, DiagNotes);
  }

  return false;
}

bool CheckForwardDeclarableHelper(Sema &S, TagDecl *TD,
                                  PartialDiagStack &DiagNotes) {
  if (auto *ED = dyn_cast<EnumDecl>(TD))
    return CheckForwardDeclarableHelper(S, ED, DiagNotes);
  if (auto *RD = dyn_cast<RecordDecl>(TD))
    return CheckForwardDeclarableHelper(S, RD, DiagNotes);
  assert(0 && "TagDecl that is neither an EnumDecl or a RecordDecl");
  return false;
}

bool CheckForwardDeclarableHelper(Sema &S, EnumDecl *ED,
                                  PartialDiagStack &DiagNotes) {
  if (CheckDeclIsNamed(S, ED, DiagNotes))
    return true;
  if (CheckDeclarableAtNamespaceScope(S, ED, DiagNotes))
    return true;
  if (!ED->isScoped() && !ED->isFixed()) {
    DiagNotes.push_back(
      { ED->getLocation(),
        S.PDiag(diag::note_sycl_kernel_name_not_forward_declarable) << ED
                << 3 /* it is an unscoped enumeration type without a fixed
                        underlying type */ });
    return true;
  }
  return false;
}

bool CheckForwardDeclarableHelper(Sema &S, RecordDecl *RD,
                                  PartialDiagStack &DiagNotes) {
  // From SYCL 2020 section 5.2, "Naming of kernels":
  // - If the kernel is defined as a lambda, a typename can optionally be
  //   provided to the kernel invoking interface as described in Section
  //   4.9.4.2, so that the developer can control the kernel name for
  //   purposes such as debugging or referring to the kernel when applying
  //   build options.
  // When an explicit kernel name is not provided, the type of the lambda
  // closure class is the only unique identifier available and thus must
  // be used directly or indirectly in the implicit kernel name type.
  // SYCL 2020 acknowledges the implementation challenges posed by implicit
  // kernel name types for lambdas and therefore specifies a reduced feature
  // set that does not include support for them. A pedantic warning could be
  // issued to note use of a lambda closure type in a kernel name as a
  // non-portable SYCL feature, but such use is very common and so far
  // considered unwarranted. Support for implici kernel names for lambda
  // kernels is not an extension.
  if (RD->isLambda())
    return false;

  if (CheckDeclIsNamed(S, RD, DiagNotes))
    return true;
  if (CheckDeclarableAtNamespaceScope(S, RD, DiagNotes))
    return true;
  return false;
}

bool CheckSYCLKernelName(Sema &S, SourceLocation Loc, QualType KernelName) {
  assert(!KernelName->isDependentType());

  if (!KernelName->isStructureOrClassType()) {
    // SYCL 2020 section 5.2, "Naming of kernels", only requires that the
    // kernel name be a C++ typename. However, the definition of "kernel name"
    // in the glossary states that a kernel name is a class type. Neither
    // section explicitly states whether the kernel name type can be
    // cv-qualified. For now, kernel name types are required to be class types
    // and that they may be cv-qualified. The following issue requests
    // clarification from the SYCL WG.
    //   https://github.com/KhronosGroup/SYCL-Docs/issues/568
    S.Diag(Loc, diag::warn_sycl_kernel_name_not_a_class_type) << KernelName;
    SourceLocation DeclTypeLoc = SourceLocationForType(KernelName);
    if (DeclTypeLoc.isValid())
      S.Diag(DeclTypeLoc, diag::note_entity_declared_at) << KernelName;
    return true;
  }

  PartialDiagStack DiagNotes;
  if (CheckForwardDeclarable(S, KernelName, DiagNotes)) {
    S.Diag(Loc, diag::warn_sycl_kernel_name_invalid) << KernelName;
    while (!DiagNotes.empty()) {
      PartialDiagnosticAt &NoteAt = DiagNotes.back();
      S.Diag(NoteAt.first, NoteAt.second);
      DiagNotes.pop_back();
    }
  }

  return false;
}

} // unnamed namespace

void SemaSYCL::CheckSYCLEntryPointFunctionDecl(FunctionDecl *FD) {
  // Ensure that all attributes present on the declaration are consistent
  // and warn about any redundant ones.
  const SYCLKernelEntryPointAttr *SKEPAttr = nullptr;
  for (auto SAI = FD->specific_attr_begin<SYCLKernelEntryPointAttr>();
       SAI != FD->specific_attr_end<SYCLKernelEntryPointAttr>();
       ++SAI) {
    if (!SKEPAttr) {
      SKEPAttr = *SAI;
      continue;
    }
    if (!getASTContext().hasSameType(SAI->getKernelName(),
                                     SKEPAttr->getKernelName())) {
      Diag(SAI->getKernelNameLoc()->getTypeLoc().getBeginLoc(),
           diag::err_sycl_entry_point_invalid_redeclaration)
          << SAI->getKernelName() << SKEPAttr->getKernelName();
      Diag(SKEPAttr->getKernelNameLoc()->getTypeLoc().getBeginLoc(),
           diag::note_previous_attribute);
      FD->setInvalidDecl();
    } else {
      Diag(SAI->getLocation(),
           diag::warn_sycl_entry_point_redundant_declaration);
      Diag(SKEPAttr->getLocation(), diag::note_previous_attribute);
    }
  }
  assert(SKEPAttr && "Missing sycl_kernel_entry_point attribute");

  // Ensure that an attribute present on the previous declaration
  // matches the one on this declaration.
  FunctionDecl *PrevFD = FD->getPreviousDecl();
  if (PrevFD && !PrevFD->isInvalidDecl()) {
    const auto *PrevSKEPAttr = PrevFD->getAttr<SYCLKernelEntryPointAttr>();
    if (PrevSKEPAttr) {
      if (!getASTContext().hasSameType(SKEPAttr->getKernelName(),
                                       PrevSKEPAttr->getKernelName())) {
        Diag(SKEPAttr->getKernelNameLoc()->getTypeLoc().getBeginLoc(),
             diag::err_sycl_entry_point_invalid_redeclaration)
            << SKEPAttr->getKernelName() << PrevSKEPAttr->getKernelName();
        Diag(PrevSKEPAttr->getKernelNameLoc()->getTypeLoc().getBeginLoc(),
             diag::note_previous_decl)
            << PrevFD;;
        FD->setInvalidDecl();
      }
    }
  }

  if (auto *MD = dyn_cast<CXXMethodDecl>(FD)) {
    if (!MD->isStatic()) {
      Diag(SKEPAttr->getLocation(), diag::err_sycl_entry_point_invalid)
          << /*non-static member function*/0;
      FD->setInvalidDecl();
    }
  }
  if (FD->isVariadic()) {
    Diag(SKEPAttr->getLocation(), diag::err_sycl_entry_point_invalid)
        << /*variadic function*/1;
    FD->setInvalidDecl();
  }
  if (FD->isConsteval()) {
    Diag(SKEPAttr->getLocation(), diag::err_sycl_entry_point_invalid)
        << /*consteval function*/5;
    FD->setInvalidDecl();
  } else if (FD->isConstexpr()) {
    Diag(SKEPAttr->getLocation(), diag::err_sycl_entry_point_invalid)
        << /*constexpr function*/4;
    FD->setInvalidDecl();
  }
  if (FD->isNoReturn()) {
    Diag(SKEPAttr->getLocation(), diag::err_sycl_entry_point_invalid)
        << /*noreturn function*/6;
    FD->setInvalidDecl();
  }

  if (!FD->getReturnType()->isVoidType()) {
    Diag(SKEPAttr->getLocation(), diag::err_sycl_entry_point_return_type);
    FD->setInvalidDecl();
  }

  // Ensure the kernel name type is valid.
  if (!SKEPAttr->getKernelName()->isDependentType() &&
      CheckSYCLKernelName(SemaRef,
                          SKEPAttr->getKernelNameLoc()->getTypeLoc()
                              .getBeginLoc(),
                          SKEPAttr->getKernelName())) {
    FD->setInvalidDecl();
  }

  if (!FD->isInvalidDecl() && !FD->isDependentContext()) {
    const SYCLKernelInfo *SKI =
        getASTContext().findSYCLKernelInfo(SKEPAttr->getKernelName());
    if (SKI) {
      if (!declaresSameEntity(FD, SKI->GetKernelEntryPointDecl())) {
        // FIXME: This diagnostic should include the origin of the kernel
        // FIXME: names; not just the locations of the conflicting declarations.
        Diag(FD->getLocation(), diag::err_sycl_kernel_name_conflict);
        Diag(SKI->GetKernelEntryPointDecl()->getLocation(),
             diag::note_previous_declaration);
        FD->setInvalidDecl();
      }
    } else {
      getASTContext().registerSYCLEntryPointFunction(FD);
    }
  }
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

} // unnamed namespace

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
  if (!declaresSameEntity(SKI.GetKernelEntryPointDecl(), FD))
    llvm::report_fatal_error("SYCL kernel name conflict");

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
