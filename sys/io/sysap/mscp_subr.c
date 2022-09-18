#ifndef	lint
static char *sccsid = "@(#)mscp_subr.c	4.3	(ULTRIX)	2/12/91";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987 - 1989 by                    *
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
 *		Disk Class Driver
 *
 *   Abstract:	This module contains all routines necessary to
 *		implement the disk MSCP.
 *
 *   Author:	David E. Eiche	Creation Date:	September 30, 1985
 *
 *   History:
 *
 *   11-Feb-1991	Pete Keilty
 *	Changed use of NRSPID constant to a variable defined in mscp_data.c
 *	nrspid. This will allow for adjustable rspid table size on a 
 *	system basic if needed. 
 *	To Do - nrspid size based on the credits per controller multiplied 
 *	by the number of controllers.
 *
 *   04-Dec-1990	Pete Keilty
 *	Changed mscp_control() checking of CRE_NEW_PATH event.
 *	Added new routine mscp_adapt_check which checks if this
 *	controller should be attached to this adapter as specified
 *	in the config file. This is an attempt to static load
 *	balance over multiple adapters. Also if the connection is
 *	not the this adapter set a system_poll in timeout for 60sec.
 *	If the other connection does not come in use what is there.
 *
 *   13-Mar-1990	David E. Eiche		DEE0083
 *	Fix mscp_dealloc_msg() to get the address of the next waiting
 *	request block.  The code got mangled during the ISIS merge.
 *
 *   21-Jul-1989	David E. Eiche		DEE0070
 *	Change reference from MSLG_FM_DSK_TRN to MSLG_FM_DISK_TRN.
 *	Change reference from MSLG_FM_BUS_ADR to MSLG_FM_BUS_ADDR.
 *
 *   11-Jul-1989	Tim Burke
 *	Modify mscp_poll_wait to wait at least one 15 second interval before
 *	concluding that all controllers have been identified.
 *
 *   16-May-1989	Pete Keilty
 *	Change mscp_restart_next to check restart queue is empty by
 *	comparing restart.flink with restart.
 *
 *   05-May-1989	Tim Burke	
 *	Merged 3.1 changes into the pu and isis pools.
 *
 *   31-Mar-1989	Pete Keilty
 *	Fixed alloc rspid Remove_entry to use rtp->flink;
 *
 *   15-Mar-1989	Tim Burke
 *	Changed splx( IPL_SCS ) to Splscs();
 *	Changed queue manipulations to use the following macros:
 *	insque ..... Insert_entry
 *	remque ..... Remove_entry
 *	remqck ..... Remove_entry and check to see if any elements on queue.
 *
 *   07-Mar-1989	Todd M. Katz		TMK0002
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   28-Dec-1988	Tim Burke
 *	Added the new error log format MSLG_FM_IBMSENSE to mscp_logerr() to
 *	handle informational error logs from the TA90.
 *
 *   20-Oct-1988	Pete Keilty
 *	Change mscp_poll_wait to use timeout() to do timing
 *	instead of DELAY.  DELAY raises IPL on CVAX processors
 *	and blocks interrupts for an extended period.
 *
 *   17-Oct-1988	Pete Keilty
 *	Added untimeout to mscp_alloc_reqb().
 *
 *   28-Sep-1988	David E. Eiche		DEE0057
 *	Change panic message formats to make them consistent and
 *	eliminate unnecessary panic messages.
 *
 *   09-Sep-1988	David E. Eiche		DEE0056
 *	Move initialization of request state from mscp_conqrestart
 *	in mscp_conpol.c to mscp_restart_next.  This fixes a bug in
 *	which the connection management request block was having
 *	its state erroneously initialized.
 *
 *   07-Sep-1988	Larry Cohen
 *	Add 15 second delay to mscp_poll_wait routine so that installation
 *	catches more disks.  Need to come up with a better way to wait.
 *
 *   06-Sep-1988	David E. Eiche		DEE0054
 *	Fix code which caused disks to hang when a second loss of
 *	connection occurred while recovering from the first.
 *
 *   27-Jul-1988	Pete Keilty
 *	Changed datagram routine to check for bbr busy and transfer
 *	datagram to bbr code. Also added new logerr routine.
 *
 *   17-Jul-1988	David E. Eiche		DEE0047
 *	Add routine mscp_avail_attn to process available attention
 *	messages.  Call it from mscp_message.
 *
 *   17-Jul-1988	David E. Eiche		DEE0046
 *	Change mscp_control to call mscp_get_connb directly, so
 *	that new path events can be responded to directly.
 *
 *   17-July-1988	David E. Eiche		DEE0045
 *	Move mscp_find_model to mscp_config.c.
 *
 *
 *   08-Jul-1988	Pete Keilty
 *	Added case MSLG_FM_REPLACE for errlogging in datagram routine.
 *
 *   23-Jun-1988	David E. Eiche		DEE0041
 *	Fixed mscp_send_msg to put the connection management request
 *	block in the active queue when there are requests waiting for credits.
 *
 *   16-June-1988	Larry Cohen
 *	Add global variable mscp_polls that is incremented for each
 *	connection attempt and decremented for each connection completion.
 *	The system waits in init_main for mscp_polls to go to zero so that
 *	autoconfigure output can be delayed until the disks/tapes have
 *	been sized.
 *
 *   08-Jun-1988	David E. Eiche		DEE0040
 *	Added a new routine mscp_service_bufferq which is called periodically
 *	from mscp_timer when the buffer wait queue on a connection is not
 *	empty.  The routine attempts to allocate buffers for requests waiting
 *	on the queue.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   02-Apr-1988	David E. Eiche		DEE0021
 *	Move sleep out of KM_ALLOC into mscp_alloc_reqb	to
 *	eliminate a sleep on interrupt stack double panic.
 *
 *   12-Feb-1988	David E. Eiche		DEE0014
 *	Change references to Insq and Remq macros to call the
 *	underlying insque and remqck functions directly.
 *
 *   02-Feb-1988	David E. Eiche		DEE0011
 *	Remove buf structure queuing code from mscp_alloc_reqb.
 *	Also rearrange mscp_timer code that detects waiting map
 *	requests.
 *
 *   15-Jan-1988	Todd M. Katz		TMK0001
 *	Include new header file ../vaxmsi/msisysap.h.
 */
/**/

/* Libraries and Include Files.
 */
