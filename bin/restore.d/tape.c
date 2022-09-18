# ifndef lint
static char *sccsid = "@(#)tape.c	4.1    (ULTRIX)        7/2/90";
# endif not lint

/***********************************************************************
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
 **********************************************************************/

/* ---------------------------------------------------------------------
 * Modification History: /usr/src/etc/restore/tape.c
 *
 * 29 Jan 90 -- lambert
 *	Bugfix in for next-volume prompting for TA90 - added "goto" in 
 *	switch statement after calling next_tape().
 *
 * 12-Dec-89 -- lambert
 *	Added support for DEV_LOADER characteristic in has_loader().
 *
 * 28 Aug 89 -- lambert
 *	Fix n-buffered I/O bug by toggling try_async flag in readtape() and
 *	getvol() routines.
 *
 * 11 Aug 89 -- lambert
 *	Modified cacheing routines to use constants instead of integers.
 *
 * 10 Jun 89 -- lambert
 *	Enable use of n-buffered I/O in readtape().
 *
 * 05 Jun 89 -- lambert
 *	Enable use of tape controller cacheing; modules readtape() and
 *	closemt(), new flag "cacheing".
 *
 * 09 Sep 87 -- fries
 *	Reworded volume # prompt message to work from last to first.
 *
 * 01 Sep 87 -- fries
 *	Added code to allow proper restoration of dumped dump
 *	image files(See || header->c_date != dumpdate).
 *
 * 16 Apr 87 -- fries
 *	Added code to allow proper multi-volume functionality of
 *	devices or device partitions whose size is not a multiple
 *	of twenty(blocking factor).
 *
 * 31 Oct 86 -- fries
 *	Added code to not print the What volume message when restoring
 *	from a file.
 *
 *  8 Sep 86 -- fries
 *	Added code to support single device open for both status and
 *	usage.
 *
 * 28 Apr 86 -- lp
 *	Fixed restore of multivolume tapes which run to eot.
 *
 * 08 Oct 85 -- reilly
 *	Named pipes and socket file will not be restored
 *
 * 7) - 15 Mar 85 -- funding							*
 *	Added named pipe support (re. System V named pipes)		*
 *
 * 27 Dec 84 --  reilly
 *	Fix a typo.
 *
 * 03 Dec 84 --  reilly
 *	A fix to the 'R" switch.  This fix came from the net.
 *
 * 23 Apr 84 --  jmcg
 *	To aid in dumping to fixed-size devices, like floppy or cartridge
 *	disks, added a -B flag that specifies the size of the device in
 *	blocks.
 *
 *	Also removed advice to start with last volume of dump.  This fails
 *	when one of the files to be extracted spans an inter-tape gap.
 *
 * 23 Apr 84 -- jmcg
 *	Larry Cohen made this change on Ultrix-32 sources.  It solves a
 *	un-initialized variable problem.
 *
 * 23 Apr 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		tape.c	3.21	(Berkeley)	83/08/11
 *
 * Copyright (c) 1983 Regents of the University of California
 *
 * ---------------------------------------------------------------------
 */

#define NPIPES							/* 7 */

#include "restore.h"
#include <setjmp.h>

static long	fssize = MAXBSIZE;
static int	bct = NTREC+1;
static char	tbf[NTREC*TP_BSIZE];
static char	*nbuf[NBCNT];
static union	u_spcl endoftapemark;
static long	blksread;
static long	tapesread;
static jmp_buf	restart;
static int	gettingfile = 0;	/* restart has a valid frame */

static int	ofile;
static char	*map;
static char	lnkbuf[MAXPATHLEN + 1];
static int	pathlen;
static int	cacheing = MTCACHE_OFF;		/* cache enabled flag */
static int      bufs_allocd = 0;

extern long	devblocks;		/* device size in blocks */
static int	blockcount = 0;		/* blocks read on current tape */

static int	async = 0;		/* n-buf I/O in progress flag */
static int	done_once = 0;		/* flag for readtape routine */
static int	nbidx = 0;		/* index counter for n-buf I/O */
static int	read_done = 0;		/* read complete flag */
static int	pending[NBCNT];
static int 	bufcnt = NBCNT;		/* max # of buffers to use for I/O */
static int	save_errno = 0;		/* storage for errno in readtape */
static int	read_failed = 0;	/* more stuff for n-buf I/O */
static int	try_async = 1;		/* more stuff for n-buf I/O */

struct mtop mt_offline = {MTOFFL, 1};

/*
 * Set up an input source
 */
setinput(source)
	char *source;
{
#ifdef RRESTORE
	char *host;
#endif RRESTORE

	terminal = stdin;
#ifdef RRESTORE
	host = source;
	magtape = index(host, ':');
	if (magtape == 0) {
nohost:
		msg("need keyletter ``f'' and device ``host:tape/disk''\n");
		done(1);
	}
	*magtape++ = '\0';
	if (rmthost(host) == 0)
		done(1);
	setuid(getuid());	/* no longer need or want root privileges */
#else
	if (strcmp(source, "-") == 0) {
		/*
		 * Since input is coming from a pipe we must establish
		 * our own connection to the terminal.
		 */
		terminal = fopen("/dev/tty", "r");
		if (terminal == NULL) {
			perror("open(\"/dev/tty\")");
			done(1);
		}
		pipein++;
	}
	magtape = source;
#endif RRESTORE
}

