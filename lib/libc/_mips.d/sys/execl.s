/*	@(#)execl.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: execl.s,v 1.1 87/02/16 11:19:54 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

FRMSIZE	=	20

NESTED(execl, FRMSIZE, zero)
	subu	sp,FRMSIZE
	sw	ra,FRMSIZE-4(sp)
	sw	a1,FRMSIZE+4(sp)
	sw	a2,FRMSIZE+8(sp)
	sw	a3,FRMSIZE+12(sp)
	move	a1,sp
	addu	a1,FRMSIZE+4
	jal	execv
	lw	ra,FRMSIZE-4(sp)
	addu	sp,FRMSIZE
	RET		# execl(file, arg1, arg2, ..., 0);
.end execl
