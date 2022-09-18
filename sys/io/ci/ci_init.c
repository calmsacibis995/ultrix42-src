#ifndef	lint
static char *sccsid = "@(#)ci_init.c	4.4	(ULTRIX)	12/20/90";
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
 *		initialization routines and functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	February 3, 1986
 *
 *   Function/Routines:
 *
 *   ci_init_port		Initialize a Local CI Port
 *   ci_probe			Probe a Local CI Port
 *   ci_setup_port		Prepare Local CI Port for Initialization
 *   ci_test_port		Test Operational Status of Local CI Port
 *   ci7b_load			Load CI7B Family Functional Microcode
 *   ci7b_start			Start a Local CI7B Family Port
 *   cibca_aa_load		Load CIBCA-AA Functional Microcode
 *   cibx_start			Start a Local CIBX Family Port
 *   cishc_start		Start a Local CISHC Family Port
 *
 *   Modification History:
 *
 *   19-Dec-1990	Pete Keilty
 *	1. Disable transaction timeouts for VAX_9000 systems
 *	   because on cpu errors the scan logic code can delay
 *	   XMI transactions.
 *	2. Changed pccb->Burst = cippd_max_port, poll all the nodes first
 *	   on start of ci polling. This allows us to find all systems
 *	   as soon as possible. After the first pass Burst is set to
 *	   ci_cippdburst.
 *
 *   16-Oct-1990	Pete Keilty
 *	Changed the smp_lock cidevice locking to use the new macros
 *	define in ciscs.h. Also added DELAY( 1000 ) around register
 *	writes because of the CIXCD XMOV bug, this for the 9000/6000
 *	systems.
 *
 *   16-Jul-1990	Pete Keilty
 * 	Add smp_lock lk_cidevice in the cibx_start routine for CIXCD,
 *	software workaround for XMOV hardware bug of back to back register
 *	accesses the first one a read the second a write to a software
 *	register.
 *
 *   06-Jun-1990	Pete Keilty
 *	1. Cleanup CIXCD code.
 *	2. Kmalloc CIADAP structure now and passed into ci_probe().
 *	   Added new structure CIISR define in ciadapter.h.
 *	3. Add preliminary support for CIKMF and SHAC.
 *
 *   08-Dec-1989	Pete Keilty
 *	1. TEMP -  do not check or set rev level for XCD.
 *	2. Use new macros Cibx_wait_unin() & Cibx_wait_pic_clear() in 
 *	   cibx_start() needed for CIXCD support.
 *	3. Set pidr & pvr in cibx_start() routine.
 *
 *   09-Nov-1989	David E. Eiche		DEE0080
 *	1. Rename all references to interconnects from IC_xxx to
 *	   ICT_xxx to match the new definitions in sysap.h.
 *	2. Add code to fill in the new software port type and
 *	   interconnect type fields in the LPIB.
 *
 *   19-Sep-1989	Pete Keilty
 *	1. Add XCD support, remove XCB.
 *	2. Add support for Explicit command addressing. CI/CIPORT ECO.
 *
 *   25-May-1989	Pete Keilty
 *	Add new macro WBFLUSH for mips cpu's.
 *
 *   24-May-89		darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's 
 *	Add include file ../io/scs/scamachmac.h, this file has machine
 *	depend items.
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h adn external lock_t struct
 *	lk_scadb
 *
 *   17-Jan-1989	Todd M. Katz		TMK0006
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Modify ci_probe() to longer initialize the formative path PB queue
 *	   listhead.  It is now initialized by the CI PPD.
 *	3. Include header file ../vaxmsi/msisysap.h.
 *	4. Use the ../machine link to refer to machine specific header files.
 *
 *   22-Aug-1988	Todd M. Katz		TMK0005
 *	1. Modify ci_probe() to switch the local CI port interrupt service
 *	   routine to ci_unmapped_isr() whenever it aborts local port probing
 *	   due to a fatal error.  All subsequent interrupts on the inoperative
 *	   local port are discarded.
 *	2. Modify ci_init_port() to mark the local port broken whenever its
 *	   initialization is permanently aborted due to exhaustion of all
 *	   consecutive port initialization attempts.
 *
 *   19-Aug-1988	Todd M. Katz		TMK0004
 *	 1. Cast all control blocks to ( char * ) before deallocating.
 *	 2. Initialize the new PCCB cell( pd.gvp.rrspq_remerr ) containing the
 *	    event code( SE_RRSPQ ) passed to ci_crash_lport() on failure to
 *	    obtain a local CI port response queue interlock.
 *	 3. The following Informational Events( ES_I ) have been defined as
 *	    Warning Events( ES_WE ): UCODE_WARN.
 *	 4. The following Informational Events( ES_I ) have been defined as
 *	    Error Events( ES_E ): UCODE_LOAD, UCODE_START.
 *	 5. The following Informational Events( ES_I ) have been defined as
 *	    Fatal Error Events( ES_FE ): INIT_NOMEM, INIT_ZEROID, INIT_MISMTCH,
 *	    INIT_NOUCODE, INIT_UNKHPT, NOCI, INIT_CPU.
 *	 6. The following CI local port crash codes are now defined as Severe
 *	    Error Events( ES_SE ): ICMDQ0, ICMDQ1, ICMDQ2, ICMDQ3, IDFREEQ,
 *	    IMFREEQ, RDFREEQ, RMFREEQ, RRSPQ; and Fatal Error Events( ES_FE ):
 *	    BADMAXPORT, BADUCODE.  The local port crash severity modifier(
 *	    ESM_LPC ) is applied by ci_crash_lport() but only when the crashed
 *	    local port is not in the process of being cleaned up from a
 *	    previous crash.
 *	 7. Modify ci_setup_port() to log an error( NOMEM ) instead of a fatal
 *	    error( INIT_NOMEM ) event whenever the functions fails to fully
 *	    allocate an initial number of free datagrams and messages.
 *	 8. Modify ci7b_load() and cibca_aa_load() to log a fatal error(
 *	    FE_UCODE_LOAD ) instead of an error( E_UCODE_LOAD ) event when the
 *	    current attempt to load functional microcode fails and the number
 *	    of consecutive re-initialization attempts left has been exhausted.
 *	 9. Modify ci7b_start() and cibx_start() to log a fatal error(
 *	    FE_UCODE_START ) instead of an error( E_UCODE_START ) event when
 *	    the current attempt to start the local CI port fails and the number
 *	    of consecutive re-initialization attempts left has been exhausted.
 *	10. Modify ci_setup_lport() to log additional information on CPU
 *	    microcode verification failures.
 *	11. Refer to error logging as event logging.
 *	12. Change the format of the permanent shutdown/offline messages.
 *	13. Rename FE_INIT_CPU -> FE_CPU.
 *	14. Modify ci_probe() to initialize the following CI PPD specific PCCB
 *	    fields using CI configuration variables ci_cippdburst and
 *	    ci_cippdcontact: burst( port polling burst size ), contact( port
 *	    polling contact frequency ).  Modify ci_init_port() to reset the
 *	    values of these fields following each successful re-initialization
 *	    of a local CI port.
 *	15. Modify ci_init_port() to event log ALL local CI port
 *	    initializations.  Also, change this routine so that all local port
 *	    permanent shutdown notifications are printed only on the console,
 *	    they are no longer also event logged.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *	 1. Create a single unified hierarchical set of naming conventions for
 *	    use within the CI port driver and describe them within ciport.h.
 *	    Apply these conventions to all names( routine, macro, constant, and
 *	    data structure ) used within the driver.  Restructure the driver to
 *	    segregate most CI family and port type specific code into separate
 *	    routines.  Such restructuring requires:
 *		1) Renaming ci_load_ucode() -> ci7b_load(), ci_start_port() ->
 *		   ci7b_start(), cibca_load() -> cibca_aa_load(), and
 *		   cibca_start() -> cibx_start().
 *		2) Initializing the new PCCB fields dg_cache, msg_cache,
 *		   start_port, disable_port, load_ucode, max_fn_level, mrltab,
 *		   and max_rom_level within ci_probe() according to the CI
 *		   family and hardware port type.
 *		3) Modifying ci_setup_port() to utilize dg_cache and msg_cache
 *		   when allocating free datagrams and messages to fill internal
 *		   local port free queue caches.
 *		4) Modifying ci_init_port() to utilize max_fn_level,
 *		   max_rom_level, and mrltab when verifying port microcode
 *		   revision levels.
 *		5) Modifying ci_init_port() to indirectly invoke appropriate CI
 *		   family/port specific functions for loading of functional
 *		   microcode and starting of local ports through the PCCB
 *		   instead of through a local variable as was previously done.
 *		6) Modifying ci_probe(), ci7b_load(), ci7b_start(),
 *		   cibca_aa_load(), and cibx_start() to indirectly invoke
 *		   appropriate CI family specific local CI port disablement
 *		   routines instead of directly invoking ci_disable_port() as
 *		   was previously done.
 *	 2. Add support for the CIXCB hardware port type by:
 *		1) Modifying ci_check_port() to check for the presence of
 *		   operational XMI based CI local ports.
 *		2) Modifying ci_init_port() to print out an appropriate local
 *		   port start up console message for this hardware port type.
 *		3) Modifying cibx_start() to retrieve functional and self-test
 *		   microcode revision levels for this hardware port type.
 *		4) Modifying ci_probe() to initialize the PCCBs of XMI based CI
 *		   ports and more specifically, CIXCB PCCBs.
 *		5) Adding include file ../vaxxmi/xmireg.h.
 *	 3. Modify the circumstances under which local ports are unmapped.
 *	    Unmapping is now done only when the local port adapter itself loses
 *	    power( CI750/CI780/CIBCI only ) or the port is marked broken and is
 *	    permanently shutdown.  Formerly, unmapping was done whenever a
 *	    local port was crashed, but before its re-initialization commenced;
 *	    and just immediately prior to its initial initialization.  This
 *	    change requires:
 *		1) Rename ci_check_port() -> ci_test_port().
 *		2) Appropriate modifications to the comments of routines
 *		   ci_init_port(), ci_setup_port(), ci_test_port(),
 *		   ci7b_load(), ci7b_start(), cibca_aa_load(), and
 *		   cibx_start().
 *		3) Validating the hardware port type within ci_test_port().
 *		4) Marking the local port broken within ci_init_port() and
 *		   explicitly unmapping it to permanently shut it down when
 *		   retries are exhausted.
 *		5) Unmapping the local port within ci_setup_port() when it is
 *		   marked broken and is to be permanently shutdown.
 *	 4. Add use of macros Cibx_onboard(), Ci7b_allram(), Ci7b_wait_mif(),
 *	    and Cibx_wait_mif().
 *	 5. Add hardware port type as an argument to ci_probe() and remove code
 *	    for determining it from the routine.
 *	 6. Modify all calls to ci_log_initerr() to reflect all changes to the
 *	    routine interface.
 *	 7. Move the 11/750 microcode revision level verification check from
 *	    ci_probe() to ci_setup_port().  This check is now executed each and
 *	    every time a local CI750 port is initialized instead of just once
 *	    when the port is originally probed.
 *	 8. Modify ci_setup_port() to flush all local port free queues prior
 *	    to returning a failure status.  Also, modify this routine to
 *	    deallocate all buffers it is unable to insert into appropriate
 *	    local port free queues because it finds the queues to be locked.
 *	 9. Resolve all problems associated with the retrieval of CI7B family
 *	    microcode revision levels.  Previously, this retrieval was not
 *	    being correctly done.
 *	10. Always verify both functional/ram and self-test/prom microcode
 *	    revision levels, regardless of the hardware port type.  Previously,
 *	    only functional/ram revision levels were being verified for CIBX
 *	    family ports.
 *	11. Eliminate the initialization of the BIIC error interrupt control
 *	    register within ci_probe().  The CI port driver never needs to make
 *	    use of this register because it never requests to be notified of
 *	    soft errors; therefore, there is no longer a need to initialize it.
 *	12. Streamline PCCB interconnect and hardware port type specific
 *	    initialization by ci_probe().
 *	13. Add include file ../vax/nexus.h.
 *	14. Eliminate the unnecessary use of the macro Get_reg() by ci7b_load()
 *	    and cibca_aa_load().  The accessibility of local CI port registers
 *	    has already been verified by the time these functions attempt to
 *	    load CI functional microcode.
 *	15. CI device attention event packets used to reserve space for fields
 *	    initialized only in the case of certain events.  This has changed.
 *	    All CI event packets are now tailered exactly to the event being
 *	    logged.  Therefore, there is no longer any need to initialize the
 *	    PCCB optional device attention information structure using the
 *	    macro Init_dattnopt()) since this structure is no longer blindly
 *	    copied into each and every device attention event log packet.
 *	    ci_setup_port(), ci7b_load(), ci7b_start(), cibca_aa_load(), and
 *	    cibx_start() have been changed appropriately.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   09-Apr-1988	Todd M. Katz		TMK0002
 *	1. Add support for the CIBCA-BA hardware port type and for onboard
 *	   CIBCA-BA port microcode.  Differentiate CIBCA-BA from CIBCA-AA
 *	   hardware ports when necessary; otherwise, refer to both types as
 *	   just CIBCA ports.
 *	2. Currently, the maximum( future ) supportable port station address is
 *	   223( CI_CSZ_224 - 1 ).  This is incorrect.  The correct maximum(
 *	   future ) supportable port station is actually 127( CI_CSZ_128 - 1 ).
 *	   Make the appropriate changes so as to treat 223 as an invalid port
 *	   station address.
 *	3. Compartmentalize the portion of ci_probe() which has more to do with
 *	   CI port driver initialization than probing of a specific CI port.
 *	   Execute this portion of ci_probe() only when the routine is invoked
 *	   for the very first time.
 *	4. Add use of Ctrl_from_num(), Pccb_fork(), and Pccb_fork_done() macros
 *	   in place of straight-line code.
 *	5. Move the portions of ci_start_port() and cibca_start() responsible
 *	   for loading port microcode into the new functions ci_load_ucode()
 *	   and cibca_load() respectively.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, restructured code paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/dyntypes.h"
