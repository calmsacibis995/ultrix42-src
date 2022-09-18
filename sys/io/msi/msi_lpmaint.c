#ifndef	lint
static char *sccsid = "@(#)msi_lpmaint.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989                              *
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
 *		Mayfair Storage Interconnect Port Driver
 *
 *   Abstract:	This module contains Mayfair Storage Interconnect Port
 *		Driver( MSI ) local port maintenance service functions.
 *
 *   Creator:   Todd M. Katz    Creation Date:  December 17, 1988
 *
 *   Function/Routines:
 *
 *   msi_crash_lport		Crash Local MSI Port
 *   msi_remote_rst		Reset Remote Port and System
 *   msi_remote_strt		Start Remote Port and System
 *   msi_shutdown		Inform Known Systems of Local Shutdown
 *
 *   Modification History:
 *
 *   14-Jun-1989	Pete Keilty
 *	Add include file smp_lock.h
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/time.h"
#include		"../h/kmalloc.h"
#include		"../h/ksched.h"
#include		"../h/errlog.h"
#include		"../h/smp_lock.h"
#ifdef mips
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#endif
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
#include		"../io/ci/cippd.h"
#include		"../io/msi/msiport.h"

/* External Variables and Routines.
 */
extern	PB		*cippd_get_pb();
extern	u_short		msi_lpc_panic;
extern	MSIB		*msi_alloc_pkt();
extern	void		msi_clean_port(),
			msi_dealloc_pkt(),
			msi_disable(),
			msi_log_devattn(),
			msi_log_packet(),
			msi_xfp();

/*   Name:	msi_crash_lport	- Crash MSI Local Port
 *
 *   Abstract:	This routine crashes a specific local MSI port and initiates
 *		the process by which it is disabled, cleaned up, and
 *		re-initialized.  It may be sequentially invoked any number of
 *		times to crash a specific port; however, only its first
 *		invocation actually results in crashing of the port.  All
 *		subsequent invocations only log crash requests.  This situation
 *		persists until after the next incarnation of the port begins
 *		following completion of port clean up and re-initialization.
 *
 *		NOTE: This routine may NEVER be invoked with the address of the
 *		      SCS header in an optional MSIB buffer.  The CI PPD
 *		      currently never makes use of this routine parameter and
 *		      the MSI port driver caches the address, size, and remote
 *		      port station address of any optional MSI packet it wishes
 *		      logged.
 *
 *		NOTE: All error recovery is bypassed in favor of immediately
 *		      panicing the system whenever the MSI configuration
 *		      variable flag msi_lpc_panic is set.  Normally, this flag
 *		      is 0 and error recovery is the default action taken.  It
 *		      is set only when special debugging activity is required.
 *
 *		NOTE: The MSI port driver NEVER requires sanity checks to be
 *		      made on its local ports.  It always initializes the PCCB
 *		      specific finite state machine status bit "nosanity_chk"
 *		      so that the CI PPD skips such checks.  Therefore, the CI
 *		      PPD must NEVER to invoke this routine to crash a local
 *		      MSI port because of a failed sanity check( reason ==
 *		      SE_PPDSANITY ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   msi_lpc_panic		- MSI local port crash panic flag
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpcinfo		-   Optional local port crash information
 *		pkth		-    Address of MSI packet
 *		pktsize		-    Size of msi packet
 *		pport_addr	-    Packet remote port station address
 *	    lpstatus.optlpcinfo -   Optional local port crash information flag
 *	    save_dssr		-   Cached DSSI status register contents
 *	    save_dstat		-   Cached data transfer status reg contents
 *   reason			- Reason for crashing local MSI port
 *   scsbp			- NULL !!!!
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	forkb			-  PCCB fork block
 *	lpinfo.reason		-  Reason for port failure
 *	pd.msi			-  MSI specific PCCB fields
 *	    errlogopt.portnum	-   Remote port station address
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.cleanup	-   1
 *	    fsmstatus.fkip	-   1
 *	    fsmstatus.online	-   0
 *
 *   SMP:	The PCCB is locked INTERNALLY whenever it was not locked
 *		EXTERNALLY prior to routine invocation.  Locking the PCCB
 *		allows exclusive access to PCCB contents, prevents PB deletion,
 *		and is required by both msi_log_packet() and msi_log_devattn(),
 *		the routines responsible for logging local port crash requests.
 *		PCCB addresses are always valid because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *   		The PCCB specific XFP and RFP are locked INTERNALLY whenever
 *		they were not locked EXTERNALLY prior to routine invocation.
 *		Their locking is required by msi_disable(), the routine
 *		responsible for disabling the local MSI port.
 *
 *		The PCCB specific XFP and RFP are not only locked but the
 *		respective fork processes are synchronized to as well.  This
 *		synchronization is necessary because occassions exist when
 *		active fork processes must protect against crashing of the
 *		local port while temporarily releasing their locks.
 *		Synchronization to the fork processes by means of their
 *		in-progress SIIBUF pointers is the mechanism chosen to provide
 *		this protection.
 */
