#ifndef	lint
static char *sccsid = "@(#)scs_directory.c	4.1	(ULTRIX)	7/2/90";
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
 *		Systems Communication Services Directory SYSAP
 *
 *   Abstract:	This module contains Systems Communication Services
 *		Directory SYSAP( SCS$DIRECTORY ) routines.
 *
 *   Creator:	Todd M. Katz	Creation Date:	August 25, 1985
 *
 *   Functions/Routines:
 *
 *	scs$dir_control		Process SCS$DIRECTORY Connection Events
 *	scs$dir_init		Initialize SCS$DIRECTORY
 *	scs$dir_receive		Receive and Process Directory Requests
 *
 *   Creator:	Todd M. Katz	Creation Date:	August 25, 1985
 *
 *   Modification History:
 *
 *   07-Mar-1989	Todd M. Katz		TMK0004
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   26-Sep-1988	Todd M. Katz		TMK0003
 *	Remove SCS$DIRECTORY comments, definitions, and macros.  They have
 *	resided within ../vaxsysap/scs_directory.h for some time now.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0002
 *	Added comments.
 *
 *   08-Dec-1987	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, and
 *	added SMP support by altering routine logic to make any SMP locking
 *	unnecessary.
 */

/*
 *			SCS Directory SYSAP Protocol
 *
 * The SCS Directory SYSAP( SCS$DIRECTORY ) allows remote SYSAPs to query the
 * local system-wide configuration database as to the availability of specific
 * SYSAPs for logical SCS connection establishment.  Any SYSAP desiring such
 * information must first establish a connection with the local SCS$DIRECTORY
 * which accepts all such connection requests.  The SYSAP may then request
 * information about specific SYSAPs.  SCS$DIRECTORY responds to each request
 * by indicating whether or not the specified local SYSAP has made itself
 * available for receiving connection requests.
 *
 * Communication across all connections with SCS$DIRECTORY obey the SCS
 * Directory SYSAP Protocol.  This protocol defines two types of application
 * sequenced messages: 
 *
 *	1. SCS Directory Requests.
 *	2. SCS Directory Responses.
 *
 * Each query about the availability of a specific SYSAP for connection
 * establishment takes the form of a SCS directory request.  Each response to a
 * query takes the form of a SCS directory response.  SCS$DIRECTORY only
 * receives requests and transmits responses.
 *
 * There are two forms of SCS directory requests.  The first form targets
 * SYSAPs explicitly by name while the second form targets SYSAPs by their
 * directory identification number.  Thus, the former form allows the
 * connection availability of specific SYSAPs to be determined while the latter
 * allows retrieval of the names of every connection available SYSAP on a
 * specific system.
 *
 * The SCS Directory Protocol provides for flow control of requests across a
 * logical SCS connection utilizing the following simple rules:
 *
 * 	1. Each SYSAP limits the maximum number of concurrent requests it
 *	   makes.  This number is passed to SCS$DIRECTORY as the SYSAP's
 *	   minimum send credit requirement within the connection request.
 *
 *	2. SYSAPs must always specify conversion of their message buffers into
 *	   receive message buffers following transmission of their requests.
 *	   These converted buffers are used to receive the responses to their
 *	   requests.
 *
 *	3. SCS$DIRECTORY guarantees to provide a sufficient number of receive
 *	   message buffers on the connection.  The number initially provided
 *	   is sufficient to handle the case when its counterpart is
 *	   transmitting its maximum number of concurrent requests.
 *
 *	4. SCS$DIRECTORY must always specify conversion of their message
 *	   buffers into receive message buffers following transmission of their
 *	   responses.  This rules serves to maintain the number of receive
 *	   message buffers associated with the connection thereby maximizing
 *	   throughput across it.
 *
 * These simple rules together with the SCS flow control mechanisms guarantee
 * a smooth transfer of information between SCS$DIRECTORY and its counterparts.
 * The only potential problem is if the SYSAP violates rule 2.  SCS$DIRECTORY
 * is capable of detecting this condition and immediately terminates the
 * offending logical SCS connection when it occurs.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#include		"../h/errlog.h"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"../io/ci/cippdsysap.h"
#include		"../io/ci/cisysap.h"
#include		"../io/bi/bvpsysap.h"
#include		"../io/gvp/gvpsysap.h"
#include		"../io/msi/msisysap.h"
#include		"../io/uba/uqsysap.h"
#include		"../io/sysap/sysap.h"
#include		"../io/sysap/scs_directory.h"

/* External Variables and Routines.
 */
