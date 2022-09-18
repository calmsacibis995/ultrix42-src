#ifndef	lint
static char *sccsid = "@(#)cippd_error.c	4.1	(ULTRIX)	7/2/90";
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
 *		Driver( CI PPD ) error processing functions and routines.
 *
 *   Creator:	Todd M. Katz	Creation Date:	January 15, 1988
 *
 *   Function/Routines:
 *
 *   cippd_clean_fpb		Clean up Aborted Formative CI PPD Path
 *   cippd_clean_pb		Clean up CI PPD Path Following Path Failure
 *   cippd_conlog		Log CI PPD Events to Console Terminal
 *   cippd_csyslev		Process CI PPD Common System Level Events
 *   cippd_log_path		Event Log CI PPD Path Specific Events
 *   cippd_log_sys		Event Log CI PPD System Specific Events
 *
 *   Modification History:
 *
 *   19-Sep-1989	Pete Keilty
 *	Add SCA errlogging levels to cippd.
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's 
 *	Changed splx( IPL_SCS ) to new macro Splscs()
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   17-Jan-1989	Todd M. Katz		TMK0006
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Some hardware platforms require field alignments and access types
 *	   to match( ie- only longword aligned entities may be longword
 *	   accessed ).  Structure fields of type c_scaaddr present a potential
 *	   problem because they are 6 bytes long but are often treated as
 *	   contigious longword and word.  Such fields have been longword
 *	   aligned whenever possible.  However, it is not feasible to align
 *	   certain fields with this format within the CI PPD error log event
 *	   packet for compatibility reasons.  Therefore, what has been changed
 *	   for these fields is how they are accessed.  They are now accessed as
 *	   three contigious words instead of as contigious longword and word.
 *	3. Modify cippd_log_path() and cippd_log_sys() to always initialize the
 *	   CI PPD error log packet field cippd_optfmask2 to 0.
 *	4. Modify cippd_conlog() to include the local port name when
 *	   identifying the path associated with the event being reported.
 *	5. Include header file ../vaxmsi/msisysap.h.
 *
 *   23-Sep-1988	Todd M. Katz		TMK0005
 *	1. Modify cippd_conlog() to add remote port station address( CF_RPORT )
 *	   as a class of variable information displayed.
 *	2. Modify cippd_log_path() to treat all events of remote error severity
 *	   as remote port specific events for the purpose of logging them to
 *	   the console.
 *	3. Modify cippd_log_path() to completely set the remote port station
 *	   address field( cippd_rsaddr ) of the event record to undefined(
 *	   EL_UNDEF ) when the identity of the remote port is unknown.
 *	   Currently, only the low order longword of this field is being set.
 *
 *   18-Aug-1988	Todd M. Katz		TMK0004
 *	 1. Refer to error logging as event logging.
 *	 2. SCA event codes have been completely revised.
 *	 3. The following informational events( ES_I ) have been redefined as
 *	    warning events( ES_W ): TALKUP, BADSWTYPE.
 *	 4. The following informational events( ES_I ) have been redefined as
 *	     remote error events( ES_RE ): DBCONFLICT, RERROR.
 *	 5. The following informational events( ES_I ) have been redefined as
 *	    error events( ES_E ): TALKDOWN.
 *	 6. Rename the informational event( ES_IC ) mnemonic from IE -> I.
 *	 7. Modifications to cippd_conlog():
 *		1) Invalid remote port station address( CF_BADPORTNUM ) and
 *		   path information( CF_PATH ) have been eliminated and local
 *		   SYSAP( CF_SYSAPNAME ) and port( CF_LPORTNAME ) names have
 *		   been added as classes of variable information displayed.
 *		2) All event code verification checks have been redone.
 *		3) Logging of certain specific events now occurs regardless of
 *		   the current CI PPD severity level.
 *		4) Define a new arguement to the routine, event_type( type of
 *		   event being console logged ).
 *		5) The general format of all messages has been redesigned.
 *		   Path specific information is always displayed by default if
 *		   the event being logged is a path specific event(
 *		   PATH_EVENT ).  The remote system name is always displayed by
 *		   default if the event being logged is a system specific
 *		   event( SYSTEM_EVENT ).  No information is displayed by
 *		   default if the event being logged is a remote port specific
 *		   event( RPORT_EVENT ).
 *	 8. Modifications to cippd_log_sys():
 *		1) Initialize the new CI PPD event logging packet field
 *		   cippd_evpktver.
 *		2) Initialize the new CI PPD system level information
 *		   structure cippdscommon.
 *		3) Log the current number of paths to the remote system.
 *		4) Eliminate remote system software type as a class of optional
 *	    	   information logged.
 *	 9. Modifications to cippd_log_path():
 *		1) Initialize the new CI PPD event logging packet field
 *		   cippd_evpktver.
 *		2) Log the path state and current number of paths to the remote
 *		   system.
 *		3) Increment the number of errors associated with the
 *		   appropriate local port only when the severity of the current
 *		   event is error( ES_E ) or higher.
 *		4) Add new path information and local SYSAP name as classes of
 *		   optional information logged.
 *		5) This routine is now capable of logging both path and remote
 *		   port specific events.
 *	10. Modify cippd_clean_fpb() to bypass removal and deallocation of the
 *	    CI PPD datagram reserved for path establishment from the
 *	    appropriate local port datagram free queue when local port failure
 *	    was responsible for abortion of path establishment.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *	1. Modify cippd_errorlog() to log path establishment failures
 *	   IE_ALLOCFAIL and IE_NORETRIES and unsupported remote system software
 *	   type informational event IE_BADSWTYPE.
 *	2. Add path( CF_RPORT ) and system software type( CF_SWTYPE ) as
 *	   classes of variable information displayed by cippd_conlog().  Also,
 *	   update the format in which various events are displayed.
 *	3. Split cippd_errorlog() into the two routines cippd_log_path() and
 *	   cippd_log_sys() to differentiate between logging of path specific
 *	   and common system level events respectively.  Modify both routines
 *	   to appropriately initialize the new CI PPD event log packet field
 *	   cippd_optfmask according to the information being logged.  This
 *	   field consists of a bit mask with a unique bit assigned for each
 *	   field optionally present within a CI PPD event log packet.
 *	4. Add System Block as an optional argument to cippd_conlog().
 *	5. Add routine cippd_csyslev() to govern both logging and bookkeeping
 *	   of common system level events.  Such events are associated with
 *	   specific remote systems and not with specific ports on those
 *	   systems.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   11-Apr-1988	Todd M. Katz		TMK0002
 *	1. Add use of Pb_fork_done() and Pccb_fork() macros in place of
 *	   straight-line code.
 *	2. Modify cippd_errorlog() to include logging of the local port name
 *	   whenever an event pertains to a specific CI PPD path.
 *
 *   15-Jan-1988	Todd M. Katz		TMK0001
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
#include		"../h/time.h"
#include		"../h/ksched.h"
#include		"../h/kmalloc.h"
#include		"../h/errlog.h"
#include		"../h/smp_lock.h"
#include		"../h/dyntypes.h"
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
extern	SCSIB		lscs;
extern	struct el_rec	*ealloc();
extern	cippd_slibq	cippd_sli_db;
extern	u_short		cippd_severity;
extern	u_short		cippd_errlog;
extern	CLSTAB		cippd_cltab[ ES_SE + 1 ][ EC_PD + 1 ];
extern	void		cippd_conlog(), cippd_log_sys(), scs_dealloc_pb(),
			scs_dealloc_sb(), scs_path_crash();

