/*	@(#)recvmsg.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: recvmsg.s,v 1.1 87/02/16 11:20:51 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>

SYSCALL(recvmsg)
	RET
.end recvmsg