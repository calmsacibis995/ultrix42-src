/* @(#)_open.s	4.1	ULTRIX	7/3/90 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include "SYS.h"

SYSCALLV(_open)
	RET
.end	_open
