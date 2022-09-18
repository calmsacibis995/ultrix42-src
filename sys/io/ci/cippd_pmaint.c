#ifndef	lint
static char *sccsid = "@(#)cippd_pmaint.c	4.1	(ULTRIX)	7/2/90";
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
 *		Driver( CI PPD ) path maintenance service functions.  All
 *		port drivers which utilize the CI PPD for path establishment,
 *		maintenance, and termination should use these functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 6, 1985
 *
 *   Function/Routines:
 *
 *   cippd_crash_pb		Crash CI PPD Path Represented by Path Block
 *   cippd_get_pb		Retrieve Path Block
 *   cippd_open_pb		Transition Formative Path Block to Open State
 *   cippd_remove_pb		Remove Path Block from System-wide Databases
 *
 *   Modification History:
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's 
 *	Add include file ../h/systm.h this file has define for remque
 *	insque for mips.
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   17-Jan-1989	Todd M. Katz		TMK0004
 *	1. The PCCB address is now part of the interface to Get_port.
 *	2. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	3. Include header file ../vaxmsi/msisysap.h.
 *
 *   17-Aug-1988	Todd M. Katz		TMK0003
 *	1. Modifications to cippd_get_pb().
 *		1) Update the conditions under which cippd_get_pb() considers
 *		   remote port station address invalid.  Formerly, for a remote
 *		   port station address to be considered invalid it only had to
 *		   exceed the software( CI PPD ) maximum port number(
 *		   CIPPD_MAXPATHS - 1 ) and be provided to the function in the
 *		   form of buffer.  Now, not only must both these criteria
 *		   still be met; but in addition, the remote port station
 *		   address must also exceed the hardware maximum port number of
 *		   the specified local port.
 *	   	2) Invoke an appropriate port driver specific routine to log
 *		   invalid remote port station addresses.  Previously the
 *		   logging was done by the CI PPD itself.
 *	2. Modifications to cippd_crash_pb():
 *		1) Initialize the CI PPD specific PCCB field elogopt.port_num
 *		   to EL_UNDEF when it is invoked to crash an non-existent
 *		   path.  This initialization is required by cippd_log_path(),
 *		   the routine invoked to log SCS and CI PPD path crash event
 *		   codes.
 *		2) Apply the path crash severity modifier( ESM_PC ) to the path
 *		   crash reason code whenever the path has not yet been failed.
 *		3) Cache the reason code within the PB only when the path has
 *		   not yet been failed.  Cache a general reason when the path
 *		   is formative.  Cache the specific path crash reason when the
 *		   path is open.  Previously, the path crash reason was always
 *		   being cached.  This could have lead to loss of the
 *		   legitimate reason when the routine is invoked twice in quick
 *		   succession on the same path.  
 *		4) The routine parameter scsbp ALWAYS points to a character
 *		   string of size NAME_SIZE instead of to the SCS header of a
 *		   datagram/message buffer whenever SCS invokes the routine
 *		   with a reason code of E_SYSAP.  This character string
 *		   consists of the name of the local SYSAP responsible for
 *		   crashing the path.
 *		5) Add a comment as to how individual port drivers go about
 *		   crashing paths following discover of physical path specific
 *		   errors.
 *	3. Refer to error logging as event logging.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   11-Apr-1988	Todd M. Katz		TMK0002
 *	1. Add use of Pccb_fork() macro in place of straight-line code.
 *	2. Modify cippd_crash_pb() to optionally panic on requests to crash
 *	   paths.  Error recovery is the default action taken; however,
 *	   panicing occurs whenever the CI PPD configuration variable
 *	   cippd_pc_panic has been appropriately set.  This variable may be set
 *	   to trigger panicing on any path crash request or on only requests to
 *	   crash open paths.
 *	3. CI PPD event logging is now split between the routines
 *	   cippd_log_path() and cippd_log_sys() to differentiate between
 *	   logging of path specific and system level events respectively.
 *	   Modify cippd_crash_pb() and cippd_get_pb() to invoke
 *	   cippd_log_path().
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
extern	u_short		cippd_pc_panic;
extern	void		cippd_log_path(), ksched(), scs_dealloc_pb(),
			scs_dealloc_sb();

