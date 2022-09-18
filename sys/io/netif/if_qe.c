#ifndef lint
static char *sccsid = "@(#)if_qe.c	4.5	(ULTRIX)	2/26/91";
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
 *   University	of   California,   Berkeley,   and   from   Bell	*
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

/* ---------------------------------------------------------------------
 * Modification History 
 *
 * 24-Feb-91 - jsd
 *      Allow loopback packets if COPYALL mode is set
 *
 * 16-Nov-89 - chc
 *      Added code to print out the station address at the system boot time.
 *
 * 14-Nov-89 - chc
 *	Fixed the unaligned access problem for the 802.2 LLC transmit
 *	frame.
 *
 * 27-Oct-89 - Uttam Shikarpur
 *	Added:
 *		1) Ability to report back the type of network interface
 *		   we are running on.
 *		2) Counters to  keep track of the multicast pack., bytes sent.
 *
 * 20-OCT-89 - chc
 *	Added code to fixed the mapping register release problem
 *	Changed the mapping register information mask ( form 21 to
 *	22 ) 
 *
 * 31-Aug-89 - chc
 *      Added code to restart the qe under the non-existant memory
 *      condition
 *      Modified the code in order to support MIPS processor
 *
 * 3-May-89 Uttam Shikarpur
 *      Add support for Ethernet packet filter
 *      Moved common read code to ../net/ether_read.c
 *
 * 06-Mar-89 - Lea Gottfredsen
 * 	Correct decnet counters not to include sizeof ethernet header
 *
 * 05-Dec-88 - robin
 *	Changed the code that used the output of qballoc because the
 *	format of the way map registers is coded in the return value
 *	changed.
 *
 * 29-Jun-88 - Fred L. Templin
 *	Added code to the qeoutput() routine to prevent qeoutput from
 *	running if the interface has not yet been marked as "IFF_UP"
 *	and "IFF_RUNNING".
 *
 * 15-Jan-88 - lp
 *	Reworked how softc gets allocated. Also fixed driver to use new
 *	larger mbufs.
 *
 * 26-Feb-87  -- jsd
 *	Check return of iftype_to_proto so that we don't jump to 0.
 *	Also, restored Ed Ferris DECnet fix from 04-Jan-87 that got zapped
 *
 * 06-Jan-87  -- robin
 *	changed the way transmit and receive rings are mapped.  The
 *	old method used a loop and called the allocation routine in each
 *	iteration, this wasted a map register (fire wall) on each call.
 *	the new way makes one allocation call and the divides the mapped
 *	area up, this saves the wasted fire walls (except one).
 *
 * 04-Jan-87 -- ejf
 *	Fix to DECnet receive bytes/packets counters.
 *
 * 02-Sept-86 -- ejf
 *	Fixed problem where qe driver was not properly handling packets
 *	with an odd number of bytes less than the minimum packet size.
 *	Also, added code in interrupt routine to validate transmit list
 *	if it became invalid while there were still outstanding transmit
 *	buffers.
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 * 19-May-86 -- Chase
 *	Added code to free up mbufs left in the transmit ring after a
 *	lockup.  Failure to do this may cause systems under a heavy
 *	network load to run out of mbufs.
 *
 * 06-May-86  -- larry
 *	increase QETIMO to 1 second which implies 3 seconds till a qerestart.
 *	clear RESET bit in qeprobe and qerestart so the LQA will work.
 *
 * 15-Apr-86  -- afd
 *	Rename "unused_multi" to "qunused_multi" for extending Generic
 *	kernel to MicroVAXen.
 *
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 * 12 March 86 -- Jeff Chase
 *	Modified to handle the new MCLGET macro
 *	Changed if_qe_data.c to use more receive buffers
 *	Added a flag to poke with adb to log qe_restarts on console
 *
 * 19 Oct 85 -- rjl
 *	Changed the watch dog timer from 30 seconds to 3.  VMS is using
 * 	less than 1 second in their's. Also turned the printf into an
 *	mprintf.
 *
 *  09/16/85 -- Larry Cohen
 * 		Add 43bsd alpha tape changes for subnet routing		
 *
 *  1 Aug 85 -- rjl
 *	Panic on a non-existent memory interrupt and the case where a packet
 *	was chained.  The first should never happen because non-existant 
 *	memory interrupts cause a bus reset. The second should never happen
 *	because we hang 2k input buffers on the device.
 *
 *  1 Aug 85 -- rich
 *      Fixed the broadcast loopback code to handle Clusters without
 *      wedging the system.
 *
 *  27 Feb. 85 -- ejf
 *	Return default hardware address on ioctl request.
 *
 *  12 Feb. 85 -- ejf
 *	Added internal extended loopback capability.
 *
 *  27 Dec. 84 -- rjl
 *	Fixed bug that caused every other transmit descriptor to be used
 *	instead of every descriptor.
 *
 *  21 Dec. 84 -- rjl
 *	Added watchdog timer to mask hardware bug that causes device lockup.
 *
 *  18 Dec. 84 -- rjl
 *	Reworked driver to use q-bus mapping routines.  MicroVAX-I now does
 *	copying instead of m-buf shuffleing.
 *	A number of deficencies in the hardware/firmware were compensated
 *	for. See comments in qestart and qerint.
 *
 *  14 Nov. 84 -- jf
 *	Added usage counts for multicast addresses.
 *	Updated general protocol support to allow access to the Ethernet
 *	header.
 *
 *  04 Oct. 84 -- jf
 *	Added support for new ioctls to add and delete multicast addresses
 *	and set the physical address. 
 *	Add support for general protocols.
 *
 *  14 Aug. 84 -- rjl
 *	Integrated Shannon changes. (allow arp above 1024 and ? )
 *
 *  13 Feb. 84 -- rjl
 *
 *	Initial version of driver. derived from IL driver.
 * 
 * ---------------------------------------------------------------------
 */

#include "qe.h"
#if	NQE > 0 || defined(BINARY)
/*
 * Digital Q-BUS to NI Adapter
 */

#include "packetfilter.h"       /* NPACKETFILTER */
#include "../data/if_qe_data.c"
extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern int net_output();
extern struct timeval time;
extern timeout();

int	qeprobe(), qeattach(), qeint(), qewatch();
int	qeinit(),qestart(),qeioctl(),qereset(),qewatch();
struct mbuf *qeget();

u_short qestd[] = { 0 };
struct	uba_driver qedriver =
	{ qeprobe, 0, qeattach, 0, qestd, "qe", qeinfo };

