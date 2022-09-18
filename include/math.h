/*	@(#)math.h	4.2	(ULTRIX)	9/4/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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
 *			Modification History				*
 *
 * 011	DECwest ANSI 8-jun-1990
 *	atof now defined always, not just if ! _POSIX_SOURCE.
 *									*
 * 010  Dan Smith, 1990 Feb 21                                          *
 *      Added const to atof prototype. Protect symbols not required     *
 *      for ANSI/POSIX/X-OPEN compliance.                               *
 *                                                                      *
 * 009	Reeves, 1989 Dec 07						*
 *	Namespace protection.						*
 *									*
 * 008	Reeves, 1989 Jul 14						*
 *	Add isnan, lgamma declarations for X/Open			*
 *									*
 * 007	Reeves, 1989 Jun 16						*
 *	Make HUGE_VAL more amenable to multiple definitions (limits.h	*
 *	needs it too).							*
 *									*
 * 006	Reeves, 1989 Jun 05						*
 *	Add HUGE_VAL for ANSI, strtod, atol for historical reasons	*
 *									*
 * 005	Reeves, 1988 Dec 14						*
 *	X/Open requires MAXFLOAT both places; add it back here with	*
 *	the right value.						*
 *									*
 * 004	Jon Reeves, 1988 Nov 16						*
 *	Fix MAXFLOAT collision with values.h				*
 *									*
 * 003	Jon Reeves, 1988 Sept 26					*
 *	Add MIPS definitions						*
 *									*
 * 002	Jon Reeves, 1988 March 07					*
 *	Delete if/endif pairs for other machines that gave vcc trouble. *
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001 Add definitions for system V compatibility			*
 *									*
 ************************************************************************/


#include <ansi_compat.h>
#ifndef _POLY9
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
extern int errno, signgam;
#endif
#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
struct exception {
	int type;
	char *name;
	double arg1;
	double arg2;
	double retval;
};
#endif
#ifdef __STDC__
/*
 * prototypes
 *
 */
extern double	acos( double __x );
extern double	asin( double __x );
extern double	atan( double __x );
extern double	atan2( double __x, double __y );
extern double	ceil( double __x );
extern double	cos( double __x );
extern double	cosh( double __x );
extern double	exp( double __x );
extern double	fabs( double __x );
extern double	floor( double __x );
extern double	fmod( double __x, double __y );
extern double	frexp( double __value, int *__eptr );
extern double	ldexp( double __value, int __exp );
extern double	log( double __x );
extern double	log10( double __x );
extern double	modf( double __value, double *__iptr);
extern double	pow( double __x, double __y );
extern double	sin( double __x );
extern double	sinh( double __x );
extern double	sqrt( double __x );
extern double	tan( double __x );
extern double	tanh( double __x );
extern double	atof( const char *__nptr );
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
extern double	j0( double __x );
extern double	j1( double __x );
extern double	jn( int __n, double __x );
extern double	y0( double __x );
extern double	y1( double __x );
extern double	yn( int __n, double __x );
extern double	erf( double __x );
extern double	erfc( double __x );
extern double	gamma( double __x );
extern double   lgamma( double __x );
extern double	hypot( double __x, double __y );
extern int 	isnan();
#endif
#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
extern int	matherr(struct exception *__x);
#endif

#else

extern double atof(), frexp(), ldexp(), modf(), strtod();
extern double exp(), log(), log10(), pow(), sqrt();
extern double floor(), ceil(), fmod(), fabs();
extern double sinh(), cosh(), tanh();
extern double sin(), cos(), tan(), asin(), acos(), atan(), atan2();
extern long atol();
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
extern double j0(), j1(), jn(), y0(), y1(), yn();
extern double erf(), erfc();
extern double gamma();
extern double lgamma();
extern double hypot();
extern int isnan();
#endif
#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
extern int matherr();
#endif
#endif /* __STDC__ */

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
/* some useful constants */
#define M_E	2.7182818284590452354
#define M_LOG2E	1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2	0.69314718055994530942
#define M_LN10	2.30258509299404568402
#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923
#define M_PI_4	0.78539816339744830962
#define M_1_PI	0.31830988618379067154
#define M_2_PI	0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2	1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440
#endif

#ifdef	__vax
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define MAXFLOAT	((float)1.701411733192644299e+38)
#endif
#if	defined(__GFLOAT) || CC$gfloat
#define HUGE_VAL	8.9884656743115790e+307 
#else
#define HUGE_VAL	1.701411834604692293e+38 
#endif /* __GFLOAT */
#endif
#ifdef	__mips /* This number generates +Infinity */
#define	HUGE_VAL	1.8e+308
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define	MAXFLOAT	((float)3.40282346638528860e+38)
#endif
#endif

#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define HUGE HUGE_VAL
#endif

/* These names are OK according to ANSI extension rules */
#define _ABS(x)	((x) < 0 ? -(x) : (x))
#define _REDUCE(TYPE, X, XN, C1, C2)	{ \
	double x1 = (double)(TYPE)X, x2 = X - x1; \
	X = x1 - (XN) * (C1); X += x2; X -= (XN) * (C2); }
#define _POLY1(x, c)	((c)[0] * (x) + (c)[1])
#define _POLY2(x, c)	(_POLY1((x), (c)) * (x) + (c)[2])
#define _POLY3(x, c)	(_POLY2((x), (c)) * (x) + (c)[3])
#define _POLY4(x, c)	(_POLY3((x), (c)) * (x) + (c)[4])
#define _POLY5(x, c)	(_POLY4((x), (c)) * (x) + (c)[5])
#define _POLY6(x, c)	(_POLY5((x), (c)) * (x) + (c)[6])
#define _POLY7(x, c)	(_POLY6((x), (c)) * (x) + (c)[7])
#define _POLY8(x, c)	(_POLY7((x), (c)) * (x) + (c)[8])
#define _POLY9(x, c)	(_POLY8((x), (c)) * (x) + (c)[9])


#if !defined(_POSIX_SOURCE)
#define DOMAIN		1
#define	SING		2
#define	OVERFLOW	3
#define	UNDERFLOW	4
#define	TLOSS		5
#define	PLOSS		6
#endif

#endif
