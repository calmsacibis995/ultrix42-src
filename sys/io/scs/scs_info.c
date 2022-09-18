#ifndef	lint
static        char    *sccsid = "@(#)scs_info.c	4.1  (ULTRIX)        7/2/90";
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
 *		information service functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 27, 1985
 *
 *   Function/Routines:
 *
 *   scs_info_conn		Return Logical SCS Connection Information
 *   scs_info_listen		Return Listening SCS Connection Information
 *   scs_info_lport		Return Local Port Information
 *   scs_info_path		Return Path Information
 *   scs_info_scs		Return SCS Information
 *   scs_info_system		Return System Information
 *
 *   Modification History:
 *
 *   06-Jun-1990	Pete Keilty
 *	Added check for isb->next_lport_name == 0 in scs_info_lport().
 *
 *   06-Apr-1989	Pete Keilty
 *	Added include file smp_lock.h
 *
 *   05-Mar-1989	Todd M. Katz		TMK0002
 *	Include header file ../vaxmsi/msisysap.h.
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
#include		"../h/errlog.h"
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
extern	SCSIB		lscs;
extern	struct lock_t	lk_scadb;
extern	sbq		scs_config_db;
extern	cbq		scs_listeners;
extern	CBVTDB		*scs_cbvtdb;
extern	pccbq		scs_lport_db;
extern	u_char		scs_sanity;
extern	u_long		scs_cushion;
extern	void		( *gvp_info )();

/*   Name:	scs_info_conn	- Return Logical SCS Connection Information
 *
 *   Abstract:	This function returns information about a logical SCS
 *		connection.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cib			- Connection Information Block pointer
 *   isb			- Information Service Block pointer
 *	lport_name		-  Local port name of path
 *	next_connid		-  Identification number of target connection
 *				   ( 0 requests first connection over path to
 *				     system )
 *	rport_addr		-  Remote port station address of path
 *	sysid			-  Identification number of system
 *   lk_scadb			- SCA database lock structure
 *   scs_config_db		- System-wide configuration database queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cib			- Connection Information Block pointer
 *				   INITIALIZED
 *   isb			- Information Service Block pointer
 *	connid			-  Identification number of found connection
 *	next_connid		-  Identification number of next connection
 *				   ( 0 -> last connection over path to system
 *				     found )
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Target logical SCS connection found
 *   RET_NOCONN			- Target logical SCS connection not found
 *   RET_NOSYSTEM		- Specified system not known locally
 *   RET_NOPATH			- Specified path not found
 *
 *   SMP:	The SCA database is locked to postpone potential modifications
 *		to the system-wide configuration database while it is being
 *		traversed.
 */
