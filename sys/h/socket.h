/* static	char	*sccsid = "@(#)socket.h	4.4	(ULTRIX)	11/14/90"; */

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
 *	Ursula Sinkewicz - 9-Sept-90					*
 *		Added AF_X25						*
 *	Matt Thomas - 19-Aug-90						*
 *		Add AF_NETMAN	                 			*
 *	Michael Mc Menemy - 03-May-89					*
 *		Add XTI support                 			*
 *	Larry Palmer - 15-Jan-88					*
 *		Changes for 43BSD. Added APPLETALK.			*
 *	John Forecast - -8/06/86					*
 *		Change LAS File Server domain name			*
 *	John Forecast - 04/01/86					*
 *		Add LAS File Server control domain			*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes  				*
 *	Larry Cohen  -  09/20/85					*
 *		SO_DONTLINGER now defunct so it was removed		*
 *      Ursula Sinkewic - 03/05/86			                *
 *		Added changes for BISYNC 2780/3780		        *
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	socket.h	6.6 (Berkeley) 6/8/85
 */

#ifndef KERNEL
#include <sys/types.h>  /* for u_short */
#else /* KERNEL */
#include "../h/types.h"
#endif /* KERNEL */

#ifndef XTI /* define XTI if not defined */
#define XTI
#endif /*  XTI */

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Types
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#define	SOCK_RDM	4		/* reliably-delivered message */
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x01		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x02		/* socket has had listen() */
#define	SO_REUSEADDR	0x04		/* allow local address reuse */
#define	SO_KEEPALIVE	0x08		/* keep connections alive */
#define	SO_DONTROUTE	0x10		/* just use interface addresses */
#define	SO_BROADCAST	0x20		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x40		/* bypass hardware when possible */
#define	SO_LINGER	0x80		/* linger on close if data present */
#define SO_OOBINLINE	0x100		/* leave received OOB data in line */
#ifdef XTI
#define SO_BUFFOOB	0x200           /* OOB data will be socket buffered */
#endif /* XTI */

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF	0x1001		/* send buffer size */
#define SO_RCVBUF	0x1002		/* receive buffer size */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define SO_SNDTIMEO	0x1005		/* send timeout */
#define SO_RCVTIMEO	0x1006		/* receive timeout */
#define SO_ERROR	0x1007		/* get error status and clear */
#define SO_TYPE		0x1008		/* get socket type */
#ifdef XTI
#define SO_XTIFDVALID   0x1009          /* set/get xti endpoint valid */
#define SO_XTITPSTATE   0x100a          /* set/get XTI state for endpoint */
#define SO_XTITPDFLT    0x100b          /* get char. of transport provider */
#define SO_XTIPEEKEVENT 0x100c          /* peek at event */
#define SO_XTISYNC      0x100d          /* synchronize XTI data structures */
#define SO_XTISEQNUM    0x100e          /* get seq. number from kernel */
#define SO_XTIACCEPTCHK 0x100f          /* does exan of t_accept params */
#define SO_XTICLREVENT  0x1010          /* clear event from queue */
#define SO_XTIENEVENT   0x1011          /* enable/disable events */
#define SO_XTIABORT     0x1012          /* find proper socket+abort connect. */
#define SO_XTITPPROTO   0x1013          /* get protocol family information */
#define SO_XTIUNBIND    0x1014          /* unbind socket */
#define SO_XTIREADEX    0x1015          /* read size of so_exrcv queue */
#endif /* XTI */

/*
 * Structure used for manipulating linger option.
 */
struct	linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#define	AF_UNIX		1		/* local to host (pipes, portals) */
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#define	AF_IMPLINK	3		/* arpanet imp addresses */
#define	AF_PUP		4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS	5		/* mit CHAOS protocols */
#define	AF_NS		6		/* XEROX NS protocols */
#define	AF_NBS		7		/* nbs protocols */
#define	AF_ECMA		8		/* european computer manufacturers */
#define	AF_DATAKIT	9		/* datakit protocols */
#define	AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	AF_SNA		11		/* IBM SNA */
#define AF_DECnet	12		/* DECnet */
#define AF_DLI		13		/* Direct data link interface */
#define AF_LAT		14		/* LAT */
#define	AF_HYLINK	15		/* NSC Hyperchannel */
#define AF_APPLETALK	16		/* Apple talk */
#define AF_BSC		17		/* BISYNC 2780/3780 */
#define AF_DSS		18		/* Distributed system services */
#define AF_OSI          19              /* OSI Protocols */
#define AF_NETMAN	20		/* Phase V network management */
#define AF_X25		21		/* X25 protocols */
#define AF_MAX		22

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	u_short	sa_family;		/* address family */
	char	sa_data[14];		/* up to 14 bytes of direct address */
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	u_short	sp_family;		/* address family */
	u_short	sp_protocol;		/* protocol */
};

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_UNIX		AF_UNIX
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NS		AF_NS
#define	PF_NBS		AF_NBS
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI		AF_DLI
#define PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define PF_APPLETALK	AF_APPLETALK
#define PF_BSC		AF_BSC
#define PF_DSS		AF_DSS
#define PF_OSI		AF_OSI
#define PF_NETMAN	AF_NETMAN
#define PF_X25		AF_X25
#define	PF_MAX		AF_MAX

/*
 * Maximum queue length specifiable by listen.
 */
#define	SOMAXCONN	8

/*
 * Message header for recvmsg and sendmsg calls.
 */
struct msghdr {
	caddr_t	msg_name;		/* optional address */
	int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
};

#define	MSG_OOB		0x1		/* process out-of-band data */
#define	MSG_PEEK	0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */
#ifdef XTI
#define MSG_MORE        0x8		/* allow xti TSDU (T_MORE) feature */
#endif /* XTI */

#define	MSG_MAXIOVLEN	16
