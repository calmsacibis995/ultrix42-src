#ifndef lint
static char *sccsid = "@(#)lat_hic.c	4.3	7/28/88";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *	Chung Wong - 1/7/88   First version.                            *
 *		Subroutines to provide transparent host initiated       *
 *              connection, with algorithm similar to the handling of   *
 *              ioctls LIOCINI, LIOCSOL, LIOCRES and LIOCCMD.           *
 *		Provision for LAT master.				*
 *                                                                      *
 ************************************************************************/

/*	lat_hic.c	0.0	12/11/84	*/

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
#include "../h/file.h"
#include "../h/gnode.h"
#include "../h/proc.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"
#include "../h/ioctl.h"
#include "../h/tty.h"

extern struct socket *lat_traceso;

extern struct sclass *sclass[];
extern struct sclass class1;
extern struct ecb statable[];
extern u_short reqid;
extern int wakeup();
extern caddr_t buildvchdr();
extern struct lat_vc *newvc(), *findvc(); 
extern struct lat_vc *vc[]; 
extern struct lat_slot sl[];
extern struct tty lata[];
extern struct ecb statable[];
extern struct sclass *sclass[];
extern struct vc_start startvc;
extern u_char slotsinuse;
extern struct lat_counters latctrs;
extern int nLAT1;

extern struct hic_entity lat_obj[];
struct mbuf *lat_hicres();



/*
 * LAT host-initiated connection routines.
 */

 /*
  *		l a t _ h i c i n i
  *
  * Process initialization for LAT host-initiated connection 
  *
  * Returns:		0 if success
  *			error code if not
  *
  * Inputs:
  *	paths		= Pointer to parameters (subj port, obj name/port)
  *                       (in format: /dev/ttyxx:server_name:server_port)
  *     pathlen 	= length of path name for subject port
  *	unit		= index to "statable" (= minor device number)
  *	cmdbyte		= LIOCINI command byte
  */
lat_hicini(paths,pathlen,unit,cmdbyte)
char *paths;
int pathlen,unit;
u_char cmdbyte;
{
    struct hic_entity *lptr = &lat_obj[unit];
    struct ecb *ecbp = &statable[unit];
    int i;

    /* return error if device busy
    if ((ecbp->ecb_inuse & ECB_INUSE) || (lata[unit].t_addr))
        return(EBUSY); */

    lptr->status &= ~(HIC | MHIC);
    if (cmdbyte != '\001') return (0);

    /*
     * if no objects specified, return
     */
    if (paths[pathlen-1] != ':') return(0);

    /* 
     * save subject port name and length
     */
    paths[pathlen-1] = '\0';
    if ((lptr->subj_portlen = pathlen - 1) > MAXNAM) 
        return(EADDRNOTAVAIL);
    bcopy(paths, lptr->subj_port, pathlen);
    paths[pathlen-1] = ':';
    lptr->status |= SUBJPORT;

    /*
     * save object name
     */
    paths += pathlen;
    if ((pathlen = lat_pathlen(paths)) == 1) return(0);
    if (paths[pathlen-1] != ':') return(0);
    if ((lptr->obj_namelen = pathlen - 1) > MAXNAM)
        return(EADDRNOTAVAIL);
    bcopy(paths, lptr->obj_name, pathlen-1);
    lptr->obj_name[pathlen-1] = 0;
    lptr->status |= OBJNAME;

    /*
     * save object port
     */
    paths += pathlen;
    if ((pathlen = lat_pathlen(paths)) == 1) return(0);
    if ((lptr->obj_portlen = pathlen - 1) > MAXNAM)
        return(EADDRNOTAVAIL);
    
#ifdef LATMASTER
    if (*paths == '.') /* latmaster */
    {
        char *hex, ch1, ch2;

        paths++;
        for (i=0; *paths!='.'; i++,paths++) 
        {
            ch1 = *paths++;
            ch2 = *paths;
            if (ch1<='9') ch1 &= 0x0f;
            else ch1 = (ch1 & 0x07) + 9;
            if (ch2<='9') ch2 &= 0x0f;
            else ch2 = (ch2 & 0x07) + 9;
            lptr->obj_port[i] = (ch1 << 4) + ch2;
        }
        lptr->obj_port[i] = '\0';
        lptr->obj_portlen = i;
        lptr->status |= HOSTMASTER;
    } 
    else
#endif
    {
        bcopy(paths, lptr->obj_port, pathlen-1);
        lptr->status |= OBJPORT;
    }

    ecbp->ecb_hostinit = LAT_HIC;
    return(0);
}





 /*
  *		l a t _ h i c o p e n
  *
  * Process latopen request in case of host initiated connection
  *
  * Returns:		0 if success
  *			error code if not
  *
  * Inputs:
  *	unit		= index to "statable" (= minor[subj port])
  */
