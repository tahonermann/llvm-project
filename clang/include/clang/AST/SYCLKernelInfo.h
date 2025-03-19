//===--- SYCLKernelInfo.h --- Information about SYCL kernels --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file declares types used to describe SYCL kernels.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_SYCLKERNELINFO_H
#define LLVM_CLANG_AST_SYCLKERNELINFO_H

#include "clang/AST/CanonicalType.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

namespace clang {

enum class kernel_param_kind_t : int {
  kind_accessor = 0,
  kind_std_layout = 1,
  kind_sampler = 2,
  kind_pointer = 3,
  kind_specialization_constants_buffer = 4,
  kind_stream = 5,
  kind_invalid = 0xf, // not a valid kernel kind
};

class SYCLKernelInfo {
public:
  SYCLKernelInfo(CanQualType KernelNameType,
                 const FunctionDecl *KernelEntryPointDecl,
                 const std::string &KernelName)
      : KernelNameType(KernelNameType),
        KernelEntryPointDecl(KernelEntryPointDecl),
	KernelName(KernelName) {}

  CanQualType getKernelNameType() const { return KernelNameType; }

  const FunctionDecl *getKernelEntryPointDecl() const {
    return KernelEntryPointDecl;
  }

  const std::string& GetKernelName() const {
    return KernelName;
  }

  size_t GetParamCount() const { return Params.size(); }

  void addParamDesc(kernel_param_kind_t Kind, QualType Ty) {
    KernelParamDesc PD;
    PD.Kind = Kind;
    PD.Type = Ty;
    Params.push_back(PD);
  }

  const kernel_param_kind_t &GetParamKind(int i) const {
    return Params[i].Kind;
  }

  const QualType &GetParamTy(int i) const { return Params[i].Type; }

private:
  // Kernel caller function parameter descriptor.
  struct KernelParamDesc {
    kernel_param_kind_t Kind = kernel_param_kind_t::kind_invalid;
    QualType Type;
    KernelParamDesc() = default;
  };

  CanQualType KernelNameType;
  const FunctionDecl *KernelEntryPointDecl;
  std::string KernelName;
  SmallVector<KernelParamDesc, 8> Params;
};

} // namespace clang

#endif // LLVM_CLANG_AST_SYCLKERNELINFO_H
