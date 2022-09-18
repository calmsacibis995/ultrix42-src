#ifndef lint
static char *sccsid = "@(#)kdm.c	4.2	(ULTRIX) 8/3/90";
#endif lint

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ------------------------------------------------------------------------
 * Modification History:
 *
 *	03-Aug-1990	rafiey (Ali Rafieymehr)
 *		Changes for Stuart Hollander for multiple XMIs support.
 *
 *	20-Jul-1989	map (Mark Parenti)
 *		First version. Derived from bda.c.
 *
 *
 * ------------------------------------------------------------------------
 */

#include "../h/types.h"
#include "../h/buf.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/kmalloc.h"

#include "../machine/scb.h"
#include "../machine/pte.h"
#include "../io/uba/ubavar.h"
#include "../h/config.h"

#include "../io/xmi/xmireg.h"
#include "../io/xmi/sspxmi.h"

extern int numuba;
extern int numxmi;
extern int nbicpus;
extern int nbitypes;
extern int nNUBA;

/*
 *
 * This routine set up the bda and then looks for its drivers.
 * First the interrupt vectors are set then the device is initialized
 *
 * Next the drivers for attached if they can be found
 *
 */
#define PHYS(addr)	((long) \
		((long)ptob(Sysmap[btop((long)(addr)&0x7fffffff)].pg_pfnum)  \
			| (long)( PGOFSET & (long)(addr) ) ) )

extern struct uba_driver uqdriver;
extern struct uba_ctlr *xmifindum(); 

kdminit(nxv,nxp,xminumber,xminode,xmidata)
struct kdm_regs *nxv;
caddr_t	*nxp;
register int xminumber;
register int xminode;
register struct xmidata *xmidata;
{
	register char *puba;
	register char *vuba;
	register int savenumuba;
	register struct uba_ctlr *um;
	struct uba_driver *udp;
	int (**xmiprobe)();

	udp = &uqdriver;
	xmiprobe = &udp->ud_probe;
	if ((um = xmifindum(xminumber,xminode,xmiprobe,"kdm")) == 0)
		if ((um = xmifindum(xminumber,'?',xmiprobe,"kdm"))==0) 
			if ((um = xmifindum('?','?',xmiprobe,"kdm"))==0) {
				xminotconf(nxv,nxp,xminumber,xminode,xmidata);
				return;
			}		

	um->um_addr = (caddr_t) 0x40;
	savenumuba = numuba;
	numuba = um->um_ubanum;

	KM_ALLOC(vuba,char *,2048,KM_DEVBUF,KM_NOW_CL_CO_CA)
	if (vuba == 0) {
		numuba = savenumuba;
		printf("km_alloc returned 0 size \n");
		return;
	}

#ifdef vax
	puba = (char *) PHYS(vuba);
	puba = (char *) (((((long)puba)) & 0x7ffffe00)- 2048);
	vuba = (char *) ((((((long)vuba)) & 
				0x7ffffe00)- 2048) | 0x80000000);
#endif vax
#ifdef mips
	puba = (char *) PHYS(vuba);
	puba = (char *) (((((long)puba)) & 0x3ffffe00)- 2048);
	vuba = (char *) ((((((long)vuba)) & 
				0x3ffffe00)- 2048) | 0xc0000000);
#endif mips

	uba_hd[numuba].uh_vec = SCB_XMI_ADDR(xmidata);
	uba_hd[numuba].uh_lastiv= SCB_XMI_LWOFFSET(xminode,LEVEL15)+4;
	
/*	nxv->bda_biic.biic_int_ctrl = SCB_BI_LWOFFSET(xminode,LEVEL15); */

	uba_hd[numuba].uba_type = UBAXMI;
	numxmi=xminumber;
	unifind(vuba,puba, nxv, nxp, 0, nxp, nxv, 0,xminumber,xminode);

	numuba=savenumuba;
/*
	if (um->um_alive == 0) xmi_init_failed(nxv,nxp,cpup,xminumber,xminode);
*/
}

