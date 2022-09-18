#ifndef lint
static char *sccsid = "@(#)trace.c	4.1	ULTRIX	7/17/90";
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
 *	System Call Tracer User Interface Routine - Robert Rodriguez
 *
 *	EDIT HISTORY:
 *	09-Jun-1988  Mark Parenti
 *		Changed signal handlers to void.
 *
 *	15-Jan-1987  Marc Teitelbaum
 *		Add ability to send trace info to stdout in real time
 *		by specifying "-" to -f option.  In this mode, trace
 *		polls the trace buffer rather than waiting for select
 *		to wake us up - somewhat analagous to doing a "tail -f"
 *		on a growing file.
 */

#include <stdio.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/systrace.h>

int loop_cnt = 0, pid ,bad_read = -1;
void onterm(), onintr();
char *usage=
  "Usage: trace [-f filename ] [-c|-g|-p|-s|-u]#,#,...# cmd args args\n";

extern void sleep(),perror(),exit();

main(argc,argv)
char **argv;
{
	register int fd,fd_out,i=0;
	register char *filename, *cmd;
	int pgrp[TR_PGRP],pids[TR_PIDS],uids[TR_UIDS],sysc[TR_SYSC];
	int cflag=0,gflag=0,pflag=0,sflag=0,uflag=0,zflag=0;
	int poll = 0;  /* poll trace buffer rather than use select */
	char buf[MAXBSIZE];
	char cmdbuf[BUFSIZ];
	int c,errflg=0,cpid,readfds;
	extern int optind;
	extern char *optarg;

	filename = "trace.dump";	/* default output file name */
	while (( c=getopt(argc,argv,"c:f:g:p:s:u:z")) != EOF) {
		switch (c) {
		case 'c':	/* child flag */
			cpid = atoi(optarg);
			cflag++;
			break;
		case 'f':	/* file flag */
			filename = optarg;
			break;
		case 'g':	/* pgrp flag */
			gflag++;
			getcommalist(optarg,pgrp,TR_PGRP);
			break;
		case 'p':	/* pid flag */
			pflag++;
			getcommalist(optarg,pids,TR_PIDS);
			break;
		case 's':	/* syscall flag */
			sflag++;
			getcommalist(optarg,sysc,TR_SYSC);
			break;
		case 'u':	/* uid flag */
			uflag++;
			getcommalist(optarg,uids,TR_UIDS);
			break;
		case 'z':
			zflag++;
			break;
		case '?':
			errflg++;
		}
	}
	if (errflg) {
		(void)fprintf(stderr,usage);
		exit(1);
	}
	if (zflag) {
		(void)fprintf(stderr,"Logfile = %s\n",filename);
	}
	if (cflag && zflag) {
		(void)fprintf(stderr,"cflag = %d\ncpid %d, ",cflag,cpid);
		(void)fprintf(stderr,"pgrp = %d\n",getpgrp(cpid));
	}
	if (gflag && zflag) {
		(void)fprintf(stderr,"gflag = %d\npgrp\t",gflag);
		for(i=0;i<TR_PGRP;i++) (void)fprintf(stderr,"%d ",pgrp[i]);
		(void)fprintf(stderr,"\n");
	}
	if (pflag && zflag) {
		(void)fprintf(stderr,"pflag = %d\npids\t",pflag);
		for(i=0;i<TR_PIDS;i++) (void)fprintf(stderr,"%d ",pids[i]);
		(void)fprintf(stderr,"\n");
	}
	if (sflag && zflag) {
		(void)fprintf(stderr,"sflag = %d\nsysc\t",sflag);
		for(i=0;i<TR_SYSC;i++) (void)fprintf(stderr,"%d ",sysc[i]);
		(void)fprintf(stderr,"\n");
	}
	if (uflag && zflag) {
		(void)fprintf(stderr,"uflag = %d\nuids\t",uflag);
		for(i=0;i<TR_UIDS;i++) (void)fprintf(stderr,"%d ",uids[i]);
		(void)fprintf(stderr,"\n");
	}
	/* the rest of the args form the command */
	cmd = cmdbuf;
	*cmd = '\0';
	for (i=optind;i<argc;i++) {
		(void) strcat(cmd,argv[i]);
		(void) strcat(cmd," ");
	}
	if (zflag) {
		(void)fprintf(stderr,
			"argc = %d, optind = %d, cmd = %s\n",argc,optind,cmd);
		exit(0);
	}
	if (strcmp(filename,"-") == 0) {
		/* send output to stdout */
		fd_out=1;
		/* and poll buffer every second rather than wait for select */
		poll=1; 
	} else {
		fd_out = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0666);
		if (fd_out < 0) {
			(void)fprintf(stderr,"Can't open %s\n",filename);
			exit(2);
		}
	}
	pid = getpid();
	if ((int)signal(15,onterm) == -1 || (int)signal(2,onintr) == -1) {
		(void) perror("signal");
		exit(3);
	}
	fd = open("/dev/trace",0);
	if (fd < 0) {
		(void)fprintf(stderr,"Can't open /dev/trace\n");
		exit(4);
	}
	if (gflag+pflag+sflag+uflag == 0 && *cmd != '\0') {
		/* no flags -- just do the command */
		pgrp[0] = dofork(cmd);
		for (i=1;i<TR_PGRP;i++) pgrp[i] = 0;
		i = ioctl(fd,IOTR_SETPGRP,(char *)pgrp);
	}
	else {
		/* here is where we do each flag option */
		if (cflag) {
			pgrp[0] = getpgrp(cpid);
			for (i=1;i<TR_PGRP;i++) pgrp[i] = 0;
			i = ioctl(fd,IOTR_SETPGRP,(char *)pgrp);
		}
		else if (gflag) {
			i = ioctl(fd,IOTR_SETPGRP,(char *)pgrp);
		}
		else if (pflag) {
			i = ioctl(fd,IOTR_SETPIDS,(char *)pids);
		}
		else if (sflag) {
			i = ioctl(fd,IOTR_SETSYSC,(char *)sysc);
		}
		else if (uflag) {
			i = ioctl(fd,IOTR_SETUIDS,(char *)uids);
		}
		else {
			/* can't get here!!! */
			(void)fprintf(stderr,"options not yet implemented\n");
			exit(5);
		}
	}
	/*
	 *	We take a lot of care to be sure we get all the records
	 *	of a trace. When the command exits we will get the termination
	 *	signal or the user sends us the termination signal or the
	 *	interrupt signal. To handle all cases, we want to do 2 reads
	 *	to be sure we got all the data back.
	 */
	while (1) {
		if (bad_read == 0) loop_cnt++;
		if (loop_cnt > 2) break;	/* after signal and 2 tries */
		if (loop_cnt == 0) {
			/* once the signal has been received  */
			/* this loop is not done */
			if (!poll) {  /* normal case */
				readfds = 1<<fd;
				i = select(32,&readfds,(int *)0,(int *)0,
						(struct timeval *) 0);
				if (i == -1 && bad_read == -1) {
					(void) perror("select");
					break;
				}
			}
		}
		if (readfds & (1<<fd) || loop_cnt > 0 || poll) {
			/* this loop is done only if the select returned */
			/* for the /dev/trace fiel descriptor or if the */
			/* signal reset bad_read and thus bumped loop_cnt */
			/* - or if we are polling buffer		*/
			i = read(fd,buf,sizeof(buf));
			if (i > 0) {
				if (write(fd_out,buf,i) != i) {
					(void) perror("write");
				}
			}
			if (i <= bad_read) break;
			/* bad_read is -1 to start, 0 after we get a signal */
		}
		/* sleep a second if polling buffer */
		if (loop_cnt == 0 && poll)
			sleep(1);
	}
	i = close(fd);
	exit(0);
}

