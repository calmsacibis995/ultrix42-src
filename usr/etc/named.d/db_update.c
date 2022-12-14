#ifndef lint
static	char	*sccsid = "@(#)db_update.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1990 by			*
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
 * static char sccsid[] = "@(#)db_update.c	4.16 (Berkeley) 2/28/88";
 */

/*
 * Modification History:
 *
 * 18-Jan-88	logcher
 *	Added BIND 4.7.2 with ifdef-ed MIT Hesiod support.
 *
 * 26-Jan-88	logcher
 *	Added BIND 4.7.3.
 *
 * 08-Mar-88	logcher
 *	Added bug fix to db_update to check type and class after XXX
 *	per bind-test mailing list dated 19-Feb-88
 *
 * 08-Jun-88	logcher
 *	Added the correct MIT Hesiod support.
 *
 * 11-Nov-88	logcher
 *	Updated with V3.0 changes.
 *
 * 17-May-89	logcher
 *	Added BIND 4.8.
 *
 * 10-Jan-90	sue
 *	Incorporated Hesiod changes from MIT to implement multiple
 *	"character strings" in a TXT record according to RFC1035.
 */

#include <sys/types.h>
#include <sys/socket.h>
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

extern struct timeval	tt;
extern FILE *ddt;
extern struct zoneinfo zones[];
extern struct sockaddr_in from_addr;	/* Source addr of last packet */
extern int needs_prime_cache;

int	max_cache_ttl = (7*24*60*60);	/* ONE_WEEK maximum ttl */
int	min_cache_ttl = (2*60);		/* 5 minute minimum ttl */

/*
 * Update data base. Flags control the action.
 * Inverse query tables modified.
 */
db_update(name, odp, newdp, flags, htp)
	char name[];
	struct databuf *odp, *newdp;
	int flags;
	struct hashbuf *htp;
{
	register struct namebuf *np;
	register struct databuf *dp, *pdp;
	char *fname;
        int foundRR = 0;

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"db_update(%s, 0x%x, 0x%x, 0%o, 0x%x)%s\n",
		    name, odp, newdp, flags, htp,
		    (odp && (odp->d_flags&DB_F_HINT)) ? " hint":"" );
