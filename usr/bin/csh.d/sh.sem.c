#ifndef lint
static char *sccsid = "@(#)sh.sem.c	4.1  (ULTRIX)        7/17/90";
#endif
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sh.sem.c,v 1.3 86/07/11 10:46:55 dce Exp $ */

#include "sh.h"
#include "sh.proc.h"
#include <sys/ioctl.h>

/*
 * C shell
 * Modification History
 *
 * 003 - Gary A. Gaudet - Tue Jan  2 11:59:29 EST 1990
 *	added return value check from ioctl()
 *
 * 02 Sat Aug 13 15:28:57 EDT 1988, Gary A. Gaudet
 *	merging mips & ultrix for 8 bit clean and bug fixes
 *
 * 01 Thu Aug 11 12:48:39 EDT 1988, Gary A. Gaudet
 *	Changed vffree to void.
 */

/*VARARGS 1*/
execute(t, wanttty, pipein, pipeout)
	register struct command *t;
	int wanttty, *pipein, *pipeout;
{
	bool forked = 0;
	struct biltins *bifunc;
	int pid = 0;
	int pv[2];

	if (t == 0)
		return;
	if ((t->t_dflg & FAND) && wanttty > 0)
		wanttty = 0;
	switch (t->t_dtyp) {

	case TCOM:
		if (((t->t_dcom[0][0]) & TRIM) == QUOTECHAR)
			(void) strcpy(t->t_dcom[0], t->t_dcom[0] + 2);
		if ((t->t_dflg & FREDO) == 0)
			Dfix(t);		/* $ " ' \ */
		if (t->t_dcom[0] == 0)
			return;
		/* fall into... */

	case TPAR:
		if (t->t_dflg & FPOU)
			mypipe(pipeout);
		/*
		 * Must do << early so parent will know
		 * where input pointer should be.
		 * If noexec then this is all we do.
		 */
		if (t->t_dflg & FHERE) {
			(void) close(0);
			heredoc(t->t_dlef);
			if (noexec)
				(void) close(0);
		}
		if (noexec)
			break;

		set("status", "0");

		/*
		 * This mess is the necessary kludge to handle the prefix
		 * builtins: nice, nohup, time.  These commands can also
		 * be used by themselves, and this is not handled here.
		 * This will also work when loops are parsed.
		 */
		while (t->t_dtyp == TCOM)
			if (eq(t->t_dcom[0], "nice"))
				if (t->t_dcom[1])
					if (any(t->t_dcom[1][0], "+-"))
						if (t->t_dcom[2]) {
							setname("nice");
							t->t_nice = getn(t->t_dcom[1]);
							lshift(t->t_dcom, 2);
							t->t_dflg |= FNICE;
						} else
							break;
					else {
						t->t_nice = 4;
						lshift(t->t_dcom, 1);
						t->t_dflg |= FNICE;
					}
				else
					break;
			else if (eq(t->t_dcom[0], "nohup"))
				if (t->t_dcom[1]) {
					t->t_dflg |= FNOHUP;
					lshift(t->t_dcom, 1);
				} else
					break;
			else if (eq(t->t_dcom[0], "time"))
				if (t->t_dcom[1]) {
					t->t_dflg |= FTIME;
					lshift(t->t_dcom, 1);
				} else
					break;
			else
				break;
		/*
		 * Check if we have a builtin function and remember which one.
		 */
		bifunc = t->t_dtyp == TCOM ? isbfunc(t) : (struct biltins *) 0;

		/*
		 * We fork only if we are timed, or are not the end of
		 * a parenthesized list and not a simple builtin function.
		 * Simple meaning one that is not pipedout, niced, nohupped,
		 * or &'d.
		 * It would be nice(?) to not fork in some of these cases.
		 */
		if (((t->t_dflg & FTIME) || (t->t_dflg & FPAR) == 0 &&
		     (!bifunc || t->t_dflg & (FPOU|FAND|FNICE|FNOHUP))))
#ifdef VFORK
		    if (t->t_dtyp == TPAR || t->t_dflg&(FREDO|FAND) || bifunc)
#endif
			{ forked++; pid = pfork(t, wanttty); }
#ifdef VFORK
		    else {
			void vffree();
			int ochild, osetintr, ohaderr, odidfds;
			int oSHIN, oSHOUT, oSHDIAG, oOLDSTD, otpgrp;
			int omask;

			/* 
			 * Prepare for the vfork by saving everything
			 * that the child corrupts before it exec's.
			 * Note that in some signal implementations
			 * which keep the signal info in user space
			 * (e.g. Sun's) it will also be necessary to
 			 * save and restore the current sigvec's for
			 * the signals the child touches before it
			 * exec's.
			 */
			omask = sigblock(sigmask(SIGCHLD));
			ochild = child; osetintr = setintr;
			ohaderr = haderr; odidfds = didfds;
			oSHIN = SHIN; oSHOUT = SHOUT;
			oSHDIAG = SHDIAG; oOLDSTD = OLDSTD; otpgrp = tpgrp;
			Vsav = Vdp = 0; Vav = 0;
			pid = vfork();
			if (pid < 0) {
				(void) sigsetmask(omask);
				error("No more processes");
			}
			forked++;
			if (pid) {	/* parent */
				child = ochild; setintr = osetintr;
				haderr = ohaderr; didfds = odidfds;
				SHIN = oSHIN;
				SHOUT = oSHOUT; SHDIAG = oSHDIAG;
				OLDSTD = oOLDSTD; tpgrp = otpgrp;
				xfree(Vsav); Vsav = 0;
				xfree(Vdp); Vdp = 0;
				xfree((char *)Vav); Vav = 0;
				/* this is from pfork() */
				palloc(pid, t);
				(void) sigsetmask(omask);
			} else {	/* child */
				/* this is from pfork() */
				int pgrp;
				bool ignint = 0;

				if (setintr)
					ignint =
					    (tpgrp == -1 && (t->t_dflg&FINT))
					    || gointr && eq(gointr, "-");
				pgrp = pcurrjob ? pcurrjob->p_jobid : getpid();
				child++;
				if (setintr) {
					setintr = 0;
#ifdef notdef
					(void) signal(SIGCHLD, SIG_DFL);
#endif
					(void) signal(SIGINT, ignint ?
						SIG_IGN : ((void (*)())(vffree)));
					(void) signal(SIGQUIT, ignint ?
						SIG_IGN : SIG_DFL);
					if (wanttty >= 0) {
						(void) signal(SIGTSTP, SIG_DFL);
						(void) signal(SIGTTIN, SIG_DFL);
						(void) signal(SIGTTOU, SIG_DFL);
					}
					(void) signal(SIGTERM, parterm);
				} else if (tpgrp == -1 && (t->t_dflg&FINT)) {
					(void) signal(SIGINT, SIG_IGN);
					(void) signal(SIGQUIT, SIG_IGN);
				}
				if (wanttty > 0)
					if (ioctl(FSHTTY, TIOCSPGRP,	/* 003 - GAG */
						(char *)&pgrp)) {
						Perror ("ioctl");
					}
				if (wanttty >= 0 && tpgrp >= 0)
					(void) setpgrp(0, pgrp);
				if (tpgrp > 0)
					tpgrp = 0;
				if (t->t_dflg & FNOHUP)
					(void) signal(SIGHUP, SIG_IGN);
				if (t->t_dflg & FNICE)
					(void) setpriority(PRIO_PROCESS,
						0, t->t_nice);
			}

		    }
#endif
		if (pid != 0) {
			/*
			 * It would be better if we could wait for the
			 * whole job when we knew the last process
			 * had been started.  Pwait, in fact, does
			 * wait for the whole job anyway, but this test
			 * doesn't really express our intentions.
			 */
			if (didfds==0 && t->t_dflg&FPIN) {
				(void) close(pipein[0]);
				(void) close(pipein[1]);
			}
			if ((t->t_dflg & (FPOU|FAND)) == 0)
				pwait();
			break;
		}
		doio(t, pipein, pipeout);
		if (t->t_dflg & FPOU) {
			(void) close(pipeout[0]);
			(void) close(pipeout[1]);
		}

		/*
		 * Perform a builtin function.
		 * If we are not forked, arrange for possible stopping
		 */
		if (bifunc) {
			func(t, bifunc);
			if (forked)
				exitstat();
			break;
		}
		if (t->t_dtyp != TPAR) {
			doexec(t);
			/*NOTREACHED*/
		}
		/*
		 * For () commands must put new 0,1,2 in FSH* and recurse
		 */
		OLDSTD = dcopy(0, FOLDSTD);
		SHOUT = dcopy(1, FSHOUT);
		SHDIAG = dcopy(2, FSHDIAG);
		(void) close(SHIN);
		SHIN = -1;
		didfds = 0;
		wanttty = -1;
		t->t_dspr->t_dflg |= t->t_dflg & FINT;
		execute(t->t_dspr, wanttty);
		exitstat();

	case TFIL:
		t->t_dcar->t_dflg |= FPOU |
		    (t->t_dflg & (FPIN|FAND|FDIAG|FINT));
		execute(t->t_dcar, wanttty, pipein, pv);
		t->t_dcdr->t_dflg |= FPIN |
		    (t->t_dflg & (FPOU|FAND|FPAR|FINT));
		if (wanttty > 0)
			wanttty = 0;		/* got tty already */
		execute(t->t_dcdr, wanttty, pv, pipeout);
		break;

	case TLST:
		if (t->t_dcar) {
			t->t_dcar->t_dflg |= t->t_dflg & FINT;
			execute(t->t_dcar, wanttty);
			/*
			 * In strange case of A&B make a new job after A
			 */
			if (t->t_dcar->t_dflg&FAND && t->t_dcdr &&
			    (t->t_dcdr->t_dflg&FAND) == 0)
				pendjob();
		}
		if (t->t_dcdr) {
			t->t_dcdr->t_dflg |= t->t_dflg & (FPAR|FINT);
			execute(t->t_dcdr, wanttty);
		}
		break;

	case TOR:
	case TAND:
		if (t->t_dcar) {
			t->t_dcar->t_dflg |= t->t_dflg & FINT;
			execute(t->t_dcar, wanttty);
			if ((getn(value("status")) == 0) != (t->t_dtyp == TAND))
				return;
		}
		if (t->t_dcdr) {
			t->t_dcdr->t_dflg |= t->t_dflg & (FPAR|FINT);
			execute(t->t_dcdr, wanttty);
		}
		break;
	}
	/*
	 * Fall through for all breaks from switch
	 *
	 * If there will be no more executions of this
	 * command, flush all file descriptors.
	 * Places that turn on the FREDO bit are responsible
	 * for doing donefds after the last re-execution
	 */
	if (didfds && !(t->t_dflg & FREDO))
		donefds();
}

