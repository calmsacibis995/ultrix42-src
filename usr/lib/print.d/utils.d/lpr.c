#ifndef lint
static char *sccsid = "@(#)lpr.c	4.2	ULTRIX	9/11/90";
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


/***************************************************************
 *                lpr -- off line print                        *
 *                                                             *
 * Allows access to multiple printers and printers on remote   *
 * machines by using information from a printer data base.     *
 ***************************************************************/

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  01/11/83 -- sccs
 * date and time created 83/11/01 20:58:05 by sccs
 * 
 * ***************************************************************
 * 
 * 1.2  11/12/84 -- root
 * added Ultrix id keywords - lp
 * 
 * 
 * ***************************************************************
 * 
 * 1.3  11/12/84 -- root
 * merged into library - lp
 * several additions by williams of:
 * add of ditroff filter call
 * fix sticky spooler control file param
 * add PP print replacement filter
 * add -z option for regulating page length
 * 
 * 
 * ***************************************************************
 * 
 * 1.4  02/10/86 -- root
 * Comments taken from: 1.4:
 * *** Working Pool Statistics ***: 1.4 86/07/08 17:19:16 hoffman 00001/00000/00646
 * 
 * Added pass-thru filter parameter xf
 * 
 * ***************************************************************
 * 
 * 1.5  08/01/88 -- maxwell
 * user interface rewritten using getopt(3c) - now checks for invalid options
 * and missing arguments.
 * Functionally unchanged
 * 
 * 
 * ***************************************************************
 * 
 * 1.6  13/01/88 -- maxwell
 * added new lps40 options: -D -I -F -O -L -N -S -R -M
 * tightened up command line error testing
 * 
 * 
 * ***************************************************************
 * 
 * 1.7  27/01/88 -- maxwell
 * Added multiple filter test
 * 
 * 
 * ***************************************************************
 * 
 * 1.8  11/02/88 -- maxwell
 * 
 * 
 * ***************************************************************
 * 
 * 1.9  05/05/88 -- maxwell
 * added table to map command line options to command file
 * 
 * 
 * ***************************************************************
 * 
 * 1.10  06/05/88 -- thoms
 * Fixed PostScript parameter strings
 * Fixed message and number up command file letters to E and G
 * 
 * ***************************************************************
 * 
 * 1.11  13/05/88 -- maxwell
 * added code to spool layup file
 * made minor changes to order of control file entries
 * 
 * ***************************************************************
 * 
 * 1.12  16/05/88 -- maxwell
 * put arg strings for command line into separate module (argstrings.c)
 * allow -i0 (zero length indent)    
 * 
 * ***************************************************************
 * 
 * 1.13  17/05/88 -- thoms
 * Put 'N' line in command file after data files (else lpq fouls up)
 * 
 * ***************************************************************
 * 
 * 1.14  31/05/88 -- maxwell
 * Minor changes to error messages
 * 
 * ***************************************************************
 * 
 * 1.15  21/07/88 -- maxwell
 * Changes as per code review:
 * Added more comments
 * Tidied up exit codes - added warn() routine to pass warning messages to stderr
 * replaced '?' switch in case with 'default'
 * made changes to keep lint happy
 * 
 * ***************************************************************
 *
 * 1.16  21/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * ***************************************************************
 *
 * 1.17  05/08/88 -- maxwell
 * changed n-up range to 0-100
 *
 * ***************************************************************
 *
 * 1.18  21/07/88 -- maxwell
 * put back unique abbre code for data_types
 *
 * ***************************************************************
 *
 * 1.19  12/08/88 -- maxwell
 * search for layup files in LUPDIR if rel path name and not in current dir
 * 
 * ***************************************************************
 *
 * 1.20 16/08/88 -- maxwell
 * rationalised handling of numerical args - added valid_num()
 * fixed syntax of -Z (e.g. -Z 4 now equals -Z4,)
 * corrected usage message for -Z
 *
 * ****************************************************************
 *
 * 1.21  10/11/88 -- thoms
 * Removed #include of a.out.h, lpr.c now includes exec.h instead
 *
 * ****************************************************************
 * 1.22  20/08/90 -- atkinson
 * Modifications to block symbolic link tricks.
 * Name symbolic links the same way as copies
 *
 * SCCS history end
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/exec.h>
#include <pwd.h>
#include <signal.h>
#include <ctype.h>
#include "lp.local.h"
#include "argstrings.h"

