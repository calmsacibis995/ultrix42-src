#ifndef	lint
static char *sccsid = "@(#)dli_input.c	4.3	(DECnet-ULTRIX)	4/30/91";
#endif	lint

/*
 * Program dli_input.c,  Module DLI 
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
 *		DECnet-Ultrix	V2.0
 *
 * Added sysid and point-to-point support
 *
 * 2.01 18-Mar-1988
 *      DECnet-ULTRIX   V2.4
 *		- Allowed use of reserved bit in individual and group SAPs
 *		- Fixed request counters and request sysid processing code
 *			to examine packet count.
 *
 * 2.02  02-Jun-1988
 *              DECnet-Ultrix   V2.4
 *              Added support for ln driver.
 *
 * 2.03 25-Jul-1988
 *              DECnet-Ultrix   V3.0
 *              Restore sysid destination to Console Carrier multicast
 *                      after transmitting to specific node.
 *
 * 	19-Dec-1988	Matt Thomas
 *	Fix unaligned accesses for PMAX (for sysid and rqctrs).
 *
 *	29-Apr-1991	Matt Thomas
 *	Add code so that dliintrq is emptied by one cpu at a time.
 *
 */
#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/protosw.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/domain.h"
#include "../../h/errno.h"
#include "../../h/ioctl.h"
#include "../../h/user.h"
#include "../../h/buf.h"
#include "../../h/conf.h"
#include "../../h/proc.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../net/net/if.h"

#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/evl.h"
#include "../../net/netdnet/dn.h"
#include "../../net/netdnet/decnet_types.h"

#include "../../net/netdnet/dli_var.h"

extern struct ifqueue dli_intrq;

extern struct sockaddr_dl sysid_dst;

extern struct ether_header no_enet_header;

extern struct domain dlidomain;
extern struct protosw dlisw[];
extern u_char sysid_mcast[];
extern u_char loopback_mcast[];
extern struct dli_sysid_dev mop_dev_code[];
extern struct dli_sysid_to sysid_to[];
extern u_short nqna;
extern struct lock_t lk_dli;



/*
 * DLI domain received message processing.
 */

 /*
 *		d l i i n t r
 *
 * DLI domain input routine. This routine is 'called' from the network software
 * ISR routine to process incoming packets. The first MBUF in any chain
 * contains a DLI receive descriptor containing the received ethernet header
 * and a pointer to the interface structure.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 */
dliintr()
{
    register struct mbuf *m;
    register struct dli_recv *recv;
    register struct protosw *pr;
    struct mbuf *loopback_enet_msg(), *loopback_ptop_msg();
    int s, owner = 0;
    static int dliintrq_owned = 0;

next:
    /*
     * Try to pull an input message (MBUF chain) from the DLI input queue.
     */
    s = splimp();
    m = NULL;
    smp_lock(&lk_dliintrq, LK_RETRY);
    if (owner || !dliintrq_owned) {
	IF_DEQUEUE(&dli_intrq, m);
	owner = dliintrq_owned = (m != NULL);
    }
    smp_unlock(&lk_dliintrq);
    splx(s);
    if (m)
    {
	recv = mtod(m, struct dli_recv *);

	/*
	 * Pass to destined protocol module.  Probe DLPROTO_DLI
	 * last since it is lower priority.
	 */
	pr = &dlisw[0];
	while ( ++pr != dlidomain.dom_protoswNPROTOSW ) {
		if ( pr->pr_protocol == recv->rcv_protocol && pr->pr_input )
		{
			m = (struct mbuf *) (*pr->pr_input)(m);
			break;
		}
	}
	if ( m  && recv->rcv_protocol == DLPROTO_CSMACD )
		dli_input(m);
	else
	{
		if ( m )
			m_freem(m);
	}
	goto next;
    }
}



/*
 *		d l i _ i n p u t
 *
 * DLPROTO_DLI input routine. This routine is 'called' from dliintr()
 * to process incoming packets. The first MBUF in any chain
 * contains a DLI receive descriptor containing the received ethernet header
 * and a pointer to the interface structure.  This routine makes sure all
 * mbufs are disposed properly.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 */
dli_input(m)
struct mbuf *m;
{
    	register struct dli_recv *recv;
    	struct dli_recv rcv;
    	struct mbuf *loopback_enet_msg(), *loopback_ptop_msg();
    	int s;

