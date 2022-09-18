#ifndef lint
static	char	*sccsid = "@(#)uipc_socket2.c	4.3	(ULTRIX)	11/14/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88,89 by			*
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

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	uipc_socket2.c	6.10 (Berkeley) 6/8/85
 */

/************************************************************************
 *			Modification History				*
 *									*
 *	06 Feb 90 -- R. Bhanukitsiri					*
 *		Copy the hiwat too in sonewconn().			*
 *									*
 *	28 Dec 89 -- ejf						*
 *		Check sizeof of address in sbappendanyaddr().		*
 *									*
 *	05 Aug 89 -- jsd						*
 *		Add KM_ALLOC error check in sonewconn			*
 *									*
 *	9-June-89 -- U. Sinkewicz					*
 *		Added SO_LOCK and so->ref which control socket access	*
 *		while the socket is unlocked for either the heierarchy	*
 *		or for sleeping.					*
 *									*
 *	05 May 89 -- Michael G. Mc Menemy      				*
 *		Add support for XTI.            			*
 *									*
 *	23 Aug 88 -- miche						*
 *		Add support for ref'ing a process.			*
 *									*
 *	04/10/87 -- Suzanne Logcher					*
 *		Merged in Larry Palmer's changes			*
 *									*
 *	04/15/86 -- John Forecast					*
 *		Add support for system processes			*
 *		Use SB_SEL to indicate that a socket is selected	*
 *									*
 *	02/07/86 -- Larry Cohen						*
 *		sbflush/sbdrop now handle zero length mbufs correctly	*
 *		- prevents the "sbflush 2" panic			*
 *									*
 *	09/24/85 -- Larry Cohen						*
 *		From DECnet-add sbappendanyaddr, a more generic version *	
 *			of sbappendaddr.				*  	
 ************************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/gnode.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/kmalloc.h"

/*
 * Primitive routines for operating on sockets and socket buffers
 */

/*
 * Procedures to manipulate state flags of socket
 * and do appropriate wakeups.  Normal sequence from the
 * active (originating) side is that soisconnecting() is
 * called during processing of connect() call,
 * resulting in an eventual call to soisconnected() if/when the
 * connection is established.  When the connection is torn down
 * soisdisconnecting() is called during processing of disconnect() call,
 * and soisdisconnected() is called when the connection to the peer
 * is totally severed.  The semantics of these routines are such that
 * connectionless protocols can call soisconnected() and soisdisconnected()
 * only, bypassing the in-progress calls when setting up a ``connection''
 * takes no time.
 *
 * From the passive side, a socket is created with
 * two queues of sockets: so_q0 for connections in progress
 * and so_q for connections already made and awaiting user acceptance.
 * As a protocol is preparing incoming connections, it creates a socket
 * structure queued on so_q0 by calling sonewconn().  When the connection
 * is established, soisconnected() is called, and transfers the
 * socket structure to so_q, making it available to accept().
 * 
 * If a socket is closed with sockets on either
 * so_q0 or so_q, these sockets are dropped.
 *
 * If higher level protocols are implemented in
 * the kernel, the wakeups done here will sometimes
 * cause software-interrupt process scheduling.
 */

soisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;
	wakeup((caddr_t)&so->so_timeo);
}

soisconnected(so)
	register struct socket *so;
{
	register struct socket *head = so->so_head;

	if (head) {
		SO_LOCK(head);
		if (soqremque(so, 0) == 0)
			panic("soisconnected");
		soqinsque(head, so, 1);
		sorwakeup(head);
		wakeup((caddr_t)&head->so_timeo);
		smp_unlock(&head->lk_socket);
	}
	so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTED;
	wakeup((caddr_t)&so->so_timeo);
	sorwakeup(so);
	sowwakeup(so);
}

soisdisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~SS_ISCONNECTING;
	so->so_state |= (SS_ISDISCONNECTING|SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

soisdisconnected(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

/*
 * When an attempt at a new connection is noted on a socket
 * which accepts connections, sonewconn is called.  If the
 * connection is possible (subject to space constraints, etc.)
 * then we allocate a new structure, propoerly linked into the
 * data structure of the original socket, and return this.
 */
struct socket *
sonewconn(head)
	register struct socket *head;
{
	register struct socket *so;
	register struct solock *solk;
	int s; /* SMP */
#ifdef XTI
	static int xti_seq_num;
#endif XTI

	if (head->so_qlen + head->so_q0len >= head->so_qlimit)
		goto bad;

	KM_ALLOC(so, struct socket *, sizeof(struct socket), KM_SOCKET, KM_CLEAR|KM_NOWAIT);
	if(so == NULL)
		goto bad;

	so->so_type = head->so_type;
	so->so_options = head->so_options &~ SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgrp = head->so_pgrp;
#ifdef XTI
        so->so_exrcv.sb_hiwat = head->so_exrcv.sb_hiwat;
	so->so_xticb = head->so_xticb;

	if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
	  xti_seq_num++;
	  so->so_xticb.xti_seqnum = xti_seq_num;

	}
#endif XTI
	s = splnet();
	lockinit(&so->lk_socket, &lock_socket_d); 
	SO_LOCK(so);
	soqinsque(head, so, 0);
	if ((*so->so_proto->pr_usrreq)(so, PRU_ATTACH,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0)) {
		if (so->so_head)
			(void) soqremque(so, 0);
		smp_unlock(&so->lk_socket);
		KM_FREE(so, KM_SOCKET);
		splx(s); /* SMP */
		goto bad;
	}
	smp_unlock(&so->lk_socket);
	splx(s);
#ifdef XTI
	(void) soxticopy_tp_specifc(head,so);
#endif XTI
	return (so);
bad:
	return ((struct socket *)0);
}

#ifdef XTI
soxtiqinsque(head, so)
	register struct socket *head, *so;
{
	register struct socket *next;

	/* 
	 * FIFO code
	 */

	next = head;

	for (;;) {

	  if (next->so_xticb.xti_q_flink == 0) {
	    next->so_xticb.xti_q_flink = so;
	    so->so_xticb.xti_q_flink = 0;
	    so->so_xticb.xti_q_blink = next;
	    break;
	  }
	  else
	    next = next->so_xticb.xti_q_flink;
	}
	
}

soxtiqremque(so, torem)
        register struct socket *so;
        struct socket *torem;
{
	register struct socket *head, *prev, *next;
  

	/*scan through queue looking for a torem match */

	head = so;
	prev = head;
	for (;;) {
	  next =  prev->so_xticb.xti_q_flink;
	  if (next == torem)
	    break;
	  if (next == head || next == 0)
	    return (0);
	  prev = next;
	}
  
	prev->so_xticb.xti_q_flink  = next->so_xticb.xti_q_flink;

	/*
	 * if the entry is not the last one in the chain, set the backlink for the
	 * next entry to the previous entry. (because this entry is history)
	 */

	if (next->so_xticb.xti_q_flink)
	  ( (struct socket *) next->so_xticb.xti_q_flink)->so_xticb.xti_q_blink = prev;

	return (1);
}
#endif XTI

soqinsque(head, so, q)
	register struct socket *head, *so;
	int q;
{

	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;
		so->so_q0 = head->so_q0;
		head->so_q0 = so;
	} else {
		head->so_qlen++;
		so->so_q = head->so_q;
		head->so_q = so;
	}
}

soqremque(so, q)
	register struct socket *so;
	int q;
{
	register struct socket *head, *prev, *next;

	head = so->so_head;
	prev = head;
	for (;;) {
		next = q ? prev->so_q : prev->so_q0;
		if (next == so)
			break;
		if (next == head)
			return (0);
		prev = next;
	}
	if (q == 0) {
		prev->so_q0 = next->so_q0;
		head->so_q0len--;
	} else {
		prev->so_q = next->so_q;
		head->so_qlen--;
	}
	next->so_q0 = next->so_q = 0;
	next->so_head = 0;
	return (1);
}

/*
 * Socantsendmore indicates that no more data will be sent on the
 * socket; it would normally be applied to a socket when the user
 * informs the system that no more data is to be sent, by the protocol
 * code (in case PRU_SHUTDOWN).  Socantrcvmore indicates that no more data
 * will be received, and will normally be applied to the socket by a
 * protocol when it detects that the peer will send no more data.
 * Data queued for reading in the socket may yet be read.
 */

socantsendmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTSENDMORE;
	sowwakeup(so);
}

socantrcvmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTRCVMORE;
	sorwakeup(so);
}

/*
 * Socket select/wakeup routines.
 */

/*
 * Queue a process for a select on a socket buffer.
 */
sbselqueue(sb)
	struct sockbuf *sb;
{
	register struct proc *p;

	if ((p = sb->sb_sel) && p->p_wchan == (caddr_t)&selwait)
		sb->sb_flags |= SB_COLL;
	else
		sb->sb_sel = u.u_procp;
	sb->sb_flags |= SB_SEL;
}

/*
 * Wait for data to arrive at/drain from a socket buffer.
 */
/* 
 * We should be using only sbwait2.
 */
/* #ifdef notdef */
sbwait(sb)
	struct sockbuf *sb;
{

	sb->sb_flags |= SB_WAIT;
	sleep((caddr_t)&sb->sb_cc, PZERO+1);
}
/* #endif */

/*
 * Wakeup processes waiting on a socket buffer.
 */
sbwakeup(sb)
	register struct sockbuf *sb;
{

	if (sb->sb_flags & SB_SEL) {
		selwakeup(sb->sb_sel, sb->sb_flags & SB_COLL);
		sb->sb_sel = 0;
		sb->sb_flags &= ~(SB_COLL|SB_SEL);
	}
	if (sb->sb_flags & SB_WAIT) {
		sb->sb_flags &= ~SB_WAIT;
		wakeup((caddr_t)&sb->sb_cc);
	}
}

/*
 * Wakeup socket readers and writers.
 * Do asynchronous notification via SIGIO
 * if the socket has the SS_ASYNC flag set.
 */
sowakeup(so, sb)
	register struct socket *so;
	struct sockbuf *sb;
{
	register struct proc *p;

	sbwakeup(sb);
	if (so->so_state & SS_ASYNC) {
		if (so->so_pgrp < 0)
			gsignal(-so->so_pgrp, SIGIO);
		else {
			if ((so->so_pgrp > 0) && (p = proc_get(so->so_pgrp))) {
				psignal(p, SIGIO);
				proc_rele(p);
			}
		}
	}
}

/*
 * Socket buffer (struct sockbuf) utility routines.
 *
 * Each socket contains two socket buffers: one for sending data and
 * one for receiving data.  Each buffer contains a queue of mbufs,
 * information about the number of mbufs and amount of data in the
 * queue, and other fields allowing select() statements and notification
 * on data availability to be implemented.
 *
 * Data stored in a socket buffer is maintained as a list of records.
 * Each record is a list of mbufs chained together with the m_next
 * field.  Records are chained together with the m_act field. The upper
 * level routine soreceive() expects the following conventions to be
 * observed when placing information in the receive buffer:
 *
 * 1. If the protocol requires each message be preceded by the sender's
 *    name, then a record containing that name must be present before
 *    any associated data (mbuf's must be of type MT_SONAME).
 * 2. If the protocol supports the exchange of ``access rights'' (really
 *    just additional data associated with the message), and there are
 *    ``rights'' to be received, then a record containing this data
 *    should be present (mbuf's must be of type MT_RIGHTS).
 * 3. If a name or rights record exists, then it must be followed by
 *    a data record, perhaps of zero length.
 *
 * Before using a new socket structure it is first necessary to reserve
 * buffer space to the socket, by calling sbreserve().  This commits
 * some of the available buffer space in the system buffer pool for the
 * socket.  The space should be released by calling sbrelease() when the
 * socket is destroyed.
 *
 * The routines sbappend() or sbappendrecord() are normally called to
 * append new mbufs to a socket buffer, after checking that adequate
 * space is available, comparing the function sbspace() with the amount
 * of data to be added.  sbappendrecord() differs from sbappend() in
 * that data supplied is treated as the beginning of a new record.
 * Data is normally removed from a socket buffer in a protocol by
 * first calling m_copy on the socket buffer mbuf chain and sending this
 * to a peer, and then removing the data from the socket buffer with
 * sbdrop() or sbdroprecord() when the data is acknowledged by the peer
 * (or immediately in the case of unreliable protocols.)
 *
 * To place a sender's name, optionally, access rights, and data in a
 * socket buffer sbappendaddr() should be used.  To place access rights
 * and data in a socket buffer sbappendrights() should be used.  Note
 * that unlike sbappend() and sbappendrecord(), these routines check
 * for the caller that there will be enough space to store the data.
 * Each fails if there is not enough space, or if it cannot find mbufs
 * to store additional information in.
 */

