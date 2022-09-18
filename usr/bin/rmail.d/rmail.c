#ifndef lint
static	char	*sccsid = "@(#)rmail.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * rmail.c
 *
 *	static char sccsid[] =	"@(#)rmail.c	4.4 (Berkeley) 8/11/83";
 *
 *	19-Apr-84	mah. Increase the size of ufrom.
 *
 */

/*
**  RMAIL -- UUCP mail server.
**
**	This program reads the >From ... remote from ... lines that
**	UUCP is so fond of and turns them into something reasonable.
**	It calls sendmail giving it a -f option built from these
**	lines.
*/

# include <stdio.h>
# include <sysexits.h>

typedef char	bool;
#define TRUE	1
#define FALSE	0

extern FILE	*popen();
extern char	*index();
extern char	*rindex();
char *strfind();

bool	Debug;

# define MAILER	"/usr/lib/sendmail"

main(argc, argv)
	char **argv;
{
	FILE *out;	/* output to sendmail */
	char lbuf[512];	/* one line of the message */
	char from[512];	/* accumulated path of sender */
	char ufrom[512];	/* user on remote system */
	char sys[64];	/* a system in path */
	char junk[512];	/* scratchpad */
	char cmd[2000];
	register char *cp;
	register char *uf;	/* ptr into ufrom */
	int i;

# ifdef DEBUG
	if (argc > 1 && strcmp(argv[1], "-T") == 0)
	{
		Debug = TRUE;
		argc--;
		argv++;
	}
# endif DEBUG

	if (argc < 2)
	{
		fprintf(stderr, "Usage: rmail user ...\n");
		exit(EX_USAGE);
	}
						/*set defaults for from and
						 *ufrom.
						 */
	(void) strcpy(from, "");
	(void) strcpy(ufrom, "/dev/null");

	for (;;)
	{
nextline:
		(void) fgets(lbuf, sizeof lbuf, stdin);

		if (strncmp(lbuf, "From ", 5) != 0 && strncmp(lbuf, ">From ", 6) != 0)
			break;			/*if not a From or >From line
						 *exit loop.
						 */
	/* Check for lines containing a
	 * "forwarded by " string,
 	 * if found then ignore the line.
	 * These lines appear in mail received
	 * from system V machines where the
	 * mail has been forwarded from one
	 * user to another.
	 */
	cp = lbuf;
	if (strfind(cp, "forwarded by "))
		goto nextline;

		(void) sscanf(lbuf, "%s %s", junk, ufrom);
		uf = ufrom;
		for (;;)
		{
			cp = index(cp+1, 'r');	/*go to next lowercase r*/
			if (cp == NULL)		/*if end of string then...*/
			{			/*initialize p to point to last
						 *occurance of !.
						 */
				register char *p = rindex(uf, '!');

				if (p != NULL)	/*if a ! found then...*/
				{
					*p = '\0';	/*terminate ufrom str*/
							/*copy into system the
							 *path.
							 */
					(void) strcpy(sys, uf);
					uf = p + 1;	/*set ptr beyond*/
					break;		/*exit for loop*/
				}

				/* If all else fails, insert default
				 * name string "somewhere".
				 */

				cp = "remote from somewhere";
			}
#ifdef DEBUG
			if (Debug)
				printf("cp='%s'\n", cp);
#endif
			if (strncmp(cp, "remote from ", 12)==0)
				break;
		}

		if (cp != NULL)
						/*place "from" into sys*/
			(void) sscanf(cp, "remote from %s", sys);
		(void) strcat(from, sys);	/*append to from str*/
		(void) strcat(from, "!");	/*append ! at end of from str*/
#ifdef DEBUG
		if (Debug)
			printf("ufrom='%s', sys='%s', from now '%s'\n", uf, sys, from);
#endif
	}/*E for (;;) */

	(void) strcat(from, uf);	/*append user to end of from string*/
					/*/usr/lib/sendmail -em -f "from"*/
	(void) sprintf(cmd, "%s -em -f%s", MAILER, from);
	while (*++argv != NULL)
	{
		(void) strcat(cmd, " '");
		if (**argv == '(')
			(void) strncat(cmd, *argv + 1, strlen(*argv) - 2);
		else
			(void) strcat(cmd, *argv);
		(void) strcat(cmd, "'");
	}
#ifdef DEBUG
	if (Debug)
		printf("cmd='%s'\n", cmd);
#endif
	out = popen(cmd, "w");
	fputs(lbuf, out);
	while (fgets(lbuf, sizeof lbuf, stdin))
		fputs(lbuf, out);
	i = pclose(out);
	if ((i & 0377) != 0)
	{
		fprintf(stderr, "pclose: status 0%o\n", i);
		exit(EX_OSERR);
	}

	exit((i >> 8) & 0377);
}


/*	Find a string of text in another
 *	string of text.
 */
char * strfind(text,substr)
		char *text;
		char *substr;
{
	int	i;	/* counter for possible fits */
	int	substrlen;/* len of substr--to avoid recalculating */

substrlen = strlen(substr);

/* Loop through text until not found or match.
 */
for (i = strlen(text) - substrlen; i >= 0 &&
     strncmp(text, substr, substrlen); i--)

	text++;

/* Return NULL if not found, ptr else NULL.
 */ 
return ((i < 0 ? NULL : text));

}/*E strfind() */

