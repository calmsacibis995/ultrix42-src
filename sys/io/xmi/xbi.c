#ifndef lint
static char *sccsid = "@(#)xbi.c	4.3	(ULTRIX)	12/6/90";
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
/*
 * Revision History
 *
 * 03-Dec-90	jas
 *	Removed bit from error mask in xbiplus_check_errs() to prevent
 *	reporting of a nonexistent XBI+ error.  Also ensured that return
 *	values from xbiplus_check_errs() make it back to calling
 *	routine.
 *
 * 15-Sep-90	jas
 *	Fixed xbi+ support.  Must only look at low 16-bits of type
 *	register.
 *
 * 17-May-90	map (Mark Parenti)
 *	Add XVME support.
 *
 * 02-May-90    jas
 *	added xbi+ support to xbiinit() and xbi_check_errs().
 *
 * 08-Dec-89 	jaw
 *	make printf's use decimal for node and bus numbers.
 *
 * 28-Nov-89    Paul Grist
 *      modified xbi_check_errs() so that it returns recovery-status to the
 *      calling routine instead of panicing. It can now be shared by any 
 *      exception handlers wishing to check for XBI errors. This will be
 *      used to increase mchk info for XMI machines.
 *
 * 13-Nov-89	burns
 *	Fixed all xbi adapter mapping for mips based system to be consistent
 *	with the vax. Thsi fixed a bug in dealing with XBI adapters in low
 *	XMI slots on DECsystem 5800 systems.
 *
 * 20-Jul-89	rafiey (Ali Rafieymehr)
 *	Added another parameter (xmidata) to xminotconf.
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
 ************************************************************************/

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

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#ifdef mips
#include "../machine/kn5800.h"
#endif mips
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/bi/bireg.h"
#include "../io/xmi/xmireg.h"
#include "../io/xmi/xbireg.h"
#ifdef vax
#include "../machine/sas/vmb.h"
#endif vax

int vecbi=0;
extern int (*vax8800bivec[])();
extern int nNVAXBI;
extern struct bidata bidata[];

xbibinit(nxv,nxp,binumber,binode) 
register struct xbi_reg *nxv;
register struct xbi_reg *nxp;
register int binumber;
register int binode;


{	
	/* do nothing? */

}
xbiinit(nxv,nxp,xminumber,xminode,xmidata) 
register struct xbi_reg *nxv;
register struct xbi_reg *nxp;
register int xminumber;
register int xminode;
struct xmidata *xmidata;

{	
	register struct bi_nodespace *pxbib;
	register int xbibnode;
	int numbi = 0;

	/* If this is an XBIA+ then we need to check if it is a XVME
	 * adapter.  If it is, call the VME configuration routine.
	 */
	if(((short)nxv->xbi_dtype == XMI_XBIPLUS) &&
		((nxv->xbi_vdcr & XBIAP_TYPE_MASK) == XBIAP_XVME)) {
		xvmeconf(nxv, nxp, xminumber, xminode, xmidata);
		return(0);
	}

	numbi = xminode | (xminumber <<4);
	if(is_adapt_configured("vaxbi",numbi) == 0 ) {
		xminotconf(nxv,nxp,xminumber,xminode,xmidata);
		return(-1);
	}

	printf("vaxbi%d at xmi%d node %d\n",numbi,xminumber,xminode);
	/*
	 * Set up physical and virtual addresses for the BI adapter.
	 */
	bidata[numbi].biphys = (struct bi_nodespace *)
				xmi_io_space(xminumber, xminode);
	bidata[numbi].bivirt = ((struct bi_nodespace *)(nexus));

	bidata[numbi].bivirt += (vecbi*16);
	bidata[numbi].bivec_page = vax8800bivec+(vecbi*128);

	/* set-up XBI+ to behave as XBI */
	if((short)nxv->xbi_dtype == XMI_XBIPLUS) {
		nxv->xbi_acsr = (nxv->xbi_acsr & ~0x40000080) |
		                0xa; /*Disable ASSERT resp, set RVD, MIE*/
		nxv->xbi_autlr |= 0xf0000000; /* Max # of IREAD retry and how
                                                  long XMI_LOCKOUT stays set */
	}
		


	/* set-up XBI vector offset register */
#ifdef vax
	nxv->xbi_bvor =  ( (  ((int) vax8800bivec) & ~VA_SYS)
			- ((int)&scb.scb_stray & ~ VA_SYS)) + (vecbi*0x200);
#endif vax
#ifdef mips
	nxv->xbi_bvor = (((int) vax8800bivec)
			- ((int)&scb.scb_stray)) + (vecbi*0x200);
#endif mips
	
