/*
 * if_ni.c
 */

#ifndef lint
static char *sccsid = "@(#)if_ni.c	4.3      ULTRIX  3/7/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986-89 by			*
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

#include "bvpni.h"
#if NBVPNI > 0 || defined(BINARY)

/* Modification History:
 *
 * 24-Feb-91 - jsd
 *	Allow loopback packets if COPYALL mode is set
 *
 * 28-Oct-89 - Uttam Shikarpur						
 *	Added:								
 *		1) Reporting counters for multicast packs. and bytes	
 *		2) Ability to report back the type of network interface.
 *
 * 3-May-89 Uttam Shikarpur
 *      Add support for Ethernet packet filter
 *      Moved common read code to ../net/ether_read.c
 *
 * 3-Mar-89 U. Sinkewicz
 * 	Picked up R. Bhanukitsiri changes for pmax/vax.
 *
 * 28-Dec-88 Fred L. Templin
 *	Rearranged "nistart" to schedule an interrupt for every datagram
 *	sent. (Instead of just NFS and broadcast loopbacks as was done
 *	before). This prevents temporary transmit-side livelocks in the
 *	case of many transmits enqueued WITHOUT any receive datagrams
 *	to trigger an interrupt.
 *
 * 16-Sep-88 condylis
 *	Unlocked lk_ni_softc during call to m_freem in nirecv to permit
 *	the acquisition of locks in the free routine specified in
 *	the mclgetx call for the mbuf
 *
 * 22-Aug-88 U. Sinkewicz
 *	Changed flags to KM_ALLOC calls to prevent any possibility of
 *	sleeping.  Needed for SMP.
 *
 * 22-Jun-88 Fred L. Templin
 *	Moved m_compress() code into the nistart routine, and used MCLGET
 *	instead of messing around with little mbuf's, since the	old scheme
 *	could possibly leave you with a chain that's still too long to be
 *	mapped onto a command queue entry. Fixed another bug in which the
 *	start routine wasn't checking to see if an mbuf was actually a
 *	cluster-type mbuf before checking the m_cltype field. This could
 *	possibly generate unnecessary interrupts for non-cluster mbufs.
 *	Also, added check to make sure that nioutput isn't called with
 *	the interface down.
 *
 * 10-Jun-88 lp
 *	Crank up in niinit when laying out bde's to keep reset from
 *	corrupting them.
 *
 * 15-Jan-88 lp (Larry Palmer)
 *			Minor rewrite to better use allocated mbufs.
 *			43BSD support added. NO MORE PAGE FLIPPING!
 *
 * 25-Aug-87  -- templin@decvax
 *			Added fix pointed out by Ed Ferris which subtracts
 *			the CRC field length from the length of the data field
 *			used by niget(). Also added code to avoid reporting
 *			non-error free queue exhaustions.
 * 28-Jul-87  -- lp
 *			Fix problem with multiple interfaces.
 *
 * 07-Jul-87  -- templin@decvax
 *			Fixed "ni_counters" type casts to correct a
 *			word ordering problem with the counters.
 * 31-Mar-87  -- lp	
 *			Put apa back on a reset for decnet.
 * 06-Feb-87  -- lp
 *			Check return of ifproto_type to gaurantee that
 *			we dont jump to 0.
 * 28-Jan-87  -- lp
 *			Cleanup as LINT showed some extra
 *			variables.
 *
 * 12-Dec-86  -- lp	Post FT-v2.0. Added dpaddr, Bumped up 
 *			freeq0 allocation.
 *
 * 23-Oct-86  -- lp	Type 2 mbuf are forced to return on respq.
 *
 * 2-Sep-86   -- lp	Cleanup. Bugfix for long (improperly chained)
 *			packets.
 *
 * 7-Aug-86   -- lp	Removed some printf's ('I' baseline).
 *
 * 7-Jul-86   -- lp	Fixed a timing problem in niattach.
 *
 * 5-Jun-86   -- lp 	Fixed a little bug in reset code.
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 22 May 86 -- bjg
 *	Include types.h and errlog.h for error logging.
 *
 * 21 May 86 -- lp
 *	Reworked sptdb routine to allow clear prior to setting. Bugfixes
 *	for decnet. Errlog for SUME errors.
 *
 * 08 May 86 -- lp
 *	Trailer packets work. General cleanup. Vaddr no longer saved
 *	in receive packets.
 *
 * 09 Apr 1986 -- lp
 * 	DEC AIE/NI ethernet driver
 *		By 
 * 	Larry Palmer (decvax!lp).
 *	(rev 40 or higher aie firmware needed)
 *
 */

#include "packetfilter.h"	/* NPACKETFILTER */
#include "../data/if_ni_data.c"
#include "../h/types.h"
#include "../h/errlog.h"

extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern int net_output();
struct	mbuf *niget();
int	niattach(), bvpniintr(), niprobe();
int	niinit(),nistart(),niioctl(),nireset(), ni_ignore;
u_char  ni_multi[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00};
u_char  ni_notset[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u_short nistd[] = { 0 };

#define vaddr(y) \
((char *)(((unsigned long)(ptetosv((struct pte *)((y)->pt_addr))))+(y)->offset))

int nidebug = 0;
int ni_fqe = 0;

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 * We initialize big mbuf storage for receives here.
 */
niattach(ni)
	struct ni *ni;
{
	register struct ni_softc *ds = &ni_softc[ni->unit];
	register struct ifnet *ifp = &ds->ds_if;
	int i=0;
	struct sockaddr_in *sin;

	/* Initialize SMP lock */
	lockinit(&ds->lk_ni_softc, &lock_device15_d);

	ds->ds_devid = 0;

	ifp->if_unit = ni->unit;
	ifp->if_name = "ni";
	ifp->lk_softc = &ds->lk_ni_softc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_type = IFT_ETHER;
	ifp->if_flags |= IFF_BROADCAST|IFF_DYNPROTO|IFF_NOTRAILERS;
	((struct arpcom *)ifp)->ac_ipaddr.s_addr = 0;

	/* 
	 * Set ni to initialized state
	 */
	i=0;
	while((ni->ni_regs->ps&PS_STATEMASK) != PS_UNDEFINED) {
	    if(++i > 200) {
		printf("ni%d in wrong state\n", ni->unit);
		return;
	    }
	    DELAY(100000);
	}

	DELAY(200000);
	ni->ni_regs->ps &= ~PS_OWN;
	ni->ni_regs->pc &= ~PC_OWN;

	/* Go to initialized state */
	ni->ni_regs->pc = ni->phys_pqb|PC_INIT|PC_OWN;
	DELAY(1000000);
	while((ni->ni_regs->pc&PC_OWN)) 
		;
	i=0;
	while((ni->ni_regs->ps&PS_INITIALIZED) == 0) {
		if(i++ > 15) {
			printf("ni%d Cannot initialize\n", ni->unit);
			return;
		} else 
			DELAY(1000000);
	}
	ni->ni_regs->ps &= ~PS_OWN;

	/*
	 * Fill the multicast address table with unused entries (broadcast
	 * address) so that we can always give the full table to the device
	 * and we don't have to worry about gaps.
	 */
	for(i=0; i<NMULTI; i++) {
		ds->ds_muse[i] = 0;
		bcopy(ni_multi, ds->ds_multi[i], 8);
	}

	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;

	ifp->if_init = niinit;
	ifp->if_output = net_output;
	ifp->if_start = nistart;
	ifp->if_ioctl = niioctl;
	ifp->if_reset = nireset;
	ifp->d_affinity = ALLCPU;	/* 8.9.88.us  For nonsym drivers. */
	/*
	 * Fill out the if_version field with HW (and SW) info
	 */
	bcopy("DEC DEBNA Ethernet Interface", ifp->if_version, 28);
	ifp->if_version[28] = '\0';

	/*
	 * Hardware address unavailable at this point. (Only avail.
	 * after niinit().
	 */
	printf("ni%d: %s\n", ifp->if_unit, ifp->if_version);
#if NPACKETFILTER > 0
	/* Tell the packet filter that we are here */
	attachpfilter(&(ds->ds_ed));
#endif NPACKETFILTER
	if_attach(ifp);
}

/*
 * Reset of interface after reset.
 */
nireset(unit)
	int unit;
{
	register struct ni *ni;
	register struct ni_softc *ds = &ni_softc[unit];
	register char *mem;
	register int i;
	struct ifnet *ifp = &ds->ds_if;

	if (unit >= nNI || (ni = &niinfo[unit]) == 0 || ni->alive == 0)
		return;
	printf("reset ni%d %x %x %x %x\n", unit, ni->ni_regs->pc,
		ni->ni_regs->ps, ni->ni_regs->pe, ni->ni_regs->pd);
	if((ni->ni_regs->ps&PS_STATEMASK) != PS_ENABLED) {

	/* Eat freeq 2 */
		while((mem = (char *)remqhi(&ni->freeq2, NI_MAXITRY)) != (char *)QEMPTY) {
			struct _bd *bde = (struct _bd *)ni->ni_pqb->ni.bdt_base;
			bde += ((struct ni_data *)mem)->cbufs[0].bdt_index;
			for(i=0; i<NI_RBUF; bde++, i++) {
				if(i == 1) {
					KM_FREE(vaddr(bde), KM_CLUSTER);
				} else {
					KM_FREE(vaddr(bde), KM_DEVBUF);
				}
			}
			KM_FREE(mem, KM_DEVBUF);
		}
	/* Eat freeq0 & freeq1 */
		while((mem = (char *)remqhi(&ni->freeq0, NI_MAXITRY)) != (char *)QEMPTY) {
			KM_FREE(mem, KM_DEVBUF);
		}
		while((mem = (char *)remqhi(&ni->freeq1, NI_MAXITRY)) != (char *)QEMPTY) {
			KM_FREE(mem, KM_DEVBUF);
		}

	/* Eat the respq */
		while((mem = (char *)remqhi(&ni->respq, NI_MAXITRY)) != (char *)QEMPTY) {
			if(((struct ni_data *)mem)->opcode == DGREC) {
			struct _bd *bde = (struct _bd *)ni->ni_pqb->ni.bdt_base;
			bde += ((struct ni_data *)mem)->cbufs[0].bdt_index;
			for(i=0; i<NI_RBUF; bde++, i++) {
				if(i == 1) {
					KM_FREE(vaddr(bde), KM_CLUSTER);
				} else {
					KM_FREE(vaddr(bde), KM_DEVBUF);
				}
			}
			KM_FREE(mem, KM_DEVBUF);
			} else if(((struct ni_data *)mem)->opcode == SNDDG) {
				KM_FREE(mem, KM_DEVBUF);
			} else {
				KM_FREE(mem, KM_DEVBUF);
			}
		}
			

		if((ni->ni_regs->ps&PS_STATEMASK) == PS_STOPPED) {
			ni->ni_regs->ps &= ~PS_OWN;
			ni->ni_regs->pc = PC_RESTART|PC_OWN;
			DELAY(1000000);
			while((ni->ni_regs->pc&PC_OWN)) 
				;
		}
		ni->ni_regs->ps &= ~PS_OWN;
		/* 
	 	* Set ni to initialized state
	 	*/
		i=0;
		while((ni->ni_regs->ps&PS_STATEMASK) != PS_UNDEFINED) {
	    	if(++i > 200) {
			printf("ni%d in wrong state\n", ni->unit);
			return(0);
	    	}
	    	DELAY(100000);
		}
		ni->ni_regs->ps &= ~PS_OWN;
		/* Go to initialized state */
		ni->ni_regs->pc = ni->phys_pqb|PC_INIT|PC_OWN;
		DELAY(100000);
		while((ni->ni_regs->pc&PC_OWN)) 
			;
		if((ni->ni_regs->ps&PS_INITIALIZED) == 0) {
			printf("ni%d Cannot initialize\n", ni->unit);
			printf("nireset: init failed - %d %x %x %x %x\n",
				unit, ni->ni_regs->pc, ni->ni_regs->ps,
				ni->ni_regs->pe, ni->ni_regs->pd);
			return(0);
		}
		ni->ni_regs->ps &= ~PS_OWN;
		ni->ni_regs->pe = 0;
		ni->ni_regs->pd = 0;
		ifp->if_flags &= ~(IFF_RUNNING);
		untimeout(niinit, unit);
		timeout(niinit, unit, 1);
		return(0);
	}
	return(1);
}

/*
 * Initialization of interface; clear recorded pending
 * operations.
 */
niinit(unit)
	int unit;
{
	register struct ni_softc *ds = &ni_softc[unit];
	register struct ni *ni = &niinfo[unit];
	register struct nidevice *addr;
	struct ifnet *ifp = &ds->ds_if;
	int s,i,empty;

	/* not yet, if address still unknown */
	/* DECnet must set this somewhere to make device happy */

	if (ifp->if_addrlist == (struct ifaddr *)0)
			return;
	if (ifp->if_flags & IFF_RUNNING)
		return;

	/* 
	 * Set ni to enabled state. Still dont have freeq ready.
	 */

	addr = (struct nidevice *)ni->ni_regs;
	while((addr->pc&PC_OWN))
		;
	addr->pc = PC_ENABLE|PC_OWN;
	/* Should interrupt here */
	while((addr->pc&PC_OWN))
		;
	while((addr->ps&PS_OWN))
		;
	i = 0;
	while((addr->ps&PS_STATEMASK) != PS_ENABLED) {
		if(i++ > 500000) {
			printf("ni%d Cannot enable\n", ni->unit);
			return;
		}
	}
	addr->ps &= ~PS_OWN;

	s = splimp();
	/* 
	 * Setup message freeq Just a few so its not empty 
	 */
	{
		struct ni_msg *ni_msg;
		for(i=0; i<6; i++) {
		KM_ALLOC(ni_msg, struct ni_msg *, sizeof(struct ni_msg), KM_DEVBUF, KM_NOW_CL);
		if (ni_msg == 0){
			printf("ni%d Init out of memory - ni_msg\n",ni->unit);
			return;
		}
		if((empty=insqti(ni_msg, &ni->freeq0, NI_MAXITRY)) > 0)
			printf("insqti failed\n");
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				;
			addr->pc = PC_FREEQNE|PC_MFREEQ|PC_OWN;
			while((addr->pc&PC_OWN))
				;
		}
		}
	}
	/* Setup xmit buffers (empty) */
	for(i=0; i<NI_FREEQ_1; i += NI_NUMBUF) {
		struct ni_data *ni_data;
		struct _bd *bde;
		int j;
		KM_ALLOC(ni_data, struct ni_data *, NI_DQSIZE, KM_DEVBUF, KM_NOW_CL);
		if (ni_data == 0){
			printf("ni%d Init out of memory - ni_data\n",ni->unit);
			return;
		}
		ni_data->status = 0;
		ni_data->dg_len = NI_DQSIZE-NI_DQHEAD;
		ni_data->dg_ptdb_index = 1;
		ni_data->opcode = SNDDG;
		for(j=0; j<NI_NUMBUF; j++) {	
			ni_data->cbufs[j].offset = 0;
			ni_data->cbufs[j].buffer_key = 1;	
			bde = (struct _bd *) ni->ni_pqb->ni.bdt_base;
			bde += i+j;
			bde->key = 1;
			bde->valid = 0;
			ni_data->cbufs[j].bdt_index = i+j;
		}
		if((empty=insqti(ni_data, &ni->freeq1, NI_MAXITRY)) > 0)
			printf("xmit insqti failed %d\n", i);
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				;
			addr->pc =  PC_FREEQNE|PC_DFREEQ|PC_OWN;
			while((addr->pc&PC_OWN))
				;
		}
	}
	/*
	 * Setup recv buffers 
	 */	
	/* Note that these have mbuf data areas associated */
	for(i=NI_FREEQ_1; i<(NI_FREEQ_1+NI_FREEQ_2); i += NI_RBUF) {
		struct ni_data *ni_data;
	  	struct _bd *bde;
	  	char *buffer;
		int j=0;
		KM_ALLOC(ni_data, struct ni_data *, sizeof(struct ni_data), KM_DEVBUF, KM_NOW_CL);
		if (ni_data == 0){
			printf("ni%d Init out of memory - ni_data\n",ni->unit);
			return;
		}
		/* header + space for DGRLEN buffer names */
		ni_data->dg_len = NI_DGRLEN;
		ni_data->opcode = DGREC;
		ni_data->dg_ptdb_index = 2;
		for(j=0; j<NI_RBUF; j++) {
		bde = (struct _bd *) ni->ni_pqb->ni.bdt_base;
		bde += i+j;
/* Need to have 1st buffer be ether_header */
		if(j == 0) {
			KM_ALLOC(buffer, char *, sizeof(struct ether_header), KM_DEVBUF, KM_NOWAIT);
		if (buffer == 0){
			printf("ni%d Init out of memory - buffer\n",ni->unit);
			return;
		}
			bde->buf_len = sizeof(struct ether_header);
			bde->pt_addr = svtopte(buffer);
			bde->offset = (unsigned)buffer&PGOFSET;
		} else if ( j == 1) {
			KM_ALLOC(buffer, char *,M_CLUSTERSZ,KM_CLUSTER, KM_NOWAIT); 
		if (buffer == 0){
			printf("ni%d Init out of memory - buffer\n",ni->unit);
			return;
		}
			bde->pt_addr = svtopte(buffer);
			bde->offset = (unsigned)buffer&PGOFSET;
			bde->buf_len = NI_MAXPACKETSZ;
		} else {
			cprintf("Too many recv desc");
		}
		bde->key = 1;
		bde->valid = 1;
		ni_data->cbufs[j].offset = 0;
		ni_data->cbufs[j].s_len = bde->buf_len;
		ni_data->cbufs[j].bdt_index = i+j;
		ni_data->cbufs[j].buffer_key = 1;
		}
		if((empty=insqti(ni_data, &ni->freeq2, NI_MAXITRY)) > 0)
			printf("recv insqti failed %d\n", i);
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				;
			addr->pc = PC_FREEQNE|PC_RFREEQ|PC_OWN;
			while((addr->pc&PC_OWN))
				;
		}
	}

	splx(s);

	while((addr->ps&PS_OWN))
		;
	/* Write Parameters */
	{
		struct ni_msg *ni_msg;
		KM_ALLOC(ni_msg, struct ni_msg *, sizeof(struct ni_msg), KM_DEVBUF, KM_NOW_CL);
		if (ni_msg == 0){
			printf("ni%d Init out of memory - ni_msg\n",ni->unit);
			return;
		}
		ni_msg->opcode = SNDMSG;
		ni_msg->status = 0;
		ni_msg->msg_len = sizeof(struct ni_param) + 6;
		ni_msg->ni_opcode = NIOP_WPARAM;

		/* Someone has set the apa at least once */
		if((bcmp(ds->ds_addr, ni_notset, 6) != 0))
		bcopy(ds->ds_addr,((struct ni_param *)&ni_msg->text[0])->apa, 6);
		((struct ni_param *)&ni_msg->text[0])->flags = NI_PAD;
		if((empty=insqti(ni_msg, &ni->comq0, NI_MAXITRY)) > 0)
			printf("insqti failed\n");
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				; 
			addr->pc = PC_CMDQNE|PC_CMDQ0|PC_OWN;
			while((addr->pc&PC_OWN))
				; 
		}
	}
	{
		struct ni_msg *ni_msg;
		KM_ALLOC(ni_msg, struct ni_msg *, sizeof(struct ni_msg), KM_DEVBUF, KM_NOW_CL);
		if (ni_msg == 0){
			printf("ni%d Init out of memory - ni_msg\n",ni->unit);
			return;
		}
		ni_msg->opcode = SNDMSG;
		ni_msg->status = 0;
		ni_msg->msg_len = sizeof(struct ni_param) + 6;
		ni_msg->ni_opcode = NIOP_RCCNTR;
		if((empty=insqti(ni_msg, &ni->comq0, NI_MAXITRY)) > 0)
			printf("insqti failed\n");
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				; 
			addr->pc = PC_CMDQNE|PC_CMDQ0|PC_OWN;
			while((addr->pc&PC_OWN))
				; 
		}
	}
	/* Let all these commands complete */
	while((addr->ps & PS_OWN))
		;
	/* Hardware address not set yet */
	while((bcmp(ds->ds_addr, ni_notset, 6) == 0))
		;
	/* Set up PTDB's */
	ni_sptdb(ni,0,1,1,0,1);	/* #1 for send queue returns */
	ni_sptdb(ni,ETHERTYPE_IP,2,2,PTDB_UNK|PTDB_BDC,1); /* #2 all incoming */
	ds->nxmit = 0;
	ifp->if_flags &= ~IFF_OACTIVE;
	/* The board is up (ooo rah) */
	s = splimp();
	smp_lock(&ds->lk_ni_softc, LK_RETRY);
	ds->ds_if.if_flags |= IFF_UP|IFF_RUNNING;
	nistart(unit);				/* queue output packets */
	smp_unlock(&ds->lk_ni_softc);
	splx(s);
}