#define LUPDIR  "/usr/lib/lpdfilters/"

char	*name;			/* program name */

/* global variables */

static char    *tfname;		/* tmp copy of cf before linking */
static char    *cfname;		/* daemon control files, linked from tf's */
static char    *dfname;		/* data files */

static struct stat statb;	/* Used by test() and for symbolic links */

static int	tfd;		/* control file descriptor */
static int	userid;		/* user id */
static char	host[32];	/* host name */
static int	nact;		/* number of jobs to act on */
static char	format = 'f';	/* format char for printing files (default plain text) */
static int	inchar;		/* location to increment char in file names */
static int	ncopies = 1;	/* # of copies to make */
static char	*title;		/* pr'ing title */
static char	buf[BUFSIZ];

static int	MX;		/* maximum number of blocks to copy */
static int	MC;		/* maximum number of copies allowed */
static int	DU;		/* daemon user-id */
static char	*SD;		/* spool directory */
static char	*LO;		/* lock file name */
static short	SC;		/* suppress multiple copies */

/* external functions */

extern char	*getenv();
extern char	*getwd();
extern char	*rindex();
extern char	*malloc();
extern char	*index();

/* forward declarations */

static char	*linked();
static void	cleanup();
static void	chkprinter();
static void	mktemps();
static char 	*itoa();
int	check_arg();
static int	valid_num();
static void	invalid_arg();
static void	usage();
static void	warn();
static void	fatal();
static void	nextname();

