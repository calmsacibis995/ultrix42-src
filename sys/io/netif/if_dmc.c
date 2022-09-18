#ifndef lint
static char *sccsid = "@(#)if_dmc.c	4.1		(ULTRIX)	7/2/90";
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/vaxif/if_dmc.c
 *
 * 29-nov-88 -- jaw     remove sysproc stuff.
 *
 * 02-mar-87  -- ejf    check address of protocol switch routines before
 *			calling them.
 *
 * 04-jan-86  -- ejf    fixed problem whereby DECnet counters were being
 *			zeroed whenever the line cycled.
 *
 * 16-dec-86  -- ejf    manipulate Tx Q at splimp, not spl5.
 *
 * 18-apr-86  -- ejf     added DECnet support.
 *
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 *  03/12/86 -- Chet Juszczak
 *		Modify MCLGET macro call to match new definition
 *
 *  09/16/85 -- Larry Cohen
 * 		Add 43bsd alpha tape changes for subnet routing
 *							
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 13 Mar 85 -- jaw
 *		add support for VAX 8200 and bua
 *
 * 29 Oct 84 -- jrs
 *	Update with latest version (1.12) from:
 *		Bill Nesheim (Cornell)
 *		Lou Salkind  (NYU)
 *
 *	Derived from 4.2BSD, labeled:
 *		if_dmc.c 6.1	83/07/29
 *
 * -----------------------------------------------------------------------
 */

#include "dmc.h"
#if NDMC > 0 || defined(BINARY)

/*
 * DMC11 device driver, internet version
 */

/* #define DEBUG 	/* for base table dump on fatal error */

#include "../data/if_dmc_data.c"

int	dmctimer;			/* timer started? */
int	dmc_timeout = 25;		/* timeout value */
int	dmcwatch();

#define printd if(dmcdebug)printf
int dmcdebug = 0;

/* error reporting intervals */
#define DMC_RPNBFS	50
#define DMC_RPDSC	50
#define	DMC_RPTMO	10
#define DMC_RPDCK	10



struct mbuf *dmc_get();

extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern struct timeval time;
extern int boot_cpu_mask;

/*
 * DMC software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * sc_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a  set of 7 UBA interface structures
 * for each, which
 * contain information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */

/* flags */
#define DMC_ALLOC       01              /* unibus resources allocated */
#define DMC_BMAPPED     02              /* base table mapped */
#define	DMC_RESTART	04		/* software restart in progress */
#define	DMC_ACTIVE	08		/* device active */


/*
 * Driver information for auto-configuration stuff.
 */
int     dmcprobe(), dmcattach(), dmcinit(), dmcioctl();
int     dmcoutput(), dmcreset();
u_short dmcstd[] = { 0 };
struct  uba_driver dmcdriver =
        { dmcprobe, 0, dmcattach, 0, dmcstd, "dmc", dmcinfo };

/*
 * dmc software packet encapsulation.  This allows the dmc
 * link to be multiplexed among several protocols.
 * The first eight bytes of the dmc header are garbage,
 * since on a vax the uba has been known to mung these
 * bytes.  The next two bytes encapsulate packet type.
 */
struct dmc_header {
	char	dmc_buf[8];	/* space for uba on vax */
	short	dmc_type;	/* encapsulate packet type */
};

/* queue manipulation macros */
#define	QUEUE_AT_HEAD(qp, head, tail) \
	(qp)->qp_next = (head); \
	(head) = (qp); \
	if((tail) == (struct dmc_command *) 0) \
		(tail) = (head) 

#define QUEUE_AT_TAIL(qp, head, tail) \
	if((tail)) \
		(tail)->qp_next = (qp); \
	else \
		(head) = (qp); \
	(qp)->qp_next = (struct dmc_command *) 0; \
	(tail) = (qp)

#define DEQUEUE(head, tail) \
	(head) = (head)->qp_next;\
	if((head) == (struct dmc_command *) 0)\
		(tail) = (head)


