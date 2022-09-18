#ifndef	lint
static char *sccsid = "@(#)dli_close.c	4.2	ULTRIX	9/4/90";
#endif	lint

/*
 * Program dli_close.c,  Module DLI 
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 */

#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/protosw.h"
#include "../../h/errno.h"
#include "../../h/ioctl.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../h/user.h"
#include "../../h/buf.h"
#include "../../h/conf.h"
#include "../../h/proc.h"

#include "../../net/net/if.h"

#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"
#include "../../net/dli/csmacd_if.h"





/*
 *		d l i _ c l o s e
 *
 * Process a DLI close socket request.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	uentry		= Pointer to the line table entry for this request.
 */
dli_close( uentry )
register struct dli_line *uentry;
{
	int saveaffinity;  /* for nonsymm drivers.  8.18.88.us  */
	int i, close_port = 1;  /* assume close port is to be issued */
	extern struct dli_line dli_ltable[];

	/*
	 * unlock socket.
	 * if no longer needed
	 *     close data link port first.
	 * lock socket.
	 */
	uentry->dli_so->ref = 80;
	smp_unlock(&uentry->dli_so->lk_socket);
	for ( i = 0; i < dli_maxline; i++ )
	{
		if ( &dli_ltable[i] == uentry )
			continue;
		smp_lock(&dli_ltable[i].dli_lk, LK_RETRY);
		if ( dli_ltable[i].dli_portid == uentry->dli_portid )
		{
			smp_unlock(&dli_ltable[i].dli_lk);
			close_port = 0;
			break;
		}
		smp_unlock(&dli_ltable[i].dli_lk);
		
	}
	smp_lock(&uentry->dli_so->lk_socket, LK_RETRY);
	uentry->dli_so->ref = 0;
	if ( uentry->dli_proto && close_port )
		(*uentry->dli_proto->pr_ctloutput)(PRCO_PIF, NULL, 0, CSMACD_CLOSEPORT,
			uentry->dli_portid);
	uentry->dli_proto = NULL;

	switch ( uentry->dli_lineid.dli_substructype )
	{

		case DLI_802:
			/*
			 * disable the individual sap in the system tables
			 * this will also disable any group saps that this
			 * isap has enabled 
			 * also disable any multicast addresses this isap may have
			 */
			osi_dis_802isap(uentry->dli_if, uentry->dli_lineid.choose_addr.dli_802addr.eh_802.ssap, uentry->dli_so);

		case DLI_ETHERNET:
			/*
			 * disable possible multicast addresses
			 */
			mcast_cmd(uentry->dli_sockopt.dli_mcast, SIOCDELMULTI, uentry->dli_if, uentry->dli_proto, uentry->dli_portid);
			break;

		case DLI_POINTOPOINT:
			/*
			 * relinquish ownership of device
			 */
			uentry->dli_sockopt.dli_state = DLS_OFF;
			uentry->dli_lineid.choose_addr.dli_paddr.dev_cstate.if_ustate = IFS_USROFF;
			uentry->dli_lineid.choose_addr.dli_paddr.dev_pstate.if_next_family = uentry->dli_lineid.choose_addr.dli_paddr.dev_pstate.if_family;
			uentry->dli_lineid.choose_addr.dli_paddr.dev_pstate.if_family = AF_DLI;
			uentry->dli_lineid.choose_addr.dli_paddr.dev_pstate.if_wrstate = IFS_WRSTATE;
			uentry->dli_lineid.choose_addr.dli_paddr.dev_pstate.if_xferctl = IFS_XFERCTL;
			CALL_TO_NONSMP_DRIVER( (*uentry->dli_if), saveaffinity);
			(*uentry->dli_if->if_ioctl)(uentry->dli_if, SIOCSTATE, &uentry->dli_lineid.choose_addr.dli_paddr.dev_pstate);
			RETURN_FROM_NONSMP_DRIVER( (*uentry->dli_if), saveaffinity);
			break;

		default:
			panic("dli_close");
			break;
	}

	/*
	 * clear out entry in table
	 */
	bzero((u_char *) uentry, 
		((u_char *) &uentry->dli_lk - (u_char *) uentry));
	return(NULL);
}
