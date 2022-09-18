#ifndef lint
static	char	*sccsid = "@(#)makeseed.c	4.1	(ULTRIX)	7/17/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * This code is modelled after the VMS auto-gen password random seed
 * generator.
 *
 * 7 June, 1989 - D. Long
 */
#include <signal.h>
#include <sys/time.h>

static int timeout_flag;

static void handler(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
	timeout_flag = 0;
}

makeseed(seed, size)
char *seed;
int size;
{
	register int i;
	int j, count, x;
	struct itimerval value, ovalue;
	struct sigvec vector;

	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 10000;
	vector.sv_handler = handler;
	vector.sv_mask = 0;
	vector.sv_onstack = 0;
	sigvec(SIGALRM, &vector, 0);
	for(count=0; count < size; count++) {
		x = 0;
		for(j=0; j < 4; j++) {
			timeout_flag = 1;
			setitimer(ITIMER_REAL, &value, &ovalue);
			for(i=0; timeout_flag; i++) ;
			x = (x<<2) | (i&3);
		}
		seed[count] = x;
	}
}