u_char qunused_multi[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
u_char qe_multi[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }; 

#define OMASK 0x1ff
#define REGMASK 0x1fff
#define NREGMASK 0x1ff
#define QBAI_MR(i) ((int)((i) >> 9) & REGMASK)
#define QBAI_NMR(i) ((int)((i) >> 22) & NREGMASK)
#define QE_TIMEO	(1 * hz)
#define	QEUNIT(x)	minor(x)
static int mask = 0x1fffff;		/* address mask		*/
int qewatchrun = 0;			/* watchdog running	*/
int qe_show_restarts = 0;		/* restart diagnosis	*/ 	

int qe_runt = 0;
/*
 * The deqna shouldn't recieve more than ETHERMTU + sizeof(struct ether_header)
 * but will actually take in up to 2048 bytes. To guard against the receiver
 * chaining buffers (which we aren't prepared to handle) we allocate 2kb 
 * size buffers.
 */
#define MAXPACKETSIZE 2048		/* Should really be ETHERMTU	*/
/*
 * Probe the QNA to see if it's there
 */
qeprobe(reg)
	caddr_t reg;
{

#ifdef vax
	register struct qedevice *addr = (struct qedevice *)reg;
#endif vax
#ifdef mips
	volatile struct qedevice *addr = (struct qedevice *)reg;
#endif mips
	register struct qe_ring *rp; 
	register struct qe_ring *prp; 	/* physical rp 		*/
	register int i, j, ncl;
	register struct qe_softc *sc;
	struct uba_hd *uh = uba_hd;	/* only one bus so no index here */
	int unit;

	KM_ALLOC(sc,struct qe_softc *,sizeof(struct qe_softc),KM_DEVBUF,KM_CLEAR|KM_CONTIG|KM_CALL);
	qe_softc[nNQE] = sc;
	unit = nNQE++;

	/*
	 * Set the address mask for the particular cpu
	 */
	if( uba_hd[numuba].uba_type&UBAUVI )
		mask = 0x3fffff;
	else
		mask = 0x1fffff;

	/*
	 * The QNA interrupts on i/o operations. To do an I/O operation 
	 * we have to setup the interface by transmitting a setup  packet.
	 */
	addr->qe_csr = QE_RESET;
	addr->qe_vector = (uba_hd[numuba].uh_lastiv -= 4);

	/*
	 * Map the communications area and the setup packet.
	 */
	sc->setupaddr =
		qballoc(0, sc->setup_pkt, sizeof(sc->setup_pkt), 0);
	sc->rringaddr = (struct qe_ring *)
		qballoc(0, sc->rring, sizeof(struct qe_ring)*(nNTOT+2),0);
	prp = (struct qe_ring *)((int)sc->rringaddr & mask);

        addr->qe_csr &= ~QE_RESET;

	/*
	 * The QNA will loop the setup packet back to the receive ring
	 * for verification, therefore we initialize the first 
	 * receive & transmit ring descriptors and link the setup packet
	 * to them.
	 */
	qeinitdesc( sc->tring, sc->setupaddr & mask, sizeof(sc->setup_pkt));
	qeinitdesc( sc->rring, sc->setupaddr & mask, sizeof(sc->setup_pkt));

	rp = (struct qe_ring *)sc->tring;
	rp->qe_setup = 1;
	rp->qe_eomsg = 1;
	rp->qe_flag = rp->qe_status1 = QE_NOTYET;
	rp->qe_valid = 1;

	rp = (struct qe_ring *)sc->rring;
	rp->qe_flag = rp->qe_status1 = QE_NOTYET;
	rp->qe_valid = 1;

	/*
	 * Get the addr off of the interface and place it into the setup
	 * packet. This code looks strange due to the fact that the address
	 * is placed in the setup packet in col. major order. 
	 */
	for( i = 0 ; i < 6 ; i++ )
		sc->setup_pkt[i][1] = (u_char)addr->qe_sta_addr[i];

	qesetup( sc );
	/*
	 * Start the interface and wait for the packet.
	 */
	j = cvec;
	addr->qe_csr = QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT;
	addr->qe_rcvlist_lo = (short)prp;
	addr->qe_rcvlist_hi = (short)((int)prp >> 16);
	prp += nNRCV+1;
	addr->qe_xmtlist_lo = (short)prp;
	addr->qe_xmtlist_hi = (short)((int)prp >> 16);
	DELAY(10000);
	/*
	 * All done with the bus resources. If it's a uVAX-I they weren't
	 * really allocated otherwise deallocated them.
	 */
	if((uba_hd[numuba].uba_type&UBAUVI) == 0 ) {
		qbarelse(0, &sc->setupaddr);
		qbarelse(0, &sc->rringaddr);
	}
	if( cvec == j ) 
		return 0;		/* didn't interrupt	*/

	/*
	 * Allocate page size buffers now. If we wait until the network
	 * is setup they have already fragmented. By doing it here in
	 * conjunction with always coping on uVAX-I processors we obtain
	 * physically contigous buffers for dma transfers.
	 */
	ncl = clrnd((int)btoc(MAXPACKETSIZE) + CLSIZE) / CLSIZE;
	KM_ALLOC(sc->buffers, char *, nNTOT*ncl*CLBYTES, KM_DEVBUF, KM_NOWAIT|KM_CONTIG|KM_CALL);
	if(sc->buffers == NULL)
		panic("noncontig alloc in qe");
	return( sizeof(struct qedevice) );
}
 
/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
qeattach(ui)
	struct uba_device *ui;
{
	register struct qe_softc *sc = qe_softc[ui->ui_unit];
	register struct ifnet *ifp = &sc->is_if;
#ifdef vax
	register struct qedevice *addr = (struct qedevice *)ui->ui_addr;
#endif vax
#ifdef mips
	volatile  struct qedevice *addr = (struct qedevice *)ui->ui_addr;
#endif mips
	register int i;
	struct sockaddr_in *sin;
	int nb;

	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "qe";
	ifp->if_mtu = ETHERMTU;
	ifp->if_type = IFT_ETHER;
	ifp->if_flags |= IFF_BROADCAST | IFF_DYNPROTO;
	((struct arpcom *)ifp)->ac_ipaddr.s_addr = 0;

	/*
	 * Read the address from the prom and save it.
	 */
	for( i=0 ; i<6 ; i++ )
		sc->setup_pkt[i][1] = sc->is_addr[i] = (u_char)addr->qe_sta_addr[i];

	/*
	 * Save the vector for initialization at reset time.
	 */
	sc->qe_intvec = addr->qe_vector;

	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
	ifp->if_init = qeinit;
	ifp->if_output = net_output;
	ifp->if_start = qestart;
	ifp->if_ioctl = qeioctl;
	ifp->if_reset = qereset;
	addr->qe_vector |= 1;	
	if(addr->qe_vector & 01) {
		ifp->if_sysid_type = 37;
		if(addr->qe_vector & 0x8000) {
			bcopy("DEC DELQA Ethernet Interface Normal Mode", ifp->if_version, 40);
			nb = 40;
		} else {
			bcopy("DEC DELQA Ethernet Interface DEQNA-lock Mode", ifp->if_version, 44);
			nb = 44;
		}
	} else {
		ifp->if_sysid_type = 5;
		bcopy("DEC DEQNA Ethernet Interface", ifp->if_version, 28);
		nb = 28;
	}
	ifp->if_version[nb] = '\0';
        printf("qe%d: %s, hardware address %s\n", ui->ui_unit,
		ifp->if_version,
                ether_sprintf(sc->is_addr));
	addr->qe_vector &= ~1;


#if NPACKETFILTER > 0
	/* Tell the packet filter we are here */
	attachpfilter(&(sc->qe_ed));
#endif NPACKETFILTER
	if_attach(ifp);
#ifdef mips
	ifp->if_snd.ifq_maxlen = 1000;
#endif mips
}

/*
 * Reset of interface after UNIBUS reset.
 * If interface is on specified uba, reset its state.
 */
qereset(unit, uban)
	int unit, uban;
{
	register struct uba_device *ui;

	if (unit >= nNQE || (ui = qeinfo[unit]) == 0 || ui->ui_alive == 0 ||
		ui->ui_ubanum != uban)
		return;
	printf(" qe%d", unit);
	qeinit(unit);
}
 
/*
 * Initialization of interface. 
 */
qeinit(unit)
	int unit;
{
	register struct qe_softc *sc = qe_softc[unit];
	register struct uba_device *ui = qeinfo[unit];
#ifdef vax
	register struct qedevice *addr = (struct qedevice *)ui->ui_addr;
#endif vax
#ifdef mips
	volatile struct qedevice *addr = (struct qedevice *)ui->ui_addr;
#endif mips
	register struct ifnet *ifp = &sc->is_if;
	register int i;
	int s;

	/* address not known */
	/* DECnet must set this somewhere to make device happy */
	if (ifp->if_addrlist == (struct ifaddr *)0)
			return;
	if (ifp->if_flags & IFF_RUNNING)
		return;

	/*
	 * map the communications area onto the device 
	 */
	sc->rringaddr = (struct qe_ring *)((int)qballoc(0,
		sc->rring, sizeof(struct qe_ring)*(nNTOT+2),0)&mask);
	sc->tringaddr = sc->rringaddr+nNRCV+1;
	sc->setupaddr =	qballoc(0, sc->setup_pkt, sizeof(sc->setup_pkt), 0) & mask;
	/*
	 * init buffers and maps
	 */
	if (qe_ubainit(&sc->qeuba, ui->ui_ubanum,
	    sizeof (struct ether_header), (int)btoc(MAXPACKETSIZE), sc->buffers,unit) == 0) { 
		printf("qe%d: can't initialize\n", unit);
		sc->is_if.if_flags &= ~IFF_UP;
		return;
	}
	/*
	 * Init the buffer descriptors and indexes for each of the lists and
	 * loop them back to form a ring.
	 */
	for( i = 0 ; i < nNRCV ; i++ ){
		qeinitdesc( &sc->rring[i],
			sc->qeuba.ifu_r[i].ifrw_info & mask, MAXPACKETSIZE);
		sc->rring[i].qe_flag = sc->rring[i].qe_status1 = QE_NOTYET;
		sc->rring[i].qe_valid = 1;
	}
	qeinitdesc( &sc->rring[i], NULL, 0 );

	sc->rring[i].qe_addr_lo = (short)sc->rringaddr;
	sc->rring[i].qe_addr_hi = (short)((int)sc->rringaddr >> 16);
	sc->rring[i].qe_chain = 1;
	sc->rring[i].qe_flag = sc->rring[i].qe_status1 = QE_NOTYET;
	sc->rring[i].qe_valid = 1;

	for( i = 0 ; i <= nNXMT ; i++ )
		qeinitdesc( &sc->tring[i], NULL, 0 );
	i--;

	sc->tring[i].qe_addr_lo = (short)sc->tringaddr;
	sc->tring[i].qe_addr_hi = (short)((int)sc->tringaddr >> 16);
	sc->tring[i].qe_chain = 1;
	sc->tring[i].qe_flag = sc->tring[i].qe_status1 = QE_NOTYET;
	sc->tring[i].qe_valid = 1;

	sc->nxmit = sc->otindex = sc->tindex = sc->rindex = 0;

	/*
	 * Take the interface out of reset, program the vector, 
	 * enable interrupts, and tell the world we are up.
	 */
	s = splimp();
	addr->qe_vector = (u_short)sc->qe_intvec;
	sc->addr = (struct qedevice *) addr;
	if ( ifp->if_flags & IFF_LOOPBACK )
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ELOOP;
	else
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ILOOP;
	addr->qe_rcvlist_lo = (short)sc->rringaddr;
	addr->qe_rcvlist_hi = (short)((int)sc->rringaddr >> 16);
	ifp->if_flags |= IFF_UP | IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	qesetup( sc );
	qestart( unit );
	sc->ztime = time.tv_sec;
	splx( s );

}
 
/*
 * Start output on interface.
 *
 */
qestart(unit)
	int unit;
{
	register struct qe_softc *sc = qe_softc[unit];
	register struct qedevice *addr;
	register struct qe_ring *rp;
	register int index;
	struct mbuf *m;
	int buf_addr, len, s;
	struct uba_device *ui = qeinfo[unit];

	 
	s = splimp();
	addr = (struct qedevice *)ui->ui_addr;
	/*
	 * The deqna doesn't look at anything but the valid bit
	 * to determine if it should transmit this packet. If you have
	 * a ring and fill it the device will loop indefinately on the
	 * packet and continue to flood the net with packets until you
	 * break the ring. For this reason we never queue more than n-1
	 * packets in the transmit ring. 
	 *
	 * The microcoders should have obeyed their own defination of the
	 * flag and status words, but instead we have to compensate.
	 */
	for( index = sc->tindex; 
		sc->tring[index].qe_valid == 0 && sc->nxmit < (nNXMT-1) ;
		sc->tindex = index = ++index % nNXMT){
		rp = &sc->tring[index];
		if( sc->setupqueued ) {
			buf_addr = sc->setupaddr;
			len = 128;
			rp->qe_setup = 1;
			sc->setupqueued = 0;
		} else {
			IF_DEQUEUE(&sc->is_if.if_snd, m);
			if( m == 0 ){
				splx(s);
				return;
			}
			buf_addr = sc->qeuba.ifu_w[index].x_ifrw.ifrw_info;
			len = qeput(&sc->qeuba, index, m);
		}
		/*
		 *  Does buffer end on odd byte ? 
		 */
		if( len & 1 ) {
			len++;
			rp->qe_odd_end = 1;
		}
#ifdef  IFF_PROMISC
		if (rp->qe_setup && (sc->is_if.if_flags & IFF_PROMISC)) {
			len |= QE_PROMISCUOUS;
		}
#endif  IFF_PROMISC
#ifdef  IFF_ALLMULTI
		if (rp->qe_setup && (sc->is_if.if_flags & IFF_ALLMULTI))
			len |= QE_MULTICAST;
#endif  IFF_ALLMULTI
		rp->qe_buf_len = -(len/2);
		buf_addr &= mask;
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_addr_lo = (short)buf_addr;
		rp->qe_addr_hi = (short)(buf_addr >> 16);
		rp->qe_eomsg = 1;
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 1;
		sc->nxmit++;
		sc->is_if.if_flags |= IFF_OACTIVE;
		/*
		 * If the watchdog time isn't running kick it.
		 */
		sc->timeout=1;
		if( !qewatchrun++ ) 
			timeout(qewatch,0,QE_TIMEO);
			
		/*
		 * See if the xmit list is invalid.
		 */
		if( (addr->qe_csr & QE_XL_INVALID) && (rp->qe_status1 == QE_NOTYET)) {
			buf_addr = (int)(sc->tringaddr+index);
			addr->qe_xmtlist_lo = (short)buf_addr;
			addr->qe_xmtlist_hi = (short)(buf_addr >> 16);
		}
	}
	splx( s );
}
 
/*
 * Ethernet interface interrupt processor
 */
qeintr(unit)
	int unit;
{
	register struct qe_softc *sc = qe_softc[unit];
#ifdef vax
	struct qedevice *addr = (struct qedevice *)qeinfo[unit]->ui_addr;
#endif vax
#ifdef mips
	volatile struct qedevice *addr = (struct qedevice *)qeinfo[unit]->ui_addr;
#endif mips
	int s, buf_addr;
	u_short csr;

	s = splimp();
	csr = addr->qe_csr;
	addr->qe_csr = csr;
	if( csr & QE_RCV_INT ) 
		qerint( unit );
	if( csr & QE_XMIT_INT )
		qetint( unit );
	if( csr & QE_NEX_MEM_INT )
		{
		if ( qe_show_restarts ) 
			mprintf("qe%d: non-existant memory  \n", unit);
		qerestart( sc );  
	}
	if( addr->qe_csr & QE_RL_INVALID && sc->rring[sc->rindex].qe_status1 == QE_NOTYET ) {
		buf_addr = (int)&sc->rringaddr[sc->rindex];
		addr->qe_rcvlist_lo = (short)buf_addr;
		addr->qe_rcvlist_hi = (short)(buf_addr >> 16);
	}

	if( addr->qe_csr & QE_XL_INVALID && sc->tring[sc->otindex].qe_status1 == QE_NOTYET && sc->nxmit > 0 ) {
		buf_addr = (int)&sc->tringaddr[sc->otindex];
		addr->qe_xmtlist_lo = (short)buf_addr;
		addr->qe_xmtlist_hi = (short)(buf_addr >> 16);
	}
	splx( s );
}
 
/*
 * Ethernet interface transmit interrupt.
 */

qetint(unit)
	int unit;
{
	register struct qe_softc *sc = qe_softc[unit];
	register struct qe_ring *rp;
	register struct ifrw *ifrw;
	register struct ifxmt *ifxp;
	register struct ether_header *eh;
	register int status1, setupflag;
	short len;


	sc->qe_rl_invalid = 0; /* dallas */
#ifdef mips 
	/*
	 * Flush the cache for the transmit ring descriptor
	 */
 	clean_dcache(PHYS_TO_K0(svtophy(&sc->tring[0])), sizeof(struct qe_ring)* nNXMT);  
#endif mips 
	while( sc->otindex != sc->tindex && sc->tring[sc->otindex].qe_status1 != QE_NOTYET && sc->nxmit > 0 ) {
		/*
		 * Save the status words from the descriptor so that it can
		 * be released.
		 */
		rp = &sc->tring[sc->otindex];
		status1 = rp->qe_status1;
		setupflag = rp->qe_setup;
		len = (-rp->qe_buf_len) * 2;
		if( rp->qe_odd_end )
			len++;
		/*
		 * Init the buffer descriptor
		 */
		bzero( rp, sizeof(struct qe_ring));

		if( --sc->nxmit == 0 ) {
			sc->timeout = 0;
			sc->is_if.if_flags &= ~IFF_OACTIVE;
		}

		if( !setupflag ) {
			/*
			 * Do some statistics.
			 */
			sc->is_if.if_opackets++;
			sc->is_if.if_collisions += ( status1 & QE_CCNT ) >> 4;
			/*
			 * Accumulate DECnet statistics
			 */
			if (status1 & QE_CCNT) {
				if (((status1 & QE_CCNT) >> 4) == 1) {
					if (sc->ctrblk.est_single != 0xffffffff)
						sc->ctrblk.est_single++;
				} else {
					if (sc->ctrblk.est_multiple != 0xffffffff)
						sc->ctrblk.est_multiple++;
				}
			}
			if (status1 & QE_FAIL)
				if (sc->ctrblk.est_collis != 0xffff)
					sc->ctrblk.est_collis++;
			if( status1 & QE_ERROR ) { 
				sc->is_if.if_oerrors++;
				if (sc->ctrblk.est_sendfail != 0xffff)
					sc->ctrblk.est_sendfail++;
				sc->ctrblk.est_sendfail_bm = 0;
				if (status1 & QE_ABORT)
					sc->ctrblk.est_sendfail_bm |= 1;
				if (status1 & (QE_NOCAR | QE_LOSS))
					sc->ctrblk.est_sendfail_bm |= 2;
				
			} else {
                                /*
                                 * Accumulate statistics for DECnet
                                 */
                                sc->ctrblk.est_bytesent += len;
                                if (sc->ctrblk.est_bloksent !=
                                    (unsigned) 0xffffffff)
                                        sc->ctrblk.est_bloksent++;
			}
			/*
			 * If this was a broadcast packet loop it
			 * back because the hardware can't hear it's own
			 * transmits and the rwho deamon expects to see them.
			 * This code will have to be expanded to include multi-
			 * cast if the same situation developes.
			 */
			ifxp = &sc->qeuba.ifu_w[sc->otindex];
			ifrw = &sc->qeuba.ifu_w[sc->otindex].x_ifrw;
			eh = (struct ether_header *)ifrw->ifrw_addr;
                        /*
                         * Accumulate statistics for DECnet
                         */
			if(eh->ether_dhost[0] & 1) {
				sc->ctrblk.est_mbytesent += len;
			if (sc->ctrblk.est_mbloksent != (unsigned) 0xffffffff)
				sc->ctrblk.est_mbloksent++;
			}

/*
 * This is a Kludge to do a fast check to see if the ethernet
 * address is all 1's, the ethernet broadcast addr, and loop the
 * packet back.
 */

			if((!(bcmp(eh->ether_dhost,qe_multi,6))) || (sc->is_if.if_flags & IFF_PFCOPYALL)) {
				qeread(sc, ifrw, len, ifxp->x_xtofree);
				ifxp->x_xtofree =0;
			}else if( ifxp->x_xtofree ) {
				m_freem( ifxp->x_xtofree );
				ifxp->x_xtofree = 0;
			}
		}
		sc->otindex = ++sc->otindex % nNXMT;
	}
	if (!(sc->is_if.if_flags & IFF_OACTIVE))
		qestart( unit );
}
 
/*
 * Ethernet interface receiver interrupt.
 * If can't determine length from type, then have to drop packet.  
 * Othewise decapsulate packet based on type and pass to type specific 
 * higher-level input routine.
 */
qerint(unit)
	int unit;
{
	register struct qe_softc *sc = qe_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	register struct qe_ring *rp;
	register int last;
	u_short status1, status2;
	short len;
	int bufaddr;
	struct ether_header *eh;

	/*
	 * Traverse the receive ring looking for packets to pass back.
	 * The search is complete when we find a descriptor not in use.
	 *
	 * As in the transmit case the deqna doesn't honor it's own protocols
	 * so there exists the possibility that the device can beat us around
	 * the ring. The proper way to guard against this is to insure that
	 * there is always at least one invalid descriptor. We chose instead
	 * to make the ring large enough to minimize the problem. With a ring
	 * size of 4 we haven't been able to see the problem. To be safe we
	 * doubled that to 8.
	 *
	 */

#ifdef mips 
	/*
	 * Flush the cache for the receive ring descriptor
	 */
	clean_dcache(PHYS_TO_K0(svtophy(&sc->rring[0])), sizeof(struct qe_ring)* nNRCV);  
#endif mips 

	for( last = sc->rindex; sc->rring[sc->rindex].qe_status1 != QE_NOTYET ;
	     last = sc->rindex = ++sc->rindex % nNRCV ){
		rp = &sc->rring[sc->rindex];
		status1 = rp->qe_status1;
		status2 = rp->qe_status2;
		bzero( rp, sizeof(struct qe_ring));
		/* Discard chained packets */
		while( (status1 & QE_MASK) == QE_MASK ) {
			last = ++last % nNRCV;
			rp = &sc->rring[last];
			status1 = rp->qe_status1;
			bzero( rp, sizeof(struct qe_ring));
			if ((status1 & QE_MASK) != QE_MASK) {
				if (status1 == QE_NOTYET)
					/* Should NEVER happen */
					panic("qe: chained packet");
				else
					goto drop;
			}
		}
		len = ((status1 & QE_RBL_HI) | (status2 & QE_RBL_LO));
 
		if( ! (ifp->if_flags & IFF_LOOPBACK) ) {
			if( status1 & QE_ERROR ) {
				if (status1 & QE_RUNT )
					qe_runt++;
				if ((status1 & QE_DISCARD) && 
					(status1&(QE_OVF|QE_CRCERR|QE_FRAME))){
					/* A real error */
					ifp->if_ierrors++;
					ifp->if_ipackets++;
  					/* Bump up receive fails */
                                        if (sc->ctrblk.est_recvfail != 0xffff)
					sc->ctrblk.est_recvfail++;
                                        /* Clear previous bitmask; set bits */
					sc->ctrblk.est_recvfail_bm = 0;
					if (status1 & QE_OVF) {
						sc->ctrblk.est_recvfail_bm |= 4;
#if     NPACKETFILTER > 0
						sc->qe_ed.ess_missed++;
#endif  NPACKETFILTER
					}
					if (status1 & QE_CRCERR)
						sc->ctrblk.est_recvfail_bm |= 1;
					if (status1 & QE_FRAME)
						sc->ctrblk.est_recvfail_bm |= 2;
				}
			} else {
				/*
				 * We don't process setup packets.
				 */
				if( !(status1 & QE_ESETUP) ) {
						len += 60;
#ifdef mips 
					/*
					 * Flush the cache memory for the
					 * receiver ring buffer
					 */
					clean_dcache(PHYS_TO_K0(svtophy(sc->qeuba.ifu_r[sc->rindex].ifrw_addr)),len);
#endif mips
					qeread(sc, &sc->qeuba.ifu_r[sc->rindex],
						len - sizeof(struct ether_header),0);
				}
			}
		} else {
#ifdef mips 
			/*
			 * Flush the cache memory for the
			 * receiver ring buffer
			 */
			clean_dcache(PHYS_TO_K0(svtophy(sc->qeuba.ifu_r[sc->rindex].ifrw_addr)),len);
#endif mips
			eh = (struct ether_header *)sc->qeuba.ifu_r[sc->rindex].ifrw_addr;
			if ( bcmp(eh->ether_dhost, sc->is_addr, 6) == NULL )
					qeread(sc, &sc->qeuba.ifu_r[sc->rindex],
						len - sizeof(struct ether_header),0);
		}

		/*
		 * Return the buffer(s) to the ring. Only have multiple
		 * buffers on chained packets.
		 */
drop:
		bufaddr = sc->qeuba.ifu_r[sc->rindex].ifrw_info & mask;
		rp->qe_buf_len = -((MAXPACKETSIZE)/2);
		rp->qe_addr_lo = (short)bufaddr;
		rp->qe_addr_hi = (short)((int)bufaddr >> 16);
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 1;
		if (sc->rindex != last) {
			sc->rindex = ++sc->rindex % nNRCV;
			goto drop;
		}
	}
}

/*
 * Process an ioctl request.
 */
qeioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct qe_softc *sc = qe_softc[ifp->if_unit];
	struct uba_device *ui = qeinfo[ifp->if_unit];
	struct qedevice *addr = (struct qedevice *)ui->ui_addr;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	struct ctrreq *ctr = (struct ctrreq *)data;
	struct protosw *pr;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int i,j = -1,s = splimp(), error = 0;

	switch (cmd) {

	case SIOCENABLBACK:
		printf("qe%d: internal loopback enable requested\n", ifp->if_unit);
                ifp->if_flags |= IFF_LOOPBACK;
#ifdef notdef
		if((ifp->if_flags |= IFF_LOOPBACK) & IFF_RUNNING)
			if_rtinit(ifp, -1);
#endif
		qerestart( sc );
		break;
 
	case SIOCDISABLBACK:
		printf("qe%d: internal loopback disable requested\n", ifp->if_unit);
                ifp->if_flags &= ~IFF_LOOPBACK;
#ifdef notdef
		if((ifp->if_flags &= ~IFF_LOOPBACK) & IFF_RUNNING)
			if_rtinit(ifp, -1);
#endif
		qerestart( sc );
		qeinit( ifp->if_unit );
		break;
 
	case SIOCRPHYSADDR:
		bcopy(sc->is_addr, ifd->current_pa, 6);
		for( i = 0; i < 6; i++ )
			ifd->default_pa[i] = (u_char)addr->qe_sta_addr[i];
		break;
 
	case SIOCSPHYSADDR:
		bcopy(ifr->ifr_addr.sa_data,sc->is_addr,MULTISIZE);
#if     NPACKETFILTER > 0
		pfilt_newaddress(sc->qe_ed.ess_enetunit, sc->is_addr);
#endif  NPACKETFILTER > 0
		for ( i = 0; i < 6; i++ )
			sc->setup_pkt[i][1] = sc->is_addr[i];
		if (ifp->if_flags & IFF_RUNNING) {
			qesetup( sc );
		}
		qeinit(ifp->if_unit);
		break;

	case SIOCDELMULTI:
	case SIOCADDMULTI:
		if (cmd == SIOCDELMULTI) {
			for (i = 0; i < NMULTI; i++)
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					if (--sc->muse[i] == 0)
						bcopy(qunused_multi,&sc->multi[i],MULTISIZE);
				}
		} else {
			for (i = 0; i < NMULTI; i++) {
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					sc->muse[i]++;
					goto done;
				}
				if (bcmp(&sc->multi[i],qunused_multi,MULTISIZE) == 0)
					j = i;
			}
			if (j == -1) {
				printf("qe%d: SIOCADDMULTI failed, multicast list full: %d\n",ui->ui_unit,NMULTI);
				error = ENOBUFS;
				goto done;
			}
			bcopy(ifr->ifr_addr.sa_data, &sc->multi[j], MULTISIZE);
			sc->muse[j]++;
		}
		for ( i = 0; i < 6; i++ )
			sc->setup_pkt[i][1] = sc->is_addr[i];
		if (ifp->if_flags & IFF_RUNNING) {
			qesetup( sc );
		}
		break;

	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		ctr->ctr_ether = sc->ctrblk;
		ctr->ctr_type = CTR_ETHER;
		ctr->ctr_ether.est_seconds = (time.tv_sec - sc->ztime) > 0xfffe ? 0xffff : (time.tv_sec - sc->ztime);
		if (cmd == SIOCRDZCTRS) {
			sc->ztime = time.tv_sec;
			bzero(&sc->ctrblk, sizeof(struct estat));
			qe_runt = 0;
		}
		break;

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		qeinit(ifp->if_unit);
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
 		* Run a setup packet in case things have changed;
 		* someone may be trying to turn on (or off) promiscuous
 		* mode, for example.
 		*/
		if (ifp->if_flags & IFF_RUNNING)
			qesetup(sc);
		break;
