
#ifndef lint
static	char	*sccsid = "@(#)diaglib.c	4.1		7/2/90";
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
#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/time.h>
#include "diag.h"

#define PT_55		0x55
#define PT_AA		0xaa
#define PT_NULL		0
#define OPN_FLAGS	O_WRONLY+O_CREAT+O_APPEND+O_EXCL
#define MODE		0644
#define LOGOFF		10

/* Global variables */
int offset = 0;				/* printer pattern offset */
int logfd;				/* file descriptor of log file */
int filefd = -1;			/* file descriptor of save file */
int tailpid;				/* process ID of "tail" utility */
long randx;				/* random number seed */
char filename[127] = "";		/* file to save output into */
char *fileptr;				/* pointer to filename */


/*
 *
 *	PATTERN -- This routine will fill a buffer, of a given length, 
 *		   with a requested pattern.
 *
 */

pattern(select,size,bufptr)
register select;			/* Requested pattern */
register size;				/* number of bytes in buffer */
char *bufptr;				/* buffer to fill with pattern */
{
register i;
char pattern;				/* temp char variable */
char *from, *to;			/* temp buffer 	*/
long *rto;				/*    pointers  */
static char prpattn[] = 		/* printer pattern seed */
"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+`~;'<>,.?abcdefghijklmnopqrstuvwxyz";

	switch (select)
	{
	case DG_PRINT:			/* fill buffer with printer pattern */

		to = bufptr;
		if (offset >= sizeof(prpattn) - 1)
			offset = 0;
		from = prpattn + offset;
		for (i = 0; i < size; i++)
		{
			*to++ = *from++;
			if ((from - prpattn) >= sizeof(prpattn) -1)
				from = prpattn;
		}
		*to++ = '\n';
		*to = 0;
		offset++;
		break;

	case DG_5555:			/* fill buffer with 0x55 */

		to = bufptr;
		for (i = 0; i < size; i++)
			*to++ = PT_55;
		break;

	case DG_AAAA:			/* fill buffer with 0xaa */

		to = bufptr;
		for (i = 0; i < size; i++)
			*to++ = PT_AA;
		break;
			
	case DG_NULLS:			/* zero buffer */

		to = bufptr;
		for (i = 0; i < size; i++)
			*to++ = PT_NULL;
		break;
			
	case DG_RANDM:			/* fill buffer with data 
					   starting at a random long and then
					   increasing by 1 */

		rto = (long *) bufptr;
		randx = rng(randx); 
		for (i = 0; i < size; i += 4)
			*rto++ = randx++;
		break;

	default:
		printf("pattern: Invalid pattern request - %d\n",select);	
		break;
	}
}


/*
 *
 *	REPORT -- This routine will format and report a fault
 *
 */

report(flag,mod,buf)
int flag;
char *mod;
char *buf;
{
extern int errno;
char *logp;
long dattm;
char *ctime();
int cppid;
static char errbuf[1024];

	switch (flag)
	{

	case DR_OPEN:

		/* open temporary log file */
		logp = buf + LOGOFF;
		while ((logfd = open(buf,OPN_FLAGS,MODE)) < 0) {
		    if (errno == EEXIST) {
			if (*logp == '9') {
			    fprintf(stderr,"\n%s: Remove old log files\n",mod);
			    return(1);
			}
			else
			    ++*logp;
		    }
		    else {
			fprintf(stderr,"\n%s: Can not open %s\n",mod,buf);
		        return(1);
		    }
		}

		/* fork and exec tail process */
		if ((tailpid = fork()) == 0)
		{
			if (setpgrp(tailpid,getpid()) < 0)
			{
				fprintf(stderr,"\n%s: Could not setpgrp for \"tail\"\n", mod);
				exit(0);
			}
			if (execl("/usr/ucb/tail","tail","-f",buf,0) < 0)
				fprintf(stderr,"\n%s: Could not execl \"tail\"\n", mod);
			exit(0);
		}
		if (tailpid == -1)
		{
			fprintf(stderr,"\n%s: Could not fork \"tail\"\n",mod);
			return(1);
		}
		else		/* delay to allow "tail" to be started */
			sleep(5);
		break;

	case DR_WRITE:

		/* finish formatting message and write to logfile */
		dattm = time(0);
		sprintf(errbuf,"\n%s%s: %s\n\n",ctime(&dattm),mod,buf);
		write(logfd,errbuf,strlen(errbuf)); 
		break;

	case DR_CLOSE:

		/* close logfile and kill "tail" */
		sleep(10);	/* give enough time for children to finish */
		close(logfd);
		kill (tailpid,SIGTERM);

		/* if output file to be saved; copy it */
		if (filename[0] != '\0')
		{
			if ((cppid = fork()) == 0)
			{
				if(execl("/bin/mv","mv",buf,filename,0) < 0)
					fprintf(stderr,"%s: Could not copy output to %s; errno = %d\n",filename,errno);
				exit(0);
			}
			if (cppid == -1)
				fprintf(stderr,"%s: Could not copy output to %s; errno = %d\n",filename,errno);
			else
				while (wait(0) != cppid);
		}
	/*	unlink(buf);*/
		break;

	default:
		fprintf(stderr,"%s: Invalid report option %d\n",flag);
		return(1);
	}
return(0);
}


/* 
 *	CMPSTR -- This routine will compare 2 strings, stopping when it
 *		  finds a null in the first (without comparing the null).
 *		  It returns a (1) if strings contain at least one discrepancy.
 */

cmpstr(a,b)
register char *a;
register char *b;
{

	while (*a)
		if (*a++ != *b++)
			return (1);
	return (0);
}


/* 
 *
 *	CPYSTR - will copy (c) bytes from string (b) into string (a), including
 *		 nulls.
 *
 */
cpystr(a,b,c)
register char *a;
register char *b;
int c;
{
register i;

	for (i = 0; i < c; i++)
		*a++ = *b++;
}