lat_hicopen(unit,tp)
int unit;
struct tty *tp;
{
    int s, err;
    u_short solid;
    struct hic_entity *h = &lat_obj[unit];
    struct mbuf *m0,*m1;
    struct solicit_1 *solptr;
    struct response_1 *resptr;
    struct ecb *ecbp = &(statable[unit]);
    struct ifnet *ifp = ifnet;
    struct proc *pp = u.u_procp;

    /*
     * if defined as master, return
     */
     if (h->status & HOSTMASTER) return(0); 

    /*
     * check if host-initiated connection fully defined
     */
     if (ecbp->ecb_hostinit != LAT_HIC) return(0);
     if (((h->status & HIC) != HIC)) return(EDESTADDRREQ); 

    /*
     * LAT open should be exclusive, unless multiplexing added
     */
     if (ecbp->ecb_inuse & ECB_INUSE)
     {
         if (ecbp->ecb_inuse & ECB_NONHIC) return (0);
         return(EBUSY);
     }
     if (tp->t_addr) return(EBUSY);

    /*
     * if lat not running, return error
     */
     if (class1.scl_state != LST_RUNNING) return(ENOPROTOOPT);

     ecbp->ecb_inuse |= ECB_INUSE;
     s = splnet();

    /*
     * For each network interface,  build solicit msg 
     * with object name, then wait for response.  Send
     * command msg to the first server that responded.
     */
    while (ifp)
    {
	if ((ifp->if_flags & (IFF_BROADCAST|IFF_DYNPROTO|IFF_UP)) == (IFF_BROADCAST|IFF_DYNPROTO|IFF_UP))
	{
            if (!(m0 = m_get(M_DONTWAIT, MT_DATA)))
	    {
                err = ENOBUFS;
                goto badret;
	    }

            m0->m_len = sizeof(struct solicit_1);
            solptr = mtod(m0, struct solicit_1 *);
            solptr->sol1_dstnodelen = h->obj_namelen;
            bcopy(h->obj_name, solptr->sol1_dstnode, h->obj_namelen);
            class1.scl_solicit(m0, ifp);
            solid = *(u_short *)solptr->sol1_solid;
            sleep(solptr->sol1_solid, TTIPRI);

            /*
             * get response msg
             */
            if (m1 = lat_hicres(solid))
                goto gotres;
	}
	ifp = ifp->if_next;
    }
    err = EADDRNOTAVAIL;
badret:
    ecbp->ecb_inuse &= ~ECB_INUSE;
    splx(s);
    return (err);
    
gotres:
    /*
     * send command msg
     */
    err = lat_hiccmd(h,ifp,m1,unit);
    if (!err) lata[unit].t_pgrp = pp->p_pgrp;
    splx(s);
    return (err);
    
}





/*
 *		l a t _ h i c r e s
 *
 * Get response msg with matched id
 *
 * Returns:		mbuf with the response msg
 *			0 if no match found
 *
 * Inputs:
 *	solid		= id in the solicit msg
 *
 */