/*
 * Verify that the tape drive can be accessed and
 * that it actually is a dump tape.
 */
setup()
{
	int i, j, *ip;
	struct stat stbuf;
	extern char *ctime();
	extern int xtrmap(), xtrmapskip();

	switch (devtyp){
	   case TAP:
              vprintf(stdout, "Verify tape and initialize maps\n");
              break;
	   case DSK:
              vprintf(stdout, "Verify disk and initialize maps\n");
	      break;
	   case FIL:
              vprintf(stdout, "Verify file and initialize maps\n");
	      break;
	}

#ifdef RRESTORE
	if(devtyp != FIL)
	  mt = rmtstatchk(magtape,O_RDONLY);
#else
	if (pipein)
 	     mt = 0;
	else
	     if(devtyp != FIL)
	        mt = statchk(magtape, O_RDONLY);
#endif
	if ((devtyp != FIL) && (mt < 0)){
	     if (mt == -3)done(1);
		if(errno)perror(magtape);
		done(1);
	     }
	
	volno = 1;
	setdumpnum();
	flsht();
	if (gethead(&spcl) == FAIL) {
		bct--; /* push back this block */
		cvtflag++;
		if (gethead(&spcl) == FAIL) {
			switch(devtyp){
			case TAP:
			   fprintf(stderr, "Tape is not a dump tape\n");
			   break;
			case DSK:
			   fprintf(stderr, "Disk is not a dump disk\n");
			   break;
			case FIL:
			   fprintf(stderr, "File is not a dump file\n");
			   break;
			}
			done(1);
		}
		fprintf(stderr, "Converting to new file system format.\n");
	}
	if (pipein) {
		endoftapemark.s_spcl.c_magic = cvtflag ? OFS_MAGIC : NFS_MAGIC;
		endoftapemark.s_spcl.c_type = TS_END;
		ip = (int *)&endoftapemark;
		j = sizeof(union u_spcl) / sizeof(int);
		i = 0;
		do
			i += *ip++;
		while (--j);
		endoftapemark.s_spcl.c_checksum = CHECKSUM - i;
	}
	if (vflag || command == 't') {
		fprintf(stdout, "Dump   date: %s", ctime(&spcl.c_date));
		fprintf(stdout, "Dumped from: %s", ctime(&spcl.c_ddate));
	}
	dumptime = spcl.c_ddate;
	dumpdate = spcl.c_date;
	if (stat(".", &stbuf) < 0) {
		perror("cannot stat .");
		done(1);
	}
	fssize = stbuf.st_blksize;
	if (fssize <= 0 || ((fssize - 1) & fssize) != 0) {
		fprintf(stderr, "bad block size %d\n", fssize);
		done(1);
	}
	if (checkvol(&spcl, (long)1) == FAIL) {
		switch(devtyp){
		case TAP:
		   fprintf(stderr, "Tape");
		   break;
		case DSK:
		   fprintf(stderr, "Disk");
		   break;
		}

		fprintf(stderr, " is not volume 1 of the dump\n");
		done(1);
	}
	if (readhdr(&spcl) == FAIL)
		panic("no header after volume mark!\n");
	findinode(&spcl, 1);
	if (checktype(&spcl, TS_CLRI) == FAIL) {
		fprintf(stderr, "Cannot find file removal list\n");
		done(1);
	}
	maxino = (spcl.c_count * TP_BSIZE * NBBY) + 1;
	dprintf(stdout, "maxino = %d\n", maxino);
	map = calloc((unsigned)1, (unsigned)howmany(maxino, NBBY));
	if (map == (char *)NIL)
		panic("no memory for file removal list\n");
	clrimap = map;
	curfile.action = USING;
	getfile(xtrmap, xtrmapskip);
	if (checktype(&spcl, TS_BITS) == FAIL) {
		fprintf(stderr, "Cannot find file dump list\n");
		done(1);
	}
	map = calloc((unsigned)1, (unsigned)howmany(maxino, NBBY));
	if (map == (char *)NULL)
		panic("no memory for file dump list\n");
	dumpmap = map;
	curfile.action = USING;
	getfile(xtrmap, xtrmapskip);
}

/*
 *	Initialize fssize variable for the 'R' command to work.
 */

setup1()
{
	struct stat stbuf;

	if (stat(".", &stbuf) < 0 ) {
		perror("cannot stat .");
		done(1);
	}
	fssize = stbuf.st_blksize;
	if (fssize <= 0 || ((fssize - 1) & fssize) != 0) {
		fprintf(stderr, " bad block size %d\n", fssize);
		done(1);
	}
}