u_long
scs_info_conn( isb, cib )
    register ISB	*isb;
    CIB			*cib;
{
    register sbq	*sb;
    register pbq	*pb;
    register cbq	*cb;
    register u_long	status = RET_NOSYSTEM;

    /* The steps involved in returning connection information are as follows:
     *
     * 1. Lock the SCA database.
     * 2. Search the system-wide configuration database for the target
     *	  connection.
     * 3. Retrieve the target connection's information.
     * 4. Unlock the SCA database.
     * 5. Return an appropriate status.
     */
    Lock_scadb()
    for( sb = scs_config_db.flink; sb != &scs_config_db; sb = sb->flink ) {
	if( Comp_scaaddr( isb->sysid, Sb->sinfo.sysid )) {
	    for( pb = Sb->pbs.flink; pb != &Sb->pbs; pb = pb->flink ) {
		if( isb->lport_name == Pb->pinfo.lport_name &&
		     Comp_scaaddr( isb->rport_addr, Pb->pinfo.rport_addr )) {
		    for( cb = Pb->cbs.flink; cb != &Pb->cbs; cb = cb->flink ) {
			if( Test_connid( isb->next_connid ) ||
			     Comp_connid( isb->next_connid,
					  Cb->cinfo.lconnid )) {
			    if( cib ) {
				*cib = Cb->cinfo;
			    }
			    Move_connid( Cb->cinfo.lconnid, isb->connid )
			    if(( cb = cb->flink ) == &Pb->cbs ) {
				Zero_connid( isb->next_connid )
			    } else {
				Move_connid( Cb->cinfo.lconnid,
					     isb->next_connid )
			    }
			    status = RET_SUCCESS;
			    break;
			}
		    }
		    if( status == RET_NOSYSTEM ) {
			status = RET_NOCONN;
		    }
		    break;
		}
	    }
	    if( status == RET_NOSYSTEM ) {
		status = RET_NOPATH;
	    }
	    break;
	}
    }
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_info_listen	- Return Listening SCS Connection Information
 *
 *   Abstract:	This function returns information about a listening SCS
 *		connection.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cib			- Connection Information Block pointer
 *   isb			- Information Service Block pointer
 *	next_connid		-  Identification number of target connection
 *				   ( 0 requests first listening connection )
 *   lk_scadb			- SCA database lock structure
 *   scs_listeners		- Listening SYSAP queue queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   cib			- Connection Information Block pointer
 *				   INITIALIZED
 *   isb			- Information Service Block pointer
 *	connid			-  Identification number of found connection
 *	next_connid		-  Identification number of next connection
 *				   ( 0 -> last listening SCS connection )
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Target listening SCS connection found
 *   RET_NOCONN			- Target listening SCS connection not found
 *
 *   SMP:	The SCA database is locked to postpone potential modifications
 *		to the queue of listening SYSAPs while it is being traversed.
 */
u_long
scs_info_listen( isb, cib )
    register ISB	*isb;
    CIB			*cib;
{
    register cbq	*cb;
    register u_long	status = RET_NOCONN;

    /* The steps involved in returning listening SCS connection information are
     * as follows:
     *
     * 1. Lock the SCA database.
     * 2. Search the queue of listening SYSAPs for the target connection.
     * 3. Retrieve the target connection's information.
     * 4. Unlock the SCA database.
     * 5. Return an appropriate status.
     */
    Lock_scadb()
    for( cb = scs_listeners.flink; cb != &scs_listeners; cb = cb->flink ) {
	if( Test_connid( isb->next_connid ) ||
	     Comp_connid( isb->next_connid, Cb->cinfo.lconnid )) {
	    if( cib ) {
		*cib = Cb->cinfo;
	    }
	    Move_connid( Cb->cinfo.lconnid, isb->connid )
	    if(( cb = cb->flink ) == &scs_listeners ) {
		Zero_connid( isb->next_connid )
	    } else {
		Move_connid( Cb->cinfo.lconnid, isb->next_connid )
	    }
	    status = RET_SUCCESS;
	    break;
	}
    }
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_info_lport	- Return Local Port Information
 *
 *   Abstract:	This function returns information about a local port.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   isb			- Information Service Block pointer
 *	next_lport_name		-  Name of target local port
 *				   ( 0 requests first local port )
 *   lpib			- Local Port Information Block pointer
 *   lk_scadb			- SCA database lock structure
 *   scs_lport_db		- System-wide local port database queue head
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   isb			- Information Service Block pointer
 *	lport_name		-  Name of found local port
 *	next_lport_name		-  Name of next local port
 *				   ( 0 -> last local port )
 *   lpib			- Local Port Information Block pointer
 *				   INITIALIZED
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Target local port found
 *   RET_NOLPORT		- Target local port not found
 *
 *   SMP:	The SCA database is locked to postpone potential modifications
 *		to the system-wide local port database while it is being
 *		traversed.
 */
u_long
scs_info_lport( isb, lpib )
    register ISB	*isb;
    LPIB		*lpib;
{
    register pccbq	*qp;
    register PCCB	*pccb;
    register u_long	status = RET_NOLPORT;

    /* The steps involved in returning local port information are as follows:
     *
     * 1. Lock the SCA database.
     * 2. Search the system-wide local port database for the target local port.
     * 3. Retrieve the target local port's information.
     * 4. Unlock the SCA database.
     * 5. Return an appropriate status.
     */
    Lock_scadb()
    for( qp = scs_lport_db.flink; qp != &scs_lport_db; qp = qp->flink ) {
	pccb = Pos_to_pccb( qp, flink );
	if( isb->next_lport_name == 0 ||
	        pccb->lpinfo.name == isb->next_lport_name ) {
	    if( lpib ) {
		*lpib = pccb->lpinfo;
	    }
	    isb->lport_name = pccb->lpinfo.name;
	    if(( qp = qp->flink ) == &scs_lport_db ) {
		isb->next_lport_name = 0;
	    } else {
		isb->next_lport_name = Pos_to_pccb( qp, flink )->lpinfo.name;
	    }
	    status = RET_SUCCESS;
	    break;
	}
    }
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_info_path	- Return Path Information
 *
 *   Abstract:	This function returns information about a path to a locally
 *		known system.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   isb			- Information Service Block pointer
 *	next_lport_name		-  Local port name of target path
 *				   ( 0 requests first path to system )
 *	next_rport_addr		-  Remote port address of target path
 *	sysid			-  Identification number of system
 *   lk_scadb			- SCA database lock structure
 *   scs_config_db		- System-wide configuration database queue head
 *   pib			- Path Information Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   isb			- Information Service Block pointer
 *	lport_name		-  Local port name of found path
 *	next_lport_name		-  Local port name of next path
 *				   ( 0 -> last path to system found )
 *	next_rport_addr		-  Remote port address of next path
 *	rport_addr		-  Remote port address of found path
 *   pib			- Path Information Block pointer
 *				   INITIALIZED
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Target path found
 *   RET_NOSYSTEM		- Specified system not known locally
 *   RET_NOPATH			- Target path not found
 *
 *   SMP:	The SCA database is locked to postpone potential modifications
 *		to the system-wide configuration database while it is being
 *		traversed.
 */
u_long
scs_info_path( isb, pib )
    register ISB	*isb;
    PIB			*pib;
{
    register sbq	*sb;
    register pbq	*pb;
    register u_long	status = RET_NOSYSTEM;

    /* The steps involved in returning path information are as follows:
     *
     * 1. Lock the SCA database.
     * 2. Search the system-wide configuration database for the target path.
     * 3. Retrieve the target path's information.
     * 4. Unlock the SCA database.
     * 5. Return an appropriate status.
     */
    Lock_scadb()
    for( sb = scs_config_db.flink; sb != &scs_config_db; sb = sb->flink ) {
	if( Comp_scaaddr( isb->sysid, Sb->sinfo.sysid )) {
	    for( pb = Sb->pbs.flink; pb != &Sb->pbs; pb = pb->flink ) {
		if( isb->next_lport_name == 0 ||
		     ( isb->next_lport_name == Pb->pinfo.lport_name &&
		        Comp_scaaddr( isb->next_rport_addr,
				      Pb->pinfo.rport_addr ))) {
		    if( pib ) {
			*pib = Pb->pinfo;
		    }
		    isb->lport_name = Pb->pinfo.lport_name;
		    Move_scaaddr( Pb->pinfo.rport_addr, isb->rport_addr );
		    if(( pb = pb->flink ) == &Sb->pbs ) {
			isb->next_lport_name = 0;
			Zero_scaaddr( isb->next_rport_addr )
		    } else {
			isb->next_lport_name = Pb->pinfo.lport_name;
			Move_scaaddr( Pb->pinfo.rport_addr,
				      isb->next_rport_addr )
		    }
		    status = RET_SUCCESS;
		    break;
		}
	    }
	    if( status == RET_NOSYSTEM ) {
		status = RET_NOPATH;
	    }
	    break;
	}
    }
    Unlock_scadb()
    return( status );
}

/*   Name:	scs_info_scs	- Return SCS Information
 *
 *   Abstract:	This routine returns current SCS information.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   gvp_info			- GVP routine to retrieve GVP information
 *   lscs			- Local system permanent information
 *   lk_scadb			- SCA database lock structure
 *   scs_cbvtdb			- CB vector table database pointer
 *   scs_config_db		- System-wide configuration database queue head
 *   scs_cushion		- SCS send credit cushion
 *   scs_listeners		- Listening SYSAP queue queue head
 *   scs_lport_db		- System-wide local port database queue head
 *   scs_sanity			- SCS sanity timer interval
 *   scsib			- SCS Information Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   scsib			- SCS Information Block pointer
 *				   INITIALIZED
 *
 *   SMP:	The SCA database is locked to postpone potential modifications
 *		while the following databases, queues, and lists are being
 *		traversed:
 *
 *			1. The system-wide configuration database.
 *			2. The system-wide local port database.
 *			3. The queue of listening SYSAPs.
 *			4. The list of free CBVTEs.
 */
void
scs_info_scs( scsib )
    register SCSIB	*scsib;
{
    register sbq	*sb;
    register pbq	*pb;
    register cbq	*cb;
    register CBVTE	*cbvte;
    register pccbq	*pccb;
    register void	( *info )();

    /* The steps involved in returning current SCS information are as follows:
     *
     * 1. Retrieve the local system permanent information.
     * 2. Retrieve the values of selected SCS configuration parameters.
     * 3. Lock the SCA database.
     * 4. Traverse the system-wide configuration database and determine the
     *	  current number of systems, paths, and connections.
     * 5. Traverse the the queue of listening SYSAPs and determine the current
     *    number of listening SCS connections.
     * 6. Determine the current number of free CBVTEs.
     * 7. Traverse the system-wide local port database and determine the
     *	  current number of local ports.
     * 8. Unlock the SCA database.
     * 5. Invoke the GVP specific routine to obtain GVP related information(
     *	  provided this routine is provided ).
     */
    if( scsib == NULL ) {
	return;
    }
    *scsib = lscs;
    scsib->cushion = scs_cushion;
    scsib->sanity = scs_sanity;
    Lock_scadb()
    for( sb = scs_config_db.flink,
	 scsib->nconns = 0, scsib->npaths = 0, scsib->nsystems = 0;
	 sb != &scs_config_db;
	 sb = sb->flink, ++scsib->nsystems ) {
	for( pb = Sb->pbs.flink;
	     pb != &Sb->pbs;
	     pb = pb->flink, ++scsib->npaths ) {
	    for( cb = Pb->cbs.flink;
		 cb != &Pb->cbs;
		 cb = cb->flink, ++scsib->nconns ) {}
	}
    }
    for( cb = scs_listeners.flink, scsib->nlisteners = 0;
	 cb != &scs_listeners;
	 cb = cb->flink, ++scsib->nlisteners ) {}
    for( cbvte = scs_cbvtdb->free_cbvte, scsib->free_cbvtes = 0;
	 cbvte;
	 cbvte = cbvte->ov1.cbvte ) {
	++scsib->free_cbvtes;
    }
    for( pccb = scs_lport_db.flink, scsib->nlports = 0;
	 pccb != &scs_lport_db;
	 pccb = pccb->flink, ++scsib->nlports ) {}
    Unlock_scadb()
    if(( info = gvp_info )) {
	( void )( *gvp_info )( scsib );
    }
}

/*   Name:	scs_info_system	- Return System Information
 *
 *   Abstract:	This function returns information about a locally known system.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   isb			- Information Service Block pointer
 *	next_sysid		-  Identification number of target system
 *				   ( 0 requests first system in database )
 *   lk_scadb			- SCA database lock structure
 *   scs_config_db		- System-wide configuration database queue head
 *   sib			- System Information Block pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   isb			- Information Service Block pointer
 *	next_sysid		-  Identification number of next system
 *				   ( 0 -> last system found )
 *	sysid			- Identification number of found system
 *   sib			- System Information Block pointer
 *				   INITIALIZED
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Target system found
 *   RET_NOSYSTEM		- Target system not known locally
 *
 *   SMP:	The SCA database is locked to postpone potential modifications
 *		to the system-wide configuration database while it is being
 *		traversed.
 */
u_long
scs_info_system( isb, sib )
    register ISB	*isb;
    SIB			*sib;
{
    register sbq	*sb;
    register u_long	status = RET_NOSYSTEM;

    /* The steps involved in returning system information are as follows:
     *
     * 1. Lock the SCA database.
     * 2. Search the system-wide configuration database for the target system.
     * 3. Retrieve the target system's information.
     * 4. Unlock the SCA database.
     * 5. Return an appropriate status.
     */
    Lock_scadb()
    for( sb = scs_config_db.flink; sb != &scs_config_db; sb = sb->flink ) {
	if( Test_scaaddr( isb->next_sysid ) ||
	     Comp_scaaddr( isb->next_sysid, Sb->sinfo.sysid )) {
	    if( sib ) {
		*sib = Sb->sinfo;
	    }
	    Move_scaaddr( Sb->sinfo.sysid, isb->sysid );
	    if(( sb = sb->flink ) == &scs_config_db ) {
		Zero_scaaddr( isb->next_sysid )
	    } else {
		Move_scaaddr( Sb->sinfo.sysid, isb->next_sysid )
	    }
	    status = RET_SUCCESS;
	    break;
	}
    }
    Unlock_scadb()
    return( status );
}
