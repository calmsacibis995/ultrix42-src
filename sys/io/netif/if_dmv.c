#ifndef lint
static char *sccsid = "@(#)if_dmv.c	4.2	(ULTRIX)	9/4/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * Modification History: /sys/vaxif/if_dmv.c
 *
 *  11/29/88 -- jaw - remove cursysproc stuff.
 *
 *  12/28/86 -- ejf - report threshold error only once.
 *
 *  12/16/86 -- ejf - fixed problem where IPL could be lowered below device ipl.
 *
 *  03/06/86 -- ejf - cloned from dmc driver
 *							
 * -----------------------------------------------------------------------
 */

#include "dmv.h"
#if NDMV > 0 || defined(BINARY)

/*
 * DMV11 device driver
 */


#include "../data/if_dmv_data.c"

int	dmvtimer;			/* timer started? */
int	dmv_timeout = 25;		/* timeout value */
int	dmvwatch();

#define printd if(dmvdebug)printf
#define printdmverr if(sc->sc_dmvcs.if_dstate == IFS_RUNNING || ! sc->sc_dmverrmsg++ || dmvdebug)printf
int dmvdebug = 0;

/* error reporting intervals */
#define DMV_RPNBFS	50
#define DMV_RPDSC	50
#define	DMV_RPTMO	10
#define DMV_RPDCK	10



struct mbuf *dmv_get();

extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern struct timeval time;
extern int boot_cpu_mask;


/*
 * DMV software status per interface.
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
#define DMV_ALLOC       01              /* unibus resources allocated */
#define DMV_BMAPPED     02              /* base table mapped */
#define	DMV_RESTART	04		/* software restart in progress */
#define	DMV_ACTIVE	08		/* device active */


/*
 * Driver information for auto-configuration stuff.
 */
int     dmvprobe(), dmvattach(), dmvinit(), dmvioctl();
int     dmvoutput(), dmvreset();
u_short dmvstd[] = { 0 };
struct  uba_driver dmvdriver =
        { dmvprobe, 0, dmvattach, 0, dmvstd, "dmv", dmvinfo };

/*
 * dmv software packet encapsulation.  This allows the dmv
 * link to be multiplexed among several protocols.
 * The first eight bytes of the dmv header are garbage; they
 * exist for historical reasons only since this structure is
 * the same one used for the DMC driver.
 * The next two bytes encapsulate packet type.
 */
struct dmv_header {
	char	dmv_buf[8];	/* space for uba on vax */
	short	dmv_type;	/* encapsulate packet type */
};

/* queue manipulation macros */
#define	QUEUE_AT_HEAD(qp, head, tail) \
	(qp)->qp_next = (head); \
	(head) = (qp); \
	if((tail) == (struct dmv_command *) 0) \
		(tail) = (head) 

#define QUEUE_AT_TAIL(qp, head, tail) \
	if((tail)) \
		(tail)->qp_next = (qp); \
	else \
		(head) = (qp); \
	(qp)->qp_next = (struct dmv_command *) 0; \
	(tail) = (qp)

#define DEQUEUE(head, tail) \
	(head) = (head)->qp_next;\
	if((head) == (struct dmv_command *) 0)\
		(tail) = (head)


