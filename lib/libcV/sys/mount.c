#ifndef lint
static	char	*sccsid = "@(#)mount.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987, 1988 by		*
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
 *	From Doug Gwyn
 *
 * 	Mark A Parenti, 09-Oct-1987
 * 001	Change _mount() system call for new Ultrix interface
 *
 *	Mark A. Parenti, 26-Aug-1988
 * 002	Allow relative pathname as argument to mount().
 ************************************************************************/

#include	<errno.h>
#include	<sys/types.h>
#include	<sys/fs_types.h>
#include	<limits.h>

extern int	_mount(), geteuid();

int
mount( spec, dir, rwflag )
	char	*spec;			/* special file being mounted */
	char	*dir;			/* directory serving as root */
	int	rwflag; 		/* low bit 1 iff read-only */
	{
	char	path[PATH_MAX];
	char	*ptr;
	int	pos;

	ptr = dir;
	if(*dir != '/') {	/* Not an absolute pathname */
		ptr = path;
		getcwd(ptr, PATH_MAX);
		if((strlen(ptr) + strlen(dir) + 1) > PATH_MAX) {
			errno = ENOENT;
			return(-1);
		}
		strcat( ptr, "/");
		strcat( ptr, dir);
	}
	if ( _mount( spec, ptr, rwflag & 1, GT_ULTRIX, 0 ) != 0 )
		{
		switch ( errno )
			{
		case ENODEV:
			if ( geteuid() != 0 )
				errno = EPERM;	/* not super-user */
			else
				errno = ENOENT;	/* spec nonexistent */
			break;

		case EROFS:		/* `dir' on read-only FS */
			errno = ENOTDIR;
			break;
/*
		case ENOTBLK:
		case ENXIO:
		case ENOTDIR:
		case EBUSY:
			break;
*/
			}
		return -1;
		}

	return 0;
	}
