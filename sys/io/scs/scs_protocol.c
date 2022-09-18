#ifndef	lint
static char *sccsid = "@(#)scs_protocol.c	4.1	(ULTRIX)	7/2/90";
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
 *		protocol handler functions and routines.
 *
 *   Creator:	Todd M. Katz	Creation Date:	June 15, 1985
 *
 *   Function/Routines:
 *
 *   scs_receive		SCS Sequenced Message Received
 *   scs_request		Send SCS Sequenced Message Request
 *   scs_response		Send SCS Sequenced Message Response
 *   scs_timer			SCS Protocol Interval Timer
 *
 *   Modification History:
 *
 *   06-Jun-1990	Pete Keilty
 *	Corrected the use of a possible null pb pointer in the scs_receive
 *	routine SCS_CONN_REQ to use the pccb->Open_path.
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   10-Feb-1989	Todd M. Katz		TMK0005
 *	1. Move scs_abort_conn() to module ../vaxscs/scs_error.c
 *	2. Add support for SCS event logging.  Log establishments and
 *	   terminations of full SCS connections, and terminations and
 *	   rejections of formative SCS connections.
 *	3. Include header file ../vaxmsi/msisysap.h.
 *
 *   27-Sep-1988	Todd M. Katz		TMK0004
 *	1. Modify the order in which scs_receive() executes the steps required
 *	   to process received SCS sequenced messages.  This re-ordering
 *	   explicitly unlocks the targeted CB prior to initiating transmission
 *	   of an appropriate response or the next request pending transmission
 *	   across the path.  It is necessary because scs_request(), the routine
 *	   responsible for sending all SCS requests, assumes the CB associated
 *	   with the next pending request is NOT locked by the current thread.
 *	   This was not the case when the next request pending transmission is
 *	   on behalf of the CB targeted by the current received SCS response.
 *	   With this re-ordering this assumption is now always true.
 *	2. The sanity check interval for SCS protocol sequences was being
 *	   incorrectly computed by scs_request().  Fix it. 
 *
 *   20-Aug-1988	Todd M. Katz		TMK0003
 *	SCA event codes have been completed revised.  All former SCS path crash
 *	codes are now defined as either error events or severe error events.
 *	The path crash attribute is only applied by the individual port driver
 *	routines responsible for crashing paths and only when the crashed path
 *	is currently open.
 *
 *   12-Jul-1988	Todd M. Katz		TMK0002
 *	Macros Copy_name() and Copy_data() have been renamed to Move_name() and
 *	Move_data() respectively.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, restructured
 *	code paths, and added SMP support.
 */

