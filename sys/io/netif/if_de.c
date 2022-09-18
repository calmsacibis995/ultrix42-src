/*
 * if_de.c
 */
#ifndef lint
static char *sccsid = "@(#)if_de.c	4.3	(ULTRIX)	2/26/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-89 by			*
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
 * 24-Feb-91 - jsd
 * 	Allow loopback packets if COPYALL mode is set
 *
 * 10-Nov-89 - jaw
 * 	replace references to maxcpu with smp.
 *
 * 28-Oct-89 - Uttam Shikarpur						*
 *	Added:								*
 *		1) Reporting counters for multicast packs. and bytes	*
 *		2) Added "if_version" support.				*
 *									*
 * 3-Mar-89 - Uttam S.N							*
 *	Add support for Ethernet packet filter				*
 *	Moved common read code to ../net/ether_read.c			*
 *									*
 * 3-Mar-89 - U. Sinkewicz						*
 *	Added smp support.						*
 *									*
 * 29-Jun-88 - Fred L. Templin						*
 *	Added code to the "deoutput" routine to verify that the		*
 *	interface has been marked as "IFF_UP" and "IFF_RUNNING"		*
 *									*
 * 05-Feb-88 - lp							*
 *	Fix deget to switch only 1 cluster as unibus mapping had not	*
 *	changed.							*
 *									*
 * 15-Jan-88 - Larry Palmer						*
 *	Added changes for 43BSD. Also added changes for malloced mbufs.	*
 * 12-Aug-87  -- Fred L. Templin					*
 *	Removed the "de0: buffer unavailable" printf. This has been     *
 *   around since the beginning of time, and this information is        *
 *   already maintained by the deuna counter block. Also, added check   *
 *   in deintr for USCI and SERI error interrupts. No error recovery    *
 *   is attempted; only print diagnostic messages. Fix suggested by	*
 *   Bill Dallas.							*
 *									*
 * 26-Feb-87  -- jsd							*
 *	Check return of iftype_to_proto so that we don't jump to 0.	*
 *									*
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.			*
 *									*
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.		*
 *									*
 *	Chet Juszczak - 03/12/86					*
 *		Modify MCLGET macro call to match new definition	*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 *	10-Sep-85 -- pjt						*
 *		Delua support 						*
 *									*
 * 	19-Jun-85 -- jaw					 	*	
 *		VAX8200 name change.					*
 *									*
 *	13 Mar 85 - Jaw							*
 *		add support for VAX8200 and bua				*
 *									*
 *	LSC001 - Larry Cohen, 3/6/85: add internal loopback 		*
 *									*
 *	LSC002 - Larry cohen, 3/685: poke deuna when transmit ring	*
 *		   is full and more data to send. Prevents system from 	*
 *		   dropping off the network and hanging occasionally	*
 ***********************************************************************/
#include "de.h"
#if NDE > 0 || defined(BINARY)

/*
 * DEC DEUNA interface
 *
 *	Lou Salkind
 *	New York University
 *
 * TODO:
 *	timeout routine (get statistics)
 */

#include "packetfilter.h"	/* NPACKETFILTER */
#include "../data/if_de_data.c"

int dedebug = 0;

