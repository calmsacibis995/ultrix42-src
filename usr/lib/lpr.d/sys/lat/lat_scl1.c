#ifndef lint
static char *sccsid = "@(#)lat_scl1.c	4.1.1.9	6/27/88";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1987 by		*
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
 *	Peter Harbo - 4/15/86						*
 *		Added routines to send solicit info, retrieve response  *
 *		information messages (both called by LAT socket ioctls),*
 *		changes to newslot() to accept new 5.1 start format.    *
 *                                                                      *
 *	Chung Wong - 7/17/87                                            *
 *		Added '*dptr++ = 0' in sdatab_class1() to ensure packet *
 * 		end with 0's.                                           *
 *									*
 *	Tim Burke - 12/1/87						*
 *		Changed to examine parameters in termios data structure.*
 *		Look for start and stop character in termios t_cc.	*
 *                                                                      *
 *      Chung Wong - 1/7/88                                             *
 *              Added 'wakeup(sol->sol1_solid)' for host initiated      *
 *              connection.                                             *
 *		Changed solicit timer to individual device based, using *
 *		storage after the data in the solicit mbuf.		*
 *		When start a new slot, check for specific service and	*
 *		save object and subject port names.			*
 *		In sdatab_class1(), send disable flow control when in   *
 *		RAW mode or flow control characters are not ^S/^Q. Also *
 *		added break signal bit when requested.  TTY speeds also	*
 *		sent to the DECserver (the existing DECserver does not	*
 *		yet set port speeds, though).				*
 *                                                                      *
 *      Chung Wong - 6/15/88                                            *
 *		For service rating, check if terminal is available for  *
 *		the service.  If not, set service rating to 1.		*
 *                                                                      *
 ***********************************************************************/

 /*	lat_scl1.c      2.0	4/15/86	 */


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
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/tty.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../vax/cpu.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"

extern struct lataddr LATmcast;
extern struct sclass *sclass[];
extern struct ecb statable[];
extern int nLAT1;
extern struct tty lata[];
extern u_short mtimer;
extern u_short soltimer, solxmit;
extern u_char slotsinuse;
extern struct socket *lat_traceso;
extern struct hic_entity lat_obj[]; 
extern struct lat_service lat_service[];	
extern int lat_debug; /* latmaster */


int ttrstrt(),direct_class1(),solicit_class1(),response_class1(),
	new_class1(),rdataa_class1(),rdatab_class1(),rother_class1(),
	sdataa_class1(),sdatab_class1(),sother_class1(),hangup_class1();

/*
 * LAT service class 1 support routines.
 */

/*
 * Service class 1 definition structure.
 */
struct sclass class1 =
    {
	LST_OFF,
	direct_class1,
	solicit_class1,
	response_class1,
	new_class1,
	rdataa_class1,
	rdatab_class1,
	rother_class1,
	sdataa_class1,
	sdatab_class1,
	sother_class1,
	hangup_class1
    };

/*
 * Processor rating weight indexed by processor type.
 */
static u_char weight[VAX_MAX+1] =
    {	12,			/* ??? */
	12,			/* 11/780 */
	16,			/* 11/750 */
	32,			/* 11/730 */
	12,			/* ??? */
	12,			/* ??? */
	12,			/* ??? */
	32,			/* MicroVax I */
	16			/* MicroVax II */
    };

static int class1_rating = 256;
static u_short solid;		/* Solicitation information msg ID number */


/*
 *		d i r e c t _ c l a s s 1
 *
 * Modify the directory message stored in the class 1 structure (if any)
 * and transmit it.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	ifp		= Pointer to network interface descriptor.
 */
direct_class1( ifp )
struct ifnet *ifp;
{
    struct mbuf *m;
    int services,rate = 2,serviceid,serviceid1,available,i;
    register struct direct_1 *dir;
    register caddr_t dptr;

