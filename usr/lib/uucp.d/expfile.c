#ifndef lint
static char sccsid[] = "@(#)expfile.c	4.1 (decvax!larry) 7/2/90";
#endif

#include "uucp.h"
#include <sys/types.h>
#include <sys/stat.h>


/*
 * decvax!larry - changes since 4.2 BSD - isdir() does not
 *		     use subfile().  It expects fully expanded
 *		     path names.
 */


/*******
 *	expfile(file)	expand file name
 *	char *file;
 *
 *	return codes: 0 - Ordinary spool area file
 *		      1 - Other normal file
 *		      FAIL - no Wrkdir name available
 *
 *
 *
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





expfile(file)
char *file;
{
	register char *fpart, *p;
	char user[20], *up;
	char full[100];
	int uid;

	switch(file[0]) {
	case '/':
		return(1);
	case '~':
		for (fpart = file + 1, up = user; *fpart != '\0'
			&& *fpart != '/'; fpart++)
				*up++ = *fpart;
		*up = '\0';
/* There should not be a null entry in /etc/passwd but just in case ... */
		if (gninfo(user, &uid, full) != 0 || user[0]=='\0') {
			strcpy(full, PUBDIR);
		}
	
		strcat(full, fpart);
		strcpy(file, full);
		return(1);
	default:
		p = index(file, '/');
		strcpy(full, Wrkdir);
		strcat(full, "/");
		strcat(full, file);
		strcpy(file, full);
		if (Wrkdir[0] == '\0')
			return(FAIL);
		else if (p != NULL)
			return(1);
		return(0);
	}
}


/***
 *	isdir(name)	check if directory name
 *	char *name;
 *
 *	return codes:  0 - not directory  |  1 - is directory
 */

isdir(name)
char *name;
{
	int ret;
	struct stat s;

	ret = stat(name, &s);
	if (ret < 0)
		return(0);
	if ((s.st_mode & S_IFMT) == S_IFDIR)
		return(1);
	return(0);
}


/***
 *	mkdirs(name)	make all necessary directories
 *	char *name;
 *
 *	return 0  |  FAIL
 */

mkdirs(name)
char *name;
{
	int ret, mask;
	char cmd[100], dir[100], *p;

	for (p = dir + 1;; p++) {
		strcpy(dir, name);
		if ((p = index(p, '/')) == NULL)
			return(0);
		*p = '\0';
		if (isdir(dir))
			continue;
		sprintf(cmd, "mkdir %s", dir);
		DEBUG(4, "mkdir - %s\n", dir);
		mask = umask(0);
		ret = shio(cmd, CNULL, CNULL, User, CNULL);
		umask(mask);
		if (ret != 0)
			return(FAIL);
	}
}

/***
 *	ckexpf - expfile and check return
 *		print error if it failed.
 *
 *	return code - 0 - ok; FAIL if expfile failed
 */

ckexpf(file)
char *file;
{

	if (expfile(file) != FAIL)
		return(0);

	/*  could not expand file name */
	/* the gwd routine failed */

	fprintf(stderr, "Can't expand filename (%s). Pwd failed.\n", file+1);
	return(FAIL);
}
