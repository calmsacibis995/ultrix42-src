#ifndef	lint
static char *sccsid = "@(#)msi_subr.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
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
 *		Driver( MSI ) functions provided for use by the CI PPD
 *		finite state machine in the establishment, maintenance,
 *		and termination of paths.  Other MSI functions(
 *		msi_init_port() and msi_log_badport() ) required by the
 *		CI PPD are located in other modules.
 *
 *		This module also contains miscellaneous MSI functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	January 16, 1989
 *
 *   Functions/Routines:
 *
 *   Functions and Routines Required by the CI PPD Finite State Machine
 *	msi_get_port		Retrieve Port Number from Buffer
 *   	msi_init_pb		Initialize a Path Block
 *	msi_send_reqid		Request Remote Port Identification
 *   	msi_set_circuit		Set Virtual Circuit State to On/Off
 *    MSI Port Driver Miscellaneous Functions
 *	msi_alloc_pkt		Allocate MSI Port Command Packet
 *	msi_dealloc_pkt		Deallocate MSI Port Command Packet
 *
 *   Modification History:
 *
 *   14-Jun-1989	Pete Keilty
 *	Add include file smp_lock.h
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/dyntypes.h"
#include		"../h/param.h"
#include		"../h/time.h"
#include		"../h/ksched.h"
#include		"../h/kmalloc.h"
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
#include		"../io/ci/cippd.h"
#include		"../io/msi/msiport.h"

/* External Variables and Routines.
 */
extern	MSIB		*msi_alloc_pkt();
extern	void		msi_dealloc_pkt(),
			msi_log_packet(),
			msi_xfp();

/*   Name:	msi_get_port	- Retrieve Port Number from Buffer
 *
 *   Abstract:	This function retrieves the remote port station address from a
 *		MSI specific packet.
 *
 *		NOTE: This is a mandatory PD function( Get_port ) for use by
 *		      the CI PPD.
 *
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum
 *		      MSI and CI PPD port numbers only occupy 1 byte, the
 *		      low-order byte of a port station address.  Port numbers
 *		      are passed as 4 bytes entities back and forth between the
 *		      CI PPD and MSI port driver.
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
 *   Sending/Destination port number of MSI packet
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid because
 *		these data structures are never deleted once their
 *		corresponding ports have been initialized.
 */
u_long
msi_get_port( pccb, cippdbp )
    PCCB		*pccb;
    CIPPDH		*cippdbp;
{
    register MSIB	*msibp = Ppd_to_pd( pccb, cippdbp );

    return(( u_long )msibp->Rport_addr );
}

/*   Name:	msi_init_pb	- Initialize a Path Block
 *
 *   Abstract:	This function initializes the MSI port specific portion of a
 *		Path Block and verifies the suitability of the remote port for
 *		path establishment.  The information for PB initialization is
 *		obtained from the ID response received from the target remote
 *		port.  This information also indicates whether the remote port
 *		is in a state( ENABLED or MAINTENANCE/ENABLED ) suitable for
 *		path establishment.
 *
 *		NOTE: This is an optional PD function( Init_pb ) for use by the
 *		      CI PPD finite state machine.  The MSI port driver
 *		      provides it because the driver has MSI port specific
 *		      information to store and because remote MSI ports may be
 *		      in states unsuitable for path establishment.  Other port
 *		      drivers do not have such needs and need not provide this
 *		      function.  Those port drivers which do provide this
 *		      function and allow it to return failure status must log
 *		      the failure reason in a PD specific fashion.  However,
 *		      only the very first occurrence of failure on each path
 *		      should be logged.  Subsequent failures should not be
 *		      logged until after a success status is to be returned for
 *		      the path.
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
 *	pinfo.pd.msi		-  MSI specific path information
 *	    rpi			-   Remote port information( INITIALIZED )
 *	pinfo.type.hwtype	-  Hardware type of remote port
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.pd.msi		-  MSI specific local port information
 *	    rpslogmap		-   Remote port state port logging bitmap
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- PB successfully initialized
 *   RET_FAILURE		- Remote port is NOT in an acceptable state
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by
 *		msi_log_packet() in case logging becomes necessary.
 *
 *		The PB is locked( EXTERNALLY ) postponing potential deletion
 *		and allowing exclusive access to PB contents.
 */
