#ifndef lint
static char *sccsid = "@(#)bici.c	4.1	(ULTRIX)	7/2/90";
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
 *		BI based port autoconfiguration routines.
 *
 *   Creator:	Todd M. Katz	Creation Date:	March 22, 1986
 *
 *   Functions/Routines:
 *
 *   biciinit			BI to CI Autoconfiguration Glue Routine
 *
 *   Modification History:
 *
 *   06-Jun-90		Pete Keilty
 *	CIADAP is now KMALLOC and passed to ci_probe().
 *	Also new structure added CIISR which is now used to hold the
 *	device isr routine and pccb.
 *
 *   09-Nov-89		David E. Eiche		DEE0080
 *	Changed parameter in call to ci_probe() from IC_BI to
 *	ICT_BI, matching the new definition in sysap.h.
 *
 *   16-Jun-89		Darrell A. Dunnuck
 *	Removed cpup as an arg passed to functons.
 *
 *   06-Mar-1989	Todd M. Katz		TMK0005
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   05-Mar-1988	Todd M. Katz		TMK0004
 *	1. Determine the hardware port type and pass it into ci_probe().
 *	2. Add several include files but remove msisysap.h
 *	3. CIBCI hardware ports now have their own interrupt service routine,
 *	   cibci_isr().  Make the appropriate changes.
 *	4. CIBCA local ports are never unmapped except when the local port is
 *	   marked broken and has been permanently shutdown.  Therefore, as
 *	   there is never any need to re-map them, eliminate initialization of
 *	   the CIADAP field mapped_isr for these hardware port types.
 *	5. Remove conditional compilations based upon the definition NCI( ie-
 *	   always include the "conditional" code because this constant is
 *	   always non-zero when this module is compiled ).
 *	6. Print out type of CI adapter in addition to where it is located.
 *	7. Do not panic on discovery of unacceptable/unknown hardware port
 *	   types.  Instead, print out a message and skip probing.
 *
 *   25-Mar-1988	Todd M. Katz		TMK0003
 *	Verify the hardware CI port type by querying the BIIC device type
 *	register and panicing on discovery of an unacceptable port type.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0002
 *	Invoke the function config_set_alive() for each CI adapter probed.
 *	This serves to mark the adapter alive and stores relevant information
 *	about it within the appropriate configuration adapter structure.
 *
 *   08-Dec-1987	Todd M. Katz		TMK0001
 *	Re-formatted module, revised comments, and added SMP comments.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/kmalloc.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#include		"../h/errlog.h"
#include		"../io/bi/bireg.h"
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
extern	struct bidata	bidata[];
extern	CIISR		ci_isr[];
extern	void		ci_probe(), cibca_isr(), cibci_isr(), ci_unmapped_isr();

/*
 *   Name:	biciinit	- BI to CI Autoconfiguration Glue Routine
 *
 *   Abstract:	This routine is invoked whenever the BI probe routine
 *		discovers a CI port during autoconfiguration.
 *
 *   Inputs:
 *
 *   0				- Interrupt processor level
 *   bidata			- BI device/adapter information
 *   binode			- BI node number
 *   binumber			- BI number
 *   ci_adap			- Vector of CI Adapter Interface Blocks
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   numci			- Number of CI ports AND number of this CI port
 *   nxv			- Adapter I/O space virtual address
 *   nxp			- Adapter I/O space physical address
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
biciinit( nxv, nxp, binumber, binode )
    register u_char	*nxv;
    u_char		*nxp;
    register u_long	binumber;
    register u_long	binode;
{
    register u_long	hpt;
    register CIADAP	*ciadap;

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
    ( void )printf( "ci%d at vaxbi%d node %d ", numci, binumber, binode );
    switch((( struct bi_regs * )nxv )->bi_typ & BITYP_TYPE ) {
	case BI_CIBCI:
	    ( void )printf( "(CIBCI)\n" );
	    hpt = HPT_CIBCI;
	    break;

	case BI_CIBCA:
	    if((( struct bi_regs * )nxv )->bi_typ & CIBCA_DEV_BCABA ) {
		( void )printf( "(CIBCA-BA)\n" );
		hpt = HPT_CIBCA_BA;
	    } else {
		( void )printf( "(CIBCA-AA)\n" );
		hpt = HPT_CIBCA_AA;
	    }
	    break;

	default:
	    ( void )printf( "(0x%04x)\n", (( struct bi_regs * )nxv )->bi_typ );
	    hpt = 0;
	    break;
    }
    KM_ALLOC( ciadap, CIADAP *, sizeof( CIADAP ), KM_SCA, KM_NOW_CL_CA )
    if( ciadap == 0 ) {
        ( void )ci_log_initerr( numci, ICT_BI, hpt, FE_INIT_NOMEM );
        ci_isr[ numci ].isr = ci_unmapped_isr;
    } else if( numci >= NCI_SUPPORTED ) {
	( void )printf( "ci%d unsupported\n", numci );
    } else if( numci >= nNCI ) {
	( void )printf( "ci%d not configured\n", numci );
    } else if( hpt == 0 ) {
	( void )printf( "ci%d has unsupported hardware port type\n", numci );
    } else {

	switch( hpt ) {
	    case HPT_CIBCI:
        	ci_isr[ numci ].isr = cibci_isr;
		ciadap->isr = cibci_isr;
		ciadap->mapped_isr = cibci_isr;
		break;

	    case HPT_CIBCA_AA:
	    case HPT_CIBCA_BA:
        	ci_isr[ numci ].isr = cibca_isr;
		ciadap->isr = cibca_isr;
		break;
	}
	ciadap->viraddr = nxv;
	ciadap->iopte = &Sysmap[ btop((( u_long )nxv ) & 0x7fffffff )];
	ciadap->phyaddr = nxp;
	ciadap->npages = CI_ADAPSIZE;
	ciadap->Binum = binumber;
	ciadap->Binode = binode;
	ciadap->Biic_int_ctrl = SCB_BI_LWOFFSET( binode, LEVEL14 );
	ciadap->Biic_int_dst = bidata[ binumber ].biintr_dst;
	*SCB_BI_VEC_ADDR( binumber, binode, LEVEL14 ) =
		scbentry( ciintv[ numci ], SCB_ISTACK );
	( void )ci_probe( numci, nxv, ICT_BI, hpt , ciadap );
	( void )config_set_alive( "ci", numci, binumber, binode );
    }
    numci++;
}
