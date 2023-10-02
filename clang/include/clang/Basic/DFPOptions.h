//===--- DFPOptions.h -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_DFPOPTIONS_H
#define LLVM_CLANG_BASIC_DFPOPTIONS_H

#include "llvm/ADT/StringRef.h"
#include <cstdint>
#include <optional>

namespace clang {

// ISO/IEC TS 18661-2, ISO/IEC TR 24733, and C23 decimal floating-point.
// Support for decimal floating-point (DFP) operations generally requires use
// of a run-time library to perform DFP operations. There are several such
// run-time libraries available as well as at least two distinct encodings
// used to store DFP values (BID and DPD). Some libraries, libgcc for example,
// provide multiple implementations corresponding to each supported encoding.
// A single platform may support multiple DFP libraries and potentially
// multiple implementations within a single library. This enumeration provides
// enumerators for each known implementation.
enum class DFPLibImpl : uint8_t {
  Default,    // The default run-time library for the target.
  None,       // No run-time library; for targets with native hardware support.
  CompilerRT, // compiler-rt.
  Libgcc,     // libgcc with platform dependent default encoding.
  Libgcc_BID, // libgcc with BID (Binary Integer Decimal) encoding.
  Libgcc_DPD, // libgcc with DPD (Densely Packed Decimal) encoding.
};

// Returns the '-dfplib' option argument corresponding to the specified DFP
// run-time implementation.
const char* getDFPLibImplOptionSpelling(DFPLibImpl LibImpl);

// Returns the DFP run-time implementation corresponding to the specified
// '-dfplib' option argument. If the option argument is not recognized, a
// disengaged optional value is returned.
std::optional<DFPLibImpl> getDFPLibImplFromOptionValue(llvm::StringRef Value);

} // end namespace clang

#endif
