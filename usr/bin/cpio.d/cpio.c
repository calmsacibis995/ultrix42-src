# ifndef lint
static char *sccsid = "@(#)cpio.c	4.4	(ULTRIX)	4/11/91";
# endif /* not lint */

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
/*
 * cpio.c
 *	Modification History
 *	------------ -------
 *
 *	09-Apr-91	lambert
 *			Add "compatability mode" (-C flag) to be able to
 *			read old, "pre-CLD 126 era" binary archives.
 *
 *	07-Feb-91	lambert
 *			Further fix for CLD #126, mentioned below.  Cpio
 * 			was changed to simply not archive files with inodes
 *			greater than those representable in 18 bits.  This
 *			was deemed unacceptable.  Since the inode numbers in
 *			the archive are not really used (upon extraction the 
 *			archived inode #'s cannot be replicated - that's a
 *			function of the underlying filesystem) the algorith 
 *			was changed to use an "internal" inode 
 *			number for files.  A map of inodes with multiple links
 *			was also created, to facilitate maintaining links on
 *			extraction from the archive.  Additionally, the inode
 *			counter is kept on a "per device" basis (based on the
 *			dev number returned by stat()) so cpio can support up
 *			to 256K distinct inodes per device.
 *
 *			Changed EOT message, per Andy's request.
 *
 *	18-Sep-90	carito
 *			Fixed negative uid and gid values.  Specifically,
 *			extracted sixteen least-significant-bits.  See
 *			sprintf().  QAR 03546  Version 4FT1 10-JAN-1990
 *			service request A900110-1109
 *
 *	08-Aug-90	kegel
 *			Upgraded *_ino from ushort to ino_t (ulong) 
 *			and fixed related sscanf to pick up a long
 *			instead of a short;  ditto related printf.
 *			A CLD reported that files were being mistakenly
 *			linked during extraction because of wrap-around
 *			in the 16-bit inode numbers used within cpio.
 *			CLD #126  MUH_01595
 *
 *			Also, added code for "MAGIC" fix in cpio.h to
 *			meet XPG3.  This fixes the problem for which Digital
 *			has a temporary waiver.  MAGIC was defined as an
 *			octal number (070707) in cpio.h;  XPG3 expects
 *			it to be an octal string ("070707").  Look for
 *			MAGIC and imagic to find the changes - trivial.
 *
 *	07-Feb-90	lambert
 *			Changed declaration of "m_ino" in postml() routine
 *			from type short to type ushort due to conflict with
 *			ushort type of "h_ino" in cpio header.
 *
 * 	10-Jan-90	lambert
 *			Added code for "Pflag" processing to support (non-)use 
 *			of umask when writing out files.  Also added support 
 *			for named pipes.
 *
 *	29-Sep-89	lambert
 *			Added reference to X/Open compliant <cpio.h> file.
 *
 *	20-Aug-89	lambert
 *			Fixed null pointer problem in main().
 *
 *	27-Sep-88	lambert
 *			Added code to ensure symbolic "linked to" file name
 *			is correct (routine: main(), line 475).  (V24_QAR #533)
 *	
 *	24-Aug-87	fries
 *			Corrected bug to provide correct name
 *			length for symbolic link data.
 *			(SMU-01690)
 *
 *	 5-Jan-87	fries
 *			Completed adding symbolic link support
 *			to PASS, IN and OUT functions.
 *			Cleaned up code and commented.
 *
 *	12-Nov-86	robin
 *			Added symbolic link support to IN and
 *			OUT functions.
 *
 *	29-May-86	fries	
 *			Added copyright notice.	
 *
 *	29-May-86	fries
 *			corrected typo utime to utimes
 *
 */
#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <limits.h>
#include <cpio.h>
#include <errno.h>
#ifdef	RT
#include <rt/macro.h>
#include <rt/types.h>
#include <rt/stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#define EQ(x,y)	(strcmp(x,y)==0)
/* for VAX, Interdata, ... */
#define MKSHORT(v,lv) {U.l=1L;if(U.c[0]) U.l=lv,v[0]=U.s[1],v[1]=U.s[0]; else U.l=lv,v[0]=U.s[0],v[1]=U.s[1];}
#define IN	1		/* copy in */
#define OUT	2		/* copy out */
#define PASS	3		/* directory copy */
#define HDRSIZE	(Hdr.h_name - (char *)&Hdr)	/* header size minus filename field */
#define O_HDRSIZE (O_Hdr.h_name - (char *)&O_Hdr)	/* Old header size minus filename field */
#define LINKS	1000		/* max no. of links allowed */
#define CHARS	76		/* ASCII header size    */
				/* minus filename field */
#define BUFSIZE 512		/* In u370, can't use BUFSIZ nor BSIZE */
#define CPIOBSZ 4096		/* file read/write */
#define printd if(debug) fprintf/* if debug mode(see int below) */
#define STAT(n,d) (Slink)?lstat(n,d):stat(n,d) /* type of stat to use */
int debug=0;			/* flag to signal debug mode */
#ifdef RT
extern long filespace;
#endif
#define	OCTAL	8		/* for MAGIC/imagic fix */
#define MAXDEVS 1000		/* max # of h_dev entries per archive */
				/* (needed for "inode too big" fix) */

struct	stat	Statb, Xstatb;  /* file stat structure  */

/* Cpio header format */
struct header {
	short	h_magic,
		h_dev;
	ino_t	h_ino;
	ushort	h_mode,
		h_uid,
		h_gid;
	short	h_nlink,
		h_rdev,
		h_mtime[2],
		h_namesize,
		h_filesize[2];
	char	h_name[256];
} Hdr;

/* Old Cpio header format */
struct old_header {
	short	h_magic,
		h_dev;
	ushort	h_ino,		/* Note difference between old + new headers */
		h_mode,
		h_uid,
		h_gid;
	short	h_nlink,
		h_rdev,
		h_mtime[2],
		h_namesize,
		h_filesize[2];
	char	h_name[256];
} O_Hdr;

unsigned	Bufsize = BUFSIZE;/* default record size */
short	Buf[CPIOBSZ/2], *Dbuf;    /* 2048 byte buffer    */
char	BBuf[CPIOBSZ], *Cbuf;	  /* Character Mode buffer */
char 	*Cptr;			  /* general use char. ptr.*/
int	Wct, Wc, Symct;		  /* Counts(words, bytes, and    */
				  /* # of bytes in symbolic link */
short	*Wp;                      /* Word Pointer(!Cflag) */
char	*Cp;			  /* Character Pointer(Cflag) */
unsigned	imagic;		  /* integer form of MAGIC from cpio.h */

