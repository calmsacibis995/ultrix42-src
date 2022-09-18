#ifndef lint
static char *sccsid = "@(#)xmici.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 - 1989 by			*
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
 ************************************************************************
 *
 *   Facility:	Systems Communication Architecture
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		XMI based port autoconfiguration routines.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 08, 1988
 *
 *   Functions/Routines:
 *
 *   xmiciinit			XMI to CI Autoconfiguration Glue Routine
 *
 *   Modification History:
 *
 *   06-Jun-1990	Pete Keilty
 *	1. CIADAP is KMALLOC and passed to ci_probe().
 *	2. Added new structure CIISR which now holds the isr the 
 *	   device and the pccb.
 *	3. Added preliminary support for CIKMF(dash).
 *
 *   08-Dec-1989	Pete Keilty
 *	Added interrupt level for CIXCD.
 *
 *   09-Nov-89		David E. Eiche		DEE0080
 *	Changed parameter in call to ci_probe() from IC_XMI to
 *	ICT_XMI, matching the new definition in sysap.h.
 *
 *   19-Sep-1989	Pete Keilty
 *	Add support for XCD, remove XCB.
 *
 *   23-May-1989	Darrell A. Dunnuck
 *	Removed cpup from the list of args passed into xmiciinit.  Cpup
 *	is now defined globally -- as part of the new cpusw.
 *
 *   06-Mar-1989	Todd M. Katz		TMK0002
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   09-Feb-1989	Mark A. Parenti
 *	Change include syntax for merged pool.
 *
 *   22-Jun-1988	Todd M. Katz		TMK0001
 *	Remove conditional compilations based upon the definition NCI( ie-
 *	always include the "conditional" code because this constant is always
 *	non-zero when this module is compiled ).
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/kmalloc.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#include		"../h/errlog.h"
#include		"../io/xmi/xmireg.h"
#include		"../data/autoconf_data.c"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"../io/ci/cippdsysap.h"
#include		"../io/ci/cisysap.h"
#include		"../io/msi/msisysap.h"
#include		"../io/bi/bvpsysap.h"
#include		"../io/gvp/gvpsysap.h"
#include		"../io/uba/uqsysap.h"
#include		"../io/sysap/sysap.h"
#include		"../io/ci/cippdscs.h"
#include		"../io/ci/ciscs.h"
#include		"../io/msi/msiscs.h"
#include		"../io/bi/bvpscs.h"
#include		"../io/gvp/gvpscs.h"
#include		"../io/uba/uqscs.h"
#include		"../io/scs/scs.h"
#include		"../io/gvp/gvp.h"
#include		"../io/ci/cippd.h"
#include		"../io/ci/ciport.h"

/* External Variables and Routines.
 */
extern	struct pte	Sysmap[];
extern	CIISR		ci_isr[];
extern	void		ci_probe(), cixcd_isr(), cikmf_isr(), ci_unmapped_isr();

/*
 *   Name:	xmiciinit	- XMI to CI Autoconfiguration Glue Routine
 *
 *   Abstract:	This routine is invoked whenever the XMI configuration routine
 *		discovers a CI port during autoconfiguration.
 *
 *   Inputs:
 *
 *   0				- Interrupt processor level
 *   ci_adap			- Vector of CI Adapter Interface Blocks
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   numci			- Number of CI ports AND number of this CI port
 *   nxv			- Adapter I/O space virtual address
 *   nxp			- Adapter I/O space physical address
 *   xmidata			- XMI device/adapter information
 *   xminode			- XMI node number
 *   xminumber			- XMI number
 *
 *   Outputs:
 *
 *   0				- Interrupt processor level
 *   ciadap			- Target CI Adapter Interface block pointer
 *				   ( INITIALIZED as required )
 *   numci			- Number of CI ports 
 *
 *   SMP:	No locks are required.  This function is only called during
 *		system initialization and at that time only the processor
 *		executing this code is operational.  This guarantees
 *		uncompromised access to all data structures without the added
 *		complications of locking.
 */
xmiciinit( nxv, nxp, xminumber, xminode, xmidata )
    register u_char	*nxv;
    u_char		*nxp;
    register u_long	xminumber;
    register u_long	xminode;
    struct xmidata	*xmidata;
{
    register CIADAP	*ciadap;
    register u_long	hpt;

