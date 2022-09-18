#ifndef lint
static char *sccsid = "@(#)net_common.c	4.7	(ULTRIX)	4/4/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990    by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University	of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ---------------------------------------------------------------------
 * Common code for processing received ethernet packets
 *
 * Modification History 
 *
 *
 * 02-Apr-1991 - jsd
 *	Fix FDDI packetfilter code (transmit side)
 *
 * 02-Jan-1991 - John Dustin and Matt Thomas
 *	Move packetfilter code into pfilt.c to keep net_read() simple.
 *	Fix PROMISC mode so it can co-exist with LAT.  Restore lost
 *	PFF_COPYALL changes from earlier release.  Rework pfilt_filter()
 *	for 20% performance improvement when in PROMISC mode.
 *
 * 20-Aug-1990 - Matt Thomas
 *	Fix support for 802.3 frames that it will work with DLI.
 *	This means moving some checking from DLI to here.
 *
 * 26-Mar-1990 - lp & templin
 *	Worked in idea of generic net_output routine so that ethernet
 *	drivers may use 802.2 encapsulation as well. Eventually all lan
 *	drivers should use these routines to input and output
 *	data. Action of each routine is driven by ifp->if_type
 *	and ifp->if_flags.
 *
 * ---------------------------------------------------------------------
 */

/*
 * TODO:
 *	- Look at a way of providing trailer support for interfaces
 *	  which actually reap benefits of using it. (i.e. QNA, UNA).
 *
 *	- Packet Filter with FDDI
 *
 */

#include "packetfilter.h"	/* NPACKETFILTER */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/kernel.h"
#include "../h/ipc.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#if	NPACKETFILTER > 0
#include "../net/net/pfilt.h"
#endif	NPACKETFILTER
#include "../net/net/if_llc.h
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/if_ether.h"
#include "../net/netinet/if_fddi.h"
#include "../net/net/ether_driver.h"

#ifdef	vax
#include "../machine/mtpr.h"
#endif	vax

extern struct protosw *iftype_to_proto();
int net_output(), net_read();

u_char	broadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
u_char	org_code_ether[3] = { 0x0, 0x0, 0x0 };	/* Org. Code for SNAP */
u_char	org_code_dec[3] = { 0x08, 0x00, 0x2b };	/* DIGITAL company code */
#define	ac ((struct arpcom *)ifp)

extern int pfactive;	/* from pfilt.c, number of currently active filters */

/*
 * LAN output routine.
 * Encapsulate a packet of type family for the local net
 * using 802.2 "encapsulated ethernet" SNAP-SAP format
 * for IP and ARP in compliance with RFC1103 and RFC1042.
 * (Also, for any other protocols which do not perform LLC
 * encapsulation themselves). Assumes that ifp is actually
 * pointer to arpcom structure.
 */
net_output(ifp, m0, dst)
	register struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	short type;
	int s, error = 0;
	struct in_addr idst;
	u_char odst[6];
	register struct ether_header *eh;
	register struct fddi_header *fh;
	register struct mbuf *m = m0;
	int usetrailers;		/* Trailers ignored; see RFC1103 */
	extern struct timeval time;
	struct protosw *pr, *iffamily_to_proto();

        if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		error = ENETDOWN;
		goto bad;
        }
	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		if (nINET == 0 || ac->ac_ipaddr.s_addr == 0) {
			printf("net_output: %s%d: can't handle af%d\n",
				ifp->if_name, ifp->if_unit, dst->sa_family);
			error = EAFNOSUPPORT;
			goto bad;
		}
		idst = ((struct sockaddr_in *)dst)->sin_addr;
		if (!arpresolve(ac, m, &idst, (caddr_t)odst, &usetrailers))
			return (0);	/* if not yet resolved */
		type = ETHERTYPE_IP;
		break;
