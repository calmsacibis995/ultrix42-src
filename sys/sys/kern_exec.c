#ifndef lint
static char *sccsid = "@(#)kern_exec.c	4.8	ULTRIX	4/11/91";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1988 by		*
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
 * 11-Apr-91	dlh
 * 	execve():
 * 		- #if defined(__vax) the call to vp_cleanup()
 *
 * 28 Feb 91 -- prs
 *	Added support for a configurable number of
 *	open file descriptors.
 *
 * 12 Nov 90 -- scott
 *	added audit of exec argp,envp
 *
 * 4-Sep-90	dlh
 *	added vector processor support code
 *
 * 10 Aug 90 -- jaa
 *	Unaligned access fixup is default behaviour
 *
 * 31 Jul 90 -- jmartin
 *	Avoid page faulting on the map of the next image, which may not
 *	have the same start addresses as the current image.  Thanks to
 *	Bruce Cole of U. of Wisconsin.
 *
 *  03 Apr 90 gmm
 *	Do flush_tlb() for mips (like TBIA in VAX) in getxfile()
 *
 *  17 Apr 90 jaa
 *	mark "407" proc's so that we'll flush the i-cache when 
 *	giving them memory
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 30 Nov 89 -- lan
 *	chg'd u.u_envp to be u.u_execstkp and make it be base of the
 *	built stack, not just the argument/environment variable strings.
 *
 * 17 Oct 89 -- jaa
 *	change mips to return ENOEXEC if it tries to 
 *	 exec() a vax binary
 *
 * 16 Oct 89 -- bp
 *	added u.u_envp (base of arguments and environement)
 *
 * 22 Sep 89 -- gg
 *	(merged after 17 Oct 89 change)
 *	fixed a vax/mips merge problem. In vax, in POSIX environment
 *	system call was getting restarted instead of returning EINTR.
 *
 * 21 Jul 89 -- jaa
 *	added new chksiz() interface to getxfile()
 *
 * 14 Jun 89 -- prs
 *	Changed execve() to release gp while closing close on exec files.
 *	Releasing the gnode is fine becasue text table holds reference.
 *
 * 12-Jun-89 - gg
 *	Dynamic swap changes - changed call swpexpand() to dmalloc
 *
 * 09-Jun-89 -- scott
 *	added audit support
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 * 25 Jul 88 -- jmartin
 *	Use the macros SET_P_VM and CLEAR_P_VM to replace "|=" and "&=~".
 *
 * 24 Mar 88 -- prs
 *      Changed conditionals after calls to copy*str routines from
 *      ENOENT to ENAMETOOLONG. The copy*str routines now return
 *      the latter error code.
 *
 * 28 Jan 88 -- us
 *	Changed to new kmalloc.
 *
 ****** SMP CHANGES above ******
 *
 * 11 Jul 88 -- map
 *	Add setting of SEXECDN flag to indicate the a process has been
 *	exec'd.  Needed for POSIX job control permission checks.
 *
 * 02 Jun 88 -- map
 *	Removed setting of CLDSTP flag since flag is now NOCLDSTP and
 *	the default is correct for BSD environment. (POSIX 12.3)
 *
 * 07 Mar 88 -- prs
 *      Changed conditionals after calls to copy*str routines from
 *      ENOENT to ENAMETOOLONG. The copy*str routines now return
 *      the latter error code.
 *
 * 12 Jan 88 -- fglover
 *	add call to release sys-V locks in execve
 *
 * 14 Dec 87 -- jaa
 *	changed km_alloc/km_free's to new KM_ALLOC/KM_FREE macros
 *
 * 14 Oct 87 -- map
 *	Set p_suid field to saved effective uid
 *	Set p_sgid field to saved effective gid
 *
 * 03 Nov 87 -- map
 *      Set p_progenv to the value specified in the mode section of the
 *      magic number.
 *
 * 27 Jul 87 -- depp
 *      Changed argument kmemall() to km_alloc().
 *
 * 14 Jul 87 -- cb
 * 	Added in rr's changes.
 *
 * 29 Jan 87
 *	Add new arg to bdwrite() calls.
 *
 * 15 Dec 86 -- depp
 *	Fixed problem with error return from rdwri (in getxfile()).
 *
 * 11 Sep 86 -- koehler
 *	gfs namei interface change
 *
 * 02 Apr 86 -- depp
 *	Integrated two performance changes into execve.  From 4.3UCB, 
 *	arguments are now copied by strings rather than by character, and
 *	the a.out header information (struct exec) is now placed on the 
 *	stack, rather than in the process' "u"-area.  From SUN, we
 *	now will fully read in small 0413 executables, rather than
 *	demand paging them in.
 *
 * 12-Feb-86 -- jrs
 *	Added calls to tbsync() to control mp translation buffer
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 14 Oct 85 -- Reilly
 *	Modified user.h
 *
 * 18 Sep 85 -- depp
 *	Added punlock call to clear memory locks before text detach (execve)
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 *
 * 11 Mar 85 -- depp
 *	Added in System V shared memory support
 *
 *  05-May-85 - Larry Cohen
 *	loop up to u_omax instead of NOFILE
 *
 *  19 Jul 85 -- depp
 *	Removed call to smexec as this call is made in vrelvm as smclean
 *
 */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/acct.h"
