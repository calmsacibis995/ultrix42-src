#ifndef lint
static char *sccsid = "@(#)vbainit.c	4.9	(ULTRIX)	4/4/91";
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
 *	VMEbus adapters and to perform configuration of VMEbus devices.
 *
 * Revision History
 *
 *	18-Mar-91	map -- Mark A. Parenti
 *		Fix initialization of I/O space address maps.
 *
 *	26-Feb-91	map -- Mark A. Parenti
 *		Check return value from rmalloc in vme_map_csr() and return
 *		if unable to allocate map registers.
 *		Modify check of address space to account for boundary
 *		conditions.
 *
 *	19-Feb-91	map -- Mark A. Parenti
 *		Enable/disable block mode DMA based on the board rev
 *		level. See comment for more information.
 *
 *	22-Jan-91	map -- Mark A. Parenti
 *		Make selection of address space mapping dependent on
 *		setting in vbadata structure.  The DMA PMRs can be mapped
 *		into either the 1st or 2nd GB of VMEbus address space.
 *
 *	03-Jan-91	map -- Mark A. Parenti
 *		Fix initialization of ui_ and um_priority levels(used for
 *		splx).
 *
 *	15-Nov-90	Ron Bruckman, DEIC
 *		Changes to initialization logic
 *		1) Check clock for MVIB
 *		2) Clear interrupts from CSR during init
 *		3) Initialize "B" board registers
 *		4) Enable interrupts
 *
 *	12-Oct-90	map -- Mark A. Parenti
 *		Fix various hardware initialization bugs.
 *		Use correct data size definitions for hardware registers.
 *		Add priority to config print message.
 *		Add various startup error messages.
 *
 *	31-Aug-90	map -- Mark A. Parenti
 *		Add vba_get_vmeaddr() routine.
 *		Complete interrupt initialization.
 *		Various fixes from debug.
 *
 *	10-Apr-90	map -- Mark A. Parenti
 *		Change getvba() and insvba() to work with vba_hd pointers.
 *		Added initialization of various registers.
 *		Restructured module layout.
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
#endif /* vax */
#include "../machine/nexus.h"
#include "../machine/scb.h"

#include "../io/xmi/xmireg.h"
#include "../io/xmi/xbireg.h"
#include "../io/vme/xviareg.h"
#include "../io/vme/xvibreg.h"
#include "../io/vme/vbareg.h"
#include "../io/vme/vbavar.h"
#include "../io/uba/ubavar.h"

extern int (*VMEvec[])();
#ifdef vax
extern int catcher[256];
#endif /* vax */
#ifdef mips
extern int	stray();
#endif /* mips */
extern int nNVBA;
int vecvme = 0;
int nvme_config = 0;

extern struct cpusw *cpup;	/* pointer to cpusw entry */
extern	int		cpu;
extern	int		splm[];
extern	struct vba_hd *head_vba;
extern	struct	vbadata	vbadata;
extern	int	vbavec;
extern struct config_adpt config_adpt[];
extern	int	MVIBerrors(), XBIAerrors();
extern struct bus_dispatch vbaerr_dispatch[];
extern	int	passive_release();
extern	int	dkn;
int	vme_init_maps();
u_long	vme_map_csr();

extern int	vmedebug;		/* Used to trigger debug printf's	*/

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

#ifdef mips
#define WBFLUSH	wbflush()		/* Flush write buffer on mips */
#else
#define WBFLUSH ;			/* NOP for vax	*/
#endif /* mips */

#define	DELAY200MS	200		/* Delay 200MS with a 1MS loop */
char	*vme_spaces[3][3] = {
	{"vmea16d08", "vmea24d08", "vmea32d08"},
	{"vmea16d16", "vmea24d16", "vmea32d16"},
	{"vmea16d32", "vmea24d32", "vmea32d32"}
};
struct vba_hd *get_vba(vbanumber) 
int vbanumber;
{
	register struct vba_hd *vhp;

	vhp = head_vba;

	while(vhp) {
		if(vhp->vbanum == vbanumber) {
			return(vhp);
		}
		vhp = vhp->next;
	}
	panic("vba: no vba_hd");
	/*NOTREACHED*/
}

int	ins_vba(vhp)
struct vba_hd	*vhp;
{
struct	vba_hd	*vhp_ptr;

	vhp_ptr = head_vba;

	while(vhp_ptr) {
		if(vhp_ptr->next == 0)
			break;
		vhp_ptr = vhp_ptr->next;
	}
        if(vhp_ptr) {
		vhp_ptr->next = vhp;
	}
        else {
		head_vba = vhp;
	}
}

probevba(vhp)
struct vba_hd *vhp;
{
	register int i;
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register struct uba_driver *udp;
	register struct xbi_reg *xbireg;
        register struct config_adpt *p_adpt;
	register int (**vecaddr)(), (**ivec)(), (**intr_dispatch)();
	struct xmidata *xmidata;
	int ser, vbatype, vec, intr_bit, vecidx;
	unsigned long map_addr, map_addr2, vdcr, vvor, vcsr, viacsr, csr;
	volatile unsigned char *reg;
	int alive, savectlr, pmrsize, rev;
	u_short addr;
	int 	*io;

