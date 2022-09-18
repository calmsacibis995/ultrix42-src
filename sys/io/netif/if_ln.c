#ifndef lint
static char *sccsid = "@(#)if_ln.c	4.14      (ULTRIX)  4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1989 by			*
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
 * Modification History:
 *
 * 04/11/91	Randall Brown
 *	Before stopping the lance on 3min, disable DMA on I/O ASIC
 *
 * 02/24/91	jsd
 *	Allow loopback packets if COPYALL mode is set
 *
 * 01/21/91	Randall Brown
 *	Moved code for cpyin4x4 and cpyout4x4 into if_ln_copy.s for
 *	performance improvement.
 *
 * 01/01/91	Chran-Ham Chang
 *	Change the code to allow the driver promisc mode to be disabled
 *	after enabling it.
 *
 * 12/06/90	Randall Brown
 *	Fixed a bug in the 4x4() routines dealing with small mbufs not on 8
 *	boundarys.  The routines would return an incorrect lrbptr.
 *
 * 10/15/90	Randall Brown
 *	Fixed a bug in probe routine for 3min's with large memory configs.
 *	Changed a svtolance32 to svtophy.
 *
 * 8/31/90	Lea Gottfredsen
 *	Turned on cacheing for 3min, added clean_dcache calls, and added
 *	DMA memory read error interrupt check in lnintr
 *
 * 08/19/90	Fred L. Templin
 *	Fixed setting of "IFF_UP" and "IFF_RUNNING" bits.
 *
 * 7/11/90	Lea Gottfredsen
 * 	Added 3min support: 3max-like version and DMA version with funky
 *	4 word read/writes with skips in between.
 *
 * 03/26/90     Paul Grist
 *      Added support to lnprobe for mipsmate -- DS_5100.
 *
 * 12/19/89     Chran-Ham Chang
 *	Added code to fixed the panic problem when the incoming packet does 
 *	not turn on the start packet bit. 
 * 
 * 11/16/89     Chran-Ham Chang
 *      Added code to print out the station address at the system boot time.
 *
 * 11/15/89	Chran-Ham Chang
 *	Fixed the unaligned access problem for the 802.2 LLC transmit
 *      frame. 
 *
 * 11/14/89	Chran-Ham Chang
 *	Fixed 3MAX probing problem.
 *
 * 10/27/89	Uttam Shikarpur
 *	Added:
 *		1) Ability to report back the type of network interface.
 *		2) Counters to  keep track of the multicast pack., bytes sent.
 *
 * 9/22/89      Chran-Ham Chang
 *      Fixed collisions counters problem. Merged 3MAX version to the
 *      pool.
 *
 * 9/11/89      afd
 *      Bug fixes from testing on KMAX.  Set up to really run on 3MAX.
 *
 * 31/8/89      Chran-Ham Chang
 *      Added code to check the mbuf return from lnget(). In addition,
 *      Added code to restart the LANCE, if the STP bit not found for
 *      the first incoming packet.
 *
 * 8/7/89       Lea Gottfredsen
 *      Added 3max support.
 *      An interim version with 3max disguised
 *      as a PMAX in order to test a specially configured PMAX that
 *      has a 3max IO module.
 *      Also, changed multi unit handling in lnprobe, removed next.
 *
 * 7/17/89	Chran-Ham Chang
 *	Merged se DMA receiver architecture into ln driver.
 *      Changed ifnet name from se to ln.
 *	
 * 6/29/89	Lea Gottfredsen
 *	Merged packet filter and SMP back into this new 
 * 	lance driver. Merge of pu and isis pools. Multi-unit support.
 *
 * 5/2/89       Lea Gottfredsen
 *      Added lnsw structure to provide ease of multi architecture
 *      implementation and readablity.
 *	Mipsfair support.
 *
 * The following comments are a subset of the ones found in the two
 * old versions of if_ln.c and if_se.c
 *
 * 6/2/89 Uttam Shikarpur
 *      Add support for Ethernet packet filter
 *
 * 9/21/88 U. Sinkewicz
 *      Added locks for SMP.
 *
 * 6/8/88       lp
 *      PMAX
 *
 * 04-01-88     Fred L. Templin
 *      Several changes from initial version. Now up and running with
 *      Mayfair II.
 *
 * 01-04-88     templin (Fred L. Templin)
 *      Created the if_ln.c module. This module is based on
 *      a modified version of if_se.c
 *
 *  6-May-88 - 10-Feb-8	   Fred Templin
 *	Several enhancements, performance improvements and bug fixes
 *
 *  15-Feb-88 fred (Fred Canter)
 *      Changes for VAX420 (CVAXstar/PVAX) support.
 *
 *  18-Jun-86  jsd (John Dustin)
 *      Created this VAXstar network controller driver.
 *      For Ethernet Lance chip implementation.
 *      Derived from if_qe.c.
 *
 * --------------------------------------------------------------------- */
#include "ln.h"

#if     NLN > 0 || defined(BINARY)
/*
 * Digital LANCE Network Interface
 */
#include "packetfilter.h"       /* NPACKETFILTER */
#include "../data/if_ln_data.c"

extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern struct timeval time;
extern int net_output();
#ifdef vax
extern struct nexus nexus[];
#else
volatile unsigned long *rdpptr;
char *nexus;
#endif
extern int cpu;
extern struct lnsw mayfairsw[], ffoxsw[], vaxstarsw[];
typedef volatile unsigned char * pvoluchar;

int	lndebug = 0;		/* debug flag, range 0->4 */
int	lnprobe(), lnattach(), lnintr();
int	lninit(), lnstart(), lnioctl(), lnwatch();
struct	mbuf *lnget(), *lnget_dma();

u_short lnstd[] = { 0 };
struct	uba_driver lndriver =
	{ lnprobe, 0, lnattach, 0, lnstd, "ln", lninfo };

u_char ln_sunused_multi[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

unsigned long ln_crc_table[16];	/* crc initialization table */
unsigned int ln_poly = 0xedb88320;	/* polynomial initialization */

#define LRBADDR(start,off)	(((int)(start))&0x01 ? \
				  (caddr_t)((int)(start)+(off*2)+ \
				  (off%2)): \
				  ((caddr_t)((int)(start)+(off*2)- \
				  (off%2))))

/*
 * Probe the Lance to see if it's there
 */
lnprobe(reg,ui)
	caddr_t reg;
	struct uba_device *ui;   /* Only for MIPS machine */
{
	static int unit=0; 	/* Walk thru softc's only for VAX */	
	register struct ln_softc *sc;
	register struct lnsw *lnsw;	/* ptr to switch struct */
	register struct ln_initb *initb; /* LRB initb ptr */
	int prp;	 	/* physical addr ring pointer */
	int pi;			/* physical addr init block */
	int i, j, oldcvec,index;
	unsigned int tmp, x;
	unsigned short flag;
	char buf[TC_ROMNAMLEN + 1];

	KM_ALLOC(sc,struct ln_softc *, sizeof(struct ln_softc),KM_DEVBUF, KM_CLEAR);
	if (!sc)
		return(0);		
	/*
	 * CPU identifiers are found in cpuconf.h
	 */
	switch (cpu) {
		case VAX_3400:			/* mayfair II */
			ln_softc[unit] = sc;
			sc->lnsw = lnsw = mayfairsw;
			sc->rdpaddr = (struct lnreg *)(&((struct ni_regs *)reg)->ni_rdp);
			sc->rapaddr = (struct lnreg *)(&((struct ni_regs *)reg)->ni_rap);
			sc->ln_narom = (u_long *)(((struct ni_regs *)reg)->ni_sar);
			sc->ln_lrb = (u_char *)(((struct ni_regs *)reg)->ni_nilrb);
			sc->is_if.if_sysid_type = 60;
			flag = (LN_IDON | LN_INIT);
			break;
		case DS_5400:			/* mipsfair */
			unit = ui->ui_unit;
			ln_softc[unit] = sc;
			sc->lnsw = lnsw = ds5400sw;
			sc->rdpaddr = (struct lnreg *)PHYS_TO_K1(lnsw->ln_phys_rdp);
			sc->rapaddr = (struct lnreg *)PHYS_TO_K1(lnsw->ln_phys_rap);
			sc->ln_narom = (u_long *)PHYS_TO_K1(lnsw->ln_phys_narom);
			sc->ln_lrb = (pvoluchar)PHYS_TO_K1(lnsw->ln_phys_lrb);
			sc->is_if.if_sysid_type = 94;
			flag = (LN_IDON | LN_INIT);
			break;
		case DS_5000:			/* 3max */
		case DS_5000_100:		/* 3min */
			unit = ui->ui_unit;
			ln_softc[unit] = sc;
			sc->is_if.if_sysid_type = (cpu == DS_5000_100 ? 118 : 94);
			tc_addr_to_name(reg, buf);
			if (!strcmp(buf,"PMAD-AA ")) {  /* 3max like */
				sc->lnsw = lnsw = ds5000sw;
				sc->rdpaddr = (struct lnreg *)((u_long)reg + lnsw->ln_phys_rdp);
				sc->rapaddr = (struct lnreg *)((u_long)reg + lnsw->ln_phys_rap);
				sc->ln_narom = (u_long *)((u_long)reg + lnsw->ln_phys_narom);
				sc->ln_lrb = (pvoluchar)((u_long)reg + lnsw->ln_phys_lrb);
				flag = (LN_IDON | LN_INIT);
			  } else {	/* PMAD_BA, DMA version  */
				sc->lnsw = lnsw = ds3minsw;
				sc->rdpaddr = (struct lnreg *)PHYS_TO_K1(lnsw->ln_phys_rdp);
				sc->rapaddr = (struct lnreg *)PHYS_TO_K1(lnsw->ln_phys_rap);
				sc->ln_narom = (u_long *)PHYS_TO_K1(lnsw->ln_phys_narom);
				sc->ssraddr = (u_long *)PHYS_TO_K1(lnsw->ln_phys_ssr);
				sc->siraddr = (u_long *)PHYS_TO_K1(lnsw->ln_phys_sir);
				sc->ldpaddr = (u_long *)PHYS_TO_K1(lnsw->ln_phys_ldp);
				KM_ALLOC(sc->ln_lrb,pvoluchar,LN_LRB_SIZE, KM_DEVBUF, KM_CLEAR);
				if (!sc->ln_lrb) return(0);

				while (svtophy(sc->ln_lrb) & 0xffff)
				    sc->ln_lrb++;	/* 64K align */

				/* enable IOASIC to do lance DMA */
   				*(u_int *)(sc->ssraddr) |= BIT16SET;

				/* next is really svtoioasic */
				*(sc->ldpaddr) = (((svtophy(sc->ln_lrb)) & LDPBITS) << 3 );
				flag = (LN_IDON | LN_INIT);
				}
			break;
		case DS_3100:		/* pmax */
 	        case DS_5100:		/* mipsmate */
			unit = ui->ui_unit;
			ln_softc[unit] = sc;
			sc->lnsw = lnsw = pmaxsw;
			sc->rdpaddr = (struct lnreg *)PHYS_TO_K1(lnsw->ln_phys_rdp);
			sc->rapaddr = (struct lnreg *)PHYS_TO_K1(lnsw->ln_phys_rap);
			sc->ln_narom = (u_long *)PHYS_TO_K1(lnsw->ln_phys_narom);
			sc->ln_lrb = (pvoluchar)PHYS_TO_K1(lnsw->ln_phys_lrb);
			flag = (LN_IDON | LN_INIT);
			sc->is_if.if_sysid_type = (cpu == DS_3100 ? 67 : 94);
			break;
		case VAX_60:			/* firefox */
			ln_softc[unit] = sc;
			sc->lnsw = lnsw = ffoxsw;
			sc->rdpaddr = (struct lnreg *)(&((struct ni_regs *)reg)->ni_rdp);
			sc->rapaddr = (struct lnreg *)(&((struct ni_regs *)reg)->ni_rap);
			sc->ln_narom = (u_long *)(((struct ni_regs *)reg)->ni_sar);
			sc->ln_lrb = (u_char *)(((struct ni_regs *)reg)->ni_nilrb);
			flag = (LN_IDON | LN_INIT);
			sc->is_if.if_sysid_type = 39;
			break;
#ifdef vax
		case VAXSTAR:			/* vs2000 */ 
		case C_VAXSTAR:			/* pvax  */ 
			ln_softc[unit] = sc;
			sc->lnsw = lnsw = vaxstarsw;
			sc->rdpaddr = (struct lnreg *)(&(((struct nb1_regs *)qmem)->nb_ni_rdp));
			sc->rapaddr = (struct lnreg *)(&(((struct nb1_regs *)qmem)->nb_ni_rap));
			sc->ln_narom = (u_long *)(((struct nb_regs *)nexus)->nb_narom);
			sc->ln_lrb = (u_char *)( &ln_lrb[unit][0] );
	
			flag = (LN_IDON | LN_INIT | LN_INEA);
			sc->is_if.if_sysid_type = 39;
			/* system network interrupt */
			((struct nb_regs *)nexus)->nb_int_msk |= LN_INT_NP ;	
			break;
#endif vax
		default: 
			printf("lnprobe : cpu type %d unknown\n", cpu );
			return(0);
	}
	unit++; 		/* for VAX architecture multiple units */   
	sc->ln_rap = LN_CSR0;
	sc->ln_rdp = LN_STOP;

