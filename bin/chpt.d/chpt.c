#ifndef lint
static char sccsid[] = "@(#)chpt.c	4.1	(ultrix)	7/2/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 by			*
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
 *	Paul Shaughnessy, 16-Dec-86
 * 005-	Added code that will open the device for read-only if the
 *	-q flag is specified.
 *
 *	Stephen Reilly, 24-Sep-85
 * 004- Added the -d switch, which will allow the user to set the disk
 *	partition tables back to the orginal size.
 *
 *	Stephen Reilly, 16-Apr-85
 * 003- Occasionally, when using the -q switch on a disk that does not
 *	have a valid superblock, the program will core dump.
 *	
 *	Stephen Reilly, 28-Jan-85
 * 002- Have the -q flag print out the partition table even if no superblock
 *	is present.
 *
 *	Stephen Reilly, 16-Jan-85
 * 001- Have -q flag print out the driver's table if it can't find the
 *	partition table in the superblock.
 *
 *	Mike Gancarz, 22-Nov-84
 * 000- Original version
 *
 ***********************************************************************/

/*
 * chpt	- change the partition tables on a disk
 *
 * usage: chpt [-a] [-q] [-v] [-d] [-px block-offset #blocks] device
 *  where:
 *	-a	Add partition tables to the disk if none are found
 *	-q	(query) run 'chpt' without writing new partition tables
 *		to the disk or setting them in the driver;implies -v
 *	-d	Add the paritition table to the disk with the default
 *		partition tables.
 *	-v	Print verbose messages showing what 'chpt' is doing
 *	-px	Override the driver's current partition values for
 *		partition 'x' with the specified block-offset and #blocks
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>

union {
	struct fs fs;
	char pad[MAXBSIZE];
} fsun;
#define	sblock	fsun.fs		/* used to access the s.b. as a file system */
#define sarray	fsun.pad	/* used to access the s.b. as a char array */

struct stat	st;		/* file status table */
struct pt	*newpt;		/* the new partition table to be written out */
struct
{
	int ovrlay[8];	/* these are set if part. was specified on cmd line */
	struct pt pt; /* partition values specified on the command line */
} av_pt;
#define av_part		av_pt.pt.pt_part[p]
#define new_part	newpt->pt_part[p]


char device[MAXPATHLEN];	/* full path to the special file */
char *part;			/* pointer to the device partition name */

int addpt;	/* if > 0, add part. tables to disk if none are found */
int query;	/* query mode */
int verbose;	/* verbose mode */
int dfpt;       /* 004 -d switch indicator */
int nopt;	/* if > 0, no partition table found in superblock */
int fd;		/* file descriptor */
int getcmd;	/* 004 type of ioctl request to use */

main(argc, argv)
int argc;
char *argv[];
{
	register int p;
	int q;
	register long i;
	long blk;

	/*
	 * must be superuser to run 'chpt'
	 */
	if (geteuid() != 0)
		fatal("must be superuser");

	/*
	 * process the argument list
	 */
	argc--, argv++;
	if (argc < 1)
		usage();

	while (argc > 0 && argv[0][0] == '-')
	{
		switch (argv[0][1])
		{
			case 'a':
				addpt++;
				break;

			case 'p':
				if (argv[0][2] < 'a' || argv[0][2] > 'h')
					fatal("bad switch: %s", *argv);
				p = argv[0][2] - 'a';

				--argc, ++argv;
				if (argc < 2)
					usage();
				if ((q = sscanf(*argv, "%d",
						&av_part.pi_blkoff)) != 1)
					usage();
				if (av_part.pi_blkoff < 0)
					fatal("negative block offset: %s",
						*argv);

				--argc, ++argv;
				if ((q = sscanf(*argv, "%ld",
						&av_part.pi_nblocks)) != 1)
					usage();
				if (av_part.pi_nblocks < 0)
					fatal("negative block count: %s",
						*argv);
				av_pt.ovrlay[p]++;
				break;

			case 'q':
				query++;
				/* FALLTHROUGH to case 'v' */

			case 'v':
				verbose++;
				break;

			case 'd':			/* 004 */
				dfpt++;
				break;
			default:
				fatal("bad switch: %s", *argv);
		}

		--argc, ++argv;
	}

	/*
	 * at this point, *argv points to the device (we hope)
	 */
	if (argc != 1)
		usage();
	strcpy(device, *argv);
	part = device + strlen(device) - 1;
	if ((*part != 'a') && (*part != 'c'))
		fatal("must specify `a' or `c' partition");

	/*
	 *	Which of the ioctl request will be used through the rest
	 *	of the program.
	 */
	if ( dfpt )
		getcmd = DIOCDGTPT;
	else
		getcmd = DIOCGETPT;
	/*
	 * let's be a pain and only allow the raw device
	 */
	if (stat(device, &st) < 0)
	{
		fprintf(stderr, "chpt: ");
		perror(device);
		exit(1);
	}
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a character device", device);

	/*
	 * read in the superblock from the named partition;
	 * complain if none exists
	 */
	if (verbose)
		printf("%s\n", device);
	/*
	 * 005 - If -q flag was specified, open device read-only.
	 */
	if (query)
		fd = open(device, O_RDONLY);
	else
		fd = open(device, O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "chpt: cannot open %s", device);
		perror(" ");
		exit(1);
	}
	doseek(SBLOCK);
	if (read(fd, (char *)&sblock, SBSIZE) != SBSIZE)
	{
		fprintf(stderr, "chpt: %s: cannot read superblock", device);
		perror(" ");
		exit(1);
	}

	/*
	 * validate the superblock just read in
	 */
	if (sblock.fs_magic != FS_MAGIC && !query)
	       fatal("%s: blk %ld: bad superblock -- run fsck", device, SBLOCK);

	/*
	 * see if the superblock already has a partition table
	 */
	newpt = (struct pt *)&sarray[SBSIZE - sizeof(struct pt)];
	if (newpt->pt_magic != PT_MAGIC)
	{
		if (!addpt && !dfpt && !query)
			fatal("%s: no partition table in superblock", device);

		nopt++;
		if (verbose)
			printf("%s\n%s\n",
				"No partition table found in superblock...",
				"using default table from device driver.");
	}

	/*
	 * if the superblock has no partition table or are we using the
	 * the default table built into the driver ...
	 */
	if (nopt || dfpt)
	{
		/*
		 * ...then obtain a partition table from the driver and...
		 */
		if (ioctl(fd, getcmd, (char *)newpt) < 0)
		{
			perror("chpt: can't get partition table from driver");
			exit(1);
		}

		/*
		 * If the suprerblock is bad then skip the following
		 * code because we can core dump because of the divide
		 * instruction.
		 */
		if ( sblock.fs_magic != FS_MAGIC )		/* 001 */
			goto out;				/* 001 */

		/*
		 * ...warn the user if a new partition table would
		 * overwrite the rotational delay table in the driver
		 */
		sblock.fs_sbsize = SBSIZE; /* part. tbl. now part of size */
		blk = sblock.fs_spc * sblock.fs_cpc / NSPF(&sblock);
		for (i = 0; i < blk; i += sblock.fs_frag)
			/* void */;
		if ((struct pt *)(&sblock.fs_rotbl[(i - sblock.fs_frag) /
			sblock.fs_frag]) >= newpt)
		{
			printf("%s\n%s\n%s\n",
			  "Warning: new partition table overwriting existing",
			  "         rotational delay table. Filesystem",
			  "         performance may be impaired.");
			sblock.fs_cpc = 0;
		}
	}
