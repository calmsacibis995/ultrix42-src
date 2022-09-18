#ifndef lint
static char *sccsid = "@(#)autoconf.c	4.2	(ULTRIX) 9/11/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Modification History: autoconf.c
 *
 * 10-Sep-90  Charles Richmond  IIS Corp.
 *	Moved the printing of a new line inorder to fix a bug that caused
 *	SCSI devices on the Q bus to print the vendor ID fields at the
 *	start of the next line.
 *
 * 10-Aug-89 -- afd
 *	In ib_config_dev, added "ui" (ptr to uba_device struct) as a parameter
 *	to probe routine.  This is for systems that allow more than one
 *	of a given device type.
 *
 *	In ib_config_conf, added "um" (ptr to uba_ctlr struct) as a parameter
 *	to probe routine.  This is for systems that allow more than one
 *	of a given controller type.
 *
 * 20-Feb-89 -- Kong
 *	Changed ib_config_dev to set up the phyaddr field.
 *	Added Unibus, Qbus support here.
 *
 * 01-Feb-89 -- Kong
 *	Moved over VAX-like configuration routines.
 *
 * 13-Jan-89 -- Kong
 *	Moved pmax specific configuration routines to kn01.c.  Configuration
 *	is called through the system switch table.
 *
 * 09-Nov-88 -- afd
 *	Examine the "systype" from the PROM & log what we are (in configure()).
 *
 * 06-Sep-88 -- afd
 *	Changed autoconfiguration messages from "cprintf" to "printf".
 *
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
#include "../h/config.h"
#include "../h/kmalloc.h"

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../machine/debug.h"
#include "../machine/cpu.h"
#include "../machine/fpu.h"
#include "../machine/hwconf.h"
#include "../machine/scb.h"
#include "../../machine/common/cpuconf.h"

extern int nNUBA;
extern int nNKDB;
extern int nNKLESIB;
extern int nNKDM;

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		/* if 1, still working on cold-start */
int	dkn;		/* number of iostat dk numbers assigned so far */
int	autoconf_cvec;	/* global for interrupt vector from probe routines */
int	autoconf_br;	/* global for IPL from probe routines */
int	(*autoconf_intr)();	/* interrupt handler */
u_int	autoconf_csr;	/* csr base address in k1seg */

/*
 * cache sizes
 */
unsigned icache_size;
unsigned dcache_size;

/*
 * coprocessor revision identifiers
 */
unsigned cputype_word;		/* PRId word */
unsigned fptype_word;
extern unsigned cpu_systype;	/* systype word in boot PROM */

struct config_adpt *ni_port_adpt;

struct imp_tbl cpu_imp_tbl[] = {
	{ "MIPS R2000 Processor Chip",			0 },
	{ "MIPS R2000 Processor Chip",			1 },
	{ 0,						0 }
};

struct imp_tbl fp_imp_tbl[] = {
	{ "MIPS R2360 Floating Point Board",		1 },
	{ "MIPS R2010 VLSI Floating Point Chip",	2 },
	{ 0,						0 }
};

coproc_find()
{
	char *imp_name();
	union rev_id ri;
	extern char mfc0_start[], mfc0_end[];

	hwconf.cpu_processor.ri_uint = ri.ri_uint = cputype_word = 
								get_cpu_irr();
	if (ri.ri_imp == 0) {
		ri.ri_majrev = 1;
		ri.ri_minrev = 5;
	}
	printf("cpu0 ( version %d.%d, implementation %d )\n",
	    ri.ri_majrev, ri.ri_minrev,ri.ri_imp);
#ifdef oldmips
#ifndef SABLE
	/*
	 * Make sure the PROBE_BUG option and the mfc0 assembler option
	 * are turned on for the old 1.5 chips.
	 */
	if (ri.ri_majrev < 2) {
#ifndef PROBE_BUG
		panic("Kernel must be compiled with -DPROBE_BUG for 1.5 revision cpu chip");
#endif !PROBE_BUG
		if ((mfc0_end - mfc0_start) <= 4)
			panic("Kernel must be assembled with -Wb,-mfc0 for 1.5 revision cpu chip");
	}
#endif !SABLE
	
	/*
	 * TODO:
	 *	check cpu_config register
	 *	print a message about vme or local memory
	 */
#ifdef SABLE
	hwconf.cpubd_config = CONFIG_NOCP1|CONFIG_NOCP2|CONFIG_POWERUP;
#else !SABLE
	hwconf.cpubd_config = *(char *)PHYS_TO_K1(CPU_CONFIG);
#endif SABLE
#endif oldmips

#ifdef oldmips
/*	This is out....if you don't have floating point you may hang */
	if (hwconf.cpubd_config & CONFIG_NOCP1) {
		fptype_word = 0;
		hwconf.fpu_processor.ri_uint = 0;
		printf("No floating point processor\n");
	} else {
		hwconf.fpu_processor.ri_uint =
#endif oldmips
		ri.ri_uint = fptype_word = get_fpc_irr();
		fptype_word &= IRR_IMP_MASK;
		printf("fpu0 ( version %d.%d, implementation %d )\n",
	    		ri.ri_majrev, ri.ri_minrev,ri.ri_imp);
		fp_init();
#ifdef oldmips
	}
#endif oldmips
}