/*
 * Prompt user to load a new dump volume.
 * "Nextvol" is the next suggested volume to use.
 * This suggested volume is enforced when doing full
 * or incremental restores, but can be overrridden by
 * the user when only extracting a subset of the files.
 */
union u_spcl tmp1spcl;
#define tmp1buf tmp1spcl.s_spcl

getvol(nextvol)
	long nextvol;
{
	long newvol;
	long savecnt, i;
	union u_spcl tmpspcl;
#	define tmpbuf tmpspcl.s_spcl

	/* Attempt to use async on the new vol */
 	if (try_async == 0) try_async = 1;
	if (nextvol == 1)
		tapesread = 0;
	if (pipein) {
		if (nextvol != 1)
			panic("Changing volumes on pipe input?\n");
		if (volno == 1)
			return;
		goto gethdr;
	}
	savecnt = blksread;
again:
	if (pipein)
		done(1); /* pipes do not get a second chance */
	if (command == 'R' || command == 'r' || curfile.action != SKIP)
		newvol = nextvol;
	else 
		newvol = 0;
	while (newvol <= 0) {
		if (tapesread == 0) {
			switch(devtyp){
			   case TAP:
			   	fprintf(stderr,"You have not read any tapes yet.\n");
			   	break;
			   case DSK:
			   	fprintf(stderr,"You have not read any disks yet.\n");
				 break;
			}
			if ((devtyp == DSK) || (devtyp == TAP))
			fprintf(stderr, "%s%s%s%s",
			    "Unless you know which volume your",
			    " file(s) are on you should start\n",
			    "with the last volume and work",
			    " forward towards the first.\n");
		} else {
			fprintf(stderr, "You have read volumes");
			strcpy(tbf, ": ");
			for (i = 1; i < 32; i++)
				if (tapesread & (1 << i)) {
					fprintf(stderr, "%s%d", tbf, i);
					strcpy(tbf, ", ");
				}
			fprintf(stderr, "\n");
		}
		if((devtyp == TAP) || (devtyp ==DSK)){
		do	{
			fprintf(stderr, "Specify next volume #: ");
			(void) fflush(stderr);
			(void) fgets(tbf, BUFSIZ, terminal);
		} while (!feof(terminal) && tbf[0] == '\n');
		if (feof(terminal))
			done(1);
		newvol = atoi(tbf);
		}
	        else newvol = 1;	
		if (newvol <= 0) {
			fprintf(stderr,
			    "Volume numbers are positive numerics\n");
		}
	}
	if (newvol == volno) {
		tapesread |= 1 << volno;
		return;
	}
	closemt();
	if((devtyp == TAP) || (devtyp == DSK)){
	   switch(devtyp){
	     case TAP:
 		if (next_tape(newvol))
			goto got_volume;
	        break;
	     case DSK:
                fprintf(stderr, "Mount disk volume %d then type return ", newvol);
	        break;
	   }
	   (void) fflush(stderr);
           while (getc(terminal) != '\n');
	}
got_volume:
#ifdef RRESTORE
	if(devtyp != FIL)
	   mt = rmtstatchk(magtape,O_RDONLY);
#else
	if(devtyp != FIL)
	   mt = statchk(magtape,O_RDONLY);
#endif
	if ((devtyp != FIL) && (mt < 0)){
	if ( mt == -3)done(1);

	switch(devtyp){
	case TAP:
		fprintf(stderr, "Cannot open tape!\n");
		break;
	case DSK:
		fprintf(stderr, "Cannot open disk!\n");
		break;
	}

	if ((devtyp == TAP) || (devtyp == DSK))
	   goto again;
	}
gethdr:
	volno = newvol;
	setdumpnum();
	blockcount = 0;
	flsht();
	if (readhdr(&tmpbuf) == FAIL) {
		switch(devtyp){
		case TAP:
	           fprintf(stderr, "tape is not dump tape\n");
		   break;
		case DSK:
	           fprintf(stderr, "disk is not dump disk\n");
		   break;
		case FIL:
	           fprintf(stderr, "file is not dump file\n");
		   done(1);
		   break;
		}
		volno = 0;
		goto again;
	}
	if (checkvol(&tmpbuf, volno) == FAIL) {
		fprintf(stderr, "Wrong volume (%d)\n", tmpbuf.c_volume);
		volno = 0;
		goto again;
	}
	if (tmpbuf.c_date != dumpdate || tmpbuf.c_ddate != dumptime) {
		fprintf(stderr, "Wrong dump date\n\tgot: %s",
			ctime(&tmpbuf.c_date));
		fprintf(stderr, "\twanted: %s", ctime(&dumpdate));
		volno = 0;
		goto again;
	}
	tapesread |= 1 << volno;
	blksread = savecnt;
	if (curfile.action == USING ) {
		if (volno == 1)
			panic("active file into volume 1\n");
		return;
	}
	(void) gethead(&spcl);
	findinode(&spcl, curfile.action == UNKNOWN ? 1 : 0);
	if (gettingfile) {
		gettingfile = 0;
		longjmp(restart, 1);
	}
}

