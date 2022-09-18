
# ifndef lint
static char *sccsid = "@(#)command.c	4.2	(ULTRIX)	12/6/90";
# endif not lint

/****************************************************************
 *								*
 *			Copyright (c) 1985, 1988, 1989 by	*
 *		Digital Equipment Corporation, Maynard, MA	*
 *			All rights reserved.			*
 *								*
 *   This software is furnished under a license and may be used *
 *   and copied  only  in accordance with the terms of such	*
 *   license and with the  inclusion  of  the  above  copyright *
 *   notice. This software  or  any  other copies thereof may	*
 *   not be provided or otherwise made available to any other	*
 *   person.  No title to and ownership of the software is	*
 *   hereby transferred.					*
 *								*
 *   The information in this software is subject to change	*
 *   without  notice  and should not be construed as a		*
 *   commitment by Digital  Equipment Corporation.		*
 *								*
 *   Digital assumes  no responsibility   for  the use  or	*
 *   reliability of its software on equipment which is not	*
 *   supplied by Digital.					*
 *								*
 ****************************************************************/
/**/
/*
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *	31-Oct-89	rsp
 *			Moved needed extern declarations to this file.
 *
 *	30-May-89	bstevens
 *			Added new -R flag for lists of archive files.
 *
 *	16-May-89	rsp
 *                      Added status checking for ioctl's related to
 *                      tape positioning. Report all ioctl failures
 *                      except those related to nbuffio since some of
 *                      these 'errors' are used to branch to certain code.
 *
 *	31-Aug-88	lambert
 *			Changed include line from "tar.h" to <tar.h> for
 *			POSIX support.
 *
 *	15-Mar-88	mjk - added Posix support
 *
 *	03-Aug-87	fries
 *			-p Warns only if not uid = 0 & vflag set.
 *
 *	23-Sep-87	fries
 *			Added code in flushtape() to Clear Serious
 *			Exception should a write need to be performed
 *			while past EOT to flush the tape buffer.
 *
 *	03-Aug-87	fries
 *			-p Warns only if not uid = 0 but still
 *			sets the pflag and continues.
 *
 *	28-Jul-87	fries
 *			Modified code to allow use of -p if
 *			geteuid() effective uid is that of
 *			root(was using getuid()).
 *
 *	20-Apr-87	fries
 *			Modified how code reports bad blocksize.
 *
 *	10-Apr-87	fries
 *			Added code to  insure  if  device  is
 *			off-line, write protected...etc. that
 *			input is not from pipe or  redirected
 *			prior to requesting retry.
 *
 *	10-Feb-87	fries
 *			Put the 0 - 9 option code back in place.
 *
 *	19-Dec-86	fries
 *			Modified default device from /dev/rmt0h
 *			to /dev/rra1a when invoked as mdtar.
 *
 *	26-Nov-86	fries
 *			Modified comments to make -o option
 *			read. Will save directory information
 *			if -o specified.
 *			Extra argument to writetape().
 *
 *	16-Oct-86	fries
 *			Moved AFLAG check below statchk() calls.
 *			Extra argument to writetape().
 *
 *	16-Oct-86	lp
 *			remove MTCACHE
 *
 *	 9-Oct-86	fries
 *			Modified code to get default disk partition sizes.
 *
 *	29-Sep-86	fries
 *			Added code to get default disk partition sizes.
 *			for "mdtar". Added code to put -f device name
 *			into char string named magtape.
 *			
 *	15-Sep-86	fries
 *			Modified code after statchk to check for error.
 *			
 *	 2-Jul-86	lp
 *			N-bufferring now works with eot.
 *			
 *	19-Jun-86	lp
 *			Added hooks for n-buffered i/o. Cleanup &
 *			reformat somewhat. Remove non-U32 ifdefs.
 *
 *	IV		15-May-86 fries/
 *			Added generic ioctl support(calls of statchk)
 *
 *	III		28-Jan-86 rjg/
 *			Do not allow non-super user to set pflag
 *
 *	II		Ray Glaser, 09-Jan-86
 *			Do not set block size to 10 for disks (d flag)
 *
 *	I		19-Dec-85 rjg/
 *			Create original version.
 *	
 */
#include <tar.h>
#include <sys/types.h>
#include <sys/fs.h>

