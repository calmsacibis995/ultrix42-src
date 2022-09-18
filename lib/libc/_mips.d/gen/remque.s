/*	@(#)remque.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: remque.s,v 1.1 87/02/16 11:17:34 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>

/* remque(entry) */

LEAF(remque)
	lw	v0, 0(a0)	# ep->link
	lw	v1, 4(a0)	# ep->rlink
	sw	v0, 0(v1)	# ep->rlink->link = ep->link
	sw	v1, 4(v0)	# ep->link->rlink = ep->rlink
	j	ra
.end	remque
