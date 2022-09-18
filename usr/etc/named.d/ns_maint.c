#ifndef lint
static	char	*sccsid = "@(#)ns_maint.c	4.2	(ULTRIX)	11/15/90";
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
 * static char sccsid[] = "@(#)ns_maint.c	4.23 (Berkeley) 2/28/88";
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
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef ULTRIXFUNC
#include <sys/stat.h>
#endif ULTRIXFUNC
#if defined(SYSV)
#include <unistd.h>
#endif SYSV
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <arpa/nameser.h>
#ifdef ULTRIXFUNC
#include <netdb.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <resolv.h>
#endif ULTRIXFUNC
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "ns.h"
#include "db.h"

extern int errno;
extern int maint_interval;

#ifdef AUTHEN
extern char *res_dotname_head();
extern int netsafe;
extern int defauthentype;
extern int defauthenver;
#endif AUTHEN

#ifdef ULTRIXFUNC
extern struct protoent *getprotobyname_local();
#define MAXPACKET       4096			/* max packet size */
#endif ULTRIXFUNC

extern int needzoneload;
#ifdef ULTRIXFUNC
extern int continuemaint;
extern int ns_port;
extern map_t m_credval[];
extern map_t m_credtype[];
extern file_ref_st *load_prep();
extern char *m_val_str();
#endif ULTRIXFUNC

int xfers_running;	       /* number of xfers running */
int xfers_deferred;	       /* number of needed xfers not run yet */
static int alarm_pending;

#ifdef ULTRIXFUNC

/*
 * Invoked at regular intervals by signal interrupt; refresh all secondary
 * zones from primary name server and remove old cache entries.  Also,
 * ifdef'd ALLOW_UPDATES, dump database if it has changed since last
 * dump/bootup.
 */
ns_continuemaint()
{
	register struct zoneinfo *zp;
	int zonenum;
	time_t next_refresh = 0;
	struct itimerval ival;
	int needsched;
	int status;

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"ns_continuemaint()\n");
#endif

	needsched = 0;						
	continuemaint = 0;
	gettime(&tt);
	for (zp = zones, zonenum = 0; zp < &zones[nzones]; zp++, zonenum++) {

#ifdef DEBUG
		if (debug >= 2)
			printzoneinfo(zonenum);
#endif
		/* if the zone is being slowly loaded, continue to load. */
		if (zp->z_state & Z_SLOW_RELOAD) {

			switch(zp->z_type) {
#ifdef NEW_SIG
			case Z_PRIMARY:

				if ((status = db_load(zp->z_load_info,
						NUM_ITER_LOAD)) != -2 ) {
					/* the load is done or it failed */

					if(status >= 0 &&
					    !zp->z_load_info->format_errs) {
						/* load is done */
						replace_data(hashtab,zonenum);
						set_zp(zp->z_load_info->zp,
							zp);
						free_files(zp->z_files);
						zp->z_files =
						  zp->z_load_info->files;
						zp->z_auth = 1;
					} else {
						delete_zone(1, hashtab, zonenum);
						free_files(zp->z_load_info->files);
#ifdef PRIMARY_MAINT
						zp->z_time = tt.tv_sec + zp->z_retry;
#endif PRIMARY_MAINT
					}
					needsched = 1;
					free(zp->z_load_info->zp);
					free(zp->z_load_info);
					zp->z_load_info = NULL;
					zp->z_state &= ~Z_SLOW_RELOAD;
				} else {
					/* The load is not finished */
					continuemaint = 1;
				}

				break;
#endif NEW_SIG
			case Z_SECONDARY:

				if ((status = db_load(zp->z_load_info,
						NUM_ITER_LOAD)) != -2 ) {
					/* the load is done or it failed */

					if(status >= 0 &&
					    !zp->z_load_info->format_errs) {

						/* load is done */
						replace_data(hashtab, zonenum);
						set_zp(zp->z_load_info->zp,
							zp);
						free_files(zp->z_files);
						zp->z_files =
						  zp->z_load_info->files;
						zp->z_auth = 1;
					} else {
						delete_zone(1, hashtab, zonenum);
						free_files(zp->z_load_info->files);
						zp->z_time = tt.tv_sec + zp->z_retry;
					}

					needsched = 1;

					free(zp->z_load_info->zp);
					free(zp->z_load_info);
					zp->z_load_info = NULL;
					zp->z_state &= ~Z_SLOW_RELOAD;
				} else {
					/* The load is not finished */
					continuemaint = 1;
				}
				break;

			case Z_CACHE:
				break;
			default:
				break;
			}
			gettime(&tt);
		}
	}
	if(needsched)
		sched_maint();
