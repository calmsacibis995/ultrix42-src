#ifndef	lint
static char *sccsid = "@(#)cippd_protocol.c	4.1	(ULTRIX)	7/2/90";
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
 *		Driver( CI PPD ) finite state machine action functions and
 *		control routines.
 *
 *   Creator:	Todd M. Katz	Creation Date:	October 20, 1985
 *
 *   Function/Routines:
 *
 *   Control Routines
 *	cippd_dispatch		CI PPD Action Dispatcher
 *   Action Functions
 *	cippd_ack_rec		Discard Received ACK CI PPD Datagram
 *	cippd_build_sb		Build Formative System Block
 *	cippd_comp_trys		Compute Number of Transmission Attempts
 *	cippd_disb_path		Disable CI PPD Path
 *	cippd_enab_path		Enable CI PPD Path
 *	cippd_enter_db		Enter Path Block into Databases
 *	cippd_error_dg		Process Received ERROR CI PPD Datagram
 *	cippd_ignore_dg		Ignore CI PPD Datagram
 *	cippd_init_pb		Initialize a Path Block
 *	cippd_inv_cache		Invalidate Local Port Translation Cache
 *	cippd_panic		Unexpected State-Event Combination
 *	cippd_path_schd		Schedule Asynchronous CI PPD Path Clean Up
 *	cippd_pcreason		Map Path Crash Reason
 *	cippd_ppderror		Process CI PPD Protocol Errors
 *	cippd_req_id		Request Remote Port Identification
 *	cippd_rrestart		Process Remote CI PPD Path Restart Requests
 *	cippd_snd_ack		Send ACK CI PPD Datagram
 *	cippd_snd_stack		Send STACK CI PPD Datagram
 *	cippd_snd_start		Send START CI PPD Datagram
 *	cippd_snd_stop		Send STOP CI PPD Datagram
 *	cippd_start_tmr		Start CI PPD Traffic Interval Timer
 *	cippd_stop_dg		Process Received STOP CI PPD Datagram
 *	cippd_stop_tmr		Stop CI PPD Traffic Interval Timer
 *	cippd_test_trys		Test/Decrement Number of Transmission Attempts
 *	cippd_update_sb		Update Formative System Block
 *	cippd_upd_ptype		Update Hardware Port Type of Remote Port
 *
 *   Modification History:
 *
 *   14-Sep-1989	Pete Keilty
 *	1. Added bcmp instead of cast u_long for strings because of 
 *	   mips cpu's data alignment.
 *
 *   07-Sep-1989	Pete Keilty
 *	1. Added break in the scadb scan for loop of cippd_enter_db routine
 *	   on failure. This prevents Smp_unlock panic's lock not held.
 *	   On failure status is set to FAILURE so no need to continue 
 *	   scanning scadb.
 *
 *   06-Apr-1989	Pete Keilty
 *	1. Added include fiel smp_lock.h adn extern lock_t lk_scadb
 *
 *   17-Jan-1989	Todd M. Katz		TMK0005
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Include header file ../vaxmsi/msisysap.h.
 *
 *   17-Aug-1988        Todd M. Katz		TMK0004
 *	 1. The following informational events( ES_I ) have been redefined as
 *	    warning events( ES_W ): TALKUP, BADSWTYPE.
 *	 2. The following informational events( ES_I ) have been redefined as
 *	    remote error events( ES_RE ): DBCONFLICT, RERROR.
 *	 3. The following informational events( ES_I ) have been redefined as
 *	    error events( ES_E ): TALKDOWN, ALLOCFAIL, NORETRIES.
 *	 4. Modifications to cippd_enter_db():
 *		1) Allow DSSI tape storage controllers( software type == TFXX )
 *		   to pass remote system software verification.
 *		2) Event log all new path establishments.
 *	 5. Routine cippd_error() has been changed to function cippd_ppderror()
 *	    and is invoked only when a CI PPD protocol violation occurs on a
 *	    formative path.  It processes all violations by explicitly logging
 *	    them with an error event code of SE_PPDPROTOCOL and returning a
 *	    failure status to force clean up of the formative path.
 *	 6. Add a new routine cippd_rrestart().  This routine is invoked only
 *	    when a START CI PPD datagram is received on a fully established
 *	    path.  Such datagrams indicate remote CI PPD termination and clean
 *	    up of the path and an attempt by the remote CI PPD to re-establish
 *	    it.
 *	 7. No need exists to cache the remote port station address in the PCCB
 *	    before invoking cippd_log_path().  Modify the following routines
 *	    and functions appropriately: cippd_build_sb(), cippd_enab_path(),
 *	    and cippd_test_trys().
 *	 8. Modify cippd_stop_dg() to crash only fully established paths.  The
 *	    reception of a STOP CI PPD datagram over a formative path is logged
 *	    before path establishment is aborted by returning a failure status.
 *	 9. Modifications to cippd_pcreason() are as follows:
 *		1) It is now invocable only on established paths.
 *		2) The amount of error checking performed by it during mapping
 *		   has been greatly increased.  A panic occurs when an invalid
 *		   path crash code is encountered.
 *		3) All invocations of appropriate PD specific mapping routines
 *		   have been eliminated.  Port driver specific path crash
 *		   reason codes no longer exist.  All reason code mapping takes
 *		   place entirely within this routine.
 *      	4) It now handles both severe and "routine" path crash reasons
 *		   as required by the newly redefined sca error code formats.
 *	10. Modify cippd_comp_trys() to obtain port polling contact frequency
 *	    from a CI PPD specific PCCB field instead of from a CI PPD
 *	    configuration variable.
 *	11. Refer to error logging as event logging.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *      1. Modify cippd_build_sb() and cippd_enab_path() to event log instances
 *         of insufficient memory for new path establishment.
 *	2. Add logging of path establishment abortion due to exhausted retries.
 *	   This involves moving the check for exhausted retransmission attempts
 *	   from cippd_req_id(), cippd_snd_start(), and cippd_snd_stack() to a
 *	   new function, cippd_test_trys().  This new function determines
 *	   whether any retransmission attempts remain and aborts path
 *	   establishment when there are none.  This event is also logged
 *	   provided it is the first such failure to establish the path.
 *	   Logging of subsequent failures is bypassed until the CI PPD is
 *	   successful at establishing the path( cippd_enter_db() has been
 *	   appropriately modified ).
 *	3. Eliminate return of status by cippd_req_id(), cippd_snd_start(), and
 *	   cippd_snd_stack().  Formerly, these "routines" returned failure
 *	   status when unsuccessful at allocating a port specific buffer during
 *	   a retry attempt.  Such failures aborted path establishment.  Now
 *	   these routines just return on encountering an allocation failure.
 *	   The failure is not logged and path establishment is not aborted.
 *	   This change allows multiple attempts to be made to complete the
 *	   current step in the CI PPD path establishment sequence regardless of
 *	   intermediate failures.  Only when retries are exhausted is path
 *	   establishment aborted and the failure logged( by cippd_test_trys()).
 *	4. The following macros have been renamed:
 *		1) Test_mapbit() -> Test_lpinfomap()
 *		2) Set_mapbit() -> Set_lpinfomap()
 *		3) Clear_mapbit() -> Clear_lpinfomap()
 *	5. Eliminate the function cippd_chk_ver() and move its CI PPD protocol
 *	   checking function into cippd_enter_db().  Check for version
 *	   mismatches immediately after confirming that no database conflict
 *	   exists.  Also, modify cippd_enter_db() to log path establishments to
 *	   remote systems with unsupported system software types.  These
 *	   changes required extensive modifications to cippd_enter_db().
 *	6. CI PPD event logging is now split between the routines
 *	   cippd_log_path() and cippd_log_sys() to differentiate between
 *	   logging of path specific and system level events respectively.
 *	   Modify cippd_build_sb(), cippd_enab_path(), cippd_enter_db(), and
 *	   cippd_test_trys() to invoke cippd_log_path(); and, cippd_errorlog()
 *	   to invoke cippd_log_sys().
 *	7. Log the remote system's local port station address on database
 *	   conflicts( IE_DBCONFLICT ).
 *
 *   02-Jun-1988	Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   09-Apr-1988	Todd M. Katz		TMK0002
 *	1. Add comments to cippd_dispatch() concerning disposal of optional
 *	   port specific message buffers.  Such buffers are always disposed of
 *	   external to the CI PPD finite state machine and never within it.
 *	2. Add use of Pb_fork() macro in place of straight-line code.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made
 *	CI PPD and GVP completely independent from underlying port drivers,
 *	restructured code paths, and added SMP support.
 */

