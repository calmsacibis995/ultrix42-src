#ifndef lint
static char *sccsid = "@(#)ka610.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86,87 by			*
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

/***********************************************************************
 *
 * Modification History:
 *
 * 27-Nov-89    Paul Grist
 *      added frame_type argument to logmck() calls.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 31-Jan-89 -- map (Mark A. Parenti)
 *	Change include syntax for merged pool.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass/kmalloc support to the system.
 *
 * 30-Aug-86   -- fred (Fred Canter)
 *	Removed unnecessary reference to ra_info, which cause the
 *	kernel not to load if no UQ devices were configured.
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 * 16-Apr-86 - ricky palmer
 *	Fixed an include file problem due to some changes to udareg.h
 *	for devioctl.
 *
 * 16-Apr-86	darrell
 *	Removed a call to scbprot which is called in configure, and
 *	doesn't need to be called here.
 *
 * 16-Apr-86	afd
 *	Changed UMEMmap to QMEMmap and umem to qmem.
 *
 * 14-Apr-86	afd
 *	Put "contigphys" routine in here, which used to be in
 *	/sys/vaxuba/uba.c, since its only used by the uVAX I.
 *
 * 15-Mar-86	Darrell Dunnuck
 *	Moved ka610 specific portion of configure and probnexus
 *	here into ka610conf.
 *
 * 05-Mar-86 -- pmk
 *	Added arg recover to logmck and changed display to cprintf
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 12-Feb-86	Darrell Dunnuck
 *	Removed the routines memerr, setcache, and tocons
 *	from machdep.c and put them here for this processor type.
 *	Added a new routine cachenbl.
 *
 * 12-Dec-85	Darrell Dunnuck
 *	Created this file to as part of machdep restructuring.
 *
 **********************************************************************/

#include "../h/types.h"
#include "../h/time.h"
#include "../machine/cons.h"
#include "mba.h"
#include "uba.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/dmap.h"
#include "../h/reboot.h"
#include "../h/devio.h"
#include "../h/vmmac.h"
#include "../h/kmalloc.h"
#include "../h/errlog.h"

#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/ioa.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/mba/vax/mbareg.h"
#include "../io/mba/vax/mbavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../../machine/common/cpuconf.h"

#define QMEMCSRBASE 0x1440
#define QMEMCSREND  0x145e
#define QMEMSIZE    (512*8192)

/* save record of sbis present for sbi error logging for 780 and 8600 */
extern long sbi_there;	/* bits 0-15 for nexi,sbi0; 16-31 for nexi on sbi1*/
extern struct cpusw *cpup;	/* pointer to cpusw entry */

#define NMCUVI 10
char *mcUVI[] = {
	"memory controller bugcheck",		/* 0 */
	"unrecoverable read error",		/* 1 */
	"nonexistent memory",			/* 2 */
	"illegal operation",			/* 3 */
	"unrecoverable page table read error",	/* 4 */
	"unrecoverable page table write error", /* 5 */
	"control store parity error",		/* 6 */
	"micromachine bug check",		/* 7 */
	"Q22-bus vector read error",		/* 8 */
	"write parameter error" 		/* 9 */
};

struct mcUVIframe {
	int	mc1_bcnt;			/* byte count == 0xc */
	int	mc1_summary;			/* summary parameter */
	int	mc1_parm[2];			/* parameter 1 and 2 */
	int	mc1_pc; 			/* trapped pc */
	int	mc1_psl;			/* trapped psl */
};

ka610machcheck (cmcf)
caddr_t cmcf;
{
	register u_int type = ((struct mcframe	 *) cmcf) -> mc_summary;
	register struct mcUVIframe *mcf = (struct mcUVIframe   *) cmcf;
	int recover = 0;
	int cpunum = 0;
	unsigned t;

	logmck((int *)cmcf, ELMCKT_UVI, cpunum, recover);
	if (recover == 0) {
	    cprintf("\nmachine check %x: ", type);
	    switch (type) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			cprintf("%s\n", mcUVI[type]);
			break;
		default:
			cprintf("%s\n", "Unknown machine check type code");
			break;
	    }
	    cprintf("\tsumpar\t= %x\n", mcf -> mc1_summary);
	    cprintf("\tparam1\t= %x\n", mcf -> mc1_parm[0]);
	    cprintf("\tparam2\t= %x\n", mcf -> mc1_parm[1]);
	    cprintf("\tpc\t= %x\n", mcf -> mc1_pc);
	    cprintf("\tpsl\t= %x\n\n", mcf -> mc1_psl);
	    cprintf("\tmcesr\t= %x\n", mfpr (MCESR));
	}
	mtpr (MCESR, 0xf);

	memerr ();
	panic ("mchk");
	return(0);
}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka610setcache(state)
int state;
{
	mtpr (CADR, state);
	return(0);
}