#include	"../h/types.h"
#include	"../h/time.h"
#include	"../h/param.h"
#include	"../h/kmalloc.h"
#include	"../h/buf.h"
#include	"../h/errno.h"
#include	"../h/ioctl.h"
#include	"../h/devio.h"
#include	"../h/file.h"
#ifdef mips
#include	"../h/systm.h"
#endif mips
#include	"../fs/ufs/fs.h"
#include	"../h/errlog.h"
#include	"../machine/pte.h"
#include	"../h/vmmac.h"
#include	"../io/scs/sca.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include        "../io/msi/msisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/uba/ubavar.h"
#include	"../io/sysap/mscp_msg.h"
#include	"../io/sysap/mscp_defs.h"
#include	"../io/sysap/mscp_bbrdefs.h"

/**/

/* External variables and routines.
 */
extern	u_long			scs_alloc_msg();
extern	u_long			scs_dealloc_msg();
extern	u_long			scs_map_buf();
extern	u_long			scs_send_msg();
extern	u_long			scs_queue_dgs();
extern	u_long			scs_unmap_buf();
extern	int			wakeup();
extern  CONNB *			mscp_get_connb();
extern	UNITB *			mscp_get_unitb();
extern	RSPID_TBL		mscp_rspid_tbl[];
extern	LISTHD			mscp_rspid_lh;
extern	LISTHD			mscp_rspid_wait_lh;
extern	u_long			mscp_gbl_flags;
extern	int			hz;
extern	int			nrspid;
extern  int			rspid_wq_cnt;
extern	int			mscp_adapt_check();

void				mscp_avail_attn();
void				mscp_dealloc_all();
void				mscp_dispatch();
void				mscp_reserve_credit();
void				mscp_restart_next();
void				mscp_service_mapq();
void				mscp_service_bufferq();
void				mscp_service_creditq();
void				mscp_unstall_unit();

/**/

/*
 *
 *   Name:	mscp_dispatch - Finite state machine dispatcher.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */


void mscp_dispatch( event, rp )
    register u_long	    event;
    register REQB	    *rp;
{

    register u_long	    state;
    register STATE	    *sp;


    /* The following "do forever" allows redispatching when a non-NULL
     * event is returned by an action routine.
     */
    for( ;; ) {

	state = rp->state;
	sp = rp->state_tbl + ( state * ( EV_MAXEVENT + 1 ) + event );
	rp->state = sp->new_state;

	/* If the event code returned by the action routine is NULL,
	 * break out of the "do forever" and exit.  Otherwise, dispatch
	 * the new event.
	 */
	if(( event = ( *sp->action_rtn )( event, rp )) == EV_NULL )
	    break;


    }
    return;
}

/**/

/*
 *
 *   Name:	mscp_invevent - Process invalid event.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

u_long mscp_invevent( event, rp )
    register u_long		event;
    REQB			*rp;

{
    register u_long		state = rp->state;

    printf( "mscp_invevent:  invalid event %d in state %d, reqb %x\n",
	    event,
	    state,
	    rp );
    panic( "mscp_invevent: fatal mscp error\n" );
}


/**/

/*
 *
 *   Name:	mscp_noaction - Null action routine.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

u_long mscp_noaction( event, rp )
    register u_long		event;
    REQB			*rp;

{
    return( EV_NULL );
}
/**/

/*
 *
 *   Name:	mscp_timer - provide connection/resource timing services.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_timer( clp )
    register CLASSB		*clp;

{
    register CONNB		*cp;
    int				s;

    /* Raise the IPL to synchronize with SCS.
     */
    s = Splscs();

    /* If system polling is requested, start up the poller.
     */
    if( clp->flags.need_poll )
	mscp_system_poll( clp );

    /* Scan through the list of known connections.
     */
    for( cp = clp->flink;
	 cp != ( CONNB * )&clp->flink;
	 cp = cp->flink ) {

	/* If command timeouts are active on the connection and the timeout
	 * interval has expired, call connection management with a timeout
	 * event.
	 */
	if(( cp->cmdtmo_intvl != 0 ) && ( --cp->cmdtmo_intvl == 0 ))
	    mscp_dispatch( EV_TIMEOUT, &cp->timeout_reqb );
	
	/* If the resource wait queue timing threshhold has been reached and
	 * the either the buffer or mapping wait queue is not empty, service
	 * the waiting requests and reset the timeout interval.
	 */
	if( cp->rsrctmo_intvl == 0 ) {
	    if( cp->buffer_wq.flink != ( REQB * )&cp->buffer_wq.flink )
		mscp_service_bufferq( cp );
	    if( cp->map_wq.flink != ( REQB * )&cp->map_wq.flink )
		mscp_service_mapq( cp );
	    cp->rsrctmo_intvl = RSRC_WAIT_TMO;
	} else {
	    --cp->rsrctmo_intvl;
	}
    }

    /* Restore the entry IPL, start another timer interval, and exit.
     */
    ( void )splx( s );
    ( void )timeout( mscp_timer, ( caddr_t )clp, hz );
    return;
}

/**/

/*
 *
 *   Name:	mscp_control - Process asynchronous connection events.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_control( event, cmsp )
    register u_short		event;
    register CMSB		*cmsp;

{

    register CONNB		*cp;
    register REQB		*rp;
    register CLASSB		*clp;

    /* Store the CMSB address in the request block and dispatch
     * on asynchronous event type.  Note that cmsp->aux contains the
     * class block address in the new system case, but contains the
     * connection block address in all other cases.
     */
    if( event == CRE_NEW_PATH ) {
	clp = ( CLASSB * )cmsp->aux;
	if( mscp_adapt_check( cmsp )) {
	    cp = mscp_get_connb( clp,
			     &cmsp->sysid,
			     &cmsp->rport_addr,
			     cmsp->lport_name );
	    if( cp == NULL ) {
	        clp->flags.need_poll = 1;
	    }
	} else {
    	    ( void )timeout( mscp_system_poll, ( caddr_t )clp, hz * 60 );
	}
    } else {
	cp = ( CONNB * )cmsp->aux;
	rp = &cp->timeout_reqb;
	rp->aux = ( u_char * )cmsp;
	switch( event ) {
	    case	CRE_CONN_DONE:
		mscp_dispatch( EV_CONCOMPLETE, rp );
		break;
	    case	CRE_PATH_FAILURE:
		cp->flags.path_fail = 1;
		/* Fall through */
	    case	CRE_DISCONN_REC:
		mscp_dispatch( EV_PATHFAILURE, rp );
		break;
	    case	CRE_DISCONN_DONE:
		mscp_dispatch( EV_DISCOMPLETE, rp );
		break;
	    case	CRE_CREDIT_AVAIL:
		mscp_service_creditq( rp );
		break;

	    /* None of the following events should occur.
	     */
	    case	CRE_ACCEPT_DONE:
	    case	CRE_BLOCK_DONE:
	    case	CRE_CONN_REC:
	    case	CRE_REJECT_DONE:
	    default:
	    panic( "mscp_control: unexpected connection management event" );
	}
    }
    return;
}

