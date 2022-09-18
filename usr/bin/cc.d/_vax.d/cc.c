#ifndef lint
static	char	*sccsid = "@(#)cc.c	4.2	(ULTRIX)	11/9/90";
#endif

/*
 * cc - front end for C compiler
 */
#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/dir.h>

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1987, 1988 by		*
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
 *  Jim McHale, 22-Oct-90; added -bswitch to gen heap switches (seperate
 *              cmp/jbr instead of casel); fixes cld 153/qar 6081.
 *
 *  Jim McHale, 15-Oct-90; added -v.
 *
 *	Lee Miller, 19-Sep-1990 for qar 5752				*
 * 015	create Jflag so that the -J option for the assembler can be passed *
 *  in from the cc command line.	*
 *									*
 *	Jon Reeves, 9-May-1989 for Sid Maxwell				*
 * 014	On cpp error, continue to compile (not just preprocess).	*
 *	Code from 4.3-Tahoe.						*
 *									*
 *	Mark Parenti, 8-Jun-1988					*
 * 013	Change signal handlers to void.					*
 *									*
 *	Jon Reeves, 7-Dec-1987						*
 * 012	Integrate BSD4.3 features: clean up on SIGHUP, pass -L through	*
 *	to linker							*
 *									*
 *	Jon Reeves, 23-Nov-1987						*
 * 011	Expanded definition of -Y option to -YSYSTEM_FIVE, -YBSD, or	*
 *	-YPOSIX.  Major changes to 007.					*
 *									*
 * 	Lu Anne Van de Pas, 20-Jun-1986					*
 * 010  Added check to see if multiple files were specified with the -Em*
 *	switch, if so don't output "filename:". 			*
 *									*
 *	Lu Anne Van de Pas, 21-Jan-1986					*
 * 009  Added -f flag so we do do single precision arithmetic in single *
 *	precision instead of promoting it to double. 			*
 *									*
 *	David L Ballenger, 7-Jun-1985					*
 * 008	Check PROG_ENV environment variable instead of SYSTYPE, for	*
 *	System V environment.  Also select appropriate math library	*
 *	when using System V environment.				*
 *									*
 *	David L. Ballenger, 31-Mar-1985					*
 * 007-	Add System V environment support.				*
 *	Also add handling for alternate assembler and loader to -t	*
 *	switch, ie. -t[alp012].						*
 *									*
 *	David L. Ballenger, 7-Nov-1984					*
 * 006-	Allow the -M flag to be passed to the loader. 			*
 *									*
 * 005-	David L Ballenger,	23-Aug-1984				*
 *	If the -Mg flag is specified, use -lcg instead of -lc as the	*
 *	library argument to ld.  This causes the loader to use the	*
 *	GFLOAT version of libc.						*
 *									*
 *	Rich Phillips,  01-Aug-84					*
 * 004- Issue a warning when -o is used with -S or -c, since the loader *
 *	will not be called in either case.				*
 *									*
 *	Stephen Reilly, 16-Apr-84					*
 * 003- Changed the symbol MVAXI to GFLOAT				*
 *									*
 *	Stephen Reilly, 04-Apr-84					*
 * 002- Changed the symbol MicroVAX to MVAXI. Also added new flag "-b"	*
 *	when specified will not pass -lc to the loader.			*
 *									*
 *	Stephen Reilly, 21-Oct-83:					*
 * 001- Add code for the -Mx flag.  This flag indicates that gfloat  	*
 *	instructions are to be emited instead of dfloat instructions	*
 *									*
 ************************************************************************/

char	*cpp = "/lib/cpp";
char	*ccom = "/lib/ccom";
char	*c2 = "/lib/c2";
char	*as = "/bin/as";
char	*ld = "/bin/ld";
char	*crt0 = "/lib/crt0.o";


