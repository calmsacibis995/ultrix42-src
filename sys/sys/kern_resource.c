#ifndef lint
static	char	*sccsid = "@(#)kern_resource.c	4.1	(ULTRIX)	7/2/90";
#endif

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

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/kern_resource.c
 *
 * 09 Dec  88 -- jaw
 *	fix up get/set priority.  They were returning incorrect values.
 *
 * 23 Aug 88 -- miche
 *	Add support for ref'ing a process.
 *
 * 07 Jun 88 - miche
 *	SMP procqs:  use FORALLPROC to find all processes
 *
 * 25 Oct 84 -- jrs
 *	Add support for linked list proc queues
 *	Derived from 4.2BSD, labeled:
 *		kern_resource.c 6.2	84/05/22
 *
 * -----------------------------------------------------------------------
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/uio.h"
#include "../h/vm.h"
#include "../h/kernel.h"

/*
 * Resource controls and accounting.
 */

getpriority()
{
	register struct a {
		int	which;
		int	who;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	u.u_error = ESRCH;
	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			p = u.u_procp;
		else
			p = pfind(uap->who);
		if (p == 0)
			return;
		u.u_r.r_val1 = p->p_nice - NZERO;
		u.u_error = 0;
		return;
		/*NOTREACHED*/
	default:
		u.u_error = EINVAL;
		u.u_r.r_val1 = 20;
		return;
		/*NOTREACHED*/

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		break;

	}
	u.u_r.r_val1 = NZERO+20;  	/* always return 20 when error */
	FORALLPROC(
		if (pp->p_nice < u.u_r.r_val1) {
			if ((uap->which == PRIO_USER && pp->p_uid == uap->who) ||
			    (uap->which == PRIO_PGRP && pp->p_pgrp == uap->who)) {
				u.u_r.r_val1 = pp->p_nice;
				u.u_error = 0;
			}
		}
	)

	u.u_r.r_val1 -= NZERO;
}

setpriority()
{
	register struct a {
		int	which;
		int	who;
		int	prio;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	u.u_error = ESRCH;
	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0) {
			p = u.u_procp;
			donice(p, uap->prio);
		} else {
			if (p = proc_get(uap->who)) {
				donice(p, uap->prio);
				proc_rele(p);
			}
		}
		return;

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		break;

	default:
		u.u_error = EINVAL;
		return;
	}
	FORALLPROC (
		if ((uap->which == PRIO_USER && pp->p_uid == uap->who) ||
		    (uap->which == PRIO_PGRP && pp->p_pgrp == uap->who))
			donice(pp, uap->prio);
	)
}

donice(p, n)
	register struct proc *p;
	register int n;
{

	if (u.u_uid && u.u_ruid &&
	    u.u_uid != p->p_uid && u.u_ruid != p->p_uid) {
		u.u_error = EACCES;
		return;
	}
	n += NZERO;
	if (n >= 2*NZERO)
		n = 2*NZERO - 1;
	if (n < 0)
		n = 0;
	if (n < p->p_nice && !suser()) {
		u.u_error = EACCES;
		return;
	}
	p->p_nice = n;
	(void) setpri(p);
	if (u.u_error == ESRCH)
		u.u_error = 0;
}

setrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *lim;
	} *uap = (struct a *)u.u_ap;
	struct rlimit alim;
	register struct rlimit *alimp;
	extern int maxssiz;
	extern int maxdsiz;

	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	alimp = &u.u_rlimit[uap->which];
	u.u_error = copyin((caddr_t)uap->lim, (caddr_t)&alim,
		sizeof (struct rlimit));
	if (u.u_error)
		return;
	if (alim.rlim_cur > alimp->rlim_max || alim.rlim_max > alimp->rlim_max)
		if (!suser())
			return;
	switch (uap->which) {

	case RLIMIT_DATA:
		if (alim.rlim_cur > ctob(maxdsiz))
			alim.rlim_cur = ctob(maxdsiz);
		break;

	case RLIMIT_STACK:
		if (alim.rlim_cur > ctob(maxssiz))
			alim.rlim_cur = ctob(maxssiz);
		break;
	}
	*alimp = alim;
	if (uap->which == RLIMIT_RSS)
		u.u_procp->p_maxrss = alim.rlim_cur/NBPG;
}

getrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *rlp;
	} *uap = (struct a *)u.u_ap;

	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyout((caddr_t)&u.u_rlimit[uap->which], (caddr_t)uap->rlp,
	    sizeof (struct rlimit));
}

getrusage()
{
	register struct a {
		int	who;
		struct	rusage *rusage;
	} *uap = (struct a *)u.u_ap;
	register struct rusage *rup;

	switch (uap->who) {

	case RUSAGE_SELF:
		rup = &u.u_ru;
		break;

	case RUSAGE_CHILDREN:
		rup = &u.u_cru;
		break;

	default:
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyout((caddr_t)rup, (caddr_t)uap->rusage,
	    sizeof (struct rusage));
}

ruadd(ru, ru2)
	register struct rusage *ru, *ru2;
{
	register long *ip, *ip2;
	register int i;

	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i >= 0; i--)
		*ip++ += *ip2++;
}
#ifdef mips
/*
 * mips_getrusage() provides the same functionality as getrusage() above but
 * allows the rusage structure to grow so that mips specific stats can be
 * returned in the future.
 */
mips_getrusage(who, rusage, rusage_size)
int	who;
struct	rusage *rusage;
int	rusage_size;
{
	register struct rusage *rup;
	int error;

	switch (who) {

	case RUSAGE_SELF:
		rup = &u.u_ru;
		break;

	case RUSAGE_CHILDREN:
		rup = &u.u_cru;
		break;

	default:
		return(EINVAL);
	}
	if(rusage_size < 0 || rusage_size > sizeof(struct rusage)){
		return(EINVAL);
	}
	error = copyout((caddr_t)rup, (caddr_t)rusage, rusage_size);
	return(error);
}
#endif mips
