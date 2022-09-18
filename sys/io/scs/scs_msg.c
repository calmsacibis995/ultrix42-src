#ifndef	lint
static char *sccsid = "@(#)scs_msg.c	4.1	(ULTRIX)	7/2/90";
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
 *		sequenced message communication service functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 27, 1985
 *
 *   Function/Routines:
 *
 *   scs_alloc_msg		Allocate Application Message Buffer
 *   scs_dealloc_msg		Deallocate Application Message Buffer
 *   scs_mod_credits		Adjust Maximum Number of Send Credits
 *   scs_add_credit		Add back Reserved Send Credit
 *   scs_rsv_credit		Reserve Send Credit for Emergency Use
 *   scs_send_msg		Send Application Sequenced Message
 *
 *   Modification History:
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   06-Mar-1989	Todd M. Katz		TMK0003
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   14-Mar-1988	Todd M. Katz		TMK0002
 *	Fixed typo in scs_send_msg by changing one instance of "==" to "="
 *	( which is what it should have been in the first place ).
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
#ifdef mips
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#endif mips
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
extern	struct lock_t	lk_scadb;
extern	CBVTDB		*scs_cbvtdb;
extern	u_long		scs_cushion;
extern	void		scs_dispose_msg();

/*   Name:	scs_alloc_msg	- Allocate Application Message Buffer
 *
 *   Abstract:	This function allocates an application message buffer on a
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
 *   RET_SUCCESS		- Allocated application message buffer
 *   RET_ALLOCFAIL		- Failed to allocate storage
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 */
u_long
scs_alloc_msg( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register SCSH	*scsbp;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* Allocations of application message buffers proceed as follows:
     *
     * 1. Lock and retrieve the CB.
     * 2. Invoke a PD specific function to allocate a port specific message
     *	  buffer.
     * 3. Unlock the CB.
     * 4. Return an appropriate status.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_OPEN ) {
	status = RET_INVCSTATE;
    } else if(( scsbp = ( *cb->Alloc_msg )( cb->pccb ))) {
	csb->buf = Scs_to_appl( scsbp );
    } else {
	status = RET_ALLOCFAIL;
    }
    Unlock_cbvte( cbvte );
    return( status );
}

/*   Name:	scs_dealloc_msg	- Deallocate Application Message Buffer
 *
 *   Abstract:	This function deallocates an application message buffer on a
 *		specific logical SCS connection.  Under appropriate conditions
 *		the message buffer is converted into a receive message buffer
 *		for the logical SCS connection instead of deallocated.
 *
 *		Conversion of a message buffer into a receive message message
 *		buffer may trigger explicit extension of send credits across
 *		the logical SCS connection.
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
 *   RET_SUCCESS		- Deallocated application message buffer
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 */
u_long
scs_dealloc_msg( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* Deallocations of application message buffers proceed as follows:
     *
     * 1. Lock and retrieve the CB.
     * 2. Dispose of the application message buffer according to the current
     *	  credit situation whenever the connection is open; otherwise invoke a
     *    PD specific routine to deallocate the port specific message buffer.
     * 3. Unlock the CB.
     * 4. Return an appropriate status.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate == CS_OPEN ) {
	( void )scs_dispose_msg( cb, Appl_to_scs( csb->buf ));
	csb->buf = NULL;
    } else if( cb->cinfo.cstate == CS_DISCONN_REC ||
	      cb->cinfo.cstate == CS_PATH_FAILURE ) {
	( void )( *cb->Dealloc_msg )( cb->pccb, Appl_to_scs( csb->buf ));
	csb->buf = NULL;
    } else {
	status = RET_INVCSTATE;
    }
    Unlock_cbvte( cbvte );
    return( status );
}

