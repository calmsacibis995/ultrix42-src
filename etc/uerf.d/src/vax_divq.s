LL0:
	.data
	.text
	.align	2
	.globl	_divq
_divq:
	.word	L12
	jbr 	L14
L15:
	ediv 4(ap),8(ap),*16(ap),*20(ap)
	ret
	.set	L12,0x0
L14:
	jbr 	L15
	.data
