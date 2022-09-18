#ifndef	lint
static char *sccsid = "@(#)cippd_event.c	4.1	(ULTRIX)	7/2/90";
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
 *		Computer Interconnect Port-to-Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port-to-Port
 *		Driver( CI PPD ) event notification routines.  There are
 *		two types of such routines: routines utilized by port
 *		drivers to notify the CI PPD finite state machine of the
 *		occurrence of specific path related events and event driven
 *		routines which are never directly invoked by port drivers.
 *
 *   Creator:	Todd M. Katz	Creation Date:	October 20, 1985
 *
 *   Function/Routines:
 *
 *   Routines Invoked by Port Drivers
 *	cippd_receive		CI PPD Datagram/ID Packet Received
 *	cippd_reqid_snt		Identification Request Transmission Completed
 *	cippd_start		Start CI PPD Activity on Local Port
 *	cippd_stop		Stop CI PPD Activity on Local Port
 *   Routines NOT Invoked by Port Drivers
 *	cippd_poll		Poll Remote Ports for their Identification
 *	cippd_timer		CI PPD Interval Timer Routine
 *
 *   Modification History:
 *
 *   20-May-1989	Pete Keilty
 *	Changed splx(IPL_SCS) to new macro Splscs() and remove forkb.ipl.
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_locl.h
 *
 *   17-Jan-1989	Todd M. Katz		TMK0006
 *	1. The PCCB address is now part of the interface to Get_port.
 *	2. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	3. Allow sanity checks on local port functioning to be permanently
 *	   disabled on a per local port basis at the discretion of the
 *	   appropriate port driver.
 *	4. Modify the cippd_start() to initialize the formative path PB queue
 *	   listhead.  Formerly it was down within the individual port drivers.
 *	5. Include header file ../vaxmsi/msisysap.h.
 *
 *   23-Sep-1988	Todd M. Katz		TMK0005
 *	Modify cippd_receive() to verify the message type of received CI PPD
 *	datagrams.  Unknown CI PPD message types crash the paths over which the
 *	datagrams were received, or are logged and discarded if no path exists.
 *
 *   17-Aug-1988	Todd M. Katz		TMK0004
 *	1. Rename IE_ALLOCFAIL -> E_ALLOCFAIL( it is now an error event ) and
 *	   LPC_BROKEN -> SE_PPDSANITY( the local port crash severity modifier(
 *	   ESM_LPC ) is applied only by the individual port driver routines
 *	   responsible for crashing paths and only when the local port has not
 *	   yet already been crashed ).
 *	2. cippd_stop() no longer needs to map specific port failure reasons to
 *	   generic ones.  This mapping is now performed by the individual port
 *	   drivers and passed into the routine in place of the specific reason
 *	   for crashing the port.
 *	3. Modify cippd_start(), cippd_timer(), and cippd_poll() to obtain
 *	   port polling burst size and contact frequency from CI PPD specific
 *	   PCCB fields instead of from CI PPD configuration variables.
 *	4. Modify cippd_stop() to event log the failure of each formative and
 *	   established path associated with a failed local port.
 *	5. Modify cippd_start() and cippd_poll() to allocate and add to each
 *	   local port an initial number of free datagram buffers for the
 *	   reception of solicited remote port identifications and unsolicitated
 *	   START CI PPD datagrams.
 *	6. Refer to error logging as event logging.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *	1. Modify cippd_receive() to event log instances of insufficient memory
 *	   for new path establishment.
 *	2. When cippd_receive() is unable to allocate sufficient resources, it
 *	   aborts new path establishment and deallocates those resources it was
 *	   able to allocate.  Unfortunately, an attempt was being made to
 *	   always deallocate the datagram allocated specifically for use during
 *	   path establishment, even though this datagram might not have been
 *	   successfully allocated.  Resolve the problem with this error path.
 *	3. Modify cippd_stop() to no longer zero log maps( dbclogmap,
 *	   ptdlogmap ) during CI PPD specific PCCB clean up.
 *	4. CI PPD event logging is now split between the routines
 *	   cippd_log_path() and cippd_log_sys() to differentiate between
 *	   logging of path specific and system level events respectively.
 *	   Modify cippd_receive() to invoke cippd_log_path().
 *
 *   02-Jun-1988	Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   11-Apr-1988	Todd M. Katz		TMK0002
 *	Add use of Pb_fork() and Pccb_fork() macros in place of straight-line
 *	code.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made
 *	CI PPD and GVP completely independent from underlying port drivers,
 *	restructured code paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/systm.h"
