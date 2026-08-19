	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #192
	stp	x29, x30, [sp, #176]            ; 16-byte Folded Spill
	add	x29, sp, #176
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, ___stack_chk_guard@GOTPAGE
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
	ldr	x8, [x8]
	stur	x8, [x29, #-8]
	stur	wzr, [x29, #-64]
	mov	x0, #0                          ; =0x0
	bl	_time
	mov	x8, x0
	sub	x0, x29, #80
	stur	x8, [x29, #-80]
	bl	_localtime
	mov	x1, x0
	add	x0, sp, #40
	mov	x2, #56                         ; =0x38
	bl	_memcpy
	ldr	w8, [sp, #60]
	add	w8, w8, #1900
	stur	w8, [x29, #-72]
	adrp	x0, l_.str@PAGE
	add	x0, x0, l_.str@PAGEOFF
	bl	_printf
	adrp	x8, ___stdinp@GOTPAGE
	ldr	x8, [x8, ___stdinp@GOTPAGEOFF]
	ldr	x2, [x8]
	sub	x0, x29, #58
	str	x0, [sp, #24]                   ; 8-byte Folded Spill
	mov	w1, #50                         ; =0x32
	bl	_fgets
	ldr	x0, [sp, #24]                   ; 8-byte Folded Reload
	adrp	x1, l_.str.1@PAGE
	add	x1, x1, l_.str.1@PAGEOFF
	bl	_strcspn
	ldr	x8, [sp, #24]                   ; 8-byte Folded Reload
	add	x8, x8, x0
	strb	wzr, [x8]
	adrp	x0, l_.str.2@PAGE
	add	x0, x0, l_.str.2@PAGEOFF
	bl	_printf
	mov	x9, sp
	sub	x8, x29, #68
	str	x8, [x9]
	adrp	x0, l_.str.3@PAGE
	add	x0, x0, l_.str.3@PAGEOFF
	bl	_scanf
	ldr	x10, [sp, #24]                  ; 8-byte Folded Reload
	ldur	w8, [x29, #-72]
	ldur	w11, [x29, #-68]
	mov	w9, #100                        ; =0x64
	subs	w9, w9, w11
	add	w8, w8, w9
	str	w8, [sp, #36]
	ldr	w8, [sp, #36]
                                        ; kill: def $x8 killed $w8
	mov	x9, sp
	str	x10, [x9]
	str	x8, [x9, #8]
	adrp	x0, l_.str.4@PAGE
	add	x0, x0, l_.str.4@PAGEOFF
	bl	_printf
	adrp	x0, l_.str.5@PAGE
	add	x0, x0, l_.str.5@PAGEOFF
	bl	_printf
	ldur	w8, [x29, #-68]
	str	w8, [sp, #32]
	b	LBB0_1
LBB0_1:                                 ; =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #32]
	tbnz	w8, #31, LBB0_4
	b	LBB0_2
LBB0_2:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldr	w8, [sp, #32]
                                        ; kill: def $x8 killed $w8
	mov	x9, sp
	str	x8, [x9]
	adrp	x0, l_.str.6@PAGE
	add	x0, x0, l_.str.6@PAGEOFF
	bl	_printf
	b	LBB0_3
LBB0_3:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldr	w8, [sp, #32]
	subs	w8, w8, #10
	str	w8, [sp, #32]
	b	LBB0_1
LBB0_4:
	ldur	x9, [x29, #-8]
	adrp	x8, ___stack_chk_guard@GOTPAGE
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
	ldr	x8, [x8]
	subs	x8, x8, x9
	b.eq	LBB0_6
	b	LBB0_5
LBB0_5:
	bl	___stack_chk_fail
LBB0_6:
	mov	w0, #0                          ; =0x0
	ldp	x29, x30, [sp, #176]            ; 16-byte Folded Reload
	add	sp, sp, #192
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Enter your name: "

l_.str.1:                               ; @.str.1
	.asciz	"\n"

l_.str.2:                               ; @.str.2
	.asciz	"Enter your age: "

l_.str.3:                               ; @.str.3
	.asciz	"%d"

l_.str.4:                               ; @.str.4
	.asciz	"Hello, %s! You will turn 100 years old in the year %d.\n"

l_.str.5:                               ; @.str.5
	.asciz	"Counting down your age:\n"

l_.str.6:                               ; @.str.6
	.asciz	"%d...\n"

.subsections_via_symbols
