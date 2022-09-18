#ifndef	lint
static char *sccsid = "@(#)msi_error.c	4.2	(ULTRIX)	11/13/90";
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
 *		Driver( MSI ) error processing routines and functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 17, 1988
 *
 *   Function/Routines:
 *
 *   msi_clean_port		Clean up Local MSI Port
 *   msi_console_log		Log MSI Events to Console Terminal
 *   msi_disable		Disable Local MSI Port
 *   msi_log_badport		Log Bad Port Numbers in MSI Packets
 *   msi_log_devattn		Log MSI Device Attention Events
 *   msi_log_initerr		Log MSI Port Initialization Fatal Errors
 *   msi_log_packet		Log MSI Packet Related Events
 *
 *   Modification History:
 *
 *   13-Nov-1990	Pete Keilty
 *	Change PF_ERROR reason to PF_PORTERROR in msi_clean_port
 *	because of SCS resource fix.
 *
 *   25-Sep-1989	Pete Keilty
 *	Add Splscs() to msi_clean_port raise ipl to IPL_SCS.
 *
 *   14-Jun-1989	Pete Keilty
 *	Add include file smp_lock.h
 *
 *   27-Mar-1989	Todd M. Katz		TMK0001
 *	All local port specific MSI fatal errors are currently fully logged by
 *	msi_log_initerr().  Modify msi_console_log() appropriately.
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
#include		"../machine/pte.h"
#include		"../io/uba/ubareg.h"
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
extern	SCSIB		lscs;
extern	struct el_rec	*ealloc();
extern	u_short		msi_severity;
extern  scaaddr         scs_system_id;
extern  u_char          scs_node_name[];
extern	CLSTAB		msi_cltab[ ES_FE + 1 ][ ESC_PPD + 1 ];
extern	void		cippd_stop(),
			msi_console_log(),
			msi_log_packet();

/*   Name:	msi_clean_port	- Clean up Local MSI Port
 *
 *   Abstract:	This routine directs the second stage of local MSI port clean
 *		up.  It is always invoked by forking to it.
 *
 *		Failed local MSI ports are cleaned up in two stages.  The first
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
msi_clean_port( pccb )
    register PCCB	*pccb;
{
    register u_long	save_ipl = Splscs();

    /* The steps involved in the second stage of local port clean up include:
     *
     * 1. Locking the PCCB.
     * 2. Unlocking the PCCB.
     * 3. Notifying the CI PPD of failure of the local port.
     *
     * The CI PPD completes the clean up of its portion of the local port
     * including the failure and clean up all paths, both formative and fully
     * established, originating at the port( Step 3 ).  Clean up of the last
     * path triggers scheduling of port re-initialization by the CI PPD.  The
     * PCCB lock is released( Step 4 ) prior to notifying the CI PPD of local
     * port failure instead of after it as required by the SCA architecture.
     */
    Lock_pccb( pccb )
    Pccb_fork_done( pccb, PANIC_PCCBFB )
    Unlock_pccb( pccb )
    ( void )cippd_stop( pccb, PF_PORTERROR );
    ( void )splx( save_ipl );
}

