#ifndef lint
static char	*sccsid = "@(#)exp.c	4.1	(ULTRIX)	7/17/90";
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
*		David Metsky		13-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade
*
* 002	Jon Reeves, 1990-Jan-19
*	Fixed error cutoff limit; without this, cosh couldn't handle large
*	arguments (not to mention exp itself...)
*
*	Based on:	exp.c		4.3		8/21/85
*
*************************************************************************/

/* EXP(X)
 * RETURN THE EXPONENTIAL OF X
 * DOUBLE PRECISION (IEEE 53 bits, VAX D FORMAT 56 BITS)
 * CODED IN C BY K.C. NG, 1/19/85; 
 * REVISED BY K.C. NG on 2/6/85, 2/15/85, 3/7/85, 3/24/85, 4/16/85.
 *
 * Required system supported functions:
 *	scalb(x,n)	
 *	copysign(x,y)	
 *	finite(x)
 *
 * Kernel function:
 *	exp__E(x,c)
 *
 * Method:
 *	1. Argument Reduction: given the input x, find r and integer k such 
 *	   that
 *	                   x = k*ln2 + r,  |r| <= 0.5*ln2 .  
 *	   r will be represented as r := z+c for better accuracy.
 *
 *	2. Compute expm1(r)=exp(r)-1 by 
 *
 *			expm1(r=z+c) := z + exp__E(z,r)
 *
 *	3. exp(x) = 2^k * ( expm1(r) + 1 ).
 *
 * Special cases:
 *	exp(INF) is INF, exp(NaN) is NaN;
 *	exp(-INF)=  0;
 *	for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *	exp(x) returns the exponential of x nearly rounded. In a test run
 *	with 1,156,000 random arguments on a VAX, the maximum observed
 *	error was .768 ulps (units in the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>
#include <errno.h>
#include <float.h>

#define LN_MAXDOUBLE (M_LN2 * DBL_MAX_EXP)

#ifdef VAX	/* VAX D format */
/* double static */
/* ln2hi  =  6.9314718055829871446E-1    , Hex  2^  0   *  .B17217F7D00000 */
/* ln2lo  =  1.6465949582897081279E-12   , Hex  2^-39   *  .E7BCD5E4F1D9CC */
/* lnhuge =  9.4961163736712506989E1     , Hex  2^  7   *  .BDEC1DA73E9010 */
/* lntiny = -9.5654310917272452386E1     , Hex  2^  7   * -.BF4F01D72E33AF */
/* invln2 =  1.4426950408889634148E0     ; Hex  2^  1   *  .B8AA3B295C17F1 */
static long     ln2hix[] = { 0x72174031, 0x0000f7d0};
static long     ln2lox[] = { 0xbcd52ce7, 0xd9cce4f1};
static long    lnhugex[] = { 0xec1d43bd, 0x9010a73e};
static long    lntinyx[] = { 0x4f01c3bf, 0x33afd72e};
static long    invln2x[] = { 0xaa3b40b8, 0x17f1295c};
#define    ln2hi    (*(double*)ln2hix)
#define    ln2lo    (*(double*)ln2lox)
#define   lnhuge    (*(double*)lnhugex)
#define   lntiny    (*(double*)lntinyx)
#define   invln2    (*(double*)invln2x)
#else	/* IEEE double */
double static
ln2hi  =  6.9314718036912381649E-1    , /*Hex  2^ -1   *  1.62E42FEE00000 */
ln2lo  =  1.9082149292705877000E-10   , /*Hex  2^-33   *  1.A39EF35793C76 */
lnhuge =  7.1602103751842355450E2     , /*Hex  2^  9   *  1.6602B15B7ECF2 */
lntiny = -7.5137154372698068983E2     , /*Hex  2^  9   * -1.77AF8EBEAE354 */
invln2 =  1.4426950408889633870E0     ; /*Hex  2^  0   *  1.71547652B82FE */
#endif

double exp(x)
double x;
{
	double scalb(), copysign(), exp__E(), z,hi,lo,c;
	int k,finite();
	double range;

	range = LN_MAXDOUBLE;

#ifndef vax
	if(x!=x) return(x);	/* x is NaN */
#endif
	if( x <= range ) {
		if( x >= lntiny ) {

		    /* argument reduction : x --> x - k*ln2 */

			k=invln2*x+copysign(0.5,x);	/* k=NINT(x/ln2) */

			/* express x-k*ln2 as z+c */
			hi=x-k*ln2hi;
			z=hi-(lo=k*ln2lo);
			c=(hi-z)-lo;

		    /* return 2^k*[expm1(x) + 1]  */
			z += exp__E(z,c);
			return (scalb(z+1.0,k));  
		}
		/* end of x > lntiny */

		else 
		     /* exp(-big#) underflows to zero */
		     if(finite(x)){
			errno = ERANGE;
			return(0.0);
		     }

		     /* exp(-INF) is zero */
		     else return(0.0);
	}
	/* end of x < range */

	else 
	/* exp(INF) is INF, exp(+big#) overflows to INF */
		errno = ERANGE;
		return(HUGE_VAL);
}
