#ifndef lint
static char *sccsid = "@(#)uipc_sem.c	4.2	ULTRIX	11/9/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988 by			*
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
 *	uipc_sem.c
 *
 *   Source file description:
 *
 *	This file contains the system calls and support functions to implement
 *	System V semphores.
 *
 *   System Calls:
 *
 *
 *	semctl 		Provides semaphore control to applications
 *
 *	semget 		Gets a semaphore, or array of semaphores for
 *			application use.
 *
 *	semop 		Provides semaphore operations.
 *
 *
 *   Functions:
 *
 *	semaoe 		Create or update adjust on exit entry.
 *
 *	semconv 	Convert user supplied semid into a ptr to the associated
 *			semaphore header.
 *
 *	semexit 	Called by exit(sys1.c) to clean up on process exit.
 *
 *	seminit 	Called by main(main.c) to initialize the semaphore map.
 *
 *	semundo 	Undo work done up to finding an operation that can't 
 *			be done.
 *
 *	semunrm 	Undo entry remover.
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
 * 09-Nov-89 -- jaw
 *	fix smp locking in semconv and protect against negitive index.
 *
 * 25 Jul 89 -- chet
 *	Change declaration of time
 *
 * 18 May 89 - jaw
 *	fix merge problems 
 *
 * 16 Aug 88 - miche
 * 	Add SMP support
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 12 Feb 85 -- depp
 *	New file
 *
 */

/*
**	Inter-Process Communication Semaphore Facility.
*/

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/map.h"
#include "../h/errno.h"
#include "../h/signal.h"
#include "../h/ipc.h"
#include "../h/sem.h"
#include "../h/user.h"
#include "../h/seg.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/kmalloc.h"

extern struct semid_ds	sema[];		/* semaphore data structures */
extern struct sem	sem[];		/* semaphores */
extern struct map	semmap[];	/* sem allocation map */
extern struct sem_undo	*sem_undo[];	/* undo table pointers */
extern struct sem_undo	semu[];		/* operation adjust on exit table */
extern struct seminfo seminfo;		/* param information structure */
extern union semtmp {
	ushort			semvals[1]; /* set semaphore values */
	struct	semid_ds	ds;	/* set permission values */
	struct sembuf		semops[1];	/* operation holding area */
} ;
extern int semtmp_size;			/* size of the semtmp union */
struct sem_undo	*semunp;		/* ptr to head of undo chain */
struct sem_undo	*semfup;		/* ptr to head of free undo chain */

struct lock_t lk_semq;			/* global semaphore resources */
struct lock_t lk_semu;			/* global undo information */

extern struct timeval time;		/* system idea of date */

struct semid_ds	*ipcget(),
		*semconv();

/*
**	semaoe - Create or update adjust on exit entry.
*/

semaoe(val, id, num)
short	val,	/* operation value to be adjusted on exit */
	num;	/* semaphore # */
