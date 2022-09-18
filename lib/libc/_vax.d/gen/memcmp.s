/* @(#)memcmp.s	4.1 (ULTRIX) 7/3/90 */

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
 * 0001	Ken Lesniak, 03-Oct-1989
 *	Treat comparision as unsigned
 *
 * 0000	Ken Lesniak, 04-Apr-1989
 *	Derived from bcmp.s
 *
 ************************************************************************/

/* memcmp(s1, s2, n) */

#include "DEFS.h"

ENTRY(memcmp)
	movl	4(ap),r1
	movl	8(ap),r3
	movl	12(ap),r4
1:
	movzwl	$65535,r0
	cmpl	r4,r0
	jleq	3f
	subl2	r0,r4
	cmpc3	r0,(r1),(r3)
	jeql	1b

	/* We get here if a byte compared unequal */
	/* The C-bit is set if s1 < s2; otherwise it is clear */
2:
	sbwc	r0, r0			/* r0 = (s1 < s2 ? -1 : 0) */
	addl2	r0, r0			/* r0 = (s1 < s2 ? -2 : 0) */
	incl	r0			/* r0 = (s1 < s2 ? -1 : 1) */
	ret
3:
	cmpc3	r4,(r1),(r3)
	jneq	2b
	ret
