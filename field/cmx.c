
#ifndef lint
static char *sccsid = "@(#)cmx.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1989 by			*
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
 *	CMX.C	--  This routine will set up the I/O comm loopback test
 *
 */
 
/*************************************************************************
 *
 *	Modification History
 *
 * 06-28-89	Tim Burke
 *	General cleanups.  Display extra bytes read if more than the requested
 *	number of bytes are received.
 *
 * 07-15-88	prs
 *	Added char string DR_CMX to allow proper use with -o option.
 *
 * 07-21-87	afd (Al Delorey)
 *	Increase size of errbuf, since it is overflowed upon termination
 *	of 14 or more lines on one cmx process.  Make its size a factor
 *	of the constant DL_MAXTTY.
 *
 * 07-07-87	Tim Burke
 *	Send out fewer characters on a sh device (on Microvax 2000) to prevent
 * 	silo overflow.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/errlog.h>
#include <errno.h>
#include <ctype.h>
#include "diag.h"

#define MODULE		"cmx"			/* name of module */
#define MINUTE		60			/* minute in 1 sec counts */
#define SECOND		1 			/* one second */
#define MAXERRCT	10
#define DEVICE		"/dev/tty"
#define WAIT_TIME	180			/*Wait this many secs for read*/
#define ERROR_LIMIT	20			/* Number of error print lines*/

#define LAT_MAJOR	39			/* Major number for lat */
#define PTY_M_MAJOR	20			/* Major number for pty master*/
#define PTY_S_MAJOR	21			/* Major number for pty slave */
#define SH_MAJOR	51			/* Major number for sh.c */

/*
 *
 *	Global data
 *
 */

/* help message table */
char *help[] =
{
	"\n\n(cmx) - ULTRIX-32 I/O Communication Loopback Test\n",
	"\n",
	"usage:\n",
	"\tcmx [-h] [-ofile] [-t#] -l ## ##-##\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-ofile\t(optional) Save output diagnostics in file\n",
	"-t#\t(optional) Run time in minutes (default: run forever until cntl-c)\n",
	"-l ## ##-##\tTest all tty lines that follow\n",
	"##\tLines to test as per the /dev directory! (i.e. 00 13 42-53 74 )\n",
	"\n",
	"All lines to be tested must have loop back connectors on them!\n",
	"All lines to be tested must be disabled in the /etc/ttys file!\n",
	"The max. # of lines you can test is 32.\n",
	"DO NOT TEST pseudo or lta devices major numbers 20,21 & 39!\n",
	"Use the \"file\" command to determine tty line information.\n",
	"\n",
	"examples:\n",
	"\tcmx -t60 -l 00 02 07-11\n",
	"\t{ Run for 60 min. testing lines 00 02 07 08 09 10 11 }\n",
	"\tcmx -l 10 21 32-34 &\n",
	"\t{ Run forever testing lines 10 21 32 33 34 in the background }\n",
	"\n",
	"",
};

/* DL_TTY information table */
struct dl_tty dl_tty[DL_MAXTTY];	/* table dl_tty */
struct dl_tty *dl_ttyp;			/* pointer to entry in dl_tty */

struct sgttyb tsgtty = { 0,		/* sg_ispeed */
			 0,		/* sg_ospeed */
			'#',		/* sg_erase  */
			'@',		/* sg_kill   */
			(RAW|ANYP)};	/* sg_flags  */
struct sgttyb *tsgttyp;

/*
 * A table of baud rates.  The first column is the tty driver's representation
 * of the baud rate to be used in TIOCSETP calls.  The second column is a
 * numerical representation of the baud rate which is printed out in the 
 * event of error.
 */
int brate[8][2] = {
	{ B300,  300 },			/*  300 baud */
	{ B1200, 1200 },		/*  1200 baud */
	{ B2400, 2400 },		/*  2400 baud */
	{ B4800, 4800 },		/*  4800 baud */
	{ B9600, 9600 },		/*  9600 baud */
	{ B9600, 9600 },		/*  19.2k baud - placeholder */
	{ B9600, 9600 },
	{ B9600, 9600 }
};

/* packet pointer  */
struct dl_pkt *packet;

char errbuf[90 * DL_MAXTTY];			/* buffer for error message */
char *errptr;				/* error message pointer */

char device[14];			/* device pathname */
int rb;					/* requested bytes in read/write */
int lcnt = 0;				/* line count to be tested */
int cmxdebug = 0;

char DR_CMX[] = "#LOG_CMX_01";		/* Logfile name */

