#ifndef lint
static char *sccsid = "@(#)lat_vc.c	4.1.1.5	7/15/88";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *	Larry Cohen  -	09/16/85					*
 * 	    Update to new protsw format, ethernet addr struct change	*
 *									*
 *	Jeff Chase   -  03/12/86                                        *
 *	    Modified to handle the new MCLGET macro			*
 *									*
 *	Peter Harbo  -  04/15/86					*
 *	    Addition of LAT 5.1 support for application terminals:      *
 *	    interpret response and status messages.  Place status	*
 *	    message errors in appropriate entry of statable[].	        *
 *									*
 *	Chung Wong - 1/7/88 						*
 *	    Add check on master bit for conformance.			*
 *	    Add checkhostname() and checkmastername() for conformance.  *
 *	    Add m_freem(m) before exiting process_status().		*
 *	    Add 'wakeup((caddr_t)&lata[j].t_rawq)' in process_status(). *
 *									*
 ************************************************************************/


/*	lat_vc.c	0.0	11/9/84	*/
/*	lat_vc.c	2.0	04/15/86 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/file.h"
#include "../h/ioctl.h"
#include "../h/tty.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"

extern struct ifqueue latintrq;
extern struct lat_vc *vc[];
extern u_char rand;
extern struct vc_start startvc;
extern struct ifnet *rcvif;
extern struct lataddr rcvaddr;
extern struct lat_counters latctrs;
extern struct tty lata[];

extern struct mbuf *demux();
extern struct sclass class1;
extern struct ecb statable[];
extern int nLAT1;
extern struct socket *lat_traceso;
extern lat_debug; /* latmaster */
extern char lat_hostname[];
extern int lat_hostnamelen;

int latmnl;
char *latmn;

static struct vc_hdr vhdr;

/*
 * LAT virtual circuit management.
 */

struct lat_vc *findvc(),*newvc(),*duplicatevc();

/*
 *		l a t i n t r
 *
 * LAT domain input routine. This routine is 'called' from the network software
 * ISR routine to process incoming packets. The first MBUF in any chain
 * contains a LAT receive descriptor containing the received ethernet header
 * and a pointer to the interface structure.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 */
latintr()
{
    register struct mbuf *m;
    register struct latrecv *recv;
    register struct lat_vc *vcir;
    int s;

next:
    /*
     * Try to pull an input message (MBUF chain) from the LAT input queue.
     */
    s = splimp();
    IF_DEQUEUE(&latintrq, m);
    splx(s);
    if (m)
    {
	recv = mtod(m, struct latrecv *);
	rcvif = recv->rcv_ifp;
	bcopy((char *)recv->rcv_hdr.ether_shost,
		(char *)rcvaddr.lat_addr, sizeof(rcvaddr.lat_addr));
	m = m_free(m);
	if (m = LAT_PULLUP(m, sizeof(struct vc_hdr)))
	{
	    if (lat_traceso)
		ltt_trace(1,0,m,rcvaddr.lat_addr);
	    INC(lco_rcvframes);
	    vhdr = *mtod(m, struct vc_hdr *);
	    
	    /*
	     * Handle response and status messages apart
	     * from virtual circuit messages.
	     */

	    switch (vhdr.vhd_type)
	    {
		case MSG_RESP:
	            if ( m = LAT_PULLUP(m,sizeof(struct response_1)) )
		        class1.scl_response(m);
		    else
		    {
		        INC(lco_badmsg);
		        m_freem(m);
		    }
		    goto next;

                case MSG_STAT:
		    if ( m = LAT_PULLUP(m,sizeof(struct lat_stat)) )
			process_status(m);
		    else
		    {
		        INC(lco_badmsg);
			m_freem(m);
		    }
		    goto next;

                case MSG_DR1:
                case MSG_SOL:
		    m_freem(m);
		    goto next;
	    }
	            
	    m_adj(m, sizeof(struct vc_hdr));
      
            /*
	     * latmaster: check for START message and START SLOT. 
	     * All other message and slots are processed the same 
	     * way in either slave mode or master mode.
	     */
            if (vhdr.vhd_mas == 0) 
            {
                if (vhdr.vhd_type == MSG_START)
                {
                    mvcstart1(m,&vhdr);
                    goto next;
                }
                if (vhdr.vhd_type == MSG_RUN)
                {
                    struct slot_hdr *slothdr;

                    m = LAT_PULLUP(m, sizeof(struct slot_hdr));
                    slothdr = mtod(m, struct slot_hdr *);
                    if (slothdr->shd_type == SLOT_START)
                    {
                        mslotstart(slothdr);
                        m_freem(m);
                        goto next;
                    }
                } 
            }

	    switch (vhdr.vhd_type)
	    {
		/*
		 * The run message contains slots which should be given to the
		 * slot demultiplexor.
		 */
		case MSG_RUN:
		    process_vc_run(m);
		    goto next;

		/*
		 * The start message requests that a virtual circuit be started
		 * or restarted.
		 */
		case MSG_START:
		    process_vc_start(m);
		    goto next;

		/*
		 * The stop message requests that a virtual circuit be stopped.
		 */
		case MSG_STOP:
		    process_vc_stop(m);
		    goto next;

		/*
		 * Unknown message - if we can find an associated virtual
		 * circuit, stop all the slots and tear down the virtual
		 * circuit otherwise just toss the message.
		 */
		default:
                    goto proto_error;
		/*  INC(lco_badmsg);
		    if (vcir = findvc())
		    {
			terminatevc(vcir, STOP_BADFORMAT);
		    }
		    m_freem(m);
		    goto next;   */
	    }
	}
    }

return;
/*
 * Protocol error - if we can find an associated virtual
 * circuit, stop all the slots and tear down the virtual
 * circuit otherwise just toss the message.
 */
proto_error:
    INC(lco_badmsg);
    if (vcir = findvc())
	terminatevc(vcir, STOP_BADFORMAT);
    m_freem(m);
    goto next;
}