dmcprobe(reg)
        caddr_t reg;
{
        register struct dmcdevice *addr = (struct dmcdevice *)reg;
        register int i;

#ifdef lint
        dmcrint(0); dmcxint(0);
#endif
        addr->bsel1 = DMC_MCLR;
        for (i = 100000; i && (addr->bsel1 & DMC_RUN) == 0; i--)
                ;
        if ((addr->bsel1 & DMC_RUN) == 0) {
		printf("dmcprobe: can't start device\n" );
                return (0);
	}
        addr->bsel0 = DMC_RQI|DMC_IEI;
	/* let's be paranoid */
        addr->bsel0 |= DMC_RQI|DMC_IEI;
        DELAY(1000000);
        addr->bsel1 = DMC_MCLR;
        for (i = 100000; i && (addr->bsel1 & DMC_RUN) == 0; i--)
                ;
        return (1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
dmcattach(ui)
        register struct uba_device *ui;
{
        register struct dmc_softc *sc = &dmc_softc[ui->ui_unit];
	struct dmcdevice *addr = (struct dmcdevice *)dmcinfo[ui->ui_unit]->ui_addr;

        sc->sc_if.if_unit = ui->ui_unit;
        sc->sc_if.if_name = "dmc";
        sc->sc_if.if_mtu = DMCMTU;
        sc->sc_if.if_init = dmcinit;
        sc->sc_if.if_output = dmcoutput;
        sc->sc_if.if_ioctl = dmcioctl;
        sc->sc_if.if_reset = dmcreset;
        sc->sc_if.if_flags = IFF_POINTOPOINT | IFF_DYNPROTO;
        sc->sc_if.d_affinity = boot_cpu_mask;
        sc->sc_ifuba.ifu_flags = UBA_CANTWAIT;

	bzero(&sc->sc_dmccs, sizeof(sc->sc_dmccs));
	bzero(sc->sc_basectrs, sizeof(sc->sc_basectrs));
	bzero(sc->sc_errctrs, sizeof(sc->sc_errctrs));
	sc->sc_dmccs.if_family = AF_UNSPEC;
	sc->sc_dmccs.if_next_family = AF_UNSPEC;
	sc->sc_dmccs.if_mode = ui->ui_flags;

	if ( (sc->sc_bufres.dmrdev = dmc_is_dmr(addr)) && 
		(sc->sc_bufres.nxmt = ((NXMT_MASK & ui->ui_flags) >> NXMT_SHIFT)) > NXMT_MIN ) {
		if (sc->sc_bufres.nxmt > NXMT_MAX) 
			sc->sc_bufres.nxmt = NXMT_MAX;
	} else
		sc->sc_bufres.nxmt = NXMT_MIN;
	sc->sc_bufres.nrcv = NRCV;
	sc->sc_bufres.ntot = sc->sc_bufres.nrcv + sc->sc_bufres.nxmt;
	sc->sc_bufres.ncmds = sc->sc_bufres.ntot + 4;

        if_attach(&sc->sc_if);
	if (dmctimer == 0) {
		dmctimer = 1;
		timeout(dmcwatch, (caddr_t) 0, hz);
	}
}

/*
 * Check to see if device is a dmc or dmr.
 * Return 0 if dmc, return 1 if dmr.
 */
dmc_is_dmr(addr)
struct dmcdevice *addr;
{
	int i;
	addr->bsel3 = DMC_DMRCHK;
        addr->bsel1 = DMC_MCLR;
        for (i = 100000; i && (addr->bsel1 & DMC_RUN) == 0; i--)
                ;
        if ((addr->bsel1 & DMC_RUN) == 0) {
		printf("dmc_is_dmr: can't start device\n" );
                return (0);
	}

	switch ( addr->bsel3 & DMR_MASK )
	{
		case DMR_0001:
		case DMR_0002:
		case DMR_0100:
		case DMR_0200:
			return(1);

		default:
			return(0);
	}
}

/*
 * Reset of interface after UNIBUS reset.
 * If interface is on specified UBA, reset it's state.
 */
dmcreset(unit, uban)
        int unit, uban;
{
        register struct uba_device *ui;
        register struct dmc_softc *sc = &dmc_softc[unit];

        if (unit >= nNDMC || (ui = dmcinfo[unit]) == 0 || ui->ui_alive == 0 ||
            ui->ui_ubanum != uban)
                return;
        printf(" dmc%d", unit);
        sc->sc_flag = 0;
        dmcinit(unit);
}

/*
 * Initialization of interface; reinitialize UNIBUS usage.
 */
dmcinit(unit)
        int unit;
{
        register struct dmc_softc *sc = &dmc_softc[unit];
        register struct uba_device *ui = dmcinfo[unit];
        register struct dmcdevice *addr;
        register struct ifnet *ifp = &sc->sc_if;
        register struct ifrw *ifrw;
	register struct ifxmt *ifxp;
        register struct dmcbufs *rp;
	register struct dmc_command *qp;
	register struct ifaddr *ifa;
        int base;
	int s;

        addr = (struct dmcdevice *)ui->ui_addr;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr.sa_family && ifa->ifa_dstaddr.sa_family)
			break;
	if (ifa == (struct ifaddr *) 0)
		return;


        if((addr->bsel1&DMC_RUN) == 0) {
                printf("dmcinit: DMC not running\n");
                ifp->if_flags &= ~(IFF_RUNNING|IFF_UP);
                return;
        }
        /* map base table */
        if ((sc->sc_flag & DMC_BMAPPED) == 0) {
                sc->sc_ubinfo = uballoc(ui->ui_ubanum,
                        (caddr_t)&dmc_base[unit],
                        sizeof (struct dmc_base), 0);
                sc->sc_flag |= DMC_BMAPPED;
        }
        /* initialize UNIBUS resources */
        sc->sc_iused = sc->sc_oused = 0;
        if ((sc->sc_flag & DMC_ALLOC) == 0) {
                if (dmc_ubainit(&sc->sc_ifuba, ui->ui_ubanum,
		    sizeof(struct dmc_header), (int)btoc(DMCMTU), &sc->sc_bufres) == 0) {
                        printf("dmc%d: can't initialize\n", unit);
                        ifp->if_flags &= ~IFF_UP;
                        return;
                }
                sc->sc_flag |= DMC_ALLOC;
        }

        /* initialize buffer pool */
        /* recieves */
        ifrw= &sc->sc_ifuba.ifu_r[0];
        for(rp = &sc->sc_rbufs[0]; rp < &sc->sc_rbufs[sc->sc_bufres.nrcv]; rp++) {
                rp->ubinfo = ifrw->ifrw_info & 0x3ffff;
		rp->cc = DMCMTU + sizeof (struct dmc_header);
                rp->flags = DBUF_OURS|DBUF_RCV;
                ifrw++; 
        }
        /* transmits */
        ifxp= &sc->sc_ifuba.ifu_w[0];
        for(rp = &sc->sc_xbufs[0]; rp < &sc->sc_xbufs[sc->sc_bufres.nxmt]; rp++) {
                rp->ubinfo = ifxp->x_ifrw.ifrw_info & 0x3ffff;
                rp->cc = 0;
                rp->flags = DBUF_OURS|DBUF_XMIT;
                ifxp++; 
        }

	/* set up command queues */
	sc->sc_qfreeh = sc->sc_qfreet
		 = sc->sc_qhead = sc->sc_qtail = sc->sc_qactive =
		(struct dmc_command *)0;
	/* set up free command buffer list */
	for (qp = &sc->sc_cmdbuf[0]; qp < &sc->sc_cmdbuf[sc->sc_bufres.ncmds]; qp++) {
		QUEUE_AT_HEAD(qp, sc->sc_qfreeh, sc->sc_qfreet);
	}

	/*
	 * if coming through for the first time, zero out counters.
	 * else remember counters
	 */
	if ( ! (ifp->if_flags & IFF_RUNNING) ) {
		sc->sc_ztime = time.tv_sec;
		bzero(&sc->sc_rxtxctrs[DMCZ_RXBYTE], sizeof(sc->sc_rxtxctrs));
		bzero(sc->sc_errctrs, sizeof(sc->sc_errctrs));
	}
	else
		dmc_update_errctrs(sc, (u_char *) &dmc_base[unit].d_base[0]);
	bzero(sc->sc_basectrs, sizeof(sc->sc_basectrs));

	/* set up internal loopback if requested */
	if ( ifp->if_flags & IFF_LOOPBACK )
		addr->bsel1 |= DMC_ILOOP;

        /* base in */
        base = sc->sc_ubinfo & 0x3ffff;
        dmcload(sc, DMC_BASEI, base, (base>>2) & DMC_XMEM);
        /* specify half duplex operation, flags tell if primary */
        /* or secondary station */
        if (sc->sc_dmccs.if_mode == 0)
                /* use DDMCP mode in full duplex */
		dmcload(sc, DMC_CNTLI, 0, 0);
        else if (sc->sc_dmccs.if_mode == 1)
                /* use MAINTENENCE mode */
                dmcload(sc, DMC_CNTLI, 0, DMC_MAINT );
        else if (sc->sc_dmccs.if_mode == 2)
                /* use DDCMP half duplex as primary station */
                dmcload(sc, DMC_CNTLI, 0, DMC_HDPLX);
        else if (sc->sc_dmccs.if_mode == 3)
                /* use DDCMP half duplex as secondary station */
                dmcload(sc, DMC_CNTLI, 0, DMC_HDPLX | DMC_SEC);

        /* enable operation done interrupts */
        sc->sc_flag &= ~DMC_ACTIVE;
        while ((addr->bsel2 & DMC_IEO) == 0)
                addr->bsel2 |= DMC_IEO;
        s = splimp();
        /* queue first NRCV buffers for DMC to fill */
        for(rp = &sc->sc_rbufs[0]; rp < &sc->sc_rbufs[sc->sc_bufres.nrcv]; rp++) {
                rp->flags |= DBUF_DMCS;
                dmcload(sc, DMC_READ, rp->ubinfo,
                        (((rp->ubinfo>>2)&DMC_XMEM) | rp->cc));
                sc->sc_iused++;
        }
	splx(s);
        ifp->if_flags |= IFF_UP|IFF_RUNNING;
	if ( sc->sc_dmccs.if_mode == IFS_MOP )
		ifp->if_flags |= IFF_MOP;
	else
		ifp->if_flags &= ~IFF_MOP;

	/*
	 * Inform owner that device is up.
	 */
	if ( ifp->if_addr.sa_family != AF_UNSPEC ) {
		struct protosw *pr;
		sc->sc_dmccs.if_dstate = IFS_RUNNING;
		if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && pr->pr_ifstate) 
			(*pr->pr_ifstate)(ifp, IFS_RUNNING, &sc->sc_dmccs); 
	}

}
/*
 * Start output on interface.  Get another datagram
 * to send from the interface queue and map it to
 * the interface before starting output.
 *
 * Must be called at pri 5
 */