dmvprobe(reg)
        caddr_t reg;
{
        register struct dmvdevice *addr = (struct dmvdevice *)reg;
        register int i;

#ifdef lint
        dmvrint(0); dmvxint(0);
#endif

        addr->bsel1 = DMV_MCLR;
        for (i = 100000; i && (addr->bsel1 & DMV_RUN) == 0; i--)
                ;
        if ((addr->bsel1 & DMV_RUN) == 0) {
		printf("dmvprobe: can't start device\n" );
                return (0);
	}

        if ( (addr->sel4 & DMV_LBSEL_MASK)  != 033 || (addr->sel6 & DMV_LBSEL_MASK) != 0305 ) {
		printf("dmvprobe: device failed diagnostics, octal failure code = %o\n", (addr->sel6 & DMV_LBSEL_MASK) );
                return (0);
	}

        addr->bsel0 = DMV_RQI|DMV_IEI;
        DELAY(1000000);
        addr->bsel1 = DMV_MCLR;
        for (i = 100000; i && (addr->bsel1 & DMV_RUN) == 0; i--)
                ;

        return (sizeof(struct dmvdevice));
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
dmvattach(ui)
        register struct uba_device *ui;
{
        register struct dmv_softc *sc = &dmv_softc[ui->ui_unit];
	struct dmvdevice *addr = (struct dmvdevice *)dmvinfo[ui->ui_unit]->ui_addr;
	int ncl;

        sc->sc_if.if_unit = ui->ui_unit;
        sc->sc_if.if_name = "dmv";
        sc->sc_if.if_mtu = DMVMTU;
        sc->sc_if.if_init = dmvinit;
        sc->sc_if.if_output = dmvoutput;
        sc->sc_if.if_ioctl = dmvioctl;
        sc->sc_if.if_reset = dmvreset;
        sc->sc_if.if_flags = IFF_POINTOPOINT | IFF_DYNPROTO;
	sc->sc_if.d_affinity = boot_cpu_mask;
        sc->sc_ifuba.ifu_flags = UBA_CANTWAIT;

	bzero(&sc->sc_dmvcs, sizeof(sc->sc_dmvcs));
	bzero(&sc->sc_errctrs, sizeof(sc->sc_errctrs));
	sc->sc_dmvcs.if_family = AF_UNSPEC;
	sc->sc_dmvcs.if_next_family = AF_UNSPEC;
	sc->sc_dmvcs.if_mode = ui->ui_flags;

	if ((sc->sc_bufres.nxmt = ((NXMT_MASK & ui->ui_flags) >> NXMT_SHIFT)) > NXMT_MIN ) {
		if (sc->sc_bufres.nxmt > NXMT_MAX) 
			sc->sc_bufres.nxmt = NXMT_MAX;
	} else
		sc->sc_bufres.nxmt = NXMT_MIN;
	sc->sc_bufres.nrcv = NRCV;
	sc->sc_bufres.ntot = sc->sc_bufres.nrcv + sc->sc_bufres.nxmt;
	sc->sc_bufres.ncmds = sc->sc_bufres.ntot + 4;

	/*
	 * Allocate page size buffers now. If we wait until the network
	 * is setup they have already fragmented. By doing it here in
	 * conjunction with always coping on uVAX-I processors we obtain
	 * physically contigous buffers for dma transfers.
	 */
	ncl = clrnd((int)btoc(DMVMTU) + CLSIZE) / CLSIZE;
	sc->sc_bufres.buffers = m_clalloc(sc->sc_bufres.ntot * ncl, MPG_SPACE);

        if_attach(&sc->sc_if);
	if (dmvtimer == 0) {
		dmvtimer = 1;
		timeout(dmvwatch, (caddr_t) 0, hz);
	}
}

/*
 * Reset of interface after UNIBUS reset.
 * If interface is on specified UBA, reset it's state.
 */
dmvreset(unit, uban)
        int unit, uban;
{
        register struct uba_device *ui;
        register struct dmv_softc *sc = &dmv_softc[unit];

        if (unit >= nNDMV || (ui = dmvinfo[unit]) == 0 || ui->ui_alive == 0 ||
            ui->ui_ubanum != uban)
                return;

        printf("resetting dmv%d", unit);
        sc->sc_flag = 0;
        dmvinit(unit);
}

/*
 * Initialization of interface; reinitialize UNIBUS usage.
 */
dmvinit(unit)
        int unit;
{
        register struct dmv_softc *sc = &dmv_softc[unit];
        register struct uba_device *ui = dmvinfo[unit];
        register struct dmvdevice *addr;
        register struct ifnet *ifp = &sc->sc_if;
        register struct ifrw *ifrw;
	register struct ifxmt *ifxp;
        register struct dmvbufs *rp;
	register struct dmv_command *qp;
	register struct ifaddr *ifa;
	struct dmv_command cmd;
        int base;
	int s;
	u_short dev_strt = 0;

        addr = (struct dmvdevice *)ui->ui_addr;

	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if ((ifa->ifa_addr.sa_family && ifa->ifa_dstaddr.sa_family) || ifa->ifa_addr.sa_family == AF_OSI)
			break;

	if (ifa == (struct ifaddr *) 0)
		return;

        if((addr->bsel1&DMV_RUN) == 0) {
                printf("dmvinit: DMV%d not running\n", unit);
                ifp->if_flags &= ~(IFF_RUNNING|IFF_UP);
                return;
        }


        /* map base table */
        if ((sc->sc_flag & DMV_BMAPPED) == 0) {
                sc->sc_ubinfo = uballoc(ui->ui_ubanum,
                        (caddr_t)&dmv_base[unit],
                        sizeof (struct dmv_base), 0);
                sc->sc_flag |= DMV_BMAPPED;
        }
        /* initialize UNIBUS resources */
        sc->sc_iused = sc->sc_oused = 0;
        if ((sc->sc_flag & DMV_ALLOC) == 0) {
                if (dmv_ubainit(&sc->sc_ifuba, ui->ui_ubanum,
		    sizeof(struct dmv_header), (int)btoc(DMVMTU), &sc->sc_bufres) == 0) {
                        printf("dmv%d: can't initialize\n", unit);
                        ifp->if_flags &= ~IFF_UP;
                        return;
                }
                sc->sc_flag |= DMV_ALLOC;
        }

        /* initialize buffer pool */
        /* recieves */
        ifrw= &sc->sc_ifuba.ifu_r[0];
        for(rp = &sc->sc_rbufs[0]; rp < &sc->sc_rbufs[sc->sc_bufres.nrcv]; rp++) {
                rp->ubinfo = ifrw->ifrw_info & 0x3ffff;
		rp->cc = DMVMTU + sizeof (struct dmv_header);
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
		(struct dmv_command *)0;
	/* set up free command buffer list */
	for (qp = &sc->sc_cmdbuf[0]; qp < &sc->sc_cmdbuf[sc->sc_bufres.ncmds]; qp++) {
		QUEUE_AT_HEAD(qp, sc->sc_qfreeh, sc->sc_qfreet);
	}

	/* if first time through, zero out counters */
	if ( ! (ifp->if_flags & IFF_RUNNING) ) {
		sc->sc_ztime = time.tv_sec;
		bzero(&sc->sc_errctrs, sizeof(sc->sc_errctrs));
	}

	/* set up internal loopback if requested */
	if ( ifp->if_flags & IFF_LOOPBACK ) {
		int i;
		addr->bsel1 |= (DMV_ILOOP | DMV_MCLR);
		for ( i = 10000; i && ! (addr->bsel2 & DMV_RDYO); i-- ) ;
		if ( ! (addr->bsel2 & DMV_RDYO) )
			printf("dmvinit: can't place dmv%d into internal loopback\n", unit);
		else
			addr->bsel2 = DMV_MAINT_ILOOP;
	}


        /* enable operation done interrupts */
        sc->sc_flag &= ~DMV_ACTIVE;
        while ((addr->bsel0 & DMV_IEO) == 0)
                addr->bsel0 |= DMV_IEO;

        s = splimp();

	/* Issue mode command, establish trib and start device */
        switch ( (ifp->if_flags & IFF_LOOPBACK) ? DMV_MFLAGS_FDPLX_DMC : (sc->sc_dmvcs.if_mode & DMV_MFLAGS_MASK) )
	{
                /* use DDCMP point to point full duplex mode */
		case DMV_MFLAGS_FDPLX_DMC:
			dev_strt = DMV_CNTL_RQSTRT;
			if ( (ifp->if_flags & IFF_RUNNING) && ! (sc->sc_flag & DMV_RESTART) && ! (ifp->if_flags & IFF_LOOPBACK) ) 
				break;
			printd("dmv%d: starting device in full duplex, dmc compatible mode\n", unit);
			dmv_issue_mode(sc, DMV_MODE_FDPLX_DMC);
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_ESTRIB );
			break;

		case DMV_MFLAGS_FDPLX:
			dev_strt = DMV_CNTL_RQSTRT;
			if ( (ifp->if_flags & IFF_RUNNING) && ! (sc->sc_flag & DMV_RESTART) ) 
				break;
			printf("dmv%d: starting device in full duplex mode\n", unit);
			dmv_issue_mode(sc, DMV_MODE_FDPLX);
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_ESTRIB );
			break;

                /* use DDCMP half duplex as primary station */
		case DMV_MFLAGS_HDPLX_DMC:
			dev_strt = DMV_CNTL_RQSTRT;
			if ( (ifp->if_flags & IFF_RUNNING) && ! (sc->sc_flag & DMV_RESTART) ) 
				break;
			printd("dmv%d: starting device in half duplex, dmc compatible mode\n", unit);
			dmv_issue_mode(sc, DMV_MODE_HDPLX_DMC);
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_ESTRIB );
			break;

		case DMV_MFLAGS_HDPLX:
			dev_strt = DMV_CNTL_RQSTRT;
			if ( (ifp->if_flags & IFF_RUNNING) && ! (sc->sc_flag & DMV_RESTART) ) 
				break;
			printd("dmv%d: starting device in half duplex mode\n", unit);
			dmv_issue_mode(sc, DMV_MODE_HDPLX);
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_ESTRIB );
			break;

                /* use MAINTENANCE full duplex mode */
		case DMV_MFLAGS_MAINT_FDPLX:
			dev_strt = DMV_CNTL_RQMAINT;
			if ( (ifp->if_flags & IFF_RUNNING) && ! (sc->sc_flag & DMV_RESTART) ) 
				break;
			printf("dmv%d: starting device in full duplex maintenance mode\n", unit);
			dmv_issue_mode(sc, DMV_MODE_FDPLX_DMC);
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_ESTRIB );
			break;

                /* use MAINTENANCE half duplex mode */
		case DMV_MFLAGS_MAINT_HDPLX:
		default:
			dev_strt = DMV_CNTL_RQMAINT;
			if ( (ifp->if_flags & IFF_RUNNING) && ! (sc->sc_flag & DMV_RESTART) ) 
				break;
			printd("dmv%d: starting device in half duplex maintenance mode\n", unit);
			dmv_issue_mode(sc, DMV_MODE_HDPLX_DMC);
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_ESTRIB );
			break;
	}

	/* override device default values for select and babble timers */
	dmv_issue_ctl( sc, DMV_TRIB_POINT, DMV_TIMER_SEL, (DMV_CNTL_WRITE | DMV_KEY_SELECT) );
	dmv_issue_ctl( sc, DMV_TRIB_POINT, DMV_TIMER_BAB, (DMV_CNTL_WRITE | DMV_KEY_BABBLE) );

	/* tell device to start protocol */
	sc->sc_dmvcs.if_dstate = IFS_STARTING;
	dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, dev_strt );

        /* queue first NRCV buffers for DMV to fill */
        for(rp = &sc->sc_rbufs[0]; rp < &sc->sc_rbufs[sc->sc_bufres.nrcv]; rp++) {
		printd("dmv%d: assigning receive buffer\n", unit);
                rp->flags |= DBUF_DMVS;
                dmv_issue_buf(sc, DMV_CMD_RXBUF, 1, rp->ubinfo, rp->cc);
                sc->sc_iused++;
        }

	/*
	 * If in Maintenance, inform owner that device is up.
	 */
	if ( dev_strt == DMV_CNTL_RQMAINT) {
		sc->sc_if.if_flags |= IFF_UP;
		sc->sc_dmvcs.if_dstate = IFS_RUNNING;
		if ( sc->sc_if.if_addr.sa_family != AF_UNSPEC ) {
			struct protosw *pr;
			if ((pr=iffamily_to_proto(sc->sc_if.if_addr.sa_family)) && pr->pr_ifstate ) 
				(*pr->pr_ifstate)(&sc->sc_if, sc->sc_dmvcs.if_dstate, &sc->sc_dmvcs); 
		}
	}
	splx(s);

        ifp->if_flags |= IFF_RUNNING;
	if ( sc->sc_dmvcs.if_mode == IFS_MOP )
		ifp->if_flags |= IFF_MOP;
	else
		ifp->if_flags &= ~IFF_MOP;

}

