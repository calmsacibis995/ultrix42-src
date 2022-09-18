#ifndef lint
static char *sccsid = "@(#)main.c	4.1	ULTRIX	7/2/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88 by			*
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
 * 22-Sep-89 -- prs
 *	Checked validity of fs_deftimer before initializing
 *	clean byte timeout factor.
 *
 *************************************************************************/

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/devio.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <fstab.h>
#include <strings.h>
#include "fsck.h"

char	*rawname(), *unrawname(), *blockcheck();
void	catch(), catchquit(), voidquit();
int	returntosingle;

#ifndef NUMRETRIES
#define NUMRETRIES 3
#endif

main(argc, argv)
	int	argc;
	char	*argv[];
{
	struct fstab *fsp;
	int pid, passno, anygtr, sumstatus, dufus;
	char *name;

	sync();
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {

		case 'p':
			only_when_needed++;
		case 'P':
			preen++;
			break;

		case 'b':
			if (argv[0][1] != '\0') {
				bflag = atoi(argv[0]+1);
			} else {
				bflag = atoi(*++argv);
				argc--;
			}
			if (bflag == 0)
				errexit("Alternate super block location invalid\n");
			printf("Alternate super block location: %d\n", bflag);
			break;

		case 'd':
			debug++;
			break;

		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		default:
			errexit("%c option?\n", **argv);
		}
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);
	fs_data = (struct fs_data *) malloc(MSIZE);
	if (fs_data == NULL) {
		printf("cannot alloc %d bytes for fs_data\n", MSIZE);
		exit(1);
	}
	nmountedfs = getmountent(&dufus, fs_data, NMOUNT);
	if (argc) {
		while (argc-- > 0) {
			hotroot = 0;
			checkfilesys(*argv++);
		}
		exit(0);
	}
	sumstatus = 0;
	passno = 1;
	do {
		anygtr = 0;
		if (setfsent() == 0)
			errexit("Can't open checklist file: %s\n", FSTAB);
		while ((fsp = getfsent()) != 0) {
			if (strcmp(fsp->fs_type, FSTAB_RW) &&
			    strcmp(fsp->fs_type, FSTAB_RO) &&
			    strcmp(fsp->fs_type, FSTAB_RQ))
				continue;
			/*
			 * 005 - If fs_type is NOT read-write, nor read-only,
			 * nor read-quota, get next entry.
			 */
			if (strcmp(fsp->fs_name,"ufs"))
				continue;
			if (preen == 0 ||
			    passno == 1 && fsp->fs_passno == passno) {
				name = blockcheck(fsp->fs_spec);
				if (name != NULL)
					checkfilesys(name);
				else if (preen)
					exit(8);
			} else if (fsp->fs_passno > passno) {
				anygtr = 1;
			} else if (fsp->fs_passno == passno) {
				pid = fork();
				if (pid < 0) {
					perror("fork");
					exit(8);
				}
				if (pid == 0) {
					(void)signal(SIGQUIT, voidquit);
					name = blockcheck(fsp->fs_spec);
					if (name == NULL)
						exit(8);
					checkfilesys(name);
					exit(0);
				}
			}
		}
		if (preen) {
			union wait status;
			while (wait(&status) != -1)
				sumstatus |= status.w_retcode;
		}
		passno++;
	} while (anygtr);
	if (sumstatus)
		exit(8);
	(void)endfsent();
	if (returntosingle)
		exit(2);
	exit(0);
}