/*
 * Setup output on interface.
 */
nistart(unit)
	int unit;
{
	register struct ni *ni = &niinfo[unit];
	register struct mbuf *m, *m0;
	register struct _bd *bde;
	register struct ni_data *nid;
	register int len, curindex;
	struct mbuf *mprev;
	struct ni_softc *ds = &ni_softc[unit];
	struct nidevice *addr = (struct nidevice *)ni->ni_regs;
        int empty;


	if((addr->ps&PS_STATEMASK) != PS_ENABLED) {
		cprintf("ni%d state(nistart) %x %x %x %x\n", unit, addr->ps, 
			addr->pe, addr->pc, addr->pd);
		if((addr->ps&PS_STATEMASK) == PS_UNDEFINED ||
		   (addr->ps&PS_STATEMASK) == PS_STOPPED)
			nireset(unit);
	}

	/*
	 * Check the queues BEFORE dequeueing an entry
	 */
	while (ds->ds_if.if_snd.ifq_head && ni->freeq1.flink) {

		if((nid = (struct ni_data *)remqhi(&ni->freeq1, NI_MAXITRY))
			== (struct ni_data *)QEMPTY)
			goto done;

		IF_DEQUEUE(&ds->ds_if.if_snd, m0);
		m = m0;

		bde = (struct _bd *) ni->ni_pqb->ni.bdt_base;
		bde += nid->cbufs[0].bdt_index;

		mprev = 0;
		for (len = 0, curindex = 0; m; curindex++, bde++) {
			if ((curindex == (NI_NUMBUF - 2)) &&
			    (m->m_next && m->m_next->m_next)) {
				int off = 0;
				struct mbuf *n, *p = 0;
				MGET(n, M_DONTWAIT, MT_DATA);
				if (n) {
					MCLGET(n, p);
					if (p == 0)
						m_freem(n);
				}
				if (!p) {
					if((empty=insqti(nid, &ni->freeq1, NI_MAXITRY)) > 0)
						printf("insqti failed\n");
					else if(empty == QEMPTY) {
						while((addr->pc&PC_OWN))
							;
						addr->pc =  PC_FREEQNE|PC_DFREEQ|PC_OWN;
					}
					m_freem(m0);
					goto done;
				}
				while (m->m_next) {
					bcopy(mtod(m, caddr_t),
					      mtod(n, caddr_t)+off,
					      (unsigned)m->m_len);
					off += m->m_len;
					m = m_free(m);
				}
				n->m_len = off;
				n->m_next = m;
				m = n;
				if (mprev)
					mprev->m_next = n;
			}
			bde->offset = mtod(m, unsigned)&PGOFSET;
			bde->pt_addr = svtopte(mtod(m, char *));
			bde->buf_len = m->m_len;
			bde->valid = 1;
			nid->cbufs[curindex].offset = 0;
			nid->cbufs[curindex].s_len = bde->buf_len;
			nid->cbufs[curindex].chain = 1;
			len += m->m_len;
			mprev = m;
			m = m->m_next;
		}
		if(len < 64) { /* Last buffer may get something tacked on */
			(--bde)->buf_len += 64 - len;
			nid->cbufs[curindex-1].s_len += 64 - len;
		}
		nid->opcode = SNDDG;
		nid->R = 1;
		nid->status = 0;
		nid->dg_ptdb_index = 1;
		nid->dg_len = 10 + curindex*8;
		nid->cbufs[--curindex].chain = 0;
		nid->mbuf_tofree = (unsigned long)m0;
		ds->nxmit++;
		ds->ds_if.if_flags |= IFF_OACTIVE;
		ds->ds_if.if_opackets++;
		if((empty=insqti(nid, &ni->comq0, NI_MAXITRY)) > 0)
			printf("insqti failed\n");
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				;
			addr->pc =  PC_CMDQNE|PC_CMDQ0|PC_OWN;
		} 
	}
