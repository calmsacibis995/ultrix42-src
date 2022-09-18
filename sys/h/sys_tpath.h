/*      @(#)sys_tpath.h	2.1     (ULTRIX)        6/12/89     */

/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989 by			*
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

#ifndef __TPATH__
#define __TPATH__

extern	int	do_tpath;		/* Tpath on/off switch 			*/
extern	int	tpath_handler;		/* Tpath handler sleep/wakeup point 	*/
extern 	struct	lock_t lk_tpath;	/* Tpath daemon lock			*/

#define TPATH_SLEEP \
	 { \
		int s = spltty(); \
		smp_lock(&lk_tpath, LK_RETRY); \
		while (tpath_handler == 0) { \
			sleep_unlock((caddr_t)&tpath_handler, PUSER, &lk_tpath); \
			smp_lock(&lk_tpath, LK_RETRY); \
		} \
		tpath_handler = 0; \
		smp_unlock(&lk_tpath); \
		splx(s); \
	}

#define TPATH_WAKEUP \
	{ \
		int s = spltty(); \
		smp_lock(&lk_tpath, LK_RETRY); \
		tpath_handler = 1; \
		wakeup((caddr_t)&tpath_handler); \
		smp_unlock(&lk_tpath); \
		splx(s); \
	}

#endif /* __TPATH__ */