char	tmp0[30];		/* big enough for /tmp/ctm%05.5d */
char	*tmp1, *tmp2, *tmp3, *tmp4, *tmp5;
char	*outfile;
char	*savestr(), *strspl(), *setsuf();
void	idexit();
char	**av, **clist, **llist, **plist;
int	cflag, eflag, oflag, pflag, sflag, wflag, Rflag, exflag, proflag;
int	gflag, Gflag;
int vflag = 0;								/* jpm */
int	Jflag = 0;		/*lrm015*/
int bswitch = 0;							/* jpm: fix cld 153 */
int 	fflag = 0; 		/* do single precision arithmetic lvp009*/ 
int	bflag;			/* slr002 Don't pass -lc to loader */
int 	makedepend = 0;		/* vdp010 don't print file names if -Em*/
# ifdef	GFLOAT			/* slr003 changed from MVAXI to GFLOAT */
int	Mflag = 1;		/* slr001 default for MicroVAX is gfloat */
# else
int	Mflag;			/* slr001 default for VAX is dfloat */
# endif
int	Yflag;			/* jlr011 Set if -Y was given */
char	*dflag;
int	exfail;
char	*chpass;
char	*npassname;
enum	run_mode {bsd, sys_v, posix} mode = bsd; /* jlr011 Desired run mode */
					/* assume bsd for compatibility */

int	nc, nl, np, nxo, na;

#define	cunlink(s)	if (s) unlink(s)

