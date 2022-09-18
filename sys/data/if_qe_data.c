/*
 * @(#)if_qe_data.c	4.1  (ULTRIX)        7/2/90
 */

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

/************************************************************************
 *			Modification History				*
 *								        *
 *      Chran-Ham Chang - 06/08/89   				        *
 *		Only the VAX processors need mtpr.h file 	        *
 *									*
 *	Uttam Shikarpur 3-May-89					*
 *	Merged packetfilter changes made by Jeff Mogul			*
 *	Revised to use ether_driver substructure			*
 *	Incorporated R.Bhanukitsiri's changes to reflect v3.2		*
 *	source change.							*
 *									*
 *	Larry Palmer - 15-Jan-88					*
 *		Changed definition of qe_softc to a pointer (malloced)	*
 *									*
 *	Bill Dallas - 05/31/87 (by jsd)					*
 *		Changed qe_softc structure for recv buffer invalid.	*
 *		Can now detect if we have to qerestart and/or process	*
 *		the recv ring.						*
 *									*
 *	Fred L. Templin - 03/11/87					*
 *		Changed number of receive buffers BACK to 25 due to	*
 *		the addition of "qballoc" code which grabs map		*
 *		registers out of the entire Q-bus map register block.	*
 *		(Previously, only 512 of the 8096 Q-bus map registers	*
 *		were accessed due to use of "uballoc".			*
 *									*
 *	Robin Lewis - 12/15/86						*
 *		Changed receive buffers from 25 to 15 in an effort to	*
 *		recover some map registers on the UNIBUS.		*
 *									*
 *	Ed Ferris - 09/2/86						*
 *		Changed minimum packet size from 64 to 60. 		*
 *									*
 *	Jeff Chase - 03/12/86						*
 *		Changed number of receive buffers from 8 to 25.		*
 *		The number 25 is arbitrary, but the number should be	*
 *		at least 20.						*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

#include "qe.h"
#include "packetfilter.h"		/* NPACKETFILTER */
/*
 * Digital Q-BUS to NI Adapter
 */
#include "../machine/pte.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/ipc.h"
#include "../h/shm.h"

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
#ifdef vax
#include "../machine/mtpr.h"
#endif
#include "../io/netif/if_qereg.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"


#define NRCV	25	 		/* Receive descriptors		*/
#define NXMT	5	 		/* Transmit descriptors		*/
#define NTOT	(NXMT + NRCV)
#define NMULTI	12			/* Number of multicast addresses*/
#define QBMSIZ	100			/* Q-bus map resource */
#define MAXDEQNA 4			/* 2 deqna / 2 deqla */

/*
 * This constant should really be 60 because the qna adds 4 bytes of crc.
 * However when set to 60 our packets are ignored by deuna's , 3coms are
 * okay ??????????????????????????????????????????
 *
 * Note: The bug was in the qe driver itself.  Whenever an odd number of
 *	 bytes less than the minimum packet size was sent, the driver
 *	 would set the "odd" bit in the descriptor and then reset the
 *	 length to the minimum packe size which happened to be "even."
 */
#define MINDATA 60

/*
 * The qeuba structures generalizes the ifuba structure
 * to an arbitrary number of receive and transmit buffers.
 */
struct	ifxmt {
	struct	ifrw x_ifrw;			/* mapping information */
	struct	pte x_map[IF_MAXNUBAMR];	/* output base pages */
	short	x_xswapd;			/* mask of clusters swapped */
	struct	mbuf *x_xtofree;		/* pages being dma'ed out */
};

struct	qeuba {
	short	ifu_uban;		/* uba number */
	short	ifu_hlen;		/* local net heaqer length */
	struct	uba_regs *ifu_uba;	/* uba regs, in vm */
	struct	ifrw ifu_r[NRCV];	/* receive information */
	struct	ifxmt ifu_w[NXMT];	/* transmit information */
	short	ifu_flags;		/* used during uballoc's */
};
struct qe_multi {
	u_char	qm_char[6];
};
#define MULTISIZE sizeof(struct qe_multi)

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * is_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
#define	is_if	is_ac.ac_if		/* network-visible interface 	*/
#define	is_addr	is_ac.ac_enaddr		/* hardware Ethernet address 	*/

struct	qe_softc {
	struct	ether_driver qe_ed;	/* Ethernet driver common part	*/
#define	is_ac	qe_ed.ess_ac		/* Ethernet common part 	*/
#define ctrblk	qe_ed.ess_ctrblk	/* Counter block		*/
#define	ztime	qe_ed.ess_ztime		/* Time counters last zeroed	*/
	struct	qeuba qeuba;		/* Q-bus resources 		*/
	int	setupaddr;		/* mapping info for setup pkts  */
	struct	qe_ring *rringaddr;	/* mapping info for rings	*/
	struct	qe_ring *tringaddr;	/*       ""			*/
	struct	qe_ring rring[NRCV+1];	/* Receive ring descriptors 	*/
	struct	qe_ring tring[NXMT+1];	/* Transmit ring descriptors 	*/
	u_char	setup_pkt[16][8];	/* Setup packet			*/
	struct	qe_multi multi[NMULTI];	/* Multicast address list	*/
	u_char	muse[NMULTI];		/* Multicast address usage count*/
	int	rindex;			/* Receive index		*/
	int	tindex;			/* Transmit index		*/
	int	otindex;		/* Old transmit index		*/
	int	qe_intvec;		/* Interrupt vector 		*/
	struct	qedevice *addr;		/* device addr			*/
	int 	setupqueued;		/* setup packet queued		*/
	int	nxmit;			/* Transmits in progress	*/
	char	*buffers;		/* Buffers for packets		*/
	int	timeout;		/* watchdog			*/
	int	qe_rl_invalid;		/* recv buffer invalid		*/
	struct	map *qb_map;		/* Q bus resource map		*/
	struct	qmap_regs *qb_mregs;	/* Q bus map registers		*/
};

#ifdef BINARY

extern	struct	qe_softc *qe_softc[];
extern	struct	uba_device *qeinfo[];
extern	int	nNQE;
extern  int	nNRCV;
extern	int	nNXMT;
extern	int	nNTOT;

#else BINARY

struct	qe_softc  *qe_softc[MAXDEQNA];
struct	uba_device *qeinfo[MAXDEQNA];
int	nNQE = 0;
int	nNXMT = NXMT;
int 	nNRCV = NRCV;
int	nNTOT = NTOT;

#endif
