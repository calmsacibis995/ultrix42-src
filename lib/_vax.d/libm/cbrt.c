#ifndef lint
static char	*sccsid = "@(#)cbrt.c	4.1	(ULTRIX)	7/17/90";
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
*	David Metsky, 12/18/86
* 001	Adapted from cbrt.c 1.1 (Berkeley) 5/23/85
*
*************************************************************************/

/* kahan's cube root (53 bits IEEE double precision)
 * for IEEE machines only
 * coded in C by K.C. Ng, 4/30/85
 *
 * Accuracy:
 *	better than 0.667 ulps according to an error analysis. Maximum
 * error observed was 0.666 ulps in an 1,000,000 random arguments test.
 *
 * Warning: this code is semi machine dependent; the ordering of words in
 * a floating point number must be known in advance. I assume that the
 * long interger at the address of a floating point number will be the
 * leading 32 bits of that floating point number (i.e., sign, exponent,
 * and the 20 most significant bits).
 * On a National machine, it has different ordering; therefore, this code 
 * must be compiled with flag -DNATIONAL. 
 */
#ifndef VAX

static unsigned long B1 = 715094163, /* B1 = (682-0.03306235651)*2**20 */
	             B2 = 696219795; /* B2 = (664-0.03306235651)*2**20 */
static double
	    C= 19./35.,
	    D= -864./1225.,
	    E= 99./70.,
	    F= 45./28.,
	    G= 5./14.;

double cbrt(x) 
double x;
{
	double r,s,t=0.0,w;
	unsigned long *px = (unsigned long *) &x,
	              *pt = (unsigned long *) &t,
		      mexp,sign;

#ifdef NATIONAL /* ordering of words in a floating points number */
	int n0=1,n1=0;
#else
	int n0=0,n1=1;
#endif

	mexp=px[n0]&0x7ff00000;
	if(mexp==0x7ff00000) return(x); /* cbrt(NaN,INF) is itself */
	if(x==0.0) return(x);		/* cbrt(0) is itself */

	sign=px[n0]&0x80000000; /* sign= sign(x) */
	px[n0] ^= sign;		/* x=|x| */


    /* rough cbrt to 5 bits */
	if(mexp==0) 		/* subnormal number */
	  {pt[n0]=0x43500000; 	/* set t= 2**54 */
	   t*=x; pt[n0]=pt[n0]/3+B2;
	  }
	else
	  pt[n0]=px[n0]/3+B1;	


    /* new cbrt to 23 bits, may be implemented in single precision */
	r=t*t/x;
	s=C+r*t;
	t*=G+F/(s+E+D/s);	

    /* chopped to 20 bits and make it larger than cbrt(x) */ 
	pt[n1]=0; pt[n0]+=0x00000001;


    /* one step newton iteration to 53 bits with error less than 0.667 ulps */
	s=t*t;		/* t*t is exact */
	r=x/s;
	w=t+t;
	r=(r-t)/(w+r);	/* r-s is exact */
	t=t+t*r;


    /* retore the sign bit */
	pt[n0] |= sign;
	return(t);
}
#endif
