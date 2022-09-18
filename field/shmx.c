
#ifndef lint
static	char	*sccsid = "@(#)shmx.c	4.1		7/2/90";
#endif lint

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
 * 4/11/89 -- jaa
 *	when running through shared memory segments, don't assume
 *	they are contigous (side effect on VAX)
 *
 * 7/15/88 -- prs
 *      Added char string DR_SHMX to allow proper use with -o option.
 *
 ************************************************************************/
/*
 *
 *  SHMX.C --  Shared memory exerciser tests shared memory segments
 *	       with the shmxb background process.
 *
 */

#include <stdio.h>
#define SHMEM			/* to satisfy param.h IFDEFs	*/
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <nlist.h>
#include "diag.h"


#define MODULE		"shmx"			/* name of module */
#define MINUTE		60			/* minute in 1 sec counts */
#define HOUR		60*MINUTE
#define SECOND		1 			/* one second */
#define ZZZZZZ		5

struct nlist nl[] =
{
	{"_sminfo"},
	{""},
};

char *help[] =
{
	"\n\n(shmx) - ULTRIX-32 Shared memory exerciser\n",
	"\n",
	"usage:\n",
	"\tshmx [-h] [-ofile] [-m#] [-s#] [-v] [-t#]\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-ofile\t(optional) Save output diagnostics in file\n",
	"-m#\t(optional) Shared memory size specified (# > 1 bytes)\n",
	"-s#\t(optional) Number of shared segments specified 1-6 (default: 6)\n",
	"-v\t(optional) Use fork (default: vfork)\n",
	"-t#\t(optional) Run time in minutes (default: run forever until cntl-c)\n",
	"\n",
	"examples:\n",
	"\tshmx -s1\n",
	"\t{ Run 1 shared memory segment forever }\n",
	"\tshmx -t180 &\n",
	"\t{ Run 6 (default) shared memory segments for 180 min. in background }\n",
	"\n",
	"",
};

void shmclean();
int pid;			/* process id for shmxb routine spawned */
union wait status[DM_MAXPROC];
char errbuf[512];			/* buffer for error message */
int timedelta;
long stoptime;

unsigned long totpasses;
unsigned long failures;
int semid = -1;
int shmid[12];
char *shmp[12];
int fork(), vfork();
int (*frk[])() = {vfork,fork};
int key[12];			/* key and ascii verson of key */
char a_key[12];
struct sminfo sminfo;
long memsize;
int smseg;

char DR_SHMX[] = "#LOG_SHMX_1";	/* Logfile name */

