// RUN: %clang_cc1 -triple x86_64-linux-gnu -std=c++17 -fsyntax-only -fsycl-is-device -verify %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -std=c++20 -fsyntax-only -fsycl-is-device -verify %s

// These tests validate that substitution failures that occur in an argument of
// a sycl_kernel_entry_point attribute occur in the immediate context and
// therefore influence overload resolution.

// FIXME: C++23 [temp.expl.spec]p12 states:
// FIXME:   ... Similarly, attributes appearing in the declaration of a template
// FIXME:   have no effect on an explicit specialization of that template.
// FIXME: Clang currently instantiates and propagates attributes from a function
// FIXME: template to its explicit specializations resulting in the following
// FIXME: spurious error.
struct S1; // #S1-decl
// expected-error@+4 {{incomplete type 'S1' named in nested name specifier}}
// expected-note@+5 {{in instantiation of function template specialization 'ok1<S1>' requested here}}
// expected-note@#S1-decl {{forward declaration of 'S1'}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::invalid)]] void ok1() {}
template<>
void ok1<S1>() {}

// FIXME: A substitution failure that occurs during instantiation of an
// FIXME: attribute specifier sequence should make the declaration invalid for
// FIXME: overload resolution but should not elicit a diagnostic (assuming
// FIXME: another overload candidate is selected).
struct S2; // #S2-decl
// expected-error@+4 {{incomplete type 'S2' named in nested name specifier}}
// expected-note@+7 {{in instantiation of function template specialization 'ok2<S2>' requested here}}
// expected-note@#S2-decl {{forward declaration of 'S2'}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::invalid)]] void ok2(int) {}
template<typename T>
[[clang::sycl_kernel_entry_point(T)]] void ok2(long) {}
void test_ok2() {
  ok2<S2>(2); // ok2(int) would be a better match, but SFINAE should result
              // in overload resolution selecting ok2(long).
}
