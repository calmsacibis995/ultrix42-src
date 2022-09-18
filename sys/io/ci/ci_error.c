#ifndef	lint
static char *sccsid = "@(#)ci_error.c	4.3	(ULTRIX)	10/16/90";
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
 *		error processing routines and functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	November 12, 1985
 *
 *   Function/Routines:
 *
 *   ci_cleanup_port		Clean up Local CI Port
 *   ci_console_log		Log CI Events to Console Terminal
 *   ci_log_badport		Log Bad Port Numbers in CI Packets
 *   ci_log_dev_attn		Log CI Device Attention Events
 *   ci_log_initerr		Log CI Port Initialization Fatal Errors
 *   ci_log_packet		Log CI Packet Related Events
 *   ci_map_port		Map Local CI Port
 *   ci_unmap_port		Unmap Local CI Port
 *   ci7b_disable		Disable a Local CI7B Family Port
 *   cibx_disable		Disable a Local CIBX Family Port
 *
 *   Modification History:
 *
 *   16-Oct-1990	Pete Keilty
 *	1. Changed smp_lock cidevice locking to use new macros define in 
 *	   ciscs.h. Also added DELAY( 1000 ) before and after register
 *	   writes on initialization because of CIXCD XMOV bug for back to
 *	   register writes, needed on 9000/6000 systems.
 *	2. Changed failure reason in ci_cleanup_port from PF_ERROR to
 *	   PF_PORTERROR SCS would give back resources on none port errors.
 *
 *   16-Jul-1990	Pete Keilty
 * 	Add smp_lock lk_cidevice in the cibx_disable routine for CIXCD,
 *	software workaround for XMOV hardware bug of back to back register
 *	accesses the first one a read the second a write to a software
 *	register.
 *
 *   15-Jun-1990	Pete Keilty
 *	Added check for explicit address packet in ci_log_packet() and
 *	set new ci packet flag CI_EXPADRS for uerf decode.
 *
 *   06-Jun-1990	Pete Keilty
 * 	1. Added printout of pfar register to console logging.
 *	2. Added preliminary support for CIKMF ( dash )
 *
 *   06-Feb-1990	Pete Keilty
 *	1. Added a check in the cibx_disable() if the adapter is all
 *	   ready in the uninitialized state don't set port disable.
 *	   This is because CIXCD pdr register is not availible in the
 *	   uninitialized state.
 *	2. Pass in the starting address of the node space to xmisst().
 *
 *   08-Dec-1989	Pete Keilty
 *	1. Use new macro Get_pgrp() to get the port node number. This
 * 	   will be used until full subnode addressing is done.
 *	2. The pidr & pvr are not set in cibx_disable() the port
 *	   has to be in the uninitialized state to set. Pidr & pvr
 *	   are set in cibx_start().
 *
 *   09-Nov-1989	David E. Eiche		DEE0080
 *	Rename all references to interconnects from IC_xxx to
 *	ICT_xxx to match the new definitions in sysap.h.
 *
 *   19-Sep-1989	Pete Keilty
 *	Added pccb to macro Format_gvph, Ppd_to_pd & Pd_to_ppd.
 *	Added XCD support, removed XCB.
 *
 *   24-May-89		darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's 
 *	Add include file ../io/scs/scamachmac.h this file has machine
 *	depend items.
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   17-Jan-1989	Todd M. Katz		TMK0005
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Some hardware platforms require field alignments and access types
 *	   to match( ie- only longword aligned entities may be longword
 *	   accessed ).  Structure fields of type c_scaaddr present a potential
 *	   problem because they are 6 bytes long but are often treated as
 *	   contigious longword and word.  Such fields have been longword
 *	   aligned whenever possible.  However, it is not feasible to align
 *	   certain fields with this format within the CI error log event packet
 *	   for compatibility reasons.  Therefore, what has been changed for
 *	   these fields is how they are accessed.  They are now accessed as
 *	   three contigious words instead of as contigious longword and word.
 *	3. Modify ci_log_initerr() to obtain system identification number and
 *	   node name from the appropriate configuration variables instead of
 *	   from local system permanent information.  The latter might NOT have
 *	   been initialized at the time this routine is invoked to log a fatal
 *	   local CI port initialization error.
 *	4. Modify ci_log_dev_attn(), ci__log_initerr(), and ci_log_packet() to
 *	   always initialize the CI error log packet field ci_optfmask2 to 0.
 *	5. Include header file ../vaxmsi/msisysap.h.
 *	6. Use the ../machine link to refer to machine specific header files.
 *
 *   21-Aug-1988	Todd M. Katz		TMK0004
 *   	 1. The interface to cippd_stop() has been changed so that the routine
 *	    no longer has to map specific local port crash reasons to generic
 *	    path failure reasons.  This mapping must now be accomplished within
 *	    the individual port drivers.  Modify ci_cleanup_port()
 *	    appropriately.
 *	 2. Refer to error logging as event logging.
 *	 3. SCA event codes have been completely revised.
 *	 4. The following informational events( ES_I ) have been redefined as
 *	    warning events( ES_W ): UCODE_WARN.
 *	 5. The following informational events( ES_I ) have been redefined as
 *	    error events( ES_E ): UCODE_LOAD.
 *	 6. The following informational events( ES_I ) have been redefined as
 *	    fatal error events( ES_FE ): INIT_NOMEM, INIT_ZEROID, INIT_NOCODE,
 *	    INIT_UNKHPT, INIT_MISMTCH, BADUCODE.
 *	 7. Rename the informational event( ES_IC ) mnemonic from IE -> I.
 *	 8. Modifications to ci_console_log():
 *		1) All event code verification checks have been redone.
 *		2) Logging of certain specific events now occurs regardless of
 *		   the current CI severity level.
 *		3) Define a new arguement to the routine, event_type( type of
 *		   event being console logged ).  The optional arguement
 *		   cippdbp( address of CI PPD header ) has been changed to
 *		   cibp( address of CI port header ).
 *		4) The general format of all messages has been redesigned.
 *		   Path specific information is always displayed by default if
 *		   the event being logged is a path specific event(
 *		   PATH_EVENT ).  The local port station address is always
 *		   displayed by default if the event being logged is a local
 *		   port specific event( LPORT_EVENT ) and of severity level
 *		   greater than or equal to error event( ES_E ).
 *		5) CPU revision level( CF_CPU ), local port station address(
 *		   CF_LPORT ), and initial local port initialization
 *		   information( CF_INIT ) have been added as classes of
 *		   variable information displayed.
 *		6) Modify the microcode revision level class of variable
 *		   information( CF_UCODE ) to display both current and
 *		   supported microcode revision levels.  Also, display the
 *		   local port station address when the event is of severity
 *		   level less than error event( ES_E ).
 *	 9. Modifications to ci_log_dev_attn():
 *		1) Increment the number of errors associated with the
 *		   appropriate local port only when the severity of the current
 *		   event is error( ES_E ) or higher.
 *		2) Initialize the new CI event logging packet field
 *		   ci_evpktver.
 *		3) Add CPU microcode revision levels as a class of optional
 *		   information logged.
 *		4) Add logging of CI microcode revision levels for the
 *		   following informational events: LPORT_INIT, LPORT_REINIT.
 *		5) Initialize event log record fields correspond to unused or
 *		   inaccessible registers to EL_DEF instead of zeroing them.
 *		6) The device attention specific portion of a CI event log
 *		   record was not being correctly initialized for the majority
 *		   of instances when logging of optional information was
 *		   required by the specific event code being logged.  This
 *		   problem was due to incorrect assumptions about the layout of
 *		   the device attention specific portion of the record.  One
 *		   additional consequence of these incorrect assumptions was
 *		   the potential for writing into event log buffer space beyond
 *		   the current record being initialized.  This problem has been
 *		   fully resolved.
 *	10. Modifications to ci_log_initerr():
 *		1) Initialize the new CI event logging packet field
 *		   ci_evpktver.
 *		2) Change the format of the console messages printed by this
 *		   routine.
 *	11. Modifications to ci_log_packet():
 *		1) Define a new arguement to the routine, event_type( type of
 *		   event being console logged ).  The optional arguement
 *		   cippdbp( address of CI PPD header ) has been changed to
 *		   cibp( address of CI port header ).
 *		2) Increment the number of errors associated with the
 *		   appropriate local port only when the severity of the current
 *		   event is error( ES_E ) or higher.
 *		3) Initialize the new CI event logging packet field
 *		   ci_evpktver.
 *	12. Add the new routine ci_log_badport(), Log Bad Port Numbers in CI
 *	    Packets, for use by the CI PPD.
 *	13. Modify ci7b_disable() and cibx_disable() to log all decisions to
 *	    mark local ports broken and force their permanent shutdown with the
 *	    exception of those involving local ports undergoing their initial
 *	    local port initialization.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *	 1. Create a single unified hierarchical set of naming conventions for
 *	    use within the CI port driver and describe them within ciport.h.
 *	    Apply these conventions to all names( routine, macro, constant, and
 *	    data structure ) used within the driver.  Restructure the driver to
 *	    segregate most CI family and port type specific code into separate
 *	    routines.  Such restructuring requires:
 *		1) Modifying ci_console_log to utilize the new PCCB fields
 *		   max_fn_level and max_rom_level when displaying current
 *		   microcode revision levels.
 *		2) Splitting ci_disable_port() into ci7b_disable(), a routine
 *		   responsible for disabling CI7B family ports; and, into
 *		   cibx_disable(), a routine responsible for disabling CIBX
 *		   family ports.
 *	 2. Add support for the CIXCB hardware port type by:
 *		1) Modifying ci_console_log() and ci_log_dev_attn() to log the
 *		   appropriate registers for this hardware port type.
 *		2) Modifying cibx_disable() to disable XMI based CI ports.
 *		3) Adding include file ../vaxxmi/xmireg.h.
 *	 3. Modify the circumstances under which local ports are unmapped.
 *	    Unmapping is now done only when the local port adapter itself loses
 *	    power( CI750/CI780/CIBCI only ) or the port is marked broken and is
 *	    permanently shutdown.  Formerly, unmapping was done whenever a
 *	    local port was crashed, but before its re-initialization commenced;
 *	    and just immediately prior to its initial initialization.  This
 *	    change requires:
 *		1) Appropriate modifications to the comments of routines
 *		   ci_map_port(), ci_unmap_port(), ci7b_disable(), and
 *		   cibx_disable().
 *		2) Modifying ci7b_disable() and cibx_disable() to unmap local
 *		   ports only when appropriate conditions are met( loss of
 *		   power or marked broken ).
 *		3) Moving PCCB hardware port type validation from ci_map_port()
 *		   -> ci_test_port() so that such validations are performed as
 *		   part of every initialization of every local port.
 *	 4. Add use of macros Ci7b_wait_mif() and Cibx_wait_mif().
 *	 5. Add hardware port type and remove adapter I/O space virtual address
 *	    as arguments to ci_log_initerr() and modify the routine
 *	    appropriately.
 *	 6. Remove IE_INIT_CPU as an event logged by ci_log_initerr().
 *	 7. Eliminate all references to the BIIC error interrupt control
 *	    register including:
 *	 	1) All logging of the register within ci_log_dev_attn().
 *	 	2) All clearing of the register within ci7b_disable() and
 *	 	   cibx_disable() which should never have been done anyway.
 *	    The CI port driver never initializes this register because it never
 *	    requests to be notified of soft errors; therefore, there is no
 *	    longer a need to reference it.
 *	 8. Rename CF_PORT -> CF_RPORT.
 *	 9. Minimize the use of BADADDRs since they are currently expensive and
 *	    can not completely protect against machine checks.  This requires
 *	    modifications to the routines ci7b_disable(), cibx_disable() and
 *	    ci_log_dev_attn().  Add an explanation as to why and when register
 *	    accesses require protection.
 *	10. Modify ci_cleanup_port() to no longer zero log maps( rsplogmap )
 *	    during CI specific PCCB clean up.
 *	11. Update the format in which ci_console_log() displays path crash
 *	    error events.
 *	12. Modify ci_log_dev_attn(), ci_log_initerr(), and ci_log_packet() to
 *	    appropriately initialize the new CI event log packet field
 *	    ci_optfmask according to the information being logged.  This field
 *	    is a bit mask with a unique bit assigned for each field optionally
 *	    present within a CI event log packet.  Also change the routine
 *	    ci_log_dev_attn() so that it no longer always allocates full size
 *	    device attention event log packets.  Packet sizes are now always
 *	    tailored to the exact information to be saved for each event
 *	    logged.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   09-Apr-1988	Todd M. Katz		TMK0002
 *	1. Add support for the CIBCA-BA hardware port type.  Differentiate
 *	   CIBCA-BA from CIBCA-AA hardware ports when necessary; otherwise,
 *	   refer to both types as just CIBCA ports.
 *	2. Fix a problem with ci_cleanup_port().  Currently, this routine
 *	   expects two parameters although it may only have one because is it
 *	   only invoked through forking.  Delete the second parameter( reason
 *	   - the reason for port failure ) and obtain its value from the PCCB
 *	   where it was stored before routine execution was scheduled.
 *	3. Add use of Elcicommon(), Ctrl_from_num(), and Pccb_fork_done()
 *	   macros.
 *	4. The fields ci_lsaddr, ci_lsysid, and ci_lname have been moved from
 *	   structure ci_lcommon to structure elci_common in the definition of
 *	   a CI event packet.  Modify ci_log_dev_attn(), ci_log_initerr(), and
 *	   ci_log_packet() to initialize these fields for every event logged.
 *
 *   16-Mar-1988	Jaw
 *	Bug fix in ci_disable_port routine.  The badaddr probe of the 
 *	bierr register was using the register content as the address.  This
 *	caused BI machines with CI's to panic with a segmentation fault.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, restructured code paths, and added SMP support.
 */

