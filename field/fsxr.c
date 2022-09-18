
#ifndef lint
static	char	*sccsid = "@(#)fsxr.c	1.2		4/16/86";
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
/*
 *
 *	FSXR.C -- This routine perform the file system exercises.  
 *
 */

#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/param.h>
#include "diag.h"


#define MODULE		"fsxr"
#define MAXFILES	5
#define FILEN		"FILESYS"
#define MAXFSIZE	15872
#define MAXERRCT	10

/*
 *	Global data
 */
struct fs_stat myfstat;

char errbuf[1024];
char *errptr;

u_char tstbuf[MAXFSIZE];
u_char rtnbuf[MAXFSIZE];

int *fdptr;
int fd[MAXFILES];

char file[138];

/* run time variables */
int timedelta;
long stoptime;
long pid;

int testnum;
char module[14];

main (argc,argv)
int argc;
char *argv[];
{
register int i,j;
void clean();
int rb,fsize,errct,ret;
u_char *tst, *rtn;

	/* Initialize stuff */
	pid = getpid();

	if (strcmp(argv[1],"/") == 0)
	    argv[1] = '\0';
	sprintf(file,"%s%s%s.%d",argv[1],"/",FILEN,pid);

	sprintf(module,"%s%s",MODULE,argv[2]);
	testnum = atoi(argv[2]);
	logfd = atoi(argv[3]);
	timedelta = atoi(argv[4]);

	/* insure that 'kill' signal will be cleaned up properly */
	signal(SIGTERM,clean);
	signal(SIGINT,clean);

	randx = time(0) & 0x1ff;

	if (timedelta)
		stoptime = time(0) + (timedelta * 60);

	/* Keep testing until stoptime or killed */
	fdptr = fd;
	forever {
		fsize = time(0) & 0x1ff;
		fsize = rng(fsize) & 0x1f;
		if (fsize == 0)
			fsize = 1;
		fsize *= DEV_BSIZE;

		/* Fill buffer with pattern */
		pattern(DG_RANDM,fsize,tstbuf);

		/* create file; write fsize; and close file */
		if ((*fdptr = open(file,O_WRONLY+O_CREAT,0600)) < 0)
		{
			sprintf(errbuf,"Can not create %s; %s\n",file,
				sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			myfstat.fs_fail++;
			clean();
		}
		myfstat.fs_cr++;
		myfstat.fs_op++;
		rb = ret = 0;
readloop:
		if ((ret = write(*fdptr,tstbuf,fsize-rb)) < 0)
		{
			sprintf(errbuf,"Write error on %s; %s\n",file,
				sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			myfstat.fs_fail++;
			clean();
		}
		if ( (rb += ret) < fsize)
			goto readloop; 
		myfstat.fs_wr++;
		close(*fdptr);
		myfstat.fs_cl++;

		/* open file; read fsize; close and delete file */
		if ((*fdptr = open(file,O_RDONLY)) < 0)
		{
			sprintf(errbuf,"Can not open %s; %s\n",file,
				sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			myfstat.fs_fail++;
			clean();
		}
		myfstat.fs_op++;
		if ((rb = read(*fdptr,rtnbuf,fsize)) < 0)
		{
			sprintf(errbuf,"Read error on %s; %s\n",file,
				sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			myfstat.fs_fail++;
			clean();
		}
		myfstat.fs_rd++;

		/* test data written with data read */
		tst = tstbuf;
		rtn = rtnbuf;
		sprintf(errbuf,"Data error in file %s\n",file);
		errptr = errbuf;
		bumpptr(errptr);
		errct = 0;
		for (j = 0; j < fsize; j++) {
			if (*rtn++ != *tst++) {
			    if (++errct > MAXERRCT) {
				sprintf(errptr,"[error printout limit exceeded]\n");
				break;
			    }
			sprintf(errptr,"Byte = %d Good = %x Bad = %x\n",j,*(tst-1),*(rtn-1));
			bumpptr(errptr);
			}
		}
		if (errct) {
			report(DR_WRITE,MODULE,errbuf);
			myfstat.fs_fail++;
		}
		close(*fdptr);
		myfstat.fs_cl++;
		unlink(file);
		myfstat.fs_ul++;
		myfstat.fs_pass++;

		if (stoptime && stoptime < time(0))
			clean();
	}

}

void clean()
{
register i;

	/* insure that file is deleted */
	close(*fdptr);
	unlink(file);

	/* print final report */
	errptr = errbuf;
	sprintf(errptr,"Exerciser Process Stopped; pid %d\n",pid);
	bumpptr(errptr);
	sprintf(errptr,"\n Creates    Opens   Writes    Reads   Closes");
	bumpptr(errptr);
	sprintf(errptr,"  Unlinks   Passed Failed\n");
	bumpptr(errptr);
	sprintf(errptr,"%8d %8d %8d %8d %8d %8d %8d %6d\n",myfstat.fs_cr,
	  myfstat.fs_op,myfstat.fs_wr,myfstat.fs_rd,myfstat.fs_cl,myfstat.fs_ul,
	  myfstat.fs_pass,myfstat.fs_fail);
	bumpptr(errptr);
	report(DR_WRITE,module,errbuf);
	exit(0);
}
