#ifndef lint
static	char	*sccsid = "@(#)hypot.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/******************************************************************************
 *			Modifications
 * 	Lu Anne Van de Pas
 * 001  Added check so that we don't produce a floating point exception when
 *	a and b are both 0.
 *
 *****************************************************************************/

/*LINTLIBRARY*/
/*
 *	hypot(a, b) returns sqrt(a^2 + b^2), avoiding unnecessary overflows.
 *	Returns ERANGE error and value HUGE if the correct value would overflow.
 */

#include <math.h>
#include <values.h>
#include <errno.h>
#define ITERATIONS	4

double
hypot(a, b)
register double a, b;
{
	register double t;
	register int i = ITERATIONS;
	struct exception exc;

	if ((exc.arg1 = a) < 0)
		a = -a;
	if ((exc.arg2 = b) < 0)
		b = -b;
	if (a > b) {				/* make sure |a| <= |b| */
		t = a;
		a = b;
		b = t;
	}
	if ((a==0) && (b==0)) 
		return (0);			/*Don't divide by 0*/
	
	if ((t = a/b) < X_EPS)			/* t <= 1 */
		return (b);			/* t << 1 */
	a = 1 + t * t;				/* a = 1 + (a/b)^2 */
	t = 0.5 + 0.5 * a;			/* first guess for sqrt */
	do {
		t = 0.5 * (t + a/t);
	} while (--i > 0);			/* t <= sqrt(2) */
	if (b < MAXDOUBLE/M_SQRT2)		/* result can't overflow */
		return (t * b);
	if ((t *= 0.5 * b) < MAXDOUBLE/2)	/* result won't overflow */
		return (t + t);
	exc.type = OVERFLOW;
	exc.name = "hypot";
	exc.retval = HUGE;
	if (!matherr(&exc))
		errno = ERANGE;
	return (exc.retval);
}