struct mbuf *
lat_hicres(solid)
u_short solid;
{
    struct mbuf *n, *prev;
    struct response_1 *res_que;

    n = class1.scl_rmsg.q_head;

    /*
     * Find response information structure in response
     * queue matching solicit ID of user structure
     */
    while (n) {
        res_que = mtod(n, struct response_1 *);
        if ( solid == *(u_short *)res_que->rs1_solid )
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
             return(n);
         }
	 else
	 {
	      prev = n;
	      n = n->m_act;
	 }
    } /* while n */

    /*
     * response mas not found
     */
    return( (struct mbuf *) 0);
}





/*
 *		l a t _ h i c c m d
 *
 * Send command msg to request a connection
 *
 * Returns:		0 if success
 *			error code if not
 *
 * Inputs:
 *	h		= pointer to the hic_entity structure
 *	ifp		= pointer to the network interface structure
 *	m1		= pointer to mbuf with response msg
 *	unit		= index to "statable" (= minor[subj port])
 */
lat_hiccmd(h,ifp,m1,unit)
struct hic_entity *h;
struct ifnet *ifp;
struct mbuf *m1;
int unit;
{
    caddr_t commsg, dptr;
    u_char u_len;
    struct mbuf *m0, *cm;
    struct response_1 *resptr = mtod(m1,struct response_1 *);
    struct lat_cmd *commsgp;
    struct lataddr objaddr;
    struct ecb *ecbp = &(statable[unit]);

    /*
     * Copy Ethernet address of object node.
     */
    bcopy((caddr_t)resptr->rs1_srcnode,(caddr_t)objaddr.lat_addr,6);
    objaddr.lat_family = AF_LAT;
    m_freem(m1);

    m0 = m_get(M_DONTWAIT,MT_DATA);
    commsgp = mtod(m0,struct lat_cmd *);

    /*
     * Build command message header.
     */
    commsgp->lcm_type = MSG_CMD << 2;
    commsgp->lcm_protofmt = 0;
    commsgp->lcm_Hver = commsgp->lcm_Cver = LAT_VER;
    commsgp->lcm_Lver = LAT_VER_LOW;
    commsgp->lcm_eco = LAT_ECO;
    *(u_short *)commsgp->lcm_framesize = LAT_FRAMESIZE;
    *(u_short *)commsgp->lcm_reqid = reqid;
    *(u_short *)commsgp->lcm_entryid = (u_short)0;

    /* 
     *  set up command type and command modifier
     */
    if (lata[unit].t_state & TS_ONDELAY) 
    {
	/* for no delay, get non-queue service */
        commsgp->lcm_cmdtype = 1;
        commsgp->lcm_cmdmod = 0;
    }
    else 
    {
	/* queue service, with periodic return status */
        commsgp->lcm_cmdtype = 2;
        commsgp->lcm_cmdmod = 1;
    }

    commsg = (caddr_t)commsgp;
    commsg += sizeof(struct lat_cmd);

    /*
     * object node name 
     */
    u_len = h->obj_namelen;
    *commsg++ = (char)h->obj_namelen;
    bcopy(h->obj_name,commsg,(int)u_len);
    commsg += u_len;

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
			
    /*
     * subject port
     */
    u_len = h->subj_portlen;
    *commsg++ = u_len;
    bcopy(h->subj_port, commsg, (int)u_len);
    commsg += u_len;

    /*
     * subject description =0,  object service = 0
     */
    *commsg++ = 0;
    *commsg++ = 0;

    /*
     * object port
     */
    u_len = h->obj_portlen;
    *commsg++ = u_len;
    bcopy(h->obj_port, commsg, (int)u_len);
    commsg += u_len;

    /*
     * null termination
     */
    *commsg++ = 0;
    m0->m_len = (short)(commsg - (caddr_t)commsgp);

    ecbp->ecb_cmdmsg = m0;
    ecbp->ecb_reqid = *(u_short *)commsgp->lcm_reqid;
    ecbp->ecb_if = ifp;
    bcopy((caddr_t)objaddr.lat_addr, (caddr_t)(ecbp->ecb_addr.lat_addr),6);
    ecbp->ecb_addr.lat_family = AF_LAT;
    ecbp->ecb_statrecd = 0;

    /*
     * invoke network interface
     */
    if (lat_traceso)
	ltt_trace(0,0,m0,objaddr.lat_addr);
    if (cm = m_copy(m0,0,M_COPYALL))
        (*ifp->if_output)(ifp,cm,&objaddr);

    /* 
     * Increment reqid, which must not be 0.
     */
    reqid = ( (reqid == 0xffff) ? 1 : reqid + 1);

    return(0);
}





 /*
  *		l a t _ p a t h l e n
  *
  */
