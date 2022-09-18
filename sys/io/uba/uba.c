#ifndef lint
static char *sccsid = "@(#)uba.c	4.2      (ULTRIX)        11/9/90";
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
 ************************************************************************
 * Modification History:
 *
 * 23-Feb-90 -- sekhar
 * 	Merged Joe Martin's fix for 3.1 cld. When copying user PTEs, 
 *	check for page crossing and reevaluate vtopte.
 *
 * 15-Oct-89	Robin Lewis
 *	patched the qbus map register assignment to set the High bit on vax's
 *
 * 20-Jul-89	Mark Parenti
 *	Add code for reset of KDM70.
 *
 * 15-Apr-89	Kong
 *	Fixed bug in qbasetup so that the magic cookie returned is
 *	in the format defined in the routine block comment.
 *	
 * 05-Nov-88	Robin Lewis
 *	Changed the Q22-Bus map register allocation to return registers
 *	in clicks of 8.  This is necessary because there are insuficient
 *	bits in the int to pass back the offset in the page, the starting
 *	map register and the number of map registers allocated.  Nine
 *	bit are needed to have a 512 offset value and that can not change.
 *	The remainint bits return the starting map register with a max
 *	of 8K and the number of registers which can also be 8K ..?
 *	Passing back clicks of 8 registers allows this to work but
 *	does waste registers (but only until they are released.)
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 15-Sep-87  -- darrell
 *	Removed unnecessary consistency checks in vs_bufctl.
 *
 * 19-May-87  -- darrell
 *	Fixed a hole in vs_bufctl that was causing a panic during the
 *	installation on VAXstation/MicroVAX 2000. 
 *	
 * 12-May-87  -- darrell
 *	Added a temporary variable to vs_bufctl to keep track of a 
 * 	pointer to the vsdev structure for the driver that is active.
 *	This fixes several panics.
 *
 * 23-Apr-87  -- darrell
 *	vs_bufctl has been changed to accpet a pointer to structure
 *	that contains the the ID of the device calling it, the action
 *	to be performed, and a pointer to the routine to call back
 *	into to driver. (stc.c or sdc.c)
 *
 * 29-Sep-86  -- darrell
 *	Space for vsbuf is now allocated here instead of in 
 *	../data/sdc_data.c.
 *
 * 26-Sep-86  -- darrell
 *	Fixed a bug in vs_bufctl that caused the vaxstar tape/disk
 *	buffer to be allocated incorrectly.
 *
 * 05-Sep-86  -- darrell
 *	Added a panic. vs_bufctl will now panic if called by the
 *	owner of the buffer.
 *
 * 30-Aug-86  -- darrell (Darrell Dunnuck)
 *	Fix bugs in VAXstar data buffer interlock code, which
 *	allows TZK50 and disk to share a common DMA data buffer.
 *
 *  5-Aug-86   -- gmm (George Mathew) and darrell (Darrell Dunnuck)
 *	Added routines to allow sharing the common disk data buffer
 *	between the VAXstar disk and TZK50 drivers.
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 * 14-May-86 -- bjg
 *	Move uba# field in subid when logging uba errors
 *
 * 16-Apr-86 -- afd
 *	Changed UMEMmap to QMEMmap and umem to qmem for QBUS reset.
 *
 * 19-feb-86 -- bjg  add uba error logging
 *
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * 15-jul-85 -- jaw
 *	VAX8800 support
 *
 * 11 Nov 85   depp
 *	Removed System V conditional compiles.
 *
 * 08-Aug-85	darrell
 *	Zero vectors are now timed.  If we get too many, a message is 
 *	printed into the message buffer reporting the rate at which they
 *	are accuring.  The routine "ubatimer" was added.
 *
 * 11-jul-85 -- jaw
 *	fix bua/bda map registers.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 06 Jun 85 -- jaw
 *	add in the BDA support.
 *
 *  7 May 85 -- rjl
 *	Turned on Q-bus map as part of bus init. 
 * 
 * 22 Mar 85 -- depp
 *	Added Sys V Shared memory support
 *
 * 13-MAR-85 -jaw
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 12 Nov 84 -- rjl
 *	Added support for MicroVAX-II notion of a q-bus adapter.
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/dk.h"
#include "../h/kernel.h"
#include "../h/clist.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/errlog.h"

#include "../machine/cpu.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#ifdef mips
#include "../machine/ssc.h"
#endif mips

#include "../machine/nexus.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

#include "../io/bi/buareg.h"


#define QIPCR 0x1f40			/* Q-bus Inter-processor csr	*/
#define LMEA 0x20			/* Local memory access enable	*/
#define QMCIA 0x4000			/* Invalidate all cached map regs*/

#ifdef mips
#define WBFLUSH	wbflush()		/* Flush write buffer on mips */
#else
#define WBFLUSH ;			/* NOP for vax	*/
#endif mips

/*
 *  For zero vector timer -- should really be in a data.c file, but
 *  but there isn't one for uba.c
 */
int	ubatimer();
int	zvintvl = ZVINTVL;	/* zero vector timer interval in seconds */
int	zvthresh = ZVTHRESH;	/* zero vector timer threhsold for reporting */