	recv = mtod(m, struct dli_recv *);
	bcopy(recv, &rcv, sizeof(rcv));
	/*
	 * If there is no data link header,
	 *	process as point to point.
	 * else 
	 *	process as ethernet.
	 */
	if ( bcmp(&(recv->rcv_hdr.rcv_ether), &no_enet_header, sizeof(recv->rcv_hdr.rcv_ether)) == 0 )
	{
		if ( m = loopback_ptop_msg(m, (recv->rcv_ifp->if_flags & IFF_MOP), &rcv) )
		{
			forward_to_user(m, &rcv);
		}
	}
	else
	{
		/*
		 * if ethernet protocol is < 0x5dc
		 * 	process 802.3 packet
		 * else if message is request sysid
		 *	send sysid message
		 * else if message is request for mop counters
		 *	send counters.
		 * else if loopback response requested
		 *	send loopback response
		 * else
		 *	attempt to pass ethernet message to user.
		 */
		if(recv->rcv_hdr.rcv_ether.ether_type <= OSI802)
		{
			osi_802input(m, &rcv);
		}
		else if( (recv->rcv_hdr.rcv_ether.ether_type == SYSIDPROTO) && 
		(*(mtod(m->m_next, u_char *) + sizeof(u_short)) == REQSYSID) )
		{
				dli_proc_reqsysid(m, &rcv);
		}
		else if( (recv->rcv_hdr.rcv_ether.ether_type == CCRPROTO) && 
		(*(mtod(m->m_next, u_char *) + sizeof(u_short)) == DLI_REQCTRS) )
		{
				dli_proc_reqctrs(m, &rcv);
		}
		else if ( (recv->rcv_hdr.rcv_ether.ether_type != DLI_LBACK) || (m = loopback_enet_msg(m, &rcv)) )
		{
			forward_to_user(m, &rcv);
		}
	}
}



/*
 *		f o r w a r d _ t o _ u s e r 	
 *
 * This routine attempts to find the user to whom the current
 * packet belongs.  If successful, the message is placed on
 * the user's socket receive queue.  Otherwise, the packet is
 * dropped.
 *
 * Outputs:		mbuf chain (possibly) place on user's rcv queue.
 *
 * Inputs:		m = mbuf chain containing packet.  Note that the
 *				first mbuf contains junk at this point.
 *			rcv = pointer to data link header structure.
 */

extern struct dli_line dli_ltable[];

