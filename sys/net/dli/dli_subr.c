#ifndef	lint
static char *sccsid = "@(#)dli_subr.c	4.4	(DECnet-ULTRIX)	4/30/91";
#endif

/*
 * Program dli_subr.c,  Module DLI 
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
 * 1.00 22-May-1986
 *      DECnet-ULTRIX   V2.0
 *
 * 2.01 18-Mar-1988
 *      DECnet-ULTRIX   V2.4
 *		- Allowed use of reserved bit in individual and group SAPs
 *
 *	29-Apr-1991	Matt Thomas
 *	staticly allocate m_ar array.  Fix gsap to seach all 128 isaps.
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

#include "../../h/user.h"
#include "../../h/buf.h"
#include "../../h/conf.h"
#include "../../h/proc.h"

#include "../../net/net/if.h"
#include "../../net/net/if_to_proto.h"

#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"


extern struct lock_t lk_dli;
extern struct if_isapt if_isapt[];
extern struct protosw *iftype_to_proto(), *iffamily_to_proto();

extern int xti_debug;

u_int dlidebug = 0;
#define printd if(dlidebug)printf

/*
 * initialize the system data structure
 * for 802.3 support.
 *
 * Note:  Since this is part of DLI init at boot time,
 *	  all necessary locks taken out before this
 *	  routine is called.
 */
osi_802init(ifp)
struct ifnet *ifp;
{
	static int k = 0;

	if(k >= MAX_BROADCSTDEV)
	{
		return(-1);
	}
	bzero(&if_isapt[k], sizeof(struct if_isapt));
	if_isapt[k++].ifp = ifp;
	return(NULL);
}



/*
 * enable an isap in the system wide table
 *
 * Note: DLI line table entry and socke locks
 *	 taken out before this routine called.
 */

osi_ena_802isap(ifp, so, socket)
struct ifnet *ifp;
struct sockaddr_802 *so;
struct socket *socket;
{
	int i, n, byte, error;
	u_char bit;

	if((so->eh_802.ssap & VSAP) || (so->eh_802.ssap >= 255) )
		return(EINVAL);
	n = so->eh_802.ssap>>VSAP;    /* throw out low bit */
	byte = n>>3;               /* byte in table = sap / 8 */
	bit = 1<<(n%8);            /* bit to set/clear = sap mod 8 */

	smp_lock(&lk_dli, LK_RETRY);
	for(i = 0; i < MAX_BROADCSTDEV; i++)
	{
		if(if_isapt[i].ifp != ifp)
			continue;
		if(if_isapt[i].so[n] != 0)
		{
			smp_unlock(&lk_dli);
			return(EADDRINUSE);
		}
		if(so->svc == TYPE1)  
			if_isapt[i].svc[byte] |= bit;       /* set class 1 svc */
		else
			if_isapt[i].svc[byte] &= ~bit;      /* clear for user svc */
		if_isapt[i].so[n] = socket;
		smp_unlock(&lk_dli);
		return(NULL);
	}
	smp_unlock(&lk_dli);
	return(ENOTFND);
}


/*
 * disable an isap in the system wide table
 * and any gsaps that this isap has enabled
 *
 * Note: DLI line table entry and socke locks
 *	 taken out before this routine called.
 */

osi_dis_802isap(ifp, sap, sop)
struct ifnet *ifp;
u_char sap;
struct socket *sop;
{
	int error, j, i, byte, bit, vsap;

	if( (sap & VSAP) || (sap >= 255) )
		return(EINVAL);

