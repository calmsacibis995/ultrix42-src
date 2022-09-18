# ifndef lint
static char *sccsid = "@(#)dumpmain.c	4.2    (ULTRIX)        2/14/91";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-1989 by			*
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
 * Modification History: /usr/src/etc/dump/dumpmain.c
 *
 * 06 Feb 91 -- lambert
 *	Add support for TA91.
 *
 * 29 Jan 90 -- lambert
 *	Add support for TLZ04 (RDAT), TF70(L)
 *
 * 13 Sep 89 -- Tim Burke
 *	Added 3 new TA90 densities.
 *
 * 11 Aug 89 -- lambert
 *	Corrected tape "length" of TA90 cartridge
 *
 *  1 May 89 -- lambert
 *	Added logic to tape length calcs for TK70, TA90
 *
 * 12 Dec 88 -- lambert
 *	Removed calls to "emsg" that used floating point numbers.
 *
 *  8 Sep 87 -- fries
 *	Added oflag for rdump V2.0 to pre-V2.0 rmt and Berkeley rmt
 *	compatability.
 *
 *  6 Jan 87 -- fries
 *	Added newline to "Need keyletter ``f''..." rdump message.
 *
 * 11 Dec 86 -- fries
 *	Added code to get disk sizing information from either /dev/rra#a
 *	or /dev/rra#c.
 *
 *  1 Dec 86 -- fries
 *	Added code to set TK50's tape length.
 *	Added code to set TK70's tape length.
 *
 * 31 Oct 86 -- fries
 *	Added code to support tape devices with density of 10240 BPI.
 *
 * 16 Oct 86 -- fries
 *	Added code to support open & close of non-rewind device
 *	this allows device characteristics to be acquired without
 *	mis-positioning the /dev/rmt0h device.
 *
 * 18 Sep 86 -- fries
 *	Still more code modifications to open normal files
 *	(non spcial) correctly.
 *
 * 11 Sep 86 -- fries
 *	Modified code to open normal files (non spcial) correctly.
 *
 *  9 Sep 86 -- fries
 *	Modified code to only open tape devices once for both status and
 *	usage.
 *
 * 28 Jul 86 -- fries
 *	Added code to support the -S option. This option returns sizing
 *	information pertaining to command line supplied file system.
 *	Ex. "/etc/dump 0S /" gives the size of the root file system.
 *	Output from this option is sent to the stderr output.
 *
 *  2 Jul 86 -- fries
 *	Modified to allow '-' before options.
 *
 *  1 Jul 86 -- fries
 *	Changed get partition system call DIOCGETPT to get default
 *	partition information.(DIOCDGTPT)
 *
 *  9 May 86 -- fries
 *	Added code to get device information from device generic ioctl
 *	returned data.
 *
 * 13 Feb 86 -- fries
 *	Changed messages to indicate tape or disk according to what
 *	the output device is.
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
 *		dumpmain.c	1.15 (Berkeley) 8/19/83
 *
 * ------------------------------------------------------------------------
 */

#include "dump.h"

extern void rewind_tape();

int	device_open = 0;/* flag to indicate that device is already open*/
int	size_only = 0;	/* just report size of space required for dump */
			/* then exit                                   */