forward_to_user( m, rcv )
register struct mbuf *m;
register struct dli_recv *rcv;
{
	register struct sockaddr_edl *eaddr;
	register struct sockaddr_802 *oaddr;
	register struct sockaddr_pdl *paddr;
	register int i;
	register int save_i = -1;
	struct socket *dliso, *save_so = NULL;
	struct lock_t *dlisolk;


	for ( i = 0; i < dli_maxline; i++ )
	{
		smp_lock(&dli_ltable[i].dli_lk, LK_RETRY);
		if ( rcv->rcv_ifp != dli_ltable[i].dli_if )
 		{
			smp_unlock(&dli_ltable[i].dli_lk);
 			continue;
 		}

		switch ( dli_ltable[i].dli_lineid.dli_substructype )
		{
			case DLI_802:
				/*
				 * the only valid way to get here 
				 * is from the 802 input routine
				 * for snap saps.
				 * all other saps have either been
				 * handled by the responder, or gone 
				 * directly to found_user
				 */
				if(rcv->rcv_hdr.osi_header.dsap != SNAP_SAP)
					break;
				oaddr = &dli_ltable[i].dli_lineid.choose_addr.dli_802addr;
				switch ( oaddr->ioctl )
				{
					case DLI_EXCLUSIVE:
						if(bcmp(rcv->rcv_hdr.osi_header.osi_pi, oaddr->eh_802.osi_pi, 5) == NULL)
						{
							smp_lock((dlisolk = &(dli_ltable[i].dli_so->lk_socket)), LK_RETRY);
							found_user(m, dli_ltable[i].dli_so, DLI_802, rcv);
							smp_unlock(dlisolk);
							smp_unlock(&dli_ltable[i].dli_lk);
							return;
						}
						break;

					case DLI_NORMAL:
						if( (bcmp(rcv->rcv_hdr.osi_header.osi_pi, oaddr->eh_802.osi_pi, 5) == NULL) &&
							match_targets(oaddr->eh_802.dst, rcv->rcv_hdr.osi_header.src) &&
							match_mcast(dli_ltable[i].dli_sockopt.dli_mcast,
							rcv->rcv_hdr.osi_header.dst))
						{
							smp_lock((dlisolk = &(dli_ltable[i].dli_so->lk_socket)), LK_RETRY);
							found_user(m, dli_ltable[i].dli_so, DLI_802, rcv);
							smp_unlock(dlisolk);
							smp_unlock(&dli_ltable[i].dli_lk);
							return;
						}
						break;

					case DLI_DEFAULT:
						if(bcmp(rcv->rcv_hdr.osi_header.osi_pi, oaddr->eh_802.osi_pi, 5) == NULL) 
						{
							save_i = i;
							save_so = dli_ltable[i].dli_so;
						}
						break;

					default:
						break;
				}
				break;

			case DLI_ETHERNET:
				eaddr = &dli_ltable[i].dli_lineid.choose_addr.dli_eaddr;
				switch ( eaddr->dli_ioctlflg )
				{
					case DLI_EXCLUSIVE:
						if (rcv->rcv_hdr.rcv_ether.ether_type == eaddr->dli_protype)
						{
							smp_lock((dlisolk = &(dli_ltable[i].dli_so->lk_socket)), LK_RETRY);
							found_user(m, dli_ltable[i].dli_so, DLI_ETHERNET, rcv);
							smp_unlock(dlisolk);
							smp_unlock(&dli_ltable[i].dli_lk);
							return;
						}
						break;

					case DLI_NORMAL:
						if ( (rcv->rcv_hdr.rcv_ether.ether_type ==  eaddr->dli_protype) &&
						    match_targets(eaddr->dli_target, rcv->rcv_hdr.rcv_ether.ether_shost) &&
						    match_mcast(dli_ltable[i].dli_sockopt.dli_mcast,
						    rcv->rcv_hdr.rcv_ether.ether_dhost))
						{
							smp_lock((dlisolk = &(dli_ltable[i].dli_so->lk_socket)), LK_RETRY);
							found_user(m, dli_ltable[i].dli_so, DLI_ETHERNET, rcv);
							smp_unlock(dlisolk);
							smp_unlock(&dli_ltable[i].dli_lk);
							return;
						}
						break;

					case DLI_DEFAULT:
						if (rcv->rcv_hdr.rcv_ether.ether_type == eaddr->dli_protype)
						{
							save_i = i;
							save_so = dli_ltable[i].dli_so;
						}
						break;

					default:
						break;
				}
				break;

			case DLI_POINTOPOINT:
				smp_lock((dlisolk = &(dli_ltable[i].dli_so->lk_socket)), LK_RETRY);
				found_user(m, dli_ltable[i].dli_so, DLI_POINTOPOINT, rcv);
				smp_unlock(dlisolk);
				smp_unlock(&dli_ltable[i].dli_lk);
				return;
				break;

			default:
				panic( "dli_input: forward_to_user" );
				break;
		}
		smp_unlock(&dli_ltable[i].dli_lk);
	}
	if ( save_so ) 
	{
		smp_lock(&dli_ltable[save_i].dli_lk, LK_RETRY);
		if (dli_ltable[save_i].dli_so == save_so)
		{
		    smp_lock((dlisolk = &(dli_ltable[save_i].dli_so->lk_socket)), LK_RETRY);
		    found_user(m, dli_ltable[save_i].dli_so, dli_ltable[save_i].dli_lineid.dli_substructype, rcv);
		    smp_unlock(dlisolk);
		}
		else 
		    m_freem(m);
		smp_unlock(&dli_ltable[save_i].dli_lk);
		return;
	}
	m_freem(m);
	return;
}





/*
 *		f o u n d _ u s e r
 *
 * This routine places an mbuf chain on a user's receive queue.
 *
 * Note:		Both line table entry and socket must be locked
 *				before this routine called.
 *
 * Outputs:		None.
 *
 * Inputs:		m = Pointer to mbuf chain; first mbuf is
 *				garbage.
 * 			so = Pointer to user's socket structure.
 *			link_type = type of link ( ethernet, point to point )
 *			rcv = pointer to data link header structure.
 */