/*******************************************************************
 * main
 * parse and validate command line, create control file in spool dir, 
 * copy/symlink data file to spool dir and start print deamon
 *******************************************************************/

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char *argv[];
{
	static char *options =
		"cdfghi:lmnpqrstvw:xz:C:P:J:T:#:1:2:3:4:D:I:o:O:F:Z:X:S:M:N:L:K:";

	extern struct passwd *getpwuid();
	extern int getopt();
	extern int optind;
	extern char *optarg;
	register char *arg, *cp;
	int num, i, opt, lifd;
	struct passwd *pw;
	struct stat stb;

	int	mailflg = 0;		/* send mail */
	int	qflag = 0;		/* q job but don't exec daemon (undocumented option) */
	int	sflag = 0;		/* symbolic link flag */
	int	hdr = 1;		/* print header or not (default is yes) */
	int	rflag = 0;		/* remove files upon completion */	
	int	remove = 0;		/* can user remove file */
	char	*data_type = NULL;	/* name of data_type */
	char 	*input_tray = NULL;	/* name of input_tray */
	char	*output_tray = NULL;	/* name of output_tray */
	char	*orientation = NULL;	/* name of orientation */
	char	*page_size = NULL;	/* name of page_size */
	char	*nsheets = NULL;	/* num of sheets to print */
	char    *numberup = NULL;	/* # for number_up */
	char    *layup = NULL;		/* name of layup defn file */
	char    *sides = NULL;		/* name of sides */
	char	*lower = NULL,		/* range of sheets to print */
		*upper = NULL;
	char	*sheet_size = NULL;	/* name of sheet_size */
	char	*message = NULL;	/* name of message */
	char	*indent = NULL;		/* amount to indent (default is 0) */
	char	*person = NULL;		/* user name */
	char	*fonts[4];		/* troff font names */
	char	*width = NULL;		/* width for printing */
	char	*length = NULL;		/* length for printing */
	char	*class = host;		/* class title on header page */
	char	*jobname = NULL;	/* job name on header page */
	char	*printer = NULL;	/* printer name */


	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, cleanup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, cleanup);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGQUIT, cleanup);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void) signal(SIGTERM, cleanup);

	name = argv[0];
	(void)gethostname(host, sizeof (host));
	init_args();

	/* parse options */
	/* note numerical arguments are stored as strings so that only */
	/* one card fn is required */
	while ((opt = getopt(argc, argv, options)) != EOF ) {

		switch (opt) {

		/* deal with filters */

		case 'c':		/* print cifplot output */
		case 'd':		/* print tex output (dvi files) */
		case 'g':		/* print graph(1G) output */
		case 'l':		/* literal output */
		case 'n':		/* print ditroff output */
		case 'p':		/* print using ``pr'' */
		case 't':		/* print troff output (cat files) */
		case 'v':		/* print vplot output */
		case 'x':		/* print via pass-thru filter */
			format = opt;
			break;

		case 'f':      		/* print FORTRAN output (f is used for text files) */
			format = 'r';
			break;

		case 'h':		/* suppress header page */
			hdr = 0;
			break;

		case 'i':		/* indent output */
			if (valid_num(optarg,1,-1) ==  0)
				indent = optarg;
			else
				fatal("invalid argument %s - must be number > 0",optarg);
			break;

		case 'm':		/* send mail when done */
			mailflg++;
			break;

		case 'r':		/* remove file when done */
			rflag++;
			break;

		case 's':		/* try to link files */
			sflag++;
			break;

		case 'w':		/* versatec page width */
			if (valid_num(optarg,0,-1) == 0)
				width = optarg;
			else
				fatal("invalid argument %s - must be number > 0",optarg);
			break;

		case 'z':		/* page length */
			if (valid_num(optarg,0,-1) == 0)
				length=optarg;
			else
				fatal("invalid argument %s - must be number > 0",optarg);
			break;

		case 'C':		/* classification spec */
			hdr++;		/* override header suppression */
			class = optarg;
			break;

		case 'J':		/* job name */
			hdr++;		/* override header suppression */
			jobname = optarg;
			break;

		case 'P':		/* specifiy printer name */
			printer = optarg;
			chkprinter(printer);
			break;

		case 'T':		/* pr's title line */
			title = optarg;
			break;

		case '1':		/* troff fonts */
		case '2':
		case '3':
		case '4':
			fonts[opt - '1'] = optarg;
			break;

		case 'q':		/* just q job */
			qflag++;
			break;

		case '#':		/* multiple copies */
			if ((num = atoi(optarg)) > 0)
				ncopies = num;
			else
				fatal("invalid argument %s - must be number > 0",optarg);
			break;

		case 'D':		/* data type */
			switch (check_arg(optarg,as_data_types,&data_type)) {
			case 0:         /* match with predefined data_type */
				break;
			case -1:        /* ambiguous data type */
				invalid_arg("D",as_data_types);
			case -2:        /* no match so assume user data_type */
				data_type = optarg;
				break;
			}
			break;
		case 'I':		/* input tray */
			if  (check_arg(optarg,as_input_trays,&input_tray) != 0)
				invalid_arg("I",as_input_trays);
			break;

		case 'o':		/* output tray */
			if (check_arg(optarg,as_output_trays,&output_tray) != 0)
				invalid_arg("o",as_output_trays);
			break;

		case 'O':		/* orientation */
			if (check_arg(optarg,as_orientations,&orientation) != 0)
				invalid_arg("O",as_orientations);
			break;

		case 'F':		/*page size */
			if  (check_arg(optarg, as_page_sizes, &page_size) != 0)
				invalid_arg("F", as_page_sizes);
			break;

		case 'Z':		/* page limit */
			if ((num = valid_range(optarg, &lower,&upper)) != 0) {
				if (num == 2)  		/* second param must be wrong */
					optarg = upper;
				fatal("invalid argument %s - must be number in range 1-%d", optarg,MAXARG);
				}
			break;

		case 'X':		/* sheet count */
			if (valid_num(optarg,0,MAXARG) == 0)
				nsheets = optarg;
			else
				fatal("invalid argument %s - must be number in range 1-%d", optarg,MAXARG);
			break;


		case 'S':		/* sheet size */
			if  (check_arg(optarg, as_page_sizes, &sheet_size) != 0)
				invalid_arg("S", as_page_sizes);
			break;

		case 'M':		/* message log */
			if  (check_arg(optarg, as_messages, &message) != 0)
				invalid_arg("M", as_messages);
			break;


		 case 'N':		/* number up */
			if (valid_num(optarg,0,100) == 0)
				numberup = optarg;
			else
				fatal("invalid argument %s - must be number in range 0-100", optarg);
			break;

		 case 'L':		/* layup defn file */
			layup = optarg;
			break;

		 case 'K':		/* sides */
			if  (check_arg(optarg, as_sides, &sides) != 0)
				invalid_arg("K", as_sides);
			break;

		 default:		/* invalid opt or no arg */
			usage();
		}
	}
	if (printer == NULL){ 
		if ((printer = getenv("PRINTER")) == NULL)
			printer = DEFLP;
		chkprinter(printer);
	}
	if (SC && ncopies > 1)
		fatal("multiple copies are not allowed");
	if (MC > 0 && ncopies > MC)
		fatal("only %d copies are allowed", MC);
	/*
	 * Get the identity of the person doing the lpr using the same
	 * algorithm as lprm. 
	 */
	userid = getuid();
	if ((pw = getpwuid(userid)) == NULL)
		fatal("Who are you?");
	person = pw->pw_name;
	/*
	 * Check to make sure queuing is enabled if userid is not root.
	 */
	(void) sprintf(buf, "%s/%s", SD, LO);
	if (userid && stat(buf, &stb) == 0 && (stb.st_mode & 010))
		fatal("Printer queue is disabled");
	/*
	 * Initialize the control file.
	 */
	mktemps();
	tfd = nfile(tfname);
	(void) fchown(tfd, DU, -1);	/* owned by daemon for protection */
	card('H', host);
	card('P', person);
	if (hdr) {
		if (jobname == NULL) {
			if (optind == argc)
				jobname = "stdin";
			else                   /* extract filename in path */
				jobname = (arg = rindex(argv[optind], '/')) ? arg+1 : argv[optind];
		}
		card('J', jobname);
		card('C', class);
		card('L', person);
	}
	if (indent)
	card('I', indent);
	if (mailflg)
		card('M', person);
	if (format == 't' || format == 'n' || format == 'd')
		for (i = 0; i < 4; i++)
			if (fonts[i] != NULL)
				card('1'+i, fonts[i]);
	if (width)
		card('W', width);
	if (length)
		card('Z', length);
	if (data_type)
		card('D', data_type);
	if (input_tray)
		card('<', input_tray);
	if (output_tray)
		card('>', output_tray);
	if (orientation)
		card('O', orientation);
	if (page_size)
		card('F', page_size);
	if (sheet_size)
		card('S', sheet_size);
	if (message)
		card('E', message);
	if (nsheets)
		card('X', nsheets);
	if (lower)
		card('A', lower);
	if (upper)
		card('B', upper);
	if (numberup)
	        card('G', numberup);
	if (sides)
	        card('K', sides);

	/*
	 * Spool layup file if specified
	 */


	if (layup) {
		char *tmpstr = (char *)malloc(strlen(layup) + strlen(LUPDIR) + 1);
		strcpy(tmpstr,layup);
		if ((i = open(layup, O_RDONLY)) < 0 )   /* check if file exists as given */
			if (layup[0] == '/')
				fatal("cannot access file %s", layup);
			else {                          /* rel pathname so check LUPDIR */
				strcpy(tmpstr,LUPDIR);
				if ((i = open(strcat(tmpstr,layup), O_RDONLY)) < 0 )
					fatal("cannot access file %s", layup);
			}
		copy(i, tmpstr, 'z');                   /* copy found layup file to spooldir */
		(void) close(i);
		free(tmpstr);
	}


	/*
	 * Read the files and spool them.
	 */

	if (optind == argc)  {                      /* if no files, use stdin */
		if (format == 'p')
			card('T', title ? title : " ");
		copy(0, " ", format);
	} else for (; optind < argc; optind++) {    /* loop through files */
		if (test(arg = argv[optind]) < 0)   /* file unprintable */
			continue;	            /* try next file */

		/*
		 * check if file can be removed
		 */

		if (rflag) {
			remove = 1;
			if (access(arg, W_OK) < 0) {     /* no write access to file */
				remove = 0;
			} else {
				if ((cp = rindex(arg, '/')) == NULL) {    /* file is in current dir */
					if (access(".", W_OK) < 0) {       /* check access to current dir */
						remove = 0;
					}
				} else {
					*cp = '\0';    /* extract dir and check access */
					i = access(arg, W_OK);
					*cp = '/';
					if (i < 0) {
						remove = 0;
					}
				}
			}
			if (!remove)
				warn("%s is read only - not removed", arg);
		}

		if (format == 'p') {              /* use title if given, otherwise use filename */
			card('T', title ? title : arg);
		}

		/*
		 * deal with symbolic link
		 */

		if (sflag) {
			if ((cp = linked(arg)) != NULL) {
				for (i = 0; i < ncopies; i++)
					card(format, &dfname[inchar-2]);
				card('N', arg);
				card('U', &dfname[inchar-2]);
				if (remove)
					card('U', cp);

				/* Symbolic links pointing out of the spool
				 * directory are a security risk.
				 * Who knows what they are pointing at?
				 * Record details of the real file in the
				 * spool area to be checked later by lpd.
				 * To keep the control file compatible with
				 * other ULTRIX versions this information goes
				 * into a separate control file.  The name of
				 * this file is the name of the data file
				 * with the first character in upper case.
				 * See nextname() to see why that works.
				 * (Bsd uses an 'S' line in the control
				 *  file but we already use that.)
				 */

				dfname[inchar-2] = _toupper(dfname[inchar-2]);
				i = umask(0);
				if ((lifd = open(dfname,
						 O_WRONLY|O_EXCL|O_CREAT, 
						 FILMOD)) < 0)
					fatal("cannot create %s\n", dfname);
				(void) umask(i);
				fchown(lifd, DU, -1);

				/* Data left in statb by test() */

				sprintf(buf, "I%d\nD%d\n",
					statb.st_ino, statb.st_dev);
				i = strlen(buf);
				if (write(lifd, buf, i) != i)
					fatal("cannot write %s\n", dfname);
				close(lifd);
				card('U', &dfname[inchar-2]);
				dfname[inchar-2] = _tolower(dfname[inchar-2]);
				
				nextname(dfname);
				nact++;
				continue;
			}
			else
				warn("%s not linked, copying instead", arg);
		}
		if ((i = open(arg, O_RDONLY)) < 0) {
			warn("cannot open %s", arg);
			continue;
		}

		copy(i, arg, format);
		(void) close(i);
		if (remove && unlink(arg) < 0)
			warn("%s not removed", arg);
	}

	if (nact) {                     /* num files to act on > 0 */
		(void) close(tfd);
		tfname[inchar]--;
		/*
		 * Touch the control file to fix position in the queue.
		 */
		if ((tfd = open(tfname, O_RDWR)) >= 0) {
			char c;

			if ((read(tfd, &c, 1) == 1) && (lseek(tfd, 0L, 0) == 0) &&
			(write(tfd, &c, 1) != 1)) {
				tfname[inchar]++;
				fatal("cannot touch file %s", tfname);
			}
			(void) close(tfd);
		}
		if (link(tfname, cfname) < 0) {
			tfname[inchar]++;
			fatal("cannot rename file %s", cfname);
		}
		(void) unlink(tfname);
		if (!qflag)             /* exec daemon */
			if (!startdaemon(printer))
				warn("jobs queued, but cannot start daemon");
		exit(0);               /* SUCCESSFUL COMPLETION */
	}
	cleanup();
	/* NOTREACHED */
} /* end of main */

