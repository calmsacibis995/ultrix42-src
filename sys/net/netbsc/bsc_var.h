/* static 	char	*sccsid = "@(#)bsc_var.h	4.2	(ULTRIX)		9/4/90";	*/
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
/*	bsc_var.h	1.0	85/03/20	U. Sinkewicz		*/
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif


/*
 * Kernel variables for bsc.
 */

/*
 * BSC control block, one per bsc message.  
 * Common structure pcb for the bisync protocol implementation.
 * Here are stored pointers to socket numbers, and pointers
 * up (to a socket structure).
 */

struct bscpcb {
	struct	bscpcb *bscp_next,*bscp_prev;
					/* pointers to other pcb's */
	struct	bscpcb *bscp_head;	/* pointer back to chain of bscpcb's
					   for this protocol */
	struct  sockaddr_bsc bscp_laddr;	/* local host table entry */
	struct	socket *bscp_socket;	/* back pointer to socket */
	char    bscp_modem[14];		/* kernal storage for phone number */
	caddr_t	bscp_ppcb;		/* pointer to per-protocol pcb */
	short	bscp_state;		/* state of this connection */
	short	b_timer;		/* timeout period */
	short   b_rexmt;		/* keep track of # of retransmits */
	char	b_iobc;			/* the out of band character */
	u_char	bscp_flags;
	char	bscp_oobflags;		/* have some out of band data- unused*/
};

#define	sotobscpcb(so)	((struct bscpcb *)(so)->so_pcb)

/* Used as the format indicator in the mtod macro	*/
struct  bsc_data{
	char	data[1024];
};

/* Not used - put statistics information here	*/
struct	bscstat {
	int	bscs_badsum;
	int	bscs_badoff;
	int	bscs_hdrops;
	int	bscs_badsegs;
	int	bscs_unack;
};

#if !defined(__vax)
#define ntohl(x)	(x)
#define ntohs(x)	(x)
#define htonl(x)	(x)
#define htons(x)	(x)
#endif

#ifdef KERNEL
struct	bscpcb bsb;		/* head of queue of active bscpcb's */
struct	bscstat bscstat;	/* bsc statistics */
struct	bsciphdr *bsc_template();
struct	bscpcb *bsc_close(), *bsc_drop();
struct	bscpcb *bsc_disconnect(), *bsc_usrclosed();
#endif

