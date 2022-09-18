/*	@(#)fifonode.h	4.1	(ULTRIX)	7/2/90	*/

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
 * 11-Jul-88 -- prs
 *	Added two fields to struct fifonode: fn_wpgrp fn_rpgrp.
 *
 * 27-Apr-88 -- prs
 *	Added two fields to struct fifonode: fn_wselp fn_rselp.
 *
 * 10-Feb-88 -- prs
 *	New fifo code.
 *
 ************************************************************************/

#ifdef KERNEL
#include "../h/types.h"

struct fifonode {
	struct fifo_bufhdr *fn_buf;	/* ptr to first buffer */
	struct fifo_bufhdr *fn_bufend;	/* ptr to last buffer */
	struct proc	*fn_wselp;	/* ptr to write proc sleeping on select */
	struct proc	*fn_rselp;	/* ptr to read proc sleeping on select */
	u_long		fn_size;	/* number of bytes in fifo */
	u_long		fn_wcnt;	/* number of waiting readers */
	u_long		fn_rcnt;	/* number of waiting writers */
	u_long          fn_rpgrp;       /* process group of read side */
	u_long          fn_wpgrp;       /* process group of write side */
	u_long		fn_wptr;	/* write offset */
	u_long		fn_rptr;	/* read offset */
	u_long		fn_flag;	/* (see fifo.h) */
	u_long		fn_nbuf;	/* number of buffers allocated */
};

/*
 * Convert between fifonode, snode, and vnode pointers
 */
#define GTOF(GP)        ((struct fifonode *)(GP)->g_fifo)

#endif KERNEL
