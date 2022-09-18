#ifndef lint
static char *sccsid = "@(#)auto_subr.c	4.1      (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1987 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */
/*
 *	Modification History:
 * 
 *      10 Nov 89 -- lebel
 *              Added direct maps, bugfixes, metacharacter handling and
 *              other fun stuff from the reference tape.
 * 	14 Jun 89 -- condylis
 *		Added copyright header.
 *
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <nfs/nfs_clnt.h>
#include <nfs/vfs.h>
#include <nfs/nfs_gfs.h>
#include <ctype.h>
#include <stdio.h>
#include <mntent.h>


#define	MINRSIZE	512
#define MAXRSIZE	8192
#define	MINWSIZE	512
#define MAXWSIZE	8192
#define MINTIMEO	2
#define MAXTIMEO	1000
#define MINRETRANS	1
#define	MAXRETRANS	10000
#define MINRETRY	1
#define MAXRETRY	10000

#define PGUNITS		1024	/* to convert MINPGTHRESH to K */
#define SIXTYFOUR	64	/* default page threshhold */


char * hasmntopt();

int	ro = 0;
int	verbose = 0;
int	printed = 0;
struct	nfs_gfs_mount args;
u_short port;
int	retry;		/* number of times to retry a mount request */
int	bg;		/* put this mount in background if no answer? */

#define	BGSLEEP	5	/* initial sleep time for background mount in seconds */
#define MAXSLEEP 120	/* max sleep time for background mount in seconds */

extern int errno;

char	*index(), *rindex();
char	host[MNTMAXSTR];
char	name[MNTMAXSTR];
char	dir[MNTMAXSTR];
char	opts[MNTMAXSTR];
char	tmpopts[MNTMAXSTR];


static char *
mntopt(p)
char **p;
{
	char *cp = *p;
	char *retstr;
	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && *cp != ',')
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return (retstr);
}

char *
hasmntopt(mnt, opt)
register struct mntent *mnt;
register char *opt;
{
	char *f, *o;

	strcpy(tmpopts, mnt->mnt_opts);
	o = tmpopts;
	f = mntopt(&o);
	for (; *f; f = mntopt(&o)) {
		if (strncmp(opt, f, strlen(opt)) == 0)
			return (f - tmpopts + mnt->mnt_opts);
	} 
	return (NULL);
}

removemntopt(mnt,opt)
register struct mntent *mnt;
register char *opt;
{
	char *optp, *optend;

	if ((optp = hasmntopt(mnt, opt)) != NULL) {
		optend = index(optp, ',');
		if (optp != mnt->mnt_opts) {
			optp--;
			if (optend == NULL)
				*optp = '\0';
		}
		else {
			if (optend == NULL)
				*optp = '\0';
			else
				optend++;
		}
		if (optend != NULL)			
			while (*optp++ = *optend++)
				;
		return (1);
	}
	else
		return (0);
}

prmount()
{
	struct v_fs_data fsdata;
	struct mntent mnt;
	char optbuf[MNTMAXSTR];

	mnt.mnt_opts = optbuf;

	while (read(0, &fsdata, sizeof(struct v_fs_data)) == 
		sizeof(struct v_fs_data)) {

	opts[0] = '\0';
	strcpy(mnt.mnt_opts, fsdata.fd_un.gvfs.mi.mi_optstr);
	while (removemntopt(&mnt, MNTOPT_RO)) ;
	while (removemntopt(&mnt, MNTOPT_RW)) ;
	while (removemntopt(&mnt, MNTOPT_HARD)) ;
	while (removemntopt(&mnt, MNTOPT_SOFT)) ;

	if (fsdata.fd_flags & M_RONLY)
		strcat(opts, MNTOPT_RO);
 	else
		strcat(opts, MNTOPT_RW);
	strcat(opts, ",");

	if (fsdata.fd_un.gvfs.mi.mi_hard)
		strcat(opts, MNTOPT_HARD);
	else
		strcat(opts, MNTOPT_SOFT);

	if (mnt.mnt_opts[0] != '\0')
		strcat(opts, ",");
	strcat(opts, mnt.mnt_opts);

	(void) fprintf(stdout, "%s on %s type nfs (%s)\n",
		fsdata.fd_devname, fsdata.fd_path, opts);

	}
	return(0);
}

/*
 * Returns true if s1 is a pathname substring of s2.
 */
substr(s1, s2)
char *s1;
char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
}

getflags(mnt)
struct mntent *mnt;
{

	char optbuf[MNTMAXSTR];
	register char *p = optbuf, *q = tmpopts, *r;
	register int len;

	/*
	 * set default mount opts
	 */
	args.flags = args.gfs_flags = 0;
	args.flags |= NFSMNT_INT;
	args.pg_thresh = SIXTYFOUR;
	port = 0;
	retry = 0;
	bg = 0;