getcommalist(string,array,size)	/* parse comma separated list of numbers */
register char *string;
register int *array;
register int size;
{
	register int i;
	register char *ptr;
	ptr = string;
	for(i=0;i<size;i++) {
		if (*ptr != NULL) {
			array[i] = atoi(ptr);
			/* negatives not allowed */
			if (array[i] < 0) array[i] = 0;
			while(*ptr!=',' && *ptr != NULL) ptr++;
			while(*ptr==',') ptr++;
		}
		else array[i] = 0;
	}
}

void
onterm()  /* when child sends sigterm a zero read is bad */
{
	(void) signal(15,SIG_IGN);
	(void) signal(2,SIG_IGN);
	bad_read = 0;
}

void
onintr()  /* when we receive sigint a zero read is bad */
{
	(void) signal(2,SIG_IGN);
	(void) signal(15,SIG_IGN);
	bad_read = 0;
}

dofork(cmd)
register char *cmd;
{
	register int forkpid;
	if ((forkpid = fork())==0) {  /* child */
		(void) sleep((unsigned)2);
			/* sleep to let parent set up for trace */
		forkpid = getpid();
		(void) setpgrp(forkpid,forkpid);   /* set pgrp for tracing */
		(void) system(cmd);	/* do command */
		(void) kill(pid,15);	/* tell parent no more tracing to do */
		exit(0);
	}
	return forkpid;	/* give back pgrp to trace */
}