#include		"../h/param.h"
#include		"../h/systm.h"
#include		"../h/time.h"
#include		"../h/kmalloc.h"
#include		"../h/vmmac.h"
#include		"../h/ksched.h"
#include		"../h/errlog.h"
#include		"../h/smp_lock.h"
#include		"../machine/pte.h"
#include		"../../machine/common/cpuconf.h"
#include		"../machine/cpu.h"
#ifdef vax
#include		"../machine/mtpr.h"
#endif vax
#include		"../machine/nexus.h"
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
extern	int		cpu;
extern	SCSIB		lscs;
extern	PDT		ci_pdt;
extern	struct pte	Sysmap[];
extern	CIISR		ci_isr[];
extern	GVPBDDB		*gvp_bddb;
extern	int		*ci_ucode;
extern	struct lock_t	lk_scadb;
extern	pccbq		scs_lport_db;
extern	u_char		*ci_black_hole;
extern	MRLTAB		ci7b_mrltable[], cibca_aa_mrltab[], cibca_ba_mrltab[],
			cixcd_mrltable[], cikmf_mrltable[], cishc_mrltable[];
extern	SCSH		*gvp_alloc_dg(), *gvp_alloc_msg();
extern	u_long		*ci780_regoff[], *cibci_regoff[], *cibca_regoff[],
			*cixcd_regoff[], *cikmf1_regoff[], *cikmf2_regoff[],
			*cishc_regoff[];
extern	u_short		ci_cippdburst, ci_cippdcontact, ci_first_port,
			ci_max_reinits, ci_maint_intrvl, ci_maint_timer,
			ci_ucode_type, cippd_max_port;
extern	void		ci_crash_lport(), ci_dealloc_pkt(), ci_init_port(),
			ci_log_dev_attn(), ci_log_initerr(), ci_notify_port(),
			ci_unmap_port(), ci_unmapped_isr(), ci7b_disable(),
			cibx_disable(), cishc_disable(),
			cippd_start(), gvp_dealloc_dg(),
			gvp_dealloc_msg();
extern	u_long		ci_bhole_pfn, ci_crctable[], gvp_max_bds,
			gvp_queue_retry, scs_dg_size, scs_msg_size,
			ci_setup_port(), ci_test_port(), ci7b_load(),
			ci7b_start(), cibca_aa_load(), cibx_start(),
			gvp_initialize(), scs_initialize();

/*   Name:	ci_init_port	- Initialize a Local CI Port
 *
 *   Abstract:	This routine directs the initialization of a local CI port.  It
 *		also oversees the port's initial initialization.  It is always
 *		invoked by forking to it.
 *
 *		The local port must be completely disabled whenever execution
 *		of this routine is scheduled including disablement of:
 *
 *		1. Port interrupts.
 *		2. Port boot/sanity timer.
 *		3. CI PPD finite state machine activity on the local port.
 *
 *		The local CI port must also be in the UNINITIALIZED port state.
 *
 *		This routine may also be scheduled to permanently shutdown
 *		local ports.  Such ports are always unmapped and marked broken
 *		in addition to having been disabled.  This distinguishes them
 *		from local ports to be initialized which are mapped and not
 *		marked broken.
 *
 *		NOTE: This is a mandatory PD function( Init_port ) for use by
 *		      the CI PPD.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_cippdburst		- CI PPD port polling burst size
 *   ci_cippdcontact		- CI PPD port polling contact frequency
 *   ci_first_port		- Port number of first local CI port
 *   ci_maint_intrvl		- CI port maintenance timer interval
 *   ci_maint_timer             - CI port maintenance timer enable flag
 *   ci_max_reinits		- CI max number consecutive reinitializations
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.cleanup	-   1
 *	    fsmstatus.fkip	-   1
 *	    fsmstatus.online	-   0
 *   lk_scadb			- SCA database lock structure
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_first_port		- Port number of first local CI port
 *   pccb			- Port Command and Control Block pinter
 *	lpinfo.addr		-  Local port station address
 *	lpinfo.nreinits		-  Number of local port re-initializations
 *	lpinfo.ppd.cippd	-  CI PPD specific local port information
 *	    max_port		-   Maximum hardware remote port number
 *	pd.gvp.pqb.type.ci	-  CI specific PQB fields
 *	    keepalive		-   Variable maintenance timer interval cell
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    devattn.cirevlev	-   Port microcode information
 *	    lbcrc		-   Loopback CRC
 *	    lbstatus		-   0
 *	    lpstatus		-   Local port status flags
 *	        init		-    First time initialization status flag
 *		connectivity	-    0
 *	    reinit_tries	-   Number consecutive re-initializations left
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    burst		-   Port polling burst size
 *	    contact		-   Port polling contact frequency
 *	    fsmstatus.broken	-   Port is broken status flag
 *	    fsmstatus.cleanup	-   0
 *	    fsmstatus.fkip	-   Fork operation in progress status flag
 *
 *   SMP:	The SCA database is locked for deletion of the PCCB following
 *		exhaustion of consecutive re-initialization attempts without a
 *		successful initial port initialization.
 *
 *		The PCCB is locked to synchronize access and as required by
 *		ci_log_dev_attn() in case event logging becomes necessary.
 *		PCCB locking is probably unnecessary because of lack of
 *		conflict for the PCCB due to the single threadedness of port
 *		clean up and initialization.  It is done anyway to guarantee
 *		unrestricted access and because the CI PPD interval timer may
 *		still be active.  PCCB addresses are always valid because these
 *		data structures are never deleted( except by this function on
 *		exhaustion of consecutive re-initialization attempts without a
 *		successful initial port initialization ).
 *
 *		Access to port queues is by means of memory interlocking
 *		queuing instructions.
 */