u_long
msi_init_pb( pccb, pb, cippdbp )
    PCCB		*pccb;
    register PB		*pb;
    register CIPPDH	*cippdbp;
{
    register MSIB	*msibp = Ppd_to_pd( pccb, cippdbp );
    register u_long	status = RET_SUCCESS;

    pb->pinfo.type.hwtype = ( u_char )msibp->Id.id.port_type[ 0 ];
    ( void )bcopy(( u_char * )&msibp->Id.id.portinfo,
		  ( u_char * )&pb->pinfo.Rpinfo,
		  sizeof( MSIRPI ));
    if( pb->pinfo.Rpinfo.sys_state.port_state == PS_ENAB ||
	 pb->pinfo.Rpinfo.sys_state.port_state == PS_ENAB_MAINT ) {
	Clear_lpinfomap( Rpslogmap, Scaaddr_lob( pb->pinfo.rport_addr ))
    } else {
	if( !Test_lpinfomap( Rpslogmap, Scaaddr_lob( pb->pinfo.rport_addr ))) {
	    Set_lpinfomap( Rpslogmap, Scaaddr_lob( pb->pinfo.rport_addr ))
	    ( void )msi_log_packet( pccb,
				    pb,
				    NULL,
				    &msibp->Ph,
				    sizeof( MSI_ID ),
				    RE_RPORTSTATE,
				    RPORT_EVENT );
	}
	status = RET_FAILURE;
    }
    return( status );
}

/*   Name:	msi_send_reqid	- Request Remote Port Identification
 *
 *   Abstract:	This function initiates a request for the identification of a
 *		specific remote port.  It is provided by the port driver for
 *		use by the CI PPD finite state machine during port polling for
 *		the maintenance of existing paths and the initiation of new
 *		ones.
 *
 *		Execution of this function may also result in asynchronous
 *		reception of information corresponding to the report port whose
 *		identification was requested.  Such reception through
 *		invocation of cippd_receive() occurs only when a request is
 *		made for an existing remote port.
 *
 *		Initiating a request for remote port identification is
 *		accomplished by initiating transmission of a MSI port specific
 *		IDREQ packet to the target remote port.  Either a MSI port
 *		command packet or a MSI port specific datagram buffer may be
 *		used to contain the identification request.  Datagram buffers
 *		are used only when explicitly supplied to the function and are
 *		disposed of following packet transmission as specified.
 *		Command packets are allocated by this function when datagram
 *		buffers are not provided.  They are always deallocated
 *		following packet transmission.
 *
 *		NOTE: This is a mandatory PD function( Send_Reqid ) for use by
 *		      the CI PPD finite state machine.  Not all port drivers
 *		      issue port commands in order to obtain remote port
 *		      identifications like the MSI port driver.  However, all
 *		      drivers must still supply this function for scheduling
 *		      purposes.  All port drivers must schedule CI PPD
 *		      reception of remote port identification information.
 *		      Those drivers, unlike the MSI port driver, which have NOT
 *		      permanently disabled sanity checking on their local ports
 *		      must also schedule CI PPD notification of the completion
 *		      of their requests for remote port identifications.  Such
 *		      notifications occur through invocation of
 *		      cippd_reqid_snt(), but only when the caller requests
 *		      deallocation of the packet used in satisfying the request
 *		      ( dispose == DEALLOC_BUF ).  How each driver goes about
 *		      performing each scheduling is driver specific, but all
 *		      notifications MUST be done asynchronously.  They may NOT
 *		      occur through immediate call backs.  All drivers must
 *		      also dispose of all port specific datagram buffers
 *		      optionally provided to this function as specified.
 *		      
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum
 *		      MSI and CI PPD port numbers only occupy 1 byte, the
 *		      low-order byte of a port station address.  Port numbers
 *		      are passed as 4 bytes entities back and forth between the
 *		      CI PPD and MSI port driver.
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
 *		NOTE: All attempts to transmit packets to the local ports own
 *		      station address are bypassed.  The SII chip is not
 *		      capable of either internal loopback or simultaneous
 *		      transmission and reception; and, no need exists to
 *		      provide this function in software.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cippdbp			- Address of CI PPD header in datagram buffer
 *   dispose			- RECEIVE_BUF or DEALLOC_BUF
 *   pccb			- Port Command and Control Block pointer
 *   rport_addr			- Address of target remote port
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
 *   RET_SUCCESS		- Successfully initiated request transmission
 *   RET_ALLOCFAIL		- Unable to allocate MSI command packet
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) allowing exclusive access to
 *		PCCB contents.
 *
 *		The PCCB specific COMQL is locked allowing exclusive access to
 *		the corresponding low priority command queue
 */