checkfilesys(filesys)
	char *filesys;
{
	daddr_t n_ffree, n_bfree;
	struct dups *dp;
	struct zlncnt *zlnp;

	struct stat sb, stat_raw;
	struct pt *pt, partbl;
	register struct fs_data *fs_dp = fs_data;
	struct devget devinfo;
	char mess[100];
	char *raw;
	int gids[NGROUPS];		/* 005 - Groups user in */
	int ngids = NGROUPS;		/* 005 - Number of groups user is in */
	int *gidp = gids;
	int val_user;			/* 005 - valid for user level fsck */
	int fso;			/* 005 - file descriptor */
	int nodrvpt;			/* 005 - partition exists ? */
	int bot, top, mbot, mtop;	/* 005 - partition values */
	int retries = NUMRETRIES;	/* 003 - Number of retries for a clean
						 fsck pass */
	int rootdirty = 0;		/* 003 - Root was or is dirty */
	char block_device[MAXPATHLEN];	/* Array for block dev name */
	char *bdev = block_device;	/* and the pointer */
	int chk_raw_dev = 0;		/* User specified raw dev */
	int fsdirty = 0;		/* File system was or is dirty */

	if(stat(filesys, &sb)) {
		sprintf(mess, "cannot stat %s\n", filesys);
		pfatal(mess);
		return;
	}
	/*
	 * 005 - If we are not root, make sure
	 * device has execute permission set.
	 */
#define OTHER_EXEC 0000001
#define GROUP_EXEC 0000010
	if (getuid()) {
		val_user = 0;
	    	if (sb.st_mode & OTHER_EXEC)
			val_user++;
	    	if (sb.st_mode & GROUP_EXEC) {
			ngids = getgroups(ngids, gidp);
			for (; gidp < &gids[ngids]; gidp++)
				if (sb.st_gid == *gidp)
					val_user++;
		}
		if (!val_user) {
			sprintf(mess, "no execute permission on %s\n", filesys);
			pfatal(mess);
			return;
		}
	}
	if ((sb.st_mode & GFMT) == S_IFBLK)
		raw = rawname(filesys);
	else {
	       /*
		* We must stat the corresponding block device here. Further
		* down in this routine, we check the mount table against
		* the rdev field of the stat buffer (sb), to determine if
		* the device is mounted. If we don't stat the block device
		* here, then this check will not work when the major numbers
		* of the block and character devices do not match.
		*/
		bzero(bdev, MAXPATHLEN);
		strcpy(bdev, filesys);
		bdev = unrawname(bdev);
		if (stat(bdev, &sb)) {
			sprintf(mess, "cannot stat %s\n", bdev);
			pfatal(mess);
			return;
		}
		chk_raw_dev = 1;
		raw = filesys;
	}
	if ((sb.st_mode & GFMT) != S_IFBLK) {
		sprintf(mess, "%s not a block device\n", bdev);
		pfatal(mess);
		return;
	}
	fso = open(raw, O_RDONLY, 0);
	if (fso < 0) {
	        if (errno == ENOENT) {
		        sprintf(mess, "character device must exist for %s\n",
				filesys);
			pfatal(mess);
			return;
		}
		sprintf(mess,"couldn't open character device for %s\n",
			filesys);
		pfatal(mess);
		return;
	}
	stat_raw.st_mode = 0;
	if (stat(raw, &stat_raw)) {
	        sprintf(mess, "cannot stat %s\n", filesys);
		pfatal(mess);
		close(fso);
		return;
        }
	if ((stat_raw.st_mode & GFMT) != S_IFCHR) {
	        sprintf(mess, "%s not a character device\n", raw);
		pfatal(mess);
		close(fso);
		return;
	}

	/*
	 * If device is write locked, set nflag so no writes will
	 * be attempted on the device. ioctl's can only have a
	 * character device as the file descriptor.
	 */


	devinfo.stat = 0;
	if (ioctl(fso, DEVIOCGET, (char *)&devinfo) >= 0)
	        if (devinfo.stat & DEV_WRTLCK)
		        nflag = 1;

	/*
	 * 005 - Get partition table info.
	 */

	nodrvpt = 0;
	if (ioctl(fso, DIOCGETPT, (char *)&partbl) < 0)
		nodrvpt++;

	close(fso);
	devname = filesys;
	for(; fs_dp < &fs_data[nmountedfs]; fs_dp++) {
		if(sb.st_rdev == fs_dp->fd_dev) {
			/*
			 * 005 - If the user is not the super user, do not
			 * allow fsck's on a mounted file system.
			 */
 			if (getuid()) {
				sprintf(mess, "%s mounted: cannot fsck\n",
				fs_dp->fd_devname);
				pfatal(mess);
				return;
			}
			/*
			 * 005 - If the user is the super user, then force
			 * them to fsck the block device.
			 */
 			if (chk_raw_dev) {
				sprintf(mess, "%s mounted: use block device\n",
				fs_dp->fd_devname);
				pfatal(mess);
				return;
			}
		}
		/*
		 * 005 - If the user is not the super user, verify that the
		 * file system about to be checked does not overlap
		 * with a mounted file system.
		 */
		if (getuid()) {		/* Not super user */
			if (nodrvpt) {
				sprintf(mess, "cannot get partition table from driver\n");
				pfatal(mess);
				return;
			}
#define UNIT(dev) (minor(dev) / 8)
#define PART(dev) (minor(dev) % 8)
			/*
			 * 005 - Check for overlap
			 */
			if (major(sb.st_rdev) == major(fs_dp->fd_dev) &&
			    UNIT(sb.st_rdev) == UNIT(fs_dp->fd_dev)) {
				bot = partbl.pt_part[PART(sb.st_rdev)].pi_blkoff;
				top = bot + partbl.pt_part[PART(sb.st_rdev)].pi_nblocks-1;
				mbot = partbl.pt_part[PART(fs_dp->fd_dev)].pi_blkoff;
				mtop = mbot + partbl.pt_part[PART(fs_dp->fd_dev)].pi_nblocks-1;

				if ((bot >= mbot && bot <= mtop) ||
			      	    (top >= mbot && top <= mtop) ||
			      	    (mbot >= bot && mbot <= top)) {
			  		sprintf(mess, "%s would overlap mounted filesystem\n", filesys);
					pfatal(mess);
					return;
				}
			}
		}
	}
	for (; retries > 0; retries--) {	/* 003 - Loop for NUMRETRIES or
							 until clean */
		devname = filesys;
		switch (setup(filesys)) {
		      case 0:
			      if (preen)
				      pfatal("CAN'T CHECK FILE SYSTEM.");
		      /* fall through */
		      case FS_CLEAN:
			      return;
		      case 1:
			      break;
		      default:
			      errexit("Bad return from setup\n");
			      exit(2);
		}	
		/*
		 * 1: scan inodes tallying blocks used
		 */
		if (preen == 0) {
			printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
			if (hotroot)
				printf("** Root file system\n");
			printf("** Phase 1 - Check Blocks and Sizes\n");
		}
		pass1();

		/*
		 * 1b: locate first references to duplicates, if any
		 */
		if (duplist) {
			if (preen)
				pfatal("INTERNAL ERROR: dups with -p");
			printf("** Phase 1b - Rescan For More DUPS\n");
			pass1b();
		}

		/*
		 * 2: traverse directories from root to mark all connected directories
		 */	
		if (preen == 0)
			printf("** Phase 2 - Check Pathnames\n");
		pass2();

		/*
		 * 3: scan inodes looking for disconnected directories
		 */
		if (preen == 0)
			printf("** Phase 3 - Check Connectivity\n");
		pass3();

		/*
		 * 4: scan inodes looking for disconnected files; check reference counts
		 */
		if (preen == 0)
			printf("** Phase 4 - Check Reference Counts\n");
		pass4();

		/*
		 * 5: check and repair resource counts in cylinder groups
		 */
		if (preen == 0)
			printf("** Phase 5 - Check Cyl groups\n");
		pass5();

		if ((dfile.mod) || (sblk.b_dirty) || (fileblk.b_dirty) || 
		    (inoblk.b_dirty) || (cgblk.b_dirty)) {
			fsdirty = 1;
			if (hotroot)
				rootdirty = 1;
			if (retries <= 1) {
				/*
				 * 003 - Number of retry attempts have been
				 * exhausted. Mark super block as dirty.
				 */
			     (void)time(&sblock.fs_time);
			     sbdirty();
			}
		} else {
			/*
			 * 003 - If checking root file system, and it was
			 * modified during any pass, don't assume it is
			 * ok. Must reboot.
			 */
			if (rootdirty) {
				sbdirty();
				retries = 0;
			}
			else {
				if ((!hotroot) && (!bflag) && (!nflag) && (!num_errs)) {
					sblock.fs_fmod = 0;
					sblock.fs_clean = FS_CLEAN;
					(void)time(&sblock.fs_time);
					(void)time(&sblock.fs_lastfsck);
					if ((sblock.fs_deftimer) &&
					    (sblock.fs_deftimer > 0) &&
					    (sblock.fs_deftimer < 255))
						sblock.fs_cleantimer =
						   sblock.fs_deftimer;
					else
						sblock.fs_cleantimer = 
						sblock.fs_deftimer =
						   FSCLEAN_TIMEOUTFACTOR;
					sbdirty();
				}
				/*
				 * 006 - If an alternate super block was used,
				 * we want to re fsck the partition after 
				 * updating the primary super block.
				 */
				if (!bflag)
					retries = 0;
			}
		}
		/*
		 * 006 - Unless no updates are to be done, write out maps.
		 */

		if (nflag)
			retries = 0;
		else
			ckfini();

		if (debug) {
			daddr_t nn_files = n_files;
			daddr_t nn_blks = n_blks;

			n_ffree = sblock.fs_cstotal.cs_nffree;
			n_bfree = sblock.fs_cstotal.cs_nbfree;
			if (nn_files -= imax - ROOTINO - sblock.fs_cstotal.cs_nifree)
				printf("%d files missing\n", nn_files);
			nn_blks += sblock.fs_ncg *
				(cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
			nn_blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
			nn_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
			if (nn_blks -= fmax - (n_ffree + sblock.fs_frag * n_bfree))
				printf("%d blocks missing\n", nn_blks);
		}
		if (duplist != NULL) {
			if (debug)
				printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next) {
				if (debug)
					printf(" %d,", dp->dup);
				free(dp);
			}
			if (debug)
				printf("\n");
		}
		if (zlnhead != NULL) {
			if (debug)
				printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next) {
				if (debug)
					printf(" %d,", zlnp->zlncnt);
				free(zlnp);
			}
			if (debug)
				printf("\n");
		}
		zlnhead = (struct zlncnt *)0;
		duplist = (struct dups *)0;

		free(blockmap);
		free(statemap);
		free((char *)lncntp);
		/*
		 * 003 - Print out retry message, and fsck file system
		 *	 again.
		 */
		if (retries > 1)
		   if (preen)		
		 	printf("%s: FILE SYSTEM MODIFIED, VERIFYING\n",filesys);
		   else
		 	printf("**** FILE SYSTEM MODIFIED, VERIFYING\n");
	} /* for retries */

	/*
	 * print out summary statistics
	 */
	n_ffree = sblock.fs_cstotal.cs_nffree;
	n_bfree = sblock.fs_cstotal.cs_nbfree;
	if (preen)
		printf("%s: ", devname);
	printf("%d files, %d used, %d free ",
	    n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
	printf("(%d frags, %d blocks, %.1f%% fragmentation)\n",
	    n_ffree, n_bfree, (float)(n_ffree * 100) / sblock.fs_dsize);

	if ((!fsdirty) && (!rootdirty))
		return;
	if (!preen) {
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
		if (hotroot)
			printf("\n***** HALT PROCESSOR WITHOUT SYNCING DISK *****\n");
	}
	if (hotroot) {
		sync();
		exit(4);
	}
}

