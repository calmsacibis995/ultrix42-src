
/*
 * static	char	*sccsid = "@(#)mt_data.c	4.1	(ULTRIX)	7/2/90"
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
 * mt_data.c
 *
 * Modification history
 *
 * TM78/TU78 data file
 *
 * 26-Jan-86 - ricky palmer
 *
 *	Added "dis_eot_mu" character array for EOT code. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 * 10-Feb-87 - pmk
 *	Added include mt.h and if NMU 0 init structures to 1.
 */

#include "mu.h"
#include "mt.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/file.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/ioctl.h"
#include "../h/mtio.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../io/mba/vax/mbareg.h"
#include "../io/mba/vax/mbavar.h"
#include "../io/mba/vax/mtreg.h"

#ifdef	BINARY

extern	struct	buf	rmtbuf[];
extern	struct	buf	cmtbuf[];
extern	struct	mba_device *mtinfo[];
extern	struct	mu_softc  mu_softc[];
extern	int	nNMT;
extern	int	nNMU;
extern	short	mutomt[];
extern	char	dis_eot_mu[];

#else

struct	buf	rmtbuf[NMT];
struct	buf	cmtbuf[NMT];
struct	mba_device *mtinfo[NMT];

#if NMU > 0
struct	mu_softc  mu_softc[NMU];
short	mutomt[NMU];
char	dis_eot_mu[NMU];
#else
struct	mu_softc  mu_softc[1];
short	mutomt[1];
char	dis_eot_mu[1];
#endif

int	nNMT = NMT;
int	nNMU = NMU;

#endif	BINARY