/*******************************************************************/
/* validate numerical argument and check range                     */
/* assumes 0<= r1 <= r2 otherwise range not checked                */
/* return 0 if ok or 1 otherwise                                   */
/*******************************************************************/
static int valid_num(str,r1,r2)
char *str;
int r1,r2;
{
	int num;

	if ((((num = atoi(str)) > 0) || ((num = strcmp(str,"0")) == 0)) &&
	(strlen(str) == strlen(itoa(num))) &&
	(r1 < 0 || num >= r1) &&
	(r2 < 0 || num <= r2))
	{
		return(0);
	}
	else
	{
		return(1);
	}
}

/*******************************************************************/
/* parse and validate range argument [n][,[m]]                       */
/* must do two things: find the two args if they exist then check  */
/* are valid numbers.                                              */
/* pointers to lower and upper limits are returned in arg_1 and    */
/* arg_2 resp.                                                     */
/* returns 0 - if one or more args exist and is/are valid          */
/*	   1 - if lower is invalid                                 */
/*	   2 - if upper is invalid                                 */
/*******************************************************************/

static int valid_range(arg_str, arg_1, arg_2)
char *arg_str, **arg_1, **arg_2;
{
	char *ptr;

	/* find args in string */
	*arg_1 = *arg_2 = NULL;
	if ((ptr = index(arg_str, ',')) == NULL)   /* no comma found */
		*arg_1 = arg_str;
	else {
		if (ptr != arg_str)               /* first arg exits */
			*arg_1 = arg_str;
		*ptr = NULL;		          /* split string */
		if (*(++ptr))                     /* second arg exits */
			*arg_2 = ptr;
	}

	/* check if valid nums */
	if (*arg_1 == NULL && *arg_2 == NULL)               /* neither arg exists */
		return(1);
	if (*arg_1 && valid_num(*arg_1,1,MAXARG) != 0)      /* arg_1 exists and is invalid */
		return(1);
	if (*arg_2 && valid_num(*arg_2,1,MAXARG) != 0)      /* arg_2 exists and is invalid */
		return(2);

	return(0);
}

