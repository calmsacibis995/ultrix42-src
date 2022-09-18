#ifndef	lint
static char *sccsid = "@(#)msi_isr.c	4.2	(ULTRIX)	7/17/90";
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
 *		Driver( MSI ) interrupt service routines and functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 23, 1988
 *
 *   Function/Routines:
 *
 *   msi_isr			MSI Interrupt Service Routine
 *   msi_rfp			MSI Receive Fork Process Routine
 *   msi_xfp			MSI Transmit Fork Process Routine
 *   msi_xfp_timer		MSI Transmit Fork Process Timer Routine
 *
 *   Modification History:
 *
 *   05-Jul-1990	Pete Keilty
 *	Added DELAY(2) to the msi_xfp routine before getting status.
 *	This is to prevent false status causing retransmit of packets
 *	( duplicates ). The SII is still updating the status.
 *	       
 *   11-Jul-1989	Pete Keilty
 * 	The change made on 9-Jul-1989 has to do with the SII clearing the 
 *	DSCR_OUT bit in Msidscr and then writting status. The DSCR_OUT
 *	bit should indicate that the SII is "stop" but it DOESN'T so check
 *	the SII state bits in Msiisr3 for writting status. 
 *	This is done in a new macro Xsii_busy( pccb ) in msi_xfp().
 *
 *	SII issues from Rob Frame's memo:
 *	1) The first pertains to determining when the SII has finished
 *	   working on an outgoing packet. As the SII sends out packets, it
 *	   advances its pointer (ILP) to the next outgoing packet. The
 *	   exception to this is in the case where the packet was unsuccessfully
 *	   transmitted. In this case, the ILP does not advance, not rather
 *	   remains pointed to the failed packet. The intuitive way to determine
 *	   whether this case has occurred is to check the OUT bit in the DSCTRL
 *	   register.  If this bit is set, the SII is happily working on the
 *	   list. If it is cleared, an error must have happened and the SII is
 *	   stopped. Unfortunately, the SII clears the OUT bit before it 
 *	   completes writing status to the failed buffer. Therefore, simply
 *	   looking at the OUT bit is not a foolproof method of determining
 *	   whether the SII is stopped. This is why Steve looks at some of the
 *	   state bits.
 *	2) Another issue is that surrounding adding receive buffers. This
 *	   problem only occurs when the SII runs out of receive buffers. When
 *	   the SII gets selected, it checks to see if it has a free receive 
 *	   buffer. If it doesn't, it returns a NAK to the DSSI initiator. To 
 *	   retain consistency, the SII also attempts to write status to memory.
 *	   Since no buffer is there, no status gets written. The problem
 *	   occurs when the software supplies a new receive buffer after the
 *	   first check that the SII does but before it looks to write status.
 *	   This is fixed by clearing the opcode byte in the command bytes and
 *	   tossing the packet away if the status is 8000H and the opcode is 0.
 *	3) There is another problem associated with adding buffers to the SII
 *	   while it is active. When checking whether the SII needs a new buffer
 *	   address (i.e. the appropriate pointer is zero), the pointer may read
 *	   non-zero, when in fact, the SII has determined that it will be zero 
 *	   shortly. This is due to double buffering of the pointers. This is
 *	   another area where Steve checks various state bits to decide whether
 *	   the SII xLP will be going to zero soon.
 *
 *	At this time item 2 is being done in the driver. Item 3 is handle
 *	by threading the new buffer on the list and if it not caught this
 *	pass it is done on the next interrupt.
 *
 *   9-Jul-1989         Stephen W. Bailey       SWBXXXX
 *      Added logic to look at Msiisr3 when the TLP, or ILP are loaded,
 *      or when a completed packet on the IL seems to contain an error.
 *      This fixes several possible race conditions which occur 
 *      when servicing the SII.
 *
 *   14-Jun-1989	Pete Keilty
 *	Add include file smp_lock.h
 *
 *   10-Jun-1989	Pete Keilty
 *	Add Splscs() to raise ipl to IPL_SCS in msi_rfp, msi_xfp, msi_isr.
 *	The fork processes now raise there ipl on entering the routine.
 *	This change is because ksched routine no longer does it.
 *
 *   21-Mar-1989	Todd M. Katz		TMK0001
 *	The macro Xfp_timer_started() has been modified to appropriately set
 *	the IPL.  Modify the routine msi_xfp_timer() to reflect this change.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/dyntypes.h"