dmcstart(dev)
        dev_t dev;
{
        int unit = minor(dev);
        register struct dmc_softc *sc = &dmc_softc[unit];
        struct mbuf *m;
        register struct dmcbufs *rp;
	register int n;

        /*
         * Dequeue up to NXMT requests and map them to the UNIBUS.
         * If no more requests, or no dmc buffers available, just return.
         */
	n = 0;
        for(rp = &sc->sc_xbufs[0]; rp < &sc->sc_xbufs[sc->sc_bufres.nxmt]; rp++ ) {
                /* find an available buffer */
                if ((rp->flags & DBUF_DMCS) == 0) {
                        IF_DEQUEUE(&sc->sc_if.if_snd, m);
                        if (m == 0)
                                return;
                        /* mark it dmcs */
                        rp->flags |= (DBUF_DMCS);
                        /*
                        * Have request mapped to UNIBUS for transmission
                        * and start the output.
                        */
                        rp->cc = dmcput(&sc->sc_ifuba, n, m);
			rp->cc &= DMC_CCOUNT;
                        sc->sc_oused++;
                        dmcload(sc, DMC_WRITE, rp->ubinfo, 
                                rp->cc | ((rp->ubinfo>>2)&DMC_XMEM));
                }
		n++;
        }
}

/*
 * Utility routine to load the DMC device registers.
 */
dmcload(sc, type, w0, w1)
        register struct dmc_softc *sc;
        int type, w0, w1;
{
        register struct dmcdevice *addr;
        register int unit, sps;
	register struct dmc_command *qp;

	unit = sc->sc_if.if_unit;
        addr = (struct dmcdevice *)dmcinfo[unit]->ui_addr;
        sps = splimp();

	/* grab a command buffer from the free list */
	if((qp = sc->sc_qfreeh) == (struct dmc_command *)0) {
		printf("dmc%d: no free command buffer\n", unit);
        	splx(sps);
		return;
	}
	DEQUEUE(sc->sc_qfreeh, sc->sc_qfreet);

	/* fill in requested info */
	qp->qp_cmd = (type | DMC_RQI);
	qp->qp_ubaddr = w0;
	qp->qp_cc = w1;
	
	if (sc->sc_qactive){	/* command in progress */
		if(type == DMC_READ) {
			QUEUE_AT_HEAD(qp, sc->sc_qhead, sc->sc_qtail);
		} else {
			QUEUE_AT_TAIL(qp, sc->sc_qhead, sc->sc_qtail);
		}
	} else {	/* command port free */
		sc->sc_qactive = qp;
		addr->bsel0 = qp->qp_cmd;
		dmcrint(unit);
	}
        splx(sps);
}

/*
 * DMC interface receiver interrupt.
 * Ready to accept another command,
 * pull one off the command queue.
 */
dmcrint(unit)
        int unit;
{
        register struct dmc_softc *sc;
        register struct dmcdevice *addr;
        register struct dmc_command *qp;
	register int n;

        addr = (struct dmcdevice *)dmcinfo[unit]->ui_addr;
        sc = &dmc_softc[unit];
	if ((qp = sc->sc_qactive) == (struct dmc_command *) 0) {
		printf("dmcrint: no command for dmc%d\n", unit);
		return;
	}
        while (addr->bsel0&DMC_RDYI) {
                addr->sel4 = qp->qp_ubaddr;
                addr->sel6 = qp->qp_cc;
                addr->bsel0 &= ~(DMC_IEI|DMC_RQI);
		/* free command buffer */
		QUEUE_AT_HEAD(qp, sc->sc_qfreeh, sc->sc_qfreet);
                while (addr->bsel0&DMC_RDYI)
                        /* can't check for RDYO here 'cause this
                           routine isn't reentrant!             */
                                ;
		/* move on to next command */
                if ((sc->sc_qactive = sc->sc_qhead) == (struct dmc_command *)0)
                        break;		/* all done */
		/* more commands to do, start the next one */
		qp = sc->sc_qactive;
		DEQUEUE(sc->sc_qhead, sc->sc_qtail);
                addr->bsel0 =  qp->qp_cmd;
                n = RDYSCAN;
                while (n-- > 0)
			if ((addr->bsel0&DMC_RDYI) || (addr->bsel2&DMC_RDYO))
				break;
        }
        if (sc->sc_qactive) {
                addr->bsel0 |= DMC_IEI|DMC_RQI;
                /* VMS does it twice !*$%@# */
                addr->bsel0 |= DMC_IEI|DMC_RQI;
        }

}

/*
 * DMC interface transmitter interrupt.
 * A transfer may have completed, check for errors.
 * If it was a read, notify appropriate protocol.
 * If it was a write, pull the next one off the queue.
 */
