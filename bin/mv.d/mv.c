#ifndef lint
static char *sccsid = "@(#)mv.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * mv file1 file2
 */

/*
 *		Modification History					*
 *									*
 * 1) - 15 Mar 85 -- funding						*
 *	Added named pipe support (re. System V named pipes)		*
 * 2) - 18 Aug 86  -- Aya Konishi (konishi@gully)
 *      Fix for SMU-00301. error message corrected. 
 * 3) - 08 Jun 88 - chetal (Pradeep Chetal)
 *	Fixed interpretation of errno from rename system call.
 * 4) - 03 May 89 - chetal (Pradeep Chetal)
 *	No longer runs cp(1) to copy file; does it itself for optimization
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <stdio.h>
#include <sys/dir.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#define	DELIM	'/'
#define MODEBITS 07777

#define	ISDIR(st)	(((st).st_mode&S_IFMT) == S_IFDIR)
#define	ISLNK(st)	(((st).st_mode&S_IFMT) == S_IFLNK)
#define	ISREG(st)	(((st).st_mode&S_IFMT) == S_IFREG)
#define	ISDEV(st) \
	(((st).st_mode&S_IFMT) == S_IFCHR  || \
		((st).st_mode&S_IFMT) == S_IFBLK  || \
		((st).st_mode&S_IFMT) == S_IFPORT)		/* 1 */

char	*dname();
char    *rindex();
struct	stat s1, s2;
int	iflag = 0;	/* interactive mode */
int	fflag = 0;	/* force overwriting */

main(argc, argv)
	register char *argv[];
{
	register i, r;
	register char *arg;
	char *dest;

	if (argc < 2)
		goto usage;
	while (argc > 1 && *argv[1] == '-') {
		argc--;
		arg = *++argv;

		/*
		 * all files following a null option
		 * are considered file names
		 */
		if (*(arg+1) == '\0')
			break;
		while (*++arg != '\0') switch (*arg) {

		case 'i':
			iflag++;
			break;

		case 'f':
			fflag++;
			break;

		default:
			goto usage;
		}
	}
	if (argc < 3)
		goto usage;
	dest = argv[argc-1];
	if (stat(dest, &s2) >= 0 && ISDIR(s2)) {
		r = 0;
		for (i = 1; i < argc-1; i++)
			r |= movewithshortname(argv[i], dest);
		exit(r);
	}
	if (argc > 3)
		goto usage;
	r = move(argv[1], argv[2]);
	exit(r);
	/*NOTREACHED*/
usage:
	fprintf(stderr,
"usage: mv [-if] f1 f2 or mv [-if] f1 ... fn d1 (`fn' is a file or directory)\n");
	return (1);
}

movewithshortname(src, dest)
	char *src, *dest;
{
	register char *shortname;
	char target[MAXPATHLEN + 1];

	shortname = dname(src);
	if (strlen(dest) + strlen(shortname) > MAXPATHLEN - 1) {
		error("%s/%s: pathname too long", dest,
			shortname);
		return (1);
	}
	(void) sprintf(target, "%s/%s", dest, shortname);
	return (move(src, target));
}

