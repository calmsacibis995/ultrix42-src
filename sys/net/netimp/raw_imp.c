#ifndef lint
static	char	*sccsid = "@(#)raw_imp.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)raw_imp.c	6.5 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netimp/if_imp.h"

/*
 * Raw interface to IMP.
 */

/*
 * Generate IMP leader and pass packet to impoutput.
 * The user must create a skeletal leader in order to
 * communicate message type, message subtype, etc.
 * We fill in holes where needed and verify parameters
 * supplied by user.
 */
rimp_output(m, so)
	register struct mbuf *m;
	struct socket *so;
{
	struct mbuf *n;
	int len, error = 0;
	register struct imp_leader *ip;
	register struct sockaddr_in *sin;
	register struct rawcb *rp = sotorawcb(so);
	struct in_ifaddr *ia;
	struct control_leader *cp;

	/*
	 * Verify user has supplied necessary space
	 * for the leader and check parameters in it.
	 */
	if ((m->m_off > MMAXOFF || m->m_len < sizeof(struct control_leader)) &&
	    (m = m_pullup(m, sizeof(struct control_leader))) == 0) {
		error = EMSGSIZE;	/* XXX */
		goto bad;
	}
	cp = mtod(m, struct control_leader *);
	if (cp->dl_mtype == IMPTYPE_DATA)
		if (m->m_len < sizeof(struct imp_leader) &&
		    (m = m_pullup(m, sizeof(struct imp_leader))) == 0) {
			error = EMSGSIZE;	/* XXX */
			goto bad;
		}
	ip = mtod(m, struct imp_leader *);
	if (ip->il_format != IMP_NFF) {
		error = EMSGSIZE;		/* XXX */
		goto bad;
	}
#ifdef notdef
	if (ip->il_link != IMPLINK_IP &&
	    (ip->il_link<IMPLINK_LOWEXPER || ip->il_link>IMPLINK_HIGHEXPER)) {
		error = EPERM;
		goto bad;
	}
#endif

	/*
	 * Fill in IMP leader -- impoutput refrains from rebuilding
	 * the leader when it sees the protocol family PF_IMPLINK.
	 * (message size calculated by walking through mbuf's)
	 */
	for (len = 0, n = m; n; n = n->m_next)
		len += n->m_len;
	ip->il_length = htons((u_short)(len << 3));
	sin = (struct sockaddr_in *)&rp->rcb_faddr;
	imp_addr_to_leader(ip, sin->sin_addr.s_addr);	/* BRL */
	/* no routing here */
	ia = in_iaonnetof(in_netof(sin->sin_addr));
	if (ia)
		return (impoutput(ia->ia_ifp, m, (struct sockaddr *)sin));
	error = ENETUNREACH;
bad:
	m_freem(m);
	return (error);
}