/*   Name:	msi_console_log	- Log MSI Events to Console Terminal
 *
 *   Abstract:	This routine logs MSI events to the console terminal.  The
 *		event is always one of the following types:
 *
 *		PATH_EVENT	- Path specific event
 *		RPORT_EVENT	- Remote port specific event
 *		LPORT_EVENT	- Local port specific event
 *
 *		Explicit formatting information must be provided for each
 *		event.  This requires updating of the following tables each
 *		time a new event is defined:
 *
 *		1. The appropriate entry within the MSI console logging table(
 *		   msi_cltab[][] ) must be updated to reflect the new maximum
 *		   code represented within the associated format table.
 *
 *		2. The associated format table itself( msi_cli[], msi_clw[],
 *		   msi_clre[], msi_cle[], msi_clse[], msi_clfe[],
 *		   msi_clppdse[] ) must be updated with both the class of
 *		   variable information and exact text to be displayed.
 *		   However, the appropriate table should be updated with a NULL
 *		   entry whenever the MSI port driver is specifically NOT to
 *		   log a new event.  This applies only to msi_clppdse[] when a
 *		   new CI PPD severe error event is to be specifically logged
 *		   only by the CI PPD and not by appropriate port drivers such
 *		   as the MSI port driver.
 *
 *		NOTE: Console logging of events is bypassed whenever the event
 *		      severity does not warrant console logging according to
 *		      the current MSI severity level( msi_severity ).  Such
 *		      bypassing is overridden when the ECLAWAYS bit is set in
 *		      the event code indicating that the event is always to be
 *		      logged regardless of the current severity level.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      MSI port driver with the exception of those CI PPD severe
 *		      error events which are candidates for application of the
 *		      local port crash severity modifier( ESM_LPC ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   msi_cltab			- MSI console logging table
 *   msi_severity		- MSI console logging severity level
 *   msicmdblkp			- Address of MSI command block( OPTIONAL )
 *   msipp			- Address of MSI packet( OPTIONAL )
 *   event			- Event code
 *   event_type			- PATH_EVENT, LPORT_EVENT, RPORT_EVENT
 *   pb				- Path Block pointer( OPTIONAL )
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    errlogopt.portnum	-   Remote port station address
 *				     ( required ONLY when PB not provided )
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
msi_console_log( pccb, pb, msicmdblkp, msipp, event, event_type )
    register PCCB	*pccb;
    register PB		*pb;
    register MSICMDBLK	*msicmdblkp;
    MSIPACKET		*msipp;
    register u_long	event;
    u_long		event_type;
{
    register u_long	fcode, severity = Eseverity( event );

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
     * 1. Remote MSI port station address.
     * 2. Local MSI port station address.
     * 3. MSI port registers.
     * 4. MSI packet fields.
     *
     * Certain events may also be logged without displaying any variable
     * information.
     *
     * Logging of MSI packet fields( Class 4 ) may also include logging of
     * selected SIIBUF command block fields in the case of certain specific
     * events.
     *
     * A panic occurs whenever the MSI port driver is not prepared to log the
     * event due to detection of any of the following circumstances:
     *
     * 1. The event type is unknown.
     * 2. The event is a SCS specific event.
     * 3. The severity level of the event is invalid.
     * 4. The code of the event exceeds the current maximum known code for the
     *	  class( MSI or CI PPD ) and severity level of the event.
     * 5. The event is not represented within the appropriate console logging
     *    formatting table( indicating that the MSI port driver should never
     *	  have been asked to log it in the first place ).
     * 6. The class of variable information associated with the event is
     *	  unknown.
     *
     * None of these circumstances should ever occur.
     *
     * NOTE: Events represented within console logging format tables by NULL
     *	     entries are events which are NOT to be logged by the MSI port
     *	     driver.  This routine should never be invoked to console log such
     *	     events.  Currently, only certain path specific CI PPD severe error
     *	     events fall into this category.
     *
     * NOTE: All local port specific MSI fatal errors are currently fully
     *	     logged by msi_log_initerr().  This routine is never invoked either
     *	     to validate such events or to optionally log them to the console.
     */
    if(( event_type < PATH_EVENT || event_type > LPORT_EVENT ) ||
	 Test_scs_event( event )			       ||
	 severity > ES_SE				       ||
	 Ecode( event ) > Clog_maxcode( msi_cltab, event )     ||
	 Clog_tabmsg( msi_cltab, event ) == NULL ) {
	( void )panic( PANIC_UNKCODE );
    } else if(( fcode = Clog_tabcode( msi_cltab, event )) &&
	      ( fcode < CF_RPORT || fcode > CF_PKT2 )) {
	( void )panic( PANIC_UNKCF );
    } else if( msi_severity > severity && !Test_cloverride( event )) {
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
		( void )cprintf( "( local/remote port: %u/%u, remote ",
				 Scaaddr_lob( pccb->lpinfo.addr ),
				 (( pb ) ? Scaaddr_lob( pb->pinfo.rport_addr )
					 : pccb->Errlogopt.portnum ));
		if( pb && pb->sb ) {
		    ( void )cprintf( "system: %8s )\n\t- ",
				     pb->sb->sinfo.node_name );
		} else {
		    ( void )cprintf( "system: ? )\n\t- " );
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
					 Scaaddr_lob( pccb->lpinfo.addr ));
			break;

		    case ES_SE:
			( void )cprintf( "severe error( local port %u )\n\t- ",
					 Scaaddr_lob( pccb->lpinfo.addr ));
			break;

		    case ES_FE:
			( void )cprintf( "fatal error( local port %u )\n\t- ",
					 Scaaddr_lob( pccb->lpinfo.addr ));
			break;
		}
	    }
	    break;
    }
    ( void )cprintf( "%s", Clog_tabmsg( msi_cltab, event ));
    switch( fcode ) {

	case CF_NONE:
	    ( void )cprintf( "\n" );
	    break;

	case CF_RPORT:
	    ( void )cprintf( "( remote port: %u )\n",
			     (( pb ) ? Scaaddr_lob( pb->pinfo.rport_addr )
				     : pccb->Errlogopt.portnum ));
	    break;

	case CF_LPORT:
	    ( void )cprintf( "( local port %u )\n",
			     Scaaddr_lob( pccb->lpinfo.addr ));
	    break;

	case CF_REGS:
	    ( void )cprintf( "\n\t- dscr/dssr/dstat: 0x%04x/0x%04x/0x%04x\n",
			     ( u_long )*pccb->Msidscr,
			     ( u_long )*pccb->Msidssr,
			     ( u_long )*pccb->Msidstat );
	    break;

	case CF_PKT:
	    ( void )cprintf( "\n\t- opcode/flags: ");
	    if( msipp ) {
		( void )cprintf( "0x%02x/0x%02x\n",
				 ( u_long )msipp->ph.opcode,
				 ( u_long )*( u_char * )&msipp->ph.flags );
	    } else {
		( void )cprintf( "0x??/0x??\n" );
	    }
	    break;

	case CF_PKT2:
	    ( void )cprintf( "\n\t- status/dst/src/opcode/flags: ");
	    if( msicmdblkp ) {
		( void )cprintf( "0x%04x/0x%02x/0x%02x/",
				 ( u_long )msicmdblkp->status,
				 ( u_long )msicmdblkp->dst,
				 ( u_long )msicmdblkp->src );
	    } else {
		( void )cprintf( "0x????/0x??/0x??/" );
	    }
	    if( msipp ) {
		( void )cprintf( "0x%02x/0x%02x\n",
				 ( u_long )msipp->ph.opcode,
				 ( u_long )*( u_char * )&msipp->ph.flags );
	    } else {
		( void )cprintf( "0x??/0x??\n" );
	    }
	    break;
    }
}

