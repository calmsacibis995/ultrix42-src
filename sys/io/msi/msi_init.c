#ifndef	lint
static char *sccsid = "@(#)msi_init.c	4.1	(ULTRIX)	7/2/90";
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
 *		Driver( MSI ) initialization routines and functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 07, 1988
 *
 *   Function/Routines:
 *
 *   msi_init_port		Initialize a Local MSI Port
 *   msi_probe			Probe a Local MSI Port
 *   msi_start_port		Start a Local MSI Port
 *
 *   Modification History:
 *
 *   28-Dec-89		Robin
 *	Removed the cpu specific code on the 5400 and made it a ifdef mips.
 *
 *   09-Nov-1989	David E. Eiche		DEE0080
 *	Add code to msi_probe() to store the new software port type and
 *	interconnect type fields in the LPIB.
 *
 *   9-Jul-1989         Stephen W. Bailey       SWBXXXX
 *      Added initialization for pccb->Msiisr3 (the SII Main Control
 *      Diagnostic Register) necessary to fix race conditions with ILP,
 *      TLP and packet STATUS word for packets in the IL.
 *
 *   29-Jun-1989	Todd M. Katz		TMK0001
 *	*TEMP* MIPSfair II debugging code. Allocate dynamic memory for caching
 * 	SII RAM buffer contents during panics.
 *
 *   14-Jun-1989	Pete Keilty
 *	Add include file smp_lock.h
 *
 *   10-Jun-1989	Pete Keilty
 *	Add WBFLUSH, MSI_RPROTOPTE, MSI_XPROTOPTE macro's for MIPS cpu.
 *	Add Splscs() macro for raise ipl to IPL_SCS.
 *	Modifed msi_start_port for MIPS cpu's.	
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/dyntypes.h"
#include		"../h/param.h"
#include		"../h/time.h"
#include		"../h/kmalloc.h"
#include		"../h/ksched.h"
#include		"../h/errlog.h"
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#include		"../h/smp_lock.h"
#include		"../machine/pte.h"
#include		"../../machine/common/cpuconf.h"
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
#include		"../io/scs/scamachmac.h"

/* External Variables and Routines.
 */
extern	struct timeval	time;
extern	PDT		msi_pdt;
extern	PCCB		*msi_adapt[];
extern	pccbq		scs_lport_db;
extern	caddr_t		get_sys_ptes();
extern	u_short		msi_cippdburst,
			msi_cippdcontct;
extern	void		msi_dealloc_pkt(),
			msi_disable(),
			msi_init_port(),
			msi_log_devattn(),
			msi_log_initerr(),
			msi_start_port();
extern	u_long		scs_dg_size,
			scs_msg_size,
			gvp_initialize(),
			scs_initialize();
extern int		cpu;

/* Temporary Mipsfair 1 debug code */
#ifdef notdef
extern u_char	*siirambuf;
#endif notdef

