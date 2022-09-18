#ifndef	lint
static char *sccsid = "@(#)scs_conn.c	4.1	(ULTRIX)	7/2/90";
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
 *		connection management service functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 27, 1985
 *
 *   Function/Routines:
 *
 *   scs_listen			Listen for SCS Connection Requests
 *   scs_connect		Request Logical SCS Connection
 *   scs_accept			Accept Logical SCS Connection Request
 *   scs_reject			Reject Logical SCS Connection Request
 *   scs_disconnect		Disconnect SCS Connection
 *
 *   Modification History:
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h, external lk_scadb
 *
 *   11-Feb-1989	Todd M. Katz		TMK0004
 *	1. Add support for SCS event logging.  Log establishments and
 *	   terminations of SCS listening connections and disconnects of fully
 *	   established SCS connections terminated by path failure.
 *	2. Include header file ../vaxmsi/msisysap.h.
 *
 *   13-Sep-1988	Todd M. Katz		TMK0003
 *	Return an error( RET_NORESOURCES ) from scs_listen() if SCS has never
 *	been initialized.
 *
 *   12-Jul-1988	Todd M. Katz		TMK0002
 *	Macros Copy_name() and Copy_data() have been renamed to Move_name() and
 *	Move_data() respectively.
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
#include		"../h/param.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#ifdef mips
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#endif mips
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
extern	struct lock_t	lk_scadb;
extern	sbq		scs_config_db;
extern	cbq		scs_listeners;
extern	CB		*scs_alloc_cb();
extern	PB		*scs_choose_path();
extern	void		ksched(), scs_abort_conn(), scs_dealloc_cb(),
			scs_log_event(), scs_request();

/*   Name:	scs_listen	- Listen for SCS Connection Requests
 *
 *   Abstract:	This function initiates passive listening for SCS connection
 *		requests.  The SYSAP receives requests for connection
 *		establishment and notification of the existence of new paths
 *		through asynchronous invocations of the listening connection's
 *		control event routine.
 *
 *		The SYSAP's name and optionally supplied connection data is
 *		passed to any curious SYSAP requesting such information from
 *		the local directory SYSAP.
 *
 *		A CB is allocated for the new listening SCS connection.  A
 *		CBVTE is allocated to reference the new CB.
 *.
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *	aux			-  Address of auxiliary structure( OPTIONAL )
 *	conn_data		-  Connection data( OPTIONAL )
 *	control			-  Address of connection control event routine
 *	lproc_name		-  Local SYSAP name( blank filled )
 *   lk_scadb			- SCA database lock structure
 *   scs_listeners		- Listening SYSAP queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *	connid			-  Identification of listening SCS connection
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- SYSAP is passively listening for requests
 *   RET_ALLOCFAIL		- Failed to allocate a CB
 *   RET_NORESOURCES		- SCS has never been initialized
 *
 *   SMP:	The SCA database is locked for CBVTE allocation and for CB
 *		insertion into the queue of listening SYSAPs.
 *
 *		There is no need to lock the new CB.  It can not be accessed
 *		through its CBVTE until used and is otherwise inaccessible
 *		until the SCA database lock is released.
 */