/*
 * handle multiple dumps per tape by skipping forward to the
 * appropriate one.
 */
setdumpnum()
{
	struct mtop tcom;

	if (dumpnum == 1 || volno != 1)
		return;
	if (pipein) {
		fprintf(stderr, "Cannot have multiple dumps on pipe input\n");
		done(1);
	}
	tcom.mt_op = MTFSF;
	tcom.mt_count = dumpnum - 1;
#ifdef RRESTORE
	rmtioctl(MTFSF, dumpnum - 1);
#else
	if (ioctl(mt, (int)MTIOCTOP, (char *)&tcom) < 0)
		perror("ioctl MTFSF");
#endif
}

extractfile(name)
	char *name;
{
	int mode;
	time_t timep[2];
	struct entry *ep;
	extern int xtrlnkfile(), xtrlnkskip();
	extern int xtrfile(), xtrskip();

	curfile.name = name;
	curfile.action = USING;
	timep[0] = curfile.dip->di_atime.tv_sec;
	timep[1] = curfile.dip->di_mtime.tv_sec;
	mode = curfile.dip->di_mode;
	switch (mode & IFMT) {

	default:
		fprintf(stderr, "%s: unknown file mode 0%o\n", name, mode);
		skipfile();
		return (FAIL);

	case IFPORT:						/* 7 */
		/*
		 *	Don't restore named pipes files.
		 */
		if ( vflag )
			fprintf(stdout,"Not restoring named pipe file %s\n", name);
		skipfile();
		return(GOOD);

	case IFSOCK:
		/*
		 *	Don't restore socket files.
		 */
		if ( vflag )
			fprintf(stdout,"Not restoring socket file %s\n", name);
		skipfile();
		return(GOOD);

	case IFDIR:
		if (mflag) {
			ep = lookupname(name);
			if (ep == NIL || ep->e_flags & EXTRACT)
				panic("unextracted directory %s\n", name);
			skipfile();
			return (GOOD);
		}
		vprintf(stdout, "extract file %s\n", name);
		return (genliteraldir(name, curfile.ino));

	case IFLNK:
		lnkbuf[0] = '\0';
		pathlen = 0;
		getfile(xtrlnkfile, xtrlnkskip);
		if (pathlen == 0) {
			vprintf(stdout,
			    "%s: zero length symbolic link (ignored)\n", name);
		} else if (symlink(lnkbuf, name) < 0) {
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create symbolic link");
			return (FAIL);
		} else
			vprintf(stdout, "extract symbolic link %s\n", name);
		return (GOOD);

	case IFCHR:
	case IFBLK:
		vprintf(stdout, "extract special file %s\n", name);
		if (mknod(name, mode, (int)curfile.dip->di_rdev) < 0) {
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create special file");
			skipfile();
			return (FAIL);
		}
		(void) chown(name, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) chmod(name, mode);
		skipfile();
		utime(name, timep);
		return (GOOD);

	case IFREG:
		vprintf(stdout, "extract file %s\n", name);
		if ((ofile = creat(name, 0666)) < 0) {
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create file");
			skipfile();
			return (FAIL);
		}
		(void) fchown(ofile, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) fchmod(ofile, mode);
		getfile(xtrfile, xtrskip);
		(void) close(ofile);
		utime(name, timep);
		return (GOOD);
	}
	/* NOTREACHED */
}

/*
 * skip over bit maps on the tape
 */
skipmaps()
{

	while (checktype(&spcl, TS_CLRI) == GOOD ||
	       checktype(&spcl, TS_BITS) == GOOD)
		skipfile();
}

/*
 * skip over a file on the tape
 */
skipfile()
{
	extern int null();

	curfile.action = SKIP;
	getfile(null, null);
}

/*
 * Do the file extraction, calling the supplied functions
 * with the blocks
 */
getfile(f1, f2)
	int	(*f2)(), (*f1)();
{
	register int i;
	int curblk = 0;
	off_t size = spcl.c_dinode.di_size;
	static char clearedbuf[MAXBSIZE];
	char buf[MAXBSIZE / TP_BSIZE][TP_BSIZE];

	if (checktype(&spcl, TS_END) == GOOD)
		panic("ran off end of tape\n");
	if (ishead(&spcl) == FAIL)
		panic("not at beginning of a file\n");
	if (!gettingfile && setjmp(restart) != 0)
		return;
	gettingfile++;
loop:
	for (i = 0; i < spcl.c_count; i++) {
		if (spcl.c_addr[i]) {
			readtape(&buf[curblk++][0]);
			if (curblk == fssize / TP_BSIZE) {
				(*f1)(buf, size > TP_BSIZE ?
				     (long) (fssize) :
				     (curblk - 1) * TP_BSIZE + size);
				curblk = 0;
			}
		} else {
			if (curblk > 0) {
				(*f1)(buf, size > TP_BSIZE ?
				     (long) (curblk * TP_BSIZE) :
				     (curblk - 1) * TP_BSIZE + size);
				curblk = 0;
			}
			(*f2)(clearedbuf, size > TP_BSIZE ?
				(long) TP_BSIZE : size);
		}
		if ((size -= TP_BSIZE) <= 0)
			break;
	}
	if (readhdr(&spcl) == GOOD && size > 0) {
		if (checktype(&spcl, TS_ADDR) == GOOD)
			goto loop;
		dprintf(stdout, "Missing address (header) block for %s\n",
			curfile.name);
	}
	if (curblk > 0)
		(*f1)(buf, (curblk * TP_BSIZE) + size);
	findinode(&spcl, 1);
	gettingfile = 0;
}

