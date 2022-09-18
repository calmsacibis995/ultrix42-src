#ifndef lint
static char *sccsid = "@(#)lat_usrreq.c	6.2	(ULTRIX)	1/28/88";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86,87 by			*
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
 * 12-11-87     Robin L. and Larry C.					*
 *      Added new kmalloc memory allocation to system.			*
 *									*
 *	koehler - 09/11/86						*
 *		changed the namei interface				*
 *                                                                      *
 *	Larry Cohen  -	09/16/85					*
 * 		protosw and ETHERTYPE changes caused by subnet routing  *
 *									*
 *	Peter Harbo - 4/15/86						*
 *		Addition of LAT 5.1 support: LAT socket ioctls LIOCSOL, *
 *		LIOCRES, LIOCCMD come through soo_ioctl() - if_ioctl -  *
 *		PRU_CONTROL of lat_usrreq for sending solicit info mgs, *
 *		response information msgs, sending command msgs.        *
 *                                                                      *
 *	Chung Wong - 7/17/87                                            *
 *		Check if LST_RUNNING for ioctl LIOCSOL.                 *
 *                                                                      *
 ************************************************************************/

/*	lat_usrreq.c	0.0	12/11/84	*/
/*	lat_usrreq.c	1.0	4/15/86		*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/gnode.h"
#include "../h/kmalloc.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_var.h"
#include "../lat/lat_protocol.h"
#include "../h/ioctl.h"

extern struct lat_slot sl[];
extern u_char sockactive;
extern struct lat_counters latctrs;

extern struct sclass *sclass[];
extern struct ecb statable[];
extern u_short reqid;
extern struct sclass class1;
extern u_short mtimer;

/*
 * LAT control socket support routines.
 */

 /*
  *		l a t _ u s r r e q
  *
  * Process a LAT user request.
  *
  * Returns:		0 if success
  *			error code if not
  *
  * Inputs:
  *	so		= Pointer to the socket for this request.
  *	req		= Request function code.
  *	m		= Pointer to an MBUF chain.
  *	nam		= Pointer to an MBUF chain for addressing.
  *	rights		= Pointer to an MBUF chain for access rights.
  */
/*ARGSUSED*/
lat_usrreq( so,req,m,nam,rights )
register struct socket *so;
int req;
struct mbuf *m,*nam,*rights;
{
    int error = 0,class,size = CLBYTES,len,i,cmd;
    register caddr_t dptr;
    register struct lat_params *pptr;
    register struct lat_state *sptr;
    register struct sclass *scl;
    register struct lat_slot *slot;
    register struct mbuf *dm;
    register struct direct_1 *dirptr;
    int s = splnet();

    char *rp;
    caddr_t rptr, commsg, p, solmsg, portp;
    struct mbuf *prev, *n, *m0, *m1, *cm, *m_get(), *m_copy();
    struct response_1 *res, *res_que;
    struct ifnet *ifp;
    struct lat_cmd *commsgp;
    struct lataddr objaddr;
    struct ecb *ecbp;
    u_char u_len, cmdbyte;
    int unit;
    struct nameidata nd, *ndp = &nd;
    struct gnode g, id, *gp = &g, *gfs_namei();

