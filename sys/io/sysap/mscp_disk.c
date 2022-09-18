#ifndef	lint
static char *sccsid = "@(#)mscp_disk.c	4.3    (ULTRIX)        4/4/91";
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
 *   Abstract:	This module contains the standard ULTRIX "top half"
 *		routines and other routines specific to the disk
 *		variant of MSCP.
 *
 *   Author:	David E. Eiche	Creation Date:	September 30, 1985
 *
 *   History:
 *
 *   02-Apr-1991	Tom Tierney
 *	Fixed recovery problem: mscp_markoffline routine was updated to
 *	check if we are currently in recovery processing and if so, to 
 *	return correct new event to continue recovery processing along
 *	with notification via printf of error.  This corrects a problem
 *	where the system would hang if recovery failed for a particular
 *	disk.
 *
 *   22-Feb-1991	Brian Nadeau
 *	Allow up to 2 minutes for critical devices (swap, dump, root) to
 *	become available and print a status message after we wait the first
 *	minute.
 *
 *   09-Nov-1989	David E. Eiche		DEE0083
 *	Fix status subcode test to use MSCP_SC_EXUSE instead of MSCP_ST_OFFLN.
 *
 *   23-Oct-1989	Tim Burke
 *	Added support for the exclusive access unit attribute.  This involved
 *	adding the DKIOCEXCL ioctl, the set unit characteristics state table
 *	and routines and modifiying online and available command routines.
 *
 *   09-Aug-1989	Tim Burke		
 *	Added the DEVGETGEOM ioctl which is used to pass disk geometry 
 *	information.  Removed all references to dk_busy because it is no
 *	longer used.
 *
 *   08-Apr-1989	Tom Kong
 *	In mscp_close, took out the floating point operation for mips.
 *
 *   15-Mar-1989	Tim Burke
 *	Changed splx( IPL_SCS ) to Splscs();
 *
 *   07-Mar-1989	Todd M. Katz		TMK0002
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   17-Oct-1988	Pete Keilty
 *	Changed mscp_open and mscp_close now does explicit wakeup on
 *	up at end of routine.
 *
 *   27-Sep-1988	David E. Eiche		DEE0057
 *	Modified panic message formats to make them consistent.
 *	
 *   13-Sep-1988	Pete Keilty
 *	Changed transferem routine so the requested lbn is not
 *	over written on a BBR status return which was causing the 
 *	wrong lbn to be re-read.
 *
 *   19-Aug-1988	Pete Keilty
 *	Added synchronization code to the open and close routine.
 *	New flags busy and close_ip.
 *
 *   05-Aug-1988	Pete Keilty
 *	Change mscp_close added sleep on rp. Corrects case where
 *	an open_ip flags is clear out when it should not have been
 *	durning a open.
 *
 *   17-Jul-1988	David E. Eiche		DEE0045
 *	Change the get device information ioctl code to use the
 *	connection block as the source of its controller model
 *	information.
 *
 *   08-Jul-1988	Pete Keilty
 *	Added accscancm and accscanem routines, ioctl ACC_SCAN code,
 * 	and changes force_ip to force_scan_ip where used.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   16-May-1988	Stephen Reilly
 *	Close is only called once per device.
 *
 *   25-Apr-1988	Robin
 *	Added code in ioctl to stop bbr from starting on controllers that do
 *	BBR. Also added code to keep track of I/O which is used by iostat(1).
 *
 *   19-Apr-1988	Robin
 *	To stop a unit from going avail when it is still in use the
 *	partition mask was changed to an open counter.  A unit can have
 *	several opens on one or more partitions and the unit should
 *	never go avail unless all opens are followed by a close.
 *
 *   07-Apr-1988	David E. Eiche		DEE0027
 *	Fix DEE0022 to not panic when booting from an HSC that doesn't
 *	come online.
 *
 *   03-Apr-1988	David E. Eiche		DEE0024
 *	Update DEE0013 to make write protection work with open NDELAY.
 *
 *   03-Apr-1988	David E. Eiche		DEE0023
 *	Add code to implement exclusive access to disk units, leaving
 *	it commented out pending action by the HSC group.
 *
 *   03-Apr-1988	David E. Eiche		DEE0022
 *	Reorder mscp_open routine, eliminate the mscp_onlineinit
 *	routine and references to it, and add an additional test
 *	to mscp_size, to eliminate another race window caused by
 *	the interaction of open NDELAY and close.
 *
 *   23-Mar-1988	David E. Eiche		DEE0019
 *	Fix race between open and close in which a partition was
 *	reopened before the close processing had completed.
 *
 *   21-Mar-1988	David E. Eiche		DEE0018
 *	Change the mscp_open routine to process NDELAY flag in the
 *	historically approved manner.
 *
 *   17-Mar-1988	David E. Eiche		DEE0017
 *	Add jacket routines mscp_bopen, mscp_copen, mscp_bclose and
 *	mscp_cclose whose purpose is to pass a flag into mscp_open and
 *	mscp_close indicating whether the operation is being done in
 *	raw or block mode.  The flag is used with the partition	index
 *	portion of the dev parameter to determine which partitions are
 *	active.
 *
 *   12-Feb-1988	David E. Eiche		DEE0013
 *	Change mscp_onlgtuntem and mscp_ioctl to detect and report
 *	write protection status correctly.
 *	
 *   02-Feb-1988	David E. Eiche		DEE0011
 *	Change mscp_strategy to sleep if the request block is not
 *	available immediately.  Also remove code that initialized
 *	unused request block wait queue.
 *
 *   15-Jan-1988	Todd M. Katz		TMK0001
 *	Include new header file ../vaxmsi/msisysap.h.
 */
/**/


/* Libraries and Include Files.
 */
#include	"../h/dk.h"
#include	"../h/types.h"
#include	"../h/time.h"
#include	"../h/param.h"
#include	"../h/buf.h"
#include	"../h/errno.h"
#include	"../h/ioctl.h"
#include	"../h/devio.h"
#include	"../h/dkio.h"
#include	"../h/file.h"
#include	"../h/conf.h"
#include	"../fs/ufs/fs.h"
#include	"../h/errlog.h"
#include	"../machine/pte.h"
#include	"../h/vmmac.h"
#include	"../io/scs/sca.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/uba/ubavar.h"
#include	"../io/sysap/mscp_msg.h"
#include	"../io/sysap/mscp_defs.h"



/* External Variables and Routines.
 */