struct linkbuf {
	ino_t	real_ino;
	dev_t	devnum;
	ino_t	fake_ino;
	struct	linkbuf *next_lbp;
} *lbp;
long internal_ino[MAXDEVS]; 	  /* "internal" inode number */
short devlist[MAXDEVS];		  /* list of dev #s encountered */
short devidx = 0;		  /* global index into devlist array */
short last_dev_seen = 0;	  /* more device indexing noise */

ino_t next_ino();		  /* make pcc shutup about redeclarations */

#ifdef RT
short	Actual_size[2];
#endif

/* Flags */
short	Option,
	Dir,
	Uncond,
	Link,
	Rename,
	Toc,
	Slink,
	Verbose,
	Select,
	Mod_time,
	Acc_time,
	Cflag,
	C_mode,
	fflag,
 	Pflag,
#ifdef RT
	Extent,
#endif
	Swap,
	byteswap,
	bothswap,
	halfswap;

/* file descriptors */
int	Ifile,
	Ofile,
	Input = 0,
	Output = 1;

long	Blocks,
	Longfile,
	Longtime;

char	Fullname[256],
	Name[256];
int	Pathend;

/* File Pointers for tty read and write */
FILE	*Rtty,
	*Wtty;

char	*Pattern[100];
char	Strhdr[500];
char	*Chdr = Strhdr;
char	symstring[BUFSIZE*2];

/* Device # , uid #, gid # & Type of File */
short	Dev,
	Uid,
	Gid,
	A_directory,
	A_special,
	A_symlink,
#ifdef RT
	One_extent,
	Multi_extent,
#endif
	Filetype = S_IFMT;

/* External Globals */
extern	errno;

/* External Functions */
char	*malloc();
FILE 	*popen();

union { long l; short s[2]; char c[4]; } U;

/* for VAX, Interdata, ... */
long mklong(v)
short v[];
{
	U.l = 1;
	if(U.c[0])
		U.s[0] = v[1], U.s[1] = v[0];
	else
		U.s[0] = v[0], U.s[1] = v[1];
	return U.l;
}

