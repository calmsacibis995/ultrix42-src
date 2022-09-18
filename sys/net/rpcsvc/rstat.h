/* "@(#)rstat.h	4.1 (ULTRIX)        7/2/90 */

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

#ifndef CPUSTATES
#ifdef KERNEL
#include "../h/dk.h"
#else /* KERNEL */
#include <sys/dk.h>
#endif /* KERNEL */
#endif

#ifndef DST_NONE
#ifdef KERNEL
#include "../h/time.h"
#else /* KERNEL */
#include <sys/time.h>
#endif /* KERNEL */
#endif

#define RSTATPROG 100001
#define RSTATVERS_ORIG 1
#define RSTATVERS_SWTCH 2
#define RSTATVERS_TIME  3
#define RSTATVERS 3
#define RSTATPROC_STATS 1
#define RSTATPROC_HAVEDISK 2

struct stats {				/* version 1 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
};

struct statsswtch {				/* version 2 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	unsigned v_swtch;
	long avenrun[3];
	struct timeval boottime;
};
struct statstime {				/* version 3 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	unsigned v_swtch;
	long avenrun[3];
	struct timeval boottime;
	struct timeval curtime;
};

int xdr_stats();
int xdr_statsswtch();
int xdr_statstime();
int havedisk();
