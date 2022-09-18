#ifndef	lint
static char *sccsid = "@(#)scs_event.c	4.1	(ULTRIX)	7/2/90";
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
 *		event notification routines.  It includes routines invoked
 *		by port drivers to notify SCS.
 *
 *   Creator:	Todd M. Katz	Creation Date:	July 7, 1985
 *
 *   Function/Routines:
 *
 *   scs_data_done		Block Data Transfer Completed
 *   scs_dg_rec			Application Datagram Received
 *   scs_msg_rec		Sequenced Message Received
 *   scs_msg_snt		Application Sequenced Message Sent
 *   scs_new_path		New Path to System Discovered
 *   scs_path_crash		Path to System Crashed
 *
 *   Modification History:
 *
 *   18-Apr-1989	Pete Keilty
 *	Added smp_lock.h, and extern struct lock_t lk_scadb.
 *
 *   21-Feb-1989	Todd M. Katz		TMK0005
 *	Fix a bug introduced by TMK0004.  Modify scs_path_crash() to insure
 *	that the local variable "event" is zeroed prior to the processing of
 *	each Connection Block associated with the failed path.
 *
 *   11-Feb-1989	Todd M. Katz		TMK0004
 *	1. Add support for SCS event logging.  Log abortions of connection
 *	   establishment and terminations of fully established SCS connections
 *	   due to path failure.
 *	2. Include header file ../vaxmsi/msisysap.h.
 *
 *   20-Aug-1988	Todd M. Katz		TMK0003
 *	SCA event codes have been completed revised.  All former SCS path crash
 *	codes are now defined as either error events or severe error events.
 *	The path crash attribute is only applied by the individual port driver
 *	routines responsible for crashing paths and only when the crashed path
 *	is currently open.
 *
 *   09-Jul-1988	Todd M. Katz		TMK0002
 *	Update comments.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, restructured
 *	code paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/time.h"
#include		"../h/param.h"
#include		"../h/ksched.h"
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
extern	pbq		scs_timeoutq;
extern	cbq		scs_listeners;
extern	struct timezone tz;
extern	void		scs_dispose_msg(), scs_init_cmsb(), scs_log_event(),
			unksched();

