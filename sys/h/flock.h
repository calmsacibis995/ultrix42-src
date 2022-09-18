/* @(#)flock.h	4.1  (ULTRIX)        7/2/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 by			*
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
 *	Stephen Reilly, 09-Sept-85
 *	Created for the lockf code
 *
 *	Paul Shaughnessy, 15-Sept-86
 *	Changed values of MAKELOCK - DEADLOCK. Fix for lockf code.
 *
 ***********************************************************************/
/* file segment locking set data type - information passed to system by user */
/* it is also found in fcntl.h */
#ifndef	F_RDLCK
struct	flock	{
	short	l_type;
	short	l_whence;
	long	l_start;
	long	l_len;		/* len = 0 means until end of file */
	int	l_pid;
};
#endif

/* file locking structure (connected to file table entry) */
struct	filock	{
	struct	flock set;	/* contains type, start, and length */
	union	{
		int wakeflg;	/* for locks sleeping on this one */
		int blkpid;	/* pid of blocking lock
				 * (for sleeping locks only)
				 */
	}	stat;
	struct	filock *prev;
	struct	filock *next;
};

/* table to associate files with chain of locks */
struct	flino {
	dev_t	fl_dev;
	ino_t	fl_number;
	int	fl_refcnt;	 /* # of procs currently referencing this flino */
	struct	filock *fl_flck; /* pointer to chain of locks for this file */
	struct	flino  *prev;
	struct	flino  *next;
};

/* file and record locking configuration structure */
/* record and file use totals may overflow */
struct flckinfo {
	long recs;	/* number of records configured on system */
	long fils;	/* number of file headers configured on system */
	long reccnt;	/* number of records currently in use */
	long filcnt;	/* number of file headers currently in use */
	long rectot;	/* number of records used since system boot */
	long filtot;	/* number of file headers used since system boot */
};

extern struct flckinfo	flckinfo;
extern struct filock  	*flox;
extern struct flino   	*flinotab;

/*
 * these defines allow us to perform locking commands in the sfs rlock
 * routines
 */

#define MAKELOCK	0x0
#define DELLOCK		0x1
#define ADJLOCK		0x2
#define BLOCKLOCK	0x3
#define DEADLOCK	0x4