#endif
#if NPACKETFILTER > 0
        case AF_IMPLINK:
		if(ifp->if_type == IFT_ETHER) {
	                eh = mtod(m, struct ether_header *);
        		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)eh->ether_shost, 
		      		sizeof(ac->ac_enaddr));
                	goto gotheader;
		}
		else if (ifp->if_type == IFT_FDDI) {
		    fh = mtod(m, struct fddi_header *);

		    /* 
		     * Extract Frame Control stuff and sanity-check it.
		     * These checks probably need improvement.
		     * We only allow 48-bit address mode.
		     */
		    switch ((fh->fddi_fc) & (FDDIFC_C|FDDIFC_L|FDDIFC_F)) {
			case FDDIFC_LLC_ASYNC:
			    /* legal priorities are 0 through 7 */
			    if ((fh->fddi_fc & FDDIFC_Z) > 7) {
				error = EPROTONOSUPPORT;
				goto bad;
			    }
			    break;
			case FDDIFC_LLC_SYNC:
			    /* FDDIFC_Z bits reserved, must be zero */
			    if (fh->fddi_fc & FDDIFC_Z) {
				error = EPROTONOSUPPORT;
				goto bad;
			    }
			    break;
			case FDDIFC_SMT:
			    /* FDDIFC_Z bits must be non zero */
			    if ((fh->fddi_fc & FDDIFC_Z) == 0) {
				error = EPROTONOSUPPORT;
				goto bad;
			    }
			    break;
			default:
			    /* anything else is too dangerous */
			    error = EPROTONOSUPPORT;
			    goto bad;
		    }
		    
		    /* Fill in must-be-right parts of FDDI header */
		    bcopy((caddr_t)ac->ac_enaddr, (caddr_t)fh->fddi_shost,
			      sizeof(ac->ac_enaddr));
		    fh->fddi_ph[0] = FDDIPH0;
		    fh->fddi_ph[1] = FDDIPH1; 
		    fh->fddi_ph[2] = FDDIPH2;
		    
		    goto gotheader;
		} else {
                        error = EAFNOSUPPORT;
			goto bad;
		}
		break;
#endif NPACKETFILTER
	case AF_UNSPEC:
		/* 
		 * Constructed by ARP with ethernet header. Strip dhost and
		 * type from ethernet header for use in MAC header and LLC
		 * encapsulated ethernet header. BIT ORDER FOR ALL MAC HDRS
		 * ASSUMED "CANONICAL" AS FOR ENET. (See RFC1103 and RFC1042).
		 */
		eh = (struct ether_header *)dst->sa_data;
		bcopy((caddr_t)eh->ether_dhost, (caddr_t)odst,
		      sizeof(odst));
		type = eh->ether_type;
		break;

	default:
                /*
                 * Try to find other address families and call protocol
                 * specific output routine. Protocols which perform LLC
		 * encapsulation in higher layers pass a type value of
		 * zero as an indication.
                 */
                if (pr = iffamily_to_proto(dst->sa_family)) {
                        (*pr->pr_ifoutput)(ifp, m0, dst, &type, (char *)odst);
                } else {
                        printf("net_output: %s%d: can't handle af%d\n",
				ifp->if_name, ifp->if_unit, dst->sa_family);
                        error = EAFNOSUPPORT;
                        m_freem(m0);
                        return(error);
                }
	}

	/*
	 * Grab off enough space for LLC and MAC headers
	 */
	if (m->m_off > MMAXOFF || (MMINOFF + M_NETPAD) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_DATA);
		if (m == 0) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = m0;
		m->m_off = MMINOFF + M_NETPAD;
		m->m_len = 0;
	}
	/*
	 * Add LLC header for 802.2 media using "Encapsulated Ethernet" frame
	 * format.
	 */
	if ((ifp->if_flags & IFF_802HDR) && (type != 0))  {
		register struct llc *lh;

		m->m_off -= sizeof (struct llc);
		m->m_len += sizeof (struct llc);

		lh = mtod(m, struct llc *);
		/*
		 * Fill out encapsulated ethernet (SNAP-SAP) 802.2 header
		 * in accordance with RFC1103 and RFC1042:
		 *
		 *	- DSAP = SNAP = 170 (AA Hex)
		 *	- SSAP = SNAP = 170 (AA Hex)
		 *	- Control = UI = 3
		 *	- PID contains:
		 *		- 3-Octet Organization Code = 00-00-00
		 *		- 2-Octet Ether Type (Encapsulated Ethertype)
		 */
		type = htons((u_short)type);
		bcopy((caddr_t)&type,
		      (caddr_t)&lh->llc_un.type_snap.ether_type,
		      sizeof(u_short));
		lh->llc_dsap = LLC_SNAP_LSAP;
		lh->llc_ssap = LLC_SNAP_LSAP;
		lh->llc_un.type_snap.control = LLC_UI;
		bcopy(org_code_ether,
		      (caddr_t)lh->llc_un.type_snap.org_code, 3);
		type = 0;
	}

	/*
	 * Add MAC header.
	 */
	switch (ifp->if_type) {
	case IFT_ISO88023:
	case IFT_ETHER:

		/*
		 * Add Ethernet dhost, shost, and type. type == 0 indicates
		 * frame is in 802.3 format and type really contains length.
		 */
		if (!type) {
			struct mbuf *n = m0;
			while (n) {
				type += n->m_len;
				n = n->m_next;
			}
		}
		m->m_off -= sizeof (struct ether_header);
		m->m_len += sizeof (struct ether_header);
		type = htons((u_short)type);
		eh = mtod(m, struct ether_header *);
		bcopy((caddr_t)&type,(caddr_t)&eh->ether_type,
		      sizeof(eh->ether_type));
 		bcopy((caddr_t)odst, (caddr_t)eh->ether_dhost, sizeof (odst));
        	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)eh->ether_shost, 
		      sizeof(ac->ac_enaddr));
		break;
	case IFT_FDDI:

		m->m_off -= sizeof (struct fddi_header);
		m->m_len += sizeof (struct fddi_header);
		/*
		 * Add FDDI dhost, shost, PA, SD, and frame control
		 */
		fh = mtod(m, struct fddi_header *);
 		bcopy((caddr_t)odst, (caddr_t)fh->fddi_dhost, sizeof (odst));
        	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)fh->fddi_shost, 
		      sizeof(ac->ac_enaddr));
		fh->fddi_ph[0] = FDDIPH0;
		fh->fddi_ph[1] = FDDIPH1;
		fh->fddi_ph[2] = FDDIPH2;
		fh->fddi_fc = (u_char)(FDDIFC_LLC_ASYNC | FDDIFC_LLC_PRI4);
		break;
	default:
		error = EAFNOSUPPORT;
		goto bad;
	}