char	ubasr_bits[] = UBASR_BITS;

/*
 * Do transfer on device argument.  The controller
 * and uba involved are implied by the device.
 * We queue for resource wait in the uba code if necessary.
 * We return 1 if the transfer was started, 0 if it was not.
 * If you call this routine with the head of the queue for a
 * UBA, it will automatically remove the device from the UBA
 * queue before it returns.  If some other device is given
 * as argument, it will be added to the request queue if the
 * request cannot be started immediately.  This means that
 * passing a device which is on the queue but not at the head
 * of the request queue is likely to be a disaster.
 */
ubago(ui)
	register struct uba_device *ui;
{
	register struct uba_ctlr *um = ui->ui_mi;
	register struct uba_hd *uh;
	register int s, unit;

	uh = &uba_hd[um->um_ubanum];
	s = spl6();
	if (um->um_driver->ud_xclu && uh->uh_users > 0 || uh->uh_xclu)
		goto rwait;
	um->um_ubinfo = ubasetup(um->um_ubanum, um->um_tab.b_actf->b_actf,
	    UBA_NEEDBDP|UBA_CANTWAIT);
	if (um->um_ubinfo == 0)
		goto rwait;
	uh->uh_users++;
	if (um->um_driver->ud_xclu)
		uh->uh_xclu = 1;
	splx(s);
	if (ui->ui_dk >= 0) {
		unit = ui->ui_dk;
		dk_busy |= 1<<unit;
		dk_xfer[unit]++;
		dk_wds[unit] += um->um_tab.b_actf->b_actf->b_bcount>>6;
	}
	if (uh->uh_actf == ui)
		uh->uh_actf = ui->ui_forw;
	(*um->um_driver->ud_dgo)(um);
	return (1);
rwait:
	if (uh->uh_actf != ui) {
		ui->ui_forw = NULL;
		if (uh->uh_actf == NULL)
			uh->uh_actf = ui;
		else
			uh->uh_actl->ui_forw = ui;
		uh->uh_actl = ui;
	}
	splx(s);
	return (0);
}

ubadone(um)
	register struct uba_ctlr *um;
{
	register struct uba_hd *uh = &uba_hd[um->um_ubanum];

	if (um->um_driver->ud_xclu)
		uh->uh_xclu = 0;
	uh->uh_users--;
	/* already released map registers if it's a MicroVAX I */
	if( (uh->uba_type & UBAUVI) ==0)
		ubarelse(um->um_ubanum, &um->um_ubinfo);
}

/*
 * Allocate and setup UBA map registers, and bdp's
 * Flags says whether bdp is needed, whether the caller can't
 * wait (e.g. if the caller is at interrupt level).
 *
 * Return value:
 *	Bits 0-8	Byte offset
 *	Bits 9-17	Start map reg. no.
 *	Bits 18-27	No. mapping reg's
 *	Bits 28-31	BDP no.
 */

ubasetup(uban, bp, flags)
	struct buf *bp;
{
	register struct uba_hd *uh = &uba_hd[uban];
	register int temp;
	int npf, reg, bdp;
	unsigned v;
	register struct pte *pte, *io;
	struct proc *rp;
	int a, o, ubinfo, vax_pfn;
	int user_addr = 0;
#ifdef mips
	int	mips_of;		  /* offset within a mips page	*/
	int	mips_pfn;		  /* PFN of a mips pte		*/
#endif mips

	if (uh->uba_type & (UBA730|UBAUVI|UBAUVII))
		flags &= ~UBA_NEEDBDP;
	v = btop(bp->b_un.b_addr);
	o = (int)bp->b_un.b_addr & 0x1ff; /* offset within a VAX page, even for
					   mips based systesm where PGOFFSET is
					   different - DO NOT replace 0x1ff with
					   PGOFFSET */
#ifdef mips
	/* offset within a mips page */
	mips_of = (int)bp->b_un.b_addr & PGOFSET;
#endif

	/* 
	 * Number of VAX pages spanned by the buffer + 1(hence number of
	 * map registers needed since we need one guard page)
	 */
	npf = (((unsigned)(bp->b_bcount + o)+0x1ff) >> 9) + 1; 