/*
 * The next routines are called during file extraction to
 * put the data into the right form and place.
 */
xtrfile(buf, size)
	char	*buf;
	long	size;
{

	if (write(ofile, buf, (int) size) == -1) {
		fprintf(stderr, "write error extracting inode %d, name %s\n",
			curfile.ino, curfile.name);
		perror("write");
		done(1);
	}
}

xtrskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf;
#endif
	if (lseek(ofile, size, 1) == (long)-1) {
		fprintf(stderr, "seek error extracting inode %d, name %s\n",
			curfile.ino, curfile.name);
		perror("lseek");
		done(1);
	}
}

xtrlnkfile(buf, size)
	char	*buf;
	long	size;
{

	pathlen += size;
	if (pathlen > MAXPATHLEN) {
		fprintf(stderr, "symbolic link name: %s->%s%s; too long %d\n",
		    curfile.name, lnkbuf, buf, pathlen);
		done(1);
	}
	(void) strcat(lnkbuf, buf);
}

xtrlnkskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf, size = size;
#endif
	fprintf(stderr, "unallocated block in symbolic link %s\n",
		curfile.name);
	done(1);
}

xtrmap(buf, size)
	char	*buf;
	long	size;
{

	bcopy(buf, map, size);
	map += size;
}

xtrmapskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf;
#endif
	panic("hole in map\n");
	map += size;
}

null() {;}

/*
 * Do the tape i/o, dealing with volume changes
 * etc..
 */
