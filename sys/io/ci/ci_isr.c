#ifndef	lint
static char *sccsid = "@(#)ci_isr.c	4.3	(ULTRIX)	10/16/90";
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
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		interrupt service routines and functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	January 14, 1986
 *
 *   Function/Routines:
 *
 *   ci_rsp_handler		Process All CI Port Responses
 *   ci_unmapped_isr		Unmapped CI Adapter Interrupt Service Routine
 *   ci780_isr			CI750/CI780 Interrupt Service Routine
 *   cibca_isr			CIBCA-AA/CIBCA-BA Interrupt Service Routine
 *   cibci_isr			CIBCI Interrupt Service Routine
 *   cixcd_isr			CIXCD Interrupt Service Routine
 *   cikmf_isr			CIKMF Interrupt Service Routine
 *
 *   Modification History:
 *
 *   16-Oct-1990	Pete Keilty
 *	Changed smp_lock cidevice locking to use the new macros define
 *	in ciscs.h. Also changed the how registers are accessed only once.
 *	Also added DELAY( 1000 ) after writing Xber register because of
 *	the CIXCD XMOV bug.
 *
 *   16-Jul-1990	Pete Keilty
 * 	Add smp_lock lk_cidevice in the cixcd_isr routine for CIXCD,
 *	software workaround for XMOV hardware bug of back to back register
 *	accesses the first one a read the second a write to a software
 *	register. Psr read Psrcr write.
 *
 *   15-Jun-1990	Pete Keilty
 *	Added temporary smp lock lk_cidevice for CIXCD because of XMOV bug.
 *	
 *   06-Jun-1990	Pete Keilty
 *	Preliminary added CIKMF ( dash ) cikmf_isr() routine.
 *
 *   08-Dec-1989	Pete Keilty
 *	1. Use new macro Get_pgrp() to get port node number until 
 *	   full subnode addressing is done.
 *	2. Mask off upper two bits of opcode. Bit 7 is the new explicit
 *	   address bit of opcode not used for now.
 *	3. Changes cixcd_isr() routine to handle errors correctly.
 *
 *   19-Sep-1989	Pete Keilty
 *	1. Add XCD support, remove XCB.
 *	2. Add pccb to Pd_to_ppd macro.
 *
 *   25-May-1989	Pete Keilty
 *	Add new macro WBFLUSH for mips cpu's.
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's 
 *	Changed splx( IPL_SCS ) to new macro Splscs()
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   24-Mar-1989	Todd M. Katz		TMK0006
 *	Fix a bug introduced by TMK0005.  Following crashing of an established
 *	path to a remote port found to be in an improper state, the PB was
 *	being unlocked twice instead of unlocking both the PB and PCCB.
 *
 *   13-Jan-1989	Todd M. Katz		TMK0005
 *	1. Whenever an IDREC is received over an established path( path state
 *	   == PS_OPEN ) a check is made as to whether the remote port state has
 *	   transitioned from PS_ENAB/PS_ENAB_MAINT.  The path is crashed if it
 *	   has.  However, this check was not being correctly made.  The remote
 *	   port state in the PB was being checked, not the state within the
 *	   IDREC itself.  Also, the PCCB was not being unlocked following path
 *	   crashing.  Fix this error path.
 *	2. Include header file ../vaxmsi/msisysap.h.
 *
 *   22-Aug-1988	Todd M. Katz		TMK0004
 *	1. Rename the Informational Event( ES_I ) mnemonic from IE -> I.
 *	2. The following Informational Events( ES_I ) have been defined as
 *	   Warning Events( ES_WE ): POWER, STRAY.
 *	3. The following Informational Events( ES_I ) have been defined as
 *	   Fatal Error Events( ES_FE ): NOCI
 *	4. The following former CI path crash codes are now defined as Error
 *	   Events( ES_E ): NOCABLES, RPORTSTATE, CLOSEDVCD; and Severe Error
 *	   Events( ES_SE ): INVRPKTSIZE, OSEQMSG, UNRECPKT, BMSE.
 *	5. The following CI local port crash codes are now defined as Severe
 *	   Error Events( ES_SE ): PORTERROR, MFQE, MSE, DSE, SANITYTIMER,
 *	   PARITY, BIPARITY, BIERROR, BIMSE, POWER, POWERUP, UNKOPCODE,
 *	   INVOPCODE, UNKSTATUS, ABORTPKT, UNKCMD, INVDPORT, INVLPKTSIZE,
 *	   BACCVIO, INVBSIZE, INVBNAME Fatal Error Events( ES_FE ): NOCI. The
 *	   local port crash severity modifier is applied by ci_crash_lport()
 *	   but only when the crashed local port is not in the process of being
 *	   cleaned up from a previous crash.
 *	6. Modify ci780_isr() and cibci_isr() to log a fatal( FE_PORTERROR )
 *         instead of a severe( SE_PORTERROR ) error event when a broken link
 *	   module( L0100 ) is discovered.
 *	7. Refer to error logging as event logging.
 *	8. Physical errors on specific paths are now processed by first logging
 *	   the specific error and then crashing the specific path with a
 *	   generic path crash code appropriate to event severity.  Formerly,
 *	   just the path was crashed.  This required a unnecessairly convoluted
 *	   callback scheme to eventually allow the CI port driver to log the
 *	   port driver specific path related event.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *	1. Create a single unified hierarchical set of naming conventions for
 *	   use within the CI port driver and describe them within ciport.h.
 *	   Apply these conventions to all names( routine, macro, constant, and
 *	   data structure ) used within the driver.  Restructure the driver to
 *	   segregate most CI family and port type specific code into separate
 *	   routines.  Such restructuring requires splitting ci_isr() into
 *	   ci780_isr() and cibci_isr().  The former handles interrupts for
 *	   CI750/CI780 local ports while the latter is responsible for CIBCI
 *	   local port interrupts.
 *	2. Add support for the CIXCB hardware port type by:
 *		1) Adding routine cixcb_isr() to handle CIXCB interrupts.
 *		2) Adding include file ../vaxxmi/xmireg.h.
 *	3. Eliminate all locking from ci780_isr().  CI750 and CI780 local ports
 *	   never exist in SMP environments and so there is never any need to
 *	   lock any data structures.
 *	4. Modify cibci_isr() and cibca_isr() to properly synchronize access to
 *	   port register contents when processing errors.  Such synchronization
 *	   is required in SMP environments and is achieved through a PCCB lock.
 *	5. Modify the circumstances under which local ports are unmapped.
 *	   Unmapping is now done only when the local port adapter itself loses
 *	   power( CI750/CI780/CIBCI only ) or the port is marked broken and is
 *	   permanently shutdown.  Formerly, unmapping was done whenever a local
 *	   port was crashed, but before its re-initialization commenced; and
 *	   just immediately prior to its initial initialization.  This change
 *	   requires appropriate modifications to the comments of the routines
 *	   ci780_isr(), cibci_isr(), and cibca_isr().  The routine
 *	   ci_unmapped_isr() has also been extensively re-written to more
 *	   accurately reflect the new circumstances under which it can be
 *	   expected to be invoked.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   09-Apr-1988	Todd M. Katz		TMK0002
 *	Add support for the CIBCA-BA hardware port type.  Differentiate
 *	CIBCA-BA from CIBCA-AA hardware ports when necessary; otherwise, refer
 *	to both types as just CIBCA ports.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, restructured code paths, and added SMP support.
 */

/*			Interrupt Processing Philosophy
 *
 * The CI port driver's interrupt processing philosophy is to optimize its
 * primary interrupt service routines for response processing.  This is because
 * the placement of responses by local CI ports onto previously empty response
 * queues is the most like cause of interrupts.  Response processing is
 * optimized for by minimizing the number of checks made for errors.  It is
 * necessary to make some checks because most port detected errors represent
 * very serious situations which can only be recovered from by crashing and
 * re-initialization of the affected port.  In such instances responses are
 * never processed.  Instead, additional time is spent determining the exact
 * cause of the error before the local port is crashed.
 *
 * One major consequence of the minimization of error detection arising from
 * the optimization of response processing is the use of separate routines as
 * interrupt service handlers by different local CI ports and for all CI ports
 * when they are unmapped.  Different hardware port types require different
 * checks to be made for errors and the requirements for processing of
 * interrupts by unmapped local ports are radically different from the
 * requirements of mapped ones.  Whenever processing requirements sufficiently
 * diverge the CI port driver employs a different routine as interrupt service
 * handler for the local port.  
 */

