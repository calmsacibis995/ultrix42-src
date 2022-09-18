
#ifndef lint
static	char	*sccsid = "@(#)dskx.c	4.1		7/2/90";
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
 *	DSKX.C -- This file contains the disk exerciser modules
 *
 */

/*************************************************************************
 *			Modification History
 *
 * 09/30/89 - Tim Burke
 *	In the dsk_random routine make sure that there is at least one
 *	partition to test to prevent an infinite loop.
 *
 * 11/04/88 - John A. Gallant
 *	Remerged with the V3.0 pool.  Changed the termination of the help
 *	string to "", instead of 0.  The 0 caused a core dump.
 *
 * 8/15/88 - Fred Canter
 *	Added support for 'rz' disks.
 *
 * 7/15/88 - prs
 *      Added char string DR_DSKX to allow proper use with -o option.
 *
 * 3/20/88 - Larry Cohen
 *		Fix seekmsk function to handle larger disks
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <stdio.h>
#include "diag.h"

#define MODULE		"dskx"			/* name of module */
#define MINUTE		60			/* minute in 1 sec counts */
#define SECOND		1 			/* one second */
#define PARTOFFS	5			/* device offset in partition*/
#define MAXERRCT	20			/* maximum number of bad char */
#define MAXPART		8			/* maximum number of disk
						   partitions */

#define TESTREAD	0			/* value for a read only test*/
#define TESTWRT 	1			/* value for a write/read test*/
#define NOPART		0x8000			/* flag value for partition */
#define NOTEST		0x4000			/* not available (vpartmap) */


/*
 *	Global data
 */

char partition[14] = "/dev/";			/* pathname of partition */
char *partid;					/* pointer to partition id 
						   [a-h] in partition */
char tpartition[14]= "/dev/";
char *tpart;

char errbuf[1024];				/* error message buffer */
char *errptr;					/* pointer to err buf */

u_char tstbuf[65536];				/* write message buffer */
u_char rtnbuf[65536];				/* read data buffer */

struct d_dskparm *disk;				/* entry in d_dskparm for 
						   requested disk type */

char *device;					/* ascii device */
int rb, wb;					/* number of chars from 
						   read/write*/
char *help[] =
{
	"\n\n(dskx) - ULTRIX-32 Generic disk exerciser\n",
	"\n",
	"usage:\n",
	"\tdskx [-h] [-ofile] [-t#] [-d#] -pdev#part or -cdev# or -rdev#\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-ofile\t(optional) Save output diagnostics in file\n",
	"-d#\t(optional) Print statistics every # minutes\n",
	"-t#\t(optional) Run time in minutes (default: run forever until cntl-c)\n",
	"-pdev#part\tExercise partition only, write/read\n",
	"-rdev#\t\tExercise random partitions, read only\n",
	"-cdev#\t\tExercise random partitions, write/read\n",
	"\n",
	"dev# - device name raw or buffered and number (i.e. rhp0, ra3, rk1)\n",
	"part - partition [a - h]\n",
	"\n",
	"examples:\n",
	"\tdskx -t60 -cra1 &\n",
	"\t{ Exercise RA type disk, number 1, for 60 min. in background }\n",
	"\tdskx -prhp0d\n",
	"\t{ Exercise raw HP type disk, number 0, partition d, run forever }\n",
	"\n",
	"",
};

int dsk_prtest();
int dsk_setuprw();
int (*func1[])() = {dsk_prtest,dsk_setuprw};
int dsk_read();
int dsk_wrrd();
int (*func2[])() = {dsk_read,dsk_wrrd};

/*
 *
 *	OVLYMAP  -- Partition overlay table.  This table contains the overlay
 *		    information so that when a partition is to be tested, all
 *		    partitions that the given partition encompasses can
 *		    be checked for file systems
 */

short ovlymap[MAXPART] =
{
	D_PARTa,
	D_PARTb,
	D_PARTc,
	D_PARTd,
	D_PARTe,
	D_PARTf,
	D_PARTg,
	D_PARTh,
};

/*
 *
 *	VPARTMAP - Valid partition map table. This table contains data similar
 *		   to ovlymap (same bitmap format), however these entries are
 *		   of the subordinate partitions that passed the file system
 *		   check.  If ovlymap[i] != vpartmap[i], then a write/read
 *		   test will not be performed on the partition.  If the most
 *		   significate bit is set, then the partition is not available.
 */