/*   Name:	msi_init_port	- Initialize a Local MSI Port
 *
 *   Abstract:	This routine directs the initialization of a local MSI port.
 *		It also oversees the port's initial initialization.  It is
 *		always invoked by forking to it.
 *
 *		The local port must be completely disabled whenever execution
 *		of this routine is scheduled including disablement of:
 *
 *		1. Port interrupts.
 *		2. XFP, RFP, and XFP_TIMER activity.
 *		3. CI PPD finite state machine activity on the local port.
 *
 *		The local MSI port SII chip must also have been reset.
 *
 *		NOTE: This is a mandatory PD function( Init_port ) for use by
 *		      the CI PPD.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   msi_cippdburst		- CI PPD port polling burst size
 *   msi_cippdcontct		- CI PPD port contact interval( in seconds )
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.active	-   0
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.cleanup	-   1
 *	    fsmstatus.fkip	-   1
 *	    fsmstatus.online	-   0
 *   time			- Current time of day
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.addr		-  Local port station address
 *	lpinfo.nreinits		-  Number of local port re-initializations
 *	pd.msi			-  MSI specific PCCB fields
 *	    comqh		-   MSIB high priority command queue
 *	    comql		-   MSIB low priority command queue
 *	    dfreeq		-   MSIB datagram free queue
 *	    lpidinfo		-   Local port id information( INITIALIZED )
 *	    lpstatus.init	-   0
 *	    mfreeq		-   MSIB message free queue
 *	    randomseed		-   Random number generator seed
 *	    perport		-   Per-DSSI port information
 *		rcounters	-    Receive counters( INITIALIZED )
 *		rpstatus.dip	-    0
 *		rpstatus.path	-    0
 *		rpstatus.vc	-    0
 *		rseqno		-    0
 *		xcounters	-    Transmit counters( INITIALIZED )
 *		xretries	-    0
 *		xretryq		-    MSIB transmit retry queue
 *		xretry_timer	-    0
 *		xseqno		-    0
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    burst		-   Port polling burst size
 *	    contact		-   Port polling contact frequency
 *	    fsmstatus.cleanup	-   0
 *	    fsmstatus.fkip	-   0
 *
 *   SMP:	The PCCB and PCCB specific XFP, RFP, COMQH, COMQL, DFREEQ, and
 *		MFREEQ are locked to synchronize access.  Any locking done by
 *		this routine is probably unnecessary.  This is because of lack
 *		of conflict for all lock controlled data structures and
 *		variables due to the single threadedness of port clean up and
 *		initialization.  It is done anyway to guarantee unrestricted
 *		access and because stale CI PPD interval timer, RFP, XFP, or
 *		XFP_TIMER threads could still be scheduled or even active.
 *		PCCB addresses are always valid because these data structures
 *		are never deleted.
 */
void
msi_init_port( pccb )
    register PCCB		*pccb;
{
    register MSI_PPORTINFO	*ppi;
    register u_long		init_evcode, num;
    register u_long		save_ipl = Splscs();

    /* The steps involved in local MSI port initialization are as follows:
     *
     *  1. The PCCB, RFP, and XFP are locked.
     *  2. Port clean up is marked completed.
     *
     *  - PORT RE-INITIALIZATION ONLY:
     *  3. CI PPD port polling burst size and port polling contact frequency
     *	   values are reset.
     *  4. All port queues are flushed and all flushed packets are deallocated.
     *
     *  - ALL INITIALIZATIONS:
     *  5. SII chip target and initiator timeouts are enabled and timer values
     *	   set.
     *  6. SII chip DSSI bus drivers are turned on.
     *  7. DSSI mode operation of the SII chip is enabled.
     *  8. SII chip interrupts, parity error reporting, and receptions are
     *	   enabled.
     *  9. Local port SII RAM buffer is subdivided into transmit and receive
     *	   SIIBUFs and each of them is appropriately initialized.
     * 10. SII chip is handed its initial receive buffer list.
     * 11. The local MSI port is marked active.
     *
     * - INITIAL PORT INITIALIZATION ONLY:
     * 12. The local port address is retrieved.
     * 13. The random transmit delay interval generator is initialized.
     * 14. The identification information for the local port is initialized.
     *
     *  - ALL INITIALIZATIONS:
     * 15. Initialization of the local MSI port is logged.
     * 16. The XFP, RFP, and PCCB are unlocked.
     * 17. The CI PPD is notified of successful port initialization.
     *
     * Local port CI PPD port polling burst size and contact frequency are
     * reset( Step 3 ) using configuration variables.  This allows their values
     * to change following failure and re-initialization of a local port.
     *
     * All MSIB port queues containing dynammically allocated host memory are
     * flushed immediately prior to port re-initialization( Step 4 ).  This
     * action insures their pristine state and recovers the dynamic memory.
     * Logically this action should occur as part of port clean up.  However,
     * flushing port queues specifically at this time does not prevent
     * subsequent packet queueing.  Nothing can prevent SYSAP, SCS, CI PPD, or
     * even MSI Receive Fork Process( RFP ) motivated insertion of packets onto
     * port queues without seriously affecting performance until all paths
     * associated with the failed port have been terminated.  This condition is
     * not reached until the virtual end of port clean up.  This is why port
     * queues are flushed during port re-initialization instead of during port
     * clean up.  It is just easier and more straightforward to insure their
     * pristine state if packet flushing is done at this time.
     *
     * The SII chip allows its port station address to be either programmed in
     * or specified by on-board jumper settings.  The MSI port driver makes use
     * of the latter option to avoid addressing conflicts with other remote
     * ports on the DSSI bus.  During startup of the local MSI port( Steps 6-10
     * ) the SII chip was told to use the on-board jumper settings to specify
     * its port station address.  This address is retrived in Step 12 and used
     * as the local port station address.
     */
    Lock_pccb( pccb )
    Pccb_fork_done( pccb, PANIC_PCCBFB )
    Lock_rfp( pccb )
    Lock_xfp( pccb )
    pccb->Fsmstatus.cleanup = 0;
    if( !pccb->Lpstatus.init ) {
	++pccb->lpinfo.nreinits;
	pccb->Contact = msi_cippdcontct;
	pccb->Burst = msi_cippdburst;
	Flush_comqh( pccb )
	Flush_comql( pccb )
	Flush_dfreeq( pccb )
	Flush_mfreeq( pccb )
	for( num = 0, ppi = &pccb->Perport[ 0 ];
	     num < MSI_MAXNUM_PORT;
	     ++num, ++ppi ){
	    Init_portinfo( ppi )
	}
    }
    ( void )msi_start_port( pccb );
    if( pccb->Lpstatus.init ) {
	pccb->Lpstatus.init = 0;
	init_evcode = I_LPORT_INIT;
	Scaaddr_lob( pccb->lpinfo.addr ) = ( *pccb->Msiidr & MSIIDR_BUSID );
	Xinit_generator( pccb )
	Init_lpdinfo( pccb )
    } else {
	init_evcode = I_LPORT_REINIT;
    }
    ( void )msi_log_devattn( pccb, init_evcode, LOG_NOREGS );
    Unlock_xfp( pccb )
    Unlock_rfp( pccb )
    Unlock_pccb( pccb )
    ( void )cippd_start( pccb );
    ( void )splx( save_ipl );
}

