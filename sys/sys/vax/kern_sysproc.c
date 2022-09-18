#ifndef lint
static	char	*sccsid = "@(#)kern_sysproc.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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
 * Modification History: /sys/sys/kern_sysproc.c
 *
 *	Joe Amato - 12/14/87
 *		Added new KM_ALLOC/KM_FREE macros
 *
 *	John Forecast	- 08/06/86
 *		Clear the sysproc structure since km_alloc won't do it
 *		for us with large allocations
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/sysproc.h"
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/kmalloc.h"

#ifdef vax
#include "../machine/mtpr.h"	/* XXX */
#endif

caddr_t interruptstate;
struct syspq sysproc = { &sysproc, &sysproc };
struct syspq slpsysproc = { &slpsysproc, &slpsysproc };

/*
 * Create a new system process.
 */
struct sysproc *createsysproc(func,arg)
int (*func)();
caddr_t arg;
{
	int s;
	register struct sysproc *sp;

	KM_ALLOC(sp, struct sysproc *, sizeof(struct sysproc), KM_SYSPROC, KM_CLEAR);
	if (sp) {
		bzero(sp, sizeof(struct sysproc));
		sp->sp_cred.cr_ref = 1;
		sp->sp_func = func;
		sp->sp_args = arg;
		sp->sp_state = SPS_INIT;
		Initsysproc((caddr_t)sp + sizeof(struct sysproc), &sp->sp_savestate);
		s = spl6();
		sp->sp_state = SPS_ACTIVE;
		insque(sp, sysproc.sq_back);
		setsysproc();
		splx(s);
	}
	return (sp);
}

/*
 * Execute a system process
 */
Runsysproc()
{
	(*cusysproc->sp_func)(cusysproc, cusysproc->sp_args);
	cusysproc->sp_state = SPS_DEAD;
	Swtchsysproc(interruptstate, &cusysproc->sp_savestate);
}

/*
 * Interrupt service routine for system process execution.
 */
sysprocintr()
{
	while ((cusysproc = (struct sysproc *)sysproc.sq_forw) != (struct sysproc *)&sysproc) {
		remque(cusysproc);
		Swtchsysproc(cusysproc->sp_savestate, &interruptstate);
		/*
		 * If the system process is now completed, destroy it.
		 */
		if (cusysproc->sp_state == SPS_DEAD)
			KM_FREE(cusysproc, KM_SYSPROC);
	}
	cusysproc = 0;
}

/*
 * Place a system process to sleep on a channel.
 */
sleepsysproc(chan)
caddr_t chan;
{
	int s = spl6();

	if (chan == 0 || cusysproc->sp_state != SPS_ACTIVE)
		panic("sleepsysproc");
	cusysproc->sp_wchan = chan;
	cusysproc->sp_state = SPS_SLEEP;
	insque(cusysproc, slpsysproc.sq_back);
	Swtchsysproc(interruptstate, &cusysproc->sp_savestate);
	splx(s);
}

/*
 * Wakeup system processes.
 */
wakeupsysproc(chan)
caddr_t chan;
{
	register struct sysproc *sp = (struct sysproc *)slpsysproc.sq_forw,*np;

	while (sp != (struct sysproc *)&slpsysproc) {
		np = (struct sysproc *)sp->sp_queue.sq_forw;
		if (sp->sp_wchan == chan) {
			remque(sp);
			sp->sp_state = SPS_ACTIVE;
			insque(sp, sysproc.sq_back);
			setsysproc();
		}
		sp = np;
	}
}

/*
 * Find I/O device table entry for current system process.
 */
findsysprocio()
{
	register int i;

	for (i = 0; i < SYSMAXIO; i++)
		if (cusysproc->sp_io[i] == 0)
			return (i);
	return (-1);
}
