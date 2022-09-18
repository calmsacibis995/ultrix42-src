
/*	@(#)uname.c	1.2	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984 - 1989 by			*
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
 * 7 Nov 90  -- Piyanai
 *      Changed printed string length to be __SYS_NMLN instead of 9.
 *      This solves the PCTS failed test. 
 * Early 90  -- ?
 *      Original source from AT&T release 2.
 */

#include	<stdio.h>
#include	<sys/utsname.h>

struct utsname	unstr, *un;

main(argc, argv)
char **argv;
int argc;
{
	register i;
	int	sflg=1, nflg=0, rflg=0, vflg=0, mflg=0, errflg=0;
	int	optlet;

	un = &unstr;
	uname(un);

	while((optlet=getopt(argc, argv, "asnrvm")) != EOF) switch(optlet) {
	case 'a':
		sflg++; nflg++; rflg++; vflg++; mflg++;
		break;
	case 's':
		sflg++;
		break;
	case 'n':
		nflg++;
		break;
	case 'r':
		rflg++;
		break;
	case 'v':
		vflg++;
		break;
	case 'm':
		mflg++;
		break;
	case '?':
		errflg++;
	}
	if(errflg) {
		fprintf(stderr, "usage: uname [-snrvma]\n");
		exit(1);
	}
	if(nflg | rflg | vflg | mflg) sflg--;
	if(sflg)
		fprintf(stdout, "%.*s", __SYS_NMLN, un->sysname);
	if(nflg) {
		if(sflg) putchar(' ');
		fprintf(stdout, "%.*s", __SYS_NMLN, un->nodename);
	}
	if(rflg) {
		if(sflg | nflg) putchar(' ');
		fprintf(stdout, "%.*s", __SYS_NMLN, un->release);
	}
	if(vflg) {
		if(sflg | nflg | rflg) putchar(' ');
		fprintf(stdout, "%.*s", __SYS_NMLN, un->version);
	}
	if(mflg) {
		if(sflg | nflg | rflg | vflg) putchar(' ');
		fprintf(stdout, "%.*s", __SYS_NMLN, un->machine);
	}
	putchar('\n');
	exit(0);
}