#ifdef DEBUG
	if (debug)
		fprintf(ddt,"exit ns_continuemaint()\n");
#endif
}

#endif ULTRIXFUNC

/*
 * Invoked at regular intervals by signal interrupt; refresh all secondary
 * zones from primary name server and remove old cache entries.  Also,
 * ifdef'd ALLOW_UPDATES, dump database if it has changed since last
 * dump/bootup.
 */
ns_maint()
{
	register struct zoneinfo *zp;
	struct itimerval ival;
	time_t next_refresh = 0;
	int zonenum;

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"\nns_maint()\n");
#endif

	gettime(&tt);
	xfers_deferred = 0;
	alarm_pending = 0;
	for (zp = zones, zonenum = 0; zp < &zones[nzones]; zp++, zonenum++) {

#ifdef DEBUG
		if (debug >= 2)
			printzoneinfo(zonenum);
#endif


		if ((tt.tv_sec >= zp->z_time) && zp->z_refresh > 0) {


			/*
			 * Set default time for next action first,
			 * so that it can be changed later if necessary.
			 */
			zp->z_time = tt.tv_sec + zp->z_refresh;

			switch (zp->z_type) {

			case Z_CACHE:

				doachkpt();
				break;

			case Z_SECONDARY:

#ifndef ULTRIXFUNC
				if ((zp->z_state & Z_NEED_RELOAD) == 0)
#endif ULTRIXFUNC
				    if (zp->z_state & Z_XFER_RUNNING)
					abortxfer(zp);
#ifdef ULTRIXFUNC
				    else if (zp->z_state & Z_NEED_RELOAD) {
					zp->z_state &= ~Z_NEED_RELOAD;
					zp->z_time = tt.tv_sec + zp->z_retry;
				    } else if (zp->z_state & Z_SLOW_RELOAD)
					abortslowreload(zp, zonenum);
#endif ULTRIXFUNC
				    else if (xfers_running < MAX_XFERS_RUNNING)
					startxfer(zp);
				    else {
					zp->z_state |= Z_NEED_XFER;
					++xfers_deferred;

#ifdef DEBUG
					if (debug > 1)
					    fprintf(ddt,
						"xfer deferred for %s\n",
						zp->z_origin);
#endif
				    }
				break;
#ifdef PRIMARY_MAINT
			case Z_PRIMARY:
				if (zp->z_state & Z_SLOW_RELOAD)
				    abortslowreload(zp, zonenum);
				else {
				    if(need_primaryload(zp)) {
					zp->z_time = zp->z_refresh+ tt.tv_sec;
					break;
				    }

				    if((zp->z_load_info = load_prep(zp,
						filename, origin,
						zp - zones)) == NULL) {
					zp->z_time = zp->z_retry+ tt.tv_sec;
					break;
				    }

				    if ((status = db_load(zp->z_load_info,
						NUM_ITER_LOAD)) != -2 ) {
					/* the load is done or it failed */

					if(status >= 0 &&
					    !zp->z_load_info->format_errs) {

					    /* load is done */
					    replace_data(hashtab, zonenum);

					    set_zp(zp->z_load_info->zp, zp);
					    free_files(zp->z_files);
					    zp->z_files = load_info->files;
						
					} else {
					    delete_zone(1, hashtab, zonenum);
					    free_files(zp->z_load_info->files);
					    zp->z_time = tt.tv_sec + zp->z_retry;
					}
					free(zp->z_load_info->zp);
					free(zp->z_load_info);
					zp->z_load_info = NULL;
					zp->z_state &= ~Z_SLOW_RELOAD;
				    } else {
					/* The load is not finished */
					zp->z_time = zp->z_refresh+ tt.tv_sec;
					zp->z_state |= Z_SLOW_RELOAD;

					continuemaint = 1;
				    }
				}

				break;
#else PRIMARY_MAINT
#ifdef ALLOW_UPDATES
			case Z_PRIMARY:
				/*
				 * Checkpoint the zone if it has changed
				 * since we last checkpointed
				 */
				if (zp->hasChanged)
					zonedump(zp);
				break;
#endif ALLOW_UPDATES
#endif PRIMARY_MAINT

			}
			gettime(&tt);
		}
	}
	sched_maint();
