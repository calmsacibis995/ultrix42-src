/*	@(#)index.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: index.s,v 1.1 87/02/16 11:17:29 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/asm.h>
#include <mips/regdef.h>

LEAF(index)
1:	lb	a2,0(a0)
	addu	a0,1
	beq	a2,a1,2f
	bne	a2,zero,1b
	move	v0,zero
	j	ra

2:	subu	v0,a0,1
	j	ra
.end	index
