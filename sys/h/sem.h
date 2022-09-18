/* @(#)sem.h	4.1  (ULTRIX)        7/2/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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
 *
 *   Modification history:
 *
 * 19 Mar 90 -- burns
 *	Added ifdef kernel around SMP lock imbedded in
 *	a user visable data structure (msqid_ds).
 *
 * 13 Dec 89 -- scott
 *	xpg compliance changes
 *
 * 16 Aug 88 -- miche
 *	Add support for SMP
 *
 * 02 Apr 86 -- depp
 *	Moved sizing constants from /sys/h/param.h to here.
 *
 * 01 Mar 85 -- depp
 *	New file derived from System V IPC
 *
 */

/*
**	IPC Semaphore Facility.
*/

#ifndef KERNEL
#include <sys/smp_lock.h>
extern int semctl();
extern int semget();
extern int semop();
#endif /* KERNEL */

#if !defined(_POSIX_SOURCE)
/*
**	Implementation Constants.
*/

#define	PSEMN	(PZERO + 3)	/* sleep priority waiting for greater value */
#define	PSEMZ	(PZERO + 2)	/* sleep priority waiting for zero */

/*
**	Permission Definitions.
*/

#define	SEM_A	0200	/* alter permission */
#define	SEM_R	0400	/* read permission */

#endif /* !defined(_POSIX_SOURCE) */
/*
**	Semaphore Operation Flags.
*/

#define	SEM_UNDO	010000	/* set up adjust on exit entry */

/*
**	Semctl Command Definitions.
*/

#define	GETNCNT	3	/* get semncnt */
#define	GETPID	4	/* get sempid */
#define	GETVAL	5	/* get semval */
#define	GETALL	6	/* get all semval's */
#define	GETZCNT	7	/* get semzcnt */
#define	SETVAL	8	/* set semval */
#define	SETALL	9	/* set all semval's */

/*
**	Structure Definitions.
*/

/*
**	There is one semaphore id data structure for each set of semaphores
**		in the system. The ipc_perm structure must be first and
**		the lock must be last.
*/

struct semid_ds {
	struct ipc_perm	sem_perm;	/* operation permission struct */
	struct sem	*sem_base;	/* ptr to first semaphore in set */
	unsigned short	sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	time_t		sem_ctime;	/* last change time */
#ifdef KERNEL
	struct __lock_t	sem_lk;		/* SMP lock for the semaphore queue */
#endif /* KERNEL */
};

/*
**	There is one semaphore structure for each semaphore in the system.
*/

struct sem {
	unsigned short semval;	/* semaphore text map address */
	pid_t    sempid;	/* pid of last operation */
	unsigned short semncnt;	/* # awaiting semval > cval */
	unsigned short semzcnt;	/* # awaiting semval = 0 */
	unsigned short semnwakup;/* wake up those waiting on semncnt */
};

#if !defined(_POSIX_SOURCE)

/*
**	There is one undo structure per process in the system.
*/

struct sem_undo {
	struct sem_undo	*un_np;	/* ptr to next active undo structure */
	short		un_cnt;	/* # of active entries */
	struct undo {
		short	un_aoe;	/* adjust on exit values */
		short	un_num;	/* semaphore # */
		int	un_id;	/* semid */
	}	un_ent[1];	/* undo entries (one minimum) */
};

/*
** semaphore information structure
*/
struct	seminfo	{
	int	semmap,		/* # of entries in semaphore map */
		semmni,		/* # of semaphore identifiers */
		semmns,		/* # of semaphores in system */
		semmnu,		/* # of undo structures in system */
		semmsl,		/* max # of semaphores per id */
		semopm,		/* max # of operations per semop call */
		semume,		/* max # of undo entries per process */
		semusz,		/* size in bytes of undo structure */
		semvmx,		/* semaphore maximum value */
		semaem;		/* adjust on exit max value */
};

/*
**	User semaphore template for semop system calls.
*/

struct sembuf {
	unsigned short sem_num;	/* semaphore # */
	short	sem_op;		/* semaphore operation */
	short	sem_flg;	/* operation flags */
};

/*
 * Sizing constants
 */

#define SEMMAP 10
#define SEMMNI 10
#define SEMMNS 60
#define SEMMNU 30
#define SEMMSL 25
#define SEMOPM 10
#define	SEMUME 10
#define SEMVMX 32767
#define SEMAEM 16384

#endif /* !defined(_POSIX_SOURCE) */
