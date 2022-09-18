#ifndef lint
static char *sccsid = "@(#)ka630.c	4.2	ULTRIX	10/10/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86,87,88 by		*
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
 * 18-Apr-90    Tony Griffiths, TaN Engineering (Australia)
 *
 *      Add code to allow config of the dsh (DST32/DSH32 Sync) device
 *      driver on the uVAX-2000.
 *
 * 03-Dec-89	Fred Canter
 *	Add code to allow config of the sp pseudo driver for user
 *	devices on the MV2000.
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
 * 15-Feb-1989	-- afd (Al Delorey)
 *	Re-synch with VAX 3.0 for 3.2 pool merge.
 *
 * 31-Jan-1989	-- map (Mark Parenti)
 *	Change include syntax for merged pool.
 *
 * 25-Sep-88		Fred Canter
 *	Remove old (already commented out) printfs.
 *
 * 03-Jun-1988		Fred Canter
 *	Print KA410 processor on startup instead of VS2000/MV2000.
 *
 * 08-Jan-1988		Todd M. Katz
 *	The configuration adapter structure has been notified to include
 *	fields for bus and nexus numbers, and parameters have been added
 *	to the routine config_set_alive() so that it may set these fields.
 *	Appropriately modify all invocations of this function within this
 *	module.
 *
 * 14-May-87   -- fred (Fred Canter)
 *	Turn off graphics console loopback of kernel printf's
 *	when the system panics (via machine check).
 *
 * 06-Jan-87   -- gmm (George Mathew)
 *	updated ui_dk with dkn (no. of iostat dk numbers assigned) in 
 *	uba_device structure for Vaxstar disk drives for iostat and vmstat
 *	to work correctly.
 *
 * 13-Dec-86   -- rafiey (Ali Rafieymehr)
 *	Changed VAXstar color driver (sg) CSR.
 *
 * 24-Sep-86   -- fred (Fred Canter)
 *	Call config_set_alive("uba" 0) to vaxstar_conf() so installation
 *	sizer program will see uba0 alive.
 *
 * 11-Sep-86   -- fred (Fred Canter)
 *	They changed the VAXstar system names again!
 *
 * 30-Aug-86   -- fred (Fred Canter)
 *	Removed unnecessary reference to ra_info, which caused the
 *	kernel not to link if no UQ devices were configured.
 *	Change VAXstar to VAXstation 2200 and general cleanup.
 *
 * 27-Aug-86  -- bstevens
 *	Cleared map registers set by the vmb so that sizing for
 *	multiple qdss will work.
 *
 * 27-Aug-86  -- fred (Fred Canter)
 *	Remove commented out code and unnecessary comments.
 *
 * 13-Aug-86  -- bjg
 *	Clean up memory register cprintfs
 *
 *  5-Aug-86  -- fred (Fred Canter)
 *	Changed st to stc and many changes to vaxstar_conf().
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 *  9-Jul-86 -- bjg
 *	Identify Vaxstar memory error uniquely
 *
 *  2-Jul-86   -- fred (Fred Canter)
 *	General cleanup of VAXstar autoconfigure code and
 *	changes to make swap on boot and swap on generic work.
 *
 * 18-Jun-86   -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 30-May-86 -- pmk
 *	Changed ka630memerr routine error type and controller value
 *
 * 23-Apr-86 -- pmk
 *	Added ka630memerr() routine, added new machine check type
 *	new 630 doc.
 *
 * 16-Apr-86 - ricky palmer
 *	Fixed an include file problem due to some changes to udareg.h
 *	for devioctl.
 *
 * 16-Apr-86	darrell
 *	Removed call to scbprot which is called in configure, and
 *	doesn't need to be called here.
 *
 * 16-Apr-86	afd
 *	Changed UMEMmap to QMEMmap and umem to qmem.
 *
 * 15-Mar-86	Darrell Dunnuck
 *	Moved ka630 specific portions of configure() and probenexus()
 *	here into ka630conf.
 *
 * 05-Mar-86 -- pmk
 *	added arg recover to logmck and replaced display with cprintf
 *
 * 05-Mar-86 -- jaw  VAXBI device and controller config code added.
 *		     todr code put in cpusw.
 *
 * 18-Feb-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 12-Feb-86	Darrell Dunnuck
 *	Removed the routines memerr, memenable, setcache, and tocons
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
#include "../machine/clock.h"
/*
#include "mba.h"
#include "uba.h"
*/

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
#include "../h/errlog.h"
#include "../io/uba/qdioctl.h"	/* for QD_KERN_UNLOOP below */

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

