#ifndef lint
static char *sccsid = "@(#)fstab.c	4.2	(ULTRIX)	9/11/90";
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

#include <fstab.h>
#include <stdio.h>
#include <ctype.h>

static	struct fstab fs;
static	char line[BUFSIZ+1];
static	FILE *fs_file = 0;
#define NFIELDS 7

static char *
fsskip(p)
	register char *p;
{

	if(p == NULL)
		return(NULL);
	while (*p && *p != ':')
		++p;
	if (*p)
		*p++ = 0;
	return (p);
}

static char *
fsdigit(backp, string, end)
	int *backp;
	char *string, end;
{
	register int value = 0;
	register char *cp;

	if(string == NULL)
		return(NULL);
	for (cp = string; *cp && isdigit(*cp); cp++) {
		value *= 10;
		value += *cp - '0';
	}
	if (*cp == '\0')
		return ((char *)0);
	*backp = value;
	while (*cp && *cp != end)
		cp++;
	if (*cp == '\0')
		return ((char *)0);
	return (cp+1);
}

static
fstabscan(fs)
	struct fstab *fs;
{
	register char *cp;

	cp = fgets(line, 256, fs_file);
	if (cp == NULL)
		return (EOF);
	fs->fs_spec = cp;
	cp = fsskip(cp);
	fs->fs_file = cp;
	cp = fsskip(cp);
	fs->fs_type = cp;
	cp = fsskip(cp);
	cp = fsdigit(&fs->fs_freq, cp, ':');
	cp = fsdigit(&fs->fs_passno, cp, ':');
	fs->fs_name = cp;
	cp = fsskip(cp);
	fs->fs_opts = cp;
	cp = fsskip(cp);
	return (NFIELDS);
}
	
setfsent()
{

	if (fs_file)
		endfsent();
	if ((fs_file = fopen(FSTAB, "r")) == NULL) {
		fs_file = 0;
		return (0);
	}
	return (1);
}

endfsent()
{

	if (fs_file) {
		fclose(fs_file);
		fs_file = 0;
	}
	return (1);
}

struct fstab *
getfsent()
{
	int nfields;

	if ((fs_file == 0) && (setfsent() == 0))
		return ((struct fstab *)0);
	nfields = fstabscan(&fs);
	if (nfields == EOF || nfields != NFIELDS)
		return ((struct fstab *)0);
	return (&fs);
}

struct fstab *
getfsspec(name)
	char *name;
{
	register struct fstab *fsp;

	if (setfsent() == 0)	/* start from the beginning */
		return ((struct fstab *)0);
	while((fsp = getfsent()) != 0)
		if (strcmp(fsp->fs_spec, name) == 0)
			return (fsp);
	return ((struct fstab *)0);
}

struct fstab *
getfsfile(name)
	char *name;
{
	register struct fstab *fsp;

	if (setfsent() == 0)	/* start from the beginning */
		return ((struct fstab *)0);
	while ((fsp = getfsent()) != 0)
		if (strcmp(fsp->fs_file, name) == 0)
			return (fsp);
	return ((struct fstab *)0);
}

struct fstab *
getfstype(type)
	char *type;
{
	register struct fstab *fs;

	if (setfsent() == 0)
		return ((struct fstab *)0);
	while ((fs = getfsent()) != 0)
		if (strcmp(fs->fs_type, type) == 0)
			return (fs);
	return ((struct fstab *)0);
}
