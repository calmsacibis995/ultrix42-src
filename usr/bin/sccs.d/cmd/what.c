#ifndef lint
static	char	*sccsid = "@(#)what.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * 		Modification History
 *
 * 29-Aug-1989, Ken Lesniak
 *	Added -s switch for X/Open
 */

# include	"stdio.h"
# include	"sys/types.h"
# include	"macros.h"
# include	"fatal.h"



char pattern[]  =  "@(#)";
char opattern[]  =  "~|^`";
int had_s = 0;

main(argc,argv)
int argc;
register char **argv;
{
	register FILE *iop;
	register char *p;
	register int c;
	int prev_optind;
	extern int optind, opterr;
	extern char *optarg;
	extern int getopt();

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine, and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;

	/*
	The following loop processes keyletters and arguments.
	Note that these are processed only once for each
	invocation of 'main'.
	*/
	opterr = 0;
	for (;;) {
		prev_optind = optind;

		if ((c = getopt(argc, argv, "s")) == EOF)
			break;

		switch (c) {
		case 's':	/* Quit after first match */
			if (prev_optind == optind)
				fatal("value after s arg (cm8)");
			if (had_s++)
				fatal("key letter twice (cm2)");
			break;

		default:
			fatal("unknown key letter (cm1)");
		}
	}

	/*
	Now process the files.
	*/
	if (optind >= argc)
		dowhat(stdin);
	else
		do {
			if (p = argv[optind]) {
				if ((iop = fopen(p,"r")) == NULL)
					fprintf(stderr,"can't open %s (26)\n",p);
				else {
					printf("%s:\n",p);
					dowhat(iop);
				}
			}
		} while (++optind < argc);
}


dowhat(iop)
register FILE *iop;
{
	register int c;

	while ((c = getc(iop)) != EOF) {
		if (c == pattern[0]) {
			if (trypat(iop, &pattern[1]) && had_s)
				break;
		} else if (c == opattern[0]) {
			if (trypat(iop, &opattern[1]) && had_s)
				break;
		}
	}
	fclose(iop);
}


int trypat(iop,pat)
register FILE *iop;
register char *pat;
{
	register int c;
	register int match = 0;

	for (; *pat; pat++)
		if ((c = getc(iop)) != *pat)
			break;
	if (!*pat) {
		putchar('\t');
		while ((c = getc(iop)) != EOF && c && !any(c,"\"\\>\n"))
			putchar(c);
		putchar('\n');

		match++;
	}
	else if (c != EOF)
		ungetc(c, iop);

	return match;
}