extern struct devget mt_info;
extern int revwhole;
extern int revdec;

/*.sbttl checkf() */
checkf(longname, mode, howmuch)
	char *longname;
	int	mode;
	int	howmuch;
{

	char *shortname;


/* Strip off any directory and get the base filename.
 */ 
if ((shortname = rindex(longname, '/')))
	shortname++;
else
	shortname = longname;

/* Basic skip set ?
 */
if (howmuch > 0) {
	/*
	 * Skip SCCS directories on input
	 */
	if ((mode & S_IFMT) == S_IFDIR)
		return (strcmp(shortname, "SCCS") != 0);

	/*
	 * Skip SCCS directory files on extraction
	 * Check SCCS as a toplevel directory and
	 * as a subdirectory.
	 */
	if (strfind(longname,"SCCS/") == longname || strfind(longname,"/SCCS/"))
		return (0);
	/*
	 * Skip core and errs files. 
	 */
	if (strcmp(shortname,"core")==0 || strcmp(shortname,"errs")==0)
		return (0);

}/*E if howmuch > 0 */

/* First level additions ?
 */
if (howmuch > 1) {
	int l;

	l = strlen(shortname);	/* get string len */

	/* Skip .o files
	 */
	if (shortname[--l] == 'o' && shortname[--l] == '.')
		return (0);

	/* Skip a.out
	 */
	if (strcmp(shortname, "a.out") == 0)
		return (0);

}/*E if howmuch > 1 */

return (1);	/* Default action is not to skip */

}/*E checkf() */

/*.sbttl done() */
done(n)
{

	int	i,status;
	extern int device_open;

status = unlink(tname);
close(mt);
device_open = 0;

if (NFLAG)
	exit(n);

if (n == A_WRITE_ERR) {

	struct linkbuf	*lihead;
	struct linkbuf	*linkp;
	

	/* Return malloc'd linked file list to freemem
	 */
	for (lihead = ihead; lihead;) {
		linkp = lihead->nextp;	
		free ((char *)lihead);
		lihead = linkp;
	}
	ihead = 0;

	/* Return malloc'd directory list to freemem
	 */
	fdlist();

	start_archive = CARCH;
	size_of_media[CARCH] = size_of_media[0];
	blocks_used = 0L;
	PUTE = AFLAG = EOTFLAG = EODFLAG = recno = NMEM4L = NMEM4D = 0;
	dcount1 = dcount2 = dcount3 = lcount1 = lcount2 = 0;

	if (chdir(hdir) < 0) {
		fprintf(stderr, "%s: Can't change directory back ?", progname);
		perror(hdir);
		exit(FAIL);
	}

REOPEN:
/**/
	fprintf(stderr,"\007%s: Please press  RETURN  to re-write %s %d ",progname, Archive, CARCH);

	CARCH = 1;
	sprintf(CARCHS, "%d", CARCH);
	response();
	fprintf(stderr,"\n%s: Starting error recovery\n",progname);

	if ((mt = statchk(usefile, O_RDWR))< 0) {
		fprintf(stderr,"\n%s: Can't open: %s\n",progname,usefile);
		if(errno)perror(usefile);
		goto REOPEN;
	}
	return(A_WRITE_ERR);
}
if (n == SUCCEED)
	n = 0;

exit(n);

}/*E done() */

/*.sbttl fdlist() */

fdlist()
{

	struct DIRE	*lDhead;
	struct DIRE	*dlinkp;


if (Dhead)
	dcount3++;

/* Return malloc'd directory list to freemem
 */
for (lDhead = Dhead; lDhead;) {

	dlinkp = lDhead->dir_next;	
	free ((char *)lDhead);
	lDhead = dlinkp;
}
Dhead = 0;

}/*E fdlist() */

