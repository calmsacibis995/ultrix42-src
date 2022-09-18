#ifndef lint
static char *sccsid = "@(#)if_ne.c	4.7	(ULTRIX)  2/26/91";
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
/*-----------------------------------------------------------------------
 * Modification History:
 *
 * 24-Feb-91 jsd
 *	Allow loopback packets if COPYALL mode is set
 *
 * 9-Aug-90 chc (Chran-Ham Chang)
 *	Fixed the SIOCRPHYSADDR ioctl problem.
 *
 * 25-Sep-89 chc (Chran-Ham Chang) 
 * 	Created this Second Generation Ethernet Controller (SGEC)
 *	network driver for mipsfairII  
 *
 * 08-Mar-90 chc
 *      reworked on the neinit routine
 *      fixed the mbuf free problem 
 *
 * --------------------------------------------------------------------*/
/*
 * Digital SGEC Network Interface
 */
#include "ne.h"
#include "packetfilter.h"       /* NPACKETFILTER */
#include "../data/if_ne_data.c"

extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern int net_output();
extern struct timeval time;
int	neprobe(), neattach(), neintr(),nereset();
int	neinit(),nestart(),neioctl(),newatch();
extern int cpu;

u_short nestd[] = { 0 };
struct	uba_driver nedriver =
	{ neprobe, 0, neattach, 0, nestd, "ne", neinfo };

u_char ne_unused_multi[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
u_char ne_multi[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 };

#ifdef vax
#define svtopte(x) (unsigned long)(&Sysmap[btop((int)x&~VA_SYS)])
#define ptetosv(x) (unsigned long)((unsigned)ptob((struct pte *)(x)-&Sysmap[0])|
VA_SYS)
#define vaddr(y) (char *)(ptetosv((y)->pt_addr)+(y)->offset)
#endif

int	nedebug = 0;

#define NEMCLGET(m) { \
	register struct mbuf *p ; \
	MGET((m), M_DONTWAIT, MT_DATA); \
	if ((m)) { \
		MCLGET((m), p); \
 		m->m_len = CLBYTES; \
		if (p == 0) { \
			m_freem((m)); \
		       (m) = (struct mbuf *)NULL; \
		 } \
	  } \
}


#define NEALIGN(start,align)	(((int)start&align) ? \
				  (caddr_t)((((int)start)&(~align))+ \
				  align+1): \
				  (caddr_t)(start))
/*
 * Probe the SGEC to see if it's there
 */

neprobe(reg,ui)
	caddr_t reg;
	struct uba_device *ui;
{
	int unit = ui->ui_unit;
        register struct ne_softc *sc = &ne_softc[unit];
	register struct nesw *nesw;     /* ptr to switch struct */

	int i,j;
	caddr_t ringaddr,mp; 
	struct mbuf **mbp;

	/* 
	 * CPU identifiers 
	 */
	 switch (cpu) {
		case DS_5500:		/* mipsfair 2 */
			sc->nesw = nesw = ds5500sw;
			sc->ne_narom = (u_char *)PHYS_TO_K1(nesw->ne_phys_narom);
			sc->ne_csrs = (NECSRS *)PHYS_TO_K1(nesw->ne_phys_csr);
			sc->ne_initcsr = (u_long)( nesw->ne_ipl | nesw->ne_vec | nesw->ne_mode | 0x1fff0003 );
			sc->ne_cmdcsr = nesw->ne_burst << NE_BURST_SHIFT | nesw->ne_sigle | NE_CSR6_DC ; 
			sc->ntring = nesw->ntdesc;
			sc->nrring = nesw->nrdesc;
			sc->is_if.if_sysid_type = 111; 
			break;
		default:
			printf("neprobe : cpu type %d unknown",cpu);
			return(0);
	}

	/*
	 * reset the SGEC and check the self test result
	 */
	if(!nesoftreset(sc,unit))
		return(0);  /* self test fail */
	
	/* 
	 * Initial the CSR0
	 */
	if(!nesetcsr0(sc,unit))
		return(0);  /* csr0 write error */

	/*
	 * Load the System Base Register for VAX arch.
         */
#ifdef vax
	csrpt->csr7 = (u_long) mfpr(SBR);
#endif
	/*
	 * Allocate contigous, octaword aligned (performance purpose)
	 * space for both descriptor rings   
	 */
	 KM_ALLOC(ringaddr, caddr_t, sizeof(NEDESC) * (sc->ntring +
	 sc->nrring + 3), KM_DEVBUF,KM_NOW_CL_CO_CA);
	if(ringaddr == 0 ) {
		printf(" ne %d: couldn't allocate memory for  descriptor ring\n",unit);
		return(0);
	}
	/* make the OCTAWORD alignment to increase performance */
	sc->tring = (NEDESC *)NEALIGN(ringaddr,NE_OCTAWORD); 

	sc->tring = (NEDESC *)PHYS_TO_K1(svtophy(sc->tring));
	/* chain back the ring */
	sc->tring[sc->ntring].ne_bfaddr = svtophy(sc->tring);
	sc->tring[sc->ntring].ne_info = NE_CA; 
	sc->tring[sc->ntring].ne_own = NE_OWN;

	sc->rring = sc->tring + sc->ntring + 1;

