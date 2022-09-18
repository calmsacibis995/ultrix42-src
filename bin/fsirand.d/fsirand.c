#ifndef lint
static	char	*sccsid = "@(#)fsirand.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <ufs/fs.h>
#include <sys/gnode_common.h>
#include <ufs/ufs_inode.h>

char fsbuf[SBSIZE];
struct dinode dibuf[8192/sizeof (struct dinode)];
extern int errno;

main(argc, argv)
int	argc;
char	*argv[];
{
	struct fs *fs;
	int fd;
	char *dev;
	int bno;
	struct dinode *dip;
	int inum, imax;
	int n;
	int seekaddr, bsize;
	int pflag = 0;
	struct timeval timeval;

	argv++;
	argc--;
	if (argc > 0 && strcmp(*argv, "-p") == 0) {
		pflag++;
		argv++;
		argc--;
	}
	if (argc <= 0) {
		fprintf(stderr, "Usage: fsirand [-p] special\n");
		exit(1);
	}
	dev = *argv;
	fd = open(dev, pflag ? 0 : 2);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s\n", dev);
		exit(1);
	}
	if (lseek(fd, SBLOCK * DEV_BSIZE, 0) == -1) {
		fprintf(stderr, "Seek to superblock failed\n");
		exit(1);
	}
	fs = (struct fs *) fsbuf;
	if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
		fprintf(stderr, "Read of superblock failed %d\n", errno);
		exit(1);
	}
	if (fs->fs_magic != FS_MAGIC) {
		fprintf(stderr, "Not a superblock\n");
		exit(1);
	}
	if (!pflag) {
		n = getpid();
		srandom(timeval.tv_sec + timeval.tv_usec + n);
		while (n--) {
			random();
		}
	}
	bsize = INOPB(fs) * sizeof (struct dinode);
	inum = 0;
	imax = fs->fs_ipg * fs->fs_ncg;
	while (inum < imax) {
		bno = itod(fs, inum);
		seekaddr = fsbtodb(fs, bno) * DEV_BSIZE;
		if (lseek(fd, seekaddr, 0) == -1) {
			fprintf(stderr, "lseek to %d failed\n", seekaddr);
			exit(1);
		}
		n = read(fd, (char *) dibuf, bsize);
		if (n != bsize) {
			printf("premature EOF\n");
			exit(1);
		}
		for (dip = dibuf; dip < &dibuf[INOPB(fs)]; dip++) {
			if (pflag) {
				printf("ino %d gen %x\n", inum, dip->di_gennum);
			} else {
				dip->di_gennum = random();
			}
			inum++;
		}
		if (!pflag) {
			if (lseek(fd, seekaddr, 0) == -1) {
				fprintf(stderr, "lseek to %d failed\n",
				    seekaddr);
				exit(1);
			}
			n = write(fd, (char *) dibuf, bsize);
			if (n != bsize) {
				printf("premature EOF\n");
				exit(1);
			}
		}
	}
	if (!pflag) {
		gettimeofday(&timeval, 0);
		if (lseek(fd, SBLOCK * DEV_BSIZE, 0) == -1) {
			fprintf(stderr, "Seek to superblock failed\n");
			exit(1);
		}
		if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
			fprintf(stderr, "Write of superblock failed %d\n",
			    errno);
			exit(1);
		}
	}
	for (n = 0; n < fs->fs_ncg; n++ ) {
		seekaddr = fsbtodb(fs, cgsblock(fs, n)) * DEV_BSIZE;
		if (lseek(fd,  seekaddr, 0) == -1) {
			fprintf(stderr, "Seek to alt superblock failed\n");
			exit(1);
		}
		if (pflag) {
			if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
				fprintf(stderr,
				    "Read of  alt superblock failed %d %d\n",
				    errno, seekaddr);
				exit(1);
			}
			if (fs->fs_magic != FS_MAGIC) {
				fprintf(stderr, "Not an alt superblock\n");
				exit(1);
			}
		} else {
			if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
				fprintf(stderr,
				    "Write of alt superblock failed\n");
				exit(1);
			}
		}
	}
}