main (argc,argv)
int argc;
char **argv;
{
char *rtn, *tptr;			/* temporary string pointers */
short *checksum;			/* pointer to checksum word in packet */
int rtx;
int max_xfer = 124;			/* maximum packet size */
char t1[10], t2[10];			/* temporary string for parsing
					   input range */
int errct = 0;				/* bad character counter */
int test = 0;
int j = 0;
int i = 0;
int z = 0;
int num = 0;
int num2 = 0;
int zero = 0;
int intloop = 0;
int timedelta = 0;
long timebuf = 0;
long stoptime = 0;
char *malloc();				/* memory allocation function */
void cmx_clean();			/* function to cleanup after "kill" */
struct stat sbuf;
struct stat *sbufp;
int extra;
unsigned char *chrptr;

	/* set up kill handling */
	signal(SIGTERM,cmx_clean);
	signal(SIGINT,cmx_clean);

	if (argc == 1) {
		fprintf(stderr,"usage: cmx arg, type \"cmx -h\" for help\n");
		exit(0);
	}

	/* handle input args */
	while (--argc > 0) {
		if (**++argv == '-') {
			switch (*++*argv) {
			case 'h':	/* print help message */
				for (i = 0; *help[i]; i++)
					printf("%s",help[i]);
				exit(0);
			case 'o':	/* save output into file */
				fileptr = filename;
				while (*fileptr++ = *++*argv);
				break;
			case 't':
				timedelta = atoi(++*argv);
				break;
			case 'i':
				intloop = DL_LOOPBACK;
				break;
			case 'l':	/* ttys follow */
				break;
			default:
				printf("cmx: Invalid arg %s\n",*argv);
				exit(0);
			}
		}
		else {		/* retrieve and parse input range */
			tptr = t1;
			while (**argv && (**argv != '-'))
				*tptr++ = *(*argv)++;
			*tptr = NULL;
			num = atoi(t1);
			tptr = dl_tty[i].lname;
			sprintf(tptr,"%.2d",num);
			i++;
			lcnt++;
			if (**argv == '-') {
			        num2 = atoi(++*argv);
			        tptr = dl_tty[i-1].lname;
			        num = atoi(tptr);
			        while ((++num) <= num2) {
				    tptr = dl_tty[i].lname;
				    sprintf(tptr,"%.2d",num);
				    i++;
				    lcnt++;
			        }
			}
		}

	}
	if (lcnt == 0) {
		fprintf(stderr,"No tty lines chosen to test, type \"cmx -h\" for help\n");
		exit(0);
	}

	/* open report logger */
	if (report(DR_OPEN,MODULE,DR_CMX))
	{
		fprintf(stderr,"%s: Could not open report logger, test aborted\n",MODULE);
		exit(0);
	}
	if (lcnt > DL_MAXTTY) {
		lcnt = DL_MAXTTY;
		sprintf(errbuf,"To many tty lines first %d will be tested only!\n", DL_MAXTTY);
		report(DR_WRITE,MODULE,errbuf);
	}

	/* set up dl_tty for each TTY to be exercised */
	for (i = 0; i < lcnt; i++) {
		sprintf(device,"%s%s",DEVICE,dl_tty[i].lname);
		sbufp = &sbuf;
		if (lstat(device,sbufp) < 0) {
		    sprintf(errbuf,"Stat error on %s, %s\n",device,
			    sys_errlist[errno]);
		    report(DR_WRITE,MODULE,errbuf);
		    continue;
		}
		switch (major(sbufp->st_rdev)) {
		    case LAT_MAJOR:
		    	sprintf(errbuf,"Can't test %s: lta device\n",device);
		    	report(DR_WRITE,MODULE,errbuf);
		    	continue;
		    case PTY_M_MAJOR:
		    case PTY_S_MAJOR:
			sprintf(errbuf,"Can't test %s: pseudo device\n",device);
		    	report(DR_WRITE,MODULE,errbuf);
		    	continue;
		    case SH_MAJOR:
			/*
			 * Limit the number of characters on the sh driver
			 * due to the fact that its input buffers overflow 
			 * easily.
			 */
			max_xfer = 30;
			break;
		    default:
			break;
		}
		if ((dl_tty[i].fd = open(device,O_RDWR|O_NDELAY)) < 0) {
		    sprintf(errbuf,"Open error on %s, %s\n",device,
			    sys_errlist[errno]);
		    report(DR_WRITE,MODULE,errbuf);
		    continue;
		}
		dl_tty[i].flags = NULL;		/* clear flags */
		if (ioctl(dl_tty[i].fd, TIOCNMODEM, &dl_tty[i].flags) < 0) {
			sprintf(errbuf,"ioctl set TIOCNMODEM failed: %s\n",
				sys_errlist[errno]);
		        report(DR_WRITE,MODULE,errbuf);
		        continue;
		}
		if (fcntl(dl_tty[i].fd, F_SETFL, FNDELAY) < 0) {
			sprintf(errbuf,"fcntl set FNDELAY failed: %s\n",
				sys_errlist[errno]);
		        report(DR_WRITE,MODULE,errbuf);
		        continue;
		}
		dl_tty[i].flags |= DL_TEST;	/* indicate tty to be tested */
		test++;
	}
	if (!test) {
		sprintf(errbuf,"No lines to test!\n");
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}

	if (timedelta)
		stoptime = time(0) + (timedelta * 60);

	/* log start time */
	errptr = errbuf;
	sprintf(errptr,"Started cmx exerciser - testing:\n");
	bumpptr(errptr);
	for (i = 0; i < lcnt; i++) {
	    if (dl_tty[i].flags & DL_TEST) {
		sprintf(errptr,"tty%02s ",dl_tty[i].lname);
		bumpptr(errptr);
	    }
	}
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);

	tsgttyp = &tsgtty;

	/* loop until killed */
	forever {
			/* setup lines to be tested & writeout data */
		for (i = 0; i < lcnt; i++) {
		    if (dl_tty[i].flags & DL_TEST &&
			!(dl_tty[i].flags & DL_SETUP)) {

			ioctl(dl_tty[i].fd, TIOCFLUSH, &zero);
			rtx = time(0) & 0xff;
			dl_tty[i].baudrate = rng(rtx) & 0x7;
			tsgtty.sg_ispeed = brate[dl_tty[i].baudrate][0];
			tsgtty.sg_ospeed = brate[dl_tty[i].baudrate][0];
			ioctl(dl_tty[i].fd, TIOCSETP, tsgttyp);

			/* Set internal loop back mode - 
		 	* Not all drivers correctly implement the TIOCSMLB 
		 	* ioctl.  This does work on the dhu driver.  The dmb 
		 	* driver will incorectly clear the loopback bit in the 
		 	* driver in some places.  Consequently the line will not
		 	* remain in loopback mode.
		 	*/
			if (intloop == DL_LOOPBACK) {
		    	    if (ioctl(dl_tty[i].fd, TIOCSMLB) < 0) {
				sprintf(errbuf,"ioctl set loop back failed: %s\n",
					sys_errlist[errno]);
		        	report(DR_WRITE,MODULE,errbuf);
		        	continue;
		    	    }
		    	    dl_tty[i].flags |= DL_LOOPBACK;
			}

			/*
			 * Select a random transfer size.
			 */
			rtx = (rng(rtx) & 0x7f) + 10;
			dl_tty[i].datalen = rtx > max_xfer ? max_xfer : rtx;

			if (cmxdebug)
				printf("datalen %d\n",dl_tty[i].datalen);
			if (dl_tty[i].pktout == 0)
				if ((dl_tty[i].pktout = (struct dl_pkt *)
					malloc(DL_MAXPKT)) == NULL) {
					sprintf(errbuf,"Error allocating memory for tty%s",dl_tty[i].lname);
					report(DR_WRITE,MODULE,errbuf);
					dl_tty[i].flags = (DL_TEST|intloop); 
					dl_tty[i].errs++;
					continue;
				}
			if (dl_tty[i].pktin == 0)
				if ((dl_tty[i].pktin = (struct dl_pkt *)
					malloc(DL_MAXPKT)) == NULL) {
					sprintf(errbuf,"Error allocating memory for tty%s",dl_tty[i].lname);
					report(DR_WRITE,MODULE,errbuf);
					dl_tty[i].flags = (DL_TEST|intloop);
					dl_tty[i].errs++;
					continue;
				}
			dl_tty[i].pktp = (char *)dl_tty[i].pktin;
			/*
			 * Initialize all elements of the write packet to
			 * be zero.
			 */
			for (j = 0; j < DL_MAXPKT; j++)
				*dl_tty[i].pktp++ = 0;
			dl_tty[i].pktp = (char *)dl_tty[i].pktin;
			dl_tty[i].pktcnt = 0;
			dl_tty[i].flags |= DL_SETUP;
		    }

			/* place writes on all devices */
		    if (dl_tty[i].flags & DL_SETUP &&
			!(dl_tty[i].flags & DL_WRITE)) {

			/* set up packet; fill with data; calc checksum 
			 * The first 2 bytes of a packet contain the tty
			 * number.
			 */
			packet = dl_tty[i].pktout;
			packet->ttyline[0] = dl_tty[i].lname[0];
			packet->ttyline[1] = dl_tty[i].lname[1];
			checksum = &packet->checksum;
			*checksum = 0;
			*checksum += packet->ttyline[0];
			*checksum += packet->ttyline[1];
			dl_tty[i].pktlen = dl_tty[i].datalen + DL_PHDLEN;
			tptr = packet->data;
			pattern(DG_PRINT,dl_tty[i].datalen,tptr);
			for (j = 0; j < dl_tty[i].datalen; j++) {
				*checksum += *tptr++;
			}
			*checksum = -(*checksum);

			/* place write on character queue */
			if ((rb = write(dl_tty[i].fd,(char *) packet,
				dl_tty[i].pktlen)) < 0) {
				sprintf(errbuf,"Write error on tty%s, %s\n",
					dl_tty[i].lname,sys_errlist[errno]);
				report(DR_WRITE,MODULE,errbuf);
				dl_tty[i].flags = (DL_TEST|intloop);
				dl_tty[i].errs++;
				continue;
			}
			dl_tty[i].flags |= DL_WRITE;
			dl_tty[i].writes++;
			dl_tty[i].ccw += dl_tty[i].pktlen;
			if (cmxdebug)
				printf("write tty%s\n",dl_tty[i].lname);
			time(&dl_tty[i].wtime);
		    }
		}	/* End write loop */

		/*
		 * Give a short delay to allow the characters to transmit
		 * before initiating any reads.  The intent is to reduce the
		 * number of system calls.
		 */
		for (i = 0; i < 20000; i++) ;   

			/* read all devices */
		for (i = 0; i < lcnt; i++) {
		    if (dl_tty[i].flags & DL_WRITE &&
			!(dl_tty[i].flags & DL_READ)) {

			/* read characters into queue */
			if ((rb = read(dl_tty[i].fd,(char *) dl_tty[i].pktp,
				dl_tty[i].pktlen)) < NULL) {
				if (errno != EWOULDBLOCK) {
				    sprintf(errbuf,"read error on tty%s, %s\n",
				            dl_tty[i].lname,sys_errlist[errno]);
				    report(DR_WRITE,MODULE,errbuf);
				    dl_tty[i].flags = (DL_TEST|intloop);
				    dl_tty[i].errs++;
				    continue;
				}
				rb = 0;
			        time(&timebuf);
			        if ((timebuf - dl_tty[i].wtime) < WAIT_TIME) {
				    continue;
			        }
			        sprintf(errbuf,"Read data timeout on tty%s, brate:%d xmit#:%d recv#:%d\n", dl_tty[i].lname, brate[dl_tty[i].baudrate][1], dl_tty[i].pktlen, dl_tty[i].pktcnt);
			        report(DR_WRITE,MODULE,errbuf);
				dl_tty[i].flags = (DL_TEST|intloop);
				dl_tty[i].errs++;
				continue;
			}
			if (cmxdebug)
				printf("read tty%s\n",dl_tty[i].lname);
			dl_tty[i].reads++;
			dl_tty[i].ccr += rb;
			dl_tty[i].pktcnt += rb;
			dl_tty[i].pktp += rb;
			/*
			 * Continue reading until enough characters have been
			 * received.  If too many characters have been received
			 * then flush the line.  This is ok because another 
			 * write on this line will not commence until read
			 * completion.
			 */
			if (rb != 0) {
				if (dl_tty[i].pktcnt < dl_tty[i].pktlen) {
					continue;
				}
				else if (dl_tty[i].pktcnt > dl_tty[i].pktlen) {
					extra = dl_tty[i].pktcnt - 
						dl_tty[i].pktlen;
					sprintf(errbuf,"%d extra bytes on tty%s,brate:%d\n",extra, dl_tty[i].lname, brate[dl_tty[i].baudrate][1]);
					errptr = errbuf;
					bumpptr(errptr);
					sprintf(errptr,"Requested %d bytes, received %d bytes.\n",dl_tty[i].pktlen,dl_tty[i].pktcnt);
					bumpptr(errptr);
					chrptr=(unsigned char *)dl_tty[i].pktin;
					chrptr += dl_tty[i].pktlen;
					for (z = 0; z < extra; z++) {
						sprintf(errptr,"EXTRA[%d] = %.2x\n",z,*chrptr);
						bumpptr(errptr);
						chrptr++;
						if (z > ERROR_LIMIT) {
				    			sprintf(errptr,"[error printout limit exceeded]\n");
							break;
						}
					}
					bumpptr(errptr);
					sprintf(errptr,"Flushing line.\n");
					bumpptr(errptr);
					ioctl(dl_tty[i].fd, TIOCFLUSH, &zero);
					report(DR_WRITE,MODULE,errbuf);
					dl_tty[i].errs++;
				}
				/* Otherwise a full packet has been read. */
			}
			dl_tty[i].flags |= DL_READ;
		    }
		}	/* end read loop */

			/* validate data and checksum */
		for (i = 0; i < lcnt; i++) {
		    if (dl_tty[i].flags & DL_READ &&
		        dl_tty[i].flags & DL_WRITE) {

			tptr = (char *)dl_tty[i].pktout;
			rtn = (char *)dl_tty[i].pktin;
			sprintf(errbuf,"Data error on tty%s\n",dl_tty[i].lname);
			errptr = errbuf;
			bumpptr(errptr);
			errct = 0;
			for (j = 0; j < dl_tty[i].pktlen; j++) {
			    if (*rtn++ != *tptr++) {
				if (++errct > ERROR_LIMIT) {
				    sprintf(errptr,"[error printout limit exceeded]\n");
				    break;
				}
				sprintf(errptr,"BYTE = %d	GOOD = %.2x	BAD = %.2x\n",j,(unsigned char)*(tptr - 1),(unsigned char)*(rtn - 1));
				bumpptr(errptr);
			    }
			}

			/* if data error then report it */
			if (errct) {
				report(DR_WRITE,MODULE,errbuf);
				dl_tty[i].flags = (DL_TEST|intloop);
				dl_tty[i].errs++;
				continue;
			}
			packet = dl_tty[i].pktin;
			checksum = &packet->checksum;
			*checksum += packet->ttyline[0];
			*checksum += packet->ttyline[1];
			tptr = packet->data;
			for (j = 0; j < dl_tty[i].datalen; j++)
				*checksum += *tptr++;
			if (*checksum) {
				sprintf(errbuf,"checksum error on tty%s",dl_tty[i].lname);
				report(DR_WRITE,MODULE,errbuf);
				dl_tty[i].flags = (DL_TEST|intloop);
				dl_tty[i].errs++;
				continue;
			}
			if (cmxdebug)
				printf("tested tty%s\n",dl_tty[i].lname);
			dl_tty[i].flags = (DL_TEST|intloop);
			dl_tty[i].cp++;
			if (stoptime && stoptime < time(0))
				cmx_clean();
		    }
		}
	}
}