/*   Name:	cippd_clean_fpb	- Clean up Aborted Formative CI PPD Path
 *
 *   Abstract:	This routine cleans up formative paths whose establishment has
 *		been aborted due to either errors or failure of the local ports
 *		to which they are associated.  It is always invoked by forking
 *		to it.
 *
 *		The PB is deallocated following clean up.  Clean up may involve
 *		deallocation of a formative SB.
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
 *   pb				- Path Block pointer
 *	pinfo.state		-  PS_PATH_FAILURE
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus.fkip	-   1
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.ppd.cippd	-  CI PPD specific local port information
 *	    nform_paths		-   Current number of formative paths
 *
 *   SMP:	The PCCB is locked to synchronize access, for PB removal from
 *		the local port formative PB queue, and for deletion of the PB 
 *		representing the formative path.  PCCB addresses are always
 *		valid because these data structures are never deleted once
 *		their corresponding ports have been initialized.
 *
 *		The PB is locked allowing exclusive access for deleting both PB
 *		and optional formative SB and as required by optional PD
 *		routines which delete emergency port specific buffers.  PB
 *		address validity is guaranteed both by single threading of path
 *		clean up and by incrementing the PB semaphore prior to
 *		scheduling of the asynchronous invocation of this routine.  The
 *		former should be sufficient to guarantee validity, but the
 *		latter is done anyway as additional protection and to detect
 *		errors in error recovery logic.
 *
 *		PB semaphores are synchronized to before path clean up is
 *		actually initiated because occasions exist elsewhere within
 *		the CI PPD when PBs must be unlocked and yet temporarily
 *		protected against the premature deletion which path clean up
 *		entails.  PB semaphores are also used to guarantee PB validity
 *		on entry to this routine.
 */
void
cippd_clean_fpb( pb )
    register PB		*pb;
{
    register SB		*sb;
    register SCSH	*scsbp;
    register PCCB	*pccb;
    register u_long	( *dealloc )();
    register u_long	save_ipl = Splscs();

    /* Clean up of an aborted formative path includes:
     *
     * 1. Locking the PCCB.
     * 2. Locking the PB.
     * 3. Synchronizing to the PB semaphore.
     * 4. Removing and deallocating the CI PPD datagram reserved for path
     *    establishment from the appropriate local port datagram free queue.
     * 5. Optionally deallocating all emergency port specific buffers.
     * 6. Optionally deallocating a formative SB.
     * 7. Removing the formative PB from the appropriate local port formative
     *	  PB queue, unlocking it, and deallocating it.
     * 8. Decrementing the number of formative paths originating at the port
     *    and scheduling port initialization through forking only when there
     *    are no longer any paths originating at the port and port clean up is
     *	  currently in progress.
     * 9. Unlocking the PCCB.
     *
     * Synchronization to the PB semaphore( Step 3 ) is important in a SMP
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
     * The removal and deallocation of the CI PPD datagram reserved for path
     * establishment( Step 4 ) is bypassed whenever failure of the local port
     * was responsible for abortion of path establishment.
     *
     * The PCCB fork block is used to schedule port initialization( Step 8 ).
     * It should always be available because it is used only for port clean up
     * and initialization, these activities are single threaded, and
     * initialization always follows clean up.  Guaranteed availability of the
     * PCCB fork block is one of the benefits of single threading of port clean
     * up and initialization.
     *
     * One final note.  Once PB removal and deallocation completes, the CI PPD
     * is free to attempt establishment of a new path incarnation.  Such
     * attempts do not occur until after the path is re-discovered through
     * polling.  New path incarnations are fully subject to crashing on
     * encountering of sufficient serious errors.
     */
    pccb = pb->pccb;
    Lock_pccb( pccb )
    Lock_pb( pb )
    Pb_fork_done( pb, PPDPANIC_PBFB )
    Decr_pb_sem( pb );
    while( Test_pb_sem( pb )) {
	Incr_pb_sem( pb );
	Unlock_pb( pb )
	Unlock_pccb( pccb )
	Lock_pccb( pccb )
	Lock_pb( pb )
	Decr_pb_sem( pb );
    }
    if( !Port_failure( pb->pinfo.reason )) {
	if(( scsbp = ( *pccb->Remove_dg )( pccb ))) {
	    ( void )( *pccb->Dealloc_dg )( pccb, scsbp );
	}
    }
    if(( dealloc = pccb->Dealloc_buf )) {
	( void )( *dealloc )( pb );
    }
    if(( sb = pb->sb )) {
	( void )scs_dealloc_sb( sb );
    }
    Remove_entry( pb->flink )
    Unlock_pb( pb )
    ( void )scs_dealloc_pb( pccb, pb );
    if( --pccb->lpinfo.Nform_paths == 0 &&
	 pccb->Fsmstatus.cleanup	&&
	 pccb->lpinfo.Npaths == 0 ) {
	Pccb_fork( pccb, pccb->Init_port, PPDPANIC_PCCBFB )
    }
    Unlock_pccb( pccb )
    ( void )splx( save_ipl );
}

/*   Name:	cippd_clean_pb	- Clean up CI PPD Path Following Path Failure
 *
 *   Abstract:	This routine cleans up established paths which have failed due
 *		to either errors or failure of the local port to which they are
 *		associated.  It is always invoked through forking.
 *
 *		Asynchronous invocation of this routine marks the completion of
 *		path disablement and the beginning of path clean.  This routine
 *		initiates path clean by notifying SCS of the occurrence of path
 *		failure and allowing it to direct clean up of the failed path.
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
 *   pb				- Path Block pointer
 *	pinfo.state		-  PS_PATH_FAILURE
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus.fkip	-   1
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PB is locked during synchronization to its semaphore.  It
 *		is unlocked prior to SCS notification of path failure.  PB
 *		address validity is guaranteed both by single threading of path
 *		clean up and by incrementing the PB semaphore prior to
 *		scheduling of the asynchronous invocation of this routine.  The
 *		former should be sufficient to guarantee validity, but the
 *		latter is done anyway as additional protection and to detect
 *		errors in error recovery logic.
 *
 *		PB semaphores are synchronized to before SCS notification of
 *		path failure occurs because occasions exist elsewhere within
 *		the CI PPD when PBs must be unlocked and yet temporarily
 *		protected against the premature deletion which this SCS
 *		notification eventually results in.  PB semaphores are also
 *		used to guarantee PB validity on entry to this routine.
 */
