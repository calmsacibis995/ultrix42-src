#ifndef lint
static char *sccsid = "@(#)ka730.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86 by			*
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

/***********************************************************************
 *
 * Modification History:
 *
 * 27-Nov-89    Paul Grist
 *      added frame_type argument to logmck() call.
 *
 * 15-Mar-86	Darrell Dunnuck
 *	Moved ka730 specific parts of configure() here into ka730conf.
 *
 * 05-Mar-86 -- pmk
 *	added arg recover to logmck and replaced display with cprintf
 *
 * 19-Feb-86 -- pmk
 *	added check to memerr and memenable for controller
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 12-Feb-86	Darrell Dunnuck
 *	Removed the routines memerr, memenable, setcache, and tocons
 *	from machdep.c and put them here for this processor type.
 *	Added a new routine cachenbl.
 *
 * 12-Dec-85 	Darrell Dunnuck
 *	Created this file to as part of machdep restructuring.
 *
 **********************************************************************/

#include "../h/types.h"
#include "../h/time.h"
#include "../h/param.h"
#include "../machine/cons.h"
#include "../h/errlog.h"
#include "../machine/mtpr.h"
#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/pte.h"
#include "../machine/nexus.h"
#include "../io/uba/ubareg.h"
 
#define NMC730	12
char   *mc730[] = {
	"microcode lost",			/* 0 */
	"translation buffer parity error",	/* 1 */
	0,					/* 2 */
	"impossible value in memory csr",	/* 3 */
	"fast interrupt without support",	/* 4 */
	"fpa parity error",			/* 5 */
	"error on spte reference",		/* 6 */
	"uncorrectable ecc error",		/* 7 */
	"non-existent memory",			/* 8 */
	"unaligned or non-longword reference to i/o space",/* 9 */
	"illegal i/o space address",		/* A */
	"unaligned or non-longword unibus reference"/* B */
};

struct mc730frame {
	int	mc3_bcnt;			/* byte count == 0xc */
	int	mc3_summary;			/* summary parameter */
	int	mc3_parm[2];			/* parameter 1 and 2 */
	int	mc3_pc; 			/* trapped pc */
	int	mc3_psl;			/* trapped psl */
};


ka730machcheck (cmcf)
caddr_t cmcf;
{
	register u_int type = ((struct mcframe	 *) cmcf) -> mc_summary;
	register struct mc730frame *mcf = (struct mc730frame   *) cmcf;
	int recover = 0;
	int cpunum = 0;
	unsigned t;

	logmck((int *)cmcf, ELMCKT_730, cpunum, recover);
	if (recover == 0) {
	    cprintf("\nmachine check %x: ", type);
	    switch (type) {
		case 0:
		case 1:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 0xa:
		case 0xb:
			cprintf("%s\n", mc730[type]);
			break;
		default:
			cprintf("%s\n", "Unknown machine check type code");
			break;
	    }

	/* Display the machine check stack */

	    cprintf("\tsumpar\t= %x\n", mcf -> mc3_summary);
	    cprintf("\tparam1\t= %x\n", mcf -> mc3_parm[0]);
	    cprintf("\tparam2\t= %x\n", mcf -> mc3_parm[1]);
	    cprintf("\tpc\t= %x\n", mcf -> mc3_pc);
	    cprintf("\tpsl\t= %x\n\n", mcf -> mc3_psl);
	    cprintf("\tmcesr\t= %x\n", mfpr (MCESR));
	}
	mtpr (MCESR, 0xf);
	memerr ();
	panic ("mchk");
	return(0);
}

/*
 *  Function:
 *	ka730memerr()
 *
 *  Description:
 *	log memory errors in kernel buffer
 *
 *  Arguments:
 *	none
 *
 *  Return value:
 *	none
 *
 *  Side effects:
 *	none
 */

ka730memerr()
{
	register struct mcr *mcr;
	register int m;
	struct mcr amcr;
	struct el_rec *elrp;
	struct el_mem *mrp;

	for (m = 0; m < nmcr; m++) {
	    mcr = mcrdata[m].mcraddr;
	    /* csr0 clears and some of csr1 clears when read, so save them */
	    amcr.mc_reg[0] = mcr->mc_reg[0];
	    amcr.mc_reg[1] = mcr->mc_reg[1];
	    amcr.mc_reg[2] = mcr->mc_reg[2];
	    if (M730_ERR (&amcr)) {
	        elrp = ealloc(EL_MEMSIZE,EL_PRILOW);
	        if (elrp != NULL) {
		    LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_730,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		    mrp = &elrp->el_body.elmem;
		    mrp->elmem_cnt = 1;
		    mrp->elmemerr.cntl = 1;
		    mrp->elmemerr.type = M730_HRDERR(&amcr) ? 2 : 1;
		    mrp->elmemerr.numerr = 1;
		    mrp->elmemerr.regs[0] = amcr.mc_reg[0];
		    mrp->elmemerr.regs[1] = amcr.mc_reg[1];
		    mrp->elmemerr.regs[2] = amcr.mc_reg[2];
		    mrp->elmemerr.regs[3] = EL_UNDEF;
		    EVALID(elrp);
	        }
	        M730_INH(mcr);
	    }
	}
	return(0);
}

/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka730memenable ()
{
	register struct mcr *mcr;
	register int m;

	for (m = 0; m < nmcr; m++) {
	    mcr = mcrdata[m].mcraddr;
	    M730_ENA (mcr);
	}
	return(0);
}

ka730tocons(c)
	register int c;
{
	while ((mfpr (TXCS) & TXCS_RDY) == 0)
		continue;
	mtpr (TXDB, c);
	return(0);
}

ka730conf(cpup)
struct cpusw *cpup;
{
	union cpusid cpusid;

	cpusid.cpusid = mfpr(SID);
	printf("VAX 11/730, microcode level = %d\n",
	cpusid.cpu730.cp_urev);
	probesbi(cpup);
	return(0);
}

short *ka730nexaddr(ioadpt,nexnum) 
 	int ioadpt,nexnum;
{

	return(NEX730(nexnum));

}

u_short *ka730umaddr(ioadpt,nexnum) 
 	int ioadpt,nexnum;
{

	return(UMEM730);

}

u_short *ka730udevaddr(ioadpt,nexnum) 
 	int nexnum,ioadpt;
{

	return(UDEVADDR730);

}
