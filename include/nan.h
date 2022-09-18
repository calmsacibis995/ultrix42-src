/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: nan.h,v 2010.5.1.5 89/11/29 22:41:16 bettina Exp $ */
/* $Log:	nan.h,v $
 * Revision 2010.5.1.5  89/11/29  22:41:16  bettina
 * 2.10 BETA2
 * 
 * Revision 2010.1  89/09/26  20:47:56  lai
 * updated to 2.10
 * 
 * Revision 1.3  89/09/21  14:46:36  bettina
 * updating copyright
 * 
 * Revision 1.2  89/07/19  15:10:33  lai
 * fix bug 4448
 * 
 * Revision 1.1  89/07/19  15:03:56  lai
 * Initial revision
 *  */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Handling of Not_a_Number's (only in IEEE floating-point standard) */
#include <ansi_compat.h>
#if _IEEE
typedef union 
{
         struct	
	 {
#ifdef __MIPSEL
	    unsigned fraction_low:32;
            unsigned bits:20;
	    unsigned exponent :11;
	    unsigned sign     : 1;
#else
	    unsigned sign     : 1;
	    unsigned exponent :11;
            unsigned bits:20;
	    unsigned fraction_low:32;
#endif
         } inf_parts;
	 struct 
	 {
#ifdef __MIPSEL
	    unsigned fraction_low: 32;
	    unsigned bits     :19;
	    unsigned qnan_bit : 1;
            unsigned exponent :11;
	    unsigned sign     : 1;
#else
	    unsigned sign     : 1;
            unsigned exponent :11;
	    unsigned qnan_bit : 1;
	    unsigned bits     :19;
	    unsigned fraction_low: 32;
#endif
         } nan_parts;
         double d;

} dnan; 

	/* IsNANorINF checks that exponent of double == 2047 *
	 * i.e. that number is a NaN or an infinity	     */
	
#define IsNANorINF(X)  (((dnan *)&(X))->nan_parts.exponent == 0x7ff)

	/* IsINF must be used after IsNANorINF		*
 	 * has checked the exponent 			*/

#define IsINF(X)  (((dnan *)&(X))->inf_parts.bits == 0 &&  \
                    ((dnan *)&(X))->inf_parts.fraction_low == 0)

	/* IsPosNAN and IsNegNAN can be used 		*
 	 * to check the sign of infinities too		*/

#define IsPosNAN(X)  (((dnan *)&(X))->nan_parts.sign == 0)

#define IsNegNAN(X)  (((dnan *)&(X))->nan_parts.sign == 1)

	/* GETNaNPC gets the leftmost 32 bits 		*	
	 * of the fraction part				*/

#define GETNaNPC(dval)   (((dnan *)&(dval))->inf_parts.bits << 12 | \
			  ((dnan *)&(dval))->nan_parts.fraction_low>> 20) 

#define KILLFPE()       (void) kill(getpid(), 8)
#define NaN(X)  (((dnan *)&(X))->nan_parts.exponent == 0x7ff)
#define KILLNaN(X)      if (NaN(X)) KILLFPE()

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
#endif