void
cippd_clean_pb( pb )
    register PB		*pb;
{
    register u_long	save_ipl = Splscs();

    /* Clean up of failed established paths includes:
     *
     * 1. Locking the PB.
     * 2. Synchronizing to the PB semaphore.
     * 3. Unlocking the PB.
     * 4. Notifying SCS of failure of the specified path.
     *
     * Once SCS is notified of path failure( Step 4 ), it takes over
     * responsibility for directing PB clean up including clean up of all SCS
     * connections associated with the failed path.  Clean up of the last
     * connection triggers SCS invocation of cippd_remove_pb().  It is this CI
     * PPD routine which removes the PB from from all system-wide databases,
     * decrements its semaphore, deallocates it, and also schedules port
     * initialization through forking whenever there are no longer any paths
     * originating at the port and port clean up is currently in progress.
     *
     * SCS requires the PB address to be guaranteed at the time it is notified
     * of path failure and the same mechanisms used to guarantee PB address
     * validity on entry to this routine are employed.  These are single
     * threading of path clean up so that no other thread can possibly delete
     * the PB and incrementing the PB semaphore.  The former mechanism should
     * be all that is necessary to protect the PB, but the latter is utilized
     * as further protection and to detect errors in error recovery logic.
     *
     * Synchronization to the PB semaphore( Step 2 ) is important in a SMP
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
     * One final note.  Once PB removal and deallocation completes, the CI PPD
     * is free to attempt establishment of a new path incarnation.  Such
     * attempts do not occur until after the path is re-discovered through
     * polling.  New path incarnations are fully subject to crashing on
     * encountering of sufficient serious errors.
     */
    Lock_pb( pb )
    Pb_fork_done( pb, PPDPANIC_PBFB )
    Decr_pb_sem( pb )
    while( Test_pb_sem( pb )) {
	Incr_pb_sem( pb )
	Unlock_pb( pb )
	Lock_pb( pb )
	Decr_pb_sem( pb )
    }
    Incr_pb_sem( pb )
    Unlock_pb( pb )
    ( void )scs_path_crash( pb );
    ( void )splx( save_ipl );
}

/*   Name:	cippd_conlog	- Log CI PPD Events to Console Terminal
 *
 *   Abstract:	This routine logs CI PPD events to the console terminal.  The
 *		event is always one of the following types:
 *
 *		PATH_EVENT	- Path specific event
 *		SYSTEM_EVENT	- System specific event
 *		RPORT_EVENT	- Remote port specific event
 *
 *		Explicit formatting information must be provided for each
 *		event.  This requires updating of the following tables each
 *		time a new event is defined:
 *
 *		1. The appropriate entry within the CI PPD console logging
 *		   table( cippd_cltab[][] ) must be updated to reflect the new
 *		   maximum code represented within the associated format table.
 *
 *		2. The associated format table itself( cippd_cli[],
 *		   cippd_clw[], cippd_clre[], cippd_cle[], cippd_clse[],
 *		   cippd_clscse[], cippd_clscsse[] ) must be updated with both
 *		   the class of variable information and exact text to be
 *		   displayed.  However, the appropriate table should be updated
 *		   with a NULL entry when the CI PPD is specifically NOT to log
 *		   the new event.  This applies to cippd_clse[] when a new CI
 *		   PPD severe error event is to be specifically logged by the
 *		   appropriate port drivers only.  It also applies to
 *		   cippd_clscse[] or cippd_clscsse[] when a new SCS event of
 *		   severity error( ES_E ) or severe error( ES_SE ) respectively
 *		   is to be logged only by SCS and not by the CI PPD.
 *
 *		NOTE: Console logging of events is bypassed whenever the event
 *		      severity does not warrant console logging according to
 *		      the current CI PPD severity level( cippd_severity ).
 *		      Such bypassing is overridden when the ECLAWAYS bit is
 *		      set in the event code indicating that the event is always
 *		      to be logged regardless of the current severity level.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      CI PPD with the exception of those SCS error and severe
 *		      error events which are candidates for application of the
 *		      path crash severity modifier( ESM_PC ).  It also does NOT
 *		      currently log any CI PPD local port specific events(
 *		      LPORT_EVENT ), all of which are candidates for
 *		      application of the local port crash severity modifier(
 *		      ESM_LPC ).  Such events( the latter ones ) have NULL
 *		      entries in the appropriate console logging format table.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_cltab		- CI PPD Console logging formatting table
 *   cippd_severity		- CI PPD console logging severity level
 *   event			- Event logging code
 *   event_type			- PATH_EVENT, SYSTEM_EVENT, RPORT_EVENT
 *   pb				- Path Block pointer( OPTIONAL )
 *   sb				- System Block pointer( OPTIONAL )
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    elogopt.dbcoll	-   Database collision event information
 *          elogopt.port_num	-   Remote port number( when PB == NULL )
 *	    elogopt.protocol	-   CI PPD protocol information
 *	    elogopt.rswtype	-   Remote system software type
 *	    elogopt.sysapname	-   Name of local SYSAP
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:      	The PCCB is locked( EXTERNALLY ) to synchronize access and
 *		prevent premature PB deletion when a PB is provided.
 *
 *		PBs do NOT require locking when provided because only static
 *		fields are accessed and they are prevented from deletion by
 *		EXTERNAL PCCB locks.  SBs NEVER require locking.
 */