/*   Name:	scs_data_done	- Block Data Transfer Completed
 *
 *   Abstract:	This SCS event notification routine is asynchronously invoked
 *		following PD completion of a specific block data transfer.  The
 *		PD should not have notified SCS of the failure of the path over
 *		which the block data transfer occurred although in a SMP
 *		environment this may prove unavoidable.
 *
 *		The transfer initiating SYSAP is notified of transfer
 *		completion through asynchronous invocation of the connection's
 *		control event routine and the send credit temporarily withdrawn
 *		during block data transfer is returned.  The latter may trigger
 *		notification of the appropriate SYSAP of the availability of
 *		send credits on the connection through asynchronous invocation
 *		of the connection's control event routine.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *   scsbp			- Address of SCS header in buffer
 *				   ( READ ONLY )
 *   tid			- Transaction identification pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.ntransfers	-  Number of block data transfers in progress
 *	cinfo.snd_credit	-  Number of send credits
 *	cinfo.status.cwait	-  0
 *
 *   SMP:	The SCA database is locked for PB retrieval.  It also prevents
 *		premature PB deletion as required by PD routines which crash
 *		paths.
 *
 *		The CB is locked to synchronize access and to prevent deletion.
 *		It is indirectly locked through its CBVTE.  Locking the CB also
 *		prevents premature PB deletion as required by scs_init_cmsb().
 *
 *		The CBVTE semaphore is incremented prior to any SYSAP
 *		notification to prevent any changes in connection state while
 *		both potential SYSAP notifications are in progress.  The
 *		semaphore is decremented following completion of all SYSAP
 *		notifications.
 *
 *		PCCB addresses are always valid allowing access to static
 *		fields because PCCBs are never deleted once SCS becomes aware
 *		of their existence.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
scs_data_done( pccb, scsbp, tid )
    PCCB		*pccb;
    SCSH		*scsbp;
    register TID	*tid;
{
    register PB		*pb;
    register CB		*cb;
    u_long		event;
    register CBVTE	*cbvte;
    register u_long	credit;
    register void	( *control )();
    CMSB		data_cmsb, credit_cmsb;

    /* Block data transfer completion notifications are processed as follows:
     *
     * 1. Lock and retrieve the CB targeted by the transaction identification
     *    number.
     * 2. Validate the connection.
     * 3. Decrement the number of transfers currently in progress on the
     *	  connection.
     * 4. Return the send credit temporarily withdrawn during the block data
     *    transfer.
     * 5. Increment the CBVTE semaphore.
     * 6. Unlock the CB.
     * 7. Notify the SYSAP of send credit availability on the connection if it
     *    was waiting for them to become available.
     * 8. Notify the SYSAP of the completion of its block data transfer.
     * 9. Decrement the CBVTE semaphore.
     *
     * Incrementing the appropriate CBVTE semaphore protects the connection
     * against all changes in state for the duration of both SYSAP
     * notifications and while any call backs on the connection are in
     * progress.
     *
     * The path on which the block data transfer completed is crashed under any
     * of the following circumstances:
     *
     * 1. The CB can not be retrieved.
     * 2. A connection identification number mismatch exists.
     * 3. The connection is in an inappropriate state.
     * 4. The connection does not have any block data transfers in progress.
     *
     * None of these circumstances are ever expected in a single processor
     * environment.  This explains the drastic actions taken when they occur.
     * However, the occurrence of any of these circumstances can be expected in
     * a SMP environment, albeit with an extremely low frequency, whenever a
     * path fails.  This is because no mechanisms exist to coordinate PD event
     * notifications of SCS as there are mechanisms to coordinate SCS
     * notifications of SYSAPs.  This allows PD notifications of block transfer
     * completion to actually occur after PD notifications of path failure
     * even though PDs initiate the former notifications first.  The actions
     * taken remain the same as in the single processor case even though they
     * are drastic and may well result in double crashing of the path.  There
     * is no other alternative.
     *
     * The buffer provided to this routine is READ ONLY and is only provided
     * for PB retrieval if it becomes necessary to crash the path.
     */
    if( tid->lconnid.index > ( lscs.max_conns - 1 )) {
	cbvte = NULL;
	event = SE_BADCONNID;
    } else {
	cbvte = Get_cbvte( tid->lconnid );
	Lock_cbvte( cbvte )
	cb = Get_cb( cbvte );
	if( tid->lconnid.seq_num != cbvte->connid.seq_num ) {
	    event = SE_BADCONNID;
	} else if( cb->cinfo.cstate != CS_OPEN &&
		  cb->cinfo.cstate != CS_DISCONN_REC ) {
	    event = SE_BADCSTATE;
	} else if( cb->cinfo.ntransfers-- > 0 ) {
	    control = cb->control;
	    ++cb->cinfo.snd_credit;
	    if( cb->cinfo.status.cwait && cb->cinfo.cstate == CS_OPEN ) {
		cb->cinfo.status.cwait = 0;
		credit = 1;
		( void )scs_init_cmsb( CRE_CREDIT_AVAIL,
				       ADR_SUCCESS,
				       &credit_cmsb,
				       cb,
				       cb->pb,
				       0 );
	    } else {
		credit = 0;
	    }
	    ( void )scs_init_cmsb( CRE_BLOCK_DONE,
				   ADR_SUCCESS,
				   &data_cmsb,
				   cb,
				   cb->pb,
				   tid->blockid );
	    Incr_cbvte_sem( cbvte )
	    Unlock_cbvte( cbvte )
	    if( credit ) {
		( void )( *control )( CRE_CREDIT_AVAIL, &credit_cmsb );
	    }
	    ( void )( *control )( CRE_BLOCK_DONE, &data_cmsb );
	    Decr_cbvte_sem( cbvte )
	    return;
	} else {
	    cb->cinfo.ntransfers = 0;
	    event = SE_NOTRANSFERS;
	}
    }
    if( cbvte ) {
	Unlock_cbvte( cbvte )
    }
    Lock_scadb()
    pb = ( *pccb->Get_pb )( pccb, scsbp, BUF );
    ( void )( *pccb->Crash_path )( pccb, pb, event, 0, NULL );
    Unlock_scadb()
}