/*****************************************************************/
/* Copy letter and argument into the control file
/* assumes args will fit int buf
/*****************************************************************/

static int card(c, p2)
	register char c, *p2;
{
	register char *p1 = buf;
	register int len = 2;      /* allow for char at start and NULL at end */

	*p1++ = c;
	while ((c = *p2++) != '\0') {
		*p1++ = c;
		len++;
	}
	*p1++ = '\n';
	(void) write(tfd, buf, len);
}


/************************************************************************/
/* Create the data file dfname in the spool directory with format letter fmt
/* and copy file named n into it using file descriptor f 
/* add file specific entries to control file                            */
/************************************************************************/

static int copy(f, n, fmt)
	int f;
	char n[];
	char fmt;
{
	register int fd, i, nr, nc;

	for (i = 0; i < ncopies; i++)
		card(fmt, &dfname[inchar-2]);
	card('N', n);
	card('U', &dfname[inchar-2]);

	fd = nfile(dfname);
	nr = nc = 0;
	while ((i = read(f, buf, BUFSIZ)) > 0) { /* copy to tmp file in spool dir */
		if (write(fd, buf, i) != i) {
			warn("%s temp file write error - file truncated to %d bytes", n, nr * BUFSIZ + nc);
			break;
		}
		nc += i;
		if (nc >= BUFSIZ) {
			nc -= BUFSIZ;
			nr++;
			if (MX > 0 && nr > MX) {/* check buffer max */
				warn("%s file too large - truncated to %d bytes", n, nr * BUFSIZ);
				break;
			}
		}
	}
	(void) close(fd);
	if (nc==0 && nr==0) 
		warn("%s empty input file", f ? n : "stdin");
	else
		nact++;
}