short vpartmap[MAXPART];


/* 
 *
 *	D_DSKPARM -- This table disk parameter information to the routine
 *		     It contains the ascii file name of the driver, data 
 *		     flags, sector size, start address of superblock, and
 *		     start address of data area
 *
 */

struct d_dskparm d_dskparm[] = {

"cs",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,	
"hp",0x8000,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rk",0x8000,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"ra",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rb",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rl",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rd",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rx",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rz",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rcs",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rhp",0x8000,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rrk",0x8000,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rra",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rrb",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rrl",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,
"rrd",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0, 		/* vaxstation disk */
"rrx",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,		/* vaxstation floppy */
"rrz",0,DEV_BSIZE,SBLOCK*DEV_BSIZE,0,

"",0,0,0,0};

int dskdebug = 0;

/* disk file descriptor */
int fd;
/* report time variables */
long prtdelta;				/* number of minutes between reports
					   (0 = report only at the end) */
long nextprt;				/* time of next report */
/* run time variables */
int timedelta;
long stoptime;

struct d_rpt d_rpt[MAXPART];
struct pt pt;
struct stat st;
int rorw,fschkopt;

char DR_DSKX[] = "#LOG_DSKX_1";			/* Logfile name */

main (argc,argv)
int argc;
char **argv;
{
register i;
register char *partptr;
register testtype;
void dsk_clean();

	/* set up 'kill' signal */
	signal(SIGTERM,dsk_clean);
	signal(SIGINT,dsk_clean);

	if (argc == 1) {
		printf("usage: dskx arg, type \"dskx -h\" for help\n");
		exit(0);
	}

	testtype = -1;

	for (i = 0; i < MAXPART; i++)
		vpartmap[i] = 0;

	/* handle input args */
	while (--argc > 0 && **++argv == '-') {

		switch (*++*argv) {

		case 'p':		/* partition only test */
			/* retrieve and coordinate partition path */
			partptr = partition + PARTOFFS;
			while (*partptr++ = *++*argv);

			/* set up pointer to partition id */
			partid = partptr - 2;

			testtype = 0;
			break;

		case 'c':		/* complete */
			/* retrieve and coordinate partition path */
			partptr = partition + PARTOFFS;
			while (*partptr++ = *++*argv);

			/* set up pointer to partition id */
			partid = partptr - 1;
			*(partid + 1) = 0;
			fschkopt = DD_FULL;
			rorw = TESTWRT;
			testtype = 1;
			break;

		case 'r':		/* read only test */
			/* retrieve and coordinate partition path */
			partptr = partition + PARTOFFS;
			while (*partptr++ = *++*argv);

			/* set up pointer to partition id */
			partid = partptr - 1;
			*(partid + 1) = 0;
			fschkopt = DD_READ;
			rorw = TESTREAD;
			testtype = 1;
			break;

		case 't':		/* run time interval */
			timedelta = atoi(++*argv);
			break;

		case 'd':		/* diagnostic print interval */
			prtdelta = atoi(++*argv);
			prtdelta *= MINUTE;
			nextprt = time(0) + prtdelta;
			break;

		case 'o':		/* save output into file */
			fileptr = filename;
			while (*fileptr++ = *++*argv);
			break;

		case 'h':
			for (i = 0; *help[i]; i++)
				printf("%s",help[i]);
			exit(0);

		default:
			printf("dskx: Invalid arg %s type \"dskx -h\" for help\n",*argv);
			exit(0);
		}
	}

	if (testtype == -1) {
		printf("dksx: No test chosen, type \"dskx -h\" for help\n");
		exit(0);
	}

	if (timedelta)
		stoptime = (timedelta * 60) + time(0);
		
	(*func1[testtype])();

	/* print final statistics and clean up */
	dsk_clean();
}

/*
 *	PARTITION TEST
 */