soreserve(so, sndcc, rcvcc)
	register struct socket *so;
	int sndcc, rcvcc;
{

	if (sbreserve(&so->so_snd, sndcc) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, rcvcc) == 0)
		goto bad2;
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	return (ENOBUFS);
}

#ifdef XTI
soexreserve(so, sndcc, rcvcc)
        register struct socket *so;
        int sndcc, rcvcc;
{

  /*
   * send expedited space currently not defines in socket area 
  
   if (sbreserve(&so->so_exsnd, sndcc) == 0)
   goto bad;

   *
   */

  if (sbreserve(&so->so_exrcv, rcvcc) == 0)
    goto bad2;
  return (0);
bad2:
/* when send expedited is implemented
  sbrelease(&so->so_exsnd);
*/
bad:
  return (ENOBUFS);
}
#endif XTI

/*
 * Allot mbufs to a sockbuf.
 */
sbreserve(sb, cc)
	struct sockbuf *sb;
{

	if ((unsigned)cc > SB_MAX)
		return (0);
	/* someday maybe this routine will fail... */
	sb->sb_hiwat = cc;

	/* * 2 implies names can be no more than 1 mbuf each */

	/* I dont understand the above statement fully but 2 seems
	 *  to be too small when sending small packets.  I increased
	 *  it to 4.
	 */

	sb->sb_mbmax = MIN(cc * 4, SB_MAX);
	return (1);
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */
sbrelease(sb)
	struct sockbuf *sb;
{

	sbflush(sb);
	sb->sb_hiwat = sb->sb_mbmax = 0;
}

/*
 * Routines to add and remove
 * data from an mbuf queue.
 */

/*
 * Append mbuf chain m to the last record in the
 * socket buffer sb.  The additional space associated
 * the mbuf chain is recorded in sb.  Empty mbufs are
 * discarded and mbufs are compacted where possible.
 */
sbappend(sb, m)
	struct sockbuf *sb;
	struct mbuf *m;
{
	register struct mbuf *n;

	if (m == 0)
		return;
	if (n = sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		while (n->m_next)
			n = n->m_next;
	}
	sbcompress(sb, m, n);
}

/*
 * As above, except the mbuf chain
 * begins a new record.
 */
sbappendrecord(sb, m0)
	register struct sockbuf *sb;
	register struct mbuf *m0;
{
	register struct mbuf *m;

	if (m0 == 0)
		return;
	if (m = sb->sb_mb)
		while (m->m_act)
			m = m->m_act;
	/*
	 * Put the first mbuf on the queue.
	 * Note this permits zero length records.
	 */
	sballoc(sb, m0);
	if (m)
		m->m_act = m0;
	else
		sb->sb_mb = m0;
	m = m0->m_next;
	m0->m_next = 0;
	sbcompress(sb, m, m0);
}

/*
 * Append address and data, and optionally, rights
 * to the receive queue of a socket.  Return 0 if
 * no space in sockbuf or insufficient mbufs.
 */
sbappendaddr(sb, asa, m0, rights0)		/* XXX */
	register struct sockbuf *sb;
	struct sockaddr *asa;
	struct mbuf *rights0, *m0;
{
	return(sbappendanyaddr(sb, asa, sizeof(*asa), m0, rights0));
}


sbappendanyaddr(sb, asa, asalen, m0, rights0)		/* XXX */
	register struct sockbuf *sb;
	u_char *asa;
	int asalen;
	struct mbuf *rights0, *m0;
{
	register struct mbuf *m, *n;
	int space = asalen;

