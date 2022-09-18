#ifndef lint
static	char	*sccsid = "@(#)main.c	4.2	(ULTRIX)	10/16/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,1987,1988,1989 by		*
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
 *
 *   Modification History:
 *
 *	14-Apr-89	Tim N
 *	Removed def and init of nopdir which is not used.  Changed method
 *	of determining operating mode.  It used to check command line to see
 *	if it was called make or s5make and then run either BSD or SYSV mode.
 *	Now it checks the command line for its name, if named 's5make' then
 *	it will run in s5mode regardless of PROG_ENV.  If it is not being run
 *	as 's5make' then it checks for PROG_ENV:
 *		not set or = BSD, gets BSD mode
 *		SYSTEM_FIVE, gets SYSV mode
 *		POSIX, gets SYSV mode with POSIX changes for SHELL and
 *		       MAKESHELL vars.
 *	We have to do this up front because the mode also determines the
 *	behavoir to use with reading SHELL env var value.
 */

# include "defs"
/*
command make to update programs.
Flags:	'd'  print out debugging comments
	'p'  print out a version of the input graph
	's'  silent mode--don't print out commands
	'f'  the next argument is the name of the description file;
	     makefile is the default
	'i'  ignore error codes from the shell
	'S'  stop after any command fails (normally do parallel work)
	'n'   don't issue, just print, commands
	't'   touch (update time of) files but don't issue command
	'q'   don't do anything, but check if object is up to date;
	      returns exit code 0 if up to date, -1 if not
*/

char makefile[] = "makefile";
char Nullstr[] = "";
char Makefile[] =	"Makefile";
char RELEASE[] = "RELEASE";
CHARSTAR badptr = (CHARSTAR)-1;

NAMEBLOCK mainname ;
NAMEBLOCK firstname;
LINEBLOCK sufflist;
VARBLOCK firstvar;
PATTERN firstpat ;
DIRHDR firstdir;


#include <signal.h>
#include <strings.h>

int sigivalue=0;
int sigqvalue=0;
int waitpid=0;

int Mflags=MH_DEP;
int ndocoms=0;
int okdel=YES;

CHARSTAR prompt="\t";	/* other systems -- pick what you want */
char junkname[20];
char funny[128];




char Makeflags[]="MAKEFLAGS";

char * s5name = "s5make";
int sysvmode;			/* 0 if in bsd mode and 1 if in sysvmode */
extern char * getenv();
void check_makeshell();