/*   Name:	msi_disable	- Disable Local MSI Port
 *
 *   Abstract:	This routine completely disables a local MSI port.  There are
 *		three occasions when this routine is invoked:
 *
 *		1. Prior to the initial initialization of a local MSI port.
 *		2. During crashing of a local MSI port.
 *		3. During disablement of a local MSI port as part of system
 *		   shutdown.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.active	-   0
 *	    siiregptrs		-   MSI register pointers
 *		msicomm		-    SII command register 
 *
 *   SMP:	No locks are required.  However exclusive access to
 *	 	lpstatus.active, a MSI specific PCCB field, must be guaranteed
 *		EXTERNALLY.  It may be guaranteed by locking the PCCB specific
 *		XFP and RFP or by guaranteeing that only the processor
 *		executing this routine has access to it.
 */
void
msi_disable( pccb )
    register PCCB	*pccb;
{
    /* The steps involved in disabling a local MSI port are as follows:
     *
     * 1. Reset the SII chip.
     * 2. Mark the local MSI port inactive.
     *
     * Resetting the SII chip( Step 1 ) stops any operation in progress,
     * disconnects the chip from the DSSI bus, returns all registers to their
     * default values, and inhibits all access to the SII RAM buffer.  Marking
     * the local MSI port inactive( Step 2 ) disables all future processing by
     * scheduled XFP, RFP and XFP_TIMER threads.  The end result of both these
     * steps is to drive the local MSI port into a completely quiescent state
     * allowing for full clean up and eventual local port re-initialization(
     * except of course in the case where disablement was requested during
     * system shutdown ).
     */
    *pccb->Msicomm = SIICOM_SIIRESET;
    pccb->Lpstatus.active = 0;
}

/*   Name:	msi_log_badport	- Log Bad Port Numbers in MSI Packets
 *
 *   Abstract:	This routine logs bad port numbers in MSI packets.  For a port
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
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    errlogopt.portnum	-   Remote port station address
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) as required by
 *		msi_log_packet(), the routine which performs the actual event
 *		logging.
 */
void
msi_log_badport( pccb, cippdbp )
    PCCB		*pccb;
    CIPPDH		*cippdbp;
{
    register MSIB	*msibp = Ppd_to_pd( pccb, cippdbp );

    pccb->Errlogopt.portnum = msibp->Rport_addr;
    ( void )msi_log_packet( pccb,
			    NULL,
			    NULL,
			    &msibp->Ph,
			    msibp->Framelength,
			    SE_BADPORTNUM,
			    LPORT_EVENT );
}

