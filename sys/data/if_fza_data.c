/*
 *	@(#)if_fza_data.c	4.6	(ULTRIX)	2/28/91
 */

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

/* -----------------------------------------------------------------------
 *
 * Modification history:
 *
 * 27-Feb-91    chc
 *      Added nduflag field
 *
 * 9-Nov-90	chc (Chran-Ham Chang)
 *	Added smt_rmc array to save the
 *	RMC descriptor for SMT frame
 *
 * 27-Apr-90	chc (Chran-Ham Chang)
 *	Created the if_fza_data.c module
 *
 *---------------------------------------------------------------------- */
#include "fza.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"  
#include "../h/kmalloc.h"
#include "../h/proc.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/if_ether.h"
#include "../net/net/ether_driver.h"
#include "../net/netinet/if_fddi.h"
#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../io/uba/ubavar.h"

#include "../io/netif/if_fzareg.h"



/*
 * The number of DEFZA receive ring is programmerable 
 * which defined  by the INIT command. The maximum is 256 
 */
#define	NFZARCV		32	

/*
 * The number of DEFZA transmit ring is programmerable
 * which defined  by the INIT command. Only two sizes 
 * of DEFZA RMC transmit ring which are 512 or 1024 
 */
#define FZA_XMT_MODE	0		/* 512 RMC transmit ring */
#define FZASHIFT	9 

#define	FZAXMTADDR(addr,base)	(((u_long)addr & 0xffff) << 5 | 0x200000) + (u_long)base	

struct cam {
	u_short addr1;		/* high order two byte */
	u_char  offset;		/* CAM address */
	u_char  result;		/* Results */
	u_long  addr2;		/* low order four bytes */
}; 	

struct rmbuf {
	struct mbuf *mbufa;
	struct mbuf *mbufb;
	u_long	phymbufa;
	u_long	phymbufb;
};

struct fzadebug {
	struct cmdcnt {
		u_short	init;
		u_short param;
		u_short modcam;
		u_short modprom;
		u_short setchar;
		u_short rdcam;
		u_long	rdcntr;
		u_long	status;
	} cmdcnt;
	struct 	fstat 	fstat;
	u_long  fzareset;
        u_long  fzaundrop;
        u_long  fzasmtrcvd;
        u_long  fzanombuf;
	u_long  fzasmtdrop;
	u_long  fzalarge;
	u_long  fzasmall;
	u_long  fzamiddle;
}; 


/*
 * FDDI software status per interface.
 * The FDDI network interface structure will based on ethernet interface 
 * structure.
 *
 * Each interface is referenced by a network interface structure,
 * ds_if, which the routing code uses to locate the interface.
 */
struct	fza_softc {
	struct	ether_driver is_ed;		/* FDDI driver */
#define	is_ac	is_ed.ess_ac			/* FDDI common part */
#define	ztime	is_ed.ess_ztime			/* Time since last zeroed */
#define	is_if	is_ac.ac_if			/* network-visible interface */
#define	is_addr	is_ac.ac_enaddr			/* hardware FDDI address */
#define is_ctrblk is_ed.ess_ctrblk		/* counter block */
	u_long	basereg;			/* base register  */		
	PORT_REG _fza_reg;			/* port register */
	struct	ifqueue is_smt;			/* FZA SMT RCV queue */
	char	is_multi[NMULTI][8];		/* multicast buffer */
	long	is_muse[NMULTI];		/* number of entrys */
	struct _fzactrs  *ctrblk;		/* Per-unit line counters */
	struct fzainit *initblk;		/* init command info */
	struct fzastatus *statusblk;		/* Per-unit status info */
	char	is_dpaddr[6];			/* Default phys. address */
	struct rmbuf rmbuf[NFZARCV];		/* receive mbuf chain */
	FZARCVRING	*rring;			/* Receive ring addr. */
	FZAXMTRING	*tring;			/* Xmit ring addr. */
	FZASMTRING 	*smttring;		/* SMT Xmit ring addr. */
	FZASMTRING	*smtrring;		/* SMT Rcv ring addr. */
	FZACMDRING	*cmdring;		/* Cmd ring addr. */
	FZAUNSRING	*unsring;		/* Unsolicited ring addr. */
	int	nrmcxmt;			/* no. of RMC xmt entrys */
	int	nsmtxmt;			/* no. of SMT xmt entrys */	
	int 	nsmtrcv;			/* no. of SMT rcv entrys */
	char	pmc_rev[4];			/* RMC module rev. */
	char 	phy_rev[4];			/* PHY module rev. */
	char 	fw_rev[4];				/* FW module rev. */
	int	mop_type;			/* MOP Device type */

	int	tindex;				/* Current xmit index */
	int	ltindex;			/* Last xmit processed */
	int	nxmit;				/* Number of active xmits */

	int	tsmtindex;			/* SMT transmit index */	
	int	rsmtindex;			/* RCV smt index */

	int	cmdindex;			/* current command index */
	int 	lcmdindex;			/* last command processed */

	int	unsindex;			/* unsolicited ring addr.*/

	int	rindex;				/* Index of last active desc. */