u_long
msi_send_reqid( pccb, cippdbp, rport_addr, dispose )
    register PCCB	*pccb;
    CIPPDH		*cippdbp;
    u_long		rport_addr;
    u_long		dispose;
{
    register MSIB	*msibp;

    /* Only the Transmit Fork Process( XFP ) interfaces directly with the SII
     * chip for the purpose of processing outgoing MSI packets.  All other
     * driver routines must interface with the XFP.  Therefore, the steps
     * involved in requesting transmission of a remote port identification
     * request are as follows:
     *
     * 1. Verify the target port station address.
     * 2. Allocate a MSI port command packet if a MSI port specific datagram
     *	  buffer was not explicitly provided.
     * 3. Format the MSIB buffer.
     * 4. Insert the MSIB buffer onto the appropriate command queue.
     * 5. Request the XFP to process outgoing MSI packets.
     *
     * Requests for identification of the local ports own station address are
     * always satisfied without transmitting an IDREQ packet( Step 1 ).  All
     * MSI port specific datagram buffers provided are immediately disposed of
     * as directed.  Furthermore, the CI PPD is purposely never asynchronously
     * notified of the existence of the local port.
     *
     * Formatting the MSIB buffer( Step 3 ) consists of formatting the:
     *
     * MSIB buffer header - information needed by XFP for packet transmission
     * MSI port header    - common MSI packet fields
     * MSI packet body	  - IDREQ specific MSI packet fields
     *
     * IDREQ specific MSI packet fields requiring formatting by this routine
     * include only the transaction identifier.  This field is currently always
     * set to 0.
     *
     * MSIB buffers containing identification requests are always inserted onto
     * the appropriate low priority command queue( Step 4 ).  The PCCB specific
     * COMQL is locked immediately prior to insertion and unlocked immediately
     * afterwards.  This guarantees exclusive access to the queue.
     *
     * The XFP is requested to begin processing of outgoing MSI packets( Step
     * 5 ) by scheduling its asynchronous execution.  This step is bypassed
     * whenever XFP execution has already been scheduled but has not yet
     * commenced.  During XFP scheduling the PCCB is locked( EXTERNALLY ) to
     * guarantee the existence of only 1 scheduled asynchronous XFP thread at
     * any given moment.
     *
     * NOTE:	It is possible for a XFP thread to be currently active.  This
     *		does not prevent scheduling of asynchronous XFP execution.
     *		However, the new thread does not begin to process outgoing MSI
     *		packets until the currently active thread completes.  Also, no
     *		other additional XFP threads are scheduled until the new thread
     *		begins processing outgoing MSI packets.
     *
     * NOTE:	Existence of the targeted remote port results in automatic
     *		remote initiated transmission of its identification.  Reception
     *		of this information automatically triggers asynchronous CI PPD
     *		notification of identification reception.
     */
    if( rport_addr == Scaaddr_lob( pccb->lpinfo.addr )) {
	if( cippdbp ) {
	    msibp = Ppd_to_pd( pccb, cippdbp );
	    if( dispose == DEALLOC_BUF ) {
		( void )msi_dealloc_pkt( msibp );
	    } else {
		Reset_msib( Msibp )
		Insert_dfreeq( pccb, msibp )
	    }
	}
	return( RET_SUCCESS );
    }
    if( cippdbp ) {
	msibp = Ppd_to_pd( pccb, cippdbp );
    } else if(( msibp = msi_alloc_pkt( pccb ))) {
	dispose = DEALLOC_BUF;
    } else {
	return( RET_ALLOCFAIL );
    }
    Format_msibh( msibp, rport_addr, sizeof( MSI_IDREQ ), dispose )
    Format_msih( msibp, IDREQ )
    Format_idreq( msibp )
    Insert_comql( pccb, msibp )
    Xstart_xfp( pccb )
    return( RET_SUCCESS );
}