/*
 *		    System Communication Services Protocol
 *
 * SCS provides logical SCS connections for use as communication conduits
 * between SYSAPs.  The establishment, maintenance, and termination of these
 * connections requires SCS to communicate with equivalent SCS layers on other
 * systems.  Such communication between peers utilizes a SCS specific
 * communication protocol.  Three such protocol sequences are currently
 * defined:
 *
 *	1. Establishment of a logical SCS connection.
 *	2. Maintenance of flow control across a logical SCS connection.
 *	3. Termination of a logical SCS connection.
 *
 * Each SCS protocol sequence consists of the transmission of one or more
 * requests.  For each request transmitted a response is received.  This
 * response acknowledges reception of the request and in some cases terminates
 * the SCS protocol sequence.
 *
 * The connection establishment SCS protocol sequence is triggered by a SYSAP
 * actively requesting a logical SCS connection to a remote counterpart
 * passively listening for such connection requests.  The passive SYSAP
 * responds to the connection request by accepting or rejecting the connection.
 * There are three possible pathways to this SCS protocol sequence:
 *
 *	Active SYSAP					Passive SYSAP
 *	------------					-------------
 *
 *		      Connection can NOT be Established
 *
 *	SCS_CONN_REQ	  ------------------------->
 *			  <-------------------------	SCS_CONN_RSP
 *
 *		    Connection is Accepted and Established
 *
 *	SCS_CONN_REQ	  ------------------------->
 *			  <-------------------------	SCS_CONN_RSP
 *			  <-------------------------	SCS_ACCEPT_REQ
 *	SCS_ACCEPT_RSP	  ------------------------->
 *
 *			    Connection is Rejected
 *
 *	SCS_CONN_REQ	  ------------------------->
 *			  <-------------------------	SCS_CONN_RSP
 *			  <-------------------------	SCS_REJECT_REQ
 *	SCS_REJECT_RSP	  ------------------------->
 *
 * The maintenance flow control SCS protocol sequence is triggered whenever SCS
 * determines that send credits must be extended on a logical SCS connection in
 * order to maintain maximum throughput across the connection.  This occurs as
 * a side-effect of explicit SYSAP action and as a result of SCS processing of
 * deallocated application sequenced messages.  SCS also allows for withdrawal
 * of send credits already extended ( but not used ) through this same
 * sequence.  There is only one possible pathway to this SCS protocol sequence:
 *
 *	Local SCS					Remote SCS
 *	---------					----------
 *
 *	SCS_CREDIT_REQ	  ------------------------->
 *			  <-------------------------	SCS_CREDIT_RSP
 *
 * The connection termination SCS protocol sequence is triggered by a SYSAP
 * actively requesting termination of a logical SCS connection to a remote
 * counterpart.  The remote SYSAP responds to the termination request by
 * issuing its own request for connection termination.  As both the local and
 * remote SYSAPs may simultaneously initiate connection termination there are
 * two possible pathways to this SCS protocol sequence:
 *
 *	Local SYSAP					Remote SYSAP
 *	-----------					------------
 *
 *		      Sequential Exchange of Disconnects
 *
 *	SCS_DISCONN_REQ	  ------------------------->
 *			  <-------------------------	SCS_DISCONN_RSP
 *			  <-------------------------	SCS_DISCONN_REQ
 *	SCS_DISCONN_RSP	  ------------------------->
 *
 *		     Simultaneous Exchange of Disconnects
 *
 *	SCS_DISCONN_REQ	  ------------------------->
 *			  <-------------------------	SCS_DISCONN_REQ
 *	SCS_DISCONN_RSP	  ------------------------->
 *			  <-------------------------	SCS_DISCONN_RSP
 *
 * The transmission of each SCS request is timed.  If a response to a request
 * is not received before the timer expires then the path over which the
 * request was transmitted is terminated.  Because SCS flow control always
 * allows SCS responses to be transmitted, this use of a sanity timer
 * guarantees that SCS peer communication over a path is never impeded by
 * problems on a remote system.
 *
 * Associated with each logical SCS connection is its connection state.  This
 * state impacts on its connection in the following ways:
 *
 * 1. It marks the connection's place within a SCS protocol sequence.
 * 2. It dictates the actions taken by SCS in response to asynchronous events
 *    occurring on the connection.
 * 3. It affects the validity of explicit SYSAP requests on the the connection.
 *
 * The connection state table documented below summarizes for each state the
 * allowed explicit SYSAP requests and the actions taken by SCS in response to
 * asynchronous events.
 *
 * BEGINNING							   ENDING
 * CONNECTION							   CONNECTION
 * STATE	EVENT OR SYSAP REQUEST	ACTION			   STATE
 * ----------	----------------------	-------------------------  ------------
 * CLOSED	Issue LISTEN			-		   LISTEN
 *		Issue CONNECT		Transmit CONN_REQ	   CONN_SNT
 *		Receive CONN_RSP		-		      -
 *		Receive ACCEPT_REQ	Transmit ACCEPT_RSP( F )      -
 *		Receive REJECT_REQ	Transmit REJECT_RSP	      -
 *
 * LISTEN	Issue DISCONNECT		-		   CLOSED
 *		Receive CONN_REQ	Transmit CONN_RSP( S )	   CONN_REC
 *					Notify SYSAP( S )
 *		Discover new path	Notify SYSAP		      -
 *
 * CONN_REC	Issue ACCEPT		Transmit ACCEPT_REQ	   ACCEPT_SNT
 *		Issue REJECT		Transmit REJECT_REQ	   REJECT_SNT
 *		Issue DISCONNECT	Transmit REJECT_REQ	   REJECT_SNT
 *		Path failure			-		   LISTEN
 *
 * CONN_SNT	Issue DISCONNECT	Abort Connection	   CLOSED
 *		Receive CONN_RSP( S )		-		   CONN_ACK
 *		Receive CONN_RSP( F )	Notify SYSAP( F )	   CLOSED
 *		Path failure		Notify SYSAP( F )	   CLOSED
 *
 * CONN_ACK	Issue DISCONNECT	Abort Connection	   CLOSED
 *		Receive ACCEPT_REQ	Transmit ACCEPT_RSP( S )   OPEN
 *					Notify SYSAP( S )
 *		Receive REJECT_REQ	Notify SYSAP( F )	   CLOSED
 *		Path failure		Notify SYSAP( F )	   CLOSED
 *
 * ACCEPT_SNT	Receive ACCEPT_RSP( S )	Notify SYSAP( S )	   OPEN
 *		Receive ACCEPT_RSP( F )	Notify SYSAP( F )	   CLOSED
 *		Receive CONN_REQ	Transmit CONN_RSP( F )	      -
 *		Discover new path	Notify SYSAP		      -
 *		Path failure		Notify SYSAP( F )	   LISTEN
 *
 * REJECT_SNT	Receive REJECT_RSP	Notify SYSAP( S )	   CLOSED
 *		Receive CONN_REQ	Transmit CONN_RSP( F )	      -
 *		Discover new path	Notify SYSAP		      -
 *		Path failure		Notify SYSAP		   LISTEN
 *
 * DISCONN_REC	Issue DISCONNECT	Transmit DISCONN_REQ	   DISCONN_MTCH
 *		Issue DEALLOC_MSG	Deallocate message buffer     -
 *		Issue DEALLOC_DG	Deallocate datagram buffer    -
 *		Issue UNMAP_BUF		Unmap local buffer	      -
 *		Receive CREDIT_RSP		-		      -
 *		Block Transfer Done	Notify SYSAP		      -
 *		Datagram Transmitted	Return Datagram to SYSAP      -
 *		Message Transmitted	Return Message to SYSAP	      -
 *		Path failure		Notify SYSAP		   PATH_FAILURE
 *
 * DISCONN_SNT	Receive DISCONN_RSP		-		   DISCONN_ACK
 *		Receive DISCONN_REQ	Transmit DISCONN_RSP	   DISCONN_MTCH
 *		Receive CREDIT_REQ	Ignore Request		      -
 *					Transmit CREDIT_RSP
 *		Receive CREDIT_RSP		-		      -
 *		Receive Appl Datagram	Discard Datagram	      -
 *		Receive Appl Message	Discard Message		      -
 *		Datagram Transmitted	Discard Datagram	      -
 *		Message Transmitted	Discard Message		      -
 *		Path failure		Notify SYSAP		   CLOSED
 *
 * DISCONN_ACK	Receive DISCONN_REQ	Transmit DISCONN_RSP	   CLOSED
 *					Notify SYSAP
 *		Path failure		Notify SYSAP		   CLOSED
 *
 * DISCONN_MTCH	Receive DISCONN_RSP	Notify SYSAP		   CLOSED
 *		Receive CREDIT_RSP		-		      -
 *		Datagram Transmitted	Discard Datagram	      -
 *		Message Transmitted	Discard Message		      -
 *		Path failure		Notify SYSAP		   CLOSED
 *
 * PATH_FAILURE	Issue DISCONNECT		-		   CLOSED
 *		Issue DEALLOC_MSG	Deallocate message buffer     -
 *		Issue DEALLOC_DG	Deallocate datagram buffer    -
 *		Issue UNMAP_BUF		Unmap local buffer	      -
 *
 * OPEN		Issue DISCONNECT	Transmit DISCONN_REQ	   DISCONN_SNT
 *		Issue ALLOC_DG		Allocate Datagram Buffer      -
 *		Issue DEALLOC_DG	Deallocate Datagram Buffer    -
 *		Issue QUEUE_DGS		Add Receive Datagrams	      -
 *		Issue SEND_DG		Transmit Appl Datagram	      -
 *		Issue ALLOC_MSG		Allocate Message Buffer	      -
 *		Issue DEALLOC_MSG	Deallocate Message Buffer     -
 *		Issue MOD_CREDITS	Transmit CREDIT_REQ	      -
 *		Issue ALLOC_RSPID	Allocate RSPID		      -
 *		Issue SEND_MSG		Transmit Appl Message	      -
 *		Issue MAP_BUF		Map Local Buffer	      -
 *		Issue UNMAP_BUF		Unmap Local Buffer	      -
 *		Issue SEND_DATA		Transfer Block Data	      -
 *		Issue REQ_DATA		Transfer Block Data	      -
 *		Deallocate appl msg	Transmit CREDIT_REQ	      -
 *		Receive DISCONN_REQ	Transmit DISCONN_RSP	   DISCONN_REC
 *					Notify SYSAP
 *		Receive CREDIT_REQ	Process Request		      -
 *					Transmit CREDIT_RSP
 *					Notify SYSAP
 *		Receive CREDIT_RSP		-		      -
 *		Block Transfer Done	Notify SYSAP		      -
 *		Receive Appl Datagram	Return Datagram to SYSAP      -
 *		Receive Appl Message	Return Message to SYSAP	      -
 *		Datagram Transmitted	Return Datagram to SYSAP      -
 *		Message Transmitted	Return Message to SYSAP	      -
 *		Path failure		Notify SYSAP		   PATH_FAILURE
 *
 * NOTES:	1. Associated with all SYSAP notifications and some messages is
 *		   a status.  Success status are indicated by ( S ) while ( F )
 *		   indicates a failure status.
 *		2. The reception of CREDIT_REQ SCS sequenced messages while in
 *		   the OPEN connection state sometimes results in SYSAP
 *		   notification.
 *		3. The deallocation of application sequenced messages while in
 *		   the OPEN connection state sometimes results in transmission
 *		   of SCS CREDIT_REQ sequenced messages.
 *
 * There are some additional rules which serve to explain all the cases not
 * covered by the connection state table:
 *
 * 1. The issuing of a SCS service function by a SYSAP on a logical SCS
 *    connection in any other state returns an error.
 * 2. The reception of a CONN_REQ SCS sequenced message for a logical SCS
 *    connection in any other state panics the local system.
 * 3. The reception of a SCS sequenced message for a logical SCS connection in
 *    any other state results in termination of the path.
 * 4. The reception of an unknown SCS sequenced message results in termination
 *    of the path.
 * 5. Notification of block data transfer completion for a logical SCS
 *    connection in any other state results in termination of the path.
 * 6. The reception of an application datagram or sequenced message for a
 *    logical SCS connection in any other state results in termination of the
 *    path.
 * 7. The completion of application datagram or sequenced message transmission
 *    for a logical SCS connection in any other state results in termination of
 *    the path.
 * 8. The occurrence of path failure for a logical SCS connection in any other
 *    state panics the local system.
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
extern	int		hz;
extern	SCSIB		lscs;
extern	struct lock_t	lk_scadb;
extern	u_char		scs_sanity;
extern	CBVTDB		*scs_cbvtdb;
extern	cbq		scs_listeners; 
extern	pbq		scs_timeoutq;
extern	CB		*scs_alloc_cb();
extern	void		scs_dealloc_cb(), scs_init_cmsb(), scs_log_event(),
			scs_request(), scs_response(), scs_timer(), timeout();

/*   Name:	scs_receive	- SCS Sequenced Message Received
 *
 *   Abstract:	This routine processes and disposes of SCS sequenced messages
 *		received over specific paths.  Such messages contain either
 *		SCS requests or SCS responses.  There are five types of SCS
 *		requests:
 *
 *		1. Request for establishment of a logical SCS connection.
 *		2. Request for acceptance of a connection request.
 *		3. Request for rejection of a connection request.
 *		4. Request for termination of a logical SCS connection.
 *		5. Request for credit extension/withdrawal.
 *
 *		Processing of each request includes transmission of an
 *		appropriate SCS response.  The message buffer containing the
 *		received request is used to transmit the response.  The
 *		response can be immediately transmitted because the message
 *		buffer which contained the SCS request on the originating
 *		system has been reserved for reception of the corresponding SCS
 *		response.
 *
 *		There are five types of SCS responses:
 *
 *		1. Response to a connection request.
 *		2. Response to an acceptance request.
 *		3. Response to an rejection request.
 *		4. Response to a termination request.
 *		5. Response to a credit request.
 *
 *		Following processing of each response, its message buffer is
 *		used to transmit the next SCS request pending on the path.  If
 *		If there are no such pending requests the message buffer
 *		becomes the path's SCS send message buffer.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *   scsbp			- Address of SCS header of message buffer
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *   scs_listeners		- Listening SYSAP queue head
 *   scs_timeoutq		- SCS protocol sequence timeout queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.cstate		-  Connection state
 *	cinfo.snd_credit	-  Number of send credits
 *	cinfo.status.cwait	-  SYSAP waiting for credits flag
 *	errlogopt.rreason	-  SYSAP/SCS connection rejection reason
 *   pb				- Path Block pointer
 *	pinfo.nconns		-  Number of connections
 *	pinfo.status.sanity	-  Sanity timer enabled flag
 *
 *   SMP:	The SCA database is locked for CBVTE allocation and CB
 *		insertion into the system-wide configuration database in some
 *		cases, or for CB removal and CBVTE deallocation from this
 *		database in others.  It is also required for traversing the
 *		queue of listening SYSAPs, for PB retrieval, and for PB removal
 *		from the SCS protocol sequence timeout queue.  Locking the SCA
 *		database also prevents premature PB deletion as required by
 *		scs_init_cmsb(), scs_request(), scs_response() and PD routines
 *		which both crash and open paths.
 *
 *		The CB is locked to synchronize access, for deletion in some
 *		cases, to prevent deletion in others, and as required by
 *		scs_log_event() in case logging becomes necessary.  It is
 *		indirectly locked through its CBVTE.
 *
 *		The CBVTE semaphore is incremented in certain situations prior
 *		to SYSAP notification and synchronized to in others.
 *		Incrementing the semaphore prevents any changes in connection
 *		state while SYSAP notification is in progress.  The semaphore
 *		is decremented after notification completes.
 *
 *		The PB is locked only when it is found necessary to update it,
 *		or remove the CB from the PB's work queue, or remove the PB
 *		from the SCS protocol sequence timeout queue.  This leaves the
 *		PB susceptible to path failure and even to a portion of the
 *		clean up which accompanies such failures.  However this never
 *		presents any problems because the PB is always protected
 *		against deletion by various mechanisms( SCA database lock, CB
 *		lock, CBVTE semaphore ) whenever such protection is required.
 *		PBs are always unlocked after the tasks requiring their locking
 *		have completed because many routines invoked by scs_receive
 *		explicitly prohibit any PB locks at the time of their
 *		invocation.
 *
 *		PCCB addresses are always valid allowing access to static
 *		fields because PCCBs are never deleted once SCS becomes aware
 *		of their existence.
 */
