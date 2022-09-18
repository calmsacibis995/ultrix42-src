#ifndef	lint
static char *sccsid = "@(#)dli_output.c	4.2	ULTRIX	9/4/90";
#endif	lint

/*
 * Program dli_output.c,  Module DLI 
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
 * 2.00 18-Apr-1986
 *		DECnet-Ultrix	V2.0
 *
 * Added sysid and point-to-point support
 *
 */

#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/protosw.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/errno.h"
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
 * DLI output subroutine.
 *
 *
 *
 *
 * Attempt to transmit all pending messages for the specified virtual circuit.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	user_line	= Pointer to the user's line descriptor.
 *	m		= Pointer to mbuf chain to be transmitted.
 *	dst_addr	= Pointer to structure containing target address.
 */
dli_output( user_line, m, dst_addr )
register struct dli_line *user_line;
struct mbuf *m;
struct sockaddr_dl *dst_addr;
{
	register struct ifnet *ifp;
	register struct sockaddr_dl *out_addr;
	struct ifnet *match_device();
	register struct mbuf *m0 = NULL;
	struct mbuf *osi_buildhdr();
	int len;
	int error;
	int saveaffinity;  /* for nonsymm drivers.  8.18.88.us  */
	u_int port;

	if ( dst_addr )
	{
		if ( dst_addr->dli_family != AF_DLI )
		{
			m_freem(m);
			return(EAFNOSUPPORT);
		}

		switch (dst_addr->dli_substructype)
		{
			case DLI_ETHERNET:
			case DLI_POINTOPOINT:
				m0 = m;
			case DLI_802:
				ifp = match_device(&dst_addr->dli_device);
				break;

			default:
				m_freem(m);
				return(EOPNOTSUPP);
				break;
		}

		out_addr = dst_addr;
	}
	else
	{
		if ( (out_addr = &user_line->dli_lineid )->dli_substructype != DLI_802 )
			m0 = m;
		ifp = user_line->dli_if;
	}

	if(ifp == NULL)
	{
		m_freem(m);
		return(ENODEV);
	}

	if ( ! (ifp->if_flags & IFF_RUNNING) || ! user_line->dli_sockopt.dli_state )
	{
		m_freem(m);
		return(ENETDOWN);
	}

	/*
	 * for 802 packets, validate user data length
	 * and then build the 802 header for transmission
	 */
	len = m_length(m);
	if(m0 == NULL)
	{
		if(user_line->dli_lineid.choose_addr.dli_802addr.eh_802.ssap == SNAP_SAP && len > ifp->if_mtu - MAX802DATAP)
		{
			m_freem(m);
			return(EMSGSIZE);
		}
		else if (len > ifp->if_mtu - MAX802DATANP)
		{
			m_freem(m);
			return(EMSGSIZE);
		}
		if( (m0 = osi_buildhdr(out_addr)) == NULL)
		{
			m_freem(m);
			return(ENOBUFS);
		}
		m0->m_next = m;
	}
	else
	{
#ifdef IFT_FDDI
		if ( ifp->if_flags & IFF_802HDR )
		{
			if ( len > ETHERMTU - MAX802DATAP)
			{
				m_freem(m);
				return(EMSGSIZE);
			}
		}
		else
#endif
		{
			if ( len > ifp->if_mtu )
			{
				m_freem(m);
				return(EMSGSIZE);
			}
		}
	}

	port = user_line->dli_portid;
	len = m_length(m0);
	/*
	 * No need to keep dli line table entry and socket locked
	 * while in driver. 
	 */
	smp_unlock(&user_line->dli_so->lk_socket);
	smp_unlock(&user_line->dli_lk);
	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = ( (*ifp->if_output)(ifp, m0, out_addr) );
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
	if ( !error && port && ( out_addr->dli_substructype == DLI_ETHERNET || 
		out_addr->dli_substructype == DLI_802) )
	{
		struct dli_ifincout ic;
		ic.port = (caddr_t) port;
		ic.octets_sent = len;
		if ( out_addr->dli_substructype == DLI_ETHERNET )
		    ic.mcast = out_addr->choose_addr.dli_eaddr.dli_target[0] & 1;
		else
		    ic.mcast = out_addr->choose_addr.dli_802addr.eh_802.dst[0] & 1;
		(*user_line->dli_proto->pr_ctloutput)(PRCO_PIF, NULL, 0, CSMACD_INCOUT, &ic);
	}
	smp_lock(&user_line->dli_lk, LK_RETRY);
	if ( user_line->dli_so != NULL )
		smp_lock(&user_line->dli_so->lk_socket, LK_RETRY);
	return( error );
}