void
cippd_conlog( pccb, pb, sb, event, event_type )
    register PCCB	*pccb;
    register PB		*pb;
    register SB		*sb;
    register u_long	event;
    register u_long	event_type;
{
    register u_long	severity = Eseverity( event );

    /* Events are logged according to the class of variable information they
     * display.  The following classes of variable information currently exist:
     *
     * 1. Remote system name.
     * 2. Database conflict system node names and path specific information.
     * 3. Remote system name and system software type.
     * 4. Local SYSAP name.
     * 5. Local port name.
     * 6. Remote port station address.
     *
     * Certain events may also be logged without displaying any variable
     * information.
     *
     * Console messages for path specific events( SCS and CI PPD ) always
     * display the local port name, local and remote port station addresses and
     * remote system name by default.  Console messages for system specific
     * events( CI PPD only ) always display the remote system name by default.
     * Console messages for remote port specific events( CI PPD only ) do NOT
     * display any information by default.
     *
     * A panic occurs whenever the CI PPD is not prepared to log the event due
     * to detection of any of the following circumstances:
     *
     * 1. The event type is unknown.
     * 2. The event is a port driver specific event.
     * 3. The severity level of the event is invalid.
     * 4. The code of the event exceeds the current maximum known code for the
     *	  class( SCS or CI PPD ) and severity level of the event.
     * 5. The event is not represented within the appropriate console logging
     *    formatting table( indicating that the CI PPD should never have been
     *	  asked to log it in the first place ).
     * 6. The class of variable information associated with the event is
     *	  unknown.
     *
     * None of these circumstances should ever occur.
     *
     * NOTE: Currently, all SCS error and severe error events are candidates
     *	     for application of the path crash severity modifier( ESM_PC ) and
     *	     are logged by the CI PPD( and other individual port drivers ).
     *	     There are no SCS error and severe error events logged only by SCS.
     *
     * NOTE: Events represented within console logging format tables by NULL
     *	     entries are events which are always to be logged only by
     *	     individual port drivers and never by the CI PPD.  Currently, only
     *	     local port specific CI PPD severe error events fall into this
     *	     category.
     */
    if( event_type > RPORT_EVENT				||
	 Test_pd_event( event )					||
	 severity > ES_SE					||
	 Ecode( event ) > Cippd_clmaxcode( cippd_cltab, event )	||
	 Cippd_cltabmsg( cippd_cltab, event ) == NULL ) {
	( void )panic( PPDPANIC_UNKCOD );
    } else if( Cippd_cltabcode( cippd_cltab, event ) > CF_RPORT ) {
	( void )panic( PPDPANIC_UNKCF );
    } else if( cippd_severity > severity && !Test_cloverride( event )) {
	return;
    }
    ( void )cprintf( "ci ppd\t- " );

    switch( event_type ) {

	case SYSTEM_EVENT:
	    ( void )cprintf( "%s( remote system: ",
			     Cippd_cltabmsg( cippd_cltab, event ));
	    if( sb ) {
		( void )cprintf( "%8s )", sb->sinfo.node_name );
	    } else {
		( void )cprintf( "? )" );
	    }
	    break;

	case PATH_EVENT:
	    if( Test_pc_event( event )) {
		cprintf( "path closing( local( %4s )/remote port: %u/%u",
			 &pccb->lpinfo.name,
			 Scaaddr_low( pccb->lpinfo.addr ),
			 (( pb ) ? Scaaddr_low( pb->pinfo.rport_addr )
				 : pccb->Elogopt.port_num ));
		if( sb ) {
		    ( void )cprintf( ", remote system: %8s )\n\t- ",
				     sb->sinfo.node_name );
		} else {
		    ( void )cprintf( ", remote system: ? )\n\t- " );
		}
	    }
	    ( void )cprintf( "%s", Cippd_cltabmsg( cippd_cltab, event ));
	    if( !Test_pc_event( event )) {
		cprintf( "\n\t- path( local( %4s )/remote port: %u/%u",
			 &pccb->lpinfo.name,
			 Scaaddr_low( pccb->lpinfo.addr ),
			 (( pb ) ? Scaaddr_low( pb->pinfo.rport_addr )
				 : pccb->Elogopt.port_num ));
		if( sb ) {
		    ( void )cprintf( ", remote system: %8s )",
				     sb->sinfo.node_name );
		} else {
		    ( void )cprintf( ", remote system: ? )" );
		}
	    }
	    break;

	case RPORT_EVENT:
	    ( void )cprintf( "%s", Cippd_cltabmsg( cippd_cltab, event ));
	    break;
    }
    switch( Cippd_cltabcode( cippd_cltab, event )) {

	case CF_NONE:
	    ( void )cprintf( "\n" );
	    break;

	case CF_DBCONFLICT:
	    ( void )cprintf( "\n\t- known system:  %8s, ",
			     pccb->Elogopt.dbcoll.cippd_kname );
	    ( void )cprintf( "on path( local/remote port: %u/%u )",
			  Scaaddr_low( *pccb->Elogopt.dbcoll.cippd_klsaddr ),
			  Scaaddr_low( *pccb->Elogopt.dbcoll.cippd_krsaddr ));
	    if( sb ) {
		( void )cprintf( "\n\t- remote system: %8s, ",
				 sb->sinfo.node_name );
	    } else {
		( void )cprintf( "\n\t- remote system: ?, " );
	    }
	    ( void )cprintf( "on path( local/remote port: %u/",
			     Scaaddr_low( pccb->lpinfo.addr ));
	    if( pb ) {
		( void )cprintf( "%u )\n",
				 Scaaddr_low( pb->pinfo.rport_addr ));
	    } else {
		( void )cprintf( "? )\n" );
	    }
	    break;

	case CF_SWTYPE:
	    ( void )cprintf( "\n\t- remote system software type: %4s\n",
			     &pccb->Elogopt.rswtype );
	    break;

	case CF_SYSAPNAME:
	    ( void )cprintf( "\n\t- local sysap name: %16s\n",
			     pccb->Elogopt.sysapname );
	    break;

	case CF_LPORTNAME:
	    ( void )cprintf( "\n\t- local port name: %4s\n",
			     &pccb->lpinfo.name );
	    break;

	case CF_RPORT:
	    ( void )cprintf( "( remote port: %u )\n",
			     (( pb ) ? Scaaddr_low( pb->pinfo.rport_addr )
				     : pccb->Elogopt.port_num ));
	    break;
    }
}