/* 		    Interrupt Processing in SMP Environments
 *
 * In a SMP environment it is conceivable that multiple threads may be brought
 * into existence to simultaneously process interrupts.  For example, a local
 * port may signal an interrupt to report response availability.  While
 * processing of this initial interrupt is proceeding the same port may signal
 * another interrupt to again report response availability.  This scenario is
 * possible because the port status register in which response availability is
 * reported is released prior to initiating response processing.  It is also
 * possible that a second or even third interrupt may be signaled by the same
 * port to report an error either within the port status register or a bus
 * specific error register.  The end result is the same, each interrupt
 * potentially activates a new thread, each thread executes the same
 * ( appropriate ) interrupt service routine, and all such threads may be
 * simultaneously active( on separate processors ).
 *
 * The existence of multiple threads all executing the same interrupt service
 * routine never poses a problem provided all interrupts were signaled by the
 * local port to report response availability.  Only one caveat exists: it may
 * not be assumed that the absence of errors implies availability of responses
 * awaiting processing.  The existence of responses is always explicitly
 * checked for before the port status register is released and responses are
 * processed.
 *
 * The situation is not so straightforward when adapter reported errors are
 * involved.  The major source of complexity originates in the following facts:
 *
 * 1. Not all adapter reported errors are equal, an error hierarchy actually
 *    exists: soft error -> port( PSR ) error -> bus( non-port ) error in
 *    ascending order.
 * 2. Occurrence of a new interrupt wipes out the state of previous interrupts(
 *    interrupt state is reported within bus and port registers ).
 *
 * The error hierarchy allows an interrupt reporting response availability, to
 * be followed by an interrupt reporting a soft error, to be followed by an
 * interrupt reporting a port error, to be followed by an interrupt reporting a
 * bus error with the occurrence of each interrupt wiping out the state of
 * prior interrupts.  This sequence of ever more serious error interrupts would
 * never pose a problem in a single processor environment where only one
 * interrupt is processed at a time and is processed to completion.
 * Unfortunately, this is not the case in a SMP environment.  Multiple
 * interrupts may be processed simultaneously, some signaled to report
 * response availability while others to report errors.  Each new interrupt
 * potentially affects how previous interrupts are processed by completely
 * independent and concurrent threads.  Even the relative order in which
 * interrupts are processed to completion may not reflect the order in which
 * they were initially signaled due to temporary suspension of some threads
 * for handling of higher priority interrupts.
 * 
 * While processing of error interrupts is potentially a very complicated
 * situation in SMP environments it is by no means a hopeless one.  It is made
 * manageable by having the appropriate interrupt service routine execute the 
 * following general steps once it detects an adapter reported error:
 *
 * 1. The PCCB is locked to synchronize access to port registers.
 * 2. Bus and port register register contents are cached.
 * 3. A second determination is made as to whether an error is being reported
 *    by the adapter using the cached register contents.
 * 4. Interrupt processing is allowed to proceed( after unlocking the PCCB ) if
 *    no error is detected on the second check for errors.
 * 4. Soft errors are cleared or ignored and interrupt processing is allowed to
 *    proceed( after unlocking the PCCB ) as if no error had been detected.
 * 5. Hard errors crash the local port and terminate interrupt processing(
 *    after unlocking the PCCB ).
 *
 * The major benefit derived from executing this general sequence is that
 * threads complete error processing in the order in which they obtain the
 * appropriate PCCB lock after detecting an error.  This adds back a
 * deterministic element to error processing in SMP environments.
 *
 * A second benefit derived from this sequence is the reduction in the effect
 * new error interrupts have on existing threads attempting to process previous
 * adapter reported errors.  This reduction is achieved through utilization of
 * cached register contents in the determination of the nature of the reported
 * error.  It allows error processing to proceed in a consistent fashion.  New
 * error interrupts may still occur and still change register contents;
 * however, processing of new interrupts by newly spawned threads no longer has
 * major affect on error processing by existing threads.
 *
 * Use of cached register contents is not without its disadvantages though.
 * While determination of the nature of the error proceeds from cached
 * contents, logging of the event utilizes the contents of the registers
 * themselves.  Thus, registers logged may bear no relation to the event logged
 * when multiple virtually simultaneous( ie - one after the other ) errors
 * occur.  Such synchronicity should be extremely rare and does not outweigh
 * the advantages of using cached register contents.
 *
 * Use of cached register contents also does not completely eliminate the
 * effect of new error interrupts on processing of previous interrupts by
 * completely independent and concurrent threads.  This can best be observed
 * through use of a hypothetical example.  Consider the following set of time
 * sequential virtually simultaneous interrupts originated on the same local
 * port:
 *
 * 1. Interrupt to report response availability occurs.
 * 2. Interrupt to report soft error occurs.
 * 3. Interrupt to report a fatal error occurs.
 *
 * The following scenario may be concocted in order to demonstrate that while
 * side effects on interrupt processing resulting from new error interrupts in
 * SMP environments may be minimized, it may never be completely eliminated.
 *
 * 1. The interrupt reporting response availability occurs and is received by
 *    Processor 1.  Before the thread processing the interrupt( T1 ) has the
 *    opportunity to check for errors it is pre-empted by an interrupt with a
 *    higher priority.
 * 2. The interrupt reporting the soft error occurs and is received by
 *    Processor 2.  The thread processing the interrupt( T2 ) detects the
 *    error, locks the appropriate PCCB, and caches register contents.
 * 3. The interrupt reporting the fatal error occurs and is received by
 *    Processor 3.  The thread processing the interrupt( T3 ) detects the error
 *    but is unable to lock the appropriate PCCB because it is locked by T2.
 *    It spins waits waiting for the PCCB lock to become available.
 * 4. T1 resumes execution, checks for errors, notices that an error exists,
 *    and attempts to lock the appropriate PCCB.  It too spin waits waiting for
 *    the PCCB lock( locked by T2 ) to become available.
 * 5. T2 checks its cached register contents for errors, discovers the soft
 *    error, clears/ignores it, unlocks the PCCB, and proceeds to determine
 *    whether responses are available for processing.  As there are responses
 *    available, it proceeds to process them even though a fatal error has
 *    occurred since response availability was initially signaled.  Thread
 *    execution is terminated following processing of all available responses.
 * 6. T1 wins the race with T3 for the PCCB lock.  After obtaining the lock, it
 *    caches register contents, checks them, and determines that a fatal error
 *    has occurred.  It proceeds to crash the local port, unlock the PCCB and
 *    terminate thread execution.
 * 7. T3 obtains the PCCB lock, caches register contents, checks them, and
 *    discovers the lack of an error.  It does not find an error because
 *    crashing of the local port by T1 involved disabling it, and the act of
 *    disabling a local port resets its registers.  Thus, when T3 caches
 *    register contents, it caches contents which do not display an error.  T3
 *    proceeds to unlock the PCCB and continue as if no error had been
 *    initially detected.  When T3 checks explicitly for response availability
 *    it finds none, once again begin the registers have been reset, and the
 *    interrupt is dismissed.
 *
 * Many scenarios such as this one are possible.  What is evident in all of
 * them is that a potential for side effects is introduced when error
 * interrupts occur while prior interrupts are currently being processed.
 * There is no way to eliminate them short of always locking the PCCB when an
 * interrupt occurs and this choice is unacceptable for performance reasons.
 * Fortunately, the impact of such side effects may be minimized and that has
 * been one of the guiding principles during development of the methodology for
 * processing errors in SMP environments.
 * 
 * One final note, differences do exist in how individual interrupt service
 * routines go about executing the general steps outlined above on initial
 * detection of errors.  This is to be expected because of the differences in
 * port interfaces between the various hardware port types.  If such
 * differences did not exist there would be no need for separate interrupt
 * service routines in the first place.  However, every CI port driver
 * interrupt service which must be prepared for execution within SMP
 * environments( cibci_isr(), cibca_isr(), cixcd_isr()) does follow these
 * general steps on detection of adapter reported errors.
 *
 * NOTE: This discussion pertains only to processing in SMP environments by the
 *	 primary interrupt service routines( ci780_isr(), cibci_isr(),
 *	 cibca_isr(), cixcd_isr()).  It does not pertain to any processing by
 *	 the special routine( ci_unmapped_isr()) used as interrupt service
 *	 handler by local ports temporarily without power( CI750/CI780//CIBCI
 *	 only ) or marked broken and permanently shutdown.  The special routine
 *	 itself contains a full description of any additional processing
 *	 requirements brought about by the introduction of SMP environments.
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
#include		"../io/xmi/xmireg.h"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"../io/scs/scamachmac.h"
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
#include		"../io/ci/ciport.h"
#include		"../io/ci/ciadapter.h"

/* External Variables and Routines.
 */
extern	PB		*cippd_get_pb();
extern	u_long		gvp_queue_retry, ci_map_port();
extern	void		ci_crash_lport(), ci_dealloc_pkt(), ci_init_port(),
			ci_log_dev_attn(), ci_log_packet(), ci_rsp_handler(),
			ci_unmap_lport(), ci_update_cable(), cippd_crash_pb(),
			cippd_receive(), cippd_reqid_snt(), gvp_add_dg(),
			gvp_add_msg(), scs_data_done(), scs_dg_rec(),
			scs_msg_rec(), scs_msg_snt();


/*   Name:	ci_rsp_handler	- Process All CI Port Responses
 *
 *   Abstract:	This routine is responsible for processing all entries on a CI
 *		port's response queue.  It is invoked by a CI port interrupt
 *		service routine whenever a port places a response onto a
 *		previously empty response queue and requests an interrupt to
 *		notify the port driver of the queue transition.
 *
 *		There are several types of responses including:
 *
 *		1. Received packets.
 *		2. Locally initiated port commands with requested statuses.
 *		3. Locally initiated port commands with error statuses.
 *		4. Local port initiated commands with error statuses.
 *		5. Remotely initiated port commands with error statuses.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *   gvp_queue_retry		- Queuing failure retry count
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	ppd.cippd		-  CI PPD specific PB fields
 *	    pstatus.path_closed	-   Path already closed by port flag
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.pqb.rspq		-  Port response queue
 *
 *   SMP:	The PCCB is locked for PB retrieval and prevents premature PB
 *		deletion.  This is required only when processing packet
 *		reported errors and when updating cable statuses following the
 *		reception of identification requests and loopback response
 *		packets.  PCCB addresses are always valid because these data
 *		structures are never deleted once their corresponding ports
 *		have been initialized.
 *
 *		PBs are locked to synchronize access and prevent premature
 *		deletion.  This is required only when processing packet
 *		reported errors and when updating cable statuses following the
 *		reception of identification requests.
 *
 *		Access to port response queues is by means of memory
 *		interlocking queuing instructions.
 */