	a = spl6();
	while ((reg = rmalloc(uh->uh_map, (long)npf)) == 0) {
		if (flags & UBA_CANTWAIT) {
			splx(a);
			return (0);
		}
		uh->uh_mrwant++;
		sleep((caddr_t)&uh->uh_mrwant, PSWP);
	}
	bdp = 0;
	if (flags & UBA_NEEDBDP) {
		while ((bdp = ffs(uh->uh_bdpfree)) == 0) {
			if (flags & UBA_CANTWAIT) {
				rmfree(uh->uh_map, (long)npf, (long)reg);
				splx(a);
				return (0);
			}
			uh->uh_bdpwant++;
			sleep((caddr_t)&uh->uh_bdpwant, PSWP);
		}
		uh->uh_bdpfree &= ~(1 << (bdp-1));
	} else if (flags & UBA_HAVEBDP)
		bdp = (flags >> 28) & 0xf;
	splx(a);
	reg--;
	ubinfo = (bdp << 28) | (npf << 18) | (reg << 9) | o;
	temp = (bdp << 21) | UBAMR_MRV;
	if (bdp && (o & 01))
		temp |= UBAMR_BO;
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	pte = 0;
	if ((bp->b_flags & B_PHYS) == 0) {
#ifdef vax
		pte = &Sysmap[btop(((int)bp->b_un.b_addr)&0x7fffffff)];
#else   	
		/* mips KSEG Addressing */
	        if(IS_KSEG0(bp->b_un.b_addr)){ 
			/* unmapped KSEG areas */
                	mips_pfn = btop(K0_TO_PHYS(bp->b_un.b_addr));
	        } else if(IS_KSEG1(bp->b_un.b_addr)){
			/* unmapped KSEG areas */
                	mips_pfn = btop(K1_TO_PHYS(bp->b_un.b_addr));
	        } else if(IS_KSEG2(bp->b_un.b_addr)){
			/* mapped KSEG areas */
	                pte = &Sysmap[btop(bp->b_un.b_addr - K2BASE)];
        	}
#endif vax
	}
	else if (bp->b_flags & B_UAREA)
		pte = &rp->p_addr[v];
	else if (bp->b_flags & B_PAGET)
		pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
	else if ((bp->b_flags & B_SMEM)  &&	/* SHMEM */
					((bp->b_flags & B_DIRTY) == 0))
		pte = ((struct smem *)rp)->sm_ptaddr + v;
	else {
		pte = vtopte(rp, v);
		user_addr++;
	}
	if ((uh->uba_type & UBAUVI) ==0 || (flags&UBA_MAPANYWAY)) {
		/* get address of starting UNIBUS map register */
		io = &uh->uh_uba->uba_map[reg];

                /* Loop to load all the map registers with pte information.
                 * For a mips we need to simulate vax pte structures.
		 *
		 * On a VAX, we have npf map registers to fill, each
		 * map register is to be filled by a VAX pte.
		 *
		 * On a mips, we have npf map registers to fill, but
		 * each mips page is 4KB, so we have fewer mips ptes
		 * to use.
		 *
		 * The following loop iterates for each map register.
                 */
		while (--npf != 0) {
#ifdef mips
			if(pte != 0)
			{	/* pte points to the mips pte */
				if (pte->pg_pfnum == 0)
					panic("uba zero uentry");
				vax_pfn = (pte->pg_pfnum << 3) + 
					  (mips_of / 0x200);
			} else {
				/* There are no pte's, use pfn's
				 */
				vax_pfn = (mips_pfn << 3) +
					  (mips_of / 0x200);
			}
			*(int *)io++ = vax_pfn | UBAMR_MRV;
			mips_of += 0x200; /* next vax page */
			if (mips_of >= 0x1000) {
				/* Crossing mips page boundary */
				/* Go on to next mips page */
				mips_of -= 0x1000;
				if (pte) 
					pte++;
				else
					mips_pfn++;
			}
#else
			if (user_addr &&
			    (((int)pte & PGOFSET) < CLSIZE*sizeof(struct pte)
			     || pte->pg_pfnum == 0))
				pte = vtopte(rp, v);
			if (pte->pg_pfnum == 0)
				panic("uba zero uentry");
			*(int *)io++ = pte++->pg_pfnum | temp;
			v++;
#endif mips
		}
		*(int *)io++ = 0;	/* Set up fire wall */
		WBFLUSH;
	} else	{
		ubinfo = contigphys( ubinfo, bp->b_bcount, pte);
		++reg;
		a = spl6();
		rmfree( uh->uh_map, (long)npf, (long)reg);
		splx(a);
	}
	return (ubinfo);
}

/*
 * Non buffer setup interface... set up a buffer and call ubasetup.
 */
uballoc(uban, addr, bcnt, flags)
	int uban;
	caddr_t addr;
	int bcnt, flags;
{
	struct buf ubabuf;

	ubabuf.b_un.b_addr = addr;
	ubabuf.b_flags = B_BUSY;
	ubabuf.b_bcount = bcnt;
	/* that's all the fields ubasetup() needs */
	return (ubasetup(uban, &ubabuf, flags));
}
 
/*
 * Release resources on uba uban, and then unblock resource waiters.
 * The map register parameter is by value since we need to block
 * against uba resets on 11/780's.
 */