void
ci_init_port( pccb )
    register PCCB	*pccb;
{
    register MRLTAB	*mrltab;
    register gvpbq	*q, *qend;
    register u_long	status;
    register u_long 	save_ipl = Splscs();

    /* The steps involved in port initialization are as follows:
     *
     *  1. The PCCB is locked.
     *  2. Port clean up is marked completed.
     *  3. The existence of port power is verified( non-broken ports only ).
     *
     *  - PORT RE-INITIALIZATION ONLY:
     *  4. All port queues are flushed and all flushed packets are deallocated.
     *  5. The local port maintenance timer interval, CI PPD port polling burst
     *	   size, and CI PPD port polling contact frequency are reset.
     *  6. Local port loopback is enabled and loopback status is cleared.
     *
     *  - ALL INITIALIZATIONS:
     *  7. Presence of an operational port is verified.
     *	8. All configuration register errors are cleared( CI750, CI780, CIBCI
     *	   only ).
     *  9. The CPU microcode revision level is verified( 11/750 only ).
     * 10. An initial number of free datagrams and messages are allocated and
     *	   and placed on the appropriate port queues.
     * 11. Functional port microcode is loaded, read back, and verified( CI750,
     *	   CI780, CIBCI, and CIBCA-AA only ).
     * 12. Both port interrupts and the port itself are enabled.
     * 13. The port is notified of free message and datagram buffer
     *	   availability.
     *
     * - INITIAL PORT INITIALIZATION ONLY:
     * 14. The local port number is retrieved and stored as the port number of
     *	   the first local CI port encountered( when appropriate ).
     * 15. The local port loopback datagram CRC is computed.
     * 16. The maximum addressable port number is retrieved and verified.
     * 17. Microcode revision levels( ram/functional and prom/self-test ) are
     *	   verified.
     *
     *  - ALL INITIALIZATIONS:
     * 18. The initialization of the local CI port is logged.
     * 19. The local CI port maintenance timer is engaged provided CI port
     *	   maintenance timers have been optionally enabled.
     * 20. The PCCB is unlocked.
     * 21. The CI PPD is notified of successful port initialization.
     *
     * Local ports can not be initialized when they are without power.  This
     * presents no problems for those instances of power failure which involve
     * the entire system.  It also poses no problems for local CIBCA/CIXCD
     * ports which can not independently power failure.  Unfortunately,
     * potential problems do exist for power failure of local CI750/CI780/CIBCI
     * ports.  Such ports can power failure independently.  Worse yet, power
     * loss and gain on such ports can be discrete events separated in time.
     * This allows for the local port disablement and clean associated with the
     * loss of port power to complete and local port initialization to commence
     * before power is restored to the port.  It is also the reason why
     * verification of port power occurs( Step 3 ) before proceeding further
     * with port initialization, to insure that initialization is postponed
     * while local ports are without power.  For a more extensive discussion of
     * possible power failure recovery scenarios consult ci_crash_lport().
     *
     * All port queues are flushed immediately prior to port re-initialization(
     * Step 4 ) to insure their pristine state.  Logically this action should
     * occur as part of port clean up.  However, it is done immediately prior
     * to port re-initialization in order to be able to meet the following
     * requirements:
     *
     * 1. Port queues can not be flushed until all possibility of further
     *	  packet queuing to them has ceased.
     *
     * 2. Port queue flushing must involve determination of whether or not the
     *	  queues were left permanently locked by failure of the port( Discovery
     *    of a locked queue or any failure to obtain a queue interlock during
     *	  queue flushing results in zeroing of the offending queue and
     *	  permanent loss of all packets residing on it ).
     *
     * Neither of these requirements can be easily met during port clean up.
     * Nothing can prevent SYSAP or SCS motivated insertion of packets into
     * port queues without seriously affecting performance until all paths
     * associated with the failed port have been terminated.  This condition is
     * not reached until the virtual end of port clean up.  Furthermore, until
     * port re-initialization begins, multiple simultaneously active threads
     * may exist to complicate the determination of queues left permanently
     * locked by port failure.  It is only during re-initialization that a
     * single thread with access to port queues exists, the thread responsible
     * for the re-initialization itself.  Therefore, port queues are flushed
     * during port re-initialization instead of during port clean up mainly
     * because it is easier to meet these requirements and because it is just
     * safer and more straight forward to do so.
     *
     * Local port maintenance timer interval, CI PPD port polling burst size,
     * and CI PPD port polling contact frequency are all reset( Step 5 ) using
     * configuration variables.  This allows their values to change following
     * failure and re-initialization of a local port.  The values for CI PPD
     * port polling burst size and contact frequency actually change much more
     * frequently.  They are reset on a per local port basis following the
     * completion of each CI PPD polling sweep( by ci_test_lpconn()).
     *
     * An error in Steps 7,9 or exhaustion of all consecutive port
     * initialization attempts permanently aborts port initialization.  This is
     * also the fate of ports marked broken.
     *
     * Errors in Steps 10-12 abort the current port initialization attempt.  A
     * consecutive port initialization attempt is scheduled and the PCCB is
     * unlocked.
     *
     * Retrieval of the maximum addressable port number( Step 16 ) is dependent
     * upon the format of the CI port parameter register.  There are currently
     * two possible formats easily distinguishable by means of the diagnostic
     * information field.  One format implements this field while for the other
     * it is always 0.  Failure to retrieve a valid maximum addressable port
     * number crashes the local port and shuts it down permanently.
     *
     * Failure to verify the microcode revision level( Step 17 ) results either
     * in logging of an appropriate warning message or crashing of the local
     * port.  The former occurs when the revision level is known, just out of
     * date.  The port is allowed to continue to function.  The latter occurs
     * when the revision level is both unknown and does NOT exceed the known
     * maximum level for the hardware port type.  The local port is permanently
     * shut down.  No action is taken when an unknown level is found to exceed
     * the known maximum revision level for the hardware port type.  It is just
     * assumed that the port driver has not yet been updated to recognize a new
     * maximum microcode revision level.
     *
     * Permanent abortion of port initialization proceeds as follows:
     *
     * 1. The port is left disabled and unmapped.
     * 2. All associated resources are deallocated.
     * 3. The local port is marked broken.
     * 4. A console message is printed.
     * 5. The PCCB is removed from the system-wide local port database.
     * 6. The PCCB is unlocked.
     * 7. The PCCB is deallocated( only when the initial port initialization
     *    attempts fail ).
     *
     * The local port is marked broken( Step 3 ) only when it was previously
     * NOT marked broken.  It is also unmapped at this time.  This situation
     * exists only when all consecutive port initialization attempts have been
     * exhausted.
     *
     * The PCCB is deallocated( Step 7 ) only when the initial port
     * initialization attempts fails.  The SCA database is always locked prior
     * to PCCB removal and unlocked following it.  In order to preserve the SCA
     * locking hierarchy the PCCB lock is first released and then re-obtained
     * after the SCA database is locked.  Cached PCCB addresses remain valid at
     * this point because PCCBs are only deleted by this routine and such
     * action has not yet occurred.
     * 
     * NOTE: Broken local ports are unmapped at the time of their disablement.
     *	     This is not the case for those local ports which are to be
     *	     shutdown because they have exhausted their consecutive
     *	     initialization attempts.  Such ports are explicitly unmapped by
     *	     this routine during shutdown.
     */
    Lock_pccb( pccb )
    Pccb_fork_done( pccb, PANIC_PCCBFB )
    pccb->Fsmstatus.cleanup = 0;
    if( !pccb->Fsmstatus.broken && !pccb->Lpstatus.power ) {
	Unlock_pccb( pccb )
        ( void )splx( save_ipl );
	return;
    } else if( !pccb->Lpstatus.init     &&
		!pccb->Fsmstatus.broken &&
		pccb->Reinit_tries == ci_max_reinits ) {
	++pccb->lpinfo.nreinits;
	for( q = &pccb->Pqb.cmdq0, qend = &pccb->Pqb.rspq; q <= qend; ++q ) {
	    Flushq( pccb, q )
	}
	for( q = &pccb->Dfreeq, qend = &pccb->Mfreeq; q <= qend; ++q ) {
	    Flushq( pccb, q )
	}
	pccb->Pqb.Keepalive = ( u_long )ci_maint_intrvl;
	pccb->Burst = cippd_max_port;		/* poll all the nodes first */
	pccb->Contact = ci_cippdcontact;
	pccb->Lpstatus.connectivity = 0;
	*( u_char * )&pccb->Lbstatus = 0;
    }
    if( !pccb->Fsmstatus.broken					      &&
	 ( status = ci_setup_port( pccb )) == RET_SUCCESS	      &&
	 ( pccb->Load_ucode == NULL ||
	   ( status = ( *pccb->Load_ucode )( pccb )) == RET_SUCCESS ) &&
	 ( status = ( *pccb->Start_port )( pccb )) == RET_SUCCESS ) {
	status = 0;
	if( pccb->Lpstatus.init ) {
	    register u_long	ppr;
	    Lock_cidevice( pccb )
	    ppr = *pccb->Ppr;
	    Unlock_cidevice( pccb )

	    pccb->Lpstatus.init = 0;
	    pccb->Devattn.cirevlev.ci_romlev = pccb->Rom_level;
	    pccb->Devattn.cirevlev.ci_ramlev = pccb->Fn_level;
	    ( void )ci_log_dev_attn( pccb, I_LPORT_INIT, LOG_NOREGS );
	    if( ci_first_port == 0 ) {
		ci_first_port = Scaaddr_low( pccb->lpinfo.addr );
	    }    
	    Compute_lbcrc( pccb )
	    if( ppr & PPR_DIAG_INFO ) {
		switch( Cluster_size( ppr )) {
		    case 0:
		        pccb->lpinfo.Max_port = 
    				(( pccb->lpinfo.type.hwtype == HPT_CIKMF ) ?
		         	  CSZ_8 - 1 :  CSZ_16 - 1 ); 
			break;
		    case 1: pccb->lpinfo.Max_port = CSZ_32 - 1; break;
		    case 2: pccb->lpinfo.Max_port = CSZ_64 - 1; break;
		    case 3: pccb->lpinfo.Max_port = CSZ_128 - 1; break;
		    default: pccb->lpinfo.Max_port = 0; break;
		}
	    } else {
		if( Cluster_oldsize( ppr ) == 0 ) {
		    pccb->lpinfo.Max_port = CSZ_16 - 1;
		} else {
		    pccb->lpinfo.Max_port = 0;
		}
	    }
	    if( pccb->lpinfo.Max_port ) {
		if( pccb->lpinfo.Max_port > CIPPD_MAXPATHS ) {
		    pccb->lpinfo.Max_port = CIPPD_MAXPATHS;
		}
		for( mrltab = pccb->Mrltab; mrltab->ram; mrltab++ ) {
		    if( mrltab->ram == pccb->Fn_level &&
			 mrltab->rom == pccb->Rom_level ) {
			if( mrltab->warn == UCODE_WARNING ) {
			    ( void )ci_log_dev_attn( pccb,
						     W_UCODE_WARN,
						     LOG_NOREGS );
			}
			break;
		    }
		}
		if( mrltab->ram == 0 			  &&
		     pccb->Fn_level <= pccb->Max_fn_level &&
		     pccb->Rom_level <= pccb->Max_rom_level ) {
		    /* XCD&DASH temp */
    		    if(( pccb->lpinfo.type.hwtype == HPT_CIXCD ) ||
    		       ( pccb->lpinfo.type.hwtype == HPT_CIKMF )) {
		    } else {
		        status = FE_BADUCODE;
		    }
		}
	    } else {
		status = FE_BADMAXPORT;
	    }
	    if( status ) {
		pccb->Fsmstatus.broken = 1;
		( void )ci_crash_lport( pccb, status, NULL );
	    }
	} else {
	    ( void )ci_log_dev_attn( pccb, I_LPORT_REINIT, LOG_NOREGS );
	}
	if( status == 0 && ci_maint_timer ) {
	    ( void )ci_notify_port( pccb );
	}
	Unlock_pccb( pccb )
	if( status == 0 ) {
	    ( void )cippd_start( pccb );
	}
    } else if( !pccb->Fsmstatus.broken && --pccb->Reinit_tries ) {
	Pccb_fork( pccb, ci_init_port, PANIC_PCCBFB )
	Unlock_pccb( pccb )
    } else {
	for( q = &pccb->Dfreeq, qend = &pccb->Mfreeq; q <= qend; ++q ) {
	    Flushq( pccb, q )
	}
	if( !pccb->Fsmstatus.broken ) {
	    pccb->Fsmstatus.broken = 1;
	    ( void )ci_unmap_port( pccb );
	}
	Unlock_pccb( pccb )
	Lock_scadb()
	Lock_pccb( pccb )
	Remove_entry( pccb->flink )
	Unlock_pccb( pccb )
	Unlock_scadb()
	if( pccb->Lpstatus.init ) {
	    ( void )cprintf( "%4s\t- permanently offline( local port ? )\n",
			    ( u_char * )&pccb->lpinfo.name );
	    pccb->Ciadap->pccb = NULL;
	    if( pccb->Asb )
	        KM_FREE(( char * )pccb->Asb, KM_SCA )
	    KM_FREE(( char * )pccb, KM_SCA )
	} else {
	    cprintf( "%4s\t- permanently shutting down( local port %u )\n",
		    ( u_char * )&pccb->lpinfo.name,
		    Scaaddr_low( pccb->lpinfo.addr ));
	}
    }
    ( void )splx( save_ipl );
}