/*   Name:	cippd_csyslev	- Process CI PPD Common System Level Events
 *
 *   Abstract:	This routine processes CI PPD common system level events.  Such
 *		events are associated with specific remote systems and not with
 *		specific ports on those systems.  They are normally ascertained
 *		indirectly from received CI PPD datagrams or messages although
 *		such packets are never passed to this routine.
 *
 *		CI PPD common system level event processing has a single goal:
 *		to determine whether or not a reported event should be logged.
 *		This determination is based upon whether the event has been
 *		previously reported for the system in question.  If it has,
 *		logging of new event reports is bypassed.  If it has not, the
 *		event is logged and kept track of.  This state is allowed to
 *		persist until the condition underlying the event is reported to
 *		no longer be associated with the system.
 *
 *		To allow this goal to be met this routine is called both to
 *		report the discovered presence and absence of the following
 *		CI PPD common system level events:
 *
 *		1. E_TALKDOWN  - CI PPD protocol mismatch: Can not talk down
 *		2. W_TALKUP    - CI PPD protocol mismatch: Attempt to talk up
 *		3. W_BADSWTYPE - Remote system has unsupported software type
 *
 *		The first of these events is serious as its severity level
 *		indicates.  The local CI PPD never attempts to talk down to a
 *		remote CI PPD running an earlier CI PPD protocol version.  The
 *		latter two events are less serious and require only warnings to
 *		be given.  Communication with the remote CI PPD is allowed to
 *		continue( or at least be attempted ).
 *
 *		NOTE: The static structure, event_to_flag[], within this
 *		      routine must be updated for each and every new common
 *		      system level event.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_sli_db		- CI PPD Common Sys Level Logging Info Database
 *   event			- W_BADSWTYPE, W_TALKUP, E_TALKDOWN
 *   sb				- System Block pointer
 *   option			- SET or CLEAR
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    elogopt.protocol	-   CI PPD protocol information
 *	    elogopt.rswtype	-   Remote system software type
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_sli_db		- CI PPD Common Sys Level Logging Info Database
 *
 *   SMP:	The SCA database is locked( EXTERNALLY ) for both traversal and
 *		modifications of the CI PPD common system level logging
 *		informational database, and to prevent SB deletion.
 *
 *		The PCCB is locked( EXTERNALLY ) as required by cippd_log_sys()
 *		in case logging becomes necessary.
 *
 *		SBs NEVER require locking.  Neither do SLIBs.
 */
void
cippd_csyslev( pccb, sb, event, option )
    PCCB			*pccb;
    register SB			*sb;
    register u_long		event;
    u_long			option;
{
    register cippd_slibq	*slib;
    register u_long		flag, i;
    static struct {
	u_long	event_code;
	u_long	event_flag;
    } event_to_flag[] = {
	{ W_BADSWTYPE,	SWTYPE	 },
	{ W_TALKUP,	TALKUP	 },
	{ E_TALKDOWN,	TALKDOWN },
	{ 0,		0	 }
	};

    /* Processing of common system level events proceeds as follows:
     *
     * 1. The common system level event code is verified.
     * 2. The CI PPD system level logging information database is searched for
     *    the SLIB corresponding to the remote system associated with the event
     *	  being processed.
     * 3. The event is processed according to whether its presence or absence
     *	  is being reported.
     *
     * A bitmask is retrieved if the common system level event code is
     * satisfactorily verified( Step 1 ).  This mask designates the bit within
     * the logging flags field of the appropriate SLIB which is to be set or
     * cleared according to whether the event being reported is present or
     * absent.  A panic occurs whenever code verification is presented with an
     * unknown common system level event and fails.
     *
     * Failure to retrieve a SLIB( Step 2 ) indicates that no common system
     * level events were previously associated with the system in question.
     * This is not a problem but represents the normal situation especially
     * when it is the absence of a event being reported.  However, when it is
     * the presence of an event being reported, failure to retrieve a SLIB
     * does trigger allocation of a new SLIB to represent the system in
     * question.  This SLIB is inserted into the SLIB database from which it is
     * retrieved during processing of subsequent event reports.
     *
     * Processing of event absences( Step 3 ) is dependent upon whether a SLIB
     * corresponding to the system in question was retrieved( Step 2 ).  If one
     * was not, event processing is immediately terminated.  This is the normal
     * and preferred outcome of event processing.  If a SLIB was retrieved then
     * the bit corresponding to the absent event within the the logging flags
     * field of the retrieved SLIB is cleared.  The SLIB is itself deallocated
     * whenever the corresponding system is discovered to be currently devoid
     * of all reported common system level events.  The SLIB is removed from
     * the SLIB database before deallocation.
     *
     * Processing of event presences( Step 3 ) involves determining whether
     * existence of the reported event was previously associated with the
     * system in question.  This is accomplished by checking the appropriate
     * bit within the logging flags field of the retrieved SLIB.  If the bit is
     * set then the event was previously reported and processing of the current
     * report is immediately terminated.  If the bit is not set then existence
     * of the event was not previously detected.  The bit is set to indicate
     * observation of the event, and the occurrence of the event is logged.
     *
     * NOTE: New SLIB allocations can fail.  Such failures do not substantially
     *	     affect event processing and event detections continue to be
     *	     logged.  What is affected is the keeping track of reported events
     *	     for the system in question.  All subsequent reports of any events
     *	     associated with the system result in event loggings.  This state
     * 	     persists until such time as a SLIB can be successfully allocated,
     *       even though such multiple loggings runs contrary to the goal of
     *	     common system level event processing.
     */
    for( flag = 0, i = 0; event_to_flag[ i ].event_code; i++ ) {
	if( event_to_flag[ i ].event_code == Mask_esevmod( event )) {
	    flag = event_to_flag[ i ].event_flag;
	    break;
	}
    }
    if( flag == 0 ) {
	( void )panic( PPDPANIC_UNKSLC );
    }
    for( slib = cippd_sli_db.flink;
	 slib != &cippd_sli_db;
	 slib = slib->flink ) {
	if( Comp_node( Slib->node_name, sb->sinfo.node_name )) {
	    break;
	}
    }
    if( slib == &cippd_sli_db ) {
	if( option == CLEAR ) {
	    return;
	} else {
	    KM_ALLOC( slib,
		      cippd_slibq *,
		      sizeof( CIPPD_SLIB ),
		      KM_SCA,
		      KM_NOW_CL_CA )
	    if( slib ) {
		Slib->size = sizeof( CIPPD_SLIB );
		Slib->type = DYN_CIPPDSLIB;
		Move_node( sb->sinfo.node_name, Slib->node_name )
		Insert_entry( Slib->flink, cippd_sli_db )
	    }
	}
    }
    if( option == CLEAR ) {
	if(( Slib->log_flags &= ~flag ) == 0 ) {
	    Remove_entry( Slib->flink )
	    KM_FREE(( char * )Slib, KM_SCA )
	}
    } else {
	if( slib == NULL || ( Slib->log_flags & flag ) == 0 ) {
	    if( slib ) {
		Slib->log_flags |= flag;
	    }
	    ( void )cippd_log_sys( pccb, sb, NULL, event );
	}
    }
}

