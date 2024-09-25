// RUN: %clang_cc1 -triple spir64-unknown-unknown-sycldevice -aux-triple x86_64-pc-windows-msvc -fsycl-is-device -disable-llvm-passes -fsycl-is-device -emit-llvm %s -o - | FileCheck %s '-D$ADDRSPACE=addrspace(1) '
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fsycl-is-device -disable-llvm-passes -fsycl-is-device -emit-llvm %s -o - | FileCheck %s '-D$ADDRSPACE='


template<typename KN, typename Func>
__attribute__((sycl_kernel_entry_point(KN))) void kernel(Func F){
  F();
}

template<typename KN, typename Func>
__attribute__((sycl_kernel_entry_point(KN))) void kernel2(Func F){
  F(1);
}

template<typename KN, typename Func>
__attribute__((sycl_kernel_entry_point(KN))) void kernel3(Func F){
  F(1.1);
}

template<typename Func>
void kernel_wrapper(Func F) {
  kernel<Func>(F);
}

int main() {
  auto lambda1 = [](){};
  auto lambda2 = [](int){};
  auto lambda3 = [](double){};

  // Ensure the kernels are named the same between the device and host
  // invocations.
  kernel_wrapper([](){
  (void)__builtin_sycl_unique_stable_name(decltype(lambda1));
  (void)__builtin_sycl_unique_stable_name(decltype(lambda2));
  (void)__builtin_sycl_unique_stable_name(decltype(lambda3));
  });

  // Make sure the following 3 are the same between the host and device compile.
  // Note that these are NOT the same value as eachother, they differ by the
  // signature.
  // CHECK: private unnamed_addr [[$ADDRSPACE]]constant [17 x i8] c"_ZTSZ4mainEUlvE_\00"
  // CHECK: private unnamed_addr [[$ADDRSPACE]]constant [17 x i8] c"_ZTSZ4mainEUliE_\00"
  // CHECK: private unnamed_addr [[$ADDRSPACE]]constant [17 x i8] c"_ZTSZ4mainEUldE_\00"
}
