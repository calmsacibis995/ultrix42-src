/* static	char	*sccsid = "@(#)socketvar.h	4.4	(ULTRIX)	11/21/90"; */

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/h/socketvar.h
 *
 * 21-Nov-1990	Joe Szczypek
 *	Changed "int ref" in struct socket to "volatile int ref" to
 *	force reading of variable from memory.  Location was being
 *	optimized into a register, which resulted in "hangs" on SMP 
 *	MIPS machines.
 *
 * 14-Nov-1990	Matt Thomas
 *	Change sb_lowat and sb_timeo to ints.  Fix soreadable and sowriteable
 *	to work correctly with seq packet sockets.
 *
 * 2-Jan-90  -- U. Sinkweicz
 *	Performance enhancements to uniprocessor kernel.
 *
 * 12 Dec 89 -- R. Bhanukitsiri
 *	Add XTI macros.
 *
 * 10 Dec 89 -- chet
 *	change sballoc() and sbfree() macros to use NCLBYTES
 *
 * 30 May 89 -- U. Sinkewicz
 *	Added macro SO_LOCK.  Point is to guarantee that whenever the
 * socket is unlocked, and locked again soon thereafter, the lock is
 * on a valid socket pointer.  SO_LOCK replaces most straight 
 * smp_lock(&so->lk_socket, LK_RETRY) calls.  The straight smp_lock()
 * is still used when the socket is unlocked and then locked again
 * to accomodate either the lock heirarchy or uiomoves (sleeps).  What
 * happens in this case, is a ref field in the socket is set, the
 * socket is unlocked, then the socket is locked and the ref field is
 * set to zero.  Routines that are doing SO_LOCK check the ref field.
 * if it is set, they back off, spin testing the ref field and eventually
 * grabbing the socket lock once the ref field is back to zero.
 *
 * 03-May 89 -- Michael G. McMenemy
 *      Added XTI support.
 *
 * 10-April -- U. Sinkewicz
 *	Added macros for smp.
 *
 * 19 May 87 -- lp
 *	Changed sb_lowat and sb_timeo back to shorts (not used anyway).
 *
 * 19 May 87 -- logcher
 *	Changed sockbuf fields to int and SB_MAX back to 64K
 *
 * 12 May 87 -- jsd  (John Dustin)
 *	Backed out Larry Palmer's changes from 10-APR-87.  This
 *	fixes the VAXstar ris installation hangs and excessive
 *	mbuf cluster consumption problem.
 *
 * 10 Apr 87 -- logcher
 *	Added Larry Palmer's changes to make most fields in sockbuf
 *	unsigned and double SB_MAX
 *
 * 23 Oct 84 -- jrs
 *	Change definitions for sowakeup
 *	Derived from 4.2BSD, labeled:
 *		socketvar.h 6.3	84/03/22
 *
 * 09/16/85 -- Larry Cohen						*
 * 		Add 43bsd alpha tape changes  				*
 * -----------------------------------------------------------------------
 */

#ifndef XTI
#define XTI
#endif /*  XTI */

#ifndef KERNEL
#include <sys/smp_lock.h>
#ifdef XTI
#include <sys/xti.h>
#endif /* XTI */
#else
#include "../h/smp_lock.h"	/* 8.4.88.us */
#ifdef XTI
#include "../h/xti.h"
#endif /* XTI */
#endif

#define MAX_N_SP 9000

/*
 * Kernel structure per socket.
 * Contains send and receive buffer queues,
 * handle on protocol and pointer to protocol
 * private data and error information.
 */
