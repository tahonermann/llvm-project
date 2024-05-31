// Test without serialization:
// RUN: %clang_cc1 -std=c++17 -triple x86_64-unknown-unknown -fsycl-is-device \
// RUN: -ast-dump %s \
// RUN: | FileCheck --match-full-lines %s
//
// Test with serialization:
// RUN: %clang_cc1 -std=c++17 -triple x86_64-unknown-unknown -fsycl-is-device \
// RUN: -emit-pch -o %t %s
// RUN: %clang_cc1 -x c++ -std=c++17 -triple x86_64-unknown-unknown -fsycl-is-device \
// RUN: -include-pch %t -ast-dump-all /dev/null \
// RUN: | sed -e "s/ <undeserialized declarations>//" -e "s/ imported//" \
// RUN: | FileCheck --match-full-lines %s

// These tests validate the AST body produced for functions declared with the
// sycl_kernel_entry_point attribute.

// CHECK: TranslationUnitDecl {{.*}}

// A unique kernel name type is required for each declared kernel entry point.
template<int> struct KN;

// A unique invocable type for use with each declared kernel entry point.
template<int> struct K {
  template<typename... Ts>
  void operator()(Ts...) const;
};


[[clang::sycl_kernel_entry_point(KN<1>)]]
void skep1() {
}
// CHECK:      |-FunctionDecl {{.*}} skep1 'void ()'
// CHECK-NEXT: | |-SYCLKernelCallStmt {{.*}}
// CHECK-NEXT: | | |-CompoundStmt {{.*}}
// CHECK-NEXT: | | `-OutlinedFunctionDecl {{.*}}
// CHECK-NEXT: | |   `-CompoundStmt {{.*}}
// CHECK-NEXT: | `-SYCLKernelEntryPointAttr {{.*}} KN<1>

template<typename KNT, typename KT>
[[clang::sycl_kernel_entry_point(KNT)]]
void skep2(KT k) {
  k();
}
template
void skep2<KN<2>>(K<2>);
// CHECK:      |-FunctionTemplateDecl {{.*}} skep2
// CHECK-NEXT: | |-TemplateTypeParmDecl {{.*}} KNT
// CHECK-NEXT: | |-TemplateTypeParmDecl {{.*}} KT
// CHECK-NEXT: | |-FunctionDecl {{.*}} skep2 'void (KT)'
// CHECK-NEXT: | | |-ParmVarDecl {{.*}} k 'KT'
// CHECK-NEXT: | | |-CompoundStmt {{.*}}
// CHECK-NEXT: | | | `-CallExpr {{.*}} '<dependent type>'
// CHECK-NEXT: | | |   `-DeclRefExpr {{.*}} 'KT' lvalue ParmVar {{.*}} 'k' 'KT'
// CHECK-NEXT: | | `-SYCLKernelEntryPointAttr {{.*}} KNT

// CHECK-NEXT: | `-FunctionDecl {{.*}} skep2 'void (K<2>)' explicit_instantiation_definition
// CHECK-NEXT: |   |-TemplateArgument type 'KN<2>'
// CHECK-NEXT: |   | `-RecordType {{.*}} 'KN<2>'
// CHECK-NEXT: |   |   `-ClassTemplateSpecialization {{.*}} 'KN'
// CHECK-NEXT: |   |-TemplateArgument type 'K<2>'
// CHECK-NEXT: |   | `-RecordType {{.*}} 'K<2>'
// CHECK-NEXT: |   |   `-ClassTemplateSpecialization {{.*}} 'K'
// CHECK-NEXT: |   |-ParmVarDecl {{.*}} k 'K<2>'
// CHECK-NEXT: |   |-SYCLKernelCallStmt {{.*}}
// CHECK-NEXT: |   | |-CompoundStmt {{.*}}
// CHECK-NEXT: |   | | `-CXXOperatorCallExpr {{.*}} 'void' '()'
// CHECK-NEXT: |   | |   |-ImplicitCastExpr {{.*}} 'void (*)() const' <FunctionToPointerDecay>
// CHECK-NEXT: |   | |   | `-DeclRefExpr {{.*}} 'void () const' lvalue CXXMethod {{.*}} 'operator()' 'void () const'
// CHECK-NEXT: |   | |   `-ImplicitCastExpr {{.*}} 'const K<2>' lvalue <NoOp>
// CHECK-NEXT: |   | |     `-DeclRefExpr {{.*}} 'K<2>' lvalue ParmVar {{.*}} 'k' 'K<2>'
// CHECK-NEXT: |   | `-OutlinedFunctionDecl {{.*}}
// CHECK-NEXT: |   |   |-ImplicitParamDecl {{.*}} implicit k 'K<2>'
// CHECK-NEXT: |   |   `-CompoundStmt {{.*}}
// CHECK-NEXT: |   |     `-DeclRefExpr {{.*}} 'K<2>' lvalue ImplicitParam {{.*}} 'k' 'K<2>'
// CHECK-NEXT: |   `-SYCLKernelEntryPointAttr {{.*}} KN<2>

template<typename KNT, typename KT>
[[clang::sycl_kernel_entry_point(KNT)]]
void skep3(KT k) {
  k();
}
template<>
[[clang::sycl_kernel_entry_point(KN<3>)]]
void skep3<KN<3>>(K<3> k) {
  k();
}
// CHECK:      |-FunctionTemplateDecl {{.*}} skep3
// CHECK-NEXT: | |-TemplateTypeParmDecl {{.*}} KNT
// CHECK-NEXT: | |-TemplateTypeParmDecl {{.*}} KT
// CHECK-NEXT: | |-FunctionDecl {{.*}} skep3 'void (KT)'
// CHECK-NEXT: | | |-ParmVarDecl {{.*}} k 'KT'
// CHECK-NEXT: | | |-CompoundStmt {{.*}}
// CHECK-NEXT: | | | `-CallExpr {{.*}} '<dependent type>'
// CHECK-NEXT: | | |   `-DeclRefExpr {{.*}} 'KT' lvalue ParmVar {{.*}} 'k' 'KT'
// CHECK-NEXT: | | `-SYCLKernelEntryPointAttr {{.*}} KNT

