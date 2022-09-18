#ifndef lint
static char *sccsid = "@(#)uipc_socket.c	4.11	(ULTRIX)	4/4/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
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
 *	03-Apr-91 -- Heather Gray
 *	Fix xti/osi buffer handling.
 *
 *	02-Apr-91 -- lp
 *	Fix performance problem when sending large buffers.
 *
 *      07-Mar-91 -- Michael G. Mc Menemy
 *      Fixed smp lock position messup.
 *
 *      07-Mar-91 -- Michael G. Mc Menemy
 *      Fixed oob handling code. 
 *
 *	28-Feb-91 -- prs
 *	Added support for a configurable number of
 *	open file descriptors.
 *
 *      27-Feb-91 -- Michael G. Mc Menemy
 *      Fix memory leak in soclose().
 *
 *	10-Aug-90 -- H. Gray
 *	Close dup'd from endpoint in XTIACCEPTCHK of sosetopt() to
 *	free descriptor for further use.
 *
 *	11-Jun-90 -- Larry Palmer
 *	Change buffer allocation on mips processors in sosend. This
 *	allows 4K buffers to be sent to protocol layers (fddi).
 *
 *	16-May-90 -- R. Bhanukitsiri
 *	Fix race condition between soclose() and sofree() (courtesy
 *	of Jim Woodward).
 *
 *	2-Jan-90  -- U. Sinkewicz
 *	Performance enhancements to uniprocessor kernel.
 *
 *	12-Dec-89 -- R. Bhanukitsiri
 *	Fix SMP bug that causes the system to hang when XTI t_accept
 *	specifies the same transport endpoint.
 *
 *	11-Dec-89 -- jsd
 *	wait for SS_ISCONNECTED if in SS_ISCONNECTING state in sosend
 *
 *	09-Nov-89 -- jaw
 *	allow spin locks to be compiled out
 *
 *	18 Jul 89 -- R. Bhanukitsiri
 *	Make sure shutdown() on unconnected socket returns ENOTCONN.
 *	Make sure after shutdown that disallowed receives, a recv()
 *	would return ENOTCONN.
 *
 *	24 May 89 -- U. Sinkewicz
 *	Added SO_LOCK macros to fix the smp problem of unlocking the 
 *	socket then locking it again.  The problem is that the lock
 *	might act on either a freed or a changed socket.
 *				
 *      05 May 89 -- Michael G. Mc Menemy
 *      Add XTI support.
 *
 * 	28 Feb 89 -- U. Sinkewicz
 *	SMP/mips merge.  (Added R. Bhanukitsiri changes from 1/31/89)
 *
 * 	02 Nov 88 -- jaw
 *	minor fix to socket code to unlock rcv before unlocking the
 * 	socket.
 *
 * 	23 Aug 88 -- miche						*
 *	Add support for ref'ing a process.				*
 *									*
 *      28-Jan-88  -us
 *      Removed references to cusysproc.
 *	
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes. Use new memory allocation
 *	scheme for mbufs.
 *
 *	Larry Palmer - 03/03/87				 		*
 *	Fix recvfrom + MSG_PEEK panic. Fix deleted lines for 		*
 *	decnet zero length recieves.			 		*
 * 									*
 *	Larry Palmer - 01/29/87						*
 *	Added sodisconnect to some connect calls (for nameserver)	*
 *									*
 *	John Forecast - 04/15/86					*
 *		Add support for system processes			*
 *		Make sure errors are corectly returned			*
 *									*
 *	Jeff Chase - 03/12/86						*
 *		Changed sogetopt and sosend for new MCLGET macro	*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes  				*
 *									*
 *	Larry Cohen  -  10/02/85					*
 *		make getsockopt() backwards compatable			*
 *									*
 *	Larry Cohen  -  10/03/85					*
 *		zero length reads on connected sockets are ok (return 0)*
 *		zero length reads on disconnected sockets return 	*
 *			EWOULDBLOCK					*
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uipc_socket.c	6.15 (Berkeley) 6/8/85
 */

#ifndef XTI
#define XTI
#endif XTI

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#ifdef vax
#include "../h/sysproc.h"
#endif 
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/smp_lock.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/un.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/net/if.h"
#include "../h/kmalloc.h"

/* 
 * prurequests used to be defined in ../h/protosw.h but caused multiple
 * define problems.  It should not be defined in a header file. 
 * The external definition for prurequests is now in ../h/protosw.h
 * The definition follows.  I could not find a better place for it.
 */

extern int smp;
int wakeup();

#ifdef XTI
int xti_debug = 0; /* XTI debug variable */
#define PRINTXTID(level, msg)   \
  /*                            \
   * level:                     \
   * 0x01 = generate events;    \
   * 0x04 = acceptchk/abort support;  \
   * 0x08 = peek events;        \
   * 0x10 = tpdu T_MORE support; \
   * 0x20 = oob mark;           \
   * 0x40 = options neg.        \
   * 0x80 = protocol;           \
   */                           \
  if (xti_debug & (level))      \
     cprintf((msg))
#else
#define PRINTXTID(level, msg)
#endif XTI

char *prurequests[] = PRU_REQLIST;

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 *
 * TODO:
 *	test socketpair
 *	clean up async
 *	out-of-band is a kludge
 */
/*
 * SMP:
 * socket()
 *    socreate()
 *	elevate ipl
 *	lock()
 *	    update so fields
 *	    usrreq()
 *	unlock()
 *	splx()
 * 
 * Called from socket(), no smp locks held coming in.
 */
/*ARGSUSED*/

socreate(dom, aso, type, proto)
	struct socket **aso;
	register int type;
	int proto;
{
	register struct protosw *prp;
	register struct socket *so;
	register int error;
	int s;

	if (proto)
		prp = pffindproto(dom, proto, type);
	else
		prp = pffindtype(dom, type);
	if (prp == 0 || prp->pr_usrreq == 0)
		return (EPROTONOSUPPORT);
	if (prp->pr_type != type)
		return (EPROTOTYPE);
	KM_ALLOC(so, struct socket *, sizeof(struct socket), KM_SOCKET, KM_CLEAR);
	/* Initialize the lock */
	lockinit(&so->lk_socket, &lock_socket_d);
	s = splnet(); /* SMP */
	SO_LOCK(so);
	so->ref = 0;
	so->so_type = type;
	if (suser())
		so->so_state = SS_PRIV;
	so->so_proto = prp;

	/* Enter usrreq() with smp lock held; */

	error =
	    (*prp->pr_usrreq)(so, PRU_ATTACH,
		(struct mbuf *)0, (struct mbuf *)proto, (struct mbuf *)0);
	/* 4.13.89.us  You need the owner check because DECNET
	 * releases the socket lock to accomodate the heirarchy.
	 * There is a very small possibility that the socket will disappear
	 * while unlocked.  DECNET will detect the problem and 
	 * return without a socket lock.  TCP/IP uses the so->ref field as
	 * a busy indicator.  When that field is set, the socket cannot be
 	 * locked via SO_LOCK.  SO_LOCK eleminates possibility of locking
	 * an invalid socket.  
	 */
	
	if (smp){
	  if (!(smp_owner(&so->lk_socket))){
		if ( !(error) ){
		  mprintf("socreate: error not returned from protocols\n");
		  error = ENXIO;
		}
		splx(s);
		return(error);
	  }
	}
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		splx(s);
		return (error);
	}
	*aso = so;
	smp_unlock(&so->lk_socket);
	splx(s);
	return (0);
}

/*
 * SMP:
 * bind()
 *	getsock()
 *	sockargs()
 *	sobind()
 *	    splnet
 *	    lock()
 *		usrreq -> unlock
 *		    gfs_namei (sleep)
 *		    lock
 *		    unlock
 *		    gfs...
 *		    lock
 *	    unlock
 *	    splx	
 * 
 * No smp locks held coming in.  At priority < splnet.
 */
sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	SO_LOCK(so);
	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);
	if (smp){
	  if (!(smp_owner(&so->lk_socket))){
		if ( !(error) ){
		   mprintf(" sobind: error not returned from protocols\n");
		   error = ENXIO;
		}
		splx(s);
		return(error);
	  }
	}
	smp_unlock(&so->lk_socket);
	splx(s);
	return (error);
}

/* 
 * SMP:
 * listen()
 * 	getsock()
 * 	solisten()
 *		splnet
 *		lock
 *		usrreq
 *		unlock
 *		splx
 *
 * No smp locks held coming in.
 */
int somaxconn = SOMAXCONN;

solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int s, error;

	s = splnet();
	SO_LOCK(so);
	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (smp){
	  if( !(smp_owner(&so->lk_socket))){
		if ( !(error) ){
		   mprintf(" solisten: error not returned from protocols\n");
		   error = ENXIO;
		}
		splx(s);
		return(error);
	  }
	}
	if (error) {
		smp_unlock(&so->lk_socket);
		splx(s);
		return (error);
	}
	if (so->so_q == 0) {
		so->so_q = so;
		so->so_q0 = so;
		so->so_options |= SO_ACCEPTCONN;
	}
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = MIN(backlog, somaxconn);
	smp_unlock(&so->lk_socket);
	splx(s);
	return (0);
}

/*
 * SMP:
 * Called with smp lock held.  Called at splnet.  Conditionally
 * relinquishes the lock. 
 * Sofree is called by:
       bsc_usrreq, bsc_close, bsc_pcbdetach,
       copen [gfs_syscalls.c],
       in_pcbdetach, pipe, raw_usrreq,
       raw_detach, soclose,
       socreate (on error only),
       unp_drop, udp_usrreq (on abort only)
 */
sofree(so)
	register struct socket *so;
{
	struct socket *head;
	int owner = 0;  /* SMP */

	if (smp_debug){
		if( !(smp_owner(&so->lk_socket)))
			panic("sofree not lock owner");
	}

	head = so->so_head;
	/*
	 * If this is an accept socket, then delete
	 * the queues of pending connections. 
	 */
        /* 3.29.89.us.  Bug fix.  The imbeded if construct below is
         * necessary because 1. we can be freeing a rendezvous socket
         * and 2. we can be freeing a queued socket with one connection.
         * In the first case, head is the socket itself which is
         * already locked coming into sofree.  In the second case, which
         * comes up though so_close, so is locked and the queued socket
         * is locked coming into sofree.  Here, the locked so is the
         * head.  
         */
	if (head) {
		if ((head != so) && (!(smp_owner(&head->lk_socket))) ){
			owner = 1;
			SO_LOCK(head);
		}
		if (!soqremque(so, 0) && !soqremque(so, 1))
			panic("sofree dq");
		if ((head != so) && (owner == 1))
			smp_unlock(&head->lk_socket);
		so->so_head = 0;
	}
	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0) {
		return(0);
	}
	sbrelease(&so->so_snd);
	sorflush(so);
	smp_unlock(&so->lk_socket);
	KM_FREE(so, KM_SOCKET);
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 *
 * SMP:
 * Called at < splnet with no smp locks held.  Lock is free'd in sofree().
 * Called from soo_close, closep, socketpair, pipe, sclose.
 *
 */
soclose(so)
	register struct socket *so;
{
	int s;
	int error = 0;
	int error2;

	s = splnet();
	SO_LOCK(so);
	if (so->so_options & SO_ACCEPTCONN) {
	  
	  so->so_options = (so->so_options & ~SO_ACCEPTCONN); /* disallow any more connections */

retry:
		while (so->so_q0 != so){
			if (smp) {
	  			if (!smp_lock(&so->so_q0->lk_socket,LK_ONCE)) {
					so_backoff(so);
					goto retry;

	   			} else {
					if (so->so_q0->ref) {
						smp_unlock(&so->so_q0->lk_socket);
						so_backoff(so);
						goto retry;
					}
	   			}
			}
			(void) soabort(so->so_q0);
		}
		while (so->so_q != so){
			if (smp) {
	  			if (!smp_lock(&so->so_q->lk_socket,LK_ONCE)) {
					so_backoff(so);
					goto retry;

	   			} else {
					if (so->so_q->ref) {
						smp_unlock(&so->so_q->lk_socket);
						so_backoff(so);
						goto retry;
					}
	   			}
			}
			(void) soabort(so->so_q);
		}
	}

	if (smp_debug){
		if( !(smp_owner(&so->lk_socket)))
			panic("soclose 1 not lock owner");
	}
	if (so->so_pcb == 0)
		goto discard;
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so);
			if (error)
				goto drop;
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO))
				goto drop;
			while (so->so_state & SS_ISCONNECTED) {
				if (!smp)
					sleep((caddr_t)&so->so_timeo,PZERO+1);
				else{
				  sleep_unlock((caddr_t)&so->so_timeo,PZERO+1,&so->lk_socket);
				  SO_LOCK(so);
				}
			}
		}
	}
drop:
	if (smp_debug){
		if( !(smp_owner(&so->lk_socket)))
			panic("soclose 2 not lock owner\n");
	}
	if (so->so_pcb) {
		error2 =
		    (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
			(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
		if (smp){
		 if( !(smp_owner(&so->lk_socket))){
		    if ( !(error2) ){
		      mprintf(" soclose: error not returned from protocols\n");
		      error2 = ENXIO;
		    }
		    error = error2;
		    return(error);
		 }
		}
		if (error == 0)
			error = error2;
	}
discard:
	if (smp_debug){
		if( !(smp_owner(&so->lk_socket)))
			panic("soclose 3 not lock owner\n");
	}

	if (so->so_state & SS_NOFDREF)
		panic("soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	if (so->so_pcb) {
		sofree(so);
		smp_unlock(&so->lk_socket);
	} else
		sofree(so);	/* sofree does the smp_unlock() */
	if (smp_debug){
		if (smp_owner(&so->lk_socket))
			panic("soclose should not own lock\n");
	}

	splx(s);
	return (error);
}

/* enter with socket lock asserted and also leave that way */
so_backoff(so) 
struct socket *so;
{
	/* wait a tick */
	timeout(wakeup,&u.u_procp->p_sched ,1);

	/* note that sleep is interruptable. */
	sleep_unlock(&u.u_procp->p_sched,PZERO+1,&so->lk_socket);
	SO_LOCK(so);
}

/*
 * Must be called at splnet...
 *
 * SMP:
 * Called from soclose at splnet with smp lock held.
 */
soabort(so)
	struct socket *so;
{

	return (
	    (*so->so_proto->pr_usrreq)(so, PRU_ABORT,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
}

/*
 * SMP:
 * Called from accept() at splnet with smp lock held.
 */
soaccept(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int error;
	if (smp_debug){
		if ( !(smp_owner(&so->lk_socket)) )
			panic("soaccept not lock owner");
	}
	if ((so->so_state & SS_NOFDREF) == 0)
		panic("soaccept: !NOFDREF");
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT,
	    (struct mbuf *)0, nam, (struct mbuf *)0);
	return (error);
}

/*
 * SMP:
 * Called at splnet with smp lock held.
 * Called from connect() and from in uipc_syssocket.c
 * Connect() and elevate to splnet before calling soconnect.
 */
soconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int error;
	if (smp_debug){
		if ( !(smp_owner(&so->lk_socket)) )
			panic("soconnect not lock owner");
	}
	if ((so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING)) &&
		((so->so_proto->pr_flags&PR_CONNREQUIRED) ||
		(error = sodisconnect(so)))) {
		error = EISCONN;
	} else
		error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
			(struct mbuf *)0, nam, (struct mbuf *)0);
	return (error);
}

/*
 * SMP:  Note that this is one instance where usrreq
 * is NOT called with smp lock held.  
 */
soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int s = splnet();
	int error;

	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2,
	    (struct mbuf *)0, (struct mbuf *)so2, (struct mbuf *)0);
	splx(s);
	return (error);
}

