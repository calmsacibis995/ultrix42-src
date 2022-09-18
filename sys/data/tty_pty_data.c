
/*
 * 	@(#)tty_pty_data.c	4.3	(ULTRIX)	10/15/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987 by			*
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
 * Modification History
 *
 * 9-Sept-90 - U. Sinkewicz
 *	X.29 changes.
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 */
#include "pty.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/exec.h"



#ifdef BINARY
extern	struct	tty pt_tty[];
extern	struct	pt_ioctl {
	int	pt_flags;
	int	pt_sel_flags;
	int	pt_gensym;
	struct	proc *pt_selr, *pt_selw;
	int	pt_send;
	char    x29_mdata[X29DSIZE];
	char    x29_sdata[X29DSIZE];
	struct	lock_t	pt_lk_pty_sel;
} pt_ioctl[];

extern int nNPTY;

#else

#if NPTY == 1
#undef	NPTY
#define	NPTY	32		/* crude XXX */
#endif


struct	tty pt_tty[NPTY];
struct	pt_ioctl {
	int	pt_flags;
	int	pt_sel_flags;
	int	pt_gensym;
	struct	proc *pt_selr, *pt_selw;
	int	pt_send;
	char    x29_mdata[X29DSIZE];
	char    x29_sdata[X29DSIZE];
	struct	lock_t	pt_lk_pty_sel;
} pt_ioctl[NPTY];
int nNPTY = NPTY;
#endif
