/*
 * Start output on interface.  Get another datagram
 * to send from the interface queue and map it to
 * the interface before starting output.
 *
 * Must be called at pri 5.
 */
dmvstart(dev)
        dev_t dev;
{
        int unit = minor(dev);
        register struct dmv_softc *sc = &dmv_softc[unit];
        struct mbuf *m;
        register struct dmvbufs *rp;
	register int n;

        /*
         * Dequeue up to NXMT requests and map them to the UNIBUS.
         * If no more requests, or no dmv buffers available, just return.
         */
	n = 0;
        for(rp = &sc->sc_xbufs[0]; rp < &sc->sc_xbufs[sc->sc_bufres.nxmt]; rp++ ) {
                /* find an available buffer */
                if ((rp->flags & DBUF_DMVS) == 0) {
                        IF_DEQUEUE(&sc->sc_if.if_snd, m);
                        if (m == 0)
                                return;
                        /* mark it dmvs */
                        rp->flags |= DBUF_DMVS;
                        /*
                         * Have request mapped to UNIBUS for transmission
                         * and start the output.
                         */
                        rp->cc = dmvput(&sc->sc_ifuba, n, m);
			rp->cc &= DMV_CCOUNT;
                        sc->sc_oused++;
                	dmv_issue_buf(sc, DMV_CMD_TXBUF, 1, rp->ubinfo, rp->cc);
                }
		n++;
        }
}

/*
 * Routine to issue mode command.
 */
dmv_issue_mode( sc, mode )
register struct dmv_softc *sc;
register u_short mode;
{
	struct dmv_command cmd;

	bzero(&cmd, sizeof(cmd));
	cmd.qp_cmd = DMV_CMD_MODE;
	cmd.qp_data = 0;
	cmd.qp_trib = DMV_TRIB_POINT;
	cmd.qp_mode = mode;
	dmvload(sc, &cmd);
}
/*
 * Routine to issue control command.
 */
dmv_issue_ctl( sc, trib, data, ctl )
register struct dmv_softc *sc;
register u_char trib;
register u_short data;
register u_short ctl;
{
	struct dmv_command cmd;

	bzero(&cmd, sizeof(cmd));
	cmd.qp_cmd = DMV_CMD_CNTL;
	cmd.qp_trib = trib;
	cmd.qp_data = data;
	cmd.qp_ctl = ctl;
	dmvload(sc, &cmd);
}

/*
 * Routine to issue Tx/Rx buffer command.
 */
dmv_issue_buf( sc, cmd_type, trib, addr, cc )
register struct dmv_softc *sc;
register u_char cmd_type, trib;
register u_int addr;
register u_short cc;
{
	struct dmv_command cmd;

	bzero(&cmd, sizeof(cmd));
	cmd.qp_cmd = cmd_type | DMV_22BIT;
	cmd.qp_trib = trib;
	cmd.qp_lowaddr = (u_short) addr;
	cmd.qp_hiaddr = (u_short) (addr >> 16);
	cmd.qp_cc = cc;
	dmvload(sc, &cmd);
}


/*
 * Utility routine to load the DMV device registers.
 */
dmvload(sc, cmd)
        register struct dmv_softc *sc;
	register struct dmv_command *cmd;
{
        register struct dmvdevice *addr;
        register int unit, sps;
	register struct dmv_command *qp;

        unit = sc->sc_if.if_unit;
        addr = (struct dmvdevice *)dmvinfo[unit]->ui_addr;
        sps = splimp();

	/* grab a command buffer from the free list */
	if((qp = sc->sc_qfreeh) == (struct dmv_command *)0) {
		printf("dmv%d: no free command buffer\n", unit);
        	splx(sps);
		return;
	}
	DEQUEUE(sc->sc_qfreeh, sc->sc_qfreet);

	/* fill in requested info and pass to device */
	*qp = *cmd;
	if (sc->sc_qactive){	/* command in progress */
		if(((cmd->qp_cmd & DMV_CMD_MASK) == DMV_CMD_RXBUF) && ! (sc->sc_dmvcs.if_dstate == IFS_STARTING) ) {
			QUEUE_AT_HEAD(qp, sc->sc_qhead, sc->sc_qtail);
		} else {
			QUEUE_AT_TAIL(qp, sc->sc_qhead, sc->sc_qtail);
		}
	} else {	/* command port free */
		sc->sc_qactive = qp;
                addr->bsel0 |= DMV_IEI|DMV_RQI;
	}
        splx(sps);
}

/*
 * DMV interface receiver interrupt.
 * Ready to accept another command,
 * pull one off the command queue.
 */
dmvrint(unit)
        int unit;
{
        register struct dmv_softc *sc;
        register struct dmvdevice *addr;
        register struct dmv_command *qp;

        addr = (struct dmvdevice *)dmvinfo[unit]->ui_addr;
        sc = &dmv_softc[unit];

	if ((qp = sc->sc_qactive) == (struct dmv_command *) 0) {
		printd("dmvrint: no command\n");
        	addr->bsel2 |= DMV_CMD_CNTL;
		addr->sel6 = DMV_CNTL_NOP;
		addr->bsel0 &= ~(DMV_RQI | DMV_IEI);
		addr->bsel2 &= (DMV_CMD_CNTL | DMV_RDYO);
		return;
	}

	addr->bsel3 = qp->qp_trib;
        addr->bsel2 |= (qp->qp_cmd & 0x0f);
	switch ( qp->qp_cmd & 0x7 )
	{
		case DMV_CMD_RXBUF:
		case DMV_CMD_TXBUF:
                	addr->sel4 = qp->qp_lowaddr;
                	addr->sel6 = qp->qp_hiaddr & HIADDR_MASK;
                	addr->sel10 = qp->qp_cc & CC_MASK;
			break;

		case DMV_CMD_CNTL:
                	addr->sel4 = qp->qp_data;
                	addr->sel6 = qp->qp_ctl & CTL_MASK;
			break;

		case DMV_CMD_MODE:
                	addr->sel6 = qp->qp_mode & MODE_MASK;
			break;

		default:
			printd("dmvrint: bad command = %x\n", qp->qp_cmd);
			addr->sel6 = DMV_CNTL_NOP;
        		addr->bsel2 |= DMV_CMD_CNTL;
			break;
	}

	/* free command buffer */
	QUEUE_AT_HEAD(qp, sc->sc_qfreeh, sc->sc_qfreet);

	/* pre-fetch next command */
        if ( (sc->sc_qactive = sc->sc_qhead) == (struct dmv_command *)0 ) 
		addr->bsel0 &= ~(DMV_RQI | DMV_IEI);
	else
		DEQUEUE(sc->sc_qhead, sc->sc_qtail);

        addr->bsel2 &= ~DMV_RDYI;
}

