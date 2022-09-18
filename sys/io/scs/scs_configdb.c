#ifndef	lint
static char *sccsid = "@(#)scs_configdb.c	4.1	(ULTRIX)	7/2/90";
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
 *		Systems Communication Services
 *
 *   Abstract:	This module contains Systems Communication Services( SCS )
 *		functions for the allocation and deallocation of system-wide
 *		configuration database components.  It includes routines
 *		invoked by both port drivers and SCS.
 *
 *   Creator:	Todd M. Katz	Creation Date:	July 7, 1985
 *
 *   Function/Routines:
 *
 *   scs_alloc_cb		Allocate Connection Block
 *   scs_alloc_pb		Allocate Path Block
 *   scs_alloc_sb		Allocate System Block
 *   scs_dealloc_cb		Deallocate Connection Block
 *   scs_dealloc_pb		Deallocate Path Block
 *   scs_dealloc_sb		Deallocate System Block
 *
 *   Modification History:
 *
 *   22-Sep-1989	Pete Keilty
 *	Remove forkb.ipl setting. Calling routine now raises ipl.
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   06-Mar-1989	Todd M. Katz		TMK0004
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   19-Aug-1988	Todd M. Katz		TMK0003
 *	Cast all control blocks to ( char * ) before deallocating.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0002
 *	1. Correctly set the size fields of Connection Blocks( CB ), Path
 *	   Blocks( PB ), and System Blocks( SB ) following their allocation
 *	   within scs_alloc_cb(), scs_alloc_pb(), and scs_alloc_sb()
 *	   respectively.
 *	2. Macros Copy_name() and Copy_data() have been renamed to Move_name()
 *	   and Move_data() respectively.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, restructured
 *	code paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#include		"../h/kmalloc.h"
#include		"../h/errlog.h"
#include		"../h/dyntypes.h"
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

/* External Variables and Routines.
 */
extern	CBVTDB		*scs_cbvtdb;
extern	void		scs_dealloc_cb();

/*   Name:	scs_alloc_cb	- Allocate Connection Block
 *
 *   Abstract:	This function allocates and initializes a Connection Block.  A
 *		CB vector table entry is also allocated to reference the CB.
 *
 *		An initial credits worth of datagram and message buffers may be
 *		allocated and added to the appropriate port's free datagram and
 *		message pools respectively.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *				   ( OPTIONAL )
 *   pb				- Path Block pointer( OPTIONAL )
 *   scs_cbvtdb			- CB vector table database pointer
 *   state			- Connection state of newly allocated CB
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   scs_cbvtdb			- CB vector table database pointer
 *	free_cbvte		-  Free CBVTE list head
 *
 *   Return Values:
 *
 *   Address of initialized CB on success
 *   Otherwise NULL on all allocation failures
 *
 *   SMP:	The SCA database is locked( EXTERNALLY ) allowing for CBVTE
 *		allocation and guarantees the validity of PB addresses.
 *
 *		The PB itself must NOT be EXTERNALLY locked as required by
 *		scs_alloc_buf and scs_dealloc_buf.
 */
CB *
scs_alloc_cb( state, cmsb, pb )
    u_long		state;
    register CMSB	*cmsb;
    register PB		*pb;
{
    register CB		*cb;
    register CBVTE	*cbvte;

    /* Message and buffer allocation is contingent upon the presence of both
     * a CMSB and a PB.  The latter indicates how many while the former
     * specifies from where( ie - on which local port are buffers allocated ).
     */
    if(( cbvte = scs_cbvtdb->free_cbvte )) {
	KM_ALLOC( cb, CB *, sizeof( CB ), KM_SCA, KM_NOW_CL_CA )
	if( cb ) {
	    scs_cbvtdb->free_cbvte = cbvte->ov1.cbvte;
	} else {
	    return( NULL );
	}
    } else {
	return( NULL );
    }
    Get_cb( cbvte ) = cb;
    U_long( cb->size ) = sizeof( CB );
    cb->type = DYN_CB;
    cb->cinfo.cstate = state;
    Move_connid( cbvte->connid, cb->cinfo.lconnid )
    if( pb ) {
	cb->pb = pb;
	cb->pdt = pb->pdt;
	cb->pccb = pb->pccb;
    }
    if( cmsb ) {
	cb->control = cmsb->control;
	cb->aux = cmsb->aux;
	Move_name( cmsb->lproc_name, cb->cinfo.lproc_name )
	Move_data( cmsb->conn_data, cb->cinfo.lconn_data )
	if( state != CS_LISTEN ) {
	    cb->msg_event = cmsb->msg_event;
	    cb->dg_event = cmsb->dg_event;
	    cb->cinfo.min_snd_credit = cmsb->min_snd_credit;
	    cb->cinfo.init_rec_credit = cmsb->init_rec_credit;
	    Move_name( cmsb->rproc_name, cb->cinfo.rproc_name )
	    if( pb ) {
		if( scs_alloc_buf( cb,
				   cmsb->init_rec_credit,
				   cmsb->init_dg_credit ) != RET_SUCCESS ) {
		    ( void )scs_dealloc_cb( cb );
		    cb = NULL;
		}
	    }
	} else {
	    Move_name( "                ", cb->cinfo.rproc_name )
	}
    }
    return( cb );
}

