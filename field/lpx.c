
#ifndef lint
static char *sccsid = "@(#)lpx.c	4.2	ULTRIX	7/15/88";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *			Modification History
 *
 * 7/15/88 -- prs
 *      Added char string DR_LPX to allow proper use with -o option.
 *
 ************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include "diag.h"


#define MODULE		"lpx"			/* name of module */
#define LINELENGTH	132			/* print buffer size */
#define CONTROL		2			/* additional length for
						   control characters */
#define MAXPAGES	5 			/* Maximum number of pages*/
#define MAXLINES	60			/* maximum number of lines*/
#define FF		"\014"			/* ASCII form feed character */
#define LPDEV		"/dev/" 		/* device */
#define LOCK		"/usr/spool/lpd/lock"   /* path to check spooler */
#define MINUTE		60			/* minute in 1 sec counts */
#define SECOND		1 			/* one second */
#define PAUSE		15 			/* number of default pause min*/

/* help message */
static char *help[] =
{
	"\n\nLPX -- Ultrix-32 line printer exerciser\n",
	"\n",
	"Usage:\n",
	"\tlpx [-h] [-ofile] [-p#] [-t#] -ddev#\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-ofile\t(optional) Save output diagnostics in file\n",
	"-p#\t(optional) Set pause for # minutes, pause period will only exercise\n",
	"   \t controller, saving paper. (default 15, 0 no pause)\n",
	"-t#\t(optional) Run time in minutes # (default run forever until cntr-c)\n",
	"-ddev#\tLine printer to test as per /dev directory (i.e. lp, lp1)\n",
	"\n",
	"examples:\n",
	"\tlpx -dlp1\n",
	"\t{ Exercise lp1 forever }\n",
	"\tlpx -t120 -dlp &\n",
	"\t{ Exercise lp for 120 min. in the background }\n",
	"\n",
	"",
};

/*
 *	Global Variables
 */
int timedelta = 0;
long stoptime = 0;

char lpdev[14] = LPDEV;
char *lpdevp;
char errbuf[256];

char DR_LPX[] = "#LOG_LPX_01";		/* Logfile name */

main (argc,argv)
int argc;
char **argv;
{
register i,j;
register pn;				/* number of pages */
register ln;				/* number of lines per page */
int fd;
int pause;
long oldt;
void lp_clean();
static char prbuffer[LINELENGTH + CONTROL];

	/* set up 'kill' signal */
	signal(SIGTERM,lp_clean);
	signal(SIGINT,lp_clean);

	/* set default number of pause seconds */
	pause = PAUSE * MINUTE;

	if (argc == 1) {
		fprintf(stderr,"useage: lpx arg, type \"lpx -h\" for help!\n");
		exit(0);
	}

	/* handle input args */
	while (--argc > 0 && **++argv == '-')
	{
		switch (*++*argv)
		{
		case 'p':		/* calculate requested pause */
			pause = 0;
			while ((i = *++*argv) >= '0' && i <= '9')
			{
				pause *= 10;
				pause += i - '0';
			}
			pause *= MINUTE;
			break;

		case 'd':		/* specify lp */
			lpdevp = lpdev;
			bumpptr(lpdevp);
			while (*lpdevp++ = *++*argv);
			break;

		case 'o':		/* save output into file */
			fileptr = filename;
			while (*fileptr++ = *++*argv);
			break;

		case 't':		/* run time interval */
			timedelta = atoi(++*argv);
			break;

		case 'h':
			for (i =0; *help[i]; i++)
				printf("%s",help[i]);
			exit();

		}
	}
	if (lpdev[5] == '\0') {
		fprintf(stderr,"No device chosen, type \"lpx -h\" for help!\n");
		exit(0);
	}

	if (timedelta)
		stoptime = (timedelta * 60) + time(0);
	
	/* open report logger */
	if (report(DR_OPEN,MODULE,DR_LPX))
	{
		fprintf(stderr,"%s: Could not open report logger; test aborted\n",MODULE);
		exit(0);
	}

	/* check to insure that the spooler does not have the device */
/*	if (access(LOCK,F_OK) == 0)
	{
		report(DR_WRITE,MODULE,"Printer in use by spooler; waiting for print job to finish");
		do sleep(SECOND);
		while (access(LOCK,F_OK) == 0);
	}
*/
	/* attempt to open printer; if can not, try in 1 minute */
	if ((fd = open(lpdev,O_WRONLY)) == -1)
	{
		sprintf(errbuf,"Can not open %s: %s; or check for lp offline;\n	or lp queue active;	Will retry at one minute intervals!",lpdev,sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		do sleep(MINUTE);
		while ((fd = open(lpdev,O_WRONLY)) == -1);
	}

	/* start exercising */
	sprintf(errbuf,"Started lpx exerciser - testing: %s\n",lpdev + 5);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);
	
	/* perform exercise until process killed */
	forever
	{
		if (stoptime && stoptime < time(0))
			lp_clean();

		/* print rolling pattern */
		pn = MAXPAGES;
		while (pn--)
		{
			offset = 0;
			ln = MAXLINES;
			while (ln--)
			{
				pattern(DG_PRINT,LINELENGTH,prbuffer);
				write (fd,prbuffer,LINELENGTH+CONTROL);
			}
			write (fd,FF,1);
		}

		/* continue to exercise controller, but save paper */
		oldt = time(0);
		pattern(DG_NULLS,LINELENGTH,prbuffer);
		while (oldt + pause > time(0)) {
			write (fd,prbuffer,LINELENGTH);
			if (stoptime && stoptime < time(0))
				lp_clean();
		}
	}
}


void lp_clean()
{
	sprintf(errbuf,"Stopped lpx exerciser - tested: %s\n",lpdev + 5);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);

	/* close report logger */
	report(DR_CLOSE,0,DR_LPX);
	exit(0);
}
