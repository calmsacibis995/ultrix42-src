/* @(#)times.h	4.2  (ULTRIX)        9/4/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1984,1986,1987 by			*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/h/times.h
 *
 *	12-Dec-1989	-- scott
 *		X/OPEN: add times declaration (non-kernel) and clock_t typedef
 *
 *	13-Jan-1988	-- map
 *		Added this history.
 *		Changed type to clock_t for POSIX compliance
 *
 * -----------------------------------------------------------------------
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifndef _CLOCK_T_
#define _CLOCK_T_
typedef int	clock_t;
#endif /* _CLOCK_T_ */

#ifdef __POSIX
#ifndef KERNEL
extern clock_t times();
#endif /* KERNEL */
#endif /* __POSIX */

#ifndef _TIMESH_
#define	_TIMESH_

/*
 * Structure returned by times()
 */
struct tms {
	clock_t	tms_utime;		/* user time */
	clock_t	tms_stime;		/* system time */
	clock_t	tms_cutime;		/* user time, children */
	clock_t	tms_cstime;		/* system time, children */
};
#endif
