/*	@(#)strtoul.c	4.1	(ULTRIX)	7/3/90 */

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
/************************************************************************
 *			Modification History				*
 *									*
 * 001	Ken Lesniak, 21-Sep-1989					*
 *	Return EINVAL in error if invalid base				*
 *									*
 * 000	Ken Lesniak, 03-May-1989					*
 *	Adapted from strtol.c						*
 ************************************************************************/

/*LINTLIBRARY*/
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define DIGIT(x)	(isdigit(x) ? (x) - '0' : \
			islower(x) ? (x) + 10 - 'a' : (x) + 10 - 'A')
#define MBASE	('z' - 'a' + 1 + 10)

unsigned long
strtoul(str, ptr, base)
register char *str;
char **ptr;
register int base;
{
	register unsigned long val;
	register int c;
	register unsigned long maxval;
	int xx, neg = 0;
	int range_error = 0;

	if (ptr != (char **)0)
		*ptr = str; /* in case no number is formed */
	if (base < 0 || base == 1 || base > MBASE) {
		errno = EINVAL;
		return 0;
	};
	if (!isalnum(c = *str)) {
		while (isspace(c))
			c = *++str;
		switch (c) {
		case '-':
			neg++;
		case '+': /* fall-through */
			c = *++str;
		}
	}
	if (base == 0)
		if (c != '0')
			base = 10;
		else if (str[1] == 'x' || str[1] == 'X')
			base = 16;
		else
			base = 8;
	/*
	 * for any base > 10, the digits incrementally following
	 *	9 are assumed to be "abc...z" or "ABC...Z"
	 */
	if (!isalnum(c) || (xx = DIGIT(c)) >= base)
		return (0); /* no number formed */
	if (base == 16 && c == '0' && isxdigit(str[2]) &&
	    (str[1] == 'x' || str[1] == 'X'))
		c = *(str += 2); /* skip over leading "0x" or "0X" */
	maxval = ULONG_MAX / base;
	for (val = DIGIT(c); isalnum(c = *++str) && (xx = DIGIT(c)) < base; ) {
		if (val > maxval)
			range_error++;
		val *= base;
		if (val > ULONG_MAX - xx)
			range_error++;
		val += xx;
	};
	if (ptr != (char **)0)
		*ptr = str;
	if (neg)
		val = -val;
	if (range_error) {
		errno = ERANGE;
		val = ULONG_MAX;
	};
	return val;
}