	Cprintf("probevba: Entered vhp = 0x%x\n", vhp);
	vhp->adptnum = nvme_config;
	vhp->vbavec_page = VMEvec + (nvme_config * 0x400);

	vbatype = vhp->vba_type;
	switch(vbatype) {
	      case VBA_3VIA:
		vhp->vba_err = MVIBerrors;
		viacsr = (VIACSR_ENAB_ADP | VBA_ADPT_VEC | VIACSR_MVIB_RST);
		if (cpu == DS_5000_100)
			viacsr |= (VIACSR_32MB_IO | VIACSR_MODEL100);
		if (BADADDR(Xviaregs.viacsr, 4)) {
			printf("vba%d not configured: Unable to access 3VIA\n",
			       vhp->vbanum);
			return(0);
		}
		/* check MVIB clock */
		vcsr = *Xviaregs.viacsr;
		if (vcsr & VIACSR_YAB_NCLK)
			{
			printf("vba%d not configured: No clock, check cable\n",
			       vhp->vbanum);
			return(0);
			}		

		*Xviaregs.viacsr = viacsr;
		WBFLUSH;
		if (BADADDR(Xviaregs.csr, 4)) {
			printf("vba%d not configured: Unable to access MVIB\n",
			       vhp->vbanum);
			return(0);
		}
		for(i=0; i < DELAY200MS; i++) {
			DELAY(1000)    
			csr = *Xviaregs.csr;
			if(csr & CSR_VME_SYSRST)
				break;
		}
		if (i == DELAY200MS) {
			printf("vba%d not configured: SYSRST Timeout\n",
			       vhp->vbanum);
			return(0);
		}
		if (csr & CSR_VME_ACLOW)
			printf("vba%d: AC LOW Indicated - check backplane\n",
			       vhp->vbanum);

		/* check VIA CSR for errors */
		vcsr = *Xviaregs.viacsr;
		
		if (vcsr & VIACSR_ERR_MSK)
		{
		/* clear status register of interrupts */
		*Xviaregs.viaclr = VIACLR_CLR_ALL;
		WBFLUSH;
		/* check if it worked */
		DELAY(1000)
		vcsr = *Xviaregs.viacsr;
		if (vcsr & VIACSR_ERR_MSK);
			{
			printf("vba%d not configured: CSR Error\n",vhp->vbanum);
			return(0);
			} 
		}

		/*setup B board here */

		csr = (CSR_DMA | CSR_VME_RESET	| CSR_MVIB_RST);
		if(vbadata.asc)
			csr |= CSR_SEL_UDMA;
		else
			csr |= CSR_SEL_LDMA;
		*Xviaregs.csr = csr;
		*Xviaregs.arcr = (u_char)(vbadata.vme_brl << ARCR_BR_SHIFT);
		*Xviaregs.ttr = (u_char)(vbadata.arb_to << TTR_VMETO_SHIFT) |
			(vbadata.arb_to << TTR_LBTO_SHIFT);
		*Xviaregs.rcr = (u_char)(vbadata.release);
		*Xviaregs.lvb = (u_char)0x08;
		*Xviaregs.besr = (u_char)0x00;
		*Xviaregs.icfr = (u_char)0x04;  /*metastability interval*/

		/* Read B-board revision from CSR		*/
		csr = *Xviaregs.csr;
		rev = (csr & CSR_REV) >> 16;

		/* Revisions 0-3 have special meaning.  Due to a bug in */
		/* the VIC chip, block mode DMA can only be enabled for */
		/* one address space.  A later revision of the VIC is   */
		/* expected to fix the problem.  If the rev is 0, both  */
		/* address spaces are disabled. This shouldn't be set   */
		/* so print a message and enable both.			*/
		/* If revision is 1, then enable A24 block mode.  If 	*/
		/* revision is 2, then enable A32 block mode.  If 	*/
		/* revision is 3 or greater then enable both.		*/
		/* The revision is set via jumpers on the B-board.	*/
		/* If a jumper is installed, the bit will be read as	*/
		/* a 0 (typical hardware backwards logic!!).		*/

		switch (rev) {
		      case 0:
			printf("vba%d: Invalid block mode jumper settings\n",
			       vhp->vbanum);
			printf("\tA24 and A32 block mode DMA enabled\n");
			*Xviaregs.s0c0 = (u_char)0x16;	/*enables A24 blk */
			*Xviaregs.s1c0 = (u_char)0x12;  /*enables A32 blk */
			break;

		      case 1:
			*Xviaregs.s0c0 = (u_char)0x16;	/*enables A24 blk */
			*Xviaregs.s1c0 = (u_char)0x10;  /*disables A32 blk*/
			break;

		      case 2:
			*Xviaregs.s0c0 = (u_char)0x14;	/*disables A24 blk*/
			*Xviaregs.s1c0 = (u_char)0x12;  /*enables A32 blk */
			break;

		      default:
			*Xviaregs.s0c0 = (u_char)0x16; /*enables A24 blk */
			*Xviaregs.s1c0 = (u_char)0x12;  /*enables A32 blk */
		}


		/* Initialize vector table to stray() catcher */
		for(i=0; i < NVME_VECS; i++)
			*(int (**)())_3VIA_VEC_ADDR(vhp, i) = stray;

		/* Setup vector 0 as passive release */
		intr_dispatch = (int (**)())_3VIA_VEC_ADDR(vhp, 0);
		*intr_dispatch = passive_release;

		/* Setup to handle 3VIA generated interrupts */
		for(vecidx=0; vbaerr_dispatch[vecidx].bus_num != 
		    vhp->vbanum ; vecidx++) {
			if (vbaerr_dispatch[vecidx].bus_num == -1) 
				panic("vba: no adapter error vector");
		}
		intr_dispatch = (int (**)())_3VIA_VEC_ADDR(vhp, 
					   VBA_ADPT_VEC);
		*intr_dispatch = (int (*)())vbaerr_dispatch[vecidx].bus_vec;

		/* Setup to handle MVIB generated interrupts */
		for(i = 0x9; i <= 0xF; i++) {
		       intr_dispatch = (int (**)())_3VIA_VEC_ADDR(vhp, 
					   i);
		       *intr_dispatch = (int (*)())vbaerr_dispatch[vecidx].bus_vec;
	       }
				
		/* Initialize map registers to invalid  */
		io = (int *)(Xviaregs.dma_pmr);
		for (i=0; i < vhp->n32dmapmr; i++)
			*(int *)io++ = 0;

		/* Setup interrupt handling.  If IPL level set in vbadata */
		/* then enable the corresponding register		  */
		reg = Xviaregs.icr;
		intr_bit = 1;
		for(i = 0; i < 7; i++) {
			if(intr_bit & vbadata.intr_mask)
				*reg = (u_char)ICR_IPL_2;
			reg += 4;  /* Can't just ++ here because registers */
			           /* are 8-bits on 32-bit boundaries.     */
			intr_bit <<= 1;
		}
	/* set local interrupts for parity, transaction pio and dma faults */

		reg = Xviaregs.licr;


		/* Enable local interrupts 1 - 3 */
		for(i = 0; i < 3; i++) {  
			*reg = (u_char) (ICR_IPL_2 | LICR_EDGE | LICR_VECTOR); 
			reg += 4; /* Can't just ++ here because registers */
			           /* are 8-bits on 32-bit boundaries.     */
		}
		/* Don't enable DMA Page Map error. Just let VMEbus */
		/* timeout occur.				    */
		*reg = (u_char)LICR_DISABLE;
		reg +=4;

		/* Enable local interrupts 5 - 7 */
		for(i = 5; i < 8; i++) { 
			*reg = (u_char) (ICR_IPL_2 | LICR_EDGE | LICR_VECTOR);
			reg +=4; /* Can't just ++ here because registers */
			         /* are 8-bits on 32-bit boundaries      */
		}

		*Xviaregs.errgi = (u_char)(ERRGI_ENABLE | ICR_IPL_2);
		WBFLUSH;


		/* initialize interrupts */
		vcsr = *Xviaregs.viacsr;
		*Xviaregs.viacsr = (vcsr | VIACSR_ENAB_INT);
		WBFLUSH;
		break;

	      case VBA_XBIA:
		vhp->vba_err = XBIAerrors;
		xbireg = (struct xbi_reg *)vhp->vbavirt;
		Cprintf("probevba: xbireg = 0x%x\n", xbireg);
		ser = xbireg->xbi_aesr;
		vdcr = *Xvibregs.vdcr;
    	        if (((ser & XBIA_IB_OK) == 0) || (vdcr & VDCR_ERR_SUM)) {
			return(0);
		}
	        /* clear out any errors generated during XBIA init */
		/* and setup XBIA+ registers			   */
	        xbireg->xbi_aesr = xbireg->xbi_aesr;
	        xbireg->xbi_xbe = xbireg->xbi_xbe & ~XMI_XBAD; /* write 1 to clear */
	        xbireg->xbi_besr = xbireg->xbi_besr; 

		xmidata = vhp->adapt_info.xbia_info.xmidata;
 	        xbireg->xbi_aimr = (XBIA_ENABLE_IVINTR |
				  XBIA_ENABLE_CC | /* corrected confirmaiton*/
				  XBIA_ENABLE_PE | /* parity error */
				  XBIA_ENABLE_WSE| /* write seq error */
				  XBIA_ENABLE_RDN| /* read data noack */
				  XBIA_ENABLE_WDN | /* write data noack */
				  XBIA_ENABLE_NRR | /* No read response */
				  XBIA_ENABLE_RSE | /* Read seq error */
				  XBIA_ENABLE_RER | /* Read error response */
				  XBIA_ENABLE_IBUS_APE |
				  XBIA_ENABLE_IBUS_BPE |
				  XBIA_ENABLE_IBUS_DPE |
				  XBIA_ENABLE_NXM); /* non-exist memory */

		xbireg->xbi_aivintr = xmidata->xmiintr_dst; 

		/* Setup PMR page size 	*/
#ifdef mips
		switch(cpu) {

		      case DS_5800:
		      default:
			pmrsize = XBIAP_DMA_4K;
			break;
		}
#endif /* mips */
#ifdef vax
		pmrsize = XBIAP_DMA_512;
#endif /* vax */
		xbireg->xbi_autlr |= pmrsize; 
		
		/* Setup XVIB registers		*/
		*Xvibregs.vcar = (VCAR_WRITE | ADSEL_ADD_ENAB | VCAR_ADD_SEL);
		vdcr = (vbadata.syscon) | 
			(vbadata.vme_brl << VDCR_BR_SHIFT) | 
				VDCR_PSIZE_8K;
		*Xvibregs.vdcr = vdcr;
		*Xvibregs.vicr = VICR_INIT_SET |
			(vbadata.intr_mask << VICR_IRQ_SHIFT);
#ifdef vax
		vvor =  ( ( ((int)vhp->vbavec_page) & ~VA_SYS)
			- ((int)&scb.scb_stray & ~ VA_SYS));
#endif /* vax */
#ifdef mips
		vvor = ( ((int)vhp->vbavec_page )
			- ((int)&scb.scb_stray));
#endif /* mips */
		*Xvibregs.vvor = vvor | (xmidata->xmiintr_dst << 16);
		*Xvibregs.vevr = (xmidata->xmiintr_dst << 16); 
		/* initialize SCB to catcher */
		for ( i=0 ; i < 256; i++)
			*(SCB_VME_ADDR(vhp->vbavec_page) + i) =
#ifdef vax
			    scbentry(&catcher[i*2], SCB_ISTACK);
#endif /* vax */
#ifdef mips
			    scbentry(stray, 0);
#endif /* mips */
		vecaddr = (int (**)())SCB_VME_VEC_ADDR(vhp->vbavec_page, 0); 
		*vecaddr = scbentry(passive_release, SCB_ISTACK);
		for(vecidx=0; vbaerr_dispatch[vecidx].bus_num != 
		    vhp->vbanum ; vecidx++) {
			if (vbaerr_dispatch[vecidx].bus_num == -1) 
				panic("vba: no adapter error vector");
		}
		vecaddr = (int (**)())SCB_VME_VEC_ADDR(vhp->vbavec_page, VBA_ADPT_VEC << 2); 
		*vecaddr = scbentry(vbaerr_dispatch[vecidx].bus_vec, SCB_ISTACK);
	
		break;

	      default:
		printf("vba%d: Unsupported adapter type\n", vhp->vbanum);
		return(0);
		break;
	}




