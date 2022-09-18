#ifndef lint
static	char	*sccsid = "@(#)umount.c	4.1	(ULTRIX)	7/2/90";
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
 * 12 Jul 88  -- chet
 *	Added -f (fast!) flag for nfs_umount
 *
 * 28-Apr-87 -- prs
 *	Fixed return status
 *
 ************************************************************************/
/*
 * umount
 */
#include <sys/param.h>

#include <stdio.h>
#include <fstab.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/fs_types.h>

int errflag,aflag,vflag,fflag;
int loc;
char *malloc(),*strcpy(),*strcat();
#define MSIZE (NMOUNT*sizeof(struct fs_data))

main(argc, argv)
	int argc;
	char **argv;
{
	register int c;
	char *progname;
	extern char *optarg;
	extern int optind;
	register int ret,rettemp;
	register struct fs_data *mountbuffer,*fs_data;
	register int notdone = 1;		/* didn't do unmount yet */
	char errmess[300];
	int	didnfs = 0;
	
	progname = argv[0];
	while ((c = getopt(argc,argv,"avf")) != EOF) {
		switch (c) {
		case 'a':
			aflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'f':
			fflag++;
			break;
		case '?':
			errflag++;
		}
	}
	/* give usage message if any bad flags are used or if there are */
	if (errflag || argc == 1) {
		fprintf(stderr, "Usage: %s [-a|-v] special\n", progname);
		exit(1);
	}
	/* get the mounted file systems */
	if((mountbuffer = (struct fs_data *) malloc(MSIZE)) == NULL) {
		perror("malloc");
		exit(1);
	}
	/* use this so that we don't hang if server's down with nfs file sys */
	ret = getmountent(&loc, mountbuffer, NMOUNT);
	if (ret == 0) {
		perror("getmountent");
		exit(3);
	}
	if (ret == 1)
		exit(0);	/* only root mounted */
	/* unmount in reverse order!!! */
	for (fs_data = &mountbuffer[ret-1], ret=0; fs_data >= &mountbuffer[1];
								fs_data--) {
		if (aflag || strcmp(fs_data->fd_devname,argv[optind]) == 0 ||
			     strcmp(fs_data->fd_path,argv[optind]) == 0 ) {
			rettemp = 0;
			notdone = 0;
			if(rettemp = umount(fs_data->fd_dev)) {
				(void) sprintf(errmess, "umount %s",
				fs_data->fd_devname);
				perror(errmess);
			}
			ret += rettemp;
			/* only NFS needs a cleanup routine */
			if (!rettemp && (fs_data->fd_fstype == GT_NFS)) {
				umountfs(fs_data);
				didnfs++;
			}
			if (vflag) {
				if (rettemp) {
					printf("Couldn't unmount %s\n",
							fs_data->fd_devname);
				}
				else {
					printf("Unmounted %s\n",
							fs_data->fd_devname);
				}
			if (!aflag) break;
			}
		}
	}
	if (!aflag && notdone) {
		fprintf(stderr,"Cannot find %s\n",argv[optind]);
	}

	/* If unmounting everything, and doing it `fast', and there was
	 * an NFS filesystem unmounted, then be nice to that server
	 * and let them know about it in a non-blocking manner.
	 */
	if (aflag && didnfs && fflag) {
		(void) system("/etc/nfs_umount -b");
	}

	exit(ret+notdone);
}

umountfs(fs_data)
	register struct fs_data *fs_data;
{
	char progname[BUFSIZ];
	(void) strcpy(progname, "/etc/");
	(void) strcat(progname,gt_names[fs_data->fd_fstype]);
	(void) strcat(progname,"_umount ");
	if (vflag) (void) strcat(progname,"-v ");
	if (fflag) (void) strcat(progname,"-f ");
	(void) strcat(progname,fs_data->fd_devname);
	(void) system(progname);
}
