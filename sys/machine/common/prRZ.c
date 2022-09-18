#ifndef lint
static	char	*sccsid = "@(#)prRZ.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by			*
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
 *  25 May 90 -- chet
 *	Added this file; it was derived from Legato sources.
 *
 */

/*
 * presto - to - SCSI disk driver interface routines
 */

#include "../h/prestoioctl.h"

extern struct kprtab RZprtabs[];

int RZprtabs_init = 0;

#define RZ_devtoprt(dev) (&(RZprtabs[(major(dev)-FIRST_RZ_MAJOR)])) 

extern int rzopen(), rzstrategy(), rzread(), rzwrite();
int nulldev();

void
RZprtabs_initialize()
{
	int i;
	for (i = 0; i < N_RZ_MAJOR; i++) {
		RZprtabs[i].pr_strategy = rzstrategy;
		RZprtabs[i].pr_ioctl.bmajordev = FIRST_RZ_MAJOR + i;
	}
	RZprtabs_init = 1;
}

int
RZbopen(dev, flag)
	dev_t dev;
	int flag;
{
	if (!RZprtabs_init)
		RZprtabs_initialize();

	return (rzopen(dev, flag));
}

int
RZbclose(dev, flag)
	dev_t dev;
	int flag;
{
	if (!RZprtabs_init)
		RZprtabs_initialize();

	return (PRclose(dev, flag, nulldev, RZ_devtoprt(dev)));
}

RZstrategy(bp)
	struct buf *bp;
{
	if (!RZprtabs_init)
		RZprtabs_initialize();

	PRstrategy(bp, rzstrategy, RZ_devtoprt(bp->b_dev));
}

int
RZread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	if (!RZprtabs_init)
		RZprtabs_initialize();

	return (PRread(dev, uio, rzread, RZ_devtoprt(dev)));
}

int
RZwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	if (!RZprtabs_init)
		RZprtabs_initialize();

	return (PRwrite(dev, uio, rzwrite, RZ_devtoprt(dev)));
}