#ifdef VFORK
void
vffree()
{
	register char **v;

	if (v = gargv)
		gargv = 0, xfree((char *)v);
	if (v = pargv)
		pargv = 0, xfree((char *)v);
	_exit(1);
}
#endif

/*
 * Perform io redirection.
 * We may or maynot be forked here.
 */
doio(t, pipein, pipeout)
	register struct command *t;
	int *pipein, *pipeout;
{
	register char *cp;
	register int flags = t->t_dflg;

	if (didfds || (flags & FREDO))
		return;
	if ((flags & FHERE) == 0) {	/* FHERE already done */
		(void) close(0);
		if (cp = t->t_dlef) {
			cp = globone(Dfix1(cp));
			xfree(cp);
			if (open(cp, 0) < 0)
				Perror(cp);
		} else if (flags & FPIN) {
			(void) dup(pipein[0]);
			(void) close(pipein[0]);
			(void) close(pipein[1]);
		} else if ((flags & FINT) && tpgrp == -1) {
			(void) close(0);
			(void) open("/dev/null", 0);
		} else
			(void) dup(OLDSTD);
	}
	(void) close(1);
	if (cp = t->t_drit) {
		cp = globone(Dfix1(cp));
		xfree(cp);
		if ((flags & FCAT) && open(cp, 1) >= 0)
			(void) lseek(1, (off_t)0, 2);
		else {
			if (!(flags & FANY) && adrof("noclobber")) {
				if (flags & FCAT)
					Perror(cp);
				chkclob(cp);
			}
			if (creat(cp, 0666) < 0)
				Perror(cp);
		}
	} else if (flags & FPOU)
		(void) dup(pipeout[1]);
	else
		(void) dup(SHOUT);

	(void) close(2);
	if (flags & FDIAG)
		(void) dup(1);
	else
		(void) dup(SHDIAG);
	didfds = 1;
}

mypipe(pv)
	register int *pv;
{

	if (pipe(pv) < 0)
		goto oops;
	pv[0] = dmove(pv[0], -1);
	pv[1] = dmove(pv[1], -1);
	if (pv[0] >= 0 && pv[1] >= 0)
		return;
oops:
	error("Can't make pipe");
}

chkclob(cp)
	register char *cp;
{
	struct stat stb;

	if (stat(cp, &stb) < 0)
		return;
	if ((stb.st_mode & S_IFMT) == S_IFCHR)
		return;
	error("%s: File exists", cp);
}