#ifdef DEBUG
	if (debug)
		fprintf(ddt,"exit ns_maint()\n");
#endif
}

#ifdef NEW_SIG
/*
 * Invoked at regular intervals by signal interrupt; refresh all secondary
 * zones from primary name server and remove old cache entries.  Also,
 * ifdef'd ALLOW_UPDATES, dump database if it has changed since last
 * dump/bootup.
 */
ns_rereadzones()
{
	register struct zoneinfo *zp;
	struct itimerval ival;
	time_t next_refresh = 0;
	int zonenum;
	int status;

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"\nns_rereadzones()\n");
#endif

	gettime(&tt);
	xfers_deferred = 0;
	for (zp = zones, zonenum = 0; zp < &zones[nzones]; zp++, zonenum++) {

#ifdef DEBUG
		if (debug >= 2)
			printzoneinfo(zonenum);
#endif

		/*
		 * Set default time for next action first,
		 * so that it can be changed later if necessary.
		 */
		switch (zp->z_type) {

		case Z_CACHE:
			break;

		case Z_SECONDARY:
		    if (zp->z_state & Z_XFER_RUNNING)
			break;
		    else if (zp->z_state & Z_NEED_RELOAD)
			zp->z_state &= ~Z_NEED_RELOAD;
		    else if (zp->z_state & Z_SLOW_RELOAD)
			abortslowreload(zp, zonenum);

		    if (xfers_running < MAX_XFERS_RUNNING)
			startxfer(zp);
		    else {
			zp->z_time = tt.tv_sec + zp->z_refresh;

			zp->z_state |= Z_NEED_XFER;
			++xfers_deferred;
#ifdef DEBUG
			if (debug > 1)
			    fprintf(ddt,
				"xfer deferred for %s\n",
				zp->z_origin);
#endif
		    }
		    break;
		case Z_PRIMARY:
		    if (zp->z_state & Z_SLOW_RELOAD)
			abortslowreload(zp, zonenum);
		    else {
			if(!need_primaryload(zp)) {
#ifdef PRIMARY_MAINT
			    zp->z_time = zp->z_refresh+ tt.tv_sec;
#endif PRIMARY_MAINT
			    break;
			}

			if((zp->z_load_info = load_prep(zp,
				zp->z_source, zp->z_origin,
				zp - zones)) == NULL) {
#ifdef PRIMARY_MAINT
			    zp->z_time = zp->z_retry+ tt.tv_sec;
#endif PRIMARY_MAINT
			    break;
			}

			if ((status = db_load(zp->z_load_info,
				NUM_ITER_LOAD)) != -2 ) {
			    /* the load is done or it failed */
			    if(status >= 0 &&
					!zp->z_load_info->format_errs) {

				/* load is done */
				replace_data(hashtab, zonenum);
				set_zp(zp->z_load_info->zp, zp);
				free_files(zp->z_files);
				zp->z_files = zp->z_load_info->files;
#ifdef PRIMARY_MAINT
				zp->z_time = tt.tv_sec + zp->z_refresh;
#endif PRIMARY_MAINT
			    } else {
				delete_zone(1, hashtab, zonenum);
				free_files(zp->z_load_info->files);
#ifdef PRIMARY_MAINT
				zp->z_time = tt.tv_sec + zp->z_retry;
#endif PRIMARY_MAINT
			    }
			    free(zp->z_load_info->zp);
			    free(zp->z_load_info);
			    zp->z_load_info = NULL;
			    zp->z_state &= ~Z_SLOW_RELOAD;
			} else {
			    /* The load is not finished */
#ifdef PRIMARY_MAINT
			    zp->z_time = zp->z_refresh+ tt.tv_sec;
#endif PRIMARY_MAINT
			    zp->z_state |= Z_SLOW_RELOAD;
			    continuemaint = 1;
			}

		    }
		    break;

		}
		gettime(&tt);
	}
	sched_maint();
#ifdef DEBUG
	if (debug)
		fprintf(ddt,"exit ns_maint()\n");
#endif
}
#endif NEW_SIG