main(argc,argv)
int argc;
CHARSTAR argv[];
{
	register NAMEBLOCK p;
	int i, j;
	int descset, nfargs;
	TIMETYPE tjunk;
	char c;
	CHARSTAR s, *e;
	char *modestr;

#ifdef unix
	int intrupt();
	if ( (modestr = rindex(argv[0],'/')) == (char *)0 )
	    modestr = argv[0];
	else
	    modestr++;

	if (strcmp(modestr,s5name) == 0){
	    sysvmode = SYSV_ON;
	} else {
	    sysvmode = 0;
	    if( ((modestr = getenv("PROG_ENV")) != NULL ) && (*modestr)) {
	        if( strcmp(modestr,"SYSTEM_FIVE") == 0)
		    sysvmode = SYSV_ON;
	        else if( strcmp(modestr,"POSIX") == 0)
		    sysvmode = POSIX_ON;
	    }
	}

#endif

#ifdef METERFILE
	meter(METERFILE);
#endif

	descset = 0;

	for(s = "|=^();&<>*?[]:$`'\"\\\n" ; *s ; ++s)
		funny[*s] |= META;
	if (sysvmode) funny['#'] |= META;
	for(s = "\n\t :=;{}&>|" ; *s ; ++s)
		funny[*s] |= TERMINAL;
	funny['\0'] |= TERMINAL;

	TURNON(INTRULE);		/* Default internal rules, turned on */

/*
 *	Set command line flags
 */

	getmflgs();				/* Init $(MAKEFLAGS) variable */
	setflags(argc, argv);

	setvar("$","$");


/*
 *	Read command line "=" type args and make them readonly.
 */
	TURNON(INARGS|EXPORT);
	if(IS_ON(DBUG))printf("Reading \"=\" type args on command line.\n");
	for(i=1; i<argc; ++i)
		if(argv[i]!=0 && argv[i][0]!=MINUS && (eqsign(argv[i]) == YES) )
			argv[i] = 0;
	TURNOFF(INARGS|EXPORT);

/*
 *	Read internal definitions and rules.
 */

	if( IS_ON(INTRULE) )
	{
		if(IS_ON(DBUG))printf("Reading internal rules.\n");
		rdd1(NULL);
	}

/*
 *	Done with internal rules, now.
 */
	TURNOFF(INTRULE);

/*
 *	Read environment args.  Let file args which follow override.
 *	unless 'e' in MAKEFLAGS variable is set.
 */
	if( any( (varptr(Makeflags))->varval, 'e') )
		TURNON(ENVOVER);
	if(IS_ON(DBUG))printf("Reading environment.\n");
	TURNON(EXPORT);
	readenv();
	TURNOFF(EXPORT|ENVOVER);

/*
 *	Read command line "-f" arguments.
 */

	rdmakecomm();

	for(i = 1; i < argc; i++)
		if( argv[i] && argv[i][0] == MINUS && argv[i][1] == 'f' && argv[i][2] == CNULL)
		{
			argv[i] = 0;
			if(i >= argc-1)
				fatal("No description argument after -f flag");
			if( rddescf(argv[++i], YES) )
				fatal1("Cannot open %s", argv[i]);
			argv[i] = 0;
			++descset;
		}


/*
 *	If no command line "-f" args then look for some form of "makefile"
 */
	if( !descset )
#ifdef unix
		if( rddescf(makefile, NO))
		if( rddescf(Makefile, NO))
		if( rddescf(makefile, YES))
			rddescf(Makefile, YES);

#endif
#ifdef gcos
		rddescf(makefile, NO);
#endif


	if(IS_ON(PRTR)) printdesc(NO);

	if( srchname(".IGNORE") ) TURNON(IGNERR);
	if( srchname(".SILENT") ) TURNON(SIL);
	if(p=srchname(".SUFFIXES")) sufflist = p->linep;
	if( !sufflist ) fprintf(stderr,"No suffix list.\n");

#ifdef unix
	sigivalue = (int)signal(SIGINT,1) & 01;
	sigqvalue = (int)signal(SIGQUIT,1) & 01;
	enbint(intrupt);
#endif


	/*
	 * Now set SHELL to MAKESHELL if needed if in posix mode
	*/
	if(sysvmode == POSIX_ON)
		check_makeshell();

	nfargs = 0;
	if(IS_ON(DBUG)) {
		char pathname[MAXPATHLEN];

		if(getwd(pathname) == 0) {
			fprintf(stderr,
				"error trying to get current working directory, %s\n",
																  pathname);
		} else {
			printf("Current working directory for make is %s\n", pathname);
		}
	}

	for(i=1; i<argc; ++i)
		if((s=argv[i]) != 0)
		{
			if((p=srchname(s)) == 0)
			{
				p = makename(s);
			}
			++nfargs;
			doname(p, 0, &tjunk);
			if(IS_ON(DBUG)) printdesc(YES);
		}

/*
	If no file arguments have been encountered, make the first
	name encountered that doesn't start with a dot
	*/

	if(nfargs == 0)
		if(mainname == 0)
			fatal("No arguments or description file");
		else
		{
			doname(mainname, 0, &tjunk);
			if(IS_ON(DBUG)) printdesc(YES);
		}

	mexit(0);
}



#ifdef unix
intrupt()
{
	CHARSTAR p;
	NAMEBLOCK q;

	if(okdel && IS_OFF(NOEX) && IS_OFF(TOUCH) &&
	   (p=varptr("@")->varval) && 
	   (q=srchname(p)) && 
	   (exists(q)>0) &&
	   !isprecious(p) )
	{
		if(isdir(p))
			fprintf(stderr, "\n*** %s NOT REMOVED.",p);
		else if(unlink(p) == 0)
			fprintf(stderr, "\n***  %s removed.", p);
	}
	if(junkname[0])
		unlink(junkname);
	fprintf(stderr, "\n");
	mexit(2);
}




isprecious(p)
CHARSTAR p;
{
	register NAMEBLOCK np;
	register LINEBLOCK lp;
	register DEPBLOCK dp;

	if(np = srchname(".PRECIOUS"))
	    for(lp = np->linep ; lp ; lp = lp->nextline)
		for(dp = lp->depp ; dp; dp = dp->nextdep)
			if(equal(p, dp->depname->namep))
				return(YES);

	return(NO);
}


