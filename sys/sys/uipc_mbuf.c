#ifndef lint
static char *sccsid = "@(#)uipc_mbuf.c	4.3	(ULTRIX)	9/6/90";
#endif /* lint */

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
 *	R. Bhanukitsiri - 31-Jan-89					*
 *		Merge in VAX V3.0 changes:				*
 *		(Fred L. Templin - 21-Sep-88)				*
 *		Added check to m_pullup() to simply return if the	*
 *		caller already has enough in the mbuf. Also, removed	*
 *		defunct m_pullup_42() routine.				*
 *									*
 *	Larry Palmer - 15-Jan-87  				 	*
 *		Major changes for malloced mbufs/4.3 code		*
 *									*
 *	Larry Cohen - 08/05/86						*
 *		- make mclrefcnt a global pointer and allocate space 	*
 *		  in mbinit.   						*
 *		- the number pages reserved is now a function of 	*
 *			maxusers.					*
 *	Jeff Chase - 03/12/86						*
 *		Added routine mclgetx to create a type 2 mbuf		*
 *		Changed m_copy to handle type 2 mbufs			*
 *	Larry Cohen  -  02/07/86					*
 *		in mcopy use M_DONTWAIT because mcopy can be called	*
 *		from an interrupt routine.				*
 *	R. Rodriguez -  11/11/85					*
 *		Add 43bsd beta tape fixes!				*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes  				*
 *									*
 ************************************************************************/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/mbuf.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/kmalloc.h"

caddr_t
m_clalloc(ncl, how, canwait)	/* must be called at splimp since rmallocing */
	register int ncl;
	register int how;
	int canwait;
{
	register int npg, mbx;
	register struct mbuf *m;
	register int i;

	npg = ncl * CLSIZE;
	if(how == MPG_CLUSTERS) {
		KM_ALLOC(m,struct mbuf *,2*npg*NBPG,KM_CLUSTER,KM_NOWAIT|KM_CALL);
	} else {
		KM_ALLOC(m,struct mbuf *,npg*NBPG,KM_DEVBUF,KM_NOWAIT|KM_CALL);
	}
	if(m == NULL)
		return(0);
	if(how == MPG_CLUSTERS) {
		if(ncl != 1)
			cprintf("m_clalloc for %d clusters invalid\n", ncl);
	}
	return ((caddr_t)m);
}

m_pgfree(addr, n)
	caddr_t addr;
	int n;
{
#ifdef lint
	addr = addr; n = n;
#endif
}

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */
struct mbuf *
m_get(canwait, type)
	register int canwait, type;
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	return (m);
}

struct mbuf *
m_getclr(canwait, type)
	register int canwait, type;
{
	register struct mbuf *m;

	MGET(m,canwait,type);
	if (m == 0)
		return (0);
	bzero(mtod(m, caddr_t), MLEN);
	return (m);
}

struct mbuf *
m_free(m)
	register struct mbuf *m;
{
	register struct mbuf *n;

	MFREE(m, n);
	return (n);
}

m_freem(m)
	register struct mbuf *m;
{
	register struct mbuf *n;
	register int s;

	if (m == NULL)
		return;
	s = splimp();
	do {
		MFREE(m, n);
	} while (m = n);
	splx(s);
}

/*
 * Mbuffer utility routines.
 */
struct mbuf *
m_copy(m, off, len)
	register struct mbuf *m;
	register int off;
	register int len;
{
	register struct mbuf *n, **np;
	struct mbuf *top;

	if (len == 0)
		return (0);
	if (off < 0 || len < 0)
		panic("m_copy1");
	while (off > 0) {
		if (m == 0)
			panic("m_copy2");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copy3");
			break;
		}
		MGET(n, M_DONTWAIT, m->m_type);
		*np = n;
		if (n == 0)
			goto nospace;
		n->m_len = MIN(len, m->m_len - off);
		if (m->m_off > MMAXOFF && n->m_len > MLEN) {
			mcldup(m, n, off);
			n->m_off += off;
		} else
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
			    (unsigned)n->m_len);
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	return (top);
nospace:
	m_freem(top);
	return (0);
}