char *
imp_name(ri, itp)
struct rev_id ri;
struct imp_tbl *itp;
{
	for (; itp->it_name; itp++)
		if (itp->it_imp == ri.ri_imp)
			return(itp->it_name);
	return("Unknown implementation");
}

int fpu_inited;

fp_init()
{
	int led;

	wbflush();
	DELAY(10);
	wbflush();
	DELAY(10);
	set_fpc_csr(0);
	fpu_inited++;
}

/*
 * Cpu board serial number
 */
machineid()
{
#ifdef TODO
	fill this in based on cpu board id ??
#endif TODO
	return (123);
}

/*
 * Name:	ib_config_cont (nxv, nxp, slot, name, scb_vec_addr);
 *
 * Args:	nxv	- The virtual address of the "controller"
 *
 *		nxp	- The physicall address of the "controller"
 *
 *		slot	- The mbus slot number containing the "controller"
 *
 *		name	- The name of the "controller" to match with in the
 *			  ubminit and ubdinit structures
 *
 *		scb_vec_addr - The offset from the begining of scb block
 *				 zero that you want the address of the 
 *				 interrupt routine specified in the um
 *				 structure inserted.  If the value equals
 *				 zero, do not insert the the address of the
 *				 interrupt routine into the scb.
 *
 * Returns:	1 - if the "controller" was found
 *		0 - if the "controller" wasn't found
 */
