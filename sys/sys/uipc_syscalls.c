#ifndef lint
static char *sccsid = "@(#)uipc_syscalls.c	4.3    ULTRIX  2/28/91";
#endif

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

/************************************************************************
 *			Modification History
 *
 *	28-Feb-91	prs
 *	Added support for a configurable number of open
 *	file descriptors.
 *
 *	27-Sep-90	jaw
 *	fill-in file table entry after allocation but before sleeping.
 *
 *	18-May-90	R. Bhanukitsiri/Jim Woodward
 *	Use so_backoff() where appropiate.
 *
 *	17-May-90	prs
 *	Clean up of file table locks and zeroing f_count.
 *
 *	03-Apr-90	U. Sinkewicz / gmm
 *	Fix an accept/free race
 *
 *	13-Mar-90	jaw
 *	fix to accept to free allocated buffer on longjmp.
 *
 *	2-Jan-90	U. Sinkewicz
 *	Performance enhancements to uniprocessor kernel.
 *
 *	09-Nov-89	jaw
 *	allow spin locks to be compiled out.
 *
 *	30-May-89	U. Sinkewicz
 *	Added SO_LOCK macro as part of the work done to fix the smp
 *	hole caused by unlocking the socket and locking it again soon
 *	afterwards.  The socket was unlocked to accomodate either the
 *	lock heirarchy or sleeping in a uiomove().  The fix guarantees 
 *	that the smp_lock will act on a valid socket pointer.
 *
 *      05-May-89      Michael G. McMenemy
 *              Add XTI support.
 *
 *	11-Apr-89	U. Sinkewicz
 *		Picked up mcmenemy changes 2/10/89 adding
 *		length check in setsockopt for >M_CLUSTERSZ socket opt
 *		calls.  
 *	
 *	28-Feb-89	U.Sinkewicz
 *		SMP/mips merge.
 *
 *	16-Feb-89	prs
 *		Changed GMOD to GUPD in pipe() to set gnode modify
 *		time when a pipe is created.
 *
 *	5-Dec-88	U.Sinkewicz
 *		Added lp's changes from 19-Feb-88 - Remove length check
 *		in sosetopt which prevented large (>128) socket opt calls.
 *
 *	24-Aug-88	U.Sinkewicz
 *		Added sleep_unlock() for SMP.
 *
 *	28-Jul-88	prs
 *		SMP - pipe() bug fix.
 *	
 *	19-May-88	cb
 *		Modified GFS interface.
 *	
 *	10-Feb-88	prs
 *		Modified pipe system call to support new fifo code.
 *
 *	28-Jan-88	us
 *		Update of 4.3bsd changes to smp pools.
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 *	Jeff Chase    -	    03/12/86
 *		Changed setsockopt for the new MCLGET macro
 *
 *	Stephen Reilly, 09-Sept-85
 *	Modified to handle the new 4.3BSD namei code.
 *
 *	Larry Cohen    -    09/16/85 -  Add 43bsd changes
 *	R. Rodriguez   -    11/11/85 - Fix bug in pipe() and socketpair()
 *	They were calling sofree() when they should have called soclose()
 *	because socreate() allocated the pcb mbuf that sofree() didn't
 *	deallocate and soclose() does.
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	uipc_syscalls.c	6.9 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/gnode.h"
#include "../h/buf.h"
#include "../h/smp_lock.h"
#include "../h/mbuf.h"
#ifdef XTI
#include "../h/domain.h"
#endif XTI
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"
#include "../h/mount.h"
#include "../h/kernel.h"
#ifdef vax
#include "../machine/vax/mtpr.h"
#endif vax

/*
 * System call interface to the socket abstraction.
 */

struct	file *getsock();
extern	struct fileops socketops;
#ifdef XTI
extern int xti_debug;
#endif

/* 
 * SMP: Locks are in socreate.
 */
socket()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
	} *uap = (struct a *)u.u_ap;
	struct socket *so;
	register struct file *fp;

	if ((fp = falloc()) == NULL)
		return;
	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	fp->f_ops = &socketops;
	u.u_error = socreate(uap->domain, &so, uap->type, uap->protocol);
	if (u.u_error)
		goto bad;
	fp->f_data = (caddr_t)so;
	return;