/**/

/*
 *
 *   Name:	mscp_message - Message input routine
 *
 *   Abstract:	
 *		This routine handles end messages and attention
 *		messages arriving on a connection.  End messages are
 *		distinguished from attention messages by the presence
 *		of the end flag in the mscp opcode.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_message( csp )

    register CSB		*csp;
{
    register CONNB		*cp = ( CONNB * )csp->Aux;
    register MSCP		*mp = ( MSCP * )csp->buf;
    register REQB		*rp;
    register u_short		index, seq_no;

    /* If the message is an end message, interpret the command reference
     * number as a RSPID and break it into its component parts.
     */
    if( mp->mscp_opcode & MSCP_OP_END ) {
	index = (( RSPID * )&mp->mscp_cmd_ref )->index;
	seq_no = (( RSPID * )&mp->mscp_cmd_ref )->seq_no;

	/* If the RSPID index is out of range or the sequence number is not
	 * current, deallocate the message without further processing.
	 */
	if(( index >= nrspid ) ||
	   ( seq_no != mscp_rspid_tbl[ index ].rspid.seq_no )) {
	    printf( "mscp_message: dropping msg:  mp %x, op %x, RSPID %x\n",
		mp, mp->mscp_opcode, mp->mscp_cmd_ref );
	    if( scs_dealloc_msg( csp ) != RET_SUCCESS )
		panic( "mscp_message: scs_dealloc_msg failed\n" );
	    return;
	} else {
	    /* If the RSPID represents the oldest command outstanding in
	     * the controller, clear the oldest command field in the
	     * connection block to let connection management know that work
	     * is progressing.
	     */
	    if( mp->mscp_cmd_ref == *( u_long * )&cp->old_rspid )
		*( u_long * )&cp->old_rspid = NULL;

	    /* Get the request block pointer that corresponds to the RSPID,
	     * remove the request block from the connection active queue,
	     * store the MSCP message buffer address and size in the REQB,
	     * and call back the thread that was waiting for message completion.
	     */
	    rp = mscp_rspid_tbl[ index ].reqb;

	    if( mp->mscp_cmd_ref != *( u_long * )&rp->rspid ) {
		printf( "mscp_message: end msg rspid %x != rp rspid %x\n",
		    mp->mscp_cmd_ref, *( u_long * )&rp->rspid );
		panic( "mscp_message: invalid rspid\n");
	    }

	    Remove_entry( rp->flink );
	    rp->flink = NULL;
	    rp->blink = NULL;
	    rp->msgptr = mp;
	    rp->msgsize = csp->size;
	    mscp_dispatch( EV_ENDMSG, rp );
	}

    /* The message is not an end message.  Treat it as an attention message
     * and dispatch on MSCP opcode.
     */
    } else {
	switch( mp->mscp_opcode ) {
	    default:
		printf( "mscp_message:  unknown attention message 0x%2x\n",
			mp->mscp_opcode );
	    case MSCP_OP_AVATN:
		mscp_avail_attn( csp );
	    case MSCP_OP_DUPUN:
	    case MSCP_OP_ACPTH:
		if( scs_dealloc_msg( csp ) != RET_SUCCESS )
		    panic( "mscp_message: scs_dealloc_msg failed\n" );
	}
    }

    return;
}
/**/

/*
 *
 *   Name:	mscp_avail_attn- Process available attention messages
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_avail_attn( csp )
    register CSB		*csp;
{
    register CONNB		*cp = ( CONNB * )csp->Aux;
    register MSCP		*mp = ( MSCP * )csp->buf;
    UNITB			*up;

    up = mscp_get_unitb( cp, mp, csp->size );
}

/**/

/*
 *
 *   Name:	mscp_datagram - Datagram input routine
 *
 *   Abstract:	
 *		This routine handles error logging datagrams.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_datagram( csp )

    CSB			*csp;
{
	register CONNB *cp = (CONNB *)csp->Aux;
	register MSLG *mp = (MSLG *)csp->buf;
	register REQB *rp = &cp->bbrb->bbr_reqb; /* bbr request cmd */
	register MSLG *bbrmp = &cp->bbrb->bbr_mslg; /* bbr mslg */

	void mscp_logerr();

		/* error durning bbr cmd - log via bbr code */

	if ((cp->bbrb != 0) && (cp->bbrb->flags.bit.busy == 1) &&
	    (mp->mslg_cmd_ref == *(u_long *)&rp->rspid)) {

	    if (cp->bbrb->flags.bit.logerr == 0) {
		     *(bbrmp) = *(mp);
	    }
	}
	else {
	    mscp_logerr( cp, mp , csp->size);
	}

        csp->Nbufs = 0;
        scs_queue_dgs( csp );
        return;

}

/**/

/*
 *   Name:	mscp_logerr - mscp log error routine
 *
 *   Abstract:	
 *		This routine logs datagram error packets to the system
 *		error log.
 *
 *   Inputs:	cp - connection pointer
 *		mp - mslg packet pointer
 *		size - mslg packet size
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */
 
