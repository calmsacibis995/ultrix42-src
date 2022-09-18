/*
 *		@(#)brk.c	4.1	(ULTRIX)	7/3/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *			Modification History				*
 *									*
 *	David L Ballenger, 20-May-1985					*
 * 001	Use an unsigned comparison to compare the argument to brk() and	*
 *	minbrk, since addresses are unsigned.  Otherwise, attempts to	*
 *	set the program break to system space are not correctly flagged *
 *	as an error.							*
 *									*
 ************************************************************************/

/* brk.c 4.2 83/07/26 */

#include "SYS.h"

#define	SYS_brk		17

	.globl	curbrk
	.globl	minbrk

ENTRY(brk)
	cmpl	4(ap),minbrk
	bgequ	ok		/* Addresses are unsigned */
	movl	minbrk,4(ap)
ok:
	chmk	$SYS_brk
	jcs	err
	movl	4(ap),curbrk
	clrl	r0
	ret
err:
	jmp	cerror
