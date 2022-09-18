#ifndef lint
static char *sccsid = "@(#)text.c	4.2	(ULTRIX)	7/17/90";
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

#include	"crash.h"
#include	<sys/ipc.h>
#include	<sys/shm.h>
#include	<sys/text.h>
#include	<sys/smp_lock.h>
#include	<sys/gnode.h>
#include	<sys/proc.h>
#include	<machine/pte.h>
#include	<machine/param.h>
#include	<machine/vmparam.h>
#include	<sys/vmmac.h>

prtext(c, all)
	int	c;
	int	all;
{
	struct	text	tbuf;
	char	x_flag;
	
	if(c == -1)
		return;
	if(c >= tab[TEXT_T].ents) {
		printf("%4d  out of range\n", c);
		return;
	}
	if(readmem((char *)&tbuf,(int)(tab[TEXT_T].first + c*sizeof tbuf),
	    sizeof tbuf) != sizeof tbuf) {
			printf("%4d  read error on text table\n", c);
			return;
	}
	if(!all && tbuf.x_gptr == NULL)
		return;
	printf("%4u %5u %3u  %5u",
	    c, tbuf.x_gptr ? gnodetab[getindex((char *)tbuf.x_gptr,
	    gnodebuckets, GNODEBUCKETS)].g_number : -1, tbuf.x_count,
	    tbuf.x_ccount);
	printf(" %4u",
	    tbuf.x_caddr ? getindex((char *)tbuf.x_caddr, procbuckets, 
	    PROCBUCKETS) : 0);
	printf(" %4u %4d", tbuf.x_size, tbuf.x_rssize);

	x_flag = tbuf.x_flag;
	printf("%s%s%s%s%s%s\n",
		x_flag & XTRC ? " exclu-write" : "",
		x_flag & XWRIT ? " write" : "",
		x_flag & XLOAD ? " load" : "",
		x_flag & XLOCK ? " lock" : "",
		x_flag & XWANT ? " want" : "",
		x_flag & XPAGI ? " page" : "");
}
/* uarea to pte macro to dump UAREA PTES */
#ifdef vax
#define UATOPTE(p,i) (((p)->p_p0br + (p)->p_szpt*NPTEPG) - (i))
#endif
#ifdef mips
#define UATOPTE(p,i) (((p)->p_stakbr + (p)->p_stakpt*NPTEPG) - (i))
#endif

prpte(c)
	int	c;
{
	struct proc *p;
	struct	text	tbuf;
	int	i;
	
	if(c == -1)
		return;
	if(c >= tab[PROC_T].ents) {
		printf("%4d  out of range\n", c);
		return;
	}
	p = &proctab[c];
	if ((p->p_sched & SLOAD) == 0) {
	        printf("process swapped out; no page tables\n");
		return; 
	}

	if(readmem((char *)&tbuf, (int)p->p_textp, sizeof tbuf) !=
	    sizeof tbuf) {
		printf("%4d  read error on text table\n", ((int)
		p->p_textp - (int) tab[TEXT_T].first) / sizeof tbuf);
		return;
	}
	for(i = 0; i < tbuf.x_size; i++)
		printpte(tptopte(p, i), "text");
	for(i = 0; i < p->p_dsize; i++)
		printpte(dptopte(p, i), "data");
	if (p->p_smbeg)
	        printsmpte(p);
	for (i = (p->p_ssize - 1); i >= 0; i--)
		printpte(sptopte(p, i), "stack");
	for (i = UPAGES; i > 0; i--)
	        printpte(UATOPTE(p, i), "uarea");
}

void
fprtext(c)
	int	c;
{
	struct	text	tbuf;
	char	x_flag;
	struct gnode *gp;
	int gslot;	
	
	if(c == -1)
		return;
	if(c >= tab[TEXT_T].ents) {
		printf("%4d  out of range\n", c);
		return;
	}
	if(readmem((char *)&tbuf,(int)(tab[TEXT_T].first + c*sizeof tbuf),
	sizeof tbuf) != sizeof tbuf) {
			printf("%4d  read error on text table\n", c);
			return;
	}
	if(tbuf.x_count != NULL)
		return;
	if(tbuf.x_gptr) {
		gslot = getindex((char *)tbuf.x_gptr, gnodebuckets,
		    GNODEBUCKETS);
		if(gslot == -1) {
			error("text->g_ptr not valid");
			return;
		}
		gp = &gnodetab[gslot];
	}
	if((gp->g_mode & GSVTX) == GSVTX)
		return;
	printf("%4u %5u %3u  %5u",
		c, tbuf.x_gptr ? gslot : 0, tbuf.x_count, tbuf.x_ccount);
	printf(" %4u",
	    tbuf.x_caddr ? getindex((char *) tbuf.x_caddr, procbuckets,
	    PROCBUCKETS) : 0);
	printf(" %4u %4d", tbuf.x_size, tbuf.x_rssize);
#ifdef notdef
	if(((char *) tbuf.x_freef < (char *) tab[TEXT_T].first) ||
	((char *) tbuf.x_freef > (char *) tab[TEXT_T].last))
		printf(" head/");
	else
		printf(" %4d/", ((char *) tbuf.x_freef - (char *)
		tab[TEXT_T].first) / sizeof(struct text));
	if(((char *) tbuf.x_freeb < (char *) tab[TEXT_T].first) ||
	((char *) tbuf.x_freeb > (char *) tab[TEXT_T].last))
		printf("head ");
	else
		printf("%-4d ", ((char *) tbuf.x_freeb - (char *)
		tab[TEXT_T].first) / sizeof(struct text));
#endif
	x_flag = tbuf.x_flag;
	printf("%s%s%s%s%s%s\n",
		x_flag & XTRC ? " exclu-write" : "",
		x_flag & XWRIT ? " write" : "",
		x_flag & XLOAD ? " load" : "",
		x_flag & XLOCK ? " lock" : "",
		x_flag & XWANT ? " want" : "",
		x_flag & XPAGI ? " page" : "");
}

