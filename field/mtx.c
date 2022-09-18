
#ifndef lint
static char *sccsid = "@(#)mtx.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988, 1989 by		*
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
 * 07/17/89 -- Tim Burke
 *	Fixed up help message to prevent seg faults on risc systems.
 *
 * 12/20/88 -- Tim Burke
 *	Added end of media support for scsi tape devices.
 *
 * 12/18/88 -- Fred Canter
 *	Added MT_ISSCSI for SCSI tapes (TZ30 & TZK50). Added EOTMASK
 *	for SCSI tapes so EOT handling will work.
 *	Limit maximum record size to 16KB for SCSI tapes on VS3100.
 *
 * 7/15/88 -- prs
 *      Added char string DR_MTX to allow proper use with -o option.
 *
 ************************************************************************/
/* USEP File - mtx.c */

/*
 *
 *	MTX.C -- This file contains the Magnetic Tape exerciser modules
 *
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/devio.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include "diag.h"

#define MODULE		"mtx"			/* name of module */
#define DEVOFFS		5			/* offset of device name */
#define MINUTE		60			/* minute in 1 sec counts */
#define SECOND		1 			/* one second */
#define MAXERRCT	10			/* maximum number of bad char */
#define TSEOTMASK 	000001
#define HTEOTMASK 	002000
#define TMEOTMASK 	002000
#define MTEOTMASK 	001000
#define UTEOTMASK 	002000
#define TMSEOTMASK    	(DEV_EOM << 8)
#define SCSEOTMASK    	000002
#define	SCSIEOTMASK	DEV_EOM



/*
 *	Global data
 */

/* routine routing table */
int mt_short();
int mt_long();
int mt_varln();
int (*func[])() = {mt_short,mt_long,mt_varln};

char device[14] = "/dev/";			/* pathname of partition */
char *devid;					/* pointer to partition id 
						   [a-h] in partition */

char errbuf[1024];				/* error message buffer */
char *errptr;

char tstbuf[20480];				/* write message buffer */
char rtnbuf[20480];				/* read data buffer */

int rb, wb;					/* number of chars from 
						   read/write*/
int testctr;					/* number of successful tests*/

char *help[] =
{
	"\n\n(mtx) - ULTRIX-32 Generic Magnetic Tape Exerciser\n",
	"\n",
	"usage:\n",
	"\tmtx [-h] [-t#] [-ofile] [-r#] [-f#] -adev# or -sdev# or -ldev# or -vdev#\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-r#\t(optional) Record length for long record exercise (100-20480)\n",
	"-t#\t(optional) Run time in minutes (default: run forever until cntl-c)\n",
	"-ofile\t(optional) Save output diagnostics in file\n",
	"-f#\t(optional) Size of file in records (EOT = -1; default: -1 EOT)\n",
	"-adev#\tPerform short,long and variable record tests\n",
	"-sdev#\tPerform short record exercise (512 bytes)\n",
	"-ldev#\tPerform long record exercise (10240 bytes)\n",
	"-vdev#\tPerform variable length record exercise (512-20480 bytes)\n",
	"\n",
	"dev# - raw device name,number and high or low density(rmt[0-31][hl])\n",
	"\n",
	"examples:\n",
	"\tmtx -lrmt0l \n",
	"\t{ Perform long record test with record length of 10240 bytes forever }\n",
	"\tmtx -armt1h -t60 &\n",
	"\t{ Perform short,long & variable tests for 60 min. in the background }\n",
	"\n",
	""
};


/* mt file descriptor */
int fd;

/* mtio structures */
struct mtget mtget;
struct mtop mtop;

/* device ioctl get structure */
struct devget devget;

/* mt status structure */
struct mt_stat mtstat;

/* record length */
long reclen;

/* error counters */
int errct;
int errct1;
int errno1;

/* run time variables */
int timedelta;
long stoptime;

/* maximum number of records to be written/read */
long maxrec = 500;
int goto_eot = 1;		/* flag to indicate that reads/writes
				   should continue until EOT */
int mtxdebug = 0;

char DR_MTX[] = "#LOG_MTX_01";	/* Logfile name */

