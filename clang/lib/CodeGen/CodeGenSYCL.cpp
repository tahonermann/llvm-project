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
  Fn->setCallingConv(llvm::CallingConv::SPIR_KERNEL);
  if (CGF.checkIfFunctionMustProgress())
    Fn->addFnAttr(llvm::Attribute::MustProgress);
}

void CodeGenModule::EmitSYCLKernelCaller(const FunctionDecl *KernelEntryPointFn,
                                         ASTContext &Ctx) {

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
  const auto *KernelEntryPointAttr =
      KernelEntryPointFn->getAttr<SYCLKernelEntryPointAttr>();
  CanQualType KernelNameType =
      Ctx.getCanonicalType(KernelEntryPointAttr->getKernelName());
  const SYCLKernelInfo *KernelInfo = Ctx.findSYCLKernelInfo(KernelNameType);
  assert(KernelInfo && "Type does not correspond to a kernel name");
  auto *Fn = llvm::Function::Create(FnTy, llvm::GlobalVariable::ExternalLinkage,
                                    KernelInfo->GetKernelName(), &getModule());

  // Emit the SYCL kernel caller function
  CodeGenFunction CGF(*this);
  SetLLVMFunctionAttributes(GlobalDecl(), FnInfo, Fn, false);
  SetSYCLKernelAttributes(Fn, FnInfo, CGF);
  CGF.StartFunction(GlobalDecl(), Ctx.VoidTy, Fn, FnInfo, Args,
                    SourceLocation(), SourceLocation());
  CGF.EmitFunctionBody(OutlinedFnDecl->getBody());
  setDSOLocal(Fn);
  SetLLVMFunctionAttributesForDefinition(cast<Decl>(OutlinedFnDecl), Fn);
  CGF.FinishFunction();
}
