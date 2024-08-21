// RUN: %clang_cc1 -fsycl-is-device -emit-llvm  -triple spir64  %s -o - | FileCheck --check-prefix=CHECK-DEVICE %s
// RUN: %clang_cc1 -fsycl-is-host -emit-llvm -triple x86_64  %s -o - | FileCheck --check-prefix=CHECK-HOST %s

// Test the generation of SYCL kernel caller function. The arguments of the SYCL kernel
// caller function correspond to either the SYCL Kernel Object, or decomposed fields of
// the SYCL kernel object if special SYCL types are captured (not yet supported). In the
// latter case, the SYCL kernel object is reconstited in the body of the SYCL caller
// function. The body of the SYCL kernel caller function then invokes the SYCL kernel
// i.e. the operator method of the SYCL kernel object. SYCL kernel caller should not be
// generated during host compilation.

template <typename name, typename Func>
__attribute__((sycl_kernel_entry_point(name))) void kernel_single_task(const Func kernelFunc) {
  kernelFunc();
}

struct single_purpose_kernel_name;
struct single_purpose_kernel {
  void operator()() const;
};

__attribute__((sycl_kernel_entry_point(single_purpose_kernel_name)))
void single_purpose_kernel_task(single_purpose_kernel kernelFunc) {
  kernelFunc();
}

int main() {
  int capture;
  kernel_single_task<class test_kernel>(
    [=]() {
      (void) capture;
    });
  single_purpose_kernel obj;
  single_purpose_kernel_task(obj);
}

// CHECK-HOST-NOT: __sycl_kernel_caller

// IR for compiler generated SYCL kernel caller function corresponding to
// single_purpose_kernel_name:

// CHECK-DEVICE: Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// FIXME: Pointer Alignment mismatch with syclos must be clarified and confirmed.
// CHECK-DEVICE: define dso_local spir_kernel void @_Z20__sycl_kernel_callerI26single_purpose_kernel_nameEvv
// CHECK-DEVICE-SAME: (ptr noundef byval(%struct.single_purpose_kernel) align 1 %kernelFunc) #[[ATTR0:[0-9]+]] {
// CHECK-DEVICE-NEXT: entry:
// CHECK-DEVICE-NEXT: %kernelFunc.ascast = addrspacecast ptr %kernelFunc to ptr addrspace(4)
// CHECK-DEVICE-NEXT: call spir_func void @_ZNK21single_purpose_kernelclEv
// CHECK-DEVICE-SAME: (ptr addrspace(4) noundef align 1 dereferenceable_or_null(1) %kernelFunc.ascast) #[[ATTR1:[0-9]+]]
// CHECK-DEVICE-NEXT:  ret void
// CHECK-DEVICE-NEXT:}

// IR for compiler generated SYCL kernel caller function corresponding to
// test_kernel:

// CHECK-DEVICE: Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// FIXME: Pointer Alignment mismatch with syclos must be clarified and confirmed.
// CHECK-DEVICE: define dso_local spir_kernel void @_Z20__sycl_kernel_callerIZ4mainE11test_kernelEvv(ptr noundef byval(%class.anon) align 4 %kernelFunc) #[[ATTR0]] {
// CHECK-DEVICE-NEXT: entry:
// CHECK-DEVICE-NEXT:  %kernelFunc.ascast = addrspacecast ptr %kernelFunc to ptr addrspace(4)
// CHECK-DEVICE-NEXT:  call spir_func void @_ZZ4mainENKUlvE_clEv
// CHECK-DEVICE-SAME:  (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %kernelFunc.ascast) #[[ATTR1]]
// CHECK-DEVICE-NEXT:  ret void
// CHECK-DEVICE-NEXT:}

// IR for operator method of kernel object:
// CHECK-DEVICE: define internal spir_func void @_ZZ4mainENKUlvE_clEv
// FIXME: Pointer Alignment mismatch with syclos must be clarified and confirmed.
// FIXME: !srcloc metadata present in syclos (with incorrect value?). Why is this not present in llvm.org ?
// CHECK-DEVICE-SAME: (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %this) #[[ATTR0]] align 2 {
// CHECK-DEVICE-NEXT: entry:
// CHECK-DEVICE-NEXT:  %this.addr = alloca ptr addrspace(4), align 8
// CHECK-DEVICE-NEXT:  %this.addr.ascast = addrspacecast ptr %this.addr to ptr addrspace(4)
// CHECK-DEVICE-NEXT:  store ptr addrspace(4) %this, ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-DEVICE-NEXT:  %this1 = load ptr addrspace(4), ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-DEVICE-NEXT:  %[[CAPTURE:.+]] = getelementptr inbounds %class.anon, ptr addrspace(4) %this1, i32 0, i32 0
// CHECK-DEVICE-NEXT:  ret void
// CHECK-DEVICE-NEXT:}

// FIXME:: Additional function attribute "sycl-optlevel"="0" generated in syclos vs llvm.org by a LLVM pass.
// CHECK-DEVICE: #[[ATTR0]] = { convergent mustprogress noinline norecurse nounwind optnone "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
// CHECK-DEVICE: #[[ATTR1]] = { convergent nounwind }
