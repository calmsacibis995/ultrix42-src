#ifndef lint
static char *sccsid = "@(#)if_fza.c	4.14      (ULTRIX)        4/30/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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


/*-----------------------------------------------------------------------
 * Modification History: 
 *
 * 02-APR-91   jsd
 *	Allow packets to be looped back if COPYALL mode is set
 *
 * 27-FEB-91   chc
 *	Added code to make the interface state consitently before and
 *	after the EEPROM update.
 *      
 * 19-DEC-90   chc
 * 	Fixed the counter initialization problem. Also allow to
 *	read counter while the adapter is in the MAINTENANCE
 *      state.
 *
 * 31-Oct-90   chc
 *	Added code to fix the SMT chained and SMT 6.2 duplicated
 *	address detected problems. Also added code to support the
 *	read status. 
 *
 * 5-Oct-90	chc
 *	Added code to sync the firmware and fixed the counter.
 *
 * 7-Sep-90	chc
 *	Added code to support FDDI MIB
 *
 * 9-Aug-90	chc (Chran-Ham Chang)
 *	Bugs Fixed and support the EEPROM update
 *
 * 27-Apr-90 	chc (Chran-Ham Chang)
 *	Created the if_fza.c module   
 *	Derived from if_xna.c.
 *
 *---------------------------------------------------------------------- */
#include "fza.h"

#if NFZA > 0 || defined(BINARY)

#include "packetfilter.h"       /* NPACKETFILTER */
#include "../data/if_fza_data.c"
#include "../h/types.h"
#include "../h/errlog.h"

extern	struct protosw *iftype_to_proto(), *iffamily_to_proto();
extern	struct timeval	time;
int	fzaattach(), fzaintr(), fzaprobe(),fzastart();
int	fzainit(),fzaioctl(),fzareset(),fzawatch();

u_short fzastd[] = { 0 };
struct  uba_driver fzadriver =
	{ fzaprobe, 0 ,fzaattach, 0 , fzastd, "fza", fzainfo };

extern  int net_output();
struct	mbuf *fzamkparam();
FZACMDRING *fzamkcmd();
u_char  fza_dbeacon[] = {0x01, 0x80, 0xc2, 0x00, 0x01, 0x00};
u_char  fza_rpurge[] = {0x09, 0x00, 0x2b, 0x02, 0x01, 0x05};
u_char  fza_notset[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int fzadebug = 0 ;
int dludebug = 0 ;

#define XMTSEGSIZE	512

#define FZAMCLGET(m) { \
	register struct mbuf *p; \
	MGET((m), M_DONTWAIT, MT_DATA); \
	if ((m)) { \
		MCLGET((m), p); \
		if (p == 0) { \
			m_freem((m)); \
			(m) = (struct mbuf *)NULL; \
		} \
	} \
}

#define	FZATIMEOUT(cp,timeout ) { \
	register int s = splnet(); \
	register long sec0 = time.tv_sec; \
	while ( !(cp->own) && ((time.tv_sec - sec0) < 15 )); \
	if ( !(cp->own) ) \
		timeout = 1; \
	splx(s); \
}

/*
 * Probe the FZA to see if there 
 * 
 */
fzaprobe(reg,ui)
	caddr_t reg;
	struct	uba_device *ui;
{
	register struct	fza_softc *sc = &fza_softc[ui->ui_unit];
	register int delay;
	union fzacmd_buf *cmdpt;
	FZARCVRING *rp;
	FZAXMTRING *tp;
	struct rmbuf *bp;
	struct mbuf *m,*m1;
	int unit = ui->ui_unit;
	caddr_t mp;
	int i;

	sc->basereg = PHYS_TO_K1((u_long)reg);
	/* fill out the register address and others */
	sc->resetaddr = (struct fzareg  *)(sc->basereg + FZA_RESET);	
	sc->ctlaaddr = (struct fzareg *)(sc->basereg + FZA_CTL_A);	
	sc->ctlbaddr = (struct fzareg *)(sc->basereg + FZA_CTL_B);	
	sc->intraddr = (struct fzareg *)(sc->basereg + FZA_INT_EVENT);	
	sc->maskaddr = (struct fzareg *)(sc->basereg + FZA_INT_MASK);	
	sc->statusaddr  = (struct fzareg *)(sc->basereg + FZA_STATUS );	
	
	/* fill out the cmd and unsolicited ring address */
	sc->cmdring = (FZACMDRING *)(sc->basereg + FZACMD_PHY_ADDR);
	sc->unsring = (FZACMDRING *)(sc->basereg + FZAUNS_PHY_ADDR); 


	/* turn on the driver active mode */
	sc->reg_ctlb = FZA_ACTIVE ;

	/* Clear the interrupt event register */
	sc->reg_mask = FZA_INTR_MASK;
	sc->reg_intr = sc->reg_intr;
	wbflush();

	/*
	 * perform the adapter self test
	*/

	if(!fzaselftest(sc,unit)) {
		printf("fza%d: fzaprobe selftest fail \n",unit);
		return(0);
	}
	
	/* set the interrup mask */
	sc->reg_mask = FZA_INTR_MASK;


	/*
	 * Allocate space for the ctrblk, initblk and statusblk
	 */
	 KM_ALLOC(mp, caddr_t, 512 * 3  , KM_DEVBUF, KM_NOW_CL_CO_CA);
	 if (mp == 0) {
		printf("fza%d: Couldn't allocate memory for internal structure\n", unit);
		return(0);
	}
	sc->ctrblk = (struct _fzactrs *)mp;
	sc->initblk =  (struct fzainit *)(mp + 512);
	sc->statusblk = (struct fzastatus *)(mp + 512 * 2);

	/*
	 * initialize all the indexes 
	 *
         * sc->tindex == index of next transmit desc. to activate
         * sc->ltindex  == index of last transmit processed
         * sc->nxmit  == # of transmit pending
         * sc->rindex == index of next recv. descriptor to activate
         * sc->tsmtindex  == index of next SMT tansmit to CNS
         * sc->rsmtindex  == index of next SMT receive from CNS
         * sc->cmdindex  == index of next command desc. to activate
         * sc->lcmdindex  == index of last comman processed
         * sc->unsindex ==  index of next unsolicited desc. to activate
         */
        sc->tindex = sc->nxmit = sc->rindex = 0;
        sc->tsmtindex = sc->rsmtindex = 0;
        sc->cmdindex = sc->unsindex = 0;
        sc->ltindex = sc->lcmdindex = -1;
	
	sc->smtrmcindex = sc->lsmtrmcindex = 0;	
	sc->initflag = 1;
	/*
	 * We are now guaranteed that the port has succesfully completed
	 * self test and is in the "UNINITIALIZED" state. At this point,
	 * we can prepare a command buffer to issue the INIT command
	 * 
	 */
	if(!fzacmdinit(sc,unit)) {
		printf("fzaprobe init command fail \n",unit);
		KM_FREE(mp,KM_DEVBUF);
		return(0);
	}
	
	sc->reg_intr = sc->reg_intr;
	/* 
	 * get the initial value from the INIT command
	 * fill out the init value 
	 */
	fzafillinit(sc,sc->initblk);

	sc->reg_mask = FZA_INTR_MASK;

	/*
	 * put two multicast addresses :
	 *	
 	 *	Ring Purger	: 09-00-2b-02-01-05
	 *	Directed Beacon : 01-80-c2-00-01-00
	 */
	bcopy(fza_rpurge,&sc->is_multi[0][0],6);
	bcopy(fza_dbeacon,&sc->is_multi[1][0],6);
	sc->is_muse[0] = sc->is_muse[1] = 1;
	
	/*
	 * allocate mbufs for receive ring
	 */

	for (i = 0, rp = &sc->rring[0], bp = &sc->rmbuf[0]; (i < NFZARCV ); 
					i++, rp++, bp++) {
		FZAMCLGET(m)
		if(m) {
			FZAMCLGET(m1)
			if (m1) {
				fzainitdesc(rp, bp, m, m1);
			} else {
				goto fail;
				break;
			}
		} else {
			goto fail;	
			break;
		}

	}
	/*
	 * clear the transmit ring mbuf entry
	 */
	for ( i=0, tp = &sc->tring[0]; i < sc->nrmcxmt ; i++, tp++) 
			tp->xmt_mbuf = (struct mbuf *)0;	


	return (sizeof(struct fza_softc));

fail:
	KM_FREE(mp,KM_DEVBUF);
	for (i = 0, bp = &sc->rmbuf[0]; (i < NFZARCV ); i++, bp++) {
		if(bp->mbufa != NULL)
			m_freem(bp->mbufa);
		if(bp->mbufb != NULL)
			m_freem(bp->mbufb);
	}
	printf("fza%d: fzaprobe fail - can not allocate enough receive buffer",unit);
	return(0);	
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
fzaattach(ui)
	struct uba_device *ui;
{
	register struct fza_softc *sc = &fza_softc[ui->ui_unit];
	register struct ifnet *ifp = &sc->is_if;
	struct sockaddr_in *sin;
	register int i;

	/* initialize the lock */
	lockinit(&sc->lk_fza_softc, &lock_device15_d);
	
	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "fza";
	ifp->if_mtu = FDDIMTU;
	ifp->if_flags |= IFF_BROADCAST|IFF_DYNPROTO|IFF_802HDR| IFF_NOTRAILERS;
	ifp->if_type = IFT_FDDI;
	ifp->lk_softc = &sc->lk_fza_softc;
	((struct arpcom *)ifp)->ac_ipaddr.s_addr = 0;
	ifp->if_sysid_type = 119; 

	/*
	 * Initialize multicast address table
	 */
	for (i = 2; i < NMULTI; i++) {
		bcopy((caddr_t)etherbroadcastaddr, sc->is_multi[i], 6);
		sc->is_muse[i] = 0;
	}

	/*
	 * Set maxlen for the SMT receive queue; attach interface
	 */
	sc->is_smt.ifq_maxlen = IFQ_MAXLEN;
	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
	ifp->if_output = net_output;
	ifp->if_start = fzastart;
	ifp->if_init = fzainit;
	ifp->if_ioctl = fzaioctl;
	ifp->if_reset = fzareset;
	ifp->if_watchdog = fzawatch;
	ifp->if_timer = 1;
	ifp->d_affinity = ALLCPU;

	/*
	 * Hook up the device interrupt vector to begin allowing interrupts.
	 * We know that the device has been successfully initialized at this
	 * point. Initialize if_version string.
	 */
	bcopy("DEC DEFZA FDDI Interface", ifp->if_version, 25);
	ifp->if_version[25] = '\0';

	printf("fza%d: %s, hardware address %s ROM rev %c%c%c%c Firmware rev %c%c%c%c \n", 
		ifp->if_unit, ifp->if_version, ether_sprintf(sc->is_dpaddr),
		sc->phy_rev[0],sc->phy_rev[1],sc->phy_rev[2],sc->phy_rev[3],
		sc->fw_rev[0],sc->fw_rev[1],sc->fw_rev[2],sc->fw_rev[3]);
#if	NPACKETFILTER > 0
	attachpfilter(&(sc->is_ed));
#endif	NPACKETFILTER

