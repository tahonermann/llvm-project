// RUN: %clang_cc1 -fsycl-is-device -emit-llvm  -triple spir64  %s -o - | FileCheck --check-prefix=CHECK-SPIR %s
// RUN: %clang_cc1 -fsycl-is-device -emit-llvm  -triple nvptx  %s -o - | FileCheck --check-prefix=CHECK-NVPTX %s
// RUN: %clang_cc1 -fsycl-is-device -emit-llvm  -triple amdgcn  %s -o - | FileCheck --check-prefix=CHECK-AMDGCN %s
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

// CHECK-HOST-NOT: __sycl_kernel_caller

// IR for compiler generated SYCL kernel caller function corresponding to
// single_purpose_kernel_name:

// CHECK-SPIR: Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// CHECK-SPIR: define dso_local spir_kernel void @_Z20__sycl_kernel_callerI26single_purpose_kernel_nameEvv
// CHECK-SPIR: (ptr noundef byval(%struct.single_purpose_kernel) align 1 %kernelFunc) #[[ATTR0:[0-9]+]]
// CHECK-SPIR-SAME: !kernel_arg_addr_space ![[ADDRSP:[0-9]+]] !kernel_arg_access_qual ![[ACCQUAL:[0-9]+]]
// CHECK-SPIR-SAME: !kernel_arg_type ![[ARGTY1:[0-9]+]] !kernel_arg_base_type ![[ARGBASETY1:[0-9]+]]
// CHECK-SPIR-SAME: !kernel_arg_type_qual ![[ARGTYQUAL:[0-9]+]]
// CHECK-SPIR-NEXT: entry:
// CHECK-SPIR-NEXT: %kernelFunc.ascast = addrspacecast ptr %kernelFunc to ptr addrspace(4)
// CHECK-SPIR-NEXT: call spir_func void @_ZNK21single_purpose_kernelclEv
// CHECK-SPIR-SAME: (ptr addrspace(4) noundef align 1 dereferenceable_or_null(1) %kernelFunc.ascast) #[[ATTR1:[0-9]+]]
// CHECK-SPIR-NEXT:  ret void
// CHECK-SPIR-NEXT:}

// CHECK-NVPTX:  define dso_local void @_Z20__sycl_kernel_callerI26single_purpose_kernel_nameEvv
// CHECK-AMDGCN: define dso_local amdgpu_kernel void @_Z20__sycl_kernel_callerI26single_purpose_kernel_nameEvv

// IR for compiler generated SYCL kernel caller function corresponding to
// test_kernel:

// CHECK-SPIR: Function Attrs: convergent mustprogress noinline norecurse nounwind optnone
// CHECK-SPIR: define dso_local spir_kernel void @_Z20__sycl_kernel_callerIZ4mainE11test_kernelEvv(ptr noundef byval(%class.anon) align 4 %kernelFunc) #[[ATTR0]]
// CHECK-SPIR-SAME: !kernel_arg_addr_space ![[ADDRSP]] !kernel_arg_access_qual ![[ACCQUAL]]
// CHECK-SPIR-SAME: !kernel_arg_type ![[ARGTY2:[0-9]+]] !kernel_arg_base_type ![[ARGBASETY2:[0-9]+]]
// CHECK-SPIR-SAME: !kernel_arg_type_qual ![[ARGTYQUAL]]
// CHECK-SPIR-NEXT: entry:
// CHECK-SPIR-NEXT:  %kernelFunc.ascast = addrspacecast ptr %kernelFunc to ptr addrspace(4)
// CHECK-SPIR-NEXT:  call spir_func void @_ZZ4mainENKUlvE_clEv
// CHECK-SPIR-SAME:  (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %kernelFunc.ascast) #[[ATTR1]]
// CHECK-SPIR-NEXT:  ret void
// CHECK-SPIR-NEXT:}

// CHECK-NVPTX:  define dso_local void @_Z20__sycl_kernel_callerIZ4mainE11test_kernelEvv
// CHECK-AMDGCN: define dso_local amdgpu_kernel void @_Z20__sycl_kernel_callerIZ4mainE11test_kernelEvv

// IR for operator method of kernel object:
// CHECK-SPIR: define internal spir_func void @_ZZ4mainENKUlvE_clEv
// CHECK-SPIR-SAME: (ptr addrspace(4) noundef align 4 dereferenceable_or_null(4) %this) #[[ATTR0]] align 2 {
// CHECK-SPIR-NEXT: entry:
// CHECK-SPIR-NEXT:  %this.addr = alloca ptr addrspace(4), align 8
// CHECK-SPIR-NEXT:  %this.addr.ascast = addrspacecast ptr %this.addr to ptr addrspace(4)
// CHECK-SPIR-NEXT:  store ptr addrspace(4) %this, ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-SPIR-NEXT:  %this1 = load ptr addrspace(4), ptr addrspace(4) %this.addr.ascast, align 8
// CHECK-SPIR-NEXT:  %[[CAPTURE:.+]] = getelementptr inbounds %class.anon, ptr addrspace(4) %this1, i32 0, i32 0
// CHECK-SPIR-NEXT:  ret void
// CHECK-SPIR-NEXT:}

// CHECK-NVPTX: define internal void @_ZZ4mainENKUlvE_clEv
// CHECK-AMDGCN: define internal void @_ZZ4mainENKUlvE_clEv

// CHECK-SPIR: #[[ATTR0]] = { convergent mustprogress noinline norecurse nounwind optnone "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
// CHECK-SPIR: #[[ATTR1]] = { convergent nounwind }

// CHECK-SPIR: ![[ADDRSP]] = !{i32 0}
// CHECK-SPIR: ![[ACCQUAL]] = !{!"none"}
// CHECK-SPIR: ![[ARGTY1]] = !{!"single_purpose_kernel"}
// CHECK-SPIR: ![[ARGTYQUAL]] = !{!""}
// CHECK-SPIR: ![[ARGTY2]] = !{!"(lambda at