readtape(b)
	char *b;
{
	register long i;
	long newvol, rd;
	int cnt, n;
	int repost = 0;

#ifndef RRESTORE
 	/* Only attempt the following on local devices */
	if (cacheing == MTCACHE_OFF) {
		struct mtop mt_com;
		
		mt_com.mt_op = MTCACHE;
		mt_com.mt_count = 0;
		if (ioctl(mt, MTIOCTOP, &mt_com) < 0) {
			cacheing = MTCACHE_BAD;
		}
		else cacheing = MTCACHE_ON;
	}
	
#endif
	/* Turn on n-buffered I/O for this device */
	if ((try_async == 1) && !async) {
		if (ioctl(mt, FIONBUF, &bufcnt) < 0) {
			bufcnt = 1;
			try_async = -1;
		}
		else {
			async++;
 			done_once = 0;
			read_done = 0;
			read_failed = 0;
		}
	}
	/* Allocate buffers */
        if (!bufs_allocd) {
		for (i = 0; i < bufcnt; i++)
			nbuf[i] = (char *)malloc(NTREC*TP_BSIZE);
                bufs_allocd++;
	}
	if (bct >= NTREC) {
 		if (async) {

	 	 	/* Starting operations on "next" buffer */
			if (nbidx == (bufcnt-1)) {
				nbidx = 0;
			}
			else if (done_once) nbidx++;
		}
		else nbidx = 0;		

next_buf:
 		if (pending[nbidx] == 0) {
		    for (i = 0; i < NTREC; i++)
		    	((struct s_spcl *)(nbuf[nbidx]+(i*TP_BSIZE)))->c_magic = 0;
 		}

		if (!repost) bct = 0;
		cnt = NTREC*TP_BSIZE;
		rd = 0;
		/* check for end-of-media when device size has been given */
		if( devblocks && blockcount >= devblocks) {
nextv:
			newvol = volno + 1;
			volno = 0;
			getvol(newvol);
			readtape(b);
			return;
		}
	getmore:
		errno = 0;
#ifdef RRESTORE
		i = rmtread((nbuf[nbidx]+rd), cnt);
#else
		/* If nothing is pending on this buffer then post a read,
		   placing the return value of the read call into pending[]. */
		if (pending[nbidx] == 0) {
			i = read(mt, (nbuf[nbidx]+rd), cnt);
			if (async) {
				pending[nbidx] = i;
 				read_done = 0;
 			}
		}

		/* Otherwise complete the pending buffer. */
		else if (async && (pending[nbidx] > 0)) {
			i = ioctl(mt, FIONBDONE, &nbuf[nbidx]);
			read_done++;
		}

		/* Read error on previous posting of this buf. */
		else i = pending[nbidx];
#endif
		/* Check value of returned status. */
		if (async) {

			/* If buf was just completed and the wrong # of
			   bytes were returned that means all outstanding
			   bufs are invalid and must be trashed. */
			if (read_done && (i != NTREC*TP_BSIZE)) {
				async = 0;
				nbidx = 0;
				(void) ioctl(mt, FIONBUF, &nbidx);
			   	for (n = 0; n < bufcnt; n++)
					pending[n] = 0;
 				if (read_failed) errno = save_errno;
			}

			/* Successful completion of buffer. */
			else if (read_done) pending[nbidx] = 0; 

			/* Read was just posted for this buf, not completed. */
			else {

				/* Successful posting of read to this buf. */
				if (pending[nbidx] == cnt) {

				    /* Note: RETURN done here if read is
				       successfully posted for this buf and it's
				       not the "first time" through the loop. */
				    if (done_once) return;

				    /* Otherwise just drop thru. */
				    goto skip_checks;
				} 					

				/* If previous branch not taken it indicates
				   a read error on reposting a buf.  All
				   outstanding bufs must be completed and 
				   returned in order, but no more reads can
				   be posted. */
				read_failed++;
				try_async = 0;
								   
				/* Save context for next access of this buf */
				save_errno = errno;
 				return;
			}
		}
		
		if (i > 0 && i != NTREC*TP_BSIZE) {
			if (!pipein)
				panic("partial block read: %d should be %d\n",
					i, NTREC*TP_BSIZE);
			rd += i;
			cnt -= i;
			if (cnt > 0)
				goto getmore;
			i = rd;
		}
		if (i < 0) {
			if(errno == ENOSPC){
			struct mtop mt_com;

			/* This is to handle any partitions or */
			/* device sizes not a multiple of the  */
			/* 20 block blocking factor...         */
			if(devtyp == DSK)
			  goto nextv;

#ifndef RRESTORE

			mt_com.mt_op = MTCSE;
			mt_com.mt_count = 1;
			if(ioctl(mt, MTIOCTOP, &mt_com) < 0)
				perror("read after eom failed");
#else
		        rmtioctl(MTCSE,1);
#endif
			  eomflag = 1;
			  goto getmore;
			}
			else{
				switch(devtyp){
				  case TAP:
				   fprintf(stderr, "Tape");
				   break;
				  case DSK:
				   fprintf(stderr, "Disk");
				   break;
				  case FIL:
				   fprintf(stderr, "File");
				   break;
				}

				fprintf(stderr, " read error while ");
				switch (curfile.action) {
				default:
				switch(devtyp){
				  case TAP:
					   fprintf(stderr, "trying to set up tape\n");
				   break;
				  case DSK:
					   fprintf(stderr, "trying to set up disk\n");
				   break;
				  case FIL:
					   fprintf(stderr, "trying to set up file\n");
				   break;
				}
					break;
				case UNKNOWN:
					fprintf(stderr, "trying to resyncronize\n");
					break;
				case USING:
					fprintf(stderr, "restoring %s\n", curfile.name);
					break;
				case SKIP:
					fprintf(stderr, "skipping over inode %d\n",
						curfile.ino);
					break;
				}
				if (!yflag && !query("continue"))
					done(1);
				i = NTREC*TP_BSIZE;
				bzero(nbuf[nbidx], i);
#ifdef RRESTORE
				if (rmtseek(i, 1) < 0)
#else
				if (lseek(mt, i, 1) == (long)-1)
#endif
				{
					perror("continuation failed");
					done(1);
				}
			   }
			}
		if (i == 0) {
			if (pipein) {
				bcopy((char *)&endoftapemark, b,
					(long)TP_BSIZE);
				flsht();
				return;
			}
			newvol = volno + 1;
			volno = 0;
			getvol(newvol);
			readtape(b);
			eomflag = 0;
			return;
		}
	}
skip_checks:
	if (async) {

		/* Set done_once if a read has been completed. */
		if (read_done && !done_once) done_once++;

		/* First time thru post all bufs. */
		if (!done_once) {

			/* If already posted all bufs go complete buf 0. */
			if (nbidx == (bufcnt-1)) {
				nbidx = 0;
				goto getmore;
			}

			/* Otherwise inc. nbidx and post read on "next" buf. */
			nbidx++;
			goto next_buf;
		}

	}
	bcopy((nbuf[nbidx]+(bct++*TP_BSIZE)), b, (long)TP_BSIZE);
	blksread++;
	blockcount++;		/* blocks read on this tape */

	/* Repost this buf after walking through bct. */
	if ((async) && (bct == NTREC) && (!read_failed)) {
		repost++;
		goto next_buf;
	}
}

flsht()
{

	bct = NTREC+1;
}

closemt()
{

	device_open = 0;

	if (mt < 0)
		return;
#ifdef RRESTORE
	rmtclose();
#else
 	if (cacheing == MTCACHE_ON) {
		struct mtop mt_com;
		
		mt_com.mt_op = MTNOCACHE;
		mt_com.mt_count = 0;
		(void) ioctl(mt, MTIOCTOP, &mt_com);

 		cacheing = MTCACHE_OFF;
	}			
	(void) close(mt);
#endif
}

