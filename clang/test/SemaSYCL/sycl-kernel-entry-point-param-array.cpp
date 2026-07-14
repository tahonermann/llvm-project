// RUN: %clang_cc1 -triple x86_64-linux-gnu -std=c++17 -fsyntax-only -fsycl-is-host -Wno-vla-cxx-extension -verify %s
// RUN: %clang_cc1 -triple spirv64-unknown-unknown -std=c++17 -fsyntax-only -fsycl-is-device -Wno-vla-cxx-extension -verify %s

// Test handling of parameters declared as arrays in functions declared with
// the sycl_kernel_entry_point attribute. This test validates that diagnostics
// issued for implicit calls to sycl_kernel_launch() match those issued for
// matching explicit calls from a control function. This ensures that type
// decay for parameters is correctly handled for the implicit calls.

// FIXME: The same-type static assertions for the lvalue reference to array/VMT
// type cases are arguably incorrect. When generating the kernel entry point
// function, the parameters of the sycl_kernel_entry_point attributed function
// are copied and adjusted to remove top-level references. Parameters of
// reference type are allowed for sycl_kernel_entry_point attributed functions
// so that parameters can be efficiently forwarded to sycl_kernel_launch(),
// but since the SYCL RT has no way to pass arguments by reference to a kernel
// entry point function, the parameter types are adjusted with an expectation
// that the SYCL RT will copy corresponding arguments to the device and provide
// them as by-value arguments for the kernel invocation. However, no adjustments
// are made to the AST for the function body to correct for exposure of the
// formal parameter type via decltype or other reflection-like features.
// Given a declaration like this:
//   struct X { ... };
//   [[clang::syck_kernel_entry_point(KN)]]
//   void skep(const X& x) {
//     decltype(x) lv; // lv has type const X&.
//   }
// the generated kernel entry point function will be something like this:
//   void kep(const X x) {
//     decltype(x) lv; // lv has type const X&.
//   }
// Note that 'lv' will still have type 'const X&' even though decltype(x)
// should yield a type of just 'const X'.


// Polyfill std::is_same type trait for testing.
template<typename T, typename U>
struct is_same {
  static constexpr bool value = false;
};
template<typename T>
struct is_same<T, T> {
  static constexpr bool value = true;
};
template<typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

// Polyfill std::move() implementation for testing.
template<typename T>
auto move(T t) {
  return static_cast<T&&>(t);
}


// A unique kernel name type is required for each declared kernel entry point.
template<int, int = 0> struct KN;

// sycl_kernel_launch that receives arguments by value.
namespace test1 {
  template<typename KN, typename... Ts>
  void sycl_kernel_launch(const char *, Ts...);

  template<typename... Ts>
  void ft(Ts...);

