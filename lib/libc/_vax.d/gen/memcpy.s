/* @(#)memcpy.s	4.1 (ULTRIX) 7/3/90 */

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
 *
 *		Modification History
 *
 * 0000	Ken Lesniak, 04-Apr-1989
 *	Derived from bcopy.s
 *
 ************************************************************************/

/* memcpy(to, from, size) */

#include "DEFS.h"

	.globl	_memcpy
_memcpy:
	.word	0x0040
#ifdef PROF
	.data; 1:; .long 0; .text; moval 1b,r0; jsb mcount
#endif PROF
	movl	8(ap),r1
	movl	4(ap),r3
	movl	12(ap),r6
	jbr	2f
1:
	subl2	r0,r6
	movc3	r0,(r1),(r3)
2:
	movzwl	$65535,r0
	cmpl	r6,r0
	jgtr	1b
	movc3	r6,(r1),(r3)
	movl	4(ap),r0
	ret