dmcxint(unit)
        int unit;
{
        register struct dmc_softc *sc;
        register struct ifnet *ifp;
        struct uba_device *ui = dmcinfo[unit];
        struct dmcdevice *addr;
        struct mbuf *m;
        struct ifqueue *inq;
        int arg, pkaddr, cmd, len;
        register struct ifrw *ifrw;
        register struct dmcbufs *rp;
	register struct ifxmt *ifxp;
	struct dmc_header *dh;
	int off, resid;
	struct protosw *pr;

        addr = (struct dmcdevice *)ui->ui_addr;
        sc = &dmc_softc[unit];
        ifp = &sc->sc_if;

	while (addr->bsel2 & DMC_RDYO) {

		cmd = addr->bsel2 & 0xff;
		arg = addr->sel6 & 0xffff;
		/* reconstruct UNIBUS address of buffer returned to us */
		pkaddr = ((arg&DMC_XMEM)<<2) | (addr->sel4 & 0xffff);
		/* release port */
		addr->bsel2 &= ~DMC_RDYO;
		switch (cmd & 07) {

		case DMC_OUR:
			/*
			 * A read has completed.  
			 * Pass packet to type specific
			 * higher-level input routine.
			 */
			ifp->if_ipackets++;
			/*
			 * Accumulate statistics for DECnet
			 */
			if ( (sc->sc_rxtxctrs[DMCZ_RXBYTE] + (arg & DMC_CCOUNT)) > sc->sc_rxtxctrs[DMCZ_RXBYTE] )
				sc->sc_rxtxctrs[DMCZ_RXBYTE] += (arg & DMC_CCOUNT); 
			if ( sc->sc_rxtxctrs[DMCZ_RXBLOK] != 0xffffffff )
				sc->sc_rxtxctrs[DMCZ_RXBLOK]++;

			/* find location in dmcuba struct */
			ifrw= &sc->sc_ifuba.ifu_r[0];
			for (rp = &sc->sc_rbufs[0]; rp < &sc->sc_rbufs[sc->sc_bufres.nrcv]; rp++) {
				if(rp->ubinfo == pkaddr)
					break;
				ifrw++;
			}
			if (rp >= &sc->sc_rbufs[sc->sc_bufres.nrcv])
				panic("dmc rcv");
			if ((rp->flags & DBUF_DMCS) == 0)
				printf("dmc%d: done unalloc rbuf\n", unit);

			if ( sc->sc_dmccs.if_nomuxhdr ) {
				len = (arg & DMC_CCOUNT); 
			} else {
				len = (arg & DMC_CCOUNT) - sizeof (struct dmc_header);
			}

			if (len < 0 || len > DMCMTU) {
				ifp->if_ierrors++;
				printd("dmc%d: bad rcv pkt addr 0x%x len 0x%x\n",
				    unit, pkaddr, len);
				goto setup;
			}
			/* 
			 * If using dmc header, then
			 * deal with trailer protocol: if type is trailer
			 * get true type from first 16-bit word past data.
			 * Remember that type was trailer by setting off.
			 */
			if ( ! sc->sc_dmccs.if_nomuxhdr ) {
				dh = (struct dmc_header *)ifrw->ifrw_addr;
				dh->dmc_type = ntohs((u_short)dh->dmc_type);
#define	dmcdataaddr(dh, off, type)	((type)(((caddr_t)((dh)+1)+(off))))
				if (dh->dmc_type >= DMC_TRAILER &&
				    dh->dmc_type < DMC_TRAILER+DMC_NTRAILER) {
					off = (dh->dmc_type - DMC_TRAILER) * 512;
					if (off >= DMCMTU)
						goto setup;		/* sanity */
					dh->dmc_type = ntohs(*dmcdataaddr(dh, off, u_short *));
					resid = ntohs(*(dmcdataaddr(dh, off+2, u_short *)));
					if (off + resid > len)
						goto setup;		/* sanity */
					len = off + resid;
				} else
					off = 0;
			} else
				off = 0;
			if (len == 0)
				goto setup;

			/*
			 * Pull packet off interface.  Off is nonzero if
			 * packet has trailing header; dmc_get will then
			 * force this header information to be at the front,
			 * but we still have to drop the type and length
			 * which are at the front of any trailer data.
			 */
			m = dmc_get(&sc->sc_ifuba, ifrw, len, off, ! (sc->sc_dmccs.if_nomuxhdr));
			if (m == 0)
				goto setup;
			if (off) {
				m->m_off += 2 * sizeof (u_short);
				m->m_len -= 2 * sizeof (u_short);
			}

			/*
			 * Find protocol to which packet is destined
			 */
			if ( ! sc->sc_dmccs.if_nomuxhdr ) {
				/*
				 * Multiplexed - find protocol as a
				 * function of packet type
				 */
				switch (dh->dmc_type) {
	
#ifdef INET
				case DMC_IPTYPE:
					if (nINET) {
						inq = &ipintrq;
						smp_lock(&ipintrq.lk_ifqueue, LK_RETRY);
						schednetisr(NETISR_IP);
						break;
					} else {
						printf("dmc%d: unknown address type %d\n",
							unit, ifp->if_addr.sa_family);
						goto setup;
					}
#endif
				default:
				       /*
				 	* see if other protocol families defined
				 	* and call protocol specific routines.
				 	* If no other protocols defined then dump message.
				 	*/
					if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && pr->pr_ifinput) {
						if (( m = (struct mbuf *)(*pr->pr_ifinput)(m, ifp, &inq, NULL)) == 0)
							goto setup;
					} else {
	                        		printf("dmc%d: unknown address type %d\n", unit,
	                            			ifp->if_addr.sa_family);
						m_freem(m);
						goto setup;
					}
				}
			}
			else
			{
				/*
				 * Not multiplexed - find protocol as a
				 * function of family
				 */
				if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && pr->pr_ifinput) {
					if (( m = (struct mbuf *)(*pr->pr_ifinput)(m, ifp, &inq, NULL)) == 0) 
						goto setup;
				} else {
	                        	printf("dmc%d: unknown address type %d\n", unit,
	                            		ifp->if_addr.sa_family);
					m_freem(m);
					goto setup;
				}
			}

			if (IF_QFULL(inq)) {
				IF_DROP(inq);
				m_freem(m);
			} else
				IF_ENQUEUEIF(inq, m, ifp);
                        smp_unlock(&inq->lk_ifqueue);

	setup:
			/* is this needed? */
			rp->ubinfo = ifrw->ifrw_info & 0x3ffff;

			dmcload(sc, DMC_READ, rp->ubinfo, 
			    ((rp->ubinfo >> 2) & DMC_XMEM) | rp->cc);
			break;

		case DMC_OUX:
			/*
			 * A write has completed, start another
			 * transfer if there is more data to send.
			 */
			ifp->if_opackets++;
			/*
			 * Accumulate statistics for DECnet
			 */
			if ( (sc->sc_rxtxctrs[DMCZ_TXBYTE] + (arg & DMC_CCOUNT)) > sc->sc_rxtxctrs[DMCZ_TXBYTE] )
				sc->sc_rxtxctrs[DMCZ_TXBYTE] += (arg & DMC_CCOUNT); 
			if ( sc->sc_rxtxctrs[DMCZ_TXBLOK] != 0xffffffff )
				sc->sc_rxtxctrs[DMCZ_TXBLOK]++;


			/* find associated dmcbuf structure */
			ifxp = &sc->sc_ifuba.ifu_w[0];
			for (rp = &sc->sc_xbufs[0]; rp < &sc->sc_xbufs[sc->sc_bufres.nxmt]; rp++) {
				if(rp->ubinfo == pkaddr)
					break;
				ifxp++;
			}
			if (rp >= &sc->sc_xbufs[sc->sc_bufres.nxmt]) {
				printf("dmc%d: bad packet address 0x%x\n",
				    unit, pkaddr);
				break;
			}
			if ((rp->flags & DBUF_DMCS) == 0)
				printf("dmc%d: unallocated packet 0x%x\n",
				    unit, pkaddr);
			/* mark buffer free */
			if (ifxp->x_xtofree) {
				(void)m_freem(ifxp->x_xtofree);
				ifxp->x_xtofree = 0;
			}
			rp->flags &= ~DBUF_DMCS;
			sc->sc_oused--;
			sc->sc_nticks = 0;
			sc->sc_flag |= DMC_ACTIVE;
			break;

		case DMC_CNTLO:
			arg &= DMC_CNTMASK;
			if (arg & DMC_FATAL) {
				sc->sc_dmccs.if_dstate = ( arg & DMC_MAINTREC ) ? IFS_ENTEREDMOP : IFS_HALTED;
				if (pr=iffamily_to_proto(ifp->if_addr.sa_family)) 
					if (pr->pr_ifstate && (*pr->pr_ifstate)(ifp, sc->sc_dmccs.if_dstate, &sc->sc_dmccs)) 
						dmcstate(ifp, sc, ui);
					else
						printd("dmc%d: fatal error, flags=%b\n", unit, arg, CNTLO_BITS);
				else
					printd("dmc%d: fatal error, flags=%b\n", unit, arg, CNTLO_BITS);
				ifp->if_flags &= ~IFF_UP;
				if ( ifp->if_addr.sa_family == AF_UNSPEC || sc->sc_dmccs.if_dstate == IFS_STARTING ) {
					dmcrestart(unit);
				}
				break;
			}
			/* ACCUMULATE STATISTICS */
			switch(arg) {
			case DMC_NOBUFS:
				ifp->if_ierrors++;
				if ((sc->sc_nobuf++ % DMC_RPNBFS) == 0)
					goto report;
				break;
			case DMC_DISCONN:
				if ((sc->sc_disc++ % DMC_RPDSC) == 0)
					goto report;
				break;
			case DMC_TIMEOUT:
				if ((sc->sc_timeo++ % DMC_RPTMO) == 0)
					goto report;
				break;
			case DMC_DATACK:
				ifp->if_oerrors++;
				if ((sc->sc_datck++ % DMC_RPDCK) == 0)
					goto report;
				break;
			default:
				goto report;
			}
			break;
		report:
			printd("dmc%d: soft error, flags=%b\n", unit,
			    arg, CNTLO_BITS);
			if ((sc->sc_flag & DMC_RESTART) == 0) {
				/*
				 * kill off the dmc to get things
				 * going again by generating a
				 * procedure error
				 */
				sc->sc_flag |= DMC_RESTART;
				arg = sc->sc_ubinfo & 0x3ffff;
				dmcload(sc, DMC_BASEI, arg, (arg>>2)&DMC_XMEM);
			}
			break;

		default:
			printf("dmc%d: bad control %o\n", unit, cmd);
			break;
		}
	}
	dmcstart(unit);
        return;
}

