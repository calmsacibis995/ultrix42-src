/*
 *	@(#)if_ni_data.c	4.1	(ULTRIX)	7/2/90
 */

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


/************************************************************************
 *			Modification History				*
 *
 *									*
 *	Uttam Shikarpur 13-June-89
 *	Merged packet filter changes made by Jeff Mogul
 *	Revised ether_driver substructure and also to have
 *	only one declaration for struct ni_softc.
 *
 *	Picked up SMP/pmax changes
 *
 * 	12-Dec-86  -- lp	Added dpaddr.
 * 
 * 	5-Jun-86   -- jaw 	changes to config.
 *
 *	09-Apr-86 -- lp							*
 *	Created by Larry Palmer (lp!decvax)				*
 *									*
 ************************************************************************/

#include "bvpni.h"
#include "packetfilter.h"	/* NPACKETFILTER */

#include "../machine/pte.h"
#include "../machine/psl.h"

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
#include "../io/netif/if_dereg.h"
#include "../io/netif/if_uba.h"

#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"
#include "../io/bi/nireg.h"

#include "../io/uba/ubavar.h"


/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ds_if, which the routing code uses to locate the interface.
 */
struct	ni_softc {
	struct	ether_driver ds_ed;	/* Ethernet driver common part	*/
#define	ds_ac	ds_ed.ess_ac		/* Ethernet common part 	*/
#define	ds_if	ds_ac.ac_if		/* network-visible interface */
#define	ds_addr	ds_ac.ac_enaddr		/* hardware Ethernet address */
	int	ds_flags;
	long	ds_devid;
	u_char  ds_multi[NMULTI][8];
	long	ds_muse[NMULTI];
	u_char  ds_dpaddr[6];
	struct	lock_t	lk_ni_softc;	/* SMP lock for ni_softc	*/
	int	nxmit;			/* Count of transmits in progress */
};

#ifdef	BINARY

extern	struct	ni niinfo[];
extern	struct	ni_softc ni_softc[];

extern int	nNI;

#else

struct ni niinfo[NBVPNI];
struct	ni_softc ni_softc[NBVPNI];

int nNI = NBVPNI;


#endif