void
scs_receive( pccb, scsbp )
    PCCB		*pccb;
    register SCSH	*scsbp;
{
    register CB		*cb;
    register PB		*pb;
    register CBVTE	*cbvte;
    CMSB		cmsb;
    register u_long	notify_event = 0;
    CBVTE		*lcbvte = NULL;
    u_long		aborted_conn = 0, event_code = 0, semaphore = 0,
			status = ADR_SUCCESS;

    /* The steps involved in processing received SCS sequenced messages are:
     *
     *  1. Lock the SCA database.
     *  2. Lock and retrieve the CB targeted by the received SCS message.
     *  3. Determine whether the local SYSAP unilaterally aborted connection 
     *     establishment.
     *  4. Retrieve the PB over which the message was SCS received.
     *  5. Process the message according to message SCS type and current
     *     connection state.
     *  6. Optionally remove the CB from the system-wide configuration database
     *	   and deallocate it along with all associated resources.
     *  7. Unlock the CB( if locked ).
     *  8. Initiate transmission of an appropriate response or the next request
     *     pending transmission across the path.
     *  9. Unlock the SCA database.
     * 10. Optionally notify the local SYSAP of the occurrence of a specific
     *	   event on the connection.
     *
     * Steps 2-3 are bypassed for received messages containing connection
     * requests.  No local CB exists to be retrieved or to be checked for those
     * connection identification number mismatches which when associated with
     * the reception of specific types of messages signal unilateral connection
     * abortion.
     *
     * Little processing is required in Step 5 if connection establishment has
     * or is in the process of being unilaterally aborted( Step 3 ).  Steps 6
     * and 10 are also bypassed in such circumstances.
     *
     * Whether response or next request transmission is initiated( Step 8 )
     * depends upon whether the received message contained a request or a
     * response respectively.  This determination is optimized by the following
     * fact:
     *
     *		SCS requests have EVEN numbered message types while SCS
     *		responses have ODD numbered message types.
     *
     * Prior to initiation of next request transmission, the sanity check on
     * the appropriate path is aborted by removing the corresponding PB from
     * the SCS protocol sequence timeout queue.  This may be done because
     * reception of the response signals reception of the current request by
     * the remote SCS.  Note that the SCS interval timer is not unscheduled
     * even when there are no longer any ongoing SCS protocol sequences.  The
     * next invocation of the SCS interval timer routine discovers the absence
     * of work and "suspends" its own execution until there are SCS protocol
     * sequences requiring timing.
     *
     * CB removal and deallocation( Step 6 ) is dependent upon message
     * reception triggering of CB closure.  Local SYSAP notification( Step 10 )
     * is also dependent upon some event occurring as a result of message
     * reception and occurs through asynchronous invocation of the connection's
     * control event routine.  By optionally incrementing the appropriate CBVTE
     * semaphore before the CB is unlocked( Step 7 ), connections are
     * protected during such call backs against state changes.  The semaphore
     * is decremented upon completion of local SYSAP notification.  Whether the
     * connection requires protection is dependent upon both the event the
     * SYSAP is being notified of and the connection state.
     *
     * Steps 2-3 and 5 may uncover any one of the following error conditions:
     *
     * o The CB can not be retrieved.
     * o A connection identification number mismatch exists and is not due to
     *	 unilateral abortion of connection establishment.
     * o A message with an unknown SCS message type is received.
     * o The connection state is found to be inappropriate for the SCS message
     *   received.
     *
     * When one of these conditions is detected:
     *
     * 1. Message processing is aborted.
     * 2. The path on which the message was received is crashed.
     * 3. The message is returned to the appropriate local port free message
     *	  pool.
     * 4. All held locks are released.
     *
     * This course of action may seem to be drastic, but none of these
     * conditions is ever expected in a single processor environment.  The same
     * is not true for SMP environments where the occurrence of any of these
     * conditions is possible, albeit with an extremely low frequency, whenever
     * a path or local port fails.  This is because no mechanisms exist to
     * coordinate PD event notifications of SCS as there are mechanisms to
     * coordinate SCS notifications of SYSAPs.  This lack of synchronization
     * allows SCS sequenced messages to be received after notifications of path
     * failure even though PDs had initiated the former before the latter.  The
     * actions taken remain the same as in the single processor case even
     * though they are drastic and may well result in double crashing of path
     * or port.  There is no other alternative.
     *
     * The following table lists supported SCS message types and the processing
     * messages of each type undergoes( Step 5-6,8, 10 ) when one is received:
     *
     * SCS Message Type		Actions
     *
     * SCS_CONN_REQ		1. The PB is retrieved
     *				2. The path is opened( if not already open ).
     *				3. Search the queue of listening connections
     *				   for the target SYSAP.
     *				4. Allocate, initialize, and lock a CB for the
     *				   new logical SCS connection.  Set its
     *				   connection state to CONN_REC.
     *				5. Insert the new CB into the system-wide
     *				   configuration database.
     *				6. Initiate transmission of a SCS_CONN_RSP SCS
     *				   sequenced message containing an appropriate
     *				   status.
     *				7. Protect the listening connection while
     *				   notifying the local SYSAP of the request for
     *				   connection establishment.
     *
     * Failure to retrieve the PB( Step 1 ) or to open the path( Step 2 )
     * aborts the remaining steps.  The message is returned to the appropriate
     * local port message free pool and the connection request is forgotten
     * about.  The remote port eventually discovers the path does not exist
     * either through polling or through timeout of the connection request.
     * Failure to find the target SYSAP( Step 3 ) or allocate a new CB( Step 4)
     * forces bypassing of Steps 5 and 7 and transmission of an appropriate
     * error status in the response to the connection request( Step 6 ).
     *
     * SCS_CONN_RSP		- If return status == ADR_SUCCESS:
     *				1. Transition the connection into the CONN_ACK
     *				   connection state.
     *				2. "Wait" for the remote SYSAP to accept|reject
     *				   the connection request.
     *				- If return status != ADR_SUCCESS:
     *				1. Log formative connection termination.
     *				2. Close the formative connection.
     *				3. Remove the CB from the system-wide
     *				   configuration database.
     *				4. Deallocate the CB along with all associated
     *				   resources.
     *				5. Notify the local SYSAP of failure to
     *				   establish a logical SCS connection.
     *
     * SCS_ACCEPT_REQ		1. Update the CB.
     *				2. Log establishment of new SCS connection.
     *				3. Open the connection.
     *				4. Initiate transmission of a SCS_ACCEPT_RSP
     *				   SCS sequenced message containing an
     *				   appropriate status.
     *				5. Protect the connection while notifying the
     *				   local SYSAP of successful connection
     *				   establishment.
     *
     * SCS_ACCEPT_RSP		- If return status == ADR_SUCCESS:
     *				1. Log establishment of new SCS connection.
     *				2. Open the connection.
     *				3. Protect the connection while notifying the
     *				   local SYSAP of successful connection
     *				   establishment.
     *				- If return status != ADR_SUCCESS:
     *				1. Log formative connection termination.
     *				2. Close the formative connection.
     *				3. Remove the CB from the system-wide
     *				   configuration database.
     *				4. Deallocate the CB along with all associated
     *				   resources.
     *				5. Notify the local SYSAP of failure to
     *				   establish a logical SCS connection.
     *
     * SCS_REJECT_REQ		1. Log formative connection rejection.
     *				2. Close the formative connection.
     *				3. Remove the CB from the system-wide
     *				   configuration database.
     *				4. Deallocate the CB along with all associated
     *				   resources.
     *				5. Initiate transmission of a SCS_REJECT_RSP
     *				   SCS sequenced message.
     *				6. Notify the local SYSAP of rejection of its
     *				   connection request.
     *
     * SCS_REJECT_RSP		1. Log formative connection rejection.
     *				2. Close the formative connection.
     *				3. Remove the CB from the system-wide
     *				   configuration database.
     *				4. Deallocate the CB along with all associated
     *				   resources.
     *				5. Notify the local SYSAP of successful
     *				   rejection of the connection request.
     *
     * SCS_DISCONN_REQ		1. Synchronize to the CBVTE semaphore.
     *				- If the connection is OPEN:
     *				2. Prohibit further activity on the connection
     *				   by transitioning it to the DISCONN_REC
     *				   connection state.
     *				3. Abort any pending credit extensions.
     *				4. Initiate transmission of a SCS_DISCONN_RSP
     *				   SCS sequenced message.
     *				5. Protect the connection while notifying the
     *				   local SYSAP of the request for connection
     *				   termination.
     *				- If the local SYSAP has sent but not received
     *				  an acknowledgement of its disconnect request:
     *				2. Transition the connection into the
     *				   DISCONN_MTCH connection state.
     *				3. Initiate transmission of a SCS_DISCONN_RSP
     *				   SCS sequenced message.
     *				4. "Wait" for the remote SYSAP to acknowledge
     *				   the local SYSAP's request for connection
     *				   termination.
     *				- If the local SYSAP has sent and received an
     *				  acknowledgement of its disconnect request:
     *				2. Log connection termination.
     *				3. Close the connection.
     *				4. Remove the CB from the system-wide
     *				   configuration database.
     *				5. Deallocate the CB along with all associated
     *				   resources.
     *				6. Initiate transmission of a SCS_DISCONN_RSP
     *				   SCS sequenced message.
     *				7. Notify the local SYSAP of connection
     *				   termination.
     *
     * Synchronization to the CBVTE semaphore postpones processing of the
     * disconnect request and subsequent changes in the state of the connection
     * while call backs to the corresponding SYSAP are in progress.  Failure to
     * synchronize can lead to numerous aberrant situations in SMP environments
     * such as SYSAP notification of application message reception FOLLOWING
     * notification of its remote counterpart's request for connection
     * termination.  Semaphore synchronization is achieved by releasing the SCA
     * database and CB locks and re-obtaining them in the proper order until
     * all call backs have completed( CBVTE semaphore == 0 ).  Only open
     * connections require repeated synchronization attempts because only open
     * connections may have call backs to the corresponding SYSAP in progress.
     * Connection failure and termination due to path failure may occur during
     * a synchronization attempt between the time locks are released and
     * re-established.  When such failures occur:
     *
     * 1. All attempts to synchronize to the semaphore are immediately
     *	  terminated.
     * 2. The DISCONN_REQ SCS sequenced message is returned to the appropriate
     *    local port message free pool without processing the request.
     * 3. All locks are released.
     *
     * SCS_DISCONN_RSP		- If the local SYSAP has sent but not received
     *				  a request for connection termination.
     *				1. Transition the connection into the
     *				   DISCONN_ACK connection state.
     *				2. "Wait" for the remote SYSAP to issue a
     *				   matching request for connection termination.
     *				- If the local SYSAP has both sent and received
     *				  a request for connection termination.
     *				1. Log connection termination.
     *				2. Close the connection.
     *				3. Remove the CB from the system-wide
     *				   configuration database.
     *				4. Deallocate the CB along with all associated
     *				   resources.
     *				5. Notify the local SYSAP of connection
     *				   termination.
     *
     *
     * SCS_CREDIT_REQ		1. Update the CB
     *				- If credits are being extended:
     *				2. Initiate transmission of a SCS_CREDIT_RSP
     *				   SCS sequenced message containing the number
     *				   of credits accepted.
     *				3. Protect the connection while notifying the
     *				   local SYSAP of the availability of send
     *				   credits provide it had been waiting for such
     *				   notification.
     *				- If credits are being withdrawn:
     *				2. Initiate transmission of a SCS_CREDIT_RSP
     *				   SCS sequenced message containing the actual
     *				   number of credits withdrawn.
     *
     * When credits are being withdrawn, the available number of send credits
     * is never allowed to fall below the local SYSAP's minimum send credit
     * requirement.
     *
     * SCS_CREDIT_RSP		No required message type dependent processing.
     *     
     * While no message type dependent processing is required, the path is
     * crashed whenever a confirmation of credit withdrawal is received as
     * Ultrix never attempts to withdraw credits.
     *
     * More information about the SCS protocol sequences and the state changes
     * connections may undergo may be found by referring to the comments at the
     * front of this module.
     */
    Lock_scadb()
    if( scsbp->mtype != SCS_CONN_REQ ) {
	if( scsbp->rconnid.index > ( lscs.max_conns - 1 )) {
	    event_code = SE_BADCONNID;
	} else {
	    cbvte = Get_cbvte( scsbp->rconnid );
	    Lock_cbvte( cbvte )
	    cb = Get_cb( cbvte );
	    if( scsbp->rconnid.seq_num != cbvte->connid.seq_num ) {
		if( scsbp->mtype == SCS_ACCEPT_REQ  ||
		     scsbp->mtype == SCS_REJECT_REQ ||
		     scsbp->mtype == SCS_CONN_RSP ) {
		    aborted_conn = 1;
		} else {
		    event_code = SE_BADCONNID;
		    Unlock_cbvte( cbvte )
		}
	    }
	}
	if( event_code ) {
	    pb = ( *pccb->Get_pb )( pccb, scsbp, BUF );
	    ( void )( *pccb->Crash_path )( pccb,
					   pb,
					   event_code,
					   RECEIVE_BUF,
					   scsbp );
	    Unlock_scadb()
	    return;
	} else {
	    pb = cb->pb;
	}
    }
    switch( scsbp->mtype ) {

	case SCS_CONN_REQ: {
	    register cbq	*lcb;

	    if((( pb = ( *pccb->Get_pb )( pccb, scsbp, BUF )) == NULL ||
	           pb->pinfo.state != PS_OPEN ) &&
		 ( *pccb->Open_path )( pccb, pb, scsbp ) != RET_SUCCESS ) {
		( void )( *pccb->Add_msg )( pccb, scsbp );
		Unlock_scadb()
		return;
	    }
	    cbvte = NULL;
	    Conn_rsp( scsbp )->Req_status = ADR_NOLISTENER;
	    for( lcb = scs_listeners.flink;
		 lcb != &scs_listeners;
		 lcb = lcb->flink ) {
		if( Comp_name( Conn_req( scsbp )->rproc_name,
			       Lcb->cinfo.lproc_name )) {
		    if( Lcb->cinfo.cstate != CS_LISTEN ) {
			( void )panic( SCSPANIC_LQUEUE );
		    } else if(( cb = scs_alloc_cb( CS_CONN_REC, NULL, pb ))) {
			lcbvte = Get_cbvte( Lcb->cinfo.lconnid );
			cb->control = Lcb->control;
			cb->cinfo.snd_credit = Conn_req( scsbp )->credit;
			Move_connid( Conn_req( scsbp )->sconnid,
				     cb->cinfo.rconnid )
			cb->cinfo.min_rec_credit =
				Conn_req( scsbp )->min_credit;
			Move_name( Conn_req( scsbp )->rproc_name,
				   cb->cinfo.lproc_name )
			Move_name( Conn_req( scsbp )->sproc_name,
				   cb->cinfo.rproc_name )
			Move_data( Conn_req( scsbp )->sproc_data,
				   cb->cinfo.rconn_data )
			cbvte = Get_cbvte( cb->cinfo.lconnid );
			Lock_cbvte( cbvte )
			Insert_cb( cb, pb )
			Conn_rsp( scsbp )->Req_status = ADR_SUCCESS;
			Move_connid( cb->cinfo.lconnid,
				     Conn_req( scsbp )->rconnid )
			notify_event = CRE_CONN_REC;
		    } else {
			Conn_rsp( scsbp )->Req_status = ADR_NORESOURCE;
		    }
		    break;
		}
	    }
	    break;
	}

	case SCS_CONN_RSP:
	    if( aborted_conn || cb->cinfo.status.abort_fork ) {
		;
	    } else if( cb->cinfo.cstate == CS_CONN_SNT ) {
		if( Conn_rsp( scsbp )->Req_status == ADR_SUCCESS ) {
		    cb->cinfo.cstate = CS_CONN_ACK;
		} else {
		    status = Conn_rsp( scsbp )->Req_status;
		    cb->errlogopt.rreason = status;
		    ( void )scs_log_event( cb, W_TERM_FCONN, CONN_EVENT );
		    cb->cinfo.cstate = CS_CLOSED;
		    notify_event = CRE_CONN_DONE;
		}
	    } else {
		event_code = SE_BADCSTATE;
	    }
	    break;

	case SCS_ACCEPT_REQ:
	    if( aborted_conn || cb->cinfo.status.abort_fork ) {
		Accept_rsp( scsbp )->Req_status = ADR_DISCONN;
	    } else if( cb->cinfo.cstate == CS_CONN_ACK ) {
		Accept_rsp( scsbp )->Req_status = ADR_SUCCESS;
		Move_connid( Accept_req( scsbp )->sconnid, cb->cinfo.rconnid )
		cb->cinfo.snd_credit = Accept_req( scsbp )->credit;
		cb->cinfo.min_rec_credit = Accept_req( scsbp )->min_credit;
		Move_data( Accept_req( scsbp )->sproc_data,
			   cb->cinfo.rconn_data )
		( void )scs_log_event( cb, I_NEW_CONN, CONN_EVENT );
		cb->cinfo.cstate = CS_OPEN;
		notify_event = CRE_CONN_DONE;
		semaphore = 1;
	    } else {
		event_code = SE_BADCSTATE;
	    }
	    break;

	case SCS_ACCEPT_RSP:
	    if( cb->cinfo.cstate == CS_ACCEPT_SNT ) {
		notify_event = CRE_ACCEPT_DONE;
		if( Accept_rsp( scsbp )->Req_status == ADR_SUCCESS ) {
		    ( void )scs_log_event( cb, I_NEW_CONN, CONN_EVENT );
		    cb->cinfo.cstate = CS_OPEN;
		    semaphore = 1;
		} else {
		    status = Accept_rsp( scsbp )->Req_status;
		    cb->errlogopt.rreason = status;
		    ( void )scs_log_event( cb, W_TERM_FCONN, CONN_EVENT );
		    cb->cinfo.cstate = CS_CLOSED;
		}
	    } else {
		event_code = SE_BADCSTATE;
	    }
	    break;

	case SCS_REJECT_REQ:
	    if( aborted_conn || cb->cinfo.status.abort_fork ) {
		;
	    } else if( cb->cinfo.cstate == CS_CONN_ACK ) {
		status = Reject_req( scsbp )->reason;
		cb->errlogopt.rreason = status;
		( void )scs_log_event( cb, W_REJECT_FCONN, CONN_EVENT );
		cb->cinfo.cstate = CS_CLOSED;
		notify_event = CRE_CONN_DONE;
	    } else {
		event_code = SE_BADCSTATE;
	    }
	    break;

	case SCS_REJECT_RSP:
	    if( cb->cinfo.cstate == CS_REJECT_SNT ) {
		cb->cinfo.cstate = CS_CLOSED;
		if( !cb->cinfo.status.disconnect ) {
		    notify_event = CRE_REJECT_DONE;
		} else {
		    notify_event = CRE_DISCONN_DONE;
		}
	    cb->errlogopt.rreason = cb->cinfo.reason;
	    ( void )scs_log_event( cb, W_REJECT_FCONN, CONN_EVENT );
	    } else {
		event_code = SE_BADCSTATE;
	    }
	    break;

	case SCS_DISCONN_REQ:
	    while( Test_cbvte_sem( cbvte )) {
		Unlock_cbvte( cbvte )
		Unlock_scadb()
		Lock_scadb()
		Lock_cbvte( cbvte )
		if( scsbp->sconnid.seq_num != cbvte->connid.seq_num ||
		     cb->cinfo.cstate == CS_PATH_FAILURE ) {
		    ( void )( *pccb->Add_msg )( pccb, scsbp );
		    Unlock_cbvte( cbvte )
		    Unlock_scadb()
		    return;
		}
	    }
	    if ( cb->cinfo.cstate == CS_OPEN ) {
		cb->cinfo.cstate = CS_DISCONN_REC;
		Remove_pb_waitq( cb )
		notify_event = CRE_DISCONN_REC;
		status = Disconn_req( scsbp )->reason;
		semaphore = 1;
	    } else if( cb->cinfo.cstate == CS_DISCONN_SNT ) {
		cb->cinfo.cstate = CS_DISCONN_MTCH;
	    } else if( cb->cinfo.cstate == CS_DISCONN_ACK ) {
		( void )scs_log_event( cb, W_TERM_CONN, CONN_EVENT );
		cb->cinfo.cstate = CS_CLOSED;
		notify_event = CRE_DISCONN_DONE;
	    } else {
		event_code = SE_BADCSTATE;
	    }
	    break;

	case SCS_DISCONN_RSP:
	    if( cb->cinfo.cstate == CS_DISCONN_SNT ) {
		cb->cinfo.cstate = CS_DISCONN_ACK;
	    } else if( cb->cinfo.cstate == CS_DISCONN_MTCH ) {
		( void )scs_log_event( cb, W_TERM_CONN, CONN_EVENT );
		cb->cinfo.cstate = CS_CLOSED;
		notify_event = CRE_DISCONN_DONE;
	    } else {
		event_code = SE_BADCSTATE;
	    }
	    break;

	case SCS_CREDIT_REQ: {
	    register long	credits;

	    /* Only credit requests for open connections are processed.  Credit
	     * requests for connections in the DISCONN_SNT connection state(
	     * the local SYSAP has requested connection termination but the
	     * request has not yet been acknowledged by the remote SCS ) are
	     * not processed and the path is crashed if the connection is
	     * found to be in any other state.
	     */
	    if( cb->cinfo.cstate == CS_OPEN ) {
		if(( credits = scsbp->credit ) > 0 ) {
		    cb->cinfo.snd_credit += credits;
		    if( cb->cinfo.status.cwait ) {
			cb->cinfo.status.cwait = 0;
			notify_event = CRE_CREDIT_AVAIL;
			semaphore = 1;
		    }
		} else if( credits < 0 ) {
		    if(( credits = cb->cinfo.snd_credit -
				   cb->cinfo.min_snd_credit ) <= 0 ) {
			scsbp->credit = 0;
		    } else {
			if( credits < -scsbp->credit ) {
			    scsbp->credit = -credits;
			cb->cinfo.snd_credit += scsbp->credit;
			}
		    }
		}
	    } else {
		if( cb->cinfo.cstate != CS_DISCONN_SNT ) {
		    event_code = SE_BADCSTATE;
		}
	    }
	    break;
	}

	case SCS_CREDIT_RSP:
	    if( cb->cinfo.cstate != CS_OPEN	    &&
		 cb->cinfo.cstate != CS_DISCONN_SNT &&
		 cb->cinfo.cstate != CS_DISCONN_REC &&
		 cb->cinfo.cstate != CS_DISCONN_MTCH ) {
		event_code = SE_BADCSTATE;
	    } else if( scsbp->credit < 0 ) {
		event_code = SE_NEGCREDITS;
	    }
	    break;

	default:
	    event_code = SE_BADSCSMTYPE;
    }
    {
    register void	( *control )();

    if( notify_event ) {
	( void )scs_init_cmsb( notify_event, status, &cmsb, cb, pb, 0 );
	control = cb->control;
	if( cb->cinfo.cstate == CS_CLOSED ) {
	    Remove_cb( cb, pb )
	} else if( semaphore ) {
	    Incr_cbvte_sem( cbvte )
	} else if( lcbvte ) {
	    Lock_cbvte( lcbvte )
	    Incr_cbvte_sem( lcbvte )
	    Unlock_cbvte( lcbvte )
	}
    }
    if( cbvte ) {
	Unlock_cbvte( cbvte )
    }
    if( event_code == 0 ) {
	if(!( scsbp->mtype & 1 )) {
	    ( void )scs_response( scsbp, pb );
	} else {
	    Lock_pb( pb )
	    if( pb->pinfo.status.sanity ) {
		Remove_scs_timeoutq( pb )
	    } else if( pb->pinfo.state != PS_PATH_FAILURE ) {
		( void )panic( SCSPANIC_SANITY );
	    }
	    Unlock_pb( pb )
	    ( void )scs_request( 0, NULL, pb, scsbp );
	}
    } else {
	( void )( *cb->Crash_path )( pccb, pb, event_code, RECEIVE_BUF, scsbp);
    }
    Unlock_scadb()
    if( notify_event ) {
	( void )( *control )( notify_event, &cmsb );
	if( semaphore ) {
	    Decr_cbvte_sem( cbvte )
	} else if( lcbvte ) {
	    Decr_cbvte_sem( lcbvte )
	}
    }
    }
}