#include		"../h/param.h"
#include		"../h/time.h"
#include		"../h/ksched.h"
#include		"../h/kmalloc.h"
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#include		"../h/errlog.h"
#include		"../h/smp_lock.h"
#include		"../machine/cpu.h"
#ifdef vax
#include		"../machine/mtpr.h"
#endif vax
#include		"../machine/pte.h"
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
extern	SCSIB		lscs;
extern  MSIBDDB		*msi_bddb;
extern	int		sysptsize;
extern	PCCB		*msi_adapt[];
extern	PB		*cippd_get_pb();
extern	SCSH		*msi_alloc_msg();
extern	void		cippd_crash_pb(),
			cippd_receive(),
			msi_crash_lport(),
			msi_dealloc_pkt(),
			msi_log_packet(),
			msi_rfp(),
			msi_xfp(),
			msi_xfp_timer(),
			scs_dg_rec(),
			scs_msg_rec(),
			scs_msg_snt(),
			timeout();

/*   Name:	msi_isr		- MSI Interrupt Service Routine
 *
 *   Abstract:	This is the interrupt service routine for all local MSI ports.
 *
 *   Inputs:
 *
 *   IPL_MSI			- Interrupt processor level
 *   msi_adapt			- Vector of MSI Adapter Control Blocks
 *   pccb			- Port Command and Control Block pointer
 *	lpinfo.type.hwtype	-  HPT_SII
 *
 *   Outputs:
 *
 *   IPL_MSI			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.xfork	-   1
 *	    lpstatus.rfork	-   1
 *	    rforkb		-   Receive Fork Process fork block
 *	    save_dssr		-   Cached DSSI status register contents
 *	    save_dstat		-   Cached data transfer status reg contents
 *	    siiregptrs		-   MSI register pointers
 *		msidssr		-    DSSI status register
 *		dstat		-    Data transfer status register
 *	    xforkb		-   Transmit Fork Process fork block
 *
 *   SMP:	PCCB
 */
void
msi_isr( msinum )
    u_long		msinum;
{
    register PCCB	*pccb;
    register u_long	dssr,
			lpcrash,
			save_ipl;

    /* Four errors:
     *
     * 1. BUS ERROR:			DSSR( BER ) <- 1
     * 2. Selected with attention	DSSR( SWA ) <- 1 && DSSR( SCH ) <- 1 
     * 3. Selected non-DSSI device	DSSR( BUF ) <- 1 && DSSR( SCH ) <- 1 
     * 3. Selected by non-DSSI device	DSSR( SCH ) <- 1 
     */
    save_ipl = Splscs();
    pccb = msi_adapt[ msinum ];
    if(( dssr = *pccb->Msidssr ) & MSIDSSR_FERRS ) {
	Lock_pccb( pccb )
	if(( dssr = *pccb->Msidssr ) & MSIDSSR_FERRS ) {
	    if( dssr & MSIDSSR_BER ) {
		lpcrash = SE_BUSERROR;
	    } else if( dssr & MSIDSSR_SWA ) {
		lpcrash = SE_SWA;
	    } else if( dssr & MSIDSSR_BUF ) {
		lpcrash = SE_IMODE;
	    } else {
		lpcrash = SE_TMODE;
	    }
	    pccb->Save_dssr = dssr;
	    pccb->Save_dstat = *pccb->Msidstat;
	    ( void )msi_crash_lport( pccb, lpcrash, NULL );
	    Unlock_pccb( pccb )
	    ( void )splx( save_ipl );
	    return;
	} else {
	    Unlock_pccb( pccb )
	}
    }
    *pccb->Msidssr = dssr;
    *pccb->Msidstat = *pccb->Msidstat;
    Xstart_xfp( pccb )
    Rstart_rfp( pccb )
    ( void )splx( save_ipl );
}

/*   Name:	msi_rfp		- MSI Receive Fork Process Routine
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.rfork	-   1
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    comqh		-   MSIB high priority command queue
 *	    dfreeq		-   MSIB datagram free queue
 *	    errlogopt.portnum	-   Remote port station address
 *	    lpcinfo		-   Optional local port crash information
 *		pkth		-    Address of MSI packet
 *		pktsize		-    Size of msi packet
 *		pport_addr	-    Packet remote port station address
 *	    lpstatus.rfork	-   0
 *	    lpstatus.xfork	-   Transmit Fork Process scheduled flag
 *	    mfreeq		-   MSIB message free queue
 *	    perport		-   Per-DSSI port information
 *		rseqno		-    Next expected receive sequence number
 *	    rbusy		-   First receive-in-progress SIIBUF pointer
 *	    rdmap		-   Receive Fork Process DMapping Buffer Info
 *	    siiregptrs		-   MSI register pointers
 *		msitlp		-    Target list pointer register
 *	    xforkb		-   Transmit Fork Process fork block
 *
 *   SMP:
 */
