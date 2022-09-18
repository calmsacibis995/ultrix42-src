#ifndef	lint
static char *sccsid = "@(#)ci_block.c	4.1	(ULTRIX)	7/2/90";
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
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		block data communication service functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 6, 1985
 *
 *   Function/Routines:
 *
 *   ci_req_data		Request Block Data
 *   ci_send_data		Send Block Data
 *
 *   Modification History:
 *
 *   19-Sep-1989	Pete Keilty
 *	Added pccb to macro Format_gvph & Pd_to_ppd.
 *
 *   17-Jan-1989	Todd M. Katz		TMK0003
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Include header file ../vaxmsi/msisysap.h.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0002
 *	Create a single unified hierarchical set of naming conventions for use
 *	within the CI port driver and describe them within ciport.h.  Apply
 *	these conventions to all names( routine, macro, constant, and data
 *	structure ) used within the driver.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, restructured code paths, and added SMP support.
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
#include		"../io/ci/cippd.h"
#include		"../io/ci/ciport.h"

/* External Variables and Routines.
 */
extern	u_long		gvp_queue_retry;
extern	SCSH		*gvp_alloc_msg();

/*   Name:	ci_req_data	- Request Block Data
 *
 *   Abstract:	This function initiates transfer of block data from a remote
 *		buffer into a local buffer over a specific path.  The transfer
 *		is initiated by placing a REQDAT1 command packet onto the
 *		second lowest priority port command queue and notifying the
 *		port when the queue was previously empty.
 *
 *		A CI message buffer is temporarily allocated by this function
 *		and used to contain the command.  The buffer is placed onto the
 *		message free queue following command execution for use in
 *		containing confirmation of the transfer.  It is deallocated
 *		following transfer confirmation.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   lbh			- Local buffer handle pointer
 *   lboff			- Offset into local buffer
 *   rbh			- Remote buffer handle pointer
 *   rboff			- Offset into remote buffer
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *   gvp_queue_retry		- Queuing failure retry count
 *   size			- Size of transfer
 *   tid			- Transaction identifier pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Transfer successfully initiated
 *   RET_ALLOCFAIL		- Failed to allocate message buffer
 *
 *   SMP:	No locks are required; however, the PB must EXTERNALLY be
 *		prevented from deletion.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		Access to port command queues is by means of memory
 *		interlocking queuing instructions.
 *
 *		Locks lower than the PCCB in the SCA locking hierarchy may NOT
 *		be held EXTERNALLY without also holding the PCCB lock in case
 *		the port requires crashing.
 */
u_long
ci_req_data( pccb, pb, size, tid, lbh, lboff, rbh, rboff )
    register PCCB	*pccb;
    PB			*pb;
    u_long		size;
    TID			*tid;
    BHANDLE		*lbh;
    u_long		lboff;
    BHANDLE		*rbh;
    u_long		rboff;
{
    register GVPH	*cibp;
    register SCSH	*scsbp;
    register REQDATH	*bp;

    /* The steps involved in requesting block data are:
     *
     * 1. Allocate a CI message buffer to contain the REQDAT command.
     * 2. Format the REQDAT specific portion of the command packet.
     * 3. Format the generic Vaxport header.
     * 4. Initiate the block data transfer.
     *
     * The port is crashed if transfer can not be successfully initiated.
     */
    if(( scsbp = gvp_alloc_msg( pccb ))) {
	cibp = Scs_to_pd( scsbp, pccb );
	bp = ( REQDATH * )Pd_to_ppd( cibp, pccb );
	bp->xctid = *tid;
	bp->length = size;
	bp->sbname = rbh->Bname;
	bp->sboff = rbh->Boff + rboff;
	bp->rbname = lbh->Bname;
	bp->rboff = lbh->Boff + lboff;
	Format_gvph( pccb,
		     cibp,
		     REQDAT1,
		     Scaaddr_low( pb->pinfo.rport_addr ),
		     RECEIVE_BUF )
	Insqti_block( cibp, pccb )
	return( RET_SUCCESS );
    } else {
	return( RET_ALLOCFAIL );
    }
}

/*   Name:	ci_send_data	- Send Block Data
 *
 *   Abstract:	This function initiates transfer of block data from a local
 *		buffer into a remote buffer over a specific path.  The transfer
 *		is initiated by placing a SNDDAT command packet onto the second
 *		lowest priority port command queue and notifying the port when
 *		the queue was previously empty.
 *
 *		A CI message buffer is temporarily allocated by this function
 *		and used to contain the command.  The buffer is placed onto the
 *		message free queue following command execution for use in
 *		containing confirmation of the transfer.  It is deallocated
 *		following transfer confirmation.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   lbh			- Local buffer handle pointer
 *   lboff			- Offset into local buffer
 *   rbh			- Remote buffer handle pointer
 *   rboff			- Offset into remote buffer
 *   pb				- Path Block pointer
 *   pccb			- Port Command and Control Block pointer
 *   gvp_queue_retry		- Queuing failure retry count
 *   size			- Size of transfer
 *   tid			- Transaction identifier pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Transfer successfully initiated
 *   RET_ALLOCFAIL		- Failed to allocate message buffer
 *
 *   SMP:	No locks are required; however, the PB must EXTERNALLY be
 *		prevented from deletion.  PCCB addresses are always valid
 *		allowing access to static fields because these data structures
 *		are never deleted once their corresponding ports have been
 *		initialized.
 *
 *		Access to port command queues is by means of memory
 *		interlocking queuing instructions.
 *
 *              Locks lower than the PCCB in the SCA locking hierarchy may NOT
 *              be held EXTERNALLY without also holding the PCCB lock in case
 *              the port requires crashing.
 */
u_long
ci_send_data( pccb, pb, size, tid, lbh, lboff, rbh, rboff )
    register PCCB	*pccb;
    PB			*pb;
    u_long		size;
    TID			*tid;
    BHANDLE		*lbh;
    u_long		lboff;
    BHANDLE		*rbh;
    u_long		rboff;
{
    register GVPH	*cibp;
    register SCSH	*scsbp;
    register SNDDATH	*bp;

    /* The steps involved in sending block data are as follows:
     *
     * 1. Allocate a CI message buffer to contain the SNDDAT command.
     * 2. Format the SNDDAT specific portion of the command packet.
     * 3. Format the generic Vaxport header.
     * 4. Initiate the block data transfer.
     *
     * The port is crashed if transfer can not be successfully initiated.
     */
    if(( scsbp = gvp_alloc_msg( pccb ))) {
	cibp = Scs_to_pd( scsbp, pccb );
	bp = ( SNDDATH * )Pd_to_ppd( cibp, pccb );
	bp->xctid = *tid;
	bp->length = size;
	bp->rbname = rbh->Bname;
	bp->rboff = rbh->Boff + rboff;
	bp->sbname = lbh->Bname;
	bp->sboff = lbh->Boff + lboff;
	Format_gvph( pccb,
		     cibp, 
		     SNDDAT,
		     Scaaddr_low( pb->pinfo.rport_addr ), 
		     RECEIVE_BUF )
	Insqti_block( cibp, pccb )
	return( RET_SUCCESS );
    } else {
	return( RET_ALLOCFAIL );
    }
}
