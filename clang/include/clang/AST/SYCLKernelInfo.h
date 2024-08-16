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

#include <string>
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

namespace clang {

class SYCLKernelInfo {
public:
  // FIXME: Added full enum to match the library implementation.
  // Why does kind_first and kind_last exist?
  enum kernel_param_kind_t {
    kind_first,
    kind_accessor = kind_first,
    kind_std_layout,
    kind_sampler,
    kind_pointer,
    kind_specialization_constants_buffer,
    kind_stream,
    kind_last = kind_stream
  };

public:
  SYCLKernelInfo(CanQualType KernelNameType,
                 const FunctionDecl *KernelEntryPointDecl,
                 const std::string &KernelName)
      : KernelNameType(KernelNameType),
        KernelEntryPointDecl(KernelEntryPointDecl), KernelName(KernelName) {}

  CanQualType GetKernelNameType() const {
    return KernelNameType;
  }

  const FunctionDecl* GetKernelEntryPointDecl() const {
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
    kernel_param_kind_t Kind = kind_last;
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
