#ifndef	lint
static        char    *sccsid = "@(#)scs_dg.c	4.1  (ULTRIX)        7/2/90";
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
 *		Systems Communication Services
 *
 *   Abstract:	This module contains Systems Communication Services( SCS )
 *		datagram communication service functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 27, 1985
 *
 *   Function/Routines:
 *
 *   scs_alloc_dg		Allocate Application Datagram Buffer
 *   scs_dealloc_dg		Deallocate Application Datagram Buffer
 *   scs_queue_dgs		Adjust Number of Receive Datagram Buffers
 *   scs_send_dg		Send Application Datagram
 *
 *   Modification History:
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   06-Mar-1989	Todd M. Katz		TMK0002
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, restructured
 *	code paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#include		"../h/errlog.h"
#include		"../h/smp_lock.h"
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

/* External Variables and Routines.
 */
extern	SCSIB		lscs;
extern	CBVTDB		*scs_cbvtdb;

/*   Name:	scs_alloc_dg	- Allocate Application Datagram Buffer
 *
 *   Abstract:	This function allocates an application datagram buffer on a
 *		specific logical SCS connection.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	connid			-  Identification of logical SCS connection
 *   lscs			- Local system permanent information
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	buf			-  Address of application data buffer
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Allocated application datagram buffer
 *   RET_ALLOCFAIL		- Failed to allocate storage
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 */
u_long
scs_alloc_dg( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register SCSH	*scsbp;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* The steps involved in allocating an application datagram buffer are:
     *
     * 1. Lock and retrieve the CB.
     * 2. Invoke a PD specific function to allocate a datagram buffer.
     * 3. Unlock the CB.
     * 4. Return an appropriate status.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_OPEN ) {
	status = RET_INVCSTATE;
    } else if(( scsbp = ( *cb->Alloc_dg )( cb->pccb ))) {
	csb->buf = Scs_to_appl( scsbp );
    } else {
	status = RET_ALLOCFAIL;
    }
    Unlock_cbvte( cbvte );
    return( status );
}

/*   Name:	scs_dealloc_dg	- Deallocate Application Datagram Buffer
 *
 *   Abstract:	This function deallocates an application datagram buffer on a
 *		specific logical SCS connection.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	buf			-  Address of application data buffer
 *	connid			-  Identification of logical SCS connection
 *   lscs			- Local system permanent information
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	buf			-  NULL
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Deallocated application datagram buffer
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 */
u_long
scs_dealloc_dg( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* The steps involved in deallocating an application datagram buffer are:
     *
     * 1. Lock and retrieve the CB.
     * 2. Invoke a PD specific routine to deallocate the datagram buffer.
     * 3. Unlock the CB.
     * 4. Return an appropriate status.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate == CS_OPEN	    ||
	 cb->cinfo.cstate == CS_DISCONN_REC ||
	 cb->cinfo.cstate == CS_PATH_FAILURE ) {
	( void )( *cb->Dealloc_dg )( cb->pccb, Appl_to_scs( csb->buf ));
	csb->buf = NULL;
    } else {
	status = RET_INVCSTATE;
    }
    Unlock_cbvte( cbvte );
    return( status );
}

