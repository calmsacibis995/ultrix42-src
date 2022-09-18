/*
 * @(#)if_ln_data.c	4.6	(ULTRIX)	1/22/91
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985-1989 by			*
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
 *  Modification History:						*
 *									*
 *  21-Jan-91	Randall Brown						*
 *	Added stubs for routines found only on mips side       		*
 *									*
 *  6-Jul-90	Lea Gottfredsen						*	
 *	3min support. Added additional fields to lnsw.	        	*
 *								        *
 *  29-Jun-89	Lea Gottfredsen						*
 *	Merge of isis and pu pools, added back in packet filter		*
 * 	changes and lock to softc; multi-unit support			*
 *									*
 *  1-Jun-89 -- Lea Gottfredsen						*
 *	Added lnsw structure in order to easily accommodate		* 
 *	new hardward support.					 	*
 *									*
 *  14-Dec-88 -- templin (Fred L. Templin)				*
 *	Hardwired NRCV and NXMT to 16 for all architectures		*
 *									*
 *   28-sep-88 -- jaw							*
 *      added lock field to softc struct				*
 *									*
 *   7-Jan-88 -- templin (Fred L. Templin)				*
 *	Created the if_ln_data.c module. This module is based upon	*
 *	a modified version of the if_se_data.c module.			*
 *									*
 ************************************************************************/

/*
 * Digital LANCE NI
 */

#include "ln.h"

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
#include "../h/proc.h"  /* Needed for nonsymmetric drivers. us */

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/if_ether.h"
#include "../net/net/ether_driver.h"

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../io/netif/if_lnreg.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/tc/tc.h"
#ifdef vax
#include "../machine/cvax.h"
#include "../machine/mtpr.h"
#else
/* a dummy structure added to allow compilation. */
struct ni_regs
{
	u_long ni_rdp;		
	u_long ni_rap;	
	u_long *ni_sar;
	u_long *ni_nilrb;
};
						
#endif vax
#ifndef mips 
#define PHYS_TO_K1(x)	((x))
#define PHYS_TO_K0(x)	((x))
#endif 
#define RLEN	4	/* 2**4 = 16  receive descriptors */
#define TLEN	4	/* 2**4 = 16  transmit descriptors */

#define NRCV	(0x0001<<RLEN) 	/* Receive descriptors */
#define NXMT	(0x0001<<TLEN) 	/* Transmit descriptors	*/
#define NTOT	(NXMT + NRCV)
#define NMULTI	64		/* Size of multicast address table */
#define MINDATA 60

struct ln_multi {
	u_char	ln_multi_char[6];
};
#define MULTISIZE sizeof(struct ln_multi)

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * is_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
#define	is_if	is_ac.ac_if		/* network-visible interface 	*/
#define	is_addr	is_ac.ac_enaddr		/* hardware Ethernet address 	*/

struct lnreg {
#ifdef mips 
	volatile unsigned short reg;
#else
	u_short reg;
#endif
};

struct	ln_softc {

        struct  ether_driver is_ed;     /* Ethernet driver common part    */
#define is_ac   is_ed.ess_ac            /* Ethernet common part */

	/*
	 * address of "lndevice" block for this unit, plus LRAM initb
	 */
	struct lnreg *rdpaddr;		/* ptr to register data port */
	struct lnreg *rapaddr;		/* ptr to register addr port */
	u_long	*ln_narom;		/* ptr to Network addr rom */
#ifdef vax
	u_char 	*ln_lrb;		/* ptr to Local ram buffer */
#else /* mips */
	volatile unsigned char *ln_lrb;	/* ptr to Local ram buffer */
#endif 
	u_long	*ldpaddr;		/* ptr to Lance DMA register */
	u_long	*ssraddr;		/* ptr to System Support reg */
	u_long	*siraddr;		/* ptr to Sys Interrupt reg  */

	/* 
	 * our own copy of LRB structures
 	 */
	struct	ln_initb ln_initb;	/* init block */
	struct  ln_ring ln_ring;	/* ring entry */
	
	struct lnsw *lnsw;

	/*
	 * list of local RAM buffers 
	 */
	caddr_t	initbaddr;		/* Init block address		*/
	caddr_t	rring[NRCV];		/* Receive ring desc. addresses */
	caddr_t	tring[NXMT];		/* Transmit ring desc. addresses */
	caddr_t	rlbuf[NRCV];		/* Receive local RAM buffers	*/
	caddr_t	tlbuf[NXMT];		/* Transmit local RAM buffers	*/
	struct  mbuf *tmbuf[NXMT+1];	/* Xmt mbuf chains (freed on xmt) */
	struct  mbuf *rmbuf[NRCV+1];	/* Receive mbuf chains  */
	u_char	dma[NRCV];	        /* for DMA receiver architectures */ 