main (argc,argv)
int argc;
char **argv;
{
register i,j;
register char *devptr;
int testtype,alltype;
char t;
void mt_clean();
static char buffer[];

	/* set up 'kill' signal */
	signal(SIGTERM,mt_clean);
	signal(SIGINT,mt_clean);

	/* initialize var */
	testtype = -1;
	reclen = DT_RECLN;
	alltype = 0;
	errct = 0;
	randx = time(0) & 0x1ffff;

	if (argc == 1) {
		printf("usage: mtx arg, type \"mtx -h\" for help\n");
		exit(0);
	}

	/* handle input args */
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {
		case 'a':		/* do all three tests */
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv);
			testtype= DT_SHORT;
			alltype = DT_ALL;
			break;

		case 's':		/* short record test */
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv);

			testtype = DT_SHORT;
			break;

		case 'l':		/* long record exercise */
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv);

			testtype = DT_LONG;
			break;

		case 'v':		/* variable length record test */
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv);

			testtype = DT_VARLN;
			break;

		case 't':		/* run time in minutes */
			timedelta = atoi(++*argv);
			break;

		case 'r':		/* Length of long record */
			reclen = atoi(++*argv);
			if (reclen < 100 || reclen > 20480) {
				printf("mtx: Invalid record length %s\n",*argv);
				exit(0);
			}
			break;

		case 'o':		/* save output into file */
			fileptr = filename;
			while (*fileptr++ = *++*argv);
			break;

		case 'h':
			for (i = 0; *help[i]; i++)
				printf("%s",help[i]);
			exit(0);

		case 'f':
			if ((maxrec = atoi(++*argv)) < 0) {
				goto_eot = 1;
				break;
			}
			else if (maxrec == 0) {
				printf("mtx: Invalid parameter %s\n",*argv);
				exit(0);
			}
			goto_eot = 0;
			break;

		default:
			printf("mtx: Invalid arg %s\n",*argv);
			exit(0);
		}
	}

	/* check to insure that a test was chosen */
	if (testtype == -1) {
		printf("mtx: No test chosen, type \"mtx -h\" for help!\n");
		exit(0);
	}

	/* open logger */
	if (report(DR_OPEN,MODULE,DR_MTX)) {
		fprintf(stderr,"%s: Could not open report logger; test aborted\n",MODULE);
		exit(0);
	}

	sprintf(errbuf,"Started mtx exerciser - testing: %s",device + DEVOFFS);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);

	if (timedelta)
		stoptime = time(0) + (timedelta * 60);

	/* invoke correct routine to begin exercise to loop until error */
	while(((*func[testtype])()) == 0) {
		if (alltype == 3) {
			testtype = testtype == 2 ? 0 : ++testtype;
			if (mtxdebug)
				printf("testtype = %d\n",testtype);
		}
		if (tstop())
			break;
	}

	/* clean up */
	mt_clean();

}


/**/
/*
 *
 *	MT_SHORT
 *
 */

mt_short()
{

	reclen = DT_SRLEN;
	if (mt_wrrd()) {
		if (++errct1 > MAXERRCT) {
			report(DR_WRITE,MODULE,"Too many errors");
			return(1);
		}
	}

	return(0);
}


/**/
/*
 *
 *	MT_LONG 
 *
 */

mt_long()
{

	if (mt_wrrd()) {
		if (++errct1 > MAXERRCT) {
			report(DR_WRITE,MODULE,"Too many errors");
			return(1);
		}
	}

	return(0);
}


/**/
/*
 *
 *	MT_VARLN
 *
 */

mt_varln()
{

	/* set record length at random number between 512 and 20480 bytes */
	if ((reclen = rng(randx) & 0x5fff) > 20480)
		reclen = 20480;
	if (reclen < 512)
		reclen = 512;
	if (mt_wrrd()) {
		if (++errct1 > MAXERRCT) {
			report(DR_WRITE,MODULE,"Too many errors");
			return(1);
		}
	}

	return(0);
}




/**/
/*
 *
 *	MT_WRRD -- write/read test on mag tape
 *
 */