void
ci_rsp_handler( pccb )
    register PCCB	*pccb;
{
    register PB		*pb;
    register GVPH	*cibp;

    /* Each response is removed from the specified port's response queue and
     * checked for errors.  These errors fall into one of the following
     * categories based upon the level of error recovery required.
     *
     * 1. Informational Errors.
     *	  The error is processed and response processing continues as if the
     *	  error had never been reported.  This may involve placing the response
     * 	  onto the appropriate free queue if this is what the port would have
     *	  done itself had it not needed to report the error.  Errors classified
     *	  as informational include:
     *
     *		- Cable Failed but Port Command Succeeded.  This status is
     *		  returned whenever a cable fails during the successful
     *		  execution of those local port commands which initiate packet
     *		  transmission and do not explicitly specify the cable to be
     *		  used.  Error processing consists of updating PB cable status
     *		  and logging any good->bad cable transition.  Such processing
     *		  is required only for fully established paths.
     *
     *		- Lack of a Good Cable Forced Local Port Command Failure.  This
     *		  status may be returned by locally executed REQID and SNDLB
     *		  port commands.  Both commands initiate packet transmission
     *		  over specific cables; however, the meaning of this status is
     *		  different for each command as is the error processing on
     *		  detection of this status.
     *
     *		  SNDLB: A NOPATH status indicates failure to loopback to the
     *			 local port over the specified cable.  No error
     *			 processing is required and the packet is immediately
     *			 disposed of.  Note that final disposition of the
     *			 packet is NOT the same as if the error had never
     *			 occurred.  There is no need to queue the packet to
     *			 the appropriate local port datagram free queue for
     *			 reception of a loopback response because no response
     *			 is forthcoming due to failure of the port command.
     *			 Instead the packet is just deallocated.
     *
     *		  REQID: A NOPATH status indicates failure of the specified
     *		  	 cable and is considered only informational in nature
     *		  	 whenever no record of the path currently exists within
     *		  	 the system-wide configuration database, the path is
     *			 not established, or at least one other operational
     *		  	 cable is still known.  Error processing consists of
     *			 updating the PB's cable status, logging any good->bad
     *		  	 cable transition, and forcing transmission of a REQID
     *		  	 over the other cable to determine if it too is bad.
     *			 Such processing is required only for fully established
     *			 paths.
     *
     *		- Unrecognized Packet.  This status is generated whenever the
     *		  port receives a SNDSTRT or SNDRST maintenance packet while in
     *		  the enabled state.  Such statuses are not totally unexpected.
     *		  Remote MSCP clients request resetting and restarting of local
     *		  ports whenever their logical SCS connections to the local
     *		  MSCP server times out and local ports are always to be found
     *		  in the enabled state.  Error processing consists of just
     *		  disposing of the response.
     *
     * 2. Severe Errors.  
     *    The error is logged, the appropriate path is crashed and the response
     *	  is disposed of either by deallocating the buffer( locally executed
     *	  command packets with the R bit set ) or adding it to the appropriate
     *	  port free queue( received packets, local port generated port command
     *	  packets and locally executed port command packets with the R bit
     *	  clear ).  Response processing continues with the next response on the
     *	  queue.  Errors classified as severe include:
     *
     *		- Virtual Circuit Closed by Previous Command.  This status may
     *		  be returned by locally executed port commands which initiate
     *		  packet transmission over a virtual circuit.  The current
     *		  response is just disposed of.  There is no need to log the
     *		  event or crash the path because these actions have already
     *		  taken place.
     *
     *		- Lack of a Good Cable Forced Local Port Command Failure.  This
     *		  status may be returned by any locally executed port commands
     *		  which initiate packet transmission.  The occurrence of such
     *		  an error may close the associated virtual circuit.  The path
     *		  is crashed provided it has not already failed.  Additional
     *		  error processing is also required in the case of those REQIDs
     *		  which had attempted packet transmission over specific cables
     *		  previously thought to be good.  Such processing includes
     *		  updating the PB's cable status and logging any good->bad
     *		  cable transition before logging the error and crashing the
     *		  path.
     *
     *		- Buffer Memory System Error Occurred.  This status may be
     *		  returned within block data transfer packets which return
     *		  status about block data transfer operations.  The occurrence
     *		  of such an error also automatically closes the associated
     *		  virtual circuit.
     *
     *		- Packet Size Violation.  This status may be returned by
     *		  remotely initiated locally executed port commands which
     *		  initiate packet transmission and also within received
     *		  packets.  The occurrence of such an error may also
     *		  automatically close the associated virtual circuit.
     *
     *		- Unrecognized Packet.  This status is generated whenever the
     *		  port receives a packet with an illegal opcode or flags field,
     *		  or an invalid source source address, or a maintenance packet
     *		  when the port is in the enabled state.
     *
     *		- Out-of-Sequence Message Received.  This status may only be
     *		  returned within received packets.  It MAY now be obsolete.
     *		  It is not documented and might just have been used in some
     *		  interim CI ucode for debugging purposes.  Presumably, the
     *		  occurrence of such an error also automatically closes the
     *		  associated virtual circuit.
     *
     *		- Message Received with Closed Virtual Circuit Descriptor.
     *		  This status may only be returned only within received
     *		  packets.  It MAY now be obsolete.  It is not documented and
     *		  might just have been used in some interim CI ucode for
     *		  debugging purposes.  Presumably, the occurrence of such an
     *		  error also automatically closes the associated virtual
     *		  circuit.
     *
     * 3. Fatal Errors.
     *	  The local port is crashed and response processing is immediately
     *	  terminated.  The response is disposed of and any responses remaining
     *	  on the queue are flushed during crashing of the port.  Errors
     *    classified as fatal include:
     *
     *		- Invalid Local Buffer Name.  This status may be returned by
     *		  local block data transfer port commands.  The occurrence of
     *		  such an error also automatically closes the associated
     *		  virtual circuit.
     *
     *		- Local Buffer Length Violation.  This status may be returned
     *		  by local block data transfer port commands.  The occurrence
     *		  of such an error also automatically closes the associated
     *		  virtual circuit.
     *
     *		- Access Control Violation during Local Buffer Access.  This
     *		  status may be returned by local block data transfer port
     *		  commands.  The occurrence of such an error also automatically
     *		  closes the associated virtual circuit.
     *
     *		- Packet Size Violation.  This status may be returned by
     *		  locally executed port commands which initiate packet
     *		  transmission.  In addition, loopback responses( LBREC ) may
     *		  also return this status.  These responses are both locally
     *		  initiated and executed because of how loopback is used by the
     *		  CI port driver.  The occurrence of such an error may also
     *		  automatically close the associated virtual circuit.
     *
     *		- Invalid Destination Port.  This status may be returned by
     *		  those locally and remotely initiated, locally executed port
     *		  commands which require specification of a destination port.
     *		  The occurrence of such an error may also automatically close
     *		  the associated virtual circuit.
     *
     *		- Unknown Local Port Command.  This status is generated
     *		  whenever a local port executes a command with an invalid or
     *		  unimplemented opcode field, invalid flags field, or non-zero
     *		  status field.
     *
     *		- Aborted Local Port Command Encountered.  Packets with this
     *		  status should never be encountered.  Only those port commands
     *		  currently being processed by the port when it transitions
     *		  into the disabled state can have this status.  However once
     *		  a port enters the disabled state it either is in the process
     *		  of being crashed or shortly will be, and all packets on all
     *		  port queues are flushed during crashing of the port.
     *
     *		- All Other( Unknown ) Error Statuses.
     *
     * If a response does not contain any errors, or the error is only
     * informational in nature, then the response is itself processed according
     * to its port operation code.  Processing then continues with the next
     * response on the port's response queue.  There is one exception to this
     * rule: detection of a NOPATH status in a SNDLB packet.  The detection of
     * this informational error forces deallocation of the packet instead of
     * queuing it to the appropriate local port datagram free queue as would
     * have automatically taken place had not the error occurred.  Deallocation
     * of the packet is forced by explicit setting of the response bit within
     * the flags field of the generic Vaxport driver header.
     *
     * Response processing terminates either when this queue is exhausted or a
     * fatal error is detected triggering crashing of the port.  The port is
     * also crashed if the queue interlock can not be obtained.
     *
     * Only cable statuses of open paths are ever updated.
     */

    do	{
	Remqhi_rspq( pccb, cibp )
	if( cibp == NULL ) {
	    break;
	}
	cibp->opcode &= 0x3f;
	if( cibp->status.failure ) {
	    u_long		crash_lport = 0;
	    register u_long	dispatch = 0, event = 0;

	    Lock_pccb( pccb )
	    if(( pb = cippd_get_pb( pccb, Pd_to_scs( cibp, pccb ), BUF ))) {
		Lock_pb( pb )
	    }
	    switch((( cibp->status.type != T_OTHER )
			? cibp->status.type
			: ( T_OTHER + cibp->status.subtype ))) {

		case T_OK:
		    dispatch = 1;
		    if( pb && pb->pinfo.state == PS_OPEN ) {
			( void )ci_update_cable( pccb, pb, cibp, CABLE_GB );
		    }
		    break;

		case T_VCC:
		    break;

		case T_INVBNAME:
		    crash_lport = 1;
		    event = SE_INVBNAME;
		    break;

		case T_INVBSIZE:
		    crash_lport = 1;
		    event = SE_INVBSIZE;
		    break;

		case T_ACCVIO:
		    crash_lport = 1;
		    event = SE_BACCVIO;
		    break;

		case T_NOPATH:
		    if( cibp->opcode == IDREQ ) {
			dispatch = 1;
			if( Cselect( cibp ) != CS_AUTO ) {
			    if( pb && pb->pinfo.state == PS_OPEN ) {
				( void )ci_update_cable( pccb,
							 pb,
							 cibp,
							 CABLE_GB );
				if( pb->Pstatus.cable0 && pb->Pstatus.cable1 ){
				    event = E_NOCABLES;

				/* The response IDREQ packet is not re-used for
				 * the transmission of a REQID over the other
				 * cable.  Its processing must continue in case
				 * it represents the REQID currently being
				 * sanity checked.
				 */
				} else {
				    register u_long	save_cable;

				    save_cable = pccb->Poll_cable;
				    if( Cselect( cibp ) == CS_CABLE0 ) {
					pccb->Poll_cable = CS_CABLE1;
				    } else {
					pccb->Poll_cable = CS_CABLE0;
				    }
				    ( void )ci_send_reqid( pccb,
							   NULL,
						 	   Get_pgrp( pccb,cibp),
							   DEALLOC_BUF );
				    pccb->Poll_cable = save_cable;
				}
			    }
			} else if( pb && pb->pinfo.state != PS_PATH_FAILURE ) {
			    event = E_NOCABLES;
			}
		    } else if( cibp->opcode == LBSNT ) {
			cibp->flags.rsp = 1;
		    } else if( cibp->opcode == DGSNT   ||
				cibp->opcode == RSTSNT ||
				cibp->opcode == STRTSNT ) {
			if( pb && pb->pinfo.state != PS_PATH_FAILURE ) {
			    event = E_NOCABLES;
			}
		    } else if( pb ) {
			pb->Fsmpstatus.path_closed = 1;
			if( pb->pinfo.state != PS_PATH_FAILURE ) {
			    event = E_NOCABLES;
			}
		    }
		    break;

		case T_BMSE:
		    event = SE_BMSE;
		    break;

		case( T_OTHER + ST_PSVIO ):
		    if( cibp->opcode == DGSNT   ||
			 cibp->opcode == MSGSNT ||
			 cibp->opcode == DATSNT ||
			 cibp->opcode == DATREC ||
			 cibp->opcode == LBSNT  ||
			 cibp->opcode == LBREC ) {
			crash_lport = 1;
			event = SE_INVLPKTSIZE;
		    } else {
			event = SE_INVRPKTSIZE;
			if( cibp->opcode != DGREC && pb ) {
			    pb->Fsmpstatus.path_closed = 1;
			}
		    }
		    break;

		case( T_OTHER + ST_UPKT ):
		    if( cibp->opcode != SNDRST && cibp->opcode != SNDSTRT ) {
			event = SE_UNRECPKT;
		    }
		    break;

		case( T_OTHER + ST_DPORT ):
		    crash_lport = 1;
		    event = SE_INVDPORT;
		    break;

		case( T_OTHER + ST_UCMD ):
		    crash_lport = 1;
		    event = SE_UNKCMD;
		    break;

		case( T_OTHER + ST_ABORT ):
		    crash_lport = 1;
		    event = SE_ABORTPKT;
		    break;

		case( T_OTHER + ST_INVPA ):
		    crash_lport = 1;
		    event = SE_INVPA;
		    break;

		case( T_OTHER + ST_INVSN ):
		    crash_lport = 1;
		    event = SE_INVSN;
		    break;

		case( T_OTHER + ST_IRESVCD ):
		    crash_lport = 1;
		    event = SE_IRESVCD;
		    break;

		case( T_OTHER + ST_IRESEQ ):
		    crash_lport = 1;
		    event = SE_IRESEQ;
		    break;

		case( T_OTHER + ST_DISCVCPKT ):
		    crash_lport = 1;
		    event = SE_DISCVCPKT;
		    break;

		case( T_OTHER + ST_INVDDL):
		    crash_lport = 1;
		    event = SE_INVDDL;
		    break;

		default:
		    crash_lport = 1;
		    event = SE_UNKSTATUS;
		    break;
	    }
	    if( event ) {
		if( crash_lport == 0 ) {
		    ( void )ci_log_packet( pccb, pb, cibp, event, PATH_EVENT );
		    ( void )cippd_crash_pb( pccb,
					    pb,
					    (( Eseverity( event ) == ES_E )
						? E_PD : SE_PD ),
					    0,
					    NULL );
		} else {
		    ( void )ci_crash_lport( pccb,
					    event,
					    Pd_to_scs( cibp, pccb ));
					    
		}
	    }
	    if( pb ) {
		Unlock_pb( pb )
	    }
	    Unlock_pccb( pccb )
	    if( crash_lport ) {
		return;
	    }
	    if( !cibp->flags.rsp ) {
		if( cibp->opcode == IDREC ||
		     cibp->opcode == IDREQ ||
		     cibp->opcode == DGSNT ||
		     cibp->opcode == DGREC ||
		     cibp->opcode == LBSNT ||
		     cibp->opcode == LBREC ||
		     ( cibp->status.type == T_OTHER &&
		       cibp->status.subtype == ST_UPKT )) {
		    ( void )gvp_add_dg( pccb, Pd_to_scs( cibp, pccb ));
		} else {
		    ( void )gvp_add_msg( pccb, Pd_to_scs( cibp, pccb ));
		}
		continue;
	    } else if( !dispatch ) {
		( void )ci_dealloc_pkt( cibp );
		continue;
	    }
	}

	/* Responses with good or informational status are processed according
	 * to their port operation codes as follows:
	 *
	 * Response		Processing
	 *
	 * Received/Transmitted	- SCS processes and disposes of response.
	 *  Sequence Message
	 *
	 * Completed Block Data	- SCS is notified of block data transfer
	 *  Transfer		  completion.
	 *			- Port driver disposes of response.
	 *
	 * Received Datagram 	- SCS processes and disposes of datagram(
	 *			  application datagrams only ).
	 *			- CI PPD processes and disposes of datagram(
	 *			  all other datagrams ).
	 *
	 * Invalidate		- Port driver disposes of response.
	 *  Translation Cache
	 *  Command
	 * Transmitted Datagram
	 * Reset Request
	 * Start Request
	 * Set Circuit Command
	 *
	 * Transmitted		- CI PPD is notified of request identification
	 *  Identification	  command completion.
	 *  Request		- Port driver disposes of response.
	 *
	 * Received		- The appropriate PB's cable status is updated(
	 *  Identification	  open paths only ).
	 *  Packet		- Log all crossed->uncrossed,
	 *			  uncrossed->crossed, and bad->good cable
	 *			  transitions( open paths only ).
	 *			- CI PPD processes and disposes of response.
	 *
	 * Received		- The appropriate PCCB's loopback cable status
	 *  Loopback Response	  is updated.
         *			- Log all loopback bad->good transitions.
	 *			- Port driver disposes of response.
	 *
	 * All other port	- The local port is crashed.
	 *  operation codes	- Response processing is terminated.
	 *
         * Received sequenced messages, datagrams, and block data transfer
         * completion packets are checked for and processed BEFORE dispatching
         * on the operation code contained within the response.  Such special
         * handling results in significantly improved performance.
	 */
	{
	register SCSH		*scsbp = Pd_to_scs( cibp, pccb );
	register GVPPPDH	*cippdbp = Pd_to_ppd( cibp, pccb );

	if( cibp->opcode == MSGREC ) {
	    ( void )scs_msg_rec( pccb, scsbp, Appl_size( cippdbp ));
	    continue;
	} else if( cibp->opcode == CNFREC || cibp->opcode == DATREC ) {
	    ( void )scs_data_done( pccb, scsbp, &Cnfrec( cippdbp )->xctid );
	} else if ( cibp->opcode == DGREC ) {
	    if( cippdbp->mtype == SCSDG ) {
		( void )scs_dg_rec( pccb, scsbp, Appl_size( cippdbp ));
	    } else {
		( void )cippd_receive( pccb,
				       cippdbp,
				       ( u_long )cippdbp->mtype );
	    }
	    continue;
	} else {
	    switch( cibp->opcode ) {

		case DGSNT:
		case RSTSNT:
		case STRTSNT:
		case CKTSET:
		case TCINV:
		    break;

		case MSGSNT:
		    ( void )scs_msg_snt( pccb, scsbp, Appl_size( cippdbp ));
		    continue;

		case IDREQ:
		    ( void )cippd_reqid_snt( pccb, Get_pgrp( pccb, cibp ));
		    break;

		case IDREC:
		    Lock_pccb( pccb )
		    if(( pb = cippd_get_pb( pccb, scsbp, BUF ))) {
			Lock_pb( pb )

			/* Crash the path if the remote port is not in an
			 * appropriate state.
			 */
			if( pb->pinfo.state == PS_OPEN ) {
			    if( Idrec( cippdbp )->port_state != PS_ENAB &&
				Idrec( cippdbp )->port_state != PS_ENAB_MAINT){
				( void )ci_log_packet( pccb,
						       pb,
						       cibp,
						       E_RPORTSTATE,
						       PATH_EVENT );
				( void )cippd_crash_pb( pccb,
							pb,
							E_PD,
							0,
							NULL );
				Unlock_pb( pb )
				Unlock_pccb( pccb )
				( void )gvp_add_dg( pccb, scsbp );
				continue;
			    } else {
				( void )ci_update_cable( pccb,
							 pb,
							 cibp,
							 CABLE_CROSSED );
				( void )ci_update_cable( pccb,
							 pb,
							 cibp,
							 CABLE_BG );
			    }
			}
			Unlock_pb( pb )
		    }
		    Unlock_pccb( pccb )
		    ( void )cippd_receive( pccb, cippdbp, CNFE_ID_REC );
		    continue;

		case LBREC:
		    Lock_pccb( pccb )
		    ( void )ci_update_cable( pccb, NULL, cibp, CABLE_LB_BG );
		    Unlock_pccb( pccb )
		    break;

		case MSGREC:	case CNFRET:	case DATREQ0:	case DATREQ2:
		case CNTRD:	case LBSNT:	case MDATSNT:	case MDATREQ:
		case MCNFREC:	case MDATREC:	case CNFREC:	case DATREC:
		case DGREC:
		    ( void )ci_crash_lport( pccb, SE_INVOPCODE, scsbp );
		    return;

		default:
		    ( void )ci_crash_lport( pccb, SE_UNKOPCODE, scsbp );
		    return;
	    }
	}
	( void )ci_dealloc_pkt( cibp );
	}
    }	while( pccb->Pqb.rspq.flink );
}