/*
 *			p r o c e s s _ v c _ r u n
 *
 * Process a received run message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	m		= Pointer to MBUF chain with the run message.
 */
process_vc_run( m )
struct mbuf *m;
{
    register struct lat_vc *vcir;
    struct mbuf *m0;
    int i,msgnum;

    if (vcir = findvc())
    {
	/* 
	 * latmastrer: save rrf indicator, 
         * in case of master balance mode 
         */
        if (!vhdr.vhd_rrf) vcir->lvc_rcvact |= 0x40;
        else vcir->lvc_rcvact &= 0x0bf;

	if (vcir->lvc_state != VST_STARTING)
	{
	    /* make sure master bit is set correctly */
            if (!lat_master(vcir))
            {
                if (vhdr.vhd_mas == 0) goto proto_error;
            }
            else
                if (vhdr.vhd_mas == 1) goto proto_error;

	    /*
	     * If the message is out of sequence, effectively throw the data it
	     * contains away by zeroing the slot count and increment the 
	     * duplicate message counter, otherwise, increment the 
	     * acknowledgement number.
	     */
	    if (vhdr.vhd_seq != ((vcir->lvc_ack + 1) & 0377))
	    {
		vhdr.vhd_slots = 0;
		INC(lco_rcvdup);
	    }
	    else vcir->lvc_ack++;
	    /*
	     * Process the acknowledgement number from the message.
	     */
	    msgnum = (vhdr.vhd_ack - vcir->lvc_lxmt) & 0377;
	    if (msgnum <= ((vcir->lvc_hxmt - vcir->lvc_lxmt) & 0377))
	    {
		for (i = 0; i < msgnum; i++)
		{
		    DEQUEUE(&vcir->lvc_ackq, m0);
		    if (m0 == 0)
			break;
		    m_freem(m0);
		    vcir->lvc_lxmt++;
		}
	    }
	    /*
	     * If there are no messages awaiting acknowledgement, we
	     * can cancel the retransmission timer and allow further
	     * transmissions.  */
	    if (vcir->lvc_ackq.q_head == 0)
	    {
		vcir->lvc_timer = 0;
		vcir->lvc_rrf = 0;
	    }
	    else 
	    {
		rxmit_ackq(vcir);
		if (lat_master(vcir))	/* latmaster waits for empty ackq */
		    vcir->lvc_rcvact |= 0x40;
	    }
	    /*
	     * If there are any slots present in the message hand them to the
	     * slot demultiplexor.
	     */
	    if (vhdr.vhd_slots)
		m = demux(vcir, m, (int)vhdr.vhd_slots);
	    if ( !vc[vhdr.vhd_dstid & 0377] )
		goto freembuf;
	    /*
	     * If we are trying to stop this virtual circuit, and there are no
	     * messages awaiting transmission or acknowledgement we can stop it
	     * now.
	     */
	    if ((vcir->lvc_state == VST_STOPING) && (vcir->lvc_ackq.q_head == 0) && (vcir->lvc_xmtq.q_head == 0))
	    {
		freevc(vcir, STOP_NOSLOTS);
	    }
	    else 
		/* 
	  	 * latmaster: slave always acknowledges 
		 * the last message even when rrf not set.
		 */
                if (!lat_master(vcir) || !(vcir->lvc_rcvact & 0x40) || 
                    ((vcir->lvc_rcvact&0x80) && vcir->lvc_ackq.q_head==0))
                    vcrun(vcir); 
	}
	else latsend(vcir);
    }
freembuf:
    m_freem( m );
    return;

proto_error:
    INC(lco_badmsg);
    terminatevc(vcir, STOP_BADFORMAT);
    m_freem(m);
}