found_user( m, so, link_type, rcv )
register struct mbuf *m;
register struct socket *so;
u_char link_type;
register struct dli_recv *rcv;
{
	register struct sockaddr_dl *so_dl;
	register struct mbuf *temp;
	u_short len;

	if ( so == NULL )
	{
		panic( "dli_input: found_user1" );
	}

        if ( so == (struct socket *) -1 )
        {
                panic( "dli_input, found_user2: bad socket" );
        }

	/*
	 * Make sure there's enough room on Rx Q
	 */
	if ( sbspace(&so->so_rcv) < 0 )
	{
		m_freem(m);
		return;
	}

	/*
	 * Allocate some memory for the socket address structure
	 * needed by sbappendaddr().
	 * clear out memory and place address info in it.
	 */
	KM_ALLOC(so_dl, struct sockaddr_dl *, sizeof(struct sockaddr_dl), KM_SONAME, KM_NOW_CL);
	if ( so_dl == NULL )
	{
		m_freem(m);
		return;
	}
	so_dl->dli_family = AF_DLI;
	switch ( so_dl->dli_substructype = link_type )
	{
		case DLI_802:
			/*
			 * only individual saps should be coming through here
			 */
			so_dl->choose_addr.dli_802addr.eh_802.len = rcv->rcv_hdr.osi_header.len;
			*(struct ether_pa *) so_dl->choose_addr.dli_802addr.eh_802.dst = *(struct ether_pa *) rcv->rcv_hdr.rcv_ether.ether_shost;
			*(struct ether_pa *) so_dl->choose_addr.dli_802addr.eh_802.src = *(struct ether_pa *) rcv->rcv_hdr.rcv_ether.ether_dhost;
			/* this is who enabled the sap */
			so_dl->choose_addr.dli_802addr.eh_802.ssap = rcv->rcv_hdr.osi_header.dsap;
			/* this is who it came from */
			so_dl->choose_addr.dli_802addr.eh_802.dsap = rcv->rcv_hdr.osi_header.ssap;
			if (so_dl->choose_addr.dli_802addr.eh_802.dsap == SNAP_SAP)
				bcopy(rcv->rcv_hdr.osi_header.osi_pi, 
				  so_dl->choose_addr.dli_802addr.eh_802.osi_pi, 
				  sizeof(so_dl->choose_addr.dli_802addr.eh_802.osi_pi));
			if( (rcv->rcv_hdr.osi_header.ctl.U_fmt & 3) == 3)
			{
				so_dl->choose_addr.dli_802addr.eh_802.ctl.U_fmt = rcv->rcv_hdr.osi_header.ctl.U_fmt;
			}
			else
			{
				so_dl->choose_addr.dli_802addr.eh_802.ctl.I_S_fmt = rcv->rcv_hdr.osi_header.ctl.I_S_fmt;
			}
			break;

		case DLI_ETHERNET:
			so_dl->choose_addr.dli_eaddr.dli_protype = rcv->rcv_hdr.rcv_ether.ether_type;
			*(struct ether_pa *) so_dl->choose_addr.dli_eaddr.dli_target = *(struct ether_pa *) rcv->rcv_hdr.rcv_ether.ether_shost;
			*(struct ether_pa *) so_dl->choose_addr.dli_eaddr.dli_dest = *(struct ether_pa *) rcv->rcv_hdr.rcv_ether.ether_dhost;
			break;

		case DLI_POINTOPOINT:
			break;

		default:
			panic( "dli, found_user2" );
			break;
	}

	so_dl->dli_device.dli_devnumber = rcv->rcv_ifp->if_unit;
	bcopy(rcv->rcv_ifp->if_name, so_dl->dli_device.dli_devname, strlen(rcv->rcv_ifp->if_name));
	if ( sbappendanyaddr(&so->so_rcv, (u_char *) so_dl, sizeof(struct sockaddr_dl), m->m_next, NULL) == 0 )
	{
		KM_FREE(so_dl, KM_SONAME);
		m_freem(m);
		return;
	}
	KM_FREE(so_dl, KM_SONAME);
	MFREE(m, temp);
	sorwakeup(so);
	return;
}



extern struct dli_timers lback_timers[];
/*
 *		l o o p b a c k _ e n e t _ m s g
 *
 *		This routine processes Ethernet loopback messages.  If the
 *		function code is "forward data," then the message
 *		is forwarded to its next destination.  Otherwise,
 *		nothing is done.
 *
 * Outputs:		mbuf chain given to driver if message to be forwared.
 *			returns NULL if message looped, otherwise mbuf pointer returned.
 *
 * Inputs:		m = mbuf chain containing packet.  
 *			rcv = pointer to data link header structure.
 */
