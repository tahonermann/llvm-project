; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 5
; RUN: llc < %s -march mips -fast-isel -relocation-model pic | FileCheck %s -check-prefixes=MIPS-32
; RUN: llc < %s -march mips64 -fast-isel -relocation-model pic | FileCheck %s -check-prefixes=MIPS-64
; RUN: llc < %s -march mips -mcpu mips32r6 -fast-isel -relocation-model pic | FileCheck %s -check-prefixes=MIPS-32-R6
; RUN: llc < %s -march mips -mcpu mips32r6 -mattr +strict-align -fast-isel -relocation-model pic | FileCheck %s -check-prefixes=MIPS-32-R6-STRICT
; RUN: llc < %s -march mips64 -mcpu mips64r6 -fast-isel -relocation-model pic | FileCheck %s -check-prefixes=MIPS-64-R6
; RUN: llc < %s -march mips64 -mcpu mips64r6 -mattr +strict-align -fast-isel -relocation-model pic | FileCheck %s -check-prefixes=MIPS-64-R6-STRICT

@var = external global i32, align 1

; FastISel should bail on the underaligned load and store, except on r6 with non-strict alignment.
define dso_local ccc i32 @__start() {
; MIPS-32-LABEL: __start:
; MIPS-32:       # %bb.0:
; MIPS-32-NEXT:    lui $2, %hi(_gp_disp)
; MIPS-32-NEXT:    addiu $2, $2, %lo(_gp_disp)
; MIPS-32-NEXT:    addu $1, $2, $25
; MIPS-32-NEXT:    lw $1, %got(var)($1)
; MIPS-32-NEXT:    lwl $2, 0($1)
; MIPS-32-NEXT:    lwr $2, 3($1)
; MIPS-32-NEXT:    addiu $3, $zero, 42
; MIPS-32-NEXT:    swl $3, 0($1)
; MIPS-32-NEXT:    jr $ra
; MIPS-32-NEXT:    swr $3, 3($1)
;
; MIPS-64-LABEL: __start:
; MIPS-64:       # %bb.0:
; MIPS-64-NEXT:    lui $1, %hi(%neg(%gp_rel(__start)))
; MIPS-64-NEXT:    daddu $1, $1, $25
; MIPS-64-NEXT:    daddiu $1, $1, %lo(%neg(%gp_rel(__start)))
; MIPS-64-NEXT:    ld $1, %got_disp(var)($1)
; MIPS-64-NEXT:    lwl $2, 0($1)
; MIPS-64-NEXT:    lwr $2, 3($1)
; MIPS-64-NEXT:    addiu $3, $zero, 42
; MIPS-64-NEXT:    swl $3, 0($1)
; MIPS-64-NEXT:    jr $ra
; MIPS-64-NEXT:    swr $3, 3($1)
;
; MIPS-32-R6-LABEL: __start:
; MIPS-32-R6:       # %bb.0:
; MIPS-32-R6-NEXT:    lui $2, %hi(_gp_disp)
; MIPS-32-R6-NEXT:    addiu $2, $2, %lo(_gp_disp)
; MIPS-32-R6-NEXT:    addu $1, $2, $25
; MIPS-32-R6-NEXT:    lw $1, %got(var)($1)
; MIPS-32-R6-NEXT:    lw $2, 0($1)
; MIPS-32-R6-NEXT:    addiu $3, $zero, 42
; MIPS-32-R6-NEXT:    jr $ra
; MIPS-32-R6-NEXT:    sw $3, 0($1)
;
; MIPS-32-R6-STRICT-LABEL: __start:
; MIPS-32-R6-STRICT:       # %bb.0:
; MIPS-32-R6-STRICT-NEXT:    lui $2, %hi(_gp_disp)
; MIPS-32-R6-STRICT-NEXT:    addiu $2, $2, %lo(_gp_disp)
; MIPS-32-R6-STRICT-NEXT:    addu $1, $2, $25
; MIPS-32-R6-STRICT-NEXT:    lw $1, %got(var)($1)
; MIPS-32-R6-STRICT-NEXT:    lbu $2, 0($1)
; MIPS-32-R6-STRICT-NEXT:    lbu $3, 1($1)
; MIPS-32-R6-STRICT-NEXT:    lbu $4, 3($1)
; MIPS-32-R6-STRICT-NEXT:    lbu $5, 2($1)
; MIPS-32-R6-STRICT-NEXT:    addiu $6, $zero, 42
; MIPS-32-R6-STRICT-NEXT:    sb $zero, 2($1)
; MIPS-32-R6-STRICT-NEXT:    sb $6, 3($1)
; MIPS-32-R6-STRICT-NEXT:    sb $zero, 0($1)
; MIPS-32-R6-STRICT-NEXT:    sb $zero, 1($1)
; MIPS-32-R6-STRICT-NEXT:    sll $1, $5, 8
; MIPS-32-R6-STRICT-NEXT:    or $1, $1, $4
; MIPS-32-R6-STRICT-NEXT:    sll $3, $3, 16
; MIPS-32-R6-STRICT-NEXT:    sll $2, $2, 24
; MIPS-32-R6-STRICT-NEXT:    or $2, $2, $3
; MIPS-32-R6-STRICT-NEXT:    jr $ra
; MIPS-32-R6-STRICT-NEXT:    or $2, $2, $1
;
; MIPS-64-R6-LABEL: __start:
; MIPS-64-R6:       # %bb.0:
; MIPS-64-R6-NEXT:    lui $1, %hi(%neg(%gp_rel(__start)))
; MIPS-64-R6-NEXT:    daddu $1, $1, $25
; MIPS-64-R6-NEXT:    daddiu $1, $1, %lo(%neg(%gp_rel(__start)))
; MIPS-64-R6-NEXT:    ld $1, %got_disp(var)($1)
; MIPS-64-R6-NEXT:    lw $2, 0($1)
; MIPS-64-R6-NEXT:    addiu $3, $zero, 42
; MIPS-64-R6-NEXT:    jr $ra
; MIPS-64-R6-NEXT:    sw $3, 0($1)
;
; MIPS-64-R6-STRICT-LABEL: __start:
; MIPS-64-R6-STRICT:       # %bb.0:
; MIPS-64-R6-STRICT-NEXT:    lui $1, %hi(%neg(%gp_rel(__start)))
; MIPS-64-R6-STRICT-NEXT:    daddu $1, $1, $25
; MIPS-64-R6-STRICT-NEXT:    daddiu $1, $1, %lo(%neg(%gp_rel(__start)))
; MIPS-64-R6-STRICT-NEXT:    ld $1, %got_disp(var)($1)
; MIPS-64-R6-STRICT-NEXT:    lbu $2, 0($1)
; MIPS-64-R6-STRICT-NEXT:    lbu $3, 1($1)
; MIPS-64-R6-STRICT-NEXT:    lbu $4, 3($1)
; MIPS-64-R6-STRICT-NEXT:    lbu $5, 2($1)
; MIPS-64-R6-STRICT-NEXT:    addiu $6, $zero, 42
; MIPS-64-R6-STRICT-NEXT:    sb $zero, 2($1)
; MIPS-64-R6-STRICT-NEXT:    sb $6, 3($1)
; MIPS-64-R6-STRICT-NEXT:    sb $zero, 0($1)
; MIPS-64-R6-STRICT-NEXT:    sb $zero, 1($1)
; MIPS-64-R6-STRICT-NEXT:    sll $1, $5, 8
; MIPS-64-R6-STRICT-NEXT:    or $1, $1, $4
; MIPS-64-R6-STRICT-NEXT:    sll $3, $3, 16
; MIPS-64-R6-STRICT-NEXT:    sll $2, $2, 24
; MIPS-64-R6-STRICT-NEXT:    or $2, $2, $3
; MIPS-64-R6-STRICT-NEXT:    jr $ra
; MIPS-64-R6-STRICT-NEXT:    or $2, $2, $1
    %1 = load i32, ptr @var, align 1
    store i32 42, ptr @var, align 1
    ret i32 %1
}