	Cprintf("probevba: Init maps\n");
	/* Initialize resource maps for DMA, PIO, and VME memory space */
	if(vme_init_maps(vhp) == 0) {
		printf("vba%d not configured: Map initialization failed\n", 
		       vhp->vbanum);
		return(0);
	}	
	


	/*
	 * Search controller table for VMEbus controller devices,
	 * see if it is really there, and if it is record it and
	 * then go looking for slaves.
	 */
	for (um = ubminit; udp = um->um_driver; um++) {	
		if((um->um_alive) ||
		   (um->um_adpt != vhp->vbanum) ||
		   (um->um_vbanum == '?')) continue;
                for(p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {
			if(strcmp("vba", p_adpt->p_name)==0 &&
                       		(char *)udp == p_adpt->c_name &&
                       		p_adpt->c_num == um->um_ctlr) break;
		}
		if (p_adpt->p_name == 0)
			continue;
		map_addr = 0;
		map_addr2 = 0;
	        vec = um->um_ivnum;
		if((vec < 0x40) || (vec > 0xFF)) {
			printf("%s%d not configured: Invalid vector 0x%x\n", um->um_ctlrname, um->um_ctlr, vec);
			continue;
		}
		Cprintf("probevba: Found device\n");
		Cprintf("probevba: name = %s%d Addr1 = 0x%x  Addr2 = 0x%x\n",
			um->um_ctlrname, um->um_ctlr, 
			um->um_addr, um->um_addr2);

		/* Map csr space(s)			*/
	        /* Map csr1 address space if present	*/
		if(um->um_addr) {
			if ( (map_addr = vme_map_csr(um->um_addr, 
				    udp->ud_addr1_size,
				    udp->ud_addr1_atype,
				    udp,
				    um,
				    vhp)) == 0){
				continue;
			}
		}

		
		/* Map csr2 address space if present	*/
		if(um->um_addr2) {
			if( (map_addr2 = vme_map_csr(um->um_addr2, 
				    udp->ud_addr2_size,
				    udp->ud_addr2_atype,
				    udp,
				    um,
				    vhp)) == 0) {
				continue;
			}
		}

		i = (*udp->ud_probe)(um->um_ctlr, map_addr, map_addr2);
		if (i == 0) continue;
		
		um->um_vbanum = vhp->vbanum;
		um->um_adpt   = vhp->adptnum;
		config_fillin(um);
		printf(" csr 0x%x", um->um_addr);
		config_vme(udp->ud_addr1_atype);
		if(map_addr2) {
			printf(" csr2 0x%x", um->um_addr2);
			config_vme(udp->ud_addr2_atype);
		}
		printf(" vec 0x%x", um->um_ivnum);
		printf(" priority %d\n", um->um_bus_priority);
		um->um_alive = 1;
		um->um_vbahd = vhp;
		um->um_addr = (caddr_t)map_addr;
		um->um_addr2 = (caddr_t)map_addr2;
		um->um_physaddr = (caddr_t)svtophy(map_addr);
		udp->ud_minfo[um->um_ctlr] = um;
		
		switch(vbatype) {
		      case VBA_3VIA:
			intr_dispatch = (int (**)())_3VIA_VEC_ADDR(vhp, vec);
			Cprintf("probevba: intr_dispatch = 0x%x\n", 
				intr_dispatch);
			for (ivec = um->um_intr; *ivec; ivec++) {
				*intr_dispatch = *ivec;
				intr_dispatch++;
			}
			um->um_priority = splm[SPLBIO];
			break;

		      case VBA_XBIA:
			for (ivec = um->um_intr; *ivec; ivec++) {
				vecaddr = (int (**)())SCB_VME_VEC_ADDR(vhp->vbavec_page, vec); 
				*vecaddr = scbentry(*ivec, SCB_ISTACK);
			Cprintf("probevba: vecaddr = 0x%x, *vecaddr = 0x%x\n", 
				vecaddr, *vecaddr);
				vec++;
			}
			switch(um->um_bus_priority) {
			      case 1:
			      case 2:
				um->um_priority = splm[SPLBIO];
				break;
			      case 3:
			      case 4:
				um->um_priority = splm[SPLBIO];
				break;
			      case 5:
			      case 6:
				um->um_priority = splm[SPLBIO];
				break;
			      case 7:
				um->um_priority = splm[SPLBIO];
				break;
			      default:
				break;
			}
			break;
		}
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if (ui->ui_driver != udp || 
			    ui->ui_alive ||
			    ui->ui_ctlr != um->um_ctlr && ui->ui_ctlr != '?'||
			    ui->ui_adpt != vhp->vbanum ||
			    ui->ui_vbanum == '?')
				continue;
			Cprintf("probevba: Found matching ui entry\n");
			savectlr = ui->ui_ctlr;
			ui->ui_ctlr = um->um_ctlr;
			
			if ((*udp->ud_slave)(ui, map_addr, map_addr2)) {
				ui->ui_alive = 1;
				ui->ui_ctlr = um->um_ctlr;
				ui->ui_vbanum = vhp->vbanum;
				ui->ui_adpt   = vhp->adptnum;
				ui->ui_vbahd = vhp;
				ui->ui_addr = (caddr_t)map_addr;
				ui->ui_addr2 = (caddr_t)map_addr2;

				ui->ui_physaddr = (caddr_t)svtophy(map_addr);
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
				if(udp->ud_attach)
					(*udp->ud_attach)(ui);
			}
			else ui->ui_ctlr = savectlr;
		}
	}
	/*
	 * Now look for non-controller devices
	 */
	for (ui = ubdinit; udp = ui->ui_driver; ui++) {
		if((ui->ui_alive) ||
		   (ui->ui_adpt != vhp->vbanum) ||
		   (ui->ui_slave != -1) ||
		   (ui->ui_vbanum == '?')) continue;
                for(p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {
			if(strcmp("vba", p_adpt->p_name)==0 &&
                       		(char *)udp == p_adpt->c_name &&
			        (p_adpt->c_type == 'D') &&
                       		p_adpt->c_num == ui->ui_unit) break;
		}
		if (p_adpt->p_name == 0)
			continue;
		Cprintf("probevba: Non-controller - name = %s\n",
			ui->ui_devname);
		map_addr = 0;
		map_addr2 = 0;
	        vec = ui->ui_ivnum;
		if((vec < 0x40) || (vec > 0xFF)) {
			printf("%s%d not configured: Invalid vector 0x%x\n", ui->ui_devname, ui->ui_unit, vec);
			continue;
		}
		/* Map csr space(s)			*/
	        /* Map csr1 address space if present	*/
		if(ui->ui_addr) {
			if ( (map_addr = vme_map_csr(ui->ui_addr, 
				    udp->ud_addr1_size,
				    udp->ud_addr1_atype,
				    udp,
				    ui,
				    vhp)) == 0) {
				continue;
			}

		}
		/* Map csr2 address space if present	*/
		if(ui->ui_addr2) {
			if ( (map_addr2 = vme_map_csr(ui->ui_addr2, 
				    udp->ud_addr2_size,
				    udp->ud_addr2_atype,
				    udp,
				    ui,
				    vhp)) == 0) {
				continue;
			}
		}

		i = (*udp->ud_probe)(ui->ui_unit, map_addr, map_addr2);
		if (i == 0)
			continue;
	
		ui->ui_vbanum = vhp->vbanum;
		ui->ui_adpt   = vhp->adptnum;
		config_fillin(ui);
		printf(" csr 0x%x", ui->ui_addr);
		config_vme(udp->ud_addr1_atype);
		if(map_addr2){
			printf(" csr2 0x%x", ui->ui_addr2);
			config_vme(udp->ud_addr2_atype);
		}		       
		printf(" vec 0x%x", ui->ui_ivnum);
		printf(" priority %d\n", ui->ui_bus_priority);

		ui->ui_vbahd = vhp;
		switch(vbatype) {
		      case VBA_3VIA:
			intr_dispatch = (int (**)())_3VIA_VEC_ADDR(vhp, vec);
			Cprintf("probevba: intr_dispatch = 0x%x\n", 
				intr_dispatch);
			for (ivec = ui->ui_intr; *ivec; ivec++) {
				*intr_dispatch = *ivec;
				intr_dispatch++;
			}
			ui->ui_priority = splm[SPLBIO];
			break;

		      case VBA_XBIA:
			for (ivec = ui->ui_intr; *ivec; ivec++) {
				vecaddr = (int (**)())SCB_VME_VEC_ADDR(vhp->vbavec_page, vec); 
				*vecaddr = scbentry(*ivec, SCB_ISTACK);
			Cprintf("probevba: vecaddr = 0x%x, *vecaddr = 0x%x\n", 
				vecaddr, *vecaddr);
				vec++;
			}
			switch(ui->ui_bus_priority) {
			      case 1:
			      case 2:
				ui->ui_priority = splm[SPLBIO];
				break;
			      case 3:
			      case 4:
				ui->ui_priority = splm[SPLBIO];
				break;
			      case 5:
			      case 6:
				ui->ui_priority = splm[SPLBIO];
				break;
			      case 7:
				ui->ui_priority = splm[SPLBIO];
				break;
			      default:
				break;
			}
			break;
		}
		ui->ui_alive = 1;
		ui->ui_addr = (caddr_t)map_addr;
		ui->ui_addr2 = (caddr_t)map_addr2;
		ui->ui_physaddr = (caddr_t)svtophy(map_addr);
		ui->ui_dk = -1;
		/* ui_type comes from driver */
		udp->ud_dinfo[ui->ui_unit] = ui;
		if(udp->ud_attach)
			(*udp->ud_attach)(ui);
	}





        vecvme++;
	return(1);
}



u_long
vme_map_csr(addr, size, atype, udp, um, vhp) 
u_long		addr;
u_long		size;
u_long		atype;
struct	uba_driver	*udp;
struct	uba_ctlr	*um;
struct	vba_hd		*vhp;

{
register int	reg, treg, nmr, nmr16, atype_size, byte_swap;
register u_int	offset, reg_shift;
register u_long	map_addr, taddr, vcar, pio;
volatile u_int	 *via_pmr_ptr;
volatile u_int	 *pmr_ptr;
int	i, pmrinit = 0,	inval = 0;