    if (class1.scl_dmsg)
    {
	dir = mtod(class1.scl_dmsg, struct direct_1 *);
	dir->dr1_type = MSG_DR1 << 2;
	dir->dr1_Hver = dir->dr1_Lver = dir->dr1_Cver = LAT_VER;
	dir->dr1_eco = LAT_ECO;
	*(u_short *)dir->dr1_framesize = LAT_FRAMESIZE;
	dir->dr1_nodetimer = mtimer;
	dptr = (caddr_t)((int)dir + sizeof(struct direct_1));
	/*
	 * skip over the node groups, the node name and the node description.
	 */
	dptr += *dptr++;
	dptr += *dptr++;
	dptr += *dptr++;
	if (services = *dptr++)
	{
	    /*
	     * Compute the rating for this node.
	     */
	    if (cpu <= VAX_MAX)
	    {
		rate = weight[cpu];
	    }
	    rate = 255 - (rate * avenrun[0]);
	    rate = rate > 0 ? rate : 1;
	    /* if (rate != class1_rating) */
	    {
		class1_rating = rate;
		dir->dr1_change ^= CHA_SRVRAT;
		dir->dr1_inc++;
	    }
	    /*
	     * scan the services filling in the current service rating.
	     */
	    serviceid = 0;
	    while (services--)
	    {
		available = 0;
		serviceid1 = serviceid;
		if (serviceid) serviceid1 += LAT_SERVICEID - 1;
		/* 
		 * check if at least one terminal available for this service
		 */ 
		for (i = 0; i <= nLAT1; i++)
		{
		    if (lata[i].t_addr == 0 && (lata[i].t_state & TS_WOPEN) &&
			statable[i].ecb_hostinit == serviceid1)
                    {
                        available++;
                        break;
                    }
		}
		serviceid++;

		/* if no terminal available, set service rating to 1 */
		*dptr++ = available ? class1_rating : 1;
		dptr += *dptr++;
		dptr += *dptr++;
	    }
	}
	if (m = m_copy(class1.scl_dmsg, 0, M_COPYALL))
	{
	    if (lat_traceso)
		ltt_trace(0,0,m,LATmcast.lat_addr);
	    (*ifp->if_output)(ifp, m, &LATmcast);
	}
    }
}

/*
 *		s o l i c i t _ c l a s s 1
 *
 * Modify the solicit information message stored in the mbuf and
 * transmit it.
 *
 * Outputs:		None.
 * 
 * Inputs:
 *	m	=	mbuf containing solicit information message.
 *	ifp	=	pointer to network interface structure.
 *
 */
solicit_class1(m,ifp)
struct mbuf *m;
struct ifnet *ifp;
{
    register struct solicit_1 *sol; 
    register caddr_t solptr, dirptr;
    struct que *solque;
    struct mbuf *sm;
    u_char len;

    /*
     * Build an up-to-date Solicit information message.
     * An ioctl() has already placed a solicit_1 struct in
     * the class_1 structure.
     */

    sol = mtod(m, struct solicit_1 *); 
    sol->sol1_type = MSG_SOL << 2;
    sol->sol1_protofmt = 0;
    sol->sol1_Hver = sol->sol1_Cver = LAT_VER ;
    sol->sol1_Lver = LAT_VER_LOW ;
    sol->sol1_eco = LAT_ECO ;
    *(u_short *)sol->sol1_framesize = LAT_FRAMESIZE;
    *(u_short *)sol->sol1_solid = solid++;
    *(u_short *)sol->sol1_resptimer = LAT_RESPTIMER ;

    solptr = (caddr_t)&(sol->sol1_dstnodelen);
    /*
     * Skip over destination node name and length
     */	
    solptr += *solptr++;

    /*
     * Add source node group identifier list, node name directly from
     * class1.scl_dmsg
     */
    
