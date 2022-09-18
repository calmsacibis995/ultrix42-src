/* @(#)_getpgrp.s	4.1	ULTRIX	7/3/90 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include "SYS.h"

SYSCALLV(_getpgrp)
	RET		# pgrp = getpgrp(pid);
.end	_getpgrp