	if_attach(ifp);
}

/*
 * Initialize interface. May be called by a user process or a software
 * interrupt scheduled by fzareset().
 */
fzainit(unit)
	int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];
	register union fzacmd_buf *xcmd;
	register struct mbuf *m;
	register int delay;
	FZACMDRING *cp; 
	struct ifnet *ifp = &sc->is_if;
	int i, s, t,timeout=0;

	/* not yet, if address still unknown */

	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;
	if (ifp->if_flags & IFF_RUNNING)
		return;

	/*
	 * Lock the softc to ensure coherency of softc data needed to fill
	 * out the command buffers by "fzamkcmd". 
	 */
	t = splnet();
	s = splimp();
	smp_lock(&sc->lk_fza_softc, LK_RETRY);

	/* clear the interrupt event register */
	sc->reg_intr = sc->reg_intr;
	wbflush();


	
	/*
	 * Issue the MODCAM command to set up the CAM addresses
         */
	if(cp=fzamkcmd(sc,CMD_MODCAM,unit)) {
		smp_unlock(&sc->lk_fza_softc);
                splx(s);
                FZATIMEOUT(cp,timeout)
                s = splimp();
                smp_lock(&sc->lk_fza_softc,LK_RETRY);
                if(timeout) {
			printf ("fza%d: port modify CAM command  timeout \n", unit);
                        goto done;
		 } else if ( cp->status_id != CMD_SUCCESS ) {
                        printf("fza%d: port init failed\n", unit);
                        goto done;
                }
        } else {
                printf("fza%d: port init failed\n", unit);
                goto done;
        }
					
	/*
	 * Issue a "PARAM" command to fill in the initial  
	 * system parameters. Wait for completion,
	 * which will be signaled by the interrupt handler.
	 */
	if(cp=fzamkcmd(sc, CMD_PARAM, unit)) {  
		smp_unlock(&sc->lk_fza_softc);
                splx(s);
                FZATIMEOUT(cp,timeout)
                s = splimp();
                smp_lock(&sc->lk_fza_softc, LK_RETRY);
                if(timeout) {
                        printf ("fza%d: port PARAM command  timeout \n", unit);
                        goto done;
                } else if ( cp->status_id != CMD_SUCCESS ) {
                        printf("fza%d: port init failed\n", unit);
                        goto done;
                }
        } else {
                printf("fza%d: port init failed\n", unit);
                goto done;
        }


	if (ifp->if_flags & IFF_PROMISC) {
		if (cp =fzamkcmd(sc, CMD_MODPROM,ifp->if_unit)) {
			smp_unlock(&sc->lk_fza_softc);
			splx(s);
			FZATIMEOUT(cp,timeout)
			if(timeout) {
				printf ("fza%d: port MODPROM command timeout \n",unit);
				goto done;
			} else if (cp->status_id != CMD_SUCCESS) { 
                        	printf("fza%d: port init failed\n", unit);
                        	goto done;
			}
		} else {
                        printf("fza%d: port init failed\n", unit);
                        goto done;
		}
	}
	/*
	 * Mark interface up and running; start output
	 * on device.
	 */
	sc->is_if.if_flags |= (IFF_UP|IFF_RUNNING);
	sc->is_if.if_flags &= ~IFF_OACTIVE; 
	if(sc->initflag) {
		sc->ztime = time.tv_sec;
		sc->initflag = 0;
	}
	if (sc->is_if.if_snd.ifq_head)
		fzastart(unit);		/* queue output packets */
done:
	/*
	 * Relinqusih softc lock, drop IPL.
	 */
	smp_unlock(&sc->lk_fza_softc);
	splx(s); 
	splx(t);

	if(fzadebug >  0 )
		PRT_REG(sc)
	return;
}



/*
 * FZA  start routine. Strings output packets and SMT output packets  
 * onto the transmit ring. 
 */
fzastart(unit)
	int unit;
{
	register FZAXMTRING  *tp;
	register struct mbuf *m;
	register int index;

	struct fza_softc *sc = &fza_softc[unit];
	struct ifnet *ifp = &sc->is_if;
	FZAXMTRING *tpstart;
	struct mbuf *m0;
	int last,totlen;
	u_long tmp;
	/*
	 * Process the transmit queues.
	 */
	for (index = sc->tindex, tp = &sc->tring[index] ; index !=
sc->ltindex && !(tp->own & FZA_RMC_OWN) ;
	     index = ++index % sc->nrmcxmt , tp = &sc->tring[index]) {
next_m:
		if (sc->is_if.if_snd.ifq_head ) {
			IF_DEQUEUE(&sc->is_if.if_snd, m0);
		} else  {
			sc->tindex = index;
			return;
		}

		/* 
		 * get the total packet size and check the owner ship
 		 * bit for the required number of RMC descriptors
		 */
		m = m0;
		totlen = 0;
		while (m) {
			totlen += m->m_len ;
			m = m->m_next;
		}

		/*
		 * two bytes Preamble and one byte starting delimiter 
		 */
		if ( totlen > FDDIMAX )  {
			/* 
			 * drop this packet for size too long *
			 */ 
			if(fzadebug)
				printf("fza%d: size too long %d",unit,totlen);
			m_freem(m0);
			goto next_m;
		}
		
		/* 
		 * the last descriptor not own by host
		 * put the mbuf back to the queue 
		 */
		last = index + totlen/XMTSEGSIZE - (((totlen % XMTSEGSIZE) != 0)? 0 : 1 );
		if ( sc->tring[last % sc->nrmcxmt].own == FZA_RMC_OWN ) {
			IF_PREPEND(&sc->is_if.if_snd, m0); 
			sc->tindex = index;
			return;
		}
		
			
		/*
		 * String the mbuf chain onto the transmit ring entry's
		 * buffer segment addresses.
		 * Architectures which need to use program I/O to data copy
		 * mbuf chain into one or several ( maximum 9 ) RMC transimit
		 * rings which are 512 bytes segments 
		 */
		if(fzadebug > 1 )
			printf("fzastart: ready to copy data len %d \n",totlen);
		tpstart = tp;
		{
		register int tleft = 512 , mleft;
		register  caddr_t mptr, tptr;

		int space;
		int	oindex, nring = 1;
		u_long	rmc;	

		 
			
		/*
		 *	rleft : space left for current RMC XMT ring
		 *	mleft : bytes left for copying in current mbuf 
		 */

		m = m0;
		tptr = (caddr_t)FZAXMTADDR(tp,sc->basereg); 
		oindex = index ;

		while (m) {
			mleft = m->m_len ;
			mptr = mtod(m, caddr_t);
			while (mleft) { 
				space = MIN(tleft,mleft); 
				fzacpy(mptr, tptr , space );
				tptr   +=  space ;	
				mptr   += space ;
				mleft  -= space  ;  
				tleft  -= space ;
				if (tleft == 0 && mleft > 0 ) {
					index = ++index % sc->nrmcxmt ;
					sc->tring[index].own = FZA_RMC_OWN;
					sc->tring[index].rmc = 0;
					if(index == 0 )
						tptr = (caddr_t)FZAXMTADDR(&sc->tring[index],sc->basereg);
					tleft = 512;
					nring++;
				}	
			}	
			
		m = m->m_next;
		}

		/*
		 * need more than one XMT rings
		 */
		if( nring > 1 ) { 
			sc->tring[index].rmc = FZA_EOP ;
			tpstart->rmc = FZA_SOP | FZA_XMT_VBC | totlen ;	
		} else
			tpstart->rmc = FZA_SOP | FZA_EOP | FZA_XMT_VBC | totlen ;	
		/*
		 * save the mbuf chain
		 */
		tpstart->xmt_mbuf = m0;
		tpstart->own = FZA_RMC_OWN;
		
		/* 
		 * this is try to slove the RMC pipe line problem
		 */
		tmp = tpstart->own ; 

		if(fzadebug > 1)
			printf("fzastart index %d rmc 0x%x \n",index,tpstart->own);

		}
		sc->nxmit++;
		ifp->if_flags |= IFF_OACTIVE ;
		/*
		 * Advise the port that a new transmit packet is pending
		 */
		sc->reg_ctla |= XMT_POLL ; 
		wbflush();

	}
	sc->tindex = index;
}


/*
 * FZA device interrupt handler
 */