int lat_pathlen(path)
caddr_t path;
{
    int count;

    count = 1;
    while (*path != '\0' && *path != ':')
        count++, path++;
    return (count);
}



/****maserver****/
/*
 * For the server, a few 'quick and dirty' kludges need to be fixed:
 *  - add a bit/byte to lat_vc for master indicator, instead of using
 *    lvc_dgsize (<= 60 implied as a master).
 *  - add a master keep_alive byte to lat_vc, instead of using 
 *    lvc_dgsize (timer count up to 60 for 30 seconds keep alive interval).
 *  - lvc_rcvact in lat_vc is used as multiple bit filed:
 *    1000 0000 - outgoing data waiting for master vc
 *    0100 0000 - master vc in balance mode
 *    0000 0001 - vc receiver active
 *  - add a byte filed to lat_slot for credit count for master side, instead
 *    of using ls1_attsize.
 *
 * The master timer (defined in lat_mtick, set to 80 milliseconds) is
 * used to check if the tty input queue is empty so credit can be granted
 * to the remote slave. Without checking for empty tty queue can easily
 * cause the queue to overflow.
 * Master mode also ensures no more outgoing message unless ack queue is
 * empty.  Unack sequence number greater than 1 seems to get acknowledgement
 * handshaking into chaos.  See traces.
 */
/****maserver****/

int lat_debug = 0;
int lat_mtick = 0x08;
int lat_msize = 0x0ff;
int lat_mflag = 0;
#define LAT_MASTIMER 1

/*
 *	m v c s t a r t
 */
