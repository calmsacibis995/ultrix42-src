
# ifndef lint
static char *sccsid = "@(#)main.c	4.1	(ULTRIX)	7/2/90";
# endif not lint

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
 * Modification History: /usr/src/etc/restore/main.c
 *
 * 29 Jun 88 -- Sam
 *      Changed messages associated with O_NDELAY open failures to be more
 *      meaningful.
 *
 * 19 Jan 88 -- map
 *	Remove definition of signal() as it is defined in signal.h
 *	Change signal handler(s) to type void.
 *
 *  8 Sep 87 -- fries
 *	Added oflag to maintain compatability with non V2.0 or later "rmt".
 *
 * 17 Sep 86 -- fries
 *	Modified local code to properly open non-special dump image files.
 *
 * 11 Sep 86 -- fries
 *	Modified code to properly open non-special dump image files.
 *
 *  8 Sep 86 -- fries
 *	Added code to support single device open for status check and usage
 *
 *  1 Jul 86 -- fries
 *	Replaced ioctl call to get disk partition info. DIOCGETPT with
 *	DIOCDGTPT to get default partition info.
 *
 * 12 May 86 -- fries
 *	Added code to make use of device generic ioctl information.	
 *
 * 10 Apr 86 -- rr
 *	Fixed "devblocks = atol((char *) *argv++)" line
 *
 * 29 Jan 86 -- fries
 *	Renamed input default tape device to DEFTAPE_RH as driver
 *	naming convention has changed.
 *
 * 03 Dec 84 -- reilly
 *	Fix a problem with the restart switch "R".  This fix came from the
 *	net.
 *
 * 23 Apr 84 -- jmcg
 *	To aid in dumping to fixed-size devices, like floppy or cartridge
 *	disks, added a -B flag that specifies the size of the device in
 *	blocks.
 *
 * 23 Apr 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *
 *		main.c	3.12	(Berkeley)	83/08/11
 *
 * Copyright (c) 1983 Regents of the University of California
 *
 *
 *	Modified to recursively extract all files within a subtree
 *	(supressed by the h option) and recreate the heirarchical
 *	structure of that subtree and move extracted files to their
 *	proper homes (supressed by the m option).
 *	Includes the s (skip files) option for use with multiple
 *	dumps on a single tape.
 *	8/29/80		by Mike Litzkow
 *
 *	Modified to work on the new file system and to recover from
 *	tape read errors.
 *	1/19/82		by Kirk McKusick
 *
 *	Full incremental restore running entirely in user code and
 *	interactive tape browser.
 *	1/19/83		by Kirk McKusick
 *
 * ------------------------------------------------------------------------
 */

#include "restore.h"
#include <signal.h>