/* save record of sbis present for sbi error logging for 780 and 8600 */
extern long sbi_there;	/* bits 0-15 for nexi,sbi0; 16-31 for nexi on sbi1*/
extern struct cpusw *cpup;	/* pointer to cpusw entry */

#define NMCUVII 14
char *mcUVII[] = {
	"unknown ????????????",				/* 0 */
	"impossible microcode state (FSD)",		/* 1 */
	"impossible microcode state (SSD)",		/* 2 */
	"undefined FPU error code 0",			/* 3 */
	"undefined FPU error code 7",			/* 4 */
	"undefined memory management status (TB miss)",	/* 5 */
	"undefined memory management status (M = 0)",	/* 6 */
	"process PTE address in P0 space",		/* 7 */
	"process PTE address in P1 space",		/* 8 */
	"undefined interrupt ID code",			/* 9 */
	"read bus error, VAP is virtual",		/* 80 */
	"read bus error, VAP is physical",		/* 81 */
	"write bus error, VAP is virtual",		/* 82 */
	"write bus error, VAP is physical",		/* 83 */
};

struct mcUVIIframe {
	int	mc1_bcnt;			/* byte count == 0xc */
	int	mc1_summary;			/* summary parameter */
	int	mc1_vap;			/* most recent virtual addr */
	int	mc1_internal_state;		/* internal state ? */
	int	mc1_pc; 			/* trapped pc */
	int	mc1_psl;			/* trapped psl */
};

extern	int	ws_display_type;

ka630machcheck (cmcf)
caddr_t cmcf;
{
	register u_int type = ((struct mcframe	 *) cmcf) -> mc_summary;
	register struct mcUVIIframe *mcf = (struct mcUVIIframe	 *) cmcf;
	int recover = 0;
	int cpunum = 0;
	unsigned t;
	int i;

	logmck((int *)cmcf, ELMCKT_UVII, cpunum, recover);
	/*
	 * If console is a graphics device,
	 * force cprintf messages directly to screen.
	 */
	if (ws_display_type) {
	    i = ws_display_type << 8;
	    (*cdevsw[ws_display_type].d_ioctl)(i, QD_KERN_UNLOOP, 0, 0);
	}
	if (recover == 0) {
	    cprintf("\nmachine check %x: ", type);
	    switch (type) {
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
			/*
			 * Types are disjoint. Have to convert to
			 * linear range.
			 */
			type = type - 0x80 + 10;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			cprintf("%s\n", mcUVII[type]);
			break;
		default:
			cprintf("%s\n", "Unknown machine check type code");
			break;
	    }
	    cprintf("\tsumpar\t= %x\n", mcf -> mc1_summary);
	    cprintf("\tmost recent virtual addr\t=%x\n", mcf -> mc1_vap);
	    cprintf("\tinternal state\t=%x\n", mcf -> mc1_internal_state);
	    cprintf("\tpc\t= %x\n", mcf -> mc1_pc);
	    cprintf("\tpsl\t= %x\n\n", mcf -> mc1_psl);
	    if(cpu_subtype == ST_MVAXII) {
		cprintf("\tmser\t= %x\n", ((struct qb_regs *)nexus)->qb_mser);
		cprintf("\tcear\t= %x\n", ((struct qb_regs *)nexus)->qb_caer);
	    } else {
		cprintf("\tmser\t= %x\n", ((struct nb_regs *)nexus)->nb_mser);
		cprintf("\tmear\t= %x\n", ((struct nb_regs *)nexus)->nb_mear);
	    }
	    if(cpu_subtype == ST_MVAXII)
	        cprintf("\tdear\t= %x\n", ((struct qb_regs *)nexus)->qb_daer);
	}
	ka630memerr();
	mtpr (MCESR, 0xf);	/* nop on VAXstar */

	panic ("mchk");
	return(0);
}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 * NOTE: this routine does nothing on a VAXstation 2000 or MicroVAX 2000.
 */

ka630setcache(state)
int state;
{
	mtpr (CADR, state);
	return(0);
}

/*
 * Enable cache
 */

extern	int	cache_state;

ka630cachenbl()
{
	cache_state = 0;
	return(0);
}

