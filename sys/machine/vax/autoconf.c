#ifndef lint
static char *sccsid = "@(#)autoconf.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87,88 by			*
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

/* ------------------------------------------------------------------------
 * Modification History:
 *
 * 20-Jul-89	rafiey (Ali Rafieymehr)
 *	Moved is_adapt_alive from biinit.c here. XMI and BI share
 *	this routine.
 *
 * 20-Jul-89 -- map (Mark A. Parenti)
 *	Map UBAXMI the same as UBABDA.
 *
 * 22-May-89 -- darrell
 *	changed cpup to be global -- as part of the new cpusw.
 *
 * 05-Nov-88 -- Robin
 *	Changed the size of the Q22-bus map register to not use all 8k
 *	registers.  This is necessary because the QDSS driver needs to
 *	use the top 1K maps directly.  By reducing the size by
 *	1K the registers should never be used at the same time.
 *
 * 28-June-1988 -- tresvik
 *	Move filling of installation specific rpb information from machdep
 *	to here.  This is necessary to get the	ws_display_type for the 
 *	LYNX which isn't filled in until lxprobe.
 *
 * 27-June-1988 -- larry
 *	Move appendflg to init_main. We flush print buffers after mscp
 *	driver has finished polling for units.
 * 01-June-1988	-- robin
 *	Make a change to get the correct adapter number printed for
 *	ibus entries.
 *
 * 24-May-1988  -- robin (for Larry C.)
 *	Moved setcache functions here from machdep, the I/O space needs
 *	to be mapped before setcache is called.
 *
 * 04-Feb-1988	-- jaw
 *	add a routine to check that an adapter has been configed.
 *
 * 08-Jan-1988	-- jaw
 *	fix to unifind to NOT allow the BLA (klesib) to match on 
 *	wildcarded unibus devices.
 *
 * 08-Jan-1988		Todd M. Katz
 *	The configuration adapter structure has been notified to include
 *	fields for bus and nexus numbers.  Add parameters to the function
 *	config_set_alive() so that these fields may be set and appropriately
 *	modify all invocations of this function within this module.
 *
 * 12-11-87	Robin L. and Larry C.
 *			Added portclass/kmalloc support to the system.
 *
 * 12-Aug-86   -- prs   Removed #if GENERIC compiler option around call
 *			to setconf. Now, setconf should always be called.
 *
 * 22-Jul-86   -- bjg	Change appendflg so ALL startup mesgs logged together
 *
 * 17-Jul-86   -- jaw	NEXUS is now lower case..
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 * 07-Jul-86   -- jaw	added alive bit for adapters for Mr. Installation.
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 * 10-Jun-86   -- jaw 	fixed the uba autoconf that I broke for the 780 and 
 *			8600.
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 17-Apr-86  -- jaw     re-write scbprot so it protects ALL of the scb.  This
 *			 also fixes the "lost" instack page bug.
 *
 * 09-Apr-86	Darrell Dunnuck
 *	Called badaddr via the macro BADADDR.
 *
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 * 10-Mar-86 -- darrell
 *	Moved probenexus to sbi.c and renamed it probesbi, moved probeioa
 *	to sbi.c, and moved machine dependent portions of configure to 
 *	the kaXXXX.c files. 
 *
 * 05-Mar-86 -- pmk  added appenflg for errlogging of printf msg.
 *
 * 05-Mar-86 -- jaw  VAXBI device and controller config code added.
 *		     todr code put in cpusw.
 *
 * 19-Feb-86 -- bjg  add sbi_there flag to be used later for error logging
 *		     set in probenexus.
 *
 * 18-Feb-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 03-Feb-86 -- jaw  changes to SCB vectoring scheme.
 *
 * 15-jul-85 -- jaw
 *	add support for VAX8800
 *
 * 22 Oct 85 -- tresvik
 *	fixed typo `ifdef VAX8600:' changed to `ifdef VAX8600'
 *
 * 25 Sep 85 -- tresvik
 *	Changed cpu identification message to make the distinction
 *	between the VAX 8600 and VAX 8650.
 *
 *  8 Aug 85 -- lp	
 *	Changed unifind routine to clear strays correctly on VAX8200.
 *	Also changed how wildcarded devices get seen by bda and bua
 *	(check for small addr values).
 *
 *  8 Aug 85 -- rjl
 *	The MicroVAX-I behavior of uballoc doesn't return a bus virtual
 *	address of zero. It also doesn't need this code due to the fact
 *	that none of the supported devices need to do I/O during probe
 *	so the code is conditionalized for it.
 *
 *  1 Aug 85 -- rjl
 *	The first page of memory was mapped directly to allow devices to do
 *	dma transfers in the probe routines.  This same map register was 
 *	also used by devices to map communications areas.  Fixed unifind to
 *	use uballoc like everyone else to avoid double mapping.
 *
 * 26-jul-85 -- jaw
 *	VAX8200 vector page allocation for unifind done in biinit.c.
 *
 * 11-jul-85 -- jaw
 *	fix bua/bda map registers.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 *  6 Jun 85 -- jaw
 * 	add support for the BDA.
 *
 *  6 Jun 85 -- rjl
 *	Set up memory systems on MicroVAXen. This was previously done
 *	in the boot code but needs to be here so that the bits set while
 *	probing I/O space get cleared properly.
 *
 * 20-Mar-85 -- jaw
 * 	Changes for the VAX8200
 *
 * 12-Mar-85 -- tresvik
 *	Clear SBIA error bits after autoconfig and enable SBI faults.
 *	Fix sizing of second SBIA.
 *
 *  9 Mar 85 -- rjl
 *	Changed MicroVAX selection of root floppy to be based on a relative
 *	unit number rather than a predetermined one.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 19 Nov 84 -- rjl
 * 	Generalized the tables that probenexus and unifind work from to
 *	support the MicroVAXen's notion of the various spaces. Generalized
 *	ubaacces to make it useful for the initing any set of pte's.
 *
 * 31 Aug 84 -- reilly
 *	Change the way the swap device(s) is handle because of the new
 *	disk partitioning scheme.  All of swapconf is no longer in here, it
 *	can now be found init_main.c
 * 13 Nov 84 -- jrs
 *	Add code to check for unconfigured UBA's to avoid panic.
 *
 * 11 Aug 84 -- rjl
 *	Merged all MicroVAX changes
 *
 *  3 Aug 84 -- rjl
 *	Fixed bug that caused a configuration with two or more of the same
 *	devices on a uba to configure missing devices using address of present
 *	devices.
 *
 * 14 Apr 84 -- rjl
 * 	Added check and release of contigous buffers.
 *
 * 12 Apr 84 -- rjl
 *	Added floating point mismatch warning. This check depends on a
 * 	related change in locore.s.
 *
 * 16 Feb 84 -- rjl
 *	Added support for dynamic configuration of the rootdevice to be
 *	the same as the boot device.
 *
 *  5 Jan 84 --jmcg
 *	This version has been tested on MicroVAX 1.  Having the physical
 *	address of the pseudo-nexus space makes things easier elsewhere.
 *
 *  2 Jan 84 -- jmcg
 *	Added support for MicroVAX 1.
 *
 *  2 Jan 84 --jmcg
 *	Derived from Ultrix-32 baseline sources 1.5; heritage is 4.2BSD,
 *	labeled:
 *		autoconf.c	6.3	83/08/11
 *
 * ------------------------------------------------------------------------
 */