#include		"../h/vmmac.h"
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
#include		"../io/gvp/gvp.h"
#include		"../io/ci/cippd.h"

/* External Variables and Routines.
 */
extern	int		hz;
extern	u_long		cippd_dispatch();
extern	PB		*cippd_get_pb(), *scs_alloc_pb();
extern	u_short		cippd_init_dgs, cippd_itime, cippd_max_port,
			cippd_penable;
extern	void		cippd_clean_fpb(), cippd_clean_pb(), cippd_crash_pb(),
			cippd_log_path(), cippd_poll(), cippd_timer(),
			scs_dealloc_pb();

/*   Name:	cippd_receive	- CI PPD Datagram/ID Packet Received
 *
 *   Abstract:	This CI PPD event notification routine is asynchronously
 *		invoked whenever a port driver receives one of the following ID
 *		packets or non-application CI PPD datagrams:
 *
 *		1. ID packet.
 *		2. START CI PPD datagram.
 *		3. STACK( Start Acknowledge ) CI PPD datagram.
 *		4. ACK CI PPD datagram.
 *		5. ERROR CI PPD datagram.
 *		6. STOP CI PPD datagram.
 *
 *		Its purpose is to process and dispose of the received packet.
 *
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum
 *		      CI PPD port numbers only occupy 1 byte, the low-order
 *		      byte of a port station address.  Port numbers are passed
 *		      as 4 bytes entities back and forth between the CI PPD and
 *		      its client port drivers.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_max_port		- CI PPD maximum port to recognize
 *   cippd_penable		- CI PPD port polling enable flag
 *   cippdbp			- Address CI PPD header in datagram/ID buffer
 *   pccb			- Port Command and Control Block pointer
 *   type			- Type of packet received
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	forkb			-  Fork block for path clean up
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus		-   Finite State Machine path status flags
 *		path_closed	-    Path already closed by port status bit
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.ppd.cippd	-  CI PPD specific local port information
 *	    nform_paths		-   Current number of formative paths
 *	    npaths		-   Current number of paths
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    aflogmap		-   Path establishment insufficient mem log map
 *	    elogopt.port_num	-   Remote port number
 *
 *   SMP:	The PCCB is locked to synchronize access, for retrieval of
 *		existing PBs, for insertion of new PBs onto the queue of
 *		formative PBs to postpone potential PB deletion, and as
 *		required by cippd_log_path() in case logging becomes necessary.
 *		PCCB addresses are always valid because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		The PB is locked to postpone potential deletion and for
 *		declaration of an event on the corresponding path appropriate
 *		to the packet received and the path's current state.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
cippd_receive( pccb, cippdbp, type )
    register PCCB	*pccb;
    GVPPPDH		*cippdbp;
    u_long		type;
{
    register PB		*pb;
    register SCSH	*scsbp;
    register u_long	( *alloc )(), port;
    PIB			cippd_pib;

    /* Lock the PCCB, process and dispose of the received packet, and unlock
     * the PCCB before returning.  Received packets are processed in one of the
     * following ways:
     *
     * 1. ID packets and CI PPD datagrams with known message types received
     *	  over existent paths are processed and disposed of by the CI PPD
     *	  finite state machine.
     * 2. Non-START CI PPD datagrams received over non-existent paths are
     *    returned to the appropriate local port datagram free queue.
     * 3. ID packets and START CI PPD datagrams received over unknown paths
     *	  initiate new path establishment and are processed and disposed of by
     *	  the CI PPD finite state machine.
     * 4. CI PPD datagrams with unknown message types crash the paths over
     *	  which they were received.  If no path exists then the invalid
     *	  datagrams are just logged before being discarded.
     *
     * The appropriate PB is always locked prior and unlocked after invocation
     * of the finite state machine.
     *
     * New path establishment requires the following actions prior to
     * invocation of the finite state machine:
     *
     * 1. A new PB is allocated.
     * 2. A datagram is allocated for use during path establishment and added
     *    to the appropriate port's datagram free queue.
     * 3. Any port driver specific buffers are allocated.
     * 4. The PB fork block reserved for clean up of the path is initialized.
     * 5. The appropriate PB CI PPD path state bits are initialized.
     *
     * Insufficient memory in Steps 1-3 aborts new path establishment.  Only
     * the first such failure to establish a specific path is logged.  Logging
     * of all subsequent failures is bypassed until after the CI PPD is
     * successful at allocating all required memory for establishment of the
     * specific path( see cippd_enab_path()).
     *
     * The allocated datagram( Step 2 ) is conceptually used for all protocol
     * exchanges during establishment of the new path.  It is only temporarily
     * required and is removed from the free queue and deallocated following
     * path establishment.
     *
     * START CI PPD datagrams and ID packets received over unknown paths may
     * also be returned to the appropriate port's datagram free queue instead
     * of initiating new path establishment when one the following conditions
     * exists:
     *
     * 1. Polling is not currently enabled.
     * 2. The station address of the sending CI PPD exceeds the maximum address
     *	  the local CI PPD is currently willing to converse with.
     * 3. A failure occurs during new path establishment initiation.
     */
    Lock_pccb( pccb )
    if(( pb = cippd_get_pb( pccb, Ppd_to_scs( cippdbp ), BUF )) == NULL ) {
	port = ( *pccb->Get_port )( pccb, cippdbp );
	if(( type == CNFE_START_REC || type == CNFE_ID_REC ) &&
	      cippd_penable				     &&
	      port <= Maxport( pccb )) {
	    ( void )bzero(( u_char * )&cippd_pib, sizeof( PIB ));
	    cippd_pib.lport_name = pccb->lpinfo.name;
	    Scaaddr_low( cippd_pib.rport_addr ) = port;
	    if(( pb = scs_alloc_pb( PS_CLOSED, pccb, &cippd_pib )) &&
		 ( scsbp = ( *pccb->Alloc_dg )( pccb ))		   &&
		 (( alloc = pccb->Alloc_buf )
			? ( *alloc )( pccb, pb )
			: RET_SUCCESS ) == RET_SUCCESS ) {
		( void )( *pccb->Add_dg )( pccb, scsbp );
		pb->Fsmpstatus.path_closed = 1;
		Insert_entry( pb->flink, pccb->Form_pb )
		++pccb->lpinfo.Nform_paths;
	    } else {
		if( pb ) {
		    if( scsbp ) {
			( void )( *pccb->Dealloc_dg )( pccb, scsbp );
		    }
		    ( void )scs_dealloc_pb( pccb, pb );
		    pb = NULL;
		}
		if( !Test_map( Aflogmap, port )){
		    Set_map( Aflogmap, port )
		    pccb->Elogopt.port_num = port;
		    ( void )cippd_log_path( pccb, NULL, NULL, E_ALLOCFAIL );
		}
	    }
	}
    }
    if( pb ) {
	Lock_pb( pb )
	if( type <= CNFE_STOP_REC && type != CNFE_SCSMSG_REC ) {
	    ( void )cippd_dispatch( pccb, pb, type, cippdbp );
	} else {
	    ( void )cippd_crash_pb( pccb,
				    pb,
				    SE_BADPPDMTYPE,
				    RECEIVE_BUF,
				    Ppd_to_scs( cippdbp ));
	}
	Unlock_pb( pb )
    } else {
	if( type > CNFE_STOP_REC || type == CNFE_SCSMSG_REC ) {
	    pccb->Elogopt.port_num = port;
	    ( void )cippd_log_path( pccb, NULL, cippdbp, RE_BADPPDMTYPE );
	}
	( void )( *pccb->Add_dg )( pccb, Ppd_to_scs( cippdbp ));
    }
    Unlock_pccb( pccb )
}

