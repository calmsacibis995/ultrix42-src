/*	@(#)ldexp.s	1.5		1/23/85	*/

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

/************************************************************************
 *
 *		Modification History
 *
 *	Stephen Reilly, 14-May-84
 * 003- Changed the synbol MVAXI to GFLOAT to reflect the meaning of the code
 *
 *	Stephen Reilly, 04-Apr-84
 * 002- Changed the symbol MicroVAX to MVAXI
 *
 *	Stephen Reilly, 05-Oct-83:
 * 001 - Modified code so it can handle glfoat numbers. 
 *
 ************************************************************************/

/*
 * double ldexp (value, exp)
 * double value;
 * int exp;
 *
 * Ldexp returns value*2**exp, if that result is in range.
 * If underflow occurs, it returns zero.  If overflow occurs,
 * it returns a value of appropriate sign and largest
 * possible magnitude.  In case of either overflow or underflow,
 * errno is set to ERANGE.  Note that errno is not modified if
 * no error occurs.
 */

#include "DEFS.h"
#define	KERNEL
#include <errno.h>
#undef	KERNEL

	.globl	_errno

ENTRY(ldexp)
#ifdef	GFLOAT

	movg	4(ap),r0	# slr001 Fetch "value"
	extzv	$4,$11,r0,r2	# slr001 r2 = exponent
#else
	movd	4(ap),r0	# Fetch "value"
	extzv	$7,$8,r0,r2	# r2 := biased exponent
#endif

	jeql	1f		/* if zero, done */

	addl2	12(ap),r2	/* r2 := new biased exponent */
	jleq	2f		/* if <= 0, underflow */

#ifdef	GFLOAT
	cmpl	r2,$2048	# slr001 check for do big 
#else
	cmpl	r2,$256		# Otherwise check if it's too big
#endif

	jgeq	3f		/* jump if overflow */

#ifdef	GFLOAT
	insv	r2,$4,$11,r0	# slr001 put exponent back in the result
#else
	insv	r2,$7,$8,r0	# Put the exponent back in the result
#endif

1:
	ret
2:

/*
 *	We are here if underflow
*/

#ifdef	GFLOAT
	clrg	r0		# slr001 Result is zero
#else
	clrd	r0		# Result is zero
#endif
	jbr	1f
3:
/*
 *	We are here if overflow
*/
#ifdef	GFLOAT
	movg	huge,r0		# slr001 Largest possible floating magnitude
	jbc	$15,4(ap),1f	# slr001 Jump if argument was positive
	mnegg	r0,r0		# slr001 If arg < 0, make result negative
#else
	movd	huge,r0		# Largest possible floating magnitude
	jbc	$15,4(ap),1f	# Jump if argument was positive
	mnegd	r0,r0		# If arg < 0, make result negative
#endif
1:
	movl	$ERANGE,_errno
	ret

	.data
huge:	.word	0x7fff		/* the largest number that can */
	.word	0xffff		/*   be represented in a long floating */
	.word	0xffff		/*   number.  This is given in hex in order */
	.word	0xffff		/*   to avoid floating conversions */
