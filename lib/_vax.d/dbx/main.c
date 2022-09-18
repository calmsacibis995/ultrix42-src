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
/************************************************************************
 *									*
 *			Modification History				*
 *                                                                      *
 *  011	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *      010 - Modifications for dxdb, format of printf and .dbxinit     *
 *	      (David Metsky, Feb 6, 1989)				*
 *									*
 *	009 - Added a xterm path argument to -x for xdb.		*
 *	      (David Metsky, July 27, 1988)				*
 *									*
 *	008 - Changed signal handlers to void.				*
 *	      (Mark Parenti, June 9, 1988)				*
 *									*
 *      007 - Defined a new flag, -x for xdb.				*
 *            (David Metsky, March 8, 1988)				*
 *									*
 *	006 - Added new debugging flag "tracefuncs".			*
 *	      (vjh, July 16, 1986)					*
 *									*
 *	005 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	004 - Update copyright.						*
 *	      (vjh, August 23, 1985)					*
 *									*
 *	003 - Require inclusion of version.h, to print out current	*
 *	      software release version number.				*
 *	      (vjh, May 10, 1985)					*
 *									*
 *	002 - Require a flag INHOUSE to be defined for the command	*
 *	      line switches -b, -e, -s, and -n to be permitted.		*
 *	      These are all switches used in debugging dbx.		*
 *	      (vjh, April 15, 1985)					*
 *									*
 *	001 - Shortened prompt by calling rindex to remove preceding	*
 *	      directory specification.  Added routine mkinitfile to	*
 *	      "build" the name of the initialization file; makes	*
 *	      debugging the debugger easier.				*
 *	      (Victoria Holt, April 9, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char *sccsid = "@(#)main.c	4.2	ULTRIX	11/9/90";
#endif not lint

/*
 * Debugger main routine.
 */


#include "defs.h"
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include "main.h"
#include "eval.h"
#include "debug.h"
#include "symbols.h"
#include "scanner.h"
#include "keywords.h"
#include "process.h"
#include "runtime.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "coredump.h"
#include "version.h"

#ifndef public

#define isterm(file)	(interactive or isatty(fileno(file)))

#include <sgtty.h>
#include <fcntl.h>

typedef struct {
    struct sgttyb sg;		/* standard sgttyb structure */
    struct tchars tc;		/* terminal characters */
    struct ltchars ltc;		/* local special characters */
    integer ldisc;		/* line discipline */
    integer local;		/* TIOCLGET */
    integer fcflags;		/* fcntl(2) F_GETFL, F_SETFL */
} Ttyinfo;

#endif

public boolean vectorcapable;   /* true if vector capable processor */
public boolean found_a_vec_inst; /*  true if instrction decode in   */
								 /*  findnextaddr found a vector    */
								 /*  instruction.                   */
public boolean needruncmd;       /* used by rerun/reread to force run()*/
public boolean coredump;		/* true if using a core dump */
public boolean runfirst;		/* run program immediately */
public boolean interactive;		/* standard input IS a terminal */
public boolean lexdebug;		/* trace scanner return values */
public boolean tracebpts;		/* trace create/delete breakpoints */
public boolean traceexec;		/* trace execution */
public boolean traceeval;		/* trace expression evaluation */
public boolean tracesyms;		/* print symbols are they are read */
public boolean traceblocks;		/* trace blocks when reading symbols */
public boolean tracefuncs;		/* print names of functions as they */
					/*   are entered into function table */
public boolean vaddrs;			/* map addresses through page tables */
public boolean xdb;			/* xdb specific feature */
public String xdbpath;   		/* xdb specific feature */
public File corefile;			/* File id of core dump */
public char **myenvp;			/* global environ pointer */

#define FIRST_TIME 0			/* initial value setjmp returns */
#define MAXFILNAMSIZ 14			/* limit init file to 14 chars */

private Boolean initdone = false;	/* true if initialization done */
private jmp_buf env;			/* setjmp/longjmp data */
private char outbuf[BUFSIZ];		/* standard output buffer */
private char namebuf[512];		/* possible name of object file */
private int firstarg;			/* first program argument (for -r) */

private Ttyinfo ttyinfo;
private String corename;		/* name of core file */

private void catchintr();

/*
 * Main program.
 */

main(argc, argv, envp)
int argc;
String argv[];
String envp[];
{
    register integer i;
    extern String date;
    extern integer versionNumber;

	myenvp = envp;
	cmdname = argv[0];
    if (rindex(cmdname, '/') != nil) {
	cmdname = rindex(cmdname, '/') + 1;
    }
    mkinitfile();
    catcherrs();
    onsyserr(EINTR, nil);
    printf("dbx version %s of %s.\nType 'help' for help.\n",
														version, date);
    fflush(stdout);
    scanargs(argc, argv);
	Input_init();
    language_init();
	OpInit();
    symbols_init();
    process_init();
	needruncmd = false;
    if (runfirst) {
	if (setjmp(env) == FIRST_TIME) {
	    arginit();
	    for (i = firstarg; i < argc; i++) {
		newarg(argv[i]);
	    }
	    run();
	    /* NOTREACHED */
	} else {
	    runfirst = false;
	}
    } else {
	init();
    }
    if (setjmp(env) != FIRST_TIME) {
	restoretty(stdout, &ttyinfo);
    }
    signal(SIGINT, catchintr);
    yyparse();
    putchar('\n');
    quit(0);
}