main(argc, argv)
	char **argv;
{
	char *t;
	char *assource;
	int i, j, c;
	char *penv, *getenv();

	/* ld currently adds upto 5 args; 10 is room to spare */
	av = (char **)calloc(argc+10, sizeof (char **));
	clist = (char **)calloc(argc, sizeof (char **));
	llist = (char **)calloc(argc, sizeof (char **));
	plist = (char **)calloc(argc, sizeof (char **));

	/*
	 * Check the PROG_ENV environment variable to see what environment
	 * we should use. (jlr011)
	 */
	if ( strcmp("SYSTEM_FIVE", penv = getenv("PROG_ENV")) == 0)
	 	mode = sys_v;
	else if ( strcmp("POSIX", penv) == 0)
		mode = posix;
	else if ( strcmp("BSD", penv) == 0)
		mode = bsd;
	 
	for (i = 1; i < argc; i++) {
		if (*argv[i] == '-') switch (argv[i][1]) {

		case 'S':
			sflag++;
			cflag++;
			continue;
		case 'o':
			if (++i < argc) {
				outfile = argv[i];
				switch (getsuf(outfile)) {

				case 'c':
				case 'o':
					error("-o would overwrite %s",
					    outfile);
					exit(8);
				}
			}
			continue;

		/*
		 *	When -Mx flag is set it indicates that user wants
		 *	to override default. If x is not either g or d
		 *	then stay with default
		 */

		case 'M':				/* slr001 */
			/*
			 *	See what the argument after the -M is
			 */
			switch ( argv[i][2] )
			    {
			    case 'g': 	      /* slr001 if g then gfloat */
				/*
				 *	Set flag indicating gfloat for
				 *	MicroVAX and define MVAXI for
			 	 *	cpp.
				 */
				Mflag = 1;    			/* slr001 */
				plist[np++] = "-DGFLOAT"; 	/* slr003 */
				continue;

			    case 'd': 	      /* slr001 if d then dfloat */
				Mflag = 0;    /* slr001 set flag for dfloat */
				continue;
			    }
			    /* If the flag wasn't -Md or -Mg then break so
			     * the -M flag can be passed on to the loader.
			     */
			    break;		/* DLB006 */
		case 'b':

			/* '-b' alone dictates that the user does not want
			 *  the -lc flag passed to the loader.
			 *
			 * -bswitch says to never generate casel instructions.
			 */
			if (argv[i][2] == '\0')			/* if just -b */
			{
				bflag++;			/* slr002 */
			}
			else if (strcmp( "bswitch", &argv[i][1] ) == 0) /* jpm: cld 153 */
			{
				bswitch++;
			}
			continue;
			
		case 'f':
			fflag++;			/*vdp009*/ 
			continue; 
		
		case 'J':		/*lrm015*/
			Jflag++;
			continue;
		
		case 'R':
			Rflag++;
			continue;
		case 'O':
			oflag++;
			continue;
		case 'p':
			proflag++;
			crt0 = "/lib/mcrt0.o";
			if (argv[i][2] == 'g')
				crt0 = "/usr/lib/gcrt0.o";
			continue;
		case 'g':
			if (argv[i][2] == 'o') {
			    Gflag++;	/* old format for -go */
			} else {
			    gflag++;	/* new format for -g */
			}
			continue;
		  case 'v':
			++vflag;
			continue;
		case 'w':
			wflag++;
			continue;
		case 'E':
			exflag++;
			if (argv[i][2] == 'm') makedepend++;
		case 'P':
			pflag++;
			if (argv[i][1]=='P')
				fprintf(stderr,
	"cc: warning: -P option obsolete; you should use -E instead\n");
			plist[np++] = argv[i];
		case 'c':
			cflag++;
			continue;
		case 'D':
		case 'I':
		case 'U':
		case 'C':
			plist[np++] = argv[i];
			continue;
		case 'L':
			llist[nl++] = argv[i];
			continue;
		case 't':
			if (chpass)
				error("-t overwrites earlier option", 0);
			chpass = argv[i]+2;
			if (chpass[0]==0)
				chpass = "012p";
			continue;
		case 'B':
			if (npassname)
				error("-B overwrites earlier option", 0);
			npassname = argv[i]+2;
			if (npassname[0]==0)
				npassname = "/usr/c/o";
			continue;
		case 'd':
			dflag = argv[i];
			continue;

		case 'Y':
			if (argv[i][2] == '\0') {
				mode = sys_v; /* For pre-V2.4 compatibility */
				Yflag++;
			} else if (strcmp("SYSTEM_FIVE", &argv[i][2]) == 0) {
				mode = sys_v;
				Yflag++;
			} else if (strcmp("POSIX", &argv[i][2]) == 0) {
				mode = posix;
				Yflag++;
			} else if (strcmp("BSD", &argv[i][2]) == 0) {
				mode = bsd;
				Yflag++;
			} else
				(void) fprintf(stderr,
					"cc: warning: bad -Y argument %s ignored\n", 
					&argv[i][2]);
			continue;
		}
		t = argv[i];
		c = getsuf(t);
		if (c=='c' || c=='s' || exflag) {
			clist[nc++] = t;
			t = setsuf(t, 'o');
		}
		if (nodup(llist, t)) {
			if (mode == sys_v) {
				/* Select System V math library, if
				 * math library is being used.
				 */
				if (strcmp(t,"-lm") == 0)
					llist[nl++] = "-lmV";
				else if (strcmp(t,"-lmg") == 0)
					llist[nl++] = "-lmVg" ;
				else if (strcmp(t,"-lm_p") == 0)
					llist[nl++] = "-lmV_p" ;
				else
					llist[nl++] = t;
			} else
				llist[nl++] = t;
			if (getsuf(t)=='o')
				nxo++;
		}
	}
	switch (mode) {
	case sys_v:
		plist[np++] = "-DSYSTEM_FIVE" ;
		break;
	case posix:
		plist[np++] = "-DPOSIX" ;
		break;
	case bsd:
		break;
	}
	if (cflag && outfile) /*RAP004*/
		fprintf(stderr,"cc: warning: -o ignored when -c or -S used\n");
	if (gflag || Gflag) {
		if (oflag)
			fprintf(stderr, "cc: warning: -g disables -O\n");
		oflag = 0;
	}
	if (npassname && chpass ==0)
		chpass = "012p";
	if (chpass && npassname==0)
		npassname = "/usr/new";
	if (chpass)
	for (t=chpass; *t; t++) {
		switch (*t) {

		case '0':
			ccom = strspl(npassname, "ccom");
			continue;
		case '2':
			c2 = strspl(npassname, "c2");
			continue;
		case 'a':
			as = strspl(npassname, "as");
			continue;
		case 'l':
			ld = strspl(npassname, "ld");
			continue;
		case 'p':
			cpp = strspl(npassname, "cpp");
			continue;
		}
	}
	if (nc==0)
		goto nocom;
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, idexit);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, idexit);
	if (pflag==0)
		sprintf(tmp0, "/tmp/ctm%05.5d", getpid());
	tmp1 = strspl(tmp0, "1");
	tmp2 = strspl(tmp0, "2");
	tmp3 = strspl(tmp0, "3");
	if (pflag==0)
		tmp4 = strspl(tmp0, "4");
	if (oflag)
		tmp5 = strspl(tmp0, "5");
	for (i=0; i<nc; i++) {
		if (nc > 1 && !makedepend) {
			printf("%s:\n", clist[i]);
			fflush(stdout);
		}
		if (getsuf(clist[i]) == 's') {
			assource = clist[i];
			goto assemble;
		} else
			assource = tmp3;
		if (pflag)
			tmp4 = setsuf(clist[i], 'i');
		av[0] = "cpp"; av[1] = clist[i]; av[2] = exflag ? "-" : tmp4;
		na = 3;
		for (j = 0; j < np; j++)
			av[na++] = plist[j];
		av[na++] = 0;
		if (callsys(cpp, av)) {
			cflag++;
			eflag++;
			continue;
		}
		if (pflag || exfail) {
			cflag++;
			continue;
		}
		if (sflag)
			assource = tmp3 = setsuf(clist[i], 's');
		av[0] = "ccom"; av[1] = tmp4; av[2] = oflag?tmp5:tmp3; na = 3;
		if (bswitch)						/* jpm: cld 153 */
		{
			av[na++] = "-XS";
		}
		if (proflag)
			av[na++] = "-XP";
		if (gflag) {
			av[na++] = "-Xg";
		} else if (Gflag) {
			av[na++] = "-XG";
		}
		if (wflag)
			av[na++] = "-w";
		if (fflag)			/* pass fflag to the */ 
			av[na++] = "-f";        /* compiler  lvp009 */  

		/*
		 *	If -Mx flag set we must pass this off to the
		 *	compiler proper.  We do not need to carry
		 *	the suffix after -M because the flag indicates
		 *	the type of float instruction the user wants
		 *	and is internal to the compiler.
		 */

		if ( Mflag )				/* slr001 */
			av[na++] = "-M";		/* slr001 */

		av[na] = 0;
		if (callsys(ccom, av)) {
			cflag++;
			eflag++;
			continue;
		}
		if (oflag) {
			av[0] = "c2"; av[1] = tmp5; av[2] = tmp3; av[3] = 0;
			if (callsys(c2, av)) {
				unlink(tmp3);
				tmp3 = assource = tmp5;
			} else
				unlink(tmp5);
		}
		if (sflag)
			continue;
	assemble:
		cunlink(tmp1); cunlink(tmp2); cunlink(tmp4);
		av[0] = "as"; av[1] = "-o"; av[2] = setsuf(clist[i], 'o');
		na = 3;
		if (Jflag)			/*lrm015*/
			av[na++] = "-J";
		if (Rflag)
			av[na++] = "-R";
		if (dflag)
			av[na++] = dflag;
		av[na++] = assource;
		av[na] = 0;
		if (callsys(as, av) > 1) {
			cflag++;
			eflag++;
			continue;
		}
	}
