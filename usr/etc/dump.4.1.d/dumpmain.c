#ifndef lint
static	char	*sccsid = "@(#)dumpmain.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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
 * 			Modification History				*
 *									*
 * 22 Apr 85 -- dlb							*
 *	Change declaration of dump specific rewind() function to 	*
 *	rewind_tape() to avoid conflict with declaration of rewind()	*
 *	in <stdio.h>.							*
 *									*
 ************************************************************************/

#include "dump.h"

extern void rewind_tape();

int	notify = 0;	/* notify operator flag */
int	blockswritten = 0;	/* number of blocks written on current tape */
int	tapeno = 0;	/* current tape number */
int	density = 160;	/* density in 0.1" units */

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char		*arg;
	register	i;
	float		fetapes;
	register	struct	fstab	*dt;

	time(&(spcl.c_date));

	tsize = 2300L*12L*10L;
	tape = TAPE;
	disk = DISK;
	increm = NINCREM;

	incno = '9';
	uflag = 0;
	arg = "u";
	if(argc > 1) {
		argv++;
		argc--;
		arg = *argv;
		if (*arg == '-')
			argc++;
	}
	while(*arg)
	switch (*arg++) {
	case 'w':
		lastdump('w');		/* tell us only what has to be done */
		exit(0);
		break;
	case 'W':			/* what to do */
		lastdump('W');		/* tell us the current state of what has been done */
		exit(0);		/* do nothing else */
		break;

	case 'J':			/* update old to new */
		o_nconvert();
		exit(0);		/* do nothing else */
		break;

	case 'f':			/* output file */
		if(argc > 1) {
			argv++;
			argc--;
			tape = *argv;
		}
		break;

	case 'd':			/* density, in bits per inch */
		if (argc > 1) {
			argv++;
			argc--;
			density = atoi(*argv) / 10;
		}
		break;

	case 's':			/* tape size, feet */
		if(argc > 1) {
			argv++;
			argc--;
			tsize = atol(*argv);
			tsize *= 12L*10L;
		}
		break;

	case '0':			/* dump level */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		incno = arg[-1];
		break;

	case 'u':			/* update /etc/dumpdates */
		uflag++;
		break;

	case 'n':			/* notify operators */
		notify++;
		break;

	default:
		fprintf(stderr, "bad key '%c%'\n", arg[-1]);
		Exit(X_ABORT);
	}
	if(strcmp(tape, "-") == 0) {
		pipeout++;
		tape = "standard output";
	}
	if(argc > 1) {
		argv++;
		argc--;
		disk = *argv;
	}

	if (signal(SIGHUP, sighup) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTRAP, sigtrap) == SIG_IGN)
		signal(SIGTRAP, SIG_IGN);
	if (signal(SIGFPE, sigfpe) == SIG_IGN)
		signal(SIGFPE, SIG_IGN);
	if (signal(SIGBUS, sigbus) == SIG_IGN)
		signal(SIGBUS, SIG_IGN);
	if (signal(SIGSEGV, sigsegv) == SIG_IGN)
		signal(SIGSEGV, SIG_IGN);
	if (signal(SIGTERM, sigterm) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	

	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	set_operators();	/* /etc/group snarfed */
	getfstab();		/* /etc/fstab snarfed */
	/*
	 *	disk can be either the full special file name,
	 *	the suffix of the special file name,
	 *	the special name missing the leading '/',
	 *	the file system name with or without the leading '/'.
	 */
	dt = fstabsearch(disk);
	if (dt != 0)
		disk = rawname(dt->fs_spec);
	getitime();		/* /etc/dumpdates snarfed */

	msg("Date of this level %c dump: %s\n", incno, prdate(spcl.c_date));
	msg("Date of last level %c dump: %s\n", incno, prdate(spcl.c_ddate));
	msg("Dumping %s ", disk);
	if (dt != 0)
		msgtail("(%s) ", dt->fs_file);
	msgtail("to %s\n", tape);

	fi = open(disk, 0);
	if (fi < 0) {
		msg("Cannot open %s\n", disk);
		Exit(X_ABORT);
	}
	CLR(clrmap);
	CLR(dirmap);
	CLR(nodmap);
	esize = 0;

	msg("mapping (Pass I) [regular files]\n");
	pass(mark, (short *)NULL);		/* mark updates esize */

	do {
		msg("mapping (Pass II) [directories]\n");
		nadded = 0;
		pass(add, dirmap);
	} while(nadded);

	bmapest(clrmap);
	bmapest(nodmap);

	fetapes =
		(	 esize		/* blocks */
			*BSIZE		/* bytes / block */
			*(1.0/density)	/* 0.1" / byte */
		  +
			 esize		/* blocks */
			*(1.0/NTREC)	/* IRG's / block */
			*7		/* 0.1" / IRG */
		) * (1.0 / tsize )	/* tape / 0.1" */
	;
	etapes = fetapes;		/* truncating assignment */
	etapes++;
	/*
	 *	esize is typically about 5% too low; we frob it here
	 */
	esize += ((5*esize)/100);
	msg("estimated %ld tape blocks on %3.2f tape(s).\n", esize, fetapes);

	otape();			/* bitmap is the first to tape write */
	time(&(tstart_writing));
	bitmap(clrmap, TS_CLRI);

	msg("dumping (Pass III) [directories]\n");
	pass(dump, dirmap);

	msg("dumping (Pass IV) [regular files]\n");
	pass(dump, nodmap);

	spcl.c_type = TS_END;
	for(i=0; i<NTREC; i++)
		spclrec();
	msg("DUMP: %ld tape blocks on %d tape(s)\n",spcl.c_tapea,spcl.c_volume);
	msg("DUMP IS DONE\n");

	putitime();
	if (!pipeout) {
		close(to);
		rewind_tape();
	}
	broadcast("DUMP IS DONE!\7\7\n");
	Exit(X_FINOK);
}

int	sighup(){	msg("SIGHUP()  try rewriting\n"); sigAbort();}
int	sigtrap(){	msg("SIGTRAP()  try rewriting\n"); sigAbort();}
int	sigfpe(){	msg("SIGFPE()  try rewriting\n"); sigAbort();}
int	sigbus(){	msg("SIGBUS()  try rewriting\n"); sigAbort();}
int	sigsegv(){	msg("SIGSEGV()  ABORTING!\n"); abort();}
int	sigalrm(){	msg("SIGALRM()  try rewriting\n"); sigAbort();}
int	sigterm(){	msg("SIGTERM()  try rewriting\n"); sigAbort();}

sigAbort()
{
	msg("Rewriting attempted as response to unknown signal.\n");
	fflush(stderr);
	fflush(stdout);
	close_rewind();
	exit(X_REWRITE);
}

char *rawname(cp)
	char *cp;
{
	static char rawbuf[32];
	char *dp = (char *)rindex(cp, '/');

	if (dp == 0)
		return (0);
	*dp = 0;
	strcpy(rawbuf, cp);
	*dp = '/';
	strcat(rawbuf, "/r");
	strcat(rawbuf, dp+1);
	return (rawbuf);
}
