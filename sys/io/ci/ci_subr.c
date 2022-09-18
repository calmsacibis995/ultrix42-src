#ifndef	lint
static char *sccsid = "@(#)ci_subr.c	4.5	(ULTRIX)	3/7/91";
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
 *		functions provided for use by the CI PPD finite state machine
 *		in the establishment, maintenance, and termination of paths.
 *		Other CI functions( ci_init_port() and ci_log_badport() )
 *		required by the CI PPD are located in other modules.
 *
 *		This module also contains miscellaneous CI functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	November 6, 1985
 *
 *   Functions/Routines:
 *
 *   Functions and Routines Required by the CI PPD Finite State Machine
 *	ci_alloc_buf		Allocate Emergency CI Command Packets
 *   	ci_dealloc_buf		Deallocate Emergency CI Command Packets
 *	ci_get_port		Retrieve Port Number from Buffer
 *   	ci_init_pb		Initialize a Path Block
 *	ci_inv_cache		Invalidate CI Port's Translation Cache
 *	ci_send_reqid		Request Remote Port Identification
 *   	ci_set_circuit		Set Virtual Circuit State to On/Off
 *   	ci_notify_port		Notify CI Port of Continued CI PPD Activity
 *	ci_test_lpconn		Test Local CI Port Connectivity
 *	ci_update_ptype		Update Hardware Port Type of Remote Port
 *    CI Port Driver Miscellaneous Functions
 *	ci_alloc_pkt		Allocate CI Port Command Packet
 *	ci_dealloc_pkt		Deallocate CI Port Command Packet
 *	ci_update_cable		Update Cable Status of Path Block
 *
 *   Modification History:
 *
 *   07-Mar-1991	Pete Keilty
 *	Moved REQID back to the communication queue priority 2 
 *	because under heavy I/O loads reqid's would not be serviced
 *	in time on the maintence queue priority 0 causing cippd
 *	sanity timeouts and port crashes.
 *
 *   19-Dec-1990	Pete Keilty
 *	Modified ci_set_circuit for CIXCD support of NADP bit.
 *
 *   16-Oct-1990	Pete Keilty
 *	1. Changed smp locking to used new macros Lock_cidevice and
 *	   Unlock_cidevice define in ciscs.h.
 *	2. Added DELAY( 1000 ) around register write again because of
 *	   the CIXCD XMOV bug, this was needed on 9000/6000 syatems.
 *
 *   16-Jul-1990	Pete Keilty
 * 	Add smp_lock lk_cidevice in the ci_notify_port routine for CIXCD,
 *	software workaround for XMOV hardware bug of back to back register
 *	accesses the first one a read the second a write to a software
 *	register.
 *
 *   06-Jun-1990	Pete Keilty
 *	1. Added preliminary support for CIKMF( dash ).
 *	2. Added packet multiple size detection from ID packet
 *	   will be used with 4K pkt eco.
 *	3. REQID & SNDLB now go on the maintence queue0.
 *	4. Modified ci_set_circuit for CIXCD.
 *
 *   08-Dec-1989	Pete Keilty
 *	1. Use new macro Get_pgrp() to get port node number for now
 *	   untill full subenode addressing is added.
 *	2. Switch off of hardware type in ci_set_circuit() routine.
 *
 *   19-Sep-1989	Pete Keilty
 *	1. Add pccb to Format_gvph, Pd_to_ppd, & Ppd_to_pd.
 *	2. Change ci_get_port & ci_set_circuit check for explicit
 *	   address format and used it.
 *
 *   23-Aug-1989	Pete Keilty
 * 	Change ci_update_ptype checks to use bcmp to prevent mips cpu
 *	alignment errors.
 *
 *   25-May-1989	Pete Keilty
 *	Add Dm_msg_dg double mapping macro for mips cpu ISIS.
 *	Add new include file scamachmac.h
 *
 *   17-Jan-1989	Todd M. Katz		TMK0004
 *	1. Modify the interface to ci_get_port to include the PCCB address.
 *	2. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	3. Some hardware platforms require field alignments and access types
 *	   to match( ie- only longword aligned entities may be longword
 *	   accessed ).  All fields which need to be and are feasible to be
 *	   longword aligned have been so aligned.  One field which can not be
 *	   aligned as such is the crc field within the SNDLB packet.  It can
 *	   not be longword aligned because its position is mandated by the CI
 *	   protocol.  Therefore, instead of attempting to align this field,
 *	   change how it is accessed.  It is now accessed "generically" instead
 *	   of as a longword.
 *	4. Include header file ../vaxmsi/msisysap.h.
 *
 *   19-Aug-1988	Todd M. Katz		TMK0003
 *	1. Cast all control blocks to ( char * ) before deallocating.
 *      2. Delete ci_map_reason().  Port driver specific path crash reason
 *	   codes no longer exist and all reason code mapping takes place
 *	   entirely within the CI PPD.
 *	3. Rename the Informational Event( ES_I ) mnemonic from IE -> I.
 *	4. The following Informational Events( ES_I ) have been defined as
 *	   Warning Events( ES_WE ): CABLE0_GB, CABLE1_GB, CABLES_UC,
 *	   CABLE0_LBGB, CABLE1_LBGB.
 *	5. The following Informational Events( ES_I ) have been defined as
 *	   Remote Error Events( ES_RE ): RPORT_STATE.
 *	6. Refer to error logging as event logging.
 *	7. Modify ci_test_lpconn() to reset the following CI PPD specific PCCB
 *	   fields using CI configuration variables ci_cippdburst and
 *	   ci_cippdcontact immediately prior to testing local port
 *	   connectivity: burst( port polling burst size ), contact( port
 *	   polling contact frequency ).  
 *
 *   03-Jun-1988	Todd M. Katz		TMK0002
 *	1. Create a single unified hierarchical set of naming conventions for
 *	   use within the CI port driver and describe them within ciport.h.
 *	   Apply these conventions to all names( routine, macro, constant, and
 *	   data structure ) used within the driver.
 *	2. The following macros have been renamed:
 *		1) Test_mapbit() -> Test_lpinfomap()
 *		2) Set_mapbit() -> Set__lpinfomap()
 *		3) Clear_mapbit() -> Clear_lpinfomap()
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
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
#include		"../h/time.h"
#include		"../h/ksched.h"
#include		"../h/kmalloc.h"
#include		"../h/vmmac.h"
#include		"../h/errlog.h"
#include		"../h/smp_lock.h"
#include		"../machine/pte.h"
#include		"../machine/cpu.h"
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