/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the uba and mba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 *
 * N.B.: A lot of the conditionals based on processor type say
 *	#if VAX780
 * and
 *	#if VAX750
 * which may be incorrect after more processors are introduced if they
 * are unlike either of these machines.
 *
 * TODO:
 *	use cpusw info about whether a ubasr exists
 */

#include "../h/config.h"
#include "../data/autoconf_data.c"
#include "../h/kmalloc.h"
#include "../vax/rpb.h"
#include "../machine/sas/vmb.h"
/* save record of sbis present for sbi error logging for 780 and 8600 */
long	sbi_there = 0;	/* bits 0-15 for nexi,sbi0; 16-31 for nexi on sbi1*/
extern struct cpusw *cpup;	/* pointer to cpusw entry */

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		/* if 1, still working on cold-start */
int	ioanum = 0;	/* only greater than zero on an 8600 */
int	dkn;		/* number of iostat dk numbers assigned so far */

/*
 * Cache state (enable/disable)
 */
int	cache_state;
int	cache2_state;

/*
 * Determine mass storage and memory configuration for a machine.
 * Get cpu type, and then switch out to machine specific procedures
 * which will probe adaptors to see what is out there.
 */
configure()
{
	int *ip;
	extern int ws_display_type;
 	extern char Sysbase[];

	if((*cpup->config)(cpup) < 0) {
		printf("No config routine for cpu type %d\n", cpu);
		asm("halt");
	}
	scbprot();
	cold = 0;
	timer_action();
/*
 * Enable cache - (and floating point accelerator if necessary)
 */
	cachenbl();
	setcache (cache_state);

 /*
  * Plug in some variables needed by standalone environments to
  * be able to sense the configuration during installation.  THIS 
  * SEECTION OF CODE SHOULD DIE AND MOVE TO USE GETSYSINFO IN THE
  * FUTURE.
  */
	/*
	 * write enable the RPB
	 */
 	ip = (int *)Sysmap; *ip &= ~PG_PROT; *ip |= PG_KW;
 	mtpr(TBIS, Sysbase);	
	/*
	 * plug in the address of vmb_info
	 */
	rpb.vmbinfo = (long *)&vmb_info;
	/*
	 * plug in the cpu type and cpu subtype while the RPB 
	 * is still writeable
	 */
	rpb.cpu = cpu;
	rpb.cpu_subtype = cpu_subtype;
	/*
	 * plug in the ws_display_type - this must be done after
	 * graphic cons_init routines are run where it is filled in
	 */
	rpb.ws_display_type = ws_display_type;
	/*
	 * write protect the RPB again
	 */
 	ip = (int *)Sysmap; *ip &= ~PG_PROT; *ip |= PG_KR;
 	mtpr(TBIS, Sysbase);	

	return;
}

