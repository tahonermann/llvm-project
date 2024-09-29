// RUN: %clang_cc1 -triple x86_64-linux-gnu -std=c++17 -fsyntax-only -fsycl-is-device -verify %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -std=c++20 -fsyntax-only -fsycl-is-device -verify %s

// These tests validate proper handling of a sycl_kernel_entry_point attribute
// argument list. The single argument is required to denote a class type that
// meets the requirements of a SYCL kernel name as described in section 5.2,
// "Naming of kernels", of the SYCL 2020 specification.

// Common entities used to validate kernel name arguments.
template<int> struct ST; // #ST-decl
template<int N> using TTA = ST<N>; // #TTA-decl


////////////////////////////////////////////////////////////////////////////////
// Valid declarations.
////////////////////////////////////////////////////////////////////////////////
struct S1;
[[clang::sycl_kernel_entry_point(S1)]] void ok1();

typedef struct {} TA2;
[[clang::sycl_kernel_entry_point(TA2)]] void ok2();

using TA3 = struct {};
[[clang::sycl_kernel_entry_point(TA3)]] void ok3();

[[clang::sycl_kernel_entry_point(ST<4>)]] void ok4();

[[clang::sycl_kernel_entry_point(TTA<5>)]] void ok5();

namespace NS6 {
  struct NSS;
}
[[clang::sycl_kernel_entry_point(NS6::NSS)]] void ok6();

namespace {
  struct UNSS7;
}
[[clang::sycl_kernel_entry_point(UNSS7)]] void ok7();

struct {} s;
[[clang::sycl_kernel_entry_point(decltype(s))]] void ok8();

template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok9();
void test_ok9() {
  ok9<struct LS1>();
}

template<int, typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok10();
void test_ok10() {
  ok10<1, struct LS2>();
}

namespace NS11 {
  struct NSS;
}
template<typename T>
[[clang::sycl_kernel_entry_point(T)]] void ok11() {}
template<>
[[clang::sycl_kernel_entry_point(NS11::NSS)]] void ok11<NS11::NSS>() {}

struct S12;
[[clang::sycl_kernel_entry_point(S12)]] void ok12();
[[clang::sycl_kernel_entry_point(S12)]] void ok12() {}

struct S13;
struct S13;
// expected-warning@+3 {{redundant 'sycl_kernel_entry_point' attribute}}
// expected-note@+1  {{previous attribute is here}}
[[clang::sycl_kernel_entry_point(S13),
  clang::sycl_kernel_entry_point(S13)]]
void ok13();

template<typename T>
[[clang::sycl_kernel_entry_point(T)]] void ok14(T k);
void test_ok14() {
  ok14([]{});
}


////////////////////////////////////////////////////////////////////////////////
// Invalid declarations.
////////////////////////////////////////////////////////////////////////////////

// expected-error@+1 {{'sycl_kernel_entry_point' attribute takes one argument}}
[[clang::sycl_kernel_entry_point]] void bad1();

// expected-error@+1 {{'sycl_kernel_entry_point' attribute takes one argument}}
[[clang::sycl_kernel_entry_point()]] void bad2();

struct B3;
// expected-error@+2 {{expected ')'}}
// expected-error@+1 {{expected ']'}}
[[clang::sycl_kernel_entry_point(B3,)]] void bad3();

struct B4;
// expected-error@+3 {{expected ')'}}
// expected-error@+2 {{expected ','}}
// expected-warning@+1 {{unknown attribute 'X' ignored}}
[[clang::sycl_kernel_entry_point(B4, X)]] void bad4();

// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(1)]] void bad5();

// expected-error@+1 {{'int' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int)]] void bad6();

// expected-error@+1 {{'int ()' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int())]] void bad7();

// expected-error@+1 {{'int (*)()' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int(*)())]] void bad8();

// expected-error@+1 {{'int (&)()' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int(&)())]] void bad9();

// expected-error@+1 {{'decltype(nullptr)' (aka 'std::nullptr_t') is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(decltype(nullptr))]] void bad10();

void f11();
// expected-error@+1 {{unknown type name 'f11'}}
[[clang::sycl_kernel_entry_point(f11)]] void bad11();

