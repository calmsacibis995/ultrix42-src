#ifndef lint
static char sccsid[] = "@(#)edquota.c	4.2		2/28/91";
#endif

/*
 * Disk quota editor.
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
 *	dws, 28-Feb-91
 *	Pay attention to error returns from quota().
 *
 *	Bob Fontaine, 26-Feb-90
 * 003- Previous fix introduced new bug. Changed getentry() routine to
 *	check the uid against largest possible uid, UID_MAX, as defined in
 *	limits.h header file.  If uid is greater than max, report the error 
 *	and exit.
 *
 *	Bob Fontaine, 13-Jun-89
 *
 * 003- Fixed a bug in the alldigits() routine.  It was incorrectly checking
 *	the return status of the isdigit() function.  This caused the
 *	alldigits() routineto allow numeric user names to be used instead
 *	printing an error message and exiting.
 *
 *	Susan Smith, 21-Nov-85
 * 002- qar fix - mst-0028 can't extend quotas for newly-added user
 *		  changes in routine getdiscq()
 *
 *	Stephen Reilly,	28-Nov-84
 * 001- Added code that notifies the user that we did find and quotas
 *	files, rather than just returning with no errors.
 *
 *	4.5 (Berkeley, from Melbourne) 6/12/83 
 ***********************************************************************/
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <ctype.h>
#include <fstab.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/quota.h>

#define	DEFEDITOR	"/usr/ucb/vi"

struct	dquot dq[NMOUNT];
struct	dquot odq[NMOUNT];
char	dqf[NMOUNT][MAXPATHLEN + 1];
char	odqf[NMOUNT][MAXPATHLEN + 1];

char	tmpfil[] = "/tmp/EdP.aXXXXX";
char	*qfname = "quotas";
char	*getenv();


main(argc, argv)
	char **argv;
{
	int uid;
	char *arg0;

	mktemp(tmpfil);
	close(creat(tmpfil, 0600));
	chown(tmpfil, getuid(), getgid());
	arg0 = *argv++;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [-p username] username ...\n", arg0);
		unlink(tmpfil);
		exit(1);
	}
	--argc;
	if (getuid()) {
		fprintf(stderr, "%s: permission denied\n", arg0);
		unlink(tmpfil);
		exit(1);
	}
	if (argc > 2 && strcmp(*argv, "-p") == 0) {	/* use proto-user template */
		argc--, argv++;
		uid = getentry(*argv++);
		if (uid < 0) {				/* proto-user not found */
			unlink(tmpfil);
			exit(1);
		}
		getprivs(uid);				/* get template */
		argc--;
		while (argc-- > 0) {
			uid = getentry(*argv++);	/* find user in passwd file */
			if (uid < 0)
				continue;
			getdiscq(uid, odq, odqf);	/* find user in quota file */
			putprivs(uid);			/* copy proto info */
		}
		unlink(tmpfil);
		exit(0);
	}
	while (--argc >= 0) {				/* no proto specified so edit */
		uid = getentry(*argv++);
		if (uid < 0)
			continue;
		getprivs(uid);				/* set up with current info */
		if (editit())				/* successful edit? */
			putprivs(uid);			/* so store in tmpfil */
	}
	unlink(tmpfil);
	exit(0);
}

getentry(name)
	char *name;
{
	struct passwd *pw,*getpwnam();
	int uid;

	if (alldigits(name))
		uid = atoi(name);
	else 
	if (pw = getpwnam(name))
		uid = pw->pw_uid;
	else
	{
		fprintf(stderr, "%s: no such user\n", name);
		sleep(1);
		return (-1);
	}
	if(uid > UID_MAX || uid < -UID_MAX)
	{
		fprintf(stderr, "%d: uid out of range\n", uid);
		sleep(1);
		return (-1);
	}
	return (uid);
}