int	eom_flag = 0;	/* end of media flag    */
int	notify = 0;	/* notify operator flag */
int	usecreate = 0;	/* flag - create to be used instead of open */
int	blockswritten = 0;	/* number of blocks written on current tape */
int	tapeno = 0;	/* current tape number */
int	density = 0;	/* density in bytes/0.1" */
int	ntrec = NTREC;	/* # tape blocks in each tape record */
int	cartridge = 0;	/* Assume non-cartridge tape */
int	devblocks = 0;	/* device size in blocks */
int	devtyp = FIL;   /* device type of output device */
int	uvaxrx50 = 0;	/* uVAXrx50: dump is on uvax to rx50's */
struct	stat statbf;	/* struct returning file header info.  */
struct	pt	part;	/* partition information structure     */
char	tname[30];	/* temp name buffer to make raw device */
char 	*cp;		/* temp character pointer              */

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char		*arg;
	int		i,tfd;
	float		fetapes;
	register	struct	fstab	*dt;

	time(&(spcl.c_date));

	tsize = 0;	/* Default later, based on 'c' option for cart tapes */
	tape = TAPE;	/* Default tape device */
	disk = DISK;	/* Default disk device */
	increm = NINCREM;
	temp = TEMP;	/* pointer to temp file */
	if (TP_BSIZE / DEV_BSIZE == 0 || TP_BSIZE % DEV_BSIZE != 0) {
		msg("TP_BSIZE must be a multiple of DEV_BSIZE\n");
		dumpabort();
	}
	incno = '9';	/* Default dump level */
	uflag = 0;	/* update `/etc/dumpdates' file flag */
	oflag = 0;	/* rdump compatability mode flag */
	arg = "u";	/* Set default key to update file */
	if(argc > 1) {
		argv++;
		argc--;
		arg = *argv;
		if (*arg == '-')
			arg++;
	}
	while(*arg)
	switch (*arg++) {
	case 'w':
		lastdump('w');		/* tell us only what has to be done */
		exit(0);
		break;
	case 'W':			/* what to do */
		lastdump('W');		/* tell us the current state of what has been done */
		exit(0);		/* do nothing else */
		break;

	case 'f':			/* output file */
		if(argc > 1) {
			argv++;
			argc--;
			tape = *argv;
		}
		break;

	case 'd':			/* density, in bits per inch */
		if (argc > 1) {
			argv++;
			argc--;
			density = atoi(*argv) / 10;
		}
		break;

	case 'B':			/* disk device size in blocks */
		if(argc > 1) {
			argv++;
			argc--;
			devblocks = atol(*argv);
			uvaxrx50 = 1;	/* uVAXrx50 */
		}
		break;

	case 's':			/* tape size, feet */
		if(argc > 1) {
			argv++;
			argc--;
			user_tsize++;
			tsize = atol(*argv);
			tsize *= 12L*10L;
		}
		break;

#ifndef RDUMP
	case 'b':			/* blocks per tape write */
		if(argc > 1) {
			argv++;
			argc--;
			ntrec = atol(*argv);
		}
		break;

#else

	case 'o':			/* compatability mode */
					/* for rdump          */
		oflag++;
		devtyp = TAP;		/* default to tape    */
		break;

#endif RDUMP

	case 'c':			/* Tape is cart. not 9-track */
		cartridge++;
		break;

	case '0':			/* dump level */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		incno = arg[-1];
		break;

	case 'u':			/* update /etc/dumpdates */
		uflag++;
		break;

	case 'S':			/* report size */
		size_only++;
		break;

	case 'n':			/* notify operators */
		notify++;
		break;

	default:
		fprintf(stderr, "bad key '%c%'\n", arg[-1]);
		Exit(X_ABORT);
	}
	/* Set INPUT DISK device NAME */
	if(argc > 1) {
		argv++;
		argc--;
		disk = *argv;
	}

	/* Check that user is Superuser */
	if(geteuid()){
		fprintf(stderr,"Must be Superuser to run dump\n");
		Exit(X_ABORT);
	}

	/* If output device = `-', then output to standard output */
	if (strcmp(tape, "-") == 0) {
#ifdef RDUMP
	msg("Can not pipe to a Remote System\n");
	Exit(X_ABORT);
#endif
		tape = "standard output";
		devtyp = PIP;
	}