mt_wrrd()
{
register i;
register char *tst, *rtn;
char *errptr;
long itime;
int errct,record;

	/* open device */
	if ((fd = open(device,D_RDWRT|O_NDELAY)) < 0) {
		sprintf(errbuf,"Can not open %s, %s\n",device,
			sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		mtstat.mt_failed++;
		mt_clean();
	}
	if (ioctl(fd,DEVIOCGET,&devget) < 0) {
		sprintf(errbuf,"DEVIOCGET failed: %s\n",
			sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		mtstat.mt_failed++;
		close(fd);
		mt_clean();
	}
	if (devget.stat & DEV_OFFLINE) {
		sprintf(errbuf,"%s device offline!\n",device);
		report(DR_WRITE,MODULE,errbuf);
		close(fd);
		mt_clean();
	}
	if (devget.stat & DEV_WRTLCK) {
		sprintf(errbuf,"%s device write locked!\n",device);
		report(DR_WRITE,MODULE,errbuf);
		close(fd);
		mt_clean();
	}

	/* limit max record size to 16KB for VS3100 SCSI tapes */
	if (strncmp(DEV_VS_SCSI, devget.interface, strlen(DEV_VS_SCSI)) == 0) {
		if (reclen > 16384)
			reclen = 16384;
	}

	/* initialize random number seed */
	itime = time(0) & 0x1ffff;

	/* write random patterned records onto tape */
	randx = itime;
	for (record = 0; (goto_eot || record < maxrec); record++) {
		pattern(DG_RANDM,reclen,tstbuf);

		/* perform write on device */
		if ((wb = write(fd,tstbuf,reclen)) != reclen) {
			errno1 = errno;
			if (chk_eot())
				break;

			/* else error */
			sprintf(errbuf,"Write failure %s; %s;record: %d;\n	requested bytes = %d; received bytes = %d\n	mt_dsreg %x mt_erreg %x",device,sys_errlist[errno1],record,reclen,wb,mtget.mt_dsreg&0xffff,mtget.mt_erreg&0xffff);
			report(DR_WRITE,MODULE,errbuf);
			mtstat.mt_failed++;
			close(fd);
			return(1);
		}
		mtstat.mt_wr++;
		mtstat.mt_wcc += reclen;
	
		/* read the random data back and test it */
		/* back up a record to do read */

		mtop.mt_op = MTBSR;
		mtop.mt_count = 1;
		if (ioctl(fd,MTIOCTOP,&mtop) < 0) {
			if (chk_eot())
				break;
			sprintf(errbuf,"MTIOCTOP failed: %s mt_dsreg %x mt_erreg %x\n",sys_errlist[errno],mtget.mt_dsreg&0xffff,mtget.mt_erreg&0xffff);
			report(DR_WRITE,MODULE,errbuf);
			mtstat.mt_failed++;
			close(fd);
			return(1);
		}
			
		/* perform read on device */
		if ((rb = read (fd,rtnbuf,reclen)) != reclen) {
			
			/* if EOT then break */
			if (rb == 0 && goto_eot)
				break;

			/* else error */
			sprintf(errbuf,"Read failure %s; %s; record: %d\n	requested bytes = %d; received bytes = %d",device,sys_errlist[errno],record,reclen,rb);
			report(DR_WRITE,MODULE,errbuf);
			mtstat.mt_failed++;
			close(fd);
			return(1);
		}
		mtstat.mt_rd++;
		mtstat.mt_rcc += reclen;

		/* check data from device */
		tst = tstbuf;
		rtn = rtnbuf;
		sprintf(errbuf,"Data error on %s\n",device);
		errptr = errbuf;
		bumpptr(errptr);
		errct = 0;
		for (i = 0; i < reclen; i++)
			if (*rtn++ != *tst++) {
				if (++errct > MAXERRCT) {
					sprintf(errptr,"[error printout limit exceeded]\n");
					break;
				}
				sprintf(errptr,"RECORD = %d BYTE = %d GOOD = %x BAD = %x\n",record,i,*(tst - 1)&0xff,*(rtn - 1)&0xff);
				bumpptr(errptr);
			}

		/* if data error then report it */
		if (errct) {
			report(DR_WRITE,MODULE,errbuf);
			mtstat.mt_failed++;
			/* close(fd); */
			/* return(1); */
		}
		else
			mtstat.mt_passed++;
		if (tstop())
			break;
	}

	/* close file and return */
	close (fd);
	return(0);
}

int chk_eot()
{
int got_eot = 0;

	/* if EOT then break */
	if (ioctl(fd,MTIOCGET,&mtget) < 0) {
		sprintf(errbuf,"MTIOCGET failed: %s\n",
			sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		mtstat.mt_failed++;
		return(0);
	}
/*	printf("mt_type %d mt_dsreg %o mt_erreg %o\n",
	      mtget.mt_type,mtget.mt_dsreg,mtget.mt_erreg);
 */		 
	switch(mtget.mt_type) {

		case MT_ISTS:
			if (mtget.mt_erreg & TSEOTMASK)
				got_eot = 1;
			break;
		case MT_ISHT:
			if (mtget.mt_dsreg & HTEOTMASK)
				got_eot = 1;
			break;
		case MT_ISTM:
			if (mtget.mt_erreg & TMEOTMASK)
				got_eot = 1;
			break;
		case MT_ISMT:
			if (mtget.mt_dsreg & MTEOTMASK)
				got_eot = 1;
			break;
		case MT_ISUT:
			if (mtget.mt_dsreg & UTEOTMASK)
				got_eot = 1;
			break;
		case MT_ISTMSCP:
			if (mtget.mt_dsreg & TMSEOTMASK)
				got_eot = 1;
			break;
		case MT_ISST:
			if (mtget.mt_dsreg & SCSEOTMASK)
				got_eot = 1;
			break;
		case MT_ISSCSI:
			if (mtget.mt_dsreg & SCSIEOTMASK)
				got_eot = 1;
			break;
		default:
			break;
	}
	return(got_eot);
}

int tstop()
{
	return(stoptime && stoptime < time(0) ? 1 : 0);
}

void mt_clean()
{
	/* print final message */
	sprintf(errbuf,"Stopped mtx exerciser - tested: %s\n",device+DEVOFFS);
	(void)logerr(ELMSGT_DIAG,errbuf);
	errptr = errbuf;
	bumpptr(errptr);
	sprintf(errptr,"Device       Writes     Kbytes      Reads     Kbytes");
	bumpptr(errptr);
	sprintf(errptr,"     Passed Failed\n");
	bumpptr(errptr);
	sprintf(errptr,"%-8s %10d %10.1f %10d %10.1f %10d %6d\n",device+DEVOFFS,
	        mtstat.mt_wr,mtstat.mt_wcc/1000,mtstat.mt_rd,
	        mtstat.mt_rcc/1000,mtstat.mt_passed,mtstat.mt_failed);
	bumpptr(errptr);
	report(DR_WRITE,MODULE,errbuf);

	/* close logger */
	report(DR_CLOSE,0,DR_MTX);
	exit(0);
}