void
msi_rfp( pccb )
    register PCCB		*pccb;
{
    register msibq		*msibp;
    register siibq		*siibp,
    				*rfree = NULL;
    register MSI_PPORTINFO	*spi;
    msibq			rdoneq;
    u_long			event = 0,
				lpc = 0,
				start_xfp = 0;
    u_long			save_ipl;

    /*
     */
    Rfp_started( pccb, save_ipl )
    Init_queue( rdoneq )

    /*
     */
    for( siibp = pccb->Rbusy;
	 ( *pccb->Msitlp != Siibp->cmdblkaddr && siibp != rfree );
	 pccb->Rbusy = siibp = siibp->flink ) {
	if( Rreceive_good( pccb, Siibp )) {
	    spi = &pccb->Perport[ Siibp->cmdblk.src ]; Msibp = NULL;
	    if( Siibp->Rpkt.ph.opcode == MSG ) {
		Rcheck_msg( pccb, Siibp, spi, msibp, event, lpc )
		if( Msibp ) {
		    Rformat_msib( Siibp, Msibp )
		    Insert_msib( Msibp, rdoneq )
		}
	    } else if( Siibp->Rpkt.ph.opcode == SNTDAT ) {
		Rcheck_sntdat( pccb, Siibp, spi, msibp, event, lpc )
		if( Msibp ) {
		    ( void )bcopy( Siibp->Rpkt.sntdat.data,
				   pccb->Rdmap.Saddr,
				   pccb->Rdmap.ssize );
		    if( Lp_msih( Siibp->Rpkt.ph ) == 0 ) {
			Reset_msib( Msibp )
			Insert_mfreeq( pccb, Msibp )
		    } else {
			Rqueue_cnf( pccb, Msibp, Siibp )
			start_xfp = 1;
		    }
		}
	    } else {
		switch( Siibp->Rpkt.ph.opcode ) {

		    case DATREQ0:
		    case DATREQ1:
		    case DATREQ2:
			Rcheck_datreq( pccb, Siibp, spi, msibp, event, lpc )
			if( Msibp ) {
			    Rqueue_lretdat( pccb, Msibp, Siibp )
			    start_xfp = 1;
			}
			break;

		    case DG:
			Rcheck_dg( pccb, Siibp, msibp, event )
			if( Msibp ) {
			    Rformat_msib( Siibp, Msibp )
			    Insert_msib( Msibp, rdoneq )
			}
			break;

		    case ID:
			Rcheck_id( pccb, Siibp, msibp, event )
			if( Msibp ) {
			    Rformat_msib( Siibp, Msibp )
			    Insert_msib( Msibp, rdoneq )
			}
			break;

		    case IDREQ:
			Rcheck_idreq( pccb, Siibp, msibp, event )
			if( Msibp ) {
			    Rqueue_id( pccb, Msibp, Siibp )
			    start_xfp = 1;
			}
			break;

		    case RST:
		    case STRT:
			break;

		    case CNF:
		    case RETDAT:
			event = SE_INVOPCODE;
			break;

		    default:
			event = SE_UNKOPCODE;
		}
	    }
	    if( event ) {
		Rproc_error( pccb, Siibp, msibp, event, lpc, rdoneq )
		event = 0;
		if( lpc ) {
		    Unlock_rfp( pccb )
		    ( void )splx( save_ipl );
		    return;
		}
	    }
	} else {
	    if( Rinvalid_srcaddr( pccb, Siibp )) {
		Rlog_invsrcaddr( pccb, Siibp )
	    }
	}
	Rreset_cmdblk( Siibp )
	if( rfree == NULL ) {
	    rfree = siibp;
	}
    }

    /*
     */
    if( rfree ) {
	if( rfree != ( siibp = pccb->Rbusy )) {
	    Term_thread( siibp )
	    Thread_cmdblk( rfree, rfree )
	    if( *pccb->Msitlp == 0 ) {
		*pccb->Msitlp = (( SIIBUF * )rfree )->cmdblkaddr;
	    }
	} else {
	    *pccb->Msitlp = (( SIIBUF * )rfree )->cmdblkaddr;
	}
    }

    /*
     */
    {
    register msibq	*qaddr = &rdoneq;

    Unlock_rfp( pccb )
    if( start_xfp ) {
	Xstart_xfp( pccb )
    }
    for( msibp = qaddr->flink; msibp != qaddr; msibp = qaddr->flink ) {
	Remove_msib( Msibp )
	Rfinish_receive( pccb, Msibp )
    }
    }
    ( void )splx( save_ipl );
}