gotheader:
	s = splimp();
	if (ifp->lk_softc) {
	        smp_lock(ifp->lk_softc, LK_RETRY);
	}
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		if (ifp->lk_softc) {
			smp_unlock(ifp->lk_softc);
		}
		splx(s);
		error = ENOBUFS;
		goto bad;
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	if (!(ifp->if_flags & IFF_OACTIVE)) 
		(*ifp->if_start)(ifp->if_unit);
	if (ifp->lk_softc) {
		smp_unlock(ifp->lk_softc);
	}
        splx(s);
        return(0);

bad:
	if (m)
		m_freem(m);
	return (error);
}

/*
 * Called from the "read" routine of a LAN driver to
 * dispatch a received packet.
 */
net_read(edp, eptr, m, len, lenbogus, istrailer)
	struct ether_driver *edp;
	register struct ether_header *eptr;
	struct mbuf *m;
	int len, lenbogus, istrailer;
{
	struct ifqueue *inq;
	register struct ifnet *ifp = &edp->ess_if;
	struct protosw *pr;
	struct ether_header eh;
	u_short etype;
	int priority = 0;

	/*
	 * Convert FDDI headers to ethernet header for upper levels.
	 */
	if (ifp->if_type == IFT_FDDI) {
		struct fddi_header *fh = (struct fddi_header *)eptr;
		eptr = &eh;
		bcopy ((caddr_t)fh->fddi_dhost, (caddr_t)eptr->ether_dhost,
		       sizeof(eptr->ether_dhost));
		bcopy ((caddr_t)fh->fddi_shost, (caddr_t)eptr->ether_shost,
		       sizeof(eptr->ether_dhost));
		eptr->ether_type = 0;
		priority = 0;
	}

	/*
	 * Sanity check the 802.3 frames rcvd over an Ethernet interface.
	 * If the length in the protocol field is not equal to the
	 * length of the data and data length > Ethernet minimum data
	 * size, drop the packet.  If the data length equals the Ethernet
	 * minimum, adjust off the extra data at the end of the mbuf.
	 */
	if (ifp->if_type == IFT_ETHER && eptr->ether_type <= ETHERMTU) {
		if (eptr->ether_type != len && len != ETHERMIN) {
			if (eptr->ether_type > len) {
				edp->ess_ctrblk.est_recvfail_bm |= (1 << 2);
			} else {
				edp->ess_ctrblk.est_recvfail_bm |= (1 << 1);
			}
			if (edp->ess_ctrblk.est_recvfail != 0xffff)
				edp->ess_ctrblk.est_recvfail++;
			goto dropanyway;
		} else if (eptr->ether_type < len && len == ETHERMIN) {
			m_adj(m, -(len - eptr->ether_type));
			if (m->m_len <= 0 && ((m = m_free(m)) == 0))
				return;
			len = eptr->ether_type;
		}
		eptr->ether_type = 0;
	}

	/*
	 * Ethernet/802.3 has two-octet type field in ether_header for
	 * Protocol ID (Ethernet VII) OR packet length (OSI 802.3).
	 * FDDI has ZERO in type field.
	 */
	if (eptr->ether_type <= ETHERMTU) {
		struct llc *lh = mtod(m, struct llc *);
		if ((lh->llc_control == LLC_UI) &&
		    (lh->llc_dsap == LLC_SNAP_LSAP) &&
		    (lh->llc_ssap == LLC_SNAP_LSAP) &&
		    (!bcmp((caddr_t)lh->llc_org_code,
		     (caddr_t)org_code_ether,3))) {
			/*
			 * 802.2 LLC Encapsulated Ethernet type. Grab off
			 * the encapsulated type and trim LLC hdr.
			 */
			eptr->ether_type = ntohs((u_short)lh->llc_ether_type);
			if (eptr->ether_type > ETHERMTU) {
				m->m_off += sizeof (struct llc);
				m->m_len -= sizeof (struct llc);
				if (m->m_len <= 0 && ((m = m_free(m)) == 0))
					return;
			} else {
				eptr->ether_type = 0;
			}
		}
	}

	if (pfactive || (ifp->if_flags & IFF_PROMISC)) {
		m = (struct mbuf *) pfilt_filter(edp, m, eptr, istrailer);
		/*
		 * If the packet filter consumed the mbuf it was not
		 * destined for the local node, so just return.
		 */
		if (m == 0)
			return;
	}

#ifdef INET
	switch (eptr->ether_type) {
	case ETHERTYPE_IP:
		if (nINET == 0 || ac->ac_ipaddr.s_addr == 0)
			goto dropanyway;
		/*
		 * Need to lock ipintrq so we don't allow schednetisr
		 * to process the packet prior to us calling IF_ENQUEUE
		 * below.
		 */
		inq = &ipintrq;
		smp_lock(&inq->lk_ifqueue, LK_RETRY);
		schednetisr(NETISR_IP);
		break;

	case ETHERTYPE_ARP:
		if ((nETHER==0 && nFDDI==0) || ac->ac_ipaddr.s_addr==0)
			goto dropanyway;
		/*
		 * Need to unlock before calling arpinput, since
		 * it calls the driver output routine and we will
		 * already have locked the driver's softc.
		 */
		if (ifp->lk_softc) {
			smp_unlock(ifp->lk_softc);
			arpinput(ac, m);
			smp_lock(ifp->lk_softc, LK_RETRY);
		} else
			arpinput(ac, m);
		return;
#endif
	default:
		/*
		 * see if other protocol families defined
		 * and call protocol specific routines.
		 * If no other protocols defined then dump message.
		 */
		if ((pr=iftype_to_proto(eptr->ether_type)) && pr->pr_ifinput)  {
			if ((m = (struct mbuf *)(*pr->pr_ifinput)(m, ifp, &inq, eptr, priority)) == 0) {
				if (edp->ess_ctrblk.est_unrecog != 0xffff)
					edp->ess_ctrblk.est_unrecog++;
				return;
			}
			/* else break; */ /* goes to IF_ENQUEUEIF below */
		} else {
			if (edp->ess_ctrblk.est_unrecog != 0xffff)
				edp->ess_ctrblk.est_unrecog++;
			goto dropanyway;
		}
	} /* end switch */

	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		smp_unlock(&inq->lk_ifqueue);
		goto dropanyway;
	}
	IF_ENQUEUEIF(inq, m, ifp);
	smp_unlock(&inq->lk_ifqueue);
	return;