	struct	ln_multi multi[NMULTI];	/* Multicast address list	*/
#define ctrblk  is_ed.ess_ctrblk        /* Counter block		*/
 	u_char	muse[NMULTI];		/* Multicast address usage count*/
#define ztime   is_ed.ess_ztime         /* Time counters last zeroed      */
	int	rindex;			/* Receive index		*/
	int	tindex;			/* Transmit index		*/
	int	otindex;		/* Old transmit index		*/
 	int	nxmit;			/* Transmits in progress	*/
	int	nmulti;			/* Current # of multicast slots */
	int	lrb_offset;		/* Current allocation offset	*/
	int ln_crc;			/* crc, must be declared global */
	int callno;			/* lninit called */
	struct lndebug { 
		unsigned int trfull;	/* transmit side called lnwatch */
		int ln_showmulti; 	/* debug: show multicast add/delete */
		int ln_bablcnt;		/* transmitter timeout counter */
		int ln_misscnt;		/* missed packet counter */
		int ln_merrcnt;		/* memory error counter */
		int ln_restarts;	/* number of times chip was restarted */
		int ln_dmareaderr;	/* # dma read errors (ioasic parity) */
	} ln_debug;
	struct lock_t lk_ln_softc;	/* SMP lock */
};

#define ln_rdp	rdpaddr->reg
#define ln_rap	rapaddr->reg
#define lnshowmulti sc->ln_debug.ln_showmulti
#define lnbablcnt sc->ln_debug.ln_bablcnt
#define lnmisscnt sc->ln_debug.ln_misscnt
#define lnmerrcnt sc->ln_debug.ln_merrcnt
#define lnrestarts sc->ln_debug.ln_restarts
#define lndmareaderr sc->ln_debug.ln_dmareaderr

/*
 * LANCE "switch" structure. One structure PER ARCHITECTURE, PER UNIT.
 * Per-architecture tables are indexed by unit number.
 */
struct lnsw {
	u_long	ln_phys_rdp;		/* Phys. address of RDP */
	u_long	ln_phys_rap;		/* Phys. address of RAP */
	u_long	ln_phys_narom;		/* Phys. address of NA ROM */
	u_long	ln_phys_lrb;		/* Phys. address of Local RAM BuF. */
	u_long	ln_phys_ldp;		/* DMA Physical address */
	u_long	ln_phys_ssr;		/* System Support Register */
	u_long	ln_phys_sir;		/* System Interrupt Register */
	int	ln_na_align;		/* Byte offset for NA ROM */
	int	ln_dodma;		/* for lance DMA */
	int	ln_dma;			/* for strange lance DMA 3min style */
	caddr_t	(*ln_cpyin)();		/* Routine to copy desc/initb FROM LRB */
	caddr_t	(*ln_cpyout)();		/* Routine to copy desc/initb TO LRB */
	caddr_t (*ln_alloc)();		/* Routine to alloc d/i out of LRB */
	int	(*ln_bzero)();		/* Routine to zero d/i portions of LRB */
	int	(*ln_svtolance)();	/* Routine to map virtual to lance */
	caddr_t	(*ln_cpyinb)();		/* Routine to copy buffer data FROM LRB */
	caddr_t	(*ln_cpyoutb)();		/* Routine to copy buffer data TO LRB */
	caddr_t (*ln_allocb)();		/* Routine to alloc buffers out of LRB */
	int	(*ln_bzerob)();		/* Routine to zero buf portions of LRB */
	int	(*ln_setflag)();	/* Routine to set ring ownership flag */
	int     (*lninitdesc)();	/* Routine to initilize ring */
	struct mbuf * (*lnget)();	/* Routine to do ln get */

};

#ifdef BINARY
extern struct lnsw mayfairsw[], ffoxsw[], pmaxsw[], ds5400sw[],
	vaxstarsw[],ds5000sw[],ds3minsw[];
#else
extern int ln_bzero16(), ln_bzero32(), ln_setflag16(), ln_setflag32();
extern int svtolance16(), svtolance32(), lninitdesc(), lninitdesc_dma();
extern caddr_t ln_cpyout32(), ln_cpyout16(), ln_cpyin16(), ln_cpyin32();
extern caddr_t ln_alloc16(), ln_alloc32(), ln_alloc4x4();
extern int ln_bzero4x4();
extern caddr_t ln_cpyout4x4(), ln_cpyin4x4();
extern struct mbuf *lnget(), *lnget_dma();
/*
 * VAX 3400 - mayfair II
 */
struct lnsw mayfairsw[] = { 
/* Unit 0 */
{	0,	0,	0,	0,	0,	0,	0, 
	0,	LN_NONDMA_RCV,	0,	ln_cpyin16,	ln_cpyout16,	ln_alloc16,
	ln_bzero16,	svtolance16,	ln_cpyin16,	ln_cpyout16, ln_alloc16,
	ln_bzero16,	ln_setflag16,	lninitdesc,	lnget }
};
/*
 * VAX 60 - Firefox
 */