	/* set-up xbib xmi interrupt destination register */
	nxv->xbi_bidr = xmidata->xmiintr_dst;
	
	/* enable xbib interrupts */
	nxv->xbi_bcsr |= BCSR_ENABLE_INT;


	/* set up the xbib to be BI node zero */
	pxbib = bidata[numbi].bivirt;

	/* map bi address space */
	nxaccess(bidata[numbi].biphys,Nexmap[((vecbi*16))],BINODE_SIZE);

	/* force loopback so we can get node ID */
	nxv->xbi_bdcr1 |=XBI_LOOPBACK;
	xbibnode = pxbib->biic.biic_ctrl & BICTRL_ID;
	nxv->xbi_bdcr1 &=(~XBI_LOOPBACK);
	
	/* set up mapping for proper BI node */
	pxbib = (struct bi_nodespace *) nexus;
	pxbib += ((vecbi*16)+xbibnode);

	bidata[numbi].cpu_biic_addr = pxbib;

	nxaccess(bidata[numbi].biphys+xbibnode,
			Nexmap[((vecbi*16)+xbibnode)],BINODE_SIZE);

	bidata[numbi].biintr_dst = 1 << xbibnode;

	/* initialize that nbib */
	pxbib->biic.biic_typ = BI_XBI;
	pxbib->biic.biic_bci_ctrl = (BCI_INTREN|BCI_RTOEVEN);

/*
	pxbib->biic.biic_strt = 0;
	pxbib->biic.biic_end = (vmb_info.memsiz+0x3ffff) & (~0x3ffff); 
*/
	pxbib->biic.biic_err = pxbib->biic.biic_err;
	pxbib->biic.biic_ctrl |= BICTRL_BROKE;

	probebi(numbi);

	
	/* clear out any errors generated when BI was probed */
	nxv->xbi_aesr = nxv->xbi_aesr; 
	nxv->xbi_xbe = nxv->xbi_xbe & ~XMI_XBAD;  /* write 1 to clear */
	nxv->xbi_besr = nxv->xbi_besr; 
	
	/* set up vector for error interrupt */
	nxv->xbi_bvr = 0x50;
	nxv->xbi_aivintr  = xmidata->xmiintr_dst;

	/* enable error interrupts -- note that CRD errors are not
	initialize.  This is because there is no easy way to call into
	the CRD logging code from the XMI dispatch.  The CRD should be
	picked up by the processor soon. */
	if((short)nxv->xbi_dtype == XMI_XBI)
		nxv->xbi_aimr = (XBIA_ENABLE_IVINTR |
				   XBIA_ENABLE_CC | /* corrected confirmaiton*/
				   XBIA_ENABLE_PE | /* parity error */
				   XBIA_ENABLE_WSE| /* write seq error */
				   XBIA_ENABLE_RDN| /* read data noack */
				   XBIA_ENABLE_WDN | /* write data noack */
				   XBIA_ENABLE_NRR | /* No read response */
				   XBIA_ENABLE_RSE | /* Read seq error */
				   XBIA_ENABLE_RER | /* Read error response */
				   XBIA_ENABLE_NXM | /* non-exist memory */
				   XBIA_ENABLE_IBUS_APE| /* a bus parity */
				   XBIA_ENABLE_IBUS_BPE| /* b bus parirty */
				   XBIA_ENABLE_IBUS_DPE); /* data parity */
	/* XBI+ support */	
	if((short)nxv->xbi_dtype == XMI_XBIPLUS)
 	         nxv->xbi_aimr = (XBIAP_ENABLE_IPE | 
				  XBIAP_ENABLE_TTO |      
				  XBIAP_ENABLE_IPFN |      
				  XBIAP_ENABLE_CECCERR |
				  XBIAP_ENABLE_UERRERR |   
				  XBIAP_ENABLE_IBIA |      
				  XBIAP_ENABLE_IE |         
 				  XBIAP_ENABLE_IOWRTFAIL |
				  XBIAP_ENABLE_BCIACLO |   
				  XBIAP_ENABLE_ADPE |      
				  XBIAP_ENABLE_ACPE |      
				  XBIAP_ENABLE_BDPE |      
				  XBIAP_ENABLE_BCPE |      
				  XBIAP_ENABLE_IORDPE |
				  XBIA_ENABLE_IVINTR |
				  XBIA_ENABLE_CC | /* corrected confirmaiton*/
				  XBIA_ENABLE_PE | /* parity error */
				  XBIA_ENABLE_WSE| /* write seq error */
				  XBIA_ENABLE_RDN| /* read data noack */
				  XBIA_ENABLE_WDN | /* write data noack */
				  XBIA_ENABLE_NRR | /* No read response */
				  XBIA_ENABLE_RSE | /* Read seq error */
				  XBIA_ENABLE_RER | /* Read error response */
				  XBIA_ENABLE_NXM); /* non-exist memory */