/* Main Code Entry Point */
main(argc, argv)
	int argc;
	char **argv;
{
	char *namep;
	register ct;
	long	filesz;
	register char *fullp;
	register i;
	int ans, oumask;

	signal(SIGSYS, 1);
	imagic = (unsigned) strtoul(MAGIC, NULL, OCTAL );

	/* check for options signaller */
	if(argc < 2)
		usage();

	/* get uid */
	Uid = getuid();

	/* save old user mask and set to zero */
 	oumask = umask(0);

	/* get group id */
	Gid = getgid();

	/* set default for patterns to match */
	Pattern[0] = "*";

	/* handle options */
	while(*++argv[1]) {
		switch(*argv[1]) {
		case 'a':		/* reset access time */
			Acc_time++;
			break;
		case 'B':		/* change record size to 5120 bytes */
			Bufsize = 5120;
			break;
		case 'i':		/* input data */
			Option = IN;
			if(argc > 2 ) {	/* save patterns, if any */
				for(i = 0; (i+2) < argc; ++i)
					Pattern[i] = argv[i+2];
			}
			break;
		case 'f':		/* copy all except patterns */
			fflag++;
			break;
		case 'k':		/* read/create symbolic links */
			Slink++;
			break;
		case 'o':		/* output data */
			if(argc != 2)
				usage();
			Option = OUT;
			break;
 		case 'P':		/* POSIX mode */
			Pflag++;
			break;

		case 'p':		/* directory copy */
			if(argc != 3)
				usage();
			if(access(argv[2], 2) == -1) {
accerr:
				fprintf(stderr,"cannot write in <%s>\n", argv[2]);
				exit(2);
			}
			/* destination directory */
			strcpy(Fullname, argv[2]);
			strcat(Fullname, "/");

			/* Check for name being a directory */
			stat(Fullname, &Xstatb);
			if((Xstatb.st_mode&S_IFMT) != S_IFDIR)
				goto accerr;
			Option = PASS;
			Dev = Xstatb.st_dev;
			break;
		case 'c':		/* ASCII header */
			Cflag++;
			break;
		case 'C':		/* Compatability mode - use old header */
			C_mode++;
			break;
		case 'd':		/* create directories when needed */
			Dir++;
			break;
		case 'l':		/* link files, when necessary */
			Link++;
			break;
		case 'm':		/* retain mod time */
			Mod_time++;
			break;
		case 'r':		/* rename files interactively */
			Rename++;
			Rtty = fopen("/dev/tty", "r");
			Wtty = fopen("/dev/tty", "w");
			if(Rtty==NULL || Wtty==NULL) {
				fprintf(stderr,
				  "Cannot rename (/dev/tty missing)\n");
				exit(2);
			}
			break;
		case 'S':		/* swap halfwords */
			halfswap++;
			Swap++;
			break;
		case 's':		/* swap bytes */
			byteswap++;
			Swap++;
			break;
		case 'b':		/* swap bytes and halfwords */
			bothswap++;
			Swap++;
			break;
		case 't':		/* table of contents */
			Toc++;
			break;
		case 'u':		/* copy unconditionally */
			Uncond++;
			break;
		case 'v':		/* verbose table of contents */
			Verbose++;
			break;
		case 'V':		/* big Verbose mode */
			Verbose++;
			debug++;	/* set "debug" int (see above) */
			break;
		case '6':		/* for old, sixth-edition files */
			Filetype = 060000;
			break;
#ifdef RT
		case 'e':
			Extent++;
			break;
#endif
		default:
			usage();
		}
	}
	/* Check that the user specified input, output or pass */
	if(!Option) {
		fprintf(stderr,"Options must include o|i|p\n");
		exit(2);
	}
	/* Only allow compatability mode when reading in */
	if (Option != IN && C_mode) {
		fprintf(stderr, "-C only valid on copy in operations - ignored\n");
		C_mode = 0;
	}
#ifdef RT
		setio(-1,1);	/* turn on physio */
#endif
	/* if Pflag specified, and user is not root, reset mask to what
	   it was originally (POSIX/XOPEN mode) */
	if ((Pflag) && (Uid != 0)) umask(oumask);

	/* If directory copy and Rename selected together... */
	if(Option == PASS) {
		if(Rename) {
			fprintf(stderr,"Pass and Rename cannot be used together\n");
			exit(2);
		}
		if(Bufsize == 5120) {
			printf("`B' option is irrelevant with the '-p' option\n");
			printf("Setting Input/Output blocking to %d bytes to a record\n", BUFSIZE);
			Bufsize = BUFSIZE;
		}

	}else  {
		if(Cflag)
		    Cp = Cbuf = (char *)malloc(Bufsize);
		else
		    Wp = Dbuf = (short *)malloc(Bufsize);
	}
	Wct = Bufsize >> 1; /* set word count */
	Wc = Bufsize;       /* and byte count */

	/* Perform OUTPUT, INPUT, or PASS operations */
	switch(Option) {

	case OUT:		/* get filename, copy header and file out */
		while(getname()) {
			if(A_symlink){
				Symct=readlink(Hdr.h_name, Cflag? BBuf:(char *)Buf, Cflag? sizeof(BBuf): sizeof(Buf));
				if (Symct < 0) {
					perror("cpio");
					fprintf(stderr,"Cannot read %s\n", Hdr.h_name);
					continue;
				}
				strncpy(symstring,Cflag ? (char *)BBuf : (char *)Buf,Symct);

				Cptr = Cflag ? (char *)BBuf : (char *) Buf;
				Cptr[Symct] = '\0' ;
				symstring[Symct]='\0';
				printd(stderr,"Smyct %d\n BUF: %s\n symstring %s\n namesize %d\n filesize %d\n",
				Symct,(Cflag?(char *)BBuf:(char *)Buf),symstring,Hdr.h_namesize,Hdr.h_filesize[1]);
				
				/* Write the header */
				if ( Cflag )
					writehdr(Chdr,CHARS+Hdr.h_namesize);
				else
					bwrite(&Hdr, HDRSIZE+Hdr.h_namesize);

				Cflag? writehdr(BBuf,(int) mklong(Hdr.h_filesize)): bwrite(Buf,(int) mklong(Hdr.h_filesize));
				goto setout;
			}
			if( mklong(Hdr.h_filesize) == 0L) {
			if( Cflag )
				writehdr(Chdr,CHARS+Hdr.h_namesize);
			else
				bwrite(&Hdr, HDRSIZE+Hdr.h_namesize);
#ifdef RT
			if (One_extent || Multi_extent) {
			   actsize(0);
			   if( Cflag )
				writehdr(Chdr,CHARS+Hdr.h_namesize);
			   else
				bwrite(&Hdr, HDRSIZE+Hdr.h_namesize);
			}
#endif
				continue;
			}
			if((Ifile = open(Hdr.h_name,O_RDONLY )) < 0) {
				fprintf(stderr,"<%s> ?\n", Hdr.h_name);
				continue;
			}
			if ( Cflag )
				writehdr(Chdr,CHARS+Hdr.h_namesize);
			else
				bwrite(&Hdr, HDRSIZE+Hdr.h_namesize);
#ifdef RT
			if (One_extent || Multi_extent) {
			   actsize(Ifile);
			   if(Cflag)
				writehdr(Chdr,CHARS+Hdr.h_namesize);
			   else
				bwrite(&Hdr, HDRSIZE+Hdr.h_namesize);
			   Hdr.h_filesize[0] = Actual_size[0];
			   Hdr.h_filesize[1] = Actual_size[1];
			}
#endif
			for(filesz=mklong(Hdr.h_filesize); filesz>0; filesz-= CPIOBSZ){
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
				if(read(Ifile, Cflag? BBuf: (char *)Buf, ct) < 0) {
					fprintf(stderr,"Cannot read %s\n", Hdr.h_name);
					continue;
				}
				Cflag? writehdr(BBuf,ct): bwrite(Buf,ct);
			}
			close(Ifile);
setout:
			if(Acc_time)
				utimes(Hdr.h_name, &Statb.st_atime);
			if(Verbose)
				fprintf(stderr,"%s\n", Hdr.h_name);
		}

		/* Output the TRAILER data...                     */
		/* copy trailer, after all files have been copied */
		strcpy(Hdr.h_name, "TRAILER!!!");
		Hdr.h_magic = imagic;
		MKSHORT(Hdr.h_filesize, 0L);
		Hdr.h_namesize = strlen("TRAILER!!!") + 1;
		if ( Cflag )  {
			bintochar(0L);
			writehdr(Chdr,CHARS+Hdr.h_namesize);
		}
		else
			bwrite(&Hdr, HDRSIZE+Hdr.h_namesize);
		Cflag? writehdr(Cbuf, Bufsize): bwrite(Dbuf, Bufsize);
		break;

	case IN:
		pwd();
		while(gethdr()) {
			Ofile = ckname(Hdr.h_name)? openout(Hdr.h_name): 0;
			if(A_symlink) {
				namep = Hdr.h_name;
				if(!strncmp(namep,"./",2))
					namep += 2;
				Cflag? readhdr(BBuf,(int)mklong(Hdr.h_filesize)): bread(Buf, (int)mklong(Hdr.h_filesize));

				if(!Toc){
					printd(stderr,"BBUF: %s\n namesize %d\n filesize %d\n",
					BBuf,Hdr.h_namesize,Hdr.h_filesize[1]);

					/* Clear out 'symstring' for it's entire
					   length to ensure a clean slate.
						(from V24_QAR #533)
					*/
					strncpy(symstring, "", sizeof(symstring));
					strncpy(symstring,Cflag? (char *)BBuf: (char *)Buf, (int)mklong(Hdr.h_filesize));
					umask(oumask);
					if(symlink( symstring, Hdr.h_name) < 0) {
						perror("cpio");
						fprintf(stderr,"Cannot create symlink %s\n", Hdr.h_name);
						umask(0);
						continue;
					}
					umask(0);
					if(Uid == 0)
						if(chown(namep, Hdr.h_uid, Hdr.h_gid) < 0) {
							fprintf(stderr,"Cannot chown <%s> (errno:%d)\n", namep, errno);
						}
					set_time(namep, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
				}
			}else {
				for(filesz=mklong(Hdr.h_filesize); filesz>0; filesz-= CPIOBSZ){
					ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
					Cflag? readhdr(BBuf,ct): bread(Buf, ct);
					if(Ofile) {
						if(Swap)
						   Cflag? swap(BBuf,ct): swap(Buf,ct);
						if(write(Ofile, Cflag? BBuf: (char *)Buf, ct) < 0) {
						 fprintf(stderr,"Cannot write %s\n", Hdr.h_name);
						 continue;
						}
					}
				}
			}
			if(Ofile) {
				close(Ofile);
				if(!A_symlink)
 				if(!Pflag)
				if(chmod(Hdr.h_name, Hdr.h_mode) < 0) {
					fprintf(stderr,"Cannot chmod <%s> (errno:%d)\n", Hdr.h_name, errno);
				}
				set_time(Hdr.h_name, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
			}
			if(!Select)
				continue;
			if(Verbose)
				if(Toc)
					pentry(Hdr.h_name);
				else
					puts(Hdr.h_name);
			else if(Toc)
				puts(Hdr.h_name);
		}
		break;

	case PASS:		/* move files around */
		fullp = Fullname + strlen(Fullname);

		while(getname()) {
			if (A_directory && !Dir)
				fprintf(stderr,"Use `-d' option to copy <%s>\n",Hdr.h_name);
			if(!ckname(Hdr.h_name))
				continue;
			i = 0;
			while(Hdr.h_name[i] == '/')
				i++;
			strcpy(fullp, &(Hdr.h_name[i]));
			if(A_symlink)
			{
				
				Symct=readlink(Hdr.h_name, Cflag? BBuf:(char *)Buf, Cflag? sizeof(BBuf): sizeof(Buf));
				if(Symct < 0){
					perror("cpio");
					fprintf(stderr,"Cannot read %s\n", Hdr.h_name);
					continue;
				}
				Cptr = Cflag ? (char *)BBuf : (char *) Buf;
				Cptr[Symct]='\0';

				umask(oumask);
				if(symlink(Cptr,Fullname) < 0) {
					perror("cpio");
					fprintf(stderr,"Cannot create symlink %s-> %s\n", Fullname, BBuf);
					umask(0);
					continue;
				}
				umask(0);
				goto set;
			}

			if(Link
			&& !A_directory
			&& Dev == Statb.st_dev)  {
				if(link(Hdr.h_name, Fullname) < 0) { /* missing dir.? */
					unlink(Fullname);
					missdir(Fullname);
					if(link(Hdr.h_name, Fullname) < 0) {
						fprintf(stderr,
						 "Cannot link <%s> & <%s>\n",
						 Hdr.h_name, Fullname);
						continue;
					}
				}

				/* try creating (only twice) */
				ans = 0;
				do {
					/* missing dir. ? */
					if(link(Hdr.h_name, Fullname) < 0) {
						unlink(Fullname);
						ans += 1;
					}else {
						ans = 0;
						break;
					}
				}while(ans < 2 && missdir(Fullname) == 0);
				if(ans == 1) {
					fprintf(stderr,"Cannot create directory for <%s> (errno:%d)\n", Fullname, errno);
					exit(0);
				}else if(ans == 2) {
					fprintf(stderr,"Cannot link <%s> & <%s>\n", Hdr.h_name, Fullname);
					exit(0);
				}

				if(!A_symlink)
 				if(!Pflag)
				if(chmod(Hdr.h_name, Hdr.h_mode) < 0) {
					fprintf(stderr,"Cannot chmod <%s> (errno:%d)\n", Hdr.h_name, errno);
				}
				set_time(Hdr.h_name, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
				goto ckverbose;
			}
#ifdef RT
			if (One_extent || Multi_extent)
				actsize(0);
#endif
			if(!(Ofile = openout(Fullname)))
				continue;
			if((Ifile = open(Hdr.h_name, O_RDONLY)) < 0) {
				fprintf(stderr,"<%s> ?\n", Hdr.h_name);
				close(Ofile);
				continue;
			}
			filesz = Statb.st_size;
			for(; filesz > 0; filesz -= CPIOBSZ) {
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
				if(read(Ifile, Buf, ct) < 0) {
					fprintf(stderr,"Cannot read %s\n", Hdr.h_name);
					break;
				}
				if(Ofile)
					if(write(Ofile, Buf, ct) < 0) {
					 fprintf(stderr,"Cannot write %s\n", Hdr.h_name);
					 break;
					}
#ifndef u370
				Blocks += ((ct + (BUFSIZE - 1)) / BUFSIZE);
#else
				++Blocks;
#endif
			}
			close(Ifile);
set:
			if( (Acc_time) && (!A_symlink) )
				utimes(Hdr.h_name, &Statb.st_atime);
			if(Ofile) {
				close(Ofile);
				if(!A_symlink)
 				if(!Pflag)
				if(chmod(Fullname, Hdr.h_mode) < 0) {
					fprintf(stderr,"Cannot chmod <%s> (errno:%d)\n", Fullname, errno);
				}
				set_time(Fullname, Statb.st_atime, mklong(Hdr.h_mtime));
ckverbose:
				if(Verbose)
					puts(Fullname);
			}
			if(A_symlink)
				if(Verbose)
					puts(Fullname);
		}
	}

	/* print number of blocks actually copied */
	if(Blocks)
		fprintf(stderr,"%ld blocks\n", Blocks * (Bufsize>>9));

	/* Normal Exit */
	exit(0);
}
usage()
{
	fprintf(stderr,"Usage: cpio -o[acvkBP] <name-list >collection\n%s\n%s\n",
	"       cpio -i[bcdmrstuvfkBPS6] [pattern ...] <collection",
	"       cpio -p[adlmuvkP] directory <name-list");
	exit(2);
}

/* Set up header with filename and other file information */
getname()
{
	register char *namep = Name;
	register ushort ftype;
	long tlong;

	for(;;) {
		if(gets(namep) == NULL)
			return 0;
		if(*namep == '.' && namep[1] == '/')
			namep += 2;
		strcpy(Hdr.h_name, namep);
		if(STAT(namep, &Statb) < 0) {
			fprintf(stderr,"< %s > ?\n", Hdr.h_name);
			continue;
		}
		ftype = Statb.st_mode & Filetype;
		if(Slink)
			A_symlink = (ftype == S_IFLNK);
		A_directory = (ftype == S_IFDIR);
		A_special = (ftype == S_IFBLK)
			|| (ftype == S_IFCHR)
			|| (ftype == S_IFIFO)
			|| (ftype == S_IFSOCK);
#ifdef RT
		A_special |= (ftype == S_IFREC);
		One_extent = (ftype == S_IF1EXT);
		Multi_extent = (ftype == S_IFEXT);
#endif
		Hdr.h_magic = imagic;
		Hdr.h_namesize = strlen(Hdr.h_name) + 1;
		Hdr.h_uid = Statb.st_uid;
		Hdr.h_gid = Statb.st_gid;
		Hdr.h_dev = Statb.st_dev;

		/* 
		 * Maintain a list of devices seen by cpio.
		 * NB: if "last_dev_seen == 0" then it's the first
		 * device cpio has encountered on this invocation.
		 */
		if (last_dev_seen == 0) {
#ifdef DEBUG
			if (devidx != 0) {
				fprintf(stderr, "cpio has problems with devidx\n");
			}
#endif
			devlist[devidx] = last_dev_seen = Hdr.h_dev;
		}
		if (Hdr.h_dev != last_dev_seen) {
			short i;
			for (i = devidx; i >= 0; i--) {
				if (Hdr.h_dev == devlist[i]) {
					devidx = i;
					break;
				}
			}

			/* 
			 * Wasn't found on search, and isn't the same as 
			 * "last" device, so it must be a new one.  
			 */
			if (i < 0) {
				devlist[++devidx] = Hdr.h_dev;
			}

			last_dev_seen = devlist[devidx];
		}

		/* 
		 * Fix for "inode too big" error.
		 * There is no reason to use actual inode numbers of files
		 * in the archive as this information cannot be used on 
		 * copy-in operations.  So an artificial inode number is created
		 * internally and used in the header in place of the "real" one.
		 * A linked list of those inodes with multiple links is maintained
		 * so the real inode can be "remembered" on subsequent accesses
		 * (see routine match_ino()).
		 */
		Hdr.h_nlink = Statb.st_nlink;
		if (Hdr.h_nlink > 1) {
			match_ino();
		}
		else {
			/* Set inode in header to our "internal" number */
			Hdr.h_ino = next_ino();
		}

		Hdr.h_mode = Statb.st_mode;
		MKSHORT(Hdr.h_mtime, Statb.st_mtime);

		tlong = (ftype == S_IFREG)? (long) Statb.st_size: 0L;
		if (A_symlink)
			tlong = (long) Statb.st_size;
#ifdef RT
		if (One_extent || Multi_extent) tlong = Statb.st_size;
#endif
		MKSHORT(Hdr.h_filesize, tlong);
		Hdr.h_rdev = Statb.st_rdev;
		if( Cflag )
		   bintochar(tlong);
		printd(stderr,"GETNAME\n\tname %s\n\tmagic %o \n\t namesize %d filesize %d\n",
			namep, Hdr.h_magic, Hdr.h_namesize,tlong);
		return 1;
	}
}

/* Convert Binary to ASCII for ASCII header write */
bintochar(t)	
long t;
{
	sprintf(Chdr,"%.6o%.6ho%.6lo%.6ho%.6ho%.6ho%.6ho%.6ho%.11lo%.6ho%.11lo%s",
		imagic,Statb.st_dev,Statb.st_ino,Statb.st_mode,
		Statb.st_uid & 00000177777, Statb.st_gid & 00000177777,
		Statb.st_nlink,Statb.st_rdev & 00000177777,
		Statb.st_mtime,(short)strlen(Hdr.h_name)+1,t,Hdr.h_name);
}

/* Convert ASCII to binary for ASCII header read */
chartobin()
{
	sscanf(Chdr,"%6ho%6ho%6lo%6ho%6ho%6ho%6ho%6ho%11lo%6ho%11lo",
		&Hdr.h_magic,&Hdr.h_dev,&Hdr.h_ino,&Hdr.h_mode,&Hdr.h_uid,
		&Hdr.h_gid,&Hdr.h_nlink,&Hdr.h_rdev,&Longtime,&Hdr.h_namesize,
		&Longfile);
	MKSHORT(Hdr.h_filesize, Longfile);
	MKSHORT(Hdr.h_mtime, Longtime);
}

/* Get File Header */
gethdr()
{
	register ushort ftype;

	if (Cflag)  {
		readhdr(Chdr,CHARS);
		chartobin();
	}
	else {
		if (C_mode) {	/* User specified compatability mode - use old header format */

			bread(&O_Hdr, O_HDRSIZE);

			/* Place old header values into new header */
			Hdr.h_magic = O_Hdr.h_magic;
			Hdr.h_dev = O_Hdr.h_dev;
			Hdr.h_ino = (ino_t)O_Hdr.h_ino;
			Hdr.h_mode = O_Hdr.h_mode;
			Hdr.h_uid = O_Hdr.h_uid;
			Hdr.h_gid = O_Hdr.h_gid;
			Hdr.h_nlink = O_Hdr.h_nlink;
			Hdr.h_rdev = O_Hdr.h_rdev;
			Hdr.h_namesize = O_Hdr.h_namesize;
			Hdr.h_mtime[0] = O_Hdr.h_mtime[0];
			Hdr.h_mtime[1] = O_Hdr.h_mtime[1];
			Hdr.h_filesize[0] = O_Hdr.h_filesize[0];
			Hdr.h_filesize[1] = O_Hdr.h_filesize[1];
		}
		else {
			/* Use normal, "new" header format */
			bread(&Hdr, HDRSIZE);
		}
	}

	if(Hdr.h_magic != imagic) {
		fprintf(stderr,"Out of phase--get help\n");
		exit(2);
	}
	if(Cflag)
		readhdr(Hdr.h_name, Hdr.h_namesize);
	else
		bread(Hdr.h_name, Hdr.h_namesize);
	printd(stderr,"GETHDR\n\tname %s\n\tmagic %o \n\t namesize %d filesize %d\n",
		Hdr.h_name, Hdr.h_magic, Hdr.h_namesize,mklong(Hdr.h_filesize));
	if(EQ(Hdr.h_name, "TRAILER!!!"))
		return 0;
	ftype = Hdr.h_mode & Filetype;
	if(Slink)
		A_symlink = (ftype == S_IFLNK);
	A_directory = (ftype == S_IFDIR);
	A_special =(ftype == S_IFBLK)
		|| (ftype == S_IFCHR)
		|| (ftype == S_IFIFO)
		|| (ftype == S_IFSOCK);
#ifdef RT
	A_special |= (ftype == S_IFREC);
	One_extent = (ftype == S_IF1EXT);
	Multi_extent = (ftype == S_IFEXT);
	if (One_extent || Multi_extent) {
		Actual_size[0] = Hdr.h_filesize[0];
		Actual_size[1] = Hdr.h_filesize[1];
		if (Cflag)  {
			readhdr(Chdr,CHARS);
			chartobin();
		}
		else
			bread(&Hdr, HDRSIZE);
	
		if(Hdr.h_magic != imagic) {
			fprintf(stderr,"Out of phase--get RT help\n");
			exit(2);
		}
		if(Cflag)
			readhdr(Hdr.h_name, Hdr.h_namesize);
		else
			bread(Hdr.h_name, Hdr.h_namesize);
	}
#endif
	return 1;
}

/* Compare filename with pattern given on Command Line */
ckname(namep)
register char *namep;
{
	++Select;
	if(fflag ^ !nmatch(namep, Pattern)) {
		Select = 0;
		return 0;
	}
	/* If Interactive Rename... */
	if(Rename && !A_directory) {
		fprintf(Wtty, "Rename <%s>\n", namep);
		fflush(Wtty);
		fgets(namep, 128, Rtty);
		if(feof(Rtty))
			exit(2);
		namep[strlen(namep) - 1] = '\0';
		if(EQ(namep, "")) {
			printf("Skipped\n");
			return 0;
		}
	}
	return !Toc;
}

/* Open files for writing, setup all neccessary information */
openout(namep)
register char *namep;
{
	register f;
	register char *np;
	int ans;

	if(!strncmp(namep, "./", 2))
		namep += 2;
	np = namep;

	/* If a directory... */
	if(A_directory) {
		if(!Dir
		|| Rename
		|| EQ(namep, ".")
		|| EQ(namep, ".."))	/* do not consider . or .. files */
			return 0;
		if(STAT(namep, &Xstatb) == -1) {

			/* try creating (only twice) */
			ans = 0;
			do {
				if(makdir(namep) != 0) {
					ans += 1;
				}else {
					ans = 0;
					break;
				}
			}while(ans < 2 && missdir(namep) == 0);
			if(ans == 1) {
				fprintf(stderr,"Cannot create directory for <%s> (errno:%d)\n", namep, errno);
				return(0);
			}else if(ans == 2) {
				fprintf(stderr,"Cannot create directory <%s> (errno:%d)\n", namep, errno);
				return(0);
			}
		}

ret:
		if(!A_symlink)
 		if(!Pflag)
		if(chmod(namep, Hdr.h_mode) < 0) {
			fprintf(stderr,"Cannot chmod <%s> (errno:%d)\n", namep, errno);
		}
		if(Uid == 0)
			if(chown(namep, Hdr.h_uid, Hdr.h_gid) < 0) {
				fprintf(stderr,"Cannot chown <%s> (errno:%d)\n", namep, errno);
			}
		set_time(namep, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
		return 0;
	}
	if(Hdr.h_nlink > 1)
		if(!postml(namep, np))
			return 0;
	if(stat(namep, &Xstatb) == 0) {
		if(Uncond && !((!(Xstatb.st_mode & S_IWRITE) || A_special) && (Uid != 0))) {
			if(unlink(namep) < 0) {
				fprintf(stderr,"cannot unlink current <%s> (errno:%d)\n", namep, errno);
			}
		}
		if(!Uncond && (mklong(Hdr.h_mtime) <= Xstatb.st_mtime)) {
			/* There's a newer version of file on destination */
			if(mklong(Hdr.h_mtime) < Xstatb.st_mtime)
				fprintf(stderr,"current <%s> newer\n", np);
			return 0;
		}
	}
	if(Option == PASS
	&& Hdr.h_ino == Xstatb.st_ino
	&& Hdr.h_dev == Xstatb.st_dev) {
		fprintf(stderr,"Attempt to pass file to self!\n");
		exit(2);
	}
	if(A_symlink) 
		return 0;
	if(A_special) {
		if (((Hdr.h_mode & Filetype) == S_IFSOCK) || 
		    ((Hdr.h_mode & Filetype) == S_IFIFO))
			Hdr.h_rdev = 0;

		/* try creating (only twice) */
		ans = 0;
		do {
			if(mknod(namep, Hdr.h_mode, Hdr.h_rdev) < 0) {
				ans += 1;
			}else {
				ans = 0;
				break;
			}
		}while(ans < 2 && missdir(np) == 0);
		if(ans == 1) {
			fprintf(stderr,"Cannot create directory for <%s> (errno:%d)\n", namep, errno);
			return(0);
		}else if(ans == 2) {
			fprintf(stderr,"Cannot mknod <%s> (errno:%d)\n", namep, errno);
			return(0);
		}

		goto ret;
	}
#ifdef RT
	if(One_extent || Multi_extent) {

		/* try creating (only twice) */
		ans = 0;
		do {
			if((f = falloc(namep, Hdr.h_mode, longword(Hdr.h_filesize[0]))) < 0) {
				ans += 1;
			}else {
				ans = 0;
				break;
			}
		}while(ans < 2 && missdir(np) == 0);
		if(ans == 1) {
			fprintf(stderr,"Cannot create directory for <%s> (errno:%d)\n", namep, errno);
			return(0);
		}else if(ans == 2) {
			fprintf(stderr,"Cannot create <%s> (errno:%d)\n", namep, errno);
			return(0);
		}

		if(filespace < longword(Hdr.h_filesize[0])){
			fprintf(stderr,"Cannot create contiguous file <%s> proper size\n", namep);
			fprintf(stderr,"    <%s> will be created as a regular file\n", namep);
			if(unlink(Fullname) != 0)
				fprintf(stderr,"<%s> not removed\n", namep);
			Hdr.h_mode = (Hdr.h_mode & !S_IFMT) | S_IFREG;
			One_extent = Multi_extent = 0;
		}
	Hdr.h_filesize[0] = Actual_size[0];
	Hdr.h_filesize[1] = Actual_size[1];
	}
	if (!(One_extent || Multi_extent)) {
#endif

	/* try creating (only twice) */
	ans = 0;
	do {
		if((f = creat(namep, Hdr.h_mode)) < 0) {
			ans += 1;
		}else {
			ans = 0;
			break;
		}
	}while(ans < 2 && missdir(np) == 0);
	if(ans == 1) {
		fprintf(stderr,"Cannot create directory for <%s> (errno:%d)\n", namep, errno);
		return(0);
	}else if(ans == 2) {
		fprintf(stderr,"Cannot create <%s> (errno:%d)\n", namep, errno);
		return(0);
	}

#ifdef RT
	}
#endif
	if(Uid == 0)
		chown(namep, Hdr.h_uid, Hdr.h_gid);
	return f;
}

/* Binary Read */
bread(b, c)
register c;
register short *b;
{
	static nleft = 0;
	static short *ip;
	register int rv;
	register short *p = ip;
	register int in;
	printd(stderr,"%s  ",Hdr.h_name);

	printd(stderr,"bread size %d\n", c);
	c = (c+1)>>1;
	while(c--) {
		if(nleft == 0) {
			in = 0;
			while((rv=read(Input, &(((char *)Dbuf)[in]), Bufsize - in)) != Bufsize - in) {
				if(rv <= 0) {
					Input = chgreel(0, Input);
					continue;
				}
				in += rv;
				nleft += (rv >> 1);
			}
			nleft += (rv >> 1);
			p = Dbuf;
			++Blocks;
		}
		*b++ = *p++;
		--nleft;
	}
	ip = p;
}

/* Read Header */
readhdr(b, c)
register c;
register char *b;
{
	static nleft = 0;
	static char *ip;
	register int rv;
	register char *p = ip;
	register int in;
	printd(stderr,"%s  ",Hdr.h_name);
	printd(stderr,"readhdr size %d \n",c);

	while(c--)  {
		if(nleft == 0) {
			in = 0;
			while((rv=read(Input, &(((char *)Cbuf)[in]), Bufsize - in)) != Bufsize - in) {
				if(rv <= 0) {
					Input = chgreel(0, Input);
					continue;
				}
				in += rv;
				nleft += rv;
			}
			nleft += rv;
			p = Cbuf;
			++Blocks;
		}
		*b++ = *p++;
		--nleft;
	}
	ip = p;
}

/* Binary Write */
bwrite(rp, c)
register short *rp;
register c;
{
	register short *wp = Wp;
	printd(stderr,"%s  ",Hdr.h_name);
	printd(stderr,"bwrite size = %d \n",c);

	c = (c+1) >> 1;
	while(c--) {
		if(!Wct) {
again:
			if(write(Output, Dbuf, Bufsize)<0) {
				Output = chgreel(1, Output);
				goto again;
			}
			Wct = Bufsize >> 1;
			wp = Dbuf;
			++Blocks;
		}
		*wp++ = *rp++;
		--Wct;
	}
	Wp = wp;
}

/* Write Header */
writehdr(rp, c)
register char *rp;
register c;
{
	register char *cp = Cp;
	printd(stderr,"%s  ",Hdr.h_name);
	printd(stderr,"writehdr size %d \n",c);
	printd(stderr,"writehdr Hdr.h_filesize %d \n", Hdr.h_filesize);
	printd(stderr,"writehdr Hdr.h_namesize %d \n", Hdr.h_namesize);

	while(c--)  {
		if(!Wc)  {
again:
			if(write(Output,Cbuf,Bufsize)<0)  {
				Output = chgreel(1,Output);
				goto again;
			}
			Wc = Bufsize;
			cp = Cbuf;
			++Blocks;
		}
		*cp++ = *rp++;
		--Wc;
	}
	Cp = cp;
}

/* Perform Post Linking */
postml(namep, np)
register char *namep, *np;
{
	register i;
	static struct ml {
		short	m_dev;
		ino_t	m_ino;
		char	m_name[2];
	} *ml[LINKS];
	static	mlinks = 0;
	char *mlp;
	int ans;

	for(i = 0; i < mlinks; ++i) {
		if(mlinks == LINKS) break;
		if(ml[i]->m_ino==Hdr.h_ino &&
			ml[i]->m_dev==Hdr.h_dev) {
			if(Verbose)
			  printf("%s linked to %s\n", ml[i]->m_name,
				np);
			unlink(namep);
			if(Option == IN && *ml[i]->m_name != '/') {
				Fullname[Pathend] = '\0';
				strcat(Fullname, ml[i]->m_name);
				mlp = Fullname;
			}
			mlp = ml[i]->m_name;

			/* try linking (only twice) */
			ans = 0;
			do {
				if(link(mlp, namep) < 0) {
					ans += 1;
				}else {
					ans = 0;
					break;
				}
			}while(ans < 2 && missdir(np) == 0);
			if(ans == 1) {
				fprintf(stderr,"Cannot create directory for <%s> (errno:%d)\n", np, errno);
				return(0);
			}else if(ans == 2) {
				fprintf(stderr,"Cannot link <%s> & <%s>.\n", ml[i]->m_name, np);
				return(0);
			}

			set_time(namep, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
			return 0;
		}
	}
	if(mlinks == LINKS
	|| !(ml[mlinks] = (struct ml *)malloc(strlen(np) + 2 + sizeof(struct ml)))) {
		static int first=1;

		if(first)
			if(mlinks == LINKS)
				fprintf(stderr,"Too many links\n");
			else
				fprintf(stderr,"No memory for links\n");
		mlinks = LINKS;
		first = 0;
		return 1;
	}
	ml[mlinks]->m_dev = Hdr.h_dev;
	ml[mlinks]->m_ino = Hdr.h_ino;
	strcpy(ml[mlinks]->m_name, np);
	++mlinks;
	return 1;
}

/* Print Verbose Table of Contents */
pentry(namep)
register char *namep;
{

	static short lastid = -1;
#include <pwd.h>
	static struct passwd *pw;
	struct passwd *getpwuid();
	static char tbuf[32];
	char *ctime();

	printf("%-7o", Hdr.h_mode & 0177777);
	if(lastid == Hdr.h_uid)
		printf("%-6s", pw->pw_name);
	else {
		setpwent();
		if(pw = getpwuid((int)Hdr.h_uid)) {
			printf("%-6s", pw->pw_name);
			lastid = Hdr.h_uid;
		} else {
			printf("%-6d", Hdr.h_uid);
			lastid = -1;
		}
	}
	printf("%7ld ", mklong(Hdr.h_filesize));
	U.l = mklong(Hdr.h_mtime);
	strcpy(tbuf, ctime((long *)&U.l));
	tbuf[24] = '\0';
	printf(" %s  %s\n", &tbuf[4], namep);
}

/* pattern matching functions */
nmatch(s, pat)
char *s, **pat;
{
	if(EQ(*pat, "*"))
		return 1;
	while(*pat) {
		if((**pat == '!' && !gmatch(s, *pat+1))
		|| gmatch(s, *pat))
			return 1;
		++pat;
	}
	return 0;
}
gmatch(s, p)
register char *s, *p;
{
	register int c;
	register cc, ok, lc, scc;

	scc = *s;
	lc = 077777;
	switch (c = *p) {

	case '[':
		ok = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (ok)
					return(gmatch(++s, ++p));
				else
					return(0);

			case '-':
				ok |= ((lc <= scc) && (scc <= (cc=p[1])));
			}
			if (scc==(lc=cc)) ok++;
		}
		return(0);

	case '?':
	caseq:
		if(scc) return(gmatch(++s, ++p));
		return(0);
	case '*':
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if (c==scc) goto caseq;
	return(0);
}

umatch(s, p)
register char *s, *p;
{
	if(*p==0) return(1);
	while(*s)
		if (gmatch(s++,p)) return(1);
	return(0);
}

/* make needed directories */
makdir(namep)
register char *namep;
{
	static status;
	register pid;

	if(pid = fork())
		while(wait(&status) != pid);
	else {
		close(2);
		execl("/bin/mkdir", "mkdir", namep, 0);
		exit(2);
	}
	return ((status>>8) & 0377)? 1: 0;
}

/* swap halfwords, bytes or both */
swap(buf, ct)
register ct;
register char *buf;
{
	register char c;
	register union swp { long	longw; short	shortv[2]; char charv[4]; } *pbuf;
	int savect, n, i;
	char *savebuf;
	short cc;

	savect = ct;	savebuf = buf;
	if(byteswap || bothswap) {
		if (ct % 2) buf[ct] = 0;
		ct = (ct + 1) / 2;
		while (ct--) {
			c = *buf;
			*buf = *(buf + 1);
			*(buf + 1) = c;
			buf += 2;
		}
		if (bothswap) {
			ct = savect;
			pbuf = (union swp *)savebuf;
			if (n = ct % sizeof(union swp)) {
				if(n % 2)
					for(i = ct + 1; i <= ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
				else
					for (i = ct; i < ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
			}
			ct = (ct + (sizeof(union swp) -1)) / sizeof(union swp);
			while(ct--) {
				cc = pbuf->shortv[0];
				pbuf->shortv[0] = pbuf->shortv[1];
				pbuf->shortv[1] = cc;
				++pbuf;
			}
		}
	}
	else if (halfswap) {
		pbuf = (union swp *)buf;
		if (n = ct % sizeof(union swp))
			for (i = ct; i < ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
		ct = (ct + (sizeof(union swp) -1)) / sizeof(union swp);
		while (ct--) {
			cc = pbuf->shortv[0];
			pbuf->shortv[0] = pbuf->shortv[1];
			pbuf->shortv[1] = cc;
			++pbuf;
		}
	}
}

/* set access and modification times */
set_time(namep, atime, mtime)
register *namep;
long atime, mtime;
{
	static long timevec[2];

	if( (A_symlink) || (!Mod_time))
		return;
	timevec[0] = atime;
	timevec[1] = mtime;
	utime(namep, timevec);
}

/* Request a Reel Change(for multi-volume tapes */
chgreel(x, fl)
{
	register f;
	char str[22];
	FILE *devtty;
	struct stat statb;

	fprintf(stderr,"errno: %d, ", errno);
	if (errno == ENOSPC) {
		fprintf(stderr, "No space left on device\n");
	}
	else {
		fprintf(stderr,"Can't %s\n", x? "write output": "read input");
	}
	fstat(fl, &statb);
#ifndef RT
	if((statb.st_mode&S_IFMT) != S_IFCHR)
		exit(2);
#else
	if((statb.st_mode & (S_IFBLK|S_IFREC))==0)
		exit(2);
#endif
again:
	fprintf(stderr,"If you want to go on, type device/file name when ready\n");
	devtty = fopen("/dev/tty", "r");
	fgets(str, 20, devtty);
	str[strlen(str) - 1] = '\0';
	if(!*str)
		exit(2);
	close(fl);
	if((f = open(str, x? O_WRONLY: O_RDONLY)) < 0) {
		fprintf(stderr,"That didn't work");
		fclose(devtty);
		goto again;
	}
	return f;
}

/* Missing Directory */
missdir(namep)
register char *namep;
{
	register char *np;
	register ct = 2;

	for(np = namep; *np; ++np)
		if(*np == '/') {
			/* skip over 'root slash' */
			if(np == namep) continue;
			*np = '\0';
			if(STAT(namep, &Xstatb) == -1) {
				if(Dir) {
					if((ct = makdir(namep)) != 0) {
						*np = '/';
						return(ct);
					}
				}else {
					fprintf(stderr,"missing 'd' option\n");
					return(-1);
				}
			}
			*np = '/';
		}
	if (ct == 2) ct = 0;		/* the file already exists */
	return ct;
}

/* get working directory */
pwd()	
{
	FILE *dir;

	dir = popen("pwd", "r");
	fgets(Fullname, 256, dir);
	if(pclose(dir))
	{
		perror("cpio pwd");
		exit(2);
	}
	Pathend = strlen(Fullname);
	Fullname[Pathend - 1] = '/';
}

#ifdef RT
actsize(file)
register int file;
{
	long tlong;
	long fsize();
	register int tfile;

	Actual_size[0] = Hdr.h_filesize[0];
	Actual_size[1] = Hdr.h_filesize[1];
	if (!Extent)
		return;
	if (file)
		tfile = file;
	else if ((tfile = open(Hdr.h_name,O_RDONLY)) < 0)
		return;
	tlong = fsize(tfile);
	MKSHORT(Hdr.h_filesize,tlong);
	if (Cflag)
		bintochar(tlong);
	if (!file)
		close(tfile);
}
#endif

match_ino()
{
	short i, found_ino = 0;
	struct linkbuf *idx, *save_lbp;

	for (idx = lbp; idx; idx = idx->next_lbp) {
		save_lbp = idx;
		if (idx->real_ino == Statb.st_ino &&
		    idx->devnum == Statb.st_dev) {

			/* 
			 * Found the link.  Set inode field in
			 * this file's header to be the "fake" 
			 * inode of the linked-to file.
			 */
			Hdr.h_ino = idx->fake_ino;
			found_ino++;
			break;
		}
	}

	if (!found_ino) {

		/* Inode not found on list; create new entry */
		idx = (struct linkbuf *) malloc(sizeof(*idx));
		if (!idx) {
			fprintf(stderr, "Out of memory for links\n");
			fprintf(stderr, "File link data may be corrupt\n");
			return(0);
		}
		idx->real_ino = Statb.st_ino;
		idx->devnum = Statb.st_dev;
		Hdr.h_ino = idx->fake_ino = next_ino();

		if (lbp) {
			save_lbp->next_lbp = idx;
		}
		else {
			lbp = idx;
		}
	}
	return(0);
}

/*
 * Establish next available internal inode.
 * Inodes are allocated (as in reality) on a per devnum basis.
 */
ino_t
next_ino() 
{
	if (internal_ino[devidx]++ > 0777777L) {
		fprintf(stderr,
			"Cannot archive < %s > (too many files in archive)\n",
			Hdr.h_name);
		exit(2);
	}
	return(internal_ino[devidx]);
}
