
# ifndef lint
static char *sccsid = "@(#)readtape.c	4.2	(ULTRIX)	12/6/90";
# endif not lint

/****************************************************************
 *								*
 *			Copyright (c) 1985, 1987, 1988, 1989 by	*
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
/*
 *
 *	File name:
 *
 *		readtape.c
 *
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *	10-Sep-90	lambert
 *			Fixed problem with archive switching on non-tape
 *			archives.  Doxtract was hosing the "my_name" char
 *			pointer, thus not allowing files on "n+1" archives
 *			to be extracted.
 *
 *	25-Mar-90	kegel
 *			Fixed "append CLD" discovered by Boeing.
 *			See note about "xtra_backspace" for more detail:
 *			Added code to check for case when End-of-Archive
 *			(two zero-filled blocks of 512 by, starting with
 *			a name-block) spans two tape records (10240 by).
 *			Code does additional Tape Record Backspace to
 *			compensate for tape-read-ahead with N-buffered I/O.
 *			Changed V version to 20.1, what version to CLD-FIX.
 *			Latent bug:  did *not* add code to fix problem when
 *			End-of-Archive spans spools of tape.
 *
 *	02-Mar-90	lambert
 *			Added "smarts" to file/device-opening code so that
 *			it will no longer prompt to "retry" files that don't
 *			exist.
 *
 *	12-Jan-90	lambert
 *			Corrected action in getdir() for determining tar
 *			header type.  Was not picking up some info from
 *			POSIX header.
 *
 *	31-Oct-89	rsp
 *			Updated POSIX define usage to be upto spec.
 *
 *	 3-Aug-89	rsp
 *			Changed MTBSR to MTBSF as per Tim Burke's request
 *			so that repositioning actions line up with new
 *			behavior of tape drivers.
 *			Added new EOT code for the sake of SCSI tape
 *			devices which generally don't do read EOT
 *			notification. Only failure case now is if
 *			archive/file is multi-volume but data does not
 *			span volume. In this case, tar will complete
 *			a given volume but not prompt for next volume.
 *			User must know to load next tape and issue
 *			tar command anew. This condition does not occur
 *			very often so it should not be a problem.
 *
 *	30-May-89	bstevens
 *			Added new -R switch for lists of archive files.
 *
 *	16-May-89	rsp
 *			Added status checking for ioctl's related to
 *			tape positioning. Report all ioctl failures
 *			except those related to nbuffio since some of
 *			these 'errors' are used to branch to certain code.
 *
 *      31-Aug-88       lambert 
 *                      Changed include line from "tar.h" to <tar.h> for
 *                      POSIX support.
 *
 *	15-Mar-88	mjk - added posix support
 *
 *	16-Dec-87	fries - added support for named pipes.
 *
 *	23-Sep-87	fries - corrected code which read past EOT on tape.
 *			(was giving directory checksum errors on next tape)
 *
 *	18-May-87	rsp - Removed tu78 code since driver is now fixed.
 *
 *	14-Apr-87	fries - Added code to handle tu78 work around.
 *
 *	16-Mar-87	fries - removed curbuf = 0 in endtape function.
 *			Buffer was getting corrupted by faulty index.
 *
 *	29-Jan-87	lp - Do MTCSE prior to checking DEVIOCGET
 *
 *	15-Jan-87	fries - Corrected -p incompatibility
 *			problem. 
 *			Removed -p usage when xtracting sym.
 *			links.
 *
 *	 8-Jan-87	fries - Modified endtape to always
 *			disable and re-enable nbuffering.
 *
 *	22-Dec-86	fries - Fixed problem blank tape with use of -r
 *			option gave "tar: blocksize 0" and archive
 *			backspace error messages.
 *
 *	19-Dec-86	fries - Made struct mtsts available to
 *			the bread() function.
 *
 *	 2-Dec-86	fries - Modified method of detecting when
 *			EOT mark was crossed.
 *
 *	17-Nov-86	fries - Added code to disallow
 *			restoration of a symbolic link atop
 *			an existing directory.
 *
 *	17-Nov-86	fries - Added code to restore
 *			permissions of special files to
 *			original.
 *
 *	23-Oct-86	lp - bugfix for eot. bugfix for backtape.
 *
 *	16-Oct-86	lp - Number of bugfixes
 *
 *	 9-Oct-86	lp
 *			Fixed a bug where multiple files on a single
 *			tape would not work with n-buffered reads.
 *
 *	25-Aug-86	lp
 *			Fixed a bug where multiple files on a single
 *			tape would not work with n-buffered reads.
 *
 *	02-Jul-86	lp
 *			N-bufferring now works with eot.
 *
 *	19-Jun-86	lp
 *			Added hooks for n-buffered i/o. Cleanup &
 *			reformat somewhat. Remove non-U32 ifdefs.
 *
 *	II		15-May-86 fries/
 *			Added support for generic device ioctl(statchk
 *			calls)
 *	
 *	I		19-Dec-85 rjg/
 *			Create orginal version.
 *	
 */
#include <tar.h>

extern int device_open;
extern struct devget mt_info;

backtape()
{
static struct devget mtsts;
static int deviog = 1;
static	struct	mtop	lmtop = {MTBSR, 1};

if(deviog == 1)
	deviog = ioctl(mt, DEVIOCGET, &mtsts);

if (deviog == 0 && mtsts.category == DEV_TAPE) {
	if (ioctl(mt, MTIOCTOP, &lmtop) < 0) {
		fprintf(stderr, "%s: Archive backspace error\n", progname);
		done(FAIL);
	}
} else

	lseek(mt, (SIZE_L) -TBLOCK * nblock, 1);

recno--;

}/*E backtape() */