/*   Name:	cippd_crash_pb	- Crash CI PPD Path Represented by Path Block
 *
 *   Abstract:	This function crashes a specific CI PPD path to a known system
 *		and initiates the process by which it is disabled and cleaned
 *		up.  This routine may be sequentially invoked any number of
 *		times to crash a specific path; however, only its first
 *		invocation actually results in crashing of the path.  All
 *		subsequent invocations only log crash requests and dispose of
 *		optional buffers.  This situation persists until the next
 *		incarnation of the path which does not occur until after the
 *		path has been cleaned up, re-discovered, and re-established.
 *
 *		An optional message or datagram buffer can be provided.
 *		Alternatively an optional port command packet can be provided
 *		when this routine is invoked by the port drivers themselves.
 *		Three options exist for disposal of this optional buffer:
 *
 *		1. Buffers may be returned to the routine invocator.
 *		2. Buffers may be deallocated.
 *		3. Buffers may be inserted onto the appropriate free queue of
 *		   the appropriate local port.
 *
 *		Only datagram and message buffers may be disposed of by
 *		deallocation or placement onto an appropriate free queue.  PDs
 *		must ALWAYS request return of the buffers they provide when
 *		invoking this routine.
 *
 *		A pointer to a character string of size NAME_SIZE is ALWAYS
 *		provided in place of an optional message or datagram buffer
 *		whenever this routine is invoked with a reason code == E_SYSAP.
 *		This character string contains the name of the local SYSAP
 *		responsible for crashing the specified path.
 *
 *		The appropriate local port is crashed whenever the optional PB
 *		is not provided.
 *
 *		NOTE: Port drivers may invoke this routine following discovery
 *		      of a physical error associated with a specific path.
 *		      However, they must log the explicit physical error
 *		      themselves before invoking this routine with a "generic"
 *		      CI PPD event code( E_PD or SE_PD ) appropriate to the
 *		      severity of the physical error.
 *
 *		NOTE: When port drivers invoke this routine to crash a path,
 *		      the path may have already been disabled.  If so, they
 *		      must set the PB path_closed state bit BEFORE calling this
 *		      routine.
 *
 *		NOTE: All error recovery is bypassed in favor of immediately
 *		      panicing the system whenever the CI PPD configuration
 *		      variable flag cippd_pc_panic is appropriately set.
 *		      Normally, this flag is 0 and error recovery is the
 *		      default action taken.  It is set only when special
 *		      debugging activity is required and may be set to trigger
 *		      panicing on only requests to crash open paths or on
 *		      requests to crash any path, formative or open.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_pc_panic		- CI PPD path crash panic flag
 *   disposal			- RETURN_BUF or DEALLOC_BUF or RECEIVE_BUF
 *   pccb			- Port Command and Control Block pointer
 *   reason			- Reason for crashing virtual path
 *   scsbp			- Address of SCS header in buffer( OPTIONAL )
 *				- Address of local SYSAP name( OPTIONAL )
 *   pb				- Path Block pointer( OPTIONAL )
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer( OPTIONAL )
 *	pinfo.reason		-  Reason for crashing virtual path
 *   pccb			- Port Command and Control Block pointer
 *      ppd.cippd               -  CI PPD specific PCCB fields
 *          elogopt.port_num	-   Remote port number
 *	    elogopt.sysapname	-   Name of local SYSAP crashing path
 *
 *   SMP:	The PCCB is locked INTERNALLY whenever it was not locked
 *		EXTERNALLY prior to routine invocation.  Locking the PCCB
 *		synchronizes access; prevents premature PB deletion; and is
 *		required by cippd_log_path(), by PD routines which add
 *		datagrams or messages to appropriate local port free queues in
 *		this particular case( It is required in this particular case
 *		because the PB is locked ), and by the CI PPD finite state
 *		machine.  PCCB addresses are always valid because these data
 *		structures are never deleted once their corresponding ports
 *		have been initialized.
 *
 *		The PB is locked INTERNALLY( provided it exists ) whenever it
 *		was not locked EXTERNALLY prior to routine invocation.  Locking
 *		the PB allows exclusive access to PB contents and is required
 *		by the CI PPD finite state machine.  PB address validity must
 *		be EXTERNALLY guaranteed either through EXTERNALLY held locks
 *		or by some other mechanism.
 *
 *		Whenever the PB is locked EXTERNALLY the PCCB must also be
 *		locked EXTERNALLY.  This is because the latter occupies a
 *		higher relative position than the former within the SCA locking
 *		hierarchy.
 */
