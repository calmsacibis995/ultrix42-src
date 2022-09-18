#ifndef lint
static	char	*sccsid = "@(#)spec_subr.c	4.1	(ULTRIX)	7/2/90";
#endif lint
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
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/************************************************************************
 *			Modification History
 *
 * 10-Feb-88 -- prs
 *	Modified to support new fifo code.
 *
 ************************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/gnode.h"

extern struct gnode_ops *spec_gnodeops;
extern struct gnode_ops *fifo_gnodeops;

struct vnode *
specvp(gp)
	struct gnode *gp;
{
	int mode;

	mode = gp->g_mode & GFMT;
        gp->g_altops = gp->g_ops;
        if ((mode == GFPORT) || (mode == GFPIPE)) {
		gp->g_ops = fifo_gnodeops;
	}
	else
		gp->g_ops = spec_gnodeops;
}
