
/*
 * @(#)tty_conf_data.c	4.2	(ULTRIX)	8/13/90
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

/************************************************************************
 * 
 * MODIFICATION HISTORY
 *
 * 15-Jan-88	Larry Palmer
 *	Added line discipline #7 (SLPDISC)
 *
 * 05-26-87 	Tim Burke
 * Added line discipline #6 (TERMIODISC)
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/conf.h"

int	nodev();
int	nulldev();

int	ttyopen(),ttyclose(),ttread(),ttwrite(),nullioctl(),ttstart();
int	ttyinput();

#ifdef vax
#include "bk.h"
#if NBK > 0
int	bkopen(),bkclose(),bkread(),bkinput(),bkioctl();
#endif

#include "tb.h"
#if NTB > 0
int	tbopen(),tbclose(),tbread(),tbinput(),tbioctl();
#endif

#include "hc.h"
#if NHC > 0
int	hcopen(),hcclose(),hcread(),hcinput(),hcioctl();
#endif
#endif vax

#include "pcm.h"
#if NPCM >0
int	pcmopen(), pcmclose(), pcminput(), pcmioctl();
#endif

#include "sl.h"
#if NSL > 0
int   slopen(), slclose(), slinput(), sltioctl(), slstart();
#endif

#ifdef BINARY

extern	int	nldisp;
extern	struct	linesw linesw[];

#else

struct	linesw linesw[] =
{
	ttyopen, nulldev, ttread, ttwrite, nullioctl,
	ttyinput, nodev, nulldev, ttstart, nulldev,		/* 0 */
#if NBK > 0
	bkopen, bkclose, bkread, ttwrite, bkioctl,
	bkinput, nodev, nulldev, ttstart, nulldev,		/* 1 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
	ttyopen, ttyclose, ttread, ttwrite, nullioctl,
	ttyinput, nodev, nulldev, ttstart, nulldev,		/* 2 */
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nulldev,		/* 3 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nulldev,		/* 4 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if NHC > 0							
	hcopen, hcclose, hcread, ttwrite, hcioctl,
	hcinput, nodev, nulldev, ttstart, nulldev,		/* 5 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
	ttyopen, ttyclose, ttread, ttwrite, nullioctl,		/* 6 */
	ttyinput, nodev, nulldev, ttstart, nulldev,
#if NSL > 0
        slopen, slclose, nodev, nodev, sltioctl,
        slinput, nodev, nulldev, slstart, nulldev,              /* 7 */
#else
        nodev, nodev, nodev, nodev, nodev,
        nodev, nodev, nodev, nodev, nodev,
#endif

#if NPCM > 0
	pcmopen, pcmclose, nodev, ttwrite, pcmioctl,		/* 8 */
	pcminput, nodev, nodev, ttstart, nodev,
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif

};

int	nldisp = sizeof (linesw) / sizeof (linesw[0]);
#endif
