#ifndef lint
static	char	*sccsid = "@(#)cksum.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
/* Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *static char sccsid[] = "@(#)cksum.c	1.2 (Berkeley) 9/18/85";
 */

#include <sys/types.h>

#define ADD	asm("adwc (r9)+,r8;");

/* computes the checksum for ip packets for the VAX */

in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;	/* on vax, (user mode), r11 */
#ifndef lint
	register int xxx;		/* on vax, (user mode), r10 */
#endif not lint
	register u_short *w = addr;	/* on vax, known to be r9 */
	register int sum = 0;		/* on vax, known to be r8 */

	if (((int)w&0x2) && nleft >= 2) {
		sum += *w++;
		nleft -= 2;
	}
	while ((nleft -= 32) >= 0) {
		asm("clrl r0");		/* clears carry */
		ADD; ADD; ADD; ADD; ADD; ADD; ADD; ADD;
		asm("adwc $0,r8");
	}
	nleft += 32;
	while ((nleft -= 8) >= 0) {
		asm("clrl r0");
		ADD; ADD;
		asm("adwc $0,r8");
	}
	nleft += 8;
	{ asm("ashl $-16,r8,r0; addw2 r0,r8");
	  asm("adwc $0,r8; movzwl r8,r8"); }
	while ((nleft -= 2) >= 0) {
		asm("movzwl (r9)+,r0; addl2 r0,r8");
	}
	if (nleft == -1) {
		sum += *(u_char *)w;
	}

	{ asm("ashl $-16,r8,r0; addw2 r0,r8; adwc $0,r8");
	  asm("mcoml r8,r8; movzwl r8,r8"); }
	return (sum);
}
