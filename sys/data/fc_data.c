/*
 * static	char	*sccsid = "@(#)fc_data.c	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987 by			*
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
 *fc_data.c
 *
 * Modification history:
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
 *
 * 24-May-89	darrell
 *	Changed #include of cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *  12-16-87	darrell
 *	Derived from ss_data.c.
 *
 */

#include	"fc.h"

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

#include "../h/types.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/cpu.h"
#include "../machine/ka60.h"
#include "../machine/scb.h" /* only here for the reference to SCB_ISTACK in fc.c */
#include "../machine/nexus.h"
#include "../machine/mtpr.h"
#include "../io/uba/pdma.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/ubareg.h"
#include "../machine/cvax.h"
#include "../io/uba/fcreg.h"
#include "../io/uba/smreg.h"

#ifdef BINARY

extern	struct	uba_device *fcinfo[];
extern	struct	fc_softc fc_softc[];
extern	struct	tty fc_tty[];
extern	int	fc_cnt;

/*
* Software copy of fcbrk since it isn't readable
*/
extern	char	fc_brk[];
extern	char	fcsoftCAR[];
extern	char	fcdefaultCAR[];

/*
* Pdma structures for fast output code
*/
extern	struct	pdma fcpdma[];

extern	int	fcchars[];		/* recent input count */
extern	int	fcrate[];		/* smoothed input count */

extern	int	nNFC;
extern	int	nNFCLINE;
extern u_char	fcmodem;
extern struct timeval fctimestamp;
extern	int	fcdsr;

#else BINARY

#define NFCLINE 	(4*NFC)
struct	uba_device *fcinfo[NFC];
struct	fc_softc fc_softc[NFC];
struct	tty fc_tty[NFCLINE];
int	fc_cnt = { NFCLINE };

/*
 * Pdma structures for fast output code
 */
struct	pdma fcpdma[NFCLINE];


/*
 * Software copy of fcbrk since it isn't readable
 */
char	fc_brk[NFC];
char	fcsoftCAR[NFC];
char	fcdefaultCAR[NFC];
int	fcrate[NFC];
int	fcchars[NFC];

int	nNFC = NFC;
int	nNFCLINE = NFCLINE;
u_char	fcmodem;		/* keeps track of modem state */
struct timeval fctimestamp;	/* Record length of carrier drop */

#ifdef NODSR
int fcdsr = 0;				/*"0"=Ignore DSR*/
#else NODSR
int fcdsr = 1;				/*"1"=follow DECSTD52 */
#endif NODSR

#endif BINARY