void
cippd_crash_pb( pccb, pb, reason, disposal, scsbp )
    PCCB		*pccb;
    register PB		*pb;
    register u_long	reason;
    u_long		disposal;
    register SCSH	*scsbp;
{
    register GVPPPDH	*cippdbp;
    register u_long	unlock_pb = 0, unlock_pccb = 0;

    /* Crashing of formative and established CI PPD paths is extremely similar
     * with only a few minor differences.  These are pointed out as they are
     * encountered.  Crashing of either type of path consists of the following
     * distinct steps:
     *
     * 1. The crash request is pre-processed.
     * 2. The path is disabled.
     * 3. The path is asynchronously cleaned up.
     *
     * The first step is executed each time this routine is invoked.  It is the
     * latter two steps which constitute crashing the path and are executed
     * only once per path incarnation.  The major benefits of this single
     * threading of path crashing are the assumptions made during path clean up
     * which follow directly from it.  These assumptions greatly reduce code
     * complexity and allow extension of the CI PPD to an SMP environment with
     * a minimum of pain.  They are pointed out as they occur.
     *
     * Pre-processing of crash requests includes:
     *
     * 1. Locking the PCCB whenever it was not locked EXTERNALLY.
     * 2. Locking the PB whenever one was provided and it was not locked
     *    EXTERNALLY.
     * 3. Applying the path crash severity modifier( ESM_PC ) to the path crash
     *    reason code.
     * 4. Caching the path crash reason within the PB for latter reference.
     * 5. Caching the name of the local SYSAP responsible for crashing the path
     *	  for event logging purposes.
     * 6. Logging the crash request.
     * 7. Optionally panicing the system.
     * 8. Disposing of the optional buffer as requested.
     *
     * Steps 3 is executed only if the path is still open as only open paths
     * can be actually crashed.  Formative paths are really just prematurely
     * terminated and failed paths have already been crashed.
     *
     * Step 4 is executed only if the path has not yet been crashed.  This step
     * must only be executed once per path incarnation( ie- only if the path
     * has not yet been and is just about to be crashed ).  However, the reason
     * which is cached depends upon the state of the path at the time it is
     * crashed.  A general path failure reason is cached in PBs representing
     * formative paths.  The specific path crash reason is cached for all open
     * paths.  It is latter mapped into the appropriate general reason during
     * actual processing of the crash request.
     *
     * It is only necessary to cache the name of the local SYSAP( Step 5 )
     * responsible for crashing the path when the reason for crashing the path
     * == E_SYSAP.  In all other cases, no SYSAP is responsible for crashing
     * the path and no name exists to be cached.
     *
     * Panicing of the system( Step 7 ) immediately terminates all further
     * processing of the crash request.  Such panicing is OPTIONAL.  It is
     * requested only when the CI PPD configuration variable flag
     * cippd_pc_panic is appropriately set.  This flag is set only when special
     * debugging activity is required.  It may be set to trigger panicing on
     * requests to crash any path or on only requests to crash open paths.
     *
     * Following pre-processing this routine invokes the CI PPD finite state
     * machine to ascertain whether the path has already been crashed but has
     * not yet been cleaned up.  If this is indeed the case, as indicated by
     * current path state == PS_PATH_FAILURE, then the current request is
     * dismissed.  Otherwise, actual crashing of the path by the finite state
     * machine commences with disablement of the path including:
     *
     * 1. Mapping the specific reason for crashing the path into a more generic
     *	  reason for latter SYSAP( local or remote ) consumption.
     * 2. Disabling the CI PPD path in a PD specific fashion.
     * 3. Transmitting a CI PPD STOP datagram to the remote CI PPD to inform it
     *	  of local failure of the specified path.
     * 4. Invoking a PD specific routine to optionally invalidate the
     *	  appropriate local port translation cache.
     * 5. Incrementing the PB semaphore.
     * 6. Scheduling asynchronous PB clean up.
     * 7. Setting the path state to PS_PATH_FAILURE to prohibit additional
     *    crashings of this incarnation of the path.
     * 8. Unlocking the PB whenever it was locked INTERNALLY.
     * 9. Unlocking the PCCB whenever it was locked INTERNALLY.
     *
     * Established CI PPD paths always proceed through these nine steps.  This
     * is not always the case for formative paths, some of which do not require
     * disabling( Step 2 ) because the CI PPD has not yet enabled them.
     *
     * At the time the decision is made to crash the path, the processor on
     * which the path is crashed exists in one of two environments
     * distinguished by whether the processor is at kernel mode or interrupt
     * level.  The existence of two possible environments does not interfere
     * with path disablement.  Unfortunately, the same is not necessary true
     * for path clean up, a section of the CI PPD which is quite complicated in
     * its own right.  The solution to this potential problem is to decouple
     * path disablement from path clean up by always scheduling clean up of the
     * failed path to occur asynchronously( Step 6 ).  Now when path clean up
     * eventually commences, it always proceeds in a constant environment.
     * This avoids all potential environmental related problems and allows
     * certain code simplifying assumptions to be made.
     *
     * Prior to scheduling asynchronous path clean up, the PB semaphore is
     * incremented to prevent another CI PPD thread from deallocating the PB.
     * Actually, this incrementing is superfluous.  Only cleaned up paths are
     * deallocated and only crashed paths are cleaned up and each path may only
     * be crashed once per path incarnation.  Therefore, it is not possible for
     * another CI PPD thread to be in a position to deallocate the PB and the
     * single threaded nature of path clean up should be sufficient to
     * guarantee PB validity when scheduled clean up eventually commences.  The
     * semaphore is incremented anyway to further protect the PB and to detect
     * errors in error recovery logic.  The semaphore is always decremented
     * prior to deallocation of the PB following its clean up.
     *
     * Finally, a few additional notes on path disablement before proceeding
     * with what is involved in path clean up:
     *
     * 1. The CI PPD finite state machine never unlocks PBs while processing
     *	  path failures.  This requirement is necessary to maintain path crash
     *	  single threading by preventing other CI PPD threads from gaining
     *	  access to the PB until the state change which prohibits sequential
     *	  crashings of this path incarnation occurs( Step 7 ).
     * 2. Transmission of CI PPD STOP datagrams( Step 3 ) is currently not
     *	  supported because of VMS unwillingness to compromise during SCA
     *	  Review Group meetings.  The intent is to support such remote
     *	  notification when and if the issue is finally resolved.
     *
     * Clean up of aborted formative paths proceeds differently from clean up
     * of failed established ones and as a consequence different routines are
     * scheduled to perform each type of clean up.  The routine
     * cippd_clean_fpb() is scheduled whenever an aborted formative path is to
     * be cleaned up.  When it executes, it cleans up the specified path as
     * follows:
     *
     *  1. The PCCB is locked.
     *  2. The PB is locked.
     *  3. The PB semaphore is synchronized to.
     *  4. The CI PPD datagram reserved for path establishment is removed from
     *     the appropriate local port datagram free queue and deallocated.
     *  5. All emergency port specific buffers are optionally deallocated.
     *  6. The formative SB is  optionally deallocated.
     *  7. The PB semaphore is decremented.
     *  8. The formative PB is removed from the appropriate local port
     *	   formative PB queue, unlocked, and deallocated.
     *  9. The number of formative paths originating at the port is decremented
     *     and port initialization is scheduled through forking whenever there
     *     are no longer any paths originating at the port and port clean up is
     *	   currently in progress.
     * 10. The PCCB is unlocked.
     *
     * The routine cippd_clean_pb() is scheduled whenever a failed established
     * path is to be cleaned up.  When it executes, it cleans up the specified
     * path as follows:
     *
     * 1. The PB is locked.
     * 2. The PB semaphore is synchronized to.
     * 3. The PB is unlocked.
     * 4. SCS is notified of failure of the specified path.
     *
     * Once SCS is notified of path failure( Step 4 ), it assumes
     * responsibility for directing PB clean up including the clean up of all
     * SCS connections associated with the failed path.  Clean up of the last
     * connection triggers SCS invocation of cippd_remove_pb().  It is this CI
     * PPD routine which removes the PB from from all system-wide databases,
     * decrements its semaphore, deallocates it, and also schedules port
     * initialization through forking whenever there are no longer any paths
     * originating at the port and port clean up is currently in progress.
     *
     * SCS requires the PB address to be guaranteed at the time it is notified
     * of path failure and the same mechanisms used to guarantee PB address
     * validity on entry to cippd_remove_pb() are employed.  These are single
     * threading of path clean up so that no other thread can possibly delete
     * the PB and incrementing the PB semaphore.  The former mechanism should
     * be all that is necessary to protect the PB, but the latter is utilized
     * as further protection and to detect errors in error recovery logic.
     *
     * Prominent in the clean up of both types of paths is synchronization to
     * the PB semaphore.  Such synchronization is very important in a SMP
     * environment for the following reason.  Occasions exist within the CI
     * PPD which require threads to temporarily release all locks, including PB
     * locks, while still protecting PBs from deletion.  PB semaphores are the
     * mechanism chosen to meet this requirement.  A thread wishing to
     * temporarily protect a PB, increments its semaphore prior to releasing
     * all locks.  The semaphore is decremented only after all locks have been
     * re-obtained and the need for protecting the PB in this fashion has
     * passed.  In the interim, any thread desiring to clean up and deallocate
     * this PB must first synchronize to its semaphore before proceeding.
     *
     * The PCCB fork block is used to schedule port initialization whenever and
     * wherever such initialization becomes necessary.  It should always be
     * available because it is used only for port clean up and initialization,
     * these activities are single threaded, and initialization always follows
     * clean up.  Guaranteed availability of the PCCB fork block is one of the
     * benefits of single threading of port clean up and initialization.
     *
     * One final note on path clean up.  Once PB removal and deallocation
     * completes, no matter who does it, the CI PPD is free to attempt
     * establishment of a new path incarnation.  Such attempts do not occur
     * until after the path is re-discovered through polling.  New path
     * incarnations are fully subject to crashing on encountering of
     * sufficiently serious errors.
     *
     * It is possible for this routine to be invoked to crash a non-existent
     * path.  Typically this situation develops when some error requiring path 
     * crashing for recovery purposes is encountered, but the PB representing
     * the path can NOT be retrieved, and this routine is invoked without a PB.
     * Non-existent paths are pre-processed as if they existed, and then the
     * appropriate local ports are themselves crashed with a special reason
     * code( SE_NOPATH ).  Handling non-existent paths in this fashion allows
     * for recovery from the error to occur while still obtaining and saving
     * the maximum amount of information about the error for latter analysis.
     *
     * Path failures may also occur or be discovered within the CI PPD finite
     * state machine itself during processing of some asynchronously occurring
     * event.  This routine is invoked by the finite state machine to crash
     * such failed paths only when the paths were established at time of
     * failure.  Otherwise, processing of these formative path failures is left
     * entirely to the finite state machine.  This processing is very similar
     * to the crashing of formative paths by the finite state machine.  There
     * are a few significant exceptions and these are listed below:
     *
     * 1. The formative path is always crashable.  It is never in a
     *	  PS_PATH_FAILURE path state, nor is it ever in the process of
     *	  transitioning to such a state.
     * 2. Pre-processing of the crash request is handled internally to the
     *	  finite state machine.
     *
     * Other than these few exceptions, processing of these failed formative
     * paths is identical to processing of aborted formative paths including:
     *
     * 1. Logging of the failure provided it is the first such occurrence.
     * 2. Disabling the path if it had been enabled at time of failure.
     * 3. Transitioning the path state to PS_PATH_FAILURE to prevent other CI
     *	  PPD threads from crashing the same path incarnation.
     * 4. Scheduling the identical routine to asynchronously clean up the
     *    failed formative path.
     */
    if( !Test_pccb_lock( pccb )) {
	Lock_pccb( pccb )
	unlock_pccb = 1;
    }
    if( pb ) {
	if( !Test_pb_lock( pb )) {
	    Lock_pb( pb )
	    unlock_pb = 1;
	}
	if( pb->pinfo.state == PS_OPEN ) {
	    Set_pc_event( reason )
	    pb->pinfo.reason = reason;
	} else if( pb->pinfo.state != PS_PATH_FAILURE ) {
	    pb->pinfo.reason = PF_PPDPROTOCOL;
	}
    } else {
	pccb->Elogopt.port_num = EL_UNDEF;
    }
    if( scsbp ) {
	if( Mask_esevmod( reason ) != E_SYSAP ) {
	    cippdbp = Scs_to_ppd( scsbp );
	} else {
	    cippdbp = NULL;
	    Move_name(( u_char * )scsbp, pccb->Elogopt.sysapname )
	}
    } else {
	cippdbp = NULL;
    }
    ( void )cippd_log_path( pccb, pb, cippdbp, reason );
    if( cippd_pc_panic > SCA_PANIC1 &&
	 ( pb->pinfo.state == PS_OPEN || cippd_pc_panic > SCA_PANIC2 )) {
	( void )panic( PPDPANIC_REQPC );
    } else if( cippdbp ) {
	if( disposal == RECEIVE_BUF ) {
	    if( cippdbp->mtype == SCSMSG ) {
		( void )( *pccb->Add_msg )( pccb, scsbp );
	    } else {
		( void )( *pccb->Add_dg )( pccb, scsbp );
	    }
	} else if( disposal == DEALLOC_BUF ) {
	    if( cippdbp->mtype == SCSMSG ) {
		( void )( *pccb->Dealloc_msg )( pccb, scsbp );
	    } else {
		( void )( *pccb->Dealloc_dg )( pccb, scsbp );
	    }
	}
    }
    if( pb ) {
	( void )cippd_dispatch( pccb, pb, CNFE_PATH_FAIL, NULL );
    } else {
	( void )( *pccb->Crash_lport )( pccb, SE_NOPATH, NULL );
    }
    if( unlock_pb ) {
	Unlock_pb( pb )
    }
    if( unlock_pccb ) {
	Unlock_pccb( pccb )
    }
}