	sc->rring = (NEDESC *)PHYS_TO_K1(svtophy(sc->rring));
	/* chain back the ring */
	sc->rring[sc->nrring].ne_bfaddr = svtophy(sc->rring);
	sc->rring[sc->nrring].ne_info = NE_CA; 
	sc->rring[sc->nrring].ne_own = NE_OWN;
			
	
	/* 
	 * Allocate mbuf pointer for transmit and receive buffer
	 *
	 *	tmbuf : mbuf pointer which points the transmit
	 *		data buffer 
	 *	smbuf : mbuf chained point which is passed by
	 *		higher network layer
	 *	rmbuf : mbuf pointer which points the receive buffer
	 */
	KM_ALLOC(sc->tmbuf, struct mbuf ** , sizeof(struct mbuf *) * (sc->ntring * 2 + sc->nrring), KM_DEVBUF, KM_NOW_CL_CO_CA);
	if(sc->tmbuf == 0 ) {
		printf(" ne %d: couldn't allocate memory for buffer pointer\n",unit);
		KM_FREE(ringaddr,KM_DEVBUF);
		return(0);
	}
	/* clean the mbuf pointer */
	bzero(sc->tmbuf,((sizeof(struct mbuf *) * (sc->ntring *2 + sc->nrring))));

	sc->smbuf = sc->tmbuf + sc->ntring ;
	sc->rmbuf = sc->smbuf + sc->ntring ;



	/* setup buffer must be word aligned */
	KM_ALLOC(mp,caddr_t ,SETUPSIZE+1, KM_DEVBUF,KM_NOW_CL_CO_CA);   
	if(mp == 0) {
		printf(" ne%d: couldn't allocate setup buffer \n",unit);
		KM_FREE(ringaddr,KM_DEVBUF);
		KM_FREE(sc->tmbuf,KM_DEVBUF);
		return(0);
	}
	sc->ne_setup = (struct ne_setup *)NEALIGN(mp,NE_WORD); 

	/*
	 * initialize multicast address table 
	 */
	for (i=0; i<NMULTI; i++) {
		sc->muse[i] = 0;
		bcopy(ne_multi, &sc->multi[i],MULTISIZE);
	}

	/* 
	 * fill out the station address from narom
	 */
	 for ( i = j = 0; i < 6 ; i++, j +=4 ) {
		sc->ne_setup[0].setup_char[i]=sc->is_addr[i]=sc->ne_narom[j];
	}
	
return(sizeof(struct ne_softc));

}
/*
 * Interface exeists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
neattach(ui)
	struct	uba_device *ui;
{
	register struct ne_softc *sc = &ne_softc[ui->ui_unit];
	register struct ifnet *ifp = &sc->is_if;
	register int i;
        register struct sockaddr_in *sin;
	register NECSRS *csrpt = sc->ne_csrs;

	int delay;

	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "ne";
	ifp->if_mtu = ETHERMTU;
	ifp->if_type = IFT_ETHER;
	ifp->if_flags |= IFF_BROADCAST | IFF_DYNPROTO | IFF_NOTRAILERS;
	((struct arpcom *)ifp)->ac_ipaddr.s_addr = 0;
        sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
	ifp->if_init = neinit;
	ifp->if_output = net_output;
	ifp->if_start = nestart;
	ifp->if_ioctl = neioctl;
	ifp->if_timer = 0;
	ifp->if_watchdog = newatch;
	ifp->if_reset = nereset;
	/*
	 * Get the SGEC version
	 */
	sc->ne_rev = csrpt->csr10; 
	for (delay = 100000; delay && !(csrpt->csr5 & NE_CSR5_DN); delay--);
	if(!delay)
		{
		printf("ne%d: timeout accessing virtual CSR10 \n",ui->ui_unit);
/* XXX */
		sc->ne_rev = 3;
		}
	sc->ne_rev = csrpt->csr10 >> NE_RE_SHIFT ;
 	bcopy("DEC SGEC Ethernet Interface", ifp->if_version, 27); 
	     ifp->if_version[28] = '\0';  
	printf("ne%d: %s, V%d  hardware address: %s \n", ui->ui_unit, 
          ifp->if_version,sc->ne_rev,ether_sprintf(sc->is_addr)); 

#if NPACKETFILTER > 0
	attachpfilter(&(sc->is_ed));
#endif NPACKETFILTER > 0 

	if_attach(ifp);
}

/* 
 * Initialization of interface, allocate the mbuf for reveive ring,
 * fill up the setup frame and start the interface  
 */
