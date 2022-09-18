/*	@(#)m_wait3.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: m_wait3.s,v 1.2 87/04/15 17:14:05 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include <sysmips.h>

/*
 * mips_wait3 is implemented via the SYS_mips system call. The 4 arguments
 * are shuffled back so the MIPS_WAIT3 vector can be put in the first
 * argument.  The SYS_mips system call takes 5 arguments and unused arguments
 * must have zero values (in this case all are used).
 */
LEAF(mips_wait3)
	subu	sp,8		# The fifth argument goes at sp+16 so room on
	sw	a3,16(sp)	#  the stack needs to be made for it.
	move	a3,a2
	move	a2,a1
	move	a1,a0
	li	a0,MIPS_WAIT3
	li	v0,SYS_sysmips
	syscall
	addu	sp,8
	beq	a3,zero,9f
	j	_cerror
9:
	RET
.end mips_wait3