enbint(k)
int (*k)();
{
	if(sigivalue == 0)
		signal(SIGINT,k);
	if(sigqvalue == 0)
		signal(SIGQUIT,k);
}
#endif

extern CHARSTAR builtin[];

CHARSTAR *linesptr=builtin;

FILE * fin;
int firstrd=0;

rdmakecomm()
{
#ifdef PWB
	register char *nlog;
	char s[128];
	CHARSTAR concat(), getenv();

	if(rddescf( concat((nlog=getenv("HOME")),"/makecomm",s), NO))
		rddescf( concat(nlog,"/Makecomm",s), NO);

	if(rddescf("makecomm", NO))
		rddescf("Makecomm", NO);
#endif
}

extern int yylineno;
extern CHARSTAR zznextc;
rddescf(descfile, flg)
CHARSTAR descfile;
int flg;			/* if YES try s.descfile */
{
	FILE * k;

/* read and parse description */

	if(equal(descfile, "-"))
		return( rdd1(stdin) );

retry:
	if( (k = fopen(descfile,"r")) != NULL)
	{
		if(IS_ON(DBUG))printf("Reading %s\n", descfile);
		return( rdd1(k) );
	}

	if(flg == NO)
		return(1);
	if(get(descfile, CD, varptr(RELEASE)->varval) == NO)
		return(1);
	flg = NO;
	goto retry;

}




rdd1(k)
FILE * k;
{
	fin = k;
	yylineno = 0;
	zznextc = 0;

	if( yyparse() )
		fatal("Description file error");

	if(fin != NULL)
		fclose(fin);

	return(0);
}

printdesc(prntflag)
int prntflag;
{
	NAMEBLOCK p;
	DEPBLOCK dp;
	VARBLOCK vp;
	DIRHDR od;
	SHBLOCK sp;
	LINEBLOCK lp;

#ifdef unix
	if(prntflag)
	{
		fprintf(stderr,"Open directories:\n");
		for(od=firstdir; od!=0; od = od->nextdirhdr)
			fprintf(stderr,"\t%d: %s\n",od->dirfc->dd_fd,od->dirn);
	}
#endif

	if(firstvar != 0) fprintf(stderr,"Macros:\n");
	for(vp=firstvar; vp!=0; vp=vp->nextvar)
		if(vp->v_aflg == NO)
			printf("%s = %s\n" , vp->varname , vp->varval);
		else
		{
			CHAIN pch;

			fprintf(stderr,"Lookup chain: %s\n\t", vp->varname);
			for(pch = (CHAIN)vp->varval; pch; pch = pch->nextchain)
				fprintf(stderr," %s",
					((NAMEBLOCK)pch->datap)->namep);
			fprintf(stderr,"\n");
		}

	for(p=firstname; p!=0; p = p->nextname)
		prname(p, prntflag);
	printf("\n");
	fflush(stdout);
}

prname(p, prntflag)
register NAMEBLOCK p;
{
	register LINEBLOCK lp;
	register DEPBLOCK dp;
	register SHBLOCK sp;

	if(p->linep != 0)
		printf("\n\n%s:",p->namep);
	else
		fprintf(stderr, "\n\n%s", p->namep);
	if(prntflag)
	{
		fprintf(stderr,"  done=%d",p->done);
	}
	if(p==mainname) fprintf(stderr,"  (MAIN NAME)");
	for(lp = p->linep ; lp!=0 ; lp = lp->nextline)
	{
		if( dp = lp->depp )
		{
			fprintf(stderr,"\n depends on:");
			for(; dp!=0 ; dp = dp->nextdep)
				if(dp->depname != 0)
				{
					printf(" %s", dp->depname->namep);
					printf(" ");
				}
		}
		if(sp = lp->shp)
		{
			printf("\n");
			fprintf(stderr," commands:\n");
			for( ; sp!=0 ; sp = sp->nextsh)
				printf("\t%s\n", sp->shbp);
		}
	}
}