/*   Name:	msi_probe	- Probe a Local MSI Port
 *
 *   Abstract:	This routine probes a newly discovered MSI port culminating in
 *		its first initialization.  Any encountered errors abort the MSI
 *		port probing.
 *
 *		NOTE: The MSI port driver NEVER requires sanity checks to be
 *		      made on its local ports.  It always initializes the PCCB
 *		      specific finite state machine status bit "nosanity_chk"
 *		      so that the CI PPD skips such checks.
 *
 *   Inputs:
 *
 *   0				- Interrupt processor level
 *   msi_cippdburst		- CI PPD port polling burst size
 *   msi_cippdcontct		- CI PPD port contact interval( in seconds )
 *   msi_pdt			- MSI Port Dispatch Table
 *   msibuf			- Adapter I/O space 128K RAM buffer address
 *   msinum			- MSI adapter number
 *   msi_adapt			- Vector of MSI Adapter Control Blocks
 *   msiregs			- Adapter I/O space register addresses
 *   scs_dg_size		- Maximum application datagram size
 *   scs_lport_db		- System-wide local port database queue head
 *   scs_msg_size		- Maximum application message size
 *
 *   Outputs:
 *
 *   0				- Interrupt processor level
 *   msi_adapt			- Vector of MSI Adapter Control Block
 *   pccb			- Port Command and Control Block pointer
 *				   ( INITIALIZED as required )
 *   pccb			- Port Command and Control Block pointer
 *				   ( INITIALIZED as required )
 *   scs_lport_db		- System-wide local port database queue head
 *
 *   Return Values:
 *
 *   1 if probing is successful; otherwise, 0
 *
 *   SMP:	No locks are required even though the PCCB, PCCB specifc XFP
 *		and SCA database are manipulated.  This function is only called
 *		during system initialization and at that time only the
 *		processor executing this code is operational.  This guarantees
 *		uncompromised access to all data structures without locking the
 *		PCCB, PCCB specific XFP, or the SCA database.
 *
 *		PCCB and PCCB specific XFP, RFP, COMQH, COMQL, DFREEQ, and
 *		MFREEQ lock structures are initialized.
 */
