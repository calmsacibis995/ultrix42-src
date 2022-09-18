#ifndef lint
static char *sccsid = "@(#)kern_prot.c	4.1	ULTRIX	7/2/90";
#endif lint

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

/************************************************************************
 * 
 *		Modification History
 *
 * 28 Sep 89 -- gg
 *	fix bug in getgroups. getgroups kernel function assumes
 *	gidsetsize argument as unsigned integer instead of signed integer.
 *
 * 09 Aug 89 -- gg
 *	modified setreuid/setregid function to return EPERM error in POSIX mode
 *	when the user doesn't have proper privileges.
 *
 * 23 Aug 88 -- miche
 *	Add support for ref'ing a process.
 *
 * 19 May 88 -- condylis
 *	Added smp credential locking
 ****************SMP changes above ***************
 *
 * 05 Aug 88 -- map
 *	Fix check regarding -1 to setre{ug}id to be NOT BSD. This will 
 *	catch the System V case.
 *
 * 11 Jul 88 -- map
 *	Add setsid() system call.
 *	Rewrite setpgrp() in POSIX mode to reflect change from jcsetpgrp()
 *	to setpgid().
 *
 * 22 Mar 88 -- map
 *	Fix problem in setreuid() and setregid() in POSIX and SYSV modes.
 *	Now only return error if BOTH ids are equal to -1.
 *
 * 15 Feb 88 -- map
 *	Cleaned up if() statements when checking for POSIX and SYSV modes.
 *
 * 03 Feb 88 -- map
 *	Fix problem in setpgrp.  In POSIX mode used p-> before it was
 *	initialized.
 *
 * 17 Jan 88 -- map
 *	Numerous changes for POSIX compliance.
 *		Added saved-set-id's to setreuid and setregid.
 *		Modified getgroups() for POSIX compliance.
 *		Modified getpgrp(), setpgrp() for POISX compliance.
 * 14 Dec 87 -- jaa
 *	Added new KM_ALLOC/KM_FREE macros
 *
 * 01 Oct 87 -- map
 *	Change assignment of p->p_uid to effective uid.  This fixes
 *	problem with permission checking in kill().(From 4.3BSD)
 *
 * 13 Nov 87 -- chet
 *	Added unbelievably gross hack at one minute to midnight
 *	to avoid cred ref count wraparound
 *
 * 11 Sep 86 -- koehler
 *	added this nice modification history and allowed km_alloc fetch
 *	clear memory (i.e., avoid the bzero)
 *
 ***********************************************************************/

#include "../machine/reg.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/smp_lock.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/exec.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/quota.h"
#include "../h/acct.h"
#include "../h/kmalloc.h"
#include "../h/limits.h"

getpid()
{

	u.u_r.r_val1 = u.u_procp->p_pid;
	u.u_r.r_val2 = u.u_procp->p_ppid;
}

