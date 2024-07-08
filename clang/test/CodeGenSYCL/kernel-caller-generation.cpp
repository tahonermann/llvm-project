// RUN: %clang_cc1 -fsycl-is-device -emit-llvm  -triple spir64  %s -o - | FileCheck %s

template <typename name, typename Func>
__attribute__((sycl_kernel_entry_point(name))) void kernel_single_task(const Func kernelFunc) {
  kernelFunc();
}

int main() {
  int capture;
  kernel_single_task<class test_kernel>(
    [=]() {
      (void) capture;
    });
}


// IR for compiler generated SYCL kernel caller function :
// The arguments of the SYCL kernel caller function correspond to either the SYCL Kernel
// Object, or decomposed fields of the SYCL kernel object if special SYCL types are
// captured (not yet supported). In the latter case, the SYCL kernel object is reconstited
// in the body of the SYCL caller function. The body of the SYCL kernel caller function
// then invokes the SYCL kernel i.e. the operator method of the SYCL kernel object.

// CHECK: Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// FIXME: Pointer Alignment mismatch with syclos must be clarified and confirmed.
// CHECK: define dso_local spir_kernel void @_Z20__sycl_kernel_callerIZ4mainE11test_kernelEvv(ptr noundef byval(%class.anon) align 4 %kernelFunc) #[[ATTR0:[0-9]+]] {
// CHECK-NEXT: entry:
// CHECK-NEXT:  %kernelFunc.ascast = addrspacecast ptr %kernelFunc to ptr addrspace(4)
// CHECK-NEXT:  call spir_func void @_ZZ4mainENKUlvE_clEv
// CHECK-SAME:  (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %kernelFunc.ascast) #[[ATTR1:[0-9]+]]
// CHECK-NEXT:  ret void
// CHECK-NEXT:}

// IR for operator method of kernel object:
// CHECK: define internal spir_func void @_ZZ4mainENKUlvE_clEv
// FIXME: Pointer Alignment mismatch with syclos must be clarified and confirmed.
// FIXME: !srcloc metadata present in syclos (with incorrect value?). Why is this not present in llvm.org ?
// CHECK-SAME: (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %this) #[[ATTR0]] align 2 {
// CHECK-NEXT: entry:
// CHECK-NEXT:  %this.addr = alloca ptr addrspace(4), align 8
// CHECK-NEXT:  %this.addr.ascast = addrspacecast ptr %this.addr to ptr addrspace(4)
// CHECK-NEXT:  store ptr addrspace(4) %this, ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-NEXT:  %this1 = load ptr addrspace(4), ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-NEXT:  %[[CAPTURE:.+]] = getelementptr inbounds %class.anon, ptr addrspace(4) %this1, i32 0, i32 0
// CHECK-NEXT:  ret void
// CHECK-NEXT:}

// FIXME:: Additional function attribute "sycl-optlevel"="0" generated in syclos vs llvm.org by a LLVM pass.
// CHECK: #[[ATTR0]] = { convergent mustprogress noinline norecurse nounwind optnone "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
// CHECK: #[[ATTR1]] = { convergent nounwind }
