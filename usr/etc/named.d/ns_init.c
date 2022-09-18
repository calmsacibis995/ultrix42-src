#ifndef lint
static	char	*sccsid = "@(#)ns_init.c	4.1	(ULTRIX)	7/2/90";
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
 * static char sccsid[] = "@(#)ns_init.c	4.23 (Berkeley) 2/28/88";
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
#include <sys/socket.h>
#include <sys/time.h>
#ifdef ULTRIXFUNC
#include <sys/stat.h>
#endif ULTRIXFUNC
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <ctype.h>
#include <arpa/nameser.h>
#ifdef AUTHEN
#include <netdb.h>	
#endif AUTHEN
#include "ns.h"
#include "db.h"

struct	zoneinfo zones[MAXZONES];	/* zone information */
int	nzones;				/* number of zones in use */
int	forward_only = 0;		/* run only as a slave */
char    *cache_file;
char    *localdomain;			/* "default" for non-dotted names */
int	maint_interval = 30;		/* minimum ns_maint() interval */

extern	int lineno;
#ifdef AUTHEN
extern struct hostent *gethostbyaddr_local();
#endif AUTHEN
#ifdef ULTRIXFUNC
extern file_ref_st *load_prep();
#endif ULTRIXFUNC
/*
 * Read boot file for configuration info.
 */

