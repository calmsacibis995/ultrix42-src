#ifndef lint
static char sccsid[] = "@(#)newfs.c	4.1	(ULTRIX)	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1989 by			*
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
 *
 *			Modification History
 *
 * 008- Tim Burke, 13-Sep-89
 *	Added dynamic disk support by calling "creatediskbyname" to determine
 *	the disktab entry if it can't be derived from the disktab file by
 *	calling getdiskbyname.
 *
 *	Paul Shaughnessy, 14-Jun-89
 *	Merged in 43 functionality.
 *
 * 006-	Jon Reeves, 09-Jun-89
 *	Clean up sprintf usage.
 *
 *    - Tom Tresvik, 16-Sep-88
 *	Changed the name of vaxboot to bootblks for multiple
 *	architectures.  This will be a gerneric bootblk name.
 *
 * 004-	Tom Tresvik, 20-Jan-86
 *	Changeover to VMB based Ultrix boot path.  Replaced boot image
 *	development code with default opening of `vaxboot'.  This 
 *	image is both the bootblock and the 1st level boot code.  The
 *	new image must be installed on all Ultrix disks.
 *
 *	Mike Gancarz, 22-Nov-84
 * 003- Added support for disk-resident partition table.
 *
 *	Stephen Reilly, 25-Jul-84
 * 002- Have code that will not try to copy a rbboot, since there is
 *	no such thing.  Also when copying over boot make sure that the
 *	file size is within reasonable ranges.
 *
 *	Stephen Reilly, 26-Jun-84
 * 001- Let the stat function determine if the device is mountable.
 *
 ***********************************************************************/

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <sys/dir.h>
#include <sys/file.h>

#include <stdio.h>
#include <disktab.h>

#define	BOOTDIR	"/usr/mdec"	/* directory for boot blocks */
#define sblock	fsun.fs

union {
	struct fs fs;
	char pad[MAXBSIZE];
} fsun;

struct pt *pt, partbl;

int	Nflag;			/* run mkfs without writing file system */
int	verbose;		/* show mkfs line before exec */
int	noboot;			/* do not fill boot blocks */
int	fssize;			/* file system size */
int	fsize;			/* fragment size */
int	bsize;			/* block size */
int	ntracks;		/* # tracks/cylinder */
int	nsectors;		/* # sectors/track */
int	sectorsize;		/* bytes/sector */
int	cpg;			/* cylinders/cylinder group */
int	minfree = -1;		/* free space threshold */
int	opt;			/* optimization preference (space or time) */
int	rpm;			/* revolutions/minute of drive */
int	density;		/* number of bytes per inode */
int	nowarn;			/* do not warn about missing 'a' part. tbls */
int	pt_exist;		/* partition table exists in the 'a' part. */

char	*av[20];		/* argv array and buffers for exec */
char	a2[20];
char	a3[20];
char	a4[20];
char	a5[20];
char	a6[20];
char	a7[20];
char	a8[20];
char	a9[20];
char	a10[20];
char	device[MAXPATHLEN];
char	cmd[BUFSIZ];

char	*index();
char	*rindex();
char	*sprintf();

main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp, *special;
	register struct disktab *dp;
	register struct partition *pp;
	struct stat st;
	register int i;
	int status;
	int fd;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {

			case 'v':
				verbose++;
				break;

			case 'N':
				Nflag++;
				/* fall through to */

			case 'n':
				noboot++;
				break;

			case 'w':
				nowarn++;
				break;

			case 's':
				if (argc < 1)
					fatal("-s: missing file system size");
				argc--, argv++;
				fssize = atoi(*argv);
				if (fssize < 0)
					fatal("%s: bad file system size",
						*argv);
				goto next;

			case 't':
				if (argc < 1)
					fatal("-t: missing track total");
				argc--, argv++;
				ntracks = atoi(*argv);
				if (ntracks < 0)
					fatal("%s: bad total tracks", *argv);
				goto next;

			case 'o':
				if (argc < 1)
					fatal("-o: missing optimization preference");
				argc--, argv++;
				if (strcmp(*argv, "space") == 0)
					opt = FS_OPTSPACE;
				else if (strcmp(*argv, "time") == 0)
					opt = FS_OPTTIME;
				else
					fatal("%s: bad optimization preference %s",
					    *argv,
					    "(options are `space' or `time')");
				goto next;

			case 'b':
				if (argc < 1)
					fatal("-b: missing block size");
				argc--, argv++;
				bsize = atoi(*argv);
				if (bsize < 0 || bsize < MINBSIZE)
					fatal("%s: bad block size", *argv);
				goto next;

			case 'f':
				if (argc < 1)
					fatal("-f: missing frag size");
				argc--, argv++;
				fsize = atoi(*argv);
				if (fsize < 0)
					fatal("%s: bad frag size", *argv);
				goto next;

			case 'S':
				if (argc < 1)
					fatal("-S: missing sector size");
				argc--, argv++;
				sectorsize = atoi(*argv);
				if (sectorsize < 0)
					fatal("%s: bad sector size", *argv);
				goto next;

			case 'c':
				if (argc < 1)
					fatal("-c: missing cylinders/group");
				argc--, argv++;
				cpg = atoi(*argv);
				if (cpg < 0)
					fatal("%s: bad cylinders/group", *argv);
				goto next;

			case 'm':
				if (argc < 1)
					fatal("-m: missing free space %%\n");
				argc--, argv++;
				minfree = atoi(*argv);
				if (minfree < 0 || minfree > 99)
					fatal("%s: bad free space %%\n",
						*argv);
				goto next;

			case 'r':
				if (argc < 1)
					fatal("-r: missing revs/minute\n");
				argc--, argv++;
				rpm = atoi(*argv);
				if (rpm < 0)
					fatal("%s: bad revs/minute\n", *argv);
				goto next;

			case 'i':
				if (argc < 1)
					fatal("-i: missing bytes per inode\n");
				argc--, argv++;
				density = atoi(*argv);
				if (density < 0)
					fatal("%s: bad bytes per inode\n",
						*argv);
				goto next;

			default:
				fatal("-%c: unknown flag", cp);
			}
