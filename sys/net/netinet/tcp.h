/* static	char	*sccsid = "@(#)tcp.h	4.2	(ULTRIX)	9/4/90"; */

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
 *
 *      Michael G. Mc Menemy
 *              Add XTI support.    
 *  
 *	Larry Cohen  -  01/28/87
 *		Add new tcp option definitions: TCP_NODELAY, TCP_MAXSEG
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tcp.h	6.3 (Berkeley) 6/8/85
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define TCPLOOPBACK
typedef	u_long	tcp_seq;
/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct tcphdr {
	u_short	th_sport;		/* source port */
	u_short	th_dport;		/* destination port */
	tcp_seq	th_seq;			/* sequence number */
	tcp_seq	th_ack;			/* acknowledgement number */

#ifdef __vax
	u_char	th_x2:4,		/* (unused) */
		th_off:4;		/* data offset */
#endif /* __vax */

#ifdef __mips
#ifdef __MIPSEL
	u_char	th_x2:4,		/* (unused) */
		th_off:4;		/* data offset */
#endif /* __MIPSEL */
#ifdef __MIPSEB
	u_char	th_off:4,		/* data offset */
		th_x2:4;		/* (unused) */
#endif /* __MIPSEB */
#endif /* __mips */

	u_char	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
	u_short	th_win;			/* window */
	u_short	th_sum;			/* checksum */
	u_short	th_urp;			/* urgent pointer */
};

#define	TCPOPT_EOL	0
#define	TCPOPT_NOP	1
#define	TCPOPT_MAXSEG	2

/*
 *  Default maximum segment size for TCP.
 *  With an IP MSS of 576, this is 536,
 *  but 512 is probably more convenient.
 */
#ifdef	lint
#define	TCP_MSS	536
#else
#define	TCP_MSS	MIN(512, IP_MSS - sizeof (struct tcpiphdr))
#endif

/*
 * User-settable options (used with setsockopt).
 */
#define TCP_NODELAY     0x01    /* don't delay send to coalesce packets */
#define TCP_MAXSEG      0x02    /* set maximum segment size */
#ifdef XTI
#define TCP_ACCEPTMODE  0x12    /* set accept mode */
#define TCP_CONOPT      0x13	/* set/get connect options */
#define TCP_CONACCEPT   0x14    /* accept deferred connection */
#define TCP_DFLTQOS     0x15    /* get default quality of service
				   supported by local tranport provider */
#define TCP_CHKQOS      0x16    /* verify quality of service */
#define TCP_NEGQOS      0x17    /* negotiate quality of service */

#ifndef ACC_IMMED
#define ACC_IMMED       0       /* accept immediate */
#define ACC_DEFER       1       /* defer acceptance */
#endif
#endif /* XTI */

