/*
 *		@(#)pwd.h	4.2	(ULTRIX)	9/4/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987, 1989 by		*
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
 *			Modification History				*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Add definitions for System V compatibility.			*
 *									*
 * 	Mark A. Parenti, 14-Jan-1988					*
 * 0002	Change pw_uid and pw_gid to type uid_t and add pad for		*
 *	POSIX compliance.						*
 *									*
 *	Jon Reeves, 21-Jun-1989						*
 * 0003	More POSIX compliance: protect against double inclusion, add	*
 *	_POSIX_SOURCE test.						*
 *									*
 *	D. Long, 16-Oct-1989
 * 0004	Use type gid_t for group, not uid_t.				*
 ************************************************************************/
#include <ansi_compat.h>
#ifndef	_PWD_H_
#define	_PWD_H_

#include <sys/types.h>

struct	passwd { /* see getpwent(3) */
	char	*pw_name;
	char	*pw_passwd;
	uid_t	pw_uid;
	short	pad;
	gid_t	pw_gid;
	short	pad1;
#ifndef	__SYSTEM_FIVE
	int	pw_quota;	/* ULTRIX, BSD-4.2 */
#else /*	SYSTEM_FIVE */
	char	*pw_age;	/* System V */
#endif /*	__SYSTEM_FIVE */
	char	*pw_comment;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};

#ifndef	_POSIX_SOURCE

struct comment {
	char	*c_dept;
	char	*c_name;
	char	*c_acct;
	char	*c_bin;
};
#endif /* _POSIX_SOURCE */

struct passwd *getpwent(), *getpwuid(), *getpwnam();

#endif /* _PWD_H_ */