/*
 * Build the name of the initialization file:
 *	. + <1st 8 chars. of cmdname> + init + \0
 * Filenames are limited to 14 characters.
 */

private mkinitfile()
{
    Char buf[MAXFILNAMSIZ];
    String prefix;
    Integer i;

    prefix = cmdname;
    if (*prefix == '\0') {
	initfile = strdup(".dbxinit");
    } else {
        buf[0] = '.';
        for (i = 1; i <= 8; i++) {
	    if (*prefix == '\0') break;
       	    buf[i] = *prefix++;
        }
        strcpy(&buf[i], "init");
        buf[i+4] = '\0';
        initfile = strdup(buf);
    }
}
    
/*
 * Initialize the world, including setting initial input file
 * if the file exists.
 */

public init()
{
    File f;
    String home;
    char buf[100];
    extern String getenv();

    savetty(stdout, &ttyinfo);
    enterkeywords();
    scanner_init();
    if (not coredump and not runfirst) {
	start(nil, nil, nil);
    }
    printf("reading symbolic information ...");
    fflush(stdout);
    readobj(objname);
    printf("\n");
    fflush(stdout);
    if (coredump) {
	printf("[using memory image in %s]\n", corename);
	if (vaddrs) {
	    coredump_getkerinfo();
	}
	setcurfunc(whatblock(pc));
	never_run = false;
    } else {
	setcurfunc(program);
    }
    bpinit(needruncmd);
    if (!xdb) {                      /* 010 - don't use .dbxinit */
        f = fopen(initfile, "r");
        if (f != nil) {
	    fclose(f);
	    setinput(initfile);
        } else {
	    home = getenv("HOME");
	    if (home != nil) {
	        sprintf(buf, "%s/%s", home, initfile);
	        f = fopen(buf, "r");
	        if (f != nil) {
		    fclose(f);
		    setinput(strdup(buf));
	        }
	    }
        }
    }  /* end if !xdb */
    initdone = true;
}

/*
 * Re-initialize the world, first de-allocating all storage.
 * This is necessary when the symbol information must be re-read
 * from the object file when it has changed.
 *
 * Before "forgetting" things, we save the current tracing/breakpoint
 * information to a temp file.  Then after re-creating the world,
 * we read the temp file as commands.  This isn't always the right thing;
 * if a procedure that was being traced is deleted, an error message
 * will be generated.
 *
 * If the argument vector is not nil, then this is re-initialize is being
 * done in preparation for running the program.  Since we want to process
 * the commands in the temp file before running the program, we add the
 * run command at the end of the temp file.  In this case, reinit longjmps
 * back to parsing rather than returning.
 */

public reinit(argv, infile, outfile)
String *argv;
String infile;
String outfile;
{
    register Integer i;
    String tmpfile;
    extern String mktemp();


    tmpfile = mktemp("/tmp/dbxXXXX");
    setout(tmpfile);
    alias(nil, nil, nil, false);
	/*    the vpdeclare command was removed lrm 9/26/90
	if(vectorcapable && vector_context())
		printf("vpdeclare\n");
	*/
	if(needruncmd){
		if (argv != nil) {
			printf("run");
			for (i = 1; argv[i] != nil; i++) {
	    		printf(" %s", argv[i]);
			}
			if (infile != nil) {
	    		printf(" < %s", infile);
			}
			if (outfile != nil) {
	    		printf(" > %s", outfile);
			}
			putchar('\n');
    	}
    }
    unsetout();
    objfree();
	OpInit();
    symbols_init();
    process_init();
    enterkeywords();
    scanner_init();
    readobj(objname);
    bpinit(needruncmd);
    fflush(stdout);
    setinput(tmpfile);
    unlink(tmpfile);
    if (argv != nil) {
	longjmp(env, 1);
	/* NOTREACHED */
    }
}

/*
 * After a non-fatal error we skip the rest of the current input line, and
 * jump back to command parsing.
 */

public erecover()
{
    if (initdone) {
	gobble();
	longjmp(env, 1);
    }
}

/*
 * This routine is called when an interrupt occurs.
 */

private void catchintr()
{
    if (isredirected()) {
	fflush(stdout);
	unsetout();
    }
    putchar('\n');
    longjmp(env, 1);
}

/*
 * Scan the argument list.
 */

