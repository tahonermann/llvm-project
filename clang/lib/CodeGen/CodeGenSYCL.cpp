//===--------- CodeGenSYCL.cpp - Code for SYCL kernel generation ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This contains code required for SYCL kernel generation.
//
//===----------------------------------------------------------------------===//

#include "CodeGenFunction.h"
#include "CodeGenModule.h"

using namespace clang;
using namespace CodeGen;

static void SetSYCLKernelAttributes(llvm::Function *Fn,
                                    const CGFunctionInfo &FnInfo,
                                    CodeGenFunction &CGF) {
  Fn->setDoesNotRecurse();
  if (CGF.checkIfFunctionMustProgress())
    Fn->addFnAttr(llvm::Attribute::MustProgress);
}

static CanQualType GetKernelNameType(const FunctionDecl *KernelEntryPointFn,
                                     ASTContext &Ctx) {
  const auto *KernelEntryPointAttr =
      KernelEntryPointFn->getAttr<SYCLKernelEntryPointAttr>();
  assert(KernelEntryPointAttr && "Missing sycl_kernel_entry_point attribute");
  CanQualType KernelNameType =
      Ctx.getCanonicalType(KernelEntryPointAttr->getKernelName());
  return KernelNameType;
}

static const SYCLKernelInfo *
GetKernelInfo(const FunctionDecl *KernelEntryPointFn, ASTContext &Ctx) {
  CanQualType KernelNameType = GetKernelNameType(KernelEntryPointFn, Ctx);
  const SYCLKernelInfo *KernelInfo = Ctx.findSYCLKernelInfo(KernelNameType);
  assert(KernelInfo && "Type does not correspond to a kernel name");
  return KernelInfo;
}

void CodeGenModule::EmitSYCLKernelCaller(const FunctionDecl *KernelEntryPointFn,
                                         ASTContext &Ctx) {
  assert(KernelEntryPointFn->getAttr<SYCLKernelEntryPointAttr>() &&
         "Missing sycl_kernel_entry_point attribute");
  assert(!KernelEntryPointFn->getAttr<SYCLKernelEntryPointAttr>()
              ->isInvalidAttr() &&
         "sycl_kernel_entry_point attribute is invalid");

  SYCLKernelCallStmt *KernelCallStmt =
      dyn_cast<SYCLKernelCallStmt>(KernelEntryPointFn->getBody());
  assert(KernelCallStmt && "SYCLKernelCallStmt must exist");

  // Build Kernel Arguments from OutlinedFunctionDecl
  FunctionArgList Args;
  const OutlinedFunctionDecl *OutlinedFnDecl =
      KernelCallStmt->getOutlinedFunctionDecl();
  Args.append(OutlinedFnDecl->param_begin(), OutlinedFnDecl->param_end());

  // Compute the function info and LLVM Type
  const CGFunctionInfo &FnInfo =
      getTypes().arrangeSYCLKernelCallerDeclaration(Ctx.VoidTy, Args);
  llvm::FunctionType *FnTy = getTypes().GetFunctionType(FnInfo);

  // Retrieve the generated name for the SYCL kernel caller function
  const SYCLKernelInfo *KernelInfo = GetKernelInfo(KernelEntryPointFn, Ctx);
  auto *Fn = llvm::Function::Create(FnTy, llvm::Function::ExternalLinkage,
                                    KernelInfo->GetKernelName(), &getModule());

  // Emit the SYCL kernel caller function
  CodeGenFunction CGF(*this);
  SetLLVMFunctionAttributes(GlobalDecl(), FnInfo, Fn, false);
  SetSYCLKernelAttributes(Fn, FnInfo, CGF);
  CGF.StartFunction(GlobalDecl(), Ctx.VoidTy, Fn, FnInfo, Args,
                    SourceLocation(), SourceLocation(), OutlinedFnDecl);
  CGF.EmitFunctionBody(OutlinedFnDecl->getBody());
  setDSOLocal(Fn);
  SetLLVMFunctionAttributesForDefinition(cast<Decl>(OutlinedFnDecl), Fn);
  CGF.FinishFunction();
}

void CodeGenModule::AddSYCLKernelNameSymbol(CanQualType KernelNameType,
                                            llvm::GlobalVariable *GV) {
  SYCLKernelNameSymbols[KernelNameType] = GV;
}

llvm::GlobalVariable *
CodeGenModule::GetSYCLKernelNameSymbol(CanQualType KernelNameType) {
  auto it = SYCLKernelNameSymbols.find(KernelNameType);
  if (it != SYCLKernelNameSymbols.end())
    return it->second;
  return nullptr;
}

void CodeGenModule::InitSYCLKernelInfoSymbolsForBuiltins(
    const FunctionDecl *KernelEntryPointFn, ASTContext &Ctx) {
  CanQualType KernelNameType = GetKernelNameType(KernelEntryPointFn, Ctx);
  llvm::GlobalVariable *GV = GetSYCLKernelNameSymbol(KernelNameType);
  if (GV && !GV->hasInitializer()) {
    const SYCLKernelInfo *KernelInfo = GetKernelInfo(KernelEntryPointFn, Ctx);
    ConstantAddress KernelNameStrConstantAddr =
        GetAddrOfConstantCString(KernelInfo->GetKernelName(), "");
    llvm::Constant *KernelNameStr = KernelNameStrConstantAddr.getPointer();
    // FIXME: It is unclear to me whether the right API here is
    // replaceInitializer or setInitializer. Unfortunately since the branch I am
    // working on is outdated, my workspace does not have replaceInitializer
    // Hopefully the person continuing to work on builtins can check this out.
    GV->setInitializer(KernelNameStr);
    GV->setLinkage(llvm::GlobalValue::PrivateLinkage);
  }
}
