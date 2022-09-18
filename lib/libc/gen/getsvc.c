#ifndef lint
static	char	*sccsid = "@(#)getsvc.c	4.5	(ULTRIX)	11/9/90";
#endif lint
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   This software is  derived  from  software  received  from  the     *
 *   Sun Microsystems, Inc,   and   from   Bell Laboratories.  Use,     *
 *   duplication, or disclosure is  subject  to restrictions  under     *
 *   license  agreements  with  Sun Microsystems, Inc. and with AT&T.   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/*
 * Modification History:
 *
 * 08-Nov-90	jsd
 *	Replace fscanf with fgets and cleaner logic to save time and space.
 *
 * 13-Nov-89	sue
 *	Added an fclose to an error leg in init_svc().
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/svcinfo.h>
#include <strings.h>
#include <sys/stat.h>

#define STUFFSIZE 1000

int svc_lastlookup; /* svc_lastlookup is used externally */

/* strings in file svc.conf */

static char * databases[] = {
	"aliases",
	"auth",
	"group",
	"hosts",
	"netgroup",
	"networks",
	"passwd",
	"protocols",
	"rpc",
	"services",
	""
};

struct svcinfo svc_info;
struct stat svc_stat;

struct svcinfo *
getsvc()
{

	if (stat(SVC_CONF, &svc_stat) < 0) {
		(void) fprintf(stderr, "getsvc: stat of %s failed\n", SVC_CONF);
		perror("getsvc: stat failed");
		return(NULL);
	}
	if (svc_stat.st_mtime != (time_t)svc_info.svcdate)
		if ((init_svc()) == NULL) {
			(void) fprintf(stderr, "getsvc: cannot initialize from %s\n", SVC_CONF);
			return(NULL);
		}
	return(&svc_info);
}

init_svc()
{

	FILE *stream;
	char stuff[STUFFSIZE];

	if ((stream = fopen (SVC_CONF, "r")) == 0) {
	        fprintf(stderr, "init_svc: cannot open %s, errno = %d\n", SVC_CONF, errno);
		return(NULL);
	}
	else {
		while (getsvcstring(stream,stuff) > 0)
			if (map_string_to_data (stuff) == -1) {
				(void) fprintf(stderr, "init_svc: bad format in file %s\n", SVC_CONF);
        			if (fclose(stream) == EOF)
					(void) fprintf(stderr, "init_svc: cannot close %s, errno = %d\n", SVC_CONF, errno);
				return(NULL);
			}
	}
        if (fclose(stream) == EOF)
		(void) fprintf(stderr, "init_svc: cannot close %s, errno = %d\n", SVC_CONF, errno);
	(time_t)svc_info.svcdate=svc_stat.st_mtime;
	return(1);
}

/* This function reads lines from /etc/svc.conf.  It returns each
 * line seperately, stripped of comments and white space.
 * It returns NULL at end of file.
 */
getsvcstring(stream,buff)
	FILE * stream;
	char *buff;
{
	char *cp;
	int i;

top:
	if (fgets(buff, STUFFSIZE-1, stream) == NULL)
		return(NULL);

	/* zap the newline */
	buff[strlen(buff)-1] = NULL;

	if ((buff[0] == NULL) || (buff[0] == '#'))
		goto top;	/* skip blank lines or comment lines */

	/* zap trailing comment and/or white space */
	cp=buff;
	i=0;
	while (*cp) {
		if ((*cp == ' ') || (*cp == '\t')) {
			cp++;	/* skip over space or tab */
		} else {
			if (*cp == '#')	/* stop if we reach a comment */
				break;
			buff[i++] = *cp++;
		}
	}
	buff[i] = NULL;
	if (i == 0)	/* blank line (white space only) */
		goto top;
	/* printf("buff=|%s|\n",buff); */
	return(strlen(buff));
}

map_string_to_data (p)
	char *p;
{
	char *rhs, *s, *eol;
	int i, j;
        int lastone;

	if (*p == '#')
	      return(NULL);
	if ((rhs = strchr( p, '=')) == (char *)0)
		return(-1);
	*rhs = '\0';

	i = 0;
	s = databases[0];
	while (*s != '\0') {
		if (strcmp(s,p) == 0)
			break;
		i++;
		s = databases[i];
	}
	rhs++;
	for (eol = rhs; eol && *eol && (*eol != ' ' || *eol != '\t' || *eol != '\n'); eol++)
		;
	*eol = '\0';
	if (*s == '\0') {
		if (strcmp("PASSLENMIN",p) == 0)
			svc_info.svcauth.passlenmin = atoi(rhs);
		else if (strcmp("PASSLENMAX",p) == 0)
			svc_info.svcauth.passlenmax = atoi(rhs);
		else if (strcmp("SOFTEXP",p) == 0)
			svc_info.svcauth.softexp = atoi(rhs);
		else if (strcmp("SECLEVEL",p) == 0) {
			if (strcmp("ENHANCED",rhs) == 0)
				svc_info.svcauth.seclevel = SEC_ENHANCED;
			else if (strcmp("BSD",rhs) == 0)
				svc_info.svcauth.seclevel = SEC_BSD;
			else if (strcmp("UPGRADE",rhs) == 0)
			      	svc_info.svcauth.seclevel = SEC_UPGRADE;
			else
				return(-1);
		}
		else
			return(-1);
		return(NULL);
	}

        lastone = 0;
	for (j=0; ; j++) {
		s = rhs;
		while (1) {
			if (*s == ',')
				break;
			if (*s == '\0') {
				lastone = 1;
				break;
			}
			s++;
        	}
		*s = '\0';
 		if (strcmp("local", rhs) == 0)
			svc_info.svcpath[i][j] = SVC_LOCAL;
		else if (strcmp("yp", rhs) == 0)
			svc_info.svcpath[i][j] = SVC_YP;
		else if (strcmp("bind", rhs) == 0)
			svc_info.svcpath[i][j] = SVC_BIND;
		else
			return(-1);
		if (lastone) {
			svc_info.svcpath[i][j+1] = SVC_LAST;
			break;
		}
		rhs = ++s;
	}
	return(NULL);
}