/*
 * Write protect the scb and UNIBUS interrupt vectors.
 * It is strange that this code is here, but this is
 * as soon as we are done mucking with it, and the
 * write-enable was done in assembly language
 * to which we will never return.
 */

scbprot()
{
	register int *ip,i;
	extern char scbend;
	char *scbptr;

	ip = (int *)Sysmap + (btop(((int) &scb.scb_stray) & 0x7fffffff));

	for(scbptr = (char *) &scb.scb_stray; scbptr != (char *) &scbend; 
		scbptr += NBPG) {
		*ip &= ~PG_PROT; *ip |= PG_KR;
		mtpr(TBIS, scbptr);
		ip++; 
	}
	setconf();
}

ubacsrcheck(vubp,uhp) 
struct uba_hd *uhp;
char *vubp;

{
#if defined(VAX780) || defined(VAX8600) || defined(VAX8200) || defined(VAX8800)
	if (uhp->uba_type & UBABUA) {
		if (((struct bua_regs *)vubp)->bua_ctrl & BUACR_ERR) {
			((struct bua_regs *)vubp)->bua_ctrl = ((struct bua_regs *)vubp)->bua_ctrl;
			return(1);
		}

	}
	else {
		((struct uba_regs *)vubp)->uba_cr = UBACR_IFS|UBACR_BRIE;
		 if (((struct uba_regs *)vubp)->uba_sr) {
			((struct uba_regs *)vubp)->uba_sr = ((struct uba_regs *)vubp)->uba_sr;
			return(1);
		}
	}
#endif
	return(0);
}

/*
 * Find devices on a UNIBUS.
 * Uses per-driver routine to set <br,cvec> 
 * and then fills in the tables, with help from a per-driver
 * slave initialization routine.
 */
