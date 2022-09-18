/*	@(#)Ovfork.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: Ovfork.s,v 1.1 87/02/16 11:19:37 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

/*
 * pid = vfork();
 *
 * v1 == 0 in parent process, v1 == 1 in child process.
 * v0 == pid of child in parent, v0 == pid of parent in child.
 */

SYSCALL(vfork)
	beq	v1,zero,parent
	move	v0,zero
parent:
	RET
.end vfork