        Cprintf("vme_map_csr: addr = 0x%x, size = 0x%x, atype = 0x%x\n",
		addr, size, atype);
        switch(atype & VME_ASPACE_MASK) {
	      case VME_A16:
		if( (addr + size - 1) > 0xFFFF )
			inval++;
		break;
	      case VME_A24:
		if(((addr & VME_A24_VALID) == 0) || 
		   ((addr + size - 1) > 0xFFFFFF))
			inval++;
		break;
	      case VME_A32:
		if((addr & VME_A32_VALID) == 0)
			inval++;
		break;
	}
        if(inval) {
		printf("%s%d not configured: csr out of range\n", um->um_ctlrname, um->um_ctlr);
		return(0);
	}

        /* Reserve space consumed by device registers	*/
	if (rmget(vhp->vba_map[atype & VME_ASPACE_MASK], size, addr) == 0) {
		printf("%s%d not configured: Overlapping csr space\n", um->um_ctlrname, um->um_ctlr);
		return(0);
	}

	switch(vhp->vba_type) {

	      case VBA_3VIA:
		offset = (u_int)addr & XVIA_PIO_OFFSET;
		reg_shift = XVIA_PIO_REGSHFT;
		nmr = (((int)size + offset + (vhp->nbyte_piopmr - 1) )  >> XVIA_PIO_REGSHFT);
		break;

	      case VBA_XBIA:
		offset = (u_int)addr & XVME_PIO_OFFSET;
		reg_shift = XVIB_PIO_REGSHFT;
		nmr = (((int)size + offset + (vhp->nbyte_piopmr - 1) )  >> XVME_PIO_SHIFT);
		Cprintf("vme_map_csr: vhp->pio_map = 0x%x\n", vhp->pio_map);
		Cprintf("vme_map_csr: vhp->nbyte_piopmr = 0x%x\n", 
			vhp->nbyte_piopmr);
		break;
	}

