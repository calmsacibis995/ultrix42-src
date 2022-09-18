/*	@(#)sbrk.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sbrk.s,v 1.1 87/02/16 11:20:53 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

	.globl	end
	.globl	_minbrk
	.globl	_curbrk

.sdata
_minbrk:.word	end
_curbrk:.word	end

.text

LEAF(sbrk)
	lw	v1,_curbrk
	addu	a0,v1
	li	v0,SYS_brk
	syscall
	bne	a3,zero,err
	move	v0,v1			# return previous curbrk
	sw	a0,_curbrk		# update to new curbrk
	RET

err:
	j	_cerror
.end sbrk
