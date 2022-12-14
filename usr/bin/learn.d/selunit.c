#ifndef lint
static	char	*sccsid = "@(#)selunit.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * Modification history
 *
 * 15 Sep 1988 -- D. Long
 *	Fixed null pointer problem for PMAX.
 */

#include "stdio.h"
#include "lrnref.h"

int	nsave	= 0;
int	review	= 0;

selunit()
{
	static char dobuff[50];
	static char saved[20];
	char fnam[80], s[80], zb[200];
	char posslev[20][20];
	int diff[20], i, k, m, n, best, alts;
	char *getlesson();
	FILE *f;

	if (again) {
		again = 0;
		if (todo=getlesson()) {
			if (!review)
				unsetdid(todo);
			return;
		}
		wrapup(1);
	}
	while (ask) {
		printf("What lesson? ");
		fflush(stdout);
		if (gets(dobuff) == NULL)
			wrapup(1);
		if (strcmp(dobuff, "bye") == 0)
			wrapup(1);
		level = dobuff;
		if (todo=getlesson())
			return;
	}
	alts = 0;
retry:
	f = scrin;			/* use old lesson to find next */
	if (f==NULL) {
		sprintf(fnam, "%s/%s/L%s", direct, sname, level);
		f = fopen(fnam, "r");
		if (f==NULL) {
			perror(fnam);
			fprintf(stderr, "Selunit:  no script for lesson %s.\n", level);
			wrapup(1);
		}
		while (fgets(zb, 200, f)) {
			trim(zb);
			if (strcmp(zb, "#next")==0)
				break;
		}
	}
	if (feof(f)) {
		printf("Congratulations; you have finished this sequence.\n");
		fflush(stdout);
		todo = 0;
		return;
	}
	for(i=0; fgets(s, 80, f); i++) {
		sscanf(s, "%s %d", posslev[i], &diff[i]);
	}
	best = -1;
	/* cycle through lessons from random start */
	/* first try the current place, failing that back up to
	     last place there are untried alternatives (but only one backup) */
	n = grand()%i;
	for(k=0; k<i; k++) {
		m = (n+k)%i;
		if (already(posslev[m]))
			continue;
		if (best<0)
			best = m;
		alts++;				/* real alternatives */
		if (abs(diff[m]-speed) < abs(diff[best]-speed))
			best = m;
	}
	if (best < 0 && nsave) {
		nsave--;
		strcpy(level, saved);
		goto retry;
	}
	if (best < 0) {
		/* lessons exhausted or missing */
		printf("Sorry, there are no alternative lessons at this stage.\n");
		printf("See someone for help.\n");
		fflush(stdout);
		todo = 0;
		return;
	}
	strcpy (dobuff, posslev[best]);
	if (alts>1) {
		nsave = 1;
		strcpy(saved, level);
	}
	todo = dobuff;
	fclose(f);
}

abs(x)
{
	return(x>=0 ? x : -x);
}

grand()
{
	static int garbage;
	int a[2], b;

	time(a);
	b = a[1]+10*garbage++;
	return(b&077777);
}