/* External Variables and Routines.
 */
extern  struct pte 	Sysmap[];
extern	PB		*cippd_get_pb();
extern	SCSH		*gvp_alloc_dg();
extern	GVPH		*ci_alloc_pkt();
extern	u_long		ci_cippdburst, ci_cippdcontact, gvp_queue_retry;
extern	void		ci_dealloc_pkt(), ci_log_packet(), ci_update_cable();
extern	u_short		ci_maint_timer;

/*   Name:	ci_alloc_buf	- Allocate Emergency CI Command Packets
 *
 *   Abstract:	This function preallocates the following emergency CI command
 *		packets required for CI PPD directed termination of the
 *		specified path when fully established:
 *
 *		1. Emergency set circuit off command packet.
 *		2. Emergency invalidate translation cache command packet.
 *
 *		These command packets are preallocated so as to always be
 *		available for termination of the path.  They are not used if
 *		the path is aborted during its establishment.
 *
 *		NOTE: This is an optional PD function( Alloc_buf ) for use by
 *		      the CI PPD finite state machine.  The CI port driver
 *		      provides it because the driver requires emergency command
 *		      packets during path termination.  Other port drivers do
 *		      not have such needs and need not provide this function.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pd.gvp.ci		-  CI specific PB fields
 *	    invtcpkt		-   Invalidate translation cache command packet
 *	    scpkt		-   Set circuit off command packet
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Allocated emergency CI command packets
 *   RET_FAILURE		- Unable to allocate emergency command packets
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 *
 *		PCCB addresses are always valid allowing access to static
 *		fields because these data structures are never deleted once
 *		their corresponding ports have been initialized( The PCCB is
 *		locked EXTERNALLY anyway ).
 */
u_long
ci_alloc_buf( pccb, pb )
    PCCB		*pccb;
    PB			*pb;
{
    register GVPH	*cibp;

    if(( pb->Invtcpkt = ci_alloc_pkt( pccb ))) {
	if(( pb->Scpkt = ci_alloc_pkt( pccb ))) {
	    return( RET_SUCCESS );
	} else {
	    ( void )ci_dealloc_pkt( pb->Invtcpkt );
	    pb->Invtcpkt = NULL;
	}
    }
    return( RET_FAILURE );
}

/*   Name:	ci_dealloc_buf	- Deallocate Emergency CI Command Packets
 *
 *   Abstract:	This routine deallocates the following preallocated emergency
 *		CI command packets required for CI PPD directed termination of
 *		the specified path when fully established:
 *
 *		1. Emergency set circuit off command packet.
 *		2. Emergency invalidate translation cache command packet.
 *
 *		This routine is never invoked once the specified path has been
 *		fully established.  It is only invoked during the abortion of
 *		path establishment.
 *
 *		NOTE: This is an optional PD routine( Dealloc_buf ) for use by
 *		      the CI PPD finite state machine.  The CI port driver
 *		      provides it because the driver requires emergency command
 *		      packets during path termination.  Other port drivers do
 *		      not have such needs and need not provide this routine.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pd.gvp.ci		-  CI specific PB fields
 *	    invtcpkt		-   NULL
 *	    scpkt		-   NULL
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing premature deletion
 *		and allowing exclusive access to PB contents.
 */
void
ci_dealloc_buf( pb )
    register PB		*pb;
{
    register GVPH	*cibp;

    if(( cibp = pb->Invtcpkt )) {
	( void )ci_dealloc_pkt( cibp );
	pb->Invtcpkt = NULL;
    }
    if(( cibp = pb->Scpkt )) {
	( void )ci_dealloc_pkt( cibp );
	pb->Scpkt = NULL;
    }
}

/*   Name:	ci_get_port	- Retrieve Port Number from Buffer
 *
 *   Abstract:	This function retrieves the sending/destination port address
 *		from a CI specific packet.  
 *
 *		NOTE: This is a mandatory PD function( Get_port ) for use by
 *		      the CI PPD.
 *
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum CI
 *		      and CI PPD port numbers only occupy 1 byte, the low-order
 *		      byte of a port station address.  Port numbers are passed
 *		      as 4 bytes entities back and forth between the CI PPD and
 *		      CI port driver.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in packet
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   Sending/Destination port number of CI packet
 *
 *   SMP:	No locks are required.
 */
u_long
ci_get_port( pccb, cippdbp )
    PCCB		*pccb;
    GVPPPDH		*cippdbp;
{
    register GVPH	*cibp = Ppd_to_pd( cippdbp, pccb );
    return( Get_pgrp( pccb, cibp ));
}