void
msi_crash_lport( pccb, reason, scsbp )
    register PCCB	*pccb;
    register u_long	reason;
    SCSH		*scsbp;
{
    register PB		*pb;
    u_long		unlock_pccb,
			unlock_rfp,
			unlock_xfp;

    /* Crashing a local MSI port consists of a number of distinct steps:
     *
     * 1. The crash request is pre-processed.
     * 2. The local port is disabled.
     * 3. The local port is cleaned up.
     * 4. The local port is re-initialized.
     *
     * The first step is executed each time this routine is invoked.  It is the
     * latter three steps which constitute crashing the port and are executed
     * only once per port incarnation.  The major benefits of this single 
     * threading of port crashing are the assumptions made during port clean up
     * and re-initialization which follow directly from it.  These assumptions
     * greatly reduce code complexity and allow extension of the MSI port
     * driver to an SMP environment with a minimum of pain.  They are pointed
     * out as they occur.
     *
     * Pre-processing of crash requests includes:
     *
     * 1. Locking the PCCB whenever it was not locked EXTERNALLY.
     * 2. Retrieving the appropriate PB whenever an optional MSI packet was
     *	  cached within the PCCB.
     * 3. Applying the local port crash severity modifier( ESM_LPC ) to the
     *    local port crash reason code.
     * 4. Logging the crash request according to the provided crash reason.
     * 5. Optionally panicing the system.
     *
     * The presence of an optional MSI packet is indicated by the status of the
     * "optlpcinfo" local port status flag.  This flag is set when optional
     * local port crash information is cached within the PCCB, cleared
     * otherwise.
     *
     * The local port crash severity modifier is applied( Step 3 ) ONLY if the
     * local port is not already in the process of being crashed.  
     *
     * Panicing of the system( Step 5 ) immediately terminates all further
     * processing of the crash request.  Such panicing is OPTIONAL.  It is
     * requested only when the MSI configuration variable flag msi_lpc_panic is
     * set.  This flag is set only when special debugging activity is required.
     *
     * Following pre-processing this routine ascertains whether or not the port
     * has already been crashed but not yet re-initialized.  If this is indeed
     * the case, as indicated by a PCCB clean up in progress status flag, then
     * the current request is dismissed after the PCCB is unlocked( provided it
     * was unlocked within this routine ).  Otherwise, actual crashing of the
     * port commences with disablement of the local port including:
     *
     * 1. Setting the port clean up in progress flag to prohibit additional
     *    crashings of the port until it has been re-initialized.
     * 2. Taking the port offline.
     * 3. Locking the PCCB specific RFP and XFP.
     * 4. Synchronizing to any active Transmit and Receive Fork Processes.
     * 5. Flushing the SIIBUF transmit-in-progress circular queue by
     *	  deallocating all associated MSIB packets.
     * 6. Resetting the SII chip.
     * 7. Marking the local MSI port inactive.
     * 8. Unlocking the PCCB specific RFP and XFP.
     *
     * The first five steps of local port disablement( Steps 1-5 ) and Step 8
     * are executed by this routine.  The remaining two steps( Steps 6-7 ) are
     * accomplished through invocation of msi_disable(), the routine
     * responsible for disabling local MSI ports.
     *
     * Fork process synchronization( Step 4 ) is accomplished by means of the
     * in-progress SIIBUF pointers.  These pointers are never NULL except
     * during those rare circumstances when active fork processes must
     * temporarily release their locks.  Therefore, the synchronization
     * protocol just consists of releasing and re-obtaining all locks( PCCB,
     * PCCB specific XFP and RFP ) until such time as both in-progress pointers
     * are tested to be non-NULL.  The reason why such synchronization is
     * necessary is discussed at greater length below.
     *
     * Flushing the transmit-in-progress circular queue( Step 5 ) does not
     * disturb the SIIBUFs in the queue.  What occurs is deallocation of the
     * MSI packets associated with each SIIBUF in the queue.  This allows
     * recovery of their dynamically allocated memory.  Logically this action
     * should occur during local port clean up.  It MUST be done at this time
     * during the port disablement step because once the SII chip is reset(
     * Step 5 ) the SII RAM buffer may NOT be accessed.  It is perfectly safe
     * to flush the transmit-in-progress circular queue at this time even
     * though it is not the logical moment to do so.  This is because once the
     * local MSI port has been marked inactive( Step 7 ) no further attempt is
     * made either to access the elements on this queue or to add new ones.
     *
     * Resetting the SII chip( Step 6 ) stops any operation in progress,
     * disconnects the chip from the DSSI bus, returns all registers to their
     * default values, and prevents all access to the SII RAM buffer.  Marking
     * the local MSI port inactive( Step 7 ) disables all future processing by
     * scheduled XFP, RFP and XFP_TIMER threads.  The end result of both these
     * steps is to drive the local MSI port into a completely quiescent state
     * allowing for full clean up and eventual local port re-initialization.
     *
     * Synchronization to active fork processes( Step 4 ) is important in a
     * SMP environment for the following reason.  Occassions exist when an
     * active fork process must temporarily release all locks including its
     * specific fork process lock( RFP or XFP ).  Its expectations are to
     * eventually re-obtain these locks and continue with its processing.  In a
     * SMP environment lacking fork process synchronization releasing these
     * locks allows another completely independent thread to obtain them and
     * crash the local port.  Unfortunately, this almost always guarantees a
     * machine check when the fork process re-obtains its locks and resumes
     * processing.  This is because the fork process does not know the SII RAM
     * buffer has been rendered inaccessible due to crashing of the local port
     * and almost always attempts to access it anyway.  Fork process
     * synchronization protects against such machine checks by forcing
     * temporary postponement of local port crashing during those rare
     * occassions when active fork processes must temporarily release their
     * locks.
     *
     * At the time the decision is made to crash the port, the processor from
     * which the port is crashed exists in one of two states distinguished
     * mainly by whether the processor is at kernel mode or interrupt level.
     * The existence of these two possible environments does not interfere with
     * port disablement.  Unfortunately, the same is not necessary true for
     * port clean up, a section of the port driver which is quite complicated
     * in its own right.  Therefore, to avoid potential problems and to allow
     * certain code simplifying assumptions to be made, port clean up has been
     * structured into two stages, separated by both time and environment.
     *
     * The first stage of port clean up consists of those actions which should
     * be performed immediately following port disablement and are insensitive
     * to processor state.  The second stage consists of those activities which
     * need not be performed immediately following port disablement and should
     * be executed within a constant well-defined processor state.  The first
     * stage of port clean up is directed by this routine which then schedules
     * the second stage, directed by msi_clean_port(), through forking.  It is
     * this act of forking which generates the constant environment necessary
     * for the second stage of port clean up.
     *
     * The PCCB fork block is used to schedule the second stage of clean up.
     * It should always be available because it is used only for port clean up
     * and re-initialization, these activities are single threaded, and
     * re-initialization always follows clean up.  Guaranteed availability
     * of the PCCB fork block is one of the benefits of single threading of
     * port clean up and re-initialization.
     *
     * The first stage of port clean up includes:
     *
     * 1. Scheduling clean up of the local MSI port through forking.
     * 2. Unlocking the PCCB provided it was locked within this routine.
     *
     * Note that the first stage of port clean up currently does not contain
     * any processor state insensitive actions which must be immediately
     * performed following port disablement.  However, the structure exists to
     * add them as it becomes necessary to do so.
     *
     * The second stage of port clean up consists solely of notifying the CI
     * PPD of failure of the local port.  The CI PPD completes the clean up of
     * its portion of the local port including the failure and clean up all
     * paths, both formative and fully established, originating at the port.
     * Clean up of the last path triggers scheduling of port re-initialization
     * by the CI PPD.  Successful completion of port re-initialization marks
     * the beginning of the next incarnation of the port and releases all
     * restriction on crashing the port if further requests to do so are made.
     */
    if( !Test_pccb_lock( pccb )) {
	Lock_pccb( pccb )
	unlock_pccb = 1;
    } else {
	unlock_pccb = 0;
    }
    if( pccb->Lpstatus.optlpcinfo ) {
	if(( pb = cippd_get_pb( pccb,
				( SCSH * )&pccb->Lpcinfo.pport_addr,
				NO_BUF )) == NULL ) {
	    pccb->Errlogopt.portnum = pccb->Lpcinfo.pport_addr;
	}
    } else {
	pb = NULL;
	pccb->Errlogopt.portnum = EL_UNDEF;
    }
    if( !pccb->Fsmstatus.cleanup ) {
	Set_lpc_event( reason );
    }
    switch( Mask_esevmod( reason )) {

	case SE_BUSERROR:	case SE_SWA:		case SE_IMODE:
	case SE_TMODE:
	    ( void )msi_log_devattn( pccb, reason, LOG_REGS );
	    break;

	case SE_NOPATH:		case SE_MFQE:		case SE_INVBNAME:
	case SE_INVBSIZE:
	    ( void )msi_log_packet( pccb,
				    pb,
				    NULL,
				    (( pccb->Lpstatus.optlpcinfo )
						? pccb->Lpcinfo.pkth : NULL ),
				    (( pccb->Lpstatus.optlpcinfo )
						? pccb->Lpcinfo.pktsize : 0 ),
				    reason,
				    LPORT_EVENT );
	    break;

	case SE_PPDSANITY:
	default:
	    ( void )panic( PANIC_UNKLPC );
    }
    if( msi_lpc_panic ) {
	( void )panic( PANIC_REQLPC );
    }
    if( !pccb->Fsmstatus.cleanup ) {
	pccb->Fsmstatus.cleanup = 1;
	pccb->Fsmstatus.online = 0;

	if( !Test_rfp_lock( pccb )) {
	    Lock_rfp( pccb )
	    unlock_rfp = 1;
	} else {
	    unlock_rfp = 0;
	}
	if( !Test_xfp_lock( pccb )) {
	    Lock_xfp( pccb )
	    unlock_xfp = 1;
	} else {
	    unlock_xfp = 0;
	}
	while( pccb->Xbusy == NULL || pccb->Rbusy == NULL ) {
	    Unlock_xfp( pccb )
	    Unlock_rfp( pccb )
	    Unlock_pccb( pccb )
	    Lock_pccb( pccb )
	    Lock_rfp( pccb )
	    Lock_xfp( pccb )
	}
	Xflush_xbusy( pccb )
	( void )msi_disable( pccb );
	if( unlock_xfp ) {
	    Unlock_xfp( pccb )
	}
	if( unlock_rfp ) {
	    Unlock_rfp( pccb )
	}
	pccb->lpinfo.reason = reason;
	Pccb_fork( pccb, msi_clean_port, PANIC_PCCBFB )
    }
    if( unlock_pccb ) {
	Unlock_pccb( pccb )
    }
}