main (argc,argv)
int argc;
char **argv;
{
register i;
long rb,mem;
int frkindx = 0;

	/* set up kill signal */
	signal(SIGHUP,shmclean);
	signal(SIGINT,shmclean);
	signal(SIGTERM,shmclean);

	/* handle input args */
	memsize = 0;
	while (--argc > 0 && **++argv == '-')
	{
		switch (*++*argv)
		{

		case 'm':
			memsize = atoi(++*argv);
			if (memsize < 1) {
				printf("shmx: Memory size must be at least %d bytes\n","1");
				exit(0);
			}
			break;

		case 's':
			smseg = atoi(++*argv);
			if (smseg < 1)
				smseg = 1;
			break;

		case 't':
			timedelta = atoi(++*argv);
			break;

		case 'v':
			frkindx = 1;
			break;

		case 'o':	/* save output into file */
			fileptr = filename;
			while (*fileptr++ = *++*argv);
			break;

		case 'h':
			for (i = 0; *help[i]; i++)
				printf("%s",help[i]);
			exit(0);

		default:
			printf("shmx: Invalid arg %s\n",*argv);
		}

	}

	/* open log file */
	if (report(DR_OPEN,MODULE,DR_SHMX))
	{
		fprintf(stderr,"%s: Can not start report generator, test aborted\n", MODULE);
		exit(0);
	}

	/* 
	 *  The following section looks into kernel memory and retrieves
	 *  the usable memory sizes
	 */
	nlist("/vmunix",nl);
	for (i = 0; i < 1; i++)
		if (nl[i].n_type == N_UNDF)
		{
			sprintf(errbuf,"nl[%d] Not Accessed\n",i);
			report(DR_WRITE,MODULE,errbuf);
			exit(0);
		}

	if ((mem = open("/dev/kmem",0)) < 0)
	{
		sprintf(errbuf,"Could Not Open Memory: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}
	if (lseek(mem,nl[0].n_value,L_SET) == -1)
	{
		sprintf(errbuf,"Seek On nl[0] Failed: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}
	if ((rb = read(mem,(char *) &sminfo,sizeof(sminfo))) != sizeof(sminfo))
	{
		sprintf(errbuf,"Read On nl[0] Failed: %s; rb = %d; request = %d",sys_errlist[errno],rb,sizeof(sminfo));
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}

	if (memsize == 0) {
	    memsize = sminfo.smmax/sminfo.smseg;
	}
	else if (memsize > sminfo.smmax) {
	    memsize = sminfo.smmax;
	}

	if (smseg == 0) {
	    smseg = sminfo.smseg > 12 ? 12 : sminfo.smseg;
	}
	else if (smseg > sminfo.smseg) {
	    smseg = sminfo.smseg > 12 ? 12 : sminfo.smseg;
	}

	/* establish keys */
	key[0] = time(0);
	sprintf(a_key,"%d",key[0]);
	for (i = 1; i < smseg; i++)
		key[i] = key[i - 1] + 1;

	shm(frkindx);
	shmclean();
	exit(0);
}

/* Uses the various shared memory routines to allocate shared memory    */
/* and exercise it                                                      */

shm(finx) 
int finx;			/* index to indicate type of fork */
{

static struct sembuf sops[DS_NUMSEM] = {0,0,0,1,0,0};
static ushort semval[DS_NUMSEM] = {0,0};
char *shmat();
char *tp;
int i, j;
int k, m;
int n;
char cmemsize[10];
char csmseg[10];
char ctime[10],clogfd[5];

	/*
	 * get semaphore array of 2:
	 *      #0 blocks shmback while shmtest works
	 *      #1 blocks shmtest while shmback works
	 */
	if ((semid = semget(key[0], DS_NUMSEM,DS_MODESEM | IPC_CREAT)) < 0)
	{
		sprintf(errbuf,"Semget Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		shmclean();
	}


	/* Set both semaphores to 0 */
	if (semctl(semid,0,SETALL,semval) < 0) {
		sprintf("Semctl Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		shmclean();
	}

	/* Get smseg shared memory segments and attach */
	for (i = 0; i < smseg; i++) {
		if ((shmid[i] = shmget(key[i], memsize,DS_MODESM | IPC_CREAT)) < 0) {
			sprintf("Shmget[%d] Fault: %s\n",i,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			shmclean();
		}

		/* the segments must be attached contiguously */
		if ((shmp[i] = shmat(shmid[i],0,0)) < 0) {
			sprintf("Shmat[%d] Fault: %s\n",i,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			shmclean();
		}
	}
	/* set both semaphores */
	sops[0].sem_op = DS_SSET;
	sops[1].sem_op = DS_SSET;
	if (semop(semid,sops,2) < 0) {
		sprintf(errbuf,"Semop Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		shmclean();
	}

	sprintf(cmemsize,"%d",memsize);
	sprintf(csmseg,"%d",smseg);
	sprintf(clogfd,"%d",logfd);
	sprintf(ctime,"%d",timedelta);
	/* spawn shmback */
	if ((pid = (*frk[finx])()) == 0) {
		execl("shmxb","shmxb",a_key,csmseg,cmemsize,ctime,clogfd,0);
		sprintf(errbuf,"Could Not Exec Shmxb: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		shmclean();
	}
	if (pid < 0) {
		sprintf(errbuf,"Could Not Fork: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		shmclean();
	}
	sprintf(errbuf,"Started Shared Memory Exerciser Process; pid %d\n",getpid());
	report(DR_WRITE,MODULE,errbuf);

	if (timedelta)
		stoptime = time(0) + (timedelta * 60);

	(void)logerr(ELMSGT_DIAG,"Started Shared Memory Exerciser");

	totpasses = k = 0;
	while (DS_INFINITE) {
		/* write first test pattern into shared memory segments */
		for(n = 0; n < smseg; n++) {
			tp = shmp[n];
			for (i = 0; i < memsize; i++,tp++)
				*tp = DS_PAT1;
		}

		/* release shmback to check it */
		sops[0].sem_op = DS_SRESET;
		if (semop(semid,sops,1) < 0) {
			sprintf(errbuf,"Semop Fault: %s\n",sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			break;
		}

		/* wait for shmback to write in new pattern */
		sops[1].sem_op = DS_SWAIT;
		if (semop(semid,&sops[1],1) < 0) {
			sprintf(errbuf,"Semop Fault: %s\n",sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			break;
		}
		/* check new pattern */
		for(n = 0; n < smseg; n++) {
			tp = shmp[n];
			j = 0;
			for (i = 0; i < memsize; i++,tp++) {
				if (*tp != DS_PAT2 && j < 20) {
					if (!j) {
						sprintf(errbuf,"Patterns Don't Match:\n");
						report(DR_WRITE,MODULE,errbuf);
					}
					sprintf(errbuf,"%x: %x  %x\n",tp,DS_PAT2,*tp);
					report(DR_WRITE,MODULE,errbuf);
					j++;
				}
			}
			if (j) {
				failures++;
				if (failures > 10) {
				    sprintf(errbuf,"Too Many Errors, Exiting\n");
				    report(DR_WRITE,MODULE,errbuf);
				    break;
				}
			}
			else
				totpasses++;
		}

		/* set semaphore to hold myself off */
		sops[1].sem_op = DS_SSET;
		if (semop(semid,&sops[1],1) < 0) {
			sprintf(errbuf,"Semop Fault: %s\n",sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			break;
		}
		if (tstop())
			break;

		if (k++ > DS_NOLOOPS) {
			k = 0;
			sleep(ZZZZZZ);
		}
	}
	shmclean();
}

int tstop()
{
	return(stoptime && (stoptime < time(0)) ? 1 : 0);
}

void shmclean()
{
int i;
	if (pid > 0) {
	    kill(pid,SIGTERM);
	    wait(0);
	}

	sprintf(errbuf,"Stopped Shared Memory Exerciser Process; %d Successful Passes %d Failures\n",totpasses,failures);
	report(DR_WRITE,MODULE,errbuf);

	(void)logerr(ELMSGT_DIAG,"Stopped Shared Memory Exerciser");

	for (i = 0; i < smseg; i++) {
	    if (shmp[i]) {
		if (shmdt(shmp[i]) < 0) {
		    sprintf(errbuf,"Shmdt Fault: %s\n",sys_errlist[errno]);
		    report(DR_WRITE,MODULE,errbuf);
		}
	    }
	    if (shmid[i]) {
		if (shmctl(shmid[i], IPC_RMID,0) < 0) {
		    sprintf(errbuf,"Shmctl Fault: %s\n",sys_errlist[errno]);
		    report(DR_WRITE,MODULE,errbuf);
		}
	    }
	}
	if (semid >= 0)
	    if (semctl(semid,0,IPC_RMID,0) < 0) {
		sprintf(errbuf,"Semctl Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
	    }

	report(DR_CLOSE,0,DR_SHMX);
	exit(0);
}
