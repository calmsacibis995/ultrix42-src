#ifndef lint
static char *sccsid = "@(#)xvmeinit.c	4.3	(ULTRIX)	12/20/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *
 * Abstract:
 *	This module contains the routines which are used to initialize
 *	the XBIA+ VME adapter.
 *
 * Revision History
 *
 *	14-Nov-89	map -- Mark A. Parenti
 *		Original Version
 *************************************************************************/
#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errlog.h"
#include "../../machine/common/cpuconf.h"
#include "../h/dk.h"
#include "../h/config.h"
#include "../h/kmalloc.h"
#include "../h/vmmac.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/nexus.h"
#include "../machine/scb.h"

#include "../io/xmi/xmireg.h"
#include "../io/xmi/xbireg.h"
#include "../io/vme/xviareg.h"
#include "../io/vme/xvibreg.h"
#include "../io/vme/vbareg.h"
#include "../io/vme/vbavar.h"
#include "../io/uba/ubavar.h"

extern	int		cpu;
extern	int		nvme_config;
extern	XVIBREGPTRS	xbia_xvib_regoff;
extern struct config_adpt config_adpt[];
extern	int	vmedebug;
extern	int	ins_vba();

/* Define debugging stuff.
 */
#define DEBUG
#ifdef DEBUG
#define Cprintf if(vmedebug)cprintf
#define Dprintf if( vmedebug >= 2 )cprintf
#else
#define Cprintf ;
#define Dprintf ;
#endif
xvmeconf(nxv, nxp, xminumber, xminode, xmidata)
register struct xbi_reg *nxv;
register struct xbi_reg *nxp;
register int xminumber;
register int xminode;
struct xmidata *xmidata;
{
	register struct	vba_hd	*vhp;
	register struct config_adpt *p_adpt;


	Cprintf("xbiaconf: Entered\n");
	Cprintf("xbiaconf: nxv = 0x%x\n", nxv);
	if(is_adapt_configured("vba", xminode) == 0 ) {
		xminotconf(nxv,nxp,xminumber,xminode,xmidata);
		return(0);
	}
	Cprintf("xbiaconf: After is_adapt_configured\n");
	KM_ALLOC(vhp, 
		 struct vba_hd *, 
		 sizeof(struct vba_hd),
		 KM_DEVBUF,
		 KM_NOW_CL_CA)
	if (vhp == (struct vba_hd *)NULL)
		return(0);
	(void)ins_vba(vhp);

	/* set vba alive in adpter struct */
	config_set_alive("vba", nvme_config, xminumber, xminode);

	vhp->vbavirt = (caddr_t)nxv;
	vhp->vbaphys = (caddr_t)nxp;
        vhp->vbanum = nvme_config;
        vhp->adapt_info.xbia_info.xmidata = xmidata;
	Cprintf("xbiaconf:xmidata = 0x%x\n", xmidata);
	xbia_setregs(nxv, vhp);
	vhp->n16dmapmr = 0x8000 >> XVME_DMAPMR_SHIFT;
	vhp->n24dmapmr = 0x800000 >> XVME_DMAPMR_SHIFT;
	vhp->n32dmapmr = XVME_NDMAPMR - 
		(0x800000 >> XVME_DMAPMR_SHIFT) -
			(0x8000 >> XVME_DMAPMR_SHIFT);
        vhp->nbyte_dmapmr = XVME_NBYTE_DMAPMR;
        vhp->npiopmr = XVIB_NPIOPMR;
	vhp->pio_base = (caddr_t)xmi_io_space(xminumber, xminode);

	printf("vba%d at xmi%d node %d ", vhp->vbanum, xminumber, xminode);
        printf("(XBIA/XVME)\n");
	Cprintf("xbiaconf: Before probevba\n");
	probevba(vhp);
	Cprintf("xbiaconf: After probevba\n");
        nvme_config++;
	return(1);
}

xbia_setregs(nxv, vhp)
caddr_t	nxv;
struct	vba_hd	*vhp;
{

	Cprintf("xbia_setregs: Entered\n");
	Xvibregs = xbia_xvib_regoff;
	vhp->nbyte_piopmr = XVME_NBYTE_PIOPMR;
	vhp->vba_type = VBA_XBIA;
	{
		register u_long	**regptr, **end;

		for( regptr = ( u_long ** )&Xvibregs,
		    end = ( u_long ** )( &Xvibregs + 1 );
		    regptr != end;
		    ++regptr ) {
			*regptr = ( u_long * )(( u_long )nxv + (u_long)*regptr );
		}
	}


}