/*
 * DMC state routine
 * This routine either halts the DMC or sets it up to operate
 * in a particular mode.
 */
dmcstate(ifp, sc, ui)
        register struct ifnet *ifp;
        register struct dmc_softc *sc;
        register struct uba_device *ui;
{

	ifp->if_addr.sa_family = sc->sc_dmccs.if_family; 

	if ( sc->sc_dmccs.if_ustate == IFS_USROFF ) {
		sc->sc_dmccs.if_mode = ui->ui_flags;
		sc->sc_dmccs.if_dstate = IFS_HALTED;
	} else {
		    sc->sc_dmccs.if_dstate = IFS_STARTING;
	}
}

/*
 * DMC output routine.
 * Encapsulate a packet of type family for the dmc.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 */
dmcoutput(ifp, m0, dst)
        register struct ifnet *ifp;
        register struct mbuf *m0;
        struct sockaddr *dst;
{
	register struct dmc_softc *sc = &dmc_softc[ifp->if_unit];
        int type, error, s;
        register struct mbuf *m = m0;
	register struct dmc_header *dh;
	register int off;
	struct protosw *pr;
        struct ifqueue *inq;

	/*
	 * check for device ownership if if_nomuxhdr is asserted!
	 */
	if ( (sc->sc_dmccs.if_nomuxhdr) && (ifp->if_addr.sa_family != dst->sa_family) )
	{
		error = EADDRINUSE;
		goto bad;
	}

