#ifndef lint
static  char	*sccsid = "@(#)scanf.c	4.1 (ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 * Internationised scanf(), fscanf(), and sscanf() routines.
 *
 * These hooks cause the internationised _idoscan to be called before
 * _doscan, allowing I18N format strings to be parsed and converted into
 * normal printf format strings.
 *
 */

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 001  David Lindner Tue Nov 14 1989
 *      - Removed vax/mips ifdefs and totally implemented varargs
 *        structure for both architectures. This involved modifying
 *        every single routine to use the argument pointer provided
 *        by varargs.
 *      - Removed SYSTEM_FIVE ifdefs. Libi should be compiled in
 *        POSIX mode to begin with, and should always return the
 *        number of characters sucessfully output per XPG3.
 *
 */

#include	<stdio.h>
#include	<varargs.h>

/*
 * -----------------------------------------------------------------------
 * scanf()
 *
 * Modified scanf to use varargs structure.
 * -----------------------------------------------------------------------
 */

scanf(fmt, va_alist)
char *fmt;
va_dcl
{
	va_list ap;
	
	va_start(ap);
	return(_idoscan(stdin, fmt, ap));
}

/*
 * -----------------------------------------------------------------------
 * fscanf()
 *
 * Modified fscanf to use varargs structure.
 * -----------------------------------------------------------------------
 */

fscanf(iop, fmt, va_alist)
FILE *iop;
char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return(_idoscan(iop, fmt, ap));
}

/*
 * -----------------------------------------------------------------------
 * sscanf()
 *
 * Modified sscanf to use varargs structure.
 * -----------------------------------------------------------------------
 */

sscanf(str, fmt, va_alist)
register char *str;
char *fmt;
va_dcl
{
	FILE _strbuf;
	va_list ap;

	va_start(ap);

	_strbuf._flag = _IOREAD|_IOSTRG;
	_strbuf._ptr = _strbuf._base = str;
	_strbuf._cnt = 0;
	while (*str++)
		_strbuf._cnt++;
	_strbuf._bufsiz = _strbuf._cnt;

	return(_idoscan(&_strbuf, fmt, ap));
}




/*
 * -----------------------------------------------------------------------
 * nl_scanf(), nl_fscanf() and nl_sscan() provided for backwards
 * compatibility with XPG-2.
 *
 * Modified nl_scanfs to use varargs structure.
 * -----------------------------------------------------------------------
 */

nl_scanf(fmt, va_alist)
char *fmt;
va_dcl
{
	va_list ap;
	
	va_start(ap);
	return(_idoscan(stdin, fmt, ap));
}


nl_fscanf(iop, fmt, va_alist)
FILE *iop;
char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return(_idoscan(iop, fmt, ap));
}

nl_sscanf(str, fmt, va_alist)
register char *str;
char *fmt;
va_dcl
{
	FILE _strbuf;
	va_list ap;

	va_start(ap);

	_strbuf._flag = _IOREAD|_IOSTRG;
	_strbuf._ptr = _strbuf._base = str;
	_strbuf._cnt = 0;
	while (*str++)
		_strbuf._cnt++;
	_strbuf._bufsiz = _strbuf._cnt;

	return(_idoscan(&_strbuf, fmt, ap));
}
