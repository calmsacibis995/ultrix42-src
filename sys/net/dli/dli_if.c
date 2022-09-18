#ifndef	lint
static char *sccsid = "@(#)dli_if.c	4.2	ULTRIX	9/4/90";
#endif	lint

/*
 * Program dli_if.c,  Module DLI 
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
 *      DECnet-ULTRIX   V2.0
 *		- added sysid and point to point support
 */

#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/protosw.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/errno.h"
#include "../../h/ioctl.h"
#include "../../h/user.h"
#include "../../h/buf.h"
#include "../../h/conf.h"
#include "../../h/proc.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../net/net/if.h"
#include "../../net/net/netisr.h"
#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax

int smpdlidebug = 0;
#define printd	if ( smpdlidebug ) printf
extern struct ifqueue dli_intrq;
extern struct lock_t lk_dli;




/*
 * Routines to handle protocol specific needs of interface drivers.
 */

/*
 *		d l i _ i f o u t p u t
 *
 * Perform protocol specific functions for message transmission.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	ifp		= Pointer to output device structure
 *	m		= Pointer to MBUF chain to be output
 *	sockdst		= Destination socket address
 *	type		= Location to return link level protocol type
 *	net_dst		= Location(s) to return destination address
 */
dli_ifoutput( ifp,m,sockdst,type,net_dst )
struct ifnet *ifp;
struct mbuf *m;
struct sockaddr_dl *sockdst;
int *type;
char *net_dst;
{

    switch ( sockdst->dli_substructype )
    {

	case DLI_802:
		*(struct ether_pa *)net_dst = *(struct ether_pa *)sockdst->choose_addr.dli_802addr.eh_802.dst;
		/*
		 * 802.3 - the ethernet protocol field is the length 
		 * of the 802.3 header information and the user data
		 */
#ifdef IFT_FDDI
		*type = 0;
#else
		*type = (u_short) m_length(m);
#endif
		break;

	case DLI_ETHERNET:
		*(struct ether_pa *) net_dst = *(struct ether_pa *) sockdst->choose_addr.dli_eaddr.dli_target;
    		*type = sockdst->choose_addr.dli_eaddr.dli_protype;
		break;

	case DLI_POINTOPOINT:
		break;

	default:
		panic( "dli_ifoutput" );
		break;
    }
}



/*
 *		d l i _ i f i n p u t
 *
 * Perform protocol specific functions for message reception.
 *
 * Returns:		Pointer to the (possibly modified) MBUF chain of
 *			the received message
 *			0 if allocation failed.
 *
 * Inputs:
 *	m		= Pointer to MBUF chain of received message
 *	ifp		= Pointer to input device structure
 *	inq		= Location to return pointer to protocol input queue
 *	eh		= Pointer to received Ethernet header, if appropriate
 */


extern struct dli_line dli_ltable[];
extern struct if_isapt if_isapt[];
extern struct ether_header no_enet_header;
extern u_char loopback_mcast[];
extern u_char sysid_mcast[];