int	id;	/* semid */
{
	register struct undo		*uup,	/* ptr to entry to update */
					*uup2;	/* ptr to move entry */
	register struct sem_undo	*up,	/* ptr to process undo struct */
					*up2;	/* ptr to undo list */
	register int			i,	/* loop control */
					found;	/* matching entry found flag */

	/*
	 * There's nothing to test here, but we do count on the
	 * underlying semaphore being locked
	 */
	if(val == 0)
		return(0);
	if(val > seminfo.semaem || val < -seminfo.semaem) {
		u.u_error = ERANGE;
		return(1);
	}
	smp_lock(&lk_semu, LK_RETRY);
	if((up = sem_undo[u.u_procp - proc]) == NULL)
		if (up = semfup) {
			semfup = up->un_np;
			up->un_np = NULL;
			sem_undo[u.u_procp - proc] = up;
		} else {
			u.u_error = ENOSPC;
			smp_unlock(&lk_semu);
			return(1);
		}
	for(uup = up->un_ent, found = i = 0;i < up->un_cnt;i++) {
		if(uup->un_id < id || (uup->un_id == id && uup->un_num < num)) {
			uup++;
			continue;
		}
		if(uup->un_id == id && uup->un_num == num)
			found = 1;
		break;
	}
	if(!found) {
		if(up->un_cnt >= seminfo.semume) {
			u.u_error = EINVAL;
			smp_unlock(&lk_semu);
			return(1);
		}
		if(up->un_cnt == 0) {
			up->un_np = semunp;
			semunp = up;
		}
		uup2 = &up->un_ent[up->un_cnt++];
		while(uup2-- > uup)
			*(uup2 + 1) = *uup2;
		uup->un_id = id;
		uup->un_num = num;
		uup->un_aoe = -val;
		smp_unlock(&lk_semu);
		return(0);
	}
	uup->un_aoe -= val;
	if(uup->un_aoe > seminfo.semaem || uup->un_aoe < -seminfo.semaem) {
		u.u_error = ERANGE;
		uup->un_aoe += val;
		smp_unlock(&lk_semu);
		return(1);
	}
	if(uup->un_aoe == 0) {
		uup2 = &up->un_ent[--(up->un_cnt)];
		while(uup++ < uup2)
			*(uup - 1) = *uup;
		if(up->un_cnt == 0) {

			/* Remove process from undo list. */
			if(semunp == up)
				semunp = up->un_np;
			else
				for(up2 = semunp;up2 != NULL;up2 = up2->un_np)
					if(up2->un_np == up) {
						up2->un_np = up->un_np;
						break;
					}
			up->un_np = NULL;
		}
	}
	smp_unlock(&lk_semu);
	return(0);
}

/*
**	semconv - Convert user supplied semid into a ptr to the associated
**		semaphore header.
*/

struct semid_ds *
semconv(s)
register int	s;	/* semid */
{
	/*
	 * This is a two lock scheme:  lk_semq keeps queues from being
	 * created and destroyed.  sem_lk locks an individual queue.
	 * Hold the global lock while finding the instantiation, then
	 * release the global lock while still holding the specific q.
	 */
	register struct semid_ds	*sp;	/* ptr to associated header */
	register int index;

	smp_lock(&lk_semq, LK_RETRY);
	index = s % seminfo.semmni;

	if (index < 0) {	/* check for negitive index */
			u.u_error = EINVAL;
			sp = NULL;
	} else {
		sp = &sema[index];
		smp_lock(&sp->sem_lk, LK_RETRY);
		if((sp->sem_perm.mode & IPC_ALLOC) == 0 ||
			s / seminfo.semmni != sp->sem_perm.seq) {
			smp_unlock(&sp->sem_lk);  /* release on on error */
			u.u_error = EINVAL;
			sp = NULL;
		}
	}
	smp_unlock(&lk_semq);
	return(sp);
}

/*
**	semctl - Semctl system call.
*/

