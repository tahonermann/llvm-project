// RUN: %clang_cc1 -x c -ast-dump -fexperimental-decimal-floating-point %s | FileCheck %s --strict-whitespace

_Decimal32 x;
_Decimal64 y;
_Decimal128 z;

//CHECK:      |-VarDecl {{.*}} x '_Decimal32'
//CHECK-NEXT: |-VarDecl {{.*}} y '_Decimal64'
//CHECK-NEXT: |-VarDecl {{.*}} z '_Decimal128'

void f(void) {
  _Decimal32 x = 0.0DF;
  _Decimal32 x2 = 1234.0DF;
  _Decimal32 x3 = 945.75DF;
  _Decimal32 x4 = 1E-6DF; // DEC32_EPSILON
  _Decimal32 x5 = 9.999999E40DF;
  _Decimal32 x6 = 100000.0DF;
  _Decimal32 x7 = 9.999999E96DF; // DEC32_MAX
  _Decimal32 x8 = 1E-95DF; // DEC32_MIN
  _Decimal32 x9 = 0.000001E-95DF; // DEC32_TRUE_MIN
}

//CHECK:      | `-VarDecl {{.*}} x '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 0E+0 
//CHECK:      | `-VarDecl {{.*}} x2 '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 12340E-1 
//CHECK:      | `-VarDecl {{.*}} x3 '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 94575E-2
//CHECK:      | `-VarDecl {{.*}} x4 '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 1E-6
//CHECK:      | `-VarDecl {{.*}} x5 '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 9999999E+34
//CHECK:      | `-VarDecl {{.*}} x6 '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 1000000E-1
//CHECK:      | `-VarDecl {{.*}} x7 '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 9999999E+90 
//CHECK:      | `-VarDecl {{.*}} x8 '_Decimal32'
//CHECK-NEXT: |   `-FloatingLiteral {{.*}} '_Decimal32' 1E-95 
//CHECK:        `-VarDecl {{.*}} x9 '_Decimal32'
//CHECK-NEXT:     `-FloatingLiteral {{.*}} '_Decimal32' 1E-11 