/*   Name:	msi_log_devattn	- Log MSI Device Attention Events
 *
 *   Abstract:	This routine logs MSI device attention events.  Such events are
 *		detected directly from a specific local MSI port as opposed to
 *		those events ascertained indirectly from a MSI port packet.
 *		The event is also optionally logged to the console.
 *
 *		Two classes of events are currently logged by this routine:
 *
 *		1. Explicit hardware errors.
 *		2. Local port initializations.
 *
 *		Many of these events represent serious errors and are logged to
 *		save relevant information before drastic steps are attempted to
 *		resolve them.  Others are less serious and are logged only to
 *		give a warning or for informational purposes only.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      MSI port driver.  It currently does NOT even log those CI
 *		      PPD events which are candidates for application of the
 *		      local port crash severity modifier( ESM_LPC ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   event			- Event code
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    save_dssr		-   Cached DSSI status register contents
 *	    save_dstat		-   Cached data transfer status reg contents
 *   regs			- LOG_REGS or LOG_NOREGS
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.nerrs		-  Number of errors on port
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access and as
 *		required by msi_console_log(), the routine responsible for
 *		logging events to the console terminal.
 */
void
msi_log_devattn( pccb, event, regs )
    register PCCB		*pccb;
    register u_long		event;
    u_long			regs;
{
    register u_long		size;
    register u_char		*celp;
    register struct el_rec	*elp;
    register struct sii_regs	*siiregs = ( struct sii_regs * )pccb->Siiregs;

    /* The steps involved in logging device attention events include:
     *
     * 1. Logging the event to the console.
     * 2. Incrementing the counter of local port errors.
     * 3. Computing the size of the application portion of the event log
     *	  record.
     * 4. Allocating an event log record and initializing the record's sub id
     *	  packet fields.
     * 5. Initializing the portion of the record common to all MSI events.
     * 6. Initializing the portion of the record reserved for register
     *    contents.
     * 7. Validating the event log record.
     *
     * The ability of this routine to log the event is validated during console
     * logging( Step 1 ).  A panic occurs whenever the MSI port driver is not
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
     * register contents only for those that do.
     */
    ( void )msi_console_log( pccb, NULL, NULL, NULL, event, LPORT_EVENT );
    if( Eseverity( event ) >= ES_E ) {
	Event_counter( pccb->lpinfo.nerrs )
    }
    size = sizeof( struct msi_common );
    if( regs == LOG_REGS ) {
	size += sizeof( struct msi_regs );
    }
    if(( elp = ealloc( size, EL_PRIHIGH )) == EL_FULL ) {
	return;
    }
    LSUBID( elp,
	    ELCT_DCNTL,
	    ELMSI_ATTN,
	    pccb->lpinfo.type.hwtype,
	    Ctrl_from_name( pccb->lpinfo.name ),
	    EL_UNDEF,
	    event )
    Elmsicommon( elp )->msi_optfmask1 = 0;
    Elmsicommon( elp )->msi_optfmask2 = 0;
    Elmsicommon( elp )->msi_evpktver = MSI_EVPKTVER;
    U_long( *Elmsicommon( elp )->msi_lpname ) = pccb->lpinfo.name;
    Move_node( lscs.system.node_name, Elmsicommon( elp )->msi_lname )
    Move_scaaddr( lscs.system.sysid, *Elmsicommon( elp )->msi_lsysid )
    Move_scaaddr( pccb->lpinfo.addr, *Elmsicommon( elp )->msi_lsaddr )
    Elmsicommon( elp )->msi_nerrs = pccb->lpinfo.nerrs;
    Elmsicommon( elp )->msi_nreinits = pccb->lpinfo.nreinits;
    if( regs == LOG_REGS ) {
	celp = ( u_char * )Elmsidattn( elp );
	Elmsicommon( elp )->msi_optfmask1 |= MSI_REGS;
	Elmsiregs( celp )->msi_csr = *pccb->Msicsr;
	Elmsiregs( celp )->msi_idr = *pccb->Msiidr;
	Elmsiregs( celp )->msi_slcs = ( u_short )siiregs->sii_msislcs;
	Elmsiregs( celp )->msi_destat = ( u_short )siiregs->sii_msidestat;
	Elmsiregs( celp )->msi_tr = *pccb->Msitr;
	Elmsiregs( celp )->msi_dmctlr = ( u_short )siiregs->sii_msidmctlr;
	Elmsiregs( celp )->msi_dmlotc = ( u_short )siiregs->sii_msidmlotc;
	Elmsiregs( celp )->msi_dmaaddrl = ( u_short )siiregs->sii_msidmaddrl;
	Elmsiregs( celp )->msi_dmaaddrh = ( u_short )siiregs->sii_msidmaddrh;
	Elmsiregs( celp )->msi_stlp = ( u_short )siiregs->sii_msistlp;
	Elmsiregs( celp )->msi_tlp = *pccb->Msitlp;
	Elmsiregs( celp )->msi_ilp = *pccb->Msiilp;
	Elmsiregs( celp )->msi_dscr = *pccb->Msidscr;
	Elmsiregs( celp )->msi_dssr = *pccb->Msidssr;
	Elmsiregs( celp )->msi_dstat = *pccb->Msidstat;
	Elmsiregs( celp )->msi_dcr = *pccb->Msidcr;
	Elmsiregs( celp )->msi_save_dssr = pccb->Save_dssr;
	Elmsiregs( celp )->msi_save_dstat = pccb->Save_dstat;
    }
    EVALID( elp )
}