struct mbuf *dli_ifinput( m,ifp,inq,eh )
struct mbuf *m;
struct ifnet *ifp;
struct ifqueue **inq;
struct ether_header *eh;
{
    struct mbuf *m0;
    struct dli_recv *recv;
    register int i;


#ifdef notdef
    /*
     * Make a quick check to see if packet is wanted.
     * Note that the DLI line table entries are purposely not
     * locked.  The entries are only being read, and if they
     * happen to be momentarily incorrect, then this will only
     * result in a packet being accepted only to be dropped at
     * a later time.
     */
    i = 0;
    if ( eh != NULL && (eh->ether_dhost[0] & 1) == 0 )
    {
	if ( eh->ether_type > OSI802 ) 
	{
      	    if ( eh->ether_type != DLI_LBACK && eh->ether_type != CCRPROTO )
		for ( ; i < dli_maxline; i++ )
		{
	    	    if ( ifp != dli_ltable[i].dli_if )
		        continue;
	    	    if ( dli_ltable[i].dli_lineid.dli_substructype != DLI_ETHERNET )
		        continue;
	    	    if ( eh->ether_type == dli_ltable[i].dli_lineid.choose_addr.dli_eaddr.dli_protype )
		        break;
		}
	}
        else
        {
	    u_char dsap = *(mtod(m, u_char *));
	    if ( (dsap & 1) == 0 && dsap != SNAP_SAP && dsap != 0 )
	    {
	        for( ; i < MAX_BROADCSTDEV; i++)
	        {
	            if ( ifp != if_isapt[i].ifp )
		        continue;
		    if ( if_isapt[i].so[dsap>>VSAP] == NULL )
		        continue;	/* don't want pkt on this device */
		    break;		/* want packet */
	        }
		if ( i == MAX_BROADCSTDEV )
		    i = dli_maxline;
	    }
	}
    }
    if ( i == dli_maxline )	/* don't want packet */
    {
        m_freem(m);
        return (0);
    }
#endif

    /* Accept it. */
    MGET(m0, M_DONTWAIT, MT_DATA);
    if ( m0 )
    {
	m0->m_next = m;
	m0->m_act = NULL;
	m0->m_len = sizeof(struct dli_recv);
	recv = mtod(m0, struct dli_recv *);
	if ( eh )
		recv->rcv_hdr.rcv_ether = *eh;
	else
		recv->rcv_hdr.rcv_ether = no_enet_header;
	recv->rcv_ifp = ifp;
        if (ifp->if_flags & IFF_POINTOPOINT)
        {
		struct protosw *pr = pffindproto(AF_DLI, DLPROTO_DDCMP, 0);
		if ( pr )
                        recv->rcv_protocol = DLPROTO_DDCMP;
                else
                        recv->rcv_protocol = DLPROTO_DLI;
        }
        else
                recv->rcv_protocol = DLPROTO_CSMACD;
	smp_lock(&lk_dliintrq, LK_RETRY);
	schednetisr(NETISR_DLI);
	*inq = &dli_intrq;
	return (m0);
    }
    m_freem(m);
    return (0);
}



extern struct dli_timers lback_timers[];
/*
 *		d l i _ i f s t a t e
 *
 * Perform protocol specific function for device state change.
 * Should be called from a driver.
 *
 * Returns:		0 = false, 1 = true.
 *
 * Inputs:
 *	ifp		= Pointer to device structure
 *	state		= I/O control command
 *	data		= Pointer to I/O control data
 */
