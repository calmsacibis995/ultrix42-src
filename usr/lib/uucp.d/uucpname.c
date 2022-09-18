#ifndef lint
static	char	*sccsid = "@(#)uucpname.c	4.1	(ULTRIX)	7/2/90";
#endif

/*******
 *	uucpname(name)		get the uucp name
 *
 *	return code - none
 */


/*********************
 * Mods:
 *	16-May-88	logcher
 *		     - when creating "name", check for a "." before
 *			truncating at 7 characters.
 *	decvax!larry - create list of systems that have there own
 *			spool directory.
 ********************/




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




#include "uucp.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif



#ifdef	CCWHOAMI
/*  compile in local uucp name of this machine */
#include <whoami.h>
#endif


uucpname(name)
register char *name;
{
	register char *s, *d;
	char hostname[15];

	/* HUGE KLUDGE HERE!  rti!trt
	 * Since some UNIX systems do not honor the set-user-id bit
	 * when the invoking user is root, we must change the uid here.
	 * So uucp files are created with the correct owner.
	 * This is a real nuisance.  And it has not been tested.
	 */
	if (geteuid() == 0 && getuid() == 0) {
		struct stat stbuf;
		stbuf.st_uid = 0;	/* In case the stat fails */
		stbuf.st_gid = 0;
		stat(UUCICO, &stbuf);	/* Assume uucico is correctly owned */
		setgid(stbuf.st_gid);
		setuid(stbuf.st_uid);
	}

#ifdef	UUNAME		/* This gets home site name from file  */
    {
	FILE *uucpf;
	char stmp[10];

	s = stmp;
	if (((uucpf = fopen("/etc/uucpname", "r")) == NULL &&
	     (uucpf = fopen("/local/uucpname", "r")) == NULL) ||
		fgets(s, 8, uucpf) == NULL) {
			s = "unknown";
	} else {
		for (d = stmp; *d && *d != '\n' && d < stmp + 8; d++)
			;
		*d = '\0';
	}
	if (uucpf != NULL)
		fclose(uucpf);
    }
#endif

#ifdef	GETHOST
    {
	int hostlength=15;
	gethostname(hostname, hostlength);
	s = hostname;
    }
#endif

#ifdef	CCWHOAMI
    {
	s = sysname;
    }
#endif

	d = name;
	while ((*d = *s++) && (*d != '.') && (d < name + 7))
		d++;
	*d = '\0';
#ifdef	UUDIR
	sprintf(DLocal, "D.%s", name);
	sprintf(DLocalX, "D.%sX", name);
	build_DIRLIST();
#endif
	return;
}

/****************
 *    initialize list of remote system subdirectories
 *
 *************/

#ifdef UUDIR

build_DIRLIST()
{
	struct direct *Cdirp;
	DIR *Cfd;
	char sysdir[MAXFULLNAME];

	Subdirs = 0;
	sprintf(sysdir,"%s/sys",SPOOL);
	Cfd=opendir(sysdir,"r");
	ASSERT(Cfd != NULL,"CAN NOT OPEN", sysdir, 0);
	while ((Cdirp=readdir(Cfd)) != NULL) {
                if(Cdirp->d_ino==(ino_t)0 || strcmp("DEFAULT",Cdirp->d_name)==0 			|| strcmp(".",Cdirp->d_name)==0 || strcmp("..",Cdirp->d_name)==0)
			continue;
		Dirlist[Subdirs] = malloc((unsigned)(strlen(Cdirp->d_name)+1));
		strcpy(Dirlist[Subdirs++], Cdirp->d_name);
		DEBUG(9, "In uucpname, Dirlist:%s:\n",Dirlist[Subdirs-1]);
	} 
	closedir(Cfd);
}
#endif
