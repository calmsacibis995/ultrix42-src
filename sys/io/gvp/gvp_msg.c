#ifndef	lint
static char *sccsid = "@(#)gvp_msg.c	4.1	(ULTRIX)	7/2/90";
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
 *		Generic Vaxport Port Driver
 *
 *   Abstract:	This module contains Generic Vaxport Port Driver( GVP )
 *		message communication service functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 6, 1985
 *
 *   Function/Routines:
 *
 *   gvp_alloc_msg		Allocate Message Buffer
 *   gvp_dealloc_msg		Deallocate Message Buffer
 *   gvp_add_msg		Add Message Buffer to Free Queue
 *   gvp_remove_msg		Remove Message Buffer from Free Queue
 *   gvp_send_msg		Send Message
 *
 *   Modification History:
 *
 *
 *   15-Jun-1990	Pete Keilty
 * 	Added include files smp_lock.h and ciport.h for CIXCD XMOV bug(temp).
 *
 *   19-Sep-1989	Pete Keilty
 *	Added pccb to macro Format_gvph & Ppd_to_pd.
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's double mapped buffer & pte's
 *	into a Vaxmap of the system.
 *	CI/BVP ports need to have VAX pte's and system addresses.	
 *
 *   17-Jan-1989	Todd M. Katz		TMK0003
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Include header file ../vaxmsi/msisysap.h.
 *
 *   19-Aug-1988	Todd M. Katz		TMK0002
 *	Cast all control blocks to ( char * ) before deallocating.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made GVP
 *	completely independent from underlying port drivers, restructured code
 *	paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#include		"../h/time.h"
#include		"../h/kmalloc.h"
#include		"../h/errlog.h"
#include		"../h/dyntypes.h"
#include		"../h/ksched.h"
#include		"../h/smp_lock.h"
#include		"../machine/cpu.h"
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
#include		"../io/gvp/gvp.h"
#include		"../io/ci/ciport.h"
#include		"../io/scs/scamachmac.h"

/* External Variables and Routines.
 */
extern	u_long		gvp_queue_retry;
extern  struct pte 	Sysmap[];

/*   Name	gvp_alloc_msg	- Allocate Message Buffer
 *
 *   Abstract:	This function allocates a port specific message buffer from
 *		dynamic kernel memory.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   Address of SCS header in message buffer on success
 *   Otherwise NULL
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 */
SCSH *
gvp_alloc_msg( pccb )
    register PCCB	*pccb;
{
    register GVPH	*gvpbp;

    KM_ALLOC( gvpbp, GVPH *, pccb->lpinfo.Msg_size, KM_SCABUF, 
	      KM_NOWAIT|KM_WIRED )
    if( gvpbp ) {
	U_long( gvpbp->size ) = ( u_long )(( DYN_GVPMSG << 16 ) |
					    pccb->lpinfo.Msg_size );
	Dm_msg_dg( gvpbp, pccb->lpinfo.Msg_size )
	return( Pd_to_scs( gvpbp, pccb ));
    } else {
	return( NULL );
    }
}

/*   Name:	gvp_dealloc_msg	- Deallocate Message Buffer
 *
 *   Abstract:	This function deallocates a port specific message buffer to
 *		dynamic kernel memory.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *   scsbp			- Address of SCS header in message buffer
 *
 *   Outputs:	
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 */
void
gvp_dealloc_msg( pccb, scsbp )
    PCCB		*pccb;
    SCSH		*scsbp;
{
    register GVPH	*gvpbp = Scs_to_pd( scsbp, pccb );

    KM_FREE(( char * )gvpbp, KM_SCABUF )
}