/*   Name:	cippd_get_pb	- Retrieve Path Block
 *
 *   Abstract:	This function retrieves a PB corresponding to a specific path.
 *		The path may either have failed, be fully established, or be in
 *		a formative path state.
 *
 *		The path may be targeted either by a buffer( message or
 *		datagram ) or explicitly by its remote port station address.
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
 *   type			- NO_BUF or BUF
 *   pccb			- Port Command and Control Block pointer
 *   scsbp			- Address of SCS header( type == BUF )
 *				- Address of remote port address( type != BUF )
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   pb				- Address of PB if successful
 *   NULL			- No PB found
 *
 *   SMP:	The PCCB is locked INTERNALLY whenever the PCCB was not locked
 *		EXTERNALLY prior to function invocation.  Locking the PCCB
 *		allows exclusive access to PCCB contents, prevents potential PB
 *		deletion, and is required by PD routines which log invalid port
 *		numbers in case such logging becomes necessary.  PCCB addresses
 *		are always valid because these data structures are never
 *		deleted once their corresponding ports have been initialized.
 *
 *		EXTERNALLY held locks are responsible for preventing PB
 *		deletion once this function retrieves and returns it.
 */
PB *
cippd_get_pb( pccb, scsbp, type )
    register PCCB	*pccb;
    SCSH		*scsbp;
    u_long		type;
{
    register pbq	*pb;
    register u_long	port, unlock = 0;

    /* The steps involved in retrieving the target PB are:
     *
     * 1. Lock the PCCB whenever it was not locked EXTERNALLY.
     * 2. Retrieve the remote port station address of the target PB.
     * 3. Verify the validity of the port station address.
     * 4. Attempt to retrieve the PB from the vector of "open PBs".
     * 5. Search the formative PB queue for the target PB provided the attempt
     *	  made in Step 4 failed.
     * 6. Unlock the PCCB provided it was locked within this function.
     * 7. Return the PB.
     *
     * Invalid remote port station addresses trigger event logging of the
     * offending packet by the appropriate port driver.  Such logging occurs
     * only when:
     *
     * 1. The remote port station address exceeds the hardware maximum port
     *    number for the specified local port.
     * 2. The remote port station address is provided to this function in the
     *	  form of a buffer( type == BUF ).
     *
     * In other words, event logging is bypassed whenever the remote port
     * station address exceeds the current software( CI PPD ) but not the
     * hardware maximum port number of the specified local port; or, the remote
     * port station address is explicitly provided to this function(
     * type == NO_BUF ).
     */
    if( !Test_pccb_lock( pccb )) {
	Lock_pccb( pccb )
	unlock = 1;
    }
    port = (( type == BUF ) ? ( *pccb->Get_port )( pccb, Scs_to_ppd( scsbp ))
			    : ( u_long )*( u_char * )scsbp );
    if( port <= ( CIPPD_MAXPATHS - 1 )) {
	if(( pb = pccb->Open_pb[ port ] ) == NULL ) {
	    for( pb = pccb->Form_pb.flink;
		 pb != &pccb->Form_pb &&
		  Scaaddr_low( Pb->pinfo.rport_addr ) != port;
		 pb = pb->flink ) {}
	    if( pb == &pccb->Form_pb ) {
		pb = NULL;
	    }
	}
    } else {
	pb = NULL;
	if( port > pccb->lpinfo.Max_port && type == BUF ) {
	    ( void )( *pccb->Log_badportnum )( pccb, Scs_to_ppd( scsbp ));
	}
    }
    if( unlock ) {
	Unlock_pccb( pccb )
    }
    return( Pb );
}