#include "../h/exec.h"
#include "../h/kmalloc.h"

#ifdef mips
#include "../h/ptrace.h"
#endif mips
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#ifdef vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif vax

#ifdef GFSDEBUG
extern short GFS[];
#endif GFSDEBUG

#ifdef vax 
/* SETREGS currently only sets the PC, but in the future ...? */
#define SETREGS(x)	(u.u_ar0[PC] = (x) + 2)
#endif vax

/*
 * text+data (not bss) smaller than fd_pgthresh will be read in (if there
 * is enough free memory), even though the file is 0413
 */

#define IS_SPAGI(ep,gp,ts)	/* for p_vm */	\
	(((ep)->a_magic == 0413) && \
	(((ts) + clrnd(btoc((ep)->a_data))) > \
	MIN(freemem,(gp)->g_mp->m_fs_data->fd_pgthresh)))
/*
 * exec system call, with and without environments.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

execv()
{
	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

execve()
{
	register int nc;
	register int cc;
	register char *cp;
	register struct execa *uap = (struct execa *)u.u_ap;
	register struct nameidata *ndp = &u.u_nd;
	register struct gnode *gp;
	struct file *fp;
	struct proc *p = u.u_procp;
	int na, ne, ucp, ap, len, indir;
	short uid, gid;
	char *sharg;
	char *arg_holder = 0;
	char *audptr[4];
	int do_aud = 0;
	char cfname[MAXCOMLEN + 1];
	char cfarg[SHSIZE];
	union {
		char	ex_shell[SHSIZE];  /* #! and name of interpreter */
		struct	exec ex_exec;
	} exdata;
	int resid, error, s;
	struct exec *ep = &exdata.ex_exec;
	char *shstart;
	int shlength;
#ifdef mips
	unsigned long text_start, data_start;
	unsigned long stack_top;
	int set_vm_start = 0;
