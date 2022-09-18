#ifndef lint
static	char	*sccsid = "@(#)db_load.c	4.2	(ULTRIX)	11/15/90";
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
 * static char sccsid[] = "@(#)db_load.c	4.26 (Berkeley) 2/28/88";
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
 * 08-Jun-88	logcher
 *	Added the correct MIT Hesiod support.
 *
 * 11-Nov-88	logcher
 *	Updated with V3.0 changes.
 *
 * 17-May-89	logcher
 *	Added BIND 4.8 with MIT Hesiod support.
 *
 * 10-Jan-90	sue
 *	Incorporated Hesiod changes from MIT to implement multiple
 *	"character strings" in a TXT record according to RFC1035.
 */


/*
 * Load data base from ascii backupfile.  Format similar to RFC 883.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <ctype.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "db.h"

extern char *index();
extern int max_cache_ttl;

/*
 * Map class and type names to number
 */
struct map {
	char	token[8];
	int	val;
};

struct map m_class[] = {
	"in",		C_IN,
#ifdef notdef
	"any",		C_ANY,		/* any is a QCLASS, not CLASS */
#endif
	"chaos",	C_CHAOS,
	"hs",		C_HS,
};
#define NCLASS (sizeof(m_class)/sizeof(struct map))

struct map m_type[] = {
	"a",		T_A,
	"ns",		T_NS,
	"cname",	T_CNAME,
	"soa",		T_SOA,
	"mb",		T_MB,
	"mg",		T_MG,
	"mr",		T_MR,
	"null",		T_NULL,
	"wks",		T_WKS,
	"ptr",		T_PTR,
	"hinfo",	T_HINFO,
	"minfo",	T_MINFO,
	"mx",		T_MX,
	"uinfo",	T_UINFO,
	"uid",		T_UID,
	"gid",		T_GID,
	"txt",		T_TXT,
#ifdef notdef
	"any",		T_ANY,		/* any is a QTYPE, not TYPE */
#endif
#ifdef ALLOW_T_UNSPEC
        "unspec",       T_UNSPEC,
#endif ALLOW_T_UNSPEC
};
#define NTYPE (sizeof(m_type)/sizeof(struct map))

/*
 * Parser token values
 */
#define CURRENT	1
#define DOT	2
#define AT	3
#define DNAME	4
#define INCLUDE	5
#define ORIGIN	6
#define ERROR	7

int	lineno;		/* current line number */

struct valuelist {
	struct valuelist *next, *prev;
	char	*name;
	char	*proto;
	short	port;
} *servicelist, *protolist;



file_ref_st *
load_prep(zp, filename, origin, zonenum)
	struct zoneinfo *zp;
	char *filename;	
	char *origin;
        int zonenum;
{
	file_ref_st *load_info;

	if((load_info = (file_ref_st *)malloc(sizeof(file_ref_st))) == NULL) {
		syslog(LOG_ERR, "load_prep: Bad Malloc.\n");

#ifdef DEBUG
		if (debug)
 			fprintf(ddt,"load_prep: Bad Malloc.\n");
#endif
		return(NULL);
	}

	if((load_info->files = (files_st *)malloc(sizeof(files_st))) == NULL) {
		syslog(LOG_ERR, "load_prep: Bad Malloc.\n");

#ifdef DEBUG
		if (debug)
 			fprintf(ddt,"load_prep: Bad Malloc.\n");
#endif
		free(load_info);
		return(NULL);
	}

	if((load_info->zp = (struct zoneinfo *)
	    malloc(sizeof(struct zoneinfo))) == NULL) {
		syslog(LOG_ERR, "load_prep: Bad Malloc.\n");

#ifdef DEBUG
		if (debug)
 			fprintf(ddt,"load_prep: Bad Malloc.\n");
#endif
		free(load_info->files);
		free(load_info);
		return(NULL);
	}

	zp->z_load_info = load_info;

	set_zp(zp, load_info->zp);

	(void) strcpy(load_info->files->f_name, filename);
	load_info->files->next = NULL;
	load_info->files_tail = load_info->files;

	load_info->fp = NULL;
	(void) strcpy(load_info->origin, origin);
	load_info->next = NULL;

	load_info->zonenum = zonenum;

	return(load_info);
}

#ifdef ULTRIXFUNC