        atype_size = (atype & VME_ASIZE_MASK) >> VME_ASIZE_SHIFT;
        byte_swap = (atype & VME_BS_MASK) >> VME_BS_SHIFT;
	reg = rmalloc(vhp->pio_map, nmr);
        if (reg == 0) {
		printf("%s%d not configured: Insufficient mapping resources\n",
		       um->um_ctlrname, um->um_ctlr);
		return(0);
	}
	reg--;
	treg = reg;
	taddr = addr;
	switch(vhp->vba_type) {

	      case VBA_3VIA:
		if( (atype & VME_ASPACE_MASK) == VME_A32)
			pio = (XVIA_PIO_A32 << XVIA_PIO_AS_SHIFT);
		else if ( (atype & VME_ASPACE_MASK) == VME_A16 ) 
			pio = (XVIA_PIO_A16 << XVIA_PIO_AS_SHIFT);
		else
			pio = (XVIA_PIO_A24 << XVIA_PIO_AS_SHIFT);

		pio |= ( (atype_size + 1) << XVIA_PIO_DL_SHIFT) |
		       (byte_swap << XVIA_PIO_BS_SHIFT) |
		       (XVIA_PIO_SPROG << XVIA_PIO_FC_SHIFT) | 
		       XVIA_PIO_VALID;

		via_pmr_ptr = (u_int *)(Xviaregs.pio_pmr) + reg;
		Cprintf("vme_map_csr: via_pmr_ptr = 0x%x, nmr = %d\n",
			via_pmr_ptr, nmr);
		while (nmr-- != 0) { 
			*(int *)via_pmr_ptr++ = pio |
			      ((u_int)taddr & XVIA_PIO_MASK);
			(u_int)taddr += vhp->nbyte_piopmr;
		}
		WBFLUSH;
		break;

	      case VBA_XBIA:
		if( (atype & VME_ASPACE_MASK) == VME_A24)
			vcar = XVIB_PIO_A24;
		else if( (atype & VME_ASPACE_MASK) == VME_A32)
			vcar = XVIB_PIO_A32;
		else
			vcar = XVIB_PIO_A16;


		vcar |= ( (atype_size + 1) << XVIB_PIO_DL_SHIFT );
		while (nmr-- != 0) { 
			XVIB_STORE_PMR(treg, 
				       ((u_int)taddr & XVIB_PIO_MASK) |
				       vcar);
			(u_int)taddr += vhp->nbyte_piopmr;
			treg++;
		}			
		break;
	}


/* Create system virtual address of VME address space	*/
	map_addr = offset;
	map_addr |=  (reg << reg_shift);
	map_addr += (u_long)vhp->pio_base;
        Cprintf("vme_map_csr: map_addr = 0x%x\n", map_addr);
	return(map_addr);
}

int
vme_unmap_csr(vhp, addr, size)
struct	vba_hd	*vhp;
u_long	addr;
int	size;
{
	int	reg, reg_shift, nmr, offset, i, s;
	volatile u_int	 *via_pmr_ptr;
	
	switch(vhp->vba_type) {
	      case VBA_3VIA:
		offset = (u_int)addr & XVIA_PIO_OFFSET;
		reg_shift = XVIA_PIO_REGSHFT;
		nmr = (((int)size + offset + (vhp->nbyte_piopmr - 1) )  >> XVIA_PIO_REGSHFT);
		break;

	      case VBA_XBIA:
		offset = (u_int)addr & XVME_PIO_OFFSET;
		reg_shift = XVIB_PIO_REGSHFT;
		nmr = (((int)size + offset + (vhp->nbyte_piopmr - 1) )  >> XVIB_PIO_REGSHFT);
		break;
	}
	reg = addr - (u_long)vhp->pio_base;
	reg = reg >> reg_shift;
	switch(vhp->vba_type) {
	      case VBA_3VIA:
		via_pmr_ptr = (u_int *)(Xviaregs.pio_pmr) + reg;
		for (i = 0; i < nmr; i++) {
			*(int *)via_pmr_ptr++ = 0;
		}
		s = spl6();
		rmfree(vhp->pio_map, (long)nmr, (long)reg);
		splx(s);
		break;

	      case VBA_XBIA:
		break;
	}
}


/* Allocate the address space resource maps. These maps control	*/
/* the I/O section of VME address space.  This area will contain */
/* device registers and onboard memory.  It will also be used	*/
/* for device to device DMA.					*/
/* The A32 map will map the second 2GB of VME address space	*/
/* The A24 map will map the second 8MB of VME address space	*/
/* The A16 map will map the first 64KB of VME address space	*/
int
vme_init_maps(vhp)
struct	vba_hd	*vhp;
{
	int	i;


