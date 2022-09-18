/*
 * static       char    *sccsid = "@(#)if_de_data.c	4.1  (ULTRIX)        7/2/90";
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
 *			Modification History				
 * 	Uttam Shikarpur 3-May-1989
 *	Picked up R.Bhanukitsiri's changes or 03/06/89
 *	to reflect v3.2 source pool changes.
 *	Merged in Jeffrey Mogul's packet filter changes;revised
 *	the ether_driver substructure to have only ONE declaration.
 *									
 *	Larry Cohen  -	09/16/85					
 * 		Add 43bsd alpha tape changes for subnet routing		
 *									
 * 
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * 11-jul-85 -- jaw
 *	fix bua/bda map registers.
 ************************************************************************/

#include "de.h"

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

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/netif/if_dereg.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"


#define	NXMT	8	/* number of transmit buffers */
#define	NRCV	8	/* number of receive buffers (must be > 1) */
#define	NTOT	(NXMT + NRCV)
#define NMULTI	10	/* number of multicast table entries */

/*
 * The deuba structures generalizes the ifuba structure
 * to an arbitrary number of receive and transmit buffers.
 */
struct	ifxmt {
	struct	ifrw x_ifrw;			/* mapping information */
	struct	pte x_map[IF_MAXNUBAMR];	/* output base pages */
	short	x_xswapd;			/* mask of clusters swapped */
	struct	mbuf *x_xtofree;		/* pages being dma'ed out */
};

struct	deuba {
	short	ifu_uban;		/* uba number */
	short	ifu_hlen;		/* local net header length */
	union {
	 struct	uba_regs *iifu_uba;	/* uba regs, in vm */
	 struct	bua_regs *iifu_bua;	/* bua regs, in vm */
	} ifubatype;
#define	ifu_uba	ifubatype.iifu_uba
#define	ifu_bua	ifubatype.iifu_bua
	struct	ifrw ifu_r[NRCV];	/* receive information */
	struct	ifxmt ifu_w[NXMT];	/* transmit information */
	short	ifu_flags;		/* used during uballoc's */
};

/*
 * Multicast list structure
 */
struct de_multi_add {
	u_char dm_char[6];
};
#define MULTISIZE sizeof(struct de_multi_add)

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ds_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
struct	de_softc {
	struct	ether_driver ds_ed;	/* Ethernet driver common part	*/
#define	ds_ac	ds_ed.ess_ac		/* Ethernet common part 	*/
#define	ds_if	ds_ac.ac_if		/* network-visible interface */
#define	ds_addr	ds_ac.ac_enaddr		/* hardware Ethernet address */
#define	ds_ztime ds_ed.ess_ztime	/* Time counters last zeroed	*/
#define ds_ctrblk ds_ed.ess_ctrblk	/* Generic Counter block	*/
#define	ds_unrecog ds_ed.ess_ctrblk.est_unrecog
					/* unrecognized frame destination */
	int	ds_flags;
#define	DSF_LOCK	1		/* lock out destart */
#define	DSF_RUNNING	2
	int	ds_ubaddr;		/* map info for incore structs */
	struct	deuba ds_deuba;		/* unibus resource structure */
	/* the following structures are always mapped in */
	struct	de_pcbb ds_pcbb;	/* port control block */
	struct	de_ring ds_xrent[NXMT];	/* transmit ring entrys */
	struct	de_ring ds_rrent[NRCV];	/* receive ring entrys */
	struct	de_udbbuf ds_udbbuf;	/* UNIBUS data buffer */
	struct	de_multi_add ds_multicast[NMULTI]; /* multicast address list */
	struct	de_counters ds_counters;/* counter block */
	/* end mapped area */
#define	INCORE_BASE(p)	((char *)&(p)->ds_pcbb)
#define	RVAL_OFF(n)	((char *)&de_softc[0].n - INCORE_BASE(&de_softc[0]))
#define	LVAL_OFF(n)	((char *)de_softc[0].n - INCORE_BASE(&de_softc[0]))
#define	PCBB_OFFSET	RVAL_OFF(ds_pcbb)
#define	XRENT_OFFSET	LVAL_OFF(ds_xrent)
#define	RRENT_OFFSET	LVAL_OFF(ds_rrent)
#define	UDBBUF_OFFSET	RVAL_OFF(ds_udbbuf)
#define MULTI_OFFSET	RVAL_OFF(ds_multicast[0])
#define COUNTER_OFFSET	RVAL_OFF(ds_counters)
#define	INCORE_SIZE	RVAL_OFF(ds_xindex)
	int	ds_xindex;		/* UNA index into transmit chain */
	int	ds_rindex;		/* UNA index into receive chain */
	int	ds_xfree;		/* index for next transmit buffer */
	int	ds_nxmit;		/* # of transmits in progress */
	u_char	ds_muse[NMULTI];	/* multicast address use count */
	u_char	ds_devid;		/* device id DEUNA=0, DELUA=1 */
	struct	lock_t lk_de_softc; 	/* SMP lock for de_softc */
};


#ifdef	BINARY

extern	struct	uba_device *deinfo[];

extern	struct	de_softc    de_softc[];

extern int	nNDE;

#else

struct uba_device	*deinfo[NDE];

struct	de_softc	de_softc[NDE];

int nNDE = NDE;

#endif