db_load(load_info, num_iter)
	file_ref_st *load_info;	/* pointer to the structure which
				   describes what data to load. */
	int num_iter;		/* the number of iterations through the while
				   loop below that this function should go
				   through.  If an include entry is found,
				   num_iter is overrided.  If num_iter == -1
				   then no limit exists. */
{
	register struct zoneinfo *zp;
	register char domain[MAXDNAME];
	register char origin[MAXDNAME];
	register FILE *fp;

	register u_char *cp;
	register struct map *mp;
	char tmporigin[MAXDNAME];
	u_char buf[MAXDATA], *tmpdata, *tmpbuf;
	u_char data[MAXDATA];
	char *op;
	int c;
	int class, type;
	int ttl;
	struct databuf *dp;
	int i, errs = -2;

	int result;
	FILE *fp_in;
/* If T_UNSPEC is used, this can't be a register variable. */

#ifndef ALLOW_T_UNSPEC
	register u_long n;
#else
	u_long n;
#endif

	if(load_info->next != NULL) {
		
		if((result = db_load(load_info->next, num_iter)) < -2) {
			errs = result;
			goto err;
		} else {
			if(result == -2)
				return(-2);
			else {
				load_info->format_errs +=
					load_info->next->format_errs;
				free(load_info->next);
				load_info->next = NULL;
				num_iter = result;

			}
		}
	} else {
		if(num_iter < -1)
			errs--;
	}


	if(load_info->fp == NULL) {
		/* This load_info struct has not been initialized.  Start
			reading the file from the start. */

#ifdef DEBUG
		if (debug)
			fprintf(ddt,"db_load(%s, %s, %d)\n",
				load_info->zp->z_source, load_info->origin,
				load_info->zonenum);
#endif

		/*  */
		if ((load_info->fp = fopen(load_info->files->f_name, "r")) == NULL) {
			if (load_info->zp->z_type != Z_SECONDARY)
				syslog(LOG_ERR,"db_load: error opening file %s\n", 
					load_info->files->f_name);
#ifdef DEBUG
			if (debug)
			    fprintf(ddt,"db_load: error opening file %s\n",
				load_info->files->f_name);
#endif
			errs--;
			goto err;
		}

		load_info->lineno = 1;

		if (load_info->zp->z_type == Z_CACHE) {
		    load_info->dbflags = DB_NODATA | DB_NOHINTS;
		    load_info->dataflags = DB_F_HINT;
		} else {
		    load_info->dbflags = DB_NODATA;
		    load_info->dataflags = 0;
		}

		gettime(&tt);

		if (fstat(fileno(load_info->fp), &load_info->files->f_stats) < 0) {
			syslog(LOG_ERR,"db_load: error in fstat, file %s\n",
				load_info->files->f_name);
#ifdef DEBUG
			if (debug)
			    fprintf(ddt,"db_load: error in fstat file %s\n",
				load_info->files->f_name);
#endif
			errs--;
			goto err;
		}

		if(load_info->files->f_stats.st_mtime >	load_info->zp->z_ftime)
			load_info->zp->z_ftime = load_info->files->f_stats.st_mtime;

		/* A primary assumes that the data in the load file is
		   is not in the process of expiring. */
		if(load_info->zp->z_type == Z_PRIMARY)
			load_info->zp->z_lastupdate = tt.tv_sec;
		else
			load_info->zp->z_lastupdate = load_info->files->f_stats.st_mtime;

		load_info->domain[0] = '\0';
		load_info->class = C_IN;
		load_info->format_errs = 0;
	}

	/* copy the following vars into stackspace in order to prevent
	   any de-referencing */
	fp = load_info->fp;
	zp = load_info->zp;
	(void) strcpy(origin, load_info->origin);
	(void) strcpy(domain, load_info->domain);


	/* Set lineno to the line in cur_file that we are "reading". */
	lineno = load_info->lineno;
	class = load_info->class;

	/* We have started to read the file.  continue. */
	while ((num_iter == -1 || num_iter - 1 >= 0) &&
			(c = gettoken(fp)) != EOF &&
			(num_iter == -1 || --num_iter >= 0)) {
			
		switch (c) {
		case INCLUDE:
			/* An include entry implies that another file should
			   be read at this point.  Copy all of the new 
			   file to the temp file, and set environment vars */
				
			if (!getword(buf, sizeof(buf), fp)) /* file name */
				break;
			if (!getword(tmporigin, sizeof(tmporigin), fp))
				strcpy(tmporigin, origin);
			else {
				makename(tmporigin, origin);
				endline(fp);
			}

			if((load_info->next =
			    load_prep(zp, buf, tmporigin,
				      load_info->zonenum)) == NULL){
				syslog(LOG_ERR, "Bad load_prep call");
				errs--;
				goto err;
			}				
			free(load_info->next->zp);
			load_info->next->zp = zp;
			zp->z_load_info = load_info;

			load_info->lineno = lineno;

			/* if num_iter == -1 then process till the end
			   otherwise, consider the copying of an entire
			   file enough steps to use all of num_iter. */

			if (num_iter != -1)
				num_iter = 1;

			if((result = db_load(load_info->next,num_iter)) < -2) {
				free(load_info->next);
				load_info->next = NULL;
				errs = result;
				goto err;
			}

			lineno = load_info->lineno;

			load_info->files_tail->next = load_info->next->files;
			load_info->files_tail = load_info->next->files_tail;


			if(result == -2) {
				num_iter = 0;
			} else {
				load_info->format_errs +=
					load_info->next->format_errs;
				free(load_info->next);
				load_info->next = NULL;
				num_iter = result;
			}

			continue;

		case ORIGIN:
			(void) strcpy(buf, origin);
			if (!getword(origin, sizeof(origin), fp))
				break;
#ifdef DEBUG
			if (debug > 3)
				fprintf(ddt,"db_load: origin %s, buf %s\n",
				    origin, buf);
#endif
			makename(origin, buf);
#ifdef DEBUG
			if (debug > 3)
				fprintf(ddt,"db_load: origin now %s\n", origin);
#endif
			continue;

		case DNAME:
			if (!getword(domain, sizeof(domain), fp))
				break;
			n = strlen(domain) - 1;
			if (domain[n] == '.')
				domain[n] = '\0';
			else if (*origin) {
				(void) strcat(domain, ".");
				(void) strcat(domain, origin);
			}
			goto gotdomain;

		case AT:
			(void) strcpy(domain, origin);
			goto gotdomain;

		case DOT:
			domain[0] = '\0';
			/* fall thru ... */
		case CURRENT:
		gotdomain:
			if (!getword(buf, sizeof(buf), fp)) {
				if (c == CURRENT)
					continue;
				break;
			}
			cp = buf;
			ttl = 0;
			if (isdigit(*cp)) {
				n = 0;
				do
					n = n * 10 + (*cp++ - '0');
				while (isdigit(*cp));
				if (zp->z_type == Z_CACHE) {
				    /* this allows the cache entry to age */
				    /* while sitting on disk (powered off) */
				    if (n > max_cache_ttl)
					n = max_cache_ttl;
				    n += load_info->files->f_stats.st_mtime;
				}
				ttl = n;
				if (!getword(buf, sizeof(buf), fp))
					break;
			}
			for (mp = m_class; mp < m_class+NCLASS; mp++)
				if (!strcasecmp(buf, mp->token)) {
					class = mp->val;
					(void) getword(buf, sizeof(buf), fp);
					break;
				}
			for (mp = m_type; mp < m_type+NTYPE; mp++)
				if (!strcasecmp(buf, mp->token)) {
					type = mp->val;
					goto fndtype;
				}
#ifdef DEBUG
			if (debug)
				fprintf(ddt,"Line %d: Unknown type: %s.\n",
					lineno, buf);
#endif
			load_info->format_errs++;
 			syslog(LOG_ERR, "Line %d: Unknown type: %s.\n",
				lineno, buf);
			break;
		fndtype:
#ifdef ALLOW_T_UNSPEC
			/* Don't do anything here for T_UNSPEC...
			 * read input separately later
			 */
                        if (type != T_UNSPEC) {
#endif ALLOW_T_UNSPEC
			    if (!getword(buf, sizeof(buf), fp))
				break;
#ifdef DEBUG
			    if (debug >= 3)
			        fprintf(ddt,
				    "d='%s', c=%d, t=%d, ttl=%d, data='%s'\n",
				    domain, class, type, ttl, buf);
#endif
#ifdef ALLOW_T_UNSPEC
                        }
#endif ALLOW_T_UNSPEC
			/*
			 * Convert the ascii data 'buf' to the proper format
			 * based on the type and pack into 'data'.
			 */
			switch (type) {
			case T_A:
				n = ntohl((u_long)inet_addr((char *)buf));
				cp = data;
				PUTLONG(n, cp);
				n = sizeof(u_long);
				break;

			case T_HINFO:
				n = strlen(buf);
				if (n > 255) {
				    syslog(LOG_WARNING,
					"%s: line %d: CPU type too long",
					load_info->files->f_name, lineno);
				    n = 255;
				}
				data[0] = n;
				bcopy(buf, (char *)data + 1, (int)n);
				n++;
				if (!getword(buf, sizeof(buf), fp))
					break;
				i = strlen(buf);
				if (i > 255) {
				    syslog(LOG_WARNING,
					"%s: line %d: OS type too long",
					load_info->files->f_name, lineno);
				    i = 255;
				}
				data[n] = i;
				bcopy(buf, data + n + 1, i);
				n += i + 1;
				endline(fp);
				break;

			case T_SOA:
			case T_MINFO:
				(void) strcpy(data, buf);
				makename(data, origin);
				cp = data + strlen(data) + 1;
				if (!getword(cp, sizeof(data) - (cp - data),fp)) {
					n = cp - data;
					break;
				}
				makename(cp, origin);
				cp += strlen(cp) + 1;
				if (type == T_MINFO) {
					n = cp - data;
					break;
				}
				if (getnonblank(fp) != '(')
					goto format_err;
				zp->z_serial = getnum(fp);
				n = (u_long) zp->z_serial;
				PUTLONG(n, cp);
				zp->z_refresh = getnum(fp);
				n = (u_long) zp->z_refresh;
				PUTLONG(n, cp);
				/* A primary assumes that the data in the
				   file being loaded is correct now.  The
				   modification time of the file is of no
				   consequence.				*/
#ifdef PRIMARY_MAINT
				if(zp->z_type == Z_PRIMARY)
					load_info->z_time = tt + zp->z_refresh;
				else if(zp->type == Z_SECONDARY)
#else PRIMARY_MAINT
				if(zp->z_type == Z_SECONDARY)
#endif PRIMARY_MAINT
					zp->z_time =
						load_info->files->
						f_stats.st_mtime +
						zp->z_refresh;
				zp->z_retry = getnum(fp);
				n = (u_long) zp->z_retry;
				PUTLONG(n, cp)
				zp->z_expire = getnum(fp);
				n = (u_long) zp->z_expire;
				PUTLONG (n, cp);
				zp->z_minimum = getnum(fp);
				n = (u_long) zp->z_minimum;
				PUTLONG (n, cp);
				n = cp - data;
				if (getnonblank(fp) != ')')
					goto format_err;
				endline(fp);
#ifdef ULTRIXFUNC
				zp->z_class = class;
#endif ULTRIXFUNC
				break;

			case T_UID:
			case T_GID:
				n = 0;
				cp = buf;
				while (isdigit(*cp))
					n = n * 10 + (*cp++ - '0');
				if (cp == buf)
					goto format_err;
				cp = data;
				PUTLONG(n, cp);
				n = sizeof(long);
				break;

			case T_WKS:
				/* Address */
				n = ntohl((u_long)inet_addr((char *)buf));
				cp = data;
				PUTLONG(n, cp);
				*cp = getprotocol(fp, load_info->files->f_name);
				/* Protocol */
				n = sizeof(u_long) + sizeof(char);
				/* Services */
				n = getservices((int)n, data, fp,
				    load_info->files->f_name);
				break;

			case T_NS:
			case T_CNAME:
			case T_MB:
			case T_MG:
			case T_MR:
			case T_PTR:
				(void) strcpy(data, buf);
				makename(data, origin);
				n = strlen(data) + 1;
				break;

			case T_UINFO:
				cp = (u_char *)index(buf, '&');
				bzero(data, sizeof(data));
				if ( cp != NULL) {
					(void) strncpy(data, buf, cp - buf);
					op = index(domain, '.');
					if ( op != NULL)
					    (void) strncat(data,
						domain,op-domain);
					else
						(void) strcat(data, domain);
					(void) strcat(data, ++cp);
				} else
					(void) strcpy(data, buf);
				n = strlen(data) + 1;
				break;
			case T_MX:
				n = 0;
				cp = buf;
				while (isdigit(*cp))
					n = n * 10 + (*cp++ - '0');
				/* catch bad values */
				if ((cp == buf) || (n > 64535))
					goto format_err;

				cp = data;
				PUTSHORT((u_short)n, cp);

				if (!getword(buf, sizeof(buf), fp))
					    break;
				(void) strcpy(cp,buf);
				makename(cp, origin);
				/* get pointer to place in data */
				cp += strlen(cp) +1;

				/* now save length */
				n = (cp - data);
				break;
			case T_TXT:
				i = strlen(buf);
				n = 0;
				tmpdata = &data[0];
				tmpbuf = buf;
				while (i > 255) {
					*tmpdata++ = 255;
					bcopy(tmpbuf, tmpdata, 255);
					tmpdata += 255;
					tmpbuf += 255;
					i -= 255;
					n += 256;
				}
				*tmpdata++ = i;
				bcopy(tmpbuf, tmpdata, i);
				n += i + 1;
				break;
#ifdef ALLOW_T_UNSPEC
                        case T_UNSPEC:
                                {
                                    int rcode;
                                    fgets(buf, sizeof(buf), fp);
#ifdef DEBUG
				    if (debug)
                                    	fprintf(ddt, "loading T_UNSPEC\n");
#endif DEBUG
                                    if (rcode = atob(buf, strlen(buf), data,                                                         MAXDATA, &n)) {
                                        if (rcode == CONV_OVERFLOW) {
#ifdef DEBUG
                                            if (debug)
                                               fprintf(ddt,
		       				   "Load T_UNSPEC: input buffer overflow\n");
#endif DEBUG
					    load_info->format_errs++;
                                            syslog(LOG_ERR,
						 "Load T_UNSPEC: input buffer overflow");
                                         } else {
#ifdef DEBUG
                                            if (debug)
                                                fprintf(ddt,
						   "Load T_UNSPEC: Data in bad atob format\n");
#endif DEBUG
					    load_info->format_errs++;
                                            syslog(LOG_ERR,
						   "Load T_UNSPEC: Data in bad atob format");
                                         }
                                    }
                                }
                                break;
#endif ALLOW_T_UNSPEC

			default:
				goto format_err;
			}
#ifdef AUTHEN
			dp = savedata(class, type, (u_long)ttl, AUTH_CACHE,
				      ONE, data, (int)n);
#else AUTHEN
			dp = savedata(class, type, (u_long)ttl, data, (int)n);
#endif AUTHEN
			dp->d_zone = load_info->zonenum;
			dp->d_flags = load_info ->dataflags;
			if ((c = db_update(domain, dp, dp, load_info->dbflags
			   | DB_LOADDB,
			   (zp->z_type == Z_CACHE)? fcachetab : hashtab)) < 0) {
#ifdef DEBUG
				if (debug && (c != DATAEXISTS))
					fprintf(ddt,"update failed\n");
#endif
				free( (struct databuf *) dp);
			}
			continue;

		case ERROR:
			break;
		}

	format_err:

		load_info->format_errs++;
		syslog(LOG_ERR, "%s: line %d: database format error (%s)",
			load_info->files->f_name, lineno, buf);
#ifdef DEBUG
		if (debug)
			fprintf(ddt,"%s: line %d: database format error ('%s', %d)\n",
				load_info->files->f_name, lineno, buf, n);
#endif
		while ((c = getc(fp)) != EOF && c != '\n')
			;
		if (c == '\n')
			lineno++;
	}

err:
	if(errs < -2) {
		/* the load encountered an error */
		if(load_info->next != NULL) {
			free(load_info->next);
			load_info->next = NULL;
		}
		if(load_info->fp)
			fclose(load_info->fp);

		return(errs);
	}
	
	if(c != EOF) {
		/* the load is not finished but so far there are no errors */

		/* copy the following vars back into the load_info struct */
		(void) strcpy(load_info->origin, origin);
		(void) strcpy(load_info->domain, domain);
		load_info->class = class;

		/* Set load_info->lineno to the line in fp that we
			are "reading". */
		load_info->lineno = lineno;


		return(-2);
	} else {
		/* the load finished correctly */
		(void) fclose(load_info->fp);

		return(num_iter);
	}

}
#else ULTRIXFUNC
/*
 * Load the database from 'filename'. Origin is appended to all domain
 * names in the file.
 */
