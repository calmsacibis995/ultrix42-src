#ifndef lint
static char *sccsid = "@(#)strlen.c	4.1	(ULTRIX)	7/3/90";
#endif not lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: strlen.c,v 1.1 87/02/16 11:17:42 dce Exp $ */

/*
 * Returns the number of non-NULL bytes in string argument.
 * Original code:
 *	strlen(s)
 *	register char *s;
 *	{
 *		register n;
 *		n = 0;
 *		while (*s++)
 *			n++;
 *		return(n);
 *	}
 */

/* Code optimized for mips.  4 cycles/byte. */
strlen(s)
register char *s;
{
	register char *p = s + 1;
	register unsigned c;	/* c exists only because UOPT wastes a
				   register (but no cycles) if the
				   while is written as *s++ != '\0' */
	do {
		c = *s++;
	} while (c != '\0');
	return s - p;
}
