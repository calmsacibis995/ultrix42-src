

/*	@(#)utsname.h	4.1	(ULTRIX)	7/2/90	*/


/************************************************************************
 *									*
 *			Copyright (c) 1984 - 1989 by			*
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
 *
 *   Modification history:
 *
 * 28 Dec 89 -- reeves
 *	POSIX: remove reliance on non-POSIX SYS_NMLN definition
 * 17 Jul 89 -- reeves
 *	X/Open: add uname() declaration (non-kernel)
 * 21 Jun 89 -- reeves
 *	POSIX: latch against double inclusion.
 * 15 Apr 88 -- map
 *	Include limits.h here because POSIX does not
 *	require that it be included.
 * 24 May 85 --depp
 *	Changed string length to constant from limits.h
 *
 */
/* @(#)utsname.h	6.1 */

#ifdef KERNEL
#include "../h/limits.h"
#else /* user mode */
extern	int	uname();
#endif
#define __SYS_NMLN 32

#ifndef	_UTSNAME_H_
#define	_UTSNAME_H_

struct utsname {
	char	sysname[__SYS_NMLN];
	char	nodename[__SYS_NMLN];
	char	release[__SYS_NMLN];
	char	version[__SYS_NMLN];
	char	machine[__SYS_NMLN];
};

#endif /* _UTSNAME_H_ */
