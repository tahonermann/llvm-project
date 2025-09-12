//===- StmtSYCL.h - Classes for SYCL kernel calls ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file defines SYCL AST classes used to represent calls to SYCL kernels.
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_STMTSYCL_H
#define LLVM_CLANG_AST_STMTSYCL_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {

//===----------------------------------------------------------------------===//
// AST classes for SYCL kernel calls.
//===----------------------------------------------------------------------===//

/// SYCLKernelCallStmt represents the transformation that is applied to the body
/// of a function declared with the sycl_kernel_entry_point attribute. The body
/// of such a function specifies the statements to be executed on a SYCL device
/// to invoke a SYCL kernel with a particular set of kernel arguments. The
/// SYCLKernelCallStmt associates an original statement (the compound statement
/// that is the function body) with a kernel launch statement to execute on a
/// SYCL host and an OutlinedFunctionDecl that holds the kernel parameters and
/// the transformed body to execute on a SYCL device. During code generation,
/// the OutlinedFunctionDecl is used to emit an offload kernel entry point
/// suitable for invocation from a SYCL library implementation.
class SYCLKernelCallStmt : public Stmt {
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

private:
  Stmt *OriginalStmt = nullptr;
  Stmt *KernelLaunchStmt = nullptr;
  OutlinedFunctionDecl *OFDecl = nullptr;

public:
  /// Construct a SYCL kernel call statement.
  SYCLKernelCallStmt(CompoundStmt *CS, Stmt *S, OutlinedFunctionDecl *OFD)
      : Stmt(SYCLKernelCallStmtClass), OriginalStmt(CS), KernelLaunchStmt(S),
        OFDecl(OFD) {}

  /// Construct an empty SYCL kernel call statement.
  SYCLKernelCallStmt(EmptyShell Empty) : Stmt(SYCLKernelCallStmtClass, Empty) {}

  /// Retrieve the original statement.
  CompoundStmt *getOriginalStmt() { return cast<CompoundStmt>(OriginalStmt); }
  const CompoundStmt *getOriginalStmt() const {
    return cast<CompoundStmt>(OriginalStmt);
  }

  /// Set the original statement.
  void setOriginalStmt(CompoundStmt *CS) { OriginalStmt = CS; }

  /// Retrieve the kernel launch statement.
  Stmt *getKernelLaunchStmt() { return KernelLaunchStmt; }
  const Stmt *getKernelLaunchStmt() const { return KernelLaunchStmt; }

  /// Set the kernel launch statement.
  void setKernelLaunchStmt(Stmt *S) { KernelLaunchStmt = S; }

  /// Retrieve the outlined function declaration.
  OutlinedFunctionDecl *getOutlinedFunctionDecl() { return OFDecl; }
  const OutlinedFunctionDecl *getOutlinedFunctionDecl() const { return OFDecl; }

  /// Set the outlined function declaration.
  void setOutlinedFunctionDecl(OutlinedFunctionDecl *OFD) { OFDecl = OFD; }

  SourceLocation getBeginLoc() const LLVM_READONLY {
    return getOriginalStmt()->getBeginLoc();
  }

  SourceLocation getEndLoc() const LLVM_READONLY {
    return getOriginalStmt()->getEndLoc();
  }

  SourceRange getSourceRange() const LLVM_READONLY {
    return getOriginalStmt()->getSourceRange();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == SYCLKernelCallStmtClass;
  }

  child_range children() {
    return child_range(&OriginalStmt, &OriginalStmt + 1);
  }

  const_child_range children() const {
    return const_child_range(&OriginalStmt, &OriginalStmt + 1);
  }
};

// UnresolvedSYCLKernelLaunchExpr a call to the kernel launch function for a
// kernel that has not been instantiated yet. This Expr should be transformed to
// a CallExpr once the kernel and its name is known.
class UnresolvedSYCLKernelLaunchExpr : public Expr {
  friend class ASTStmtReader;
  QualType KernelNameType;
  SourceLocation Loc;
  // This is either UnresolvedLookupExpr or UnresolvedMemberExpr.
  Expr *IdExpr = nullptr;
  UnresolvedSYCLKernelLaunchExpr(SourceLocation L, QualType ExprTy,
                                 QualType KNT, Expr *_IdExpr)
      : Expr(UnresolvedSYCLKernelLaunchExprClass, ExprTy, VK_PRValue,
             OK_Ordinary),
        KernelNameType(KNT), Loc(L), IdExpr(_IdExpr) {
    setDependence(computeDependence(this));
  }

  void setKernelNameType(QualType T) { KernelNameType = T; }
  void setLocation(SourceLocation L) { Loc = L; }
  void setIdExpr(Expr *Id) { IdExpr = Id; }

public:
  static UnresolvedSYCLKernelLaunchExpr *Create(const ASTContext &C,
                                                QualType ExprTy, QualType KNT,
                                                SourceLocation Loc,
                                                Expr *IdExpr) {
    return new (C) UnresolvedSYCLKernelLaunchExpr(Loc, ExprTy, KNT, IdExpr);
  }

  static UnresolvedSYCLKernelLaunchExpr *CreateEmpty(const ASTContext &C) {
    return new (C)
        UnresolvedSYCLKernelLaunchExpr({}, C.VoidTy, C.VoidTy, nullptr);
  }

  QualType getKernelNameType() const { return KernelNameType; }
  Expr *getIdExpr() const { return IdExpr; }
  SourceLocation getLocation() const { return Loc; }

  SourceLocation getBeginLoc() const { return Loc; }
  SourceLocation getEndLoc() const { return Loc; }
  static bool classof(const Stmt *T) {
    return T->getStmtClass() == UnresolvedSYCLKernelLaunchExprClass;
  }
  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  const_child_range children() const {
    return const_child_range(const_child_iterator(), const_child_iterator());
  }
};

} // end namespace clang

#endif // LLVM_CLANG_AST_STMTSYCL_H