    dirptr = mtod(class1.scl_dmsg,caddr_t);
    dirptr = (caddr_t)dirptr + sizeof(struct direct_1);
    len = *solptr = *dirptr ;
    solptr++, dirptr++;
    (void)bcopy((caddr_t)dirptr,(caddr_t)solptr,(int)len);
    dirptr += len;
    solptr += len;
    len = *solptr = *dirptr;
    solptr++, dirptr++;
    (void)bcopy((caddr_t)dirptr,(caddr_t)solptr,(int)len);
    solptr += len;

    /*
     * No services supported in this msg.
     */
    *solptr++ = 0; 

    /*
     * No parameters.
     */
    *solptr++ = 0;
    m->m_len = (short)(solptr - (caddr_t)sol);
    *solptr++ = LAT_SOLTIMER * 2;
    *solptr++ = LAT_SOLXMIT;
    
    if (lat_traceso)
	ltt_trace(0,0,m,LATmcast.lat_addr);
    if (sm = m_copy(m,0,M_COPYALL))
    {
	(*ifp->if_output)(ifp,sm,&LATmcast);
    }
    solque = &(class1.scl_smsg);
    ENQUEUE(solque,m);
    /* soltimer is used to clean up response queue, see lat_slowtimo() */
    soltimer = LAT_SOLTIMER * 2 * LAT_SOLXMIT + 2;

} /* solicit_1 */

/*
 *
 *			 r e s p o n s e _ c l a s s 1
 *
 * Match the incoming message to a solicit info message on the queue and
 * place in the response queue.
 *
 * Outputs:		None.
 *
 * Inputs:
 *      m		= Incoming response message.
 */
response_class1(m)
struct mbuf *m;
{

    struct response_1 *res,*start;
    struct solicit_1 *sol;
    struct mbuf *n, *prev, *p;
    struct que *resque;

    res = mtod(m,struct response_1 *);
    n = class1.scl_smsg.q_head;

    while ( n )
    {
        sol = mtod(n,struct solicit_1 *);

	/* 
	 * Solicit ID matches solicit ID in queue.
	 */

	if (*(u_short *)sol->sol1_solid == 
	    *(u_short *)res->rs1_solid)
	{
	    if (n == class1.scl_smsg.q_head)
	    {
	        if ( (class1.scl_smsg.q_head = n->m_act) == 0)
		     class1.scl_smsg.q_tail = 0;
	    }
	    else
	    {
	        if (n == class1.scl_smsg.q_tail)
		{
		    
		    class1.scl_smsg.q_tail = prev;
		    prev->m_act = 0;
		}
		else
		    prev->m_act = n->m_act;
	    }

	    resque = &(class1.scl_rmsg);
            if (n->m_type != MT_FREE) m_freem(n);
	    /*
	     * If the first message on the queue is more than
	     * 10 messages behind the one that just came in, drop it
	     * (limit the queue to ten mbufs).
	     */
	    if (class1.scl_rmsg.q_head)
            {
		start = mtod(class1.scl_rmsg.q_head,struct response_1 *);
	        if (*(u_short *)start->rs1_solid + 10 <
		    *(u_short *)res->rs1_solid) 
		{
		    DEQUEUE(resque,p);
		    m_freem(p);
		}
	    }
	    ENQUEUE(resque,m);
            wakeup(sol->sol1_solid); 
	    return;
        }
	else /* not found */
	{
	    prev = n;
	    n = n->m_act;
	}
    } /* while */

    /*
     * The incoming response message was not solicited. 
     * Free associated mbufs.
     */
    if (m) m_freem(m);
}

/*
 *		n e w _ c l a s s 1
 *
 * Allocate service class dependent resources for a new slot.
 *
 * Outputs:		Pointer to service class dependent data structure
 *			0 if no resources.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *      sst		= Pointer to slot start.
 *	reason		= Reason code for failure to open LAT tty.
 */