	for(i = 0; i < VME_NMAPS; i++) {
	    KM_ALLOC(vhp->vba_map[i], struct map *, VBAMSIZ*sizeof(struct map), KM_RMAP, KM_CLEAR|KM_NOWAIT);
	    if(vhp->vba_map[i] == (struct map *)NULL)
		    return(0);
        }
	
	/* Only allocate A32 and A24 DMA maps as we do not support A16 DMA */

        KM_ALLOC(vhp->dma_map[VME_A32], struct map *, VME_DMASIZ*sizeof(struct map), KM_RMAP, KM_CLEAR|KM_NOWAIT);
	if(vhp->dma_map[VME_A32] == (struct map *)NULL)
	    return(0);
        KM_ALLOC(vhp->dma_map[VME_A24], struct map *, VME_DMASIZ*sizeof(struct map), KM_RMAP, KM_CLEAR|KM_NOWAIT);
	if(vhp->dma_map[VME_A24] == (struct map *)NULL)
	    return(0);


	KM_ALLOC(vhp->pio_map, struct map *, VME_PIOSIZ*sizeof(struct map), KM_RMAP, KM_CLEAR|KM_NOWAIT);
        if(vhp->pio_map == (struct map *)NULL)
	    return(0);

	rminit(vhp->vba_map[VME_A32], 0x80000000, 0x80000000, "vme A32 map",VBAMSIZ);
	rminit(vhp->vba_map[VME_A24], 0x800000, 0x800000, "vme A24 map",VBAMSIZ);
	rminit(vhp->vba_map[VME_A16], 0x10000, 0x1, "vme A16 map",VBAMSIZ);
	