int 	device_open = 0; /* flag to indicate input device already open */
int	mt = -1;
int	pipein = 0,i;
char	*magtape;
int	cvtflag = 0, dflag = 0, oflag = 0, vflag = 0, yflag = 0, eomflag = 0;
int	hflag = 1, mflag = 1;
int	tfd;			/* test file desc.   */
int 	devtyp = TAP;		/* input device type */
char	command = '\0';
long	dumpnum = 1;
long	volno = 0;
char	*dumpmap;
char	*clrimap;
ino_t	maxino;
time_t	dumptime;
time_t	dumpdate;
FILE 	*terminal;
long	devblocks = 0;	 /* device size in blocks */
struct	stat	statbf;  /* stat struct           */
struct	stat	*statbfp;/* stat struct pointer   */
struct	pt	part;	 /* partition information */
struct	pt	*partp;	 /* partition info. ptr.  */
struct	devget	mt_info; /* generic status struct */
struct	devget	*devgetp;/* devget struct pointer */
char	tname[10];	 /* temp. buffer	  */
char	*cp;		 /* temp. character ptr.  */

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;
	ino_t ino;
	char *inputdev = DEFTAPE_RH;
	char *symtbl = "./restoresymtable";
	char name[MAXPATHLEN];
	extern void onintr();

	if (signal(SIGINT, onintr) == SIG_IGN)
		(void) signal(SIGINT, SIG_IGN);
	if (signal(SIGTERM, onintr) == SIG_IGN)
		(void) signal(SIGTERM, SIG_IGN);
	setlinebuf(stderr);
	if (argc < 2) {
usage:
		fprintf(stderr, "Usage:\n%s%s%s%s%s",
			"\trestore tfBhsvy [file file ...]\n",
			"\trestore xfBhmsvy [file file ...]\n",
			"\trestore ifBhmsvy\n",
			"\trestore rfBsvy\n",
			"\trestore RfBsvy\n");
		done(1);
	}
	argv++;
	argc -= 2;
	command = '\0';
	for (cp = *argv++; *cp; cp++) {
		switch (*cp) {
		case '-':
			break;
		case 'c':
			cvtflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'h':
			hflag = 0;
			break;
		case 'm':
			mflag = 0;
			break;
		case 'o':
			oflag++;
			devtyp = TAP;	/* default to tape */
			break;
		case 'v':
			vflag++;
			break;
		case 'y':
			yflag++;
			break;
		case 'f':
			if (argc < 1) {
				fprintf(stderr, "missing device specifier\n");
				done(1);
			}
			inputdev = *argv++;
			argc--;
			break;

		case 'B':
			if (argc < 1) {
				fprintf(stderr, "missing device size\n");
				done(1);
			}
			devblocks = atol((char *) *argv++);
			if ( devblocks <= 0)
				{
				fprintf( stderr, "device size must be positive integer\n");
				done(1);
				}

			argc--;
			break;

		case 's':
			/*
			 * dumpnum (skip to) for multifile dump tapes
			 */
			if (argc < 1) {
				fprintf(stderr, "missing dump number\n");
				done(1);
			}
			dumpnum = atoi(*argv++);
			if (dumpnum <= 0) {
				fprintf(stderr, "Dump number must be a positive integer\n");
				done(1);
			}
			argc--;
			break;
		case 't':
		case 'R':
		case 'r':
		case 'x':
		case 'i':
			if (command != '\0') {
				fprintf(stderr,
					"%c and %c are mutually exclusive\n",
					*cp, command);
				goto usage;
			}
			command = *cp;
			break;
		default:
			fprintf(stderr, "Bad key character %c\n", *cp);
			goto usage;
		}
	}
	if (command == '\0') {
		fprintf(stderr, "must specify i, t, r, R, or x\n");
		goto usage;
	}
	setinput(inputdev);
	if (argc == 0) {
		argc = 1;
		*--argv = ".";
	}

/*******************************************************************/
/* IF REMOTE GO ACROSS THE NETWORK TO OBTAIN:			   */
/*   device type - file, tape or disk				   */
/*   device size(if disk) 				 	   */
/*******************************************************************/
#ifdef RRESTORE
	   if(pipein){
	     fprintf(stderr, "Can not use `-' for rrestore input device\n");
	     done(1);
	   }

	  /* If in compatability mode, perform goto */
	  if(oflag)
   		goto ignore;
	
	  /* Perform 'stat' on input file on other end of Network */
	  if ((statbfp = rmtstat(magtape)) != NULL){
		if((( statbfp->st_mode & S_IFMT) == S_IFBLK)
		  ||((statbfp->st_mode & S_IFMT) == S_IFCHR)){

			   cp = (char *)magtape;

			   /* If a Block Special, then modify name to */
			   /* a Character Special file		   */
			   if ((statbfp->st_mode & S_IFMT) == S_IFBLK){
	   		       strncpy(tname,magtape,5);
	       		       tname[5] = 'r';
		   	       strcat(tname,cp+5);
			   }
			   else strcpy(tname,magtape);
	
			   /* Open device to get device size */
			/* open device to determine it's type */
			if((tfd = rmtstatchk(tname,O_RDONLY)) < 0){
			   if( tfd == -1){
			     if(errno == ENXIO){
				msg("No such device %s or address\n",tname);
				done(1);
			     }
			     else{
			        msg("Can not open device %s\n",tname);
				done(1);
			     }
			   }
			   if( tfd == -2){
			   msg("Can not open O_NDELAY remote device %s\n",tname);
			   msg("Using user supplied device information\n");
			   goto ignore;
			   }
			   if (tfd == -3)done(1);
		   	}

			/* If Tape Drive... */
			if (devgetp->category == DEV_TAPE)
		 	   devtyp = TAP;  /* device type = tape */
			else{

			   /* If Disk Device... */
			   devtyp = DSK;  /* device type = disk */

			   /* Specify disk partition "c" */
			   name[strlen(tname)-1] = 'c';

			   /* Close input device */
			   rmtclose();

			   /* Modify Device open flag */
			   device_open = 0;

			   if((rmtopen(tname,O_RDONLY)) < 0){
			      msg("Unable to open Remote device %s\n",tname);
			      msg("Using user supplied device information\n");
			      goto ignore;
		   	   }

			   /* Get partition information */
			   if ((partp = rmtgetpart()) == NULL){
			      msg("Can not get size of Remote disk device\n");
			      msg("Using user supplied device information\n");
			      goto ignore;
			   }

			   /* Calculate partition index */
			   i  = magtape[strlen(magtape)-1] - 'a';

			   /* Set # of 1024 bytes block for device */
			   if(!devblocks)
			     devblocks = partp->pt_part[i].pi_nblocks/2;

			   /* Close input device */
			   rmtclose();
			}
		}
		else{
			devtyp = FIL;
			if(rmtstatchk(magtape, O_RDONLY) < 0){
			  perror("rrestore");
			  done(1);
			}
		}
	  }
	  else{
	       perror("rrestore");
	       done(1);
	  }
