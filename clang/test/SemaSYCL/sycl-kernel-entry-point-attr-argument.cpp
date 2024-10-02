// RUN: %clang_cc1 -triple x86_64-linux-gnu -std=c++17 -fsyntax-only -fno-spell-checking -fsycl-is-device -verify %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -std=c++20 -fsyntax-only -fno-spell-checking -fsycl-is-device -verify %s

// These tests validate proper handling of a sycl_kernel_entry_point attribute
// argument list. The single argument is required to denote a class type that
// meets the requirements of a SYCL kernel name as described in section 5.2,
// "Naming of kernels", of the SYCL 2020 specification.

// Common entities used to validate kernel name arguments.
template<int> struct ST; // #ST-decl
template<int N> using TTA = ST<N>; // #TTA-decl


////////////////////////////////////////////////////////////////////////////////
// Valid declarations.
////////////////////////////////////////////////////////////////////////////////
struct S1;
[[clang::sycl_kernel_entry_point(S1)]] void ok1();

typedef struct TA2 {} TA2;
[[clang::sycl_kernel_entry_point(TA2)]] void ok2();

using TA3 = struct TA3 {};
[[clang::sycl_kernel_entry_point(TA3)]] void ok3();

[[clang::sycl_kernel_entry_point(ST<4>)]] void ok4();

[[clang::sycl_kernel_entry_point(TTA<5>)]] void ok5();

namespace NS6 {
  struct NSS;
}
[[clang::sycl_kernel_entry_point(NS6::NSS)]] void ok6();

namespace {
  struct UNSS7;
}
[[clang::sycl_kernel_entry_point(UNSS7)]] void ok7();

struct S8 {} s8;
[[clang::sycl_kernel_entry_point(decltype(s8))]] void ok8();

template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok9();
void test_ok9() {
  ok9<struct LS9>();
}

template<int, typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok10();
void test_ok10() {
  ok10<1, struct LS10>();
}

namespace NS11 {
  struct NSS;
}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok11() {}
template<>
[[clang::sycl_kernel_entry_point(NS11::NSS)]] void ok11<NS11::NSS>() {}

struct S12;
[[clang::sycl_kernel_entry_point(S12)]] void ok12();
[[clang::sycl_kernel_entry_point(S12)]] void ok12() {}

struct S13;
struct S13;
// expected-warning@+3 {{redundant 'sycl_kernel_entry_point' attribute}}
// expected-note@+1  {{previous attribute is here}}
[[clang::sycl_kernel_entry_point(S13),
  clang::sycl_kernel_entry_point(S13)]]
void ok13();

template<typename T>
[[clang::sycl_kernel_entry_point(T)]] void ok14(T k);
void test_ok14() {
  ok14([]{});
}

template<typename> struct KN15;
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok15();
void test_ok15() {
  ok15<KN15<struct LS15>>();
}

template<typename> struct KN16;
enum E16 : int {};
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok16();
void test_ok16() {
  ok16<KN16<E16>>();
}

template<typename> struct KN17;
enum class E17 {};
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok17();
void test_ok17() {
  ok17<KN17<E17>>();
}

template<int> struct KN18;
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok18();
void test_ok18() {
  ok18<KN18<0>>();
}

int gv19;
template<int*> struct KN19;
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok19();
void test_ok19() {
  ok19<KN19<&gv19>>();
}

int gf20();
template<int(&)()> struct KN20;
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok20();
void test_ok20() {
  ok20<KN20<gf20>>();
}

struct S21;
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok21();
namespace std {
void test_ok21() {
  ok21<struct S21>();
}
}

template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void ok22();
namespace std {
void test_ok22() {
  ok22<struct LS22>();
}
}


////////////////////////////////////////////////////////////////////////////////
// Invalid declarations.
////////////////////////////////////////////////////////////////////////////////

// expected-error@+1 {{'sycl_kernel_entry_point' attribute takes one argument}}
[[clang::sycl_kernel_entry_point]] void bad1();

// expected-error@+1 {{'sycl_kernel_entry_point' attribute takes one argument}}
[[clang::sycl_kernel_entry_point()]] void bad2();

struct B3;
// expected-error@+2 {{expected ')'}}
// expected-error@+1 {{expected ']'}}
[[clang::sycl_kernel_entry_point(B3,)]] void bad3();

struct B4;
// expected-error@+3 {{expected ')'}}
// expected-error@+2 {{expected ','}}
// expected-warning@+1 {{unknown attribute 'X' ignored}}
[[clang::sycl_kernel_entry_point(B4, X)]] void bad4();

// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(1)]] void bad5();

// expected-error@+1 {{'int' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int)]] void bad6();

// expected-error@+1 {{'int ()' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int())]] void bad7();

// expected-error@+1 {{'int (*)()' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int(*)())]] void bad8();

// expected-error@+1 {{'int (&)()' is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(int(&)())]] void bad9();

// expected-error@+1 {{'decltype(nullptr)' (aka 'std::nullptr_t') is not a valid SYCL kernel name type; a class type is required}}
[[clang::sycl_kernel_entry_point(decltype(nullptr))]] void bad10();