bad:
	U_OFILE_SET(u.u_r.r_val1,0);
#ifdef MALLOCFILE
        F_REMQ(fp);
#else
        crfree(fp->f_cred);
	smp_lock(&lk_file, LK_RETRY);
        fp->f_count = 0;
	smp_unlock(&lk_file);
#endif MALLOCFILE
}

/*
 * SMP: Locks are in sobind.
 */
bind()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *nam;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	u.u_error = sockargs(&nam, uap->name, uap->namelen, MT_SONAME);
	if (u.u_error)
		return;
	u.u_error = sobind((struct socket *)fp->f_data, nam);
	m_freem(nam);
}

listen()
{
	register struct a {
		int	s;
		int	backlog;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	u.u_error = solisten((struct socket *)fp->f_data, uap->backlog);
}

/*
 * SMP:
 * Accept works on a queue of requests that can be from several clients.
 * Accept needs to lock the queues of requests against contention because
 * several processes can try to update the queue at once.
 */
/* There are locks at this level because there is socket manipulation
 * here.  The locks and splnets are like this:
 *	accept()
 *		splnet
 *		lock
 *		while (so->so_qlen == 0)
 *			unlock/sleep/lock
 *		soaccept
 *			usrreq(ACCEPT)-manipulate unp pointer (pcb for pipes)
 *		unlock
 *		splx
 */			
int wakeup();

accept()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	*anamelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct file  *fpnew;
	struct mbuf *nam;
	int namelen;
	int s;
	register struct socket *so;

	if (uap->name == 0)
		goto noname;
	u.u_error = copyin((caddr_t)uap->anamelen, (caddr_t)&namelen,
		sizeof (namelen));
	if (u.u_error)
		return;
	if (useracc((caddr_t)uap->name, (u_int)namelen, B_WRITE) == 0) {
		u.u_error = EFAULT;
		return;
	}
noname:
	fp = getsock(uap->s);
	if (fp == 0)
		return;

	nam = m_get(M_WAIT, MT_SONAME);

	if (ufalloc(0) < 0) {
		m_freem(nam);
		return;
	}

	fpnew = falloc();
	if (fpnew == 0) {
		U_OFILE_SET(u.u_r.r_val1,0);
		m_freem(nam);
		return;
	}

	fpnew->f_type = DTYPE_SOCKET;
	fpnew->f_flag = FREAD|FWRITE;
	fpnew->f_ops = &socketops;
	fpnew->f_data = (caddr_t)0;

	s = splnet();
	so = (struct socket *)fp->f_data;
	SO_LOCK(so);
	if ((so->so_options & SO_ACCEPTCONN) == 0) {
		u.u_error = EINVAL;
		goto bad;
	}
	if ((so->so_state & (SS_NBIO|SS_CANTRCVMORE)) == SS_NBIO &&
			so->so_qlen == 0 && so->so_error == 0) {
		u.u_error = EWOULDBLOCK;
		goto bad;
	}

	/* need to cleanup "nam" buffer if following sleep gets interrupted */
	if (setjmp(&u.u_qsave)) {
		u.u_error= EINTR;
		goto bad;
	}
retry:
	while (so->so_qlen == 0 && so->so_error == 0) {
		if (so->so_state & SS_CANTRCVMORE) {
			so->so_error = ECONNABORTED;
			break;
		}
		/* Can't sleep here with the lock held. */
		if (!smp)
			sleep((caddr_t)&so->so_timeo, PZERO+1);
		else{
		 sleep_unlock((caddr_t)&so->so_timeo, PZERO+1,&so->lk_socket);
		 SO_LOCK(so);
		}
	}
	if (so->so_error) {
		u.u_error = so->so_error;
		so->so_error = 0;
		goto bad;
	}
	
	/* If so and so_q are equal, bypass the additional locking. */
	if ( so != so->so_q){
	   if (!smp_lock(&so->so_q->lk_socket,LK_ONCE)) {
		/* need to wait */
		so_backoff(so);
		goto retry;

	   } else {
		if (so->so_q->ref) {
			smp_unlock(&so->so_q->lk_socket);

			/* need to wait */
			so_backoff(so);
			goto retry;
		}
	   }
	}
	/* Need to lock aso and socket here
	 * as both are being changed in soqremque. 
	 * Also need to protect aso head, if
	 * it's there.
	 */
	{
	    struct socket *aso_head;
	    struct socket *aso = so->so_q;
/*	    if (aso != so)	LOCK ABOVE
		    SO_LOCK(aso); */
	    aso_head = aso->so_head;
	    if (aso_head)
		if (aso_head != so) {
			SO_LOCK(aso_head);
		}
#ifdef XTI
	    if (so->so_xticb.xti_epvalid) {
	      {
		struct socket *xti_flink;
		
		xti_flink = so->so_xticb.xti_q_flink;
		if (xti_flink)
		  if (xti_flink != so && xti_flink != aso
		      && xti_flink != aso_head) {
		    SO_LOCK(xti_flink);
		  }
		soxtiqinsque(so, so->so_q);
		
		if (xti_flink)
		  if (xti_flink != so && xti_flink != aso
		      && xti_flink != aso_head) {
		    smp_unlock(&xti_flink->lk_socket);
		  }
	      }
	    }
#endif XTI
	    if (soqremque(aso, 1) == 0)
		panic("accept");
	    if (aso_head)
		if (aso_head != so)
			smp_unlock(&aso_head->lk_socket);
	    if (aso != so)
		    smp_unlock(&so->lk_socket);
	    so = aso;
	    /* note that we have a lock set. */
	}
	fpnew->f_type = DTYPE_SOCKET;
	fpnew->f_flag = FREAD|FWRITE;
	fpnew->f_ops = &socketops;
	fpnew->f_data = (caddr_t)so;
#ifdef XTI
	if (so->so_xticb.xti_evtenabled) 
	  if (so->so_xticb.xti_evtarray[ffs(T_LISTEN)])
	    so->so_xticb.xti_evtarray[ffs(T_LISTEN)]--;
#endif XTI
	(void) soaccept(so, nam);
  	smp_unlock(&so->lk_socket);
	if (uap->name) {
		if (namelen > nam->m_len)
			namelen = nam->m_len;

		/* SHOULD COPY OUT A CHAIN HERE */
		(void) copyout(mtod(nam, caddr_t), (caddr_t)uap->name,
		    (u_int)namelen);
		(void) copyout((caddr_t)&namelen, (caddr_t)uap->anamelen,
		    sizeof (*uap->anamelen));
	}
	m_freem(nam);
	splx(s);
	return;

bad:
	if (smp && smp_owner(&so->lk_socket))
		smp_unlock(&so->lk_socket);
	m_freem(nam);
	U_OFILE_SET(u.u_r.r_val1,0);

#ifdef MALLOCFILE
        F_REMQ(fpnew);
#else
       	crfree(fpnew->f_cred);
	smp_lock(&lk_file, LK_RETRY);
       	fpnew->f_count = 0;
	smp_unlock(&lk_file);
#endif MALLOCFILE
	splx(s);
	return;
}