/*   Name:	scs_request	- Send SCS Sequenced Message Request
 *
 *   Abstract:	This routine initiates the transmission of a SCS request over
 *		a specific path.  Five such requests are supported:
 *
 *		1. Request establishment of a logical SCS connection.
 *		2. Request acceptance of a logical SCS connection.
 *		3. Request rejection of a logical SCS connection.
 *		4. Request termination of a logical SCS connection.
 *		5. Request extension of send credits across a logical SCS
 *		   connection.
 *
 *		SCS applies flow control to its own communications on a per
 *		path basis.  Only one SCS request may be transmitted across
 *		each path at a given moment.  The transmission of all
 *		subsequent requests is postponed until a response to the active
 *		request is received.
 *
 *		Flow control is provided by allocating for each path a single
 *		message buffer.  These buffers are attached to their PBs as SCS
 *		send message buffers when not in use.  A SCS request is
 *		immediately transmitted only when the SCS send message buffer
 *		for the path is available.  Otherwise, transmission of the
 *		request is postponed by queuing the CB on whose behalf the
 *		request was made to the appropriate PB until the SCS send
 *		message buffer is again available.
 *
 *		Message buffers containing SCS requests are always added to the
 *		appropriate ports' free message pools following their
 *		transmissions.  There they serve as receive message buffers for
 *		their corresponding SCS responses.  After receiving and
 *		processing a SCS response its message buffer is used to
 *		transmit the next SCS request pending on the appropriate
 *		path.  The message buffer is attached to the PB as its SCS send
 *		message buffer if there are no such requests pending.
 *
 *		A sanity timer is set on the target path prior to transmission
 *		of a SCS request.  SCS assumes the path has failed and
 *		explicitly crash it if this timer expires before the SCS
 *		response to the current SCS request is received.
 *
 *		There are two ways in which this routine may be invoked.
 *
 *		1. The routine may be invoked to process a specific SCS request
 *		   on behalf of an explicitly provided connection.  Such
 *		   invocations never supply the message buffer to contain the
 *		   request( scsbp == NULL ).
 *
 *		2. The routine may be invoked to resume processing of the next
 *		   SCS request pending transmission on the path.  Such
 *		   invocations never supply the connection on whose behalf the
 *		   transmission is to be made( cb == NULL ) or the explicit
 *		   request type( request == CB_NOT_WAIT ) but always supply the
 *		   message buffer to contain the request.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *   request			- Request type
 *   pb				- Path Block pointer
 *   scsbp			- Address of SCS header of message buffer
 *   scs_sanity			- SCS sanity timer interval
 *   scs_timeoutq		- SCS protocol sequence timeout queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *	cinfo.cbstate		-  CB state
 *	cinfo.pend_rec_credit	-  Number of credits pending extension
 *	cinfo.reason		-  Rejection/Disconnection reason
 *	cinfo.rec_credit	-  Send credit held by remote SYSAP
 *   pb				- Path Block pointer
 *	pinfo.duetime		-  SCS response due time( seconds )
 *	pinfo.status.sanity	-  1
 *	scs_msgbuf		-  Address of SCS send message buffer
 *	timeout			-  SCS protocol sequence timeout queue pointers
 *
 *   SMP:	The SCA database is locked( EXTERNALLY ) to guarantee the
 *		validity of PB addresses and to allow for PB insertion onto the
 *		SCS protocol sequence timeout queue.
 *
 *		The CB is locked to synchronize access and to prevent deletion.
 *		Locking the CB also prevents PB deletion as required by PD
 *		routines which transmit messages.  Only those CBs retrieved by
 *		this routine from PB work queues are explicitly locked by it
 *		and these CBs are unlocked prior to exiting.  CBs provided to
 *		this routine must be locked EXTERNALLY and are never unlocked
 *		before exiting.  CBs are always indirectly locked through their
 *		CBVTEs.  
 *
 *		The PB is locked to synchronize access, to prevent deletion,
 *		and for PB insertion onto the SCS protocol sequence timeout
 *		queue.  PBs are always unlocked just prior to request
 *		transmission as required by PD routines which transmit
 *		messages in this particular case.
 */