extern	REQB 		*mscp_alloc_reqb();
extern	u_long		mscp_alloc_rspid();
extern	u_long		mscp_bbr_online();
extern	u_long		mscp_bbr_replace();
extern	void		mscp_common_init();
extern	void		mscp_control();
extern	void		mscp_datagram();
extern	void		mscp_dealloc_reqb();
extern	void		mscp_getdefpt();
extern	void		mscp_media_to_ascii();
extern	void		mscp_message();
extern	u_long		mscp_recycle_rspid();
extern	void		mscp_restart_next();
extern	u_long		mscp_send_msg();
extern	void		mscp_system_poll();
extern	void		mscp_timer();
extern	CLASSB		mscp_classb;
extern	UNITB		*mscp_unit_tbl[];
extern	RSPID_TBL	mscp_rspid_tbl[];
extern	LISTHD		mscp_rspid_lh;
extern	STATE		mscp_avl_states[];
extern	STATE		mscp_onl_states[];
extern	STATE		mscp_rec_states[];
extern	STATE		mscp_accscan_states[];
extern	STATE		mscp_stu_states[];
extern	STATE		mscp_xfr_states[];
extern	STATE		mscp_repl_states[];
extern	DMSCP_MEDIA	dmscp_media[];

extern	int		dmscp_media_ct;
extern	int		hz;
extern	int		lbolt;
extern	int		wakeup();
extern	dev_t		rootdev;
extern	dev_t		dumpdev;
extern	struct	swdevt	swdevt[];

	UNITB		*mscp_check_sysdev();

/**/

/*
 *
 *   Name:	mscp_init_driver	- Initialize class driver
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

/* TODO (?) Should initial sequence number be randomized?
 */

void
mscp_init_driver()
{
    register int	i;
    int			s;
    u_short		init_seq_no = 1;
    register CLASSB	*clp = &mscp_classb;
    register CMSB	*cmsp = &clp->cmsb;

    /* Do initialization common to disk and tape class drivers.
     */
    mscp_common_init();

    /* Prevent re-entry during initialization
     */
    s = Splscs();
    if ( clp->flags.init_ip || clp->flags.init_done ) {
	( void )splx( s );
	return;
    }
    /* Inititialize class block flags
     */
    clp->flags.init_ip = 1;
    clp->flags.disk = 1;

    /* Init class driver block listheads
     */
    clp->flink = ( CONNB * )&clp->flink;
    clp->blink = ( CONNB * )&clp->flink;

    /* Zero and fill in the embedded connection management
     * services block (CMSB).
     */
    cmsp->control = mscp_control;
    cmsp->msg_event = mscp_message;
    cmsp->dg_event = mscp_datagram;
    cmsp->lport_name = 0;
    Zero_scaaddr( cmsp->rport_addr );
    cmsp->init_dg_credit = 2;
    cmsp->min_snd_credit = 2;
    cmsp->init_rec_credit = 10;
    ( void )bcopy( "U32_DISK_CL_DRVR",
		    cmsp->lproc_name,
		    NAME_SIZE );
    ( void )bcopy( "MSCP$DISK       ",
		    cmsp->rproc_name,
		    NAME_SIZE );
    ( void )bcopy( "                ",
		    cmsp->conn_data,
		    DATA_SIZE );

    /* Initialize the unit table and point the class block at it.
     */
    for( i = 0; i < NUNIT; i++ )
	mscp_unit_tbl[ i ] = ( UNITB * )NULL;

    clp->unit_tbl = mscp_unit_tbl;

    /* Initialize the ULTRIX device disk device name string.
     */
    clp->dev_name = "ra";

    /* Initialize recovery state table pointer
     */
    clp->recov_states = mscp_rec_states;

    /* Start up a 1 second timer for use by connection management and
     * the resource allocation routines.
     */
    ( void )timeout( mscp_timer, ( caddr_t )clp, hz );

    /* Find all the currently known subsystems, start the
     * connection process (which will complete asynchronously),
     * restore the entry IPL and exit.
     */
    mscp_system_poll( clp );
    ( void )splx( s );
    return;
}

/**/

/*
 *
 *   Name:	mscp_onlinecm - Format and send an MSCP ONLIN command.
 *
 *   Abstract:	Format and queue an MSCP online command to be sent.
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_onlinecm( event, rp )

    u_long			event;
    register REQB		*rp;
{
    register MSCP		*mp = rp->msgptr;
    register UNITB		*up = rp->unitb;
    register u_long		new_event;

    /* Format the message buffer as a ONLINE command with the exclusive
     * access modifier.  Update the state and send the message, then exit
     * to wait for the ONLINE end message to come back.
     * If excl_acc is specified then set the unit into the exclusive access
     * pseudo state.  The unit will remain in exclusive access mode if the
     * modifier was previously set.  If the exclusive access modifier is not
     * set and the unit is presently in the exclusive access state the unit
     * will no longer be in exclusive access mode.
     */
    Init_msg( mp, rp->rspid, up->unit );
    mp->mscp_opcode = MSCP_OP_ONLIN;
    if ( up->flags.excl_acc )
        mp->mscp_modifier = MSCP_MD_EXCAC;
    new_event = mscp_send_msg( rp );
    return ( new_event );
}

/**/