/*
 *		p r o c e s s _ v c _ s t a r t
 *
 * Process a received start message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	m		= Pointer to MBUF chain with the start message.
 */
process_vc_start( m )
struct mbuf *m;
{
    register struct lat_vc *vcir;
    register struct vc_start *vst;
    int reason = 0;
    int vcstartlen = m->m_len;
    struct mbuf *m0 = m->m_next;

    while (m0) {
        vcstartlen += m0->m_len;
        m0 = m0->m_next;
    }
    if (m = LAT_PULLUP(m, vcstartlen))
    {
	vst = mtod(m, struct vc_start *);
	if (vhdr.vhd_srcid != 0) 
	{
	    if ((vhdr.vhd_dstid == 0) && checkhostname(vst) && checkmastername(vst))
	    {
		if (vst->vst_pver >= LAT_VER)
		{
		    if (vcir = duplicatevc()) 
		    {
                        if (vcir->lvc_state == VST_STARTING)
			    vcstart(vcir);
                        else
                            reason = STOP_BADFORMAT;
		    }
		    else
		    {
			if (vcir = newvc())
			{
			    vcir->lvc_dgsize = vst->vst_dgsize < CLBYTES ? vst->vst_dgsize : CLBYTES;
			    vcir->lvc_remid = vhdr.vhd_srcid;
			    vcir->lvc_kalive = vst->vst_kalive;
			    vcstart(vcir);
			}
			else reason = STOP_NORESOURCE;
		    }
		}
		else reason = STOP_BADFORMAT;
	    }
	    else reason = STOP_BADFORMAT;
	}
	m_freem(m);
	if (reason)
	{
            if (reason == STOP_BADFORMAT) 
                INC(lco_badmsg);
	    vcstop(rcvif, vhdr.vhd_dstid, vhdr.vhd_srcid, reason);
	}
    }
}

/*
 *		p r o c e s s _ v c _ s t o p
 *
 * Process a received stop message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	m		= Pointer to MBUF chain with the stop message.
 */
process_vc_stop( m )
struct mbuf *m;
{
    register struct lat_vc *vcir;

    if (vcir = findvc())
    {
	terminatevc(vcir, 0);
    }
    m_freem(m);
}

/*		p r o c e s s _ s t a t u s
 *
 * Process an incoming status message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *		m =	Mbuf containing status message.
 */
process_status(m)
struct mbuf *m;
{
    struct lat_stat *statp;
    int entries, namelen, i, j;
    struct lat_statent *ent;

    /*
     * Strip off status message header, subject node name.
     */
    statp = mtod(m,struct lat_stat *);
    entries = (int)statp->lstat_entries;
    namelen = (int)statp->lstat_nodenamlen;