	for (m = m0; m; m = m->m_next)
		space += m->m_len;
	if (rights0)
		space += rights0->m_len;
	if (space > sbspace(sb)) {
		return (0);
	}
	m = m_get(M_DONTWAIT, MT_SONAME);
	if (m == 0) {
		cprintf("cant get mbufs\n");
		return (0);
	}
	if ( asalen > m->m_len ) {
		struct mbuf *page;
		MCLGET(m,page);
		if ( page == 0 ) {
			cprintf("cant get mcluster\n");
			m_free(m);
			return (0);
		}
	}
	bcopy(asa, mtod(m, u_char *), (m->m_len = asalen));
	if (rights0 && rights0->m_len) {
		m->m_next = m_copy(rights0, 0, rights0->m_len);
		if (m->m_next == 0) {
			m_freem(m);
			return (0);
		}
		sballoc(sb, m->m_next);
	}
	sballoc(sb, m);
	if (n = sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		n->m_act = m;
	} else
		sb->sb_mb = m;
	if (m->m_next)
		m = m->m_next;
	if (m0)
		sbcompress(sb, m0, m);
	return (1);
}

sbappendrights(sb, m0, rights)
	struct sockbuf *sb;
	struct mbuf *rights, *m0;
{
	register struct mbuf *m, *n;
	int space = 0;

	if(rights == 0)
		panic("sbappendrights");
	for(m = m0; m; m = m->m_next)
		space += m->m_len;
	space += rights->m_len;
	if (space > sbspace(sb))
		return (0);
	m = m_copy(rights, 0, rights->m_len);
	if (m == 0)
		return (0);
	sballoc(sb, m);
	if (n = sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		n->m_act = m;
	} else
		sb->sb_mb = m;
	if (m0)
		sbcompress(sb, m0, m);
	return (1);
}

/*
 * Compress mbuf chain m into the socket
 * buffer sb following mbuf n.  If n
 * is null, the buffer is presumed empty.
 */
sbcompress(sb, m, n)
	register struct sockbuf *sb;
	register struct mbuf *m, *n;
{

	while (m) {
		if (m->m_len == 0) {
			m = m_free(m);
			continue;
		}
		if (n && n->m_off <= MMAXOFF && 
			m->m_off <= MMAXOFF &&
		    (n->m_off + n->m_len + m->m_len) <= MMAXOFF &&
			n->m_type == m->m_type) {
			bcopy(mtod(m, caddr_t), mtod(n, caddr_t) + n->m_len,
			    (unsigned)m->m_len);
			n->m_len += m->m_len;
			sb->sb_cc += m->m_len;
			m = m_free(m);
			continue;
		}
		sballoc(sb, m);
		if (n)
			n->m_next = m;
		else
			sb->sb_mb = m;
		n = m;
		m = m->m_next;
		n->m_next = 0;
	}
}

/*
 * Free all mbufs in a sockbuf.
 * Check that all resources are reclaimed.
 */
sbflush(sb)
	register struct sockbuf *sb;
{

	if (sb->sb_flags & SB_LOCK)
		panic("sbflush");
	while (sb->sb_mbcnt) 
		sbdrop(sb, (int)sb->sb_cc);
	if (sb->sb_cc || sb->sb_mbcnt || sb->sb_mb)
		panic("sbflush 2");
}

/*
 * Drop data from (the front of) a sockbuf.
 */
struct mbuf *
sbdrop(sb, len)
	register struct sockbuf *sb;
	register int len;
{
	register struct mbuf *m, *mn;
	struct mbuf *next;

	next = (m = sb->sb_mb) ? m->m_act : 0;
	while (len > 0) {
		if (m == 0) {
			if (next == 0)
				panic("sbdrop");
			m = next;
			next = m->m_act;
			continue;
		}
		if (m->m_len > len) {
			m->m_len -= len;
			m->m_off += len;
			sb->sb_cc -= len;
			break;
		}
		len -= m->m_len;
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	while (m && m->m_len == 0) {
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	if (m) {
		sb->sb_mb = m;
		m->m_act = next;
	} else
		sb->sb_mb = next;
	return (sb->sb_mb);
}

/*
 * Drop a record off the front of a sockbuf
 * and move the next record to the front.
 */
struct mbuf *
sbdroprecord(sb)
	register struct sockbuf *sb;
{
	register struct mbuf *m, *mn;

	m = sb->sb_mb;
	if (m) {
		sb->sb_mb = m->m_act;
		do {
			sbfree(sb, m);
			MFREE(m, mn);
		} while (m = mn);
	}
	return (sb->sb_mb);
}
