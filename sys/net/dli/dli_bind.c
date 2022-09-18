#ifndef	lint
static char *sccsid = "@(#)dli_bind.c	4.2	ULTRIX	9/4/90";
#endif	lint

/*
 * Program dli_bind.c,  Module DLI 
 *
 * Copyright (C) 1985, 1988 by
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
 *      DECnet-ULTRIX   V2.0
 *		- added sysid and point to point support
 *
 * 2.01 18-Mar-1988
 *      DECnet-ULTRIX   V2.4
 *		- Allowed use of reserved bit in individual and group SAPs
 *
 *
 */

#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/protosw.h"
#include "../../h/errno.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../net/net/if.h"
#include "../../net/net/if_to_proto.h"

#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"
#include "../../net/dli/csmacd_if.h"




/*
 *		d l i _ b i n d
 *
 * Process a DLI bind request by binding to a particular device and
 * optionally to an address.
 *
 * Returns:		Error code if error occurs, otherwise NULL.
 *
 * Inputs:
 *	uentry		= Pointer to the user's line table entry for this request.
 *	so_addr		= Pointer to structure containing address.
 */
dli_bind( uentry, so_addr )
register struct dli_line *uentry;
register struct sockaddr_dl *so_addr;
{
	extern struct dli_line dli_ltable[];
	register int error = NULL;
	register struct ifnet *ifp;
	struct ifnet *match_device();
	struct sockaddr_802 *so_802;
	u_char sap, dmy_pi[5];
	int s;

	if(so_addr->dli_family != AF_DLI)
	{
		return(EINVAL);
	}

	if ( uentry->dli_if )
	{
		return(EADDRINUSE);
	}

	if( ! (ifp = match_device(&so_addr->dli_device)))
	{
		return(ENODEV);
	}

	switch (so_addr->dli_substructype)
	{
		case DLI_802:
			so_802 = &so_addr->choose_addr.dli_802addr;

			/*
			 *  dest addr must be physical
			 */
			if( so_802->eh_802.dst[0] & 1 )
			{
					return(EINVAL);
			}

			/* 
			 * source sap validation, set bit 1 for user 
			 * also, can't have a pi if ssap != SNAP SAP
			 */
			bzero(dmy_pi, 5);
			sap = so_802->eh_802.ssap;
			if( (sap != SNAP_SAP ) && 
			( (bcmp(so_802->eh_802.osi_pi, dmy_pi, 5) != 0) || 
			(sap & VSAP) || sap == 0) ) {
				return(EINVAL);
			}

			/* 
			 * class 1 users only allowed these types of ctl formats
			 */
			if(so_802->svc == TYPE1)
			{
				switch(so_802->eh_802.ctl.U_fmt)
				{
					case UI_NPCMD:
					case TEST_PCMD:
					case TEST_NPCMD:
					case XID_PCMD:
					case XID_NPCMD:
						break;
					default:
						return(EINVAL);
				}
			}

			/*
			 * for SNAP SAP, validate service (class 1 this release)
			 * and enable protocol
			 * this is the same as enabling an ethernet protocol,
			 * except the protocol field is 5 bytes
			 */
			if(sap == SNAP_SAP)
			{
				if((so_802->svc != TYPE1) || (so_802->eh_802.ctl.U_fmt != UI_NPCMD))
				{
					return(EINVAL);
				}
				smp_unlock(&uentry->dli_so->lk_socket);
				if(error = osi_ena_802pi(so_802, ifp, uentry))
				{
					smp_lock(&uentry->dli_so->lk_socket, LK_RETRY);
					return(error);
				}
				smp_lock(&uentry->dli_so->lk_socket, LK_RETRY);
			}
			/* validate & enable other individual saps */
			else if(error = osi_ena_802isap(ifp, so_802, uentry->dli_so))
			{
				return(error);
			}

			uentry->dli_sockopt.dli_state = DLS_ON;
			bcopy((u_char *)so_802, (u_char *)&uentry->dli_lineid.choose_addr.dli_802addr, sizeof(struct sockaddr_802));
			break;

		case DLI_ETHERNET:
			if( error = proto_reserved(so_addr->choose_addr.dli_eaddr.dli_protype) )
			{
				return(error);
			}
			if( error = eaddr_reserved(&so_addr->choose_addr.dli_eaddr, ifp, uentry) )
			{
				return(error);
			}
			uentry->dli_sockopt.dli_state = DLS_ON;
			bcopy((u_char *) &so_addr->choose_addr.dli_eaddr, 
				(u_char *) &uentry->dli_lineid.choose_addr.dli_eaddr,
				 sizeof(struct sockaddr_edl));
			break;

		case DLI_POINTOPOINT:
			if ( ! so_addr->choose_addr.dli_paddr.dev_cstate.if_next_family )
			{
				so_addr->choose_addr.dli_paddr.dev_cstate.if_next_family = AF_DLI;
			}
			if ( error = change_dlistate(ifp,
				&so_addr->choose_addr.dli_paddr.dev_cstate,
				&uentry->dli_lineid.choose_addr.dli_paddr.dev_cstate,
				&uentry->dli_lineid.choose_addr.dli_paddr.dev_pstate) )
			{
				return(error);
			}
			/*
			 * NOTE: the lineid entry must be fully set up before
			 * going to sleep.
			 */
			uentry->dli_lineid.dli_substructype = so_addr->dli_substructype;
			uentry->dli_lineid.dli_family = so_addr->dli_family;
			bcopy((u_char *) &so_addr->dli_device, 
				(u_char *) &uentry->dli_lineid.dli_device,
				 sizeof(struct dli_devid));
			uentry->dli_if = ifp;
			/*
			 * need to lock out device interrupts when making 
			 * decision to go to sleep.
			 */
			s = splimp();
			if ( ifp->if_flags & IFF_UP )
				uentry->dli_sockopt.dli_state = DLS_ON;
			else {
				uentry->dli_sockopt.dli_state = DLS_SLEEP;
				while ( uentry->dli_sockopt.dli_state == DLS_SLEEP )
				{
					smp_unlock(&uentry->dli_so->lk_socket);
					smp_unlock(&uentry->dli_lk);
					sleep((caddr_t) ifp, PZERO+1);
					smp_lock(&uentry->dli_lk, LK_RETRY);
					if (uentry->dli_so == NULL )
						panic("dli_bind: socket gone!");
					smp_lock(&uentry->dli_so->lk_socket, LK_RETRY);
				}
			}
			splx(s);
			break;
		default:
			return(EOPNOTSUPP);
			break;
	}
	uentry->dli_lineid.dli_substructype = so_addr->dli_substructype;
	uentry->dli_lineid.dli_family = so_addr->dli_family;
	uentry->dli_if = ifp;
	uentry->dli_portid = dli_openport(ifp, &uentry->dli_lineid, uentry->dli_sockopt.dli_mcast, &uentry->dli_proto);
	return(0);

}