db_load(filename, in_origin, zp)
	char *filename, *in_origin;
	struct zoneinfo *zp;
{
	register u_char *cp;
	register struct map *mp;
	char domain[MAXDNAME];
	char origin[MAXDNAME];
	char tmporigin[MAXDNAME];
	u_char buf[MAXDATA], *tmpdata, *tmpbuf;
	u_char data[MAXDATA];
	char *op;
	int c;
	int class, type, ttl, dbflags, dataflags;
	struct databuf *dp;
	FILE *fp;
	int slineno, i, errs = 0, didinclude = 0;

/* If T_UNSPEC is used, this can't be a register variable. */

#ifndef ALLOW_T_UNSPEC
	register u_long n;
#else
	u_long n;
#endif
	struct stat sb;

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"db_load(%s, %s, %d)\n",
		    filename, in_origin, zp - zones);
#endif

	(void) strcpy(origin, in_origin);
	if ((fp = fopen(filename, "r")) == NULL) {
		if (zp->z_type != Z_SECONDARY)
			syslog(LOG_ERR, "%s: %m", filename);
#ifdef DEBUG
		if (debug)
		    fprintf(ddt,"db_load: error opening file %s\n", filename);
#endif
		return (-1);
	}
	if (zp->z_type == Z_CACHE) {
	    dbflags = DB_NODATA | DB_NOHINTS;
	    dataflags = DB_F_HINT;
	} else {
	    dbflags = DB_NODATA;
	    dataflags = 0;
	}
	gettime(&tt);
	if (fstat(fileno(fp), &sb) < 0) {
	    syslog(LOG_ERR, "%s: %m", filename);
	    sb.st_mtime = (int)tt.tv_sec;
	}
	slineno = lineno;
	lineno = 1;
	domain[0] = '\0';
	class = C_IN;
	while ((c = gettoken(fp)) != EOF) {
		switch (c) {
		case INCLUDE:
			if (!getword(buf, sizeof(buf), fp)) /* file name */
				break;
			if (!getword(tmporigin, sizeof(tmporigin), fp))
				strcpy(tmporigin, origin);
			else {
				makename(tmporigin, origin);
				endline(fp);
			}
			didinclude = 1;
			errs += db_load(buf, tmporigin, zp);
			continue;

		case ORIGIN:
			(void) strcpy(buf, origin);
			if (!getword(origin, sizeof(origin), fp))
				break;
#ifdef DEBUG
			if (debug > 3)
				fprintf(ddt,"db_load: origin %s, buf %s\n",
				    origin, buf);
#endif
			makename(origin, buf);
#ifdef DEBUG
			if (debug > 3)
				fprintf(ddt,"db_load: origin now %s\n", origin);
#endif
			continue;

		case DNAME:
			if (!getword(domain, sizeof(domain), fp))
				break;
			n = strlen(domain) - 1;
			if (domain[n] == '.')
				domain[n] = '\0';
			else if (*origin) {
				(void) strcat(domain, ".");
				(void) strcat(domain, origin);
			}
			goto gotdomain;

		case AT:
			(void) strcpy(domain, origin);
			goto gotdomain;

		case DOT:
			domain[0] = '\0';
			/* fall thru ... */
		case CURRENT:
		gotdomain:
			if (!getword(buf, sizeof(buf), fp)) {
				if (c == CURRENT)
					continue;
				break;
			}
			cp = buf;
			ttl = 0;
			if (isdigit(*cp)) {
				n = 0;
				do
					n = n * 10 + (*cp++ - '0');
				while (isdigit(*cp));
				if (zp->z_type == Z_CACHE) {
				    /* this allows the cache entry to age */
				    /* while sitting on disk (powered off) */
				    if (n > max_cache_ttl)
					n = max_cache_ttl;
				    n += sb.st_mtime;
				}
				ttl = n;
				if (!getword(buf, sizeof(buf), fp))
					break;
			}
			for (mp = m_class; mp < m_class+NCLASS; mp++)
				if (!strcasecmp(buf, mp->token)) {
					class = mp->val;
					(void) getword(buf, sizeof(buf), fp);
					break;
				}
			for (mp = m_type; mp < m_type+NTYPE; mp++)
				if (!strcasecmp(buf, mp->token)) {
					type = mp->val;
					goto fndtype;
				}
#ifdef DEBUG
			if (debug)
				fprintf(ddt,"Line %d: Unknown type: %s.\n",
					lineno, buf);
#endif
			errs++;
 			syslog(LOG_ERR, "Line %d: Unknown type: %s.\n",
				lineno, buf);
			break;
		fndtype:
#ifdef ALLOW_T_UNSPEC
			/* Don't do anything here for T_UNSPEC...
			 * read input separately later
			 */
                        if (type != T_UNSPEC) {
#endif ALLOW_T_UNSPEC
			    if (!getword(buf, sizeof(buf), fp))
				break;
#ifdef DEBUG
			    if (debug >= 3)
			        fprintf(ddt,
				    "d='%s', c=%d, t=%d, ttl=%d, data='%s'\n",
				    domain, class, type, ttl, buf);
#endif
#ifdef ALLOW_T_UNSPEC
                        }
#endif ALLOW_T_UNSPEC
			/*
			 * Convert the ascii data 'buf' to the proper format
			 * based on the type and pack into 'data'.
			 */
			switch (type) {
			case T_A:
				n = ntohl((u_long)inet_addr((char *)buf));
				cp = data;
				PUTLONG(n, cp);
				n = sizeof(u_long);
				break;

			case T_HINFO:
				n = strlen(buf);
				if (n > 255) {
				    syslog(LOG_WARNING,
					"%s: line %d: CPU type too long",
					filename, lineno);
				    n = 255;
				}
				data[0] = n;
				bcopy(buf, (char *)data + 1, (int)n);
				n++;
				if (!getword(buf, sizeof(buf), fp))
					break;
				i = strlen(buf);
				if (i > 255) {
				    syslog(LOG_WARNING,
					"%s: line %d: OS type too long",
					filename, lineno);
				    i = 255;
				}
				data[n] = i;
				bcopy(buf, data + n + 1, i);
				n += i + 1;
				endline(fp);
				break;

			case T_SOA:
			case T_MINFO:
				(void) strcpy(data, buf);
				makename(data, origin);
				cp = data + strlen(data) + 1;
				if (!getword(cp, sizeof(data) - (cp - data),fp)) {
					n = cp - data;
					break;
				}
				makename(cp, origin);
				cp += strlen(cp) + 1;
				if (type == T_MINFO) {
					n = cp - data;
					break;
				}
				if (getnonblank(fp) != '(')
					goto err;
				zp->z_serial = getnum(fp);
				n = (u_long) zp->z_serial;
				PUTLONG(n, cp);
				zp->z_refresh = getnum(fp);
				n = (u_long) zp->z_refresh;
				PUTLONG(n, cp);
				zp->z_time = sb.st_mtime + zp->z_refresh;
				zp->z_retry = getnum(fp);
				n = (u_long) zp->z_retry;
				PUTLONG(n, cp);
				zp->z_expire = getnum(fp);
				n = (u_long) zp->z_expire;
				PUTLONG (n, cp);
				zp->z_minimum = getnum(fp);
				n = (u_long) zp->z_minimum;
				PUTLONG (n, cp);
				n = cp - data;
				if (getnonblank(fp) != ')')
					goto err;
				endline(fp);
#ifdef ULTRIXFUNC
				zp->z_class = class;
#endif ULTRIXFUNC
				break;

			case T_UID:
			case T_GID:
				n = 0;
				cp = buf;
				while (isdigit(*cp))
					n = n * 10 + (*cp++ - '0');
				if (cp == buf)
					goto err;
				cp = data;
				PUTLONG(n, cp);
				n = sizeof(long);
				break;

			case T_WKS:
				/* Address */
				n = ntohl((u_long)inet_addr((char *)buf));
				cp = data;
				PUTLONG(n, cp);
				*cp = getprotocol(fp, filename);
				/* Protocol */
				n = sizeof(u_long) + sizeof(char);
				/* Services */
				n = getservices((int)n, data, fp, filename);
				break;

			case T_NS:
			case T_CNAME:
			case T_MB:
			case T_MG:
			case T_MR:
			case T_PTR:
				(void) strcpy(data, buf);
				makename(data, origin);
				n = strlen(data) + 1;
				break;

			case T_UINFO:
				cp = (u_char *)index(buf, '&');
				bzero(data, sizeof(data));
				if ( cp != NULL) {
					(void) strncpy(data, buf, cp - buf);
					op = index(domain, '.');
					if ( op != NULL)
					    (void) strncat(data,
						domain,op-domain);
					else
						(void) strcat(data, domain);
					(void) strcat(data, ++cp);
				} else
					(void) strcpy(data, buf);
				n = strlen(data) + 1;
				break;
			case T_MX:
				n = 0;
				cp = buf;
				while (isdigit(*cp))
					n = n * 10 + (*cp++ - '0');
				/* catch bad values */
				if ((cp == buf) || (n > 64535))
					goto err;

				cp = data;
				PUTSHORT((u_short)n, cp);

				if (!getword(buf, sizeof(buf), fp))
					    break;
				(void) strcpy(cp,buf);
				makename(cp, origin);
				/* get pointer to place in data */
				cp += strlen(cp) +1;

				/* now save length */
				n = (cp - data);
				break;
			case T_TXT:
				i = strlen(buf);
				n = 0;
				tmpdata = &data[0];
				tmpbuf = buf;
				while (i > 255) {
					*tmpdata++ = 255;
					bcopy(tmpbuf, tmpdata, 255);
					tmpdata += 255;
					tmpbuf += 255;
					i -= 255;
					n += 256;
				}
				*tmpdata++ = i;
				bcopy(tmpbuf, tmpdata, i);
				n += i + 1;
				break;
#ifdef ALLOW_T_UNSPEC
                        case T_UNSPEC:
                                {
                                    int rcode;
                                    fgets(buf, sizeof(buf), fp);
#ifdef DEBUG
				    if (debug)
                                    	fprintf(ddt, "loading T_UNSPEC\n");
#endif DEBUG
                                    if (rcode = atob(buf, strlen(buf), data,                                                         MAXDATA, &n)) {
                                        if (rcode == CONV_OVERFLOW) {
#ifdef DEBUG
                                            if (debug)
                                               fprintf(ddt,
		       				   "Load T_UNSPEC: input buffer overflow\n");
#endif DEBUG
					    errs++;
                                            syslog(LOG_ERR,
						 "Load T_UNSPEC: input buffer overflow");
                                         } else {
#ifdef DEBUG
                                            if (debug)
                                                fprintf(ddt,
						   "Load T_UNSPEC: Data in bad atob format\n");
#endif DEBUG
					    errs++;
                                            syslog(LOG_ERR,
						   "Load T_UNSPEC: Data in bad atob format");
                                         }
                                    }
                                }
                                break;
#endif ALLOW_T_UNSPEC

			default:
				goto err;
			}
#ifdef AUTHEN
			dp = savedata(class, type, (u_long)ttl, AUTH_CACHE,
				      ONE, data, (int)n);
#else AUTHEN
			dp = savedata(class, type, (u_long)ttl, data, (int)n);
#endif AUTHEN
			dp->d_zone = zp - zones;
			dp->d_flags = dataflags;
			if ((c = db_update(domain, dp, dp, dbflags,
			   (zp->z_type == Z_CACHE)? fcachetab : hashtab)) < 0) {
#ifdef DEBUG
				if (debug && (c != DATAEXISTS))
					fprintf(ddt,"update failed\n");
#endif
			}
			continue;

		case ERROR:
			break;
		}
	err:
		errs++;
		syslog(LOG_ERR, "%s: line %d: database format error (%s)",
			filename, lineno, buf);
#ifdef DEBUG
		if (debug)
			fprintf(ddt,"%s: line %d: database format error ('%s', %d)\n",
				filename, lineno, buf, n);
#endif
		while ((c = getc(fp)) != EOF && c != '\n')
			;
		if (c == '\n')
			lineno++;
	}
	(void) fclose(fp);
	if (didinclude)
		zp->z_ftime = 0;
	else
		zp->z_ftime = sb.st_mtime;
	zp->z_lastupdate = sb.st_mtime;
	lineno = slineno;
	return (errs);
}
#endif ULTRIXFUNC