/*   Name:	cippd_log_path	- Event Log CI PPD Path Specific Events
 *
 *   Abstract:	This routine logs CI PPD path specific events.  It also
 *		currently logs CI PPD remote port specific events.  All of
 *		these events are normally ascertained indirectly from a
 *		received CI PPD datagram or message.  The event is also
 *		optionally logged to the console.
 *
 *		Eight classes of events are currently logged by this routine:
 *
 *		1. Path establishments.
 *		2. Path establishment failures.
 *		3. Path failures due to local port failures.
 *		4. System-wide database conflicts.
 *		5. SCS and CI PPD protocol violations.
 *		6. Local SYSAP requests for path termination.
 *		7. Remote system requests for path termination.
 *		8. Remote system requests for path restarts( ie - breakage and
 *		   re-establishment of an existing path ).
 *
 *		Of all of these events only the system-wide database conflicts
 *		are considered CI PPD remote port specific events.  The
 *		remaining events logged by this routine are CI PPD path
 *		specific events.
 *
 *		Many of these events represent serious errors and are logged to
 *		save relevant information before drastic steps are attempted to
 *		resolve them.  Others are less serious and are logged only to
 *		give a warning or for informational purposes only.
 *
 *		NOTE: While all events logged therein arise indirectly from CI
 *		      PPD packets, the logging of each event does not
 *		      necessarily involve logging of the packet itself.
 *
 *		NOTE: Only packets with CI PPD headers( CI PPD datagrams or
 *		      messages ) may be passed to this routine.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      CI PPD with the exception of those SCS error and severe
 *		      error events which are candidates for application of the
 *		      path crash severity modifier( ESM_PC ).  It also does NOT
 *		      currently log any CI PPD events which are candidates for
 *		      application of the local port crash severity modifier(
 *		      ESM_LPC ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header( OPTIONAL )
 *   event			- Event code
 *   lscs			- Local system permanent information
 *   pb				- Path Block pointer( OPTIONAL )
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    elogopt.dbcoll	-   Database collision event information
 *          elogopt.port_num	-   Remote port number
 *				     ( required ONLY when PB not provided )
 *	    elogopt.sysapname	-   Name of local SYSAP
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.nerrs		-  Number of errors on port
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access and
 *		prevent premature PB deletion when a PB is provided.
 *
 *		PBs do NOT require locking when provided because only static
 *		fields are accessed.  SBs NEVER require locking.
 */
void
cippd_log_path( pccb, pb, cippdbp, event )
    PCCB			*pccb;
    PB				*pb;
    register GVPPPDH		*cippdbp;
    register u_long		event;
{
    register struct el_rec	*elp;
    register u_long		opt_size, size;
    register SB			*sb = (( pb ) ? pb->sb : NULL );
    register u_long		severity = Eseverity( event );

    /* The steps involved in logging CI PPD path specific and remote port
     * events include:
     *
     * 1. Logging the event to the console.
     * 2. Incrementing the counter of local port errors.
     * 3. Computing the size of the application portion of the event log
     *	  record.
     * 4. Allocating an event log record and initializing the record's sub id
     *	  packet fields.
     * 5. Initializing the portion of the record common to all CI PPD events.
     * 6. Initializing the portion of the record reserved for identifying
     *    specific CI PPD paths.
     * 7. Moving any optional information into the record.
     * 8. Validating the event log record.
     *
     * The ability of the CI PPD to log the event is validated during console
     * logging( Step 1 ).  A panic occurs whenever the CI PPD is not prepared
     * to log the reported event.  Note that console logging only validates the
     * ability of the CI PPD to log the event, NOT the ability of this routine
     * to correctly log it.  Therefore, while all attempts by this routine to
     * log events more properly logged by cippd_log_sys() are successful,
     * important information may be loss due to this routine's unpreparedness
     * to deal with it.
     *
     * Not all events increment the counter of errors which have occurred on
     * the specified local port( Step 2 ).  Only those events with severity
     * level equal to or greater than error( ES_E ) increment the counter.
     *
     * This routine immediately terminates on failure to allocate an event log
     * record( Step 4 ).
     *
     * The size of the application portion of each event log record( Step 3 )
     * is dependent upon the presence or absence of optional information to be
     * included within the record( Step 7 ).  The following types of mutually
     * exclusive optional information may be logged:
     *
     * 1. A CI PPD datagram/message.
     * 2. Information about known and remote systems participating in a
     *	  configuration database conflict.
     * 3. New path information.
     * 4. Local SYSAP name.
     *
     * The last three types of optional information are associated with only
     * single specific events( RE_DBCONFLICT, I_NEW_PATH, and E_SYSAP
     * respectively ).  Optional CI PPD datagrams and messages are associated
     * with many different events.  They also vary widely in size unlike other
     * types of optional information whose size is fixed.  Logged CI PPD
     * packets are truncated whenever logging their full size would exceed the
     * maximum size of an event log record.
     *
     * Processing of CI PPD path and remote port specific events is for the
     * most port identical.  Even that portion of the event log record reserved
     * for identifying specific CI PPD paths is initialized for remote port
     * specific events( step 6 ).  This is because while the event may be
     * remote port specific it is discovered only during the course of
     * attempting to establish a CI PPD path.  The only notable difference
     * between logging of these two types of events is the event type passed to
     * cippd_conlog(), the routine responsible for logging CI PPD events to the
     * console terminal( Step 1 ).
     *
     * NOTE: The new path information fields( structure cippd_newpath ) are
     *	     assummed to match up 1:1 with their corresponding SB system
     *	     information fields( SIB ).
     *
     * NOTE: Currently all events of remote error severity logged by this
     *	     routine are remote port specific events.  All other events logged
     *	     by this routine are path specific events.
     */
    ( void )cippd_conlog( pccb,
			  pb,
			  sb,
			  event,
			  (( Eseverity( event ) != ES_RE ) ? PATH_EVENT
							   : RPORT_EVENT ));
    if( Eseverity( event ) >= ES_E ) {
	Event_counter( pccb->lpinfo.nerrs )
    }
    if( cippd_errlog > severity && 
	!Test_cloverride( event ) &&
	cippd_errlog < SCA_ERRLOG3 ) {
	return;
    }
    size = sizeof( struct cippd_common ) + sizeof( struct cippd_pcommon );
    if( cippdbp ) {
	opt_size = sizeof( struct cippd_ppacket ) + Appl_size( cippdbp );
	if(( size += opt_size ) > EL_MAXAPPSIZE ) {
	    opt_size += ( EL_MAXAPPSIZE - size );
	    size = EL_MAXAPPSIZE;
	}
    } else {
	switch( Mask_esevmod( event )) {

	    case I_NEW_PATH:
		size += sizeof( struct cippd_newpath );
		break;

	    case E_SYSAP:
		size += NAME_SIZE;
		break;

	    case RE_DBCONFLICT:
		size += sizeof( struct cippd_dbcoll );
		break;
	}
    }
    if(( elp = ealloc( size, EL_PRIHIGH )) == EL_FULL ) {
	return;
    }
    LSUBID( elp,
	    ELSW_CIPPD,
	    EL_UNDEF,
	    pccb->lpinfo.type.hwtype,
	    Ctrl_from_name( pccb->lpinfo.name ),
	    EL_UNDEF,
	    event )
    Elcippdcommon( elp )->cippd_optfmask1 = ( CIPPD_CLTDEVTYP |
					      CIPPD_CLTDEVNUM |
					      CIPPD_PCOMMON );
    Elcippdcommon( elp )->cippd_optfmask2 = 0;
    Elcippdcommon( elp )->cippd_evpktver = CIPPD_EVPKTVER;
    Elcippdcommon( elp )->cippd_nerrs = pccb->lpinfo.nerrs;
    Move_scaaddr( lscs.system.sysid, *Elcippdcommon( elp )->cippd_lsysid )
    Move_node( lscs.system.node_name, Elcippdcommon( elp )->cippd_lname )
    if( sb ) {
	Elcippdcommon( elp )->cippd_npaths = sb->sinfo.npaths;
	Move_scaaddr( sb->sinfo.sysid, *Elcippdcommon( elp )->cippd_rsysid )
	Move_node( sb->sinfo.node_name, Elcippdcommon( elp )->cippd_rname )
    } else {
	Elcippdcommon( elp )->cippd_npaths = EL_UNDEF;
	U_short( Elcippdcommon( elp )->cippd_rsysid[ 0 ]) = EL_UNDEF;
	U_short( Elcippdcommon( elp )->cippd_rsysid[ 2 ]) = EL_UNDEF;
	U_short( Elcippdcommon( elp )->cippd_rsysid[ 4 ]) = EL_UNDEF;
	U_long( Elcippdcommon( elp )->cippd_rname[ 0 ]) = EL_UNDEF;
	U_long( Elcippdcommon( elp )->cippd_rname[ 4 ]) = EL_UNDEF;
    }
    U_long( *Elcippdpcommon( elp )->cippd_lpname ) = pccb->lpinfo.name;
    Move_scaaddr( pccb->lpinfo.addr, *Elcippdpcommon( elp )->cippd_lsaddr )
    if( pb ) {
	Elcippdpcommon( elp )->cippd_pstate = pb->pinfo.state;
	Move_scaaddr( pb->pinfo.rport_addr,
		      *Elcippdpcommon( elp )->cippd_rsaddr )
    } else {
	Elcippdpcommon( elp )->cippd_pstate = EL_UNDEF;
	if(( U_short( Elcippdpcommon( elp )->cippd_rsaddr[ 0 ])
		= pccb->Elogopt.port_num ) != EL_UNDEF ) {
	    U_short( Elcippdpcommon( elp )->cippd_rsaddr[ 2 ]) = 0;
	    U_short( Elcippdpcommon( elp )->cippd_rsaddr[ 4 ]) = 0;
	} else {
	    U_short( Elcippdpcommon( elp )->cippd_rsaddr[ 2 ]) = EL_UNDEF;
	    U_short( Elcippdpcommon( elp )->cippd_rsaddr[ 4 ]) = EL_UNDEF;
	}
    }
    if( cippdbp ) {
	( void )bcopy(( u_char * )&cippdbp->mtype,
		      ( u_char * )&Elcippdppacket( elp )->cippd_mtype,
		      opt_size );
	Elcippdcommon( elp )->cippd_optfmask1 |= CIPPD_PPACKET;
    } else {
	switch( Mask_esevmod( event )) {

	    case I_NEW_PATH:
		Elcippdcommon( elp )->cippd_optfmask1 |= CIPPD_NEWPATH;
		if( sb ) {
		    ( void )bcopy(( u_char * )&sb->sinfo.max_dg,
				  ( u_char * )Elcippdnewpath( elp ),
				  sizeof( struct cippd_newpath ));
		} else {
		    Elcippdnewpath( elp )->cippd_max_dg = EL_UNDEF;
		    Elcippdnewpath( elp )->cippd_max_msg = EL_UNDEF;
		    U_long( *Elcippdnewpath( elp )->cippd_swtype ) = EL_UNDEF;
		    U_long( *Elcippdnewpath( elp )->cippd_swver ) = EL_UNDEF;
		    U_long( Elcippdnewpath( elp )->cippd_swincrn[ 0 ])
								= EL_UNDEF;
		    U_long( Elcippdnewpath( elp )->cippd_swincrn[ 4 ])
								= EL_UNDEF;
		    U_long( *Elcippdnewpath( elp )->cippd_hwtype ) = EL_UNDEF;
		    U_long( Elcippdnewpath( elp )->cippd_hwver[ 0 ])
								= EL_UNDEF;
		    U_long( Elcippdnewpath( elp )->cippd_hwver[ 4 ])
								= EL_UNDEF;
		    U_long( Elcippdnewpath( elp )->cippd_hwver[ 8 ])
								= EL_UNDEF;
		}
		break;

	    case E_SYSAP:
		Move_name( pccb->Elogopt.sysapname, Elcippdsysapnam( elp ))
		Elcippdcommon( elp )->cippd_optfmask1 |= CIPPD_SYSAP;
		break;

	    case RE_DBCONFLICT:
		*Elcippddbcoll( elp ) = pccb->Elogopt.dbcoll;
		Elcippdcommon( elp )->cippd_optfmask1 |= CIPPD_DBCOLL;
		break;
	}
    }
    EVALID( elp )
}