/*
 *		m a t c h _ d e v i c e
 *
 * Find the ifnet interface for the selected device.
 *
 * Returns:		= Pointer to ifnet structure if found,
 *			  otherwise, NULL.
 *
 * Inputs:
 *	device		= Pointer to the structure containing the device id.
 */
struct ifnet *match_device( device )
register struct dli_devid  *device;
{
	register struct ifnet *ifp = ifnet;
	register int i, nlen;
	u_char *c;

	/*
	 * validate device name.
	 */
	if( (nlen = strlen(device->dli_devname)) > DLI_DEVSIZE )
	{
		return(NULL);
	}
	for ( c = device->dli_devname, i = 0; i < nlen; i++)
	{
		if ( ! ((*c >='a' && *c <='z') || (*c >='A' && *c <='Z')) )
		{
			return(NULL);
		}
	} 

	/*
	 * search ifnet linked list for device.
	 */
	while (ifp)
	{
		if ( (i = strlen(ifp->if_name)) == nlen
			&& bcmp(ifp->if_name, device->dli_devname, nlen) == NULL
			&& ifp->if_unit == device->dli_devnumber )
		{
			if ( ifp->if_flags &  IFF_RUNNING )
			{
				return(ifp);
			}
			else
			{
				return(NULL);
			}
		}
		ifp = ifp->if_next;
	}
	return(NULL);
}





/*
 *		e a d d r _ r e s e r v e d
 *
 * make sure address is not reserved.
 *
 * Returns:		Error code if error occurs, otherwise NULL.
 *
 * Inputs:
 *	eaddr		= Pointer to structure containing Ethernet addr info.
 *	ifp		= Pointer to devices ifnet structure.
 *	ue		= Pointer to user's dli line table entry.
 */
eaddr_reserved( eaddr, ifp, ue )
register struct sockaddr_edl *eaddr;
struct ifnet *ifp;
struct dli_line *ue;
{
	extern struct dli_line dli_ltable[];
	register int i, error;
	register struct sockaddr_edl *search_eaddr;
	u_char	flags = eaddr->dli_ioctlflg;

