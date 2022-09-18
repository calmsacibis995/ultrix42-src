#ifndef lint
static char *sccsid = "@(#)refer1.c	4.1	ULTRIX	7/17/90";

/* Original Id: "@(#)refer1.c	4.2 (Berkeley) 8/26/85" */
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *	Modification History
 *      --------------------
 *
 *	1-Dec-88		Tim N
 *			Fixed null pointer dereference in loop looking
 *			at argv.  Also added header and modification
 *			history sections.
 ************************************************************************/

#include <signal.h>
#include "refer..c"

main(argc,argv)		/* process command-line arguments */
char *argv[];
{
	char line[BUFSIZ], *s;
	int nodeflt = 0;

	signals();
	while ((argc>1) && (argv[1][0] == '-')) {
		switch(argv[1][1]) {
		case 'e':
			endpush++; 
			break;
		case 's':
			sort++;
			endpush = 1;
			if (argv[1][2])
				keystr = argv[1]+2;
			break;
		case 'l': 
			labels++;
			s = argv[1]+2;
			nmlen = atoi(s);
			while (*s)
				if (*s++ == ',')
					break;
			dtlen = atoi(s);
			break;
		case 'k':
			keywant = (argv[1][2] ? argv[1][2] : 'L');
			labels++;
			break;
		case 'n':
			nodeflt = 1;
			break;
		case 'p':
			argc--; 
			argv++;
			*search++ = argv[1];
			if (search-rdata > NSERCH)
				err("too many -p options (%d)", NSERCH);
			break;
		case 'a':
			authrev = atoi(argv[1]+2);
			if (authrev<=0)
				authrev = 1000;
			break;
		case 'b':
			bare = (argv[1][2] == '1') ? 1 : 2;
			break;
		case 'c':
			smallcaps = argv[1]+2;
			break;
		case 'f':
			refnum = atoi(argv[1]+2) - 1;
			break;
		case 'B':
			biblio++;
			bare = 2;
			if (argv[1][2])
				convert = argv[1]+2;
			break;
		case 'S':
			science++;
			labels = 1;
			break;
		case 'P':
			postpunct++;
			break;
		}
		argc--; 
		argv++;
	}
	if (getenv("REFER") != NULL)
		*search++ = getenv("REFER");
	else if (nodeflt == 0)
		*search++ = "/usr/dict/papers/Ind";
	if (endpush) {
		sprintf(tfile, "/tmp/rj%da", getpid());
		fo = fopen(tfile, "w");
		if (fo == NULL) {
			fo = ftemp;
			fprintf(stderr, "Can't open scratch file");
		}
		sep = 002; /* separate records without confusing sort..*/
	}
	if (sort && !labels) {
		sprintf(ofile, "/tmp/rj%db", getpid());
		ftemp = fopen(ofile, "w");
		if (ftemp == NULL) {
			fprintf(stderr, "Can't open scratch file\n");
			exit(1);
		}
	}
	do {
		if (argc > 1) {
			fclose(in);
			Iline = 0;
			in = fopen(Ifile = argv[1], "r");
			argc--; 
			argv++;
			if (in == NULL) {
				err("Can't read %s", Ifile);
				continue;
			}
		}
		while (input(line)) {
			Iline++;
			if (biblio && *line == '\n')
				doref(line);
			else if (biblio && Iline == 1 && *line == '%')
				doref(line);
			else if (!prefix(".[", line))
				output(line);
			else
				doref(line);
		}
	} while (argc > 1);

	if (endpush && fo != NULL)
		dumpold();
	output("");
	if (sort && !labels)
		recopy(ofile);
	clfgrep();
	cleanup();
	exit(0);
}

extern int intr();

signals()
{
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, intr);
	signal(SIGHUP, intr);
	signal(SIGPIPE, intr);
	signal(SIGTERM, intr);
}

intr()
{
	signal(SIGINT, SIG_IGN);
	cleanup();
	exit(1);
}

cleanup()
{
	if (tfile[0])
		unlink(tfile);
	if (gfile[0])
		unlink(gfile);
	if (ofile[0])
		unlink(ofile);
	if (hidenam[0])
		unlink(hidenam);
}
