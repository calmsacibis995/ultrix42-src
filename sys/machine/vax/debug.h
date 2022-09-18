
/* 	@(#)debug.h	4.1	(ULTRIX)	7/2/90 	*/
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

#ifndef _DEBUG_
#define	_DEBUG_
#ifdef KERNEL
/*
 *	Define this NULL macro to reduce number of #ifdefs in common code.
 */
#define	XPRINTF(flags, format, arg1, arg2, arg3, arg4) /* */

#endif /* KERNEL */
/*
 * flags
 */
#define XPR_CLOCK	0x00000001	/* Clock interrupt handler */
#define XPR_TLB		0x00000002	/* TLB miss handler */
#define XPR_INIT	0x00000004	/* routines called during init */
#define XPR_SCHED	0x00000008	/* Scheduler */
#define XPR_PROCESS	0x00000010	/* newproc/fork */
#define XPR_EXEC	0x00000020	/* Exec */
#define XPR_SYSCALL	0x00000040	/* System calls */
#define XPR_TRAP	0x00000080	/* Trap handler */
#define XPR_NOFAULT	0x00000100	/* Nofault bus error */
#define XPR_VM		0x00000200	/* VM */
#define XPR_SWAP	0x00000400	/* swapin/swapout */
#define XPR_SWTCH	0x00000800	/* swtch, setrq, remrq */
#define	XPR_DISK	0x00001000	/* disk i/o */
#define	XPR_TTY		0x00002000	/* mux i/o */
#define	XPR_TAPE	0x00004000	/* tape i/o */
#define	XPR_BIO		0x00008000	/* blk i/o */
#define	XPR_INTR	0x00010000	/* interrupt handling */
#define	XPR_RMAP	0x00020000	/* resource map handling */
#define	XPR_TEXT	0x00040000	/* shared text stuff */
#define	XPR_CACHE	0x00080000	/* cache handling */
#define	XPR_NFS		0x00100000	/* nfs */
#define	XPR_RPC		0x00200000	/* rpc */
#define	XPR_SIGNAL	0x00400000	/* signal handling */
#define	XPR_FPINTR	0x00800000	/* fp interrupt handling */
#define XPR_SM          0x01000000      /* Shared memory */

#endif /* _DEBUG_ */
