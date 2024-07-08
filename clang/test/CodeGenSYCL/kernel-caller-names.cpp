// RUN: %clang_cc1 -std=c++17 -fsycl-is-device -emit-llvm -triple spir -aux-triple x86_64-linux-gnu %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++17 -fsycl-is-device -emit-llvm -triple spir -aux-triple x86_64-pc-windows-msvc %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++17 -fsycl-is-device -emit-llvm -triple spir64 -aux-triple x86_64-linux-gnu %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++17 -fsycl-is-device -emit-llvm -triple spir64 -aux-triple x86_64-pc-windows-msvc %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++17 -fsycl-is-device -emit-llvm -triple nvptx -aux-triple x86_64-linux-gnu %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++17 -fsycl-is-device -emit-llvm -triple nvptx -aux-triple x86_64-pc-windows-msvc %s -o - | FileCheck %s

// use_default_kernel_name is used as a tag type to opt in to use of the
// kernel type as the kernel name.
struct use_default_kernel_name;
template<typename KN, typename KT>
struct kernel_name {
  using type = KN;
};
template<typename KT>
struct kernel_name<use_default_kernel_name, KT> {
  using type = KT;
};
template<typename KN, typename KT>
using kernel_name_t = typename kernel_name<KN, KT>::type;

template<typename KN = use_default_kernel_name, typename KT>
__attribute__((sycl_kernel_entry_point(kernel_name_t<KN, KT>)))
void skep(KT k) {
  k();
}

auto gl1 = []{};
auto gl2 = []{};
struct GF1 {
  void operator()() const {}
};
struct explicit_kernel_name;

// Validate SYCL kernel caller names generated for kernel name types
// declared at namespace scope.
void ok1() {
  skep(gl1);
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIUlvE_Evv{{.*}} {
  skep(gl2);
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIUlvE0_Evv{{.*}} {
  skep(GF1{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerI3GF1Evv{{.*}} {
  skep<explicit_kernel_name>([]{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerI20explicit_kernel_nameEvv{{.*}} {
}

// Validate SYCL kernel caller names generated for kernel name types
// declared at block scope.
void ok2() {
  auto ll1 = []{};
  auto ll2 = []{};
  struct LF1 {
    void operator()() const {}
  };
  struct local_explicit_kernel_name {};

  skep(ll1);
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIZ3ok2vEUlvE_Evv{{.*}} {
  skep(ll2);
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIZ3ok2vEUlvE0_Evv{{.*}} {
  skep(LF1{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIZ3ok2vE3LF1Evv{{.*}} {
  skep<local_explicit_kernel_name>([]{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIZ3ok2vE26local_explicit_kernel_nameEvv{{.*}} {
}

// Validate SYCL kernel caller names generated for kernel name types
// that are cv-qualified.
void ok3() {
  // The SYCL 2020 specification is not clear regarding whether the types used
  // for kernel names may be cv-qualified. The following issue tracks clarifying
  // the intent.
  // - https://github.com/KhronosGroup/SYCL-Docs/issues/568
  struct kn {};
  skep<kn>([]{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIZ3ok3vE2knEvv{{.*}} {
  skep<const kn>([]{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIKZ3ok3vE2knEvv{{.*}} {
  skep<volatile kn>([]{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIVZ3ok3vE2knEvv{{.*}} {
  skep<const volatile kn>([]{});
  // CHECK: define {{.*}} @_Z20__sycl_kernel_callerIVKZ3ok3vE2knEvv{{.*}} {
}
