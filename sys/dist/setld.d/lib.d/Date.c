/*	Date.c -
 *		routines for DateT
 *
 *			Copyright (c) 1989 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
 *	mods:
 *	000	19-jun-1989	ccb
 *		pulled from inv.c
*/

#ifndef lint
static	char *sccsid = "@(#)Date.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<time.h>
#include	<stdio.h>
#include	"setld.h"

#define	I_DATFMT	"%d/%d/%d"

/*LINTLIBRARY*/

/*	char	*DateFormat() -
 *		format a time_t as a date in inventory format.
 *
 *	given:	time_t t - the time to format
 *	does:	formats the time into a static buffer
 *	return:	a pointer to the (static) buffer
*/

char *DateFormat(t)
time_t t;
{
	struct tm	*tms;
	static DateT	ds;

	tms = localtime( (long *) &t );
	(void) sprintf( ds, I_DATFMT, tms->tm_mon + 1, tms->tm_mday,
		tms->tm_year );
	return( (char *) ds );
}

