#ifndef lint
static char *sccsid = "@(#)bla.c	4.1	(ULTRIX)	7/2/90";
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
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 *
 * ------------------------------------------------------------------------
 */
#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../../machine/common/cpuconf.h"
#include "../h/config.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#endif vax
#include "../machine/clock.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"

extern int nNUBA;
extern int numuba;
extern struct bidata bidata[];
extern struct bisw bisw[];
extern struct uba_driver tmscpdriver;
extern struct uba_ctlr *bifindum(); 
extern char kbmem[][512*NBPG];
extern	struct pte KBMEMmap[][512];
extern struct uba_driver uqdriver;
extern struct cpusw *cpup;	/* pointer to cpusw entry */
int	klesib = 0;


blainit(bua,physbua,binumber,binode,j)
register struct bua_regs *bua;
register char *physbua;
int binumber;
int binode;
{
	int i;
	register int savenumuba;
	register struct uba_ctlr *um;
	struct uba_driver *udp;
	int (**biprobe)();

	udp = &uqdriver;
	biprobe = &udp->ud_probe;
        if ((um = bifindum(binumber,binode,biprobe,"klesib")) == 0)
                if ((um = bifindum(binumber,'?',biprobe,"klesib"))==0)
                        if ((um = bifindum('?','?',biprobe, "klesib"))==0) {
				binotconf(bua,physbua,binumber,binode);
				return;
			}		

	um->um_addr = (caddr_t) 0174500;
	savenumuba = numuba;
	numuba = um->um_ubanum;

	/* set offset in SCB */
	bua->bua_vec = SCB_BI_OFFSET(binumber);
	uba_hd[numuba].uh_vec = SCB_BI_ADDR(binumber);

	/* last interrupt vector is set to 4 past true vector
	   I want for BLA to FAKE unifind to do right thing */

	uba_hd[numuba].uh_lastiv=SCB_BI_LWOFFSET(binode,LEVEL15)+4;
	uba_hd[numuba].uba_type = UBABUA|UBABLA; /* looks alot like BUA */

	unifind(bua,physbua,kbmem[klesib],cpup->umaddr(binumber,binode),
		cpup->pc_umsize,cpup->udevaddr(binumber,binode),
		KBMEMmap[klesib],cpup->pc_haveubasr,binumber,binode);
	
	bua->bua_ctrl |= BUACR_BUAEIE;
	numuba = savenumuba;
	klesib++;
	if (um->um_alive == 0) bi_init_failed(bua,physbua,binumber,binode);

}