void
scs_request( request, cb, pb, scsbp )
    register u_long	request;
    register CB		*cb;
    register PB		*pb;
    register SCSH	*scsbp;
{
    register CBVTE	*cbvte;
    static struct {
	u_short	mtype;
	u_short	mlength;
	struct	{
	    u_char credits	: 1;
	    u_char conn_info	: 1;
	    u_char		: 6;
	} flags;
    } format[ 5 ] = {
	{ SCS_CONN_REQ,	   sizeof( CONN_REQ ),	  1,	1 },
	{ SCS_ACCEPT_REQ,  sizeof( ACCEPT_REQ ),  1,	1 },
	{ SCS_REJECT_REQ,  sizeof( REJECT_REQ ),  0,	0 },
	{ SCS_DISCONN_REQ, sizeof( DISCONN_REQ ), 0,	0 },
	{ SCS_CREDIT_REQ,  sizeof( CREDIT_REQ ),  1,	0 }
	};

    /* Servicing SCS requests and initiating transmission of SCS sequenced
     * messages proceeds as follows:
     *
     *  1. Lock the PB.
     *  2. Verify the path state.
     *  3. Obtain both a CB on whose behalf the request is to be made and a
     *	   port specific message buffer to contain the request as needed.
     *  4. Lock the CB whenever it was obtained by this routine and not
     *	   provided to it.
     *  5. Format the SCS sequenced message according to the its type.
     *  6. Unlock the PB.
     *  7. Initiate sequenced message transmission.
     *  8. Lock the PB.
     *  9. Start a sanity check on this current protocol sequence provided the
     *	   path is still open.
     * 10. Unlock the PB.
     * 11. Unlock the CB whenever it was obtained by this routine and not
     *	   provided to it.
     *
     * Only failure of the path can result in a failure to verify the path
     * state( Step 2 ).  Servicing of the current request is aborted and any
     * message buffer provided to this routine is attached to the PB as its SCS
     * send message buffer.  Provided CBs are not placed on the PB work queue
     * because their requests would never be serviced anyway due to failure of
     * the path.
     *
     * When obtaining CBs and message buffers( Step 3 ), the following two
     * possibilities exist:
     *
     * 1. The message buffer is explicitly provided but the CB is not.
     * 2. The CB is explicitly provided but the message buffer is not.
     *
     * The first possibility occurs when when servicing of a postponed request
     * is to resume making use of the explicitly provided message buffer.  The
     * very first CB on the PB's work queue is obtained( Step 3 ) and locked(
     * Step 4 ).  The absence of connections with postponed requests or failure
     * of the path immediately terminates next request processing.  The
     * explicitly provided message buffer is attached to the PB as its SCS
     * send message buffer and the PB is unlocked before returning.  The PB
     * must always be unlocked and then re-locked after locking each CB in
     * order to maintain the SCA locking hierarchy.  Cached PB addresses remain
     * valid because possession of the SCA database lock prevents PB deletion.
     *
     * The second possibility occurs when servicing on behalf of a connection
     * is directly requested.  The PB's SCS send message buffer is retrieved(
     * Step 3 ).  Unavailability of this buffer forces postponement of the
     * request by caching the request type and placing the CB on to the PB work
     * queue.
     *
     * Sanity checks on current protocol sequences( Step 9 ) are initiated as
     * follows:
     *
     * 1. Compute the maximum time interval for reception of the corresponding
     *    response.
     * 2. Place the PB on to the SCS protocol sequence timeout queue.
     * 3. Schedule the SCS interval timer routine whenever the queue was
     *    previously empty.
     *
     * PBs are removed from the SCS protocol sequence timeout queue when
     * corresponding responses are received.  SCS assumes a path has failed if
     * the proper response is not received before the PB's sanity interval
     * timer expires and the path is crashed.
     */
    Lock_pb( pb )
    if( pb->pinfo.state != PS_OPEN ) {
	if( scsbp ) {
	    pb->scs_msgbuf = scsbp;
	}
	Unlock_pb( pb )
	return;
    } else if( cb == NULL ) {
	while( pb->scs_cb.flink != &pb->scs_cb ) {
	    cb = Pos_to_cb( pb->scs_cb.flink, scs_cb );
	    cbvte = Get_cbvte( cb->cinfo.lconnid );
	    Unlock_pb( pb )
	    Lock_cbvte( cbvte )
	    Lock_pb( pb )
	    if( pb->pinfo.state == PS_OPEN && &cb->scs_cb == pb->scs_cb.flink){
		Remove_entry( cb->scs_cb )
		request = cb->cinfo.cbstate;
		cb->cinfo.cbstate = CB_NOT_WAIT;
		break;
	    } else {
		Unlock_cbvte( cbvte )
		if( pb->pinfo.state != PS_OPEN ) {
		    break;
		}
	    }
	}
	if( request == CB_NOT_WAIT ) {
	    pb->scs_msgbuf = scsbp;
	    Unlock_pb( pb )
	    return;
	}
    } else if(( scsbp = pb->scs_msgbuf )) {
	pb->scs_msgbuf = NULL;
	cbvte = NULL;
    } else {
	cb->cinfo.cbstate = request;
	Insert_entry( cb->scs_cb, pb->scs_cb )
	Unlock_pb( pb )
	return;
    }
    if( request <= CB_MAX_PEND ) {
	scsbp->mtype = format[ --request ].mtype;
    } else {
	( void )panic( SCSPANIC_SCSMSG );
    }
    if( format[ request ].flags.credits ) {
	scsbp->credit = cb->cinfo.pend_rec_credit;
	cb->cinfo.rec_credit += cb->cinfo.pend_rec_credit;
	cb->cinfo.pend_rec_credit = 0;
    } else {
	scsbp->credit = 0;
    }
    Move_connid( cb->cinfo.rconnid, scsbp->rconnid )
    Move_connid( cb->cinfo.lconnid, scsbp->sconnid )
    if(( Reject_req( scsbp )->reason = cb->cinfo.reason )) {
	cb->cinfo.reason = 0;
    }
    if( format[ request ].flags.conn_info ) {
	Conn_req(  scsbp )->min_credit = cb->cinfo.min_snd_credit;
	Move_name( cb->cinfo.rproc_name, Conn_req( scsbp )->rproc_name )
	Move_name( cb->cinfo.lproc_name, Conn_req( scsbp )->sproc_name )
	Move_name( cb->cinfo.lconn_data, Conn_req( scsbp )->sproc_data )
    } else {
	Conn_req( scsbp )->min_credit = 0;
    }
    if( pb->pinfo.status.sanity ) {
	( void )panic( SCSPANIC_SANITY );
    } else {
	Unlock_pb( pb )
	( void )( *cb->Send_msg )( cb->pccb,
				   pb,
				   scsbp,
				   format[ request ].mlength,
				   RECEIVE_BUF );
	Lock_pb( pb )
	if( pb->pinfo.state == PS_OPEN ) {
	    pb->pinfo.status.sanity = 1;
	    pb->pinfo.duetime = scs_sanity;
	    Insert_entry( pb->timeout, scs_timeoutq )
	    if( scs_timeoutq.flink == &scs_timeoutq ) {
		( void )timeout( scs_timer, NULL, 1 );
	    }
	}
    }
    Unlock_pb( pb )
    if( cbvte ) {
	Unlock_cbvte( cbvte )
    }
}

