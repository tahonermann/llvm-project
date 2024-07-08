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
#include "clang/AST/Type.h"

namespace clang {

class SYCLKernelInfo {
public:
  SYCLKernelInfo(
      QualType KernelNameType,
      const std::string &KernelName)
  :
      KernelNameType(KernelNameType),
      KernelName(KernelName)
  {}

  QualType GetKernelNameType() const {
    return KernelNameType;
  }

  const std::string& GetKernelName() const {
    return KernelName;
  }

private:
  QualType KernelNameType;
  std::string KernelName;
};

} // namespace clang

#endif // LLVM_CLANG_AST_SYCLKERNELINFO_H