u_long
scs_listen( cmsb )
    CMSB		*cmsb;
{
    register cbq	*cb, *ptr;
    register u_long	status = RET_SUCCESS;

    /* Passive listening for SCS connection requests is initiated as follows:
     *
     * 1. Lock the SCA database.
     * 2. Allocate and initialize a CB.
     * 3. Insert the CB into the first available slot within the queue of
     *    listening SYSAPs.
     * 4. Log establishment of the new listening SCS connection.
     * 5. Unlock the SCA database.
     * 6. Return an appropriate status.
     *
     * The CB is assigned a directory identification number based upon its slot
     * assignment.  If the queue was previously empty the CB is assigned a
     * directory identification number of 1.
     */
    Lock_scadb()
    if( lscs.system.swtype == 0 ) {
	status = RET_NORESOURCES;
    } else if(( cb = ( cbq * )scs_alloc_cb( CS_LISTEN, cmsb, NULL ))) {
	Move_connid( Cb->cinfo.lconnid, cmsb->connid )
	for( Cb->cinfo.Dirid = 1, ptr = scs_listeners.flink;
	     ptr != &scs_listeners;
	     ++Cb->cinfo.Dirid, ptr = ptr->flink ) {
	    if( Cb->cinfo.Dirid < (( CB * )ptr )->cinfo.Dirid ) {
		break;
	    }
	}
	( void )scs_log_event( Cb, I_NEW_LISTENER, LSYSAP_EVENT );
	Insert_entry( cb->flink, ( *ptr ))
    } else {
	status = RET_ALLOCFAIL;
    }
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_connect	- Request Logical SCS Connection
 *
 *   Abstract:	This function initiates establishment of a logical SCS
 *		connection to a SYSAP passively listening for such connection
 *		requests.  The SYSAP is notified of the outcome of its
 *		connection request through asynchronous invocation of the
 *		connection's control event routine.
 *
 *		While the ultimate outcome of a connection request is either
 *		success or failure, there are numerous reasons why failure to
 *		establish a connection may occur.  Some of these reasons are
 *		SYSAP protocol specific.  In other situations SCS itself aborts
 *		connection establishment with one of the following reasons:
 *
 *		NOLISTENER   -	Target SYSAP does not exist on remote system.
 *		NORESOURCE   -	Insufficient resources on remote system for
 *				connection establishment.
 *		NOSUPPORT    -	Target SYSAP is not supported on remote system.
 *		BUSY	     -	Target SYSAP only supports one simultaneous
 *				connection on remote system and such a
 *				connection already exists.
 *		DISCONN	     -	Target SYSAP rejected connection without
 *				specifying a reason or initiating SYSAP
 *				unilaterally aborted connection establishment.
 *		PATH_FAILURE -	Path failed before connection was established.
 *
 *		A CB and associated resources are allocated for the new logical
 *		SCS connection.  A CBVTE is allocated to reference the new CB.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *	aux			-  Address of auxiliary structure( OPTIONAL )
 *	conn_data		-  Connection data( OPTIONAL )
 *	control			-  Address of connection control event routine
 *	dg_event		-  Address of dg event routine( OPTIONAL )
 *	init_dg_credit		-  Initial datagram credit( OPTIONAL )
 *	init_rec_credit		-  Initial message credit( OPTIONAL )
 *	lport_name		-  Name of local port( OPTIONAL )
 *				    ( 0 -> Choice of path left to SCS )
 *	lproc_name		-  Local SYSAP name( blank filled )
 *	min_snd_credit		-  Minimum send credit requirement( OPTIONAL )
 *	msg_event		-  Address of msg event routine( OPTIONAL )
 *	rport_addr		-  Remote port station address( OPTIONAL )
 *	rproc_name		-  Remote SYSAP name
 *	sysid			-  Identification of target system
 *   lk_scadb			- SCA database lock structure
 *   scs_config_db		- System-wide configuration database queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *	connid			-  Identification of logical SCS connection
 *   pb	 			- Path Block pointer
 *	pinfo.nconns		-  Number of connections
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Initiated SCS connection establishment
 *   RET_ALLOCFAIL		- Failed to allocate a CB
 *				  Failed to allocate receive datagram buffers
 *				  Failed to allocate receive message buffers
 *   RET_INVPSTATE		- Target path in invalid state
 *   RET_NOSYSTEM		- Target system does not exist
 *   RET_NOPATH			- Requested path does not exist
 *				  No paths exist
 *
 *   SMP:	The SCA database is locked for CBVTE allocation, CB insertion
 *		into the system-wide configuration database, and for requesting
 *		a new connection to the target SYSAP.  It also prevents PB
 *		deletion.
 *
 *		The new CB is locked prior to insertion into the system-wide
 *		configuration database even though there is no real need to do
 *		so.  It can not be accessed through its CBVTE until its
 *		connection request has been processed and is otherwise
 *		inaccessible until the SCA database lock is released.  However,
 *		locking it simplifies the interface to scs_request() and this
 *		is why it is done.  It is indirectly locked through its CBVTE.
 */
u_long
scs_connect( cmsb )
    register CMSB	*cmsb;
{
    register CB		*cb;
    register sbq	*sb;
    register pbq	*pb;
    register CBVTE	*cbvte;
    register u_long	status = RET_NOSYSTEM;

    /* Initiation of logical SCS connection establishment proceeds as follows:
     *
     * 1. Lock the SCA database.
     * 2. Search the system-wide configuration database for the target system.
     * 3. Choose an open path to the target system for the new connection if
     *    the choice of path was left up to SCS; otherwise, continue searching
     *	  the system-wide configuration database for the target path.
     * 4. Allocate and initialize a CB.
     * 5. Lock the CB.
     * 6. Insert the CB into the system-wide configuration database.
     * 7. Request establishment of a new connection to the target SYSAP
     *	  provided a suitable path to the target system exists.
     * 8. Unlock the CB and SCA database.
     * 9. Return an appropriate status.
     */
    Lock_scadb()
    for( sb = scs_config_db.flink; sb != &scs_config_db; sb = sb->flink ) {
	if( Comp_scaaddr( cmsb->sysid, Sb->sinfo.sysid )) {
	    if( cmsb->lport_name == 0 ) {
		if(( pb = ( pbq * )scs_choose_path( Sb )) == NULL ) {
		    status = RET_NOPATH;
		    break;
		}
	    } else {
		for( pb = Sb->pbs.flink;
		     pb != &Sb->pbs &&
		      ( cmsb->lport_name != Pb->pinfo.lport_name ||
		        !Comp_scaaddr( cmsb->rport_addr,
				       Pb->pinfo.rport_addr ));
		     pb = pb->flink ) {}
		if( pb == &Sb->	pbs ) {
		    status = RET_NOPATH;
		} else if( Pb->pinfo.state != PS_OPEN ) {
		    status = RET_INVPSTATE;
		}
		if( status != RET_NOSYSTEM ) {
		    break;
		}
	    }
	    if(( cb = scs_alloc_cb( CS_CONN_SNT, cmsb, Pb ))) {
		status = RET_SUCCESS;
		Move_connid( cb->cinfo.lconnid, cmsb->connid )
		cbvte = Get_cbvte( cb->cinfo.lconnid );
		Lock_cbvte( cbvte )
		Insert_cb( cb, Pb )
		( void )scs_request( CB_CONN_PEND, cb, Pb, NULL );
		Unlock_cbvte( cbvte )
	    } else {
		status = RET_ALLOCFAIL;
	    }
	    break;
	}
    }
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_accept	- Accept Logical SCS Connection Request
 *
 *   Abstract:	This function establishes a logical SCS connection by accepting
 *		a connection request.  The SYSAP is notified of the outcome of
 *		its acceptance request through asynchronous invocation of the
 *		connection's control event routine.
 *
 *		There are several reasons why unsuccessful outcomes to
 *		acceptances can occur.  These reasons are listed below:
 *
 *		DISCONN	      -	The connection initiating SYSAP unilaterally
 *				aborted connection establishment.
 *		PATH_FAILURE  -	Path failed before connection established.
 *
 *		Resources are allocated for the new logical SCS connection.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *	aux			-  Address of auxiliary structure( OPTIONAL )
 *	conn_data		-  Connection data( OPTIONAL )
 *	connid			-  Identification of accepted SCS connection
 *	control			-  Address of connection control event routine
 *	dg_event		-  Address of dg event routine( OPTIONAL )
 *	init_dg_credit		-  Initial datagram credit( OPTIONAL )
 *	init_rec_credit		-  Initial message credit( OPTIONAL )
 *	min_snd_credit		-  Minimum send credit requirement( OPTIONAL )
 *	msg_event		-  Address of msg event routine( OPTIONAL )
 *   lscs			- Local system permanent information
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Initiated acceptance of connection request
 *   RET_ALLOCFAIL		- Failed to allocate receive datagram buffers
 *				  Failed to allocate receive message buffers
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The SCA database is locked for requesting acceptance and
 *		establishment of the new connection.
 *		
 *		The CB is locked to synchronize access, prevent deletion, and
 *		for requesting acceptance and establishment of the new
 *		connection.  It is indirectly locked through its CBVTE.
 *		Locking the CB also prevents PB deletion.
 */
u_long
scs_accept( cmsb )
    register CMSB	*cmsb;
{
    register CB		*cb;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* Acceptance of a logical SCS connection request proceeds as follows:
     *
     * 1. Lock the SCA database.
     * 2. Lock and retrieve the CB for the new connection.
     * 3. Allocate and add to the appropriate local port's free message and
     *	  datagram pools an initial credits worth of message and datagram
     *    buffers.
     * 4. Complete CB initialization.
     * 5. Request acceptance of the connection request and establishment of a
     *	  new connection.
     * 6. Unlock the CB and SCA database.
     * 7. Return an appropriate status.
     *
     * Partial buffer allocations may occur( Step 3 ).  Buffers allocated are
     * deallocated, but the connection is NOT cleaned up.  This allows the
     * SYSAP to retry acceptance of the connection request.  
     */
    Check_connid2( cmsb->connid, cbvte, cb )
    if( cb->cinfo.cstate != CS_CONN_REC ) {
	status = RET_INVCSTATE;
    } else if( scs_alloc_buf( cb,
			    cmsb->init_rec_credit,
			    cmsb->init_dg_credit ) == RET_SUCCESS ) {
	cb->control = cmsb->control;
	cb->msg_event = cmsb->msg_event;
	cb->dg_event = cmsb->dg_event;
	cb->aux = cmsb->aux;
	Move_data( cmsb->conn_data, cb->cinfo.lconn_data )
	cb->cinfo.cstate = CS_ACCEPT_SNT;
	cb->cinfo.min_snd_credit = cmsb->min_snd_credit;
	cb->cinfo.init_rec_credit = cmsb->init_rec_credit;
	( void )scs_request( CB_ACCEPT_PEND, cb, cb->pb, NULL );
    } else {
	( void )scs_dealloc_buf( cb );
	status = RET_ALLOCFAIL;
    }
    Unlock_cbvte( cbvte )
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_reject	- Reject Logical SCS Connection Request
 *
 *   Abstract:	This function prevents establishment of a logical SCS
 *		connection by rejecting a connection request.  The SYSAP is
 *		notified of the outcome of its rejection request through
 *		asynchronous invocation of the connection's control event
 *		routine.
 *
 *		The CB allocated for the connection is deallocated just prior
 *		to SYSAP notification.  The CBVTE referencing the CB is also
 *		deallocated.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *	connid			-  Identification of rejecting SCS connection
 *	Reason			-  Reject reason( OPTIONAL )
 *   lscs			- Local system permanent information
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.reason		-  Rejection reason
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Initiated rejection of connection request
 *   RET_INVCONNID		- Invalid connection identification number
 *   RET_INVCSTATE		- Connection in invalid state
 *
 *   SMP:	The SCA database is locked for requesting rejection of the
 *		connection request.
 *
 *		The CB is locked to synchronize access, prevent premature
 *		deletion, and for requesting rejection of the connection
 *		request.  It is indirectly locked through its CBVTE.  Locking
 *		the CB also prevents PB deletion.
 */
u_long
scs_reject( cmsb )
    register CMSB	*cmsb;
{
    register CB		*cb;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* Rejecting a logical SCS connection request proceeds as follows:
     *
     * 1. Lock the SCA database.
     * 2. Lock and retrieve the CB for the connection.
     * 3. Request rejection of the connection request.
     * 4. Unlock the CB and SCA database.
     * 5. Return an appropriate status.
     */
    Check_connid2( cmsb->connid, cbvte, cb )
    if( cb->cinfo.cstate == CS_CONN_REC ) {
	cb->cinfo.cstate = CS_REJECT_SNT;
	if(( cb->cinfo.reason = cmsb->Reason ) == 0 ) {
	    cb->cinfo.reason = ADR_DISCONN;
	}
	( void )scs_request( CB_REJECT_PEND, cb, cb->pb, NULL );
    } else {
	status = RET_INVCSTATE;
    }
    Unlock_cbvte( cbvte )
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_disconnect	- Disconnect SCS Connection
 *
 *   Abstract:	This function terminates logical, listening, and formative
 *		SCS connections.  Connection termination may be temporarily
 *		disallowed.  This occurs only when termination may lead to
 *		database corruption or an unknown SCS state.
 *
 *		Connection termination always involves CB removal from all
 *		internal resource queues and the system-wide configuration
 *		database and its deallocation along with all associated
 *		resources.  The CBVTE referencing the CB is also deallocated.
 *
 *		SYSAPs are usually notified of connection termination through
 *		asynchronous invocation of their connections' control event
 *		routines.  However, the termination of listening SCS
 *		connections and those connections established across now failed
 *		paths always conclude synchronously.  No notification occurs.
 *
 *		NOTE: Termination of open connections must always be
 *		      asynchronously initiated.  All SYSAP attempts to
 *		      disconnect connections following call back on those
 *		      connections fail with a "connection busy" error.
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
 *   cmsb			- Connection Management Service Block pointer
 *	connid			-  Identification of SCS connection
 *	Reason			-  Disconnect reason( OPTIONAL )
 *   lscs			- Local system permanent information
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.cstate		-  Connection state
 *	cinfo.reason		-  Disconnection reason
 *	cinfo.status.abort_fork	-  Connection abortion fork in progress
 *	cinfo.status.disconnect -  Disconnect converted into reject flag
 *	fork			-  CB fork context ( INITIALIZED )
 *   pb				- Address of Path Block
 *	pinfo.nconns		-  Number of connections
 *
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Disconnection initiated ( or completed )
 *   RET_CONNBUSY		- Connection is busy & can not be disconnected
 *   RET_INVCONNID		- Invalid connection identification number
 *
 *   SMP:	The SCA database is locked for CB removal from either the queue
 *		of listening SYSAPs or the system-wide configuration database(
 *		depending upon the SCS connection state ), CBVTE deallocation,
 *		and for requesting connection termination( or rejection of a
 *		connection request when connection state == CS_CONN_REC ).  It
 *		is also required for PB deletion from the system-wide
 *		configuration database when such action becomes necessary.
 *
 *		The CB is locked to synchronize access, for deletion in some
 *		cases, to prevent premature deletion in others, and as required
 *		by scs_log_event() in case logging becomes necessary.  It is
 *		indirectly locked through its CBVTE.  Locking the CB also
 *		prevents PB deletion and is required for requesting connection
 *		termination( or rejection of a connection request when
 *		connection state == CS_CONN_REC ).
 *
 *		The CBVTE semaphore is accessed to ascertain whether or not
 *		call backs on the connection are currently in progress.
 *		Connection termination is not allowed as long as call backs are
 *		in progress.
 *
 *		The PB is locked whenever it is found necessary to remove a CB
 *		from its SCS CB wait queue.
 */
u_long
scs_disconnect( cmsb )
    register CMSB	*cmsb;
{
    register CB		*cb;
    register PB		*pb;
    register CBVTE	*cbvte;
    register u_long	status = RET_SUCCESS;

    /* The steps involved in terminating a connection are as follows:
     *
     * 1. Lock the SCA database.
     * 2. Lock and retrieve the CB for the connection.
     * 3. Terminate the connection according to its current connection state.
     * 4. Unlock the CB and SCA database.
     * 5. Return an appropriate status.
     *
     * The following table lists the states connections may be found in and the
     * corresponding actions which are required to delete such connections:
     *
     * State		Actions
     *
     * LISTEN		1. Remove the CB from the queue of listening SYSAPs
     *			2. Log termination of the listening SCS connection.
     *			3. Delete the CB releasing all associated resources.
     *
     * OPEN		1. Remove the CB from various resource queues.
     * DISCONN_REC	2. Request SCS to terminate the connection.
     *			-- Following asynchronous connection termination --
     *			3. Remove the CB from the system-wide configuration
     *			   database.
     *			4. Delete the CB releasing all associated resources.
     *			5. Notify the SYSAP of connection termination.
     *
     * CONN_SNT		1. Abort any pending SCS connection request.
     * CONN_ACK		2. Asynchronously call back SCS.
     *			-- Following asynchronous SCS notification --
     *			3. Remove the CB from the system-wide configuration
     *			   database.
     *			4. Delete the CB releasing all associated resources.
     *			5. Notify the SYSAP of connection abortion.
     *
     * CONN_REC		1. Request SCS to reject the connection request.
     *			-- Following asynchronous connection termination --
     *			2. Remove the CB from the system-wide configuration
     *			   database.
     *			3. Delete the CB releasing all associated resources.
     *			4. Notify the SYSAP of connection termination.
     *
     * PATH_FAILURE	1. Remove the CB from the system-wide configuration
     *			   database.
     *			2. Log failure of the established SCS connection.
     *			3. Delete the CB releasing all associated resources.
     *			4. Invoke the appropriate PD specific routine to
     *			   eliminate the appropriate PB when all connections
     *			   across the corresponding path have been terminated.
     * 
     * CLOSED		1. Return an illegal connection state error.
     * DISCONN_ACK
     * DISCONN_SNT
     * DISCONN_MTCH
     * ACCEPT_SNT
     * REJECT_SNT
     *
     * SYSAPs are never allowed to terminate connections in either the LISTEN,
     * or OPEN connection states when asynchronous call backs to the SYSAP are
     * currently in progress on the connection.  SYSAPs are never allowed to
     * terminate connections in either the OPEN or DISCONN_REC connection
     * states when block data transfers are currently in progress on the
     * connection.  In both cases, SYSAPs should prevent further activity on
     * the connection and schedule connection termination; however, when
     * connection termination should be scheduled is different in each case.
     * Connection termination may immediately be scheduled in the former case
     * as it may well be the call back of the current thread which is
     * preventing disconnection.  In the latter case connection termination
     * should be scheduled following completion of all transfers currently in
     * progress.  SYSAPs are notified on the completion of each block data
     * transfer through asynchronous invocations of the connection's control
     * event routine.  Once scheduled connection termination commences, the
     * SYSAP should repeatedly attempt to disconnect the connection until
     * either successful or an error other than "connection busy" is returned.
     *
     * NOTE: Allowing SYSAPs to disconnect connections with call backs in
     *	     progress can lead to numerous aberrant situations in SMP
     *	     environments such as SYSAP notification of application message
     *	     reception FOLLOWING notification of connection termination.  There
     *	     is no way to prevent such situations from developing although
     *	     granted the possibility of their occurrence is extremely low.
     *	     Therefore, the decision was made to maintain a consistent meaning
     *	     to connection disconnection and avoid these situations altogether
     *	     by not allowing disconnects while call backs to the SYSAP on the
     *	     connection are in progress.  Ultimately, this requires SYSAPs to
     *	     schedule their connection disconnections.  This was thought to be
     *	     the least of all possible evils.
     */
    Check_connid2( cmsb->connid, cbvte, cb )
    switch( cb->cinfo.cstate ) {

	case CS_LISTEN:
	    if( Test_cbvte_sem( cbvte )) {
		status = RET_CONNBUSY;
	    } else {
		( void )scs_log_event( cb, W_TERM_LISTENER, LSYSAP_EVENT );
		Remove_entry( cb->flink )
		( void )scs_dealloc_cb( cb );
	    }
	    break;

	case CS_OPEN:
	    if( Test_cbvte_sem( cbvte )) {
		status = RET_CONNBUSY;
		break;
	    }
	    /* Fall through.
	     */
	case CS_DISCONN_REC:
	    if( cb->cinfo.ntransfers ) {
		status = RET_CONNBUSY;
	    } else {
		cb->cinfo.status.cwait = 0;
		Remove_pb_waitq( cb )
		if( cb->cinfo.cstate == CS_OPEN ) {
		    cb->cinfo.cstate = CS_DISCONN_SNT;
		} else {
		    cb->cinfo.cstate = CS_DISCONN_MTCH;
		}
		cb->cinfo.reason = cmsb->Reason;
		( void )scs_request( CB_DISCONN_PEND, cb, cb->pb, NULL );
	    }
	    break;

	case CS_CONN_SNT:
	case CS_CONN_ACK:
	    if( cb->cinfo.status.abort_fork ) {
		status = RET_CONNBUSY;
	    } else {
		cb->cinfo.status.abort_fork = 1;
		Remove_pb_waitq( cb )
		Kfork( &cb->forkb, scs_abort_conn, U_long( cbvte->connid ));
	    }
	    break;

	case CS_CONN_REC:
	    cb->cinfo.cstate = CS_REJECT_SNT;
	    cb->cinfo.status.disconnect = 1;
	    cb->cinfo.reason = ADR_DISCONN;
	    ( void )scs_request( CB_REJECT_PEND, cb, cb->pb, NULL );
	    break;

	case CS_PATH_FAILURE:
	    ( void )scs_log_event( cb, W_FAIL_CONN, CONN_EVENT );
	    pb = cb->pb;
	    Remove_cb( cb, pb )
	    if( pb->pinfo.nconns == 0 ) {
		( void )( *pb->Remove_pb )( pb->pccb, pb );
	    }
	    break;

	case CS_CLOSED:
	case CS_DISCONN_ACK:
	case CS_DISCONN_SNT:
	case CS_DISCONN_MTCH:
	case CS_ACCEPT_SNT:
	case CS_REJECT_SNT:
	    status = RET_INVCSTATE;
	    break;

	default:
	    ( void )panic( SCSPANIC_CSTATE );
    }
    Unlock_cbvte( cbvte )
    Unlock_scadb()
    return( status );
}