getpgrp()
{
	register struct a {
		int	pid;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	if ((uap->pid == 0) || (u.u_procp->p_progenv == A_POSIX))
		uap->pid = u.u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
	u.u_r.r_val1 = p->p_pgrp;
}

getuid()
{

	u.u_r.r_val1 = u.u_ruid;
	u.u_r.r_val2 = u.u_uid;
}

getgid()
{

	u.u_r.r_val1 = u.u_rgid;
	u.u_r.r_val2 = u.u_gid;
}

getgroups()
{
	register struct	a {
		int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;
	register uid_t *lp;
	uid_t groups[NGROUPS];

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--)
		if (gp[-1] >= 0)
			break;
/*
 *	If in POSIX mode and gidsetsize = 0 then simply return number
 *	of groups.
 */
	if ((u.u_procp->p_progenv == A_POSIX) && (uap->gidsetsize == 0)) {
		uap->gidsetsize = gp - u.u_groups;
		u.u_r.r_val1 = uap->gidsetsize;
		return;
	}

	if (uap->gidsetsize < gp - u.u_groups) {
		u.u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u.u_groups;
/*
 *	If in POSIX mode must do everything in uid_t(short) instead of
 *	int.
 */
	if (u.u_procp->p_progenv == A_POSIX) {
	        for (lp = groups, gp = u.u_groups; lp < &groups[uap->gidsetsize]; )
	                *lp++ = *gp++;
		u.u_error = copyout((caddr_t)groups, (caddr_t)uap->gidset,
		    uap->gidsetsize * sizeof (groups[0]));
	}
	else {
		u.u_error = copyout((caddr_t)u.u_groups, (caddr_t)uap->gidset,
		    uap->gidsetsize * sizeof (u.u_groups[0]));
	}
	if (u.u_error)
		return;
	u.u_r.r_val1 = uap->gidsetsize;
}

setsid()
{
	register struct proc *p, *pp;

	p = u.u_procp;
	if(p->p_pgrp == p->p_pid) {
		u.u_error = EPERM;
		return;
	}
	FORALLPROC(
		if( (pp->p_pid != p->p_pid) && (pp->p_pgrp == p->p_pid)) {
			u.u_error = EPERM;
			return;
		}
	)
	p->p_pgrp = p->p_pid;
	p->p_sid = p->p_pid;
	p->p_ttyp = 0;
	u.u_r.r_val1 = u.u_procp->p_pgrp;

}
setpgrp()
{
	register struct proc *p;
	register struct a {
		int	pid;
		int	pgrp;
	} *uap = (struct a *)u.u_ap;
	register struct proc *pp, *q, *nq, *np;
	register int f;

	/* If POSIX mode - must be from setpgid()	*/
	if (u.u_procp->p_progenv == A_POSIX) {
		if ((uap->pgrp < 0) || (uap->pgrp > PID_MAX)) {
			u.u_error = EINVAL;
			return;
		}
			
		if (uap->pid == 0)
			uap->pid = u.u_procp->p_pid;
		if (uap->pgrp == 0)
			uap->pgrp = uap->pid;

		if ((p = proc_get(uap->pid)) == 0) {
			u.u_error = ESRCH;
			return;
		}
		if( p->p_sid == p->p_pid) {
			proc_rele(p); /* release ref'd proc */
			u.u_error = EPERM;
			return;
		}
		if( uap->pgrp != uap->pid) {
			f=0;
			FORALLPROC(
				if( (pp->p_pgrp == uap->pgrp) && (pp->p_sid == u.u_procp->p_sid)) {
				f++;
				break;
				}
			)
			if( f == 0) {
				proc_rele(p); /* release ref'd proc */
				u.u_error = EPERM;
				return;
			}
		}
		f = 0;
		(void) spl5();
		smp_lock(&lk_procqs, LK_RETRY);
		for (q = u.u_procp->p_cptr; q != NULL; q = nq) {
			nq = q->p_osptr;
			if (uap->pid == q->p_pid) {
				f++; /* Found a child (for error checking) */
				if( q->p_vm & SEXECDN) {
					u.u_error = EACCES;
					smp_unlock(&lk_procqs);
					proc_rele(p); /* release ref'd proc */
					(void) spl0();
					return;
				}
				if( q->p_sid != u.u_procp->p_sid) {
					u.u_error = EPERM;
					smp_unlock(&lk_procqs);
					proc_rele(p); /* release ref'd proc */
					(void) spl0();
					return;
				}
				break;
			}
		}
		if ((uap->pid != u.u_procp->p_pid) && (f == 0)) {
			u.u_error = ESRCH;
			smp_unlock(&lk_procqs);
			proc_rele(p); /* release ref'd proc */
			(void) spl0();
			return;
		}
	}
	else { /* BSD mode */
		if (uap->pid == 0)
			uap->pid = u.u_procp->p_pid;

 		if ((uap->pgrp == 0) && (u.u_uid)) {
 			u.u_error = EPERM;
 			return;
 		}

		if ((p = proc_get(uap->pid)) == 0) {
			u.u_error = ESRCH;
			return;
		}
/* need better control mechanisms for process groups */
		if (p->p_uid != u.u_uid && u.u_uid && !inferior(p)) {
			proc_rele(p); /* release ref'd proc */
			u.u_error = EPERM;
			return;
		}
		(void) spl5();
		smp_lock(&lk_procqs,LK_RETRY);
	}
/*
 * Got through all the protection checks!!!!
 */
	p->p_pgrp = uap->pgrp;
	smp_unlock(&lk_procqs);
	proc_rele(p); /* release ref'd proc */
	(void) spl0();
}

setreuid()
{
	struct a {
		int	ruid;
		int	euid;
	} *uap;
	register int ruid, euid;

	uap = (struct a *)u.u_ap;
	euid = uap->euid;
	ruid = uap->ruid;
	/* If both uid's are -1 then this in an error case in POSIX and
	 * System V, which
	 * does not allow the -1 syntax.  We must allow one uid = -1 because
	 * the other setuid functions (seteuid(), setruid()) call this function
	 * with one parameter = -1.
	*/
	if ((ruid == -1 && euid == -1) && (u.u_procp->p_progenv != A_BSD))  {
		u.u_error = EINVAL;
		return;
	}
	if (ruid == -1) 
		ruid = u.u_ruid;
	if ( ruid > UID_MAX ) {
		u.u_error = EINVAL;
		return;
	}
	if(u.u_procp->p_progenv == A_POSIX) {
		if (u.u_ruid != ruid && u.u_procp->p_suid != ruid && !suser())
			return;
	}
	else{
		if(u.u_ruid != ruid && u.u_uid != ruid && u.u_procp->p_suid != ruid && !suser())
			return;
	}
	if ( euid == -1 ) 
		euid = u.u_uid;
	if ( euid > UID_MAX ) {
		u.u_error = EINVAL;
		return;
	}
	if(u.u_procp->p_progenv == A_POSIX) {
		if (u.u_ruid != euid && u.u_procp->p_suid != euid && !suser())
			return;
	}
	else{
		if(u.u_ruid != euid && u.u_uid != euid && u.u_procp->p_suid != euid && !suser())
			return;
	}
	/*
	 * Everything's okay, do it.
	 */
	u.u_cred = crcopy(u.u_cred);
#ifdef QUOTA
	if (u.u_quota->q_uid != ruid) {
		qclean();
		qstart(getquota(ruid, 0, 0));
	}
#endif
	if((u.u_procp->p_progenv == A_POSIX) || 
				(u.u_procp->p_progenv == A_SYSV)) {
		if(u.u_uid == 0) {
			u.u_procp->p_suid = euid; 
			u.u_procp->p_uid = ruid;
			u.u_ruid = ruid;
		}
		u.u_uid = euid;
	}
	else {
		u.u_procp->p_uid = ruid;
		u.u_procp->p_suid = euid; 
		u.u_ruid = ruid;
		u.u_uid = euid;
	}
}

setregid()
{
	register struct a {
		int	rgid;
		int	egid;
	} *uap;
	register int rgid, egid;

	uap = (struct a *)u.u_ap;
	rgid = uap->rgid;
	egid = uap->egid;
	/* If both gid's are -1 then this in an error case in POSIX and
	 * System V , which
	 * does not allow the -1 syntax.  We must allow one gid = -1 because
	 * the other setgid functions (setegid(), setrgid()) call this function
	 * with one parameter = -1.
	*/
	if ((rgid == -1 && egid == -1) && (u.u_procp->p_progenv != A_BSD))  {
		u.u_error = EINVAL;
		return;
	}
	if (rgid == -1) 
		rgid = u.u_rgid;
	if (rgid > UID_MAX) {
		u.u_error = EINVAL;
		return;
	}
	if(u.u_procp->p_progenv == A_POSIX) {
		if (u.u_rgid != rgid && u.u_procp->p_sgid != rgid && !suser())
			return;
	}
	else{
		if(u.u_rgid != rgid && u.u_gid != rgid && u.u_procp->p_sgid != rgid && !suser())
			return;
	}
	if (egid == -1)
		egid = u.u_gid;
	if (egid > UID_MAX) {
		u.u_error = EINVAL;
		return;
	}
	if(u.u_procp->p_progenv == A_POSIX) {
		if (u.u_rgid != egid && u.u_procp->p_sgid != egid && !suser())
			return;
	}
	else{
		if(u.u_rgid != egid && u.u_gid != egid && u.u_procp->p_sgid != egid && !suser())
			return;
	}
	u.u_cred = crcopy(u.u_cred);
	if((u.u_procp->p_progenv == A_POSIX) || 
				(u.u_procp->p_progenv == A_SYSV)) {
		if(u.u_uid == 0) {
			if (u.u_rgid != rgid) {
				leavegroup(u.u_rgid);
				(void) entergroup(rgid);
				u.u_rgid = rgid;
			}
			u.u_procp->p_sgid = egid; 
		}
	u.u_gid = egid;
	}
	else {
		if (u.u_rgid != rgid) {
			leavegroup(u.u_rgid);
			(void) entergroup(rgid);
			u.u_rgid = rgid;
		}
		u.u_gid = egid;
		u.u_procp->p_sgid = egid; 
	}
}


setgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;
	struct ucred *newcr, *tmpcr;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u.u_groups) / sizeof (u.u_groups[0])) {
		u.u_error = EINVAL;
		return;
	}
	newcr = crdup(u.u_cred);
	u.u_error = copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    uap->gidsetsize * sizeof (newcr->cr_groups[0]));
	if (u.u_error) {
		crfree(newcr);
		return;
	}
	tmpcr = u.u_cred;
	u.u_cred = newcr;
	crfree(tmpcr);
	for (gp = &u.u_groups[uap->gidsetsize]; gp < &u.u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;
}