fzaintr(unit)
	int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];
	register FZASMTRING *smttp;
	register FZAXMTRING *tp;
	register int index;

	struct mbuf *m0;
	struct ifnet *ifp = &sc->is_if;
	u_short csr ; 

	int s = splimp();

	csr = sc->reg_intr;

	/* clear the interrupt event register */
	sc->reg_intr = csr;	

	/*
	 * Lock softc, since we will be updating the per-unit ring pointers
	 * and active ring entry counts frequently.
	 */
	smp_lock(&sc->lk_fza_softc, LK_RETRY);

	/*
	 * See if we got here due to a port state change interrupt. 
	 */
	if (csr & STATE_CHG )  {
		switch ((sc->reg_status & ADAPTER_STATE)) {    
			case STATE_HALTED :
					printf("fza%d: port in the Halt State -> ",unit);
					switch(sc->reg_status & ID_FIELD ) {
						case HALT_UNKNOWN:
							printf("Unknown reason\n");
							break;
						case HALT_HOST_DIRECTED:
							printf("Host request to halt \n");
							break;
						case HALT_HBUS_PARITY:
							printf("Host Maxbus parity errors \n");
							break;
						case HALT_HNXM:
							printf("Host Non-exist Memory \n");
							break;
						case HALT_ADAP_SW_FAULT:
							printf("Adapter software fault\n");
							break;	
						case HALT_ADAP_HW_FAULT:
							printf("Adapter hardware fault\n");
							break;	
						case HALT_CNS_PC_TEST:
							printf("CNS PC trace path test\n");
							sc->flag = FZA_PC_TRACED ;
							break;
						case HALT_CNS_SW_FAULT:
							printf("CNS software fault\n");
							break;
						case HALT_CNS_HW_FAULT:
							printf("CNS hardware fault\n");
							break;
						default:
							printf("unknown halt id %d\n",sc->reg_status & ADAPTER_STATE);
							break;
					}
					{
						struct el_rec *elrp;
					
						if((elrp = ealloc(sizeof(struct el_fza), EL_PRILOW))) {
							struct el_fza *elbod = &elrp->el_body.el_fza;
							bcopy(((char *)(sc->basereg + FZA_DLU_ADDR )),elbod,sizeof(struct
el_fza));
							LSUBID(elrp,ELCT_DCNTL,ELFZA,0,0,unit,(sc->reg_status & ID_FIELD));
						} 
						EVALID(elrp);
					}
					/*
					 * This is used for HALT recovrey.
					 * It will bring back the
					 * RUNNING state that help the
					 * link unavilable before the 
					 * adapter change to HALT state
					 * case.
					 */
					if (!(ifp->if_flags & IFF_RUNNING)) 
						ifp->if_flags |= IFF_RUNNING ;
					fzareset(unit);
					break;

			case STATE_RESET:
				if(fzadebug)
					cprintf("fza%d: port in the Resetting State \n",unit);
				break;
			case STATE_UNINITIALIZED:
				 if(fzadebug)
                                        cprintf("fza%d: port in the UninitialiedState \n",unit);
                                break; 
			case STATE_INITIALIZED:
				if(fzadebug)
                                        cprintf("fza%d: port in the Initialized State \n",unit);
                                break;
			case STATE_RUNNING:
				if(fzadebug)
                                        cprintf("fza%d: port in the Running  State \n",unit);
                                break;
			case STATE_MAINTENANCE:
				if(fzadebug)
                                        cprintf("fza%d: port in the Maintenance  State \n",unit);
                                break;
			default: 
				printf("fza%d: Undefined state id  %d \n", unit, sc->reg_status & ADAPTER_STATE  );
				break;
			}
		/*
		 * turn off the interface because of unrecovery errors
		 */
		smp_unlock(&sc->lk_fza_softc);
		splx(s);
		return;
	}

	
	/*
	 * See if we get the FLUSH_TX interrupt. If so, the driver
	 * will return the ownership of all pending SMT XMT buffer back
	 * to the adapter and turn on the DTP bit in all the Tx descriptors 
	 * which is owned by the RMC 
	 */
	if(csr & FLUSH_TX) {

		if(fzadebug)
			printf("fza%d: flush the XMT buffer \n",unit);
		for ( index = sc->tsmtindex, smttp = &sc->smttring[index] ;
				 (smttp->own & FZA_HOST_OWN); 
			index = index++ % sc->nsmtxmt,smttp = &sc->smttring[index] )  
				smttp->own &= ~(FZA_HOST_OWN);
		sc->tsmtindex = index;

		/*
		 * Clean up the Tx ring
		 */ 

		for ( index = sc->tindex - 1  , tp = &sc->tring[index] ; 
			(index != sc->tindex) && (tp->own & FZA_RMC_OWN) ;) { 

			tp->rmc |= FZA_XMT_DTP ;
			if(--index < 0 )
				index =  sc->nrmcxmt-1;
			tp = &sc->tring[index];
		}

		sc->ltindex = sc->tindex - 1 ;
		sc->nxmit = 0;
		/*
		 * notice adapter the flush was done
		 */

		sc->reg_ctla |= FLUSH_DONE;
		wbflush();
		smp_unlock(&sc->lk_fza_softc);
		splx(s);
		return;
	}

			
	/*
	 * See if we get the LINK STATUS CHANGE interrupt. If so,
	 * we need to turn off or turn on the interface 
	 */
	if (csr & LINK_STATUS_CHG) {
		if(fzadebug)
			printf("fza%d: LINK STATE CHANGE \n",unit);
		if (sc->reg_status & LINK_AVAILABLE ) {	
			if (!(ifp->if_flags & IFF_RUNNING)) {
				ifp->if_flags |= IFF_RUNNING ;
				cprintf("fza%d: LINK available \n",unit);	
			}
		} else {
			if (ifp->if_flags & IFF_RUNNING ) {
				ifp->if_flags &= ~IFF_RUNNING;
				cprintf("fza%d: LINK unavailable \n",unit);
				/* 
		 		 * drop packets on the transmit queue
		 		 */
					while(sc->is_if.if_snd.ifq_head) {
					IF_DEQUEUE(&sc->is_if.if_snd, m0);
					m_freem(m0);
				}
			} 
		}

	}

	
	if (csr & RCV_POLL)  /* receive interrupt */
		fzarint(unit);

	if (csr & XMT_PKT_DONE) /* transmit done interrupt */ 
		fzatint(unit);

	if (csr & CMD_DONE ) /* command done interrupt */ 
		fzacmdint(unit);

	if (csr & SMT_XMT_POLL) /* SMT transmit done interrupt */ 
		fzasmtint(unit);

	if (csr & UNS_POLL ) {  /* unsolicited event interrupt */
		register int index;
		register FZAUNSRING *up;
	
		for(index = sc->unsindex, up= &sc->unsring[index];
				(up->own) ; index = ++index % NFZAUNS, 
					up = &sc->unsring[index] ) {
		/*
		 * process the unsolicited ring
		 */
			if(fzadebug)
				printf("fza%d: Unsolicited Event -> ",unit);
			else 
				mprintf("fza%d: Unsolicited Event -> ",unit);
			switch(up->status_id) {
				case UNS_UNDEFINED:
					if(fzadebug)
						printf(" Undefined \n");
					else
						mprintf(" Undefined \n");
					break;
				case UNS_RINGINIT:
					if(fzadebug)
						printf(" Ring Init Initiated \n");
					else
						mprintf(" Ring Init Initiated \n");
					break;
				case UNS_RINGINITRCV:
					if(fzadebug)
						printf(" Ring Init Received \n");
					else 
						mprintf(" Ring Init Received \n");
					break;
				case UNS_RINGBEACONINIT:
					if(fzadebug)
						printf(" Ring Beaconing Initiated \n");
					else
						mprintf(" Ring Beaconing Initiated \n");
					break;
				case UNS_DUPADDR:
					if(fzadebug)
						printf(" Duplicate Address Detected \n");
					else
						mprintf(" Duplicate Address Detected \n");
					break;
				case UNS_DUPTOKEN:
					if(fzadebug)
						printf(" Duplicated Token Detected \n");
					else
						mprintf(" Duplicated Token Detected \n");
					break;
				case UNS_RINGPURGEERR:
					if(fzadebug)
						printf(" Ring Purge Error \n");
					else
						mprintf(" Ring Purge Error \n");
					break;
				case UNS_BRIDGESTRIPERR:
					if(fzadebug)
						printf(" Bridge Strip Error \n");
					else
						mprintf(" Bridge Strip Error \n");
					break;
				case UNS_RINGOPOSC:
					if(fzadebug)
						printf(" Ring Op Oscillation \n");
					else
						mprintf(" Ring Op Oscillation \n");
					break;
				case UNS_DIRECTEDBEACON:
					if(fzadebug)
						printf(" Directed Beacon Received \n");
					else
						mprintf(" Directed Beacon Received \n");
					break;
				case UNS_PCINIT:
					if(fzadebug)
						printf("PC Trace Initiated \n"); 
					else
						mprintf("PC Trace Initiated \n"); 	
					break;
				case UNS_PCRCV:
					if(fzadebug)
						printf("PC Trace Received \n");
					else
						mprintf("PC Trace Received \n");
					break;
				case UNS_XMT_UNDERRUN:
					if(fzadebug)
						printf("Transmit Underrun \n");
					else
						mprintf("Transmit Underrun \n");
						
					break;
				case UNS_XMT_FAILURE:
					if(fzadebug)
						printf("Transmit Failure \n");
					else 
						mprintf("Transmit Failure \n");
					break;
				case UNS_RCV_OVERRUN:
					if(fzadebug)
						printf("Receive Overrun \n");
					else
						mprintf("Receive Overrun \n");
					break;
				default:
					if(fzadebug)
						printf("Unknown event %d \n",up->status_id);
					else
						mprintf("Unknown event %d \n",up->status_id);
					break;
			}
		up->own = 0 ; /* turn back the ownership to RMC */
		}
		sc->unsindex = index;
	} 
		
			
	 /* 
	  * Dequeue next SMT received packet from queue and copy to SMT RCV ring
	  */
	{
	FZASMTRING *sp = &sc->smtrring[sc->rsmtindex];
	register struct mbuf *m;
	register caddr_t bp;
	int len; 

	/*
	 * check for something need to be send to the SMT RCV ring
	 */
	while ( sc->is_smt.ifq_head && (sp->own & FZA_HOST_OWN) ) { 
			IF_DEQUEUE(&sc->is_smt, m0);
			bp = (caddr_t)(sc->basereg + sp->buf_addr);
			len = 0;
			m = m0;
			while(m) {
				fzacpy(mtod(m,caddr_t),bp,m->m_len);
				bp += m->m_len;
				len += m->m_len;
				m = m->m_next;
			}
			/*
			 * use the original RMC descriptor
			 */
			sp->rmc = FZA_SOP | FZA_EOP | sc->smt_rmc[sc->lsmtrmcindex];
			sp->own &= ~FZA_HOST_OWN;
			sc->lsmtrmcindex = ++sc->lsmtrmcindex % IFQ_MAXLEN; 
			m_freem(m0);
			sc->rsmtindex = ++sc->rsmtindex % sc->nsmtrcv;
			sp = &sc->smtrring[sc->rsmtindex];
		}
	/*
	 * ask adapter to process this smt frame
	 */
	sc->reg_ctla |= SMT_RCV_POLL ; 
	}
		
	/*
	 * Dequeue next transmit request if interface is no longer busy.
	 */
	if (sc->nxmit <= 0) {
		sc->is_if.if_flags &= ~IFF_OACTIVE;
		fzastart( unit );
	}

	/*
	 * Drop softc lock and return .
	 */

	smp_unlock(&sc->lk_fza_softc);
	splx(s);
}

/*
 * FZA smt interrupt routine
 */
fzasmtint(unit)
int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];
	register FZASMTRING *sp;
	register FZAXMTRING *tp;
	register int index;	

	FZAXMTRING *tpstart;
	caddr_t smtbp,tbp;
	int len,last,nring,tlen;
	u_long tmp;
	/*
	 * process SMT XMT frame
	 */
	for(index = sc->tsmtindex , sp = &sc->smttring[index];
		(sp->own & FZA_HOST_OWN); index = ++index % sc->nsmtxmt, 
					sp= &sc->smttring[index]) {
		tpstart = &sc->tring[sc->tindex];
		tlen = len = sp->rmc & RMC_PBC_MASK;
		
		/*
		 * check the last xmt descriptor 
		 */
		last = sc->tindex + len/XMTSEGSIZE - (((len % XMTSEGSIZE) != 0) ? 0 : 1);
		/*
		 * last descriptor not own by host
		 */
		if( sc->tring[last % sc->nsmtxmt].own & FZA_RMC_OWN ) {
			sc->tsmtindex = index ;
			return;
		} 
		
		/*
		 * copy the data from SMT XMT ring to RMC XMT ring
		 */
		nring = 0;	
		smtbp = (caddr_t)(sp->buf_addr + sc->basereg); 
		while(len > 0) {
			tp = &sc->tring[sc->tindex];
			tbp = (caddr_t)FZAXMTADDR(tp,sc->basereg);
			fzacpy(smtbp,tbp,MIN(len,XMTSEGSIZE));
			smtbp += MIN(len,XMTSEGSIZE);
			len -= MIN(len,XMTSEGSIZE);
                        if(nring) {
                                tp->own = FZA_RMC_OWN;
                                tp->rmc = 0;
			}
			sc->tindex = ++sc->tindex % sc->nrmcxmt;
			nring++;
		}
		if(nring > 1 ) {
				tpstart->rmc = FZA_SOP | FZA_XMT_VBC | tlen;
				if(sc->tindex)
					sc->tring[sc->tindex-1].rmc = FZA_EOP ;
				else
					sc->tring[sc->nrmcxmt-1].rmc = FZA_EOP;
		} else 
			tpstart->rmc = FZA_SOP | FZA_EOP | FZA_XMT_VBC | tlen;
		
		tpstart->own = FZA_RMC_OWN;
		tpstart->xmt_mbuf = (struct mbuf *)0;

		tmp = tpstart->own ; 
		/*
		 * give back the ownership to adapter
		 */
		sp->own &= ~FZA_HOST_OWN; 
		sc->nxmit++;
	}
	/* 
	 * notice the port that one or more transmit packet is pending
	 */
	sc->reg_ctla |= XMT_POLL;
	wbflush();
	sc->tsmtindex = index ;
}

	
/*
 * FZA command interrupt routine
 */
fzacmdint(unit)
int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];
	register FZACMDRING *cp;
	register FZACMD_BUF *xcmd;
	register int index;

	struct mbuf *m;

	for ( index = (sc->lcmdindex + 1) % NFZACMD, cp = &sc->cmdring[index];
		(index != sc->cmdindex ) && (cp->own); 
			index = ++index % NFZACMD ,cp = &sc->cmdring[index] ) {
		/*
		 * process the cmd descriptor
		 */
		if(cp->status_id == 0 ) { /* command successed */
			xcmd = (FZACMD_BUF *) (cp->buf_addr + sc->basereg) ; 
			switch(cp->cmdid) {
				case CMD_RDCNTR: /* copy the counter*/
					if(fzadebug)
						sc->fza_debug.cmdcnt.rdcntr++;
					bcopy(xcmd,sc->ctrblk,sizeof(struct _fzactrs));
					break;
				case CMD_STATUS:
					if(fzadebug)
						sc->fza_debug.cmdcnt.status++;
					bcopy(xcmd,sc->statusblk,sizeof(struct
fzastatus));
					break;
				case CMD_MODCAM:
					if(fzadebug)
						sc->fza_debug.cmdcnt.modcam++;
					break;
				case CMD_SETCHAR:
					if(fzadebug)
						sc->fza_debug.cmdcnt.setchar++;
					break;
				case CMD_RDCAM:
					if(fzadebug)
						sc->fza_debug.cmdcnt.rdcam++;
					bcopy(xcmd,&sc->is_multi[0][0],512);	
                                        break;
				case CMD_MODPROM:
					if(fzadebug)
						sc->fza_debug.cmdcnt.modprom++;
					break;
				case CMD_PARAM:
					if(fzadebug)
						sc->fza_debug.cmdcnt.param++;
					break;
				case CMD_INIT:
					if(fzadebug)
						sc->fza_debug.cmdcnt.init++;
					break;
				case CMD_NOP:
						break;
				default:
					if(fzadebug)
						mprintf("fza%d: unknown command id %d\n",unit,cp->cmdid);	
					break;
			}
		} else { /* error command */
			printf("fza%d: command failed, ",unit);
			fzacmdstatus(sc,cp->status_id,unit,"fzacmdint");	
		}	
	}
	if(index)
		sc->lcmdindex = index - 1;
	else 
		sc->lcmdindex = NFZACMD - 1;
}
/*
 * FZA transmit interrupt routine
 */