/*   Name:	ci_unmapped_isr	- Unmapped CI Adapter Interrupt Service Routine
 *
 *   Abstract:
 *
 *   Inputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   0
 *	    lpstatus.power	-   Port has power status flag
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   Port is broken status flag
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.power	-   Port has power status flag
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   Port is broken status flag
 *	    fsmstatus.cleanup	-   Cleanup in progress status flag
 *
 *   SMP:
 */
void
ci_unmapped_isr( pccb )
    register PCCB	*pccb;
{
    register u_long	cnfr, event, log_regs = LOG_REGS, save_ipl;

    /* The steps involved in processing an interrupt from an unmapped local CI
     * port are as follows:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The PCCB is locked.
     * 3. The mapped state of the local port is verified.
     * 4. The local port is mapped.
     * 5. The interrupt is processed according to its nature.
     * 6. The interrupt is logged.
     * 7. The local port is disabled.
     * 8. The PCCB is unlocked.
     * 9. IPL is restored.
     *
     * The local port is always unmapped on entry into this routine; however it
     * may be found to be mapped following locking of the PCCB( Step 2 ).
     * There is only one way this can occur, if another thread were to process
     * an interrupt notifying the port driver of power availability on the
     * local port.  Such processing leaves the local port mapped because the
     * severe error which caused the port to be unmapped in the first place(
     * power loss ) has been alleviated.  It also removes any reason for
     * continued execution of the current thread.   Either the power up
     * interrupt processed by the other thread was originally designated for
     * the current one, or the interrupt designated for the current thread was
     * superseded by the power up interrupt processed by the other one.  In any
     * event, there is no reason for the current thread to continue execution
     * when it finds the local port mapped, and this is why the mapped state of
     * the local port is verified( Step 3 ) after the PCCB is unlocked.
     *
     * This routine services interrupts only when the local port is temporarily
     * without power or marked marked broken and permanently shutdown.  A panic
     * is issued in any other circumstance.
     *
     * All interrupts on broken local ports are considered to be stray.
     * Processing of these stray interrupts includes re-mapping of the local
     * port adapter I/O space( Step 4 ).  This allows access to local port
     * registers for the purpose of logging the interrupt( Step 6 ) and
     * disabling the local port( Step 7 ).  Note that only the adapter I/O
     * space is re-mapped.  The interrupt service handler for the local port is
     * NOT restored to the routine appropriate for the local port hardware port
     * type.  All interrupts on this local port continue to be serviced by this
     * special routine.  Furthermore, this re-mapping is only temporary.  The
     * local port is automatically returned to the unmapped state in which it
     * was found at the beginning of interrupt processing during disablement of
     * the local port( Step 7 ).
     *
     * Interrupts on local ports temporarily without power( CI750/CI780/CIBCI
     * only ) may occur for many reasons.  In all cases the first action taken
     * is to fully re-map the local port( Step 4 ).  This includes restoration
     * of the appropriate routine as interrupt service handler for the local
     * port.  Full re-mapping is done because the most likely reason for an
     * interrupt from such a port is notification of restoration of port power.
     * Such notification is the only interrupt reason which results in the
     * local port left fully mapped at the conclusion of interrupt processing.
     * As such, it is just more efficient to fully re-map local ports
     * temporarily without power during processing of their interrupts.
     *
     * Note that full re-mapping of such local ports is not without its
     * consequences.  Restoration of the normal routine as interrupt service
     * handler forces subsequent interrupts to be processed by that routine,
     * even if the current interrupt has not yet been fully deallt with.
     * Fortunately, such sequences of events can only occur on those local 
     * ports subject to independent power loss which may function also in SMP
     * environments( CIBCI only ).
     * All routines which normally serve as interrupt service handlers for such
     * local ports( cibci_isr()) have been modified to deal correctly with
     * these situations when they occur.
     *


     * Once the local port has been re-mapped, the exact cause of the interrupt
     * may be ascertained from cached contents of the bus specific
     * configuration register.  Cached contents are employed
 
     * 1. Port power up.
     * 2. Port power down.
     * 3. CI adapter not present( CI750/CIBCI only ).
     *
     * Any other interrupt is unexpected and is treated as a stray interrupt.
     *

     */
    if( pccb == NULL ) {
	return;
    }
    save_ipl = Splscs();
    Lock_pccb( pccb )
    if( pccb->Lpstatus.mapped ) {
	Unlock_pccb( pccb )
	( void )splx( save_ipl );
	return;
    }
    if( pccb->Fsmstatus.broken ) {
	( void )ci_map_port( pccb, MAP_REGS );
	event = W_STRAY;
    } else if( !pccb->Lpstatus.power ) {
	if( ci_map_port( pccb, MAP_FULL ) == RET_SUCCESS ) {
	    cnfr = *pccb->Cnfr;
	    switch( pccb->lpinfo.type.hwtype ) {

		case HPT_CI750:
		case HPT_CI780:
		    if( cnfr & CI780_CNF_PUP ) {
			event = I_POWERUP;
			pccb->Lpstatus.power = 1;
		    } else if( cnfr & CI780_CNF_PDWN ) {
			event = W_POWER;
		    } else if( cnfr & CI780_CNF_NOCI ) {
			event = FE_NOCI;
			pccb->Fsmstatus.broken = 1;
		    } else {
			event = W_STRAY;
		    }
		    break;

		case HPT_CIBCI:
		    if( cnfr & CIBCI_CNF_PUP ) {
			event = I_POWERUP;
			pccb->Lpstatus.power = 1;
		    } else if( cnfr & CIBCI_CNF_PDWN ) {
			event = W_POWER;
		    } else if( cnfr & ( CIBCI_CNF_NOCI | CIBCI_CNF_DCLO )) {
			event = FE_NOCI;
			pccb->Fsmstatus.broken = 1;
		    } else {
			event = W_STRAY;
		    }
		    break;

		default:
		    ( void )panic( PANIC_HPT );
	    }
	} else {
	    event = FE_NOCI;
	    pccb->Fsmstatus.broken = 1;
	}
	if(( pccb->Lpstatus.power || pccb->Fsmstatus.broken ) &&
	     !pccb->Fsmstatus.cleanup ) {
	    pccb->Fsmstatus.cleanup = 1;
	    Pccb_fork(  pccb, ci_init_port, PANIC_PCCBFB );
	}
    } else {
	( void )panic( PANIC_BADUNMAP );
    }
    ( void )ci_log_dev_attn( pccb, event, LOG_REGS );
    ( void )( *pccb->Disable_port )( pccb, PS_UNINIT );
    Unlock_pccb( pccb )
    ( void )splx( save_ipl );
}

