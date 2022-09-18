#ifndef lint
static	char *sccsid = "@(#)df.c	4.2	(ULTRIX)	9/7/90";
#endif

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
/************************************************************************
 *			Modification History
 *
 * 06-Aug-90 -- lebel
 *	Added -l option for reporting locally mounted disks only.
 *
 * 08-Dec-89 -- prs
 *	Merged in V3.1 big fixes.
 *
 * 24-Jun-88 -- prs
 *	Fixed a bug that would display the root file systems data,
 *	when root was mounted on (0,0), and df [mounted dir] would 
 *	be entered as the command.
 *
 ************************************************************************/

#include <sys/param.h>
#include <sys/mount.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ustat.h>
#include <strings.h>
#include <sys/fs_types.h>

/*
 * df
 */
struct	fs_data *mountbuffer;
#define MSIZE (NMOUNT*sizeof(struct fs_data))

#define NUMTITLES 9
char *title1[NUMTITLES] = {
	"Filesystem","Total","kbytes","kbytes","%","inodes","inodes","%",""};
char *title2[NUMTITLES] = {
	"node","kbytes","used","free","used","used","free","used","Mounted on"};
int fields[2*NUMTITLES] = { -12,12,-8,8,-8,8,-8,8,-6,6,-8,8,-8,8,-4,4,-8,8};
int num_fields[2*NUMTITLES] = { -12,12,8,8,8,8,8,8,6,0,8,8,8,8,6,0,-8,8};
#define MOUNTED_ON 8
	
int	lflag;
int	iflag;
int	nflag;

char	*strcpy();

main(argc, argv)
	int argc;
	char **argv;
{
	register int i,used,ret,did_one;
	register struct	fs_data *fd;
	struct stat sbuf;
	register struct stat *sbp = &sbuf;
	char temp[BUFSIZ];
	int loc;
	char *malloc();
	
	while (argc > 1 && argv[1][0]=='-') {
		switch (argv[1][1]) {

		case 'i':
			iflag++;
			break;

		case 'n':
			nflag++;
			break;

		case 'l':
			lflag++;
			break;

		default:
			fprintf(stderr, "usage: df [-i] [-l] [-n] [filsys...]\n");
			exit(1);
		}
		argc--, argv++;
	}
	mountbuffer = (struct fs_data *) malloc(MSIZE);

	if (mountbuffer == NULL) {
		perror("malloc");
		exit(1);
	}

	if ((nflag) || (argc > 1))
		ret = getmnt(&loc,mountbuffer,MSIZE,NOSTAT_MANY,0);
	else
		ret = getmnt(&loc,mountbuffer,MSIZE,STAT_MANY,0);

	if (ret < 0) {
		perror("getmnt");
		fprintf(stderr, "%s: cannot get mount info\n",argv[0]);
		exit(1);
	}
	
	if (!iflag && argc <= 1) {
		/* find largest first name */
		int big = 0,i;
		for (fd=mountbuffer; fd < &mountbuffer[ret]; fd++) {
			i = strlen(fd->fd_devname);
			if (i> big) big=i;
		}
		if (big > fields[1]) {
			fields[0] = -big;
			fields[1] = big;
			num_fields[0] = -big;
			num_fields[1] = big;
		}
	}

	if (argc <= 1) {
		/* print them all */
		for (fd=mountbuffer; fd < &mountbuffer[ret]; fd++) {
			if ( !lflag || (fd->fd_fstype != GT_NFS))
				print_df(fd);
		}
		exit(0);
	}
	for (i=1; i<argc; i++) {
	
		if (stat(argv[i], sbp) == -1) {
			sprintf(temp, "cannot stat %s ", argv[i]);
			perror (temp);
			exit(1);
		}
		did_one = 0;
		/* try rdev for special file */
		if (((sbp->st_mode & S_IFMT) == S_IFBLK) || 
		    ((sbp->st_mode & S_IFMT) == S_IFCHR)) {
		        for (fd=mountbuffer; fd < &mountbuffer[ret]; fd++) {
			        if (fd->fd_dev == sbp->st_rdev) {
				        if (!nflag)
					      getmnt(&loc,fd,MSIZE,STAT_ONE,fd->fd_path);
				        print_df(fd);
					did_one = 1;
					break;
			        }
		        }
		}
		if (!did_one) {	/* try dev for regular file */
			for (fd=mountbuffer; fd < &mountbuffer[ret]; fd++) {
				if (fd->fd_dev == sbp->st_dev) {
					if (!nflag)
						getmnt(&loc,fd,MSIZE,STAT_ONE,fd->fd_path);
					print_df(fd);
					did_one = 1;
					break;
				}
			}
		}
		if (!did_one) {
			fprintf(stderr,"Can't find %s in mount table\n",
								argv[i]);
			exit(2);
		}
	}
	exit(0);
}

print_df(fd)
	register struct fs_data *fd;
{
	register int used;
	static int titles_printed = 0;

	if (!titles_printed) {
		titles_printed++;
		print_titles();
	}

	used = fd->fd_btot - fd->fd_bfree;
	printf("%*.*s",num_fields[0],num_fields[1], fd->fd_devname);
	printf("%*d%*d%*d",
		num_fields[2],fd->fd_btot,
		num_fields[4],used,
		num_fields[6],(int)fd->fd_bfreen > 0 ? fd->fd_bfreen : 0);
	printf("%*.*f%%", num_fields[8],num_fields[9],
		fd->fd_btot == 0 ? 0.0 : used /
		(double)(fd->fd_btot - (fd->fd_bfree - fd->fd_bfreen)) * 100.0);
	if (iflag) {
		used = fd->fd_gtot - fd->fd_gfree;
		printf(" %*ld%*ld%*.*f%%",
			num_fields[10],used,
			num_fields[12],fd->fd_gfree,
			num_fields[14],num_fields[15],fd->fd_gtot == 0
				? 0.0 : used / (double)fd->fd_gtot * 100.0);
	}
	if (iflag) printf("  %s\n", fd->fd_path);
	else printf("   %s\n", fd->fd_path);
}

print_titles()
{
	/*
	 * print top part of title.
	 */
	printf("%*.*s %*.*s %*.*s %*.*s %*.*s",
	       fields[0],fields[1],title1[0],
	       fields[2],fields[3],title1[1],
	       fields[4],fields[5],title1[2],
	       fields[6],fields[7],title1[3],
	       fields[8],fields[9],title1[4]);
	if (iflag) {	
		printf("%*.*s %*.*s %*.*s %*.*s",
		       fields[10],fields[11],title1[5],
		       fields[12],fields[13],title1[6],
		       fields[14],fields[15],title1[7],
		       fields[16],fields[17],title1[8]);
	}
	printf("\n");
	/*
	 * print second part of titles.
	 */
	printf("%*.*s %*.*s %*.*s %*.*s %*.*s",
	       fields[0],fields[1],title2[0],
	       fields[2],fields[3],title2[1],
	       fields[4],fields[5],title2[2],
	       fields[6],fields[7],title2[3],
	       fields[8],fields[9],title2[4]);
	if (iflag) {
		printf("%*.*s %*.*s %*.*s %s\n",
		       fields[10],fields[11],title2[5],
		       fields[12],fields[13],title2[6],
		       fields[14],fields[15],title2[7],
		       title2[8]);
	}
	else
		printf("%s\n",title2[MOUNTED_ON]);
}

