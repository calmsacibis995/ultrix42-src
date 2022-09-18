#ifndef lint
static char *sccsid = "@(#)bvp_serv.c	4.4	(ULTRIX)	11/13/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988 - 1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *   Modification History
 *
 *   13-Nov-1990	Pete Keilty
 *	Change all instances of PF_ERROR -> PF_PORTERROR because of a
 *	fix to SCS resource deallocation.
 *	
 *   20-Jul-1989	Mark A. Parenti
 *	Check that path is not already failed before setting the
 *	reason and calling uq_disable().
 *
 *   25-May-1989	Pete Keilty
 *	Put ifdef vax around mtpr.h for mips cpu's.
 *
 *   05-Mar-1989	Todd M. Katz		TMK0003
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   19-Aug-1988	Todd M. Katz		TMK0002
 *	1. Modify bvp_crash_lport() to always apply the local port crash
 *	   severity modifier( ESM_LPC ) to the local port crash reason code.
 *	2. Make the following modifications to bvp_crash_path():
 *		1) Apply the path crash severity modifier( ESM_PC ) to the path
 *		   crash reason code whenever the path is open.
 *	        2) Use the appropriate SCS path crash reason code mapping
 *		   table( scs_map_pc[] has been split into scs_map_pc[] and
 *		   scs_map_spc[] ).
 *		3) The routine parameter scsbp ALWAYS points to a character
 *		   string of size NAME_SIZE instead of to the SCS header of a
 *		   datagram/message buffer whenever SCS invokes the routine
 *		   with a reason code of E_SYSAP.  This character string
 *		   consists of the name of the local SYSAP responsible for
 *		   crashing the path.
 *	3. Change all instances of PF_FATALERROR -> PF_ERROR as no current
 *	   instance of crashing the local port is fatal.
 *
 *   18-July-1988	Larry Cohen
 *	Remove debugging printfs
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   09-Jan-1988	Todd M. Katz		TMK0001
 *	Make the following changes:
 *	1. Reference scs_map_pc instead of bvp_map_genpc and delete the latter.
 *	2. Include new header files ../vaxscs/scaparam.h, ../vaxmsi/msisysap.h,
 *	   and ../vaxmsi/msiscs.h.
 */

/*	Include files
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/vmmac.h"
#include "../h/dk.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/fs.h"

#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax

#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"
#include "../io/bi/bvpreg.h"
#include "../io/bi/bvpport.h"
#include "../io/bi/bdareg.h"

#include	"../h/errlog.h"
#include	"../h/ksched.h"
#include	"../io/scs/sca.h"
#include	"../io/scs/scaparam.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/ci/cippdscs.h"
#include	"../io/ci/ciscs.h"
#include	"../io/msi/msiscs.h"
#include	"../io/bi/bvpscs.h"
#include	"../io/gvp/gvpscs.h"
#include	"../io/uba/uqscs.h"
#include	"../io/scs/scs.h"
#include	"../io/gvp/gvp.h"
#include	"../io/bi/bvpppd.h"




/*	Data definitions
 */

extern	void	bvp_disable();
extern	void	bvp_reinit();

extern	u_short	scs_map_pc[], scs_map_spc[];

/* 	Port Info Block
 */
extern	struct bvp_port_info bvp_port_info[];

/* BVP switch structure
 */
extern	struct bvp_sw bvp_sw[];

/*	BVP command table
 */
extern	struct 	bvp_cmd	bvp_cmd[];