/*   Name:	ci_probe	- Probe a Local CI Port
 *
 *   Abstract:	This routine probes a newly discovered CI port culminating in
 *		its first initialization.  Any encountered errors abort the CI
 *		port probing.
 *
 *		The following CI hardware port types are currently supported:
 *
 *			CI750, CI780, CIBCI, CIBCA-AA, CIBCA-BA, CIXCD
 *
 *   Inputs:
 *
 *   0				- Interrupt processor level
 *   SBR			- System Page Table Address processor register
 *   SLR			- System Page Table Length processor register
 *   ci_adap			- Vector of CI Adapter Interface Blocks
 *   ci_black_hole		- CI black hole mapping page pointer
 *   ci_cippdburst		- CI PPD port polling burst size
 *   ci_cippdcontact		- CI PPD port polling contact frequency
 *   ci_maint_intrvl		- CI port maintenance timer interval
 *   ci_max_reinits		- CI max number consecutive reinitializations
 *   ci_pdt			- CI Port Dispatch Table
 *   ci_ucode_type		- CI functional microcode type
 *   ci780_regoff		- CI750/CI780 register offsets
 *   ci7b_mrltable		- CI750/CI780/CIBCI Microcode Revision Table
 *   cibca_aa_mrltab		- CIBCA-AA Functional Microcode Revision Table
 *   cibca_ba_mrltab		- CIBCA-BA Functional Microcode Revision Table
 *   cibca_regoff		- CIBCA register offsets
 *   cibci_regoff		- CIBCI register offsets
 *   cinum			- CI adapter number
 *   cixcd_mrltable		- CIXCD Functional Microcode Revision Table
 *   cixcd_regoff		- CIXCD register offsets
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *   gvp_max_bds		- Size of system-wide buffer descriptor table
 *   hpt			- Hardware port type
 *   interconnect		- Interconnect type
 *   ioaddr			- Adapter I/O space virtual address
 *   scs_dg_size		- Maximum application datagram size
 *   scs_lport_db		- System-wide local port database queue head
 *   scs_msg_size		- Maximum application message size
 *   Sysmap			- System Page Table
 *
 *   Outputs:
 *
 *   0				- Interrupt processor level
 *   ci_adap			- Vector of CI Adapter Interface Blocks
 *   ci_bhole_pfn		- CI black hole mapping page page frame number
 *   ci_black_hole		- CI black hole mapping page pointer
 *   ci_ucode_type		- CI functional microcode type
 *   pccb			- Port Command and Control Block pointer
 *				   ( INITIALIZED as required )
 *   scs_lport_db		- System-wide local port database queue
 *
 *   SMP:	No locks are required even though the PCCB and SCA database are
 *		manipulated.  This function is only called during system
 *		initialization and at that time only the processor executing
 *		this code is operational.  This guarantees uncompromised access
 *		to all data structures without locking the PCCB or the SCA
 *		database.
 *
 *		PCCB lock structure is initialized.
 */