	u_int	t_max;
	u_int	t_req;
	u_int	tvx;
	int	lem_threshold;
	int	pmd_type;	
	int	smt_version;	
	struct 	fzactr_ent	station_id;	/* station id */
	int	flag;				/* adapter state */
	struct  fzadebug  fza_debug;		/* debug counter */
	struct	lock_t	lk_fza_softc;		/* SMP lock for xna_softc */
	/* 
	 * RMC descriptor for SMT frame need to be saved
	 * and pass back to the adapter in order to detect
	 * duplicate station address
	 */
	u_long	smt_rmc[IFQ_MAXLEN];		/* smt rmc descriptor */
	int      smtrmcindex;			/* smt rmc array index */
	int      lsmtrmcindex;			/* last smt rmc array index */
	char	 initflag;			/* a first time init flag */ 
	short	 nduflag;			/* keep the interface state */
};


#define resetaddr 	_fza_reg.fza_reset
#define ctlaaddr	_fza_reg.fza_ctl_a 
#define	ctlbaddr 	_fza_reg.fza_ctl_b 
#define intraddr	_fza_reg.fza_intr_event 
#define maskaddr	_fza_reg.fza_intr_mask  
#define statusaddr	_fza_reg.fza_status  

#define reg_reset 	_fza_reg.fza_reset->reg
#define reg_ctla	_fza_reg.fza_ctl_a->reg 
#define	reg_ctlb 	_fza_reg.fza_ctl_b->reg 
#define reg_intr	_fza_reg.fza_intr_event->reg 
#define reg_mask	_fza_reg.fza_intr_mask->reg  
#define reg_status	_fza_reg.fza_status->reg  


#define fstc_second 	sc->fza_debug.fstat.fst_second
#define fstc_frame 	sc->fza_debug.fstat.fst_frame
#define fstc_error 	sc->fza_debug.fstat.fst_error
#define fstc_lost 	sc->fza_debug.fstat.fst_lost
#define fstc_bytercvd 	sc->fza_debug.fstat.fst_bytercvd
#define fstc_bytesent 	sc->fza_debug.fstat.fst_bytesent
#define fstc_pdurcvd 	sc->fza_debug.fstat.fst_pdurcvd
#define fstc_pdusent 	sc->fza_debug.fstat.fst_pdusent
#define fstc_underrun 	sc->fza_debug.fstat.fst_underrun
#define fstc_sendfail 	sc->fza_debug.fstat.fst_sendfail
#define fstc_fcserror 	sc->fza_debug.fstat.fst_fcserror
#define fstc_fseerror 	sc->fza_debug.fstat.fst_fseerror
#define fstc_pdualig 	sc->fza_debug.fstat.fst_pdualig
#define fstc_pdulen 	sc->fza_debug.fstat.fst_pdulen
#define fstc_pduunrecog sc->fza_debug.fstat.fst_pduunrecog
#define fstc_overrun 	sc->fza_debug.fstat.fst_overrun
#define fstc_sysbuf 	sc->fza_debug.fstat.fst_sysbuf
#define fstc_userbuf 	sc->fza_debug.fstat.fst_userbuf
#define fstc_ringinit 	sc->fza_debug.fstat.fst_ringinit
#define fstc_ringinitrcv sc->fza_debug.fstat.fst_ringinitrcv
#define fstc_ringbeacon sc->fza_debug.fstat.fst_ringbeacon
#define fstc_dupaddfail sc->fza_debug.fstat.fst_dupaddfail
#define fstc_duptoken   sc->fza_debug.fstat.fst_duptoken
#define fstc_ringpurge 	sc->fza_debug.fstat.fst_ringpurge
#define fstc_bridgestrip sc->fza_debug.fstat.fst_bridgestrip
#define fstc_traceinit 	sc->fza_debug.fstat.fst_traceinit
#define fstc_tracerecv 	sc->fza_debug.fstat.fst_tracerecv
#define fstc_selftest 	sc->fza_debug.fstat.fst_selftest
#define fstc_mbytesent 	sc->fza_debug.fstat.fst_mbytesent
#define fstc_mpdusent 	sc->fza_debug.fstat.fst_mpdusent
#define fstc_mbytercvd 	sc->fza_debug.fstat.fst_mbytercvd
#define fstc_mpdurcvd 	sc->fza_debug.fstat.fst_mpdurcvd
#define fstc_mpduunrecog sc->fza_debug.fstat.fst_mpduunrecog
#define fstc_connection sc->fza_debug.fstat.fst_connection
#define fstc_tne_exp_rej sc->fza_debug.fstat.fst_tne_exp_rej
#define fstc_lct_rej  	sc->fza_debug.fstat.fst_lct_rej
#define fstc_lem_rej  	sc->fza_debug.fstat.fst_lem_rej
#define fzanreset	sc->fza_debug.fzareset	
#define fzanundrop	sc->fza_debug.fzaundrop
#define fzansmtrcvd	sc->fza_debug.fzasmtrcvd
#define fzansmtdrop	sc->fza_debug.fzasmtdrop
#define fzannombuf	sc->fza_debug.fzanombuf
#define fzanlarge	sc->fza_debug.fzalarge
#define fzanmiddle	sc->fza_debug.fzamiddle
#define fzansmall	sc->fza_debug.fzasmall

#ifdef BINARY

extern	struct 	fza_softc fza_softc[];
extern	struct	uba_device *fzainfo[];

#else BINARY

struct 	fza_softc fza_softc[NFZA];
struct	uba_device *fzainfo[NFZA];

#endif	BINARY