#endif mips

	/*
	 * if this process was a vector process, then clean up
	 */

	if (p->p_vpcontext) {
		vp_cleanup(p);
	}


	ndp->ni_nameiop = LOOKUP | FOLLOW;
	
	
	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN + 1, KM_NAMEI, KM_NOARG);
	if(ndp->ni_dirp == NULL) {
		return;
	}
 	if(u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN + 1, &len)) {
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		return;
	}

	if ( DO_AUDIT(u.u_event) ) {
		KM_ALLOC(audptr[0], char *, len, KM_TEMP, KM_CLEAR);
		if ( audptr[0] == NULL ) {
			KM_FREE(ndp->ni_dirp, KM_NAMEI);
			return;
		}
		if (u.u_error = copystr(ndp->ni_dirp, audptr[0], len, &len)) {
			KM_FREE(ndp->ni_dirp, KM_NAMEI);
			KM_FREE(audptr[0], KM_TEMP);
			return;
		}
		do_aud = 1;
		audptr[2] = audptr[3] = '\0';
	}

	if ((gp = gfs_namei(ndp)) == NULL) {
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		if ( do_aud ) KM_FREE(audptr[0], KM_TEMP);
		return;
	}
	
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
	ndp->ni_dirp = (char *)NULL;

	if((gp->g_mp->m_flags & M_NOEXEC) && u.u_uid) {
		u.u_error = EROFS;
		goto bad;
	}

	if((gp->g_mp->m_flags & M_NOSUID) && (gp->g_mode & (GSUID | GSGID))
	&& u.u_uid) {
		u.u_error = EROFS;
		goto bad;
	}
	
#ifdef GFSDEBUG
	if(GFS[7])
		cprintf("execve: gp 0x%x (%d)\n", gp, gp->g_number);
#endif
	indir = 0;
	uid = u.u_uid;
	gid = u.u_gid;
	if (gp->g_mode & GSUID)
		uid = gp->g_uid;
	if (gp->g_mode & GSGID)
		gid = gp->g_gid;

#ifdef GFSDEBUG
	if(GFS[7])
		cprintf("execve: uid (%d) u.u_uid %d cdir 0x%x (%d) count %d\n",
		uid, u.u_uid, u.u_cdir, u.u_cdir->g_number, u.u_cdir->g_count);
#endif GFSDEBUG
  again:
	if (access(gp, GEXEC))
		goto bad;
	if ((p->p_trace&STRC) && access(gp, GREAD))
		goto bad;
	if ((gp->g_mode & GFMT) != GFREG ||
	   (gp->g_mode & (GEXEC|(GEXEC>>3)|(GEXEC>>6))) == 0) {
		u.u_error = EACCES;
		goto bad;
	}

	/*
	 * Read in first few bytes of file for segment sizes, magic number:
	 *	407 = plain executable
	 *	410 = RO text
	 *	413 = demand paged RO text
	 * Also an ASCII line beginning with #! is
	 * the file name of a ``shell'' and arguments may be prepended
	 * to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 */
	exdata.ex_shell[0] = '\0';	/* for zero length files */
	u.u_error = rdwri(UIO_READ, gp, (caddr_t)&exdata, sizeof (exdata),
	    0, 1, &resid);
	if (u.u_error)
		goto bad;
#ifndef lint
	if (resid > sizeof(exdata) - sizeof(exdata.ex_exec) &&
	    exdata.ex_shell[0] != '#') {
		u.u_error = ENOEXEC;
		goto bad;
	}
#endif
#ifdef mips
	if (exdata.ex_exec.ex_f.f_magic != OBJMAGIC) {
		/* if not mips be more careful...*/
		short vaxmagic = *((short *)(&exdata));
		if (vaxmagic == 0407 || vaxmagic == 0410 || vaxmagic == 0413) {
			u.u_error = ENOEXEC;     /* invalid object format */
			goto bad;
		}
		/* now try script */
		goto try_script;
	}
	/*
	 * check for unaligned entry point
	 */
	if (exdata.ex_exec.ex_o.entry & (sizeof(int)-1)) {
		u.u_error = ENOEXEC;
		goto bad;
	}
