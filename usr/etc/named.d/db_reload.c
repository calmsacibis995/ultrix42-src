#ifndef lint
static	char	*sccsid = "@(#)db_reload.c	4.2	(ULTRIX)	11/15/90";
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
 * static char sccsid[] = "@(#)db_reload.c	4.15 (Berkeley) 2/28/88";
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
 * 17-May-89	logcher
 *	Added BIND 4.8.
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#ifdef ULTRIXFUNC
#include <sys/stat.h>
#endif ULTRIXFUNC
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "db.h"

extern time_t	resettime;

/*
 * Flush and reload data base.
 */

db_reload()
{
	int looper;
	extern char *bootfile;

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"reload()\n");
#endif
	syslog(LOG_NOTICE, "reloading nameserver\n");

	abort_loads();
	qflush();
	sqflush();
	fwdtab_free();
	if (hashtab != NULL)
		db_free(hashtab);
	hashtab = NULL;
	if (fcachetab != NULL)
		db_free(fcachetab);
	fcachetab = NULL;
	db_inv_free();
	fwdtab_free();
	for(looper = 0;  looper < nzones; looper++) {
		free_files(zones[looper].z_files);
	}
	ns_init(bootfile);
	time(&resettime);
}

db_free(htp)
	struct hashbuf *htp;
{
	register struct databuf *dp, *nextdp;
	register struct namebuf *np, *nextnp;
	struct namebuf **npp, **nppend;

	npp = htp->h_tab;
	nppend = npp + htp->h_size;
	while (npp < nppend) {
	    for (np = *npp++; np != NULL; np = nextnp) {
		if (np->n_hash != NULL)
			db_free(np->n_hash);
		(void) free((char *)np->n_dname);
		for (dp = np->n_data; dp != NULL; ) {
			nextdp = dp->d_next;
			(void) free((char *)dp);
			dp = nextdp;
		}
		nextnp = np->n_next;
		free((char *)np);
	    }
	}
	(void) free((char *)htp);
}

db_inv_free()
{
	register struct invbuf *ip;
	register int i, j;

	for (i = 0; i < INVHASHSZ; i++)
		for (ip = invtab[i]; ip != NULL; ip = ip->i_next)
			for (j = 0; j < INVBLKSZ; j++)
				ip->i_dname[j] = NULL;
}

fwdtab_free()
{
	extern	struct fwdinfo *fwdtab;
	struct fwdinfo *fp, *nextfp;

	for (fp = fwdtab; fp != NULL; fp = nextfp) {
		nextfp = fp->next;
		free((char *)fp);
	}
	fwdtab = NULL;
}