void bf11();
// expected-error@+1 {{unknown type name 'bf11'}}
[[clang::sycl_kernel_entry_point(bf11)]] void bad11();

// expected-error@+2 {{use of class template 'ST' requires template arguments; argument deduction not allowed here}}
// expected-note@#ST-decl {{template is declared here}}
[[clang::sycl_kernel_entry_point(ST)]] void bad12();

// expected-error@+2 {{use of alias template 'TTA' requires template arguments; argument deduction not allowed here}}
// expected-note@#TTA-decl {{template is declared here}}
[[clang::sycl_kernel_entry_point(TTA)]] void bad13();

union BU14; // #U-decl
// expected-error@+2 {{'BU14' is not a valid SYCL kernel name type; a class type is required}}
// expected-note@#U-decl {{'BU14' declared here}}
[[clang::sycl_kernel_entry_point(BU14)]] void bad14();

enum BE15 {}; // #BE15-decl
// expected-error@+2 {{'BE15' is not a valid SYCL kernel name type; a class type is required}}
// expected-note@#BE15-decl {{'BE15' declared here}}
[[clang::sycl_kernel_entry_point(BE15)]] void bad15();

enum BE16 : int; // #BE16-decl
// expected-error@+2 {{'BE16' is not a valid SYCL kernel name type; a class type is required}}
// expected-note@#BE16-decl {{'BE16' declared here}}
[[clang::sycl_kernel_entry_point(BE16)]] void bad16();

enum {
  be17
};
// expected-error@+1 {{unknown type name 'be17'}}
[[clang::sycl_kernel_entry_point(be17)]] void bad17();

#if __cplusplus >= 202002L
template<typename> concept C = true;
// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(C)]] void bad18();

// expected-error@+1 {{expected a type}}
[[clang::sycl_kernel_entry_point(C<int>)]] void bad19();
#endif

struct B20 {
  struct MS; // #B20-MS-decl
};
// expected-error@+2 {{'B20::MS' is not a valid SYCL kernel name type}}
// expected-note@#B20-MS-decl {{'MS' is not forward declarable because it is not declared at global or namespace scope}}
[[clang::sycl_kernel_entry_point(B20::MS)]] void bad20();

struct B21 {
  struct MS; // #B21-MS-decl
};
// expected-error@+4 {{'typename B21::MS' is not a valid SYCL kernel name type}}
// expected-note@+4  {{in instantiation of function template specialization 'bad21<B21>' requested here}}
// expected-note@#B21-MS-decl {{'MS' is not forward declarable because it is not declared at global or namespace scope}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::MS)]] void bad21() {}
template void bad21<B21>();

struct B22; // #B22-decl
// FIXME: C++23 [temp.expl.spec]p12 states:
// FIXME:   ... Similarly, attributes appearing in the declaration of a template
// FIXME:   have no effect on an explicit specialization of that template.
// FIXME: Clang currently instantiates and propagates attributes from a function
// FIXME: template to its explicit specializations resulting in the following
// FIXME: spurious error.
// expected-error@+4 {{incomplete type 'B22' named in nested name specifier}}
// expected-note@+5 {{in instantiation of function template specialization 'bad22<B22>' requested here}}
// expected-note@#B22-decl {{forward declaration of 'B22'}}
template<typename T>
[[clang::sycl_kernel_entry_point(typename T::not_found)]] void bad22() {}
template<>
void bad22<B22>() {}

struct B23_1;
struct B23_2;
// expected-error@+3 {{'sycl_kernel_entry_point' kernel name argument does not match prior declaration: 'B23_2' vs 'B23_1'}}
// expected-note@+1  {{'bad23' declared here}}
[[clang::sycl_kernel_entry_point(B23_1)]] void bad23();
[[clang::sycl_kernel_entry_point(B23_2)]] void bad23() {}

struct B24_1;
struct B24_2;
// expected-error@+3 {{'sycl_kernel_entry_point' kernel name argument does not match prior declaration: 'B24_2' vs 'B24_1'}}
// expected-note@+1  {{previous attribute is here}}
[[clang::sycl_kernel_entry_point(B24_1),
  clang::sycl_kernel_entry_point(B24_2)]]
void bad24();

struct B25;
// expected-error@+3 {{'sycl_kernel_entry_point' kernel name argument conflicts with a previous declaration}}
// expected-note@+1  {{previous declaration is here}}
[[clang::sycl_kernel_entry_point(B25)]] void bad25_1();
[[clang::sycl_kernel_entry_point(B25)]] void bad25_2();

typedef struct {} TA26; // #TA26-decl
// expected-error@+2 {{'TA26' is not a valid SYCL kernel name type}}
// expected-note@#TA26-decl {{'TA26' is not forward declarable because it is an alias of an unnamed type}}
[[clang::sycl_kernel_entry_point(TA26)]] void bad26();

using TA27 = struct {}; // #TA27-decl
// expected-error@+2 {{'TA27' is not a valid SYCL kernel name type}}
// expected-note@#TA27-decl {{'TA27' is not forward declarable because it is an alias of an unnamed type}}
[[clang::sycl_kernel_entry_point(TA27)]] void bad27();