void
ci_probe( cinum, ioaddr, interconnect, hpt, ciadap )
    u_long			cinum;
    register u_char		*ioaddr;
    u_long			interconnect;
    u_long			hpt;
    register CIADAP		*ciadap;
{
    register PCCB		*pccb;
    register u_long		status = 0;
    static u_long		initialized = 0;
    u_long			save_ipl = Splscs();
    register u_long		**regoffptr;

    /* The steps involved in probing a CI port are as follows:
     *
     *  1. IPL is synchronized to IPL_SCS.
     *  2. SCS is initialized.
     *  3. The Generic Vaxport Driver is initialized.
     *  4. A black hole mapping page is allocated.
     *  5. The CI functional microcode type is ascertained.
     *  6. A PCCB is allocated.
     *  7. Presence of CI functional microcode of the appropriate type is
     *	   verified( CI750, CI780, CIBCI, and CIBCA-AA only ).
     *  8. The PCCB is initialized.
     *  9. The CI port is disabled.
     * 10. The PCCB is inserted into the system-wide local port database.
     * 11. Initialization of the CI port is scheduled through forking.
     * 12. IPL is restored.
     *
     * Steps 2-5 constitute CI port driver initialization and are only executed
     * during probing of the very first local CI port encountered during device
     * auto-configuration.  The remaining steps constitute the actual probing
     * of the specified local CI port and are executed whenever this routine is
     * invoked.  Any errors( Steps 2-7 ) encountered during either portion of
     * this routine immediately abort probing of the current local CI port
     * following logging of the fatal event.  The interrupt service routine for
     * the local CI port is also switched to the ci_unmapped_isr(), the routine
     * responsible for handling interrupts on unmapped local CI ports.  Any
     * subsequent interrupts on the inoperative local port are discarded.
     *
     * NOTE: CIBCA-AA functional microcode is self identifying.  It possesses a
     *	     512 byte header which allows it to be uniquely identified as being
     *	     for a CIBCA-AA.  The other type of functional microcode, CI780
     *	     microcode for CI750/CI780/CIBCI ports, has no such header, and is
     *	     not self identifying, but can never be confused with CIBCA-AA
     *	     functional microcode.  Therefore, CI functional microcode identity
     *	     is ascertained( Step 6 ) by determining whether it is CIBCA_AA
     *	     microcode, and assuming it is CI780 functional microcode if this
     *	     determination fails.
     *
     * NOTE: Certain CI hardware port types( CIBCA-BA, CIXCD ) do not require
     *	     functional microcode to be loaded.  All needed microcode is
     *	     onboard.  Lack of onboard functional microcode in CI ports of
     *	     these hardware types is regarded as a fatal error.  The event is
     *	     logged and probing of the local CI port is aborted.
     *
     * NOTE: All PCCB pointers to adapter registers which are not used by a
     *	     local port because of its hardware port type are zeroed.  This
     *	     includes:
     *
     *		1. Pointers to the maintenance address and maintenance data
     *		   port registers( not required by CIBCA-BA and CIXCD local
     *		   ports because they possess onboard functional microcode ).
     *		2. Pointers to the configuration register( not required by
     *		   CIBCA and CIXCD local ports because they do not define it ).
     *
     * NOTE: The PCCB fork block is only used for clean up and initialization
     *	     of the local port.  Therefore, it should always be available for
     *	     the port's first initialization.
     */
    if( initialized == 0 ) {
	if(( status = scs_initialize()) != RET_SUCCESS ||
	   ( status = gvp_initialize()) != RET_SUCCESS ) {
	    status = ( status == RET_ALLOCFAIL )
				? FE_INIT_NOMEM : FE_INIT_ZEROID;
	} else {
	    KM_ALLOC( ci_black_hole, u_char *, NBPG, KM_SCA, KM_NOW_CL_CA )
	    if( ci_black_hole ) {
		status = 0;
		ci_bhole_pfn = btop((( u_long )ci_bhole_pfn ) & BHOLE_MASK );
		if( ci_ucode ) {
		    if( Cibca_aa_ucode( ci_ucode )) {
			ci_ucode_type = UCODE_CIBCA;
		    } else {
			ci_ucode_type = UCODE_CI780;
		    }
		}
	    } else {
		status = FE_INIT_NOMEM;
	    }
        }
	if( status == 0 ) {
	    initialized = 1;
	} else {
	    ( void )ci_log_initerr( cinum, interconnect, hpt, status );
	    ci_isr[ cinum ].isr = ci_unmapped_isr;
	    ( void )splx( save_ipl );
	    return;
	}
    }
    KM_ALLOC( pccb, PCCB *, sizeof( PCCB ), KM_SCA, KM_NOW_CL_CA )
    if( pccb ) {
	ci_isr[ cinum ].pccb = pccb;
	pccb->Ciadap = ciadap;
	pccb->Ciisr = &ci_isr[ cinum ];
    } else {
	( void )ci_log_initerr( cinum, interconnect, hpt, FE_INIT_NOMEM );
	ci_isr[ cinum ].isr = ci_unmapped_isr;
	( void )splx( save_ipl );
	return;
    }
    switch(( pccb->Interconnect = interconnect )) {

	case ICT_SBICMI:
	    if( hpt == HPT_CI780 || hpt == HPT_CI750 ) {
		if( ci_ucode ) {
		    if( ci_ucode_type == UCODE_CI780 ) {
			regoffptr = ci780_regoff;
			pccb->Dg_cache = CI7B_DG_CACHE;
			pccb->Msg_cache = CI7B_MSG_CACHE;
			pccb->Max_fn_level = CI7B_MAX_RAM;
			pccb->Max_rom_level = CI7B_MAX_ROM;
			pccb->Mrltab = ci7b_mrltable;
			pccb->Load_ucode = ci7b_load;
			pccb->Start_port = ci7b_start;
			pccb->Disable_port = ci7b_disable;
		    } else {
			status = FE_INIT_MISMTCH;
		    }
		} else {
		    status = FE_INIT_NOUCODE;
		}
	    } else {
		status = FE_INIT_UNKHPT;
	    }
	    break;

 	case ICT_BI: {
	    register struct biic_regs	*biic = ( struct biic_regs * )ioaddr;

	    pccb->Bityp = ( u_long * )&biic->biic_typ;
	    pccb->Bictrl = ( u_long * )&biic->biic_ctrl;
	    pccb->Bierr = ( u_long * )&biic->biic_err;
	    pccb->Biint_dst = ( u_long * )&biic->biic_int_dst;
	    pccb->Bibci_ctrl = ( u_long * )&biic->biic_bci_ctrl;
	    pccb->Biint_ctrl = ( u_long * )&biic->biic_int_ctrl;
	    switch( hpt ) {

		case HPT_CIBCI:
		    if( ci_ucode ) {
			if( ci_ucode_type == UCODE_CI780 ) {
			    regoffptr = cibci_regoff;
			    pccb->Dg_cache = CI7B_DG_CACHE;
			    pccb->Msg_cache = CI7B_MSG_CACHE;
			    pccb->Max_fn_level = CI7B_MAX_RAM;
			    pccb->Max_rom_level = CI7B_MAX_ROM;
			    pccb->Mrltab = ci7b_mrltable;
			    pccb->Load_ucode = ci7b_load;
			    pccb->Start_port = ci7b_start;
			    pccb->Disable_port = ci7b_disable;
			} else {
			    status = FE_INIT_MISMTCH;
			}
		    } else {
			status = FE_INIT_NOUCODE;
		    }
		    break;

		case HPT_CIBCA_AA:
		case HPT_CIBCA_BA:
		    regoffptr = cibca_regoff;
		    pccb->Dg_cache = CIBX_DG_CACHE;
		    pccb->Msg_cache = CIBX_MSG_CACHE;
		    pccb->Start_port = cibx_start;
		    pccb->Disable_port = cibx_disable;
		    if( hpt == HPT_CIBCA_BA ) {
			if( Cibx_onboard( biic->biic_typ )) {
			    pccb->Lpstatus.onboard = 1;
			    pccb->Max_fn_level = CIBCA_BA_MAXFN;
			    pccb->Max_rom_level = CIBCA_BA_MAXST;
			    pccb->Mrltab = cibca_ba_mrltab;
			} else {
			    status = FE_INIT_UNKHPT;
			}
		    } else {
			if( Cibx_onboard( biic->biic_typ ) == 0 ) {
			    if( ci_ucode ) {
				if( ci_ucode_type == UCODE_CIBCA ) {
				    pccb->Max_fn_level = CIBCA_AA_MAXFN;
				    pccb->Max_rom_level = CIBCA_AA_MAXST;
				    pccb->Mrltab = cibca_aa_mrltab;
				    pccb->Load_ucode = cibca_aa_load;
				} else {
				    status = FE_INIT_MISMTCH;
				}
			    } else {
				status = FE_INIT_NOUCODE;
			    }
			} else {
			    status = FE_INIT_UNKHPT;
			}
		    }
		    break;

		default:
		    status = FE_INIT_UNKHPT;
	    }
	    break;
	}

	case ICT_XMI: {
	    register struct xmi_reg	*xmireg = ( struct xmi_reg * )ioaddr;

	    switch( hpt ) {

		case HPT_CIXCD:
	    	    pccb->Xdev = ( u_long * )&xmireg->xmi_dtype;
	    	    pccb->Xbe = ( u_long * )&xmireg->xmi_xbe;
	    	    pccb->Xfadrl = ( u_long * )&xmireg->xmi_fadr;
	    	    pccb->Xfadrh = 
				( u_long * )(( u_long )xmireg + CIXCD_XFAER);
	    	    pccb->Xcd_pidr = 
				( u_long * )(( u_long )xmireg + CIXCD_PIDR );
	    	    pccb->Xcd_pvr = 
				( u_long * )(( u_long )xmireg + CIXCD_PVR );

		    regoffptr = cixcd_regoff;
		    pccb->Lpstatus.onboard = 1;
		    pccb->Dg_cache = CIBX_DG_CACHE;
		    pccb->Msg_cache = CIBX_MSG_CACHE;
		    pccb->Start_port = cibx_start;
		    pccb->Disable_port = cibx_disable;
		    pccb->Max_fn_level = CIXCD_MAXFN;
		    pccb->Max_rom_level = CIXCD_MAXST;
		    pccb->Mrltab = cixcd_mrltable;

		    break;

		case HPT_CIKMF:
		    if( !ciadap->status.pccb ) {
	    		pccb->Xdev = ( u_long * )&xmireg->xmi_dtype;
	    		pccb->Xbe = ( u_long * )&xmireg->xmi_xbe;
	    		pccb->Xfadrl = ( u_long * )&xmireg->xmi_fadr;
	    	        pccb->Xfadrh = 
				( u_long * )(( u_long )xmireg + CIKMF_XFAER);
	    	        pccb->Kmf_xpcctl = 
				( u_long * )(( u_long )xmireg + CIKMF_XPCCTRL );
	    	        pccb->Kmf_xpccsr = 
				( u_long * )(( u_long )xmireg + CIKMF_XPCCSR );
	    	        pccb->Kmf_pidr1 = 
				( u_long * )(( u_long )xmireg + CIKMF_IDEST1 );
	    	        pccb->Kmf_pidr2 = 
				( u_long * )(( u_long )xmireg + CIKMF_IDEST2 );
	    	        pccb->Kmf_pidr3 = 
				( u_long * )(( u_long )xmireg + CIKMF_IDEST3 );
	    	        pccb->Kmf_pvr1 = 
				( u_long * )(( u_long )xmireg + CIKMF_IVECT1 );
	    	        pccb->Kmf_pvr2 = 
				( u_long * )(( u_long )xmireg + CIKMF_IVECT2 );
	    	        pccb->Kmf_pvr3 = 
				( u_long * )(( u_long )xmireg + CIKMF_IVECT3 );
	    	        pccb->Kmf_piplr1 = 
				( u_long * )(( u_long )xmireg + CIKMF_IPL1 );
	    	        pccb->Kmf_piplr2 = 
				( u_long * )(( u_long )xmireg + CIKMF_IPL2 );
	    	        pccb->Kmf_piplr3 = 
				( u_long * )(( u_long )xmireg + CIKMF_IPL3 );
			ciadap->status.pccb = 1;
			pccb->Lpstatus.adapt = 1;
			regoffptr = cikmf1_regoff;
		    } else {
			regoffptr = cikmf2_regoff;
		    }
			pccb->Lpstatus.onboard = 1;
			pccb->Dg_cache = CIBX_DG_CACHE;
			pccb->Msg_cache = CIBX_MSG_CACHE;
			pccb->Start_port = cibx_start;
			pccb->Disable_port = cibx_disable;
			pccb->Max_fn_level = CIKMF_MAXFN;
			pccb->Max_rom_level = CIKMF_MAXST;
			pccb->Mrltab = cikmf_mrltable;

		    break;

		default:
		    status = FE_INIT_UNKHPT;
	    }
	    break;
	}
	case ICT_SHAC: {
	    switch( hpt ) {
		case HPT_SHAC:
/*
			regoffptr = cishc_regoff;
			pccb->Lpstatus.onboard = 1;
			pccb->Dg_cache = CIBX_DG_CACHE;
			pccb->Msg_cache = CIBX_MSG_CACHE;
			pccb->Start_port = cishc_start;
			pccb->Disable_port = cishc_disable;
			pccb->Max_fn_level = CISHC_MAXFN;
			pccb->Max_rom_level = CISHC_MAXST;
			pccb->Mrltab = cishc_mrltable;
*/
		    break;

		default:
		    status = FE_INIT_UNKHPT;
	    }
	    break;
        }
	default:
	    ( void )panic( PANIC_IC );
    }
    if( status ) {
	KM_FREE(( char * )pccb, KM_SCA )
	( void )ci_log_initerr( cinum, interconnect, hpt, status );
	ci_isr[ cinum ].pccb = NULL;
	ci_isr[ cinum ].isr = ci_unmapped_isr;
	( void )splx( save_ipl );
	return;
    }
    U_long( pccb->size ) = sizeof( PCCB );
    pccb->type = DYN_PCCB;
    pccb->pdt = &ci_pdt;
    Init_pccb_lock( pccb )
    Init_cidevice_lock( pccb )
    pccb->lpinfo.type.hwtype = hpt;
    pccb->lpinfo.type.swtype = SPT_CI;
    pccb->lpinfo.type.ictype = interconnect;
    pccb->lpinfo.type.dual_path = 1;
    pccb->lpinfo.name = Ctrl_from_num( "ci  ", cinum );
    {
    register u_long	**regptr, **end;
    for( regptr = ( u_long ** )&pccb->Ciregptrs,
	 end = ( u_long ** )( &pccb->Ciregptrs + 1 );
	 regptr != end;
	 ++regptr, ++regoffptr ) {
	*regptr = ( u_long * )(( u_long )ioaddr + U_long( *regoffptr ));
    }
    }
    switch( pccb->lpinfo.type.hwtype ) {

	case HPT_CIXCD:
            pccb->lpinfo.flags.expl = 1;
            pccb->lpinfo.Ovhd_pd = Gvph_size_exp;
	    pccb->Madr = 0;
	    pccb->Mdatr = 0;
	    pccb->Cnfr = 0;
	    break;

	case HPT_CIKMF:
            pccb->lpinfo.Ovhd_pd = Gvph_size_imp;
	    pccb->Cnfr = 0;
	    pccb->Pper = 0;
	    break;

	case HPT_SHAC:
	case HPT_CIBCA_BA:
	    pccb->Madr = 0;
	    pccb->Mdatr = 0;

	case HPT_CIBCA_AA:
	    pccb->Cnfr = 0;

	case HPT_CI750:
	case HPT_CI780:
	case HPT_CIBCI:
            pccb->lpinfo.Ovhd_pd = Gvph_size_imp;
	    pccb->Pper = 0;
	    break;

	default:
	    ( void )panic( PANIC_HPT );
    }
    pccb->Lpstatus.init = 1;
    pccb->Lpstatus.power = 1;
    pccb->Lpstatus.mapped = 1;
    pccb->Reinit_tries = ci_max_reinits;
    pccb->Pkt_size = ( pccb->lpinfo.Ovhd_pd + sizeof( SETCKTH ));
    if(( pccb->Pkt_mult = (( *pccb->Ppr & PPR_IBUF_LEN ) >> 25 )) > 0 ) {
       pccb->Pkt_mult--;
    }
    {
    register u_char	*c;
    register int	n;

    for( c = &pccb->Lbdata[ 0 ], n = 0; n < LBDSIZE; n++, c++ ) {
	*c = ( u_char )n;
    }
    }
    pccb->Pmaintq.header = &pccb->Pqb.cmdq0;
    pccb->Pmaintq.creg = pccb->Pcq0cr;
    pccb->Pmaintq.cmask = PCQ0CR_CMDQ0C;
    pccb->Pmaintq.error = SE_ICMDQ0;
    pccb->Pblockq.header = &pccb->Pqb.cmdq1;
    pccb->Pblockq.creg = pccb->Pcq1cr;
    pccb->Pblockq.cmask = PCQ1CR_CMDQ1C;
    pccb->Pblockq.error = SE_ICMDQ1;
    pccb->Pcommq.header = &pccb->Pqb.cmdq2;
    pccb->Pcommq.creg = pccb->Pcq2cr;
    pccb->Pcommq.cmask = PCQ2CR_CMDQ2C;
    pccb->Pcommq.error = SE_ICMDQ2;
    pccb->Pcontrolq.header = &pccb->Pqb.cmdq3;
    pccb->Pcontrolq.creg = pccb->Pcq3cr;
    pccb->Pcontrolq.cmask = PCQ3CR_CMDQ3C;
    pccb->Pcontrolq.error = SE_ICMDQ3;
    pccb->Pdfreeq.header = &pccb->Dfreeq;
    pccb->Pdfreeq.creg = pccb->Pdfqcr;
    pccb->Pdfreeq.cmask = PDFQCR_DFQC;
    pccb->Pdfreeq.error = SE_IDFREEQ;
    pccb->Pmfreeq.header = &pccb->Mfreeq;
    pccb->Pmfreeq.creg = pccb->Pmfqcr;
    pccb->Pmfreeq.cmask = PMFQCR_MFQC;
    pccb->Pmfreeq.error = SE_IMFREEQ;
    pccb->Rspq_remerr = SE_RRSPQ;
    pccb->Dfreeq_remerr = SE_RDFREEQ;
    pccb->Mfreeq_remerr = SE_RMFREEQ;
    pccb->Pqb.Dfreeq_hdr = CI_DFREEQ;
    pccb->Pqb.Mfreeq_hdr = CI_MFREEQ;
    pccb->Pqb.Dqe_len = pccb->lpinfo.Ovhd_pd +
			sizeof( GVPPPDH ) +
			sizeof( SCSH ) +
			scs_dg_size;
    pccb->Pqb.Mqe_len = pccb->lpinfo.Ovhd_pd +
			sizeof( GVPPPDH ) +
			sizeof( SCSH ) +
			scs_msg_size;
    pccb->Pqb.Keepalive = ( u_long )ci_maint_intrvl;
    pccb->Pqb.Vpqb_base = VPQB_BASE;
    pccb->Pqb.Bdt_base = BDT_BASE;
    pccb->Pqb.Bdt_len = gvp_max_bds;
    pccb->Pqb.Spt_base = SPT_BASE;
    pccb->Pqb.Spt_len = SPT_LEN;
    pccb->Pqb.Gpt_base = GPT_BASE;
    pccb->Pqb.Gpt_len = pccb->Pqb.Spt_len;
    pccb->Pqb.Func_mask = 0;
    {
    register GVPH	**lp, **ep;

    for( lp = ( GVPH ** )pccb->Pqb.Dqe_logout,
	 ep = ( GVPH ** )pccb->Pqb.Dqe_logout + ( 2 * CI_NLOG );
	 lp < ep;
	 ++lp ) {
	*lp = ( GVPH * )GVPH_FREE;
    }
    }

    Dm_pccbifISIS

    Init_queue( pccb->Form_pb )
    pccb->Burst = cippd_max_port;		/* poll all nodes first */
    pccb->Contact = ci_cippdcontact;
    pccb->Max_cables = MAX_CABLES;
    pccb->lpinfo.Dg_size = pccb->Pqb.Dqe_len;
    pccb->lpinfo.Msg_size = pccb->Pqb.Mqe_len;
    pccb->lpinfo.Ovhd =  pccb->lpinfo.Ovhd_pd + sizeof( GVPPPDH );
    pccb->lpinfo.Protocol = CIPPD_VERSION;
    ( void )( *pccb->Disable_port )( pccb, PS_UNINIT );
    Insert_entry( pccb->flink, scs_lport_db );
    Pccb_fork( pccb, ci_init_port, PANIC_PCCBFB )
    ( void )splx( save_ipl );
}

