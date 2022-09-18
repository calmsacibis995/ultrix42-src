#ifndef lint
static char *sccsid = "@(#)printf.c	4.2  (ULTRIX)        8/13/90";
#endif
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

#include <varargs.h>

/*
 * Hacked "printf" which prints through putchar.
 * DONT USE WITH STDIO!
 *
 * Modification History
 * 002 - Bob Fontaine - Thu Jun 21 10:22:20 EDT 1990
 *	changed names of printf and _doprnt to csh_printf and csh_doprnt
 *	respectively.  This eliminates a conflict witht the stdio library
 *	routines.  Fixes QAR #4449.
 *
 * 001 - Gary A. Gaudet - Thu Dec 28 17:29:34 EST 1989
 *	Added VARARGS comment.
 */
/*VARARGS1*/
csh_printf(fmt, va_alist)
char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	csh_doprnt(fmt, ap);
	va_end(ap);
}

_strout(count, string, adjust, foo, fillch)
register char *string;
register int count;
int adjust;
register struct { int a[6]; } *foo;
{

	if (foo != 0)
		abort();
	while (adjust < 0) {
		if (*string=='-' && fillch=='0') {
			putchar(*string++);
			count--;
		}
		putchar(fillch);
		adjust++;
	}
	while (--count>=0)
		putchar(*string++);
	while (adjust) {
		putchar(fillch);
		adjust--;
	}
}