/*   Name:	msi_remote_rst	- Reset Remote Port and System
 *
 *   Abstract:	This function initiates resetting of a remote MSI port and
 *		system.  The remote system need NOT be known to be reset;
 *		however, the remote port must be in the appropriate maintenance
 *		state for it and the remote system to be reset.
 *
 *		Initiating resetting of a remote port and system is
 *		accomplished by initiating transmission of a MSI port specific
 *		RST packet to the target remote port.  A MSI port command
 *		packet is used to contain the reset request.  It is allocated
 *		by this function and deallocated following packet transmission.
 *
 *		Resetting of the remote port and system can optionally be
 *		forced; however, the port must in an appropriate maintenance
 *		state for this to occur.
 *
 *              NOTE: SCA port numbers are 6 bytes in size; however, maximum
 *                    MSI port numbers only occupy 1 byte.
 *
 *		NOTE: All attempts to transmit packets to the local ports own
 *		      station address are bypassed.  The SII chip is not
 *		      capable of either internal loopback or simultaneous
 *		      transmission and reception; and, no need exists to
 *		      provide this function in software.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   force			- If set, force maintenance reset
 *   pccb			- Port Command and Control Block pointer
 *   rport			- Station address of target remote port
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    comql		-   MSIB low priority command queue
 *	    lpstatus.xfork	-   1
 *	    xforkb		-   Transmit Fork Process fork block
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Transmission successfully initiated
 *   RET_ALLOCFAIL		- Failed to allocate command packet
 *   RET_INVLPSTATE		- Local port in invalid state
 *   RET_NOPATH			- Invalid remote port station address
 *
 *   SMP:       The PCCB is locked allowing exclusive access to PCCB contents.
 *              PCCB addresses are always valid because these data structures
 *              are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		The PCCB specific COMQL is locked allowing exclusive access to
 *		the corresponding low priority command queue
 */