	vsap = sap >> VSAP;
	byte = vsap/8;
	bit = 1 << vsap%8;
	smp_lock(&lk_dli, LK_RETRY);
	for(i = 0; i < MAX_BROADCSTDEV; i++)
	{
		if(if_isapt[i].ifp != ifp)
			continue;
		if(if_isapt[i].so[vsap] == 0)
		{
			smp_unlock(&lk_dli);
			return(EINVAL);
		}
		if(if_isapt[i].so[vsap] != sop)
		{
			smp_unlock(&lk_dli);
			return(EINVAL);
		}
		if_isapt[i].so[vsap] = NULL;
		/* look for and disable any gsaps this isap has enabled */
		for(j = 0; j < NISAPS; j++)              
		{
			if(if_isapt[i].gsap[j][byte] & bit)
				if(error = osi_dis_802gsap(ifp, ((j<<VSAP)|1), sap))
				{
					smp_unlock(&lk_dli);
					return(error);
				}
		}
		smp_unlock(&lk_dli);
		return(NULL);
	}
	smp_unlock(&lk_dli);
	return(ENOTFND);
}


/*
 * enable gsap in the system wide table
 *
 * Note: DLI line table entry and socket locks
 *	 taken out before this routine called.
 */

osi_ena_802gsap(ifp, gsap, sap)
struct ifnet *ifp;
int gsap, sap;
{
	int i, byte, bit;

	if( (sap >= 255) || (sap & VSAP))
		return(EINVAL);
	if( (gsap <= NULL) || !(gsap & VSAP) || (gsap > 255) )
	    return(EINVAL);
	byte = (sap >> VSAP) / 8; 
	bit = (sap >> VSAP) % 8;

	smp_lock(&lk_dli, LK_RETRY);
	for(i = 0; i < MAX_BROADCSTDEV; i++)
	{
		if(if_isapt[i].ifp != ifp)
			continue;
		if_isapt[i].gsap[gsap>>VSAP][byte] |= 1 << bit;
		smp_unlock(&lk_dli);
		return(NULL);
	}
	smp_unlock(&lk_dli);
	return(ENOTFND);
}


/*
 * disable gsap in system wide table
 *
 * Note: DLI line table entry and socke locks
 *	 taken out before this routine called.
 */

osi_dis_802gsap(ifp, gsap, sap)
struct ifnet *ifp;
int gsap, sap;
{
	int i, byte, bit;
	u_char lk_dli_asserted_locally = 0;

	if( (sap >= 255) || (sap & VSAP))
		return(EINVAL);
	if( (gsap <= NULL) || !(gsap & 1) || (gsap > 255) )
	    return(EINVAL);
	byte = (sap >> VSAP) / 8;
	bit = (sap >> VSAP) % 8;

	if ( smp_owner(&lk_dli) == LK_FALSE )
	{
		smp_lock(&lk_dli, LK_RETRY);
		lk_dli_asserted_locally = 1;
	}
	for(i = 0; i < MAX_BROADCSTDEV; i++)
	{
		if(if_isapt[i].ifp != ifp)
			continue;
		if_isapt[i].gsap[gsap>>VSAP][byte]  &= ~(1<< bit);
		if ( lk_dli_asserted_locally )
			smp_unlock(&lk_dli);
		return(NULL);
	}
	if ( lk_dli_asserted_locally )
		smp_unlock(&lk_dli);
	return(ENOTFND);
}


/*
 * test if an isap has this gsap enabled in system wide table
 *
 * Note: DLI line table entry and socket locks
 *	 taken out before this routine called.
 */

osi_tst_802gsap(ifp, gsap, sap)
struct ifnet *ifp;
int gsap, sap;
{
	int i, byte, bit;

	if( (sap >= 255) || (sap & VSAP))
		return(EINVAL);
	if( (gsap <= 0) || !(gsap & 1) || (gsap > 255) )
	    return(EINVAL);

	byte = (sap >> VSAP) / 8;
	bit = (sap >> VSAP) % 8;

	smp_lock(&lk_dli, LK_RETRY);
	for(i = 0; i < MAX_BROADCSTDEV; i++)
	{
		if(if_isapt[i].ifp != ifp)
			continue;
		if(if_isapt[i].gsap[gsap>>VSAP][byte]  & (1<< bit))
		{
			smp_unlock(&lk_dli);
			return(1);
		}
		else
		{
			smp_unlock(&lk_dli);
			return(NULL);
		}
	}
	smp_unlock(&lk_dli);
	return(ENOTFND);
}


