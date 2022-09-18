#ifndef lint
static  char	*sccsid = "@(#)printf.c	4.1 (ULTRIX) 7/2/90";
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
 * Internationised vprintf(), printf(), fprintf(), and sprintf() routines.
 *
 * These hooks cause the internationised _idoprnt to be called before
 * _doprnt, allowing I18N format strings to be parsed and converted into
 * normal printf format strings.
 *
 */

/* 
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 001	David Lindner Tue Nov 14 1989
 *	- Removed vax/mips ifdefs and totally implemented varargs
 *	  structure for both architectures. This involved modifying
 *	  every single routine to use the argument pointer provided
 *	  by varargs.
 *	- Removed SYSTEM_FIVE ifdefs. Libi should be compiled in
 *	  POSIX mode to begin with, and should always return the
 *	  number of characters sucessfully output per XPG3.
 *	- Implemented all of the vprintf routines as they were
 *	  previously unimplemented.
 *	- Added <values.h> to include list, for all those wonderful
 *	  constants and definitions.
 *	- Thanks to Greg Tarsa for some serious code bashing.
 *
 */

#include	<stdio.h>
#include	<varargs.h>
#include	<values.h>

/*
 * -----------------------------------------------------------------------
 * printf()
 *
 * Modified printfs to use varargs structure.
 * -----------------------------------------------------------------------
 */

printf(fmt, va_alist)
char *fmt;
va_dcl
{
	int n_chars;
	va_list ap;
	
	va_start(ap);
	n_chars = _idoprnt(fmt, ap, stdout);
	return(ferror(stdout)? EOF: n_chars);
}

/*
 * nl_printf()
 *
 * Provided the old nl_printf() function for backwards compatibilty.
 *
 */

nl_printf(fmt, va_alist)
char *fmt;
va_dcl
{
	int n_chars;
	va_list ap;
	
	va_start(ap);
	n_chars = _idoprnt(fmt, ap, stdout);
	return(ferror(stdout)? EOF: n_chars);
}


/*
 * -----------------------------------------------------------------------
 * fprintf()
 *
 * Modified fprintfs to use varargs structure.
 * -----------------------------------------------------------------------
 */

fprintf(iop, fmt, va_alist)
register FILE *iop;
char *fmt;
va_dcl
{
	int unbuffered;
	char temp_buf[BUFSIZ];
	int n_chars;		/* characters transmitted by _idoprnt() */
	va_list ap;
	
	va_start(ap);

	/* If the file is unbuffered, then use the temporary buffer, and
	 * make the file look like it is buffered.  This prevents a
	 * write() system call from being done for every character.
	 */
	unbuffered = iop->_flag & _IONBF;
	if (unbuffered) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = temp_buf;
		iop->_bufsiz = BUFSIZ;
	}

	n_chars = _idoprnt(fmt, ap, iop);

	/* If the file is unbuffered, then flush it to make sure that
	 * anything in the temporary buffer is written, then make it
	 * into an unbuffered file again.
	 */
	if (unbuffered) {
		(void)fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = 0;
		iop->_cnt = 0;
	}

	return(ferror(iop)? EOF: n_chars);
}


/*
 * nl_fprintf
 *
 * Provided the old nl_fprintf() function for backwards compatibilty.
 *
 */

nl_fprintf(iop, fmt, va_alist)
register FILE *iop;
char *fmt;
va_dcl
{
	int unbuffered;
	char temp_buf[BUFSIZ];
	int n_chars;		/* characters transmitted by _idoprnt() */
	va_list ap;
	
	va_start(ap);

	/* If the file is unbuffered, then use the temporary buffer, and
	 * make the file look like it is buffered.  This prevents a
	 * write() system call from being done for every character.
	 */
	unbuffered = iop->_flag & _IONBF;
	if (unbuffered) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = temp_buf;
		iop->_bufsiz = BUFSIZ;
	}

	n_chars = _idoprnt(fmt, ap, iop);

	/* If the file is unbuffered, then flush it to make sure that
	 * anything in the temporary buffer is written, then make it
	 * into an unbuffered file again.
	 */
	if (unbuffered) {
		(void)fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = 0;
		iop->_cnt = 0;
	}

	return(ferror(iop)? EOF: n_chars);
}


/*
 * -----------------------------------------------------------------------
 * sprintf()
 *
 * Modified sprintfs to use varargs structure.
 * -----------------------------------------------------------------------
 */

sprintf(str, fmt, va_alist)
char *str, *fmt;
va_dcl
{
	register int n_chars;
	FILE _strbuf;
	va_list ap;

	va_start(ap);
	_strbuf._flag = _IOWRT|_IOSTRG;
	_strbuf._base = _strbuf._ptr = str;
	_strbuf._cnt = 32767;

	n_chars = _idoprnt(fmt, ap, &_strbuf);

	/* Terminate the string with a null character */
	*_strbuf._ptr = '\0';

	return(n_chars);
}


/*
 * nl_sprintf
 *
 * Provided the old nl_fprintf() function for backwards compatibilty.
 */

nl_sprintf(str, fmt, va_alist)
char *str, *fmt;
va_dcl
{
	register int n_chars;
	FILE _strbuf;
	va_list ap;

	va_start(ap);
	_strbuf._flag = _IOWRT|_IOSTRG;
	_strbuf._base = _strbuf._ptr = str;
	_strbuf._cnt = 32767;

	n_chars = _idoprnt(fmt, ap, &_strbuf);

	/* Terminate the string with a null character */
	*_strbuf._ptr = '\0';

	return(n_chars);
}


/*
 * -----------------------------------------------------------------------
 * vprintf routines
 *
 * Added vprintf, vfprintf, vsprintf.
 * -----------------------------------------------------------------------
 */


/*
 * vfprintf()
 */

vfprintf(iop, format, ap)
FILE *iop;
char *format;
va_list ap;
{
	register int count;
	int unbuffered;
	char temp_buf[BUFSIZ];

	/* If the file is unbuffered, then use the temporary buffer, and
	 * make the file look like it is buffered.  This prevents a
	 * write() system call from being done for every character.
	 */
	unbuffered = iop->_flag & _IONBF;
	if (unbuffered) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = temp_buf;
		iop->_bufsiz = BUFSIZ;
	}

	count = _idoprnt(format, ap, iop);

	/* If the file is unbuffered, then flush it to make sure that
	 * anything in the temporary buffer is written, then make it
	 * into an unbuffered file again.
	 */
	if (unbuffered) {
		(void)fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = 0;
		iop->_cnt = 0;
	}

	return(ferror(iop)? EOF: count);
}


/*
 * vprintf()
 */

vprintf(format, ap)
char *format;
va_list ap;
{
	register int count;

	count = _idoprnt(format, ap, stdout);
	return(ferror(stdout)? EOF: count);
}


/*
 * vsprintf()
 */

vsprintf(string, format, ap)
char *string, *format;
va_list ap;
{
	register int count;
	FILE siop;

	siop._cnt = MAXINT;
	siop._base = siop._ptr = string;
	siop._flag = _IOWRT | _IOSTRG ;
	siop._file = _NFILE;
	count = _idoprnt(format, ap, &siop);
	*siop._ptr = '\0'; /* plant terminating null character */
	return(count);
}