abort_loads()
{
	register struct zoneinfo *zp;
	int zonenum;

	xfers_deferred = 0;

	for (zp = zones, zonenum = 0; zp < &zones[nzones]; zp++, zonenum++) {

		if(zp->z_state & Z_NEED_RELOAD) {
			zp->z_state &= ~Z_NEED_RELOAD;
			zp->z_time = tt.tv_sec + zp->z_retry;
		}
		else if(zp->z_state & Z_XFER_RUNNING)
			abortxfer(zp);
		else if(zp->z_state & Z_NEED_XFER) {
			zp->z_state &= ~Z_NEED_XFER;
		}
		else if(zp->z_state & Z_SLOW_RELOAD)
			abortslowreload(zp, zonenum);
	}
}

abortslowreload(zp, zonenum)
	register struct zoneinfo *zp;
	int zonenum;
{
	int status;

	if ((status = db_load(zp->z_load_info, STOP_LOAD)) >= -2 ) {
		syslog(LOG_ERR, "abortslowreload failed\n");
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "abortslowreload failed\n");
#endif
	} else {
		free_files(zp->z_load_info->files);
		delete_zone(1, hashtab, zonenum);
		zp->z_time = tt.tv_sec + zp->z_retry;

		free(zp->z_load_info->zp);
		free(zp->z_load_info);
		zp->z_load_info = NULL;
		zp->z_state &= ~Z_SLOW_RELOAD;
	}
}

/*
 * Find when the next refresh needs to be and set
 * interrupt time accordingly.
 */
sched_maint()
{
	register struct zoneinfo *zp;
	struct itimerval ival;
	time_t next_refresh = 0;
	static time_t next_alarm;

	for (zp = zones; zp < &zones[nzones]; zp++)
		if (zp->z_time != 0 &&
		    (next_refresh == 0 || next_refresh > zp->z_time))
			next_refresh = zp->z_time;
        /*
	 *  Schedule the next call to ns_maint.
	 *  Don't visit any sooner than maint_interval.
	 */
	bzero((char *)&ival, sizeof (ival));
	if (next_refresh != 0) {
		if (next_refresh == next_alarm && alarm_pending) {
#ifdef DEBUG
			if (debug)
			    fprintf(ddt,"sched_maint: no schedule change\n");
#endif
			return;
		}
		ival.it_value.tv_sec = next_refresh - tt.tv_sec;
		if (ival.it_value.tv_sec < maint_interval)
			ival.it_value.tv_sec = maint_interval;
		next_alarm = next_refresh;
		alarm_pending = 1;
	}
	(void) setitimer(ITIMER_REAL, &ival, (struct itimerval *)NULL);
#ifdef DEBUG
	if (debug)
		fprintf(ddt,"sched_maint: Next interrupt in %d sec\n",
			ival.it_value.tv_sec);
#endif
}

/*
 * Start an asynchronous zone transfer for a zone.
 * Depends on current time being in tt.
 * The caller must call sched_maint after startxfer.
 */
startxfer(zp)
	struct zoneinfo *zp;
{
	static char *argv[NSMAX + 20], argv_ns[NSMAX][MAXDNAME];
	int cnt, argc = 0, argc_ns = 0, pid, omask;
	char debug_str[10];
	char serial_str[10];
	char port_str[10];
#ifdef AUTHEN
	char authen_str[(MAXDNAME * 2) + 1];
#endif AUTHEN
#ifdef ULTRIXFUNC
	char *tmp_cptr;
#endif ULTRIXFUNC

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"startxfer() %s\n", zp->z_origin);
#endif

	argv[argc++] = "named-xfer";
	argv[argc++] = "-z";
	argv[argc++] = zp->z_origin;
	argv[argc++] = "-f";
	argv[argc++] = zp->z_source;
	argv[argc++] = "-s";
	sprintf(serial_str, "%d", zp->z_serial);
	argv[argc++] = serial_str;
#ifndef ULTRIXFUNC
	if (zp->z_state & Z_SYSLOGGED)
		argv[argc++] = "-q";
#endif ULTRIXFUNC
	argv[argc++] = "-P";
	sprintf(port_str, "%d", ns_port);
	argv[argc++] = port_str;
#ifdef AUTHEN
	if(netsafe) {
		argv[argc++] = "-n";

		argv[argc++] = "-a";

		if((tmp_cptr = m_val_str(m_credtype, NUMMAPCREDTYPE,
		    defauthentype)) == NULL) {
		
		}
		strcpy(authen_str, tmp_cptr);

		strcat(authen_str, ".");

		if((tmp_cptr = m_val_str(m_credval, NUMMAPCREDVAL,
		    defauthenver)) == NULL) {
		
		}
		strcat(authen_str, tmp_cptr);
		argv[argc++] = authen_str;
	}