/*   Name:	msi_set_circuit	- Set Virtual Circuit State to On/Off
 *
 *   Abstract:	This function sets the virtual circuit associated with a
 *		specific remote port to either an on or an off state.  It is
 *		provided by the port driver for use by the CI PPD finite state
 *		machine during path establishment and disablement.
 *
 *		NOTE: This is a mandatory PD function( Set_circuit ) for use by
 *		      the CI PPD finite state machine.  Some port drivers are
 *		      required to issue port commands in order to set the
 *		      virtual circuit.  The MSI port driver does not possess
 *		      this requirement.  However, all drivers must supply this
 *		      function for use by the CI PPD.  How each driver
 *		      accomplishes circuit state modification is left up to it.
 *
 *		NOTE: SCA port numbers are 6 bytes in size; however, maximum
 *		      MSI and CI PPD port numbers only occupy 1 byte, the
 *		      low-order byte of a port station address.  Port numbers
 *		      are passed as 4 bytes entities back and forth between the
 *		      CI PPD and MSI port driver.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *   rport_addr			- Address of target remote port
 *   state			- SET_VC_CLOSE or SET_VC_OPEN
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	    perport		-   Per-DSSI port information
 *		rpstatus.vc	-    Virtual circuit enabled flag
 *		rseqno		-    0
 *		xseqno		-    0
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Virtual circuit state set as directed
 *
 *   SMP:	The PB is locked( EXTERNALLY ) allowing exclusive access to PB
 *		contents.
 *
 *   		The PCCB specific XFP and RFP are locked to synchronize access.
 *		Both locks are required for exclusive access to the MSI
 *		specific PCCB remote port status flag corresponding to the
 *		virtual circuit state of the target remote port.
 *
 *		PCCB addresses are always valid because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized( The PCCB is locked EXTERNALLY anyway ).
 */
u_long
msi_set_circuit( pccb, pb, rport_addr, state )
    register PCCB		*pccb;
    PB				*pb;
    register u_long		rport_addr;
    u_long			state;
{
    register MSI_PPORTINFO	*tpi = &pccb->Perport[ rport_addr ];
    Lock_rfp( pccb )
    Lock_xfp( pccb )
    tpi->xseqno = 0;
    tpi->rseqno = 0;
    tpi->rpstatus.vc = state;
    Unlock_xfp( pccb )
    Unlock_rfp( pccb )
    return( RET_SUCCESS );
}

/*   Name:	msi_alloc_pkt	- Allocate MSI Port Command Packet
 *
 *   Abstract:	This function allocates a MSI port specific command packet from
 *		dynamic kernel memory.  Such packets must always be deallocated
 *		following packet transmission.  They must never be inserted
 *		onto the appropriate local port free queue.
 *
 *		NOTE: The MSI specific PCCB field pkt_size must contain the
 *		      size of the largest MSI port command to always be
 *		      deallocated following packet transmission.  Currently
 *		      this is the STRT port command.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    pkt_size		-   Size of port command packet
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   Address of MSIB buffer in buffer on success
 *   Otherwise NULL
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 */
MSIB *
msi_alloc_pkt( pccb )
    register PCCB	*pccb;
{
    register MSIB	*msibp;

    KM_ALLOC( msibp, MSIB *, pccb->Pkt_size, KM_SCABUF, KM_NOWAIT )
    if( msibp ) {
	Format_msib( msibp, pccb->Pkt_size, DYN_MSICMD )
    }
    return( msibp );
}

/*   Name:	msi_dealloc_pkt	- Deallocate MSI Port Command Packet
 *
 *   Abstract:	This routine deallocates a MSI port specific command packet to
 *		dynamic kernel memory.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   msibp			- Address of MSIB buffer
 *
 *   Outputs:	
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.
 */
void
msi_dealloc_pkt( msibp )
    MSIB	*msibp;
{
    KM_FREE(( char * )msibp, KM_SCABUF )
}