/*

*/
/*
 *
 *
 *	Name:		bvp_open_path
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
u_short	bvp_open_path( pccb,pb )
PCCB	*pccb;
PB	*pb;
{
/* BVP adapters never initiate connects and therefore this routine 	*/
/* should never be called.						*/

	panic("bvpdriver: Attempt to open path");
}
/**/
/*
 *
 *
 *	Name:		bvp_crash_path
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_crash_path( pccb,pb,reason,disposal,scsbp )
PCCB	*pccb;
PB	*pb;
u_long	reason;
u_long	disposal;
SCSH	*scsbp;
{
register GVPPPDH	*bvpppdbp;
register u_long		pf_reason = 0;

    /* Log the path crash request and then dispose of the optional buffer as
     * directed to by the routine's invocator.
     *
     *	NOTE: When reason == E_SYSAP, scsbp is a pointer to a character string
     *	      of size NAME_SIZE containing the name of the SYSAP responsible
     *	      for crashing the path.
     */
    if ( scsbp != ( SCSH * )NULL && reason != E_SYSAP ) {
	bvpppdbp = Scs_to_ppd( scsbp );
	if ( disposal == RECEIVE_BUF )
	    if ( bvpppdbp->mtype == SCSMSG )
		( void )gvp_add_msg( pccb, scsbp );
	    else
		( void )gvp_add_dg( pccb, scsbp );
	else if ( disposal == DEALLOC_BUF )
	    if ( bvpppdbp->mtype == SCSMSG )
		( void )gvp_dealloc_msg( pccb, scsbp );
	    else
		( void )gvp_dealloc_dg( pccb, scsbp );
    }

    /* Set the port reason code and call the disable routine. LOGGING
     * SHOULD BE DONE AT THIS POINT.
     */
    if ( (pb != ( PB * )NULL) && (pb->pinfo.state != PS_PATH_FAILURE) ) {
	if( pb->pinfo.state == PS_OPEN ) {
	    Set_pc_event( reason )
	}
	if( Test_spc_event( reason )) {
	    if( Ecode( reason ) <= Ecode( SE_MAX_SCS )) {
		pf_reason = scs_map_spc[ Ecode( reason ) - 1 ];
	    }
	} else if( Test_pc_event( reason )) {
	    if( Ecode( reason ) <= Ecode( E_MAX_SCS )) {
		pf_reason = scs_map_pc[ Ecode( reason ) - 1 ];
	    }
	}
	if( pf_reason == 0 ) {
	    panic("bvpsspdriver: invoked with illegal path crash reason");
	}
	bvp_disable( pccb, pf_reason );

	}
}
/**/
/*
 *
 *
 *	Name:		bvp_get_pb
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
PB	*bvp_get_pb( pccb,scsbp,type )
PCCB	*pccb;
SCSH	*scsbp;
u_long	type;
{
	return(Pccb.pb);	/* Only one possible path block for BVP */
}
/**/
/*
 *
 *
 *	Name:		bvp_remove_pb
 *	
 *	Abstract:	
 *
 * 		This is where the pb and sb are removed from system queues 
 *		and deallocated. A check is made on the reinit counter. 
 *		If it is 0 then we give up, else a fork is done to 
 *		re-init the controller and control is returned to SCS.
 *
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_remove_pb( pccb,pb )
PCCB	*pccb;
PB	*pb;
{
SB	*sb;

	sb = pb->sb;
	Pccb.pb = 0;	/* Zero pb pointer		*/
	Remove_entry( pb->flink );
	Remove_entry( sb->flink );
	(void)scs_dealloc_pb( pccb, pb );
	(void)scs_dealloc_sb( sb );	
/*
 *	Start the re-initialization
 */
	Kfork( &pccb->forkb, bvp_reinit, pccb )
}
/**/
/*
 *
 *
 *	Name:		bvp_crash_lport
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_crash_lport( pccb,reason,scsbp )
PCCB		*pccb;
u_long		reason;
SCSH		*scsbp;
{
GVPH		*bvpbp;
gvpbq		*q;
u_long		nreason;

    /* The only time we get called with a buffer pointer is from the response
     * processing routine when a DGSNT opcode is received. In this case
     * the buffer is placed onto the datagram free queue.
     */

    if ( scsbp != ( SCSH * )NULL ) {
	( void )gvp_add_dg( pccb, scsbp );
    }	

	/* Unlock the local port queues.  Those queues which require unlocking
	 * are zeroed and all packets on it are lost.
	 */
	for( q = &Vpqb.cmdq0; q <= &Vpqb.rspq; ++q )
	    if ( U_long( q->flink ) & Q_LOCKED ) {
		q->flink = ( gvpbq * )NULL;
		q->blink = ( gvpbq * )NULL;
		}
	if ( U_long( Pccb.dfreeq.flink ) & Q_LOCKED ) {
		Pccb.dfreeq.flink = ( gvpbq * )NULL;
		Pccb.dfreeq.blink = ( gvpbq * )NULL;
		}
	if ( U_long( Pccb.mfreeq.flink ) & Q_LOCKED ) {
		Pccb.mfreeq.flink = ( gvpbq * )NULL;
		Pccb.mfreeq.blink = ( gvpbq * )NULL;
		}

/*
 *	Map Port Crash reason to Port Failure reason
 */

	switch (reason) {
	
	case SE_NOPATH:

		nreason = PF_PORTERROR;
		break;

	default:

		nreason = PF_SYSAP;
		break;
	}

	/* Set the LPC flag within the event code.
	 */
	Set_lpc_event( reason )

	pccb->lpinfo.reason = reason;

	/* Fork to initiate clean up of the local port.
	 *
	 * NOTE: The PCCB fork block is only used for clean up and
	 *	 re-initialization of the local port.  Therefore, it is always
	 *	 available because clean up and re-initialization is single
	 *	 threaded.  In other words, the PCCB fork block is interlocked
	 *	 by the PCCB "cleanup" local port status bit.
	 */

	bvp_disable( pccb, nreason );

}
/**/
/*
 *
 *
 *	Name:		bvp_mstart
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
u_short	bvp_mstart( pccb,rport_addr,start_addr )
PCCB	*pccb;
scaaddr	*rport_addr;
u_long	start_addr;
{
	return(RET_SUCCESS);
}
/**/
/*
 *
 *
 *	Name:		bvp_mreset
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
u_short	bvp_mreset( pccb,rport_addr,force )
PCCB	*pccb;
scaaddr	*rport_addr;
u_long	force;
{
	bvp_disable( pccb, PF_SYSAP );	/* SYSAP requested port crash	*/
	return(RET_SUCCESS);
}
/**/