neinit(unit)
	int	unit;
{
	register struct ne_softc *sc = &ne_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	register NEDESC *rp;
	register struct mbuf *m;	
	register s;

	NECSRS *csrpt = sc->ne_csrs;
	int i,status,delay;


	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;
	if (ifp->if_flags & IFF_RUNNING)
		return;


	s = splimp();
	if ( ifp->if_flags & IFF_PROMISC ) {
		sc->ne_cmdcsr |= NE_AF_PRO ;
		csrpt->csr6 &= ~(NE_CSR6_SR | NE_CSR6_ST);
	}
	else {
		sc->ne_cmdcsr &= ~NE_AF_PRO ;

       		/* prepare the setup frame */
		nesetup( sc , NE_SETUP_IC);

	        sc->tring[0].ne_bfaddr = svtophy(sc->ne_setup);
        	sc->tring[0].ne_bsize = SETUPSIZE ;
        	sc->tring[0].ne_info = NE_DT_TSET | NE_SETUP_IC;
        	sc->tring[0].ne_own = NE_OWN;

		/* startup the setup packet and wait for interrupt */
		csrpt->csr4 = svtophy(sc->tring);
		csrpt->csr6 = sc->ne_cmdcsr | NE_CSR6_ST | NE_CSR6_IE ;
		csrpt->csr1 = NE_CSR1_PD;
		wbflush();
	
		for (delay = 100; delay > 0 && !(csrpt->csr5 & NE_CSR5_TI); delay--)
			DELAY(10000);  
		
		if(!delay) {   
			printf("ne%d : setup frame transmit timeout \n",unit);	
			status = EIO;
			splx(s);
			return (status);
			}
		else {
			if (sc->tring[0].ne_flag & NE_SETUP_SE) {
				printf("ne%d : setup frame error \n",unit);
				status = EIO;
				splx(s);
				return (status);
			}
			sc->setupqueued = 0;
		}
		/* clear the csr5 */
		csrpt->csr5 = csrpt->csr5;		 
		wbflush();
	}

	/* set up the receive buffer */
	for ( i=0,rp=sc->rring ; i < sc->nrring; i++,rp++) {
		if(!(sc->rmbuf[i])) {
			NEMCLGET(m);	
		} else {
			m = sc->rmbuf[i];
		}
		if(m) {
			neinitdesc(rp,m,1);
			rp->ne_own = NE_OWN;
			sc->rmbuf[i]=m;
		} else {
			printf("ne%d: couldn't allocate receive buffer\n",unit);
			status = EIO;
			splx(s);
			return (status);
			}
	}
	/* set up the transmit buffer */
	for ( i=0,rp=sc->tring ; i < sc->ntring; i++,rp++) {
		if(!(sc->tmbuf[i])) {
			NEMCLGET(m);
		} else {
			m = sc->tmbuf[i];
		}
		if(m) {
			neinitdesc(rp,m,0);
			sc->tmbuf[i]=m;
		} else {
			printf("ne%d: couldn't allocate transmit buffer\n",unit);
			status = EIO;
			splx(s);
			return (status);
		}
	}


	sc->nxmit = sc->tindex = sc->otindex = sc->rindex = 0;
	/* fill out the descriptor address list */
	csrpt->csr3 = svtophy(sc->rring);
	csrpt->csr4 = svtophy(sc->tring);

	if ( ifp->if_flags & IFF_ALLMULTI )
		sc->ne_cmdcsr |= NE_AF_MUL ;

	if ((ifp->if_flags & IFF_LOOPBACK)) 
			/* if this is a internal loop back */
		csrpt->csr6 = sc->ne_cmdcsr | NE_CSR6_SR | NE_CSR6_ST
				| NE_CSR6_IE | NE_OM6_EXL ;
	else
		csrpt->csr6 = sc->ne_cmdcsr | NE_CSR6_SR | NE_CSR6_ST
				| NE_CSR6_IE | NE_OM6_NOR ;

	/*
	 * mark the interface up; place the setup frame in the xmit list
	 */
	ifp->if_flags |= IFF_UP | IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	nestart( unit );
	sc->ztime = time.tv_sec;
	splx(s);
}	

/* 
 * Start output on the interface 
 */
nestart(unit)
	int	unit;
{
	register struct ne_softc *sc = &ne_softc[unit];
	register NEDESC *rp;
	register int index;

	struct mbuf *m,*m0;
	int bufaddr,tlen,s;
	int nNENXMT=sc->ntring;
	NECSRS *csrpt = sc->ne_csrs;

	for ( index = sc->tindex; sc->tring[index].ne_own == 0 &&
		sc->nxmit < ( nNENXMT - 1 ) ; sc->tindex = index =
		++index % nNENXMT ){
		rp = &sc->tring[index];
		if(sc->setupqueued){
			rp->ne_bfaddr = svtophy(sc->ne_setup);
			tlen = SETUPSIZE;  
			rp->ne_info = NE_DT_TSET;
			sc->setupqueued = 0;
			if(sc->ne_setup_ic) {
				rp->ne_info |= NE_SETUP_IC ;
				sc->ne_setup_ic = 0;
			}
		} else { 
			IF_DEQUEUE(&sc->is_if.if_snd, m);
			if(m == 0) {
				return;
			}
			tlen = neput(sc,index,m);
			rp->ne_info |=  NE_TFS | NE_TLS ; 
		}

		rp->ne_bsize = tlen;
		rp->ne_own = NE_OWN;	
		sc->nxmit++;
		sc->is_if.if_flags |= IFF_OACTIVE;
		csrpt->csr1 = NE_CSR1_PD; 
		wbflush();
		sc->is_if.if_timer = 5;			 
	} 
} 
/*
 * Ethernet interface interrup processor 
 */