mvcstart(unit)
{
#ifndef LATMASTER
    return(0);
#else

    extern int lat_mtimer();
    extern struct lat_vc *vcduplicate(); 
    register struct mbuf *m;
    register struct lat_vc *vcir;
    struct ecb *ecbp = &(statable[unit]);
    struct hic_entity *h = &(lat_obj[unit]);
    struct vc_hdr *hdr;
    struct vc_start *vst;
    struct lat_slot *slot;
    struct slot_hdr *slhdr;
    struct slot_start *ssl;
    struct mbuf *page;
    caddr_t sptr,nptr;
    char ch;
    struct ifnet *ifp = ifnet;
    struct proc *pp = u.u_procp;
    int i,s;

    /*
     * if lat not running, return error
     */
    if (!sclass[1] || (sclass[1]->scl_state != LST_RUNNING))
         return(ENOPROTOOPT);

    /*
     * if host-initiated connection not defined, return
     */
    if (ecbp->ecb_hostinit != LAT_HIC) return(0);

    /*
     * if not defined as master, return
     */
     if (h->status & OBJPORT) return(0); 

    /*
     * if host master initiated connection not fully defined, return error
     */
    if ((h->status & MHIC) != MHIC) return (EDESTADDRREQ); 

    /*
     * host_initiated open should be exclusive 
     * unless multiplexing has been added
     */
    if ((ecbp->ecb_inuse & ECB_INUSE) || lata[unit].t_addr)
	return(EBUSY);

    /* set up id for signals */
    lata[unit].t_pgrp = pp->p_pgrp;
  
    s = splnet();

    /* see if vc exists */
    if (vcir = vcduplicate(h)) goto startslot1;

    /* get vc block */
    if (vcir = newvc())
    {
        bcopy(h->obj_port, vcir->lvc_addr.lat_addr, 6); 
        vcir->lvc_addr.lat_family = AF_LAT;
        vcir->lvc_ack = 255;
        vcir->lvc_dgsize = 0;
    }
    else goto nobuf;

    ecbp->ecb_inuse |= ECB_INUSE;
    while (ifp)
    {
	if ((ifp->if_flags & (IFF_BROADCAST|IFF_DYNPROTO|IFF_UP)) == (IFF_BROADCAST|IFF_DYNPROTO|IFF_UP))
	{
            if (m = m_get(M_DONTWAIT, MT_DATA))
            {
	        MCLGET(m,page);
	        if (page)
	        {
                    hdr = (struct vc_hdr *)page;
	            vcir->lvc_state = VST_RUNNING;
                    vcir->lvc_if = ifp;
	            sptr = buildvchdr(vcir, (struct vc_hdr *)page, 0, MSG_START, 0);
                    hdr->vhd_mas = 1;
	            bcopy((char *)&startvc, (char *)sptr, sizeof(struct vc_start));
                    vst = (struct vc_start *)sptr;
                    vst->vst_dgsize = 1518;
                    vst->vst_slots = 0x30;
                    vst->vst_product = 0x102; 
                    vst->vst_stimer = 8;
                    vst->vst_kalive = vcir->lvc_kalive = 30;
	            sptr += sizeof(struct vc_start);
                    if (*sptr++ = h->obj_namelen)
	            {
		        nptr = (caddr_t)h->obj_name;
		        while (ch = *nptr++)
		        {
	    	            if (('a' <= ch) && (ch <= 'z')) ch = ch -'a' + 'A';
	    	            *sptr++ = ch;
	        	}
	            }
	            if (*sptr++ = hostnamelen)
	            {
		        nptr = hostname;
		        while (ch = *nptr++)
		        {
		            if (('a' <= ch) && (ch <= 'z')) ch = ch -'a' + 'A';
		            *sptr++ = ch;
	                }
	            }
	            /*  Terminate the parameters  */
	            *sptr++ = 0;
	            m->m_len = (int)sptr - (int)page;
	            INC(lco_xmtframes);
	            ENQUEUE(&vcir->lvc_xmtq, m);
	            latsend(vcir);
                    sleep(m, TTIPRI);
	    
                    /* check if timeout */
                    if (vcir->lvc_remid) goto startslot;
                    break; 
	        }
	        m_free(m);
                ecbp->ecb_inuse &= ~ECB_INUSE;
                goto nobuf;
            }
            else
            {
                ecbp->ecb_inuse &= ~ECB_INUSE;
                goto nobuf;
            }
        }
	ifp = ifp->if_next;
    }

    ecbp->ecb_inuse &= ~ECB_INUSE;
    splx(s);
    return (ENETUNREACH);

nobuf:
    splx(s);
    return (ENOBUFS);


startslot:
    vcir->lvc_state = VST_RUNNING;

startslot1:
    /* get a slot table entry  */
    for (slot = sl, i = 0; i < LAT_MAXSLOTS; slot++, i++)
        if (slot->lsl_state == SST_FREE) break;

    /* Initialise the new slot database  */
    slotsinuse++;
    vcir->lvc_act++;
    slot->lsl_vc = vcir;
    slot->lsl_class = 1;
    slot->lsl_state = SST_STARTING;
    slot->lsl_locid = i + 1;
    slot->lsl_remid = 0;
    slot->lsl_remcredits = 0;
    slot->lsl_loccredits = 0;
    slot->lsl_attsize = 0; 
    slot->lsl_scl = sclass[1];
    
    lata[unit].t_addr = (caddr_t)slot;
    lata[unit].t_state |= TS_CARR_ON;
    lata[unit].t_flags &= ~ECHO;
    slot->lsl_data = (caddr_t) &lata[unit];
    slot->lsl_bslot = 0;
    ttyflush(&lata[i], FREAD|FWRITE);

    if (m = m_get(M_DONTWAIT, MT_DATA))
    {
        MCLGET(m,page);
        if (page)
        {
            hdr = (struct vc_hdr *)page;
	    sptr = buildvchdr(vcir, (struct vc_hdr *)page, 1, MSG_RUN, 0);
            hdr->vhd_mas = 1;
            slhdr = (struct slot_hdr *)sptr;
            slhdr->shd_dstid = 0;
            slhdr->shd_srcid = slot->lsl_locid;
            slhdr->shd_count = 4;
            slhdr->shd_credits = 0; 
            slhdr->shd_type = SLOT_START;
            sptr += sizeof(struct slot_hdr);
            ssl = (struct slot_start *)sptr;
            ssl->sst_class = slot->lsl_class = 1;
            ssl->sst_minAsize = ssl->sst_minDsize =slot->lsl_datasize=lat_msize;
            sptr += sizeof(struct slot_start);
            if (*sptr++ = h->obj_namelen)
	    {
	        nptr = (caddr_t)h->obj_name;
	        while (ch = *nptr++)
	        {
	            if (('a' <= ch) && (ch <= 'z')) ch = ch -'a' + 'A';
	            *sptr++ = ch;
		}
	    }
            slhdr->shd_count += h->obj_namelen + 1;
            if (*sptr++ = h->subj_portlen)
	    {
	        nptr = (caddr_t)h->subj_port;
	        while (ch = *nptr++)
	        {
	            if (('a' <= ch) && (ch <= 'z')) ch = ch -'a' + 'A';
	            *sptr++ = ch;
		}
	    }
            slhdr->shd_count += h->subj_portlen + 1;
            /* add group code ??????? */
            *sptr++ = 0;

	    m->m_len = (int)sptr - (int)page;
	    INC(lco_xmtframes);
	    ENQUEUE(&vcir->lvc_xmtq, m);
	    latsend(vcir);
            sleep(m, TTIPRI);
              
            /* check for timeout */
            if (!slot->lsl_remid) 
            {
                ecbp->ecb_inuse &= ~ECB_INUSE;
		splx(s);
                return (ENETUNREACH);
            }
            lat_mack(vcir);
            if (!(lat_mflag & LAT_MASTIMER)) timeout(lat_mtimer, 0, lat_mtick);
        }
        splx(s);
        return(0);
    }
    splx(s);
    return(ENOBUFS);

#endif
}