#else
/*******************************************************************/
/* ELSE IF LOCAL then get the local information:		   */
/*   device type - pipe, file, tape or disk			   */
/*   device size(if disk) 				 	   */
/*******************************************************************/
	if (pipein)
	   devtyp = PIP;
	else if (stat(magtape,&statbf) >= 0){
		if((( statbf.st_mode & S_IFMT) == S_IFBLK)
		  ||((statbf.st_mode & S_IFMT) == S_IFCHR)){

			cp = (char *)magtape;

			/* If a Block Special, then modify name to */
			/* a Character Special file		   */
			if ((statbf.st_mode & S_IFMT) == S_IFBLK){
	   		    strncpy(tname,magtape,5);
	       		    tname[5] = 'r';
		   	    strcat(tname,cp+5);
			}
			else strcpy(tname,magtape);
	
			 /* Open device to get device size */
			if((mt = statchk(tname,O_RDONLY)) < 0){
			   if( mt == -1){
			     if(errno == ENXIO){
				msg("No such device %s or address\n",tname);
				done(1);
			     }
			     else{
			        msg("Can not open device %s\n",tname);
				done(1);
			     }
			   }
			   if( mt == -2){
			   msg("Can not open O_NDELAY device %s\n",tname);
			   msg("Using user supplied device information\n");
			   goto ignore;
			   }
			   if (mt == -3)done(1);
		   	}

			/* If Tape Drive... */
			if (mt_info.category == DEV_TAPE)
		 	   devtyp = TAP;   /* device type = tape */
			else{
			   devtyp = DSK;   /* device type = disk */

			   /* temporarily close device */
			   close(mt);

			   device_open = 0;

			   cp = (char *)magtape;

			   /* get the disk partition sizes */
			   tname[strlen(tname)-1] = 'c';
	
			   /* Open device to get device size */
			   if((mt = open(tname,O_RDONLY)) < 0){
			      msg("Unable to open device %s\n",tname);
			      msg("Using user supplied device information\n");
			      goto ignore;
		   	   }

			   /* Get partition information */
			   if (ioctl(mt,DIOCDGTPT,&part) < 0){
			      msg("Can not get output disk sizing information");
			      msg("Using user supplied disk device size\n");
			      goto ignore;
			   }

			   /* Close Output Device */
			   close(mt);

			   /* Calculate partition index */
			   i  = magtape[strlen(magtape)-1] - 'a';

			   /* Set # of 1024 bytes block for device */
			   if(!devblocks)
				devblocks = part.pt_part[i].pi_nblocks/2;
			   }
			}else{
				 devtyp = FIL;
				 mt = open(magtape,O_RDONLY);
				 if(mt < 0){
				   perror(magtape);
				   done(1);
				 }
			}
		}
		else{
		     perror(magtape);
		     done(1);
		}