/*   Name:	scs_dg_rec	- Application Datagram Received
 *
 *   Abstract:	This SCS event notification routine is asynchronously invoked
 *		whenever a PD receives an application datagram.  SCS disposes
 *		of the received application datagram in one of two ways:
 *
 *		1. The application datagram is handed off to the appropriate
 *		   SYSAP through asynchronous invocation of the connection's
 *		   datagram event routine.
 *		2. The application datagram buffer is added to the appropriate
 *		   port's free datagram buffer pool.
 *
 *		NOTE: It is possible for SCS to have been previously notified
 *		      of the failure of the path over which the application
 *		      datagram was received.  This is because datagrams may be
 *		      both sent and received over failed paths.  In such cases
 *		      the datagram is just returned to the appropriate port's
 *		      free datagram buffer pool.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *   lk_scadb			- SCA database lock structure
 *   scsbp			- Address of SCS header in datagram buffer
 *   scs_cbvtdb			- CB vector table database pointer
 *   size			- Size of application data + SCS header
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.dg_credit		-  Number of local datagram credits
 *	cinfo_dgs_discard	-  Number of datagrams discarded
 *	cinfo_dgs_rec		-  Number of datagrams received
 *
 *   SMP:	The SCA database is locked for PB retrieval.  It also prevents
 *		premature PB deletion as required by PD routines which crash
 *		paths.
 *
 *		The CB is locked to synchronize access and to prevent deletion.
 *		It is indirectly locked through its CBVTE.  Locking the CB also
 *		prevents premature PB deletion as required by scs_init_cmsb().
 *
 *		The CBVTE semaphore is incremented prior to SYSAP reception of
 *		the received application datagram to prevent any changes in
 *		connection state while SYSAP reception is in progress.  The
 *		semaphore is decremented after SYSAP reception of the
 *		application datagram completes.
 *
 *		PCCB addresses are always valid allowing access to static
 *		fields because PCCBs are never deleted once SCS becomes aware
 *		of their existence.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
scs_dg_rec( pccb, scsbp, size )
    PCCB		*pccb;
    register SCSH	*scsbp;
    u_long		size;
{
    CSB			dg_csb;
    register CB		*cb;
    register CBVTE	*cbvte;
    register u_long	event;
    register void	( *notify )();

    /* Application datagram reception notifications are processed as follows:
     *
     * 1. Lock and retrieve the CB targeted by the received application
     *	  datagram.
     * 2. Validate the connection.
     * 3. Determine whether suitable conditions exist for reception and
     *    disposal of the application datagram on the connection by the
     *	  corresponding SYSAP.
     * 4. Perform connection bookkeeping
     * 5. Increment the CBVTE semaphore.
     * 6. Unlock the CB.
     * 7. Invoke the connection's datagram event routine to allow the
     *    corresponding SYSAP to receive and dispose of the the application
     *    datagram provided suitable conditions exist.
     * 8. Decrement the CBVTE semaphore.
     *
     * Incrementing the appropriate CBVTE semaphore protects the connection
     * against all changes in state for the duration of SYSAP reception of the
     * application datagram and while any call backs on the connection are in
     * progress.
     *
     * Failure to validate the connection( Step 2 ) is normally a result of
     * connection termination due to path failure.  Failure to find suitable
     * conditions for SYSAP disposal of the application datagram( Step 3 ) may
     * be due to:
     *
     * 1. Inappropriate connection state.
     * 2. Absence of a datagram event routine associated with the connection.
     * 3. Current lack of receive datagram credits on the connection.
     *
     * Either failure aborts handing off the application datagram to the SYSAP
     * for disposal.  Instead, the datagram buffer is returned to the
     * appropriate port's free datagram pool and the number of datagrams
     * discarded on the connection is incremented( provided the connection is
     * in an acceptable state ).
     *
     * The path on which the datagram was received is crashed under any of the
     * following circumstances:
     *
     * 1. The CB can not be retrieved.
     * 2. The connection is in an inappropriate state.
     */
    if( scsbp->rconnid.index > ( lscs.max_conns - 1 )) {
	cbvte = NULL;
	event = SE_BADCONNID;
    } else {
	cbvte = Get_cbvte( scsbp->rconnid );
	Lock_cbvte( cbvte )
	cb = Get_cb( cbvte );
	if( scsbp->rconnid.seq_num != cbvte->connid.seq_num ) {
	    event = 0;
	    ( void )( *pccb->Add_dg )( pccb, scsbp );
	} else if( cb->cinfo.cstate == CS_OPEN &&
		    ( notify = cb->dg_event )  &&
		    cb->cinfo.dg_credit ) {
	    register CSB	*csb = &dg_csb;

	    Event_counter( cb->cinfo.dgs_rec )
	    --cb->cinfo.dg_credit;
	    csb = &dg_csb;
	    Init_csb( csb, cb, scsbp, size )
	    Incr_cbvte_sem( cbvte )
	    Unlock_cbvte( cbvte )
	    ( void )( *notify )( csb );
	    Decr_cbvte_sem( cbvte )
	    return;
	} else if( cb->cinfo.cstate == CS_OPEN		||
		    cb->cinfo.cstate == CS_PATH_FAILURE ||
		    cb->cinfo.cstate == CS_DISCONN_SNT  ||
		    cb->cinfo.cstate == CS_DISCONN_REC  ||
		    cb->cinfo.cstate == CS_DISCONN_ACK  ||
		    cb->cinfo.cstate == CS_DISCONN_MTCH ) {
	    event = 0;
	    Event_counter( cb->cinfo.dgs_discard )
	    ( void )( *cb->Add_dg )( pccb, scsbp );
	} else {
	    event = SE_BADCSTATE;
	}
    }
    if( cbvte ) {
	Unlock_cbvte( cbvte )
    }
    if( event ) {
	register PB	*pb;

	Lock_scadb()
	pb = ( *pccb->Get_pb )( pccb, scsbp, BUF );
	( void )( *pccb->Crash_path )( pccb, pb, event, RECEIVE_BUF, scsbp );
	Unlock_scadb()
    }
}