	switch (dst->sa_family) {
#ifdef	INET
	case AF_INET:
		off = ntohs((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;
		if ((ifp->if_flags & IFF_NOTRAILERS) == 0)
		if (off > 0 && (off & 0x1ff) == 0 &&
		    m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = DMC_TRAILER + (off>>9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = htons((u_short)DMC_IPTYPE);
			*(mtod(m, u_short *) + 1) = htons((u_short)m->m_len);
			goto gottrailertype;
		}
		type = DMC_IPTYPE;
		off = 0;
		goto gottype;
#endif

	case AF_UNSPEC:
		dh = (struct dmc_header *)dst->sa_data;
		type = dh->dmc_type;
		goto gottype;

	default:
		if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && pr->pr_ifoutput) 
			(*pr->pr_ifoutput)(ifp, m, dst, &type, NULL);
		else {
			printf("dmc%d: can't handle af%d\n", ifp->if_unit,
				dst->sa_family);
			error = EAFNOSUPPORT;
			goto bad;
		}
		goto gottype;
        }

gottrailertype:
	/*
	 * Packet to be sent as a trailer; move first packet
	 * (control information) to end of chain.
	 */
	while (m->m_next)
		m = m->m_next;
	m->m_next = m0;
	m = m0->m_next;
	m0->m_next = 0;
	m0 = m;

gottype:
	/*
	 * Add local header only if if_nomuxhdr is NOT asserted!
	 * (there is space for a uba on a vax to step on)
	 */
	if ( ! sc->sc_dmccs.if_nomuxhdr ) {
		if (m->m_off > MMAXOFF ||
		    MMINOFF + sizeof(struct dmc_header) > m->m_off) {
			m = m_get(M_DONTWAIT, MT_DATA);
			if (m == 0) {
				error = ENOBUFS;
				goto bad;
			}
			m->m_next = m0;
			m->m_off = MMINOFF;
			m->m_len = sizeof (struct dmc_header);
		} else {
			m->m_off -= sizeof (struct dmc_header);
			m->m_len += sizeof (struct dmc_header);
		}
		dh = mtod(m, struct dmc_header *);
		dh->dmc_type = htons((u_short)type);
	}

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
        if (IF_QFULL(&ifp->if_snd)) {
                IF_DROP(&ifp->if_snd);
                m_freem(m);
                splx(s);
                return (ENOBUFS);
        }
        IF_ENQUEUE(&ifp->if_snd, m);
        dmcstart(ifp->if_unit);
        splx(s);
        return(0);

bad:
	m_freem(m0);
	return(error);
}


/*
 * Process an ioctl request.
 */
dmcioctl(ifp, cmd, data)
        register struct ifnet *ifp;
        int cmd;
        caddr_t data;
{
        int s = splimp(), error = 0;
	int arg;
	register struct dmc_softc *sc = &dmc_softc[ifp->if_unit];
	register struct uba_device *ui = dmcinfo[ifp->if_unit];

        switch (cmd) {

	case SIOCSTATE:
		/*
		 * read and/or set current ownership and state of device.
		 */
		{
			register struct ifstate *dmcstate = (struct ifstate *) data;
			register struct protosw *pr;
			register struct ifaddr *ifa;

			for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
				if (ifa->ifa_addr.sa_family && ifa->ifa_dstaddr.sa_family)
					break;

			if (ifa == (struct ifaddr *) 0)
				return(ENETDOWN);

			if ( ! dmcsuser() ) 
				return(EACCES);

			/*
			 * If currently owned by another family, request
			 * ownership from that family.
			 */
			if (ifp->if_addr.sa_family && (ifp->if_addr.sa_family != dmcstate->if_family)) {
				if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && pr->pr_ifstate) 
				{
					if (! (*pr->pr_ifstate)(ifp, IFS_OWNREQ, dmcstate)) 
						return(EBUSY);
					else
						dmcstate->if_next_family = ifp->if_addr.sa_family;
				}
				else
					return(EBUSY);
			}

			/*
			 * check validity of ioctl request
			 */
			if ( !( dmcstate->if_wrstate | dmcstate->if_rdstate | dmcstate->if_xferctl ) )
				return(EINVAL);

			/*
			 * set line ownership
			 */
			if ( dmcstate->if_wrstate ) {
				ifp->if_addr.sa_family = dmcstate->if_family;
				bcopy(dmcstate, &sc->sc_dmccs, sizeof(struct ifstate));
				sc->sc_dmccs.if_wrstate = ~IFS_WRSTATE;
				sc->sc_dmccs.if_rdstate = ~IFS_RDSTATE;
				sc->sc_dmccs.if_xferctl = ~IFS_XFERCTL;
			}

			/*
			 * current owner can transfer control to another family
			 */
			if ( dmcstate->if_xferctl && dmcstate->if_wrstate ) {
				ifp->if_addr.sa_family = (sc->sc_dmccs.if_family = sc->sc_dmccs.if_next_family);
				sc->sc_dmccs.if_next_family = ifp->if_addr.sa_family;
			}

			if ( dmcstate->if_wrstate ) {
				/*
				 * If stopping line, force procedure error in
				 *	order to drop dtr and restart device.
				 * Else if currently running, just restart device.
				 * Otherwise start protocol.
				 */
				if ( dmcstate->if_ustate == IFS_USROFF &&
					(sc->sc_flag & (DMC_ALLOC | DMC_BMAPPED)) == (DMC_ALLOC | DMC_BMAPPED) &&
					(sc->sc_flag & DMC_RESTART) == 0 ) {
					sc->sc_flag |= DMC_RESTART;
					ifp->if_flags &= ~IFF_UP;
					sc->sc_dmccs.if_dstate = IFS_HALTING;
					arg = sc->sc_ubinfo & 0x3ffff;
					dmcload(sc, DMC_BASEI, arg, (arg>>2)&DMC_XMEM);
				} else {
					if ( (sc->sc_flag & DMC_RESTART) == 0 ) {
						sc->sc_flag |= DMC_RESTART;
						ifp->if_flags &= ~IFF_UP;
						sc->sc_dmccs.if_dstate = IFS_HALTING;
						dmcrestart(ifp->if_unit);
					}
				}
			}
			/*
			 * pass back current state if requested.
			 */
			if ( dmcstate->if_rdstate ) {
				bcopy(&sc->sc_dmccs, dmcstate, sizeof(struct ifstate));
			}
		break;
		}

	case SIOCENABLBACK:
	/*
	 * place device in loopback
	 */
		if ( ! dmcsuser() ) 
			return(EACCES);

		printf("dmc%d: internal loopback enable requested\n", ifp->if_unit);
		ifp->if_flags |= IFF_LOOPBACK;
		/*
		 * kill off the dmc to reset state
		 * by generating a procedure error
		 */
		if ((sc->sc_flag & DMC_RESTART) == 0) {
			sc->sc_dmccs.if_dstate = IFS_HALTING;
			ifp->if_flags &= ~IFF_UP;
			sc->sc_flag |= DMC_RESTART;
			arg = sc->sc_ubinfo & 0x3ffff;
			dmcload(sc, DMC_BASEI, arg, (arg>>2)&DMC_XMEM);
		} else {
			if ( sc->sc_dmccs.if_dstate == IFS_HALTED )
				dmcrestart(ifp->if_unit);
		}
		break;

	case SIOCDISABLBACK:
	/*
	 * place device out of loopback
	 */
		if ( ! dmcsuser() ) 
			return(EACCES);

		printf("dmc%d: internal loopback disable requested\n", ifp->if_unit);
		ifp->if_flags &= ~IFF_LOOPBACK;
		/*
		 * kill off the dmc to reset state
		 * by generating a procedure error
		 */
		if ((sc->sc_flag & DMC_RESTART) == 0) {
			sc->sc_dmccs.if_dstate = IFS_HALTING;
			ifp->if_flags &= ~IFF_UP;
			sc->sc_flag |= DMC_RESTART;
			arg = sc->sc_ubinfo & 0x3ffff;
			dmcload(sc, DMC_BASEI, arg, (arg>>2)&DMC_XMEM);
		} else {
			if ( sc->sc_dmccs.if_dstate == IFS_HALTED )
				dmcrestart(ifp->if_unit);
		}
		break;

	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		/*
		 * reading and zeroing line counters
		 */
	{
		int unit = ifp->if_unit;
		register struct ctrreq *ctr = (struct ctrreq *)data;

		dmc_update_errctrs(sc, (u_char *) &dmc_base[unit].d_base[0]);
		bzero(&ctr->ctr_ctrs, sizeof(struct dstat));
		ctr->ctr_type = CTR_DDCMP;
		ctr->ctr_ddcmp.dst_seconds = (time.tv_sec - sc->sc_ztime) > 0xfffe ? 0xffff : (time.tv_sec - sc->sc_ztime);
		ctr->ctr_ddcmp.dst_bytercvd = sc->sc_rxtxctrs[DMCZ_RXBYTE];
		ctr->ctr_ddcmp.dst_bytesent = sc->sc_rxtxctrs[DMCZ_TXBYTE];
		ctr->ctr_ddcmp.dst_blockrcvd = sc->sc_rxtxctrs[DMCZ_RXBLOK];
		ctr->ctr_ddcmp.dst_blocksent = sc->sc_rxtxctrs[DMCZ_TXBLOK];
		/* outbound erros */
		if ( sc->sc_errctrs[DMCZ_RXHCRC] ) {
			ctr->ctr_ddcmp.dst_outbound_bm |= (1 << CTR_HDRCRC);
		}
		if ( sc->sc_errctrs[DMCZ_RXDCRC] ) {
			ctr->ctr_ddcmp.dst_outbound_bm |= (1 << CTR_DATCRC);
		}
		if ( (short) (ctr->ctr_ddcmp.dst_outbound = (sc->sc_errctrs[DMCZ_RXHCRC] + sc->sc_errctrs[DMCZ_RXDCRC])) > 0x0ff )
			ctr->ctr_ddcmp.dst_outbound = 0x0ff;

		/* inbound errors */
		if ( sc->sc_errctrs[DMCZ_TXHCRC] ) {
			ctr->ctr_ddcmp.dst_inbound_bm |= (1 << CTR_HDRCRC);
		}
		if ( sc->sc_errctrs[DMCZ_TXDCRC] ) {
			ctr->ctr_ddcmp.dst_inbound_bm |= (1<< CTR_DATCRC);
		}
		if ( (short) (ctr->ctr_ddcmp.dst_inbound = (sc->sc_errctrs[DMCZ_TXHCRC] + sc->sc_errctrs[DMCZ_TXDCRC])) > 0x0ff )
			ctr->ctr_ddcmp.dst_outbound = 0x0ff;

		/* buffer unavailable */
		if (ctr->ctr_ddcmp.dst_remotebuf = sc->sc_errctrs[DMCZ_RXNOBUF] ) {
			ctr->ctr_ddcmp.dst_remotebuf_bm |= (1 << CTR_BUFUNAVAIL);
		}
		if (ctr->ctr_ddcmp.dst_localbuf = sc->sc_errctrs[DMCZ_TXNOBUF] ) {
			ctr->ctr_ddcmp.dst_localbuf_bm |= (1 << CTR_BUFUNAVAIL);
		}
		ctr->ctr_ddcmp.dst_remotetmo = sc->sc_errctrs[DMCZ_REMOTETMO];
		ctr->ctr_ddcmp.dst_localtmo = sc->sc_errctrs[DMCZ_LOCALTMO];

		if ( cmd == SIOCRDZCTRS ) {
			sc->sc_ztime = time.tv_sec;
			bzero(&sc->sc_rxtxctrs[DMCZ_RXBYTE], sizeof(sc->sc_rxtxctrs));
			bzero(sc->sc_errctrs, sizeof(sc->sc_errctrs));
		}
		break;
	}

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			dmcinit(ifp->if_unit); 
		break;

	case SIOCSIFDSTADDR:
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			dmcinit(ifp->if_unit); 
		break;
		
        default:
                error = EINVAL;
        }
        splx(s);
        return(error);
}

/*
 * update drivers copy of line error counters
 */
dmc_update_errctrs(sc, base_table)
register struct dmc_softc *sc;
register u_char *base_table;

{
	short tmp_cnt;

	if ( (tmp_cnt = base_table[DMCD_RXHCRC] - sc->sc_basectrs[DMCZ_RXHCRC]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_RXHCRC] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_RXHCRC] = 0xff;
		else
			sc->sc_errctrs[DMCZ_RXHCRC] += tmp_cnt;
	}
	if ( (tmp_cnt = base_table[DMCD_RXDCRC] - sc->sc_basectrs[DMCZ_RXDCRC]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_RXDCRC] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_RXDCRC] = 0xff;
		else
			sc->sc_errctrs[DMCZ_RXDCRC] += tmp_cnt;
	}
	if ( (tmp_cnt = base_table[DMCD_TXHCRC] - sc->sc_basectrs[DMCZ_TXHCRC]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_TXHCRC] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_TXHCRC] = 0xff;
		else
			sc->sc_errctrs[DMCZ_TXHCRC] += tmp_cnt;
	}
	if ( (tmp_cnt = base_table[DMCD_TXDCRC] - sc->sc_basectrs[DMCZ_TXDCRC]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_TXDCRC] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_TXDCRC] = 0xff;
		else
			sc->sc_errctrs[DMCZ_TXDCRC] += tmp_cnt;
	}
	if ( (tmp_cnt = base_table[DMCD_RXNOBUF] - sc->sc_basectrs[DMCZ_RXNOBUF]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_RXNOBUF] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_RXNOBUF] = 0xff;
		else
			sc->sc_errctrs[DMCZ_RXNOBUF] += tmp_cnt;
	}
	if ( (tmp_cnt = base_table[DMCD_TXNOBUF] - sc->sc_basectrs[DMCZ_TXNOBUF]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_TXNOBUF] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_TXNOBUF] = 0xff;
		else
			sc->sc_errctrs[DMCZ_TXNOBUF] += tmp_cnt;
	}
	if ( (tmp_cnt = base_table[DMCD_REMOTETMO] - sc->sc_basectrs[DMCZ_REMOTETMO]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_REMOTETMO] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_REMOTETMO] = 0xff;
		else
			sc->sc_errctrs[DMCZ_REMOTETMO] += tmp_cnt;
	}
	if ( (tmp_cnt = base_table[DMCD_LOCALTMO] - sc->sc_basectrs[DMCZ_LOCALTMO]) ) {
		if ( tmp_cnt < 0 || (sc->sc_errctrs[DMCZ_LOCALTMO] + tmp_cnt) > 0x0ff )
			sc->sc_errctrs[DMCZ_LOCALTMO] = 0xff;
		else
			sc->sc_errctrs[DMCZ_LOCALTMO] += tmp_cnt;
	}
	bcopy(&base_table[DMCD_RXNOBUF], &sc->sc_basectrs[DMCZ_RXNOBUF], DMCZ_SIZE);
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
dmc_ubainit(ifu, uban, hlen, nmr, bufres)
	register struct dmcuba *ifu;
	int uban, hlen, nmr;
	struct dmcbufres *bufres;
{
	register caddr_t cp, dp;
	register struct ifrw *ifrw;
	register struct ifxmt *ifxp;
	int i, ncl;