/*	      Protecting Register Accesses Against Machine Checks
 *
 * Machine checks are relatively rare events which may occur for many reasons.
 * The vast majority have absolutely no CI involvement; however, a few result
 * from CI port driver attempts to access inaccessible addresses in CI adapter
 * I/O space.  It is these machine checks the CI port driver is specifically
 * concerned about and wants to protect against.
 * 
 * The only CI adapter I/O space addresses the CI port driver directly accesses
 * are those corresponding to adapter local port registers.  These registers
 * are accessed by the driver during the following events:
 *
 * 1. Local CI port initialization.
 * 2. Local CI port notification of command and free datagram/message buffer
 *    availability.
 * 3. Processing of local CI port interrupts.
 * 4. Local CI port notification of the continued functioning of host software.
 * 5. Logging of certain local CI port errors.
 * 6. Error recovery following failure of a local CI port.
 *
 * Events 2-4 take place quite frequently.  The remaining events occur
 * extremely rarely.  Any scheme for port driver protection against extraneous
 * machine checks must take the frequencies of these events into account.
 *
 * To summarize, the only machine checks the CI port driver needs to guard
 * against are those that result from driver attempts to access inaccessible
 * local port registers.  Normally, all registers are of course fully
 * accessible.  Machine checks would be extremely common if this was not the
 * case.  However, a number of well-defined circumstances do exist under which
 * registers become inaccessible.  The most likely of these is loss of power,
 * either system-wide or just isolated to the bus on which the CI adapter
 * resides.  Certain CI hardware port types( CI750, CIBCI ) are also subject to
 * completely independent losses of power.  The adapters for these port types
 * are located within their own physically separate cabinets.  This makes them
 * vulnerable not only to separate losses of power but to becoming uncabled
 * from the busses on which they reside.  Either unfortunate circumstances
 * results in inaccessibility of most, but not all, local port registers.  The
 * registers of a sufficiently broken local port may also become inaccessible.
 *
 * The best most idealistic machine check protection strategy for the CI port
 * driver would be for it to always provide transparent protection against
 * machine checks during all register accesses.  Unfortunately, this is not
 * possible because the only protective mechanisms currently available are
 * expensive and are not transparent.
 *
 * The next best strategy is a defensive one: the CI port driver only protects
 * against machine checks on local CI ports known to have lost power, become
 * physically uncabled, or to have suffered a sufficient hardware failure.  In
 * other words, protective mechanisms are employed only for those local ports
 * determined to be at risk.  At all other times the driver makes no attempt to
 * protect against machine checks during register accesses.
 *
 * Implementation of this strategy is also unfortunately not possible.  This is
 * because for it to succeed the CI port driver would have to meet the
 * following two requirements:
 *
 * 1. It would have to be immediately notified when any of the events which
 *    could result in inaccessible local CI port registers occurs.
 * 2. It would have to immediately protect all subsequent register accesses on
 *    notification of local port machine check vulnerability.
 *
 * The first requirement can not be met because the driver is never notified of
 * system-wide or bus associated power loss.  Ironically ULTRIX crashes with a
 * machine check when such power loss occurs!  Furthermore, while the driver is
 * notified of independent CI port power loss, uncabling, and fatal errors
 * through interrupts; handling of such interrupts maybe temporarily blocked
 * postponing notification.  Such blockage is due to processor IPL level in
 * single processor environments and current unavailability of critical locks
 * in SMP environments.  The end result is the same, inability to immediately
 * notify the driver of local port machine check vulnerability on register
 * accesses.
 *
 * The second requirement for implementation of the next best strategy can also
 * not be met.  To meet it requires CI port driver determination of whether a
 * local CI port is currently vulnerable to machine checks prior to each
 * register access and to employ protective mechanisms only if it is.  Meeting
 * this requirement is much too costly given the frequency with which certain
 * register accesses are made.
 *
 * The machine check protection strategy actually employed by the CI port
 * driver is a realistic one and possesses the following two basic
 * characteristics:
 *
 * 1. It never attempts to protect against machine checks during register
 *    accesses that take place frequently( Events 2-4 above ).
 * 2. It considers a local port to always be at risk and protects against
 *    machine checks during all infrequent register accesses( Events 1,5-6
 *    above ).
 * 3. The first checks it makes during interrupt processing are for uncabled
 *    or powerless local ports before allowing any subsequent register accesses
 *    by the current thread to occur.
 *
 * At first glance it may seem that this strategy is not all that rigorous,
 * and that better ones could be designed, and to some extent this is true.
 * However, this strategy is actually the best one available given the tools
 * which are currently provided by the Ultrix kernel.
 *
 * The basic tool employed for machine check protection is the BADADDR() macro.
 * This macro determines whether a specified byte, word, or longword address is
 * addressable and "returns" 1 if it is not.  It is used in the machine check
 * protection strategy to determine whether a register is accessible before
 * attempting to access it.  Therefore, the actual register accesses are never
 * themselves actually protected.  
 *
 * This mechanism is as defensive as it is possible to be without being
 * paranoid and without seriously affecting performance of mainline code paths.
 * It will serve until it is possible to transparently protect against machine
 * during all register accesses.
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
#ifdef vax
#include		"../machine/mtpr.h"
#endif vax
#include		"../../machine/common/cpuconf.h"
#include		"../machine/cpu.h"
#include		"../machine/pte.h"
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
extern	SCSIB		lscs;
extern	struct el_rec	*ealloc();
extern	u_short		ci_severity;
extern	u_short		ci_errlog;
extern	u_long		ci_bhole_pfn;
extern  scaaddr         scs_system_id;
extern	CLSTAB		ci_cltab[ ES_FE + 1 ][ ESC_PPD + 1 ];
extern	u_char		*ci_black_hole, scs_node_name[];
extern	void		ci_dealloc_pkt(), ci_log_packet(), ci_unmap_port(),
			ci_unmapped_isr(), cippd_stop();

/*   Name:	ci_cleanup_port	- Clean up Local CI Port
 *
 *   Abstract:	This routine directs the second stage of local CI port clean
 *		up.  It is always invoked by forking to it.
 *
 *		Failed local CI ports are cleaned up in two stages.  The first
 *		stage consists of those actions which should be performed
 *		immediately following port disablement and are insensitive to
 *		processor state.  The second stage consists of those activities
 *		which need not be performed immediately following port
 *		disablement and should be executed within a constant
 *		well-defined processor state.  This routine direct this second
 *		stage of port clean up and the constant environment necessary
 *		for its proper execution is provided by always forking to it.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.reason		-  Reason for port failure
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.cleanup	-   1
 *	    fsmstatus.fkip	-   1
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.fkip	-   0
 *
 *   SMP:	The PCCB is locked to synchronize access.  PCCB locking is
 *		probably unnecessary because of lack of conflict for the PCCB
 *		due to single threading of port clean up and re-initialization.
 *		It is done anyway to guarantee unrestricted access and because
 *		the CI PPD interval timer may still be active.  PCCB addresses
 *		are always valid because these data structures are never
 *		deleted once their corresponding ports have been initialized.
 */
void
ci_cleanup_port( pccb )
    register PCCB	*pccb;
{
    register GVPH	**lp, **ep, *cibp;
    register u_long	code, pf_reason;
    register u_long	save_ipl = Splscs();

    /* The steps involved in the second stage of local port clean up include:
     *
     * 1. Locking the PCCB.
     * 2. Removing and deallocating all packets logged to the local port
     *    datagram and message buffer logout areas at time of failure.
     * 3. Map the specific local port crash reason into a generic one.
     * 4. Unlocking the PCCB.
     * 5. Notifying the CI PPD of failure of the local port.
     *
     * CI ports logout all internalized queue entries to the appropriate area
     * only when power failure is detected.  Both logout areas are scanned in
     * their entirety and any packets found are deallocated( Step 3 ).  The
     * logout areas are also re-initialized to GVPH_FREE( -1 ) to be able to
     * differentiate logged packets from empty logout area slots on subsequent
     * power failures.
     *
     * The CI PPD completes the clean up of its portion of the local port
     * including the failure and clean up all paths, both formative and fully
     * established, originating at the port( Step 5 ).  Clean up of the last
     * path triggers scheduling of port re-initialization by the CI PPD.  The
     * PCCB lock is released( Step 4 ) prior to notifying the CI PPD of local
     * port failure instead of after it as required by the SCA architecture.
     */
    Lock_pccb( pccb )
    Pccb_fork_done( pccb, PANIC_PCCBFB )
    for( lp = ( GVPH ** )pccb->Pqb.Dqe_logout,
	 ep = ( GVPH ** )pccb->Pqb.Dqe_logout + ( 2 * CI_NLOG );
	 lp < ep;
	 ++lp ) {
	if(( cibp = *lp ) != ( GVPH * )GVPH_FREE ) {
	    ( void )ci_dealloc_pkt( cibp );
	    *lp = ( GVPH * )GVPH_FREE;
	}
    }
    if( Eseverity( pccb->lpinfo.reason ) == ES_FE ) {
	pf_reason = PF_FATALERROR;
    } else if( Ecode( pccb->lpinfo.reason ) == SE_POWER ||
	       Ecode( pccb->lpinfo.reason ) == SE_POWERUP ) {
	pf_reason = PF_POWER;
    } else {
	pf_reason = PF_PORTERROR;
    }
    Unlock_pccb( pccb )
    ( void )cippd_stop( pccb, pf_reason );
    ( void )splx( save_ipl );
}