/*   Name:	gvp_add_msg	- Add Message Buffer to Free Queue
 *
 *   Abstract:	This function adds a port specific message buffer to a
 *		specific port's message free queue and notifies the port when
 *		the queue was previously empty.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   gvp_queue_retry		- Queuing failure retry count
 *   pccb			- Port Command and Control Block pointer
 *   scsbp			- Address of SCS header in message buffer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		Access to port message free queues is by means of memory
 *		interlocking queuing instructions.
 *
 *		Locks lower than the PCCB in the SCA locking hierarchy may NOT
 *		be held EXTERNALLY without also holding the PCCB lock in case
 *		the port requires crashing.
 */
void
gvp_add_msg( pccb, scsbp )
    register PCCB	*pccb;
    SCSH		*scsbp;
{
    register GVPH	*gvpbp = Scs_to_pd( scsbp, pccb );

    /* Specifically mark the message buffer as a free message.  This allows
     * differentiation between cached commands and free message buffers when
     * buffer addresses are written out by the port to its message logout area
     * during port failure.
     */
    gvpbp->opt = GVPH_FREE;
    Insqti_mfreeq( gvpbp, pccb )
}

/*   Name:	gvp_remove_msg	- Remove Message Buffer from Free Queue
 *
 *   Abstract:	This function removes a port specific message buffer from a
 *		specific port's message free queue.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   gvp_queue_retry		- Queuing failure retry count
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   Address of SCS header of removed message buffer if successful
 *   Otherwise NULL
 *
 *   SMP:	No locks are required.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		Access to port message free queues is by means of memory
 *		interlocking queuing instructions.
 *
 *		Locks lower than the PCCB in the SCA locking hierarchy may NOT
 *		be held EXTERNALLY without also holding the PCCB lock in case
 *		the port requires crashing.
 */
SCSH *
gvp_remove_msg( pccb )
    register PCCB	*pccb;
{
    register GVPH	*gvpbp;

    Remqhi_mfreeq( pccb, gvpbp )
    if( gvpbp ) {
	gvpbp->opt = 0;
	return( Pd_to_scs( gvpbp, pccb ));
    }
    else {
	return( NULL );
    }
}

/*   Name:	gvp_send_msg	- Send Message
 *
 *   Abstract:	This function initiates transmission of a port specific message
 *		over a specific path.  Transmission is initiated by placing a
 *		SNDMSG command packet onto the second highest priority port
 *		command queue and notifying the port when the queue was
 *		previously empty.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *		Two options exist for disposal of the buffer following
 *		transmission of the message:
 *
 *		1. Add the buffer to the port's message free queue.
 *		2. Return the buffer to SCS for deallocation.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   disposal			- DEALLOC_BUF or RECEIVE_BUF
 *   gvp_queue_retry		- Queuing failure retry count
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *   scsbp			- Address of SCS header in message buffer
 *   size			- Size of application data
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   SMP:	No locks are required; however, the PB must EXTERNALLY be
 *		prevented from deletion.  PCCB addresses are always valid
 *		because these data structures are never deleted once their
 *		corresponding ports have been initialized.
 *
 *		Access to port command queues is by means of memory
 *		interlocking queuing instructions.
 *
 *		Locks lower than the PCCB in the SCA locking hierarchy may NOT
 *		be held EXTERNALLY without also holding the PCCB lock in case
 *		the port requires crashing.
 */
void
gvp_send_msg( pccb, pb, scsbp, size, disposal )
    register PCCB	*pccb;
    PB			*pb;
    SCSH		*scsbp;
    u_long		size;
    u_long		disposal;
{
    register GVPH	*gvpbp;
    register GVPPPDH	*gvpppdbp;

    /* Both the generic port-to-port and Vaxport driver header must be
     * initialized before initiating message transmission.
     */
    gvpppdbp = Scs_to_ppd( scsbp );
    Format_gvpppdh( gvpppdbp, SCSMSG, size )
    gvpbp = Ppd_to_pd( gvpppdbp, pccb );
    Format_gvph( pccb, gvpbp, SNDMSG, Scaaddr_low( pb->pinfo.rport_addr ),
		 disposal )
    Insqti_communication( gvpbp, pccb )
}
