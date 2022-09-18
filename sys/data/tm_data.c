/*
 * static	char	*sccsid = "@(#)tm_data.c	4.1	(ULTRIX)	7/2/90";
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
 * tm_data.c
 *
 * Modification history
 *
 * TM11/TE10 data file
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *  8-Feb-86 - ricky palmer
 *
 *	Added "dis_eot_te" character array for EOT code. V2.0
 *
 * 18-Mar-86 - jaw
 *
 *	Add routines to cpu switch for nexus/unibus addreses,
 *	also got rid of some globals like nexnum.
 *	ka8800 cleanup. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 */

#include "te.h"

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
#include "../h/ioctl.h"
#include "../h/mtio.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../../machine/common/cpuconf.h"
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/tmreg.h"

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

extern	struct	buf	rtmbuf[];
extern	struct	buf	ctmbuf[];
extern	struct	uba_ctlr *tmminfo[];
extern	struct	uba_device *tedinfo[];
extern	struct	buf teutab[];
extern	struct	te_softc te_softc[];
extern	short	tetotm[];
extern	int	nNTM;
extern	int	nNTE;
extern	char	dis_eot_te[];

#else

struct	buf	rtmbuf[NTM];
struct	buf	ctmbuf[NTM];
struct	uba_ctlr *tmminfo[NTM];
struct	uba_device *tedinfo[NTE];
struct	buf teutab[NTE];
struct	te_softc te_softc[NTE];
short	tetotm[NTE];
int	nNTM = NTM;
int	nNTE = NTE;
char	dis_eot_te[NTE];

#endif	BINARY