dsk_prtest()
{
char t;
int errctr = 0;

	/* temp store partitionid (as dsk_fschk will trash it)*/
	t = *partid;

	/*
	 * validate that FS do not currently exist on partition
	 * and if one does, insure that the operator is willing
	 * to trash it
	 */
	if (dsk_fschk(DD_PART,1 << ((*partid - 1) & 0x07))) {
		report(DR_CLOSE,0,DR_DSKX);
		exit(0);
	}
	*partid = t;

	/* If partition not accessable then exit */
	if (vpartmap[--t & 0x07] & NOPART) {
		sprintf(errbuf,"Partition %s not accessable",partition);
		report(DR_WRITE,MODULE,errbuf);
		exit(0);
	}
	t++;

	/* start infinite partition write/read test */
	forever {
		*partid = t;
		if (dsk_wrrd()) {
		    if (++errctr > MAXERRCT)
			break;
		}
		if (tstop())
			break;
		if ((prtdelta != 0) && (nextprt < time(0)))
			dsk_report();
	}
}

/*
 *	SETUP READ or WRITE-READ TEST
 */

dsk_setuprw()
{
	/*
	 * validate that FS do not currently exist on partitions
	 * and if one does, insure that the operator is willing
	 * to trash it
	 */
	if (dsk_fschk(fschkopt,D_PARTMSK)) {
		report(DR_CLOSE,0,DR_DSKX);
		exit(0);
	}

	/* start infinite random write/read test */
	forever {
		if (dsk_random(rorw))
			break;
		if (tstop())
			break;
		if ((prtdelta != 0) && (nextprt < time(0)))
			dsk_report();
	}
}

/**/
/*
 *
 *	DSK_RANDOM - random sector test
 *
 */

dsk_random(testflg)
int testflg;
{
register i,k;
register finish;
int errctr;

	/*
	 * Make sure that there are partitions to test to prevent an
	 * infinite loop from occuring in the do loop below.
	 */
	for (i = 0; i < MAXPART; i++) {
		if (vpartmap[i] & NOPART)
			errctr++;
	}
	if (errctr == MAXPART) {
		report(DR_WRITE,MODULE,"No testable partitions.");
		dsk_clean();
	}
	errctr = 0;
	randx = time(0) & 0xff;
	finish = (rng(randx) & 0x0f) + 1;
	for (i = 0; i < finish; i++) {
		do {
		    randx = rng(randx) & 0xff;
		    *partid = (randx & 0x07) + 0x61;
		    k = (*partid -1) & 0x07;
		} while (ovlymap[k] != vpartmap[k]);
		if ((*func2[testflg & 0x01])()) {
			if (++errctr > MAXERRCT)
				return(1);
		}
		if (tstop())
			break;
		if ((prtdelta != 0) && (nextprt < time(0)))
			dsk_report();
	}
	return(0);
}



/**/
/*
 *
 *	DSK_WRRD -- write/read test
 *
 */

