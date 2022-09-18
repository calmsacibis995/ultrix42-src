/* waitpid.c  */

#include "SYS.h"

SYSCALL(waitpid)
	tstl	8(ap)
	jeql	1f
	movl	r1,*8(ap)
1:
	ret
