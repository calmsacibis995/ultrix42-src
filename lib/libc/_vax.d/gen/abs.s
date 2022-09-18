/* "@(#)abs.s	4.1	(ULTRIX)	7/3/90" */

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
 *			Modification History				*
 *									*
 *	Ken Lesniak, 02-Jun-1989					*
 * 001	Added entry point for labs().					*
 *									*
 ************************************************************************/

/* abs - int absolute value */
/* labs - long absolute value */

#include "DEFS.h"

	.globl	_abs
	.globl	_labs
_abs:
_labs:
	.word	0
#ifdef PROF
	.data; 1:; .long 0; .text; moval 1b,r0; jsb mcount
#endif PROF
	movl	4(ap),r0
	bgeq	1f
	mnegl	r0,r0
1:
	ret