out:

	/*
	 * overlay the partition table in the superblock with any values
	 * that the user may have specified on the command line; show
	 * what the new partition table looks like
	 */
	ptoverlay();
	if (verbose)
		report();

	/*
	 * set the new partition table in the driver via ioctl(2)
	 * and write the updated superblock back out to disk
	 */
	newpt->pt_magic = PT_MAGIC;
	newpt->pt_valid = PT_VALID;
	if (!query)
	{
		if (ioctl(fd, DIOCSETPT, (char *)newpt) < 0)
		{
			perror("chpt: cannot set partition table in driver");
			exit(1);
		}
		writefs();
	}
	exit(0);
}

/*
 * overlay the partition table in the superblock
 */
ptoverlay()
{
	register int p;

	for (p = 0; p < 8; p++)
	{
		if (av_pt.ovrlay[p])
		{
			new_part.pi_nblocks = av_part.pi_nblocks;
			new_part.pi_blkoff = av_part.pi_blkoff;
		}
	}
}

/*
 * report on overlapping partitions in the new partition table, regardless
 * of their mount status
 */
report()
{
	register int p, np;
	register daddr_t bot[8], top[8];
	int flag;

	for (p = 0; p < 8; p++)
	{
		bot[p] = new_part.pi_blkoff;
		top[p] = bot[p] + new_part.pi_nblocks;
		if (top[p] >= 2)
			top[p]--;
	}
	printf("%s partition table:\n", query ? "Current" : "New");
	printf("%-10s%10s %10s %10s %s\n",
		"partition", "bottom", "top", "size", "   overlap");
	for (p = 0; p < 8; p++)
	{
		printf("    %c     %10ld %10ld %10ld    ",
			'a' + p, bot[p], top[p], new_part.pi_nblocks);
		flag = 0;
		for (np = 0; np < 8; np++)
		{
			if (p == np)
				continue;
			if ((bot[p] >= bot[np] && bot[p] <= top[np]) ||
			    (top[p] >= bot[np] && top[p] <= top[np]) ||
			    (bot[np] >= bot[p] && bot[np] <= top[p]))
			{
				printf("%s%c", flag ? "," : "", 'a' + np);
				flag++;
			}
		}
		putchar('\n');
	}
}

/*
 * re-write the superblocks on the disk, trashing each one as we go
 * (jes' kiddin') 
 */
writefs()
{
	long cylno;

	wrblk(SBLOCK, SBSIZE, (char *)&sblock);

	for (cylno = 0; cylno < sblock.fs_ncg; cylno++)
		wrblk(fsbtodb(&sblock, cgsblock(&sblock, cylno)),
		      SBSIZE, (char *)&sblock);
}

/*
 * seek to your favorite block on the disk
 */
doseek(blkno)
daddr_t blkno;
{
	if (lseek(fd, blkno * DEV_BSIZE, 0) < 0)
	{
		fprintf(stderr, "chpt: seek error: %ld", blkno * DEV_BSIZE);
		perror(" ");
		exit(1);
	}
}

/*
 * write a block to the disk
 */
wrblk(blkno, size, buf)
daddr_t blkno;
int size;
char *buf;
{
	int n;

	doseek(blkno);
	n = write(fd, buf, size);
	if (n != size)
	{
		fprintf(stderr, "chpt: write error: %ld", blkno);
		perror(" ");
		exit(1);
	}
}

/*VARARGS*/
/*
 * print an error message on stderr and exit
 */
fatal(fmt, arg1, arg2)
char *fmt;
{
	fprintf(stderr, "chpt: ");
	fprintf(stderr, fmt, arg1, arg2);
	putc('\n', stderr);
	exit(1);
}

/*
 * print a "usage" message and exit
 */
usage()
{
	printf("usage: chpt [-a] [-q] [-v] [-d] [-px blk-offset size] device\n");
	exit(1);
}
