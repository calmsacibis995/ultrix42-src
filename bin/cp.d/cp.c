#ifndef lint
static char sccsid[] = "@(#)cp.c	4.2	(ultrix)	12/6/90";
#endif
  
/*
 *			Copyright (c) 1985 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	This software is  derived  from  software  received  from  the
 *	University    of   California,   Berkeley,   and   from   Bell
 *	Laboratories.  Use, duplication, or disclosure is  subject  to
 *	restrictions  under  license  agreements  with  University  of
 *	California and with AT&T.					
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
*/

/*
 * cp
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/time.h>

int	iflag;	/* -i interrogate user on each file */
int	rflag;	/* -r or -R recurse */
int	pflag;	/* -p preserve modes on copy if possible */
int	fflag;	/* -f force; rm dest file before cp, still show err msgs */
char	*rindex();

main(argc, argv)
	int argc;
	char **argv;
{
	struct stat stb;
	int rc, i;

	argc--, argv++;
	while (argc > 0 && **argv == '-') {
		(*argv)++;
		while (**argv) switch (*(*argv)++) {

		case 'i':
			iflag++; break;

		case 'f':	/* force destination file to be removed */
			fflag++; break;

		case 'R':	/* allow R as in "ls -R" */
		case 'r':
			rflag++; break;

		case 'p':	/* preserve mtimes, atimes, and modes */
			pflag++;
			(void) umask(0);
			break;

		default:
			goto usage;
		}
		argc--; argv++;
	}
	if (argc < 2) 
		goto usage;
	if (argc > 2) {
		if (stat(argv[argc-1], &stb) < 0)
			goto usage;
		if ((stb.st_mode&S_IFMT) != S_IFDIR) 
			goto usage;
	}
	rc = 0;
	for (i = 0; i < argc-1; i++)
		rc |= copy(argv[i], argv[argc-1]);
	exit(rc);
usage:
	fprintf(stderr,
	    "Usage: cp [-ifp] f1 f2; or: cp [-ifpr] f1 ... fn d2\n");
	exit(1);
}

			/* I/O buffer; guarantee long-word alignment */
static char	buf[MAXBSIZE];

copy(from, to)
	char *from, *to;
{
	int fold, fnew, n, exists;
	char *last, destname[MAXPATHLEN + 1];
	struct stat stfrom, stto;

	fold = open(from, 0);
	if (fold < 0) {
		Perror(from);
		return (1);
	}
	if (fstat(fold, &stfrom) < 0) {
		Perror(from);
		(void) close(fold);
		return (1);
	}
	if (stat(to, &stto) >= 0 &&
	   (stto.st_mode&S_IFMT) == S_IFDIR) {
		last = rindex(from, '/');
		if (last) last++; else last = from;
		if (strlen(to) + strlen(last) >= sizeof destname - 1) {
			fprintf(stderr, "cp: %s/%s: Name too long", to, last);
			(void) close(fold);
			return(1);
		}
		(void) sprintf(destname, "%s/%s", to, last);
		to = destname;
	}


	if (rflag && (stfrom.st_mode&S_IFMT) == S_IFDIR) {
		int fixmode = 0;	/* cleanup mode after rcopy */

		if (stfrom.st_dev == stto.st_dev &&
		   stfrom.st_ino == stto.st_ino) {
			fprintf(stderr,
				"cp: source and destination directories are indentical (not copied)\n");
			(void) close(fold);
			return (1);
		}

		(void) close(fold);
		if (stat(to, &stto) < 0) {
			if (mkdir(to, (stfrom.st_mode & 07777) | 0700) < 0) {
				Perror(to);
				return (1);
			}
			fixmode = 1;
		} else if ((stto.st_mode&S_IFMT) != S_IFDIR) {
			fprintf(stderr, "cp: %s: Not a directory.\n", to);
			return (1);
		} else if (pflag)
			fixmode = 1;
		n = rcopy(from, to);
		if (fixmode)
			(void) chmod(to, stfrom.st_mode & 07777);
		return (n);
	}

	if ((stfrom.st_mode&S_IFMT) == S_IFDIR) {  /* no rflag */

		if (stfrom.st_dev == stto.st_dev &&
		   stfrom.st_ino == stto.st_ino) {
			fprintf(stderr,
				"cp: source and destination directories are identical (not copied)\n");
			(void) close(fold);
			return (1);
		}

		fprintf(stderr,
			"cp: %s is a directory and -r not specified.\n", from);
		return (1);
	}

	exists = stat(to, &stto) == 0;
	if (exists) {
		if (stfrom.st_dev == stto.st_dev &&
		   stfrom.st_ino == stto.st_ino) {
			fprintf(stderr,
				"cp: %s and %s are identical (not copied).\n",
					from, to);
			(void) close(fold);
			return (1);
		}
		if (fflag) {	/* force removal if possible */ 
			if (unlink(to) < 0) {
				fprintf(stderr, "cp: %s not removed\n", to);
				return (1);
			}
		}
		if (!fflag && iflag) {
			int i, c;

			fprintf (stderr, "overwrite %s? ", to);
			i = c = getchar();
			while (c != '\n' && c != EOF)
				c = getchar();
			if (i != 'y') {
				(void) close(fold);
				return(1);
			}
		}
	}
	fnew = creat(to, stfrom.st_mode & 07777);
	if (fnew < 0) {
		Perror(to);
		(void) close(fold); return(1);
	}
	if (exists && pflag)
		(void) fchmod(fnew, stfrom.st_mode & 07777);
			
	for (;;) {
		n = read(fold, buf, sizeof buf);
		if (n == 0)
			break;
		if (n < 0) {
			Perror(from);
			(void) close(fold); (void) close(fnew); return (1);
		}
		if (write(fnew, buf, n) != n) {
			Perror(to);
			(void) close(fold); (void) close(fnew); return (1);
		}
	}
	(void) close(fold); (void) close(fnew); 
	if (pflag)
		return (setimes(to, &stfrom));
	return (0);
}

rcopy(from, to)
	char *from, *to;
{
	DIR *fold = opendir(from);
	struct direct *dp;
	struct stat statb;
	int errs = 0;
	char fromname[MAXPATHLEN + 1];

	if (fold == 0 || (pflag && fstat(fold->dd_fd, &statb) < 0)) {
		Perror(from);
		return (1);
	}
	for (;;) {
		dp = readdir(fold);
		if (dp == 0) {
			closedir(fold);
			if (pflag)
				return (setimes(to, &statb) + errs);
			return (errs);
		}
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(from)+1+strlen(dp->d_name) >= sizeof fromname - 1) {
			fprintf(stderr, "cp: %s/%s: Name too long.\n",
			    from, dp->d_name);
			errs++;
			continue;
		}
		(void) sprintf(fromname, "%s/%s", from, dp->d_name);
		errs += copy(fromname, to);
	}
}

int
setimes(path, statp)
	char *path;
	struct stat *statp;
{
	struct timeval tv[2];
	
	tv[0].tv_sec = statp->st_atime;
	tv[1].tv_sec = statp->st_mtime;
	tv[0].tv_usec = tv[1].tv_usec = 0;
	if (utimes(path, tv) < 0) {
		Perror(path);
		return (1);
	}
	return (0);
}

Perror(s)
	char *s;
{

	fprintf(stderr, "cp: ");
	perror(s);
}
