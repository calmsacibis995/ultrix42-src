/*
 * static	char	*sccsid = "@(#)if_vv_data.c	4.1	(ULTRIX)	7/2/90";
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
 *									*
 *	30-May-89	darrell						*
 *		Added include of ../../machine/common/cpuconf.h -- cpu  *
 *		types were moved there.					*
 *									*
 *	R. Bhanukitsiri - 02/06/89					*
 *		Reflect V3.2 source pool changes.			*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

#include "vv.h"
/*
 * Proteon 10 Meg Ring Driver.
 * This device is called "vv" because its "real name",
 * V2LNI won't work if shortened to the obvious "v2".
 * Hence the subterfuge.
 *
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/errno.h"
#include "../h/ioctl.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/mtpr.h"
#include "../io/netif/if_vv.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

/*
 * 80 megabit configuration
 * Uncomment the next line if you are using the 80 megabit system. The
 * only change is the disposition of packets with parity/link_data_error
 * indication.
 */
/* #define PRONET80 */


#ifdef BINARY
extern struct	uba_device *vvinfo[];
extern int nNVV;
#else
struct	uba_device *vvinfo[NVV];
int nNVV = NVV;
#endif


/*
 * Software status of each interface.
 *
 * Each interface is referenced by a network interface structure,
 * vs_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
#ifdef BINARY
struct	vv_softc {
	struct	ifnet vs_if;		/* network-visible interface */
	struct	ifuba vs_ifuba;		/* UNIBUS resources */
	int	vs_host;
	short	vs_oactive;		/* is output active */
        short   vs_is80;                /* is 80 megabit version */
	short	vs_olen;		/* length of last output */
	u_short	vs_lastx;		/* address of last packet sent */
	u_short	vs_lastr;		/* address of last packet received */
	short	vs_tries;		/* transmit current retry count */
	short	vs_init;		/* number of ring inits */
	short	vs_refused;		/* number of packets refused */
	short	vs_timeouts;		/* number of transmit timeouts */
	short	vs_otimeout;		/* number of output timeouts */
	short	vs_ibadf;		/* number of input bad formats */
	short	vs_parity;		/* number of parity errors on 10 meg, */
					/* link data errors on 80 meg */
} vv_softc[];
#else
struct	vv_softc {
	struct	ifnet vs_if;		/* network-visible interface */
	struct	ifuba vs_ifuba;		/* UNIBUS resources */
	int	vs_host;
	short	vs_oactive;		/* is output active */
	short	vs_olen;		/* length of last output */
        short   vs_is80;                /* is 80 megabit version */
	u_short	vs_lastx;		/* address of last packet sent */
	u_short	vs_lastr;		/* address of last packet received */
	short	vs_tries;		/* transmit current retry count */
	short	vs_init;		/* number of ring inits */
	short	vs_refused;		/* number of packets refused */
	short	vs_timeouts;		/* number of transmit timeouts */
	short	vs_otimeout;		/* number of output timeouts */
	short	vs_ibadf;		/* number of input bad formats */
	short	vs_parity;		/* number of parity errors on 10 meg, */
					/* link data errors on 80 meg */
} vv_softc[NVV];
#endif
