/* @(#)execvp.c	4.1	(ULTRIX)	7/3/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History				*
 *									*
 *	Mark A. Parenti, 02-Jun-1988					*
 * 0001 Remove use of "-" as a path separator as it is a legal character* 
 *	in a path name.							*
 *									*
 *	L. Scott, 21-Dec-1989        					*
 * 0002 Check for null pathname; return ENOENT				*
 *									*
 *	L. Scott, 10-Jan-1989						*
 * 0003 change use of index to strchr					*
 ************************************************************************/
/*
 *	execlp(name, arg,...,0)	(like execl, but does path search)
 *	execvp(name, argv)	(like execv, but does path search)
 *
 */
#ifdef mips
#include <varargs.h>
#endif mips
#include <errno.h>
#include <limits.h>
#define	NULL	0

static	char shell[] =	"/bin/sh";
char	*execat(), *getenv();
extern	errno;

#ifdef vax
execlp(name, argv)
char *name, *argv;
{
	return(execvp(name, &argv));
}
#endif vax
#ifdef mips
execlp(name, va_alist)
char *name;
va_dcl
{
        va_list ap;
        va_start(ap);
        return(execvp(name, ap));
}
#endif mips

execvp(name, argv)
char *name, **argv;
{
	char fname[PATH_MAX+1];
	char *newargs[256];
	register char *pathstr;
	register char *cp;
	register int i;
	register unsigned etxtbsy = 1;
	register int eacces = 0;

	if (*name == '\0') {
		errno = ENOENT;
		return(-1);
	}

	if ((pathstr = getenv("PATH")) == NULL)
		pathstr = ":/bin:/usr/bin";
	cp = strchr(name, '/')? "": pathstr;

	do {
		cp = execat(cp, name, fname);
		if(cp == (char *)-1){
			errno = ENAMETOOLONG;
			return(-1);
		}
	retry:
		execv(fname, argv);
		switch(errno) {
		case ENOEXEC:
			newargs[0] = "sh";
			newargs[1] = fname;
			for (i=1; newargs[i+1]=argv[i]; i++) {
				if (i>=254) {
					errno = E2BIG;
					return(-1);
				}
			}
			execv(shell, newargs);
			return(-1);
		case ETXTBSY:
			if (++etxtbsy > 5)
				return(-1);
			sleep(etxtbsy);
			goto retry;
		case EACCES:
			eacces++;
			break;
		case ENOMEM:
		case E2BIG:
			return(-1);
		}
	} while (cp);
	if (eacces)
		errno = EACCES;
	return(-1);
}

static char *
execat(s1, s2, si)
register char *s1, *s2;
char *si;
{
	register char *s;
	register int count=0;

	s = si;
	while (*s1 && *s1 != ':'){
		if(count ++ > PATH_MAX)
			return((char *)-1);
		*s++ = *s1++;
	}
	if (si != s){
		if(count++ > PATH_MAX)
			return((char *) -1);
		*s++ = '/';
	}
	while (*s2){
		if(count++ > PATH_MAX)
			return((char *) -1);
		*s++ = *s2++;
	}
	*s = '\0';
	return(*s1? ++s1: 0);
}