struct socket {
	short	so_type;		/* generic type, see socket.h */
	short	so_options;		/* from socket call, see socket.h */
	short	so_linger;		/* time to linger while closing */
	short	so_state;		/* internal state flags SS_*, below */
	caddr_t	so_pcb;			/* protocol control block */
	struct	protosw *so_proto;	/* protocol handle */
/*
 * Variables for connection queueing.
 * Socket where accepts occur is so_head in all subsidiary sockets.
 * If so_head is 0, socket is not related to an accept.
 * For head socket so_q0 queues partially completed connections,
 * while so_q is a queue of connections ready to be accepted.
 * If a connection is aborted and it has so_head set, then
 * it has to be pulled out of either so_q0 or so_q.
 * We allow connections to queue up based on current queue lengths
 * and limit on number of queued connections for this socket.
 */
	struct	socket *so_head;	/* back pointer to accept socket */
	struct	socket *so_q0;		/* queue of partial connections */
	short	so_q0len;		/* partials on so_q0 */
	struct	socket *so_q;		/* queue of incoming connections */
	short	so_qlen;		/* number of connections on so_q */
	short	so_qlimit;		/* max number queued connections */
/*
 * Variables for socket buffering.
 */
	struct	sockbuf {
		int	sb_cc;		/* actual chars in buffer */
		int	sb_hiwat;	/* max actual char count */
		int	sb_mbcnt;	/* chars of mbufs used */
		int	sb_mbmax;	/* max chars of mbufs to use */
		int	sb_lowat;	/* low water mark */
		int	sb_timeo;	/* timeout (not used yet) */
		struct	mbuf *sb_mb;	/* the mbuf chain */
		struct	proc *sb_sel;	/* process selecting read/write */
		short	sb_flags;	/* flags, see below */
	} so_rcv, so_snd;
#define	SB_MAX		65535		/* max chars in sockbuf */
#define	SB_LOCK		0x01		/* lock on data queue (so_rcv only) */
#define	SB_WANT		0x02		/* someone is waiting to lock */
#define	SB_WAIT		0x04		/* someone is waiting for data/space */
#define	SB_SEL		0x08		/* buffer is selected */
#define	SB_COLL		0x10		/* collision selecting */
	short	so_timeo;		/* connection timeout */
	u_short	so_error;		/* error affecting connection */
	short	so_oobmark;		/* chars to oob mark */
	short	so_pgrp;		/* pgrp for signals */
	struct	lock_t	lk_socket;	/* SMP socket lock */
	volatile int ref;
#ifdef XTI
#define XTI_EVT_UNUSED     0            /* unused event number */
#define XTI_EVT_T_LISTEN   1		/* T_LISTEN bit position */
#define XTI_EVT_T_CONNECT  2		/* T_CONNECT bit position */
#define XTI_EVT_T_DATA	   3		/* T_DATA bit position */
#define XTI_EVT_T_EXDATA   4		/* T_EXDATA bit position */
#define XTI_EVT_T_DISCONNECT 5		/* T_DISCONNECT bit position */
#define XTI_EVT_T_UDERR	   7		/* T_UDERR bit position */
#define XTI_EVT_T_ORDREL   8		/* T_ORDREL bit position */
#define XTI_EVT_T_GODATA   9		/* T_GODATA bit position */
#define XTI_EVT_T_GOEXDATA 10		/* T_GOEXDATA bit position */

#define XTI_MAX_EVTS           11	/* maximun events */
	struct sockbuf so_exrcv;	/* expedited receive buffers */
	struct xticb {
	  int xti_states;               /* XTI states */
	  int xti_epvalid;		/* if (1) then valid xti endpoint */
	  int xti_evtenabled;		/* enable event generation */
	  int xti_evtarray[XTI_MAX_EVTS];  /* array for event counts */
	  struct t_info *xti_tpinfo;    /* ptrs to default chars. */
	  int xti_blocked;              /* used for flow control event gen. */
	  unsigned int xti_seqnum;
	  struct socket *xti_q_flink;   /* forward link sonewconn connections */
	  struct socket *xti_q_blink;   /* backward link sonewconn connections */
	  struct sockbuf xtisb;		/* socket buffer for T_MORE use */
	} so_xticb;
#endif /* XTI */
};

/*
 * Socket state bits.
 */
#define	SS_NOFDREF		0x001	/* no file table ref any more */
#define	SS_ISCONNECTED		0x002	/* socket connected to a peer */
#define	SS_ISCONNECTING		0x004	/* in process of connecting to peer */
#define	SS_ISDISCONNECTING	0x008	/* in process of disconnecting */
#define	SS_CANTSENDMORE		0x010	/* can't send more data to peer */
#define	SS_CANTRCVMORE		0x020	/* can't receive more data from peer */
#define	SS_RCVATMARK		0x040	/* at mark on input */