/*****************************************************************/
/* Try and link the file to dfname. Return a pointer to the full */
/* path name if successful.                                      */
/*****************************************************************/

static char * linked(file)
	register char *file;
{
	register char *cp;
	static char buf[BUFSIZ];

	if (*file != '/') {                     /* relative path name */
		if (getwd(buf) == NULL)
			return(NULL);
		while (file[0] == '.') {
			switch (file[1]) {
			case '/':               /* ignore './' */
				file += 2;
				continue;
			case '.':               /* '../' so strip top level off dir */
				if (file[2] == '/') {
					if ((cp = rindex(buf, '/')) != NULL)
						*cp = '\0';
					file += 3;
					continue;
				}
			}
			break;
		}
		(void) strcat(buf, "/");
		(void) strcat(buf, file);
		file = buf;
	}
	return(symlink(file, dfname) ? NULL : file);
}

/*****************************************************************/
/* Create a new file in the spool directory.                     */
/*****************************************************************/

static int nfile(n)
	char *n;
{
	register f;
	int oldumask = umask(0);		/* should block signals */

	f = creat(n, FILMOD);
	(void) umask(oldumask);
	if (f < 0) {
		fatal("cannot create %s", n);
	}
	if (fchown(f, userid, -1) < 0) {
		fatal("cannot chown %s", n);
	}
	nextname(n);
	return(f);
}

