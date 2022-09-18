
#ifndef lint
/* static	char	*sccsid = "@(#)diag.h	1.5		4/22/86"; */
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
 *	DIAG.H -- This header file is to be used by the diagnostic routines.
 *
 *	Prefixes:
 *		D_		general diagnostic prefix
 *		DR_		Report generation
 *		DG_		pattern generation
 *		DM_		memory exerciser
 *		DS_		shared memory exerciser
 *		DD_		disk exerciser
 *		DT_		Mag tape exerciser
 *		DL_		Communication Loop back
 *
 */

extern int errno;			/* system error code */
extern char *sys_errlist[];		/* system error messages */
extern char *sys_siglist[];		/* system signal messages */
extern int offset;			/* printer pattern offset */
extern long randx;			/* random number */
extern int logfd;			/* log file descr */
extern int filefd;			/* save log file descr */
extern int tailpid;			/* process ID for "tail" */
extern char filename[];			/* save log file name */
extern char *fileptr;			/* pointer to filename */


#define D_READ		0
#define D_WRITE		1
#define D_RDWRT		2

/*
 *
 * Report generation
 *
 */

/* report indicators */
#define DR_OPEN		1		/* open report generator */
#define DR_WRITE	2		/* write an entry */
#define DR_CLOSE	3		/* close report generator */


/*
 *
 * DIAGLIB structures and defines 
 *
 */

/* Types of patterns for use as input parameter of pattern */
#define DG_5555		1		/* 5555(16) pattern request */
#define DG_AAAA		2		/* AAAA(16) pattern request */
#define DG_PRINT	3		/* printer rolling pattern request */
#define DG_NULLS	4		/* all null char request */
#define DG_RANDM	5		/* randomly generated pattern request */

/* NET structure */
#define DN_MAXNODES 7

struct net_stat {
	char node[10];
	int n_wrts;
	double n_wcc;
	int n_rds;
	double n_rcc;
	int n_pass;
	int n_fail;
};

/* FSX structure */
struct fs_stat {
	int fs_cr;
	int fs_op;
	int fs_wr;
	int fs_rd;
	int fs_cl;
	int fs_ul;
	int fs_pass;
	int fs_fail;
};

#define DF_DEFMAXPROC	20	/* defualt number of fsxr processes */
#define DF_MAXPROC	250	/* maximum number of fsxr processes */

/*
 *
 * DISK STRUCTURES AND DEFINES (DD_) 
 *
 */

/* D_DSKPARM structure and defines  */
struct d_dskparm {
	char d_dev[10];		/* device label */
	short d_flags;			/* flags, see below */
	short d_secsize;		/* size of sector */
	long  d_saddr;			/* super block start address */
	long  d_daddr;			/* data area start address */
};


/* d_rpt -- partition statistic reporting information */

struct d_rpt {
	int d_sk;
	int d_wr;
	double d_wcc;
	int d_rd;
	double d_rcc;
	int d_passed;			/* number passed */
	int d_failed;
	char d_commt[32];		/* comment */
};

/* d_flags values */

#define D_WNCHTR	0x0001		/* disk is winchester */
#define D_NOWRTC	0x8000		/* don't test 'c' partition */

/* Type of disk test values */

#define DD_PART		1		/* partition read/write test */
#define DD_FULL		2		/* read/write on full disk */
#define DD_READ		3		/* read only on full disk */

/* partition overlay definitions */

#define D_PARTa		0x01
#define D_PARTb		0x02
#define D_PARTc		0x04
#define D_PARTd		0x08
#define D_PARTe		0x10
#define D_PARTf		0x20
#define D_PARTg		0x40
#define D_PARTh		0x80

#define D_PARTMSK	0xff

/*
 *
 * MEMORY EXERCISER DEFINES
 *
 */

#define DM_MINMEM	4095		/* minimum amount of memory to be ex*/
#define DM_MAXPROC	20		/* maximum number of memxr processes
					   to be spawned */
#define DM_CLICK	512		/* size of memory click */

