/*	@(#)abort.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: abort.s,v 1.2 87/04/05 15:41:06 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>

/* C library -- abort(3)
 *
 * Abort(3) is to execute an instruction which is illegal in user mode
 * and resulting in the process getting a SIGILL.
 */
LEAF(abort)
	.set noreorder
	c0	0	# should be illegal to do in user mode
	nop
	.set reorder
	li	v0,0
	RET
END(abort)