/*   Name:	cippd_open_pb	- Transition Formative Path Block to Open State
 *
 *   Abstract:	This function oversees transitioning of formative paths into
 *		the PS_OPEN path state.  It is only invoked when SCS receives a
 *		request for connection establishment on a path that is not
 *		already open.  It must NEVER be invoked for "open" paths.
 *
 *		NOTE: The SCS message buffer passed to this routine must NEVER
 *		      be consumed by it.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *   pb				- Path Block pointer( OPTIONAL )
 *   scsbp			- Address of SCS header in message buffer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Path is successfully transitioned
 *   RET_FAILURE		- Path is unsuccessfully transitioned
 *
 *   SMP:	The PCCB is locked to further prevent PB deletion and as
 *		required by both the CI PPD finite state machine and by PD
 *		routines which close virtual circuits in case such action
 *		becomes necessary.  PCCBs addresses are always valid because
 *		these data structures are never deleted once their
 *		corresponding ports have been initialized.
 *
 *		The PB is locked to synchronize access, postpone potential
 *		deletion, and as required by the CI PPD finite state machine.
 *
 *		The PB must EXTERNALLY be prevented from deletion to guarantee
 *		the validity of its address.
 */
u_long
cippd_open_pb( pccb, pb, scsbp )
    register PCCB	*pccb;
    register PB		*pb;
    register SCSH	*scsbp;
{
    register u_long	status = RET_FAILURE, setcirc = 0;
    register GVPPPDH	*cippdbp = Scs_to_ppd( scsbp );

    /* The steps involved in transitioning PBs representing formative paths
     * into the open state are:
     *
     * 1. Lock the PCCB.
     * 2. Lock the PB.
     * 3. Invoke the CI PPD finite state machine to transition the path.
     * 4. Unlock the PB.
     * 5. Unlock the PCCB.
     *
     * No attempt is made to transition the formative path in the absence of a
     * PB.  This situation occurs when both the path and the attempt to disable
     * it fail and the remote system continues to believe the path is "open".
     * A STOP CI PPD datagram can NOT be transmitted to notify the remote SYSAP
     * of local path failure because no PB exists in order to target datagram
     * transmission.  Neither can the finite state machine be invoked to
     * disable the path because of PB unavailability.  The only action which
     * can and is taken is for this function to make an additional attempt to
     * disable the path.
     *
     * It is possible for SCS sequenced messages to be received over "failed"
     * paths because at time of reception it is possible for neither the path
     * to have been disabled nor the remote CI PPD to have been notified of
     * local path failure.  It is also possible for both actions to have taken
     * place or some combinations in between to have occurred.  Failed paths
     * require special handling and are processed as follows:
     *
     * 1. A STOP CI PPD datagram is transmitted to the remote CI PPD.
     * 2. The return status is changed from RET_SUCCESS to RET_FAILURE
     * 3. The path is disabled.  
     *
     * Step 1 is carried out by the CI PPD finite state machine while Steps 2-3
     * are executed by this function following finite state machine invocation.
     * Step 2 is required because the finite state machine always returns
     * success when processing failed paths and ultimately this routine must
     * return a failure status to indicate its inability to open the path.
     * Step 3 must also be done by this function instead of by the finite state
     * machine.  This is because any PD specific resources associated with the
     * PB and reserved for disabling the corresponding path were exhausted
     * during the prior path failure processing.  Explicitly disabling the path
     * within this function guarantees that at least the attempt is made.
     */
    Lock_pccb( pccb )
    if( pb ) {
	Lock_pb( pb )
	if( pb->pinfo.state == PS_PATH_FAILURE ) {
	    setcirc = 1;
	}
	status = cippd_dispatch( pccb, pb, CNFE_SCSMSG_REC, cippdbp );
	if( pb->pinfo.state == PS_PATH_FAILURE ) {
	    status = RET_FAILURE;
	}
	Unlock_pb( pb )
    } else {
	setcirc = 1;
    }
    if( setcirc ) {
	( void )( *pccb->Set_circuit )( pccb,
					NULL,
					( *pccb->Get_port )( pccb, cippdbp ),
					SET_VC_CLOSE );
    }
    Unlock_pccb( pccb )
    return( status );
}

