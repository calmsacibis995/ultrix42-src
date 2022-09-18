
# ifndef lint
static char *sccsid = "@(#)dumprtape.c	4.4	(ULTRIX)	2/14/91";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/************************************************************************
 * 			Modification History				*
 *									*
 * 06 Feb 91 -- lambert							*
 *	Always write to EOT mark on tape unless overridden by user 	*
 *	supplied command line parameter.				*
 *									*
 * 28 OCt 86 -- fries                                                   * 
 *	Added code to set up signal handler for SIGEMT. Rdump to remote *
 *	diskette was giving an EMT Trap(Core Dumped) message after Dump *
 *	is Done Message.                                                *
 *									*
 * 16 OCt 86 -- fries                                                   *
 *	Added code to properly rewind/not rewind tape devices.          *
 *	(based on whether the rewind/no-rewind device was specified)    *
 *									*
 * 18 Sep 86 -- fries                                                   *
 *	Modified code to open normal files correctly.                   *
 *									*
 *  9 Sep 86 -- fries							*
 *	Modified code to use device_open flag to signal device status.  *
 *									*
 * 10 Mar 86 -- fries							*
 *	Added code to determine if output device is tape and report     *
 *	tape rewinding only if it is not at BOT when closed.            *
 *									*
 * 13 Feb 86 -- fries							*
 *	Modified code to output disk messages if output device is	*
 *	a disk device.  						*
 *									*
 * 22 Apr 85 -- dlb							*
 *	Change declaration of dump specific rewind() function to 	*
 *	rewind_tape() to avoid conflict with declaration of rewind()	*
 *	in <stdio.h>.							*
 *									*
 ************************************************************************/

#include "dump.h"
extern int uvaxrx50;

/*
 * tape buffering routines double buffer for remote dump.
 * tblock[1-rotor] is written to remote in tape order
 * as tblock[rotor] is filled in in seek order.
 */

struct	atblock {
	char	tblock[TP_BSIZE];
};
struct atblock *tblock[2]; 	/* Pointers to malloc()ed buffers for tape */
int	writesize;		/* Size of single malloc()ed buffer for tape */
int	trotor = 0;
daddr_t *tdaddr;		/* Pointer to array of disk addrs */
int	toldindex, tcurindex, trecno;
extern int ntrec;		/* blocking factor on tape */
extern int devtyp;		/* device type of output device */

/*
 * Allocate the buffer for tape operations.
 *
 * Depends on global variable ntrec, set from 'b' option in command line.
 * Returns 1 if successful, 0 if failed.
 *
 * For later kernel performance improvement, this buffer should be allocated
 * on a page boundary.
 */
alloctape()
{
	/* Write size for tape/disk writes = blocking factor times */
	/* Size of one tape/disk output block                      */
	writesize = ntrec * TP_BSIZE;

	/* Allocate space for two tape buffers */
	tblock[0] = (struct atblock *)malloc(2 * writesize);
	if (tblock[0] == 0)
		return (0);

	/* Set pointer to second tape buffer */
	tblock[1] = tblock[0]+ntrec;	/* Point to second bigbuffer */

	/* Allocate space for disk block address pointer */
	/* for ntrec blocks			         */
	tdaddr = (daddr_t *)malloc(ntrec * sizeof(daddr_t));
	return (tdaddr != NULL);
}

void rewind_tape()
{
	if ((devtyp == TAP) && (tape[5] != 'n'))
	   msg("Tape rewinding\n");

	rmtclose();
	device_open = 0;
}

taprec(dp)
	char *dp;
{
	register i;
	register struct atblock *bp = tblock[tcurindex];

	tadvance();
	tdaddr[tcurindex] = 0;
	*(&tblock[trotor][tcurindex++]) = *(struct atblock *)dp;

	/* Increment Record # */
	spcl.c_tapea++;

	/* If blocking factor met */
	if (tcurindex >= ntrec)
		flusht();
}

/* Dump disk block */
dmpblk(blkno, size)
	daddr_t blkno; /* Starting Disk Block # */
	int size;      /* number of bytes(multiple of TP_BSIZE) */
{
	int tpblks, dblkno;

	if (size % TP_BSIZE != 0)
		msg("bad size to dmpblk: %d\n", size);

	/* Convert to starting disk block number */
	dblkno = fsbtodb(sblock, blkno);

	/* Dump blocks */
	for (tpblks = size / TP_BSIZE; tpblks > 0; tpblks--) {
		tapsrec(dblkno);
		dblkno += TP_BSIZE / DEV_BSIZE;/* calc. next disk block # */
	}
}

int	nogripe = 0;

/* Write to Output Device */
tadvance()
{
	if (trecno == 0)
		return;
	if (toldindex == 0)
		rmtwrite0(TP_BSIZE * ntrec);

	rmtwrite1((char *)(&tblock[1 - trotor][toldindex++]), TP_BSIZE);

	if (toldindex != ntrec)
		return;
	toldindex = 0;

	/* If Write Error */
	if (rmtwrite2() != writesize) {
		/* IF TAPE EOT ERROR */
		if(errno == ENOSPC){
		  eom_flag = 1;
			
		  /* Allow 1 more write to complete     */
		  rmtioctl(MTCSE,1);

		  /* returns in child */
		}
		else{
			if(devtyp == TAP){
			  msg("Write error on tape %d\n", tapeno);
			  broadcast("TAPE ERROR!\n");
			}
			else{
			  msg("Write error on disk %d\n", tapeno);
			  broadcast("DISK ERROR!\n");
			}
	
			if (query("Restart this volume?") == 0)
			    dumpabort();

			if(devtyp == TAP)
		  	  msg("After this tape rewinds, replace the reel\n");
			else
			  msg("Please remove this disk and replace it with a new one\n");
			msg("and the dump volume will be rewritten.\n");
			close_rewind(0);
			exit(X_REWRITE);
		    }
		}
}

