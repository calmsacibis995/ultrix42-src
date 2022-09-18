#ifndef lint
static	char	*sccsid = "@(#)ufs_mount.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87 by			*
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
 *
 *			Modification History
 *
 * 15-Jan-87 -- prs
 *	Added parsing of the quota option to getflags()
 *
 ************************************************************************/

/*
 * ufs_mount
 */
#include <stdio.h>
#ifdef DEBUG
#include "param.h"
#include "mount.h"
#include "ufs_mount.h"
#else DEBUG
#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs_mount.h>
#endif DEBUG
#include <sys/fs_types.h>
#include <signal.h>
#include <strings.h>

int errflag,oflag,pflag,rflag,vflag;
char *our_command;
char *our_name;
int our_type = 0;

#define max(a,b)	((a) > (b) ? (a) : (b))
#define PGUNITS		1024	/* to convert MINPGTHRESH to K */
#define SIXTYFOUR	64	/* default page threshhold */

struct ufs_specific u_opts, *getflags();
void perror(),exit();

main(argc, argv)
	int argc;
	char **argv;
{
	char *options = NULL, *progname;
	int c;
	extern char *optarg;
	extern int optind;
	int ret;
	char *rindex();
	
#ifdef lint
	progname = gt_names[GT_ULTRIX];
#endif lint

	progname = argv[0];
	
	/* strip off the leading pathnames */
	
	if((our_command = rindex(progname, '/')) == NULL)
		our_command = progname;
	else
		our_command++;
		
	/* find the name of our file system type */
		
	for(our_type  = 0; our_type < NUM_FS; our_type++)
		if(strncmp(our_command, gt_names[our_type],
		rindex(our_command, '_') - our_command) == 0)
			break;		/* we have our name */
	if(our_type == NUM_FS)
		our_type = 0;
	our_name = gt_names[our_type];
	u_opts.ufs_flags = 0;
	u_opts.ufs_pgthresh = SIXTYFOUR;	/* default 64k for ufs */
	while ((c = getopt(argc,argv,"o:prv")) != EOF) {
		switch (c) {
		case 'o':
			oflag++;
			options = optarg;
			break;
		case 'p':
			pflag++;
			break;
		case 'r':
			rflag++;
			break;
		case 'v':
			vflag++;
			break;
		case '?':
			errflag++;
		}
	}
	/* give usage message if any bad flags are used or if there are */
	/* NOT exactly 2 arguments left for us to do the mount */
	if (errflag || (((optind + 2) != argc) && !pflag)
		    || (pflag && (oflag||rflag))) {
		(void) fprintf(stderr,
		"Usage: %s [-o options] [-v] [-r] [-p] special path\n",
			progname);
		exit(1);
	}
	if (pflag) {
		ret = prmount();
	}
	else {
		ret = mountfs(argv[optind], argv[optind+1],
				(rflag?M_RONLY:0), getflags(options));
	}
	exit(ret);
}

prmount()
{
	struct fs_data fsdata;
	while (fread((char *)&fsdata,sizeof(struct fs_data),1,stdin)==1) {
		(void) printf("%s on %s type %s", fsdata.fd_devname,
		fsdata.fd_path, our_name);
		if (fsdata.fd_flags & M_RONLY)
			(void) printf("\t(read-only)");
		if (fsdata.fd_flags & M_QUOTA)
			(void) printf("\t(quotas)");
		if (fsdata.fd_flags & M_NOEXEC)
			(void) printf("\t(noexec)");
		if (fsdata.fd_flags & M_NOSUID)
			(void) printf("\t(nosuid)");
		if (fsdata.fd_flags & M_NODEV)
			(void) printf("\t(nodev)");
		if (fsdata.fd_flags & M_FORCE)
			(void) printf("\t(force)");
		if (fsdata.fd_flags & M_SYNC)
			(void) printf("\t(sync)");
		if (fsdata.fd_flags & M_NOCACHE)
			(void) printf("\t(nocache)");
		(void) printf("\n");
	}
	return(0);
}