/*
 * DMV interface transmitter interrupt.
 * A transfer may have completed, check for errors.
 * If it was a read, notify appropriate protocol.
 * If it was a write, pull the next one off the queue.
 */
dmvxint(unit)
        int unit;
{
        register struct dmv_softc *sc;
        register struct ifnet *ifp;
        struct uba_device *ui = dmvinfo[unit];
        struct dmvdevice *addr;
        struct mbuf *m;
        struct ifqueue *inq;
        int len, pkaddr;
        register struct ifrw *ifrw;
        register struct dmvbufs *rp;
	register struct ifxmt *ifxp;
	struct dmv_header *dh;
	struct dmv_command response;
	int off, resid;
	struct protosw *pr;

        addr = (struct dmvdevice *)ui->ui_addr;
        sc = &dmv_softc[unit];
        ifp = &sc->sc_if;

	if ( (response.qp_cmd = addr->bsel2) & DMV_22BIT )
		response.qp_cc = addr->sel10;
	response.qp_lowaddr = addr->sel4;
	response.qp_hiaddr = addr->sel6;
	response.qp_trib = addr->bsel3;

	/* release port */
	addr->bsel2 &= ~DMV_RDYO;

	/*
	 * analyze response
	 */
	switch (response.qp_cmd & DMV_CMD_MASK) {

		case DMV_RSP_RXBUFOK:
			/*
			 * A read has completed.  
			 * Pass packet to type specific
			 * higher-level input routine.
			 */
			pkaddr = (response.qp_hiaddr << 16) | response.qp_lowaddr;
			ifp->if_ipackets++;
			/*
			 * Accumulate statistics for DECnet
			 */
			if ( (sc->sc_errctrs.ctr_ddcmp.dst_bytercvd + (response.qp_cc & CC_MASK)) > sc->sc_errctrs.ctr_ddcmp.dst_bytercvd ) 
				sc->sc_errctrs.ctr_ddcmp.dst_bytercvd += (response.qp_cc & CC_MASK); 
			if (sc->sc_errctrs.ctr_ddcmp.dst_blockrcvd != 0xffffffff)
				sc->sc_errctrs.ctr_ddcmp.dst_blockrcvd++;

			/* find location in dmvuba struct */
			ifrw= &sc->sc_ifuba.ifu_r[0];
			for (rp = &sc->sc_rbufs[0]; rp < &sc->sc_rbufs[sc->sc_bufres.nrcv]; rp++) {
				if(rp->ubinfo == pkaddr)
					break;
				ifrw++;
			}
			if (rp >= &sc->sc_rbufs[sc->sc_bufres.nrcv])
				panic("dmv%d rcv", unit);
			if ((rp->flags & DBUF_DMVS) == 0)
				printf("dmv%d: done unalloc rbuf\n", unit);

			if ( sc->sc_dmvcs.if_nomuxhdr ) {
				len = (response.qp_cc & CC_MASK); 
			} else {
				len = (response.qp_cc & CC_MASK) - sizeof (struct dmv_header);
			}

			if (len < 0 || len > DMVMTU) {
				ifp->if_ierrors++;
				printd("dmv%d: bad rcv pkt addr 0x%x len 0x%x\n",
				    unit, pkaddr, len);
				goto setup;
			}
			/* 
			 * If using dmv header, then
			 * deal with trailer protocol: if type is trailer
			 * get true type from first 16-bit word past data.
			 * Remember that type was trailer by setting off.
			 */
			if ( ! sc->sc_dmvcs.if_nomuxhdr ) {
				dh = (struct dmv_header *)ifrw->ifrw_addr;
				dh->dmv_type = ntohs((u_short)dh->dmv_type);
#define	dmvdataaddr(dh, off, type)	((type)(((caddr_t)((dh)+1)+(off))))
				if (dh->dmv_type >= DMV_TRAILER &&
				    dh->dmv_type < DMV_TRAILER+DMV_NTRAILER) {
					off = (dh->dmv_type - DMV_TRAILER) * 512;
					if (off >= DMVMTU)
						goto setup;		/* sanity */
					dh->dmv_type = ntohs(*dmvdataaddr(dh, off, u_short *));
					resid = ntohs(*(dmvdataaddr(dh, off+2, u_short *)));
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
			 * packet has trailing header; dmv_get will then
			 * force this header information to be at the front,
			 * but we still have to drop the type and length
			 * which are at the front of any trailer data.
			 */
			m = dmv_get(&sc->sc_ifuba, ifrw, len, off, ! (sc->sc_dmvcs.if_nomuxhdr));
			if (m == 0)
				goto setup;
			if (off) {
				m->m_off += 2 * sizeof (u_short);
				m->m_len -= 2 * sizeof (u_short);
			}

			/*
			 * Find protocol to which packet is destined
			 */
			if ( ! sc->sc_dmvcs.if_nomuxhdr ) {
				/*
				 * Multiplexed - find protocol as a
				 * function of packet type
				 */
				switch (dh->dmv_type) {
	
#ifdef INET
				case DMV_IPTYPE:
					if (nINET) {
						inq = &ipintrq;
						smp_lock(&ipintrq.lk_ifqueue, LK_RETRY);
						schednetisr(NETISR_IP);
						break;
					} else {
						printf("dmv%d: unknown address type %d\n",
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
	                        		printf("dmv%d: unknown address type %d\n", unit,
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
	                        	printf("dmv%d: unknown address type %d\n", unit,
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
			if ( sc->sc_dmvcs.if_dstate != IFS_HALTING )
                		dmv_issue_buf(sc, DMV_CMD_RXBUF, 1, rp->ubinfo, rp->cc);
			else {
				rp->flags &= ~DBUF_DMVS;
				sc->sc_iused--;
			}
			break;

		case DMV_RSP_RXBUF:
			/*
			 * An Rx buffer has just been returned unused.  
			 */
			pkaddr = (response.qp_hiaddr << 16) | response.qp_lowaddr;
			/* find location in dmvuba struct */
			ifrw= &sc->sc_ifuba.ifu_r[0];
			for (rp = &sc->sc_rbufs[0]; rp < &sc->sc_rbufs[sc->sc_bufres.nrcv]; rp++) {
				if(rp->ubinfo == pkaddr)
					break;
				ifrw++;
			}
			if (rp >= &sc->sc_rbufs[sc->sc_bufres.nrcv])
				panic("dmv%d rcv", unit);
			if ((rp->flags & DBUF_DMVS) == 0)
				printf("dmv%d: done unalloc rbuf\n", unit);

			rp->flags &= ~DBUF_DMVS;
			sc->sc_iused--;
			break;

		case DMV_RSP_TXBUFOK:
			/*
			 * A write has completed, start another
			 * transfer if there is more data to send.
			 */
			ifp->if_opackets++;
			/*
			 * Accumulate statistics for DECnet
			 */
			if ( (sc->sc_errctrs.ctr_ddcmp.dst_bytesent + (response.qp_cc & CC_MASK)) > sc->sc_errctrs.ctr_ddcmp.dst_bytesent ) 
				sc->sc_errctrs.ctr_ddcmp.dst_bytesent += (response.qp_cc & CC_MASK); 
			if (sc->sc_errctrs.ctr_ddcmp.dst_blocksent != 0xffffffff)
				sc->sc_errctrs.ctr_ddcmp.dst_blocksent++;

		case DMV_RSP_TXBUF1:
		case DMV_RSP_TXBUF2:
			/* find associated dmvbuf structure */
			pkaddr = (response.qp_hiaddr << 16) | response.qp_lowaddr;
			ifxp = &sc->sc_ifuba.ifu_w[0];
			for (rp = &sc->sc_xbufs[0]; rp < &sc->sc_xbufs[sc->sc_bufres.nxmt]; rp++) {
				if(rp->ubinfo == pkaddr)
					break;
				ifxp++;
			}
			if (rp >= &sc->sc_xbufs[sc->sc_bufres.nxmt]) {
				printf("dmv%d: bad packet address 0x%x\n",
				    unit, pkaddr);
				break;
			}
			if ((rp->flags & DBUF_DMVS) == 0)
				printf("dmv%d: unallocated packet 0x%x\n",
				    unit, pkaddr);
			/* mark buffer free */
			if (ifxp->x_xtofree) {
				(void)m_freem(ifxp->x_xtofree);
				ifxp->x_xtofree = 0;
			}
			rp->flags &= ~DBUF_DMVS;
			sc->sc_oused--;
			sc->sc_nticks = 0;
			sc->sc_flag |= DMV_ACTIVE;
			break;

		case DMV_RSP_CNTL:
			dmv_rsp_cntl(sc, &response);
			break;

		case DMV_RSP_INFO:
			if ( response.qp_ctl & (DMV_INFO_READ | DMV_INFO_READZ) ) {
				dmv_format_tgss(sc, &response);
			} else if ( (response.qp_ctl & KEY_MASK) == DMV_INFO_BUFRET ) {
				if ( sc->sc_if.if_addr.sa_family != AF_UNSPEC ) {
					struct protosw *pr;
					if ((pr=iffamily_to_proto(sc->sc_if.if_addr.sa_family)) && pr->pr_ifstate ) 
						(*pr->pr_ifstate)(&sc->sc_if, (sc->sc_dmvcs.if_dstate == IFS_ENTEREDMOP) ? IFS_ENTEREDMOP : IFS_HALTED, &sc->sc_dmvcs); 
					dmvstate(&sc->sc_if, sc, ui);
				} else
		    			sc->sc_dmvcs.if_dstate = IFS_STARTING;
					
				if ( sc->sc_dmvcs.if_dstate == IFS_STARTING ) 
					dmvinit(unit);
			} else if ( (response.qp_ctl & KEY_MASK) == DMV_INFO_MODEM ) {
				printf("dmv%d: information response; modem control = %x\n", unit, response.qp_data);
			} else {
				printd("dmv%d: unsolicited information response; ctl = %x, data = %x\n", unit, response.qp_ctl, response.qp_data);
			}
			break;

		default:
			printd("dmv%d: bad control %o\n", unit, response.qp_cmd);
			break;
	}
	dmvstart(unit);
        return;
}

/*
 * DMV routine to process control responses.
 */
dmv_rsp_cntl(sc, rsp)
register struct dmv_softc *sc;
register struct dmv_command *rsp;
{
	register struct protosw *pr;
        register struct uba_device *ui = dmvinfo[sc->sc_if.if_unit];

	switch ( rsp->qp_ctl & EVENT_MASK )
	{
		case DMV_EVT_DISCONN:
		case DMV_EVT_CARLOSS:
			printdmverr("dmv%d: modem disconnect\n", sc->sc_if.if_unit);
			sc->sc_disc++;
			sc->sc_if.if_flags &= ~IFF_UP;
			sc->sc_dmvcs.if_dstate = IFS_HALTED;
			if ( sc->sc_if.if_addr.sa_family != AF_UNSPEC ) {
				if ((pr=iffamily_to_proto(sc->sc_if.if_addr.sa_family)) && pr->pr_ifstate ) 
					(*pr->pr_ifstate)(&sc->sc_if, sc->sc_dmvcs.if_dstate, &sc->sc_dmvcs); 
			}
			if ((sc->sc_flag & (DMV_ALLOC | DMV_BMAPPED)) == (DMV_ALLOC | DMV_BMAPPED) && ( sc->sc_flag & DMV_RESTART) == 0 ) {
				sc->sc_flag |= DMV_RESTART;
				dmvrestart(sc->sc_if.if_unit);
			}
			break;

		case DMV_PRC_BUF:
			printdmverr("dmv%d: buffer too small\n", sc->sc_if.if_unit);
			sc->sc_if.if_ierrors++;
			sc->sc_dmvcs.if_dstate = IFS_HALTING;
			sc->sc_if.if_flags &= ~IFF_UP;
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_RQHALT );
			break;

		case DMV_ERR_RXTHRES:
			printdmverr("dmv%d: receive threshold reported\n", sc->sc_if.if_unit);
			sc->sc_if.if_oerrors++;
			sc->sc_dmvcs.if_dstate = IFS_HALTING;
			sc->sc_if.if_flags &= ~IFF_UP;
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_RQHALT );
			break;

		case DMV_ERR_TXTHRES:
			printdmverr("dmv%d: transmit threshold reached\n", sc->sc_if.if_unit);
			goto dmv_rsp_cntl_txtimeo;

		case DMV_ERR_SETHRES:
			printdmverr("dmv%d: select threshold reached\n", sc->sc_if.if_unit);
			goto dmv_rsp_cntl_txtimeo;

		case DMV_ERR_BABBLE:
			printdmverr("dmv%d: babbling tributary reported\n", sc->sc_if.if_unit);
			goto dmv_rsp_cntl_txtimeo;

		case DMV_ERR_STREAM:
			printdmverr("dmv%d: streaming tributary reported\n", sc->sc_if.if_unit);

		dmv_rsp_cntl_txtimeo:
			sc->sc_timeo++;
			sc->sc_dmvcs.if_dstate = IFS_HALTING;
			sc->sc_if.if_flags &= ~IFF_UP;
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_RQHALT );
			break;
	
		case DMV_EVT_RXMNT_RU:
			printf("dmv%d: MOP mode entered while DDCMP was running\n", sc->sc_if.if_unit);
			sc->sc_dmvcs.if_dstate = IFS_ENTEREDMOP;
			sc->sc_if.if_flags &= ~IFF_UP;
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_RQHALT );
			break;

		case DMV_EVT_RXMNT_HA:
			printf("dmv%d: MOP mode entered while device was halted\n", sc->sc_if.if_unit);
			sc->sc_dmvcs.if_dstate = IFS_ENTEREDMOP;
			sc->sc_dmvcs.if_ustate = IFS_USROFF;
			if ( sc->sc_if.if_addr.sa_family != AF_UNSPEC ) {
				if ((pr=iffamily_to_proto(sc->sc_if.if_addr.sa_family)) && pr->pr_ifstate ) 
					(*pr->pr_ifstate)(&sc->sc_if, sc->sc_dmvcs.if_dstate, &sc->sc_dmvcs); 
			}
			sc->sc_dmvcs.if_dstate = IFS_HALTING;
			sc->sc_if.if_flags &= ~IFF_UP;
			dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_RQHALT );
			break;

		case DMV_EVT_RXSTRT_MA:
			break;

		case DMV_EVT_RXSTRT_RU:
			printf("dmv%d: received start while DDCMP running\n", sc->sc_if.if_unit);
			sc->sc_dmvcs.if_dstate = IFS_HALTING;
			break;

		case DMV_EVT_DDCMPRUN:
			/*
			 * Inform owner that device is up.
			 */
			printd("dmv%d: device is up\n", sc->sc_if.if_unit);
			sc->sc_if.if_flags |= IFF_UP;
			sc->sc_dmvcs.if_dstate = IFS_RUNNING;
			sc->sc_dmverrmsg = 0;
			if ( sc->sc_if.if_addr.sa_family != AF_UNSPEC ) {
				if ((pr=iffamily_to_proto(sc->sc_if.if_addr.sa_family)) && pr->pr_ifstate ) 
					(*pr->pr_ifstate)(&sc->sc_if, sc->sc_dmvcs.if_dstate, &sc->sc_dmvcs); 
			}
			break;

		case DMV_PRC_NXM:
			printf("dmv%d: non existent memory reported\n", sc->sc_if.if_unit);
			goto dmv_rsp_cntl_harderr;

		case DMV_EVT_QOVF:
			printf("dmv%d: device queue overflow reported\n", sc->sc_if.if_unit);
			goto dmv_rsp_cntl_harderr;

		case DMV_PRC_INVMODE:
			printf("dmv%d: device mode being overridden with an invalid value\n", sc->sc_if.if_unit);
			break;

		default:
			printd("dmv%d: procedure error (0%o) reported\n", sc->sc_if.if_unit, rsp->qp_ctl);
		dmv_rsp_cntl_harderr:
			if ((sc->sc_flag & (DMV_ALLOC | DMV_BMAPPED)) == (DMV_ALLOC | DMV_BMAPPED) && ( sc->sc_flag & DMV_RESTART) == 0 ) {
				sc->sc_flag |= DMV_RESTART;
				sc->sc_if.if_flags &= ~IFF_UP;
				sc->sc_dmvcs.if_dstate = IFS_HALTING;
				dmvrestart(sc->sc_if.if_unit);
			}
			break;
	}
	return;
}

/*
 * DMV counter formatter routine.
 * This routine stores the counter information in the softc structure.
 */
dmv_format_tgss(sc, rsp)
register struct dmv_softc *sc;
register struct dmv_command *rsp;
{

	switch ( rsp->qp_ctl & KEY_MASK )
	{

		case DMV_KEY_SELINT:
			sc->sc_errctrs.ctr_ddcmp.dst_select = (rsp->qp_data > 0377 ) ? 0377 : rsp->qp_cntlo;
			sc->sc_ctrmask |= DMV_MSK_SELINT;
			break;

		case DMV_KEY_DEOUT:
			sc->sc_errctrs.ctr_ddcmp.dst_outbound = rsp->qp_cntlo;
			sc->sc_errctrs.ctr_ddcmp.dst_outbound_bm = rsp->qp_cnthi & 0x7;
			sc->sc_ctrmask |= DMV_MSK_DEOUT;
			break;

		case DMV_KEY_DEIN:
			sc->sc_errctrs.ctr_ddcmp.dst_inbound = rsp->qp_cntlo;
			sc->sc_errctrs.ctr_ddcmp.dst_inbound_bm = rsp->qp_cnthi & 0x7;
			sc->sc_ctrmask |= DMV_MSK_DEIN;
			break;

		case DMV_KEY_LBERR:
			sc->sc_errctrs.ctr_ddcmp.dst_localbuf = rsp->qp_cntlo;
			sc->sc_errctrs.ctr_ddcmp.dst_localbuf_bm = rsp->qp_cnthi & 0x3;
			sc->sc_ctrmask |= DMV_MSK_LBERR;
			break;

		case DMV_KEY_RBERR:
		/* also defined as DMV_KEY_RSERR */
			if ( rsp->qp_trib ) {
				sc->sc_errctrs.ctr_ddcmp.dst_remotebuf = rsp->qp_cntlo;
				sc->sc_errctrs.ctr_ddcmp.dst_remotebuf_bm = rsp->qp_cnthi & 0x3;
				sc->sc_ctrmask |= DMV_MSK_RBERR;
			} else {
				sc->sc_errctrs.ctr_ddcmp.dst_remotesta = rsp->qp_cntlo;
				sc->sc_errctrs.ctr_ddcmp.dst_remotesta_bm = rsp->qp_cnthi & 0xf;
				sc->sc_ctrmask |= DMV_MSK_RSERR;
			}
			break;

		case DMV_KEY_SELTO:
		/* also defined as DMV_KEY_LSERR */
			if ( rsp->qp_trib ) {
				sc->sc_errctrs.ctr_ddcmp.dst_selecttmo = rsp->qp_cntlo;
				sc->sc_errctrs.ctr_ddcmp.dst_selecttmo_bm = rsp->qp_cnthi & 0x3;
				sc->sc_ctrmask |= DMV_MSK_SELTO;
			} else {
				sc->sc_errctrs.ctr_ddcmp.dst_localsta = rsp->qp_cntlo;
				sc->sc_errctrs.ctr_ddcmp.dst_localsta_bm = rsp->qp_cnthi & 0xf;
				sc->sc_ctrmask |= DMV_MSK_LSERR;
			}
			break;

		case DMV_KEY_REPTO:
			sc->sc_errctrs.ctr_ddcmp.dst_localtmo = rsp->qp_cntlo;
			sc->sc_errctrs.ctr_ddcmp.dst_remotetmo = rsp->qp_cnthi;
			sc->sc_ctrmask |= DMV_MSK_REPTO;
			break;

		default:
			printd("dmv%d: unexpected information response\n", sc->sc_if.if_unit);
			break;

	}
	if ( sc->sc_ctrmask == DMV_MSK_CMPLT )
	{
		struct ifnet *ifp = &sc->sc_if;
		int unit = ifp->if_unit;
		struct protosw *pr;

		if (!dmv_instack[unit])
			wakeup((caddr_t) &sc->sc_ctrmask);
		else
		{
			/*
			 * get counter command was from timer routine
			 * call ifiotctl to pass the counter info
			 */
			if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && 
			     pr->pr_ifioctl)
				(*pr->pr_ifioctl)(ifp, SIOCRDZCTRS, &sc->sc_errctrs.ctr_ddcmp);
			dmv_instack[unit] = 0;
		 	sc->sc_ctrmask = 0;
			sc->sc_ztime = time.tv_sec;
			sc->sc_errctrs.ctr_ddcmp.dst_bytercvd = 0;
			sc->sc_errctrs.ctr_ddcmp.dst_bytesent = 0;
			sc->sc_errctrs.ctr_ddcmp.dst_blockrcvd = 0;
			sc->sc_errctrs.ctr_ddcmp.dst_blocksent = 0;
		}
	}
	return;
}

/*
 * DMV state routine
 * This routine either halts the DMV or sets it up to operate
 * in a particular mode.
 */
dmvstate(ifp, sc, ui)
        register struct ifnet *ifp;
        register struct dmv_softc *sc;
        register struct uba_device *ui;
{
	ifp->if_addr.sa_family = sc->sc_dmvcs.if_family; 

	if ( sc->sc_dmvcs.if_dstate == IFS_ENTEREDMOP ) {
		sc->sc_dmvcs.if_dstate = IFS_HALTING;
		sc->sc_dmvcs.if_ustate = IFS_USRON;
	} else if ( sc->sc_dmvcs.if_ustate == IFS_USROFF ) {
		sc->sc_dmvcs.if_mode = ui->ui_flags;
		sc->sc_dmvcs.if_dstate = IFS_HALTED;
	} else {
		    sc->sc_dmvcs.if_dstate = IFS_STARTING;
	}
}

/*
 * DMV output routine.
 * Encapsulate a packet of type family for the dmv.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 */
dmvoutput(ifp, m0, dst)
        register struct ifnet *ifp;
        register struct mbuf *m0;
        struct sockaddr *dst;
{
	register struct dmv_softc *sc = &dmv_softc[ifp->if_unit];
        int type, error, s;
        register struct mbuf *m = m0;
	register struct dmv_header *dh;
	register int off;
	struct protosw *pr;
        struct ifqueue *inq;

	/*
	 * check to make sure device state is currently on
	 */
	if ( (sc->sc_dmvcs.if_dstate == IFS_HALTING) || (sc->sc_dmvcs.if_dstate == IFS_HALTED) )
	{
		error = ENETDOWN;
		goto bad;
	}

	/*
	 * check for device ownership if if_nomuxhdr is asserted!
	 */
	if ( (sc->sc_dmvcs.if_nomuxhdr) && (ifp->if_addr.sa_family != dst->sa_family) )
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
			type = DMV_TRAILER + (off>>9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = htons((u_short)DMV_IPTYPE);
			*(mtod(m, u_short *) + 1) = htons((u_short)m->m_len);
			goto gottrailertype;
		}
		type = DMV_IPTYPE;
		off = 0;
		goto gottype;
#endif

	case AF_UNSPEC:
		dh = (struct dmv_header *)dst->sa_data;
		type = dh->dmv_type;
		goto gottype;

	default:
		if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && pr->pr_ifoutput) 
			(*pr->pr_ifoutput)(ifp, m, dst, &type, NULL);
		else {
			printf("dmv%d: can't handle af%d\n", ifp->if_unit,
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
	if ( ! sc->sc_dmvcs.if_nomuxhdr ) {
		if (m->m_off > MMAXOFF ||
		    MMINOFF + sizeof(struct dmv_header) > m->m_off) {
			m = m_get(M_DONTWAIT, MT_DATA);
			if (m == 0) {
				error = ENOBUFS;
				goto bad;
			}
			m->m_next = m0;
			m->m_off = MMINOFF;
			m->m_len = sizeof (struct dmv_header);
		} else {
			m->m_off -= sizeof (struct dmv_header);
			m->m_len += sizeof (struct dmv_header);
		}
		dh = mtod(m, struct dmv_header *);
		dh->dmv_type = htons((u_short)type);
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
        dmvstart(ifp->if_unit);
        splx(s);
        return(0);

bad:
	m_freem(m0);
	return(error);
}


/*
 * Process an ioctl request.
 */
dmvioctl(ifp, cmd, data)
        register struct ifnet *ifp;
        int cmd;
        caddr_t data;
{
        int s = splimp(), error = 0;
	int arg;
	register struct dmv_softc *sc = &dmv_softc[ifp->if_unit];
	register struct uba_device *ui = dmvinfo[ifp->if_unit];

        switch (cmd) {

	case SIOCSTATE:
		/*
		 * read and/or set current ownership and state of device.
		 */
		{
			register struct ifstate *dmvstate = (struct ifstate *) data;
			register struct protosw *pr;
			register struct ifaddr *ifa;

			for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
				if ((ifa->ifa_addr.sa_family && ifa->ifa_dstaddr.sa_family) || ifa->ifa_addr.sa_family == AF_OSI)
					break;

			if (ifa == (struct ifaddr *) 0)
				return(ENETDOWN);

			if ( ! dmvsuser() ) 
				return(EACCES);

			/*
			 * If currently owned by another family, request
			 * ownership from that family.
			 */
			if (ifp->if_addr.sa_family && (ifp->if_addr.sa_family != dmvstate->if_family)) {
				if ((pr=iffamily_to_proto(ifp->if_addr.sa_family)) && pr->pr_ifstate) 
				{
					if (! (*pr->pr_ifstate)(ifp, IFS_OWNREQ, dmvstate)) 
						return(EBUSY);
					else
						dmvstate->if_next_family = ifp->if_addr.sa_family;
				}
				else
					return(EBUSY);
			}

			/*
			 * check validity of ioctl request
			 */
			if ( !( dmvstate->if_wrstate | dmvstate->if_rdstate | dmvstate->if_xferctl ) )
				return(EINVAL);

			/*
			 * set line ownership
			 */
			if ( dmvstate->if_wrstate ) {
				u_char save_state = sc->sc_dmvcs.if_dstate;
				ifp->if_addr.sa_family = dmvstate->if_family;
				sc->sc_dmvcs = *dmvstate;
				sc->sc_dmvcs.if_dstate = save_state;
				sc->sc_dmvcs.if_wrstate = ~IFS_WRSTATE;
				sc->sc_dmvcs.if_rdstate = ~IFS_RDSTATE;
				sc->sc_dmvcs.if_xferctl = ~IFS_XFERCTL;
			}

			/*
			 * current owner can transfer control to another family
			 */
			if ( dmvstate->if_xferctl && dmvstate->if_wrstate ) {
				ifp->if_addr.sa_family = (sc->sc_dmvcs.if_family = sc->sc_dmvcs.if_next_family);
				sc->sc_dmvcs.if_next_family = ifp->if_addr.sa_family;
			}

			if ( dmvstate->if_wrstate ) {
				/*
				 * If stopping line, restart device to force dtr drop
				 * Else if currently running, stop device ( to be started again
				 * later).  Otherwise, start protocol up.
				 */
				
				if ( dmvstate->if_ustate == IFS_USROFF && 
				     (sc->sc_flag & (DMV_ALLOC | DMV_BMAPPED)) == (DMV_ALLOC | DMV_BMAPPED) && 
				     ( sc->sc_flag & DMV_RESTART) == 0 ) {
					sc->sc_flag |= DMV_RESTART;
					sc->sc_if.if_flags &= ~IFF_UP;
					sc->sc_dmvcs.if_dstate = IFS_HALTING;
					dmvrestart(sc->sc_if.if_unit);
				}
				else if (sc->sc_dmvcs.if_dstate != IFS_HALTED && sc->sc_dmvcs.if_dstate != IFS_HALTING) {
					ifp->if_flags &= ~IFF_UP;
					sc->sc_dmvcs.if_dstate = IFS_HALTING;
					dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, DMV_CNTL_RQHALT );
				} else {
					dmvinit(ifp->if_unit);
				}
			}
			/*
			 * pass back current state if requested.
			 */
			if ( dmvstate->if_rdstate ) {
				bcopy(&sc->sc_dmvcs, dmvstate, sizeof(struct ifstate));
			}
		break;
		}

	case SIOCENABLBACK:
	/*
	 * place device in loopback
	 */
		if ( ! dmvsuser() ) 
			return(EACCES);

		printf("dmv%d: internal loopback enable requested\n", ifp->if_unit);
		ifp->if_flags |= IFF_LOOPBACK;
		if ((sc->sc_flag & (DMV_ALLOC | DMV_BMAPPED)) == (DMV_ALLOC | DMV_BMAPPED) && ( sc->sc_flag & DMV_RESTART) == 0 ) {
			sc->sc_flag |= DMV_RESTART;
			sc->sc_if.if_flags &= ~IFF_UP;
			sc->sc_dmvcs.if_dstate = IFS_HALTING;
			dmvrestart(sc->sc_if.if_unit);
		}
		break;

	case SIOCDISABLBACK:
	/*
	 * place device out of loopback
	 */
		if ( ! dmvsuser() ) 
			return(EACCES);

		printf("dmv%d: internal loopback disable requested\n", ifp->if_unit);
		ifp->if_flags &= ~IFF_LOOPBACK;
		if ((sc->sc_flag & (DMV_ALLOC | DMV_BMAPPED)) == (DMV_ALLOC | DMV_BMAPPED) && ( sc->sc_flag & DMV_RESTART) == 0 ) {
			sc->sc_flag |= DMV_RESTART;
			sc->sc_if.if_flags &= ~IFF_UP;
			sc->sc_dmvcs.if_dstate = IFS_HALTING;
			dmvrestart(sc->sc_if.if_unit);
		}
		break;

	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		/*
		 * reading and zeroing line counters
		 */
	{
		u_char rqflg;
		register struct ctrreq *ctr = (struct ctrreq *)data;

		/*
 		 * Since this code is not reentrant, only one process
		 * can call it at any one time.
		 */
		if ( (sc->sc_ctrmask & DMV_MSK_BUSY) == DMV_MSK_BUSY )
			return(EBUSY);

		sc->sc_ctrmask = DMV_MSK_BUSY;

		if ( cmd == SIOCRDZCTRS ) {
			rqflg = DMV_CNTL_READZ;
		} else 
			rqflg = DMV_CNTL_READ;
		/*
		 * issue request for counters.
		 */
		dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, (rqflg | DMV_KEY_SELINT) );
		dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, (rqflg | DMV_KEY_DEOUT) );
		dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, (rqflg | DMV_KEY_DEIN) );
		dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, (rqflg | DMV_KEY_LBERR) );
		dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, (rqflg | DMV_KEY_RBERR) );
		dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, (rqflg | DMV_KEY_SELTO) );
		dmv_issue_ctl( sc, DMV_TRIB_POINT, 0, (rqflg | DMV_KEY_REPTO) );
		dmv_issue_ctl( sc, 0, 0, (rqflg | DMV_KEY_RSERR) );
		dmv_issue_ctl( sc, 0, 0, (rqflg | DMV_KEY_LSERR) );

		/*
		 * wait for response from device.
		 *
		 * If on interrupt stack, call came from timer routine.
		 * Return without sleep.  Call ifioctl later when done.
		 */
		if ( ! (movpsl() & PSL_IS))
		{
		    	while ( sc->sc_ctrmask != DMV_MSK_CMPLT )
				sleep((caddr_t) &sc->sc_ctrmask, PZERO+1);
		}
		else 
		{
			dmv_instack[sc->sc_if.if_unit] = 1;
                	error = EINVAL;
			break;
		}

		/*
		 * format counters
		 */
		ctr->ctr_type = CTR_DDCMP;
		sc->sc_errctrs.ctr_ddcmp.dst_seconds = (time.tv_sec - sc->sc_ztime) > 0xfffe ? 0xffff : (time.tv_sec - sc->sc_ztime);
		ctr->ctr_ddcmp = sc->sc_errctrs.ctr_ddcmp;
		if ( cmd == SIOCRDZCTRS ) {
			sc->sc_ztime = time.tv_sec;
			sc->sc_errctrs.ctr_ddcmp.dst_bytercvd = 0;
			sc->sc_errctrs.ctr_ddcmp.dst_bytesent = 0;
			sc->sc_errctrs.ctr_ddcmp.dst_blockrcvd = 0;
			sc->sc_errctrs.ctr_ddcmp.dst_blocksent = 0;
		} 
		sc->sc_ctrmask = 0;
		break;
	}

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			dmvinit(ifp->if_unit); 
		break;

	case SIOCSIFDSTADDR:
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			dmvinit(ifp->if_unit); 
		break;
		
        default:
                error = EINVAL;
        }
        splx(s);
        return(error);
}