/*   Name:	cippd_remove_pb	- Remove Path Block from System-wide Databases
 *
 *   Abstract:	This routine removes and deallocates the PB representing a
 *		specific failed path from all system-wide databases.  It may be
 *		invoked only for established paths and only by SCS.  PBs
 *		representing aborted formative paths are deallocated by
 *		cippd_clean_fpb() and never through SCS intervention.
 *
 *		The SB associated with the path is deallocated following its
 *		removal from the system-wide configuration database provided
 *		there are no longer any remaining paths to it.
 *
 *		Scheduling of PD specific port re-initialization occurs if
 *		crash initiated clean up of the local port has completed.
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
 *	    fsmstatus.fkip	-   0
 *   pb				- Path Block pointer
 *	pinfo.state		-  PS_PATH_FAILURE
 *	pinfo.nconns		-  0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	forkb			-  PCCB fork block
 *	lpinfo.ppd.cippd	-  CI PPD specific local port information
 *	    npaths		-   Number of paths associated with the port
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.fkip	-   Fork operation in progress
 *   sb				- System Block pointer
 *	sinfo.npaths		-  Number of paths to system
 *
 *   SMP:	The SCA database is locked( EXTERNALLY ) for PB removal from
 *		the system-wide configuration database, for SB removal from
 *		this database if it becomes necessary, and to guarantee the
 *		validity of the PB address on entry to this routine.
 *
 *		The PCCB is locked to allow exclusive access to PCCB contents.
 *		PCCB addresses are always valid because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		The PB is locked to synchronize access, for removal from all
 *		system-wide databases, for deallocation, and as required by PD
 *		routines which optionally deallocate emergency command packets
 *		associated with the PB.  PB locks are released immediately
 *		prior to PB deallocation as required by scs_dealloc_pb().
 *		PB address validity is guaranteed both by single threading of
 *		path clean up and by incrementing the PB semaphore prior
 *		to scheduling of asynchronous PB clean up.  The former should
 *		be sufficient to guarantee validity, but the latter is done
 *		anyway as additional protection and to detect errors in error
 *		recovery logic.
 *
 *		The PB semaphore is decremented following PB locking.  It was
 *		originally incremented prior to scheduling of asynchronous path
 *		clean up to act as a further guarantee of PB validity and to
 *		detect errors in error recovery logic when clean up eventually
 *		commenced.
 */