	/* Allocate the DMA PMR space resource maps			*/
	/* These maps are used for allocating the DMA Page Map Registers */
	/* which reside on the host adapter modules.  Each register maps */
	/* one page.  Since the address spaces overlap, some space must be */
	/* removed from the A32 and A24 maps to account for the A24 and A16 */
	/* spaces respectively.						 */

	if(vhp->n32dmapmr) 
		rminit(vhp->dma_map[VME_A32], vhp->n32dmapmr, 1, "vme A32 DMA PMR map",VME_DMASIZ);
	if(vhp->n24dmapmr) 
		rminit(vhp->dma_map[VME_A24], vhp->n24dmapmr, 1, "vme A24 DMA PMR map",VME_DMASIZ);

	/* Reserve DMA registers that can't be used because of address	*/
	/*   space overlap						*/
	rmget(vhp->dma_map[VME_A24], 0x10000/vhp->nbyte_dmapmr,
	      0x1); /* A16 space	*/
	rmget(vhp->dma_map[VME_A32], 0x1000000/vhp->nbyte_dmapmr, 
	      0x1); /* A24 space	*/


	/* Allocate PIO PMR resource map	     		   */
	/* This map controls the Page Map Registers used to access */
	/* VME space from system space.  The amount of memory that */
	/* each register maps is adapter-dependent.		   */