	/*
	 * start lrb_offset to point to first byte of the local RAM
	 * buffer. lnalloc bumps pointer as chunks of the buffer are
	 * allocated.
	 */
	sc->lrb_offset = 0;

	/*
	 * Initialize some per unit counters
	 */
	sc->callno = 0;

	lnshowmulti = 0;	
	lnbablcnt=0;	
	lnmisscnt=0;	
	lnmerrcnt=0;
	lnrestarts=0;	

	/*
	 * Allocate contiguous, quadword aligned (required)
	 * space for both descriptor rings. "lnalloc" takes into account
	 * the "alignment factor" for LRB sizing.
	 */
	for (i=0; i<nLNNRCV; i++) {
		sc->rring[i] = lnsw->ln_alloc(sc,sizeof(struct ln_ring), (i==0?LN_QUAD_ALIGN : 0));
		if (sc->rring[i] == NULL) {
		printf("ln%d: lnalloc: cannot alloc memory for recv descriptor rings\n", unit-1);
		return(0);
		}
	}
	for (i=0; i<nLNNXMT; i++) {
		sc->tring[i] = lnsw->ln_alloc(sc,sizeof(struct ln_ring), (i==0 ? LN_QUAD_ALIGN : 0));
		if (sc->tring[i] == NULL) {
		printf("ln%d: lnalloc: cannot alloc memory for xmit descriptor rings\n", unit-1);
		return(0);
		}
	}

	/*
	 * Allocate local RAM buffer memory for the init block
	 */
	sc->initbaddr = lnsw->ln_alloc(sc, sizeof(struct ln_initb), LN_WORD_ALIGN);
	if (sc->initbaddr == NULL) {
		printf("ln%d: lnalloc: cannot alloc memory for init block\n", unit-1);
		return(0);
	}

	/* 
	 * Initialize multicast address array. Number of active entries
	 * is driven by number of "ADDMULTI" operations. (1, initailly).
	 */
	for (i=0; i<nLNMULTI; i++) {
		sc->muse[i] = 0;
		bcopy(ln_sunused_multi,&sc->multi[i],MULTISIZE);
	}
	sc->nmulti = 0;

	/*
	 * Initialize Lance chip with init block, ln_initb
	 *
	  ln_mode;			mode word
	  ln_sta_addr;			station address
	  ln_multi_mask;		multicast address mask
	  ln_rcvlist_lo, ln_rcvlist_hi;	rcv descriptor addr
	  ln_rcvlen;			rcv length
	  ln_xmtlist_lo, ln_xmtlist_hi;	xmt descriptor addr
	  ln_xmtlen;			xmt length
	 */


	initb = &sc->ln_initb;
	initb->ln_mode = 0;	/* normal operation (mode==0) */

	/*
	 * fill out station address from the narom
	 */
	for (i = j = 0; i < 3; i++, j += 2) {
		initb->ln_sta_addr[i] = 
	   	(short)((sc->ln_narom[j]>>lnsw->ln_na_align)&0xff);
		initb->ln_sta_addr[i] |= 
	   	(short)(((sc->ln_narom[j+1]>>lnsw->ln_na_align)&0xff)<<8);
	}

	/*
	 * fill out multicast address mask
	 */
	for (i = 0; i < 4; i++) {
		initb->ln_multi_mask[i] = 0x0000;
	}

	/*
	 * initialize the multicast address CRC table,
	 * using initb as a dummy variable.
	 */
	for (index=0; !(unit-1) && index<16; index++) {
			tmp = index;
			for (j=0; j<4; j++) {
			    x = (tmp & 0x01);
			    tmp = (tmp >> 1);	/* logical shift right 1 bit */
			    if (x == 1)
				tmp = (tmp ^ ln_poly);	/* XOR */
			}
			ln_crc_table[index] = tmp;
		}
	
	/*
	 * Convert VAX Virtual to LANCE relative
	 */
	prp = lnsw->ln_svtolance(sc->rring[0]);
	initb->ln_rcvlist_lo = (short)prp & 0xffff;
	initb->ln_rcvlist_hi = 
		  (char)(((int)prp >> 16) & 0xff);
	initb->ln_rcvlen = RLEN; /* Also clears rcvresv */

	prp = lnsw->ln_svtolance(sc->tring[0]);
	initb->ln_xmtlist_lo = (short)prp & 0xffff;
	initb->ln_xmtlist_hi =
		  (char)(((int)prp >> 16) & 0xff);
	initb->ln_xmtlen = TLEN; /* Also clears xmtresv */

	lnsw->ln_cpyout(initb,sc->initbaddr,sizeof(struct ln_initb),0);
	/*
	 * get physical address of init block for Lance
	 */
	/* set-up CSR 1 */
	sc->ln_rap = LN_CSR1;
	pi = lnsw->ln_svtolance(sc->initbaddr);
	sc->ln_rdp = (short)(pi & 0xffff);

	/* set-up CSR 2 */
	sc->ln_rap = LN_CSR2;
	sc->ln_rdp = (short)(((int)pi>>16) & 0xff);	/* hi 8 bits */

	sc->ln_rap = LN_CSR0;

	/*
	 * clear IDON by writing 1, and start INIT sequence
	 */
	sc->ln_rdp = flag ;

	/* wait for init done */
	j=0;
	while (j++ < 100) {
		if ((sc->ln_rdp & LN_IDON) != 0)
			break;
		DELAY(10000);
	}
	/* make sure got out okay */
	if ((sc->ln_rdp & LN_IDON) == 0) {
		if (sc->ln_rdp & LN_ERR) {
			printf("ln%d: initialization error, csr = %04x\n",unit-1,(sc->ln_rdp & 0xffff));
		} else {
			printf("ln%d: cannot initialize Lance\n",unit-1);
		}
		return(0);		/* didn't interrupt */
	}

	/* set STOP to clear INIT and IDON (and everything else) */
	sc->ln_rdp = LN_STOP;
	return( sizeof(struct ln_initb));
}
 
/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
lnattach(ui)
 	struct uba_device *ui;

{
	register struct ln_softc *sc = ln_softc[ui->ui_unit];
	register struct ifnet *ifp = &sc->is_if;
	register int i;
	register struct sockaddr_in *sin;

        /* Initialize the lock. */
        lockinit(&sc->lk_ln_softc, &lock_device15_d);

	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "ln";
	ifp->lk_softc = &sc->lk_ln_softc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_type = IFT_ETHER;
	ifp->if_flags |= IFF_BROADCAST | IFF_DYNPROTO | IFF_NOTRAILERS;
	((struct arpcom *)ifp)->ac_ipaddr.s_addr = 0;

	/*
	 * Read the address from the prom and save it.
	 */
	for ( i=0 ; i<6 ; i++ ) {
		sc->is_addr[i] = (u_char)((sc->ln_narom[i] >> sc->lnsw->ln_na_align) & 0xff);
	}
	

	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
	ifp->if_init = lninit;
	ifp->if_output = net_output;
	ifp->if_start = lnstart;
	ifp->if_ioctl = lnioctl;
	ifp->if_timer = 0;
	ifp->if_watchdog = lnwatch; 
	ifp->if_reset = 0;
        ifp->d_affinity = ALLCPU;       /* SMP */
	bcopy("DEC LANCE Ethernet Interface", ifp->if_version, 28);
	ifp->if_version[28] = '\0';
	printf("ln%d: %s, hardware address: %s \n", ui->ui_unit,
		ifp->if_version,ether_sprintf(sc->is_addr));
	
#if NPACKETFILTER > 0
        attachpfilter(&(sc->is_ed));
#endif NPACKETFILTER > 0
	if_attach(ifp);
}

/*
 * Initialization of interface and allocation of mbufs for receive ring
 */