u_long
msi_remote_rst( pccb, rport, force )
    register PCCB	*pccb;
    scaaddr		*rport;
    u_long		force;
{
    register MSIB	*msibp;
    register u_long	status = RET_SUCCESS;

    /* Only the Transmit Fork Process( XFP ) interfaces directly with the SII
     * chip for the purpose of processing outgoing MSI packets.  All other
     * driver routines must interface with the XFP.  Therefore the steps
     * involved in requesting resetting of a remote port and system are as
     * follows:
     *
     * 1. Lock the PCCB.
     * 2. Verify operational status of the local port and validity of the
     *    target remote port station address.
     * 3. Allocate a MSI port command packet.
     * 4. Format the MSIB buffer.
     * 5. Insert the MSIB buffer onto the appropriate command queue.
     * 6. Request the XFP to process outgoing MSI packets.
     * 7. Unlock the PCCB.
     *
     * Requests to reset the local port and system are always satisfied without
     * transmitting an RST packet( Step 2 ) and without performing the actual
     * reset.  Resets of the local port and system have no meaning in the
     * current context.
     *
     * Formatting the MSIB buffer( Step 4 ) consists of formatting the:
     *
     * MSIB buffer header - information needed by XFP for packet transmission
     * MSI port header    - common MSI packet fields
     * MSI packet body	  - RST specific MSI packet fields
     *
     * RST specific MSI packet fields requiring formatting by this routine
     * include only the transaction identifier.  This field is currently always
     * set to 0.  Also, the appropriate bit is set in the MSI port header flags
     * field whenever resetting of the remote port and system is to be forced.
     *
     * MSIB buffers containing reset requests are always inserted onto the
     * appropriate low priority command queue( Step 5 ).  The PCCB specific
     * COMQL is locked immediately prior to insertion and unlocked immediately
     * afterwards.  This guarantees exclusive access to the queue.
     *
     * The XFP is requested to begin processing of outgoing MSI packets( Step
     * 6 ) by scheduling its asynchronous execution.  This step is bypassed
     * whenever XFP execution has already been scheduled but has not yet
     * commenced.  During XFP scheduling the PCCB lock guarantees the existence
     * of only 1 scheduled asynchronous XFP thread at any given moment.
     *
     * NOTE:	It is possible for a XFP thread to be currently active.  This
     *		does not prevent scheduling of asynchronous XFP execution.
     *		However, the new thread does not begin to process outgoing MSI
     *		packets until the currently active thread completes.  Also, no
     *		other additional XFP threads are scheduled until the new thread
     *		begins processing outgoing MSI packets.
     */
    Lock_pccb( pccb )
    if( !pccb->Fsmstatus.online ) {
	status = RET_INVLPSTATE;
    } else if( Scaaddr_lob( *rport ) > pccb->lpinfo.Max_port ) {
	status = RET_NOPATH;
    } else if( Scaaddr_lob( *rport ) != Scaaddr_lob( pccb->lpinfo.addr )){
	if(( msibp = msi_alloc_pkt( pccb ))) {
	    Format_msibh( msibp, rport->low, sizeof( MSI_RST ), DEALLOC_BUF )
	    Format_msih( msibp, RST )
	    msibp->Ph.flags.Freset = force;
	    Format_rst( msibp )
	    Insert_comql( pccb, msibp )
	    Xstart_xfp( pccb )
	} else {
	    status = RET_ALLOCFAIL;
	}
    }
    Unlock_pccb( pccb )
    return( status );
}

