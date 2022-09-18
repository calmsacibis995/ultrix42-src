/*#@(#)udiv.c	4.1	Ultrix	7/17/90*/
# $Header: udiv.c,v 1.4 84/05/19 11:40:21 powell Exp $
	.align	1
	.globl	_runtime__udiv
_runtime__udiv:	.word	0
	pushl	8(ap)
	pushl	4(ap)
	calls	$2,udiv
	ret
	.globl	_runtime__umod
_runtime__umod:	.word	0
	pushl	8(ap)
	pushl	4(ap)
	calls	$2,udiv
	mull2	8(ap),r0
	subl3	r0,4(ap),r0
	ret
