/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* @(#)_wait.s	4.1	ULTRIX	7/3/90 */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include "SYS.h"

LEAF(_wait)
	move	a1,zero
	move	a2,zero
	li	v0,SYS_wait3
	syscall
	bne	a3,zero,err
	RET

err:
	j	_cerror
.end	_wait