ns_init(bootfile)
	char *bootfile;
{
	register struct zoneinfo *zp;
	char buf[BUFSIZ];
	FILE *fp;
	int type;
	time_t next_refresh = 0;
	struct itimerval ival;
	extern int needmaint;

#ifdef AUTHEN
	struct hostent *host;
#endif AUTHEN

#ifdef ULTRIXFUNC
	file_ref_st	*load_info;
	int		slineno;
	int		status;
	int		zonenum;
#endif ULTRIXFUNC

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"ns_init(%s)\n", bootfile);
#endif
	gettime(&tt);

	if ((fp = fopen(bootfile, "r")) == NULL) {
		syslog(LOG_ERR, "%s: %m", bootfile);
		exit(1);
	}
	lineno = 0;

	/* allocate cache hash table, formerly the root hash table. */
	hashtab = savehash((struct hashbuf *)NULL);

	/* allocate root-hints/file-cache hash table */
	fcachetab = savehash((struct hashbuf *)NULL);

	if (localdomain)
		free(localdomain);
	localdomain = NULL;

	/* init zone data */
	cache_file = NULL;
	nzones = 1;		/* zone zero is cache data */
	zones[0].z_type = Z_CACHE;
	while (!feof(fp) && !ferror(fp)) {
		if (!getword(buf, sizeof(buf), fp))
			continue;
		/* read named.boot keyword and process args */
		if (strcasecmp(buf, "cache") == 0) {
			type = Z_CACHE;
			zp = zones;
		}
		else if (strcasecmp(buf, "primary") == 0)
			type = Z_PRIMARY;
		else if (strcasecmp(buf, "secondary") == 0)
			type = Z_SECONDARY;
		else if (strcasecmp(buf, "directory") == 0) {
			(void) getword(buf, sizeof(buf), fp);
			if (chdir(buf) < 0) {
				syslog(LOG_CRIT, "directory %s: %m\n",
					buf);
				exit(1);
			}
			continue;
		}
		else if (strcasecmp(buf, "sortlist") == 0) {
			get_sort_list(fp);
			continue;
		}
		else if (strcasecmp(buf, "forwarders") == 0) {
			get_forwarders(fp);
			continue;
		}
		else if (strcasecmp(buf, "slave") == 0) {
			forward_only++;
			endline(fp);
			continue;
		}
		else if (strcasecmp(buf, "domain") == 0) {
			if (getword(buf, sizeof(buf), fp))
				localdomain = savestr(buf);
			endline(fp);
			continue;
		} else {
			syslog(LOG_ERR, "%s: line %d: unknown field '%s'\n",
				bootfile, lineno, buf);
			endline(fp);
			continue;
		}
		if (nzones >= MAXZONES) {
			syslog(LOG_ERR, "too many zones (MAXZONES=%d)\n",
				MAXZONES);
			endline(fp);
			continue;
		}
		if (type != Z_CACHE)
			zp = &zones[nzones++];
		if (zp->z_origin) {
			free(zp->z_origin);
			zp->z_origin = 0;
		}
		if (zp->z_source) {
			free(zp->z_source);
			zp->z_source = 0;
		}
		zp->z_type = type;
		zp->z_addrcnt = 0;
		zp->z_auth = 0;
#ifdef ULTRIXFUNC
		zp->z_state = 0;
		zp->z_xferpid = 0;
		zp->z_files = NULL;
		zp->z_ftime = 0;
		zp->z_load_info = NULL;
#endif ULTRIXFUNC
		/*
		 * read zone origin
		 */
		if (!getword(buf, sizeof(buf), fp)) {
			syslog(LOG_ERR, "%s: line %d: missing origin\n",
						bootfile, lineno);
			continue;
		}
		if (buf[0] == '.')
			buf[0] = '\0';
		zp->z_origin = savestr(buf);
		/*
		 * read source file or host address
		 */
		if (!getword(buf, sizeof(buf), fp)) {
			syslog(LOG_ERR, "%s: line %d: missing origin\n",
						bootfile, lineno);
			continue;
		}
#ifdef DEBUG
		if (debug)
			fprintf(ddt,"zone[%d] type %d: '%s'",
			        zp-zones, zp->z_type,
				*(zp->z_origin) == '\0' ? "." : zp->z_origin);
#endif
		zp->z_time = 0;
		zp->z_refresh = 0;	/* by default, no dumping */
		switch (type) {
		case Z_CACHE:
			zp->z_source = savestr(buf);
#ifdef DEBUG
			if (debug)
				fprintf(ddt,", source = %s\n", zp->z_source);
#endif
			if (getword(buf, sizeof(buf), fp)) {
#ifdef notyet
				zp->z_refresh = atoi(buf);
				if (zp->z_refresh <= 0) {
					syslog(LOG_ERR,
				"%s: line %d: bad refresh '%s', ignored\n",
						bootfile, lineno, buf);
					zp->z_refresh = 0;
				} else if (cache_file == NULL)
					cache_file = zp->z_source;
#else
				syslog(LOG_WARNING,
				"%s: line %d: cache refresh ignored\n",
					bootfile, lineno);
#endif
				endline(fp);
				continue;
			}
#ifdef ULTRIXFUNC
			if((load_info = load_prep(zp, zp->z_source,
			    zp->z_origin, zp - zones)) == NULL) {
				endline(fp);
				continue;
			}				

			slineno = lineno;
			
			if ((status = db_load(load_info, NO_ITER_LIMIT)) != -2 ) {
				if(status == -1) {
				    /* load is done */
				    move_loaddb(fcachetab, ALL_DATA);
				    set_zp(load_info->zp, zp);

				    zp->z_files = load_info->files;
						
				} else {
				    delete_zone(1, fcachetab, ALL_DATA);
				    free_files(load_info->files);
				}
				free(load_info->zp);
				zp->z_load_info = NULL;
				free(load_info);

				lineno = slineno;
			} else {
				syslog(LOG_ERR,	"%s: line %d: Non-incremental load did not finish, abort.\n", bootfile, lineno);
				abortslowreload(zp, ALL_DATA);
#ifdef DEBUG
				if (debug)
					fprintf(ddt, "%s: line %d: Non-incremental load did not finish, abort.\n", bootfile, lineno);
#endif

			}

#else ULTRIXFUNC
			(void) db_load(zp->z_source, zp->z_origin, zp);
#endif ULTRIXFUNC
			break;

		case Z_PRIMARY:
			zp->z_source = savestr(buf);
#ifdef DEBUG
			if (debug)
				fprintf(ddt,", source = %s\n", zp->z_source);
#endif
#ifdef ULTRIXFUNC
			if((load_info = load_prep(zp, zp->z_source,
			    zp->z_origin, zp - zones)) == NULL) {
				endline(fp);
				continue;
			}				

			slineno = lineno;
			
			if ((status = db_load(load_info, NO_ITER_LIMIT)) != -2 ) {
				if(status == -1) {
					/* load is done */
					replace_data(hashtab, zp - zones);
					set_zp(load_info->zp, zp);
					if(!load_info->format_errs)
						zp->z_auth = 1;
					zp->z_files = load_info->files;
				} else {
				    delete_zone(1, hashtab, zonenum);
				    free_files(load_info->files);
				}
				free(load_info->zp);
				zp->z_load_info = NULL;
				free(load_info);
				lineno = slineno;
			} else {
				syslog(LOG_ERR,	"%s: line %d: Non-incremental load did not finish, abort.\n", bootfile, lineno);
				abortslowreload(zp, ALL_DATA);
#ifdef DEBUG
				if (debug)
					fprintf(ddt, "%s: line %d: Non-incremental load did not finish, abort.\n", bootfile, lineno);
#endif

			}
#else ULTRIXFUNC
			if (db_load(zp->z_source, zp->z_origin, zp) == 0)
				zp->z_auth = 1;
#endif ULTRIXFUNC

#ifdef ALLOW_UPDATES
			/* Guarantee calls to ns_maint() */
			zp->z_refresh = maint_interval;
#else
#ifndef PRIMARY_MAINT
			zp->z_refresh = 0;
			zp->z_time = 0;
#endif PRIMARY_MAINT
#endif ALLOW_UPDATES
			break;

		case Z_SECONDARY:
#ifdef DEBUG
			if (debug)
				fprintf(ddt,"\n\taddrs: %s, ", buf);
#endif
			zp->z_addr[zp->z_addrcnt].s_addr =
				inet_addr(buf);
#ifdef AUTHEN
			if((host = gethostbyaddr_local(&(zp->z_addr[zp->z_addrcnt].s_addr),
					    sizeof(struct in_addr),AF_INET)) == (struct hostent *)NULL){
				syslog(LOG_ERR, "%s:", "address of primary server not in local db");
				exit(1);
			}

			strcpy(zp->z_dname[zp->z_addrcnt], host->h_name);
#endif AUTHEN				
			/* Indicate no cache for this zone yet */
			zp->z_source = (char *) NULL;
			if (zp->z_addr[zp->z_addrcnt].s_addr != (unsigned)-1)
				zp->z_addrcnt++;
			while (getword(buf, sizeof(buf), fp)) {
				if (buf[0] == '\0')
					break;

				zp->z_addr[zp->z_addrcnt].s_addr =
					inet_addr(buf);
				if (zp->z_addr[zp->z_addrcnt].s_addr ==
						(unsigned)-1) {
					zp->z_source = savestr(buf);
					break;
				}
#ifdef AUTHEN
				if((host = gethostbyaddr_local(&(zp->z_addr[zp->z_addrcnt].s_addr),
							       sizeof(struct in_addr),
							       AF_INET))
				   == (struct hostent *)NULL) {
					syslog(LOG_ERR, "%s:", "address of primary server not in local db");
					exit(1);
				}

				strcpy(zp->z_dname[zp->z_addrcnt],
				       host->h_name);
#endif AUTHEN

#ifdef DEBUG
				if (debug)
					fprintf(ddt,"%s, ",buf);
#endif
				if (++zp->z_addrcnt >= NSMAX) {
					zp->z_addrcnt = NSMAX;
#ifdef DEBUG
					if (debug)
					    fprintf(ddt,
						"\nns.h NSMAX reached\n");
#endif
					break;
				}
			}
#ifdef DEBUG
			if (debug)
				fprintf(ddt,"addrcnt = %d\n", zp->z_addrcnt);
#endif
			zoneinit(zp);
			break;

		}

		if (zp->z_refresh && zp->z_time == 0)
			zp->z_time = zp->z_refresh + tt.tv_sec;
		if (zp->z_time <= tt.tv_sec)
			needmaint = 1;
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "z_time %d, z_refresh %d\n",
			    zp->z_time, zp->z_refresh);
