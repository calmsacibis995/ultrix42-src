/* 
 *		@(#)msgctl.s	4.1	(ULTRIX)	7/3/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *                                                                      *
 ***********************************************************************/

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

SYSCALL(msgctl)
	RET
.end msgctl