/*   Name:	cippd_reqid_snt	- Identification Request Transmission Completed
 *
 *   Abstract:	This CI PPD event notification routine is asynchronously
 *		invoked by port drivers following transmission of each
 *		identification request.  This occurs regardless of whether the
 *		request had been initiated during polling or by the CI PPD
 *		finite state machine during path establishment.  Its purpose is
 *		to determine whether the sanity check initiated during the
 *		port's last polling interval has been satisfied.
 *
 *		The local port sanity check crudely verifies the operational
 *		status of the local port.  During the last polling interval the
 *		port number of the first remote port polled is saved.
 *		Successful transmission of the request for this remote port's
 *		identification must complete before the next timer interval or
 *		the port is assumed to be broken and is crashed.
 *
 *		NOTE: Some port drivers do NOT require sanity checks to be made
 *		      on their local ports.  Such drivers set the
 *		      "nosanity_chk" finite state machine status bit in their
 *		      PCCBs as an appropriate indicator to the CI PPD.  They
 *		      also NEVER bother to invoke this function following
 *		      transmission of an identification request.
 *
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum
 *		      CI PPD port numbers only occupy 1 byte, the low-order
 *		      byte of a port station address.  Port numbers are passed
 *		      as 4 bytes entities back and forth between the CI PPD and
 *		      its client port drivers.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus		-   Finite State Machine status flags
 *		nosanity_chk	-    Skip sanity checking flag 
 *   port			- Port number of completed request
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.sanity	-   Port polling ( sanity check ) in progress
 *
 *   SMP:	The PCCB is locked to synchronize access.  PCCB addresses are
 *		always valid because these data structures are never deleted
 *		once their corresponding ports have been initialized.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
cippd_reqid_snt( pccb, port )
    PCCB		*pccb;
    u_long		port;
{
    Lock_pccb( pccb )
    if( pccb->Fsmstatus.sanity && pccb->Sanity_port == port ) {
	pccb->Fsmstatus.sanity = 0;
    }
    Unlock_pccb( pccb )
}

/*   Name:	cippd_start	- Start CI PPD Activity on Local Port
 *
 *   Abstract:	This CI PPD event notification routine is asynchronously
 *		invoked following local port initialization.  It engages the CI
 *		PPD finite state machine on the port by activating the CI PPD
 *		interval timer routine.  Once engaged this machine
 *		automatically polls for remote systems and attempts to
 *		establish CI PPD paths to those it discovers.
 *
 *		There are two situations when this routine is invoked by a port
 *		driver and they are identically handled.
 *
 *		1. During auto-configuration at system boot time following
 *		   successful initialization of a local port.
 *		2. Following the failure and successful re-initialization
 *		   of a local port.
 *
 *		NOTE: Some port drivers NEVER require sanity checks to be made
 *		      on their local ports.  Such drivers set the
 *		      "nosanity_chk" finite state machine status bit in their
 *		      PCCBs as an appropriate indicator prior to engaging the
 *		      CI PPD on their local ports.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_max_port		- CI PPD maximum port to recognize
 *   cippd_init_dgs		- Number of CI PPD path establishment datagrams
 *   cippd_itime		- CI PPD port timer interval
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    burst		-   Port polling burst size
 *	    contact		-   Port polling contact frequency
 *	    fsmstatus		-   Finite State Machine status flags
 *		nosanity_chk	-    Skip sanity checking flag 
 *	    	online		-    0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.ppd.cippd	-  CI PPD specific local port information
 *	    protocol		-   CI PPD protocol version level
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    form_pb		-   Formative PB queue( INITIALIZED )
 *	    fsmstatus.online	-   1
 *	    fsmstatus.timer	-   1
 *	    next_port		-   Next port to poll
 *	    poll_cable		-   CI cable to use for polling
 *	    poll_due		-   Port polling interval timer
 *	    poll_interval	-   Current port polling interval
 *	    ppddgs		-   Num path establishment dgs left to allocate
 *	    timer_interval	-   Current CI timer interval
 *
 *   SMP:	The PCCB is locked to synchronize access.  This is probably
 *		unnecessary because of lack of conflict for the PCCB due to the
 *		single threadedness of port clean up and initialization.  It is
 *		done anyway to guarantee unrestricted access and because the CI
 *		PPD interval timer may still be active.  PCCB addresses are
 *		always valid because these data structures are never deleted
 *		once their corresponding ports have been initialized.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
cippd_start( pccb )
    register PCCB	*pccb;
{
    /* The steps involved on starting the CI PPD on a local port are:
     *
     * 1. Lock the PCCB.
     * 2. Initialize the CI PPD specific PCCB fields controlling polling and
     *    specifying the number of CI PPD datagrams left to allocate and add to
     *    the specified local port datagram free queue.
     * 3. Schedule the first invocation of the CI PPD interval timer on the
     *    local port.
     * 4. Unlock the PCCB.
     *
     * CI PPD datagrams added to the specified local port datagram free queue(
     * Step 2 ) are used for reception of solicited remote port identifications
     * and unsolicited START CI PPD datagrams.  Actual buffer allocations are
     * performed prior to the initial polling for remote ports from the
     * specified local port( by cippd_poll()).
     *
     * Scheduling the first invocation of the CI PPD interval timer on a local
     * port is only necessary following the initial initialization of the port(
     * Step 3 ).  Once activated the CI PPD interval timer is self maintaining
     * and remains permanently active on the local port.
     */
    Lock_pccb( pccb )
    pccb->Fsmstatus.online = 1;
    Init_queue( pccb->Form_pb )
    pccb->Poll_due = 0;
    pccb->Next_port = 0;
    pccb->Poll_cable = FIRST_CABLE;
    pccb->Poll_interval = Pinterval( pccb );
    pccb->Ppddgs = cippd_init_dgs;
    if( !pccb->Fsmstatus.timer ) {
	pccb->Fsmstatus.timer = 1;
	pccb->Timer_interval = cippd_itime;
	( void )timeout( cippd_timer, ( u_char * )pccb, 0 );
    }
    Unlock_pccb( pccb )
}