/*   Name:	ci_init_pb	- Initialize a Path Block
 *
 *   Abstract:	This function initializes the CI port specific portion of a
 *		Path Block and verifies the suitability of the remote port for
 *		path establishment.  The information for PB initialization is
 *		obtained from the ID response received from the target remote
 *		port.  This information also indicates whether the remote port
 *		is in a state( ENABLED or MAINTENANCE/ENABLED ) suitable for
 *		path establishment.
 *
 *		Reception of ID responses from target ports other than the
 *		local port automatically establish the port's connectivity.
 *
 *		NOTE: This is an optional PD function( Init_pb ) for use by the
 *		      CI PPD finite state machine.  The CI port driver provides
 *		      it because the driver has CI port specific information to
 *		      store and because remote CI ports may be in states
 *		      unsuitable for path establishment.  Other port drivers do
 *		      not have such needs and need not provide this function.
 *		      Those port drivers which do provide this function and
 *		      allow it to return failure status must log the failure
 *		      reason in a PD specific fashion.  However, only the very
 *		      first occurrence of failure on each path should be
 *		      logged.  Subsequent failures should not be logged until
 *		      after a success status is to be returned for the path.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in ID packet
 *				   ( if it had such a header )
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pinfo.pd.gvp.type.ci	-  CI specific path information
 *	    port_fcn		-   Remote port function mask
 *	    port_fcn_ext	-   Remote port function extension
 *	    reset_port		-   Address of remote port's last resetter
 *	    rport_state		-   State of remote port
 *	    ucode_rev		-   Remote port microcode revision level
 *	pinfo.type.dual_path	-  Dual path remote port flag
 *	pinfo.type.hwtype	-  Hardware type of remote port
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.pd.gvp.type.ci	-  CI specific local port information
 *	    rpslogmap		-   Remote port state port logging bitmap
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lpstatus		-   Local port status flags
 *		connectivity	-    Port connectivity established status flag
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- PB successfully initialized
 *   RET_FAILURE		- Remote port is NOT in an acceptable state
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by ci_log_packet()
 *		in case logging becomes necessary.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
u_long
ci_init_pb( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    register GVPPPDH	*cippdbp;
{
    register u_long	status = RET_SUCCESS;
    register u_long	pktmult;

    U_long( pb->pinfo.type ) = Idrec( cippdbp )->port_type;
    U_long( pb->pinfo.Ucode_rev ) = Idrec( cippdbp )->ucode_rev;
    pb->pinfo.Port_fcn = Idrec( cippdbp )->port_fcn;
    pb->pinfo.Port_fcn_ext = Idrec( cippdbp )->port_fcn_ext;
    pb->pinfo.Port_fcn_ext2 = Idrec( cippdbp )->port_fcn_ext2;
    pb->pinfo.Rport_state = Idrec( cippdbp )->port_state;
    pb->pinfo.Reset_port = Idrec( cippdbp )->reset_port;
    if(( pktmult = (( pb->pinfo.Port_fcn_ext & PPR_IBUF_LEN ) >> 25 )) > 0 ) {
       pktmult--;
    }
    if( pktmult > pccb->Pkt_mult ) {
	pktmult = pccb->Pkt_mult;
    }
    pb->pinfo.path_pktmult = pktmult << 28;

    if( Scaaddr_low( pccb->lpinfo.addr ) !=
	Scaaddr_low( pb->pinfo.rport_addr )) {
	pccb->Lpstatus.connectivity = 1;
    }
    if( pb->pinfo.Rport_state == PS_ENAB ||
	 pb->pinfo.Rport_state == PS_ENAB_MAINT ) {
	Clear_lpinfomap( Rpslogmap, Scaaddr_low( pb->pinfo.rport_addr ))
    } else {
	if( !Test_lpinfomap( Rpslogmap, Scaaddr_low( pb->pinfo.rport_addr ))) {
	    Set_lpinfomap( Rpslogmap, Scaaddr_low( pb->pinfo.rport_addr ))
	    ( void )ci_log_packet( pccb,
				   pb,
				   Ppd_to_pd( cippdbp, pccb ),
				   RE_RPORT_STATE,
				   RPORT_EVENT );
	}
	status = RET_FAILURE;
    }
    return( status );
}

/*   Name:	ci_inv_cache	- Invalidate CI Port's Translation Cache
 *
 *   Abstract:	This routine initiates invalidation of a specific local CI
 *		port's translation cache.  It is invoked by the CI PPD finite
 *		state machine only during the termination of established paths.
 *
 *		Translation cache invalidation is initiated by placing a INVTC
 *		command packet onto the highest priority port command queue
 *		and notifying the port when the queue was previously empty.
 *
 *		A CI port command packet is used to contain the command.  It is
 *		provided to this routine and is deallocated following command
 *		execution.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *		NOTE: This is an optional PD routine( Inv_cache ) for use by
 *		      the CI PPD finite state machine.  The CI port driver
 *		      provides it because it oversee ports containing
 *		      translation caches requiring invalidation.  Other port
 *		      drivers do not have such needs and need not provide it.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pd.gvp.ci		-  CI specific PB fields
 *	    invtcpkt		-   Invalidate translation cache command packet
 *   pccb			- Port Command and Control Block pointer
 *   gvp_queue_retry		- SCA queuing failure retry count
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pd.gvp.ci		-  CI specific PB fields
 *	    invtcpkt		-   NULL
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing premature deletion
 *		and allowing exclusive access to PB contents.
 *
 *		Access to port command queues is by means of memory
 *		interlocking queuing instructions.
 *
 *              This routine requires the PCCB to be locked( EXTERNALLY )
 *		because locks lower than the PCCB in the SCA locking hierarchy
 *              such as the PB may NOT be held without also holding the PCCB
 *              lock in case the port requires crashing.
 */
void
ci_inv_cache( pccb, pb, cippdbp )
    register PCCB	*pccb;
    PB			*pb;
    GVPPPDH		*cippdbp;
{
    register GVPH	*cibp;

    /* The steps involved in invalidating a local CI port's translation cache
     * are as follows:
     *
     * 1. Retrieve the preallocated emergency invalidate translation cache
     *    command packet associated with the failed path.
     * 2. Format the generic Vaxport port header of the command packet.
     * 3. Initiate invalidation of the local CI port's translation cache.
     *
     * The port is crashed if command execution can not be successfully
     * initiated.
     */
    if(( cibp = pb->Invtcpkt )) {
	pb->Invtcpkt = NULL;
    } else {
	panic( PANIC_NOTCPKT );
    }
    Format_gvph( pccb, cibp, INVTC, 0, DEALLOC_BUF )
    Insqti_control( cibp, pccb )
}

/*   Name:	ci_notify_port	- Notify Port of CI PPD Continued Activity
 *
 *   Abstract:	This routine notifies the specified local CI port of CI PPD
 *		continued activity.  It is invoked on an ongoing basis by the
 *		CI PPD finite state machine.  It is also invoked during
 *		initialization of all local CI ports to initially engage local
 *		port maintenance timers, but only when such timers have been
 *		optionally enabled.
 *
 *		NOTE: This is an optional PD routine( Notify_port ) for use by
 *		      the CI PPD.  The CI port driver provides it because the
 *		      CI port requires continuous notification of CI PPD
 *		      activity.  Other ports do not have such needs and their
 *		      port drivers need not provide this routine and still
 *		      other port drivers may have such needs but utilize
 *		      different mechanisms to perform such notifications.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_maint_timer		- CI port maintenance timer enable flag
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    ciregptrs.pmcsr	-   Port status register pointer
 *	    ciregptrs.pmtcr	-   Port maintenance timer control reg pointer
 *	    lpstatus.mtimer	-   Maintenance timer is operational flag
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents.
 */