int gettoken(fp)
	register FILE *fp;
{
	register int c;
	int junk;
	char op[32];

	for (;;) {
		c = (int)getc(fp);
	top:
		switch (c) {
		case EOF:
			return (EOF);
/*		case 127:
			return (EOF);*/

		case '$':
			if (getword(op, sizeof(op), fp)) {
				if (!strcasecmp("include", op))
					return (INCLUDE);
				if (!strcasecmp("origin", op))
					return (ORIGIN);
			}
#ifdef DEBUG
			if (debug)
				fprintf(ddt,"Line %d: Unknown $ option: $%s\n", 
				    lineno, op);
#endif
			syslog(LOG_ERR,"Line %d: Unknown $ option: $%s\n", 
			    lineno, op);
			return (ERROR);

		case ';':
			while ((c = getc(fp)) != EOF && c != '\n')
				;
			goto top;

		case ' ':
		case '\t':
			return (CURRENT);

		case '.':
			return (DOT);

		case '@':
			return (AT);

		case '\n':
			lineno++;
			continue;

		default:
/*			junk =  ungetc(c, fp);*/
			(void) ungetc(c, fp);
			return (DNAME);
		}
	}
}

/*
 * Get next word, skipping blanks & comments.
 */
getword(buf, size, fp)
	char *buf;
	int size;
	FILE *fp;
{
	register char *cp;
	register int c;

	for (cp = buf; (c = getc(fp)) != EOF; ) {
		if (c == ';') {
			while ((c = getc(fp)) != EOF && c != '\n')
				;
			c = '\n';
		}
		if (c == '\n') {
			if (cp != buf)
				(void) ungetc(c, fp);
			else
				lineno++;
			break;
		}
		if (isspace(c)) {
			while (isspace(c = getc(fp)) && c != '\n')
				;
			(void) ungetc(c, fp);
			if (cp != buf)		/* Trailing whitespace */
				break;
			continue;		/* Leading whitespace */
		}
		if (c == '"') {
			while ((c = getc(fp)) != EOF && c != '"' && c != '\n') {
				if (c == '\\') {
					if ((c = getc(fp)) == EOF)
						c = '\\';
					if (c == '\n')
						lineno++;
				}
				if (cp >= buf+size-1)
					break;
				*cp++ = c;
			}
			if (c == '\n') {
				lineno++;
				break;
			}
			continue;
		}
		if (c == '\\') {
			if ((c = getc(fp)) == EOF)
				c = '\\';
			if (c == '\n')
				lineno++;
		}
		if (cp >= buf+size-1)
			break;
		*cp++ = c;
	}
	*cp = '\0';
	return (cp != buf);
}

