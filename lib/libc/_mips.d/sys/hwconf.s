/*	@(#)hwconf.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include <sysmips.h>

/*
 * hwconf is implemented via the SYS_mips system call. The 2 arguments are
 * shuffled back so the MIPS_HWCONF vector can be put in the first argument.
 * The SYS_mips system call takes 5 arguments and unused arguments must
 * have zero values.
 */
LEAF(hwconf)
	subu	sp,8
	sw	zero,16(sp)
	move	a3,zero
	move	a2,a1
	move	a1,a0
	li	a0,MIPS_HWCONF
	li	v0,SYS_sysmips
	syscall
	addu	sp,8
	beq	a3,zero,9f
	j	_cerror
9:
	RET
.end hwconf