void
ci_notify_port( pccb )
    PCCB		*pccb;
{
    /* Notifying the specified local CI port of continuing CI PPD activity is
     * required only when CI port maintenance timers have been optionally
     * enabled.  When such enabling occurs, the maintenance timers on all CI
     * ports must be written to periodically to notify the ports of continued
     * functioning of host software.  Failure to write to a timer within a CI
     * port specific sanity interval triggers transitioning of the
     * corresponding port into the Uninitialized/Maintenance port state,
     * generation of an interrupt, and crashing of the local port.
     *
     * The sanity interval for the CI750/CI780/CIBCI maintenance timer is 100
     * seconds.  The interval for the CIBCA/CIXCA maintenance timer is
     * variable.
     *
     * Management of the specified local CI port maintenance timer includes:
     *
     * 1. Reflecting any changes in software maintenance timer state by
     *    disabling or enabling the specified local port maintenance timer as
     *	  required.
     * 2. Poking the specified local CI port maintenance timer if it is
     *	  currently software enabled to prevent it from expiring.
     */
    if( ci_maint_timer ^ pccb->Lpstatus.mtimer ) {
	DELAY( 1000 );
	Lock_cidevice( pccb )
	if(( pccb->Lpstatus.mtimer = ci_maint_timer )) {
	    *pccb->Pmcsr &= ~PMCSR_MTD;
	} else {
	    *pccb->Pmcsr |= PMCSR_MTD;
	}
	Unlock_cidevice( pccb )
	DELAY( 1000 );
    }
    if( pccb->Lpstatus.mtimer ) {
	Lock_cidevice( pccb )
	*pccb->Pmtcr = PMTCR_MTC;
	Unlock_cidevice( pccb )
        WBFLUSH
    }
}

/*   Name:	ci_send_reqid	- Request Remote Port Identification
 *
 *   Abstract:	This function initiates a request for the identification of a
 *		specific remote port.  It is provided by the port driver for
 *		use by the CI PPD finite state machine during port polling for
 *		the maintenance of existing paths and the initiation of new
 *		ones.  It is also invoked by the CI port driver under special
 *		circumstances( See ci_rsp_handler()).
 *
 *		Execution of this function may also result in scheduling of the
 *		following asynchronous CI PPD notifications:
 *
 *		1. Completion of a request for remote port identification.
 *		2. Reception of information corresponding to the report port
 *		   whose identification was requested.
 *
 *		Neither notification requires scheduling every time this
 *		function is invoked.  Notification of the former through
 *		invocation of cippd_reqid_snt() occurs only when the caller
 *		requests deallocation of the packet used in satisfying the
 *		request( dispose == DEALLOC_BUF ).  Notification of the latter
 *		through invocation of cippd_receive() occurs only when a
 *		request is made for an existing remote port.  Some requests
 *		result in no asynchronous CI PPD notifications.  Others result
 *		in both possible notifications with no restrictions placed on
 *		the order in which they occur.
 *
 *		A request for identification is initiated by placing a REQID
 *		command packet onto the second highest priority port command
 *		queue and notifying the port when the queue was previously
 *		empty.
 *
 *		Either a CI port command packet or a CI port specific datagram
 *		buffer is used to contain the command.  Datagram buffers are
 *		used only when explicitly supplied to the function and are
 *		disposed of following command execution as specified.  Command
 *		packets are allocated by the function when datagram buffers are
 *		not provided.  They are always deallocated following command
 *		execution.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *		NOTE: This is a mandatory PD function( Send_Reqid ) for use by
 *		      the CI PPD finite state machine.  Not all port drivers
 *		      issue port commands in order to obtain remote port
 *		      identifications like the CI port driver.  However, all
 *		      drivers must still supply this function for scheduling
 *		      purposes.  All port drivers must schedule CI PPD
 *		      reception of remote port identification information.
 *		      Those drivers like the CI port driver which have NOT
 *		      permanently disabled sanity checking on their local ports
 *		      must also schedule CI PPD notification of the completion
 *		      of their requests for remote port identifications.  How
 *		      each driver goes about performing each scheduling is
 *		      driver specific, but all notifications MUST be done
 *		      asynchronously.  They may NOT occur through immediate
 *		      call backs.  All drivers must also dispose of all port
 *		      specific datagram buffers optionally provided to this
 *		      function as specified.
 *		      
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum CI
 *		      and CI PPD port numbers only occupy 1 byte, the low-order
 *		      byte of a port station address.  Port numbers are passed
 *		      as 4 bytes entities back and forth between the CI PPD and
 *		      CI port driver.
 *
 *		NOTE: The requested identification information must always be
 *		      passed back to the CI PPD within a port specific datagram
 *		      buffer whenever the requested remote port is found to
 *		      exist.  The contents of this information is however
 *		      driver specific.  It is passed by the CI PPD to the
 *		      driver's optionally provided Init_pb routine during path
 *		      establishment, presumably for storage in the PB
 *		      corresponding to the new path being established.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram buffer
 *   dispose			- RECEIVE_BUF or DEALLOC_BUF
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    poll_cable		-   CI cable to use for polling
 *   gvp_queue_retry		- SCA queuing failure retry count
 *   rport_addr			- Address of target remote port
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Successfully initiated request transmission
 *   RET_ALLOCFAIL		- Unable to allocate CI command packet
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents.
 *
 *		Access to port command queues is by means of memory
 *		interlocking queuing instructions.
 */
