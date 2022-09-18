
/*	@(#)stat.h	4.2	(ULTRIX)	9/4/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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

/*
 *   Modification History:
 *
 *	Paul Shaughnessy,	27-Mar-1990
 *	Fixed XOPEN extention defines.
 *
 *	Paul Shaughnessy,	17-Jan-1990
 *	Added _POSIX_SOURCE and _XOPEN_SOURCE test macros.
 *
 * 0004	Jon Reeves, 02-Jun-89
 *	Added POSIX-mandated function definitions (user mode only); changed
 *	stat members to use POSIX type names; added double-inclusion
 *	protection
 *
 * 14 Jan 88 -- chet
 *	added st_gennum to struct stat
 *
 *	Paul Shaughnessy (prs), 04-Dec-1986
 * 0002 Modified usr/group compatability section.
 *
 *	David L Ballenger, 28-Mar-1985
 * 0001 Add definitions from BRL package for System V support
 *
 */

#ifndef	_STAT_H_
#define	_STAT_H_

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

struct	stat
{
	dev_t		st_dev;
	ino_t		st_ino;
	mode_t		st_mode;
	nlink_t		st_nlink;
	uid_t		st_uid;
	gid_t		st_gid;
	dev_t		st_rdev;
	off_t		st_size;
	time_t		st_atime;
	int		st_spare1;
	time_t		st_mtime;
	int		st_spare2;
	time_t		st_ctime;
	int		st_spare3;
	long		st_blksize;
	long		st_blocks;
	unsigned long	st_gennum;
	long		st_spare4;
};

/* file modes */
#define	S_IRWXU	00700		/* read, write, execute: owner */
#define	S_IRUSR	00400		/*  read permission: owner */
#define	S_IWUSR	00200		/*  write permission: owner */
#define	S_IXUSR	00100		/*  execute permission: owner */

#define	S_IRWXG	00070		/* read, write, execute: group */
#define	S_IRGRP	00040		/*  read permission: group */
#define	S_IWGRP	00020		/*  write permission: group */
#define	S_IXGRP	00010		/*  execute permission: group */

#define	S_IRWXO	00007		/* read, write, execute: other */
#define	S_IROTH	00004		/*  read permission: other */
#define	S_IWOTH	00002		/*  write permission: other */
#define	S_IXOTH	00001		/*  execute permission: other */

#define	S_ISUID	0004000		/* set user id on execution */
#define	S_ISGID	0002000		/* set group id on execution */


/*
 * Need to define file modes and a mode mask which will not
 * polute the POSIX name space, and allow applications which
 * define _POSIX_SOURCE only to use file type test macros.
 */

#define _S_IFMT	 0170000	/* type of file; sync with S_IFMT */
#define	_S_IFBLK 0060000	/* block special; sync with S_IFBLK */
#define	_S_IFCHR 0020000	/* character special sync with S_IFCHR */
#define	_S_IFDIR 0040000	/* directory; sync with S_IFDIR */
#define	_S_IFIFO 0010000	/* FIFO - named pipe; sync with S_IFIFO */
#define	_S_IFREG 0100000	/* regular; sync with S_IFREG */

/* File type test macros */

/* macro to test for block special file */
#define	S_ISBLK( mode )		(((mode) & _S_IFMT) == _S_IFBLK)

/* macro to test for character special file */
#define	S_ISCHR( mode )		(((mode) & _S_IFMT) == _S_IFCHR)

/* macro to test for directory file */
#define	S_ISDIR( mode )		(((mode) & _S_IFMT) == _S_IFDIR)

/* macro to test for fifo special file */
#define	S_ISFIFO( mode )	(((mode) & _S_IFMT) == _S_IFIFO)

/* macro to test for regular file */
#define	S_ISREG( mode )		(((mode) & _S_IFMT) == _S_IFREG)

#if	!defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)

/* Base and XOPEN File types */
#define	S_IFMT	0170000		/* type of file; sync with _S_IFMT */
#define	S_IFBLK 0060000		/* block special; sync with _S_IFBLK */
#define	S_IFCHR 0020000		/* character special; sync with _S_IFCHR */
#define	S_IFDIR 0040000		/* directory; sync with _S_IFIFO */
#define	S_IFIFO	0010000		/* FIFO - named pipe; sync with _S_IFIFO */
#define	S_IFREG 0100000		/* regular; sync with _S_IFREG */

/* Base and XOPEN File modes */
#define	S_ISVTX	0001000		/* save swapped text even after use */
#define	S_IREAD	0000400		/* read permission, owner */
#define	S_IWRITE 0000200	/* write permission, owner */
#define	S_IEXEC	0000100		/* execute/search permission, owner */

#endif /* BASE and XOPEN extentions */

#if	!defined(_POSIX_SOURCE)

/* Non POSIX and XOPEN file types */
#define	S_IFLNK		0120000		/* symbolic link */
#define	S_IFSOCK	0140000		/* socket */
#define	S_IFPORT	S_IFIFO
/* GFS defines for file types */
#define	S_GFMT		S_IFMT
#define	S_GFBLK		S_IFBLK
#define	S_GFCHR		S_IFCHR
#define	S_GFDIR		S_IFDIR
#define	S_GFREG		S_IFREG
#define	S_GFLNK		S_IFLNK
#define	S_GFSOCK 	S_IFSOCK
#define	S_GFPORT 	S_IFIFO
/* GFS defines for file modes */
#define	S_GSUID	S_ISUID
#define	S_GSGID	S_ISGID
#define	S_GSVTX	S_ISVTX
#define	S_GREAD	S_IREAD
#define	S_GWRITE S_IWRITE
#define	S_GEXEC	S_IEXEC


/* Additions from BRL package to support /usr/group standard
 */

#define	S_ENFMT	S_GSGID		/* record locking enforcement flag */

/* these are included for compatibility */

#define	S_GSBLK S_ISBLK	
#define	S_GSCHR S_ISCHR	
#define	S_GSDIR S_ISDIR	
#define	S_GSFIFO S_ISFIFO 
#define S_GFIFO S_IFIFO
#define	S_GSREG S_ISREG	

#define	S_GRWXU S_IRWXU	
#define	S_GRUSR S_IRUSR	
#define	S_GWUSR S_IWUSR	
#define	S_GXUSR S_IXUSR	

#define	S_GRWXG S_IRWXG	
#define	S_GRGRP S_IRGRP	
#define	S_GWGRP S_IWGRP	
#define	S_GXGRP S_IXGRP	

#define	S_GRWXO S_IRWXO	
#define	S_GROTH S_IROTH	
#define	S_GWOTH S_IWOTH	
#define	S_GXOTH S_IXOTH	

#endif	/* NOT _POSIX_SOURCE */

#if	((!defined(__POSIX) && !defined(KERNEL)) || \
	  (defined(POSIX) && !defined(_POSIX_SOURCE)) || \
	  (defined(POSIX) &&  defined(_XOPEN_SOURCE)))
extern	mode_t	umask();
extern	int
	chmod(),
	fstat(),
	mkdir(),
	mkfifo(),
	stat();
#endif

#endif	/* _STAT_H_ */