/*   Name:	ci780_isr	- CI750/CI780 Interrupt Service Routine
 *
 *   Abstract:	This routine is the primary interrupt service routine for CI750
 *		and CI780 local ports.  It services all interrupts except:
 *
 *		1. When a local port adapter has lost power.
 *		2. When a local port is marked broken and permanently shutdown.
 *
 *		At such times the local port is unmapped and all interrupts are
 *		serviced instead by ci_unmapped_isr().  
 *
 *		CI750 adapters are located within their own physically separate
 *		cabinets.  They may lose power independently of the rest of the
 *		system.  They may also become uncabled from the CMI on which
 *		they nominally reside leading to their local ports being marked
 *		broken and permanently shutdown.  When either of these events
 *		occurs, much( but not all ) of adapter I/O space becomes
 *		unaccessible and any access attempt results in a machine check.
 *		It is as a protection against these extraneous machine checks
 *		that the local port is unmapped and interrupts re-directed to
 *		ci_unmapped_isr().
 *
 *		CI780 adapters do NOT suffer from the same handicaps as CI750
 *		adapters.  They are not located within their own physically
 *		separate cabinets and are not subject to extraneous machine
 *		checks.  There is no need to unmap a local CI780 port on loss
 *		of power.  However, such ports are unmapped anyway on loss of
 *		power because it is not worth distinguishing between two
 *		hardware port types( CI750 and CI780 ) which are otherwise so
 *		identical on the occurrence of an extremely rare condition that
 *		the remainder of the system normally panics on.  As for
 *		unmapping CI780 adapters whenever they are marked broken and
 *		are permanently shutdown, all local ports regardless of their
 *		hardware port types undergo such unmapping, and there is no
 *		reason to do anything different for CI780 local ports.
 *
 *   Inputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.type.hwtype	-  HPT_CI750 or HPT_CI780
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *	    lpstatus.power	-   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *
 *   Outputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.cnfr	-   Configuration register pointer
 *	    ciregptrs.psrcr	-   Port status release control reg pointer
 *	    lpstatus.power	-   Port has power status flag
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   Port is broken status flag
 *
 *   SMP:	No locks are required even though the PCCB may be manipulated.
 *		Locks are never required for normal( ie - non-error ) interrupt
 *		processing for the following reasons:
 *
 *			1. No changes to interlocked structures are made.
 *			2. PCCB references are only to static fields.
 *			3. Port registers either indicate no errors at time of
 *		   	   access or can not change until released.
 *
 *		Locks are also not required during error processing because
 *		CI750 and CI780 local ports are NEVER present in SMP
 *		environments.
 */