/*
 *
 * SHARED MEMORY DEFINES
 *
 */

#define DS_SSET	 	 1
#define DS_SRESET	-1
#define DS_SWAIT	 0
#define DS_NUMSEM	 2
#define DS_MODESEM	 (0600)
#define DS_MODESM	 (0600)
#define DS_PAT1	((char) 0xff)
#define DS_PAT2	((char) 0x34)
#define DS_ZZZZZZ	(2)		/* seconds to sleep */
#define DS_FORKIT	1
#define DS_INFINITE	1	 	/* iterate infinitely */
#define DS_NOLOOPS	5		/* number of interations before sleep */

/*
 *
 * MAGNETIC TAPE DEFINES
 *
 */

#define DT_SHORT	0		/* short record test */
#define DT_LONG		1		/* Long record test */
#define DT_VARLN	2		/* Variable length record test */
#define DT_ALL		3		/* All three tests */
#define DT_SRLEN	512		/* short length record default */
#define DT_RECLN	10240		/* Long length record default */

struct mt_stat {
	int mt_wr;
	double mt_wcc;
	int mt_rd;
	double mt_rcc;
	int mt_passed;
	int mt_failed;
};

/*
 * COMMUNICATION LOOP BACK STRUCTURES AND DEFINES 
 */

#define DL_MAXTTY	32		/* maximum number of TTY permitted on
					   VAX (must be power of 2) */
#define DL_MASTTY	DL_MAXTTY - 1	/* mask for legal range of TTYs */
#define DL_MINTTY	0		/* minimum TTY number to be exercised */
#define DL_PHDLEN	4		/* length of header information in
					   DL_PKT */
#define DL_MAXPKT	256		/* maximum packet length */
					/* maximum data buffer length */
#define DL_MAXBUF	DL_MAXPKT-DL_PHDLEN

/* DL_PKT -- structure for data packet */
struct dl_pkt {
	char ttyline[2];
	short checksum;
	char data[DL_MAXBUF];
};

/* DL_TTY -- structure for TTY exercise information */
struct dl_tty {
	int fd;				/* file descriptor */
	short flags;			/* indicators */
	char lname[4];			/* line name number */
	short baudrate;			/* baudrate */
	short pktlen;			/* length of packet */
	short datalen;			/* number of bytes in data segment */
	short pktcnt;			/* cnt of recv chars */
	char *pktp;			/* pkt char pointer */
	long wtime;			/* time of write */
	struct dl_pkt *pktout;		/* pointer to output data packet */
	struct dl_pkt *pktin;		/* pointer to input data packet */
	int writes;			/* number of successful "writes" */
	int reads;			/* number of successful "reads" */
unsigned int ccw;			/* number of characters written */
unsigned int ccr;			/* number of characters read */
	int cp;				/* number of complete passes */
	int errs;			/*   "    of error for line */
};

/* Flag values */
#define DL_TEST 	0x0001		/* TTY to be exercised */
#define DL_SETUP	0x0002		/* TTY to be exercised */
#define DL_WRITE	0x0004		/* Write has been placed on line */
#define DL_READ		0x0008		/* Read has been received */
#define DL_VALID	0x0010		/* Data has been validated */
#define DL_LOOPBACK	0x0100		/* Loop back on module mode */

					/* indicates Successful pass */
#define DL_CMPLT	(DL_WRITE+DL_READ+DL_VALID)


/*
 *
 *	MACROS
 *
 */

/* macro to bump pointer to null character */
#define bumpptr(a) while ((char *) *(a)) (char *) (a)++

/* pseudo random number generator that calculates a 32 bit random number */
#define rng(a) ((((a) * 1103515245) + 12345))


/* forever looping macro */
#define forever for(;;)

/* printf macro */
#ifdef DEBUG
#define DPRINTF(s,a,b,c,d,e,f) fprintf(stderr, (s),(a),(b),(c),(d),(e),(f))
#else  DEBUG
#define DPRINTF(a,b)
#endif DEBUG