void
mscp_logerr( cp, mp, size)
    CONNB	*cp;
    MSLG	*mp;
    long	size;
{

	register struct	el_rec	*elp;
	int	class, type, devtype, unitnum, subidnum;

	if ((elp = ealloc((sizeof(struct el_bdev)), EL_PRIHIGH)) == EL_FULL)
		return;

	switch (mp->mslg_format) {

	case MSLG_FM_CNT_ER:
	case MSLG_FM_BUS_ADDR:

		class = ELCT_DCNTL;	    /* Device controller class	*/
		if (mp->mslg_cnt_id[7] == 3)
		    type = ELTMSCP_CNTRL;   /* TMSCP controller type 	*/
		else
		    type = ELMSCP_CNTRL;    /* MSCP controller type 	*/

		unitnum = cp->cnt_number;    /* Controller number	*/
		devtype = mp->mslg_cnt_id[6] & 0xFF;

		/* Need to find  adpt or bus # from ? */
		subidnum = EL_UNDEF; 	   /* adpt or bus controller #	*/

		break;

	case MSLG_FM_DISK_TRN:
	case MSLG_FM_SDI:
	case MSLG_FM_SML_DSK:
	case MSLG_FM_REPLACE:

		class = ELCT_DISK;		/* Disk class		*/
		type = ELDEV_MSCP;		/* MSCP disk type	*/
		devtype = (mp->mslg_unit_id[1] >> 16) & 0xFF;
		unitnum = mp->mslg_unit;	/* Unit number		*/
		subidnum = cp->cnt_number;      /* Controller number	*/
		break;

	case MSLG_FM_TAPE_TRN:
	case MSLG_FM_STI_ERR:
	case MSLG_FM_STI_DEL:
	case MSLG_FM_STI_FEL:
	case MSLG_FM_IBMSENSE:

		class = ELCT_TAPE;		/* Disk class		*/
		type = ELDEV_MSCP;		/* MSCP disk type	*/
		devtype = (mp->mslg_unit_id[1] >> 16) & 0xFF;
		unitnum = mp->mslg_unit;	/* Unit number		*/
		subidnum = cp->cnt_number;      /* Controller number	*/
		break;

	default:

		class = EL_UNDEF;		/* Unknown		*/
		type = EL_UNDEF;		/* Unknown		*/
		devtype = EL_UNDEF;		/* Unknown		*/
		unitnum = EL_UNDEF;		/* Unknown		*/
		subidnum = EL_UNDEF;		/* Unknown		*/
		break;
	}

	LSUBID(elp, class, type, devtype, subidnum, unitnum, 
		  (u_long)mp->mslg_format)

	/* What should go in here?   */
	elp->el_body.elbdev.eldevhdr.devhdr_dev = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_flags = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_bcount = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_blkno = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_retrycnt = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_herrcnt = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_serrcnt = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_csr = EL_UNDEF;

	elp->el_body.elbdev.eldevdata.elmslg.mslg_len = size;
	elp->el_body.elbdev.eldevdata.elmslg.mscp_mslg = *(mp);
	EVALID(elp);
}

/**/

/*
 *
 *   Name:	mscp_alloc_rspid - Allocate a Response ID
 *
 *   Abstract:	Allocate a Response ID (RSPID) for use as a command
 *		reference number.  If there are no RSPIDs available,
 *		insert the REQB onto the tail of the RSPID wait queue.
 *
 *   Inputs:	rp			Request block pointer
 *		    connb		Connection block pointer
 *			classb		Class block pointer
 *
 *   Outputs:   rp
 *		    rspid		The allocated RSPID
 *
 *
 *   Return	NONE
 *   Values:
 */

/* ** TO DO ** Put the RSPID table or a pointer to it in the CLASSB
 * so that it doesn't have to be referenced as a global variable.
 */

u_long
mscp_alloc_rspid( event, rp )
    u_long		event;
    register REQB	*rp;

{

    register RSPID_TBL	*rtp;
    register u_long	new_event = EV_RSPID;
 
    /* ** TEMP **  Panic if this reqb already has a RSPID.
     */
    if( *( u_long * )&rp->rspid ) {
	printf(" rp %x has non-zero RSPID %x\n", rp, rp->rspid );
	panic( "mscp_alloc_rspid: double RSPID allocation\n" );
    }

    /* If the RSPID wait queue is empty and there is an available RSPID,
     * store the REQB address in the RSPID table and copy the RSPID from
     * the RSPID table into the REQB.
     */
    if(( mscp_rspid_wait_lh.flink == ( QE * )&mscp_rspid_wait_lh.flink ) &&
       ((rtp = (RSPID_TBL *)mscp_rspid_lh.flink) != 
	( RSPID_TBL *)&mscp_rspid_lh )) {
	    Remove_entry( rtp->flink );
	    rtp->reqb = rp;
	    rp->rspid = rtp->rspid;

    /* If the wait queue is not empty or there are no available RSPIDs,
     * thread the REQB into the RSPID wait queue and bump the wait count
     * to stall requests on the unit.
     */
    } else {
	Insert_entry( rp->flink, mscp_rspid_wait_lh );
	Incr_rwait( rp );
	rspid_wq_cnt++;
	new_event = EV_NULL;
    }
    
    return( new_event );
}

/**/

/*
 *
 *   Name:	mscp_recycle_rspid - Recycle a Response ID
 *
 *   Abstract:	Recycle a response ID by updating its sequence number
 *		field.  Recycling has no effect on threads in the RSPID
 *		wait queue.
 *
 *   Inputs:	rp			Request block pointer
 *		    rspid		RSPID
 *		rtp			RSPID table pointer
 *		    rspid		RSPID
 *
 *   Outputs:	rp			Request block pointer
 *		    rspid		Updated RSPID.
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_recycle_rspid( rp )

    register REQB	*rp;
{
    register RSPID_TBL	*rtp;

    /* Locate the RSPID table entry that corresponds to the input RSPID.
     */
    rtp = &mscp_rspid_tbl[ rp->rspid.index ];
 
    /* If the sequence numbers don't agree, something has been corrupted.
     * Panic.
     */
    if( rtp->rspid.seq_no != rp->rspid.seq_no)
	panic( "mscp_recycle_rspid - sequence number error.\n" );

    /* Update the sequence number and copy the RSPID into the REQB.
     */
    else {
	if( ++rtp->rspid.seq_no == 0 )
	   ++rtp->rspid.seq_no;
	rp->rspid = rtp->rspid;
	return;
    }
}

/**/

/*
 *
 *   Name:	mscp_dealloc_rspid	- deallocate Response ID
 *
 *   Abstract:	Return a RSPID entry to the free queue and activate
 *		the first thread on the RSPID wait queue, if any.
 *
 *   Inputs:	rp			Request block pointer
 *		    rspid		Response ID
 *		rtp			RSPID table pointer
 *		    rspid		Response ID
 *		    flink		Forward link
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_dealloc_rspid( rp )

    register REQB	*rp;
{
    register RSPID_TBL	*rtp;
    register REQB	*wrp;
    register CLASSB	*clp = rp->classb;

    /* Locate the RSPID table entry that corresponds to the input RSPID.
     */
    rtp = &mscp_rspid_tbl[ rp->rspid.index ];
 
    /* If the sequence numbers don't agree, something has been corrupted.
     * Panic.
     */
    if( rtp->rspid.seq_no != rp->rspid.seq_no) {
	printf("mscp_dealloc_rspid: error rtp->rspid %x, rp->rspid %x\n",
	    *( u_long * )&rtp->rspid, *( u_long * )&rp->rspid);
	panic( "mscp_dealloc_rspid:  sequence number mismatch\n" );

    /* Update the sequence number in the RSPID table and zero the
     * RSPID field in the request block as a safety precaution.
     */
    } else {
	if( ++rtp->rspid.seq_no == 0 )
	    ++rtp->rspid.seq_no;
	rp->rspid.index = 0;
	rp->rspid.seq_no = 0;

	/* If there is a request block waiting for a RSPID, do the
	 * required bookkeeping and dispatch the waiting thread.
	 */
        if(( wrp = (REQB *)mscp_rspid_wait_lh.flink ) != (REQB *)&mscp_rspid_wait_lh ) {
            Remove_entry( wrp->flink );
	    rtp->reqb = wrp;
	    wrp->rspid = rtp->rspid;
	    mscp_dispatch( EV_RSPID, wrp );
	    Decr_rwait( wrp );

	/* No waiters.  Clear the request block pointer in the RSPID
	 * table and put the deallocated entry in the free list.
	 */
	} else {
	    rtp->reqb = NULL;
	    Insert_entry( rtp->flink, mscp_rspid_lh );
	}

	return;
    }
}

