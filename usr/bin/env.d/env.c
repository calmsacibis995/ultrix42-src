#ifndef lint
static char *sccsid = "@(#)env.c	4.1 ULTRIX 7/17/90";
#endif
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
/*
 *	env [ - ] [ name=value ]... [command arg...]
 *	set environment, then execute command (or print environment)
 *	- says start fresh, otherwise merge with inherited environment
 */
#include <stdio.h>

#define NENV	100
char	*newenv[NENV];
char	*nullp = NULL;

extern	char **environ;
extern	errno;
extern	char *sys_errlist[];
char	*nvmatch(), *strchr();
void	exit();

main(argc, argv, envp)
register char **argv, **envp;
{

	argc--;
	argv++;
	if (argc && strcmp(*argv, "-") == 0) {
		envp = &nullp;
		argc--;
		argv++;
	}

	for (; *envp != NULL; envp++)
		if (strchr(*envp, '=') != NULL)
			addname(*envp);
	while (*argv != NULL && strchr(*argv, '=') != NULL)
		addname(*argv++);

	if (*argv == NULL)
		print(0); /* doesn't return */
	else {
		environ = newenv;
		(void) execvp(*argv, argv);
		(void) fputs(sys_errlist[errno], stderr);
		(void) fputs(": ", stderr);
		(void) fputs(*argv, stderr);
		(void) putc('\n', stderr);
		exit(1);
	}
}

addname(arg)
register char *arg;
{
	register char **p;

	for (p = newenv; *p != NULL; p++) {
		if (p >= &newenv[NENV-1]) {
			(void) fputs("too many values in environment\n", stderr);
			print(1); /* doesn't return */
		}
		if (nvmatch(arg, *p) != NULL)
			break;
	}
	*p = arg;
}

print(code)
{
	register char **p = newenv;

	while (*p != NULL)
		(void) puts(*p++);
	exit(code);
}

/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of s2, else NULL
 */

char *
nvmatch(s1, s2)
register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++ == '=')
			return(s2);
	if (*s1 == '\0' && *(s2-1) == '=')
		return(s2);
	return(NULL);
}