/*   Name:	ci_console_log	- Log CI Events to Console Terminal
 *
 *   Abstract:	This routine logs CI events to the console terminal.  The event
 *		is always one of the following types:
 *
 *		PATH_EVENT	- Path specific event
 *		RPORT_EVENT	- Remote port specific event
 *		LPORT_EVENT	- Local port specific event
 *
 *		Explicit formatting information must be provided for each
 *		event.  This requires updating of the following tables each
 *		time a new event is defined:
 *
 *		1. The appropriate entry within the CI console logging table(
 *		   ci_cltab[][] ) must be updated to reflect the new maximum
 *		   code represented within the associated format table.
 *
 *		2. The associated format table itself( ci_cli[], ci_clw[],
 *		   ci_clre[], ci_cle[], ci_clse[], ci_clfe[], ci_clppdse[] )
 *		   must be updated with both the class of variable information
 *		   and exact text to be displayed.  However, the appropriate
 *		   table should be updated with a NULL entry whenever the CI
 *		   port driver is specifically NOT to log a new event.  This
 *		   applies only to ci_clppdse[] when a new CI PPD severe error
 *		   event is to be specifically logged only by the CI PPD and
 *		   not by appropriate port drivers such as the CI port driver.
 *
 *		NOTE: Console logging of events is bypassed whenever the event
 *		      severity does not warrant console logging according to
 *		      the current CI severity level( ci_severity ).  Such
 *		      bypassing is overridden when the ECLAWAYS bit is set in
 *		      the event code indicating that the event is always to be
 *		      logged regardless of the current severity level.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      CI port driver with the exception of those CI PPD severe
 *		      error events which are candidates for application of the
 *		      local port crash severity modifier( ESM_LPC ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_cltab			- CI console logging table
 *   ci_severity		- CI console logging severity level
 *   cibp			- Address of CI port header( OPTIONAL )
 *   event			- Event code
 *   event_type			- PATH_EVENT, LPORT_EVENT, RPORT_EVENT
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   pb				- Path Block pointer( OPTIONAL )
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    devattn.cicpurevlev	-   Out-of-revision CPU microcode logging info
 *	    devattn.cirevlev	-   Port microcode information
 *	    max_fn_level	-   Maximum functional microcode revision level
 *	    max_rom_level	-   Maximum PROM/self-test microcode rev level
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access and
 *		prevent premature PB deletion when a PB is provided.
 *
 *		PBs do NOT require locking when provided because only static
 *		fields are accessed.  SBs NEVER require locking.
 */
void
ci_console_log( pccb, pb, cibp, event, event_type )
    register PCCB	*pccb;
    register PB		*pb;
    register GVPH	*cibp;
    register u_long	event;
    u_long		event_type;
{
    register u_long	fcode, severity = Eseverity( event ), data;

    /* Events are logged according to their type and the class of variable
     * information they display.  Console messages for path specific events(
     * PATH_EVENT ) display the local and remote port station addresses and
     * remote system name by default when the event is of severity level error(
     * ES_E ) or severe error( ES_SE ).  Console messages for local port
     * specific events( LPORT_EVENT ) display the local port station address by
     * default when they are of severity level error( ES_E ) or higher.
     * Console messages for remote port specific events( RPORT_EVENT ) do not
     * display any information by default.
     *     
     * The following classes of variable information currently exist:
     *
     * 1. Remote CI port station address.
     * 2. Local CI port station address.
     * 3. CI port parameter register( PPR ) only.
     * 4. CI port registers.
     * 5. BI registers.
     * 6. CI packet fields.
     * 7. Appropriate CI port microcode revision levels.
     * 8. CPU microcode revision levels.
     * 9. Initial local CI port initialization information.
     *
     * Certain events may also be logged without displaying any variable
     * information.
     *
     * Which CI port registers are selected for console logging( Class 3 ) is
     * dependent upon hardware port type:
     *
     * CI750		CNFR, PMCSR, and PSR
     * CI780
     * CIBCI
     *
     * CIBCA		BER, PMCSR, and PSR.
     *
     * CIXCD		XBER, PMCSR, and PSR.
     *
     * In the case of certain specific events the PESR is also logged.
     *
     * Only the CIBCI logs events which fall into the BI register class( Class
     * 5 ).  The CIBCA does NOT.  Registers displayed include: BICSR, BER, and
     * CNFR.
     *
     * The accessibility of each register location is always checked prior to
     * accessing because the local port is itself suspect whenever this routine
     * is invoked and requested to display register contents.  Inability to
     * access a location implies unavailability of the port itself( ie - brain
     * damage ) and the specific access is bypassed.  A more extensive
     * explanation of why and when register accesses require protection may be
     * found at the front of this module.
     *
     * A panic occurs whenever the CI port driver is not prepared to log the
     * event due to detection of any of the following circumstances:
     *
     * 1. The event type is unknown.
     * 2. The event is a SCS specific event.
     * 3. The severity level of the event is invalid.
     * 4. The code of the event exceeds the current maximum known code for the
     *	  class( CI or CI PPD ) and severity level of the event.
     * 5. The event is not represented within the appropriate console logging
     *    formatting table( indicating that the CI port driver should never
     *	  have been asked to log it in the first place ).
     * 6. The class of variable information associated with the event is
     *	  unknown.
     *
     * None of these circumstances should ever occur.
     *
     * NOTE: Events represented within console logging format tables by NULL
     *	     entries are events which are to be logged only by the CI PPD and
     *	     never by individual port drivers like the CI port driver.
     *	     Currently, only certain path specific CI PPD severe error events
     *	     fall into this category.
     */
    if(( event_type < PATH_EVENT || event_type > LPORT_EVENT ) ||
	 Test_scs_event( event )			       ||
	 severity > ES_FE				       ||
	 Ecode( event ) > Clog_maxcode( ci_cltab, event )      ||
	 Clog_tabmsg( ci_cltab, event ) == NULL ) {
	( void )panic( PANIC_UNKCODE );
    } else if(( fcode = Clog_tabcode( ci_cltab, event )) &&
	      ( fcode < CF_RPORT || fcode > CF_INIT )) {
	( void )panic( PANIC_UNKCF );
    } else if( ci_severity > severity && !Test_cloverride( event )) {
	return;
    }
    ( void )cprintf( "%4s\t- ", &pccb->lpinfo.name );
    switch( event_type ) {

	case PATH_EVENT:
	    if( severity == ES_E || severity == ES_SE ) {
		if( severity == ES_E ) {
		    ( void )cprintf( "error on path" );
		} else {
		    ( void )cprintf( "severe error on path" );
		}
		( void )cprintf( "( local/remote port: %u/",
				 Scaaddr_low( pccb->lpinfo.addr ));
		if( pb ) {
		    ( void )cprintf( "%u, remote system: ",
				     Scaaddr_low( pb->pinfo.rport_addr ));
		    if( pb->sb ) {
			( void )cprintf( "%8s )\n\t- ",
					 pb->sb->sinfo.node_name );
		    } else {
			( void )cprintf( "? )\n\t- " );
		    }
		} else {
		    ( void )cprintf( "?, remote system: ? )\n\t- " );
		}
	    }
	    break;

	case LPORT_EVENT:
	    if( severity >= ES_E ) {
		if( Test_lpc_event( event )) {
		    ( void )cprintf( "port failing, " );
		}
		switch( severity ) {

		    case ES_E:
			( void )cprintf( "error( local port %u )\n\t- ",
					 Scaaddr_low( pccb->lpinfo.addr ));
			break;

		    case ES_SE:
			( void )cprintf( "severe error( local port %u )\n\t- ",
					 Scaaddr_low( pccb->lpinfo.addr ));
			break;

		    case ES_FE:
			( void )cprintf( "fatal error( local port %u )\n\t- ",
					 Scaaddr_low( pccb->lpinfo.addr ));
			break;
		}
	    }
	    break;
    }
    ( void )cprintf( "%s", Clog_tabmsg( ci_cltab, event ));
    switch( fcode ) {

	case CF_NONE:
	    ( void )cprintf( "\n" );
	    break;

	case CF_RPORT:
	    if( pb ) {
		( void )cprintf( "( remote port: %u )\n",
				 Scaaddr_low( pb->pinfo.rport_addr ));
	    } else {
		( void )cprintf( "( remote port: ? )\n" );
	    }
	    break;

	case CF_LPORT:
	    ( void )cprintf( "( local port %u )\n",
			     Scaaddr_low( pccb->lpinfo.addr ));
	    break;

	case CF_PPR:
	    Lock_cidevice( pccb )
	    data = Get_reg( pccb->Ppr );
	    Unlock_cidevice( pccb )
	    ( void )cprintf( "\n\t- ppr: 0x%08lx\n", data );
	    break;

	case CF_REGS:
	    switch( pccb->lpinfo.type.hwtype ) {

		case HPT_CI750:
		case HPT_CI780:
		case HPT_CIBCI:
		    ( void )cprintf( "\n\t- cnfr/pmcsr/psr: 0x%08lx",
				     Get_reg( pccb->Cnfr ));
		    break;

		case HPT_CIBCA_AA:
		case HPT_CIBCA_BA:
		    ( void )cprintf( "\n\t- ber/pmcsr/psr: 0x%08lx",
				     Get_reg( pccb->Bierr ));
		    break;

		case HPT_CIXCD:
	       	    Lock_cidevice( pccb )
		    data = Get_reg( pccb->Xbe );
	    	    Unlock_cidevice( pccb )
		    ( void )cprintf( "\n\t- xber/pmcsr/psr: 0x%08lx", data );
		    break;

		case HPT_CIKMF:
		    ( void )cprintf( "\n\t- xpcpser/xpcpstat/pmcsr/psr:\n\t0x%08lx/0x%08lx",
			    Get_reg( pccb->Xpcpser ),Get_reg( pccb->Xpcpstat ));
		    break;

		default:
		    ( void )panic( PANIC_HPT );
	    }
	    Lock_cidevice( pccb )
	    data = Get_reg( pccb->Pmcsr );
	    Unlock_cidevice( pccb )
	    ( void )cprintf( "/0x%08lx", data );
	    Lock_cidevice( pccb )
	    data = Get_reg( pccb->Psr );
	    Unlock_cidevice( pccb )
	    ( void )cprintf( "/0x%08lx\n", data );
	    break;

	case CF_REGS2:
	    switch( pccb->lpinfo.type.hwtype ) {

		case HPT_CI750:
		case HPT_CI780:
		case HPT_CIBCI:
		    ( void )cprintf( "\n\t- cnfr/pmcsr/psr/pesr/pfar:\n\t0x%08lx", Get_reg( pccb->Cnfr ));
		    break;

		case HPT_CIBCA_AA:
		case HPT_CIBCA_BA:
		    ( void )cprintf( "\n\t- ber/pmcsr/psr/pesr/pfar:\n\t0x%08lx", Get_reg( pccb->Bierr ));
		    break;

		case HPT_CIXCD:
	    	    Lock_cidevice( pccb )
	    	    data = Get_reg( pccb->Xbe );
	    	    Unlock_cidevice( pccb )
		    ( void )cprintf( "\n\t- xber/pmcsr/psr/pesr/pfar:\n\t0x%08lx", data );
		    break;

		case HPT_CIKMF:
		    ( void )cprintf( "\n\t- xpcpser/xpcpstat/pmcsr/psr/pesr/pfar:\n\t0x%08lx/0x%08lx", Get_reg( pccb->Xpcpser ),Get_reg( pccb->Xpcpstat ));
		    break;

		default:
		    ( void )panic( PANIC_HPT );
	    }
	    Lock_cidevice( pccb )
	    data = Get_reg( pccb->Pmcsr );
	    Unlock_cidevice( pccb )
	    ( void )cprintf( "/0x%08lx", data );
	    Lock_cidevice( pccb )
	    data = Get_reg( pccb->Psr );
	    Unlock_cidevice( pccb )
	    ( void )cprintf( "/0x%08lx", data );
	    Lock_cidevice( pccb )
	    data = Get_reg( pccb->Pesr );
	    Unlock_cidevice( pccb )
	    ( void )cprintf( "/0x%08lx", data );
	    Lock_cidevice( pccb )
	    data = Get_reg( pccb->Pfar );
	    Unlock_cidevice( pccb )
	    ( void )cprintf( "/0x%08lx\n", data );
	    break;

	case CF_BIREGS:
	    if( pccb->lpinfo.type.hwtype == HPT_CIBCI ) {
		cprintf( "\n\t- bicsr/ber/cnfr: 0x%08lx/0x%08lx/0x%08lx\n",
			 Get_reg( pccb->Bictrl ),
			 Get_reg( pccb->Bierr ),
			 Get_reg( pccb->Cnfr ));
	    } else {
		( void )panic( PANIC_HPT );
	    }
	    break;

	case CF_PKT:
	    ( void )cprintf( "\n\t- flags/opcode/status/port: " );
	    if( cibp ) {
		( void )cprintf( "0x%02x/0x%02x/0x%02x/0x%02x\n",
				 ( u_long )*( u_char * )&cibp->flags,
				 ( u_long )cibp->opcode,
				 ( u_long )*( u_char * )&cibp->status,
				 ( u_long )Get_pgrp( pccb, cibp ));
	    } else {
		( void )cprintf( "0x??/0x??/0x??/0x??\n" );
	    }
	    break;

	case CF_UCODE:
	    if( severity < ES_SE ) {
		( void )cprintf( "( local port %u )",
				 Scaaddr_low( pccb->lpinfo.addr ));
	    }
	    switch( pccb->lpinfo.type.hwtype ) {

		case HPT_CI750:
		case HPT_CI780:
		case HPT_CIBCI:
		    ( void )cprintf( "\n\t- current ram/prom:   %u/%u",
				     pccb->Devattn.cirevlev.ci_ramlev,
				     pccb->Devattn.cirevlev.ci_romlev );
		    ( void )cprintf( "\n\t- supported ram/prom: %u/%u\n",
				     pccb->Max_fn_level,
				     pccb->Max_rom_level );
		    break;

		case HPT_CIBCA_AA:
		case HPT_CIBCA_BA:
		case HPT_CIXCD:
		case HPT_CIKMF:
		    ( void )cprintf( "\n\t- current functional/self-test " );
		    ( void )cprintf( "microcode levels:   %u/%u",
				     pccb->Devattn.cirevlev.ci_ramlev,
				     pccb->Devattn.cirevlev.ci_romlev );
		    ( void )cprintf( "\n\t- supported functional/self-test " );
		    ( void )cprintf( "microcode levels: %u/%u\n",
				     pccb->Max_fn_level,
				     pccb->Max_rom_level );
		    break;

		default:
		    ( void )panic( PANIC_HPT );
	    }
	    break;

	case CF_CPU:
	    ( void )cprintf( "\n\t- current/minimum %4s CPU microcode ",
			     ( u_char * )&pccb->Devattn.cicpurevlev.ci_hwtype);
	    ( void )cprintf( "revision level is: %u/%u\n",
			     pccb->Devattn.cicpurevlev.ci_currevlev,
			     pccb->Devattn.cicpurevlev.ci_mincpurev );
	    break;

	case CF_INIT:
	    ( void )cprintf( "( local port %u )",
			     Scaaddr_low( pccb->lpinfo.addr ));
	    switch( pccb->lpinfo.type.hwtype ) {

		case HPT_CI750:
		case HPT_CI780:
		case HPT_CIBCI:
		    ( void )cprintf( "\n\t- ram/prom: " );
		    break;

		case HPT_CIBCA_AA:
		case HPT_CIBCA_BA:
		case HPT_CIXCD:
		case HPT_CIKMF:
		    cprintf("\n\t- functional/self-test microcode levels: " );
		    break;

		default:
		    ( void )panic( PANIC_HPT );
	    }
	    ( void )cprintf( "%u/%u\n",
			     pccb->Devattn.cirevlev.ci_ramlev,
			     pccb->Devattn.cirevlev.ci_romlev );
	    break;
    }
}

