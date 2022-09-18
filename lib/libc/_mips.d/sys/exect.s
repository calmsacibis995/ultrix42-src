/*	@(#)exect.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: exect.s,v 1.1 87/02/16 11:19:55 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

LEAF(exect)
	li	a0,2
	la	a1,errmsg
	li	a2,22
	li	v0,SYS_write
	syscall
	jal	abort
.end exect

.rdata
errmsg:
	.ascii	"exect not implemented\n"