/*.sbttl flushtape() */
flushtape()
{
	extern int curbuf;
	extern struct atblock *fblock[];

if (CARCH >= start_archive) {
	if (recno) {
		mtops.mt_op = MTCSE;
		if (is_tape && !MDTAR && device_open &&
		    (ioctl(mt,MTIOCTOP,&mtops)< 0)) {
			fprintf(stderr, "%s: ", progname);
			perror("ioctl MTCSE failed on write");
		}
		if (write(mt, (char *)&fblock[curbuf][0], TBLOCK*nblock) < 0) {
			if ((ioctl(mt, DEVIOCGET, &mtsts)<0) ||
				size_of_media[CARCH] ||
				(errno != ENOSPC) || NFLAG) {
FERR:
/**/
				fprintf(stderr,"\n\n\007%s: Archive %d write error on last block\n",
					progname, CARCH);

				fprintf(stderr,"%s: Blocks used = %ld\n",progname, blocks_used);
				perror(usefile);
				done(A_WRITE_ERR);
				return(A_WRITE_ERR);
			}
			else {
				if (!(mtsts.stat & DEV_EOM))
					goto FERR;
				else {
					mtops.mt_op = MTCSE;
					if (ioctl(mt,MTIOCTOP,&mtops)< 0)
						goto FERR;
					else {

						OARCH = CARCH;
						EOTFLAG++;
						MULTI++;
						FEOT++;

				if(writetape(&fblock[curbuf][recno],recno,recno,
						(char *)cblock.dbuf.name,
						(char *)cblock.dbuf.name, 1) == A_WRITE_ERR)
							goto FERR;
					}
				}
			}
		}/*E if write ..*/
	}/*E if recno */
}/*E if CARCH >= start_archive */

if (vflag) {
	if (CARCH >= start_archive) {
		if (MULTI)
	        	fprintf(stderr,"\n%s: End of %s media",
				progname,Archive);
		if (VFLAG)
			fprintf(stderr,"\n%s: %ld Blocks used on %s for %s %d\n",
				progname, blocks_used, usefile, Archive, CARCH);
	}
	else
		if (AFLAG || ((start_archive > CARCH) && VFLAG))
			fprintf(stderr,"%s: %s %d  skipped.\n",
				progname, Archive, CARCH);
}/*E if vflag */

recno = 0;
return(SUCCEED);

}/*E flushtape() */

/*.sbttl parse() - Parse the command line */
/* Parse the command line and set up control variables.
 */