/*   Name:	scs_mod_credits	- Adjust Maximum Number of Send Credits
 *
 *   Abstract:	This function adjusts the maximum number of send credits that
 *		may be extended to a counterpart across a logical SCS
 *		connection.  This number may either be increased or decreased.
 *
 *		Partial increases or decreases in the maximum number of
 *		extendible send	credits may occur.
 *
 *		Explicit extension of send credits across the logical SCS
 *		connection may occur.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	connid			-  Identification of logical SCS connection
 *	ov3.ncredits		-  Number to adjust maximum send credits by
 *   lscs			- Local system permanent information
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *   scs_cushion		- SCS send credit cushion
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.cbstate		-  CB state
 *	cinfo.init_rec_credit	-  Remote SYSAP's maximum send credit
 *	cinfo.pend_rec_credit	-  Number of credits pending extension
 *   csb			- Communication Services Block pointer
 *	ov3.ncredits		-  Adjusted maximum number of send credits
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Modified maximum number of send credits
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The SCA database is locked in the event explicit extension of
 *		send credits to the SYSAP's remote counterpart is required.
 *
 *		The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 *
 *		The PB is locked whenever it is found necessary to remove a CB
 *		from its SCS CB wait queue.
 */
u_long
scs_mod_credits( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register SCSH	*scsbp;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* Adjustments in the maximum number of send credits proceed as follows:
     *
     * 1. Lock the SCA database.
     * 2. Lock and retrieve the CB.
     * 3. Adjust the number of send credits extendible across the connection
     *	  using PD specific functions and routines.  The maximum number of send
     *    credits may be increased by adding allocated port specific message
     *    buffers to the appropriate local port's free message pool.  The
     *    maximum number of send credits is decreased by deallocating receive
     *    message buffers withdrawn from the appropriate local port's free
     *	  message pool.  Only those buffer representing credits pending
     *	  extension may be withdrawn and deallocated.
     * 4. Perform connection bookkeeping.
     * 5. Retrieve the adjusted maximum number of extendible send credits.
     * 6. Unlock the CB and SCA database.
     * 7. Return an appropriate status.
     * 
     * Adjustments are immediately aborted on encountering any errors.
     *
     * Explicit SCS extension of send credits across the connection to the
     * SYSAP's remote counterpart is requested whenever:
     *
     * 1. SCS is not currently extending credits across the connection.
     * 2. There is at least one credit pending extension.
     * 3. The number of send credits held by the counterpart is less than its
     *    minimum credit requirement plus a local system-wide credit cushion.
     *
     * Pending explicit SCS extensions of send credits are aborted whenever
     * there are no longer any send credits to extend.
     *
     * NOTE: The maximum number of send credits may not be decreased below the
     *	     remote SYSAP's minimum send credit requirement.  In addition, it
     *       may not be decreased by more then the current number of send
     *	     credits pending extension across the connection.
     */
    Check_connid2( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_OPEN ) {
	status = RET_INVCSTATE;
    } else if( csb->Ncredits > 0 ) {
	for( ;
	     csb->Ncredits > 0;
	     ++cb->cinfo.init_rec_credit,
	     ++cb->cinfo.pend_rec_credit,
	     --csb->Ncredits ) {
	    if(( scsbp = ( *cb->Alloc_msg )( cb->pccb ))) {
		( void )( *cb->Add_msg )( cb->pccb, scsbp );
	    } else {
		break;
	    }
	}
	if( cb->cinfo.cbstate == CB_NOT_WAIT  &&
	     cb->cinfo.pend_rec_credit        &&
	     cb->cinfo.rec_credit < ( cb->cinfo.min_rec_credit + scs_cushion)){
	    ( void )scs_request( CB_CREDIT_PEND, cb, cb->pb, NULL );
	}
    } else if( csb->Ncredits < 0 ) {
	for( ;
	     ( csb->Ncredits < 0	 &&
	       cb->cinfo.pend_rec_credit &&
	       cb->cinfo.min_rec_credit < cb->cinfo.init_rec_credit );
	     ++csb->Ncredits,
	     --cb->cinfo.pend_rec_credit,
	     --cb->cinfo.init_rec_credit ) {
	    if(( scsbp = ( *cb->Remove_msg )( cb->pccb ))) {
		( void )( *cb->Dealloc_msg )( cb->pccb, scsbp );
	    } else {
		break;
	    }
	}
	if( cb->cinfo.pend_rec_credit == 0 ) {
	    Remove_pb_waitq( cb )
	}
    }
    csb->Ncredits = cb->cinfo.init_rec_credit;
    Unlock_cbvte( cbvte );
    Unlock_scadb();
    return( status );
}

