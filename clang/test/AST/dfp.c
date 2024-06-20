// RUN: %clang_cc1 -x c -ast-dump %s | FileCheck %s --strict-whitespace

_Decimal32 x;
_Decimal64 y;
_Decimal128 z;

//CHECK:      |-VarDecl {{.*}} x '_Decimal32'
//CHECK-NEXT: |-VarDecl {{.*}} y '_Decimal64'
//CHECK-NEXT: `-VarDecl {{.*}} z '_Decimal128'
