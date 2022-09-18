/*
 * static	char	*sccsid = "@(#)stc_data.c	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * stc_data.c
 *
 * Modification history:
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *   5-Aug-86  darrell (Darrell Dunnuck)
 *	Created data file for real VAXstar TZK50 driver (was st_data.c).
 *
 *  18-Jun-86  darrell (Darrell Dunnuck)
 *	Created data file for VAXstar TZK50 driver.
 *
 */

#include "st.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/mtio.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../../machine/common/cpuconf.h"
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/stcvar.h"
#include "../io/uba/stcreg.h"

#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"

/*
 * Tape operations use rtmbuf.	The driver
 * notices when rtmbuf is being used and allows the user
 * program to continue after errors and read records
 * not of the standard length (BSIZE).
 * There is also a ctmbuf per tape controller.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls.
 * In particular, when the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for ctmbuf.
 */

#ifdef	BINARY

extern	struct	buf	rstbuf[];
extern	struct	buf	cstbuf[];
extern	struct	uba_ctlr *stminfo[];
extern	struct	uba_device *stdinfo[];
extern	struct	buf stutab[];
extern	struct	st_softc st_softc[];
extern	short	sttotm[];
extern	int	nNST;
extern struct stspace {
	char st_pad[16384];
};
/*
extern	int	nNTE;
*/
extern	char	dis_eot_st[];

#else

struct	buf	rstbuf[NST];
struct	buf	cstbuf[NST];
struct	uba_ctlr *stminfo[NST];
struct	uba_device *stdinfo[NST];
struct	buf stutab[NST];
struct	st_softc st_softc[NST];
short	sttotm[NST];
int	nNST = NST;
struct stspace {
	char st_pad[16384];
};
/*
int	nNTE = NTE;
*/
char	dis_eot_st[NST];

#endif	BINARY