/*
 *
 *   Name:	mscp_onlineem - process an online end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_onlineem( event, rp )

    u_long			event;
    register REQB		*rp;

{

    register MSCP		*mp = rp->msgptr;
    register UNITB		*up = rp->unitb;
    register u_long		new_event;
    u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
    u_short			em_subcode = mp->mscp_status >> MSCP_ST_SBBIT;

    /* Check to see if the online succeeded.
     */
    if( em_status ==  MSCP_ST_SUCC ) {

	/* Set a unit flag to remember to bypass BBR processing
	 * if the unit was already online.
	 */
	up->flags.alonl = ( em_subcode == MSCP_SC_ALONL );

	/* If the unit is formatted for the DEC10/20, it is not
	 * usable.  Redispatch to avail the unit.
	 * TODO - Check D0 and D1 of media ID against unitb.
	 */
	if( mp->mscp_unt_flgs & MSCP_UF_576 ) {
	    mscp_recycle_rspid( rp );
	    new_event = EV_ONLERRAVAIL;

	/* Store the contents of the online end message, recycle the
	 * RSPID, then format and send a get unit status message.
	 */
	} else {
	    up->unit_id = *( UNIQ_ID * )mp->mscp_unit_id;
	    up->media_id = mp->mscp_media_id;
	    up->unt_size = mp->mscp_unt_size;
	    up->vol_ser = mp->mscp_vol_ser;
	    up->unt_flgs = mp->mscp_unt_flgs;

	    mscp_recycle_rspid( rp );
	    Init_msg( mp, rp->rspid, up->unit );
	    mp->mscp_opcode = MSCP_OP_GTUNT;
	    new_event = mscp_send_msg( rp );
	}
		
    /* The online command did not succeed.  If the error was caused by
     * a duplicate unit number, print a message on the console.  For all
     * errors, reset online-in-progress and redispatch with an online
     * complete event.
     */
    } else { 
	if( em_status == MSCP_ST_OFFLN) {
	    switch (em_subcode) {
		case MSCP_SC_DUPUN:
		    printf( "mscp - Duplicate unit %d detected", up->unit );
		    printf( " on controller xxx\n");
		    break;
		case MSCP_SC_EXUSE:
		    printf( "mscp - Unit %d is exclusive access", up->unit );
                    printf( "to another host.\n" );
		    break;
	    }
	} else if(( em_status == MSCP_ST_AVLBL ) &&
		  ( em_subcode == MSCP_SC_ALUSE )) {
	    printf( "mscp - Unit %d is online to another host.\n",up->unit );
	}
	new_event = EV_ONLERROR;
    }

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_onlgtuntem - process GTUNT end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long 
mscp_onlgtuntem( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register MSCP		*mp = rp->msgptr;
    register UNITB		*up = rp->unitb;
    register u_long		new_event = EV_ONLCOMPLETE;
    u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
    u_short			em_subcode = mp->mscp_status >> MSCP_ST_SBBIT;
	
    /* If the Get Unit Status command succeeded, store the unit
     * status in the unit block.
     */
    if( em_status == MSCP_ST_SUCC ) {
	Pad_msg( mp, rp->msgsize );
	up->mult_unt = mp->mscp_mult_unt;
	up->unt_flgs = mp->mscp_unt_flgs;
	up->unit_id = *( UNIQ_ID * )mp->mscp_unit_id;
	up->media_id = mp->mscp_media_id;
	up->shdw_unt = mp->mscp_shdw_unt;
	up->shdw_sts = mp->mscp_shdw_sts;
	up->track = mp->mscp_track;
	up->group = mp->mscp_group;
	up->cylinder = mp->mscp_cylinder;
	up->unit_svr = mp->mscp_unit_svr;
	up->unit_hvr = mp->mscp_unit_hvr;
	up->rct_size = mp->mscp_rct_size;
	up->rbns = mp->mscp_rbns;
	up->rct_cpys = mp->mscp_rct_cpys;
	up->flags.wrtp = (( mp->mscp_unt_flgs &
			 ( MSCP_UF_WRTPH |
			   MSCP_UF_WRTPS |
			   MSCP_UF_WRTPD )) != 0 );
	up->flags.rct_pres = ( mp->mscp_rct_cpys != 0 );
	up->tot_size = up->unt_size + ( up->rct_size * up->rct_cpys );

	/* If BBR is done by the host, the unit is write enabled, and the
	 * unit was not already online when we issued the most recent ONLINE
	 * command, call the BBR code to do ONLINE-time processing.
	 * Otherwise, redispatch with the (preset) online complete event.
	 */
	if( !( up->unt_flgs & MSCP_UF_REPLC ||
	       up->flags.wrtp ||
	       up->flags.alonl ))
	    new_event = mscp_bbr_online( rp );

    /* The Get Unit Status failed.  Recycle the RSPID and redispatch
     * to retry the ONLINE command.
     */
    } else {
	mscp_recycle_rspid( rp );
	new_event = EV_ONLERROR;
    }

    return( new_event );

}    
/**/

/*
 *
 *   Name:	mscp_markonline - Mark unit online.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long 
mscp_markonline( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register UNITB		*up = rp->unitb;
    register u_long		new_event = EV_NULL;
	
    up->flags.online = 1;
    up->flags.online_ip = 0;

    mscp_dealloc_reqb( rp );
    return( new_event );
}

/**/

/*
 *
 *   Name:	mscp_availcm - Send an AVAIL command message
 *
 *   Abstract:	
 *
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_availcm( event, rp )

    u_long			event;
    register REQB		*rp;

{

    register UNITB              *up = rp->unitb;
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event;

    /* Format the message buffer as a AVAILABLE command.  Queue the message
     * for transmission, then wait for the AVAILABLE end message.
     * If the unit is set to exclusive access mode then set the exclusive
     * modifier to hold onto the exclusive access operation.  This implies that
     * the only way to clear exclusive access is to issue an ioctl to clear
     * the excl_acc flag and then do an avail.  The other way exclusive access
     * is cleared is when the unit becomes inoperative, disabled or unknown.
     */
    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = MSCP_OP_AVAIL;
    if ( up->flags.excl_acc ) {
        mp->mscp_modifier = MSCP_MD_EXCAC;
    }
    new_event = mscp_send_msg( rp );
    return( new_event );
}
	
/**/

/*
 *
 *   Name:	mscp_markoffline - Mark a unit offline
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_markoffline( event, rp )

    u_long			event;
    register REQB		*rp;

{

    register UNITB		*up = rp->unitb;
    register CONNB		*cp = rp->connb;
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event = EV_NULL;
    u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
    u_short			em_subcode = mp->mscp_status >> MSCP_ST_SBBIT;

    /* Reset the online-in-progress bit, deallocate the request block
     * and exit with an EV_NULL event.
     */
    up->flags.online_ip = 0;
    up->flags.close_ip = 0;
    up->flags.online = 0;

    /* If unit recovery is in progress: this unit did not come back 
     * online so document the failure (printf) and return EV_ONLERROR
     * to continue the unit recovery process.
     */
    if(( cp->flags.restart) && (rp->flags.perm_reqb)) {
      printf("\nDisk unit #%d failed unit recovery.\n",up->unit);
        return (EV_ONLERROR);
    }

    mscp_dealloc_reqb( rp );
    return( new_event );

}

/**/

