#ifndef lint
static char	*sccsid = " @(#)main.c	1.2	(ULTRIX)	1/16/86";
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

/************************************************************************
*
*			Modification History
*
*	David Metsky		10-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	main.c		5.2		6/26/85
*
*************************************************************************/

#include <signal.h>
#include "../libI77/fiodefs.h"

int xargc;
char **xargv;

main(argc, argv, arge)
int argc;
char **argv;
char **arge;
{
int sigdie();
long int (*sigf)();
int signum;

xargc = argc;
xargv = argv;

for (signum=1; signum<=16; signum++)
{
	if((sigf=signal(signum, sigdie)) != SIG_DFL) signal(signum, sigf);
}

#ifdef pdp11
	ldfps(01200); /* detect overflow as an exception */
#endif

f_init();
MAIN_();
f_exit();
return 0;
}

struct action {
	char *mesg;
	int   core;
} sig_act[16] = {
	{"Hangup", 0},			/* SIGHUP  */
	{"Interrupt!", 0},		/* SIGINT  */
	{"Quit!", 1},			/* SIGQUIT */
	{"Illegal ", 1},		/* SIGILL  */
	{"Trace Trap", 1},		/* SIGTRAP */
	{"IOT Trap", 1},		/* SIGIOT  */
	{"EMT Trap", 1},		/* SIGEMT  */
	{"Arithmetic Exception", 1},	/* SIGFPE  */
	{ 0, 0},			/* SIGKILL */
	{"Bus error", 1},		/* SIGBUS  */
	{"Segmentation violation", 1},	/* SIGSEGV */
	{"Sys arg", 1},			/* SIGSYS  */
	{"Open pipe", 0},		/* SIGPIPE */
	{"Alarm", 0},			/* SIGALRM */
	{"Terminated", 0},		/* SIGTERM */
	{"Sig 16", 0},			/* unassigned */
};

struct action act_fpe[] = {
	{"Integer overflow", 1},
	{"Integer divide by 0", 1},
	{"Floating point overflow trap", 1},
	{"Floating divide by zero trap", 1},
	{"Floating point underflow trap", 1},
	{"Decimal overflow", 1},
	{"Subscript range", 1},
	{"Floating point overflow", 0},
	{"Floating divide by zero", 0},
	{"Floating point underflow", 0},
};

struct action act_ill[] = {
	{"addr mode", 1},
	{"instruction", 1},
	{"operand", 0},
};

#if	vax
sigdie(s, t, sc)
int s; int t; struct sigcontext *sc;

#else	pdp11
sigdie(s, t, pc)
int s; int t; long pc;

#endif
{
extern unit units[];
register struct action *act = &sig_act[s-1];
/* print error message, then flush buffers */

if (s == SIGHUP || s == SIGINT || s == SIGQUIT)
	signal(s, SIG_IGN);	/* don't allow it again */
else
	signal(s, SIG_DFL);	/* shouldn't happen again, but ... */

if (act->mesg)
	{
	fprintf(units[STDERR].ufd, "*** %s", act->mesg);
	if (s == SIGFPE)
		{
		if (t >= 1 && t <= 10)
			fprintf(units[STDERR].ufd, ": %s", act_fpe[t-1].mesg);
		else
			fprintf(units[STDERR].ufd, ": Type=%d?", t);
		}
	else if (s == SIGILL)
		{
		if (t == 4) t = 2;	/* 4.0bsd botch */
		if (t >= 0 && t <= 2)
			fprintf(units[STDERR].ufd, "%s", act_ill[t].mesg);
		else
			fprintf(units[STDERR].ufd, "compat mode: Code=%d", t);
		}
	putc('\n', units[STDERR].ufd);
	}
f77_abort( s, act->core );
}