    /* The name field is padded to an even number of bytes.
     */
    if (namelen % 2)
	namelen++;
    m_adj(m,sizeof(struct lat_stat));
    m_adj(m,namelen);
    
    /*
     * For each entry in status message, if there is an error, place in
     * entry control block for the lta associated with that reqid.
     */

    for(;entries;entries--)
    {
	if ( m = LAT_PULLUP(m,sizeof(struct lat_statent)) )
	{
	    ent = mtod(m,struct lat_statent *);
	    for ( j = 0; ; j++ )
	    {
		if (j <= nLAT1)
		{
		    if (statable[j].ecb_reqid == ent->lent_reqid)
		        break;
		}
		else
	        {
		    m_freem(m);
		    return;
	        }
            }
	    statable[j].ecb_statrecd = 1;
	    statable[j].ecb_entryid = ent->lent_entryid;
	    wakeup((caddr_t)&lata[j].t_rawq);
			    
	    if ( ent->lent_stat & 0200 )
	    {
		switch (ent->lent_err)
		{
		    case STOP_SLDISC:
		    case STOP_SLNOSTART:
			    statable[j].ecb_error = ECONNABORTED;
			    break;
		    case STOP_SLSHUT:
			    statable[j].ecb_error = ESHUTDOWN;
			    break;
		    case STOP_SLINVSLOT:
			    statable[j].ecb_error = EMSGSIZE;
			    break;
		    case STOP_SLINVSERV:
			    statable[j].ecb_error = ENODEV;
			    break;
		    case STOP_SLNORES:
			    statable[j].ecb_error = EPROCLIM;
			    break;
		    case STOP_SLINUSE:
			    statable[j].ecb_error = EBUSY;
			    break;
		    case STOP_SLNOSUCH:
		    case STOP_SLDISABLED:
			    statable[j].ecb_error = EADDRNOTAVAIL;
			    break;
		    case STOP_SLNOTOFF:
			    statable[j].ecb_error = ENXIO;
			    break;
		    case STOP_SLNAME:
		    case STOP_SLDELETE:
			    statable[j].ecb_error = ENOENT;
			    break;
		    case STOP_SLPASSWD:
		    case STOP_SLCORRUPT:
		    case STOP_SLILLEGAL:
		    case STOP_SLREQPAR:
			    statable[j].ecb_error = EINVAL;
			    break;
		    case STOP_SLQUEUE:
			    statable[j].ecb_error = ESPIPE;
			    break;
		    case STOP_SLREJ:
		    case STOP_SLDENIED:
			    statable[j].ecb_error = EACCES;
			    break;
		    /*
		     * If an illegal error is received, return a generic
		     * access error.
		     */
		    default:
			    statable[j].ecb_error = EACCES;
			    break;
		} /* switch */
	        ttyflush(&lata[j],FREAD | FWRITE);
	    } /* if */
	}
	else 
	{
	    m_freem(m);
	    return;
	}
    }
    m_freem(m); 
    
}

/*
 *		f i n d v c
 *
 * Find the virtual circuit defined by the LAT header on an incoming message.
 *
 * Outputs:		Pointer to the virtual circuit descriptor
 *			0 if none.
 *
 * Inputs:		None.
 */
struct lat_vc *findvc()
{
    register struct lat_vc *vcir;
    register u_short remid = 0;
    int index = vhdr.vhd_dstid & 0377;

    if (index < LAT_MAXVC)
    {
	if (vcir = vc[index])
	{
	    if (vhdr.vhd_type != MSG_STOP)
	    {
		remid = vcir->lvc_remid;
	    }
            else remid = vhdr.vhd_srcid; /* srcid should be 0, just to be sure */
	    if ((vcir->lvc_locid == vhdr.vhd_dstid) && (remid == vhdr.vhd_srcid))
	    {
		return (vcir);
	    }
	}
    }
    return (0);
}

/*
 *		d u p l i c a t e v c
 *
 * Determine if the start message just received is a duplicate for an already
 * active virtual circuit.
 *
 * Outputs:		Pointer to the duplicate virtual circuit
 *			0  if virtual circuit does not already exist.
 *
 * Inputs:		None.
 */