/*******************************************************************/
/* IF REMOTE GO ACROSS THE NETWORK TO OBTAIN:			   */
/*   device type - file, tape or disk				   */
/*   device size(if disk) 				 	   */
/*******************************************************************/
#ifdef RDUMP
{
	  char *index();

	  /* Set host name and tape name strings */
	  host = tape;
	  tape = index(host, ':');

	  /* If no tape name... */
	  if (tape == 0) {
		msg("Need keyletter ``f'' and device ``host:(tape, disk or file name)''\n");
		exit(1);
	  }

	  *tape++ = 0; /* delimit host name string */

	  /* Connect to Remote System */
	  if (rmthost(host) == 0)
		exit(X_ABORT);

	  /* If old "rmt" code, then bypass following code */
	  if(oflag)
		goto ignore;

	  /* Perform 'stat' on output file on other end of Network */
	  if ((statbfp = rmtstat(tape)) !=  NULL){

		if (((statbfp->st_mode & S_IFMT) == S_IFBLK)
		    ||((statbfp->st_mode & S_IFMT) == S_IFCHR)){

			/* Preset device type to tape if generic   */
			/* ioctl fails...			   */
			devtyp = TAP;

			/* If a Block Special, then modify name to */
			/* a Character Special file		   */
			if ((statbfp->st_mode & S_IFMT) == S_IFBLK){
			   cp = (char *)tape;
	   	           strncpy(tname,tape,5);
	       	           tname[5] = 'r';
			   strcat(tname,cp+5);
			}
			else strcpy(tname,tape);	
	
			/* Check for /dev/rmt */
			if (strncmp("/dev/rmt", tape, 8) == 0)
			   if((tape[9] == 'h')|| (tape[9] == 'l'))
			     if( isdigit(tape[8])){
				strcpy(tname, "/dev/nrmt");
				tname[9] = tape[8];
				tname[10] = tape[9];
			     }
	
			/* open device to determine it's type */
			if((tfd = rmtstatchk(tname,O_RDWR)) < 0){
			   if (tfd == -3)dumpabort();
			   if(tfd == -2){
			      msg("Can not perform generic ioctl to device %s\n",tape);
			      msg("Proceeding with user supplied parameters\n");
			      goto ignore;
			    }
			   if (errno == ENXIO){
			      msg("No such device or address\n");
			      dumpabort();
			   }
		   	}

			/* If Tape Drive... */
			if (devgetp->category == DEV_TAPE){

			   /* Get tape density information */
			   if(!density){
			     if(devgetp->category_stat & DEV_800BPI)
			       density = 80;
			     if(devgetp->category_stat & DEV_1600BPI)
			       density = 160;
			     if(devgetp->category_stat & DEV_6250BPI)
			       density = 625;
			     if(devgetp->category_stat & DEV_6666BPI)
			       density = 667;
			     if(devgetp->category_stat & DEV_10240BPI)
			       density = 1024;
			     if(devgetp->category_stat & DEV_38000BPI)
			       density = 3800;
			     if(devgetp->category_stat & DEV_38000_CP)
			       density = 3800;
			     if(devgetp->category_stat & DEV_61000_BPI)
			       density = 6100;
			     if(devgetp->category_stat & DEV_76000BPI)
			       density = 7600;
			     if(devgetp->category_stat & DEV_76000_CP)
			       density = 7600;
			   }
			 /* If TK50 set tape length */
			 if ((strcmp(devgetp->device, DEV_TK50) == 0) &&
			 	(!tsize)) tsize =  1200L*120L;

			 /* If TK70 set tape length */
			 if (((strcmp(devgetp->device, DEV_TK70) == 0) ||
			      (strcmp(devgetp->device, DEV_TF70) == 0) ||
			      (strcmp(devgetp->device, DEV_TF70L) == 0)) &&
			 	(!tsize)) tsize =  2409L*120L;
 
			 /* If TA90 set tape length */
			 if ((strcmp(devgetp->device, DEV_TA90) == 0) ||
			     (strcmp(devgetp->device, DEV_TA91) == 0) &&
			 	(!tsize)) tsize =  1200L*120L;
 
			 /* If TLZ04 set tape length */
			 if ((strcmp(devgetp->device, DEV_TLZ04) == 0) &&
			 	(!tsize)) tsize =  1818L*120L;
 
			}
			else {
				uvaxrx50 = 1;

				devtyp = DSK;

				if(!devblocks){

				/* Open "a" partition device */
			        tname[strlen(tname)-1] = 'a';

				/* Temporarily Close device */
				rmtclose();

				/* Open device to get device size */
				if((rmtopen(tname,O_RDONLY)) < 0){

				      msg("Unable to open Remote device %s\n",tname);
				      dumpabort();
		   		   }

				/* Get partition information */
				if ((partp = rmtgetpart()) == NULL){

				   /* Close the /dev/rra#a device */
				   rmtclose();

				   /* and Open the "c" device instead */
			           tname[strlen(tname)-1] = 'c';

				   /* If open still fails... */
				   if((rmtopen(tname,O_RDONLY)) < 0){
				      msg("Unable to open Remote device %s\n",tname);
				      dumpabort();
		   		   }

				if ((partp = rmtgetpart()) == NULL){
				      msg("Can not get size of Remote device\n");
				      msg("Use the -B option to specify size\n");
				      dumpabort();
				   }
				}

				/* Close output device */
				rmtclose();

				device_open = 0;

				/* Calculate partition index */
				i  = tape[strlen(tape)-1] - 'a';

				/* Set # of 1024 bytes block for device */
				devblocks = partp->pt_part[i].pi_nblocks/2;
				}/* End of if(!devblocks) */
				}
		     }else usecreate = 1;
	}else usecreate = 1;
ignore:
	if(!devblocks){
           rmtclose();
	   device_open = 0;
	}
}
#else
/*******************************************************************/
/* ELSE IF LOCAL then get the local information:		   */
/*   device type - pipe, file, tape or disk			   */
/*   device size(if disk) 				 	   */
/*******************************************************************/
	if (devtyp == PIP);
	else if (stat(tape,&statbf) < 0){
		usecreate = 1;
		}
		else{

		/* If Block or Character Special File */
		if(((statbf.st_mode & S_IFMT) == S_IFBLK)
		   ||((statbf.st_mode & S_IFMT) == S_IFCHR)){

			/* Preset device type to tape if generic   */
			/* ioctl fails...			   */
			devtyp = TAP;

			/* If a Block Special, then modify name to */
			/* a Character Special file		   */
			if ((statbf.st_mode & S_IFMT) == S_IFBLK){
			   cp = tape;
   		           strncpy(tname,tape,5);
       		           tname[5] = 'r';
	   		   strcat(tname,cp+5);
			}
			else strcpy(tname,tape);
	
			/* Check for /dev/rmt */
			if (strncmp("/dev/rmt", tape, 8) == 0)
			   if((tape[9] == 'h')|| (tape[9] == 'l'))
			     if( isdigit(tape[8])){
				strcpy(tname, "/dev/nrmt");
				tname[9] = tape[8];
				tname[10] = tape[9];
			     }
	
			/* open device to determine it's type */
			if((tfd = statchk(tname,O_RDWR)) < 0){
			   if (tfd == -3)dumpabort();
			   if(tfd == -2){
			      msg("Can not perform generic ioctl to device %s\n",tname);
			      msg("Proceeding with user supplied parameters\n");
			      goto ignore;
			   }
			   if (errno == ENXIO){
			      msg("No such device or address\n");
			      dumpabort();
			   }
		        }

			/* If Tape Drive... */
			if (mt_info.category == DEV_TAPE){

			   /* Get tape density information */
			   /* If user did not specify it   */
			   if(!density){
			     if(mt_info.category_stat & DEV_800BPI)
			       density = 80;
			     if(mt_info.category_stat & DEV_1600BPI)
			       density = 160;
			     if(mt_info.category_stat & DEV_6250BPI)
			       density = 625;
			     if(mt_info.category_stat & DEV_6666BPI)
			       density = 667;
			     if(mt_info.category_stat & DEV_10240BPI)
			       density = 1024;
			     if(mt_info.category_stat & DEV_38000BPI)
			       density = 3800;
			     if(mt_info.category_stat & DEV_38000_CP)
			       density = 3800;
			     if(mt_info.category_stat & DEV_61000_BPI)
			       density = 6100;
			     if(mt_info.category_stat & DEV_76000BPI)
			       density = 7600;
			     if(mt_info.category_stat & DEV_76000_CP)
			       density = 7600;
			   }
			 /* If TK50 set tape length */
			 if ((strcmp(mt_info.device, DEV_TK50) == 0) &&
			 	(!tsize)) tsize =  1200L*120L;

			 /* If TK70 set tape length */
			  if (((strcmp(mt_info.device, DEV_TK70) == 0) ||
			       (strcmp(mt_info.device, DEV_TF70) == 0) ||
			       (strcmp(mt_info.device, DEV_TF70L) == 0)) &&
			 	(!tsize)) tsize =  2409L*120L;
 
			 /* If TA90 set tape length */
			 if ((strcmp(mt_info.device, DEV_TA90) == 0) ||
			     (strcmp(mt_info.device, DEV_TA91) == 0) &&
			 	(!tsize)) tsize =  1200L*120L;

			 /* If TLZ04 set tape length */
			 if ((strcmp(mt_info.device, DEV_TLZ04) == 0) &&
			 	(!tsize)) tsize =  1818L*120L;
 
			}
			else {
				uvaxrx50 = 1;

				devtyp = DSK;

				if(!devblocks){

				close(tfd);

				/* Temporarily open the 'a' */
				/* device to get part. info.*/
				tname[strlen(tname)-1] = 'a';
	
				/* Open device to get device size */
				if((tfd = open(tname,O_NDELAY)) < 0){

				   /* If the 'a' open failed  */
				   /* then try the 'c' instead*/
				   tname[strlen(tname)-1] = 'c';

				   if((tfd = open(tname,O_NDELAY)) < 0){
				      msg("Unable to open device %s\n",tname);
				      msg("to perform disk device sizing\n");
				      msg("Use the -B option for sizing\n");
				      exit(1);
		   		   }
				}

				/* Get partition information */
				if (ioctl(tfd,DIOCDGTPT,(char *)&part) < 0){
				   msg("Can not get size of device %s\n",tname);
				   msg("Use the -B option for sizing\n");
				   exit(1);
				}

				/* Close Output Device */
				close(tfd);

				device_open = 0;

				/* Calculate partition index */
				i  = tape[strlen(tape)-1] - 'a';

				/* Set # of 1024 bytes block for device */
				devblocks = part.pt_part[i].pi_nblocks/2;
				} /* End of if(!devblocks) */
				}
			}else usecreate = 1;
		     }