/*
 * enable an 802.3 protocol (5 byte field)
 * for someone using the snap sap
 *
 * Note: DLI line table entry lock
 *	 taken out before this routine called.
 *	 No socket locks must be asserted!
 */

osi_ena_802pi(so_802, ifp, ue)
register struct sockaddr_802 *so_802;
struct ifnet *ifp;
struct dli_line *ue;
{
	u_char dmy_pi[5];
	extern struct dli_line dli_ltable[];
	register int i, error;
	register struct sockaddr_802 *search_eaddr;
	u_char	flags = so_802->ioctl;

	bzero(dmy_pi, 5);
	if(bcmp(so_802->eh_802.osi_pi, dmy_pi, 5) == 0) 
		return(EINVAL);

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
	for ( i = 0; i < dli_maxline; i++ )
	{
		if ( &dli_ltable[i] == ue )
			continue;
		smp_lock(&dli_ltable[i].dli_lk, LK_RETRY);
		if (dli_ltable[i].dli_lineid.dli_substructype != DLI_802 ||
			dli_ltable[i].dli_so == NULL ||
			ifp != dli_ltable[i].dli_if )
		{
			smp_unlock(&dli_ltable[i].dli_lk);
			continue;
		}
		search_eaddr = &dli_ltable[i].dli_lineid.choose_addr.dli_802addr;
		error = NULL;
		switch (search_eaddr->ioctl)
		{
			case NULL:
				break;

			case DLI_EXCLUSIVE:
				error = (bcmp(search_eaddr->eh_802.osi_pi, so_802->eh_802.osi_pi, 5) == NULL) ? EADDRINUSE : NULL;
				break;

			case DLI_NORMAL:
				switch (flags)
				{
					case DLI_EXCLUSIVE:
						error = (bcmp(search_eaddr->eh_802.osi_pi, so_802->eh_802.osi_pi, 5) == NULL) ? EADDRINUSE : NULL;
						break;

					case DLI_NORMAL:
						if(bcmp(search_eaddr->eh_802.osi_pi, so_802->eh_802.osi_pi, 5) == NULL)
						{
							error = match_targets(search_eaddr->eh_802.dst, so_802->eh_802.dst);
						}
						else 
						{
							error = NULL;
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
						error = (bcmp(search_eaddr->eh_802.osi_pi, so_802->eh_802.osi_pi, 5) == NULL) ? EADDRINUSE : NULL;
						break;

					default:
						break;
				}
				break;


			default:
				panic( "dli_bind: osi_ena_802pi:" );
				break;
		}
		smp_unlock(&dli_ltable[i].dli_lk);
		if (error != NULL)
		{
			return(error);
		}
	}
	return(NULL);

}


/* 
 * build an 802 header for transmission
 */

struct mbuf *
osi_buildhdr(dst_addr)
struct sockaddr_dl *dst_addr;
{
	struct mbuf *temp, *m = NULL;
	u_char *cp;

	MGET(m, M_DONTWAIT, MT_DATA);
	if ( m )
	{
		cp = mtod(m,u_char *);
		PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.dsap);
		PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.ssap);
		if((dst_addr->choose_addr.dli_802addr.eh_802.ctl.U_fmt & 3) == 3)
		{
			PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.ctl.U_fmt);
			if(dst_addr->choose_addr.dli_802addr.eh_802.ssap == SNAP_SAP)
			{
				PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.osi_pi[0]);
				PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.osi_pi[1]);
				PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.osi_pi[2]);
				PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.osi_pi[3]);
				PUT8B(cp, dst_addr->choose_addr.dli_802addr.eh_802.osi_pi[4]);
				m->m_len = 8;
			}
			else
				m->m_len = 3;
		}
		else
		{
			PUT16B(cp, dst_addr->choose_addr.dli_802addr.eh_802.ctl.I_S_fmt);
			m->m_len = 4;
		}
	}
	return(m);
}


