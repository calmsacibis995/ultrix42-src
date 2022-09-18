#ifndef lint
static char *sccsid = "@(#)uipc_msg.c	4.1	ULTRIX	7/2/90";
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
/**/
/*
 *
 *   File name:
 *
 *	uipc_msg.h
 *
 *
 *   Source file description:
 *
 *	This file contains system calls and associated support functions to 
 *	implement System V IPC.
 *
 *   System Calls
 *
 *	msgctl 		System call to provide control functions to the
 *			application's message queues.
 *
 *	msgget 		System call to get a message queue for write or read.
 *
 *	msgrcv 		System call to receive a message.
 *
 *	msgsnd 		System call to send a message.
 *
 *
 *   Functions:
 *
 *	msgconv 	Convert a user supplied message queue id into a ptr to a
 *			msqid_ds structure.
 *
 *	msgfree  	Free up space and message header, relink pointers on q,
 *			and wakeup anyone waiting for resources.
 *
 *	msginit 	Called by main(main.c) to initialize message queues.
 *
 *
 *   Usage:
 *
 *
 *   Compile:
 *
 *
 *   Modification history:
 *
 * 09 Nov 89 -- jaw
 *	fix locking bug in msgconv and protect against negitive index.
 *
 * 09 Nov 89 -- bp
 *	optimized lk_msgtxt lock in the event we page fault while doing
 *	the copying in msgsnd.
 *
 * 25 Jul 89 -- chet
 *	Change declaration of time
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 14 Oct 85 -- reilly
 *	Modified the user.h file
 *
 * 29 Jul 85 -- depp
 *	Removed "curpri = PMSG" statements as they are not needed and
 *	they interfere with multiprocessor scheduling.  I'm not sure
 *	why they were in the code to begin with, but they are in System V
 *	v2r2.
 *
 * 23 Jul 85 -- depp
 *	Fixed minor bug in msgfree, a set of {} braces missing.
 *
 * 22 Feb 85 -- depp
 *	New file in system
 *
 */

/*
**	Inter-Process Communication Message Facility.
*/

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/seg.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/errno.h"
#include "../h/map.h"
#include "../h/ipc.h"
#include "../h/msg.h"
#include "../h/systm.h"

extern struct map	msgmap[];	/* msg allocation map */
extern struct msqid_ds	msgque[];	/* msg queue headers */
extern struct msg	msgh[];		/* message headers */
extern struct msginfo	msginfo;	/* message parameters */
extern char ipcmsgbuf[];		/* message buffer */
struct msg		*msgfp;		/* ptr to head of free header list */
extern struct timeval time;		/* system idea of date */

struct msqid_ds		*ipcget(),
			*msgconv();

struct lock_t	lk_msgq;		/* get/conv coordination */
struct lock_t	lk_msgtxt;		/* msgfp, msginfo.msgwnt */

/* Convert bytes to msg segments. */
#define	btoq(X)	((X + msginfo.msgssz - 1) / msginfo.msgssz)

/*
**	msgconv - Convert a user supplied message queue id into a ptr to a
**		msqid_ds structure.
**	On success this routine returns a LOCKED queue pointer.
*/