// expected-error@+2 {{use of class template 'ST' requires template arguments; argument deduction not allowed here}}
// expected-note@#ST-decl {{template is declared here}}
[[clang::sycl_kernel_entry_point(ST)]] void bad12();

// expected-error@+2 {{use of alias template 'TTA' requires template arguments; argument deduction not allowed here}}
// expected-note@#TTA-decl {{template is declared here}}
[[clang::sycl_kernel_entry_point(TTA)]] void bad13();

union U; // #U-decl
// expected-error@+2 {{'U' is not a valid SYCL kernel name type; a class type is required}}
// expected-note@#U-decl {{'U' declared here}}
[[clang::sycl_kernel_entry_point(U)]] void bad14();

enum E15 {}; // #E15-decl
// expected-error@+2 {{'E15' is not a valid SYCL kernel name type; a class type is required}}
// expected-note@#E15-decl {{'E15' declared here}}
[[clang::sycl_kernel_entry_point(E15)]] void bad15();

enum E16 : int; // #E16-decl
// expected-error@+2 {{'E16' is not a valid SYCL kernel name type; a class type is required}}
// expected-note@#E16-decl {{'E16' declared here}}
[[clang::sycl_kernel_entry_point(E16)]] void bad16();

enum {
  e17
};
// expected-error@+1 {{unknown type name 'e17'}}
[[clang::sycl_kernel_entry_point(e17)]] void bad17();

#if __cplusplus >= 202002L
template<typename> concept C = true;
// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(C)]] void bad18();

// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(C<int>)]] void bad19();
#endif

struct B20 {
  struct MS;
};
// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a forward declarable class type}}
[[clang::sycl_kernel_entry_point(B20::MS)]] void bad20();

struct B21 {
  struct MS;
};
// FIXME-expected-error@+3 {{'sycl_kernel_entry_point' attribute argument must be a forward declarable class type}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::MS)]] void bad21() {}
template void bad21<B21>();

struct B22; // #B22-decl
// FIXME: C++23 [temp.expl.spec]p12 states:
// FIXME:   ... Similarly, attributes appearing in the declaration of a template
// FIXME:   have no effect on an explicit specialization of that template.
// FIXME: Clang currently instantiates and propagates attributes from a function
// FIXME: template to its explicit specializations resulting in the following
// FIXME: spurious error.
// expected-error@+4 {{incomplete type 'B22' named in nested name specifier}}
// expected-note@+5 {{in instantiation of function template specialization 'bad22<B22>' requested here}}
// expected-note@#B22-decl {{forward declaration of 'B22'}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::not_found)]] void bad22() {}
template<>
void bad22<B22>() {}

template<typename T>
[[clang::sycl_kernel_entry_point(T)]] void bad23();
void f23() {
  // FIXME-expected-error@+2 {{'sycl_kernel_entry_point' attribute argument must be a forward declarable class type}}
  struct LS;
  bad23<LS>();
}

struct B24_1;
struct B24_2;
// expected-error@+3 {{'sycl_kernel_entry_point' kernel name argument does not match prior declaration: 'B24_2' vs 'B24_1'}}
// expected-note@+1  {{'bad24' declared here}}
[[clang::sycl_kernel_entry_point(B24_1)]] void bad24();
[[clang::sycl_kernel_entry_point(B24_2)]] void bad24() {}

struct B25_1;
struct B25_2;
// expected-error@+3 {{'sycl_kernel_entry_point' kernel name argument does not match prior declaration: 'B25_2' vs 'B25_1'}}
// expected-note@+1  {{previous attribute is here}}
[[clang::sycl_kernel_entry_point(B25_1),
  clang::sycl_kernel_entry_point(B25_2)]]
void bad25();

struct B26;
// expected-error@+3 {{'sycl_kernel_entry_point' kernel name argument conflicts with a previous declaration}}
// expected-note@+1  {{previous declaration is here}}
[[clang::sycl_kernel_entry_point(B26)]] void bad26_1();
[[clang::sycl_kernel_entry_point(B26)]] void bad26_2();