checkvol(b, t)
	struct s_spcl *b;
	long t;
{

	if (b->c_volume != t)
		return(FAIL);
	return(GOOD);
}

readhdr(b)
	struct s_spcl *b;
{

	if (gethead(b) == FAIL) {
		dprintf(stdout, "readhdr fails at %d blocks\n", blksread);
		return(FAIL);
	}
	return(GOOD);
}

/*
 * read the tape into buf, then return whether or
 * or not it is a header block.
 */
gethead(buf)
	struct s_spcl *buf;
{
	long i;
	union u_ospcl {
		char dummy[TP_BSIZE];
		struct	s_ospcl {
			long	c_type;
			long	c_date;
			long	c_ddate;
			long	c_volume;
			long	c_tapea;
			u_short	c_inumber;
			long	c_magic;
			long	c_checksum;
			struct odinode {
				unsigned short odi_mode;
				u_short	odi_nlink;
				u_short	odi_uid;
				u_short	odi_gid;
				long	odi_size;
				long	odi_rdev;
				char	odi_addr[36];
				long	odi_atime;
				long	odi_mtime;
				long	odi_ctime;
			} c_dinode;
			long	c_count;
			char	c_addr[256];
		} s_ospcl;
	} u_ospcl;

	if (!cvtflag) {
		readtape((char *)buf);
		if (buf->c_magic != NFS_MAGIC || checksum((int *)buf) == FAIL)
			return(FAIL);
		goto good;
	}
	readtape((char *)(&u_ospcl.s_ospcl));
	bzero((char *)buf, (long)TP_BSIZE);
	buf->c_type = u_ospcl.s_ospcl.c_type;
	buf->c_date = u_ospcl.s_ospcl.c_date;
	buf->c_ddate = u_ospcl.s_ospcl.c_ddate;
	buf->c_volume = u_ospcl.s_ospcl.c_volume;
	buf->c_tapea = u_ospcl.s_ospcl.c_tapea;
	buf->c_inumber = u_ospcl.s_ospcl.c_inumber;
	buf->c_checksum = u_ospcl.s_ospcl.c_checksum;
	buf->c_magic = u_ospcl.s_ospcl.c_magic;
	buf->c_dinode.di_mode = u_ospcl.s_ospcl.c_dinode.odi_mode;
	buf->c_dinode.di_nlink = u_ospcl.s_ospcl.c_dinode.odi_nlink;
	buf->c_dinode.di_uid = u_ospcl.s_ospcl.c_dinode.odi_uid;
	buf->c_dinode.di_gid = u_ospcl.s_ospcl.c_dinode.odi_gid;
	buf->c_dinode.di_size = u_ospcl.s_ospcl.c_dinode.odi_size;
	buf->c_dinode.di_rdev = u_ospcl.s_ospcl.c_dinode.odi_rdev;
	buf->c_dinode.di_atime.tv_sec = u_ospcl.s_ospcl.c_dinode.odi_atime;
	buf->c_dinode.di_mtime.tv_sec = u_ospcl.s_ospcl.c_dinode.odi_mtime;
	buf->c_dinode.di_ctime.tv_sec = u_ospcl.s_ospcl.c_dinode.odi_ctime;
	buf->c_count = u_ospcl.s_ospcl.c_count;
	bcopy(u_ospcl.s_ospcl.c_addr, buf->c_addr, (long)256);
	if (u_ospcl.s_ospcl.c_magic != OFS_MAGIC ||
	    checksum((int *)(&u_ospcl.s_ospcl)) == FAIL)
		return(FAIL);
	buf->c_magic = NFS_MAGIC;

good:
	switch (buf->c_type) {

	case TS_CLRI:
	case TS_BITS:
		/*
		 * Have to patch up missing information in bit map headers
		 */
		buf->c_inumber = 0;
		buf->c_dinode.di_size = buf->c_count * TP_BSIZE;
		for (i = 0; i < buf->c_count; i++)
			buf->c_addr[i]++;
		break;

	case TS_TAPE:
	case TS_END:
		buf->c_inumber = 0;
		break;

	case TS_INODE:
	case TS_ADDR:
		break;

	default:
		panic("gethead: unknown inode type %d\n", buf->c_type);
		break;
	}
	if (dflag)
		accthdr(buf);
	return(GOOD);
}

/*
 * Check that a header is where it belongs and predict the next header
 */
accthdr(header)
	struct s_spcl *header;
{
	static ino_t previno = 0x7fffffff;
	static int prevtype;
	static long predict;
	long blks, i;