/*   Name:	scs_queue_dgs	- Adjust Number of Receive Datagram Buffers
 *
 *   Abstract:	This function adjusts the number of receive datagram buffers
 *		available on a specific logical SCS connection.  The number of
 *		receive datagram buffers may either be increased or decreased.
 *		It also makes a specific datagram buffer available as a receive
 *		datagram buffer on a specific logical SCS connection.
 *
 *		Partial increases or decreases in the number of receive
 *		datagram buffers may occur.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	buf			-  Address of application data buffer
 *	connid			-  Identification of logical SCS connection
 *	ov3.nbufs		-  Number datagram buffers to add/withdrawal
 *   lscs			- Local system permanent information
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.dg_credit		-  Number of local datagram credits
 *   csb			- Communication Services Block pointer
 *	buf			-  NULL
 *	ov3.nbufs		-  Number datagram credits following adjustment
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Modified number of receive datagram buffers
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 */
u_long
scs_queue_dgs( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register SCSH	*scsbp;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* Adjustments in receive datagram buffers proceed as follows:
     *
     * 1. Lock and retrieve the CB.
     * 2. Convert any specified application datagram buffer into a receive
     *    datagram buffer using a PD specific routine.
     * 3. Adjust the number of receive datagram buffers using PD specific
     *	  functions and routines.  The number of receive datagram buffers is
     *	  increased by adding allocated port specific datagram buffers to the
     *    appropriate local port's free datagram pool.  The number is decreased
     *    by deallocating receive datagram buffers withdrawn from the
     *	  appropriate local port's free datagram pool.
     * 4. Perform connection bookkeeping.
     * 5. Retrieve the adjusted number of receive datagram buffers.
     * 6. Unlock the CB.
     * 7. Return an appropriate status.
     * 
     * Adjustments are immediately aborted on encountering any errors.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate == CS_OPEN ) {
	if( csb->buf ) {
	    ( void )( *cb->Add_dg )( cb->pccb, Appl_to_scs( csb->buf ));
	    csb->buf = NULL;
	    ++cb->cinfo.dg_credit;
	}
	if( csb->Nbufs > 0 ) {
	    for( ; csb->Nbufs > 0; --csb->Nbufs, ++cb->cinfo.dg_credit ) {
		if(( scsbp = ( *cb->Alloc_dg )( cb->pccb ))) {
		    ( void )( *cb->Add_dg )( cb->pccb, scsbp );
		} else {
		    break;
		}
	    }
	} else if( csb->Nbufs < 0 ) {
	    for( ;
		 csb->Nbufs < 0 && cb->cinfo.dg_credit;
		 ++csb->Nbufs, --cb->cinfo.dg_credit ) {
		if(( scsbp = ( *cb->Remove_dg )( cb->pccb ))) {
		    ( void )( *cb->Dealloc_dg )( cb->pccb, scsbp );
		} else {
		    break;
		}
	    }
	}
    } else {
	status = RET_INVCSTATE;
    }

    csb->Nbufs = cb->cinfo.dg_credit;
    Unlock_cbvte( cbvte );
    return( status );
}

/*   Name:	scs_send_dg	- Send Application Datagram
 *
 *   Abstract:	This function initiates transmission of an application datagram
 *		over a specific logical SCS connection.  Two options exist for
 *		disposal of the buffer following transmission:
 *
 *		1. Convert the buffer into a receive datagram buffer.
 *		2. Deallocate the buffer.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	buf			-  Address of application data buffer
 *	connid			-  Identification of logical SCS connection
 *	ov3.disposal		-  Disposition of sent buffer
 *	size			-  Application data transfer size
 *   lscs			- Local system permanent information
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.dg_credit		-  Number of local datagram credits
 *	cinfo.dgs_snt		-  Number of datagrams transmitted
 *   csb			- Communication Services Block pointer
 *	buf			-  NULL
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Successfully initiated datagram transmission
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *   RET_NOSUPPORT		- Function unsupported on SCS connection
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.  Locking the CB also
 *		prevents PB deletion as required by PD routines which transmit
 *		datagrams.
 */
u_long
scs_send_dg( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register SCSH	*scsbp;
    register CBVTE	*cbvte;
    register void	( *send )();
    register u_long	status = RET_SUCCESS;

    /* Transmission of an application datagram proceeds as follows:
     *
     * 1. Lock and retrieve the CB.
     * 2. Initialize the SCS header of the application datagram buffer.
     * 3. Perform connection bookkeeping.
     * 4. Invoke a PD specific routine to initiate datagram transmission.
     * 5. Unlock the CB.
     * 6. Return an appropriate status.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_OPEN ) {
	status = RET_INVCSTATE;
    } else if(( send = cb->Send_dg )) {
	scsbp = Appl_to_scs( csb->buf );
	csb->buf = NULL;
	U_long( scsbp->mtype ) = SCS_APPL_DG;
	Move_connid( cb->cinfo.rconnid, scsbp->rconnid )
	Move_connid( cb->cinfo.lconnid, scsbp->sconnid )
	if( csb->Disposal == RECEIVE_BUF ) {
	    ++cb->cinfo.dg_credit;
	}
	Event_counter( cb->cinfo.dgs_snt )
	( void )( *send )( cb->pccb,
			   cb->pb,
			   scsbp,
			   ( csb->size + sizeof( SCSH )),
			   csb->Disposal,
			   SCSDG );
    } else {
	status = RET_NOSUPPORT;
    }
    Unlock_cbvte( cbvte );
    return( status );
}
