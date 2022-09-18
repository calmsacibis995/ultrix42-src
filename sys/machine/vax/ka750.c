
#ifndef lint
static char *sccsid = "@(#)ka750.c	4.1	ULTRIX	7/2/90";
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
 *	Moved ka750 specific parts of configure() here into ka750conf.
 *
 * 05-Mar-86 --pmk
 *	added arg recover to logmck and replaced display with cprintf
 *
 * 19-Feb-86 -- pmk  
 *	Added check to 750memerr for controller, also memenable
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

#define NMC750	8
#define MC750_TBPAR	0x4
#define MC750_CACHERR	0xc
#define MC750_RDS	0x4
#define MC750_NXM	0x8

char   *mc750[] = {
	0,					/* 0 */
	"control store parity error",		/* 1 */
	" error",				/*
						 * 2 - "translation buffer
						 * parity", "bus" or "cache"
						 * will preceed this message
						 */
	0, 0, 0,				/* 3, 4 & 5 */
	"microcode lost",			/* 6 */
	"ird rom lost"				/* 7 */
};

struct mc750frame {
	int	mc5_bcnt;			/* byte count == 0x28 */
	int	mc5_summary;			/* summary parameter */
	int	mc5_va; 			/* virtual address register */
	int	mc5_errpc;			/* error pc */
	int	mc5_mdr;
	int	mc5_svmode;			/* saved mode register */
	int	mc5_rdtimo;			/* read lock timeout */
	int	mc5_tbgpar;			/* tb group parity error reg */
	int	mc5_cacherr;			/* cache error register */
	int	mc5_buserr;			/* bus error register */
	int	mc5_mcesr;			/* machine check status reg */
	int	mc5_pc; 			/* trapped pc */
	int	mc5_psl;			/* trapped psl */
};

unsigned last_cacherr = 0;
unsigned last_tbpar = 0;
extern	int	cache_state;
#define MCHK_THRESHOLD 10			/* 10 MS */

ka750machcheck (cmcf)
caddr_t cmcf;
{
	register u_int type = ((struct mcframe	 *) cmcf) -> mc_summary;
	register struct mc750frame *mcf = (struct mc750frame   *) cmcf;
	int recover = 0;
	int cpunum = 0;
	unsigned t;

	setcache(1);	/* disable cache */

	/*
	 * The following `if' is to determine if we are
	 * here because of a tb parity error or cache
	 * parity error. If we are, then handle them quietly
	 * (mprintf).  Check to see if the current error has
	 * occurred within the MCHK_THRESHOLD.	If it is a
	 * reoccurring TB Parity error then we die.  If it
	 * is a reoccurring Cache Parity error then disable
	 * cache and return.
	 */
	if (type == 2) {
		register struct mcr *mcr;
		unsigned t;

		mcr = mcrdata[0].mcraddr;	/* get MS750 CSR */
		if (mcf -> mc5_mcesr & MC750_TBPAR) {
			t = mfpr (TODR);
			if ((t - last_tbpar) < MCHK_THRESHOLD) {
				cprintf("tbuf par: flushing and returning\n");
				mtpr (TBIA, 0);
				recover = 1;
			}
			last_tbpar = t;
		} else {
			if ((mcf -> mc5_cacherr & MC750_CACHERR) ||
			((mcf -> mc5_buserr & MC750_RDS) &&
			(!M750_HRDERR(mcr)))) {
				t = mfpr (TODR);
				if ((t - last_cacherr) < MCHK_THRESHOLD) {
					cache_state = 1; /* disable */
					cprintf("\nFatal cache error - Cache is disabled\n");
				}
				last_cacherr = t;
				mtpr (CAER, mcf -> mc5_cacherr);
				recover = 1;
			}
		}
	}

	logmck((int *)cmcf, ELMCKT_750, cpunum, recover);
	if (recover == 0) {
	    cprintf("\nmachine check %x: ", type);
	    switch (type) {
		case 1:
		case 6:
		case 7:
			cprintf("%s\n", mc750[type]);
			break;
		case 2: {
			char *ptr;
			if (mcf -> mc5_mcesr & MC750_TBPAR)
				ptr = "tb parity";
			else if (mcf -> mc5_cacherr & MC750_CACHERR)
				ptr = "cache parity";
			else if (mcf -> mc5_buserr & MC750_RDS)
				ptr = "uncorrected data";
			else if (mcf -> mc5_buserr & MC750_NXM)
				ptr = "non-existant reference";
			else
				ptr = "Unkown";

			cprintf("%s%s\n", ptr, mc750[type]);
			break;
		}
		default:
			cprintf("%s\n", "Unknown machine check type code");
			break;
	    }
	    cprintf("\tsumpar\t= %x\n", mcf -> mc5_summary);
	    cprintf("\tva\t= %x\n", mcf -> mc5_va);
	    cprintf("\terrpc\t= %x\n", mcf -> mc5_errpc);
	    cprintf("\tmdr\t= %x\n", mcf -> mc5_mdr);
	    cprintf("\tsmr\t= %x\n", mcf -> mc5_svmode);
	    cprintf("\trdtimo\t= %x\n", mcf -> mc5_rdtimo);
	    cprintf("\ttbgpar\t= %x\n", mcf -> mc5_tbgpar);
	    cprintf("\tcacherr\t= %x\n", mcf -> mc5_cacherr);
	    cprintf("\tbuserr\t= %x\n", mcf -> mc5_buserr);
	    cprintf("\tmcesr\t= %x\n", mcf -> mc5_mcesr);
	    cprintf("\tpc\t= %x\n", mcf -> mc5_pc);
	    cprintf("\tpsl\t= %x\n\n", mcf -> mc5_psl);
	    cprintf("\tmcsr\t= %x\n", mfpr (MCSR));
	}
	mtpr (MCESR, mcf -> mc5_mcesr);
	if (recover) {
		setcache(cache_state);
		return;
	}
	memerr ();
	panic ("mchk");
	return(0);
}

