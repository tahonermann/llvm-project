// DEFINE: %{common_opts_x86} = -###  \
// DEFINE: -fexperimental-decimal-floating-point

// RUN: %clang %{common_opts_x86} --target=x86_64-unknown-linux-gnu \
// RUN: %s 2>&1 | FileCheck -check-prefix=BID %s

// RUN: %clang %{common_opts_x86} --target=x86_64-unknown-linux-gnu \
// RUN: -mdecimal-float-abi=libgcc:bid %s 2>&1 \
// RUN: | FileCheck -check-prefix=BID %s

// RUN: %clang %{common_opts_x86} --target=x86_64-windows \
// RUN: %s 2>&1 | FileCheck -check-prefix=NONE %s

// RUN: %clang %{common_opts_x86} --target=x86_64-windows \
// RUN: -mdecimal-float-abi=libgcc:bid %s 2>&1 \
// RUN: | FileCheck -check-prefix=BID %s

// RUN: %clang %{common_opts_x86} --target=x86_64-windows-gnu \
// RUN: -mdecimal-float-abi=libgcc:bid %s 2>&1 | FileCheck -check-prefix=BID %s

// RUN: %clang %{common_opts_x86} --target=x86_64-unknown-linux  \
// RUN: -mdecimal-float-abi=libgcc:bid %s 2>&1 \
// RUN: | FileCheck -check-prefix=BID %s

// RUN: not %clang %{common_opts_x86} --target=x86_64-unknown-linux-gnu \
// RUN: -mdecimal-float-abi=libgcc:dpd %s 2>&1 \
// RUN: | FileCheck -check-prefix=ERROR_DPD %s

// RUN: not %clang %{common_opts_x86} --target=x86_64-unknown-linux-gnu \
// RUN: -mdecimal-float-abi=hard %s 2>&1 \
// RUN: | FileCheck -check-prefix=ERROR_HARD %s

// RUN not %clang %{common_opts_x86} --target=x86_64-windows \
// RUN -mdecimal-float-abi=libgcc:bid %s 2>&1 \
// RUN | FileCheck -check-prefix=ERROR_BID %s

// RUN: %clang %{common_opts_x86} --target=x86_64-unknown-linux-gnu \
// RUN: -mdecimal-float-abi=libgcc:dpd %s 2>&1 \
// RUN: | FileCheck -check-prefix=ERROR_DPD %s

// BID: "-mdecimal-float-abi=libgcc:bid" {{.*}}
// NONE: "-mdecimal-float-abi=none" {{.*}}

// ERROR_DPD: unsupported option '-mdecimal-float-abi=libgcc:dpd' for target 'x86_64-unknown-linux-gnu'
// ERROR_HARD: unsupported option '-mdecimal-float-abi=hard' for target 'x86_64-unknown-linux-gnu'
// ERROR_BID: unsupported option '-mdecimal-float-abi=libgcc:bid' for target 'x86_64-unknown-windows-msvc'