m_cat(m, n)
	register struct mbuf *m, *n;
{
	while (m->m_next) {
		m = m->m_next;
	}
	while (n) {
		if (m->m_off >= MMAXOFF ||
		    m->m_off + m->m_len + n->m_len > MMAXOFF) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

m_adj(mp, len)
	register struct mbuf *mp;
	register int len;
{
	register struct mbuf *m;
	register int count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_off += len;
				break;
			}
		}
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			return;
		}
		count -= len;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		for (m = mp; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while (m = m->m_next)
			m->m_len = 0;
	}
}
/* 
 * m_pullup() pulls AT LEAST "len" bytes up into the mbuf at the head of
 * the chain. The mbuf at the head of the returned chain will ALWAYS be
 * a non-cluster mbuf.
 */
#define MPULL_EXTRA 32
struct mbuf *
m_pullup(n, len)
	register struct mbuf *n;
	register int len;
{
	register struct mbuf *m;
	register int count;
	int space;

	if (n->m_off + len <= MMAXOFF && n->m_next) {
		/*
		 * If the caller already has enough data in the mbuf,
		 * return. We check here INSTEAD of above, since other
		 * code assumes that m_pullup will be returning a small
		 * mbuf at the head of the chain. (Namely, ipintr calls
		 * m_pullup(), then ip_forward calls "dtom()").
		 */
		if (n->m_len >= len)
			return (n);
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
	}
	space = MMAXOFF - m->m_off;
	do {
		count = MIN(MIN(space - m->m_len, len + MPULL_EXTRA),n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t)+m->m_len,
		  (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		if (n->m_len)
			n->m_off += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return (0);
}

struct mbuf *
mclgetx(fun, arg, addr, len, wait)
	int (*fun)(), arg, len, wait;
	caddr_t addr;
{
	register struct mbuf *m;

	MGET(m, wait, MT_DATA);
	if (m == 0)
		return (0);
	m->m_off = (int)addr;
	m->m_len = len;
	m->m_cltype = M_CLTYPE2;
	m->m_clfun = fun;
	m->m_clarg = arg;
	m->m_clswp = NULL;
	return (m);
}

static
buffree(arg)
	int arg;
{
	KM_FREE(arg, KM_NFS);
}

mcldup(m, n, off)
	register struct mbuf *m, *n;
	int off;
{
	register caddr_t copybuf;
	int s;

	switch (m->m_cltype) {
	case M_CLTYPE1:
		n->m_off = m->m_off;
		n->m_cltype = M_CLTYPE1;
		n->m_clptr = m->m_clptr;
		KM_MEMDUP(m->m_clptr);
		break;
	case M_CLTYPE2:
		KM_ALLOC(copybuf, caddr_t, n->m_len, KM_NFS, KM_NOWAIT|KM_CALL);
		bcopy(mtod(m, caddr_t) + off, copybuf,n->m_len);
		n->m_off = (int)copybuf - off;
		n->m_cltype = M_CLTYPE2;
		n->m_clfun = buffree;
		n->m_clarg = (int)copybuf;
		n->m_clswp = NULL;
		break;
	default:
		panic("mcldup has bad m_cltype");
	}
}

m_length(m)
	register struct mbuf *m;
{
	register int length = 0;

	if (m) do {
		length += m->m_len;
	} while (m = m->m_next);
	return length;
}

m_copydata(m, buf, length)
	register struct mbuf *m;
	register caddr_t buf;
	register int length;
{
	do {
		register int count = min(length, m->m_len);
		bcopy(mtod(m, caddr_t), buf, count);
		length -= count;
		buf += count;
		m = m->m_next;
	} while (length > 0);
}