/*   Name:	scs_alloc_pb	- Allocate Path Block
 *
 *   Abstract:	This function allocates and initializes a Path Block.
 *
 *		Two SCS sequenced messages buffers are also allocated.  One is
 *		added to the appropriate port's free message pool for the
 *		reception of SCS requests and the other becomes the PB's SCS
 *		send message buffer for transmission of SCS requests.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *   state			- Path state of newly allocated PB
 *   pib			- Path Information Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pb				- Path Block pointer
 *				   INITIALIZED
 *
 *   Return Values:
 *
 *   Address of initialized Path Block on success
 *   Otherwise NULL on all allocation failures
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because PCCBs are never
 *		deleted once SCS becomes aware of their existence.
 *
 *		PB lock structure is initialized.
 *
 *		Locks lower than the PCCB in the SCA locking hierarchy may NOT
 *		be held EXTERNALLY without also holding the PCCB lock as
 *		required by PD routines which add message buffers to local
 *		port message free pools.
 */
PB *
scs_alloc_pb( state, pccb, pib )
    u_long		state;
    register PCCB	*pccb;
    PIB			*pib;
{
    register pbq	*pb;
    register SCSH	*send, *receive;

    KM_ALLOC( pb, pbq *, sizeof( PB ), KM_SCA, KM_NOW_CL_CA )
    if( pb ) {
	if(( send = ( *pccb->Alloc_msg )( pccb )) &&
	     ( receive = ( *pccb->Alloc_msg )( pccb ))) {
	    ( void )( *pccb->Add_msg )( pccb, receive );
  	    U_long( Pb->size ) = sizeof( PB );
	    Pb->type = DYN_PB;
	    Init_queue( Pb->cbs )
	    Init_queue( Pb->scs_cb )
	    Pb->pccb = pccb;
	    Pb->pdt = pccb->pdt;
	    Pb->scs_msgbuf = send;
	    Pb->pinfo = *pib;
	    Pb->pinfo.state = state;
	    Init_pb_lock( Pb )
	} else {
	    if( send ) {
		( void )( *pccb->Dealloc_msg )( pccb, send );
	    }
	    KM_FREE( pb, KM_SCA )
	    pb = NULL;
	}
    }
    return( Pb );
}

/*   Name:	scs_alloc_sb	- Allocate System Block
 *
 *   Abstract:	This function allocates and initializes a System Block.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   sib			- System Information Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   sb				- System Block pointer
 *				   INITIALIZED
 *
 *   Return Values:
 *
 *   Address of initialized System Block on success
 *   Otherwise NULL on all allocation failures
 *
 *   SMP:	No locks are required.
 */
SB *
scs_alloc_sb( sib )
    SIB			*sib;
{
    register sbq	*sb;

    KM_ALLOC( sb, sbq *, sizeof( SB ), KM_SCA, KM_NOW_CL_CA )
    if( sb ) {
	U_long( Sb->size ) = sizeof( SB );
	Sb->type = DYN_SB;
	Init_queue( Sb->pbs )
	Sb->sinfo = *sib;
    }
    return( Sb );
}