int
msi_probe( msinum, msiregs, msibuf )
    u_long			msinum;
    register struct sii_regs	*msiregs;
    struct siibuf		*msibuf;
{
    static   u_long		initialized = 0;
    register PCCB		*pccb;
    register u_long		save_ipl,
				status = 0;

    /* The steps involved in probing a MSI port are as follows:
     *
     *  1. IPL is synchronized to IPL_SCS.
     *  2. SCS is initialized.
     *  3. The Generic Vaxport Driver is initialized.
     *  4. A PCCB is allocated.
     *  5. Double mapping PTEs are allocated and initialized.
     *  6. The PCCB is initialized.
     *  7. The PCCB is inserted into the system-wide local port database.
     *  8. The MSI port is disabled.
     *  9. Initialization of the local MSI port is scheduled through forking.
     * 10. IPL is restored.
     *
     * Steps 2-3 constitute MSI port driver initialization and are only
     * executed during probing of the very first local MSI port encountered
     * during device auto-configuration.  The remaining steps constitute the
     * actual probing of the specified local MSI port and are executed whenever
     * this routine is invoked.  Any errors( Steps 2-5 ) encountered during
     * either portion of this routine immediately abort probing of the current
     * MSI port following logging of the fatal event.  The interrupt service
     * routine for the local MSI port is also switched back to the appropriate
     * stray interrupt handler.  Any subsequent interrupts on the inoperative
     * local port are discarded.
     *
     * Separate double mapping PTEs are allocated for use by the Transmit and
     * Receive Fork Processes( Step 5 ).  These PTEs are used whenever the fork
     * processes find it necessary to transfer large amounts of data to or from
     * non-System buffers.  Such buffers are NOT within the system address
     * space and therefore may not be directly accessed by fork processes.
     * Separate mapping PTEs are required because both fork processes maybe
     * simultaneously active and making use of their mapping PTEs in a SMP
     * environment.  Both sets of PTEs are pre-initialized to the extent that
     * only page frame numbers need be inserted during double mapping.
     *
     * NOTE: The PCCB fork block is only used for clean up and initialization
     *	     of the local port.  Therefore, it should always be available for
     *	     the port's first initialization.
     */
    save_ipl = Splscs();
    if( initialized == 0 ) {
	if(( status = scs_initialize()) == RET_SUCCESS &&
	   ( status = gvp_initialize()) == RET_SUCCESS ) {
	    status = 0;
	    initialized = 1;
	} else {
	    status = (( status == RET_ALLOCFAIL )
				? FE_INIT_NOMEM : FE_INIT_ZEROID );
	}
    }
    if( status == 0 ) {
	KM_ALLOC( pccb, PCCB *, sizeof( PCCB ), KM_SCA, KM_NOW_CL_CA )
	if( pccb ) {
	    msi_adapt[ msinum ] = pccb;
	} else {
	    status = FE_INIT_NOMEM;
	}
    } else {
	pccb = NULL;
    }
#ifdef notdef
/* *TEMP* MIPSfair II debugging code.  Allocate dynamic memory for caching
 * SII RAM buffer contents during panics.
 */
{
    if( status == 0 ) {
        KM_ALLOC( siirambuf, u_char *, sizeof( struct siibuf ), KM_SCA,
	      	  KM_NOW_CL_CA )
        if( siirambuf == NULL ) {
	    status = FE_INIT_NOMEM;
        }
    }
}
#endif notdef
    if( status == 0 ) {
	register u_long		nptes;

	nptes = rbtop( MAX_DATA_SIZE ) + 1;
	pccb->Rdmap.dmap_baddr = get_sys_ptes( nptes,
					       &pccb->Rdmap.dmap_bpteaddr );
	pccb->Xdmap.dmap_baddr = get_sys_ptes( nptes,
					       &pccb->Xdmap.dmap_bpteaddr );
	if( pccb->Rdmap.dmap_baddr && pccb->Xdmap.dmap_baddr ) {
	    pccb->Rdmap.protopte = MSI_RPROTOPTE;
	    pccb->Xdmap.protopte = MSI_XPROTOPTE;
	} else {
	    status = FE_INIT_NOPTES;
	}
    }
    if( status ) {
	if( pccb ) {
	    KM_FREE(( char * )pccb, KM_SCA )
	}
#ifdef notdef
/* *TEMP* MIPSfair II debugging code.  Allocate dynamic memory for caching
 * SII RAM buffer contents during panics.
 */
{
    if( siirambuf ) {
        KM_FREE(( char * )siirambuf, KM_SCA )
        siirambuf = NULL;
    }
}
#endif notdef
	( void )msi_log_initerr( msinum, HPT_SII, status );
	( void )splx( save_ipl );
	return( 0 );
    }
    U_long( pccb->size ) = sizeof( PCCB );
    pccb->type = DYN_PCCB;
    pccb->pdt = &msi_pdt;
    Init_pccb_lock( pccb )

