# ifndef lint
static char *sccsid = "@(#)dumptape.c	4.4    (ULTRIX)        12/6/90";
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

/* ------------------------------------------------------------------------
 * Modification History: /usr/src/etc/dump/dumptape.c
 *
 * 03 Dec 90 -- lambert
 *      Changed mallocs to vallocs to align I/O buffers on page boundries,
 *	and changed file reading logic in dmpblk() (see header comments in
 *	dmpblk() for more info).
 *
 * 12 Dec 89 -- lambert
 *	Added support in has_loader for DEV_LOADER device characteristic.
 *
 * 11 Aug 89 -- lambert
 *	Changed cacheing checks to use constants instead of integers.
 *
 * 24 Jul 89 -- lambert
 *	Added checks for cache enabled in mtcache_flush and mtcache_off.
 *
 *  5 Jun 89 -- lambert
 *	Added controller cacheing support for tape writes in flusht().
 *
 *  1 May 89 -- lambert
 *	Added TA90 support via next_tape() and has_loader() routines.
 *	Modified code in close_rewind to test/take advantage of auto-loader 
 *	if present.
 *
 *  7 Apr 89 -- lambert
 *	Corrected "eom_flag" processing in flusht() to fix multi-volume bug
 *
 * 12 Sep 88 -- lambert
 *	Rearranged code in close_rewind() to so that multi-voluming works
 *	when dumping to no-rewind tapes.  
 *
 * 22 Aug 88 -- lambert
 *	Added check at end of successful write to tape in flusht() to see
 *	if user supplied length of tape has been written (for 's' key).
 *
 * 16 Oct 86 -- fries
 *	Added code to properly rewind/ not rewind tape devices.
 *	(based on whether the rewind/no-rewind device was specified)
 *
 * 18 Sep 86 -- fries
 *	Modified code to open files correctly.
 *
 *  9 Sep 86 -- fries
 *	Modified code to signal device open or closed using device_open
 *	flag.
 *
 *  9 May 86 -- fries
 *	Added code to support EOT handling.
 *
 * 28 Apr 86 -- lp
 *	Added n-buffered support (which includes read across tape 
 *	blocks for improved input performance). Fixed eot code
 *	to work with n-buffered.
 *
 * 10 Mar 86 -- fries
 *	Added code to not report tape rewinding if it is already at
 *	EOT mark.
 *
 * 13 Feb 86 -- fries
 *	modified messages to report disk device if output device is
 *	a disk device.
 *
 * 22 Apr 85 -- dlb
 *	Change declaration of dump specific rewind() function to 
 *	rewind_tape() to avoid conflict with declaration of rewind()
 *	in <stdio.h>.
 *
 * 28 Feb 85 -- afd
 *	It is necessary to minimize the number of concurrent processes on
 *	the MicroVAX when dumping to rx50's.  The flag "uvaxrx50" is set
 *	if the B flag (dumping to floppies) is specified.  When "uvaxrx50"
 *	is set, processes kill their parent process just before they
 *	fork; the master process waits until the last child signals it
 *	to exit.  Thus there are never more than 3 dump processes active
 *	at a time.  All modifications for this change are restricted to
 *	3 files: dump.h, dumpmain.c, dumptape.c   and are noted by a
 *	comment containing the word: "uVAXrx50".
 *	
 * 18 Dec 84 -- reilly
 *	Replaced a for loop with a structure copy.  This is a performance
 *	enhancement.
 *
 * 23 Apr 84 -- jmcg
 *	To aid in dumping to fixed-size devices, like floppy or cartridge
 *	disks, added a -B flag that specifies the size of the device in
 *	blocks.  This size will override the other size calcualations.
 *	[This is a band-aid; fixing dump to truly understand end-of-media,
 *	and fixing the rest of the system to consistently signal it, was
 *	left for the future.]
 *
 * 23 Apr 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		dumptape.c	1.7 (Berkeley) 5/8/83
 *
 * ------------------------------------------------------------------------
 */