fzatint(unit)
int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];
	register FZAXMTRING *tp;
	register struct mbuf *m0;
	register int index,len = 0;

	struct mbuf *mp;
	struct fddi_header *fh;
	/*
	 * Process all outstanding transmits completed by
	 * the port.
	 */
	for (index = (sc->ltindex + 1)%sc->nrmcxmt ,tp = &sc->tring[index]; 
			(sc->nxmit > 0) && !(tp->own & FZA_RMC_OWN) ; 
		index = ++index % sc->nrmcxmt, tp = &sc->tring[index]) { 

		/* 
		 * if this is a start packet, process it. if not, do nothing
		 */

		if(tp->rmc & FZA_SOP ) { 
			/*
		 	 * Process xmit descriptor, we have no way to know the
		 	 * packet trasnmit status
		 	 */
			sc->is_if.if_opackets++;
			mp = (struct mbuf *)tp->xmt_mbuf;
			tp->xmt_mbuf = (struct mbuf *)0;
			if( tp->rmc & FZA_XMT_SUCCESS ) {
				fstc_bytesent += tp->rmc & RMC_PBC_MASK;
				if( fstc_pdusent != 0xffffffff)
					fstc_pdusent++;
				/*
		 	 	 * Loop back any LLC broadcasts we send
				 * 
				 * mp == 0 that means this is a SMT packet 
				 * this packet was copy from SMT XMT ring
		 	 	 */
				if(mp) { /* LLC packet */
					fh = mtod(mp, struct fddi_header *);
					if ( (!bcmp(&fh->fddi_dhost[0],
						etherbroadcastaddr, 6))
						|| (sc->is_if.if_flags & IFF_PFCOPYALL)
						) {
						m0 = mp;
						while (m0) {
							len += m0->m_len;
							m0 = m0->m_next;
						}
						fzaread(sc, 0, len, mp);
					} else {
						if( fh->fddi_dhost[0] & 1 ) {
							fstc_mbytesent += tp->rmc & RMC_PBC_MASK;
							if(fstc_mpdusent != 0xffffffff)
							fstc_mpdusent++;
						}
						m_freem(mp);
					}
				}
			} else {
				if(mp)
					m_freem(mp);
				sc->is_if.if_oerrors++;
			}

			sc->nxmit--;
		}

	}
	if(index)
		sc->ltindex = index - 1;	/* Last xmit processed */
	else
		sc->ltindex = sc->nrmcxmt - 1;
}
	


/*
 * FZA receive interrupt routine 
 */
fzarint(unit)
int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];
	register int index,len;
	register FZARCVRING *rp;

	struct mbuf *m,*m1,*mp;
	struct fddi_header *fptr;
	struct rmbuf *bp;
	int nrcv = 0;
	int fddi_type;
	/*
	 * Process all incoming packets on the receive ring. Stop if
	 * we get to the current receive index to avoid locking out
	 * the system, but give back one descriptor for each one we
	 * process to keep the device busy.
	 * 
	 * 
	 */
	for (index = sc->rindex, rp = &sc->rring[index], bp = &sc->rmbuf[index];
			(rp->rcv_own & FZA_RCV_OWN) && nrcv < NFZARCV - 1;
	     index = sc->rindex = ++index % NFZARCV, rp = &sc->rring[index], bp = &sc->rmbuf[index], nrcv++) {
		/*
		 * check the DMA RCV status. If no error, process it
		 * we only process the LLC and SMT frame for decword
		 */
		if(!(rp->rmc & FZA_RCV_ERROR)) {
			len = rp->rmc & RMC_PBC_MASK  ;
			if ( len > FDDIMAX ) { /* Frame too long */
				if(fstc_pdulen != 0xffffffff)
					fstc_pdulen++;
				goto error;
			} else {
			fptr = (struct fddi_header *)PHYS_TO_K1(bp->phymbufa);
				switch (fptr->fddi_fc & ~FDDIFC_Z ) { /* take out the priority */
					case FDDIFC_LLC_ASYNC: /* for LLC frame */
					case FDDIFC_LLC_SYNC: 
						if( len < FDDILLCMIM ) {
							if(fzadebug)
								printf("fza%d: LLC frame too short - frame len %d",unit,len);
						if(fstc_pdulen != 0xffffffff)
					  	        fstc_pdulen++;
							goto error;
						}
						fddi_type = FZA_LLC;
			/*
			 * The length reported by RMC is including one
			 * byte Frame Control, real data and 4 bytes CRC. 
			 * The driver interprets the frame as 4 bytes
			 * FDDI header ( including one byte Frame Control)  
			 * and real data. So, we need to decrement one
		  	 * for the length. 
			 */ 
						len--;
						break;	
					case FDDIFC_SMT:	/* for SMT frame */
						if( len < FDDISMTMIM) {
							if(fzadebug)
								printf("fza%d: LLC frame too short - frame len %d",unit,len);
							if(fstc_pdulen != 0xffffffff)
						  	        fstc_pdulen++;
							goto error;
						}
						fddi_type = FZA_SMT;
					/*
                                         * mismatch with the firmware
                                         * RMC told us the wrong length
                                         */
						len = len + 3 ;

						break;
					case FDDIFC_MAC:
					default:
						if(fzadebug)
							mprintf("fza%d: unrecognize frame FC 0x%2x\n",unit,fptr->fddi_fc);	
						fzanundrop++;
						goto error;
						break;
				}
			}
				
			/*
			 * Allocate a pair of new mbufs for the current
			 * receive descriptor. 
			 *
			 * Each receive buffer will use two cluster mbufs.
			 * 
			 * The first cut will be :
			 *
			 * If the size of packet less than 4K, then the second
			 * mbuf will be reused.
			 *
			 * The second cut will be:
			 *
 			 * If the size of FDDI frame less than the small
			 * mbuf length (which is MLEN = 108), driver will 
			 * allocate a small mbuf then data copy the
			 * packet to it. This will save a large cluster mbuf. 
			 *
			 * In addition, if the second mbuf only is used for 
			 * less than MLEN size, a small mbuf will be allocated  
			 * and data copy the rest of packet from the second 
			 * cluster mbuf to this small mbuf.
			 * 
			 */
			if(fzadebug > 1 ) {
				printf("fzarint: got packet size %d type",len); 
				if(fddi_type == FZA_LLC )
					printf(" LLC frame \n");
				else
					printf(" SMT frame \n");
			}

			if(len > MLEN ) { 
				FZAMCLGET(mp)
			} else { 
				MGET(mp, M_DONTWAIT, MT_DATA)
			}
			m = bp->mbufa ; 
			if ( mp ) {
				if ( len > M_CLUSTERSZ ) { 
					FZAMCLGET(m1)
					if(m1) {
						clean_dcache(PHYS_TO_K0(bp->phymbufa),M_CLUSTERSZ); 
						clean_dcache(PHYS_TO_K0(bp->phymbufb),len - M_CLUSTERSZ);
						m->m_next = bp->mbufb;
						m->m_next->m_len = len - M_CLUSTERSZ;
						fzanlarge++;
					} else {
						m_freem(mp);
						fzannombuf++;
						goto doinit;
					}
				} else if ( len > MLEN ) {
					m1 = bp->mbufb;
					clean_dcache(PHYS_TO_K0(bp->phymbufa),len); 
					m->m_len = len;
					fzanmiddle++;
				} else {
					/* 
					 * if size < = MLEN
					 */
					bcopy((PHYS_TO_K1(bp->phymbufa)),mtod(mp,caddr_t),len);
					m = mp;
					m->m_len = len;
					mp = bp->mbufa; 
					m1 = bp->mbufb;
					fzansmall++;
				}
				if(fddi_type == FZA_LLC )
					fzaread (sc, m, len, (struct mbuf *)0);
				else { /* for SMT frame, just queue it */
					if(IF_QFULL(&sc->is_smt)){
						IF_DROP(&sc->is_smt);
						/*
						 * increase the system buffer
						 * unavailable
						 */
						sc->reg_ctla = SMT_RCV_OVERFLOW;		
						fzansmtdrop++;
						m_freem(m);
					} else {
						fzansmtrcvd++;
						IF_ENQUEUE(&sc->is_smt,m);
						/* Save the RMC descriptor */
						sc->smt_rmc[sc->smtrmcindex]= rp->rmc; 
						sc->smtrmcindex = ++sc->smtrmcindex % IFQ_MAXLEN;
					}
				}
				fzainitdesc(rp,bp,mp,m1);
			} else { 
				fzannombuf++;
				goto doinit;
			}
			
		} else { /* if error happened, paser the RMC descriptor */ 
			if(fzadebug)
				mprintf("fza%d: recv err x%x\n",unit,rp->rmc);
			switch(rp->rmc & FZA_RCV_RCC ) {
			case FZA_RCV_OVERRUN: /* frame too long */
					if(len == 8192 || len == 8191) 
					        fstc_pdulen++;
					else {
					     if (fzadebug)
						printf("xfa%d: RMC FIFO Overflow",unit); 		
					}				
					break;
			case FZA_RCV_INTERFACE_ERR:/* RMC/MAC interface error*/ 
					/*
					 * adapter should take care this
					 * driver will never see this
					 */
					printf("fza%d: RMC/MAC interface error",unit);					
					break;
			default:
				switch(rp->rmc & FZA_RCV_RCC_rrr) {
					case FZA_RCV_RCC_NORMAL:
						if(rp->rmc & FZA_RCV_RCC_C) {
							fstc_fcserror++;
							if(fzadebug)
								printf("fza%d: Block Check Error\n",unit); 
						} else if ( !(rp->rmc & FZA_RCV_FSC) || (rp->rmc & FZA_RCV_FSB_E)){
							fstc_fseerror++;
							if(fzadebug)
								printf("fza%d: Frame status error\n",unit); 
						}
						break;
					case FZA_RCV_RCC_INVALID_LEN:
						fstc_pdualig++;
						if(fzadebug)		 
							printf("fza%d: Frame Alignment Error\n",unit);
						break;
					case FZA_RCV_RCC_SA_MATCHED:
					case FZA_RCV_RCC_DA_NOMATCHED:
					case FZA_RCV_RCC_RMC_ABORT:
						if(fzadebug) 
							printf("fza%d: Hardware problem \n",unit);
						/*
						 * go to halt state, log
						 * the errors
						 */
						sc->reg_ctla = HALT;
						return;
						break;
					case FZA_RCV_RCC_FRAGMENT:
					case FZA_RCV_RCC_FORMAT_ERR:
					case FZA_RCV_MAC_RESET:
						if(fzadebug)
							printf("fza%d: Fragment or format error or MAC reset\n",unit);
						break;
					defualt:
						if(fzadebug)
							printf("fza%d: Wrong RMC descriptor report 0x%x \n",unit,rp->rmc);
						break;
				}
				break;
			}	 
error:
			sc->is_if.if_ierrors++;
doinit:
			fzainitdesc(rp,bp,bp->mbufa,bp->mbufb);						
		}

	}
}

/*
 * FZA read routine. Pass input packets up to higher levels.
 */
fzaread (sc, m, len, swloop)
	register struct fza_softc *sc;
	register int len;
	struct mbuf *m;
	struct mbuf *swloop;
{
	register struct fddi_header *eptr;
	register int off, resid;
	struct mbuf *swloop_tmp1;
	struct ether_header eh;
	struct protosw *pr;
	struct ifqueue *inq;


	/*
	 * Not supporting the trailer protocol
	 */
	if (swloop) {
		eptr = mtod(swloop, struct fddi_header *);
		if ( swloop->m_len > sizeof(struct fddi_header))
			m_adj(swloop, sizeof(struct fddi_header));
		else {
			MFREE(swloop, swloop_tmp1);
			if ( ! swloop_tmp1 )
				return;
			else
				swloop = swloop_tmp1;
		}
	} else 
		eptr = mtod(m, struct fddi_header *);


	/*
	 * Pull packet off interface. 
	 */
	if (swloop) {
		m = m_copy(swloop, 0, M_COPYALL);
		m_freem(swloop);
		if (m == 0)
			return;

	} else {
		/*
		 * Trim off fddi header
		 */
		m->m_off += sizeof (struct fddi_header);
		m->m_len -= sizeof (struct fddi_header);
	}

	/*
	 * Subtract length of header from len
	 */
	len -= sizeof (struct fddi_header);

	/*
	 * feed the ehter struct by the fddi struct
	 */
	/*	bcopy(&eptr->fddi_dhost[0],&eh.ether_dhost[0],6); */
	/*	bcopy(&eptr->fddi_shost[0],&eh.ether_shost[0],6); */
	/*      eh.ether_type = len; */
	/*
	 * Bunp up the DECnet counter
	 */
	sc->is_if.if_ipackets++;
	fstc_bytercvd += len;
	if ( fstc_pdurcvd != (unsigned) 0xffffffff)
		fstc_pdurcvd++;