neintr(unit)
	int	unit;
{
        register struct ne_softc *sc = &ne_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	register u_long	csr;

	u_long csr6;
	NECSRS *csrpt = sc->ne_csrs;
	int s;

	s = splimp();
	csr =csrpt->csr5;	/* save the old csr5 */
	csrpt->csr5 = csr; 	/* clear the csr5 */
	wbflush();
	if ( csr & NE_CSR5_TW ) {
		netbablcnt++;
		if(nedebug)
			mprintf("ne%d: transmit timeout (TW)\n",unit);
		/* reset the chip */
		nereset(sc);
		splx(s);
		return;
	}
	if ( csr & NE_CSR5_RU ) {
			/* Receive Buffer Unavailable. 
			 * When set, indicateds that the next descriptor
			 * on the receive list is owned by the host.
			 * That means a packet was lost
			 */
		 nemisscnt++;
		 if(nedebug)
			mprintf("ne%d: missed packet (Receiv Buffer Unavailable)",unit);
		if(sc->ctrblk.est_sysbuf != 0xffff)
			sc->ctrblk.est_sysbuf++;
		/* Do not need to poll the receive. The SGEC will turn
		 * on the receive running state when next frame arrive 
	         */ 
	}

	if ( csr & NE_CSR5_ME ) {
			/*
			 * Memory Error
			 * It happened when any one of followings occur: 
			 * 1) host memory problem
			 * 2) Parity error detected on an host to SGEC
			 *   CSR write or SGEC read from memory
			 */
		
		nemerrcnt++;
		mprintf("ne%d: memory error (MERR) \n", unit);
		nereset(sc);
		splx(s);
		return;
		}
	if ( csr & NE_CSR5_RI )
		nerint(unit);
	if ( csr & NE_CSR5_TI )
		netint(unit);
	splx(s);
}

/*
 * Ethernet interface transmit interrupt
 */
netint(unit)
	int unit;
{
	register struct ne_softc *sc = &ne_softc[unit];
	register int index = sc->otindex;
	register int i;
	register NEDESC *rp= &sc->tring[sc->otindex];
	register u_long status;

	struct mbuf *mp,*mp0;
	struct ether_header *eh;
	NECSRS *csrpt = sc->ne_csrs;
	int 	nNENXMT = sc->ntring;
	struct ifnet *ifp = &sc->is_if;
	int	ccount;
	short   tlen;

	while ( (sc->otindex != sc->tindex) && ( rp->ne_own != NE_OWN ) && sc->nxmit > 0 ) { 
		status = rp->ne_flag;
		tlen = rp->ne_bsize; 

		/*
		 * Found last buffer in the packet
		 * (hence a valid string of descriptors)
		 * so free things up.
		 */
		mp = sc->smbuf[index];
		sc->smbuf[index] = (struct mbuf *)NULL; 

		if(!(--sc->nxmit)) {
			ifp->if_timer=0; 
			sc->is_if.if_flags &= ~IFF_OACTIVE;
		}

		if(!(rp->ne_info & NE_DT_TSET)) { /* not a setup packet */
			/* 
			 * Network statistics
			 */
			sc->is_if.if_opackets++;
			ccount = (status & NE_CC) >> 3;	
			if(ccount && !(status & NE_LC)) {
				sc->is_if.if_collisions += ccount;
				/*
				 * Decnet statistics
				 */
				if (ccount) { /* collision happened */
					if(ccount == 1) {
						if (sc->ctrblk.est_single != (unsigned)0xffffffff)
							sc->ctrblk.est_single++;
					} else  /* more than one */
						if(sc->ctrblk.est_multiple != (unsigned)0xffffffff);
							sc->ctrblk.est_multiple++;
			  	} 
			}
			if(status & NE_DE) 
				if (sc->ctrblk.est_deferred != (unsigned) 0xffffffff)
			 		sc->ctrblk.est_deferred++;
			
			 /* Heartbeat Fail */
			 if((status & NE_HF) && !(status & NE_UF)) 
				if (sc->ctrblk.est_collis != 0xffff)
					sc->ctrblk.est_collis++;

			/*
			 * Check for transmission errors
			 */
			 if(status & NE_ES) {
				sc->is_if.if_oerrors++;
				if (sc->ctrblk.est_sendfail != 0xffff) {
					sc->ctrblk.est_sendfail++;
					if (status & NE_EC ) {/* Excessive Collisions */
						if (nedebug) mprintf("ne%d: excseeive collision (EC)\n",unit);
							sc->ctrblk.est_sendfail_bm |= 1;
					}
					if (status & NE_LC && !(status & NE_UF)) {
						if (nedebug) mprintf("ne%d: late transmit collision (LC)\n",unit);
							sc->ctrblk.est_sendfail_bm |= 2; 
						; /* not implemented */
					}
					if (status & (NE_NC | NE_LO) ) {
					/* no carrier */
						if (nedebug) mprintf("ne%d: no carrier or lost carrier (LO or NC)\n",unit);
							sc->ctrblk.est_sendfail_bm |= 4;
					}
					if (status & NE_UF ) {/* Underflow error */
						if (nedebug) mprintf("ne%d: underflow error (UF)\n",unit);
					}
					if (status & NE_TLE ) {/* Length error */
						if (nedebug) mprintf("ne%d: transmit buffer length error (LE)\n",unit);
					}
				}
				m_freem(mp);
			}  else {
				/*
			 	 * Accumulate statistics for DECnet
		 		 */
				if ((sc->ctrblk.est_bytesent + tlen) > sc->ctrblk.est_bytesent)
					sc->ctrblk.est_bytesent += tlen;
                		if (sc->ctrblk.est_bloksent != (unsigned)0xffffffff)
					sc->ctrblk.est_bloksent++;
				/*
			 	 * If this was a broadcast packet or if we
				 * are in COPYALL mode, loop it back.
				 * We will not free this buffer. 
			 	 * This buffer will be reused. 
			 	 */
				if (mp) {
					eh = mtod( mp, struct ether_header *);
					if ((!(bcmp(eh->ether_dhost,ne_multi,6))) || (sc->is_if.if_flags & IFF_PFCOPYALL)) {
						for(mp0 = mp ,tlen=0; mp0 ;mp0 =mp0->m_next) {
	tlen += mp0->m_len; 
}

						neread(sc,0,mp,index,tlen);
					} else
						m_freem( mp );
				}
			}
		} else { /* this is a setup frame */
			if ( status & NE_SETUP_SE ) {
				printf("ne%d: setup frame buffer size error ( should be 128 bytes ) \n ",unit);
				neinit(unit);
			}
		}
		/*
		 * Init the buffer descriptor
		 */
		neinitdesc(&sc->tring[index],sc->tmbuf[index],0);
		sc->otindex = index = ++index % nNENXMT;
		rp = (NEDESC *) &sc->tring[index];
	}
	/*
	 * Dequeue next transmit request
	 */
	if (sc->nxmit > 0 && (sc->tring[sc->otindex].ne_own == NE_OWN )) {
		csrpt->csr1 = NE_CSR1_PD;
		wbflush();
	}
	if (!(ifp->if_flags & IFF_OACTIVE))
 		nestart( unit );
}