/*   Name:	cippd_stop	- Stop CI PPD Activity on Local Port
 *
 *   Abstract:	This CI PPD event notification routine is asynchronously
 *		invoked following disablement of a local port.  It directs the
 *		clean up the CI PPD portion of the failed port primarily by
 *		directing the clean up all paths associated with the port, both
 *		formative and established.
 *
 *		Port re-initialization is triggered in one of two ways.  It is
 *		immediately initiated by this routine if there are no paths
 *		associated with the failed port.  Otherwise, clean up the last
 *		associated path, formative or established, triggers it.
 *
 *		NOTE: The appropriate port drivers are always responsible for
 *		      the removal and deallocation of all free datagram and
 *		      message buffers associated with failed local ports.  The
 *		      CI PPD must never attempt to dispose of such buffers.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.cleanup	-   1
 *	    fsmstatus.fkip	-   0
 *	    fsmstatus.online	-   0
 *   reason			- Generic reason why the local port was crashed
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	forkb			-  PCCB fork block
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.fkip	-   Fork operation in progress
 *	    fsmstatus.sanity	-   0
 *
 *   SMP:	The PCCB is locked to synchronize access and as required by
 *		cippd_log_path() in case event logging becomes necessary.  PCCB
 *		locking is probably unnecessary because of lack of conflict for
 *		the PCCB due to single threading of port clean up and
 *		re-initialization.  It is done anyway to guarantee unrestricted
 *		access and because the CI PPD interval timer may still be
 *		active.  Locking the PCCB also prevents premature PB deletion.
 *		PCCB addresses are always valid because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		PBs are locked to synchronize access and prevent premature
 *		deletion.
 *
 *		PB semaphores are incremented prior to scheduling asynchronous
 *		clean up of both formative and established paths.  Incrementing
 *		the semaphore should guarantee PB validity when path clean up
 *		commences.  Actually, single threading of clean up should 
 *		sufficient for this purpose, but the PB semaphore is
 *		incremented anyway as further protection and to detect errors
 *		in error recovery logic.
 *
 *		NO EXTERNAL locks may be held when this routine is invoked.
 */
