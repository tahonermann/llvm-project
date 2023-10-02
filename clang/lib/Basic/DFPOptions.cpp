//===--- DFPOptions.cpp -----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/DFPOptions.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include <utility>

namespace clang {

const char* getDFPLibImplOptionSpelling(DFPLibImpl LibImpl) {
  // This map of enumerators to names must match the option arguments and enum
  // marshalling defined for the '-dfplib=' driver option.
  switch (LibImpl) {
  case DFPLibImpl::Default:
    return "default";
  case DFPLibImpl::None:
    return "none";
  case DFPLibImpl::CompilerRT:
    return "compiler-rt";
  case DFPLibImpl::Libgcc:
    return "libgcc";
  case DFPLibImpl::Libgcc_BID:
    return "libgcc:bid";
  case DFPLibImpl::Libgcc_DPD:
    return "libgcc:dpd";
  }
  llvm_unreachable("Unknown DFPLibImpl");
}

std::optional<DFPLibImpl> getDFPLibImplFromOptionValue(llvm::StringRef Value) {
  // This map of names to enumerators must match the option arguments and enum
  // marshalling defined for the '-dfplib=' driver option.
  auto P = llvm::StringSwitch<std::pair<bool, DFPLibImpl>>(Value)
      .Case("default",     {true,  DFPLibImpl::Default})
      .Case("none",        {true,  DFPLibImpl::None})
      .Case("compiler-rt", {true,  DFPLibImpl::CompilerRT})
      .Case("libgcc",      {true,  DFPLibImpl::Libgcc})
      .Case("libgcc:bid",  {true,  DFPLibImpl::Libgcc_BID})
      .Case("libgcc:dpd",  {true,  DFPLibImpl::Libgcc_DPD})
      .Default(            {false, DFPLibImpl::Default});
  if (P.first)
    return P.second;
  return {};
}

} // end namespace clang