int dideot = 0;
int didfirst = 0;
int didasync = 0;
/* buf NOT always used! */
bread(fd, buf, size)
	FILE_D	fd;
	char *buf;
	SIZE_I	size;
{
	SIZE_I	bytes;
	SIZE_I	count;
	SIZE_I	lastread = 0;
	register int i, j;
	struct devget mtsts;
	int sawtpmrk=0;

/***********/
next_volume:
/***********/
if (!Bflag) {
	extern int doingasync;
	extern int pending[MAXASYNC];

	/* If Aynchronous i/o buffer count > 0	*/
	/* AND ???				*/ 
	if(doingasync && first)
	{
		extern int curbuf;
	 	extern struct atblock *fblock[MAXASYNC];
		int read_done = 0;
		didfirst = 1;
/***********/
wait_til_read_done:
/***********/
	for(i=0, j=curbuf; i<doingasync && !read_done; i++)
	{
		if(pending[j] == 0)
		{
			bytes = read(fd, (char *)&fblock[j][0], size);
			pending[j] = bytes;
		}
		else
		{
			char *tmp;
			tmp = (char *)&fblock[j][0];
			curbuf = j;

			/* Check status of I/O completion */
			bytes = ioctl(fd, FIONBDONE, &tmp);
		
			/* If requested bytes <> actual read bytes    */
			/* OR read error and error equal EOT detected */
			/* OR software EOT detected in data stream    */
			if((pending[j] != bytes || bytes <= 0)) {
				int k=0;

				if(ioctl(mt, DEVIOCGET, &mtsts) < 0) {
					return(bytes);
				} else {
					if ((mtsts.category_stat & DEV_TPMARK) && blocks > 0 ) {
						sawtpmrk++;
					}
				}

				/* Clear the serious exception     */
				/* which occured from reading into */
				/* the EOT marker...               */
				mtops.mt_op = MTCSE;
				mtops.mt_count = 1;
				if(ioctl(fd, MTIOCTOP, &mtops) < 0) {
					fprintf(stderr, "%s: ", progname);
					perror("ioctl MTCSE failed on read");
				}

				if(ioctl(mt, DEVIOCGET, &mtsts) < 0)
					return(bytes);

				if ((mtsts.stat & DEV_EOM) || sawtpmrk)
				{
				/* Set did eot handling flag */
				dideot++;

				/* disable Asynchronous I/O */
				(void) ioctl(fd, FIONBUF, &k);
				didasync = doingasync;
				doingasync = 0;

				/* Clear the serious exception     */
				/* which occured from reading into */
				/* the EOT marker...               */
				mtops.mt_op = MTCSE;
				mtops.mt_count = 1;
				if(ioctl(fd, MTIOCTOP, &mtops) < 0) {
					fprintf(stderr, "%s: ", progname);
					perror("ioctl MTCSE failed on read");
				}
				
				goto read_past_eot;
				}
			}
			/* indicate buffer available */
			pending[curbuf] = 0;

			/* Indicate A Read Completed */
			read_done++;
		}
		/* If at max. # of buffers index, then  */
		/* roll back buffer pointer array index */
		if(++j >= doingasync)
			j = 0;
	}

	/* If the read is not done, then wait for completion */
	if(!read_done) 
		goto wait_til_read_done;

	} /* Bottom of if async. i/o & first */
	else
	{
		/* Normal Synchronous Read */
read_past_eot:
		bytes = read(fd, (char *)&fblock[curbuf][0], size);

		/* Check # of bytes read */
		if(bytes != size && dideot)
		{
			errno = ENOSPC;
			dideot = 0;
		}
		
		/* If in Asynchronous I/O mode... */
		if(doingasync)
		{
			char *tmp;
			tmp = (char *)&fblock[curbuf][0];
			bytes = ioctl(fd, FIONBDONE, &tmp);
			if (bytes <= 0) {
			    if(ioctl(mt, DEVIOCGET, &mtsts) < 0) {
				return(bytes);
			    } else {
				if ((mtsts.category_stat & DEV_TPMARK) && blocks > 0 ) {
					sawtpmrk++;
				}
			    }
			}
		}
	}

	/* Code common for both Synchronous and Asynchronous I/O */
	if (bytes <= 0)
	{
		/* If ioctl(DEVIOCGET) fails or still media space... */
		if ((ioctl(mt, DEVIOCGET, &mtsts) < 0) ||
			size_of_media[CARCH])
			return(bytes);
		else
		{
			/* If not at End of Media, then just */
			/* return the number of bytes read   */
			if (!(mtsts.stat & DEV_EOM) && !(sawtpmrk))
				if((recno == 0) &&
                                   (cflag == 0) &&
                                   (rflag == 1) &&
                                   (mtsts.category_stat & DEV_TPMARK))
					{
					fprintf(stderr,"\007%s: No Archive data found on tape.\n%s: Use the -c option to create tar tape\n", progname, progname);
					exit(3);
					}
				else
					return(bytes);
			else
			{
				/* If still blocks left in file */
				if (blocks >= 0)
					/* blocks < 0 implies that a
					 * file ended on an exact EOT
					 * boundry and is not continued
					 * to the next archive.
					 */
					FILE_CONTINUES++;
				EOTFLAG++;
				MULTI++;

				/* Go announce end of media & get user
				 * to mount the next tape archive.
				 */
				getdir();

				/* Now go read the requested amount of
				 * data from the newly mounted tape.
				 */
				errno = 0;
				goto next_volume;
			}
		}

	/* Normal read completion */
	}
	else
	{
		return(bytes);
	}
} /* End of if(!Bflag) */

for (count = 0; count < size; count += lastread)
{
	if (lastread < 0)
	{
		if (count > 0)
			return (count);

		return (lastread);
	}

	lastread = read(fd, buf, size - count);

	/* Zero byte read indicates EOF.
	 * Return that or the count of last good read.
	 * ie. If we never read anything, count will be 0.
	 */
	if (!lastread) 
		return(count);

	buf += lastread;

}/*E for count = 0 ..*/

return (count);

}/*E bread() */

/*.sbttl bsrch() */
daddr_t bsrch(s, n, l, h)
	daddr_t	l, h;
	char *s;
{
	char	b[N];
	int	i, j;
	daddr_t	m, m1;


njab = 0;

loop:
/*--:
 */
if (l >= h)
	return (-1L);

m = l + (h-l)/2 - N/2;
if (m < l)
	m = l;

fseek(tfile, m, 0);
fread(b, 1, N, tfile);
njab++;

for(i=0; i<N; i++) {
	if (b[i] == '\n')
		break;
	m++;
}
if (m >= h)
	return (-1L);
m1 = m;
j = i;

for(i++; i<N; i++) {
	m1++;
	if (b[i] == '\n')
		break;
}
i = cmp(b+j, s, n);
if (i < 0) {
	h = m;
	goto loop;
}
if (i > 0) {
	l = m1;
	goto loop;
}
return (m);

}/*E bsrch() */

/*.sbttl checkdir() */
checkdir(name)
	char *name;
{
	char *cp;
 	char *slash = "/";
	int	i;

/*
 * Quick check for existance of directory.
 *
 *	POSIX does not specify that a DIRTYPE has to be stored with a trailing
 *	slash on the filename, but our tar expects this.
 */
if ((is_posix) && (dblock.dbuf.typeflag == DIRTYPE) && (!rindex(name, slash)))
	strcat(name, slash);

if (!(cp = rindex(name, '/')))
	return (0);

*cp = '\0';
if (access(name, 0) >= 0) {
	*cp = '/';
	return (cp[1] == '\0');
}

*cp = '/';
/*
 * No luck, try to make all directories in path.
 */
for (cp = name; *cp; cp++) {
	if (*cp != '/')
		continue;

	*cp = '\0';
	if (access(name, 0) < 0) {
		if (mkdir(name, 0777) < 0) {
			perror(name);
			*cp = '/';
			return (FAIL);
		}
	if (VFLAG)
		fprintf(stderr,"x %s/ (directory created)\n", name);
	else
		if (vflag)
			fprintf(stderr,"x %s/\n", name);

	chown(name, stbuf.st_uid, stbuf.st_gid);

	if (pflag && cp[1] == '\0')
		chmod(name, stbuf.st_mode & 07777);

	}/*E if (access(name, 0) < 0) */

	*cp = '/';

}/*E for (cp = name; *cp; cp++) */

return (cp[-1]=='/');

}/*E checkdir() */

/*.sbttl CHECKF() */

#ifdef TAR40

/* Function:
 *
 *	CHECKF
 *
 * Function Description:
 *
 *   Routine to determine whether the specified file should
 *   be skipped or not. Implements the F, FF and FFF modifiers.
 *
 *   The set of files skipped depends upon the value of howmuch.
 *
 *   When howmuch > 0 the set of skip files consists of:
 *
 *		SCCS directories
 *		core files
 *		errs files
 *
 *    Howmuch > 1 increases the skip set to include:
 *
 *		a.out
 *		*.o
 *
 *    Howmuch > 2 causes executable files to be skipped
 *    as well. (A file is determined to be executable by
 *    looking at its "magic numbers")
 *
 *
 * Arguments:
 *
 *	char	*longname	Pointer to name string of file to
 *				to be checked
 *	int	mode		File mode bits of the file
 *	int	howmuch		Skip set modifier
 *
 * Return values:
 *
 *	1	The file is to be skipped
 *	0	The file is not to be skipped
 *
 * Side Effects:
 *
 *	The routine assumes that the file is in the current directory.
 *	The routine is not fully implemented as described above. See
 *	the #ifdef below.. 
 *	
 */
CHECKF(longname, mode, howmuch)
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
#ifdef TAR40
	if (STRFIND(longname,"SCCS/") == longname || STRFIND(longname,"/SCCS/"))
#else
	if (strfind(longname,"SCCS/") == longname || strfind(longname,"/SCCS/"))
#endif
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

#ifdef notdefFFF
/*
 * This routine works for -c and -r options, but is
 * not sufficent for -x option.  Since it cannot be implemented
 * fully, it is not implemented at all at this time.
 */
/*
 * Second level additions ?
 */
if (howmuch > 2) {
	/*
	 * Open the file to examine the magic numbers.
	 * If the file cannot be opened, then assume
	 * that it is not to be skipped and that another
	 * routine will perform the error handling for it.
	 * Otherwise, read from the file and check the
	 * magic numbers, indicate skipping if they are good.
	 */
	int	ifile, in;
	struct exec	hdr;

	ifile = open(shortname,O_RDONLY,0);

	if (ifile >= 0)	{
		in = read(ifile, &hdr, sizeof (struct exec));
		close(ifile);
		device_open = 0;
		if (in > 0 && ! N_BADMAG(hdr))	/* executable: skip */
			return (0);
	}
}/* end if howmuch > 2 */
#endif notdefFFF