/*
 * 		Communication Interconnect Port-to-Port Driver
 * 
 * The CI PPD provides device independent paths for use as communication
 * conduits between equivalent SCSs on different systems.  The CI PPD is
 * responsible for the establishment and termination of these paths.  It is not
 * responsible for their maintenance.  That is the province of the port drivers
 * which monitor the physical status of their paths and notify the CI PPD when
 * a path has failed.  The CI PPD is also never involved with either SCS or
 * application communications.  Such communications are always handled directly
 * by the port drivers.
 *
 * Path establishment requires the CI PPD to communicate with equivalent CI
 * PPDs on other systems.  Such communication between peers utilizes a CI PPD
 * specific communication protocol consisting of a sequence of exchanged
 * datagrams triggered by the discovery of a previously unknown remote CI PPD.
 * The purpose of these datagram exchanges is for both participants to meet the
 * following criteria necessary for path establishment:
 *
 * 	1. Discovery of the remote CI PPD.
 *  	2. Transmission of local system information.
 * 	3. Reception of remote system information.
 * 	4. Transmission of an acknowledgement of remote system information.
 * 	5. Reception of an acknowledgement of local system information.
 *
 * The order in which these criteria are met may vary; and therefore, the
 * actual sequence of exchanged datagrams may also vary.  This is because the
 * path establishment CI PPD protocol is datagram based and the delivery of
 * datagrams is never guaranteed.  This requires a certain robustness to be
 * built into the path establishment protocol so that multiple attempts are
 * made to meet a particular criteria before aborting path establishment.
 * However, while the exact order of datagram exchanges may vary there are two
 * basic exchanges.
 *
 * 	        CI PPD 1		   	       CI PPD 2
 * 	-----------------------		------------------------------------
 *
 * 			Path Establishment Sequence 1
 *
 * 	Discovers Remote CI PPD
 * 	Transmits START -------------->
 * 					Discovers Remote CI PPD
 * 				<------	Transmits STACK
 * 	Transmits ACK ---------------->
 * 
 * 			Path Establishment Sequence 2
 * 
 * 	Discovers Remote CI PPD		Discovers Remote CI PPD
 * 	Transmits START ----------> <--	Transmits START
 * 	Transmits STACK ----------> <--	Transmits STACK
 * 	Transmits ACK ------------> <--	Transmits ACK
 * 
 * In the first sequence CI PPD 1 discovers CI PPD 2, transmits its system
 * information within a START CI PPD datagram, receives both remote system
 * information and an acknowledgement of its information within a STACK CI PPD
 * datagram, and transmits an acknowledgement of the remote information within
 * an ACK CI PPD datagram.  Meanwhile, CI PPD 2 receives remote system
 * information within a START, discovers CI PPD 1, transmits both its
 * information and an acknowledgement of the remote information within a STACK,
 * and receives acknowledgement of its information within an ACK.
 * 
 * In the second sequence both CI PPDs mirror their counterpart in what perhaps
 * is an overly redundant exchange of datagrams.  Both simultaneously discover
 * each other, transmit their system information within a START, receive their
 * counterpart's information also within a START, transmit an acknowledgement
 * of their counterpart's information within a STACK, receive an
 * acknowledgement of their information also within a STACK, and then transmit
 * a superfluous ACK for good measure.  All other path establishment sequences
 * are hybrids of these two basic sequences.
 * 
 * Discovery of the remote CI PPD is achieved by requesting the identification
 * of its remote port.  Such requests are accomplished in a port driver
 * specific fashion which may involve the underlying port's datagram
 * capabilities but never its guaranteed delivery mechanisms( if such exist ).
 * Two situations requiring remote port identification requests exist:
 *
 *	1. CI PPDs routinely poll all possible remote ports for their
 *	   identities.
 *	2. CI PPDs also request the identification of a specific remote port.
 *
 * The first situation represents how CI PPDs discover previously unknown
 * remote ports and the possible existence of their CI PPDs.  It is how CI PPD
 * 1 discovers CI PPD 2 in the first path establishment sequence.  The second
 * situation represents how a CI PPD discovers a remote port after receiving
 * its system information.  It is how CI PPD 2 discovers CI PPD 1 also in this
 * same sequence.  Polling( Situation 1 ) also provides one means by which
 * port drivers may monitor the physical state of their established paths.  
 * 
 * As mentioned, a certain robustness must be built into the path establishment
 * sequence because it is datagram based and datagrams are subject to both loss
 * and duplication.   The reception of duplicate datagrams is easily detected
 * and handled.  Overcomming the loss of datagrams presents a greater
 * challenge.  It is solved by the following retry mechanism based upon the
 * re-transmission of datagrams on expiration of path associated timers:
 *
 *	1. The appropriate PB interval timer is activated each time a datagram
 *	   is transmitted on a formative CI PPD path.
 *	2. Reception of the response expected for the datagram transmitted
 *	   deactivates this timer.
 *	3. Expiration of a path associated timer triggers re-transmission of
 *	   the last datagram transmitted on the formative path and re-activates
 *	   the timer.
 *
 * The number of re-transmissions made( Step 3 ) is not infinite, but computed
 * based upon a whole set of CI PPD and local port specific parameters.
 * Reception of certain acceptable but NOT expected datagrams can force
 * re-computation of this number prior to datagram re-transmission and timer
 * re-activation.  Expiration of the number of retries remaining terminates
 * path establishment and forces clean of the formative CI PPD.
 *
 * Path associated interval timers are not activated for every datagram
 * transmitted by the CI PPD, but only for those datagram transmissions for
 * which specific responses are expected.  This includes START and STACK CI PPD
 * datagram transmissions and specific requests for remote port
 * identifications.  It does not include routine CI PPD polling or CI PPD ACK
 * datagram transmissions, neither of which expect specific responses.
 *
 * Path termination is done unilaterally by each CI PPD, unlike path
 * establishment.  It does not require the CI PPD to notify its counterparts.
 * There is no path termination CI PPD protocol sequence. 
 * 
 * The CI PPD is implemented as a finite state machine.  Associated with each
 * path is its path state.  When an asynchronous event occurs on a path its
 * state dictates what actions are taken.  It also represents the path's
 * current position within a CI PPD protocol sequence.  Therefore, certain
 * events trigger transitioning of paths into different path state.  The
 * following table illustrates for each possible path state - event combination
 * the actions which are taken and the concluding path state:
 *
 *  Path State	    EVENT		   ACTION		    Path State
 * ------------	  ----------	-----------------------------	   ------------
 * CLOSED	  ID_REC	Allocate and Initialize PB	   START_SNT
 *				Send START CI PPD Datagram
 *				Start CI PPD Timer
 *		  START_REC	Allocate and Initialize PB	   START_REC
 *				Allocate and Initialize SB
 *	 			Request Remote Identification
 *				Start CI PPD Timer
 *		  STACK_REC	Discard Datagram			-
 *		  ACK_REC	Discard Datagram			-
 *		  SCSMSG_REC	Disable CI PPD Path			-
 *		  ERROR_REC	Discard Datagram			-
 *		  STOP_REC	Discard Datagram			-
 *		  TIMEOUT	Invalid State-Event Combination		-
 *		  PATH_FAIL	Invalid State-Event Combination		-
 *		  PROC_ERROR	Discard Datagram		   PATH_FAILURE
 *				Send STOP CI PPD Datagram
 *				Schedule Asynchronous PB Clean Up
 *
 * START_SNT	  ID_REC	Discard Datagram			-
 *		  START_REC	Stop CI PPD Timer		   STACK_SNT
 *				Allocate and Initialize SB
 *				Update PB
 *				Enable CI PPD Path
 *				Send STACK CI PPD Datagram
 *				Start CI PPD Timer
 *		  STACK_REC	Stop CI PPD Timer		   OPEN
 *				Allocate and Initialize SB
 *				Update PB
 *				Enable CI PPD Path
 *				Enter PB and SB into Databases
 *				Send ACK CI PPD Datagram
 *		  ACK_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Abort CI PPD Path Establishment
 *		  SCSMSG_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Abort CI PPD Path Establishment
 *		  ERROR_REC	Event log ERROR CI PPD Datagram		-
 *				Discard Datagram
 *		  STOP_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Abort CI PPD Path Establishment
 *		  TIMEOUT	Send START CI PPD Datagram		-
 *				Start CI PPD Timer
 *		  PATH_FAIL	Stop CI PPD Timer		   PATH_FAILURE
 *				Send STOP CI PPD Datagram
 *				Schedule Asynchronous PB Clean Up
 *		  PROC_ERROR	Discard Optional Datagram	   PATH_FAILURE
 *				Disable CI PPD Path
 *				Send STOP CI PPD Datagram
 *				Schedule Asynchronous PB Clean Up
 *
 * START_REC	  ID_REC	Stop CI PPD Timer		   STACK_SNT
 *				Initialize and Update PB
 *				Enable CI PPD Path
 *				Send STACK CI PPD Datagram
 *				Start CI PPD Timer
 *		  START_REC	Stop CI PPD Timer			-
 *				Update SB
 *	 			Request Remote Identification
 *				Start CI PPD Timer
 *		  STACK_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Abort CI PPD Path Establishment
 *		  ACK_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Abort CI PPD Path Establishment
 *		  SCSMSG_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Disable CI PPD Path
 *				Abort CI PPD Path Establishment
 *		  ERROR_REC	Event log ERROR CI PPD Datagram		-
 *				Discard Datagram
 *		  STOP_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Abort CI PPD Path Establishment
 *		  TIMEOUT	Request Remote Identification		-
 *				Start CI PPD Timer
 *		  PATH_FAIL	Stop CI PPD Timer		   PATH_FAILURE
 *				Send STOP CI PPD Datagram
 *				Schedule Asynchronous PB Clean Up
 *		  PROC_ERROR	Discard Optional Datagram	   PATH_FAILURE
 *				Send STOP CI PPD Datagram
 *				Schedule Asynchronous PB Clean Up
 *
 * STACK_SNT	  ID_REC	Discard Datagram			-
 *		  START_REC	Stop CI PPD Timer			-
 *				Update SB
 *				Send STACK CI PPD Datagram
 *				Start CI PPD Timer
 *		  STACK_REC	Stop CI PPD Timer		   OPEN
 *				Update SB
 *				Enter PB and SB into Databases
 *				Send ACK CI PPD Datagram
 *		  ACK_REC	Stop CI PPD Timer		   OPEN
 *				Enter PB and SB into Databases
 *				Discard Datagram
 *		  SCSMSG_REC	Stop CI PPD Timer		   OPEN
 *				Enter PB and SB into Databases
 *		  ERROR_REC	Event log ERROR CI PPD Datagram		-
 *				Discard Datagram
 *		  STOP_REC	Stop CI PPD Timer		   PATH_FAILURE
 *				Abort CI PPD Path Establishment
 *		  TIMEOUT	Send STACK CI PPD Datagram		-
 *				Start CI PPD Timer
 *		  PATH_FAIL	Stop CI PPD Timer		   PATH_FAILURE
 *				Disable CI PPD Path
 *				Send STOP CI PPD Datagram
 *				Schedule Asynchronous PB Clean Up
 *		  PROC_ERROR	Discard Optional Datagram	   PATH_FAILURE
 *				Disable CI PPD Path		
 *				Send STOP CI PPD Datagram
 *				Schedule Asynchronous PB Clean Up
 *
 * OPEN		  ID_REC	Discard Datagram			-
 *		  START_REC	Crash CI PPD Path		   PATH_FAILURE
 *				Discard Datagram
 *		  STACK_REC	Update SB				-
 *				Send ACK CI PPD Datagram
 *		  ACK_REC	Discard Datagram			-
 *		  SCSMSG_REC		-				-
 *		  ERROR_REC	Event log ERROR CI PPD Datagram		-
 *				Discard Datagram
 *		  STOP_REC	Crash CI PPD Path		   PATH_FAILURE
 *				Discard Datagram
 *		  TIMEOUT	Invalid State-Event Combination		-
 *		  PATH_FAIL	Disable CI PPD Path		   PATH_FAILURE
 *				Send STOP CI PPD Datagram
 *				Invalidate Translation Cache
 *				Schedule Asynchronous PB Clean Up
 *		  PROC_ERROR	Invalid State-Event Combination		-
 *
 * PATH_FAILURE	  ID_REC	Discard Datagram			-
 *		  START_REC	Send STOP CI PPD Datagram		-
 *				Discard Datagram
 *		  STACK_REC	Send STOP CI PPD Datagram		-
 *				Discard Datagram
 *		  ACK_REC	Send STOP CI PPD Datagram		-
 *				Discard Datagram
 *		  SCSMSG_REC	Send STOP CI PPD Datagram		-
 *		  ERROR_REC	Event log ERROR CI PPD Datagram		-
 *				Discard Datagram
 *		  STOP_REC	Discard Datagram			-
 *		  TIMEOUT	Invalid State-Event Combination		-
 *		  PATH_FAIL		-				-
 *		  PROC_ERROR	Invalid State-Event Combination		-
 *
 * This table is presented to give an overall picture of the finite state
 * machine.  It is by no means complete.  Some of the excluded actions are
 * just unimportant as is the case with certain ancillary ones.  Others
 * include tests which exist only to potentially divert the flow of control
 * into secondary error paths if various criteria are not met.  Only those
 * actions which illustrate the primary flow of control and whose presence
 * contributes to the overall picture were included within the table.
 *
 * Official documentation of the CI PPD protocol is included within the
 * corporate SCA architectural specification.  A comparison between this
 * specification and the table documented above would reveal several marked
 * discrepancies, the most important of which is the lack of a START_REC state
 * in the latter.  This state was added to the Ultrix implementation of the
 * CI PPD protocol to both increase the protocol's robustness and reduce the
 * amount of time required for synchronizing local and remote CI PPDs during
 * path establishment.  Its presence does not violate the official protocol
 * as it is a purely local CI PPD convention.  Its benignity also has been
 * firmly established by co-existence of Ultrix, VMS, and HSC implementations
 * on the same CI.
 *
 * Many of the actions executed by the finite state machine in its processing
 * of events have no resultant status and can not affect the flow of control
 * within the machine.  Others do have a resultant status including both
 * actions represented in the table above and some that are not.  These actions
 * can affect the flow of control within the finite state machine by
 * prematurely terminating processing of the current event whenever their
 * outcome is not successful.
 *
 * Premature termination of event processing has one major designed side
 * effect: it also terminates the path on which the event originally occurred.
 * One consequence of this outcome is that event processing on established or
 * failed paths may never be terminated.  Failed paths have already been
 * terminated and established paths may only be terminated by explicit crashing
 * of the path.  Only event processing on formative paths can be terminated.
 * In fact, premature termination of event processing represents the only
 * supported mechanism for crashing formative paths and aborting their
 * establishment from within the finite state machine itself.  Formative paths
 * may be explicitly crashed external to the finite state machine, but they
 * may never be explicitly crashed from within it.  This insures that formative
 * paths are correctly aborted.
 *
 * All actions responsible for prematurely terminating event processing and
 * aborting establishment of the formative path are also responsible for
 * logging the reason for such termination.  Only the very first occurrence of
 * a specific reason for aborting establishment of a specific formative path is
 * logged.  Logging of subsequent failures is bypassed until such time as the
 * condition responsible has been satisfactorily resolved for the specific
 * path.
 *
 * Once an action returns a failure status, actual termination of event
 * processing and abortion of path establishment is relatively straightforward.
 * The finite state machine declares a special event( event == CNFE_PROC_ERROR)
 * on the path to be aborted by iteratively invoking itself.  The path state
 * remains the same because processing of the previous event was prematurely
 * terminated before the path could be transitioned to its new state.  It is
 * during processing of this special event that establishment of the formative
 * path is aborted by disabling the path, scheduling its asynchronously clean
 * up, and transitioning its state to PS_PATH_FAILURE.
 *
 * A number of general rules exist which are quite helpful in understanding the
 * overall design of the CI PPD finite state machine and the machine's
 * implementation of the CI PPD protocol.  These rules are listed below:
 *
 *  1. Entry to the CI PPD finite state machine is by means of the function
 *     cippd_dispatch() and only this function.
 *  2. Each action in the table above is represented by its own finite state
 *     machine action function/routine.  Functions return status while routines
 *     do not.  All action functions/routines have the exact same interface.  
 *  3. The CI PPD finite state machine executes in turn all actions associated
 *     with the current path state - event pairing.  On successful invocation
 *     of all actions the path is transitioned into its new state.  Failure of
 *     any action terminates processing of the current event and iteratively
 *     invokes the finite state machine( event == CNFE_PROC_ERROR ) to abort
 *     establishment of the path.
 *  4. Action functions executed while processing established CI PPD paths must
 *     never return failure.  This would trigger iterative invocation of the
 *     finite state machine( event == CNFE_PROC_ERROR ) for the purpose of
 *     aborting the path.  This is not allowed.  Established paths can only be
 *     terminated through explicit invocation of cippd_crash_pb().  This
 *     insures that they are terminated correctly.
 *  5. Action functions executed while processing failed paths( path state ==
 *     PS_PATH_FAILURE ) or either process error( event == CNFE_PROC_ERROR ) or
 *     path failure( event == CNFE_PATH_FAIL ) events must never return
 *     failure.  This would trigger termination of paths that are already in
 *     the process of being terminated.
 *  6. The functions cippd_pcreason, cippd_disb_path, and cippd_path_sched may
 *     not be included among the action functions executed while processing
 *     failed paths( path state == PS_PATH_FAILURE ).  Such functions may only
 *     be executed once per path incarnation and were invoked during processing
 *     of the path failure event itself( event == CNFE_PATH_FAIL ).  In
 *     addition, the function cippd_pcreason() may only be executed on
 *     established paths.  Only the path crash reasons associated with such
 *     paths require mapping.
 *  7. The status returned by certain action functions may be ignored when the
 *     function is invoked during processing of certain path state - event
 *     combinations.  It may be ignored either when the status is guaranteed to
 *     be successful under the current circumstances or must be successful
 *     because of the current event or path state( See Rules 4 and 5 ).
 *  8. Datagrams are optional in all calls to the finite state machine.  For
 *     example, datagrams are never supplied when the machine is invoked to
 *     process path failures( event == CNFE_PATH_FAIL ) or processing errors(
 *     event == CNFE_PROC_ERROR ).
 *  9. Datagrams must never be disposed of as a side effect of executing some
 *     action, but only by those action functions/routines which exist solely
 *     for this purpose.  They are also purposely deallocated during abortion
 *     of formative paths following premature termination of event processing.
 * 10. SCS sequenced messages must never be deallocated by the CI PPD finite
 *     state machine even when their processing leads to failure of the target
 *     path.  They are always returned to the original invocators of the finite
 *     state machine who have the proper contexts to dispose of them correctly.
 * 11. The finite state machine never explicitly crashes formative paths.  An
 *     action function wishing to crash a formative path returns a failure
 *     status.  Such a status prematurely terminates event processing and
 *     aborts establishment of the formative path.
 *	
 *
 * These basic rules must be careful obeyed when modifying the CI PPD finite
 * state machine, especially when changing existing action functions/routines.
 * Action routines should not be changed to return status and the conditions
 * under which failure statuses are returned by action functions should not be
 * modified without a thorough understanding of the consequences.
 * 
 * Only CI PPD path related asynchronous events processed directly by the CI
 * PPD finite state machine are documented by the CI PPD state table.  There
 * are other events important to the CI PPD but not processed directly by the
 * finite state machine including:
 * 
 * 	1. Asynchronous notification of port starting.
 * 	2. Asynchronous notification of port failure.
 * 	3. CI PPD interval timer events.
 *	4. Asynchronous path clean up.
 * 
 * The first event activates the finite state machine on the specified local
 * port and schedules its first interval timer event.  The second event aborts
 * the establishment of formative paths, terminates existing paths, cancels all
 * interval timers, and in general deactivates the finite state machine on the
 * specified local port.  In between local port start up and failure, interval
 * timer events drive the discovery of new remote CI PPDs through polling for
 * their remote port identifications.  It is also through these events that the
 * timeouts listed in the CI PPD state table are declared on specific formative
 * paths.  Finally, the last event in the list is scheduled by the CI PPD
 * finite state machine itself and its asynchronous occurrence cleans up the
 * specified CI PPD path.
 *
 * NOTE: The action "Send STOP CI PPD Datagram" is specified for many of the
 *	 event-state combinations listed in the CI PPD state table.  Yet, it is
 *	 also true that the local SYSAP never notifies its remote counterpart
 *	 of local termination of a path.  This apparent discrepancy requires
 *	 further clarification.  The DEC standard SCA CI PPD protocol specifies
 *	 a HOST SHUTDOWN CI PPD datagram.  An attempt was made by Ultrix to
 *	 redefine this datagram as a more general STOP CI PPD datagram.  While
 *	 this attempt was in progress and in anticipation of its success, the
 *	 CI PPD was modified to send this datagram whenever it terminated a
 *	 path.  Needless to say the attempt to ECO the SCA architecture failed
 *	 mainly due to an extremely pig headed VMS programmer( who shall remain
 *	 nameless because he does not even deserve to be mentioned ).  Anyway,
 *	 to conform to the SCA architecture Ultrix was forced to abandon use of
 *	 the STOP CI PPD datagram.  This was accomplished by just commenting
 *	 out the action function responsible for its transmission.  Attempts to
 *	 transmit a STOP CI PPD datagram were left in place as were all
 *	 comments on when such a datagram should be transmitted.  Just nothing
 *	 is transmitted.  This strategy allows for quick resumption of STOP CI
 *	 PPD datagram transmission should the situation change and a modified
 *	 version of the STOP CI PPD datagram was accepted by the SCA
 *	 architecture group.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#include		"../h/time.h"
#include		"../h/ksched.h"
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
extern	SCSIB		lscs;
extern	struct lock_t	lk_scadb;
extern	sbq		scs_config_db;
extern	struct timeval	boottime, time;
extern	PB		*cippd_get_pb();
extern	SB		*scs_alloc_sb();
extern	u_long		cippd_dispatch();
extern	u_short		cippd_itime, cippd_map_pc[], cippd_map_spc[],
			scs_map_pc[], scs_map_spc[];
extern	void		cippd_clean_pb(), cippd_clean_fpb(), cippd_crash_pb(),
			cippd_csyslev(), cippd_log_path(), cippd_log_sys(),
			scs_dealloc_pb(), scs_dealloc_sb(), scs_new_path(),
			scs_unix_to_vms();
extern	CIPPD_STAB	cippd_stab[ PS_MAX_STATE + 1 ][ CNFE_MAX_EVENT + 1];

/*   Name:	cippd_dispatch	- CI PPD Action Dispatcher
 *
 *   Abstract:	This function dispatches the current event according to the
 *		current path state event.  There are many possible side effects
 *		from such dispatching and the CI PPD state table at the
 *	 	beginning of this module should be consulted for an
 *		understanding of them.
 *
 *		Many of the event-state combinations represent errors and
 *		directly abort path establishment through explicit crashing of
 *		the path.  The path is also terminated when errors are
 *		encountered during processing of the current event.  Several
 *		event-state combinations may also log remote errors.
 *
 *		Any optional port specific datagram buffer provided is
 *		consumed by the action of the finite state machine either
 *		through transmission or addition to the appropriate local port
 *		free queue.
 *
 *		Any optional port specific message buffer provided is NOT and
 *		must not be consumed by the action of the finite state machine.
 *		Disposal of such buffers always occurs outside the context of
 *		the CI PPD finite state machine.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address CI PPD header in datagram( OPTIONAL )
 *   cippd_stab			- CI PPD State Transition table
 *   event			- Event
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pinfo.reason		-  General path failure reason
 *	pinfo.state		-  Path state
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- State transition successfully occurred
 *   RET_FAILURE		- Path establishment aborted
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents within the finite state machine.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
u_long
cippd_dispatch( pccb, pb, event, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    u_long		event;
    GVPPPDH		*cippdbp;
{
    register CIPPD_ATAB		*ap;
    register u_long		state;

    /* Dispatch the current event by executing the set of actions on the target
     * path appropriate for the event - path state combination.  Successful
     * execution of all actions transitions the CI PPD path to a new state as
     * specified by the CI PPD state table.  Failure of any action in the set
     * is handled as follows by this function:
     *
     * 1. Optionally deallocate the CI PPD datagram buffer( it must be present
     *    and it must BE a datagram and not a SCS message buffer ).
     * 2. Store the reason for terminating the path for latter SYSAP( remote )
     *	  consumption.
     * 3. Iteratively invoke the finite state machine to terminate and clean up
     *	  the path( event == CNFE_PROC_ERROR ).
     * 4. Return a failure status.
     *
     * There is never any need to either terminate established( path state ==
     * PS_OPEN ) or failed( path state == PS_PATH_FAILURE ) paths in this
     * fashion.  Execution of their actions is always successful.  Furthermore,
     * execution of actions associated with path failure( event ==
     * CNFE_PATH_FAIL ) or processing error( event == CNFE_PROC_ERROR ) events
     * is also always successful protecting paths undergoing termination from
     * repeated terminations.
     *
     * There is one exception to this rule.  The action function
     * cippd_enter_db() must temporarily release all locks.  By the time it re
     * obtains its PB lock it is possible for the corresponding path to have
     * failed, but not  cleaned up, and for its state to have been transitioned
     * to PS_PATH_FAILURE.  This causes cippd_enter_db() to immediately return
     * failure and this function to iteratively invoke itself to terminate the
     * path( event == CNFE_PROC_ERROR ).  Thus, under such obscure
     * circumstances it is possible for the finite state machine to be invoked
     * to handle processing errors on paths that have already failed.  The
     * finite state machine is prepared for such occurrences by completely
     * ignoring them and performing no processing as is done for invocations of
     * the finite state machine to handle failure of already failed paths.
     *
     * Each action executed during processing of the current event is
     * represented by its own function/routine.  Functions return status while
     * routines do not.  Function status is occasionally ignored whenever it
     * is known to always be good under the current set of circumstances.  The
     * interface to all functions/routines is consistent allowing them to be
     * invoked with regard to only whether or not they return status.
     */
    if(( state = pb->pinfo.state ) > PS_MAX_STATE ) {
	( void )panic( PPDPANIC_PSTATE );
    } else if( event > CNFE_MAX_EVENT ) {
	( void )panic( PPDPANIC_EVENT );
    }
    for( ap = cippd_stab[ state ][ event ].actions; ap->action; ap++ ) {
	if( ap->flags == CIPPD_NOSTATUS ) {
	    ( void )( *ap->action )( pccb, pb, cippdbp );
	} else if(( *ap->action )( pccb, pb, cippdbp ) != RET_SUCCESS ) {
	    if( cippdbp && ( event != CNFE_SCSMSG_REC )) {
		( void )( *pccb->Add_dg )( pccb, Ppd_to_scs( cippdbp ));
	    }
	    pb->pinfo.reason = PF_PPDPROTOCOL;
	    ( void )cippd_dispatch( pccb, pb, CNFE_PROC_ERROR, NULL );
	    return( RET_FAILURE );
	}
    }
    pb->pinfo.state = cippd_stab[ state ][ event ].fstate;
    return( RET_SUCCESS );
}

