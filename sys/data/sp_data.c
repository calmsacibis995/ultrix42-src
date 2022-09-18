/*
 * static	char	*sccsid = "@(#)sp_data.c	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1987, 1989 by		*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * sp_data.c
 *
 * Modification history
 *
 * MicroVAX 2000/3100 pseudo driver for user devices
 *
 * 03-Dec-89	Fred Canter
 *	Created from sh_data.c.
 *
 */

#include	"sp.h"

/*
 * The following are the header files included by the "sh" driver.
 * This is an example. Header file inclusion is driver dependent.
 */

#include "../machine/pte.h"

#include "bk.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../h/exec.h"
#include "../h/sys_tpath.h"

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/spreg.h"

#include "../h/bk.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"

/*
 * Example of device driver data structure declarations.
 * Actual data structures are driver dependent.
 * Note: each data structure is declared twice.
 */

#ifdef BINARY

extern	struct	uba_device *spinfo[];
extern	struct	sp_softc sp_softc[];

extern	struct	tty sp_tty[];

extern	int	nNSP;

#else BINARY

struct	uba_device *spinfo[NSP];
struct	sp_softc sp_softc[NSP];

#define	SP_LPU	1		    /* number of lines per unit */
struct	tty sp_tty[NSP*SP_LPU];	    /* one tty structure per line */
int	nNSP	= NSP;

#endif BINARY