#include "dump.h"
#include <sys/errno.h>
static int cacheing = MTCACHE_OFF; /* Flag for write cacheing */

struct atblock {		/* Pointer to valloc'd buffers for I/O */
	char tblock[TP_BSIZE];
};
struct atblock *fblock[MAXASYNC];
char	*bigbuf;		/* "Overflow" buffer */
int	bbuf_size;		/* Size of bigbuf */

int	writesize;		/* Size of valloc()ed buffer for tape */
int	trecno = 0;		/* present # of tape record */
extern int devtyp;		/* device type(Tape or disk ?)  */
extern int ntrec;		/* blocking factor on tape     */
				/* # of tape records per block */
int	maxrasync=0;		/* Read ahead possible */
int	maxwasync=MAXASYNC; 	/* How many write buffers we use */
int	dowasync=0;		/* Non-blocking writes possible */
int 	curbuf=0;		/* Current buffer we're using */

extern int uvaxrx50;		/* uVAXrx50: dump is on uvax to rx50's */
int	blockcount = 0;		/* blocks written on current tape */
struct mtop mt_com;
struct mtop mt_rew = {MTREW, 1};
struct mtop mt_offline = {MTOFFL, 1};

/*
 * Allocate the buffer for tape operations.
 * ---------------------------------------
 *
 * Depends on global variable ntrec, set from 'b' option in command line.
 * Returns 1 if successful, 0 if failed.
 *
 */
alloctape()
{
	int i;

	writesize = ntrec * TP_BSIZE;

	/* Allocate a large buffer to handle read overflow conditions */
	bbuf_size = writesize * 20L;
	bigbuf = (char *) valloc(bbuf_size);
	if (bigbuf == 0) {
		perror("valloc");
		return(0);
	}

	for (i = 0; i < MAXASYNC; i++) {
		/* Use valloc to provide page-aligned buffers for
		 * increased I/O performance.
		 */
		fblock[i] = ( struct atblock *)valloc(writesize);
		if (fblock[i] == 0)
			return(0);
	}
	return(1);
}

/* Update tape record number, if tape record # > or = to the    */
/* # of blocks to write, then flush tape write buffer to tape   */
taprec(dp)
	char *dp;
{
	register i;

	*(union u_spcl *)(&fblock[curbuf][trecno]) = *(union u_spcl *)dp; 

	trecno++;	/* Bump # of tape block counter */
	spcl.c_tapea++;	/* update record # in header	*/
	if(trecno >= ntrec)	/* If blocking factor met... */
		flusht();	/* Output data to tape	     */
}

#ifdef notdef
int nbufdeep = 0; /* # of read ahead blocks going */
#endif

/*
 * Read blocks from disk and move them to tape.
 *
 * History:  It was determined that dump was providing poor performance
 * on some systems due to the fact that it's I/O buffers were not page
 * aligned.  Data tranfers required the VM subsystem to copy all 
 * non-aligned I/O buffers into aligned ones prior to handing the I/O 
 * request off to the driver.  This copying of each request, combined 
 * with a lack of cache support on some tape devices, slowed the whole 
 * dump operation down to the point that streaming was not being 
 * accomplished when it should be.
 *
 * The reason for the unaligned buffers was that the original 
 * implementor created one large buffer and segmented it into smaller (10K
 * default) tape buffers to do the actual output to tape.  Disk reads could 
 * then be read into this large buffer as long as they would fit, without 
 * concern for buffer boundry conditions, except for the last 10K buffer.  
 * Experimentation with page aligned buffers showed that the number of disk 
 * reads required increased significantly due to the new buffers not being 
 * contiguous with each other (page aligned 10K tape records cannot be 
 * contiguous with each other within the 4kb MIPS pagesize).
 *
 * Current method:  If the requested read will fit in the current tape 
 * (output) buffer, it is performed that way.  Otherwise the data is read 
 * into a seperate buffer, then bcopy-ed into the output buffer.  Testing
 * has shown that this is much faster than issuing the smaller reads neces-
 * sary for the old algorithm.  Overall dump performance is superior on all
 * configurations with this method, and strikingly superior on certain
 * configs (those which originally showed the poorest performance; RISC 
 * 5400s and 5800s with non-caching tape devices).
 *						Sam Lambert, Dec. 1990
 */