dropanyway:
	m_freem(m);
	return;
}

#if	NPACKETFILTER > 0
/*
 * Initialize the part of the ether_driver structure concerned with
 * the packet filter, and tell the packet filter driver about us
 */
attachpfilter_ethernet(edp)
struct ether_driver *edp;
{
	struct endevp enp;

	enp.end_dev_type = ENDT_10MB;
	enp.end_addr_len = sizeof(edp->ess_addr);
	enp.end_hdr_len = sizeof(struct ether_header);
	enp.end_MTU = ETHERMTU;
	bcopy((caddr_t)(edp->ess_addr),
		    (caddr_t)(enp.end_addr), sizeof(edp->ess_addr));
	bcopy((caddr_t)etherbroadcastaddr,
		    (caddr_t)(enp.end_broadaddr), sizeof(edp->ess_addr));

	edp->ess_enetunit = pfilt_attach(&(edp->ess_if), &enp);
	edp->ess_missed = 0;
}

/* compatibility with old driver code; remove this some day */
attachpfilter(edp)
struct ether_driver *edp;
{
	attachpfilter_ethernet(edp);
}

attachpfilter_fddi(edp)
struct ether_driver *edp;
{
	struct endevp enp;

	enp.end_dev_type = ENDT_FDDI;
	enp.end_addr_len = sizeof(edp->ess_addr);
	enp.end_hdr_len = sizeof(struct fddi_header);
	enp.end_MTU = FDDIMTU;
	bcopy((caddr_t)(edp->ess_addr),
		    (caddr_t)(enp.end_addr), sizeof(edp->ess_addr));
	bcopy((caddr_t)etherbroadcastaddr,
		    (caddr_t)(enp.end_broadaddr), sizeof(edp->ess_addr));

	edp->ess_enetunit = pfilt_attach(&(edp->ess_if), &enp);
	edp->ess_missed = 0;
}
#endif	NPACKETFILTER > 0
