
#ifndef lint
static char *sccsid = "@(#)fsx.c	4.2	ULTRIX	7/15/88";
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
 *      Added char string DR_FSX to allow proper use with -o option.
 *
 ************************************************************************/
/*
 *
 *	FSX.C	--  This routine will set up the file system test, and spawn the
 *		    processes that actually perform the exercising
 *
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <nlist.h>
#include "diag.h"

#define MODULE		"fsx"			/* name of module */
#define MINUTE		60			/* minute in 1 sec counts */
#define SECOND		1 			/* one second */

char *help[] =
{
	"\n\n(fsx) - ULTRIX-32 file system exerciser\n",
	"\n",
	"usage:\n",
	"\tfsx [-h] [-p#] [-fpath] [-t#] [-ofile]\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-ofile\t(optional) Save output in file\n",
	"-p#\t(optional) Number of processes to be spawned (1-250 default: 20)\n",
	"-t#\t(optional) Run time in minutes (default: run forever until cntl-c\n",
	"-fpath\t(optional) Path name of directory on mounted f.s. you wish\n",
	"      \tto test (ie. /usr or /mnt etc, default /usr/field)\n",
	"\n",
	"examples:\n",
	"\tfsx -p10\n",
	"\t{ Run 10 fsx processes (default path=/usr/field) forever }\n",
	"\tfsx -t180 -f/mnt &\n",
	"\t{ Run 20(default) fsx processes for 180 min. path=/mnt in background }\n",
	"\n",
	"",
};

int pid[DF_MAXPROC];			/* array of process id for fsxr
					   routines spawned */
int pid2[DF_MAXPROC];
union wait status[DF_MAXPROC];

char errbuf[512];			/* buffer for error message */

char path[128];
char *pathptr;
/* run time variables */
int timedelta;

char DR_FSX[] = "#LOG_FSX_01";		/* Logfile name */

main (argc,argv)
int argc;
char **argv;
{
register i,j;
int maxproc;
int numproc;
int sig;
void fsx_clean();
char procid[10];
char clogfd[10];
char time[10];

	/* set up kill signal */
	signal(SIGINT,fsx_clean);
	signal(SIGTERM,fsx_clean);

	/* handle input args */
	numproc = 0;
	maxproc = DF_DEFMAXPROC;
	while (--argc > 0 && **++argv == '-')
	{
		switch (*++*argv)
		{

		case 'h':	/* print help message */
			for (i = 0; *help[i]; i++)
				printf("%s",help[i]);
			exit(0);

		case 'o':	/* save output into file */
			fileptr = filename;
			while (*fileptr++ = *++*argv);
			break;

		case 'p':	/* restrict number of processes spawned */
			maxproc = atoi(++*argv);
			if (maxproc < 1)
				maxproc = 1;
			if (maxproc > DF_MAXPROC)
				maxproc = DF_MAXPROC;
			break;

		case 't':
			timedelta = atoi(++*argv);
			break;

		case 'f':
			pathptr = path;
			while (*pathptr++ = *++*argv);
			if (path[0] != '\0' && path[1] != '\0') {
			    --pathptr;
			    if (*(--pathptr) == '/')
			        *pathptr = '\0';
			}
			break;
		default:
			printf("fsx: Invalid arg %s, type \"fsx -h\" for help\n",*argv);
			exit(0);
		}
	}

	/* open logger */
	if (report(DR_OPEN,MODULE,DR_FSX))
	{
		fprintf(stderr,"%s: Can not start report generator, test aborted\n", MODULE);
		exit(0);
	}

	if (path[0] == '\0')
	    strcpy(path,"/usr/field");

	/* fork and exec fsxr processes */
	for (i = 0; i < maxproc; i++)
	{
		if ((pid[i] = fork()) == 0)
		{
			sprintf(procid,"%d",i + 1);
			sprintf(clogfd,"%d",logfd);
			sprintf(time,"%d",timedelta);
			if (execl("fsxr","fsxr",path,procid,clogfd,time,0) < 0)
			{
				sprintf(errbuf,"Could not execl fsxr%d %s",
				  i+1,sys_errlist[errno]);
				report(DR_WRITE,MODULE,errbuf);
			}
			exit(0);
		}

		if (pid[i] == -1)
		{
			sprintf(errbuf,"Could not fork fsxr%d %s",
				i+1,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			continue;
		}

		sprintf(errbuf,"Exerciser Process fsxr%d pid %d Started",i+1,pid[i]);
		report(DR_WRITE,MODULE,errbuf);
		numproc++;
	}
	sprintf(errbuf,"Started fsx exerciser - on %s", path);
	(void)logerr(ELMSGT_DIAG,errbuf);

	for(i = 0; i < numproc; i++)
		pid2[i] = wait(&status[i]);

	for ( i = 0; i < numproc; i++) {
		for ( j = 0; pid2[i] != pid[j] && j < numproc; j++) ;
		sig = status[i].w_termsig;
		sprintf(errbuf,"Process Termination fsxr%d pid %d Status %d %s",j+1,pid2[i],sig,sys_siglist[sig]);
		report(DR_WRITE,MODULE,errbuf);
	}
	sprintf(errbuf,"Stopped fsx exerciser - on %s", path);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_CLOSE,0,DR_FSX);
	exit(0);

}


void fsx_clean()
{
register i;

	for(i = 0; i < DF_MAXPROC; i++)
		if (pid[i] > 0)
			kill (pid[i],SIGINT);

}