dli_ifstate( ifp,state,data )
struct ifnet *ifp;
int state;
caddr_t data;
{
	int i;
	struct ifstate  *dmcstate = (struct ifstate *) data;
	int saveaffinity;  /* for nonsmp drivers.  8.18.88.us */
        int locked;

	switch ( state )
	{
		case IFS_HALTED:
		case IFS_HALTING:
			if ( dmcstate->if_family == AF_DLI ) {
				for ( i = 0 ; i < dli_maxline; i++ )
				{
                                        if (!(locked = smp_owner(&dli_ltable[i].dli_lk)))
					smp_lock(&dli_ltable[i].dli_lk, LK_RETRY);
					if ( (ifp == dli_ltable[i].dli_if) &&
						(dli_ltable[i].dli_sockopt.dli_state == DLS_ON) ) {
						*dmcstate = dli_ltable[i].dli_lineid.choose_addr.dli_paddr.dev_cstate;
						dmcstate->if_wrstate = ~IFS_WRSTATE;
						dmcstate->if_rdstate = ~IFS_RDSTATE;
						dmcstate->if_xferctl = ~IFS_XFERCTL;
					}
					smp_unlock(&dli_ltable[i].dli_lk);
				}
                                if ( i == dli_maxline )
				{
					struct protosw *pr = pffindproto(AF_DLI, DLPROTO_DDCMP, 0);
					if ( pr )
                                        	(*pr->pr_ifstate)( ifp,state,data);
				}
			} else {
				dmcstate->if_family = dmcstate->if_next_family;
			}
			return(1);
			break;

		case IFS_ENTEREDMOP:
			smp_lock(&lk_dli, LK_RETRY);
			for ( i = 0; i < DLI_MAX_LBTIMR; i++ )
			{
				if ( ! lback_timers[i].ifp || lback_timers[i].ifp == ifp)
				{
					smp_unlock(&lk_dli);
					break;
				}
			}
			if ( i == DLI_MAX_LBTIMR )
			{
				smp_unlock(&lk_dli);
				if ( (dmcstate->if_family = dmcstate->if_next_family) == AF_DECnet )
				{
					dmcstate->if_next_family = AF_UNSPEC;
					dmcstate->if_mode = IFS_DDCMPFDX;
					dmcstate->if_ustate = IFS_USRON;
				}
				else
				{
					dmcstate->if_ustate = IFS_USROFF;
				}

			}
			else
			{
				lback_timers[i].ifp = ifp;
				lback_timers[i].tval = DLI_LBEVL_POP*2;
				bzero(lback_timers[i].actv_addr, DLI_EADDRSIZE);
				lback_timers[i].prev_devstate = *dmcstate;
				smp_unlock(&lk_dli);
				log_event(ifp, DLI_EVLOP_LBINI);
				dmcstate->if_family = AF_DLI;;
				dmcstate->if_next_family = AF_UNSPEC;
				dmcstate->if_mode = IFS_MOP;
				dmcstate->if_nomuxhdr = IFS_NOMUXHDR;
				dmcstate->if_ustate = IFS_USRON;
			}
			return(1);
			break;

		case IFS_RUNNING:
			if ( (ifp->if_flags & IFF_POINTOPOINT) && (dmcstate->if_family == AF_DLI) ) {
				for ( i = 0 ; i < dli_maxline; i++ )
				{
                                        if (!(locked = smp_owner(&dli_ltable[i].dli_lk)))
					smp_lock(&dli_ltable[i].dli_lk, LK_RETRY);
					if (ifp == dli_ltable[i].dli_if) {
						if ( dli_ltable[i].dli_sockopt.dli_state == DLS_SLEEP ) {
							dli_ltable[i].dli_sockopt.dli_state = DLS_ON; 
							wakeup((caddr_t) ifp);
						}
                                                if (!locked)
						smp_unlock(&dli_ltable[i].dli_lk);
						break;
					}
                                        if (!locked)
					smp_unlock(&dli_ltable[i].dli_lk);
				}
                                if ( i == dli_maxline )
				{
					struct protosw *pr = pffindproto(AF_DLI, DLPROTO_DDCMP, 0);
					if ( pr )
                                        	(*pr->pr_ifstate)( ifp,state,data);
				}
				if ( i == dli_maxline ) {
					u_char dli_locked = 1;
					smp_lock(&lk_dli, LK_RETRY);
					for ( i = 0; i < DLI_MAX_LBTIMR; i++ )
					{
						if ( lback_timers[i].ifp == ifp)
						{
							struct sockaddr_dl dst;
							struct mbuf *m;
    							MGET(m, M_DONTWAIT, MT_DATA);
    							if ( m )
    							{
								m->m_next = NULL;
								m->m_act = NULL;
								m->m_len = 1;
								*mtod(m, u_char *) = 0xc4;
								dst.dli_family = AF_DLI;
								dst.dli_substructype = DLI_POINTOPOINT;
								smp_unlock(&lk_dli);
								dli_locked = 0;
								CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
								ifp->if_output(ifp, m, &dst);
								RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
							}
							break;
						}
					}
					if ( dli_locked )
						smp_unlock(&lk_dli);
				}
			} 
			break;

		case IFS_STARTING:
		default:
			break;
	}

	return(0);
}


/*
 *		d l i _ i f i o c t l
 *
 * Perform protocol specific function for I/O control processing.
 * Should be called from a driver.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	ifp		= Pointer to device structure
 *	cmd		= I/O control command
 *	data		= Pointer to I/O control data
 */