#endif mips
	if (ep->a_magic ==  0407) {
		ep->a_data += ep->a_text;
		ep->a_text = 0;
		goto getargs;
	}
	if (ep->a_magic ==  0413 ||  ep->a_magic ==  0410) {
		if (ep->a_text == 0) {
			u.u_error = ENOEXEC;
			goto bad;
		}
#ifdef mips
		/*
		 * Currently kernel enforces segment alignment. A 
		 * segment is defined as the amount of memory that
		 * can be mapped by one page of page tables, currently
		 * on an R2000 system this is 4Meg (1024 ptes per page 
		 * and 1 pte maps 4096 bytes).
		 * The kernel also disallows the use of the 0th segment,
		 * 0 through 4Meg-1. This is due to a bug in the 3.0 rev
		 * R2000 chips which trashes the context register to 0.
		 * A utlbmiss can look like an access to the 0th segment 
		 * since the context register gets set to 0. If anything 
		 * in the 0th segment is valid, a tlbdropin will occur 
		 * without a probe being done. This could cause multiple 
		 * matching tlb entries which can lead to a tlb dead condition.
		 * A reset is required if a tlb dead situation occurs. 
		 * Rumor is that the rev 5 R2000s will fix this problem.
		 * We'll wait and see......
		 */
		text_start = exdata.ex_exec.ex_o.text_start;
		data_start = exdata.ex_exec.ex_o.data_start;
		if ((text_start & (NPTEPG*NBPG-1)) || 
		    (data_start & (NPTEPG*NBPG-1)) || 
		    (text_start == 0) || (data_start == 0)) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		/*
		 * Make sure the text and data segments do not overlap.
		 */
		if ((text_start >= data_start) && (text_start < 
		    (data_start + exdata.ex_exec.ex_o.dsize + 
		    exdata.ex_exec.ex_o.bsize))) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		if ((data_start >= text_start) && (data_start < 
		    (text_start + exdata.ex_exec.ex_o.tsize))) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		/*
		 * Make sure the data and text segments do not overlap
		 * the stack segment.
		 */
		stack_top = USRSTACK - ((NPTEPG-HIGHPAGES)*NBPG);
		if ((text_start + exdata.ex_exec.ex_o.tsize) > stack_top) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		if ((data_start + exdata.ex_exec.ex_o.dsize + 
		    exdata.ex_exec.ex_o.bsize) > stack_top) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		/*
		 * Defer map changes until after any page faults which
		 * may occur getting arguments.
		 */
		set_vm_start = 1;
#endif mips
		goto getargs;
	}
#ifdef mips
try_script:
#endif mips
	if (exdata.ex_shell[0] != '#' || exdata.ex_shell[1] != '!' ||
		indir) {
		u.u_error = ENOEXEC;
		goto bad;
	}
	/* handle the shell script */
	cp = &exdata.ex_shell[2];		/* skip "#!" */
	while (cp < &exdata.ex_shell[SHSIZE]) {
		if (*cp == '\t')
			*cp = ' ';
		else if (*cp == '\n') {
			*cp = '\0';
			break;
		}
		cp++;
	}
	if (*cp != '\0') {		/* This guarantees that the rest of
					 * the parsing is using a null-
					 * terminated string.
					 */
		u.u_error = ENOEXEC;
		goto bad;
	}
	cp = &exdata.ex_shell[2];
	while (*cp == ' ')
		cp++;
	shstart = cp;			/* start of shell name */
	while (*cp && *cp != ' ')
		cp++;
	shlength = cp - shstart;	/* length of shell name */
	cfarg[0] = '\0';
	if (*cp) {
		*cp++ = '\0';
		while (*cp == ' ')
			cp++;
		if (*cp)
			bcopy((caddr_t)cp, (caddr_t)cfarg,
			      &exdata.ex_shell[SHSIZE] - cp);
	}
	indir = 1;
	gput(gp);
	ndp->ni_nameiop = LOOKUP | FOLLOW;
	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN + 1, KM_NAMEI, KM_NOARG);
	if(ndp->ni_dirp == NULL) {
		gp = NULL;
		goto bad;
	}
	bcopy((caddr_t)shstart, (caddr_t)ndp->ni_dirp, shlength);
	ndp->ni_dirp[shlength] = '\0';
	gp = gfs_namei(ndp);
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
	if (gp == NULL)
		goto bad;


#ifdef GFSDEBUG
	if(GFS[7])
		cprintf("execve: gp 2 0x%x (%d)\n", gp, gp->g_number);