char *
blockcheck(name)
	char *name;
{
	struct stat stslash, stblock, stchar;
	char *raw;
	int looped = 0;

	hotroot = 0;
	if (stat("/", &stslash) < 0){
		printf("Can't stat root\n");
		return (0);
	}
retry:
	if (stat(name, &stblock) < 0){
		printf("Can't stat %s\n", name);
		return (0);
	}
	if (stblock.st_mode & S_IFBLK) {
		raw = rawname(name);
		if (stat(raw, &stchar) < 0){
			printf("Can't stat %s\n", raw);
			return (0);
		}
		if (stchar.st_mode & S_IFCHR) {
			if (stslash.st_dev == stblock.st_rdev) {
				hotroot++;
				raw = unrawname(name);
			}
			return (raw);
		} else {
			printf("%s is not a character device\n", raw);
			return (0);
		}
	} else if (stblock.st_mode & S_IFCHR) {
		if (looped) {
			printf("Can't make sense out of name %s\n", name);
			return (0);
		}
		name = unrawname(name);
		looped++;
		goto retry;
	}
	printf("Can't make sense out of name %s\n", name);
	return (0);
}

char *
unrawname(cp)
	char *cp;
{
	char *dp = rindex(cp, '/');
	struct stat stb;

	if (dp == 0)
		return (cp);
	if (stat(cp, &stb) < 0)
		return (cp);
	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);
	if (*(dp+1) != 'r')
		return (cp);
	(void)strcpy(dp+1, dp+2);
	return (cp);
}

char *
rawname(cp)
	char *cp;
{
	static char rawbuf[32];
	char *dp = rindex(cp, '/');

	if (dp == 0)
		return (0);
	*dp = 0;
	(void)strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);
	return (rawbuf);
}