struct mbuf *loopback_enet_msg( m, rcv )
register struct mbuf *m;
register struct dli_recv *rcv;
{
    int saveaffinity;  /* for nonsymm drivers. 8.18.88.us */
    int error;
    struct sockaddr_dl dst_addr;
    u_char *loop_msg;
    u_short loop_sc, timer_active, i;
    struct ifdevea ifd;

    /*
     * If to multicast, make sure its the correct one.
     */
    if ( rcv->rcv_hdr.rcv_ether.ether_dhost[0] & MCASTADDR )
	if (bcmp( rcv->rcv_hdr.rcv_ether.ether_dhost, loopback_mcast, DLI_EADDRSIZE) != 0)
	{
		m_freem(m);
		return(NULL);
	}

    /*
     * pull up header into second mbuf
     */
    if ( ! pull_header(m, 30) )
    {
	return(NULL);
    }

    /*
     * determine if message should be looped back
     */
    if ( (loop_sc = EXT16(mtod(m->m_next, u_short *)) + sizeof(loop_sc)) > 28 )
    {
	return(m);
    }
    loop_msg = mtod(m->m_next, u_char *);
    if ( loop_msg[loop_sc++] != DLI_LBACK_FWD || loop_msg[loop_sc++] != NULL )
    {
	return(m);
    }
    CALL_TO_NONSMP_DRIVER( (*rcv->rcv_ifp), saveaffinity);
    error = rcv->rcv_ifp->if_ioctl(rcv->rcv_ifp, SIOCRPHYSADDR, (caddr_t)&ifd);
    RETURN_FROM_NONSMP_DRIVER( (*rcv->rcv_ifp), saveaffinity);
    if ( (error)
	|| (bcmp(ifd.current_pa, rcv->rcv_hdr.rcv_ether.ether_shost, DLI_EADDRSIZE) == 0) )
    {
		m_freem(m);
		return(NULL);
    }

    /*
     * log passive loopback initiated event if not already given for
     * present node.
     */
    timer_active = NULL;
    smp_lock(&lk_dli, LK_RETRY);
    for( i = 0; i < DLI_MAX_LBTIMR; i++)
    {
	if ( (lback_timers[i].tval != 0) && 
     		bcmp(rcv->rcv_hdr.rcv_ether.ether_shost, lback_timers[i].actv_addr, DLI_EADDRSIZE) == NULL )
	{
		lback_timers[i].tval = DLI_LBEVL_WAIT;
		timer_active = 1;
		break;
	}
    }
    if ( ! timer_active && establish_event( &rcv->rcv_hdr.rcv_ether, rcv ) )
    {
	smp_unlock(&lk_dli);
	log_event(rcv->rcv_ifp, DLI_EVLOP_LBINI);
    }
    else
	smp_unlock(&lk_dli);


    /*
     * forward loopback message.
     */
    dst_addr.dli_family = AF_DLI;
    dst_addr.dli_substructype = DLI_ETHERNET;
    *(struct ether_pa *) dst_addr.choose_addr.dli_eaddr.dli_target = *(struct ether_pa *) (loop_msg+loop_sc);
    dst_addr.choose_addr.dli_eaddr.dli_protype = rcv->rcv_hdr.rcv_ether.ether_type;
    loop_sc += (DLI_EADDRSIZE - sizeof(loop_sc));
    INS16(loop_msg, loop_sc);
    CALL_TO_NONSMP_DRIVER( (*rcv->rcv_ifp), saveaffinity);
    rcv->rcv_ifp->if_output(rcv->rcv_ifp, m->m_next, &dst_addr);
    RETURN_FROM_NONSMP_DRIVER( (*rcv->rcv_ifp), saveaffinity);
    m_free(m);
    return(NULL);
}



/*
 *		l o o p b a c k _ p t o p _ m s g
 *
 *		This routine processes Point to Point loopback messages.  
 *
 * Outputs:		mbuf chain given to driver if message to be forwared.
 *			returns NULL if message looped, otherwise mbuf pointer returned.
 *
 * Inputs:		m = mbuf chain containing packet.  
 *			mop = 1 if MOP, 0 if not MOP mode
 *			rcv = pointer to data link header structure.
 */
struct mbuf *loopback_ptop_msg( m, mop, rcv )
register struct mbuf *m;
int mop;
register struct dli_recv *rcv;
{
	u_char *mop_code;
	u_short i;
	int saveaffinity;  /* for nonsym drivers.  8.18.88.us  */
	struct sockaddr_dl dst_addr;

	/*
	 * make sure device is currently looping; if so, reset loopback timer;
	 * if not, packet belongs to a user.
	 */
	smp_lock(&lk_dli, LK_RETRY);
 	for( i = 0; i < DLI_MAX_LBTIMR; i++)
 	{
		if ( (lback_timers[i].tval != 0) &&  (rcv->rcv_ifp == lback_timers[i].ifp) )
		{
			lback_timers[i].tval = DLI_LBEVL_POP;
			break;
		}
     	}
	smp_unlock(&lk_dli);
	if ( i == DLI_MAX_LBTIMR && ! mop )
	{
		return(m);
	}

	/*
	 * pull MOP code into second mbuf if not already there
	 */
	if ( ! pull_header(m, 1) )
	{
		return(NULL);
	}
	else if ( *(mop_code = mtod(m->m_next, u_char *)) == DLI_LBACK_LOOP )
	{
		/* free first mbuf which has no relevant data */
		m = m_free(m);

		/*
		 * forward loopback message.
		 */
		dst_addr.dli_family = AF_DLI;
		dst_addr.dli_substructype = DLI_POINTOPOINT;
		*mop_code = (u_char) DLI_LBACK_LOOPED;
		CALL_TO_NONSMP_DRIVER( (*rcv->rcv_ifp), saveaffinity);
		rcv->rcv_ifp->if_output(rcv->rcv_ifp, m, &dst_addr);
		RETURN_FROM_NONSMP_DRIVER( (*rcv->rcv_ifp), saveaffinity);
		return(NULL);
	}
	else
	{
		return(m);
	}
}