/*   Name:	cippd_log_sys	- Event Log CI PPD System Specific Events
 *
 *   Abstract:	This routine logs CI PPD system specific events.  Such events
 *		are associated with specific remote systems and not with
 *		specific ports on those systems.  They are normally ascertained
 *		indirectly from a received CI PPD datagram or message.  The
 *		event is also optionally logged to the console.
 *
 *		Three classes of events are currently logged by this routine:
 *
 *		 1. CI PPD protocol mismatches.
 *		 2. Remote system error log packets.
 *		 3. Software detected unsupported remote system software types.
 *
 *		Some of these events represent serious errors and are logged to
 *		save relevant information before drastic steps are attempted to
 *		resolve them.  Others are less serious and are logged only to
 *		give a warning or for informational purposes only.
 *
 *		NOTE: While all events logged therein arise indirectly from CI
 *		      PPD packets, the logging of each event does not
 *		      necessarily involve logging of the packet itself.
 *
 *		NOTE: Only packets with CI PPD headers( CI PPD datagrams or
 *		      messages ) may be passed to this routine.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      CI PPD.  It also does NOT log any CI PPD event which is a
 *		      candidate for application of either the path( ESM_PC ) or
 *		      local port( ESM_LPC ) crash severity modifier.
 *
 *		NOTE: System specific events never increment the counter of
 *		      errors associated with any local port even when their
 *		      severity is error( ES_E ) or higher.  This is because the
 *		      event is not associated with a specific path to a remote
 *		      system; and therefore, can not be associated with any
 *		      specific local port.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header( OPTIONAL )
 *   event			- Event code
 *   lscs			- Local system permanent information
 *   sb				- System Block pointer
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    elogopt.protocol	-   CI PPD protocol information
 *	    elogopt.rswtype	-   Remote system software type
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access.
 *
 *		SBs NEVER require locking however they must EXTERNALLY be
 *		prevented from deletion by means of some other lock( such as a
 *		lock on the SCA database or on a PB representing a path to the
 *		system ).
 */