void
cippd_stop( pccb, reason )
    register PCCB	*pccb;
    u_long		reason;
{
    register pbq	*pb, *next_pb;
    register SB		*sb;
    register long	i;

    /* The steps involved in stopping CI PPD activity on a local port include:
     *
     * 1. Locking the PCCB.
     * 2. Cleaning up the CI PPD specific portion of the PCCB.
     * 3. Failing all formative paths associated with the local port.
     * 4. Failing all established paths associated with the local port.
     * 5. Unlocking the PCCB.
     *
     *
     * The failure of each formative path( Step 3 ) and each established path(
     * Step 4 ) is event logged.  Local port re-initialization follows clean up
     * of the very last path associated with the failed port.  It is initiated
     * directly by this routine if no paths are associated with the port at
     * time of failure.
     *
     * Failing paths associated with operational ports is a complex process.
     * It requires CI PPD finite state machine intervention to pre-process,
     * disable, and direct path clean up.  Failing paths associated with failed
     * ports is not as complicated.  No crash pre-processing must be done and
     * the port failure itself automatically disabled all associated paths.
     * Only path clean up is required and may be accomplished without CI PPD
     * intervention.  However, synchronization to the path state interlock is
     * still required before path clean up commences.  This maintains the
     * single threading necessary to prevent both multiple crashings of the
     * same path incarnation and multiple attempts to clean up the same path.
     * Such a need exists only in SMP environments where multiple CI PPD
     * threads may exist and interact.
     *
     * Synchronization to the path state interlock as a prelude to path clean
     * up proceeds as follows:
     *
     *		The state of each path is checked.  Another CI PPD thread is
     *		assumed to have crashed the path and taken responsibility for
     *		clean up if path state == PS_PATH_FAILURE.  Otherwise, the
     *		current thread assumes responsibility for cleaning up the path
     *		by transitioning its state to PS_PATH_FAILURE.
     *
     * When this routine assumes responsibility for cleaning up a path, it
     * increments the appropriate PB semaphore and schedules asynchronous path
     * clean up through forking.  Different routines are employed to clean up
     * formative and established paths.  Using the semaphore in this fashion
     * should guarantee PB validity when path clean up commences.  Actually,
     * single threading of clean up should be sufficient for this purpose, but
     * the semaphore is incremented anyway as further protection, and to detect
     * errors in error recovery logic.
     *
     * NOTE: The PB fork block is only used for path clean up.  Therefore, it
     *	     should always be available because path clean up is single
     *	     threaded.
     *
     * NOTE: The PCCB fork block is only used for clean up and initialization
     *	     of the local port.  Therefore, it should always be available
     *	     because port clean up and initialization are single threaded.
     */
    Lock_pccb( pccb )
    pccb->Fsmstatus.sanity = 0;
    if( pccb->lpinfo.Npaths == 0 && pccb->lpinfo.Nform_paths == 0 ) {
	Pccb_fork( pccb, pccb->Init_port, PPDPANIC_PCCBFB )
    } else {
	for( pb = pccb->Form_pb.flink, next_pb = Pb->flink;
	     pb != &pccb->Form_pb;
	     pb = next_pb, next_pb = Pb->flink ) {
	    Lock_pb( Pb )
	    if( Pb->pinfo.state != PS_PATH_FAILURE ) {
		if( Pb->pinfo.state != PS_OPEN ) {
		    Pb->pinfo.reason = reason;
		    ( void )cippd_log_path( pccb, Pb, NULL, W_ABORT_PATH );
		    Pb->pinfo.state = PS_PATH_FAILURE;
		    Incr_pb_sem( Pb )
		    Pb_fork( Pb, cippd_clean_fpb, PPDPANIC_PBFB )
		} else {
		    ( void )panic( PPDPANIC_PSTATE );
		}
	    }
	    Unlock_pb( Pb )
	}
	for( i = ( CIPPD_MAXPATHS - 1 ); i >= 0; --i ) {
	    if(( pb = pccb->Open_pb[ i ])) {
		Lock_pb( Pb )
		if( Pb->pinfo.state == PS_OPEN ) {
		    Pb->pinfo.reason = reason;
		    ( void )cippd_log_path( pccb, Pb, NULL, W_FAIL_PATH );
		    Pb->pinfo.state = PS_PATH_FAILURE;
		    Incr_pb_sem( Pb )
		    Pb_fork( Pb, cippd_clean_pb, PPDPANIC_PBFB )
		} else if( Pb->pinfo.state != PS_PATH_FAILURE ) {
		    ( void )panic( PPDPANIC_PSTATE );
		}
		Unlock_pb( Pb )
	    }
	}
    }
    Unlock_pccb( pccb )
}

