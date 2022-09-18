#ifndef lint
static CHTYPE *sccsid = "@(#)fault.c	4.1 (ULTRIX) 7/17/90";
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
/*
 *
 *   Modification History:
 *
 * 003 - David Lindner Fri Nov  3 10:11:58 EST 1989
 *       Added a new signal routine nsignal(), so system calls such
 *       as read, wait, and flock, do not restart after interruption.
 *       Changed all the calls from signal to nsignal.
 *	 Added new signals.
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	 i18n version of csh
 *
 *
 *
 */

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
void             (*nsignal())();        /* DJL 003 */

CHTYPE	*trapcom[MAXTRAP];		/* DJL 003 */
BOOL	trapflg[MAXTRAP] =
{
	0,
	0,	/* 1 hangup */
	0,	/* 2 interrupt */
	0,	/* 3 quit */
	0,	/* 4 illegal instr */
	0,	/* 5 trace trap */
	0,	/* 6 IOT */
	0,	/* 7 EMT */
	0,	/* 8 float pt. exception */
	0,	/* 9 kill */
	0, 	/* 10 bus error */
	0,	/* 11 segmentation violation */
	0,	/* 12 bad sys call */
	0,	/* 13 bad pipe call */
	0,	/* 14 alarm */
	0, 	/* 15 software termination */
	0,	/* 16 urgent condition on socket */
	0,	/* 17 stop */
	0,	/* 18 stop from keyboard */
	0,	/* 19 continue after stop */
	0,	/* 20 child status has changed */
	0,	/* 21 background read from control terminal */
	0,	/* 22 background write from control terminal */
	0,	/* 23 I/O possible on descriptor */
	0,	/* 24 cpu time limit exceeded */
	0,	/* 25 file size limit exceeded */
	0,	/* 26 virtual time alarm */
	0,	/* 27 profiling time alarm */
	0,	/* 28 window size change */
	0,	/* 29 system V record locking */
	0,	/* 30 user defined signal */
	0	/* 31 user defined signal */
};

void 	(*(sigval[]))() =	/* GAG */ /* DJL 003 */
{
	0,
	done,   /* 1 hangup */
	fault,	/* 2 interrupt */
	fault,	/* 3 quit */
	done,	/* 4 illegal instr */
	done,	/* 5 trace trap */
	done,	/* 6 IOT */
	done,	/* 7 EMT */
	done,	/* 8 float pt. exception */
	0,	/* 9 kill */
	done,	/* 10 bus error */
	done,	/* 11 segmentation violation */
	done,	/* 12 bad sys call */
	done,	/* 13 bad pipe call */
	fault,	/* 14 alarm */
	fault,	/* 15 software termination */
	fault,	/* 16 urgent condition on socket */
	0,	/* 17 stop */
	0,	/* 18 stop from keyboard */
	0,	/* 19 continue after stop */
	0,	/* 20 child status has changed */
	0,	/* 21 background read from control terminal */
	0,	/* 22 background write from control terminal */
	0,	/* 23 I/O possible on descriptor */
	done,	/* 24 cpu time limit exceeded */
	done,	/* 25 file size limit exceeded */
	0,	/* 26 virtual time alarm */
	0,	/* 27 profiling time alarm */
	0,	/* 28 window size change */
	0,	/* 29 system V record locking */
	0,	/* 30 user defined signal */
	0	/* 31 user defined signal */
};

/* ========	fault handling routines	   ======== */

void		/* DAG */
fault(sig)
register int	sig;
{
	register int	flag;

	nsignal(sig, fault);
	if (sig == SIGSEGV)
	{
		if (setbrk(brkincr) == -1)
			error(nospace);
	}
	else if (sig == SIGALRM)
	{
		if (flags & waiting)
			done();
	}
	else
	{
		flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
		if (sig == SIGINT)
			wasintr++;
	}
}

