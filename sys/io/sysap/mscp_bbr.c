#ifndef	lint
static char *sccsid = "@(#)mscp_bbr.c	4.3    (ULTRIX)        12/20/90";
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
 *   Abstract:	This module contains functions which implement host
 *		initiated bad block replacement for the disk variant
 *		of MSCP.
 *
 *   mscp_multi_read		Initiate read of multicopy structure
 *   mscp_multi_read_cont	Continue read of multicopy structure
 *   mscp_multi_write		Initiate write of multicopy structure
 *   mscp_multi_write_cont	Continue write of multicopy structure
 *
 *   Author:	David E. Eiche	Creation Date:	October 15, 1987
 *
 *   History:
 *
 *    4-Dec-1990	Brian Nadeau
 *      Clear out flags in mscp_bbr_step14/16 before we use it.  This
 *      will stop lint from complaining about using flags before we
 *      set it.
 *
 *   28-Aug-1990	Matthew  Sacks
 *	Changed mscp_multi_write_cont so that it will NOT log the no
 * 	multi-copy protection warning if the unit always had just one
 *	copy of the RCT anyway; this is for cases like the ESE20 which
 * 	have exactly one copy (albeit a fake one) of the RCT.
 *
 *   31-Aug-1989	David E. Eiche		DEE0077
 *	Fix synchronization bug in step 1 wherein processing on
 *	a new BBR request was begun before processing on the
 *	previous request had terminated.
 *
 *   21-Jul-1989	David E. Eiche		DEE0070
 *	Change reference from MSLG_FM_DSK_TRN to MSLG_FM_DISK_TRN.
 *
 *   17-Mar-1989	Tim Burke
 *	Changed queue manipulations to use the following macros:
 *	insque ..... Insert_entry
 *	remque ..... Remove_entry
 *	remqck ..... Remove_entry and check to see if any elements on queue.
 *
 *   07-Mar-1989	Todd M. Katz		TMK0001
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   19-Aug-1988	Pete Keilty
 *	Corrected the error event code for multi-copy warning to
 *	MSCP_ST_MFMTE.
 *
 *   27-Jul-1988 	Pete Keilty
 *	Made changes or corrections to the following routines bbr_log
 *	grant, force, step1, step12a, step8 during bbr verification.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   17-Apr-1988        Ricky S. Palmer
 *      Include header file "../vaxmsi/msisysap.h".
 *
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
#include	"../h/smp_lock.h"
#include	"../machine/pte.h"
#include	"../h/vmmac.h"
#include	"../h/config.h"
#include	"../io/scs/sca.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/ci/ciadapter.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/uba/ubavar.h"
#include	"../io/sysap/mscp_msg.h"
#include	"../io/sysap/mscp_defs.h"
#include	"../io/sysap/mscp_bbrdefs.h"

/**/

/* Routine Definitions
 */
void	mscp_bbr_lock(), mscp_bbr_grant();
u_long	mscp_rct_search(), mscp_test_descr(), mscp_multi_read(),
	mscp_multi_write(), mscp_bbr_read(), mscp_bbr_write(),
	mscp_bbr_log();

/* External Variables and Routines.
 */

extern	STATE		mscp_bbr_states[];
extern  void		mscp_logerr();

/* Define debugging stuff.
 */
int 	bbrdebug=0;
#define BBRDEBUG
#ifdef BBRDEBUG
#define Cprintf if(bbrdebug)cprintf
#define Dprintf if( bbrdebug >= 2 )cprintf
#else
#define Cprintf ;
#define Dprintf ;
#endif
/**/

/*
 *
 *   Name:	mscp_bbr_init - Allocate/initialize BBR work area
 *
 *   Abstract:	This function is entered from connection establishment
 *		or recovery to allocate and initialize the the bad block
 *		work area.
 *
 *   Inputs:	IPL_SCS
 *		cp			Connection block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

void
mscp_bbr_init( cp )
    register CONNB		*cp;

{
    register BBRB		*bbrp = cp->bbrb;
    register REQB		*rp;

    /* If the BBR block doesn't exist, allocate it now.
     */
    if( bbrp == NULL )
	KM_ALLOC( bbrp, BBRB *, sizeof( BBRB ), KM_SCA, KM_NOW_CL_CA );

    /* If the allocation succeeded or the BBRB was already in place,
     * initialize it.
     */
    if( bbrp != NULL ) {
	cp->bbrb = bbrp;
	bbrp->bbr_wq.flink = ( REQB * )&bbrp->bbr_wq.flink;
	bbrp->bbr_wq.blink = ( REQB * )&bbrp->bbr_wq.flink;
	bbrp->cur_reqb = NULL;

	rp = ( REQB * )&bbrp->bbr_reqb;
	rp->unitb = NULL;
	rp->connb = cp;
	rp->classb = cp->classb;
	rp->bufptr = ( struct buf * )&bbrp->bbr_buf;
	rp->p1 = 0;
	rp->p2 = 0;
	rp->op_seq_num = 0;
	rp->flags.perm_reqb = 1;
	rp->state_tbl = mscp_bbr_states;
    /*** TEMP Replace with timed retry ***/
    } else {
	panic( "mscp_bbr_init:  couldn't allocate BBRB\n" );
    }

    return;
}
/**/

/*
 *
 *   Name:	mscp_bbr_online - Complete possible partial replacement
 *
 *   Abstract:	This function is the entry point into BBR during online
 *		processing.  It is used to complete any BBR operation
 *		that may have been in progress when the unit went offline.
 *
 *   Inputs:	IPL_SCS
 *		cur_rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_online( cur_rp )

    register REQB			*cur_rp;
{
    Cprintf("mscp_bbr_online: entered\n");
    cur_rp->flags.online = 1;
    mscp_bbr_lock( cur_rp );
    return( EV_NULL );
}
/**/

/*
 *
 *   Name:	mscp_bbr_replace - Perform bad block replacement
 *
 *   Abstract:	This function is the entry point into BBR when a
 *		bad block has been detected during a data transfer
 *		operation.
 *
 *   Inputs:	IPL_SCS
 *		cur_rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_replace( cur_rp )

    register REQB			*cur_rp;
{
    mscp_bbr_lock( cur_rp );
    return( EV_NULL );
}
/**/

/*
 *
 *   Name:	mscp_bbr_force - Force bad block replacement
 *
 *   Abstract:	This function is the entry point into BBR when a
 *		replace operation is requested via the DKIOACC ioctl.
 *		This is generated from the radisk utility.
 *
 *   Inputs:	IPL_SCS
 *		cur_rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_force( cur_rp )

    register REQB			*cur_rp;
{
    register BBRB			*bbrp = cur_rp->connb->bbrb;

    Cprintf("mscp_bbr_force: entered\n");
    if( bbrp == NULL )
	panic( "mscp_bbr_force:  no BBR work area allocated\n" );
    cur_rp->flags.force = 1;
    mscp_bbr_lock( cur_rp );
    return( EV_NULL );
}
/**/

/*
 *
 *   Name:	mscp_bbr_lock - 
 *
 *   Abstract:	This function 
 *
 *   Inputs:	IPL_SCS
 *		cur_rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

void
mscp_bbr_lock( cur_rp )

    register REQB			*cur_rp;

{
    register BBRB			*bbrp = cur_rp->connb->bbrb;

    Cprintf("mscp_bbr_lock: entered\n");
    /* Check to see that the BBR work area block is present.
     */
    if( bbrp == NULL )
	panic( "mscp_bbr_lock:  no BBR work area allocated\n" );

    /* If there are no BBR requests active on the connection, set the
     * BBR lock and start the replacement process.  Otherwise, queue the
     * request block for later processing.
     */
    if( !bbrp->flags.bit.busy ) {
	bbrp->flags.bit.busy = 1;
	mscp_bbr_grant( cur_rp );
    } else {
	Insert_entry( cur_rp->flink, bbrp->bbr_wq );
    }

    /* Stall requests for the unit until the replacement process
     * completes.
     */
    Incr_rwait( cur_rp );
    return;
}
/**/

/*
 *
 *   Name:	mscp_bbr_unlock - 
 *
 *   Abstract:	This function 
 *
 *   Inputs:	IPL_SCS
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

void
mscp_bbr_unlock( rp )

    register REQB			*rp;

{
    register BBRB			*bbrp = rp->connb->bbrb;
    register REQB			*cur_rp = bbrp->cur_reqb;

    /* Dispatch the current request with an error recovery complete
     * event, decrement the stall count and unstall the unit if
     * appropriate.  Clear all BBR flags except for busy.
     * Then process the next replacement request, if any.
     * If no replacement requests remain, reset the BBR busy bit.
     */
    Decr_rwait( cur_rp );
    mscp_dispatch( EV_ERRECOV, cur_rp );
    bbrp->cur_reqb = NULL;
    mscp_dealloc_all( rp );
    bbrp->flags.mask &= BBR_FL_BUSY;
    if(( cur_rp = bbrp->bbr_wq.flink ) != ( REQB * )&bbrp->bbr_wq ) {
	Remove_entry( cur_rp->flink );
	mscp_bbr_grant( cur_rp );
    } else {
	bbrp->flags.bit.busy = 0;
    }
    return;
}
/**/