lninit(unit)
	int unit;
{
	register struct ln_softc *sc = ln_softc[unit];
	register struct ln_initb *initb = &sc->ln_initb;
	register struct lnsw *lnsw = sc->lnsw;
	register struct ifnet *ifp = &sc->is_if;
	int i,k;

	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;
	if (ifp->if_flags & IFF_RUNNING)
		return;

	/*
	 * We only nail down our buffer addresses on the very
	 * first call, then don't relinquish them again.
	 */
        k = splimp();
        smp_lock(&sc->lk_ln_softc, LK_RETRY);

	sc->callno++;

	/*
	 * Init the buffer descriptors and
	 * indexes for each of the lists.
	 */
	for ( i = 0 ; i < nLNNRCV ; i++ ) {
		lnsw->lninitdesc (sc, sc->rring[i], &sc->rlbuf[i],
			    (sc->callno == 1) ? LNALLOC : LNNOALLOC, i);
		/* give away rring */
		lnsw->ln_setflag(sc->rring[i], LN_OWN);
	}
	for ( i = 0 ; i < nLNNXMT ; i++ ) {
		lninitdesc (sc, sc->tring[i], &sc->tlbuf[i],
			    (sc->callno == 1) ? LNALLOC : LNNOALLOC);
	}
	sc->nxmit = sc->otindex = sc->tindex = sc->rindex = 0;
	/*
	 * Take the interface out of reset, program the vector, 
	 * enable interrupts, and tell the world we are up.
	 */
        /* s=splimp(); */  /* SMP */

	sc->ln_rap = LN_CSR0;

	if ( (ifp->if_flags & IFF_LOOPBACK)
	   && (initb->ln_mode & LN_LOOP) == 0) {
		/* if not in loopback mode, do loopback */
		initb->ln_mode &= ~LN_INTL;
		initb->ln_mode |= LN_LOOP;
                smp_unlock(&sc->lk_ln_softc);
		lnrestart(ifp);
		lninit(ifp->if_unit);
		splx(k);
		return;
	}

	/* start the Lance; enable interrupts, etc */
	sc->ln_rdp = (LN_START | LN_INEA);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	lnstart( unit );
	sc->ztime = time.tv_sec;
        smp_unlock(&sc->lk_ln_softc);
	splx(k);
}
 
/*
 * Ethernet interface interrupt processor
 */
lnintr(unit)
	int unit;
{
	register struct ln_softc *sc = ln_softc[unit];
	register struct ifnet *ifp;
	register int s, oldcsr;


	ifp = &sc->is_if;
	s = splimp();


	/*
	 * If a set-1-to-reset bit is 1, then writing a 1
	 * back into the csr register will clear that bit.
	 * This is OK; the intent is to clear the csr of all
	 * errors/interrupts and then process the saved bits
	 * in the old csr. We grab the "ln_flag" byte out of
	 * the ring entry to check the ownership bit FIRST,
	 * then grab the "ln_flag2" short word later to eliminate
	 * a timing window in which the LANCE may update the
	 * "ln_flag" before updating "ln_flag2".
	 */
	oldcsr = sc->ln_rdp;		/* save the old csr */

	oldcsr &= ~LN_INEA;	/* clear interrupt enable */

#ifdef mips
	if(cpu == DS_5000) {  /* work around for R3000 bug */
		rdpptr=	(volatile unsigned long *)&sc->ln_rdp;
		*rdpptr = oldcsr;
	}
	else 
#endif
		sc->ln_rdp = oldcsr;

	sc->ln_rdp = LN_INEA;	/* set interrupt enable */

	/*
	 * check error bits
	 */
	if ( oldcsr & LN_ERR ) {
		if (oldcsr & LN_MISS) {
			/*
			 * LN_MISS is signaled when the LANCE receiver
			 * loses a packet because it doesn't own a
			 * receive descriptor. Rev. D LANCE chips, which
			 * are no longer used, require a chip reset as
			 * described below.
			 */

			lnmisscnt++;
			if (lndebug)
				mprintf("ln%d: missed packet (MISS)\n",unit);
			if(sc->ctrblk.est_sysbuf != 0xffff)
				sc->ctrblk.est_sysbuf++;

			/* This is a toss-up.  We don't increment
			 * data overrun here because it's most likely
			 * the host's fault that packets were missed.
			 * No way to tell from here whether the LANCE
			 * is at fault.
			 *
			if (sc->ctrblk.est_overrun != 0xffff)
				sc->ctrblk.est_overrun++;
			 */
		}
		if (oldcsr & LN_CERR) {
			if (sc->ctrblk.est_collis != 0xffff)
				sc->ctrblk.est_collis++;
		}
		if (oldcsr & LN_BABL) {
			lnbablcnt++;
			if (lndebug)
			    mprintf("ln%d: transmitter timeout (BABL)\n",unit);
		}
		if (oldcsr & LN_MERR) {
			lnmerrcnt++;
			/* warn the user (printf on the terminal) */

			mprintf("ln%d: memory error (MERR) \n", unit);
			if (((oldcsr & LN_RXON) == 0)
			    || ((oldcsr & LN_TXON) == 0)) {
				lnrestart(ifp);
				lninit(ifp->if_unit);
				splx(s);
				return;
			}
		}
	}

	if ( oldcsr & LN_RINT )
		lnrint( unit );
	
	/* check for lance dma memory read error, happens on xmit only 
	 * disables DMA, times out lance, interrupts processor
	 */
	if (sc->lnsw->ln_dma) {
	        caddr_t pa;
		struct tc_memerr_status status;

		if ( *(u_int *)(sc->siraddr) & BIT16SET) { 
		        pa = (caddr_t)((*(u_int *)(sc->ldpaddr)) >> 3);
			printf("ln%d: dma memory read error \n", unit);
			status.pa = pa;
			status.va = 0;
			status.log = TC_LOG_MEMERR;
			status.blocksize = 4;
			tc_isolate_memerr(&status);
   			*(u_int *)(sc->siraddr) &= ~BIT16SET;
   			*(u_int *)(sc->ssraddr) |= BIT16SET;
			lnrestart(ifp);
			lninit(ifp->if_unit);
			lndmareaderr++;
			splx(s);
			return;
		}
	}

	if ( oldcsr & LN_TINT )
		lntint( unit );

	if (oldcsr == (LN_INTR|LN_RXON|LN_TXON))
		mprintf("ln%d: stray interrupt\n",unit);

	splx(s);
}
 
/*
 * Ethernet interface transmit interrupt.
 */
lntint(unit)
	int unit;
{
	register struct ln_softc *sc = ln_softc[unit];
	register int index;
	register int i;
	register struct ln_ring *rp = &sc->ln_ring;
	register struct lnsw *lnsw = sc->lnsw;

	struct mbuf *mp, *mp0;
	struct ifnet *ifp = &sc->is_if;
	struct ether_header *eh;
	short len;

	/*
	 * While we haven't caught up to current transmit index,
	 * AND the buffer is ours, AND there are still transmits
	 * which haven't been pulled off the ring yet, proceed
	 * around the ring in search of the last packet. We grab
	 * the "ln_flag" byte out of the ring entry to check the
	 * ownership bit FIRST, then grab the "ln_flag2" short
	 * word later to eliminate a timing window in which the
	 * LANCE may update the "ln_flag" before updating "ln_flag2".
	 */
        smp_lock(&sc->lk_ln_softc, LK_RETRY);

	lnsw->ln_cpyin(sc, sc->tring[sc->otindex], &rp->ln_flag,
		       sizeof(u_char), (u_long)&rp->ln_flag - (u_long)rp);
	while ( (sc->otindex != sc->tindex)
	 && !(rp->ln_flag & LN_OWN)
	 && sc->nxmit > 0 ) {

		index = sc->otindex;


		/*
		 * Found last buffer in the packet
		 * (hence a valid string of descriptors)
		 * so free things up.
		 */
		mp = sc->tmbuf[index];

		sc->tmbuf[index] = (struct mbuf *)NULL;

		/* increment old index pointer, if it catches up
		 * with current index pointer, we will break out
		 * of the loop.  Otherwise, go around again
		 * looking for next end-of-packet cycle.
		 */
		if (!(--sc->nxmit)) {
			ifp->if_timer = 0;
			ifp->if_flags &= ~IFF_OACTIVE;
		}
		sc->is_if.if_opackets++;

		lnsw->ln_cpyin(sc, sc->tring[sc->otindex], &rp->ln_flag2,
			       sizeof(u_short), (u_long)&rp->ln_flag2 - (u_long)rp);
		/*
		 * DECnet statistics
		 */
		/* exactly one collision? */
		if ((rp->ln_flag & LN_ONE) && !(rp->ln_flag2 & LN_LCOL)) {
			sc->is_if.if_collisions++;
			if (sc->ctrblk.est_single != (unsigned)0xffffffff)
				sc->ctrblk.est_single++;
		/* more than one collision? */
		} else if (rp->ln_flag & LN_MORE) {
			sc->is_if.if_collisions += 2;
			if (sc->ctrblk.est_multiple != (unsigned)0xffffffff)
				sc->ctrblk.est_multiple++;
		}

		/*
		 * Check for transmission errors.
		 * This assumes that transmission errors
		 * are always reported in the last packet.
		 */
		if (rp->ln_flag & LN_RT_ERR) {
			sc->is_if.if_oerrors++;
			if (sc->ctrblk.est_sendfail != 0xffff) {
				sc->ctrblk.est_sendfail++;
				if (rp->ln_flag2 & LN_RTRY) {
					/* excessive collisions */
					if (lndebug) mprintf("ln%d: excessive collisions (RTRY)\n",unit);
					sc->ctrblk.est_sendfail_bm |= 1;
				}
				if (rp->ln_flag2 & LN_LCOL) {
					if (lndebug) mprintf("ln%d: late transmit collision (LCOL)\n",unit);
					; /* not implemented */
				}
				if (rp->ln_flag2 & LN_LCAR) {
					if (lndebug) mprintf("ln%d: lost carrier during transmit (LCAR)\n",unit);
					sc->ctrblk.est_sendfail_bm |= 2;
				}
				if (rp->ln_flag2 & LN_UFLO) {
					if (lndebug) mprintf("ln%d: packet truncated (UFLO)\n,unit");
				}
				if (rp->ln_flag2 & LN_TBUFF) {
					if (lndebug) mprintf("ln%d: transmit buffer error (BUFF)\n,unit");
				}
			}
			/*
			 * Restart chip if transmitter got turned off
			 * due to transmit errors: UFLO, TBUFF or RTRY.
			 */
			if (rp->ln_flag2 & (LN_UFLO | LN_TBUFF | LN_RTRY)) {
                                smp_unlock(&sc->lk_ln_softc);
				/*
				 * Need to free mp here, since we've
				 * already destroyed its copy in the
				 * "tmbuf" list.
				 */
				m_freem(mp);
				lnrestart(ifp);
				lninit(ifp->if_unit);
				return;
			}
			m_freem(mp);
		} else {
			/*
			 * If this was a broadcast packet or if the
			 * interface is in COPYALL mode, loop it
			 * back, otherwise just free the packet.
			 */
			if (mp) {
				eh = mtod( mp, struct ether_header *);
				for (i=0; (i < 6) && (eh->ether_dhost[i] == 0xff); i++)
					; /*nop*/
				if ( (i == 6) ||
				   (ifp->if_flags & IFF_PFCOPYALL)) {
					for ( mp0 = mp, len=0 ; mp0 ; mp0 = mp0->m_next ) {
						len += mp0->m_len;
					}
					/* Bump up DECnet statistics */
					if(eh->ether_dhost[0] & 1) {
						sc->ctrblk.est_mbytesent += len;
						if (sc->ctrblk.est_mbloksent != (unsigned) 0xffffffff)
							sc->ctrblk.est_mbloksent++;
					}
					lnread( sc, 0, len, mp, 0 );
				} else {
					m_freem( mp );
				}
			}
		}
		/*
		 * Init the buffer descriptor
		 */
		lninitdesc(sc, sc->tring[index], &sc->tlbuf[index], LNNOALLOC);
		sc->otindex = ++index % nLNNXMT;
		lnsw->ln_cpyin(sc, sc->tring[sc->otindex],&rp->ln_flag,
			       sizeof(u_char), (u_long)&rp->ln_flag - (u_long)rp); /* for the next time thru while */
	}
	/*
	 * Dequeue next transmit request, if any.
	 */
	if (!(ifp->if_flags & IFF_OACTIVE))
		lnstart( unit );
        smp_unlock(&sc->lk_ln_softc);
}
 
