#ifndef lint
static char sccsid[] = "@(#)us_crs.c	4.1 (decvax!larry) 7/2/90";
#endif

 
/*
 * Whenever a command file (i.e. C.*****) file is spooled by uucp,
 * creates an entry in the beginning of "R_stat" file. 
 * Future expansion: An R_stat entry may be created by, e.g.
 * uux, rmail, or any command using uucp.
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */


/***************
 * Mods:
 *	decvax!larry - store data in binary format 
 **************/



/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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


#include "uucp.h"
#ifdef UUSTAT
#include <sys/types.h>
#include "uust.h"


us_crs(cfile)
char *cfile;
{
	register FILE *fq;
	register short i;
	char *name, *s, buf[BUFSIZ];
	struct us_rsf u;
	long time();
 
	DEBUG(6, "Enter us_crs, cfile: %s\n", cfile);
	clear(&u,sizeof(u));
	if ((fq = fopen(R_stat, "a+")) == NULL) {
		DEBUG(3, "fopen of %s failed\n", s);
		return(FAIL);
	}

	/*
	 * manufacture a new entery
	 */
	name = cfile + strlen(cfile) - 4;
	strncpy(u.jobn, name, 4);
	u.jobn[4] = '\0';
	u.qtime = u.stime = time((long *) 0);
	u.ustat = USR_QUEUED;
	strncpy(u.user, User, NAME7);
	u.user[NAME7-1] = '\0';
	strncpy(u.rmt, Rmtname, 7);
	u.user[6] = '\0';
	fwrite(&u, sizeof(u), 1, fq);
	fflush(fq);
	fclose(fq);
	return(0);
}
clear(p, c)
register char *p;
register int c;
{
	register i;

	for(i=0;i<c;i++)
		*p++ = 0;
}
#endif