/*****************************************************************/
/* Create the next name in a family.				 */
/*****************************************************************/

static void nextname(n)
	char *n;
{
	if (++n[inchar] > 'z') {
		if (++n[inchar-2] == 't') {   /* this is a tmp file */
			fatal("too many files - break up the job");
		}
		n[inchar] = 'A';
	} else if (n[inchar] == '[')          /* char after 'Z' */
		n[inchar] = 'a';
}

/*****************************************************************/
/* Test to see if this is a printable file.                      */
/* Return -1 if it is not, 0 if it is                            */
/*****************************************************************/

static int test(file)
	char *file;
{
	struct exec execb;
	register int fd;

	if (access(file, R_OK) < 0) {
		warn("cannot access file %s", file);
		return(-1);
	}
	if (stat(file, &statb) < 0) {
		warn("cannot stat file %s", file);
		return(-1);
	}
	if ((statb.st_mode & S_IFMT) == S_IFDIR) {
		warn("%s is a directory", file);
		return(-1);
	}
	if (statb.st_size == 0) {
		warn("%s is an empty file", file);
		return(-1);
 	}
	if ((fd = open(file, O_RDONLY)) < 0) {
		warn("cannot open file %s", file);
		return(-1);
	}
	if (read(fd, (char *) &execb, sizeof(execb)) == sizeof(execb))
		switch(execb.a_magic) {
		case A_MAGIC1:
		case A_MAGIC2:
		case A_MAGIC3:
#ifdef A_MAGIC4
		case A_MAGIC4:
#endif
			warn("%s is an executable program and is unprintable", file);
			(void) close(fd);
			return(-1);

		case ARMAG:
			warn("%s is an archive file and is unprintable", file);
			(void) close(fd);
			return(-1);
		}
	(void) close(fd);
	return(0);

}

/*****************************************************************/
/* Perform lookup for printer name or abbreviation --            */
/*****************************************************************/

static void chkprinter(s)
	char *s;
{
	int status;
	static char pbuf[BUFSIZ/2];
	char *bp = pbuf;
	extern char *pgetstr();

	if ((status = pgetent(buf, s)) < 0)
		fatal("cannot open printer description file");
	else if (status == 0)
		fatal("unknown printer %s", s);
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((MX = pgetnum("mx")) < 0)
		MX = DEFMX;
	if ((MC = pgetnum("mc")) < 0)
		MC = DEFMAXCOPIES;
	if ((DU = pgetnum("du")) < 0)
		DU = DEFUID;
	SC = pgetflag("sc");
}

/*****************************************************************/
/* Assemble the spool file names using the contents of .seq      */
/*****************************************************************/

static void mktemps()
{
	register int len, fd, n;
	register char *cp;
	char *mktemp();

	(void) sprintf(buf, "%s/.seq", SD);
	if ((fd = open(buf, O_RDWR|O_CREAT, 0661)) < 0) {
		fatal("cannot create %s", buf);
	}
	if (flock(fd, LOCK_EX)) {
		fatal("cannot lock %s", buf);
	}
	n = 0;
	if ((len = read(fd, buf, sizeof(buf))) > 0) { /* read .seq and convert to int */
		for (cp = buf; len--; ) {
			if (*cp < '0' || *cp > '9')
				break;
			n = n * 10 + (*cp++ - '0');
		}
	}
	len = strlen(SD) + strlen(host) + 8;
	tfname = mktemp("tf", n, len);
	cfname = mktemp("cf", n, len);
	dfname = mktemp("df", n, len);
	inchar = strlen(SD) + 3;
	n = (n + 1) % 1000;                           /* inc n and write back to .seq */
	(void) lseek(fd, 0L, 0);
	(void) sprintf(buf, "%03d\n", n);
	(void) write(fd, buf, strlen(buf));
	(void) close(fd);	                     /* unlocks as well */
}

