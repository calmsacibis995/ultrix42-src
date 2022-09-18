/*	@(#)kopt.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: kopt.s,v 1.3 87/04/15 17:13:59 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include <sysmips.h>

/*
 * kopt is implemented via the SYS_mips system call. The 3 arguments are
 * shuffled back so the MIPS_KOPT vector can be put in the first argument.
 * The SYS_mips system call takes 5 arguments and unused arguments must
 * have zero values.
 */
LEAF(kopt)
	subu	sp,8
	sw	zero,16(sp)
	move	a3,a2
	move	a2,a1
	move	a1,a0
	li	a0,MIPS_KOPT
	li	v0,SYS_sysmips
	syscall
	addu	sp,8
	beq	a3,zero,9f
	j	_cerror
9:
	RET
.end kopt
