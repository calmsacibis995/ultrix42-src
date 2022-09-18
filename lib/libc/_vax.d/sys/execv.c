/* @(#)execv.c	4.1	ULTRIX	7/3/90 */

#include "SYS.h"

ENTRY(execv)
	.globl	___environ
	pushl	___environ
	pushl	8(ap)
	pushl	4(ap)
	calls	$3,_execve
	ret			# execv(file, argv)
