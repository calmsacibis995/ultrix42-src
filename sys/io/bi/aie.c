#ifndef lint
static char *sccsid = "@(#)aie.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ------------------------------------------------------------------------
 * Modification History:
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added new kmalloc memory allocation to system.
 *
 * 03-Oct-87 -- lp
 *	Bump aie's interrupt to IPL15 to block against other ethernet
 *	controllers (namely the deuna/delua).
 *
 * 28-Jan-87 -- lp
 *	Cleaned up unused variables pointed out by LINT.
 *
 * 14-Jan-87 -- lp
 *	Added rev level check to insure we are at 0x100+ firmware.
 *
 * 16-Dec-86   -- lp
 *	Dont init unused command buffers to -1 (change to 0)
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 5 May 86 -- lp
 *	Some cleanup. NI now uses LEVEL 14.
 *
 * ------------------------------------------------------------------------
 *
 * DEC AIE interface. Port specific initialization routines here.
 *     By
 * Larry Palmer (decvax!lp)
 *
 */
#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vmmac.h"
#include "../h/kmalloc.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#endif vax
#include "../machine/clock.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/scs/scamachmac.h"
#include "../io/bi/bireg.h"
#include "../io/bi/nireg.h"
#include "../io/uba/ubavar.h"

extern int sysptsize;
extern int nNI;
extern struct ni niinfo[];
extern struct bidata bidata[];

/* 
 * Heres the rev level check 
 */

#define NI_MINREV 0x100

bvpniprobe(nxv, nxp, binumber, binode, ui)
struct uba_device *ui;
{
	if(ui->ui_unit > nNI)
		return(0);
	return(1);
}

bvpniattach(ui)
struct uba_device *ui;
{
	/* niinfo[] is what we talk to in the driver.
	 * ui->ui_unit will be unit passed in from interrupt jump.
	 */
	register struct ni *pni;
	register struct _fqb *p_fqb;
	register struct _bd *p_bd;
	char *nxv = ui->ui_addr;
	int number = ui->ui_adpt;
	int slot = ui->ui_nexus;
	
	/*
	 * Initialize Port queue block & pqb related structures
	 */

	pni = &niinfo[ui->ui_unit];
	/* Save port registers */
	pni->ni_regs = (struct nidevice *) (nxv + NI_NI_ADDR);
	pni->unit = ui->ui_unit;
	pni->alive++;
	pni->ui = (char *)ui;
	KM_ALLOC(pni->ni_pqb, GVPPQB * ,512,KM_DEVBUF,KM_NOW_CL_CA)
	pni->phys_pqb = svtophy(pni->ni_pqb);

	pni->ni_pqb->ni.piv.level = 1;

	pni->ni_pqb->ni.piv.vector = SCB_BI_VEC_ADDR(number,slot,LEVEL15)
		- &scb.scb_stray;

/* &scb.scb_ipl14[slot] - &scb.scb_stray; */ 
	pni->ni_pqb->ni.piv.bi_node = bidata[number].biintr_dst;
	pni->ni_pqb->ni.vpqb_base = (caddr_t) pni->ni_pqb;

#ifdef vax
	pni->ni_pqb->ni.spt_base = (unsigned long) mfpr(SBR);
	pni->ni_pqb->ni.spt_len = (long) mfpr(SLR);


	/* Global page table is a nop */
	pni->ni_pqb->ni.gpt_base = (unsigned long) mfpr(SBR);
	pni->ni_pqb->ni.gpt_len = (long) mfpr(SLR);
#endif vax
#ifdef mips
	pni->ni_pqb->ni.spt_base = (unsigned long) Vaxmap;
	pni->ni_pqb->ni.spt_len = (long) ((sysptsize * 8)/4);

	/* Global page table is a nop */
	pni->ni_pqb->ni.gpt_base = (unsigned long) Vaxmap;
	pni->ni_pqb->ni.gpt_len = (long) ((sysptsize * 8)/4);
#endif mips

	pni->ni_pqb->ni.func_mask = 0;
	pni->ni_pqb->ni.bvp_level = 1; 

	KM_ALLOC(pni->ni_pqb->ni.vfqb_base,struct _fqb *,512,KM_DEVBUF,KM_NOW_CL_CA)
	/* Setup buffer descriptor table */
	{
	register unsigned long size;

	size = (NI_NBUF* (NI_NFREEQ-1) * sizeof(struct _bd));
	KM_ALLOC(pni->ni_pqb->ni.bdt_base,caddr_t,size,KM_DEVBUF,KM_NOW_CL_CA)
	pni->ni_pqb->ni.bdt_len = NI_NBUF * (NI_NFREEQ-1);
	}

	/* Set up free queue block */
	pni->ni_pqb->ni.num_freeq = NI_NFREEQ;
	p_fqb = (struct _fqb *)pni->ni_pqb->ni.vfqb_base;
	p_fqb->mfreeq_size = NI_MQSIZE;
	p_fqb->dfreeq_size = NI_DQSIZE;
	p_fqb->rfreeq_size = NI_DQHEAD+34;
	p_fqb->mfreeq.flink = p_fqb->mfreeq.blink = 0;
	p_fqb->dfreeq.flink = p_fqb->dfreeq.blink = 0;
	p_fqb->rfreeq.flink = p_fqb->rfreeq.blink = 0;

	/* Init the queues */
	pni->ni_pqb->cmdq0.flink = pni->ni_pqb->cmdq0.blink = 0;
	pni->ni_pqb->rspq.flink = pni->ni_pqb->rspq.blink = 0;
	/* Only use cmdq0 & respq */
	pni->ni_pqb->cmdq1.flink = pni->ni_pqb->cmdq1.blink = 0;
	pni->ni_pqb->cmdq2.flink = pni->ni_pqb->cmdq2.blink = 0;
	pni->ni_pqb->cmdq3.flink = pni->ni_pqb->cmdq3.blink = 0;

	/* Hook the interrupt vector to scb */
	bidev_vec(number, slot, LEVEL15, ui);

	niattach(pni);
	
	if(((pni->ni_pqb->ni.ad_hw_vers[0] >> 16) & 0xffff) < NI_MINREV)
		printf("Warning aie revision level %x out of date (MUST be %x or greater)\n", ((pni->ni_pqb->ni.ad_hw_vers[0] >> 16) & 0xffff), NI_MINREV);

}