/*
 * Ethernet interface receiver interrupt.
 * If can't determine length from type, then have to drop packet.  
 * Othewise decapsulate packet based on type and pass to type specific 
 * higher-level input routine.
 */
lnrint(unit)
	int unit;
{
	register struct ln_softc *sc = ln_softc[unit];
	register struct ln_ring *rp = &sc->ln_ring;
	register struct lnsw *lnsw = sc->lnsw;
	register int index, len;
	register struct ifnet *ifp = &sc->is_if;

	int first;
	u_char flag;	/* saved status flag */
	int ring_cnt;
#ifdef lint
	unit = unit;
#endif

	/*
	 * Traverse the receive ring looking for packets to pass back.
	 * The search is complete when we find a descriptor that is in
	 * use (owned by Lance). 
	 */
        smp_lock(&sc->lk_ln_softc, LK_RETRY);

	lnsw->ln_cpyin(sc, sc->rring[sc->rindex],&rp->ln_flag,
		       sizeof(u_char),(u_long)&rp->ln_flag - (u_long)rp);
	for ( ; !(rp->ln_flag & LN_OWN);
	      sc->rindex = ++index % nLNNRCV, 
	lnsw->ln_cpyin(sc, sc->rring[sc->rindex],&rp->ln_flag,
		       sizeof(u_char), (u_long)&rp->ln_flag - (u_long)rp)) {
		first = index = sc->rindex;
		

		/*
		 * If not the start of a packet, error
		 */
		if ( !(rp->ln_flag & LN_STP)) {
			if (lndebug) {
				mprintf("ln%d: recv: start of packet expected #%d, flag=%02x\n",
		unit, index,(rp->ln_flag&0xff));
			}
			lnsw->ln_setflag(sc->rring[first], LN_OWN);
                	smp_unlock(&sc->lk_ln_softc);
			lnrestart(ifp);
			lninit(ifp->if_unit);
			return;
		}

		/*
		 * Find the index of the last descriptor in this
		 * packet (LN_OWN clear and LN_ENP set). If cannot
		 * find it then Lance is still working.
		 */
		ring_cnt=1;
		while (((rp->ln_flag &
		         (LN_RT_ERR|LN_OWN|LN_ENP)) == 0) &&
			 (ring_cnt++ <= nLNNRCV)) {
			index = ++index % nLNNRCV;
			lnsw->ln_cpyin(sc, sc->rring[index],&rp->ln_flag,
				       sizeof(u_char),(u_long)&rp->ln_flag - (u_long)rp);
		}

		/*
		 * more than one; re-init the descriptors involved and
		 * ignore this packet. (May have to bump up DECnet counters).
		 */
		if (ring_cnt > 1) {
			/*
			 * Went all the way around, wait
			 */
			if (ring_cnt > nLNNRCV) {
				if (lndebug)
					mprintf("ln%d: recv ring_cnt exceeded\n",unit);
				break;
			}

			/*
			 * "free" up the descriptors, but really just use
			 * them over again.  Give them back to the Lance
			 * using the same local RAM buffers. IF ERROR INFO.
			 * IS SET, WAIT!
			 */
			if (!(rp->ln_flag & LN_RT_ERR)) {
				lnsw->ln_setflag(sc->rring[first], LN_OWN);
				while ( first != index ) {
					first = ++first % nLNNRCV;
					lnsw->ln_setflag(sc->rring[first], LN_OWN);
				}
				if (lndebug)
					mprintf("ln%d: chained receive packet dropped\n",unit);
				continue;
			}
		}

		/*
		 * Is it a valid descriptor, ie.
		 * we own the descriptor (LN_OWN==0),
		 * and end of packet (ENP) set?
		 */
		if ( (rp->ln_flag & ~LN_STP) == LN_ENP) {
			lnsw->ln_cpyin(sc, sc->rring[index],&rp->ln_flag2,
				       sizeof(u_short),(u_long)&rp->ln_flag2 - (u_long)rp);
			/* len here includes the 4 CRC bytes */
			len = (rp->ln_flag2 & LN_MCNT);
			lnread(sc, sc->rlbuf[index], len-4, 0, index);

			/*
			 * If we're on an architecture which provides DMA,
			 * re-initialize the descriptor to get a new buf.
			 */
			if (lnsw->ln_dodma)
				lnsw->lninitdesc (sc, sc->rring[index],
					&sc->rlbuf[index], LNNOALLOC,index);

			/*
			 * "free" up the descriptors. Turn ownership back
			 * back to LANCE.
			 */
			lnsw->ln_setflag(sc->rring[first], LN_OWN);

		/*
		 * else Not a good packet sequence, check for receiver errors
		 */
		} else if ((flag = rp->ln_flag) & LN_RT_ERR){
			sc->is_if.if_ierrors++;

			if (flag & (LN_OFLO | LN_CRC | LN_FRAM | LN_RBUFF)) {
				if (lndebug)
					mprintf("ln%d: recv err %02x\n",unit, flag&0xff);
				if (sc->ctrblk.est_recvfail != 0xffff) {
					sc->ctrblk.est_recvfail++;
					if (flag & LN_OFLO) {
						sc->ctrblk.est_recvfail_bm |= 4;
#if NPACKETFILTER > 0
                                           	sc->is_ed.ess_missed++;
#endif NPACKETFILTER > 0
					}
					if (flag & LN_CRC) {
				    		sc->ctrblk.est_recvfail_bm |= 1;
					}
					if (flag & LN_FRAM) {
					    	sc->ctrblk.est_recvfail_bm |= 2;
					}
					if (flag & LN_RBUFF) {
				    		; /* causes LN_OFLO to be set */
					}
				}
			} else {
				if (lndebug)
					mprintf("ln%d: stray recv err %02x\n",unit,flag&0xff);
			}
			/*
			 * "free" up the descriptors, but really just use
			 * them over again.  Give them back to the Lance
			 * using the same pre-allocated mbufs.
			 */

			lnsw->ln_setflag(sc->rring[first], LN_OWN);
			while ( first != index ) {
				first = ++first % nLNNRCV;
				/* give away */
				lnsw->ln_setflag(sc->rring[first], LN_OWN);
			}
		} else {
			/*  else:
			 *   - not a good packet, 
			 *   - not an error situation,
			 *
			 * This can happen if we beat the Lance to the
			 * end of a valid list of receive buffers -- the
			 * Lance hasn't relinquished the last buffer (one
			 * with ENP set) or we just found a buffer still
			 * owned by Lance, without finding the ENP bit.
			 * In either case, just return.  We can pick up
			 * the unfinished chain on the next interrupt.
			 */
                        smp_unlock(&sc->lk_ln_softc);
			return;
		}
	}
        smp_unlock(&sc->lk_ln_softc);
}

/*
 * Start output on interface.
 *
 */
/* SMP - call with device lock already set.  us.  */

lnstart(unit)
int	unit;
{
	register struct ln_softc *sc = ln_softc[unit];
	register struct mbuf *m;
	register int dp; 	/* data buffer pointer */
	register struct ln_ring *rp = &sc->ln_ring;
	register struct lnsw *lnsw = sc->lnsw;
	int tlen;
	int index;
	register struct ifnet *ifp = &sc->is_if;

        /* s = splimp(); */  /* SMP */

	for (index = sc->tindex;
	     sc->nxmit < (nLNNXMT - 1);
	     sc->tindex = index = ++index % nLNNXMT) {

		/*
		 * Dequeue the transmit request, if any.
		 */
		IF_DEQUEUE(&sc->is_if.if_snd, m);
		if (m == 0) {
			return;		/* Nothing on the queue	*/
		}
	
		/*
		 * "lnput" pads out anything less than "MINDATA" with NULL's
		 */
		tlen = lnput(sc, index, m);

		dp = lnsw->ln_svtolance(sc->tlbuf[index]);
		rp->ln_buf_len = -(tlen) | 0xf000;
		rp->ln_addr_lo = (short)dp & 0xffff;
		rp->ln_addr_hi =
			         (short)(((int)dp >> 16) & 0xff);
		rp->ln_flag = ( LN_STP | LN_OWN | LN_ENP);
		lnsw->ln_cpyout(rp,sc->tring[index],sizeof(struct ln_ring),0);
		sc->ln_rap = LN_CSR0;
		sc->ln_rdp = ( LN_TDMD | LN_INEA );
		sc->nxmit++;
		ifp->if_flags |= IFF_OACTIVE;
		/*
		 * Accumulate statistics for DECnet
		 */
		if ((sc->ctrblk.est_bytesent + tlen) > sc->ctrblk.est_bytesent)
			sc->ctrblk.est_bytesent += tlen;
		if (sc->ctrblk.est_bloksent != (unsigned)0xffffffff)
			sc->ctrblk.est_bloksent++;
		sc->is_if.if_timer = 5;
	}
}

/*
 * Process an ioctl request.
 */
lnioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ln_softc *sc = ln_softc[ifp->if_unit];
	register struct ln_initb *initb = &sc->ln_initb;
	register int i;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	struct ctrreq *ctr = (struct ctrreq *)data;
	struct protosw *pr;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int bitpos;		/* top 6 bits of crc = bit in multicast mask */
	u_short newmask[4];	/* new value of multicast address mask */
	int j = -1, s, error=0;

	s = splimp();
	switch (cmd) {

	case SIOCENABLBACK:
		if (lndebug>1) printf("SIOCENABLBACK ");
		printf("ln: internal loopback requested\n");

                smp_lock(&sc->lk_ln_softc, LK_RETRY);
		/* set external loopback */
		initb->ln_mode &= ~LN_INTL;
		initb->ln_mode |= LN_LOOP;
                smp_unlock(&sc->lk_ln_softc);
		lnrestart( ifp );
		ifp->if_flags |= IFF_LOOPBACK;
		lninit( ifp->if_unit );
		break;
 
	case SIOCDISABLBACK:
		if (lndebug>1) printf("SIOCDISABLBACK ");
		printf("ln: internal loopback disable requested\n");

                smp_lock(&sc->lk_ln_softc, LK_RETRY);
		/* disable external loopback */
		initb->ln_mode &= ~LN_INTL;
		initb->ln_mode &= ~LN_LOOP;
                smp_unlock(&sc->lk_ln_softc);
		lnrestart( ifp );
		ifp->if_flags &= ~IFF_LOOPBACK;
		lninit( ifp->if_unit );
		break;
 
	case SIOCRPHYSADDR:
		/*
		 * read default hardware address.
		 */
		if (lndebug>1) printf("SIOCRPHYSADDR ");
                smp_lock(&sc->lk_ln_softc, LK_RETRY);
		bcopy(sc->is_addr, ifd->current_pa, 6);
		for ( i=0 ; i<6 ; i++ ) {
			ifd->default_pa[i] =
			(u_char)((sc->ln_narom[i]>>sc->lnsw->ln_na_align)&0xff);
		}
                smp_unlock(&sc->lk_ln_softc);
		break;
 
	case SIOCSPHYSADDR:
		if (lndebug>1) printf("SIOCSPHYSADDR ");
                smp_lock(&sc->lk_ln_softc, LK_RETRY);
		bcopy(ifr->ifr_addr.sa_data, sc->is_addr, 6);
#ifdef NPACKETFILTER > 0
                pfilt_newaddress(sc->is_ed.ess_enetunit, sc->is_addr);
#endif NPACKETFILTER > 0


		/*
		 * fill in the initb station address, and restart.
		 */
		for (i=j=0; i < 3; i++, j += 2) {
			initb->ln_sta_addr[i] = 
				(short)(sc->is_addr[j]&0xff);
			initb->ln_sta_addr[i] |= 
				(short)((sc->is_addr[j+1]&0xff)<<8);
		}
	        smp_unlock(&sc->lk_ln_softc);

		if (ifp->if_flags & IFF_RUNNING) {
			lnrestart(ifp);
			lninit(ifp->if_unit);
		} else {
			lnrestart(ifp);
		}
		break;

	case SIOCDELMULTI:
	case SIOCADDMULTI:

                smp_lock(&sc->lk_ln_softc, LK_RETRY);
		if (cmd == SIOCDELMULTI) {
			if (lndebug>1) printf("SIOCDELMULTI ");
			for (i = 0; i < sc->nmulti; i++)
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					if (--sc->muse[i] == 0) {
						bcopy(ln_sunused_multi,&sc->multi[i],MULTISIZE);
					}
					if (lnshowmulti) printf("%d deleted.\n",i);

				}
		} else {
			if (lndebug>1) printf("SIOCADDMULTI ");
			for (i = 0; i < sc->nmulti; i++) {
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					sc->muse[i]++;
					if (lnshowmulti) printf("already using index %d\n", i);
                			smp_unlock(&sc->lk_ln_softc);
					goto done;
				}
				if (bcmp(&sc->multi[i],ln_sunused_multi,MULTISIZE) == 0)
					j = i;
			}
			/*
			 * j is initialized to -1; if j > 0, then
			 * represents the last valid unused location
			 * in the multicast table.
			 */
			if (j == -1) {
				if (sc->nmulti == nLNMULTI) {
					printf("ln: SIOCADDMULTI failed, multicast list full: %d\n",nLNMULTI);
					error = ENOBUFS;
                			smp_unlock(&sc->lk_ln_softc);
					goto done;
				} else {
					j = sc->nmulti++;
				}

			}
			bcopy(ifr->ifr_addr.sa_data, &sc->multi[j], MULTISIZE);
			sc->muse[j]++;

			if (lnshowmulti)
				printf("added index %d.\n", j);
		}
		/*
		 * Recalculate all current multimask crc/bits
		 * and reload multimask info.
		 *
		 * For each currently used multicast address,
		 * calculate CRC, save top 6 bits, load
		 * appropriate mask bit into newmask[i]
		 */
		for (i=0; i<4; i++)
			newmask[i] = 0x0000;

		for (i=0; i<sc->nmulti; i++) {
			if (sc->muse[i] == 0)
				continue;
			/* returns 32-bit crc in global variable _ln_crc */
			ln_docrc(&sc->multi[i], 0, sc);
			bitpos = ((unsigned int)sc->ln_crc >> 26) & 0x3f;
			if (lnshowmulti)
				printf("crc=%x, bit=%d.\n",sc->ln_crc,bitpos);

			/* 0-15 */
			if (bitpos >= 0 && bitpos < 16)
				newmask[0] |= (1 << (bitpos - 0));
			/* 16-31 */
			else if (bitpos < 32)
				newmask[1] |= (1 << (bitpos - 16));
			/* 32-47 */
			else if (bitpos < 48)
				newmask[2] |= (1 << (bitpos - 32));
			/* 48-63 */
			else if (bitpos < 64)
				newmask[3] |= (1 << (bitpos - 48));
			else {
				if (lndebug || lnshowmulti)
					printf("ln: bad crc, bitpos=%d.\n", bitpos);
			}
		}
		i = 0;
		for (i = 0; i < 4; i++)
			initb->ln_multi_mask[i] =
			                           newmask[i] & 0xffff;
		if (lnshowmulti) {
		    printf("new 64-bit multimask= %04x %04x %04x %04x\n",
			newmask[0], newmask[1], newmask[2], newmask[3]);
		}
                smp_unlock(&sc->lk_ln_softc);

		if (ifp->if_flags & IFF_RUNNING) {
			lnrestart(ifp);
			lninit(ifp->if_unit);
		} else {
			lnrestart(ifp);
		}
		break;

	case SIOCRDCTRS:
	case SIOCRDZCTRS:

		if (lndebug>1) printf("SIOCRDCTRS ");
		ctr->ctr_ether = sc->ctrblk;
		ctr->ctr_type = CTR_ETHER;
		ctr->ctr_ether.est_seconds = (time.tv_sec - sc->ztime) > 0xfffe ? 0xffff : (time.tv_sec - sc->ztime);
		if (cmd == SIOCRDZCTRS) {
			if (lndebug>1) printf("SIOCRDZCTRS ");
                	smp_lock(&sc->lk_ln_softc, LK_RETRY); 
			sc->ztime = time.tv_sec;
			bzero(&sc->ctrblk, sizeof(struct estat));
                	smp_unlock(&sc->lk_ln_softc); 

		}
		break;

	case SIOCSIFADDR:
		if (lndebug>1) printf("SIOCSIFADDR ");
		if (!(ifp->if_flags & IFF_RUNNING)) {
			lninit(ifp->if_unit);
			ifp->if_flags |= IFF_UP;
		}
		switch(ifa->ifa_addr.sa_family) {
#ifdef INET
		case AF_INET:
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			/*
			 * DISABLE
			 */
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
        case SIOCSIFFLAGS:
		if (lndebug > 1) printf("SIOCSIFFLAGS ");
		/*
		 * If promisuous mode is enabled/disabled restart
		 * to change LANCE mode
	 	 */
                smp_lock(&sc->lk_ln_softc, LK_RETRY);
		if ((ifp->if_flags & IFF_PROMISC) &&
		    !(initb->ln_mode & LN_PROM)) {

			/* should be in promiscuous mode */
			initb->ln_mode |= LN_PROM;
		} else if (!(ifp->if_flags & IFF_PROMISC)) {

			/* should not be in promiscuous mode */
			initb->ln_mode &= ~LN_PROM;
		} 
                smp_unlock(&sc->lk_ln_softc); 
		if (ifp->if_flags & IFF_RUNNING) {
			lnrestart(ifp);
			lninit(ifp->if_unit);
		} else {
			lnrestart(ifp);
		}
		break;

	default:
		error = EINVAL;
	}
done:	splx(s);
	return (error);
}

/*
 * Calculate 32-bit CRC (AUTODIN-II) for the given 48-bit
 * multicast address.  The CRC table must first be initialized
 * for the vax crc instruction to work.  The crc is returned in
 * variable ln_crc.
 */
ln_docrc(addr, flag, sc)
	struct ln_multi *addr;
	int flag;
	struct ln_softc *sc;
{
	/*
	 * NOTE: do not change the order of these
	 * register declarations due to asm() instruction
	 * used below.
	 */
	register unsigned long *tbl_addr;	/* initialization table addr */
	register int inicrc;			/* initial CRC */
	register int len;			/* length of multicast addr */
	register struct ln_multi *multi_addr;	/* multicast address, 48 bits */
	register int *ln_crc = &sc->ln_crc;

	/* initialize polynomial table, only done once from lnprobe() */

	if (lnshowmulti) {
		printf("addr=%x.%x.%x.%x.%x.%x, ",
			(struct ln_multi *)addr->ln_multi_char[0],
			(struct ln_multi *)addr->ln_multi_char[1],
			(struct ln_multi *)addr->ln_multi_char[2],
			(struct ln_multi *)addr->ln_multi_char[3],
			(struct ln_multi *)addr->ln_multi_char[4],
			(struct ln_multi *)addr->ln_multi_char[5]);
	}

	/* initialize arguments */
	tbl_addr = ln_crc_table;
	inicrc = -1;
	len = 6;
	multi_addr = addr;

#ifdef lint
	tbl_addr = tbl_addr;
	inicrc = inicrc;
	len = len;
	multi_addr = multi_addr;
#endif

#ifdef vax
	/* calculate CRC */
	asm( "crc	(r11),r10,r9,(r8)" );
	asm( "movl	r0, (r7)" );
#else 
	/* Need to calculate CRC */
	*ln_crc = crc256(multi_addr, len);
#endif vax 

	return(0);
}


int  crctbl[256] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

crc256(addr, len)
u_char *addr;
int len;
{
	register unsigned int crc;

	crc = (u_long)0xffffffffL;
	while(len--)
                crc = (crc >> 8) ^ crctbl[*addr++] ^ crctbl[crc&0xff];

	return(crc);
}


/*
 * Restart the Lance chip -- this routine is called:
 *   - after changing the multicast address filter mask
 *   - on any loopback mode state change
 *   - on any error which disables the transmitter
 *   - on all missed packet errors
 *
 * The routine first halts the Lance chip, clears any previously
 * allcated mbufs, and then re-initializes the hardware (same as in
 * lnprobe() routine).  The lninit() routine is usually called next
 * to reallocate mbufs for the receive ring, and then actually
 * start the chip running again.
 */
