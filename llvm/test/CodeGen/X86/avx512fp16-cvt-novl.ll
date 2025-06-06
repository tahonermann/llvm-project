; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 5
; RUN: llc < %s -mtriple=x86_64 -mattr=+avx512fp16 | FileCheck %s

define <2 x half> @vector_sint64ToHalf(<2 x i64> %int64) {
; CHECK-LABEL: vector_sint64ToHalf:
; CHECK:       # %bb.0:
; CHECK-NEXT:    # kill: def $xmm0 killed $xmm0 def $zmm0
; CHECK-NEXT:    vcvtqq2ph %zmm0, %xmm0
; CHECK-NEXT:    vzeroupper
; CHECK-NEXT:    retq
    %fp16 = sitofp <2 x i64> %int64 to <2 x half>
    ret <2 x half> %fp16
}

define <4 x half> @vector_sint32ToHalf(<4 x i32> %int32) {
; CHECK-LABEL: vector_sint32ToHalf:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vextractps $3, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm1, %xmm1
; CHECK-NEXT:    vextractps $2, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm2, %xmm2
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm1 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; CHECK-NEXT:    vextractps $1, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm3, %xmm2
; CHECK-NEXT:    vmovd %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm3, %xmm0
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3]
; CHECK-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0],xmm1[0],zero,zero
; CHECK-NEXT:    retq
    %fp16 = sitofp <4 x i32> %int32 to <4 x half>
    ret <4 x half> %fp16
}

define <8 x half> @vector_sint16ToHalf(<8 x i16> %int16) {
; CHECK-LABEL: vector_sint16ToHalf:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vpextrw $7, %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm1, %xmm1
; CHECK-NEXT:    vpextrw $6, %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm2, %xmm2
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm1 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; CHECK-NEXT:    vpextrw $5, %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm3, %xmm2
; CHECK-NEXT:    vpextrw $4, %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm3, %xmm3
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm2 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3]
; CHECK-NEXT:    vpunpckldq {{.*#+}} xmm1 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; CHECK-NEXT:    vpextrw $3, %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm2
; CHECK-NEXT:    vpextrw $2, %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm3
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm2 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3]
; CHECK-NEXT:    vpextrw $1, %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm3
; CHECK-NEXT:    vmovw %xmm0, %eax
; CHECK-NEXT:    cwtl
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm0
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3]
; CHECK-NEXT:    vpunpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; CHECK-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; CHECK-NEXT:    retq
    %fp16 = sitofp <8 x i16> %int16 to <8 x half>
    ret <8 x half> %fp16
}

define <2 x half> @vector_uint64ToHalf(<2 x i64> %int64) {
; CHECK-LABEL: vector_uint64ToHalf:
; CHECK:       # %bb.0:
; CHECK-NEXT:    # kill: def $xmm0 killed $xmm0 def $zmm0
; CHECK-NEXT:    vcvtuqq2ph %zmm0, %xmm0
; CHECK-NEXT:    vzeroupper
; CHECK-NEXT:    retq
    %fp16 = uitofp <2 x i64> %int64 to <2 x half>
    ret <2 x half> %fp16
}

; define <4 x half> @vector_uint32ToHalf(<4 x i32> %int32) {
;     %fp16 = uitofp <4 x i32> %int32 to <4 x half>
;     ret <4 x half> %fp16
; }

define <8 x half> @vector_uint16ToHalf(<8 x i16> %int16) {
; CHECK-LABEL: vector_uint16ToHalf:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vpextrw $7, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm1, %xmm1
; CHECK-NEXT:    vpextrw $6, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm2, %xmm2
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm1 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; CHECK-NEXT:    vpextrw $5, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm3, %xmm2
; CHECK-NEXT:    vpextrw $4, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm3, %xmm3
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm2 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3]
; CHECK-NEXT:    vpunpckldq {{.*#+}} xmm1 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; CHECK-NEXT:    vpextrw $3, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm2
; CHECK-NEXT:    vpextrw $2, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm3
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm2 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3]
; CHECK-NEXT:    vpextrw $1, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm3
; CHECK-NEXT:    vpextrw $0, %xmm0, %eax
; CHECK-NEXT:    vcvtsi2sh %eax, %xmm4, %xmm0
; CHECK-NEXT:    vpunpcklwd {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3]
; CHECK-NEXT:    vpunpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; CHECK-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; CHECK-NEXT:    retq
    %fp16 = uitofp <8 x i16> %int16 to <8 x half>
    ret <8 x half> %fp16
}