/*
 * SMP:
 * Need smp locks at this level in connect because several connects might
 * be made at once to the same socket.  Some connect requests will be
 * queued, one will be answered, etc.  The socket needs to be locked
 * against contention until its connect request is satisfied.
 */
/* The lock situation here is like this:
 *	connect()
 *		splnet
 *		lock
 *		soconnect
 *			sodisconnect
 *			usrreq(CONNECT)
 *		while(SS_ISCONNECTING)
 *			unlock/sleep/lock
 *		unlock
 *		splx
 */
connect()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *nam;
	int s;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	s = splnet();
	so = (struct socket *)fp->f_data;
	SO_LOCK(so);
	if ((so->so_state & SS_NBIO) && 
		(so->so_state & SS_ISCONNECTING)) {
		   u.u_error = EALREADY;
		   smp_unlock(&so->lk_socket);
		   splx(s);
		   return;
	}
/*
 *  Sockargs cannot be called with the socket lock
 *  set.  Sockargs does an m_get call with a WAIT option which will
 *  cause a panic if unallocated memory becomes scarce.
 */
   	smp_unlock(&so->lk_socket);
	u.u_error = sockargs(&nam, uap->name, uap->namelen, MT_SONAME);
	SO_LOCK(so);
	if (u.u_error){
		smp_unlock(&so->lk_socket);
		splx(s);
		return;
	}
	u.u_error = soconnect(so, nam);
	if (smp && smp_owner(&so->lk_socket) == 0){
		if ( !(u.u_error) ){
		  mprintf("connect: error not returned from protocols\n");
		  u.u_error = ENXIO;
		}
		m_freem(nam); 
		splx(s);
		return;
	}
	if (u.u_error)
		goto bad;
	if ((so->so_state & SS_NBIO) &&
	    (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EINPROGRESS;
		m_freem(nam);
		smp_unlock(&so->lk_socket);
		splx(s);
		return;
	}
	/* Don't unlock here, just cleanup and return. */
	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0)
			u.u_error = EINTR;
		m_freem(nam);
		splx(s);
		return;
	}
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0){
		if (!smp)
			sleep((caddr_t)&so->so_timeo, PZERO+1);
		else{
		 sleep_unlock((caddr_t)&so->so_timeo, PZERO+1,&so->lk_socket);
		 SO_LOCK(so);
		}
	}
	u.u_error = so->so_error;
	so->so_error = 0;