/*   Name:	cippd_ack_rec	- Discard Received ACK CI PPD Datagram
 *
 *   Abstract:	This action routine discards received ACK CI PPD datagrams by
 *		deallocating them.  It is only invoked when the reception of an
 *		ACK transitions the formative path into a fully established
 *		path.  The datagrams are deallocated because they represent
 *		those datagrams temporarily allocated for all protocol
 *		exchanges during path establishment when such establishment was
 *		originally initiated.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *	pinfo.state		-  PS_STACK_SNT
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized( The PCCB is locked EXTERNALLY anyway ).
 */
void
cippd_ack_rec( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    ( void )( *pccb->Dealloc_dg )( pccb, Ppd_to_scs( cippdbp ));
}

/*   Name:	cippd_build_sb	- Build Formative System Block
 *
 *   Abstract:	This action function builds a formative System Block from a
 *		START/STACK CI PPD datagram.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	sb			- Address of formative System Block
 *	pinfo.protocol		- Remote systems CI PPD protocol version
 *   pccb			- Port Command and Control Block pointer
 *      ppd.cippd               -  CI PPD specific PCCB fields
 *          aflogmap            -   Path establishment insufficient mem log map
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Formative System Block successfully built
 *   RET_FAILURE		- Failed to allocate a System Block
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents, postponing premature PB deletion, and as
 *		required by cippd_log_path() in case logging becomes necessary.
 *
 *   		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
u_long
cippd_build_sb( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register SB		*sb;
    register u_long	status = RET_SUCCESS;
    register CIPPD_START *bp = Cippd_start( Ppd_to_scs( cippdbp ));

    /* Insufficient memory for SB allocation aborts new path establishment.
     * Only the first such failure to establish a specific path is logged.
     * Logging of all subsequent failures is bypassed until after the CI PPD is
     * successful at allocating all required memory for establishment of the
     * specific path( see cippd_enab_path()).
     */
    if(( sb = pb->sb = scs_alloc_sb(( SIB * )bp ))) {
	pb->pinfo.protocol = bp->protocol;
	sb->sinfo.npaths = 0;
    } else {
	if( !Test_map( Aflogmap, Scaaddr_low( pb->pinfo.rport_addr ))) {
	    Set_map( Aflogmap, Scaaddr_low( pb->pinfo.rport_addr ))
	    ( void )cippd_log_path( pccb, pb, cippdbp, E_ALLOCFAIL );
	}
	status = RET_FAILURE;
    }
    return( status );
}

