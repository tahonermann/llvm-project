=============================================
SYCL Compiler and Runtime architecture design
=============================================

.. contents::
   :local:


Introduction
============

This document describes the architecture of the SYCL compiler and runtime
library. More details are provided in
`external document <https://github.com/intel/llvm/blob/sycl/sycl/doc/design/CompilerAndRuntimeDesign.md>`_\ ,
which are going to be added to clang documentation in the future.


Address space handling
======================

The SYCL specification represents pointers to disjoint memory regions using C++
wrapper classes on an accelerator to enable compilation with a standard C++
toolchain and a SYCL compiler toolchain. Section 3.8.2 of SYCL 2020
specification defines
`memory model <https://www.khronos.org/registry/SYCL/specs/sycl-2020/html/sycl-2020.html#_sycl_device_memory_model>`_\ ,
section 4.7.7 - `address space classes <https://www.khronos.org/registry/SYCL/specs/sycl-2020/html/sycl-2020.html#_address_space_classes>`_
and section 5.9 covers `address space deduction <https://www.khronos.org/registry/SYCL/specs/sycl-2020/html/sycl-2020.html#_address_space_deduction>`_.
The SYCL specification allows two modes of address space deduction: "generic as
default address space" (see section 5.9.3) and "inferred address space" (see
section 5.9.4). Current implementation supports only "generic as default address
space" mode.

SYCL borrows its memory model from OpenCL however SYCL doesn't perform
the address space qualifier inference as detailed in
`OpenCL C v3.0 6.7.8 <https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#addr-spaces-inference>`_.

The default address space is "generic-memory", which is a virtual address space
that overlaps the global, local, and private address spaces. SYCL mode enables
following conversions:

- explicit conversions to/from the default address space from/to the address
  space-attributed type
- implicit conversions from the address space-attributed type to the default
  address space
- explicit conversions to/from the global address space from/to the
  ``__attribute__((opencl_global_device))`` or
  ``__attribute__((opencl_global_host))`` address space-attributed type
- implicit conversions from the ``__attribute__((opencl_global_device))`` or
  ``__attribute__((opencl_global_host))`` address space-attributed type to the
  global address space

All named address spaces are disjoint and sub-sets of default address space.

The SPIR target allocates SYCL namespace scope variables in the global address
space.

Pointers to default address space should get lowered into a pointer to a generic
address space (or flat to reuse more general terminology). But depending on the
allocation context, the default address space of a non-pointer type is assigned
to a specific address space. This is described in
`common address space deduction rules <https://www.khronos.org/registry/SYCL/specs/sycl-2020/html/sycl-2020.html#subsec:commonAddressSpace>`_
section.

This is also in line with the behaviour of CUDA (`small example
<https://godbolt.org/z/veqTfo9PK>`_).

``multi_ptr`` class implementation example:

.. code-block:: C++

   // check that SYCL mode is ON and we can use non-standard decorations
   #if defined(__SYCL_DEVICE_ONLY__)
   // GPU/accelerator implementation
   template <typename T, address_space AS> class multi_ptr {
     // DecoratedType applies corresponding address space attribute to the type T
     // DecoratedType<T, global_space>::type == "__attribute__((opencl_global)) T"
     // See sycl/include/CL/sycl/access/access.hpp for more details
     using pointer_t = typename DecoratedType<T, AS>::type *;

     pointer_t m_Pointer;
     public:
     pointer_t get() { return m_Pointer; }
     T& operator* () { return *reinterpret_cast<T*>(m_Pointer); }
   }
   #else
   // CPU/host implementation
   template <typename T, address_space AS> class multi_ptr {
     T *m_Pointer; // regular undecorated pointer
     public:
     T *get() { return m_Pointer; }
     T& operator* () { return *m_Pointer; }
   }
   #endif

Depending on the compiler mode, ``multi_ptr`` will either decorate its internal
data with the address space attribute or not.

To utilize clang's existing functionality, we reuse the following OpenCL address
space attributes for pointers:

.. list-table::
   :header-rows: 1

   * - Address space attribute
     - SYCL address_space enumeration
   * - ``__attribute__((opencl_global))``
     - global_space, constant_space
   * - ``__attribute__((opencl_global_device))``
     - global_space
   * - ``__attribute__((opencl_global_host))``
     - global_space
   * - ``__attribute__((opencl_local))``
     - local_space
   * - ``__attribute__((opencl_private))``
     - private_space

.. code-block:: C++

    //TODO: add support for __attribute__((opencl_global_host)) and __attribute__((opencl_global_device)).


Kernel argument validation and decomposition
============================================