/*   Name:	scs_response	- Send SCS Sequenced Message Response
 *
 *   Abstract:	This routine initiates transmission of a SCS response over a
 *		specific path.  A SCS response is only transmitted following
 *		reception and processing of the corresponding SCS request.
 *		Five such responses are supported:
 *
 *		1. Response to a connection request
 *		2. Response to a connection acceptance request.
 *		3. Response to a connection rejection request.
 *		4. Response to a connection termination request.
 *		5. Response to a request for extension or withdrawal of send
 *		   credits.
 *
 *		SCS applies flow control to its own communication on a per path
 *		basis.  Only one SCS request is transmitted across each path at
 *		a given moment.  However, SCS responses may always be
 *		immediately transmitted because the message buffer containing
 *		the SCS request on the originating system is reserved for
 *		reception of its corresponding SCS response.
 *
 *		The message buffer containing the SCS response is always
 *		derived from recyclement of the message buffer containing the
 *		SCS request.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   scsbp			- Address of SCS header of message buffer
 *   pb				- Path Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The SCA database is locked( EXTERNALLY ) to guarantee the
 *		validity of PB addresses and to prevent PB deletion.
 *
 *		The PB is locked to synchronize access to the path's current
 *		path state.  It is unlocked immediately following retrieval of
 *		the current path state as required by PD functions/routines
 *		which both transmit messages and add them to appropriate local
 *		port free pools in these particular cases.
 */
