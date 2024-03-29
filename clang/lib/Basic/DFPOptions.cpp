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

const char* getSpellingOfDFPLibOption(DFPLibOption DFPLibOpt) {
  // This map of enumerators to names must match the option arguments and enum
  // marshalling defined for the '-dfplib=' driver option.
  switch (DFPLibOpt) {
  case DFPLibOption::Default:
    return "default";
  case DFPLibOption::None:
    return "none";
  case DFPLibOption::CompilerRT:
    return "compiler-rt";
  case DFPLibOption::Libgcc:
    return "libgcc";
  case DFPLibOption::Libgcc_BID:
    return "libgcc:bid";
  case DFPLibOption::Libgcc_DPD:
    return "libgcc:dpd";
  }
  llvm_unreachable("Unknown DFPLibOption");
}

std::optional<DFPLibOption> getDFPLibOptionFromSpelling(llvm::StringRef Value) {
  // This map of names to enumerators must match the option arguments and enum
  // marshalling defined for the '-dfplib=' driver option.
  auto P = llvm::StringSwitch<std::pair<bool, DFPLibOption>>(Value)
      .Case("default",     {true,  DFPLibOption::Default})
      .Case("none",        {true,  DFPLibOption::None})
      .Case("compiler-rt", {true,  DFPLibOption::CompilerRT})
      .Case("libgcc",      {true,  DFPLibOption::Libgcc})
      .Case("libgcc:bid",  {true,  DFPLibOption::Libgcc_BID})
      .Case("libgcc:dpd",  {true,  DFPLibOption::Libgcc_DPD})
      .Default(            {false, DFPLibOption::Default});
  if (P.first)
    return P.second;
  return {};
}

} // end namespace clang
