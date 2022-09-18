
#ifndef lint
static char *sccsid = "@(#)memx.c	4.1	ULTRIX	7/2/90";
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
 *      Added char string DR_MEMX to allow proper use with -o option.
 *
 ************************************************************************/
/*
 *
 *	MEMX.C	--  This routine will set up the memory test, and spawn the
 *		    processes that actually perform the memory exercising
 *
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <nlist.h>
#include "diag.h"

#define MODULE		"memx"			/* name of module */
#define MINUTE		60			/* minute in 1 sec counts */
#define HOUR		60*MINUTE
#define SECOND		1 			/* one second */
#define ZZZZZZ		HOUR

struct nlist nl[] =
{
	{"_maxmem"},
	{"_physmem"},
	{""},
};

char *help[] =
{
	"\n\n(memx) - ULTRIX-32 Generic memory exerciser\n",
	"\n",
	"usage:\n",
	"\tmemx [-h] [-s] [-ofile] [-m#] [-p#] [-t#]\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-s\t(optional) Disable shared memory testing\n",
	"-ofile\t(optional) Save output diagnostics in file\n",
	"-m#\t(optional) Memory size specified (# > 4095 bytes)\n",
	"-p#\t(optional) Number of processes specified (1-20 default 20)\n",
	"-t#\t(optional) Run time in minutes (default: run forever until cntl-c)\n",
	"\n",
	"examples:\n",
	"\tmemx -p10\n",
	"\t{ Run 10 memx processes forever }\n",
	"\tmemx -t180 &\n",
	"\t{ Run 20(default) memx processes for 180 min. in background }\n",
	"\n",
	"",
};

int pid[DM_MAXPROC];			/* array of process id for memxr
					   routines spawned */
int pid2[DM_MAXPROC];
union wait status[DM_MAXPROC];

int nmemxr;				/* number of nmemxr routines spawned */
char errbuf[512];			/* buffer for error message */

int timedelta;

char DR_MEMX[] = "#LOG_MEMX_1";		/* Logfile name */