	if( eptr->fddi_dhost[0] & 1 ) {
		fstc_mbytercvd += len;
		if(fstc_mpdurcvd != 0xffffffff)
			fstc_mpdurcvd++;
	}

	/* Dispatch this packet */
	net_read(&(sc->is_ed), (struct ether_header *)eptr, m, len, (swloop != NULL), 0 );
}

/*
 * Perform a node halt/reset (SOFT reset) due to the HALT state change
 * interrupt.
 */
fzareset(unit)
	int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];

	struct ifnet *ifp = &sc->is_if;
	struct mbuf *mp, *m, *m1;
	struct rmbuf *bp;
	FZAXMTRING *tp;
	FZARCVRING *rp;	
	int i;


	/*
	 * release all of the receive mbufs, transmit mbufs , and then reset 
	 * the adapter
	 */

	for (i = 0, bp = &sc->rmbuf[0];  i < NFZARCV ; i++ ,bp++) {
		if(bp->mbufa)
			m_freem(bp->mbufa);
		if(bp->mbufb)
			m_freem(bp->mbufb);
		bp->mbufa = bp->mbufb = 0; 
		bp->phymbufa = bp->phymbufb = 0;
	}
	for ( i=0, tp = &sc->tring[0]; i < sc->nrmcxmt && tp->xmt_mbuf ; i++, tp++ ) {
		m_freem(tp->xmt_mbuf);
	}


	/*
	 * If the adapter is in SHUT state, don't do the selftest
         */
         if((sc->flag == FZA_NORMAL || sc->flag == FZA_PC_TRACED )) {
		/*
		 * take out this because it may happen FZAMAXRESET
		 * times per year; the best way is use a ratio of 
		 * how many times per hour . 
		 */
		/* 
		if (!(sc->flag & FZA_PC_TRACED) && (++fzanreset > FZAMAXRESET))
		 {
			ifp->if_flags &= ~IFF_UP;
			printf("fza %d: exceed the maximum number of reset\n",unit);
			return(0);
		}
		*/
		/*
	 	 * do the adapter selftest
	 	 */
		if(!(fzaselftest(sc,unit))) { 
			printf("fza%d: fzareset selftest fail \n",unit);
			return(0);				
		}
		/* set the interrup mask */
		sc->reg_mask = FZA_INTR_MASK;
	}

        sc->tindex = sc->nxmit = sc->rindex = 0;
        sc->tsmtindex = sc->rsmtindex = 0;
        sc->cmdindex = sc->unsindex = 0;
        sc->ltindex = sc->lcmdindex = -1;

	sc->smtrmcindex = sc->lsmtrmcindex = 0;	

	if(!fzacmdinit(sc,unit)) {
			printf("fza%d: fzrestart init command fail \n",unit);
			return(0);
	}

	/*
	 * fill the information
	 */
	fzafillinit(sc,sc->initblk);

	/* set the interrup mask */
	sc->reg_mask = FZA_INTR_MASK;

	if(sc->flag == FZA_DLU) {
		printf("fza%d: Firmware revision %c%c%c%c \n",unit,
				sc->fw_rev[0],sc->fw_rev[1],sc->fw_rev[2],
				sc->fw_rev[3]);
		if(sc->nduflag & IFF_UP ) /* turn on this device */
			ifp->if_flags |= IFF_RUNNING ;
		else 			  /* turn off this device */
			ifp->if_flags &= ~IFF_RUNNING ;
	}

	/*
	 * clean the flag
	 */
	sc->flag = FZA_NORMAL;
        /*
         * allocate mbufs for receive ring
         */
        for (i = 0, rp = &sc->rring[0], bp = &sc->rmbuf[0] ; (i < NFZARCV ); 
		i++, rp++ , bp++) {
                FZAMCLGET(m)
                if(m) { 
                        FZAMCLGET(m1)
                	if (m1) 
                        	fzainitdesc(rp,bp, m, m1);
			else
                        	goto free;
                 }  else 
                        goto free;
        }
        /*
         * clear the transmit ring mbuf entry
         */
        for ( i=0, tp = &sc->tring[0]; i < sc->nrmcxmt ; i++, tp++) 
                        tp->xmt_mbuf = (struct mbuf *)0;

	
	if(ifp->if_flags & IFF_RUNNING) {
			ifp->if_flags &= ~(IFF_RUNNING);
			untimeout(fzainit, unit); 
			timeout(fzainit, unit, 1);
	}
	
	return(1);
free:
        for (i = 0, bp = &sc->rmbuf[i]; (i < NFZARCV ); i++, rp++) {
                if(bp->mbufa)
                        m_freem(bp->mbufa);
                if(bp->mbufb)
                        m_freem(bp->mbufb);
        }
	printf("fza%d: fzareset fail - can not allocate enough receive buffer \n",unit);
	return(0);	
}
/*
 * Process an ioctl request.
 */
fzaioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct fza_softc *sc = &fza_softc[ifp->if_unit];
	register struct fzacmd_buf *xcmd;
	struct protosw *pr;
	struct mbuf *m;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct ctrreq *ctr = (struct ctrreq *)data;
	struct ifeeprom *ife = (struct ifeeprom *)data;
	int s, delay, error = 0, timeout = 0;

	switch (cmd) {

        case SIOCENABLBACK:
        case SIOCDISABLBACK:
		if (cmd == SIOCENABLBACK) { 
			if(fzadebug) printf("SIOCENABLBACK");
                	ifp->if_flags |= IFF_LOOPBACK;
		} else {
			if(fzadebug) printf("SIOCDISABLBACK");
                	ifp->if_flags &= ~IFF_LOOPBACK;
		}
		if (ifp->if_flags & IFF_RUNNING) {

			/*
			 * Lock softc. issue a SHUT command 
			 * to cause a state change and then
		         * bring down the adapter and reset it 
			 */
			s = splimp();
			smp_lock(&sc->lk_fza_softc, LK_RETRY);
			sc->reg_ctla = SHUT;
			sc->flag = FZA_SHUT;
			/*
		 	 * Wait 30 seconds for FZA change to Uninitialized state
		 	 */ 	
			for (delay = 3000; delay > 0 && ((sc->reg_status & ADAPTER_STATE) != STATE_UNINITIALIZED) ; delay--)  
				 DELAY(10000);
			if((sc->reg_status & ADAPTER_STATE) == STATE_UNINITIALIZED)
				fzareset(ifp->if_unit);
			else
				printf("fza%d: Can't transition to Uninitialize State \n",ifp->if_unit); 
			smp_unlock(&sc->lk_fza_softc);
			splx(s);
		}
                break;
 
        case SIOCRPHYSADDR: 
                /*
                 * read default hardware address. Lock softc while accessing
		 * per-unit physical address info.
                 */
		s = splimp();
		smp_lock(&sc->lk_fza_softc, LK_RETRY);
		bcopy(sc->is_dpaddr, ifd->default_pa, 6);
		bcopy(sc->is_addr, ifd->current_pa, 6);
		smp_unlock(&sc->lk_fza_softc);
		splx(s);
                break;
 

	case SIOCSPHYSADDR: 
	case SIOCDELMULTI: 
	case SIOCADDMULTI: 

		/*
		 * Lock softc while updating per-unit multicast address
		 * list and for command processing as in "fzainit()".
		 */
		s = splimp();
		smp_lock(&sc->lk_fza_softc, LK_RETRY);
		if (cmd == SIOCDELMULTI) {
			int i;
			if(fzadebug > 1 ) printf("SIOCDELMULTI");
			/*
			 * If we're deleting a multicast address, decrement
			 * the is_muse count and invalidate the address if
			 * count goes to zero.
			 */
			for (i = 0; i < NMULTI; i++) {
				if (bcmp(sc->is_multi[i],
				    ifr->ifr_addr.sa_data,6) == 0)
					break;
			}
			if ((i < NMULTI) && (--sc->is_muse[i] == 0))
				bcopy(etherbroadcastaddr,sc->is_multi[i],6);
			else {
				smp_unlock(&sc->lk_fza_softc);
				splx(s);
				goto done;
			}
		} else {
			int i, j = -1;
			if( cmd == SIOCSPHYSADDR ) {
			/*
			 * we can not change the physical station
			 * address; just add an entry to the CAM as 
			 * an alais address
			 */
				if(fzadebug > 1)
					printf("SIOCSPHYSADDR");
		
               			bcopy(ifr->ifr_addr.sa_data, sc->is_addr, 6);
#if NPACKETFILTER > 0
                		pfilt_newaddress(sc->is_ed.ess_enetunit, ifr->ifr_addr.sa_data);
#endif NPACKETFILTER > 0
			
			}
			else {
				if(fzadebug > 1 ) 
					printf("SIOCADDMULTI");
			
			} 
			/*
			 * If we're adding a multicat address, increment the
			 * is_muse count if it's already in our table, and
			 * return. Otherwise, add it to the table or return
			 * ENOBUFS if we're out of entries.
			 */
			for (i = 0; i < NMULTI; i++) {
				if (bcmp(sc->is_multi[i],
				    ifr->ifr_addr.sa_data,6) == 0) {
					sc->is_muse[i]++;
					smp_unlock(&sc->lk_fza_softc);
					splx(s);
					goto done;
				}
				if ((j < 0) && (bcmp(sc->is_multi[i],
				    etherbroadcastaddr,6) == 0))
					j = i;
			}
			if (j < 0) {
				printf("fza%d: addmulti failed, multicast list full: %d\n",
					ifp->if_unit, NMULTI);
				smp_unlock(&sc->lk_fza_softc);
				error = ENOBUFS;
				splx(s);
				goto done;
			} else {
		    		bcopy(ifr->ifr_addr.sa_data,
				      sc->is_multi[j], 6);
		    		sc->is_muse[j]++;
			}
		}
		if (ifp->if_flags & IFF_RUNNING) {
			/*
			 * If we've successfully init'ed the interface,
			 * issue a MODCAM command to update the fddi
			 * user's multicast address list. Otherwise, the
			 * list will be initialized upon the first call
			 * to "fzainit()".
			 */
			FZACMDRING *cp;
			if (cp=fzamkcmd(sc, CMD_MODCAM,ifp->if_unit)) {
				smp_unlock(&sc->lk_fza_softc);
				splx(s);
				FZATIMEOUT(cp,timeout)/* Wait */
				if(timeout ||  (cp->status_id != CMD_SUCCESS)) 
						error = EINVAL;
			} else {
				smp_unlock(&sc->lk_fza_softc);
				error = ENOBUFS;
				splx(s);
			}
		}
		else {
			smp_unlock(&sc->lk_fza_softc);
			splx(s);
		}
		break;

	case SIOCRDZCTRS:
	case SIOCRDCTRS:

		if(fzadebug) {
			if (cmd == SIOCRDZCTRS) 
					printf("SIOCRDZCTRS");
			else 
					printf("SIOCRDCTRS");
		}
			
		/*
		 * The adapter does not support clear counter command.
		 * So, we only support read counter. 
		 *
		 * Copyin most recent contents of unit's counter
		 * block.
		 */
		if ((ifp->if_flags & IFF_RUNNING) ||
		   (sc->reg_status & ADAPTER_STATE) == STATE_RUNNING || 
		   (sc->reg_status & ADAPTER_STATE) == STATE_MAINTENANCE || 
		   (sc->reg_status & ADAPTER_STATE) == STATE_INITIALIZED ) 
		{
			switch (ctr->ctr_type) {
				case FDDIMIB_SMT:
				case FDDIMIB_MAC:
				case FDDIMIB_PATH:
				case FDDIMIB_PORT:
				case FDDIMIB_ATTA:
					fmib_fill(sc,ctr,ctr->ctr_type);
					break;	
				case FDDI_STATUS:
					fzagetstatus(sc, &ctr->sts_fddi,sc->statusblk);
					break;
				default:
					fzagetctrs(sc, &ctr->ctr_fddi,sc->ctrblk);
					ctr->ctr_type = CTR_FDDI;
					break;
			}
		}
		break;

	case SIOCSIFADDR:

		/*
		 * Init the interface if its not already running
		 */
		if(fzadebug) printf("SIOCSIFADDR");
		fzainit(ifp->if_unit);
		switch(ifa->ifa_addr.sa_family) {
#ifdef INET
		case AF_INET:
			s = splimp();
			smp_lock(&lk_ifnet, LK_RETRY);
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			smp_unlock(&lk_ifnet);
			splx(s);
			/* 1st packet out */
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif INET

		default:
			if (pr=iffamily_to_proto(ifa->ifa_addr.sa_family))
				error = (*pr->pr_ifioctl)(ifp, cmd, data);
			break;
		}
		break;

#ifdef	IFF_PROMISC	/* IFF_ALLMULTI and NPACKETFILTER, as well */
	case SIOCSIFFLAGS:

		if (ifp->if_flags & IFF_RUNNING) {
			/* 
			 * If we've successfully init'ed the interface, 
			 * issue a MODPROM command to update the ethernet 
			 * user's promiscuous bit based upon the interface
			 * flags.
			 *
			 * Only support LLC promiscuous mode for now 
			 */
			FZACMDRING *cp;
			s = splimp();
			smp_lock(&sc->lk_fza_softc, LK_RETRY);
			if (cp =fzamkcmd(sc, CMD_MODPROM,ifp->if_unit)) {
				smp_unlock(&sc->lk_fza_softc);
				splx(s);
				FZATIMEOUT(cp,timeout)
				if(timeout ||  (cp->status_id != CMD_SUCCESS)) 
						error = EINVAL;
			} else {
				smp_unlock(&sc->lk_fza_softc);
				error = ENOBUFS;
				splx(s);
			}
		}
		break;
#endif	IFF_PROMISC

	case SIOCIFRESET: /* reset the adapter */
			
		if(fzadebug) printf ("SIOCIFRESET \n");
		s = splimp();
		smp_lock(&sc->lk_fza_softc, LK_RETRY);
		if(!fzaselftest(sc,ifp->if_unit)) {
			printf(" SIOCIFRESET selftest fail \n");
			smp_unlock(&sc->lk_fza_softc);
			splx(s);
			return(sc->reg_status & ID_FIELD);
		} else
			/*
			 * self test successed the adapter is in
			 * Uninitialized state, do the driver 
			 * reset.
			 */
			sc->flag = FZA_DLU;
			fzareset(ifp->if_unit);
			/*
			 * turn on the timer 
			 */
			ifp->if_timer = 1;
			smp_unlock(&sc->lk_fza_softc);
			splx(s);
			return(0);
		break;
	case SIOCEEUPDATE: /* EEPROM update */
	
		if(fzadebug) printf("SIOCEEUPDATE \n");
		s = splimp();
		smp_lock(&sc->lk_fza_softc, LK_RETRY);

		/*
	 	 * if the adapter is not in the uninitialized mode, force it
	 	 * to that mode.
	 	 */
		if(((sc->reg_status & ADAPTER_STATE) != STATE_UNINITIALIZED) && 
			((sc->reg_status & ADAPTER_STATE) !=  STATE_RESET )) {

			/*turn off the read counters routine */
			ifp->if_timer = 0; 

			/* save the original state */
			sc->nduflag = ifp->if_flags ;

			/* disable the interface */
			ifp->if_flags &= ~IFF_UP;

			sc->reg_intr = sc->reg_intr ;	
			sc->reg_ctla = SHUT;
			/*
		 	 * Wait 30 seconds for FZA state change
		 	 */ 	
			for (delay = 3000; delay > 0 && !(sc->reg_intr & STATE_CHG); delay-- )
				 DELAY(10000);
			/*
		 	 * check for state change
			 */
			if(!(sc->reg_intr & STATE_CHG)) {
				printf("fza%d: Down Line Upgrade state change time out\n",ifp->if_unit);
				smp_unlock(&sc->lk_fza_softc);
				splx(s);
				return(1);
			} else if((sc->reg_status & ADAPTER_STATE) !=  
					STATE_UNINITIALIZED) { 
				printf("fza%d: Down Line Upgrade can not change to Uninitialized Mode",ifp->if_unit);
				smp_unlock(&sc->lk_fza_softc);
				splx(s);
				return(1);
			}
		}
		/*
	 	 * set the DLU and reset bit
	 	 */
		sc->reg_reset = DLU_MODE | RESET ;			
	
		if(!fzadlu(ifp->if_unit,sc,ife->ife_data,ife->ife_offset,ife->ife_blklen)) { 
			smp_unlock(&sc->lk_fza_softc);
			splx(s);
			return(1); /* dlu failure */
		}
		/*
		 * if this is last block, blast flush
		 */
		if(ife->ife_lastblk == IFE_LASTBLOCK ) {
			int i;
			sc->reg_reset &= ~RESET ;
			/*
			 * wait for 90 seconds for blast FLUSH finish
			 */
			for (i = 9000; i > 0 && !(sc->reg_intr & DLU_DONE); i--)
				DELAY(10000);
			/*
			 * check the DLU_DONE bit
			 */
			if(!(sc->reg_intr & DLU_DONE)) {
				printf("fza%d: Code Update not complete - image may be corrupted",ifp->if_unit);
				smp_unlock(&sc->lk_fza_softc);
				splx(s);
				return(1);
			} else {
				switch ( sc->reg_status & DLU_STATUS ) {
					case DLU_FAILURE:
							printf("fza%d: DLU fatal failure - Brain Dead \n",ifp->if_unit);
							smp_unlock(&sc->lk_fza_softc);
							splx(s);
							return(1);
							break;	
					case DLU_ERROR:
							printf("fza%d: DLU error - Recoverable \n",ifp->if_unit);
							smp_unlock(&sc->lk_fza_softc);
							splx(s);
							return(1);
							break;
					case DLU_SUCCESS: 
							sc->reg_reset = FZAREG_CLEAR ;	
							break;
				}
			}
		} 
		/* turn off the interface */
		smp_unlock(&sc->lk_fza_softc);
		splx(s);
		return (0);
		break;
		
	default:
		error = EINVAL;
	}