u_long
ci_send_reqid( pccb, cippdbp, rport_addr, dispose )
    register PCCB	*pccb;
    GVPPPDH		*cippdbp;
    u_long		rport_addr;
    u_long		dispose;
{
    register REQIDH	*bp;
    register GVPH	*cibp;

    /* The steps involved in requesting a specific remote port's identification
     * are as follows:
     *
     * 1. Allocate a CI port command packet if a CI datagram buffer was not
     *	  provided.
     * 2. Format the REQID specific portion of the command packet.
     * 3. Format the generic Vaxport port header.
     * 4. Initiate request transmission.
     *
     * The request for remote port identification is always transmitted over
     * the current polling cable instead of allowing the port to choose the
     * physical cable for transmission.  Note that it is still possible to
     * allow the port to choose the physical cable for transmission by
     * specifying the current polling cable as ANY_CABLE.
     *
     * The port is crashed if transmission can not be successfully initiated.
     *
     * NOTE: The port driver utilizes port command completion as the trigger
     *	     for asynchronously notifying the CI PPD of completion of its
     *	     request for identification of the specific remote port.  The
     *	     completed command packet contains all information necessary for
     *	     this notification.  No auxiliary information need be stored.
     *
     * NOTE: Existence of the targeted remote port results in automatic remote
     *	     initiated transmission of its identification.  Reception of this
     *	     information automatically triggers asynchronous CI PPD
     *	     notification of identification reception.
     */
    if(( bp = ( REQIDH * )cippdbp )) {
	cibp = Ppd_to_pd( bp, pccb );
    } else if(( cibp = ci_alloc_pkt( pccb ))) {
	dispose = DEALLOC_BUF;
	bp = ( REQIDH * )Pd_to_ppd( cibp, pccb );
    } else {
	return( RET_ALLOCFAIL );
    }
    bp->xctid.blockid = 0;
    U_long( bp->xctid.lconnid ) = 0;
    Format_gvph( pccb, cibp, REQID, rport_addr, dispose )
    Cselect( cibp ) = pccb->Poll_cable;
    Insqti_communication( cibp, pccb )
    return( RET_SUCCESS );
}

/*   Name:	ci_set_circuit	- Set Virtual Circuit State to On/Off
 *
 *   Abstract:	This function initiates the setting of the virtual circuit
 *		associated with a specific remote port to either an on or an
 *		off state.  It is provided by the port driver for use by the
 *		CI PPD finite state machine during path establishment and
 *		disablement.
 *
 *		Circuit state modification is initiated by placing a SETCKT
 *		command packet onto the highest priority port command queue
 *		and notifying the port when the queue was previously empty.
 *
 *		A CI command packet is used to contain the command.  It is
 *		provided to this function only when the virtual circuit is to
 *		be turned off and a PB is provided.  The PB's emergency set
 *		circuit off command packet, preallocated during path
 *		establishment, is used for the command.  In all other
 *		situations the command packet must be allocated.  Packets are
 *		always deallocated following command execution.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *		NOTE: This is a mandatory PD function( Set_circuit ) for use by
 *		      the CI PPD finite state machine.  Not all port drivers
 *		      are required to issue port commands in order to set the
 *		      virtual circuit.  However, all must supply this function
 *		      for use by the CI PPD.  How each driver accomplishes
 *		      circuit state modification is left up to it.
 *
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum CI
 *		      and CI PPD port numbers only occupy 1 byte, the low-order
 *		      byte of a port station address.  Port numbers are passed
 *		      as 4 bytes entities back and forth between the CI PPD and
 *		      CI port driver.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pd.gvp.ci		-  CI specific PB fields
 *	    scpkt		-   Set circuit off command packet
 *   pccb			- Port Command and Control Block pointer
 *   rport_addr			- Address of target remote port
 *   state			- SET_VC_CLOSE or SET_VC_OPEN
 *   gvp_queue_retry		- SCA queuing failure retry count
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pd.gvp.ci		-  CI specific PB fields
 *	    scpkt		-   Set circuit off command packet
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Set circuit command successfully initiated
 *   RET_ALLOCFAIL		- Unable to allocate CI port command packet
 *
 *   SMP:	The PB is locked( EXTERNALLY ) allowing exclusive access to PB
 *		contents.
 *
 *		Access to port command queues is by means of memory
 *		interlocking queuing instructions.
 *
 *              This function requires the PCCB to be locked( EXTERNALLY )
 *		because locks lower than the PCCB in the SCA locking hierarchy
 *              such as the PB may NOT be held without also holding the PCCB
 *              lock in case the port requires crashing.
 */
u_long
ci_set_circuit( pccb, pb, rport_addr, state )
    register PCCB	*pccb;
    PB			*pb;
    u_long		rport_addr;
    u_long		state;
{
    register GVPH	*cibp;
    register SETCKTH	*bp;

    /* The steps involved in setting a specific virtual circuit state are as
     * follows:
     *
     * 1. Allocate a CI port command packet if such a packet was not provided.
     * 2. Format the SETCKT specific portion of the command packet.
     * 3. Format the generic Vaxport port header
     * 4. Initiate port command execution.
     *
     * The port is crashed if command execution can not be successfully
     * initiated.
     */
    if( pb == NULL || state == SET_VC_OPEN ) {
	if(( cibp = ci_alloc_pkt( pccb )) == NULL ) {
	    return( RET_ALLOCFAIL );
	}
    } else if(( cibp = pb->Scpkt )) {
	pb->Scpkt = NULL;
    } else {
	( void )panic( PANIC_NOSCPKT );
    }
    bp = ( SETCKTH * )Pd_to_ppd( cibp, pccb );
    U_long( bp->mvalue ) = 0;
    switch( pccb->lpinfo.type.hwtype ) {
        case HPT_CIXCD:
	    if( state ) {
                U_long( bp->mask ) = Exp_mask;
                U_long( bp->mvalue ) = EFR_MASK( pb->pinfo.Port_fcn_ext2 );
                U_long( bp->mvalue ) |= NADP_MASK( pb->pinfo.Port_fcn_ext2 );
	    } else if( pb == NULL ) {
                U_long( bp->mask ) = Exp_mask;
		/* for full subnode need to check dst_port.mem != 0 and
		   set mvalue bit to 1 or change the mask for EAS = 0 */
	    } else {
                U_long( bp->mask ) = Exp_mask;
                U_long( bp->mvalue ) = FR_MASK( pb->pinfo.Port_fcn_ext2 );
		/* for full subnode need to check dst_port.mem != 0 and
		   set mvalue bit to 1 or change the mask for EAS = 0 */
	    }
            break;

        case HPT_CIKMF:
            U_long( bp->mask ) = Kmf_mask;
            U_long( bp->mvalue ) = Kmf_mvalue;
            break;

        default:
            U_long( bp->mask ) = Imp_mask;
            break;
    }
    bp->mvalue.vcd_ov.fsn.vcstate = state;
    Format_gvph( pccb, cibp, SETCKT, rport_addr, DEALLOC_BUF )
    Insqti_control( cibp, pccb )
    return( RET_SUCCESS );
}