ubarelse(uban, amr)
	int *amr;
{
	register struct uba_hd *uh = &uba_hd[uban];
	register int bdp, reg, npf, s;
	int mr;
 
	/*
	 * Carefully see if we should release the space, since
	 * it may be released asynchronously at uba reset time.
	 */
	s = spl6();
	mr = *amr;
	if (mr == 0) {
		/*
		 * A ubareset() occurred before we got around
		 * to releasing the space... no need to bother.
		 */
		splx(s);
		return;
	}
	*amr = 0;
	splx(s);		/* let interrupts in, we're safe for a while */
	bdp = (mr >> 28) & 0x0f;

	if (bdp) {
		if (uh->uba_type&UBABUA){
			bdp = bdp & 0x07;
			((struct bua_regs *)uh->uh_uba)->bua_dpr[bdp] |= BUADPR_PURGE;
		}
		else if (uh->uba_type&UBA780)		
			uh->uh_uba->uba_dpr[bdp] |= UBADPR_BNE;

		else if (uh->uba_type&UBA750)
			uh->uh_uba->uba_dpr[bdp] |=
				    UBADPR_PURGE|UBADPR_NXM|UBADPR_UCE;

		uh->uh_bdpfree |= 1 << (bdp-1);		/* atomic */
		if (uh->uh_bdpwant) {
			uh->uh_bdpwant = 0;
			wakeup((caddr_t)&uh->uh_bdpwant);
		}
	}
	/*
	 * Put back the registers in the resource map.
	 * The map code must not be reentered, so we do this
	 * at high ipl.
	 */
	npf = (mr >> 18) & 0x3ff;
	reg = ((mr >> 9) & 0x1ff) + 1;
	s = spl6();
	rmfree(uh->uh_map, (long)npf, (long)reg);
	splx(s);

	/*
	 * Wakeup sleepers for map registers,
	 * and also, if there are processes blocked in dgo(),
	 * give them a chance at the UNIBUS.
	 */
	if (uh->uh_mrwant) {
		uh->uh_mrwant = 0;
		wakeup((caddr_t)&uh->uh_mrwant);
	}
	while (uh->uh_actf && ubago(uh->uh_actf))
		;
}

ubapurge(um)
	register struct uba_ctlr *um;
{
	register struct uba_hd *uh = um->um_hd;
	register int bdp = (um->um_ubinfo >> 28) & 0x0f;


	if (uh->uba_type & UBABUA) {
		bdp = bdp & 0x07;
		((struct bua_regs *)uh->uh_uba)->bua_dpr[bdp] |= BUADPR_PURGE;
	}
	else if (uh->uba_type & UBA780)
		uh->uh_uba->uba_dpr[bdp] |= UBADPR_BNE;
	else if (uh->uba_type & UBA750)
		uh->uh_uba->uba_dpr[bdp]|=UBADPR_PURGE|UBADPR_NXM|UBADPR_UCE;
}

ubainitmaps(uhp)
	register struct uba_hd *uhp;
{

	rminit(uhp->uh_map, (long)NUBMREG, (long)1, "uba", UAMSIZ);

	if (uhp->uba_type&UBABUA) 
		uhp->uh_bdpfree = (1<<NBDP_BUA) - 1;	
	else if(uhp->uba_type & UBA780)	
		uhp->uh_bdpfree = (1<<NBDP8600) - 1;
	else if(uhp->uba_type & UBA750)
		uhp->uh_bdpfree = (1<<NBDP750) - 1;
}

/*
 * Generate a reset on uba number uban.  Then
 * call each device in the character device table,
 * giving it a chance to clean up so as to be able to continue.
 */
ubareset(uban)
	int uban;
{
	register struct cdevsw *cdp;
	register struct uba_hd *uh = &uba_hd[uban];
	int s;

	s = spl6();
	uh->uh_users = 0;
	uh->uh_zvcnt = 0;
	uh->uh_xclu = 0;
	uh->uh_actf = uh->uh_actl = 0;
	uh->uh_bdpwant = 0;
	uh->uh_mrwant = 0;
	ubainitmaps(uh);
	wakeup((caddr_t)&uh->uh_bdpwant);
	wakeup((caddr_t)&uh->uh_mrwant);

	if (uh->uba_type & UBABDA) {
		printf("bda%d: reset",uban);
	}
	else if (uh->uba_type & UBAXMI) {
		printf("kdm%d: reset",uban);
	}
	else {
		printf("uba%d: reset", uban);
		ubainit(uh->uh_uba,uh->uba_type);
	}

	/* reallocate global unibus space for tty drivers */
	if (tty_ubinfo[uban] != 0)
		tty_ubinfo[uban] = uballoc(uban, (caddr_t)cfree,
	    		nclist*sizeof (struct cblock), 0);

	for (cdp = cdevsw; cdp < cdevsw + nchrdev; cdp++)
		(*cdp->d_reset)(uban);
#ifdef INET
	ifubareset(uban);
#endif
	printf("\n");
	splx(s);
}

/*
 * Init a uba.  This is called with a pointer
 * rather than a virtual address since it is called
 * by code which runs with memory mapping disabled.
 * In these cases we really don't need the interrupts
 * enabled, but since we run with ipl high, we don't care
 * if they are, they will never happen anyways.
 */
ubainit(uba,ubatype)
	register int ubatype;
	register struct uba_regs *uba;