  // Test array decay to pointer; resulting pointer is move eligible.
  [[clang::sycl_kernel_entry_point(KN<1, 1>)]]
  void skep_array(int a[10]) {
    ft(move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }
  void control_array(int a[10]) {
    sycl_kernel_launch<KN<1, 1>>("", move(a));
    ft(move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }

  // Test lvalue reference to array; parameter is not move eligible.
  [[clang::sycl_kernel_entry_point(KN<1, 2>)]]
  void skep_array_ref(int (&a)[10]) {
    ft(a);
    static_assert(is_same_v<decltype(a), int(&)[10]>);
  }
  void control_array_ref(int (&a)[10]) {
    sycl_kernel_launch<KN<1, 2>>("", a);
    ft(a);
    static_assert(is_same_v<decltype(a), int(&)[10]>);
  }

  // Test VMT decay to pointer; resulting pointer is move eligible.
  [[clang::sycl_kernel_entry_point(KN<1, 3>)]]
  void skep_vmt(int n, int a[n]) {
    ft(move(n), move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }
  void control_vmt(int n, int a[n]) {
    sycl_kernel_launch<KN<1, 4>>("", move(n), move(a));
    ft(move(n), move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }

  // Test lvalue reference to VMT; parameter is not move eligible.
  [[clang::sycl_kernel_entry_point(KN<1, 4>)]]
  void skep_vmt_ref(int n, int (&a)[n]) {
    ft(move(n), a);
    // expected-error@+1 {{variably modified type 'decltype(a)' (aka 'int (&)[n]') cannot be used as a template argument}}
    static_assert(is_same_v<decltype(a), int*>);
  }
  void control_vmt_ref(int n, int (&a)[n]) {
    sycl_kernel_launch<KN<1, 4>>("", move(n), a);
    ft(move(n), a);
    // expected-error@+1 {{variably modified type 'decltype(a)' (aka 'int (&)[n]') cannot be used as a template argument}}
    static_assert(is_same_v<decltype(a), int*>);
  }
}

// sycl_kernel_launch that receives arguments by lvalue reference.
namespace test2 {
  template<typename KN, typename... Ts>
  void sycl_kernel_launch(const char *, Ts&...); // #skl2_decl

  template<typename... Ts>
  void ft(Ts&...); // #ft2_decl

  // Test array decay to pointer; resulting pointer is move eligible.
  // expected-error@+5           {{no matching function for call to 'sycl_kernel_launch'}}
  // expected-note@+3            {{this indicates a problem with the SYCL runtime header files; please consider reporting this to your SYCL runtime provider}}
  // expected-note@+2            {{in implicit call to 'sycl_kernel_launch' with template argument 'KN<2, 1>' and function arguments (lvalue of type 'const char[18]', xvalue of type 'int *') required here}}
  // expected-note@#skl2_decl    {{candidate function [with KN = KN<2, 1>, Ts = <int *>] not viable: expects an lvalue for 2nd argument}}
  [[clang::sycl_kernel_entry_point(KN<2, 1>)]]
  void skep_array(int a[10]) {
    // expected-error@+2         {{no matching function for call to 'ft'}}
    // expected-note@#ft2_decl   {{candidate function [with Ts = <int *>] not viable: expects an lvalue for 1st argument}}
    ft(move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }
  void control_array(int a[10]) {
    // expected-error@+2         {{no matching function for call to 'sycl_kernel_launch'}}
    // expected-note@#skl2_decl  {{candidate function [with KN = KN<2, 1>, Ts = <int *>] not viable: expects an lvalue for 2nd argument}}
    sycl_kernel_launch<KN<2, 1>>("", move(a));
    // expected-error@+2         {{no matching function for call to 'ft'}}
    // expected-note@#ft2_decl   {{candidate function [with Ts = <int *>] not viable: expects an lvalue for 1st argument}}
    ft(move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }

  // Test lvalue reference to array; parameter is not move eligible.
  [[clang::sycl_kernel_entry_point(KN<2, 2>)]]
  void skep_array_ref(int (&a)[10]) {
    ft(a);
    static_assert(is_same_v<decltype(a), int(&)[10]>);
  }
  void control_array_ref(int (&a)[10]) {
    sycl_kernel_launch<KN<2, 2>>("", a);
    ft(a);
    static_assert(is_same_v<decltype(a), int(&)[10]>);
  }

  // Test VMT decay to pointer; resulting pointer is move eligible.
  // expected-error@+5          {{no matching function for call to 'sycl_kernel_launch'}}
  // expected-note@+3           {{this indicates a problem with the SYCL runtime header files; please consider reporting this to your SYCL runtime provider}}
  // expected-note@+2           {{in implicit call to 'sycl_kernel_launch' with template argument 'KN<2, 3>' and function arguments (lvalue of type 'const char[18]', xvalue of type 'int', xvalue of type 'int *') required here}}
  // expected-note@#skl2_decl   {{candidate function [with KN = KN<2, 3>, Ts = <int, int *>] not viable: expects an lvalue for 2nd argument}}
  [[clang::sycl_kernel_entry_point(KN<2, 3>)]]
  void skep_vmt(int n, int a[n]) {
    // expected-error@+2        {{no matching function for call to 'ft'}}
    // expected-note@#ft2_decl  {{candidate function [with Ts = <int, int *>] not viable: expects an lvalue for 1st argument}}
    ft(move(n), move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }
  void control_vmt(int n, int a[n]) {
    // expected-error@+2        {{no matching function for call to 'sycl_kernel_launch'}}
    // expected-note@#skl2_decl {{candidate function [with KN = KN<2, 3>, Ts = <int, int *>] not viable: expects an lvalue for 2nd argument}}
    sycl_kernel_launch<KN<2, 3>>("", move(n), move(a));
    // expected-error@+2        {{no matching function for call to 'ft'}}
    // expected-note@#ft2_decl  {{candidate function [with Ts = <int, int *>] not viable: expects an lvalue for 1st argument}}
    ft(move(n), move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }

  // Test lvalue reference to VMT; parameter is not move eligible.
  // expected-error@+5          {{no matching function for call to 'sycl_kernel_launch'}}
  // expected-note@+3           {{this indicates a problem with the SYCL runtime header files; please consider reporting this to your SYCL runtime provider}}
  // expected-note@+2           {{in implicit call to 'sycl_kernel_launch' with template argument 'KN<2, 4>' and function arguments (lvalue of type 'const char[18]', xvalue of type 'int', lvalue of type 'int[n]') required here}}
  // expected-note@#skl2_decl   {{candidate template ignored: substitution failure [with KN = KN<2, 4>]: variably modified type 'int[n]' cannot be used as a template argument}}
  [[clang::sycl_kernel_entry_point(KN<2, 4>)]]
  void skep_vmt_ref(int n, int (&a)[n]) {
    // expected-error@+2        {{no matching function for call to 'ft'}}
    // expected-note@#ft2_decl  {{candidate template ignored: substitution failure: variably modified type 'int[n]' cannot be used as a template argument}}
    ft(move(n), a);
#if defined(__SYCL_DEVICE_ONLY__)
    // expected-error@+3        {{variable length arrays are not supported for the current target}}
#endif
    // expected-error@+1        {{variably modified type 'decltype(a)' (aka 'int (&)[n]') cannot be used as a template argument}}
    static_assert(is_same_v<decltype(a), int(&)[n]>);
  }
  void control_vmt_ref(int n, int (&a)[n]) {
    // expected-error@+2        {{no matching function for call to 'sycl_kernel_launch'}}
    // expected-note@#skl2_decl {{candidate template ignored: substitution failure [with KN = KN<2, 4>]: variably modified type 'int[n]' cannot be used as a template argument}}
    sycl_kernel_launch<KN<2, 4>>("", move(n), a);
    // expected-error@+2        {{no matching function for call to 'ft'}}
    // expected-note@#ft2_decl  {{candidate template ignored: substitution failure: variably modified type 'int[n]' cannot be used as a template argument}}
    ft(move(n), a);
    // expected-error@+1        {{variably modified type 'decltype(a)' (aka 'int (&)[n]') cannot be used as a template argument}}
    static_assert(is_same_v<decltype(a), int(&)[n]>);
  }
}

// sycl_kernel_launch that receives arguments by rvalue reference.
namespace test3 {
  template<typename KN, typename... Ts>
  void sycl_kernel_launch(const char *, Ts&&...); // #skl3_decl

  template<typename... Ts>
  void ft(Ts&&...); // #ft3_decl

  // Test array decay to pointer; resulting pointer is move eligible.
  [[clang::sycl_kernel_entry_point(KN<3, 1>)]]
  void skep_array(int a[10]) {
    ft(move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }
  void control_array(int a[10]) {
    sycl_kernel_launch<KN<3, 1>>("", move(a));
    ft(move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }

  // Test lvalue reference to array; parameter is not move eligible.
  [[clang::sycl_kernel_entry_point(KN<3, 2>)]]
  void skep_array_ref(int (&a)[10]) {
    ft(a);
    static_assert(is_same_v<decltype(a), int(&)[10]>);
  }
  void control_array_ref(int (&a)[10]) {
    sycl_kernel_launch<KN<3, 2>>("", a);
    ft(a);
    static_assert(is_same_v<decltype(a), int(&)[10]>);
  }

  // Test VMT decay to pointer; resulting pointer is move eligible.
  [[clang::sycl_kernel_entry_point(KN<3, 3>)]]
  void skep_vmt(int n, int a[n]) {
    ft(move(n), move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }
  void control_vmt(int n, int a[n]) {
    sycl_kernel_launch<KN<3, 3>>("", move(n), move(a));
    ft(move(n), move(a));
    static_assert(is_same_v<decltype(a), int*>);
  }

  // Test lvalue reference to VMT; parameter is not move eligible.
  // expected-error@+5          {{no matching function for call to 'sycl_kernel_launch'}}
  // expected-note@+3           {{this indicates a problem with the SYCL runtime header files; please consider reporting this to your SYCL runtime provider}}
  // expected-note@+2           {{in implicit call to 'sycl_kernel_launch' with template argument 'KN<3, 4>' and function arguments (lvalue of type 'const char[18]', xvalue of type 'int', lvalue of type 'int[n]') required here}}
  // expected-note@#skl3_decl   {{candidate template ignored: substitution failure [with KN = KN<3, 4>]: variably modified type 'int (&)[n]' cannot be used as a template argument}}
  [[clang::sycl_kernel_entry_point(KN<3, 4>)]]
  void skep_vmt_ref(int n, int (&a)[n]) {
    // expected-error@+2        {{no matching function for call to 'ft'}}
    // expected-note@#ft3_decl  {{candidate template ignored: substitution failure: variably modified type 'int (&)[n]' cannot be used as a template argument}}
    ft(move(n), a);
#if defined(__SYCL_DEVICE_ONLY__)
    // expected-error@+3        {{variable length arrays are not supported for the current target}}
#endif
    // expected-error@+1        {{variably modified type 'decltype(a)' (aka 'int (&)[n]') cannot be used as a template argument}}
    static_assert(is_same_v<decltype(a), int(&)[n]>);
  }
  void control_vmt_ref(int n, int (&a)[n]) {
    // expected-error@+2        {{no matching function for call to 'sycl_kernel_launch'}}
    // expected-note@#skl3_decl {{candidate template ignored: substitution failure [with KN = KN<3, 4>]: variably modified type 'int (&)[n]' cannot be used as a template argument}}
    sycl_kernel_launch<KN<3, 4>>("", move(n), a);
    // expected-error@+2        {{no matching function for call to 'ft'}}
    // expected-note@#ft3_decl  {{candidate template ignored: substitution failure: variably modified type 'int (&)[n]' cannot be used as a template argument}}
    ft(move(n), a);
    // expected-error@+1        {{variably modified type 'decltype(a)' (aka 'int (&)[n]') cannot be used as a template argument}}
    static_assert(is_same_v<decltype(a), int(&)[n]>);
  }
}
