/*	@(#)insque.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: insque.s,v 1.1 87/02/16 11:17:31 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>

/*
 * insque(ep, pp)
 * ep is new element, pp is predecessor
 */
LEAF(insque)
	lw	v0, 0(a1)	# pp->link
	sw	v0, 0(a0)	# ep->link = pp->link
	sw	a1, 4(a0)	# ep->rlink = pp
	sw	a0, 4(v0)	# pp->link->rlink = ep
	sw	a0, 0(a1)	# pp->link = ep
	j	ra
.end	insque