/*
 * Ethernet interface  receiver interrupt.
 */
nerint(unit)
	int	unit;
{
	register struct ne_softc *sc = &ne_softc[unit];
	register NEDESC  *rp;
	register int index;
	register struct ifnet *ifp = &sc->is_if;
	register int first;

	int	nNENRCV = sc->nrring;
	int	rlen;	

	/*
	* Traverse the receive ring looking for packets to pass back.
	* The search is complete when we find a descriptor not in use (
	* which means own by SGEC ).
	*/
	rp = &sc->rring[sc->rindex];
	for (index = sc->rindex; !(rp->ne_own & NE_OWN) ; rp = &sc->rring[index]) {
		
		/*
		 * We don't need to check the chained packet.
		 * The SGEC will filter out those chained packet by
		 * setting the CSR6 DC bit on. The RDES0<FS,LS> will
		 * always be set
		 */
		
		if(!(rp->ne_flag & NE_ES)) { /* valid descriptor */
			struct mbuf *m1;
			/*
		 	 * If can not allocate a cluster mbuf for switch, just
		 	 * drop this packet
		 	 */
			NEMCLGET(m1);
			if(m1 == 0) {
				nenolmbuf++;
				/*
				 * already bumpped up 2 bytes
				 */
				neinitdesc(rp,sc->rmbuf[index],0);
				
			} else {

				rlen = rp->ne_com & 0x00007fff ;
#ifdef mips
				clean_dcache(PHYS_TO_K0(rp->ne_bfaddr),rlen);
#endif mips
				neread(sc,rp,(struct mbuf *)0,index,rlen);
				neinitdesc(rp,m1,1);
				sc->rmbuf[index]=m1;
			}
		/* 
		 * Else not a good packet, check for errors
		 */
		} else {
			sc->is_if.if_ierrors++;
			if(nedebug)  
				mprintf("ne%d: recv err %02x\n",unit,rp->ne_flag&0xff);
			if(sc->ctrblk.est_recvfail != 0xffff) {
				sc->ctrblk.est_recvfail++;
				if(rp->ne_flag & NE_OF) { /* overflow */
					sc->ctrblk.est_recvfail_bm |=4 ;
#if NPACKETFILTER > 0
					sc->is_ed.ess_missed++;
#endif NPACKETFILTER > 0
				}
				if(rp->ne_flag & NE_CE ) { /* CRC error */
					if(rp->ne_flag & NE_DB ) 
					/* Block check error */
						sc->ctrblk.est_recvfail_bm |=1 ;
					else
					/* Frame error */
						sc->ctrblk.est_recvfail_bm |=2 ;
				}
				if(rp->ne_flag & NE_CS) { /* Collision seen */
					sc->ctrblk.est_recvfail_bm |=2 ;
				}
				if(rp->ne_flag & NE_RTL) { /* Frame too long */
					sc->ctrblk.est_recvfail_bm |=4 ;
				}
				if((rp->ne_flag & NE_RF) && !(rp->ne_flag & NE_OF)) { 
				/* Runt Frame - will only receive if */
				/* CSRT6 <PB> is set. Meaningless if */
				/* RDES0<OF> is set	*/
					sc->ctrblk.est_recvfail_bm |=4 ;
				}
				if(rp->ne_flag & NE_RLE ) { /* length error */
							;
				}
			neinitdesc(rp,sc->rmbuf[index],0);
			} 
		}
		rp->ne_own = NE_OWN;
		sc->rindex = index =  ++index % nNENRCV;
	}
}
		
/* 
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol
 */
neread(sc,dp,swloop,index,rlen)
register struct ne_softc *sc;
register NEDESC *dp;
int index,rlen;
struct mbuf *swloop;
{
	register struct ether_header *eptr, eh;

