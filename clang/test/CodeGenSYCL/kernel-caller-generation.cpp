// RUN: %clang_cc1 -fsycl-is-device -emit-llvm  -triple spir64  %s -o - | FileCheck --check-prefix=CHECK-DEVICE %s
// RUN: %clang_cc1 -fsycl-is-host -emit-llvm -triple x86_64  %s -o - | FileCheck --check-prefix=CHECK-HOST %s

// Test the generation of SYCL kernel caller function. SYCL kernel caller function
// should not be generated during host compilation.

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

// Verify that no SYCL kernel caller functions are generated for the host.
// CHECK-HOST-NOT: __sycl_kernel_caller

// Verify that no code is generated for the body of the SYCL kernel entry point
// functions on the host. These functions are used to trigger emission of SYCL
// kernel caller functions on the device; calls to them on the host are intended
// to have no other effect (the body of these functions define a pattern for
// code to be executed on the device).
//
// CHECK-HOST:      define dso_local void @_Z26single_purpose_kernel_task21single_purpose_kernel() #0 {
// CHECK-HOST-NEXT: entry:
// CHECK-HOST-NEXT:   %kernelFunc = alloca %struct.single_purpose_kernel, align 1
// CHECK-HOST-NEXT:   ret void
// CHECK-HOST-NEXT: }
//
// CHECK-HOST:      define internal void @_Z18kernel_single_taskIZ4mainE11test_kernelZ4mainEUlvE_EvT0_(i32 %kernelFunc.coerce) #0 {
// CHECK-HOST-NEXT: entry:
// CHECK-HOST-NEXT:   %kernelFunc = alloca %class.anon, align 4
// CHECK-HOST-NEXT:   %coerce.dive = getelementptr inbounds %class.anon, ptr %kernelFunc, i32 0, i32 0
// CHECK-HOST-NEXT:   store i32 %kernelFunc.coerce, ptr %coerce.dive, align 4
// CHECK-HOST-NEXT:   ret void
// CHECK-HOST-NEXT: }

// FIXME: A match on the next two lines is done to enable skipping over the
// FIXME: declaration of main(). main() shouldn't be emitted in device code,
// FIXME: but that pruning isn't performed yet. Remove these two checks
// FIXME: once it is.
// CHECK-DEVICE:      Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// CHECK-DEVICE-NEXT: define dso_local noundef i32 @main() #0

// IR for compiler generated SYCL kernel caller function corresponding to
// single_purpose_kernel_name:
//
// CHECK-DEVICE:      Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// CHECK-DEVICE-NEXT: define dso_local spir_kernel void @_Z20__sycl_kernel_callerI26single_purpose_kernel_nameEvv
// CHECK-DEVICE-SAME:   (ptr noundef byval(%struct.single_purpose_kernel) align 1 %kernelFunc) #[[ATTR0:[0-9]+]] {
// CHECK-DEVICE-NEXT: entry:
// CHECK-DEVICE-NEXT:   %kernelFunc.ascast = addrspacecast ptr %kernelFunc to ptr addrspace(4)
// CHECK-DEVICE-NEXT:   call spir_func void @_ZNK21single_purpose_kernelclEv
// CHECK-DEVICE-SAME:     (ptr addrspace(4) noundef align 1 dereferenceable_or_null(1) %kernelFunc.ascast) #[[ATTR1:[0-9]+]]
// CHECK-DEVICE-NEXT:   ret void
// CHECK-DEVICE-NEXT: }

// IR for compiler generated SYCL kernel caller function corresponding to
// test_kernel:
//
// CHECK-DEVICE:      Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// CHECK-DEVICE-NEXT: define dso_local spir_kernel void @_Z20__sycl_kernel_callerIZ4mainE11test_kernelEvv
// CHECK-DEVICE-SAME:   (ptr noundef byval(%class.anon) align 4 %kernelFunc) #[[ATTR0]] {
// CHECK-DEVICE-NEXT: entry:
// CHECK-DEVICE-NEXT:   %kernelFunc.ascast = addrspacecast ptr %kernelFunc to ptr addrspace(4)
// CHECK-DEVICE-NEXT:   call spir_func void @_ZZ4mainENKUlvE_clEv
// CHECK-DEVICE-SAME:   (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %kernelFunc.ascast) #[[ATTR1]]
// CHECK-DEVICE-NEXT:   ret void
// CHECK-DEVICE-NEXT: }

// IR for operator method of kernel object:
//
// CHECK-DEVICE:      define internal spir_func void @_ZZ4mainENKUlvE_clEv
// CHECK-DEVICE-SAME:   (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %this) #[[ATTR0]] align 2 {
// CHECK-DEVICE-NEXT: entry:
// CHECK-DEVICE-NEXT:   %this.addr = alloca ptr addrspace(4), align 8
// CHECK-DEVICE-NEXT:   %this.addr.ascast = addrspacecast ptr %this.addr to ptr addrspace(4)
// CHECK-DEVICE-NEXT:   store ptr addrspace(4) %this, ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-DEVICE-NEXT:   %this1 = load ptr addrspace(4), ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-DEVICE-NEXT:   %[[CAPTURE:.+]] = getelementptr inbounds %class.anon, ptr addrspace(4) %this1, i32 0, i32 0
// CHECK-DEVICE-NEXT:   ret void
// CHECK-DEVICE-NEXT: }

// CHECK-DEVICE: #[[ATTR0]] = { convergent mustprogress noinline norecurse nounwind optnone "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
// CHECK-DEVICE: #[[ATTR1]] = { convergent nounwind }