{
	extern struct ssc_regs *ssc_ptr;

	if (ubatype&UBA780) {
		uba->uba_cr = UBACR_ADINIT;
		uba->uba_cr = UBACR_IFS|UBACR_BRIE|UBACR_USEFIE|UBACR_SUEFIE;
		while ((uba->uba_cnfgr & UBACNFGR_UBIC) == 0)
			;
	}
	else if (ubatype&UBABUA) 
		buainit(uba);
#ifdef vax
	else if (ubatype&(UBA750|UBA730|UBAUVI)) {
		mtpr(IUR, 0);
		/* give devices time to recover from power fail */
		DELAY(500000);
	}
#endif vax
	else if (ubatype&UBAUVII) {
#define LMEA 0x20			/* Local memory access enable	*/
#define QIPCR 0x1f40			/* Q-bus Inter-processor csr	*/
		/*
		 * Reset the bus and wait for the devices to
		 * settle down
		 */
#ifdef vax
		mtpr(IUR, 0);
#endif vax
#ifdef mips
		ssc_ptr->ssc_ioreset = 0;  /* equiv to mtpr(IUR, 0) */
#endif mips
		DELAY(500000);
		/*
		 * The bus reset turns off the q-bus map (unfortunately)
		 * The problem is further agravated by the fact that the
		 * enable bit is in the IPC register which is in I/O space
		 * instead of local register space.  Because of this we
		 * have to figure out if we're virtual or physical.
		 */
#ifdef vax
		if( mfpr(MAPEN) & 0x1 ) {
			/*
			 * Virtual
			 */
			*(u_short *)((char *)qmem+QMEMSIZEUVI+QIPCR) = LMEA;
		} else {
			/*
			 * Physical
			 */
			*(u_short *)((char *)QDEVADDRUVI+QIPCR) = LMEA;
		}
#endif vax
#ifdef mips
		/*
		 * We are on a Qbus mips machine.  The bus reset we just
		 * did turned off the Qbus map.  We need to allow
		 * external access to Qbus memory space via the Qbus map.
		 * Since on a mips we always have memory management,
		 * we access the IPCR (implemented in the CQBIC chip) 
		 * through Kseg 1 space
		 *
		 * IPCR is a 16-bit register located in offset
		 * 0x1f40 from the Q bus I/O space.  
		 */
		*(u_short *)((char *)qmem+QMEMSIZEUVI+QIPCR) = LMEA;
#endif mips

	}
}


int	ubawedgecnt = 10;
int	ubacrazy = 500;
/*
 * This routine is called by the locore code to
 * process a UBA error on an 11/780.  The arguments are passed
 * on the stack, and value-result (through some trickery).
 * In particular, the uvec argument is used for further
 * uba processing so the result aspect of it is very important.
 * It must not be declared register.
 */
/*ARGSUSED*/
ubaerror(uban, uh, xx, uvec, uba, ubapc)
	register int uban;
	register struct uba_hd *uh;
	int uvec;
	register struct uba_regs *uba;
	int *ubapc;
{
	register int sr, s;
	struct el_rec *elrp;

	/*
	 *	Start a timer to time the rate of zero vectors.
	 *	The counting is done in locore.s.
	 */

	if (uvec == 0) {
		if (uh->uh_zvflg)
			mprintf("ubaerror: zero vector flag shouldn't be set\n");
		else {
			uh->uh_zvcnt++;
			uh->uh_zvflg++;
			timeout(ubatimer, uban, hz * zvintvl);
		}
		return;
	}
	if (uh->uba_type&UBABUA) {
		sr = ((struct bua_regs *)uh->uh_uba)->bua_ctrl;
		s = spl7();
		printf("bua%d: bua error ctrl=%x",uban,sr); 
		splx(s);
		((struct bua_regs *)uh->uh_uba)->bua_ctrl = sr;
		ubareset(uban);
	}
	else if ((uh->uba_type&UBABDA)==0) {	


		if (uba->uba_cnfgr & NEX_CFGFLT) {
			elrp = ealloc(EL_UBASIZE,EL_PRILOW);
			if (elrp != NULL) {
			    LSUBID(elrp,ELCT_ADPTR,ELADP_UBA,EL_UNDEF,EL_UNDEF,uban,EL_UNDEF);
		 	    elrp->el_body.eluba780.uba_cf = uba->uba_cnfgr;
		 	    elrp->el_body.eluba780.uba_cr = uba->uba_cr;
			    elrp->el_body.eluba780.uba_sr = uba->uba_sr;
		 	    elrp->el_body.eluba780.uba_dcr = uba->uba_dcr;
		 	    elrp->el_body.eluba780.uba_fmer = uba->uba_fmer;
			    elrp->el_body.eluba780.uba_fubar = uba->uba_fubar;
		 	    elrp->el_body.eluba780.uba_pc = *ubapc;
		 	    elrp->el_body.eluba780.uba_psl = *++ubapc;
			    EVALID(elrp);
			}	
			else {
				cprintf("uba%d: sbi fault sr=%b cnfgr=%b\n",
				    uban, uba->uba_sr, ubasr_bits,
				    uba->uba_cnfgr, NEXFLT_BITS);
			}
			ubareset(uban);
			uvec = 0;
			return;
		}
		sr = uba->uba_sr;
		s = spl7();
		elrp = ealloc(EL_UBASIZE,EL_PRILOW);
		if (elrp != NULL) {
		    LSUBID(elrp,ELCT_ADPTR,ELADP_UBA,EL_UNDEF,EL_UNDEF,uban,EL_UNDEF);
	 	    elrp->el_body.eluba780.uba_cf = uba->uba_cnfgr;
	 	    elrp->el_body.eluba780.uba_cr = uba->uba_cr;
		    elrp->el_body.eluba780.uba_sr = uba->uba_sr;
	 	    elrp->el_body.eluba780.uba_dcr = uba->uba_dcr;
	 	    elrp->el_body.eluba780.uba_fmer = uba->uba_fmer;
		    elrp->el_body.eluba780.uba_fubar = uba->uba_fubar;
	 	    elrp->el_body.eluba780.uba_pc = *ubapc;
	 	    elrp->el_body.eluba780.uba_psl = *++ubapc;
		    EVALID(elrp);
		}	
		else {
			cprintf("uba%d: uba error sr=%b fmer=%x fubar=%o\n",
			    uban, uba->uba_sr, ubasr_bits, uba->uba_fmer, 4*uba->uba_fubar);
		}
		splx(s);
		uba->uba_sr = sr;
		uvec &= UBABRRVR_DIV;
	}
	if (++uh->uh_errcnt % ubawedgecnt == 0) {
		if (uh->uh_errcnt > ubacrazy)
			panic("uba crazy");
		printf("ERROR LIMIT ");
		ubareset(uban);
		uvec = 0;
		return;
	}
	return;
}