done:
	return;
}

/*
 * Command done interrupt.
 */
bvpniintr(unit)
	register int unit;
{
	register struct ni *ni = &niinfo[unit];
	register struct nidevice *addr = (struct nidevice *)ni->ni_regs;
	register struct ni_softc *ds = &ni_softc[unit];
	int s;

retry:
	if((addr->ps&PS_STATEMASK) != PS_ENABLED) {
		if((addr->ps&PS_STATEMASK) == PS_UNDEFINED ||
		   (addr->ps&PS_STATEMASK) == PS_STOPPED)
			nireset(unit);
		addr->ps &= ~PS_SUME;
		goto done;
	}

	/*
	 * Check for incoming packets.
	 */
		s = splimp();
		smp_lock(&ds->lk_ni_softc, LK_RETRY);
		if(addr->ps&PS_RSQ) {
			nirecv(unit);
		}
	/*
	 * Check for outgoing packets.
	 */
		if (!(ds->ds_if.if_flags & IFF_OACTIVE))
			nistart(unit);
		smp_unlock(&ds->lk_ni_softc);
		splx(s);
done:
	if(addr->ps & PS_SUME) {
		register struct el_rec *elrp;

		if ((addr->ps & PS_FQE) && (addr->pd != 0))
			/*
			 * PS_FQE for other than free queue 0 NOT an
			 * error condition.
			 */
			ni_fqe++;
		else {
			if((elrp = ealloc(sizeof(struct el_bvp), EL_PRILOW))) {
				register struct el_bvp *elbod;
				struct biic_regs *nxv;
				elbod = &elrp->el_body.elbvp;
				nxv = (struct biic_regs *)
				      ((char *)(ni->ni_regs) - NI_NI_ADDR);
				elbod->bvp_biic_typ = nxv->biic_typ;
				elbod->bvp_biic_csr = nxv->biic_ctrl;
				elbod->bvp_pcntl = addr->pc;
				elbod->bvp_pstatus = addr->ps;
				elbod->bvp_perr = addr->pe;
				elbod->bvp_pdata = addr->pd;
				LSUBID(elrp,ELCT_DCNTL,ELBI_BVP,ELBVP_AIE,
				      ni->ni_pqb->ni.piv.bi_node,unit,addr->pe);
				EVALID(elrp);
			}
		}
		if((addr->ps&PS_STATEMASK) == PS_UNDEFINED ||
		   (addr->ps&PS_STATEMASK) == PS_STOPPED)
		goto retry;
	}
		
	addr->ps &= ~(PS_OWN|PS_SUME|PS_RSQ);

}