/*
 * Routines supporting UNIBUS network interfaces.
 */

/*
 * Init Q-BUS for interface on uban whose headers of size hlen are to
 * end on a page boundary.  We allocate a Q-BUS map register for the page
 * with the header, and nmr more UNIBUS map registers for i/o on the adapter,
 * doing this for each receive and transmit buffer.  We also
 * allocate page frames in the mbuffer pool for these pages.
 */
dmv_ubainit(ifu, uban, hlen, nmr, bufres)
	register struct dmvuba *ifu;
	int uban, hlen, nmr;
	struct dmvbufres *bufres;
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
		cp = bufres->buffers;
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
		if (dmv_ubaalloc(ifu, ifrw, nmr) == 0) {
			struct ifrw *rw;

			for (rw = ifu->ifu_r; rw < ifrw; rw++)
				ubarelse(ifu->ifu_uban, &rw->ifrw_info);
			goto bad;
		}
	}
	/* and now transmit ring */
	for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[bufres->nxmt]; ifxp++) {
		ifrw = &ifxp->x_ifrw;
		if (dmv_ubaalloc(ifu, ifrw, nmr) == 0) {
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
 * Setup either an ifrw structure by allocating Q-BUS map registers,
 * possibly a buffered data path, and initializing the fields of
 * the ifrw structure to minimize run-time overhead.
 */
static
dmv_ubaalloc(ifu, ifrw, nmr)
	struct dmvuba *ifu;
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
dmv_get(ifu, ifrw, totlen, off0, dmv_header)
	register struct dmvuba *ifu;
	register struct ifrw *ifrw;
	int totlen, off0, dmv_header;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = ifrw->ifrw_addr;
	register short hlen = 0;

	if ( dmv_header )
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
			 * Switch pages mapped to Q-BUS with new page p,
			 * as quick form of copy.  Remap Q-BUS and invalidate.
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
dmvput(ifu, n, m)
	struct dmvuba *ifu;
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
dmvrestart(unit)
	int unit;
{
	register struct dmv_softc *sc = &dmv_softc[unit];
	register struct uba_device *ui = dmvinfo[unit];
	register struct dmvdevice *addr;
	register struct ifxmt *ifxp;
	register int i;
	register struct mbuf *m;
	struct dmvuba *ifu;

	addr = (struct dmvdevice *)ui->ui_addr;
	ifu = &sc->sc_ifuba;
	/*
	 * Let the DMV finish the MCLR.  At 1 Mbit, it should do so
	 * in about a max of 6.4 milliseconds with diagnostics enabled.
	 */
	addr->bsel1 = DMV_MCLR;
	for (i = 100000; i && (addr->bsel1 & DMV_RUN) == 0; i--)
		;
	/* Did the timer expire or did the DMR finish? */
	if ((addr->bsel1 & DMV_RUN) == 0) {
		printf("dmv%d: Startup Diagnostics Test Failed\n", unit);
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

	/* restart DMV */
	dmvinit(unit);
	sc->sc_flag &= ~DMV_RESTART;
	sc->sc_if.if_collisions++;	/* why not? */
}

/*
 * Check to see that transmitted packets don't
 * lose interrupts.  The device has to be active.
 */
dmvwatch()
{
        register struct uba_device *ui;
	register struct dmv_softc *sc;
	struct dmvdevice *addr;
	register int i;

	for (i = 0; i < nNDMV; i++) {
		sc = &dmv_softc[i];
		if ((sc->sc_flag & DMV_ACTIVE) == 0)
			continue;
        	if ((ui = dmvinfo[i]) == 0 || ui->ui_alive == 0)
			continue;
		if (sc->sc_oused) {
			sc->sc_nticks++;
			if (sc->sc_nticks > dmv_timeout) {
				sc->sc_nticks = 0;
        			addr = (struct dmvdevice *)ui->ui_addr;
				printd("dmvwatch: dmv%d hung, bsel0 = 0x%x, bsel1 = 0x%x, bsel2 = 0x%x\n",
				   i, addr->bsel0 & 0xff, addr->bsel1 & 0xff,
				   addr->bsel2 & 0xff);
				if (( sc->sc_flag & DMV_RESTART) == 0 ) {
					sc->sc_flag |= DMV_RESTART;
					sc->sc_if.if_flags &= ~IFF_UP;
					sc->sc_dmvcs.if_dstate = IFS_HALTING;
					dmvrestart(i);
				}
			}
		}
	}
	timeout(dmvwatch, (caddr_t) 0, hz);
}
/*
 * Check to make sure requestor is priveleged.  suser is
 * called when not on interrupt stack or when 
 * there is a system process.
 */
dmvsuser()
{
	if ( ! (movpsl() & PSL_IS))
		return(suser());
	return(1);
}
#endif
