
/*
 * 	@(#)dz_data.c	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1987 by		*
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
 * dz_data.c
 *
 * Modification history
 *
 * DZ11/DZ32/DZV11/DZQ11 data file
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 */

#include	"dz.h"

#include "../machine/pte.h"

#include "bk.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/bk.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../h/exec.h"
#include "../h/sys_tpath.h"

#include "../machine/cpu.h"
#include "../io/uba/pdma.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/dzreg.h"

#ifdef BINARY

extern	struct	uba_device *dzinfo[];
extern	struct	dz_softc dz_softc[];
extern	struct	tty dz_tty[];
extern	int	dz_cnt;

/*
* Software copy of dzbrk since it isn't readable
*/
extern	char	dz_brk[];
extern	char	dzsoftCAR[];
extern	char	dzdefaultCAR[];
extern	char	dz_lnen[];	/* saved line enable bits for DZ32 */

/*
* Pdma structures for fast output code
*/
extern	struct	pdma dzpdma[];

extern	int	dzchars[];		/* recent input count */
extern	int	dzrate[];		/* smoothed input count */

extern	int	nNDZ;
extern	int	nNDZLINE;

#else BINARY

#define NDZLINE 	(8*NDZ)
struct	uba_device *dzinfo[NDZ];
struct	dz_softc dz_softc[NDZ];
struct	tty dz_tty[NDZLINE];
int	dz_cnt = { NDZLINE };

/*
 * Pdma structures for fast output code
 */
struct	pdma dzpdma[NDZLINE];


/*
 * Software copy of dzbrk since it isn't readable
 */
char	dz_brk[NDZ];
char	dzsoftCAR[NDZ];
char	dzdefaultCAR[NDZ];
char	dz_lnen[NDZ];	/* saved line enable bits for DZ32 */
int	dzrate[NDZ];
int	dzchars[NDZ];

int	nNDZ = NDZ;
int	nNDZLINE = NDZLINE;

#endif BINARY

