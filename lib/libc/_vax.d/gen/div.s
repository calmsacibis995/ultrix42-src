/* "@(#)div.s	4.1	(ULTRIX)	7/3/90" */

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
 *			Modification History
 *
 *	Ken Lesniak, 02-Jun-1989
 * 001	Created for ANSI compliance.
 *
 ************************************************************************/

#include "DEFS.h"

/*
 * div_t div(int numer, int denom)
 * ldiv_t ldiv(long numer, long denom)
 *
 * This routine implements the ANSI div and ldiv functions, which are
 * identical on a VAX, since an int and long are the same size. The
 * numerator "numer" is divided by the denominator "denom" and the
 * quotient and remainder are returned in a div_t/ldiv_t structure.
 *
 * It is assumed that ordering of the fields in both the div_t
 * and ldiv_t structures are the quotient field first, followed by
 * the remainder field.
 *
 * There is no check for division by zero, since ANSI specifies it
 * as "undefined behavior". For a vax, division by zero will result
 * in a signal being delivered to the process.
 */

	.lcomm	result, 8	# a scratch div_t/ldiv_t for return value

	.text

	.globl	_div
	.globl	_ldiv
_div:
_ldiv:
	.word	0
#ifdef PROF
	.data; 1:; .long 0; .text; moval 1b,r0; jsb mcount
#endif PROF
	movaq	result, r0	# point to result structure
	movl	4(ap), r1	# get the numerator
	extv	$31, $1, r1, r2	# sign extend to quadword
	ediv	8(ap), r1, (r0), 4(r0) # get the quotient and remainder
	ret			# return with r0 pointing to structure