#endif

	}
	(void) fclose(fp);

	/*
	 * Schedule calls to ns_maint().
	 */
	if (needmaint == 0)
		sched_maint();
#ifdef DEBUG
	if (debug)
		fprintf(ddt,"exit ns_init()%s\n", needmaint ?
		    ", need maintenance immediately" : "");
#endif
}

zoneinit(zp)
	register struct zoneinfo *zp;
{
	
#ifdef ULTRIXFUNC
	file_ref_st *load_info;
	int slineno;
	int status;
	
#ifdef DEBUG
	if (debug)
		fprintf(ddt,"zoneinit()\n");
#endif

	/*
	 * Try to load zone from backup file,
	 * if one was specified and it exists.
	 * If not, or if the data are out of date,
	 * we will refresh the zone from a primary
	 * immediately.
	 */
	if (zp->z_source != NULL && ((load_info = load_prep(zp, zp->z_source,
				zp->z_origin, zp - zones)) != NULL)) {

		slineno = lineno;
			
		if((status = db_load(load_info, NO_ITER_LIMIT)) == -1) {
			lineno = slineno;
		        replace_data(hashtab, zp - zones);
			set_zp(load_info->zp, zp);
			zp->z_files = load_info->files;
			free(load_info->zp);
			zp->z_load_info = NULL;
			if(!load_info->format_errs) {
				free(load_info);
				zp->z_auth = 1;
				return;
			}
			free(load_info);
		} else if (status == -2) {
			syslog(LOG_ERR,	"bootfile: line %d: Non-incremental load did not finish, abort.\n", lineno);
			abortslowreload(zp, ALL_DATA);
#ifdef DEBUG
				if (debug)
					fprintf(ddt, "bootfile: line %d: Non-incremental load did not finish, abort.\n", lineno);
#endif

		} else {
			delete_zone(1, hashtab, zp - zones);
			free_files(load_info->files);
			free(load_info->zp);
			zp->z_load_info = NULL;
			free(load_info);
			lineno = slineno;
		}
		  
		lineno = slineno;
	}

	/*
	 * Set zone to be refreshed immediately.
	 */
	zp->z_refresh = INIT_REFRESH;
	zp->z_retry = INIT_REFRESH;
	zp->z_time = tt.tv_sec;
	zp->z_class = C_ANY;

		
#else ULTRIXFUNC

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"zoneinit()\n");
#endif

	/*
	 * Try to load zone from backup file,
	 * if one was specified and it exists.
	 * If not, or if the data are out of date,
	 * we will refresh the zone from a primary
	 * immediately.
	 */
	if (zp->z_source == NULL ||
	    db_load(zp->z_source, zp->z_origin, zp) != 0) {
		/*
		 * Set zone to be refreshed immediately.
		 */
		zp->z_refresh = INIT_REFRESH;
		zp->z_retry = INIT_REFRESH;
		zp->z_time = tt.tv_sec;
	} else
		zp->z_auth = 1;