	struct mbuf *m, *swloop_tmp1;
	int off,resid,len;
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
			MFREE(swloop,swloop_tmp1);
			if ( ! swloop_tmp1 )
				return;
			else
				 swloop = swloop_tmp1;
		}
	} else {
		m = sc->rmbuf[index];
		m->m_len = len = rlen - 4;	/* CRC */
		eptr = mtod(m, struct ether_header *);
		eh = *eptr;
		eptr = &eh;

	}

        eptr->ether_type = ntohs((u_short)eptr->ether_type);
	if ((eptr->ether_type >= ETHERTYPE_TRAIL &&
	    eptr->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER)) {
	    	off = (eptr->ether_type - ETHERTYPE_TRAIL) * 512 +
			  sizeof(struct ether_header);
		if (off >= ETHERMTU)
			  return;         /* sanity */
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
		} else {
			eptr->ether_type = 
				ntohs(*(short *)(mtod(m, caddr_t) + off));
			resid = ntohs(*(short *)(mtod(m, caddr_t) + off +2));
			if (off + resid > m->m_len)
				return;            /* sanity */
		}
	 } else {
		off = 0;
	 }

	/*
	 * Pull packet off interface.  (In the case of NE, only need
	 * to allocate another mbuf to switch the old one )
	 * Off is nonzero if packet has trailing header; 
	 * this routine will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	if (swloop) {
		m = m_copy(swloop, 0, M_COPYALL);
		m_freem(swloop);
		if (m == 0)
			return;
		len = m_length(m);
	} else {
		struct mbuf *m0 = m;
		if (off) {
			int nbytes;
			int cnt = resid;
			struct mbuf *n;
			struct mbuf **mp = &m;

			while (cnt > 0) {
				MGET(n,M_DONTWAIT,MT_DATA);
				if (n == 0) {
					m_freem(m);
					nenosmbuf++;
					return;
				}
				nbytes = MIN(MLEN,cnt);
				bcopy((mtod(m, caddr_t) + off),
				       mtod(n, caddr_t), nbytes);
				n->m_len = nbytes;
				off += nbytes;
				cnt -= nbytes;
				*mp = n;
				mp = &n->m_next;
			}
			*mp = m0;
			m0->m_len -= resid;
			/* strip out the trailer header */
			m->m_off += 2 * sizeof (u_short);
			m->m_len -= 2 * sizeof (u_short);
		}

		/*
		 * Trim off ethernet header
		 */
		 m0->m_off += sizeof (struct ether_header);
		 m0->m_len -= sizeof (struct ether_header);

		/*
	 	 * Subtract length of header from len
	 	 */
		len -= sizeof (struct ether_header);
	 }


	/*
	 * Bump up DECnet counters. Input packets for "netstat" include
	 * ALL directed, multicast, and error inputs. For DECnet, only
	 * error-free input packets are counted.
	 */
	sc->is_if.if_ipackets++;
	sc->ctrblk.est_bytercvd += len ;
	if (sc->ctrblk.est_blokrcvd != (unsigned) 0xffffffff)
		sc->ctrblk.est_blokrcvd++;

	if( eptr->ether_dhost[0] & 1 ) {
		sc->ctrblk.est_mbytercvd += len;
			if (sc->ctrblk.est_mblokrcvd != (unsigned) 0xffffffff)
					sc->ctrblk.est_mblokrcvd++;
	}
	/* Dispatch this packet */
	net_read(&(sc->is_ed), eptr, m, len, (swloop != NULL), (off != 0));

}

/*
 * Process an ioctl request
 */