struct lnsw ffoxsw[] = {
/* Unit 0 */
{	0,	0,	0,	0, 	0,	0,	0,
	0,	LN_NONDMA_RCV,	0,	ln_cpyin32,	ln_cpyout32,	ln_alloc32,
	ln_bzero32,	svtolance32,	ln_cpyin32,	ln_cpyout32,	
	ln_alloc32,	ln_bzero32,	ln_setflag32,
	lninitdesc,	lnget }
};
/*
 * DecStation3100 original PMAX
 */

struct lnsw pmaxsw[] = {
/* Unit 0 */
{	0x18000000,	0x18000004,	0x1d000000,	0x19000000,
	0,	0,	0,	8,	LN_NONDMA_RCV,	0,	ln_cpyin16,
	ln_cpyout16,	ln_alloc16,	ln_bzero16,	svtolance16,
	ln_cpyin16,	ln_cpyout16,	ln_alloc16,	ln_bzero16,	
	ln_setflag16,	lninitdesc,	lnget }
};
/*
 * DecStation5400 - Mipsfair
 */
struct lnsw ds5400sw[] = {
/* Unit 0 */
{	0x10084400,	0x10084404,	0x10084200,	0x10120000,
	0,	0,	0,	0,	LN_NONDMA_RCV,	0,	ln_cpyin32,
	ln_cpyout32,	ln_alloc32,	ln_bzero32,	svtolance32,	
	ln_cpyin32,	ln_cpyout32,	ln_alloc32,	ln_bzero32,	
	ln_setflag32,	lninitdesc,	lnget }
};
/*
 * VS2000 and VS3100
 */
struct lnsw vaxstarsw[]= {
{	0,	0,	0,	0,	0,	0,	0,
	0,	LN_DMA_RCV,	0,	ln_cpyin32,	ln_cpyout32,	ln_alloc32,
	ln_bzero32,	svtolance32,	ln_cpyin32,	ln_cpyout32,	
	ln_alloc32,	ln_bzero32,	ln_setflag32,	
	lninitdesc_dma,	lnget_dma }
}; 
/*
 * DecStation5000 - 3MAX
 */
struct lnsw ds5000sw[] = {
/* Unit 0 */
{	0x00100000,   0x00100004,     0x001C0000,     0x00000000,
        0,	0,	0,	16,     LN_NONDMA_RCV,	0,	ln_cpyin32,
	ln_cpyout32,	ln_alloc32,	ln_bzero32,	svtolance32,
	ln_cpyin32,	ln_cpyout32,	ln_alloc32,	ln_bzero32,	
	ln_setflag32,	lninitdesc,	lnget }
};
/*
 * DecStation5000 Model 100 - 3MIN
 */
struct lnsw ds3minsw[] = {
/* Unit 0 */
{ 	0x1C0C0000,   0x1C0C0004,     0x1C080000,     0x00000000,
	0x1C040020,	0x1C040100,	0x1C040110,	0,     LN_NONDMA_RCV,
	LN_DMA_3MIN,	ln_cpyin16,	ln_cpyout16,	 ln_alloc16,	ln_bzero16,
	svtolance16,	ln_cpyin4x4,	ln_cpyout4x4,	ln_alloc4x4,
	ln_bzero4x4,	ln_setflag16,	lninitdesc,	lnget}
};

#endif



#ifdef BINARY

extern	struct	ln_softc *ln_softc[];
extern	struct	uba_device *lninfo[];
extern  int	nLNNRCV;
extern	int	nLNNXMT;
extern	int	nLNNTOT;
extern	int	nLNMULTI;

#if defined VAX420 || defined MVAX
extern  char ln_lrb[][LN_LRB_SIZE];
#else
extern char ln_lrb[][1];
#endif 

#else

#ifndef mips
tc_addr_to_name(){}
tc_isolate_memerr(){}
clean_dcache(){}
ln_clean_dcache4x4(){}
caddr_t ln_cpyin4x4s(){}
caddr_t ln_cpyout4x4(){}
#endif nmips
/*
 * added multiple support to softc. Also, the ln_initb blocks will be
 * directly accessed in the local RAM buffers
 */
struct	ln_softc  *ln_softc[NLN];
struct	uba_device *lninfo[NLN];
int	nLNNXMT = NXMT;
int 	nLNNRCV = NRCV;
int	nLNNTOT = NTOT;
int	nLNMULTI = NMULTI;

/* the following because of 24 bit addressing on lance */
#if defined VAX420 || defined MVAX
char    ln_lrb[NLN][LN_LRB_SIZE];	 
#else
char	ln_lrb[NLN][1];
#endif 

#endif