#endif  IFF_PROMISC
	default:
		error = EINVAL;

	}
done:	splx(s);
	return (error);
}

 

/*
 * Initialize a ring descriptor with mbuf allocation side effects
 */
qeinitdesc( rp, buf, len )
	register struct qe_ring *rp;
	char *buf; 			/* mapped address	*/
	short len;
{
	/*
	 * clear the entire descriptor
	 */
	bzero( rp, sizeof(struct qe_ring));

	if( len ) {
		rp->qe_buf_len = (short)(-(len/2));
		rp->qe_addr_lo = (short)buf;
		rp->qe_addr_hi = (short)((int)buf >> 16);
	}
}
/*
 * Build a setup packet - the physical address will already be present
 * in first column.
 */
qesetup( sc )
struct qe_softc *sc;
{
	int i, j, offset = 0, next = 3;

	/*
	 * Copy the target address to the rest of the entries in this row.
	 */
	 for ( j = 0; j < 6 ; j++ )
		for ( i = 2 ; i < 8 ; i++ )
			sc->setup_pkt[j][i] = sc->setup_pkt[j][1];
	/*
	 * Duplicate the first half.
	 */
	bcopy(sc->setup_pkt, sc->setup_pkt[8], 64);
	/*
	 * Fill in the broadcast address.
	 */
	for ( i = 0; i < 6 ; i++ )
		sc->setup_pkt[i][2] = 0xff;
	/*
	 * If the device structure is available fill in the multicast address
	 * in the rest of the setup packet.
	 */
	for ( i = 0; i < NMULTI; i++ ) {
		if (bcmp(&sc->multi[i],qunused_multi,MULTISIZE) != 0) {
			for ( j = 0; j < 6; j++ )
				sc->setup_pkt[offset+j][next] = sc->multi[i].qm_char[j];
			if (++next == 8) {
				next = 1;
				offset = 8;
			}
		}
	}
	sc->setupqueued++;
}
/*
 * Routines supporting Q-BUS network interfaces.
 */