dmpblk(blkno, size)
	daddr_t blkno;
	int size;
{
	int avail, tpblks, dblkno, leftover, idx = 0;

	if (size % TP_BSIZE != 0)
		msg("bad size to dmpblk: %d\n", size);
	avail = ntrec - trecno;		/* # of blocks left in this fblock */
	tpblks = size / TP_BSIZE;	/* # of blocks needed to do this I/O */

	dblkno = fsbtodb(sblock, blkno);
	leftover = size;

	/* If amount requested won't fit in current tape record... */
	while (tpblks > avail) {

		/* If it'll fit into bigbuf put it there. */
		if (leftover <= bbuf_size) {
			bread(dblkno, bigbuf, leftover, 0);
			leftover = 0;	/* No leftovers - all fit into bigbuf. */
		}
		else {
			/* Amount requested wouldn't fit in bigbuf. */
			bread(dblkno, bigbuf, bbuf_size, 0);
			leftover -= bbuf_size;		/* Set residual amount. */
#ifdef D_DEBUG
			fprintf(stderr, "*****dmpblk: multiple bigbuf reads needed;\n");
			fprintf(stderr, "*****dmpblk: size = %d, leftover = %d, old dblkno = %d",
				size, leftover, dblkno);
#endif
			/* Adjust pointer to next disk block, based on amount already read. */
			dblkno += (bbuf_size / DEV_BSIZE);
#ifdef D_DEBUG
			fprintf(stderr, ", new dblkno = %d\n", dblkno);
#endif
		}

		/* If amount requested won't fit in current tape record... */
		while (tpblks > avail) {

			/* Copy what will fit into current tape record. */
			bcopy((char *)&bigbuf[TP_BSIZE * idx],
				(char *)&fblock[curbuf][trecno], (TP_BSIZE * avail));

			/* Make adjustments for what was just copied in. */
			idx += avail;
	                trecno += avail;
			spcl.c_tapea += avail;
                	flusht();	/* Flush this record to tape */

			/* Make adjustments for new, empty tape record. */
                	tpblks -= avail;
                	avail = ntrec - trecno;

			/*
			 * The following test will only be true if the bigbuf was completely
			 * full and all data has been copied out of it.  The "break" will get
			 * us out of the inner "while" loop.  If there is more data to be read
			 * for this request the "if" test below will catch it and reiterate the
			 * loop.
			 */
			if ((idx * TP_BSIZE) >= bbuf_size) {
#ifdef D_DEBUG
				fprintf(stderr, "*****dmpblk: taking BREAK; tpblks = %d\n", tpblks);
#endif
				break;
			}
		}

		/* If more to read go to top of loop */
		if (leftover) {
			continue;
		}

		/* Handle case where "tpblks" of data will fit into current tape record. */
		if (tpblks) {
			bcopy((char *)&bigbuf[TP_BSIZE * idx],
				(char *)&fblock[curbuf][trecno], (TP_BSIZE * tpblks));
	        	trecno += tpblks;
			spcl.c_tapea += tpblks;

			/* Flush record to tape, if necessary. */
                	if (trecno >= ntrec) {
				flusht();
			}

			/* 
			 * The following test will probably always be true, but it can't hurt
			 * to check.  If it is false then just go back to the top of the loop
			 * and read any leftover amount.
 			 */
			if (!leftover) {
				return(0);
			}
#ifdef D_DEBUG
			else {
				fprintf(stderr, 
					"*****dmpblk: 'if (!leftover)' test is false; size = %d, leftover = %d\n",
					size, leftover);
			}
#endif
		}
	}
	/* 
	 * Requested read will fit into current tape record.  
	 * Perform read directly into fblock.  
	 * (Don't bother, unless there are tpblks left to be read...)
	 */
	if (tpblks) {
		bread(dblkno, (char *)&fblock[curbuf][trecno], 
			(TP_BSIZE * tpblks), 0);
		trecno += tpblks;
		spcl.c_tapea += tpblks;

		/* Flush record to tape if necessary. */
		if (trecno >= ntrec)
			flusht();
	}
	
	/* And return to caller. */
	return(0);
}

