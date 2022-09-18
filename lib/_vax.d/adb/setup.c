#ifndef lint
static char *sccsid = "@(#)setup.c	4.1	ULTRIX	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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
 * 09 Nov 89 -- jaw
 *	expanded $x command to print locks held.
 *
 * 12 jan 88 -- jaw
 *	expanded $x command function.
 *
 * George M. Mathew, 14-Mar-1988
 *	Changes for the new cpudata structure for SMP 
 *
 * 08 Apr 86 -- depp
 *	Removed reference to u_exdata as it no longer exists
 *
 * 05-Feb-86 -- jrs
 *	Changed ref of masterpaddr to cpudata for mp debugging
 *
 * 002	David L Ballenger, 25-Oct-1985
 *	Changed too many %X's to %x in the previous fix.  Those in calls
 *	to adb's internal printf routine should remain %X, since it has
 *	a different meaning than those used in external sprintf() calls.
 *
 * 001	David L Ballenger, 12-Sep-1985
 *	Change all %X to %x in [fs]printf() format strings, so that all
 *	hexadecimal numbers print in lower case.  This restores the old
 *	behavior of adb, which changed when the %X bug was fixed in the
 *	[fs]printf() routines.
 *
 *	Based on:  setup.c	4.6 82/10/28
 *
 ************************************************************************/
/*
 * adb - routines to read a.out+core at startup
 */
#include "defs.h"
#include <sys/stat.h>

off_t	datbas;			/* offset of the base of the data segment */
off_t	stksiz;			/* stack size in the core image */
INT	sigcode;	/* belongs in head.h */

char	*symfil	= "a.out";
char	*corfil	= "core";

setsym()
{
	off_t loc;
	struct exec hdr;
	register struct nlist *sp;
	int ssiz;
	char *strtab;

	fsym = getfile(symfil, 1);
	txtmap.ufd = fsym;
	if (read(fsym, (char *)&hdr, sizeof hdr) != sizeof hdr ||
	    N_BADMAG(hdr)) {
		txtmap.e1 = MAXFILE;
		return;
	}
	filhdr = hdr;
	loc = filhdr.a_text+filhdr.a_data;
	txtmap.f1 = txtmap.f2 = N_TXTOFF(filhdr);
	txtmap.b1 = 0;
	switch (filhdr.a_magic) {

	case OMAGIC:
		txtmap.b1 = txtmap.e1 = 0;
		txtmap.b2 = datbas = 0;
		txtmap.e2 = loc;
		break;

	case ZMAGIC:
	case NMAGIC:
		txtmap.e1 = filhdr.a_text;
		txtmap.b2 = datbas = round(filhdr.a_text, PAGSIZ);
		txtmap.e2 = datbas + filhdr.a_data;
		txtmap.f2 += txtmap.e1;
	}
	loc = N_SYMOFF(filhdr);
	symtab = (struct nlist *) malloc(filhdr.a_syms);
	esymtab = &symtab[filhdr.a_syms / sizeof (struct nlist)];
	if (symtab == NULL)
		goto nospac;
	lseek(fsym, loc, 0);
	if (filhdr.a_syms == 0)
		goto nosymt;
	/* SHOULD SQUISH OUT STABS HERE!!! */
	if (read(fsym, symtab, filhdr.a_syms) != filhdr.a_syms)
		goto readerr;
	if (read(fsym, &ssiz, sizeof (ssiz)) != sizeof (ssiz))
		goto oldfmt;
	strtab = (char *) malloc(ssiz);
	if (strtab == 0)
		goto nospac;
	*(int *)strtab = ssiz;
	ssiz -= sizeof (ssiz);
	if (read(fsym, strtab + sizeof (ssiz), ssiz) != ssiz)
		goto readerr;
	for (sp = symtab; sp < esymtab; sp++)
		if (sp->n_strx)
			/* SHOULD PERFORM RANGE CHECK HERE */
			sp->n_un.n_name = strtab + sp->n_un.n_strx;
nosymt:
	if (INKERNEL(filhdr.a_entry)) {
		txtmap.b1 += KERNOFF;
		txtmap.e1 += KERNOFF;
		txtmap.b2 += KERNOFF;
		txtmap.e2 += KERNOFF;
	}
	return;
readerr:
	printf("Error reading symbol|string table\n");
	exit(1);
nospac:
	printf("Not enough space for symbol|string table\n");
	exit(1);
oldfmt:
	printf("Old format a.out - no string table\n");
	exit(1);
}

