/*	@(#)ntohs.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: ntohs.s,v 1.1 87/02/16 11:18:30 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/* hostorder = ntohs(netorder) */

#include <mips/regdef.h>
#include <mips/asm.h>

LEAF(ntohs)
#ifdef MIPSEL
	# start with ab
	sll	v0,a0,8			# ?ab0
	srl	v1,a0,8			# 0??a
	and	v1,0xff			# 000a
	or	v0,v1			# ??ba
	and	v0,0xffff		# 00ba
#else
	and	v0,a0,0xffff
#endif
	j	ra
.end ntohs