ignore:
	if(!devblocks){
	  close(tfd);
	  device_open = 0;
	}
#endif

	/* If device size set, then assume disk device */
	if(devblocks)
	  devtyp = DSK;

	/*
	 * Determine how to default tape size and density
	 * (Note: density & bytes = amts. in .1" of tape)
	 *
	 *         	density				tape size
	 * 9-track	1600 bpi (160 bytes/.1")	2300 ft.(default)
	 * 9-track	6250 bpi (625 bytes/.1")	2300 ft.
	 * cartridge	8000 bpi (100 bytes/.1")	4000 ft. (450*9 - slop)
	 */
	if (density == 0)
		density = cartridge ? 100 : 160;

	/* Calculate length of tape */
	if (tsize == 0)
		tsize = cartridge ? 4000L*120L : 2300L*120L;

#ifdef TS_DEBUG
	fprintf(stderr, "tsize = %d\n", tsize);
	fflush(stderr);
#endif

	/************************************************/
	/*         Set up Signal Handlers		*/
	/************************************************/
	if (signal(SIGHUP, sighup) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTRAP, sigtrap) == SIG_IGN)
		signal(SIGTRAP, SIG_IGN);
	if (signal(SIGFPE, sigfpe) == SIG_IGN)
		signal(SIGFPE, SIG_IGN);
	if (signal(SIGBUS, sigbus) == SIG_IGN)
		signal(SIGBUS, SIG_IGN);
	if (signal(SIGSEGV, sigsegv) == SIG_IGN)
		signal(SIGSEGV, SIG_IGN);
	if (signal(SIGTERM, sigterm) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	/************************************************/

	set_operators();	/* /etc/group snarfed */
	getfstab();		/* /etc/fstab snarfed */
	/*
	 *	disk can be either the full special file name,
	 *	the suffix of the special file name,
	 *	the special name missing the leading '/',
	 *	the file system name with or without the leading '/'.
	 */
	dt = fstabsearch(disk);
	if (dt != 0)
		disk = rawname(dt->fs_spec);

	/* uVAXrx50: Added to minimize the # of concurrent processes */
	if (uvaxrx50) {
		mpid = getpid();
		signal(SIGTERM, SIG_IGN);
	};

	getitime();		/* /etc/dumpdates snarfed */

	msg("Date of this level %c dump: %s\n", incno, prdate(spcl.c_date));
 	msg("Date of last level %c dump: %s\n",
		lastincno, prdate(spcl.c_ddate));
	msg("Dumping %s ", disk);
	if (dt != 0)
		msgtail("(%s) ", dt->fs_file);
#ifdef RDUMP
	msgtail("to %s on host %s\n", tape, host);
#else
	msgtail("to %s\n", tape);
#endif

	fi = open(disk,O_RDONLY); /* Open input disk device */

	/* If open error... */
	if (fi < 0) {
		msg("Cannot open %s\n", disk);
		Exit(X_ABORT);
	}
	esize = 0;	/* estimated tape size in blocks */
	sblock = (struct fs *)buf; /* set pointer to superblock area */
	sync(); /* make sure all disks up to date */
	bread(SBLOCK, sblock, SBSIZE); /* read in superblock */

	/* Check that data read in is really the superblock */
	if (sblock->fs_magic != FS_MAGIC) {
		msg("Bad sblock magic number\n");
		dumpabort();
	}
	/* Calc. size of bit map areas */
	msiz = roundup(howmany(sblock->fs_ipg * sblock->fs_ncg, NBBY),
		TP_BSIZE);

	/************************************************/
	/* Allocate space for bit maps 			*/
	/* ---------------------------			*/
	/* Bit Maps handle data as follows:		*/
	/* clrmap - a bit is set for each active node	*/
	/*          in the file system.			*/
	/* dirmap - a bit is set for each inode which	*/
	/*          is a directory.			*/
	/* nodmap - a bit is set for each node whose	*/
	/*          date is later than the last dump.	*/
	/************************************************/
	clrmap = (char *)calloc(msiz, sizeof(char));
	dirmap = (char *)calloc(msiz, sizeof(char));
	nodmap = (char *)calloc(msiz, sizeof(char));

	msg("Mapping (Pass I) [regular files]\n");

	/* Set bit maps up to represent the file system */
	pass(mark, (char *)NULL);		/* mark updates esize */

	do {
		msg("Mapping (Pass II) [directories]\n");
		nadded = 0;
		pass(add, dirmap);
	} while(nadded);

	/* Adjust tape blocks estimated by # of blocks used for map */
	bmapest(clrmap);
	bmapest(nodmap);

	/* Estimate # of tape reels or cartridges */
	if( devblocks) {
		/* Estimate number of pieces of media. */
		fetapes = ( esize + devblocks - 1) / devblocks;
		}
	else if (cartridge) {
		/* Estimate number of tapes, assuming streaming stops at
		   the end of each block written, and not in mid-block.
		   Assume no erroneous blocks; this can be compensated for
		   with an artificially low tape size. */
		fetapes = 
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes/block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* streaming-stops per block */
			* 15.48		/* 0.1" / streaming-stop */
		) * (1.0 / tsize );	/* tape / 0.1" */
	} else {
		/* Estimate number of tapes, for old fashioned 9-track tape */
		int tenthsperirg = (density == 625) ? 3 : 7;
		fetapes =
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes / block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* IRG's / block */
			* tenthsperirg	/* 0.1" / IRG */
		) * (1.0 / tsize );	/* tape / 0.1" */
	}
	etapes = fetapes;		/* truncating assignment */
	etapes++;
	/* count the nodemap on each additional tape */
	for (i = 1; i < etapes; i++)
		bmapest(nodmap);
	esize += i + 10;	/* headers + 10 trailer blocks */

	/* Report estimated # of tape reels or disks */
	msgtail("\n");
	
	/* If output is to a tape */
	if(devtyp == TAP){
	emsg("Estimates based on %ld feet of tape at a density of %d BPI...\n",tsize/120L,density*10);
	fprintf(stderr, "  DUMP: This dump will occupy %ld (%d byte) blocks on %3.2f tape(s).\n", esize/ntrec,ntrec * TP_BSIZE, fetapes);
	fflush(stdout);
        fflush(stderr);
	}

	/* Else if output is to a disk */
	else if (devtyp == DSK){
	fprintf(stderr, "  DUMP: Estimated %ld (%d byte) blocks on %3.2f disk(s).\n", esize/ntrec,ntrec * TP_BSIZE, fetapes);
	fflush(stdout);
        fflush(stderr);
	}

	/* If output is to a pipe */
	else if(devtyp == PIP)
	emsg("Estimated %ld bytes output to Standard Output\n",(esize - 8) * TP_BSIZE);

	/* Otherwise output is to a file */
	else
	emsg("Estimated %ld bytes output to file named %s\n",(esize - 8) * TP_BSIZE,tape);
	msgtail("\n");

	if(size_only)
		Exit(X_FINOK);	/* NORMAL EXIT */

	alloctape();			/* Allocate tape buffer */

	otape();
	time(&(tstart_writing));	/* get dump starting time */
	bitmap(clrmap, TS_CLRI);

	msg("Dumping (Pass III) [directories]\n");
	pass(dump, dirmap);

	msg("Dumping (Pass IV) [regular files]\n");
	pass(dump, nodmap);

	spcl.c_type = TS_END;