neioctl(ifp, cmd, data)
register struct ifnet *ifp;
int cmd;
caddr_t data;
{
	register struct ne_softc *sc = &ne_softc[ifp->if_unit];

	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	struct ctrreq *ctr = (struct ctrreq *)data;
	struct protosw *pr;
	struct ifaddr *ifa = (struct ifaddr *)data;
	NECSRS	*csrpt = sc->ne_csrs;
	int s, i, error = 0 , j = -1, delay;

	s = splimp();
	switch (cmd) {

	case SIOCENABLBACK:
	case SIOCDISABLBACK:
		if ( cmd == SIOCENABLBACK ) { 
			if (nedebug>1) printf("SIOCENABLBACK ");
			mprintf("ne%d: internal loopback enable requested\n",
				ifp->if_unit);
       		        ifp->if_flags |= IFF_LOOPBACK;
		} else {
			if (nedebug>1) printf("SIOCDISABLBACK ");
			mprintf("ne%d: internal loopback disable requested\n", 
				ifp->if_unit);
                	ifp->if_flags &= ~IFF_LOOPBACK;
		}
		if ( ifp->if_flags & IFF_RUNNING ) { 
			csrpt->csr6 = ~(NE_CSR6_SR | NE_CSR6_ST);
			/*
			 *  polling CSR5 <TS> and <RS>
			 */
			for (delay = 10000; delay && (csrpt->csr5 &
			(NE_TS_STP | NE_RS_STP)) ; delay--);
			neinit(ifp->if_unit);
		}
		break;
 
	case SIOCRPHYSADDR:
		/*
		 * read default hardware address
		 */
		if (nedebug>1) printf("SIOCRPHYSADDR ");
		bcopy(sc->is_addr, ifd->current_pa, 6);
		for ( i = 0 ; i < 6 ; i++ ) 	
			ifd->default_pa[i] = sc->ne_narom[i * 4 ];
		break;
 
	case SIOCSPHYSADDR:
		if (nedebug>1) printf("SIOCSPHYSADDR ");
		bcopy(ifr->ifr_addr.sa_data,sc->is_addr,6);
#if NPACKETFILTER > 0
		pfilt_newaddress(sc->is_ed.ess_enetunit, sc->is_addr);
#endif NPACKETFILTER > 0

		for ( i = 0; i < 6; i++ )
			sc->ne_setup[0].setup_char[i] = sc->is_addr[i];
		if (ifp->if_flags & IFF_RUNNING) {
			nesetup( sc,0 );
			nestart( ifp->if_unit ); 
		} else
			nesetup( sc, 0 );
		break;
	case SIOCDELMULTI:
	case SIOCADDMULTI:
		if (cmd == SIOCDELMULTI) {
			if (nedebug>1) printf("SIOCDELMULTI ");
			for (i = 0; i < NMULTI - 1 ; i++) {
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					if (--sc->muse[i] == 0)
						bcopy(ne_multi,&sc->multi[i],MULTISIZE);
					if (neshowmulti) 
						printf("%d deleted.  \n",i);
				}
			}
		} else {
			if (nedebug>1) printf("SIOCADDDMULTI ");
			for (i = 0; i < NMULTI - 1 ; i++) {
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					sc->muse[i]++;
					if (neshowmulti) printf("already using index %d\n",i);
					goto done;
				}
				if (bcmp(&sc->multi[i],ne_multi,MULTISIZE) == 0)
					j = i;
			}
			/*
			 * j is initialized to -1; if j > 0, then
			 * represents the last valid unused location
			 * in the multicast table.
			 */
			if (j == -1) {
				printf("ne%d: SIOCADDMULTI failed, multicast list full: %d\n",ifp->if_unit,NMULTI);
				error = ENOBUFS;
				goto done;
			}
			bcopy(ifr->ifr_addr.sa_data, &sc->multi[j], MULTISIZE);
			sc->muse[j]++;

			if (neshowmulti)
				printf("added index %d.\n", j);
		}
		for ( i = 0; i < 6; i++ )
			sc->ne_setup[0].setup_char[i] = sc->is_addr[i];
		if (ifp->if_flags & IFF_RUNNING) {
			nesetup( sc, 0 );
			nestart ( ifp->if_unit );
		} else
			nesetup( sc, 0 );
		break;

	case SIOCRDCTRS:
	case SIOCRDZCTRS:

		if (nedebug>1) printf("SIOCRDCTRS ");
		ctr->ctr_ether = sc->ctrblk;
		ctr->ctr_type = CTR_ETHER;
		ctr->ctr_ether.est_seconds = (time.tv_sec - sc->ztime) > 0xfffe ? 0xffff : (time.tv_sec - sc->ztime);
		if (cmd == SIOCRDZCTRS) {
			if (nedebug>1) printf("SIOCRDZCTRS ");
			sc->ztime = time.tv_sec;
			bzero(&sc->ctrblk, sizeof(struct estat));
		}
		break;

	case SIOCSIFADDR:
		if (nedebug>1) printf("SIOCSIFADDR ");
		ifp->if_flags |= IFF_UP;
		neinit(ifp->if_unit);
		switch(ifa->ifa_addr.sa_family) {
#ifdef INET
		case AF_INET:
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif

		default:
			if (pr = iffamily_to_proto(ifa->ifa_addr.sa_family)) {
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
		if (ifp->if_flags & IFF_RUNNING) { 
			ifp->if_flags &= ~IFF_RUNNING;  
			neinit( ifp->if_unit );
			}
		break;
#endif IFF_PROMISC
	default:
		error = EINVAL;

	}
done:	splx(s);
	return (error);
}

/*
 * Put an mbuf chain into the appropriate transmit mbuf
 */
neput(sc, index, m)
struct ne_softc *sc;
int index;
register struct mbuf *m;
{
	register caddr_t dp,bp;
	register int len = 0;
	struct mbuf *m0;

	m0 = m;
	bp = mtod(sc->tmbuf[index],caddr_t);
	while(m) {
		if (m->m_len != 0) { 
			dp = mtod(m,caddr_t);
			bcopy(dp,bp,(unsigned)m->m_len);
			bp += m->m_len;
			len += m->m_len;
		}
		m = m->m_next;
	}
	
	if(len < MINDATA) {
		bzero(bp,MINDATA - len);
		len = MINDATA;
	}

	/* 
	 * save the mbuf chain pointer
	 */
	sc->smbuf[index]=m0;

return(len);
}

/*			 
 * fill out the setup packet - the physical address will already be
 * presented in first row
 */
nesetup(sc,flag)
struct ne_softc *sc;
int flag;
{
	register int i, j;

	/* 
	 * fill out the broadcast address to the rest of the entries 
	 * the first one will be the station address. The second one
         * will be all 1's 
	 *  
	 */
	for( j=1; j <= 15  ; j ++ )
		bcopy(ne_multi,&sc->ne_setup[j],6);
	/* 
	 * If there have multicast addresses, just fill in the rest of
	 * setup frame. 
	 */
	for( i = 0, j = 2 ; i < NMULTI - 2 ; i++) {
		if(bcmp(&sc->multi[i],ne_multi,6) != 0) {
			bcopy(&sc->multi[i],&sc->ne_setup[j],6);	
			j++;
		}
	}
	sc->setupqueued++;
	if(flag)
		sc->ne_setup_ic = 1;
}

			
/* 
 * Initialize a ring descriptor
 */