/*   Name:	ci_log_badport	- Log Bad Port Numbers in CI Packets
 *
 *   Abstract:	This routine logs bad port numbers in CI packets.  For a port
 *		number within a packet to be considered bad it must exceed the
 *		hardware maximum port number of the specified local port.
 *
 *		NOTE: This is a mandatory PD routine( Log_badportnum ) for use
 *		      by the CI PPD.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by
 *		ci_log_packet(), the routine which performs the actual logging.
 */
void
ci_log_badport( pccb, cippdbp )
    PCCB	*pccb;
    GVPPPDH	*cippdbp;
{
    ( void )ci_log_packet( pccb,
			   NULL,
			   Ppd_to_pd( cippdbp, pccb ),
			   SE_BADPORTNUM,
			   LPORT_EVENT );
}

/*   Name:	ci_log_dev_attn	- Log CI Device Attention Events
 *
 *   Abstract:	This routine logs CI device attention events.  Such events are
 *		detected directly from a specific local CI port as opposed to
 *		those events ascertained indirectly from a CI port packet.  The
 *		event is also optionally logged to the console.
 *
 *		Nine classes of events are currently logged by this routine:
 *
 *		1. Software errors detected during port initialization.
 *		2. Software detected hardware problems.
 *		3. Software detected invalid hardware CI port types.
 *		4. Software detected local port failures.
 *		5. CPU or port microcode problems.
 *		6. Explicit hardware errors.
 *		7. Stray interrupts.
 *		8. Failures to obtain access to port queue memory interlocks.
 *		9. Local port initializations.
 *
 *		Many of these events represent serious errors and are logged to
 *		save relevant information before drastic steps are attempted to
 *		resolve them.  Others are less serious and are logged only to
 *		give a warning or for informational purposes only.
 *
 *		NOTE: Stray interrupts are specific interrupts processed by the
 *		      special routine ci_unmapped_isr().  This routine serves
 *		      as the interrupt service handler for local ports without
 *		      power or marked broken and permanently shutdown.  Refer
 *		      to it for a more extensive explanation of what interrupts
 *		      are considered stray.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      CI port driver.  It currently does NOT even log those CI
 *		      PPD events which are candidates for application of the
 *		      local port crash severity modifier( ESM_LPC ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   event			- Event code
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *      pd.gvp.type.ci          -  CI specific PCCB fields
 *	    devattn.cicpurevlev	-   Out-of-revision CPU microcode information
 *	    devattn.cirevlev	-   Port microcode information
 *	    devattn.ciucode	-   Faulty microcode information
 *   regs			- LOG_REGS or LOG_NOREGS
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.nerrs		-  Number of errors on port
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access and as
 *		required by ci_console_log(), the routine responsible for
 *		logging events to the console terminal.
 */