void
ci780_isr( pccb )
    register PCCB	*pccb;
{
    register u_long	save_ipl, cnfr, psr, lpcrash;

    /* The steps involved in processing an interrupt are as follows:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The port configuration and status registers are checked for errors.
     * 3. Responses on the port response queue are processed after releasing
     *	  the port status register.
     * 4. IPL is restored.
     *
     * CI750/CI780 port configuration register errors include:
     *
     * 1. SBI faults( CI780 only ).
     * 2. Corrected read datas( CRDs ).
     * 3. Port power up.
     * 4. Port power down.
     * 5. CI adapter not present( CI750 only ).
     * 6. Memory system errors( NXM, UCE, and RLTO on CI750; CXTERR, CXTMO, and
     *	  RDS on CI780 ).
     * 7. Adapter port parity errors( CI750 only ).
     * 8. Miscellaneous errors( PFD and any undefined errors; TDCLO, TACLO,
     *    and CTO on CI750; TDEAD, and TFAIL on CI780  ).
     *
     * CI750/CI780 port status register errors include:
     *
     * 1. Internal hardware failures( reported as parity errors ).
     * 2. Sanity time expiration.
     * 3. Data structure error.
     * 4. Message free queue empty.
     * 5. Miscellaneous errors( MISC on CI750 or undefined errors ).
     *
     * SBI faults( CI780 ) are totally ignored because they are processed
     * elsewhere.  CRDs are cleared and then ignored because they are just
     * informational.  Processing of the interrupt then proceeds as if the
     * error had never occurred.  The existence of any other error forces
     * bypassing of response processing and triggers crashing, clean up, and
     * re-initialization of the local port.  
     *
     * Port power down and CI adapter absent( CI750 only ) are special cases.
     * Both trigger crashing, disablement, and clean up of the affected local
     * port.  Both result in scheduling of local port initialization.  However,
     * while local port initialization is postponed while port power is
     * unavailable, no attempt is made to initialize absent local ports.  They
     * are just shutdown and left permanently offline.  A more detailed
     * description of possible power failure recovery scenarios may be obtained
     * by consulting ci_crash_lport().
     *
     * There is one special port status register miscellaneous error which
     * requires further explanation.  Broken link modules( L0100 ) may result
     * in link module acceptance of all packets transmitted on the CI.
     * Needless to say this can cause havoc especially when the module begins
     * to ACK/NACK packets not really addressed to it causing ACK/NACK packet
     * collisions and a great deal of confusion.  CI port microcode has been
     * upgraded( hopefully ) to compare packet destination with local node
     * addresses and to signal mismatches by means of a special code in the
     * PESR.  Action taken by the CI port driver on discovery of this condition
     * includes crashing the port, marking it broken, and taking it permanently
     * offline.
     *
     * NOTE: Port power up unlike port power down is not a special case.  It is
     *	     treated the same as any other fatal port error, triggering
     *	     crashing, disablement, clean up, and initialization of the
     *	     affected local port.
     */
    save_ipl = Splscs();
    if( *pccb->Cnfr & ~CI780_CNF_ADAP ) {
	if((( cnfr = *pccb->Cnfr ) & CI780_CNF_ERRS ) == 0 ) {
	    *pccb->Cnfr = cnfr;
	} else {
	    if( cnfr & CI780_CNF_PUP ) {
		pccb->Lpstatus.power = 1;
		lpcrash = SE_POWERUP;
	    } else if( cnfr & CI780_CNF_PDWN ) {
		pccb->Lpstatus.power = 0;
		lpcrash = SE_POWER;
	    } else if( cnfr & CI780_CNF_NOCI ) {
		pccb->Fsmstatus.broken = 1;
		lpcrash = FE_NOCI;
	    } else if ( cnfr & CI780_CNF_MSE ) {
		lpcrash = SE_MSE;
	    } else if( cnfr & CI780_CNF_CBPE ) {
		lpcrash = SE_PARITY;
	    } else {
		lpcrash = SE_PORTERROR;
	    }
	    ( void )ci_crash_lport( pccb, lpcrash, NULL );
	    ( void )splx( save_ipl );
	    return;
	}
    }
    if(( *pccb->Psr & ~PSR_RQA ) == 0 ) {
	*pccb->Psrcr = PSRCR_PSRC;
	( void )ci_rsp_handler( pccb );
    } else {
	if(( psr = *pccb->Psr ) & CI7B_PS_MTE ) {
	    lpcrash = SE_PARITY;
	} else if( psr & PSR_SE ) {
	    lpcrash = SE_SANITYTIMER;
	} else if( psr & PSR_DSE ) {
	    lpcrash = SE_DSE;
	} else if( psr & PSR_MFQE ) {
	    lpcrash = SE_MFQE;
	} else {
	    lpcrash = FE_PORTERROR;
	    if(( psr & PSR_MISC ) && Pesr_misc( *pccb->Pesr ) == PESR_BRKLINK){
		pccb->Fsmstatus.broken = 1;
	    }
	}
	( void )ci_crash_lport( pccb, lpcrash, NULL );
    }
    ( void )splx( save_ipl );
}

/*   Name:	cibci_isr	- CIBCI Interrupt Service Routine
 *
 *   Abstract:	This routine is the primary interrupt service routine for the
 *		CIBCI local port.  It services all interrupts except:
 *
 *		1. When a local port adapter has lost power.
 *		2. When a local port is marked broken and permanently shutdown.
 *
 *		At such times the local port is unmapped and all interrupts are
 *		serviced instead by ci_unmapped_isr().  
 *
 *		CIBCI adapters are located within their own physically separate
 *		cabinets.  They may lose power independently of the rest of the
 *		system.  They may also become uncabled from the BI on which
 *		they nominally reside leading to their local ports being marked
 *		broken and permanently shutdown.  When either of these events
 *		occurs, much( but not all ) of adapter I/O space becomes
 *		unaccessible and any access attempt results in a machine check.
 *		It is as a protection against these extraneous machine checks
 *		that CIBCI local ports are unmapped and interrupts re-directed
 *		to ci_unmapped_isr().
 *
 *   Inputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.type.hwtype	-  HPT_CIBCI
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *	    lpstatus.power	-   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *
 *   Outputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.psrcr	-   Port status release control reg pointer
 *	    lpstatus.power	-   Port has power status flag
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   Port is broken status flag
 *
 *   SMP:	The PCCB is locked to synchronize access both to itself and to
 *		adapter register contents but only during error processing.
 *		PCCB addresses are always valid because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		No locks are required for normal( ie - non-error ) interrupt
 *		processing for the following reasons:
 *
 *			1. No changes to interlocked structures are made.
 *			2. PCCB references are only to static fields.
 *			3. Port registers either indicate no errors at time of
 *		   	   access or can not change until released.
 */