extern	SCSIB		lscs;
extern	u_long		scs_cushion;
extern	void		scs$dir_receive();

/*   Name:	scs$dir_control	- Process SCS$DIRECTORY Connection Events
 *
 *   Abstract:	This routine processes all events which occur asynchronously
 *		on logical SCS connections established by SCS$DIRECTORY.  It
 *		also processes those events which can asynchronously occur
 *		on SCS$DIRECTORY's listening SCS connection.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Services Block
 *   event			- Event type
 *   lscs			- Local system permanent information
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cmsb			- Connection Management Service Block pointer
 *				   INITIALIZED( depending upon actions taken )
 */

void
scs$dir_control( event, cmsb )
    u_long		event;
    register CMSB	*cmsb;
{
    register u_long	status;

    /* The events which can occur, and the actions which are taken, are as
     * follows:
     *
     *	Event			Action
     *
     *	CRE_CONN_REC		Accept the connection request.
     *
     *	CRE_DISCONN_REC		Disconnect the connection.  No clean up is
     *	CRE_PATH_FAILURE	required before disconnecting.
     *
     *	CRE_ACCEPT_DONE		No action is required.
     *	CRE_REJECT_DONE
     *	CRE_DISCONN_DONE
     *	CRE_NEW_PATH
     *
     *	CRE_CONN_DONE		PANIC - Such events should never occur.
     *	CRE_BLOCK_DONE
     *	CRE_CREDIT_AVAIL
     *  CRE_MAP_AVAIL
     *
     * The only acceptable failure on any SCS call made by this routine is an
     * allocation error on an acceptance of a connection request.  In such
     * cases the connection request is just rejected.  All other failures
     * indicate the existence of some serious problem and a panic is issued.
     */
    switch( event ) {
	case CRE_CONN_REC:
	    cmsb->aux = NULL;
	    cmsb->control = scs$dir_control;
	    ( void )bcopy( DIRDATA, cmsb->conn_data, DATA_SIZE );
	    cmsb->dg_event = NULL;
	    cmsb->min_snd_credit = 0;
	    cmsb->msg_event = scs$dir_receive;
	    cmsb->init_dg_credit = 0;

	    /* The number of send credits initially provided is equivalent to
	     * either the remote SYSAP's minimum send credit requirement plus
	     * the current SCS send credit cushion or 1, whichever is larger.
	     */
	    cmsb->init_rec_credit = cmsb->min_snd_credit + lscs.cushion;
	    if( cmsb->init_rec_credit == 0 ) {
		++cmsb->init_rec_credit;
	    }
	    if(( status = scs_accept( cmsb )) != RET_SUCCESS ) {
		if( status == RET_ALLOCFAIL ) {
		    cmsb->Reason = ADR_NORESOURCE;
		    if( scs_reject( cmsb ) != RET_SUCCESS )
			( void )panic( DIRPANIC_REJECT );
		} else {
		    ( void )panic( DIRPANIC_ACCEPT );
		}
	    }
	    break;

	case CRE_DISCONN_REC:
	case CRE_PATH_FAILURE:
	    if( cmsb->Reason != ADR_PATH_FAILURE ) {
		cmsb->Reason = ADR_SUCCESS;
	    }
	    if( scs_disconnect( cmsb ) != RET_SUCCESS ) {
		( void )panic( DIRPANIC_DISCON );
	    }
	    break;

	case CRE_ACCEPT_DONE:
	case CRE_REJECT_DONE:
	case CRE_DISCONN_DONE:
	case CRE_NEW_PATH:
	    break;

	case CRE_CONN_DONE:
	case CRE_BLOCK_DONE:
	case CRE_CREDIT_AVAIL:
	case CRE_MAP_AVAIL:
	default:
	    ( void )panic( DIRPANIC_EVENT );
    }
}