/*   Name:	msi_log_initerr	- Log MSI Port Initialization Fatal Errors
 *
 *   Abstract:	This routine logs a special type of MSI device attention event:
 *		software errors detected during probing of local MSI ports.
 *		These fatal error events are logged as device attentions
 *		because they pertain to a specific local MSI port.  However,
 *		they are considered special because they pre-date allocation of
 *		a PCCB for the local port, and therefore, may not make use of
 *		it for event logging purposes.  The following special events
 *		are currently defined:
 *
 *		1. FE_INIT_ZEROID  - Uninitialized system identification num
 *		2. FE_INIT_NOMEM   - Insufficient dynamic memory
 *		3. FE_INIT_NOPTES  - Insufficient ptes for double mapping
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
 *		time a new MSI fatal error event is defined:
 *
 *		1. The MSI fatal error event table( msi_clfe[]) must be updated
 *		   with the exact text to be displayed and the console logging
 *		   format code CF_NONE( Console logged MSI fatal error events
 *		   currently never display variable information ).
 *
 *		2. The fatal error event entry within the MSI console logging
 *		   table( msi_cltab[][] ) must be updated to reflect the new
 *		   maximum fatal error event code.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   hpt			- Hardware port type
 *   msi_cltab			- MSI console logging table
 *   msinum			- MSI adapter number
 *   event			- Fatal error event code
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
msi_log_initerr( msinum, hpt, event )
    u_long			msinum;
    u_long			hpt;
    register u_long		event;
{
    register struct el_rec	*elp;

    /* Logging of fatal errors detected during MSI port probings proceeds as
     * follows:
     *
     * 1. The fatal error is validated.
     * 2. The fatal error event and a local port permanently offline message
     *	  are both printed on the console.
     * 3. An event log record is allocated and the record's sub id packet
     *	  fields are initialized.
     * 4. The portion of record common to all MSI events is initialized.
     * 5. The event log record is validated.
     *
     * This routine panics if validation of the fatal error event( Step 1 )
     * fails indicating inability of the MSI port driver to log the reported
     * event.
     *
     * Event logging is bypassed on failures to allocate an event log record(
     * Step 3 ).
     *
     * Note that no other MSI specific information common to device attention
     * events is logged and that no variable information is displayed within
     * logged console messages.
     */
    switch( event ) {

	case FE_INIT_ZEROID:
	case FE_INIT_NOMEM:
	case FE_INIT_NOPTES:
	    break;

	default:
	    ( void )panic( PANIC_UNKCODE );
    }
    ( void )cprintf( "msi%u\t- fatal error( local port ? )\n\t- %s\n",
		     msinum, Clog_tabmsg( msi_cltab, event ));
    ( void )cprintf( "msi%u\t- permanently offline( local port ? )\n", msinum);
    if(( elp = ealloc( sizeof( struct msi_common ), EL_PRIHIGH )) == EL_FULL ){
	LSUBID( elp, ELCT_DCNTL, ELMSI_ATTN, hpt, msinum, EL_UNDEF, event )
	Elmsicommon( elp )->msi_optfmask1 = 0;
	Elmsicommon( elp )->msi_optfmask2 = 0;
	Elmsicommon( elp )->msi_evpktver = MSI_EVPKTVER;
	U_long( *Elmsicommon( elp )->msi_lpname )
			= Ctrl_from_num( "msi  ", msinum );
	Move_node( scs_node_name, Elmsicommon( elp )->msi_lname )
	Move_scaaddr( scs_system_id, *Elmsicommon( elp )->msi_lsysid )
	U_short( Elmsicommon( elp )->msi_lsaddr[ 0 ]) = EL_UNDEF;
	U_short( Elmsicommon( elp )->msi_lsaddr[ 2 ]) = EL_UNDEF;
	U_short( Elmsicommon( elp )->msi_lsaddr[ 4 ]) = EL_UNDEF;
	Elmsicommon( elp )->msi_nerrs = 1;
	Elmsicommon( elp )->msi_nreinits = 0;
	EVALID( elp )
    }
}