	nxv->xbi_bcsr |= (BCSR_INTERLOCK_MASK | BCSR_PARITY_ERROR);
	vecbi++;
	return(0);
}

/* 
 *  Routine: xbi_check_errs.
 *
 *	This routine checks to see if the XBI adapter pointed to by
 *  the addresss in "xbi" has detected an error.  If so an error log
 *  packet is generated. It returns recovery-status back to the calling
 *  routine, so it can decide to panic or not.
 *
 */

xbi_check_errs(xminumber,xminode,xbi) 
int xminumber;
int xminode;
struct xbi_reg *xbi;

{

	register struct el_xbi *elxbi_ptr;
	int recover=0;
        register struct el_rec *elrp;

	/* check for xbi+ */
	if ((short)xbi->xbi_dtype == XMI_XBIPLUS)
		return(xbiplus_check_errs(xminumber,xminode,xbi));
	else {
	/* check for error */
	if (((xbi->xbi_xbe & 0x3f7e000)==0) &&  /* check XMI errors */
	    ((xbi->xbi_besr & 0xf)== 0) &&      /* xbib errors */
	    ((xbi->xbi_aesr & 0xff) == 0)) 	/* xbia errors */
	    return(1); /* no error pending, recover=YES */	

        /* get error log packet and fill in header. */
        elrp = ealloc(sizeof(struct el_xbi),(recover?EL_PRILOW:EL_PRISEVERE));
	
        if (elrp != NULL) {
            LSUBID(elrp,ELCT_ADPTR,ELADP_XBI,EL_UNDEF,xminumber,
	    		EL_UNDEF,EL_UNDEF);

        	elxbi_ptr = &elrp->el_body.el_xbi;
		
		elxbi_ptr->xbi_dtype = xbi->xbi_dtype;
		elxbi_ptr->xbi_fadr = xbi->xbi_fadr;
		elxbi_ptr->xbi_arear = xbi->xbi_arear;
		elxbi_ptr->xbi_aesr = xbi->xbi_aesr;
		elxbi_ptr->xbi_aimr = xbi->xbi_aimr;
		elxbi_ptr->xbi_aivintr = xbi->xbi_aivintr;
		elxbi_ptr->xbi_adg1 = xbi->xbi_adg1;
		elxbi_ptr->xbi_bcsr = xbi->xbi_bcsr;
		elxbi_ptr->xbi_besr = xbi->xbi_besr;
		elxbi_ptr->xbi_bidr = xbi->xbi_bidr;
		elxbi_ptr->xbi_btim = xbi->xbi_btim;
		elxbi_ptr->xbi_bvor = xbi->xbi_bvor;
		elxbi_ptr->xbi_bvr = xbi->xbi_bvr;
		elxbi_ptr->xbi_bdcr1 = xbi->xbi_bdcr1;
		EVALID(elrp);
        }


	if (!recover) {
		cprintf("hard error XBI at xmi%d node %d\n",xminumber,xminode);

		cprintf("\tdtype %x,\txbe %x,\tfadr %x,\tarear %x,\taesr %x\n",
			xbi->xbi_dtype, xbi->xbi_xbe, xbi->xbi_fadr, 
			xbi->xbi_arear, xbi->xbi_aesr);
		cprintf("\tsimr %x,\taivintr %x,\tadg1 %x,\tbcsr %x,\tbesr %x\n",
			xbi->xbi_aimr,xbi->xbi_aivintr, xbi->xbi_adg1, 
			xbi->xbi_bcsr,xbi->xbi_besr);
			
		cprintf("\tbidr %x,\tbtim %x,\tbvor %x,\tbvr %x,\n",
			xbi->xbi_bidr, xbi->xbi_btim, xbi->xbi_bvor,
			xbi->xbi_bvr);

		cprintf("\tbdcr1 %x\n",xbi->xbi_bdcr1);

		return(recover); /* recover=0 */
	}

	/* clear out any errors generated when BI was probed */
	xbi->xbi_aesr = xbi->xbi_aesr; 
	xbi->xbi_xbe = xbi->xbi_xbe & ~XMI_XBAD;  /* write 1 to clear */
	xbi->xbi_besr = xbi->xbi_besr; 

	return(recover);
	} /* END BIG IF-ELSE */
}
/* 
 *  Routine: xbiplus_check_errs.
 *
 *	This routine checks to see if the XBI+ adapter pointed to by
 *  the addresss in "xbi" has detected an error.  If so an error log
 *  entry is made.
 */