#define	SS_PRIV			0x080	/* privileged for broadcast, raw... */
#define	SS_NBIO			0x100	/* non-blocking ops */
#define	SS_ASYNC		0x200	/* async i/o notify */


/*
 * Macros for sockets and socket buffering.
 */

/* how much space is there in a socket buffer (so->so_snd or so->so_rcv) */
#define	sbspace(sb) \
    (MIN((sb)->sb_hiwat-(sb)->sb_cc, ((sb)->sb_mbmax-(sb)->sb_mbcnt)))

/* do we have to send all at once on a socket? */
#define	sosendallatonce(so) \
    ((so)->so_proto->pr_flags & PR_ATOMIC)

/* can we read something from so? */
#define	soreadable(so) \
    ((so)->so_rcv.sb_mb || ((so)->so_state & SS_CANTRCVMORE) || (so)->so_qlen)

/* can we write something to so? */
#define	sowriteable(so) \
    (sbspace(&(so)->so_snd) > (so)->so_snd.sb_lowat && \
	(((so)->so_state&SS_ISCONNECTED) || \
	  ((so)->so_proto->pr_flags&PR_CONNREQUIRED)==0) || \
     ((so)->so_state & SS_CANTSENDMORE))

/* adjust counters in sb reflecting allocation of m */
#define	sballoc(sb, m) { \
	(sb)->sb_cc += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_off > MMAXOFF) \
		(sb)->sb_mbcnt += NCLBYTES; \
}

/* adjust counters in sb reflecting freeing of m */
#define	sbfree(sb, m) { \
	(sb)->sb_cc -= (m)->m_len; \
	(sb)->sb_mbcnt -= MSIZE; \
	if ((m)->m_off > MMAXOFF) \
		(sb)->sb_mbcnt -= NCLBYTES; \
}

/* set lock on sockbuf sb */
/* We should only be using sblock2.  */
/* #ifdef notdef */
#define sblock(sb) { \
	while ((sb)->sb_flags & SB_LOCK) { \
		(sb)->sb_flags |= SB_WANT; \
		sleep((caddr_t)&(sb)->sb_flags, PZERO+1); \
	} \
	(sb)->sb_flags |= SB_LOCK; \
}
/* #endif */

/* release lock on sockbuf sb */
#define	sbunlock(sb) { \
	(sb)->sb_flags &= ~SB_LOCK; \
	if ((sb)->sb_flags & SB_WANT) { \
		(sb)->sb_flags &= ~SB_WANT; \
		wakeup((caddr_t)&(sb)->sb_flags); \
	} \
}

#define	sorwakeup(so)	sowakeup((so), &(so)->so_rcv)
#define	sowwakeup(so)	sowakeup((so), &(so)->so_snd)

/* SMP macros.  */
#define SO_LOCK(s)\
    { extern int smp; \
     if (smp){\
	smp_lock(&s->lk_socket, LK_RETRY);\
	while (s->ref > 0){\
		int spincount = 0; \
		smp_unlock(&s->lk_socket); \
		while (s->ref > 0){\
			spincount++;\
			if (spincount >= MAX_N_SP){\
				mprintf("SO_LOCK: s->ref %x\n", s->ref);\
				spincount = 0;\
				}\
			}\
		smp_lock(&s->lk_socket, LK_RETRY);\
	}\
     }\
   }

#ifdef XTI
/*
 * XTI macros
 */
#define	XTIEP(s)	(s->so_xticb.xti_epvalid ? 1 : 0)
#define	XTIEVTENABLE(s)	\
	((s->so_xticb.xti_epvalid && s->so_xticb.xti_evtenabled) ? 1 : 0)
#endif /* XTI */

#ifdef KERNEL
struct	mbuf *sbdrop();
struct	mbuf *sbdroprecord();
struct	socket *sonewconn();
#endif