// CHECK-NEXT: | `-Function {{.*}} 'skep3' 'void (K<3>)'
// CHECK-NEXT: |-FunctionDecl {{.*}} skep3 'void (K<3>)' explicit_specialization
// CHECK-NEXT: | |-TemplateArgument type 'KN<3>'
// CHECK-NEXT: | | `-RecordType {{.*}} 'KN<3>'
// CHECK-NEXT: | |   `-ClassTemplateSpecialization {{.*}} 'KN'
// CHECK-NEXT: | |-TemplateArgument type 'K<3>'
// CHECK-NEXT: | | `-RecordType {{.*}} 'K<3>'
// CHECK-NEXT: | |   `-ClassTemplateSpecialization {{.*}} 'K'
// CHECK-NEXT: | |-ParmVarDecl {{.*}} k 'K<3>'
// CHECK-NEXT: | |-SYCLKernelCallStmt {{.*}}
// CHECK-NEXT: | | |-CompoundStmt {{.*}}
// CHECK-NEXT: | | | `-CXXOperatorCallExpr {{.*}} 'void' '()'
// CHECK-NEXT: | | |   |-ImplicitCastExpr {{.*}} 'void (*)() const' <FunctionToPointerDecay>
// CHECK-NEXT: | | |   | `-DeclRefExpr {{.*}} 'void () const' lvalue CXXMethod {{.*}} 'operator()' 'void () const'
// CHECK-NEXT: | | |   `-ImplicitCastExpr {{.*}} 'const K<3>' lvalue <NoOp>
// CHECK-NEXT: | | |     `-DeclRefExpr {{.*}} 'K<3>' lvalue ParmVar {{.*}} 'k' 'K<3>'
// CHECK-NEXT: | | `-OutlinedFunctionDecl {{.*}}
// CHECK-NEXT: | |   |-ImplicitParamDecl {{.*}} implicit k 'K<3>'
// CHECK-NEXT: | |   `-CompoundStmt {{.*}}
// CHECK-NEXT: | |     `-DeclRefExpr {{.*}} 'K<3>' lvalue ImplicitParam {{.*}} 'k' 'K<3>'
// CHECK-NEXT: | `-SYCLKernelEntryPointAttr {{.*}} KN<3>

[[clang::sycl_kernel_entry_point(KN<4>)]]
void skep4(K<4> k, int p1, int p2) {
  k(p1, p2);
}
// CHECK:      |-FunctionDecl {{.*}} skep4 'void (K<4>, int, int)'
// CHECK-NEXT: | |-ParmVarDecl {{.*}} k 'K<4>'
// CHECK-NEXT: | |-ParmVarDecl {{.*}} p1 'int'
// CHECK-NEXT: | |-ParmVarDecl {{.*}} p2 'int'
// CHECK-NEXT: | |-SYCLKernelCallStmt {{.*}}
// CHECK-NEXT: | | |-CompoundStmt {{.*}}
// CHECK-NEXT: | | | `-CXXOperatorCallExpr {{.*}} 'void' '()'
// CHECK-NEXT: | | |   |-ImplicitCastExpr {{.*}} 'void (*)(int, int) const' <FunctionToPointerDecay>
// CHECK-NEXT: | | |   | `-DeclRefExpr {{.*}} 'void (int, int) const' lvalue CXXMethod {{.*}} 'operator()' 'void (int, int) const'
// CHECK-NEXT: | | |   |-ImplicitCastExpr {{.*}} 'const K<4>' lvalue <NoOp>
// CHECK-NEXT: | | |   | `-DeclRefExpr {{.*}} 'K<4>' lvalue ParmVar {{.*}} 'k' 'K<4>'
// CHECK-NEXT: | | |   |-ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
// CHECK-NEXT: | | |   | `-DeclRefExpr {{.*}} 'int' lvalue ParmVar {{.*}} 'p1' 'int'
// CHECK-NEXT: | | |   `-ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
// CHECK-NEXT: | | |     `-DeclRefExpr {{.*}} 'int' lvalue ParmVar {{.*}} 'p2' 'int'
// CHECK-NEXT: | | `-OutlinedFunctionDecl {{.*}}
// CHECK-NEXT: | |   |-ImplicitParamDecl {{.*}} implicit k 'K<4>'
// CHECK-NEXT: | |   |-ImplicitParamDecl {{.*}} implicit p1 'int'
// CHECK-NEXT: | |   |-ImplicitParamDecl {{.*}} implicit p2 'int'
// CHECK-NEXT: | |   `-CompoundStmt {{.*}}
// CHECK-NEXT: | |     |-DeclRefExpr {{.*}} 'K<4>' lvalue ImplicitParam {{.*}} 'k' 'K<4>'
// CHECK-NEXT: | |     |-DeclRefExpr {{.*}} 'int' lvalue ImplicitParam {{.*}} 'p1' 'int'
// CHECK-NEXT: | |     `-DeclRefExpr {{.*}} 'int' lvalue ImplicitParam {{.*}} 'p2' 'int'
// CHECK-NEXT: | `-SYCLKernelEntryPointAttr {{.*}} KN<4>

void the_end() {}
// CHECK:      `-FunctionDecl {{.*}} the_end 'void ()'