void
scs_response( scsbp, pb )
    register SCSH	*scsbp;
    PB			*pb;
{
    CONNID		save_connid;
    register u_long	state;
    static u_short	rsp_lengths[] = {
	sizeof( CONN_RSP ),
	sizeof( ACCEPT_RSP ),
	sizeof( REJECT_RSP ),
	sizeof( DISCONN_RSP ),
	sizeof( CREDIT_RSP )
    };

    /* The SCS response is formated from the SCS request and transmitted only
     * if the path is currently open.  Otherwise the message buffer is added to
     * the appropriate local port's free message pool or deallocated depending
     * upon whether the path( but not the local port ) has failed.
     *
     * The PB is locked only for retrieval of the current path state because
     * the interfaces to both the message transmission and message addition
     * routines require it to be unlocked in this particular instance.
     * Unlocking the PB does not present a problem for the following reasons:
     *
     * 1. It is not particularly crucial if the path state changes between the
     *    time the PB is unlocked and the cached value is checked.  It is true
     *    this could result in an attempt to transmit a SCS response on a
     *	  failed path.  However, this possibility has always existed and there
     *	  is no way to eliminate it.
     *
     * 2. It is acceptable to continue to reference PB contents even though it
     *    the data structure is unlocked.  The cached PB address is valid
     *	  because the SCA database is currently locked preventing PB deletion,
     *    and the contents accessed are static.  This includes the path failure
     *	  reason which can not change once set for a given path incarnation and
     *	  is accessed only after the path has been found to have failed.
     *
     * NOTE: The SCS response message type is always 1 more then its
     *	     corresponding SCS request message type.
     *
     * NOTE: The sending and receiving connection identification numbers of the
     *	     SCS request are always the receiving and sending connection
     *	     identification numbers of the corresponding SCS response.
     */
    if( scsbp->mtype > SCS_MAX_SCSMSG ) {
	( void )panic( SCSPANIC_SCSMSG );
    }
    Lock_pb( pb )
    state = pb->pinfo.state;
    Unlock_pb( pb )
    if( state == PS_OPEN ) {
	++scsbp->mtype;
	if( scsbp->mtype != SCS_CREDIT_RSP ) {
	    scsbp->credit = 0;
	}
	Move_connid( scsbp->rconnid, save_connid )
	Move_connid( scsbp->sconnid, scsbp->rconnid )
	Move_connid( save_connid, scsbp->sconnid )
	Accept_req( scsbp )->min_credit = 0;
	( void )( *pb->Send_msg )( pb->pccb,
				   pb,
				   scsbp,
				   rsp_lengths[(( scsbp->mtype - 1 ) >>1 )],
				   RECEIVE_BUF );
    } else if( state == PS_PATH_FAILURE && Port_failure( pb->pinfo.reason )) {
	( void )( *pb->Dealloc_msg )( pb->pccb, scsbp );
    } else {
	( void )( *pb->Add_msg )( pb->pccb, scsbp );
    }
}