editit()
{
	register pid, xpid;
	int stat, omask;

#define	mask(s)	(1<<((s)-1))
	omask = sigblock(mask(SIGINT)|mask(SIGQUIT)|mask(SIGHUP));
 top:
	if ((pid = fork()) < 0) {
		extern errno;

		if (errno == EPROCLIM) {
			fprintf(stderr, "You have too many processes\n");
			return(0);
		}
		if (errno == EAGAIN) {
			sleep(1);
			goto top;
		}
		perror("fork");
		return (0);
	}
	if (pid == 0) {
		register char *ed;

		sigsetmask(omask);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = DEFEDITOR;
		execlp(ed, ed, tmpfil, 0);
		perror(ed);
		exit(1);
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	sigsetmask(omask);
	return (!stat);
}

getprivs(uid)
	register uid;
{
	register i;
	FILE *fd;

	getdiscq(uid, dq, dqf);		/* find user in quota file */
	for (i = 0; i < NMOUNT; i++) {
		odq[i] = dq[i];
		strcpy(odqf[i], dqf[i]);
	}
	if ((fd = fopen(tmpfil, "w")) == NULL) {
		fprintf(stderr, "edquota: ");
		perror(tmpfil);
		exit(1);
	}
	for (i = 0; i < NMOUNT; i++) {
		if (*dqf[i] == '\0')
			continue;
		fprintf(fd,
"fs %s blocks (soft = %d, hard = %d) inodes (soft = %d, hard = %d)\n"
			, dqf[i]
			, dq[i].dq_bsoftlimit / btodb(1024)
			, dq[i].dq_bhardlimit / btodb(1024)
			, dq[i].dq_isoftlimit
			, dq[i].dq_ihardlimit
		);
	}
	fclose(fd);
}

putprivs(uid)
	register uid;
{
	register i, j;
	int n;
	FILE *fd;
	char line[BUFSIZ];

	fd = fopen(tmpfil, "r");
	if (fd == NULL) {
		fprintf(stderr, "Can't re-read temp file!!\n");
		return;
	}
	for (i = 0; i < NMOUNT; i++) {
		char *cp, *dp, *next();

		if (fgets(line, sizeof (line), fd) == NULL)
			break;
		cp = next(line, " \t");
		if (cp == NULL)
			break;
		*cp++ = '\0';
		while (*cp && *cp == '\t' && *cp == ' ')
			cp++;
		dp = cp, cp = next(cp, " \t");
		if (cp == NULL)
			break;
		*cp++ = '\0';
		while (*cp && *cp == '\t' && *cp == ' ')
			cp++;
		strcpy(dqf[i], dp);
		n = sscanf(cp,
"blocks (soft = %d, hard = %d) inodes (soft = %hd, hard = %hd)\n"
			, &dq[i].dq_bsoftlimit
			, &dq[i].dq_bhardlimit
			, &dq[i].dq_isoftlimit
			, &dq[i].dq_ihardlimit
		);
		if (n != 4) {
			fprintf(stderr, "%s: bad format\n", cp);
			continue;
		}
		dq[i].dq_bsoftlimit *= btodb(1024);
		dq[i].dq_bhardlimit *= btodb(1024);
	}
	fclose(fd);
	n = i;
	for (i = 0; i < n; i++) {
		if (*dqf[i] == '\0')
			break;
		for (j = 0; j < NMOUNT; j++) {
			if (strcmp(dqf[i], odqf[j]) == 0)
				break;
		}
		if (j >= NMOUNT)
			continue;
		*odqf[j] = '\0';
		if (dq[i].dq_isoftlimit == odq[j].dq_isoftlimit &&
		    dq[i].dq_ihardlimit == odq[j].dq_ihardlimit &&
		    dq[i].dq_bsoftlimit == odq[j].dq_bsoftlimit &&
		    dq[i].dq_bhardlimit == odq[j].dq_bhardlimit) {
			for (j = i; j < NMOUNT; j++) {
				dq[j] = dq[j+1];
				strcpy(dqf[j], dqf[j+1]);
			}
			*dqf[j] = '\0';
			i--;
			continue;
		}
		/*
		 * This isn't really good enough, it is quite likely
		 * to have changed while we have been away editing,
		 * but it's not important enough to worry about at
		 * the minute.
		 */
		dq[i].dq_curblocks = odq[j].dq_curblocks;
		dq[i].dq_curinodes = odq[j].dq_curinodes;
		/*
		 * If we've upped the inode or disk block limits
		 * and the guy is out of warnings, reinitialize.
		 */
		if (dq[i].dq_bsoftlimit > odq[j].dq_bsoftlimit &&
		    dq[i].dq_bwarn == 0)
			dq[i].dq_bwarn = MAX_DQ_WARN;
		if (dq[i].dq_isoftlimit > odq[j].dq_isoftlimit &&
		    dq[i].dq_iwarn == 0)
			dq[i].dq_iwarn = MAX_IQ_WARN;
	}
	if (i < NMOUNT) {
		for (j = 0; j < NMOUNT; j++) {
			if (*odqf[j] == '\0')
				continue;
			strcpy(dqf[i], odqf[j]);
			dq[i].dq_isoftlimit = 0;
			dq[i].dq_ihardlimit = 0;
			dq[i].dq_bsoftlimit = 0;
			dq[i].dq_bhardlimit = 0;
			/*
			 * Same applies as just above
			 * but matters not at all, as we are just
			 * turning quota'ing off for this filesys.
			 */
			dq[i].dq_curblocks = odq[j].dq_curblocks;
			dq[i].dq_curinodes = odq[j].dq_curinodes;
			if (++i >= NMOUNT)
				break;
		}
	}
	if (*dqf[0])
		putdiscq(uid, dq, dqf);		/* write out the changes */
}

char *
next(cp, match)
	register char *cp;
	char *match;
{
	register char *dp;

	while (cp && *cp) {
		for (dp = match; dp && *dp; dp++)
			if (*dp == *cp)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

alldigits(s)
	register char *s;
{
	register c;

	c = *s++;
	do {
		if (!isdigit(c))
			return (0);
	} while (c = *s++);
	return (1);
}

/*
*	find user's quotas - if not in memory, then check quotas file
*/

getdiscq(uid, dq, dqf)			
	register uid;
	register struct dquot *dq;
	register char (*dqf)[MAXPATHLEN + 1];
{
	register struct fstab *fs;
	char qfilename[MAXPATHLEN + 1];
	int found_qf = 0;	/* 001 Indicates if we found a quota file */
	int rv;			/* 002 Return value for read */

	setfsent();
	while (fs = getfsent()) {
		struct	stat statb;
		struct	dqblk dqblk;
		dev_t	fsdev;

		if (stat(fs->fs_spec, &statb) < 0)	
			continue;
		fsdev = statb.st_rdev;
		sprintf(qfilename, "%s/%s", fs->fs_file, qfname);
		if (stat(qfilename, &statb) < 0 || statb.st_dev != fsdev)
			continue;
		if (quota(Q_GETDLIM, uid, fsdev, &dqblk) != 0) {

		/*
		 * no quotas currently set for this user; look
		 * for entry in 'quotas' file
		 */
			register fd = open(qfilename, O_RDONLY);

			if (fd < 0)		/* no quota file on this fs */
				continue;
			lseek(fd, (long)(uid * sizeof dqblk), L_SET);

			/*
			 * 002 - If the uid we're looking for is the
			 * highest number in the passwd file, and if
			 * checkquotas has not been run since the uid
			 * was created, lseek left the pointer positioned
			 * at the end of the file.  The read will fail but
			 * we want to create a null entry for this user in
			 * the dqblk structure, so putdiscq will add the 
			 * new entry to the quotas file
 			 */
			rv = (read(fd, &dqblk, sizeof dqblk));		/* 002 */
			if (rv != sizeof(dqblk)) {			/* 002 */
				if (rv == 0)				/* 002 eof */
					bzero(&dqblk,sizeof(dqblk));  	/* 002 make null entry */
				else {					/* 002 other failure */
					close(fd);
					continue;
					}
			}
			close(fd);
		}

		found_qf++;			/* 001 */
		dq->dq_dqb = dqblk;
		dq->dq_dev = fsdev;
		strcpy(*dqf, fs->fs_file);
		dq++, dqf++;
	}
	endfsent();
	**dqf = '\0';
	/*
	 *	If we did not find either any quota files or his uid then
	 *	inform the user of this fact
	 */
	if (!found_qf) {
		fprintf(stderr, "No quota file(s) found or uid was not found, see quotacheck(8).\n");
		exit(1);
	}
}

putdiscq(uid, dq, dqf)
	register uid;
	register struct dquot *dq;
	register char (*dqf)[MAXPATHLEN + 1];
{
	register fd, cnt;
	struct stat sb;
	struct fstab *fs;

	cnt = 0;
	for (cnt = 0; ++cnt <= NMOUNT && **dqf; dq++, dqf++) {
		fs = getfsfile(*dqf);
		if (fs == NULL) {
			fprintf(stderr, "%s: not in /etc/fstab\n", *dqf);
			continue;
		}
		strcat(*dqf, "/");
		strcat(*dqf, qfname);
		if (stat(*dqf, &sb) < 0) {
			continue;
		}
		if (quota(Q_SETDLIM, uid, sb.st_dev, &dq->dq_dqb) < 0) {
			(void) fprintf(stderr, "edquota: ");
			perror(*dqf);
			continue;
		}
		if ((fd = open(*dqf, 1)) < 0) {
			perror(*dqf);
		} else {
			lseek(fd, (long)uid * (long)sizeof (struct dqblk), 0);
			if (write(fd, &dq->dq_dqb, sizeof (struct dqblk)) !=
			    sizeof (struct dqblk)) {
				fprintf(stderr, "edquota: ");
				perror(*dqf);
			}
			close(fd);
		}
	}
}