done:
	return(error);
}

fzadlu(unit,sc,data,offset,blklen)
int unit,offset,blklen;
register struct fza_softc *sc;
char *data;
{
	char *pmiaddr,*bufaddr = data;
	u_short stmp,stmp1;
	int i;
	
	pmiaddr = (char *)(sc->basereg + FZA_DLU_ADDR + offset) ;
	if(dludebug)
		printf("\n dlu address 0x%x size %d data --->",pmiaddr,blklen);
	for ( i = blklen ; i >= 2 ;
		 i = i - 2, bufaddr = bufaddr + 2 ) {
		*((u_short *)pmiaddr) = stmp = htons(*((u_short *)bufaddr));
		stmp1 = *(u_short *)pmiaddr;		
		pmiaddr = pmiaddr + 2;
		if(dludebug)
			printf("%d New %x OLD %x ",i, stmp1, *((u_short *)bufaddr));
		/*
		 * check the status
		 */  
		if(sc->reg_intr & PM_PARITY_ERR) {
			printf("fza%d, Down Line Load parity error \n",unit);
			return(0);
		}
	}
	if(i > 0 ) { 
		*(u_short *)pmiaddr  = htons(*((u_short *)bufaddr)) & 0xff00 ;
		stmp1 = *(u_short *)pmiaddr;
		/*
		 * check the status
		 */  
		if(sc->reg_intr & PM_PARITY_ERR) {
			printf("fza%d, Down Line Load parity error \n",unit);
			return(0);
		}
	}
		
	if(dludebug)
		printf("\n");
return(1);
}

/*
 * FZA watchdog timer (runs once per second). Schedule a "read counters"
 * command and "read status" to update the per-unit counter block. 
 */
fzawatch (unit)
	int unit;
{
	register struct fza_softc *sc = &fza_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	static int status;
	int s;

	if ((sc->reg_status & ADAPTER_STATE) != STATE_HALTED ) {

		s = splimp();
		smp_lock(&sc->lk_fza_softc, LK_RETRY);
		/*
		 * Schedule a read counters cmd to update the counter
		 * block for this unit. 
		 */
		fzamkcmd(sc,CMD_RDCNTR,unit);

		if(status++ > 3 ) {
			/*
			 * schedule a read status command	
			 */ 
			fzamkcmd(sc,CMD_STATUS,unit);
			status = 0;
		}
		smp_unlock(&sc->lk_fza_softc);
		splx(s);

	}
	ifp->if_timer = 1;
}

/*
 * FZA INIT command and wait until done
 */
fzacmdinit(sc,unit)
register struct fza_softc *sc;
int unit;
{
	register FZACMDRING *cp;
	int delay,index = sc->cmdindex;
	
	if((cp=fzamkcmd(sc,CMD_INIT,unit))) {
		/*
		 * wait 2 seconds for the INIT command done - only need one
	 	 * second
	 	 */	
		for (delay = 200 ; delay > 0 && !(sc->reg_intr & CMD_DONE); delay--)
				DELAY(10000);

		sc->lcmdindex++;
		if(!(sc->reg_intr & CMD_DONE)) {
			printf("fza%d: INIT command timeout \n",unit);
			if(fzadebug) 
				PRT_REG(sc)
			
			return(0);
		} else if(!(fzacmdstatus(sc,cp->status_id,unit,"fzacmdinit"))) {
			if(fzadebug)
				PRT_REG(sc)
			
			return(0);
		} else {
			if(fzadebug)
				printf("fza%d: INIT command successed \n",unit);
			bcopy((caddr_t)(cp->buf_addr+sc->basereg),sc->initblk, sizeof(struct fzainit));
			return(sizeof(struct fza_softc));
		}
	} else 
		return(0);
}
	
fzainitdesc(rp,bp,m,m1)
register FZARCVRING *rp;
register struct rmbuf *bp; 
struct mbuf *m, *m1;
{
	rp->rmc = 0;
	bp->mbufb=m1;
	bp->phymbufb = svtophy(mtod(m1,u_long *));
	rp->bufaddr2 = bp->phymbufb >> FZASHIFT ; 	
	bp->mbufa=m;
	bp->phymbufa = svtophy(mtod(m,u_long *));
	rp->bufaddr1 = ( bp->phymbufa >> FZASHIFT) & ~FZA_HOST_OWN ;
}

fzaselftest(sc,unit)
register struct fza_softc *sc;
int unit;
{
	register int delay;

	/* 
	 * perform the self test
	 */
        sc->reg_reset = RESET ;
        DELAY(1000);
        sc->reg_reset = FZAREG_CLEAR;

	/*
	 * wait 40 seconds (adapter need 25 seconds) for FZA state
	 * change from Reseting state to Uninitialized state
	 */
	for (delay = 4000 ; delay > 0 && !(sc->reg_intr & STATE_CHG); delay--)	
			DELAY(10000);

	/*
         * check for the state change
	 */
	if (!( sc->reg_intr & STATE_CHG) ) {
		/*
                 * didn't change state 
	         */
		printf("fza%d: selftest timeout, couldn't pass selftest id %d\n",
					unit,(sc->reg_status & ID_FIELD));
		if(fzadebug) 
			PRT_REG(sc)
		
		return(0);
	} else if ( sc->reg_status != STATE_UNINITIALIZED ) {
		printf("fza%d: selftest error, couldn't change to Uninitialied state id %d \n",unit,(sc->reg_status & ID_FIELD));
		if(fzadebug)  
			PRT_REG(sc)
		return(0);
	} else {
		if(fzadebug) 
			printf("fza%d: selftest successed\n");
	}
return(1);
}


fzafillinit(sc,pt)
register struct fza_softc *sc;
struct fzainit *pt;
{

	bcopy(&pt->mla[0],&sc->is_addr[0],6);
	bcopy(&pt->mla[0],&sc->is_dpaddr[0],6);

	/* 
	 * fill out the default characteristics value
	 */
	sc->t_max = pt->def_t_max;
	sc->t_req = pt->def_t_req;
	sc->tvx = pt->def_tvx;
	sc->lem_threshold= pt->lem_threshold;
	sc->pmd_type = pt->pmd_type;
	sc->smt_version = pt->smt_version;	
	/*
	 * fill out the ring address and number of ring entrys
	 */
	sc->rring=(FZARCVRING *)(pt->rcvbase_addr + sc->basereg);
	sc->tring=(FZAXMTRING *)(pt->xmtbase_addr + sc->basereg);
	sc->smttring=(FZASMTRING *)(pt->smtxmt_addr + sc->basereg);
	sc->smtrring=(FZASMTRING *)(pt->smtrcv_addr + sc->basereg);	
	