/*   Name:	msi_remote_strt - Start Remote Port and System
 *
 *   Abstract:	This function initiates the starting of a remote MSI port and
 *		system.  The remote system need NOT be known to be started;
 *		however, the local port must have previously reset the remote
 *		port for it to start it.
 *
 *		Initiating starting of a remote port and system is accomplished
 *		by initiating transmission of a MSI port specific STRT packet
 *		to the target remote port.  A MSI port command packet is used
 *		to contain the start request.  It is allocated by this function
 *		and deallocated following packet transmission.
 *
 *		A start address can be optionally supplied; otherwise, the
 *		default start address is used.
 *
 *              NOTE: SCA port numbers are 6 bytes in size; however, maximum
 *                    MSI port numbers only occupy 1 byte.
 *
 *		NOTE: All attempts to transmit packets to the local ports own
 *		      station address are bypassed.  The SII chip is not
 *		      capable of either internal loopback or simultaneous
 *		      transmission and reception; and, no need exists to
 *		      provide this function in software.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *   rport			- Station address of target remote port
 *   start_addr			- Start address( OPTIONAL )
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    comql		-   MSIB low priority command queue
 *	    lpstatus.xfork	-   1
 *	    xforkb		-   Transmit Fork Process fork block
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Transmission successfully initiated
 *   RET_ALLOCFAIL		- Failed to allocate command packet
 *   RET_INVLPSTATE		- Local port in invalid state
 *   RET_NOPATH			- Invalid remote port station address
 *
 *   SMP:       The PCCB is locked allowing exclusive access to PCCB contents.
 *              PCCB addresses are always valid because these data structures
 *              are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		The PCCB specific COMQL is locked allowing exclusive access to
 *		the corresponding low priority command queue
 */