/*
 *		l o g _ e v e n t
 *
 *	This subroutine logs a passive loopback message to evl.
 *
 * Outputs:		None.
 *
 * Inputs:		None.  
 */
log_event(ifp, evl_op)
register struct ifnet *ifp;
u_char evl_op;
{
    struct protosw *evl_ptr;
    static struct event events[DLI_MAXPROC];
#define event events[CURRENT_CPUDATA->cpu_num]
    register int i;

    /*
     * log event only if evl is present
     */
    if ( ! (evl_ptr = pffindproto( AF_DECnet, DNPROTO_EVR )) )
    {
	return;
    }


    /*
     * init event structure.
     */
    event.e_class = DLI_LBEVL_CLASS;
    event.e_type = DLI_LBEVL_TYPE;
    event.e_ent_type = DLI_LBEVL_ETYPE;
    i = fetch_decnet_devname( ifp, event.e_ent_id );
    event.e_ent_id[i++] = ifp->if_unit + '0';
    event.e_ent_id[i] = NULL;
    event.e_data[0] = DLI_EVLOP_CODE;
    event.e_data[1] = NULL;
    event.e_data[2] = DLI_EVLOP_DESC;
    event.e_data[3] = evl_op;
    event.e_data_len = 4;


    /*
     * log event
     */
    (evl_ptr->pr_input)( &event );
    return;
#undef event

}



/*
 *		s c m p
 *
 *	This subroutine compares two strings.
 *
 * Outputs:		0 if strings unequal, 1 if strings equal.
 *
 * Inputs:		s1, s2 = pointers to strings to be compared.  
 */
scmp(s1, s2)
register char *s1, *s2;
{
    while ( *s1 == *s2++ )
	if ( *s1++ == NULL )
		return(1);
    return(0);
}



/*
 *		p u l l _ h e a d e r
 *
 *		This routine pulls up a header into the second mbuf.  
 * 		NOTE: first mbuf contains info placed by dli_ifinput.
 *
 * Outputs:		1 if successful, 0 if failure.
 *
 * Inputs:		m = mbuf chain containing packet.  
 *			hsiz = size of header to be pulled up.
 */
pull_header( m, hsiz )
register struct mbuf *m;
register short hsiz;
{

    register int i = 0;
    register struct mbuf *tm = m->m_next;

    if ( hsiz <= m->m_next->m_len )
	return(1);

    while ( tm )
    {
	i += tm->m_len;
	tm = tm->m_next;
    }
    if ( (m->m_next = m_pullup(m->m_next, ((i < hsiz) ? i : hsiz))) == NULL )
    {
	m_freem(m);
	return(NULL);
    }
    return(1);

}



/*
 *		e s t a b l i s h _ e v e n t
 *
 *	This routine sets up a loopback event in the
 *	loopback timer table.
 *
 * Note:  DLI protocol lock must be asserted before this
 *	  routine is called.
 *
 * Inputs:		header = address of packet header.  
 *				 (zeroed out for point to point)
 *			rcv = pointer to data link header structure.
 *
 * Outputs:		1 if success, otherwise NULL.
 */
establish_event( eh, rcv )
register struct ether_header *eh;
register struct dli_recv *rcv;
{
	int i = -1;

	while (lback_timers[++i].tval != 0 && i < DLI_MAX_LBTIMR) ;

	if ( i < DLI_MAX_LBTIMR )
	{
		lback_timers[i].tval = DLI_LBEVL_WAIT;
		lback_timers[i].ifp = rcv->rcv_ifp;
	     	*(struct ether_pa *) lback_timers[i].actv_addr = *(struct ether_pa *) eh->ether_shost;
		return(1);
	}
	else
	{
		return(0);
	}

}






/*
 *		f e t c h _ d e c n e t _ d e v n a m e
 *
 *	This routine translates the ULTRIX device name into the DECnet
 *	device name.
 *
 * Inputs:		dn_devname = pointer where DECnet device name is to 
 *					placed.
 *
 * Outputs:		dn_devname = DECnet device name.
 *
 * Returns:		number of characters in device name.
 *
 */
fetch_decnet_devname( ifp, dn_devname )
register struct ifnet *ifp;
register u_char *dn_devname;
{
    register int i = 0;

    if ( scmp(ifp->if_name, "qe") )
    {
	bcopy( "QNA-", dn_devname, (i = 4) );
    }
    else if ( scmp(ifp->if_name, "de") )
    {
	bcopy( "UNA-", dn_devname, (i = 4) );
    }
    else if ( scmp(ifp->if_name, "ni") )
    {
	bcopy( "BNT-", dn_devname, (i = 4) );
    }
    else if ( scmp(ifp->if_name, "xna") )
    {
        bcopy( "XNA-", dn_devname, (i = 4) );
    }
    else if ( scmp(ifp->if_name, "ln") )
    {
        bcopy( "SVA-", dn_devname, (i = 4) );
    }
    else if ( scmp(ifp->if_name, "dmc") )
    {
	bcopy( "DMC-", dn_devname, (i = 4) );
    }
    else if ( scmp(ifp->if_name, "dmv") )
    {
	bcopy( "DMV-", dn_devname, (i = 4) );
    }
    else
    {
	i = 0;
	while ( ifp->if_name[i] )
	{
		dn_devname[i] = ifp->if_name[i];
		if ( dn_devname[i] >= 'a' )
			dn_devname[i] = dn_devname[i] - ('a' - 'A');
		i++;
	}
	dn_devname[i++] = '-';
    }

    return(i);

}
#ifdef notdef