/*ARGSUSED*/
new_class1( vcir,slot,sst,reason,shd_count)
struct lat_vc *vcir;	
struct lat_slot *slot; 
struct slot_start *sst;
int *reason;	
u_char shd_count;
{
    int i;
    short shd_length;
    u_short req;
    caddr_t sst_ptr;
    char *servname,*destname=0,*portname=0;
    int servnamelen, servid;
    struct lat_service *ls = &lat_service[0];
    struct hic_entity *lp;
    int dstnamecnt, portnamecnt;

    /* Skip over slot_start structure, save object service and 
     * skip subject node descriptions.
     */
    sst_ptr = (caddr_t)((int)sst + (sizeof(struct slot_start)));
    shd_length = shd_count - (sizeof (struct slot_start));
    servnamelen = *sst_ptr;
    servname = sst_ptr + 1;
    shd_length -= (*sst_ptr + 1);
    sst_ptr += *sst_ptr++;    
    shd_length -= (*sst_ptr + 1);
    sst_ptr += *sst_ptr++;   
    
    /* If a request ID is present (parameter 2), then allocate LAT
     * terminal requested in statable[] entry
     */

    for (req = 0;shd_length > 0;)
    {
	if ( *sst_ptr == '\002' )
	{
	    sst_ptr += 2;
	    req = *(u_short *)sst_ptr;
	    break;
	}
	else
	{
	    if ( ( *sst_ptr == '\000' ) || ( *sst_ptr > 7 ) )
	    {
		break;

	    }
	    else
	    {
		if ( *sst_ptr == '\004' )
			destname = sst_ptr;	/* save object port name */
		if ( *sst_ptr == '\005' )
			portname = sst_ptr;	/* save source port name */
	        sst_ptr++;	     		
		shd_length -= (*sst_ptr + 1);
		sst_ptr += *sst_ptr++;	
		continue;
	    } 
	}
    } /* for */
    
    if (req)		/* Request ID has been defined */
    {
	for ( i = 0 ; i <= nLAT1 ; i++ )
	{
	    if (statable[i].ecb_reqid == req)
	    {
		if (lata[i].t_addr != 0)
		{
		    *(reason) = STOP_SLINUSE;
		    return(0);
		}
		else
		    break;
	    }
	    else
	    {
		if ( i == nLAT1 )
		{
		    *(reason) = STOP_SLNAME;
		    return(0);
		}
	    }	    
        }   
    }
    else 		/* Find first available port */
    {
	/* check if service name defined in lat_service[] */
        int tempi = servnamelen;
        char *tempc = servname;
        for ( ; tempi; tempi--,tempc++)
        {
            if (*tempc >= 'a' && *tempc <= 'z')
                 *tempc -= 0x20;	/* convert to upper case */
        }
	for (servid=0; servid<MAXSERVICE+1; servid++,ls++)
        {
            if (servid == MAXSERVICE || !ls->id) 
	    {
		servid = 0;
		break;
	    }
	    if (bcmp(ls->name, servname, servnamelen) == 0)
 	    {
	 	servid += LAT_SERVICEID;
                break;
            }
	}

	for (i = 0; i <= nLAT1; i++)
	{
	    if (lata[i].t_addr == 0 && (statable[i].ecb_hostinit == servid) 
                && (lata[i].t_state & TS_WOPEN) )
		break;
	    else
	    {
		if ( i == nLAT1 )
		{
		    *(reason) = STOP_SLNORES;
		    return (0);
		}
		continue;
	    }
	}
    }

    lata[i].t_addr = (caddr_t)slot;
    lata[i].t_state |= TS_CARR_ON;
    slot->lsl_bslot |= 1;

    ttyflush(&lata[i], FREAD | FWRITE);

    /* for non-LAT service, save destination if defined */
    lp = &lat_obj[i];
    /* save destination name if defined */
    if (destname)
    {
        dstnamecnt = *++destname;
        bcopy(++destname, lp->subj_port, dstnamecnt);
        lp->subj_portlen = dstnamecnt;
    }
    /* save server port name */
    if (portname)
    {
        portnamecnt = *++portname;
        bcopy(++portname, lp->obj_port, portnamecnt);
        lp->obj_portlen = portnamecnt;
    }
    /* get the server name */
    bcopy(vcir->lvc_mn, lp->obj_name, vcir->lvc_mnl);
    lp->obj_namelen = (u_char)vcir->lvc_mnl;

    return ((int)&lata[i]);
}