#endif AUTHEN

#ifdef DEBUG
	if (debug) {
		argv[argc++] = "-d";
		sprintf(debug_str, "%d", debug);
		argv[argc++] = debug_str;
		argv[argc++] = "-l";
		argv[argc++] = "/usr/tmp/xfer.ddt";
		if (debug > 5) {
			argv[argc++] = "-t";
			argv[argc++] = "/usr/tmp/xfer.trace";
		}
	}
#endif
	
	/*
	 * Copy the server ip addresses into argv, after converting
	 * to ascii and saving the static inet_ntoa result
	 */
	for (cnt = 0; cnt < zp->z_addrcnt; cnt++)
		argv[argc++] = strcpy(argv_ns[argc_ns++],
		    inet_ntoa(zp->z_addr[cnt]));

	argv[argc] = 0;

#ifdef DEBUG
#ifdef ECHOARGS
	if (debug) {
		int i;
		for (i = 0; i < argc; i++) 
			fprintf(ddt, "Arg %d=%s\n", i, argv[i]);
        }
#endif /* ECHOARGS */
#endif /* DEBUG */

#ifdef SYSV
#define vfork fork
#else
	gettime(&tt);
	omask = sigblock(sigmask(SIGCHLD));
#endif
	if ((pid = vfork()) == -1) {
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "xfer [v]fork: %d\n", errno);
#endif
		syslog(LOG_ERR, "xfer [v]fork: %m");
#ifndef SYSV
		(void) sigsetmask(omask);
#endif
		zp->z_time = tt.tv_sec + XFER_TIME_FUDGE;
		return;
	}

	if (pid) {
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "started xfer child %d\n", pid);
#endif
		zp->z_state &= ~Z_NEED_XFER;
		zp->z_state |= Z_XFER_RUNNING;
		zp->z_xferpid = pid;
		xfers_running++;
		zp->z_time = tt.tv_sec + MAX_XFER_TIME;
#ifndef SYSV
		(void) sigsetmask(omask);
#endif
	} else {
		execve(_PATH_XFER, argv, NULL);
		syslog(LOG_ERR, "can't exec %s: %m", _PATH_XFER);
		_exit(XFER_FAIL);	/* avoid duplicate buffer flushes */
	}
}

#ifdef DEBUG
printzoneinfo(zonenum)
int zonenum;
{
	struct timeval  tt;
	struct zoneinfo *zp = &zones[zonenum];
	char *ZoneType;

	if (!debug)
		return; /* Else fprintf to ddt will bomb */
	fprintf(ddt, "printzoneinfo(%d):\n", zonenum);

	gettime(&tt);
	switch (zp->z_type) {
		case Z_PRIMARY: ZoneType = "Primary"; break;
		case Z_SECONDARY: ZoneType = "Secondary"; break;
		case Z_CACHE: ZoneType = "Cache"; break;
		default: ZoneType = "Unknown";
	}
#ifdef ULTRIXFUNC
	if (zp->z_origin == (char *)NULL || zp->z_origin[0] == '\0')
#else ULTRIXFUNC
	if (zp->z_origin[0] == '\0')
#endif ULTRIXFUNC
		fprintf(ddt,"origin ='.'");
	else
		fprintf(ddt,"origin ='%s'", zp->z_origin);
	fprintf(ddt,", type = %s", ZoneType);
	fprintf(ddt,", source = %s\n", zp->z_source);
	fprintf(ddt,"z_refresh = %ld", zp->z_refresh);
	fprintf(ddt,", retry = %ld", zp->z_retry);
	fprintf(ddt,", expire = %ld", zp->z_expire);
	fprintf(ddt,", minimum = %ld", zp->z_minimum);
	fprintf(ddt,", serial = %ld\n", zp->z_serial);
	fprintf(ddt,"z_time = %d", zp->z_time);
	if (zp->z_time) {
		fprintf(ddt,", now time : %d sec", tt.tv_sec);
		fprintf(ddt,", time left: %d sec", zp->z_time - tt.tv_sec);
	}
}
#endif DEBUG

#ifdef ULTRIXFUNC