#endif ULTRIXFUNC

}

#ifdef ALLOW_UPDATES
/*
 * Look for the authoritative zone with the longest matching RHS of dname
 * and return its zone # or zero if not found.
 */
findzone(dname, class)
	char *dname;
	int class;
{
	char *dZoneName, *zoneName, *index(), *dotPos;
	int dZoneNameLen, zoneNameLen;
	int maxMatchLen = 0;
	int maxMatchZoneNum = 0;
	int zoneNum;

#ifdef DEBUG
	if (debug >= 4)
		fprintf(ddt, "findzone(dname=%s, class=%d)\n", dname, class);
	if (debug >= 5) {
		fprintf(ddt, "zone dump:\n");
		for (zoneNum = 1; zoneNum < nzones; zoneNum++)
			printzoneinfo(zoneNum);
	}
#endif DEBUG

	dZoneName = index(dname, '.');
	if (dZoneName == NULL)
		dZoneName = "";	/* root */
	else
		dZoneName++;	/* There is a '.' in dname, so use remainder of
				   string as the zone name */
	dZoneNameLen = strlen(dZoneName);
	for (zoneNum = 1; zoneNum < nzones; zoneNum++) {
		zoneName = (zones[zoneNum]).z_origin;
		zoneNameLen = strlen(zoneName);
		/* The zone name may or may not end with a '.' */
		dotPos = index(zoneName, '.');
		if (dotPos)
			zoneNameLen--;
		if (dZoneNameLen != zoneNameLen)
			continue;
#ifdef DEBUG
		if (debug >= 5)
			fprintf(ddt, "about to strncasecmp('%s', '%s', %d)\n",
			        dZoneName, zoneName, dZoneNameLen);
#endif
		if (strncasecmp(dZoneName, zoneName, dZoneNameLen) == 0) {
#ifdef DEBUG
			if (debug >= 5)
				fprintf(ddt, "match\n");
#endif
			/*
			 * See if this is as long a match as any so far.
			 * Check if "<=" instead of just "<" so that if
			 * root domain (whose name length is 0) matches,
			 * we use it's zone number instead of just 0
			 */
			if (maxMatchLen <= zoneNameLen) {
				maxMatchZoneNum = zoneNum;
				maxMatchLen = zoneNameLen;
			}
		}
#ifdef DEBUG
		else
			if (debug >= 5)
				fprintf(ddt, "no match\n");
#endif
	}
#ifdef DEBUG
	if (debug >= 4)
		fprintf(ddt, "findzone: returning %d\n", maxMatchZoneNum);
#endif DEBUG
	return (maxMatchZoneNum);
}
#endif ALLOW_UPDATES

