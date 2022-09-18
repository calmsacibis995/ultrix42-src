/*
 *		@(#)grp.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987 by			*
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
 *	Mark A. Parenti, 13-Jan-1988					*
 * 0001	Change gr_gid to type uid_t and add pad for POSIX compliance	*
 *									*
 *	Mark A. Parenti, 05-Aug-1988					*
 * 0002 Change gr_gid to type gid_t as per POSIX			*
 *									*
 *	Jon Reeves, 21-Jun-1989						*
 * 0003	POSIX compliance: latch against multiple inclusion.		*
 *									*
 *	Larry Scott, 26-Dec-1989 					*
 * 0004	POSIX, X/OPEN compliance: add #ifdef's				*
 *									*
 ************************************************************************/
#ifndef	_GRP_H_
#define	_GRP_H_

#include <sys/types.h>

struct	group { /* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
	gid_t	gr_gid;
	short	pad;
	char	**gr_mem;
};

struct group *getgrgid(), *getgrnam();
#if !defined(_POSIX_SOURCE)
struct group *getgrent();
#endif /* !defined(_POSIX_SOURCE) */

#endif /* _GRP_H_ */