/*   Name:	cippd_comp_trys	- Compute Number of Transmission Attempts
 *
 *   Abstract:	This action routine computes the maximum number of attempts
 *		made by the finite state machine to transmit a CI PPD protocol
 *		packet.  The maximum number of attempts is computed as follows:
 *
 *		tries = ( port contact frequency ) / ( port polling frequency )
 *
 *		The current port polling interval is used as the port polling
 *		frequency.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_itime		- CI PPD port timer interval
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    retry		-   Retry count
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
void
cippd_comp_trys( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    pb->Retry = ((( pccb->Contact >= cippd_itime ) ? pccb->Contact
						   : cippd_itime ) /
			pccb->Poll_interval );
}

/*   Name:	cippd_disb_path	- Disable CI PPD Path
 *
 *   Abstract:	This action routine directs the disablement of a CI PPD path.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram/message
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus		-   Finite State Machine path status flags
 *		path_closed	-    1
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by PD functions
 *		which close virtual circuits.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion,
 *		allowing exclusive access to PB contents, and as required by
 *		PD functions which close virtual circuits.
 */
void
cippd_disb_path( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    GVPPPDH		*cippdbp;
{
    /* Disable the path only if it has not already been disabled by the
     * appropriate port driver.
     */
    if( !pb->Fsmpstatus.path_closed ) {
	( void )( *pccb->Set_circuit )( pccb,
					pb,
					Scaaddr_low( pb->pinfo.rport_addr ),
					SET_VC_CLOSE );
	pb->Fsmpstatus.path_closed = 1;
    }
}

/*   Name:	cippd_enab_path	- Enable CI PPD Path
 *
 *   Abstract:	This action function directs the enablement of a CI PPD path.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus		-   Finite State Machine path status flags
 *		path_closed	-    1
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus		-   Finite State Machine path status flags
 *		path_closed	-    Path already closed by port status bit
 *   pccb			- Port Command and Control Block pointer
 *      ppd.cippd               -  CI PPD specific PCCB fields
 *          aflogmap            -   Path establishment insufficient mem log map
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- CI PPD path is enabled
 *   RET_ALLOCFAIL		- Unable to allocate port specific buffer
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents, postponing premature PB deletion, and as
 *		required both by PD functions which open virtual circuits and
 *		by cippd_log_path() in case logging becomes necessary.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion,
 *		allowing exclusive access to PB contents, and as required by
 *		PD functions which open virtual circuits.
 */
u_long
cippd_enab_path( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	status;

    /* Insufficient memory for port specific buffer allocation is the only
     * reason for failure to enable a specific path.  This failure aborts new
     * path establishment and is logged.  Only the first such failure to
     * establish a specific path is logged.  Logging of all subsequent failures
     * is bypassed until after the CI PPD is successful at allocating all
     * required memory for establishment of the specific path( which occurs
     * with successful invocation of this very function ).
     */
    if( pb->Fsmpstatus.path_closed ) {
	status = ( *pccb->Set_circuit )( pccb,
					 pb,
				 	 Scaaddr_low( pb->pinfo.rport_addr ),
					 SET_VC_OPEN );
	if( status == RET_SUCCESS ) {
	    pb->Fsmpstatus.path_closed = 0;
	    Clear_map( Aflogmap, Scaaddr_low( pb->pinfo.rport_addr ))
	} else {
	    if( !Test_map( Aflogmap, Scaaddr_low( pb->pinfo.rport_addr ))) {
		Set_map( Aflogmap, Scaaddr_low( pb->pinfo.rport_addr ))
		( void )cippd_log_path( pccb, pb, NULL, E_ALLOCFAIL );
	    }
	}
    } else {
	( void )panic( PPDPANIC_POPEN );
    }
    return( status );
}

/*   Name:	cippd_enter_db	- Enter Path Block into Databases
 *
 *   Abstract:	This action function enters the PB representing the formative
 *		CI PPD path into the system-wide local port and configuration
 *		databases.  The formative SB is also entered into the
 *		system-wide configuration database provided the system is not
 *		currently known; otherwise, the formative SB is deallocated.
 *
 *		Existence of the new path is event logged and brought to the
 *		attention of SCS.
 *
 *		NOTE: No action function invoked following the invocation of
 *		      this one may ever return a failure status.  This is
 *		      because for all practical purposes the path is fully
 *		      established once this routine enters the PB into the
 *		      databases( see rule 4 at front of this module ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *   lk_scadb			- SCA database lock structure
 *   scs_config_db		- System-wide configuration database queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	flink			-  Database queuing pointers
 *	sb			-  SB pointer
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.ppd.cippd	-  CI PPD specific local port information
 *	    dbclogmap		-   Database conflict port logging bitmap
 *	    nform_paths		-   Current number of formative paths
 *	    npaths		-   Current number of paths
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    elogopt.dbcoll	-   Database collision error information
 *	    elogopt.protocol	-   CI PPD protocol information
 *	    elogopt.rswtype	-   Remote system software type
 *	    form_pb		-   Formative PB queue
 *	    open_pb		-   PS_OPEN PB vector table
 *          tmologmap           -   Path establishment timeout log map
 *   sb				- System Block pointer
 *	flink			-  Database queuing pointers
 *	pbs			-  PB queue
 *	sinfo.npaths		-  Current number of paths
 *   scs_config_db		- System-wide configuration database queue
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Path Block entered into databases
 *   RET_FAILURE		- Conflict discovered within a database
 *
 *   SMP:	The SCA database is locked INTERNALLY whenever it was not
 *		locked EXTERNALLY prior to function invocation.  It is locked
 *		for traversing the system-wide configuration database and
 *		updating it with the formative PB and occasionally the
 *		formative SB.  It is also required by cippd_csyslev() in case
 *		common system level event logging becomes necessary.
 *
 *		The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents, postponing premature PB deletion, and as
 *		required by cippd_log_path() and cippd_csyslev() in case event
 *		logging becomes necessary.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 *
 *		The PB semaphore is incremented whenever it is found necessary
 *		to unlock the PB and yet postpone potential PB deletion.  This
 *		need exists during both locking of the SCA database and SCS
 *		notification of new path existence.  The PB semaphore is
 *		decremented once the PB is again protected against premature
 *		deletion by the re-obtaining of its lock.
 */
u_long
cippd_enter_db( pccb, pb, cippdbp )
    register PCCB	*pccb;
    register PB		*pb;
    GVPPPDH		*cippdbp;
{
    register sbq	*sb;
    register SB		*target_sb = pb->sb;
    register u_long	nstatus, sstatus;
    u_long		old_pstate, lock_scadb = 0, status = RET_SUCCESS,
			portnum = Scaaddr_low( pb->pinfo.rport_addr );

    /* Lock the SCA database if it is not EXTERNALLY locked and search for the
     * target system within the system-wide configuration database.  This
     * search terminates when one of the following conditions is met:
     *
     * 1. The target system is NOT found within the database.
     * 2. The target system is found within the database.
     * 3. A system is found within the database which matches the target system
     *    but possesses a different incarnation number.
     * 4. A system with the same system identification number as the target
     *	  system but with a different node name is found within the database.
     * 5. A system with the same node name as the target system but with a
     *	  different system identification number is found within the database.
     *
     * The occurrence of any one of the last three conditions signals a
     * system-wide configuration database conflict and the following actions
     * are taken:
     *
     * 1. The SCA database is unlocked but only if it was locked INTERNALLY.
     * 2. The database conflict is logged.
     * 3. A failure status is returned to trigger path clean up, including path
     *    disablement by the finite state machine.
     *
     * Only the very first database conflict involving a specific remote port
     * is logged( Action 2 ).  Logging of subsequent conflicts is bypassed
     * until after the CI PPD determines that a conflict with the specific
     * remote port no longer exists.  Note that the conflict is designated as
     * being associated with a specific remote port instead of with a specific
     * remote system as would normally be expected.  This is because such
     * conflicts can not be treated as common system level events because the
     * identity of the remote system is itself in question.
     *
     * Once it has been determined that a system-wide configuration database
     * conflict does not exist, the remote CI PPD protocol version level is
     * checked for a mismatch.  Two types of mismatches are possible:
     *
     * 1. The remote CI PPD is running a later version of the protocol.
     * 2. The remote CI PPD is running an earlier version of the protocol.
     *
     * Any mismatch is logged as a common system level event.  Such events are
     * associated with specific remote systems and not with specific ports on
     * those systems.  Only the very first instance of each mismatch type
     * involving a specific remote system is logged.  Logging of subsequent
     * mismatches of that type are bypassed until after the CI PPD determines
     * that the specific mismatch with the remote system no longer exists.
     *
     * Mismatches with the remote CI PPD running a later version of the CI PPD
     * protocol are considered benign.  The local CI PPD always attempts to
     * "talk up" to the remote CI PPD.  This is not the case when a remote CI
     * PPD is found to be running an earlier CI PPD protocol version.  The
     * local CI PPD never attempts to "talk down" to a remote CI PPD.  Instead,
     * the SCA database is unlocked( if it was locked INTERNALLY ) and a
     * failure status is returned to trigger path clean up, including path
     * disablement by the finite state machine, after the mismatch is logged.
     *
     * If no database conflict is found and if no serious CI PPD protocol
     * mismatch exists, then this function proceeds to enter the PB
     * representing the formative path into the system-wide databases as
     * follows:
     *
     *  1. The formative SB is inserted into the system-wide configuration
     *     database if the target system was not located; otherwise, the PB's
     *	   SB pointer is updated to the target system's SB and the formative SB
     *	   is dispossed of.
     *  2. The PB is removed from the appropriate port's formative PB queue.
     *  3. The PB is inserted into the system-wide configuration database.
     *  4. The bits representing the remote port within the database conflict
     *	   and path establishment timeout log maps are cleared.
     *  5. The PB address is cached in the PCCB PS_OPEN PB vector table by its
     *	   remote port address.
     *  6. Existence of the new path is event logged.
     *  7. The remote system software type is verified.
     *  8. The SCA database is unlocked but only if it was unlocked INTERNALLY.
     *  9. SCS is notified of the existence of a new path to the system.
     * 10. A status of success is returned.
     *
     * Verification of the remote system software type( Step 7 ) is necessary
     * because paths may be established to remote systems running other than
     * the following supported software types:
     *
     * 1. U-32( Ultrix systems ).
     * 2. HSC( HSC storage controllers ).
     * 3. RFXX( DSSI disk storage controllers ).
     * 4. TFXX( DSSI tape storage controllers ).
     *
     * Nothing is done to prevent the establishment of paths to remote systems
     * running unsupported software types.  However, the discovery of such a
     * system is logged as a common system level event both to inform the
     * customer that he/she is violating the SPD and to record the violation
     * for our benefit in case some problem occurs laters on.  Only the very
     * first instance of an unsupported software type associated with a
     * specific remote system is logged.  Logging of subsequent violations is
     * bypassed until after the CI PPD ascertains that the remote system is
     * running a supported software type.
     *
     * The path's state is transitioned to "open" prior to event logging( Step
     * 6 ) and notifying SCS( Step 9 ) of the new path to the system.  This
     * allows SYSAPs so notified to immediately request logical SCS connections
     * across the newly established path.
     *
     * There are two instances when this function must temporarily release all
     * locks.  These are:
     *
     * 1. When locking the SCA database.
     * 2. When notifying SCS of new path existence.
     *
     * All locks must be released prior to locking the SCA database in order to
     * preserve the SCA locking hierarchy.  They must be released during SCS
     * notification of new path existence both to allow SCS to process the
     * event and because it is required by the SCA architecture.  In both
     * instances the PB must be prevented from being cleaned up and deallocated
     * while locks are released.  The mechanism chosen to provide this
     * protection is incrementing of the appropriate PB semaphore.
     *
     * Incrementing a PB semaphore protects the corresponding PB against clean
     * up and deallocation because routines charged with these tasks are not
     * allowed to perform them while the semaphore is incremented.  This
     * restriction is taken advantage of by this routine whenever it wishes
     * to protect its PBs as follows:
     *
     * 1. Prior to releasing all locks the appropriate PB semaphore is
     *	  incremented.
     * 2. The PCCB and PB locks are released.
     * 3. The SCS database is locked or SCS is notified of new path existence.
     * 4. The PCCB is locked.
     * 5. The PB is retrieved.
     * 6. The PB is locked.
     * 7. The PB semaphore is decremented.
     *
     * Whenever all locks are released, it allows another CI PPD thread to
     * obtain them, fail the corresponding path, and schedule asynchronous PB
     * clean up.  However, the thread which becomes responsible for PB clean up
     * and deallocation is prevented from doing so while the PB semaphore is
     * incremented.  This allows the current thread to re-obtain all locks(
     * Steps 4 and 6 ), decrement the PB semaphore( Step 7 ) and exit this
     * function.  Only when the current thread releases its re-obtained PB lock
     * can the thread responsible for PB clean up and deallocation perform
     * these tasks.  This is how protection of the PB is achieved through
     * incrementing of its semaphore.
     *
     * The PB is retrieved before re-locking( Step 5 ) whenever locks are
     * temporarily released.  This retrieval is actually unnecessary because
     * incrementing the PB semaphore should protect the validity of cached PB
     * addresses as it protects the PB themselves from clean up and
     * deallocation.  It is done anyway as a further precaution and to detect
     * the existence of errors in error recovery logic.  Cached PCCB addresses
     * remain valid because PCCBs are never deleted once their ports have been
     * initialized.
     *
     * While this routine can protect its PBs from clean up and deallocation it
     * can NOT protect them from path state changes while it has temporarily
     * released all locks.  This allows the following path state changes to
     * possibly occur when locks are released:
     *
     * 1. Paths can be opened by other CI PPD threads when locks are released
     *	  for the purpose of locking the SCA database.
     * 2. Paths may also fail, but not be cleaned up, whenever locks are
     *	  temporarily released as documented above.
     *
     * Opening a PB involves entering it into the system-wide databases.
     * Therefore, occurrence of the first type of path state change means that
     * the current CI PPD thread no longer has any need to execute this action
     * and may immediately return success.  Path failures while locks are
     * temporarily released present an entirely different matter because the
     * paths can no longer be successfully entered into the system-wide
     * databases.  Such a path state change is handled by immediately returning
     * a failure status to indicate this inability.  This does forces iterative
     * invocation of the CI PPD finite state machine to terminate the target
     * path( event == CNFE_PROC_ERROR ).  However, as the path has failed and
     * is already undergoing error recovery the iterative invocation is just
     * dismissed without further action being taken.
     *
     * NOTE: This function must determine whether to return success or failure
     *	     before inserting the PB into the system-wide databases.  This is
     *	     because the act of inserting the PB establishes the path fully and
     *	     no action function can return a failure status for a fully
     *	     established path( see rule 4 at front of module ).
     *
     * NOTE: Optional information logged for database conflicts includes the
     *	     station address of a remote port located on the known system.  The
     *	     known system may have many such remote ports.  Only the station
     *	     address of the first remote port known locally is logged.
     */
    if( !Test_scadb_lock()) {
	old_pstate = pb->pinfo.state;
	Incr_pb_sem( pb )
	Unlock_pb( pb )
	Unlock_pccb( pccb )
	Lock_scadb()
	Lock_pccb( pccb )
	if(( pb = cippd_get_pb( pccb, ( SCSH * )&portnum, NO_BUF )) == NULL ) {
	    ( void )panic( PPDPANIC_NOPATH );
	}
	Lock_pb( pb )
	Decr_pb_sem( pb )
	if( old_pstate != pb->pinfo.state ) {
	    if( pb->pinfo.state == PS_OPEN ||
		 pb->pinfo.state == PS_PATH_FAILURE ){
		Unlock_scadb()
		if( pb->pinfo.state == PS_PATH_FAILURE ) {
		    status = RET_FAILURE;
		}
		return( status );
	    } else {
		( void )panic( PPDPANIC_PSTATE );
	    }
	}
    } else {
	lock_scadb = 1;
    }
    for( sb = scs_config_db.flink; sb != &scs_config_db; sb = sb->flink ) {
	nstatus = Comp_node( target_sb->sinfo.node_name, Sb->sinfo.node_name );
	sstatus = Comp_scaaddr( target_sb->sinfo.sysid, Sb->sinfo.sysid );
	if( sstatus  &&
	     nstatus &&
	     Comp_quad( Sb->sinfo.swincrn, target_sb->sinfo.swincrn )) {
	    break;
	} else if( !sstatus && !nstatus ) {
	    continue;
	} else {
	    Move_quad( target_sb->sinfo.swincrn,
		       pccb->Elogopt.dbcoll.cippd_rswincrn[ 0 ])
	    Move_quad( Sb->sinfo.swincrn,
		       pccb->Elogopt.dbcoll.cippd_kswincrn[ 0 ])
	    Move_scaaddr( Sb->sinfo.sysid,
			  *pccb->Elogopt.dbcoll.cippd_ksysid )
	    Move_node( Sb->sinfo.node_name,
		       pccb->Elogopt.dbcoll.cippd_kname )
	    Move_scaaddr((( PB * )Sb->pbs.flink )->pinfo.rport_addr,
			  *pccb->Elogopt.dbcoll.cippd_krsaddr )
	    Move_scaaddr(((( PB * )Sb->pbs.flink )->pccb )->lpinfo.addr,
			  *pccb->Elogopt.dbcoll.cippd_klsaddr )
	    if( !lock_scadb ) {
		Unlock_scadb()
	    }
	    if( !Test_lpinfomap( Dbclogmap, portnum )) {
		Set_lpinfomap( Dbclogmap, portnum )
		( void )cippd_log_path( pccb, pb, NULL, RE_DBCONFLICT );
	    }
	    status = RET_FAILURE;
	    break;
	}
    }
    if( status == RET_SUCCESS ) {
	if( pb->pinfo.protocol == pccb->lpinfo.Protocol ) {
	    ( void )cippd_csyslev( pccb, pb->sb, W_TALKUP, CLEAR );
	    ( void )cippd_csyslev( pccb, pb->sb, E_TALKDOWN, CLEAR );
	} else {
	    pccb->Elogopt.protocol.cippd_remote = pb->pinfo.protocol;
	    pccb->Elogopt.protocol.cippd_local = pccb->lpinfo.Protocol;
	    if( pb->pinfo.protocol > pccb->lpinfo.Protocol ) {
		( void )cippd_csyslev( pccb, pb->sb, W_TALKUP, SET );
		( void )cippd_csyslev( pccb, pb->sb, E_TALKDOWN, CLEAR );
	    } else {
		( void )cippd_csyslev( pccb, pb->sb, E_TALKDOWN, SET );
		( void )cippd_csyslev( pccb, pb->sb, W_TALKUP, CLEAR );
		if( !lock_scadb ) {
		    Unlock_scadb()
		}
		status = RET_FAILURE;
	    }
	}
    }
    if( status == RET_SUCCESS ) {
	if( sb == &scs_config_db ) {
	    sb = ( sbq * )target_sb;
	    Insert_entry( Sb->flink, scs_config_db )
	} else {
	    ( void )scs_dealloc_sb( target_sb );
	    pb->sb = Sb;
	}
	Remove_entry( pb->flink )
	--pccb->lpinfo.Nform_paths;
	Insert_entry( pb->flink, Sb->pbs )
	++Sb->sinfo.npaths;
	pccb->Open_pb[ portnum ] = ( pbq * )pb;
	++pccb->lpinfo.Npaths;
	Clear_lpinfomap( Dbclogmap, portnum )
	Clear_map( Tmologmap, portnum )
	pb->pinfo.state = PS_OPEN;
	( void )cippd_log_path( pccb, pb, NULL, I_NEW_PATH );
	if(( bcmp(( char * )&Sb->sinfo.swtype, "U-32", 4 ) == 0 ) ||
	   ( bcmp(( char * )&Sb->sinfo.swtype, "HSC ", 4 ) == 0 ) ||
	   ( bcmp(( char * )&Sb->sinfo.swtype, "RFXX", 4 ) == 0 ) ||
	   ( bcmp(( char * )&Sb->sinfo.swtype, "TFXX", 4 ) == 0 )) { 
	    ( void )cippd_csyslev( pccb, Sb, W_BADSWTYPE, CLEAR );
	} else {
	    pccb->Elogopt.rswtype = Sb->sinfo.swtype;
	    ( void )cippd_csyslev( pccb, Sb, W_BADSWTYPE, SET );
	}
	Incr_pb_sem( pb )
	Unlock_pb( pb )
	Unlock_pccb( pccb )
	Unlock_scadb()
	( void )scs_new_path( Sb, pb );
	if( lock_scadb ) {
	    Lock_scadb()
	}
	Lock_pccb( pccb )
	if(( pb = ( PB * )pccb->Open_pb[ portnum ]) == NULL ) {
	    ( void )panic( PPDPANIC_NOPATH );
	}
	Lock_pb( pb )
	Decr_pb_sem( pb )
	if( pb->pinfo.state != PS_OPEN ) {
	    if( pb->pinfo.state == PS_PATH_FAILURE ) {
		status = RET_FAILURE;
	    } else {
		( void )panic( PPDPANIC_PSTATE );
	    }
	}
    }
    return( status );
}

/*   Name:	cippd_error_dg	- Process Received ERROR CI PPD Datagram
 *
 *   Abstract:	This action routine processes received ERROR CI PPD datagrams
/*		by logging them.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) postponing premature PB
 *		deletion and as required by cippd_log_sys().
 */
void
cippd_error_dg( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    ( void )cippd_log_sys( pccb, pb->sb, cippdbp, RE_RERROR );
}

/*   Name:	cippd_ignore_dg	- Ignore CI PPD Datagram
 *
 *   Abstract:	This action routine ignores received CI PPD datagrams by
 *		returning them to the appropriate local port datagram free
 *		queues.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by PD routines
 *		which add datagrams to appropriate local port datagram free
 *		queues in this particular case( It is required in this
 *		particular case ONLY because the PB is EXTERNALLY locked ).
 */
void
cippd_ignore_dg( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    ( void )( *pccb->Add_dg )( pccb, Ppd_to_scs( cippdbp ));
}

/*   Name:	cippd_init_pb	- Initialize a Path Block
 *
 *   Abstract:	This action function directs PB initialization.  It is invoked
 *		by the finite state machine only when an ID packet is received
 *		from a previously unknown remote port.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in ID packet
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:	
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- PB successfully initialized
 *   RET_FAILURE		- PB initialization failed
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by optional PD
 *		functions which perform port driver specific PB
 *		initializations.
 *
 *		The PB is locked( EXTERNALLY ) postponing premature deletion,
 *		allowing exclusive access to PB contents, and as required by
 *		optional PD functions which perform port driver specific PB
 *		initializations.
 */
u_long
cippd_init_pb( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	( *init )(), status = RET_SUCCESS;

    /* PB initialization includes:
     *
     * 1. Invoking an optional port driver specific function to initialize the
     *	  PB in a port driver specific fashion.
     *
     * NOTE: Optional PD specific functions are responsible for logging
     *	     appropriate failure reasons.  Only the first failure involving a
     *	     specific path should be logged.  Logging of subsequent failures
     *	     should be bypassed until it is determined that the failure
     *	     condition no longer exists for the specific path.
     */
    if(( init = *pccb->Init_pb )) {
	status = ( *init )( pccb, pb, cippdbp );
    }
    return( status );
}

/*   Name:	cippd_inv_cache	- Invalidate Local Port Translation Cache
 *
 *   Abstract:	This action routine optionally invalidates a specific local
 *		port translation cache utilizing an optional port driver
 *		specific routine.  It is invoked by the finite state machine
 *		only during the termination of established paths.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram buffer
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:	
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by PD routines
 *		which invalidate local port translation caches.
 *
 *		The PB is locked( EXTERNALLY ) postponing premature deletion
 *		and as required by PD routines which invalidate local port
 *		translation caches.
 */
void
cippd_inv_cache( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	( *invcache )();

    if(( invcache = *pccb->Inv_cache )) {
	( void )( *invcache )( pccb, pb );
    }
}

/*   Name:	cippd_panic	- Unexpected State-Event Combination
 *
 *   Abstract:	This action routine is invoked whenever the finite state
 *		machine encounters an unexpected and illegal path state - event
 *		combination.  All such combinations either never occur or are
 *		fully processed EXTERNAL to the CI PPD action dispatcher.  When
 *		encountered they immediately result in a protective panic.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address CI PPD header in datagram( OPTIONAL )
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.
 */
void
cippd_panic( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    ( void )panic( PPDPANIC_FSM );
}

/*   Name:	cippd_path_schd	- Schedule Asynchronous CI PPD Path Clean Up
 *
 *   Abstract:	This action function schedules asynchronous clean up of a
 *		specific CI PPD path through forking.  It is invoked by the
 *		finite state machine only during the termination of paths.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram buffer
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus.fkip	-   0
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:	
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus.fkip	-   1
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing premature deletion
 *		and allowing exclusive access to PB contents.
 *
 *		The PB semaphore is incremented to serve as an additional
 *		guarantee of PB validity when clean up asynchronously commences
 *		and to detect errors in error recovery logic.
 */
void
cippd_path_schd( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    /* Different clean up routines are scheduled for formative and established
     * CI PPD paths reflecting different clean up requirements for each type
     * of path.  Prior to scheduling asynchronous clean up the appropriate PB
     * semaphore is incremented.  Incrementing the semaphore prevents other CI
     * PPD threads from deallocating the PB.  Actually, this incrementing is
     * superfluous.  Only cleaned up paths are deallocated and only crashed
     * paths are cleaned up and each path may only be crashed once per path
     * incarnation.  Therefore, it is not possible for another CI PPD thread to
     * be in a position to deallocate the PB and the single threaded nature of
     * path clean up should be sufficient to guarantee PB validity when
     * scheduled clean up eventually commences.  The semaphore is incremented
     * anyway to further protect the PB and to detect errors in error recovery
     * logic.  The semaphore is always decremented prior to deallocation of the
     * PB following its clean up.
     */
    Incr_pb_sem( pb )
    if( pb->pinfo.state == PS_OPEN ) {
	Pb_fork( pb, cippd_clean_pb, PPDPANIC_PBFB )
    } else if( pb->pinfo.state != PS_PATH_FAILURE ) {
	Pb_fork( pb, cippd_clean_fpb, PPDPANIC_PBFB )
    } else {
	( void )panic( PPDPANIC_PSTATE );
    }
}

/*   Name:	cippd_pcreason	- Map Path Crash Reason
 *
 *   Abstract:	This action routine maps a specific path crash event code into
 *		a more general reason for path failure.  This routine must only
 *		be executed for established paths.  Formative paths perform
 *		this mapping at the time the path is initially crashed.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippd_map_pc		- CI PPD path crash reason mapping table
 *   cippd_map_spc		- CI PPD severe path crash reason mapping table
 *   cippdbp			- Address of CI PPD header in datagram buffer
 *   pb				- Path Block pointer
 *	pinfo.reason		-  Path crash reason
 *	pinfo.state		-  PS_OPEN
 *   pccb			- Port Command and Control Block pointer
 *   scs_map_pc			- SCS path crash reason mapping table
 *   scs_map_spc		- SCS severe path crash reason mapping table
 *
 *   Outputs:	
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pinfo.reason		-  General path failure reason
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing premature deletion
 *		and allowing exclusive access to PB contents.
 */
void
cippd_pcreason( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	pc_reason = pb->pinfo.reason,
			code = Ecode( pc_reason ), pf_reason = 0;

    /* Two classes/subclasses of path crash reasons requiring mapping:
     *
     * 1. SCS path crash reasons.
     * 2. CI PPD path crash reasons.
     *
     * Port drivers do NOT define specific path crash reasons.  All reasons are
     * are mapped by this function.  A panic occurs if the specific path crash
     * reason is unknown or invalid.
     *
     * NOTE: Path crash event codes may exist at either of the following two
     *	     severity levels regardless of class/subclass: Error, Severe Error.
     *	     All path crash reason mapping must take multiple severity levels
     *	     into account.
     */
    if( pb->pinfo.state == PS_OPEN && pc_reason ) {
	if( Test_scs_event( pc_reason )) {
	    if( Test_spc_event( pc_reason )) {
		if( code <= Ecode( SE_MAX_SCS )) {
		    pf_reason = scs_map_spc[ code - 1 ];
		}
	    } else if( Test_pc_event( pc_reason )) {
		if( code <= Ecode( E_MAX_SCS )) {
		    pf_reason = scs_map_pc[ code - 1 ];
		}
	    }
	} else if( Test_ppd_event( pc_reason )) {
	    if( Test_spc_event( pc_reason )) {
		if( code <= Ecode( SE_MAX_PPD )) {
		    pf_reason = cippd_map_spc[ code - 1 ];
		}
	    } else if( Test_pc_event( pc_reason )) {
		if( code <= Ecode( E_MAX_PPD )) {
		    pf_reason = cippd_map_pc[ code - 1 ];
		}
	    }
	}
    }
    if(( pb->pinfo.reason = pf_reason ) == 0 ) {
	( void )panic( PPDPANIC_PCR );
    }
}

/*   Name:	cippd_ppderror	- Process CI PPD Protocol Errors
 *
 *   Abstract:	This action function processes CI PPD protocol violations by
 *		terminating the associated CI PPD path.  CI PPD protocol
 *		violations only occur on formative paths.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram/message
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_FAILURE		- PPD Protocol Error requires path clean up
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to guarantee PB address
 *		validity as required by cippd_log_path().
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
u_long
cippd_ppderror( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    /* Occurrence of the CI PPD protocol error is explicitly logged and a
     * failure status is returned to force termination clean up of the
     * formative path.
     */
    if( pb->pinfo.state != PS_OPEN ) {
	( void )cippd_log_path( pccb, pb, cippdbp, SE_PPDPROTOCOL );
    } else {
	( void )panic( PPDPANIC_PSTATE );
    }
    return( RET_FAILURE );
}