/*
 * handle 802 incoming packets
 * otherwise drop packet 
 * otherwise drop packet 
 * pull any header information from the 2nd mbuf to
 * guarantee that only data is there
 *
 * input packet as a chain of mbufs, first mbuf has garbage
 * 2nd mbuf will have the rest of the 802 header 
 * return nothing
 */

osi_802input(m, rcv)
struct mbuf *m;
struct dli_recv *rcv;
{
    u_char *cp, bit; 
    int len, cnt, byte, i, j, k;

    struct osi_802hdr *eh;
    struct ifnet *ifp;
    /*
     * This structure is protected by the dliintrq queue ownership code
     * in dliintr().  That code assures that only one cpu at a time will
     * process input packets.
     */
    static struct {              /* used for gsaps to make copies of packet */
    	int sap;
    	struct mbuf *m;
    	struct socket *so;
    } m_ar[NISAPS];
    register struct socket *so = NULL;
    u_char svc;
    struct dli_line *ue, *dli_getlte_notrust();

    eh = &rcv->rcv_hdr.osi_header;
#ifdef IFT_FDDI
    eh->len = m_length(m->m_next);
#else
    len = m_length(m->m_next);
    if ( len != eh->len && len != DLI_MINPKT )
    {
    	printd("osi_802input: invalid length, indicated = %d, actual = %d\n", eh->len, len);
        goto dropit;
    }
    else
    {
	if ( eh->len < len && len == DLI_MINPKT )
	    m_adj(m->m_next, -(len - eh->len));
    }
#endif

    cp = mtod(m->m_next, u_char *);   /* remove up to ctl fld */
    eh->dsap = *cp++;
    eh->ssap = *cp++;
    m_adj(m->m_next, 2);     
    eh->len -= 2;

