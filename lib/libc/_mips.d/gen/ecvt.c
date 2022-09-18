#ifndef lint
static char *sccsid = "@(#)ecvt.c	4.1	(ULTRIX)	7/3/90";
#endif not lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: ecvt.c,v 1.1 87/02/16 11:17:24 dce Exp $ */

/* Support obsolete interfaces */

#define	NDIG 82
static char buffer[NDIG+2];

char *
ecvt (arg, ndigits, decpt, sign)
    register double arg;
    register int ndigits, *decpt, *sign;
{
    if (ndigits > 17) {
	register char *p, *e;
	*decpt = _dtoa (buffer, 17, arg, 0) + 1;
	for (p = buffer+18, e = buffer + 1 + (ndigits > NDIG ? NDIG : ndigits);
	     p != e; ) *p++ = '0';
	*p++ = '\0';
    }
    else if (ndigits <= 0) {
	*decpt = _dtoa (buffer, 1, arg, 0) + 1;
	buffer[1] = '\0';
    }
    else {
	*decpt = _dtoa (buffer, ndigits, arg, 0) + 1;
    }
    *sign = buffer[0] == '-';
    return buffer+1;
}

char *
fcvt (arg, ndigits, decpt, sign)
    register double arg;
    register int ndigits, *decpt, *sign;
{
    *decpt = _dtoa (buffer, ndigits, arg, 1) + 1;
    *sign = buffer[0] == '-';
    return buffer+1;
}