unifind(vubp, pubp, vumem, pumem, umemsize, pdevaddr, memmap, haveubasr,adpt_num,nexus_num)
	char *vubp, *pubp, *vumem, *pumem, *pdevaddr;
	struct pte *memmap;
	int umemsize;
	short haveubasr;
	int	adpt_num;
	int	nexus_num;
{
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	u_short *reg, *ap, addr;
	register struct uba_hd *uhp;
	register struct uba_driver *udp;
	int i, (**ivec)();
	caddr_t ualloc;
	extern int catcher[256];
	caddr_t tempio;
	int ubinfo,savectlr;	


	/*
	 * Initialize the UNIBUS, by freeing the map
	 * registers and the buffered data path registers
	 */
	uhp = &uba_hd[numuba];
	KM_ALLOC(uhp->uh_map, struct map *, UAMSIZ*sizeof(struct map), KM_RMAP, KM_CLEAR|KM_NOWAIT);
	ubainitmaps(uhp);
	KM_ALLOC(uhp->uq_map, struct map *, QAMSIZ*sizeof(struct map), KM_RMAP, KM_CLEAR|KM_NOWAIT);
	rminit(uhp->uq_map, (long)QBMREG - 1024, (long)1, "qba", QAMSIZ);

	/*
	 * Save virtual and physical addresses
	 * of adaptor, and allocate and initialize
	 * the UNIBUS interrupt vector.
	 */
	uhp->uh_uba = (struct uba_regs *) vubp;
	uhp->uh_physuba = (struct uba_regs *)pubp;

	/* set up vector if one hasn't been setup yet */
	if (numuba < nNUBA) {
		/* set the UBA alive in case empty */
		config_set_alive("uba", numuba, numuba, -1);

#if defined(VAX8600)
		if( cpu == VAX_8600)
			uhp->uh_vec = SCB_UNIBUS_PAGE((numuba+1));
		else
#endif VAX8600
			uhp->uh_vec = SCB_UNIBUS_PAGE(numuba);


		/*
		 * Set last free interrupt vector for devices with
		 * programmable interrupt vectors.  Use is to decrement
		 * this number and use result as interrupt vector.
		 */
		uhp->uh_lastiv = 0x200;

		for (i = 0; i < (uhp->uh_lastiv/4); i++)
			uhp->uh_vec[i] =
			    scbentry(&catcher[i*2], SCB_ISTACK);
	}

	/*
	 * Map the adapter memory and the i/o space. For unibuses the i/o space
	 * is the last 8k of the adapter memory. On q-bus it's totally disjoint.
	 * We map the i/o space right after the adapter memory space so that 
	 * its easy to compute the virtual addresses.
	 */
	if ((uhp->uba_type&(UBABDA|UBAXMI))==0) {   /* BDA and XMI do not have dev space! */
		ubaaccess(pumem, memmap, umemsize, PG_V|PG_KW);
		ubaaccess(pdevaddr, memmap+btop(umemsize), DEVSPACESIZE, PG_V|PG_KW);
	}
	/* clear uba csr */
	if (haveubasr) ubacsrcheck(vubp,uhp);

	/*
	 * Grab some memory to record the umem address space we allocate,
	 * so we can be sure not to place two devices at the same address.
	 *
	 * We could use just 1/8 of this (we only want a 1 bit flag) but
	 * we are going to give it back anyway, and that would make the
	 * code here bigger (which we can't give back), so ...
	 *
	 * One day, someone will make a unibus with something other than
	 * an 8K i/o address space, & screw this totally.
	 * When that happens we should add a new field to cpusw for it.
	 */
	KM_ALLOC(ualloc, caddr_t, DEVSPACESIZE, KM_TEMP, KM_CLEAR | KM_NOWAIT);
	if (ualloc == (caddr_t)0)
		panic("no mem for unifind");

	/*
	 * Map the first page of BUS i/o space to the first page of memory
	 * for devices which will need to dma output to produce an interrupt.
	 */
	if( cpu != MVAX_I ) {
		KM_ALLOC(tempio, caddr_t, 1024, KM_TEMP, KM_CLEAR | KM_NOWAIT);
		if(tempio == (caddr_t)0 )
			panic("no mem for probe i/o");

		if( (ubinfo = uballoc( numuba, tempio, 1024, 0)) & 0x3ffff )
			panic("probe i/o space not at bus virtual address 0");
	}

#define	ubaoff(off)	((off)&0x1fff)
#define	ubaddr(off)	(u_short *)((int)vumem + umemsize +(ubaoff(off)))
	/*
	 * Check each unibus mass storage controller.
	 * For each one which is potentially on this uba,
	 * see if it is really there, and if it is record it and
	 * then go looking for slaves.
	 */
	for (um = ubminit; udp = um->um_driver; um++) {
		if (uhp->uba_type&UBABLA) {
		      if (um->um_ubanum != numuba || um->um_alive) continue;
		}  else if (um->um_ubanum != numuba && um->um_ubanum != '?'||
			um->um_alive)
			continue;
		addr = (u_short)um->um_addr;
		
		/* check for VAXSTAR driver (allow 18 bits addr) */
		if (((int)um->um_addr) & 0xfffc0000) continue;

		/*
		 * use the particular address specified first,
		 * or if it is given as "0", of there is no device
		 * at that address, try all the standard addresses
		 * in the driver til we find it
		 */
	    for (ap = udp->ud_addr; addr || (addr = *ap++); addr = 0) {

		if (uhp->uba_type&(UBABDA|UBAXMI))
			if (addr>0x0ff) continue;		

		/* clear uba csr */
		if (haveubasr) ubacsrcheck(vubp,uhp);

		if (ualloc[ubaoff(addr)])
				continue;
		reg = ubaddr(addr);
		if (BADADDR((caddr_t)reg, 2)) continue;

		if (haveubasr && ubacsrcheck(vubp,uhp)) continue;
		cvec = 0x200;
		i = (*udp->ud_probe)(reg, um->um_ctlr);
		if (haveubasr && ubacsrcheck(vubp,uhp)) continue;
		if (i == 0) continue;

		um->um_ubanum = numuba;
		um->um_adpt   = adpt_num;
		um->um_nexus  =	nexus_num;
		config_fillin(um);
		printf(" csr %o ",addr);

		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		while (--i >= 0)
			ualloc[ubaoff(addr+i)] = 1;
		printf("vec %o, ipl %x\n", cvec, br);
		um->um_alive = 1;
		um->um_hd = &uba_hd[numuba];
		um->um_addr = (caddr_t)reg;
		um->um_physaddr = (char *)svtophy(um->um_addr);
		udp->ud_minfo[um->um_ctlr] = um;
		for (ivec = um->um_intr; *ivec; ivec++) {
			um->um_hd->uh_vec[cvec/4] =
			    scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if (ui->ui_driver != udp || ui->ui_alive ||
			    ui->ui_ctlr != um->um_ctlr && ui->ui_ctlr != '?'||
			    ui->ui_ubanum != numuba && ui->ui_ubanum != '?')
				continue;
			savectlr = ui->ui_ctlr;
			ui->ui_ctlr = um->um_ctlr;
			
			if ((*udp->ud_slave)(ui, reg)) {
				ui->ui_alive = 1;
				ui->ui_ctlr = um->um_ctlr;
				ui->ui_ubanum = numuba;
				ui->ui_adpt   = adpt_num;
				ui->ui_nexus  =	nexus_num;
				ui->ui_hd = &uba_hd[numuba];
				ui->ui_addr = (caddr_t)reg;
				ui->ui_physaddr = pdevaddr + ubaoff(addr);
				if (ui->ui_dk && dkn < DK_NDRIVE)
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
			else ui->ui_ctlr = savectlr;
		}
		break;
	    }
	}

	if ((uhp->uba_type &(UBABDA|UBABLA|UBAXMI)) == 0) 
	/*
	 * Now look for non-mass storage peripherals.
	 */
	for (ui = ubdinit; udp = ui->ui_driver; ui++) {
		if (ui->ui_ubanum != numuba && ui->ui_ubanum != '?' ||
		    ui->ui_alive || ui->ui_slave != -1)
			continue;
		addr = (u_short)ui->ui_addr;
		/* check for VAXSTAR driver (allow 18 bits addr) */
		if (((int)ui->ui_addr) & 0xfffc0000) continue;

	    for (ap = udp->ud_addr; addr || (addr = *ap++); addr = 0) {
		if (haveubasr) ubacsrcheck(vubp,uhp);;
		if (ualloc[ubaoff(addr)])
			continue;
		reg = ubaddr(addr);
		if (BADADDR((caddr_t)reg, 2))
			continue;
		if (haveubasr && ubacsrcheck(vubp,uhp)) continue;
		cvec = 0x200;
		i = (*udp->ud_probe)(reg);
		if (haveubasr && ubacsrcheck(vubp,uhp)) continue;
		if (i == 0)
			continue;
	
		ui->ui_adpt   = adpt_num;
		ui->ui_nexus  =	nexus_num;
		ui->ui_ubanum = numuba;
		config_fillin(ui);
		printf(" csr %o ", addr);

		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec %o, ipl %x\n", cvec, br);
		while (--i >= 0)
			ualloc[ubaoff(addr+i)] = 1;
		ui->ui_hd = &uba_hd[numuba];
		for (ivec = ui->ui_intr; *ivec; ivec++) {
			ui->ui_hd->uh_vec[cvec/4] =
			    scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		ui->ui_alive = 1;
		ui->ui_addr = (caddr_t)reg;
		ui->ui_physaddr = pdevaddr + ubaoff(addr);
		ui->ui_dk = -1;
		/* ui_type comes from driver */
		udp->ud_dinfo[ui->ui_unit] = ui;
		(*udp->ud_attach)(ui);
		break;
	    }
	}

#ifdef	AUTO_DEBUG
	printf("Unibus allocation map");
	for (i = 0; i < 8*1024; ) {
		register n, m;

		if ((i % 128) == 0) {
			printf("\n%6o:", i);
			for (n = 0; n < 128; n++)
				if (ualloc[i+n])
					break;
			if (n == 128) {
				i += 128;
				continue;
			}
		}

		for (n = m = 0; n < 16; n++) {
			m <<= 1;
			m |= ualloc[i++];
		}

		printf(" %4x", m);
	}
	printf("\n");
#endif

	/*
	 * Free resources.  We free the bus map register but it's unlikely
	 * that it will ever be used again due to the fact that it only 
	 * maps two pages.
	 */
	KM_FREE(ualloc, KM_TEMP);
	if( cpu != MVAX_I ){ 
		KM_FREE(tempio, KM_TEMP);
		ubarelse(numuba, &ubinfo);
	}
}

setscbnex(fn,nexnum)
	int (*fn)();
	int nexnum;
{
	register struct scb *scbp = &scb;
	register num = nexnum % 16;
	
	scbp = (struct scb *) ((int)scbp + (0x200 * ioanum));
	scbp->scb_ipl14[num] = scbp->scb_ipl15[num] =
	    scbp->scb_ipl16[num] = scbp->scb_ipl17[num] =
		scbentry(fn, SCB_ISTACK);
}

/*
 * Make a nexus accessible at physical address phys
 * by mapping kernel ptes starting at pte.
 *
 * WE LEAVE ALL NEXI MAPPED; THIS IS PERHAPS UNWISE
 * SINCE MISSING NEXI DONT RESPOND.  BUT THEN AGAIN
 * PRESENT NEXI DONT RESPOND TO ALL OF THEIR ADDRESS SPACE.
 */
nxaccess(physa, pte, nexsize)
	struct nexus *physa;
	register struct pte *pte;
	int nexsize;
{
	register int i = btop(nexsize);
	register unsigned v = btop(physa);
	
	do
		*(int *)pte++ = PG_V|PG_KW|v++;
	while (--i > 0);
	mtpr(TBIA, 0);
}

ubaaccess(pumem, pte, umemsize, mode)
	caddr_t pumem;
	register struct pte *pte;
	int umemsize;
	unsigned int mode;
{
	register int i = btop(umemsize);
	register unsigned v = btop(pumem);
	
	do
		*(int *)pte++ = mode|v++;
	while (--i > 0);
	mtpr(TBIA, 0);
}

extern struct config_adpt config_adpt[];
struct config_adpt *ni_port_adpt;

config_fillin(um) 
register struct uba_ctlr *um;
{
	register struct config_adpt *aptr;
	register int bitype = 0;
	register int xmitype = 0;

	for( aptr = &config_adpt[0]; aptr->p_name;  aptr++) {

		if (aptr->c_name == ((char *)um->um_driver) && 
		    um->um_ctlr == aptr->c_num &&
		    ((aptr->c_type == 'C') || (aptr->c_type == 'D'))) {

			aptr->c_ptr = (caddr_t) um;

			/* if conected to NEXUS...done */
			if (strcmp(aptr->p_name,"nexus") == 0) return;

			if (strcmp(aptr->p_name,"ibus") == 0) {
				aptr->p_num = um->um_adpt; 
			}

			if (strcmp(aptr->p_name,"vaxbi") == 0) {
				aptr->p_num = um->um_adpt; 
				bitype = 1;
			}
			if (strcmp(aptr->p_name,"xmi") == 0) {
				aptr->p_num = um->um_adpt; 
				xmitype = 1;
			}
			if (strcmp(aptr->p_name,"uba") == 0) {
				aptr->p_num = um->um_ubanum; 
			}			
			if (strcmp(aptr->p_name,"aie") == 0) {
				ni_port_adpt = aptr;
			}			
			config_find_adpt(aptr,um); 
			printf("%s%d at %s%d", 
					um->um_ctlrname, aptr->c_num,
					aptr->p_name, aptr->p_num);
			if (bitype|xmitype)
				printf(" node %d",um->um_nexus);
		}

	}

}

is_adapt_configured(name, number)
char *name;
int number;
{
	register struct config_adpt *p_adpt;
	for( p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {
		    if ((strcmp(p_adpt->c_name,name) == 0) &&
		    	(p_adpt->c_num == number)) {
			return(1);
		    }
	}
	return(0);
}


config_set_alive(name, number, busnum, nexusnum)
char *name;
int number;
int busnum;
int nexusnum;
{
	register struct config_adpt *p_adpt;
	for( p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {
		    if ((strcmp(p_adpt->c_name,name) == 0) &&
		    	(p_adpt->c_num == number)) {
			p_adpt->c_ptr = (caddr_t) CONFIG_ALIVE;
			p_adpt->c_bus_num = busnum;
			p_adpt->c_nexus_num = nexusnum;
			return;
		    }
	}
}

config_find_adpt(c_adpt,um) 
register struct uba_ctlr *um;
register struct config_adpt *c_adpt;

{
	register struct config_adpt *p_adpt;
	register int bitype = 0;
	register int xmitype = 0;

	for( p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {
		if (p_adpt->c_type == 'A' &&
		    (strcmp(p_adpt->c_name,c_adpt->p_name) == 0) &&
		    (p_adpt->c_num == c_adpt->p_num)) {

			/* if conected to NEXUS...done */
			if ((strcmp(p_adpt->p_name,"nexus") == 0) ||
			   (strcmp(p_adpt->c_name,"uba") == 0)) return;

			if (strcmp(p_adpt->p_name,"uba") == 0) {
				p_adpt->p_num = um->um_ubanum; 
			}
			if (strcmp(p_adpt->p_name,"vaxbi") == 0) {
				bitype = 1;
				p_adpt->p_num = um->um_adpt; 
			}
			if (strcmp(p_adpt->p_name,"xmi") == 0) {
				xmitype = 1;
				p_adpt->p_num = um->um_adpt; 
			}
			if (strcmp(p_adpt->p_name,"ibus") == 0) {
				p_adpt->p_num = um->um_adpt; 
			}
			config_find_adpt(p_adpt,um);
			if (!(p_adpt->c_ptr)) {
				p_adpt->c_ptr = (caddr_t) CONFIG_ALIVE;
				printf("%s%d at %s%d", 
					p_adpt->c_name,	p_adpt->c_num,
					p_adpt->p_name, p_adpt->p_num);
			
				if (bitype|xmitype)
					printf(" node %d \n",um->um_nexus);
				else
					printf("\n");
			}
			return;
		}
	}		
}	


strcmp(s,t) 
register char *s, *t;
{
	while (*s == *t++) {
		if (*s++ == '\0'){
			return(0);
		}
	}
	return(1);
}



/* this routine matches a um or ui structure with the proper adapter
   structure and returns whether it is alive or not. This routine
   is called by both BI and XMI */

is_adapt_alive(um) 
register struct uba_ctlr *um;
{
	register struct config_adpt *aptr;
	register int bitype = 0;
 	register struct config_adpt *p_adpt;

	for( aptr = &config_adpt[0]; aptr->p_name;  aptr++) {

	    if (aptr->c_name == ((char *)um->um_driver) && 
		um->um_ctlr == aptr->c_num &&
		((aptr->c_type == 'C') || (aptr->c_type == 'D'))) {

		if (strcmp(aptr->p_name,"aie")==0) {
   		    for(p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {
           	 	if (p_adpt->c_type == 'A' &&
                    	   (strcmp(p_adpt->c_name,aptr->p_name) == 0) &&
                    	   (p_adpt->c_num == aptr->p_num)) {
			    if (p_adpt->c_ptr) return(1);
			    else return(0);   	
			}
		    }
		} else {
		    if (aptr->c_ptr)  return(1);
		    else return(0);   	
		}
	    }

	}
	return(0);
}
