#ifndef lint
static	char	*sccsid = "@(#)stacktrace.c	4.1	(ULTRIX)	7/17/90";
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

 /* --------------------------------------------------- */
 /* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
 /* | All Rights Reserved.                            | */
 /* --------------------------------------------------- */
 /* $Header: stacktrace.c,v 1031.4 88/05/17 09:18:57 bettina Exp $ */
#include <stdio.h>
/* #include "signal.h" */
/* #include "setjmp.h" */
#include <a.out.h>
#include <ldfcn.h>
#include <sys/ptrace.h> 
#include <sys/file.h>
#include <sex.h>

static LDFILE *ldptr;
static char *myname;

static agetword(addr)
int * addr;
{
    return *addr;
}

#define myprintf printf
int myfd;
static char *regnames[] = {
		"r0/zero",	"r1/at",	"r2/v0",	"r3/v1",
		"r4/a0",	"r5/a1",	"r6/a2",	"r7/a3",
		"r8/t0",	"r9/t1",	"r10/t2",	"r11/t3",
		"r12/t4",	"r13/t5",	"r14/t6",	"r15/t7",
		"r16/s0",	"r17/s1",	"r18/s2",	"r19/s3",
		"r20/s4",	"r21/s5",	"r22/s6",	"r23/s7",
		"r24/t8",	"r25/t9",	"r26/k0",	"r27/k1",
		"r28/gp",	"r29/sp",	"r30/s8",	"r31/ra",
};

stacktrace (filename, startpc, startsp, regs, getword, locals)
char * filename;
int *regs;
int (*getword)();
int locals;	/* 0= no local variables, 1=local variables */
{
    unsigned sp,pc,fp,istack,value;
    int isym,framesym,ifd,ireg,nreg;
    char *pname;
    SYMR asym;
    PDR apd;
    pPDR ppd = &apd;
    AUXU aux,auxpc,*ldgetaux();
    int hostsex = gethostsex();
    int i,j;

/*    myprintf("Registers on entry:\n");
    for (i = 0; i < 32; i += 3) {
	    for (j = i; j < 32 && j < i+3; j++)
		    myprintf("%s: 0x%08x\t", regnames[j], regs[j]);
	    myprintf("\n");
    }
*/
    auxpc.ti.bt = btUChar;
    auxpc.ti.tq0 = tqPtr;

    ldptr = ldopen(filename, NULL);
    if (ldptr == NULL) {
	myprintf("cannot read in %s\n", filename);
	return (-1);
    } /* if */

    myprintf("Stack trace -- last called first\n");
    for (pc = startpc, sp = startsp, fp = 1, istack = 0; sp != 0 && fp != 0 && pc != 0; istack++) {
	ldgetpd(ldptr, ipd_adr(pc), ppd);
	isym = framesym = ppd->isym;
	if (isym != isymNil) {
	   if (ldtbread(ldptr,isym,&asym) == FAILURE) {
		myprintf("cannot read %d symbol\n", ppd->isym);
		goto next;
	    } /* if */
	    pname = (char *)ldgetname(ldptr, &asym);
	    if (ppd->framereg != GPR_BASE+29) {
		myprintf("%s has a non-sp framereg (%d)\n", pname,
		    ppd->framereg);
		return (-1);
	    } /* if */

	    ifd = ld_ifd_symnum(ldptr, ppd->isym);
	    fp = sp + ppd->frameoffset;
	    myprintf("%4d %s (", istack, pname);
	} else {
	    myprintf("%4d <name stripped> (", istack, pname);
	} /* if */

    if (isym == isymNil || asym.index == indexNil) {
	myprintf ("0x%x, 0x%x, 0x%x, 0x%x", (*getword)(fp), (*getword)(fp+4), 
	    (*getword)(fp+8), (*getword)(fp+12));
    } else {
	do { /* arguments */
	    if (ldtbread(ldptr, ++isym, &asym) == FAILURE)
		break;
	    if (asym.st == stBlock || asym.st == stEnd || asym.st == stProc
		|| asym.st == stStaticProc)
		break;
	    if (asym.st != stParam)
		continue;
	    if (isym != ppd->isym+1)
		myprintf(", ");
	    myprintf("%s = ", ldgetname(ldptr, &asym));
	    if (asym.sc == scAbs)
		myprintf("%x", value=(*getword)(fp+asym.value));
	    else if (asym.sc == scRegister && asym.value < 32)
		myprintf("%x", value=regs[asym.value]);
	    else if (asym.sc == scVarRegister && asym.value < 32)
		myprintf("(*%x) %d", regs[asym.value], value=(*getword)(regs[asym.value]));
	    else if (asym.sc == scVar) {
		value = (*getword)(fp+asym.value);
		myprintf("(*%x)", value);
		value = (*getword)(value);
		myprintf(" %x", value);
	    } /* if */
	    if (asym.index != indexNil) {
		aux = *ldgetaux(ldptr, asym.index);
		if (PFD(ldptr)[ifd].fBigendian != (hostsex == BIGENDIAN))
		    swap_aux(&aux, ST_AUX_TIR, hostsex);
		if (aux.isym == auxpc.isym) {
		    int j, buf[11];
		    for (j = 0; j < 10; j++)
			buf[j] = (*getword)(value + (j*4));
		    buf[11] = 0;
		    myprintf (" \"%s\"", buf);
		} /* if */
	    } /* if */
	} while (isym < PFD(ldptr)[ifd].csym + PFD(ldptr)[ifd].isymBase);
    } /* if */

    if (isym == isymNil) {
	myprintf(")");
    } else {
	myprintf (") [%s: ,%d 0x%x]\n", st_str_ifd_iss(ifd, 1),
	    SYMTAB(ldptr)->pline[ppd->iline+((pc-ppd->adr)/4)], 
	    pc);
    } /* if */

	if (locals) {
		int unsigned i;
		printf("Raw frame:\n");
		for (i=0; i<ppd->frameoffset; i++) {
			myprintf ("0x%08x, ", (*getword)(fp+i*4));
			if (i%4 == 3)
				printf("\n");
		}
		printf("\n");
	}

    /* set up for next time */
next:
	sp = fp;
	/* restore regs */
	for (nreg = 0, ireg = 31; ireg >=0; ireg--) {
	    if (((1<<ireg)&ppd->regmask) != 0) {
		regs[ireg] = (*getword)(fp+ppd->regoffset-((nreg++)*4));
	    } /* if */
	} /* for */
	if (ppd->pcreg == 0)
	    pc = (*getword)(fp-4);
	else
	    pc = regs[31];
	if (pc < 0x80000000)
		return;
    } /* for */
} /* stacktrace */


