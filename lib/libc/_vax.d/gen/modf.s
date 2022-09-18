/*	@(#)modf.s	4.1	Ultrix	7/3/90	*/

/*
 * double modf (value, iptr)
 * double value, *iptr;
 *
 * Modf returns the fractional part of "value",
 * and stores the integer part indirectly through "iptr".
 */

#include "DEFS.h"

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
 *			Modification History
 *
 *	David L Ballenger, 17-Dec-1984
 * 002	Fix floating point operations inside GFLOAT conditionals, so
 *	GFLOAT instructions are used.
 *
 *	Stephen Reilly, 14-May-84
 * 001 - Modified the module so that it can be built for the gfloat 
 *	 C library
 *
 ***********************************************************************/

ENTRY(modf)
#ifdef	GFLOAT
	emodg	4(ap),$0,$0f1.0,r2,r0	# DLB002
	jvs	1f			# integer overflow
	cvtlg	r2,*12(ap)		# DLB002
	ret
1:
	subg3	r0,4(ap),*12(ap)	# DLB002
	ret
#else

	emodd	4(ap),$0,$0f1.0,r2,r0
	jvs	1f			# integer overflow
	cvtld	r2,*12(ap)
	ret

1:
	subd3	r0,4(ap),*12(ap)
	ret
#endif
