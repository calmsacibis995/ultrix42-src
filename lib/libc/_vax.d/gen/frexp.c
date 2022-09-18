#ifndef lint
static	char	*sccsid = "@(#)frexp.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
/**/
/*
 *
 *   File name:
 *
 *	frexp.c
 *
 *   Source file description:
 *
 *	This file contains the standard C library routine frexp(3).
 *
 *   Function:
 *
 *	frexp	Decompose a floating point number
 *
 *   Compile:
 *
 *	(CC) -c frexp.c (for D-float version)
 *	(CC) -c -Mg frexp.c (for G-float version)
 *
 *   Modification history:
 *
 *	Created November 12, 1987, Jon Reeves.  Based on algorithm in
 *	BSD 4.3 source.  Motivations: fix bug with 2**n case in BSD
 *	machine independent case/ULTRIX V2.2; improve speed to near
 *	BSD 4.3 assembler version by depending on VAX floating point 
 *	format without introducing dependence on VAX instruction set.
 */

/*
 * Function:
 *
 *	frexp
 *
 * Function Description:
 *
 *	This function accepts a double precision number and returns its
 *	mantissa and exponent components separately.
 *
 * Arguments:
 *
 *	name  	type  	description  		side effects 
 *	value	double	value to decompose	none
 *	eptr	int *	destination of exponent	exponent stored here
 *
 * Return value:
 *
 *	name 	type	description		values
 *	--	double	mantissa portion	0.5<=x<1.0 or x=0.0
 *	*eptr	int	exponent portion	-2**n<=*eptr<=2**n
 *						(n=7 or 10 for D or G float)
 */

/*	Define exponent field size in bits	*/
#ifdef	GFLOAT
#define	EXPSIZE	11
#else
#define	EXPSIZE	8
#endif

/*	Exponent is biased by half its maximum value	*/
#define	BIAS	(1<<(EXPSIZE-1))

/*	Floating point number decomposed into fields.  Only exp is used */
struct	fpvar {
	int	frac1: 15-EXPSIZE;
	unsigned int	exp: EXPSIZE;
	unsigned int	sign: 1;
	int	frac2: 16;
	long	frac3;
};

double
frexp(value, eptr)
double	value;
int	*eptr;
{
/*
	This union allows us to access the floating exponent directly
	without compiler error messages.
 */
	union u_fp {
		double	dvar;
		struct fpvar f;
	} kludge;

	kludge.dvar = value;

/*
	If the exponent field is nonzero, we unbias it and return the
	fraction with a biased zero exponent.  Otherwise, the fields
	are returned unchanged.  This coding sequence has been tuned for
	optimal code under both pcc and vcc.
 */
	if ((*eptr = kludge.f.exp) != 0) {
		*eptr -= BIAS;
		kludge.f.exp = BIAS;
	}
	return kludge.dvar;
}