/*   Name:	scs_msg_rec	- Sequenced Message Received
 *
 *   Abstract:	This SCS event notification routine is asynchronously invoked
 *		whenever a PD receives an application or SCS sequenced
 *		message.  The PD should not have notified SCS of the failure of
 *		the path over which the sequenced message was received although
 *		in a SMP environment this may prove unavoidable.
 *
 *		SCS disposes of the received sequenced message in one of three
 *		ways:
 *
 *		1. The SCS sequenced message is processed and disposed of by
 *		   SCS according to its message type.
 *		2. The application sequenced message is handed off to the
 *		   appropriate SYSAP through asynchronous invocation of the
 *		   connection's message event routine.
 *		3. The application sequenced message buffer is added to the
 *		   appropriate port's free message pool.
 *
 *		Received application sequenced messages may contain extended
 *		send credits.  These credits may trigger notification of the
 *		appropriate SYSAP of the availability of send credits on the
 *		connection through asynchronous invocation of the connection's
 *		control event routine.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *   lk_scadb			- SCA database lock structure
 *   scsbp			- Address of SCS header in datagram buffer
 *   scs_cbvtdb			- CB vector table database pointer
 *   size			- Size of application data + SCS header
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.msgs_rec		-  Number of sequenced messages received
 *	cinfo.rec_credit	-  Number of send credits held by remote SYSAP
 *	cinfo.snd_credit	-  Number of send credits
 *	cinfo.status.cwait	-  SYSAP waiting for send credits flag
 *
 *   SMP:	The SCA database is locked for PB retrieval.  It also prevents
 *		premature PB deletion as required by PD routines which crash
 *		paths.
 *
 *		The CB is locked to synchronize access and to prevent deletion.
 *		It is indirectly locked through its CBVTE.  Locking the CB also
 *		prevents premature PB deletion as required by scs_init_cmsb().
 *
 *		The CBVTE semaphore is incremented prior to either SYSAP
 *		reception of the received application sequenced message or
 *		SYSAP notification of send credit availability.  This prevents
 *		any changes in connection state while call backs are in
 *		progress.  The semaphore is decremented after SYSAP reception
 *		of the application sequenced message completes.
 *
 *		PCCB addresses are always valid allowing access to static
 *		fields because PCCBs are never deleted once SCS becomes aware
 *		of their existence.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
scs_msg_rec( pccb, scsbp, size )
    PCCB		*pccb;
    register SCSH	*scsbp;
    u_long		size;
{
    u_long		event;
    CSB			msg_csb;
    CMSB		credit_cmsb;
    register CB		*cb;
    register CBVTE	*cbvte;
    register void	( *notify )(), ( *credit )();

    /* Invoke SCS to dispose of received non-application sequenced messages.
     * Otherwise, application sequence message reception notifications are
     * processed as follows:
     *
     * 1. Lock and retrieve the CB targeted by the received application
     *	  sequenced message.
     * 2. Validate the connection.
     * 3. Determine whether suitable conditions exist for reception and
     *    disposal of the application sequenced message on the connection by
     *	  the corresponding SYSAP.
     * 4. Perform connection bookkeeping
     * 5. Increment the CBVTE semaphore.
     * 6. Unlock the CB.
     * 7. Notify the SYSAP of send credit availability on the connection if it
     *    was waiting for them to become available and send credits were
     *	  extended across the connection piggy-backed on the received
     *    application sequence message.
     * 8. Invoke the connection's message event routine to allow the
     *    corresponding SYSAP to receive and dispose of the the application
     *    sequenced message provided suitable conditions exist.
     * 9. Decrement the CBVTE semaphore.
     *
     * Incrementing the appropriate CBVTE semaphore protects the connection
     * against all changes in state for the duration of both SYSAP
     * notifications and while any call backs on the connection are in
     * progress.
     *
     * Failure to find suitable conditions for SYSAP disposal of the
     * application sequenced message( Step 3 ) may be due to:
     *
     * 1. Inappropriate connection state.
     * 2. Absence of a message event routine associated with the connection.
     *
     * Either condition aborts handing off the application sequence message to
     * the SYSAP for disposal.  Instead, the message buffer is returned to the
     * appropriate port's message free pool provided the connection is in an
     * acceptable state.
     *
     * The path on which the sequenced message was received is crashed under
     * any of the following circumstances:
     *
     * 1. The CB can not be retrieved.
     * 2. A connection identification number mismatch exists( failure to
     *	  validate the connection ).
     * 3. The connection is in an inappropriate state.
     * 4. The remote SYSAP should not have possessed a send credit for
     *    transmission of the application sequenced message.
     *
     * None of these circumstances are ever expected in a single processor
     * environment.  This explains the drastic actions taken when they occur.
     * However, the occurrence of any of these circumstances can be expected in
     * a SMP environment, albeit with an extremely low frequency, whenever a
     * path fails.  This is because no mechanisms exist to coordinate PD event
     * notifications of SCS as there are mechanisms to coordinate SCS 
     * notifications of SYSAPs.  This allows SCS receptions of application
     * sequenced messages to actually occur after PD notifications of path
     * failure even though PDs initiate the reception first.  The actions taken
     * remain the same as in the single processor case even though they are
     * drastic and may well result in double crashing of the path.  There is no
     * other alternative.
     */
    if( scsbp->mtype != SCS_APPL_MSG ) {
	event = 0;
	cbvte = NULL;
	( void )scs_receive( pccb, scsbp );
    } else if( scsbp->rconnid.index > ( lscs.max_conns - 1 )) {
	cbvte = NULL;
	event = SE_BADCONNID;
    } else {
	cbvte = Get_cbvte( scsbp->rconnid );
	Lock_cbvte( cbvte )
	cb = Get_cb( cbvte );
	if( scsbp->rconnid.seq_num != cbvte->connid.seq_num ) {
	    event = SE_BADCONNID;
	} else if( cb->cinfo.cstate == CS_OPEN && ( notify = cb->msg_event )) {
	    if( cb->cinfo.rec_credit-- ) {
		register CSB	*csb = &msg_csb;

		if(( cb->cinfo.snd_credit += scsbp->credit ) &&
		      cb->cinfo.status.cwait ) {
		    credit = cb->control;
		    cb->cinfo.status.cwait = 0;
		    ( void )scs_init_cmsb( CRE_CREDIT_AVAIL,
					   ADR_SUCCESS,
					   &credit_cmsb,
					   cb,
					   cb->pb,
					   0 );
		} else {
		    credit = NULL;
		}
		Event_counter( cb->cinfo.msgs_rec )
		Init_csb( csb, cb, scsbp, size )
		Incr_cbvte_sem( cbvte )
		Unlock_cbvte( cbvte )
		if( credit ) {
		    ( void )( *credit )( CRE_CREDIT_AVAIL, &credit_cmsb );
		}
		( void )( *notify )( csb );
		Decr_cbvte_sem( cbvte )
		return;
	    } else {
		cb->cinfo.rec_credit = 0;
		event = SE_NOCREDITS;
	    }
	} else if( cb->cinfo.cstate == CS_OPEN ||
		  cb->cinfo.cstate == CS_DISCONN_SNT ) {
	    event = 0;
	    ( void )( *cb->Add_msg )( pccb, scsbp );
	} else {
	    event = SE_BADCSTATE;
	}
    }
    if( cbvte ) {
	Unlock_cbvte( cbvte )
    }
    if( event ) {
	register PB	*pb;

	Lock_scadb()
	pb = ( *pccb->Get_pb )( pccb, scsbp, BUF );
	( void )( *pccb->Crash_path )( pccb, pb, event, RECEIVE_BUF, scsbp );
	Unlock_scadb()
    }
}