/*
 *		r d a t a a _ c l a s s 1
 *
 * Process a received data_a slot for a service class 1 device.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF chain holding the slot data.
 *	slen		= Length of the slot data.
 */
/*ARGSUSED*/
rdataa_class1( vcir,slot,m,slen )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
int slen;
{
    register struct tty *tp = (struct tty *)slot->lsl_data;
    register caddr_t dptr;
    register int count;

    /*
     * Wake up any processes waiting for input.
     */
    if ((tp->t_state & TS_ISOPEN) == 0)
    {
	wakeup((caddr_t)&tp->t_rawq);
    }
    /*
     * Pass the received characters into the terminal driver one at a time.
     */
    do
    {
	count = min(slen, m->m_len);
	dptr = mtod(m, caddr_t);
	slen -= count;
	while (count--)
	{
	    (*linesw[tp->t_line].l_rint)(*dptr++, tp);
	}
	m = m->m_next;
    } while (slen);
}

/*
 *		r d a t a b _ c l a s s 1
 *
 * Process a received data_b slot for a service class 1 device.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF chain holding the slot data.
 *	slen		= Length of the slot data.
 */
/*ARGSUSED*/
rdatab_class1( vcir,slot,m,slen )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
int slen;
{
}

/*
 *		r o t h e r _ c l a s s 1
 *
 * Process a received stop, reject or attention slot.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF chain holding the slot.
 *	slhdr		= Pointer to the slot header.
 */
/*ARGSUSED*/
rother_class1( vcir,slot,m,slhdr )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
struct slot_hdr *slhdr;
{
    switch (slhdr->shd_type)
    {
	case SLOT_STOP:
	    hangup_class1(vcir, slot);
	    release_slot(slot);
	    break;

	case SLOT_ATT:
	    break;
    }
}

/*
 *		s d a t a a _ c l a s s 1
 *
 * Try to build a data_a slot for the service class 1 device.
 *
 * Outputs:		1 if slot successfully built
 *			0 if no slot built
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF which holds the data buffer.
 */