lnrestart( ifp )
	register struct ifnet *ifp;
{
	register struct ln_softc *sc = ln_softc[ifp->if_unit];
	register char *initb = sc->initbaddr;
	register int i, pi;
	int s;

	/*
	 * stop the chip
	 */
        s = splimp();
        smp_lock(&sc->lk_ln_softc, LK_RETRY);

	/* disable DMA before setting STOP bit in case a DMA transaction
	 * had been started.
	 */
	if (sc->lnsw->ln_dma) {
	    *(u_int *)(sc->ssraddr) &= ~BIT16SET;
	}
	sc->ln_rap = LN_CSR0;
	sc->ln_rdp = LN_STOP;

	/* renable DMA in I/O ASIC */
	if (sc->lnsw->ln_dma) {
	    *(u_int *)(sc->ssraddr) |= BIT16SET;
	}

	/*
	 * stop network activity
	 */
	ifp->if_flags &= ~IFF_RUNNING;
	lnrestarts++;

	if (lndebug)
		mprintf("lnrestart: restarted ln%d  %d\n",ifp->if_unit,lnrestarts);

	/*
	 * free up any mbufs currently in use
	 */
	for (i=0; i<nLNNXMT; i++) {
		if (sc->tmbuf[i])
			m_freem(sc->tmbuf[i]);
		sc->tmbuf[i] = (struct mbuf *)NULL;
	}

	if ( sc->lnsw->ln_dodma ) {
	/*
	 * free up any mbufs currently in use
	 */
		for (i=0; i<nLNNRCV; i++) {
			if (sc->rmbuf[i])
				m_freem(sc->rmbuf[i]);
			sc->rmbuf[i] = (struct mbuf *)NULL;
		}
	}

	/*
	 * reload Lance with init block
	 */
	sc->lnsw->ln_cpyout(&sc->ln_initb,initb,sizeof(struct ln_initb),0);
	sc->ln_rap = LN_CSR1;
	pi = sc->lnsw->ln_svtolance(initb);
	sc->ln_rdp = (short)(pi & 0xffff);
	sc->ln_rap = LN_CSR2;
	sc->ln_rdp = (short)(((int)pi>>16) & 0xff);

	/*
	 * clear IDON, and INIT the chip
	 */
	sc->ln_rap = LN_CSR0;
	sc->ln_rdp = (LN_IDON | LN_INIT | LN_INEA);

        smp_unlock(&sc->lk_ln_softc);

	i = 0;
	while (i++ < 100) {
		if ((sc->ln_rdp & LN_IDON) != 0) {
			break;
		}
		DELAY(10000);
	}
        smp_lock(&sc->lk_ln_softc, LK_RETRY);

	/* make sure got out okay */
	if ((sc->ln_rdp & LN_IDON) == 0) {
		if (sc->ln_rdp & LN_ERR)
			mprintf("lnrestart: initialization ln%d ERR csr0=%04x\n",
				ifp->if_unit,(sc->ln_rdp & 0xffff));
		else
			mprintf("lnrestart: cannot initialize ln%d\n",ifp->if_unit);
	}

	if (sc->lnsw->ln_dma) {
	    *(u_int *)(sc->ssraddr) &= ~BIT16SET;
	}
	/* set STOP to clear INIT and IDON */
	sc->ln_rdp = LN_STOP;

	if (sc->lnsw->ln_dma) {
	    *(u_int *)(sc->ssraddr) |= BIT16SET;
	}
        smp_unlock(&sc->lk_ln_softc);
        splx(s);

	return(0);
}

/*
 * Initialize a ring descriptor. Tie the buffer pointed to by
 * "dp" to the descriptor.
 */
lninitdesc (sc, desc, buf, option)
	register struct ln_softc *sc;
	register struct ln_ring *desc;
	register char **buf;
	int option;
{
	register int dp;		/* data pointer */
	register struct lnsw *lnsw = sc->lnsw;
	struct ln_ring *ring = &sc->ln_ring;

	/*
	 * clear the entire descriptor
	 */
	lnsw->ln_bzero(desc, sizeof(struct ln_ring));

	/*
	 * Perform the necessary allocation/deallocation
	 */
	if (option == LNALLOC) {
		*buf = lnsw->ln_allocb (sc, LN_BUF_SIZE, 0);
		if ( *buf == 0 ) {
			if (lndebug)
			    printf("ln: can't alloc space for buffers\n");
			return(-1);
		}
	}
	dp = lnsw->ln_svtolance(*buf);
					/* +4 for CRC */
	ring->ln_flag = 0;
	ring->ln_flag2 =0;
	ring->ln_buf_len = -(ETHERMTU + sizeof(struct ether_header) + 4);
	ring->ln_addr_lo = (short)dp & 0xffff;
	ring->ln_addr_hi =
		(short)(((int)dp >> 16) & 0xff);
	lnsw->ln_cpyout(ring,desc,sizeof(struct ln_ring),0);
	return(0);
}
/*
 * Initialize a ring descriptor for architectures which provide DMA.
 * 
 */
lninitdesc_dma (sc, desc, buf, option, index )
	register struct ln_softc *sc;
	register struct ln_ring *desc;
	register char **buf;
	int option, index;
{
	register struct lnsw *lnsw = sc->lnsw;
        register struct mbuf *m=0, *p;
        int dp, dp1;                /* data pointer */
	struct ln_ring *ring = &sc->ln_ring;


	/*
	 * clear the entire descriptor
	 */
	lnsw->ln_bzero(desc, sizeof(struct ln_ring));

	/*
	 * Perform the necessary allocation/deallocation
	 */
	if (option == LNALLOC) {
		*buf = lnsw->ln_allocb(sc, LN_BUF_SIZE, 0);
		if ( *buf == 0 ) {
			if (lndebug)
			    printf("ln: can't alloc space for buffers\n");
			return(-1);
		}
	}

	/* only for dma receive side */
	MGET(m, M_DONTWAIT, MT_DATA);
       	if ( m ) {
        	MCLGET(m,p);
       		if ( p ) {
			dp  = svtophy(mtod(m,caddr_t));
			dp1 = svtophy(mtod(m,caddr_t)+LN_BUF_SIZE); 

      			/* check for physical continguous & less than 16M */
     			if (!(dp1 & 0xff000000 ) && ((dp1 - dp) == LN_BUF_SIZE )) {
        			sc->dma[index]=1;
				m->m_off += 2; 
				dp = dp+2;
        		} else {
               			dp = svtophy((caddr_t)sc->rlbuf[index]);
        			sc->dma[index]=0;
       			}
			/* For both DMA and Non-DMA */
        		sc->rmbuf[index]=m;
		} else {
   			if (lndebug)
              			printf("ln: no mbuf clusters\n");
               		m_freem(m);
               		sc->rmbuf[index] = 0;
               		dp = svtophy((caddr_t)sc->rlbuf[index]);
        		sc->dma[index]=0;
		}	
	} else {
               	if (lndebug)
        		printf("ln: no mbuf\n");
               	sc->rmbuf[index] = 0;
               	dp = svtophy((caddr_t)sc->rlbuf[index]);
        	sc->dma[index]=0;
	}
					
	desc->ln_flag = 0;
	desc->ln_flag2 =0;
	desc->ln_buf_len = -(ETHERMTU + sizeof(struct ether_header) + 4);
	desc->ln_addr_lo = (short)dp & 0xffff;
	desc->ln_addr_hi =
		(short)(((int)dp >> 16) & 0xff);
	return(0);
}
/*
 * Put an mbuf chain into the appropriate RAM buffer.
 */
lnput(sc, index, m)
	struct ln_softc *sc;
	int index;
	register struct mbuf *m;
{
	register caddr_t dp;
	register caddr_t lrbptr = sc->tlbuf[index];
	register struct lnsw *lnsw = sc->lnsw;
	int len = 0;

	sc->tmbuf[index] = m;	/* mbuf chain to free on xmit */
	while (m) {
		if (m->m_len == 0)
			goto next_m;
		dp = mtod(m, char *);
		lrbptr = lnsw->ln_cpyoutb(dp,lrbptr,(unsigned)m->m_len,0);
next_m:	
		len += m->m_len;
		m = m->m_next;
	}

	if (len < MINDATA) {
                lnsw->ln_bzerob(lrbptr, MINDATA - len);
		len = MINDATA;
	}
	return (len);
}

/*
 * Pull read data off an interface. (For DMA architectures).
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We first copy all the normal data into a cluster then deal 
 * with trailer.
 */
struct mbuf *
lnget_dma(sc,rlbuf, totlen, off0,index)
	struct ln_softc *sc ;
	register caddr_t rlbuf;
	int totlen,index,off0;
{
        register caddr_t lrbptr,dp;
	register int dma = sc->dma[index] ; 
        register struct lnsw *lnsw = sc->lnsw;
        struct mbuf *top, **mp, *m=0;
        int off = off0, len, tlen=0;

        top = 0;
        mp = &top;

        if (dma)
                lrbptr=rlbuf=mtod(sc->rmbuf[index],caddr_t);
        else
                lrbptr=rlbuf;

	while (totlen > 0) {
                len = totlen;
                if ( off ) {
                        MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
                                m_freem(top);
                                return (0);
                                }
			len = totlen - off;
			lrbptr = rlbuf + off;
                } else {
                        /* if non dma use the mbuf we already allocated */
			if ( !dma ) {
                        	m=sc->rmbuf[index];
                        	m->m_len = len;
                       		 goto copy;
                        }
		}
nopage:
                m->m_len = MIN(MLEN, len);
                m->m_off = MMINOFF;
copy:
		lrbptr = lnsw->ln_cpyinb(sc, lrbptr,
				mtod(m, caddr_t),(unsigned)m->m_len,0);
next_m:
                *mp = m;
                mp = &m->m_next;
                if (off) {
                        off += m->m_len;
                        if (off == totlen) {
                                lrbptr = rlbuf;
                                off = 0;
                                if ( dma ) {
                                        *mp = sc->rmbuf[index];
                                        (*mp)->m_len=off0;
                                        totlen = 0;
                                } else
                                        totlen = off0;
                        }
                } else
                        totlen -= m->m_len;
        }
        return (top);
}

/*
 * Pull read data off an interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We first copy all the normal data into a cluster then deal 
 * with trailer.
 */
struct mbuf *
lnget(sc,rlbuf, totlen, off, index)
	struct ln_softc *sc;
	caddr_t rlbuf;
	int totlen,index;
	register int off;
{
	struct mbuf *top, **mp, *m, *tp=0;
	int len;
	register caddr_t lrbptr = rlbuf;
	register struct lnsw *lnsw = sc->lnsw;

	top = 0;
	mp = &top;
	
	if (off) {
		struct mbuf *pp;

		MGET(tp, M_DONTWAIT, MT_DATA);
		if (tp == 0) {
			return (0);
		}

		MCLGET(tp, pp);
		if (pp == 0)
			return (0);
		tp->m_len = off;
		lrbptr = lnsw->ln_cpyinb(sc, lrbptr,mtod(tp,caddr_t),(unsigned)off,0);
		totlen -= off;
	}
		
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			m_freem(top);
			m_freem(tp);
			return (0);
		}
		len = totlen;
		if (len >= 512) {		
			struct mbuf *p;

			MCLGET(m, p);
			if (p == 0)
				goto nopage;
			m->m_len = len;
				goto copy;
		}
nopage:
		m->m_len = MIN(MLEN, len);
		m->m_off = MMINOFF;
copy:
		lrbptr = lnsw->ln_cpyinb(sc, lrbptr,
			mtod(m, caddr_t),(unsigned)m->m_len, 0);
next_m:
		*mp = m;
		mp = &m->m_next;
		totlen -= m->m_len;
	}
	
	*mp = tp;
	
#ifdef mips
	wbflush();
#endif
	return (top);
}

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
lnread(sc, rlbuf, len, swloop, index)
	register struct ln_softc *sc;
	register char *rlbuf;
	int len,index;
	struct mbuf *swloop;
{
	register struct ether_header *eptr, eh;
	register struct lnsw *lnsw = sc->lnsw;	/* ptr to switch struct */
    	struct mbuf *m, *swloop_tmp1;
	struct protosw *pr;
	int off, resid;
	struct ifqueue *inq;