void
cibci_isr( pccb )
    register PCCB	*pccb;
{
    register u_long	save_ipl, ber, cnfr, psr, lpcrash;

    /* The steps involved in servicing CIBCI interrupts are as follows:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The BI control and status, port configuration, and port status
     *	  registers are checked for errors.
     * 3. Responses on the port response queue are processed after releasing
     *	  the port status register.
     * 4. IPL is restored.
     *
     * CIBCI port configuration register errors include:
     *
     * 1. Port power up.
     * 2. Port power down.
     * 3. CI adapter not present( NOCI and DCLO  ).
     * 4. Parity errors( CPPE, BAPE, and BIPE ).
     * 5. Miscellaneous errors( BBE; or undefined errors ).
     *
     * CIBCI hard BI errors reported within the BI error register include:
     *
     * 1. BI memory system errors( IVE, RDS, RTO, STO, BTO, NEX, and ICE )
     * 2. Parity errors( MPE, CEP, and SPE ).
     * 3. Miscellaneous errors( MTCE, CTE, ISE, TDF; or undefined errors ).
     *
     * CIBCI port status register errors include:
     *
     * 1. Parity errors( reported within PMCSR ).
     * 2. Sanity time expiration.
     * 3. Data structure error.
     * 4. Message free queue empty.
     * 5. Miscellaneous errors( MISC; or undefined errors ).
     *
     * BI soft errors including CRDs are totally ignored.  Processing of the
     * interrupt proceeds as if they had never occurred.  The existence of any
     * other error forces bypassing of response processing and triggers
     * crashing, clean up, and re-initialization of the local port.  The PCCB
     * is locked while any and all errors are being processed.
     *
     * Port power down and CI adapter absent are special cases.  Both trigger
     * crashing, disablement, and clean up of the affected local port.  Both
     * result in scheduling of local port initialization.  However, while local
     * port initialization is postponed while port power is unavailable, no
     * attempt is made to initialize absent local ports.  They are just
     * shutdown and left permanently offline.  A more detailed description of
     * possible power failure recovery scenarios may be obtained by consulting
     * ci_crash_lport().
     *
     * There is one special port status register miscellaneous error which
     * requires further explanation.  Broken link modules( L0100 ) may result
     * in link module acceptance of all packets transmitted on the CI.
     * Needless to say this can cause havoc especially when the module begins
     * to ACK/NACK packets not really addressed to it causing ACK/NACK packet
     * collisions and a great deal of confusion.  CI port microcode has been
     * upgraded( hopefully ) to compare packet destination with local node
     * addresses and to signal mismatches by means of a special code in the
     * PESR.  Action taken by the CI port driver on discovery of this condition
     * includes crashing the port, marking it broken, and taking it permanently
     * offline.
     *
     * NOTE: Consult the beginning of this module for a more in-depth
     *	     discussion on the processing of interrupts, especially interrupts
     *	     to report errors, in SMP environments.
     *
     * NOTE: Port power up unlike port power down is not a special case.  It is
     *	     treated the same as any other fatal port error, triggering
     *	     crashing, disablement, clean up, and initialization of the
     *	     affected local port.
     *
     * NOTE: The local port may be found to be unmapped following locking of
     *	     the PCCB on initial discovery of an error.  The only way in which
     *	     such unmapping could have occurred is during interrupt processing
     *	     by another thread.  This other thread must have processed either
     *	     the adapter reported error which triggered creation of the current
     *	     thread or another more serious error.  This allows the current
     *	     thread to just be terminated as either the reason for its creation
     *	     has already been satisfactorily dealt with or no longer exists.
     */
    save_ipl = Splscs();
    if(( *pccb->Bictrl & BICTRL_HES )     ||
	 ( *pccb->Cnfr & CIBCI_CNF_ERRS ) ||
	 ( *pccb->Psr & CI7B_PS_ERRS )) {
	Lock_pccb( pccb )
	if( !pccb->Lpstatus.mapped ) {
	    Unlock_pccb( pccb )
	    ( void )splx( save_ipl );
	    return;
	}
	if(( cnfr = *pccb->Cnfr ) & CIBCI_CNF_ERRS ) {
	    if( cnfr & CIBCI_CNF_PUP ) {
		pccb->Lpstatus.power = 1;
		lpcrash = SE_POWERUP;
	    } else if( cnfr & CIBCI_CNF_PDWN ) {
		pccb->Lpstatus.power = 0;
		lpcrash = SE_POWER;
	    } else if( cnfr & ( CIBCI_CNF_NOCI | CIBCI_CNF_DCLO )) {
		pccb->Fsmstatus.broken = 1;
		lpcrash = FE_NOCI;
	    } else if( cnfr & CIBCI_CNF_PE ) {
		lpcrash = SE_BIPARITY;
	    } else {
		lpcrash = SE_BIERROR;
	    }
	} else if( *pccb->Bictrl & BICTRL_HES ) {
	    if(( ber = *pccb->Bierr ) & CIBCI_MSE_ERRS ) {
		lpcrash = SE_BIMSE;
	    } else if( ber & CIBCI_PAR_ERRS ) {
		lpcrash = SE_BIPARITY;
	    } else {
		lpcrash = SE_BIERROR;
	    }
	} else if(( psr = *pccb->Psr ) & CI7B_PS_ERRS ) {
	    if( psr & CI7B_PS_MTE ) {
		lpcrash = SE_PARITY;
	    } else if( psr & PSR_SE ) {
		lpcrash = SE_SANITYTIMER;
	    } else if( psr & PSR_DSE ) {
		lpcrash = SE_DSE;
	    } else if( psr & PSR_MFQE ) {
		lpcrash = SE_MFQE;
	    } else {
		lpcrash = FE_PORTERROR;
		if(( psr & PSR_MISC ) &&
		     Pesr_misc( *pccb->Pesr ) == PESR_BRKLINK ) {
		    pccb->Fsmstatus.broken = 1;
		}
	    }
	} else {
	    Unlock_pccb( pccb )
	    lpcrash = 0;
	}
	if( lpcrash ) {
	    ( void )ci_crash_lport( pccb, lpcrash, NULL );
	    Unlock_pccb( pccb )
	    ( void )splx( save_ipl );
	    return;
	}
    }
    if( *pccb->Psr & PSR_RQA ) {
	*pccb->Psrcr = PSRCR_PSRC;
	( void )ci_rsp_handler( pccb );
    }
    ( void )splx( save_ipl );
}

/*   Name:	cibca_isr	- CIBCA-AA/CIBCA-BA Interrupt Service Routine
 *
 *   Abstract:	This routine is the primary interrupt service routine for all
 *		CIBCA local ports, both CIBCA-AA and CIBCA-BA.  It services all
 *		interrupts except when a local port is marked broken and
 *		permanently shutdown.  At such times the local port is unmapped
 *		and all interrupts are serviced instead by ci_unmapped_isr().
 *
 *   Inputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.type.hwtype	-  HPT_CIBCA_AA or HPT_CIBCA_BA
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *
 *   Outputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.psrcr	-   Port status release control reg pointer
 *
 *   IPL_CI			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked to synchronize access to adapter register
 *		contents but only during error processing.  PCCB addresses are
 *		always valid because these data structures are never deleted
 *		once their corresponding ports have been initialized.
 *
 *		No locks are required for normal( ie - non-error ) interrupt
 *		processing for the following reasons:
 *
 *			1. No changes to interlocked structures are made.
 *			2. PCCB references are only to static fields.
 *			3. Port registers either indicate no errors at time of
 *		   	   access or can not change until released.
 */
void
cibca_isr( pccb )
    register PCCB	*pccb;
{
    register u_long	save_ipl, psr, lpcrash;

    /* The steps involved in servicing CIBCA interrupts are as follows:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The BI control and status and port status registers are checked for
     *	  errors.
     * 3. Responses on the port response queue are processed after releasing
     *    the port status register.
     * 4. IPL is restored.
     * 
     * CIBCA hard BI errors reported within the BI error register include:
     * status register errors include:
     *
     * 1. Memory system errors( MTCE, MPE, IVE, SPE, RDS, RTO, STO, BTO, NEX,
     *	  and ICE ).
     * 2. Miscellaneous errors( NMR; or undefined errors ).
     *
     * CIBCA port status register errors include:
     *
     * 1. Parity errors( IIPE, CPE, XMPE, IBPE, CSPE, and BCIP in PMCSR ).
     * 2. Memory system errors( MBIE in PMCSR ).
     * 3. Sanity time expiration.
     * 4. Data structure error.
     * 5. Message free queue empty.
     * 6. Miscellaneous errors( MISC; or undefined errors ).
     *
     * BI soft errors including CRDs are totally ignored.  Processing of the
     * interrupt proceeds as if they had never occurred.  The existence of any
     * other error forces bypassing of response processing and triggers
     * crashing, clean up, and re-initialization of the local port.  The PCCB
     * is locked while any and all errors are being processed.
     *
     * NOTE: Consult the beginning of this module for a more in-depth
     *	     discussion on the processing of interrupts, especially interrupts
     *	     to report errors, in SMP environments.
     */
    save_ipl = Splscs();
    if(( *pccb->Bictrl & BICTRL_HES ) || ( *pccb->Psr & CIBX_PS_ERRS )) {
	Lock_pccb( pccb )
	if( *pccb->Bictrl & BICTRL_HES ) {
	    if( *pccb->Bierr & CIBCA_MSE_ERRS ) {
		lpcrash = SE_MSE;
	    } else {
		lpcrash = SE_PORTERROR;
	    }
	} else if(( psr = *pccb->Psr ) & CIBX_PS_ERRS ) {
	    if( psr & CIBX_PS_MTE ) {
		lpcrash = SE_PARITY;
	    } else if( psr & PSR_SE ) {
		lpcrash = SE_SANITYTIMER;
	    } else if( psr & PSR_DSE ) {
		lpcrash = SE_DSE;
	    } else if( psr & PSR_MSE ) {
		lpcrash = SE_MSE;
	    } else if( psr & PSR_MFQE ) {
		lpcrash = SE_MFQE;
	    } else {
		lpcrash = SE_PORTERROR;
	    }
	} else {
	    Unlock_pccb( pccb )
	    lpcrash = 0;
	}
	if( lpcrash ) {
	    ( void )ci_crash_lport( pccb, lpcrash, NULL );
	    Unlock_pccb( pccb )
	    ( void )splx( save_ipl );
	    return;
	}
    }
    if( *pccb->Psr & PSR_RQA ) {
	*pccb->Psrcr = PSRCR_PSRC;
	WBFLUSH
	( void )ci_rsp_handler( pccb );
    }
    ( void )splx( save_ipl );
}

/*   Name:	cixcd_isr	- CIXCD Interrupt Service Routine
 *
 *   Abstract:	This routine is the primary interrupt service routine for the
 *		CIXCD local port.  It services all interrupts except when a
 *		local port is marked broken and permanently shutdown.  At such
 *		times the local port is unmapped and all interrupts are
 *		serviced instead by ci_unmapped_isr().
 *
 *   Inputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.type.hwtype	-  HPT_CIXCD
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *
 *   Outputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.psrcr	-   Port status release control reg pointer
 *	    ic.xmi.xbe		-   XMI bus error register pointer
 *
 *   SMP:	The PCCB is locked to synchronize access to adapter register
 *		contents but only during error processing.  PCCB addresses are
 *		always valid because these data structures are never deleted
 *		once their corresponding ports have been initialized.
 *
 *		No locks are required for normal( ie - non-error ) interrupt
 *		processing for the following reasons:
 *
 *			1. No changes to interlocked structures are made.
 *			2. PCCB references are only to static fields.
 *			3. Port registers either indicate no errors at time of
 *		   	   access or can not change until released.
 */