/*   Name:	cippd_poll	- Poll Remote Ports for their Identification
 *
 *   Abstract:	This routine polls all remote ports, both known and unknown,
 *		from a specific local port.  Through this process new systems
 *		are discovered and the state of CI PPD paths to known systems
 *		verified.  The polling process is itself sanity checked to
 *		verify the operational status of the local port.
 *
 *		Port Polling is accomplished by requesting remote port
 *		identifications.  Each time this routine is invoked the
 *		identification of a subset of all potentially existing remote
 *		ports accessible from the local port is requested.  It takes
 *		may take several routine invocations to sweep through all
 *		potentially accessible remote ports.
 *
 *		The number of ports polled during each invocation is a function
 *		of the maximum number of ports to poll, the polling burst size,
 *		and the next port to poll within the port's current sweep.  The
 *		first factor is governed by a dynamically adjustable parameter
 *		as is the second at the discretion of individual port drivers.
 *
 *		The state of established CI PPD paths is determined during each
 *		sweep by transmitted all identification requests over a
 *		specific physical cable.  This allows the state of this
 *		component of an established CI PPD path to be verified.  When
 *		the current sweep completes a new cable is chosen for the next
 *		sweep.  
 *
 *		This routine is also responsible for allocating and adding to
 *		the appropriate local port datagram free queue an initial
 *		number of free datagram buffers.  These buffers are used for
 *		the reception of remote port identifications solicited both by
 *		port polling and by the CI PPD finite state machine.  They are
 *		also used for reception of unsolicited START CI PPD datagrams
 *		from various remote systems.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_max_port		- CI PPD maximum port number to recognize
 *   cippd_penable		- CI PPD port polling enable flag
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus		-   Finite State Machine status flags
 *		nosanity_chk	-    Skip sanity checking flag 
 *		online		-    1
 *	    ppddgs		-   Num path establishment dgs left to allocate
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.sanity	-   Port polling ( sanity check ) in progress
 *	    next_port		-   Next port to poll
 *	    poll_cable		-   CI cable to use for polling
 *	    ppddgs		-   Num path establishment dgs left to allocate
 *	    sanity_port		-   Sanity check remote port
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access and to
 *		postpone crashing of the local port if it fails while polling
 *		is in progress.
 */