ipd_adr (adr)
unsigned adr;

{
    int		ilow;
    int		ihigh;
    int		ihalf;
    int		ilowold;
    int		ihighold;
    int		ipd;
    PDR		apd;

    ilow = 0;
    ihigh = SYMHEADER(ldptr).ipdMax;
    /* binary search proc table */
    while (ilow < ihigh) {
	ihalf = (ilow + ihigh) / 2;
	ilowold = ilow;
	ihighold = ihigh;
	ldgetpd(ldptr, ihalf, &apd);
	if (adr < apd.adr)
	    ihigh = ihalf;
	else if (adr > apd.adr)
	    ilow = ihalf;
	else {
	    ilow = ihigh = ihalf;
	    break;
	} /* if */
	if (ilow == ilowold && ihigh == ihighold)
	    break;
    } /* while */

    ipd =  ((ilow < ihigh || ihigh < 0) ? ilow : ihigh);

    return (ipd);

} /* ipd_adr */


#ifdef notdef
void static
handler(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
	static int inhandler;
	jmp_buf	handlerbuf;

	if (inhandler) {
	    printf("recursive signal will exit internal stacktrace\n");
	    longjmp(handlerbuf);
	} else {
	    inhandler = 1;
	} /* if */

	switch(sig){
	case SIGSEGV:
	    myprintf("SIGSEGV signal caught\n");
	    break;
	case SIGBUS:
	    myprintf("SIGBUS signal caught\n");
	    break;
	case SIGILL:
	    myprintf("SIGILL signal caught\n");
	    break;
	case SIGQUIT:
	    myprintf("SIGQUIT signal caught\n");
	    break;
	default:
	    myprintf("Unknown signal caught: signo = %d\n",sig);
	    break;
	}

	if (setjmp(handlerbuf))
	    return;
	dumpscp(scp);
	stacktrace(myname, scp->sc_pc, scp->sc_regs[GPR_BASE+29], scp->sc_regs,
	    agetword);
	inhandler = 0;
	exit(0);
}

dumpscp(scp)
register struct sigcontext *scp;
{
	register int i, j;

	myprintf("sigcontext\n");
	myprintf("PC: 0x%08x, CAUSE: 0x%08x, BADVADDR: 0x%08x\n",
	    scp->sc_pc, scp->sc_cause, scp->sc_badvaddr);

	myprintf("OWNEDFP: %d, FP_CSR: 0x%08x\n", scp->sc_ownedfp,
	    scp->sc_fpc_csr);

	if (scp->sc_ownedfp == 0)
	    return;
	myprintf("fp regs\n");
	for (i = 0; i < 32; i += 4) {
		for (j = i; j < i+4; j++)
			myprintf("%2d: 0x%08x\t", j, scp->sc_fpregs[j]);
		myprintf("\n");
	}
}

static int myfd;

ugmyprintf(format, a, b, c, d)
char *format;
{
    char buf[512];

    sprintf (buf, format, a, b, c, d);
    write(myfd, buf, strlen(buf));
}
#endif notdef
initstacktrace(argv)
char **argv;
{
    myfd = open("/dev/tty", O_RDWR);
    if (myfd < 0)
	perror("initstacktrace cannot open /dev/tty");
    myname = argv[0];
/*    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);
    signal(SIGILL, handler);
    signal(SIGQUIT, handler); */
}
