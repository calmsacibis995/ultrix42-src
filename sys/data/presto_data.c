#ifndef lint
static	char	*sccsid = "@(#)presto_data.c	4.3	(ULTRIX)	4/11/91";
#endif lint

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
 *	Add XXcstrat layer for N-buf I/O strategy calls to char device.
 *
 *  11 Aug 90 -- chet
 *	Re-do data structure allocations; move stuff over from conf.c.
 * 
 *  25 May 90 -- chet
 *	Added this file; it was derived from Legato sources.
 *
 */

#include "../h/prestoioctl.h"

/* Do RZ class disks */
#include "scsi.h"
#include "sii.h"

#ifdef mips
#include "asc.h"
#else
#define NASC 0
#endif /* mips */

#if NSCSI > 0 || NSII > 0 || NASC > 0

/*
 * presto - to - SCSI disk driver interface routines
 */

extern int rzopen(), rzstrategy(), rzread(), rzwrite();
int nulldev();
int RZready();

/*
 * Define presto NVRAM pseudo-driver per-device structures.
 * NOTE:
 *	These data structures are heavily tied to the device major
 *	numbers defined in conf.c; if those change, these must
 *	also change.
 */
#define FIRST_RZ_MAJOR 21

struct prtab RZprtab0 = {
	(dev_t) FIRST_RZ_MAJOR,
	rzstrategy,
	RZready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

/*
 * Is device ready to be opened during presto crash recovery?
 * Presto crash recovery normally happens before we get to single user mode.
 * Some drivers (e.g. shadowing) may want opens deferred until they can
 * get at their configuration data.
 */
int
RZready(dev)
	dev_t dev;
{
	return (1);	/* always ready to be opened */
}

int
RZbopen(dev, flag)
	dev_t dev;
	int flag;
{
	return (rzopen(dev, flag));
}

int
RZbclose(dev, flag)
	dev_t dev;
	int flag;
{
	extern struct prtab *prtabs[];

	return (PRclose(dev, flag, nulldev, prtabs[major(dev)]));
}

RZstrategy(bp)
	struct buf *bp;
{
	extern struct prtab *prtabs[];

	PRstrategy(bp, rzstrategy, prtabs[major(bp->b_dev)]);
}

RZcstrat(bp)
	struct buf *bp;
{
	extern struct prtab *prtabs[];

	PRcstrat(bp, rzstrategy, prtabs[major(bp->b_dev)]);
}

int
RZread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	extern struct prtab *prtabs[];

	return (PRread(dev, uio, rzread, prtabs[major(dev)]));
}

int
RZwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	extern struct prtab *prtabs[];

	return (PRwrite(dev, uio, rzwrite, prtabs[major(dev)]));
}
#endif /* NSCSI > 0 || NSII > 0 || NASC > 0 */


/* Do UQ class disks */
#include "uq.h"
#include "ci.h"
#include "bvpssp.h"
#include "msi.h"

#if NCI>0 || NBVPSSP >0 || NUQ >0 || NMSI > 0

/*
 * presto - to - MSCP disk driver interface routines
 */

extern int mscp_strategy(), mscp_read(), mscp_write(),
	mscp_bopen(), mscp_bclose();
int MSCP_ready();

/*
 * Define presto NVRAM pseudo-driver per-device structures.
 * NOTE:
 *	These data structures are heavily tied to the device major
 *	numbers defined in conf.c; if those change, these must
 *	also change.
 */
#define FIRST_UQ_MAJOR 23

struct prtab UQprtab0 = {
	(dev_t) FIRST_UQ_MAJOR,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

struct prtab UQprtab1 = {
	(dev_t) FIRST_UQ_MAJOR+1,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

struct prtab UQprtab2 = {
	(dev_t) FIRST_UQ_MAJOR+2,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

struct prtab UQprtab3 = {
	(dev_t) FIRST_UQ_MAJOR+3,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

struct prtab UQprtab4 = {
	(dev_t) FIRST_UQ_MAJOR+4,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

struct prtab UQprtab5 = {
	(dev_t) FIRST_UQ_MAJOR+5,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

struct prtab UQprtab6 = {
	(dev_t) FIRST_UQ_MAJOR+6,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

struct prtab UQprtab7 = {
	(dev_t) FIRST_UQ_MAJOR+7,
	mscp_strategy,
	MSCP_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

/*
 * Is device ready to be opened during presto crash recovery?
 * Presto crash recovery normally happens before we get to single user mode.
 * Some drivers (e.g. shadowing) may want opens deferred until they can
 * get at their configuration data.
 */
int
MSCP_ready(dev)
	dev_t dev;
{
	return (1);	/* always ready to be opened */
}

int
MSCP_bopen(dev, flag)
	dev_t dev;
	int flag;
{
	return (mscp_bopen(dev, flag));
}

int
MSCP_bclose(dev, flag)
	dev_t dev;
	int flag;
{
	extern struct prtab *prtabs[];

	return (PRclose(dev, flag, mscp_bclose, prtabs[major(dev)]));
}

MSCP_strategy(bp)
	struct buf *bp;
{
	extern struct prtab *prtabs[];

	PRstrategy(bp, mscp_strategy, prtabs[major(bp->b_dev)]);
}

MSCP_cstrat(bp)
	struct buf *bp;
{
	extern struct prtab *prtabs[];