/*
 *
 *   Name:	mscp_transfercm - Send a READ/WRITE command message
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_transfercm( event, rp )

    u_long			event;
    register REQB		*rp;

{

    register MSCP		*mp = rp->msgptr;
    register struct buf		*bp = rp->bufptr;
    register UNITB		*up = rp->unitb;
    register struct uba_device	*ui = up->ubdev;

    /* set up stat data for iostat utilities.
     */
    if(ui->ui_dk >= 0)
    {
	dk_xfer[ui->ui_dk]++;
	dk_wds[ui->ui_dk] += bp->b_bcount>>6;
    }

    /* Format the message buffer as a READ/WRITE command and fill in
     * the transfer byte count, local buffer handle, and logical
     * block number.  Update the state and send the message, then
     * wait for the end message to come back.
     */
    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = ( bp->b_flags & B_READ ) ?
			MSCP_OP_READ : MSCP_OP_WRITE;
    mp->mscp_byte_cnt = bp->b_bcount;
    Move_bhandle( rp->lbhandle, mp->mscp_buffer[ 0 ] ); 
    mp->mscp_lbn = rp->p1;
    return( mscp_send_msg( rp ));
	
}

/**/

/*
 *
 *   Name:	mscp_transferem - process a data transfer end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_transferem( event, rp )
    u_long			event;
    register REQB		*rp;

{
    register MSCP		*mp = rp->msgptr;
    register struct buf		*bp = rp->bufptr;
    register u_long		new_event = EV_NULL;
    UNITB			*up = rp->unitb;
    struct uba_device		*ui = up->ubdev;
    u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
    u_short			em_subcode = mp->mscp_status >> MSCP_ST_SBBIT;

    if( mp->mscp_flags & MSCP_EF_BBLKR ) {
	new_event = mscp_bbr_replace( rp );
    } else {

        /* If an error of any kind occurred during the data transfer,
         * mark the buf structure accordingly.
         */

        if( em_status != MSCP_ST_SUCC ) {
	    bp->b_flags |= B_ERROR;
	    bp->b_error = EIO;

	    /* temporary log force error here - pmk */

	    if (em_status == MSCP_ST_DATA && em_subcode == MSCP_SC_FRCER) {
		printf("ra%d%c: hard error sn %d\n",
		       Ux( bp->b_dev ),
		       'a' + ( u_char )Px( bp->b_dev ),
		       bp->b_blkno + (mp->mscp_byte_cnt / 512));
		printf("ra%d%c: Force Error Modifer Set: LBN %d\n",
		       Ux( bp->b_dev ),
		       'a' + ( u_char )Px( bp->b_dev ),
		       rp->p1 + (mp->mscp_byte_cnt / 512));
	    }
        }

        /* Save the bytes not transferred in the request block. (It can't
         * be saved in b_resid, because mapping information is stuffed in
         * there by the port driver.  I/O done is likewise deferred until
         * the buffer has been unmapped.) 
         * Deallocate the request block with all the resources that it 
         * holds and terminate the thread.
         */

        rp->p1 = bp->b_bcount - mp->mscp_byte_cnt;

	mscp_dealloc_reqb( rp );
    }

    return( new_event );
}

/**/

/*
 *
 *   Name:	mscp_accscancm - access command message
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_accscancm( event, rp )

    u_long			event;
    register REQB		*rp;

{

    register MSCP		*mp = rp->msgptr;
    register UNITB		*up = rp->unitb;


    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = MSCP_OP_ACCESS;
    mp->mscp_byte_cnt = rp->p2;
    mp->mscp_lbn = rp->p1;
    return( mscp_send_msg( rp ));
	
}

/**/

/*
 *
 *   Name:	mscp_accscanem - process an access end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_accscanem( event, rp )
    u_long			event;
    register REQB		*rp;

{
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event = EV_NULL;
    UNITB			*up = rp->unitb;


    up->acc_status = mp->mscp_status;
    up->acc_flags = mp->mscp_flags;
    up->acc_badlbn = mp->mscp_lbn;
    up->acc_bytecnt = mp->mscp_byte_cnt;

    if( mp->mscp_flags & MSCP_EF_BBLKR ) {
	new_event = mscp_bbr_replace( rp );
    }
    else {
        up->flags.force_scan_ip = 0;
	mscp_dealloc_reqb( rp );
    }

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_setunitcm - set unit command message
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_setunitcm( event, rp )

    u_long			event;
    register REQB		*rp;

{

    register MSCP		*mp = rp->msgptr;
    register UNITB		*up = rp->unitb;


    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = MSCP_OP_STUNT;
    if ( up->flags.excl_acc )
        mp->mscp_modifier = MSCP_MD_EXCAC;
    return( mscp_send_msg( rp ) );
	
}

/**/

/*
 *
 *   Name:	mscp_setunitem - process a set unit end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_setunitem( event, rp )
    u_long			event;
    register REQB		*rp;

{
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event = EV_NULL;
    UNITB			*up = rp->unitb;
    u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
    u_short			em_subcode = mp->mscp_status >> MSCP_ST_SBBIT;

    /*
     * Check to see that the command succeeded.  A status of available with
     * a subcode of available implies success subcode normal when exclusive
     * access is granted while the unit was in the "Unit-Available" state.
     */
    if (( em_status == MSCP_ST_SUCC ) ||
	(( em_status == MSCP_ST_AVLBL ) && (em_subcode == MSCP_SC_AVAIL))) {
	/* Make sure the set of exclusive access succeeded.  If it does fail
	 * then clear the excl_acc flag so that the ioctl routine will know to
	 * return a failure status.
	 */
	if ( up->flags.excl_acc ) {
	    if (( mp->mscp_unt_flgs & MSCP_UF_EXACC ) == 0 ) {
	        up->flags.excl_acc = 0;
	    }
	}
	else {
	    if ( mp->mscp_unt_flgs & MSCP_UF_EXACC ) {
	        up->flags.excl_acc = 1;
	    }
	}
    }
    else {
	if ( up->flags.excl_acc ) {
	    up->flags.excl_acc = 0;
	}
	else {
	    up->flags.excl_acc = 1;
	}
    }
    up->flags.mscp_wait = 0;
    return( new_event );
}


/**/