return (1);	/* Default action is not to skip */

}/*E CHECKF() */
#endif
/*.sbttl CHECKSUM() */

#ifdef TAR40

/* Function:
 *
 *	CHECKSUM
 *
 * Function Description:
 *
 *	Perform first-level CHECKSUM for file header block.
 *
 * Arguments:
 *
 *	none
 *
 * Return values:
 *
 *	The CHECKSUM value is returned.
 *
 * Side Effects:
 *
 *	
 */
CHECKSUM()
{
	char *cp;
	int	i = 0;

for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
	*cp = ' ';

for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
	i += *cp;

return (i);

}/*E CHECKSUM() */
#endif

/*.sbttl checkw() */
checkw(c, name)
	char *name;
{

if (!wflag)
	return (1);

fprintf(stderr,"%c ", c);

if (vflag)
	longt(&stbuf);

fprintf(stderr,"%s: ", name);
	return (response() == 'y');

}/*E checkw() */

/*.sbttl cmp() */
cmp(b, s, n)
	char *b;
	char *s;
{

	int i;

if (b[0] != '\n')
	exit(2);

for (i = 0; i < n; i++) {
	if (b[i+1] > s[i])
		return (-1);

	if (b[i+1] < s[i])
		return (1);
}
return (b[i+1] == ' '? 0 : -1);

}/*E cmp() */