struct lat_vc *duplicatevc()
{
    register struct lat_vc *vcir;
    register int index;

    for (index = 1; index < LAT_MAXVC; index++)
    {
	if (vcir = vc[index])
	{
	    if (vhdr.vhd_srcid == vcir->lvc_remid)
	    {
		if (bcmp(rcvaddr.lat_addr, vcir->lvc_addr.lat_addr, 6) == 0)
		{
		    return (vcir);
		}
	    }
	}
    }
    return (0);
}

/*
 *		f r e e v c
 *
 * Attempt to send a stop virtual circuit message and then release the data
 * structures associated with the specified virtual circuit.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to the virtual circuit descriptor.
 *	reason		= Reason code for the stop virtual circuit message.
 *			  ( 0 if no need to send message ).
 */
freevc( vcir,reason )
register struct lat_vc *vcir;
int reason;
{
    register struct mbuf *m;

    /*
     * Release any messages waiting to be sent.
     */
    while (vcir->lvc_xmtq.q_head)
    {
	DEQUEUE(&vcir->lvc_xmtq, m);
	m_freem(m);
    }
    /*
     * Release any messages waiting for acknowledgement.
     */
    while (vcir->lvc_ackq.q_head)
    {
	DEQUEUE(&vcir->lvc_ackq, m);
        wakeup(m); /* latmaster */
	m_freem(m);
    }
    if (reason)
    {
        if (lat_master(vcir))  /* latmaster */
        {
            register struct vc_hdr *hdr;
            register caddr_t ptr;

            if (m = m_get(M_DONTWAIT, MT_DATA))
            {
	        m->m_off = LATOFFSET;
	        hdr = mtod(m, struct vc_hdr *);
	        hdr->vhd_rrf = 0;
	        hdr->vhd_mas = 1;
	        hdr->vhd_type = MSG_STOP;
	        hdr->vhd_slots = 0;
	        hdr->vhd_dstid = vcir->lvc_remid;
	        hdr->vhd_srcid = 0;
	        hdr->vhd_seq = vcir->lvc_nxmt;
	        hdr->vhd_ack = vcir->lvc_ack;
	        ptr = (caddr_t)hdr + sizeof(struct vc_hdr);
	        *ptr++ = reason;
	        *ptr++ = 0;
	        m->m_len = (int)ptr - (int)hdr;
	        INC(lco_xmtframes);
	        if (lat_traceso)
	            ltt_trace(0,0,m,(vcir->lvc_addr).lat_addr);
	        (*vcir->lvc_if->if_output)(vcir->lvc_if, m, &vcir->lvc_addr);
            }
        } else 
	vcstop(vcir->lvc_if, vcir->lvc_locid, vcir->lvc_remid, reason );
    }
    vc[vcir->lvc_locid & 0377] = 0;
    m_free(dtom(vcir));
}

/*
 *		n e w v c
 *
 * Allocate the resources required for a new virtual circuit and link them
 * into the LAT virtual circuit table.
 *
 * Outputs:		Pointer to the virtual circuit descriptor
 *			0 if no resources available.
 *
 * Inputs:		None.
 */
struct lat_vc *newvc()
{
    register int index;
    register struct mbuf *m;
    register struct lat_vc *vcir;

    for (index = 1; index < LAT_MAXVC; index++)
    {
	if (vc[index] == 0)
	{
	    if (m = m_getclr(M_DONTWAIT, MT_PCB))
	    {
		vcir = mtod(m, struct lat_vc *);
		vcir->lvc_state = VST_STARTING;
		vcir->lvc_locid = (rand++ << 8) + index;
		vcir->lvc_lxmt = 0377;
		vcir->lvc_addr = rcvaddr;
		vcir->lvc_if = rcvif;
		vcir->lvc_mnl = latmnl;
		bcopy(latmn, vcir->lvc_mn, latmnl);
		vc[index] = vcir;
		return (vcir);
	    }
	    break;
	}
    }
    return (0);
}

/*
 *		t e r m i n a t e v c
 *
 * Terminate the operation of a virtual circuit by first stopping all of the
 * slots which are active on the virtual circuit, sending a stop message (if
 * required) and releasing the virtual circuit descriptor.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to the virtual circuit descriptor.
 *	reason		= Reason code for the stop message
 *			  0 if no stop message required.
 */