/*
 * Ethernet interface receiver interface.
 */

nirecv(unit)
	int unit;
{
	register struct ni *ni = &niinfo[unit];
	register struct ni_data *nid;
	register struct nidevice *addr = (struct nidevice *)ni->ni_regs;
	register struct ni_softc *ds = &ni_softc[unit];
	int len, empty;
	struct _bd *bde;

	/* First guess is that its a data gram recieve */
	
	for(;;) {
	if((nid = (struct ni_data *)remqhi(&ni->respq, NI_MAXITRY)) 
		>= (struct ni_data *)QEMPTY) 
		break;
		if(nid->status&PCK_FAIL) {
			register struct el_rec *elrp;

			if((elrp = ealloc(sizeof(struct el_bvp), EL_PRILOW))) {
			register struct el_bvp *elbod;
			struct biic_regs *nxv;
			elbod = &elrp->el_body.elbvp;
			nxv = (struct biic_regs *)
				((char *)(ni->ni_regs) - NI_NI_ADDR);
			elbod->bvp_biic_typ = nxv->biic_typ;
			elbod->bvp_biic_csr = nxv->biic_ctrl;
			elbod->bvp_pcntl = addr->pc;
			elbod->bvp_pstatus = addr->ps;
			elbod->bvp_perr = addr->pe;
			elbod->bvp_pdata = addr->pd;
			LSUBID(elrp,ELCT_DCNTL,ELBI_BVP,ELBVP_AIE,
				ni->ni_pqb->ni.piv.bi_node,unit,nid->status);
			EVALID(elrp);
			}
		}
		switch(nid->opcode) {
		case DGIREC:
		case DGISNT:
			break;
		case DGREC:
			ds->ds_if.if_ipackets++;
			bde = (struct _bd *) ni->ni_pqb->ni.bdt_base;
			bde += nid->cbufs[0].bdt_index;
			if(nid->status&PCK_FAIL)
				ds->ds_if.if_ierrors++;
			len = 0;
			/* Walk buffers & add length */
			{ register int curindex = 0;
			  register struct _bd *pbde = bde;
			while(curindex < NI_RBUF ) {
				len += nid->cbufs[curindex].s_len;
				nid->cbufs[curindex].s_len = pbde++->buf_len;
				if(nid->cbufs[curindex].chain == 0)
					break;
				curindex++;
			}
			}
			niread(ni, ds, bde, len, 0);
			nid->opcode=DGREC;
			nid->dg_len = NI_DGRLEN;
			nid->status=0;
	/* DGREC must end up on freeq2 */
			if((empty=insqti(nid, &ni->freeq2, NI_MAXITRY)) > 0)
				printf("insqti failed\n");
			else if(empty == QEMPTY) {
				while((addr->pc&PC_OWN))
					;
				addr->pc =  PC_FREEQNE|PC_RFREEQ|PC_OWN;
			}
			break;

		case DGSNT:
	/* DGSNT must end up on freeq1 */
			{
			struct mbuf *m = (struct mbuf *)nid->mbuf_tofree;
			if (!(--ds->nxmit))
				ds->ds_if.if_flags &= ~IFF_OACTIVE;
			if(nid->status&PCK_FAIL) {
				ds->ds_if.if_oerrors++;
				m_freem(m);
			}
			else {
				if(((bcmp(mtod(m, struct ether_header *), ni_multi, 6) == 0)) || (ds->ds_if.if_flags & IFF_PFCOPYALL))
					niread(ni, ds, 0, 64, m);
				else
					m_freem(m);
			}
			nid->mbuf_tofree = 0;
			if((empty=insqti(nid, &ni->freeq1, NI_MAXITRY)) > 0)
				printf("insqti failed\n");
			else if(empty == QEMPTY) {
				while((addr->pc&PC_OWN))
					;
				addr->pc =  PC_FREEQNE|PC_DFREEQ|PC_OWN;
			}
			}
			break;
		case MSGSNT:
		case MSGREC:
			{ 
			struct ni_msg *ni_msg;
			ni_msg = (struct ni_msg *)nid;
			switch(ni_msg->ni_opcode) {
				case NIOP_WPARAM:
				case NIOP_RPARAM:
				bcopy(((struct ni_param *)&ni_msg->text[0])->apa, ds->ds_addr, 6);
				bcopy(((struct ni_param *)&ni_msg->text[0])->dpa, ds->ds_dpaddr, 6);
#if NPACKETFILTER > 0
				/* tell packet filter about new address */
				pfilt_newaddress(ds->ds_ed.ess_enetunit, ds->ds_addr);
#endif NPACKETFILTER > 0

					break;
				case NIOP_RCCNTR:
				case NIOP_RDCNTR:
				/* User may be waiting for info to come back */
					wakeup((caddr_t)ni_msg);
					break;
				case NIOP_STPTDB:
				case NIOP_CLPTDB:
				default:
					break;
			}
	/* MSGSNT must end up on freeq0 */
			if((empty=insqti(ni_msg, &ni->freeq0, NI_MAXITRY))>0)
				printf("insqti failed\n");
			else if(empty == QEMPTY) {
				while((addr->pc&PC_OWN))
					;
				addr->pc = PC_FREEQNE|PC_MFREEQ|PC_OWN;
			}
			}
			break;
		default:
			cprintf("ni%d unknown respq opcode\n", unit);
			if((empty=insqti(nid, &ni->freeq0, NI_MAXITRY))>0)
				printf("insqti failed\n");
			else if(empty == QEMPTY) {
				while((addr->pc&PC_OWN))
					;
				addr->pc = PC_FREEQNE|PC_MFREEQ|PC_OWN;
			}
			break;
		}
	}	
}
/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
niread(ni, ds, bde, len, swloop)
	struct ni *ni;
	register struct ni_softc *ds;
	register struct _bd *bde;
	int len;
	struct mbuf *swloop; 
{
    	register struct mbuf *m, *swloop_tmp1;
	struct ether_header *eh, swloop_eh;
	struct protosw *pr;
	int off, resid;
	struct ifqueue *inq;

