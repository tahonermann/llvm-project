// RUN: %clang_cc1 -std=c++17 -fsyntax-only -fsycl-is-device -verify %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -fsycl-is-device -verify %s

// These tests validate appertainment for the sycl_kernel_entry_point attribute.

// A unique kernel name type is required for each declared kernel entry point.
template<int> struct KN;


////////////////////////////////////////////////////////////////////////////////
// Valid declarations.
////////////////////////////////////////////////////////////////////////////////

// Function declaration with GNU attribute spelling
__attribute__((sycl_kernel_entry_point(KN<1>)))
void ok1();

// Function declaration with Clang attribute spelling.
[[clang::sycl_kernel_entry_point(KN<1>)]]
void ok2();

// Function definition.
[[clang::sycl_kernel_entry_point(KN<2>)]]
void ok3() {}

// Function template definition.
template<typename KNT, typename T>
[[clang::sycl_kernel_entry_point(KNT)]]
void ok4(T) {}

// Function template explicit specialization.
template<>
[[clang::sycl_kernel_entry_point(KN<3>)]]
void ok4<KN<3>>(int) {}

// Function template explicit instantiation.
template void ok4<KN<4>, long>(long);

namespace NS {
// Function declaration at namespace scope.
[[clang::sycl_kernel_entry_point(KN<5>)]]
void ok5();
}

struct S1 {
  // Static member function declaration.
  [[clang::sycl_kernel_entry_point(KN<6>)]]
  static void ok6();
};

// The sycl_kernel_entry_point attribute must match across declarations and must
// be present on the first declaration.
[[clang::sycl_kernel_entry_point(KN<7>)]]
void ok7();
[[clang::sycl_kernel_entry_point(KN<7>)]]
void ok7();
[[clang::sycl_kernel_entry_point(KN<8>)]]
void ok8();
void ok8() {}


////////////////////////////////////////////////////////////////////////////////
// Invalid declarations.
////////////////////////////////////////////////////////////////////////////////

template<int> struct BADKN;

struct S2 {
  // Non-static data member function declaration.
  // expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
  [[clang::sycl_kernel_entry_point(BADKN<1>)]]
  int bad1;

  // Static data member function declaration.
  // expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
  [[clang::sycl_kernel_entry_point(BADKN<2>)]]
  int bad2;

  // Non-static member function declaration.
  // FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to static member functions}}
  [[clang::sycl_kernel_entry_point(BADKN<3>)]]
  void bad3();
};

// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
namespace bad4 [[clang::sycl_kernel_entry_point(BADKN<4>)]] {}

#if __cplusplus >= 202002L
// expected-error@+2 {{'sycl_kernel_entry_point' attribute only applies to functions}}
template<typename>
concept bad5 [[clang::sycl_kernel_entry_point(BADKN<5>)]] = true;
#endif

// Type alias declarations.
// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
typedef void bad6 [[clang::sycl_kernel_entry_point(BADKN<6>)]] ();
// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
using bad7 [[clang::sycl_kernel_entry_point(BADKN<7>)]] = void();
// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
using bad8 [[clang::sycl_kernel_entry_point(BADKN<8>)]] = int;
// expected-error@+1 {{'sycl_kernel_entry_point' attribute cannot be applied to types}}
using bad9 = int [[clang::sycl_kernel_entry_point(BADKN<9>)]];
// expected-error@+1 {{'sycl_kernel_entry_point' attribute cannot be applied to types}}
using bad10 = int() [[clang::sycl_kernel_entry_point(BADKN<10>)]];

// Variable declaration.
// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
[[clang::sycl_kernel_entry_point(BADKN<11>)]]
int bad11;

// Class declaration.
// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
struct [[clang::sycl_kernel_entry_point(BADKN<12>)]] bad12;

// Enumeration declaration.
// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
enum [[clang::sycl_kernel_entry_point(BADKN<13>)]] bad13 {};

// Enumerator.
// expected-error@+2 {{'sycl_kernel_entry_point' attribute only applies to functions}}
enum {
  bad14 [[clang::sycl_kernel_entry_point(BADKN<14>)]]
};

// Attribute missing on the first declaration.
// FIXME-expected-error@+3 {{'sycl_kernel_entry_point' attribute must be present on the first declaration}}
void bad15();
[[clang::sycl_kernel_entry_point(BADKN<15>)]]
void bad15();

// Mismatched attributes.
// FIXME-expected-error@+4 {{'sycl_kernel_entry_point' attribute must be consistent across declarations}}
[[clang::sycl_kernel_entry_point(BADKN<16>)]]
void bad16();
[[clang::sycl_kernel_entry_point(BADKN<16>)]]
void bad16();

// The function must return void.
// FIXME-expected-error@+4 {{function with 'sycl_kernel_entry_point' attribute must have a 'void' return type}}
[[clang::sycl_kernel_entry_point(BADKN<17>)]]
void bad17();

// Function parameters.
// expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
void bad18(void (fp [[clang::sycl_kernel_entry_point(BADKN<18>)]])());

// Function template parameters.
// FIXME-expected-error@+1 {{'sycl_kernel_entry_point' attribute only applies to functions}}
template<void (fp [[clang::sycl_kernel_entry_point(BADKN<18>)]])()>
void bad19();