setflags(ac, av)
int ac;
CHARSTAR *av;
{
	register int i, j;
	register char c;
	int flflg=0;			/* flag to note `-f' option. */

	for(i=1; i<ac; ++i)
	{
		if(flflg)
		{
			flflg = 0;
			continue;
		}
		if(av[i]!=0 && av[i][0]==MINUS)
		{
			if(any(av[i], 'f'))
				flflg++;
			for(j=1 ; (c=av[i][j])!=CNULL ; ++j)
				optswitch(c);
			if(flflg)
				av[i] = "-f";
			else
				av[i] = 0;
		}
	}
}


/*
 *	Handle a single char option.
 */
optswitch(c)
register char c;
{

	switch(c)
	{

	case 'e':	/* environment override flag */
		setmflgs(c);
		break;

	case 'd':	/* debug flag */
		TURNON(DBUG);
		setmflgs(c);
		break;

	case 'p':	/* print description */
		TURNON(PRTR);
		break;

	case 's':	/* silent flag */
		TURNON(SIL);
		setmflgs(c);
		break;

	case 'i':	/* ignore errors */
		TURNON(IGNERR);
		setmflgs(c);
		break;

	case 'S':
		TURNOFF(KEEPGO);
		setmflgs(c);
		break;

	case 'k':
		TURNON(KEEPGO);
		setmflgs(c);
		break;

	case 'n':	/* do not exec any commands, just print */
		TURNON(NOEX);
		setmflgs(c);
		break;

	case 'r':	/* turn off internal rules */
		TURNOFF(INTRULE);
		break;

	case 't':	/* touch flag */
		TURNON(TOUCH);
		setmflgs(c);
		break;

	case 'q':	/* question flag */
		TURNON(QUEST);
		setmflgs(c);
		break;

	case 'g':	/* turn default $(GET) of files not found */
		TURNON(GET);
		setmflgs(c);
		break;

	case 'm':	/* print memory map */
		TURNON(MEMMAP);
		setmflgs(c);
		break;

	case 'b':	/* use MH version of test for whether a cmd exists */
		TURNON(MH_DEP);
		setmflgs(c);
		break;
	case 'B':	/* turn off -b flag */
		TURNOFF(MH_DEP);
		setmflgs(c);
		break;

	case 'f':	/* Named makefile; already handled by setflags(). */
		break;

	default:
		fatal1("Unknown flag argument %c", c);
	}
}

/*
 *	getmflgs() set the cmd line flags into an EXPORTED variable
 *	for future invocations of make to read.
 */


getmflgs()
{
	register VARBLOCK vpr;
	register CHARSTAR *pe;
	register CHARSTAR p;

	vpr = varptr(Makeflags);
	setvar(Makeflags, "ZZZZZZZZZZZZZZZZ");
	vpr->varval[0] = CNULL;
	vpr->envflg = YES;
	vpr->noreset = YES;
	optswitch('b');
	for(pe = environ; *pe; pe++)
	{
		if(sindex(*pe, "MAKEFLAGS=") == 0)
		{
			for(p = (*pe)+sizeof Makeflags; *p; p++)
				optswitch(*p);
			return;
		}
	}
}

/*
 *	setmflgs(c) sets up the cmd line input flags for EXPORT.
 */

setmflgs(c)
register char c;
{
	register VARBLOCK vpr;
	register CHARSTAR p;

	vpr = varptr(Makeflags);
	for(p = vpr->varval; *p; p++)
	{
		if(*p == c)
			return;
	}
	*p++ = c;
	*p = CNULL;
}

/*
 *	This routine checks to see if variable MAKESHELL exists and if so then
 *	it overrides variable SHELL if it exists.  This should only be called
 *	when you want make to be posix compliant.  This way we do
 *	it on one shot and never have to check again during entire
 *	execution.
 */

void check_makeshell()
{
	register VARBLOCK shell_v;
	register VARBLOCK makeshell_v;

	/* check for existing makeshell var with a value */
	makeshell_v = srchvar("MAKESHELL");
	if ((makeshell_v == 0) || (makeshell_v->varval == 0) ||
	    (*makeshell_v->varval == '\0'))
	    return;

	/* don't need to check for SHELL to exist it is in internal rules */
	shell_v = srchvar("SHELL");

	/*
	 * now just force makeshell over shell - even if shell is
         * set for noreset.  This routine gets called after all the
	 * vars get set so this should continue to overide shell for the
	 * duration.
	 */
	shell_v->varval = makeshell_v->varval;

	return;
}
