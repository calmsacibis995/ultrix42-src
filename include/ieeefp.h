/*	@(#)ieeefp.h	4.2	(ULTRIX)	9/4/90	*/
#include <ansi_compat.h>
#ifdef __mips
/* Derived from nan.h, which carried these notices: */
/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Handling of Not_a_Number's (only in IEEE floating-point standard) */
typedef union 
{
         struct	
	 {
#ifdef __MIPSEL
	    unsigned fraction_low:32;
            unsigned bits:20;
	    unsigned exponent :11;
	    unsigned sign     : 1;
         } inf_parts;
	 struct 
	 {
	    unsigned fraction_low: 32;
	    unsigned bits     :19;
	    unsigned qnan_bit : 1;
            unsigned exponent :11;
	    unsigned sign     : 1;
#else
	    unsigned sign     : 1;
	    unsigned exponent :11;
            unsigned bits:20;
	    unsigned fraction_low:32;
         } inf_parts;
	 struct 
	 {
	    unsigned sign     : 1;
            unsigned exponent :11;
	    unsigned qnan_bit : 1;
	    unsigned bits     :19;
	    unsigned fraction_low: 32;
#endif
         } nan_parts;
         double d;

} dnan; 

typedef union 
{
         struct	
	 {
#ifdef __MIPSEL
            unsigned exponent : 8;
	    unsigned sign     : 1;
	    unsigned bits     :24;
         } inf_parts;
	 struct 
	 {
            unsigned exponent : 8;
	    unsigned sign     : 1;
	    unsigned qnan_bit : 1;
	    unsigned bits     :23;
#else
	    unsigned bits     :24;
	    unsigned sign     : 1;
            unsigned exponent : 8;
         } inf_parts;
	 struct 
	 {
	    unsigned bits     :23;
	    unsigned qnan_bit : 1;
	    unsigned sign     : 1;
            unsigned exponent : 8;
#endif
         } nan_parts;
         float f;

} fnan; 

#define isnand(X)  ((((dnan *)&(X))->inf_parts.exponent == 0x7ff)&&\
	(((dnan *)&(X))->inf_parts.fraction_low != 0)&&\
	(((dnan *)&(X))->inf_parts.bits != 0))
#define isnanf(X)  ((((fnan *)&(X))->inf_parts.exponent == 0xff)&&\
	(((fnan *)&(X))->inf_parts.bits != 0))

#else

typedef double dnan;
#define IsINF(X)   0
#define IsPINF(X)  0
#define IsNegNAN(X)  0
#define IsPosNAN(X)  0
#define IsNAN(X)   0
#define GETNaNPC   0L

#define Nan(X)  0
#define KILLNaN(X)
#endif /* __mips */
