#ifndef lint
static char *sccsid = "@(#)panic.c	4.1    ULTRIX  7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983,86,87 by			*
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

/*
 * Modification History
 *
 *	13-Oct-89 gmm
 *		moved panic() to common code sys/subr_prf.c. Only vax specific
 *		panic logging done here.
 *
 *	07-Mar-89 gmm
 *		added declaration for ttykdb
 *
 *	09-Feb-89 Randall Brown
 *		Created file.  Separated file from subr_prf.c
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/seg.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/reboot.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/types.h"
#include "../h/errlog.h"
#include "../h/kmalloc.h"
#include "../h/cpudata.h"
#include "../machine/cpu.h"
#include "../io/uba/qdioctl.h"

#include "../machine/mtpr.h"



panic_log(s,cpunum,reg11)
	char *s;
	int cpunum;
	int *reg11;
{
	int i;
	int num1 = 0;
	int num2 = 0;
	char *strp;
	int *regp;
	struct el_rec *elrp;
	struct el_pnc *elpp;
	int fp, ap1, ap2;

	cprintf("\n\n\n***************************\n");
	cprintf("cpu %d register dump \n", cpunum);

	elrp = ealloc(EL_PNCSIZE, EL_PRISEVERE);
	if (elrp != NULL) {
	    LSUBID(elrp,ELSW_PNC,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	    elpp = &elrp->el_body.elpnc;
	    regp = &elpp->pncregs.pnc_ksp;

	    strp = s;
	    for (i = 1; *strp++ != '\0' && i < EL_SIZE64; i++) ;
	    bcopy(s, elpp->pnc_asc, i);
	    elpp->pnc_asc[i-1] = '\0';

	    elpp->pnc_sp = (int)reg11++;
	    elpp->pnc_ap = *++reg11;
	    elpp->pnc_fp = *++reg11;
	    elpp->pnc_pc = *++reg11;

	    for (i = 0; i < 32; i++)
	        if ((EL_REGMASK & (1 << i)) != 0)
		    *regp++ = mfpr(i);

/* Verify ksp is valid, using -
   kernacc(addr, bcnt, rw) caddr_t addr; unsigned bcnt; { return (0); } */

	    if (kernacc((caddr_t)elpp->pncregs.pnc_ksp, 4, B_READ)) {
	        num1 = 0x80000000 - elpp->pncregs.pnc_ksp;
	        if (num1 > EL_STKDUMP || num1 < 0)
	            num1 = EL_STKDUMP;
	        if (kernacc((caddr_t)(elpp->pncregs.pnc_ksp+(num1-4)),4,B_READ)) {
	            bcopy((char *)elpp->pncregs.pnc_ksp,
		          (char *)elpp->kernstk.stack, num1);
		}
		else num1 = 0;
	    }
	    elpp->kernstk.addr = elpp->pncregs.pnc_ksp;
	    elpp->kernstk.size = num1;
	    if (num1 < EL_STKDUMP) 
	        bzero((char *)elpp->kernstk.stack + num1,EL_STKDUMP - num1);
	    
	    num2 = ((elpp->pncregs.pnc_isp + 511) & (~0x1ff)) - 
		     elpp->pncregs.pnc_isp;
	    if (num2 > EL_STKDUMP || num2 < 0)
	        num2 = EL_STKDUMP;
	    bcopy((char *)elpp->pncregs.pnc_isp,(char *)elpp->intstk.stack,num2);
	    elpp->intstk.addr = elpp->pncregs.pnc_isp;
	    elpp->intstk.size = num2;
	    if (num2 < EL_STKDUMP) 
	        bzero((char *)elpp->intstk.stack + num2,EL_STKDUMP - num2);
	    
	    EVALID(elrp);

	    cprintf("sp\t= %8x\tap\t= %8x\tfp\t= %8x\n",
		    elpp->pnc_sp,elpp->pnc_ap,elpp->pnc_fp);
	    DELAY(400000);
	    cprintf("pc\t= %8x\tksp\t= %8x\tusp\t= %8x\n",
		    elpp->pnc_pc,elpp->pncregs.pnc_ksp,elpp->pncregs.pnc_usp);
	    DELAY(400000);
	    cprintf("isp\t= %8x\tp0pr\t= %8x\tp0lr\t= %8x\n",
		elpp->pncregs.pnc_isp,elpp->pncregs.pnc_p0br,elpp->pncregs.pnc_p0lr);
	    DELAY(400000);
	    cprintf("p1br\t= %8x\tp1lr\t= %8x\tsbr\t= %8x\n",
		elpp->pncregs.pnc_p1br,elpp->pncregs.pnc_p1lr,elpp->pncregs.pnc_sbr);
	    DELAY(400000);
	    cprintf("slr\t= %8x\tpcbb\t= %8x\tscbb\t= %8x\n",
		elpp->pncregs.pnc_slr,elpp->pncregs.pnc_pcbb,elpp->pncregs.pnc_scbb);
	    DELAY(400000);
	    cprintf("ipl\t= %8x\tastlvl\t= %8x\tsisr\t= %8x\n",
		elpp->pncregs.pnc_ipl,elpp->pncregs.pnc_astlvl,elpp->pncregs.pnc_sisr);
	    DELAY(400000);
	    cprintf("iccs\t= %8x\n\n",elpp->pncregs.pnc_iccs);

	    fp = elpp->pnc_sp;
	    ap1 = ap2 = 0;
	    cprintf("interrupt stack:\n");
	    prtstk(&fp,&ap1,&ap2,&elpp->pncregs.pnc_isp,&elpp->intstk);
	    cprintf("\nkernel stack:\n");
	    prtstk(&fp,&ap1,&ap2,&elpp->pncregs.pnc_ksp,&elpp->kernstk);
	    cprintf("\n");
	}

}

/*
 * Format/print stack dumps with call frame lables
 * "*" beginning of call frame, "#" beginning of arg frame
 */
prtstk(fp,ap1,ap2,sp,stkp)
int *fp;
int *ap1;
int *ap2;
int *sp;
struct el_stkdmp *stkp;
{
	register int i, j, num;
	int next = 0;
	int regmsk = 0;

	cprintf("%08x: ",*sp);
	num = stkp->size >> 2;
	for (i = 0; i < num; i++) {
	    cprintf("%08x",stkp->stack[i]);
	    if (next) {
		if (regmsk) {
		    for (j = 0; j < 12; j++) {
		        if (regmsk & (1 << j)) {
			    cprintf(" r%d",j);
			    regmsk &= ~(1 << j);
			    break;
			}
		    }
		}
		else
		    next = 0;
	    }
	    if (*fp == *sp + (i * 4))
		cprintf(" *");
	    if ((*fp)+4 == *sp + (i * 4))
		regmsk = (stkp->stack[i] & 0x0fff0000) >> 16;
	    if ((*fp)+8 == *sp + (i * 4)) {
		cprintf(" ap");
		*ap1 = *ap2;
		*ap2 = stkp->stack[i];
	    }
	    if ((*fp)+12 == *sp + (i * 4))
		cprintf(" fp");
	    if ((*fp)+16 == *sp + (i * 4)) {
		cprintf(" pc");
		*fp = stkp->stack[i-1];
		next = 1;
	    }
	    if (*ap1 == *sp + (i * 4)) {
		cprintf(" #");
	    }
	    cprintf("\t");
	    if (((i+1) % 4) == 0) {
		if ((i+1) == num)
		    cprintf("\n");
		else
		    cprintf("\n%08x: ",*sp + ((i+1) * 4));
	        DELAY(250000);
	    }
	}
}