void
ci_log_dev_attn( pccb, event, regs )
    register PCCB		*pccb;
    register u_long		event;
    u_long			regs;
{
    register u_long		size;
    register u_char		*celp;
    register struct el_rec	*elp;
    register u_long		severity = Eseverity( event ), data;

    /* The steps involved in logging device attention events include:
     *
     * 1. Logging the event to the console.
     * 2. Incrementing the counter of local port errors.
     * 3. Computing the size of the application portion of the event log
     *	  record.
     * 4. Allocating an event log record and initializing the record's sub id
     *	  packet fields.
     * 5. Initializing the portion of the record common to all CI events.
     * 6. Initializing the portion of the record reserved for register
     *    contents.
     * 7. Moving any optional device attention information into the record.
     * 8. Validating the event log record.
     *
     * The ability of this routine to log the event is validated during console
     * logging( Step 1 ).  A panic occurs whenever the CI port driver is not
     * prepared to log the reported event.
     *
     * Not all events increment the counter of errors which have occurred on
     * the specified local port( Step 2 ).  Only those events with severity
     * level equal to or greater than error( ES_E ) increment the counter.
     *
     * This routine immediately terminates on failure to allocate an event log
     * record( Step 4 ).
     *
     * Not all device attention events request logging of device registers(
     * Step 6 ).  Sufficient space is reserved within the event log record for
     * register contents only for those that do.  Both CI port registers and
     * interconnect specific registers are logged when register logging is
     * requested.
     *
     * Accessibility of register locations are always checked prior to
     * accessing because the local port is itself suspect whenever this routine
     * is invoked and requested to log register contents.  Accessibility to
     * each CI port register is explicitly tested before accessing while only
     * general accessibility to the interconnect registers, and not to each
     * separate one, is tested before accessing.  Inability to access a
     * location implies unavailability of the port itself( ie - brain damage )
     * and the specific access is bypassed.  A more extensive explanation of
     * why and when register accesses require protection may be found at the
     * front of this module.
     *
     * The following types of mutually exclusive optional device attention
     * information may require logging( Step 7 ):
     *
     * 1. Microcode revision levels.
     * 2. CPU microcode revision levels.
     * 3. Information about improperly loaded functional microcode.
     *
     * Sufficient space is reserved within the event record for such optional
     * information only for those events which require its logging.  Logging of
     * microcode revision levels is required by four events: I_LPORT_INIT,
     * I_LPORT_REINIT, FE_BADUCODE and W_UCODE_WARN.  Logging of CPU microcode
     * revision levels is required by a single event: FE_CPU.  Logging of
     * information about improperly loaded functional microcode is also
     * required only by a single event: E_UCODE_LOAD.
     *
     * NOTE: Requests for logging of register contents and requirements for
     *	     logging of optional device attention information are orthogonal.
     *	     In other words, an event may request register content logging and
     *	     require logging of optional information, or neither, or any 
     *	     combination in between.
     *
     * NOTE: Fields reserved for registers which are either unused by local CI
     *	     ports of a specific hardware port type or are currently
     *	     inaccessible are initialized to EL_UNDEF.
     */
    ( void )ci_console_log( pccb, NULL, NULL, event, LPORT_EVENT );
    if( Eseverity( event ) >= ES_E ) {
	Event_counter( pccb->lpinfo.nerrs )
    }
    if( ci_errlog > severity && 
	!Test_cloverride( event ) &&
	ci_errlog < SCA_ERRLOG3 ) {
	return;
    }
    size = sizeof( struct ci_common );
    if( regs == LOG_REGS ) {
	size += sizeof( struct ci_regs );
	switch( pccb->Interconnect ) {

	    case ICT_SBICMI:
		break;

	    case ICT_BI:
		size += sizeof( struct bi_regs );
		break;

	    case ICT_XMI:
		size += sizeof( struct cixmi_regs );
		break;

	    default:
		( void )panic( PANIC_IC );
	}
    }
    switch( Mask_esevmod( event )) {

	case I_LPORT_REINIT:
	case I_LPORT_INIT:
	case W_UCODE_WARN:
	case FE_BADUCODE:
	    size += sizeof( struct ci_revlev );
	    break;

	case E_UCODE_LOAD:
	    size += sizeof( struct ci_ucode );
	    break;

	case FE_CPU:
	    size += sizeof( struct ci_cpurevlev );
	    break;
    }
    if(( elp = ealloc( size, EL_PRIHIGH )) == EL_FULL ) {
	return;
    }
    LSUBID( elp,
	    ELCT_DCNTL,
	    ELCI_ATTN,
	    pccb->lpinfo.type.hwtype,
	    Ctrl_from_name( pccb->lpinfo.name ),
	    EL_UNDEF,
	    event )
    Elcicommon( elp )->ci_optfmask1 = 0;
    Elcicommon( elp )->ci_optfmask2 = 0;
    Elcicommon( elp )->ci_evpktver = CI_EVPKTVER;
    U_long( *Elcicommon( elp )->ci_lpname ) = pccb->lpinfo.name;
    Move_node( lscs.system.node_name, Elcicommon( elp )->ci_lname )
    Move_scaaddr( lscs.system.sysid, *Elcicommon( elp )->ci_lsysid )
    Move_scaaddr( pccb->lpinfo.addr, *Elcicommon( elp )->ci_lsaddr )
    Elcicommon( elp )->ci_nerrs = pccb->lpinfo.nerrs;
    Elcicommon( elp )->ci_nreinits = pccb->lpinfo.nreinits;
    celp = ( u_char * )Elcidattn( elp );
    if( regs == LOG_REGS ) {
	Elcicommon( elp )->ci_optfmask1 |= CI_REGS;
	switch( pccb->lpinfo.type.hwtype ) {

	    case HPT_CI750:
	    case HPT_CI780:
	    case HPT_CIBCI:
		Elciciregs( celp )->ci_cnfr = Get_reg( pccb->Cnfr );
		break;

	    case HPT_CIBCA_AA:
	    case HPT_CIBCA_BA:
	    case HPT_CIXCD:
	    case HPT_CIKMF:
		Elciciregs( celp )->ci_cnfr = EL_UNDEF;
		break;

	    default:
		( void )panic( PANIC_HPT );
	}
	Lock_cidevice( pccb )
	data = Get_reg( pccb->Pmcsr );
	Unlock_cidevice( pccb )
	Elciciregs( celp )->ci_pmcsr = data;
	Lock_cidevice( pccb )
	data = Get_reg( pccb->Psr );
	Unlock_cidevice( pccb )
	Elciciregs( celp )->ci_psr = data;
	Lock_cidevice( pccb )
	data = Get_reg( pccb->Pfar );
	Unlock_cidevice( pccb )
	Elciciregs( celp )->ci_pfaddr = data;
	Lock_cidevice( pccb )
	data = Get_reg( pccb->Pesr );
	Unlock_cidevice( pccb )
	Elciciregs( celp )->ci_pesr = data;
	Lock_cidevice( pccb )
	data = Get_reg( pccb->Ppr );
	Unlock_cidevice( pccb )
	Elciciregs( celp )->ci_ppr = data;
	celp += sizeof( struct ci_regs );
	switch( pccb->Interconnect ) {

	    case ICT_SBICMI:
		break;

	    case ICT_BI:
		Elcicommon( elp )->ci_optfmask1 |= CI_BIREGS;
		Elcibiregs( celp )->bi_err_int = EL_UNDEF;
		if( !Bad_reg( pccb->Bityp )) {
		    Elcibiregs( celp )->bi_typ = *pccb->Bityp;
		    Elcibiregs( celp )->bi_ctrl = *pccb->Bictrl;
		    Elcibiregs( celp )->bi_err = *pccb->Bierr;
		    Elcibiregs( celp )->bi_int_dst = *pccb->Biint_dst;
		} else {
		    Elcibiregs( celp )->bi_typ = EL_UNDEF;
		    Elcibiregs( celp )->bi_ctrl = EL_UNDEF;
		    Elcibiregs( celp )->bi_err = EL_UNDEF;
		    Elcibiregs( celp )->bi_int_dst = EL_UNDEF;
		}
		celp += sizeof( struct bi_regs );
		break;

	    case ICT_XMI:
		Elcicommon( elp )->ci_optfmask1 |= CI_XMIREGS;
		Lock_cidevice( pccb )
		if( !Bad_reg( pccb->Xdev )) {
		    Unlock_cidevice( pccb )
		    Lock_cidevice( pccb )
		    data = *pccb->Xdev;
		    Unlock_cidevice( pccb )
		    Elcixmiregs( celp )->xdev = data;
		    Lock_cidevice( pccb )
		    data = *pccb->Xbe;
		    Unlock_cidevice( pccb )
		    Elcixmiregs( celp )->xbe = data;
		    Lock_cidevice( pccb )
		    data = *pccb->Xfadrl;
		    Unlock_cidevice( pccb )
		    Elcixmiregs( celp )->xfadrl = data;
		    Lock_cidevice( pccb )
		    data = *pccb->Xfadrh;
		    Unlock_cidevice( pccb )
		    Elcixmiregs( celp )->xfadrh = data;
		    if( pccb->lpinfo.type.hwtype == HPT_CIXCD ) {
		        Lock_cidevice( pccb )
		        data = *pccb->Xcd_pidr;
		        Unlock_cidevice( pccb )
		        Elcixmiregs( celp )->pidr = data;
		        Lock_cidevice( pccb )
		        data = *pccb->Xcd_pvr;
		        Unlock_cidevice( pccb )
		        Elcixmiregs( celp )->pvr = data;
		    } else {
		        Elcixmiregs( celp )->pidr = EL_UNDEF;
		        Elcixmiregs( celp )->pvr = EL_UNDEF;
		    }
		} else {
		    Unlock_cidevice( pccb )
		    Elcixmiregs( celp )->xdev = EL_UNDEF;
		    Elcixmiregs( celp )->xbe = EL_UNDEF;
		    Elcixmiregs( celp )->xfadrl = EL_UNDEF;
		    Elcixmiregs( celp )->xfadrh = EL_UNDEF;
		    Elcixmiregs( celp )->pidr = EL_UNDEF;
		    Elcixmiregs( celp )->pvr = EL_UNDEF;
		}
		celp += sizeof( struct cixmi_regs );
		break;

	    default:
		( void )panic( PANIC_IC );
	}
    }
    switch( Mask_esevmod( event )) {

	case I_LPORT_REINIT:
	case I_LPORT_INIT:
	case W_UCODE_WARN:
	case FE_BADUCODE:
	    Elcicommon( elp )->ci_optfmask1 |= CI_REVLEV;
	    *Elcirevlev( celp ) = pccb->Devattn.cirevlev;
	    break;

	case E_UCODE_LOAD:
	    Elcicommon( elp )->ci_optfmask1 |= CI_UCODE;
	    *Elciucode( celp ) = pccb->Devattn.ciucode;
	    break;

	case FE_CPU:
	    Elcicommon( elp )->ci_optfmask1 |= CI_CPUREVLEV;
	    *Elcicpurevlev( celp ) = pccb->Devattn.cicpurevlev;
	    break;
    }
    EVALID( elp )
}

/*   Name:	ci_log_initerr	- Log CI Port Initialization Fatal Errors
 *
 *   Abstract:	This routine logs a special type of CI device attention event:
 *		software errors detected during probing of local CI ports.
 *		These fatal error events are logged as device attentions
 *		because they pertain to a specific local CI port.  However,
 *		they are considered special because they pre-date allocation of
 *		a PCCB for the local port, and therefore, may not make use of
 *		it for event logging purposes.  The following special events
 *		are currently defined:
 *
 *		1. FE_INIT_NOMEM   - Insufficient dynamic memory
 *		2. FE_INIT_ZEROID  - Uninitialized system identification num
 *		3. FE_INIT_NOUCODE - CI microcode absent
 *		4. FE_INIT_UNKHPT  - Unknown hardware port type
 *		5. FE_INIT_MISMTCH - Mismatched CI Port ucode & hw port types
 *
 *		All such events represent fatal errors.  None ever have the
 *		local port crash severity modified( ESM_LPC ) applied.
 *
 *		The occurrence of an port initialization fatal event is also
 *		automatically logged to the console, but without variable
 *		information.
 *
 *		Explicit formatting information must be provided for each
 *		event.  This requires updating of the following tables each
 *		time a new CI fatal error event is defined:
 *
 *		1. The CI fatal error event table( ci_clfe[]) must be updated
 *		   with the exact text to be displayed and the console logging
 *		   format code CF_NONE( CI fatal error events discovered during
 *		   port probing currently never display variable information ).
 *
 *		2. The fatal error event entry within the CI console logging
 *		   table( ci_cltab[][] ) must be updated to reflect the new
 *		   maximum fatal error event code.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_cltab			- CI console logging table
 *   cinum			- CI adapter number
 *   event			- Fatal error event code
 *   hpt			- Hardware port type
 *   interconnect		- Interconnect type
 *   scs_node_name		- SCS node name
 *   scs_system_id		- SCS system identification number
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.
 */
