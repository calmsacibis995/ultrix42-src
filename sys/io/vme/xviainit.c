#ifndef lint
static char *sccsid = "@(#)xviainit.c	4.4	(ULTRIX)	12/20/90";
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
 *	the xVIA adapters.
 *
 * Revision History
 *
 *	18-Dec-90	map -- Mark A. Parenti
 *		Remove MVIA support.
 *		Fix lint problems.
 *
 *	12-Oct-90	map -- Mark A. Parenti
 *		Fix problem with initialization of interrupt routine ptr.
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

extern	int		cpu;
extern	int		nvme_config;
extern	XVIAREGPTRS	xvia_regoff;
extern	struct vba_hd	*get_vba();
extern	int		ins_vba();

int	vmedebug = 0;		/* Used to trigger debug printf's	*/

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
void		_3viaintr();

xviaconf(nxv, nxp, slot, vbanumber, intr)
caddr_t	nxv;
caddr_t	nxp;
int	slot;
int	vbanumber;
void	(**intr)();
{
	struct	vba_hd	*vhp;

	/* set vba alive in adapter struct */
	config_set_alive("vba", vbanumber);

	KM_ALLOC(vhp, 
		 struct vba_hd *, 
		 sizeof(struct vba_hd),
		 KM_DEVBUF,
		 KM_NOW_CL_CA)
	if (vhp == (struct vba_hd *)NULL)
		return(0);

	(void)ins_vba(vhp);
	vhp->vbavirt = nxv;
	vhp->vbaphys = nxp;
        vhp->vbanum = vbanumber;
	if(xvia_setregs(nxv, vhp) == 0)
		return(0);
	vhp->n16dmapmr = 0;
	vhp->n24dmapmr = 0x800000 >> XVIA_DMAPMR_SHIFT;
	vhp->n32dmapmr = XVIA_NDMAPMR;
        vhp->nbyte_dmapmr = XVIA_NBYTE_DMAPMR;
	vhp->nbyte_piopmr = XVIA_NBYTE_PIOPMR;
	vhp->pio_base = nxv;
        if(vhp->vba_type == VBA_3VIA) {
		*intr = _3viaintr;
		KM_ALLOC(vhp->intr_vec,
			 int (**)(), 
			 NVME_VECS * VME_VEC_SIZE, 
			 KM_DEVBUF,
			 KM_NOW_CL_CA)
		if(vhp->intr_vec == (int (**)())NULL)
			return(0);
	}
	printf("vba%d at slot %d ",vhp->vbanum, slot);
	printf("(3VIA/MVIB)\n");

	probevba(vhp);
	nvme_config++;
	return(1);
}

xvia_setregs(nxv, vhp)
caddr_t	nxv;
struct	vba_hd	*vhp;
{
	Xviaregs = xvia_regoff;
	switch( cpu ) {

	      case DS_5000:			/* 3MAX		*/
		vhp->vba_type = VBA_3VIA;
		vhp->npiopmr = _3VIA_NPIOPMR;
		break;

	      case DS_5000_100:
		vhp->vba_type = VBA_3VIA;
		vhp->npiopmr = _3VIA_PLUS_NPIOPMR;
		break;

	      default:
		printf("vba%d: VMEbus not supported on this machine\n",
		       vhp->vbanum);
		return(0);
	}

	{
		register u_long	**regptr, **end;

		for( regptr = ( u_long ** )&Xviaregs,
		    end = ( u_long ** )( &Xviaregs + 1 );
		    regptr != end;
		    ++regptr ) {
			*regptr = ( u_long * )(( u_long )nxv +  (u_long)*regptr );
		}
	}
	return(1);

}


/*
 *	Interrupt handler for the 3VIA.  This routine will receive all
 *	interrupts from the 3VIA/VME and is responsible for dispatching
 *	any VME-sourced interrupts to the appropriate handler.
 */
void	_3viaintr(vban)
int	vban;
{
	struct	vba_hd	*vhp;
	u_short	vector;

	vhp = get_vba(vban);
	vector = ((u_short)*Xviaregs.ivs & IVS_VEC_MASK); 
	(*(vhp->intr_vec[vector]))();
}