void
cippd_poll( pccb )
    register PCCB	*pccb;
{
    register SCSH	*scsbp;
    register u_long	nports, max_port, ( *test )();;

    /* Execute the following steps when port polling is currently enabled:
     *
     * 1. Allocate and add to the specific local port datagram free queue any
     *	  CI PPD path establishment datagrams remaining to be allocated.
     * 2. Continue the current polling sweep by requesting the identification
     *	  of all remote ports to be polled during the current interval.
     * 3. Enable a sanity check on local port functioning.
     * 4. Optionally test the current connectivity status of the specified
     *	  local port in a port specific fashion.
     * 5. Arrange for the next sweep during the next polling interval.
     *
     * CI PPD path establishment datagrams are allocated and added to
     * appropriate local port datagram free queues by this routine( Step 1 )
     * contrary to normal expectations.  Logically, one would expect
     * cippd_start(), the routine responsible for starting CI PPD activity on
     * a specified local port, to allocate and add them.  It does not because
     * such allocations can fail, and such failures within cippd_start() create
     * unnecessairy complications.  It is much easier and more robust to
     * perform the allocations within this routine, ancillary to port polling.
     * Individual datagram allocation attempts can still fail; however, such
     * failures can be ignored because many more opportunities exist to
     * allocate all required CI PPD path establishment datagrams.
     *
     * The sanity check on local port functioning is enabled( Step 3 ) only if
     * it has not been permanently disabled on the local port and at least one
     * port was polled during the current polling interval.  This check
     * operates by requiring the request for identification of the very first
     * remote port polled to be transmitted before the next timer interval.
     * Failure to do so leads the CI PPD to assume the existence of a
     * questionable local port functional status and the local port is
     * explicitly crashed by the CI PPD.
     *
     * The current connectivity status of the specified local port is
     * optionally tested in a port specific fashion( Step 4 ) provided at least
     * one port was polled during the current polling interval.  Such testing
     * MUST occur prior to making arrangements for the next sweep during the
     * next polling interval.  Connectivity testing is bypassed whenever the
     * appropriate port driver has failed to establish a routine for testing
     * local port connectivity.  Presumably it deems such testing of its local
     * ports unnecessary.
     *
     * Arrangements are made for the next sweep during the next polling
     * interval( Step 5 ) only when the current sweep completes.  This of
     * course also requires at least one port to be polled during the current
     * polling interval.  Arrangements for the next polling interval are not
     * made by this routine but external to it by cippd_timer().
     */
    if( cippd_penable ) {
	while( pccb->Ppddgs ) {
	    if(( scsbp = ( *pccb->Alloc_dg )( pccb ))) {
		( void )( *pccb->Add_dg )( pccb, scsbp );
		--pccb->Ppddgs;
	    } else {
		break;
	    }
	}
	pccb->Sanity_port = pccb->Next_port;
	for( nports = pccb->Burst, max_port = Maxport( pccb );
	     nports > 0 && pccb->Next_port <= max_port;
	     --nports, ++pccb->Next_port ) {
	    if(( *pccb->Send_reqid )( pccb,
				      NULL,
				      ( u_long )pccb->Next_port,
				      DEALLOC_BUF ) == RET_ALLOCFAIL ) {
		break;
	    }
	}
	if( nports < pccb->Burst ) {
	    if( !pccb->Fsmstatus.nosanity_chk ) {
		pccb->Fsmstatus.sanity = 1;
	    }
	    if( pccb->Next_port > max_port ) {
		if(( test = pccb->Test_lportconn )) {
		    ( void )( *test )( pccb );
		}
		pccb->Next_port = 0;
		if( ++pccb->Poll_cable > pccb->Max_cables ) {
		    pccb->Poll_cable = FIRST_CABLE;
		}
	    }
	}
    }
}