	sc->nrmcxmt = pt->xmt_entry;
	sc->nsmtxmt = pt->smtxmt_entry;
	sc->nsmtrcv = pt->smtrcv_entry;

	/*
	 * fill out the adapter specific information
	 */
	bcopy(&pt->pmc_rev[0],&sc->pmc_rev[0],4);
	bcopy(&pt->phy_rev[0],&sc->phy_rev[0],4);
	bcopy(&pt->fw_rev[0],&sc->fw_rev[0],4);
	sc->mop_type = 	pt->mop_type;
	sc->station_id.lo = pt->def_station_id.lo;
	sc->station_id.hi = pt->def_station_id.hi;

}


fzagetstatus(sc,ctr,fs)
	register struct fza_softc *sc;
	register struct fstatus *ctr;
	register struct fzastatus *fs;
{
	/*
	 * Fill out the fddi status 
	 */
	bzero(ctr, sizeof(struct fstatus));
	
	/* 
	 * assign the default characterists value 
	 * and revision number
	 */
	ctr->t_req = sc->t_req;
	ctr->t_max = sc->t_max;
	ctr->tvx   = sc->tvx;
	ctr->lem_threshold = sc->lem_threshold;
	ctr->pmd_type = sc->pmd_type ;
	ctr->smt_version = sc->smt_version ;
	bcopy(&sc->phy_rev[0],&ctr->phy_rev[0],4);
	bcopy(&sc->fw_rev[0],&ctr->fw_rev[0],4);
	
	/*
	 * When the adapter in HALT state, we can
	 * not issue the get status command
	 */
	if((sc->reg_status & ADAPTER_STATE) == STATE_HALTED ) {
		ctr->led_state = 2 ; /* Red */
		ctr->link_state = 1 ; /* Off Ready */ 
		ctr->dup_add_test =  0 ; /* Unknown */
		ctr->ring_purge_state = 0 ; /* Purger Off */
		ctr->phy_state = 2 ; /* Off Ready */
	} else { 
		ctr->led_state = fs->led_state;
		ctr->link_state = fs->link_state;
		ctr->phy_state = fs->phy_state ;
		ctr->dup_add_test = fs->dup_add_test;
		ctr->ring_purge_state = fs->ring_purge_state;
	}
	ctr->state = (sc->reg_status & ADAPTER_STATE) >> 8 ;
	ctr->rmt_state = fs->rmt_state;		
	ctr->neg_trt = fs->neg_trt;
	bcopy(&fs->upstream[0],&ctr->upstream[0],6);
	ctr->una_timed_out = fs->una_timed_out;
	ctr->frame_strip_mode = fs->frame_strip_mode;
	ctr->claim_token_mode = fs->claim_token_mode;
	ctr->neighbor_phy_type = fs->neighbor_phy_type;
	ctr->rej_reason = fs->rej_reason;
	ctr->phy_link_error = fs->phy_link_error;
	bcopy(sc->is_dpaddr,&ctr->mla[0],6);
	ctr->ring_error = fs->ring_error & 0x0f ;
	bcopy(&fs->dir_beacon[0],&ctr->dir_beacon[0],6);
}	
	
fzagetctrs(sc,ctr,xcmd)
	register struct fza_softc *sc;
	register struct fstat *ctr;
	register struct _fzactrs *xcmd;
{
	
	register int seconds;

	/*
	 * Fill out the fddi counters through the ethernet counter
	 * "estat" structure. It is  based upon the information
	 * returned by the CMD_{RDC,RCC}CNTR command and driver  
	 * maintained counter. 
	 */
	bzero(ctr, sizeof(struct fstat));

	seconds = fstc_second = time.tv_sec - sc->ztime;
	if (seconds & 0xffff0000)
	    ctr->fst_second = 0xffff;
	else
	    ctr->fst_second = seconds & 0xffff;

	/* driver counter */

	ctr->fst_bytercvd   = fstc_bytercvd;
	ctr->fst_bytesent   = fstc_bytesent;
	ctr->fst_pdurcvd    = fstc_pdurcvd;
	ctr->fst_pdusent    = fstc_pdusent;
	ctr->fst_mbytercvd  = fstc_mbytercvd;
	ctr->fst_mpdurcvd   = fstc_mpdurcvd;
	ctr->fst_mbytesent  = fstc_mbytesent;
	ctr->fst_mpdusent   = fstc_mpdusent;
	ctr->fst_pduunrecog  = fstc_pduunrecog = sc->is_ctrblk.est_unrecog;
	ctr->fst_mpduunrecog = fstc_mpduunrecog;
	ctr->fst_fcserror    = fstc_fcserror ;
	ctr->fst_fseerror    = fstc_fseerror ;
	ctr->fst_pdulen      = fstc_pdulen;
	ctr->fst_pdualig     = fstc_pdualig;

	/* adapter counter */ 
	if (xcmd->frame_count.hi) 
		ctr->fst_frame = fstc_frame  = 0xffffffff;
	else
		ctr->fst_frame = fstc_frame = xcmd->frame_count.lo;

	if (xcmd->error_count.hi)
		ctr->fst_error = fstc_error = 0xffffffff;
	else
		ctr->fst_error = fstc_error = xcmd->error_count.lo;

	if (xcmd->lost_count.hi)
		ctr->fst_lost = fstc_lost = 0xffffffff;
	else
		ctr->fst_lost = fstc_lost = xcmd->lost_count.lo;

	if (xcmd->xmt_fail.hi || 
			(xcmd->xmt_fail.lo & 0xffff0000))
	    ctr->fst_sendfail = fstc_sendfail = 0xffff;
	else
	    ctr->fst_sendfail = fstc_sendfail = *(u_short*)(&xcmd->xmt_fail.lo);

	if (xcmd->xmt_underrun.hi ||
			(xcmd->xmt_underrun.lo & 0xffff0000))
		ctr->fst_underrun = fstc_underrun = 0xffff;
	else
		ctr->fst_underrun = fstc_underrun = *(u_short*)(&xcmd->xmt_underrun.lo);

	if ( xcmd->rcv_overrun.hi || 
			(xcmd->rcv_overrun.lo & 0xffff0000))  
	   	ctr->fst_overrun = fstc_overrun = 0xffff;

	else
	    	ctr->fst_overrun = fstc_overrun = *(u_short*)(&xcmd->rcv_overrun.lo);

	if (xcmd->sysbuf.hi ||
			(xcmd->sysbuf.lo & 0xffff0000))
	    	ctr->fst_sysbuf = fstc_sysbuf = 0xffff;
	else
	    	ctr->fst_sysbuf = fstc_sysbuf = *(u_short*)(&xcmd->sysbuf.lo);

	if (xcmd->ring_init_init.hi ||
			(xcmd->ring_init_init.lo & 0xffff0000))
		ctr->fst_ringinit = fstc_ringinit = 0xffff;
	else
		ctr->fst_ringinit = fstc_ringinit = *(u_short*)(&xcmd->ring_init_init.lo);

	if (xcmd->ring_init_rcv.hi ||
			(xcmd->ring_init_rcv.lo & 0xffff0000))
		ctr->fst_ringinitrcv = fstc_ringinitrcv = 0xffff;
	else
		ctr->fst_ringinitrcv = fstc_ringinitrcv = *(u_short*)(&xcmd->ring_init_rcv.lo);

	if (xcmd->ring_beacon_init.hi ||
			(xcmd->ring_beacon_init.lo & 0xffff0000))
		ctr->fst_ringbeacon = fstc_ringbeacon = 0xfffff;
	else
		ctr->fst_ringbeacon = fstc_ringbeacon = *(u_short*)(&xcmd->ring_beacon_init.lo);

	if (xcmd->dup_addr_fail.hi ||
			(xcmd->dup_addr_fail.lo & 0xffff0000))
		ctr->fst_dupaddfail = fstc_dupaddfail = 0xffff;
	else
		ctr->fst_dupaddfail = fstc_dupaddfail = *(u_short*)(&xcmd->dup_addr_fail.lo);

	if (xcmd->ring_purge_err.hi ||
			(xcmd->ring_purge_err.lo & 0xffff0000))
		ctr->fst_ringpurge = fstc_ringpurge = 0xffff;
	else
		ctr->fst_ringpurge = fstc_ringpurge = *(u_short*)(&xcmd->ring_purge_err.lo);

	if (xcmd->dup_token.hi || 
			(xcmd->dup_token.lo & 0xffff0000))	
		ctr->fst_duptoken = fstc_duptoken = 0xffff;
	else
		ctr->fst_duptoken = fstc_duptoken = *(u_short*)(&xcmd->dup_token.lo);

	if (xcmd->bridge_strip_err.hi ||
			(xcmd->bridge_strip_err.lo & 0xffff0000))
		ctr->fst_bridgestrip= fstc_bridgestrip= 0xffff;
	else
		ctr->fst_bridgestrip= fstc_bridgestrip= *(u_short*)(&xcmd->bridge_strip_err.lo);

	if (xcmd->trace_init.hi || 
			(xcmd->trace_init.lo & 0xffff0000))
		ctr->fst_traceinit= fstc_traceinit= 0xffff;
	else
		ctr->fst_traceinit= fstc_traceinit= *(u_short*)(&xcmd->trace_init.lo);
	
       	if (xcmd->trace_rcvd.hi ||
                        (xcmd->trace_rcvd.lo & 0xffff0000))
                ctr->fst_tracerecv= fstc_tracerecv= 0xffff;
        else
                ctr->fst_tracerecv= fstc_tracerecv= *(u_short*)(&xcmd->trace_rcvd.lo);

        if (xcmd->lem_rej.hi ||
                        (xcmd->lem_rej.lo & 0xffff0000))
                ctr->fst_lem_rej= fstc_lem_rej= 0xffff;
        else
                ctr->fst_lem_rej= fstc_lem_rej= *(u_short*)(&xcmd->lem_rej.lo);
	
	if (xcmd->lct_rej.hi ||
                        (xcmd->lct_rej.lo & 0xffff0000))
                ctr->fst_lct_rej= fstc_lct_rej= 0xffff;
        else
                ctr->fst_lct_rej = fstc_lct_rej= *(u_short*)(&xcmd->lct_rej.lo);

	if (xcmd->tne_exp_rej.hi ||
                        (xcmd->tne_exp_rej.lo & 0xffff0000))
                ctr->fst_tne_exp_rej= fstc_tne_exp_rej= 0xffff;
        else
                ctr->fst_tne_exp_rej = fstc_tne_exp_rej= *(u_short*)(&xcmd->tne_exp_rej.lo);

        if (xcmd->connection.hi ||
                        (xcmd->connection.lo & 0xffff0000))
                ctr->fst_connection= fstc_connection= 0xffff;
        else
                ctr->fst_connection= fstc_connection= *(u_short*)(&xcmd->connection.lo) ;
}