    pccb->lpinfo.type.hwtype = HPT_SII;
    pccb->lpinfo.type.swtype = SPT_MSI;
    pccb->lpinfo.type.ictype = ICT_SII;
    pccb->lpinfo.name = Ctrl_from_num( "msi ", msinum );

    pccb->Fsmstatus.nosanity_chk = 1;
    pccb->Contact = msi_cippdcontct;
    pccb->Burst = msi_cippdburst;
    pccb->Max_cables = MAX_CABLES;

    pccb->lpinfo.Max_port = ( MSI_MAXNUM_PORT - 1 );
    pccb->lpinfo.Protocol = CIPPD_VERSION;

    Init_queue( pccb->Comqh )
    Init_queue( pccb->Comql )
    Init_queue( pccb->Dfreeq )
    Init_queue( pccb->Mfreeq )
    pccb->Siiregs = ( u_char * )msiregs;
    pccb->Siibuffer = ( u_char * )msibuf;
    pccb->Pkt_size = sizeof( MSIBH ) + sizeof( MSI_STRT );
    pccb->Msg_ovhd = MSG_OVHD;
    pccb->Dg_ovhd = DG_OVHD;
    pccb->Retdat_ovhd = RETDAT_OVHD;
    pccb->Lretdat_cssize = LRETDAT_CSSIZE;
    pccb->Min_msg_size = MSG_OVHD + sizeof( SCSH );
    pccb->Max_msg_size = MSG_OVHD + sizeof( SCSH ) + scs_msg_size;
    pccb->Min_dg_size = DG_OVHD;
    pccb->Max_dg_size = DG_OVHD + sizeof( SCSH ) + scs_dg_size;
    pccb->Min_id_size = sizeof( MSI_ID );
    pccb->Max_id_size = sizeof( MSI_ID );
    pccb->Min_idreq_size = sizeof( MSI_IDREQ );
    pccb->Max_idreq_size = sizeof( MSI_IDREQ );
    pccb->Min_datreq_size = sizeof( MSI_DATREQ );
    pccb->Max_datreq_size = sizeof( MSI_DATREQ );
    pccb->Min_sntdat_size = SNTDAT_MINSIZE;
    pccb->Max_sntdat_size = SNTDAT_MINSIZE + MAX_DATA_SIZE;
    pccb->Msicsr = ( vu_short * )&msiregs->sii_msicsr;
    pccb->Msidscr = ( vu_short * )&msiregs->sii_msicr;
    pccb->Msidssr = ( vu_short * )&msiregs->sii_msisr;
    pccb->Msiidr = ( vu_short * )&msiregs->sii_msiid;
    pccb->Msitr = ( vu_short * )&msiregs->sii_msitr;
    pccb->Msitlp = ( vu_short * )&msiregs->sii_msiltlp;
    pccb->Msiilp = ( vu_short * )&msiregs->sii_msiilp;
    pccb->Msidcr = ( vu_short * )&msiregs->sii_msidcr;
    pccb->Msicomm = ( vu_short * )&msiregs->sii_msicomm;
    pccb->Msidstat = ( vu_short * )&msiregs->sii_msidstat;
    pccb->Msiisr3 = ( vu_short * )&msiregs->sii_msiisr3;
    pccb->Lpstatus.init = 1;
    Init_comqh_lock( pccb )
    Init_comql_lock( pccb )
    Init_dfree_lock( pccb )
    Init_mfree_lock( pccb )
    Init_rfp_lock( pccb )
    Init_xfp_lock( pccb )
    {
    register u_long		num;
    register MSI_PPORTINFO	*ppi;

    for( num = 0, ppi = &pccb->Perport[ 0 ];
	 num < MSI_MAXNUM_PORT;
	 ++num, ++ppi ) {
	Init_queue( ppi->xretryq )
    }
    }