/*
 *
 *   Name:	mscp_recovinit - Initiate restoration of unit states.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_recovinit( event, rp )

    u_long			event;
    register REQB		*rp;

{
    register CONNB		*cp = rp->connb;
    register UNITB		*up;
    register u_long		new_event = EV_NULL;

    /* Find the first unit that was online when the connection dropped.
     */
    for( up = cp->unit.flink;
	 up != ( UNITB * )&cp->unit.flink && !up->flags.online;
	 up = up->flink )
	{}
    
    /* If we found a previously online unit, mark it offline with
     * online in progress, store the unit block pointer in the request
     * block and allocate a RSPID to use in the online sequence.
     */
    if( up != ( UNITB * )&cp->unit.flink ) {
	up->flags.online = 0;
	up->flags.online_ip = 1;
	rp->unitb = up;
	new_event = mscp_alloc_rspid( event, rp );

    /* No unit was online when the connection dropped; however, there
     * may have been an online request enqueued, so attempt to restart
     * the next request in the restart queue.
     */
    } else {
	mscp_restart_next( cp );
	mscp_dealloc_all( rp );
    }

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_recovnext - Mark current unit online and process next
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long 
mscp_recovnext( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register UNITB		*up = rp->unitb;
    register CONNB		*cp = rp->connb;
    register u_long		new_event = EV_NULL;
	
    /* If this routine was entered as a result of successful completion
     * of the BBR algorithm or of the GTUNT command, set the unit online
     */
    if( event == EV_ERRECOV || event == EV_ONLCOMPLETE ) {
	up->flags.online = 1;
	up->flags.online_ip = 0;
    }

    /* Find the next unit that was online when the connection dropped.
     */
    for( up = up->flink;
	 up != ( UNITB * )&cp->unit.flink && !up->flags.online;
	 up = up->flink )
	{}
    
    /* If we found another previously online unit, mark it offline with
     * online in progress, store the unit block pointer in the request
     * block and redispatch to bring the unit online.
     */
    if( up != ( UNITB * )&cp->unit.flink ) {
	up->flags.online = 0;
	up->flags.online_ip = 1;
	rp->unitb = up;
	new_event = EV_ONLDONEXT;

    /* No more units were online.  Attempt to start the first request on
     * the restart queue, deallocate the resources held by the request
     * block and terminate the thread.
     */
    } else {
	mscp_restart_next( cp );
	mscp_dealloc_all( rp );
    }
    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_strategy - rtn description
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

int 
mscp_strategy( bp )

    register struct buf		*bp;

{

    register UNITB		*up;
    register int		px;
    register int		psize;
    register daddr_t		pstart;
    register int		errno = 0;

    /* Check to make sure that the unit corresponding to the device number
     * exists and is online.  If not, return an error to the user.
     */
    up = Dev_to_unitb( bp->b_dev );
    if( up == NULL || !up->flags.online ) 
	errno = ENXIO;

    /* If the partition table for the unit is not valid, panic.
     */
    else if( up->part_info.pt_valid != PT_VALID )
	panic( "mscp_strategy: invalid partition table\n" );
	
    /* Get the partition offset and size from the current partition
     * table in the unit block.  If the partition size is specified
     * as -1, calculate the partition size as the number of blocks
     * from the start of the partition to the end of the user area.
     */
    else {
	px = Px( bp->b_dev );
	pstart = up->part_info.pt_part[ px ].pi_blkoff;
	if(( psize = up->part_info.pt_part[ px ].pi_nblocks ) == -1 )
	    psize = up->unt_size - pstart;

	/* Ensure that the transfer lies entirely within the bounds of 
	 * the partition.  If so, allocate a request block and start the
	 * data transfer.  Otherwise return an error to the user.  (Note
	 * that the process sleeps until the request block is allocated.)
	 */
	if( pstart >= 0 &&
	    bp->b_blkno + (( bp->b_bcount + 511 ) >> 9 ) <= psize )
	    ( void )mscp_alloc_reqb( up,
				     bp,
				     mscp_xfr_states,
				     pstart + bp->b_blkno,
				     0 );
	else 
	    errno = ENOSPC;
    }

    /* If an error has been detected, set the error indicator and error
     * number in the buf structure and terminate the I/O operation.
     */ 
    if( errno ) {
	bp->b_error = errno;
	bp->b_flags |= B_ERROR;
	( void )iodone( bp );
    }

    return;
}
/**/

/*
 *
 *   Name:	mscp_bopen - Block mode open routine
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

int 
mscp_bopen( dev, flag )

    dev_t			dev;
    int				flag;

{
    return( mscp_open( dev, flag, 0 ));
}

/**/

/*
 *
 *   Name:	mscp_copen - Raw (character) mode open routine
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

int 
mscp_copen( dev, flag )

    dev_t			dev;
    int				flag;

{
    return( mscp_open( dev, flag, 1 ));
}

/**/

/*
 *
 *   Name:	mscp_open	- Open Disk Unit
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

int
mscp_open(dev, flag, raw)
    dev_t			dev;
    int				flag;
    int				raw;
{
    register UNITB		*up;
    REQB			*rp;
    int				errno = 0;
    int				s;

    /* If there is no unit block corresponding to the device number,
     * wait for the system devices to be configured.  If the unit
     * still cannot be found, return an error.
     */
    if(( up = Dev_to_unitb( dev )) == NULL  &&
       ( up = mscp_check_sysdev( dev )) == NULL ) {
	    errno = ENXIO;

    /* If the unit is not online, start a thread to bring it online
     * and wait for the thread to complete.
     */
    } else {
	while( 1 ) {
    	    s = Splscs();
	    if( !up->flags.busy ) {
		up->flags.busy = 1;
    		( void )splx( s );
		break;
	    }
	    else {
    		( void )splx( s );
		( void )sleep(( caddr_t )up, PSWP+1 );
	    }
	}

	if( !up->flags.online ) {
	    if( !up->flags.online_ip ) {
		up->flags.online_ip = 1;
		rp = ( REQB * )mscp_alloc_reqb( up,
						NULL,
						mscp_onl_states,
						0,
						0 );
	    }

	    while( up->flags.online_ip ) {
		timeout( wakeup, ( caddr_t )up, 3 * hz );
		( void )sleep(( caddr_t )up, PSWP+1 );
		untimeout( wakeup, ( caddr_t )up );
	    }
	}

	/* If the device is now online, bump the open count and update
	 * the partition information if necessary.
	 */
	if( up->flags.online ) {
	    up->part_mask |= ( 1 << (( raw ? 8 : 0 ) + Px( dev )));
	    if( !up->part_info.pt_valid ) {
		mscp_getdefpt( up->media_id, ( struct pt * )&up->part_info );
		up->part_info.pt_valid = 1;
		( void )rsblk( mscp_strategy,
			       dev,
			       ( struct pt * )&up->part_info );
	    }

	/* If the device could not be brought online and the NDELAY 
	 * flag is not set, return an error.
	 */
	}
	else if(( flag & O_NDELAY ) == 0 ) {
	    errno = ENXIO;
	}

        up->flags.busy = 0;
	wakeup(( caddr_t )up );
    }
    
    /* Return status to the caller.
     */
    return( errno );
}