/*   Name:	msi_log_packet	- Log MSI Packet Related Events
 *
 *   Abstract:	This routine logs MSI packet related events.  Such events are
 *		ascertained indirectly from a MSI port packet as opposed to
 *		those events detected directly from a specific local MSI port.
 *		The event is also optionally logged to the console.
 *
 *		Five classes of events are currently logged by this routine:
 *
 *		1. Software detected errors during packet reception.
 *		2. Software detected invalid remote port states.
 *		3. Software detected errors during packet transmission. 
 *		4. Software detected invalid MSI remote port station addresses.
 *		5. Reception of packets over software non-existent paths.
 *
 *		Many of these events represent serious errors and are logged to
 *		save relevant information before drastic steps are attempted to
 *		resolve them.  Others are less serious and are logged only to
 *		give a warning or for informational purposes only.
 *
 *		NOTE: While all events logged therein arise indirectly from MSI
 *		      port packets, the logging of each event does not
 *		      necessarily involve logging of the packet itself.
 *
 *		NOTE: This routine does NOT log events arising external to the
 *		      MSI port driver with the exception of those CI PPD events
 *		      which are candidates for application of the local port
 *		      crash severity modifier( ESM_LPC ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   event			- Event code
 *   event_type			- PATH_EVENT, LPORT_EVENT, RPORT_EVENT
 *   lscs			- Local system permanent information
 *   msicmdblkp			- Address of MSI command block( OPTIONAL )
 *   msipp			- Address of MSI packet( OPTIONAL )
 *   pb				- Path Block pointer( OPTIONAL )
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    errlogopt.portnum	-   Remote port station address
 *				     ( required ONLY when PB not provided )
 *   pktsize			- Size of MSI packet( OPTIONAL )
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.nerrs		-  Number of errors on port
 *
 *   SMP:	The PCCB is locked( EXTERNALLY ) to synchronize access, to
 *		prevent premature PB deletion when a PB is provided, and as
 *		required by msi_console_log(), the routine responsible for
 *		logging events to the console terminal.
 *
 *		PBs do NOT require locking when provided because only static
 *		fields are accessed.  SBs NEVER require locking.
 */