	ncl = clrnd(nmr + (hlen? CLSIZE: 0)) / CLSIZE;
	if (ifu->ifu_r[0].ifrw_addr)
		/*
		 * If the first read buffer has a non-zero
		 * address, it means we have already allocated core
		 */
		cp = ifu->ifu_r[0].ifrw_addr - (hlen? (CLBYTES - hlen): 0);
	else {
		cp = m_clalloc(bufres->ntot * ncl, MPG_SPACE);
		if (cp == 0)
			return (0);
		ifu->ifu_hlen = hlen;
		ifu->ifu_uban = uban;

		ifu->ifu_uba = uba_hd[uban].uh_uba;

		dp = cp + (hlen? (CLBYTES - hlen): 0);
		for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[bufres->nrcv]; ifrw++) {
			ifrw->ifrw_addr = dp;
			dp += ncl * CLBYTES;
		}
		for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[bufres->nxmt]; ifxp++) {
			ifxp->x_ifrw.ifrw_addr = dp;
			dp += ncl * CLBYTES;
		}
	}
	/* allocate for receive ring */
	for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[bufres->nrcv]; ifrw++) {
		if (dmc_ubaalloc(ifu, ifrw, nmr) == 0) {
			struct ifrw *rw;

			for (rw = ifu->ifu_r; rw < ifrw; rw++)
				ubarelse(ifu->ifu_uban, &rw->ifrw_info);
			goto bad;
		}
	}
	/* and now transmit ring */
	for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[bufres->nxmt]; ifxp++) {
		ifrw = &ifxp->x_ifrw;
		if (dmc_ubaalloc(ifu, ifrw, nmr) == 0) {
			struct ifxmt *xp;

			for (xp = ifu->ifu_w; xp < ifxp; xp++)
				ubarelse(ifu->ifu_uban, &xp->x_ifrw.ifrw_info);
			for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[bufres->nrcv]; ifrw++)
				ubarelse(ifu->ifu_uban, &ifrw->ifrw_info);
			goto bad;
		}
		for (i = 0; i < nmr; i++)
			ifxp->x_map[i] = ifrw->ifrw_mr[i];
		ifxp->x_xswapd = 0;
	}
	return (1);