/* 
 * SMP:
 * Called with smp locks held.
 * Called from soclose, soconnect.
 */
sodisconnect(so)
	register struct socket *so;
{
	int error;
	if (smp_debug){
		if ( !(smp_owner(&so->lk_socket)) )
			panic("sodisconnect not lock owner");
	}
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
bad:
	return (error);
}

/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 * Otherwise, if nonblocking, send as much as possible.
 * 
 * SMP:
 * Called at < splnet with no smp locks set.
 */
sosend(so, nam, uio, flags, rights)
	register struct socket *so;
	struct mbuf *nam;
	register struct uio *uio;
	int flags;
	struct mbuf *rights;
{
	struct mbuf *top = 0;
	register struct mbuf *m, **mp;
	register int space;
	int len, rlen = 0, error = 0, s, dontroute, first = 1, clen = 0;

	s = splnet(); /* SMP */
	SO_LOCK(so);
	if (sosendallatonce(so) && uio->uio_resid > so->so_snd.sb_hiwat) {
		smp_unlock(&so->lk_socket);
		splx(s);
		return (EMSGSIZE);
	}
	dontroute =
	    (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);

	if (rights)
		rlen = rights->m_len;
#define	snderr(errno)	{ error = errno; goto release; }

restart:
	/* sblock(&so->so_snd); */
	sblock2(&so->so_snd, so);
	do {
		if (smp_debug){
			if ( !(smp_owner(&so->lk_socket)) )
				panic("sosend not lock owner");
		}
		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;			/* ??? */
			goto release;	/* smp unlock in release */
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
 			if (so->so_proto->pr_flags & PR_CONNREQUIRED) {
				if ((so->so_state & SS_ISCONNECTING) == 0) {
					snderr(ENOTCONN);
					/* NOTREACHED */
				}
				if (so->so_state & SS_NBIO) {
					snderr(EWOULDBLOCK);
					/* NOTREACHED */
				}
				sbunlock(&so->so_snd);
				while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0) {
					if (!smp)
					   sleep((caddr_t)&so->so_timeo,
						PZERO+1);
					else{
					  sleep_unlock((caddr_t)&so->so_timeo,
						PZERO+1,&so->lk_socket);
					  SO_LOCK(so);
					}
				}
				goto restart;
			}
  			if (nam == 0)
				snderr(EDESTADDRREQ);
		}
		if (flags & MSG_OOB)
			space = 1024;
		else {
			space = sbspace(&so->so_snd);
			if (space <= rlen ||
			   (sosendallatonce(so) && 
				space < uio->uio_resid + rlen) ||
			   (uio->uio_resid >= NCLBYTES && 
			   space < M_CLUSTERSZ &&
			   so->so_snd.sb_cc >= NCLBYTES &&
			   (so->so_state & SS_NBIO) == 0)) {
				if (so->so_state & SS_NBIO) {
					if (first) {
						error = EWOULDBLOCK;
						if (sosendallatonce(so)) {
							so->so_snd.sb_lowat = uio->uio_resid;
						}
					}
#ifdef XTI
					if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
					  if (so->so_xticb.xti_evtenabled) {
					    so->so_xticb.xti_blocked = 1;
					  }
					}
#endif XTI
					goto release;
				}
				sbunlock(&so->so_snd);
				/* sbwait2 is smp safe version of sbwait. */
				sbwait2(&so->so_snd, so);
				goto restart;
			}
		}
		mp = &top;
		space -= rlen;
		while (space > 0) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0){
				smp_unlock(&so->lk_socket);
				MGET(m, M_WAIT, MT_DATA);
				SO_LOCK(so);
			}
			if (uio->uio_resid >= NCLBYTES) {
				register struct mbuf *p;
				MCLGET(m, p);
				if (p == 0)
					goto nopages;
				len = MIN(M_CLUSTERSZ, uio->uio_resid);
				space -= len;
			} else {
nopages:
				len = MIN(MIN(MLEN, uio->uio_resid), space);
				space -= len;
			}
			clen += len;
			smp_unlock(&so->lk_socket);
			error = uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio);
			SO_LOCK(so);
			m->m_len = len;
			*mp = m;
			if (error)
				goto release;
			mp = &m->m_next;
			if (uio->uio_resid <= 0)
				break;
		}
		if (dontroute)
			so->so_options |= SO_DONTROUTE;
