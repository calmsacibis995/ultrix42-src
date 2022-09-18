#ifndef lint
static char sccsid[] = "@(#)quotacheck.c	4.2		2/12/91";
#endif

/*
 * Fix up / report on disc quotas & usage
 */
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
/************************************************************************
 *
 *			Modification History
 *
 * dws -- 11-Feb-91
 *	Cleaned up code to read from character instead of block device.
 *	Fixed bu in handling of inode table.
 *
 *	Paul Shaughnessy, 14-Jul-87
 * 002- Added logic to zero out quota file entry for a user that
 *	no longer has any files.
 *
 *	Stephen Reilly,	28-Nov-84
 * 001- A the switch -f that will force quotacheck to create a quotas
 *	file.
 *
 *	4.4 (Berkeley, Melbourne) 6/22/83 
 ***********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/quota.h>
#include <sys/stat.h>
#include <fstab.h>
#include <pwd.h>

union {
	struct	fs	sblk;
	char	dummy[MAXBSIZE];
} un;
#define	sblock	un.sblk

struct	dinode	itab[MAXIPG];
struct	dinode	*dp;
long	blocks;
dev_t	dev;

#define LOGINNAMESIZE 8
struct fileusage {
	struct fileusage *fu_next;
	struct dqusage fu_usage;
	u_short	fu_uid;
	char fu_name[LOGINNAMESIZE + 1];
};
#define FUHASH 997
struct fileusage *fuhead[FUHASH];
struct fileusage *lookup();
struct fileusage *adduid();
int highuid;

int fi;
ino_t ino;
long done;
struct	passwd	*getpwent();
struct	dinode	*ginode();
char *malloc(), *makerawname();

int	vflag;		/* verbose */
int	aflag;		/* all file systems */
int	fflag;		/* 001 force us to create the quotas file */

char *qfname = "quotas";
char quotafile[MAXPATHLEN + 1];
struct dqblk zerodqbuf;

main(argc, argv)
	int argc;
	char **argv;
{
	register struct fstab *fs;
	register struct fileusage *fup;
	register struct passwd *pw;
	int i, errs = 0;

again:
	argc--, argv++;
	if (argc > 0 && strcmp(*argv, "-v") == 0) {
		vflag++;
		goto again;
	}
	if (argc > 0 && strcmp(*argv, "-a") == 0) {
		aflag++;
		goto again;
	}
	if (argc > 0 && strcmp(*argv, "-f") == 0) {		/* 001 */
		fflag++;					/* 001 */
		goto again;					/* 001 */
	}
	if (argc <= 0 && !aflag) {
		fprintf(stderr, "Usage:\n\t%s\n\t%s\n",
			"quotacheck [-v] -a",
			"quotacheck [-v] filesys ...");
		exit(1);
	}
	if (vflag) {
		setpwent();
		while ((pw = getpwent()) != 0) {
			fup = lookup(pw->pw_uid);
			if (fup == 0)
				fup = adduid(pw->pw_uid);
			strncpy(fup->fu_name, pw->pw_name,
				sizeof(fup->fu_name));
		}
		endpwent();
	}
	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (aflag &&
		    (fs->fs_type == 0 || strcmp(fs->fs_type, "rq") != 0))
			continue;
		if (!aflag &&
		    !(oneof(fs->fs_file, argv, argc) ||
		      oneof(fs->fs_spec, argv, argc)))
			continue;
		(void) sprintf(quotafile, "%s/%s", fs->fs_file, qfname);
		errs += chkquota(fs->fs_spec, quotafile);
	}
	endfsent();
	for (i = 0; i < argc; i++)
		if ((done & (1 << i)) == 0)
			fprintf(stderr, "%s not found in /etc/fstab\n",
				argv[i]);
	exit(errs);
}

