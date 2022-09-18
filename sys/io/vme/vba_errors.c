/*
static char *sccsid = "@(#)vba_errors.c	4.4	(ULTRIX)	4/4/91";
 */


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
 ************************************************************************
 * Abstract:
 *	This module contains the routines used for VMEbus error handling.
 *	
 *
 * Modification History:
 *
 * 01-Apr-91 --	Mark Parenti
 *	Check for clocks present before reading MVIB csr.  If the cable has
 *	been pulled, a read of the MVIB CSR will hang the system.
 *
 * 22-Jan-91 -- Mark Parenti
 *	Remove checks of VIC BESR. Any error reported here will also be
 *	reported through the 3VIA or MVIB csrs.
 *	Ignore NACK errors.  These are due to VMEbus timeouts which will
 *	cause TURBOchannel timeouts.  Logging those errors would cause
 *	error logs to be generated for failed BADADDR calls.
 *
 * 11-Dec-90 -- Mark Parenti
 *	Remove MVIA support.
 *	Cleanup error reporting.
 *
 * 04-Jan-90 -- Paul Grist
 *	Original Version - support for 3max, mipsfair2, and cmax VMEbus
 *      adapters.
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

#include "../machine/nexus.h"
#include "../io/xmi/xmireg.h"
#include "../io/xmi/xbireg.h"
#include "../io/vme/xviareg.h"
#include "../io/vme/xvibreg.h"
#include "../io/vme/vbareg.h"
#include "../io/vme/vbavar.h"
#include "../io/uba/ubavar.h"
 
extern struct vba_hd *get_vba();

#define PANIC_YES 1                      /* return value to panic system */
#define PANIC_NO  0                      /* return value to recover error */


/*****************************************************************************
 *      MVIBerrors
 *
 *                The MVIB is the VME-side board used for 
 *                the 3MAX VME implemntation. It
 *                contains the registers for the two-board sets,
 *                3VIA-MVIB (3max).
 *
 *      function: 3MAX VMEbus adapter error handler.
 *                Its job is to do the following:
 *  
 *                1. check for and classify pending error(s)
 *                2. log error(s) and pertinant additional info
 *                3. if recoverable, reset appropriate registers
 *                4. return panic status back to calling routine
 *
 *****************************************************************************/

MVIBerrors(vhp)
register struct vba_hd *vhp;