struct {} s28; // #s28-decl
// expected-error-re@+2 {{'decltype(s28)' (aka '(unnamed struct at {{.*}})') is not a valid SYCL kernel name type}}
// expected-note-re@#s28-decl {{'(unnamed struct at {{.*}})' is not forward declarable because it is an unnamed type}}
[[clang::sycl_kernel_entry_point(decltype(s28))]] void bad28();

enum { // #be29-decl
  be29
};
// expected-error-re@+2 {{'decltype(be29)' (aka '(unnamed enum at {{.*}})') is not a valid SYCL kernel name type; a class type is required}}
// expected-note-re@#be29-decl {{'decltype(be29)' (aka '(unnamed enum at {{.*}})') declared here}}
[[clang::sycl_kernel_entry_point(decltype(be29))]] void bad29();

// expected-error@+4 {{'LS30' is not a valid SYCL kernel name type}}
// expected-note@+6  {{in instantiation of function template specialization 'bad30<LS30>' requested here}}
// expected-note@#LS30-decl {{'LS30' is not forward declarable because it is not declared at global or namespace scope}}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void bad30();
void test_bad30() {
  struct LS30; // #LS30-decl
  bad30<LS30>();
}

// expected-error@+4 {{'LS31' is not a valid SYCL kernel name type}}
// expected-note@+6  {{in instantiation of function template specialization 'bad31<LS31>' requested here}}
// expected-note@#LS31-decl {{'LS31' is not forward declarable because it is not declared at global or namespace scope}}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void bad31();
void test_bad31() {
  struct LS31; // #LS31-decl
  bad31<struct LS31>();
}

// expected-error@+4 {{'LS32' is not a valid SYCL kernel name type}}
// expected-note@+6  {{in instantiation of function template specialization 'bad32<LS32>' requested here}}
// expected-note@#LS32-decl {{'LS32' is not forward declarable because it is not declared at global or namespace scope}}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void bad32();
void test_bad32() {
  struct LS32 {}; // #LS32-decl
  bad32<struct LS32>();
}

template<typename> struct BKN33;
// expected-error@+5 {{'BKN33<LS33>' is not a valid SYCL kernel name type}}
// expected-note@+7  {{in instantiation of function template specialization 'bad33<BKN33<LS33>>' requested here}}
// expected-note@+6  {{'BKN33<LS33>' is not forward declarable because it has a template argument that is not forward declarable}}
// expected-note@#LS33-decl {{'LS33' is not forward declarable because it is not declared at global or namespace scope}}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void bad33();
void test_bad33() {
  struct LS33 {}; // #LS33-decl
  bad33<BKN33<struct LS33>>();
}

template<typename> struct BKN34;
enum BE34 {}; // #BE34-decl
// expected-error@+5 {{'BKN34<BE34>' is not a valid SYCL kernel name type}}
// expected-note@+6  {{in instantiation of function template specialization 'bad34<BKN34<BE34>>' requested here}}
// expected-note@+5  {{'BKN34<BE34>' is not forward declarable because it has a template argument that is not forward declarable}}
// expected-note@#BE34-decl {{'BE34' is not forward declarable because it is an unscoped enumeration type without a fixed underlying type}}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void bad34();
void test_bad34() {
  bad34<BKN34<BE34>>();
}

typedef enum {} BE35;
BE35 be35;
template<auto*> struct BKN35;
// expected-error@+2 {{'BKN35<&be35>' is not a valid SYCL kernel name type}}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void bad35();
void test_bad35() {
  bad35<BKN35<&be35>>();
}

typedef enum {} BE36;
BE36 bgf36();
template<auto(&)()> struct BKN36;
// expected-error@+2 {{'BKN36<bf36>' is not a valid SYCL kernel name type}}
template<typename KN>
[[clang::sycl_kernel_entry_point(KN)]] void bad36();
void test_bad36() {
  bad36<BKN36<bgf36>>();
}

namespace std {
struct B37; // #B37-decl
}
// expected-error@+2 {{'std::B37' is not a valid SYCL kernel name type}}
// expected-note@#B37-decl {{'B37' is not forward declarable because it is declared within the 'std' namespace}}
[[clang::sycl_kernel_entry_point(std::B37)]] void bad37();

namespace std {
inline namespace v1 {
struct B38; // #B38-decl
}
}
// expected-error@+2 {{'std::B38' is not a valid SYCL kernel name type}}
// expected-note@#B38-decl {{'B38' is not forward declarable because it is declared within the 'std' namespace}}
[[clang::sycl_kernel_entry_point(std::B38)]] void bad38();

namespace std {
struct B39; // #B39-decl
}
template<typename> struct BKN39;
// expected-error@+2 {{'BKN39<std::B39>' is not a valid SYCL kernel name type}}
// expected-note@#B39-decl {{'B39' is not forward declarable because it is declared within the 'std' namespace}}
[[clang::sycl_kernel_entry_point(BKN39<std::B38>)]] void bad39();
