#ifndef lint
static char sccsid[] = "@(#)gnsys.c	4.1 (decvax!larry) 7/2/90";
#endif



/*******
 *	gnsys(sname, dir, pre)
 *	char *sname, *dir, pre;
 *
 *	gnsys  -  this routine will return the next
 *	system name which has work to be done.
 *	"pre" is the prefix for work files.
 *	"sname" is a string of size DIRSIZ - WSUFSIZE.
 *
 *	return codes:
 *		0  -  no more names
 *		1  -  name returned in sname
 *		FAIL  -  bad directory
 */



/*
 * Mods:
 * The "retry" code below prevents uucico from calling
 * a site which it has called earlier.
 * Also, uucico does callok() only once for each system.
 * Credit to unc!smb
 *
 * decvax!larry - modifications for new spool structure
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




#include "uucp.h"
#include <sys/types.h>
#include <errno.h>
#ifdef NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif


#define LSIZE 100	/* number of systems to store */
#define WSUFSIZE 6	/* work file name suffix size */
extern int errno;



gnsys(sname, pre)
char *sname, pre;
{
	char *s, *p1, *p2;
	char px[3];
	static char *list[LSIZE];
	static int nitem=0, n=0, base=0;
	char systname[NAMESIZE], filename[NAMESIZE];
	char sysdir[MAXFULLNAME];
	DIR *dirp;
	register int i;

retry:
	px[0] = pre;
	px[1] = '.';
	px[2] = '\0';
	if (nitem == base) {
		/* get list of systems with work */
		int i;

		for (i = base; i < LSIZE; i++)
			list[i] = NULL;

/*****
 *
 * Search through DIRLIST for systems with work.
 *
 */

		for (i=0; i<Subdirs; i++) {
		if  (LSIZE <= nitem)   break;
			sprintf(sysdir,"%s/sys/%s",SPOOL,Dirlist[i]);
			if ((dirp=opendir(sysdir,"r"))==NULL) {
				DEBUG(9,"CAN'T OPEN %s IN gnsys\n", sysdir);
				continue;
			}
			while (gnamef(dirp, filename) != 0)  {
				if (!prefix(px, filename))
					continue;
				nitem = srchst(Dirlist[i], list, nitem);
				break;
			}
			closedir(dirp);
		}

/* now try the default system directory */

		DEBUG(9,"TRY DEFAULT in gnys\n","");
		sprintf(sysdir,"%s/sys/DEFAULT/%s",SPOOL,px);
		dirp = opendir(sysdir, "r");
		ASSERT(dirp != NULL, "BAD DIRECTORY", sysdir, 0);
		while (gnamef(dirp, filename) != 0) {
			if (!prefix(px, filename))
				continue;
			p2 = filename + strlen(filename)
				- WSUFSIZE;
			p1 = filename + strlen(px);
			for(s = systname; p1 <= p2; p1++)
				*s++ = *p1;
			*s = '\0';
			if (systname[0] == '\0')
				continue;
			nitem = srchst(systname, list, nitem);
			if (LSIZE <= nitem) break;
		}
		closedir(dirp);



	}

	if (nitem == base) {
		for (n = 0; n < nitem; n++)
			if (list[n] != NULL)
				free(list[n]);
		return(0);
	}
	while(nitem > n) {
		strcpy(sname, list[n++]);

		if (pre == XQTPRE) /* dont need to check if it is ok to    */
			return(1); /* call a system if looking for X.files */
				   
		if (callok(sname) == 0)
			return(1);
	}
	base = n = nitem;
	goto retry;
}

/***
 *	srchst(name, list, n)
 *	char *name, **list;
 *	int n;
 *
 *	srchst  -  this routine will do a linear search
 *	of list (list) to find name (name).
 *	If the name is not found, it is added to the
 *	list.
 *	The number of items in the list (n) is
 *	returned (incremented if a name is added).
 *
 *	return codes:
 *		n - the number of items in the list
 */

srchst(name, list, n)
char *name, **list;
int n;
{
	int i;
	char *p;

	for (i = 0; i < n; i++)
		if (strcmp(name, list[i]) == 0)
			break;
	if (i >= n) {
		if ((p = calloc((unsigned)strlen(name) + 1, sizeof (char)))
			== NULL)
			return(n);
		strcpy(p, name);
		DEBUG(9,"srchst, new system:%s:\n",p);
		list[n++] = p;
	}
	return(n);
}
