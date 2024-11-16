// DEFINE: %{common_opts_x86} = -triple x86_64 -target-cpu x86-64 \
// DEFINE: -emit-llvm -o -

// RUN: %clang_cc1 %{common_opts_x86} %s | FileCheck %s --check-prefix=CHECK_x86

// RUN: %clang_cc1 %{common_opts_x86} -fexperimental-decimal-floating-point %s \
// RUN: | FileCheck %s --check-prefix=DFP_x86

// DEFINE: %{common_opts_ppc} = -triple powerpc-unknown-aix  \
// DEFINE: -target-cpu powerpc -emit-llvm -o -

// RUN: %clang_cc1 %{common_opts_ppc} %s | FileCheck %s \
// RUN: --check-prefixes=CHECK_PPC

// RUN: %clang_cc1 %{common_opts_ppc} -fexperimental-decimal-floating-point %s \
// RUN: | FileCheck %s --check-prefixes=DFP_PPC

// CHECK_x86: target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
// DFP_x86: target datalayout = "e-d:bid-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

// CHECK_PPC: target datalayout = "E-m:a-p:32:32-Fi32-i64:64-n32"
// DFP_PPC: target datalayout = "E-d:bid-m:a-p:32:32-Fi32-i64:64-n32"

void foo() {
}  