/* Flush Tape data to Drive */
int	nogripe = 0;
int 	wpending[MAXASYNC];

flusht()
{
	register i, si;
	daddr_t d;
	extern int errno;

	errno = trecno = 0;

	/* Turn on cacheing if available and not already on */
	if (cacheing == MTCACHE_OFF) {
		struct mtop mt_com;
		
		mt_com.mt_op = MTCACHE;
		mt_com.mt_count = 0;
		if (ioctl(to, MTIOCTOP, &mt_com) < 0) {
			cacheing = MTCACHE_BAD;
		}
		else cacheing = MTCACHE_ON;
	}
	
rewrite:
	if (write(to, (char *)&fblock[curbuf][0], writesize) != writesize){
harderr:
		if (devtyp == PIP){
			msg("Error while writing to Standard Output\n");
			msg("Cannot recover\n");
			dumpabort();
			/* NOTREACHED */
		}

		if (errno == ENOSPC){
		   eom_flag = 1;
		}
		else{
			/* If cacheing is enabled we must try to flush it */
			if (cacheing == MTCACHE_ON) {
				mtcache_flush();
				goto rewrite;
			}

			if (devtyp == TAP){
			   msg("Tape write error on tape %d\n", tapeno);
			   broadcast("TAPE ERROR!\n");
			}
			else{
			   msg("Disk write error on disk %d\n", tapeno);
			   broadcast("DISK ERROR!\n");
			}
			if (query("Do you want to restart?")){
				if (devtyp == TAP){
				   msg("This tape will rewind.  After it is rewound,\n");
				   msg("replace the faulty tape with a new one;\n");
				}
				else
				   msg("Replace this disk with a new one;\n");

				msg("this dump volume will be rewritten.\n");
				/*
				 *	Temporarily change the tapeno identification
				 */
				tapeno--;
				nogripe = 1;
				close_rewind();
				nogripe = 0;
				tapeno++;
				Exit(X_REWRITE);
			} else {
				dumpabort();
				/*NOTREACHED*/
			}
		}
	}
	asize += writesize/density;
	asize += 7;
	blockswritten += ntrec;
	blockcount += ntrec;

	/* Check to see if 'tsize' ('s' key) length of tape has been written */
	if((eom_flag)||(devblocks && ((devblocks - blockcount) < ntrec))||
	    ((devtyp == TAP) && (user_tsize) && (asize >= tsize))){
		if(dowasync) {
			int k;
			char *tmp;
			for(k=0; k<maxwasync; k++) {
			int p;
			tmp = (char *)&fblock[k][0];
			errno = 0;
			p = wpending[k];
			if(ioctl(to, FIONBDONE, &tmp) != writesize) {
				if(errno == ENOSPC) {
					wpending[k]=1;
				}
			} else
				wpending[k]=0;
			}
			k = 0;
		 	(void) ioctl(to, FIONBUF, &k);
			
 			/* check eom_flag */
			if(devtyp == TAP && eom_flag)
			  wpending[curbuf]=1;
			if(++curbuf >= maxwasync)
				curbuf = 0;
			k = 0;
			while(k++ < maxwasync) {
			/* write these synchronously */
				errno = 0;
				if(wpending[curbuf]){
				 	mt_com.mt_op    = MTCSE;
					mt_com.mt_count = 1;
					if(ioctl(to, MTIOCTOP, &mt_com) < 0)
						perror("write past eom fail");
				errno = 0;
				if(write(to,(char *)&fblock[curbuf][0], 
					writesize) != writesize)
					printf("wrt err %d\n", errno);
				}
				wpending[curbuf]=0;
				if(++curbuf >= maxwasync)
					curbuf = 0;
			}
 			/* check eom_flag */
			} else if(devtyp == TAP && eom_flag){
				   mt_com.mt_op = MTCSE;
				   mt_com.mt_count = 1;
				   if(ioctl(to, MTIOCTOP, &mt_com) < 0)
				   	perror("write past eom fail");
				if (write(to, (char *)&fblock[curbuf][0], 
					writesize) != writesize)
					goto harderr;
			}

		close_rewind();

		/* uVAXrx50: Added to minimize the # of concurrent processes */
		if (uvaxrx50)
			kill(lparentpid, SIGTERM);
		otape(); /* Child returns */
		eom_flag = errno = 0;
		goto cont;
	}

	if(maxwasync) {
		char *tmp;
		wpending[curbuf]++;
		if(++curbuf >= maxwasync)
			curbuf = 0;
		if(dowasync && wpending[curbuf]) {
			errno = 0;
			tmp = (char *)&fblock[curbuf][0];
			if(ioctl(to, FIONBDONE, &tmp) != writesize) {
				printf("iofail %d %d\n", curbuf, ino);
				if(errno != EINVAL)
					goto harderr;
			}
			wpending[curbuf]=0;
		}
	}
cont: timeest();	/* print % of dump completed & elapsed time */
}

