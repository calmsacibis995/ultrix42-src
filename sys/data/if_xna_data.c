/*
 *	@(#)if_xna_data.c	4.1	(ULTRIX)	7/2/90
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


/************************************************************************
 *			Modification History				*
 *									*
 *	6/14/89 - Fred L. Templin (templin@decvax)			*
 *		  Packet Filter support					*
 *									*
 *	4/08/89 - Fred L. Templin (templin@decvax)			*
 *		  Chaged #include lines to reflect the new		*
 *		  pool hierarchy.					*
 *									*
 *	4/08/89 - Fred L. Templin (templin@decvax)			*
 *		  SMP changes to make XNA driver SMP-safe		*
 *									*
 *	8/24/88 - Fred L. Templin (templin@decvax)			*
 *		  Created this file					*
 *									*
 ************************************************************************/

#include "xna.h"

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

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/scb.h"

#include "../io/bi/bireg.h"
#include "../io/xmi/xmireg.h"
#include "../io/netif/if_xnareg.h"

#include "../io/uba/ubavar.h"

/*
 * NRCV is a MAXIMUM of 64, but we won't map all 64 right off the bat due
 * to buffer allocations. The number of active receive descriptors will
 * grow dynamically as advised by the p_sbua counter in the port data
 * block. We'll start out with 8 active receive descriptors, and grow
 * between the range: XNANMIN <= nactv <= XNANMAX. NCMD depends on
 * XNA_XMIT_NBUFS.
 */
#define	NRCV		64
#define NACTV		8
#define XNANMAX		32
#define	XNANMIN		4
#define	NCMD		(1024 / ((XNA_XMIT_NBUFS + 1) * 8))

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ds_if, which the routing code uses to locate the interface.
 */
struct	xna_softc {
	struct	ether_driver is_ed;		/* Ethernet driver */
#define	is_ac	is_ed.ess_ac			/* Ethernet common part */
#define	ztime	is_ed.ess_ztime			/* Time since last zeroed */
#define	is_if	is_ac.ac_if			/* network-visible interface */
#define	is_addr	is_ac.ac_enaddr			/* hardware Ethernet address */
	struct	ifqueue if_cmd;			/* XNA command queue */
	struct	xnacmd_buf ctrblk;		/* Per-unit line counters */
	char	is_dpaddr[6];			/* Default phys. address */
	char	is_multi[NMULTI][8];
	long	is_muse[NMULTI];
	struct	xnadevice	xregs;		/* Addresses of xna reg's */
	struct	xnarecv_ring	*rring;		/* Receive ring addr. */
	struct	xnacmd_ring	*tring;		/* Cmd/xmit ring addr. */
	struct	xnapdb		*xpdb;		/* Port data block addr. */
	struct	xnactr_ent	xna_sbuacnt;	/* Sys buf unavail advice */
	struct	xnactr_ent	xna_ubuacnt;	/* User buf unavail ctr */
	int	nactv;				/* # recv desc. to activate */
	int	nrecv;				/* # active recv desc. */
	int	tindex;				/* Current cmd/xmit index */
	int	tlast;				/* Last cmd/xmit processed */
	int	nxmit;				/* Number of active xmits */
	int	rindex;				/* Index of last active desc. */
	int	rlast;				/* Last receive processed */
	int	flags;				/* To indicate reset */
	u_long	nproc;				/* # of entries processed */
	struct	lock_t	lk_xna_softc;		/* SMP lock for xna_softc */
};

#define XNA_RFLAG       0x01                    /* Interface is being reset */

#ifdef BINARY

extern	struct xna_softc xna_softc[];
extern	int nNXNARCV;
extern	int nNXNAACTV;
extern	int nNXNACMD;

#else BINARY

int xnaprobe(), xnaattach();

struct uba_device *tmpxna_info[NXNA];
u_short xna_std[] = { 0 };
struct uba_driver xnadriver =
	{ xnaprobe, 0, xnaattach, 0, xna_std, "xna_ethernet",
		tmpxna_info, "xna", 0};

struct	xna_softc xna_softc[NXNA];
int	nNXNARCV = NRCV;
int	nNXNAACTV = NACTV;
int	nNXNACMD = NCMD;

#endif	BINARY