ka630tocons(c)
	register int c;
{
	if (cpu_subtype == ST_MVAXII) {
		while ((mfpr (TXCS) & TXCS_RDY) == 0)
			continue;
		mtpr (TXDB, c);
	}
	return(0);
}

ka630conf()
{
	register u_short *qmemcsrbase,*qmemcsrend;
	register char  *nxv;
	register int i;
	unsigned int k;
	unsigned int *mapaddr;
	char  *nxp;
	extern int fl_ok;

	/*
	 * If this is a VAXstar,
	 * call its conf() routine.
	 */
	if (cpu_subtype == ST_VAXSTAR) {
		vaxstar_conf();
		return(0);
	}

	printf("MicroVAX-II ");
	if( fl_ok )
		printf("with an FPU\n");
	else
		printf("without an FPU\n");
	/*
	 * We now have the scb set up enough so we can handle
	 * interrupts some if they are waiting to happen.
	 */
	(void) spl0();
	nxv = (char *)nexus;
	nxp = (char *)cpup->nexaddr(0,0);
	/*
	 * Map the nexus.
	 */
	nxaccess (nxp, Nexmap[0], cpup->pc_nexsize);

	/*
	 * Clear the map registers that the vmb set.  This is needed
	 * so that the qdss sizing will work.
	 */
	mapaddr = (unsigned int *) ((struct qb_regs *)nexus)->qb_uba.qba.qb_map;
	for (k = 0 ; k < 8192 ; k++)
	    *mapaddr++ = k;

	/*
	 * See if there is anything there.
	 */
	if ((*cpup->badaddr)((caddr_t) nxv, 4))
		return(-1);
	sbi_there |= 1<<0;
	printf("Q22 bus\n");
	uba_hd[0].uba_type = UBAUVII;
	unifind ((&((struct qb_regs *)nxv)->qb_uba.uba), 
		(&((struct qb_regs *)nxp)->qb_uba.uba), 
		qmem[0],
		cpup->umaddr(0,0),
		cpup->pc_umsize,
		cpup->udevaddr(0,0),
		QMEMmap[0], cpup->pc_haveubasr,(long) 0, (long) 0);
	/*
	 * Turn on parity error detection and clear any
	 * left over bits in the mser register (nxm from probe).
	 */
	((struct qb_regs *)nexus)->qb_mser =
			QBM_NXM | QBM_LPE | QBM_QPE |
			QBM_DMAQPE | QBM_LEB | QBM_PENB;
	/*
	 * Tell the console program that we've booted and
	 * that we speak english and would like to restart
	 * if the machine panics.
	 */
	((struct qb_regs *)nexus)->qb_cpmbx=RB_RESTART;
	for( i=40 ; i<44 ; i++ )
		((struct qb_regs *)nexus)->qb_toyram[i]=0;
	return(0);
}

/*
 * Configure routine for VAXstar.
 *
 * Please forgive the "unibus flavor" of this code.
 * Much of it was copied from the MicroVAX-II configure code,
 * which was copied from the unibus VAX configure code.
 * Fred Canter -- 6/22/86
 */

extern	int	nNUBA;
extern  int	dkn;
extern	struct	uba_driver	sdcdriver;
extern	struct	uba_driver	stcdriver;
extern	struct	uba_driver	ssdriver;
extern	struct	uba_driver	lndriver;
extern	struct	uba_driver	smdriver;
extern	struct	uba_driver	sgdriver;
extern	struct	uba_driver	shdriver;
extern	struct	uba_driver	spdriver;
extern  struct  uba_driver      dshdriver;  /* DST/DSH32 Sync Device driver */