/*   Name:	scs_msg_snt	- Application Sequenced Message Sent
 *
 *   Abstract:	This SCS event notification routine is asynchronously invoked
 *		to dispose of certain specified application sequenced messages
 *		following their transmission.  The PD should not have notified
 *		SCS of the failure of the path over which the application
 *		sequenced message was transmitted although in a SMP environment
 *		this may prove unavoidable.
 *
 *		SCS disposes of the transmitted application sequenced message
 *		in one of two ways:
 *
 *		1. The application sequenced message is deallocated.
 *		2. The application sequenced message buffer is converted into a
 *		   receive message buffer for the logical SCS connection by
 *		   adding it to the appropriate port's free message pool.
 *
 *		Converting an application sequenced message buffer into a
 *		receive message buffer may trigger the explicit extension of
 *		send credits across the logical SCS connection.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *   lk_scadb			- SCA database lock structure
 *   scsbp			- Address of SCS header in message buffer
 *   scs_cbvtdb			- CB vector table database pointer
 *   size			- Size of application data + SCS header
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The SCA database is locked for PB retrieval.  It also prevents
 *		premature PB deletion as required by PD routines which crash
 *		paths.
 *
 *		The CB is locked to synchronize access and to prevent deletion.
 *		It is indirectly locked through its CBVTE.  Locking the CB also
 *		prevents premature PB deletion as required by scs_init_cmsb()
 *		and scs_dispose_msg().
 *
 *		PCCB addresses are always valid allowing access to static
 *		fields because PCCBs are never deleted once SCS becomes aware
 *		of their existence.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
scs_msg_snt( pccb, scsbp, size )
    PCCB		*pccb;
    register SCSH	*scsbp;
    u_long		size;
{
    register CB		*cb;
    register PB		*pb;
    register CBVTE	*cbvte;
    register u_long	event = 0;

    /* Application sequenced message transmission notifications are processed
     * as follows:
     *
     * 1. Lock and retrieve the CB targeted by the transmitted application
     *	  sequence message.
     * 2. Validate the connection.
     * 3. Dispose of the transmitted application sequenced message according
     *    to the connection's current credit situation whenever the connection
     *    is open.  Otherwise, deallocate the transmitted application sequenced
     *    message.  
     * 4. Unlock the CB.
     *
     * The path over which the sequenced message was transmitted is crashed
     * under any of the following circumstances:
     *
     * 1. The CB can not be retrieved.
     * 2. A connection identification number mismatch exists.
     *
     * None of these circumstances are ever expected in a single processor
     * environment.  This explains the drastic actions taken when they occur.
     * However, the occurrence of any of these circumstances can be expected in
     * a SMP environment, albeit with an extremely low frequency, whenever a
     * path fails.  This is because no mechanisms exist to coordinate PD event
     * notifications of SCS as there are mechanisms to coordinate SCS
     * notifications of SYSAPs.  This allows SCS disposal of transmitted
     * application sequenced messages to actually occur after PD notifications
     * of path failure even though PDs initiate message disposal first.  The
     * actions taken remain the same as in the single processor case even
     * though they are drastic and may well result in double crashing of the
     * path.  There is no other alternative.
     */
    if( scsbp->sconnid.index > ( lscs.max_conns - 1 )) {
	cbvte = NULL;
	event = SE_BADCONNID;
    } else {
	cbvte = Get_cbvte( scsbp->sconnid );
	Lock_cbvte( cbvte )
	cb = Get_cb( cbvte );
	if( scsbp->sconnid.seq_num != cbvte->connid.seq_num ) {
	    event = SE_BADCONNID;
	} else if( cb->cinfo.cstate == CS_OPEN ) {
	    ( void )scs_dispose_msg( cb, scsbp );
	} else {
	    ( void )( *cb->Dealloc_msg )( pccb, scsbp );
	}
    }
    if( cbvte ) {
	Unlock_cbvte( cbvte )
    }
    if( event ) {
	Lock_scadb()
	pb = ( *pccb->Get_pb )( pccb, scsbp, BUF );
	( void )( *pccb->Crash_path )( pccb, pb, event, DEALLOC_BUF, scsbp );
	Unlock_scadb()
    }
}