/*   Name:	scs_dealloc_cb	- Deallocate Connection Block
 *
 *   Abstract:	This function cleans up and deallocates Connection Blocks.  The
 *		CB's vector table entry is returned to the CBVTE free list
 *		after its sequence number is incremented.
 *
 *		Receive datagram and message buffers are deallocated following
 *		their removal from the appropriate port's free datagram and
 *		message pools respectively.
 *
 *		One additional message buffer is removed and deallocated for
 *		each block data transfer currently in progress.
 *
 *		NOTE: The appropriate port drivers are always responsible for
 *		      the clean up of all port specific resources associated
 *		      with failed local ports including free datagram and
 *		      message buffers.  SCS must never attempt to dispose of
 *		      such resources during clean up of the paths and
 *		      connections associated with such failed ports.
 *		
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cb				- Connection Block pointer
 *   scs_cbvtdb			- CB vector table database pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cbvte			- Connection Block pointer
 *	connid.seq_num		-  CB sequenced number
 *   scs_cbvtdb			- CB vector table database pointer
 *	free_cbvte		-  Free CBVTE list head
 *
 *   SMP:	The SCA database is locked( EXTERNALLY ) allowing for CBVTE
 *		deallocation.
 *
 *		The CB is locked( EXTERNALLY ) to synchronize access and
 *		prevent premature deletion but ONLY whenever it formerly
 *		resided within the system-wide configuration database and
 *		contention for it could exist.  It is locked through its CBVTE.
 *
 *		The PB itself must NOT be EXTERNALLY locked as required by
 *		scs_dealloc_buf.
 */
void
scs_dealloc_cb( cb )
    register CB		*cb;
{
    register CBVTE	*cbvte = Get_cbvte( cb->cinfo.lconnid );

    ( void )scs_dealloc_buf( cb );
    KM_FREE(( char * )cb, KM_SCA )
    Incr_seq_num( cbvte->connid )
    cbvte->ov1.cbvte = scs_cbvtdb->free_cbvte;
    scs_cbvtdb->free_cbvte = cbvte;
}

/*   Name:	scs_dealloc_pb	- Deallocate Path Block
 *
 *   Abstract:	This function cleans up and deallocates Path Blocks.  It is
 *		only invoked for failed paths, formative or established.
 *
 *		The PB's SCS send and receive message buffers are deallocated.
 *		The SCS receive message buffer is removed from the appropriate
 *		port's free message pool.  The SCS send message buffer is only
 *		obtained from this pool if it is not present on the PB.
 *
 *		NOTE: If the SCS send message buffer is retrieved from the
 *		      appropriate port's free message pool it is either already
 *		      in the pool or it appears there shortly after the local
 *		      port is through with it.
 *
 *		NOTE: The appropriate port drivers are always responsible for
 *		      the clean up of all port specific resources associated
 *		      with failed local ports including free datagram and
 *		      message buffers.  SCS must never attempt to dispose of
 *		      such resources during clean up of the paths and
 *		      connections associated with such failed ports.
 *		
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *   pb				- Path Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required; however, the validity of the PB address
 *		must be guaranteed EXTERNALLY.
 *
 *		The PB itself must NOT be EXTERNALLY locked.
 *
 *		Locks lower than the PCCB in the SCA locking hierarchy may NOT
 *		be held EXTERNALLY without also holding the PCCB lock as
 *		required by PD routines which remove message buffers from local
 *		port message free pools.
 */
void
scs_dealloc_pb( pccb, pb )
    register PB		*pb;
    register PCCB	*pccb;
{
    register SCSH	*scsbp;
    register u_long	skip;

    skip = Port_failure( pb->pinfo.reason );
    if(( scsbp = pb->scs_msgbuf ) ||
       ( !skip && ( scsbp = ( *pb->Remove_msg )( pccb )))) {
	( void )( *pb->Dealloc_msg )( pccb, scsbp );
	if( !skip && ( scsbp = ( *pb->Remove_msg )( pccb ))) {
	    ( void )( *pb->Dealloc_msg )( pccb, scsbp );
	}
    }
    KM_FREE(( char * )pb, KM_SCA )
}

/*   Name:	scs_dealloc_sb	- Deallocate System Block
 *
 *   Abstract:	This function cleans up and deallocates System Blocks.
 *
 *		NOTE: The appropriate port drivers are always responsible for
 *		      the clean up of all port specific resources associated
 *		      with failed local ports including free datagram and
 *		      message buffers.  SCS must never attempt to dispose of
 *		      such resources during clean up of the paths and
 *		      connections associated with such failed ports.
 *		
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   sb				- System Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.
 */
void
scs_dealloc_sb( sb )
    register SB		*sb;
{
    KM_FREE(( char * )sb, KM_SCA )
}
