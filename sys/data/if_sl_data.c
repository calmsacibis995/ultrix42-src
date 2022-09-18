/*      @(#)if_sl_data.c	4.3     (ULTRIX)        2/1/91     */
/*
 * Definitions for SLIP interface data structures
 * 
 * (this exists so programs like slstats can get at the definition
 *  of sl_softc.)
 *
 * $Header: if_slvar.h,v 1.4 89/12/31 08:52:53 van Exp $
 *
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	Van Jacobson (van@helios.ee.lbl.gov), Dec 31, 1989:
 *	- Initial distribution.
 */

#if !defined(ULTRIX_3) && !defined (ULTRIX_4)

#define ULTRIX_4

#endif

#ifdef ultrix
#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/socket.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/tty.h"
#include "../h/errno.h"

#ifdef ULTRIX_3
#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#if INET
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip.h"
#endif

#else /* ULTRIX_3 */

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#if INET
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/ip.h"
#endif

#endif /* ULTRIX_3 */

#include "../h/protosw.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif
#include "../h/kmalloc.h"

#else
#include "param.h"
#include "mbuf.h"
#include "buf.h"
#include "dk.h"
#include "socket.h"
#include "ioctl.h"
#include "file.h"
#include "tty.h"
#include "errno.h"

#include "if.h"
#include "netisr.h"
#include "route.h"
#if INET
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#ifndef sun
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#else
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "protosw.h"
#endif
#endif

#ifdef vax
#include "machine/mtpr.h"
#endif
#endif

#ifdef ULTRIX_3
#include "../vaxif/slcompress.h"
#else /* ULTRIX_3 */
#include "../io/netif/slcompress.h"
#endif /* ULTRIX_3 */

#ifndef NSL
#include "sl.h"
#endif

struct sl_softc {
	struct ifnet sc_if;	/* network-visible interface */
	struct ifqueue sc_fastq; /* interactive output queue */
	struct tty *sc_ttyp;	/* pointer to tty structure */
	u_char *sc_mp;		/* pointer to next available buf char */
	u_char *sc_ep;		/* pointer to last available buf char */
	u_char *sc_buf;		/* input buffer */
	u_int sc_escape;	/* =1 if last char input was FRAME_ESCAPE */
	u_int sc_bytessent;
	u_int sc_bytesrcvd;
	struct slcompress sc_comp; /* tcp compression data */
};

/*
 * There are three per-line options kept in the device specific part
 * of the interface flags word:  IFF_D1 enables compression; IFF_D2
 * enables compression if a compressed packet is received from the
 * other side; IFF_D3 will drop (not send) ICMP packets.
 */
#ifndef IFF_D1
/*
 * This system doesn't have defines for device specific interface flags,
 * Define them.
 */
#define IFF_D1	0x8000
#define IFF_D2	0x4000
#define IFF_D3	0x2000
#endif

#ifdef BINARY

extern struct sl_softc sl_softc[];

extern int nNSL;

#else /* BINARY */

struct sl_softc sl_softc[NSL];

int nNSL = NSL;

#endif /* BINARY */