/*
 * Allocate UNIBUS memory.  Allocates and initializes
 * sufficient mapping registers for access.  On a 780,
 * the configuration register is setup to disable UBA
 * response on DMA transfers to addresses controlled
 * by the disabled mapping registers.
 */
ubamem(uban, addr, npg, doalloc)
	int uban, addr, npg, doalloc;
{
	register struct uba_hd *uh = &uba_hd[uban];
	register int a;

	if (doalloc) {
		int s = spl6();
		a = rmget(uh->uh_map, npg, (addr >> 9) + 1);
		splx(s);
	} else
		a = (addr >> 9) + 1;
	if (a) {
		register int i, *m;

		m = (int *)&uh->uh_uba->uba_map[a - 1];

		for (i = 0; i < npg; i++)
			*m++ = 0;	/* All off, especially 'valid' */
		/*
		 * On a 780 and 8600, set up the map register disable
		 * field in the configuration register.  Beware
		 * of callers that request memory ``out of order''.
		 */
		if (uh->uba_type & UBA780) {
			int cr = uh->uh_uba->uba_cr;

			i = (addr + npg * 512 + 8191) / 8192;
			if (i > (cr >> 26))
				uh->uh_uba->uba_cr |= i << 26;
		}
	}
	return (a);
}

#ifdef vax
#include "ik.h"
#if NIK > 0
/*
 * Map a virtual address into users address space. Actually all we
 * do is turn on the user mode write protection bits for the particular
 * page of memory involved.
 */
maptouser(vaddress)
	caddr_t vaddress;
{

	Sysmap[(((unsigned)(vaddress))-0x80000000) >> 9].pg_prot = (PG_UW>>27);
}

unmaptouser(vaddress)
	caddr_t vaddress;
{

	Sysmap[(((unsigned)(vaddress))-0x80000000) >> 9].pg_prot = (PG_KW>>27);
}
#endif
#endif vax

/*
 *  Check the number of zero vectors and report if we get too many of them.
 *  Always reset the zero vector count and the zero vector timer flag.
 */

ubatimer(uban)
int	uban;
{
	struct uba_hd *uh;

	uh = &uba_hd[uban];
	if(uh->uh_zvcnt > zvthresh)
		mprintf("ubatimer: uba%d -- %d zero vectors in %d minutes\n",
			uban, uh->uh_zvcnt, zvintvl/60);
	uh->uh_zvcnt = 0;
	uh->uh_zvflg = 0;
}

#ifdef vax
/*
 * vs_bufctl is the locking mechanism that allows the VAXSTAR disk
 * tape controllers to share a common I/O buffer.
 *
 * This routine MUST be called from spl5.
 */
struct vsbuf vsbuf = { 0, 0, 0 };

vs_bufctl(vsdev)
struct vsdev *vsdev;
{
    register struct vsbuf *vs = &vsbuf;
    register int rval;
    struct vsdev *temp;
    int action;
    int s;

    action = vsdev->vsd_action;