/*
 * Init Q-BUS for interface on uban whose headers of size hlen are to
 * end on a page boundary.  We allocate a Q-BUS map register for the page
 * with the header, and nmr more Q-BUS map registers for i/o on the adapter,
 * doing this for each receive and transmit buffer.  We also
 * allocate page frames in the mbuffer pool for these pages.
 */
qe_ubainit(ifu, uban, hlen, nmr, mptr,unit)
	register struct qeuba *ifu;
	int uban, hlen, nmr,unit;
	char *mptr;
{
	register caddr_t cp, dp;
	register struct ifrw *ifrw;
	register struct ifxmt *ifxp;
	int i, ncl, flag, X_info, R_info, T_info;

	ncl = clrnd(nmr + CLSIZE) / CLSIZE;
	if (ifu->ifu_r[0].ifrw_addr)
		/*
		 * If the first read buffer has a non-zero
		 * address, it means we have already allocated core
		 */
		cp = ifu->ifu_r[0].ifrw_addr - (CLBYTES - hlen);
	else {
		cp = mptr;
		if (cp == 0)
			return (0);
		ifu->ifu_hlen = hlen;
		ifu->ifu_uban = uban;
		ifu->ifu_uba = uba_hd[uban].uh_uba;
		dp = cp + CLBYTES - hlen;
		for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[nNRCV]; ifrw++) {
			ifrw->ifrw_addr = dp;
			ifrw->ifrw_info = 0;
			dp += MAXPACKETSIZE + CLBYTES;
		}
		for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[nNXMT]; ifxp++) {
			ifxp->x_ifrw.ifrw_addr = dp;
			dp += MAXPACKETSIZE;
		}
	}
	/* allocate for receive ring */
	for (ifrw = ifu->ifu_r, flag=0; ifrw < &ifu->ifu_r[nNRCV]; ifrw++, flag++) {
		if((qe_ubaalloc(ifu, ifrw, nmr*2, -1, unit)) == 0) 
			goto release;
	}
	/* and now transmit ring */
	for (ifxp = ifu->ifu_w, flag=0; ifxp < &ifu->ifu_w[nNXMT]; ifxp++, flag++) {
		ifrw = &ifxp->x_ifrw;
		T_info=qe_ubaalloc(ifu, ifrw, (nmr * nNXMT) + nNXMT, flag,unit);
		if( flag == 0) X_info = T_info;
		if (T_info == 0) {
			if(flag > 0)
				qbarelse(ifu->ifu_uban, &X_info);
			goto release;
		}
		for (i = 0; i < nmr; i++)
			ifxp->x_map[i] = ifrw->ifrw_mr[i];
		ifxp->x_xswapd = 0;
	}
	return (1);
