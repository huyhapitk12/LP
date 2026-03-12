	.file	"__lp_ci_smoke.c"
	.text
	.section .rdata,"dr"
.LC0:
	.ascii "%lld\12\0"
	.section	.text.startup,"x"
	.p2align 4
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	subq	$40, %rsp
	.seh_stackalloc	40
	.seh_endprologue
	call	__main
	movl	$42, %edx
	leaq	.LC0(%rip), %rcx
	call	printf
	xorl	%eax, %eax
	addq	$40, %rsp
	ret
	.seh_endproc
	.globl	lp_current_exp
	.bss
	.align 16
lp_current_exp:
	.space 16
	.globl	lp_exp_depth
	.align 4
lp_exp_depth:
	.space 4
	.globl	lp_exp_env
	.align 32
lp_exp_env:
	.space 16384
	.def	__main;	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev8, Built by MSYS2 project) 15.2.0"
	.def	printf;	.scl	2;	.type	32;	.endef
