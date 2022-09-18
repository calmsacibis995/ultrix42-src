/*	fabs.s	4.1	83/06/27	*/
/* fabs - floating absolute value */

	.globl	fabs
	.align	2
_fabs:
	.word	0
	movd	4(ap),r0
	bgeq	1f
	mnegd	r0,r0
1:
	ret
