#ifndef lint
static char *sccsid = "@(#)atof.c	4.1	(ULTRIX)	7/3/90";
#endif not lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: atof.c,v 1.4 87/04/06 20:27:16 dce Exp $ */
/*
 *	Modification History
 *
 * 1	Jon Reeves, 1989 June 14
 *	Internationalize: use _lc_radix instead of hardcoded '.'.
 *	Overlooked in the initial port.
 *
 * 2	Jon Reeves, 1989 Sep 19
 *	Fix error handling: detect absurdly large exponents, set ERANGE
 *
 * 3	Jon Reeves, 1989 Nov 14
 *	Fix error handling: set ERANGE on exponent underflow, too
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define MAXDIGITS 17

#if MIPSEL
#  define LSW 0
#  define MSW 1
static unsigned infinity[2] = { 0x00000000, 0x7ff00000 };
#endif
#if MIPSEB
#  define MSW 0
#  define LSW 1
static unsigned infinity[2] = { 0x7ff00000, 0x00000000 };
#endif

extern double _atod ();
extern char _lc_radix;

double
atof (s)
    register char *s;
{
    register unsigned c;
    register unsigned negate, decimal_point;
    register char *d;
    register int exp;
    register double x;
    char *ssave = s;
    union {
	double d;
	unsigned w[2];
    } o;
    char digits[MAXDIGITS];

    while (c = *s++, isspace(c));

    negate = 0;
    if (c == '+') {
	c = *s++;
    }
    else if (c == '-') {
	negate = 1;
	c = *s++;
    }
    d = digits;
    decimal_point = 0;
    exp = 0;
    while (1) {
	c -= '0';
	if (c < 10) {
	    if (d == digits+MAXDIGITS) {
		/* ignore more than 17 digits, but adjust exponent */
		exp += (decimal_point ^ 1);
	    }
	    else {
		if (c == 0 && d == digits) {
		    /* ignore leading zeros */
		}
		else {
		    *d++ = c;
		}
		exp -= decimal_point;
	    }
	}
	else if (c == _lc_radix - '0' && !decimal_point) {
	    decimal_point = 1;
	}
	else {
	    break;
	}
	c = *s++;
    }

    if (c == 'e'-'0' || c == 'E'-'0') {
	register unsigned negate_exp = 0;
	register int e = 0;
	c = *s++;
	if (c == '+' || c == ' ') {
	    c = *s++;
	}
	else if (c == '-') {
	    negate_exp = 1;
	    c = *s++;
	}
	while (c -= '0', c < 10 && e < 400) {
	    e = e * 10 + c;
	    c = *s++;
	}
	if (negate_exp) {
	    e = -e;
	}
	exp += e;
    }
    if (d == digits) {
	return 0.0;
    }

    if (exp < -340) {
	x = 0;
	errno = ERANGE;
    }
    else if (exp > 308) {
	x = *(double *)infinity;
	errno = ERANGE;
    }
    else {
	x = _atod (digits, d - digits, exp);
    }
    if (negate) {
	x = -x;
    }
    return x;
}