/*
 * replace_data (htp, zone) --
 *     Delete all RR's in the zone "zone" on n_data lists.  Replace this
 *     data with records of zone "zone" on the n_loaddb lists.
 */
replace_data (htp, zone)
    register struct hashbuf *htp;
    register int zone;
{
	delete_zone(0, htp, zone);
	move_loaddb(htp, zone);
}

/*
 * move_loaddb (htp, zone) --
 *     Place data with records of zone "zone" in the n_loaddb lists on
 *     the n_data lists.
 */
move_loaddb(htp, zone)
    register struct hashbuf *htp;
    register int zone;
{

    register struct databuf *dp;
    register struct databuf *pdp;
    register struct databuf *temp;
    register struct namebuf *np;
    struct namebuf **npp, **nppend;

    nppend = htp->h_tab + htp->h_size;
    for (npp = htp->h_tab; npp < nppend; npp++) {
    	for (np = *npp; np != NULL; np = np->n_next) {

    	    for (pdp = NULL, dp = np->n_loaddb; dp != NULL; ) {
		if (dp->d_zone == zone || zone == ALL_DATA) {
			if(pdp == NULL)
				np->n_loaddb = dp->d_next;
			else
				pdp->d_next = dp->d_next;
			temp = dp->d_next;
			dp->d_next = np->n_data;
			np->n_data = dp;
			dp = temp;
		} else {
			pdp = dp;
			dp = dp->d_next;
		}
		
	    }

	    /* Call recursively to clean up subdomains. */
	    if (np->n_hash != NULL)
		move_loaddb (np->n_hash, zone);
	}
    }
}

/*
 * delete_zone (htp, zone) --
 *     Delete all RR's in the zone "zone" on either the n_loaddb lists or
 *     the n_data lists. Flag determines from which list to delete.
 */
delete_zone (flag, htp, zone)
    int flag;
    register struct hashbuf *htp;
    register int zone;
{

    register struct databuf *dp;
    register struct databuf *pdp;
    register struct namebuf *np;
    struct namebuf **npp, **nppend;

    nppend = htp->h_tab + htp->h_size;
    for (npp = htp->h_tab; npp < nppend; npp++) {
    	for (np = *npp; np != NULL; np = np->n_next) {
    	    for (pdp = NULL, dp = ((flag) ? np->n_loaddb: np->n_data);
			dp != NULL; ) {
			
		if (dp->d_zone == zone || zone == ALL_DATA) {
			dp =  rm_datum(flag, dp, np, pdp);
		} else {
			pdp = dp;
			dp = dp->d_next;
		}
    	    }

	    /* Call recursively to clean up subdomains. */
	    if (np->n_hash != NULL)
		delete_zone (flag, np->n_hash, zone);
	}
    }
}
#endif ULTRIXFUNC

   
/*
 * Abort an xfer that has taken too long.
 */
abortxfer(zp)
	register struct zoneinfo *zp;
{

	kill(zp->z_xferpid, SIGKILL); /* don't trust it at all */
#ifdef DEBUG
	if (debug)
	  fprintf(ddt, "Killed child %d (zone %s) due to timeout\n",
	     zp->z_xferpid, zp->z_origin);
#endif /* DEBUG */

#ifdef ULTRIXFUNC
	zp->z_state &= ~Z_XFER_RUNNING;
#endif ULTRIXFUNC
	zp->z_time = tt.tv_sec + zp->z_retry;
}

#ifdef SYSV
union wait {
	unsigned short	w_termsig:7;	/* termination signal */
	unsigned short	w_coredump:1;	/* core dump indicator */
	unsigned short	w_retcode:8;	/* exit code if w_termsig==0 */
};
#endif


/*
 * SIGCHLD signal handler: process exit of xfer's.
 * (Note: also called when outgoing transfer completes.)
 */