/**/

/*
 *
 *   Name:	mscp_alloc_msg - Allocate a sequenced message buffer
 *
 *   Abstract:	Allocate a sequenced message buffer via SCS.  If
 *		the allocation fails because of a shortage of
 *		buffers, insert the REQB on the buffer wait queue.
 *		If the allocation fails for any other reason, panic.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

u_long
mscp_alloc_msg( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register CONNB		*cp = rp->connb;
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    register u_long		new_event = EV_MSGBUF;

    /* ** TEMP **  Panic if this reqb already has a msg buffer.
     */
    if( rp->msgptr ) {
	printf(" rp %x has non-zero msgptr %x\n", rp, rp->msgptr );
	panic( "mscp_alloc_msg: double msg buffer allocation\n" );
    }

    /* Store the connection ID in the CSB.  If the message wait queue
     * is empty, call SCS to allocate a message buffer.  If the allocation
     * is successful, fill in the message buffer pointer in the request
     * block, and return a message buffer available event to the caller.
     */
    Move_connid( cp->connid, csp->connid );
    if(( cp->buffer_wq.flink == ( REQB * )&cp->buffer_wq.flink ) &&
       ( scs_alloc_msg( csp ) == RET_SUCCESS )) {
	    rp->msgptr = ( MSCP * )csp->buf;

    /* If the wait queue is not empty or if allocation fails for any
     * reason, insert the REQB at the tail of the message buffer wait
     * queue for the connection, increment the wait reasons counter to
     * stall new activity, and return a null event to the caller.
     */
    } else {
	Insert_entry( rp->flink, cp->buffer_wq );
	Incr_rwait( rp );
	new_event = EV_NULL;
    }

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_dealloc_msg - Deallocate a sequenced message buffer
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_dealloc_msg( rp )
    register REQB		*rp;
{
    register REQB		*wrp;
    register CONNB		*cp = rp->connb;
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    u_long			scs_status;

    /* Return the buffer to SCS.
     */
    Move_connid( cp->connid, csp->connid );
    csp->buf = ( u_char * )rp->msgptr;
    rp->msgptr = NULL;
    if(( scs_status = scs_dealloc_msg( csp )) != RET_SUCCESS ) {
	printf( "mscp_dealloc_msg: scs_status %x\n", scs_status );
	panic( "mscp_dealloc_msg: bad connection state or ID\n" );
    }

    /* If there is a request packet waiting for a buffer, try to get
     * the buffer that was just given back.  If the allocation request
     * succeeds, restart the waiting thread with a message buffer 
     * available event.
     */
    if(( wrp = cp->buffer_wq.flink ) != ( REQB * )&cp->buffer_wq.flink ) {
	if(( scs_status = scs_alloc_msg( csp )) == RET_SUCCESS ) {
	    Remove_entry( wrp->flink );
	    wrp->msgptr = ( MSCP * )csp->buf;
	    mscp_dispatch( EV_MSGBUF, wrp );
	    Decr_rwait( wrp );
	}
    }

    return;

}
/**/

/*
 *
 *   Name:	mscp_service_bufferq - get buffers for waiting requests
 *
 *   Abstract:	This routine is called from mscp_timer to attempt to
 *		service requests that are waiting for message buffers.
 *		It is possible for a shared message buffer resource to
 *		become available on one connection without notification
 *		of waiting requests on other connections; this routine is
 *		periodically invoked to deal with that eventuality.
 *
 *   Inputs:    cp		    Connection block pointer.
 *		    buffer_wq.flink Map wait queue of request blocks.
 *
 *   Outputs:	
 *		rp		    Request block pointer.
 *		    msgptr
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_service_bufferq( cp )
    register CONNB		*cp;
{
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    register REQB		*wrp;
    u_long			status;

    Move_connid( cp->connid, csp->connid );

    /* Issue an allocate message request to SCS for each waiting REQB
     * in turn until the queue is empty or until a request fails.  For
     * each successful buffer allocation, do the wait queue bookkeeping
     * and dispatch the REQB thread with the message buffer pointer
     * in hand.
     */
    for( wrp = cp->buffer_wq.flink;
	 wrp != (REQB * )&cp->buffer_wq.flink;
	 wrp = cp->buffer_wq.flink ) {

	if( scs_alloc_msg( csp ) == RET_SUCCESS ) {
	    Remove_entry( wrp->flink );
	    wrp->msgptr = ( MSCP * )csp->buf;
	    mscp_dispatch( EV_MSGBUF, wrp );
	    Decr_rwait( wrp );

	} else {
	    break;
	}
    }
    return;
}

/**/

/*
 *
 *   Name:	mscp_map_buffer - map a data buffer
 *
 *   Abstract:	Allocate mapping resources for an MSCP data transfer
 *		operation.
 *
 *   Inputs:    rp			Request block pointer.
 *
 *   Outputs:	rp			Request block pointer.
 *		    lbhandle		Local buffer handle.
 *
 *
 *   Return	NONE
 *   Values:
 */

