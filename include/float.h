/*	@(#)float.h	4.3	(ULTRIX)	9/4/90	*/
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
 *   This software is  derived  from  software  received  from  	*
 *   MIPS Computer Systems, Inc.  Use, duplication, or disclosure is	*
 *   subject to restrictions under license agreements with MIPS 	*
 *   Computer Systems, Inc.  						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: float.h,v 2010.5.1.5 89/11/29 22:41:15 bettina Exp $ */
/*
 * Revision History
 *
 * 001  Dan Smith, 1990 Feb 21
 *      Define long double macros equal to double counterparts. Use
 *      new ANSI constructs for float constants rather than casting to
 *      float.  
 *
 * 002	DECwest ANSI 8-jun-90
 *	Fixed extra space in definition of DBL_DIG from change 001.
 */

/*
 * This file specifies the Characteristics of floating types as required by
 * the proposed ANSI C standard.
 */

/*
 * Note: The values for values related to long doubles (LDBL_) are set
 * to the corresponding value for plain doubles. This needs to be
 * updated once the compiler supports long doubles.
 */

#include <ansi_compat.h>
#define	FLT_RADIX	2
#define	FLT_ROUNDS	1

#ifdef __mips
#define	FLT_MANT_DIG	24
#define	DBL_MANT_DIG	53
#define	LDBL_MANT_DIG	53

#if __STDC__ == 1
#define	FLT_EPSILON	1.19209290e-07f
#else
#define	FLT_EPSILON	((float)1.19209290e-07)
#endif
#define	DBL_EPSILON	2.2204460492503131e-16
#define	LDBL_EPSILON    2.2204460492503131e-16

#define FLT_DIG	6 
#define DBL_DIG	15 
#define LDBL_DIG 15

#define FLT_MIN_EXP	(-125)
#define DBL_MIN_EXP	(-1021)
#define LDBL_MIN_EXP	(-1021)

#if __STDC__ == 1
#define FLT_MIN	1.17549435e-38f
#else
#define FLT_MIN	((float)1.17549435e-38) 
#endif
#define DBL_MIN	2.2250738585072014e-308 
#define LDBL_MIN 2.2250738585072014e-308 

#define FLT_MIN_10_EXP	(-37)
#define DBL_MIN_10_EXP	(-307)
#define LDBL_MIN_10_EXP (-307)

#define	FLT_MAX_EXP	128
#define	DBL_MAX_EXP	1024
#define	LDBL_MAX_EXP	1024

#if __STDC__ == 1
#define FLT_MAX	3.40282347e+38f 
#else
#define FLT_MAX	((float)3.40282347e+38) 
#endif
#define DBL_MAX	1.7976931348623157e+308 
#define LDBL_MAX 1.7976931348623157e+308 

#define FLT_MAX_10_EXP	38
#define DBL_MAX_10_EXP	308
#define LDBL_MAX_10_EXP 308
#endif /* __mips */

#ifdef __vax
#define	FLT_MANT_DIG	24
#if __STDC__ == 1
#define	FLT_EPSILON	5.96046448e-08f
#else
#define	FLT_EPSILON	((float)5.96046448e-08)
#endif
#define FLT_DIG	6 
#define FLT_MIN_EXP	(-127)
#if __STDC__ == 1
#define FLT_MIN	2.93873588e-39f
#else
#define FLT_MIN	((float)2.93873588e-39)
#endif
#define FLT_MIN_10_EXP	(-38)
#define	FLT_MAX_EXP	127
#if __STDC__ == 1
#define FLT_MAX	1.701411733192644299e+38f
#else
#define FLT_MAX	((float)1.701411733192644299e+38) 
#endif
#define FLT_MAX_10_EXP	38

#if defined(__GFLOAT) || CC$gfloat
#define	DBL_MANT_DIG	53
#define	DBL_EPSILON	1.1102230246251570e-016
#define DBL_DIG	15 
#define DBL_MIN_EXP	(-1023)
/*
   DBL_MIN and DBL_MAX are problematic, since pcc doesn't use G-float
   internally.  The numbers are correct, but don't do much good.  This
   works fine with vcc.
 */
#define DBL_MIN	5.56268464626800350e-309 
#define DBL_MIN_10_EXP	(-308)
#define	DBL_MAX_EXP	1023
#define DBL_MAX	8.9884656743115790e+307 
#define DBL_MAX_10_EXP	307
#else
#define	DBL_MANT_DIG	56
#define	DBL_EPSILON	1.3877787807814457e-17
#define DBL_DIG	16 
#define DBL_MIN_EXP	(-127)
#define DBL_MIN	2.93873587705571880e-39 
#define DBL_MIN_10_EXP	(-38)
#define	DBL_MAX_EXP	127
#define DBL_MAX	1.701411834604692293e+38 
#define DBL_MAX_10_EXP	38
#endif
#define	LDBL_MANT_DIG	DBL_MANT_DIG
#define	LDBL_EPSILON	DBL_EPSILON
#define LDBL_DIG	DBL_DIG
#define LDBL_MIN_EXP	DBL_MIN_EXP
#define LDBL_MIN	DBL_MIN
#define LDBL_MIN_10_EXP	DBL_MIN_10_EXP
#define	LDBL_MAX_EXP	DBL_MAX_EXP
#define LDBL_MAX	DBL_MAX
#define LDBL_MAX_10_EXP	DBL_MAX_10_EXP
#endif