void
msi_log_packet( pccb, pb, msicmdblkp, msipp, pktsize, event, event_type )
    register PCCB		*pccb;
    register PB			*pb;
    MSICMDBLK  			*msicmdblkp;
    MSIPACKET			*msipp;
    u_long			pktsize;
    u_long			event;
    u_long			event_type;
{
    register u_char		*celp;
    register struct el_rec	*elp;
    register u_long		opt_size;

    /* The steps involved in logging MSI packet related events include:
     *
     * 1. Logging the event to the console.
     * 2. Incrementing the counter of local port errors.
     * 3. Computing the size of the application portion of the event log
     *	  record.
     * 4. Allocating an event log record and initializing the record's sub id
     *	  packet fields.
     * 5. Initializing the portion of the record common to all MSI events.
     * 6. Initializing the portion of the record common to all MSI packet
     *	  related events.
     * 7. Moving any optional logged packet information into the record.  
     * 8. Validating the event log record.
     *
     * The ability of this routine to log the event is validated during console
     * logging( Step 1 ).  A panic occurs whenever the MSI port driver is not
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
     * included within the record( Step 7 ).  The following types of orthogonal
     * optional information may be logged:
     *
     * 1. The MSI port packet responsible for the logged packet related event.
     * 2. The MSI command block associated with the responsible port packet.
     *
     * Sufficient space is reserved within the event record for such optional
     * information only for those events which request their logging.  The
     * optional MSI port packets are associated with many different events and
     * vary widely in size.  Such packets require truncation whenever logging
     * their full size would exceed the maximum size of an event log record.
     *
     * NOTE: An event may request logging of both a MSI port packet and an
     *	     associated command block, just a MSI port packet, or neither.  No
     *	     event currently ever requests logging of a MSI command block
     *	     without also requesting logging of a MSI port packet.
     */
    ( void )msi_console_log( pccb, pb, msicmdblkp, msipp, event, event_type );
    if( Eseverity( event ) >= ES_E ) {
	Event_counter( pccb->lpinfo.nerrs )
    }
    {
    register u_long		size = sizeof( struct msi_common ) +
				       sizeof( struct msi_lcommon );

    if( msicmdblkp ) {
	size += sizeof( struct msi_cmdblk );
    }
    if( msipp ) {
	opt_size = pktsize;
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
	    ELMSI_LPKT,
	    pccb->lpinfo.type.hwtype,
	    Ctrl_from_name( pccb->lpinfo.name ),
	    EL_UNDEF,
	    event )
    }
    Elmsicommon( elp )->msi_optfmask1 = MSI_LCOMMON;
    Elmsicommon( elp )->msi_optfmask2 = 0;
    Elmsicommon( elp )->msi_evpktver = MSI_EVPKTVER;
    U_long( *Elmsicommon( elp )->msi_lpname ) = pccb->lpinfo.name;
    Move_node( lscs.system.node_name, Elmsicommon( elp )->msi_lname )
    Move_scaaddr( lscs.system.sysid, *Elmsicommon( elp )->msi_lsysid )
    Move_scaaddr( pccb->lpinfo.addr, *Elmsicommon( elp )->msi_lsaddr )
    Elmsicommon( elp )->msi_nerrs = pccb->lpinfo.nerrs;
    Elmsicommon( elp )->msi_nreinits = pccb->lpinfo.nreinits;
    {
    register SB		*sb;

    if( pb ) {
	sb = pb->sb;
	Move_scaaddr( pb->pinfo.rport_addr, *Elmsilcommon( elp )->msi_rsaddr )
    } else {
	sb = NULL;
	if(( U_short( Elmsilcommon( elp )->msi_rsaddr[ 0 ])
		= pccb->Errlogopt.portnum ) != EL_UNDEF ) {
	    U_short( Elmsilcommon( elp )->msi_rsaddr[ 2 ]) = 0;
	    U_short( Elmsilcommon( elp )->msi_rsaddr[ 4 ]) = 0;
	} else {
	    U_short( Elmsilcommon( elp )->msi_rsaddr[ 2 ]) = EL_UNDEF;
	    U_short( Elmsilcommon( elp )->msi_rsaddr[ 4 ]) = EL_UNDEF;
	}
    }
    if( sb ) {
	Move_scaaddr( sb->sinfo.sysid, *Elmsilcommon( elp )->msi_rsysid )
	Move_node( sb->sinfo.node_name, Elmsilcommon( elp )->msi_rname )
    } else {
	U_short( Elmsilcommon( elp )->msi_rsysid[ 0 ])  = EL_UNDEF;
	U_short( Elmsilcommon( elp )->msi_rsysid[ 2 ])  = EL_UNDEF;
	U_short( Elmsilcommon( elp )->msi_rsysid[ 4 ])  = EL_UNDEF;
	U_long( Elmsilcommon( elp )->msi_rname[ 0 ]) = EL_UNDEF;
	U_long( Elmsilcommon( elp )->msi_rname[ 4 ]) = EL_UNDEF;
    }
    }
    celp = ( u_char * )Elmsilcommon( elp ) + sizeof( struct msi_lcommon );
    if( msicmdblkp ) {
	Elmsicommon( elp )->msi_optfmask1 |= MSI_CMDBLK;
	( void )bcopy(( u_char * )msicmdblkp,
		      celp,
		      sizeof( struct msi_cmdblk ));
	celp += sizeof( struct msi_cmdblk );
    }
    if( msipp ) {
	Elmsicommon( elp )->msi_optfmask1 |= MSI_PACKET;
	( void )bcopy(( u_char * )msipp, celp, opt_size );
    }
    EVALID( elp )
}
