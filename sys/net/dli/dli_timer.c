#ifndef	lint
static char *sccsid = "@(#)dli_timer.c	4.2	ULTRIX	9/4/90";
#endif	lint

/*
 * Program dli_timer.c,  Module DLI 
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

#include "../../net/netdnet/dli_var.h"

extern struct dli_timers lback_timers[];
extern struct lock_t lk_dli;

extern struct sockaddr_dl sysid_dst;
extern struct ether_pa *sysid_haddr_p;
extern struct dli_sysid_to sysid_to[];
extern u_short nqna;
extern u_char sysid_mcast[];
extern u_char sysid_msg[];
extern u_char *sysid_devtyp_p;





/*
 *		d l i _ s l o w t i m o
 *
 * Slow timeout routine is entered every 500ms.  If loopback timer is
 * nonzero, it is decremented.  If the loopback timers makes a transition
 * to zero, a "passive loopback terminated" message is forwarded to EVL.
 *
 * Returns:		Nothing
 *
 * Inputs:		None
 */
dli_slowtimo()
{
    register int i;
    register struct ifnet *ifp;
    int s = splnet();
    int saveaffinity;

    smp_lock(&lk_dli, LK_RETRY);
    for( i = 0; i < DLI_MAX_LBTIMR; i++ )
    {
	if ( lback_timers[i].tval != 0 )
		if ( --(lback_timers[i].tval) == 0 )
		{
			ifp = lback_timers[i].ifp;
			lback_timers[i].ifp = NULL;
			log_event(ifp, DLI_EVLOP_LBTER);
			if ( ifp->if_flags  & IFF_POINTOPOINT )
			{
				lback_timers[i].prev_devstate.if_next_family = lback_timers[i].prev_devstate.if_family;
				lback_timers[i].prev_devstate.if_family = AF_DLI;
				lback_timers[i].prev_devstate.if_xferctl = IFS_XFERCTL;
				lback_timers[i].prev_devstate.if_wrstate = IFS_WRSTATE;
				CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
				(*ifp->if_ioctl)(ifp, SIOCSTATE, &lback_timers[i].prev_devstate);
				RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			}
		}
    }

    for(i = 0; i < nqna; i++)    /* check all qna's */
    {
	if ( (sysid_to[i].ifp->if_flags & IFF_RUNNING) == IFF_RUNNING)
	{
	    if( !(sysid_to[i].ifp->if_flags & IFF_MOP) )
	    {
		if(--sysid_to[i].to == 0)        /* time to tx? */
		{
		    sysid_to[i].to = sysid_to[i].tr; /* reset timers */
		    dli_snd_sysid(sysid_to[i].ifp, 0, sysid_to[i].devtyp); /* transmit sysid */
		}
	    }
	}
    }
    smp_unlock(&lk_dli);
    splx(s);

    return;
}


/*
 *		d l i _ s n d _ s y s i d
 *
 * This routine is called to transmit the sysid for a particular qna
 *
 * Note:  Protocol lock must be asserted before routine called.
 *
 * Outputs:		None.
 *
 * Inputs:		ptr to qna who is to transmit it's sysid.
 *			receipt for the sysid message.
 *			device type value for the sysid message.
 *
 * Version History:
 *
 */
dli_snd_sysid(ifp, receipt, devtyp)
register struct ifnet *ifp;
u_short receipt;
u_char devtyp;
{

	int n;
	struct mbuf *m;
	register u_char *msgp;
	int saveaffinity;  /* for nonsmp drivers.  8.18.88.us */
	struct sockaddr_dl sysid_dstlc;

	MGET(m, M_DONTWAIT, MT_DATA);
	if( m )
	{
		u_int dev_idx = (u_int) ((u_char *) sysid_devtyp_p - sysid_msg);
		u_int hwa_idx = (u_int) ((u_char *) sysid_haddr_p - sysid_msg);
		struct ifdevea dev_addr;

		/* Build address structure. */
		bcopy(&sysid_dst, &sysid_dstlc, sizeof(sysid_dstlc));
		sysid_dstlc.dli_device.dli_devnumber = ifp->if_unit;

		/* Build sysid message. */
		msgp = mtod(m, u_char *);
		bzero(msgp, 46);		/* minimum size packet */
		bcopy(sysid_msg, msgp, SYSID_MSGL);
		smp_unlock(&lk_dli);
		msgp[dev_idx] = devtyp;
#ifdef IFT_FDDI
		if (ifp->if_sysid_type != 0)
			msgp[dev_idx] = ifp->if_sysid_type;
#endif
		*(u_short *) (msgp+4) = receipt;

		/* Get hardware address to store in sysid message. */
		CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
		(*ifp->if_ioctl)(ifp, SIOCRPHYSADDR, (caddr_t)&dev_addr);
		RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		bcopy(dev_addr.default_pa, &msgp[hwa_idx], sizeof(struct ether_pa));
		m->m_next = NULL;
		m->m_len = 46; 		/* remaining min siz pkt set to 0 */
		m->m_act = NULL;
		CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
		(*ifp->if_output)(ifp, m, &sysid_dstlc);
		RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		smp_lock(&lk_dli, LK_RETRY);
	}
	return;
}
