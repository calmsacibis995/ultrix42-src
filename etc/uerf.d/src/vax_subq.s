LL0:
	.data
	.text
	.align	2
	.globl	_subq
_subq:
	.word	L12
	jbr 	L14
L15:
	subl2 4(ap),*12(ap)
	sbwc 8(ap),*16(ap)
	jvs		L17
	jgeq	L16
L17:
	movl $1, r0
	ret
L16:
	clrl r0
	ret
	.set	L12,0x0
L14:
	jbr 	L15
	.data