/*   Name:	scs$dir_init	- Initialize SCS$DIRECTORY
 *
 *   Abstract:	This routine initializes SCS$DIRECTORY after which the 
 *		Directory SYSAP is receptive to receiving connection requests.
 *		It must only be called once during system initialization.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 */

void
scs$dir_init()
{
    CMSB	cmsb;

    /* Passively listen for connection requests.  Ignore all failures
     * ( SCS$DIRECTORY just remains disabled and unable to receive connection
     * requests ).
     */
    cmsb.control = scs$dir_control;
    ( void )bcopy( DIRNAME, cmsb.lproc_name, NAME_SIZE );
    ( void )bcopy( DIRDATA, cmsb.conn_data, DATA_SIZE );
    ( void )scs_listen( &cmsb );
}

/*   Name:	scs$dir_receive	- Receive and Process Directory Requests
 *
 *   Abstract:	This routine receives and processes all SCS$DIRECTORY directory
 *		lookup requests, the only request type currently supported by
 *		the SCS$DIRECTORY protocol.  Directory lookup requests may
 *		target listening SYSAPs in one of two fashions:
 *
 *		1. By their name.
 *		2. By their directory identification numbers.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	buf			-  Address of directory request
 *   scs_listeners		- Listening SYSAP queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   csb			- Communication Services Block pointer
 *	buf			-  Address of directory response
 */

void
scs$dir_receive( csb )
    register CSB		*csb;
{
    ISB				dir_isb;
    CIB				dir_cib;
    register CIB		*cib = &dir_cib;
    register ISB		*isb = &dir_isb;
    register SCS_DIR_REQ	*req = ( SCS_DIR_REQ * )csb->buf;
    register u_long		status = ADR_NOLISTENER;

    /* Processing a lookup request consists of:
     *
     * 1. Iteratively searching the pool of listening SCS connections for the
     *	  target SYSAP.
     * 2. Formulating an appropriate response to the request.
     * 3. Transmitting the response.
     *
     * Responses are transmitted so as to return the message buffer to the
     * appropriate local port's message free pool from whence it came.
     *
     * The transmission of a response should never fail except through lack of
     * send credits.  This situation can only exist when the remote SYSAP has
     * failed to obey the SCS$DIRECTORY protocol.  The connection is terminated
     * locally whenever this occurs.  Any other types of failures on response
     * transmission or failures to disconnect the connection on exhaustion of
     * send credits indicate the existence of some serious problem and a panic
     * is immediately issued.
     */
    Zero_connid( isb->next_connid )
    for( ;; ) {
	if( scs_info_listen( isb, cib ) == RET_SUCCESS ) {
	    if(( req->form == BY_ENTRY && req->entry <= cib->Dirid ) ||
		 bcmp( req->proc_name, cib->lproc_name, NAME_SIZE ) == 0 ) {
		status = ADR_SUCCESS;
		if( req->form == BY_ENTRY ) {
		    ( void )bcopy( cib->lproc_name, Rsp->proc_name, NAME_SIZE);
		}
		Rsp->entry = cib->Dirid;
		( void )bcopy( cib->lconn_data, Rsp->proc_info, DATA_SIZE );
		break;
	    } else if( Test_connid( isb->next_connid )) {
		break;
	    }
	} else {
	    Zero_connid( isb->next_connid )
	}
    }
    Rsp->status = status;
    csb->Disposal = RECEIVE_BUF;
    csb->size = sizeof( SCS_DIR_RSP );
    if(( status = scs_send_msg( csb )) != RET_SUCCESS ) {
	if( status == RET_NOCREDITS ) {
	    CMSB	dir_cmsb;

	    ( void )scs_dealloc_msg( csb );
	    dir_cmsb.connid = csb->connid;
	    dir_cmsb.Reason = ADR_NOCREDIT;
	    if( scs_disconnect( &dir_cmsb ) != RET_SUCCESS )
		( void )panic( DIRPANIC_DISCON );
	} else {
	    ( void )panic( DIRPANIC_RSP );
	}
    }
}
