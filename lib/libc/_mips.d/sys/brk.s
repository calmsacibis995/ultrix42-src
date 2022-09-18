/*	@(#)brk.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: brk.s,v 1.1 87/02/16 11:19:45 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

	.globl	_curbrk
	.globl	_minbrk

LEAF(_brk)
	b	1f
.end _brk

LEAF(brk)
	lw	v0,_minbrk
	bgeu	a0,v0,1f
	move	a0,v0
1:
	li	v0,SYS_brk
	syscall
	bne	a3,zero,err
	sw	a0,_curbrk
	move	v0,zero
	RET

err:
	j	_cerror
.end brk