/*   Name:	cippd_timer	- CI PPD Interval Timer Routine
 *
 *   Abstract:	This routine oversees all CI PPD timer related functions.  It
 *		is executed on a per port basis once every cippd_itime seconds
 *		where cippd_itime is a dynamically adjustable parameter.
 *
 *   Inputs:
 *
 *   IPL_SOFTCLOCK		- Interrupt processor level
 *   cippd_max_port		- CI PPD maximum port to recognize
 *   cippd_itime		- CI PPD port timer interval
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SOFTCLOCK		- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.timer	-   CI PPD interval timer activated status bit
 *	    poll_due		-   Port polling interval timer
 *	    poll_interval	-   Current port polling interval
 *	    timer_interval	-   Current CI timer interval
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    due_time		-   CI PPD traffic interval timer
 *	    fsmpstatus.timer	-   CI PPD traffic interval timer activated bit
 *
 *   SMP:	The PCCB is locked to synchronize access and as required by
 *		optional PD routines which notify local ports of continued CI
 *		PPD activity.  PCCB addresses are always valid because these
 *		data structures are never deleted once their corresponding
 *		ports have been initialized.
 *
 *		PBs are locked to synchronize access and for the declaration
 *		of CI PPD timeout configuration events when their traffic
 *		interval timers have expired.
 */
void
cippd_timer( pccb )
    register PCCB	*pccb;
{
    register pbq	*pb, *next_pb;
    register u_long	save_ipl = Splscs(), ( *notify )();

    /* Each timer interval the following sequence of actions occurs:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The PCCB is locked.
     * 3. The local port is optionally notified, in a port specific fashion, of
     *	  the continued activity of the CI PPD.
     * 4. A sanity check on port functioning is performed.
     * 5. The port's queue of formative PBs is scanned and CI PPD timeout
     *	  configuration events declared on each path whose traffic interval
     *    timer has expired.  Each PB is locked for the duration of its check.
     * 6. The next round of port polling from the port is initiated provided
     *	  the polling interval timer has expired.
     * 7. The next timer wake up on the local port is scheduled.
     * 8. The PCCB is unlocked.
     * 9. IPL is restored.
     *
     * Steps 3-6 are immediately bypassed whenever the local port is found to
     * not be currently online and functional.
     *
     * A sanity check on local port functioning( Step 4 ) consists of verifying
     * transmission of an identification request to the first remote port
     * polled during each polling interval.  The CI PPD assumes the port is
     * broken, crashes the port, and aborts the remaining timer actions( Steps
     * 5-6 ) if the targeted request is not successfully transmitted prior to
     * expiration of the next timer interval.  Sanity checking may be
     * permanently disabled on a per local port basis at the direction of the
     * appropriate port driver.
     *
     * NOTE:	Port polling intervals( frequencies ) are computed as follows:
     *
     *		            ( polling burst size )
     *		Interval =  ---------------------- * ( port contact frequency )
     *			     ( number of ports )
     *
     * Number of ports a dynamically adjustable parameter.  Port contact
     * frequency and Polling burst size vary on a per port basis( actually on
     * an individual port driver basis ) and are also dynamically adjustable
     * parameters at the discretion of individual port drivers.
     *
     * CI PPD interval timers are permanently active and completely self
     * maintaining.  Therefore, the next timer wake up on the local port is
     * scheduled regardless of whether the port is currently on or off line(
     * Step 7 ).  The only time the next wake up is not scheduled is when the
     * port is permanently broken and the appropriate port driver has taken it
     * permanently offline.
     */
    Lock_pccb( pccb )
    if( pccb->Fsmstatus.online ) {
	if(( notify = pccb->Notify_port )) {
	    ( void )( *notify )( pccb );
	}
	if( pccb->Fsmstatus.sanity ) {
	    ( void )( *pccb->Crash_lport )( pccb, SE_PPDSANITY, NULL );
	} else {
	    for( pb = pccb->Form_pb.flink, next_pb = pb->flink;
		 pb != &pccb->Form_pb;
		 pb = next_pb, next_pb = pb->flink ) {
		Lock_pb( Pb )
		if( Pb->Fsmpstatus.timer &&
		     ( Pb->Due_time -= pccb->Timer_interval ) < 0 ) {
		    Pb->Fsmpstatus.timer = 0;
		    ( void )cippd_dispatch( pccb, Pb, CNFE_TIMEOUT, NULL );
		}
		Unlock_pb( Pb )
	    }
	    if(( pccb->Poll_due -= pccb->Timer_interval ) < 0 ) {
		( void )cippd_poll( pccb );
		pccb->Poll_due = Pinterval( pccb );
		pccb->Poll_interval = pccb->Poll_due;
	    }
	}
    }
    if( !pccb->Fsmstatus.broken ) {
	pccb->Timer_interval = cippd_itime;
	( void )timeout( cippd_timer, ( u_char * )pccb, ( hz * cippd_itime ));
    } else {
	pccb->Fsmstatus.timer = 0;
    }
    Unlock_pccb( pccb )
    ( void )splx( save_ipl );
}