xbiplus_check_errs(xminumber,xminode,xbi) 
int xminumber;
int xminode;
struct xbi_reg *xbi;

{

	register struct el_xbiplus *elxbi_ptr;
	int recover=0;
        register struct el_rec *elrp;

	/* check for error */
        if (((xbi->xbi_xbe & 0x39ffa000)==0) && /* check XMI errors */
   	    ((xbi->xbi_besr & 0xf)==0)       && /* check xbib errors */
            ((xbi->xbi_aesr & 0x800001ff)==0x80000000))  /* check xbia errors */
		return(1);                   /* no error pending, recover=YES */

        /* get error log packet and fill in header. */
        elrp = ealloc(sizeof(struct el_xbiplus),(recover?EL_PRILOW:EL_PRISEVERE));
	
        if (elrp != NULL) {
                LSUBID(elrp,ELCT_ADPTR,ELADP_XBIPLUS,EL_UNDEF,xminumber,
	    	   EL_UNDEF,EL_UNDEF);
        	elxbi_ptr = &elrp->el_body.el_xbiplus;
		
		elxbi_ptr->xbi_dtype = xbi->xbi_dtype;
		elxbi_ptr->xbi_fadr = xbi->xbi_fadr;
		elxbi_ptr->xbi_arear = xbi->xbi_arear;
		elxbi_ptr->xbi_aesr = xbi->xbi_aesr;
		elxbi_ptr->xbi_aimr = xbi->xbi_aimr;
		elxbi_ptr->xbi_aivintr = xbi->xbi_aivintr;
		elxbi_ptr->xbi_adg1 = xbi->xbi_adg1;
		elxbi_ptr->xbi_bcsr = xbi->xbi_bcsr;
		elxbi_ptr->xbi_besr = xbi->xbi_besr;
		elxbi_ptr->xbi_bidr = xbi->xbi_bidr;
		elxbi_ptr->xbi_btim = xbi->xbi_btim;
		elxbi_ptr->xbi_bvor = xbi->xbi_bvor;
		elxbi_ptr->xbi_bvr = xbi->xbi_bvr;
		elxbi_ptr->xbi_bdcr1 = xbi->xbi_bdcr1;
		elxbi_ptr->xbi_autlr = xbi->xbi_autlr;
		elxbi_ptr->xbi_acsr = xbi->xbi_acsr;
		elxbi_ptr->xbi_arvr = xbi->xbi_arvr;
		elxbi_ptr->xbi_abear = xbi->xbi_abear;
		elxbi_ptr->xbi_xbe = xbi->xbi_xbe;
		elxbi_ptr->xbi_xfaer = xbi->xbi_xfaer;
		EVALID(elrp);
        }


	if (!recover) {
		cprintf("hard error XBI+ at xmi%d node %d\n",xminumber,xminode);
		cprintf("\tdtype %x,\txbe %x,\tfadr %x\n",
			xbi->xbi_dtype, xbi->xbi_xbe, xbi->xbi_fadr); 
		cprintf("\tarear %x,\taesr %x,\taimr %x\n",
			xbi->xbi_arear, xbi->xbi_aesr, xbi->xbi_aimr);
		cprintf("\taivintr %x,\tadg1 %x,\tbcsr %x\n",
			xbi->xbi_aivintr, xbi->xbi_adg1, xbi->xbi_bcsr); 
			
		cprintf("\tbesr %x,\tbidr %x,\t\tbtim %x\n",
			xbi->xbi_besr,xbi->xbi_bidr, xbi->xbi_btim);
		cprintf("\tbvor %x,\tbvr %x,\t\tbdcr1 %x\n",
			xbi->xbi_bvor, xbi->xbi_bvr, xbi->xbi_bdcr1);

		cprintf("\tautlr %x,\tacsr %x,\tarvr %x\n",
			xbi->xbi_autlr,xbi->xbi_acsr,xbi->xbi_arvr);
		cprintf("\tabear %x,\txfear %x\n",
			xbi->xbi_abear,xbi->xbi_xfaer);
		return(recover); /* recover=0 */
	}

	/* clear out any errors generated when BI was probed */
	xbi->xbi_aesr = xbi->xbi_aesr; 
	xbi->xbi_xbe = xbi->xbi_xbe & ~XMI_XBAD;  /* write 1 to clear */
	xbi->xbi_besr = xbi->xbi_besr; 

	return(recover);
}