/*   Name:	scs_timer	- SCS Protocol Interval Timer
 *
 *   Abstract:	This routine oversees all SCS timer related activities.  It is
 *		executed once per second per system but only when SCS requires
 *		some timer related function.
 *
 *		Timer related functions currently performed include:
 *
 *		1. Declaring SCS protocol sequence time outs on open paths.
 *
 *   Inputs:
 *
 *   IPL_SOFTCLOCK		- Interrupt processor level
 *   lk_scadb			- SCA database lock structure
 *   scs_timeoutq		- SCS protocol sequence timeout queue head
 *
 *   Outputs:
 *
 *   IPL_SOFTCLOCK		- Interrupt processor level
 *
 *   SMP:	The SCA database is locked for traversal of the SCS protocol
 *		sequence timeout queue and for removing PBs from this queue
 *		when it becomes necessary.  It also postpones premature PB
 *		deletion.
 *
 *		PBs are locked to synchronize access and for removing them from
 *		the SCS protocol sequence timeout queue when it becomes
 *		necessary.  PB locks are released before crashing the
 *		corresponding path.
 */
void
scs_timer()
{
    register pbq	*qp;
    register PB		*pb;
    register u_long	save_ipl = Splscs();

    /* Each timer interval the following sequence of actions occurs:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The SCA database is locked.
     * 3. The SCS protocol sequence timeout queue of PBs is scanned and the
     *	  corresponding paths crashed on protocol timer expirations.  Each PB
     *	  is locked for the duration of its check.
     * 4. The next invocation of the SCS interval timer is scheduled provided
     *	  the SCS protocol sequence timeout queue is not empty following
     *	  processing.
     * 5. The SCA database is unlocked.
     * 6. IPL is restored.
     */
    Lock_scadb()
    for( qp = scs_timeoutq.flink; qp != &scs_timeoutq; ) {
	pb = Pos_to_pb( qp, timeout );
	qp = qp->flink;
	Lock_pb( pb )
	if( !pb->pinfo.status.sanity ) {
	    ( void )panic( SCSPANIC_SANITY );
	} else if( --pb->pinfo.duetime == 0 ) {
	    Remove_scs_timeoutq( pb )
	    ( void )( *pb->Crash_path )( pb->pccb, pb, E_TIMEOUT, 0, NULL );
	}
	Unlock_pb( pb )
    }
    if( scs_timeoutq.flink != &scs_timeoutq ) {
	( void )timeout( scs_timer, NULL, hz );
    }
    Unlock_scadb()
    ( void )splx( save_ipl );
}