sdataa_class1( vcir,slot,m )
struct lat_vc *vcir;
register struct lat_slot *slot;
register struct mbuf *m;
{
    register struct slot_hdr *slhdr = mtoe(m, struct slot_hdr *);
    register caddr_t dataptr = (caddr_t)((int)slhdr + sizeof(struct slot_hdr));
    register struct tty *tp = (struct tty *)slot->lsl_data;
    register int cc;
    int slotlen = 0;

    if (slot->lsl_loccredits)
    {
	if ((tp->t_state & (TS_TIMEOUT|TS_TTSTOP)) == 0)
	{
	    while (tp->t_outq.c_cc)
	    {
#ifndef RELEASE
		if (tp->t_flags & (RAW|LITOUT))
#else
 		if ((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT) || 
 	    	    ((tp->t_oflag & OPOST) == 0))
#endif
		{
		    cc = ndqb(&tp->t_outq, 0);
		}
		else
		{
#ifndef RELEASE
		    cc = ndqb(&tp->t_outq, 0200);
#else
 		    cc = ndqb(&tp->t_outq, DELAY_FLAG);
#endif
		    if (cc == 0)
		    {
			/*
			 * The first thing on the queue is a delay. Process it
			 * as normal so that we do not affect the internal
			 * operation of the tty driver too much.
			 */
			cc = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp,(int)(cc & 0x7f)+6);
			tp->t_state |= TS_TIMEOUT;
			break;
		    }
		}
		/*
		 * If there is insufficient space left in the buffer or
		 * the slot to add the data, stop now and we will come
		 * back later.
		 */
		if (((slotlen + cc ) > slot->lsl_datasize) ||
		    ((m->m_len + cc) > (1500 - sizeof(struct slot_hdr) - sizeof(struct vc_hdr))))
			break;
		bcopy(tp->t_outq.c_cf, dataptr, cc);
		dataptr += cc;
		slotlen += cc;
		m->m_len += cc;
		ndflush(&tp->t_outq, cc);
	    }
	    if (!tp->t_outq.c_cc) vcir->lvc_rcvact &= 0x7f; /* latmaster */
	    else vcir->lvc_rcvact |= 0x80; /* latmaster */
	    if (slotlen)
	    {
		buildslothdr(slhdr, slot, slotlen, SLOT_DATA_A, 0);
		vcir->lvc_rrf = 1;
		m->m_len = (m->m_len + sizeof(struct slot_hdr) + 1) & ~1;
		ltawakeup(tp);
		return (1);
	    }
	}
    }
    else /* latmaster */
	if (tp->t_outq.c_cc) vcir->lvc_rcvact |= 0x80; /* latmaster */
    if (slot->lsl_remcredits)
    {
	/*
	 * No data can be sent but we have some credits which need to be
	 * sent to the remote system. Build a data_a slot with no slot
	 * data to transfer the credits.
	 */
	if ((m->m_len + sizeof(struct slot_hdr)) < CLBYTES)
	{
	    buildslothdr(slhdr, slot, 0, SLOT_DATA_A, 0);
	    m->m_len += sizeof(struct slot_hdr);
	    return (1);
	}
    }
    return (0);
}

/*
 *		s d a t a b _ c l a s s 1
 *
 * Try to build a data_b slot for the service class 1 device.
 *
 * Outputs:		1 if slot successfully built
 *			0 if no slot built
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF which holds the data buffer.
 */
#define BREAK_RQST (SB1_BREAK|SB1_SET)>>4
static u_short speed[14] = 
	{0,50,75,110,135,150,200,300,600,1200,1800,2400,4800,9600};

sdatab_class1( vcir,slot,m )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
{
    register struct slot_hdr *slhdr = mtoe(m, struct slot_hdr *);
    register struct slotb_1 *sbptr = (struct slotb_1 *)((int)slhdr + sizeof(struct slot_hdr));
    register struct tty *tp = (struct tty *)slot->lsl_data;
    caddr_t dptr = (caddr_t) ((int)sbptr + sizeof(struct slotb_1)); 
    int passall = 0;

    if (slot->lsl_loccredits)
    {
	if ((m->m_len + sizeof(struct slot_hdr) + sizeof(struct slotb_1)) < CLBYTES)
	{
	    buildslothdr(slhdr, slot, sizeof(struct slotb_1)+11+2, SLOT_DATA_B, 0);
	    vcir->lvc_rrf = 1;
#ifndef RELEASE
	    sbptr->sb1_spout = tp->t_stopc;
	    sbptr->sb1_stout = tp->t_startc;
#else
 	    sbptr->sb1_spout = tp->t_cc[VSTOP];
 	    sbptr->sb1_stout = tp->t_cc[VSTART];
#endif
	    sbptr->sb1_spin = '\023';
	    sbptr->sb1_stin = '\021';
	    if ((tp->t_flags & RAW) || sbptr->sb1_spout!='\023' || 
		 sbptr->sb1_stout!='\021')
	        sbptr->sb1_flags = passall = SB1_DISINPUT|SB1_DISOUTPUT;
            else
	        sbptr->sb1_flags = SB1_ENAINPUT|SB1_ENAOUTPUT;
	    if ((slot->lsl_reason & BREAK_RQST) == BREAK_RQST)
		sbptr->sb1_flags |= SB1_BREAK;
	    sbptr->sb1_flags |= SB1_SET;
	    *dptr++ = 2;	/* set input speed */
	    *dptr++ = 2;
#ifndef RELEASE
	    bcopy(&speed[tp->t_ispeed], dptr, 2);
#else
	    bcopy(&speed[tp->t_cflag & CBAUD], dptr, 2);
#endif
	    dptr += 2;
	    *dptr++ = 3;	/* set output speed */
	    *dptr++ = 2;
#ifndef RELEASE
	    bcopy(&speed[tp->t_ospeed], dptr, 2);
#else
	    bcopy(&speed[tp->t_cflag_ext & CBAUD], dptr, 2);
#endif
	    dptr += 2;
	    *dptr++ = 5;	/* set transparency mode */
	    *dptr++ = 1;
	    if (passall) *dptr++ = 1;
	    else *dptr++ = 0;
            *dptr++ = 0;	/* data_b slot must terminates with 0 */
            *dptr++ = 0;
	    m->m_len += sizeof(struct slot_hdr) + sizeof(struct slotb_1) + 11 + 2;
	    return (1);
	}
    }
    return (0);
}

