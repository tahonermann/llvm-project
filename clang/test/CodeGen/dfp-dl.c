// RUN: %clang_cc1 -emit-llvm -triple x86_64 -target-cpu x86-64 -emit-llvm -o - %s | FileCheck %s

// RUN: %clang_cc1 -emit-llvm -triple x86_64 -target-cpu x86-64 -emit-llvm -fexperimental-decimal-floating-point -o - %s | FileCheck %s --check-prefixes=DFP

// CHECK: target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
// DFP: target datalayout = "e-d:bid-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
void foo() {
}  