#ifdef XTI
	if (so->so_xticb.xti_evtenabled) 
	  if (so->so_xticb.xti_evtarray[ffs(T_CONNECT)])
	    so->so_xticb.xti_evtarray[ffs(T_CONNECT)]--;
#endif XTI
bad:
	so->so_state &= ~SS_ISCONNECTING;
	smp_unlock(&so->lk_socket);
	m_freem(nam);
	splx(s);
}

/*
 * SMP:
 * No locks set going in.  Need to lock around soconnect2.
 */
socketpair()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
		int	*rsv;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp1, *fp2;
	struct socket *so1, *so2;
	int sv[2];
	int s;

	if (useracc((caddr_t)uap->rsv, 2 * sizeof (int), B_WRITE) == 0) {
		u.u_error = EFAULT;
		return;
	}
	u.u_error = socreate(uap->domain, &so1, uap->type, uap->protocol);
	if (u.u_error)
		return;
	u.u_error = socreate(uap->domain, &so2, uap->type, uap->protocol);
	if (u.u_error)
		goto free;
	fp1 = falloc();
	if (fp1 == NULL)
		goto free2;
	sv[0] = u.u_r.r_val1;
	fp1->f_flag = FREAD|FWRITE;
	fp1->f_type = DTYPE_SOCKET;
	fp1->f_ops = &socketops;
	fp1->f_data = (caddr_t)so1;
	fp2 = falloc();
	if (fp2 == NULL)
		goto free3;
	fp2->f_flag = FREAD|FWRITE;
	fp2->f_type = DTYPE_SOCKET;
	fp2->f_ops = &socketops;
	fp2->f_data = (caddr_t)so2;
	sv[1] = u.u_r.r_val1;
	s = splnet();
	smp_lock(&so1->lk_socket, LK_RETRY);
	u.u_error = soconnect2(so1, so2);
	smp_unlock(&so1->lk_socket);
	if (u.u_error){
		splx(s);
		goto free4;
	}
	if (uap->type == SOCK_DGRAM) {
		/*
		 * Datagram socket connection is asymmetric.
		 */
		 smp_lock(&so2->lk_socket, LK_RETRY);
		 u.u_error = soconnect2(so2, so1);
		 smp_unlock(&so2->lk_socket);
		 if (u.u_error){
			splx(s);
			goto free4;
		}
	}
	splx(s);
	u.u_r.r_val1 = 0;
	(void) copyout((caddr_t)sv, (caddr_t)uap->rsv, 2 * sizeof (int));
	return;
free4:
#ifdef MALLOCFILE
        F_REMQ(fp2);
#else
	crfree(fp2->f_cred);
	smp_lock(&lk_file, LK_RETRY);
        fp2->f_count = 0;
	smp_unlock(&lk_file);
