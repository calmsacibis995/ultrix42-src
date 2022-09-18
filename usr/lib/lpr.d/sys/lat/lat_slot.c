#ifndef lint
static char *sccsid = "@(#)lat_slot.c	4.1.1.3	2/29/88";
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
 *      Chung Wong - 1/7/88                                             *
 *		Add check for valid credit count in DATA slots 		*
 *		and ATTENTION slot for conformance.			*
 *                                                                      *
 *      Chung Wong - 7/17/87                                            *
 *		Added '*dptr++ = 0' in generic_start() to ensure        *
 *		START SLOT end with 0's.                                *
 *                                                                      *
 *	Peter Harbo - 4/15/86						*
 *		Modification of newslot() routine for new start slot    *
 *		format.							*
 *                                                                      *
 ************************************************************************/


/*	lat_slot.c	0.0	11/9/84	*/
/*	lat_slot.c	2.0	04/15/86 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/tty.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"


extern struct lat_slot sl[];
extern struct sclass *sclass[];
extern int maxclass;
extern struct lat_counters latctrs;
extern u_char slotsinuse;

static struct slot_hdr shdr;
struct lat_slot *findslot();
caddr_t buildvchdr();

/*
 * LAT slot management.
 */

/*
 *		d e m u x
 *
 * De-multiplex a received run message among the slots present in the message.
 *
 * Outputs:		Pointer to remaining MBUF chain.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	m		= Pointer to an MBUF chain containing the slot data.
 *	slots		= Number of slots present in the message.
 */
struct mbuf *demux( vcir,m,slots )
register struct lat_vc *vcir;
register struct mbuf *m;
int slots;
{
    register struct slot_hdr *hdr;
    register struct lat_slot *slot;
    u_char slotlen;


    vcir->lvc_rcvact |= 1;
    while (slots-- && m)
    {
	if (m = LAT_PULLUP(m, sizeof(struct slot_hdr)))
	{
	    hdr = mtod(m, struct slot_hdr *);
	    shdr = *hdr;
	    slotlen = shdr.shd_count;
	    m_adj(m, sizeof(struct slot_hdr));
	    if (shdr.shd_type == SLOT_START)
	    {
		if (m = LAT_PULLUP(m, min(slotlen,MLEN) ) ) 
		{
		    newslot(vcir, m);
		}
	    }
	    else
	    {
		if (slot = findslot(vcir))
		{
		    switch (shdr.shd_type)
		    {
			case SLOT_DATA_A:
			    if (slotlen)
			    {
                                if (slot->lsl_remcredits >= 2) goto badslot;
				(*slot->lsl_scl->scl_rdataa)(vcir, slot, m, slotlen);
                                if (lat_master(vcir)) /* latmaster */
				    slot->lsl_attsize++; 
                                else 
				    slot->lsl_remcredits++;
			    }
			    break;

			case SLOT_DATA_B:
			    if (slotlen)
			    {
                                if (slot->lsl_remcredits >= 2) goto badslot;
				(*slot->lsl_scl->scl_rdatab)(vcir, slot, m, slotlen);
				slot->lsl_remcredits++;
                                vcir->lvc_rcvact &= 0x0bf; 
			    }
			    break;

			case SLOT_ATT:
                            /*
                             * credit filed in attention slot MBZ
                             */
                            if (shdr.shd_credits) goto badslot;
			case SLOT_STOP:
			    (*slot->lsl_scl->scl_rother)(vcir, slot, m, &shdr);
			    break;

			/*
			 * Bad slot type received. Count the error and ignore
			 * the slot.
			 */
badslot:
			default:
			    INC(lco_badslots);
			    terminatevc(vcir, STOP_BADFORMAT);
			    break;
		    }
		}
	    }
	    /*
	     * Remove the slot data from the MBUF chain.
	     */
	    m_adj(m, (slotlen+1) & ~1);
	}
    }
    vcir->lvc_rcvact &= 0x0fe; 
    return (m);
}