/*
 *  Function:
 *	ka750memerr()
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

ka750memerr()
{
	register struct mcr *mcr;
	register int m;
	struct el_rec *elrp;
	struct el_mem *mrp;

	for (m = 0; m < nmcr; m++) {
	    mcr = mcrdata[m].mcraddr;
	    if (M750_ERR (mcr)) {
	        elrp = ealloc(EL_MEMSIZE,EL_PRILOW);
	        if (elrp != NULL) {
		    LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_750,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		    mrp = &elrp->el_body.elmem;
		    mrp->elmem_cnt = 1;
		    mrp->elmemerr.cntl = 1;
		    mrp->elmemerr.type = M750_HRDERR(mcr) ? 2 : 1;
		    mrp->elmemerr.numerr = 1;
		    mrp->elmemerr.regs[0] = mcr->mc_reg[0];
		    mrp->elmemerr.regs[1] = mcr->mc_reg[1];
		    mrp->elmemerr.regs[2] = mcr->mc_reg[2];
		    mrp->elmemerr.regs[3] = EL_UNDEF;
		    EVALID(elrp);
	        }
	        M750_INH(mcr);
	    }
	}
	return(0);
}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka750setcache(state)
int state;
{
	mtpr (CADR, state);
	return(0);
}

/*
 * Enable cache
 */

ka750cachenbl()
{
	cache_state = 0;
	return(0);
}

/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka750memenable ()
{
	register struct mcr *mcr;
	register int m;

	for (m = 0; m < nmcr; m++) {
	    mcr = mcrdata[m].mcraddr;
	    M750_ENA (mcr);
	}
	return(0);
}

ka750tocons(c)
	register int c;
{
	while ((mfpr (TXCS) & TXCS_RDY) == 0)
		continue;
	mtpr (TXDB, c);
	return(0);
}

ka750conf(cpup)
struct cpusw *cpup;
{
	union cpusid cpusid;

	cpusid.cpusid = mfpr(SID);
	printf("VAX 11/750, hardware level = 0x%x, microcode level = %d\n",
		cpusid.cpu750.cp_hrev, cpusid.cpu750.cp_urev);
	probesbi(cpup);
	return(0);
}

short *ka750nexaddr(ioadpt,nexnum) 
 	int ioadpt,nexnum;
{

	return(NEX750(nexnum));

}

u_short *ka750umaddr(ioadpt,ubanumber) 
 	int ubanumber,ioadpt;
{

	return(UMEM750(ubanumber));

}

u_short *ka750udevaddr(ioadpt,ubanumber) 
 	int ioadpt,ubanumber;
{

	return(UDEVADDR750(ubanumber));

}