get_forwarders(fp)
	FILE *fp;
{
	char buf[BUFSIZ];
	struct fwdinfo *fip = NULL, *ftp = NULL;
#ifdef AUTHEN
	struct hostent *host;
#endif AUTHEN

	extern struct	sockaddr_in nsaddr;
	extern struct	fwdinfo *fwdtab;

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"forwarders ");
#endif
	while (getword(buf, sizeof(buf), fp)) {
		if (strlen(buf) == 0)
			break;
#ifdef DEBUG
		if (debug)
			fprintf(ddt," %s",buf);
#endif
		if (ftp == NULL)
			ftp = (struct fwdinfo *)malloc(sizeof(struct fwdinfo));
		if ( isdigit(buf[0]) &&
		    (ftp->fwdaddr.sin_addr.s_addr = inet_addr(buf))
		    != (unsigned)-1) {
			ftp->fwdaddr.sin_port = nsaddr.sin_port;
			ftp->fwdaddr.sin_family = AF_INET;
		} else {
			syslog(LOG_ERR, "'%s' (ignored, NOT dotted quad)", buf);
#ifdef DEBUG
			if (debug)
				fprintf(ddt," (ignored, NOT dotted quad)");
#endif
			continue;	
		}
#ifdef AUTHEN
		if ((host = gethostbyaddr_local(&(ftp->fwdaddr.sin_addr.s_addr), sizeof(struct in_addr), AF_INET)) == (struct hostent *)NULL) {
			syslog(LOG_ERR, "address of forwarder %s not in local hosts file",buf);
			continue;
		}

		strcpy(ftp->fwdname, host->h_name);
#endif AUTHEN
		ftp->next = NULL;
		if (fwdtab == NULL)
			fwdtab = ftp;	/* First time only */
		else
			fip->next = ftp;
		fip = ftp;
		ftp = NULL;
	}
	if (ftp)
		free((char *)ftp);
	
#ifdef DEBUG
	if (debug) 
		fprintf(ddt,"\n");
	if (debug > 2)
		for (ftp = fwdtab; ftp != NULL; ftp = ftp->next)
			fprintf(ddt,"ftp x%x %s next x%x\n", ftp,
				inet_ntoa(ftp->fwdaddr.sin_addr), ftp->next);
#endif
}