/*   Name:	cippd_req_id	- Request Remote Port Identification
 *
 *   Abstract:	This action routine initiates a request for identification of
 *		a specific remote port.  It is used to both make an initial
 *		identification request and to retry a request after a previous
 *		attempt has timed out without receiving identification.
 *
 *		NOTE: The PD specific function responsible for requesting
 *		      remote port identification can NEVER fail when provided
 *		      with an optional CI PPD datagram buffer at invocation.
 *		      It can only fail when it requires such a buffer, is not
 *		      provided with one, and must explicitly allocate it.  Such
 *		      failure statuses are neither logged nor returned to abort
 *		      path establishment.  They are just ignored and eventually
 *		      expiration of the path associated timer triggers the next
 *		      attempt at requesting remote port identification.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address CI PPD header in datagram( OPTIONAL )
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents and as required by PD functions which request
 *		remote port identifications.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
void
cippd_req_id( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	save_cable = pccb->Poll_cable;

    /* An initial attempt to request the identification of a target remote port
     * uses the CI PPD datagram buffer provided to this routine as necessary.
     * It is added to the appropriate local port datagram free queue following
     * command execution.  Subsequent attempts to request the identification of
     * the port do not have use of explicitly provided port specific buffers.
     * Command packets allocated by the PD specific function for use in these
     * attempts are deallocated following command execution.
     *
     * NOTE: Requests for identification of target remote ports are always made
     *	     without regard to the specific cable over which they are to be
     *	     transmitted.
     */
    pccb->Poll_cable = ANY_CABLE;
    ( void )( *pccb->Send_reqid )( pccb,
				   cippdbp,
				   Scaaddr_low( pb->pinfo.rport_addr ),
				   (( cippdbp ) ? RECEIVE_BUF : DEALLOC_BUF ));
    pccb->Poll_cable = save_cable;
}