getnum(fp)
	FILE *fp;
{
	register int c, n;
	int seendigit = 0;
	int seendecimal = 0;

	for (n = 0; (c = getc(fp)) != EOF; ) {
		if (isspace(c)) {
			if (c == '\n')
				lineno++;
			if (seendigit)
				break;
			continue;
		}
		if (c == ';') {
			while ((c = getc(fp)) != EOF && c != '\n')
				;
			if (c == '\n')
				lineno++;
			if (seendigit)
				break;
			continue;
		}
		if (!isdigit(c)) {
			if (seendecimal || c != '.') {
				syslog(LOG_ERR, "line %d: expected a number",
				lineno);
#ifdef DEBUG
				if (debug)
				    fprintf(ddt,"line %d: expected a number",
				        lineno);
#endif
				exit(1);
			} else {
				if (!seendigit)
					n = 1;
				n = n * 1000 ;
				seendigit = 1;
				seendecimal = 1;
			}
			continue;
		}
		n = n * 10 + (c - '0');
		seendigit = 1;
	}
	return (n);
}

getnonblank(fp)
	FILE *fp;
{
	register int c;

	while ( (c = getc(fp)) != EOF ) {
		if (isspace(c)) {
			if (c == '\n')
				lineno++;
			continue;
		}
		if (c == ';') {
			while ((c = getc(fp)) != EOF && c != '\n')
				;
			if (c == '\n')
				lineno++;
			continue;
		}
		return(c);
	}
	syslog(LOG_ERR, "line %d: unexpected EOF", lineno);
#ifdef DEBUG
	if (debug)
		fprintf(ddt, "line %d: unexpected EOF", lineno);
#endif
	return (EOF);
}