/*   Name:	ci_test_lpconn	- Test Local CI Port Connectivity
 *
 *   Abstract:	This routine tests the connectivity of the specified local CI
 *		port.  It also resets the CI PPD port polling contact frequency
 *		and burst size on the local port for the next polling sweep
 *		using CI configuration variables.  It is invoked by the CI PPD
 *		during port polling at the conclusion of the current polling
 *		sweep.
 *
 *		Local port connectivity is defined as the demonstrated ability
 *		to communicate with other CI ports over the CI itself.  A local
 *		ports ability to communicate with itself is not sufficient to
 *		establish local port connectivity as such communication may be
 *		looped back internally bypassing the CI.  Only those local
 *		ports with demonstrated ability to communicate with ports other
 *		than themselves are consider to possess connectivity.
 *
 *		Testing local port connectivity makes use of the special
 *		loopback function provided by CI ports.  This loopback 
 *		function guarantees transmission of special loopback datagrams
 *		over the CI, even when the datagrams are directed to the local
 *		port itself.  Loopback datagram transmission is initiated by
 *		placing a SNDLB command packet onto the second highest priority
 *		port command queue and notifying the port when the queue was
 *		previously empty.
 *
 *		A full size CI port specific datagram buffer is used to contain
 *		the command.  It is allocated by this routine and placed onto
 *		the appropriate local port datagram free queue following
 *		successful command execution for reception of the corresponding
 *		response( LBREC ).  Note: this explains why full size datagram
 *		buffers are used to contain SNDLB port commands instead of
 *		smaller CI port command packets.  Corresponding responses are
 *		always deallocated following processing.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *		NOTE: This is an optional PD routine( Test_lportconn ) for use
 *		      by the CI PPD.  The CI port driver provides it because it
 *		      oversees ports whose connectivity may come into question
 *		      and must be verified when this is indeed the case.  Other
 *		      port drivers do not have such needs and need not provide
 *		      this function.
 *
 *		NOTE: Lack of local port connectivity may be due to many
 *		      factors ranging from hardware problems to failure to
 *		      properly cable up the CI port to the coupler/switch.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   ci_cippdburst		- CI PPD port polling burst size
 *   ci_cippdcontact		- CI PPD port polling contact frequency
 *   pccb			- Port Command and Control Block pointer
 *   gvp_queue_retry		- SCA queuing failure retry count
 *
 *   Outputs:
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lbstatus		-   Loopback status flags
 *		cable0_curr	-    Cable 0 current status flag
 *		cable0_prev	-    Cable 0 previous status flag
 *		cable0_test	-    Cable 0 loopback tested status flags
 *		cable1_curr	-    Cable 1 current status flag
 *		cable1_prev	-    Cable 1 previous status flag
 *		cable1_test	-    Cable 1 loopback tested status flags
 *	    lpstatus		-   Local port status flags
 *		connectivity	-    Port connectivity established status flag
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    burst		-   Port polling burst size
 *	    contact		-   Port polling contact frequency
 *	    fsmstatus.online	-   1
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access and as
 *		required by ci_update_cable() for updating of loopback cable
 *		status.
 *
 *		Access to port command queues is by means of memory
 *		interlocking queuing instructions.
 */
