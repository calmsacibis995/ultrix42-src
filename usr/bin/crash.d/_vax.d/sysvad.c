#ifndef lint
static char *sccsid = "@(#)sysvad.c	4.2	(ULTRIX)	7/17/90";
#endif

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

#include	"../crash.h"

int sptptr;	/* offset of system page table in core file */
int sptlen;	/* length of system page table (in pte's) */

extern struct Symbol *symsrch();

vaddrinit()
{
	struct Symbol *sp;

	sp = symsrch ("_Syssize");
	if (!sp) fatal ("can't find symbol Syssize");
	sptlen = (int)sp->s_value;
	sptptr = SYM_VALUE (&Sptbase);
}


sysvad(vad)           /* map a vaddr in system space to core offset */
	unsigned vad;
{
	register unsigned r;	 /* addr during translation */
	unsigned  spte;	 /* pte for this page */
	int       binp;	 /* byte in page */

	r = vad & VIRT_MEM;
	binp = r  & 0x1ff;

	r  = (r >> 9) * 4 + sptptr;
	lseek(mem, (int)r, 0);
	read(mem, (char *)&spte, sizeof spte);
	if (!(spte & 0x80000000)) return(-1);
	r = spte & 0x1fffff;
	
	return((r << 9) | binp);
}

readmem(buf, vad, len)
	char *buf;
	int vad;
	int len;
{
	int	physaddr,
		binp,
		get,
		start;

	binp = vad & 0x1ff;
	start = 0;

	/* We do a virtual-to-physical page mapping every time we cross
	 * a hardware page boundary.  We should wait until we cross a
	 * click boundary since the paging system pages in 1k units.
	 */

	while (len) {
		physaddr = sysvad((unsigned)vad);
		get = (binp + len > 0x200) ? 0x200 - binp : len;
		if (lseek(mem, (int)physaddr, 0) == -1)
			return(0);
		if (read(mem, &buf[start], get) != get)
			return(0);
		start += get;
		len -= get;
		vad = (vad + 0x200) & ~0x1ff;
		binp = 0;
	}
	return(start);
}

readsym (sp, buf, len)
struct Symbol *sp;
char *buf;
int len;
{
	if (!sp) fatal ("can't find needed symbol");
	if (readmem (buf, (int)sp->s_value, len) != len)
		fatal (strcat("can't read symbol ",sp->s_name));
}
