/*	@(#)stopcpu.s	4.1	(ULTRIX)	7/3/90	*/
#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

SYSCALL(stopcpu)
	RET
.end stopcpu
