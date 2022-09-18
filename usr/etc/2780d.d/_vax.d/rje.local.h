/* static char sccsid[]="@(#)rje.local.h	1.2		(ULTRIX)	4/2/86";	*/

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

/*
 * Magic number mapping for binary files, used by 2780e to avoid
 *   sending  object files.
 */

#include <a.out.h>
#include <ar.h>

#ifndef A_MAGIC1	/* must be a VM/UNIX system */
#	define A_MAGIC1	OMAGIC
#	define A_MAGIC2	NMAGIC
#	define A_MAGIC3	ZMAGIC
#	undef ARMAG
#	define ARMAG	0177545
#endif

/*
 * Defaults for 2780e.c
 */
#define DEFLOCK		"lock"
#define DEFSTAT		"status"
#define DEFINIT		"init"
#define	DEFSPOOL	"/usr/spool/rje"
#define	DEFDAEMON	"/etc/2780d"
#define	DEFLOGF		"/dev/console"
#define	DEFMX		1000
#define DEFMAXCOPIES	0
#define DEFUID		1

/*
 * When files are created in the spooling area, they are normally
 *   readable only by their owner and the spooling group.  If you
 *   want otherwise, change this mode.
 */
#define FILMOD		0660

/*
 * We choose not to include this from <sys/param.h>
 */
#define NOFILE		20


/*
 * path name of files created by 2780d.
 */
#define MASTERLOCK "/usr/spool/rje/2780d.lock"



/*
 * Maximum number of user and job requests for 2780e.
 */
#define MAXUSERS	50
#define MAXREQUESTS	50
#define MAXPATHLEN	1024