u_long
mscp_map_buffer( event, rp )
    register REQB		*rp;
{
    register CONNB		*cp = rp->connb;
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    u_long			new_event = EV_MAPPING;

    /* ** TEMP **  Panic if this reqb already has a buffer handle.
     */
    if( !Test_bhandle( rp->lbhandle )) {
	printf(" rp %x has non-zero buffer handle\n", rp );
	panic( "mscp_map_buffer: double buffer handle allocation\n" );
    }

    /* *** TEMP ***
     * Zero the buffer handle while we figure out who should really do it.
     */
    Zero_bhandle( csp->lbhandle );

    /* Store the connection ID and the buf structure pointer in the CSB.
     * If the map wait queue is empty, call SCS to map the buffer.  If
     * the map request succeeds, store the local buffer handle in the
     * request block and return a MAPPING event to the caller.
     */
    Move_connid( cp->connid, csp->connid );
    csp->Sbh = rp->bufptr;
    if(( cp->map_wq.flink == ( REQB * )&cp->map_wq.flink ) &&
       ( scs_map_buf( csp ) == RET_SUCCESS )) {
	    rp->lbhandle = csp->lbhandle;

    /* If the map queue is not empty or the request fails for any reason,
     * queue the request block, stall incoming requests and return a NULL
     * event to the caller.
     */
    } else {
	Insert_entry( rp->flink, cp->map_wq );
	Incr_rwait( rp );
	new_event = EV_NULL;
    }

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_unmap_buffer - unmap a data buffer
 *
 *   Abstract:	Deallocate mapping resources after completion of
 *		an MSCP data transfer operation.
 *
 *   Inputs:    rp			Request block pointer.
 *		    lbhandle		Local buffer handle.
 *
 *   Outputs:	rp			Request block pointer.
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_unmap_buffer( rp )
    register REQB		*rp;
{
    register CONNB		*cp = rp->connb;
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    register REQB		*wrp;
    u_long			status;

    Move_connid( cp->connid, csp->connid );
    csp->lbhandle = rp->lbhandle;
    csp->Sbh = rp->bufptr;

    if(( status = scs_unmap_buf( csp )) == RET_SUCCESS ) {

	/* Zero the buffer handle to prevent its inadvertant reuse.
	 */
	Zero_bhandle( rp->lbhandle );

	/* If there are REQBs waiting for mapping resources, issue a map 
	 * request for each one in turn until the wait queue is empty or
	 * until a map request fails.
	 */
	for( wrp = cp->map_wq.flink;
	     wrp != (REQB * )&cp->map_wq.flink;
	     wrp = cp->map_wq.flink ) {

	    csp->Sbh = wrp->bufptr;

	    /* If the map request succeeds, do the wait queue bookkeeping
	     * and dispatch the REQB thread with the local buffer handle
	     * in hand.
	     */
	    if( scs_map_buf( csp ) == RET_SUCCESS ) {
	        Remove_entry( wrp->flink );
		wrp->lbhandle = csp->lbhandle;
		mscp_dispatch( EV_MAPPING, wrp );
		Decr_rwait( wrp );

	    /* If the map request failed, break out of the for loop.
	     */
	    } else
		break;
	}
        return;

    } else
	panic( "mscp_unmap_buffer: bad connection state or ID\n" );
}
/**/

/*
 *
 *   Name:	mscp_service_mapq - attempt to map waiting requests
 *
 *   Abstract:	This routine is called from mscp_timer to attempt to
 *		service requests that are waiting for mapping resources.
 *		It is possible for a shared mapping resource to become
 *		available on one connection without notification of
 *		waiting requests on other connections; this routine is
 *		periodically invoked to deal with that eventuality.
 *
 *   Inputs:    cp		    Connection block pointer.
 *		    map_wq.flink    Map wait queue of request blocks.
 *		        lbhandle    Local buffer handle.
 *
 *   Outputs:	
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_service_mapq( cp )
    register CONNB		*cp;
{
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    register REQB		*wrp;
    u_long			status;

    Move_connid( cp->connid, csp->connid );

    /* Issue a map request for each waiting REQB in turn until the queue
     * is empty or until a map request fails.
     */
    for( wrp = cp->map_wq.flink;
	 wrp != (REQB * )&cp->map_wq.flink;
	 wrp = cp->map_wq.flink ) {

	csp->Sbh = wrp->bufptr;

	/* If the map request succeeds, do the wait queue bookkeeping
	 * and dispatch the REQB thread with the local buffer handle
	 * in hand.
	 */
	if( scs_map_buf( csp ) == RET_SUCCESS ) {
	    Remove_entry( wrp->flink );
	    wrp->lbhandle = csp->lbhandle;
	    mscp_dispatch( EV_MAPPING, wrp );
	    Decr_rwait( wrp );

	/* If the map request failed, break out of the for loop.
	 */
	} else 
	    break;
    }
    return;
}

/**/

/*
 *
 *   Name:	mscp_send_msg - Send an MSCP sequenced message
 *
 *   Abstract:	Fill in the appropriate CSB fields, and send a 
 *		MSCP sequenced message across a connection.
 *
 *   Inputs:	rp			Request block pointer
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:
 */
