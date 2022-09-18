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
static char Sccsid[] = "@(#)yywhere.c	4.1	(ULTRIX)	7/17/90";
#endif

/*
 * yywhere() -- input position for yyparse()
 */

#include <stdio.h>

extern FILE *yyerfp;		/* error stream */
extern char yytext[];		/* current token */
extern int yyleng;		/* and its length */
extern int yylineno;		/* current input line number */

static char *source;		/* name of current input file */

yywhere()
{
	if (source && *source && strcmp(source,"\"\"") != 0)
	{
		char *cp = source;
		int len = strlen(source);

		if (*cp == '"')			/* strip "'s */
			++cp, len -= 2;
		if (strncmp(cp, "./", 2) == 0)	/* strip leading ./ */
			cp +=2, len -= 2;
		fprintf(yyerfp, "%.*s", len, cp);
	}
	else
		fprintf(yyerfp, "[stdin]");

	if (yylineno > 0)
		fprintf(yyerfp, ", line %d", yylineno - (*yytext == '\n' || !*yytext));
	if (*yytext)
	{
		register int i;

		for (i = 0; i < 20; i++)
			if (!yytext[i] || yytext[i] == '\n')
				break;
		if (i)
			fprintf(yyerfp, ": near \"%.*s\": ", i, yytext);
	}
}

/*
 * yymark() -- get information from '# line file' lines.
 */

yymark()
{
	extern char *calloc();
	char *savesrc = source;

	/*
	 * get current line number
	 */
	sscanf(yytext, "# %d", &yylineno);

	source = calloc(yyleng, sizeof(char));

	if (source)
	{
		sscanf(yytext, "# %*d %s", source);

		if (*source)
		{
			if (savesrc != (char *)0)
				free(savesrc);
		}
		else
		{
			free(source);
			source = savesrc;
		}
	}
	else
	{
		warning("source name in messages may be incorrect");
		source = savesrc;
	}
}
