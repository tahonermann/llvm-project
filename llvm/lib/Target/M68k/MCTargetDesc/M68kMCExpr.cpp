//===- M68k specific MC expression classes ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "M68kMCExpr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCValue.h"

using namespace llvm;

const M68kMCExpr *M68kMCExpr::create(const MCExpr *Expr, Spec S,
                                     MCContext &Ctx) {
  return new (Ctx) M68kMCExpr(Expr, S);
}

void M68kMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {}
