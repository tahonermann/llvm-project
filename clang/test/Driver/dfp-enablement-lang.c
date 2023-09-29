// This test is intended to validate whether decimal floating-point (DFP)
// extensions are enabled based on language standard and to ensure a proper
// diagnostic is issued for any use of DFP features otherwise.

// RUN: %clang -target x86_64-unknown-linux-gnu -std=c17 -fsyntax-only -Xclang -verify=dfp-off,c-dfp-off %s
// RUN: %clang -target x86_64-unknown-linux-gnu -std=c17 -fsyntax-only -fexperimental-decimal-floating-point -Xclang -verify %s
// FIXME: Remove -fexperimental-decimal-floating-point once -std=c23 implies DFP enablement.
// RUN: %clang -target x86_64-unknown-linux-gnu -std=c23 -fsyntax-only -fexperimental-decimal-floating-point -Xclang -verify %s
// RUN: %clang -target x86_64-unknown-linux-gnu -std=c23 -fsyntax-only -fno-experimental-decimal-floating-point -Xclang -verify=dfp-off,c-dfp-off %s
// RUN: %clang -target x86_64-unknown-linux-gnu -x c++ -std=c++23 -fsyntax-only -Xclang -verify=cxx,dfp-off %s
// RUN: %clang -target x86_64-unknown-linux-gnu -x c++ -std=c++23 -fsyntax-only -fexperimental-decimal-floating-point -Xclang -verify=cxx %s
// FIXME: Remove -fexperimental-decimal-floating-point once -std=c++2c (or later) implies DFP enablement.
// RUN: %clang -target x86_64-unknown-linux-gnu -x c++ -std=c++2c -fsyntax-only -fexperimental-decimal-floating-point -Xclang -verify=cxx %s
// RUN: %clang -target x86_64-unknown-linux-gnu -x c++ -std=c++2c -fsyntax-only -fno-experimental-decimal-floating-point -Xclang -verify=cxx,dfp-off %s

// expected-no-diagnostics

_Decimal32 d32;   // cxx-error {{unknown type name '_Decimal32'}} \
                  // c-dfp-off-error {{decimal floating-point extensions are not enabled}}
_Decimal64 d64;   // cxx-error {{unknown type name '_Decimal64'}} \
                  // c-dfp-off-error {{decimal floating-point extensions are not enabled}}
_Decimal128 d128; // cxx-error {{unknown type name '_Decimal128'}} \
                  // c-dfp-off-error {{decimal floating-point extensions are not enabled}}