/*
 *		h a n g u p _ c l a s s 1
 *
 * The specified slot has been stop. Perform the necessary operations for the
 * operating system to see this.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 */
/*ARGSUSED*/
hangup_class1( vcir,slot )
struct lat_vc *vcir;
struct lat_slot *slot;
{
    int s = splimp();
    register struct tty *tp = (struct tty *)slot->lsl_data;
    int unit;

    gsignal(tp->t_pgrp, SIGHUP);
    gsignal(tp->t_pgrp, SIGCONT);
    ttyflush(tp, FREAD|FWRITE);
    tp->t_state &= ~(TS_CARR_ON|TS_ISOPEN);
    tp->t_addr = 0;
    unit = minor(tp->t_dev); 
    if (statable[unit].ecb_hostinit == 0 || 
	statable[unit].ecb_hostinit >= LAT_SERVICEID)
        lat_obj[unit].subj_portlen = lat_obj[unit].obj_portlen = 
	    lat_obj[unit].obj_namelen = 0;
    splx(s);
}

/*
 *		s o t h e r _ c l a s s 1
 *
 * Try to build a slot other than data_a or data_b for the service class 1
 * device.
 *
 * Outputs:		1  if slot was built
 *			0  if no slot built
 *			-1 if routine does not know how to build the slot
 *			   (i.e. use generic build routine).
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF chain which holds the data buffer.
 */
/*ARGSUSED*/
sother_class1( vcir,slot,m )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
{
    return (-1);
}

extern int lat_mflag;	/* latmaster */
#define LAT_MALIVE 2	/* latmaster */
/*
 *		o u t p u t _ c l a s s 1
 *
 * One of the service class 1 devices (terminals) has some output available.
 * If we have credits available, try to build a virtual circuit run message
 * with this data present and any data available from other slots running
 * on the same virtual circuit. If a received run message is being processed
 * for this virtual circuit, we can delay processing so that it will all be
 * picked up on completion of processing the received message.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	tp		= Pointer to a tty structure.
 *	bslot		= 1 if data_b slot should be sent
 *			  0 if data_a slot should be sent
 */
output_class1( tp,bslot )
struct tty *tp;
int bslot;
{
    struct lat_slot *slot;
    struct lat_vc *vcir;
    int s = splnet();

    if (slot = (struct lat_slot *)tp->t_addr)
    {
	slot->lsl_bslot |= bslot;
	vcir = slot->lsl_vc;
	/*
         * latmaster: if LAT_MALIVE set, transmit on circuit timer interval 
	 *  	      if ack queue not empty, wait
	 */
        if (lat_master(vcir) && 
	    ((lat_mflag & LAT_MALIVE) || vcir->lvc_ackq.q_head))
	    vcir->lvc_rcvact |= 0x80;
        else 
	    if (((vcir->lvc_rcvact & 0x01) == 0) && slot->lsl_loccredits)
	    {
	        vcrun(vcir);
	    }
    }
    splx(s);
}