/*.sbttl dotable()  Top level logic for "t" function */
dotable()
{
	char *my_name = file_name;
	char *cp = file_name;	
for (;;) {
	extracted_size = 0L;
TNEXTA:
/**/
	getdir();

	if (endtape()) {
		if (FILE_CONTINUES)
			goto TNEXTA;

		if (VFLAG)
			printf("\n%s: %ld Blocks used on %s for archive  %d\n\n",
				progname, blocks_used, usefile, CARCH);
		break;
	}
	dostats();
	
	/* Check for F[FF] operation, skipping SCCS stuff, etc..
 	 */
	if(is_posix)  {
		strcpy(cp, dblock.dbuf.posix_prefix);
		if(*dblock.dbuf.name) 
			strcat(cp, dblock.dbuf.name);
	} else
		strcpy(cp, dblock.dbuf.name);

#ifdef TAR40
	if (Fflag && !CHECKF(my_name, stbuf.st_mode, Fflag)) {
#else
	if (Fflag && !checkf(my_name, stbuf.st_mode, Fflag)) {

#endif
		passtape();
		continue;
	}
	
	passtape();

}/*E for ;; */

}/*E dotable() */

dostats()
{	
	register char *cp = file_name;
	register char *my_name = file_name;
	
	/*
 	 * Save last file name  &  mod time for possible
 	 * archive switch compare.
 	 */
	if(is_posix) {
		strcpy(cp, dblock.dbuf.posix_prefix);
		if(dblock.dbuf.name[0]) 
			strcat(cp, dblock.dbuf.name);
	} else
		strcpy(cp, dblock.dbuf.name);
	modify_time = stbuf.st_mtime;

	if (dblock.dbuf.typeflag == DIRTYPE) {
		if (vflag)
			longt(&stbuf);

		printf("%s", my_name);
	}
	else {
		if (vflag)
			longt(&stbuf);

		printf("%s", my_name);

		if (!vflag && OARCH)
			printf("  <- continued from archive  %d,  Current archive is  %d, Original archive is  %d",
				(CARCH-1), CARCH, OARCH);
	}
	if (dblock.dbuf.typeflag == LNKTYPE)
		printf(" linked to %s", dblock.dbuf.linkname);

	if (dblock.dbuf.typeflag == SYMTYPE)
		printf(" symbolic link to %s", dblock.dbuf.linkname);

	if (VFLAG && dblock.dbuf.typeflag == CHRTYPE)
		printf(" (character special)");

	if (VFLAG && dblock.dbuf.typeflag == BLKTYPE)
		printf(" (block special)");
	
	if (VFLAG && dblock.dbuf.typeflag == FIFOTYPE)
		printf(" (named pipe)");
	
	printf("\n");
}/*E dostats() */

/*.sbttl doxtract() */
/* Function:
 *
 *	doxtract
 *
 * Function Description:
 *
 *	eXtracts (reads) files from an input archive and places
 *	them into disk files.
 *
 */
doxtract(argv)
	char *argv[];
{

	char	**cp;
	char	buf[TBLOCK];
	FILE_D	ofile;
	register char	*my_name = file_name;
	char 	*tp = file_name;
	FILE	*fp;
	char	s[NAMSIZ+1];
	
    for (;;) {
	extracted_size = 0L;
	getdir();
	if (endtape()) {

		if (VFLAG)
			fprintf(stderr,
		      	  "\n%s: %ld Blocks used on %s for archive  %d\n\n",
			  progname, blocks_used, usefile, CARCH);
			break;
	}
NEW_FILE:
/**/
        /* Re-align pointer my_name in case it got hosed during
           archive switch.
         */
        my_name = file_name;

	/* Save last file name  &  modify time for possbile
	 * archive switch ck.
	 */
	if(is_posix) {
		strcpy(tp, dblock.dbuf.posix_prefix);
		if(dblock.dbuf.name[0]) 
			strcat(tp, dblock.dbuf.name);
	} else
		strcpy(tp, dblock.dbuf.name);
	modify_time = stbuf.st_mtime;
	
	if (!*argv)
		goto gotit;

	for (cp = argv; *cp; cp++) {
		if (Rflag) {
			if ((fp = fopen(*cp, "r")) > 0) {
				while (fgets(s, NAMSIZ+1, fp)) {
					if (s[strlen(s)-1] == '\n')
                                		s[strlen(s)-1] = '\0';
                                	if (prefix(s, my_name)) {
						fclose(fp);
						goto gotit;
					}
				}
				fclose(fp);
			}
		} else {
			if (prefix(*cp, my_name))
				goto gotit;
	    	}
	}		/* end - for (cp = argv; *cp; cp++) */

	passtape();
	continue;

gotit:
/*---:
 */
	/* (w) Pass user conf.? No: skip file.
	 */
	if (wflag) {
		if (!checkw('x', my_name)) {
			passtape();
			continue;
		}
	}

	/* (F[FF]) Fast? Check to see if the file is in the skip set.
	 */
	if (Fflag) {
#ifdef TAR40
		if (!CHECKF(my_name, stbuf.st_mode, Fflag))
#else
		if (!checkf(my_name, stbuf.st_mode, Fflag))
#endif
		{
			passtape();
			continue;
		}
	}		/* end - if (Fflag) */
	if (checkdir(my_name))
		continue;

	/* See if this is a symbolic link.
	 */
	if (dblock.dbuf.typeflag == SYMTYPE) {

		/* Check to see if symbolic link */
                /* would be restored on top of a */
		/* directory. If it would,  then */
		/* dis-allow it's restoration... */
	    {
		int    i;
		struct stat statbuf;

	        i = lstat(my_name, &statbuf);
		  if((i >= 0) &&
		  (statbuf.st_mode & S_IFMT) == S_IFDIR){
			fprintf(stderr,
			  "%s: Cannot restore symbolic link named \"%s\"\n",
			  progname, my_name);
			fprintf(stderr,
			  "as a directory with the same name exists.\n");
			fprintf(stderr,
			  "Remove the directory first to restore symbolic");
			fprintf(stderr, " link in its place.\n");
			continue;
		}
	
	    }
		/*
		 * Fix for IPR-00014. Prevent tar from
		 * unlinking non-empty directories.
		 *
		 * Only unlink non-directories or
		 * empty directories.
		 */
		if (rmdir(my_name) < 0) {
			if (errno == ENOTDIR)
				unlink(my_name);
			else
				if (errno == ENOTEMPTY)
					perror(progname);
		}

		/* Make the symbolic link */
		if (symlink(dblock.dbuf.linkname, my_name)<0) {
			fprintf(stderr, "%s: Symbolic link failed: %s\n",
				progname, my_name);

			perror(my_name);
			continue;
		}
		if (vflag)
			fprintf(stderr,"x %s symbolic link to %s\n",
			    my_name, dblock.dbuf.linkname);

		chown(my_name, stbuf.st_uid, stbuf.st_gid);

		continue;

	}/*E if (dblock.dbuf.typeflag == SYMTYPE) */

	/* See if this is a hard link.
	 */
	if (dblock.dbuf.typeflag == LNKTYPE) {

		/* Fix for IPR-00014. Prevent tar from
		 * unlinking non-empty directories.
		 *
		 * Only unlink non-directories, or
		 * empty directories.
		 */
		if (rmdir(my_name) < 0) {

			if (errno == ENOTDIR)
				unlink(my_name);
			else
				if (errno == ENOTEMPTY)
					perror(progname);
		}
		if (link(dblock.dbuf.linkname, my_name) < 0) {
			fprintf(stderr, "%s: Can't link: %s\n",
			    progname, my_name);
			perror(my_name);
			continue;
		}
		if (vflag)
			fprintf(stderr,"%s linked to %s\n",
			    my_name, dblock.dbuf.linkname);
		chown(dblock.dbuf.linkname, stbuf.st_uid, stbuf.st_gid);

		continue;

	}/*E if dblock.dbuf.typeflag == LNKTYPE */

	/*
	 * Are we extracting a special file ?
	 */
	if (((stbuf.st_mode & S_IFMT) == S_IFCHR) ||
	      ((stbuf.st_mode & S_IFMT) == S_IFBLK)||
		((stbuf.st_mode & S_IFMT) == S_IFIFO)) {	

		if ((ofile = mknod(my_name, stbuf.st_mode,
		    stbuf.st_rdev)) < 0 ) {

			fprintf(stderr,"%s: Can't create special file: %s\n",
			    progname, my_name);
			perror(my_name);
			passtape();
			continue;
		}
		if (vflag)
			if((stbuf.st_mode & S_IFMT) == S_IFIFO)
				fprintf(stderr,"x %s, (named pipe)\n", my_name);
			else
				fprintf(stderr,"x %s, (special file)\n",
				    my_name);
		if (!mflag) {
			struct timeval tv[2];

			tv[0].tv_sec = time(0);
			tv[0].tv_usec = 0;
			tv[1].tv_sec = stbuf.st_mtime;
			tv[1].tv_usec = 0;
			utimes(my_name, tv);
		}

		if (pflag) {
			chown(my_name, stbuf.st_uid, stbuf.st_gid);
			chmod(my_name, stbuf.st_mode);
		}
		continue;	

	}/*E if st->st_mode etc..*/

	/*
	 * Assume a regular type of file.
	 */
	if ((ofile = creat(my_name,stbuf.st_mode&0xfff)) < 0) {
		fprintf(stderr, "%s: Can't create: %s\n", progname, my_name);
		perror(my_name);
		passtape();
		continue;
	}

NEXT_CHUNK:
/**/
	bytes = stbuf.st_size;
	blocks = (bytes + (SIZE_L)TBLOCK - 1L) / (SIZE_L)TBLOCK;

	if (vflag) {
		if (OARCH  && (original_size > chctrs_in_this_chunk)) 
			fprintf(stderr,"x %s, %ld bytes of %ld, %d blocks\n",
				my_name, chctrs_in_this_chunk,
				original_size, blocks);
		else
			fprintf(stderr,"x %s, %ld bytes, %d blocks\n",
				my_name, bytes, blocks);

	}

	for (; blocks > 0;) {
		int cnt, wcnt;
		extern int curbuf;
		extern struct atblock *fblock[MAXASYNC];

		cnt = nblock - recno;
		if(cnt <= 0)
			cnt = nblock;
		if(cnt > blocks)
			cnt = blocks;

		wcnt = readtape((char *)&fblock[curbuf][recno], cnt, 0);

		if (write(ofile, &fblock[curbuf][recno - wcnt],
			  bytes > wcnt*TBLOCK ? wcnt*TBLOCK : bytes) < 0) {
			fprintf(stderr, "%s: Write error on extract: %s\n",
				progname, my_name);

			perror(my_name);
			done(FAIL);
		}
		bytes -= bytes > wcnt*TBLOCK ? wcnt*TBLOCK : bytes;
		blocks -= wcnt;
	}/*E for ; blocks-- > 0 ..*/
	
	/* Have we extracted all of the file ?
	 * Non UMA files can't be continued, by definition.
	 */
	if ((hdrtype != UMA) || (extracted_size == original_size)) {
		close(ofile);
	}
	else {
		/* Come here when we need to process another non-tape
		 * archive to get more of the file. For tapes, the
		 * archive switching is transparant to this logic.
		 * It is delt with by the readtape(), bread(), and
		 * getdir() routines.
		 */
		char last_file[NAMSIZ+1];
		char *cp = file_name;
		
		my_name = last_file;
		if(is_posix) {
			strcpy(cp, dblock.dbuf.posix_prefix);
			if(dblock.dbuf.name[0]) 
				strcat(cp, dblock.dbuf.name);
		} else
			strcpy(cp, dblock.dbuf.name);
		modify_time = stbuf.st_mtime;
		getdir();
		if (endtape()) {

			/* If this is the last archive, we will never 
			 * see the rest of the file. Give up.
			 */
			if (!FILE_CONTINUES) {
				fprintf(stderr,"\n%s: File  %s  not correctly extracted \n\n",
				  progname, last_file);

				close(ofile);

				if (VFLAG)
					fprintf(stderr,"\n%s: %ld Blocks used on %s for archive  %d\n\n",
						progname, blocks_used,
						usefile, CARCH);
					done(FAIL);
			}
			else {
				/*
		 	 	* Else, next archive should be inserted.
		 	 	*/
				getdir();
				goto NEXT_CHUNK;
			}
		}
		else {
			fprintf(stderr,
			  "\n%s: File  %s  not correctly extracted \n\n",
			  progname, last_file);
			close(ofile);
			goto NEW_FILE;
		}
	}
	chown(my_name, stbuf.st_uid, stbuf.st_gid);

	if (!mflag) {
		struct timeval tv[2];

		tv[0].tv_sec = time(0);
		tv[0].tv_usec = 0;
		tv[1].tv_sec = stbuf.st_mtime;
		tv[1].tv_usec = 0;
		utimes(my_name, tv);
	}

	if (pflag) {
	    if ((is_posix) && (geteuid() != 0)) {
		; /* POSIX requires that when tar is run by a non-privileged
		     process that the umask of the process should be used 
		     instead of the mode of the file.  */
	    }
	    else chmod(my_name, stbuf.st_mode & 07777);
	}

    }/*E for ;; */

}/*E doxtract()*/

/*.sbttl endtape()  Determine if end of archive reached */
endtape()
{
	extern int pending[MAXASYNC];
	register char *my_name;
	char *malloc();
	
	if((my_name = malloc(NAMSIZ)) == NULL) 
		return(!TRUE);
	if(is_posix) {
		strcpy(my_name, dblock.dbuf.posix_prefix);
		if(dblock.dbuf.name[0]) 
			strcat(my_name, dblock.dbuf.name);
	} else
		strcpy(my_name, dblock.dbuf.name);
    EODFLAG = FILE_CONTINUES = 0;

/* If this is an empty block, read one more block so we don't break
 * our pipe by closing down before reading ALL the blocks ! 
 * From a tape, it doesn't matter, but thru a pipe...
 */ 

    if (pipein) {
	if (*my_name == '\0') {
		int cnt=1;
	/* Make sure the %!#% pipe is empty */
		while(cnt > 0)
			cnt = read(mt, (char *)&dblock, TBLOCK);
		free(my_name);
		return(TRUE);
	}		/* end - if (*my_name == '\0') */
    }			/* end - if (pipein) */

/* For multi_archive operations, normally read thru the 2 zero-filled
 * blocks and return status from next one. This block will tell
 * us if the file is continued to another archive - but only if
 * the last file on the archive was written by this version of tar.
 * For appends and updates, we need to NOT read thru the zero-blocks
 * in order to maintain correct physical postion.
 */

    if (!is_posix && OARCH && rflag && !*my_name && size_of_media[CARCH]) {
	/*
	 * The current file either is the last one on the media
	 * or the first and is continued from a previous archive.
	 */

	/* The file must be the last one and is continued
	 * to next archive.
	 */
	if (blocks_used == (size_of_media[CARCH] - (2L))) {
		blocks_used += 2L;
		EODFLAG = FILE_CONTINUES = TRUE;
		free(my_name);
		return(TRUE);
	}
	else {
		/* There is more room on the archive.
		 */
	    if(doingasync && !EOTFLAG) {
		int i, rcnt=1,j;
		for(i=0; i<doingasync; i++) {
			char *tmp;
			int bytes;
			extern struct atblock *fblock[MAXASYNC];
			tmp = (char *)&fblock[i][0];
			if((bytes = ioctl(mt, FIONBDONE, &tmp)) > 0 )
				if(bytes == pending[i])
					rcnt++;
		}
		if(rcnt && didfirst) {
			/* Clear Serious Ex. */
			mtops.mt_op = MTCSE;
			mtops.mt_count = 1;
			if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
				fprintf(stderr, "%s: ", progname);
				perror("ioctl MTCSE failed on read");
			}
			/* Backspace */
			mtops.mt_op = MTBSF;
			mtops.mt_count = 1;
			if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
				fprintf(stderr, "%s: ", progname);
				perror("ioctl MTBSF failed - 1");
			}
		}
	    }		/* end - if(doingasync && !EOTFLAG) */
		blocks_used -= 1;
		free(my_name);
		return(TRUE);
	}	/* end if/else - (blocks_used==(size_of_media[CARCH]-(2L))) */
    }		/* end - if (!is_posix && OARCH && rflag &&
		 *	!*my_name && size_of_media[CARCH])
		 */