#endif
	cc = ndp->ni_dent.d_namlen;
	cc = (cc > MAXCOMLEN ? MAXCOMLEN : cc);
	bcopy((caddr_t)ndp->ni_dent.d_name, (caddr_t)cfname, cc);
	cfname[cc] = '\0';
	/* end of shell script */
	goto again;

getargs:

	KM_ALLOC(arg_holder, char *, NCARGS, KM_TEMP, KM_CLEAR);
	if (arg_holder == NULL) {
		swkill(p, "exec: km_alloc fault");
		goto bad;
	}
	cp = arg_holder;	/* incr. us through holding area */
	na = 0;			/* incr. for each arg/env.var found */
	ne = 0;			/* incr. for each env.var found */
	nc = 0;			/* count of #chars in arg/env.var strings */
	cc = 0;			/* remainder of string holding area */
	/*
	 * Copy arguments into safe memory
	 */
	if (uap->argp) for (;;) {
		ap = NULL;
		sharg = NULL;
		if (indir && na == 0) {
			sharg = cfname;
			ap = (int)sharg;
			uap->argp++;		/* ignore argv[0] */
		}
		else if (indir && (na == 1 && cfarg[0])) {
			sharg = cfarg;
			ap = (int)sharg;
		}
		else if (indir && (na == 1 || na == 2 && cfarg[0])) {
			ap = (int)uap->fname;
		}
		else if (uap->argp) {
			ap = fuword((caddr_t)uap->argp);
			uap->argp++;
		}
		if (ap == NULL && uap->envp) {	/* ==> all environment
						       vars. caught here */
			uap->argp = NULL;	/* force env.var. handling
						   for remaining passes */
			if ((ap = fuword((caddr_t)uap->envp)) != NULL) {
				uap->envp++;
				ne++;		/* track #env.vars. */
			}
		}
		if (ap == NULL)		/* no more args or env.vars */
			break;
		na++;
		if (ap == -1) {
			/* fuword returns -1 on fault */
			error = EFAULT;
			goto argerr;	/* get out */
		}
		do {			/* get arg/env.var string */
			if (cc <= 0) {
				if (nc >= NCARGS-1) {
					error = E2BIG;
					break;
				}
				cc = NCARGS;
			}
			if (sharg) {
				error = copystr(sharg, cp, (unsigned)cc, &len);
				sharg += len;
			} else {
				error = copyinstr((caddr_t)ap, cp, (unsigned)cc, &len);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENAMETOOLONG);
argerr:
		if (error) {
			u.u_error = error;
			goto bad;
		}
	}
#ifdef mips
	if (set_vm_start) {
		u.u_procp->p_textstart = text_start;
		u.u_procp->p_datastart = data_start;
	}
#endif mips
	nc = (nc + NBPW-1) & ~(NBPW-1);
	getxfile(gp, ep, nc + (na+4)*NBPW, uid, gid);
	if (u.u_error) {
		goto bad;
	}

	/*
	 * Copy back arglist.
	 *  ucp - (ustack char ptr) starts as addr(ustack area for
	 *		arg/env.var. strings, and walks up the ustack
	 *  ap  - starts as addr(ustack area for #args), and walks up
	 *		through *argv[] and *envp[].
	 */
#ifdef vax
	ucp = USRSTACK - nc - NBPW;
	ap = ucp - na*NBPW - 3*NBPW;
	u.u_ar0[SP] = ap;
#endif vax
#ifdef mips
	ucp = USRSTACK - nc - NBPW - EA_SIZE;
	ap = ucp - na*NBPW - 3*NBPW;
	if (ap & 0xf) {
		int fudge;

		fudge = ap - (ap & ~0xf);
		ap -= fudge;
		ucp -= fudge;
	}
	u.u_ar0[EF_SP] = ap;
#endif mips
	u.u_execstkp = (caddr_t) ap;	/* save built ustack area base */
	(void) suword((caddr_t)ap, na-ne);	/* save # args */

/*	na: #arg/env.vars to be copied, decremented though loop  */
/*	ne: #env.vars found, ctl's switching between argv & envp */
	nc = 0;			/* #chars copied counter */
	cc = 0;			/* space left counter */
	cp = arg_holder;	/* address holding area again */
	for (;;) {
		ap += NBPW;
		if (na == ne) {	/* ==> end of args. to process,
					start in on environment vars. */
			(void) suword((caddr_t)ap, 0);	/* mark end *argv[] */
			ap += NBPW;	/* now addr(*envp[0]) */
			if (cp != arg_holder) {
				*(cp-1) = '\0';
				audptr[2] = cp;
			}
		}
		if (--na < 0)		/* done */
			break;
		(void) suword((caddr_t)ap, ucp);
		do {
			if (cc <= 0) {
				cc = NCARGS;
			}
			error = copyoutstr(cp, (caddr_t)ucp, (unsigned)cc, 
			    &len);
			ucp += len;
			cp += len;
			if (len) *(cp-1) = ' ';
			nc += len;
			cc -= len;
		} while (error == ENAMETOOLONG);
		if (error == EFAULT)
			panic("exec: EFAULT");
	}
	if (cp != arg_holder) {
		*(cp-1) = '\0';
		audptr[3] = cp-1;
	}
	(void) suword((caddr_t)ap, 0);	/* mark end of *envp[] */

	/*
	 * Reset caught signals.  Held signals
	 * remain held through p_sigmask.
	 */
	while (p->p_sigcatch) {
		s = splhigh();
		nc = ffs(p->p_sigcatch);
		p->p_sigcatch &= ~sigmask(nc);
		u.u_signal[nc] = SIG_DFL;
		splx(s);
	}
	/*
	 * Reset stack state to the user stack.
	 * Clear set of signals caught on the signal stack.
	 */
	u.u_onstack = 0;
	u.u_sigsp = 0;
	u.u_sigonstack = 0;
#ifdef mips
        /*
	 * pmax does the compatibility mode in crt0 through set_sysinfo(). 
	 * u.u_sigintr will be set to -1 if the mode is POSIX or SYSV.
         */
	u.u_sigintr = 0;
	u.u_procp->p_mips_flag |= SFIXADE;
#endif mips 

#ifdef vax
	/*
	 * set u_structure flags if in POSIX mode/SYSV mode
	 */

	if (p->p_progenv == A_POSIX || p->p_progenv == A_SYSV) {
		u.u_sigintr = -1;	/* Set all bits ON so will interrupt */
	}
	else
		u.u_sigintr = 0; /*BSD mode clear it */
#endif vax

#ifdef vax
	SETREGS(ep->a_entry);
#endif vax
#ifdef mips
	setregs(exdata.ex_exec.a_entry);
#endif mips
	/*
	 * Remember file name for accounting.
	 */
	u.u_acflag &= ~AFORK;
	if (indir)
		bcopy((caddr_t)cfname, (caddr_t)u.u_comm, MAXCOMLEN);
	else {
                /* u.u_comm is a directory entry in length */
                /* which is MAXNAMLEN+1 so we copy the whole length and */
                /* set the last byte to null which we always have room for */
                cc = ndp->ni_dent.d_namlen;
                bcopy((caddr_t)ndp->ni_dent.d_name, (caddr_t)u.u_comm, cc);
                u.u_comm[cc]='\0';
	}
	if (gp) {
		gput(gp);
		gp = NULL;
	}
	for (nc = u.u_omax; nc >= 0; --nc) {
		if (!U_OFILE(nc)) {
			U_POFILE_SET(nc, 0);
			continue;
		}
		if (U_POFILE(nc) & UF_EXCLOSE) {
			/* Release all sys-V locks */
			(void) gno_lockrelease(U_OFILE(nc));
			fp = U_OFILE(nc);
			U_OFILE_SET(nc, NULL);
			U_POFILE_SET(nc, 0);
			closef(fp);
		}
	     /*
	      * Mapped files are no-op'd for now
	      *
	      * else u.u_pofile[nc] &= ~UF_MAPPED;
	      */	
	}
	while (u.u_lastfile >= 0 && (U_OFILE(u.u_lastfile) == NULL))
		u.u_lastfile--;
bad:
	if (gp)
		gput(gp);
	if ( do_aud ) {
		/* adjust ptrs to argp, envp */
		audptr[1] = arg_holder;
		if ( arg_holder && audptr[2] ) {
			if ( (audstyle&AUD_EXEC_ARGP) == 0 ) audptr[1][0] = '\0';
			else if ( audptr[2] - audptr[1] > AUD_BUF_SIZ )
				*(audptr[1]+AUD_BUF_SIZ) = '\0';
			if ( (audstyle&AUD_EXEC_ENVP) == 0 ) audptr[2][0] = '\0';
			else if ( audptr[3] - audptr[2] > AUD_BUF_SIZ )
				*(audptr[2]+AUD_BUF_SIZ) = '\0';
		}
		audit_rec_build ( u.u_event, audptr, aud_param[u.u_event],
		u.u_error, u.u_r.r_val1, (int *)0, AUD_HDR|AUD_PRM|AUD_RES );
		KM_FREE(audptr[0], KM_TEMP);
	}
	if (arg_holder)
		KM_FREE(arg_holder, KM_TEMP);
}