/*   Name:	scs_add_credit	- Add back Reserved Send Credit
 *
 *   Abstract:	This function adds back a send credit previously reserved by a
 *		SYSAP for emergency communication across a specific logical SCS
 *		connection.
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
 *   cb				- Connection Block pointer
 *	cinfo.reserved_credit	-  Number of credits reserved by SYSAP
 *	cinfo.snd_credit	-  Number of send credits
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Successfully added back reserved send credit
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *   RET_NOCREDITS		- No send credits available to add back
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 */
u_long
scs_add_credit( csb )
    CSB			*csb;
{
    register CB		*cb;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* The steps involved in adding back a reserved send credit are as follows:
     *
     * 1. Lock and retrieve the CB.
     * 2. Add back a reserved send credit.
     * 3. Unlock the CB.
     * 4. Return an appropriate status.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_OPEN ) {
	status = RET_INVCSTATE;
    } else if( cb->cinfo.reserved_credit > 0 ) {
	--cb->cinfo.reserved_credit;
	++cb->cinfo.snd_credit;
    } else {
	status = RET_NOCREDITS;
    }
    Unlock_cbvte( cbvte );
    return( status );
}

/*   Name:	scs_rsv_credit	- Reserve Send Credit for Emergency Use
 *
 *   Abstract:	This function withdraws a send credit from a specific logical
 *		SCS connection and reserves it for emergency communication.
 *		The lack of send credits prevents such reservation.  The SYSAP
 *		is notified of send credit availability through asynchronous
 *		invocation of the connection's control event routine.
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
 *   cb				- Connection Block pointer
 *	cinfo.reserved_credit	-  Number of credits reserved by SYSAP
 *	cinfo.snd_credit	-  Number of send credits
 *	cinfo.status.cwait	-  SYSAP waiting for send credits flag
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Successfully reserved send credit
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *   RET_NOCREDITS		- No send credits available to reserve
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.
 */
u_long
scs_rsv_credit( csb )
    CSB			*csb;
{
    register CB		*cb;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* The steps involved in reserving a send credit for emergency use are as
     * follows:
     *
     * 1. Lock and retrieve the CB.
     * 2. Withdraw and reserve a send credit.
     * 3. Unlock the CB.
     * 4. Return an appropriate status.
     *
     * Remember the lack of send credits on the connection.  The SYSAP is
     * notified when credits are again available.  
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_OPEN ) {
	status = RET_INVCSTATE;
    } else if( cb->cinfo.snd_credit > 0 ) {
	--cb->cinfo.snd_credit;
	++cb->cinfo.reserved_credit;
    } else {
	cb->cinfo.status.cwait = 1;
	status = RET_NOCREDITS;
    }
    Unlock_cbvte( cbvte );
    return( status );
}