/* Rewinds mag tape and waits for completion */
void rewind_tape()
{
	int	secs;
	int f;

	if (devtyp == PIP)
		return;
#ifdef DEBUG
	msg("Waiting 10 seconds to rewind.\n");
	sleep(10);
#else
	/*
	 *	It takes about 3 minutes, 25secs to rewind 2300' of tape
	 */
	/* Perform device ioctl to determine if tape is presently */
	/* at the BOT mark				          */
	if ((devtyp == TAP) && ( strncmp("/dev/rmt", tape, 8) == 0))
	     msg("Tape rewinding\n", secs);
	
	if(dowasync) { /* Turn off n-buffering */
		int k;
		for(k=0; k<maxwasync; k++) {
			char *tmp;
			tmp = (char *)&fblock[k][0];
			(void) ioctl(to, FIONBDONE, &tmp);
		}
		(void) ioctl(to, FIONBUF, &k);
		dowasync = 0;
	}

	if (cacheing == MTCACHE_ON) {
		mtcache_flush();
		mtcache_off();
	}
	close(to);
	device_open = 0;
#endif
}

/* Rewind Tape, wait til done and request a new tape */
close_rewind()
{
	if (devtyp == PIP)
		return;

	blockcount = 0;

	/* Close the tape device and rewind it, if it is a rewind device */
	rewind_tape();

	/* Rewind the no-rewind device */
	if((devtyp == TAP) && (strncmp(tape, "/dev/nrmt", 9) == 0)) {
	  to = open(tape, O_RDONLY);
	  ioctl(to, MTIOCTOP, &mt_rew);
	  close(to);
	}

	if (!nogripe){
		if (devtyp == TAP) {
		   if (!has_loader()) {
			msg("Change Tapes: Mount tape #%d\n", tapeno+1);
			broadcast("CHANGE TAPES!\7\7\n");
		   }
		}
		else{
		   msg("Change Disks: Mount disk #%d\n", tapeno+1);
		   broadcast("CHANGE DISKS!\7\7\n");
		}
	}
	do{
		if (devtyp == TAP){
		   if (next_tape())
		      break;
		}
		else
		   if (query ("Is the new disk mounted and ready to go?"))
		      break;
		if (query ("Do you want to abort?")){
			dumpabort();
			/*NOTREACHED*/
		}
	} while (1);
}