void
ci_log_initerr( cinum, interconnect, hpt, event )
    u_long			cinum;
    u_long			interconnect;
    u_long			hpt;
    register u_long		event;
{
    register struct el_rec	*elp;

    /* Logging of fatal errors detected during CI port probings proceeds as
     * follows:
     *
     * 1. The fatal error is validated.
     * 2. The fatal error event and a local port permanently offline message
     *	  are both printed on the console.
     * 3. An event log record is allocated and the record's sub id packet
     *	  fields are initialized.
     * 4. The portion of record common to all CI events is initialized.
     * 5. The event log record is validated.
     *
     * This routine panics if validation of the fatal error event( Step 1 )
     * fails indicating inability of the CI port driver to log the reported
     * event.
     *
     * Event logging is bypassed on failures to allocate an event log record(
     * Step 3 ).
     *
     * Note that no other CI specific information common to device attention
     * events is logged and that no variable information is displayed within
     * logged console messages.
     */
    switch( event ) {

	case FE_INIT_NOMEM:
	case FE_INIT_ZEROID:
	case FE_INIT_NOUCODE:
	case FE_INIT_UNKHPT:
	case FE_INIT_MISMTCH:
	    break;

	default:
	    ( void )panic( PANIC_UNKCODE );
    }
    ( void )cprintf( "ci%u\t- fatal error( local port ? )\n\t- %s\n",
		     cinum, Clog_tabmsg( ci_cltab, event ));
    ( void )cprintf( "ci%u\t- permanently offline( local port ? )\n", cinum );
    if(( elp = ealloc( sizeof( struct ci_common ), EL_PRIHIGH )) == EL_FULL ) {
	LSUBID( elp, ELCT_DCNTL, ELCI_ATTN, hpt, cinum, EL_UNDEF, event )
	Elcicommon( elp )->ci_optfmask1 = 0;
	Elcicommon( elp )->ci_optfmask2 = 0;
	Elcicommon( elp )->ci_evpktver = CI_EVPKTVER;
	U_long( *Elcicommon( elp )->ci_lpname ) = Ctrl_from_num( "ci  ",cinum);
	Move_node( scs_node_name, Elcicommon( elp )->ci_lname )
	Move_scaaddr( scs_system_id, *Elcicommon( elp )->ci_lsysid )
	U_short( Elcicommon( elp )->ci_lsaddr[ 0 ]) = EL_UNDEF;
	U_short( Elcicommon( elp )->ci_lsaddr[ 2 ]) = EL_UNDEF;
	U_short( Elcicommon( elp )->ci_lsaddr[ 4 ]) = EL_UNDEF;
	Elcicommon( elp )->ci_nerrs = 1;
	Elcicommon( elp )->ci_nreinits = 0;
	EVALID( elp )
    }
}

/*   Name:	ci_log_packet	- Log CI Packet Related Events
 *
 *   Abstract:	This routine logs CI packet related events.  Such events are
 *		ascertained indirectly from a CI port packet as opposed to
 *		those events detected directly from a specific local CI port.
 *		The event is also optionally logged to the console.
 *
 *		Five classes of events are currently logged by this routine:
 *
 *		1. Hardware detected errors during port command processing.
 *		2. Software detected invalid remote port states.
 *		3. Sofware detected invalid CI packet port numbers.
 *		4. All changes in cable states.
 *		5. Reception of packets over software non-existent paths.
 *
 *		Many of these events represent serious errors and are logged to
 *		save relevant information before drastic steps are attempted to
 *		resolve them.  Others are less serious and are logged only to
 *		give a warning or for informational purposes only.
 *
 *		NOTE: While all events logged therein arise indirectly from CI
 *		      port packets, the logging of each event does not
 *		      necessarily involve logging of the packet itself.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      CI port driver with the exception of those CI PPD events
 *		      which are candidates for application of the local port
 *		      crash severity modifier( ESM_LPC ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cibp			- Address of CI port header( OPTIONAL )
 *   event			- Event code
 *   event_type			- PATH_EVENT, LPORT_EVENT, RPORT_EVENT
 *   lscs			- Local system permanent information
 *   pb				- Path Block pointer( OPTIONAL )
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.nerrs		-  Number of errors on port
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access, to
 *		prevent premature PB deletion when a PB is provided, and as
 *		required by ci_console_log(), the routine responsible for
 *		logging events to the console terminal.
 *
 *		PBs do NOT require locking when provided because only static
 *		fields are accessed.  SBs NEVER require locking.
 */
void
ci_log_packet( pccb, pb, cibp, event, event_type )
    register PCCB		*pccb;
    register PB			*pb;
    GVPH			*cibp;
    register u_long		event;
    u_long			event_type;
{
    register struct el_rec	*elp;
    register u_long		opt_size;
    register u_long		severity = Eseverity( event );

    /* The steps involved in logging packet related events include:
     *
     * 1. Logging the event to the console.
     * 2. Incrementing the counter of local port errors.
     * 3. Computing the size of the application portion of the event log
     *	  record.
     * 4. Allocating an event log record and initializing the record's sub id
     *	  packet fields.
     * 5. Initializing the portion of the record common to all CI events.
     * 6. Initializing the portion of the record common to all CI packet
     *	  related events.
     * 7. Moving any optional logged packet information into the record.  
     * 8. Validating the event log record.
     *
     * The ability of this routine to log the event is validated during console
     * logging( Step 1 ).  A panic occurs whenever the CI port driver is not
     * prepared to log the reported event.
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
     * 1. The CI port packet responsible for the logged packet related event.
     *
     * Optional CI port packets are associated with many different events and
     * vary widely in size based upon their port operation codes.  Such packets
     * require truncation whenever logging their full size would exceed the
     * maximum size of an event log record.
     */
    ( void )ci_console_log( pccb, pb, cibp, event, event_type );
    if( Eseverity( event ) >= ES_E ) {
	Event_counter( pccb->lpinfo.nerrs )
    }
    if( ci_errlog > severity && 
	!Test_cloverride( event ) &&
	ci_errlog < SCA_ERRLOG3 ) {
	return;
    }
    {
    register u_long	size = sizeof( struct ci_common ) +
			       sizeof( struct ci_lcommon );

    if( cibp ) {
	opt_size = sizeof( struct ci_packet );
	switch( cibp->opcode & 0x3f ) {

	    case SNDDG:   case SNDMSG:   case DGREC:   case MSGREC:
		opt_size += ( sizeof( GVPPPDH ) +
			      Appl_size( Pd_to_ppd( cibp, pccb )));
		break;

	    case SNDDAT:  case REQDAT1:  case REQDAT0:  case REQDAT2:
	    case RETDAT:
		opt_size += sizeof( SNDDATH );
		break;

	    case RETCNF:  case CNFREC:   case DATREC:   case REQID:
	    case SNDRST:
		opt_size += sizeof( REQIDH );
		break;

	    case IDREC:
		opt_size += sizeof( IDRECH );
		break;

	    case SETCKT:
		opt_size += sizeof( SETCKTH );
		break;

	    case SNDSTRT:
		opt_size += sizeof( SNDSTRTH );
		break;

	    case SNDLB:
		opt_size += sizeof( SNDLBH );
		break;

	    case LBREC:
		opt_size += sizeof( LBRECH );
		break;

	    case INVTC:
		break;

	    default:
		opt_size += ( sizeof( IDRECH ) - pccb->lpinfo.Ovhd_pd );
		break;
	}
	if(( size += opt_size ) > EL_MAXAPPSIZE ) {
	    opt_size += ( EL_MAXAPPSIZE - size );
	    size = EL_MAXAPPSIZE;
	}
    }
    if(( elp = ealloc( size, EL_PRIHIGH )) == EL_FULL ) {
	return;
    }
    LSUBID( elp,
	    ELCT_DCNTL,
	    ELCI_LPKT,
	    pccb->lpinfo.type.hwtype,
	    Ctrl_from_name( pccb->lpinfo.name ),
	    EL_UNDEF,
	    event )
    }
    Elcicommon( elp )->ci_optfmask1 = CI_LCOMMON;
    Elcicommon( elp )->ci_optfmask2 = 0;
    Elcicommon( elp )->ci_evpktver = CI_EVPKTVER;
    U_long( *Elcicommon( elp )->ci_lpname ) = pccb->lpinfo.name;
    Move_scaaddr( pccb->lpinfo.addr, *Elcicommon( elp )->ci_lsaddr )
    Move_scaaddr( lscs.system.sysid, *Elcicommon( elp )->ci_lsysid )
    Move_node( lscs.system.node_name, Elcicommon( elp )->ci_lname )
    Elcicommon( elp )->ci_nerrs = pccb->lpinfo.nerrs;
    Elcicommon( elp )->ci_nreinits = pccb->lpinfo.nreinits;
    {
    register SB		*sb;

    if( pb ) {
	Move_scaaddr( pb->pinfo.rport_addr, *Elcilcommon( elp )->ci_rsaddr )
	sb = pb->sb;
    } else {
	U_short( Elcilcommon( elp )->ci_rsaddr[ 0 ]) = EL_UNDEF;
	U_short( Elcilcommon( elp )->ci_rsaddr[ 2 ]) = EL_UNDEF;
	U_short( Elcilcommon( elp )->ci_rsaddr[ 4 ]) = EL_UNDEF;
	sb = NULL;
    }
    if( sb ) {
	Move_scaaddr( sb->sinfo.sysid, *Elcilcommon( elp )->ci_rsysid )
	Move_node( sb->sinfo.node_name, Elcilcommon( elp )->ci_rname )
    } else {
	U_short( Elcilcommon( elp )->ci_rsysid[ 0 ])  = EL_UNDEF;
	U_short( Elcilcommon( elp )->ci_rsysid[ 2 ])  = EL_UNDEF;
	U_short( Elcilcommon( elp )->ci_rsysid[ 4 ])  = EL_UNDEF;
	U_long( Elcilcommon( elp )->ci_rname[ 0 ]) = EL_UNDEF;
	U_long( Elcilcommon( elp )->ci_rname[ 4 ]) = EL_UNDEF;
    }
    }
    if( cibp ) {
	if( pccb->lpinfo.flags.expl ) {
	    Elcicommon( elp )->ci_optfmask1 |= ( CI_PACKET | CI_EXPADRS );
	} else {
	    Elcicommon( elp )->ci_optfmask1 |= CI_PACKET;
	}
	Elcipacket( elp )->size = opt_size;
	( void )bcopy( &cibp->port, &Elcipacket( elp )->ci_port, opt_size );
    }
    EVALID( elp )
}

/*   Name:	ci_map_port	- Map Local CI Port
 *
 *   Abstract:	This routine maps a specified local CI port.  It is invoked
 *		only during processing of interrupts by the special routine(
 *		ci_unmapped_isr()) which handles all interrupts, regardless of
 *		hardware port type, whenever local ports are unmapped.  Local
 *		ports maybe unmapped because they are temporarily without power
 *		or because they are marked broken and are permanently shutdown.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   map_extent			- MAP_REGS or MAP_FULL
 *   pccb			- Port Command and Control Block pointer
 *      pd.gvp.type.ci          -  CI specific PCCB fields
 *	    lpstatus.mapped	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Adapter I/O space successfully mapped
 *   RET_FAILURE		- Adapter I/O space unsuccessfully mapped
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access to the
 *		PCCB and provide exclusive access to the CIADAP vector element
 *		corresponding to the local CI port.
 */