parse(argc,argv)
	int	argc;
	char	*argv[];
{

	char *cp;
	int i;

chksum = 2;

progname = argv[0];	/* Get our name*/

set_size(0L);	/* Default media size to tapes/files = unlimited */

if (!(cp = rindex(progname,'/')))
	cp = progname;
else 
	cp++;


if ((strcmp(cp,mdtar))) {
	MDTAR = FALSE;
}
else {
	/* Task name MDTAR implies multiple archive function
	 * with rx50 as the default output device.
	 */
	MDTAR = TRUE;
	magtape[6] = 'r';
	magtape[7] = 'a';
	magtape[8] = '1';
	magtape[9] = 'a';
}

if (argc < 2)
	usage();

tfile = NULL;

/* Setup the default archive
 */
usefile =  magtape;
argv[argc] = 0;
argv++;

for (cp = *argv++; *cp; cp++) {
	int uid;

	switch(*cp) {
		/*
	 	 *  Switches that either TAR or MDTAR will accept..
	 	 */

		/* A: Archive. Number of the archive, counting from 1,
		 * to begin physically writing.
		 */
		case 'A':
			if (!*argv) {
				fprintf(stderr, "%s: Archive number must be specified with 'A' option\n", progname);
				usage();
			}
			start_archive = atoi(*argv++);
			chksum++;
			if ((start_archive < 1) ||
				(start_archive > MAXARCHIVE) ) {

				fprintf(stderr, "%s: Invalid archive number:  %d\n", progname,start_archive);
				done(FAIL);
			}
			AFLAG++;
			break;

		/* b: use next argument as blocksize
		 */
		case 'b':
			bflag++;
			if (!*argv) {
				fprintf(stderr, "%s: Blocksize must be specified with 'b' option\n", progname);
				usage();
			}
			nblock = atoi(*argv++);
			chksum++;

			if (nblock <= 0 || nblock > MAXBLK) {
				fprintf(stderr, "%s: Invalid blocksize \"%d\"\n", progname, nblock);
				done(FAIL);
			}
			break;

		/* B: Force I/O blocking to 20 blocks per record
		 */
		case 'B':
			Bflag++;
			break;

		/* -c: create new archive. Note that -r is implied
		 * by this switch also.
		 */
		case 'c':
			cflag++;
			rflag++;
			break;

		/* -D: Directory output in old style to conserve
		 * memory and archive utilization.
		 */
		case 'D':
			DFLAG++;
			break;

		/* -d: select default diskette device.
		 */
		case 'd':
			/* Set up default media size table.
 			*/
			dflag++;
			magtape[6] = 'r';
			magtape[7] = 'a';
			magtape[8] = '1';
			magtape[9] = 'a';
			if (unitflag)
				magtape[8] = unitc;
			break;

		/* -f: use next argument as the archive of choice
		 * instead of the default.
		 */
		case 'f':
			if (!*argv) {
				fprintf(stderr, "%s: Archive file  must be specified with 'f' option\n", progname);
				usage();
			}
			usefile = *argv++;
			strcpy(magtape,usefile);
			chksum++;
			if (cflag) {
				i = stat(usefile, &stbuf);
				if ((i<0) || ((stbuf.st_mode & S_IFMT) == S_IFREG)) {
					if (!sflag && !MDTAR)
						set_size(0L);

					if (open(usefile,O_TRUNC,0)<0) {
						if (errno != ENOENT) {
							fprintf(stderr, "%s: Can't open:  %s\n", progname, usefile);
							perror(usefile);
							done(FAIL);
						}
					}
				}
			}
			fflag++;
			break;

		/* F: Fast. F causes SCCS dirs, core & errs files
		 * to be skipped.
		 * FF:	Skip  .o & a.out's also
		 * FFF: Skip executable files.
		 */
		case 'F':
			Fflag++;
			break;

		/* H: User has requested the help function. Provide
		 * expanded information about the switches/options.
		 */
		case 'H':
			HELP++;
			usage();

		/* h: Treat symbolic links as if they were normal
		 * files. ie. Put a physical copy of the linked to
		 * file on the archive.
		 */
		case 'h':
			hflag++;
			break;
		/* l: Print errors if all links to a file cannot
		 * be resolved.
		 */
		case 'l':
			lflag++;
			break;

		/* R: The named files contain lists of files which are to
                 * be put into or extracted from the archive.
                 */
		case 'R':
                        Rflag++;
                        break;

		/* -r: Write named files at the END of the archive.
		 *     Implied in -u & -c
		 */
		case 'r':
			rflag++;
			break;

		/* -u: Add to archive only those files
		 *     not already in/on it.
		 */
		case 'u':
			mktemp(tname);
			if ((tfile = fopen(tname, "w")) == NULL) {
	    		    fprintf(stderr, "%s: Can't create temporary file (%s)\n", progname, tname);
			    perror(tname);
			    done(FAIL);
			}
			fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");

			/* IMPLIED 'r' FUNCTION 
			 */
			rflag++;
			break;

		/* V: VERBOSE mode. Big verbose displays directory
		 * information not displayed by little verbose.
		 * Note that 'V' implies 'v'.
		 */
		case 'V':
			VFLAG++;
			/*_FALL THRU_*/

		/* v: Verbose mode.  Display the filenames as they
		 * are processed. Also supply further information
		 * about the archive and operation. See man page.
		 */
		case 'v':
			vflag++;
			if (VFLAG)
				fprintf(stderr,"\n%s: rev. %d.%-d\n", progname,revwhole,revdec);
			break;

		/* w: Wait mode. Request user confirmation prior to
		 * processing each file argument.
		 */
		case 'w':
			wflag++;
			break;

		/* n: Select an alternate unit number.
		 */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			unitflag++;
			unitc = *cp;

			if (!fflag)
				magtape[8] = *cp;

				/* w/o dflag Ultrix-11 dev = /dev/rht1
				 *	     Ultrix-Pro dev= /dev/rrx1
				 *           Ultrix-32 dev = /dev/rmt0h
				 */ 
			usefile = magtape;

			break;

		/* -: ignored. To allow some degree of consistency
		 * with other unix commands that do not insist on the
		 * leading -.
		 */
		case '-':
			break;

		/* i: Do not terminate if checksum errors are detected
		 * during an archive read.
		 */
		case 'i':
			iflag++;
			break;

		/* m: Do not restore modification times from the input
		 * archive file header block.
		 */
		case 'm':
			mflag++;
			break;

		/* M: Specify maximum writtable archive number.
		 * (for debugging)
		 */
		case 'M':
			if (!*argv) {
				fprintf(stderr, "%s: Archive number must be specified with 'M' option\n", progname);
				usage();
			}
			MAXAR = atoi(*argv++);
			chksum++;
			if ((MAXAR < 1) ||
				(MAXAR > MAXARCHIVE) ) {
				fprintf(stderr, "%s: Invalid archive number:  %d\n", progname,MAXAR);
				done(FAIL);
			}
			MFLAG++;
			break;

		/* N: multi-archive, file splitting, or
		 * new header format.
		 */
		case 'N':
			NFLAG = 0;
			set_size(0L);
			break;

		/* O: Include file owner and group names in 
		 * verbose output if present in this archive format.
		 */
		case 'O':
			OFLAG++;
			break;

		/* o: create the archive with directory information
		 * to produce archives that can be used to
		 * fully restore directory information.
	 	 * this adds directory permission and ownership
		 * info to the archive.
		 */
		case 'o':
			oflag++;
			break;

		/* p: Restore files to original their orginal modes
		 * and owners as recorded in the file header block.
		 * NOTE: This is only allowed if you are the super-user.
		 */
		case 'p':
			uid = geteuid();

			pflag++;

			if(vflag && (uid != 0))
				fprintf(stderr,"\n%s: \007Warning: The 'p' option will only restore the modes of files you own\n\n", progname);
			 break;
		case 'P' :		/* set the posix mode */
			is_posix = 1;
			break;
		/* S: Output User Group Standard archive format.
		 */
		case 'S':
			SFLAG++;
			break;

		/* s:  Size of the media in blocks.
		 */
		case 's':
			if (!*argv) {
				fprintf(stderr, "%s: Media size must be specified with 's' option\n", progname);
				usage();
			}
			size_of_media[0] = atoi(*argv++);
			chksum++;

			if (size_of_media[0] <= 4) {
		    		fprintf(stderr, "%s: Invalid media size:  %s\n", progname, *argv);
		    		done(FAIL);
			}
			set_size(size_of_media[0]);
			sflag++;
			break;


		/* t: List the names of the files that are in/on the
		 * given archive.
		 */
		case 't':
			tflag++;
			break;

		/* x: Extract the named files from the given
		 * input archive.
		 */
		case 'x':
			xflag++;
			break;

		default:
			fprintf(stderr, "%s: Unknown option:  %c\n", progname, *cp);
			usage();

		}/*E switch(*cp)*/
}/*E for (cp = *argv++; *cp; cp++) */


NOARGS:
/*----:
 */
if (MAXAR < start_archive) {
	fprintf(stderr,"\n%s: Max archive < start_archive\n",progname);
	done(FAIL);
}

/*
 * Check to see that a function letter was specified, 
 * either directly or by implication.
 */
if (!rflag && !xflag && !tflag)
	usage();

if (cflag || rflag)
	if (argc < 3)
		usage();


/* Does the user want to add files to the end of the current
 * archive device ?
 */
if (rflag) {

	if (cflag && tfile != NULL) {
		fprintf(stderr,"%s: c and u functions may not be given in the same command\n", progname);
		done(FAIL);
	}
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, onhup);

#ifdef notdef
 *	Added signal handler for SIGUSR1 to allow toggling
 *	of verbose mode by sending the process a USR1 signal.
	if (signal(SIGUSR1, SIG_IGN) != SIG_IGN)
		signal(SIGUSR1, onusr1);

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);

	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, onquit);

	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, onterm);