/*
 * Get the next tape loaded.  This consists of prompting the user if the tape
 * unit does not have a loader or if the loader fails to insert a tape.
 *
 * Returns: 1 if a new tape is loaded and ready to go.
 *	    0 if no tape can be loaded; requiring operator intervention.
 */
int
next_tape()
{
	int retries = 0;
	int load_failed = 0;
	int loader_present;
	int tape_ready = 0;

	loader_present = has_loader();
	if (loader_present) {
		/*
		 * If the TA90 is setup in AUTO mode then when the unit is
		 * brought offline the present tape will be placed in the 
		 * output deck and the next available tape in the input
		 * deck will be loaded.
		 */
		if ((to  = open(tape,O_RDONLY)) < 0) {
			return(0);
		}
	  	if ((ioctl(to, MTIOCTOP, &mt_offline)) < 0)
			msg("next_tape: MTOFFLINE failed\n");
	  	close(to);

		/*
		 * Just keep opening the unit until it succeeds.
		 * Only wait a limited timeout period and then prompt
		 * for another tape just as if a loader is not present.
		 *
		 * This scheme of determing when a new tape is ready should
		 * be changed when loader ioctls are ready.  This should use
		 * a loader ioctl which says something like wake me up when
		 * the next tape is ready.
		 *
 		 * Observed TA90 behavior shows that it takes about 24 seconds
		 * to load up a new tape.
		 */
		while (((to  = open(tape,O_RDONLY)) < 0)  && 
			(retries < MAX_RETRY)){
			retries++;
			sleep(1);
		}
		close(to);
		/*
		 * The load did not succeed.  The input deck of new tapes
		 * may be empty.  Give the user a chance to enter the tape.
		 */
		if (retries >= MAX_RETRY) {
			msg("Loader was unable to load the next tape\n");
			load_failed = 1;
		}
		else {
			msg("Loader has successfuly loaded the next tape\n");
			tape_ready = 1;
		}
	}
	/*
	 * If the unit does not have a loader, or the loader failed then
	 * have the user manually mount a tape and continue.
	 */
	if ((loader_present == 0) || (load_failed)) {
		if (query ("Is the new tape mounted and ready to go?")) {
			tape_ready = 1;
		}
	}
	return(tape_ready);
}
/*
 * Determine if the tape unit has a loader.
 *
 * At this time the only device with a loader is the TA90.  When other devices
 * have loaders and loader ioctls are added this routine should change.
 * 
 * Returns	0 - the unit does not have a loader
 *		1 - the unit has a loader
 */
has_loader()
{
	struct devget devget;

	if ((to  = open(tape,O_RDONLY)) < 0) {
		return(NOLOADER);
	}
	/*
	 * See if the unit has a loader.
	 *
	 * When loader ioctls are added use them.  In the meantime, base
	 * loader decision on device name.
 	 */
        if (ioctl(to, DEVIOCGET, (char *)&devget) < 0) {
		/*
		 * DEVIOCGET failed.  Don't assume that the unit has a
		 * loader then.
		 */
		close(to);
		return(NOLOADER);
        }
	else {
		close(to);
		if (
		  (strncmp(devget.device,DEV_TA90,strlen(DEV_TA90)) == 0) ||
 		  (devget.category_stat & DEV_LOADER)
		   ) {
			return(HASLOADER);
		}
		else {
			return(NOLOADER);
		}
	}
}

/*
 *	We implement taking and restoring checkpoints on
 *	the tape level.
 *	When each tape is opened, a new process is created by forking; this
 *	saves all of the necessary context in the parent.  The child
 *	continues the dump; the parent waits around, saving the context.
 *	If the child returns X_REWRITE, then it had problems writing that tape;
 *	this causes the parent to fork again, duplicating the context, and
 *	everything continues as if nothing had happened.
 */
