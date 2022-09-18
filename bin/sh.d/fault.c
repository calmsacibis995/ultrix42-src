#ifndef lint
static char sccsid[] = "@(#)fault.c	4.2 (ULTRIX) 8/13/90";
/* Original ID:  "@(#)fault.c	4.3 8/11/83" */
#endif

#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */
 /***********************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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

 /*****
   Modification History
   ~~~~~~~~~~~~~~~~~~~~

   04	David Lindner Wed Jun  6 11:18:45 EDT 1990
	- Changed wasintr from declaration to definition.
	- Replaced calls to signal with calls to nsiganl.
	- Removed old signal handling fix.
	- Added definition of nsignal, which is a call to sigvec with 
	  the SV_INTERRUPT bit set.

   03	David Lindner Wed Aug 30 14:46:59 EDT 1989
	- Added 3.1 bug patch to fault routine.

   02	David Lindner Mon Aug 21 16:06:09 EDT 1989
	- Fixed trap read interrupt bug.

   01 	David Lindner Fri Jun  9 14:38:24 EDT 1989
	- Added check for interrupt function, and set of wasintr flag.
	- Added comment header.

 *****/

#include	"defs.h"

STRING		trapcom[MAXTRAP];
BOOL		trapflg[MAXTRAP];
BOOL		trapjmp[MAXTRAP];
int		wasintr;		/* DJL 01 04 */
void            (*nsignal())();		/* DJL 04 */

/* ========	fault handling routines	   ======== */


/*
 * DJL 01 04
 * Increment wasintr on a SIGINT.
 */

VOID	fault(sig)
	REG INT		sig;
{
	REG INT		flag;

	stdsigs();			/* DJL 03 */
	IF sig==MEMF
	THEN	IF setbrk(brkincr) == -1
		THEN	error(nospace);
		FI
	ELIF sig==ALARM
	THEN	IF flags&waiting
		THEN	done();
		FI
	ELSE	flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
		if (sig == SIGINT)
			wasintr++;
	FI
}

stdsigs()
{
	ignsig(QUIT);
	getsig(INTR);
	getsig(MEMF);
	getsig(ALARM);
}

ignsig(n)
{
        REG INT         s, i;

        IF (s=nsignal(i=n,1))==0		/* DJL 04 */
        THEN    trapflg[i] |= SIGMOD;
        FI
	s&=01;
        return(s);
}

getsig(n)
{
	REG INT		i;

	IF trapflg[i=n]&SIGMOD ORF ignsig(i)==0
	THEN	nsignal(i,fault);		/* DJL 04 */
	FI
}

oldsigs()
{
	REG INT		i;
	REG STRING	t;

	i=MAXTRAP;
	WHILE i--
	DO  t=trapcom[i];
	    IF t==0 ORF *t
	    THEN clrsig(i);
	    FI
	    trapflg[i]=0;
	OD
	trapnote=0;
}

clrsig(i)
	INT		i;
{
	free(trapcom[i]); trapcom[i]=0;
	IF trapflg[i]&SIGMOD
	THEN	nsignal(i,fault);		/* DJL 04 */
		trapflg[i] &= ~SIGMOD;
	FI
}

/*
 * DJL 02 04
 */

chktrap()
{
	/* check for traps */
	REG INT		i=MAXTRAP;
	REG STRING	t;

	trapnote &= ~TRAPSET;
	WHILE --i
	DO IF trapflg[i]&TRAPSET
	   THEN trapflg[i] &= ~TRAPSET;
		IF t=trapcom[i]
		THEN	INT	savxit=exitval;
			execexp(t,0);
			exitval=savxit; exitset();
		FI
	   FI
	OD
}

/*
 * DJL 04
 * New signal routine.
 * This is essentially the signal routine in libc, however I made 
 * the system calls non-restartable, by setting the SV_INTERRUPT bit.
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