#ifndef RDUMP
	for(i=0; i<ntrec; i++)
		spclrec();
#endif
	msgtail("\n");
	if(devtyp == TAP)
	msg("%ld tape blocks were dumped on %d tape(s)\n",spcl.c_tapea/ntrec,spcl.c_volume);
	else if(devtyp == DSK)
	msg("%ld disk blocks were dumped on %d disk(s)\n",spcl.c_tapea/ntrec,spcl.c_volume);
	else if(devtyp == PIP)
	msg("%ld bytes were dumped to Standard Output\n",(spcl.c_tapea - 8) * TP_BSIZE);
	else
	msg("%ld bytes were dumped to file %s\n",(spcl.c_tapea - 8) * TP_BSIZE,tape);
	msgtail("\n");

	putitime();			/* update `/etc/dumpdates' */
#ifndef RDUMP
	/* If not output to standard output... */
	if (devtyp != PIP) {
		rewind_tape();
	}
#else
	tflush(1);			/* flush the tape buffer */
	rewind_tape();			/* and rewind tape 	 */
#endif
	/* Tell that dump is complete */
	msg("Dump is done\n");

	broadcast("Dump is done!\7\7\n");

	/* uVAXrx50: Added to minimize the # of concurrent processes */
	if (uvaxrx50) {
		kill(lparentpid, SIGTERM);	/* kill last parent */
		kill(mpid, SIGEMT);		/* signal master process */
	};

	Exit(X_FINOK);			/* NORMAL EXIT */
}