nocom:
	if (cflag==0 && nl!=0) {
		i = 0;
		av[0] = "ld"; av[1] = "-X"; av[2] = crt0; na = 3;
		if (Yflag) 			/* pass Yflag through */
			switch (mode) {		/* if present jlr011 */
			case sys_v:
				av[na++] = "-YSYSTEM_FIVE";
				break;
			case posix:
				av[na++] = "-YPOSIX";
				break;
			case bsd:
				av[na++] = "-YBSD";
				break;
			}
		if (outfile) {
			av[na++] = "-o";
			av[na++] = outfile;
		}
		while (i < nl)
			av[na++] = llist[i++];
		if (gflag || Gflag)
			av[na++] = "-lg";
		if (proflag) {
			if (mode == sys_v) av[na++] = "-lcV_p" ; /* DLB007 */
			if (mode == posix) av[na++] = "-lcP_p" ;
			av[na++] = "-lc_p";

		} else if (!bflag) {
			/*
			 * If bflag is set then don't pass the -lc
			 * flag to the loader.  
			 *
			 * If Mflag is set use the GFLOAT libraries.
			 *
			 * If mode == sys_v or posix use the System V or
			 * POSIX libraries in addition to the regular 
			 * libraries. (DLB007)
			 */
			if (mode == sys_v)
			    	av[na++] = Mflag ? "-lcVg" : "-lcV" ;
			if (mode == posix)
			    	av[na++] = Mflag ? "-lcPg" : "-lcP" ;
			av[na++] = Mflag ? "-lcg" : "-lc" ;	/* DLB005 */
		}

		av[na++] = 0;
		eflag |= callsys(ld, av);
		if (nc==1 && nxo==1 && eflag==0)
			unlink(setsuf(clist[0], 'o'));
	}
	dexit();
}