#endif

	/* Has user specified  stdout  as the archive meadia ?
 	 */
	if (strcmp(usefile, "-") == 0) {
		if (!cflag) {
			fprintf(stderr, "%s: Can only CREATE standard output archives\n", progname);
			done(FAIL);
		}
		/* When pipe output has been specified,
		 * ignore invocation name of MDTAR
		 * and any possible size of device given by
		 * cancelling the flags.
 		 */
		MDTAR = FALSE;
		sflag = FALSE;
		set_size(0L);
		mt = dup(1);
		nblock = PIPSIZ; /* Size of pipes */
	} else {
		int status;
TRYOPA:
/**/
	 	if ((mt = statchk(usefile, O_RDWR)) < 0) {
			if (!cflag || mt < 0) {
				if(errno){
				    fprintf(stderr, "\n\007%s: Can't open:  %s\n", progname, usefile);
				    perror(usefile);
				}
				if(!isatty(0))
				  exit(FAIL);

				fprintf(stderr,"%s: Please press  RETURN  to retry ", progname);
				response();
				goto TRYOPA;
			}
		}
				
	}
	goto endit;

}/*E if rflag */

/* Has user specified  stdin  as the archive device ?
 */
if (!strcmp(usefile, "-")) {
	pipein++;
	Bflag++;
	mt = dup(0);
	nblock = PIPSIZ; /* 4k pipes */
} else {
OPMTA:
/**/
	if ((mt = statchk(usefile, O_RDONLY)) < 0) {
		if(errno){
		  fprintf(stderr, "\n\007%s: Can't open:  %s\n", progname,usefile);
		  perror(usefile);
		}
		if(!isatty(0))
		  exit(FAIL);

		fprintf(stderr,"%s: Please press RETURN  to retry ", progname);	
		response();
		goto OPMTA;
	}
}
endit:
if (AFLAG && !size_of_media[0]) {
	fprintf(stderr,"\n%s: Media size conflict\n",progname);
	done(FAIL);
}