/* Deal with an append to an archive not beginning or ending
 * with a continued file.
 */

  if (!*my_name && rflag) {
    if((*my_name == '\0') && doingasync && !EOTFLAG) {
	int i, j;
	int rcnt=1, xtra_backspace = 0;
		/*
		 * xtra_backspace is used to fix the Boeing CLD 03445.
		 *
		 * Symptom:  an append (tar r) would fail intermittently.
		 * The tape would spin, and tar would return success, but
		 * the added file(s) would not appear on the archive (tar t).
		 * In fact, the file(s) would be written to tape, but in
		 * the wrong place -- after the end-of-archive marker.
		 *
		 * Problem:  tar detects the End-of-Archive (EOA),
		 * but it spans two tape records:  one null block (zeros)
		 * in one record, and one in the next.  N-buffered
		 * I/O has physically read the last data record (and the
		 * EOT marks), but tar has only logically read the
		 * next-to-last.  Tar correctly handles the EOT.
		 *
		 * Solution:  The fix is to detect the "over-read" situation
		 *             if (bytes == pending[i])
		 * and do an "extra" backspace record using the backtape()
		 * routine.  However, backtape() also adjusts the logical
		 * block pointer (recno) by decrementing it, so a "recno++"
		 * is added to undo that adjustment -- all that is needed is
		 * the physical tape-backspace;  tar ignores the second null
		 * block of the EOA anyway.
		 *
		 * Note:  backtape() does a MTBSR, not a MTBSF.
		 *
		 * Note:  there is still a latent bug where an EOA mark
		 * spans two tape spools.  It just isnt worth the trouble
		 * to fix because (1) it is very, very rare, and (2) the
		 * hassle of switching tapes and waiting for rewinds and
		 * read-forwards would be incredible.
		 */

	for(i=0; i<doingasync; i++) {
		char *tmp;
		int bytes;
		extern struct atblock *fblock[MAXASYNC];
		tmp = (char *)&fblock[i][0];
		if((bytes = ioctl(mt, FIONBDONE, &tmp)) > 0 )
			if(bytes == pending[i]) {
				rcnt++;
				if (pending[i] > 0)		/* CLD */
					xtra_backspace = 1;	/* CLD */
					/* must be latching */
			}	/* end - if(bytes == pending[i]) */
	}			/* end - for(i=0; i<doingasync; i++) */
		
	if(rcnt && didfirst) {
		mtops.mt_op = MTCSE;
		mtops.mt_count = 1;
		if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
			fprintf(stderr, "%s: ", progname);
			perror("ioctl MTCSE failed on read");
		}
		mtops.mt_op = MTBSF;
		mtops.mt_count = 1;
		if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
			fprintf(stderr, "%s: ", progname);
			perror("ioctl MTBSF failed - 2");
		}
	}
	if (xtra_backspace == 1) {				/* CLD */
		backtape();					/* CLD */
		recno++;	/* fake out */			/* CLD */
	}							/* CLD */

	/* Disable n-buf */
	rcnt = 0;
	ioctl(mt, FIONBUF, &rcnt);

	/* Reenable */
	ioctl(mt, FIONBUF, &doingasync);
    }		/* end - if((*my_name == '\0') && doingasync && !EOTFLAG) */
    blocks_used--;
    free(my_name);
    return(TRUE);
  }		/* end - if (!*my_name && rflag) */

/* Now, if NOT dealing with an update or rappend  AND
 * the last file was an Ultrix Multi Archive format.
 */
  if (!is_posix && (*my_name == '\0') && (hdrtype == UMA)) {

	EODFLAG = TRUE;
	readtape((char *)&dblock, 1, 1);

/*
	if(size_of_media[0])
*/
		readtape((char *)&dblock, 1, 1);

	if (dblock.dbuf.name[0]) {

		FILE_CONTINUES++;

		strcpy(file_name,dblock.dbuf.name);
		dblock.dbuf.name[0] = '\0';
	}
    if((dblock.dbuf.name[0] == '\0') && doingasync && !EOTFLAG) {
	int i, rcnt=1,j;
	for(i=0; i<doingasync; i++) {
		char *tmp;
		int bytes;
		extern struct atblock *fblock[MAXASYNC];
		tmp = (char *)&fblock[i][0];
		if((bytes = ioctl(mt, FIONBDONE, &tmp)) > 0 )
			if(bytes == pending[i]) {
				rcnt++;
			}
	}			/* end for (i) */
		
	if(rcnt && didfirst) {
		mtops.mt_op = MTCSE;
		mtops.mt_count = 1;
		if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
			fprintf(stderr, "%s: ", progname);
			perror("ioctl MTCSE failed on read");
		}
		mtops.mt_op = MTBSF;
		mtops.mt_count = 1;
		if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
			fprintf(stderr, "%s: ", progname);
			perror("ioctl MTBSF failed - 3");
		}
	}		/* end - if(rcnt && didfirst) */
    }			/* end - if((dblock.dbuf.name[0] == '\0') &&
			 * doingasync && !EOTFLAG) */
	free(my_name);
	return (*my_name == '\0');
  }		/* end - if (!is_posix&&(*my_name=='\0')&&(hdrtype == UMA)) */

	/* Adjust the block count for archives not ending with a file
	 * written in "UMA" format.
 	 */
  if((*my_name == '\0') && doingasync && !EOTFLAG) {
	int i, rcnt=1,j;
	for(i=0; i<doingasync; i++) {
		char *tmp;
		int bytes;
		extern struct atblock *fblock[MAXASYNC];
		tmp = (char *)&fblock[i][0];
		if((bytes = ioctl(mt, FIONBDONE, &tmp)) > 0 )
			if(bytes == pending[i]) {
				rcnt++;
			}
	}
		
	if(rcnt && didfirst) {
		mtops.mt_op = MTCSE;
		mtops.mt_count = 1;
		if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
			fprintf(stderr, "%s: ", progname);
			perror("ioctl MTCSE failed on read");
		}
		mtops.mt_op = MTBSF;
		mtops.mt_count = 1;
		if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
			fprintf(stderr, "%s: ", progname);
			perror("ioctl MTBSF failed - 4");
		}
	}
    free(my_name);
    return(TRUE);
  }

  if (*my_name == '\0') {
	if (!rflag)
		blocks_used++;
	else
		blocks_used--;
  }
  free(my_name);	/* using my_name after freeing is bad,
			 * stack vars are worse */
  return (*my_name =='\0');

}/*E endtape() */