otape()
{
	int	parentpid;
	int	childpid;
	int	status;
	int	waitpid;
	int	sig_ign_parent();
	int	interrupt();

	parentpid = getpid();
	lparentpid = parentpid;		/* uVAXrx50 */

	/* uVAXrx50: Added to minimize the # of concurrent processes */
	if (uvaxrx50 && parentpid != mpid)
	signal(SIGTERM, SIG_DFL);	/* allow processes to exit */

    restore_check_point:
	signal(SIGINT, interrupt);
	/*
	 *	All signals are inherited...
	 */
	childpid = fork();
	if (childpid < 0){
		msg("Context save fork fails in parent %d\n", parentpid);
		Exit(X_ABORT);
	}
	if (childpid != 0){
		/*
		 *	PARENT:
		 *	save the context by waiting
		 *	until the child doing all of the work returns.
		 *	don't catch the interrupt 
		 */

		/* uVAXrx50: Added to minimize the # of concurrent processes */
		if (uvaxrx50 && getpid() == mpid)
			{
			/* EMT will signal master pid for normal exit,
			 * QUIT will siganl master pid for abort exit.
			 */
			signal(SIGEMT, exitmaster);
			signal(SIGQUIT, abortmaster);
			};

		signal(SIGINT, SIG_IGN);
#ifdef TDEBUG
		msg("Tape: %d; parent process: %d child process %d\n",
			tapeno+1, parentpid, childpid);
#endif TDEBUG
		for (;;){
			waitpid = wait(&status);
			if (waitpid != childpid){
				msg("Parent %d waiting for child %d has another child %d return\n",
					parentpid, childpid, waitpid);
			} else
				break;
		}
		if (status & 0xFF){
			/* uVAXrx50: Added to minimize the # of concurrent processes */
			if (uvaxrx50 == 0 || status != SIGTERM)	/* uVAXrx50 */
				msg("Child %d returns LOB status %o\n",
					childpid, status&0xFF);
		}
		status = (status >> 8) & 0xFF;
#ifdef TDEBUG
		switch(status){
			case 0:
				msg("Child %d terminated \n", childpid);
				break;
			case X_FINOK:
				msg("Child %d finishes X_FINOK\n", childpid);
				break;
			case X_ABORT:
				msg("Child %d finishes X_ABORT\n", childpid);
				break;
			case X_REWRITE:
				msg("Child %d finishes X_REWRITE\n", childpid);
				break;
			default:
				msg("Child %d finishes unknown %d\n", childpid,status);
				break;
		}
#endif TDEBUG
		switch(status){
			/* uVAXrx50: Added to minimize the # of concurrent processes */
			/* Child was killed. If master, wait to be signaled */ 
			case 0:
				if (uvaxrx50) {
					if (getpid() == mpid) {
						sleep(mpid,PZERO);
					/* In case of accidental wakeup */
						for (;;) ;
					}
					Exit(X_FINOK);
				} else {
					msg("Bad return code from dump: %d\n", status);
					Exit(X_ABORT);
				};
			case X_FINOK:
				Exit(X_FINOK);
			case X_ABORT:
				Exit(X_ABORT);
			case X_REWRITE:
				goto restore_check_point;
			default:
				msg("Bad return code from dump: %d\n", status);
				Exit(X_ABORT);
		}
		/*NOTREACHED*/
	} else {	/* we are the child; just continue */
#ifdef TDEBUG
		sleep(4);	/* allow time for parent's message to get out */
		msg("Child on Tape %d has parent %d, my pid = %d\n",
			tapeno+1, parentpid, getpid());
#endif
around:
		do{
			if (devtyp == PIP)
				to = 1;
			else
			if (usecreate)
				to = open(tape,O_RDWR|O_CREAT, 0666);
			else
				to = statchk(tape,O_RDWR); /* disk or tape */


			/* If device open error... */
			if (to < 0){
			if(usecreate){
			  if(!query("Can not create file. Do you want to retry the create?"))
			    dumpabort();
			}
			else if (to == -3)dumpabort();

			} else break;
		} while (1);
		{
			int maxcnt = MAXASYNC;
			if(to != 1) {
				if(ioctl(to, FIONBUF, &maxcnt) >= 0) {
					maxwasync = maxcnt;
					dowasync++;
				}
			}
#ifdef notdef
			if(!maxrasync) {
				maxcnt = 2;
				if(ioctl(fi, FIONBUF, &maxcnt) < 0)
					maxrasync = 0;
				else
					maxrasync = maxcnt;
			}
			printf("async status r %d w %d %d\n", maxrasync, maxwasync, dowasync);
#endif
		}

		asize = 0;
		tapeno++;		/* current tape sequence */
		newtape++;		/* new tape signal */
		spcl.c_volume++;
		spcl.c_type = TS_TAPE;
		spclrec();
		if (tapeno > 1){
			if (devtyp == TAP)
			   msg("Tape %d begins with blocks from ino %d\n",
				tapeno, ino);
			else
			   msg("Disk %d begins with blocks from ino %d\n",
				tapeno, ino);
		}
	}
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


/*
 *	uVAXrx50: Added to minimize the # of concurrent processes.
 *	Catches SIGQUIT signal for master pid.  Call Exit with abort
 *	exit status.
 */

abortmaster()
{
	Exit(X_ABORT);
}

/*
 *	The parent still catches interrupts, but does nothing with them
 */
sig_ign_parent()
{
	msg("Waiting parent receives interrupt\n");
	signal(SIGINT, sig_ign_parent);
}

dumpabort()
{
	msg("The ENTIRE dump is aborted.\n");

	/* uVAXrx50: Added to minimize the # of concurrent processes */
	if (uvaxrx50)
		kill(mpid, SIGQUIT);	/* signal master pid to exit */

	Exit(X_ABORT);
}

Exit(status)
{
#ifdef TDEBUG
	msg("pid = %d exits with status %d\n", getpid(), status);
#endif TDEBUG
	exit(status);
}

/* Flush the cache associated with device 'to' */
mtcache_flush()
{
	struct mtop mt_com;
	int flush_cnt;

	/* If we're not using the cache just return. */
	if (cacheing != MTCACHE_ON) return(0);
	
	for (flush_cnt = 0; flush_cnt < 5; flush_cnt++) {
		mt_com.mt_op = MTFLUSH;
		mt_com.mt_count = 0;

		/* Break out of loop if flush succeeds */
		if (ioctl(to, MTIOCTOP, &mt_com) >= 0) break;

 		/* Some devices do not support cacheing, and others
		   support the cache but not the cache flush ioctl. 
		   If the MTFLUSH ioctl isn't supported ENXIO is returned
		   and we can act as if it succeeded. */
		if (errno == ENXIO) break;
	
		/* If write/flush failed then drive is in serious exception 
		   state.  We must clear this state and attempt the flush 
		   before continuing. */
		mt_com.mt_op = MTCSE;
		if (ioctl(to, MTIOCTOP, &mt_com) < 0) {
		    perror("MTIOCTOP - MTCSE failed ");
		    msg("unrecoverable tape error.\n");
		    dumpabort();
		}
				
		/* Only attempt to flush 5 times. */
		if (flush_cnt == 5) {
		    perror("Repeated cache flush errors\n");
		    msg("Continuing with cacheing disabled\n");
		    mtcache_off();
		}
	}				
}

/* Turn off cacheing for device 'to' */
mtcache_off()
{
	struct mtop mt_com;

	/* If we're not using the cache just return. */
 	if (cacheing != MTCACHE_ON) return(0);
	
	/* Turn off cacheing - if it fails we have to abort as we can't
	   count on the state of the controller. */
	mt_com.mt_op = MTNOCACHE;
	mt_com.mt_count = 0;
	if (ioctl(to, MTIOCTOP, &mt_com) < 0) {
		perror("MTIOCTOP - MTNOCACHE failed ");
		msg("cannot continue.\n");
		dumpabort();
	}
	cacheing = MTCACHE_OFF;
}