/*
 * Take name and caonoicalize it according to following rules:
 *
 * "." means root.
 * "@" means current origin.
 * "name." means no changes.
 * "name" means append origin.
 *
 * INPUT: The name to be canonicalized(name) and the known origin(origin).
 * OUTPUT: The canonicalized name(name).
 *
 */
makename(name, origin)
	char *name, *origin;
{
	int n;

	if (origin[0] == '.')
		origin++;
	n = strlen(name);
	if (n == 1) {
		if (name[0] == '.') {
			name[0] = '\0';
			return;
		}
		if (name[0] == '@') {
			(void) strcpy(name, origin);
			return;
		}
	}
	if (n > 0) {
		if (name[n - 1] == '.')
			name[n - 1] = '\0';
		else if (origin[0] != '\0') {
			name[n] = '.';
			(void) strcpy(name + n + 1, origin);
		}
	}
}

endline(fp)
	register FILE *fp;
{
     register int c;
     while (c = getc(fp))
	if (c == '\n') {
	    (void) ungetc(c,fp);
	    break;
	} else if (c == EOF)
	    break;
}

#define MAXPORT 256
#define MAXLEN 24

getprotocol(fp, src)
	FILE *fp;
	char *src;
{
	int  k;
	char b[MAXLEN];

	(void) getword(b, sizeof(b), fp);
		
	k = findservice(b, &protolist);
	if(k == -1) {
		(void) sscanf(b,"%d",&k);
		if(k <= 0)
			syslog(LOG_ERR, "%s: line %d: unknown protocol: %s.",
				src, lineno, b);
	}
	return(k);
}

