/*
 * static char *sccsid = "@(#)ts_data.c	4.1      (ULTRIX)        7/2/90";
*/

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 by			*
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
 * ts_data.c
 *
 * Modification history
 *
 * TS11/TSU05/TSV05/TU80 data file
 *
 *  4-Feb-86 - jaw
 *
 *	Removed biic.h.
 *
 *  5-Feb-86 - ricky palmer
 *
 *	Added "dis_eot_mu" character array for EOT code. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 * 10-Feb-87 - pmk
 *	Added include zs.h and if NTS 0 init structures to 1.
 */

#include "ts.h"
#include "zs.h"

#include "../machine/pte.h"
#include "../machine/common/cpuconf.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/ioctl.h"
#include "../h/mtio.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/tsreg.h"

#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"

/*
 * Tape operations use rtsbuf.	The driver
 * notices when rtsbuf is being used and allows the user
 * program to continue after errors and read records
 * not of the standard length (BSIZE).
 * There is also a ctsbuf per tape controller.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls.
 * In particular, when the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for ctsbuf.
 */

#ifdef	BINARY

extern	struct	buf	rtsbuf[];
extern	struct	buf	ctsbuf[];
extern	struct	uba_ctlr *tsminfo[];
extern	struct	uba_device *tsdinfo[];
extern	struct	buf	tsutab[];
extern	struct	ts_softc ts_softc[];
extern	int	nNTS;
extern	char	dis_eot_ts[];

#else

#if NTS > 0
struct	buf	rtsbuf[NTS];
struct	buf	ctsbuf[NTS];
struct	uba_ctlr *tsminfo[NTS];
struct	uba_device *tsdinfo[NTS];
struct	buf	tsutab[NTS];
struct	ts_softc ts_softc[NTS];
char	dis_eot_ts[NTS];
#else
struct	buf	rtsbuf[1];
struct	buf	ctsbuf[1];
struct	uba_ctlr *tsminfo[1];
struct	uba_device *tsdinfo[1];
struct	buf	tsutab[1];
struct	ts_softc ts_softc[1];
char	dis_eot_ts[1];
#endif

int	nNTS = NTS;

#endif	BINARY

