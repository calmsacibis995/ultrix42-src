#ifndef	lint
static char *sccsid = "@(#)gvp_subr.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Generic Vaxport Port Driver
 *
 *   Abstract:	This module contains Generic Vaxport Port Driver( GVP )
 *		miscellaneous functions and routines.  Some of these
 *		functions are invokable only by port drivers while others
 *		are for use only by SCS.
 *
 *   Creator:	Todd M. Katz	Creation Date:	September 24, 1987
 *
 *   Function/Routines:
 *
 *   gvp_info_gvp		Return Generic Vaxport Driver Information
 *   gvp_initialize		Initialize Generic Vaxport Driver
 *
 *   Modification History:
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's double mapped buffer & pte's
 *	into a Vaxmap of the system.
 *	CI/BVP ports need to have VAX pte's and system addresses.	
 *
 *   06-Apr-1989	Pete Keilty
 *	Add include file smp_lock.h
 *
 *   07-Mar-1989	Todd M. Katz		TMK0002
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made GVP
 *	completely independent from underlying port drivers, restructured code
 *	paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#include		"../h/kmalloc.h"
#include		"../h/errlog.h"
#include		"../h/dyntypes.h"
#include		"../h/smp_lock.h"
#include		"../machine/cpu.h"
#include		"../machine/common/cpuconf.h"
#include		"../machine/pte.h"
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
#include		"../io/scs/scamachmac.h"

/* External Variables and Routines.
 */
extern	int		cpu;
extern	SCSIB		lscs;
extern	GVPBDDB		*gvp_bddb;
extern	u_long		gvp_max_bds, gvp_queue_retry;
extern	void		( *gvp_info )(), gvp_info_gvp();

/*   Name:	gvp_info_gvp	- Return Generic Vaxport Driver Information
 *
 *   Abstract:	This function returns generic Vaxport driver information.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *   gvp_queue_retry		- Queuing failure retry account
 *   scsib			- SCS Information Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   scsib			- SCS Information Block pointer
 *	gvp_qretry		-  GVP queuing failure retry count
 *	gvp_free_bds		-  Number of free GVPBDs
 *
 *   SMP:	The GVP buffer descriptor database is locked to postpone all
 *		modifications to the free GVPBD list while it is being
 *		traversed.
 */
void
gvp_info_gvp( scsib )
    register SCSIB	*scsib;
{
    register GVPBD	*bd;

    /* The following current items of Generic Vaxport Driver information are
     * returned:
     *
     * 1. GVP queuing failure retry count.
     * 2. Number of free GVPBDs.
     *
     * The GVP buffer descriptor database is locked while the free GVPBD
     * list is traversed.
     */
    scsib->gvp_qretry = gvp_queue_retry;
    Lock_gvpbd()
    for( bd = gvp_bddb->free_bd, scsib->gvp_free_bds = 0;
	 bd;
	 bd = bd->next_free_bd, ++scsib->gvp_free_bds ) {}
    Unlock_gvpbd()
}

/*   Name:	gvp_initialize	- Initialize Generic Vaxport Driver
 *
 *   Abstract:	This function initializes the generic Vaxport driver and all of
 *		its data structures.  Each port driver may call this function
 *		once during its own initialization.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *   gvp_max_bds		- Maximum number of Buffer Descriptors
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *				  ( INITIALIZED )
 *   lscs			- Local system permanent information
 *	max_gvpbds		-  Maximum number of GVPBDs
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- GVP successfully initialized
 *   RET_ALLOCFAIL		- Storage allocation failure occurred
 *
 *   SMP:	No locks are required even though shared data structures are
 *		manipulated.  This function is only called during system
 *		initialization and at that time only the processor executing
 *		this code is operational.  This guarantees uncompromised access
 *		to any shared data structure without locking.
 *
 *		The GVP buffer descriptor database lock structure is
 *		initialized.
 */
u_long
gvp_initialize()
{
    register GVPBD	*bdp;
    register long	index;
    register u_long	size, status = RET_SUCCESS;

    /* Immediately return success if the GVP has already been initialized.
     * Otherwise, initialize the generic Vaxport driver as follows:
     *
     * 1. Allocate and zero all the dynamic memory required for the generic
     *	  Vaxport buffer descriptor database.  Immediately return if
     *	  insufficient dynamic memory is available.
     * 2. Initialize relevant local system information.
     * 3. Initialize the variable containing the address of GVP routine
     *	  returning GVP information.
     * 4. Initialize the GVP buffer descriptor database.
     * 5. Initialize the key and lock structure portions of each GVPBD and
     *	  place all the GVPBDs onto the GVPBD free list.
     * 6. Return an appropriate status.
     */
    if( gvp_bddb == 0 ) {
	size = sizeof( GVPBDDB ) + ( sizeof( GVPBD ) * gvp_max_bds );
	KM_ALLOC( gvp_bddb, GVPBDDB *, size, KM_SCA, KM_NOW_CL_CA )
	if( gvp_bddb ) {
	    gvp_info = gvp_info_gvp;
	    lscs.max_gvpbds = gvp_max_bds;
	    gvp_bddb->bdt = ( GVPBD * )( gvp_bddb + 1 );
	    U_long( gvp_bddb->size ) = size;
	    gvp_bddb->type = DYN_GVPBDDB;
	    Init_gvpbd_lock()
	    for( index = ( lscs.max_gvpbds - 1 ), bdp = gvp_bddb->bdt + index;
		 index >= 0;
		 --index, --bdp ) {
		bdp->key = 1;
		bdp->next_free_bd = gvp_bddb->free_bd;
		gvp_bddb->free_bd = bdp;
	    }
	    Dm_bddbifISIS
	} else {
	    status = RET_ALLOCFAIL;
	}
    }
    return( status );
}