    for (;;) {
	switch (action) {
	    case VS_DEALLOC:
		if (vs->vs_active == 0)
		    panic("vs_bufctl: VS_DEALLOC: no owner");
		if (vs->vs_wants) {
		    if (vs->vs_wants->vsd_id != vs->vs_active->vsd_id) {
			vs->vs_active = vs->vs_wants;
			vs->vs_status = vs->vs_active->vsd_id;
			vs->vs_wants = VS_IDLE;
		    }
		    else {
			panic("vs_bufctl: VS_DEALLOC: wanted by owner");
			return;
		    }
		}
		else {
			vs->vs_active = VS_IDLE;
			vs->vs_status = VS_IDLE;
			return;
		}
		break;

	    case VS_ALLOC:
		if (vs->vs_active == 0) {
		    vs->vs_active = vsdev;
		    vs->vs_status = vsdev->vsd_id;
		}
		else {
			vs->vs_wants = vsdev;
			return;
		}
		break;

	    case VS_KEEP:
		return;
		break;
	  
	    case VS_WANTBACK:
		if (vs->vs_wants) {
		    if(vs->vs_active == 0) {
			panic("vs_bufctl: VS_WANTBACK: not active");
		    }
		    temp = vs->vs_active;
		    vs->vs_active = vs->vs_wants;
		    vs->vs_status = vs->vs_active->vsd_id;
		    vs->vs_wants = temp;
		    vs->vs_wants->vsd_action = VS_ALLOC;
		}
		break;

	    default:
		panic("vs_bufctl: unknown action");
		break;
	}

	if (vs->vs_active != 0) {
	    action = (*vs->vs_active->vsd_funcptr)(); /* call to stc or sdc driver */
	    if(action == VS_ALLOC) {
		panic("vs_bufctl: illegal VS_ALLOC returned");
	    }
	    vs->vs_active->vsd_action = action;
	}
	else {
	    panic("vsbufctl: active pointer null");
	    return;
	}
    } /* forever */
}

#endif vax

/*
 * Allocate and setup Q-BUS map registers, and bdp's
 * Flags says whether bdp is needed, whether the caller can't
 * wait (e.g. if the caller is at interrupt level).
 *
 * Return value:
 *	Bits 0-8	Byte offset 		512  number of bytes
 *	Bits 9-21	Start map reg. no.	all the 8192 regs
 *	Bits 22-31	No. mapping reg's allocated, divided by 8
 */
qbasetup(uban, bp, flags)
	struct buf *bp;
{
	register struct uba_hd *uh = &uba_hd[uban];
	int npf, reg, fake_npf;
	unsigned v;
	register struct pte *pte, *io;
	struct proc *rp;
	int a, o, ubinfo, vax_pfn;
	static int first_time = 0;
	int user_addr = 0;
#ifdef mips
	int	mips_of;		  /* offset within a mips page	*/
	int	mips_pfn;		  /* PFN of a mips pte		*/
#endif mips

	flags &= ~UBA_NEEDBDP;

	/* The first time through to get map registers allocate 512 of them
	 * and never use them or give them back.  This will allow any calls
	 * from a users driver to uballoc to get these without conflicts with 
	 * the ones this routine controls.
	 * This is done for backward compatability.
	 */
	if( first_time == 0) {
		first_time = 1;
		rmalloc(uh->uq_map, (long)(btoc(QBNOTUB * NBPG) + 1));
	}
	/* Find the page, offset into the page and the number of vax
	 * page frames that will be needed.  This code will work for mips
	 * also because we will simulate vax pfn's when infact one mips
	 * pfn is equal to eight (8) vax pfn's.
	 */
	v = btop(bp->b_un.b_addr);
	o = (int)bp->b_un.b_addr & 0x1ff; /* offset within a VAX page */
#ifdef mips
	/* offset within a mips page */
	mips_of = (int)bp->b_un.b_addr & PGOFSET;
#endif mips

	/* 
	 * Number of VAX pages spanned by the buffer + 1(hence number of
	 * map registers needed since we need one guard page)
	 */
	npf = (((unsigned)(bp->b_bcount + o)+0x1ff) >> 9) + 1; 

	/* get regs in groups of 8 to allow the ubinfo word to
	 * hold all the information.  This may be a waste of maps but
	 * it gets us closer to using all 8K maps on a Q22 bus, we need
	 * to do this because we can't get all the information into
	 * the returned integer. Sigh!
	 *
	 * Also MIPS uses 1 map where VAX looks for 8 so if its on
	 * a MIPS system fake_npf is really the vax maps needed.
	 *
	 */
	fake_npf = (npf + (8 - (npf % 8)));

	/* Allocate the map registers for use by calling rmalloc
	 */
	a = spl6();
	while ((reg = rmalloc(uh->uq_map, (long)fake_npf)) == 0) {
		if (flags & UBA_CANTWAIT) {
			splx(a);
			return (0);
		}
		uh->uh_mrwant++;
		sleep((caddr_t)&uh->uh_mrwant, PSWP);
	}
	splx(a);