    pccb->lpinfo.Dg_size = sizeof( MSIBH )   +
			   sizeof( CIPPDH ) +
			   sizeof( SCSH )    +
			   scs_dg_size;
    pccb->lpinfo.Msg_size = sizeof( MSIBH )   +
			    sizeof( CIPPDH ) +
			    sizeof( SCSH )    +
			    scs_msg_size;
    pccb->lpinfo.Pd_ovhd = sizeof( MSIBH )   +
			   sizeof( CIPPDH );
    pccb->lpinfo.Ppd_ovhd = sizeof( MSIBH );
    Insert_entry( pccb->flink, scs_lport_db )
    ( void )msi_disable( pccb );
    Pccb_fork( pccb, msi_init_port, PANIC_PCCBFB )
    ( void )splx( save_ipl );
    return( 1 );
}

/*   Name:	msi_start_port	- Start a Local MSI Port
 *
 *   Abstract:	This function starts local MSI ports during local port
 *		initialization.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.active	-   0
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.online	-   0
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.active	-   1
 *	    rbusy		-   First receive-in-progress SIIBUF pointer
 *	    siiregptrs		-   MSI register pointers
 *		msicsr		-    Control/Status register
 *		msidcr		-    DSSI control register
 *		msidscr		-    DSSI control register
 *		msiidr		-    ID register
 *		msiilp		-    Initiator list pointer register
 *		msitlp		-    Target list pointer register
 *		msitr		-    Timeout register
 *	    xbusy		-   First transmit-in-progress SIIBUF pointer
 *	    xfree		-   First free transmit SIIBUF pointer
 *
 *   SMP:	The PCCB specific XFP and RFP are locked( EXTERNALLY ) to
 *		synchronize access to lpstatus.active, a MSI specific PCCB
 *		field.  This is probably unnecessary because of lack of
 *		conflict for this status bit due to the single threadedness of
 *		port clean up and initialization.  It is done anyway to
 *		guarantee unrestricted access and because stale RFP, XFP and
 *		XFP_TIMER threads could still be scheduled or even active.
 */