u_long
msi_remote_strt( pccb, rport, start_addr )
    register PCCB	*pccb;
    scaaddr		*rport;
    u_long		start_addr;
{
    register MSIB	*msibp;
    register u_long	status = RET_SUCCESS;

    /* Only the Transmit Fork Process( XFP ) interfaces directly with the SII
     * chip for the purpose of processing outgoing MSI packets.  All other
     * driver routines must interface with the XFP.  Therefore the steps
     * involved in requesting starting of a remote port and system are as
     * follows:
     *
     * 1. Lock the PCCB.
     * 2. Verify operational status of the local port and validity of the
     *    target remote port station address.
     * 3. Allocate a MSI port command packet.
     * 4. Format the MSIB buffer.
     * 5. Insert the MSIB buffer onto the appropriate port command queue.
     * 6. Request the XFP to process outgoing MSI packets.
     * 7. Unlock the PCCB.
     *
     * Requests to start the local port and system are always satisfied without
     * transmitting an STRT packet( Step 2 ) and without performing the actual
     * start.  Starts of the local port and system have no meaning in the
     * current context.
     *
     * Formatting the MSIB buffer( Step 4 ) consists of formatting the:
     *
     * MSIB buffer header - information needed by XFP for packet transmission
     * MSI port header    - common MSI packet fields
     * MSI packet body	  - STRT specific MSI packet fields
     *
     * STRT specific MSI packet fields requiring formatting by this routine
     * include the transaction identifier and the start address.  The former is
     * currently always set to 0.  Also, the appropriate bit is set in the MSI
     * port header flags field whenever the remote port and system is to be
     * started at its default start address.
     *
     * MSIB buffers containing start requests are always inserted onto the
     * appropriate low priority command queue( Step 5 ).  The PCCB specific
     * COMQL is locked immediately prior to insertion and unlocked immediately
     * afterwards.  This guarantees exclusive access to the queue.
     *
     * The XFP is requested to begin processing of outgoing MSI packets( Step
     * 6 ) by scheduling its asynchronous execution.  This step is bypassed
     * whenever XFP execution has already been scheduled but has not yet
     * commenced.  During XFP scheduling the PCCB lock guarantees the existence
     * of only 1 scheduled asynchronous XFP thread at any given moment.
     *
     * NOTE:	It is possible for a XFP thread to be currently active.  This
     *		does not prevent scheduling of asynchronous XFP execution.
     *		However, the new thread does not begin to process outgoing MSI
     *		packets until the currently active thread completes.  Also, no
     *		other additional XFP threads are scheduled until the new thread
     *		begins processing outgoing MSI packets.
     */
    Lock_pccb( pccb )
    if( !pccb->Fsmstatus.online ) {
	status = RET_INVLPSTATE;
    } else if( Scaaddr_lob( *rport ) > pccb->lpinfo.Max_port ) {
	status = RET_NOPATH;
    } else if( Scaaddr_lob( *rport ) != Scaaddr_lob( pccb->lpinfo.addr )){
	if(( msibp = msi_alloc_pkt( pccb ))) {
	    Format_msibh( msibp, rport->low, sizeof( MSI_STRT ), DEALLOC_BUF )
	    Format_msih( msibp, STRT )
	    msibp->Ph.flags.Dsa = (( start_addr == 0 ) ? 1: 0 );
	    Format_strt( msibp, start_addr )
	    Insert_comql( pccb, msibp )
	    Xstart_xfp( pccb )
	} else {
	    status = RET_ALLOCFAIL;
	}
    }
    Unlock_pccb( pccb )
    return( status );
}