#endif MALLOCFILE
	U_OFILE_SET(sv[1],0);
free3:
#ifdef MALLOCFILE
        F_REMQ(fp1);
#else
        crfree(fp1->f_cred);
	smp_lock(&lk_file, LK_RETRY);
        fp1->f_count = 0;
	smp_unlock(&lk_file);
#endif MALLOCFILE
	U_OFILE_SET(sv[0],0);
free2:
	soclose(so2);
free:
	soclose(so1);
}

/*
 * SMP:
 * There is no socket manipulation here so smp locks can be put in
 * subroutines.
 */
sendto()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	to;
		int	tolen;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = uap->to;
	msg.msg_namelen = uap->tolen;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
}

send()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
}

/*
 * SMP: No smp locks because nothing shareable is touched.
 */
sendmsg()
{
	register struct a {
		int	s;
		caddr_t	msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin(uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen >= sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error)
		return;
	msg.msg_iov = aiov;
#ifdef notdef
printf("sendmsg name %x namelen %d iov %x iovlen %d accrights %x &len %d\n",
msg.msg_name, msg.msg_namelen, msg.msg_iov, msg.msg_iovlen,
msg.msg_accrights, msg.msg_accrightslen);
#endif
	sendit(uap->s, &msg, uap->flags);
}


/*
 * Smp locks are in sosend().
 */
sendit(s, mp, flags)
	int s;
	register struct msghdr *mp;
	int flags;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *to, *rights;
	int len;
	
	fp = getsock(s);
	if (fp == 0)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		if (iov->iov_len == 0)
			continue;
		if (useracc(iov->iov_base, (u_int)iov->iov_len, B_READ) == 0) {
			u.u_error = EFAULT;
			return;
		}
		auio.uio_resid += iov->iov_len;
	}
	if (mp->msg_name) {
		u.u_error =
		    sockargs(&to, mp->msg_name, mp->msg_namelen, MT_SONAME);
		if (u.u_error)
			return;
	} else
		to = 0;
	if (mp->msg_accrights) {
		u.u_error =
		    sockargs(&rights, mp->msg_accrights, mp->msg_accrightslen,
		    MT_RIGHTS);
		if (u.u_error)
			goto bad;
	} else
		rights = 0;
	len = auio.uio_resid;
	u.u_error =
	    sosend((struct socket *)fp->f_data, to, &auio, flags, rights);
	u.u_r.r_val1 = len - auio.uio_resid;
	if (rights)
		m_freem(rights);
bad:
	if (to)
		m_freem(to);
}

recvfrom()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	from;
		int	*fromlenaddr;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;
	int len;

	if(uap->fromlenaddr) {
		u.u_error = copyin((caddr_t)uap->fromlenaddr, (caddr_t)&len,
			   sizeof (len));
	} else {
		len = 0;
	}
	if (u.u_error)
		return;
	msg.msg_name = uap->from;
	msg.msg_namelen = len;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)uap->fromlenaddr, (caddr_t)0);
}

recv()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)0, (caddr_t)0);
}

recvmsg()
{
	register struct a {
		int	s;
		struct	msghdr *msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin((caddr_t)uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen >= sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error)
		return;
	msg.msg_iov = aiov;
	if (msg.msg_accrights)
		if (useracc((caddr_t)msg.msg_accrights,
		    (unsigned)msg.msg_accrightslen, B_WRITE) == 0) {
			u.u_error = EFAULT;
			return;
		}
	recvit(uap->s, &msg, uap->flags,
	    (caddr_t)&uap->msg->msg_namelen,
	    (caddr_t)&uap->msg->msg_accrightslen);
}

/* 
 * Smp locks are in soreceive.
 */
recvit(s, mp, flags, namelenp, rightslenp)
	int s;
	register struct msghdr *mp;
	int flags;
	caddr_t namelenp, rightslenp;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *from, *rights;
	int len;
	
	fp = getsock(s);
	if (fp == 0)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		if (iov->iov_len == 0)
			continue;
		if (useracc(iov->iov_base, (u_int)iov->iov_len, B_WRITE) == 0) {
			u.u_error = EFAULT;
			return;
		}
		auio.uio_resid += iov->iov_len;
	}
	len = auio.uio_resid;
	u.u_error =
	    soreceive((struct socket *)fp->f_data, &from, &auio,
		flags, &rights);
	u.u_r.r_val1 = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else {
			if (len > from->m_len)
				len = from->m_len;
			(void) copyout((caddr_t)mtod(from, caddr_t),
			    (caddr_t)mp->msg_name, (unsigned)len);
		}
		(void) copyout((caddr_t)&len, namelenp, sizeof (int));
	}
	if (mp->msg_accrights) {
		len = mp->msg_accrightslen;
		if (len <= 0 || rights == 0)
			len = 0;
		else {
			if (len > rights->m_len)
				len = rights->m_len;
			(void) copyout((caddr_t)mtod(rights, caddr_t),
			    (caddr_t)mp->msg_accrights, (unsigned)len);
		}
		(void) copyout((caddr_t)&len, rightslenp, sizeof (int));
	}
	if (rights)
		m_freem(rights);
	if (from)
		m_freem(from);
}

