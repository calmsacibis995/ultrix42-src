#ifndef lint
static	char	*sccsid = "@(#)sprintf.c	4.1	(ULTRIX)	7/3/90";
#endif

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sprintf.c,v 1.1 87/02/16 11:19:33 dce Exp $ */

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
 * 002  Linda Wilson, 06-oct-1989
 *      Return int in POSIX/XOpen environments as well as the 
 *      System V environment.  See change 001.
 *
 * 001	David L Ballenger, 12-Nov-1985
 *	Add code for System V support.  In the System V environment, 
 *	sprintf() returns the number of characters (excluding the
 *	terminating '\0') placed in the string, in the ULTRIX
 *	environment the address of the string is returned.
 *
 ************************************************************************/

#include	<stdio.h>
#ifdef mips
#include 	<varargs.h>
#endif /* mips */

#if !defined(SYSTEM_FIVE) && !defined(_POSIX_SOURCE)
#define RETURN_TYPE char *
#else
#define RETURN_TYPE int
#endif /* !defined(SYSTEM_FIVE) && !defined(_POSIX_SOURCE) */

RETURN_TYPE
#ifdef vax
sprintf(str, fmt, args)
	char *str;
	char *fmt;
#endif /* vax */
#ifdef mips
sprintf(str, fmt, va_alist)
char *str, *fmt;
va_dcl
#endif /* mips */
{
#if defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE)
	register int n_chars;
#endif /* defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE) */
	FILE _strbuf;
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
	_strbuf._flag = _IOWRT|_IOSTRG;
	_strbuf._base = _strbuf._ptr = str;
	_strbuf._cnt = 32767;

	/* Call _doprnt() to do the dirty work.  In the ULTRIX environment,
	 * value is ignored.
	 */
#if !defined(SYSTEM_FIVE) && !defined(_POSIX_SOURCE)
	(void)_doprnt(fmt, ARGS, &_strbuf);
#else
	n_chars = _doprnt(fmt, ARGS, &_strbuf);
#endif /* !defined(SYSTEM_FIVE) && !defined(_POSIX_SOURCE) */
	/* Terminate the string with a null character */
	*_strbuf._ptr = '\0';

	/* In the ULTRIX environment return the string, and in the
	 * System V environment return the number of characters
	 * transmitted by _doprnt().
	 */
#if !defined(SYSTEM_FIVE) && !defined(_POSIX_SOURCE)
	return(str);
#else
	return(n_chars);
#endif /* !defined(SYSTEM_FIVE) && !defined(_POSIX_SOURCE) */
}