move(source, target)
	char *source, *target;
{
	int targetexists;
	char *parent();		/* returns parent dir of the argument */
	int oerrno;

	if (lstat(source, &s1) < 0) {
		error("cannot access %s", source);
		return (1);
	}
	/*
	 * First, try to rename source to destination.
	 * The only reason we continue on failure is if
	 * the move is on a nondirectory and not across
	 * file systems.
	 */
	targetexists = lstat(target, &s2) >= 0;
	if (targetexists) {
		if (iflag && !fflag && query("remove %s? ", target) == 0)
			return (1);
		if (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino) {
			error("%s and %s are identical", source, target);
			return (1);
		}
		if (access(target, 2) < 0 && !fflag && isatty(fileno(stdin))) {
			if (query("override protection %o for %s? ",
			  s2.st_mode & MODEBITS, target) == 0)
				return (1);
		}
	}
	if (rename(source, target) >= 0)
		return (0);
	oerrno = errno;
	if (errno != EXDEV) {
	  if (errno == EPERM || errno == EINVAL ||
	      (errno == EACCES && access(parent(source),W_OK))){ /* Can't remove from parent dir */
		errno = oerrno;
	        Perror2(source, "rename");        /* source problem: Fix 2,3 */
	  }
	  else {
		errno = oerrno;
		Perror2(target, "rename");
	  }
	return (1);
	}
	if (ISDIR(s1)) {
		error("can't mv directories across file systems");
		return (1);
	}
	if (targetexists && unlink(target) < 0) {
		error("cannot unlink %s", target);
		return (1);
	}
	/*
	 * File can't be renamed, try to recreate the symbolic
	 * link or special device, or copy the file wholesale
	 * between file systems.
	 */
	if (ISLNK(s1)) {
		register m;
		char symln[MAXPATHLEN];
		int len;

		if ((len = readlink(source, symln, sizeof (symln))) < 0) {
			Perror(source);
			return (1);
		}
		symln[len] = '\0';
		m = umask(~(s1.st_mode & MODEBITS));
		if (symlink(symln, target) < 0) {
			Perror(target);
			return (1);
		}
		(void) umask(m);
		goto cleanup;
	}
	if (ISDEV(s1)) {
		struct timeval tv[2];

		if (mknod(target, s1.st_mode, s1.st_rdev) < 0) {
			Perror(target);
			return (1);
		}
		tv[0].tv_sec = s1.st_atime;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = s1.st_mtime;
		tv[1].tv_usec = 0;
		(void) utimes(target, tv);
		goto cleanup;
	}
	if (ISREG(s1)) {
		register int fi, fo, n;
		struct timeval tv[2];
		char buf[MAXBSIZE];

		fi = open(source, 0);
		if (fi < 0) {
			Perror(source);
			return (1);
		}

		fo = creat(target, s1.st_mode & MODEBITS);
		if (fo < 0) {
			Perror(target);
			close(fi);
			return (1);
		}

		for (;;) {
			n = read(fi, buf, sizeof buf);
			if (n == 0) {
				break;
			} else if (n < 0) {
				Perror2(source, "read");
				close(fi);
				close(fo);
				return (1);
			} else if (write(fo, buf, n) != n) {
				Perror2(target, "write");
				close(fi);
				close(fo);
				return (1);
			}
		}

		close(fi);
		close(fo);

		tv[0].tv_sec = s1.st_atime;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = s1.st_mtime;
		tv[1].tv_usec = 0;
		(void) utimes(target, tv);
		goto cleanup;
	}
	error("%s: unknown file type %o", source, s1.st_mode);
	return (1);

cleanup:
	if (unlink(source) < 0) {
		error("cannot unlink %s", source);
		return (1);
	}
	return (0);
}

/*VARARGS*/
query(prompt, a1, a2)
	char *a1;
{
	register char i, c;

	fprintf(stderr, prompt, a1, a2);
	i = c = getchar();
	while (c != '\n' && c != EOF)
		c = getchar();
	return (i == 'y');
}

char *
dname(name)
	register char *name;
{
	register char *p;

	p = name;
	while (*p)
		if (*p++ == DELIM && *p)
			name = p;
	return name;
}

	
char *
parent(source)
register char *source;
{
	register char *p = (char *)malloc(strlen(source)+1);
	char *temp;

	strcpy(p,source);
	if ( (temp = rindex(p,DELIM)) != (char *)0 ){
		if ( *(temp+1) == '\0' ){ /* it ends with a DELIM */
			while ( (temp > p) && (*--temp != DELIM)  )
				;
			if ( *temp == DELIM && temp == p ) 
				*++temp = '\0';   /* e.g. "/a/"  */
			else if ( *temp == DELIM )
				*temp = '\0';	  /* e.g. "/a/b/" */
			else
				strcpy(p,".");    /* e.g. "a/" */
			return p;
		}
		*temp = '\0';
		return(p);
	}
	strcpy(p,".");	/* "." is dir for a file in current dir */
	return(p);	 
}

/*VARARGS*/
error(fmt, a1, a2)
	char *fmt;
{

	fprintf(stderr, "mv: ");
	fprintf(stderr, fmt, a1, a2);
	fprintf(stderr, "\n");
}

Perror(s)
	char *s;
{
	char buf[MAXPATHLEN + 10];
	
	(void) sprintf(buf, "mv: %s", s);
	perror(buf);
}

Perror2(s1, s2)
	char *s1, *s2;
{
	char buf[MAXPATHLEN + 20];

	(void) sprintf(buf, "mv: %s: %s", s1, s2);
	perror(buf);
}