/*   Name:	cippd_rrestart	- Process Remote CI PPD Path Restart Requests
 *
 *   Abstract:	This action routine processes remote CI PPD path restart
 *		requests.  Such requests are essentially START CI PPD datagrams
 *		received on fully established paths.  They represent an attempt
 *		at path establishment after unilateral path termination and
 *		clean up by the remote CI PPD.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram/message
 *   pb				- Path Block pointer
 *	pinfo.state		-  PS_OPEN
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to guarantee PB address
 *		validity as required by PD routines which crash paths.
 */
void
cippd_rrestart( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    /* Remote CI PPD path restart requests are processed by locally crashing
     * the specific path.  It is only possible to re-start( ie- re-establish )
     * the path after it has been terminated and cleaned up locally.
     */
    if( pb->pinfo.state == PS_OPEN ) {
	( void )cippd_crash_pb( pccb,
				pb,
				E_RRESTARTPATH,
				RETURN_BUF,
				Ppd_to_scs( cippdbp ));
    } else {
	( void )panic( PPDPANIC_PSTATE );
    }
}

/*   Name:	cippd_snd_ack	- Send ACK CI PPD Datagram
 *
 *   Abstract:	This action routine transmits an ACK CI PPD datagram to a
 *		specific remote port.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by PD functions
 *		which transmit datagrams in this particular case( It is
 *		required in this particular case only because the PB is
 *		EXTERNALLY locked ).
 *
 *    		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
void
cippd_snd_ack( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    /* Add the CI PPD datagram buffer to the appropriate local port datagram
     * free queue following transmission when the finite state machine is
     * merely re-transmitting an ACK over an established path.  Deallocate the
     * the CI PPD datagram buffer following transmission in all other cases as
     * this buffer represents the datagram temporarily allocated for all
     * protocol exchanges during path establishment when such establishment was
     * originally initiated.
     */
    ( void )( *pccb->Send_dg )( pccb,
				pb,
				Ppd_to_scs( cippdbp ),
				0,
				(( pb->pinfo.state == PS_OPEN )
					? RECEIVE_BUF : DEALLOC_BUF ),
				ACK );
}