    for(i = 0; i < MAX_BROADCSTDEV; i++)   /* look for matching device */
    {
	/*
	 * Don't need to take out lock since this member is
	 * read-only after the system is booted.
	 */
	if(if_isapt[i].ifp != rcv->rcv_ifp)
	    continue;

	/* grab possible socket and service class info now */
	if ( (eh->dsap & 1) == NULL && eh->dsap != NULL && eh->dsap != SNAP_SAP )
	{
	    smp_lock(&lk_dli, LK_RETRY);
	    svc = if_isapt[i].svc[(eh->dsap>>4)];
	    so = if_isapt[i].so[eh->dsap>>VSAP];
	    smp_unlock(&lk_dli);
	}

	if(eh->dsap == NULL || eh->dsap == SNAP_SAP)
	{
	    /*
	     * special case is DSAP = NULL or SNAP_SAP 
	     */
	    if(((eh->ctl.U_fmt = (u_char)*cp++)& 3) != 3)
		goto dropit;
	    m_adj(m->m_next, 1);      /* pull ctrl field */
	    --eh->len;
	    if(eh->ctl.U_fmt == UI_NPCMD)  /* save pi for snap */
	    {
		if(eh->dsap == NULL)
		    goto dropit;
		for(i = 0; i < 5; i++)
		    eh->osi_pi[i] = *cp++;
		m_adj(m->m_next, 5);      /* pull protocol */
		eh->len -= 5;
	    }
	    osi_proc_ctl(m,so,rcv);
	    return;
	}
	else if(so != NULL )
	{
	    /* handle DSAP = isaps */
	    bit = 1 << ( (eh->dsap>>VSAP) % 8);
	    if(svc & bit)
	    { 
		/* for class 1 service get ctl field */
		if( (eh->ctl.U_fmt = (u_char)*cp) & 3 != 3)
		    goto dropit;
		m_adj(m->m_next, 1);    
		--eh->len;
		osi_proc_ctl(m,so,rcv);
		return;
	    }
	    else
	    {
		/*
		 * must be users supplied service, just pass it all up,
		 * but leave ctl in with data
		 */
		switch(*cp & 3)
		{
		    case 0:        /* information */
		    case 1:        /* supervisory */
		    case 2:        /* information */
			eh->ctl.I_S_fmt = (u_short)*cp;
			break;
		    case 3:        /* unnumbered */
			eh->ctl.U_fmt = (u_char)*cp;
			break;
		    default:
			goto dropit;
		}
			   
		if ( (ue = dli_getlte_notrust(so)) != NULL )
		{
		    found_user(m, so, DLI_802, rcv);
		    smp_unlock(&so->lk_socket);
		    smp_unlock(&ue->dli_lk);
		}
		else
		    m_freem(m);
		return;
	    }
	}
	else if( eh->dsap & 1 )  
	{
	    /*
	     * The only thing left should be gsaps in user supplied mode.
	     * Try and get a copy of the packet for each sap that has the 
	     * gsap enabled. If any fail, free any copies and the original 
	     */
	    smp_lock(&lk_dli, LK_RETRY);
	    if ( ffs(*(long *) &if_isapt[i].gsap[(eh->dsap>>VSAP)][0]) || 
		  	  ffs(*(long *) &if_isapt[i].gsap[(eh->dsap>>VSAP)][4]) || 
		 	  ffs(*(long *) &if_isapt[i].gsap[(eh->dsap>>VSAP)][8]) || 
			  ffs(*(long *) &if_isapt[i].gsap[(eh->dsap>>VSAP)][12]) )
	    {
		/* search gsap table for any isaps with this gsap enabled */
		bit = 0;
		cnt = 0;
		for(j = 0; j < NISAPS / 8; j++)
		{
		byte = if_isapt[i].gsap[eh->dsap>>VSAP][j];
		    /* if bit is set, bit pos = isap with this gsap enabled */
		    for(k = 0; k < 8; k++)
		    {
			if(byte & 1)
			{
			    if(!(m_ar[cnt].m = m_copy(m, 0, M_COPYALL))){
				while(--cnt >= 0)
				    m_freem(m_ar[cnt].m);
				smp_unlock(&lk_dli);
				goto dropit;                 /* drop original */
			    }
			    m_ar[cnt].so = if_isapt[i].so[bit];
			    m_ar[cnt++].sap = bit;
			}
			++bit;
			byte >>= 1;
		    }
		}

		smp_unlock(&lk_dli);
		m_freem(m->m_next);             /* done with original */
		while(--cnt >= 0) {
		    if ( (ue = dli_getlte_notrust(m_ar[cnt].so)) != NULL )
		    {
			found_user(m_ar[cnt].m, m_ar[cnt].so, DLI_802, rcv);
			smp_unlock(&(m_ar[cnt].so->lk_socket));
			smp_unlock(&ue->dli_lk);
		    }
		    else
			m_freem(m_ar[cnt].m);
		}
		m_free(m);
		return;
	    }
	    else
	    	smp_unlock(&lk_dli);
	}
	goto dropit;
    }

dropit:
    if(m)
	m_freem(m);
    return;
}


/*
 * process the control field on incoming packets
 * for class 1 service only
 * the packet header must have been adjusted by now
 */

osi_proc_ctl(m,so, rcv)
struct mbuf *m;
struct socket *so;
struct dli_recv *rcv;
{
    u_char dsap;
    int len;
    struct dli_line *ue, *dli_getlte_notrust();

    dsap = rcv->rcv_hdr.osi_header.dsap;

    /* only ctl allowed for class 1 */