/*
 * Read in and set up memory for executed file.
 */
getxfile(gp, ep, nargc, uid, gid)
	register struct gnode *gp;
	register struct exec *ep;
	int nargc, uid, gid;
{
	register struct proc *p = u.u_procp;
	register struct file *fp;
	register struct text *xp;
	register int pagi = 0;
	size_t ts, ds, ids, uds, ss;
	int do_flush = 1;	/* whether or not to flush buffer cache */

	if (gp->g_textp && (gp->g_textp->x_flag & XTRC)) {
		u.u_error = ETXTBSY;
		goto bad;
	}

	if (ep->a_text != 0 && (gp->g_flag&GTEXT) == 0 && gp->g_count != 1) {
		/* check to see if someone has a descriptor for write */

		for (fp = file; fp < fileNFILE; fp++) {
			if (fp->f_type == DTYPE_INODE &&
			    fp->f_count > 0 &&
			    (struct gnode *)fp->f_data == gp &&
			    (fp->f_flag&FWRITE)) {
				u.u_error = ETXTBSY;
				goto bad;
			}
		}
	}

	/*
	 * Compute text and data sizes and make sure not too large.
	 * NB - Check data and bss separately as they may overflow 
	 * when summed together.
	 */
	ts = clrnd(btoc(ep->a_text));
	ids = clrnd(btoc(ep->a_data));
	uds = clrnd(btoc(ep->a_bss));
	ds = clrnd(btoc(ep->a_data + ep->a_bss));
	ss = clrnd(SSIZE + btoc(nargc));
	if (chksize((unsigned)ts, (unsigned)ids, (unsigned)uds, (unsigned)ss))
		goto bad;
	
	/*
	 * Make sure enough space to start process.
	 */
	if((u.u_procp->p_cdmap = dmalloc(ds, CDATA)) == 0){
		swfail_stat.exec_fail++;
		goto bad;
	}
	if((u.u_procp->p_csmap = dmalloc(ss, CSTACK)) == 0) {
		dmfree(u.u_procp->p_cdmap, ds, CDATA);
		swfail_stat.exec_fail++;
		goto bad;
	}
	

	/*
	 * At this point, committed to the new image!
	 * Release virtual memory resources of old process, and
	 * initialize the virtual memory of the new process.
	 * If we resulted from vfork(), instead wakeup our
	 * parent who will set SVFDONE (p_vm) when it has
	 * taken back our resources.
	 */
	/* BUT FIRST, clear any memory locks */
	(void) punlock();

	vrelvm();

	/*
	 * If page currently in use or reclaimable, 
	 * Then set pagi according to it's current use,
	 * Else determine whether to demand page this process
	 */
	if ((gp->g_flag & GTEXT) && (xp = gp->g_textp)) {
		do_flush = 0;	/* don't have to flush since it is clean */
		if (xp->x_flag & XPAGI)
			pagi = SPAGI;	/* for p_vm */
	} else if (IS_SPAGI(ep,gp,ts))	/* for p_vm */
		pagi = SPAGI;	/* for p_vm */

	p->p_vm &= ~(SPAGI|SSEQL|SUANOM);
	p->p_vm |= pagi;
#ifdef mips
	if (ep->a_magic == 0407) 
 		SET_P_VM(p, SXCTDAT);
	release_tlbpid(u.u_procp);
	get_tlbpid(u.u_procp);
	clear_tlbmappings(u.u_procp);
	set_tlbwired(u.u_procp);
#endif mips

	p->p_dmap = p->p_cdmap;
	p->p_cdmap = (struct dmap *) NULL;
	p->p_smap = p->p_csmap;
	p->p_csmap = (struct dmap *) NULL;
	vgetvm(ts, ds, ss);

	if (pagi == 0) {
		u.u_error =
		    rdwri(UIO_READ, gp, (char *)ctob(dptov(p, 0)),
			(int)ep->a_data,
#ifdef vax
			(int)((ep->a_magic == 0413 ?
			    CLBYTES : sizeof(struct exec)) + ep->a_text),
#endif vax
#ifdef mips
			(off_t)(N_TXTOFF(ep->ex_f, ep->ex_o) + ep->a_text),
#endif mips
			0, (int *)0);
	}
	if(u.u_error) {
		swkill(p, "exec: error reading data area");
		goto bad;
	}

	if(xalloc(gp, ep, pagi) == NULL) 
		goto bad;
	
	if (pagi && p->p_textp)
		vinifod((struct fpte *)dptopte(p, 0), PG_FTEXT, gp,
#ifdef vax
		    (long)(1 + ts/CLSIZE), (int)btoc(ep->a_data)
#endif vax
#ifdef mips
		    (long)(0 + ts/CLSIZE), (size_t)btoc(ep->a_data)
#endif mips
			);

	/* here's where we need to flush the cache so the disk has the */
	/* correct text and data to pagein */
	/* if not pagi then rdwri got it in the cache anyway for text and data*/
	/* only flush if this is not a clean text */
	/* not clean means first time ever executed */
	/* since it is locked from writes  once you execute it */
	if (pagi && do_flush)
	        flushblocks(gp);

#ifdef vax
	/* Quiesce the vector processor if necessary */
	VPSYNC ();

	/* THIS SHOULD BE DONE AT A LOWER LEVEL, IF AT ALL */
	mtpr(TBIA, 0);
#else mips
	flush_tlb();
#endif vax ^ mips
	tbsync();

	if (u.u_error) {
		swkill(p, "exec: I/O error mapping pages");
		goto bad;
	}

#ifdef vax
        /*
         * Setup compatibility mode parameter.
         * NOTE: The exec structure must be set by the loader.
	 * pmax does this in crt0 through set_sysinfo()
         */
        p->p_progenv = ep->a_mode;

#endif vax	
	SET_P_VM(p, SEXECDN);	/* Flag that exec() has been done */
	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((p->p_trace&STRC)==0) {
		if(uid != u.u_uid || gid != u.u_gid)
			u.u_cred = crcopy(u.u_cred);
		u.u_uid = uid;
		p->p_suid = uid;
		u.u_gid = gid;
		p->p_sgid = gid;
	} else {
#ifdef mips
		u.u_trapcause = CAUSEEXEC;
		u.u_trapinfo = 0;
#endif mips
		psignal(p, SIGTRAP);
	}
	u.u_tsize = ts;
	u.u_dsize = ds;
	u.u_ssize = ss;
	u.u_prof.pr_scale = 0;
bad:
	return;
}