/*.sbttl getdir() */

getdir()
{

	FLAG	retrying = 0;
	int	i;
	int	majordev, minordev;
	int	openm;
	struct	passwd	*pwp;
	FLAG	reload;
	struct	stat	*sp;
	SIZE_I	tcarch;
	SIZE_I  toarch;
	extern int lastuid, lastgid;


sp = &stbuf;
	
/*
 * Determine how much space is left on
 * this archive and assume one block as a
 * minimum for any output.
 */
if(size_of_media[0]){
	SIZE_L available_blocks;

	available_blocks = size_of_media[CARCH] - (nblock + blocks_used +3L);
	if(available_blocks < 0L && !FILE_CONTINUES){
		goto EOTD;
	}
}

/* If the condition  EOTFLAG && !VOLCHK  is true,
 * bread() has detected a real tape EOT marker & is calling us
 * to announce the end of media & to request the user to mount
 * the next archive.
 */
if ((EOTFLAG && !VOLCHK) || (FILE_CONTINUES && EODFLAG))
	goto EOTD;

/* The following test implies that  readtape()  has been told of a
 * real tape EOT by bread() & is calling us to verify that the correct
 * (next) archive was mounted.
 */
if (EOTFLAG && VOLCHK)
	goto CHKVOL;

top:
NEXTVOL:
/**/

readtape((char *)&dblock, 1, 1);

if (!dblock.dbuf.name[0] && !dblock.dbuf.magic[0]) {
	
	if (EODFLAG || EOTFLAG)
		goto CHKVOL;
	else
		return;
}

if (FILE_CONTINUES) {

EOTD:
/**/
	nextvol = CARCH+1;

	if (VFLAG) {
		if (MULTI)
			fprintf(stderr,"\n%s: End of archive media\n",progname);
		else
			fprintf(stderr,"\n");

		fprintf(stderr,"%s: %ld Blocks used on %s for archive  %d\n",
			progname, blocks_used, usefile, CARCH);
	}
	if (FILE_CONTINUES)
		fprintf(stderr,"\n%s: File  %s  is continued on archive  %d",
	 	progname, file_name, nextvol);

	FILE_CONTINUES = 0;

RELOAD:
/**/
	close(mt);
	device_open = 0;
	blocks_used = 0L;
	recno = 0;
	first = 0;
	if (retrying)
		fprintf(stderr,"\n%s: Please load correct archive on  %s  & press RETURN ",
			progname, usefile);
	else
		fprintf(stderr,"\n\007%s: Please load archive  %d  on  %s  & press RETURN ",
			progname, nextvol, usefile);

	response();
	fprintf(stderr,"\n");

	if (tflag || xflag)
		openm = O_RDONLY;
	else
		openm = O_RDWR;
OPENT:
/**/
	if ((mt = statchk(usefile, openm)) < 0) {
		if(errno){
			fprintf(stderr, "%s: Can't open: %s\n", progname, usefile);
			perror(usefile);
			fprintf(stderr,"\n");

			/* Don't retry archive files that don't exist. */
			if (errno == ENOENT) done(FAIL);
		}
		fprintf(stderr,"%s: Please press  RETURN  to retry ",progname);
		response();
		goto OPENT;
	}

	if(didasync)
		doingasync = didasync;
		
	if(doingasync) {
		int cnt = doingasync;
		if(ioctl(mt, FIONBUF, &cnt) < 0)
			doingasync = 0;
		else
			doingasync = cnt;
		for(i=0; i<doingasync; i++)
			pending[i]=0;
	}
	MULTI = 0;
	if (!EOTFLAG)
		goto NEXTVOL;
	else
		return; /*<- go back to bread() or readtape() */
}
sscanf(dblock.dbuf.mode, "%o", &i);
sp->st_mode = i;
sscanf(dblock.dbuf.uid, "%o", &i);
sp->st_uid = i;
sscanf(dblock.dbuf.gid, "%o", &i);
sp->st_gid = i;
/*
 * If this is an older version of Ultrix tar archive, this
 * will extract (if any) the major/minor device number of
 * a "special" file.
 */
sscanf(dblock.dbuf.magic,"%o", &i);
sp->st_rdev = i;
sscanf(dblock.dbuf.size, "%lo", &sp->st_size);
sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
sscanf(dblock.dbuf.chksum, "%o", &chksum);

/*
 * Determine  true  tar  header format type  and 
 * announce result if requested.
 */
OARCH = 0;
volann++;
/*
 * Default to Original Tar Archive
 */
hdrtype = OTA;

if (!dblock.dbuf.magic[0]) {
	if (VFLAG && !volann)
	    printf("%s: Original tar archive format \n\n", progname);
}
else {	/* Further determination of tar header type.
	 * There is something in the former  rdev[6] field.
	 */
	hdrtype = OUA;	/* Set type to older Ultrix format  -
			 * ie. dblock.dbuf.magic MAY contain
			 * major/minor device numbers.
			 */ 
	if((strcmp(dblock.dbuf.posix_magic, TMAGIC) == 0) &&
	    (strncmp(dblock.dbuf.posix_version, TVERSION, TVERSLEN) == 0))
		is_posix = 1;
	else if (strcmp(dblock.dbuf.magic,OTMAGIC) == 0) {

		hdrtype = UGS;	/* Set type to plain User Group
				 * standard format.
				 * Do final check of format.
				 */
		if (dblock.dbuf.carch[0]) {

			/* Is User Group standard, PLUS -> Ultrix
			 * multi-archive extensions.
			 */
			hdrtype = UMA;
			/*
			 * Extract mod time & archive #'s.
			 */
CHKVOL:
/**/
			sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
			sscanf(dblock.dbuf.carch, "%d", &tcarch);
			sscanf(dblock.dbuf.oarch, "%d", &toarch);

			if (tcarch >= CARCH) {
				CARCH = tcarch;
				OARCH = toarch;
			}

			if (EODFLAG || VOLCHK) {
				reload = 0;


				if (CARCH != nextvol) {
					reload++;
					fprintf(stderr,"%s: Incorrect archive (%d) loaded on %s\n",
						progname,CARCH,usefile);

					fprintf(stderr,"%s: Archive  %d  expected\n",
						progname, nextvol);
				}

			    if (blocks >= 0) {
				/*
				 * If blocks is less than 0, an EOT
				 * and end of file occured at the
				 * same time. In this case, the file
				 * is not continued to the next archive
				 * and therefore the following checks
				 * do not apply. A new and different
				 * file will have been started.
				 */
				if (strcmp(file_name,dblock.dbuf.name)){
					if (!reload) {
						fprintf(stderr,"%s: Archive  %d  does not begin with correct file.\n",
							progname,CARCH);

					fprintf(stderr,"%s: Continued file name:  %s\n",
						progname, file_name);

					fprintf(stderr,"%s: File name on current archive  %s\n",
					     progname,dblock.dbuf.name);

					reload++;
					}
				}
				if (!reload && (sp->st_mtime != modify_time)) {

					fprintf(stderr,"\n%s: Continued file:  %s\n%s: modification time does not match previous archive.\n",
					   progname,file_name,progname);

					reload++;
				}
			    }/*E if blocks > 0 */

				if (reload) {
					retrying = reload;
					reload = VOLCHK = 0;
					goto RELOAD;
				}
			}
			sscanf(dblock.dbuf.size, "%lo", &sp->st_size);

			/* dblock.dbuf.size  contains the size of the
			 * file, or a file chunk.
			 * For "real" tapes, the portion of a file 
			 * written when we encounter an  EOT  has the
			 * true file size as it is not possible to
			 * know in advance when we'll run out of tape.
			 * For disks,  dblock.dbuf.size  contains the
			 * the amount of data we could put on the
			 * current archive. This would be either at the
			 * end of an archive, or at the beginning of
			 * a continuation archive. This is possible
			 * because we know the media size in advance
			 * and can calculate the amount of data we
			 * can place on it without running out of
			 * available space.
			 */

			chctrs_in_this_chunk = sp->st_size;

			/* The next test is seeking to determine if
			 * this is a "continuation" header. Either
			 * continued to next archive (disks only) or
			 * continued from a previous archive.
			 * Real tapes have only continued "from" headers
			 * and the  EOT  detection is used in lieu of
			 * a "continued to next archive" header.
			 */
			if (OARCH) {
				sscanf(dblock.dbuf.org_size, "%lo", &original_size);

				/* Don't add up the size when switching
				 * real tape archives. The initial
				 * archive contained the size info
				 * required to extract the file.
				 * For non-tapes, the various portion
				 * sizes of the file are contained in
				 * each header. For tapes, you cannot
				 * know when you are going to expire
				 * the media - ie. run out of tape.
				 */
				if (!EOTFLAG)
					extracted_size += sp->st_size;
			}
			else  {
				/* When processing a "real" tape, the
				 * header of a continued file on the
				 * ORIGINAL archive will contain the
				 * true size of the file. It is this
				 * amount in blocks that the logic
				 * will attempt to read.
				 */
				extracted_size = original_size = sp->st_size;
			}
			if (VOLCHK && EOTFLAG) {
				/*
				 * Return to  readtape()  indicating
				 * that the correct continuation
				 * archive was verified as having
				 * been mounted on the input device.
				 */
				return;
			}
			if (VFLAG && !volann) {
				printf("%s: Ultrix multi-archive tar format\n", progname);
				printf("%s: Current archive  %d\n", progname, CARCH);
				if (OARCH && (OARCH != CARCH))
					printf("%s: Original archive  %d\n\n", progname, OARCH);
				else
					printf("\n");
			}
		}/*T if (dblock.dbuf.carch[0]) */
		else {
			/* Archive is User Group Standard 
			 * format.
			 */
			if (VFLAG && !volann)
				printf("%s: User Group Standard tar archive format\n\n", progname);

		}/*F if dblock.dbuf.carch[0] */
	}/*T if (strcmp(dblock.dbuf.magic,OTMAGIC)==0) */

	else {
		/* Archive is older Ultrix version ..
		 * ie. May have a device major/minor number
		 * the  dblock.dbuf.magic  (formally rdev[6]).
		 */
		if (VFLAG && !volann)
			printf("%s: Older Ultrix tar archive format\n\n", progname);

	}/*F if strcmp(dblock.dbuf.magic, OTMAGIC)==0 */
}

/* If this file is in User Group format, with or without Ultrix
 * extensions, convert fields to the internal format used by the
 * original  tar  code for ease of implementation.
 */
if ((hdrtype >= UGS) || (is_posix)) {

	/* Put possible major/minor device numbers where tar
	 * usually expects to find them.
	 */
	sscanf(dblock.dbuf.devmajor, "%o", &majordev);
	sscanf(dblock.dbuf.devminor, "%o", &minordev);
	sp->st_rdev = ((majordev << 8) | minordev);

	/* Scan /etc/passwd and /etc/group to get user id
	 * and group id numbers.
	 */
	if (!NFLAG) {
		if(lastuid != sp->st_uid) {
			if (pwp=getpwnam(dblock.dbuf.uname))
				sp->st_uid = pwp->pw_uid;
			lastuid = sp->st_uid;
		}

		if(lastgid != sp->st_gid) {
			if (gp=getgrnam(dblock.dbuf.gname))
				sp->st_gid = gp->gr_gid;
			lastgid = sp->st_gid;
		}
	}
	/*
	 * Take further special action regarding "file types"
	 * as defined by the User Group standard.
	 */
	switch(dblock.dbuf.typeflag) {

		/* A regular file ?
		 */
		case OAREGTYPE:
		case AREGTYPE:
		case REGTYPE:
			sp->st_mode |= S_IFREG;
			break;

		/* Symbolic link ?
		 *
		case SYMTYPE:
			sp->st_mode |= S_IFLNK;
			break;

		/* Character special ?
		 */
		case CHRTYPE:
			sp->st_mode |= S_IFCHR;
			break;

		/* Block special ?
		 */
		case BLKTYPE:	
			sp->st_mode |= S_IFBLK;		
			break;

		/* Directory ?
		 */
		case DIRTYPE:
			sp->st_mode |= S_IFDIR;
			break;

		/* FIFO special
		 *  and/or  Contiguous ?
		 *
 		 * (for want of anything better at the moment,
		 *  classify these as -regular- files.)
		 */
		case FIFOTYPE:
			sp->st_mode |= S_IFIFO;
			break;

		case CONTTYPE:
		default:
			sp->st_mode |= S_IFREG;
			break;

	}/*E switch(typeflag) */

}/* if hdrtype >= UGS */


EODFLAG = 0;

#ifdef TAR40
if (chksum != (i = CHECKSUM())) {
#else
if (chksum != (i = checksum())) {
#endif

	fprintf(stderr, "\n\n\007%s: Directory checksum error, possible file name:\n", progname);

	if(is_posix) {
		for(i = 0; i < TPRFXLEN; i++)
			fprintf(stderr, "%c", dblock.dbuf.posix_prefix[i]);
	}
	for (i=0; i < NAMSIZ; i++) 
		fprintf(stderr,"%c",dblock.dbuf.name[i]);

	fprintf(stderr,"\n");

	if (iflag)
		goto top;

	done(FAIL);
}
if (tfile)
	fprintf(tfile, "%-s %-12.12s\n", dblock.dbuf.name, dblock.dbuf.mtime);

}/*E getdir() */

/*.sbttl longt() */
longt(st)
	struct stat	*st;
{

	char *cp;
	char *ctime();
	SIZE_I	tblocks;
	int	majordev, minordev;


pmode(st);

if (((st->st_mode & S_IFMT) == S_IFCHR) ||
      ((st->st_mode & S_IFMT) == S_IFBLK)||
      ((st->st_mode & S_IFMT) == S_IFIFO)) {

	majordev = st->st_rdev >> 8;
	minordev = st->st_rdev & 0377;
	if (OFLAG && vflag) {
		if (hdrtype >= UGS) 
			printf(" %s/%s\t",dblock.dbuf.uname,dblock.dbuf.gname);
	}

	printf(" %3d/%d    %3d,%3d ", st->st_uid, st->st_gid, majordev,minordev);
}
else {
	if (OFLAG && vflag) {
		if (hdrtype >= UGS) 
			printf(" %s/%s  ",dblock.dbuf.uname,dblock.dbuf.gname);
	}
	printf(" %3d/%d ", st->st_uid, st->st_gid);

	if (OARCH && (original_size > chctrs_in_this_chunk)) {
		printf(" (%ld bytes of %ld)", chctrs_in_this_chunk, original_size);
	}
	else
		printf("%7D", st->st_size);

	if (!OARCH)
		tblocks = (st->st_size + (SIZE_L)(TBLOCK-1L)) / (SIZE_L)TBLOCK;
	else
		tblocks = (chctrs_in_this_chunk + (SIZE_L)(TBLOCK-1L)) / (SIZE_L)TBLOCK;

	if (VFLAG)
		printf("/%03d ", tblocks);
}
cp = ctime(&st->st_mtime);
printf(" %-12.12s %-4.4s ", cp+4, cp+20);

}/*E longt() */

/*.sbttl lookup() */
daddr_t lookup(s)
	char *s;
{

	daddr_t	a;
	INDEX	i;


for (i=0; s[i]; i++)
	if (s[i] == ' ')
		break;

a = bsrch(s, i, low, high);
	return (a);

}/*E lookup() */

/*.sbttl passtape() */
passtape()
{

	SIZE_L	pblocks;
	char	buf[TBLOCK];


if (dblock.dbuf.typeflag) {
	if ((dblock.dbuf.typeflag != REGTYPE) &&
    	    (dblock.dbuf.typeflag != AREGTYPE) &&
	    (dblock.dbuf.typeflag != OAREGTYPE))

		return;
}
pblocks = stbuf.st_size;
pblocks += (SIZE_L)TBLOCK-1L;
pblocks /= (SIZE_L)TBLOCK;

/* Number of blocks in a file better not ever exceed an "int"
 * size, or this will fail. 
 */
blocks = (SIZE_I)pblocks;

/* 		*-*  WARNING  *-*
 *
 * readtape() will make appropriate adjustments to the global "blocks"
 * variable if a real "tape" archive switch is performed because
 * of an encountered  EOT. This must be done in order to prevent
 * this logic from reading an incorrect number of blocks when
 * switching tape archives.
 */
while (blocks > 0){
	readtape(buf,1, 1);
	blocks--;
}
}/*E passtape() */

/*.sbttl pmode() */

static int	m1[]	= { 1, ROWN, 'r', '-' };
static int	m2[]	= { 1, WOWN, 'w', '-' };
static int	m3[]	= { 2, SUID, 's', XOWN, 'x', '-' };
static int	m4[]	= { 1, RGRP, 'r', '-' };
static int	m5[]	= { 1, WGRP, 'w', '-' };
static int	m6[]	= { 2, SGID, 's', XGRP, 'x', '-' };
static int	m7[]	= { 1, ROTH, 'r', '-' };
static int	m8[]	= { 1, WOTH, 'w', '-' };
static int	m9[]	= { 2, STXT, 't', XOTH, 'x', '-' };

/* Next variable must come after the above to supply addresses.
 */
static int	*m[]	= { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode(st)
	struct stat *st;
{

	char *cp;
	register int **mp;

if (VFLAG) {
	switch(st->st_mode & S_IFMT) {

		case S_IFCHR:
			printf("c");
			break;

		case S_IFBLK:
			printf("b");
			break;

		case S_IFIFO:
			printf("p");
			break;

		case S_IFDIR:
			printf("d");
			break;

		default: {
			if ((cp = rindex(dblock.dbuf.name,'/'))==0)
				printf("-");
			else {
				cp++;
				if (!*cp)
					printf("d");
				else
					printf("-");
			}
		}
	}
}/*E if VFLAG */
	for (mp = &m[0]; mp < &m[9];)
		tselect(*mp++, st);

}/*E pmode() */

/*.sbttl prefix() */
prefix(s1, s2)
	register char *s1, *s2;
{

while (*s1)
	if (*s1++ != *s2++)
		return (0);
if (*s2)
	return (*s2 == '/');

return (1);

}/*E prefix() */

readtape(buffer, cnt, bflag)
	char *buffer;
	int cnt;
{

	int i, wcnt = cnt;
	char *from, *to;
	extern struct atblock *fblock[MAXASYNC];
	extern int curbuf;


if (recno >= nblock || first == 0) {

REREAD:
/**/
	if ((i = bread(mt, (char *)&fblock[curbuf][0], TBLOCK*nblock)) < 0) {
		fprintf(stderr, "%s: Archive read error at block %ld\n",
			progname, blocks_used);
		perror(usefile);
		done(FAIL);
	}
	if (!first) {

		first = 1;
		if ((i % TBLOCK) != 0) {
			fprintf(stderr, "%s: Archive blocksize error\n",
				progname);
			done(FAIL);
		}
		i /= TBLOCK;
		if (i != nblock) {
			fprintf(stderr,"%s: blocksize = %d\n", progname, i);
			if(i != 0)
				nblock = i;
		}
	}/*E if !first */

	recno = 0;
	first = 1;

	if (EOTFLAG) {
	/*
	 * bread()  has detected an  EOT  on a "real" tape.
	 * It has called  getdir()  to announce the end of media
	 * & has asked the user to mount the next archive. This
	 * has been done & bread() has returned to us some amount of
	 * data it has read from the new archive.
	 */
		bcopy((char *)&fblock[curbuf][recno], (char *)&dblock, TBLOCK);
	    if (blocks >= 0) {
		/*
		 * If blocks is < 0 at this point, an EOT and the
		 * end of file have occured at the same time.
		 * The logic will actually be in getdir() when the
		 * EOT is detected. When that is the case, then a
		 * file is not actually split across a volume.
		 * A new and different file will be started on the
		 * next archive in the set.
		 *
		 * Account for the continuation header block.
		 */
		blocks_used++;
		recno++;
		wcnt--;

	    }/*E if blocks > 0 */

		/* Go ask  getdir()  to verify that the correct
		 * continuation archive has been mounted by the user.
		 */
		VOLCHK++;

		getdir();

		if (!VOLCHK) 
			/* 
			 * getdir()  will negate the  VOLCHK  flag if
			 * it feels that the mounted archive was not
			 * the correct archive. We'll re-cycle thru
			 * the logic & try to goad the user into
			 * putting up the correct archive.
			 */
			goto REREAD;
		
		if (blocks >= 0) {

			/* Get the number of blocks in this continued
			 * chunk of a file.
			 */
			blocks =
			    (chctrs_in_this_chunk + (SIZE_L)TBLOCK - 1L) / (SIZE_L)TBLOCK;


			if (vflag && xflag) {

				if (OARCH && (original_size > chctrs_in_this_chunk)) 
					fprintf(stderr,"x %s, %ld bytes of %ld, %d blocks\n",
					  dblock.dbuf.name, chctrs_in_this_chunk,
					  original_size, blocks);
				else
					fprintf(stderr,"x %s%s, %ld bytes, %d blocks\n",
					  is_posix ? dblock.dbuf.posix_prefix : "",
					  dblock.dbuf.name, bytes, blocks);
			}/*E if vflag && xflag */

			if (tflag)
				dostats();	
		}/*E if blocks > 0 */
			
		EOTFLAG = VOLCHK = MULTI = FILE_CONTINUES = 0;

	}/*E if EOTFLAG */
}/*E if recno >= nblock ..*/

first = 1;

if(bflag) {
	bcopy((char *)&fblock[curbuf][recno],buffer,TBLOCK);
	recno++;
} else
	recno += wcnt;

blocks_used += wcnt;
return (wcnt);

}/*E readtape() */

/*.sbttl tselect() - was select but select(2) is a system call */
tselect(pairp, st)
	int	*pairp;
	struct stat	*st;
{

	int	*ap;
	int	n;


ap = pairp;
n = *ap++;

while (--n >= 0 && (st->st_mode & *ap++) == 0)
	ap++;

printf("%c", *ap);

}/*E select() */

/*.sbttl STRFIND() */

#ifdef TAR40
/* Function:
 *
 *	STRFIND
 *
 * Function Description:
 *
 *	Searches for  substr  in text.
 *	
 * Arguments:
 *
 *	char	*text		Text to search
 *	char	*substr		String to locate
 *
 * Return values:
 *
 *	Pointer to  substr  starting position in text if found.
 *	NULL if not found.
 *
 * Side Effects:
 *
 *	
 */
char *STRFIND(text,substr)
		char *text, substr;
{
/*------*\
  Locals
\*------*/

	COUNTER	i;	/* counter for possible fits */
	SIZE_I	substrlen;/* len of substr--to avoid recalculating */

/*------*\
   Code
\*------*/

substrlen = strlen(substr);

/* Loop through text until not found or match.
 */
for (i = strlen(text) - substrlen; i >= 0 &&
     strncmp(text, substr, substrlen); i--)

	text++;

/* Return NULL if not found, ptr else NULL.
 */ 
return ((i < 0 ? NULL : text));

}/*E STRFIND() */
#endif

/*.sbttl update()  Top level logic to UPDATE an archive */
update(arg,ftime)
	char	*arg;
	time_t	ftime;
{

	daddr_t	lookup();
	long	mtime;
	char	name[MAXPATHLEN];
	daddr_t	seekp;


rewind(tfile);

for (;;) {
	if ((seekp = lookup(arg)) < 0)
		return (1);

	fseek(tfile, seekp, 0);
	fscanf(tfile, "%s %lo", name, &mtime);
	return (ftime > mtime);
}
}/*E update() */