semctl()
{
	register struct a {
		int	semid;
		u_int	semnum;
		int	cmd;
		int	arg;
	}	*uap = (struct a *)u.u_ap;
	register struct	semid_ds	*sp;	/* ptr to semaphore header */
	register struct sem		*p;	/* ptr to semaphore */
	register int			i;	/* loop control */
	short 	 *outaddr;			/* copyout address */
	union semtmp *semtmp;
	int nbytes;

	if((sp = semconv(uap->semid)) == NULL)
		return;
	u.u_r.r_val1 = 0;
	switch(uap->cmd) {

	/* Remove semaphore set. */
	case IPC_RMID:
		if(u.u_uid != sp->sem_perm.uid && u.u_uid != sp->sem_perm.cuid
			&& !suser())
			break;
		semunrm(uap->semid, 0, sp->sem_nsems);
		for(i = sp->sem_nsems, p = sp->sem_base;i--;p++) {
			p->semval = p->sempid = 0;
			if(p->semncnt) {
				wakeup(&p->semncnt);
				p->semncnt = 0;
			}
			if(p->semzcnt) {
				wakeup(&p->semzcnt);
				p->semzcnt = 0;
			}
		}
		rmfree(semmap, sp->sem_nsems, (sp->sem_base - sem) + 1);
		if(uap->semid + seminfo.semmni < 0)
			sp->sem_perm.seq = 0;
		else
			sp->sem_perm.seq++;
		sp->sem_perm.mode = 0;
		break;
	/* Set ownership and permissions. */
	case IPC_SET:
		if(u.u_uid != sp->sem_perm.uid && u.u_uid != sp->sem_perm.cuid
			 && !suser())
			break;
		KM_ALLOC(semtmp, union semtmp *, semtmp_size,
			 KM_TEMP, KM_NOARG);
		if(copyin(uap->arg, &semtmp->ds, sizeof(semtmp->ds))) {
			KM_FREE(semtmp, KM_TEMP);
			u.u_error = EFAULT;
			break;
		}
		sp->sem_perm.uid = semtmp->ds.sem_perm.uid;
		sp->sem_perm.gid = semtmp->ds.sem_perm.gid;
		sp->sem_perm.mode = semtmp->ds.sem_perm.mode & 0777 | IPC_ALLOC;
		sp->sem_ctime = time.tv_sec;
		KM_FREE(semtmp, KM_TEMP);
		break;

	/* Get semaphore data structure. */
	case IPC_STAT:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if(copyout(sp, uap->arg, (sizeof(*sp)-sizeof(sp->sem_lk)))) {
			u.u_error = EFAULT;
			break;
		}
		break;

	/* Get # of processes sleeping for greater semval. */
	case GETNCNT:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_r.r_val1 = (sp->sem_base + uap->semnum)->semncnt;
		break;

	/* Get pid of last process to operate on semaphore. */
	case GETPID:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_r.r_val1 = (sp->sem_base + uap->semnum)->sempid;
		break;

	/* Get semval of one semaphore. */
	case GETVAL:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_r.r_val1 = (sp->sem_base + uap->semnum)->semval;
		break;

	/* Get all semvals in set. */
	case GETALL:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			break;
		outaddr = (short *)uap->arg;
		for(i = sp->sem_nsems, p = sp->sem_base;i--;p++,outaddr++) {
			u.u_error = copyout((long)&p->semval, 
			  outaddr, sizeof(p->semval));
			if(u.u_error)
				break;
		}
		break;

	/* Get # of processes sleeping for semval to become zero. */
	case GETZCNT:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_r.r_val1 = (sp->sem_base + uap->semnum)->semzcnt;
		break;

	/* Set semval of one semaphore. */
	case SETVAL:
		if(ipcaccess(&sp->sem_perm, SEM_A))
			break;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		if((unsigned)uap->arg > seminfo.semvmx) {
			u.u_error = ERANGE;
			break;
		}
		if((p = sp->sem_base + uap->semnum)->semval = uap->arg) {
			if(p->semncnt) {
				p->semncnt = 0;
				wakeup(&p->semncnt);
			}
		} else
			if(p->semzcnt) {
				p->semzcnt = 0;
				wakeup(&p->semzcnt);
			}
		p->sempid = u.u_procp->p_pid;
		semunrm(uap->semid, uap->semnum, uap->semnum);
		break;

	/* Set semvals of all semaphores in set. */
	case SETALL:
		if(ipcaccess(&sp->sem_perm, SEM_A))
			break;
		KM_ALLOC(semtmp, union semtmp *, semtmp_size,
			 KM_TEMP, KM_NOARG);
		u.u_error = copyin(uap->arg, (long)semtmp->semvals,
			sizeof(semtmp->semvals[0]) * sp->sem_nsems);
		if(u.u_error) {
			KM_FREE(semtmp, KM_TEMP);
			break;
		}
		for(i = 0;i < sp->sem_nsems;)
			if(semtmp->semvals[i++] > seminfo.semvmx) {
				u.u_error = ERANGE;
				break; /* breaks out of the for() */
			}
		if(u.u_error) {
			KM_FREE(semtmp, KM_TEMP);
			break; /* breaks out of the switch */
		}
		semunrm(uap->semid, 0, sp->sem_nsems);
		for(i = 0, p = sp->sem_base;i < sp->sem_nsems;
			(p++)->sempid = u.u_procp->p_pid) {
			if(p->semval = semtmp->semvals[i++]) {
				if(p->semncnt) {
					p->semncnt = 0;
					wakeup(&p->semncnt);
				}
			} else
				if(p->semzcnt) {
					p->semzcnt = 0;
					wakeup(&p->semzcnt);
				}
		}
		KM_FREE(semtmp, KM_TEMP);
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
	smp_unlock(&sp->sem_lk);
	return;
}

/*
**	semexit - Called by exit(sys1.c) to clean up on process exit.
*/

semexit()
{
	register struct sem_undo	*up,	/* process undo struct ptr */
					*p;	/* undo struct ptr */
	register struct semid_ds	*sp;	/* semid being undone ptr */
	register int			i;	/* loop control */
	register long			v;	/* adjusted value */
	register struct sem		*semp;	/* semaphore ptr */

	if((up = sem_undo[u.u_procp - proc]) == NULL)
		return;
	if(up->un_cnt == 0) {
		smp_lock(&lk_semu, LK_RETRY);
		goto cleanup;
	}
	for(i = up->un_cnt;i--;) {
		if((sp = semconv(up->un_ent[i].un_id)) == NULL)
			continue;
		v = (long)(semp = sp->sem_base + up->un_ent[i].un_num)->semval +
			up->un_ent[i].un_aoe;
		if(v < 0 || v > seminfo.semvmx)
			/*continue after unlock*/ ;
		else {
			semp->semval = v;
			if(v == 0 && semp->semzcnt) {
				semp->semzcnt = 0;
				wakeup(&semp->semzcnt);
			}
			if(up->un_ent[i].un_aoe > 0 && semp->semncnt) {
				semp->semncnt = 0;
				wakeup(&semp->semncnt);
			}
		}
		smp_unlock(&sp->sem_lk);
	}
	smp_lock(&lk_semu, LK_RETRY);
	up->un_cnt = 0;
	if(semunp == up)
		semunp = up->un_np;
	else
		for(p = semunp;p != NULL;p = p->un_np)
			if(p->un_np == up) {
				p->un_np = up->un_np;
				break;
			}

cleanup:
	up->un_np = semfup;
	semfup = up;
	sem_undo[u.u_procp - proc] = NULL;
	smp_unlock(&lk_semu);
}

/*
**	semget - Semget system call.
*/

semget()
{
	register struct a {
		long	key;
		int	nsems;
		int	semflg;
	}	*uap = (struct a *)u.u_ap;
	register struct semid_ds	*sp;	/* semaphore header ptr */
	register int			i;	/* temp */
	int				s;	/* ipcget status return */

	smp_lock(&lk_semq, LK_RETRY);
	if(sp = ipcget(uap->key, uap->semflg, sema, seminfo.semmni, sizeof(*sp), &s)) {
		smp_lock(&sp->sem_lk, LK_RETRY);
		if(s) {
			/* This is a new semaphore set.  Finish initialization. */
			if(uap->nsems <= 0 || uap->nsems > seminfo.semmsl) {
				u.u_error = EINVAL;
				sp->sem_perm.mode = 0;
			} else if((i = rmalloc(semmap, uap->nsems)) == NULL) {
				u.u_error = ENOSPC;
				sp->sem_perm.mode = 0;
			} else {
				sp->sem_base = sem + (i - 1);
				sp->sem_nsems = uap->nsems;
				sp->sem_ctime = time.tv_sec;
				u.u_r.r_val1 = sp->sem_perm.seq * seminfo.semmni + (sp - sema);
			}
		} else {
			if(uap->nsems && sp->sem_nsems < uap->nsems) {
				u.u_error = EINVAL;
			} else	u.u_r.r_val1 = sp->sem_perm.seq * seminfo.semmni + (sp - sema);
		}
		smp_unlock(&sp->sem_lk);
	}
	smp_unlock(&lk_semq);
}

/*
**	seminit - Called by main(main.c) to initialize the semaphore map.
*/

seminit()
{
	register int i;

	rminit(semmap, seminfo.semmns, 1, "semmap", seminfo.semmap);

	semfup = semu;
	for (i = 0; i < seminfo.semmnu - 1; i++) {
		semfup->un_np = (struct sem_undo *)((u_int)semfup+seminfo.semusz);
		semfup = semfup->un_np;
	}
	semfup->un_np = NULL;
	semfup = semu;

	lockinit(&lk_semq, &lock_semq_d);
	lockinit(&lk_semu, &lock_semu_d);
	for (i=0 ; i < seminfo.semmni; i++)
		lockinit(&sema[i].sem_lk, &lock_sem_d);

}

/*
**	semop - Semop system call.
*/

semop()
{
	register struct a {
		int		semid;
		struct sembuf	*sops;
		u_int		nsops;
	}	*uap = (struct a *)u.u_ap;
	register struct sembuf		*op;	/* ptr to operation */
	register int			i;	/* loop control */
	register struct semid_ds	*sp;	/* ptr to associated header */
	register struct sem		*semp;	/* ptr to semaphore */
	int	again;
	union semtmp *semtmp;

	if((sp = semconv(uap->semid)) == NULL)
		return;
	KM_ALLOC(semtmp, union semtmp *, semtmp_size,
		 KM_TEMP, KM_NOARG);
	if(uap->nsops > seminfo.semopm) {
		u.u_error = E2BIG;
		goto bad;
	}
	u.u_error = copyin((caddr_t)uap->sops, (long)semtmp->semops, 
	  uap->nsops * sizeof(*op) );
	if(u.u_error)
		goto bad;

	/* Verify that sem #s are in range and permissions are granted. */
	for(i = 0, op = semtmp->semops;i++ < uap->nsops;op++) {
		if(ipcaccess(&sp->sem_perm, op->sem_op ? SEM_A : SEM_R))
			goto bad;
		if(op->sem_num >= sp->sem_nsems) {
			u.u_error = EFBIG;
			goto bad;
		}
	}
	again = 0;
check:
	/* Loop waiting for the operations to be satisified atomically. */
	/* Actually, do the operations and undo them if a wait is needed
		or an error is detected. */
	if (again) {
		/* Verify that the semaphores haven't been removed.
		 * semconv returns with sp->sem_lk held if successful */
		if(semconv(uap->semid) == NULL) {
			KM_FREE(semtmp, KM_TEMP);
			u.u_error = EIDRM;
			return;
		}
		/* no need to reget ops since semtmp is local */
	}
	again = 1;

	for(i = 0, op = semtmp->semops;i < uap->nsops;i++, op++) {
		semp = sp->sem_base + op->sem_num;
		if(op->sem_op > 0) {
			/* give back semop units */
			if(op->sem_op + (long)semp->semval > seminfo.semvmx ||
				(op->sem_flg & SEM_UNDO &&
				semaoe(op->sem_op, uap->semid, op->sem_num))) {
				if(u.u_error == 0)
					u.u_error = ERANGE;
				if(i)
					semundo(semtmp->semops,i,uap->semid,sp);
				goto bad;
			}
			semp->semval += op->sem_op;
			/* ncount waiting for val > n */
			semp->semnwakup = semp->semncnt;
			continue;
		}
		if(op->sem_op < 0) {
			/* try to take semop units */
			if(semp->semval >= -op->sem_op) {
				if(op->sem_flg & SEM_UNDO && semaoe(op->sem_op,
				   uap->semid, op->sem_num)) {
					if(i)
						semundo(semtmp->semops, i, 
						  uap->semid, sp);
					goto bad;
				}
				semp->semval += op->sem_op;
				if(semp->semval == 0 && semp->semzcnt) {
					semp->semzcnt = 0;
					wakeup(&semp->semzcnt);
				}
				continue;
			}
			if(i)
				semundo(semtmp->semops, i, uap->semid, sp);
			if(op->sem_flg & IPC_NOWAIT) {
				u.u_error = EAGAIN;
				goto bad;
			}
			semp->semncnt++;
			if(sleep_unlock(&semp->semncnt, PCATCH | PSEMN, &sp->sem_lk)) {
				smp_lock(&sp->sem_lk, LK_RETRY);
				if((semp->semncnt)-- <= 1) {
					semp->semncnt = 0;
					wakeup(&semp->semncnt);
				}
				u.u_error = EINTR;
				goto bad;
			}
			goto check;
		}
		if(semp->semval) {
			/* wait for semval to be zero */
			if(i)
				semundo(semtmp->semops, i, uap->semid, sp);
			if(op->sem_flg & IPC_NOWAIT) {
				u.u_error = EAGAIN;
				goto bad;
			}
			semp->semzcnt++;
			if(sleep_unlock(&semp->semzcnt, PCATCH | PSEMZ, &sp->sem_lk)) {
				smp_lock(&sp->sem_lk, LK_RETRY);
				if((semp->semzcnt)-- <= 1) {
					semp->semzcnt = 0;
					wakeup(&semp->semzcnt);
				}
				u.u_error = EINTR;
				goto bad;
			}
			goto check;
		}
	}

	/* All operations succeeded.  Update sempid for accessed semaphores. */
	for(i = 0, op = semtmp->semops;i++ < uap->nsops;
		(sp->sem_base + (op++)->sem_num)->sempid = u.u_procp->p_pid);
	sp->sem_otime = time.tv_sec;

	/* wakeup the sleepers on semncnt */
	for(i = 0, op = semtmp->semops;i < uap->nsops;i++, op++) {
		semp = sp->sem_base + op->sem_num;
		if (semp->semnwakup) {
			semp->semncnt = semp->semnwakup = 0;
			wakeup(&semp->semncnt);
		}
	}
	u.u_r.r_val1 = 0;
bad:
	smp_unlock(&sp->sem_lk);
	KM_FREE(semtmp, KM_TEMP);
	return;
}

/*
**	semundo - Undo work done up to finding an operation that can't be done.
*/

semundo(op, n, id, sp)
register struct sembuf		*op;	/* first operation that was done ptr */
register int			n,	/* # of operations that were done */
				id;	/* semaphore id */
register struct semid_ds	*sp;	/* semaphore data structure ptr */
{
	register struct sem	*semp;	/* semaphore ptr */

#ifdef SMP_DEBUG
	if (smp_debug)
		lsert(&sp->sem_lk, "semundo");
#endif SMP_DEBUG
	for(op += n - 1;n--;op--) {
		if(op->sem_op == 0)
			continue;
		semp = sp->sem_base + op->sem_num;
		semp->semval -= op->sem_op;
		if (semp->semnwakup) 
			semp->semnwakup = 0;
		if(op->sem_flg & SEM_UNDO)
			semaoe(-op->sem_op, id, op->sem_num);
	}
}

/*
**	semunrm - Undo entry remover.
**
**	This routine is called to clear all undo entries for a set of semaphores
**	that are being removed from the system or are being reset by SETVAL or
**	SETVALS commands to semctl.
*/

semunrm(id, low, high)
int	id;	/* semid */
ushort	low,	/* lowest semaphore being changed */
	high;	/* highest semaphore being changed */
{
	register struct sem_undo	*pp,	/* ptr to predecessor to p */
					*p;	/* ptr to current entry */
	register struct undo		*up;	/* ptr to undo entry */
	register int			i,	/* loop control */
					j;	/* loop control */

/* There is nothing to test, but the lock behind low/high is held (sem_lk) */
	pp = NULL;
	smp_lock(&lk_semu, LK_RETRY);
	p = semunp;
	while(p != NULL) {

		/* Search through current structure for matching entries. */
		for(up = p->un_ent, i = 0;i < p->un_cnt;) {
			if(id < up->un_id)
				break;
			if(id > up->un_id || low > up->un_num) {
				up++;
				i++;
				continue;
			}
			if(high < up->un_num)
				break;
			for(j = i;++j < p->un_cnt;
				p->un_ent[j - 1] = p->un_ent[j]);
			p->un_cnt--;
		}

		/* Reset pointers for next round. */
		if(p->un_cnt == 0)

			/* Remove from linked list. */
			if(pp == NULL) {
				semunp = p->un_np;
				p->un_np = NULL;
				p = semunp;
			} else {
				pp->un_np = p->un_np;
				p->un_np = NULL;
				p = pp->un_np;
			}
		else {
			pp = p;
			p = p->un_np;
		}
	}
	smp_unlock(&lk_semu);
}