release:
	for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[nNRCV]; ifrw) {
		if(ifrw->ifrw_info)
			qbarelse(ifu->ifu_uban, &ifrw->ifrw_info);
	}
	m_pgfree(cp, nNTOT * ncl);
	ifu->ifu_r[0].ifrw_addr = 0;
	return(0);
}

/*
 * This routine sets up the buffers and maps to the Q-bus.  The 
 * mapping is the same as the UNIBUS, ie 496 map registers.  The flag
 * input must be zero on the first call, this maps the memory.  Successive
 * calls (flags is non-zero) breaks the mapped area up into buffers 
 * and sets up ifrw structures.
 * Setup either a ifrw structure by allocating Q-BUS map registers,
 * possibly a buffered data path, and initializing the fields of
 * the ifrw structure to minimize run-time overhead.
 */
qe_ubaalloc(ifu, ifrw, nmr, do_alloc,unit)
	struct qeuba *ifu;
	register struct ifrw *ifrw;
	int nmr;
	int do_alloc, unit;
{
	static int info=0;
	static int ubai;
	static int numubai;
	int rinfo;
	register int nbpg;

	if(do_alloc == 0)
	{
		info = qballoc(ifu->ifu_uban, ifrw->ifrw_addr,
				nmr*NBPG , ifu->ifu_flags);
		if (info == 0){
			return (0);
		}
		ubai=QBAI_MR(info);
		numubai=QBAI_NMR(info);
		numubai *= 8;
	} else if(do_alloc == -1) {
		info = qballoc(ifu->ifu_uban, ifrw->ifrw_addr,
				nmr*NBPG+ifu->ifu_hlen, ifu->ifu_flags);
		if (info == 0)
			return (0);
		ifrw->ifrw_info = info;
		ifrw->ifrw_bdp = 0;
		ifrw->ifrw_proto = UBAMR_MRV ;
		ifrw->ifrw_mr = &ifu->ifu_uba->uba_map[QBAI_MR(info) + 1];
		return(1);
	}
	
	/* micro vax 1 is contigus phy. mem */
	if(uba_hd[ifu->ifu_uban].uba_type & UBAUVI)
	{
		ifrw->ifrw_info = info;
		ifrw->ifrw_bdp = 0;
		ifrw->ifrw_proto = UBAMR_MRV ;
		ifrw->ifrw_mr = &ifu->ifu_uba->uba_map[UBAI_MR(info) + 1];
		rinfo = info;
		info = info + (((int)((MAXPACKETSIZE+(NBPG - 1))/NBPG)+1) * NBPG);
		return(rinfo);
	}
		
	info = (info & ~(0xfffffe00));
	/* 
	 * using the same QBUS mapping register for transmit buffer on both VAX
	 * and MIPS architecture 
	 */
#ifdef vax
	nbpg=NBPG;
#else 
	nbpg=512;
#endif 
	/*
         * The number of mapping registers ( info<30:22> ) will not affect the
         * mapping at all.
         */
	info = info | (ubai << 9) | (((int)((MAXPACKETSIZE + (nbpg - 1))/nbpg)+1) << 22);
	ifrw->ifrw_info = info;
	ifrw->ifrw_bdp = 0;
	ifrw->ifrw_proto = UBAMR_MRV ;
	ifrw->ifrw_mr = &ifu->ifu_uba->uba_map[QBAI_MR(info) + 1];
	if(numubai >= (int)((MAXPACKETSIZE + (nbpg - 1))/nbpg))
	{
		numubai = numubai - ((int)((MAXPACKETSIZE + (nbpg - 1))/nbpg));
		ubai = ubai + ((int)((MAXPACKETSIZE + (nbpg - 1))/nbpg));
	}
	else
	{
		return (0);
	}
	return (info);
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
#define pgaligned(x) (((int)(x)&(NBPG-1)) == 0)

struct mbuf *
qeget(ifu, ifrw, totlen, off0)
	register struct qeuba *ifu;
	register struct ifrw *ifrw;
	int totlen, off0;
{
	struct mbuf *top, **mp, *m, *p;
	int off = off0, len, tlen=0;
	register caddr_t cp = ifrw->ifrw_addr + ifu->ifu_hlen;

	top = 0;
	mp = &top;
#ifdef mips
/* use bcopy instead of switch the page */
	MGET(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return(0);
	if (totlen > MLEN) {
		MCLGET(m, p);
		if (p == 0) {
			m_freem(m);
			return(0);
		}
	}
	if (off) {
		/* crunch trailer */
		off = totlen - off0;
		bcopy(cp, (mtod(m, caddr_t) + off), off0);
		cp += off0;
		bcopy(cp, mtod(m, caddr_t), off);

	} else {
		bcopy(cp, mtod(m, caddr_t), totlen);
	}
	m->m_len = totlen;
	return (m);
#endif mips
#ifdef vax
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		if (off) {
			len = totlen - off;
			cp = ifrw->ifrw_addr + ifu->ifu_hlen + off;
		} else
			len = totlen;
		if (len >= CLBYTES) {
			struct mbuf *p;
			struct pte *cpte, *ppte;
			int x, *ip, i, swt=2;
			caddr_t dp, tcp;

			MCLGET(m, p);
			if (p == 0)
				goto nopage;
			m->m_len = len;
			if((uba_hd[ifu->ifu_uban].uba_type & UBAUVI)
						|| !pgaligned(cp))
				goto copy;

		        if(len > CLBYTES)
				swt = 4;
			/*
			 * Switch pages mapped to Q-BUS with new page p,
			 * as quick form of copy.  Remap Q-BUS and invalidate.
			 */
			dp = mtod(m, caddr_t);
			tcp = cp;
			cpte = &kmempt[mtocl(cp)];
			ppte = &kmempt[mtocl(p)];

			x = btop(cp - ifrw->ifrw_addr);
			ip = (int *)&ifrw->ifrw_mr[x];
			/* Keep pages contiguous */
			for (i = 0; i < swt; i++) {
				struct pte t;
				t = *ppte; *ppte++ = *cpte; *cpte = t;
				*ip++ =
				    cpte++->pg_pfnum|ifrw->ifrw_proto;
				mtpr(TBIS, tcp);
				tcp += NBPG;
				mtpr(TBIS, dp);
				dp += NBPG;
			}
			cp += len;
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
#endif vax
}

/*
 * Map a chain of mbufs onto a network interface
 * in preparation for an i/o operation.
 * The argument chain of mbufs includes the local network
 * header which is copied to be in the mapped, aligned
 * i/o space.
 */
int qe_showput = 0;
qeput(ifu, n, m)
	struct qeuba *ifu;
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
	cp = ifrw->ifrw_addr;
	ifxp->x_xtofree = m;
#ifdef vax
	while (m) {
		dp = mtod(m, char *);
			if((uba_hd[ifu->ifu_uban].uba_type & UBAUVI) == 0
				&& claligned(cp) && claligned(dp) &&
				m->m_len == CLBYTES) {
			struct pte *pte; int *ip;
		if(qe_showput)
			cprintf("qeput %x %x %d\n", cp, dp, m->m_len);
			pte = &kmempt[mtocl(dp)];
			x = btop(cp - ifrw->ifrw_addr);
			ip = (int *)&ifrw->ifrw_mr[x];
			for (i = 0; i < CLSIZE; i++)
				*ip++ =
				    ifrw->ifrw_proto | pte++->pg_pfnum;
			xswapd |= 1 << (x>>(CLSHIFT-PGSHIFT));
			cp += m->m_len;
		} else {
		if(qe_showput)
			cprintf("qeput_slow %x %x %d\n", cp, dp, m->m_len);
			bcopy(dp, cp, (unsigned)m->m_len);
			cp += m->m_len;
		}
		m = m->m_next;
	}

	/*
	 * Pad out short transmits with NULL's
	 */
	if ((cc = cp - ifrw->ifrw_addr) < MINDATA) {
		bzero(cp, MINDATA - cc);
		cc = MINDATA;
	}

	/*
	 * Xswapd is the set of clusters we just mapped out.  Ifxp->x_xswapd
	 * is the set of clusters mapped out from before.  We compute
	 * the number of clusters involved in this operation in x.
	 * Clusters mapped out before and involved in this operation
	 * should be unmapped so original pages will be accessed by the device.
	 */
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
#endif vax
/* use bcopy instead of swapping the page */ 
#ifdef mips
	while (m) {
		dp = mtod(m, char *);
		if(qe_showput)
			cprintf("qeput_slow %x %x %d\n", cp, dp, m->m_len);
		bcopy(dp, cp, (unsigned)m->m_len);
		cp += m->m_len;
		m = m->m_next;
	}

	/*
	 * Pad out short transmits with NULL's
	 */
	if ((cc = cp - ifrw->ifrw_addr) < MINDATA) {
		bzero(cp, MINDATA - cc);
		cc = MINDATA;
	}
#endif mips
	return (cc);
}
/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
qeread(sc, ifrw, len, swloop)
	register struct qe_softc *sc;
	struct ifrw *ifrw;
	int len;
	struct mbuf *swloop;
{
	struct ether_header *eh, swloop_eh,eh1;
    	struct mbuf *m, *swloop_tmp1, *swloop_tmp2;
	struct protosw *pr;
	int off, resid;
	struct ifqueue *inq;
	static int qe_drop_no=0;
	/*
	 * Deal with trailer protocol: if type is INET trailer
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
	}

	eh = (struct ether_header *)ifrw->ifrw_addr;
	
	/* try to aviod the assignment problem due to instruction
	pipelines */

	eh1 = *eh;
	eh1.ether_type = ntohs((u_short)eh->ether_type);
	eh = &eh1;

#define	qedataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
	if ((eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) 
		) {
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
		        eh->ether_type = ntohs(*(qedataaddr((struct ether_header *)ifrw->ifrw_addr,off, u_short *)));
		        resid = ntohs(*(qedataaddr((struct ether_header *)ifrw->ifrw_addr,off+2, u_short *)));
			if (off + resid > len)
			     return;		/* sanity */
			len = off + resid;
		}
	} else {
		off = 0;
	}
	if (len == 0)
		return;

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; qeget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	if (swloop) {
		m = m_copy(swloop, 0, M_COPYALL);
		m_freem(swloop);
	} else {
		m = qeget(&sc->qeuba, ifrw, len, off);
	}

	if (m == 0)
		return;

	if (off) {
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
	}



	/*
	 * Bump up input packets and DECnet counters. Input packets for
	 * "netstat" include ALL directed, multicast, and error inputs.
	 * For DECnet, only error-free input packets are counted. See the
	 * DEUNA User's Guide for a breakdown of the counters.
	 */
	sc->is_if.if_ipackets++;
	sc->ctrblk.est_bytercvd += len ;
	if (sc->ctrblk.est_blokrcvd != (unsigned) 0xffffffff)
		sc->ctrblk.est_blokrcvd++;

	if( eh->ether_dhost[0] & 1 ) {
		sc->ctrblk.est_mbytercvd += len  ;
		if (sc->ctrblk.est_mblokrcvd != (unsigned) 0xffffffff)
			sc->ctrblk.est_mblokrcvd++;
	}
	/* Dispatch this packet */
	net_read(&(sc->qe_ed), eh, m, len, (swloop != NULL), (off != 0));
}

/*
 * Watchdog timer routine. There is a condition in the hardware that
 * causes the board to lock up under heavy load. This routine detects
 * the hang up and restarts the device.
 */
qewatch()
{
	register struct qe_softc *sc;
	register int i;
#ifdef mips
	volatile struct qedevice *addr; /* dallas */
#endif mips
#ifdef vax
	register struct qedevice *addr; /* dallas */
#endif vax
	register int s;			/* dallas */
	int inprogress=0;


	for( i=0 ; i<nNQE ; i++ ) {
		sc = qe_softc[i];
		addr = sc->addr; /* dallas */
		if( sc->timeout ) {
			if( ++sc->timeout > 3 ) {
				/*
				 * Check for the reason the transmit timed out
				 */
				if( addr->qe_csr & QE_RL_INVALID) {
					/*
					 * Transmit timed out because no recv
					 * buffers available, so must now
					 * process the recv ring.  Set a fail
					 * safe just in case we have been
					 * here before, cleared in qetint().
					 */
					if (sc->qe_rl_invalid)
						qerestart( sc );
					else {
						++sc->qe_rl_invalid;
						sc->timeout = 0;
						timeout( qewatch, 0, QE_TIMEO);	
						s = splimp();
						qerint( i );
						splx( s ); 
					}
				} else
					qerestart( sc );
			} else 
				inprogress++;
		}
	}
	if( inprogress ){
		timeout(qewatch, 0, QE_TIMEO);
		qewatchrun++;
	} else
		qewatchrun=0;
}
/*
 * Restart for board lockup problem.
 */
int qe_restarts;

qerestart( sc )
	register struct qe_softc *sc;
{
	register struct ifnet *ifp = &sc->is_if;
#ifdef mips
	volatile struct qedevice *addr = sc->addr;
#endif mips
#ifdef vax 
	register struct qedevice *addr = sc->addr;
#endif vax
	register struct qe_ring *rp;
	register int i;
	register struct ifxmt *ifxp;
	int s;	/* dallas */

	qe_restarts++;
	s = splimp();	/* Raise our priority so when we reset it does not
			 * interrupt us while we are fooling with the rings.
			 */
	addr->qe_csr = QE_RESET;
	sc->timeout = 0;
	qesetup( sc );
        addr->qe_csr &= ~QE_RESET;

	for(i = 0, rp = sc->tring; i<nNXMT ; rp++, i++ ){
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 0;
		ifxp = &sc->qeuba.ifu_w[i];
		if( ifxp->x_xtofree ) {
			m_freem( ifxp->x_xtofree );
			ifxp->x_xtofree = 0;
		}
	}
	sc->nxmit = sc->otindex = sc->tindex = sc->rindex = 0;
	if ( ifp->if_flags & IFF_LOOPBACK )
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ELOOP;
	else
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ILOOP;
	ifp->if_flags &= ~IFF_OACTIVE;
	addr->qe_rcvlist_lo = (short)sc->rringaddr;
	addr->qe_rcvlist_hi = (short)((int)sc->rringaddr >> 16);
	for( i = 0 ; sc != qe_softc[i] ; i++ )
		;
	qestart( i );
	splx( s );	/* dallas */
	if (qe_show_restarts)
		printf("qerestart: restarted qe%d %d\n", i, qe_restarts);
	else
		mprintf("qerestart: restarted qe%d %d\n", i, qe_restarts);
}
#endif