int
getservices(n, data, fp, src)
	int n;
	char *data, *src;
	FILE *fp;
{
	int j, ch;
	int k;
	int maxl;
	int bracket;
	char b[MAXLEN];
	char bm[MAXPORT/8];

	for (j = 0; j < MAXPORT/8; j++)
		bm[j] = 0;
	maxl = 0;
	bracket = 0;
	while (getword(b, sizeof(b), fp) || bracket) {
		if (feof(fp) || ferror(fp))
			break;
		if (strlen(b) == 0)
			continue;
		if ( b[0] == '(') {
			bracket++;
 			continue;
		}
		if ( b[0] == ')') {
			bracket = 0;
			while ((ch = getc(fp)) != EOF && ch != '\n')
				;
			if (ch == '\n')
				lineno++;
			break;
		}
		k = findservice(b, &servicelist);
		if (k == -1) {
			(void) sscanf(b,"%d",&k);
			if (k <= 0) {
				syslog(LOG_WARNING,
			 "%s: line %d: Unknown service '%s'", src, lineno, b);
				continue;
			 }
		}
		if ((k < MAXPORT) && (k)) {
			bm[k/8] |= (0x80>>(k%8));
			if (k > maxl)
				maxl=k;
		}
		else {
			syslog(LOG_WARNING,
			    "%s: line %d: port no. (%d) too big\n",
				src, lineno, k);
#ifdef DEBUG
			if (debug)
				fprintf(ddt,
				    "%s: line %d: port no. (%d) too big\n",
					src, lineno, k);
#endif
		}
	}
	if (bracket)
		syslog(LOG_WARNING, "%s: line %d: missing close paren\n",
		    src, lineno);
	maxl = maxl/8+1;
	bcopy(bm, data+n, maxl);
	return(maxl+n);
}