/* Rewind this tape, ask for next tape */
close_rewind(sw)
	int sw;
{
	if (tape[5] == 'n'){
	    rmtioctl(MTREW,1);
	    msg("Tape rewinding\n");
	}
	rewind_tape();
	tnexttape(sw);
}

/* pad out last tape block */
tfillspec()
{

	while (tcurindex)
		spclrec();
}

/* Place disk block number into list, bumping list index */
/* If number of blocks in list >= blocking factor, then  */
/* Output to Output Device. 			         */
tapsrec(d)
	daddr_t d;
{

	if (d == 0)
		return;
	tdaddr[tcurindex] = d;
	tcurindex++;
	spcl.c_tapea++;
	if (tcurindex >= ntrec)
		flusht();
}

/* Flush Output Device Blocks */
flusht()
{
	register i, si;
	daddr_t d;

	while (tcurindex < ntrec)
		tdaddr[tcurindex++] = 1;
loop:
	d = 0;
	for (i=0; i<ntrec; i++)
		if (tdaddr[i] != 0)
			if (d == 0 || tdaddr[i] < d) {
				si = i;
				d = tdaddr[i];
			}
	if (d != 0) {
		tadvance();
		bread(d, (char *)&tblock[trotor][si], TP_BSIZE);
		tdaddr[si] = 0;
		goto loop;
	}
	/* Rewrite last blk */
	if(eom_flag && devtyp == TAP && toldindex == 0)
		tflush(0);
	tcurindex = 0;
	trecno++;
	trotor = 1 - trotor;
	asize += writesize/density;
	asize += 7;
	if (trecno != 1)
		blockswritten += ntrec;

	if(((devtyp == TAP) && ( asize > tsize ) && user_tsize)
	  ||((devtyp == TAP) && (eom_flag))
	  ||((devblocks) && ((devblocks - (blockswritten+10)) < ntrec))){
		tflush(0);
		eom_flag = 0;
		close_rewind(1);
		otape();
		blockswritten = 0;
	}
	timeest();
}

tflush(eof)
	int eof;
{
	int i;

	/* If done, then write END Type Record */
	if (eof) {
		do {
			spclrec();
		} while (tcurindex);
	}
	for (i = 0; i < ntrec; i++)
		tadvance();
}

tnexttape(sw)
	int sw;
{
      if(sw){
      	if(devtyp == TAP){
	    msg("Change Tapes: Mount tape #%d\n", tapeno+1);
	    broadcast("CHANGE TAPES!\7\7\n");
      	}
      	else{
	    msg("Change Disks: Mount disk #%d\n", tapeno+1);
	    broadcast("CHANGE DISKS!\7\7\n");
      	}
      }
      do{
	if(devtyp == TAP){
	   if (query("Next tape ready?") == 1)
	      return;
	}
	else
	   if (query("Next disk ready?") == 1)
		return;

	if (query("Want to abort?") == 1)
		dumpabort();

	}while(1);
}

otape()
{
	int ppid, child, status, tfd;
	int w, interrupt();

	ppid = getpid();
again:
	signal(SIGINT, interrupt);

	child = fork();
	if (child < 0) {
		msg("Context save fork fails in parent %d\n", ppid);
		exit(X_ABORT);
	}
	if (child != 0) {

		/* uVAXrx50: Added to minimize the # of concurrent processes */
		if (uvaxrx50 && getpid() == mpid)
			{
			/* EMT will signal master pid for normal exit,
			 * QUIT will siganl master pid for abort exit.
			 */
			signal(SIGEMT, exitmaster);
			};

		signal(SIGINT, SIG_IGN);
		for (;;) {
			w = wait(&status);
			if (w == child)
				break;
msg("Parent %d waiting for %d has another child %d return\n", ppid, child, w);
		}
		if (status & 0xff)
msg("Child %d returns LOB status %o\n", child, status & 0xff);
		switch ((status >> 8) & 0xff) {

		case X_FINOK:
			exit(X_FINOK);

		case X_ABORT:
			exit(X_ABORT);

		case X_REWRITE:
			rmtclose();
			device_open = 0;
#ifdef notdef
			do {
				if (!query("Retry conection to remote host?"))
					exit(X_ABORT);
				rmtgetconn();
			} while (rmtape < 0);
#endif
			goto again;

		default:
			msg("Bad return code from dump: %d\n", status);
			exit(X_ABORT);
		}
		/*NOTREACHED*/
	}
around:

	if (usecreate){
	   do{
           tfd = rmtopen(tape,O_RDWR);

	/* If device open error... */
	if (tfd < 0){
			  if(!query("Can not create file. Do you want to retry the create?"))
			    dumpabort();
	   	}else break;
	   }while(1);
	}
	else{	
	    tfd = rmtstatchk(tape,O_RDWR);

	    if (tfd == -3)
	       dumpabort();
	}
	asize = 0;
	tapeno++, newtape++;
	trecno = 0;
	spcl.c_volume++;
	spcl.c_type = TS_TAPE;
	spclrec();
	if (tapeno > 1){
	if(devtyp == TAP)
		msg("Tape %d begins with blocks from ino %d\n", tapeno, ino);
	else
		msg("Disk %d begins with blocks from ino %d\n", tapeno, ino);
	}
}

dumpabort()
{

	msg("The ENTIRE dump is aborted.\n");
	exit(X_ABORT);
}

/*
 *	uVAXrx50: Added to minimize the # of concurrent processes.
 *	Catches SIGEMT signal for master pid.  Call Exit with normal
 *	exit status.
 */
exitmaster()
{
	Exit(X_FINOK);
}


Exit(code)
	int code;
{
	exit(code);
}