{
register struct el_rec	*elp;
register struct el_vba *elvba_ptr;
unsigned long  csr, xvia_csr;
short must_panic=1 ;
short error_type = VBA_VME_ERROR ;
short adapter_type;

        /* get neccessary registers */

        xvia_csr = *Xviaregs.viacsr;     /* xVIA control/status */
        if (xvia_csr & VIACSR_YAB_NCLK)					 
	   {
	    /*don't read b csr, cable was pulled */
	    /* panic = 1 */
		must_panic = 1;
	        error_type = VBA_YABUS_ERROR;
		csr = 0;                  /* initialize csr */	
	     }
	else  { 

           csr = *Xviaregs.csr ;            /* control/status reg */


           /* if no error bits set -- return (PANIC_NO) */

           if  ( !(xvia_csr & VIACSR_ERR_MSK) &&     
                 !(csr & CSR_ERR_MSK) ) { 
		   if (xvia_csr & VIACSR_YAB_NACK) {
			/* clear ignored nack errors */
			*Xviaregs.viaclr = VIACSR_YAB_NACK;
		}
	     return (PANIC_NO) ;
	   }

           /* 
	    * check registers against error masks and
            * use result to classify error and set recover status.
	    */


           if ( (csr & CSR_VME_SYSRST) == 0 ) {
	     error_type = VBA_SYS_RESET ;
	     must_panic = 0 ;
	   }

           /*
            * For now, let YAB errors ride to see if they
            * will be resolved by a processor level TO
            */


           if ( (csr & CSR_YAB_PE) || 
	        (csr & CSR_YAB_XAFE) ||
	        (xvia_csr & VIACSR_ERR_MSK) ) {
	     error_type = VBA_YABUS_ERROR ;
	     must_panic = 0 ;
           }

           if ( csr & CSR_DMA_PMFE ) {
	     error_type = VBA_PMAP_FAULT ;
	     must_panic = 1 ;
           }

           if ( csr & CSR_PIO_PMFE ) {
	     error_type = VBA_PMAP_FAULT ;
	     must_panic = 1 ;
           }

           if ( csr & CSR_VME_MFE ) {
	     error_type = VBA_VME_MOD_FAIL ;
	     must_panic = 0 ;
	   }

           if ( csr & CSR_VME_ACLOW ) {
	     error_type = VBA_VME_AC_LOW ;
	     must_panic = 0 ;
	   }

        }

        /* log a vba error packet: allocate packet, fill in header */

	if ((elp = ealloc(sizeof(struct el_vba),
		(must_panic?EL_PRILOW:EL_PRIHIGH))) == EL_FULL)
		return(PANIC_YES);

        adapter_type = ELDT_3VIA;


	LSUBID( elp,
		ELCT_ADPTR,              /* adapter class */
	        ELADP_VBA,               /* VMEbus adapter */
		adapter_type,            /* what VME adapter set */
		vhp->vbanum,             /* VMEbus number */
		EL_UNDEF,
		error_type );            /* error class */ 

	elvba_ptr = &elp->el_body.el_vba;
	   
        elvba_ptr->elvba_reg.elmvib.mvib_viacsr = *Xviaregs.viacsr ;
        elvba_ptr->elvba_reg.elmvib.mvib_csr = *Xviaregs.csr ;
        elvba_ptr->elvba_reg.elmvib.mvib_vfadr = *Xviaregs.vfadr;
        elvba_ptr->elvba_reg.elmvib.mvib_cfadr = *Xviaregs.cfadr;
        elvba_ptr->elvba_reg.elmvib.mvib_ivs = *Xviaregs.ivs;
        elvba_ptr->elvba_reg.elmvib.mvib_besr = *Xviaregs.besr;
        elvba_ptr->elvba_reg.elmvib.mvib_errgi = *Xviaregs.errgi;
        elvba_ptr->elvba_reg.elmvib.mvib_lvb = *Xviaregs.lvb;
        elvba_ptr->elvba_reg.elmvib.mvib_err = *Xviaregs.err;

	EVALID( elp );  /* validate error packet */

        /* reset/clear status registers */
	if (csr)   	/* check if B csr hard-coded to 0, means we can't
			   access it */
	{

          csr = csr &  CSR_RESET_ERROR_BITS; /*reset all error bits using mask*/

 	  *Xviaregs.csr = csr ;
	}

	*Xviaregs.viaclr = (xvia_csr & VIACSR_ERR_MSK);

        /* report to console and return panic status to calling routine */

        if (must_panic) {
	  cprintf("Fatal VMEbus adapter error has occured\n");
	  return(PANIC_YES);
	}

        return(PANIC_NO);
} 


/**************************************************************************
 *      XBIAerrors
 *
 *      function: CMAX VMEbus adapter error handler. Its job
 *                is do the following:
 *  
 *                1. check for and classify pending error(s)
 *                2. log error(s) and pertinant additional info
 *                3. if recoverable, reset appropriate registers
 *                4. return panic status back to calling routine
 *
 **************************************************************************/

XBIAerrors(vhp)
register struct vba_hd *vhp;

