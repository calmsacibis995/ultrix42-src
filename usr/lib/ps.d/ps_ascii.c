#ifndef lint
static char *sccsid = "@(#)ps_ascii.c	4.1	ULTRIX	7/2/90";
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
/*
 *    ps_ascii.c	ASCII / PostScript filter
 *	Ensures postscript output from text file.
 *	If file begins with Postscript Magic Number, assume postscript already
 */
#include <stdio.h>			/* standard i/o definitions */
#include <sys/signal.h>

#define hdr_file "/usr/lib/ps/ascii_pro.ps"
char	*prolog[] =
{
#include "ascii_pro.h"
	0
};


int cc;
char buf[133];
int control_data;			/* flag to indicate we are processing
					 * PostScript commands, not ascii text*/

/*
 * The translation from ascii to PostScript is done in two parts:
 *	1. Functions definitions are read from a file, and output.
 *	   These functions perform start-of-job, end-of-job, and
 *	   control character operations.
 *	2. The ascii file to be printed is read, and scanned for
 *	   special characters. When one is encountered, the appropriate
 *	   PostScript function is invoked. All printable characters
 *	   are collected and output in "show" commands.
 *
 *	example:
 *		Printing the file:
 *			line 1
 *			line 2
 *		Would result in output:
 *			...PostScript function definitions...
 *			do-soj
 *			(line 1)show do-n
 *			(line 2)show do-n
 *			do-eoj
 */

/*
***	Determine if file has Postscript magic number
 */

#define POSTSCRIPT_MN "%!"
#define PS 1

#define PSBUFSIZE 25
char spbuf[PSBUFSIZE], *sp=spbuf;

is_ps()
{
	register char c, *ps=POSTSCRIPT_MN;

	while (*ps)
	{
		if ((c=getchar()) == EOF)
			exit(0);
		*sp++ = c;	/* put in buffer */
		if (c != *ps++)
		{
			*sp = NULL ;	/* null terminate buffer */
			sp = spbuf;
			return NULL ;
		}
	}
	*sp = NULL ;	/* null terminate buffer */
	sp=spbuf;
	return PS;
}

#ifdef NDEF
char
spgetchar()
{
	if (*sp) return (*sp++); /* buffered characters */
	return (getchar());
}
#else
#define spgetchar() ((*sp) ? (*sp++) : (getchar()))
#endif 

#define PS_EOF '\004'	/* PostScript End of File */

ps_end(stat)
int stat;
{
	putchar(PS_EOF);
	exit(stat);
}

main(argc, argv)
int argc;
char *argv[];
{
	register char c;

	if (is_ps()==PS)
	{
		while ((c = spgetchar()) != EOF)
			putchar(c);
		ps_end(0);
	}
	/*
	 * load the PostScript function definitions
	 */
	if (load_ps_pro() == EOF)
		ps_end(1);
	/*
	 * execute "start-of-job" function
	 */
	printf("do-soj\n");
	/*
	 * read the input file, scanning for special characters,
	 * and outputing PostScript
	 */
	while ((c = spgetchar()) != EOF)
	{
		switch (c)
		{
			case ('\n'):
				flush_buf();
				puts("do-n");
				break;
			case ('\f'):
				flush_buf();
				puts("do-f");
				break;
			case ('\r'):
				flush_buf();
				puts("do-r");
				break;
			case ('\b'):
				flush_buf();
				puts("do-b");
				break;
			case ('\t'):
				flush_buf();
				puts("do-t");
				break;
			case ('\020'):
				if (!control_data)
				{
					flush_buf();
					control_data = 1;
				} else {
					flush_buf();
					control_data = 0;
				}
				break;
			case ('\031'):
				/*
			 	 * lpd needs to use a different filter 
			 	 * to print data so kill what we are 
			 	 * doing and lpd will restart us
			 	 */
				if ((c = spgetchar()) == '\1') {
					fflush(stdout);
					fprintf(stderr, "%s: Killing output filter\n", argv[0]);
					kill(getpid(), SIGKILL);
					break;
				} else {
					ungetc(c, stdin);
					c = '\031';
				}
				break;
			case ('\\'):
			case ('('):
			case (')'):
				if (cc >= (sizeof(buf) - 2))
					flush_buf();
				buf[cc++] = '\\';
				/* fall through into default code */
			default:
				if (c < 040)
					continue;
				buf[cc++] = c;
				if (cc >= (sizeof(buf) - 1))
					flush_buf();
				break;
		}
	}
	flush_buf();
	/*
	 * execute "end-of-job" function
	 */
	printf("do-eoj\n");
	ps_end(0);
}

/*
 *		l o a d _ p s _ f u n c s
 *
 * This routine is called to output PostScript function definitions
 * which are invoked by the translator. The function definitions ard
 * read from a file, which can be be provided by an 
 * environment variable "PS_ASCII_PRO", or the default file is compiled
 * via the ascii_pro.h include file, generated from ascii_pro.ps .
 *
 * Returns:		NULL on SUCCESS,
 *			EOF on error
 *
 * Inputs:		none
 */
load_ps_pro()
{
	register char c;
	char *cp, *getenv();

	FILE *file;

	if ((cp = getenv("PS_ASCII_PRO")) == NULL)
		ppss(prolog);		/* Copy fixed part of prolog   */
	else
	{
		if ((file = fopen(cp, "r")) == NULL)
		{
			perror("ps_ascii function definition file open error");
			return(EOF);
		}
		while ((c = getc(file)) != EOF)
		{
			putchar(c);
		}
		fclose(file);
	}
	return(NULL);
}

/************************************************************************/
/*	Put PostScript Sfuff						*/
/************************************************************************/
ppss(stuff)
	char	**stuff;
{
	char	*p;

	while(p = *stuff++)
		fprintf(stdout,"%s\n", p);
}

/*
 *		f l u s h _ b u f
 *
 * Flushes the output buffer to the printer (if it is not empty).
 */
flush_buf()
{
	if (cc != 0)
	{
		buf[cc] = NULL;
		cc = 0;
		if (!control_data)
			printf("(%s)show ", buf);
		else
			printf("%s\n", buf);
	}
}
