#ifndef lint
static char	*sccsid = " @(#)asincos.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
*
*			Modification History
*
* 002	Tim N	12-Oct-89
*	Changed asin() where |x|>1 to be XPG3 compliant and return
*	0, EDOM.
*
*		David Metsky		13-Jan-86
*
* 001	Added from BSD 4.3 version as part of upgrade
*
*	Based on:	asincos.c	1.1		8/21/85
*
*************************************************************************/

/* ASIN(X)
 * RETURNS ARC SINE OF X
 * DOUBLE PRECISION (IEEE DOUBLE 53 bits, VAX D FORMAT 56 bits)
 * CODED IN C BY K.C. NG, 4/16/85, REVISED ON 6/10/85.
 *
 * Required system supported functions:
 *	copysign(x,y)
 *	sqrt(x)
 *
 * Required kernel function:
 *	atan2(y,x) 
 *
 * Method :                  
 *	asin(x) = atan2(x,sqrt(1-x*x)); for better accuracy, 1-x*x is 
 *		  computed as follows
 *			1-x*x                     if x <  0.5, 
 *			2*(1-|x|)-(1-|x|)*(1-|x|) if x >= 0.5.
 *
 * Special cases:
 *	if x is NaN, return x itself;
 *	if |x|>1, return 0.0.
 *
 * Accuracy:
 * 1)  If atan2() uses machine PI, then
 * 
 *	asin(x) returns (PI/pi) * (the exact arc sine of x) nearly rounded;
 *	and PI is the exact pi rounded to machine precision (see atan2 for
 *      details):
 *
 *	in decimal:
 *		pi = 3.141592653589793 23846264338327 ..... 
 *    53 bits   PI = 3.141592653589793 115997963 ..... ,
 *    56 bits   PI = 3.141592653589793 227020265 ..... ,  
 *
 *	in hexadecimal:
 *		pi = 3.243F6A8885A308D313198A2E....
 *    53 bits   PI = 3.243F6A8885A30  =  2 * 1.921FB54442D18	error=.276ulps
 *    56 bits   PI = 3.243F6A8885A308 =  4 * .C90FDAA22168C2    error=.206ulps
 *	
 *	In a test run with more than 200,000 random arguments on a VAX, the 
 *	maximum observed error in ulps (units in the last place) was
 *	2.06 ulps.      (comparing against (PI/pi)*(exact asin(x)));
 *
 * 2)  If atan2() uses true pi, then
 *
 *	asin(x) returns the exact asin(x) with error below about 2 ulps.
 *
 *	In a test run with more than 1,024,000 random arguments on a VAX, the 
 *	maximum observed error in ulps (units in the last place) was
 *      1.99 ulps.
 */

#include <errno.h>

double asin(x)
double x;
{
	double s,t,copysign(),atan2(),sqrt(),one=1.0;
#ifndef vax
	if(x!=x) return(x);	/* x is NaN */
#endif
	if( (x > 1.0) || (x < -1.0) ){
		errno = EDOM;
		return (0.0);
	}

	s=copysign(x,one);
	if(s <= 0.5)
	    return(atan2(x,sqrt(one-x*x)));
	else 
	    { t=one-s; s=t+t; return(atan2(x,sqrt(s-t*t))); }

}

/* ACOS(X)
 * RETURNS ARC COS OF X
 * DOUBLE PRECISION (IEEE DOUBLE 53 bits, VAX D FORMAT 56 bits)
 * CODED IN C BY K.C. NG, 4/16/85, REVISED ON 6/10/85.
 *
 * Required system supported functions:
 *	copysign(x,y)
 *	sqrt(x)
 *
 * Required kernel function:
 *	atan2(y,x) 
 *
 * Method :                  
 *			      ________
 *                           / 1 - x
 *	acos(x) = 2*atan2(  / -------- , 1 ) .
 *                        \/   1 + x
 *
 * Special cases:
 *	if x is NaN, return x itself;
 *	if |x|>1, return NaN.
 *
 * Accuracy:
 * 1)  If atan2() uses machine PI, then
 * 
 *	acos(x) returns (PI/pi) * (the exact arc cosine of x) nearly rounded;
 *	and PI is the exact pi rounded to machine precision (see atan2 for
 *      details):
 *
 *	in decimal:
 *		pi = 3.141592653589793 23846264338327 ..... 
 *    53 bits   PI = 3.141592653589793 115997963 ..... ,
 *    56 bits   PI = 3.141592653589793 227020265 ..... ,  
 *
 *	in hexadecimal:
 *		pi = 3.243F6A8885A308D313198A2E....
 *    53 bits   PI = 3.243F6A8885A30  =  2 * 1.921FB54442D18	error=.276ulps
 *    56 bits   PI = 3.243F6A8885A308 =  4 * .C90FDAA22168C2    error=.206ulps
 *	
 *	In a test run with more than 200,000 random arguments on a VAX, the 
 *	maximum observed error in ulps (units in the last place) was
 *	2.07 ulps.      (comparing against (PI/pi)*(exact acos(x)));
 *
 * 2)  If atan2() uses true pi, then
 *
 *	acos(x) returns the exact acos(x) with error below about 2 ulps.
 *
 *	In a test run with more than 1,024,000 random arguments on a VAX, the 
 *	maximum observed error in ulps (units in the last place) was
 *	2.15 ulps.
 */

double acos(x)
double x;
{
	double t,copysign(),atan2(),sqrt(),one=1.0;
#ifndef vax
	if(x!=x) return(x);
#endif
	if( x != -1.0)
	    t=atan2(sqrt((one-x)/(one+x)),one);
	else
	    t=atan2(one,0.0);	/* t = PI/2 */
	return(t+t);
}