void
cippd_log_sys( pccb, sb, cippdbp, event )
    PCCB			*pccb;
    register SB			*sb;
    register GVPPPDH		*cippdbp;
    register u_long		event;
{
    register struct el_rec	*elp;
    register u_long		opt_size, size;
    register u_long		severity = Eseverity( event );

    /* The steps involved in logging system specific events include:
     *
     * 1. Logging the event to the console.
     * 2. Computing the size of the application portion of the event log
     *	  record.
     * 3. Allocating an event log record and initializing the record's sub id
     *	  packet fields.
     * 4. Initializing the portion of the record common to all CI PPD events.
     * 5. Initializing the portion of the record common to all CI PPD system
     *	  specific events.
     * 6. Moving any optional information into the record.
     * 7. Validating the event log record.
     *
     * The ability of the CI PPD to log the event is validated during console
     * logging( Step 1 ).  A panic occurs whenever the CI PPD is not prepared
     * to log the reported event.  Note that console logging only validates the
     * ability of the CI PPD to log the event, NOT the ability of this routine
     * to correctly log it.  Therefore, while all attempts by this routine to
     * log events more properly logged by cippd_log_path() are successful,
     * important information may be loss due to this routine's unpreparedness
     * to deal with it.
     *
     * This routine immediately terminates on failure to allocate an event log
     * record( Step 3 ).
     *
     * The size of the application portion of each event log record( Step 2 )
     * is dependent upon the presence or absence of optional information to be
     * included within the record( Step 6 ).  The following types of mutually
     * exclusive optional information may be logged:
     *
     * 1. A CI PPD datagram/message.
     * 2. Remote and local CI PPD mismatched protocol versions.
     *
     * The last type of optional information is associated with only certain
     * specific events( W_TALKUP and E_TALKDOWN ).  Optional CI PPD datagrams
     * and messages may be associated with many different events.  They also
     * vary widely in size unlike other types of optional information whose
     * size is fixed.  Logged CI PPD packets are truncated whenever logging
     * their full size would exceed the maximum size of an event log record.
     *
     * NOTE: The common system level information fields( structure
     *	     cippd_scommon ) are assummed to match up 1:1 with their
     *	     corresponding SB system information fields( SIB ).
     */
    ( void )cippd_conlog( pccb, NULL, sb, event, SYSTEM_EVENT );
    if( cippd_errlog > severity && 
	!Test_cloverride( event ) && 
	cippd_errlog < SCA_ERRLOG3 ) {
	return;
    }
    size = sizeof( struct cippd_common ) + sizeof( struct cippd_scommon );
    if( cippdbp ) {
	opt_size = sizeof( struct cippd_spacket ) + Appl_size( cippdbp );
	if(( size += opt_size ) > EL_MAXAPPSIZE ) {
	    opt_size += ( EL_MAXAPPSIZE - size );
	    size = EL_MAXAPPSIZE;
	}
    } else {
	switch( Mask_esevmod( event )) {

	    case W_TALKUP:
	    case E_TALKDOWN:
		size += sizeof( struct cippd_protocol );
		break;
	}
    }
    if(( elp = ealloc( size, EL_PRIHIGH )) == EL_FULL ) {
	return;
    }
    LSUBID( elp, ELSW_CIPPD, EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF, event )
    Elcippdcommon( elp )->cippd_optfmask1 = CIPPD_SCOMMON;
    Elcippdcommon( elp )->cippd_optfmask2 = 0;
    Elcippdcommon( elp )->cippd_evpktver = CIPPD_EVPKTVER;
    Elcippdcommon( elp )->cippd_nerrs = pccb->lpinfo.nerrs;
    Move_scaaddr( lscs.system.sysid, *Elcippdcommon( elp )->cippd_lsysid )
    Move_node( lscs.system.node_name, Elcippdcommon( elp )->cippd_lname )
    if( sb ) {
	Elcippdcommon( elp )->cippd_npaths = sb->sinfo.npaths;
	Move_scaaddr( sb->sinfo.sysid, *Elcippdcommon( elp )->cippd_rsysid )
	Move_node( sb->sinfo.node_name, Elcippdcommon( elp )->cippd_rname )
	( void )bcopy(( u_char * )&sb->sinfo.swtype,
		      Elcippdscommon( elp )->cippd_rswtype,
		      sizeof( struct cippd_scommon ));
    } else {
	Elcippdcommon( elp )->cippd_npaths = EL_UNDEF;
	U_short( Elcippdcommon( elp )->cippd_rsysid[ 0 ]) = EL_UNDEF;
	U_short( Elcippdcommon( elp )->cippd_rsysid[ 2 ]) = EL_UNDEF;
	U_short( Elcippdcommon( elp )->cippd_rsysid[ 4 ]) = EL_UNDEF;
	U_long( Elcippdcommon( elp )->cippd_rname[ 0 ]) = EL_UNDEF;
	U_long( Elcippdcommon( elp )->cippd_rname[ 4 ]) = EL_UNDEF;
	U_long( *Elcippdscommon( elp )->cippd_rswtype ) = EL_UNDEF;
	U_long( *Elcippdscommon( elp )->cippd_rswver ) = EL_UNDEF;
	U_long( Elcippdscommon( elp )->cippd_rswincrn[ 0 ]) = EL_UNDEF;
	U_long( Elcippdscommon( elp )->cippd_rswincrn[ 4 ]) = EL_UNDEF;
	U_long( *Elcippdscommon( elp )->cippd_rhwtype ) = EL_UNDEF;
	U_long( Elcippdscommon( elp )->cippd_rhwver[ 0 ]) = EL_UNDEF;
	U_long( Elcippdscommon( elp )->cippd_rhwver[ 4 ]) = EL_UNDEF;
	U_long( Elcippdscommon( elp )->cippd_rhwver[ 8 ]) = EL_UNDEF;
    }
    if( cippdbp ) {
	( void )bcopy(( u_char * )&cippdbp->mtype,
		      ( u_char * )&Elcippdspacket( elp )->cippd_mtype,
		      opt_size );
	Elcippdcommon( elp )->cippd_optfmask1 |= CIPPD_SPACKET;
    } else {
	switch( Mask_esevmod( event )) {

	    case W_TALKUP:
	    case E_TALKDOWN:
		*Elcippdprotocol( elp ) = pccb->Elogopt.protocol;
		Elcippdcommon( elp )->cippd_optfmask1 |= CIPPD_PROTOCOL;
	}
    }
    EVALID( elp )
}