    switch(rcv->rcv_hdr.osi_header.ctl.U_fmt)   
    {
	case XID_PCMD:
	case XID_NPCMD:
	case TEST_PCMD:
	case TEST_NPCMD:
	    if( rcv->rcv_hdr.osi_header.ssap & 1) /* got a response */
	    {
		switch(dsap)
		{
		    case NULL:   /* nobody to pass response to */
		    case SNAP_SAP:
			if(m) m_freem(m);
			return;

		    default:
			if ( (ue = dli_getlte_notrust(so)) != NULL )
			{
			    found_user(m, so, DLI_802, rcv);
			    smp_unlock(&ue->dli_lk);
			    smp_unlock(&so->lk_socket);
			}
			else
			{
			    if ( m )
			    	m_freem(m);
			}
			return;
		}
	    }
	    else                     /* command, gotta respond for these guys */
	    {
		osi_rspndr(m, rcv);
		return;
	    }

	case UI_NPCMD:                /* only type of ui */
	    switch (dsap)
	    {
		case NULL:
			if(m) m_freem(m);
			return;

		case SNAP_SAP:
			forward_to_user(m, rcv);
			return;

		default:              /* got somebody */
			if ( (ue = dli_getlte_notrust(so)) != NULL )
			{
			    found_user(m, so, DLI_802, rcv);
			    smp_unlock(&ue->dli_lk);
			    smp_unlock(&so->lk_socket);
			}
			else
			{
			    if ( m )
			    	m_freem(m);
			}
			return;
	    }

	default:                      /* throw it out */
	    if(m) m_freem(m);
	    return;
    }  
}


/* 
* respond to xid or test packet for class 1
* service, for the null sap or the snap sap only
* 
* input	packet as a chain of mbuf's, 1st mbuf garbage
* return nothing, attempt is made to tx packet (no guarantees)
*/

osi_rspndr(m, rcv)
struct mbuf *m;
struct dli_recv *rcv;
{
	u_char *cp;
	u_short len;
	struct osi_802hdr *eh = &rcv->rcv_hdr.osi_header;
	struct sockaddr_dl out_addr; 
	struct mbuf *m0, *temp, *osi_buildhdr();
	struct ifnet *ifp = rcv->rcv_ifp;
	int saveaffinity;

	/* got to do this for output routine */
	bzero(&out_addr, sizeof(struct sockaddr_dl));
	out_addr.dli_family = AF_DLI;
	out_addr.dli_substructype = DLI_802;
	out_addr.choose_addr.dli_802addr.svc = TYPE1;
	out_addr.dli_device.dli_devnumber = ifp->if_unit;
	len = strlen(ifp->if_name);
	bcopy(ifp->if_name, out_addr.dli_device.dli_devname, len);
	bcopy(rcv->rcv_hdr.osi_header.src, out_addr.choose_addr.dli_802addr.eh_802.dst, DLI_EADDRSIZE);

	/* return it with the response bit set */
	out_addr.choose_addr.dli_802addr.eh_802.ssap = eh->dsap | (u_char)1; 
	out_addr.choose_addr.dli_802addr.eh_802.dsap = eh->ssap;

	switch(eh->ctl.U_fmt & 0xEF)
	{
		case XID:
			if(eh->len != 3)
				goto bad;
			cp = mtod(m, u_char *);
			PUT8B(cp, 0x81);              
			PUT8B(cp, 0x01);
			PUT8B(cp, NULL);
			m->m_len = 3;
			m_freem(m->m_next);          /* don't need the rest */
			m->m_next = NULL;
			break;
		case TEST:   /* just turn it around, they may have sent data */
			m = m_free(m);         /* 1st mbuf has garbage */
			break;
		default:
			goto bad;
	}

	/*
	 * just have to return the control field
	 */
	out_addr.choose_addr.dli_802addr.eh_802.ctl.U_fmt = eh->ctl.U_fmt;
	if(m0 = osi_buildhdr(&out_addr))
	{
		m0->m_next = m;
		CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
		(*ifp->if_output)(ifp, m0, &out_addr); 
		RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		return;
	}

bad:
	if(m)
	    m_freem(m);
	return;
}



/*
 *		d l i _ g e t l t e _ n o t r u s t 	
 *
 * This routine returns a line table entry as a function
 * of a socket.  The line table entries are individually
 * inspected since we don't trust the socket pointer.
 * In addition, before returning, the locks for the line table 
 * entry and the socket are asserted if the routine 
 * succeeds.
 *
 * Note:  The socket lock musn't be asserted!
 *
 * Outputs:		Nothing.
 *
 * Inputs:		so = pointer to a socket.
 *
 * Returns:		pointer to a line table entry if success,
 *			otherwise NULL is returned.
 *
 */
