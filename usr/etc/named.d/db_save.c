#ifndef lint
static	char	*sccsid = "@(#)db_save.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1984-1988 by			*
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
 * Copyright (c) 1986 Regents of the University of California
 *	All Rights Reserved
 * static char sccsid[] = "@(#)db_save.c	4.13 (Berkeley) 2/17/88";
 */

/*
 * Modification History:
 *
 * 18-Jan-88	logcher
 *	Added BIND 4.7.2.
 *
 * 26-Jan-88	logcher
 *	Added BIND 4.7.3.
 *
 * 18-May-89	logcher
 *	Added BIND 4.8.
 */

/*
 * Buffer allocation and deallocation routines.
 */

#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include <arpa/nameser.h>
#include "db.h"

#ifdef DEBUG
extern int debug;
extern FILE *ddt;
#endif

extern char *strcpy();

/*
 * Allocate a name buffer & save name.
 */
struct namebuf *
savename(name)
	char *name;
{
	register struct namebuf *np;

	np = (struct namebuf *) malloc(sizeof(struct namebuf));
	if (np == NULL) {
		syslog(LOG_ERR, "savename: %m");
		exit(1);
	}
	np->n_dname = savestr(name);
	np->n_next = NULL;
#ifdef ULTRIXFUNC
	np->n_loaddb = NULL;
	np->n_mark = 0;
#endif ULTRIXFUNC
	np->n_data = NULL;
	np->n_hash = NULL;
	return (np);
}

/*
 * Allocate a data buffer & save data.
 */
#ifdef AUTHEN
struct databuf *
savedata(class, type, ttl, authentype, authenver, data, size)
	int class, type;
	u_long ttl;
	int authentype;
	int authenver;
	char *data;
	int size;
{
#else AUTHEN
struct databuf *
savedata(class, type, ttl, data, size)
	int class, type;
	u_long ttl;
	char *data;
	int size;
{
#endif AUTHEN
	register struct databuf *dp;

	if (type == T_NS)
		dp = (struct databuf *) 
		    malloc((unsigned)DATASIZE(size)+sizeof(u_long));
	else
		dp = (struct databuf *) malloc((unsigned)DATASIZE(size));
	if (dp == NULL) {
		syslog(LOG_ERR, "savedata: %m");
		exit(1);
	}
	dp->d_next = NULL;
	dp->d_type = type;
	dp->d_class = class;
	dp->d_ttl = ttl;
	dp->d_size = size;
	dp->d_mark = 0;
	dp->d_flags = 0;
	dp->d_nstime = 0;
#ifdef AUTHEN
	dp->d_authen_type = authentype;
	dp->d_authen_ver = authenver;
#endif AUTHEN
	bcopy(data, dp->d_data, dp->d_size);
	return (dp);
}

int hashsizes[] = {	/* hashtable sizes */
	2,
	11,
	113,
	337,
	977,
	2053,
	4073,
	8011,
	16001,
	0
};

/*
 * Allocate a data buffer & save data.
 */
struct hashbuf *
savehash(oldhtp)
	register struct hashbuf *oldhtp;
{
	register struct hashbuf *htp;
	register struct namebuf *np, *nnp, **hp;
	register int n;
	int newsize;

	if (oldhtp == NULL)
		newsize = hashsizes[0];
	else {
		for (n = 0; newsize = hashsizes[n++]; )
			if (oldhtp->h_size == newsize) {
				newsize = hashsizes[n];
				break;
			}
		if (newsize == 0)
			newsize = oldhtp->h_size * 2 + 1;
	}
#ifdef DEBUG
	if(debug > 3)
		fprintf(ddt, "savehash GROWING to %d\n", newsize);
#endif
	htp = (struct hashbuf *) malloc((unsigned)HASHSIZE(newsize));
	if (htp == NULL) {
		syslog(LOG_ERR, "savehash: %m");
		exit(1);
	}
	htp->h_size = newsize;
	bzero((char *) htp->h_tab, newsize * sizeof(struct hashbuf *));

	if (oldhtp == NULL) {
		htp->h_cnt = 0;
		return (htp);
	}
#ifdef DEBUG
	if (debug > 3)
		fprintf(ddt,"savehash(%#x) cnt=%d, sz=%d, newsz=%d\n",
			oldhtp, oldhtp->h_cnt, oldhtp->h_size, newsize);
#endif
	htp->h_cnt = oldhtp->h_cnt;
	for (n = 0; n < oldhtp->h_size; n++) {
		for (np = oldhtp->h_tab[n]; np != NULL; np = nnp) {
			nnp = np->n_next;
			hp = &htp->h_tab[np->n_hashval % htp->h_size];
			np->n_next = *hp;
			*hp = np;
		}
	}
	free((char *) oldhtp);
	return (htp);
}

/*
 * Allocate an inverse query buffer.
 */
struct invbuf *
saveinv()
{
	register struct invbuf *ip;

	ip = (struct invbuf *) malloc(sizeof(struct invbuf));
	if (ip == NULL) {
		syslog(LOG_ERR, "saveinv: %m");
		exit(1);
	}
	ip->i_next = NULL;
	bzero((char *)ip->i_dname, sizeof(ip->i_dname));
	return (ip);
}

/*
 * Make a copy of a string and return a pointer to it.
 */
char *
savestr(str)
	char *str;
{
	char *cp;

	cp = malloc((unsigned)strlen(str) + 1);
	if (cp == NULL) {
		syslog(LOG_ERR, "savestr: %m");
		exit(1);
	}
	(void) strcpy(cp, str);
	return (cp);
}