next:
		argc--, argv++;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: newfs [-v] [-w] [mkfs-options] %s\n",
			"special-device device-type");
		fprintf(stderr, "where mkfs-options are:\n");
		fprintf(stderr, "\t-N do not create file system, %s\n",
			"just print out parameters");
		fprintf(stderr, "\t-s file system size (sectors)\n");
		fprintf(stderr, "\t-b block size\n");
		fprintf(stderr, "\t-f frag size\n");
		fprintf(stderr, "\t-t tracks/cylinder\n");
		fprintf(stderr, "\t-c cylinders/group\n");
		fprintf(stderr, "\t-m minimum free space %%\n");
		fprintf(stderr, "\t-o optimization preference %s\n",
			"(`space' or `time')");
		fprintf(stderr, "\t-r revolutions/minute\n");
		fprintf(stderr, "\t-S sector size\n");
		fprintf(stderr, "\t-i number of bytes per inode\n");
		exit(1);
	}
	special = argv[0];
	cp = rindex(special, '/');
	if (cp != 0)
		special = cp + 1;

/*	Let's not be smart and make the user specify the raw device
 *	
 *001	if (*special == 'r' && special[1] != 'a' && special[1] != 'b') 
 *001		special++;
 */
	(void) sprintf(device, "/dev/%s", special);
	special = device;
 
	if (stat(special, &st) < 0) {
		fprintf(stderr, "newfs: "); perror(special);
		exit(2);
	}
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a character device", special);
	/*
 	 * 008
	 * Get a disktab struct for the disk.  First call getdiskbyname to
	 * look for an entry in the disktab file if that fails call
	 * creatediskbyname to dynamically generate a disktab entry by
	 * polling the disk driver for the needed information.
	 */
	dp = getdiskbyname(argv[1]);
	if (dp == 0) {
		dp = creatediskbyname(special);
		if (dp == 0) {
			fatal("%s: unknown disk type", argv[1]);
		}
		if (verbose)
			printf("Using disktab entry from creatediskbyname.\n");
	}
	cp = index(argv[0], '\0') - 1;
	if (cp == 0 || *cp < 'a' || *cp > 'h')
		fatal("%s: can't figure out file system partition", argv[0]);
	pp = &dp->d_partitions[*cp - 'a'];

	/*
	 *	check for an existing partition table in the 'a' partition
	 */
	pt_exist = chkpart(special);
	if (!nowarn && (pt_exist < 0) && (*cp != 'a'))
		fprintf(stderr, "%s\n",
				"Warning: missing disk partition table");

	/*
	 *	if no part. table exists, attempt to get one from the driver
	 */
	if (pt_exist < 0) {
		if ((fd = open(special, O_RDONLY)) < 0) {
			fprintf(stderr, "newfs: %s: cannot open: ", special);
			perror("");
			exit(1);
		}
		if (ioctl(fd, DIOCGETPT, (char *)&partbl) < 0) {
			fprintf(stderr,
				"Warning: get partition table ioctl failed: ");
			perror("");
		} else {
			pt_exist = 1;
			pt = &partbl;
		}
	}

	if (fssize == 0) {
		if (pt_exist >= 0) {
			fssize = pt->pt_part[*cp - 'a'].pi_nblocks;
			if (fssize != pp->p_size)
				printf("%s%s\n",
					"Warning: partition table overriding ",
					"/etc/disktab");
		} else {
		fssize = pp->p_size;
		if (fssize < 0)
			fatal("%s: no default size for `%c' partition",
				argv[1], *cp);
		}
	}
	if (nsectors == 0) {
		nsectors = dp->d_nsectors;
		if (nsectors < 0)
			fatal("%s: no default #sectors/track", argv[1]);
	}
	if (ntracks == 0) {
		ntracks = dp->d_ntracks;
		if (ntracks < 0)
			fatal("%s: no default #tracks", argv[1]);
	}
	if (sectorsize == 0) {
		sectorsize = dp->d_secsize;
		if (sectorsize < 0)
			fatal("%s: no default sector size", argv[1]);
	}
	if (bsize == 0) {
		bsize = pp->p_bsize;
		if (bsize < 0)
			fatal("%s: no default block size for `%c' partition",
				argv[1], *cp);
	}
	if (fsize == 0) {
		fsize = pp->p_fsize;
		if (fsize < 0)
			fatal("%s: no default frag size for `%c' partition",
				argv[1], *cp);
	}
	if (rpm == 0) {
		rpm = dp->d_rpm;
		if (rpm < 0)
			fatal("%s: no default revolutions/minute value",
				argv[1]);
	}
	if (density <= 0)
		density = 2048;
	if (minfree < 0)
		minfree = 10;
	if (minfree < 10 && opt != FS_OPTSPACE) {
		fprintf(stderr, "setting optimization for space ");
		fprintf(stderr, "with minfree less than 10%\n");
		opt = FS_OPTSPACE;
	}
	if (cpg == 0)
		cpg = 16;
	i = 0;
	if (Nflag)
		av[i++] = "-N";
	av[i++] = special;
	av[i++] = sprintf(a2, "%d", fssize);
	av[i++] = sprintf(a3, "%d", nsectors);
	av[i++] = sprintf(a4, "%d", ntracks);
	av[i++] = sprintf(a5, "%d", bsize);
	av[i++] = sprintf(a6, "%d", fsize);
	av[i++] = sprintf(a7, "%d", cpg);
	av[i++] = sprintf(a8, "%d", minfree);
	av[i++] = sprintf(a9, "%d", rpm / 60);
	av[i++] = sprintf(a10, "%d", density);
	av[i++] = opt == FS_OPTSPACE ? "s" : "t";
	av[i++] = 0;
	strcpy(cmd, "/etc/mkfs");
	for (i = 0; av[i] != 0; i++) {
		strcat(cmd, " ");
		strcat(cmd, av[i]);
	}

	if (verbose)
		printf("%s\n", cmd);
	if (status = system(cmd))
		exit(status);
	if (*cp == 'a' && !noboot) {
		char type[3];
		struct stat sb;

		cp = rindex(special, '/');
		if (cp == NULL)
			fatal("%s: can't figure out disk type from name",
				special);
		if (stat(special, &sb) >= 0 && (sb.st_mode & S_IFMT) == S_IFCHR)
			cp++;
		type[0] = *++cp;
		type[1] = *++cp;
		type[2] = '\0';
		installboot(special, type);
	}
	exit(0);
}