{
register struct el_rec	*elp;
register struct el_vba *elvba_ptr;
register struct xbi_reg *xbireg;
int aesr, xber;
unsigned long vesr, vdcr;
short must_panic = 1 ;
short error_type = VBA_VME_ERROR ;


        /* get neccessary registers */

           xbireg = (struct xbi_reg *)vhp->vbavirt;

           xber = xbireg->xbi_xbe;
           aesr = xbireg->xbi_aesr;
           vdcr = *Xvibregs.vdcr;   
           vesr = *Xvibregs.vesr;

        /* if no bits set return (PANIC_NO) */

           if (  !(xber & XMI_NSES) &&
	         !(vdcr & VDCR_ERR_SUM)) {
	     return(PANIC_NO);
	   }

        /* 
         * check registers against error masks and
         * use result to classify error and set recovery status 
         */

         if ( (aesr & XBIA_CORR_PMR_ECC) || (aesr & XBIA_CORR_DMA_ECC)) {
	   error_type = VBA_CORR_ECC;
	   must_panic = 0 ;
	 }

         if (aesr & XBIA_MULT_ERRS) {
	   error_type = VBA_MULTIPLE_ERRS;
	   must_panic = 1;
	 }

         if ( (aesr & XBIA_UNCORR_PMR_ECC) ||
	      (aesr & XBIA_UNCORR_DMA_ECC)) {
	   error_type = VBA_UNCORR_ECC;
	   must_panic = 1;
	 }

         if ( (vesr & VESR_SRAM_PE) || (vesr & VESR_TPE)) {
	   error_type = VBA_VME_PARITY;
	   must_panic = 1;
	 }

         if ( (vesr & VESR_TRNTO) || (vesr & VESR_ARBTO)) {
	   error_type = VBA_VME_TIMEOUT;
	   must_panic = 1;
	 }

         if ( !(aesr & XBIA_IB_OK) ) {
	   error_type = VBA_IBUS_CABLE_FLT;
	   must_panic = 1;
	 }

         if ( (vesr & VESR_ITPE) ||
	      (vesr & VESR_IRPE) ||
	      (aesr & XBIA_IBUS_A_DPE) ||
	      (aesr & XBIA_IBUS_B_CAPE) ||
	      (aesr & XBIA_IBUS_B_DPE) ||
	      (aesr & XBIA_IBUS_B_CAPE) ||
	      (aesr & XBIA_IBUS_DPE) ) {
	   error_type = VBA_IBUS_PARITY;
	   must_panic = 1;
	 }

         if ( (vesr & VESR_INTERLOCK) ||
	      (vesr & VESR_RMWII) ||
	      (vesr & VESR_RMW)) {
	   error_type = VBA_RMW_INTERLOCK;
	   must_panic = 1;
	 }

         if (vesr & VESR_BERR) {
	   error_type = VBA_VME_BERR;
	   must_panic = 1 ;
	 }

         if (aesr & XBIA_INT_ERR) {
	   error_type = VBA_XBIA_INTERNAL;
	   must_panic = 1;
	 }

         if (aesr & XBIA_IOW_FAIL) {
	   error_type = VBA_IO_WRITE_FAIL;
	   must_panic = 1;
	 }

         if (aesr & XBIA_INV_PFN) {
	   error_type = VBA_INVALID_PFN;
	   must_panic = 1;
	 }

        /* log vba error packet: allocate packet fill in header */

	if ((elp = ealloc(sizeof(struct el_vba),
		(must_panic?EL_PRILOW:EL_PRIHIGH))) == EL_FULL)
		return(PANIC_YES);

	LSUBID( elp,
		ELCT_ADPTR,              /* adapter class */
	        ELADP_VBA,               /* VMEbus adapter */
		ELDT_XBIA,               /* what VME adapter set */
		vhp->vbanum,             /* VMEbus number */
		EL_UNDEF,
		error_type );            /* error class */ 

	elvba_ptr = &elp->el_body.el_vba;

        elvba_ptr->elvba_reg.elxbia.xbia_dtype = xbireg->xbi_dtype;
	elvba_ptr->elvba_reg.elxbia.xbia_xbe = xbireg->xbi_xbe;
	elvba_ptr->elvba_reg.elxbia.xbia_fadr = xbireg->xbi_fadr;
	elvba_ptr->elvba_reg.elxbia.xbia_arear = xbireg->xbi_arear;
	elvba_ptr->elvba_reg.elxbia.xbia_aesr = xbireg->xbi_aesr;
	elvba_ptr->elvba_reg.elxbia.xbia_aimr = xbireg->xbi_aimr;
	elvba_ptr->elvba_reg.elxbia.xbia_aivintr=xbireg->xbi_aivintr;
	elvba_ptr->elvba_reg.elxbia.xbia_adg1 = xbireg->xbi_adg1;

	elvba_ptr->elvba_reg.elxbia.xvib_vdcr = *Xvibregs.vdcr ;
	elvba_ptr->elvba_reg.elxbia.xvib_vesr =  *Xvibregs.vesr ;
	elvba_ptr->elvba_reg.elxbia.xvib_vfadr =  *Xvibregs.vfadr ;
	elvba_ptr->elvba_reg.elxbia.xvib_vicr =  *Xvibregs.vicr ;
	elvba_ptr->elvba_reg.elxbia.xvib_vvor =  *Xvibregs.vvor ;
	elvba_ptr->elvba_reg.elxbia.xvib_vevr =  *Xvibregs.vevr ;

	EVALID( elp );  /* validate error packet */

        /* reset/clear status registers  -- !!!!!verbatum from vbainit.c for now!!!! */

           xbireg->xbi_aesr = xbireg->xbi_aesr;
           xbireg->xbi_xbe = xbireg->xbi_xbe & ~XMI_XBAD; /* write 1 to clear */
           xbireg->xbi_besr = xbireg->xbi_besr;


        /* report to console and return panic status to calling routine */

        if (must_panic) {
	  cprintf("Fatal VMEbus adapter error has occured");
	  cprintf("??report register info");
	  return(PANIC_YES);
	}

        printf("VMEbus adapter error recovery being attempted");
        return(PANIC_NO);

} 