vaxstar_conf()
{
	register int i;
	extern int fl_ok;
	struct uba_device *ui;
	struct uba_ctlr *um;
	struct uba_hd *uhp;
	struct uba_driver *udp;
	extern int catcher[256];
	int (**ivec)();
	caddr_t addr;

	printf("KA410 processor ");
	if( fl_ok )
		printf("with an FPU\n");
	else
		printf("without an FPU\n");
	/*
	 * We now have the scb set up enough so we can handle
	 * interrupts some if they are waiting to happen.
	 */
	(void) spl0();

	uhp = &uba_hd[numuba];
	uhp->uh_vec = SCB_UNIBUS_PAGE(numuba);
	uhp->uh_lastiv = 0x200;
	for (i=0; i<(uhp->uh_lastiv/4); i++)
		uhp->uh_vec[i] = scbentry(&catcher[i*2], SCB_ISTACK);

	/*
	 * Say uba0 alive (so installation sizer will see it).
	 */
	config_set_alive("uba", 0, 0, -1);

	/*
	 * Check each unibus mass storage controller.
	 * For each one which is potentially on this uba,
	 * see if it is really there, and if it is record it and
	 * then go looking for slaves.
	 *
	 * For VAXstar, only config the system disk cntlr (sdc0)
	 * and SCSI TK50 tape cntlr (stc0).
	 */
	for (um = ubminit; udp = um->um_driver; um++) {
		if(um->um_alive)
			continue;
		if (udp == &sdcdriver)
			addr = (caddr_t)0x200c0000;
		else if (udp == &stcdriver)
			addr = (caddr_t)0x200c0080;
		else
			continue;
		cvec = 0x200;
		i = (*udp->ud_probe)(addr);
		if (i == 0)
			continue;
		um->um_ubanum = numuba;
		config_fillin(um);
		printf(" csr 0x%x ", addr);
		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec 0x%x, ipl 0x%x\n", cvec, br);
		um->um_alive = 1;
		um->um_hd = &uba_hd[numuba];
		um->um_addr = (caddr_t)addr;
		um->um_physaddr = (caddr_t)addr;
		udp->ud_minfo[um->um_ctlr] = um;
		for (ivec = um->um_intr; *ivec; ivec++) {
			um->um_hd->uh_vec[cvec/4] =
			    scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		/*
		 * Only allows VAXstar disks/tapes (sd0->sd# and st0).
		 */
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if ((ui->ui_driver != udp) || ui->ui_alive)
				continue;
			if ((*udp->ud_slave)(ui)) {
				ui->ui_alive = 1;
				ui->ui_ctlr = um->um_ctlr;
				ui->ui_ubanum = numuba;
				ui->ui_hd = &uba_hd[numuba];
				ui->ui_addr = (caddr_t)addr;
				ui->ui_physaddr = addr; /* rpb: swap on boot */
				if(ui->ui_dk && dkn < DK_NDRIVE)
					ui->ui_dk = dkn++;
				else
					ui->ui_dk = -1;
				ui->ui_mi = um;
				/* ui_type comes from driver */
				udp->ud_dinfo[ui->ui_unit] = ui;
				printf("%s%d at %s%d slave %d\n",
				    ui->ui_devname, ui->ui_unit,
				    udp->ud_mname, um->um_ctlr, ui->ui_slave);
				(*udp->ud_attach)(ui);
			}
		}
	}

	/*
	 * Configure remaining VAXstar devices, no others.
	 */
	for (ui=ubdinit; udp=ui->ui_driver; ui++) {
		if (udp == &ssdriver)
			addr = (caddr_t)0x200a0000;
		else if (udp == &smdriver)
			addr = (caddr_t)0x200f0000;
		else if (udp == &sgdriver)
			addr = (caddr_t)0x3c000000;
		else if (udp == &shdriver)
			addr = (caddr_t)0x38000000;
                else if (udp == &dshdriver)     /* DST32/DSH32 sync driver  */
                        addr = (caddr_t)0x38000000;
		else if (udp == &spdriver)	/* user device pseudo driver */
			addr = ui->ui_addr;	/* get CSR from config line */
		else
			continue;
		/*
		 * Following should never happen on a VAXstar.
		 */
		if ((ui->ui_ubanum != numuba && ui->ui_ubanum != '?') ||
		    ui->ui_alive || ui->ui_slave != -1)
			continue;
		cvec = 0x200;
		i = (*udp->ud_probe)(addr);
		if(i == 0)
			continue;
                if (udp == &dshdriver)      /* DST/DSH32 driver returns CSR */
                        addr = (caddr_t)i;  /* address from probe() routine */
		ui->ui_ubanum = numuba;
		config_fillin(ui);
		printf(" csr 0x%x ", addr);
		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec 0x%x, ipl 0x%x\n", cvec, br);
		ui->ui_hd = &uba_hd[numuba];
		for (ivec=ui->ui_intr; *ivec; ivec++) {
			ui->ui_hd->uh_vec[cvec/4] =
				scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		ui->ui_alive = 1;
		ui->ui_addr = (caddr_t)addr;
		ui->ui_physaddr = 0;
		ui->ui_dk = -1;
		udp->ud_dinfo[ui->ui_unit] = ui;
		(*udp->ud_attach)(ui);
	}
	
	/* 
	 * Configure for ibus on CVAXstart
	 */
        config_set_alive("ibus", 0);
        for (ui=ubdinit; udp=ui->ui_driver; ui++) {
                if (udp == &lndriver && ui->ui_unit==0)
                        addr = (caddr_t) 0x200e0000;
                else
                        continue;
                /*
                 * Following should never happen on a CVAXstar.
                          */
                if (ui->ui_ubanum != -1 ||
                    ui->ui_alive || ui->ui_slave != -1)
                        continue;
                cvec = 0x200;
                i = (*udp->ud_probe)(addr);
                if(i == 0)
                        continue;
                ui->ui_adpt = 0;
                config_fillin(ui);
                printf(" csr 0x%x ", addr);
                if (cvec == 0) {
                        printf("zero vector\n");
                        continue;
                }
                if (cvec == 0x200) {
                        printf("didn't interrupt\n");
                        continue;
                }
                printf("vec 0x%x, ipl 0x%x\n", cvec, br);
                ui->ui_hd = &uba_hd[numuba];
                for (ivec=ui->ui_intr; *ivec; ivec++) {
                        ui->ui_hd->uh_vec[cvec/4] =
                                scbentry(*ivec, SCB_ISTACK);
                                      cvec += 4;
                }
                ui->ui_alive = 1;
                ui->ui_addr = (caddr_t)addr;
                ui->ui_physaddr = 0;
                ui->ui_dk = -1;
                udp->ud_dinfo[ui->ui_unit] = ui;
                (*udp->ud_attach)(ui);
        }

	/*
	 * Turn on parity error detection and clear any
	 * left over bits in the mser register (nxm from probe).
	 */
	((struct nb_regs *)nexus)->nb_mser =  QBM_LPE | QBM_PENB;
	/*
	 * Tell the console program that we've booted and
	 * that we would like to restart if the machine panics.
	 */
	((struct nb_regs *)nexus)->nb_cpmbx=RB_VS_RESTART;
	return(0);
}

short *ka630nexaddr(ioadpt,nexnum)
	int nexnum,ioadpt;
{

	return(NEXUVII);

}

char *ka630umaddr(ioadpt,nexnum)
	int ioadpt,nexnum;
{

	if (cpu_subtype == ST_MVAXII)
		return(QMEMUVII);
	else
		return(QMEMVAXSTAR);

}

u_short *ka630udevaddr(ioadpt,nexnum)
	int nexnum,ioadpt;
{

	return(QDEVADDRUVI);

}

ka630readtodr()
{

	u_int s,todr;
	struct tm tm;
	register struct qb_regs *qb_regs = (struct qb_regs *)nexus;

	/*
	 * If this is a VAXstar,
	 * call its readtodr() routine.
	 */
	if(cpu_subtype == ST_VAXSTAR)
		return(vaxstar_readtodr());

	/*
	 * Copy the toy register contents into tm so that we can
	 * work with. The toy must be completely read in 2.5 millisecs.
	 *
	 *
	 * Wait for update in progress to be done.
	 */
	while( qb_regs->qb_toycsra & QBT_UIP )
			;
	s = spl7();
	tm.tm_sec = qb_regs->qb_toysecs;
	tm.tm_min = qb_regs->qb_toymins;
	tm.tm_hour = qb_regs->qb_toyhours;
	tm.tm_mday = qb_regs->qb_toyday;
	tm.tm_mon = qb_regs->qb_toymonth;
	tm.tm_year = qb_regs->qb_toyyear;
	splx( s );

	todr = toyread_convert(tm);

	return(todr);
}

ka630writetodr(yrtime)
u_int yrtime;
{
	struct tm xtime;
	int s;
	register struct qb_regs *qb_regs = (struct qb_regs *)nexus;

	/*
	 * If this is a VAXstar,
	 * call its writetodr() routine.
	 */
	if(cpu_subtype == ST_VAXSTAR) {
		vaxstar_writetodr(yrtime);
		return;
	}

	toywrite_convert(&xtime,yrtime);

	qb_regs->qb_toycsrb = QBT_SETUP;
	s = spl7();
	qb_regs->qb_toysecs = xtime.tm_sec;
	qb_regs->qb_toymins = xtime.tm_min;
	qb_regs->qb_toyhours = xtime.tm_hour;
	qb_regs->qb_toyday = xtime.tm_mday;
	qb_regs->qb_toymonth = xtime.tm_mon;
	qb_regs->qb_toyyear = xtime.tm_year;
	/*
	* Start the clock again.
	*/
	qb_regs->qb_toycsra = QBT_SETA;
	qb_regs->qb_toycsrb = QBT_SETB;
	splx( s );

}

vaxstar_readtodr()
{

	u_int s,todr;
	struct tm tm;
	register struct nb_regs *nb_regs = (struct nb_regs *)nexus;

	/*
	 * All Vaxstar toy clock registers (including NVR)
	 * read/write bits 2 - 9 instead of 0 - 7.
	 */
	/*
	 * Copy the toy register contents into tm so that we can
	 * work with. The toy must be completely read in 2.5 millisecs.
	 *
	 *
	 * Wait for update in progress to be done.
	 */
	while( nb_regs->nb_toycsra & (QBT_UIP << 2) )
			;
	s = spl7();
	tm.tm_sec = ((nb_regs->nb_toysecs >> 2) & 0xff);
	tm.tm_min = ((nb_regs->nb_toymins >> 2) & 0xff);
	tm.tm_hour = ((nb_regs->nb_toyhours >> 2) & 0xff);
	tm.tm_mday = ((nb_regs->nb_toyday >> 2) & 0xff);
	tm.tm_mon = ((nb_regs->nb_toymonth >> 2) & 0xff);
	tm.tm_year = ((nb_regs->nb_toyyear >> 2) & 0xff);
	splx( s );

	todr = toyread_convert(tm);

	return(todr);
}

vaxstar_writetodr(yrtime)
u_int yrtime;
{
	struct tm xtime;
	int s;
	register struct nb_regs *nb_regs = (struct nb_regs *)nexus;

	toywrite_convert(&xtime,yrtime);

	nb_regs->nb_toycsrb = (QBT_SETUP << 2);
	s = spl7();
	nb_regs->nb_toysecs = (xtime.tm_sec << 2);
	nb_regs->nb_toymins = (xtime.tm_min << 2);
	nb_regs->nb_toyhours = (xtime.tm_hour << 2);
	nb_regs->nb_toyday = (xtime.tm_mday << 2);
	nb_regs->nb_toymonth = (xtime.tm_mon << 2);
	nb_regs->nb_toyyear = (xtime.tm_year << 2);
	/*
 	* Start the clock again.
 	*/
	nb_regs->nb_toycsra = (QBT_SETA << 2);
	nb_regs->nb_toycsrb = (QBT_SETB << 2);
	splx( s );

}

/*
 *  Function: ka630memerr()
 *
 *  Description: log memory errors in kernel buffer
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
ka630memerr()
{
	int mser;
	struct el_rec *elrp;
	struct el_mem *mrp;

	if (cpu_subtype == ST_MVAXII)
		mser = ((struct qb_regs *)nexus)->qb_mser;
	else
		mser = ((struct nb_regs *)nexus)->nb_mser;
	if (mser & QBM_EMASK) {
	    elrp = ealloc(EL_MEMSIZE,EL_PRISEVERE);
	    if (elrp != NULL) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,(cpu_subtype == ST_MVAXII)?ELMCNTR_630:ELMCNTR_VAXSTAR,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		mrp = &elrp->el_body.elmem;
		mrp->elmem_cnt = 1;
		mrp->elmemerr.cntl = (mser & 0x300) >> 8;
		mrp->elmemerr.type = (mser & 0x80) ? ELMETYP_NXM : ELMETYP_PAR;
		mrp->elmemerr.numerr = 1;
		mrp->elmemerr.regs[0] = mser;
		if (cpu_subtype == ST_MVAXII) {
		    mrp->elmemerr.regs[1] = ((struct qb_regs *)nexus)->qb_caer;
		    mrp->elmemerr.regs[2] = ((struct qb_regs *)nexus)->qb_daer;
		} else {
		    mrp->elmemerr.regs[1] = ((struct nb_regs *)nexus)->nb_mear;
		    mrp->elmemerr.regs[2] = 0;
		}
		mrp->elmemerr.regs[3] = EL_UNDEF;
		EVALID(elrp);
	    }
	}
	return(0);
}
