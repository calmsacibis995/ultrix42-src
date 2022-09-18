/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* @(#)_ptrace.s	4.1	ULTRIX	7/3/90 */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include "SYS.h"

.comm	errno,4

LEAF(_ptrace)
	sw	zero,errno
	li	v0,SYS_ptrace
	syscall
	bne	a3,zero,err
	RET

err:
	j	_cerror
.end	_ptrace