/*   Name:	msi_xfp		- MSI Transmit Fork Process Routine
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.xfork	-   1
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    comqh		-   MSIB high priority command queue
 *	    comql		-   MSIB low priority command queue
 *	    dfreeq		-   MSIB datagram free queue
 *	    lpcinfo		-   Optional local port crash information
 *		pkth		-    Address of MSI packet
 *		pktsize		-    Size of msi packet
 *		pport_addr	-    Packet remote port station address
 *	    lpstatus.timer	-   Retry delay timer active flag
 *	    lpstatus.xfork	-   0
 *	    mfreeq		-   MSIB message free queue
 *	    perport		-   Per-DSSI port information
 *		rpstatus.dip	-    Transmit delaying in progress flag
 *		rpstatus.path	-    Path exists flag
 *		xretrys		-    Current transmit retry attempt
 *		xretryq		-    MSIB transmit retry queue
 *		xretry_timer	-    Transmit retry timer( in 10 msecs )
 *		xseqno		-    Next transmit sequence number
 *	    siiregptrs		-   MSI register pointers
 *		msidscr		-    DSSI control register
 *		msiilp		-    Initiator list pointer register
 *	    xbusy		-   First transmit-in-progress SIIBUF pointer
 *	    xdmap		-   Transmit Fork Process DMapping Buffer Info
 *	    xfree		-   First free transmit SIIBUF pointer
 *
 *   SMP:	PCCB:	Interlock Access
 *			Prevent PB deletion
 *		XFP:	Interlock Access
 *			Guarantee only one active XFP and XFP_TIMER threads
 *			Prohibits Port Crashing
 *			Prohibits VC closure except at behalf this routine
 *		DFREEQ:	Interlock Access
 *		MFREEQ:	Interlock Access
 *		COMQH:	Interlock Access
 *		COMQL:	Interlock Access
 */
void
msi_xfp( pccb )
    register PCCB		*pccb;
{
    msibq			xmsg_doneq;
    register siibq		*siibp,
				*xstart_xmt = NULL;
    register msibq		*msibp;
    register MSI_PPORTINFO	*dpi;
    u_long			save_ipl;

/*
 */
    Xfp_started( pccb, save_ipl )
    Init_queue( xmsg_doneq )

    /*
     */
    while(( Msibp = (( SIIBUF * )( siibp = pccb->Xbusy ))->save_msib )) {
	if( Siibp->cmdblk.status == ST_SUCCESS ) {
	    if( *pccb->Msiilp != Siibp->cmdblkaddr ) {
		dpi = &pccb->Perport[ Siibp->cmdblk.dst ];
		Xtransmit_done( pccb, Msibp, dpi, xmsg_doneq )
		Xfree_siib( pccb, Siibp )
	    } else {
		break;
	    }
	} else if( Xsii_busy( pccb )) {
	    break;
	} else {
	    /* Delay for awhile, if not we get bad status and cause 
	       duplicate packets to be resent. We should not have to 
	       do this, UGH SII. */
	    DELAY( 2 );
	    if( Siibp->cmdblk.status == ST_SUCCESS ) {
		continue;
	    } else if( Xfalse_nack( pccb, Siibp )) {
		Siibp->cmdblk.status = ST_SUCCESS;
		continue;
	    } else if( *pccb->Msiilp ) {
		*pccb->Msiilp = 0;
		dpi = &pccb->Perport[ Siibp->cmdblk.dst ];
		if( Xignore_xmtfail( pccb, Siibp, dpi )) {
		    Xdiscard_msib( pccb, Msibp )
		    Xfree_siib( pccb, Siibp )
		} else {
		    Xretry_xmt( pccb, siibp, msibp, dpi, xmsg_doneq )
		}
	    }
	    if((( SIIBUF * )( siibp = pccb->Xbusy ))->save_msib ) {
		xstart_xmt = siibp;
		Xreset_xbusy( pccb, siibp )
	    }
	    break;
	}
    }
    if( Siibp->save_msib == 0 || xstart_xmt ) {
	siibp = pccb->Xfree;
	Thread_cmdblk( siibp, siibp )
    }

    /*
     */
    while((( SIIBUF * )( siibp = pccb->Xfree ))->save_msib == 0 ) {
	Remove_comqh( pccb, msibp )
	if( Msibp == NULL ) {
	    Remove_comql( pccb, msibp )
	    if( Msibp == NULL ) {
		break;
	    }
	}
	dpi = &pccb->Perport[ Msibp->Rport_addr ];
	if( Rpstatus_dip( dpi )) {
	    Xinsert_xretryq( dpi, Msibp )
	} else if( Xabort_xmt( dpi, Msibp )) {
	    Xdiscard_msib( pccb, Msibp )
	} else {
	    Xformat_siib( Siibp, Msibp )
	    if( Msibp->Ph.opcode == RETDAT ) {
		u_long	event = 0;

		Xcheck_lretdat( pccb, Msibp, event )
		if( event == 0 ) {
		    ( void )bcopy( pccb->Xdmap.Saddr,
				   Siibp->Xpkt.retdat.data,
				   pccb->Xdmap.ssize );
		    if( Lp_msih( Msibp->Ph )) {
			Set_lp( Siibp->Xpkt.ph )
		    }
		    Msibp->Framelength += U_short( pccb->Xdmap.ssize );
		} else {
		    Xretdat_error( pccb, msibp, event, xmsg_doneq )
		    Unlock_xfp( pccb )
		    ( void )splx( save_ipl );
		    return;
		}
	    }
	    if( Vc_msib( Msibp )) {
		Xset_xseqno( dpi, Siibp )
	    }
	    Siibp->save_msib = Msibp;
	    Xformat_cmdblk( pccb, Siibp, Msibp )
	    Xformat_xlink( Siibp, Msibp )
	    if( xstart_xmt == NULL ) {
		xstart_xmt = siibp;
	    }
	    pccb->Xfree = siibp->flink;
	}
    }

    /*
     */
    if( xstart_xmt ) {
	Interrupt_enable( xstart_xmt )
	siibp = pccb->Xfree;
	Term_thread( siibp )
	siibp = siibp->blink;
	Interrupt_enable( Siibp )
	if( *pccb->Msiilp == 0 ) {
	    *pccb->Msiilp = (( SIIBUF * )xstart_xmt )->cmdblkaddr;
	    *pccb->Msidscr = MSIDSCR_SOUT;
	}
	if( xstart_xmt != pccb->Xbusy ) {
	    Thread_cmdblk( xstart_xmt, xstart_xmt )
	}
    }

    /*
     */
    Unlock_xfp( pccb )
    while(( msibp = xmsg_doneq.flink ) != &xmsg_doneq ) {
	Remove_msib( Msibp )
	( void )scs_msg_snt( pccb,
			     ( SCSH * )Msibp->Msg.text,
			     ( Msibp->Framelength - MSG_OVHD ));
    }
    ( void )splx( save_ipl );
}