/*   Name:	scs_new_path	- New Path to System Discovered
 *
 *   Abstract:	This SCS event notification routine is asynchronously invoked
 *		whenever the PD discovers a path to a previously unknown
 *		system.  It proceeds to notify all listening SYSAPs of the
 *		existence of the new path through asynchronous invocations
 *		of their connections' control event routines.
 *
 *		SMP environments require the PB representing the path to the
 *		new system to exist in some state for the duration of the SYSAP
 *		notifications.  The path itself can fail but the PB may NOT be
 *		cleaned up until after this routine exits.  The responsibility
 *		for meeting this requirement is assigned to the PDs and the
 *		choice of mechanism is also left up to the individual PDs.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *   sb				- System Block pointer
 *   lk_scadb			- SCA database lock structure
 *   scs_listeners		- Listening SYSAP queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The SCA database is locked to postpone potential modifications
 *		to the queue of listening SYSAPs while it is being traversed.
 *
 *		The CB is locked to synchronize access and to prevent deletion.
 *		It is indirectly locked through its CBVTE.  Locking the CB also
 *		prevents premature PB deletion as required by scs_init_cmsb().
 *
 *		The CBVTE semaphore is incremented prior to SYSAP notification
 *		of new path existence.  This prevents connection termination
 *		while call backs are in progress.  The semaphore is decremented
 *		after SYSAP notification completes.
 *
 *		The validity of the PB address must be guaranteed EXTERNALLY.
 *		
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
scs_new_path( sb, pb )
    SB			*sb;
    PB			*pb;
{
    CMSB		cmsb;
    register cbq	*cb;
    register CBVTE	*cbvte;
    register void	( *control )();
    register u_long	dirid = 0;

    /* Iteratively scan the queue of listening SYSAPs.  Each scan:
     *
     * 1. Locks the SCA database.
     * 2. Positions and locks the "next" CB within the queue( now the "current"
     *    CB ).
     * 3. Increments the CBVTE semaphore.
     * 4. Unlocks the CB.
     * 5. Unlocks the SCA database.
     * 6. Notifies the SYSAP corresponding to the "current" CB of the existence
     *    of a path to a previously unknown system.
     * 7. Decrements the CBVTE semaphore.
     *
     * Incrementing the appropriate CBVTE semaphore protects the connection
     * against termination for the duration of SYSAP notification of new path
     * existence and while any call backs on the connection are in progress.
     * Such protection is necessary because termination of listening
     * connections is synchronous and immediate which would allow SYSAPs to
     * receive notifications on connections which no longer exist if such
     * protection was not provided.
     *
     * Positioning to the "next" CB is accomplished by means of the "current"
     * CB's directory identification number.  This allows positioning to be
     * relative and accomplished even when the "current" CB is deleted from the
     * the queue between iterative scans.
     */
    do	{
	Lock_scadb()
	for( cb = scs_listeners.flink;
	     cb != &scs_listeners && Cb->cinfo.Dirid <= dirid;
	     cb = cb->flink ) {}
	if( cb != &scs_listeners ) {
	    if( Cb->cinfo.cstate != CS_LISTEN ) {
		( void )panic( SCSPANIC_LQUEUE );
	    }
	    cbvte = Get_cbvte( Cb->cinfo.lconnid );
	    Lock_cbvte( cbvte )
	    ( void )scs_init_cmsb( CRE_NEW_PATH,
				   ADR_SUCCESS,
				   &cmsb,
				   cb,
				   pb,
				   0 );
	    control = Cb->control;
	    dirid = Cb->cinfo.Dirid;
	    Incr_cbvte_sem( cbvte )
	    Unlock_cbvte( cbvte )
	} else {
	    control = NULL;
	}
	Unlock_scadb()
	if( control ) {
	    ( void )( *control )( CRE_NEW_PATH, &cmsb );
	    Decr_cbvte_sem( cbvte )
	}
    }	while( cb != &scs_listeners );
}


