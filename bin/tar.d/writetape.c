
# ifndef lint
static char *sccsid = "@(#)writetape.c	4.2	(ULTRIX)	12/6/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1989 by			*
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
 *	Modification History
 *
 *	30-Nov-90	lambert
 *			Fixed bug related to "16-Aug-88" bugfix listed below.
 *			Files which were openable but not readable by root were
 *			never being closed, and if more than OPEN_MAX (from
 *			/usr/include/limits.h) of these files were opened tar 
 *			would exit ungracefully (segfault).  
 *
 *	10-Sep-90	lambert
 *			Added code to fix "sflag" CLD (MUH-01628).  Tar was
 *			not correctly writing the UMA header to non-tape archives
 *			therefore not doing multivolume operations correctly.
 *			See routine tomodes().
 *
 *	06-Jun-90	kegel
 *			Fixed buffer allocation problem for MIPS -- use
 *			MAXASYNC valloc()s instead of one malloc();
 *			returns page aligned buffers for use with
 *			N-buffer IO.  See alloctape().
 *
 *	30-Mar-90	sam
 *			Changed processing of namelength so it would correctly
 *			honor the 100 character limit.
 *			Added zero-filling to "devmajor/minor" even when it's
 *			a null/unused field to satisfy POSIX requirements.
 *
 *	31-Oct-89	rsp
 *			Updated to include extern declarations, correct
 *			POSIX support.
 *
 *	 9-Sep-89	rsp
 *			Fixed 'putdir' routine variables path and pathname
 *			to have correct length.
 *
 *	30-May-89	bstevens
 *			Added new -R switch for archive lists of files.
 *
 *      16-May-89       rsp
 *                      Added status checking for ioctl's related to
 *                      tape positioning. Report all ioctl failures
 *                      except those related to nbuffio since some of
 *                      these 'errors' are used to branch to certain code.
 *
 *      31-Aug-88       lambert 
 *                      Changed include line from "tar.h" to <tar.h> for
 *                      POSIX support.
 *
 *	16-Aug-88	lambert
 *	Check read-ability of input file after opening, but before write to
 *	archive started, for uid=0 only.  This was needed due to NFS allowing
 *	root to OPEN files which it can't READ.
 *
 *	15-Mar-88	mjk - added posix support
 *
 *
 *	18-Dec-87	fries
 *	Modified code to correctly use getpw(). It was using
 *	the data returned even though an error was returned.
 *
 *	16-Dec-87	fries
 *	Added support for named pipes.
 *
 *	23-Sep-87	fries
 *	Added code to handle EOT condition with file fitting
 *	completely on end of tape(i.e. a new file starts on next
 *	tape).
 *
 *	13-Jan-87	fries
 *	Set -o option back to what it was prior to 26-Nov-82
 *	for compatibility.
 *
 *	22-Dec-86	fries
 *	Removed debug code.
 *
 *	26-Nov-86	fries
 *	Modified meaning of -o option. -o option now informs tar
 *	to save directory information(perm's and ownership).  The
 *	default is to only save file perm's and ownership.
 *
 *	 4-Nov-86	fries
 *	Fixed bug -o switch not saving directory info. when filesystems
 *	were crossed in directory pathname. Modified use of -o flag to
 *	perform directory info. saving only if -o specified.
 *
 *	16-Oct-86	lp
 *	Fix mdtar problem. Fix eot problem.
 *
 *	02-Jul-86	lp
 *	Cleanup and made n-bufferring work with eot. 
 *
 *	19-Jun-86	lp
 *	Added n-buffered hooks. Cleanup. Remove non-U32 ifdefs.
 * 
 *	Version derived from v1.2 tar.
 *
 */
#include <tar.h>

struct atblock *fblock[MAXASYNC];
int doingasync = 0, curbuf=0, writesize;
int pending[MAXASYNC];
extern struct DIRE *Dhead;
extern int device_open;
extern char DIRECT[];

checksum()
{
	char *cp;
	int	i = 0;

	for (cp = dblock.dbuf.chksum; 
		cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		    *cp = ' ';

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;

	return (i);
}

dorep(argv)
	char *argv[];
{
	char *chp;
	char *chp2;
	int i;
	FILE	*fp;
	char	s[NAMSIZ+1];

	if (CARCH != start_archive && AFLAG)
		fprintf(stderr,"\n%s: Skipping to %s:  %d\n", 
		progname, Archive, start_archive);

	if (!cflag) {
		/* Here if doing an add to end or update */
NEXTAU:
		/**/
		/* getdir() will fill in the  stats buffer from the file
		 * header block on the input archive.
		 */
		getdir();
		/*
		 * Save last file name  &  mod time for possible
		 * archive switch compare.
		 */
		if (dblock.dbuf.name[0]) {
			strcpy(file_name,dblock.dbuf.name);
			modify_time = stbuf.st_mtime; 
		}
		do {
			passtape();

			if (term)
				done(FAIL);

			/* getdir() fills in the  stats buffer from the file
		 	 * header block on the input archive.
		 	 */
			getdir();
			/*
		 	 * Save last file name  &  mod time for possible
		 	 * archive switch compare.
		 	 */
			if (dblock.dbuf.name[0]) {
				strcpy(file_name,dblock.dbuf.name);
				modify_time = stbuf.st_mtime;
			}

		} 
		while (!endtape());

		backtape();

		if (EODFLAG && FILE_CONTINUES)
			goto NEXTAU;

		OARCH = 0;

		if (tfile) {
			sprintf(iobuf,
			    "sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX; mv %sX %s",
			    tname, tname, tname, tname, tname, tname);
			fflush(tfile);
			system(iobuf);
			freopen(tname, "r", tfile);
			fstat(fileno(tfile), &stbuf);
			high = stbuf.st_size;
		}
	}

	getcwd(hdir);
	strcpy(wdir,hdir);

	while (*argv && !term) {
		chp2 = *argv;

		if (!strcmp(chp2,"-C") && argv[1]) {
			argv++;

			if ((chdir(*argv)) < 0) 
				perror(*argv);
			else  {
				getcwd(wdir);
				strcpy(hdir,wdir);
			}
			argv++;
			continue;

		}/*E if !strncmp */

		if (Rflag) {
			if ((fp = fopen(*argv, "r")) > 0) {
				while (fgets(s, NAMSIZ+1, fp)) {
					if (s[strlen(s)-1] == '\n')
						s[strlen(s)-1] = '\0';

					chp2 = s;

					for (chp = s; *chp; chp++)
						if (*chp == '/')
							chp2 = chp;

					if (chp2 != s) {
						*chp2 = '\0';

						if (chdir(s) < 0) {
							perror(s);
							continue;
						}
						getcwd(wdir);
						*chp2 = '/';
						chp2++;
					}
					
					/* Go put files on the output archive.*/
					if (putfile(s, chp2, wdir) == A_WRITE_ERR)
						return(A_WRITE_ERR);
					if (chdir(hdir) < 0) {
						fprintf(stderr,"%s: Can't change directory back ?", progname);
						perror(hdir);
						done(FAIL);
					}
				}
				fclose(fp);
			} else {
				fprintf(stderr,"%s: Can't open: %s\n",
					progname, *argv);
				perror(*argv);
			}
			argv++;
		} else {
			for (chp = *argv; *chp; chp++) 
				if (*chp == '/')
					chp2 = chp;

			if (chp2 != *argv) {
				*chp2 = '\0';

				if (chdir(*argv) < 0) {
					perror(*argv);
					continue;
				}
				getcwd(wdir);
				*chp2 = '/';
				chp2++;

			}/*E if chp2 != *argv */

			/* Go put files on the output archive. */
			if (putfile(*argv++, chp2, wdir) == A_WRITE_ERR)
				return(A_WRITE_ERR);

			if (chdir(hdir) < 0) {
				fprintf(stderr,"%s: Can't change directory back ?", progname);
				perror(hdir);
				done(FAIL);
			}
		}
	}/*E while (*argv && ! term) */

	PUTE++;
	for (i=3; i; i--) {
		if (putempty() == A_WRITE_ERR)
			return(A_WRITE_ERR);
	}
	if (flushtape() == A_WRITE_ERR)
		return(A_WRITE_ERR);


	if (VFLAG) {
		fprintf(stderr,"%s: links = %d/%d   directorys = %d/%d/%d\n\n",
		progname,lcount1,lcount2,dcount1,dcount2,dcount3);
	}
	if (!lflag)	/* -l: print link resolution errors */
		return(SUCCEED);

	for (; ihead; ihead = ihead->nextp) {
		if (!ihead->count)
			continue;

		fprintf(stderr, "%s: Missing links to:  %s\n", 
		progname, ihead->pathname);
	}
	return(SUCCEED);
}

char *getcwd(buf)
	char	*buf;
{
	int i;

	if (!getwd(iobuf)) {
		fprintf(stderr, "%s: %s\n", progname, iobuf);
		done(FAIL);
	}
	if ((i=strlen(iobuf)) >= MAXPATHLEN) {
		fprintf(stderr,"\n%s: File name too long: %s\n",
		    progname, iobuf);
		done(FAIL);
	}
	strcpy(buf,iobuf);
	return (buf);
}


putdir(longname,shortname)
	char *longname;
	char *shortname;
{
	struct stat	Dnode;
	struct direct	dirbuf;
	int i;
	int j;
	int dfound = 0;
	char *dpath;
	char	path[MAXPATHLEN];
	char	pathname[MAXPATHLEN];
	int sp = 0;
	int	statval;
	char *malloc();

	if (!strcmp(longname,".") || !strcmp(longname,"..") ||
	    !strcmp(shortname,"/"))
		return(SUCCEED);

	new_file = TRUE;

	if (longname[0] != '/') {
		/* For relative paths, form an absolute path name
			 * to stat.
			 */
		strcpy(pathname,hdir);
		strcat(pathname,"/");

		/* Set the Start Processing flag to skip over leading
		 * absolute path name components.
		 */
		sp = strlen(pathname);
		i = strlen(longname);

		for (j = 0; j < i; j++) {
			if (longname[j] != '.' && longname[j] != '/') break;
		}
		/* Check length of complete pathname prior to concatenating.
		   Allow for extra characters in relative pathnames. */
		if (sp + strlen(longname) > (MAXPATHLEN + j)) {
			fprintf(stderr,"\n%s: Path name too long: %s\n",
		    	    progname,longname);
			return(FAIL);
		}
		else strcat(pathname,longname);
	}
	else
		strcpy(pathname,longname);

	i = strlen(pathname);

	for (j=0; j < i; ) {
		path[j] = pathname[j++];
		if (pathname[j] == '/' || pathname[j] == '\n' || !pathname[j]) {
			if ( (j >= sp) && (j < MAXPATHLEN)) {
				path[j] = 0;

				if (!hflag)
					statval = lstat(path, &Dnode);
				else
					statval = stat(path, &Dnode);

				if (statval < 0) {
					fprintf(stderr,"\n%s: putdir() can't stat: %s\n",
					progname, path);

					perror(path);
					return(FAIL);
				}

				if ((Dnode.st_mode & S_IFMT) == S_IFDIR) {
					struct DIRE *direp;

					for (dfound=0,direp=Dhead; direp;
					direp = direp->dir_next) {
						    if ((direp->rdev == Dnode.st_dev) &&
						    (direp->inode == Dnode.st_ino)) {
							dfound++;
							break;
						}
					}
					if (!dfound) {
						/* Get some memory for our next
						 * directory list entry.
						 */
						direp = (struct DIRE *) malloc(sizeof (*direp));
						if (!direp) {

							/* If no mem, return what we
							 * have & try again ?
							 */
							fdlist();
							direp = (struct DIRE *) 
								malloc(sizeof (*direp));
							if (!direp) {
								fprintf(stderr,
								"\n\007%s: putdir() can't get memory for directory list\n",
								progname);

								NMEM4D++;
								return(FAIL);
							}
						}/*E if !direp */

						/* Save new directory entry */
						dcount1++;
						direp->rdev = Dnode.st_dev;
						direp->inode = Dnode.st_ino;
						direp->dir_next = Dhead;
						Dhead = direp; /* Lists run backwards */
						Dnode.st_size = 0L;
						Dnode.st_rdev = 0;
						remaining_chctrs = written = 0L;

						if (size_of_media[CARCH]) {
							if ((blocks_used) >
							    (size_of_media[CARCH] - (3L)))

								OARCH = CARCH;
							else
								OARCH = 0;
						}
						/* Set the directory path pointer
						 * start of absolute path name, or to
						 * the start of the relative name
						 * at the end of the absolute path.
						 */
						dpath = path + sp;
						strcat(dpath,"/");
						tomodes(&Dnode,1,dpath);
						sprintf(dblock.dbuf.chksum, "%06o", checksum());

						/* Go put directory file on the archive.
					 	 */
						if (writetape((char *)&dblock,
						1,1,dpath,dpath, 1) == A_WRITE_ERR)
							return(A_WRITE_ERR);

						if (vflag && (CARCH >= start_archive))
							fprintf(stderr,"a%s %s %s\n", 
							MFLAG ? CARCHS : NULS, dpath, VFLAG ? DIRECT : NULS);

					}/*E if !dfound*/
				}/*T if Dnode.st_mode ..*/
				else
					return(SUCCEED);

			}/*E if j >= sp .. */
		}/*E if pathname[j] ..*/
	}/*E for (j=0, d=0; j<i; ) */

	return(SUCCEED);

}/*E putdir() */

/* Function:
 *	putfile
 * Function Description:
 *	This function  RECURSIVELY  puts files on the output archive.
 * Arguments:
 * Return values:
 * Side Effects:
 */

putfile(longname, shortname, parent)
	char *longname;
	char *shortname;
	char *parent;
{
	char *cp;
	struct	direct	*dp;
	struct	direct	dirbuf;

	DIR	*dirp;
	int i;
	FILE_D	infile = 0;
	int j;
	char *malloc();

	new_file = TRUE;

	if (is_posix) {
		if (strlen(shortname) > NAMSIZ) goto NAMTL;
	}
	else if (strlen(longname) >= NAMSIZ) {
NAMTL:
		/**/
		fprintf(stderr,"\n%s: Path name too long: %s\n",
		    progname,longname);
		return(FAIL);
	}

	/* -h: Follow symbolic links. Test of user flag. */
	if (!hflag)
		i = lstat(shortname, &stbuf);	/* Don't follow symlinks  -
						 * stat link itself. */
	else
		i = stat(shortname, &stbuf);	/* Follow - stat actual file */

	if (i < 0) {
		fprintf(stderr,"\n%s: putfile() can't stat: %s\n",
		progname, shortname);

		perror(shortname);
		return(FAIL);

	}/*E if i < 0 */

	/* (-u) Latest version already in archive? Yes: Skip.
	 */
	if (tfile) {
		if (!update(longname, stbuf.st_mtime))
			return(SUCCEED);
	}
	/* (-w) Passed user confirmation? No: skip.
	 */
	if (wflag) {
		if (!checkw('r', longname))
			return(SUCCEED);
	}
	/* (-F[FF]) Recreatable file? Yes: skip for speed.
	 */
	if (Fflag) {
		if (checkf(shortname, stbuf.st_mode, Fflag) == 0)
			return(SUCCEED);
	}
	/* Process directories in path..
	 */
	if (!oflag && !NMEM4D && !DFLAG && !NFLAG)
		if (putdir(longname,shortname) == A_WRITE_ERR)
			return(A_WRITE_ERR);

	switch (stbuf.st_mode & S_IFMT) {

	case S_IFDIR: 
		{

			char	curd[MAXPATHLEN];
			char	newparent[MAXPATHLEN];
			char	pfnbuf[NAMSIZ];

			getcwd(curd);
			/*
	 		 * Copy the long file name to a work buffer and
	 		 * append a "slash null".
	 		 * ie.. ->  test/0  ...
	 		 * Set up in order to tack on a file name to a
	 		 * potential directory name.
	 		 */
			for (i = 0, cp = pfnbuf; *cp++ = longname[i++];)
				; /*-NOP-*/

			*--cp = '/';
			*++cp = 0;


			/* If no memory for directory lists, or user
			 * doesn't want us to keep them, put out the
			 * directory as in the past.
			 */

			if ((NFLAG || DFLAG || NMEM4D) && !oflag) {

				if (size_of_media[CARCH]) {
					if ((blocks_used) >
					    (size_of_media[CARCH] - (3L)))

						OARCH = CARCH;
					else
						OARCH = 0;
				}
				dcount2++;
				stbuf.st_size = 0L;
				stbuf.st_rdev = 0;
				remaining_chctrs = written = 0L;
				tomodes(&stbuf,1,pfnbuf);
				sprintf(dblock.dbuf.chksum, "%06o", checksum());

				/* Go put directory file on the archive. */
				if (writetape((char *)&dblock,1,1,pfnbuf,pfnbuf,1) == A_WRITE_ERR)
					return(A_WRITE_ERR);

				if (vflag && (CARCH >= start_archive))
					fprintf(stderr,"a%s %s %s\n", MFLAG ? CARCHS : NULS, pfnbuf, VFLAG ? DIRECT : NULS);
			}/*E if NFLAG .. */


			i = 0;

			if (chdir(shortname) < 0) {
				fprintf(stderr, "%s: Can't change directory to:  %s\n", progname, shortname);
				perror(shortname);
				return(FAIL);
			}
			if ((dirp = opendir(".")) == NULL) {
				fprintf(stderr,"%s: Directory read error:  %s\n", progname, longname);
				perror(longname);

				if (chdir(curd) < 0) {
					fprintf(stderr, "%s: Can't change directory back to %s ?\n", progname, curd);
					perror(curd);
				}
				return(FAIL);
			}
			getcwd(newparent);

			while ((dp = readdir(dirp)) && !term) {
				if (!dp->d_ino)
					continue;

				if (!strcmp(".", dp->d_name) ||
				    !strcmp("..", dp->d_name))
					continue;

				strcpy(cp, dp->d_name);
				i = telldir(dirp);
				closedir(dirp);

				if (putfile(pfnbuf, cp, newparent) == A_WRITE_ERR)
					return(A_WRITE_ERR);

				dirp = opendir(".");
				seekdir(dirp, i);

			}/*E while dp = readdir ..*/

			closedir(dirp);


			if (chdir(curd) < 0) {
				fprintf(stderr, "%s: Can't change directory back to:  %s\n", progname, curd);
				perror(curd);
				return(FAIL);
			}
			break;

		}/*E case S_IFDIR */
		/*
		 */
	case S_IFLNK:

		remaining_chctrs = written = 0L;

		if (size_of_media[CARCH]) {
			if ((blocks_used + (SIZE_L)blocks) >
			    (size_of_media[CARCH] - (3L)))

				OARCH = CARCH;
			else
				OARCH = 0;
		}

		if (stbuf.st_size + 1L  >= NAMSIZ)
			goto NAMTL;

		/* Should be safe to insert dblock stats.
				 */
		stbuf.st_size = 0L;
		tomodes(&stbuf,1,longname);


		/* Warning: 
		 *	The string returned by "readlink" is not null
		 *	terminated. The code relies on the fact that
		 *	tomodes() zeroes the entire dblock. ergo: by
		 *	definition, the linkname in dblock will be
		 *	null terminated.
		 */
		i = readlink(shortname, dblock.dbuf.linkname, (NAMSIZ - 1));
		if (i < 0) {
			perror(longname);
			return(FAIL);
		}
		dblock.dbuf.typeflag = SYMTYPE;
		sprintf(dblock.dbuf.chksum, "%06o", checksum());

		if (vflag && (CARCH >= start_archive)) {
			fprintf(stderr,"a%s %s  symbolic link to %s\n",
			MFLAG ? CARCHS : NULS, longname, dblock.dbuf.linkname);
		}

		if (writetape((char *)&dblock,1,1, shortname, longname,1) == A_WRITE_ERR)
			return(A_WRITE_ERR);

		break;
		/*
		 */
	case S_IFREG:

		if ((infile = open(shortname, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: Can't open file:  %s\n", progname, longname);
			perror(longname);
			return(FAIL);
		}

		/* 
		 *  If running as root then see if we have read access to this
		 *  file.  NFS will allow root to OPEN a file it doesn't have
		 *  read access to, which will cause problems later when a read
		 *  is attempted.
		 */
		if (geteuid() == 0) {
			char	*tmp_buf[TBLOCK];
			if (read(infile, tmp_buf, 1) < 0) {
				fprintf(stderr, "%s: Can't read file: %s\n", progname, longname);
				perror(longname);
				close(infile);
				return(FAIL);
			}
			else {
				if (lseek(infile, 0, L_SET) < 0) {
					fprintf(stderr, "%s: lseek error - ", progname);
					perror(longname);
					return(FAIL);
				}
			}
		}
		if (stbuf.st_nlink > 1) {
			found = 0;
			tomodes(&stbuf,1,longname);

			for (lp = ihead; lp; lp = lp->nextp)
				if (lp->inum == stbuf.st_ino && lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			/* 
			 * Fix for IPR-00006.
			 * If the linked file was already output,
			 * don't output subsequent copies.
			 */
			if (found && (!strcmp(dblock.dbuf.name, lp->pathname))) {
				if (CARCH >= start_archive)
					fprintf(stderr,"%s: Linked file has already been output. Skipping:  %s\n",
					progname, lp->pathname);
				close(infile);
				return(FAIL);
			}
			if (found) {
				strcpy(dblock.dbuf.linkname, lp->pathname);
				dblock.dbuf.typeflag = LNKTYPE;
				sprintf(dblock.dbuf.size, "%011o", 0L);
				sprintf(dblock.dbuf.chksum, "%06o", checksum());
				if (writetape((char *) &dblock,1,1,shortname,longname, 1) == A_WRITE_ERR) {
					close(infile);
					return(A_WRITE_ERR);
				}
				if (vflag && CARCH >= start_archive) 
					fprintf(stderr,"a%s %s  link to %s\n",
					MFLAG ? CARCHS : NULS, longname, lp->pathname);
				lp->count--;

				close(infile);
				return(SUCCEED);
			}

			lp = (struct linkbuf *) malloc(sizeof(*lp));

			if (!lp) {

				fdlist();
				lp = (struct linkbuf *) malloc(sizeof(*lp));
				if (!lp && !NMEM4L) {
					fprintf(stderr,"\n\007%s: Out of memory, link information lost\n",
					progname);

					NMEM4L++;
				}
			} 
			if (lp) {
				lcount1++;
				lp->nextp = ihead;
				ihead = lp;
				lp->inum = stbuf.st_ino;
				lp->devnum = stbuf.st_dev;
				lp->count = stbuf.st_nlink - 1;

				strcpy(lp->pathname, longname);
			} 
			else
				lcount2++;

		}/*E if stbuf.st_nlink > 1 */

		blocks = (stbuf.st_size + (SIZE_L)(TBLOCK-1L)) / (SIZE_L)TBLOCK;

		written = 0L;
		remaining_chctrs = stbuf.st_size;

		if (size_of_media[CARCH]) {
			if ((blocks_used + (SIZE_L)blocks) >=
			    (size_of_media[CARCH] - (nblock+2L)))

				OARCH = CARCH;
			else
				OARCH = 0;
		}

		/* For files with multiple links "tomodes" has already been
		   called above.  */
		if (stbuf.st_nlink < 2) tomodes(&stbuf,blocks,longname);
		sprintf(dblock.dbuf.chksum, "%06o", checksum());

		/*
		 * Write directory header block for this file.
	 	 */
		if (writetape((char *)&dblock,1,blocks,shortname,longname,1) == A_WRITE_ERR) {
			close(infile);
			return(A_WRITE_ERR);
		}

		if (vflag && (CARCH >= start_archive))
			fprintf(stderr,"a%s %s %d blocks \n",
			MFLAG ? CARCHS : NULS, longname, blocks);

		if ((start_archive - 1) > CARCH) {
			/*
						 * If skipping archives, try to avoid
						 * reading the file.
						 */
			close(infile);

			for (; blocks > 0; blocks--) {	
				if (writetape(iobuf,1,blocks,shortname,longname,1) == A_WRITE_ERR)
					return(A_WRITE_ERR);	

			}
		}
		else {
			int left = nblock - recno, cnt=0;

			if(OARCH)
				left = 1;
			while ((i = read(infile, 
			(char *)&fblock[curbuf][recno], left*TBLOCK)) > 0
			    && blocks > 0) {

				cnt = ((i-1)/TBLOCK)+1;
				if(cnt > blocks)
					cnt=blocks;
				written += (SIZE_L)i;

				/* iobuf not used here */
				if (writetape(&fblock[curbuf][recno],cnt,blocks,
				shortname,longname,0) == A_WRITE_ERR) {
					close(infile);
					return(A_WRITE_ERR);
				}
				left = nblock - recno;
				if(OARCH)
					left = 1;
				if(left > stbuf.st_blksize)
					left = stbuf.st_blksize;

				blocks -= cnt;

			}/*E while i = ..*/

			if(blocks < 0 || blocks == 1)
				blocks = 0;

			close(infile);

			if (blocks != 0 || i != 0)
				fprintf(stderr,"%s: File changed size:  %s\n", 							progname, longname);
			while (--blocks >=  0)
				if (putempty()==A_WRITE_ERR)
					return(A_WRITE_ERR);

		}
		break;

		/*	Special files !?!
		 */
	case S_IFCHR:
	case S_IFBLK:
	case S_IFIFO:

		remaining_chctrs = written = 0L;

		if (size_of_media[CARCH]) {
			if ((blocks_used + (SIZE_L)blocks) >
			    (size_of_media[CARCH] - (3L)))

				OARCH = CARCH;
			else
				OARCH = 0;
		}
		stbuf.st_size = 0L;
		tomodes(&stbuf,1,longname);
		sprintf(dblock.dbuf.chksum, "%06o", checksum());

		if (writetape((char *) &dblock,1,1,shortname,longname,1) == A_WRITE_ERR)
			return(A_WRITE_ERR);

		if (vflag && (CARCH >= start_archive))
			if ((stbuf.st_mode & S_IFMT) == S_IFIFO)

				fprintf(stderr,"a%s %s (named pipe)\n",
				MFLAG ? CARCHS : NULS, longname);
			else
				fprintf(stderr,"a%s %s (special file)\n",
				MFLAG ? CARCHS : NULS, longname);

		break;

	default:
		fprintf(stderr, "%s: %s <- Is not a file. Not dumped\n", progname, longname);

		break;

	}/*E switch (stbuf.st_mode & S_IFMT) */
	return(SUCCEED);

}/*E putfile()*/

/* Function:
 *
 *	tomodes
 *
 * Function Description:
 *
 *	Put the file status modes in the tar header block
 *	for this file.
 *
 * Arguments:
 *
 *	struct stat	*sp	Pointer to filestat structure.
 *	SIZE_I	rblocks		Number of real blocks to be used by
 *				this file on the archive excluding
 *				the header block.
 *	char *name	Pointer to file name string
 *
 * Return values:
 *
 *
 * Side Effects:
 *
 *	
 */

int lastuid = -1, lastgid = -1;

tomodes(sp,rblocks,name)
	struct stat	*sp;
	SIZE_I	rblocks;
	char *name;
{
	char *cp;
	INDEX	j;
	int	majordev, minordev;
	char	pwfdat[256];


	/* Zero out directory header block.
	 */
	bzero(dblock.dummy, TBLOCK);

	/* Insert file stats into directory/header block.
	 */
	if(is_posix) {
		char *tp;

		if((tp = rindex(name, '/')) == NULL) /* file in cdir */
			strcpy(dblock.dbuf.name, name);
		else {
			/*
			 * there are two cases that need to be checked.
			 * first -- the names of created directories appear
			 * in the name field (with the trailing /) rather
			 * than the prefix field.
			 * second -- make sure that there is no prefix when
			 * tar'ng the current directory
			 */
			if(*(++tp)) {
				strcpy(dblock.dbuf.name, tp);
				strncpy(dblock.dbuf.posix_prefix, name,
				    tp - name); 
				dblock.dbuf.posix_prefix[tp - name + 1] = '\0';
			} else		/* we are doing . or / */
				strcpy(dblock.dbuf.name, name);
		}
		strcpy(dblock.dbuf.posix_magic, TMAGIC);
		strncpy(dblock.dbuf.posix_version, TVERSION, TVERSLEN);
	} 
	else {
		strcpy(dblock.dbuf.name, name);
		dblock.dbuf.org_size[0] = ' ';		

		/* Fix for "sflag CLD"
                   Tar was previously never writing a UMA header.
                   Now it will always write a UMA header if the sflag
                   is specified, or if it was invoked as mdtar.
                */
                if (sflag || MDTAR)
                        strcpy(dblock.dbuf.magic, OTMAGIC);
                else    sprintf(dblock.dbuf.magic, "%6o",sp->st_rdev);
	}
	sprintf(dblock.dbuf.mode, "%06o", sp->st_mode);
	sprintf(dblock.dbuf.uid, "%06o", sp->st_uid);
	sprintf(dblock.dbuf.gid, "%06o", sp->st_gid);
	sprintf(dblock.dbuf.size, "%011o", sp->st_size);
	sprintf(dblock.dbuf.mtime, "%011o", sp->st_mtime);
	if (!NFLAG) {
		static int pwstatus;

		/* Insert User Group standard format indicator.
			 */
		/* If new uid, then get user name from /etc/passwd file */
		if (lastuid != sp->st_uid) {
			pwstatus = getpw(sp->st_uid, pwfdat);

			/* If no error getting user name */
			if (!pwstatus) {
				/* Insert users' name from password file into
			 	* the header block.
			 	*/
				for (j=0;(j<TUNMLEN && pwfdat[j] && pwfdat[j] != ':'); j++) 
					dblock.dbuf.uname[j] = pwfdat[j];

				dblock.dbuf.uname[j] = 0;
			}
			else if (OFLAG) {
				fprintf(stderr,"\n%s: Can't find user name in password file. UID = %d\n%s: File name = %s\n\n",
				progname, sp->st_uid, progname, name);
			}
		} /* End of if new uid code */

		/* If new GID, then find group name in /etc/groups file */
		/* Bugfix for POSIX: if in POSIX mode always place gname
		   string in header */
		if ((lastgid != sp->st_gid) || (is_posix)) {
			gp = getgrgid(sp->st_gid);

			if(gp != NULL) {
				char	*cp;
				cp = gp->gr_name;
				for (j=0;(j<TGNMLEN && *cp && *cp != ':'); j++, cp++)
					dblock.dbuf.gname[j] = *cp;
				dblock.dbuf.gname[j] = 0;
			}
			else {
				if (OFLAG)
					fprintf(stderr,"\n%s: Can't find group name in /etc/group file. GID = %d\n%s: File name = %s\n\n",
					progname, sp->st_gid, progname, name);
			}
		} /* End of if new GID code */

		/* POSIX wants devmaj/min to be "zero filled octal numbers"
		   even if those fields aren't used for this type of file.  */
		if (is_posix) {
			sprintf(dblock.dbuf.devmajor, "%06o", 0L);
			sprintf(dblock.dbuf.devminor, "%06o", 0L);
		}

		switch (sp->st_mode & S_IFMT) {

		case S_IFDIR:
			dblock.dbuf.typeflag = DIRTYPE;
			break;

		case S_IFIFO:
			dblock.dbuf.typeflag = FIFOTYPE;
			goto comsd;

		case S_IFCHR:
			dblock.dbuf.typeflag = CHRTYPE;
			goto comsd;

		case S_IFBLK:
			dblock.dbuf.typeflag = BLKTYPE;
comsd:
			majordev = sp->st_rdev >> 8;
			minordev = sp->st_rdev & 0377;
			sprintf(dblock.dbuf.devmajor, "%06o", majordev);
			sprintf(dblock.dbuf.devminor, "%06o", minordev);
			break;

		default:
			dblock.dbuf.typeflag = REGTYPE;
			break;

		}/*E switch sp->st_mode & S_IFMT */

		/* Put Ultrix archive numbers in the header
		 * unless a User-Group-Standard archive is desired.
		 * ie. no multi-archive extensions.
		 */
		if (!SFLAG && !is_posix) {
			sprintf(dblock.dbuf.carch, "%2d",CARCH);
			sprintf(dblock.dbuf.oarch, "%2d",OARCH);

			if (size_of_media[CARCH] || (CARCH <= start_archive)) {

				SIZE_L  available_blocks;

				if (OARCH) {
					/*
					 * File is being split across an archive.
					 */
					sprintf(dblock.dbuf.org_size, "%11lo",sp->st_size);
					/*
					 * Determine how much space is left on
					 * this archive and assume one block as a
					 * minimum for any output.
					 */
					available_blocks =
					    size_of_media[CARCH] - (nblock + blocks_used +4L);

					/* Is there at least one block for the header ?
					 */
					if (available_blocks >= 0L) {
						/*
						 * Set continuation header flag.
						 */
						header_flag = 1;

						if (rblocks > available_blocks)
							chctrs_in_this_chunk =
							    (SIZE_L)TBLOCK * available_blocks;
						else
							chctrs_in_this_chunk =
							    (SIZE_L)TBLOCK * rblocks;

						if (remaining_chctrs >= chctrs_in_this_chunk)
							remaining_chctrs -= chctrs_in_this_chunk;
						else
							/* This will be last chunk. 
							 */
							chctrs_in_this_chunk = remaining_chctrs;

					}/*T if available_blocks >= 0 */
					else {
						/* Default chunk size to remaining
						 * chctrs because this file will
						 * start fresh on the next archive.
						 */
						chctrs_in_this_chunk = remaining_chctrs;

						header_flag = 0;

					}/*F if available_blocks >= 0 */

					sprintf(dblock.dbuf.size, "%11lo", chctrs_in_this_chunk);

					/* Re-insert mod time because previous
				 	 * sprintf overwrites p/o mtime field.
				   	 */
					sprintf(dblock.dbuf.mtime, "%11lo",sp->st_mtime);
				}/*E if OARCH */
			}
		}
	}/*E if !NFLAG */
	lastuid=sp->st_uid;
	lastgid=sp->st_gid;

}/*E tomodes() */

/* Function:
 *
 *	writetape
 *
 * Function Description:
 *
 *	Top level logic to write an output archive.
 *
 * Arguments:
 *
 *	char	*buffer		Pointer to data buffer to write from
 *	long	blocks		Number of blocks to add to those
 *				on the current archive in this call.
 *	long	total_blocks	Total number of blocks caller would
 *				like to place on the device eventually.
 *	char	*longname	Files' name. Long and short..
 *	char	*shortname
 *
 * Return values:
 *
 *	TRUE	If the number of blocks written to the device
 *		successfully.
 *
 * Side Effects:
 *
 *	When writting data, the number of blocks used is
 *	updated by the number of blocks written.
 *	If an error is detected, the routine exits via the "done"
 *	subroutine.
 */
extern long blocks;

writetape(buffer, blocks, total_blocks, shortname, longname, bflag)
	char *buffer;
	int blocks;
	int total_blocks;
	char *shortname;
	char *longname;
{
	char *from;
	char *to;
	int i;
	int write_header;


	if (FEOT)
		goto WEOA;

	if (size_of_media[CARCH]) {
		char junk[TBLOCK];

		if ((((SIZE_L)blocks + blocks_used) ==
		    (size_of_media[CARCH] - (nblock+2))) && !is_posix) {
			if (!PUTE) {
				bcopy(buffer,&junk[0],TBLOCK);
				blocks_used += (3L);
				MULTI++;

				/* Continuing to next vol, tack on the EOA blocks.
						 */
				if (puteoa() == A_WRITE_ERR)
					return(A_WRITE_ERR);
WEOA:
				/**/
				/* Force n-buffered to finish */
				if(doingasync) {
					int i,zz, rfail = 0;
					int curptr = curbuf, nleft = 0, errno=0;
					char *tmp;

					for(i=0; i<doingasync; i++) {
						tmp = (char *)&fblock[i][0];
						if(((zz = ioctl(mt,FIONBDONE,&tmp)) != TBLOCK*nblock)){
							if(pending[i] == 0)
								rfail = 1;
							nleft++;
							pending[i] = -1;
						}
					}
					/* disable eot handling */
					mtops.mt_op = MTCSE;
					mtops.mt_count = 1;
					if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
						fprintf(stderr, "%s: ", progname);
						perror("ioctl MTCSE failed on write");
					}
					/* disable nbuf */
					i = 0;
					(void) ioctl(mt, FIONBUF, &i);
					errno=0;

					if(nleft)
						if(++curptr >= doingasync)
							curptr = 0;

					if(!rfail){
						nleft--;
					}

					for(i=0; i<doingasync && nleft>0; i++) {
						int bytes;
						if(pending[curptr] == -1) {
							mtops.mt_op = MTCSE;
							mtops.mt_count = 1;
							if(ioctl(mt, MTIOCTOP, &mtops) < 0) {
								fprintf(stderr, "%s: ", progname);
								perror("ioctl MTCSE failed on write");
							}
							bytes = write(mt, (char *)&fblock[curptr][0], nblock*TBLOCK);
							if( bytes < 0){
								fprintf(stderr, "%s: ", progname);
								perror("write past eot");
								done(errno);
							}
							nleft--;
						}
						pending[curptr] = 0;
						if(++curptr >= doingasync)
							curptr = 0;
					}

					curbuf = curptr;
					for(i=0; i<doingasync; i++)
						pending[i] = 0;
				}

				if (VFLAG && EOTFLAG) 
					fprintf(stderr,"\n%s: %ld Blocks used on %s for %s %d\n",
					progname,
					FEOT ? (blocks_used - (SIZE_L)blocks)
					    : (blocks_used - (SIZE_L)nblock),
					usefile, Archive, CARCH);

			/* Remember how big this object was for possible
			 * error recovery. For tapes encountering an EOT, 3
			 * is added to the count because the pseudo writes
			 * on retry "assume" that we will need 3 blocks for
			 * end-of-archive info that really isn't present when
			 * an EOT is seen.
			 */
				if (FEOT)
					size_of_media[CARCH++] =
					    (blocks_used - (SIZE_L)blocks) + (3L);
				else

					size_of_media[CARCH++] =
					    EOTFLAG ? (blocks_used - (SIZE_L)nblock) + (3L)
						: blocks_used;

				close(mt);
				device_open = 0;
				sprintf(CARCHS, "%d", CARCH);

				/* If we have encountered a REAL end of tape, there
				 * are nblocks of data waiting to go out.
				 * If the EOT occured during flushtape(), there are
				 * "blocks" worth of data in tbuf to go out.
				 */
				if (FEOT)
					blocks_used = (SIZE_L)blocks;
				else
					blocks_used = EOTFLAG ? (SIZE_L)nblock : 0L;

				if (CARCH > start_archive) {

					if (CARCH > MAXAR) {
						fprintf(stderr,"\n%s: Reached maximum %s limit: EXITING\n"
						    ,progname, Archive);
						done(FAIL);
					}
					/*
					 * Reset media size to default when
					 * resuming real writting on error recovery.
					 */
					size_of_media[CARCH] = size_of_media[0];

					fprintf(stderr,"\n\007%s: Please change %s media on  %s  & press RETURN. ",
					progname, Archive, usefile);

					response();

				}
OPENA:
				if ((mt = statchk(usefile, O_RDWR)) < 0) {
					if(errno){
						fprintf(stderr, "\n\007%s: Can't open:  %s\n", progname, usefile);
						perror(usefile);
					}
					fprintf(stderr,"%s: Please press  RETURN  to retry "
					    , progname);
					response();
					goto OPENA;
				}

				/* curbuf is still valid */

				if (OARCH) {
					write_header = header_flag;
					recno = 0;

					if (!EOTFLAG) {
						/*
						 * __must be end of disk__
						 */
						tomodes(&stbuf, total_blocks,longname);
						sprintf(dblock.dbuf.chksum,"%06o", checksum());

						if (vflag && (CARCH >= start_archive))
							fprintf(stderr,"%s: Continuing %D bytes of  %s  to  %s %d\n\n",
							progname, chctrs_in_this_chunk, longname
							    , Archive, CARCH); 

						MULTI = 0;

						if (write_header) { 
							if (writetape((char *)&dblock,1,1
							    ,shortname,longname,1) == A_WRITE_ERR) {
								done(A_WRITE_ERR);
								return(A_WRITE_ERR);
							}
						}
						bcopy(&junk[0], &fblock[curbuf][recno], TBLOCK);
						/*
						 * Fall out
						 */
					}/*E if !EOTFLAG */
					if (EOTFLAG) {
						char	lastblock[TBLOCK];
						char *cp;
						int	i;

				/* cremain will be zero in a rare case of a
				 * file ending on an exact EOT boundry.
				 * It does not get split across tapes.
				 */
						cremain = remaining_chctrs - written;
						if (written && recno) {
							if (vflag && cremain && (CARCH >= start_archive))
								fprintf(stderr,"%s: Continuing %D bytes of  %s  to  %s %d\n\n",
								progname, cremain, cblock.dbuf.name, Archive, CARCH); 

							bcopy((char *)&fblock[curbuf][nblock-1],
							lastblock, TBLOCK);
							/* Shuffle the buffer content down
											 * one block.
											 */
							for (from = (char *)&fblock[curbuf][nblock-2],
				     to = (char *)&fblock[curbuf][nblock-1],
				     i = nblock-1; i; i--) {

								bcopy(from, to, TBLOCK);
								to = from;
								from = to - TBLOCK;
							}

							/* Initialize continuation header block.
											 */
							sprintf(cblock.dbuf.carch, "%2d",CARCH);
							sprintf(cblock.dbuf.oarch, "%2d",OARCH);
							sprintf(cblock.dbuf.size, "%11lo",cremain);
							sprintf(cblock.dbuf.mtime, "%11lo",cmtime);
							sprintf(cblock.dbuf.org_size, "%11lo",corgsize);

							/* Compute new checksum
											 */
							for (cp = cblock.dbuf.chksum;
				      cp < &cblock.dbuf.chksum[sizeof(cblock.dbuf.chksum)];
					cp++)

								*cp = ' ';

							for (cp = cblock.dummy;
				     cp < &cblock.dummy[TBLOCK]; cp++)

								i += *cp;

							sprintf(cblock.dbuf.chksum,"%06o", i);

					/* Copy in continuation header block */

							bcopy((char *)&cblock,
							(char *)&fblock[curbuf][0],TBLOCK);

						/* Write out the new buffer */

							if ((write(mt,(char *)&fblock[curbuf][0], TBLOCK*nblock)) < 0) {

								goto WERR;
							}

					/* Copy the saved lastblock to the start
				 	* of the buffer for the next write.
				 	*/
							bcopy(lastblock,(char *)&fblock[curbuf][recno++],TBLOCK);
							blocks_used++;
						}/* End of if written and not first record position */
						else{
							if (vflag && cremain && (CARCH >= start_archive))
								fprintf(stderr,"%s: Continuing %D bytes of  %s  to  %s %d\n\n",
								progname, cremain, dblock.dbuf.name, Archive, CARCH); 

							sprintf(dblock.dbuf.carch,"%2d",CARCH);
							sprintf(dblock.dbuf.oarch,"%2d",OARCH);
							sprintf(dblock.dbuf.size, "%11lo",cremain);
							sprintf(dblock.dbuf.mtime, "%011lo",stbuf.st_mtime);
							sprintf(dblock.dbuf.org_size, "%011lo",stbuf.st_size);
							sprintf(dblock.dbuf.chksum,"%06o", checksum());

							bcopy((char *)&dblock, (char *)&fblock[curbuf][0],TBLOCK);
							blocks_used++;
							recno++;
						} /* End of !written or not first record position */


						MULTI = EOTFLAG = OARCH = 0;
						/* reenable n-buffered */
						if(doingasync) {
							int cnt = doingasync;
							if(ioctl(mt, FIONBUF, &cnt) < 0)
								doingasync = 0;
							else
								doingasync = cnt;
						}

						return(SUCCEED);

					}/*E if EOTFLAG */

				}/*if OARCH */
			}/*E if !PUTE */
		}/*E if (SIZE_L)blocks + blocks_used ..*/
	}/*E if size_of_media[CARCH] */

	blocks_used += (SIZE_L)blocks;

	if(bflag == 1)
		taprec(buffer);
	else
		recno += blocks;

	if (recno >= nblock) {
		if (CARCH >= start_archive) {


			if((write(mt, (char *)&fblock[curbuf][0], TBLOCK*nblock)) < 0) {
herr:
				/* TODO set up pending for next vol! */

				if ((ioctl(mt, DEVIOCGET, &mtsts)<0) ||
				    size_of_media[CARCH] ||
				    (errno != ENOSPC) || NFLAG) {

WERR:
					/**/
					fprintf(stderr,"\n");
					perror(usefile);
					fprintf(stderr, "\007%s: Archive  %d  write error\n", progname, CARCH);
					done(A_WRITE_ERR);
					return(A_WRITE_ERR);
				}
				else {
					if (!(mtsts.stat & DEV_EOM))
						goto WERR;
					else {
						mtops.mt_op = MTCSE;

						if (ioctl(mt, MTIOCTOP, &mtops) < 0) {
							goto WERR;
						} 
						else {
							fprintf(stderr,"\n%s: End of %s media", progname, Archive);
							OARCH = CARCH;
							EOTFLAG++;
							MULTI++;
							goto WEOA;
						}
					}
				}
			}
			/* Save possible continuation block when a
					 * new/different file is started.
					 */
			if (new_file) {
				bcopy((char *)&dblock,(char *)&cblock,TBLOCK);
				new_file = FALSE;
			}

			if(doingasync) {
				pending[curbuf] = 1;
				if(++curbuf >= doingasync)
					curbuf = 0;

				if(pending[curbuf]) {
					char *tmp = (char *)&fblock[curbuf][0];
					if(ioctl(mt, FIONBDONE, &tmp) != TBLOCK*nblock) {
						if(errno != EINVAL) {
							if(errno = 0)
								errno=ENOSPC;
							goto herr;
						}
					}
					pending[curbuf] = 0;
				}
			}

			cremain = remaining_chctrs - written;
			corgsize = stbuf.st_size;
			cmtime = stbuf.st_mtime;



		}/*E if (CARCH >= start_archive) */
		recno = 0;

	}
	return (SUCCEED);

}/*E writetape() */

/*
 * Allocate the buffer for tape operations.
 * ---------------------------------------
 *
 * For later kernel performance improvement, this buffer should be allocated
 * on a page boundary.
 * DONE through valloc() -- agk 6/5/90
 */
alloctape()
{
	register char *mtemp;
	int i;
	char *malloc();
	void *valloc();

	writesize = nblock * TBLOCK;

#ifdef OLDSTUFF
	mtemp = (char *)malloc((writesize * MAXASYNC) + 1024);
	if(mtemp == (char *)0)
		return(0);

	/* Clumsy way to page align the buffer - malloc should do this */
	mtemp = (char *) ((unsigned int)mtemp +
	    (1024 - ((unsigned int)mtemp % 1024)));

	for(i=0; i<MAXASYNC; i++) {
		fblock[i] = (struct atblock *)mtemp;
		mtemp += writesize;
	}

#else /* NEW STUFF */
		/* valloc() returns page-aligned buffers.
		 * valloc each buffer individually so each
		 * buffer is properly aligned.
		 */
	for(i=0; i<MAXASYNC; i++) {
		mtemp = (char *)valloc( (size_t)(writesize) );
		if(mtemp == (char *) NULL)
			return(NULL);
		fblock[i] = (struct atblock *)mtemp;
	}
#endif /* OLDSTUFF	*/
	return (fblock[MAXASYNC-1] != NULL);
}

struct u_spcl {
	char pad[TBLOCK];
};

/* Update tape record number, if tape record # > or = to the	*/
/* # of blocks to write, then flush tape write buffer to tape	*/
taprec(dp)
	char *dp;
{
	*(struct u_spcl *)(&fblock[curbuf][recno]) = *(struct u_spcl *)dp;

	recno++;	/* Bump # of tape block counter */
}