	/*
	 * Deal with trailer protocol: if type is trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	if (swloop) {
		eh = mtod(swloop, struct ether_header *);
		swloop_eh = *eh;
		eh = &swloop_eh;
		if ( swloop->m_len > sizeof(struct ether_header))
			m_adj(swloop, sizeof(struct ether_header));
		else {
			MFREE(swloop, swloop_tmp1);
			if ( ! swloop_tmp1 )
				return;
			else
				swloop = swloop_tmp1;
		}
	} else  
	    eh = (struct ether_header *)(vaddr(bde));
	eh->ether_type = ntohs((u_short)eh->ether_type);
#define	dataaddr(eh, off, type)	((type)(((caddr_t)(eh)+(off))))
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		if (swloop) {
			struct mbuf *mprev, *m0 = swloop;
/* need to check this against off */
			mprev = m0;
			while (swloop->m_next){/*real header at end of chain*/
				mprev = swloop;
				swloop = swloop->m_next;
			}
			/* move to beginning of chain */
			mprev->m_next = 0;
			swloop->m_next = m0;
			eh->ether_type = ntohs( *mtod(swloop, u_short *));

		} else {
			struct ether_header *peh;
			peh = (struct ether_header *)vaddr(bde+1);
			eh->ether_type = ntohs(*dataaddr(peh, off, u_short *));
			resid = ntohs(*(dataaddr(peh, off+2, u_short *)));
			if (off + resid > len)
			return;		/* sanity */
		}
	} else
		off = 0;
	if (len == 0)
		return;

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; niget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	if (swloop) {
		m = m_copy(swloop, 0, M_COPYALL);
		m_freem(swloop);
	} else {
	/* Header was in the 1st buffer */
		bde++;
		/*
		 * subtract the ethernet header AND checksum field length
		 */
		len -= sizeof(struct ether_header)+4;
		m = niget(ni, bde, len, off);
	}
	if (m == 0)
		return;
	if (off) {
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
	}
	/* Dispatch this packet */
	net_read(&(ds->ds_ed), eh, m, len, (swloop != NULL), (off != 0));
}

/*
 * Process an ioctl request.
 */
niioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ni_softc *ds = &ni_softc[ifp->if_unit];
	register struct ni *ni = &niinfo[ifp->if_unit];
	register struct nidevice *addr = (struct nidevice *)ni->ni_regs;
	struct protosw *pr;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	int s,t,z = splnet(), error = 0, empty;

	switch (cmd) {

        case SIOCENABLBACK:
                ifp->if_flags |= IFF_LOOPBACK;
                break;
 
        case SIOCDISABLBACK:
                ifp->if_flags &= ~IFF_LOOPBACK;
                niinit(ifp->if_unit);
                break;
 
        case SIOCRPHYSADDR: 
                /*
                 * read default hardware address.
                 */
		s = splimp();
		smp_lock(&ds->lk_ni_softc, LK_RETRY);
		bcopy(ds->ds_dpaddr, ifd->default_pa, 6);
		bcopy(ds->ds_addr, ifd->current_pa, 6);
		smp_unlock(&ds->lk_ni_softc);
		splx(s);
                break;
 

	case SIOCSPHYSADDR: 
		/* 
		 * Set physaddr.
		 */
                niinit(ifp->if_unit);
	{
		struct ni_msg *ni_msg;
		if((ni_msg = (struct ni_msg *)remqhi(&ni->freeq0, NI_MAXITRY))
!= (struct ni_msg *)QEMPTY) {
		ni_msg->opcode = SNDMSG;
		ni_msg->status = 0;
		ni_msg->ni_opcode = NIOP_WPARAM;
		ni_msg->msg_len = sizeof (struct ni_param) + 6;
		((struct ni_param *)&ni_msg->text[0])->flags = NI_PAD;
		bcopy(ifr->ifr_addr.sa_data, 
			((struct ni_param *)&ni_msg->text[0])->apa, 6);
		bcopy(ifr->ifr_addr.sa_data, ds->ds_addr, 6);
#if NPACKETFILTER > 0
		/* tell packet filter about new address */
		pfilt_newaddress(ds->ds_ed.ess_enetunit, ds->ds_addr);
#endif NPACKETFILTER > 0

		if((empty=insqti(ni_msg, &ni->comq0, NI_MAXITRY))>0)
			printf("insqti failed\n");
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				;
			addr->pc = PC_CMDQNE|PC_CMDQ0|PC_OWN;
		}
		}
	}

		break;

	case SIOCDELMULTI: 
	case SIOCADDMULTI: 
	{
		int i,j = -1;

		s = splimp();
		smp_lock(&ds->lk_ni_softc, LK_RETRY);

		if (cmd==SIOCDELMULTI) {
		   for (i = 0; i < NMULTI; i++)
		       if (bcmp(ds->ds_multi[i], ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
			    	if (--ds->ds_muse[i] == 0)
					bcopy(ni_multi,ds->ds_multi[i],MULTISIZE);
		       }
		} else {
		    for (i = 0; i < NMULTI; i++) {
			if (bcmp(ds->ds_multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
			    ds->ds_muse[i]++;
			    smp_unlock(&ds->lk_ni_softc);
			    splx(s);
			    goto done;
			}
			if (bcmp(ds->ds_multi[i],ni_multi,MULTISIZE) == 0)
			    j = i;
		    }
		    if (j == -1) {
			printf("ni%d: multi failed, multicast list full: %d\n",
				ni->unit, NMULTI);
			error = ENOBUFS;
			smp_unlock(&ds->lk_ni_softc);
			splx(s);
			goto done;
		    }
		    bcopy(ifr->ifr_addr.sa_data, ds->ds_multi[j], MULTISIZE);
		    ds->ds_muse[j]++;
		}
	/* Update up Protocol Type Definition Block */
		if(ifp->if_flags&IFF_UP)
			ni_sptdb(ni,ETHERTYPE_IP,2,2,PTDB_UNK|PTDB_BDC,1);
		smp_unlock(&ds->lk_ni_softc);
		splx(s);
		break;
	}

	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		{
		register struct ctrreq *ctr = (struct ctrreq *)data;

	/* Read/Read-Clear counters */
		struct ni_msg *ni_msg;
		struct ni_counters *ni_counters;
		int k;

#ifdef vax
		/*
		 * If we're being called on the interrupt stack, we need
		 * to return failure to avoid a sleep panic.
		 */
		if (movpsl() & PSL_IS) {
			error = EINVAL;
			goto done;
		}
#endif vax
#ifdef mips
		/* 
		 * Currently no solution for mips. Need to fix this.
		 */
#endif mips

		ni_msg = (struct ni_msg *)remqhi(&ni->freeq0, NI_MAXITRY);
		if(ni_msg == (struct ni_msg *)QEMPTY) {
			error = ENOBUFS;
			goto done;
		}
		ni_msg->opcode = SNDMSG;
		if(cmd == SIOCRDCTRS)
			ni_msg->ni_opcode = NIOP_RDCNTR;
		else
			ni_msg->ni_opcode = NIOP_RCCNTR;
		ni_msg->msg_len = sizeof(struct ni_counters) + 6;
		while((addr->pc&PC_OWN))
			;
		k=splimp();
		if((empty=insqti(ni_msg, &ni->comq0, NI_MAXITRY))>0)
			printf("insqti failed\n");
		else if(empty == QEMPTY)
			addr->pc = PC_CMDQNE|PC_CMDQ0|PC_OWN;
		/* Wait for read counters to finish */
		sleep((caddr_t)ni_msg, PUSER);
		splx(k);
		ni_counters = (struct ni_counters *)&ni_msg->text[0];
		bzero(&ctr->ctr_ctrs, sizeof(struct estat));
		ctr->ctr_type = CTR_ETHER;
		ctr->ctr_ether.est_seconds = ni_counters->last_zero;
		ctr->ctr_ether.est_bytercvd = *(int *)ni_counters->bytes_rec;
		ctr->ctr_ether.est_bytesent = *(int *)ni_counters->bytes_snt;
		ctr->ctr_ether.est_mbytercvd = *(int *)ni_counters->mbytes_rec;
		ctr->ctr_ether.est_blokrcvd = *(int *)ni_counters->frame_rec;
		ctr->ctr_ether.est_bloksent = *(int *)ni_counters->frame_snt;
		ctr->ctr_ether.est_mblokrcvd = *(int *)ni_counters->mframe_rec;
		ctr->ctr_ether.est_deferred = *(int *)ni_counters->fs_def;
		ctr->ctr_ether.est_single = *(int *)ni_counters->fs_sc;
		ctr->ctr_ether.est_multiple = *(int *)ni_counters->fs_mc;
		ctr->ctr_ether.est_sendfail = ni_counters->sfail;
		ctr->ctr_ether.est_sendfail_bm = ni_counters->sfbm;
		ctr->ctr_ether.est_collis = ni_counters->ctf;
		ctr->ctr_ether.est_recvfail = ni_counters->rfail;
		ctr->ctr_ether.est_recvfail_bm = ni_counters->rfbm;
		ctr->ctr_ether.est_unrecog = ni_counters->unrec;
		ctr->ctr_ether.est_overrun = ni_counters->datao;
		ctr->ctr_ether.est_sysbuf = ni_counters->sbu;
		ctr->ctr_ether.est_userbuf = ni_counters->ubu;
		ctr->ctr_ether.est_mbytesent = *(int *)ni_counters->mbytes_snt;
		ctr->ctr_ether.est_mbloksent = *(int *)ni_counters->mframe_snt;
		}
		break;

	case SIOCSIFADDR:
		{
		ifp->if_flags |= IFF_UP;
		niinit(ifp->if_unit);
		switch(ifa->ifa_addr.sa_family) {
#ifdef INET
		case AF_INET:
			t = splimp();
			smp_lock(&lk_ifnet, LK_RETRY);
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			smp_unlock(&lk_ifnet);
			splx(t);
			/* 1st packet out */
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif

		default:
			if (pr=iffamily_to_proto(ifa->ifa_addr.sa_family)) {
				error = (*pr->pr_ifioctl)(ifp, cmd, data);
			}
			break;
		}
		}
		break;
#ifdef IFF_PROMISC     /* IFF_ALLMULTI and NPACKETFILTER, as well */
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			int newptdb = 0;
			if ((ifp->if_flags & IFF_PROMISC) == IFF_PROMISC)
				newptdb |= PTDB_PROM;
			if ((ifp->if_flags & IFF_ALLMULTI) == IFF_ALLMULTI)
				newptdb |= PTDB_AMC;
			ni_sptdb(ni,ETHERTYPE_IP,2,2,PTDB_UNK|PTDB_BDC|newptdb,1);
		}
		break;
#endif IFF_PROMISC

	default:
		error = EINVAL;
	}