u_long
mscp_send_msg( rp )
    register REQB		*rp;
{
    register CONNB		*cp = rp->connb;
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    register u_long		status;
    register u_long		new_event = EV_NULL;

    /* Fill in the Communications services block portion of the
     * request block with the connection ID, a pointer to the
     * MSCP message buffer, the maximum MSCP command message size,
     * and the message buffer disposition code.
     */
    Move_connid( cp->connid, csp->connid);
    csp->buf = ( u_char * )rp->msgptr;
    csp->size = sizeof( MSCP_CMDMSG );
    csp->Disposal = RECEIVE_BUF;

    /* If the credit wait queue is empty or the request is a connection
     * management immediate message call SCS to send the message. If
     * scs_send_msg returned success, queue the request to the active
     * queue, and zero out the message buffer pointer to prevent its
     * inadvertent reuse.
     */
    if(( cp->credit_wq.flink == ( REQB * )&cp->credit_wq.flink ||
	 rp->flags.nocreditw ) &&
	 ( status = scs_send_msg( csp )) == RET_SUCCESS ) {
	Insert_entry( rp->flink, cp->active );
	rp->msgptr = NULL;

    /* If the request represents a connection management message, the
     * scs_send_msg did not succeed.  Call SCS to add back the credit
     * reserved for connection management, and call SCS to try again to
     * send the message.  (The reserved credit must be added back here
     * rather than above in order to avoid using the last credit for a
     * non-immediate command, which would violate the MSCP spec.)  If
     * either call fails, return a no credits event.  Otherwise, add the
     * request to the active queue, and zero out the message buffer pointer
     * to prevent its inadvertent reuse.
     */
    } else if( rp->flags.nocreditw ) {
	if( scs_add_credit( csp ) == RET_SUCCESS  && 
	    ( status = scs_send_msg( csp )) == RET_SUCCESS ) {
	    cp->flags.need_cr = 1;
	    Insert_entry( rp->flink, cp->active );
	    rp->msgptr = NULL;
	} else {
	    new_event = EV_NOCREDITS;
	}

    /* The credit wait queue isn't empty or scs_send_msg returned an
     * error on a non privileged request. Add the request to the credit
     * wait queue and increment the resource wait count for the unit.
     */
    } else {
	Insert_entry( rp->flink, cp->credit_wq );
	Incr_rwait( rp );
    }

    /* Return status to the caller.
     */
    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_service_creditq - service requests in credit wait
 *
 *   Abstract:	This routine is called from mscp_control to service
 *		requests in the credit wait queue.
 *
 *   Inputs:    cp		    Connection block pointer.
 *		    credit_wq.flink Map wait queue of request blocks.
 *
 *   Outputs:	
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_service_creditq( rp )
    register REQB		*rp;
{
    register CONNB		*cp = rp->connb;
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    u_long			status;

    /* If the connection management reserved credit has been expended,
     * reserve it again.
     */
    if( cp->flags.need_cr ) 
	mscp_reserve_credit( rp );

    /* Issue a send message for each waiting REQB in turn until the queue
     * is empty or until a send message fails.
     */
    for( rp = cp->credit_wq.flink;
	 rp != (REQB * )&cp->credit_wq.flink;
	 rp = cp->credit_wq.flink ) {

	/* Fill in the CSB request block with the connection ID,
	 * a pointer to the MSCP message buffer, the maximum MSCP
	 * command message size, and the message buffer disposition
	 * code.
	 */
	Move_connid( cp->connid, csp->connid);
	csp->buf = ( u_char * )rp->msgptr;
	csp->size = sizeof( MSCP_CMDMSG );
	csp->Disposal = RECEIVE_BUF;


	/* Attempt to send the sequenced message. If scs_send_msg
	 * returns successfully, remove the request from the credit
	 * wait queue, decrement the wait counter, queue the request
	 * to the controller active queue, and zero out the message
	 * buffer pointer to prevent its inadvertent reuse.
	 */
	if(( status = scs_send_msg( csp )) == RET_SUCCESS ) {
	    Remove_entry( rp->flink );
	    Decr_rwait( rp );
	    Insert_entry( rp->flink, cp->active );
	    rp->msgptr = NULL;

	/* If scs_send_message returned an error, break out of the loop.
	 */
	} else {
	    break;
	}

    }

    return;
}

/**/

/*
 *
 *   Name:	mscp_reserve_credit - reserve a send credit
 *
 *   Abstract:	Reserve a send credit for use by connection management.
 *
 *   Inputs:    rp			Request block pointer.
 *
 *   Outputs:	
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_reserve_credit( rp )
    register REQB		*rp;
{
    register CONNB		*cp = rp->connb;
    CSB				csb;
    register CSB		*csp = ( CSB * )&csb;
    u_long			scs_status;

    Move_connid( cp->connid, csp->connid );
    if(( scs_status = scs_rsv_credit( csp )) == RET_SUCCESS ) {
	cp->flags.need_cr = 0;
    }
}
/**/

/*
 *
 *   Name:	mscp_unstall_unit - activate stalled requests on a unit
 *
 *   Abstract:	This routine dispatches requests which were stalled on
 *		a unit's request queue.
 *
 *   Inputs:    up			Unit block pointer.
 *
 *   Outputs:	NONE.
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_unstall_unit( up )
    register UNITB		*up;
{
    register REQB		*rp;

    while(( up->rwaitct == 0) &&
           (( rp = up->request.flink ) != ( REQB * )&up->request )) {
        Remove_entry( rp->flink );
	mscp_dispatch( EV_INITIAL, rp );
    }
}

/**/

/*
 *
 *   Name:	mscp_alloc_reqb - allocate a request block
 *
 *   Abstract:	Allocate and initialize a request block and return its
 *		address to the caller.  If no request block is available,
 *		return NULL to the caller.
 *
 *   Inputs:	up			Unit block pointer.
 *		bp			Buf structure pointer or NULL.
 *		stbl			State table used by request.
 *		p1			Function-dependent parameter 1. 
 *		p2			Function-dependent parameter 2. 
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE
 *   Values:
 */

REQB *
mscp_alloc_reqb( up, bp, stbl, p1, p2 )
    register UNITB		*up;
    register struct buf		*bp;
    STATE			*stbl;
    u_long			p1;
    u_long			p2;

{
    register REQB		*rp;
    register CLASSB		*clp = up->connb->classb;
    int				saved_ipl;

    /* Allocate a request block (sleeping until it becomes available),
     * clear and format it.  Then pass control to a functional routine
     * to start a sequence of MSCP operations or queue the request if
     * activity on the unit is stalled.
     */
    saved_ipl = Splscs();

    while( 1 ) {
	KM_ALLOC( rp, REQB *, sizeof( REQB ), KM_CDRP, KM_NOW_CL );
	if( rp != NULL )
	    break;
	timeout( wakeup, ( caddr_t )up, 5 * hz );
	( void )sleep(( caddr_t )up, PSWP+1 );
	untimeout( wakeup, ( caddr_t )up );
    }

    rp->unitb = up;
    rp->connb = up->connb;
    rp->classb = clp;
    rp->bufptr = bp;
    rp->p1 = p1;
    rp->p2 = p2;
    rp->op_seq_num = clp->operation_ct++;
    rp->rwaitptr = &up->rwaitct;
    rp->state_tbl = stbl;

    if( up->rwaitct == 0 ) {
	mscp_dispatch( EV_INITIAL, rp );
    } else {
	Insert_entry( rp->flink, up->request );
    }

    splx( saved_ipl );
    return( rp );
}
/**/

/*
 *
 *   Name:	mscp_dealloc_reqb - deallocate a request block
 *
 *   Abstract:	Deallocate a request block and all of the resources
 *		it holds.
 *
 *   Inputs:    rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_dealloc_reqb( rp )
    register REQB		*rp;
{
    register CONNB		*cp = rp->connb;
    register struct buf		*bp;

    /* Deallocate all resources held by the request block.
     */
    mscp_dealloc_all( rp );

    /* If there is a buf structure pointer associated with the operation,
     * fill in the residual byte count and issue an iodone on the buffer.
     */
    if( bp = rp->bufptr ) {
	bp->b_resid = rp->p1;
	iodone( bp );
	rp->bufptr = NULL;
    } else {
	wakeup(( caddr_t )rp );
    }

    /* If the request block is not permanently allocated, deallocate it.
     */
    if( !rp->flags.perm_reqb ) {
	KM_FREE( rp, KM_CDRP );
    }

    /* If the connection is in single stream mode, attempt to restart
     * the next request on the restart queue.
     */
    if( cp->flags.sngl_strm )
	mscp_restart_next( cp );

    return;
}