dli_ifioctl( ifp,cmd,data )
struct ifnet *ifp;
int cmd;
caddr_t data;
{
/*ckw
    return(0);
ckw*/
        if (ifp->if_flags & IFF_POINTOPOINT)
	{
		struct protosw *pr = pffindproto(AF_DLI, DLPROTO_DDCMP, 0);
		if ( pr )
                	(*pr->pr_ifioctl)(ifp, cmd, data);
	}
}


/*
 *		d l i _ m a k e n w i f
 *
 * Establish a network interface.
 *
 * Returns:		errno status.
 *
 * Inputs:
 *	ifp		= pointer to device structure
 *	family		= domain address.
 *	locsiz		= number of bytes in local address.
 *      locaddr		= pointer to local address or NULL.
 *	dstsiz		= number of bytes in destination address.
 *      dstaddr		= pointer to destination address or NULL.
 */
dli_make_nwif(ifp, family, locsiz, locaddr, dstsiz, dstaddr)
struct ifnet *ifp;
u_short family;
u_int locsiz;
u_char *locaddr;
u_int dstsiz;
u_char *dstaddr;
{
	register struct ifaddr *ifa, *ia = NULL;
	struct mbuf *m;
        struct sockaddr sin;
	u_int error;

	bzero(&sin, sizeof(sin));
	sin.sa_family = family;
	if ( locsiz )
	    bcopy(locaddr, sin.sa_data, locsiz);

	/*
	 * See if DLI has created an entry for this interface.
	 * If entry 
	 *	exit - work is done;
	 * else
	 *	create one and start device.
	 */
	if (ifp && ifp->if_addrlist)
	{
		for (ia = ifp->if_addrlist; ia->ifa_next; ia = ia->ifa_next)
		{
			if (ia->ifa_addr.sa_family == family)
				break;
		}
		if ( ia->ifa_addr.sa_family == family)
			return(0);
	}

 	m = m_getclr(M_DONTWAIT, MT_IFADDR);
 	if (m == (struct mbuf *)NULL)
	{
 		return (ENOBUFS);
	}
 	ia = mtod(m, struct ifaddr *);
	if (ifa = ifp->if_addrlist) 
	{
		/* 
		 * add to end of interface list of addr 
	   	 * families supported by the device 
		 */
		for ( ; ifa->ifa_next; ifa = ifa->ifa_next)
			;
		ifa->ifa_next = (struct ifaddr *) ia;
	} 
	else  /* start list */
		ifp->if_addrlist = (struct ifaddr *) ia;
	ia->ifa_ifp = ifp;
	/* ia->ifa_addr.sa_family = family; */
	ia->ifa_addr = sin;
	error = dli_init_driver(ifp, ia, &sin, SIOCSIFADDR);
	if ( ! error && dstsiz > 0 && (ifp->if_flags & IFF_POINTOPOINT) != 0 )
	{
        	/* ia->ifa_dstaddr.sa_family = family; */
	    	bcopy(dstaddr, sin.sa_data, dstsiz);
		ia->ifa_dstaddr = sin;
		dli_init_driver(ifp, ia, &sin, SIOCSIFDSTADDR);
	}
	return(error);
}


/*
 *              d l i _ i n i t _ d r i v e r 
 *
 * Notify driver of domain's existence.
 *
 * Returns:             Nothing.
 *
 * Inputs:
 *
 * Outputs:		Nothing.
 *
 */
dli_init_driver(ifp, ia, sin, cmd)
register struct ifnet *ifp;
register struct ifaddr *ia;
struct sockaddr *sin;
u_int cmd;
{
	int s = splimp(), error;
	int saveaffinity;


	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = (*ifp->if_ioctl)(ifp, cmd, ia);
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
	if (error){
		splx(s);
		bzero((caddr_t)&ia->ifa_addr, sizeof(ia->ifa_addr));
		return (error);
	}
	splx(s);
	return (0);
}