/* Open worked, enable async */
{
	extern int doingasync;
	int cnt=MAXASYNC, i;

	if(mt_info.category == DEV_TAPE){

#ifdef notyet
          /* enable cacheing */
	  mtops.mt_op = MTCACHE;
	  mtops.mt_count = 1;
	  (void)ioctl(mt, MTIOCTOP, &mtops);
#endif notyet
		
	   if(ioctl(mt, FIONBUF, &cnt) < 0)
		doingasync = 0;
	   else
		doingasync = cnt;
	}

	if(alloctape() <= 0)
		doingasync = 0;
}

return(chksum);

}/*E parse() */

/*.sbttl putempty()  Put an empty (EOF) block on archive */
putempty()
{
  int i;

  for (i=0; i < sizeof(iobuf); i++)
       iobuf[i] = 0;

  if (writetape(iobuf,1,1,eoa,eoa, 1) == A_WRITE_ERR)
      return(A_WRITE_ERR);

  return(SUCCEED);

}/*E putempty() */

/*.sbttl puteoa()  Put an end of archive block in the buffer */
puteoa()
{
	char *cp;
	char *from;
	COUNTER	nc;
	char *to;
	extern int curbuf;
	extern struct atblock *fblock[MAXASYNC];

for (to = (char *)&fblock[curbuf][recno++], nc = TBLOCK; nc; nc--)
	*to++ = 0;
for (to = (char *)&fblock[curbuf][recno++], nc = TBLOCK; nc; nc--)
	*to++ = 0;

/* Copy file name into output buffer.
 */
for (from = (char *)dblock.dbuf.name, to = (char *)&fblock[curbuf][recno], nc=NAMSIZ; nc; nc--)
	*to++ = *from++;

/* Indicate End of Archive by ascii zero fill of remaining header
 * block fields.
 */
for (nc=TBLOCK-NAMSIZ-1,cp = (char *)&fblock[curbuf][recno]+NAMSIZ; nc ; nc--)
	*cp++ = '0';

return(flushtape());

}/*E puteoa() */

/*.sbttl response()  Get a yes/no answer from user */
response()
{
	char	c;

c = getchar();
if (c != '\n')
	while (getchar() != '\n')
		;/*_NOP_*/
else
	c = 'n';

if (term)
	done(FAIL);

fprintf(stderr,"\n\n");
return (c);

}/*E response() */

/*.sbttl set_size() */
set_size(size)
	SIZE_L	size; 
{
	int i;


for (i = MAXARCHIVE; i >= 0; i--)
	size_of_media[i] = size;

return;

}/*E set_size() */

/*.sbttl strfind() */
char *strfind(text,substr)
		char *text, *substr;
{
	COUNTER	i;	/* counter for possible fits */
	SIZE_I	substrlen;/* len of substr--to avoid recalculating */


substrlen = strlen(substr);

/* Loop through text until not found or match.
 */
for (i = strlen(text) - substrlen; i >= 0 &&
     strncmp(text, substr, substrlen); i--)

	text++;

/* Return NULL if not found, ptr else NULL.
 */ 
return ((i < 0 ? NULL : text));

}/*E strfind() */