void
msi_start_port( pccb )
    register PCCB	*pccb;
{
    register siibq	*siibp;
    register u_long	nbufs,
			nxbufs;

    /* The steps involved in starting a local MSI port are as follows:
     *
     *  1. Indicate to the SII chip it is to use on-board jumper settings to
     *     specify the port station address.
     *  2. Indicate to the SII chip it is operating on an arbitrated bus.
     *  3. Enable SII chip timeouts and set target and initiator timer values.
     *  4. Turn on the SII chip DSSI bus drivers.
     *  5. Indicate current lack of output and input packets to the SII chip.
     *  6. Enable DSSI mode operation of the SII chip.
     *  7. Enable SII chip interrupts, parity error reporting, and receptions.
     *  8. Subdivide the local port SII RAM buffer into transmit and receive
     *	   SIIBUFs, appropriately initializing each of them.
     *  9. Hand the SII chip its initial receive buffer list.
     * 10. Mark the local MSI port active.
     *
     * The current lack of output and input packets are indicated to the SII
     * chip( Step 5 ) by setting its initator and target list pointers to 0.
     *
     * Transmit SIIBUFs are appropriately initialized( Step 8 ) and placed onto
     * the port specific free transmit SIIBUF circular queue for use by the
     * Transmit Fork Process.  Their initialization includes the circular
     * threading together of all free transmit SIIBUF command blocks.  Receive
     * SIIBUFs are initialized( Step 8 ) and placed onto the port specific
     * receive-in-progress circular queue.  Their initialization includes
     * enabling of interrupts and the threading together of all receive SIIBUF
     * command blocks.  The entire receive buffer list is handed to the local
     * MSI port( Step 9 ) by setting its target list pointer to the command
     * block of the very first available receive SIIBUF buffer.
     *
     * NOTE: Initiator( packet transmission ) timeout interval is 2800 usecs.
     *
     * NOTE: Target( packet reception ) timeout interval is 2400 usecs.
     *
     * NOTE: The size of the entire RAM buffer is 131,072 bytes.  The size of
     *	     a SIIBUF is approximately 4,160 bytes.  This allows the SII RAM
     *	     buffer to be divided up into 31 buffers.  Based upon 7 potential
     *	     remote ports( the local port is not included ), 14 SIIBUFs are
     *	     set aside as transmit SIIBUFs, 2 per remote port.  The remaining
     *	     17 SIIBUFs are used as receive SIIBUFs.
     *
     * NOTE: The SII RAM buffer may not be subdivided until AFTER the local MSI
     *	     port registers have been initialized.  It is not accessible until
     *	     such initialization has taken place.
     */
    *pccb->Msiidr  = 0;
#ifdef mips
	*pccb->Msicsr  = 0;
#else
	*pccb->Msicsr  = MSICSR_HPM; 
#endif mips
    *pccb->Msitr   = ( MSITR_ENA | MSITR_TTV | MSITR_ITV );
    *pccb->Msidcr  = MSIDCR_PRE;
    *pccb->Msiilp  = 0;
    *pccb->Msitlp  = 0;
    *pccb->Msidscr = ( MSIDSCR_DSE | MSIDSCR_ALLCH );
#ifdef mips
	*pccb->Msicsr  = ( MSICSR_SLE | MSICSR_PCE | MSICSR_IE );
#else
    *pccb->Msicsr  = ( MSICSR_HPM | MSICSR_SLE | MSICSR_PCE | MSICSR_IE );
#endif mips

    WBFLUSH;

    pccb->Xfree = NULL;
    pccb->Rbusy = NULL;
    for( siibp = ( siibq * )pccb->Siibuffer,
	  nbufs = sizeof( struct siibuf ) / sizeof( SIIBUF ),
	  nxbufs = ( 2 * ( MSI_MAXNUM_PORT - 1 ));
	 nbufs > 0;
	 --nbufs, siibp = ( siibq * )Quad_align( ++Siibp )) {
	( void )bzero(( u_char * )Siibp, sizeof( SIIBUF ));
	Siibp->size = sizeof( SIIBUF );
 	Siibp->type = DYN_SIIBUF;
	Siibp->cmdblkaddr = Siiaddr( pccb, &Siibp->cmdblk );
	if( nbufs > nxbufs ) {
	    Interrupt_enable( Siibp )
	    if( pccb->Rbusy ) {
		Thread_cmdblk( pccb->Rbusy, Siibp )
		Rinsert_rbusy( pccb, Siibp )
	    } else {
		pccb->Rbusy = siibp;
		Init_queue( *siibp )
	    }
	} else {
	    Siibp->xpktaddr = Siiaddr( pccb, &Siibp->Xpkt );
	    if( pccb->Xfree ) {
		Thread_cmdblk( pccb->Xfree, Siibp )
		Xinsert_xfree( pccb, Siibp )
	    } else {
		pccb->Xfree = siibp;
		Init_queue( *siibp )
	    }
	}
    }
    pccb->Xbusy = siibp = pccb->Xfree;
    Thread_cmdblk( siibp, siibp )
    *pccb->Msitlp = (( SIIBUF * )pccb->Rbusy )->cmdblkaddr;

    WBFLUSH;

    pccb->Lpstatus.active = 1;
}