/*****************************************************************************
 * vbaerrors - the VMEbus adapter error handler
 *
 *  function:  The entry point for vba errors. It will take
 *             care of any general adapter details and 
 *             dispatch to the individual adapter error
 *             handler to take care of the adapter specific
 *             details.
 *
 *
 *   1. get the vba_hd structure which contains the vba information 
 *   2. dispatch to the individual adapter error routine using the
 *      vba_type field from the vba_hd structure. These adapter
 *      routine will handle specific error handling details and
 *      return recovery status.
 *   3. take care of any other general details (additional logging??)
 *   4. panic if the recovery status returned says to
 *
 *****************************************************************************/

vbaerrors(vbanumber)
int vbanumber;

{
	register struct vba_hd *vhp;
	int must_panic = 0;

	vhp = get_vba(vbanumber);

	switch (vhp->vba_type) {

	      case VBA_3VIA: {if (MVIBerrors(vhp) == PANIC_YES)
		               must_panic = 1;
			      break;}

	      case VBA_XBIA: {if (XBIAerrors(vhp) == PANIC_YES)
		               must_panic = 1;
			     break;}

	      default: panic("unknown vba adapter type");

	} /*switch*/

	/*
	 * take care of any other general stuff here
         * perhaps some additional logging of information
         * depending on cpu type - ala vaxbierrors...
         */
	if (must_panic) panic("VMEbus adapter error");

} /*vba_errors*/


/******************************************************************************
 *
 *      log_vme_device_error
 *
 *	function: Provide device driver with an error logging
 *                routine for device errors (VME modules that
 *                use a uba_device structure). The routine will
 *                log a packet with an error message from the
 *                driver, device information, and the VMEbus
 *                adapter registers.
 *
 *      NOTE: Although the device and the controller routines are
 *            similar, they are going to be kept seperate for
 *            clarity and ease of use by the device drivers. 
 *            (as opposed to passing a flag indicating dev/crtl)
 *
 ******************************************************************************/


