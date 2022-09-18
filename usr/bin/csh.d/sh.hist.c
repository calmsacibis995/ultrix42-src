#ifndef lint
static char *sccsid = "@(#)sh.hist.c	4.2  (ULTRIX)        8/13/90";
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
/* $Header: sh.hist.c,v 1.4 87/04/06 14:24:46 dce Exp $ */

#include "sh.h"

/*
 * C shell
 *
 * Modification History
 *
 * 003 - Bob Fontaine	- Thu Aug  9 10:12:00 EDT 1990
 *	 Added test in phist routine so that events with negative event
 *	 numbers and not printed when a history list is displayed.
 *	 QAR #958
 *
 * 002 - Bob Fontaine	- Fri Jun 22 09:53:01 EDT 1990
 *	 Changed call to internal printf function to csh_printf to avoid
 *	 confusion with stdio library routine.
 *
 * 001 - Gary A. Gaudet - Tue Jan  2 11:41:26 EST 1990
 *	added some castings
 */

savehist(sp)
	struct wordent *sp;
{
	register struct Hist *hp, *np;
	register int histlen = 0;
	char *cp;

	/* throw away null lines */
	if (sp->next->word[0] == '\n')
		return;
	cp = value("history");
	if (*cp) {
		register char *p = cp;

		while (*p) {
			if (!digit(*p)) {
				histlen = 0;
				break;
			}
			histlen = histlen * 10 + *p++ - '0';
		}
	}
	for (hp = &Histlist; np = hp->Hnext;)
		if (eventno - np->Href >= histlen || histlen == 0)
			hp->Hnext = np->Hnext, hfree(np);
		else
			hp = np;
	(void) enthist(eventno + 1, sp, 1);
}

struct Hist *
enthist(event, lp, docopy)
	int event;
	register struct wordent *lp;
	bool docopy;
{
	register struct Hist *np;

	np = (struct Hist *) xalloc((unsigned)sizeof *np);	/* 001 - GAG */
	np->Hnum = np->Href = event;
	if (docopy)
		copylex(&np->Hlex, lp);
	else {
		np->Hlex.next = lp->next;
		lp->next->prev = &np->Hlex;
		np->Hlex.prev = lp->prev;
		lp->prev->next = &np->Hlex;
	}
	np->Hnext = Histlist.Hnext;
	Histlist.Hnext = np;
	return (np);
}

hfree(hp)
	register struct Hist *hp;
{

	freelex(&hp->Hlex);
	xfree((char *)hp);
}

dohist(vp)
	char **vp;
{
	int n, rflg = 0, hflg = 0;
	if (getn(value("history")) == 0)
		return;
	if (setintr)
		(void) sigsetmask(sigblock(0) & ~sigmask(SIGINT));
 	while (*++vp && **vp == '-') {
 		char *vp2 = *vp;
 
 		while (*++vp2)
 			switch (*vp2) {
 			case 'h':
 				hflg++;
 				break;
 			case 'r':
 				rflg++;
 				break;
 			case '-':	/* ignore multiple '-'s */
 				break;
 			default:
 				csh_printf("Unknown flag: -%c\n", *vp2); /* 002 RNF */
 				error("Usage: history [-rh] [# number of events]");
			}
	}
	if (*vp)
		n = getn(*vp);
	else {
		n = getn(value("history"));
	}
	dohist1(Histlist.Hnext, &n, rflg, hflg);
}

dohist1(hp, np, rflg, hflg)
	struct Hist *hp;
	int *np, rflg, hflg;
{
	bool print = (*np) > 0;
top:
	if (hp == 0)
		return;
	(*np)--;
	hp->Href++;
	if (rflg == 0) {
		dohist1(hp->Hnext, np, rflg, hflg);
		if (print)
			phist(hp, hflg);
		return;
	}
	if (*np >= 0)
		phist(hp, hflg);
	hp = hp->Hnext;
	goto top;
}

phist(hp, hflg)
	register struct Hist *hp;
	int hflg;
{

	if(hp->Hnum > 0)			/* 003 RNF */
	{
		if (hflg == 0)
			csh_printf("%6d\t", hp->Hnum);	/* 002 RNF */
		prlex(&hp->Hlex);
	}
}