/*
 *
 *   Name:	mscp_bbr_grant - 
 *
 *   Abstract:	This function 
 *
 *   Inputs:	IPL_SCS
 *		cur_rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

void
mscp_bbr_grant( cur_rp )

    REQB			*cur_rp;
{
    BBRB			*bbrp = cur_rp->connb->bbrb;
    REQB			*rp = &bbrp->bbr_reqb;

    Cprintf("mscp_bbr_grant entered\n");
    /* Store the current request packet address in the BBR block,
     * propogate the unit block address and RSPID into the BBR
     * permanent request block, and dispatch based on the type
     * of request.
     */
    bbrp->cur_reqb = cur_rp;
    bbrp->recursion_ct = 2;
    bbrp->rbn = 0;

    rp->unitb = cur_rp->unitb;
    if( cur_rp->flags.online )
	rp->state = ST_BB_ONLINIT;
    else
	rp->state = ST_BB_REPINIT;
    mscp_dispatch( EV_INITIAL, rp );

    return;
}
/**/

/*
 *
 *   Name:	mscp_bbr_step0 - 
 *
 *   Abstract:	This function reads RCT sector 0
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step0( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;

    Cprintf("mscp_bbr_step0: entered\n");
    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy read.
     */
    bbrp->multi_buf = bbrp->buf0;
    rp->p1 = up->unt_size;
    return( mscp_multi_read( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step0a - 
 *
 *   Abstract:	This function reads RCT sector 1
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step0a( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;

    Cprintf("mscp_bbr_step0a: entered\n");
    /* Set up the buffer address and RCT sector 1 logical block 
     * number and start the multicopy read.
     */
    bbrp->multi_buf = bbrp->buf1;
    rp->p1 = up->unt_size + 1;
    return( mscp_multi_read( rp ));
}
/**/
/*
 *
 *   Name:	mscp_bbr_step0b - 
 *
 *   Abstract:	This function updates RCT sector 0 using 
 *		multicopy write.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step0b( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;

    Cprintf("mscp_bbr_step0b: entered\n");
    /* If multi-read failed then handle it set the FE bit before
     * rewrite.
     */
    if( event == EV_BBRERROR) 
	sp->flags |= RCT_S0_FE;
    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy write.
     */
    bbrp->multi_buf = bbrp->buf0;
    rp->p1 = up->unt_size;
    return( mscp_multi_write( rp ));
}
/**/


/*
 *
 *   Name:	mscp_bbr_step0c - 
 *
 *   Abstract:	This function updates RCT sector 1
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step0c( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;

    Cprintf("mscp_bbr_step0c: entered\n");
    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy write.
     */
    bbrp->multi_buf = bbrp->buf1;
    rp->p1 = up->unt_size + 1;
    return( mscp_multi_write( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step1 - 
 *
 *   Abstract:	This function checks if BBR was in progress. If BBR was
 *		in progress it dispatches to the correct step to continue
 *		the BBR.  If BBR was not in progress it unlocks and returns
 *		to ONLINE processing.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step1( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;
    register u_long		new_event = EV_INITIAL;
    u_long			primary_rbn;

    Cprintf("mscp_bbr_step1: entered\n");
    if (sp->flags & RCT_S0_P1) {
	if (sp->flags & RCT_S0_P2) {
		/* corrupt RCT */
		new_event = EV_BBRERROR;
	}
	else
	    rp->state = ST_BB_STEP7;
    }
    else if (sp->flags & RCT_S0_P2) {
	    if (sp->flags & RCT_S0_P1) {
		/* corrupt RCT */
		new_event = EV_BBRERROR;
	    }
	    else {
		bbrp->rbn = sp->rbn;
		bbrp->flags.bit.p2recov = 1;
		if( sp->flags & RCT_S0_BR ) {
		    bbrp->match_rbn = sp->badrbn;
		    bbrp->flags.bit.match = 1;
	        }
	        rp->state = ST_BB_STEP11;
	    }
    }
    else {	/* No BBR in progress	*/
	Cprintf("No BBR in progress\n");
	mscp_bbr_unlock( rp );
	new_event = EV_NULL;
	return( new_event );
    }	
/*
 *	Check for corrupt lbn
 */
    if ((sp->lbn >= up->unt_size) || (sp->lbn < 0)) {
	new_event = EV_BBRERROR;
    }     		
    else {
	bbrp->lbn = sp->lbn;
	if (bbrp->flags.bit.p2recov) {
	    primary_rbn = (bbrp->lbn/up->track) * up->rbns;
	    if (primary_rbn != bbrp->rbn)
		bbrp->flags.bit.nonprim = 1;
	    else
		bbrp->flags.bit.nonprim = 0;
	}
	if ( sp->flags & RCT_S0_FE )
		bbrp->flags.bit.fe = 1;
    }
    if( new_event == EV_BBRERROR ) {
	bbrp->flags.bit.corrupt = 1;
	rp->state = ST_BB_STEP1;
    }
    return(new_event);

}
/**/

/*
 *
 *   Name:	mscp_bbr_step4 - Read data from bad block
 *
 *   Abstract:	This function attempts to recover the data
 *		from the bad block.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step4( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = bbrp->cur_reqb->msgptr;

    Cprintf("mscp_bbr_step4: entered lbn=%x\n", bbrp->lbn);
    /* Clear the data buffer in case no data can be transferred, and
     * start the first read attempt.
     */
    bbrp->loop_ct1 = 4;
    bbrp->flags.bit.fe = 0;
    if (bbrp->cur_reqb->flags.force == 1)
        bbrp->flags.bit.force = 1;
    ( void )bzero( bbrp->buf1, 512 );
    /* Retrieve the reported bad block from the mscp packet */
    bbrp->lbn = mp->mscp_lbn;
    rp->p1 = bbrp->lbn;
    rp->p2 = 0;
    Cprintf("Bad lbn = %x\n", bbrp->lbn);
    return( mscp_bbr_read( rp, bbrp->buf1 ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step4a - Read data from bad block, continued
 *
 *   Abstract:	This function attempts to recover the data
 *		from the bad block.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step4a( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    u_long			new_event = EV_BBRSUCCESS;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;


    Cprintf("mscp_bbr_step4a: entered, Event = %x lbn=%x\n", event, bbrp->lbn);
    /* If the read completed with forced error status, remember the
     * fact for step 6.
     */
    if(( code == MSCP_ST_DATA ) && ( subcode == MSCP_SC_FRCER ))
	bbrp->flags.bit.fe = 1;

    /* If the read completed with some other error and the retry count
     * has not gone to zero, try to read the data again.  If no retries
     * remain, set the forced error flag for step 6.
     */
    else if( code != MSCP_ST_SUCC ) {
        if( --bbrp->loop_ct1 ) {
	    new_event = mscp_bbr_read( rp, bbrp->buf1 );
	}
	else
	    bbrp->flags.bit.fe = 1;
    }

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_bbr_step5 - Save bad block data in RCT sector 1
 *
 *   Abstract:	This function writes the bad block data read in step 4
 *		into RCT sector 1, using multicopy protection.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step5( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register UNITB		*up = rp->unitb;

    Cprintf("mscp_bbr_step5: entered lbn=%x\n", bbrp->lbn);
    /* Set up the buffer address and RCT1 logical block number
     * and start the multicopy write.
     */
    bbrp->multi_buf = bbrp->buf1;
    rp->p1 = up->unt_size + 1;
    return( mscp_multi_write( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step6 - Phase 1 update to RCT sector 0
 *
 *   Abstract:	This function reads RCT sector 0 so that it can be
 *		updated to reflect the phase 1 state.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step6( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;

    Cprintf("mscp_bbr_step6: entered lbn=%x\n", bbrp->lbn);
    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy read.
     */
    bbrp->multi_buf = bbrp->buf0;
    rp->p1 = up->unt_size;
    return( mscp_multi_read( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step6a - Phase 1 update to RCT sector 0, continued
 *
 *   Abstract:	This function updates RCT sector 0 to reflect the 
 *		phase 1 state.	The sector, read in the first part
 *		of step 6, is modified and rewritten to the RCT.  
 *		This copy of RCT sector 0 is used for all subsequent
 *		sector 0 updates.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step6a( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;

    Cprintf("mscp_bbr_step6a: entered lbn=%x\n", bbrp->lbn);
    /* Update RCT sector 0 by storing the bad block LBN, clearing
     * the BR flag, setting the P1 flag, and setting or clearing the FE
     * flag depending on whether a forced error was seen in step 4.
     */
    sp->lbn = bbrp->lbn;
    sp->flags &= ~( RCT_S0_FE | RCT_S0_BR );
    sp->flags |=  RCT_S0_P1;
    if( bbrp->flags.bit.fe )
	sp->flags |=  RCT_S0_FE;

    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy write.
     */
    bbrp->multi_buf = bbrp->buf0;
    rp->p1 = up->unt_size;
    return( mscp_multi_write( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step7 - Stress test bad block
 *
 *   Abstract:	Step 7 of the bad block replacement algorithm
 *		issues reads and writes with correction and error
 *		recovery disabled to test the block in question.
 *		This function performs step 7 initialization and
 *		does the first read.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step7( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register u_char		*buf1p = bbrp->buf1;
    register u_char		*buf2p = bbrp->buf2;
    register int		i;
    u_long			new_event;

    Cprintf("mscp_bbr_step7: entered, lbn=%x\n", bbrp->lbn);
    /* If we are forcing the replace( from radisk ) then go directly to
     * step 9.
     */
    if( bbrp->flags.bit.force ) {
	rp->state = ST_BB_STEP9;
	return( EV_INITIAL );
    }
    /* Copy and invert the bad block data saved during step 4
     * into buf2.
     */
    for( i = 0; i < 128; i++ )
	*buf2p++ = ~( *buf1p++ );

    /* Initialize the read retry counter, set up and issue the
     * first stress read of the bad block.
     */
    /* TODO fix loop counts to be parameter names and fix the
     * comment above to include both.
     */
    bbrp->loop_ct1 = 4;
    bbrp->loop_ct2 = 8;
    rp->p1 = bbrp->lbn;
    rp->p2 = MSCP_MD_SECOR | MSCP_MD_SEREC;
    new_event = mscp_bbr_read( rp, bbrp->buf3 );

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_bbr_step7a - Stress test bad block, continued
 *
 *   Abstract:	Step 7 of the bad block replacement algorithm
 *		issues reads and writes with correction and error
 *		recovery disabled to test the block in question.
 *		This function performs step 7 initialization.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step7a( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;
    u_long			new_event = EV_NULL;


    Dprintf("mscp_bbr_step7a: entered, Event = %x\n", event);
    /* If the end message status is success and the bad block reported
     * flag is set or the status is a data error other than forced error,
     * the stress test has failed.  Set an error flag and set the return
     * status accordingly.
     */
    if( (( code == MSCP_ST_SUCC )  && ( mp->mscp_flags & MSCP_EF_BBLKR ))  ||
	(( code == MSCP_ST_DATA ) && ( subcode != MSCP_SC_FRCER )) ) {
	bbrp->flags.bit.error = 1;
	new_event = EV_BBRERROR;

    /* If the read succeeded and there are more reads to do, issue the
     * next one.
     */
    } else if( --bbrp->loop_ct1 ) {
	rp->p1 = bbrp->lbn;
	rp->p2 = MSCP_MD_SECOR | MSCP_MD_SEREC;
	new_event = mscp_bbr_read( rp, bbrp->buf3 );

    /* All of the initial stress reads succeeded.  Continue on to the
     * next substep.
     */
    } else {
	new_event = EV_BBRSUCCESS;
    }

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step7b - Stress test bad block, continued
 *
 *   Abstract:	Step 7 of the bad block replacement algorithm
 *		issues reads and writes with correction and error
 *		recovery disabled to test the block in question.
 *		This function writes the saved data, then reads it
 *		back up to 4 times.  An error during writing or
 *		reading means that the stress test has failed and
 *		the block will have to be replaced.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step7b( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event = EV_NULL;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;


    Dprintf("mscp_bbr_step7b: entered, Event = %x\n", event);
    /* If the event indicates initial entry for this substep, initialize
     * the loop counter and write the saved data into the block in question.
     */
    if( event == EV_BBRSUCCESS ) {
	bbrp->loop_ct1 = 4;
	rp->p1 = bbrp->lbn;
	rp->p2 = MSCP_MD_SECOR | MSCP_MD_SEREC;
	new_event = mscp_bbr_write( rp, bbrp->buf1 );

    /* Not initial entry, so an end message must have arrived.  If the
     * the message was a write, and the write succeeded, start up the
     * first read.  If the write failed, set an error flag and set the 
     * return status to cause redispatch to step 8.
     */
    } else if( mp->mscp_endcode == ( MSCP_OP_WRITE | MSCP_OP_END )) {
	if( code == MSCP_ST_SUCC ) {
	    new_event = mscp_bbr_read( rp, bbrp->buf3 );
	} else {
	    bbrp->flags.bit.error = 1;
	    new_event = EV_BBRERROR;
	}
	    
    /* The end message was a read.  If the end message status is success
     * and the bad block reported flag is set or the status is a data
     * error other than forced error, the read failed.  Set an error flag
     * and the return status.
     */
    } else if( (( code == MSCP_ST_SUCC ) && ( mp->mscp_flags & MSCP_EF_BBLKR ))
	|| (( code == MSCP_ST_DATA ) && ( subcode != MSCP_SC_FRCER )) ) {
	bbrp->flags.bit.error = 1;
	new_event = EV_BBRERROR;

    /* If the read succeeded and there are more reads to do, issue the
     * next one.
     */
    } else if( --bbrp->loop_ct1 ) {
	new_event = mscp_bbr_read( rp, bbrp->buf3 );

    /* All of the stress reads succeeded.  Continue to the next substep.
     */
    } else {
	new_event = EV_BBRSUCCESS;
    }

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step7c - Stress test bad block, continued
 *
 *   Abstract:	Step 7 of the bad block replacement algorithm
 *		issues reads and writes with correction and error
 *		recovery disabled to test the block in question.
 *		This function writes the inverse (one's complement) of
 *		the saved data, then reads it back up to 4 times.  An
 *		error during writing or	reading means that the stress
 *		test has failed and the block will have to be replaced.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step7c( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event = EV_NULL;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;


    Dprintf("mscp_bbr_step7c: entered, Event = %x\n", event);
    /* If the event indicates initial entry for this substep, initialize
     * the loop counter and write the inverse of the saved data into the
     * block in question.  The data is written with error correction and
     * error recovery suppressed, and with the forced error modifier set
     * to protect the integrity of the block.
     */
    if( event == EV_BBRSUCCESS ) {
	bbrp->loop_ct1 = 4;
	rp->p1 = bbrp->lbn;
	rp->p2 = MSCP_MD_SECOR | MSCP_MD_SEREC | MSCP_MD_ERROR;
	new_event = mscp_bbr_write( rp, bbrp->buf2 );

    /* Not initial entry, so an end message must have arrived.  If the
     * the message was a write, and the write succeeded, start up the
     * first read.  If the write failed, set an error flag and set the 
     * return status to EV_BBRERROR causing redispatch to step 8.
     */
    } else if( mp->mscp_endcode == ( MSCP_OP_WRITE | MSCP_OP_END )) {
	if( code == MSCP_ST_SUCC ) {
	    new_event = mscp_bbr_read( rp, bbrp->buf3 );
	} else {
	    bbrp->flags.bit.error = 1;
	    new_event = EV_BBRERROR;
	}
	    
    /* The end message was a read.  If the end message status is success
     * and the bad block reported flag is set or the status is a data
     * error other than forced error, the read failed.  Set an error flag
     * and set the return status to EV_BBRERROR.
     */
    } else if( (( code == MSCP_ST_SUCC ) && ( mp->mscp_flags & MSCP_EF_BBLKR ))
	|| (( code == MSCP_ST_DATA ) && ( subcode != MSCP_SC_FRCER )) ) {
	bbrp->flags.bit.error = 1;
	new_event = EV_BBRERROR;

    /* If the read succeeded and there are more reads to do, issue the
     * next one.
     */
    } else if( --bbrp->loop_ct1 ) {
	new_event = mscp_bbr_read( rp, bbrp->buf3 );

    /* All of the stress reads succeeded.  If any more repetitions of the
     * stress test remain, loop back to the substep corresponding to step
     * 7b of the algorithm.  Otherwise,  set the return status to continue
     * on to step 8.
     */
    } else if( --bbrp->loop_ct2 ) {
	new_event = EV_BBRSUCCESS;
    } else {
	new_event = EV_BBRERROR;
    }

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step8 - Write back saved data
 *
 *   Abstract:	Step 8 of the bad block replacement algorithm
 *		writes the saved data back to the bad block, then
 *		reads it back and compares it to the saved data.
 *		If the step 7 stress testing and the step 8 write,
 *		read and compare operations all succeed, the block
 *		in question is not bad and doesn't need to be
 *		replaced.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step8( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event = EV_NULL;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;

    Cprintf("mscp_bbr_step8: entered, Event = %x\n", event);
    /* On initial entry into this substep, initialize the loop counter
     * and write the saved data into the block in question. If the FE flag
     * is set, write the data with the forced error modifier.
     */
    if( (event == EV_BBRSUCCESS) || (event == EV_BBRERROR) ) {
	rp->p1 = bbrp->lbn;
	rp->p2 = 0;
	if( bbrp->flags.bit.fe )
	    rp->p2 |= MSCP_MD_ERROR;
	new_event = mscp_bbr_write( rp, bbrp->buf1 );

    /* Not the initial entry, so an end message arrived.  If the message was
     * a write, and it succeeded, read back the data into another work
     * buffer.  If the write failed or the error flag is set from step 7,
     * set the status to EV_BBRERROR.
     */
    } else if( mp->mscp_endcode == ( MSCP_OP_WRITE | MSCP_OP_END )) {
	if(( code != MSCP_ST_SUCC ) || ( bbrp->flags.bit.error ))
	    new_event = EV_BBRERROR;
	else
	    new_event = mscp_bbr_read( rp, bbrp->buf3 );
	    
    /* The end message was a read.  If the end message status is success
     * or the status is a forced error and the data was written with the
     * forced error modifier, the read succeeded.  Compare the saved data
     * to the data read back.  If the read and the compare both succeeded,
     * set the return status to EV_BBRSUCCESS.  Otherwise set it to
     * EV_BBRERROR.
     */
    } else if(( code == MSCP_ST_SUCC ) || (( code == MSCP_ST_DATA ) &&
	( subcode == MSCP_SC_FRCER ) && ( bbrp->flags.bit.fe )) &&
	( bcmp( bbrp->buf1, bbrp->buf3, 512 ) == 0 )) {
	new_event = EV_BBRSUCCESS;
	bbrp->flags.bit.trans = 1;

    /* The read failed:  set the error flag.
     */
    } else
	new_event = EV_BBRERROR;

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step9 - Search RCT for available RBN
 *
 *   Abstract:	This function invokes the RCT search routine to
 *		find an available replacement block.  In addition,
 *		a recursion counter is checked to ensure that this
 *		step is not reentered an excessive number of times
 *		from step 12 when replacing a bad replacement block.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step9( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register u_long		new_event;

    Cprintf("mscp_bbr_step9: entered\n");
    /* If the recursion count has not yet gone to zero, search the
     * RCT for an available replacement block.  Otherwise, log the
     * error and redispatch with error status.
     */
    if( bbrp->recursion_ct-- )
	new_event = mscp_rct_search( EV_NULL, rp );

    else {
	bbrp->flags.bit.recurs = 1;
	new_event = EV_BBRERROR;
    }

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_bbr_step10 - Phase 2 update to RCT sector 0
 *
 *   Abstract:	This function updates RCT sector 0 to reflect the 
 *		phase 2 state.	The sector, read in step 6, is
 *		modified and written to the RCT.  
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step10( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;

    Cprintf("mscp_bbr_step10: entered\n");
    /* Update RCT sector 0.  Set the phase 2 flag, clear the phase 1 and
     * bad replacement block flags, and store the replacement block number.
     * If the logical block has been replaced previously, set the bad
     * replacement block flag and store the previous (bad) RBN.
     */
    sp->flags |=  RCT_S0_P2;
    sp->flags &=  ~( RCT_S0_P1 | RCT_S0_BR );
    sp->rbn = bbrp->rbn;
    if( bbrp->flags.bit.match ) {
	sp->flags |=  RCT_S0_BR;
	sp->badrbn = bbrp->match_rbn;
    }

    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy write.
     */
    bbrp->multi_buf = bbrp->buf0;
    rp->p1 = up->unt_size;
    return( mscp_multi_write( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step11 - 
 *
 *   Abstract:	This function issues a read for the target RCT descriptor
 *		block if the block has not already been read (i.e. we got
 *		here from online processing)
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step11( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_DESC		*sp;
    u_long			new_block;
    u_long			new_event = EV_BBRSUCCESS;

    Cprintf("mscp_bbr_step11: entered, Event = %x\n", event);
    /* If in Phase 2 recovery must read in RCT block for destination RBN */
    if( (event == EV_INITIAL) && bbrp->flags.bit.p2recov) {
	new_block = ( bbrp->rbn / 128 ) + up->unt_size + 2;
	bbrp->cur_block = new_block;
	bbrp->multi_buf = bbrp->buf2;
	rp->p1 = new_block;
	new_event = mscp_multi_read( rp );
    }
    /* If it was necessary to read in a RCT block, processing
     * will be continued in step11a when the multi-read completes.
     * Otherwise, pass control to step11b.
     */
    return( new_event );

}
/**/
/*
 *
 *   Name:	mscp_bbr_step11a - 
 *
 *   Abstract:	This function updates the RCT descriptor block to record
 *		the replacement. If a previous replacement of this block had
 *		occurred and the descriptor is in a different RCT block, issue
 *		a read for that block.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step11a( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_DESC		*sp;
    u_long			new_block, old_block;
    u_long			new_event = EV_BBRSUBSTEP;

    Cprintf("mscp_bbr_step11a: entered\n");
    Cprintf("lbn = %x, rbn = %x, block = %x\n", bbrp->lbn, bbrp->rbn, bbrp->cur_block);
    /* Calculate the offset of the new replacement descriptor, then
     * fill it in with the bad block lbn and primary/non-primary code.
     * Set a flag to indicate that the RCT has been modified.
     */
	bbrp->flags.bit.rplatt = 1;
	sp = (( RCT_DESC * )bbrp->buf2 ) + ( bbrp->rbn % 128 );
	sp->lbn = bbrp->lbn;
	if( bbrp->flags.bit.nonprim )
	    sp->code = RCT_DS_NONPRIM;
	else
	    sp->code = RCT_DS_PRIMARY;

    /* If there has been a previous replacement of the bad block and the
     * current and previous replacement block descriptors fall into
     * different RCT blocks, read the RCT block that contains the
     * previous replacement block descriptor.  If the descriptors fall
     * into the same RCT block, save a copy of the old descriptor in the
     * BBRB, and fill in the old descriptor with the "unusable" code and a
     * zero LBN.
     */
	if( bbrp->flags.bit.match ) {
	    old_block = ( bbrp->match_rbn / 128 ) + up->unt_size + 2;
	    if( old_block != bbrp->cur_block ) {
		bbrp->multi_buf = bbrp->buf3;
		rp->p1 = old_block;
		new_event = mscp_multi_read( rp );
	    } else {
		sp = (( RCT_DESC * )bbrp->buf2 ) + ( bbrp->match_rbn % 128 );
		bbrp->prev_desc.code = sp->code;
		bbrp->prev_desc.lbn = sp->lbn;
		sp->code = RCT_DS_UNUSABL;
		sp->lbn = 0;
	    }
	}

    /* If it was necessary to read in a second RCT block, processing
     * will be continued in step11b when the multi-read completes.
     * Otherwise, pass control to step11c.
     */
    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step11b - 
 *
 *   Abstract:	This function writes the second updated RCT descriptor
 *		block to disk.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step11b( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_DESC		*sp;
    u_long			new_event;

    Cprintf("mscp_bbr_step11b: entered\n");
    /* Calculate the offset of the previous replacement descriptor, save a
     * copy of the descriptor in the BBRB, and fill it in with the "unusable"
     * descriptor code and a zero lbn.  Then multi-write the block back into
     * the RCT.
     */
    sp = ( RCT_DESC * )bbrp->buf3 + ( bbrp->match_rbn % 128 );

    bbrp->prev_desc.code = sp->code;
    bbrp->prev_desc.lbn = sp->lbn;

    sp->code = RCT_DS_UNUSABL;
    sp->lbn = 0;

    bbrp->multi_buf = bbrp->buf3;
    rp->p1 = ( bbrp->match_rbn / 128 ) + up->unt_size + 2;
    new_event = mscp_multi_write( rp );

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step11c - 
 *
 *   Abstract:	This function writes the original updated RCT descriptor
 *		block to disk.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step11c( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    u_long			new_event;

    Cprintf("mscp_bbr_step11c: entered\n");
    /* Multi-write the descriptor block back into the RCT.
     */
    bbrp->multi_buf = bbrp->buf2;
    rp->p1 = ( bbrp->rbn / 128 ) + up->unt_size + 2;
    new_event = mscp_multi_write( rp );

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step12 - 
 *
 *   Abstract:	This function issues the MSCP REPLACE command to actually
 *		mark the block on disk as replaced.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step12( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register MSCP		*mp = rp->msgptr;
    u_long			new_event;

    Cprintf("mscp_bbr_step12: entered\n");
    /* Format and send a replace command to revector the logical block.
     */
    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = MSCP_OP_REPLC;

    mp->mscp_modifier = MSCP_MD_PRIMR;
    if( bbrp->flags.bit.nonprim )
	mp->mscp_modifier = 0;

    mp->mscp_rbn = bbrp->rbn;
    mp->mscp_lbn = bbrp->lbn;
    new_event = mscp_send_msg( rp );

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step12a - 
 *
 *   Abstract:	This function 
 *
 *   Replacement blocks are initially written with a test pattern and
 *   marked with the forced error indicator.  If this routine is
 *   entered during a normal replacement operation, we can safely test
 *   that the replacement block is still in that initial state.  If the 
 *   replacement process was interrupted, however, we may have overwritten
 *   the the replacement block data, so the test is not valid.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step12a( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;
    register u_long		new_event;

    Cprintf("mscp_bbr_step12a: entered\n");
    /* If the REPLACE command succeeded and the BBR code has been
     * entered to complete a previously interrupted replacement,
     * redispatch.  Otherwise, read the replacement block data.
     */
    if(( code == MSCP_ST_SUCC ) && ( subcode == MSCP_SC_NORML )) {
	if( bbrp->flags.bit.p2recov )
	    new_event = EV_BBRSUCCESS;
	else {
	    rp->p1 = bbrp->lbn;
	    rp->p2 = 0;
	    new_event = mscp_bbr_read( rp, bbrp->buf3 );
	}

    /* Failure of a replacement command is a severe error.  Redispatch
     * to process the command failure.
     */
    } else {
	Cprintf("REPLACE failed: code = %x, subcode = %x\n", code, subcode);
	bbrp->flags.bit.repfail = 1;
	mscp_bbr_log(rp, MSLG_FM_DISK_TRN, code, subcode);
	new_event = EV_BBRERROR;
    }

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step12b - 
 *
 *   Abstract:	This function verifies that the target RBN is unused by
 *		checking that the read end status is a forced error.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step12b( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;
    register u_long		new_event;


    Cprintf("mscp_bbr_step12b: entered\n");
    /* If the read completion status is forced error, the RBN is still
     * in its initial pristine state.  If the status indicates some other
     * data error, the replacement block is bad and another will have to
     * be found.  Redispatch to step 9.  Any other status is treated as a
     * replace command failure.
     * severe error.
     */
    if( code == MSCP_ST_DATA ) {
	if( subcode == MSCP_SC_FRCER )
	    new_event = EV_BBRSUCCESS;
	else {
	    bbrp->match_rbn = bbrp->rbn;
	    bbrp->flags.bit.match = 1;
	    sp->badrbn = bbrp->rbn;
	    sp->flags |= RCT_S0_BR;
	    new_event = EV_BBRSUBSTEP;
	}
    } else {
	Cprintf("code = %x, subcode = %x\n", code, subcode);
	bbrp->flags.bit.repfail = 1;
	mscp_bbr_log(rp, MSLG_FM_DISK_TRN, code, subcode);
	new_event = EV_BBRERROR;
    }
    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step12c - Write back saved data
 *
 *   Abstract:	Step 12c of the bad block replacement algorithm
 *		writes the saved data back to the bad block, then
 *		reads it back and compares it to the saved data.
 *		If the write, read and compare all succeed, the
 *		replacement has succeeded.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step12c( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register u_long		new_event = EV_NULL;
    RCT_SECTOR_0		*sp = ( RCT_SECTOR_0 * )bbrp->buf0;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;

    Cprintf("mscp_bbr_step12c: entered, Event = %x\n", event);
    /* On initial entry into this substep, initialize the loop counter
     * and write the saved data into the block in question. If the FE
     * flag is set, write the data with the forced error modifier.
     */
    if( event == EV_BBRSUCCESS ) {
	rp->p1 = bbrp->lbn;
	rp->p2 = 0;
	if( bbrp->flags.bit.fe ) /* TODO fix this name */
	    rp->p2 |= MSCP_MD_ERROR;
	new_event = mscp_bbr_write( rp, bbrp->buf1 );

    /* Not the initial entry, so an end message arrived.  If the message was
     * a write, and it succeeded, read back the data into another work
     * buffer.  If the write failed  set the status to EV_BBRERROR.
     */
    } else if( mp->mscp_endcode == ( MSCP_OP_WRITE | MSCP_OP_END )) {
	if(( code != MSCP_ST_SUCC ) ) {
	    bbrp->match_rbn = bbrp->rbn;
	    bbrp->flags.bit.match = 1;
	    sp->badrbn = bbrp->rbn;
	    sp->flags |= RCT_S0_BR;
	    new_event = EV_BBRERROR;
	}
	else
	    new_event = mscp_bbr_read( rp, bbrp->buf3 );
	    
    /* The end message was a read.  If the end message status is success
     * or the status is a forced error and the data was written with the
     * forced error modifier, the read succeeded.  Compare the saved data
     * to the data read back.  If the read and the compare both succeeded,
     * set the return status to EV_BBRSUCCESS.  Otherwise set it to
     * EV_BBRERROR.
     */
    } else if(( code == MSCP_ST_SUCC ) || (( code == MSCP_ST_DATA ) &&
	( subcode == MSCP_SC_FRCER ) && ( bbrp->flags.bit.fe )) &&
	( bcmp( bbrp->buf1, bbrp->buf3, 512 ) == 0 )) {
	new_event = EV_BBRSUCCESS;

    /* The read failed:  set the error flag.
     */
    } else {
	bbrp->match_rbn = bbrp->rbn;
	bbrp->flags.bit.match = 1;
	sp->badrbn = bbrp->rbn;
	sp->flags |= RCT_S0_BR;
	new_event = EV_BBRERROR;
    }

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step12d - 
 *
 *   Abstract:	This function writes the saved data back to the original
 *		lbn.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step12d( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register CONNB 		*cp = rp->connb;
    register MSCP		*mp = rp->msgptr;
    u_long			new_event;


    Cprintf("mscp_bbr_step12d: entered\n");
    printf("\t\tWARNING\n");
    printf("REPLACE command failure at lbn %d\n", bbrp->lbn);
    printf("on controller %d unit %d. Notify Field Service \n", cp->cnt_number,
							up->unit);
    printf("or consult the System Managers Guide.\n");
    rp->p1 = bbrp->lbn;
    rp->p2 = 0;
    if( bbrp->flags.bit.fe )
	rp->p2 |= MSCP_MD_ERROR;
    new_event = mscp_bbr_write( rp, bbrp->buf1 );

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step12e - 
 *
 *   Abstract:	This function issues a MSCP SET UNIT CHARACTERISTICS command
 *		to software write-protect the unit.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step12e( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register MSCP		*mp = rp->msgptr;
    u_long			new_event;

    Cprintf("mscp_bbr_step12e: entered\n");
    /* Format and send a set unit characteristics command to
     * write protect the unit.
     */
    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = MSCP_OP_STUNT;
    mp->mscp_modifier = MSCP_MD_STWRP;
    mp->mscp_unt_flgs = MSCP_UF_WRTPS;

    new_event = mscp_send_msg( rp );

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step13 - End of replacement update to RCT 0
 *
 *   Abstract:	This function updates RCT sector 0 to reflect the 
 *		fact that replacement is no longer taking place.
 *		The sector, read in step 6, is modified and rewritten
 *		to the RCT.  
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step13( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;

    Cprintf("mscp_bbr_step13: entered\n");
    /* Update RCT sector 0 by clearing the replacement flags, the lbn
     * being replaced, the replacement block number, and the bad
     * replacement block number.
     */
    sp->flags = 0;
    sp->lbn = 0;
    sp->rbn = 0;
    sp->badrbn = 0;

    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy write.
     */
    bbrp->multi_buf = bbrp->buf0;
    rp->p1 = up->unt_size;
    return( mscp_multi_write( rp ));
}
/**/
/*
 *
 *   Name:	mscp_bbr_step14 - Clean up and release locks
 *
 *   Abstract:	This function logs the successful replacement attempt in
 *		the system error log and calls bbr_unlock() to restart the
 *		original I/O.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step14( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;
    register u_long		flags, subcode;

    Cprintf("mscp_bbr_step14: entered\n");
    flags = 0;
    subcode = 0;
    if(bbrp->flags.bit.fe)
	flags |= MSLG_LFR_FE;
    if(bbrp->flags.bit.nonprim)
	flags |= MSLG_LFR_TE;
    if(bbrp->flags.bit.match)
	flags |= MSLG_LFR_BR;
    if(bbrp->flags.bit.trans)
	subcode = MSCP_SC_NOTRP;
    else {
	subcode = MSCP_SC_BBROK;
	flags |= MSLG_LFR_RP;
    }
    mscp_bbr_log(rp, MSLG_FM_REPLACE, flags, subcode);
    mscp_bbr_unlock(rp);
    return( EV_NULL );
}
/**/

/*
 *
 *   Name:	mscp_bbr_step15 - 
 *
 *   Abstract:	This function attempts to "undo" the failed replacement
 *		by restoring the RCT entries to their previous state.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step15( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_DESC		*sp;
    u_long			prev_block;
    u_long			prev_offset;
    u_long			new_event;

    Cprintf("mscp_bbr_step15: entered\n");
    /* Calculate the offset of the new replacement descriptor and
     * mark it as unallocated, with an LBN of 0.
     */
    sp = ( RCT_DESC * )bbrp->buf2 + ( bbrp->rbn % 128 );
    sp->code = RCT_DS_UNALLOC;
    sp->lbn = 0;

    /* Calculate the LBN and offset of the previous replacement descriptor.
     * Fill in the descriptor with the "unusable" descriptor code and a zero
     * lbn.  If the previous descriptor is in a different RCT block from the
     * current descriptor, multi-write the previous block into the RCT.
     * If the descriptors are in the same block, redispatch to write that
     * block immediately.
     */
    prev_block = ( bbrp->match_rbn / 128 ) + up->unt_size + 2;
    prev_offset =  bbrp->match_rbn % 128;

    if( prev_block != bbrp->cur_block ) {
	sp = ( RCT_DESC * )bbrp->buf3 + prev_offset;
	sp->lbn = bbrp->prev_desc.lbn;
	sp->code = bbrp->prev_desc.code;
	bbrp->multi_buf = bbrp->buf3;
	rp->p1 = ( bbrp->match_rbn / 128 ) + up->unt_size + 2;
	new_event = mscp_multi_write( rp );

    } else {
	sp = ( RCT_DESC * )bbrp->buf2 + prev_offset;
	sp->lbn = bbrp->prev_desc.lbn;
	sp->code = bbrp->prev_desc.code;
	new_event = EV_BBRSUBSTEP;
    }

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step15a - 
 *
 *   Abstract:	This function writes the second "restored" RCT 
 *		block to the RCT.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step15a( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    u_long			new_event;

    Cprintf("mscp_bbr_step15a: entered\n");
    /* Multi-write the descriptor block back into the RCT.
     */
    bbrp->multi_buf = bbrp->buf2;
    rp->p1 = ( bbrp->rbn / 128 ) + up->unt_size + 2;
    new_event = mscp_multi_write( rp );

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step16 - 
 *
 *   Abstract:	This function attempts to write the saved data back to
 *		the original LBN.  It also logs an error to the system
 *		error log if the RCT is full or corrupt.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step16( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    u_long			new_event, flags, subcode;


    Cprintf("mscp_bbr_step16: entered\n");
    flags = 0;
    switch( event ) {

    case EV_BBRRCTFULL:

	bbrp->flags.bit.full = 1;
	subcode = MSCP_SC_NORBL;
	mscp_bbr_log(rp, MSLG_FM_DISK_TRN, flags, subcode);
	break;

    case EV_BBRINVRCT:

	bbrp->flags.bit.corrupt = 1;
	subcode = MSCP_SC_RCTBD;
	mscp_bbr_log(rp, MSLG_FM_DISK_TRN, flags, subcode);
	break;

    default:
	break;
    }
    rp->p1 = bbrp->lbn;
    rp->p2 = 0;
    if( bbrp->flags.bit.fe )
	rp->p2 |= MSCP_MD_ERROR;
    new_event = mscp_bbr_write( rp, bbrp->buf1 );

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_bbr_step17 - End of replacement update to RCT 0
 *
 *   Abstract:	This function updates RCT sector 0 to reflect the 
 *		fact that replacement is no longer taking place.
 *		The sector, read in step 6, is modified and rewritten
 *		to the RCT.  
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step17( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register RCT_SECTOR_0	*sp = ( RCT_SECTOR_0 * )bbrp->buf0;

    Cprintf("mscp_bbr_step17: entered\n");
    /* Update RCT sector 0 by clearing the replacement flags, the lbn
     * being replaced, the replacement block number, and the bad
     * replacement block number.
     */
    sp->flags = 0;
    sp->lbn = 0;
    sp->rbn = 0;
    sp->badrbn = 0;

    /* Set up the buffer address and RCT sector 0 logical block 
     * number and start the multicopy write.
     */
    bbrp->multi_buf = bbrp->buf0;
    rp->p1 = up->unt_size;
    return( mscp_multi_write( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_step18 - Write protect the disk
 *
 *   Abstract:	This function logs the replacement attempt to the system
 *		error log and calls bbr_unlock() to restart the stalled
 *		I/O.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_bbr_step18( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register MSCP		*mp = rp->msgptr;
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    u_long			flags, subcode;

    Cprintf("mscp_bbr_step18: entered\n");
    subcode = 0;
    flags = 0;
    if(bbrp->flags.bit.full) {
	subcode = MSCP_SC_RCTFL;
	if(bbrp->flags.bit.match)
	    flags = MSLG_LFR_BR;
    }
    else if(bbrp->flags.bit.mwfail) {
	flags = MSLG_LFR_RI;
	subcode = MSCP_SC_ICRCT;
	if(bbrp->flags.bit.match)
	    flags |= MSLG_LFR_BR;
	if(bbrp->flags.bit.rplatt)
	    flags |= MSLG_LFR_RP;
    }
    else if(bbrp->flags.bit.mrfail) {
	flags = 0;
	subcode = MSCP_SC_ICRCT;
	if(bbrp->flags.bit.match)
	    flags |= MSLG_LFR_BR;
    }
    else if(bbrp->flags.bit.corrupt) {
	flags = MSLG_LFR_RI;
	subcode = MSCP_SC_ICRCT;
	if(bbrp->flags.bit.match)
	    flags |= MSLG_LFR_BR;
    }
    else if(bbrp->flags.bit.recurs) {
	subcode = MSCP_SC_RCTRC;
	flags = (MSLG_LFR_BR | MSLG_LFR_RP);
    }
    else if(bbrp->flags.bit.repfail) {
	flags = (MSLG_LFR_RP | MSLG_LFR_RF);
	if(bbrp->flags.bit.match)
	    flags |= MSLG_LFR_BR;
	subcode = MSCP_SC_RPLFL;
    }
    mscp_bbr_log(rp, MSLG_FM_REPLACE, flags, subcode);
    mscp_bbr_unlock(rp);
    return( EV_NULL );


}
/**/

/*
 *
 *   Name:	mscp_rct_search - 
 *
 *   Abstract:	This function issues the multi-read for the RCT block
 *		which contains the primary RBN entry for the LBN we are
 *		replacing.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_rct_search( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register UNITB		*up = rp->unitb;
    register BBRB		*bbrp = rp->connb->bbrb;

    Cprintf("mscp_rct_search: entered\n");
    Push_bbr_state( ST_BB_RCTSEARCH );
    bbrp->hash_rbn = ( bbrp->lbn / up->track ) * up->rbns;
    bbrp->max_host_rbn = ((( up->unt_size - 1 ) / up->track ) + 1 ) * up->rbns;
    bbrp->hash_block = ( bbrp->hash_rbn / 128 ) + up->unt_size + 2;
    bbrp->hash_offset = bbrp->hash_rbn % 128;
    bbrp->cur_block = bbrp->hash_block;
    bbrp->cur_rbn = bbrp->hash_rbn;

    bbrp->multi_buf = bbrp->buf2;
    rp->p1 = bbrp->hash_block;
    return( mscp_multi_read( rp ));
}
/**/

/*
 *
 *   Name:	mscp_rct_searcha - 
 *
 *   Abstract:	This function searchs the original hashed RCT block for
 *		a free RBN.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_rct_searcha( event, rp )
    u_long			event;
    REQB			*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register RCT_SECTOR_K	*sp = ( RCT_SECTOR_K * )bbrp->buf2;
    register RCT_DESC		*descp;
    register int		delta = 0;
    register int		offset = bbrp->hash_offset;
    register int		step;
    u_long			new_event = EV_NULL;

    Cprintf("mscp_rct_searcha: entered\n");
    /* Search outward from the primary replacement block descriptor
     * until an available descriptor is found or another terminating
     * condition is seen:
     *    descriptor block overflow or underflow,
     *    end of the RCT,
     *    or corrupted RCT.
     */
    while(( offset >= 0 ) &&
	  ( offset < 128 ) &&
	  ( new_event == EV_NULL ) &&
	  ( !bbrp->flags.bit.eot )) {
	descp = &sp->desc[offset];
	new_event = mscp_test_descr( bbrp, descp );
	delta = -delta;
	if( delta >= 0 )
	    delta++;
	offset = bbrp->hash_offset + delta;
	bbrp->cur_rbn = bbrp->hash_rbn + delta;
    }

    /* If block overflow or underflow occurred without finding an
     * available descriptor or the end of the RCT, find the next
     * unexamined descriptor, and do a linear search of the remainder
     * of the block.
     */
    if( new_event == EV_NULL && !bbrp->flags.bit.eot ) {
	delta = -delta;
	if( delta >= 0 ) {
	    delta++;
	    step = 1;
	} else
	    step = -1;
	offset = bbrp->hash_offset + delta;
	bbrp->cur_rbn = bbrp->hash_rbn + delta;
	while(( offset >= 0 ) &&
	      ( offset < 128 ) &&
	      ( new_event == EV_NULL ) &&
	      ( !bbrp->flags.bit.eot )) {
	    descp = &sp->desc[offset];
	    new_event = mscp_test_descr( bbrp, descp );
	    offset +=  step;
	    bbrp->cur_rbn += step;
	}
    }

    /* If an available descriptor has been found or the RCT is corrupted,
     * pop the BBR state stack, and return status to the caller.
     */
    if( new_event != EV_NULL ) {
	Pop_bbr_state();

    /* No available descriptor has been found; continue
     * processing in the next search substep.
     */
    } else
	new_event = EV_BBRSUBSTEP;

    return( new_event );
   
}
/**/

/*
 *
 *   Name:	mscp_rct_searchb - 
 *
 *   Abstract:	This function searchs RCT blocks other than the original
 *		hashed block.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_rct_searchb( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register RCT_DESC		*descp;
    register RCT_SECTOR_K	*sp = ( RCT_SECTOR_K * )bbrp->buf2;
    int				offset = 0;
    u_long			new_event = EV_NULL;

    Cprintf("mscp_rct_searchb: entered, Event = %x\n", event);
    while( new_event == EV_NULL ) {

	/* If first time through or end of RCT block, calculate the LBN of
	 * the next RCT block to read.  If the end of the RCT has been seen,
	 * wrap around to the beginning.  Calculate the RBN corresponding to
	 * the start of the RCT block.  If the RBN is equal to the original
	 * hashed primary descriptor RBN, the entire RCT has been searched
	 * without encountering an available replacement block.  In this case,
	 * return RCT full status; otherwise, read the new block and exit to
	 * wait for the read to complete.
	 */
	if(( event == EV_BBRSUBSTEP ) || ( offset >= 128 )) {
	    if( !bbrp->flags.bit.eot ) 
		bbrp->cur_block++;
	    else {
		bbrp->cur_block = rp->unitb->unt_size + 2;
		bbrp->flags.bit.eot = 0;
	    }
	    bbrp->cur_rbn =
		128 * ( bbrp->cur_block - ( rp->unitb->unt_size + 2 ));
	    if( bbrp->cur_rbn == bbrp->hash_rbn ) {
		new_event = EV_BBRRCTFULL;
	    } else {
		bbrp->multi_buf = bbrp->buf2;
		rp->p1 = bbrp->cur_block;
		return( mscp_multi_read( rp ) );
		break;
	    }

	/* The read completed.  Do a linear search of the RCT block, looking
	 * for an available replacement block descriptor.  If the current
	 * RBN matches the hashed primary descriptor RBN at any point in the
	 * search, exit with RCT full status.
	 */
	} else {
	    while(( offset < 128 ) &&
		  ( new_event == EV_NULL ) &&
		  ( !bbrp->flags.bit.eot )) {
		descp = &sp->desc[offset];
		new_event = mscp_test_descr( bbrp, descp );
		offset++;
		bbrp->cur_rbn++;
		/* If bumped rbn equals start and previous RBN was not
		 * free, then RCT is full.
		 */
		if( (bbrp->cur_rbn == bbrp->hash_rbn) && 
				(new_event == EV_NULL) )
		    new_event = EV_BBRRCTFULL;
	    }
	}
    }

    /* If the new_event is not null, the search has terminated.  Pop into
     * the context of the caller before exiting.  If the new_event IS null,
     * the multi-read is underway and will return to this routine when
     * it completes.
     */
    if( new_event != EV_NULL )
	Pop_bbr_state();
    
    /* If at the end of the RCT then wrap to beginning
     */
    if( bbrp->flags.bit.eot )
	new_event = EV_BBRSUBSTEP;

    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_rct_searchc - Report errors back
 *
 *   Abstract:	This function pops the bbrstate and returns the event
 *		to the previous thread.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_rct_searchc( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;

	Pop_bbr_state();
	return( event );
}
/**/

/*
 *
 *   Name:	mscp_test_descr - 
 *
 *   Abstract:	This function examines the RCT descriptor which is passed
 *		as an argument. It sets the event based on the state of
 *		the descriptor.
 *
 *   Inputs:	IPL_SCS
 *		bbrp			BBR block pointer
 *		descp			RCT descriptor pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_test_descr( bbrp, descp )

    register BBRB		*bbrp;
    register RCT_DESC		*descp;

{
    register u_long		code = descp->code;
    register u_long		lbn = descp->lbn;
    register u_long		new_event = EV_NULL;

    Cprintf("mscp_test_descr: entered, code = %x, lbn = %x\n", code, lbn);
    /* If the replacement block is allocated and the descriptor lbn is
     * the same as the lbn of the block being tested, set a flag to
     * remember the match and save the RBN.
     */
    if( code == RCT_DS_PRIMARY || code == RCT_DS_NONPRIM ) {
	if( lbn == bbrp->lbn ) {
	    bbrp->flags.bit.match = 1;
	    bbrp->match_rbn = bbrp->cur_rbn;
	}

    /* If the descriptor code indicates something other than an allocated
     * replacement block, the lbn portion of the descriptor must be zero;
     * otherwise, the RCT is corrupted.
     */
    } else if( lbn != 0 )
	    new_event = EV_BBRINVRCT;

    /* If the descriptor is unallocated, an available replacement block
     * has been found.  Store the RBN and return success.
     */
    else if( code == RCT_DS_UNALLOC ) {
	    bbrp->rbn = bbrp->cur_rbn;
	    new_event = EV_BBRSUCCESS;

    /* If the descriptor is null and the RBN is greater than the largest
     * valid RBN for the unit, set the end of table flag.  Otherwise, the
     * RCT is corrupted. 
     */
    } else if( code == RCT_DS_NULL ) {
	if( bbrp->max_host_rbn < bbrp->cur_rbn )
	    bbrp->flags.bit.eot = 1;
	else
	    new_event = EV_BBRINVRCT;
	
    /* If the descriptor code is not recognizable, the RCT is corrupted.
     */
    } else if( code != RCT_DS_UNUSABL && code != RCT_DS_UNUSABLALT )
	    new_event = EV_BBRINVRCT;

    /* If an available descriptor was not found, set the nonprimary
     * flag.
     */
    if ( new_event != EV_BBRSUCCESS )
	bbrp->flags.bit.nonprim = 1;

    return( new_event );

}
/**/

/*
 *
 *   Name:	mscp_multi_read - Initiate read on multi-copy structure
 *
 *   Abstract:	This function starts the multi-read algorithm by issuing
 *		the read for the first copy of the RCT block.
 *
 *   Inputs:	IPL_SCS
 *		rp			Request block pointer
 *
 *   Implicit	
 *   Inputs:	
 *		rp->p1			LBN of first copy of RCT block
 *		bbrp->multi_buf		Address of data buffer
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_multi_read( rp )
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;

    Cprintf("mscp_bbr_multi_read: entered\n");
    Push_bbr_state( ST_BB_MULTIREAD );
    bbrp->copy_ct = 0;
    rp->p2 = 0;
    return( mscp_bbr_read( rp, bbrp->multi_buf ) );
}
/**/

/*
 *
 *   Name:	mscp_multi_read_cont - Continue/complete multi-copy read
 *
 *   Abstract:	This function implements the bulk of the multi-read algorithm
 *		as described in DSDF.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_multi_read_cont( event, rp )
    u_long			event;
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register MSCP		*mp = rp->msgptr;
    u_long			new_event = EV_BBRSUCCESS;
    u_long			flags;
    u_long			code = mp->mscp_status & MSCP_ST_MASK;
    u_long			subcode = mp->mscp_status >> MSCP_ST_SBBIT;

    Cprintf("mscp_multi_read_cont: entered\n");
    if(( mp->mscp_status & MSCP_ST_MASK ) == MSCP_ST_SUCC ) {
	Pop_bbr_state();
    } else if( ++bbrp->copy_ct < up->rct_cpys ) {
	rp->p1 += up->rct_size;
	new_event = mscp_bbr_read( rp, bbrp->multi_buf );
    } else {
	Cprintf("multi_read failed!!\n");
	bbrp->flags.bit.mrfail = 1;
	if( code == MSCP_ST_DATA )
		code = MSCP_ST_MFMTE;
	mscp_bbr_log(rp, MSLG_FM_DISK_TRN, code, subcode);
	new_event = EV_BBRERROR;
	Pop_bbr_state();
    }
    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_multi_write - Initiate write to multi-copy structure
 *
 *   Abstract:	This function starts the multi-write alogorithm by
 *		issuing the write for the first copy of the RCT block.
 *
 *   Inputs:	IPL_SCS
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *		rp->p1			LBN of first copy of RCT block
 *		bbrp->multi_buf		Address of data buffer
 *
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_multi_write( rp )
    register REQB		*rp;
{
    register BBRB		*bbrp = rp->connb->bbrb;

    Cprintf("mscp_multi_write: entered\n");
    Push_bbr_state( ST_BB_MULTIWRITE );
    bbrp->copy_ct = 0;
    bbrp->bad_copies = 0;
    rp->p2 = MSCP_MD_COMP;
    return( mscp_bbr_write( rp, bbrp->multi_buf ) );
}
/**/

/*
 *
 *   Name:	mscp_multi_write_cont - Continue/complete multi-copy write
 *
 *   Abstract:	This function implements the bulk of the multi-write
 *		algorithm as defined in DSDF.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	EV_NULL
 */

u_long
mscp_multi_write_cont( event, rp )

    u_long			event;
    register REQB		*rp;

{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register MSCP		*mp = rp->msgptr;
    u_long			new_event = EV_NULL;

    register u_long		code = ( mp->mscp_status & MSCP_ST_MASK );
    register u_long		subcode = ( mp->mscp_status >> MSCP_ST_SBBIT );
    u_long			logcode, flags;

    Cprintf("mscp_multi_write_cont: entered\n");
    /* If the returned status is not success and is not a data error
     * other than forced error, the multi-write failed.
     */
    if( !(( code == MSCP_ST_SUCC ) ||
	  ( code == MSCP_ST_DATA && subcode != MSCP_SC_FRCER ))) {
	Cprintf("multi_write failed!\n");
	bbrp->flags.bit.mwfail = 1;
	if( code == MSCP_ST_DATA )
		code = MSCP_ST_MFMTE;
	mscp_bbr_log(rp, MSLG_FM_DISK_TRN, code, subcode);
	new_event = EV_BBRERROR;
	Pop_bbr_state();
    
    /* If the write succeeded or the state indicates that we are processing
     * an end message for a write with forced error, reset the state, and
     * attempt to write the next copy of the block. 
     */
    } else if(( code == MSCP_ST_SUCC ) || ( rp->state == ST_BB_MULTIWRITE2 )) {
	rp->state = ST_BB_MULTIWRITE;

	/* If any copies remain to be written, issue the next write.
	 */
	if( ++bbrp->copy_ct < up->rct_cpys ) {
	    rp->p1 += up->rct_size;
	    rp->p2 = MSCP_MD_COMP;
	    new_event = mscp_bbr_write( rp, bbrp->multi_buf );

	/* All copies have been written.  If at least one write succeeded,
	 * the multi-write succeeded.  If exactly one write succeeded, write
	 * an error log entry to indicate that the block is not multi-copy
	 * protected.
	 */
	} else if( bbrp->bad_copies < bbrp->copy_ct ) {
	    new_event = EV_BBRSUCCESS;
	    Pop_bbr_state();
	    if (( bbrp->bad_copies == bbrp->copy_ct - 1 ) &&
		(up->rct_cpys > 1)) { /* Do not log Multicopy warning
					if there was originally just
					one copy, e.g., an ESE20 */
		logcode = MSCP_SC_MULT;
		flags = MSCP_ST_MFMTE;
		mscp_bbr_log(rp, MSLG_FM_DISK_TRN, flags, logcode);
		Cprintf("multi_write: Only one good copy!\n");
	    }

	/* There were no successful writes.  The multi-write failed.
	 */
	} else {
	    Cprintf("multi_write failed!!\n");
	    bbrp->flags.bit.mwfail = 1;
	    if( code == MSCP_ST_DATA )
		code = MSCP_ST_MFMTE;
	    mscp_bbr_log(rp, MSLG_FM_DISK_TRN, code, subcode);
	    new_event = EV_BBRERROR;
	    Pop_bbr_state();
	}

    /* The end message status indicates a data error other than forced
     * error.  Change state to avoid looping on data errors, increment
     * the bad copy counter and re-write the data with the forced error
     * modifier set.
     */
    } else {
	bbrp->bad_copies++;
	rp->state = ST_BB_MULTIWRITE2;
	rp->p2 = MSCP_MD_ERROR;
	new_event = mscp_bbr_write( rp, bbrp->multi_buf );
    }

    /* Return new_event to the event dispatcher.  Note that in some cases
     * the BBR state stack will have been popped so that the event being
     * returned will be dispatched against the new state.
     */
    return( new_event );
}
/**/

/*
 *
 *   Name:	mscp_bbr_read - Format and send an MSCP READ command
 *
 *   Abstract:	This function fills in the buf structure and the MSCP packet
 *		and then calls map_buffer to map the data buffer.
 *
 *   Inputs:	IPL_SCS
 *		rp			Request block pointer
 *		addr			Address of data buffer
 *
 *   Implicit
 *   Inputs:	
 *		rp->p1			LBN to read
 *		rp->p2			MSCP modifiers to use on read command
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_read( rp, addr )

    register REQB		*rp;
    u_char			*addr;

{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register struct buf		*bp = rp->bufptr;

    Dprintf("mscp_bbr_read: entered: lbn = %x mod = %x\n", rp->p1,rp->p2);
    Push_bbr_state( ST_BB_READ );
    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = MSCP_OP_READ;
    mp->mscp_byte_cnt = BBR_BLOCKSIZE;
    mp->mscp_lbn = rp->p1;
    mp->mscp_modifier = rp->p2;

    /* Fill in the buf structure with the minimum amount of
     * information needed to map the buffer.
     */
    bp->b_un.b_addr = ( caddr_t )addr;
    bp->b_flags = B_BUSY;
    bp->b_bcount = BBR_BLOCKSIZE;

    /* Issue the map call and return its status to our caller.
     */
    return( mscp_map_buffer( EV_NULL, rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_write - Format and send an MSCP WRITE command
 *
 *   Abstract:	This function fills in the buf structure and the MSCP packet
 *		and then calls map_buffer to map the data buffer.
 *
 *   Inputs:	IPL_SCS
 *		rp			Request block pointer
 *		addr			Address of data buffer
 *
 *   Implicit
 *   Inputs:	
 *		rp->p1			LBN to read
 *		rp->p2			MSCP modifiers to use on write command
 *
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_write( rp, addr )

    register REQB		*rp;
    u_char			*addr;

{
    register BBRB		*bbrp = rp->connb->bbrb;
    register MSCP		*mp = rp->msgptr;
    register struct buf		*bp = rp->bufptr;

    Dprintf("mscp_bbr_write: entered: lbn = %x mod = %x\n", rp->p1,rp->p2);
    Push_bbr_state( ST_BB_WRITE );
    Init_msg( mp, rp->rspid, rp->unitb->unit );
    mp->mscp_opcode = MSCP_OP_WRITE;
    mp->mscp_byte_cnt = BBR_BLOCKSIZE;
    mp->mscp_lbn = rp->p1;
    mp->mscp_modifier = rp->p2;

    /* Fill in the buf structure with the minimum amount of
     * information needed to map the buffer.
     */
    bp->b_un.b_addr = ( caddr_t )addr;
    bp->b_flags = B_BUSY;
    bp->b_bcount = BBR_BLOCKSIZE;

    /* Issue the map call and return its status to our caller.
     */
    return( mscp_map_buffer( EV_NULL, rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_rwcont - Continue BBR read/write common processing
 *
 *   Abstract:	This function stores the buffer handle in the MSCP message
 *		and calls send_msg to send the command.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_rwcont( event, rp )

    u_long			event;
    REQB			*rp;

{
    MSCP			*mp = rp->msgptr;

    Dprintf("mscp_bbr_rwcont: entered, rp = %x, event = %x\n", rp, event);
    Move_bhandle( rp->lbhandle, mp->mscp_buffer[ 0 ] );
    return( mscp_send_msg( rp ));
}
/**/

/*
 *
 *   Name:	mscp_bbr_rwfin - Finish BBR read/write common processing
 *
 *   Abstract:	This function unmaps the buffer handle, pops the bbrstate,
 *		and returns to the previous thread.
 *
 *   Inputs:	IPL_SCS
 *		event			Event code
 *		rp			Request block pointer
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_rwfin( event, rp )

    u_long			event;
    REQB			*rp;

{
    BBRB			*bbrp = rp->connb->bbrb;

    Dprintf("mscp_bbr_rwfin: entered\n");
    mscp_unmap_buffer( rp );
    Zero_bhandle( rp->lbhandle );
    Pop_bbr_state();
    return( event );
}
/**/

/*
 *
 *   Name:	mscp_bbr_log
 *
 *   Abstract:	This function logs errors to the system errorlog file
 *
 *   Inputs:	IPL_SCS
 *		rp			Request block pointer
 *		format			MSCP error log format to be used
 *		flags			For replacement logs: replace flags
 *					For other logs: event code
 *		subcode			MSCP event subcode to log
 *
 *   Implicit
 *   Inputs:	
 *
 *   Outputs:
 *
 *
 *   Return	
 *   Values:	NONE
 */

u_long
mscp_bbr_log( rp, format, flags, subcode )

    REQB			*rp;
    u_long			format, flags, subcode;

{
    register BBRB		*bbrp = rp->connb->bbrb;
    register UNITB		*up = rp->unitb;
    register CONNB 		*cp = rp->connb;
    MSCP			*mp = bbrp->cur_reqb->msgptr;
    MSLG			*bbrmp = &bbrp->bbr_mslg;
    u_long			code;

    Cprintf("mscp_bbr_log: entered, mp = %x\n", mp);

    bbrp->flags.bit.logerr = 1;
    bbrmp->mslg_flags = 0;
    switch( format ) {

    case MSLG_FM_REPLACE:

	code = MSCP_ST_BBR;
	/* Fill in error log packet
	 */
	bbrmp->mslg_rpl_flgs = flags;
	bbrmp->mslg_bad_lbn = bbrp->lbn;
	bbrmp->mslg_new_rbn = bbrp->rbn;
	if( bbrp->flags.bit.match )
	    bbrmp->mslg_old_rbn = bbrp->match_rbn;
	else
	    bbrmp->mslg_old_rbn = 0;
	bbrmp->mslg_cause = mp->mscp_status;
	break;

    case MSLG_FM_DISK_TRN:

	/* Fill in error log packet with last datagram */
	if ( mp->mscp_flags == MSCP_EF_ERLOG ) {
	    format = bbrmp->mslg_format;
	}

	bbrmp->mslg_flags = MSLG_LF_RPLER;

	switch( subcode ) {

	case MSCP_SC_NORBL:
	case MSCP_SC_RCTBD:
		code = MSCP_ST_MFMTE;
		break;

	default:	/* Multi-read or multi-write error */
		code = flags;
		break;
	}
	switch( code ) {

	case MSCP_ST_MFMTE:
	case MSCP_ST_DATA:
	case MSCP_ST_DRIVE:
	    bbrmp->mslg_hdr_code = rp->p1;
	    break;
	default: 
	    break;
	}

	break;
    }

    bbrmp->mslg_cmd_ref = mp->mscp_cmd_ref;
    bbrmp->mslg_seq_num = 0;
    bbrmp->mslg_format = (u_char)format;
    bbrmp->mslg_event = code | ( subcode << MSCP_ST_SBBIT);

    *( UNIQ_ID * )bbrmp->mslg_cnt_id = *( UNIQ_ID * )&cp->cnt_id; 
    bbrmp->mslg_cnt_svr = cp->cnt_svr;
    bbrmp->mslg_cnt_hvr = cp->cnt_hvr;

    if (code != MSCP_ST_CNTLR && code != MSCP_ST_HSTBF) {
        bbrmp->mslg_unit = up->unit;
        *( UNIQ_ID * )bbrmp->mslg_unit_id = *( UNIQ_ID * )&up->unit_id; 
        bbrmp->mslg_unit_svr = up->unit_svr;
        bbrmp->mslg_unit_hvr = up->unit_hvr;
        bbrmp->mslg_mult_unt = up->mult_unt;
        bbrmp->mslg_vol_ser = up->vol_ser;
    }

    mscp_logerr( cp, bbrmp, sizeof(MSLG));
    bbrp->flags.bit.logerr = 0;
    return(0);
}