#endif
	np = nlookup(name, &htp, &fname, newdp != NULL);
	if (np == NULL || fname != name)
		return (NONAME);

        /* Reflect certain updates in hint cache also... */
	/* Don't stick data we are authoritative for in hints. */
        if (!(flags & DB_NOHINTS) && (odp != NULL) &&
	    (odp->d_zone <= 0) && !(odp->d_flags & DB_F_HINT) &&
            ((name[0] == '\0' && odp->d_type == T_NS) ||
	     (odp->d_type == T_A)))
        {
		register struct databuf *dp;
#ifdef DEBUG
		if (debug >= 3)
			fprintf(ddt,"db_update: hint '%s' %d\n",
				name, odp->d_ttl);
#endif
#ifdef AUTHEN
		dp = savedata(odp->d_class, odp->d_type, odp->d_ttl,
			      odp->d_authen_type, odp->d_authen_ver,
			      odp->d_data, odp->d_size);
#else AUTHEN
		dp = savedata(odp->d_class, odp->d_type, odp->d_ttl,
			odp->d_data, odp->d_size);
#endif AUTHEN
		dp->d_zone = DB_Z_CACHE;
		dp->d_flags = DB_F_HINT;
		if (db_update(name, dp,dp, (flags|DB_NOHINTS), fcachetab) != OK) {
#ifdef DEBUG		
			if (debug > 2)
				fprintf(ddt, "db_update: hint %x freed\n", dp);
#endif
			(void) free((char *)dp);
		}
        }

	if (odp != NULL) {
		pdp = NULL;
#ifdef ULTRIXFUNC
		for (dp = ((flags & DB_LOADDB) ? np->n_loaddb : np->n_data);
				dp != NULL; ) {
#else ULTRIXFUNC
		for (dp = np->n_data; dp != NULL; ) {
#endif ULTRIXFUNC

			if (!match(dp, odp->d_class, odp->d_type)) {
				if ((dp->d_type == T_CNAME ||
				    odp->d_type == T_CNAME) &&
				    zones[odp->d_zone].z_type != Z_CACHE) {
					syslog(LOG_ERR,
				"%s has CNAME and other data (illegal)\n",
					    name);
#ifdef DEBUG
					if (debug)
					    fprintf(ddt,
				"db_update: %s: CNAME and more (%d, %d)\n",
						name, odp->d_type, dp->d_type);
#endif
				}
				goto skip;
			}
#ifdef DEBUG
			if (debug >= 5)
				fprintf(ddt,"db_update: flags = %#x, sizes = %d, %d (%d)\n",
				    flags, odp->d_size, dp->d_size,
				    db_cmp(dp, odp));
#endif
			if (flags & DB_NOTAUTH && dp->d_zone) {
#ifdef DEBUG
				if (debug)
					fprintf(ddt,
					"%s attempted update to auth zone %d '%s'\n",
				    inet_ntoa(from_addr.sin_addr),
				    dp->d_zone, zones[dp->d_zone].z_origin);
#endif
				return (AUTH);
			}
			if ((flags & DB_NODATA) && !db_cmp(dp, odp)) {
				/* refresh ttl if cache entry */
				if (dp->d_zone == 0) {
					fixttl(odp);
					if (odp->d_ttl > dp->d_ttl)
						dp->d_ttl = odp->d_ttl;
#ifdef DEBUG
				        if (debug >= 3)
				        fprintf(ddt,"db_update: new ttl %d, +%d\n",
				                dp->d_ttl, dp->d_ttl - tt.tv_sec);
#endif
				}
				return (DATAEXISTS);
			}
			/*
			 * If the old databuf has some data, check that the
			 * data matches that in the new databuf (so UPDATED
			 * will delete only the matching RR)
			 */
			if (odp->d_size > 0) {
				if (db_cmp(dp, odp))
					goto skip;
			}
			foundRR = 1;
			if (flags & DB_DELETE)
#ifdef ULTRIXFUNC
				dp = rm_datum(((flags & DB_LOADDB) ? 1 : 0),
						dp, np, pdp);
#else ULTRIXFUNC
				dp = rm_datum(dp, np, pdp);
#endif ULTRIXFUNC
			else {
skip:				pdp = dp;
				dp = dp->d_next;
			}
		}
                if (!foundRR) {
			if (flags & DB_DELETE)
				return(NODATA);
			if (flags & DB_MEXIST)
				return(NODATA);
		}
	}
	if (newdp == NULL)
		return (OK);
	fixttl(newdp);
#ifdef DEBUG
        if (debug >= 3)
	        fprintf(ddt,"db_update: adding%s %x\n",
			(newdp->d_flags&DB_F_HINT) ? " hint":"", newdp);
#endif
	if (!(newdp->d_flags & DB_F_HINT))
		addinv(np, newdp);	/* modify inverse query tables */

	/* Add to end of list, generally preserving order */
	newdp->d_next = NULL;
#ifdef ULTRIXFUNC
	if ((dp = ((flags & DB_LOADDB) ? np->n_loaddb : np->n_data)) == NULL) {
		if(flags & DB_LOADDB)
			np->n_loaddb = newdp;
		else
			np->n_data = newdp;
#else ULTRIXFUNC
	if ((dp = np->n_data) == NULL)  {
		np->n_data = newdp;
#endif ULTRIXFUNC
		return (OK);
	}
	/* XXX: need to check for duplicate WKS records and flag error */
	while (dp->d_next != NULL) {
		if ((flags & DB_NODATA) && (dp->d_type == newdp->d_type) && (dp->d_class == newdp->d_class) && !db_cmp(dp, newdp))
			return (DATAEXISTS);
		dp = dp->d_next;
	}
	if ((flags & DB_NODATA) && (dp->d_type == newdp->d_type) && (dp->d_class == newdp->d_class) && !db_cmp(dp, newdp))
		return (DATAEXISTS);
	dp->d_next = newdp;
	return (OK);
}

fixttl(dp)
register struct databuf *dp;
{
	if (dp->d_zone == 0 && !(dp->d_flags & DB_F_HINT)) {
		if (dp->d_ttl <= tt.tv_sec)
			return;
		else if (dp->d_ttl < tt.tv_sec+min_cache_ttl)
			dp->d_ttl = tt.tv_sec+min_cache_ttl;
		else if (dp->d_ttl > tt.tv_sec+max_cache_ttl)
			dp->d_ttl = tt.tv_sec+max_cache_ttl;
	}
	return;
}

struct invbuf *invtab[INVHASHSZ];	/* Inverse query hash table */

/*
 * Add data 'dp' to inverse query tables for name 'np'.
 */
addinv(np, dp)
	struct namebuf *np;
	struct databuf *dp;
{
	register struct invbuf *ip;
	register int hval, i;

	switch (dp->d_type) {
	case T_A:
	case T_UID:
	case T_GID:
		break;

	default:
		return;
	}

	hval = dhash(dp->d_data, dp->d_size);
	for (ip = invtab[hval]; ip != NULL; ip = ip->i_next)
		for (i = 0; i < INVBLKSZ; i++)
			if (ip->i_dname[i] == NULL) {
				ip->i_dname[i] = np;
				return;
			}
	ip = saveinv();
	ip->i_next = invtab[hval];
	invtab[hval] = ip;
	ip->i_dname[0] = np;
}

/*
 * Remove data 'odp' from inverse query table.
 */
rminv(flag, odp)
	int flag;
	struct databuf *odp;
{
	register struct invbuf *ip;
	register struct databuf *dp;
	struct namebuf *np;
	register int i;

	for (ip = invtab[dhash(odp->d_data, odp->d_size)]; ip != NULL;
	    ip = ip->i_next) {
		for (i = 0; i < INVBLKSZ; i++) {
			if ((np = ip->i_dname[i]) == NULL)
				break;
#ifdef ULTRIXFUNC
			for (dp = (flag ? np->n_loaddb : np->n_data);
					dp != NULL; dp = dp->d_next) {
#else ULTRIXFUNC
			for (dp = np->n_data; dp != NULL; dp = dp->d_next) {
#endif ULTRIXFUNC
				if (!match(dp, odp->d_class, odp->d_type))
					continue;
				if (db_cmp(dp, odp))
					continue;
				while (i < INVBLKSZ-1) {
					ip->i_dname[i] = ip->i_dname[i+1];
					i++;
				}
				ip->i_dname[i] = NULL;
				return;
			}
		}
	}
}

/*
 * Compute hash value from data.
 */
dhash(dp, dlen)
	char *dp;
	int dlen;
{
	register char *cp;
	register unsigned hval;
	register int n;

	n = dlen;
	if (n > 8)
		n = 8;
	hval = 0;
	for (cp = dp; --n >= 0; ) {
		hval <<= 1;
		hval += *cp++;
	}
	return (hval % INVHASHSZ);
}

/*
 * Compare data sections from databufs for equivalence.  Must be case
 * insensitive for some domain names.  We assume that they are the
 * same type when they are passed.  Return 0 if equivalent, nonzero
 * otherwise.
 */

db_cmp(dp1, dp2)
	register struct databuf *dp1, *dp2;

{
	register char *cp1, *cp2;
	int len;

	if (dp1->d_size != dp2->d_size)
		return(1);
	if (dp1->d_mark != dp2->d_mark)
		return(1);		/* old and new RR's are distinct */
	switch (dp1->d_type) {

	case T_A:
	case T_UID:
	case T_GID:
	case T_WKS:
	case T_NULL:
#ifdef ALLOW_T_UNSPEC
        case T_UNSPEC:
#endif ALLOW_T_UNSPEC
		return(bcmp(dp1->d_data, dp2->d_data, dp1->d_size));

	case T_NS:
	case T_CNAME:
	case T_PTR:
	case T_MB:
	case T_MG:
	case T_MR:
	case T_UINFO:
		return(strcasecmp(dp1->d_data, dp2->d_data));

	case T_HINFO:
		cp1 = dp1->d_data;
		cp2 = dp2->d_data;
		len = *cp1;
		if (strncasecmp(++cp1, ++cp2, len))
			return(1);
		cp1 += len;
		cp2 += len;
		len = *cp1;
		return(strncasecmp(++cp1, ++cp2, len));

	case T_SOA:
	case T_MINFO:
		if (strcasecmp(dp1->d_data, dp2->d_data))
			return(1);
		cp1 = dp1->d_data + strlen(dp1->d_data) + 1;
		cp2 = dp2->d_data + strlen(dp2->d_data) + 1;
		if (dp1->d_type != T_SOA)
			return(strcasecmp(cp1, cp2));
		if (strcasecmp(cp1, cp2))
			return(1);
		cp1 += strlen(cp1) + 1;
		cp2 += strlen(cp2) + 1;
		return(bcmp(cp1, cp2, sizeof(u_long) * 5));
	
	case T_MX:
		cp1 = dp1->d_data;
		cp2 = dp2->d_data;
		if (*cp1++ != *cp2++ || *cp1++ != *cp2++)	/* cmp prio */
			return(1);
		return(strcasecmp(cp1, cp2));

	case T_TXT:
		if (dp1->d_size != dp2->d_size)
			return(1);
		return(bcmp(dp1->d_data, dp2->d_data, dp1->d_size));

	default:
		return (1);
	}
}