#ifdef XTI
			/*
			 * clear flow control events
			 */

			if (so->so_xticb.xti_epvalid) {
			  if (so->so_proto->pr_domain->dom_family == AF_INET) {
			    if (so->so_xticb.xti_evtarray[ffs(T_GODATA)])
			      so->so_xticb.xti_evtarray[ffs(T_GODATA)]--;
			    if (so->so_xticb.xti_evtarray[ffs(T_GOEXDATA)])
			      so->so_xticb.xti_evtarray[ffs(T_GOEXDATA)]--;
			  }
			  else {
			    if (!(flags & MSG_OOB)) {
			      if (so->so_xticb.xti_evtarray[ffs(T_GODATA)])
				so->so_xticb.xti_evtarray[ffs(T_GODATA)]--;
			    }
			    if (flags & MSG_OOB)
			      if (so->so_xticb.xti_evtarray[ffs(T_GOEXDATA)])
				so->so_xticb.xti_evtarray[ffs(T_GOEXDATA)]--;
			  }

			  /*
			   * do we need to buffer tsdu's
			   */

			  if (so->so_proto->pr_domain->dom_family == AF_OSI &&
			      so->so_proto->pr_type == SOCK_SEQPACKET) {
	
			    if (sosendallatonce(so) && 
				(uio->uio_resid+so->so_xticb.xtisb.sb_cc+clen)>so->so_snd.sb_hiwat){
			      if (so->so_xticb.xtisb.sb_mb)
				sbrelease(&so->so_xticb.xtisb);
			      error = EMSGSIZE;
			      break;
			    }
	
			    if (flags & MSG_OOB) {
			      if (so->so_xticb.xti_tpinfo->etsdu > 0)
				if ((clen+so->so_xticb.xtisb.sb_cc)>so->so_xticb.xti_tpinfo->etsdu) {
				  if (so->so_xticb.xtisb.sb_mb)
				    sbrelease(&so->so_xticb.xtisb);
				  error = EMSGSIZE;
				  break;
				} 
			    } 
			    else if (so->so_xticb.xti_tpinfo->tsdu > 0)
			      if ((clen+so->so_xticb.xtisb.sb_cc)>so->so_xticb.xti_tpinfo->tsdu) {
				if (so->so_xticb.xtisb.sb_mb)
				  sbrelease(&so->so_xticb.xtisb);
				error = EMSGSIZE;
				break;
			      }
	
			    if (flags & MSG_MORE) {
			      sbappend(&so->so_xticb.xtisb, top);
			      PRINTXTID(16, ("MSG_MORE: so=%X, cc=%d ,clen=%d\n",so,so->so_xticb.xtisb.sb_cc,clen));
			      top = 0;
			    } 
			    else if (so->so_xticb.xtisb.sb_cc) {
			      sbappend(&so->so_xticb.xtisb, top);
			      PRINTXTID(16, ("WAS ON:MSG_MORE: so=%x, cc=%d ,clen=%d\n",so,so->so_xticb.xtisb.sb_cc,clen));
				error = (*so->so_proto->pr_usrreq)
				  (so,(flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
				   so->so_xticb.xtisb.sb_mb, (caddr_t)nam, rights);
			      
			      top = 0;
			      so->so_xticb.xtisb.sb_mb = 0;
			      so->so_xticb.xtisb.sb_cc = 0;/*needs clean up*/
			      so->so_xticb.xtisb.sb_mbcnt = 0;
			      
			    }
			    else {
			      error = (*so->so_proto->pr_usrreq)
				(so,(flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
				 top, (caddr_t)nam, rights);
			    }
			  } else { /* not sequence packet && AF_OSI */
			    error = (*so->so_proto->pr_usrreq)
			      (so,(flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
			       top, (caddr_t)nam, rights);
			  }
			}
			else { /* not an xti endpoint */
			  
			  error = (*so->so_proto->pr_usrreq)
			    (so,(flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
			     top, (caddr_t)nam, rights);
			}
			
#else
		error = (*so->so_proto->pr_usrreq)(so,
		    (flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
		    top, (caddr_t)nam, rights);
#endif XTI
		if (dontroute)
			so->so_options &= ~SO_DONTROUTE;
		rights = 0;
		rlen = 0;
		clen = 0;
		top = 0;
		first = 0;
		if (error)
			break;
	} while (uio->uio_resid);

release:
	if (!error)
		so->so_snd.sb_lowat = 0;
	sbunlock(&so->so_snd);
	smp_unlock(&so->lk_socket);
	splx(s);
	if (top)
		m_freem(top);
	if (error == EPIPE) {
		psignal(u.u_procp, SIGPIPE);
	}
	return (error);
}

soreceive(so, aname, uio, flags, rightsp)
	register struct socket *so;
	struct mbuf **aname;
	register struct uio *uio;
	int flags;
	struct mbuf **rightsp;
{
	register struct mbuf *m, *n;
	register int len, error = 0, s, tomark;
	struct protosw *pr;
	struct mbuf *nextrecord;
	int moff;
#ifdef XTI
	struct sockbuf *curr_sb;
#endif

	s = splnet(); /* SMP */
	SO_LOCK(so);
	pr = so->so_proto;
	if (rightsp)
		*rightsp = 0;
	if (aname)
		*aname = 0;

	if (flags & MSG_OOB) {
#ifdef XTI
	  if (!(so->so_options & SO_BUFFOOB)) {
#endif XTI
	/* With smp, you cannot have a wait option on the mget here. 
	 * Reason is that we would have to release the lock @mget
	 * creating a valid hole where the sb_mb mbuf could be deleted
	 * by another process.  Note that there is no danger of a socket
	 * structure being released here.
	 */
	       m=m_get(M_DONTWAIT,(pr->pr_flags&PR_OOBADDR)?MT_SONAME:MT_DATA);
	       if (m == 0){
			error = ENOBUFS;
			goto bad;
	       }
	       error = (*pr->pr_usrreq)(so, PRU_RCVOOB,
		    m, (struct mbuf *)(flags & MSG_PEEK), (struct mbuf *)0);
	       if (error)
			goto bad;
		if (pr->pr_flags & PR_OOBADDR) {
		  	if ((m->m_type != MT_SONAME) || (!m->m_next))
				panic ("soreceive 0a");
	                if (aname) {
                    		*aname = m;
                    		m = m->m_next;
                    		(*aname)->m_next = 0;
                    	}
                 	else {
                    		MFREE (m,n);
                    		m = n;
                    	}
                 	if (m->m_type != MT_DATA)
				panic ("soreceive 0b");
		}
#ifdef XTI
	       if (so->so_xticb.xti_epvalid) {
		 if (!(flags & MSG_PEEK)) {
		   if (m->m_len > 0)
		     if (so->so_xticb.xti_evtarray[ffs(T_EXDATA)])
		       so->so_xticb.xti_evtarray[ffs(T_EXDATA)]--;
		 }
	       }
#endif XTI
		do {
			len = uio->uio_resid;
			if (len > m->m_len)
				len = m->m_len;
			smp_unlock(&so->lk_socket);
			error =
			    uiomove(mtod(m, caddr_t), (int)len, UIO_READ, uio);
			SO_LOCK(so);
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		smp_unlock(&so->lk_socket);
		if (m)
			m_freem(m);
		splx(s);
		return (error);
#ifdef XTI
	     }
#endif
	}

restart:
#ifdef XTI

        if (flags & MSG_OOB) 
	  curr_sb = &so->so_exrcv;
	else
	  curr_sb = &so->so_rcv;

	sblock2(curr_sb, so);
#else
	sblock2(&so->so_rcv, so);
#endif XTI

	if (smp_debug){
		if ( !(smp_owner(&so->lk_socket)) )
			panic("sorecv not lock owner");
	}
#define	rcverr(errno)	{ error = errno; goto release; }
#ifdef XTI
	if (curr_sb->sb_cc == 0) { /* nothing in recv. queue */
		if(so->so_type == SOCK_SEQPACKET && curr_sb->sb_mb != 0)
#else
	if (so->so_rcv.sb_cc == 0) { /* nothing in recv. queue */
		if(so->so_type == SOCK_SEQPACKET && so->so_rcv.sb_mb != 0)
#endif
			goto nulmsg;
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			goto release;	/* smp unlock in release */
		}

		if (so->so_state & SS_CANTRCVMORE) {
			goto release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED))
			rcverr(ENOTCONN);
		if (uio->uio_resid == 0)
			goto release;
#ifdef XTI
		if (so->so_state & SS_NBIO || flags & MSG_OOB) /* XTI */
#else
		if (so->so_state & SS_NBIO)
#endif XTI
			rcverr(EWOULDBLOCK);
#ifdef XTI
		sbunlock(curr_sb);
		sbwait2(curr_sb, so);
#else
		sbunlock(&so->so_rcv);
		sbwait2(&so->so_rcv, so);
#endif XTI

		goto restart;
	}
nulmsg:

#ifdef XTI
	m = curr_sb->sb_mb;
#else
	m = so->so_rcv.sb_mb;
#endif XTI
	if(m == 0)
		panic("receive 1");
	nextrecord = m->m_act;
	if (pr->pr_flags & PR_ADDR) {
		if (m->m_type != MT_SONAME)
			panic("receive 1a");
		if (flags & MSG_PEEK) {
			if (aname)
				*aname = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
#ifdef XTI
			sbfree(curr_sb, m);
#else
			sbfree(&so->so_rcv, m);
#endif XTI
			if (aname) {
				*aname = m;
				m = m->m_next;
				(*aname)->m_next = 0;
#ifdef XTI
				curr_sb->sb_mb = m;
			} else {
				MFREE(m, curr_sb->sb_mb);
				m = curr_sb->sb_mb;
#else
				so->so_rcv.sb_mb = m;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
#endif XTI
			}
			if (m)
				m->m_act = nextrecord;
		}
	}
	if (m && m->m_type == MT_RIGHTS) {
		if ((pr->pr_flags & PR_RIGHTS) == 0)
			panic("receive 2a");
		if (flags & MSG_PEEK) {
			if (rightsp)
				*rightsp = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
#ifdef XTI
			sbfree(curr_sb, m);
			if (rightsp) {
				*rightsp = m;
				curr_sb->sb_mb = m->m_next;
				m->m_next = 0;
				m = curr_sb->sb_mb;
			} else {
				MFREE(m, curr_sb->sb_mb);
				m = curr_sb->sb_mb;
#else
			sbfree(&so->so_rcv, m);
			if (rightsp) {
				*rightsp = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = 0;
				m = so->so_rcv.sb_mb;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
#endif XTI
			}
			if (m)
				m->m_act = nextrecord;
		}
	}
	moff = 0;
	tomark = so->so_oobmark;
	while (m && uio->uio_resid > 0 && error == 0) {
		if (m->m_type != MT_DATA)
			panic("receive 3");
		len = uio->uio_resid;
#ifdef XTI
		if ((flags & MSG_OOB) == 0) so->so_state &= ~SS_RCVATMARK;
#else
		so->so_state &= ~SS_RCVATMARK;
#endif XTI
		if ((flags & MSG_OOB) == 0)
		  if (tomark && len > tomark)
			len = tomark;
		if (len > m->m_len - moff)
			len = m->m_len - moff;

		smp_unlock(&so->lk_socket);
		error =
		    uiomove(mtod(m, caddr_t) + moff, (int)len, UIO_READ, uio);
		SO_LOCK(so);
		if (len == m->m_len - moff) {
			if (flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				nextrecord = m->m_act;
#ifdef XTI
				sbfree(curr_sb, m);
				MFREE(m, curr_sb->sb_mb);
				m = curr_sb->sb_mb;
#else
				sbfree(&so->so_rcv, m);
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
#endif XTI
				if (m)
					m->m_act = nextrecord;
			}
		} else {
			if (flags & MSG_PEEK)
				moff += len;
			else {
				m->m_off += len;
				m->m_len -= len;
#ifdef XTI
				curr_sb->sb_cc -= len;
#else
				so->so_rcv.sb_cc -= len;
#endif XTI
			}
		}
		if ((flags & MSG_PEEK) == 0 && so->so_oobmark) {
			so->so_oobmark -= len;
			if (so->so_oobmark == 0) {
				so->so_state |= SS_RCVATMARK;
				break;
			}
		}
		if ((flags & MSG_OOB) == 0)
		  if (tomark) {
		    tomark -= len;
		    if (tomark == 0)
		      break;
		  }
	}
	if ((flags & MSG_PEEK) == 0) {
		if (m == 0)
#ifdef XTI
		        curr_sb->sb_mb = nextrecord;
		else if (pr->pr_flags & PR_ATOMIC && (!(so->so_xticb.xti_epvalid)))
			(void) sbdroprecord(curr_sb);
#else
			so->so_rcv.sb_mb = nextrecord;
                else if (pr->pr_flags & PR_ATOMIC)
			(void) sbdroprecord(&so->so_rcv);
#endif XTI     

#ifdef XTI
		if (!(flags & MSG_OOB)) { 
		  if (so->so_xticb.xti_epvalid) {
		    /*
		     * clear data events 
		     */
		    if (!(flags & MSG_PEEK)) {
		      if (so->so_xticb.xti_evtarray[ffs(T_DATA)])
			so->so_xticb.xti_evtarray[ffs(T_DATA)]--;
		    }
		  }
		}
#endif XTI
		if (!(flags & MSG_OOB)) { 
		  if (pr->pr_flags & PR_WANTRCVD && so->so_pcb)
		    (*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf *)0,
				     (struct mbuf *)0, (struct mbuf *)0);
		  if (error == 0 && rightsp && *rightsp &&
		      pr->pr_domain->dom_externalize)
		    error = (*pr->pr_domain->dom_externalize)(*rightsp);
		}
	      }

	      if (flags & MSG_OOB) { 
#ifdef XTI
		if (so->so_xticb.xti_epvalid) {
		  if (!(flags & MSG_PEEK)) {
		    if (so->so_xticb.xti_evtarray[ffs(T_EXDATA)])
		      so->so_xticb.xti_evtarray[ffs(T_EXDATA)]--;
		  }
		}
#endif XTI
		error = (*pr->pr_usrreq)(so, PRU_RCVOOB,
					 (struct mbuf *)0,
					 (struct mbuf *)(flags & MSG_PEEK),
					 (struct mbuf *)0);
	      }
release:
#ifdef XTI
	sbunlock(curr_sb);
#else
	sbunlock(&so->so_rcv);
#endif XTI
	smp_unlock(&so->lk_socket);
	splx(s);
	return (error);
}

/*
 * SMP:
 * Soshutdown is called by unp_gc and shutdown.
 * Unp_gc is in uipc_usrreq and is called by closef in gfs_descrip.c
 */		
soshutdown(so, how)
	register struct socket *so;
	register int how;
{
	register struct protosw *pr;
	int s, error = 0;
	
	s = splnet(); /* SMP */
	SO_LOCK(so);
	pr = so->so_proto;

	if ((so->so_state & SS_ISCONNECTED) == 0) { /* error if socket */
		smp_unlock(&so->lk_socket);	    /* is not connected */
		splx(s);
		return(ENOTCONN);
	}

	how++;
	if (how & FREAD)
		sorflush(so);	/* this has splimp.  */
	if (how & FWRITE) {
		error = ((*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
		smp_unlock(&so->lk_socket);
		splx(s);
		return (error);
	}
	smp_unlock(&so->lk_socket);
	splx(s);
	return (0);
}

/* 
 * SMP:
 * New routine for smp.  Augments sbwait which has a sleep.
 * Sbwait2 modified to unlock, sleep, lock.
 * Must be called with smp lock held.
 * Wait for data to arrive at/drain from a socket buffer.
 */
sbwait2(sb, so)
	register struct sockbuf *sb;
	register struct socket *so;
{
	sb->sb_flags |= SB_WAIT;
	if (!smp)
		sleep((caddr_t)&sb->sb_cc, PZERO+1);
	else{
		sleep_unlock((caddr_t)&sb->sb_cc, PZERO+1,&so->lk_socket);
		SO_LOCK(so);
	}
}

/* 
 * SMP:
 * New routine for smp.  Augments sblock which has a sleep.
 * Sblock2 modified to unlock, sleep, lock.
 * Must be called with smp lock held.
 */
sblock2(sb, so)
	register struct sockbuf *sb;
	register struct socket *so;
{

	while ((sb)->sb_flags & SB_LOCK) {
		(sb)->sb_flags |= SB_WANT; 
		if (!smp)
			sleep((caddr_t)&(sb)->sb_flags, PZERO+1);
		else{
		  sleep_unlock((caddr_t)&(sb)->sb_flags, PZERO+1,&so->lk_socket);
		  SO_LOCK(so);
		}
	} 
	(sb)->sb_flags |= SB_LOCK;
}

/* 
 * SMP:
 * Called by sofree, soshutdown.
 * Called at splnet with smp lock held.
 */
sorflush(so)
	register struct socket *so;
{
	register struct sockbuf *sb = &so->so_rcv;
	register struct protosw *pr = so->so_proto;
	register int s;
	struct sockbuf asb;
	if (smp_debug){
		if ( !(smp_owner(&so->lk_socket)) )
			panic("sorflush not lock owner");
	}
	/*
	 * sblock2 replaces sblock.  sblock2 unlocks resource, sleeps, locks.
	 * Returns from sblock2 with smp lock asserted (+splnet).
	 */
	/* sblock(sb); */
	sblock2(sb, so);
	s = splimp();
	socantrcvmore(so);
	sbunlock(sb);
	asb = *sb;
	bzero((caddr_t)sb, sizeof (*sb));
	splx(s);
	if (pr->pr_flags & PR_RIGHTS && pr->pr_domain->dom_dispose)
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	sbrelease(&asb);
}

sosetopt(so, level, optname, m0)
	register struct socket *so;
	int level, optname;
	struct mbuf *m0;
{
	int error = 0;
	int s;
#ifdef XTI
	struct mbuf *m = m0;
	struct mbuf *nodata = 0;
	struct mbuf *m1 = 0;
#else
	register struct mbuf *m = m0;
#endif XTI
	s = splnet(); /* SMP */
	SO_LOCK(so);

	if (level != SOL_SOCKET) {
		if (so->so_proto) {
		    if (so->so_proto->pr_ctloutput) {
			error = ((*so->so_proto->pr_ctloutput)
				(PRCO_SETOPT, so, level, optname, &m0));
			smp_unlock(&so->lk_socket);
			splx(s);
			return (error);
		    }
		}
		error = ENOPROTOOPT;
	} else {
		switch (optname) {

		case SO_LINGER:
			if (m == NULL || m->m_len != sizeof (struct linger)) {
				error = EINVAL;
				goto bad;
			}
			so->so_linger = mtod(m, struct linger *)->l_linger;
			/* fall thru... */

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_OOBINLINE:
#ifdef XTI
		case SO_BUFFOOB:
#endif XTI
#ifdef soon
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
#else
			/*
			 * the following if statement is strictly
			 * for backwords compatibility so that we
			 * dont have to change effected user level
			 * programs immediately (8/16/85)
			 */
			if (m == NULL || m->m_len < sizeof (int)) 
				so->so_options |= optname;
			else
#endif soon
			if (*mtod(m, int *))
				so->so_options |= optname;
			else
				so->so_options &= ~optname;
			break;


		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			switch (optname) {

			case SO_SNDBUF:
			case SO_RCVBUF:
				if (sbreserve(optname == SO_SNDBUF ? &so->so_snd :
				    &so->so_rcv, *mtod(m, int *)) == 0) {
					error = ENOBUFS;
					goto bad;
				}
				break;

			case SO_SNDLOWAT:
				so->so_snd.sb_lowat = *mtod(m, int *);
				break;
			case SO_RCVLOWAT:
				so->so_rcv.sb_lowat = *mtod(m, int *);
				break;
			case SO_SNDTIMEO:
				so->so_snd.sb_timeo = *mtod(m, int *);
				break;
			case SO_RCVTIMEO:
				so->so_rcv.sb_timeo = *mtod(m, int *);
				break;
			}
			break;
#ifdef XTI

		case SO_XTITPSTATE:
			{
			  struct tp_states_s {
			    int old_state;
			    int new_state;
			  } tp_states;
	
			  
			  if (m == NULL || m->m_len < sizeof (struct tp_states_s)) {
			    error = EINVAL;
			    goto bad;
			  }

			  tp_states = *mtod(m, struct tp_states_s *);
	
			  if (so->so_xticb.xti_states != tp_states.old_state) {
			    error = EOPNOTSUPP;
			    goto bad;
			  }
	
			  if ( (tp_states.new_state < (T_UNBND - 1)) ||
			      (tp_states.new_state > T_INREL)) {
			    error = EINVAL;
			    goto bad;
			  }
			  
			  so->so_xticb.xti_states = tp_states.new_state;
			  
			  break;
			}
    

		case SO_XTISYNC:
      
			so->so_xticb.xti_epvalid = 1; /* valid for xti */
      
			if (so->so_pcb) {
			  if(so->so_proto->pr_ctloutput) {

			    level = so->so_proto->pr_protocol;
	  

			    error = (*so->so_proto->pr_ctloutput)
			      (PRCO_XTIMAPSTATE, so, level, optname, &nodata);
			    if (error) break;

			    error = (*so->so_proto->pr_ctloutput)
			      (PRCO_XTIMAPINFO, so, level, optname, &nodata);
			    if (error) break;

			  } else {
			    error = EOPNOTSUPP;
			    break;
			  }
			} else {
			  error = EOPNOTSUPP;
			  break;
			}

			so->so_xticb.xti_evtenabled = 1;
			break;
    

		case SO_XTIACCEPTCHK:
			{
			  struct xti_accept_check {
			    int resfd; /* resfd of t_accept */
			    int seqnum; /* sequence number of t_accept */
			    union {
			      struct sockaddr generic; /* address info from t_accept */
			    } addr;
			  } tmp_accept, tmp_accept1;
			  
			  struct socket *tmp_so;
			  struct socket *old_resfd_so;
			  struct socket *next;
			  struct file *tmp_fp;
			  int tmp_seq_num;
			  int i;
			  int accept_fd;

			  /* grab parameters for checking */

			  if (m == NULL || m->m_len < sizeof (struct xti_accept_check)) {
			    error = EINVAL;
			    goto bad;
			  }

			  tmp_accept = *mtod(m, struct xti_accept_check *);

			  /* In case of timeout, or other error due to deferment */

			  if (so->so_error) {
			    error = so->so_error;
			    so->so_error = 0;
			    break;
			  }
	
			  /* check to make sure fd is valid */
	
			  if (!so->so_xticb.xti_epvalid) {
			    error = EBADF;
			    break;
			  }

			  PRINTXTID(4, ("fd is valid\n"));
	
			  /* check to make sure resfd is valid */
	
			  tmp_fp = getf(tmp_accept.resfd);
			  
			  if (!tmp_fp) {
			    error = EBADF;
			    break;
			  }

			  tmp_so = (struct socket *) tmp_fp->f_data; /* get socket pointer */
			  old_resfd_so = tmp_so;

			  if (!tmp_so) {
			    error = EBADF;
			    break;
			  }

			  if (!tmp_so->so_xticb.xti_epvalid) {
			    error = EBADF;
			    break;
			  }

			  /* In case of timeout, or other error due to deferment */
			  
			  if (tmp_so->so_error) {
			    error = tmp_so->so_error;
			    tmp_so->so_error = 0;
			    break;
			  }

			  PRINTXTID(4, ("resfd is valid\n"));
			  
			  /* check to make sure valid calling address and correct seq. number. 
			   * 
			   * Have to find correct socket structure based on seq number
			   * to check caller address
			   */
			  
			  tmp_seq_num = tmp_accept.seqnum;

			  next = so->so_xticb.xti_q_flink;

			  if (next == so) {
			    PRINTXTID(4,("next==so(should not happen)\n"));
			    error = EBADF;
			    break;
			  }
			  
			  error = -1;
			  while (next) {
			    PRINTXTID(4, ("search key=%d looking at =%d\n",tmp_seq_num,next->so_xticb.xti_seqnum));
			    if (next->so_xticb.xti_seqnum == tmp_seq_num) {
			      error = 0;
			      break;
			    }
			    
			    else {
			      next = next->so_xticb.xti_q_flink;
			      if (next == so) {
				PRINTXTID(4, ("next==so #2(should not happen)\n"));
				error = EBADF;
				break;
			      }
			    }
			  }
    
			  if (error == -1) {
			    error = EINVAL;
			    break;
			  }
	
			  PRINTXTID(4,("seqnum is valid\n"));
			  if (next->so_pcb) {
			    if (next->so_proto){
			      if(next->so_proto->pr_ctloutput) {

				level = next->so_proto->pr_protocol;

				/*
				 * Copy old transport specific QOS which may
				 * have been re-negotiated between t_listen
				 * and t_accept.
				 */ 
				(void) soxticopy_tp_specifc(old_resfd_so, next);
				/*
				 * Now check for acceptance.
				 */
				/* m_adj(m, sizeof(int) * 2); */
				error = (*next->so_proto->pr_ctloutput)
				  (PRCO_XTICHKADDR, next, level, optname, &m);

				if (error != 0) {
				  error = EADDRNOTAVAIL;
				  break;
				}

			      } else {
				error = EPROTONOSUPPORT;
				break;
			      }

			    } else {
			      error = EPROTONOSUPPORT;
			      break;
			    }

			  } else {
			    error = EPROTONOSUPPORT;
			    break;
			  }

			  PRINTXTID(4, ("caller address is valid\n"));
	
			  /* all test passed... time to duplicate endpoint
			   * that was gotten from accept system call on top of resfd.
			   * Have to scan u_ofile to get descriptor which matches fd
			   * from accept call 
			   *
			   * Months later...remember to close duped endpoint.
			   */
  
			  accept_fd = -1; /* is not found yet */
			  
			  for (i = 0; i<= u.u_omax; i++) {
			    struct file *tmp_fp;
			    struct socket *tmp_so;
			    
			    if (U_OFILE(i)) { /* file pointer exists */
			      tmp_fp = U_OFILE(i);
			      if (tmp_fp->f_type == DTYPE_SOCKET) { /* is a socket! */
				tmp_so = (struct socket *) U_OFILE(i)->f_data;
				if (tmp_so->so_xticb.xti_seqnum == tmp_accept.seqnum) {
				  accept_fd = i;
				  break;
				}
			      }
			    }
			  } /* end of for */
			  
			  if (accept_fd == -1) {
			    error = EBADF;
			    break;
			  }
			  smp_unlock(&so->lk_socket);
			  internal_dup2(accept_fd, tmp_accept.resfd);
			  smp_lock(&so->lk_socket, LK_RETRY);

			  /* finally let's remove entry from xti q */
	 
			  if (next != so)
			    SO_LOCK(next);
	  
			  soxtiqremque(so, next); /* remove entry */

			  if (next != so)
			    smp_unlock(&next->lk_socket);	    

			  /* close the dup'd endpoint */

			  tmp_fp = U_OFILE(accept_fd);
			  U_OFILE_SET(accept_fd,NULL);
			  U_POFILE_SET(accept_fd,0);

			  /*
			   * must obey lock ordering
			   */

			  so->ref = 33;
			  smp_unlock(&so->lk_socket);
			  closef(tmp_fp);
			  smp_lock(&so->lk_socket,LK_RETRY);
			  so->ref = 0;

			  /* update the state of this socket */

			  if (next->so_pcb) {
			    if(next->so_proto->pr_ctloutput) {

			      level = next->so_proto->pr_protocol;

			      error = (*next->so_proto->pr_ctloutput)
				(PRCO_XTIMAPSTATE, next, level, optname, &nodata);

			      if (error != 0) {
				break;
			      }

			    } else {
			      error = EOPNOTSUPP;
			      break;
			    }
			  } else {
			    error = EOPNOTSUPP;
			    break;
			  }
      
			  break;
			}

		case SO_XTIFDVALID:
      
			if (m == NULL || m->m_len < sizeof (int)) {
			  error = EINVAL;
			  goto bad;
			}
      
			so->so_xticb.xti_epvalid = *mtod(m, int *);
			break;
      

		case SO_XTISEQNUM:

			if (m == NULL || m->m_len < sizeof (int)) {
			  error = EINVAL;
			  goto bad;
			}

			so->so_xticb.xti_seqnum = *mtod(m, int *);
			break;

      
	        case SO_XTICLREVENT:

			{
			  unsigned int tmp_event;
			  int status;
			  
			  if (m == NULL || m->m_len < sizeof (int)) {
			    error = EINVAL;
			    goto bad;
			  }
			  
			  tmp_event = *mtod(m, unsigned int *);
			  so->so_xticb.xti_evtarray[ffs(tmp_event)]--;
			  break;
			}


                case SO_XTIENEVENT:

			{
			  struct xti_event_q (*tmp_xti_ptr)[];
			  
			  if (m == NULL || m->m_len < sizeof (int)) {
			    error = EINVAL;
			    goto bad;
			  }
	
			  so->so_xticb.xti_evtenabled = *mtod(m, int *);
			  break;
			}

                case SO_XTIABORT:
			/*
			 * For incomming connections we must use sequence #.
			 * For established connections we just use so.
			 */
			
			{
			  struct socket *save_so;
			  
			  if (m == NULL || m->m_len < sizeof (int)) {
			    error = EINVAL;
			    goto bad;
			  }
			  
			  save_so = so;
			  
			  if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			    
			    /*
			     * Abort the generic connection
			     *
			     * Search the socket queue (xti) for proper socket and
			     * then call specific provider routine to cleanup.
			     */
	  
			    int tmp_seq_num;
			    struct socket *next;
	  
			    tmp_seq_num = *mtod(m, int *);
			    next = so->so_xticb.xti_q_flink;
	  
			    if (next == so) {
			      PRINTXTID(4, ("next==so(should not happen)\n"));
			      error = EBADF;
			      break;
			    }
	  
			    error = -1;
			    if (next) {
			      while (next) {
				PRINTXTID(4, ("search key=%d looking at =%d\n",tmp_seq_num,next->so_xticb.xti_seqnum));
				if (next->so_xticb.xti_seqnum == tmp_seq_num) {
				  error = 0;
				  PRINTXTID(4, ("seqnum is valid=%X\n",next));
				  break;
				}
				else {
				  next = next->so_xticb.xti_q_flink;
				  if (next == so) {
				    PRINTXTID(4, ("next==so #2(should not happen)\n"));
				    error = EBADF;
				    break;
				  }
				}
			      }
			    } else { /* no next - use so to abort established connection. */
			      error = 0;
			      next = so; /* fake out code below */
			    }
			    
			    if (error == -1) {
			      error = EINVAL;
			      break;
			    }
			    
			    /* load so with the proper socket */
			    
			    so = next;
			    
			    /* In case of timeout, or other error due to deferment */
			    
			    if (so->so_error) {
			      error = so->so_error;
			      so->so_error = 0;
			      PRINTXTID(4, ("error found in socket=%d\n",error));
			      so = save_so;
			      break;
			    }
			    
			    /*
			     * Call provider specific reject/abort routine
			     */
			    
			    if (so->so_pcb) {
			      if(so->so_proto->pr_ctloutput) {

				level = so->so_proto->pr_protocol;

				/*
				 * have to lock so (really next) because
				 * owner check will panic us
				 *
				 * if so != save_so then
				 * so is socket of incomming connect
				 * else save_so == so and is already
				 * locked
				 */

				if (so != save_so) /* incomming connect */
				  SO_LOCK(so);

				/*
				 * have to unlock head save_so if we
				 * are aborting an incomming connect or
				 * we will panic with lock messup
				 */

				if (so != save_so) {
				  save_so->ref = 27;
				  smp_unlock(&save_so->lk_socket);
				}

				error = (*so->so_proto->pr_ctloutput)
				  (PRCO_XTIREJECT, so, level, optname, &nodata);
				/* 
				 * have to relock save_so
				 */
				
				if (so != save_so) {
				  save_so->ref = 0;
				  smp_lock(&save_so->lk_socket, LK_RETRY);
				}

				/*
				 * have to unlock so
				 */

				if (so != save_so)
				  smp_unlock(&so->lk_socket);

				
			      } else {
				error = EOPNOTSUPP;
				so = save_so;
				break;
			      }
			    } else {
			      error = EOPNOTSUPP;
			      so = save_so;
			      break;
			    }

			    if (next != save_so) {
			      SO_LOCK(next);
			      soxtiqremque(save_so, next); /* remove entry */
			      smp_unlock(&next->lk_socket);	    
			    }
			  }
			  else {
			    error = EOPNOTSUPP;
			    so = save_so;
			    break;
			  }
			  so = save_so;
			  break;
			} /* end of abort */

		case SO_XTIUNBIND:
      
      
			if (so->so_pcb) {
			  if(so->so_proto->pr_ctloutput) {

			    level = so->so_proto->pr_protocol;
	  
			    error = (*so->so_proto->pr_ctloutput)
			      (PRCO_XTIUNBIND, so, level, optname, &nodata);
	  
			    if (error) break;

			  }
			}

			break;
    

#endif XTI

		default:
			error = ENOPROTOOPT;
			break;
		}
	}
bad:
	smp_unlock(&so->lk_socket);
	if (m)
		(void) m_free(m);
	splx(s);
	return (error);
}

sogetopt(so, level, optname, mp)
	register struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	register struct mbuf *clbuf,*m;
	int error, s;

	s = splnet(); /* SMP */
	SO_LOCK(so);
	if (level != SOL_SOCKET) {
		if (so->so_proto) {
		    if (so->so_proto->pr_ctloutput) {
			error = ((*so->so_proto->pr_ctloutput)
				(PRCO_GETOPT, so, level, optname, mp));
			smp_unlock(&so->lk_socket);
			splx(s);
			return (error);
		    }
		}
		else{
			smp_unlock(&so->lk_socket);
	  		splx(s);
			return (ENOPROTOOPT);
		}
	} else {
		smp_unlock(&so->lk_socket);
		m = m_get(M_WAIT, MT_SOOPTS);
		s = splnet();
		SO_LOCK(so);
		if (m == NULL) {
			smp_unlock(&so->lk_socket);
			splx(s);
			return (ENOBUFS);
		}
		m->m_len = sizeof(int);
		switch (optname) {

		case SO_LINGER:
			m->m_len = sizeof (struct linger);
			mtod(m, struct linger *)->l_onoff =
				so->so_options & SO_LINGER;
			mtod(m, struct linger *)->l_linger = so->so_linger;
			break;

		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_OOBINLINE:
#ifdef XTI
		case SO_BUFFOOB:
#endif XTI
			*mtod(m, int *) = so->so_options & optname;
			break;

		case SO_TYPE:
			*mtod(m, int *)	= so->so_type;
			break;

		case SO_ERROR:
			*mtod(m, int *) = so->so_error;
			so->so_error = 0;
			break;

		case SO_SNDBUF:
			*mtod(m, int *) = so->so_snd.sb_hiwat;
			break;

		case SO_RCVBUF:
			*mtod(m, int *) = so->so_rcv.sb_hiwat;
			break;

		case SO_SNDLOWAT:
			*mtod(m, int *) = so->so_snd.sb_lowat;
			break;

		case SO_RCVLOWAT:
			*mtod(m, int *) = so->so_rcv.sb_lowat;
			break;

		case SO_SNDTIMEO:
			*mtod(m, int *) = so->so_snd.sb_timeo;
			break;

		case SO_RCVTIMEO:
			*mtod(m, int *) = so->so_rcv.sb_timeo;
			break;

#ifdef XTI
		case SO_XTIREADEX:
			*mtod(m, int *) = so->so_exrcv.sb_cc;
			break;

		case SO_XTISEQNUM:

			*mtod(m, int *) = so->so_xticb.xti_seqnum;
			break;

		case SO_XTITPSTATE:
			*mtod(m, int *) = so->so_xticb.xti_states;
			break;

		case SO_XTIPEEKEVENT:
			{
			  int read_next;
			  int have_entry;
			  struct socket *tmp_so;
			  struct socket *save_so;
			  int have_events = 0;
			  int i;
			  int evtarray[XTI_MAX_EVTS];
			  
			  save_so = so;
			  /* search so for events first */
			  
			  for (i = 0; i < XTI_MAX_EVTS; i++) {
			    if (so->so_xticb.xti_evtarray[i]) {
			      have_events = 1;
			      PRINTXTID(8, ("found event at index %d for so=%x\n",i,so));
			      break;
			    }
			  }
			  
			  /* if no events on so socket check other sockets!!!!
			   * if this is a listening endpoint, have to look for events on
			   * the sockets pointed to by xti_q_flink.
			   * Thsese socket pointers are to the sockets created
			   * by sonewconn. The xti_evtarray is not used for the listening
			   * endpoint only for the accepting endpoint 
			   */

			  if (!have_events) {

			    tmp_so = so->so_xticb.xti_q_flink;
			    if (!tmp_so)
			      PRINTXTID(8, ("PEEK:No chain of sockets using so=%x\n",so));

			    while (tmp_so) { /* we have a queue to check */
	    
			      for (i = 0; i < XTI_MAX_EVTS; i++) {
				if (tmp_so->so_xticb.xti_evtarray[i]) {
				  have_events = 1;
				  PRINTXTID(8, ("found event xti_q index %d for so=%x\n",i,tmp_so));
				  break;
				}
			      }
	  
			      if (have_events) {
				so = tmp_so;
				break;
			      }
	    
			      tmp_so = tmp_so->so_xticb.xti_q_flink;
	    
			    } /* end of while */

			    /*
			     * Last ditch effort
			     * It is possible for an asych t_listen to occur
			     * and then a T_LISTEN event would be generated
			     * some time in the future. If this happens we
			     * won't have xti queue ptrs set-up, but we will
			     * have so_q still available
			     */
	  
			    if (!have_events)
			      if ( (so->so_options & SO_ACCEPTCONN) && (so->so_qlen > 0)) {

				tmp_so = so->so_q;

				while (tmp_so) { /* we have to check so_q */

				  for (i = 0; i < XTI_MAX_EVTS; i++) {
				    if (tmp_so->so_xticb.xti_evtarray[i]) {
				      have_events = 1;
				      PRINTXTID(8, ("found event on so_q index %d for so=%x\n",i,tmp_so));
				      break;
				    }
				  }
	  
				  if (have_events) {
				    so = tmp_so;
				    break;
				  }
	    
				  tmp_so = tmp_so->so_q;
	      
				} /* end of while */
			      } /* end of if */
			  } /* if we need to look at other sockets */

			  /* load evtarray */

			  m->m_len = XTI_MAX_EVTS*sizeof(int);
			  bcopy(&so->so_xticb.xti_evtarray[0],
				mtod(m, char *),m->m_len);

			  so = save_so;
			  break;
			}
      
      
		case SO_XTIFDVALID:
      
			*mtod(m, int *) = so->so_xticb.xti_epvalid;
      
			break;
      
		case SO_XTITPPROTO:
			{
			  struct protoinfo {
			    int family;
			    int xti_proto;
			  } tmp_proto_info;
	
			  tmp_proto_info.family = 
			    so->so_proto->pr_domain->dom_family;

			  tmp_proto_info.xti_proto = 
			    so->so_proto->pr_protocol;

			  m->m_len = sizeof(struct protoinfo);
			  bcopy(&tmp_proto_info , (mtod(m, struct protoinfo *)),
				m->m_len);
			  break;
			}

		case SO_XTITPDFLT:
      
			m->m_len = sizeof(struct t_info);
			bcopy(so->so_xticb.xti_tpinfo,(mtod(m, struct t_info *)),
			      m->m_len);
			break;
      
#endif XTI
		default:
			smp_unlock(&so->lk_socket);
			m_free(m);
			splx(s);
			return (ENOPROTOOPT);
		}
		*mp = m;
		smp_unlock(&so->lk_socket);
		splx(s);
		return (0);
	}
}

sohasoutofband(so)
	register struct socket *so;
{
	struct proc *p;
	int s;
	int owner = 0;  /* SMP */

	s = spl5();
	if ( !(smp_owner(&so->lk_socket) )){
		SO_LOCK(so);
		owner = 1;
	}
	if (so->so_pgrp < 0) {
	/*	so->ref = 6;
	 *	smp_unlock(&so->lk_socket);
	 */
		gsignal(-so->so_pgrp, SIGURG);

	/*	smp_lock(&so->lk_socket, LK_RETRY);
	 *	so->ref = 0;
	 */
	} else {
		if (so->so_pgrp > 0) {
		/* if psignal sleeps, need to unlock socket
		 * before call to psignal
		 */
			if ((p = proc_get(so->so_pgrp)) != (struct proc *)NULL) {
				psignal(p, SIGURG);
				proc_rele(p);
			}
		}
	}
	if (so->so_rcv.sb_sel){
		selwakeup(so->so_rcv.sb_sel, so->so_rcv.sb_flags & SB_COLL);
		so->so_rcv.sb_sel = 0;
		so->so_rcv.sb_flags &= ~SB_COLL;
	}
	if (owner)
		smp_unlock(&so->lk_socket);
	splx(s);
}

#ifdef XTI
/*
 *  we now have two sockets and two provider control blocks
 *  need to copy some useful info from one provider control block
 *  to the new one.
 *  Ie. For TCP we need to copy acceptmode flag
 * (SMP: will be called with locks for old socket)
 */
int
  soxticopy_tp_specifc(old,new)
        struct socket *old;
        struct socket *new;
{
	struct mbuf *m = 0;
	int error = 0;
	int level;
	int optname = 0;

	if (new->so_pcb == 0 || old->so_pcb == 0) return(0);
	if (old->so_proto->pr_ctloutput) {

	  level = old->so_proto->pr_protocol;

	  m = m_get(M_DONTWAIT, MT_SOOPTS);

	  if (m != 0) {
	    m->m_len = sizeof(struct socket *);
	    *(mtod(m, struct socket **)) = new;

	    error = (*old->so_proto->pr_ctloutput)
	      (PRCO_XTICOPYTP, old, level, optname, &m);
	  } else {
	    error = ENOBUFS;
	  }
	} else {
	  error = EOPNOTSUPP;
	}
  
	if (m) m_free(m);
  
	return(error);
}
#endif XTI