u_long
ci_map_port( pccb, map_extent )
    register PCCB	*pccb;
    u_long		map_extent;
{
    register CIADAP	*ciadap;
    register struct pte	*pteptr;
    register u_long	iopfn, nptes;
    register u_char	*vaddr;
    u_long		status = RET_SUCCESS;

    /* Mapping a local CI port includes:
     *
     * 1. Mapping local port adapter I/O space system PTEs back to their
     *	  original physical addresses, invalidating the system translation
     *	  buffers for each address mapped.
     * 2. Changing the interrupt service handler for the local port back to the
     *	  routine normally employed for this hardware port type.
     * 3. Verifying accessibility of the local port.
     *
     * The interrupt service handler for the local port is restored back to the
     * routine normally employed for this hardware port type only if full
     * mapping( map_extent == MAP_FULL ) was requested( Step 2 ).  Otherwise,
     * only the adapter I/O space is mapped.
     *
     * Failure is returned whenever the adapter can not be successfully
     * accessed following restoration of local port adapter I/O space system
     * PTEs( Step 3 ).  A panic occurs if this routine is invoked to map an
     * already mapped adapter.
     */
    if( !pccb->Lpstatus.mapped ) {
	ciadap = pccb->Ciadap;
	for( pteptr = ciadap->iopte,
	     nptes = ciadap->npages,
	     vaddr = ciadap->viraddr,
	     iopfn = btop( ciadap->phyaddr );
	     nptes > 0;
	     --nptes, ++pteptr, ++iopfn, vaddr += NBPG ) {
	    pteptr->pg_pfnum = iopfn;
	    Tbis( vaddr );
	}
	( void )tbsync();
	pccb->Lpstatus.mapped = 1;
	if( map_extent == MAP_FULL ) {
	    ciadap->isr = ciadap->mapped_isr;
	}
	switch( pccb->Interconnect ) {

	    case ICT_SBICMI:
		if( Bad_reg( pccb->Cnfr )) {
		    status = RET_FAILURE;
		}
		break;

	    case ICT_BI:
		if( Bad_reg( pccb->Bityp )) {
		    status = RET_FAILURE;
		}
		break;

	    case ICT_XMI:
		Lock_cidevice( pccb )
		if( Bad_reg( pccb->Xdev )) {
		    status = RET_FAILURE;
		}
		Unlock_cidevice( pccb )
		break;

	    default:
		( void )panic( PANIC_IC );
	}
    } else {
	( void )panic( PANIC_MAP );
    }
    return( status );
}

/*   Name:	ci_unmap_port	- Unmap Local CI Port
 *
 *   Abstract:	This routine unmaps a specified local CI port.  CI ports are
 *		unmapped only under the following circumstances:
 *
 *		1. Consecutive attempts exhausted without success during local
 *		   port initialization.
 *		2. Local port is determined to be broken.
 *		3. Temporary loss of local port power( CI750/CI780/CIBCI ).
 *
 *		The local port is permanently unmapped in the first two
 *		circumstances while it is only temporarily unmapped until power
 *		is restored in the last circumstance.
 *
 *		Unmapping local ports provides the means without impacting
 *		normal interrupt processing performance for handling unexpected
 *		interrupts which may occur while ports are without power or are
 *		permanently shutdown.  It also serves as a protective mechanism
 *		for those hardware port types( CI750/CI780 ) whose adapters are
 *		located within their own physically separate cabinets.  Such
 *		adapters are vulnerable to separately power failing or becoming
 *		uncabled from the busses on which they reside.  When either
 *		event occurs much( but not all ) of adapter I/O space becomes
 *		unaccessible.  Any access attempt results in a machine check.
 *		Unmapping the local port provides protection against these
 *		extraneous machine checks.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_bhole_pfn		- CI black hole mapping page page frame number
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   0
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access to the
 *		PCCB and provide exclusive access to the CIADAP vector element
 *		corresponding to the local CI port.
 */
void
ci_unmap_port( pccb )
    PCCB		*pccb;
{
    register CIADAP	*ciadap;
    register struct pte	*pteptr;
    register u_long	nptes;
    register u_char	*vaddr;

    /* Unmapping a local CI port includes:
     *
     * 1. Unmapping local port adapter I/O space system PTEs to the black hole
     *	  page, invalidating the system translation buffers for each page
     *	  unmapped.
     * 2. Changing the interrupt service handler for the local port to a
     *	  special routine( ci_unmapped_isr()) for fielding of all future
     *	  interrupts while the adapter I/O space is unmapped.
     *
     * A panic occurs if this routine is invoked to unmap an already unmapped
     * adapter.
     */
    if( pccb->Lpstatus.mapped ) {
	ciadap = pccb->Ciadap;
	for( pteptr = ciadap->iopte,
	     nptes = ciadap->npages,
	     vaddr = ciadap->viraddr;
	     nptes > 0;
	     --nptes, ++pteptr, vaddr += NBPG ) {
	    pteptr->pg_pfnum = ci_bhole_pfn;
	    Tbis( vaddr );
	}
	( void )tbsync();
	pccb->Lpstatus.mapped = 0;
	ciadap->isr = ci_unmapped_isr;
	pccb->Ciisr->isr = ci_unmapped_isr;
    } else {
	( void )panic( PANIC_UNMAP );
    }
}

/*   Name:	ci7b_disable	- Disable a Local CI7B Family Port
 *
 *   Abstract:	This routine completely disables a local CI7B family port(
 *		CI750/CI780/CIBCI ).  There are five occasions when this
 *		routine is invoked:
 *
 *		1. Prior to the initial initialization of a CI7B family port.
 *		2. During crashing of a CI7B family port.
 *		3. Following failure to initialize a CI7B family port.
 *		4. During disablement of a CI7B family port as part of system
 *		   shutdown.
 *		5. During processing of certain interrupts from CI7B family
 *		   ports by the special routine( ci_unmapped_isr()) used as an
 *		   interrupt service handler by unmapped local ports either
 *		   temporarily without power or marked broken and permanently
 *		   shutdown.
 *
 *		During port disablement various port registers are accessed.
 *		The accessibility of each register location is checked prior to
 *		accessing because the port is itself suspect whenever this
 *		routine is invoked.  Inability to access a location implies
 *		unavailability of the port itself( ie - brain damage ) and the
 *		specific access and all subsequent register accesses are
 *		bypassed.  A more extensive explanation of why and when
 *		register accesses require protection may be found at the front
 *		of this module.
 *
 *		NOTE: Local ports may be marked broken for many reasons other
 *		      than inability to access port or bus specific registers.
 *		      The disabling of such ports is crucial because they are
 *		      in the process of being permanently shutdown and may
 *		      still be currently active.  This is why disabling of a
 *		      local port is always attempted even when the port is
 *		      marked broken.  Only after a register access fails is the
 *		      local port regarded as truely dead, and inaccessible, and
 *		      all subsequent register accesses are bypassed.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *   state			- Port state
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.pmcsr	-   Port maintenance cntl & status reg pointer
 *	    ciregptrs.pdcr	-   Port disable control register pointer
 *	    ic.bi.bibci_ctrl	-   BIIC BCI control & status register pointer
 *	    ic.bi.biint_ctrl	-   BIIC User interrupt control reg pointer
 *	    ic.bi.biint_dst	-   BIIC Interrupt destination register pointer
 *	    lpstatus.mtimer	-   0
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   Port is broken status flag
 *
 *   SMP:	No locks are required.  However exclusive access to the PCCB
 *		must be guaranteed EXTERNALLY.  It may be guaranteed by locking
 *		the PCCB or by guaranteeing that only the processor executing
 *		this routine has access to it.
 */
void
ci7b_disable( pccb, state )
    register PCCB	*pccb;
    u_long		state;
{
    register u_long	status = 0;

    /* Disabling a local CI7B family port includes:
     *
     * - ENABLED PORTS ONLY:
     * 1. Disabling all interrupts on the port.
     * 2. Gracefully shutting down the port by transitioning it into the
     *	  Disabled port state.
     *
     * - ALL PORTS:
     * 3. Disabling the port by transitioning it into the Uninitialized port
     *	  state.
     * 4. Resetting bus specific register contents( CIBCI only ).
     * 5. Disabling the port maintenance timer.
     *
     * - BROKEN PORTS OR PORTS TEMPORARILY WITHOUT POWER ONLY:
     * 6. Unmapping the local port adapter I/O space system PTEs to the black
     *	  hole page, invalidating the system translation buffers for each page
     *    unmapped.
     * 7. Changing the interrupt service handler for the local port to a
     *	  special routine( ci_unmapped_isr()) for fielding of all future
     *	  interrupts while the adapter I/O space is unmapped.
     *
     * Gracefully shutting down a CI port( Step 2 ) aborts all port commands
     * currently undergoing processing.  It also triggers a port interrupt
     * following transitioning of the port into the disabled state.  As port
     * interrupts are currently disabled( Step 1 ), the expected interrupt
     * must be manually checked for.  The cause of any observed interrupt is
     * never ascertained, it is just assumed to be shutdown completion.
     * Shutdown is aborted whenever an interrupt is not detected within a
     * fixed period of time.
     *
     * Ports are disabled in a hardware port type specific fashion( Step 3 ).
     * CIBCIs are disabled by resetting the BI node and waiting up to a fixed
     * period of time for the node reset to complete and the BIIC's broke bit
     * to be cleared.  CI750 and CI780 ports are disabled by maintenance
     * initializing them.
     *
     * CIBCI port adapter BIIC register contents must be reset after disabling
     * the port( Step 4 ).  This is because executing a node reset( Step 3 ) to
     * disable the port clears their contents.
     *
     * The port maintenance timer must be explicitly disabled( Step 5 ) because
     * the act of disabling the port( Step 3 ) automatically triggers
     * activation of the timer.  This step must always be performed even if the
     * maintenance timer had been previously disabled.
     *
     * Steps 6-7 constitute unmapping of the local CI port and are executed by
     * ci_unmap_port().  These steps are only executed when either the local
     * port has temporarily lost power or is marked broken and has been or is
     * in the process of being permanently shutdown.
     *
     * Inability to access a bus specific or CI port register location at any
     * time( Steps 1-5 ) results in bypassing of all subsequent register
     * accesses and execution of the following actions:
     *
     * 1. The absence of the local port is logged.
     * 2. The local port is marked broken.
     * 3. The local port is permanently shut down.
     *
     * Event logging( Action 1 ) of the local port is bypassed whenever the
     * local port was already marked broken indicating that its absence is
     * already known of and was previously logged.  Shutdown of the local
     * port( Action 3 ) is also bypassed under such circumstances.  Local ports
     * marked broken have already been shut down.
     *
     * It is not necessary for this routine to directly shutdown the local
     * port( Action 3 ).  Marking the local port broken( Action 2 ) is
     * sufficient to force local port shutdown.  This is because an attempt to
     * initialize the local port always follows port disablement and local port
     * initialization is inherently designed to shutdown broken local ports.
     *
     * One occassion does exist when local port initialization does not follow
     * local port disablement, and shutdown of broken local ports never takes
     * place.  This is when local port disablement occurs as part of system
     * shutdown.  Failure to shutdown the local port in this case does not
     * present any problems because the entire system is being shutdown and no
     * need exists to separately shutdown one isolated part of it.
     */
    if( state == PS_ENAB ) {
	if( !Bad_reg( pccb->Pmcsr ) && !Bad_reg( pccb->Pdcr )) {
	    *pccb->Pmcsr &= ~CI7B_PMCS_MIE;
	    *pccb->Pdcr = PDCR_PDC;
	    Ci7b_wait_mif()
	} else {
	    status = FE_NOCI;
	}
    }
    if( status == 0 ) {
	switch( pccb->lpinfo.type.hwtype ) {

	    case HPT_CI750:
	    case HPT_CI780:
		if( !Bad_reg( pccb->Pmcsr )) {
		    *pccb->Pmcsr |= CI7B_PMCS_MIN;
		} else {
		    status = FE_NOCI;
		}
		break;

	    case HPT_CIBCI:
		if( !Bad_reg( pccb->Bictrl ) && bisst( pccb->Bictrl ) == 0 ) {
		    *pccb->Biint_ctrl = pccb->Ciadap->Biic_int_ctrl;
		    *pccb->Biint_dst = pccb->Ciadap->Biic_int_dst;
		    *pccb->Bibci_ctrl = ( BCI_STOPEN | BCI_UCSREN );
		} else {
		    status = FE_NOCI;
		}
		break;

	    default:
		( void )panic( PANIC_HPT );
	}
    }
    if( status == 0 ) {
	if( !Bad_reg( pccb->Pmcsr )) {
	    *pccb->Pmcsr |= PMCSR_MTD;
	    pccb->Lpstatus.mtimer = 0;
	} else {
	    status = FE_NOCI;
	}
    }
    if( status && !pccb->Fsmstatus.broken ) {
	pccb->Fsmstatus.broken = 1;
	( void )ci_log_dev_attn( pccb, status, LOG_REGS );
    }
    if( pccb->Fsmstatus.broken || !pccb->Lpstatus.power ) {
	( void )ci_unmap_port( pccb );
    }
}

