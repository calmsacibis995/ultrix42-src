/* static	char	*sccsid = "@(#)if_imphost.h	4.2	(ULTRIX)	9/4/90"; */

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
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif


/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_imphost.h	6.3 (Berkeley) 6/8/85
 */

/*
 * Host structure used with IMP's.
 * Used to hold outgoing packets which
 * would exceed allowed RFNM count.
 *
 * These structures are packed into
 * mbuf's and kept as small as possible.
 */
struct host {
	struct	mbuf *h_q;		/* holding queue */
	struct	in_addr h_addr;		/* host's address */
	u_char	h_qcnt;          	/* size of holding q */
	u_char	h_timer;		/* used to stay off deletion */
	u_char	h_rfnm;			/* # outstanding rfnm's */
	u_char	h_flags;		/* see below */
};

/*
 * A host structure is kept around (even when there are no
 * references to it) for a spell to avoid constant reallocation
 * and also to reflect IMP status back to sites which aren't
 * directly connected to the IMP.  When structures are marked
 * free, a timer is started; when the timer expires the structure
 * is scavenged.
 */

#define	HF_INUSE	0x1
#define	HF_DEAD		(1<<IMPTYPE_HOSTDEAD)
#define	HF_UNREACH	(1<<IMPTYPE_HOSTUNREACH)

#define	HOSTTIMER	128		/* keep structure around awhile */

/*
 * Host structures, as seen inside an mbuf.
 * Hashing on the host address is used to
 * select an index into the first mbuf.  Collisions
 * are then resolved by searching successive
 * mbuf's at the same index.  Reclamation is done
 * automatically at the time a structure is free'd.
 */
#define	HPMBUF	((MLEN - sizeof(int)) / sizeof(struct host))
#if __vax
#define	HOSTHASH(a)	((((a).s_addr>>24)+(a).s_addr) % HPMBUF)
#endif

/*
 * In-line expansions for queuing operations on
 * host message holding queue.  Queue is maintained
 * as circular list with the head pointing to the
 * last message in the queue.
 */
#define	HOST_ENQUE(hp, m) { \
	register struct mbuf *n; \
	(hp)->h_qcnt++; \
	if ((n = (hp)->h_q) == 0) \
		(hp)->h_q = (m)->m_act = (m); \
	else { \
		(m)->m_act = n->m_act; \
		(hp)->h_q = n->m_act = (m); \
	} \
}
#define	HOST_DEQUE(hp, m) { \
	if ((m) = (hp)->h_q) { \
		if ((m)->m_act == (m)) \
			(hp)->h_q = 0; \
		else { \
			(m) = (m)->m_act; \
			(hp)->h_q->m_act = (m)->m_act; \
		} \
		(hp)->h_qcnt--; \
		(m)->m_act = 0; \
	} \
}

struct hmbuf {
	int	hm_count;		/* # of struct's in use */
	struct	host hm_hosts[HPMBUF];	/* data structures proper */
};

#ifdef KERNEL
struct host *hostlookup();
struct host *hostenter();
struct mbuf *hostdeque();
#endif