void
ci_test_lpconn( pccb )
    register PCCB	*pccb;
{
    register GVPH	*cibp;
    register SCSH	*scsbp;
    register SNDLBH	*bp;
    register u_long	npaths, skip_test;

    /* Testing local CI port connectivity is naturally subdivided into testing
     * the connectivity of each of the local port's cables.  Each time this
     * routine is invoked, the connectivity of one of the port's cables is
     * targeted.  Which cable is targeted by each invocation is defined by the
     * current CI PPD port polling cable.
     *
     * Each test of local port connectivity consists of the following phases:
     *
     * 1. Updating local port connectivity status.
     * 2. Determining whether an explicit connectivity test is required.
     * 3. Initiating any required connectivity test.
     *
     * Updating local port connectivity status( Phase 1 ) consists of updating
     * local port loopback cable status from the previous test of the current
     * cable.  Any good->bad loopback cable transitions are also logged.
     *
     * An explicit connectivity test is required( Phase 2 ) when either local
     * port connectivity has not yet been established or connectivity of the
     * target cable not yet explicitly demonstrated.  Local port connectivity
     * is demonstrated only through reception of an identification response
     * from a port other than the local one.  An explicit demonstration of
     * cable connectivity is achieved only through a successful result of a
     * previous connectivity test on the target cable.
     *
     * Determining the need for an explicit connectivity test( Phase 2 ) also
     * involves determining whether established local port connectivity has
     * been lost.  Whenever the local port has lost communication with all
     * ports, or all ports but itself, local port connectivity is considered to
     * be lost.  The PCCB is updated appropriately and connectivity tests are
     * executed each time this routine is invoked until connectivity is again
     * demonstrated on each cable.
     *
     * A cable connectivity test( Phase 3 ) basically consists of transmitting
     * a special loopback datagram to the local port over the target cable by
     * means of the following steps:
     *
     * 1. Allocating a full size CI port specific datagram buffer for use as a
     *	  port command packet.
     * 2. Formatting the SNDLB specific portion of the command packet.
     * 3. Formatting the generic Vaxport header.
     * 4. Initiating port command execution.
     *
     * Port command execution( Step 4 ) leads to loopback datagram transmission
     * over the CI to the local port itself.  The local port is used as the
     * target for the loopback datagram because it is the only port guaranteed
     * to be present.  The loopback packet is transmitted over the current
     * polling cable instead of allowing the port to choose the physical cable
     * for transmission. The port is always crashed whenever loopback datagram
     * transmission can not be successfully initiated.
     *
     * Successful reception of the loopback datagram( LBREC ) successfully
     * completes the connectivity test on the current cable.  Local port
     * loopback cable status is updated and any bad->good loopback cable
     * transitions are logged.  The connectivity test on the current cable is
     * also completed by either failure to transmit the loopback datagram or
     * failure to successfully receive the loopback datagram.  Such failure is
     * noted the next time this routine is invoked( Phase 1 ) to test the
     * connectivity status of the target cable.
     *
     * Prior to testing local port connectivity, the CI PPD port polling burst
     * size and contact frequency for the local port are reset from CI
     * configuration variables.
     */
    pccb->Contact = ci_cippdcontact;
    pccb->Burst = ci_cippdburst;
    ( void )ci_update_cable( pccb, NULL, NULL, CABLE_LB_GB );
    if( pccb->Lpstatus.connectivity ) {
	if(( npaths = pccb->lpinfo.Nform_paths + pccb->lpinfo.Npaths ) <= 1 &&
	     ( npaths == 0 ||
	       cippd_get_pb( pccb, ( SCSH * )&pccb->lpinfo.addr, NO_BUF ))) {
	    pccb->Lpstatus.connectivity = 0;
	    pccb->Lbstatus.cable0_test = 0;
	    pccb->Lbstatus.cable1_test = 0;
	    skip_test = 0;
	} else if( pccb->Poll_cable == FIRST_CABLE ) {
	    if( pccb->Lbstatus.cable0_prev || !pccb->Lbstatus.cable0_test ) {
		skip_test = 0;
	    } else {
		skip_test = 1;
	    }
	} else if( pccb->Lbstatus.cable1_prev || !pccb->Lbstatus.cable1_test ){
	    skip_test = 0;
	} else {
	    skip_test = 1;
	}
	if( skip_test ) {
	    return;
	}
    }
    if(( scsbp = gvp_alloc_dg( pccb ))) {
	cibp = Scs_to_pd( scsbp, pccb );
	bp = Sndlb( Pd_to_ppd( cibp, pccb ));
	bp->lblength = LBDSIZE;
	( void )bcopy( pccb->Lbdata, bp->lbdata, LBDSIZE );
	( void )bcopy(( u_char * )&pccb->Lbcrc, bp->crc, sizeof( u_long ));
	Format_gvph( pccb, cibp, SNDLB, 
		     Scaaddr_low( pccb->lpinfo.addr ), RECEIVE_BUF )
	if(( Cselect( cibp ) = pccb->Poll_cable ) == FIRST_CABLE ) {
	    pccb->Lbstatus.cable0_curr = 1;
	    pccb->Lbstatus.cable0_test = 1;
	} else {
	    pccb->Lbstatus.cable1_curr = 1;
	    pccb->Lbstatus.cable1_test = 1;
	}
	Insqti_maintenance( cibp, pccb )
    }
}

/*   Name:	ci_update_ptype	- Update Hardware Port Type of Remote Port
 *
 *   Abstract:	This routine updates the remote port's hardware port type as
 *		necessary.  It is required because CI750, CI780, and CIBCI CI
 *		ports all identify themselves as CI780 CI ports to requests for
 *		identification.
 *
 *		This routine is invoked by the CI PPD finite state machine
 *		after it has received both an identification packet and a
 *		START/STACK CI PPD datagram.  It is only at this time that
 *		sufficient information is available for correctly specifying
 *		the remote port's hardware port type.
 *
 *		NOTE: This is an optional PD routine( Update_ptype ) for use by
 *		      the CI PPD finite state machine.  The CI port driver
 *		      provides it because it is unable to easily distinguish
 *		      between certain CI ports until a certain point in the
 *		      path establishment process.  Other port drivers do not
 *		      have such needs and need not provide this routine.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pinfo.type.hwtype	-  Remote hardware port type
 *
 *   SMP:	The PB is locked( EXTERNALLY ) postponing potential PB deletion
 *		and allowing exclusive access to PB contents.
 */
void
ci_update_ptype( pb )
    PB			*pb;
{
    if( pb->pinfo.type.hwtype == HPT_CI780 ) {
	if( bcmp(( char *)&pb->sb->sinfo.hwtype, "V750", 4 ) == 0 ) {
	    pb->pinfo.type.hwtype = HPT_CI750;
	} else if(( bcmp(( char *)&pb->sb->sinfo.hwtype, "V780", 4 ) != 0 ) &&
		  ( bcmp(( char *)&pb->sb->sinfo.hwtype, "8600", 4 ) != 0 )) {
	    pb->pinfo.type.hwtype = HPT_CIBCI;
	}
    }
}

/*   Name:	ci_alloc_pkt	- Allocate CI Port Command Packet
 *
 *   Abstract:	This function allocates a CI port specific command packet from
 *		dynamic kernel memory.  Such packets must always be directed to
 *		the local port response queue following command execution.
 *		They must never be directed to the appropriate local port free
 *		queue.
 *
 *		NOTE: The CI specific PCCB field pkt_size must contain the size
 *		      of the largest CI port command to always be directed to
 *		      the local port response queue following command
 *		      execution.  Currently this is the SETCKT port command.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    pkt_size		-   Size of port command packet
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   Address of generic Vaxport header in buffer on success
 *   Otherwise NULL
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 */
GVPH *
ci_alloc_pkt( pccb )
    PCCB		*pccb;
{
    register GVPH	*cibp;

    KM_ALLOC( cibp, GVPH *, pccb->Pkt_size, KM_SCABUF, KM_NOWAIT|KM_WIRED )
    if( cibp ) {
	U_long( cibp->size ) = ( u_long )(( DYN_CICMD << 16 ) |
					   pccb->Pkt_size );
	Dm_msg_dg( cibp, pccb->Pkt_size )
    }
    return( cibp );
}

