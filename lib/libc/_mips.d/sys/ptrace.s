/*	@(#)ptrace.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: ptrace.s,v 1.2 87/03/24 07:17:09 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

.extern	errno 4

LEAF(ptrace)
	sw	zero,errno
	li	v0,SYS_ptrace
	syscall
	bne	a3,zero,err
	RET

err:
	j	_cerror
.end ptrace