ib_config_cont(nxv, nxp, slot, name, scb_vec_addr)
char *nxv;
char *nxp;
u_long slot;
char *name;
int scb_vec_addr;
{
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	register struct uba_driver *udp;
	register struct config_adpt *p_adpt;
	extern struct config_adpt config_adpt[];
	int (**ivec)();
	int i;
	int found = 0;

	um = ubminit;
	while (found == 0 && (um->um_driver)) {
	    if ((um->um_adpt == slot) && (strcmp(um->um_ctlrname, name) == 0) &&
		(um->um_alive == 0) && (um->um_ubanum == -1)) {
		    found = 1;
	    }
	    else {
	    	if ((um->um_adpt == '?') && (strcmp(um->um_ctlrname, name) == 0) &&
		    (um->um_alive == 0) && (um->um_ubanum == -1)) {
			found = 1;
		}
		else {
		    um++;
		}
	    }
	}

	if (found == 0) {
	    return(0);
	}
	 

	udp = um->um_driver;
	for( p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {

	/* first look for iobus entry for this board then
	 * check if the driver is correct for this board then
 	 * check that the controller number is the same as the
	 * iobus entry then
	 * check that this iobus entry has not been used
	 */
			
	    if (strcmp("ibus", p_adpt->p_name)==0 && 
		(char *)udp == p_adpt->c_name &&
		p_adpt->c_num == um->um_ctlr  &&
		p_adpt->c_ptr == 0) {
		    found = 1;
	    }
	}

	if (found == 0) {
	    return(0);
	}
/* DAD - do this here for now.  Need to set up the vector since
 * sz_siiprobe goes and looks for devices
 */
	if (scb_vec_addr)
	    ibcon_vec(scb_vec_addr, um);

	i = (*udp->ud_probe)(nxv, um);
	if (i == 0)
	    return(0);
	um->um_adpt = slot;
	um->um_alive = (i ? 1 : 0);
	um->um_addr = (char *)nxv;
	um->um_physaddr = (char *)svtophy(um->um_addr);
	udp->ud_minfo[um->um_ctlr] = um;
	config_fillin(um);
	printf("\n");


	for (ui = ubdinit; ui->ui_driver; ui++) {
	    if (ui->ui_driver != udp || ui->ui_alive ||
		ui->ui_ctlr != um->um_ctlr && ui->ui_ctlr != '?') {
		    continue;
	    }
	    if ((*udp->ud_slave)(ui, nxv)) {
		ui->ui_alive = 1;
		ui->ui_ctlr = um->um_ctlr;
		ui->ui_addr = (char *)nxv;
		ui->ui_ubanum = um->um_ubanum;
		ui->ui_hd = um->um_hd;
		ui->ui_physaddr = nxp;
		ui->ui_adpt = slot;

		if (ui->ui_dk && dkn < DK_NDRIVE)
		    ui->ui_dk = dkn++;
		else
		    ui->ui_dk = -1;
		ui->ui_mi = um;
		/* ui_type comes from driver */
		udp->ud_dinfo[ui->ui_unit] = ui;
		if(ui->ui_slave >= 0) {
		    printf("%s%d at %s%d slave %d",
			ui->ui_devname, ui->ui_unit,
			udp->ud_mname, um->um_ctlr, ui->ui_slave);
		}
		else {
		    printf("%s%d at %s%d",
			ui->ui_devname, ui->ui_unit,
			udp->ud_mname, um->um_ctlr);
		}
		(*udp->ud_attach)(ui);
		printf("\n");
	    }
	}
	return (found);
}

/*
 * Name:	ib_config_dev (nxv, nxp, slot, name, scb_vec_addr);
 *
 * Args:	nxv	- The virtual address of the "device"
 *
 *		nxp	- The physical address of the "device"
 *
 *		slot	- The ibus slot number containing the "device"
 *
 *		name	- The name of the "device" to match with in the
 *			  ubminit and ubdinit structures
 *
 *		scb_vec_addr - The offset from the begining of scb block
 *				 zero that you want the address of the 
 *				 interrupt routine specified in the um
 *				 structure inserted.  If the value equals
 *				 zero, do not insert the the address of the
 *				 interrupt routine into the scb.
 *
 * Returns:	1 - if the "device" was found
 *		0 - if the "device" wasn't found
 */
ib_config_dev(nxv, nxp, slot, name, scb_vec_addr)
char *nxv;
char *nxp;
u_long slot;
char *name;
int scb_vec_addr;
{
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	register struct uba_driver *udp;
	register struct config_adpt *p_adpt;
	extern struct config_adpt config_adpt[];
	int (**ivec)();
	int i;
	int found = 0;
	
	ui = ubdinit;
	while (found == 0 && (ui->ui_driver)) {
	    if ((ui->ui_adpt == slot) && (strcmp(ui->ui_devname, name) == 0) &&
		(ui->ui_alive == 0) && (ui->ui_slave == -1)) {
		        found = 1;
	    }
	    else {
		if ((ui->ui_adpt == '?') && (strcmp(ui->ui_devname, name) == 0) &&
		    (ui->ui_alive == 0) && (ui->ui_slave == -1)) {
		        found = 1;
	    	}
		else {
		    ui++;
		}
	    }
	}

	if (found == 0)
	    return(0);
	udp = ui->ui_driver;
	i = (*udp->ud_probe)(nxv, ui);
	if (i == 0) {
	    return(0);
	}
	ui->ui_adpt = slot;
	config_fillin(ui);
	printf("\n");
	if (scb_vec_addr)
	    ibdev_vec(scb_vec_addr, ui);
	ui->ui_alive = (i ? 1 : 0);
	ui->ui_addr = (char *)nxv;
	ui->ui_physaddr = (char *)nxp;
	ui->ui_dk = -1;
	udp->ud_dinfo[ui->ui_unit] = ui;
	(*udp->ud_attach)(ui);
	return (found);
}

/*
 * ibdev_vec(): To set up Mbus device interrupt vectors.
 * It is called with 3 parameters:
 *	slot	   - The ibus slot number
 *	scb_vec_addr - The offset from the start of slot specific
		     vector space
 *	ui:	   - the device structure (for names of interrupt routines)
 */

ibdev_vec(scb_vec_addr, ui)
int scb_vec_addr;
struct uba_device *ui;
{
    register int (**ivec)();
    register int (**addr)();	/* double indirection neccessary to keep
			   	   the C compiler happy */

    ivec = ui->ui_intr;
    addr = (int (**)())scb_vec_addr;
    *addr = scbentry(*ivec,SCB_ISTACK);
}

ibcon_vec(scb_vec_addr, um)
int scb_vec_addr;
struct uba_ctlr *um;
{
    register int (**ivec)();
    register int (**addr)();	/* double indirection neccessary to keep
			   	   the C compiler happy */
    ivec = um->um_intr;
    addr = (int (**)())scb_vec_addr;
    *addr = scbentry(*ivec,SCB_ISTACK);
return;
}

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
int *debug;
#ifdef vax
	extern int catcher[256];
#endif vax
#ifdef mips
	extern int stray();	/* Stray interrupts catcher for mips */
#endif mips
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

		/*
		 * Fill in the page of SCB for unibus interrupt
		 * vectors.
		 */
		for (i = 0; i < (uhp->uh_lastiv/4); i++)
			uhp->uh_vec[i] =
#ifdef vax
			    scbentry(&catcher[i*2], SCB_ISTACK);
#endif vax
#ifdef mips
			    scbentry(stray,0);
#endif mips

	}

	/*
	 * Map the adapter memory and the i/o space. For unibuses the io space
	 * is the last 8k of the adapter memory. On q-bus it's totally 
	 * disjoint.
	 * We map the i/o space right after the adapter memory space so that 
	 * its easy to compute the virtual addresses.
	 */
	if ((uhp->uba_type&UBABDA)==0) {   /* BDA does not have dev space! */
#ifdef vax
		ubaaccess(pumem, memmap, umemsize, PG_V|PG_KW);
		ubaaccess(pdevaddr, memmap+btop(umemsize), DEVSPACESIZE, 
			PG_V|PG_KW);
#endif vax
#ifdef mips
		/* 
		 * Mapping the pages to be Global, non-cached, valid,
		 * Dirty, and Kernel read/write.
		 */
		ubaaccess(pumem, memmap, umemsize, PG_V|PG_N|PG_M|PG_G|PG_KW);
		ubaaccess(pdevaddr, memmap+btop(umemsize), DEVSPACESIZE, 
			PG_V|PG_N|PG_M|PG_G|PG_KW);
#endif mips
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
#ifdef vax
	if( cpu != MVAX_I )
#endif vax
	{
		KM_ALLOC(tempio,caddr_t,1024, KM_TEMP, KM_CLEAR | KM_NOWAIT);
		if(tempio == (caddr_t)0 )
			panic("no mem for probe i/o");
		ubinfo = uballoc( numuba, tempio, 1024, 0);
		if( ubinfo & 0x3ffff ) {
			panic("probe i/o space not at bus virtual address 0");
		}
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
		if (uhp->uba_type&UBABDA)
			if (addr>0x0ff) continue;		
		
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
#ifdef vax
		printf("vec %o, ipl %x\n", cvec, br);
#else
		printf("vec %o\n", cvec);
#endif vax
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
				printf("%s%d at %s%d slave %d ",
				    ui->ui_devname, ui->ui_unit,
				    udp->ud_mname, um->um_ctlr, ui->ui_slave);
				(*udp->ud_attach)(ui);
				printf("\n"); /*Moved \n from above CMR*/
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
#ifdef vax
		printf("vec %o, ipl %x\n", cvec, br);
#else
		printf("vec %o\n", cvec);
#endif vax
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
#ifdef vax
	if( cpu != MVAX_I )
#endif vax
	{
		KM_FREE(tempio, KM_TEMP);
		ubarelse(numuba, &ubinfo);
	}
}

/*
 * Make unibus address space accessible.
 */
ubaaccess(pumem, pte, umemsize, mode)
	caddr_t pumem;
	register struct pte *pte;
	int umemsize;
	unsigned int mode;
{
	register int i = btop(umemsize);
	register unsigned v = btop(pumem);
	
	do {
#ifdef vax 
		*(int *)pte++ = mode|v++;
#endif vax
#ifdef mips
		*(int *)pte++ = (v++ << PTE_PFNSHIFT) | mode;
#endif mips
}
	while (--i > 0);
#ifdef vax
	mtpr(TBIA, 0);
#endif vax
}

/*
 * Configure swap space and related parameters.
 */
#ifndef ultrix
swapconf()
{
	register struct swdevt *swp;
	register int nblks = 0;

	for (swp = swdevt; swp->sw_dev; swp++) {
		if (bdevsw[major(swp->sw_dev)].d_psize)
			nblks =
			  (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
		if (swp->sw_nblks == 0 || swp->sw_nblks > nblks)
			swp->sw_nblks = nblks;
		printf("swap on dev 0x%x configured for %d blocks\n", 
			swp->sw_dev, swp->sw_nblks);
	}
	if (!cold)			/* in case called for TODO device */
		return;
	if (dumplo == 0 && bdevsw[major(dumpdev)].d_psize) {
		dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) - 
 				ctod(physmem) - btodb(BLKDEV_IOSIZE);
	}
	if (dumplo < 0)
		dumplo = 0;
}
#endif not ultrix