get_sort_list(fp)
	FILE *fp;
{
	struct netinfo *tp;
	struct netinfo *ntp = NULL;
	struct netinfo *ntip = NULL;
	char buf[BUFSIZ];

	extern struct	netinfo *fnettab;

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"sortlist ");
#endif
	if (fnettab) {
#ifdef DEBUG
		/* We can only handle the sortlist at startup. see ns_main */
		if (debug)
			fprintf(ddt," (reloading, therefore ignored)\n");
#endif
		(void) endline(fp);
		return;
	}

	while (getword(buf, sizeof(buf), fp)) {
		if (strlen(buf) == 0)
			break;
#ifdef DEBUG
		if (debug)
			fprintf(ddt," %s",buf);
#endif
		if (ntp == NULL)
			ntp = (struct netinfo *)malloc(sizeof(struct netinfo));
		ntp->my_addr.s_addr = inet_addr(buf);
		if ( ntp->my_addr.s_addr == (unsigned)-1) {
			/* resolve name to address - XXX */
			continue;	
		}
		ntp->next = NULL;
		ntp->mask = net_mask(ntp->my_addr);
		ntp->net = ntp->my_addr.s_addr & ntp->mask;

		/* Check for duplicates, then add to linked list */
		for (tp = fnettab; tp != NULL; tp = tp->next) {
			if ((ntp->mask == tp->mask) && 
			    (ntp->net == tp->net))
				continue;
		}
		if (fnettab == NULL)
			fnettab = ntp;
		else
			ntip->next = ntp;
		ntip = ntp;
		ntp = NULL;
	}
	if (ntp)
		free((char *)ntp);
	
#ifdef DEBUG
	if (debug) 
		fprintf(ddt,"\n");
	if (debug > 2)
		for (ntp = fnettab; ntp != NULL; ntp = ntp->next) {
			fprintf(ddt,"ntp x%x net x%x mask x%x", ntp, 
			ntp->net, ntp->mask);
			fprintf(ddt," my_addr x%x", ntp->my_addr);
			fprintf(ddt," %s",inet_ntoa(ntp->my_addr));
			fprintf(ddt," next x%x\n", ntp->next);
		}
#endif
}