/*
 *		b u i l d s l o t h d r
 *
 * Build a slot header in the specified buffer.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	slhdr		= Pointer to the slot header to be built.
 *	slot		= Pointer to slot descriptor.
 *	count		= Size of the slot data.
 *	msgtype		= Slot message type.
 *	reason		=  0 to include credits in slot header
 *			!= 0 code to place in reason field
 */
buildslothdr( slhdr,slot,count,msgtype,reason )
register struct slot_hdr *slhdr;
struct lat_slot *slot;
int count,msgtype,reason;
{
    slhdr->shd_dstid = slot->lsl_remid;
    slhdr->shd_srcid = slot->lsl_locid;
    slhdr->shd_count = count;
    if (reason)
    {
	slhdr->shd_reason = reason;
    }
    else
    {
	if (lat_master(slot->lsl_vc))	/* latmaster */
	    if (slot->lsl_attsize)
	    {
	 	register struct tty *tp = (struct tty *)slot->lsl_data;

		if (!tp->t_rawq.c_cc)
		{
		    slot->lsl_remcredits = slot->lsl_attsize;
		    slot->lsl_attsize = 0;
		}
	    }
	slhdr->shd_credits = slot->lsl_remcredits;
	slot->lsl_remcredits = 0;
    }
    slhdr->shd_type = msgtype;
}

/*
 *		n e w s l o t
 *
 * Try to allocate and start a new slot on the specified virtual circuit.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	m		= Pointer to MBUF chain holding the start slot.
 */
newslot( vcir,m )
register struct lat_vc *vcir;
struct mbuf *m;
{
    register struct lat_slot *slot;
    register struct slot_start *sst = mtod(m, struct slot_start *);
    register int i;
    register struct sclass *class;
    int reason = STOP_SLINVSERV;
    register struct slot_hdr *slhdr;
    struct mbuf *m0;

    /*
     * Make sure that we support the requested service class.
     */
    if ((sst->sst_class <= MAXCLASS) && (class = sclass[sst->sst_class]))
    {
	/*
	 * Scan the system slot table for a free one.
	 */
	for (slot = sl,i = 0; i < LAT_MAXSLOTS; slot++,i++)
	{
	    if (slot->lsl_state == SST_FREE)
	    {
	      if ((class->scl_state == LST_RUNNING) && 
	       (slot->lsl_data = (caddr_t)(*class->scl_new)(vcir, slot,sst,
			&reason,shdr.shd_count)))
		{

		    /*
		     * Initialise the new slot database.
		     */
		    slotsinuse++;
		    vcir->lvc_act++;
		    slot->lsl_vc = vcir;
		    slot->lsl_class = sst->sst_class;
		    slot->lsl_state = SST_STARTING;
		    slot->lsl_locid = i + 1;
		    slot->lsl_remid = shdr.shd_srcid;
		    slot->lsl_remcredits = 2;
		    slot->lsl_loccredits = shdr.shd_credits;
		    slot->lsl_attsize = sst->sst_minAsize;
		    slot->lsl_datasize = sst->sst_minDsize;
		    slot->lsl_scl = class;
                    /* 
                     * should be able to reuse a VC before torn down
                     */
	            if (vcir->lvc_state == VST_STOPING)
	               vcir->lvc_state = VST_RUNNING;
		    return;
		}
	      else
	      {
		  if ((reason == STOP_SLINUSE) || (reason == STOP_SLNAME) ||
			(reason == STOP_SLINVSLOT) || (reason == STOP_SLNORES))
			break;
	      }
	    }
	}
    }
	