/*   Name:	cippd_snd_stack	- Send STACK CI PPD Datagram
 *
 *   Abstract:	This action routine transmits a STACK CI PPD datagram to a
 *		specific remote port.  It is used to both transmit an initial
 *		STACK datagram and to retry STACK datagram transmission after a
 *		previous attempt has timed out without receiving an appropriate
 *		response.
 *
 *		NOTE: This routine can NEVER fail at STACK CI PPD datagram
 *		      transmission when provided with an optional CI PPD
 *		      datagram buffer at invocation.  It can only fail when it
 *		      is not provided with such a buffer and must explicitly
 *		      allocate one.  Such failure statuses are neither logged
 *		      nor returned to abort path establishment. They are just
 *		      ignored and eventually expiration of the path associated
 *		      timer triggers the next STACK datagram transmission
 *		      attempt.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   boottime			- Time of last system boot
 *   cippdbp			- Address CI PPD header in datagram( OPTIONAL )
 *   lscs			- Local system information
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *   time			- Current time of day
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *				- Unable to allocate port specific buffer
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by PD functions
 *		which transmit datagrams in this particular case( It is
 *		required in this particular case only because the PB is
 *		EXTERNALLY locked ).
 *
 *    		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 *
 *		The current time is retrieved in such a way so as to preserve
 *		its intrinsic monotonic increasing nature in a SMP environment.
 */
