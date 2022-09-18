
#ifndef lint
static	char	*sccsid = "@(#)shmxb.c	4.1		7/2/90";
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
/*
 *
 *	SHMXB.C -- This process runs with the shmx process to test
 *		   shared memory segments.
 *
 * 4/11/89 -- jaa
 *	when running through shared memory segments, don't assume
 *	they are contigous (side effect on VAX)
 *
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/param.h>
#include <sys/time.h>
#include "diag.h"

#define MODULE	"shmxb"

char errbuf[1024];
char *shmp[12], *tp;
int shmid[12];
int semid;
int smseg;
int timedelta;
long stoptime;
unsigned long totpasses;
unsigned long failures;

main (argc,argv) 
int argc;
char **argv;
{
static struct sembuf sops[DS_NUMSEM] = {0,0,0,1,0,0};
int i,j, n;
char *shmat();
int shmxbclean();
long key[12];
int memsize;


	/* handle signals */
	signal(SIGHUP,shmxbclean);
	signal(SIGINT,shmxbclean);
	signal(SIGTERM,shmxbclean);

	key[0] = atoi(*++argv);
	smseg = atoi(*++argv);
	memsize = atoi(*++argv);
	timedelta = atoi(*++argv);
	logfd = atoi(*++argv);

	/* initialize  key array */
	for (i = 1; i < smseg; i++) 
		key[i] = key[i - 1] + 1;

	sprintf(errbuf,"Started Shared Memory Background Exerciser Process; pid %d\n",getpid());
	report(DR_WRITE,MODULE,errbuf);

	/* 
	 * get 2 semaphores (both are set initially in shmtest); 
	 *	#0 blocks shmback while shmtest works 
	 *	#1 blocks shmtest while shmtest works
	 */
	if ((semid = semget(key[0], DS_NUMSEM,DS_MODESEM | IPC_CREAT)) < 0) {
		sprintf(errbuf,"Semget Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		shmxbclean();
	}

	/* get and attach to shared memory segments */
	for (i = 0; i < smseg; i++) {
		if ((shmid[i] = shmget(key[i],memsize,DS_MODESM | IPC_CREAT)) < 0) {
			sprintf(errbuf,"Shmget[%d] Fault: %s\n",i,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			shmxbclean();
		}

		if ((shmp[i] = shmat(shmid[i],0,0)) < 0) {
			sprintf(errbuf,"Shmat[%d] Fault: %s\n",i,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			shmxbclean();
		}
	}

	if (timedelta)
		stoptime = time(0) + (timedelta * 60);

	while (DS_INFINITE) {
	    /* wait while memory is filled by shmtest */
	    sops[0].sem_op = DS_SWAIT;
	    if (semop(semid,&sops[0],1) < 0) {
		sprintf(errbuf,"Semop1 Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		break;
	    }
	for(n = 0; n < smseg; n++) {
	    /* is data correct?? */
	    tp = shmp[n];
	    j = 0;
	    for (i = 0; i < memsize; i++,tp++) {
		if (*tp != DS_PAT1 && j < 20) {
			if (!j) {
				sprintf(errbuf,"Patterns Don't Match:\n");
				report(DR_WRITE,MODULE,errbuf);
			}
			sprintf(errbuf,"%x: %x	%x\n",tp,DS_PAT1,*tp);
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

	    /* put new data into shared memory segment */
	for(n = 0; n < smseg; n++) {
	    tp = shmp[n];
	    for (i = 0; i < memsize; i++,tp++)
		*tp = DS_PAT2;
	}

	    /* release shmtest to check data */
	    sops[1].sem_op = DS_SRESET;

	    if (semop(semid,&sops[1],1) < 0) {
		sprintf(errbuf,"Semop2 Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		break;
	    }

	    /* set to hold self off */
	    sops[0].sem_op = DS_SSET;
	    if (semop(semid,&sops[0],1) < 0) {
		sprintf(errbuf,"Semop3 Fault: %s\n",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		break;
	    }
	    if (tstop())
		break;
	}
	shmxbclean();
}

int tstop()
{
	return(stoptime && (stoptime < time(0)) ? 1 : 0);
}

shmxbclean()
{
int i;

	sprintf(errbuf,"Stopped Shared Memory Background Exerciser Process; %d Sucessful Passes %d Failures\n",totpasses,failures);
	report(DR_WRITE,MODULE,errbuf);
	for (i = 0; i < smseg; i++) {
	    if (shmp[i]) {
		if (shmdt(shmp[i]) < 0) {
		    sprintf(errbuf,"Shmdt Fault: %s\n",sys_errlist[errno]);
		    report(DR_WRITE,MODULE,errbuf);
		}
	    }
	}
	exit(0);
}