struct dli_line *dli_getlte_notrust(so)
register struct socket *so;
{
	register int i;
	register struct dli_line *ue = NULL;

	if ( so == NULL )
		return(NULL);

	for ( i = 0; i < dli_maxline; i++ )
	{
		smp_lock(&dli_ltable[i].dli_lk, LK_RETRY);
		if ( dli_ltable[i].dli_so == so )
		{
			ue = &dli_ltable[i];
			smp_lock(&(dli_ltable[i].dli_so->lk_socket), LK_RETRY);
			break;
		}
		smp_unlock(&dli_ltable[i].dli_lk);
	}
	return(ue);
}



/*
 *		d l i _ g e t l t e _ t r u s t 	
 *
 * This routine returns a line table entry as a function
 * of a socket.  The line table entry is storeed as a
 * back pointer in the socket structure.  Since this is used,
 * the socket must be locked before calling this routine.
 * In addition, before returning, the locks for the line table 
 * entry and the socket are asserted if routine succeeds. 
 * If routine fails, nothing is asserted.
 *
 * Note:  The socket lock must be asserted!
 *
 * Outputs:		Nothing.
 *
 * Inputs:		so = pointer to a socket.
 *
 * Returns:		pointer to a line table entry, if valid,
 *			otherwise NULL.
 *
 */
struct dli_line *dli_getlte_trust(so)
register struct socket *so;
{
	register struct dli_line *ue = (struct dli_line *) so->so_pcb;

	if ( so == NULL )
		return(NULL);

	smp_unlock(&so->lk_socket);
	smp_lock(&ue->dli_lk, LK_RETRY);
	if ( ue->dli_so == so )
	{
		smp_lock(&so->lk_socket, LK_RETRY);
		return(ue);
	}
	else
	{
		smp_unlock(&ue->dli_lk);
		return(NULL);
	}
}



/*
 *		m a t c h _ m c a s t
 *
 * This routine looks at the destination address, and if it is
 * multicast, it checks to see if the address matches the user's
 * multicast addresses.  Note that if the user hasn't specified
 * any, the test fails.
 *
 * Outputs:		NULL if no match, otherwise 1.
 *
 * Inputs:		mcast1 = string containing Ethernet dest address.
 * 			mcast2 = string containing user's multicast address(es).
 */
match_mcast( mcast2, mcast1 )
register u_char *mcast2, *mcast1;
{
	register int i;

	/*
	 * If destination addr not multicast, pass test.
	 */
	
	if ( !(mcast1[0] & 1) )
	{
		return(1);
	}
	/*
	 * If multicast, perform test.
	 */
	for( i = 0; i < MCAST_ASIZE; i += MCAST_SIZE)
	{
		if ( bcmp(mcast1, mcast2+i, MCAST_SIZE) == NULL )
		{
			return(1);
		}
	}

	
	return(NULL);

}





/*
 *		m a t c h _ t a r g e t s
 *
 * check to see if a pair of ethernet addresses match
 *
 * Returns:		Error code if match occurs, otherwise NULL.
 *
 * Inputs:
 *	target1		= first ethernet address
 *	target2		= second ethernet address
 */
match_targets(target1, target2)
u_char *target1, *target2;
{

	if ( bcmp(target1, target2, DLI_EADDRSIZE) == NULL )
	{
		return(EADDRINUSE);
	}
	return(NULL);
}




#ifndef IFT_FDDI
/*
 *		m _ l e n g t h
 *
 * Compute the number of bytes in a (non-empty) MBUF chain.
 *
 * Returns:		The number of bytes in the chain.
 *
 * Inputs:
 *	m		= Pointer to the MBUF chain.
 */
m_length( m )
register struct mbuf *m;
{
    register int len = 0;

    if (m) do
    {
	len += m->m_len;
    }
    while (m = m->m_next);

    return (len);
}
#endif