#ifdef vax
/*
 * Since the VAX protection scheme is difficult at best, and since
 * it probably will not change, I will define the appropriate masks
 * here
 */
#define READ_MASK  0x4
#define READ_PROT  0x4
#define WRITE_MASK  0xc
#define WRITE_PROT 0x4
#endif
#ifdef mips
#define READ_MASK  0x0
#define READ_PROT  0x0
#define WRITE_MASK  0x1
#define WRITE_PROT 0x1
#endif

printpte(pteaddr, cp)
	struct pte *pteaddr;
	char *cp;
{
	struct pte _pte;
	register struct pte *pte = &_pte;
	register int prot;
	
	if(readmem((char *)pte, (int)pteaddr, sizeof pte) != sizeof pte) {
		printf("      pte not valid\n");
		return;
	}


	

/*	printf("   PTE      Frame            Status           Segment Prot\n");
	        xxxxxxxx xxxxxxxxxx valid modified fill alloc */
	printf("%8x %10x %s %s %s %s %5s", pteaddr, pte->pg_pfnum, 
	    pte->pg_v ? "valid" : "     ",
	    pte->pg_m ? "modified" : "        ",
	    pte->pg_fod ? "fill" : "    ",
#ifdef vax
	    pte->pg_alloc ? "alloc" : "     ",
#endif
#ifdef mips
	    "      ",
#endif
	    cp);

	prot = pte->pg_prot;
	printf("   U(%s%s)\n",
	    ((prot & READ_MASK) == READ_PROT) ? "r" : "", 
	    ((prot & WRITE_MASK) == WRITE_PROT) ? "w" : "");
}

printkpte(pteaddr, cp)
	struct pte *pteaddr;
	char *cp;
{
	struct pte _pte;
	register struct pte *pte = &_pte;
	register int prot;

	if (lseek(mem, (int)pteaddr, 0) == -1) {
		printf("      pte not valid (lseek)\n");
		return;
	}
	if (read(mem, (char *)pte, sizeof pte) != sizeof pte) {
		printf("      pte not valid (read)\n");
		return;
	}


/*	printf("   PTE      Frame            Status           Segment Prot\n");
	        xxxxxxxx xxxxxxxxxx valid modified fill alloc */
	printf("%08x %10x %s %s %s %s %5s", pteaddr, pte->pg_pfnum, 
	    pte->pg_v ? "valid" : "     ",
	    pte->pg_m ? "modified" : "        ",
	    pte->pg_fod ? "fill" : "    ",
#ifdef vax
	    pte->pg_alloc ? "alloc" : "     ",
#endif
#ifdef mips
	    "      ",
#endif
	    cp);

	prot = pte->pg_prot;
	printf("   U(%s%s)\n",
	    ((prot & READ_MASK) == READ_PROT) ? "r" : "", 
	    ((prot & WRITE_MASK) == WRITE_PROT) ? "w" : "");
}


printsmpte(p)
	register struct proc *p;
{
	register int i, j;
	register struct pte *pte;
	register struct smem *sp;
	register int size;
	struct smem sm_buf;
	char buffer[8];
  
	for (i = 0; i < SMSEG; i++) {
		if ((sp = p->p_sm[i].sm_p) == NULL)
			continue;
		if(readmem((char *)&sm_buf, (int)sp, sizeof sm_buf) !=
		    sizeof sm_buf) {
			printf("0x%x  read error on smem table\n", sp);
			return;
		}
		sprintf(buffer,"smem%d",i);
		size = clrnd(btop(sm_buf.sm_size));
#ifdef vax
		for (j = 0, pte = p->p_p0br + p->p_sm[i].sm_saddr;
		    j < size; j++, pte++)
			printpte(pte,buffer);
#endif
#ifdef mips
#endif
	}
}