dsk_wrrd()
{
register i,j;
register u_char *tst, *rtn;
int maxsect,rtx,num;
char *errptr;
long itime;
int errct,l,k,rdsize;
int seekmask;

	i = (*partid - 1) & 0x07;

	/* check to see if partition exists */
	if (vpartmap[i] & NOPART)
		return(0);

	/* check to see if partition is testable */
	if (ovlymap[i] != vpartmap[i]) {
		sprintf(errbuf,"%s is not testable",partition);
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}

	/* open partition */
	if ((fd = open(partition,D_RDWRT)) < 0) {
		sprintf(errbuf,"Can not open %s %s\n",partition,sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		d_rpt[i].d_failed++;
		return(1);
	}

	/* initialize random number seed */
	itime = time(0) & 0x1ffff;

	maxsect  = (pt.pt_part[i].pi_nblocks - 1) * DEV_BSIZE;

	/* set up seek mask */
	if ((seekmask = seekmsk(&pt.pt_part[i].pi_nblocks)) == 0) {
		sprintf(errbuf,"seekmskw function failed, part[%d], nblocks=%d",
			i, pt.pt_part[i].pi_nblocks);
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}
	if (dskdebug)
		printf("seekmask %x\n",seekmask);

	if (fstat(fd, &st) < 0) {
		sprintf(errbuf,"Can not get stat information");
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}
	if ((st.st_mode & S_IFMT) == S_IFBLK)
		maxsect -= BLKDEV_IOSIZE;

	/* write random patterns onto random sectors in the partition */
	randx = itime;
	for (l = 0; l < 500; l++) {
	    	rdsize = time(0) & 0x1ff;
	    	rdsize = rng(rdsize) & 0x3f;
	    	rdsize += 1;
	    	rdsize *= DEV_BSIZE;
		if (dskdebug)
	    		printf("rdsize %d\n",rdsize);

		rtx = time(0) & 0x1fff;
		rtx = rng(rtx) & seekmask;
		num = (rtx & 0xf) + 1;
		rtx /= num;
		rtx *= DEV_BSIZE;
		if (rtx + rdsize > maxsect)
		    rtx = maxsect - rdsize;
		if (rtx < 0)
		    rtx = 0;
		if (dskdebug)
			printf("seekblk %d\n",rtx/512);

		pattern(DG_RANDM,rdsize,tstbuf);
		if (lseek(fd, rtx, L_SET) < 0) {
			sprintf(errbuf,"Seek failure: partition %s sector addr = %d, %s\n",partition,rtx/512,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_sk++;
		if ((wb = write(fd, tstbuf, rdsize)) != rdsize) {
			sprintf(errbuf,"Write failure: partition %s sector addr = %d; %s\n	requested bytes = %d; received bytes = %d",partition,rtx/512,sys_errlist[errno],rdsize,wb);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_wr++;
		d_rpt[i].d_wcc += rdsize;
		if (lseek(fd, rtx, L_SET) < 0) {
			sprintf(errbuf,"Seek failure: partition %s sector addr = %d; %s\n",partition,rtx/512,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_sk++;
		if ((rb = read (fd, rtnbuf, rdsize)) != rdsize) {
			sprintf(errbuf,"Read failure: partition %s sector addr = %d; %s\n	requested bytes = %d; received bytes = %d",partition,rtx/512,sys_errlist[errno],rdsize,rb);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_rd++;
		d_rpt[i].d_rcc += rdsize;

		/* check data from partition */
		tst = tstbuf;
		rtn = rtnbuf;
		sprintf(errbuf,"Data error on %s\n",partition);
		errptr = errbuf;
		bumpptr(errptr);
		errct = 0;
		for (j = 0; j < rdsize; j++) {
		    if (*rtn++ != *tst++) {
			if (++errct > MAXERRCT) {
			    sprintf(errptr,"[error printout limit exceeded]\n");
			    break;
			}
			sprintf(errptr,"BLK = %d	BYTE = %d	GOOD = %x	BAD = %x\n",(rtx/512),j,*(tst - 1),*(rtn - 1));
			bumpptr(errptr);
		    }
		}

		/* if data error then report it */
		if (errct) {
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}

		/* tick number passed counter */
		d_rpt[i].d_passed++;

		if (tstop())
			break;
	}

	/* close file and return */
	close (fd);
	return(0);
}


/**/
dsk_read()
{
register i;
int maxsect,sectaddr,rtx,num;
int j,k,rdsize,seekmask;

	/* check to see if partition exists */
	i = (*partid - 1) & 0x07;
	if (vpartmap[i] & NOPART)
		return(0);

	/* check to see if partition is testable */
	if (ovlymap[i] != vpartmap[i]) {
		sprintf(errbuf,"%s is not testable",partition);
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}

	/* open partition */
	if ((fd = open(partition,D_READ)) < 0) {
		sprintf(errbuf,"Can not open %s %s\n",partition,sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		d_rpt[i].d_failed++;
		return(1);
	}

	maxsect  = (pt.pt_part[i].pi_nblocks - 1) * DEV_BSIZE;

	/* set up seek mask */
	if ((seekmask = seekmsk(&pt.pt_part[i].pi_nblocks)) == 0) {
		sprintf(errbuf,"seekmskr function failed, part[%d], nblocks=%d",
			i, pt.pt_part[i].pi_nblocks);
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}
	if (dskdebug)
		printf("seekmask %x\n",seekmask);

	if (fstat(fd, &st) < 0) {
		sprintf(errbuf,"Can not get stat information");
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}
	if ((st.st_mode & S_IFMT) == S_IFBLK)
		maxsect -= BLKDEV_IOSIZE;

	/* read the sector data and report any lseek/read faults */
	rdsize = 8192;
	for (sectaddr = 0; (sectaddr + rdsize) <=  maxsect;
		sectaddr += rdsize) {

		/* perform read on partition */
		if (lseek(fd,sectaddr,L_SET) < 0) {
			sprintf(errbuf,"Seek failure: partition %s sector addr = %d; %s\n",partition,sectaddr/512,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_sk++;
		if ((rb = read (fd,rtnbuf,rdsize)) != rdsize) {
			sprintf(errbuf,"Read failure: partition %s sector addr = %d; %s\n	requested bytes = %d; received bytes = %d",partition,sectaddr/512,sys_errlist[errno],rdsize,rb);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_rd++;
		d_rpt[i].d_rcc += rdsize;
		d_rpt[i].d_passed++;
		if (tstop())
			break;
	}
	for (k = 0; k < 500; k++) {
	    	rdsize = time(0) & 0x1ff;
	    	rdsize = rng(rdsize) & 0x3f;
	    	rdsize += 1;
	    	rdsize *= DEV_BSIZE;
		if (dskdebug)
	    		printf("rdsize %d\n",rdsize);

	        rtx = time(0) & 0x1fff;
		rtx = rng(rtx) & seekmask;
		num = (rtx & 0xf) + 1;
		rtx /= num;
		rtx *= DEV_BSIZE;
		if (rtx + rdsize > maxsect)
		    rtx = maxsect - rdsize;
		if (rtx < 0)
		    rtx = 0;
		if (dskdebug)
			printf("seekblk %d\n",rtx/512);

		if (lseek(fd, rtx, L_SET) < 0) {
			sprintf(errbuf,"Seek failure: partition %s sector addr = %d; %s\n",partition,rtx/512,sys_errlist[errno]);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_sk++;
		if ((rb = read (fd,rtnbuf,rdsize)) != rdsize) {
			sprintf(errbuf,"Read failure: partition %s sector addr = %d; %s\n	requested bytes = %d; received bytes = %d",partition,rtx/512,sys_errlist[errno],rdsize,rb);
			report(DR_WRITE,MODULE,errbuf);
			close(fd);
			d_rpt[i].d_failed++;
			return(1);
		}
		d_rpt[i].d_rd++;
		d_rpt[i].d_rcc += rdsize;
			
		/* tick number passed counter */
		d_rpt[i].d_passed++;

		if (tstop())
			break;
	}

	/* close file and return */
	close (fd);
	return(0);
}

/**/
dsk_fschk(option,request)
int option;				/* type of test requested */
u_char request;				/* bit map of requested partitions*/
{
register i,j,l;
register pos,eofprt;
u_char fsexist,ovrmap,pmask;		/* bit map of partitions that contain
					   file systems */
char *errptr;
char resp[128];				/* operator response character */
struct fs sprblk;			/* super block */

	fsexist = ovrmap = pmask = 0;

	/* open report mechanism */
	if (report(DR_OPEN,MODULE,DR_DSKX)) {
		fprintf(stderr,"%s: Can not start report generator, test aborted/n",MODULE);
		return(1);
	}

	/* check out whether device requested is valid */
	device = partition + PARTOFFS;
	disk = d_dskparm;
	for (;;) {
		if (disk->d_dev[0] == '\0') {
			sprintf(errbuf,"Invalid device %s",device);
			report(DR_WRITE,MODULE,errbuf);
			return (1);
		}
		if (cmpstr(disk->d_dev,device) == 0)
			break;
		disk++;
	}

	/* Can't test 'c' partition of hp or rk devices */
	if ((*partid == 'c') && (disk->d_flags & D_NOWRTC)){
		sprintf(errbuf,"Can't test 'c' partition for device %s",device);
		report(DR_WRITE,MODULE,errbuf);
		return (1);
	}

	/* set a partition if non-given */
	if ((request & 0xff) == 0xff)
		*partid = 0x61;

	/* make sure to use raw device for ptinfo */
	tpart = tpartition + PARTOFFS;
	if (*++device == 'r' || *device == 'h' || *device == 'c') {
		--device;
		while (*tpart++ = *device++);
	}
	else {
		*tpart++ = 'r';
		--device;
		while (*tpart++ = *device++);
	}
	if ((fd = open(tpartition,D_READ)) < 0) {
		sprintf(errbuf,"Can not open %s %s\n",tpartition,sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}
	if (ioctl(fd,DIOCGETPT,&pt) < 0) {
		sprintf(errbuf,"Warning: get pt table ioctl failed: %s\n",
			sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		return(1);
	}
	close(fd);

	/* set up partition mask for overlapping partitions */
	if ((request & 0xff) != 0xff) {
		l = (*partid -1) & 0x07;
		for (i = 0; i < MAXPART; i++) {
		    if (((pt.pt_part[i].pi_blkoff >= pt.pt_part[l].pi_blkoff) &&
		         (pt.pt_part[i].pi_blkoff <= (pt.pt_part[l].pi_blkoff +
		          pt.pt_part[l].pi_nblocks - 1))) ||
		        ((pt.pt_part[l].pi_blkoff >= pt.pt_part[i].pi_blkoff) &&
		         (pt.pt_part[l].pi_blkoff <= (pt.pt_part[i].pi_blkoff +
		          pt.pt_part[i].pi_nblocks - 1))))  
			
		        ovrmap |= 1 << i;
		}
		pmask = ovrmap;
		/*printf("pmask %d\n",pmask);*/
	}
	else {
		pmask = request;
		/*printf("pmask %d\n",pmask);*/
	}

	/* for each partition, insure that it's reachable */
	for (i = 0; i < MAXPART; i++) {
		*partid = i + 0x61;
		if ((fd = open(partition,D_RDWRT)) < 0) {
			if (dskdebug)
				printf("Can't open partition %s not reachable\n",partition);
			vpartmap[i] |= NOPART;
			sprintf(d_rpt[i].d_commt,"No Partition or Not Reachable");
			continue;
		}
		if (fstat(fd, &st) < 0) {
			sprintf(errbuf,"Can not get stat information");
			report(DR_WRITE,MODULE,errbuf);
			return(1);
		}
		eofprt = (st.st_mode & S_IFMT) == S_IFBLK ?
		    ((pt.pt_part[i].pi_nblocks - 1) - BLKDEV_IOSIZE/DEV_BSIZE) *
		    DEV_BSIZE : (pt.pt_part[i].pi_nblocks - 1) * DEV_BSIZE;

		if ((rb = read(fd,rtnbuf,DEV_BSIZE)) == DEV_BSIZE) {
		    if (lseek(fd, eofprt, L_SET) >= 0) {
			if ((rb = read(fd,rtnbuf,DEV_BSIZE)) == DEV_BSIZE) {
			    close(fd);
			    continue;
			}
		    }
		}
		if (dskdebug)
			printf("%s read eofprt %d rb %d\n",partition,eofprt/512,rb);
		vpartmap[i] |= NOPART;
		sprintf(d_rpt[i].d_commt,"No Partition or Not Reachable");
		close(fd);
	}

	for (i = 0; i < MAXPART; i++) {
		if ((request & (1 << i)) == 0) {   /* partition not requested */
			vpartmap[i] |= NOTEST;
			sprintf(d_rpt[i].d_commt,"Partition Not Requested");
		}
	}
	/*
	 * check all partitions assoc with request (requested partition 
	 * and overlays)
	 */
	for (j = 0; j < MAXPART; j++) {
		if ((pmask & (1 << j)) == 0) 
			continue;	/* partition not applicable*/

		/* if partition not avail, then mark vpartmap as ok */
		if (vpartmap[j] & NOPART) {
		/*	vpartmap[j] |= 1 << j;   */
			continue;
		}

		/* append partition id to partition string */
		*partid = j + 0x61;

		/* attempt to open partition */
		if ((fd = open(partition,D_RDWRT)) < 0)
			continue;

		/* if read only test; then partition OK to test */
		if (option == DD_READ) {
			vpartmap[j] |= 1 << j;
			close(fd);
			continue;
		}

		/* find super block */
		if ((pos = lseek(fd, disk->d_saddr, L_SET)) < 0) {
			perror("sbls");
			close(fd);
			continue;
		}

		/* read super block */
		if ((rb = read (fd,(char *)&sprblk,sizeof(sprblk))) != 
						sizeof(sprblk)) {
			perror("spr");
			close(fd);
			continue;
		}

		/* check magic number */
		if (FS_MAGIC == sprblk.fs_magic) {
			fsexist |= 1 << j;
			close(fd);
			continue;
		}

		/* no file system; indicate ok to test sub partition */
		vpartmap[j] |= 1 << j;
		close(fd);
	}

	/* check results of validity check */

	if (fsexist) {
		errptr = errbuf;
		if (ovrmap) {
			sprintf(errptr,
		  "dskx: File system exist on the partition or on an overlapping partition:\n");
		}
		else {
			sprintf(errptr,
		    "dskx: File systems exist on the following partitions:\n");
		}
		for (i = 0; i < MAXPART; i++) {
			if ((fsexist & (1 << i)) == 0)
				continue;
			bumpptr(errptr);
			*partid = i + 0x61;
			sprintf(errptr,"%s\n",partition);
		}
		bumpptr(errptr);
		sprintf(errptr,"WARNING: File system[s] will be destroyed!\n");
		bumpptr(errptr);
		sprintf(errptr,"Continue write/read test? [y,n]> ");
		fprintf(stderr,"%s",errbuf);
		gets(resp);
		if (resp[0] == 'y' || resp[0] == 'Y') {
			errptr = errbuf;
			sprintf(errptr,"ARE YOU SURE? [y,n]> ");
			fprintf(stderr,"%s",errbuf);
			gets(resp);
			if (resp[0] == 'y' || resp[0] == 'Y') {
			    for (i = 0; i < MAXPART; i++) {
				vpartmap[i] |= ovlymap[i] & fsexist;
			    }
			}
		}
		if (resp[0] != 'y' && resp[0] != 'Y') {
			/* this will abort exercise */
			report(DR_WRITE,MODULE,"Exerciser Aborted");
			dsk_clean();
		}
	}

	/* all is setup for test; go for it !!!! */
	*partid = '\0';
	sprintf(errbuf,"Started dskx exerciser - testing: %s",partition + PARTOFFS);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);
	return(0);
}

int seekmsk(nblocks)
int *nblocks;
{
	int i = 0;

	if( *nblocks < 1024 )
		return (0x3ff) ;
 
	for(i = 14; i < sizeof(int) * 8; i++) {
		if( *nblocks < (1 << i))
			return( (1 << i) - 1 ) ;
	}
 
	return (0) ;

}

/* check stop time */
int tstop()
{
	return(stoptime && stoptime < time(0) ? 1 : 0);
}

/*  */
void dsk_clean()
{

	*partid = '\0';
	sprintf(errbuf,"Stopped dskx exerciser - tested: %s",partition + PARTOFFS);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);
	dsk_report();
	report(DR_CLOSE,0,DR_DSKX);
	exit(0);
}


int dsk_report()
{
register i;
register struct d_rpt *r;		/* pointer to part report data */
char p;					/* ASCII partition id */

	*partid = '\0';
	errptr = errbuf;
	sprintf(errptr,"Statistics: %s",partition + PARTOFFS);
	bumpptr(errptr);
	sprintf(errptr,"\n\nPart      Seeks     Writes     Kbytes ");
	bumpptr(errptr);
	sprintf(errptr,"     Reads     Kbytes        Pass  Fail\n");
	bumpptr(errptr);
	p = 0x60;
	r = d_rpt;

	for ( i = 0; i < MAXPART; i++, r++) {
		p++;
		if (vpartmap[i] & NOPART || vpartmap[i] & NOTEST) {
		    sprintf(errptr,"%c     %s\n",p,r->d_commt);
		    bumpptr(errptr);
		}
		else {
		    sprintf(errptr,"%c    %10d %10d %10.1f %10d",p,
		            r->d_sk,r->d_wr,r->d_wcc/1000,r->d_rd);
		    bumpptr(errptr);
		    sprintf(errptr," %10.1f  %10d  %4d\n",r->d_rcc/1000,
		            r->d_passed,r->d_failed);
		    bumpptr(errptr);
		}
	}

	sprintf(errptr,"\n");
	report(DR_WRITE,MODULE,errbuf);

	if (prtdelta != 0) {
		nextprt = time(0) + prtdelta;
	}
}