/*   Name:	scs_send_msg	- Send Application Sequenced Message
 *
 *   Abstract:	This function initiates transmission of an application
 *		sequenced message over a specific logical SCS connection.  Two
 *		options exist for disposal of the buffer following
 *		transmission:
 *
 *		1. Convert the buffer into a receive message buffer.
 *		2. Deallocate the buffer.
 *
 *		A send credit is exhausted.  The lack of send credits prevents
 *		application sequenced message transmission.  The SYSAP is
 *		notified of send credit availability through asynchronous
 *		invocation of the connection's control event routine.
 *
 *		As pending send credits are piggy backed on the application
 *		sequenced message any pending explicit send credit extension
 *		is aborted.
 *
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
 *	cinfo.cbstate		-  CB state
 *	cinfo.pend_rec_credit	-  0
 *	cinfo.msgs_sent		-  Number of sequenced messages sent
 *	cinfo.rec_credit	-  Send credit held by remote SYSAP
 *	cinfo.snd_credit	-  Number of send credits
 *	cinfo.status.cwait	-  SYSAP waiting for send credits flag
 *   csb			- Communication Services Block pointer
 *	buf			-  NULL
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Successfully initiated message transmission
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *   RET_NOCREDITS		- No send credits available
 *
 *   SMP:	The CB is locked to synchronize access and prevent deletion.
 *		It is indirectly locked through its CBVTE.  Locking the CB also
 *		prevents PB deletion as required by PD routines which transmit
 *		messages.
 *
 *		The PB is specifically locked when it is found necessary to
 *		remove the CB from the PB's work queue.
 */
u_long
scs_send_msg( csb )
    register CSB	*csb;
{
    register CB		*cb;
    register SCSH	*scsbp;
    register CBVTE	*cbvte;
    register u_long	dispose, status = RET_SUCCESS;

    /* Transmissions of application sequenced messages proceed as follows:
     *
     * 1. Lock and retrieve the CB.
     * 2. Allocate a send credit.
     * 3. Abort any explicit SCS send credit extensions in progress( Any send
     *    credits to be extended go out within the outgoing sequenced message
     *    itself ).
     * 4. Initialize the SCS header of the application message buffer.
     * 5. Perform connection bookkeeping.
     * 6. Invoke a PD specific routine to initiate application sequenced
     *	  message transmission
     * 7. Unlock the CB.
     * 8. Return an appropriate status.
     *
     * Lack of send credits on the connection is remembered.  The SYSAP is
     * notified when send credits are again available.
     *
     * The message buffer is converted into a receive message buffer instead of
     * deallocated whenever the current number of receive buffers is less than
     * the maximum number.  Furthermore, the number of send credits pending
     * extension is incremented whenever the message buffer is to be converted
     * into a receive message buffer following its transmission.  This has the
     * effect of extending the send credit for the new receive buffer within
     * the outgoing sequenced message.
     */
    Check_connid( csb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_OPEN ) {
	status = RET_INVCSTATE;
    } else if(( long )--cb->cinfo.snd_credit >= 0 ) {
	if(( dispose = csb->Disposal ) == RECEIVE_BUF ) {
	    ++cb->cinfo.pend_rec_credit;
	} else if(( cb->cinfo.rec_credit +
		  cb->cinfo.pend_rec_credit ) < cb->cinfo.init_rec_credit ) {
	    ++cb->cinfo.pend_rec_credit;
	    dispose = RECEIVE_BUF;
	}
	Remove_pb_waitq( cb )
	scsbp = Appl_to_scs( csb->buf );
	csb->buf = NULL;
	U_long( scsbp->mtype ) = SCS_APPL_MSG;
	if(( scsbp->credit = cb->cinfo.pend_rec_credit )) {
	    cb->cinfo.rec_credit += cb->cinfo.pend_rec_credit;
	    cb->cinfo.pend_rec_credit = 0;
	}
	Move_connid( cb->cinfo.rconnid, scsbp->rconnid )
	Move_connid( cb->cinfo.lconnid, scsbp->sconnid )
	Event_counter( cb->cinfo.msgs_snt )
	( void )( *cb->Send_msg )( cb->pccb,
				   cb->pb,
				   scsbp,
				   ( csb->size + sizeof( SCSH )),
				   dispose );
    } else {
	cb->cinfo.snd_credit = 0;
	cb->cinfo.status.cwait = 1;
	status = RET_NOCREDITS;
    }
    Unlock_cbvte( cbvte );
    return( status );
}