	if (header->c_type == TS_TAPE) {
		fprintf(stderr, "Volume header\n");
		return;
	}
	if (previno == 0x7fffffff)
		goto newcalc;
	switch (prevtype) {
	case TS_BITS:
		fprintf(stderr, "Dump mask header");
		break;
	case TS_CLRI:
		fprintf(stderr, "Remove mask header");
		break;
	case TS_INODE:
		fprintf(stderr, "File header, ino %d", previno);
		break;
	case TS_ADDR:
		fprintf(stderr, "File continuation header, ino %d", previno);
		break;
	case TS_END:
		switch(devtyp){
		   case TAP:
		      fprintf(stderr, "End of tape header");
		      break;
		   case DSK:
		      fprintf(stderr, "End of disk header");
		      break;
		   case FIL:
		      fprintf(stderr, "End of file header");
		      break;
		}
		break;
	}
	if (predict != blksread - 1)
		fprintf(stderr, "; predicted %d blocks, got %d blocks",
			predict, blksread - 1);
	fprintf(stderr, "\n");
newcalc:
	blks = 0;
	if (header->c_type != TS_END)
		for (i = 0; i < header->c_count; i++)
			if (header->c_addr[i] != 0)
				blks++;
	predict = blks;
	blksread = 0;
	prevtype = header->c_type;
	previno = header->c_inumber;
}

/*
 * Find an inode header.
 * Complain if had to skip, and complain is set.
 */
findinode(header, complain)
	struct s_spcl *header;
	int complain;
{
	static long skipcnt = 0;

	curfile.name = "<name unknown>";
	curfile.action = UNKNOWN;
	curfile.dip = (struct dinode *)NIL;
	curfile.ino = 0;
	if (ishead(header) == FAIL) {
		skipcnt++;
		while (gethead(header) == FAIL || header->c_date != dumpdate)
			skipcnt++;
	}
	for (;;) {
		if (checktype(header, TS_INODE) == GOOD) {
			curfile.dip = &header->c_dinode;
			curfile.ino = header->c_inumber;
			break;
		}
		if (checktype(header, TS_END) == GOOD) {
			curfile.ino = maxino;
			break;
		}
		if (checktype(header, TS_CLRI) == GOOD) {
			curfile.name = "<file removal list>";
			break;
		}
		if (checktype(header, TS_BITS) == GOOD) {
			curfile.name = "<file dump list>";
			break;
		}
		while (gethead(header) == FAIL)
			skipcnt++;
	}
	if (skipcnt > 0 && complain)
		fprintf(stderr, "resync restore, skipped %d blocks\n", skipcnt);
	skipcnt = 0;
}

/*
 * return whether or not the buffer contains a header block
 */
ishead(buf)
	struct s_spcl *buf;
{

	if (buf->c_magic != NFS_MAGIC)
		return(FAIL);
	return(GOOD);
}

checktype(b, t)
	struct s_spcl *b;
	int	t;
{

	if (b->c_type != t)
		return(FAIL);
	return(GOOD);
}

checksum(b)
	register int *b;
{
	register int i, j;

	j = sizeof(union u_spcl) / sizeof(int);
	i = 0;
	do
		i += *b++;
	while (--j);
	if (i != CHECKSUM) {
		fprintf(stderr, "Checksum error %o, inode %d file %s\n", i,
			curfile.ino, curfile.name);
		return(FAIL);
	}
	return(GOOD);
}

/* VARARGS1 */
msg(cp, a1, a2, a3)
	char *cp;
{

	fprintf(stderr, cp, a1, a2, a3);
}

/*
 * Get the next tape loaded.  This consists of prompting the user if the tape
 * unit does not have a loader or if the loader fails to insert a tape.
 *
 * Returns: 1 if a new tape is loaded and ready to go.
 *	    0 if no tape can be loaded; requiring operator intervention.
 */
int
next_tape(newvol)
 	long newvol;
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
		if ((mt  = open(magtape,O_RDONLY)) < 0) {
			return(0);
		}
	  	if ((ioctl(mt, MTIOCTOP, &mt_offline)) < 0) {
			fprintf(stderr, "next_tape: MTOFFLINE failed\n");
			fflush(stderr);
		}
	  	close(mt);

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
		while (((mt  = open(magtape,O_RDONLY)) < 0)  && 
			(retries < MAX_RETRY)){
			retries++;
			sleep(1);
		}
		close(mt);
		/*
		 * The load did not succeed.  The input deck of new tapes
		 * may be empty.  Give the user a chance to enter the tape.
		 */
		if (retries >= MAX_RETRY) {
			fprintf(stderr, "Loader was unable to load the next tape\n");
			fflush(stderr);
			load_failed = 1;
		}
		else {
			tape_ready = 1;
		}
	}
	/*
	 * If the unit does not have a loader, or the loader failed then
	 * have the user manually mount a tape and continue.
	 */
	if ((loader_present == 0) || (load_failed)) {
             fprintf(stderr, "Mount tape volume %d then type return ", newvol);
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

	if ((mt = open(magtape,O_RDONLY)) < 0) {
		return(NOLOADER);
	}
	/*
	 * See if the unit has a loader.
	 *
	 * When loader ioctls are added use them.  In the meantime, base
	 * loader decision on device name.
 	 */
        if (ioctl(mt, DEVIOCGET, (char *)&devget) < 0) {
		/*
		 * DEVIOCGET failed.  Don't assume that the unit has a
		 * loader then.
		 */
		close(mt);
		return(NOLOADER);
        }
	else {
		close(mt);
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