/*   Name:	scs_path_crash	- Path to System Crashed
 *
 *   Abstract:	This SCS event notification routine is asynchronously invoked
 *		by PDs following path failure, disablement, and invalidation of
 *		the appropriate port's translation cache( as required ).  It
 *		directs clean up the failed path with the ultimate goal of
 *		removing the PB from the system-wide configuration database and
 *		deleting it.  It must only be invoked once for each path
 *		incarnation.
 *
 *		PBs passed to this routine are guaranteed to be valid because:
 *
 *		1. Path clean up is single threaded even in SMP environments.
 *		2. It is this routine which is responsible for directing clean
 *		   up and deallocation of PBs.
 *
 *		NOTE: Once this routine is invoked the PD may only notify SCS
 *		      of the successful transmission and reception of datagrams
 *		      over this incarnation of the failed path.
 *
 *		NOTE: The appropriate port drivers are always responsible for
 *		      the clean up of all port specific resources associated
 *		      with failed local ports including free datagram and
 *		      message buffers.  SCS must never attempt to dispose of
 *		      such resources during clean up of the paths and
 *		      connections associated with such failed ports.
 *		
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pinfo.state		-  PS_PATH_FAILURE
 *  	pinfo.reason		-  Reason for path failure
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *   scs_timeoutq		- SCS protocol sequence timeout queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pinfo.nconns		-  Number of connections
 *	pinfo.status.sanity	-  0
 *
 *   SMP:	The SCA database is locked for traversing and removing CBs and
 *		PBs( when necessary ) from the system-wide configuration
 *		database, and deallocating CBVTEs.  Locking the SCA database
 *		also prevents premature PB deletion and allows their removal
 *		from the SCS protocol sequence timeout queue when necessary.
 *
 *   		CBs are locked to synchronize access, for deletion in some
 *		cases, to prevent premature deletion in others, and as required
 *		by scs_log_event() in case logging becomes necessary.  They are
 *		indirectly locked through their CBVTEs.
 *
 *		CBVTE semaphores are synchronized to in order to postpone
 *		changes in connection state while call backs to the
 *		corresponding SYSAPs are in progress.
 *
 *		The PB is locked to synchronize access and whenever it is found
 *		necessary to remove a CB from its SCS CB wait queue or the PB
 *		from the SCS protocol sequence timeout queue.
 *
 *		The validity of the PB address must be guaranteed EXTERNALLY.
 *		
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
scs_path_crash( pb )
    register PB		*pb;
{
    CMSB		cmsb;
    u_long		sn;
    void		( *control )();
    register cbq	*cb;
    register CBVTE	*cbvte;
    register u_long	port_failure, event, status;

    /* Clean up of the failed path involves the following steps:
     *
     *  1. Lock the SCA database.
     *  2. Lock the PB.
     *  3. Abort the PB sanity timer if currently operational.
     *  4. Determine whether the path failed due to local port failure.
     *  5. Unlock the PB.
     *  6. Temporarily increment the count of CBs associated with the path.
     *  7. Initiate clean up of all CBs associated with the path.
     *  8. Decrement the count of CBs associated with the path restoring it to
     * 	   its proper value.
     *  9. Invoke the appropriate PD routine to remove the PB from the
     *	   system-wide configuration database and deallocate it provided all
     *	   CBs have been cleaned up and there are none associated with the PB.
     *	   Otherwise, the PB is removed and deallocated when the last
     *	   connection across the path is terminated by SYSAP request.
     * 10. Unlock the SCA database.
     *
     * Temporarily incrementing the CB count( Step 6 ) prevents PB deletion and
     * simplifies path clean up while guaranteeing the validity of the cached
     * PB address.  Locks are insufficient for this purpose because they must
     * be repeatedly released and re-established during path clean up.
     *
     * The clean up of each CB( Step 7 ) proceeds as follows:
     *
     * 1. Lock the CB.
     * 2. Synchronize to the CBVTE semaphore.
     * 3. Abort any scheduled SCS request.
     * 4. Remove the CB from all internal resource queues.
     * 5. Prohibit the removal of buffers from the local port's free pools
     *    during CB clean up whenever path failure resulted from local port
     *    failure.  The buffers credited to the connection are retrieved and
     *    disposed of during clean up of the failed port itself.
     * 6. Complete CB clean up according to its connection state.
     * 7. Unlock the CB.
     *
     * Synchronizing to the CBVTE semaphore( Step 2 ) postpones processing of
     * the connection and subsequent changes in its state while call backs to
     * the corresponding SYSAP are in progress.  Failure to synchronize can
     * lead to numerous aberrant situations in SMP environments such as SYSAP
     * notification of application message reception FOLLOWING notification of
     * connection termination due to path failure.  Synchronization is
     * achieved by releasing the SCA database and CB locks and re-obtaining
     * them in the proper order until all call backs have completed( CBVTE
     * semaphore == 0 ).
     *
     * Connection state dependent clean up( Step 6 ) of open connections and
     * those partially disconnected due to remote SYSAP action is asynchronous.
     * The local SYSAP is notified of path failure through asynchronous
     * invocation of the connection's control event routine and it must
     * explicitly request connection termination.  It may do so at its leisure.
     *
     * Connection state dependent clean up( Step 6 ) of formative connections
     * initiated by the remote SYSAP but not yet responded to by the local
     * SYSAP is synchronous.  The event is logged.  The CB is removed from the
     * system-wide configuration database and is deallocated along with all
     * associated resources.  The corresponding local SYSAP is not notified of
     * this occurrence but discover it when it attempts to respond to the
     * connection request.
     *
     * Connection state dependent clean up( Step 6 ) of connections in all
     * other states is asynchronous and proceeds as follows:
     *
     * 1. Termination of the fully established or formative SCS connection due
     *	  to path failure is logged.
     * 2. The CB is removed from the system-wide configuration database.
     * 3. The CB is deallocated along with all associated resources.
     * 4. The local SYSAP is notified of connection failure through
     *	  asynchronous invocation of the connection's control event routine.
     *
     * Which specific event the local SYSAP is notified of depends upon the
     * connection's former state.
     *
     * NOTE: All locks are released prior to local SYSAP notification to allow
     *	     it to respond to the event.  The SCA database lock is
     *	     re-established following SYSAP notification but prior to obtaining
     *	     the next CB associated with the failed path.  Cached PB addresses
     *	     are valid following re-establishment of the SCA database lock
     *	     because the number of CBs associated with them was artificially
     *	     increased to prevent PB removal from the system-wide configuration
     *	     database and deletion.
     *
     * NOTE: Cached CB addresses are valid after synchronization to the CBVTE
     *	     semaphore is achieved because once path failure is reported to SCS
     *	     the only way for CBs to be deallocated is for their SYSAPs to
     *	     request disconnection of their corresponding connections following
     *	     explicit notification of the path failure.
     */
    Lock_scadb()
    Lock_pb( pb )
    if( pb->pinfo.status.sanity ) {
	Remove_scs_timeoutq( pb )
    }
    port_failure = Port_failure( pb->pinfo.reason );
    Unlock_pb( pb )
    ++pb->pinfo.nconns;
    for( cb = pb->cbs.flink; cb != &pb->cbs; ) {
	event = 0;
	cbvte = Get_cbvte( Cb->cinfo.lconnid );
	Lock_cbvte( cbvte )
	sn = cbvte->connid.seq_num;
	while( Test_cbvte_sem( cbvte )) {
	    Unlock_cbvte( cbvte )
	    Unlock_scadb()
	    Lock_scadb()
	    Lock_cbvte( cbvte )
	}
	if( sn != cbvte->connid.seq_num ) {
	    ( void )panic( SCSPANIC_SCADB );
	}
	Remove_pb_waitq( Cb )
	Cb->cinfo.status.cwait = 0;
	if( port_failure ) {
	    Cb->cinfo.rec_credit = 0,
	    Cb->cinfo.pend_rec_credit = 0,
	    Cb->cinfo.ntransfers = 0;
	    Cb->cinfo.dg_credit = 0;
	}
	switch( Cb->cinfo.cstate ) {

	    case CS_OPEN:
	    case CS_DISCONN_REC:
		break;

	    case CS_DISCONN_ACK:
	    case CS_DISCONN_SNT:
	    case CS_DISCONN_MTCH:
		event = W_FAIL_CONN;
		break;

	    case CS_CONN_SNT:
	    case CS_CONN_ACK:
	    case CS_CONN_REC:
	    case CS_ACCEPT_SNT:
	    case CS_REJECT_SNT:
		event = W_FAIL_FCONN;
		break;

	    default:
		( void )panic( SCSPANIC_CSTATE );
	}
	if( event ) {
	    ( void )scs_log_event( Cb, event, CONN_EVENT );
	    event = 0;
	}
	switch( Cb->cinfo.cstate ) {

	    case CS_OPEN:
	    case CS_DISCONN_REC:
		Cb->cinfo.cstate = CS_PATH_FAILURE;
		status = pb->pinfo.reason;
		event = CRE_PATH_FAILURE;
		break;

	    case CS_DISCONN_ACK:
	    case CS_DISCONN_SNT:
	    case CS_DISCONN_MTCH:
		Cb->cinfo.cstate = CS_CLOSED;
		status = ADR_PATH_FAILURE;
		event = CRE_DISCONN_DONE;
		break;

	    case CS_CONN_SNT:
	    case CS_CONN_ACK:
		Cb->cinfo.cstate = CS_CLOSED;
		status = ADR_PATH_FAILURE;
		event = CRE_CONN_DONE;

		/* Asynchronous local SYSAP notification triggered by
		 * unilateral connection abortion is unscheduled.
		 */
		if( Cb->cinfo.status.abort_fork ) {
		    ( void )unksched( &Cb->forkb );
		    Cb->cinfo.status.abort_fork = 0;
		}
		break;

	    case CS_CONN_REC: {
		cbq	*next_cb = cb->flink;

		Cb->cinfo.cstate = CS_CLOSED;
		Remove_cb( Cb, pb )
		Unlock_cbvte( cbvte )
		cb = next_cb;
		continue;
	    }

	    case CS_ACCEPT_SNT:
	    case CS_REJECT_SNT:
		if( Cb->cinfo.cstate == CS_ACCEPT_SNT ) {
		    event = CRE_ACCEPT_DONE;
		} else if( Cb->cinfo.status.disconnect ) {
		    event = CRE_DISCONN_DONE;
		} else {
		    event = CRE_REJECT_DONE;
		}
		status = ADR_PATH_FAILURE;
		Cb->cinfo.cstate = CS_CLOSED;
		break;

	    default:
		( void )panic( SCSPANIC_CSTATE );
	}
	( void )scs_init_cmsb( event, status, &cmsb, Cb, pb, 0 );
	control = Cb->control;
	if( Cb->cinfo.cstate == CS_CLOSED ) {
	    Remove_cb( Cb, pb )
	}
	Unlock_cbvte( cbvte )
	Unlock_scadb()
	( void )( *control )( event, &cmsb );
	Lock_scadb()
	for( cb = pb->cbs.flink;
	     cb != &pb->cbs && Cb->cinfo.cstate == CS_PATH_FAILURE;
	     cb = cb->flink ) {}
    }
    if( --pb->pinfo.nconns == 0 ) {
	( void )( *pb->Remove_pb )( pb->pccb, pb );
    }
    Unlock_scadb()
}
