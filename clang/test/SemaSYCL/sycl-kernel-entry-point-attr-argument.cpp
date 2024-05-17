// RUN: %clang_cc1 -std=c++17 -fsyntax-only -fsycl-is-device -verify %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -fsycl-is-device -verify %s

// These tests validate proper handling of a sycl_kernel_entry_point attribute
// argument list. The single argument is required to denote a class type that
// meets the requirements of a SYCL kernel name as described in section 5.2,
// "Naming of kernels", of the SYCL 2020 specification.

// Entities used to validate kernel name arguments.
struct S; // #S-decl
typedef S TA1;
using TA2 = S;
template<int> struct ST; // #ST-decl
template<int N> using TTA = ST<N>; // #TTA-decl
union U;
enum E {
  e
};
enum OE : int;
#if __cplusplus >= 202002L
template<typename> concept C = true;
#endif
namespace NS {
  struct NSS;
}
namespace {
  struct UNSS;
}
struct S2 {
  struct MS;
};
struct {} s;
void f();


////////////////////////////////////////////////////////////////////////////////
// Valid declarations.
////////////////////////////////////////////////////////////////////////////////
[[clang::sycl_kernel_entry_point(S)]] void ok1();
[[clang::sycl_kernel_entry_point(TA1)]] void ok2();
[[clang::sycl_kernel_entry_point(TA2)]] void ok3();
[[clang::sycl_kernel_entry_point(ST<1>)]] void ok4();
[[clang::sycl_kernel_entry_point(TTA<2>)]] void ok5();
[[clang::sycl_kernel_entry_point(NS::NSS)]] void ok6();
[[clang::sycl_kernel_entry_point(UNSS)]] void ok7();
[[clang::sycl_kernel_entry_point(decltype(s))]] void ok8();
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok9();
template<int, typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok10();
template<typename T>
[[clang::sycl_kernel_entry_point(T)]] void ok11() {}
template<>
[[clang::sycl_kernel_entry_point(NS::NSS)]] void ok11<NS::NSS>() {}
void ok() {
  ok9<struct LS1>();
  ok10<1, struct LS2>();
}


////////////////////////////////////////////////////////////////////////////////
// Invalid declarations.
////////////////////////////////////////////////////////////////////////////////

// expected-error@+1 {{'sycl_kernel_entry_point' attribute takes one argument}}
[[clang::sycl_kernel_entry_point]] void bad1();

// expected-error@+1 {{'sycl_kernel_entry_point' attribute takes one argument}}
[[clang::sycl_kernel_entry_point()]] void bad2();

// expected-error@+2 {{expected ')'}}
// expected-error@+1 {{expected ']'}}
[[clang::sycl_kernel_entry_point(S,)]] void bad3();

// expected-error@+3 {{expected ')'}}
// expected-error@+2 {{expected ','}}
// expected-warning@+1 {{unknown attribute 'X' ignored}}
[[clang::sycl_kernel_entry_point(S, X)]] void bad4();

// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(1)]] void bad5();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(int)]] void bad6();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(int())]] void bad7();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(int(*)())]] void bad8();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(int(&)())]] void bad9();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(decltype(nullptr))]] void bad10();

// expected-error@+1 {{unknown type name 'f'}}
[[clang::sycl_kernel_entry_point(f)]] void bad11();

// expected-error@+2 {{use of class template 'ST' requires template arguments; argument deduction not allowed here}}
// expected-note@#ST-decl {{template is declared here}}
[[clang::sycl_kernel_entry_point(ST)]] void bad12();

// expected-error@+2 {{use of alias template 'TTA' requires template arguments; argument deduction not allowed here}}
// expected-note@#TTA-decl {{template is declared here}}
[[clang::sycl_kernel_entry_point(TTA)]] void bad13();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(U)]] void bad14();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(E)]] void bad15();

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a class type}}
[[clang::sycl_kernel_entry_point(OE)]] void bad16();

// expected-error@+1 {{unknown type name 'e'}}
[[clang::sycl_kernel_entry_point(e)]] void bad17();

#if __cplusplus >= 202002L
// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(C)]] void bad18();
// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(C<int>)]] void bad19();
#endif

// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute argument must be a forward declarable class type}}
[[clang::sycl_kernel_entry_point(S2::MS)]] void bad20();

// FIXME-expected-error@+3 {{'sycl_kernel_entry_point' attribute argument must be a forward declarable class type}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::MS)]] void bad21() {}
template void bad21<S2>();

// FIXME: C++23 [temp.expl.spec]p12 states:
// FIXME:   ... Similarly, attributes appearing in the declaration of a template
// FIXME:   have no effect on an explicit specialization of that template.
// FIXME: Clang currently instantiates and propagates attributes from a function
// FIXME: template to its explicit specializations resulting in the following
// FIXME: spurious error.
// expected-error@+4 {{incomplete type 'S' named in nested name specifier}}
// expected-note@+5 {{in instantiation of function template specialization 'bad22<S>' requested here}}
// expected-note@#S-decl {{forward declaration of 'S'}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::not_found)]] void bad22() {}
template<>
void bad22<S>() {}

template<typename T>
[[clang::sycl_kernel_entry_point(T)]] void bad23();
void f() {
  // FIXME-expected-error@+2 {{'sycl_kernel_entry_point' attribute argument must be a forward declarable class type}}
  struct LS;
  bad23<LS>();
}
