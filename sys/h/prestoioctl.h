/*	@(#)prestoioctl.h	4.3	(ULTRIX)	4/11/91	*/

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1990 Legato Systems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 *  11 Apr 91 -- chet
 *	Add ready routine to prtab struct.
 *
 *  19 Aug 90 -- chet
 *      V4.1 version.
 *
 *  25 May 90 -- chet
 *	Added this file; it was derived from Legato sources.
 *
 */

/*
 * Definitions for the ``presto'' device driver.
 */

/*
 * Presto is initially down until an PRSETSTATE ioctl cmd
 * with arg = PRUP is done.  When presto is down, nothing is
 * being cached and everything is sync'd back to the real disk.
 */

#ifndef _PRESTOIOCTL_H_
#define _PRESTOIOCTL_H_

#ifdef KERNEL
#include "../h/types.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#else
#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#endif /* KERNEL */

/*
 * Presto is initially down until a PRSETSTATE ioctl cmd
 * with arg = PRUP is done.  When presto is down, nothing is
 * being cached and everything is written through to the real disk.
 */

enum battery {
	BAT_GOOD = 0,
	BAT_LOW = 1,
	BAT_DISABLED = 2,
	BAT_IGNORE = 3,
};
typedef enum battery battery;

#define MAX_BATTERIES 8

enum prstates {
	PRDOWN = 0,
	PRUP = 1,
	PRERROR = 2,
};
typedef enum prstates prstates;

struct io {
	u_int total;
	u_int hitclean;
	u_int hitdirty;
	u_int pass;
	u_int alloc;
};
typedef struct io io;

struct presto_status {
	prstates pr_state;	/* if not PRUP, pass all rw commands thru */
	u_int	pr_battcnt;
	battery	pr_batt[MAX_BATTERIES];	/* array of battery status flags */
	u_int	pr_maxsize;	/* total memory size in bytes available */
	u_int	pr_cursize;	/* current presto memory size */
	u_int	pr_ndirty;	/* current number of dirty presto buffers */
	u_int	pr_nclean;	/* current number of clean presto buffers */
	u_int	pr_ninval;	/* current number of invalid presto buffers */
	u_int	pr_nactive;	/* current number of active presto buffers */
	/* the io stats are zeroed each time presto is reenabled */
	io 	pr_rdstats;	/* presto read statistics */
	io 	pr_wrstats;	/* presto write statistics */
	u_int	pr_seconds;	/* seconds of stats taken */
};
typedef struct presto_status presto_status;

struct presto_modstat {
	int ps_status;
	union {
		char *ps_errmsg;
		struct presto_status ps_new;
	} presto_modstat_u;
};
typedef struct presto_modstat presto_modstat;

/*
 * Get the current presto status information.
 */
#define PRGETSTATUS	_IOR('p', 1, struct presto_status)

/*
 * Set the presto state.  Legal values are PRDOWN and PRUP.
 * When presto is enabled, all the io stats are zeroed.  If
 * presto is in the PRERROR state, it cannot be changed.
 */
#define PRSETSTATE	_IOW('p', 2, int)

/*
 * Set the current presto memory size in bytes.
 */
#define PRSETMEMSZ	_IOW('p', 3, int)

/*
 * Reset the entire presto state.  If there are any pending writes
 * back to the real disk which cannot be completed due to IO errors,
 * these writes will be lost forever.  Using this ioctl is the only
 * way presto will ever lose any dirty data if disk errors develop
 * behind presto.  Presto will be left in the PRDOWN state unless
 * all batteries are currently low.
 */
#define PRRESET		_IO('p', 4)

/*
 * Enable presto on a particular presto'ized filesystem.
 * This ioctl is performed on the presto control device, passing in
 * the *block* device number of the presto'ized filesystem to enable.
 */
#define PRENABLE	_IOW('p', 5, dev_t)

/*
 * Disable presto on a particular presto'ized filesystem.
 * This ioctl is performed on the presto control device, passing in
 * the *block* device number of the presto'ized filesystem to disable.
 */
#define PRDISABLE	_IOW('p', 6, dev_t)

struct prbits {
	unsigned char	bits[256/NBBY];	/* bit for each minor bdevno */
};

/*
 * Prutab structure - new "prtab" replacement for user visible fields.
 */
struct uprtab {
	u_int upt_bmajordev;		/* 0: block major device numer */
	struct prbits upt_enabled; 	/* 4: per minor enabled bits */
	struct prbits upt_bounceio;	/* 36: per minor dev bounceio bits */
	                                /* 68 bytes long */
};

/*
 * Fill a given uprtab structure for the given major device
 * in prtabs[] or get the "next" uprtab in the system.  A
 * bmajordev of NODEV on return says no such or no more entries.
 */
#define PRNEXTUPRTAB		_IOWR('p', 9, struct uprtab)
#define PRGETUPRTAB		_IOWR('p', 10, struct uprtab)

/*
 * Flush the any dirty buffers in the presto cache back to disk
 * without changing the current PRUP/PRDOWN state of the board.
 */
#define PRFLUSH			_IO('p', 11)

#ifdef KERNEL
/*
 * Prtab structure - kernel data structure kept per presto-ized major device.
 */
struct prtab {
	u_int pt_bmajordev;		/* 0: block major device numer */
	int (*pt_strategy)();		/* 4: major dev strategy routine */
	int (*pt_ready)();		/* 8: major dev `ready' routine */
	struct prbits pt_bounceio;	/* 12: per minor dev bounceio bits */
	struct prbits pt_enabled; 	/* 44: per minor enabled bits */
	struct prbits pt_error; 	/* 76: per minor dev error bits */
	struct prbits pt_flushing; 	/* 108: per minor dev flushing bits */
	                                /* 140 bytes long */
};
#endif /* KERNEL */

/*
 * Miscellaneous defines
 */
#define PR_BOUNCEIO	0x1		/* pr_flags value for all bounceio */
#define IOCTL_NUM(x)	((x) & 0xff)	/* macro to extract ioctl cmd no */
#define PRDEV		"/dev/pr0"	/* generic presto control device */

#endif /* __PRESTOIOCTL_H_ */