/*   Name:	ci_setup_port	- Prepare Local CI Port for Initialization
 *
 *   Abstract:	This function prepares local CI ports for initialization and is
 *		invoked only during local port initialization.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   SID			- System Identification processor register
 *   cpu			- CPU type code
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *          lpstatus.mapped     -   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pinter
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    devattn.cicpurevlev	-   Out-of-revision CPU microcode information
 *	    dfreeq		-   Port datagram free queue
 *	    mfreeq		-   Port message free queue
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   Port is broken status flag
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Local port successfully prepared
 *   RET_FAILURE		- CI adapter is absent & can't be initialized
 *   				- Insufficient memory for port initialization
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing unrestricted access.
 *		This is probably unnecessary because of lack of conflict for
 *		the PCCB due to the single threadedness of port clean up and
 *		initialization.  It is done anyway to guarantee unrestricted
 *		access and because the CI PPD interval timer may still be
 *		operational.
 *
 *		Access to port free queues is by means of memory interlocking
 *		queuing instructions.
 */
u_long
ci_setup_port( pccb )
    register PCCB	*pccb;
{
    register u_long	n;
    register SCSH	*scsbp;
    register gvpbq	*q, *qend;
    register u_long	status = 0;
    u_long		log_regs, sid = Sid;

    /* The steps involved in preparing a CI port for initialization are as
     * follows:
     *
     * 1. Presence of a local operational port is verified.
     * 2. All configuration register errors are cleared( CI750, CI780, CIBCI
     *	  only ).
     * 3. The CPU microcode revision level is verified( 11/750 only ).
     * 4. An initial number of free datagrams and messages, sufficient to fill
     *	  the port's internal cache plus 1, are allocated and inserted into the
     *    appropriate local port queues.
     *
     * Preparations for local port initialization are aborted on detection of
     * any errors.  Errors in Steps 1,3 permanently abort port initialization.
     * The fatal event is logged and the local port is taken permanently
     * offline by marking it broken and unmapping it.  Errors in Step 4 abort
     * only the current consecutive port initialization attempt.  The event is
     * logged and all local port free queues are flushed with all flushed
     * buffers being deallocated.
     *
     * VAX 11/750 CPU microcode revision levels must be checked prior to every
     * attempt at CI750 port initialization( Step 3 ).  This is because 11/750
     * writable control store is volatile and can be lost either on power
     * failure or other very serious errors.
     *
     * Allocation of messages and datagrams and their insertion into
     * appropriate local port free queues( Step 4 ) is bypassed whenever such
     * buffers had been previously allocated.  This can only occur when
     * preparation of the local port during a previous consecutive
     * initialization succeeded although all subsequent attempts at starting of
     * the port itself failed.
     *
     * NOTE: The port is not informed of the presence of free datagram and
     *	     message buffers even though the queues have been previously empty.
     *	     The port is disabled and therefore is in no condition to take
     *	     notice.  The port is notified of free queue entry availability
     *	     only after its successful enabling.
     */
    if( ci_test_port( pccb ) != RET_SUCCESS ) {
	status = FE_NOCI;
	log_regs = LOG_REGS;
	pccb->Fsmstatus.broken = 1;
    }
#ifdef vax
    else if( cpu == VAX_750 &&
	 (( struct cpu750 * )&sid )->cp_urev < MIN_VAX750_REV ) {
	status = FE_CPU;
	log_regs = LOG_NOREGS;
	pccb->Devattn.cicpurevlev.ci_hwtype = lscs.system.hwtype;
	pccb->Devattn.cicpurevlev.ci_mincpurev = MIN_VAX750_REV;
	pccb->Devattn.cicpurevlev.ci_currevlev
				= (( struct cpu750 * )&sid )->cp_urev;
	pccb->Fsmstatus.broken = 1;
    }
#endif vax
    else if( pccb->Dfreeq.flink == NULL ) {
	for( n = pccb->Dg_cache + 1; n; --n ) {
	    if(( scsbp = gvp_alloc_dg( pccb )) == NULL ||
	         insqti( Scs_to_pd( scsbp, pccb ), &pccb->Dfreeq, 0 ) > 0 ) {
		if( scsbp ) {
		    ( void )gvp_dealloc_dg( pccb, scsbp );
		}
		status = E_NOMEM;
		log_regs = LOG_NOREGS;
		break;
	    }
	}
	if( status == 0 ) {
	    for( n = pccb->Msg_cache + 1; n; --n ) {
		if(( scsbp = gvp_alloc_msg( pccb )) == NULL ||
		     insqti( Scs_to_pd( scsbp, pccb ), &pccb->Mfreeq, 0 ) > 0){
		    if( scsbp ) {
			( void )gvp_dealloc_msg( pccb, scsbp );
		    }
		    status = E_NOMEM;
		    log_regs = LOG_NOREGS;
		    break;
		}
	    }
	}
	if( status ) {
	    for( q = &pccb->Dfreeq, qend = &pccb->Mfreeq; q <= qend; ++q ) {
		Flushq( pccb, q )
	    }
	}
    }
    if( status == 0 ) {
	status = RET_SUCCESS;
    } else {
	( void )ci_log_dev_attn( pccb, status, log_regs );
	if( pccb->Fsmstatus.broken ) {
	    ( void )ci_unmap_port( pccb );
	}
	status = RET_FAILURE;
    }
    return( status );
}

/*   Name:	ci_test_port	- Test Operational Status of Local CI Port
 *
 *   Abstract:	This function checks for the presence of a local CI port and
 *		clears all bus specific errors which require clearing if the
 *		port is both present and operational.  It is invoked only
 *		during local port initialization.
 *
 *		CI780/CIBCA/CIXCD local ports are always present though they
 *		may be broken.  CI750/CIBCI local ports may completely
 *		disappear if the condition triggering failure, clean up, and
 *		re-initialization was sufficiently severe.  The most likely
 *		cause of such a disappearence would be the powering down or
 *		uncabling of a CI750/CIBCI in its adjacent cabinet.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cpu			- CPU type code
 *   cpusw			- CPU switch structure
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *          lpstatus.mapped     -   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.cnfr	-   Configuration register pointer
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Local port present and accessible
 *   RET_FAILURE		- Local port absent
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized( The PCCB is locked EXTERNALLY anyway ).
 */
u_long
ci_test_port( pccb )
    register PCCB	*pccb;
{
    register u_long	status = RET_SUCCESS;

    /* The local CI port is declared absent in the following cases:
     *
     * 1. CIBCI/CIBCA/CIXCD failed its self-test.
     * 2. CI750/CI780/CIBCI configuration register is unaccessible.
     * 3. CIBCI/CIBCA/CIXCD bus specific device type register is unaccessible.
     * 4. CI750/CIBCI port lacks power or has been uncabled in its separate
     *    cabinet.
     * 5. Verification of hardware port type fails.
     *
     * All errors are cleared from the CI750/CI780/CIBCI configuration.
     * register( Step 2 ).  The CIBCA/CIXCD do not define such a register and
     * so no clearing of errors is required.
     *
     * NOTE: There is never a need to clear errors in the BIIC/XMI error
     *	     register because resetting of the node during port disablement
     *	     accomplishes that task.
     */
    switch( pccb->lpinfo.type.hwtype ) {
#ifdef vax
	case HPT_CI750:
	case HPT_CI780:
	    if( Bad_reg( pccb->Cnfr )		  ||
		 ( *pccb->Cnfr & CI780_CNF_NOCI ) ||
		 ( *pccb->Cnfr & CI780_CNF_ADAP ) != NEX_CI ) {
		status = RET_FAILURE;
	    } else {
		*pccb->Cnfr = *pccb->Cnfr;
	    }
	    break;
#endif vax
	case HPT_CIBCI:
	    if( Bad_reg( pccb->Bityp )				      ||
		 ( *pccb->Bictrl & BICTRL_BROKE )		      ||
	         Bad_reg( pccb->Cnfr )				      ||
		 ( *pccb->Cnfr & ( CIBCI_CNF_NOCI | CIBCI_CNF_DCLO )) ||
		 ( *pccb->Bityp & BITYP_TYPE ) != BI_CIBCI ) {
		status = RET_FAILURE;
	    } else {
		*pccb->Cnfr = *pccb->Cnfr;
	    }
	    break;

	case HPT_CIBCA_AA:
	case HPT_CIBCA_BA:
	    if( Bad_reg( pccb->Bityp )			      ||
		 ( *pccb->Bictrl & BICTRL_BROKE )	      ||
		 ( *pccb->Bityp & BITYP_TYPE ) != BI_CIBCA    ||
		 ( pccb->lpinfo.type.hwtype == HPT_CIBCA_AA &&
			( *pccb->Bityp & CIBCA_DEV_BCABA ))   ||
		 ( pccb->lpinfo.type.hwtype == HPT_CIBCA_BA &&
			( *pccb->Bityp & CIBCA_DEV_BCABA ) == 0 )) {
		status = RET_FAILURE;
	    }
	    break;

	case HPT_CIXCD:
	    Lock_cidevice( pccb )
	    if( Bad_reg( pccb->Xdev )	  ||
		 ( *pccb->Xbe & XMI_STF ) ||
		 (( *pccb->Xdev & XMIDTYPE_TYPE ) != XMI_CIXCD )) {
		status = RET_FAILURE;
	    }
	    Unlock_cidevice( pccb )
	    break;

	case HPT_CIKMF:
	    if( Bad_reg( pccb->Xdev )	  ||
		 ( *pccb->Xbe & XMI_STF ) ||
		 (( *pccb->Xdev & XMIDTYPE_TYPE ) != XMI_CIKMF )) {
		status = RET_FAILURE;
	    }
	    break;

	default:
	    ( void )panic( PANIC_HPT );
    }
    return( status );
}