/**/

/*
 *
 *   Name:	mscp_restart_next - Restart next single streamed request
 *
 *   Abstract:	This routine is called during connection recovery to
 *		restart the next request block on the restart queue.
 *		Requests on the restart queue are retried one at a time
 *		in order to isolate and eliminate the command(s) which
 *		may have caused the connection failure.
 *
 *   Inputs:    cp			Connection block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_restart_next( cp )
    register CONNB		*cp;
{
    register REQB		*rp;
    register struct buf		*bp;
    register UNITB		*up;

    /* Get the next request on the restart queue, if any.
     */
    while(( rp = cp->restart.flink ) != ( REQB * )&cp->restart ) {
        Remove_entry( rp->flink );

	/* If this is the first time through restart for this request,
	 * set the single stream flag, update the pointer to the request
	 * being restarted and set the restart command retry count to its
	 * maximum value.
	 */
	if( !cp->flags.sngl_strm || rp != cp->restart_reqb ) {
	    cp->flags.sngl_strm = 1;
	    cp->restart_reqb = rp;
	    cp->restart_count = COMMAND_RETRIES;

	/* This request has been through restart before.  Decrement the
	 * restart retry count.
	 */
	} else {
	    --cp->restart_count;
	}

	/* If any retries remain, set the state to INITIAL, dispatch
	 * the request and break out of the while loop.
	 */
	if( cp->restart_count ) {
	    rp->state = ST_CMN_INITIAL;
	    mscp_dispatch( EV_INITIAL, rp );
	    break;

	/* This request has repeatedly caused the connection to be lost.
	 * If the request has a buf structure associated with it, fill
	 * in the appropriate fields in the buf structure.  Then deallocate
	 * the request block and associated resources.
	 */
	} else {
	    if( bp = rp->bufptr ) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		rp->p1 = bp->b_bcount;
	    }

	    mscp_dealloc_reqb( rp );
	}
    }
    

    /* If the restart queue is empty, reset single stream mode then scan
     * the list of unit blocks on the connection and decrement the wait
     * count for each, unstalling the units as appropriate.  When the
     * last unit has been seen, reset the connection restart flag.
     */
    if( cp->restart.flink == ( REQB * )&cp->restart ) {
	cp->flags.sngl_strm = 0;
	for( up = cp->unit.flink;
	     up != ( UNITB * )&cp->unit.flink;
	     up = up->flink ) {
	    if( up->flags.wait_bump ) {
		up->flags.wait_bump = 0;
		--up->rwaitct;
	    }
	    if( up->rwaitct == 0 )
		mscp_unstall_unit( up );
	}
	cp->flags.restart = 0;
    }

    return;
}


/**/

/*
 *
 *   Name:	dealloc_all - Deallocate all resources held by REQB
 *
 *   Abstract:	Deallocate any resource that is associated with the
 *		input REQB.  Resources may include RSPID, message
 *		buffer or mapping information (local buffer handle).
 *
 *   Inputs:	rp			REQB pointer
 *		    rspid		RSPID
 *		    msgptr		message buffer pointer
 *		    lbhandle		local buffer handle
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_dealloc_all( rp )
    REQB		*rp;
{
    /* If there is a RSPID in the request block, deallocate it.
     */
    if( *( u_long * )&rp->rspid )
	mscp_dealloc_rspid( rp );

    /* If there is a non-NULL message buffer pointer in the REQB,
     * deallocate the message buffer.
     */
    if( rp->msgptr )
	mscp_dealloc_msg( rp );

    /* If there is a non-NULL local buffer handle in the REQB,
     * deallocate it.
     */
    if( !Test_bhandle( rp->lbhandle ))
	mscp_unmap_buffer( rp );
    return;
}
/**/

/*
 *
 *   Name:	mscp_media_to_ascii - Convert MSCP media code to ASCII.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_media_to_ascii( media, ascii )
    register u_long		media;
    register u_char		*ascii;

{
    register u_long		temp;

    *ascii++ = (( media >> 17 ) & 0x1f ) + 'A' - 1;
    if( temp = (( media >> 12 ) & 0x1f )) {
	*ascii++ = ( temp + 'A' - 1 );
	if( temp = (( media >> 7 ) & 0x1f ))
	    *ascii++ = ( temp + 'A' - 1 );
    }
    *ascii++ = (( media & 0x7f ) / 10 ) + '0';
    *ascii++ = (( media & 0x7f ) % 10 ) + '0';
    *ascii = '\0';
}

/**/

/*
 *
 *   Name:	mscp_common_init - Do tape and disk common initialization.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

void
mscp_common_init()

{
    register	int		i;
    register	RSPID_TBL	*rtp;
    int		s;

    s = Splscs();
    if( !mscp_gbl_flags ) {
	for( i = 0, rtp = mscp_rspid_tbl;
	     i < nrspid;
	     i++, rtp++ ) {
	    Insert_entry( rtp->flink, mscp_rspid_lh );
	    rtp->reqb = NULL;
	    rtp->rspid.index = i;
	    rtp->rspid.seq_no = 1;
	}
	mscp_gbl_flags = 1;
    }
    splx( s );
    return;
}

/**/

/*
 *
 *   Name:	mscp_poll_wait 
 *
 *   Abstract:	- wait up to 15*count seconds for mscp polling to complete. 
 *      	Global variable mscp_polls is incremented for each
 *      	connection attempt and decremented for each connection 
 *		completion.  If there are no known controllers left to
 *		poll (mscp_polls == 0) then delay one more iteration to
 *		allow recognition of controllers which are slow in making
 *		their presence known.
 *
 *   Inputs:    count - specifies the number of 15 second intervals which are
 *		granted to allow controllers time to report their presence.
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

#ifdef vax
int mscp_polls = 0;
int pollwait = 0;
#else
volatile int mscp_polls = 0;
volatile int pollwait = 0;
#endif vax

void
mscp_poll_wait(  count )
int count;
{
    if((( mscp_polls > 0 ) && ( count-- > 0 )) ||
	(( mscp_polls == 0 ) && ( pollwait == 0 ))) {
	( void )timeout( mscp_poll_wait, ( caddr_t )count, hz*15 );
	if ( pollwait++ == 0)  {
	    while ( pollwait ) {}
	}
    } else {
	pollwait = 0;
    }
}