/*
 *	Check whether a partition table exists in the 'a' partition for
 *	a given device
 */
chkpart(dev)
	char *dev;
{
	int fd;
	int cmp;
	char *cp;
	char pt_dev[MAXPATHLEN];

	strcpy(pt_dev, dev);
	cmp = strcmp(dev, pt_dev);
	cp = pt_dev + strlen(pt_dev) - 1;
	*cp = 'a';
	if ((fd = open(pt_dev, O_RDONLY)) < 0) {
		if (!nowarn)
		{
			fprintf(stderr, "%s: %s: %s: ",
				"newfs",
				pt_dev,
				"cannot open to read partition table");
			perror("");
		}
		return(-1);
	}
	lseek(fd, SBLOCK * DEV_BSIZE, L_SET);
	if (read(fd, (char *)&sblock, SBSIZE) != SBSIZE) {
		if (!nowarn)
		{
			fprintf(stderr,
				"newfs: %s: cannot read superblock: ",
				pt_dev);
			perror("");
		}
		return(-1);
	}
	close(fd);
	pt = (struct pt *)&fsun.pad[SBSIZE - sizeof(struct pt)];
	if (pt->pt_magic != PT_MAGIC)
	{
		if (!nowarn && cmp)
			fprintf(stderr,
				"newfs: %s: %s\n",
				pt_dev,
				"no partition table found on the disk");
		return(-1);
	}
	return(0);
}

