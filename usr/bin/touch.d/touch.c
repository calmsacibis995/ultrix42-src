#ifndef lint
static	char *sccsid = "@(#)touch.c	4.1	(ULTRIX)     7/17/90";
#endif lint
/*
 *			Copyright (c) 1987, 1988 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	This software is  derived  from  software  received  from  the
 *	University    of   California,   Berkeley,   and   from   Bell
 *	Laboratories.  Use, duplication, or disclosure is  subject  to
 *	restrictions  under  license  agreements  with  University  of
 *	California and with AT&T.					
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 */

/*	Modification history:
 *	Original - from System V v2.2
 *
 *	Teoman Topcubasi
 *
 *	03/23/88, T. Topcubasi
 *	01 - corrected handling of more than one file and added a
 *	     message when -c is specified
 *
 *	02 - corrected missing variable to fprintf
 *	11/20/88, J. Reeves for D. Long
 *	03 - fixed readwrite and readbyte calls to use correct number
 *	     of arguments
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>

#define	dysize(A) (((A)%4)? 365: 366)

struct	stat	stbuf;
int	status;
int dmsize[12]={
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char	*cbp;
long	timbuf, time();

gtime()
{
	register int i, y, t;
	int d, h, m;
	long nt;

	tzset();

	t = gpair();
	if(t<1 || t>12)
		return(1);
	d = gpair();
	if(d<1 || d>31)
		return(1);
	h = gpair();
	if(h == 24) {
		h = 0;
		d++;
	}
	m = gpair();
	if(m<0 || m>59)
		return(1);
	y = gpair();
	if (y<0) {
		(void) time(&nt);
		y = localtime(&nt)->tm_year;
	}
	if (*cbp == 'p')
		h += 12;
	if (h<0 || h>23)
		return(1);
	timbuf = 0;
	y += 1900;
	for(i=1970; i<y; i++)
		timbuf += dysize(i);
	/* Leap year */
	if (dysize(y)==366 && t >= 3)
		timbuf += 1;
	while(--t)
		timbuf += dmsize[t-1];
	timbuf += (d-1);
	timbuf *= 24;
	timbuf += h;
	timbuf *= 60;
	timbuf += m;
	timbuf *= 60;
	return(0);
}

gpair()
{
	register int c, d;
	register char *cp;

	cp = cbp;
	if(*cp == 0)
		return(-1);
	c = (*cp++ - '0') * 10;
	if (c<0 || c>100)
		return(-1);
	if(*cp == 0)
		return(-1);
	if ((d = *cp++ - '0') < 0 || d > 9)
		return(-1);
	cbp = cp;
	return (c+d);
}

main(argc, argv)
char *argv[];
{
	register c;
	struct utbuf { 
		long actime, modtime; 
	} 
	times;

	int mflg=1, aflg=1, cflg=0, fflg=0, nflg=0, errflg=0, optc, fd;
	extern char *optarg;
	extern int optind;

	while ((optc=getopt(argc, argv, "amcf")) != EOF)
		switch(optc) {
		case 'm':
			mflg++;
			aflg--;
			break;
		case 'a':
			aflg++;
			mflg--;
			break;
		case 'c':
			cflg++;
			break;
		case 'f':
			fflg++;
			break;
		case '?':
			errflg++;
		}

	if(((argc-optind) < 1) || errflg) {
		(void) fprintf(stderr, "usage: touch [-amcf] [mmddhhmm[yy]] file ...\n");
		exit(2);
	}
	status = 0;
	if(!isnumber(argv[optind]))
		if((aflg <= 0) || (mflg <= 0))
			timbuf = time((long *) 0);
		else
			nflg++;
	else {
		cbp = (char *)argv[optind++];
		if(gtime()) {
			(void) fprintf(stderr,"date: bad conversion\n");
			exit(2);
		}
		timbuf += timezone;
		if (localtime(&timbuf)->tm_isdst)
			timbuf += -1*60*60;
	}
	for(c=optind; c<argc; c++) {
		if(stat(argv[c], &stbuf)) {
			if(cflg) {
	/* 01 */		(void) fprintf(stderr, "touch: %s: does not exist\n",argv[c]);	/* 02 */
				status++;
				continue;
			}
			else if ((fd = creat (argv[c], 0666)) < 0) {
				(void) fprintf(stderr, "touch: %s cannot create\n", argv[c]);
				status++;
				continue;
			}
			else {
				(void) close(fd);
				if(stat(argv[c], &stbuf)) {
					(void) fprintf(stderr,"touch: %s cannot stat\n",argv[c]);
					status++;
					continue;
				}
			}
		}

		times.actime = times.modtime = timbuf;
		if (mflg <= 0)
			times.modtime = stbuf.st_mtime;
		if (aflg <= 0)
			times.actime = stbuf.st_atime;
		if (fflg) {
			fflg--;
			if (chmod(argv[c],0666)) {
				fprintf(stderr, "touch: %s: couldn't chmod: ", argv[c]);
				perror("");
				status++;
				continue;
			}
			if(utime(argv[c], (struct utbuf *)
			    (nflg? 0:&times))) {
				if (aflg == 1 && mflg == 1) {
					if(readwrite (argv[c],stbuf.st_size))
						goto error1;
					else
						continue;
				}
				if (mflg != 1) {
					if(readbyte (argv[c],stbuf.st_size))
						goto error1;
					else
						continue;
				}
				(void) fprintf(stderr,"touch: cannot change times on %s\n",argv[c]);
				status++;
				continue;
			}
			if (chmod(argv[c],stbuf.st_mode)) {
				fprintf(stderr,"touch: %s couldn't chmod back: ", argv[c]);
				perror("");
				continue;
			}
		}
		else
		if (!access(argv[c],4|2)) {
			if(utime(argv[c], (struct utbuf *)(nflg? 0: &times))) {
				if (aflg == 1 && mflg == 1)  
					if (cflg == 1) {
						if(readwrite (argv[c],stbuf.st_size))
							goto error1;
						else
							continue;
					} 
					else {
						if (mflg != 1) {
							if(readbyte (argv[c],stbuf.st_size))
								goto error1;
							else
								continue;
						}
					}
			}
		}
		else {
error1:			
			(void) fprintf(stderr,"touch: cannot change times on %s\n",argv[c]);
			status++;
			continue;
		}
	}
	exit(status);  /* 01 */
}

isnumber(s)
char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c))
			return(0);
	return(1);
}

readwrite(input,size)
char  *input;
int    size;
{
	int	filedescriptor;
	char	first;

	if (size) {
		filedescriptor = open(input,2);
		if (filedescriptor == -1) {
error:
			fprintf(stderr, "touch: %s: ", input);
			perror("");
			return;
		}
		if (read(filedescriptor, &first, 1) != 1) {
			goto error;
		}
		if (lseek(filedescriptor,0l,0) == -1) {
			goto error;
		}
		if (write(filedescriptor, &first, 1) != 1) {
			goto error;
		}
	} 
	else {
		filedescriptor = creat(input,0666);
		if (filedescriptor == -1) {
			goto error;
		}
	}
	if (close(filedescriptor) == -1) {
		goto error;
	}
}

readbyte (input, size)
char   *input;
int     size;
{

	int	filedescriptor;
	char	first;

	if (size) {
		filedescriptor = open(input,2);
		if (filedescriptor == -1) {
rerror:
			fprintf(stderr, "touch: %s: ", input);
			perror("");
			return;
		}
		if (read(filedescriptor, &first, 1) != 1) 
			goto rerror;
	}
}