terminatevc( vcir,reason )
struct lat_vc *vcir;
int reason;
{
    stopslots(vcir);
    freevc(vcir, reason);
}

/*
 *		b u i l d v c h d r
 *
 * Build a LAT virtual circuit header in a buffer.
 *
 * Outputs:		Pointer past the virtual circuit header.
 *
 * Inputs:
 *	vcir		= Pointer to the virtual circuit descriptor.
 *	hdr		= Pointer to header template.
 *	slots		= Number of slots in the message.
 *	msgtype		= Message type.
 *	rrf		= Value for rrf flag.
 */
caddr_t buildvchdr( vcir,hdr,slots,msgtype,rrf )
register struct lat_vc *vcir;
register struct vc_hdr *hdr;
int slots,msgtype,rrf;
{
    hdr->vhd_rrf = rrf;
    hdr->vhd_mas = 0;
    if (lat_master(vcir)) hdr->vhd_mas = 1; /* latmaster */
    hdr->vhd_type = msgtype;
    hdr->vhd_slots = slots;
    hdr->vhd_dstid = vcir->lvc_remid;
    hdr->vhd_srcid = vcir->lvc_locid;
    hdr->vhd_seq = vcir->lvc_nxmt;
    vcir->lvc_hxmt = vcir->lvc_nxmt++;

    return ((caddr_t)hdr + sizeof(struct vc_hdr));
}

/*
 *		v c r u n
 *
 * Try to build and send a run message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 */
vcrun( vcir )
register struct lat_vc *vcir;
{
    register struct mbuf *m,*pm,*page;
    int slots;

    if (vcir->lvc_rrf == 0)
    {
	vcir->lvc_resource = 0;
	if (m = m_get(M_DONTWAIT, MT_DATA))
	{
	    if (pm = m_get(M_DONTWAIT, MT_DATA))
	    {
		MCLGET(pm,page);

		if (page)
		{
		    pm->m_len = 0;
		    m->m_off = MMINOFF + LATOFFSET;
		    m->m_next = pm;
		    slots = buildslots(vcir, pm);
		    if (slots == 0)
		    {
			m->m_next = 0;
			m_free(pm);
			if (lat_master(vcir))	/* latmaster */
			    /*
		 	     * do not send if balanced mode
			     */
			    if (vcir->lvc_rcvact & 0x40)
			    {
				m_freem(m);
				return;
			    }
		    }
		    (void)buildvchdr(vcir, mtod(m, struct vc_hdr *),
			slots, MSG_RUN, (int)vcir->lvc_rrf);
		    m->m_len = sizeof(struct vc_hdr);
		    INC(lco_xmtframes);
		    /* debug */
		    ENQUEUE(&vcir->lvc_xmtq, m);
		    latsend(vcir);
		    return;
		}
		m_free(pm);
	    }
	    m_free(m);
	}
	/*
	 * Start resource recovery timer.
	 */
	vcir->lvc_resource = LAT_RTIMER;
    }
}

/*
 *		v c s t a r t
 *
 * Try to send a start message in response to a received start message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 */
vcstart( vcir )
register struct lat_vc *vcir;
{
    register struct mbuf *m,*page;
    register caddr_t sptr,nptr;
    char ch;

    if (m = m_get(M_DONTWAIT, MT_DATA))
    {
	MCLGET(m,page);
	if (page)
	{
	    vcir->lvc_state = VST_RUNNING;
	    sptr = buildvchdr(vcir, (struct vc_hdr *)page, 0, MSG_START, 0);
	    bcopy((char *)&startvc, (char *)sptr, sizeof(struct vc_start));
	    sptr += sizeof(struct vc_start);
	    if (*sptr++ = lat_hostnamelen)
	    {
		nptr = lat_hostname;
		while (ch = *nptr++)
		{
		    if (('a' <= ch) && (ch <= 'z')) ch = ch -'a' + 'A';
		    *sptr++ = ch;
		}
	    }
	    /*
	     * Fill in a null system name and location.
	     */
	    *sptr++ = 0;
	    *sptr++ = 0;
	    /*
	     * Terminate the parameters.
	     */
	    *sptr++ = 0;
	    m->m_len = (int)sptr - (int)page;
	    INC(lco_xmtframes);
	    ENQUEUE(&vcir->lvc_xmtq, m);
	    latsend(vcir);
	    return;
	}
	m_free(m);
    }
}