/*   Name:	ci7b_load	- Load CI7B Family Functional Microcode
 *
 *   Abstract:	This function loads CI7B family functional microcode during
 *		local CI750/CI780/CIBCI port initialization.
 *
 *		NOTE: As presently implemented this function is not suitable
 *		      for use on hardware platforms requiring matching field
 *		      alignments and access types( ie- only longword aligned
 *		      entities may be longword accessed ).  This is due to how
 *		      each 6 byte CI7B family microcode word is currently
 *		      accessed during microcode loading and verification.  This
 *		      can be fixed but it is not worth it because CI7B family
 *		      ports will NOT be supported on non-VAX hardware
 *		      platforms.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_ucode			- CI port functional microcode pointer
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *          lpstatus.mapped     -   1
 *          lpstatus.onboard    -   0
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.madr	-   Maintenance address register pointer
 *	    ciregptrs.mdatr	-   Maintenance data register pointer
 *	    devattn.ciucode	-   Faulty microcode information
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Port microcode successfully loaded
 *   RET_FAILURE		- CI port microcode did not pass verification
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access.  This
 *		is probably unnecessary because of lack of conflict for the
 *		PCCB due to the single threadedness of port clean up and
 *		initialization.  It is done anyway to guarantee unrestricted
 *		access and because the CI PPD interval timer may still be
 *		active.
 */
u_long
ci7b_load( pccb )
    register PCCB	*pccb;
{
    register u_char	*ucode;
    register u_long	csp, event, load_addr, status = RET_SUCCESS;

    /* The port's writable control store is loaded with the in-core image of
     * the CI7B functional microcode.  Proper loading is verified by reading
     * the loaded microcode out of the port's writable control store and
     * comparing it against the in-code microcode image.  Any comparison error
     * aborts the current consecutive attempt at port initialization.  The
     * event is logged and the port itself is disabled to flush out loaded
     * microcode.
     *
     * A panic occurs whenever an attempt is made to load microcode into a
     * local port with onboard functional microcode.
     *
     * NOTE: Where loading begins in the writable control store is dependent
     *	     upon whether the port contains both PROM and RAM( CI750, CI780,
     *	     and CIBCI ) or is RAM only.
     */
    if( pccb->Lpstatus.onboard ) {
	( void )panic( PANIC_ONBOARD );
    }
    load_addr = ( Ci7b_allram( ci_ucode ) ? CI7B_ARAM_LOAD : CI7B_RAM_LOAD );
    for( csp = load_addr, ucode = ( u_char * )ci_ucode;
	 csp < CI7B_WCS_SIZE ;
	 ++csp, ucode += CI7B_UCODEWDSZ ) {
	*pccb->Madr = csp;
	*pccb->Mdatr = U_long( *ucode );
	*pccb->Madr = ( csp | CI7B_SEL_CSHO );
	*pccb->Mdatr = U_short(*( ucode + 4 ));
    }
    for( csp = load_addr, ucode = ( u_char * )ci_ucode;
	 csp < CI7B_WCS_SIZE ;
	 ++csp, ucode += CI7B_UCODEWDSZ ) {
	*pccb->Madr = csp;
	if( *pccb->Mdatr != U_long( *ucode )) {
	    pccb->Devattn.ciucode.ci_addr = csp;
	    pccb->Devattn.ciucode.ci_gvalue = U_long( *ucode );
	    break;
	}
	*pccb->Madr = ( csp | CI7B_SEL_CSHO );
	if( *pccb->Mdatr != U_short(*( ucode + 4 ))) {
	    pccb->Devattn.ciucode.ci_addr = ( csp | CI7B_SEL_CSHO );
	    pccb->Devattn.ciucode.ci_gvalue = U_short(*( ucode + 4 ));
	    break;
	}
    }
    if( csp < CI7B_WCS_SIZE ) {
	event = (( pccb->Reinit_tries > 1 ) ? E_UCODE_LOAD : FE_UCODE_LOAD );
	pccb->Devattn.ciucode.ci_bvalue = *pccb->Mdatr;
	( void )ci_log_dev_attn( pccb, event, LOG_REGS );
	( void )( *pccb->Disable_port )( pccb, PS_UNINIT );
	status = RET_FAILURE;
    }
    return( status );
}

/*   Name:	ci7b_start	- Start a Local CI7B Family Port
 *
 *   Abstract:	This function starts a CI7B family port during local
 *		CI750/CI780/CIBCI port initialization.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *          lpstatus.mapped     -   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *	    fsmstatus.online	-   0
 *   Sysmap			- System Page Table
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.madr	-   Maintenance address register pointer
 *	    ciregptrs.mdatr	-   Maintenance data register pointer
 *	    ciregptrs.pdfqcr	-   Port dg free queue control register pointer
 *	    ciregptrs.pecr	-   Port enable control register pointer
 *	    ciregptrs.picr	-   Port initialization control reg pointer
 *	    ciregptrs.pmcsr	-   Port maintenance cntl & status reg pointer
 *	    ciregptrs.pmfqcr	-   Port message free queue control reg pointer
 *	    ciregptrs.pqbbase	-   PQB base register pointer
 *	    ciregptrs.psrcr	-   Port status release control reg pointer
 *	    ram_level		-   RAM revision level of port microcode
 *	    rom_level		-   PROM revision level of port microcode
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Local port successfully started
 *   RET_FAILURE		- CI port microcode did not pass verification
 *   				- CI port can not be started
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access.  This
 *		is probably unnecessary because of lack of conflict for the
 *		PCCB due to the single threadedness of port clean up and
 *		initialization.  It is done anyway to guarantee unrestricted
 *		access and because the CI PPD interval timer may still be
 *		active.
 */
u_long
ci7b_start( pccb )
    register PCCB	*pccb;
{
    register u_char	*ucode;
    register u_long	csp, event, load_addr, status = RET_SUCCESS;

    /* The steps involved in starting a CI7B family port are as follows:
     *
     * 1. Retrieve port RAM/PROM microcode revision levels( inital
     *	  initialization only ).
     * 2. Explicitly instruct the port is to start its functional microcode at
     *	  a specific address within its writable control store.
     * 3. Initialize the port by transitioning it into the disabled state.
     * 4. Load port PQB physical address into the appropriate CI port register.
     * 5. Release the port status register.
     * 6. Enable interrupts on the port.
     * 7. Enable the port by transitioning it into the enabled state.
     * 8. Notify the port of free message and datagram buffer availability.
     *
     * The MDATR port register may be read only when the port is in the
     * Uninitialized port state.  Therefore, RAM/PROM microcode revision levels
     * must be retrieved( Step 1 ) before transitioning the port into the
     * disabled state( Step 3 ).  Pre and post version 6 microcode revision
     * levels are stored in different port control store locations.  The
     * pre-version 6 control store locations are always 0 for post-version 6
     * microcode revision levels which allows for proper retrieval of revision
     * levels( Step 1 ) under all circumstances.
     *
     * Transitioning a CI port into the disabled state( Step 3 ) triggers a
     * port interrupt.  As port interrupts are currently disabled, the expected
     * interrupt must be manually checked for.  An error is declared if either
     * an interrupt is not detected within a fixed period of time or the port
     * fails to transition into the disabled state.  Such errors abort the
     * current consecutive attempt at port initialization.  The event is logged
     * and the port itself is disabled to flush out loaded microcode and return
     * the port to a known state.
     */
    if( pccb->Lpstatus.init ) {
	*pccb->Madr = CI7B_RAM_ADDR5;
	pccb->Fn_level = ( *pccb->Mdatr >> CI7B_REV_OFF );
	if( pccb->Fn_level ) {
	    *pccb->Madr = CI7B_ROM_ADDR5;
	} else {
	    *pccb->Madr = CI7B_RAM_ADDR6;
	    pccb->Fn_level = ( *pccb->Mdatr >> CI7B_REV_OFF );
	    *pccb->Madr = CI7B_ROM_ADDR6;
	}
	pccb->Rom_level = ( *pccb->Mdatr >> CI7B_REV_OFF );
	Scaaddr_low( pccb->lpinfo.addr ) = Imp_lport_addr( *pccb->Ppr );
    }
    *pccb->Pmcsr |= CI7B_PMCS_PSA;
    *pccb->Madr = CI7B_STARTADDR;
    *pccb->Picr = PICR_PIC;
    Ci7b_wait_mif()
    if(( *pccb->Pmcsr & CI7B_PMCS_MIF ) && ( *pccb->Psr & PSR_PIC )) {
	*pccb->Pqbbase = ( u_long )svtophy( &pccb->Pqb );
	*pccb->Psrcr = PSRCR_PSRC;
	*pccb->Pmcsr |= CI7B_PMCS_MIE;
	*pccb->Pecr = PECR_PEC;
	*pccb->Pdfqcr = PDFQCR_DFQC;
	*pccb->Pmfqcr = PMFQCR_MFQC;
    } else {
	event = (( pccb->Reinit_tries > 1 ) ? E_UCODE_START : FE_UCODE_START );
	( void )ci_log_dev_attn( pccb, event, LOG_REGS );
	( void )( *pccb->Disable_port )( pccb, PS_UNINIT );
	status = RET_FAILURE;
    }
    return( status );
}

/*   Name:	cibca_aa_load	- Load CIBCA-AA Functional Microcode
 *
 *   Abstract:	This function loads CIBCA-AA functional microcode during local
 *		CIBCA-AA port initialization.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_ucode			- CI port functional microcode pointer
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *          lpstatus.mapped     -   1
 *          lpstatus.onboard    -   0
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.madr	-   Maintenance address register pointer
 *	    ciregptrs.mdatr	-   Maintenance data register pointer
 *	    devattn.ciucode	-   Faulty microcode information
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Port microcode successfully loaded
 *   RET_FAILURE		- CI port microcode did not pass verification
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access.  This
 *		is probably unnecessary because of lack of conflict for the
 *		PCCB due to the single threadedness of port clean up and
 *		initialization.  It is done anyway to guarantee unrestricted
 *		access and because the CI PPD interval timer may still be
 *		active.
 */
u_long
cibca_aa_load( pccb )
    register PCCB	*pccb;
{
    register u_char	*ucode;
    register u_long	cs, cs_size, csaddr, section;
    u_long		event, offset, status = RET_SUCCESS;

    /* The port's writable control store is loaded with the in-core image of
     * the CIBCA-AA functional microcode.  Proper loading is verified by
     * reading the loaded microcode out of the port's writable control store
     * and comparing it against the in-code microcode image.  Any comparison
     * error aborts the current consecutive attempt at port initialization.
     * The event is logged and the port itself is disabled to flush out loaded
     * microcode.
     *
     * A panic occurs whenever an attempt is made to load functional microcode
     * into a local port with onboard functional microcode.
     */
    if( pccb->Lpstatus.onboard ) {
	( void )panic( PANIC_ONBOARD );
    }
    offset = (( CIBCA_UCODEH * )ci_ucode )->ucode_lbn_off * 512;
    for( cs_size = ( CIBCA_AA_CSSIZ * CIBCA_AA_NSECT ),
	 ucode = ( u_char * )ci_ucode + offset,
	 *pccb->Madr = CIBCA_AA_CSADDR;
	 cs_size > 0;
	 --cs_size, ucode += CIBCA_AA_UCWDSZ ) {
	*pccb->Mdatr = U_short( *ucode );
    }
    for( cs = CIBCA_AA_CSADDR,
	 cs_size = CIBCA_AA_CSSIZ,
	 ucode = ( u_char * )ci_ucode + offset;
	 cs_size > 0;
	 cs++, cs_size-- ) {
	for( csaddr = cs, section = 1;
	     section <= CIBCA_AA_NSECT;
	     section++, csaddr += CIBCA_AA_CSSECT, ucode += CIBCA_AA_UCWDSZ ) {
	    *pccb->Madr = csaddr;
	    if( *pccb->Mdatr != U_short( *ucode )) {
		event = (( pccb->Reinit_tries > 1 )
				? E_UCODE_LOAD : FE_UCODE_LOAD );
		pccb->Devattn.ciucode.ci_addr = csaddr;
		pccb->Devattn.ciucode.ci_gvalue = U_short( *ucode );
		pccb->Devattn.ciucode.ci_bvalue = *pccb->Mdatr;
		( void )ci_log_dev_attn( pccb, event, LOG_REGS );
		( void )( *pccb->Disable_port )( pccb, PS_UNINIT );
		status = RET_FAILURE;
		cs_size = 1;
		break;
	    }
	}
    }
    return( status );
}