neinitdesc(descp,m,type) 
register NEDESC *descp;
register struct mbuf *m;
int type;
{

	/* 
	 * Zero the descriptor
	 */
	 bzero (descp, sizeof(NEDESC));

	/* 
	 * Tie cluster mbuf to ring descriptor. Bump up m_off by 2 bytes
	 * in order to satisfy NFS, which needs longword-aligned data.
	 * (Otherwise, the 14 byte ethernet header would place the
	 * data at a non-longword alignment.
	 */
	if(type)
		m->m_off +=2;

	/* +4 for CRC */
	descp->ne_bsize = ETHERMTU + sizeof(struct ether_header) + 4; 
#ifdef vax
	descp->ne_bfaddr = svtopte(mtod(m,u_long *));
	descp->ne_pg_off = (unsigned)dp&PGOFFSET;
	descp->ne_info = NE_VA;
#else 
	descp->ne_bfaddr = svtophy(mtod(m,u_long *));
	descp->ne_info = 0; 
#endif
	return;
}

/*
 * Reset the SGEC chip, check the selftest, reallocate the mbuf 
 */
nereset(sc)
struct ne_softc *sc;
{
	struct ifnet *ifp = &sc->is_if;
	int i;

	neresets++;
	/*
	 * Stop network activity
	 */
	if (ifp->if_flags & IFF_RUNNING) 
                ifp->if_flags &= ~(IFF_UP | IFF_RUNNING);

	if (nedebug)
		mprintf("nereset: reseted ne%d %d\n",ifp->if_unit,neresets);
	nesoftreset(sc,ifp->if_unit);
	nesetcsr0(sc,ifp->if_unit);
#ifdef vax
	csrpt->csr7 = (u_long) mfpr(SBR);
#endif
	/*
	 * free up all the xmt mbufs 
	 */
	for (i=0; i < sc->ntring; i++) {
		if(sc->tmbuf[i])
			m_freem(sc->tmbuf[i]);
		sc->tmbuf[i] = (struct mbuf *)NULL;
		if(sc->smbuf[i])
			m_freem(sc->smbuf[i]);
		sc->smbuf[i] = (struct mbuf *)NULL;
	}
	/*
	 * free up all the rcv mbufs
	 */
	for (i=0; i < sc->nrring; i++) {
		if(sc->rmbuf[i])
			m_freem(sc->rmbuf[i]);
		sc->rmbuf[i] = (struct mbuf *)NULL;
	}
	neinit(ifp->if_unit);
}	

/*
 * Reset the SGEC chip, check the selftest result and initialize the
 * CSR1
 */
nesoftreset(sc,unit)
struct ne_softc *sc;
int unit;
{
	register NECSRS *csrpt = sc->ne_csrs; 
	int delay;
	/* 
	 * Reset the SGEC chip 
	 * Wait up to 25ms for complete the reset
	 * and initialization
 	 */
	csrpt->csr6 = NE_CSR6_RE ;

	for(delay=100; !(csrpt->csr5 & NE_CSR5_ID) || delay < 0 ; delay--) 
			DELAY(2500);
	if (!(csrpt->csr5 & NE_CSR5_ID)) {
			printf("ne%d : cannot reset the SGEC chip \n",unit);
			return(0);
			}
	/*
 	 * self test check routine 
 	 * SGEC self test takes 25ms to complete after Hardware or software
 	 * reset 
 	 */
	if(!(csrpt->csr5 & NE_CSR5_SF )) 
		return ((int)csrpt); /* selftest success */
	else if(csrpt->csr5 & NE_SS_ROM) 
		printf("ne%d : self test failed - ROM error\n",unit); 
	else if(csrpt->csr5 & NE_SS_RAM)
		printf("ne%d : self test failed - RAM error\n",unit);
	else if(csrpt->csr5 & NE_SS_AFR)
		printf("ne%d : self test failed - Address Filter RAM error\n",unit);
	else if(csrpt->csr5 & NE_SS_TFF)
		printf("ne%d : self test failed - Transmit FIFO error\n",unit);
	else if(csrpt->csr5 & NE_SS_RFF)
		printf("ne%d : self test failed - Receive FIFO error\n",unit);
	else if(csrpt->csr5 & NE_SS_SLE)
		printf("ne%d : self test failed - Lookback error\n",unit);
	return(0);
}

/* 
 * initialize the CSR0 with IPL, SA and IV
 */
nesetcsr0(sc,unit)
struct ne_softc *sc;
int unit;
{
	u_long csr=sc->ne_initcsr;
	NECSRS *csrpt=sc->ne_csrs;
	int s,retry,csr0;

	s = splimp();  /* raise the ipl to disable other interrupt */ 
	for (retry=5; (csr0 != sc->ne_initcsr) && (retry >= 0); retry--) 
		{  
		csrpt->csr0 = sc->ne_initcsr;
		wbflush();
		csr0 = csrpt->csr0 ;	
		}
	splx(s);
	if(csr0 != sc->ne_initcsr) { 
		printf("ne %d : fail to write CSR0",unit);
		return(0);
		}
	else {
		/* check for the pending parrity error interrupt */
		if( csrpt->csr5 & NE_CSR5_ME )
			csrpt->csr5 = NE_CSR5_IS | NE_CSR5_ME ; 
		return(csr);
		}

}

newatch(unit)
int unit;
{
	register struct ne_softc *sc = &ne_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	int s;

	s = splimp();
	ifp->if_timer = 0;
	nereset(sc);
	splx(s);
}