#endif
ignore:

	/* What is to be done in restore */
	switch (command) {

	/*
	 * Interactive mode.
	 */
	case 'i':
		setup();
		extractdirs(1);
		initsymtable((char *)0);
		runcmdshell();
		done(0);
	/*
	 * Incremental restoration of a file system.
	 */
	case 'r':
		setup();
		if (dumptime > 0) {
			/*
			 * This is an incremental dump tape.
			 */
			vprintf(stdout, "Begin incremental restore\n");
			initsymtable(symtbl);
			extractdirs(1);
			removeoldleaves();
			vprintf(stdout, "Calculate node updates.\n");
			treescan(".", ROOTINO, nodeupdates);
			findunreflinks();
			removeoldnodes();
		} else {
			/*
			 * This is a level zero dump tape.
			 */
			vprintf(stdout, "Begin level 0 restore\n");
			initsymtable((char *)0);
			extractdirs(1);
			vprintf(stdout, "Calculate extraction list.\n");
			treescan(".", ROOTINO, nodeupdates);
		}
		createleaves(symtbl);
		createlinks();
		setdirmodes();
		checkrestore();
		if (dflag) {
			vprintf(stdout, "Verify the directory structure\n");
			treescan(".", ROOTINO, verifyfile);
		}
		dumpsymtable(symtbl, (long)1);
		done(0);
	/*
	 * Resume an incremental file system restoration.
	 */
	case 'R':
		setup1();
		initsymtable(symtbl);
		skipmaps();
		skipdirs();
		createleaves(symtbl);
		createlinks();
		setdirmodes();
		checkrestore();
		dumpsymtable(symtbl, (long)1);
		done(0);
	/*
	 * List contents of tape.
	 */
	case 't':
		setup();
		extractdirs(0);
		while (argc--) {
			canon(*argv++, name);
			ino = dirlookup(name);
			if (ino == 0)
				continue;
			treescan(name, ino, listfile);
		}
		done(0);
	/*
	 * Batch extraction of tape contents.
	 */
	case 'x':
		setup();
		extractdirs(1);
		initsymtable((char *)0);
		while (argc--) {
			canon(*argv++, name);
			ino = dirlookup(name);
			if (ino == 0)
				continue;
			if (mflag)
				pathcheck(name);
			treescan(name, ino, addfile);
		}
		createfiles();
		createlinks();
		setdirmodes();
		if (dflag)
			checkrestore();
		done(0);
	}
}

/*
 * Read and execute commands from the terminal.
 */
runcmdshell()
{
	register struct entry *np;
	ino_t ino;
	char curdir[MAXPATHLEN];
	char name[MAXPATHLEN];
	char cmd[BUFSIZ];

	canon("/", curdir);
loop:
	getcmd(curdir, cmd, name);
	switch (cmd[0]) {
	/*
	 * Add elements to the extraction list.
	 */
	case 'a':
		ino = dirlookup(name);
		if (ino == 0)
			break;
		if (mflag)
			pathcheck(name);
		treescan(name, ino, addfile);
		break;
	/*
	 * Change working directory.
	 */
	case 'c':
		ino = dirlookup(name);
		if (ino == 0)
			break;
		if (inodetype(ino) == LEAF) {
			fprintf(stderr, "%s: not a directory\n", name);
			break;
		}
		(void) strcpy(curdir, name);
		break;
	/*
	 * Delete elements from the extraction list.
	 */
	case 'd':
		np = lookupname(name);
		if (np == NIL || (np->e_flags & NEW) == 0) {
			fprintf(stderr, "%s: not on extraction list\n", name);
			break;
		}
		treescan(name, np->e_ino, deletefile);
		break;
	/*
	 * Extract the requested list.
	 */
	case 'e':
		createfiles();
		createlinks();
		setdirmodes();
		if (dflag)
			checkrestore();
		volno = 0;
		break;
	/*
	 * List available commands.
	 */
	case 'h':
	case '?':
		fprintf(stderr, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
			"Available commands are:\n",
			"\tls [arg] - list directory\n",
			"\tcd arg - change directory\n",
			"\tpwd - print current directory\n",
			"\tadd [arg] - add `arg' to list of",
			" files to be extracted\n",
			"\tdelete [arg] - delete `arg' from",
			" list of files to be extracted\n",
			"\textract - extract requested files\n",
			"\tquit - immediately exit program\n",
			"\tverbose - toggle verbose flag",
			" (useful with ``ls'')\n",
			"\thelp or `?' - print this list\n",
			"If no `arg' is supplied, the current",
			" directory is used\n");
		break;
	/*
	 * List a directory.
	 */
	case 'l':
		ino = dirlookup(name);
		if (ino == 0)
			break;
		printlist(name, ino);
		break;
	/*
	 * Print current directory.
	 */
	case 'p':
		if (curdir[1] == '\0')
			fprintf(stderr, "/\n");
		else
			fprintf(stderr, "%s\n", &curdir[1]);
		break;
	/*
	 * Quit.
	 */
	case 'q':
	case 'x':
		return;
	/*
	 * Toggle verbose mode.
	 */
	case 'v':
		if (vflag) {
			fprintf(stderr, "verbose mode off\n");
			vflag = 0;
			break;
		}
		fprintf(stderr, "verbose mode on\n");
		vflag++;
		break;
	/*
	 * Turn on debugging.
	 */
	case 'D':
		if (dflag) {
			fprintf(stderr, "debugging mode off\n");
			dflag = 0;
			break;
		}
		fprintf(stderr, "debugging mode on\n");
		dflag++;
		break;
	/*
	 * Unknown command.
	 */
	default:
		fprintf(stderr, "%s: unknown command; type ? for help\n", cmd);
		break;
	}
	goto loop;
}
