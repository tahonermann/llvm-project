// This test validates the supported DFP run-time library implementations for
// Linux on x86.

// RUN: %clang -### -target x86_64-unknown-linux-gnu -fexperimental-decimal-floating-point -dfplib=default %s 2>&1 | FileCheck --check-prefix=DPFLIB_DEFAULT %s
// DPFLIB_DEFAULT-NOT: {{error|warning}}:
// DPFLIB_DEFAULT: "-lgcc"

// RUN: not %clang -### -target x86_64-unknown-linux-gnu -fexperimental-decimal-floating-point -dfplib=none %s 2>&1 | FileCheck --check-prefix=DPFLIB_NONE %s
// DPFLIB_NONE: error: unsupported decimal floating-point library 'none' for target

// RUN: not %clang -### -target x86_64-unknown-linux-gnu -fexperimental-decimal-floating-point -dfplib=compiler-rt %s 2>&1 | FileCheck --check-prefix=DPFLIB_COMPILER_RT %s
// DPFLIB_COMPILER_RT: error: unsupported decimal floating-point library 'none' for target

// RUN: %clang -### -target x86_64-unknown-linux-gnu -fexperimental-decimal-floating-point -dfplib=libgcc %s 2>&1 | FileCheck --check-prefix=DPFLIB_LIBGCC %s
// DPFLIB_LIBGCC-NOT: {{error|warning}}:
// DPFLIB_LIBGCC: "-lgcc"

// RUN: %clang -### -target x86_64-unknown-linux-gnu -fexperimental-decimal-floating-point -dfplib=libgcc:bid %s 2>&1 | FileCheck --check-prefix=DPFLIB_LIBGCC_BID %s
// DPFLIB_LIBGCC_BID-NOT: {{error|warning}}:
// DPFLIB_LIBGCC_BID: "-lgcc"

// RUN: not %clang -### -target x86_64-unknown-linux-gnu -fexperimental-decimal-floating-point -dfplib=libgcc:dpd %s 2>&1 | FileCheck --check-prefix=DPFLIB_LIBGCC_DPD %s
// DPFLIB_LIBGCC_DPD: error: unsupported decimal floating-point library 'none' for target