bad:
	m_pgfree(cp, bufres->ntot * ncl);
	ifu->ifu_r[0].ifrw_addr = 0;
	return(0);
}

/*
 * Setup either a ifrw structure by allocating UNIBUS map registers,
 * possibly a buffered data path, and initializing the fields of
 * the ifrw structure to minimize run-time overhead.
 */
static
dmc_ubaalloc(ifu, ifrw, nmr)
	struct dmcuba *ifu;
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
	ifrw->ifrw_mr = &ifu->ifu_uba->uba_map[UBAI_MR(info) +(ifu->ifu_hlen? 1: 0)];

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
dmc_get(ifu, ifrw, totlen, off0, dmc_header)
	register struct dmcuba *ifu;
	register struct ifrw *ifrw;
	int totlen, off0, dmc_header;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = ifrw->ifrw_addr;
	register short hlen = 0;

	if ( dmc_header )
	{
		hlen = ifu->ifu_hlen;
		cp += ifu->ifu_hlen;
	}

	top = 0;
	mp = &top;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		if (off) {
			len = totlen - off;
			cp = ifrw->ifrw_addr + hlen + off;
		} else
			len = totlen;
		if (len >= CLBYTES) {
			struct mbuf *p;
			struct pte *cpte, *ppte;
			int x, *ip, i;

			MCLGET(m, p);
			if (p == 0)
				goto nopage;
			len = m->m_len = CLBYTES;
			if (!claligned(cp))
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
				cp = ifrw->ifrw_addr + hlen;
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
dmcput(ifu, n, m)
	struct dmcuba *ifu;
	int n;
	register struct mbuf *m;
{
	register struct mbuf *mp;
	register caddr_t cp;
	register struct ifxmt *ifxp;
	register struct ifrw *ifrw;
	register int i;
	int xswapd = 0;
	int x, cc, t;
	caddr_t dp;

	ifxp = &ifu->ifu_w[n];
	ifrw = &ifxp->x_ifrw;
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
			mp = m->m_next;
			m->m_next = ifxp->x_xtofree;
			ifxp->x_xtofree = m;
			cp += m->m_len;
		} else {
			bcopy(mtod(m, caddr_t), cp, (unsigned)m->m_len);
			cp += m->m_len;
			MFREE(m, mp);
		}
		m = mp;
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
 * Restart after a fatal error.
 * Clear device and reinitialize.
 */
dmcrestart(unit)
	int unit;
{
	register struct dmc_softc *sc = &dmc_softc[unit];
	register struct uba_device *ui = dmcinfo[unit];
	register struct dmcdevice *addr;
	register struct ifxmt *ifxp;
	register int i;
	register struct mbuf *m;
	struct dmcuba *ifu;
	
	addr = (struct dmcdevice *)ui->ui_addr;
	ifu = &sc->sc_ifuba;
#ifdef DEBUG
	/* dump base table */
	printf("dmc%d base table:\n", unit);
	for (i = 0; i < sizeof (struct dmc_base); i++)
		printf("%o\n" ,dmc_base[unit].d_base[i]);
#endif
	/*
	 * Let the DMR finish the MCLR.  At 1 Mbit, it should do so
	 * in about a max of 6.4 milliseconds with diagnostics enabled.
	 */
	addr->bsel1 = DMC_MCLR;
	for (i = 100000; i && (addr->bsel1 & DMC_RUN) == 0; i--)
		;
	/* Did the timer expire or did the DMR finish? */
	if ((addr->bsel1 & DMC_RUN) == 0) {
		printf("dmc%d: M820 Test Failed\n", unit);
		return;
	}

	/* purge send queue */
	IF_DEQUEUE(&sc->sc_if.if_snd, m);
	while (m) {
		m_freem(m);
		IF_DEQUEUE(&sc->sc_if.if_snd, m);
	}
        for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[sc->sc_bufres.nxmt]; ifxp++) {
		if (ifxp->x_xtofree) {
			(void) m_freem(ifxp->x_xtofree);
			ifxp->x_xtofree = 0;
		}
	}

	/* restart DMC */
	dmcinit(unit);
	sc->sc_flag &= ~DMC_RESTART;
	sc->sc_if.if_collisions++;	/* why not? */
}

/*
 * Check to see that transmitted packets don't
 * lose interrupts.  The device has to be active.
 */
dmcwatch()
{
        register struct uba_device *ui;
	register struct dmc_softc *sc;
	struct dmcdevice *addr;
	register int i;

	for (i = 0; i < nNDMC; i++) {
		sc = &dmc_softc[i];
		if ((sc->sc_flag & DMC_ACTIVE) == 0)
			continue;
        	if ((ui = dmcinfo[i]) == 0 || ui->ui_alive == 0)
			continue;
		if (sc->sc_oused) {
			sc->sc_nticks++;
			if (sc->sc_nticks > dmc_timeout) {
				sc->sc_nticks = 0;
        			addr = (struct dmcdevice *)ui->ui_addr;
				printd("dmc%d hung: bsel0=%b bsel2=%b\n", i,
				    addr->bsel0 & 0xff, DMC0BITS,
				    addr->bsel2 & 0xff, DMC2BITS);
				dmcrestart(i);
			}
		}
	}
	timeout(dmcwatch, (caddr_t) 0, hz);
}
/*
 * Check to make sure requestor is priveleged.  suser is
 * called when not on interrupt stack or when 
 * there is a system process.
 */
dmcsuser()
{
	if ( ! (movpsl() & PSL_IS))
		return(suser());
	return(1);
}
#endif