void
endxfer()
{
    	register struct zoneinfo *zp;   
	int pid, xfers = 0;
	union wait status;
	static int first_time = 1;
#ifdef ULTRIXFUNC
	int start;
	int num_zones;
#endif ULTRIXFUNC

	gettime(&tt);
	if(first_time)
		srandom((int)(tt.tv_usec & 0xffff));

#if defined(SYSV)
	{ int stat;
	pid = wait(&stat);
	status.w_termsig = stat & 0x7f;
	status.w_retcode = stat >> 8;
	}
#else /* SYSV */
	while ((pid = wait3(&status, WNOHANG, (struct rusage *)NULL)) > 0) {
#endif /* SYSV */
		for (zp = zones; zp < &zones[nzones]; zp++)
		    if (zp->z_xferpid == pid) {
			xfers++;
			xfers_running--;
			zp->z_xferpid = 0;
			zp->z_state &= ~Z_XFER_RUNNING;
#ifdef DEBUG
			if (debug) 
			    fprintf(ddt,
		"\nendxfer: child %d zone %s returned status=%d termsig=%d\n", 
				pid, zp->z_origin, status.w_retcode,
				status.w_termsig);
#endif
			if (status.w_termsig != 0) {
				if (status.w_termsig != SIGKILL) {
					syslog(LOG_ERR,
					   "named-xfer exited with signal %d\n",
					   status.w_termsig);
#ifdef DEBUG
					if (debug)
					    fprintf(ddt,
					 "\tchild termination with signal %d\n",
						status.w_termsig);
#endif
				}
				zp->z_time = tt.tv_sec + zp->z_retry;
			} else switch (status.w_retcode) {
				case XFER_UPTODATE:
#ifndef ULTRIXFUNC
					zp->z_state &= ~Z_SYSLOGGED;
#endif ULTRIXFUNC
					zp->z_lastupdate = tt.tv_sec;
					zp->z_time = tt.tv_sec + zp->z_refresh;
#ifndef ULTRIXFUNC
					/*
					 * Restore z_auth in case expired,
					 * but only if there were no errors
					 * in the zone file.
					 */
					if ((zp->z_state & Z_DB_BAD) == 0)
						zp->z_auth = 1;
#endif ULTRIXFUNC
					if (zp->z_source) {
#if defined(SYSV)
						struct utimbuf t;

						t.actime = tt.tv_sec;
						t.modtime = tt.tv_sec;
						(void) utime(zp->z_source, &t);
#else
						struct timeval t[2];

						t[0] = tt;
						t[1] = tt;
						(void) utimes(zp->z_source, t);
#endif /* SYSV */
					}
					break;

				case XFER_SUCCESS:
					zp->z_state |= Z_NEED_RELOAD;
#ifndef ULTRIXFUNC
					zp->z_state &= ~Z_SYSLOGGED;
#endif ULTRIXFUNC
					needzoneload++;
					break;

				case XFER_TIMEOUT:
#ifdef DEBUG
					if (debug) fprintf(ddt,
		    "zoneref: Masters for secondary zone %s unreachable\n",
					    zp->z_origin);
#endif
#ifndef ULTRIXFUNC
					if ((zp->z_state & Z_SYSLOGGED) == 0) {
						zp->z_state |= Z_SYSLOGGED;
#endif ULTRIXFUNC
						syslog(LOG_WARNING,
		      "zoneref: Masters for secondary zone %s unreachable",
						    zp->z_origin);
#ifndef ULTRIXFUNC
					}
#endif ULTRIXFUNC
					zp->z_time = tt.tv_sec + zp->z_retry;
					break;

				default:
#ifndef ULTRIXFUNC
					if ((zp->z_state & Z_SYSLOGGED) == 0) {
						zp->z_state |= Z_SYSLOGGED;
#endif ULTRIXFUNC
						syslog(LOG_ERR,
						    "named-xfer exit code %d",
						    status.w_retcode);
#ifndef ULTRIXFUNC
					}
#endif ULTRIXFUNC

					/* FALLTHROUGH */
				case XFER_FAIL:
#ifndef ULTRIXFUNC
					zp->z_state |= Z_SYSLOGGED;
#endif ULTRIXFUNC
					zp->z_time = tt.tv_sec + zp->z_retry;
					break;
			}
			break;
		}
#ifndef SYSV
	}
#endif /* SYSV */
#ifdef ULTRIXFUNC
	if (xfers) {
		start = random() % nzones;
		for (zp = &zones[start], num_zones = 0;
			xfers_deferred != 0 &&
			xfers_running < MAX_XFERS_RUNNING &&
			num_zones != nzones;
			num_zones++, zp++) {
		    if(zp > &zones[nzones])
			zp = zones;
		    if (zp->z_state & Z_NEED_XFER) {
			xfers_deferred--;
			startxfer(zp);
		    }
		}
		sched_maint();
	}
#else ULTRIXFUNC
	if (xfers) {
		for (zp = zones;
		    xfers_deferred != 0 && xfers_running < MAX_XFERS_RUNNING &&
		    zp < &zones[nzones]; zp++)
			if (zp->z_state & Z_NEED_XFER) {
				xfers_deferred--;
				startxfer(zp);
			}
		sched_maint();
	}
#endif ULTRIXFUNC
#if defined(SYSV)
	(void)signal(SIGCLD, endxfer);
#endif
}