/*   Name:	cibx_disable	- Disable a Local CIBX Family Port
 *
 *   Abstract:	This routine completely disables a local CIBX family port(
 *		CIBCA/CIXCD ).  There are five occasions when this routine is
 *		invoked:
 *
 *		1. Prior to the initial initialization of a CIBX family port.
 *		2. During crashing of a CIBX family port.
 *		3. Following failure to initialize a CIBX family port.
 *		4. During disablement of a CIBX family port as part of system
 *		   shutdown.
 *		5. During processing of interrupts from CIBX family ports by
 *		   the special routine( ci_unmapped_isr()) used as an interrupt
 *		   service handler by unmapped local ports marked broken and
 *		   permanently shutdown.
 *
 *		During port disablement various port registers are accessed.
 *		The accessibility of each register location is checked prior to
 *		accessing because the port is itself suspect whenever this
 *		routine is invoked.  Inability to access a location implies
 *		unavailability of the port itself( ie - brain damage ).  The
 *		specific access and all subsequent register accesses are
 *		bypassed.  A more extensive explanation of why and when
 *		register accesses require protection may be found at the front
 *		of this module.
 *
 *		NOTE: Local ports may be marked broken for many reasons other
 *		      than inability to access port or bus specific registers.
 *		      The disabling of such ports is crucial because they are
 *		      in the process of being permanently shutdown and may
 *		      still be currently active.  This is why disabling of a
 *		      local port is always attempted even when the port is
 *		      marked broken.  Only after a register access fails is the
 *		      local port regarded as truely dead, and inaccessible, and
 *		      all subsequent register accesses are bypassed.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus.mapped	-   1
 *   state			- Port state
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.pmcsr	-   Port maintenance cntl & status reg pointer
 *	    ciregptrs.pdcr	-   Port disable control register pointer
 *	    ic.bi.bibci_ctrl	-   BCI control and status register pointer
 *	    ic.bi.biint_ctrl	-   User interrupt control register pointer
 *	    ic.bi.biint_dst	-   Interrupt destination register pointer
 *	    ic.xmi.pidr		-   Port interrupt destination register pointer
 *	    ic.xmi.pvr		-   Port vector register pointer
 *	    lpstatus.mtimer	-   0
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   Port is broken status flag
 *
 *   SMP:	No locks are required.  However exclusive access to the PCCB
 *		must be guaranteed EXTERNALLY.  It may be guaranteed by locking
 *		the PCCB or by guaranteeing that only the processor executing
 *		this routine has access to it.
 */
void
cibx_disable( pccb, state )
    register PCCB	*pccb;
    u_long		state;
{
    register u_long	status = 0;

    /* Disabling a CIBX family port includes:
     *
     * - ENABLED PORTS ONLY:
     * 1. Disabling all interrupts on the port.
     * 2. Gracefully shutting down the port by transitioning it into the
     *	  Disabled port state.
     *
     * - ALL PORTS:
     * 3. Disabling the port by transitioning it into the Uninitialized port
     *	  state.
     * 4. Resetting bus specific register contents.
     * 5. Halting the port microsequencer.
     * 6. Disabling the port maintenance timer.
     *
     * - BROKEN PORTS ONLY:
     * 7. Unmapping the local port adapter I/O space system PTEs to the black
     *	  hole page, invalidating the system translation buffers for each page
     *    unmapped.
     * 8. Changing the interrupt service handler for the local port to a
     *	  special routine( ci_unmapped_isr()) for fielding of all future
     *	  interrupts while the adapter I/O space is unmapped.
     *
     * Gracefully shutting down a CI port( Step 2 ) aborts all port commands
     * currently undergoing processing.  It also triggers a port interrupt
     * following transitioning of the port into the disabled state.  As port
     * interrupts are currently disabled( Step 1 ), the expected interrupt
     * must be manually checked for.  The cause of any observed interrupt is
     * never ascertained, it is just assumed to be shutdown completion.
     * Shutdown is aborted whenever an interrupt is not detected within a
     * fixed period of time.
     *
     * CIBX family ports are disabled in a bus specific fashion( Step 3 ) by
     * resetting the node at which the port resides and waiting up to a fixed
     * period of time for the node reset to complete and the appropriate status
     * bit to be cleared.
     *
     * Bus specific register contents must be reset after disabling the port(
     * Step 4 ).  This is because executing a node reset( Step 3 ) to disable
     * the port clears their contents.
     *
     * The port microsequencer must be halted( Step 5 ) to allow access to
     * adapter control store because the act of resetting the node( Step 3 )
     * automatically started it.
     *
     * The port maintenance timer must be explicitly disabled( Step 6 ) because
     * the act of disabling the port( Step 3 ) automatically triggers
     * activation of the timer.  This step must always be performed even if the
     * maintenance timer had been previously disabled.
     *
     * Steps 7-8 constitute unmapping of the local CI port and are executed by
     * ci_unmap_port().  These steps are only executed when the local port is
     * marked broken and either has been or is in the process of being
     * permanently shutdown.
     *
     * Inability to access a bus specific or CI port register location at any
     * time( Steps 1-6 ) results in bypassing of all subsequent register
     * accesses and execution of the following actions:
     *
     * 1. The absence of the local port is logged.
     * 2. The local port is marked broken.
     * 3. The local port is permanently shut down.
     *
     * Event logging( Action 1 ) of the local port is bypassed whenever the
     * local port was already marked broken indicating that its absence is
     * already known of and was previously logged.  Shutdown of the local
     * port( Action 3 ) is also bypassed under such circumstances.  Local ports
     * marked broken have already been shut down.
     *
     * It is not necessary for this routine to directly shutdown the local
     * port( Action 3 ).  Marking the local port broken( Action 2 ) is
     * sufficient to force local port shutdown.  This is because an attempt to
     * initialize the local port always follows port disablement and local port
     * initialization is inherently designed to shutdown broken local ports.
     *
     * One occassion does exist when local port initialization does not follow
     * local port disablement, and shutdown of broken local ports never takes
     * place.  This is when local port disablement occurs as part of system
     * shutdown.  Failure to shutdown the local port in this case does not
     * present any problems because the entire system is being shutdown and no
     * need exists to separately shutdown one isolated part of it.
     */
    if( state == PS_ENAB ) {
	Lock_cidevice( pccb )
	if( !Bad_reg( pccb->Pmcsr ) &&
	     !Bad_reg( pccb->Pdcr ) &&
	     !Bad_reg( pccb->Psr )) {
	    Unlock_cidevice( pccb )
	    switch( pccb->lpinfo.type.hwtype ) {
	        case HPT_CIXCD:
	        case HPT_CIKMF:
		    DELAY( 1000 );
		    Lock_cidevice( pccb )
	            *pccb->Pmcsr &= ~PMCSR_MIE;
		    Unlock_cidevice( pccb )
		    DELAY( 1000 );
		    break;

	        case HPT_CIBCA_AA:
	        case HPT_CIBCA_BA:
	            *pccb->Pmcsr &= ~CIBX_PMCS_MIE;
		    break;
	    }
	    Lock_cidevice( pccb )
	    if( !( *pccb->Psr & CIBX_PS_UNIN )) {
	        Unlock_cidevice( pccb )
		DELAY( 1000 );
		Lock_cidevice( pccb )
	        *pccb->Pdcr = PDCR_PDC;
		Unlock_cidevice( pccb )
		DELAY( 1000 );
	        Cibx_wait_mif()
	    } else {
	        Unlock_cidevice( pccb )
	    }
	} else {
	    Unlock_cidevice( pccb )
	    status = FE_NOCI;
	}
    }
    if( status == 0 ) {
	switch( pccb->lpinfo.type.hwtype ) {

	    case HPT_CIBCA_AA:
	    case HPT_CIBCA_BA:
		if( !Bad_reg( pccb->Bictrl ) && bisst( pccb->Bictrl ) == 0 ) {
		    *pccb->Biint_ctrl = pccb->Ciadap->Biic_int_ctrl;
		    *pccb->Biint_dst = pccb->Ciadap->Biic_int_dst;
		    *pccb->Bibci_ctrl = ( BCI_STOPEN | BCI_UCSREN );
		} else {
		    status = FE_NOCI;
		}
		break;

	    case HPT_CIXCD:
		Lock_cidevice( pccb )
		if( !Bad_reg( pccb->Xbe )) {
		    Unlock_cidevice( pccb )
		    if( xmisst( pccb->Xdev ) == 0 ) {
		        status = FE_NOCI;
		    }
		} else { 
		    Unlock_cidevice( pccb )
		    status = FE_NOCI;
		}
	        break;

	    case HPT_CIKMF:
		if( pccb->Lpstatus.adapt && pccb->Ciadap->status.reset ) {
		    if( Bad_reg( pccb->Xbe ) || (( xmisst( pccb->Xdev )) == 0)) {
		        status = FE_NOCI;
		    }
		}
	        break;

	    default:
		( void )panic( PANIC_HPT );
	}
    }
    if( status == 0 ) {
	Lock_cidevice( pccb )
	if( !Bad_reg( pccb->Pmcsr )) {
	    Unlock_cidevice( pccb )
	    switch( pccb->lpinfo.type.hwtype ) {
	        case HPT_CIXCD:
	        case HPT_CIKMF:
		    DELAY( 1000 );
		    Lock_cidevice( pccb )
	            *pccb->Pmcsr =  PMCSR_MTD;
		    Unlock_cidevice( pccb )
		    DELAY( 1000 );
		    break;

	        case HPT_CIBCA_AA:
	        case HPT_CIBCA_BA:
	            *pccb->Pmcsr = ( CIBX_PMCS_HALT | PMCSR_MTD );
		    break;
	    }
	    pccb->Lpstatus.mtimer = 0;
	} else {
	    Unlock_cidevice( pccb )
	    status = FE_NOCI;
	}
    }
    if( status && !pccb->Fsmstatus.broken ) {
	pccb->Fsmstatus.broken = 1;
	( void )ci_log_dev_attn( pccb, status, LOG_REGS );
    }
    if( pccb->Fsmstatus.broken ) {
	( void )ci_unmap_port( pccb );
    }
}