shutdown()
{
	struct a {
		int	s;
		int	how;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	u.u_error = soshutdown((struct socket *)fp->f_data, uap->how);
}

/* 
 * Smp locks in sosetopt().
 */
setsockopt()
{
	struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		int	valsize;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	struct mbuf *m = NULL, *p = NULL;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
#ifdef notneeded
	if (uap->valsize > MLEN) {
		u.u_error = EINVAL;
		return;
	}
#endif notneeded
	if (uap->val)
	  if (uap->valsize < 0 || uap->valsize > M_CLUSTERSZ) {
		u.u_error = EINVAL;
		return;
	  }

	if (uap->val) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if(uap->valsize > MLEN) {
			MCLGET(m,p);
			if(p == NULL) {
				m_free(m);
				u.u_error = ENOBUFS;
				return;
			}
		}	
		u.u_error =
		    copyin(uap->val, mtod(m, caddr_t), (u_int)uap->valsize);
		if (u.u_error) {
			(void) m_free(m);
			return;
		}
		m->m_len = uap->valsize;
	}
	u.u_error =
	    sosetopt((struct socket *)fp->f_data, uap->level, uap->name, m);
}

/*
 * Smp locks in sogetopt().
 */
getsockopt()
{
	struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		int	*avalsize;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	struct mbuf *m = NULL;
	int valsize;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	if (uap->val) {
		u.u_error = copyin((caddr_t)uap->avalsize, (caddr_t)&valsize,
			sizeof (valsize));
		if (u.u_error)
			return;
	} else
		valsize = 0;
	u.u_error =
	    sogetopt((struct socket *)fp->f_data, uap->level, uap->name, &m);
	if (u.u_error)
		goto bad;
	if (uap->val && valsize && m != NULL) {
		if (valsize > m->m_len)
			valsize = m->m_len;
		u.u_error = copyout(mtod(m, caddr_t), uap->val, (u_int)valsize);
		if (u.u_error)
			goto bad;
		u.u_error = copyout((caddr_t)&valsize, (caddr_t)uap->avalsize,
		    sizeof (valsize));
	}
bad:
	if (m != NULL)
		(void) m_free(m);
}

/*
 * Smp Locks are in the socket primitives.
 * Nothing to protect at this level of pipe().
 */
