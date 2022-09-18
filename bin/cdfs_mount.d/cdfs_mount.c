#ifndef lint
static	char	*sccsid = "@(#)cdfs_mount.c	4.2	(ULTRIX)	11/9/90";
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
/************************************************************************
 *			Modification History
 *
 * 9-Nov-90 -- prs
 *	Initial creation - cdfs_mount.c
 *
 ************************************************************************/

/*
 * cdfs_mount
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/fs_types.h>
#include <cdfs/cdfs_mount.h>
#include <sys/mount.h>
#include <ctype.h>
#include <stdio.h>

#define max(a,b)	((a) > (b) ? (a) : (b))

#define	MNTOPT_RO		"ro"		/* read only */
#define	MNTOPT_DEFPERM		"defperm"	/* default permissions */
#define MNTOPT_NODEFPERM	"nodefperm"	/* no default permissions */
#define MNTOPT_NOVERSION	"noversion"	/* No file verison numbers */
#define MNTOPT_PRIMARY		"primary"	/* Primary Vol Desc */
#define MNTOPT_PGTHRESH 	"pgthresh="	/* paging threshold */

#define PGUNITS		1024	/* to convert MINPGTHRESH to K */
#define PTHRESH		128	/* default page threshhold */

struct	mntent{
	char	*mnt_fsname;		/* name of mounted file system */
	char	*mnt_dir;		/* file system path prefix */
	char	*mnt_opts;		/* MNTOPT* */
};

char * hasmntopt();

int	ro = 1;
int	verbose = 0;
int	printed = 0;
struct iso_specific spec;

extern int errno;

#define MNTMAXSTR 256
char	*index(), *rindex();
char	host[MNTMAXSTR];
char	name[MNTMAXSTR];
char	dir[MNTMAXSTR];
char	opts[MNTMAXSTR];
char	tmpopts[MNTMAXSTR];



main(argc, argv)
int argc;
char **argv;
{
	struct mntent mnt;
	char *options = NULL;
	char *colon;

	if (argc == 1) {
		usage();
		exit(0);
	}

	/*
	 * Set options
	 */
	opts[0] = '\0';
	while (argc > 1 && argv[1][0] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'o':
				if (argc < 3) {
					usage();
				}
				if (strlen(argv[2]) >= MNTMAXSTR) {
					(void) fprintf(stderr, "cdfs_mount: -o string too long\n");
					exit(1);
				}
				strcpy(opts, argv[2]);
				argv++;
				argc--;
				break;
			case 'p':
				if(argc != 2)
				{
					usage();
				}
				prmount();
				exit(0);
			case 'v':
				verbose++;
				break;
			case 'r':
				break;
			default:
				(void) fprintf(stderr, "cdfs_mount: unknown option: %c\n",
				    *options);
				usage();
			}
			options++;
		}
		argc--, argv++;
	}

	if (argc != 3) {
		usage();
	}

	if (strlen(argv[2]) >= MNTMAXSTR) {
		(void) fprintf(stderr, "cdfs_mount: otions list too long\n");
		exit(1);
	}
	strcpy(dir, argv[2]);

	if (strlen(argv[1]) >= MNTMAXSTR) {
		(void) fprintf(stderr, "cdfs_mount: file system name too long\n");
		exit(1);
	}
	strcpy(name, argv[1]);

	if (dir[0] != '/') {
		(void) fprintf(stderr, "cdfs_mount: invalid directory name \"%s\";\n", dir);
		(void) fprintf(stderr, "\tdirectory pathname must begin with '/'.\n");
		exit(1);
	}

	mnt.mnt_fsname = name;
	mnt.mnt_dir = dir;
	mnt.mnt_opts = opts;
	getflags(&mnt);

	mountloop(&mnt);

}
prmount()
{
	struct fs_data fsdata;
	char *name;
	while (fread((char *)&fsdata,sizeof(struct fs_data),1,stdin)==1) {
		(void) printf("%s on %s type %s", fsdata.fd_devname,
		fsdata.fd_path, gt_names[fsdata.fd_fstype]);
		if (fsdata.fd_flags & M_DEFPERM)
			(void) printf("\t(defperm");
		if (fsdata.fd_flags & M_NODEFPERM)
			(void) printf("\t(nodefperm");
		if (fsdata.fd_flags & M_NOVERSION)
			(void) printf(",noversion");
		if (fsdata.fd_flags & M_PRIMARY)
			(void) printf(",primary");
		if (fsdata.fd_flags & M_RONLY)
			(void) printf(",ro)\n");
	}
	return(0);
}

mountloop(mnt)
register struct mntent *mnt;
{
	int error;
	int flags = 0;
	char *optp, *optend;

	flags = 1;
	error = mount(mnt->mnt_fsname, mnt->mnt_dir, flags, GT_CDFS, &spec);
	if (error) {
		(void) fprintf(stderr,
			       "cdfs_mount: cannot mount %s on ", 
			       mnt->mnt_fsname);
		perror(mnt->mnt_dir);
		return (errno);
	}

	if (verbose)
		(void) fprintf(stdout, "%s mounted on %s\n",
		    mnt->mnt_fsname, mnt->mnt_dir);

	return (0);
}

usage()
{
	(void) fprintf(stderr,
	"Usage: mount -t iso [-v] [-o option,...] device dir\n");
	exit(1);
}

getflags(mnt)
struct mntent *mnt;
{

	char optbuf[MNTMAXSTR];
	register char *p = optbuf, *q = tmpopts, *r;
	register int len; 
	char *tmp;

	/*
	 * set default mount opts
	 */
	spec.iso_flags = 0;
	spec.iso_flags |= M_DEFPERM;
	spec.iso_pgthresh = PTHRESH;

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
	case 'd':	/* defperm */
		if (strcmp(p, MNTOPT_DEFPERM))
			opterr(p);
		spec.iso_flags |= M_DEFPERM;
		break;
	case 'n':	/* nodefperm */
		if (!strcmp(p, MNTOPT_NODEFPERM)) {
			spec.iso_flags &= ~M_DEFPERM;
			spec.iso_flags |= M_NODEFPERM;
			/* no version number */
		} else if (!strcmp(p, MNTOPT_NOVERSION))
			spec.iso_flags |= M_NOVERSION;
		else
			opterr(p);
		break;
	case 'p':
		switch(*(p+1)) {
		case 'g':	/* pgthresh= */
			len = strlen(MNTOPT_PGTHRESH);
			if (strncmp(p, MNTOPT_PGTHRESH, len))
				opterr(p);
			spec.iso_pgthresh = max(atoi(&p[len]),MINPGTHRESH/PGUNITS);
			break;
		case 'r':	/* Primary volume descriptor */
/*			spec.iso_flags |= M_PRIMARY; */
			break;
		default:
			opterr(p);
		}
		break;
	case 'r':
		break;
	default:
		opterr(p);
	}
	if (q < r) {
		p = q;
		goto top;
	}
}

opterr(opt)
char *opt;
{
	(void) fprintf(stderr,
		"cdfs_mount: invalid -o option \"%s\"\n", opt);
	exit(1);
}
