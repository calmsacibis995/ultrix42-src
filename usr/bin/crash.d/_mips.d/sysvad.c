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
#include <sys/vmmac.h>
#include <machine/pte.h>
#include <machine/cpu.h>

int sptptr;	/* offset of system page table in core file */
int sptlen;	/* length of system page table (in pte's) */

extern struct Symbol *symsrch();

vaddrinit()
{
	struct Symbol *sp;

	readsym(symsrch ("_Syssize"), &sptlen, sizeof(sptlen));
	sptptr = SYM_VALUE (&Sptbase);
}


sysvad(vad)           /* map a vaddr in system space to core offset */
	unsigned vad;
{
	register unsigned r;	 /* addr during translation */
	struct pte  pte;	 /* pte for this page */

#ifdef vax
	r = vad & VIRT_MEM;
#endif
#ifdef mips
	if (IS_KSEG0(vad))
		return(K0_TO_PHYS(vad));
	if (IS_KSEG1(vad))
		return(K1_TO_PHYS(vad));
	if (IS_KSEG2(vad)) {
		r = sptptr + btop((int)(vad) & ~SEG_BITS)*sizeof(struct pte);
		lseek(mem, (int)r, 0);
		read(mem, (char *)&pte, sizeof pte);
		if (pte.pg_v && pte.pg_pfnum)
			r = (int)ptob(pte.pg_pfnum) | ((int) (vad) & VA_BYTEOFFS);
		else
			r = -1;
		return(r);
	}
	if (IS_KUSEG(vad)) {
		printf("sysvad: KUSEG - 0x%x\n",vad);
/*		if ((pte = vtopte(u.u_procp, btop(vad))) == 0)
			return(-1); */
		if (pte.pg_v && pte.pg_pfnum)
			return((int)ptob(pte.pg_pfnum) | ((int) (vad) & VA_BYTEOFFS));
		else
			return(-1); 
	}
	return(-1);
#endif
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
/*	printf("readmem: buf = 0x%x, vad = 0x%x, len = %d\n",buf,vad,len); */
	binp = vad & PGOFSET;
	start = 0;

	/* We do a virtual-to-physical page mapping every time we cross
	 * a hardware page boundary.  We should wait until we cross a
	 * click boundary since the paging system pages in 1k units.
	 */

	while (len) {
		physaddr = sysvad((unsigned)vad);
		get = (binp + len > NBPG) ? NBPG - binp : len;
		if (lseek(mem, (int)physaddr, 0) == -1)
			return(0);
		if (read(mem, &buf[start], get) != get)
			return(0);
		start += get;
		len -= get;
		vad = (vad + NBPG) & ~PGOFSET;
		binp = 0;
	}
/*	printf("readmem: got %d\n",start); */
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