int	sighup(){	msg("SIGHUP()  try rewriting\n"); sigAbort();} 
int	sigtrap(){	msg("SIGTRAP()  try rewriting\n"); sigAbort();}
int	sigfpe(){	msg("SIGFPE()  try rewriting\n"); sigAbort();}
int	sigbus(){	msg("SIGBUS()  try rewriting\n"); sigAbort();}
int	sigsegv(){	msg("SIGSEGV()  ABORTING!\n"); abort();}
int	sigalrm(){	msg("SIGALRM()  try rewriting\n"); sigAbort();}
int	sigterm(){	msg("SIGTERM()  try rewriting\n"); sigAbort();}

/* Abort caused by a signal */
sigAbort()
{
	if (devtyp == PIP) {
		msg("Unknown signal, cannot recover\n");
		dumpabort();
	}
	msg("Rewriting attempted as response to unknown signal.\n");
	fflush(stderr);
	fflush(stdout);
	close_rewind(0);
	exit(X_REWRITE);
}

char *rawname(cp)
	char *cp;
{
	static char rawbuf[32];
	char *rindex();
	char *dp = rindex(cp, '/');

	if (dp == 0)
		return (0);
	*dp = 0;
	strcpy(rawbuf, cp);
	*dp = '/';
	strcat(rawbuf, "/r");
	strcat(rawbuf, dp+1);
	return (rawbuf);
}