	/*
	 * Target address should be physical
	 */
	if ( eaddr->dli_target[0] & 1 )
	{
		return(EINVAL);
	}

	/*
	 * Only one control flag should be set.
	 */
	switch (flags)
	{
		case DLI_EXCLUSIVE:
		case DLI_NORMAL:
		case DLI_DEFAULT:
			break;
		default:
			return(EINVAL);
			break;
	}

	/*
	 * Make sure address structure is unique per device.
	 */
	smp_unlock(&ue->dli_so->lk_socket);
	for ( i = 0; i < dli_maxline; i++ )
	{
		if ( &dli_ltable[i] == ue )
			continue;

		smp_lock(&dli_ltable[i].dli_lk, LK_RETRY);
		if (dli_ltable[i].dli_lineid.dli_substructype != DLI_ETHERNET ||
			dli_ltable[i].dli_so == NULL ||
			ifp != dli_ltable[i].dli_if )
		{
			smp_unlock(&dli_ltable[i].dli_lk);
			continue;
		}
		search_eaddr = &dli_ltable[i].dli_lineid.choose_addr.dli_eaddr;
		error = NULL;
		switch (search_eaddr->dli_ioctlflg)
		{
			case NULL:
				break;

			case DLI_EXCLUSIVE:
				error = search_eaddr->dli_protype == eaddr->dli_protype ? EADDRINUSE : NULL;
				break;

			case DLI_NORMAL:
				switch (flags)
				{
					case DLI_EXCLUSIVE:
						error = search_eaddr->dli_protype == eaddr->dli_protype ? EADDRINUSE : NULL;
						break;

					case DLI_NORMAL:
						error = search_eaddr->dli_protype == eaddr->dli_protype ? EADDRINUSE : NULL;
						if (error)
						{
							error = match_targets(search_eaddr->dli_target, eaddr->dli_target);
						}
						break;
					default:
						break;
				}
				break;

			case DLI_DEFAULT:
				switch (flags)
				{
					case DLI_EXCLUSIVE:
					case DLI_DEFAULT:
						error = search_eaddr->dli_protype == eaddr->dli_protype ? EADDRINUSE : NULL;
						break;

					default:
						break;
				}
				break;


			default:
				panic( "dli_bind: eaddr_reserved:" );
				break;
		}
		if (error != NULL)
		{
			smp_unlock(&dli_ltable[i].dli_lk);
			smp_lock(&ue->dli_so->lk_socket, LK_RETRY);
			return(error);
		}
		smp_unlock(&dli_ltable[i].dli_lk);
	}
	smp_lock(&ue->dli_so->lk_socket, LK_RETRY);
	return(NULL);

}



/*
 *		p r o t o _ r e s e r v e d
 *
 * make sure protocol type is not reserved.
 *
 * Returns:		Error code if error occurs, otherwise NULL.
 *
 * Inputs:
 *	ptype		= protocol type to be checked.
 */
extern struct if_family if_family[];
proto_reserved( ptype )
u_short ptype;
{
	register int i;

	/*
	 * Protocol types hardwired in QE and DE drivers.
	 */
	switch ( ptype )
	{
		case ETHERTYPE_IP:
		case ETHERTYPE_ARP:
			return(EACCES);
			break;
		default:
			if(ptype <= OSI802)	
			{
				return(EACCES);
			}
			else if( ptype >= ETHERTYPE_TRAIL && ptype < (ETHERTYPE_TRAIL + ETHERTYPE_NTRAILER) )
			{
				return(EACCES);
			}
			break;
	}


	/*
	 * Protocol types enabled in if_family array.
	 */
	i = -1;
	while ( if_family[++i].if_type != -1 )
	{
		if ( ptype == if_family[i].if_type )
		{
			return(EACCES);
		}
	}


	return(NULL);
}




/*
 *		d l i _ o p e n p o r t
 *
 * Open a port to correct data link module for network mgt purposes.
 *
 * Returns:		portid or zero if no port.
 *
 * Inputs:
 */
dli_openport(ifp, soaddr, mcast, protoptr)
struct ifnet *ifp;
struct sockaddr_dl *soaddr;
u_char *mcast;
struct protosw **protoptr;
{
    int i, j, portid = 0;
    struct dli_ifop op;
    extern struct dli_line dli_ltable[];
    struct protosw *pr;