mountfs(spec, name, flags, u_opts)
	char *spec, *name;
	int  flags;
	struct ufs_specific *u_opts;
{
#ifdef DEBUG
	(void) printf("Mounting special %s on directory %s with flags %x\n",
		spec,name,flags);
	(void) printf("options: flags %x pgthresh %dK\n",
			u_opts->ufs_flags,u_opts->ufs_pgthresh);
#else DEBUG
	
	/*
	 * we use our_type so that differ file systems may use the same
	 * mount command just by linking them to /etc/ufs_mount
	 */

	if (mount(spec, name, flags, our_type, (char *)u_opts) < 0) {
		(void) fprintf(stderr, "%s: %s on ", our_command, spec);
		perror(name);
		return(1);
	}
#endif DEBUG
	if (vflag) {
		(void) printf("%s: mounted %s on %s\n", our_command, spec,name);
	}
	return(0);
}

struct ufs_specific *
getflags(opts)
char *opts;
{
	char newopt[BUFSIZ];
	register char *p = opts, *q = newopt, *r;
	register int len;
	if (opts == NULL || *opts == NULL) return(&u_opts);
	/*
	 * go through the string keeping only characters in the
	 * range a-z 0-9 = or , and eliminating double commas and double =
	 */
	r = opts + strlen(opts);
	while (p < r && q < (newopt + BUFSIZ - 1)) {
		if ((*p >= 'a' && *p <= 'z') || *p == ',' || *p == '=' ||
			(*p >= '0' && *p <= '9')) {
			if ((*p == ',' || *p == '=') &&
				*(q-1) != ',' && *(q-1) != '=') *q++ = *p;
			else if (*p != ',' && *p != '=') *q++ = *p;
		}
		p++;
	}
	*q = NULL;
	p = newopt;
	r = newopt + strlen(newopt);
top:
	for (q=p;*q != ',' && q < r;q++);
	*q = NULL;
	q++;
	switch (*p) {
	case 'f':	/* force */
		if (strcmp(p,MOUNT_FORCE)) {
			(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
			exit(1);
		}
		u_opts.ufs_flags |= M_FORCE;
		break;
	case 'p':	/* pgthresh= */
		len = strlen(MOUNT_PGTHRESH);
		if (strncmp(p,MOUNT_PGTHRESH,len)) {
			(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
			exit(1);
		}
		u_opts.ufs_pgthresh = max(atoi(&p[len]),MINPGTHRESH/PGUNITS);
		break;
	case 's':	/* sync */
		if (strcmp(p,MOUNT_SYNC)) {
			(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
			exit(1);
		}
		u_opts.ufs_flags |= M_SYNC;
		break;
 	case 'q':	/* quota */
 		if (strcmp(p,MOUNT_QUOTA)) {
 			(void) fprintf(stderr,
 				"Error in options \"%s\" with option \"%s\"\n",
 				opts,p);
 			exit(1);
 		}
 		u_opts.ufs_flags |= M_QUOTA;
 		break;
	case 'n':
		switch(*(p+2)) {
		case 'e':	/* noexec */
			if (strcmp(p,MOUNT_NOEXEC)) {
				(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
				exit(1);
			}
			u_opts.ufs_flags |= M_NOEXEC;
			break;
		case 's':	/* nosuid */
			if (strcmp(p,MOUNT_NOSUID)) {
				(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
				exit(1);
			}
			u_opts.ufs_flags |= M_NOSUID;
			break;
		case 'd':	/* nodev */
			if (strcmp(p,MOUNT_NODEV)) {
				(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
				exit(1);
			}
			u_opts.ufs_flags |= M_NODEV;
			break;
		case 'c':	/* nocache */
			if (strcmp(p,MOUNT_NOCACHE)) {
				(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
				exit(1);
			}
			u_opts.ufs_flags |= M_NOCACHE;
			break;
		default:
			(void) fprintf(stderr,
				"Error in options \"%s\" with option \"%s\"\n",
				opts,p);
			exit(1);
		}
		break;
	default:
		(void) fprintf(stderr,
			"Error in options \"%s\" with option \"%s\"\n",opts,p);
		exit(1);
	}
	if (q < r) {
		p = q;
		goto top;
	}
	return(&u_opts);
}