	/* Setup the return value which holds the map register number
	 * and related info.
	 */
	reg--;
	ubinfo = ((reg & 0x1fff) << 9) | o;
	ubinfo |= ((fake_npf /8) << 22);
	/* Now load the map registers with the pfn data.  This is not a 
	 * straight forward task.  The pfn data is different on the vax and 
	 * mips systems AND the mips sometimes uses its pfn's and othertimes
	 * it uses physical.  The following code fragment will set up the
	 * map register based on this data.
	 */
	/* running process */
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	pte = 0;
	if ((bp->b_flags & B_PHYS) == 0) {	/* Not Physical */
#ifdef vax
		pte = &Sysmap[btop(((int)bp->b_un.b_addr)&0x7fffffff)];
#else	
		/* mips KSEG Addressing */
	        if(IS_KSEG0(bp->b_un.b_addr)){ 
			/* unmapped KSEG areas */
                	mips_pfn = btop(K0_TO_PHYS(bp->b_un.b_addr));
	        } else if(IS_KSEG1(bp->b_un.b_addr)){
			/* unmapped KSEG areas */
                	mips_pfn = btop(K1_TO_PHYS(bp->b_un.b_addr));
	        } else if(IS_KSEG2(bp->b_un.b_addr)){
			/* mapped KSEG areas */
	                pte = &Sysmap[btop(bp->b_un.b_addr - K2BASE)];
        	}
#endif vax
	}
	else if (bp->b_flags & B_UAREA)		/* User area */
		pte = &rp->p_addr[v];
	else if (bp->b_flags & B_PAGET)		/* Page Table */
		pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
	else if ((bp->b_flags & B_SMEM)  &&	/* SHMEM */
					((bp->b_flags & B_DIRTY) == 0))
		pte = ((struct smem *)rp)->sm_ptaddr + v;
	else {
		pte = vtopte(rp, v);
		user_addr++;
	}


	if ((uh->uba_type & UBAUVI) ==0 || (flags&UBA_MAPANYWAY)) {
		/* get address of starting UNIBUS map register */
		io = &uh->uh_uba->uba_map[reg];

                /* Loop to load all the map registers with pte information.
                 * For a mips we need to simulate vax pte structures.
		 *
		 * On a VAX, we have npf map registers to fill, each
		 * map register is to be filled by a VAX pte.
		 *
		 * On a mips, we have npf map registers to fill, but
		 * each mips page is 4KB, so we have fewer mips ptes
		 * to use.
		 *
		 * The following loop iterates for each map register.
                 */
		while (--npf != 0) {
#ifdef mips
			if(pte != 0)
			{	/* pte points to the mips pte */
				if (pte->pg_pfnum == 0)
					panic("uba zero uentry");
				vax_pfn = (pte->pg_pfnum << 3) + 
					  (mips_of / 0x200);
			} else {
				/* There are no pte's, use pfn's
				 */
				vax_pfn = (mips_pfn << 3) +
					  (mips_of / 0x200);
			}
			*(int *)io++ = vax_pfn | UBAMR_MRV;

			mips_of += 0x200; /* next vax page */
			if (mips_of >= 0x1000) {
				/* Crossing mips page boundary */
				/* Go on to next mips page */
				mips_of -= 0x1000;
				if (pte) 
					pte++;
				else
					mips_pfn++;
			}
#else
			if (user_addr &&
			    (((int)pte & PGOFSET) < CLSIZE*sizeof(struct pte)
			    || pte->pg_pfnum == 0))
				pte = vtopte(rp, v);
			if (pte->pg_pfnum == 0)
				panic("uba zero uentry");
			*(int *)io++ = pte++->pg_pfnum | UBAMR_MRV;
			v++;
#endif mips
		}
		*(int *)io++ = 0;
		WBFLUSH;
	} else	{
		ubinfo = contigphys( ubinfo, bp->b_bcount, pte);
		++reg;
		a = spl6();
		rmfree( uh->uq_map, (long)fake_npf, (long)reg);
		splx(a);
	}

	return (ubinfo);
}

/*
 * Non buffer setup interface... set up a buffer and call ubasetup.
 */
qballoc(uban, addr, bcnt, flags)
	int uban;
	caddr_t addr;
	int bcnt, flags;
{
	struct buf qbbuf;

	qbbuf.b_un.b_addr = addr;
	qbbuf.b_flags = B_BUSY;
	qbbuf.b_bcount = bcnt;
	/* that's all the fields qbasetup() needs */
	return (qbasetup(uban, &qbbuf, flags));
}
 
 
/*
 * Release resources on uba uban, and then unblock resource waiters.
 * The map register parameter is by value since we need to block
 * against uba resets on 11/780's.
 */
qbarelse(uban, amr)
	unsigned *amr;
{
	register struct uba_hd *uh = &uba_hd[uban];
	register int  reg, npf, s;
	unsigned mr;
 
	/*
	 * Carefully see if we should release the space, since
	 * it may be released asynchronously at uba reset time.
	 */
	s = spl6();
	mr = *amr;
	if (mr == 0) {
		/*
		 * A ubareset() occurred before we got around
		 * to releasing the space... no need to bother.
		 */
		splx(s);
		return;
	}
	*amr = 0;
	splx(s);		/* let interrupts in, we're safe for a while */

	/*
	 * Put back the registers in the resource map.
	 * The map code must not be reentered, so we do this
	 * at high ipl.
	 */
	npf = (mr >> 22) * 8;
	reg = (((mr >> 9) & QBREGMASK) + 1);
	s = spl6();
	rmfree(uh->uq_map, (long)npf , (long)reg);
	splx(s);

	/*
	 * Wakeup sleepers for map registers,
	 * and also, if there are processes blocked in dgo(),
	 * give them a chance at the UNIBUS.
	 */
	if (uh->uh_mrwant) {
		uh->uh_mrwant = 0;
		wakeup((caddr_t)&uh->uh_mrwant);
	}
	while (uh->uh_actf && ubago(uh->uh_actf))
		;
}
