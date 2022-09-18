/*	@(#)rindex.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: rindex.s,v 1.1 87/02/16 11:17:34 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>

LEAF(rindex)
	move	v0,zero
1:	lb	a3,0(a0)
	addu	a0,1
	bne	a3,a1,2f
	subu	v0,a0,1
2:	bne	a3,zero,1b
	j	ra
.end	rindex
