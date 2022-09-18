/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#ifndef lint
static char Sccsid[] = "@(#)subr.c	4.1	(ULTRIX)	7/17/90";
#endif

#include "ic.h"
#include <sys/types.h>
#include <sys/stat.h>

extern FILE *popen();

/*
 * strsave() -- save a string for future use.
 */
char *
strsave(s)
char *s;
{
	char *cp;

	if ((cp = new(char, strlen(s) + 1)) == (char *)0)
		fatal("no room for string");

	return strcpy(cp, s);
}

/*
 * cpp() -- preprocess input through C preprocessor.
 */

#ifndef CPP
#define CPP "/lib/cpp"
#endif

int
cpp(argv)
char **argv;
{
	char **argp;
	char *cmd;
	char *filename = (char *)0;
	extern FILE *yyin;	/* file yylex reads from */
	int i;
	char tmpfile[I_NAML];

	/* collect arguments */
	for (i = 0, argp = argv; *++argp; /*EMPTY*/)
		if (**argp == '-')
		{
			if (index("CDEIUP", (*argp)[1]) != (char *)0)
				i += strlen(*argp) + 1;
		}
		else
		{
			filename = *argp;
			i += strlen(*argp) + 1;
		}

	/* get room for argument string: */
#ifndef PIPE
	if ((cmd = new(char, (i + sizeof(CPP) + 2 + I_NAML))) == (char *)0)
#else
	if ((cmd = new(char, (i + sizeof(CPP)))) == (char *)0)
#endif
		return -1;

	/* build command string */
	strcpy(cmd, CPP);

	for (argp = argv; *++argp; /*EMPTY*/)
		if (**argp == '-' && index("CDEIUP", (*argp)[1]) != (char *)0)
			strcat(cmd, " "), strcat(cmd, *argp);

	/*
	 * filename can be null if we analyse stdin
	 */
	if (filename != (char *)0)
	{
		strcat(cmd, " ");
		strcat(cmd, filename);
	}

#ifdef PIPE
	if (yyin = popen(cmd, "r"))
		i = 0;
	else
		i = -1;
#else
	if ((yyin = tmp_make(tmpfile)) == (FILE *)0)
		fatal("cannot create temp file");

	strcat(cmd, " >");
	strcat(cmd, tmpfile);
	if (system(cmd) != 0)
		i = -1;
	else
		i = 0;
	unlink(tmpfile);
#endif

	free(cmd);
	return i;
}

zero(what, len)
register char *what;
register int len;
{
	while (--len >= 0)
		*what++ = '\0';
}

long
fillen(name)
char *name;
{
	struct stat sbuf;

	if (stat(name, &sbuf) != 0)
		return 0L;
	else
		return sbuf.st_size;
}