installboot(dev, type)
	char *dev, *type;
{
	int fd;
	char standalonecode[MAXPATHLEN];
	char bootimage[BBSIZE];
	struct stat stat_buf;

	(void) sprintf(standalonecode, "%s/bootblks", BOOTDIR);		/*004*/
	if (verbose) {
		printf("installing boot code\n");
		printf("sector 0 boot + 1st level boot = %s\n", 	/*004*/
		    standalonecode);
	}
	close(fd);
	fd = open(standalonecode, 0);
	if (fd < 0) {
		fprintf(stderr, "newfs: "); perror(standalonecode);
		exit(1);
	}
	
	fstat(fd,&stat_buf);

	/*
	 *	Make sure that the file size is not larger than
	 *	BBSIZE.
	 */
	if ( stat_buf.st_size > ( BBSIZE) ) {				/*002*/
	    fprintf(stderr, "newfs: ");					/*002*/
	    fprintf(stderr," Standalone boot too large\n");		/*002*/
	    exit(1);							/*002*/
	}
	if (read(fd, bootimage, BBSIZE) < 0) {
		fprintf(stderr, "newfs: "); perror(standalonecode);
		exit(2);
	}
	close(fd);
	fd = open(dev, 1);
	if (fd < 0) {
		fprintf(stderr, "newfs: "); perror(dev);
		exit(1);
	}
	if (write(fd, bootimage, BBSIZE) != BBSIZE) {
		fprintf(stderr, "newfs: "); perror(dev);
		exit(2);
	}
	close(fd);
}

/*VARARGS*/
fatal(fmt, arg1, arg2)
	char *fmt;
{

	fprintf(stderr, "newfs: ");
	fprintf(stderr, fmt, arg1, arg2);
	putc('\n', stderr);
	exit(10);
}
