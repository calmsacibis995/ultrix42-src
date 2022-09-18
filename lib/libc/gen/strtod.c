#ifndef lint
static	char	*sccsid = "@(#)strtod.c	4.1	ULTRIX	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Modification History
 * 001  DECwest ANSI dls 019
 *      Ensure that endptr in strtod does not change unless we can
 *      return a valid number. The endptr was being modified if we scanned
 *      white space but did not find a number.
 */

#include <ctype.h>
#include <stdio.h>

extern double atof();

/* strtod
 *
 *	Convert string to double.  This is the ULTRIX implementation of
 *	the System V strtod() routine.  This routine simply parses the
 *	string and then calls atof() to actually perform the conversion.
 *	This allows all the details of GFLOAT/DFLOAT conversion to be
 *	hidden in atof().
 */

double
strtod(str,ptr)
	char	*str,	/* string to convert */
		**ptr;	/* return pointer for terminating character */
{
	register char	*scanptr = str ;
	extern char _lc_radix;		/* INTL the radix character */

	/* Skip whitespace then parse fraction.
	 */
	while (isspace(*scanptr)) scanptr++;

	if (*scanptr == '+' || *scanptr == '-') scanptr++ ;

	/*
	 * Ensure string can be converted
	 */
	if(isdigit(*scanptr) ||
	                   (*scanptr == _lc_radix && isdigit(scanptr[1]))){
	    while (isdigit(*scanptr)) scanptr++;

	    if (*scanptr == _lc_radix) {	/* INTL */
		scanptr++;
		while (isdigit(*scanptr)) scanptr++;
	    }

	    /* Parse exponent.
	     */
	    if (*scanptr == 'E' || *scanptr == 'e') {

		char	*e_ptr = scanptr++; /* save ptr to 'E'|'e' */

		if (*scanptr == '+' || *scanptr == '-') scanptr++ ;

		if (isdigit(*scanptr++)) 
			while (isdigit(*scanptr)) scanptr++;
		else
			/* Return pointer to beginning of 'E'|'e' if
			 * this wasn't a well formed exponent.
			 */
			scanptr = e_ptr ;
	    }
	} else
	    scanptr = str;
	
	/* Return pointer to terminating character
	 */
	if (ptr != NULL) *ptr = scanptr;

	/* Return converted string
	 */
	return(atof(str));
}