    /*
     * If this was the first slot on this virtual circuit, we should stop the
     * virtual circuit after sending the reject slot.
     */
    if (vcir->lvc_act == 0)
    {
	vcir->lvc_state = VST_STOPING;
    }
    /*
     * Build and send a reject slot because we cannot add the new slot.
     */
    if (m0 = m_get(M_DONTWAIT, MT_DATA))
    {
	slhdr = (struct slot_hdr *)buildvchdr(vcir,
		mtod(m0, struct vc_hdr *), 1, MSG_RUN, 0);
	slhdr->shd_dstid = shdr.shd_srcid;
	slhdr->shd_srcid = 0;
	slhdr->shd_count = 0;
	slhdr->shd_reason = reason;
	slhdr->shd_type = SLOT_REJECT;
	m0->m_len = sizeof(struct vc_hdr) + sizeof(struct slot_hdr);
	vcir->lvc_rrf = 1;
	ENQUEUE(&vcir->lvc_xmtq, m0);
	latsend(vcir);
    }
}

/*
 *		b u i l d s l o t s
 *
 * Build the output slot information into the single cluster data buffer
 * provided.
 *
 * Outputs:		Number of slots built.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	m		= Pointer to MBUF which describes the data buffer.
 */
int buildslots( vcir,m )
register struct lat_vc *vcir;
register struct mbuf *m;
{
    register struct lat_slot *slot;
    register struct slot_hdr *slhdr;
    register int i;
    int islots,slots = 0,change;

    do
    {
	islots = slots;
	for (slot = sl,i = 0; i < LAT_MAXSLOTS; slot++,i++)
	{
	    if (slot->lsl_vc == vcir)
	    {
		slhdr = mtoe(m, struct slot_hdr *);
		switch (slot->lsl_state)
		{
		    case SST_RUNNING:
			if (slot->lsl_bslot)
			{
			    if (change = (*slot->lsl_scl->scl_sdatab)(vcir, slot, m))
			    {
				slot->lsl_bslot = 0;
			    }
			}
			else
			{
			    change = (*slot->lsl_scl->scl_sdataa)(vcir, slot, m);
			}
			if (change)
			{
			    slots += change;
			    if (slhdr->shd_count)
			    {
				slot->lsl_loccredits -= change;
			    }
			}
			break;

		    case SST_STARTING:
			if ((change = (*slot->lsl_scl->scl_sother)(vcir, slot, m)) == -1)
			{
			    change = generic_start(vcir, slot, m);
			}
			if (change)
			{
			    slots++;
			slot->lsl_state = SST_RUNNING;
			}
			break;

		    case SST_REJECT:
			if ((change = (*slot->lsl_scl->scl_sother)(vcir, slot, m)) == -1)
			{
			    change = generic_reject(vcir, slot, m);
			}
			if (change)
			{
			    slots++;
			    release_slot(slot);
			}
			break;

		    case SST_STOP:
			if ((change = 
			    (*slot->lsl_scl->scl_sother)(vcir, slot, m)) == -1)
			{

			    change = generic_stop(vcir, slot, m);
			}
			if (change)
			{
			    slots++;
			    release_slot(slot);
			}
			break;
		}
		m->m_len = (m->m_len + 1) & ~1;
	    }
	}
    } while (islots != slots);
    return (slots);
}

/*
 *		t e r m i n a t e s l o t
 *
 * Terminate the operation of the specified slot.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	slot		= Pointer to the slot descriptor.
 */
terminateslot( slot )
register struct lat_slot *slot;
{
    int s = splnet();

    if (slot->lsl_vc)
    {
	slot->lsl_state = SST_STOP;
	slot->lsl_reason = STOP_SLDISC;
	vcrun(slot->lsl_vc);
    }
    splx(s);
}

/*
 *		s t o p s l o t s
 *
 * Stop all of the slots running on the specified virtual circuit.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 */
stopslots( vcir )
struct lat_vc *vcir;
{
    register struct lat_slot *slot;
    register int i;

    for (slot = sl,i = 0; i < LAT_MAXSLOTS; slot++,i++)
    {
	if (slot->lsl_vc == vcir)
	{
	    (*slot->lsl_scl->scl_hangup)(vcir, slot);
	    release_slot(slot);
	}
    }
}

/*
 *		f i n d s l o t
 *
 * Find the slot defined by the slot header on an incoming message, updating
 * the credits extended by the remote system.
 *
 * Outputs:		Pointer to the slot descriptor
 *			0 if none.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 */
