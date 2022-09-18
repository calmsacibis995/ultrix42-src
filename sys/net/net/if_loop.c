#ifndef lint
static  char    *sccsid = "@(#)if_loop.c	4.1  (ULTRIX)                7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *   University    of   California,   Berkeley,   and   from   Bell	*
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

/************************************************************************
 *			Modification History				*
 *									*
 *	Uttam Shikarpur 23-Oct-89					*
 *		1) Added counters for network management		*
 *		2) Added support for SIOCRDCTRS, SIOCRDZCTRS and	*
 *		   SIOCRPHYSADDR ioctl's.				*
 *									*
 *	Matt Thomas 17-Jul-89						*
 *		Change ifnetptr reference to m_ifp.			*
 *									*
 *	Ursula Sinkewicz 28-Feb-89
 *		SMP/mips. (Added changes from R. Bhanukitsiri 02/06/89)
 *
 *	Larry Palmer 15-Jan-88						*
 *		Final 43bsd release (move from netinet)			*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	if_loop.c	6.6 (Berkeley) 6/8/85
 */

/*
 * Loopback interface driver for protocol testing and timing.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/smp_lock.h"

#include "../h/time.h"
#include "../h/proc.h"

#include "../net/net/if.h"
#include "../net/netinet/in.h"
#include "../net/netinet/if_ether.h"
#include "../net/net/ether_driver.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif


#define	LOMTU	(1024+512)

extern struct timeval time;
struct	ifnet loif;

struct 	lo_softc {
	struct	ether_driver lo_ed;	/* Ethernet common part	*/
#define	is_ac	lo_ed.ess_ac		/* arpcom */
#define ctrblk	lo_ed.ess_ctrblk	/* Counter block		*/
#define	ztime	lo_ed.ess_ztime		/* Time counters last zeroed	*/
} lo_softc;

int	looutput(), loioctl(), loinit();
/* initialize the fields for loop back statistics */

loattach()
{
	register struct ifnet *ifp = &loif;
	register struct sockaddr_in *sin;

	ifp->if_name = "lo";
	ifp->if_mtu = LOMTU;
	ifp->if_flags = IFF_LOOPBACK;
	ifp->if_init = loinit;
	ifp->if_ioctl = loioctl;
	ifp->if_output = looutput;
	ifp->d_affinity = ALLCPU;
	bcopy("DEC Local Loopback Interface", ifp->if_version, 28);
	ifp->if_version[28] = '\0';
	if_attach(ifp);
}

loinit(unit) {
	struct lo_softc *sc = &lo_softc;
	/* initialize the time */
	sc->ztime = time.tv_sec;
}

looutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	int s, size = 0;
	register struct ifqueue *ifq;
	struct mbuf *m;
	struct lo_softc *sc = &lo_softc;

	s = splimp();
	m0->m_ifp = ifp;
	m = m0;
	while (m != NULL) {  /* count the number of bytes */
		size += m->m_len;
		m = m->m_next;
	}
	ifp->if_opackets++;
	sc->ctrblk.est_bytesent += size;
	if (sc->ctrblk.est_bloksent != (unsigned) 0xffffffff)
	    sc->ctrblk.est_bloksent++;
	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		smp_lock(&ipintrq.lk_ifqueue, LK_RETRY);
		ifq = &ipintrq;
		if (IF_QFULL(ifq)) {
			IF_DROP(ifq);
			m_freem(m0);
			smp_unlock(&ipintrq.lk_ifqueue);
			splx(s);
			return (ENOBUFS);
		}
		IF_ENQUEUEIF(ifq, m0, ifp);
		schednetisr(NETISR_IP);
		smp_unlock(&ipintrq.lk_ifqueue);
		break;
#endif
#ifdef NS
	case AF_NS:
		smp_lock(&nsintrq.lk_ifqueue, LK_RETRY);
		ifq = &nsintrq;
		if (IF_QFULL(ifq)) {
			IF_DROP(ifq);
			m_freem(m0);
			smp_unlock(&nsintrq.lk_ifqueue);
			splx(s);
			return (ENOBUFS);
		}
		IF_ENQUEUE(ifq, m0);
		schednetisr(NETISR_NS);
		smp_unlock(&nsintrq.lk_ifqueue);
		break;
#endif
#ifdef APPLETALK
	case AF_APPLETALK:
		{
		extern struct ifqueue ddpintq;
		smp_lock(&ddpintq.lk_ifqueue, LK_RETRY);
		ifq = &ddpintq;
		if (IF_QFULL(ifq)){
			IF_DROP(ifq);
			m_freem(m0);
			smp_unlock(&ddpintq.lk_ifqueue);
			splx(s);
			return (ENOBUFS);
		}
		IF_ENQUEUEIF(ifq, m0, ifp);
		schednetisr(NETISR_DDP);
		smp_unlock(&ddpintq.lk_ifqueue);
		}
		break;
#endif APPLETALK
	default:
		splx(s);
		printf("lo%d: can't handle af%d\n", ifp->if_unit,
			dst->sa_family);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}
	ifp->if_ipackets++;
	sc->ctrblk.est_bytercvd += size;
	if (sc->ctrblk.est_blokrcvd != (unsigned) 0xffffffff)
	    sc->ctrblk.est_blokrcvd++;
	splx(s);
	return (0);
}

/*
 * Process an ioctl request.
 */
/* ARGSUSED */
loioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int error = 0, i;
	struct lo_softc *sc = &lo_softc;
	struct ifdevea *ifd = (struct ifdevea *)data;
	struct ctrreq *ctr = (struct ctrreq *)data;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		loinit(ifp->if_unit);
		/*
		 * Everything else is done at a higher level.
		 */
		break;

	case SIOCRPHYSADDR:
		error = EOPNOTSUPP;
	break;
		
	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		ctr->ctr_ether = sc->ctrblk;
		ctr->ctr_type = CTR_ETHER;
		ctr->ctr_ether.est_seconds = (time.tv_sec - sc->ztime) > 0xfffe ? 0xffff : (time.tv_sec - sc->ztime);

		if (cmd == SIOCRDZCTRS) {
			sc->ztime = time.tv_sec;
			bzero(&sc->ctrblk, sizeof(struct estat));
		}
	break;

	default:
		error = EINVAL;
	}
	return (error);
}