char *cpudata_base;
setcor()
{
	char *cpudata_addr;
	int bootcpu;

	fcor = datmap.ufd = getfile(corfil,2);
	if (kernel && fcor != -1 && INKERNEL(filhdr.a_entry)) {
		struct stat stb;

		kcore = 1;
		fstat(fcor, &stb);
		datmap.b1 = 0;
		datmap.e1 = -1;
		if (kernel == 0 && (stb.st_mode & S_IFREG))
			datmap.b1 = 0x80000000;
		lookup("_Sysmap");
		sbr = cursym->n_value;
		lookup("_Syssize");
		slr = cursym->n_value;
		printf("sbr %X slr %X\n", sbr, slr);
                lookup("_boot_cpu_num");
                physrw(fcor,vtophys(cursym->n_value), &bootcpu, 1);
		lookup("_cpudata");
		cpudata_base = cursym->n_value;
		physrw(fcor, vtophys(cursym->n_value+ (bootcpu *4)) , &cpudata_addr, 1);
		physrw(fcor, ((int)cpudata_addr)&0x7fffffff, &masterpcbb, 1);
		masterpcbb = (masterpcbb&PG_PFNUM)*512;
		getpcb();
		return;
	}
	if (read(fcor, (char *)&u, ctob(UPAGES))!=ctob(UPAGES) ||
	   !INUDOT(u.u_pcb.pcb_ksp) || !INSTACK(u.u_pcb.pcb_usp)) {
		datmap.e1 = MAXFILE;
		return;
	}
	signo = u.u_arg[0];
	sigcode = u.u_code;
	filhdr.a_text = ctob(u.u_tsize);
	filhdr.a_data = ctob(u.u_dsize);
	stksiz = ctob(u.u_ssize);
	switch (filhdr.a_magic) {

	case OMAGIC:
		datmap.b1 = 0;
		datmap.e1 = filhdr.a_text+filhdr.a_data;
		datmap.f2 = ctob(UPAGES) + datmap.e1;
		break;

	case NMAGIC:
	case ZMAGIC:
		datmap.b1 = round(filhdr.a_text, PAGSIZ);
		datmap.e1 = datmap.b1 + filhdr.a_data;
		datmap.f2 = ctob(UPAGES) + filhdr.a_data;
		break;
	}
	datbas = datmap.b1;
	datmap.f1 = ctob(UPAGES);
	datmap.b2 = MAXSTOR - stksiz;
	datmap.e2 = MAXSTOR;
#ifdef notdef	/* remove reference to u_exdata -- depp */
	if (filhdr.a_magic && u.u_exdata.ux_mag &&
	    filhdr.a_magic != u.u_exdata.ux_mag)
		printf("corefile not from this program");
#endif notdef
}

getpcb()
{

	lseek(fcor, masterpcbb&~0x80000000, 0);
	read(fcor, &pcb, sizeof (struct pcb));
	pcb.pcb_p0lr &= ~AST_CLR;
	printf("p0br %X p0lr %X p1br %X p1lr %X\n",
	    pcb.pcb_p0br, pcb.pcb_p0lr, pcb.pcb_p1br, pcb.pcb_p1lr);
}

struct cpudata cpu_data;
char string[80];
struct lock_t l_h;

print_cpudata(cpunum)
{
	int base;
	struct cpudata *pcpu;
	struct lock_t *l;

	/* read up the per-cpu structure for cpunum */
	base = cpudata_base + (cpunum * 4);
	base = vtophys(base);
	physrw(fcor,base,&pcpu,1);
	lseek(fcor, vtophys(pcpu),0);
	
	/* return if pointer is zero...cpu is not started */
	if (pcpu==0)return(0);
	read(fcor,&cpu_data,sizeof(struct cpudata));

	printf("cpu %X ",cpunum);
	printf("  proc = %X%s\n",cpu_data.cpu_proc,
		(cpu_data.cpu_noproc ? " (idle)" : " (running)"));
	
	printf("	stack  = %X \n",cpu_data.cpu_stack);
	printf("	state =");
	if (cpu_data.cpu_state & CPU_BOOT) printf(" boot ");
	if (cpu_data.cpu_state & CPU_RUN) printf(" run ");
	if (cpu_data.cpu_state & CPU_PANIC) printf(" panic ");
	if (cpu_data.cpu_state & CPU_STOP) printf(" stop ");
	printf("\n");

	if (cpu_data.cpu_panicstr) {
		lseek(fcor, vtophys(cpu_data.cpu_panicstr),0);
		read(fcor,string,80);
		printf("	panicstr = %s\n",string);
	}

	/* print locks held */
	printf ("	locks held: ");
	l = cpu_data.cpu_hlock;
	printf("%X ", l);
	while(l) {
		lseek(fcor, vtophys(l),0);
		read(fcor,&l_h,sizeof(struct lock_t));
		printf("%X ",l_h.l_plock);
		l = l_h.l_plock;
	}
	printf("\n");
	
	return(1);
}

create(f)
	char *f;
{
	register int fd;

	fd = creat(f, 0644);
	if (fd < 0)
		return (-1);
	close(fd);
	return (open(f, wtflag));
}

getfile(filnam, cnt)
	char *filnam;
{
	register int fsym;

	if (eqstr(filnam, "-"))
		return (-1);
	fsym = open(filnam, wtflag);
	if (fsym < 0 && xargc > cnt) {
		if (wtflag)
			fsym = create(filnam);
		if (fsym < 0)
			printf("cannot open `%s'\n", filnam);
	}
	return (fsym);
}

setvar()
{

	var[varchk('b')] = datbas;
	var[varchk('d')] = filhdr.a_data;
	var[varchk('e')] = filhdr.a_entry;
	var[varchk('m')] = filhdr.a_magic;
	var[varchk('s')] = stksiz;
	var[varchk('t')] = filhdr.a_text;
}
