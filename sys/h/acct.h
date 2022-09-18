/* @(#)acct.h	4.5  (ULTRIX)        4/25/91     */

/* 
 * Modification history
 *
 * Apr 11, 91	rjg
 *	correct def of ac_mem from short to u_short for mips
 *
 * 9/10/90	dlh
 *	correct placement of #ifdef __mips for AVP define
 *
 * 9/4/90	dlh
 * 	added AVP bit defintion for ac_flag
 *	(process is a vector process)
 */

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
 * Accounting structures
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef __mips
typedef u_short comp_t;
#endif /* __mips */

struct	acct
{
	char	ac_comm[10];		/* Accounting command name */
#ifdef __vax
	float	ac_utime;		/* Accounting user time */
	float	ac_stime;		/* Accounting system time */
	float	ac_etime;		/* Accounting elapsed time */
#else /* mips */
	comp_t	ac_utime;
	comp_t	ac_stime;
	comp_t	ac_etime;
#endif /* __mips */
	time_t	ac_btime;		/* Beginning time */
	short	ac_uid;			/* Accounting user ID */
	short	ac_gid;			/* Accounting group ID */
#ifdef __vax
	float	ac_mem;			/* average memory usage */
	float	ac_io;			/* number of disk IO blocks */
#else /* mips */
	u_short ac_mem;
	comp_t	ac_io;
#endif /* __mips */
	dev_t	ac_tty;			/* control typewriter */
	char	ac_flag;		/* Accounting flag */
};

#define	AFORK	0001		/* has executed fork, but no exec */
#define	ASU	0002		/* used super-user privileges */
#define	ACOMPAT	0004		/* used compatibility mode */
#define	ACORE	0010		/* dumped core */
#define	AXSIG	0020		/* killed by a signal */
#define	AVP	0040		/* this is (was) a vector process */
#ifdef __mips
#define AHZ	64		/* the accuracy of data is 1/AHZ */
#endif /* __mips */

#ifdef KERNEL
struct	acct	acctbuf;
struct	gnode	*acctp;
#endif