stdsigs()
{
	setsig(SIGHUP);
	setsig(SIGINT);
	ignsig(SIGQUIT);
	setsig(SIGILL);
	setsig(SIGTRAP);
	setsig(SIGIOT);
	setsig(SIGEMT);
	setsig(SIGFPE);
	setsig(SIGBUS);
	nsignal(SIGSEGV, fault);
	setsig(SIGSYS);
	setsig(SIGPIPE);
	setsig(SIGALRM);
	setsig(SIGTERM);
	setsig(SIGURG);
	setsig(SIGSTOP);
	setsig(SIGTSTP);
	setsig(SIGCONT);
	setsig(SIGCHLD);
	setsig(SIGTTIN);
	setsig(SIGTTOU);
	setsig(SIGIO);
	setsig(SIGXCPU);
	setsig(SIGXFSZ);
	setsig(SIGVTALRM);
	setsig(SIGPROF);
	setsig(SIGWINCH);
	setsig(SIGLOST);
	setsig(SIGUSR1);
	setsig(SIGUSR2);
}

ignsig(n)
{
	register int	s, i;

	if ((i = n) == SIGSEGV)
	{
		clrsig(i);
		failed(badtrap, "cannot trap 11");
	}
	else if ((s = (nsignal(i, SIG_IGN) == SIG_IGN)) == 0)
	{
		trapflg[i] |= SIGMOD;
	}
	return(s);
}

getsig(n)
{
	register int	i;

	if (trapflg[i = n] & SIGMOD || ignsig(i) == 0)
		nsignal(i, fault);
}


setsig(n)
{
	register int	i;

	if (ignsig(i = n) == 0)
		nsignal(i, sigval[i]);
}

oldsigs()
{
	register int	i;
	register CHTYPE	*t;

	i = MAXTRAP;
	while (i--)
	{
		t = trapcom[i];
		if (t == 0 || *t)
			clrsig(i);
		trapflg[i] = 0;
	}
	trapnote = 0;
}

clrsig(i)
int	i;
{
	free(trapcom[i]);
	trapcom[i] = 0;
	if (trapflg[i] & SIGMOD)
	{
		trapflg[i] &= ~SIGMOD;
		nsignal(i, sigval[i]);
	}
}

/*
 * check for traps
 */
chktrap()
{
	register int	i = MAXTRAP;
	register CHTYPE	*t;

	trapnote &= ~TRAPSET;
	while (--i)
	{
		if (trapflg[i] & TRAPSET)
		{
			trapflg[i] &= ~TRAPSET;
			if (t = trapcom[i])
			{
				int	savxit = exitval;

				execexp(t, 0);
				exitval = savxit;
				exitset();
			}
		}
	}
}

/*
 * DJL 003
 * New signal routine.
 * This is essentially the signal routine in libc, however I made 
 * the system calls non-restartable.
 */
void (*
nsignal(s, a))()
	register int s;
	void  (*a)();
{
	struct sigvec osv, sv;
	static int mask[NSIG];		/* saved across calls */
	static int flags[NSIG];		/* saved across calls */

	if (s < 0 || s >= NSIG)
		return (BADSIG);

	sv.sv_handler = a;
	sv.sv_mask = mask[s];
	sv.sv_flags = flags[s];
	sv.sv_flags |= SV_OLDSIG; 	/* Mark as called from signal() */
	sv.sv_flags |= SV_INTERRUPT;	/* terminate system calls */
	if (sigvec(s, &sv, &osv))	/* try it in one syscall */
		return (BADSIG);
	if (sv.sv_mask != osv.sv_mask || sv.sv_flags != osv.sv_flags) {
	/* something is different or 1st time thru - save off mask and flags */
		mask[s]     = osv.sv_mask;
		sv.sv_mask  = osv.sv_mask;
		flags[s]    = osv.sv_flags;
		sv.sv_flags = osv.sv_flags;
		sv.sv_flags |= SV_OLDSIG; 	/* Mark as called from signal() */
		sv.sv_flags |= SV_INTERRUPT;    /* terminate system calls */
		if (sigvec(s, &sv, (struct sigvec *)0) < 0)
			return (BADSIG);
	}
	return (osv.sv_handler);
}