	PRcstrat(bp, mscp_strategy, prtabs[major(bp->b_dev)]);
}

int
MSCP_read(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	extern struct prtab *prtabs[];

	return (PRread(dev, uio, mscp_read, prtabs[major(dev)]));
}

int
MSCP_write(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	extern struct prtab *prtabs[];

	return (PRwrite(dev, uio, mscp_write, prtabs[major(dev)]));
}

#endif /* NCI>0 || NBVPSSP >0 || NUQ >0 || NMSI > 0 */

/* Do SHAD class logical devices. */ 
#include "shd.h"

#if NSHD > 0 

/*
 * presto - to - ULTRIX Disk shadow driver interface routines
 */

extern int shad_bopen(), shad_close(), 
           shad_strategy(), shad_read(), shad_write(), shad_ready();

/*
 * Define presto NVRAM pseudo-driver per-device structures.
 * NOTE:
 *	These data structures are heavily tied to the device major
 *	numbers defined in conf.c; if those change, these must
 *	also change.
 */
#define FIRST_SHAD_MAJOR  31

struct prtab SHADprtab0 = {
	(dev_t) FIRST_SHAD_MAJOR,
	shad_strategy,
        shad_ready,
        { 0, },
        { 0, },
        { 0, },
        { 0, },
};

int
SHAD_bopen(dev, flag)
	dev_t dev;
	int flag;
{
	return (shad_bopen(dev, flag));
}

int
SHAD_close(dev, flag)
	dev_t dev;
	int flag;
{
	extern struct prtab *prtabs[];

	return (PRclose(dev, flag, shad_close, prtabs[major(dev)]));
}

SHAD_strategy(bp)
	struct buf *bp;
{
	extern struct prtab *prtabs[];

	PRstrategy(bp, shad_strategy, prtabs[major(bp->b_dev)]);
}

SHAD_cstrat(bp)
	struct buf *bp;
{
	extern struct prtab *prtabs[];

	PRcstrat(bp, shad_strategy, prtabs[major(bp->b_dev)]);
}
int
SHAD_read(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	extern struct prtab *prtabs[];

	return (PRread(dev, uio, shad_read, prtabs[major(dev)]));
}

int
SHAD_write(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	extern struct prtab *prtabs[];

	return (PRwrite(dev, uio, shad_write, prtabs[major(dev)]));
}
#endif /* NSHD > 0 */

/*
 * Define presto NVRAM pseudo-driver per-device control array.
 * NOTE:
 *	This data structure is heavily tied to the device major
 *	numbers defined in conf.c; if those change, this must
 *	also change.
 */

struct prtab *prtabs[] = {
	0,	/*0*/
	0,	/*1*/
	0,	/*2*/
	0,	/*3*/
	0,	/*4*/
	0,	/*5*/
	0,	/*6*/
	0,	/*7*/
	0,	/*8*/
	0,	/*9*/
	0,	/*10*/
	0,	/*11*/
	0,	/*12*/
	0,	/*13*/
	0,	/*14*/
	0,	/*15*/
	0,	/*16*/
	0,	/*17*/
	0,	/*18*/
	0,	/*19*/
	0,	/*20*/
#if NSCSI > 0 || NSII > 0 || NASC > 0
	&RZprtab0,	/*21*/	/* first scsi block device */
#else
	0,	/*21 */
#endif
	0,	/*22*/
#if NCI>0 || NBVPSSP >0 || NUQ >0 || NMSI > 0
	&UQprtab0,	/*23*/  /* first mscp (uq) block device */
	&UQprtab1,	/*24*/
	&UQprtab2,	/*25*/
	&UQprtab3,	/*26*/
	&UQprtab4,	/*27*/
	&UQprtab5,	/*28*/
	&UQprtab6,	/*29*/
	&UQprtab7,	/*30*/
#else
	0,	/*23*/
	0,	/*24*/
	0,	/*25*/
	0,	/*26*/
	0,	/*27*/
	0,	/*28*/
	0,	/*29*/
	0,	/*30*/
#endif
#if NSHD > 0
        &SHADprtab0,     /*31*/  /* ULTRIX Disk shadow block device */
#else
	0,	/*31*/
#endif
	0,	/*32*/
	0,	/*33*/
	0,	/*34*/
	0,	/*35*/
	0,	/*36*/
	0,	/*37*/
	0,	/*38*/
	0,	/*39*/
	0,	/*40*/
	0,	/*41*/
	0,	/*42*/
	0,	/*43*/
	0,	/*44*/
	0,	/*45*/
	0,	/*46*/
	0,	/*47*/
	0,	/*48*/
	0,	/*49*/
	0,	/*50*/
	0,	/*51*/
	0,	/*52*/
	0,	/*53*/
	0,	/*54*/
	0,	/*55*/
#if NSCSI > 0 || NSII > 0 || NASC > 0
	&RZprtab0,	/*56*/	/* first scsi raw device */
#else
	0,	/*56*/
#endif
	0,	/*57*/
	0,	/*58*/
	0,	/*59*/
#if NCI>0 || NBVPSSP >0 || NUQ >0 || NMSI > 0
	&UQprtab0,	/*60*/  /* first mscp (uq) raw device */
	&UQprtab1,	/*61*/
	&UQprtab2,	/*62*/
	&UQprtab3,	/*63*/
	&UQprtab4,	/*64*/
	&UQprtab5,	/*65*/
	&UQprtab6,	/*66*/
	&UQprtab7,	/*67*/
#else
	0,	/*60*/
	0,	/*61*/
	0,	/*62*/
	0,	/*63*/
	0,	/*64*/
	0,	/*65*/
	0,	/*66*/
	0,	/*67*/
#endif
	0,	/*68*/
	0,	/*69*/
	0,	/*70*/
	0,	/*71*/
	0,	/*72*/
	0,	/*73*/
	0,	/*74*/
	0,	/*75*/
#if NSHD > 0
        &SHADprtab0,     /*76*/  /* ULTRIX Disk Shadow raw device */
#else
	0,	/*76*/
#endif

};


int pr_nprbdev = 31; /* maximum block device major number that can be
                      * "prestoized"
		      */

int pr_nprdev = sizeof (prtabs) / sizeof (prtabs[0]);