done:	splx(z);
	return (error);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.
 */
struct mbuf *
niget(ni, bde, totlen, trailoff)
	struct ni *ni;
	struct _bd *bde;
	int totlen, trailoff;
{
	struct mbuf *top, **mp, *m;
	int off = trailoff, len = 0;
	/* setup for 1st buffer */
	int cp = (int) vaddr(bde); 
	int bdelen = bde->buf_len;

	top = 0;
	mp = &top;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		if (off) {
			cp = (int) vaddr(bde) + trailoff;
			bdelen = bde->buf_len;
			len = totlen - off;
		} else
			len = totlen;

		if (bdelen >= CLBYTES && len >= CLBYTES) {
			struct mbuf *p;
			struct pte *cpte, *ppte;
			int i;
			char *newdata;

			if (!claligned(cp))
				goto copy;
			KM_ALLOC(newdata, char *, M_CLUSTERSZ, KM_CLUSTER, KM_NOWAIT);
			if (newdata == 0)
				goto nopage;
			MCLPUT(m, cp);
			m->m_len = len;
			bde->pt_addr = svtopte(newdata);
			bde->offset = (unsigned)newdata&PGOFSET;
			bde->buf_len = NI_MAXPACKETSZ;
			goto nocopy;
		}
nopage:
		m->m_len = MIN(MLEN, len);
		m->m_len = MIN(m->m_len, bdelen);
		m->m_off = MMINOFF;
copy:
		bcopy((caddr_t)cp, mtod(m, caddr_t), (unsigned)m->m_len);
		cp += m->m_len;

nocopy:
		bdelen -= m->m_len;

		*mp = m;
		mp = &m->m_next;
		if (off) {
			/* sort of an ALGOL-W style for statement... */
			off += m->m_len;
			if (off == totlen) {
				cp = (int) vaddr(bde);
				bdelen = bde->buf_len;
				off = 0;
				totlen = trailoff;
			}
		} else
			totlen -= m->m_len;
	}
	return (top);
bad:
	m_freem(top);
	return (0);
}

ni_sptdb(ni, type, index, q, flags, doclear)
	struct ni *ni;
	/* Set Protocol Type Definition Block */
{
		register struct ni_softc *ds = &ni_softc[ni->unit];
		struct ni_msg *ni_msg;
		struct ptdb *ptdb;
		int empty,i;
		struct nidevice *addr = (struct nidevice *)ni->ni_regs;
		int clear = doclear;


loop:
		ni_msg = (struct ni_msg *)remqhi(&ni->freeq0, NI_MAXITRY);
		if(ni_msg == (struct ni_msg *)QEMPTY)
			return;
		bzero(&ni_msg->text[0], sizeof(struct ptdb));
		ni_msg->status = 0;
		if(clear)
			ni_msg->ni_opcode = NIOP_CLPTDB;
		else
			ni_msg->ni_opcode = NIOP_STPTDB;
		ni_msg->opcode = SNDMSG;
		ptdb = (struct ptdb *)&ni_msg->text[0];
		ptdb->flags = flags;
		ptdb->fq_index = q;
		ptdb->ptt = type;
		ptdb->ptdb_index = index;
		ptdb->adr_len = 0;
		ni_msg->msg_len = 18;
		if(!clear && flags&(PTDB_AMC|PTDB_BDC)) {
		   int nptdb=0;

		/* 1st reserved for -1 broadcast addr */			
		   ptdb->adr_len++;
		   ni_msg->msg_len += 8;
		   bcopy(ni_multi, (&ptdb->multi[nptdb++])->addr,6);

		   for (i = 0; i < NMULTI - 1; i++)
		        if (ds->ds_muse[i] > 0) {
			ptdb->adr_len++;
			ni_msg->msg_len += 8;
			bcopy(ds->ds_multi[i],(&ptdb->multi[nptdb++])->addr,6);
		       }
		}

		if((empty=insqti(ni_msg, &ni->comq0, NI_MAXITRY)) > 0)
			printf("insqti failed\n");
		else if(empty == QEMPTY) {
			while((addr->pc&PC_OWN))
				;
			addr->pc = PC_CMDQNE|PC_CMDQ0|PC_OWN;
			while((addr->pc&PC_OWN))
				;
		} else 
			while((addr->pc&PC_OWN))
				;
		if(clear) {
			clear = 0;
			goto loop;
		}
}	
#endif