void
idexit()
{

	eflag = 100;
	dexit();
}

dexit()
{

	if (!pflag) {
		cunlink(tmp1);
		cunlink(tmp2);
		if (sflag==0)
			cunlink(tmp3);
		cunlink(tmp4);
		cunlink(tmp5);
	}
	exit(eflag);
}

error(s, x)
	char *s, *x;
{
	FILE *diag = exflag ? stderr : stdout;

	fprintf(diag, "cc: ");
	fprintf(diag, s, x);
	putc('\n', diag);
	exfail++;
	cflag++;
	eflag++;
}

getsuf(as)
char as[];
{
	register int c;
	register char *s;
	register int t;

	s = as;
	c = 0;
	while (t = *s++)
		if (t=='/')
			c = 0;
		else
			c++;
	s -= 3;
	if (c <= MAXNAMLEN && c > 2 && *s++ == '.')
		return (*s);
	return (0);
}

char *
setsuf(as, ch)
	char *as;
{
	register char *s, *s1;

	s = s1 = savestr(as);
	while (*s)
		if (*s++ == '/')
			s1 = s;
	s[-1] = ch;
	return (s1);
}

callsys(f, v)
	char *f, **v;
{
	int t, status;

	if (vflag)								/* jpm */
	{
		printf("%s", f );
		for (t = 1; v[t] != 0; t++) printf(" %s", v[t]);
		printf("\n");
	}

	t = vfork();
	if (t == -1) {
		printf("No more processes\n");
		return (100);
	}
	if (t == 0) {
		execv(f, v);
		printf("Can't find %s\n", f);
		fflush(stdout);
		_exit(100);
	}
	while (t != wait(&status))
		;
	if ((t=(status&0377)) != 0 && t!=14) {
		if (t!=2) {
			printf("Fatal error in %s\n", f);
			eflag = 8;
		}
		dexit();
	}
	return ((status>>8) & 0377);
}

nodup(l, os)
	char **l, *os;
{
	register char *t, *s;
	register int c;

	s = os;
	if (getsuf(s) != 'o')
		return (1);
	while (t = *l++) {
		while (c = *s++)
			if (c != *t++)
				break;
		if (*t==0 && c==0)
			return (0);
		s = os;
	}
	return (1);
}

#define	NSAVETAB	1024
char	*savetab;
int	saveleft;

char *
savestr(cp)
	register char *cp;
{
	register int len;

	len = strlen(cp) + 1;
	if (len > saveleft) {
		saveleft = NSAVETAB;
		if (len > saveleft)
			saveleft = len;
		savetab = (char *)malloc(saveleft);
		if (savetab == 0) {
			fprintf(stderr, "ran out of memory (savestr)\n");
			exit(1);
		}
	}
	strncpy(savetab, cp, len);
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return (cp);
}

char *
strspl(left, right)
	char *left, *right;
{
	char buf[BUFSIZ];

	strcpy(buf, left);
	strcat(buf, right);
	return (savestr(buf));
}
