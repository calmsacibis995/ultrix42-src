
/*
 * static	char	*sccsid = "@(#)rk_data.c	4.1	(ULTRIX)	7/2/90"
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
 * rk_data.c
 *
 * Modification history
 *
 * RK711/RK07 data file
 *
 * 11-Oct-84 - Stephen Reilly
 *
 *	Add new data structures that will enable each pack to have
 *	it's own partition tables. -001
 *
 * 11-Mar-85 - Pete Keilty
 *
 *	Add off/on line flag to softc structure. -002
 *
 * 04-Sep-85 - Stephen Reilly
 *
 *	Make the number of drives per controller configurable. -003
 *
 * 16-Apr-86 - Ricky Palmer
 *
 *	Added include of devio.h. V2.0
 *
 * 10-Feb-87 - pmk
 *	Added include hk.h and if NRK 0 init structures to 1.
 */

#include "rk.h"
#include "hk.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/dk.h"
#include "../h/cmap.h"
#include "../h/dkbad.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../fs/ufs/fs.h"
#include "../h/dkio.h"
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/rkreg.h"

#ifdef BINARY

extern	struct	uba_ctlr *rkminfo[];
extern	struct	uba_device *rkdinfo[];
extern	struct	uba_device *rkip[][7];	/* 003 */

extern	struct	buf rkutab[];
extern	short	rkcyl[];
extern	struct	dkbad rkbad[];
extern	struct	buf brkbuf[];

extern	struct	buf rrkbuf[];
extern	struct	rk_softc rk_softc[];
extern	struct	pt rk_part[];		/* 001 Partition tbl for each pack */
extern	int	nNHK;
extern	int	nNRK;

extern	struct size {
	daddr_t nblocks;
	int	cyloff;
} rk7_sizes[], rk6_sizes[];

#else BINARY

struct	uba_ctlr *rkminfo[NHK];
struct	rk_softc rk_softc[NHK];

#if NRK > 0
struct	uba_device *rkdinfo[NRK];
struct	uba_device *rkip[NHK][NRK];	/* 003 */
struct	buf rkutab[NRK];
short	rkcyl[NRK];
struct	dkbad rkbad[NRK];
struct	buf brkbuf[NRK];
struct	buf rrkbuf[NRK];
struct	pt  rk_part[NRK];		/*001 Partition tbl for each pack */
#else
struct	uba_device *rkdinfo[1];
struct	uba_device *rkip[NHK][1];	/* 003 */
struct	buf rkutab[1];
short	rkcyl[1];
struct	dkbad rkbad[1];
struct	buf brkbuf[1];
struct	buf rrkbuf[1];
struct	pt  rk_part[1];			/*001 Partition tbl for each pack */
#endif

int	nNHK = NHK;
int	nNRK = NRK;

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
struct size {
	daddr_t nblocks;
	int	cyloff;
} rk7_sizes[8] ={
	15884,	0,		/* A=cyl 0 thru 240 */
	10032,	241,		/* B=cyl 241 thru 392 */
	53790,	0,		/* C=cyl 0 thru 814 */
	0,	0,
	0,	0,
	0,	0,
	27786,	393,		/* G=cyl 393 thru 813 */
	0,	0,
}, rk6_sizes[8] ={
	15884,	0,		/* A=cyl 0 thru 240 */
	11154,	241,		/* B=cyl 241 thru 409 */
	27126,	0,		/* C=cyl 0 thru 410 */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */
#endif BINARY
