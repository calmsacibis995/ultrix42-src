/*	@(#)ntohl.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: ntohl.s,v 1.1 87/02/16 11:18:29 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/* hostorder = ntohl(netorder) */

#include <mips/regdef.h>
#include <mips/asm.h>

LEAF(ntohl)
#ifdef MIPSEL
	# start with abcd
	sll	v0,a0,24	# d000
	srl	v1,a0,24	# 000a
	or	v0,v1		# d00a
	srl	t0,a0,8		# 0abc
	and	t1,t0,0xff00	# 00b0
	and	v1,t0,0xff	# 000c
	sll	v1,16		# 0c00
	or	v1,t1		# 0cb0
	or	v0,v1		# dcba
#else
	move	v0,a0
#endif
	j	ra
.end ntohl
