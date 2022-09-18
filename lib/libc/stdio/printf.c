#ifndef lint
static	char	*sccsid = "@(#)printf.c	4.1	(ULTRIX)	7/3/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *			Modification History
 *
 * 002  Linda Wilson, 06-oct-1989
 *	Return number of characters transmitted for X/Open conformance.
 *	Use System V additions made in 001.
 *
 * 001	David L Ballenger, 12-Nov-1985
 *	Add fixes for System V compatibility.
 *
 ************************************************************************/

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: printf.c,v 1.1 87/02/16 11:19:27 dce Exp $ */
#include	<stdio.h>

#ifdef vax
printf(fmt, args)
#endif /* vax */
#ifdef mips
#include	<varargs.h>
printf(fmt, va_alist)
#endif /* mips */
char *fmt;
#ifdef mips
va_dcl
#endif /* mips */
{
#if defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE)	/* 002 */
	int n_chars;
#endif /* defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE) */
#ifdef mips
	va_list ap;
	
	va_start(ap);
#endif /* mips */
/* the following allows the source to look the same for arg processing */
#ifdef vax
#define ARGS    &args
#endif /* vax */
#ifdef mips
#define ARGS    ap
#endif /* mips */
#if defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE)	/* 002 */
	/* For the System V environment, call _doprnt() then return EOF
	 * for failure and the number of characters transmitted by _doprnt()
	 * for success.
	 */
	n_chars = _doprnt(fmt, ARGS, stdout);
	return(ferror(stdout)? EOF: n_chars);
#else

	/* Call _doprnt() and then return EOF for failure and 0 for success.
	 */
	_doprnt(fmt, ARGS, stdout);
	return(ferror(stdout)? EOF: 0);
#endif /* defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE) */
}