/*
 *	l a t _ m a c k
 */
lat_mack(vcir)
struct lat_vc *vcir;
{
#ifdef LATMASTER
    vcir->lvc_rrf = 0;
    vcrun(vcir);
#endif
}

/*
 * 	m v c s t a r t 1
 */
mvcstart1(m,vhdr)
struct mbuf *m;
struct vc_hdr *vhdr;
{
#ifndef LATMASTER
    return(0);
#else

    register struct lat_vc *vcir;
    struct mbuf *m0;
    int index;

    index = vhdr->vhd_dstid & 0377;
    vcir = vc[index];
    vcir->lvc_remid = vhdr->vhd_srcid;
    vcir->lvc_ack++;
    DEQUEUE(&vcir->lvc_ackq, m0);
    if (m0)
    {
        wakeup(m0);
        m_freem(m0);
    }
    m_freem(m);
#endif
}


/*
 * 	m s l o t s t a r t
 */
mslotstart(slothdr)
struct slot_hdr *slothdr;
{
#ifndef LATMASTER
    return(0);
#else

    register struct lat_vc *vcir;
    register struct lat_slot *slot;
    struct mbuf *m0;

    if (vcir = findvc())
    {
        vcir->lvc_ack++;
        vcir->lvc_lxmt++;
        slot = &sl[slothdr->shd_dstid-1];
        slot->lsl_remid = slothdr->shd_srcid;
        slot->lsl_loccredits += slothdr->shd_credits;
        slot->lsl_remcredits = 1; /* set credit to 1 for now */
        slot->lsl_state = SST_RUNNING;
        DEQUEUE(&vcir->lvc_ackq, m0);
        if (m0)
        {
            wakeup(m0);
            m_freem(m0);
        }
    }
#endif
}

lat_master(vcir)
register struct lat_vc *vcir;
{
#ifdef LATMASTER
    if (vcir->lvc_dgsize <= 60) return (1);
#endif

    return(0);
}


#ifdef LATMASTER
/*
 * 	l a t _ m t i m e r
 *
 * LAT master 80 millisecond timer (in lat_mtick)
 *
 */