/*   Name:	msi_xfp_timer	- MSI Transmit Fork Process Timer Routine
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   IPL_SCS		- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    lpstatus.timer	-   1
 *
 *   Outputs:
 *
 *   IPL_SCS		- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	pd.msi			-  MSI specific PCCB fields
 *	    comqh		-   MSIB high priority command queue
 *	    lpstatus.timer	-   Retry delay timer status bit
 *	    lpstatus.xfork	-   Transmit Fork Process scheduled flag
 *	    perport		-   Per-DSSI port information
 *		rpstatus.dip	-    Transmit delaying in progress flag
 *		xretryq		-    MSIB transmit retry queue
 *		xretry_timer	-    Transmit retry timer( in 10 msecs )
 *	    xforkb		-   Transmit Fork Process fork block
 *
 *   SMP:	XFP:	Interlock Access
 *			Guarantee only one active XFP and XFP_TIMER threads
 *			Prohibits Port Crashing
 *			Prohibits VC closure except at behalf this routine
 *		COMQH:	Interlock Access
 *		PCCB:	Interlock Access
 */
void
msi_xfp_timer( pccb )
    register PCCB		*pccb;
{
    u_long			save_ipl;
    register msibq		*msibp;
    register MSI_PPORTINFO	*ppi;
    register u_long		portnum,
				reschedule = 0,
				start_xfp = 0;

    /*
     */
    Xfp_timer_started( pccb, save_ipl )
    for( portnum = 0; portnum < MSI_MAXNUM_PORT; ++portnum ) {
	ppi = &pccb->Perport[ portnum ];
	if( Rpstatus_dip( ppi )) {
	    if( --ppi->xretry_timer ) {
		reschedule = 1;
	    } else {
		Clear_rpdip( ppi )
		while(( msibp = ppi->xretryq.flink ) != &ppi->xretryq ) {
		    Remove_msib( Msibp )
		    Insert_comqh( pccb, Msibp )
		    start_xfp = 1;
		}
	    }
	}
    }
    if( reschedule ) {
	Xstart_xfp_timer( pccb )
    }
    Unlock_xfp( pccb )
    if( start_xfp ) {
	Xstart_xfp( pccb )
    }
    ( void )splx( save_ipl );
}