/*****************************************************************/
/* Make a temp file name of lenght len
/*****************************************************************/

static char * mktemp(id, num, len)
	char	*id;
	int	num;
	int 	len; 
{
	register char *s;

	if ((s = malloc((unsigned)len)) == NULL)
		fatal("out of memory");
	(void) sprintf(s, "%s/%sA%03d%s", SD, id, num, host);
	return(s);
}

/*****************************************************************/
/* itoa - integer to string conversion                           */
/*****************************************************************/

static char * itoa(i)
	register int i;
{
	static char b[10] = "########";
	register char *p;

	p = &b[8];
	do
		*p-- = i%10 + '0';
	while (i /= 10);
	return(++p);
}

/*****************************************************************/
/* Print usage message on stderr and exit
/*****************************************************************/
static void usage()
{
	(void) fprintf(stderr,"Usage: %s %s%s%s%s%s%s%s", name,
	"[-Pprinter] [-#num] [-Cclass] [-Jjob] [-Ttitle]\n",
	"           [-inum] [-1234font] [-wnum] [-znum] [-Ddata_type]\n",
	"           [-Iinput_tray] [-ooutput_tray] [-Oorientation]\n",
	"           [-Fpage_size] [-Z[lo_page_lim][,hi_page_lim]\n",
	"           [-Xsheet_count] [-Ssheet_size] [-Mmessage]\n",
	"           [-Nnumber_up] [-Llayup_definition] [-Ksides]\n",
	"           [-cdfghlmnprstvx] [filename...]\n");
	exit (2);
}

/*****************************************************************/
/* Cleanup after interrupts and errors.                          */
/*****************************************************************/

static void cleanup()
{
	register i;

	(void) signal(SIGHUP, SIG_IGN);        /* ignore further interrupts */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
	i = inchar;
	if (tfname)                            /* remove files from spool dir */
		do
			(void) unlink(tfname);
		while (tfname[i]-- != 'A');
	if (cfname)
		do
			(void) unlink(cfname);
		while (cfname[i]-- != 'A');
	if (dfname)
		do {
			do
				(void) unlink(dfname);
			while (dfname[i]-- != 'A');
			dfname[i] = 'z';
		} while (dfname[i-2]-- != 'd');
	exit(1);
}

/*****************************************************************/
/* invalid argument - print error messege + valid args and exit  */
/*****************************************************************/

#define linewidth 30

static void invalid_arg(opt, opt_num)
char *opt;
int opt_num;
{

	struct arg_pair *arg_list;
	int width=0;

	get_args(opt_num,&arg_list);
	warn("invalid argument for %s - must be one of:", opt);
	(void) fprintf(stderr,"        ");

	while (arg_list->arg) {
		(void) fprintf(stderr, "%s ", arg_list->arg);

		width += strlen(arg_list->arg);

		if (width > linewidth){               /* throw newline */
			(void) fprintf(stderr, "\n        ");
			width = 0;
		}

		arg_list++;
	}
	(void) fputc('\n', stderr);
	exit(1);
}
	
/*****************************************************************/
/* non-fatal error - print warning message on stderr
/*****************************************************************/

/*VARARGS1*/
static void warn(msg, a1, a2, a3)
char *msg;
{
	(void) fprintf(stderr, "%s: ", name);
	(void) fprintf(stderr, msg, a1, a2, a3);
	(void) fputc('\n', stderr);
}

/*****************************************************************/
/* fatal error - print message on stderr cleanup spool dir and exit
/*****************************************************************/

/*VARARGS1*/
static void fatal(msg, a1, a2, a3)
char *msg;
{
	(void) fprintf(stderr, "%s: ", name);
	(void) fprintf(stderr, msg, a1, a2, a3);
	(void) fputc('\n',stderr);
	cleanup();
}