void cmx_clean()
{
register i;
char buffer[512];
char *bufptr;


	/* print out diagnostic message */
	errptr = errbuf;
	bufptr = buffer;
	sprintf(errptr,"Stopped cmx exerciser - tested:\n");
	(void)strcpy(bufptr,errptr);
	bumpptr(errptr);
	bumpptr(bufptr);
	sprintf(errptr,"\nTTY     WRITES         CC    READS ");
	bumpptr(errptr);
	sprintf(errptr,"        CC   PASSED FAILED\n");
	bumpptr(errptr);
	for (i = 0; i < lcnt; i++)
	{
	    if (dl_tty[i].flags & DL_TEST) {
	        sprintf(errptr,"tty%02s %8d %10d %8d %10d %8d %6d\n",
		    dl_tty[i].lname,dl_tty[i].writes,dl_tty[i].ccw,
		    dl_tty[i].reads,dl_tty[i].ccr,dl_tty[i].cp,dl_tty[i].errs);
	        bumpptr(errptr);
	        if (dl_tty[i].flags & DL_LOOPBACK) {
		    if (ioctl(dl_tty[i].fd, TIOCCMLB) < 0)
			printf("ioctl clear loop back failed\n");
		}
	        close(dl_tty[i].fd);
		sprintf(bufptr,"tty%02s ",dl_tty[i].lname);
		bumpptr(bufptr);
	    }
	}
	sprintf(errptr,"\n");
	report(DR_WRITE,MODULE,errbuf);

	(void)logerr(ELMSGT_DIAG,buffer);

	/* close report logger */
	report(DR_CLOSE,0,DR_CMX);

	exit(0);
}