    /* For the "current" CI adapter to be autoconfigured the following criteria
     * must be met:
     *
     * 1. The currently supported number of CI adapters/system( NCI_SUPPORTED )
     *	  must not have already been exceeded.
     * 2. The configured number of CI adapters/system( nNCI ) must not have
     *    already been exceeded.
     *
     * Failure to meet any one of these criteria results in both an appropriate
     * message and failure to autoconfigure the "current" CI adapter.  Else,
     * the "current" CI adapter undergoes autoconfiguration as follows:
     *
     * 1. The next available CI adapter interface structure is initialized.
     * 2. The CI adapter's SCB interrupt vector is initialized.
     * 3. The CI port driver probe routine is invoked to initiate CI adapter
     *    initialization.
     * 4. Mark the CI adapter alive within the configuration database.
     */
    ( void )printf( "ci%d at xmi%d node %d ", numci, xminumber, xminode );
    switch((( struct xmi_reg * )nxv )->xmi_dtype & XMIDTYPE_TYPE ) {
	case XMI_CIXCD:
	    ( void )printf( "(CIXCD)\n" );
	    hpt = HPT_CIXCD;
	    break;

	case XMI_CIKMF:
	    ( void )printf( "(CIKMF)\n" );
	    hpt = HPT_CIKMF;
	    break;

	default:
	    ( void )printf( "(0x%04x)\n",
			    (( struct xmi_reg * )nxv )->xmi_dtype );
	    hpt = 0;
	    break;
    }

    KM_ALLOC( ciadap, CIADAP *, sizeof( CIADAP ), KM_SCA, KM_NOW_CL_CA )
    if( ciadap == 0 ) {
        ( void )ci_log_initerr( numci, ICT_XMI, hpt, FE_INIT_NOMEM );
        ci_isr[ numci ].isr = ci_unmapped_isr;
    } else if( numci >= NCI_SUPPORTED ) {
	( void )printf( "ci%d unsupported\n", numci );
    } else if( numci >= nNCI ) {
	( void )printf( "ci%d not configured\n", numci );
    } else if( hpt == 0 ) {
	( void )printf( "ci%d has unsupported hardware port type\n", numci );
    } else {

	ciadap->viraddr = nxv;
	ciadap->iopte = &Sysmap[ btop((( u_long )nxv ) & 0x7fffffff )];
	ciadap->phyaddr = nxp;
	ciadap->npages = CI_ADAPSIZE;
	ciadap->Xminum = xminumber;
	ciadap->Xminode = xminode;

	switch( hpt ) {
	    case HPT_CIXCD:
        	ci_isr[ numci ].isr = cixcd_isr;
		ciadap->isr = cixcd_isr;
		ciadap->Xcd_pid = xmidata->xmiintr_dst;
		ciadap->Xcd_pv = 
		     SCB_XMI_LWOFFSET( xminode, LEVEL14 ) | XMI_LEVEL15;
		*SCB_XMI_VEC_ADDR( xmidata, xminumber, xminode, LEVEL14 ) =
	 	     scbentry( ciintv[ numci ], SCB_ISTACK );
		break;

	    case HPT_CIKMF:
        	ci_isr[ numci ].isr = cikmf_isr;
		ciadap->isr = cikmf_isr;
		ciadap->Kmf_pid1 = xmidata->xmiintr_dst;
		ciadap->Kmf_pid2 = xmidata->xmiintr_dst;
		ciadap->Kmf_pid3 = xmidata->xmiintr_dst;
		ciadap->Kmf_pipl1 = XMI_LEVEL15 >> 16;
		ciadap->Kmf_pipl2 = XMI_LEVEL15 >> 16;
		ciadap->Kmf_pipl3 = XMI_LEVEL15 >> 16;
		ciadap->Kmf_pv1 = SCB_XMI_LWOFFSET( xminode, LEVEL14 );
		ciadap->Kmf_pv2 = SCB_XMI_LWOFFSET( xminode, LEVEL15 );
		ciadap->Kmf_pv3 = SCB_XMI_LWOFFSET( xminode, LEVEL14 );
		*SCB_XMI_VEC_ADDR( xmidata, xminumber, xminode, LEVEL14 ) =
	 	     scbentry( ciintv[ numci ], SCB_ISTACK );
/* 2 port
	        ( void )ci_probe( numci, nxv, ICT_XMI, hpt, ciadap );
	        ( void )config_set_alive( "ci", numci, xminumber, xminode );
    		numci++;
        	ci_isr[ numci ].isr = cikmf_isr;
		*SCB_XMI_VEC_ADDR( xmidata, xminumber, xminode, LEVEL15 ) =
	 	     scbentry( ciintv[ numci ], SCB_ISTACK );
*/
		break;
	}
	( void )ci_probe( numci, nxv, ICT_XMI, hpt, ciadap );
	( void )config_set_alive( "ci", numci, xminumber, xminode );
    }
    numci++;
}