main (argc,argv)
int argc;
char **argv;
{
register i,j;
long maxmem,physmem,rb,mem;
long memsize;				/* memory size for each test */
long exmemsize;				/* extra size for one test */
char cmemsize[10],ctestnum[5];
char time[10],clogfd[5];
char smemsize[10];
char smemtime[10];
int maxproc;
int numproc = 0;
int sig;
int noshmxflg = 0;
void mem_clean();

	/* set up kill signal */
	signal(SIGINT,mem_clean);
	signal(SIGTERM,mem_clean);

	/* handle input args */
	maxproc = DM_MAXPROC;
	memsize = 0;
	while (--argc > 0 && **++argv == '-')
	{
		switch (*++*argv)
		{
		case 'm':
			memsize = atoi(++*argv);
			if (memsize < DM_MINMEM)
			{
				printf("memx: Memory size must be at least %d bytes\n",DM_MINMEM);
				exit(0);
			}
			break;
		case 'p':
			maxproc = atoi(++*argv);
			if (maxproc < 1)
				maxproc = 1;
			if (maxproc > DM_MAXPROC)
				maxproc = DM_MAXPROC;
			break;
		case 't':
			timedelta = atoi(++*argv);
			break;
		case 'o':	/* save output into file */
			fileptr = filename;
			while (*fileptr++ = *++*argv);
			break;
		case 'h':
			for (i = 0; *help[i]; i++)
				printf("%s",help[i]);
			exit(0);
		case 's':
			noshmxflg = 1;
			break;
		default:
			for (i = 0; *help[i]; i++)
				printf("%s",help[i]);
			exit(0);
		}

	}

	/* open log file */
	if (report(DR_OPEN,MODULE,DR_MEMX))
	{
		fprintf(stderr,"%s: Can not start report generator, test aborted\n", MODULE);
		exit(0);
	}

	/* 
	 *  The following section looks into kernel memory and retrieves
	 *  the usable memory sizes
	 */
	nlist("/vmunix",nl);
	for (i = 0; i < 2; i++)
		if (nl[i].n_type == N_UNDF)
		{
			printf("nl[%d] not accessed\n",i);
			mem = -1;
		}
	if (mem == -1)
		exit(0);
	if ((mem = open("/dev/kmem",0)) < 0)
	{
		sprintf(errbuf,"Could not open memory; %s\n",
			sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}
	if (lseek(mem,nl[0].n_value,L_SET) == -1)
	{
		sprintf(errbuf,"seek on nl[0] failed; %s\n",
			sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}
	if ((rb = read(mem,(char *) &maxmem,sizeof(maxmem))) != sizeof(maxmem))
	{
		sprintf(errbuf,"read on nl[0] failed; %s; rb = %d; request = %d",sys_errlist[errno],rb,sizeof(maxmem));
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}
	if (lseek(mem,nl[1].n_value,L_SET) == -1)
	{
		sprintf(errbuf,"seek on nl[1] failed; %s\n",
			sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}
	if ((rb = read(mem,(char *)&physmem,sizeof(physmem))) != sizeof(physmem))
	{
		sprintf(errbuf,"read on nl[1] failed; %s; rb = %d; request = %d",sys_errlist[errno], rb,sizeof(physmem));
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}

	/*
	 * calculate the number of processes to be spawned and the memory
	 * size for each to test
	 */

	if (memsize == 0)
		memsize = ctob(maxmem/DM_MAXPROC);
	exmemsize = ctob(maxmem%DM_MAXPROC);

	/* fork and exec memxr processes */
	sprintf(cmemsize,"%d",memsize);
	sprintf(clogfd,"%d",logfd);
	sprintf(time,"%d",timedelta);
	for (i = 0; i < maxproc; i++) {
		if ((pid[i] = fork()) == 0) {
			sprintf(ctestnum,"%d",i + 1);
			if ((i == 0) && (noshmxflg == 0)) {
			    sprintf(smemsize,"-m%d",memsize/6);
			    sprintf(smemtime,"-t%d",timedelta);
			    if (execl("shmx","shmx",smemsize,smemtime,0) < 0) {
				sprintf(errbuf,"Could not execl shmx %s",
				        sys_errlist[errno]);
				report(DR_WRITE,MODULE,errbuf);
			    }
			    exit(0);
			}
			if (i == DM_MAXPROC - 1)
				sprintf(cmemsize,"%d",memsize + exmemsize);
			if (execl("memxr","memxr",cmemsize,ctestnum,clogfd,time,0) < 0) {
				sprintf(errbuf,"Could not execl memxr%d %s",
				        i+1,sys_errlist[errno]);
				report(DR_WRITE,MODULE,errbuf);
			}
			exit(0);
		}
		if ((i == 0) && (noshmxflg == 0) && (pid[i] == -1)) {
			sprintf(errbuf,"Could not fork shmx %s",
				sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			continue;
		}
		if (pid[i] == -1) {
			sprintf(errbuf,"Could not fork memxr%d %s",
				i+1,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			continue;
		}
		if ((i == 0) && (noshmxflg == 0)) {
		    sprintf(errbuf,"Shared Memory Exerciser Process pid %d Started; memory size %s bytes\n",pid[i],cmemsize);
		    report(DR_WRITE,MODULE,errbuf);
		    numproc++;
		    continue;
		}
		sprintf(errbuf,"Exerciser Process memxr%d pid %d Started; memory size %s bytes\n",i+1,pid[i],cmemsize);
		report(DR_WRITE,MODULE,errbuf);
		numproc++;
	}
	(void)logerr(ELMSGT_DIAG,"Started Memory Exerciser");

	for(i = 0; i < numproc; i++)
		pid2[i] = wait(&status[i]);
		
	for (i = 0; i < numproc; i++) {
		for (j = 0; pid2[i] != pid[j] && j < numproc; j++) ;
		sig = status[i].w_termsig;
		if ((pid2[i] == pid[0]) && (noshmxflg == 0))
		    sprintf(errbuf,"Process Termination shmx pid %d Status %d %s",pid2[i],sig,sys_siglist[sig]);
		else
		    sprintf(errbuf,"Process Termination memxr%d pid %d Status %d %s",j+1,pid2[i],sig,sys_siglist[sig]);
		report(DR_WRITE,MODULE,errbuf);
	}
	(void)logerr(ELMSGT_DIAG,"Stopped Memory Exerciser");
	report(DR_CLOSE,0,DR_MEMX);
	exit(0);
}

void mem_clean()
{
register i;

	for(i = 0; i < DM_MAXPROC; i++)
		if (pid[i] > 0)
			kill (pid[i],SIGINT);

}