/**/

/*
 *
 *   Name:	mscp_bclose - Block mode close routine
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

int 
mscp_bclose( dev, flag )

    dev_t			dev;
    int				flag;

{
    return( mscp_close( dev, flag, 0 ));
}

/**/

/*
 *
 *   Name:	mscp_cclose - Raw (character) mode close routine
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

int 
mscp_cclose( dev, flag )

    dev_t			dev;
    int				flag;

{
    return( mscp_close( dev, flag, 1 ));
}

/**/

/*
 *
 *   Name:	mscp_close	- Close Disk Unit
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

int 
mscp_close(dev, flag, raw )

    dev_t			dev;
    int				flag;
    int				raw;

{

    UNITB			*up;
    REQB			*rp;
    int				s;
    struct uba_device		*ui;

    /* If there is no known unit corresponding to the device
     * number, return an error.
     */
    if(( up = Dev_to_unitb( dev )) == NULL ) {
	return( ENXIO );
    }

    while( 1 ) {
        s = Splscs();
	if( !up->flags.busy ) {
	    up->flags.busy = 1;
    	    ( void )splx( s );
	    break;
	}
	else {
    	    ( void )splx( s );
	    ( void )sleep(( caddr_t )up, PSWP+1 );
	}
    }

    /* Overwrite default throughput constant so it reflects the
     * correct number of sectors per track
     */
    if(up->track){
	ui = up->ubdev;
        if ((ui->ui_dk >= 0) && (up->track != 0))
        {   /* assume 60 revs */
#ifdef vax
	    dk_mspw[ui->ui_dk] = 1.0 / ( 60 * up->track * 256); 
#else
	    dk_mspw[ui->ui_dk] = ( 60 * up->track * 256); 
#endif
	}
    }

    /* Reduce the open outstanding counter.  If no
     * active partitions remain, and the unit is online, set the unit
     * available.
     */
    up->part_mask &= ~( 1 << (( raw ? 8 : 0 ) + Px( dev )));
    if( up->part_mask == 0 && up->flags.online ) {
	up->flags.close_ip = 1;
	rp = ( REQB * )mscp_alloc_reqb( up,
					NULL,
					mscp_avl_states,
					0,
					0 );
	while( up->flags.close_ip ) {
    	    timeout( wakeup, ( caddr_t )rp, 3 * hz );
    	    ( void )sleep(( caddr_t )rp, PSWP+1 );
 	    untimeout( wakeup, ( caddr_t )rp );
	}

	up->part_info.pt_valid = 0;
    }

    up->flags.busy = 0;
    wakeup(( caddr_t )up );

    return( 0 );
}

/**/

/*
 *
 *   Name:	mscp_read - 
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

int 
mscp_read( dev, uio )

    dev_t		dev;
    struct uio		*uio;

{

    register UNITB	*up;
    int			status;

    /* If there is no known unit corresponding to the device
     * number, return an error.
     */
    if(( up = Dev_to_unitb( dev )) == NULL ) {
	return( ENXIO );
    }

    /* Invoke physio to fire off the strategy routine and return
     * the resulting status.
     */
    status = physio( mscp_strategy, &up->rawbuf, dev, B_READ, minphys, uio );
    return( status );
}
/**/

/*
 *
 *   Name:	mscp_write - 
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

int 
mscp_write( dev, uio )

    dev_t	dev;
    struct	uio *uio;

{

    register UNITB	*up;
    int			status;

    /* If there is no known unit corresponding to the device
     * number, return an error.
     */
    if(( up = Dev_to_unitb( dev )) == NULL ) {
	return( ENXIO );
    }

    /* Invoke physio to fire off the strategy routine and return
     * the resulting status.
     */
    status = physio( mscp_strategy, &up->rawbuf, dev, B_WRITE, minphys, uio );
    return( status );
}
/**/

/*
 *
 *   Name:	mscp_ioctl	- Process I/O Control Functions
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

/* TODO - check for possible races between get/set partition tables
 * and a unit becoming available or online.
 */

int
mscp_ioctl(dev, cmd, data, flag)

    dev_t		dev;
    int			cmd;
    caddr_t		data;
    int			flag;