/*
 * Group utility functions.
 */

/*
 * Delete gid from the group set.
 */
leavegroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			goto found;
	return;
found:
	for (; gp < &u.u_groups[NGROUPS-1]; gp++)
		*gp = *(gp+1);
	*gp = NOGROUP;
}

/*
 * Add gid to the group set.
 */
entergroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			return (0);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp < 0) {
			*gp = gid;
			return (0);
		}
	return (-1);
}

/*
 * Check if gid is a member of the group set.
 */
groupmember(gid)
	int gid;
{
	register int *gp;

	if (u.u_gid == gid)
		return (1);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

/*
 * Routines to allocate and free credentials structures
 */

int crdebug = 0;
int cractive = 0;
struct lock_t lk_cred;		/* Credential lock structure	*/
/*
 * Hold a cred structure.
 */
crhold(cr)
	struct ucred *cr;
{
	/*if (crdebug)
		printf("crhold %x %d %d %x\n",
		    cr, cr->cr_uid, cr->cr_ref, caller());
	*/

	/* Hold credential lock while using reference count of a cred*/
	smp_lock(&lk_cred, LK_RETRY);

	if (cr->cr_ref++ > 30000)
		panic("crhold: cred ref count about to wrap around");

	smp_unlock(&lk_cred);
}

/*
 * Allocate a zeroed cred structure and crhold it.
 */
struct ucred *
crget()
{
	struct ucred *cr;

	KM_ALLOC(cr, struct ucred *, sizeof(*cr), KM_CRED, KM_CLEAR);
	crhold(cr);
/*	printf("crget: %x\n", cr);*/
	/* Use cred lock to maintain integrity of cractive counter */
	smp_lock(&lk_cred, LK_RETRY);
	cractive++;
	smp_unlock(&lk_cred);
	return(cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
crfree(cr)
	struct ucred *cr;
{
	int	s = splclock();

	/*if (crdebug)
		printf("crfree %x %d %d %x\n",
		    cr, cr->cr_uid, cr->cr_ref, caller());
	*/
	/* Hold credential lock while using reference count of a cred*/
	smp_lock(&lk_cred, LK_RETRY);
	if (--cr->cr_ref > 0) {
		smp_unlock(&lk_cred);
		splx(s);
		return;
	}
	if (cr->cr_ref < 0) 
		panic("crfree: cred ref count decremented to minus value");

	/* Using cred lock to maintain integrity of cractive counter */
	cractive--;
	smp_unlock(&lk_cred);
	KM_FREE(cr, KM_CRED);
	splx(s);
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	crfree(cr);
	newcr->cr_ref = 1;
/*	printf("crcopy: old %x %d %d new %x\n", cr, cr->cr_uid, cr->cr_ref, newcr);*/
	return(newcr);
}

/*
 * Dup cred struct to a new held one.
 */
struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
/*	printf("crdup: old %x %d %d new %x\n", cr, cr->cr_uid, cr->cr_ref, newcr);*/
	return(newcr);
}

/*
 * Initialize cred lock
 */

crinit()
{
	lockinit(&lk_cred, &lock_cred_d);
}