    switch (req)
    {
	/*
	 * The LAT control program (lcp) creates a socket to perform its
	 * control functions. Note that only one control socket may be active
	 * at any time.
	 */
	case PRU_ATTACH:
	    so->so_pcb = (caddr_t)-1;
	    break;

	/*
	 * Detach is called to break the association between the socket and
	 * the LAT protocol.
	 */
	case PRU_DETACH:
	    so->so_pcb = 0;
	    break;

	/*
	 * The following requests are not supported for the LAT domain.
	 */
	case PRU_BIND:
	case PRU_LISTEN:
	case PRU_CONNECT:

	case PRU_SHUTDOWN:
	case PRU_DISCONNECT:
	case PRU_ACCEPT:
	case PRU_PEERADDR:
	case PRU_RCVD:
	case PRU_SEND:
	case PRU_ABORT:
	case PRU_RCVOOB:
	case PRU_SENDOOB:
	case PRU_SOCKADDR:
	case PRU_CONNECT2:
	case PRU_SENSE:
	    error = EOPNOTSUPP;
	    break;
	/*
	 * PRU_CONTROL is called for socket ioctl's.  The control flow is:
	 * ioctl() [sys_generic.c] calls soo_ioctl() [sys_socket.c] calls
	 * ifioctl() [net/if.c] calls (pr->pr_usrreq) (i.e., this for LAT
	 * family.)
	 */
	case PRU_CONTROL:
	    cmd = (int)m; 
	    switch ( cmd ) 
	    {
	        case LIOCSOL:
                    if (sclass[1]->scl_state != LST_RUNNING)
                    {
                        error = ENOPROTOOPT;
                        break;
                    }
		    if (nam)
		    {
			if (m0 = m_get(M_DONTWAIT, MT_DATA))
	                    solmsg = mtod(m0,caddr_t);
			else {
			    error = ENOBUFS;
			    break;
		        }
			p = (caddr_t)nam;
			/*
			 * Skip over interface name.
			 */
			while ( *p++ );
			
	                bcopy(p,solmsg,sizeof (struct solicit_1));
			m0->m_len = sizeof(struct solicit_1);
			class1.scl_solicit(m0,(struct ifnet *)rights);
			bcopy(mtod(m0,caddr_t),(caddr_t)nam,
			      sizeof(struct solicit_1));

		    }
	            break;
	    	case LIOCCMD:
		    
		    if (m0 = m_get(M_DONTWAIT,MT_DATA))
		        commsgp = mtod(m0,struct lat_cmd *);
		    else 
		    {
		        error = ENOBUFS;
		        break;
		    }
		    p = (caddr_t)nam;

		    /*
		     * Skip over interface name.
		     */
		    while ( *p++ );

		    /*
		     * Copy Ethernet address of object node.
		     */
		    bcopy(p,(caddr_t)objaddr.lat_addr,6);
       		    p += 6;

		    /*
		     * Store size of data to be copied directly from user data
		     * in *nam space to command message.
		     */
		    len = *(u_short *)p, p += 2;

		    /*
		     * Build command message header.
		     */
		    commsgp->lcm_type = MSG_CMD << 2;
		    commsgp->lcm_protofmt = 0;
		    commsgp->lcm_Hver = commsgp->lcm_Cver = LAT_VER;
		    commsgp->lcm_Lver = LAT_VER_LOW;
		    commsgp->lcm_eco = LAT_ECO;
		    *(u_short *)commsgp->lcm_framesize = 1518;
		    *(u_short *)commsgp->lcm_reqid = reqid;

		    /*
		     * Copy the reqid to the user data area.
		     */
		    *(u_short *)nam = reqid;

    		    /* 
		     * Increment reqid, which must not be 0.
		     */
		    reqid = ( (reqid == 0xffff) ? 1 : reqid + 1);
		    *(u_short *)commsgp->lcm_entryid = (u_short)0;

		    commsg = (caddr_t)((int)commsgp);
		    commsg += 12;

		    /* 
		     * Copy command type and command modifier from
		     * user structure.
		     */
		    *commsg++ = *p++;
		    *commsg++ = *p++;

		    /*
		     * Copy object node name from user data.
		     */
		    u_len = *commsg++ = *p++;
		    bcopy(p,commsg,(int)u_len);
		    commsg += u_len;
		    p += u_len;

		    /*
		     * Skip over subj. port length
		     */
		    portp = p + 1;    

                    KM_ALLOC(ndp->ni_dirp,char *,MAXPATHLEN,KM_NAMEI,KM_NOARG);
                    if(ndp->ni_dirp == NULL) {
                        error = EIO;
                        break;
                    }
 		    if(u.u_error = copyinstr(portp, ndp->ni_dirp, MAXPATHLEN,
		    (u_int *) 0)) {
			error = EFAULT;
			goto release;
	  	    }
		    ndp->ni_nameiop = LOOKUP;
		    if ( (gp = gfs_namei(ndp)) == 0 )
		    {
			error = EINVAL;
			m_free(m0);
			goto release;
		    }
		    else
		    {
			unit = minor(gp->g_rdev);
			ecbp = &(statable[unit]);
			gput(gp);
			if (ecbp->ecb_inuse)
			{
			    m_free(m0);
			    error = EADDRINUSE ;
			    goto release;
			}
			else
			{
			    ecbp->ecb_inuse = ECB_INUSE;
			    ecbp->ecb_reqid = 
			       *(u_short *)commsgp->lcm_reqid;
			    ecbp->ecb_if = (struct ifnet *)rights;
			    bcopy((caddr_t)objaddr.lat_addr,
				(caddr_t)(ecbp->ecb_addr.lat_addr),6);
			    ecbp->ecb_addr.lat_family = AF_LAT;
			}
		    }
		    
		    /*
		     * Add source node group identifier list, node name
		     * directly from class1.scl_dmsg
		     */

		    dptr = mtod(class1.scl_dmsg,caddr_t);
		    dptr = (caddr_t)dptr + sizeof(struct direct_1);
		    u_len = *commsg++ = *dptr++ ;
		    (void)bcopy((caddr_t)dptr,commsg,(int)u_len);
		    dptr += u_len, commsg += u_len;
		    u_len = *commsg++ = *dptr++;
		    (void)bcopy((caddr_t)dptr,commsg,(int)u_len);
		    commsg += u_len;
		    m0->m_len = (short)(commsg - (caddr_t)commsgp);
			
		    /*
		     * Allocate another mbuf and send the remaining data from
		     * the user message.
		     */
		    if (m1 = m_get(M_DONTWAIT,MT_DATA))
		        commsg = mtod(m1,caddr_t);
		    else {
		        m_freem(m0);
		        error = ENOBUFS;
			goto release;
		    }
		    
		    (void)bcopy(p,commsg,len);
		    m1->m_len = (short)len;
		    objaddr.lat_family = AF_LAT;

		    ifp = (struct ifnet *)rights;

		    /*
		     * Join the two mbufs and send to the interface
		     * pointed to by rights.
		     */
		    m_cat(m0,m1);
		    ecbp->ecb_cmdmsg = m0;
		    ecbp->ecb_inuse = 0;
		    if (cm = m_copy(m0,0,M_COPYALL))
		    {
			(*ifp->if_output)(ifp,cm,&objaddr);
		    }
release:
		    KM_FREE(ndp->ni_dirp, KM_NAMEI);
		    break;

	        case LIOCRES:
		    if (nam)
		    {
		        rptr = (caddr_t)nam;
			/*
			 * Skip over interface name.
			 */
			while ( *rptr++ );
			res = (struct response_1 *)rptr;
		        n = class1.scl_rmsg.q_head;

		        /*
		         * Find response information structure in response
			 * queue matching solicit ID of user structure
		         */

			while (n) {
			    res_que = mtod(n, struct response_1 *);
			    if ( *(u_short *)res->rs1_solid ==
			         *(u_short *)res_que->rs1_solid )
			    {
			        if (n == class1.scl_rmsg.q_head)
				{
			         if ( (class1.scl_rmsg.q_head = n->m_act) == 0)
				   class1.scl_rmsg.q_tail = 0;
				}
			        else 
				{
				   if (n == class1.scl_rmsg.q_tail)
				   {
				     class1.scl_rmsg.q_tail = prev;
				     prev->m_act = 0;
				   }
				   else
				     prev->m_act = n->m_act;
				}
			        rp = mtod(n,char *);
			        (void)bcopy(rp,(caddr_t)nam,n->m_len);
			        m_freem(n);
			        goto found;
 			    }
			    else
			    {
			        prev = n;
			        n = n->m_act;
			    }
		        } /* while n */

		        /*
		         * The address was not found.
		         */
		        error = EADDRNOTAVAIL;

		    } /* if nam */
found:
		    break;

		case LIOCINI:
		/* Designate the tty port in the first argument as 
		 * available only for host-initiated connections.
		 * The first cmdbyte has the following significance.
		 * 0 : Make available to server connections only.
		 * 1 : Make available to host-initiated connections only.
		 * 2 : Return in cmdbyte 0 or 1 to indicate current setting.
		 */

		    if (nam)
		    {
			p = (caddr_t)nam;
			/*
			 * Skip over interface name.
			 */
			while ( *p++ );
			cmdbyte = *p++;

                        KM_ALLOC(ndp->ni_dirp,char *,MAXPATHLEN,KM_NAMEI,KM_NOARG);
                        if(ndp->ni_dirp == NULL) {
                            error = EIO;
                            goto release2;
                        }
 			if(error = copyinstr(p, ndp->ni_dirp, MAXPATHLEN,
			(u_int *) 0)) {
			    goto release2;
			}

			ndp->ni_nameiop = LOOKUP;
			if ( (gp = gfs_namei(ndp)) == 0 )
			{
			    error = EINVAL;
			    goto release2;
		        }
		        else
		        {
			    unit = minor(gp->g_rdev);
			    ecbp = &(statable[unit]);
			    gput(gp);
			    if (cmdbyte != '\002')
			    	ecbp->ecb_hostinit = cmdbyte;
			    bcopy((caddr_t)&(ecbp->ecb_hostinit),
				(caddr_t)nam,1);
			}
		    }
release2:
		    KM_FREE(ndp->ni_dirp,KM_NAMEI);
		    break;
	        default:
		    break;
	    };
	    break;
	/*
	 * The get socket option call is used to retrieve information about
	 * the current LAT state.
	 */
	case PRU_GETSOCKOPT:
	    if (m)
	    {
		class = ((int)nam >> 8) & 0377;
		switch ((int)nam & 0377)
		{
		    /*
		     * Read the LAT counters.
		     */
		    case LAT_COUNTERS:
			bcopy((char *)&latctrs,(char *)mtod(m, caddr_t),
			      sizeof(struct lat_counters));
			m->m_len = sizeof(struct lat_counters);
			break;

		    /*
		     * Get LAT parameters.
		     */
		    case LAT_PARAMS:
			pptr = mtod(m, struct lat_params *);
			pptr->lpm_version = LAT_VER;
			pptr->lpm_eco = LAT_ECO;
			pptr->lpm_mtimer = mtimer;
			m->m_len = sizeof(struct lat_params);
			break;

		    /*
		     * Get LAT directory message.
		     */
		    case LAT_DIRMSG:
			if (class <= MAXCLASS && (scl = sclass[class]))
			{
			    dptr = mtod(m,caddr_t);
			    if (dm = scl->scl_dmsg)
			    {
				while (dm && size)
				{
				    len = size > dm->m_len ? dm->m_len : size;
				    bcopy(mtod(dm, caddr_t), dptr, len);
				    dptr += len;
				    size -= len;
				    dm = dm->m_next;
				}
			    }
			    m->m_len = CLBYTES - size;
			}
			else
			{
			    error = ERANGE;
			}
			break;

		    /*
		     * Get LAT state.
		     */
		    case LAT_STATE:
			if (class <= MAXCLASS && (scl = sclass[class]))
			{
			    sptr = mtod(m, struct lat_state *);
			    sptr->lst_state = scl->scl_state;
			    m->m_len = sizeof(struct lat_state);
			}
			else
			{
			    error = ERANGE;
			}
			break;

		    default:
			error = EOPNOTSUPP;
		}
	    }
	    else
	    {
		mprintf("lat err: getsockopt\n");
		error = EMSGSIZE;
	    }
	    break;

	/*
	 * The set socket option call is used to set up information about
	 * the current LAT state.
	 */
	case PRU_SETSOCKOPT:
	    if (suser())
	    {
	        if (sockactive)
		{
		    error = EBUSY;
	/*
	 *	    if (m)
	 *	    {
	 *		m_freem(m);
	 *	    }
	 */
		    break;
		}
		else sockactive++;
		class = ((int)nam >> 8) & 0377;
		switch ((int)nam & 0377)
		{
		    /*
		     * Zero the LAT counters.
		     */
		    case LAT_COUNTERS:
			bzero(&latctrs, sizeof(struct lat_counters));
			break;

		    /*
		     * Set LAT parameters.
		     */
		    case LAT_PARAMS:
			if (m && m->m_len >= sizeof(struct lat_params))
			{
			    pptr = mtod(m, struct lat_params *);
			    mtimer = pptr->lpm_mtimer;
			}
			else
			{
			    mprintf("lat error: LAT_PARAMS\n");
			    error = EMSGSIZE;
			}
			break;

		    /*
		     * Set LAT directory message.
		     */
		    case LAT_DIRMSG:
			if (class <= MAXCLASS && (scl = sclass[class]))
			{
			    if ((m == 0) || (dm = m_copy(m, 0, M_COPYALL)))
			    {
				if (scl->scl_dmsg)
				{
				    m_freem(scl->scl_dmsg);
				}
				else
				{
				    /*
				     * Initialise the incarnation number of
				     * the new service class to a 'random'
				     * number - the low byte of the seconds
				     * of the current time.
				     */
				    dirptr = mtod(dm, struct direct_1 *);
				    dirptr->dr1_inc = time.tv_sec;
				}
				scl->scl_dmsg = dm;
			    }
			    else
			    {
				error = ENOBUFS;
			    }
			}
			else
			{
			    error = ERANGE;
			}
			break;

		    /*
		     * Set LAT state.
		     */
		    case LAT_STATE:
			if (class <= MAXCLASS && (scl = sclass[class]))
			{
			    if (m && m->m_len >= sizeof(struct lat_state))
			    {
				sptr = mtod(m, struct lat_state *);
				if ((scl->scl_state = sptr->lst_state) == LST_OFF)
				{
				    for (slot = sl,i = 0; i < LAT_MAXSLOTS; slot++,i++)
				    {
					if (slot->lsl_state != SST_FREE && slot->lsl_class == class)
					{
					    (*scl->scl_hangup)(slot->lsl_vc, slot);
					    terminateslot(slot);
					}
				    }
				}
			    }

			    else
			    {
				mprintf("lat error: LAT_STATE\n");
				error = EMSGSIZE;
			    }
			}
			else
			{
			    error = ERANGE;
			}
			break;
		    default:
			error = EOPNOTSUPP;
		} /* switch */
		sockactive = 0;
	    }
	    else
	    {
		error = EACCES;
	    }
	    break;

	default:
	    panic("lat_usrreq");
    }
    splx(s);
    return (error);
}
