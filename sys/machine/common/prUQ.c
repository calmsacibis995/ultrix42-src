#ifndef lint
static	char	*sccsid = "@(#)prUQ.c	4.1	(ULTRIX)	7/2/90";
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
 * presto - to - MSCP disk driver interface routines
 */

#include "../h/prestoioctl.h"

extern struct kprtab UQprtabs[];

int UQprtabs_init = 0;

#define UQ_devtoprt(dev) (&(UQprtabs[(major(dev)-FIRST_UQ_MAJOR)])) 

extern int mscp_strategy(), mscp_read(), mscp_write(),
	mscp_bopen(), mscp_bclose();

void
UQprtabs_initialize()
{
	int i;
	for (i = 0; i < N_UQ_MAJOR; i++) {
		UQprtabs[i].pr_strategy = mscp_strategy;
		UQprtabs[i].pr_ioctl.bmajordev = FIRST_UQ_MAJOR + i;
	}
	UQprtabs_init = 1;
}

int
MSCP_bopen(dev, flag)
	dev_t dev;
	int flag;
{
	if (!UQprtabs_init)
		UQprtabs_initialize();

	return (mscp_bopen(dev, flag));
}

int
MSCP_bclose(dev, flag)
	dev_t dev;
	int flag;
{
	if (!UQprtabs_init)
		UQprtabs_initialize();

	return (PRclose(dev, flag, mscp_bclose, UQ_devtoprt(dev)));
}

MSCP_strategy(bp)
	struct buf *bp;
{
	if (!UQprtabs_init)
		UQprtabs_initialize();

	PRstrategy(bp, mscp_strategy, UQ_devtoprt(bp->b_dev));
}

int
MSCP_read(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	if (!UQprtabs_init)
		UQprtabs_initialize();

	return (PRread(dev, uio, mscp_read, UQ_devtoprt(dev)));
}

int
MSCP_write(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	if (!UQprtabs_init)
		UQprtabs_initialize();

	return (PRwrite(dev, uio, mscp_write, UQ_devtoprt(dev)));
}