struct lat_slot *findslot( vcir )
struct lat_vc *vcir;
{
    register struct lat_slot *slot;
    u_char srcid = 0;

    if (shdr.shd_dstid <= LAT_MAXSLOTS)
    {
	slot = &sl[shdr.shd_dstid - 1];
	if (shdr.shd_type != SLOT_STOP)
	{
	    srcid = slot->lsl_remid;
	}
	if (slot->lsl_locid == shdr.shd_dstid && srcid == shdr.shd_srcid && slot->lsl_vc == vcir)
	{
	    slot->lsl_loccredits += shdr.shd_credits;
	    return (slot);
	}
    }
    return (0);
}

/*
 *		r e l e a s e _ s l o t
 *
 * Free up the data structures associated with the specified slot.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	slot		= Pointer to slot descriptor.
 */
release_slot( slot )
struct lat_slot *slot;
{
    struct lat_vc *vcir;

    slot->lsl_state = SST_FREE;
    if (vcir = slot->lsl_vc)
    {
	slot->lsl_vc = 0;
	slotsinuse--;
	if (--vcir->lvc_act == 0)
	{
	    vcir->lvc_state = VST_STOPING;
	}
    }
}

/*
 *		g e n e r i c _ r e j e c t
 *
 * Build a generic reject slot.
 *
 * Outputs:		1 if slot successfully built
 *			0 if no slot built
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF holding the data buffer.
 */
/*ARGSUSED*/
generic_reject( vcir,slot,m )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
{
    register struct slot_hdr *slhdr = mtoe(m, struct slot_hdr *);

    if ((m->m_len + sizeof(struct slot_hdr)) < CLBYTES)
    {
	buildslothdr(slhdr, slot, 0, SLOT_REJECT,(int)slot->lsl_reason);
	m->m_len += sizeof(struct slot_hdr);
	return (1);
    }
    return (0);
}

/*
 *		g e n e r i c _ s t a r t
 *
 * Build a generic start slot.
 *
 * Outputs:		1 if slot successfully built
 *			0 if no slot built
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF holding the data buffer.
 */
/*ARGSUSED*/
generic_start( vcir,slot,m )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
{
    register struct slot_hdr *slhdr = mtoe(m, struct slot_hdr *);
    register struct slot_start *ssl = (struct slot_start *)((int)slhdr + sizeof(struct slot_hdr));
    register caddr_t dptr = (caddr_t)((int)ssl + sizeof(struct slot_start));

    if ((m->m_len + sizeof(struct slot_hdr) + sizeof(struct slot_start) + 3) < CLBYTES) 
    {
	ssl->sst_class = slot->lsl_class;
	ssl->sst_minAsize = MINASIZE;
	ssl->sst_minDsize = MINDSIZE;
	*dptr++ = 0;
	*dptr++ = 0;
	*dptr++ = 0; /* ensure packet end with 0's */
	buildslothdr(slhdr, slot, (int)dptr - (int)ssl, SLOT_START,(int)0);
	m->m_len += (int)dptr - (int)slhdr;
	return (1);
    }
    return (0);
}

/*
 *		g e n e r i c _ s t o p
 *
 * Build a generic stop slot.
 *
 * Outputs:		1 if slot successfully built
 *			0 if no slot built
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 *	slot		= Pointer to slot descriptor.
 *	m		= Pointer to MBUF holding the data buffer.
 */
/*ARGSUSED*/
generic_stop( vcir,slot,m )
struct lat_vc *vcir;
struct lat_slot *slot;
struct mbuf *m;
{
    register struct slot_hdr *slhdr = mtoe(m, struct slot_hdr *);

    if ((m->m_len + sizeof(struct slot_hdr)) < CLBYTES)
    {
	buildslothdr(slhdr, slot, 0, SLOT_STOP,(int)slot->lsl_reason);
	slhdr->shd_srcid = 0;
	m->m_len += sizeof(struct slot_hdr);
	return (1);
    }
    return (0);
}