/*
 * Reload zones whose transfers have completed.
 */
loadxfer()
{
    	register struct zoneinfo *zp;   
	int status, zonenum;

	gettime(&tt);
	for (zp = zones, zonenum = 0; zp < &zones[nzones]; zp++, zonenum++)

	    if (zp->z_state & Z_NEED_RELOAD) {
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "loadxfer() '%s'\n",
			zp->z_origin[0] ? zp->z_origin : ".");
#endif
		zp->z_state &= ~Z_NEED_RELOAD;
#ifdef ULTRIXFUNC
	 	if((zp->z_load_info = load_prep(zp, zp->z_source,
		    zp->z_origin, zonenum)) == NULL){
			zp->z_time = zp->z_retry + tt.tv_sec;
			break;
		}

		if ((status = db_load(zp->z_load_info,
				NUM_ITER_LOAD)) != -2 ) {
			/* the load is done or it failed */
			if(status >= 0 &&
			    !zp->z_load_info->format_errs) {

				/* load is done */
				replace_data(hashtab, zonenum);
				set_zp(zp->z_load_info->zp, zp);
				free_files(zp->z_files);
				zp->z_files = zp->z_load_info->files;
			} else {
				delete_zone(1, hashtab, zonenum);
				free_files(zp->z_load_info->files);
				zp->z_time = tt.tv_sec + zp->z_retry;
			}
			free(zp->z_load_info->zp);
			free(zp->z_load_info);
			zp->z_load_info = NULL;
			zp->z_state &= ~Z_SLOW_RELOAD;
		} else {
			/* The load is not finished */
			zp->z_state |= Z_SLOW_RELOAD;
			continuemaint = 1;
		}
#else ULTRIXFUNC
		zp->z_auth = 0;
		remove_zone(hashtab, zp - zones);
		if (db_load(zp->z_source, zp->z_origin, zp, 0) == 0)
			zp->z_auth = 1;
#endif ULTRIXFUNC

	    }
	sched_maint();
}

free_files(files)
	files_st *files;
{
	for(;files != (files_st *)NULL; files = files->next) {
		free(files);
	}
}
	
need_primaryload(zp)
	struct zoneinfo *zp;
{
	struct stat	f_stats;
	files_st	*files;

	if(zp->z_files == (files_st *)NULL)
		return(1);

	for(files = zp->z_files; files != (files_st *)NULL;
			files = files->next) {
		if (stat(files->f_name, &f_stats) < 0) {
			return(1);
		} else {
			if (f_stats.st_mtime > files->f_stats.st_mtime)
				return(1);
		}
	}
	return(0);
}

/*
file_copy(in, fp)
	FILE	*in;
	FILE	*out;
{
	
}
*/

set_zp(zp_in, zp_out)
     struct zoneinfo *zp_in;
     struct zoneinfo *zp_out;
{
	zp_out->z_type = zp_in->z_type;
	zp_out->z_auth = zp_in->z_auth;
	zp_out->z_origin = zp_in->z_origin;
	zp_out->z_time = zp_in->z_time;
	zp_out->z_lastupdate = zp_in->z_lastupdate;
	zp_out->z_refresh = zp_in->z_refresh;
	zp_out->z_retry = zp_in->z_retry;
	zp_out->z_expire = zp_in->z_expire;
	zp_out->z_minimum = zp_in->z_minimum;
	zp_out->z_serial = zp_in->z_serial;
	zp_out->z_source = zp_in->z_source;
	zp_out->z_ftime = zp_in->z_ftime;
	zp_out->z_addrcnt = zp_in->z_addrcnt;
	zp_out->z_sysloged = zp_in->z_sysloged;
	zp_out->z_state = zp_in->z_state;
	zp_out->z_xferpid = zp_in->z_xferpid;
	zp_out->z_class = zp_in->z_class;
	zp_out->z_files = zp_in->z_files;
	zp_out->z_load_info = zp_in->z_load_info;
}