SYCL 2020 specifies requirements on the types of arguments that can be passed to
a SYCL kernel.
These requirements are enforced on the arguments passed to functions declared
with the
`[[clang::sycl_kernel_entry_point]] <https://clang.llvm.org/docs/AttributeReference.html#sycl-kernel-entry-point>`__
attribute.

Valid kernel argument types can be broadly categorized in three groups per
`section 3.13.1, "Device copyable" <https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html#sec::device.copyable>`__
and
`section 4.12.4, "Rules for parameter passing to kernels" <https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html#sec:kernel.parameter.passing>`__.

* Types that are implicitly device copyable because they satisfy the C++ definition
  of trivially copyable
  (`[basic.types.general]p9 <https://eel.is/c++draft/basic.types.general#9>`__,
  `[class.prop]p1 <https://eel.is/c++draft/class.prop#1>`__.
  Such types may be bit-copied to the device.
* Types that are explicitly device copyable because, for a type ``T``,
  ``sycl::is_device_copyable_v<T>`` is true.
  Such types may be bit-copied to the device.
* Types that are valid kernel arguments by fiat (``sycl::accessor``,
  ``sycl::local_accessor``, ``sycl::stream``, ``sycl::reducer``, etc...).
  Such types may require special handling.

Support for the third category of types is provided through a *decomposition protocol*
that such types opt in to as described below.
The decomposition protocol facilitates transformation of an object of such a type
into a sequence of objects, each of which has a type that satisfies one of the
first two type categories.

Types that opt in to the decomposition protocol, or directly or indirectly have a
subobject
(`[intro.object]p2 <https://eel.is/c++draft/intro.object#2>`__)
type that opts in to the decomposition protocol, *require decomposition*.
A kernel argument of such a type is transformed to a sequence of arguments that
are substituted for the original argument.

Note that a type that opts into the decomposition protocol may be a device copyable
type (e.g., a type that satisfies the C++ definition of a trivially copyable type).
Such types still require decomposition.

Given a call to a function declared with the ``[[clang::sycl_kernel_entry_point]]``
attribute, each argument ``A`` of parameter type ``P`` is processed as follows.
The resulting sequence of replacement arguments constitutes the arguments to the
``sycl_kernel_launch()`` function and their types constitute the corresponding
parameters of the synthesized offload kernel entry point function (the SYCL kernel
caller function).

#. If ``P`` is a non-union class type with a ``sycl_deconstruct()`` member function,
   then the type is one that has opted in to the decomposition protocol and shall
   meet the requirements below.

   #. The ``sycl_deconstruct()`` member function shall be a non-static member
      function, shall declare no parameters, shall return a *tuple-like* type that
      satisfies the requirements for an initializer of a structured binding declaration
      (`[dcl.struct.bind] <https://eel.is/c++draft/dcl.struct.bind>`__),
      shall not be declared with the ``[[noreturn]]`` attribute, may have a
      potentially-throwing exception specification
      (`[except.spec]p1 <https://eel.is/c++draft/except.spec#1>`__),
      and may have a function body that throws exceptions.

   #. The class shall declare a ``sycl_reconstruct()`` member function.
      That function shall be a static member function, shall have a return type of
      cv-unqualified ``P``, shall not be declared with the ``[[noreturn]]``
      attribute, may have a non-throwing exception specification, and shall declare
      parameters corresponding to the elements of the tuple-like type returned by
      ``sycl_deconstruct()``.
      For each element of that tuple-like type, there shall be a corresponding in-order
      parameter with a type that is convertible from the tuple element type.
      Additional parameters with default arguments may be present.
      The body of the function shall abide by the device language restrictions
      specified in
      `section 5.4, "Language restrictions for device functions" <https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html#sec:language.restrictions.kernels>`__.

   The original argument ``A`` is replaced with the sequence of elements returned
   in the tuple-like type for a call to ``sycl_deconstruct()`` on ``A``.
   Each element with a type that requires decomposition is recursively processed
   and replaced by its sequence of decomposed objects.
   The remaining types shall be device copyable types.

#. Otherwise, if ``P`` is a non-union aggregate type
   (`[dcl.init.aggr]p1 <https://eel.is/c++draft/dcl.init.aggr#1>`__)
   or a lambda closure type
   (`[expr.prim.lambda.closure]p1 <https://eel.is/c++draft/expr.prim.lambda.closure#1>`__),
   its subobject types determine whether ``P`` requires decomposition.
   If any subobject type requires decomposition as defined above, then ``P`` requires
   decomposition.
   Each subobject of ``A`` that has a type that requires decomposition is recursively
   processed and its replacement objects are sequenced after ``A`` in the kernel
   argument list.
   All subobjects that do not require decomposition shall be device copyable.
   If all subobject types require decomposition and ``P`` does not declare any bit-fields,
   ``A`` is removed from the kernel argument list (note that bit-fields are not
   subobjects).
   Otherwise (``P`` doesn't require decomposition, has at least one subobject type that
   does not require decomposition, or declares bit-fields), ``A`` may be passed as a
   bit-copyable argument or its non-decomposed subobjects and bit-fields may be passed
   as a sequence of distinct arguments in its place.
   (the choice to pass ``A`` with the storage for its decomposed subobjects bit-copied
   and reinitialized in the offload entry point function or to pass its non-decomposed
   members as distinct argument in place of ``A`` is unspecified;
   the intent is to allow the most efficient choice to be made based on the cost of
   passing ``A`` vs the cost of passing additional arguments).

#. Otherwise, if ``P`` is a trivially copyable non-class type, then ``A`` is passed as
   a bit-copyable argument.

#. Otherwise, if the `SYCL_DEVICE_COPYABLE` macro is predefined with a value of ``1``,
   ``P`` is a class type, and ``sycl::is_device_copyable_v<P>`` is true, ``P`` shall
   satisfy the constraints listed in
   `section 3.13.1, "Device copyable" <https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html#sec::device.copyable>`__.
   ``A`` is passed as a bit-copyable argument.

#. Otherwise, if ``P`` is a union type, then each of its non-static data members shall
   have a trivially copyable type, none of which requires decomposition.
   ``A`` is passed as a bit-copyable argument.

#. Otherwise, if ``P`` is a trivially copyable class type, none of its non-static data
   members shall require decomposition and its anonymous union members and non-static
   data members of union type shall satisfy the constraints on a union type described
   above. ``A`` is passed as a bit-copyable argument.

#. Otherwise, ``P`` is not a valid kernel argument type and the program is ill-formed.

If the sequence of replacement arguments contains an argument with a type that is,
or has as a subobject type, a reference type, a type that is prohibited in device
code (e.g., ``long double``), or a pointer to data member type (see
`SYCL WG issue 612 <https://github.com/KhronosGroup/SYCL-Docs/issues/612>`__),
then the program is ill-formed.

The constraints on union types are derived from explicit restrictions specified in
`section 5.4, "Language restrictions for device functions" <https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html#sec:language.restrictions.kernels>`__
and practical restrictions that are not clearly addressed in SYCL 2020 (see
`CMPLRLLVM-61883 <https://jira.devtools.intel.com/browse/CMPLRLLVM-61883>`__
for some previous discussion).

The sequence of possibly decomposed arguments is passed to the
``sycl_kernel_launch()`` function.
Each argument is passed as an xvalue and may be move-constructed from.
Destructors are invoked as usual for the argument/parameter pairs of the
``[[clang::sycl_kernel_entry_point]]`` attributed function and for the
``sycl_kernel_launch()`` function.

The body of a function declared with the ``[[clang::sycl_kernel_entry_point]]``
attribute is incorporated in the body of the synthesized offload kernel entry point
function.
In order to execute the incorporated statements, objects matching the original
sequence of arguments must be available.
For arguments that were not subject to decomposition, the matching parameter is
used.
For arguments that were subject to decomposition, the original argument is
reconstructed as a local variable from the parameters that correspond to the
decomposed sequence of arguments.
Within the variable initialization, parameters are referenced as xvalues and
may be move-constructed from.
Destructors are invoked for local variables used to reconstruct original
kernel arguments and may be invoked for the parameters of the synthesized offload
kernel entry point function (whether such destructors are called depends on the
calling convention used for the offload entry point function).
Per
`section 3.13.1, "Device copyable" <https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html#sec::device.copyable>`__,
it is unspecified whether destructors are invoked for these cases.

Consider the following example.

.. code-block:: C++

   #include <tuple>
   #include <sycl/sycl.hpp>

   template<typename KN, typename... Ts>
   void sycl_kernel_launch(const char* kn, Ts.. ts) { ... }

   template<typename KN, typename KT>
   [[clang::sycl_kernel_entry_point(KN)]]
   void kernel_entry_point(KT k) {
     k();
   }

   struct X {
     ~X();
     int dm;
   };
   template<> bool sycl::is_device_copyable_v<X> = true;

   struct special_type {
     ~special_type();
     std::tuple<int, X> sycl_deconstruct();
     static special_type sycl_reconstruct(int, X) noexcept;
   };

   struct aggregate {
     special_type sta[2];
   };

   struct kernel_name;

   void f() {
     int i;
     special_type st;
     aggregate a;
     auto k = [i, st, a] {};
     kernel_entry_point<kernel_name>(k);
   }

For host compilation, the body of ``kernel_entry_point<kernel_name>()`` is
replaced with synthesized code that performs kernel argument decomposition
and forwards the results to ``sycl_kernel_launch()``.
The synthesized code looks approximately as follows.
Structured bindings are used to illustrate the intent.
Note that some of this code does not conform to standard C++ (a lambda
closure type can not necessarily be decomposed as shown), but it hopefully
suffices to convey the intent.

.. code-block:: C++

   // KT is decltype(k) in f().
   void kernel_entry_point<kernel_name>(KT k) {
     // 'k' is a lambda closure type with captures that require decomposition.
     auto& [ i, st, a ] = k;
     // All of the captures of 'k' have been decomposed; it is eliminated as a kernel argument.
     // 'i' is a trivially copyable non-class type; no further decomposition required.
     // 'st' is of a type that opts into the decomposition protocol.
     auto&& [ st1, st2 ] = st.sycl_deconstruct();
     // 'st1' is a trivially copyable non-class type; no further decomposition required.
     // 'st2' is an explicitly device copyable type; no further decomposition required.
     // 'a' is an aggregate with a member that requires decomposition.
     auto& [ asta ] = a;
     // All of the data members of 'a' have been decomposed; it is eliminated as a kernel argument.
     // 'asta' is an aggregate with elements that require decomposition.
     auto& [ asta1, asta2 ] = asta;
     // All of the elements of 'asta' have been decomposed; it is eliminated as a kernel argument.
     // 'asta1' is of a type that opts into the decomposition protocol.
     auto&& [ asta1_1, asta1_2 ] = asta1.sycl_deconstruct();
     // All of the elements of 'asta1' have been decomposed; it is eliminated as a kernel argument.
     // 'asta1_1' is a trivially copyable non-class type; no further decomposition required.
     // 'asta1_2' is an explicitly device copyable type; no further decomposition required.
     // 'asta2' is of a type that opts into the decomposition protocol.
     auto&& [ asta2_1, asta2_2 ] = asta2.sycl_deconstruct();
     // All of the elements of 'asta2' have been decomposed; it is eliminated as a kernel argument.
     // 'asta2_1' is a trivially copyable non-class type; no further decomposition required.
     // 'asta2_2' is an explicitly device copyable type; no further decomposition required.

     // Pass the decomposed arguments, all of which satisfy the SYCL 2020 device copyable
     // and kernel argument requirements, to the sycl_kernel_launch() function.
     sycl_kernel_launch<kernel_name>("kernel_name",
                                     std::move(i),
                                     std::move(st1), std::move(st2),
                                     std::move(asta1_1), std::move(asta1_2),
                                     std::move(asta2_1), std::move(asta2_2));

     // Destructor runs for 'k'.
   } 

For device compilation, an offload kernel entry point function is synthesized that looks
approximately as follows.
Aggregate initialization is used to illustrate the intent with the acknowledgement
that, for example, lambda closure types cannot be constructed with aggregate
initialization in standard C++.
Again, hopefully the intent is clear.

.. code-block:: C++

   void offload_kernel_entry_point<kernel_name>(
     int i, int st1, X st2, int asta1_1, X asta1_2, int asta2_1, X asta2_2)
   {
     // KT is decltype(k) in f().
     KT k = {
       i,
       special_type::sycl_reconstruct(std::move(st1), std::move(st2)),
       { // 'a'
         { // 'sta'
           special_type::sycl_reconstruct(std::move(asta1_1), std::move(asta1_2)),
           special_type::sycl_reconstruct(std::move(asta2_1), std::move(asta2_2))
         }
       }
     };
     k();
     // Destructors run for 'k', 'k.st', 'k.a.sta[0]', and 'k.a.sta[1]'.
     // Destructors may run for 'st2', 'asta1_2', and 'asta2_2'.
   } 

The ``sycl_kernel_launch()`` function is then responsible for enqueuing the kernel
invocation and arranging for each of its function arguments to be bit-copied to the
device (or, for special types used to implement decomposition for types that require
special handling, like ``local_accessor``, copied to the device in an appropriate way.
Such special requirements are handled by the SYCL RT implementation).

This design is not intended to address all possible transformations of kernel
arguments that might be desired for performance optimization purposes.
For example, a transformation to coalesce local accessors in order to perform
a single allocation request rather than one for each local accessor might be desirable.
This design leaves such transformations to the SYCL RT to implement via its own means.