	/*
	 * Deal with trailer protocol: if type is INET trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	if (swloop) {
		eptr = mtod(swloop, struct ether_header *);
		eh = *eptr;
		eptr = &eh;
		if ( swloop->m_len > sizeof(struct ether_header))
			m_adj(swloop, sizeof(struct ether_header));
		else {
			MFREE(swloop, swloop_tmp1);
			if ( ! swloop_tmp1 )
				return;
			else
				swloop = swloop_tmp1;
		}
	} else {
		if ( lnsw->ln_dodma && (m= sc->rmbuf[index]) &&
sc->dma[index] ) {
                      	eptr = mtod(m, struct ether_header *);
                       	m->m_off += sizeof( struct ether_header );
		} else {	
			if ( lnsw->ln_dodma && !m ) 
	 			/* Couldn't get an mbuf */
				return;
			eptr = &eh;
			rlbuf = lnsw->ln_cpyinb(sc, rlbuf, eptr, sizeof(struct ether_header), 0);
		}

		/*
		 * Done with ether_header; drop len
		 */
		len -= sizeof(struct ether_header);
	}

 	if (!(sc->is_if.if_flags & IFF_PROMISC) ) { 
	/*
	 * Make sure our multicast address filter doesn't hand us
	 * up something that doesn't belong to us!
	 */
	  if ((eptr->ether_dhost[0] & 1) && (bcmp ((caddr_t)eptr->ether_dhost,
	          (caddr_t)etherbroadcastaddr,MULTISIZE))) {
	    int i;
	    for (i = 0; i < sc->nmulti; i++) {
		if (sc->muse[i] &&
		    !(bcmp (&sc->multi[i],eptr->ether_dhost,MULTISIZE)))
			break;
	    }
	    if (i == sc->nmulti ) { 
			if ( lnsw->ln_dodma ) 
				m_freem(sc->rmbuf[index]);
			return;
			}
		}
	}
	sc->is_if.if_ipackets++;

	eptr->ether_type = ntohs((u_short)eptr->ether_type);
	if ((eptr->ether_type >= ETHERTYPE_TRAIL &&
	    eptr->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER)) {
		off = (eptr->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU) {
			if ( lnsw->ln_dodma ) 
				m_freem(sc->rmbuf[index]);
			return;		/* sanity */
		}
		if (swloop) {
			struct mbuf *mprev, *m0 = swloop;
			mprev = m0;
			while (swloop->m_next){/*real header at end of chain*/
				mprev = swloop;
				swloop = swloop->m_next;
			}
			/* move to beginning of chain */
			mprev->m_next = 0;
			swloop->m_next = m0;
			eptr->ether_type = ntohs( *mtod(swloop, u_short *));
			m = m_copy(swloop, 0, M_COPYALL);
			m_freem(swloop);
		} else {
		 	if(m = lnsw->lnget(sc, rlbuf, len, off, index)) {
		     		eptr->ether_type =
					ntohs( *mtod(m, u_short *));
					resid = ntohs( *(mtod(m, u_short *)+1));
				if (off + resid > len) {
					if ( lnsw->ln_dodma ) 
						m_freem(sc->rmbuf[index]);
					 return;		/* sanity */
					}
				len = off + resid;
			}	
			else /* can't get mbuf */
				return;
		}
	} else {
		off = 0;

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; lnget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
		if (swloop) {
			m = m_copy(swloop, 0, M_COPYALL);
			m_freem(swloop);
		} else {
			if(lnsw->ln_dodma && sc->dma[index])
				m->m_len = len;
			else
				m = lnsw->lnget(sc, rlbuf, len, off, index);
		}
	}

	if (m == 0) {
		return;
	}

	if (off) {
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
	}

	/*
	 * Bump up DECnet counters. Input packets for "netstat" include
	 * ALL directed, multicast, and error inputs. For DECnet, only
	 * error-free input packets are counted. See the DEUNA User's
	 * Guide for a breakdown of the counters.
	 */
	sc->ctrblk.est_bytercvd += len ;
	if (sc->ctrblk.est_blokrcvd != (unsigned) 0xffffffff)
		sc->ctrblk.est_blokrcvd++;

	if( eptr->ether_dhost[0] & 1 ) {
		sc->ctrblk.est_mbytercvd += len ;
		if (sc->ctrblk.est_mblokrcvd != (unsigned) 0xffffffff)
			sc->ctrblk.est_mblokrcvd++;
	}
        /* Dispatch this packet */
	/* is_ed really is ifp alias arpcom alias ether_driver... */
        net_read(&(sc->is_ed), eptr, m, len, (swloop != NULL), (off != 0));

}

/*
 * lntint hasn't been called in a while, so restart chip,
 * and reset timer.
 */
lnwatch(unit)
int unit;
{
	register struct ln_softc *sc = ln_softc[unit];
 	register struct ifnet *ifp = &sc->is_if;
	int s;

	s=splimp();
        smp_lock(&sc->lk_ln_softc, LK_RETRY);

	ifp->if_timer = 0;
	sc->ln_debug.trfull++;
	
	smp_unlock(&sc->lk_ln_softc);
	lnrestart(ifp);
	lninit(ifp->if_unit);
	splx(s);
}


/*
 * Allocate a chunk of local RAM buffer. Since we never
 * give Local RAM buffer memory back, we'll just step up the
 * byte-count on a per-unit basis. (16 bits on 32 bit boundary)
 */
caddr_t
ln_alloc16 (sc, size, align)
	register struct ln_softc *sc;
	int size;
	int align;
{
	register int tmpoff = sc->lrb_offset;
	register int tmpoff1;
	/*
	 * Start out on the first "align" boundary
	 */
	if (align) {
		tmpoff1 = svtolance16(sc->ln_lrb) + tmpoff
;
		while ((tmpoff1&((align*2))) &&
			tmpoff < LN_LRB_SIZE)
			{ 
			tmpoff += ((tmpoff1&0x01) ? 3 : 1);
			tmpoff1 += ((tmpoff1&0x01) ? 3 : 1);
		}
	}
	if ((int)LRBADDR(tmpoff,size) < LN_LRB_SIZE) {
		sc->lrb_offset = (int)LRBADDR(tmpoff,size);
		return ((caddr_t)((int)sc->ln_lrb + tmpoff));
	}
	else
		return (0);
}
/*
 * Allocate a chunk of local RAM buffer. Since we never
 * give Local RAM buffer memory back, we'll just step up the
 * byte-count on a per-unit basis. (32 bits wide)
 */
caddr_t
ln_alloc32 (sc, size, align)
	register struct ln_softc *sc;
	int size;
	int align;
{
	register int tmpoff = sc->lrb_offset;
	register int tmpoff1 ; 

	/*
	 * Start out on the first "align" boundary
	 */
	if (align) {
		tmpoff1 = svtolance32(sc->ln_lrb) + tmpoff ;	
		while ((tmpoff1&(align)) && tmpoff < LN_LRB_SIZE)
			{
			tmpoff++;
			tmpoff1++ ;
		}
	}
	if ((int)(tmpoff+size) < LN_LRB_SIZE) {
		sc->lrb_offset = (int)(tmpoff+size);
		return ((caddr_t)((int)sc->ln_lrb + tmpoff));
	}
	else
		return (0);
}

caddr_t
ln_alloc4x4 (sc, size, align)
	register struct ln_softc *sc;
	int size;
	int align;
{
	register int tmpoff = sc->lrb_offset;
	register int tmpoff1 ; 


	/* Alloc routine for buffers only.  Each buffer has 4 good words
	 * followed by 4 unused words, then 4 good, etc.  To save space,
	 * use the 4 unused words for the next buffer to be alloc'd, thus
	 * they will be alternating.
	 */

	if (sc->rring[0] == NULL)	  /* allocing the 1st one */
		align = LN_OCTA_ALIGN;

	/*
	 * Start out on the first octaword aligned boundary
	 */
	if (align) {
		tmpoff1 = sc->lnsw->ln_svtolance(sc->ln_lrb) +
tmpoff ;	
		while ((tmpoff1&(align)) && tmpoff < LN_LRB_SIZE)
			{
			tmpoff++;
			tmpoff1++ ;
		}
	}

	if (((int)sc->ln_lrb + tmpoff) & 0x10)	/* 5th bit set */
		sc->lrb_offset = (int)(tmpoff+16);/* next time use altrn space */
	else
		if ((int)(tmpoff+(size * 2)) < LN_LRB_SIZE) 
			sc->lrb_offset = (int)(tmpoff + (size * 2) - 16); 
		else return (0);

	return ((caddr_t)((int)sc->ln_lrb + tmpoff));
}

/*
 * Zero "len" bytes of Local RAM buffer
 */
ln_bzero16 (addr, len)
	register char *addr;
	register int len;
{
	register int i;
	register caddr_t lrbptr =  addr;

	if (len) {
		if ((u_long)lrbptr & 0x01) {
			*(u_long *)(lrbptr - 1) &= (u_short)0xff;
			lrbptr +=3;
			len--;
		}
		for (i = 0; i < (len - 1); i += sizeof(u_short)) {
			*(u_long *)lrbptr = (u_short)0;
			lrbptr += sizeof(u_long);
		}
		if (i != len)
			*(u_long *)lrbptr &= (u_short)0xff00;
	}
}

/*
 * Zero "len" bytes of Local RAM buffer
 */
ln_bzero32 (addr, len)
	register char *addr;
	register int len;
{
	if (len) 
		bzero(addr, len);
}

ln_bzero4x4 (addr, len )
	register char *addr;
	register int len;
{
	register tmp1;
	register caddr_t lrbptr = addr;
	register end4word;
	int i;

	/* bzero routine for ring buffers only */

	if ((u_long) lrbptr & 0xf) {
	/* start on a 4 word boundary */
		end4word = (0x10 - ((u_long)lrbptr & 0xf));
		tmp1 = ((len > end4word) ? end4word : len);
		bzero(lrbptr, tmp1);
		len -= tmp1;
                lrbptr += tmp1;
                if (((u_long)lrbptr & 0xf) == 0)
                        lrbptr += 0x10;
	}
	/* bzero in 4 word boundaries, skipping every 4 words */
	tmp1 = (len >> 4);
	for (i=0; i < tmp1; i++) {
		bzero(lrbptr,16);
		lrbptr += 32;
	}

	len -= (tmp1 << 4);

	/* now the left over */
	if (len) 
		bzero(lrbptr, len);
}

/*
 * Convert a system virtual address to the appropriate addressing
 * scheme as seen by the LANCE chip for a 32 bit wide buffer. 
 */
svtolance32 (virt)
	caddr_t	virt;
{
	register int off, phy,base ;
	phy = (int)(svtophy(virt));
	off = phy & (LN_LRB_SIZE-1);	/* Byte offset from base of LRB */
        base = ((phy & ~(LN_LRB_SIZE-1)) & 0xffffff); /* LRB base addr. */
	return (off+base);
}

/*
 * Convert a system virtual address to the appropriate addressing
 * scheme as seen by the LANCE chip for a 16 bit wide buffer.
 */
svtolance16 (virt)
	caddr_t	virt;
{
	register int off, phy, tmp;
	phy = (int)(svtophy(virt));
	off = phy & (LN_LRB_SIZE-1);	/* Byte offset from base of LRB */
	tmp = off/2;
	return ((off == tmp*2) ? tmp : tmp+1);
}