void
cippd_snd_stack( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	dispose;
    register CIPPD_STACK *bp;
    struct timeval	cur_time;

    /* An initial attempt to transmit a STACK CI PPD datagram uses the CI PPD
     * datagram buffer provided to this routine.  It is added to the
     * appropriate local port datagram free queue following datagram
     * transmission.  Subsequent attempts to transmit STACK CI PPD datagrams
     * use datagram buffers specifically allocated by this routine.  These
     * buffers are deallocated following datagram transmission.
     */
    if(	cippdbp ) {
	dispose = RECEIVE_BUF;
	bp = Cippd_stack( Ppd_to_scs( cippdbp ));
    } else if(( bp = Cippd_stack(( *pccb->Alloc_dg )( pccb )))) {
	dispose = DEALLOC_BUF;
    } else {
	return;
    }
    *( SIB * )bp = lscs.system;
    bp->protocol = pccb->lpinfo.Protocol;
    Fetch_time( cur_time, time )
    ( void )scs_unix_to_vms( &boottime, &bp->swincrn );
    ( void )scs_unix_to_vms( &cur_time, &bp->cur_time );
    ( void )( *pccb->Send_dg )( pccb,
				pb,
				( SCSH * )bp,
				sizeof( CIPPD_STACK ),
				dispose,
				STACK );
}

/*   Name:	cippd_snd_start	- Send START CI PPD Datagram
 *
 *   Abstract:	This action routine transmits a START CI PPD datagram to a
 *		specific remote port.  It is used to both transmit an initial
 *		START datagram and to retry START datagram transmission after a
 *		previous attempt has timed out without receiving an appropriate
 *		response.
 *
 *		NOTE: This routine can NEVER fail at START CI PPD datagram
 *		      transmission when provided with an optional CI PPD
 *		      datagram buffer at invocation.  It can only fail when it
 *		      is not provided with such a buffer and must explicitly
 *		      allocate one.  Such failure statuses are neither logged
 *		      nor returned to abort path establishment. They are just
 *		      ignored and eventually expiration of the path associated
 *		      timer triggers the next START datagram transmission
 *		      attempt.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   boottime			- Time of last system boot
 *   cippdbp			- Address CI PPD header in datagram( OPTIONAL )
 *   lscs			- Local system information
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *   time			- Current time of day
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by PD functions
 *		which transmit datagrams in this particular case( It is
 *		required in this particular case only because the PB is
 *		EXTERNALLY locked ).
 *
 *    		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 *
 *		The current time is retrieved in such a way so as to preserve
 *		its intrinsic monotonic increasing nature in a SMP environment.
 */
void
cippd_snd_start( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	dispose;
    register CIPPD_START *bp;
    struct timeval	cur_time;

    /* An initial attempt to transmit a START CI PPD datagram uses the CI PPD
     * datagram buffer provided to this routine.  It is added to the
     * appropriate local port datagram free queue following datagram
     * transmission.  Subsequent attempts to transmit START CI PPD datagrams
     * use datagram buffers specifically allocated by this routine.  These
     * buffers are deallocated following datagram transmission.
     */
    if( cippdbp ) {
	dispose = RECEIVE_BUF;
	bp = Cippd_start( Ppd_to_scs( cippdbp ));
    } else if(( bp = Cippd_start(( *pccb->Alloc_dg )( pccb )))) {
	dispose = DEALLOC_BUF;
    } else {
	return;
    }
    *( SIB * )bp = lscs.system;
    bp->protocol = pccb->lpinfo.Protocol;
    Fetch_time( cur_time, time )
    ( void )scs_unix_to_vms( &boottime, &bp->swincrn );
    ( void )scs_unix_to_vms( &cur_time, &bp->cur_time );
    ( void )( *pccb->Send_dg )( pccb,
				pb,
				( SCSH * )bp,
				sizeof( CIPPD_START ),
				dispose,
				START );
}

/*   Name:	cippd_snd_stop	- Send STOP CI PPD Datagram
 *
 *   Abstract:	This action routine transmits a STOP CI PPD datagram to a
 *		specific remote port.
 *
 *		NOTE: This function is currently NOT supported because of VMS
 *		      unwillingness to compromise during SCA Review Group
 *		      meetings.  It will be added when and if the issue is
 *		      finally resolved.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address CI PPD header in datagram( OPTIONAL )
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by PD functions
 *		which transmit datagrams in this particular case( It is
 *		required in this particular case only because the PB is
 *		EXTERNALLY locked ).
 *
 *    		The PB is locked( EXTERNALLY ) postponing premature deletion.
 */
void
cippd_snd_stop( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register SCSH	*scsbp;

    /* Always allocate a port driver specific datagram buffer to contain the
     * STOP CI PPD datagram.  This buffer is deallocated following datagram
     * transmission.
     *    if(( scsbp = ( *pccb->Alloc_dg )( pccb ))) {
     *	     ( void )( *pccb->Send_dg )( pccb, pb, scsbp, 0, DEALLOC_BUF,STOP);
     *	  }
     */
}

/*   Name:	cippd_start_tmr	- Start CI PPD Traffic Interval Timer
 *
 *   Abstract:	This action routine starts a traffic interval timer on the
 *		specified CI PPD path.  A timeout event( CNFE_TIMEOUT ) is
 *		declared on the target path if an appropriate response is not
 *		received on it before the timer expires.  The timeout period is
 *		chosen to be larger than the current port polling interval.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- NULL
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus.timer	-   0
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    due_time		-   CI PPD traffic interval timer
 *	    fsmpstatus.timer	-   1
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
void
cippd_start_tmr( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    GVPPPDH		*cippdbp;
{
    if( !pb->Fsmpstatus.timer ) {
	pb->Fsmpstatus.timer = 1;
	pb->Due_time = (( pccb->Poll_interval > pccb->Timer_interval )
				? pccb->Poll_interval
				: pccb->Timer_interval ) + 1;
    } else {
	( void )panic( PPDPANIC_TIMER );
    }
}

/*   Name:	cippd_stop_dg	- Process Received STOP CI PPD Datagram
 *
 *   Abstract:	This action routine processes received STOP CI PPD datagrams
 *		which signify remote CI PPD termination of their local paths.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_FAILURE		- Formative paths require clean up
 *   RET_SUCCESS		- Fully established paths must return success
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to guarantee PB address
 *		validity as required by PD routines which crash paths and by
 *		cippd_log_path().
 */
u_long
cippd_stop_dg( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    /* The path over which the STOP CI PPD datagram is received is terminated
     * according to its current path state:
     *
     * Fully established paths: The path is crashed and a status of success is
     *				returned.
     *
     * Formative paths:		Reception of the STOP CI PPD datagram is
     *				explicitly logged and a failure status is
     *				returned to force clean up.
     */
    if( pb->pinfo.state == PS_OPEN ) {
	( void )cippd_crash_pb( pccb,
				pb,
				E_RHOST,
				RETURN_BUF,
				Ppd_to_scs( cippdbp ));
	return( RET_SUCCESS );
    } else {
	( void )cippd_log_path( pccb, pb, cippdbp, E_RHOST );
	return( RET_FAILURE );
    }
}

/*   Name:	cippd_stop_tmr	- Stop CI PPD Traffic Interval Timer
 *
 *   Abstract:	This action routine stops the traffic interval timer for a CI
 *		PPD path.  This prevents a timeout event( CNFE_TIMEOUT ) from
 *		being generated on the target path.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address CI PPD header in datagram( OPTIONAL )
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus.timer	-   1
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    fsmpstatus.timer	-   0
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
void
cippd_stop_tmr( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    if( pb->Fsmpstatus.timer ) {
	pb->Fsmpstatus.timer = 0;
    } else {
	( void )panic( PPDPANIC_TIMER );
    }
}

/*   Name:	cippd_test_trys	- Test/Decrement Number Transmission Attempts
 *
 *   Abstract:	This action function determines whether transmission attempts
 *		remain for the current stage of path establishment.  It is only
 *		invoked following expiration of path associated timers.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    retry		-   Retry count
 *   pccb			- Port Command and Control Block pointer
 *      ppd.cippd               -  CI PPD specific PCCB fields
 *          tmologmap           -   Path establishment timeout log map
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Transmission attempts remain
 *   RET_FAILURE		- All transmission attempts exhausted
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents, postponing premature PB deletion, and as
 *		required by cippd_log_path() in case logging becomes necessary.
 *
 *   		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
u_long
cippd_test_trys( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	status = RET_SUCCESS;

    /* Exhaustion of transmission attempts aborts new path establishment.
     * Only the first such failure to establish a specific path is logged.
     * Logging of all subsequent failures is bypassed until after the CI PPD is
     * successful at establishing the specific path( see cippd_enter_db()).
     */
    if( pb->Retry-- == 0 ) {
	if( !Test_map( Tmologmap, Scaaddr_low( pb->pinfo.rport_addr ))) {
	    Set_map( Tmologmap, Scaaddr_low( pb->pinfo.rport_addr ))
	    ( void )cippd_log_path( pccb, pb, NULL, E_NORETRIES );
	}
	status = RET_FAILURE;
    }
    return( status );
}

/*   Name:	cippd_update_sb	- Update Formative System Block
 *
 *   Abstract:	This action routine updates a formative SB with information
 *		contained within a received START/STACK CI PPD datagram.  It is
 *		invoked whenever a formative SB already exists for a formative
 *		path, built by the reception of a preceding START/STACK CI PPD
 *		datagram.  The only information currently updated within the
 *		formative System Block is its software incarnation number.
 *
 *		This routine exists solely for the convenience of the HSC
 *		which does not possess an on-board clock.  It derives its
 *		software incarnation number from the current time passed to it
 *		within some system's START/STACK CI PPD datagram.  Until such
 *		time as it receives its first START/STACK its software
 *		incarnation number is 0 and is transmitted as such.  Therefore,
 *		this routine allows the finite state machine to update a
 *		remote system's software incarnation number when the
 *		appropriate value becomes available.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   sb				- Formative System Block pointer
 *	sinfo.swincrn		-  Software incarnation number
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing potential deletion.
 *
 *		The SB does not need to be locked before updating because only
 *		the software incarnation number is being updated and currently
 *		this number is used for informational purposes only.  If other
 *		critical uses for this value evolve then and only then must
 *		the SB be locked before updating.
 */
void
cippd_update_sb( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    Move_quad( Cippd_start( Ppd_to_scs( cippdbp ))->swincrn,
	       pb->sb->sinfo.swincrn )
}

/*   Name:	cippd_upd_ptype	- Update Hardware Port Type of Remote Port
 *
 *   Abstract:	This action routine optionally updates the remote port's
 *		hardware port type utilizing an optional port driver specific
 *		routine.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PB is locked( EXTERNALLY ) as required by optional PD
 *		routines which update hardware port types.
 *
 *		PCCB addresses are always valid allowing access to static
 *		fields because these data structures are never deleted once
 *		their corresponding ports have been initialized( The PCCB is
 *		locked EXTERNALLY anyway ).
 */
void
cippd_upd_ptype( pccb, pb, cippdbp )
    PCCB		*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register u_long	( *update )();

    if(( update = pccb->Update_ptype )) {
	( void )( *update )( pb );
    }
}