chkquota(fsdev, qffile)
	char *fsdev;
	char *qffile;
{
	register struct fileusage *fup;
	struct fileusage zerofupbuf;		/* Empty fileusage entry */
	dev_t quotadev;
	FILE *qf;
	u_short uid;
	int cg, i;
	char *rawdisk;
	struct stat statb;
	struct dqblk dqbuf;

	if (stat(fsdev, &statb) < 0) {
		perror(fsdev);
		return (1);
	}
        if ((statb.st_mode & GFMT) == S_IFCHR) {
		rawdisk = fsdev;
	} else if ((statb.st_mode & GFMT) == S_IFBLK) {
		rawdisk = makerawname(fsdev);
	} else {
		fprintf(stderr, "Bad device type %s\n", fsdev);
		return (1);
	}

	if (vflag)
		fprintf(stdout, "*** Check quotas for %s\n", rawdisk);
	fi = open(rawdisk, 0);
	if (fi < 0) {
		perror(rawdisk);
		return (1);
	}
	qf = fopen(qffile, "r+");
	if (qf == NULL) {
		/*
		 *	If the force flag is set then 
		 *	try creating the file
		 */
		if (fflag) 				/* 001 */
			qf = fopen(qffile, "w+");	/* 001 */

		if (qf == NULL ) {			/* 001 */
			perror(qffile);			/* 001 */
			return (1);			/* 001 */
		}
	}
	if (fstat(fileno(qf), &statb) < 0) {
		perror(qffile);
		return (1);
	}
	quotadev = statb.st_dev;
	if (stat(fsdev, &statb) < 0) {
		perror(fsdev);
		return (1);
	}
	if (quotadev != statb.st_rdev) {
		fprintf(stderr, "%s dev (0x%x) mismatch %s dev (0x%x)\n",
			qffile, quotadev, fsdev, statb.st_rdev);
		return (1);
	}
	quota(Q_SYNC, 0, quotadev, 0);
	sync();
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	ino = 0;
	for (cg = 0; cg < sblock.fs_ncg; cg++) {
		dp = NULL;
		for (i = 0; i < sblock.fs_ipg; i++)
			acct(ginode());
	}
	for (uid = 0; uid <= highuid; uid++) {
		fseek(qf, uid * sizeof(struct dqblk), 0);
		i = fread(&dqbuf, sizeof(struct dqblk), 1, qf);
		if (i == 0)
			dqbuf = zerodqbuf;
		fup = lookup(uid);
		if (fup == (struct fileusage *)0) {
			/*
			 * 002 - If no inodes were found for user, then
			 * zero out quota file entry for user if
			 * stale data exists
			 */
			if ((dqbuf.dqb_curinodes) || (dqbuf.dqb_curblocks)) {
				fup = &zerofupbuf;
				fup->fu_usage.du_curinodes = 0;
				fup->fu_usage.du_curblocks = 0;
			}
			else
				continue;
		}
		if (dqbuf.dqb_curinodes == fup->fu_usage.du_curinodes &&
		    dqbuf.dqb_curblocks == fup->fu_usage.du_curblocks) {
			fup->fu_usage.du_curinodes = 0;
			fup->fu_usage.du_curblocks = 0;
			continue;
		}
		if (vflag) {
			if (fup->fu_name[0] != '\0')
				printf("%-10s fixed:", fup->fu_name);
			else
				printf("#%-9d fixed:", uid);
			fprintf(stdout, " inodes (old %d, new %d)",
			    dqbuf.dqb_curinodes, fup->fu_usage.du_curinodes);
			fprintf(stdout, " blocks (old %d, new %d)\n",
			    dqbuf.dqb_curblocks, fup->fu_usage.du_curblocks);
		}
		dqbuf.dqb_curinodes = fup->fu_usage.du_curinodes;
		dqbuf.dqb_curblocks = fup->fu_usage.du_curblocks;
		fseek(qf, uid * sizeof(struct dqblk), 0);
		fwrite(&dqbuf, sizeof(struct dqblk), 1, qf);
		quota(Q_SETDUSE, uid, quotadev, &fup->fu_usage);
		fup->fu_usage.du_curinodes = 0;
		fup->fu_usage.du_curblocks = 0;
	}
	return (0);
}

acct(ip)
	register struct dinode *ip;
{
	register n;
	register struct fileusage *fup;

	if (ip == NULL)
		return;
	if (ip->di_mode == 0)
		return;
	fup = lookup(ip->di_uid);
	if (fup == 0)
		fup = adduid(ip->di_uid);
	fup->fu_usage.du_curinodes++;
	if ((ip->di_mode & IFMT) == IFCHR || (ip->di_mode & IFMT) == IFBLK)
		return;
	fup->fu_usage.du_curblocks += ip->di_blocks;
}

oneof(target, list, n)
	char *target, *list[];
	register int n;
{
	register int i;

	for (i = 0; i < n; i++)
		if (strcmp(target, list[i]) == 0) {
			done |= 1 << i;
			return (1);
		}
	return (0);
}

struct dinode *
ginode()
{
	register unsigned long iblk;

	if (dp == NULL || ++dp >= &itab[MAXIPG]) {
		iblk = itod(&sblock, ino);
		bread(fsbtodb(&sblock, iblk), (char *)itab, 
			sblock.fs_ipg * sizeof (struct dinode));
		dp = &itab[ino % INOPB(&sblock)];
	}
	if (ino++ < ROOTINO)
		return(NULL);
	return(dp);
}

bread(bno, buf, cnt)
	long unsigned bno;
	char *buf;
	int cnt;
{
	int i;

	lseek(fi, (long)dbtob(bno), 0);
	if ((i = read(fi, buf, cnt)) != cnt) {
		printf("read error %u\n", bno);
		if (i == -1)
			perror("read");
		exit(1);
	}
}

struct fileusage *
lookup(uid)
	u_short uid;
{
	register struct fileusage *fup;

	for (fup = fuhead[uid % FUHASH]; fup != 0; fup = fup->fu_next)
		if (fup->fu_uid == uid)
			return (fup);
	return ((struct fileusage *)0);
}

struct fileusage *
adduid(uid)
	u_short uid;
{
	struct fileusage *fup, **fhp;

	fup = lookup(uid);
	if (fup != 0)
		return (fup);
	fup = (struct fileusage *)calloc(1, sizeof(struct fileusage));
	if (fup == 0) {
		fprintf(stderr, "out of memory for fileusage structures\n");
		exit(1);
	}
	fhp = &fuhead[uid % FUHASH];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_uid = uid;
	if (uid > highuid)
		highuid = uid;
	return (fup);
}

char *
makerawname(cp)
	char *cp;
{
	static char rawbuf[MAXPATHLEN];
	char *dp, *rindex();

	if ((dp =  rindex(cp, '/')) == NULL)
		return (NULL);
	*dp = '\0';
	(void) strcpy(rawbuf, cp);
	*dp = '/';
	(void) strcat(rawbuf, "/r");
	(void) strcat(rawbuf, dp+1);

	return (rawbuf);
}
