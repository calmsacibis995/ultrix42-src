/*	@(#)sigreturn.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sigreturn.s,v 1.1 87/02/16 11:21:08 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#ifdef PROF
#undef LEAF
#define	LEAF(x)						\
	.globl	x;					\
	.ent	x,0;					\
x:;							\
	.frame	sp,0,ra					\
	subu	sp,4					\
	sw	AT,0(sp)
#endif

SYSCALL(sigreturn)
#ifdef PROF
	lw	AT,0(sp)
	addu	sp,4
#endif
	RET
.end sigreturn
