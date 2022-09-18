#ifndef lint
static	char	*sccsid = "@(#)scanf.c	4.1	(ULTRIX)	7/3/90";
#endif lint

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
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: scanf.c,v 1.1 87/02/16 11:19:30 dce Exp $ */

#include	<stdio.h>
#ifdef mips
#include 	<varargs.h>
#endif mips

#ifdef vax
scanf(fmt, args)
#endif vax
#ifdef mips
scanf(fmt, va_alist)
#endif mips
char *fmt;
#ifdef mips
va_dcl
#endif mips
{
#ifdef mips
	va_list ap;
	
	va_start(ap);
#endif mips
/* the following allows the source to look the same for arg processing */
#ifdef vax
#define ARGS    &args
#endif vax
#ifdef mips
#define ARGS    ap
#endif mips
	return(_doscan(stdin, fmt, ARGS));
}

#ifdef vax
fscanf(iop, fmt, args)
#endif vax
#ifdef mips
fscanf(iop, fmt, va_alist)
#endif mips
FILE *iop;
char *fmt;
#ifdef mips
va_dcl
#endif mips
{
#ifdef mips
	va_list ap;

	va_start(ap);
#endif mips
	return(_doscan(iop, fmt, ARGS));
}

#ifdef vax
sscanf(str, fmt, args)
#endif vax
#ifdef mips
sscanf(str, fmt, va_alist)
#endif mips
register char *str;
char *fmt;
#ifdef mips
va_dcl
#endif mips
{
	FILE _strbuf;
#ifdef mips
	va_list ap;

	va_start(ap);
#endif mips

	_strbuf._flag = _IOREAD|_IOSTRG;
	_strbuf._ptr = _strbuf._base = str;
	_strbuf._cnt = 0;
	while (*str++)
		_strbuf._cnt++;
	_strbuf._bufsiz = _strbuf._cnt;

	return(_doscan(&_strbuf, fmt, ARGS));
}