/*   Name:	ci_dealloc_pkt	- Deallocate CI Port Command Packet
 *
 *   Abstract:	This routine deallocates a CI port specific command packet to
 *		dynamic kernel memory.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cibp			- Address of generic Vaxport header
 *
 *   Outputs:	
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.
 */
void
ci_dealloc_pkt( cibp )
    GVPH	*cibp;
{
    KM_FREE(( char * )cibp, KM_SCABUF )
}

/*   Name:	ci_update_cable	- Update Cable Status of Path Block
 *
 *   Abstract:	This routine checks for the specified cable transition.  The
 *		following types of transitions may be checked for:
 *
 *		1. Cable transitions from good to bad.
 *		2. Cable transitions from bad to good.
 *		3. Cables transition from crossed to uncrossed or uncrossed to
 *		   crossed.
 *		4. Loopback cable transitions from good to bad.
 *		5. Loopback cable transitions from bad to good.
 *
 *		Occurrence of a cable transition triggers logging of the event.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cibp			- Address of generic Vaxport header( OPTIONAL )
 *   pb				- Address of Path Block( OPTIONAL )
 *   pccb			- Port Command and Control Block pointer
 *   type			- CABLE_BG, CABLE_GB, CABLE_CROSSED,
 *				   CABLE_LB_BG, or CABLE_LB_GB
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *	pd.gvp.type.ci		-  CI specific PB fields
 *	    pstatus		-   Path status flag bits
 *		cable0		-    Cable0 status flag
 *		cable1		-    Cable1 status flag
 *		cables_crossed	-    Cables crossed flag
 *   pccb			- Port Command and Control Block pointer
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    lbstatus		-   Loopback status flags
 *		cable0_curr	-    Cable 0 current status flags
 *		cable0_prev	-    Cable 0 previous status flags
 *		cable1_curr	-    Cable 1 current status flags
 *		cable1_prev	-    Cable 1 previous status flags
 *
 *   SMP:       The PCCB is locked( EXTERNALLY ) in case the occurrence of a
 *		cable transition requires its logging.
 *
 *		The PB is locked( EXTERNALLY ), whenever one is provided,
 *		postponing potential deletion and allowing exclusive access to
 *		PB contents.
 */
void
ci_update_cable( pccb, pb, cibp, type )
    PCCB		*pccb;
    PB			*pb;
    GVPH		*cibp;
    u_long		type;
{
    register LBRECH	*lb;
    register u_long	event = 0;

    /* The following types of transition checks update the cables status of
     * appropriate PBs utilizing information found within CI port packets:
     *
     * 1. CABLE_GB	- Cable transitions from cable good to cable bad.
     * 2. CABLE_BG	- Cable transitions from cable bad to cable good.
     * 3. CABLE_CROSSED	- Cable transitions from cables crossed to cables
     *			  uncrossed or from cables uncrossed to cables crossed.
     *
     * Determination of whether loopback cables have transitioned from good to
     * bad( CABLE_LB_GB ) updates the loopback cable status of appropriate
     * PCCBs and does NOT utilize CI port packets but only PCCB information.
     *
     * The last type of cable transition checking, whether loopback cables have
     * transitioned from bad to good, utilizes CI port packets but only when
     * such packets are provided.  Otherwise, only PCCB information is used in
     * such determinations together with the additional assumption that the
     * appropriate loopback cable status are now good.  The loopback cable
     * statuses of appropriate PCCBs are always updated.
     */
    switch( type ) {

	case CABLE_GB:
	    if( Cable0_status( cibp ) != CABLE_ACK ) {
		if( !pb->Pstatus.cable0 ) {
		    event = W_CABLE0_GB;
		    pb->Pstatus.cable0 = 1;
		}
	    } else if( !pb->Pstatus.cable1 ) {
		event = W_CABLE1_GB;
		pb->Pstatus.cable1 = 1;
	    }
	    break;

	case CABLE_BG:
	    if( Receive_cable( cibp ) == CS_CABLE0 ) {
		if( pb->Pstatus.cable0 ) {
		    event = I_CABLE0_BG;
		    pb->Pstatus.cable0 = 0;
		}
	    } else if( pb->Pstatus.cable1 ) {
		event = I_CABLE1_BG;
		pb->Pstatus.cable1 = 0;
	    }
	    break;

	case CABLE_CROSSED:
	    if( Send_cable( cibp ) == Receive_cable( cibp )) {
		if( pb->Pstatus.cables_crossed ) {
		    event = I_CABLES_CU;
		    pb->Pstatus.cables_crossed = 0;
		}
	    } else if( !pb->Pstatus.cables_crossed ) {
		event = W_CABLES_UC;
		pb->Pstatus.cables_crossed = 1;
	    }
	    break;

	case CABLE_LB_GB:
	    if( pccb->Poll_cable == FIRST_CABLE ) {
		if( !pccb->Lbstatus.cable0_prev && pccb->Lbstatus.cable0_curr){
		    event = W_CABLE0_LBGB;
		}
		pccb->Lbstatus.cable0_prev = pccb->Lbstatus.cable0_curr;
	    } else {
		if( !pccb->Lbstatus.cable1_prev && pccb->Lbstatus.cable1_curr){
		    event = W_CABLE1_LBGB;
		}
		pccb->Lbstatus.cable1_prev = pccb->Lbstatus.cable1_curr;
	    }
	    break;

	case CABLE_LB_BG:
	    lb = Lbrec( Pd_to_ppd( cibp, pccb ));
	    if( Comp_loopback( pccb, lb )) {
		if( Cselect( cibp ) == CS_CABLE0 ) {
		    pccb->Lbstatus.cable0_curr = 0;
		    if( pccb->Lbstatus.cable0_prev ) {
			event = I_CABLE0_LBBG;
		    }
		} else {
		    pccb->Lbstatus.cable1_curr = 0;
		    if( pccb->Lbstatus.cable1_prev ) {
			event = I_CABLE1_LBBG;
		    }
		}
	    }
	    break;

	default:
	    ( void )panic( PANIC_CABLE );
    }
    if( event ) {
	( void )ci_log_packet( pccb, pb, NULL, event, RPORT_EVENT );
    }
}