	rminit(vhp->pio_map, vhp->npiopmr, 1, "pio pmr map",VME_PIOSIZ);
	
	/*  Reserve 3VIA PIO registers which are used for 
	 *  PROM/register space. Add one to account for map
	 *  starting at 1.
	 */
	if ( vhp->vba_type == VBA_3VIA ) 
		rmget(vhp->pio_map, 128, 1);

        return(1);
}
config_vme(atype)
int	atype;
{
	int aspace, dsize;

	aspace = atype & VME_ASPACE_MASK;
	dsize = (atype & VME_ASIZE_MASK) >> VME_ASIZE_SHIFT;
	printf(" %s", vme_spaces[dsize][aspace]);
}

u_long
vba_get_vmeaddr(vhp, addr)
struct vba_hd *vhp;
u_long addr;
{
	u_long	vmeaddr;
	u_long	reg, reg_shift, nmr, offset;
	volatile u_int	 *via_pmr_ptr;

	switch(vhp->vba_type) {
	      case VBA_3VIA:
		offset = addr & XVIA_PIO_OFFSET;
		reg_shift = XVIA_PIO_REGSHFT;
		reg = addr - (u_long)vhp->pio_base;
		reg = reg >> reg_shift;
		via_pmr_ptr = (u_int *)(Xviaregs.pio_pmr) + reg;
		vmeaddr = *(int *)via_pmr_ptr & XVIA_PIO_MASK;
	        vmeaddr |= offset;
		break;

	      case VBA_XBIA:
		reg = addr - (u_long)vhp->pio_base;
		reg = reg >> reg_shift;
		offset = (u_int)addr & XVME_PIO_OFFSET;
		reg_shift = XVIB_PIO_REGSHFT;
		XVIB_READ_PMR(reg, vmeaddr);
	        vmeaddr |= offset;
		break;
	}
	return(vmeaddr);
}





