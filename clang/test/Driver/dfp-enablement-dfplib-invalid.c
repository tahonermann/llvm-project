// This test exercises invalid option arguments for the -dfplib option.

// RUN: not %clang -### -target x86_64-unknown-linux-gnu -dfplib=invalid-dfplib1 %s 2>&1 | FileCheck --check-prefix=INVALID1 %s
// INVALID1: error: invalid value 'invalid-dfplib1' in '-dfplib=invalid-dfplib1'

// RUN: not %clang -### -target x86_64-unknown-linux-gnu --dfplib=invalid-dfplib2 %s 2>&1 | FileCheck --check-prefix=INVALID2 %s
// INVALID2: error: invalid value 'invalid-dfplib2' in '--dfplib=invalid-dfplib2'

// RUN: not %clang -### -target x86_64-unknown-linux-gnu --dfplib invalid-dfplib3 %s 2>&1 | FileCheck --check-prefix=INVALID3 %s
// INVALID3: error: invalid value 'invalid-dfplib3' in '-dfplib=invalid-dfplib3'

// RUN: not %clang -### -target x86_64-unknown-linux-gnu -dfplib=LIBGCC %s 2>&1 | FileCheck --check-prefix=CASEMISMATCH %s
// CASEMISMATCH: error: invalid value 'LIBGCC' in '-dfplib=LIBGCC'
