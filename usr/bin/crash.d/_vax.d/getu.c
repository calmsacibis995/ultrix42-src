#ifndef lint
static char *sccsid = "@(#)getu.c	4.1	(ULTRIX)	7/17/90";
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
#include	<sys/proc.h>
#include	<machine/pte.h>
#include	<sys/cpudata.h>
#include	<sys/vm.h>
#include	<frame.h>
#include	<stdio.h>

#ifdef vax
#define KERNELSTACK	(0x7fffffff)
#endif vax

struct uarea u;			/* external uarea buffer */

char cmdbuf[CLSIZE*NBPG];	/* global command string buffer */
int argaddr;			/* global vaddr of command space */

/* getuarea: read a uarea into u, given process slot number. */

getuarea (p)
	int p;			/* proc slot number */
{
	struct proc *proc;
	void get_uarea();
	
	if (p < 0) {
		struct cpudata cpudata;
		get_cpudata(&cpudata,1);
		proc = cpudata.cpu_proc;
		if((p = getindex((char *)proc, procbuckets, PROCBUCKETS))
		     == -1) {
			error("read error on proc");
			printf("proc 0x%x\n", proc);
			return(-1);
		}
	} else
		if(!(proctab[p].p_sched & SLOAD))
			return(-1);
	get_uarea(&proctab[p]);
	return (p);
}

/* get_uarea -- read a uarea into u. */

void
get_uarea(p)
	struct proc *p;
{
	struct pte upt[2*UPAGES+CLSIZE];
	struct pte *pteaddr, apte, *uptma, *usrpt;
	int ncl, click;
	unsigned physaddr;
	int nbcl = NBPG * CLSIZE;

	uptma = (struct pte *)Usrptma.s_value;
	usrpt = (struct pte *)Usrpt.s_value;
	pteaddr = &uptma[btokmx(p->p_p0br) + p->p_szpt - 1];

	readmem((char *)&apte, (int)pteaddr, sizeof(apte));

	lseek(mem,(long)ctob(apte.pg_pfnum+1)-sizeof(upt),0);
	read(mem,upt,sizeof(upt));

	if (upt[0].pg_fod == 0 && upt[0].pg_pfnum)
		argaddr = ctob(upt[0].pg_pfnum);
	else argaddr = 0;

	ncl = (UPAGES * 512 + nbcl - 1) / nbcl;
	while (--ncl >= 0) {
		click = ncl * CLSIZE;
/*		if (!(upt[CLSIZE + click].pg_v))
			error ("missing page of uarea"); */
		physaddr = ctob(upt[click + CLSIZE + UPAGES].pg_pfnum);
		if (lseek(mem, physaddr, 0) == -1) {
			continue;
		}
		else if (read(mem, u.ustuff.upages[click], nbcl) != nbcl) {
			continue;
		}
	}
}

/*
 * read in the cpudata struct for the Ith cpu.
 */
get_cpudata(p,i)
	struct cpudata *p;
	int i;
{
	struct Symbol *symsrch();
	struct cpudata *cpudatap;
	unsigned int size, maxcpu;

	if(readmem((char *)&cpudatap, symsrch( "_cpudata")->s_value + 
		   i * sizeof cpudatap,
		   sizeof cpudatap) != sizeof(cpudatap)) {
		printf("could not get cpudata pointer\n");
		return(-1);
	}

	if ((i>32) || (cpudatap == NULL))
		return(-1);

	
	if(readmem((char *)p, (char *)cpudatap,
		   sizeof (struct cpudata)) != sizeof(struct cpudata)) {
		printf("could not get cpudata struct\n");
		return(-1);
	}
}