/*.sbttl usage()  Show the user correct command format(s) */

usage()
{

if (!HELP)
	fprintf(stderr,"\n%s: specify H (Help) for expanded definition of switches\n", progname);
	fprintf(stderr, "\nusage:\n%s [-]{crtux} [ABb-CDdFfHhilMmNOopsVvwR] [archivefile] [blocksize]\n   [archivenumber] [maxarchive #] [mediasize] [archivefile]\n   directory/file1 directory/file2 ..\n\n", progname);

	/* If user has selected HELP mode, give an expanded version
	 * of the letters and switches. Useful on small systems
	 * that don't have man pages and user doesn't have a
	 * manual set handy.
	 */
if (HELP) {
	printf("%s: One of the function keys enclosed in  {}  is required.\n\n", progname);
	printf("%s: c = create new archive, previous content is overwritten\n", progname);
	printf("%s: r = revise archive by adding files to end of current content\n", progname);
	printf("%s: t = give table of contents with verbosity defined by v or V\n", progname);
	printf("%s: u = update archive. Add files to end either if they are not already there\n", progname);
	printf("%s:     or if they have been modified since last put to archive. \n", progname);
	printf("%s: x = extract files from the named archive\n", progname);
	
	printf("\n%s: Items enclosed in  []  are optional\n\n", progname);
	printf("\n\n\n\n\n%s: Press RETURN to continue ..", progname); 
	response();
	fprintf(stderr,"\n\n");

	printf("%s: A = use next argument as archive number with which to begin output\n", progname);

	printf("%s: B = Invoke proper blocking logic for tar functions across machines\n", progname);
	printf("%s: b = use next argument as blocking factor for archive records\n", progname);
	printf("%s:-C = change directory to following file name (-C dirname)\n", progname);
	printf("%s: D = Directory output in original tar style\n", progname);
	printf("%s: d = select rx50 as output\n", progname);
	printf("%s:     (/dev/rra1a)\n", progname);


	printf("%s: F[F] = operate in fast mode. Skip all SCCS directories, core files,\n", progname);
	printf("%s:        & errs file. FF also skips a.out and *.o files\n", progname);
	printf("%s: f = use following argument as the name of the archive\n", progname);
	printf("%s: H = Help mode. Print this summary\n", progname);
	printf("%s: h = have a copy of a symbolically linked\n", progname);
	printf("%s:     file placed on archive instead of symbolic link\n", progname);
	printf("%s: i = ignore checksum errors in header blocks\n", progname);
	printf("%s: l = output link resolution error message summary\n", progname);
	printf("%s: M = Next arg specifies maximum archive number to be written and\n",progname);
	printf("%s:     prints current archive number on output line\n",progname);
	printf("%s: m = do not restore modification times. Use time of extraction\n", progname);

	printf("\n\n%s: Press RETURN to continue ..", progname); 
	response();
	fprintf(stderr,"\n\n");

	printf("%s: N = No multi-archive, file splitting, or new header format on output\n", progname);
	printf("%s:     Output directories in previous tar format. On input, set file\n",progname);
	printf("%s:     UID & GID from file header vs. values in /etc/passwd & group files\n",progname);

	printf("%s: O = include file owner & group names in verbose output (t & x functions)\n",progname);
	printf("%s:     if present in archive header.\n",progname);
	printf("%s:     Output warning message if owner or group name not found in\n",progname);
	printf("%s:     /etc/passwd or /etc/group file (cru functions)\n",progname);
	printf("%s: o = add directory block information to output archive\n", progname);

	printf("%s: p = change permissions and owner of extracted files to original values\n", progname);
	printf("%s:     (only works if you are the super user)\n", progname);
	printf("%s: S = Output User Group Standard archive format\n",progname);
	printf("%s: s = next argument specifies size of archive in 512 byte blocks\n", progname);
	printf("%s: V = enhanced verbose. Most informative information about current operation\n", progname);
	printf("%s: v = verbose mode. Provide additional information about files/operation\n", progname);
	printf("%s: w = wait mode. Ask for user confirmation before including specified file\n", progname);
	printf("%s: R = recursive mode. Use a file with a list of files to archive\n", progname);
}/*E if HELP */

done(FAIL);

}/*E usage() aka useage*/

