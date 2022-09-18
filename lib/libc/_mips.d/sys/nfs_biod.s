/*	@(#)nfs_biod.s	4.1	(ULTRIX)	7/3/90				      */
/*	@(#)getmnt.s	4.1	(ULTRIX)	11/23/87 */


#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

SYSCALL(nfs_biod)
	RET
.end nfs_biod