u_char unused_multi[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern struct timeval time;
extern int net_output();
int	deprobe(), deattach(), deintr();
struct	uba_device ;
u_short destd[] = { 0 };
struct	uba_driver dedriver =
	{ deprobe, 0, deattach, 0, destd, "de", deinfo };
#define	DEUNIT(x)	minor(x)
int	deinit(),destart(),deioctl(),dereset();
struct	mbuf *deget();

#define DELUA_LOOP_LEN 32-sizeof(struct ether_header)

deprobe(reg)
	caddr_t reg;
{
	register struct dedevice *addr = (struct dedevice *)reg;
	register i;

#ifdef lint
	i = 0; derint(i); deintr(i);
#endif

       /*
        * Make sure self-test is finished 
        * Self-test on a DELUA can take 15 seconds.
        */
        for (i = 0;
             i < 160 &&
             (addr->pcsr0 & PCSR0_FATI) == 0 &&
             (addr->pcsr1 & PCSR1_STMASK) == STAT_RESET;
             ++i)
                DELAY(100000);
        if ((addr->pcsr0 & PCSR0_FATI) != 0 ||
            (addr->pcsr1 & PCSR1_STMASK) != STAT_READY)
                return(0);

        addr->pcsr0 = 0;


	addr->pcsr0 = PCSR0_RSET;

	/*
	 * just in case this is not a deuna or delua 
	 * dont wait for more than 30 secs
         */
        for (i = 0; i < 300 && (addr->pcsr0 & PCSR0_INTR) == 0; i++)
                DELAY(100000);
        if ((addr->pcsr0 & PCSR0_INTR) == 0) 
                return(0);

	/* make board interrupt by executing a GETPCBB command */
	addr->pcsr0 = PCSR0_INTE;
	addr->pcsr2 = 0;
	addr->pcsr3 = 0;
	addr->pcsr0 = PCSR0_INTE|CMD_GETPCBB;
	DELAY(100000);
	return(1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
deattach(ui)
	struct uba_device *ui;
{
	register struct de_softc *ds = &de_softc[ui->ui_unit];
	register struct ifnet *ifp = &ds->ds_if;
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	struct sockaddr_in *sin;
	int csr0,i;
	u_short csr1 = addr->pcsr1;

	/* Initialize SMP lock */
	lockinit(&ds->lk_de_softc, &lock_device15_d);

	/* 
	 * Is it a DEUNA or a DELUA? Save the device id.
	 */
	ds->ds_devid = (csr1 & PCSR1_DEVID) >> 4;

	/* check status of board */
	if (csr1 & 0xff80)
		printf("de%d: hardware error, pcsr1=%x\n", csr1);

	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "de";
	ifp->lk_softc = &ds->lk_de_softc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_type = IFT_ETHER;
	ifp->if_flags |= IFF_BROADCAST|IFF_DYNPROTO;

	/*
	 * Fill the multicast address table with unused entries (broadcast
	 * address) so that we can always give the full table to the device
	 * and we don't have to worry about gaps.
	 */
	for (i=0; i < NMULTI; i++)
		bcopy(unused_multi,&ds->ds_multicast[i],MULTISIZE);

	/*
	 * Reset the board and map the pcbb buffer onto the Unibus.
	 */
	addr->pcsr0 = PCSR0_RSET;
	while ((addr->pcsr0 & PCSR0_INTR) == 0)
		;
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;
	if ((csr0 & PCSR0_PCEI) || (csr0 & PCSR0_DNI)==0) {
		if (ds->ds_devid == DEUNA)
			printf("de%d: reset failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
		else
			printf("de%d: reset failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);

	}
	ds->ds_ubaddr = uballoc(ui->ui_ubanum, INCORE_BASE(ds),
		INCORE_SIZE, 0);
	addr->pcsr2 = ds->ds_ubaddr & 0xffff;
	addr->pcsr3 = (ds->ds_ubaddr >> 16) & 0x3;
	addr->pclow = CMD_GETPCBB;
	while ((addr->pcsr0 & PCSR0_INTR) == 0)
		;
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;
	if ((csr0 & PCSR0_PCEI) || (csr0 & PCSR0_DNI)==0) {
		if (ds->ds_devid == DEUNA) 
			printf("de%d: pcbb failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
		else
			printf("de%d: pcbb failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);
	}

	ds->ds_pcbb.pcbb0 = FC_RDPHYAD;
	addr->pclow = CMD_GETCMD;
	while ((addr->pcsr0 & PCSR0_INTR) == 0)
		;
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;
	if ((csr0 & PCSR0_PCEI) || (csr0 & PCSR0_DNI)==0) {
		if (ds->ds_devid == DEUNA)
			printf("de%d: rdphyad failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
		else
			printf("de%d: rdphyad failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);
	}
	if (dedebug)
		printf("de%d: addr=%d:%d:%d:%d:%d:%d\n", ui->ui_unit,
		    ds->ds_pcbb.pcbb2&0xff, (ds->ds_pcbb.pcbb2>>8)&0xff,
		    ds->ds_pcbb.pcbb4&0xff, (ds->ds_pcbb.pcbb4>>8)&0xff,
		    ds->ds_pcbb.pcbb6&0xff, (ds->ds_pcbb.pcbb6>>8)&0xff);
	bcopy((caddr_t)&ds->ds_pcbb.pcbb2,(caddr_t)ds->ds_addr,
	    sizeof (ds->ds_addr));
	ifp->if_init = deinit;
	ifp->if_output = net_output;
	ifp->if_start = destart;
	ifp->if_ioctl = deioctl;
	ifp->if_reset = dereset;
	ifp->d_affinity = ALLCPU;	/* 10.18.88.us */
	ds->ds_deuba.ifu_flags = UBA_CANTWAIT;
#ifdef notdef
	/* CAN WE USE BDP's ??? */
	ds->ds_deuba.ifu_flags |= UBA_NEEDBDP;
#endif
	if (ds->ds_devid == DEUNA) {
		ifp->if_sysid_type = 1;
		bcopy("DEC DEUNA Ethernet Interface", ifp->if_version, 28);
	} else {
		ifp->if_sysid_type = 11;
		bcopy("DEC DELUA Ethernet Interface", ifp->if_version, 28);
	}
	ifp->if_version[28] = '\0';

	printf ("de%d: %s, hardware address %s\n", ifp->if_unit,
		 ifp->if_version, ether_sprintf(ds->ds_addr));
#if NPACKETFILTER > 0
	/* Tell the packet filter we are here */
	attachpfilter(&(ds->ds_ed));
#endif NPACKETFILTER
	if_attach(ifp);
}

/*
 * Reset of interface after UNIBUS reset.
 * If interface is on specified uba, reset its state.
 */
dereset(unit, uban)
	int unit, uban;
{
	register struct uba_device *ui;
	register struct de_softc *ds = &de_softc[unit];

	if (unit >= nNDE || (ui = deinfo[unit]) == 0 || ui->ui_alive == 0 ||
	    ui->ui_ubanum != uban)
		return;
	printf(" de%d", unit);
	ds->ds_ubaddr = uballoc(uban, INCORE_BASE(ds), INCORE_SIZE, 0);
	deinit(unit);
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize UNIBUS usage.
 */
deinit(unit)
	int unit;
{
	register struct de_softc *ds = &de_softc[unit];
	register struct uba_device *ui = deinfo[unit];
	register struct dedevice *addr;
	register struct ifrw *ifrw;
	register struct ifxmt *ifxp;
	struct ifnet *ifp = &ds->ds_if;
	int s;
	struct de_ring *rp;
	int incaddr;
	int csr0;

	/* not yet, if address still unknown */
	/* DECnet must set this somewhere to make device happy */
	if (ifp->if_addrlist == (struct ifaddr *)0)
			return;
	if (ifp->if_flags & IFF_RUNNING)
		return;

	if (de_ubainit(&ds->ds_deuba, ui->ui_ubanum,
	    sizeof (struct ether_header), (int)btoc(ETHERMTU)) == 0) { 
		printf("de%d: can't initialize\n", unit);
		ds->ds_if.if_flags &= ~IFF_UP;
		return;
	}
	addr = (struct dedevice *)ui->ui_addr;

	/* set the transmit and receive ring header addresses */
	incaddr = ds->ds_ubaddr + UDBBUF_OFFSET;
	ds->ds_pcbb.pcbb0 = FC_WTRING;
	ds->ds_pcbb.pcbb2 = incaddr & 0xffff;
	ds->ds_pcbb.pcbb4 = (incaddr >> 16) & 0x3;

	incaddr = ds->ds_ubaddr + XRENT_OFFSET;
	ds->ds_udbbuf.b_tdrbl = incaddr & 0xffff;
	ds->ds_udbbuf.b_tdrbh = (incaddr >> 16) & 0x3;
	ds->ds_udbbuf.b_telen = sizeof (struct de_ring) / sizeof (short);
	ds->ds_udbbuf.b_trlen = NXMT;
	incaddr = ds->ds_ubaddr + RRENT_OFFSET;
	ds->ds_udbbuf.b_rdrbl = incaddr & 0xffff;
	ds->ds_udbbuf.b_rdrbh = (incaddr >> 16) & 0x3;
	ds->ds_udbbuf.b_relen = sizeof (struct de_ring) / sizeof (short);
	ds->ds_udbbuf.b_rrlen = NRCV;

	addr->pclow = CMD_GETCMD;
	while ((addr->pcsr0 & PCSR0_INTR) == 0)
		;
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;
	if (csr0 & PCSR0_PCEI)
		if (ds->ds_devid == DEUNA)
			printf("de%d: wtring failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
		else
			printf("de%d: wtring failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);

	/* initialize the mode - enable hardware padding */
	ds->ds_pcbb.pcbb0 = FC_WTMODE;
	/* let hardware do padding - set MTCH bit on broadcast */
	ds->ds_pcbb.pcbb2 = MOD_TPAD|MOD_HDX;
#ifdef IFF_PROMISC
	if (ifp->if_flags & IFF_PROMISC)        /* set promiscuous if wanted */
		ds->ds_pcbb.pcbb2 |= MOD_PROM;
#endif  IFF_PROMISC
	addr->pclow = CMD_GETCMD;
	while ((addr->pcsr0 & PCSR0_INTR) == 0)
		;
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;
	if (csr0 & PCSR0_PCEI)
		if (ds->ds_devid == DEUNA)
			printf("de%d: wtmode failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
		else
			printf("de%d: wtmode failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);

	/* set up the receive and transmit ring entries */
	ifxp = &ds->ds_deuba.ifu_w[0];
	for (rp = &ds->ds_xrent[0]; rp < &ds->ds_xrent[NXMT]; rp++) {
		rp->r_segbl = ifxp->x_ifrw.ifrw_info & 0xffff;
		rp->r_segbh = (ifxp->x_ifrw.ifrw_info >> 16) & 0x3;
		rp->r_flags = 0;
		ifxp++;
	}
	ifrw = &ds->ds_deuba.ifu_r[0];
	for (rp = &ds->ds_rrent[0]; rp < &ds->ds_rrent[NRCV]; rp++) {
		rp->r_slen = sizeof (struct de_buf);
		rp->r_segbl = ifrw->ifrw_info & 0xffff;
		rp->r_segbh = (ifrw->ifrw_info >> 16) & 0x3;
		rp->r_flags = RFLG_OWN;		/* hang receive */
		ifrw++;
	}

	/* start up the board (rah rah) */
	s = splimp();
	ds->ds_rindex = ds->ds_xindex = ds->ds_xfree = ds->ds_nxmit = 0;
	ds->ds_if.if_flags |= IFF_UP|IFF_RUNNING;
	ds->ds_if.if_flags &= ~IFF_OACTIVE;
	addr->pclow = PCSR0_INTE;		/* avoid interlock */
	smp_lock(&ds->lk_de_softc, LK_RETRY);
	destart(unit);				/* queue output packets */
	smp_unlock(&ds->lk_de_softc);
	addr->pclow = CMD_START | PCSR0_INTE;
	ds->ds_flags |= DSF_RUNNING;
	ds->ds_ztime = time.tv_sec;
	splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 */
destart(unit)
	int unit;
{
        int len;
	struct uba_device *ui = deinfo[unit];
	struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	register struct de_softc *ds = &de_softc[unit];
	register struct de_ring *rp;
	struct mbuf *m;
	register int nxmit;

	for (nxmit = ds->ds_nxmit; nxmit < NXMT; nxmit++) {
		IF_DEQUEUE(&ds->ds_if.if_snd, m);
		if (m == 0)
			break;
		rp = &ds->ds_xrent[ds->ds_xfree];
		if (rp->r_flags & XFLG_OWN)
			panic("deuna xmit in progress");
		len = deput(&ds->ds_deuba, ds->ds_xfree, m);
		if (ds->ds_deuba.ifu_flags & UBA_NEEDBDP)
			UBAPURGE(ds->ds_deuba.ifu_uba,
			ds->ds_deuba.ifu_w[ds->ds_xfree].x_ifrw.ifrw_bdp,
			ds->ds_deuba.ifu_uban);
		rp->r_slen = len;
		rp->r_tdrerr = 0;
		rp->r_flags = XFLG_STP|XFLG_ENP|XFLG_OWN;

		ds->ds_xfree++;
		if (ds->ds_xfree == NXMT)
			ds->ds_xfree = 0;
	}
	if (ds->ds_nxmit != nxmit) {
		ds->ds_nxmit = nxmit;
		ds->ds_if.if_flags |= IFF_OACTIVE;
		if (ds->ds_flags & DSF_RUNNING)
			addr->pclow = PCSR0_INTE|CMD_PDMD;
	}
	else if (ds->ds_nxmit == NXMT) {  /* LSC002 */
		/*
		 * poke device if we have something to send and 
		 * transmit ring is full. 
		 */
		if (dedebug) { 
			rp = &ds->ds_xrent[0];
			mprintf("did not xmt: %d, %d, %d, flag0=%x, flag1=%x\n", ds->ds_xindex, ds->ds_nxmit, ds->ds_xfree, rp++->r_flags, rp->r_flags);
		}
		if (ds->ds_flags & DSF_RUNNING)
			addr->pclow = PCSR0_INTE|CMD_PDMD;
	}
		
}

/*
 * Command done interrupt.
 */
deintr(unit)
	int unit;
{
	struct uba_device *ui = deinfo[unit];
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	register struct de_softc *ds = &de_softc[unit];
	register struct de_ring *rp;
	register struct ifxmt *ifxp;
	short csr0;
	int s;

	s = splimp(); /* SMP */
	smp_lock(&ds->lk_de_softc, LK_RETRY);
	/* save flags right away - clear out interrupt bits */
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;

	/*
	 *   Error condition interrupts. No recovery attempted - just
	 * printf and leave the interface alone.
	 */
	if (csr0 & PCSR0_FATI)
		printf ("de%d: unsolicited state change, csr0=%b, csr1=0x%04x\n",
			 unit, csr0, PCSR0_BITS, addr->pcsr1);
	if (csr0 & PCSR0_SERI)
		mprintf ("de%d: status error, csr0=%b\n",
		         unit, csr0, PCSR0_BITS);

	/*
	 * if receive, put receive buffer on mbuf
	 * and hang the request again
	 */
	if ((ds->ds_rrent[ds->ds_rindex].r_flags & RFLG_OWN) == 0)
		derecv(unit);

	/*
	 * Poll transmit ring and check status.
	 * Be careful about loopback requests.
	 * Then free buffer space and check for
	 * more transmit requests.
	 */
	for ( ; ds->ds_nxmit > 0; ds->ds_nxmit--) {
		rp = &ds->ds_xrent[ds->ds_xindex];
		if (rp->r_flags & XFLG_OWN)
			break;
		ds->ds_if.if_opackets++;
		ifxp = &ds->ds_deuba.ifu_w[ds->ds_xindex];
		/* check for unusual conditions */
		if (rp->r_flags & (XFLG_ERRS|XFLG_MTCH|XFLG_ONE|XFLG_MORE)) {
			if (rp->r_flags & XFLG_ERRS) {
				/* output error */
				ds->ds_if.if_oerrors++;
				if (dedebug)
			printf("de%d: oerror, flags=%b tdrerr=%b (len=%d)\n",
				    unit, rp->r_flags, XFLG_BITS,
				    rp->r_tdrerr, XERR_BITS, rp->r_slen);
			} else { /* no oerrors */
				if (rp->r_flags & XFLG_ONE) {
				    /* one collision */
				    ds->ds_if.if_collisions++;
				    } else if (rp->r_flags & XFLG_MORE) {
				            /* more than one collision */
				            ds->ds_if.if_collisions += 2;	/* guess */
				    }
/*001*/			    if ( ((rp->r_flags & XFLG_MTCH) && !(ds->ds_if.if_flags & IFF_LOOPBACK)) || (ds->ds_if.if_flags & IFF_PFCOPYALL) ) {
				/* received our own packet */
				ds->ds_if.if_ipackets++;
				if (dedebug)
					mprintf("de%d: loop\n", unit);
				deread(ds, &ifxp->x_ifrw,
				    rp->r_slen - sizeof (struct ether_header),
					ifxp->x_xtofree);
				ifxp->x_xtofree = 0;
				}
			} /* end else no oerrors */

		}
		if (ifxp->x_xtofree) {
			m_freem(ifxp->x_xtofree);
			ifxp->x_xtofree = 0;
		}
		/* check if next transmit buffer also finished */
		ds->ds_xindex++;
		if (ds->ds_xindex == NXMT)
			ds->ds_xindex = 0;
	}
	if (!(ds->ds_nxmit)) {
		ds->ds_if.if_flags &= ~IFF_OACTIVE;
		destart(unit);
	}

	/*
	 * This may have happened for EITHER a user or system buffer
	 * unavailable. In both cases, need to turn on interrupts and
	 * tell the interface that receive buffers have been freed up.
	 * The "de0: buffer unavailable" printf has been removed since
	 * the counter block already records system and user buffer unavail.
	 */
	if (csr0 & PCSR0_RCBI)
		addr->pclow = PCSR0_INTE|CMD_PDMD;
	smp_unlock(&ds->lk_de_softc);
	splx(s);

}

/*
 * Ethernet interface receiver interface.
 * If input error just drop packet.
 * Otherwise purge input buffered data path and examine 
 * packet to determine type.  If can't determine length
 * from type, then have to drop packet.  Othewise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */
derecv(unit)
	int unit;
{
	register struct de_softc *ds = &de_softc[unit];
	register struct ifnet *ifp = &ds->ds_if;
	register struct de_ring *rp;
	register struct ether_header *eh;
	int len;

	rp = &ds->ds_rrent[ds->ds_rindex];
	while ((rp->r_flags & RFLG_OWN) == 0) {
		ds->ds_if.if_ipackets++;
		if (ds->ds_deuba.ifu_flags & UBA_NEEDBDP)
			UBAPURGE(ds->ds_deuba.ifu_uba,
			ds->ds_deuba.ifu_r[ds->ds_rindex].ifrw_bdp,
			ds->ds_deuba.ifu_uban);
		len = (rp->r_lenerr&RERR_MLEN) - sizeof (struct ether_header)
			- 4;	/* don't forget checksum! */
                if( ! (ifp->if_flags & IFF_LOOPBACK) ) {
		/* 
		 * check for errors 
		 *
		 * Added Bit test RERR_OVRN <bit 12>, message overrun error
		 * This bit is always zero for the DEUNA so no additional test
		 * are needed.
		 */
		    if ((rp->r_flags & (RFLG_ERRS|RFLG_FRAM|RFLG_OFLO|RFLG_CRC))
		        || (rp->r_flags&(RFLG_STP|RFLG_ENP)) != (RFLG_STP|RFLG_ENP) 
		        || (rp->r_lenerr & (RERR_OVRN|RERR_BUFL|RERR_UBTO|RERR_NCHN)) ||
		        len < ETHERMIN || len > ETHERMTU) {
		  	    ds->ds_if.if_ierrors++;
			    if (dedebug)
			     printf("de%d: ierror, flags=%b lenerr=%b (len=%d)\n",
				unit, rp->r_flags, RFLG_BITS, rp->r_lenerr,
				RERR_BITS, len);
		    } else{
			deread(ds, &ds->ds_deuba.ifu_r[ds->ds_rindex], len, 0);
			}
                } else {
                        eh = (struct ether_header *)ds->ds_deuba.ifu_r[ds->ds_rindex].ifrw_addr;
                        if ( bcmp(eh->ether_dhost, ds->ds_addr, 6) == NULL ){
                                deread(ds, &ds->ds_deuba.ifu_r[ds->ds_rindex], len, 0);
			}
                }

		/* hang the receive buffer again */
		rp->r_lenerr = 0;
		rp->r_flags = RFLG_OWN;

		/* check next receive buffer */
		ds->ds_rindex++;
		if (ds->ds_rindex == NRCV)
			ds->ds_rindex = 0;
		rp = &ds->ds_rrent[ds->ds_rindex];
	}
}

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
deread(ds, ifrw, len, swloop)
	register struct de_softc *ds;
	struct ifrw *ifrw;
	int len;
	struct mbuf *swloop;
{
	struct ether_header *eh, swloop_eh;
    	struct mbuf *m, *swloop_tmp1, *swloop_tmp2;
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
		eh = (struct ether_header *)ifrw->ifrw_addr;


	eh->ether_type = ntohs((u_short)eh->ether_type);
#define	dedataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
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

		}
		else {
			eh->ether_type = ntohs(*dedataaddr(eh, off, u_short *));
			resid = ntohs(*(dedataaddr(eh, off+2, u_short *)));
			if (off + resid > len)
				return;	/* sanity */
			len = off + resid;
		}
	} else
		off = 0;
	if (len == 0)
		return;

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; deget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	if (swloop) {
		m = m_copy(swloop, 0, M_COPYALL);
		m_freem(swloop);
	} else {
		m = deget(&ds->ds_deuba, ifrw, len, off);
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
 * Routines supporting UNIBUS network interfaces.
 */

/*
 * Init UNIBUS for interface on uban whose headers of size hlen are to
 * end on a page boundary.  We allocate a UNIBUS map register for the page
 * with the header, and nmr more UNIBUS map registers for i/o on the adapter,
 * doing this for each receive and transmit buffer.  We also
 * allocate page frames in the mbuffer pool for these pages.
 */
de_ubainit(ifu, uban, hlen, nmr)
	register struct deuba *ifu;
	int uban, hlen, nmr;
{
	register caddr_t cp, dp;
	register struct ifrw *ifrw;
	register struct ifxmt *ifxp;
	int i, ncl;

	ncl = clrnd(nmr + CLSIZE) / CLSIZE;
	if (ifu->ifu_r[0].ifrw_addr)
		/*
		 * If the first read buffer has a non-zero
		 * address, it means we have already allocated core
		 */
		cp = ifu->ifu_r[0].ifrw_addr - (CLBYTES - hlen);
	else {
		cp = m_clalloc(NTOT * ncl, MPG_SPACE);
		if (cp == 0)
			return (0);
		ifu->ifu_hlen = hlen;
		ifu->ifu_uban = uban;
		ifu->ifu_uba = uba_hd[uban].uh_uba;
		dp = cp + CLBYTES - hlen;
		for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[NRCV]; ifrw++) {
			ifrw->ifrw_addr = dp;
			dp += ncl * CLBYTES;
		}
		for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[NXMT]; ifxp++) {
			ifxp->x_ifrw.ifrw_addr = dp;
			dp += ncl * CLBYTES;
		}
	}
	/* allocate for receive ring */
	for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[NRCV]; ifrw++) {
		if (de_ubaalloc(ifu, ifrw, nmr) == 0) {
			struct ifrw *rw;

			for (rw = ifu->ifu_r; rw < ifrw; rw++)
				ubarelse(ifu->ifu_uban, &rw->ifrw_info);
			goto bad;
		}
	}
	/* and now transmit ring */
	for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[NXMT]; ifxp++) {
		ifrw = &ifxp->x_ifrw;
		if (de_ubaalloc(ifu, ifrw, nmr) == 0) {
			struct ifxmt *xp;

			for (xp = ifu->ifu_w; xp < ifxp; xp++)
				ubarelse(ifu->ifu_uban, &xp->x_ifrw.ifrw_info);
			for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[NRCV]; ifrw++)
				ubarelse(ifu->ifu_uban, &ifrw->ifrw_info);
			goto bad;
		}
		for (i = 0; i < nmr; i++)
			ifxp->x_map[i] = ifrw->ifrw_mr[i];
		ifxp->x_xswapd = 0;
	}
	return (1);
bad:
	m_pgfree(cp, NTOT * ncl);
	ifu->ifu_r[0].ifrw_addr = 0;
	return(0);
}

/*
 * Setup either a ifrw structure by allocating UNIBUS map registers,
 * possibly a buffered data path, and initializing the fields of
 * the ifrw structure to minimize run-time overhead.
 */
static
de_ubaalloc(ifu, ifrw, nmr)
	struct deuba *ifu;
	register struct ifrw *ifrw;
	int nmr;
{
	register int info;

	info =
	    uballoc(ifu->ifu_uban, ifrw->ifrw_addr, nmr*NBPG + ifu->ifu_hlen,
	        ifu->ifu_flags);
	if (info == 0)
		return (0);
	ifrw->ifrw_info = info;
	ifrw->ifrw_bdp = UBAI_BDP(info);
	ifrw->ifrw_proto = UBAMR_MRV | (UBAI_BDP(info) << UBAMR_DPSHIFT);
	ifrw->ifrw_mr = &ifu->ifu_uba->uba_map[UBAI_MR(info) + 1];
	return (1);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.  When full cluster sized units are present
 * on the interface on cluster boundaries we can get them more
 * easily by remapping, and take advantage of this here.
 */
struct mbuf *
deget(ifu, ifrw, totlen, off0)
	register struct deuba *ifu;
	register struct ifrw *ifrw;
	int totlen, off0;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = ifrw->ifrw_addr + ifu->ifu_hlen;

	top = 0;
	mp = &top;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		if (off) {
			len = totlen - off;
			cp = ifrw->ifrw_addr + ifu->ifu_hlen + off;
		} else
			len = totlen;
		if (len == CLBYTES) { /* For now only 1 cluster */
				      /* May ultimately fix mapping */
			struct mbuf *p;
			struct pte *cpte, *ppte;
			int x, *ip, i;

			MCLGET(m, p);
			if (p == 0)
				goto nopage;
			len = m->m_len = CLBYTES;
			if (!claligned(cp))
				goto copy;

			/* faster just to copy in SMP case since 
			   we would have to sync up TB's */
			if (smp)
				goto copy;

			/*
			 * Switch pages mapped to UNIBUS with new page p,
			 * as quick form of copy.  Remap UNIBUS and invalidate.
			 */
			cpte = &kmempt[mtocl(cp)];
			ppte = &kmempt[mtocl(p)];
			x = btop(cp - ifrw->ifrw_addr);
			ip = (int *)&ifrw->ifrw_mr[x];
			for (i = 0; i < CLSIZE; i++) {
				struct pte t;
				t = *ppte; *ppte++ = *cpte; *cpte = t;
				*ip++ =
				    cpte++->pg_pfnum|ifrw->ifrw_proto;
				mtpr(TBIS, cp);
				cp += NBPG;
				mtpr(TBIS, (caddr_t)p);
				p += NBPG / sizeof (*p);
			}
			goto nocopy;
		}
nopage:
		m->m_len = MIN(MLEN, len);
		m->m_off = MMINOFF;
copy:
		bcopy(cp, mtod(m, caddr_t), (unsigned)m->m_len);
		cp += m->m_len;
nocopy:
		*mp = m;
		mp = &m->m_next;
		if (off) {
			/* sort of an ALGOL-W style for statement... */
			off += m->m_len;
			if (off == totlen) {
				cp = ifrw->ifrw_addr + ifu->ifu_hlen;
				off = 0;
				totlen = off0;
			}
		} else
			totlen -= m->m_len;
	}
	return (top);
bad:
	m_freem(top);
	return (0);
}

/*
 * Map a chain of mbufs onto a network interface
 * in preparation for an i/o operation.
 * The argument chain of mbufs includes the local network
 * header which is copied to be in the mapped, aligned
 * i/o space.
 */
deput(ifu, n, m)
	struct deuba *ifu;
	int n;
	register struct mbuf *m;
{
	register caddr_t cp;
	register struct ifxmt *ifxp;
	register struct ifrw *ifrw;
	register int i;
	int xswapd = 0;
	int x, cc, t;
	caddr_t dp;

	ifxp = &ifu->ifu_w[n];
	ifrw = &ifxp->x_ifrw;
	ifxp->x_xtofree = m;
	cp = ifrw->ifrw_addr;
	while (m) {
		dp = mtod(m, char *);
		if (claligned(cp) && claligned(dp) && m->m_len == CLBYTES) {
			struct pte *pte; int *ip;
			pte = &kmempt[mtocl(dp)];
			x = btop(cp - ifrw->ifrw_addr);
			ip = (int *)&ifrw->ifrw_mr[x];
			for (i = 0; i < CLSIZE; i++)
				*ip++ =
				    ifrw->ifrw_proto | pte++->pg_pfnum;
			xswapd |= 1 << (x>>(CLSHIFT-PGSHIFT));
			cp += m->m_len;
		} else {
			bcopy(mtod(m, caddr_t), cp, (unsigned)m->m_len);
			cp += m->m_len;
		}
		m = m->m_next;
	}

	/*
	 * Xswapd is the set of clusters we just mapped out.  Ifxp->x_xswapd
	 * is the set of clusters mapped out from before.  We compute
	 * the number of clusters involved in this operation in x.
	 * Clusters mapped out before and involved in this operation
	 * should be unmapped so original pages will be accessed by the device.
	 */
	cc = cp - ifrw->ifrw_addr;
	x = ((cc - ifu->ifu_hlen) + CLBYTES - 1) >> CLSHIFT;
	ifxp->x_xswapd &= ~xswapd;
#ifdef notdef
	xswapd &= ~ifxp->x_xswapd;  /* this was in if_uba.c */
#endif
	while (i = ffs(ifxp->x_xswapd)) {
		i--;
		if (i >= x)
			break;
		ifxp->x_xswapd &= ~(1<<i);
		i *= CLSIZE;
		for (t = 0; t < CLSIZE; t++) {
			ifrw->ifrw_mr[i] = ifxp->x_map[i];
			i++;
		}
	}
	ifxp->x_xswapd |= xswapd;
	return (cc);
}

/*
 * Process an ioctl request.
 */
deioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct de_softc *ds = &de_softc[ifp->if_unit];
	register struct uba_device *ui = deinfo[ifp->if_unit];
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	struct protosw *pr;
	struct sockaddr *sa;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	int s = splimp(), error = 0;
	int csr0;

	switch (cmd) {

/*LSC001*/
        case SIOCENABLBACK:
                printf("de%d: internal loopback enable requested\n", ifp->if_unit);
                if ( (error = deloopback( ui, ifp, ds, addr, 1 )) != NULL )
                        break;
                ifp->if_flags |= IFF_LOOPBACK;
#ifdef notdef
                if (ifp->if_flags & IFF_RUNNING)
                        if_rtinit(ifp, -1);     
#endif
                break;
 
        case SIOCDISABLBACK:
                printf("de%d: internal loopback disable requested\n", ifp->if_unit);
                if ( (error = deloopback( ui, ifp, ds, addr, 0 )) != NULL )
                        break;
                ifp->if_flags &= ~IFF_LOOPBACK;
#ifdef notdef
                if (ifp->if_flags & IFF_RUNNING)
                        if_rtinit(ifp, -1);     
#endif
                deinit(ifp->if_unit);
                break;
 
        case SIOCRPHYSADDR: 
                /*
                 * read default hardware address.
                 */
		smp_lock(&ds->lk_de_softc, LK_RETRY);
                ds->ds_pcbb.pcbb0 = FC_RDDEFAULT;
                addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
                while ((addr->pcsr0 & PCSR0_DNI) == 0) 
                        ;
                csr0 = addr->pcsr0;
                addr->pchigh = csr0 >> 8;
                if (csr0 & PCSR0_PCEI) {
			if (ds->ds_devid == DEUNA)
                        	printf("de%d: read default hardware address failed, csr0=%b csr1=%b\n", 
                        	ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
			else
                        	printf("de%d: read default hardware address failed, csr0=%b csr1=%b\n", 
                        	ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);
                        error = EIO;
                        break;
                }
                /*
                 * copy current physical address and default hardware address
                 * for requestor.
                 */
                bcopy(&ds->ds_pcbb.pcbb2, ifd->default_pa, 6);
                bcopy(ds->ds_addr, ifd->current_pa, 6);
		smp_unlock(&ds->lk_de_softc);		
                break;
 

	case SIOCSPHYSADDR: 
		smp_lock(&ds->lk_de_softc, LK_RETRY);
		/* Set the DNA address as the de's physical address */
		ds->ds_pcbb.pcbb0 = FC_WTPHYAD;
		bcopy (ifr->ifr_addr.sa_data, &ds->ds_pcbb.pcbb2, 6);
		addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
		while ((addr->pcsr0 & PCSR0_DNI) == 0)
			;
		csr0 = addr->pcsr0;
		addr->pchigh = csr0 >> 8;
		if (csr0 & PCSR0_PCEI)
			if (ds->ds_devid == DEUNA)
		  		printf("de%d: wtphyad failed, csr0=%b csr1=%b\n", 
		    			ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
			else
		 		printf("de%d: wtphyad failed, csr0=%b csr1=%b\n", 
		    			ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);

		bcopy((caddr_t)&ds->ds_pcbb.pcbb2,(caddr_t)ds->ds_addr,
	    		sizeof (ds->ds_addr));
#if NPACKETFILTER > 0
		pfilt_newaddress(ds->ds_ed.ess_enetunit, ds->ds_addr);
#endif NPACKETFILTER > 0
#ifdef notdef
		if (ifp->if_flags & IFF_RUNNING)
			if_rtinit(ifp, -1);	
#endif
		/*
		 * Need to unlock before calling deinit()
		 */
		smp_unlock(&ds->lk_de_softc);		
		deinit(ifp->if_unit);
		break;

	case SIOCDELMULTI: 
	case SIOCADDMULTI: 
		smp_lock(&ds->lk_de_softc, LK_RETRY);
		{
		int i,j = -1,incaddr = ds->ds_ubaddr + MULTI_OFFSET;

		if (cmd==SIOCDELMULTI) {
		   for (i = 0; i < NMULTI; i++)
		       if (bcmp(&ds->ds_multicast[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
			    if (--ds->ds_muse[i] == 0)
				bcopy(unused_multi,&ds->ds_multicast[i],MULTISIZE);
		       }
		} else {
		    for (i = 0; i < NMULTI; i++) {
			if (bcmp(&ds->ds_multicast[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
			    ds->ds_muse[i]++;
			    smp_unlock(&ds->lk_de_softc);		
			    goto done;
			}
		        if (bcmp(&ds->ds_multicast[i],unused_multi,MULTISIZE) == 0)
			    j = i;
		    }
		    if (j == -1) {
			printf("de%d: mtmulti failed, multicast list full: %d\n",
				ui->ui_unit, NMULTI);
			error = ENOBUFS;
			smp_unlock(&ds->lk_de_softc);		
			goto done;
		    }
		    bcopy(ifr->ifr_addr.sa_data, &ds->ds_multicast[j], MULTISIZE);
		    ds->ds_muse[j]++;
		}
		ds->ds_pcbb.pcbb0 = FC_WTMULTI;
		ds->ds_pcbb.pcbb2 = incaddr & 0xffff;
		ds->ds_pcbb.pcbb4 = (NMULTI << 8) | ((incaddr >> 16) & 03);
		addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
		while ((addr->pcsr0 & PCSR0_DNI) == 0)
		;
		csr0 = addr->pcsr0;
		addr->pchigh = csr0 >> 8;
		if (csr0 & PCSR0_PCEI)
			if (ds->ds_devid == DEUNA)
		  		printf("de%d: wtmulti failed, csr0=%b csr1=%b\n", ui->ui_unit,
		    		csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
			else
		  		printf("de%d: wtmulti failed, csr0=%b csr1=%b\n", ui->ui_unit,
		 	   	csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);
		smp_unlock(&ds->lk_de_softc);		
		break;

		}

	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		smp_lock(&ds->lk_de_softc, LK_RETRY);
		{
		int incaddr = ds->ds_ubaddr + COUNTER_OFFSET;
		register struct ctrreq *ctr = (struct ctrreq *)data;

		ds->ds_pcbb.pcbb0 = cmd == SIOCRDCTRS ? FC_RDCNTS : FC_RCCNTS;
		ds->ds_pcbb.pcbb2 = incaddr & 0xffff;
		ds->ds_pcbb.pcbb4 = (incaddr >> 16) & 03;
		ds->ds_pcbb.pcbb6 = sizeof(struct de_counters);
		addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
		while ((addr->pcsr0 & PCSR0_DNI) == 0)
			;
		csr0 = addr->pcsr0;
		addr->pchigh = csr0 >> 8;
		if (csr0 & PCSR0_PCEI) {
			if (ds->ds_devid == DEUNA)
				printf("de%d: rdcnts failed, csr0=%b csr1=%b\n",
				  ui->ui_unit, csr0, PCSR0_BITS,
				  addr->pcsr1, PCSR1_BITS);
			else
				printf("de%d: rdcnts failed, csr0=%b csr1=%b\n",
				  ui->ui_unit, csr0, PCSR0_BITS,
				  addr->pcsr1, PCSR1_BITS_DELUA);
			error = ENOBUFS;
			break;
		}
		bzero(&ctr->ctr_ctrs, sizeof(struct estat));
		ctr->ctr_type = CTR_ETHER;
		ctr->ctr_ether.est_seconds = (time.tv_sec - ds->ds_ztime) > 0xfffe ? 0xffff : (time.tv_sec - ds->ds_ztime);
		ctr->ctr_ether.est_bytercvd = *(int *)ds->ds_counters.c_brcvd;
		ctr->ctr_ether.est_bytesent = *(int *)ds->ds_counters.c_bsent;
		ctr->ctr_ether.est_mbytercvd = *(int *)ds->ds_counters.c_mbrcvd;
		ctr->ctr_ether.est_blokrcvd = *(int *)ds->ds_counters.c_prcvd;
		ctr->ctr_ether.est_bloksent = *(int *)ds->ds_counters.c_psent;
		ctr->ctr_ether.est_mblokrcvd = *(int *)ds->ds_counters.c_mprcvd;
		ctr->ctr_ether.est_deferred = *(int *)ds->ds_counters.c_defer;
		ctr->ctr_ether.est_single = *(int *)ds->ds_counters.c_single;
		ctr->ctr_ether.est_multiple = *(int *)ds->ds_counters.c_multiple;
		ctr->ctr_ether.est_sendfail = ds->ds_counters.c_snderr;
		ctr->ctr_ether.est_sendfail_bm = ds->ds_counters.c_sbm & 0xff;
		ctr->ctr_ether.est_collis = ds->ds_counters.c_collis;
		ctr->ctr_ether.est_recvfail = ds->ds_counters.c_rcverr;
		ctr->ctr_ether.est_recvfail_bm = ds->ds_counters.c_rbm & 0xff;
		ctr->ctr_ether.est_unrecog = ds->ds_unrecog;
		ctr->ctr_ether.est_sysbuf = ds->ds_counters.c_ibuferr;
		ctr->ctr_ether.est_userbuf = ds->ds_counters.c_lbuferr;
		ctr->ctr_ether.est_mbytesent = *(int *)ds->ds_counters.c_mbsent;
		ctr->ctr_ether.est_mbloksent = *(int *)ds->ds_counters.c_mpsent;
		if (cmd == SIOCRDZCTRS) {
			ds->ds_ztime = time.tv_sec;
			ds->ds_unrecog = 0;
		}
		smp_unlock(&ds->lk_de_softc);		
		break;
		}

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		deinit(ifp->if_unit);
		switch(ifa->ifa_addr.sa_family) {
#ifdef INET
		case AF_INET:
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif

		default:
			if (pr=iffamily_to_proto(ifa->ifa_addr.sa_family)) {
				error = (*pr->pr_ifioctl)(ifp, cmd, data);
			}
			break;
		}
		break;
#ifdef  IFF_PROMISC     /* IFF_ALLMULTI and NPACKETFILTER, as well */
	case SIOCSIFFLAGS:
		/*
		 * Right now we simply make sure of promiscuous
		 * mode (other things might need handling, too);
		 */
		if (ifp->if_flags & IFF_RUNNING)
			if (ifp->if_flags & IFF_PROMISC)
				error = depromiscuous(ifp, 1);
			else
				error = depromiscuous(ifp, 0);
		break;
#endif  IFF_PROMISC

	default:
		error = EINVAL;
	}
done:	splx(s);
	return (error);
}



/*
 * enable or disable internal loopback   LSC001
 */
deloopback( ui, ifp, ds, addr, lb_ctl )
register struct uba_device *ui;
register struct ifnet *ifp;
register struct de_softc *ds;
register struct dedevice *addr;
u_char lb_ctl;
{
        int csr0;

        /*
         * read current mode register.
         */
        ds->ds_pcbb.pcbb0 = FC_RDMODE;
        addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
        while ((addr->pcsr0 & PCSR0_DNI) == 0) 
                ;
        csr0 = addr->pcsr0;
        addr->pchigh = csr0 >> 8;
        if (csr0 & PCSR0_PCEI) {
		if (ds->ds_devid == DEUNA)
                	printf("de%d: read mode register failed, csr0=%b csr1=%b\n", 
                        ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
		else
                	printf("de%d: read mode register failed, csr0=%b csr1=%b\n", 
                        ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);
                return(EIO);
        }

        /*
         * set or clear the loopback bit as a function of lb_ctl and
         * return mode register to driver.
         */
        if ( lb_ctl == 1 ) {
                ds->ds_pcbb.pcbb2 |= MOD_LOOP;
		if (ds->ds_devid == DELUA)
                	ds->ds_pcbb.pcbb2 |= MOD_INTL;
		else 
                	ds->ds_pcbb.pcbb2 &= ~MOD_HDX;
        } else {
                ds->ds_pcbb.pcbb2 &= ~MOD_LOOP;
		if (ds->ds_devid == DELUA)
                	ds->ds_pcbb.pcbb2 &= ~MOD_INTL;
		else 
                	ds->ds_pcbb.pcbb2 |= MOD_HDX;
        }

        ds->ds_pcbb.pcbb0 = FC_WTMODE;
        addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
        while ((addr->pcsr0 & PCSR0_DNI) == 0) 
                ;
        csr0 = addr->pcsr0;
        addr->pchigh = csr0 >> 8;
        if (csr0 & PCSR0_PCEI) {
		if (ds->ds_devid == DEUNA)
                	printf("de%d: write mode register failed, csr0=%b csr1=%b\n", 
                       		ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
		else
                	printf("de%d: write mode register failed, csr0=%b csr1=%b\n", 
                       		ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS_DELUA);

                return(EIO);
        }

        return(NULL);
}

/*
 * enable or disable promiscuous mode
 *	if promisc_ctl is non-zero then promiscuous else not
 */
depromiscuous(ifp, promisc_ctl)
register struct ifnet *ifp;
int promisc_ctl;
{
	register struct de_softc *ds = &de_softc[ifp->if_unit];
	register struct uba_device *ui = deinfo[ifp->if_unit];
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
        int csr0;

        /*
         * read current mode register.
         */
        ds->ds_pcbb.pcbb0 = FC_RDMODE;
        addr->pclow =
		CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
        while ((addr->pcsr0 & PCSR0_DNI) == 0) 
                ;
        csr0 = addr->pcsr0;
        addr->pchigh = csr0 >> 8;
        if (csr0 & PCSR0_PCEI) {
	    if (ds->ds_devid == DEUNA)
		printf("de%d: read mode register failed, csr0=%b csr1=%b\n", 
		    ui->ui_unit, csr0, PCSR0_BITS, addr->pcsr1, PCSR1_BITS);
	    else
		printf("de%d: read mode register failed, csr0=%b csr1=%b\n", 
		    ui->ui_unit, csr0, PCSR0_BITS,
		    addr->pcsr1, PCSR1_BITS_DELUA);
	    return(EIO);
        }

        /*
         * set or clear the promiscuous bit as a function of promisc_ctl and
         * return mode register to driver.
         */
        if (promisc_ctl == 1) {
                ds->ds_pcbb.pcbb2 |= MOD_PROM;
        }
	else {
                ds->ds_pcbb.pcbb2 &= ~MOD_PROM;
        }

        ds->ds_pcbb.pcbb0 = FC_WTMODE;
        addr->pclow =
		CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
        while ((addr->pcsr0 & PCSR0_DNI) == 0) 
                ;
        csr0 = addr->pcsr0;
        addr->pchigh = csr0 >> 8;
        if (csr0 & PCSR0_PCEI) {
	    if (ds->ds_devid == DEUNA)
        	printf("de%d: write mode register failed, csr0=%b csr1=%b\n", 
			ui->ui_unit, csr0, PCSR0_BITS,
			addr->pcsr1, PCSR1_BITS);
	    else
		printf("de%d: write mode register failed, csr0=%b csr1=%b\n", 
			ui->ui_unit, csr0, PCSR0_BITS,
			addr->pcsr1, PCSR1_BITS_DELUA);

	    return(EIO);
        }

        return(NULL);
}
#endif