    switch ( soaddr->dli_substructype )
    {
	case DLI_ETHERNET:
	case DLI_802:
#ifdef IFT_FDDI
    	    if (ifp->if_type == IFT_FDDI)
		pr = pffindproto(AF_DLI, DLPROTO_FDDI, 0);
    	    else
#endif
        	pr = pffindproto(AF_DLI, DLPROTO_CSMACD, 0);
	    break;

	case DLI_POINTOPOINT:
        	pr = pffindproto(AF_DLI, DLPROTO_DDCMP, 0);
		break;

	default:
		return(0);
		break;
    }


    if ( pr == NULL )
	return(0);

    /*
     * Initialize Open Port structure.  Note that
     * DLI sockets don't set up client name and station name,
     * and they don't make use of port name.
     */
    bzero(&op, sizeof(op));
    op.ifp = ifp;
    op.protosw = NULL;
    for ( i = 0, j = 0; i < MAXMCAST && j < MCAST_MAXNUM*MCAST_SIZE; j += MCAST_SIZE )
	if ( mcast[j] & 1 )
	    bcopy(&mcast[j], op.dldef.type3.mcast[i++], DLI_EADDRSIZE);

    switch ( soaddr->dli_substructype )
    {
	case DLI_ETHERNET:
            op.dldef.type3.llc = USER_SUPPLIED;
	    for ( i = 0, j = 0; i < dli_maxline; i++ )
	    {
		if ( dli_ltable[i].dli_lineid.choose_addr.dli_eaddr.dli_protype 
		     == soaddr->choose_addr.dli_eaddr.dli_protype )
		{
			if ( dli_ltable[i].dli_portid != 0 )
			{
			    pr = dli_ltable[i].dli_proto;
			    portid = dli_ltable[i].dli_portid;
			}
			j++;
		}
	    }
	    if ( j > 1 )
	    {
		*protoptr = pr;
		return(portid);	/* a port has already been opened */
	    }
    	    op.dldef.type3.ptype = soaddr->choose_addr.dli_eaddr.dli_protype;
    	    op.dltype = DLI_ETHERNET;
	    break;
		
	case DLI_802:
    	    if ( soaddr->choose_addr.dli_802addr.svc == USER )
        	op.dldef.type3.llc = USER_SUPPLIED;
    	    else
        	op.dldef.type3.llc = CLASS1_TYPE1;
	    for ( i = 0, j = 0; i < dli_maxline; i++ )
	    {
		if ( soaddr->choose_addr.dli_802addr.eh_802.ssap != SNAP_SAP )
		{
		    if ( dli_ltable[i].dli_lineid.choose_addr.dli_802addr.eh_802.ssap
		     	== soaddr->choose_addr.dli_802addr.eh_802.ssap )
		    {
			if ( dli_ltable[i].dli_portid != 0 )
			{
			    pr = dli_ltable[i].dli_proto;
			    portid = dli_ltable[i].dli_portid;
			}
			j++;
		    }
		}
		else
		{
		    if ( bcmp( dli_ltable[i].dli_lineid.choose_addr.dli_802addr.eh_802.osi_pi,
			      soaddr->choose_addr.dli_802addr.eh_802.osi_pi,
			      sizeof(soaddr->choose_addr.dli_802addr.eh_802.osi_pi))
			      == 0 )
		    {
			if ( dli_ltable[i].dli_portid != 0 )
			{
			    pr = dli_ltable[i].dli_proto;
			    portid = dli_ltable[i].dli_portid;
	    		}
			j++;
		    }
		}
	    }
	    if ( j > 1 )
	    {
		*protoptr = pr;
		return(portid);	/* a port has already been opened */
	    }
    	    op.dldef.type3.selector = soaddr->choose_addr.dli_802addr.eh_802.ssap;
	    bcopy(soaddr->choose_addr.dli_802addr.eh_802.osi_pi, 
		op.dldef.type3.pid, sizeof(op.dldef.type3.pid));
    	    op.dltype = DLI_802;
	    break;
		
	case DLI_POINTOPOINT:
    	    op.dltype = DLI_POINTOPOINT;
	    break;
		
	default:
	    return(EOPNOTSUPP);
	    break;
    }
    if ( (*pr->pr_ctloutput)(PRCO_PIF, NULL, 0, DLIPIF_OPENPORT, &op) == 0 )
	portid = (u_int) op.port;
    if ( portid )
	*protoptr = pr;
    return(portid);
}