log_vme_device_error(text,vhp, devptr)
char		   *text;                    /* ASCII error message*/
struct	vba_hd	   *vhp;                     /* ptr to vba_hd struct */
struct  uba_device *devptr;                  /* ptr to uba struct */

{
register struct el_rec	*elp;
register struct el_vme_dev_cntl *elvme_ptr;
register struct xbi_reg *xbireg;
int adapter_type;
char *msg_ptr;
int i;

/*								
 *	Allocate error log packet				
 */								

	if ((elp = ealloc(sizeof(struct el_vme_dev_cntl),
		EL_PRIHIGH)) == EL_FULL)
		return;
/*								
 *	Get adapter type and Initialize subid fields
 */							
        switch (vhp->vba_type) {
	  case VBA_3VIA: {adapter_type = ELDT_3VIA;
	                  break;}
	  case VBA_XBIA: {adapter_type = ELDT_XBIA;
	                  break;}
	} /*switch*/

	LSUBID( elp,
		ELCT_DCNTL,              /* device controller class */
	        ELVME_DEV_CNTL,          /* VME device controller */
		adapter_type,            /* what VME adapter set */
		vhp->vbanum,             /* VMEbus number */
		EL_UNDEF,
		VME_DEVICE);             /* device type error */ 

	elvme_ptr = &elp->el_body.elvme_devcntl;

/*
 *      put name string into the packet 
 */
        msg_ptr = devptr->ui_devname;
        for (i=1; *msg_ptr++ != '\0' && i > EL_SIZE64; i++) ;
        bcopy(devptr->ui_devname, elvme_ptr->module, i);
        elvme_ptr->module[i-1] = '\0' ;

/*
 *      fill in rest of packet 
 */
        elvme_ptr->num  = devptr->ui_unit ;
        elvme_ptr->csr1 = devptr->ui_addr ;
        elvme_ptr->csr2 = devptr->ui_addr2 ;

        if (text == NULL) text = "NO ERROR MESSAGE ENTERED BY DRIVER" ;
	elvme_ptr->el_vme_error_msg.msg_len = sizeof(text);
        bcopy(text, elvme_ptr->el_vme_error_msg.msg_asc, sizeof(text));

	switch (adapter_type) {
	  case ELDT_3VIA: {
            elvme_ptr->elvba.elvba_reg.elmvib.mvib_viacsr = *Xviaregs.viacsr ;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_csr = *Xviaregs.csr ;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_vfadr = *Xviaregs.vfadr;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_cfadr = *Xviaregs.cfadr;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_ivs = *Xviaregs.ivs;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_besr = *Xviaregs.besr;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_errgi = *Xviaregs.errgi;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_lvb = *Xviaregs.lvb;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_err = *Xviaregs.err;
	    break; 
          }
	  case ELDT_XBIA: {
	    xbireg = (struct xbi_reg *)vhp->vbavirt;

	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_dtype = xbireg->xbi_dtype;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_xbe = xbireg->xbi_xbe;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_fadr = xbireg->xbi_fadr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_arear = xbireg->xbi_arear;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_aesr = xbireg->xbi_aesr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_aimr = xbireg->xbi_aimr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_aivintr=xbireg->xbi_aivintr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_adg1 = xbireg->xbi_adg1;

	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vdcr = *Xvibregs.vdcr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vesr =  *Xvibregs.vesr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vfadr =  *Xvibregs.vfadr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vicr =  *Xvibregs.vicr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vvor =  *Xvibregs.vvor ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vevr =  *Xvibregs.vevr ;
            break;
          }
	} /*switch*/

/*
 *	Validate error log packet				
 */

	EVALID( elp );

} /*logdevice*/



/****************************************************************************
 *
 *      log_vme_crtl_error
 *
 *	function: Provide device driver with an error logging
 *                routine for controller errors (VME modules that
 *                use a uba_ctlr structure). The routine will
 *                log a packet with an error message from the
 *                driver, ctlr information, and the VMEbus
 *                adapter registers.
 *
 ******************************************************************************/