/*
 *		m b u f _ l e n
 *
 * Compute the number of bytes in a (non-empty) MBUF chain.
 *
 * Returns:		The number of bytes in the chain.
 *
 * Inputs:
 *	m		= Pointer to the MBUF chain.
 */
mbuf_len( m )
register struct mbuf *m;
{
    register struct mbuf *m0 = m;
    register int len = 0;

	while( m0 )
    {
		len += m0->m_len;
		m0 = m0->m_next;
    }

    return (len);
}
#endif

/*
 *		d l i _ p r o c _ r e q s y s i d
 *
 * This routine is called to process requests for sysid 
 * If this is a request for a sysid from a node
 *    find the matching sysid_to struct to tx on
 *    copy requestor node address to the target
 *    transmit the sysid 
 *
 * Outputs:		None.
 *
 * Inputs:		pointer to mbuf chain from driver
 *              pointer to dli_recv structure.
 *
 * Version History:
 * 1.0	JA
 *
 */

dli_proc_reqsysid(m, recv)
struct mbuf *m;
struct dli_recv *recv;
{
	register int i, nsiz = 0;
	u_short msglen;
    
    	/*
    	 * If to multicast, make sure its the correct one.
    	 */
    	if ( recv->rcv_hdr.rcv_ether.ether_dhost[0] & MCASTADDR )
		if (bcmp( recv->rcv_hdr.rcv_ether.ether_dhost, sysid_mcast, DLI_EADDRSIZE) != 0)
		{
			m_freem(m);
			return;
		}

	msglen = EXT16(mtod(m->m_next, u_char *));
	if ( msglen < 4 )
	{
		m_freem(m);
		return;
	}

	while( *(((u_char *) recv->rcv_ifp->if_name)+nsiz) != NULL )
		nsiz++;
	i = -1;
	while ( mop_dev_code[++i].devtyp )
		if( bcmp(mop_dev_code[i].devnam, recv->rcv_ifp->if_name, nsiz) == 0 )
		{
			smp_lock(&lk_dli, LK_RETRY);
			*(struct ether_pa *)sysid_dst.choose_addr.dli_eaddr.dli_target = *(struct ether_pa *)recv->rcv_hdr.rcv_ether.ether_shost; 
			dli_snd_sysid(recv->rcv_ifp, EXT16((mtod(m->m_next, u_char *) + 4)), mop_dev_code[i].devtyp);
			*(struct ether_pa *)sysid_dst.choose_addr.dli_eaddr.dli_target = *(struct ether_pa *) sysid_mcast; 
			smp_unlock(&lk_dli);
			break;
		}
	m_freem(m);
}


/*
 *		d l i _ p r o c _ r e q  c t r s
 *
 * This routine is called to process requests for data link counters 
 *
 * Outputs:		None.
 *
 * Inputs:		pointer to mbuf chain from driver
 *              	pointer to dli_recv structure.
 *
 * Version History:
 * 1.0	JA
 *
 */

dli_proc_reqctrs(m, recv)
struct mbuf *m;
struct dli_recv *recv;
{
    struct ctrreq ctrs;
    u_short receipt, msglen;
    register u_char *bmsg, *msg = mtod(m, u_char *);
    int saveaffinity;  /* for nonsym drivers.  8.18.88.us  */
    int error;
    struct sockaddr_dl dst_addr;
    struct mbuf *recv_hdr;

   /*
    * If to multicast, make sure its the correct one.
    */
    if ( recv->rcv_hdr.rcv_ether.ether_dhost[0] & MCASTADDR )
	if (bcmp(recv->rcv_hdr.rcv_ether.ether_dhost, sysid_mcast, DLI_EADDRSIZE) != 0)
	{
		m_freem(m);
		return;
	}

    /*
     * pull MOP message  into second mbuf if not already there
     */
    if ( ! pull_header(m, 5) )
    {
	return;
    }
    msglen = EXT16(mtod(m->m_next, u_short *));
    if ( msglen < 3 )
    {
	m_freem(m);
	return;
    }


