#ifndef lint
static char Sccsid[] = "@(#)yyerror.c	4.1 (ULTRIX) 7/17/90";
#endif

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

/*
 * yyerror() -- [detailed] error message for yyparse()
 */

/* 
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 001	David Lindner Tue Dec 19 10:43:53 EST 1989
 * 	- Changed the yac error file pointer to stderr
 *
 */

#include <stdio.h>

FILE *yyerfp = stderr;		/* error stream */

/*VARARGS1*/
yyerror(s, t)
register char *s;
register char *t;
{
	extern int yynerrs;	/* total number of errors */
	static int list = 0;	/* for sequential calls */

	if (s || !list)
	{	/* header necessary */
		yywhere();
		fprintf(yyerfp, "[error %2d] ", yynerrs + 1);
		if (s)
		{	/* simple message */
			fputs(s, yyerfp);
			putc('\n', yyerfp);
			return;
		}
		if (t)
		{	/* first token */
			fputs("expecting ", yyerfp);
			fputs(t, yyerfp);
			list = 1;
			return;
		}
		/* no tokens acceptable */
		fputs("syntax error.\n", yyerfp);
		return;
	}
	if (t)
	{	/* subsequent token */
		fputs(", ", yyerfp);
		fputs(t, yyerfp);
		return;
	}
	/* end of enhanced message */
	fputs(".\n", yyerfp);
	list = 0;
	return;
}