	strcpy(p, mnt->mnt_opts);
	if (*p == NULL) return (0);
	/*
	 * go through the string keeping only characters in the
	 * range a-z 0-9 = or , and eliminating double commas and double =
	 */
	r = p + strlen(p);
	while (p < r && q < (tmpopts + MNTMAXSTR - 1)) {
		if ((*p >= 'a' && *p <= 'z') || *p == ',' || *p == '=' ||
			(*p >= '0' && *p <= '9')) {
			if ((*p == ',' || *p == '=') &&
				*(q-1) != ',' && *(q-1) != '=') *q++ = *p;
			else if (*p != ',' && *p != '=') *q++ = *p;
		}
		p++;
	}
	*q = NULL;
	p = tmpopts;
	r = tmpopts + strlen(tmpopts);
	strcpy(mnt->mnt_opts, tmpopts);

top:
	for (q=p; *q != ',' && q < r; q++);
	*q = NULL;
	q++;
	switch (*p) {
	case 'b':	/* bg */
		if (strcmp(p, MNTOPT_BG))
			opterr(p);
		bg = 1;
		break;
	case 'f':	/* force */
		if (strcmp(p, MNTOPT_FORCE))
			opterr(p);
		args.gfs_flags |= M_FORCE;
		break;
	case 'h':	/* hard */
		if (strcmp(p, MNTOPT_HARD))
			opterr(p);
		args.flags &= ~NFSMNT_SOFT;
		break;
	case 'i':	/* intr */
		if (strcmp(p, MNTOPT_INT))
			opterr(p);
		args.flags |= NFSMNT_INT;
		break;
	case 'n':
		if (*(p+1) != 'o')
			opterr(p);
		switch(*(p+2)) {
		case 'e':	/* noexec */
			if (strcmp(p, MNTOPT_NOEXEC))
				opterr(p);
			args.gfs_flags |= M_NOEXEC;
			break;
		case 's':	/* nosuid */
			if (strcmp(p, MNTOPT_NOSUID))
				opterr(p);
			args.gfs_flags |= M_NOSUID;
			break;
		case 'd':	/* nodev */
			if (strcmp(p, MNTOPT_NODEV))
				opterr(p);
			args.gfs_flags |= M_NODEV;
			break;
		case 'c':	/* nocache */
			if (strcmp(p, MNTOPT_NOCACHE))
				opterr(p);
			args.gfs_flags |= M_NOCACHE;
			break;
		default:
			opterr(p);
		}
		break;
	case 'p':
		switch(*(p+1)) {
		case 'g':	/* pgthresh= */
			len = strlen(MNTOPT_PGTHRESH);
			if (strncmp(p, MNTOPT_PGTHRESH, len))
				opterr(p);
			args.pg_thresh = max(atoi(&p[len]),MINPGTHRESH/PGUNITS);
			args.flags |= NFSMNT_PGTHRESH;
			break;
		case 'o':	/* port= */
			len = strlen(MNTOPT_PORT);
			if (strncmp(p, MNTOPT_PORT, len))
				opterr(p);
			port = atoi(&p[len]);
			break;
		default:
			opterr(p);
		}
		break;
	case 'r':
		switch(*(p+1)) {
		case 'o':	/* ro */
			if (strcmp(p, MNTOPT_RO))
				opterr(p);
			args.flags |= NFSMNT_RONLY;
			args.gfs_flags |= M_RONLY;
			break;
		case 'w':	/* rw */
			if (strcmp(p, MNTOPT_RW))
				opterr(p);
			args.flags &= ~NFSMNT_RONLY;
			args.gfs_flags &= ~M_RONLY;
			break;
		case 's':	/* rsize= */
			len = strlen(MNTOPT_RSIZE);
			if (strncmp(p, MNTOPT_RSIZE, len))
				opterr(p);
			args.rsize = atoi(&p[len]);
			if (args.rsize < MINRSIZE || args.rsize > MAXRSIZE)
				opterr(p);
			args.flags |= NFSMNT_RSIZE;
			break;
		case 'e':
			if (*(p+2) != 't' || *(p+3) != 'r')
				opterr(p);
			switch (*(p+4)) {
			case 'a':	/* retrans= */
				len = strlen(MNTOPT_RETRANS);
				if (strncmp(p, MNTOPT_RETRANS, len))
					opterr(p);
				args.retrans = atoi(&p[len]);
				if (args.retrans < MINRETRANS ||
					args.retrans > MAXRETRANS)
					opterr(p);
				args.flags |= NFSMNT_RETRANS;
				break;
			case 'y':	/* retry= */
				len = strlen(MNTOPT_RETRY);
				if (strncmp(p, MNTOPT_RETRY, len))
					opterr(p);
				retry = atoi(&p[len]);
				if (retry < MINRETRY || retry > MAXRETRY)
					opterr(p);
				break;
			default:
				opterr (p);
			}
			break;
		default:
			opterr(p);
		}
		break;
	case 's':
		switch (*(p+1)) {
		case 'o':	/* soft */
			if (strcmp(p, MNTOPT_SOFT))
				opterr(p);
			args.flags |= NFSMNT_SOFT;
			break;
		case 'y':	/* sync */
			if (strcmp(p, MNTOPT_SYNC))
				opterr(p);
			args.gfs_flags |= M_SYNC;
			break;
		default:
			opterr(p);
		}
		break;
	case 't':	/* timeo= */
		len = strlen(MNTOPT_TIMEO);
		if (strncmp(p, MNTOPT_TIMEO, len))
			opterr(p);
		args.timeo = atoi(&p[len]);
		if (args.timeo < MINTIMEO || args.timeo > MAXTIMEO)
			opterr(p);
		args.flags |= NFSMNT_TIMEO;
		break;
	case 'w':	/* wsize= */
		len = strlen(MNTOPT_WSIZE);
		if (strncmp(p, MNTOPT_WSIZE, len))
			opterr(p);
		args.wsize = atoi(&p[len]);
		if (args.wsize < MINWSIZE || args.timeo > MAXWSIZE)
			opterr(p);
		args.flags |= NFSMNT_WSIZE;
		break;
	default:
		opterr(p);
	}

	if (q < r) {
		p = q;
		goto top;
	}

	return(0);
}

opterr(opt)
char *opt;
{
	(void) fprintf(stderr,
		"nfs_mount: invalid -o option \"%s\"\n", opt);
	exit(1);
}