/*   Name:	msi_shutdown	- Inform Known Systems of Local Shutdown
 *
 *   Abstract:	This routine informs systems known through paths originating
 *		at a specific local MSI port of local system shutdown.  This is
 *		accomplished through disabling of the specified port.  The
 *		remote system discovers the unavailability of the port either
 *		during CI PPD polling or during utilization of the virtual
 *		circuit.  At that time it terminates its path to the port and
 *		and thus to the local system.
 *
 *		NOTE: STOP CI PPD datagrams are not transmitted across all
 *		      formative and established paths.  This is because the
 *		      state of the port is unknown and may in fact preclude
 *		      datagram transmission.
 *
 *   Inputs:
 *
 *   IPL_POWER			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.online	-   0
 *
 *   IPL_POWER			- Interrupt processor level
 *
 *   SMP:       No locks are required.  This routine is only called(
 *		indirectly ) from panic() and it is invoked at maximum IPL.
 *		Only the processor executing this code is operational and this
 *              routine can not be interrupted on this processor.  This
 *              guarantees uncompromised access to the PCCB without locking it.
 */
void
msi_shutdown( pccb )
    PCCB		*pccb;
{
    /* Local port disablement is accomplished through invocation of the
     * appropriate routine.  The local port is taken explicitly offline prior
     * to disablement.  This aborts all CI PPD port polling on the local port.
     */
    pccb->Fsmstatus.online = 0;
    ( void )msi_disable( pccb );
}