/*
 * Specialized "bcopy" to move len bytes from
 * 16-bit wide by 32-bit aligned local RAM buffer.
 * Loop does up to 32 short-word moves in a single
 * shot until the buffer is depleted. Off is non-zero
 * if we wish to begin copying off-many bytes beyond
 * "from".
 */
caddr_t
ln_cpyin16(sc,from,to,len,off)
struct ln_softc *sc;
caddr_t from;
caddr_t to;
int len, off;
{
	register caddr_t lrbptr = LRBADDR(from, off);
	register caddr_t dp = to;
	register int tmp0, tmp1;

	tmp0 = (unsigned)len;

	if (sc->lnsw->ln_dma) 	  /* no read thru caching */
		clean_dcache(PHYS_TO_K0(svtophy(lrbptr)),len*2);
	if ((u_long)lrbptr & 0x01) {
		/* Start LRB on even short-word boundary */
		*dp++ = *lrbptr;
		lrbptr += 3;
		tmp0--;
	}
	while (tmp0) {
		switch (tmp1 = (tmp0 >> 1)) {
		default:
		case 32:
			tmp1 = 32;
			*(u_short *)(dp+62) = *(u_long *)(lrbptr+124);
		case 31:
			*(u_short *)(dp+60) = *(u_long *)(lrbptr+120);
		case 30:
			*(u_short *)(dp+58) = *(u_long *)(lrbptr+116);
		case 29:
			*(u_short *)(dp+56) = *(u_long *)(lrbptr+112);
		case 28:
			*(u_short *)(dp+54) = *(u_long *)(lrbptr+108);
		case 27:
			*(u_short *)(dp+52) = *(u_long *)(lrbptr+104);
		case 26:
			*(u_short *)(dp+50) = *(u_long *)(lrbptr+100);
		case 25:
			*(u_short *)(dp+48) = *(u_long *)(lrbptr+96);
		case 24:
			*(u_short *)(dp+46) = *(u_long *)(lrbptr+92);
		case 23:
			*(u_short *)(dp+44) = *(u_long *)(lrbptr+88);
		case 22:
			*(u_short *)(dp+42) = *(u_long *)(lrbptr+84);
		case 21:
			*(u_short *)(dp+40) = *(u_long *)(lrbptr+80);
		case 20:
			*(u_short *)(dp+38) = *(u_long *)(lrbptr+76);
		case 19:
			*(u_short *)(dp+36) = *(u_long *)(lrbptr+72);
		case 18:
			*(u_short *)(dp+34) = *(u_long *)(lrbptr+68);
		case 17:
			*(u_short *)(dp+32) = *(u_long *)(lrbptr+64);
		case 16:
			*(u_short *)(dp+30) = *(u_long *)(lrbptr+60);
		case 15:
			*(u_short *)(dp+28) = *(u_long *)(lrbptr+56);
		case 14:
			*(u_short *)(dp+26) = *(u_long *)(lrbptr+52);
		case 13:
			*(u_short *)(dp+24) = *(u_long *)(lrbptr+48);
		case 12:
			*(u_short *)(dp+22) = *(u_long *)(lrbptr+44);
		case 11:
			*(u_short *)(dp+20) = *(u_long *)(lrbptr+40);
		case 10:
			*(u_short *)(dp+18) = *(u_long *)(lrbptr+36);
		case  9:
			*(u_short *)(dp+16) = *(u_long *)(lrbptr+32);
		case  8:
			*(u_short *)(dp+14) = *(u_long *)(lrbptr+28);
		case  7:
			*(u_short *)(dp+12) = *(u_long *)(lrbptr+24);
		case  6:
			*(u_short *)(dp+10) = *(u_long *)(lrbptr+20);
		case  5:
			*(u_short *)(dp+8) = *(u_long *)(lrbptr+16);
		case  4:
			*(u_short *)(dp+6) = *(u_long *)(lrbptr+12);
		case  3:
			*(u_short *)(dp+4) = *(u_long *)(lrbptr+8);
		case  2:
			*(u_short *)(dp+2) = *(u_long *)(lrbptr+4);
		case  1:
			*(u_short *)(dp) = *(u_long *)(lrbptr);
			break;
		case 0:
			if (tmp0 & 0x01) {
				/* One lousy byte left over! */
				*dp = *lrbptr++;
			}
			return(lrbptr);
			
		}
		/* Actually did some word moves */
		lrbptr += (tmp1 << 2);
		dp += (tmp1 << 1);
		tmp0 -= (tmp1 << 1);
	}
	return(lrbptr);
}

caddr_t
ln_cpyin32(sc,from,to,len,off)
struct ln_softc *sc;
register caddr_t from;
register caddr_t to;
int len, off;
{
	bcopy(from+off,to,len);
	return(from+len);
}

caddr_t ln_cpyin4x4s();

caddr_t
ln_cpyin4x4(sc,from,to,len,off)
struct ln_softc *sc;
caddr_t from;
caddr_t to;
int len, off;
{
	register caddr_t lrbptr = (from+off);
	register caddr_t dp = to, k0_addr;
	register tmp1;

	/* call ln_clean_dcache using lrbptr on 4 word boundary */
	/* add 2 to tmp1 to account for fragments */
	k0_addr = (caddr_t)PHYS_TO_K0(svtophy((u_long)lrbptr & ~(0xf)));
	tmp1 = (len >> 4) + 2;
	ln_clean_dcache4x4(k0_addr, tmp1);
	
	return(ln_cpyin4x4s(lrbptr,dp,len));
}

/*
 * Specialized "bcopy" to move len bytes into
 * 16-bit wide by 32-bit aligned local RAM buffer.
 * Loop does up to 32 short-word moves in a single
 * shot until the buffer is depleted. "ln_hold" is
 * used for MIPS to short-word align buffers.
 */
#ifdef mips
u_short ln_hold[(ETHERMTU + sizeof(struct ether_header)) / 2];
#endif mips

caddr_t
ln_cpyout16(from,to,len,off)
caddr_t from;
caddr_t to;
int len, off;
{
	register int tmp0,tmp1;
	register caddr_t dp = from;
	register caddr_t lrbptr = LRBADDR(to, off);

	tmp0 = (unsigned)len;
	if ((u_long)lrbptr & 0x01) {
		/* Start LRB on even short-word boundary */
		*(u_long *)(lrbptr - 1) |=
		(u_short)((*dp++ & 0xff) << 8);
		lrbptr += 3;
		tmp0--;
	}
#ifdef mips
	/*
	 * If we're running on a MIPS, can't copy a short 
	 * from an odd boundary.  Align dp to an even boundary
	 * by using a known short-word aligned area.
	 */

	if (tmp0 && ((u_long)dp & 0x01)) {
	   bcopy(dp,(caddr_t)ln_hold, tmp0);
	   dp = (caddr_t)ln_hold;
	}
#endif mips
	while (tmp0) {
	    switch (tmp1 = (tmp0 >> 1)) {
	    default:
	   case 32:
		tmp1 = 32;
		*(u_long *)(lrbptr+124) = *(u_short *)(dp+62);
	    case 31:
		*(u_long *)(lrbptr+120) = *(u_short *)(dp+60);
	    case 30:
		*(u_long *)(lrbptr+116) = *(u_short *)(dp+58);
	    case 29:
		*(u_long *)(lrbptr+112) = *(u_short *)(dp+56);
	    case 28:
		*(u_long *)(lrbptr+108) = *(u_short *)(dp+54);
	    case 27:
		*(u_long *)(lrbptr+104) = *(u_short *)(dp+52);
	    case 26:
		*(u_long *)(lrbptr+100) = *(u_short *)(dp+50);
	    case 25:
		*(u_long *)(lrbptr+96) = *(u_short *)(dp+48);
	    case 24:
		*(u_long *)(lrbptr+92) = *(u_short *)(dp+46);
	    case 23:
		*(u_long *)(lrbptr+88) = *(u_short *)(dp+44);
	    case 22:
		*(u_long *)(lrbptr+84) = *(u_short *)(dp+42);
	    case 21:
		*(u_long *)(lrbptr+80) = *(u_short *)(dp+40);
	    case 20:
		*(u_long *)(lrbptr+76) = *(u_short *)(dp+38);
	    case 19:
		*(u_long *)(lrbptr+72) = *(u_short *)(dp+36);
	    case 18:
		*(u_long *)(lrbptr+68) = *(u_short *)(dp+34);
	    case 17:
		*(u_long *)(lrbptr+64) = *(u_short *)(dp+32);
	    case 16:
		*(u_long *)(lrbptr+60) = *(u_short *)(dp+30);
	    case 15:
		*(u_long *)(lrbptr+56) = *(u_short *)(dp+28);
	    case 14:
		*(u_long *)(lrbptr+52) = *(u_short *)(dp+26);
	    case 13:
		*(u_long *)(lrbptr+48) = *(u_short *)(dp+24);
	    case 12:
		*(u_long *)(lrbptr+44) = *(u_short *)(dp+22);
	    case 11:
		*(u_long *)(lrbptr+40) = *(u_short *)(dp+20);
	    case 10:
		*(u_long *)(lrbptr+36) = *(u_short *)(dp+18);
	    case  9:
		*(u_long *)(lrbptr+32) = *(u_short *)(dp+16);
	    case  8:
		*(u_long *)(lrbptr+28) = *(u_short *)(dp+14);
	    case  7:
		*(u_long *)(lrbptr+24) = *(u_short *)(dp+12);
	    case  6:
		*(u_long *)(lrbptr+20) = *(u_short *)(dp+10);
	    case  5:
		*(u_long *)(lrbptr+16) = *(u_short *)(dp+8);
	    case  4:
		*(u_long *)(lrbptr+12) = *(u_short *)(dp+6);
	    case  3:
		*(u_long *)(lrbptr+8) = *(u_short *)(dp+4);
	    case  2:
		*(u_long *)(lrbptr+4) = *(u_short *)(dp+2);
	    case  1:
		*(u_long *)(lrbptr) = *(u_short *)(dp);
		break;
	    case 0:
		if (tmp0 & 0x01) {
			/* One lousy byte left over! */
			*(u_long *)(lrbptr) =
				(u_short)(*dp & 0xff);
		  	lrbptr++;
		}
		return(lrbptr);
	
	    }
	    /* Actually did some word moves */
	    lrbptr += (tmp1 << 2);
	    dp += (tmp1 << 1);
	    tmp0 -= (tmp1 << 1);
	}
	return(lrbptr);
}

/* Normal 32-bit wide memory copy. */
caddr_t
ln_cpyout32(from,to,len,off)
register caddr_t from;
register caddr_t to;
int len, off;
{
	bcopy (from, to+off, len);
	return(to + len);
}

/* set the flag field within the ln_ring structure */

ln_setflag16(rbuf,flg)
register caddr_t rbuf;
register int flg;
{	
	*(rbuf + 5) = flg;
}

ln_setflag32(rbuf,flg)
register caddr_t rbuf;
register int flg;
{	
	*(rbuf + 3) = flg;
}

#endif