pipe()
{
	register struct file *rf, *wf;
	register struct gnode *gp;
	int r;
	extern struct gnode_ops *fifo_gnodeops;
	extern struct fileops gnodeops;

	KM_ALLOC(gp, struct gnode *, sizeof(struct gnode),
		KM_TEMP, KM_CLEAR);
	if (gp == NULL) {
		return;
	}

	gp->g_count = 1;
	gp->g_ops = fifo_gnodeops;
	gp->g_mp = NULL;	/* XXX Set up bogus mp - never used */
	gp->g_mode = GFPIPE;

	lockinit(&gp->g_lk, &lock_eachgnode_d);
	gp->g_init = READY_GNODE;
	rf = falloc();
	if (rf == NULL) {
		grele(gp);
		return;
	}
	r = u.u_r.r_val1;
	rf->f_flag = FREAD | FAPPEND;
	rf->f_type = DTYPE_PIPE;
	rf->f_ops = &gnodeops;
	rf->f_data = (caddr_t)gp;
	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0)
			u.u_error = EINTR;
		goto free;
	}

	/*
	 * Call to GOPEN must have FNDELAY set, or call
	 * will sleep forever.
	 */
	u.u_error = GOPEN(gp, rf->f_flag | FNDELAY);

	if (u.u_error) {
		goto free;
	}

	wf = falloc();
	if (wf == NULL){
		goto free;
	}

	gp->g_count = 2;

	wf->f_flag = FWRITE | FAPPEND;
	wf->f_type = DTYPE_PIPE;
	wf->f_ops = &gnodeops;
	wf->f_data = (caddr_t)gp;
	u.u_r.r_val2 = u.u_r.r_val1;
	u.u_r.r_val1 = r;
	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0)
			u.u_error = EINTR;
		goto free;
	}

	u.u_error = GOPEN(gp, wf->f_flag);

	if (u.u_error) {
		goto free;
	}

	gp->g_flag |= (GACC|GUPD|GCHG);
	u.u_error = GUPDATE(gp, &time, &time, 0, u.u_cred);
	return;
free:
	if (wf) {
		U_OFILE_SET(u.u_r.r_val2,NULL);
		U_POFILE_SET(u.u_r.r_val2,0);
		closef(wf);
	}
	if (rf) {
		U_OFILE_SET(r,NULL);
		U_POFILE_SET(r,0);
		closef(rf);
	}
}

/*
 * Get socket name.
 */
getsockname()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *m;
	int len;
	int s;
	
	fp = getsock(uap->fdes);
	if (fp == 0)
		return;
	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	if (u.u_error)
		return;
	s = splnet(); /* SMP */
	so = (struct socket *)fp->f_data;
	m = m_getclr(M_WAIT, MT_SONAME);
	SO_LOCK(so);
	u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR, 0, m, 0);
	smp_unlock(&so->lk_socket);
	if (u.u_error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);
	if (u.u_error)
		goto bad;
	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
bad:
	m_freem(m);
	splx(s);
}

/*
 * Get name of peer for connected socket.
 */
getpeername()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *m;
	int len;
	int s;

	fp = getsock(uap->fdes);
	if (fp == 0)
		return;
	s = splnet(); /* SMP */
	so = (struct socket *)fp->f_data;
	SO_LOCK(so);
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		u.u_error = ENOTCONN;
		smp_unlock(&so->lk_socket);
		splx(s);
		return;
	}
	smp_unlock(&so->lk_socket);
	m = m_getclr(M_WAIT, MT_SONAME);
	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	SO_LOCK(so);
	if (u.u_error){
		smp_unlock(&so->lk_socket);
		goto bad;
	}
	u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_PEERADDR, 0, m, 0);
	smp_unlock(&so->lk_socket);
	if (u.u_error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);
	if (u.u_error)
		goto bad;
	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
bad:
	m_freem(m);
	splx(s);
}

/* 
 *  Called from bind(), connect(), sendit().  
 */
sockargs(aname, name, namelen, type)
	struct mbuf **aname;
	caddr_t name;
	int namelen, type;
{
	register struct mbuf *m;
	int error;

	if (namelen > M_CLUSTERSZ)
		return (EINVAL);
	m = m_get(M_WAIT, type);
	if (namelen > MLEN) {
		register struct mbuf *page;
		MCLGET(m,page);
		if ( ! page ) {
			(void) m_free(m);
			return (ENOBUFS);
		}
	}
	m->m_len = namelen;
	error = copyin(name, mtod(m, caddr_t), (u_int)namelen);
	if (error)
		(void) m_free(m);
	else
		*aname = m;
	return (error);
}

struct file *
getsock(fdes)
	int fdes;
{
	register struct file *fp;

	fp = getf(fdes);
	if (fp == NULL)
		return (0);
	if (fp->f_type != DTYPE_SOCKET) {
		u.u_error = ENOTSOCK;
		return (0);
	}
	return (fp);
}