lat_mtimer()
{
    register struct lat_vc *vcir;
    register struct lat_slot *slot;
    register struct tty *tp;
    int s = splnet();
    int i, j, cr;

    lat_mflag &= ~LAT_MASTIMER;
    for (i=1; i<nLAT1; i++)
    {
        if (vcir = vc[i])
        {
            if (!lat_master(vcir)) continue;
            lat_mflag |= LAT_MASTIMER;
	    /* 
	     * If there are more than 1 unacked sequence number, 
 	     * the whole acknowledgement hand shaking may become
	     * chaos unless some coding changes in process_vc_run().
	     * For now, do not send if ack queue not empty
	     */
	    if (vcir->lvc_ackq.q_head) continue;

	    cr = 0;
            for (slot=sl,j=0; j<nLAT1; slot++,j++)
            {
                if ((slot->lsl_vc == vcir) && (slot->lsl_state == SST_RUNNING))
                {
                    tp = (struct tty *)slot->lsl_data;
		    /* grant credit only if tty queue is empty */
                    if (slot->lsl_attsize && !tp->t_rawq.c_cc)
                    {
                        slot->lsl_remcredits = slot->lsl_attsize;
                        slot->lsl_attsize = 0;
                        cr = 1;
                    }
		    /* check if anything can be sent out */
		    else if (tp->t_outq.c_cc)
		    {
			/* if no credit, set flag for data waiting */
		        if (!slot->lsl_loccredits) 
			    vcir->lvc_rcvact |= 0x80;
			else 
                            cr = 1;
		    }
                }   
            }
	    if (cr) lat_mack(vcir);
        }
    }

    /* start timer if there is master virtual circuit */
    if (lat_mflag & LAT_MASTIMER) timeout(lat_mtimer, 0, lat_mtick);
    splx(s);
}
#endif


lat_alive(vcir)
struct lat_vc *vcir;
{
#ifdef LATMASTER
    if (lat_master(vcir))
    { 

	/* 
	 * latmaster - vms: a possible bug in vms driver not returning
	 * 		    credit in time, send a message as reminder
	 *		    if output data waiting
         */
        if ((vcir->lvc_dgsize == 60) || (vcir->lvc_rcvact & 0x80)) 
        {
  	    vcir->lvc_rcvact &= 0x0bf;
	    if (!(vcir->lvc_ackq.q_head)) lat_mack(vcir);
            vcir->lvc_dgsize = 0; 
        }
        else vcir->lvc_dgsize++;
    }
#endif
}


#ifdef LATMASTER
struct lat_vc *vcduplicate(hptr)
struct hic_entity *hptr;
{
    register struct lat_vc *vcir;
    register int index;

    for (index = 1; index < LAT_MAXVC; index++)
    {
	if (vcir = vc[index])
	    if (lat_master(vcir))
	        if (bcmp(hptr->obj_port, vcir->lvc_addr.lat_addr, 6) == 0)
		    return (vcir);
    }
    return (0);
}
#endif


/*
 * Input:	flag = 1: add multicast address
 *		     = 0: delete multicast address
 */
lat_multi(flag)
int flag;
{
    
    register struct ifnet *ifp = ifnet;
    struct ifreq dreq;

    /*
     * enable LAT Multicast Address on each device.
     */
    bzero(dreq.ifr_addr.sa_data, 6);
    dreq.ifr_addr.sa_data[0] = 0x09;
    dreq.ifr_addr.sa_data[2] = 0x2b;
    dreq.ifr_addr.sa_data[5] = 0x0f;

    while ( ifp )
    {
        if ((ifp->if_flags & (IFF_BROADCAST|IFF_DYNPROTO)) == (IFF_BROADCAST|IFF_DYNPROTO))
        {
	    if (flag) (*ifp->if_ioctl)(ifp, SIOCADDMULTI, &dreq);
	    else (*ifp->if_ioctl)(ifp, SIOCDELMULTI, &dreq);
        }
        ifp = ifp->if_next;
    }
}