struct msqid_ds *
msgconv(id)
register int	id;
{
	register struct msqid_ds	*qp;	/* ptr to associated q slot */
	register int index;

	/*
	 * This is a two lock scheme:  lk_msgq keeps queues from being
	 * created and destroyed.  msg_lk locks an individual queue.
	 * Hold the global lock while finding the instantiation, then
	 * release the global lock while still holding the specific q.
	 */
	smp_lock(&lk_msgq, LK_RETRY);

	index = id % msginfo.msgmni;
	if (index < 0) {	/* check for negitive index */
		u.u_error = EINVAL;
		qp = NULL;
	} else {
		qp = &msgque[index];
		smp_lock(&qp->msg_lk, LK_RETRY); 
		if((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
			id / msginfo.msgmni != qp->msg_perm.seq) {
			u.u_error = EINVAL;
			smp_unlock(&qp->msg_lk); /* unlock on error */
			qp = NULL;
		}
	}
	smp_unlock(&lk_msgq);
	return(qp);
}

/*
**	msgctl - Msgctl system call.
*/

msgctl()
{
	register struct a {
		int		msgid,
				cmd;
		struct msqid_ds	*buf;
	}		*uap = (struct a *)u.u_ap;
	struct msqid_ds			ds;	/* queue work area */
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int nbytes;

	if((qp = msgconv(uap->msgid)) == NULL)
		return;
	u.u_r.r_val1 = 0;
	switch (uap->cmd) {
	case IPC_RMID:
		if(u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
			&& !suser())
			break;
		while(qp->msg_first)
			msgfree(qp, NULL, qp->msg_first);
		qp->msg_cbytes = 0;
		if(uap->msgid + msginfo.msgmni < 0)
			qp->msg_perm.seq = 0;
		else
			qp->msg_perm.seq++;
		if(qp->msg_perm.mode & MSG_RWAIT)
			wakeup(&qp->msg_qnum);
		if(qp->msg_perm.mode & MSG_WWAIT)
			wakeup(qp);
		/*
		 * the ipcget routine could allocate this structure
		 * before we have unlocked it, but no one will be able
		 * to get to it until we are done, so this is OK.
		 */
		qp->msg_perm.mode = 0;
		break;
	case IPC_SET:
		if(u.u_uid != qp->msg_perm.uid 
			&& u.u_uid != qp->msg_perm.cuid && !suser())
				break;
		if(copyin(uap->buf, &ds, sizeof(ds))) {
			u.u_error = EFAULT;
			break;
		}
		if(ds.msg_qbytes > qp->msg_qbytes && !suser())
			break;
		qp->msg_perm.uid = ds.msg_perm.uid;
		qp->msg_perm.gid = ds.msg_perm.gid;
		qp->msg_perm.mode = (qp->msg_perm.mode & ~0777) |
			(ds.msg_perm.mode & 0777);
		qp->msg_qbytes = ds.msg_qbytes;
		qp->msg_ctime = time.tv_sec;
		break;
	case IPC_STAT:
		if(ipcaccess(&qp->msg_perm, MSG_R))
			break;
		if(copyout(qp, uap->buf, (sizeof(*qp) - sizeof(qp->msg_lk))))
			u.u_error = EFAULT;
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
	smp_unlock(&qp->msg_lk);
}

/*
**	msgfree - Free up space and message header, relink pointers on q,
**	and wakeup anyone waiting for resources.
**
**	This routine must be called with the qp LOCKED by the caller
**	We return it in its locked state.
*/

msgfree(qp, pmp, mp)
register struct msqid_ds	*qp;	/* ptr to q of mesg being freed */
register struct msg		*mp,	/* ptr to msg being freed */
				*pmp;	/* ptr to mp's predecessor */
{
#ifdef SMP_DEBUG
	if (smp_debug) lsert(&qp->msg_lk, "msgfree");
#endif SMP_DEBUG
	/* Unlink message from the q. */
	if(pmp == NULL)
		qp->msg_first = mp->msg_next;
	else
		pmp->msg_next = mp->msg_next;
	if(mp->msg_next == NULL)
		qp->msg_last = pmp;
	qp->msg_qnum--;
	if(qp->msg_perm.mode & MSG_WWAIT) {
		qp->msg_perm.mode &= ~MSG_WWAIT;
		wakeup(qp);
	}

	smp_lock(&lk_msgtxt, LK_RETRY);
	/* Free up message text. */
	if(mp->msg_ts)	{
		rmfree(msgmap, btoq(mp->msg_ts), mp->msg_spot + 1);
		if(msginfo.msgwnt) {
		    msginfo.msgwnt = 0;
		    wakeup(&msginfo.msgwnt);
		}
	}
	/* Free up header */
	mp->msg_next = msgfp;
	msgfp = mp;
	if (msgfp->msg_next == NULL)
		wakeup(&msgfp);
	smp_unlock(&lk_msgtxt);
}

/*
**	msgget - Msgget system call.
*/

msgget()
{
	register struct a {
		long	key;
		int	msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int				s;	/* ipcget status return */

	smp_lock(&lk_msgq, LK_RETRY);
	if((qp = ipcget(uap->key, uap->msgflg, msgque, msginfo.msgmni, 
	  sizeof(*qp), &s))) {
		smp_lock(&qp->msg_lk, LK_RETRY);
		if(s) {
			/* This is a new queue.  Finish initialization. */
			qp->msg_first = qp->msg_last = NULL;
			qp->msg_qnum = 0;
			qp->msg_qbytes = msginfo.msgmnb;
			qp->msg_lspid = qp->msg_lrpid = 0;
			qp->msg_stime = qp->msg_rtime = 0;
			qp->msg_ctime = time.tv_sec;
		}
		u.u_r.r_val1 = qp->msg_perm.seq * msginfo.msgmni + (qp - msgque);
		smp_unlock(&qp->msg_lk);
	}
	smp_unlock(&lk_msgq);
}

/*
**	msginit - Called by main(main.c) to initialize message queues.
*/

msginit()
{
	register int		i;	/* loop control */
	register struct msg	*mp;	/* ptr to msg begin linked */

	rminit(msgmap, msginfo.msgseg, 1, "msgmap", msginfo.msgmap);
	for(i = 0, mp = msgfp = msgh;++i < msginfo.msgtql;mp++)
		mp->msg_next = mp + 1;

	lockinit(&lk_msgq, &lock_msgq_d);
	lockinit(&lk_msgtxt, &lock_msgtxt_d);
	for(i=0; i < msginfo.msgmni; i++)
		lockinit(&msgque[i].msg_lk, &lock_msg_d);
}

/*
**	msgrcv - Msgrcv system call.
*/

msgrcv()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		long		msgtyp;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct nameidata *ndp = &u.u_nd;
	register struct msg		*mp,	/* ptr to msg on q */
					*pmp,	/* ptr to mp's predecessor */
					*smp,	/* ptr to best msg on q */
					*spmp;	/* ptr to smp's predecessor */
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int				sz;	/* transfer byte count */

	if((qp = msgconv(uap->msqid)) == NULL)
		return;

	if(ipcaccess(&qp->msg_perm, MSG_R))
		goto exit;
	if(uap->msgsz < 0) {
		u.u_error = EINVAL;
		goto exit;
	}
	smp = spmp = NULL;
findmsg:
	pmp = NULL;
	mp = qp->msg_first;
	if(uap->msgtyp == 0)
		smp = mp;
	else
		for(;mp;pmp = mp, mp = mp->msg_next) {
			if(uap->msgtyp > 0) {
				if(uap->msgtyp != mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
				break;
			}
			if(mp->msg_type <= -uap->msgtyp) {
				if(smp && smp->msg_type <= mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
			}
		}
	if(smp) {
		if(uap->msgsz < smp->msg_ts)
			if(!(uap->msgflg & MSG_NOERROR)) {
				u.u_error = E2BIG;
				goto exit;
			} else
				sz = uap->msgsz;
		else
			sz = smp->msg_ts;
		u.u_error = copyout(&smp->msg_type, uap->msgp, 
		  sizeof(smp->msg_type));
		if(u.u_error)
			goto exit;
		if(sz) {
			ndp->ni_segflg = UIO_USERSPACE;
			u.u_error = copyout(ipcmsgbuf+
			 (msginfo.msgssz*smp->msg_spot),
			 (caddr_t)uap->msgp + sizeof(smp->msg_type), sz);
			if(u.u_error)
				goto exit;
		}
		u.u_r.r_val1 = sz;
		qp->msg_cbytes -= smp->msg_ts;
		qp->msg_lrpid = u.u_procp->p_pid;
		qp->msg_rtime = time.tv_sec;
		msgfree(qp, spmp, smp);
		goto exit;
	}
	if(uap->msgflg & IPC_NOWAIT) {
		u.u_error = ENOMSG;
		goto exit;
	}
	qp->msg_perm.mode |= MSG_RWAIT;
	sleep_unlock(&qp->msg_qnum, PMSG, &qp->msg_lk);
	if(msgconv(uap->msqid) == NULL) {
		u.u_error = EIDRM;
		return;
	}
	goto findmsg;
exit:
	smp_unlock(&qp->msg_lk);
	return;
}


/*
**	msgsnd - Msgsnd system call.
*/

msgsnd()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	register struct msg		*mp;	/* ptr to allocated msg hdr */
	register int			cnt,	/* byte count */
					spot;	/* msg pool allocation spot */
	long				type;	/* msg type */



	if((qp = msgconv(uap->msqid)) == NULL)
		return;
	if(ipcaccess(&qp->msg_perm, MSG_W)) {
		smp_unlock(&qp->msg_lk);
		return;
	}
	smp_unlock(&qp->msg_lk);
	u.u_error = copyin(uap->msgp, &type, sizeof(type));
	if (u.u_error)
		return;
	if((cnt = uap->msgsz) < 0 || cnt > msginfo.msgmax
	    || type < 1) {
		u.u_error = EINVAL;
		return;
	}

getres:
	/* Be sure that q has not been removed. */
	if(msgconv(uap->msqid) == NULL) {
		u.u_error = EIDRM;
		return;
	}

	/* Allocate space on q, message header, & buffer space. */
	if(cnt + qp->msg_cbytes > qp->msg_qbytes) {
		if(uap->msgflg & IPC_NOWAIT) {
			smp_unlock(&qp->msg_lk);
			u.u_error = EAGAIN;
			return;
		}
		qp->msg_perm.mode |= MSG_WWAIT;
		if(sleep_unlock(qp, PMSG | PCATCH, &qp->msg_lk)) {
			u.u_error = EINTR;
			smp_lock(&qp->msg_lk, LK_RETRY);
			if (qp->msg_perm.mode & MSG_WWAIT) {
				qp->msg_perm.mode &= ~MSG_WWAIT;
				wakeup(qp);
			}
			smp_unlock(&qp->msg_lk);
			return;
		}
		goto getres;
	}
	smp_lock(&lk_msgtxt, LK_RETRY);
	if(msgfp == NULL) {
		if(uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			smp_unlock(&lk_msgtxt);
			smp_unlock(&qp->msg_lk);
			return;
		}
		smp_unlock(&qp->msg_lk);
		sleep_unlock(&msgfp, PMSG, &lk_msgtxt);
		goto getres;
	}
	if(cnt && (spot = rmalloc(msgmap, btoq(cnt))) == NULL) {
		if(uap->msgflg & IPC_NOWAIT) {
			smp_unlock(&lk_msgtxt);
			smp_unlock(&qp->msg_lk);
			u.u_error = EAGAIN;
			return;
		}
		msginfo.msgwnt++;
		smp_unlock(&qp->msg_lk);
		sleep_unlock(&msginfo.msgwnt, PMSG, &lk_msgtxt);
		goto getres;
	}

	/* Everything is available, copy in text and put msg on q.
	 * However, we could fault so remove text structure from free queue
	 * and allow other accesses.
         */

	mp = msgfp;
	msgfp = mp->msg_next;
	smp_unlock(&lk_msgtxt);

	if(cnt) {
		u.u_error = copyin( (caddr_t)uap->msgp + sizeof(type), 
		  ipcmsgbuf + (msginfo.msgssz * --spot), cnt );
		if(u.u_error) {
			smp_lock(&lk_msgtxt, LK_RETRY);
			rmfree(msgmap, btoq(cnt), spot + 1);
			mp->msg_next = msgfp;
			msgfp = mp;
			if (msgfp->msg_next == NULL)
				wakeup(&msgfp);
			if (msginfo.msgwnt) {
			    msginfo.msgwnt = 0;
			    wakeup(&msginfo.msgwnt);
			}
			smp_unlock(&lk_msgtxt);
			smp_unlock(&qp->msg_lk);
			return;
		}
	}

	qp->msg_qnum++;
	qp->msg_cbytes += cnt;
	qp->msg_lspid = u.u_procp->p_pid;
	qp->msg_stime = time.tv_sec;
	mp->msg_next = NULL;
	mp->msg_type = type;
	mp->msg_ts = cnt;
	mp->msg_spot = cnt ? spot : -1;
	if(qp->msg_last == NULL)
		qp->msg_first = qp->msg_last = mp;
	else {
		qp->msg_last->msg_next = mp;
		qp->msg_last = mp;
	}
	if(qp->msg_perm.mode & MSG_RWAIT) {
		qp->msg_perm.mode &= ~MSG_RWAIT;
		wakeup(&qp->msg_qnum);
	}
	smp_unlock(&qp->msg_lk);
	u.u_r.r_val1 = 0;
}