cixcd_isr( pccb )
    register PCCB	*pccb;
{
    register u_long	lpcrash, psr, xbe, save_ipl;

    /* The steps involved in servicing CIXCD interrupts are as follows:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The XMI bus error status register is checked for serious errors.
     * 3. Responses on the port response queue are processed after releasing
     *    the port status register.
     * 4. IPL is restored.
     *
     * CIXCD errors reported within the XMI bus error register include:
     *
     * 1. Soft errors( CRD and CC ).
     * 2. Parity errors( PE ).
     * 3. Node specific errors( NSES ).
     * 4. Miscellaneous errors( WSE, RIDNAK, WDNAK, NRR, RSE, RER, CNAK,
     *    TTO; or undefined errors ).
     *
     * The existence of a node specific error requires querying the port status
     * register in order to determine its exact identity.  CIXCD port status
     * register errors include:
     *
     * 1. Memory system errors.
     * 2. Parity errors( reported within PMCSR ).
     * 3. Sanity time expiration.
     * 4. Data structure error.
     * 5. Message free queue empty.
     * 6. Miscellaneous errors( MISC; or undefined errors ).
     *
     * XMI soft errors are cleared and then ignored.  Processing of the
     * interrupt proceeds as if they had never occurred.  The existence of any
     * other error forces bypassing of response processing and triggers
     * crashing, clean up, and re-initialization of the local port.  The PCCB
     * is locked while any and all errors are being processed.
     *
     * NOTE: Consult the beginning of this module for a more in-depth
     *	     discussion on the processing of interrupts, especially interrupts
     *	     to report errors, in SMP environments.
     */
    save_ipl = Splscs();
    Lock_cidevice( pccb )
    xbe = *pccb->Xbe;
    psr = *pccb->Psr;
    Unlock_cidevice( pccb )
    if(( xbe & CIXCD_XBE_ERRS ) || ( psr & CIXCD_PS_NRSPE )) {
	Lock_pccb( pccb )
	if(( xbe & CIXCD_XBE_HARDE ) > 0 ) {
	    if(( xbe & XMI_PE ) || ( xbe & XMI_NSES )) {
	        lpcrash = SE_PARITY;
            } else {
	        lpcrash = SE_PORTERROR;
            }
	} else if( psr & CIXCD_PS_NRSPE ) {
	    if( psr & PSR_SE ) { 
		lpcrash = SE_SANITYTIMER;
	    } else if( psr & PSR_DSE ) {
		lpcrash = SE_DSE;
	    } else if( psr & PSR_MSE ) {
		lpcrash = SE_MSE;
	    } else if( psr & PSR_MFQE ) {
		lpcrash = SE_MFQE;
	    } else if( psr & PSR_MISC ) {
		lpcrash = SE_PORTERROR;
	    } else if( psr & CIBX_PS_UNIN ) {
		lpcrash = SE_PORTERROR;
	    } else {
	        Unlock_pccb( pccb )
		lpcrash = 0;
	    }
	} else  {
        	Lock_cidevice( pccb )
		*pccb->Xbe = xbe;
        	Unlock_cidevice( pccb )
		DELAY( 1000 );
	        Unlock_pccb( pccb )
	        lpcrash = 0;
	}
	if( lpcrash ) {
	    ( void )ci_crash_lport( pccb, lpcrash, NULL );
	    Unlock_pccb( pccb )
	    ( void )splx( save_ipl );
	    return;
	}
    }
    if( psr & PSR_RQA ) {
        Lock_cidevice( pccb )
	*pccb->Psrcr = PSRCR_PSRC;
        Unlock_cidevice( pccb )
	( void )ci_rsp_handler( pccb );
    } else if( psr & CIXCD_PS_NRSPE ) {
        Lock_cidevice( pccb )
	*pccb->Psrcr = PSRCR_PSRC;
        Unlock_cidevice( pccb )
    }
    ( void )splx( save_ipl );
}

/*   Name:	cikmf_isr	- CIKMF Interrupt Service Routine
 *
 *   Abstract:	This routine is the primary interrupt service routine for the
 *		CIKMF local port.  It services all interrupts except when a
 *		local port is marked broken and permanently shutdown.  At such
 *		times the local port is unmapped and all interrupts are
 *		serviced instead by ci_unmapped_isr().
 *
 *   Inputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.type.hwtype	-  HPT_CIKMF
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *
 *   Outputs:
 *
 *   IPL_CI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.psrcr	-   Port status release control reg pointer
 *	    ic.xmi.xbe		-   XMI bus error register pointer
 *
 *   SMP:	The PCCB is locked to synchronize access to adapter register
 *		contents but only during error processing.  PCCB addresses are
 *		always valid because these data structures are never deleted
 *		once their corresponding ports have been initialized.
 *
 *		No locks are required for normal( ie - non-error ) interrupt
 *		processing for the following reasons:
 *
 *			1. No changes to interlocked structures are made.
 *			2. PCCB references are only to static fields.
 *			3. Port registers either indicate no errors at time of
 *		   	   access or can not change until released.
 */
cikmf_isr( pccb )
    register PCCB	*pccb;
{
    register u_long	lpcrash, psr, save_ipl, xbe, apser;

    /* The steps involved in servicing CIKMF interrupts are as follows:
     *
     * 1. IPL is synchronized to IPL_SCS.
     * 2. The XMI bus error status register is checked for serious errors.
     * 3. Responses on the port response queue are processed after releasing
     *    the port status register.
     * 4. IPL is restored.
     *
     * CIKMF errors reported within the XMI bus error register include:
     *
     * 1. Soft errors( CRD and CC ).
     * 2. Parity errors( PE ).
     * 3. Node specific errors( NSES ).
     * 4. Miscellaneous errors( WSE, RIDNAK, WDNAK, NRR, RSE, RER, CNAK,
     *    TTO; or undefined errors ).
     *
     * The existence of a node specific error requires querying the port status
     * register in order to determine its exact identity.  CIKMF port status
     * register errors include:
     *
     * 1. Memory system errors.
     * 2. Parity errors( reported within PMCSR ).
     * 3. Sanity time expiration.
     * 4. Data structure error.
     * 5. Message free queue empty.
     * 6. Miscellaneous errors( MISC; or undefined errors ).
     *
     * XMI soft errors are cleared and then ignored.  Processing of the
     * interrupt proceeds as if they had never occurred.  The existence of any
     * other error forces bypassing of response processing and triggers
     * crashing, clean up, and re-initialization of the local port.  The PCCB
     * is locked while any and all errors are being processed.
     *
     * NOTE: Consult the beginning of this module for a more in-depth
     *	     discussion on the processing of interrupts, especially interrupts
     *	     to report errors, in SMP environments.
     */
    save_ipl = Splscs();
    if(( *pccb->Xpcpser & CIKMF_PSER_ERRS ) || ( *pccb->Psr & CIKMF_PS_MTE )) {
	Lock_pccb( pccb )
	if( pccb->Lpstatus.adapt && (( xbe = *pccb->Xbe ) & CIKMF_XBE_ERRS )) {
	    if( xbe & CIKMF_XBE_PERRS ) {
		pccb->Ciadap->status.reset = 1;
	        lpcrash = SE_PARITY;
            } else if( xbe & CIKMF_XBE_HERRS ) {
		pccb->Ciadap->status.reset = 1;
	        lpcrash = SE_PORTERROR;
            } else {		/* Corrected Confirmation */
		*pccb->Xbe = xbe;
	        Unlock_pccb( pccb )
	        lpcrash = 0;
	    }
	} else if(( apser = *pccb->Xpcpser ) & CIKMF_PSER_HERRS ) {
	        lpcrash = SE_PORTERROR;
	} else if(( psr = *pccb->Psr ) & CIKMF_PS_MTE ) {
	    if( psr & PSR_SE ) { 
		lpcrash = SE_SANITYTIMER;
	    } else if( psr & PSR_DSE ) {
		lpcrash = SE_DSE;
	    } else if( psr & PSR_MSE ) {
		lpcrash = SE_MSE;
	    } else if( psr & PSR_MFQE ) {
		lpcrash = SE_MFQE;
	    } else if( psr & PSR_MISC ) {
		lpcrash = SE_PORTERROR;
	    } else if( psr & CIBX_PS_UNIN ) {
		lpcrash = SE_PORTERROR;
	    } else {
	        Unlock_pccb( pccb )
		lpcrash = 0;
	    }
	} else  {		/* CRD error  */
	    *pccb->Xpcpser = apser;
	    Unlock_pccb( pccb )
	    lpcrash = 0;
	}
	if( lpcrash ) {
	    ( void )ci_crash_lport( pccb, lpcrash, NULL );
	    Unlock_pccb( pccb );
	    ( void )splx( save_ipl );
	    return;
	}
    }
    if( *pccb->Psr & PSR_RQA ) {
	*pccb->Psrcr = PSRCR_PSRC;
	( void )ci_rsp_handler( pccb );
    }
    ( void )splx( save_ipl );
}
