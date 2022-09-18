#ifndef lint
static char *sccsid = "@(#)lgamma.c	4.1	ULTRIX	7/17/90";
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

/************************************************************************
 *	Modification History
 *      --------------------
 *
 *	5-Oct-89		Tim N
 *	Copy of gamma.c.
 *	Added signgam and checking.  Took signgam cope from 4.3-5
 *	source (lgamma.c).  But code for x < 0 input gave very different
 *	results so I kept the old code in here. 
 *
 *	14-Jun-89		Tim N
 *	Replaced code with modified version from the ULTRIX libmV library.
 *	This nonrecursive version gives identical results and handles
 *	EDOM and ERANGE correctly.
 ************************************************************************/

/*
 *	gamma returns the log of the absolute value of the gamma function
 *	of its double-precision argument.
 *	Returns EDOM error and value HUGE if argument is non-negative integer.
 *	Returns ERANGE and value HUGE if the correct value would overflow.
 *
 *	The coefficients for expansion around zero
 *	are #5243 from Hart & Cheney; for expansion
 *	around infinity they are #5404.
 *
 *	Calls log and sin.
 */

#include <math.h>
#include <values.h>
#include <errno.h>

#define X_MAX	(3.0 * H_PREC)
#define GOOBIE	0.9189385332046727417803297

/* X/Open compliance - set to the sign of the gamma result (before lgamma) */
int signgam;

double
lgamma(x)
register double x;
{
	extern double pos_gamma();

	signgam = 1;

	if (x > 0)
		return (pos_gamma(x));
	else {
		static double pi = M_PI;
		double temp;

		x = -x;				/* neg number becomes pos */
		temp = floor(x);		/* round down to int */
		if( temp == x ){		/* neg ints is bad ones */
			errno = EDOM;
			return (HUGE_VAL);
		}
		if( x >= X_MAX){		/* overflow */
			errno = ERANGE;
			return (HUGE_VAL);
		}

		/* set signgam.  If temp is odd signgam = 1, even = -1 */
		if( (x - temp) > 0.5 )		/* round to closest int */
			temp = temp + 1.0;
		signgam = (int) (temp - 2*floor(temp/2));
		signgam = signgam - 1 + signgam;
		if( (x - temp) < 0.0 )
			signgam = -signgam;

		/* did not take new one from BSD 4.3-5 */
		/* it generates different (very) answers from 4.2 version */
		if ((temp = sin(pi * x)) < 0.0)
			temp = -temp;
		return (-(log(x * temp/pi) + pos_gamma(x)));
	}

	/*NOTREACHED*/

}

static double
pos_gamma(x)
register double x;
{
	static double p2[] = {
		-0.67449507245925289918e1,
		-0.50108693752970953015e2,
		-0.43933044406002567613e3,
		-0.20085274013072791214e4,
		-0.87627102978521489560e4,
		-0.20886861789269887364e5,
		-0.42353689509744089647e5,
	}, q2[] = {
		 1.0,
		-0.23081551524580124562e2,
		 0.18949823415702801641e3,
		-0.49902852662143904834e3,
		-0.15286072737795220248e4,
		 0.99403074150827709015e4,
		-0.29803853309256649932e4,
		-0.42353689509744090010e5,
	};
	register double y, z;

	if (x > 8) { /* asymptotic approximation */
		static double p[] = {
			-0.1633436431e-2,
			 0.83645878922e-3,
			-0.5951896861197e-3,
			 0.793650576493454e-3,
			-0.277777777735865004e-2,
			 0.83333333333333101837e-1,
		};
	
		if (x >= MAXDOUBLE/LN_MAXDOUBLE) {
			errno = ERANGE;
			return (HUGE_VAL);
		}
		z = (x - 0.5) * log(x) - x + GOOBIE;
		if (x > X_MAX)
			return (z);
		x = 1/x;
		y = x * x;
		return (z + x * _POLY5(y, p));
	}
	y = 1;
	if (x < y)
		y /= (x * (y + x));
	else if (x < 2) {
		y /= x;
		x -= 1;
	} else {
		for ( ; x >= 3; y *= x)
			x -= 1;
		x -= 2;
	}
	return (log(y * _POLY6(x, p2)/_POLY7(x, q2)));
}
