/* 	@(#)sysproc.h	4.2	(ULTRIX)	9/4/90	*/

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

/* ------------------------------------------------------------------------
 * Modification History: /sys/h/sysproc.h
 *
 *
 * -----------------------------------------------------------------------
 */

/*
 * A system process always runs at interrupt level on a private stack.
 * Such processes may use the normal sleep/wakeup mechanisms for
 * synchronisation.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define NIST		3		/* # of stack pages to allocate */
					/* this should track NIST in locore.s */
#define STACK		(NIST*NBPG)	/* stack space to allocate */

#define SYSMAXIO	32		/* max # of I/O channels allowed */

struct syspq {
	struct syspq	*sq_forw;	/* forward queue pointer */
	struct syspq	*sq_back;	/* backward queue pointer */
};

struct sysproc {
	struct syspq	sp_queue;	/* queue linkage */
	caddr_t		sp_wchan;	/* event process is waiting for */
	u_short		sp_errno;	/* sysproc equivalent of errno */
	u_char		sp_state;	/* process state */
	caddr_t		sp_savestate;	/* saved process state */
	int		(*sp_func)();	/* main() for this process */
	struct ucred	sp_cred;	/* credentials */
	caddr_t		sp_io[SYSMAXIO];/* I/O device table */
	caddr_t		sp_args;	/* arguments for system process */
	u_char		sp_stack[STACK];/* system process stack */
};
#define sp_uid		sp_cred.cr_uid	/* access to uid */

#define SPS_INIT	0		/* process is initializing */
#define SPS_ACTIVE	1		/* process is active */
#define SPS_SLEEP	2		/* process is sleeping */
#define SPS_DEAD	3		/* process has completed */

#ifdef __vax
#define setsysproc()	mtpr(SIRR, 10)
#endif

#ifdef KERNEL
struct sysproc *cusysproc;
#endif