/*   Name:	cibx_start	- Start a Local CIBX Family Port
 *
 *   Abstract:	This function starts a CIBX family port during local
 *		CIBCA/CIXCD port initialization.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *          lpstatus.mapped     -   1
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   0
 *	    fsmstatus.online	-   0
 *   Sysmap			- System Page Table
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.pdfqcr	-   Port dg free queue control register pointer
 *	    ciregptrs.pecr	-   Port enable control register pointer
 *	    ciregptrs.picr	-   Port initialization control reg pointer
 *	    ciregptrs.pmcsr	-   Port maintenance cntl & status reg pointer
 *	    ciregptrs.pmfqcr	-   Port message free queue control reg pointer
 *	    ciregptrs.pqbbase	-   PQB base register pointer
 *	    ciregptrs.psrcr	-   Port status release control reg pointer
 *	    ram_level		-   Functional microcode revision level
 *	    rom_level		-   Self-test microcode revision level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Local port successfully started
 *   RET_FAILURE		- CI port microcode did not pass verification
 *   				- CI port can not be started
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access.  This
 *		is probably unnecessary because of lack of conflict for the
 *		PCCB due to the single threadedness of port clean up and
 *		initialization.  It is done anyway to guarantee unrestricted
 *		access and because the CI PPD interval timer may still be
 *		active.
 */
u_long
cibx_start( pccb )
    register PCCB	*pccb;
{
    register u_long	event, status = RET_SUCCESS;

    /* The steps involved in starting a CIBX family port are as follows:
     *
     * 1. Start the port micro-engine sequencing.
     * 2. Disable port interrupts and the port maintenance timer.
     * 3. Initialize the port by transitioned it into the disabled state.
     * 4. Retrieve port functional/self-test microcode revision levels( initial
     *	  initialization only ).
     * 5. Load port PQB physical address into the appropriate CI port register.
     * 6. Release the port status register.
     * 7. Enable interrupts on the port.
     * 8. Enable the port by transitioning it into the enabled state.
     * 9. Notify the port of free message and datagram buffer availability.
     *
     * Starting the port micro-engine sequencing( Step 1 ) clears all port
     * errors.  Unfortunately, it also enables both port interrupts and the
     * port maintenance timer requiring their explicit disablement( Step 2 ).
     *
     * Transitioning a CI port into the disabled state( Step 3 ) triggers a
     * port interrupt.  As port interrupts are currently disabled, the expected
     * interrupt must be manually checked for.  An error is declared if either
     * an interrupt is not detected within a fixed period of time or the port
     * fails to transition into the disabled state.  Such errors abort the
     * current consecutive attempt at port initialization.  The event is logged
     * and the port itself is disabled to flush out any loaded microcode and to
     * return the port to a known state.
     */
    DELAY( 1000 );
    Lock_cidevice( pccb )
    *pccb->Pmcsr |= PMCSR_MIN; /* start and min are 0x1 for BCA & XCD */
    Unlock_cidevice( pccb )
    DELAY( 1000 );
    WBFLUSH
    Cibx_wait_unin()
    Lock_cidevice( pccb )
    if( *pccb->Psr & CIBX_PS_UNIN ) {
        Unlock_cidevice( pccb )
	switch( pccb->lpinfo.type.hwtype ) {
	    case HPT_CIXCD:
		DELAY( 1000 );
		Lock_cidevice( pccb )
                *pccb->Pmcsr &= ~PMCSR_MIE;
		Unlock_cidevice( pccb )
		DELAY( 1000 );
		Lock_cidevice( pccb )
	        *pccb->Xcd_pidr = pccb->Ciadap->Xcd_pid;
		Unlock_cidevice( pccb )
		DELAY( 1000 );
		Lock_cidevice( pccb )
	        *pccb->Xcd_pvr = pccb->Ciadap->Xcd_pv;
		Unlock_cidevice( pccb )
		DELAY( 1000 );
		break;

	    case HPT_CIKMF:
                *pccb->Pmcsr &= ~PMCSR_MIE;
	        *pccb->Kmf_piplr1 = pccb->Ciadap->Kmf_pipl1;
	        *pccb->Kmf_piplr2 = pccb->Ciadap->Kmf_pipl2;
	        *pccb->Kmf_piplr3 = pccb->Ciadap->Kmf_pipl3;
	        *pccb->Kmf_pidr1 = pccb->Ciadap->Kmf_pid1;
	        *pccb->Kmf_pidr2 = pccb->Ciadap->Kmf_pid2;
	        *pccb->Kmf_pidr3 = pccb->Ciadap->Kmf_pid3;
	        *pccb->Kmf_pvr1 = pccb->Ciadap->Kmf_pv1;
	        *pccb->Kmf_pvr2 = pccb->Ciadap->Kmf_pv2;
	        *pccb->Kmf_pvr3 = pccb->Ciadap->Kmf_pv3;
		break;

	    case HPT_CIBCA_AA:
	    case HPT_CIBCA_BA:
                *pccb->Pmcsr &= ~CIBX_PMCS_MIE;
		break;

        }
        WBFLUSH
	DELAY( 1000 );
	Lock_cidevice( pccb )
        *pccb->Pmcsr |= PMCSR_MTD;
	Unlock_cidevice( pccb )
	DELAY( 1000 );
        WBFLUSH
	Lock_cidevice( pccb )
        *pccb->Picr = PICR_PIC;
	Unlock_cidevice( pccb )
	DELAY( 1000 );
        WBFLUSH
        Cibx_wait_mif()
	Lock_cidevice( pccb )
        if(( *pccb->Psr & CIBX_PS_MIF ) && ( *pccb->Psr & PSR_PIC )) {
	    Unlock_cidevice( pccb )
	    if( pccb->Lpstatus.init ) {
	        switch( pccb->lpinfo.type.hwtype ) {

		    case HPT_CIBCA_AA:
		    case HPT_CIBCA_BA:
		        pccb->Fn_level = Cibca_fn_level( pccb );
		        pccb->St_level = Cibca_st_level( pccb );
	                Scaaddr_low( pccb->lpinfo.addr ) = 
				 Imp_lport_addr( *pccb->Ppr );
		        break;

		    case HPT_CIKMF:
	                Scaaddr_low( pccb->lpinfo.addr ) = 
				 Imp_lport_addr( *pccb->Ppr );
		        break;

		    case HPT_CIXCD:
/* XCD temp
		    pccb->Fn_level = Cixcd_fn_level( pccb );
		    pccb->St_level = Cixcd_st_level( pccb );
*/
	 	        pccb->Pqb.Asb_base = 0;
	                pccb->Pqb.Asb_len = 0;
/* IF XCD ever does ASB in ppe2r
	                pccb->Pqb.Asb_len = Ppe2r_asblen( *pccb->Ppe2r );
*/
		        if( !pccb->Asb && ( pccb->Pqb.Asb_len != 0 )) {
	                    KM_ALLOC( pccb->Asb, u_char *, 
			          Asblen_bytes( pccb->Pqb.Asb_len ),
			          KM_SCA, KM_NOW_CL_CA )
	                    if( pccb->Asb ) {
	 	                pccb->Pqb.Asb_base = 
			          ( u_long )Svtophy_shift( pccb->Asb );
	                    } else {
	                    /* log */
	 	                pccb->Pqb.Asb_base = 0;
	 	                pccb->Pqb.Asb_len = 0;
	                    }
		        }
	    		Lock_cidevice( pccb )
	                Scaaddr_low( pccb->lpinfo.addr ) = 
				Imp_lport_addr( *pccb->Ppr );
	    		Unlock_cidevice( pccb )
/* Not until full subnode address in the driver code !!
			Exp_lport_addr( *pccb->Ppr, *pccb->Pper );
*/
		        break;

		    default:
		        ( void )panic( PANIC_HPT );
	        }
	    }
	    DELAY( 1000 );
	    Lock_cidevice( pccb )
	    *pccb->Psrcr = PSRCR_PSRC;
	    Unlock_cidevice( pccb )
	    DELAY( 1000 );
	    WBFLUSH
	    switch( pccb->lpinfo.type.hwtype ) {
		case HPT_CIKMF:
		    *pccb->Kmf_xpccsr = 0xB;
		case HPT_CIXCD:
		    /* Disable transaction timeouts for VAX_9000 systems
		       because on cpu errors the scan logic code can delay
		       XMI transactions */
		    if( cpu == VAX_9000 ) {
		        Lock_cidevice( pccb )
		        *pccb->Xbe = CIXCD_XBE_TTO;
		        Unlock_cidevice( pccb )
		        DELAY( 1000 );
		    }
		    Lock_cidevice( pccb )
	            *pccb->Pqbbase = ( u_long )Svtophy_shift( &pccb->Pqb );
		    Unlock_cidevice( pccb )
		    DELAY( 1000 );
		    Lock_cidevice( pccb )
                    *pccb->Pmcsr |= PMCSR_MIE;
		    Unlock_cidevice( pccb )
		    DELAY( 1000 );
		    break;

		case HPT_CIBCA_AA:
		case HPT_CIBCA_BA:
	            *pccb->Pqbbase = ( u_long )svtophy( &pccb->Pqb );
                    *pccb->Pmcsr |= CIBX_PMCS_MIE;
		    break;
            }
	    WBFLUSH
	    DELAY( 1000 );
	    Lock_cidevice( pccb )
	    *pccb->Pecr = PECR_PEC;
	    Unlock_cidevice( pccb )
	    DELAY( 1000 );
	    WBFLUSH
            Cibx_wait_pic_clear()
	    Lock_cidevice( pccb )
            if( !( *pccb->Psr & PSR_PIC ) ) {
	        Unlock_cidevice( pccb )
		DELAY( 1000 );
		Lock_cidevice( pccb )
	        *pccb->Pdfqcr = PDFQCR_DFQC;
		Unlock_cidevice( pccb )
		DELAY( 1000 );
		Lock_cidevice( pccb )
	        *pccb->Pmfqcr = PMFQCR_MFQC;
		Unlock_cidevice( pccb )
		DELAY( 1000 );
	        WBFLUSH
            } else {
	        Unlock_cidevice( pccb )
	        status = RET_FAILURE;
            }
        } else {
	    Unlock_cidevice( pccb )
	    status = RET_FAILURE;
        }
    } else {
	Unlock_cidevice( pccb )
        status = RET_FAILURE;
    }
    if( status == RET_FAILURE ) {
	event = (( pccb->Reinit_tries > 1 ) ? E_UCODE_START : FE_UCODE_START );
	( void )ci_log_dev_attn( pccb, event, LOG_REGS );
	( void )( *pccb->Disable_port )( pccb, PS_UNINIT );
    }
    return( status );
}
