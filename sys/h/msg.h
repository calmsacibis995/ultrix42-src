/* @(#)msg.h	4.1  (ULTRIX)        7/2/90     */

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
 *	xpg3 compliance changes
 *
 * 29 Apr 86 -- depp
 *	Moved sizing constants from /sys/h/param.h to here.
 *
 * 01 Mar 85 -- depp
 *	New file derived from System V IPC
 *
 */

#if !defined(_POSIX_SOURCE)

/*
**	IPC Message Facility.
*/

/*
**	Implementation Constants.
*/

#define	PMSG	(PZERO + 2)	/* message facility sleep priority */

/*
**	Permission Definitions.
*/

#define	MSG_R	0400	/* read permission */
#define	MSG_W	0200	/* write permission */

/*
**	ipc_perm Mode Definitions.
*/

#define	MSG_RWAIT	01000	/* a reader is waiting for a message */
#define	MSG_WWAIT	02000	/* a writer is waiting to send */

/*
**	Sizing constants
*/

#define	MSGMAP	100
#define MSGMAX	8192
#define MSGMNB	16384
#define MSGMNI	50
#define MSGSSZ	8
#define MSGTQL	40
#define MSGSEG	1024
#endif /* !defined(_POSIX_SOURCE) */

/*
**	Message Operation Flags.
*/

#define	MSG_NOERROR	010000	/* no error if big message */

/*
**	Structure Definitions.
*/

/*
**	There is one msg queue id data structure for each q in the system.
*/
#ifndef KERNEL
#include <sys/smp_lock.h>
extern int msgctl();
extern int msgget();
extern int msgrcv();
extern int msgsnd();
#endif /* KERNEL */

/*
** The ipc_perm structure must be first and the lock must be last.
*/
struct msqid_ds {
	struct ipc_perm	msg_perm;	/* operation permission struct */
	struct __msg	*msg_first;	/* ptr to first message on q */
	struct __msg	*msg_last;	/* ptr to last message on q */
	unsigned short	msg_cbytes;	/* current # bytes on q */
	unsigned short	msg_qnum;	/* # of messages on q */
	unsigned short	msg_qbytes;	/* max # of bytes on q */
	pid_t		msg_lspid;	/* pid of last msgsnd */
	pid_t		msg_lrpid;	/* pid of last msgrcv */
	time_t		msg_stime;	/* last msgsnd time */
	time_t		msg_rtime;	/* last msgrcv time */
	time_t		msg_ctime;	/* last change time */
#ifdef KERNEL
	struct __lock_t	msg_lk;		/* SMP lock for the message q */
#endif /* KERNEL */
};

/*
**	There is one msg structure for each message that may be in the system.
*/

struct __msg {
	struct __msg	*msg_next;	/* ptr to next message on q */
	long		msg_type;	/* message type */
	short		msg_ts;		/* message text size */
	short		msg_spot;	/* message text map address */
};

#if !defined(_POSIX_SOURCE)
#define msg __msg

/*
**	User message buffer template for msgsnd and msgrecv system calls.
*/

struct msgbuf {
	long	mtype;		/* message type */
	char	mtext[1];	/* message text */
};

/*
**	Message information structure.
*/

struct msginfo {
	int	msgmap,	/* # of entries in msg map */
		msgmax,	/* max message size */
		msgmnb,	/* max # bytes on queue */
		msgmni,	/* # of message queue identifiers */
		msgssz,	/* msg segment size (should be word size multiple) */
		msgtql,	/* # of system message headers */
		msgwnt; /* # of processes waiting for free map entries */
	ushort	msgseg;	/* # of msg segments (MUST BE < 32768) */
};

#endif /* !defined(_POSIX_SOURCE) */