    /*
     * Save receipt from message and then delete remainder of message,
     * saving one mbuf for transmission of counters.
     */
    receipt = EXT16((mtod(m->m_next, u_char *) + sizeof(u_short) + 1));
    m_freem(m->m_next);
    m->m_next = NULL;

    CALL_TO_NONSMP_DRIVER( (*recv->rcv_ifp), saveaffinity);
    error = ( recv->rcv_ifp->if_ioctl(recv->rcv_ifp, SIOCRDCTRS, (caddr_t)&ctrs));
    RETURN_FROM_NONSMP_DRIVER( (*recv->rcv_ifp), saveaffinity);
    if ( error )
    {
	m_freem(m);
	return;
    }

    /*
     * place counters in mbuf and transmit
     */
     bmsg = msg;
     msg += sizeof(u_short);
     *msg++ = DLI_CTRS;
     INS16(msg, receipt);
     msg += sizeof(u_short);
     switch ( ctrs.ctr_type )
     {
	case CTR_ETHER:
		/* ethernet device  - format counters */
		INS16(msg, ctrs.ctr_ether.est_seconds);
		msg += sizeof(ctrs.ctr_ether.est_seconds);
		INS32(msg, ctrs.ctr_ether.est_bytercvd);
		msg += sizeof(ctrs.ctr_ether.est_bytercvd);
		INS32(msg, ctrs.ctr_ether.est_bytesent);
		msg += sizeof(ctrs.ctr_ether.est_bytesent);
		INS32(msg, ctrs.ctr_ether.est_blokrcvd);
		msg += sizeof(ctrs.ctr_ether.est_blokrcvd);
		INS32(msg, ctrs.ctr_ether.est_bloksent);
		msg += sizeof(ctrs.ctr_ether.est_bloksent);
		INS32(msg, ctrs.ctr_ether.est_mbytercvd);
		msg += sizeof(ctrs.ctr_ether.est_mbytercvd);
		INS32(msg, ctrs.ctr_ether.est_mblokrcvd);
		msg += sizeof(ctrs.ctr_ether.est_mblokrcvd);
		INS32(msg, ctrs.ctr_ether.est_deferred);
		msg += sizeof(ctrs.ctr_ether.est_deferred);
		INS32(msg, ctrs.ctr_ether.est_single);
		msg += sizeof(ctrs.ctr_ether.est_single);
		INS32(msg, ctrs.ctr_ether.est_multiple);
		msg += sizeof(ctrs.ctr_ether.est_multiple);
		INS16(msg, ctrs.ctr_ether.est_sendfail);
		msg += sizeof(ctrs.ctr_ether.est_sendfail);
		INS16(msg, ctrs.ctr_ether.est_sendfail_bm);
		msg += sizeof(ctrs.ctr_ether.est_sendfail_bm);
		INS16(msg, ctrs.ctr_ether.est_recvfail);
		msg += sizeof(ctrs.ctr_ether.est_recvfail);
		INS16(msg, ctrs.ctr_ether.est_recvfail_bm);
		msg += sizeof(ctrs.ctr_ether.est_recvfail_bm);
		INS16(msg, ctrs.ctr_ether.est_unrecog);
		msg += sizeof(ctrs.ctr_ether.est_unrecog);
		INS16(msg, ctrs.ctr_ether.est_overrun);
		msg += sizeof(ctrs.ctr_ether.est_overrun);
		INS16(msg, ctrs.ctr_ether.est_sysbuf);
		msg += sizeof(ctrs.ctr_ether.est_sysbuf);
		INS16(msg, ctrs.ctr_ether.est_userbuf);
		msg += sizeof(ctrs.ctr_ether.est_userbuf);
    		/*
		 * Place message size at beginning, create dest addr,
		 * and transmit
		 */
		m->m_len = (short) (msg - bmsg); 
    		INS16(bmsg, (u_short) (m->m_len - sizeof(u_short)));
    		dst_addr.dli_family = AF_DLI;
    		dst_addr.dli_substructype = DLI_ETHERNET;
    		*(struct ether_pa *) dst_addr.choose_addr.dli_eaddr.dli_target = 
			*(struct ether_pa *) recv->rcv_hdr.rcv_ether.ether_shost;
    		dst_addr.choose_addr.dli_eaddr.dli_protype = recv->rcv_hdr.rcv_ether.ether_type;
		break;

     	case CTR_DDCMP:
		/* point to point device */

	default:
		m_freem(m);
		return;
		break;
    }

    CALL_TO_NONSMP_DRIVER( (*recv->rcv_ifp), saveaffinity);
    recv->rcv_ifp->if_output(recv->rcv_ifp, m, &dst_addr);
    RETURN_FROM_NONSMP_DRIVER( (*recv->rcv_ifp), saveaffinity);
    return;
}