log_vme_ctlr_error(text,vhp, devptr)
char		   *text;                    /* ASCII error message*/
struct	vba_hd	   *vhp;                     /* ptr to vba_hd struct */
struct  uba_ctlr   *devptr;                  /* ptr to uba struct */

{
register struct el_rec	*elp;
register struct el_vme_dev_cntl *elvme_ptr;
register struct xbi_reg *xbireg;
int adapter_type;
char *msg_ptr;
int i;

/*								
 *	Allocate error log packet				
 */								

	if ((elp = ealloc(sizeof(struct el_vme_dev_cntl),
		EL_PRIHIGH)) == EL_FULL)
		return;
/*								
 *	Get adapter type and Initialize subid fields
 */							
        switch (vhp->vba_type) {
	  case VBA_3VIA: {adapter_type = ELDT_3VIA;
	                  break;}
	  case VBA_XBIA: {adapter_type = ELDT_XBIA;
	                  break;}
	} /*switch*/

	LSUBID( elp,
		ELCT_DCNTL,              /* device controller class */
	        ELVME_DEV_CNTL,          /* VME device controller */
		adapter_type,            /* what VME adapter set */
		vhp->vbanum,             /* VMEbus number */
		EL_UNDEF,
		VME_CONTROLLER);         /* controller type error */ 

	elvme_ptr = &elp->el_body.elvme_devcntl;

/*
 *      put name string into the packet 
 */
        msg_ptr = devptr->um_ctlrname;
        for (i=1; *msg_ptr++ != '\0' && i > EL_SIZE64; i++) ;
        bcopy(devptr->um_ctlrname, elvme_ptr->module, i);
        elvme_ptr->module[i-1] = '\0' ;

/*
 *      fill in rest of packet 
 */
        elvme_ptr->num  = devptr->um_ctlr ;
        elvme_ptr->csr1 = devptr->um_addr ;
        elvme_ptr->csr2 = devptr->um_addr2 ;

        if (text == NULL) text = "NO ERROR MESSAGE ENTERED BY DRIVER" ;
	elvme_ptr->el_vme_error_msg.msg_len = sizeof(text);
        bcopy(text, elvme_ptr->el_vme_error_msg.msg_asc, sizeof(text));

	switch (adapter_type) {
	  case ELDT_3VIA: {
            elvme_ptr->elvba.elvba_reg.elmvib.mvib_viacsr = *Xviaregs.viacsr ;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_csr = *Xviaregs.csr ;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_vfadr = *Xviaregs.vfadr;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_cfadr = *Xviaregs.cfadr;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_ivs = *Xviaregs.ivs;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_besr = *Xviaregs.besr;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_errgi = *Xviaregs.errgi;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_lvb = *Xviaregs.lvb;
	    elvme_ptr->elvba.elvba_reg.elmvib.mvib_err = *Xviaregs.err;
	    break; 
          }
	  case ELDT_XBIA: {
	    xbireg = (struct xbi_reg *)vhp->vbavirt;

	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_dtype = xbireg->xbi_dtype;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_xbe = xbireg->xbi_xbe;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_fadr = xbireg->xbi_fadr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_arear = xbireg->xbi_arear;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_aesr = xbireg->xbi_aesr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_aimr = xbireg->xbi_aimr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_aivintr=xbireg->xbi_aivintr;
	    elvme_ptr->elvba.elvba_reg.elxbia.xbia_adg1 = xbireg->xbi_adg1;

	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vdcr = *Xvibregs.vdcr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vesr =  *Xvibregs.vesr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vfadr =  *Xvibregs.vfadr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vicr =  *Xvibregs.vicr ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vvor =  *Xvibregs.vvor ;
	    elvme_ptr->elvba.elvba_reg.elxbia.xvib_vevr =  *Xvibregs.vevr ;
            break;
          }
	} /*switch*/

/*
 *	Validate error log packet				
 */

	EVALID( elp );

} /*logctlr*/