{

    register UNITB	*up;

    /* If there is no known unit corresponding to the device
     * number, return an error.
     */
    if(( up = Dev_to_unitb( dev )) == NULL ) {
	return( ENXIO );
    }

    switch( cmd ) {

    /* Get the current or default partition table values for the device
     * 'dev' and store them in the buffer pointed to by 'data'.
     */
    case DIOCGETPT:
    case DIOCDGTPT:

	{
	    register struct pt	*ptp = ( struct pt * )data;
	    register int	i;

	    /* If the current partition table values are wanted, use a structure
	     * copy to get them into the user buffer.
	     */
	    if( cmd == DIOCGETPT ) {
		*ptp = up->part_info;
	    }

	    /* Get the default partition table values based on the storage
	     * medium currently loaded in the device.
	     */
	    else {
		mscp_getdefpt( up->media_id, ptp );
	    }

	    /* Go through the user buffer, changing any partition sizes that
	     * were specified as -1 to the actual partition size, calculated as
	     * the size of the user-accessible portion of the disk minus the
	     * starting LBN of the partition.
	     */
	    for( i = 0; i <= 7; i++ )
		if( ptp->pt_part[ i ].pi_nblocks == -1 )
		    ptp->pt_part[ i ].pi_nblocks = 
			up->unt_size - ptp->pt_part[ i ].pi_blkoff;
	    return( 0 );
	}
	    
    /* Replace the current partition table for the device with the contents
     * of the user supplied buffer (after appropriate checking).
     */
    case DIOCSETPT:

	{
	    register struct pt	*ptp = ( struct pt * )data;
	    int			error;

	    /* If the caller is not the super-user, return a permission
	     * denied (EACCES) error.
	     */
	    if( !suser() )
		return( EACCES );

	    /* If the user-supplied partition values conflict with the
	     * active partition table, return an error.
	     */
	    if( ( error = ptcmp( dev, &up->part_info, ptp )) != 0 )
		return( error );
	    
	    /* Set the partition tables in the UNITB using the data in the
	     * user's buffer, update the superblock in the 'a' partition
	     * if necessary, set the partition table valid bit, and return
	     * success to the caller.
	     */
	    up->part_info = *ptp;
	    ( void )ssblk( dev, ptp );
	    up->part_info.pt_valid = PT_VALID;
	    return( 0 );
	}    

    case DEVIOCGET:
	{
	    register struct devget	*dp = ( struct devget * )data;
	    register struct uba_device	*ui = up->ubdev;
	    register CONNB		*cp = up->connb;

	    bzero( dp, sizeof( struct devget ));
	    dp->category = DEV_DISK;
	    dp->bus = cp->bus_type;
	    bcopy( cp->model_name, dp->interface, strlen( cp->model_name ));
	    mscp_media_to_ascii( up->media_id, dp->device );
	    dp->adpt_num = ui->ui_adpt;
	    dp->nexus_num = ui->ui_nexus;
	    dp->bus_num = ui->ui_ubanum;
	    dp->rctlr_num = ui->ui_rctlr;
	    dp->ctlr_num = ui->ui_ctlr;
	    dp->slave_num = up->unit;
	    bcopy( ui->ui_devname, dp->dev_name, strlen( ui->ui_devname ));
	    dp->unit_num = ui->ui_unit;
	    dp->soft_count = 0;
	    dp->hard_count = 0;
	    dp->stat = 0;
	    if( up->flags.online == 0 )
		dp->stat |= DEV_OFFLINE;
	    if( up->flags.wrtp == 1 )
		dp->stat |= DEV_WRTLCK;
	    dp->category_stat = DEV_DISKPART;
	    return( 0 );
	}

    /* Ioctl to obtain device geometry information.
     */
    case DEVGETGEOM:
	{
	    DEVGEOMST	*devgeom = ( DEVGEOMST * )data;
	    register int calcs, ncyl;

	    bzero( devgeom, sizeof( DEVGEOMST ));
	    calcs = up->group * up->cylinder;
	    devgeom->geom_info.dev_size = up->unt_size;
	    devgeom->geom_info.ntracks = calcs;
	    devgeom->geom_info.nsectors = up->track;
	    calcs *= up->track;
	    if (calcs > 0) {
	    	ncyl = up->unt_size / calcs;
	    	if (up->unt_size % calcs)
			ncyl++;   /* round up */
	    }
	    else {
		ncyl = 0;
	    }
	    devgeom->geom_info.ncylinders = ncyl;
	    if (up->unt_flgs & MSCP_UF_RMVBL)
		devgeom->geom_info.attributes |= DEVGEOM_REMOVE;
	    return( 0 );
	}

    /* Ioctl used by radisk(8) to scan the disk or force a replacement
     */
  case DKIOCACC:
	{
	struct dkacc *dkacc = (struct dkacc *)data;
        register REQB	*rp;
        long 	totbytes;
        long	length;
        long	lbn;

	/* Only super users can beat on the pack
	 */
	if ( !suser() )
		return(EACCES);

	switch (dkacc->dk_opcode){
	case ACC_REVEC:
	    /* Return if the controller does the work
	     */
            if(( up->unt_flgs & MSCP_UF_REPLC ||
	           up->flags.wrtp ))
		{
			return(0);
		}
	    if (up->flags.force_scan_ip == 1)
	            return(EBUSY);

	    up->flags.force_scan_ip = 1;

	    /* Force LBN  to be re-vectored
	     */
	    rp = (REQB *)mscp_alloc_reqb( up,
			     		  NULL,
			     		  mscp_repl_states,
			     		  dkacc->dk_lbn,
			     		  0 );

	    while( up->flags.force_scan_ip ) {
    		timeout( wakeup, ( caddr_t )rp, 5 * hz );
    		( void )sleep(( caddr_t )rp, PSWP+1 );
 		untimeout( wakeup, ( caddr_t )rp );
	    }

	    dkacc->dk_status = 0;	
	    break;

	case ACC_SCAN:

	    if (dkacc->dk_lbn > up->unt_size)
		    return(EINVAL);

	    lbn = dkacc->dk_lbn;
	    totbytes = 0;

	    if (up->connb->max_bcnt == 0)
	        up->connb->max_bcnt = 16777216;

	    if (up->flags.force_scan_ip == 1)
	            return(EBUSY);

	    while (dkacc->dk_length ) {

	        up->flags.force_scan_ip = 1;

		length = (dkacc->dk_length > up->connb->max_bcnt)
	         	? up->connb->max_bcnt : dkacc->dk_length;

	        rp = (REQB *)mscp_alloc_reqb( up, NULL, 
				mscp_accscan_states, lbn, length);

	        while( up->flags.force_scan_ip ) {
    		    timeout( wakeup, ( caddr_t )rp, 5 * hz );
    		    ( void )sleep(( caddr_t )rp, PSWP+1 );
    		    untimeout( wakeup, ( caddr_t )rp );
	        }

		totbytes += up->acc_bytecnt;

		if (up->acc_status != MSCP_ST_SUCC) 
		    break;
		
		lbn += btodb(up->acc_bytecnt);
		dkacc->dk_length -= up->acc_bytecnt;
	    }

    	    dkacc->dk_status = up->acc_status;
    	    dkacc->dk_flags = up->acc_flags;
    	    dkacc->dk_lbn = up->acc_badlbn;
    	    dkacc->dk_length  = totbytes;

	    break;

	default:
	    break;	
  	}
	return( 0 );
	}
    /* Ioctl used by radisk(8) to control the exclusive access attribute.
     */
  case DKIOCEXCL:
	{
	    int *action = (int *)data;
            register REQB	*rp;

	    /* Only super users can modify this attribute.
	     */
	    if ( !suser() ) {
		return(EACCES);
	    }

	    /* Set the up->flags.excl_acc flag to be the specified mode of
	     * exclusive access.  Next call the set unit characteristics state
	     * table.  When that completes if the value of excl_acc changes it
	     * means that the operation failed.
	     */

	    if ( *data == 0 ) {	/* clear */
		if ( up->flags.excl_acc == 0 ) {
		    return(EIO);
		}
		up->flags.excl_acc = 0;
	    }
	    else {		/* set */
		up->flags.excl_acc = 1;
	    }
	    /* Force a set unit characteristics to set exclusive mode.
	     */
	    if ( up->flags.mscp_wait == 1 )
	        return(EBUSY);
	    up->flags.mscp_wait = 1;
	    rp = (REQB *)mscp_alloc_reqb( up, NULL, mscp_stu_states, 0, 0);
	    while( up->flags.mscp_wait ) {
    	 	timeout( wakeup, ( caddr_t )rp, 5 * hz );
    	 	( void )sleep(( caddr_t )rp, PSWP+1 );
 	 	untimeout( wakeup, ( caddr_t )rp );
	    }
	    if ( *data == 0 ) {			/* clear failed */
		if ( up->flags.excl_acc ) {
			return(EIO);
		}
	    }
	    else {
		if ( up->flags.excl_acc == 0 ) {
			return(EIO);
		}
	    }
	    return( 0 );
	}

    default:
	return( 0 );

    }
}
/**/