/*
 * Enable cache
 */

extern	int	cache_state;

ka610cachenbl()
{
	cache_state = 0;
	return(0);
}

ka610tocons(c)
	register int c;
{
	while ((mfpr (TXCS) & TXCS_RDY) == 0)
		continue;
	mtpr (TXDB, c);
	return(0);
}

ka610conf()
{
	register u_short *qmemcsrbase,*qmemcsrend;
	register char  *nxv;
	char  *nxp;
	union cpusid cpusid;
	extern int fl_ok;
	int major_d, minor_d;
	union nexcsr nexcsr;

	cpusid.cpusid = mfpr(SID);
	if( fl_ok )
		printf("MicroVAX-I, Dfloat Microcode, level = %d\n",
			cpusid.cpuMVI.cp_urev);
	else
		printf("MicroVAX-I, Gfloat Microcode, level = %d\n",
			cpusid.cpuMVI.cp_urev);
	/*
	 * We now have an scb set up so we can handle
	 * interrupts if they are waiting to happen.
	 */
	(void) spl0();
	/*
	 * MicroVAX-I doesn't have any nexus space to map or probe,
	 * so we fake it.
	 */
	KM_ALLOC(nxv, char *, sizeof(struct nexus), KM_TEMP, KM_CLEAR|KM_NOWAIT|KM_CONTIG);
	nxp = (char *)svtophy(nxv);
	/*
	 * Map the nexus.
	 */
	nxaccess (nxp, Nexmap[0], cpup->pc_nexsize);
	/*
	 * See if there is anything there.
	 */
	if ((*cpup->badaddr)((caddr_t) nxv, 4))
		return(-1);
	sbi_there = 1<<0;
	printf("Q22 bus\n");
	uba_hd[0].uba_type = UBAUVI;
	unifind (nxv, nxp, qmem[0],
		cpup->umaddr(0,0),
		cpup->pc_umsize,
		cpup->udevaddr(0,0),
		QMEMmap[0], cpup->pc_haveubasr,(long) 0, (long) 0);
	/*
	 * Turn on parity error detection on
	 * each of the possible memory
	 * arraycards. This must be done now
	 * because we cannot probe the memory
	 * cards on uVAX-I after the SCB has
	 * been protected.
	 */
	qmemcsrbase = (u_short *)((char *)qmem+QMEMSIZE+QMEMCSRBASE);
	qmemcsrend  = (u_short *)((char *)qmem+QMEMSIZE+QMEMCSREND);
	do {
		if( !(*cpup->badaddr)( qmemcsrbase, sizeof(short) ) )
			*qmemcsrbase++ = 1;
	} while ( ++qmemcsrbase <= qmemcsrend );
	return(0);
}

short *ka610nexaddr(ioadpt,nexnum)
	int ioadpt,nexnum;
{

	return(NEXUVI);

}

char *ka610umaddr(ioadpt,nexnum)
	int nexnum,ioadpt;
{

	return(QMEMUVI);

}

u_short *ka610udevaddr(ioadpt,nexnum)
	int nexnum, ioadpt;
{

	return(QDEVADDRUVI);

}


/*
 * Convert virtual address to physical address, check that region is
 * contiguous.
 */
contigphys(va, size, pte)
	caddr_t va;
	int	size;
	struct pte *pte;
{
	int	physaddr;
	int	npte, pfn;

	physaddr = (int) va & PGOFSET;
	npte = btoc( size + physaddr);
	pfn = pte->pg_pfnum;
	physaddr |= pfn << 9;
	while( --npte > 0) {
		if( (++pte)->pg_pfnum != pfn+1) {
			printf( "contigphys: non-contiguous request\n");
			printf( " va=%x, size=%d, pa=%x, npte=%d, pfn0=%x, pfn1=%x\n",
				va, size, physaddr, npte, pfn, pte->pg_pfnum);
			return	0;
		}
		pfn = pte->pg_pfnum;
	}
	return	physaddr;
}