private scanargs(argc, argv)
int argc;
String argv[];
{
    register int i, j;
    register Boolean foundfile;
    register File f;
    char *tmp;

    runfirst = false;
    interactive = false;
    lexdebug = false;
    tracebpts = false;
    traceexec = false;
    traceeval = false;
    tracesyms = false;
    tracefuncs = false;
    traceblocks = false;
    xdb = false; 		/* 007 - DNM */
    vaddrs = false;
    foundfile = false;
    corefile = nil;
    coredump = true;
	vectorcapable = true;
	is_vector_capable();
	found_a_vec_inst = false;
    sourcepath = list_alloc();
    list_append(list_item("."), nil, sourcepath);
    i = 1;
    while (i < argc and (not foundfile or (coredump and corefile == nil))) {
	if (argv[i][0] == '-') {
	    if (streq(argv[i], "-I")) {
		++i;
		if (i >= argc) {
		    fatal("missing directory for -I");
		}
		list_append(list_item(argv[i]), nil, sourcepath);
	    } else if (streq(argv[i], "-c")) {
		++i;
		if (i >= argc) {
		    fatal("missing command file name for -c");
		}
		initfile = argv[i];
	    } else if (streq(argv[i], "-x")) { /* 009 - DNM */
		++i;
		if (i >= argc) {
		    fatal("missing terminal emulator name for -x");
		}
                xdb = true;
                interactive = true;
		xdbpath = argv[i];
	    } else {
		for (j = 1; argv[i][j] != '\0'; j++) {
		    setoption(argv[i][j]);
		}
	    }
	} else if (not foundfile) {
	    objname = argv[i];
	    foundfile = true;
	} else if (coredump and corefile == nil) {
	    corefile = fopen(argv[i], "r");
	    corename = argv[i];
	    if (corefile == nil) {
		coredump = false;
	    }
	}
	++i;
    }
    if (i < argc and not runfirst) {
	fatal("extraneous argument %s", argv[i]);
    }
    firstarg = i;
    if (not foundfile and isatty(0)) {
		printf("enter object file name (default is `%s'): ", objname);
noobj:
		fflush(stdout);
		gets(namebuf);
		if (namebuf[0] != '\0') {
			objname = namebuf;
		} else {
			if(objname[0] == '\0') {
				exit(0);
			}
		}
    }
    f = fopen(objname, "r");
    if (f == nil) {
		printf("can't read %s\n", objname);
		strcpy(objname, "");
		printf("enter object file name (<cr> to exit): ");
		goto noobj;
    } else {
		fclose(f);
    }
    if (rindex(objname, '/') != nil) {
		tmp = strdup(objname);
		*(rindex(tmp, '/')) = '\0';
		list_append(list_item(tmp), nil, sourcepath);
    }
    if (coredump and corefile == nil) {
	if (vaddrs) {
	    corefile = fopen("/dev/mem", "r");
	    corename = "/dev/mem";
	    if (corefile == nil) {
		panic("can't open /dev/mem");
	    }
	} else {
	corefile = fopen("core", "r");
	    corename = "core";
	if (corefile == nil) {
	    coredump = false;
	}
    }
    }
}

/*
 * Take appropriate action for recognized command argument.
 */

private setoption(c)
char c;
{
    switch (c) {
	case 'r':   /* run program before accepting commands */
	    runfirst = true;
	    coredump = false;
	    break;

	case 'i':
	    interactive = true;
	    break;

#	ifdef INHOUSE		/* inhouse debugging switches */
	case 'b':
	    tracebpts = true;
	    break;

	case 'e':
	    traceexec = true;
	    break;

	case 'v':
	    traceeval = true;
	    break;

	case 's':
	    tracesyms = true;
	    break;

	case 'n':
	    traceblocks = true;
	    break;

	case 'f':
	    tracefuncs = true;
	    break;
#	endif

	case 'k':
	    vaddrs = true;
	    break;

	case 'l':
#   	    ifdef LEXDEBUG
		lexdebug = true;
#	    else
		fatal("\"-l\" only applicable when compiled with LEXDEBUG");
#	    endif
	    break;

	default:
	    fatal("unknown option '%c'", c);
    }
}

/*
 * Save/restore the state of a tty.
 */

public savetty(f, t)
File f;
Ttyinfo *t;
{
    ioctl(fileno(f), TIOCGETP, &(t->sg));
    ioctl(fileno(f), TIOCGETC, &(t->tc));
    ioctl(fileno(f), TIOCGLTC, &(t->ltc));
    ioctl(fileno(f), TIOCGETD, &(t->ldisc));
    ioctl(fileno(f), TIOCLGET, &(t->local));
    t->fcflags = fcntl(fileno(f), F_GETFL, 0);
}

public restoretty(f, t)
File f;
Ttyinfo *t;
{
    ioctl(fileno(f), TIOCSETN, &(t->sg));
    ioctl(fileno(f), TIOCSETC, &(t->tc));
    ioctl(fileno(f), TIOCSLTC, &(t->ltc));
    ioctl(fileno(f), TIOCSETD, &(t->ldisc));
    ioctl(fileno(f), TIOCLSET, &(t->local));
    (void) fcntl(fileno(f), F_SETFL, t->fcflags);
}

/*
 * Exit gracefully.
 */

public quit(r)
Integer r;
{
    pterm(process);
    exit(r);
}