/*
 *
 *   Name:	mscp_forcecm - Start a forced replace
 *
 *   Abstract:	This function calls the bbr_force() routine
 *		to force a replacement.  This function is entered
 *		as a result of an radisk(8) request.
 *
 *
 *   Inputs:	event			Event code.
 *		rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_forcecm( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register MSCP		*mp = rp->msgptr;
	

	mp->mscp_lbn = rp->p1;
	mp->mscp_status = 0;
	return( mscp_bbr_force( rp ) );
}

/**/

/*
 *
 *   Name:	mscp_forceem - Cleanup afer a forced replace
 *
 *   Abstract:	This function deallocates the resources allocated for
 *		the replacement and clears the force_scan_ip bit to indicate
 *		the replacement is complete.  The ioctl() function is
 *		spinning on this bit. The access scan function is also
 *		spinning on this bit after a bad block replace.
 *
 *
 *   Inputs:	event			Event code.
 *		rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
mscp_forceem( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register UNITB		*up = rp->unitb;

    up->flags.force_scan_ip = 0;
    mscp_dealloc_reqb( rp );
    return( EV_NULL);	
}

/**/

/*
 *
 *   Name:	mscp_size - Find the size of a partition.
 *
 *   Abstract:	This routine returns the size of the partition specified
 *		by a given device number.
 *
 *   Inputs:	dev			Device number.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	Size of partition specified by dev.
 *   Values:
 */

int 
mscp_size( dev )

    dev_t		dev;

{

    register int	part_size;
    register UNITB	*up;
    register int	px = Px( dev );

    /* If the unit index is greater than the assembled-in maximum,
     * or if there is no unit in the configuration corresponding to
     * the unit index, return -1.
     */
    up = Dev_to_unitb( dev );
    if( up == NULL || !up->flags.online ) 
	return( -1 );

    /* As a sanity check, panic if the partition table information
     * is not marked valid.
     */
    if( up->part_info.pt_valid != PT_VALID )
	panic( "mscp_size: invalid partition table\n" );

    /* If the actual size of the partition is specified (not -1), return it
     * to the caller.  Otherwise, calculate the partition size as the size of
     * the user-accessible area of the disk minus the starting LBN of the
     * partition, and return it to the caller.
     */
    if(( part_size = up->part_info.pt_part[ px ].pi_nblocks ) != -1 )
	return( part_size );

    else
	return( up->unt_size - up->part_info.pt_part[ px ].pi_blkoff );
}
/**/

/*
 *
 *   Name:	mscp_getdefpt - get default partition information.
 *
 *   Abstract:	Find the partition information corresponding to the
 *		input media ID and store it in a user-provided
 *		partition structure.
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
mscp_getdefpt( media_id, ptp )

    u_long		media_id;
    struct pt		*ptp;

{
    register int	i;
    register PART_SIZE	*psp;

    /* Look up the media ID in the disk media table.  If it isn't found, use
     * reserved entry 0 as the default media type.
     */
    for( i = dmscp_media_ct - 1;
	 i > 0 && dmscp_media[ i ].media_id != media_id;
	 i-- )
	;

    /* Move the default partition values for the device (medium) into
     * the user's partition structure.
     */
    psp = dmscp_media[ i ].part_sizes;
    for( i = 0; i <= 7; i++, psp++ ) {
	ptp->pt_part[ i ].pi_nblocks = psp->p_nblocks;
	ptp->pt_part[ i ].pi_blkoff = psp->p_blkoff;
    }
    return;
}

/**/

/*
 *
 *   Name:	mscp_check_sysdev - check/wait for system device availability.
 *
 *   Abstract:	A requested device has been found to be unconfigured.
 *		Check to see if the device is the root, swap or dump
 *		device specified in the configuration file and if so,
 *		wait for the device to become available.  If the device
 *		is none of the foregoing, return an ENXIO error.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

UNITB *
mscp_check_sysdev( dev )
    dev_t			dev;
{
    register UNITB		*up = NULL;
    register struct swdevt	*sp;
    register int		retries = 120;
    dev_t			dev_min,dev_maj;

    /* If the given device is one of the swap devices or is
     * the root or dump device, wait for it to be configured.
     */
    for( sp = &swdevt[ 0 ];
 	 ( dev_t )sp->sw_dev != 0 &&
	 ( dev_t )dev != ( dev_t )sp->sw_dev;
	 sp++ )
	 ;
    if( ( dev_t )dev == ( dev_t )sp->sw_dev ||
	( dev_t )dev == ( dev_t )rootdev || 
        ( dev_t )dev == ( dev_t )dumpdev )
	while(( up = Dev_to_unitb( dev )) == NULL && ( --retries >= 0 )) {

	    /* if we're half way through the wait give some status
	     */

	    if( retries == 60 ) {
	        dev_maj = major(dev);
	        dev_min = minor(dev);
                if( ( dev_t )dev == ( dev_t )sp->sw_dev )
                    printf("Waiting up to 1 more minute for swap device \(%d,%d\) to become available\n", dev_maj,dev_min);
	        else if( ( dev_t )dev == ( dev_t )rootdev )
                    printf("Waiting up to 1 more minute for root device \(%d,%d\) to become available\n", dev_maj,dev_min);
	        else if( ( dev_t )dev == ( dev_t )dumpdev )
                    printf("Waiting up to 1 more minute for dump device \(%d,%d\) to become available\n", dev_maj,dev_min);
            }

	    timeout( wakeup, ( caddr_t )dev, 1*hz );
	    ( void )sleep(( caddr_t )dev, PSWP+1 );
	}
    return( up );
}