void
cippd_remove_pb( pccb, pb )
    register PCCB	*pccb;
    register PB		*pb;
{
    register GVPH	*cibp;
    register SB		*sb = pb->sb;
    register u_long	( *dealloc )();

    /* Removing the PB from the system-wide databases includes:
     *
     *  1. Locking the PCCB.
     *  2. Locking the PB.
     *  3. Decrementing the PB semaphore.
     *  4. Optionally removing and deallocating all PD emergency command
     *	   packets still associated with the PB.
     *  5. Removing the PB from all internal queues.
     *  6. Unlocking the PB.
     *  7. Deallocating the PB.
     *  8. Decrementing the count of paths associated with the appropriate SB,
     *	   and removing the SB from all queues and deallocating it when there
     *     are no longer any paths to the system.
     *  9. Decrementing the number of paths originating at the port and
     *     scheduling port initialization through forking only when there are
     *     no longer any paths originating at the port and port clean up is
     *	   currently in progress.
     * 10. Unlock the PCCB.
     *
     * The PB semaphore is decremented( Step 3 ) because it was incremented by
     * the CI PPD finite state machine prior to asynchronous scheduling of PB
     * clean up to act as a further guarantee of PB validity when clean up( by
     * cippd_clean_pb() ) eventually commenced and to detect errors in error
     * recovery logic.
     *
     * The PCCB fork block is used to schedule port initialization( Step 8 ).
     * It should always be available because it is used only for port clean up
     * and initialization, these activities are single threaded, and
     * initialization always follows clean up.  Guaranteed availability of the
     * PCCB fork block is one of the benefits of single threading of port clean
     * up and initialization.
     *
     * Once PB removal and deallocation completes, the CI PPD is free to
     * attempt establishment of a new path incarnation.  Such attempts do not
     * occur until after the path is re-discovered through polling.  New path
     * incarnations are fully subject to crashing on encountering of
     * sufficiently serious errors.
     */
    Lock_pccb( pccb )
    Lock_pb( pb )
    Decr_pb_sem( pb )
    if( Test_pb_sem( pb )) {
	( void )panic( PPDPANIC_BPATH );
    }
    if(( dealloc = pccb->Dealloc_buf )) {
	( void )( *dealloc )( pb );
    }
    Remove_entry( pb->flink )
    pccb->Open_pb[ Scaaddr_low( pb->pinfo.rport_addr )] = NULL;
    Unlock_pb( pb )
    ( void )scs_dealloc_pb( pccb, pb );
    if(( --sb->sinfo.npaths ) == 0 ) {
	Remove_entry( sb->flink )
	( void )scs_dealloc_sb( sb );
    }
    if( --pccb->lpinfo.Npaths == 0 &&
	 pccb->Fsmstatus.cleanup   &&
	 pccb->lpinfo.Nform_paths == 0 ) {
	Pccb_fork( pccb, pccb->Init_port, PPDPANIC_PCCBFB )
    }
    Unlock_pccb( pccb )
}
