/*	@(#)presto.h	4.4	(ULTRIX)	1/10/91	*/

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

#ifndef _PRESTO_H_
#define _PRESTO_H_
#ifdef KERNEL
#include "../h/prestoioctl.h"
#else  /*  KERNEL */
#include <sys/prestoioctl.h>
#endif /* KERNEL */

/*
 * Presto buffer management code.
 */
#define PRBQUEUES	3		/* number of free buffers queues */

#define PR_DIRTY	0		/* dirty buffers */
#define PR_CLEAN	1		/* clean buffers */
#define PR_INVAL	2		/* buffers without valid data */

#define PRBUFHSZ	64
#define PRRND		(PRBSIZE / DEV_BSIZE)

extern struct bufhd prbufhash[PRBUFHSZ];
extern struct diskhd prbfreelist[PRBQUEUES];
extern struct buf *prbufs;
extern int prnbufs;
extern struct presto_status prstatus;

/*
 * Diskhd structures used at the head of the disk unit queues.
 * We only need a few elements for these, so this abbreviated
 * definition saves some space.
 */
struct diskhd {
      long b_flags;                   /* not used, needed for consistency */
      struct  buf *b_forw, *b_back;   /* queue of unit queues */
      struct  buf *av_forw, *av_back; /* queue of bufs for this unit */
      long    b_bcount;               /* active flag */
};

#define PRBUFHASH(dev, dblkno) \
	((struct buf *)&prbufhash \
		[((u_int)(dev)+(((int)(dblkno))/PRRND)) & (PRBUFHSZ-1)])

/*
 * The nvbuf structure defines the parts of the buffer structure
 * that must be kept in non-volatile memory so that buffers can
 * survive reboots.  These fields are merely copies of the
 * corresponding fields kept in the presto buffers in main memory.
 */
struct nvbuf {
	int	nb_flags;
	daddr_t	nb_blkno;
	dev_t	nb_dev;
	u_short	nb_bcount;
};

/*
 * PRBSIZE and PRFSIZE are the basic size for presto buffering.
 * PRBSIZE is the largest block size we buffer.  PRFSIZE is the
 * "fragment" size of the smallest request we will buffer.
 *
 * We choose 8k/1K for these values; this corresponds
 * to the default ufs blocks sizes.
*/
#define PRBSIZE		MAXBSIZE		/* largest presto bsize */
#define PRFSIZE		1024			/* smallest presto bsize */
#define	NCHKSUMS	(PRBSIZE / PRFSIZE)	/* # of chksums per buffer */

#define PRMINSIZE	roundup((sizeof (struct nv) + PRBSIZE), PRBSIZE)
#define PRMAXSIZE	0x200000
#define PRMAXBUFS	((PRMAXSIZE / PRBSIZE) - 1)

#define PRBALIGN(x)	(((u_int)(x) + PRBSIZE - 1) & ~(PRBSIZE - 1))

struct nvh {
	u_int nvh_dirty;		/* value of PRESTO_DIRTY means
					 * cache contains dirty data */
	u_int nvh_magic;		/* magic number */
	u_int nvh_size;			/* total bytes being used */
	u_int nvh_version;		/* version number for validation */
	u_int nvh_bsize;		/* holds PRBSIZE for validation */
	u_int nvh_fsize;		/* holds PRFSIZE for validation */
	u_int nvh_machineid;		/* holds machine id for validation */
	u_int nvh_nbufs;		/* number of presto buffers */
};

struct nv {
	struct 	nvh nv_nvh;
	struct 	nvbuf nv_bufs[PRMAXBUFS];
	char 	nv_scratch[DEV_BSIZE + sizeof (long)];
};

/*
 * The data buffers are PRBSIZE aligned below the nv structure; since the
 * position of the header in the "pr" device space is different on various
 * boards, we have to dynamically calculate the page aligned start of the
 * data buffers.
 */
struct nvbd {
	char 		nvbd_data[PRMAXBUFS][PRBSIZE];
};

/*
 * The PRVERSION number should be increased everytime
 * the nvh or nv structure contents or layout changes.
 */
#define PRVERSION	0xb		/* version number for above */
#define PRMAGIC		0x031758	/* presto non-volatile magic number */

/* Clean/Dirty word (first word of presto NVRAM cache) value */
#define PRESTO_DIRTY	0xbd100248	/* cache contains dirty data */
					/* anything else means it does not */ 

/* Status of NVRAM diagnostics */
#define NVRAM_BAD	0	/* either read/write or read-only diagnostics
				 * run unsuccessfully */
#define NVRAM_RDWR	1	/* read/write diagnostics run successfully */
#define NVRAM_RDONLY	2	/* read-only diagnostics run successfully */

/* Array of pointers to NVRAM cache interface routines */
struct presto_interface {
	int (*nvram_status)();		/* MANDATORY */
	int (*nvram_battery_enable)();	/* OPTIONAL */
	int (*nvram_battery_disable)();	/* OPTIONAL */
	int (*nvram_battery_status)();	/* MANDATORY */
        };

#ifdef KERNEL
/* declare interface routine array for /dev/pr0 */
extern struct presto_interface presto_interface0;
#endif /* KERNEL */

/*
/* Battery information for NVRAM cache
 * In this structure:
 *
 * nv_nbatteries is the number of batteries supported
 * by the platform. This number of status fields will be inspected by
 * presto software in priority sequence (nv_batt_status[0] = primary,
 * nv_batt_status[1] = first secondary, ...). 
 *
 * Presto software will either not start normal operation, or will
 * shut itself down, if a "bad battery condition" is detected. This is
 * defined to be the case that there are less than nv_minimum_ok
 * batteries with sufficient power (BATT_OK).
 */
#define BATTCNT		4		/* a maximum of four batteries */
struct nvram_battery_info {
  	int	nv_nbatteries;		/* number of batteries supported */
	int	nv_test_retries;	/* number of successive calls
					 * to nvram_battery_status() for
					 * each presto battery check */
	int	nv_minimum_ok;	    	/* minimal # of BATT_OK batteries
					 * for normal operation */
	int	nv_primary_mandatory;	/* primary must be good */
  	int	nv_status[BATTCNT]; 	/* see status bits below */
        };

#define BATT_NONE	0	/* either no battery or completely bad */
#define BATT_ENABLED 	0x1	/* battery enabled,
				 * i.e. will back up NVRAM on power fail */
#define BATT_HIGH	0x2	/* battery has minimal energy stored,
				 * i.e. has enough power for Prestoserve use */
#define BATT_OK		0x3	/* battery is enabled AND has enough power
				 * i.e. is usable for Prestoserve */
#ifdef KERNEL
/* declare battery info structure for /dev/pr0 */
extern struct nvram_battery_info nvram_batteries0;
#endif /* KERNEL */

#endif /* _PRESTO_H_ */