FZACMDRING *fzamkcmd(sc,cmdid,unit)
	register struct fza_softc *sc;
	int cmdid,unit;
{
	register FZACMDRING *cp;
	register union fzacmd_buf *cmdbp,*cmdinit;

	struct ifnet *ifp = &sc->is_if;

	cp = &sc->cmdring[sc->cmdindex];
	if(!(cp->own)) {
		if(fzadebug)
			printf("fza%d: command ring is not own by host \n",unit);
		return((FZACMDRING *)0); /* no ring owned by host */
	}

	cmdbp = (union fzacmd_buf *)(cp->buf_addr + sc->basereg);
	
	if(fzadebug > 1) {
		printf("fzamkcmd: cmd addr 0x%x bassaddr 0x%x \n",cp->buf_addr, sc->basereg);	
		printf("cmdid: %d , index %d \n",cmdid,sc->cmdindex );
	}
	switch(cmdid) {
		case CMD_INIT:  /* INIT command */
				cmdbp->fzainit.xtm_mode = FZA_XMT_MODE;
				cmdbp->fzainit.rcv_entry = NFZARCV;
				
				/* 
				 * copy the initial counter value
				 */
				if(fzadebug > 1)
					printf("init cmd: cp from 0x%x to 0x%x len %d \n", sc->ctrblk,cmdbp->fzainit.fzactrs,sizeof(struct _fzactrs));
						
				fzacpy(sc->ctrblk,&cmdbp->fzainit.fzactrs,sizeof(struct _fzactrs));
				break;
		case CMD_PARAM:  /* PARAM command */
				if(ifp->if_flags & IFF_LOOPBACK)
					cmdbp->fzaparam.loop_mode = LOOP_INTER;
				else
					cmdbp->fzaparam.loop_mode = LOOP_NORMAL;
				cmdbp->fzaparam.t_max = sc->t_max;
				cmdbp->fzaparam.t_req = sc->t_req;
				cmdbp->fzaparam.tvx = sc->tvx;
				cmdbp->fzaparam.lem_threshold = sc->lem_threshold;
				cmdbp->fzaparam.station_id.lo = sc->station_id.lo;
				cmdbp->fzaparam.station_id.hi = sc->station_id.hi;
				break;
		case CMD_MODCAM:  /* MODCAM command */
				fzacpy(&sc->is_multi[0][0],cmdbp,512);
				break;
		
		case CMD_MODPROM:
				if(ifp->if_flags & IFF_PROMISC)
					cmdbp->fzamodprom.llc_prom = 1;
				else
					cmdbp->fzamodprom.llc_prom = 0;
				cmdbp->fzamodprom.smt_prom = 0;
				break;
		case CMD_SETCHAR: /* SET CHAR command */
		case CMD_RDCNTR:
		case CMD_STATUS:
		case CMD_RDCAM:
		case CMD_NOP:
				break;
		default:
				if(fzadebug) 
					printf("fza%d: non supported command id %d \n",unit,cmdid);	
				break;
	}
	
	cp->cmdid = cmdid;
	cp->own = 0; /* own by rmc */
	sc->reg_ctla = CMD_POLL ;
	wbflush();
	
	sc->cmdindex = ++sc->cmdindex % NFZACMD;
	return(cp);
}

fzacmdstatus(sc,statusid,unit,s)
register struct fza_softc *sc;
int statusid,unit;
char *s;
{
	if(fzadebug)
		printf("%s routine",s);
	switch(statusid) {
		case CMD_SUCCESS:
			return(1);
			break;
		case CMD_STATE_INVALID:
			printf("fza%d: command fail, invaild adapter state \n",unit);
			break;
		case CMD_XTM_INVALID:
			printf("fza%d: invalid transmit mode \n",unit);
			break;
		case CMD_RCVENT_INVALID:
			printf("fza%d: host receive entries invalid \n");
			break;
		case CMD_TMAX_INVALID:
                        printf("fza%d: invaild T_MAX value %d \n",unit,sc->t_max);
			break;
		case CMD_TREQ_INVALID:
			printf("fza%d: invaild T_REQ value %d \n", unit, sc->t_req);
                        break;
                case CMD_TVX_INVALID:
                        printf("fza%d: invaild TVX value %d \n", unit, sc->tvx);
                        break;
                case CMD_LEM_INVALID:
                        printf("fza%d: invaild LEM value %d \n", unit, sc->lem_threshold);
                        break;
                case CMD_STATION_ID_INVALID:
                        printf("fza%d: invaild station ID value \n", unit);
                        break;
                case CMD_CMD_INVALID:
                        printf("fza%d: invaild command \n", unit);
                        break;
		default:
			printf("fza%d: unknown command status %d\n",unit,statusid);
			break;
	}
	return(0);
}	

/*
 * routine fill up the snmp FDDI MIB attributes  
 */
fmib_fill(sc,fmib,type)
register struct fza_softc *sc;
struct ctrreq *fmib;
int type;
{

	switch (type) {
		case FDDIMIB_SMT:	/* SMT group */
			/* adapter will provide this number */
			bcopy(sc->is_dpaddr,&fmib->fmib_smt.smt_stationid[0],6);
			fmib->fmib_smt.smt_opversionid = 1 ;
			fmib->fmib_smt.smt_hiversionid = 1 ;
			fmib->fmib_smt.smt_loversionid = 1 ;
			fmib->fmib_smt.smt_macct = 1;  
			fmib->fmib_smt.smt_nonmasterct = 1;
			fmib->fmib_smt.smt_masterct = 1;
			fmib->fmib_smt.smt_pathsavail = 1;
			fmib->fmib_smt.smt_configcap = 0;
			fmib->fmib_smt.smt_configpolicy = 0;
			fmib->fmib_smt.smt_connectpolicy = 0x8029;
			fmib->fmib_smt.smt_timenotify = 30;
			fmib->fmib_smt.smt_statusreport = 2 ; 
			fmib->fmib_smt.smt_ecmstate = 2 ;
			fmib->fmib_smt.smt_cfstate = 2;
			fmib->fmib_smt.smt_holdstate = 1;
			fmib->fmib_smt.smt_remotedisconn = 2 ;
			/* adapter need to provide this */
			/* fmib->fmib_smt.smt_msgtimestamp[8]  */
			break;

		case FDDIMIB_MAC:	/* MAC group */
			fmib->fmib_mac.mac_index = 1;	
			fmib->fmib_mac.mac_fsc = 1;	
			fmib->fmib_mac.mac_gltmax = 2097 ; /* 167.77224*1000/80 */
			fmib->fmib_mac.mac_gltvx = 31 ;	/* 2.5*1000/80 */
			fmib->fmib_mac.mac_paths = 1;	
			fmib->fmib_mac.mac_current = 2;	
			bcopy(&sc->statusblk->upstream[0],&fmib->fmib_mac.mac_upstream[0],6);	
			bcopy(&sc->statusblk->old_una_address[0],
				&fmib->fmib_mac.mac_oldupstream[0],6);
			fmib->fmib_mac.mac_dupaddrtest = 1;	
			fmib->fmib_mac.mac_pathsreq = 1;	
			fmib->fmib_mac.mac_downstreamtype = 2;	
			bcopy(sc->is_dpaddr,&fmib->fmib_mac.mac_smtaddress[0],6);
			fmib->fmib_mac.mac_treq = sc->t_req;	
			fmib->fmib_mac.mac_tneg = sc->statusblk->neg_trt;
			fmib->fmib_mac.mac_tmax = sc->t_max;	
			fmib->fmib_mac.mac_tvx = sc->tvx;	
			fmib->fmib_mac.mac_tmin = 500 ;	/* 40000/80 */
			fmib->fmib_mac.mac_framestatus = 1;	
			fmib->fmib_mac.mac_counter = sc->ctrblk->frame_count.lo;	
			fmib->fmib_mac.mac_error = sc->ctrblk->error_count.lo;
			fmib->fmib_mac.mac_lost = sc->ctrblk->lost_count.lo;
			fmib->fmib_mac.mac_rmtstate = sc->statusblk->rmt_state;
			if(sc->statusblk->dup_add_test == 2)
				fmib->fmib_mac.mac_dupaddr = 1;	
			else 
				fmib->fmib_mac.mac_dupaddr = 2;	
			fmib->fmib_mac.mac_condition = 2;	
			break;

		case  FDDIMIB_PORT:	/* PORT group */
			fmib->fmib_port.port_index = 1;
			fmib->fmib_port.port_pctype = 3;
			fmib->fmib_port.port_pcneighbor = 
				sc->statusblk->neighbor_phy_type + 1;
			fmib->fmib_port.port_connpolicy = 4;
			/* adapter nee to add this */
			fmib->fmib_port.port_remoteind = 
					sc->statusblk->remote_mac_ind;
			if(sc->statusblk->phy_state == 7)
				fmib->fmib_port.port_CEstate = 2;
			else 
				fmib->fmib_port.port_CEstate = 1;
			fmib->fmib_port.port_pathreq = 1;
			fmib->fmib_port.port_placement = 1;
			fmib->fmib_port.port_availpaths = 1;
			fmib->fmib_port.port_looptime = 200;
			/* need to work */
			fmib->fmib_port.port_TBmax = 625;  /* 50*1000/80 */
			fmib->fmib_port.port_BSflag = 2;
			fmib->fmib_port.port_LCTfail = sc->ctrblk->lct_rej.lo;
			fmib->fmib_port.port_LerrEst = sc->statusblk->phy_link_error;
			fmib->fmib_port.port_Lemreject = sc->ctrblk->lem_rej.lo;
			fmib->fmib_port.port_Lem = sc->ctrblk->lem_event.lo;
			fmib->fmib_port.port_baseLerEst = 0 ;
			fmib->fmib_port.port_baseLerrej = 0;
			fmib->fmib_port.port_baseLerrej = 0;
			fmib->fmib_port.port_baseLer = 0;
			/* need work */
			/* fmib->fmib_port.port_baseLerTime = 0; */
			fmib->fmib_port.port_Lercutoff = sc->lem_threshold;
			fmib->fmib_port.port_alarm = sc->lem_threshold;
			switch(sc->statusblk->phy_state) {
				case 2 :
					fmib->fmib_port.port_connectstate = 1;
					fmib->fmib_port.port_PCMstate = 1;
					break;
				case 5 :
				case 6 :
					fmib->fmib_port.port_connectstate = 3;
					fmib->fmib_port.port_PCMstate = 5;
					break;
				case 7 :
					fmib->fmib_port.port_connectstate = 4;
					fmib->fmib_port.port_PCMstate = 9;
					break;
				default :
					fmib->fmib_port.port_connectstate = 2;
					fmib->fmib_port.port_PCMstate = 5;
					break;
			}
			fmib->fmib_port.port_PCwithhold = 0 ;
			fmib->fmib_port.port_Lercondition = 2 ;
			break;
		case FDDIMIB_ATTA:
			fmib->fmib_atta.atta_index = 1;
			fmib->fmib_atta.atta_class = 1;
			fmib->fmib_atta.atta_bypass = 2;
			break;
		}
			
}
			
				
		
				
/*
 * DEFZA can only support words read and short words/words write. 
 * In addition, the short words write must be aligned correctly within the 
 * words. 
 * This specialized copy routine will check the dst address and make it
 * words aligned. Then, it will use the "bcopy" routine to copy the
 * data untile the word aligned tail.
 */

fzacpy(from,to,len)
caddr_t from; 
caddr_t to;
int len;
{
	register int tlen = len; 
	register caddr_t srcp = from;
	register caddr_t dstp = to;
	register u_short tmp;
	int nbyte,shift;

	/* align the destination address */

	/* check for short word aligned */
	if((nbyte = ((u_long)dstp % 4))) {
		switch(nbyte) {
			case 1:
			case 3:
				tmp = *(u_short *)(dstp -1) & 0xff;
				*(u_short *)(dstp - 1) = tmp | (u_short)(*srcp++ & 0xff) << 8;
				tlen--;
				dstp++;
				if(nbyte == 3 || tlen < 2 )
					break;
			case 2:
				tmp = (u_short)(*srcp++ & 0xff);
				*(u_short *)(dstp) = tmp | (u_short)(*srcp++ & 0xff) << 8;
				dstp += 2;
				tlen -= 2;
				break;
			}
	} 
			
        if( tlen >=  8 ) {
		if((nbyte = tlen % 4)) { /* check for word align copy */
			bcopy(srcp,dstp,tlen - nbyte);
			srcp += tlen - nbyte ; 
			dstp += tlen - nbyte ;
			switch(nbyte) {
				case 1 :
					*(u_short *)(dstp) = (u_short)(*srcp & 0xff);
					break;
				case 2 :
				case 3 :
					tmp = (u_short)(*srcp++ & 0xff);
					*(u_short *)(dstp) = tmp | (u_short)(*srcp++ & 0xff) << 8;
					if(nbyte == 3 ) {
						dstp += 2 ;
						 *(u_short *)(dstp) = (u_short)(*srcp & 0xff);
					}
					break;

			}
		
		} else
			bcopy (srcp,dstp,tlen);
	} else {
		while ( tlen >= 2 ) {
			tmp = (u_short)(*srcp++ & 0xff);
			*(u_short *)(dstp) = tmp | (u_short)(*srcp++ & 0xff) << 8;
			dstp += 2;
			tlen -= 2;
		}
		if(tlen)
			*(u_short *)(dstp) = (u_short)(*srcp & 0xff);
	}
			 
}
#endif