/*
 *		v c s t o p
 *
 * Try to build and send a virtual circuit stop message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	latif		= Network interface to use.
 *	srcid		= Virtual circuit source id.
 *	dstid		= Virtual circuit destination id.
 *	reason		= Stop reason code.
 */
vcstop( latif,srcid,dstid,reason )
struct ifnet *latif;
u_short srcid,dstid;
int reason;
{
    register struct mbuf *m;
    register struct vc_hdr *hdr;
    register caddr_t ptr;

    if (m = m_get(M_DONTWAIT, MT_DATA))
    {
	struct lat_vc *vcir;

	m->m_off = LATOFFSET;
	hdr = mtod(m, struct vc_hdr *);
	hdr->vhd_rrf = 0;
	hdr->vhd_mas = 0;
	hdr->vhd_type = MSG_STOP;
	hdr->vhd_slots = 0;
	hdr->vhd_dstid = dstid;
	hdr->vhd_srcid = 0;
	if (vcir = vc[srcid & 0377])
	{
	    hdr->vhd_seq = vcir->lvc_nxmt;
	    hdr->vhd_ack = vcir->lvc_ack;
	}
	else
	    hdr->vhd_seq = hdr->vhd_ack = 0;
	ptr = (caddr_t)hdr + sizeof(struct vc_hdr);
	*ptr++ = reason;
	*ptr++ = 0;
	m->m_len = (int)ptr - (int)hdr;
	INC(lco_xmtframes);
	if (lat_traceso)
	    ltt_trace(0,0,m,rcvaddr.lat_addr);
	(*latif->if_output)(latif, m, &rcvaddr);
    }
}


/*
 *
 *     c h e c k h o s t n a m e
 *
 * check to match host name
 * 
 * Output:	1: host name matched
 *		0: host name not matched
 *
 * Input:	
 *	vst	= Pointer to VC_START message
 */
checkhostname(vst)
char *vst;
{
    char *p = vst + sizeof(struct vc_start);
    int slavenamelen = (int)*p++;
    char *slave_name = p, *host_name = lat_hostname, s, h;

    if (slavenamelen > lat_hostnamelen) slavenamelen = lat_hostnamelen;
    if (slavenamelen != lat_hostnamelen) return (0);
    while (slavenamelen--)
    {
        h = *host_name, s = *slave_name;
        if (h != s)
            if ((h >= 'a' && h <= 'z' && ((h - 0x20) != s)) ||
                (h >= 0x0e0 && h <= 0x0ff && ((h - 0x20) != h)))
                if ((s >= 'a' && s <= 'z' && ((s - 0x20) != h)) ||
                    (s >= 0x0e0 && s <= 0x0ff && ((s - 0x20) != h))) 
                    return (0);
        slave_name++, host_name++;
    }
    return (1);
}


/*
 *
 *     c h e c k m a s t e r n a m e
 *
 * check for valid master name
 * 
 * Output:	1: valid master name
 *		0: invalid master name
 *
 * Input:	
 *	vst	= Pointer to VC_START message
 */
checkmastername(vst)
u_char *vst;
{
    u_char *p = vst + (u_char)sizeof(struct vc_start);
    int masternamelen = (int)*p++;

    p += masternamelen;           /* skip slave name field */
    masternamelen = (int)*p++;
    if (!masternamelen) return (0);
    latmnl = masternamelen; latmn = (char *)p; 	/* save for later */
    for ( ;masternamelen--; p++)
    {
        if ((*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || 
            (*p>='a' && *p<='z') || (*p=='$') || (*p=='-') || 
            (*p=='.') || (*p=='_') || (*p>=0x0c0 && *p<=0x0ff)) continue;
        else return (0);
    }
    return (1);
}
