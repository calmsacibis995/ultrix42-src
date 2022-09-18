
/*
 * static	char	*sccsid = "@(#)up_data.c	4.1	(ULTRIX)	7/2/90"
 */
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History
 *
 *	14-Nov-84	Stephen Reilly
 * 001 - Added data structure for the new disk partitioning.
 *
 ***********************************************************************/

#include	"up.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/dkbad.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"			/* 001 */
#include "../fs/ufs/fs.h"			/* 001 */

#include "../machine/cpu.h"
#include "../machine/nexus.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/upreg.h"

#define	UPIPUNITS	8  /* Do Not Changed */

#ifdef BINARY

extern	struct	up_softc {
	int	sc_softas;
	int	sc_ndrive;
	int	sc_wticks;
	int	sc_recal;
} up_softc[];


extern	struct	size {
	daddr_t	nblocks;
	int	cyloff;
} up9300_sizes[],up9766_sizes[],up160_sizes[],upam_sizes[],up980_sizes[];

extern	struct	uba_ctlr *upminfo[];
extern	struct	uba_device *updinfo[];
extern	struct	buf	uputab[];
extern	char upinit[];
extern	struct	uba_device *upip[][UPIPUNITS]; /* fuji w/fixed head gives n,n+4 */
extern	struct	buf	rupbuf[];
extern	struct 	buf	bupbuf[];
extern	struct	dkbad	upbad[];
extern	struct	pt	up_part[];	/* 001 part tbl per pack */

extern	int	nNUP;
extern	int	nNSC;
#else

struct	up_softc {
	int	sc_softas;
	int	sc_ndrive;
	int	sc_wticks;
	int	sc_recal;
} up_softc[NSC];

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
struct	size {
	daddr_t	nblocks;
	int	cyloff;
} up9300_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 26 */
	33440,	27,		/* B=cyl 27 thru 81 */
	495520,	0,		/* C=cyl 0 thru 814 */
	15884,	562,		/* D=cyl 562 thru 588 */
	55936,	589,		/* E=cyl 589 thru 680 */
	81376,	681,		/* F=cyl 681 thru 814 */
	153728,	562,		/* G=cyl 562 thru 814 */
	291346,	82,		/* H=cyl 82 thru 561 */
}, up9766_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 26 */
	33440,	27,		/* B=cyl 27 thru 81 */
	500384,	0,		/* C=cyl 0 thru 822 */
	15884,	562,		/* D=cyl 562 thru 588 */
	55936,	589,		/* E=cyl 589 thru 680 */
	86240,	681,		/* F=cyl 681 thru 822 */
	158592,	562,		/* G=cyl 562 thru 822 */
	291346,	82,		/* H=cyl 82 thru 561 */
}, up160_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 49 */
	33440,	50,		/* B=cyl 50 thru 154 */
	263360,	0,		/* C=cyl 0 thru 822 */
	15884,	155,		/* D=cyl 155 thru 204 */
	55936,	205,		/* E=cyl 205 thru 379 */
	141664,	380,		/* F=cyl 380 thru 822 */
	213664,	155,		/* G=cyl 155 thru 822 */
	0,	0,
}, upam_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 31 */
	33440,	32,		/* B=cyl 32 thru 97 */
	524288,	0,		/* C=cyl 0 thru 1023 */
	15884,	668,		/* D=cyl 668 thru 699 */
	55936,	700,		/* E=cyl 700 thru 809 */
	109472,	810,		/* F=cyl 810 thru 1023 */
	182176,	668,		/* G=cyl 668 thru 1023 */
	291346,	98,		/* H=cyl 98 thru 667 */
}, up980_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 99 */
	33440,	100,		/* B=cyl 100 thru 308 */
	131680,	0,		/* C=cyl 0 thru 822 */
	15884,	309,		/* D=cyl 309 thru 408 */
	55936,	409,		/* E=cyl 409 thru 758 */
	10080,	759,		/* F=cyl 759 thru 822 */
	82080,	309,		/* G=cyl 309 thru 822 */
	0,	0,
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */
struct	uba_ctlr *upminfo[NSC];
struct	uba_device *updinfo[NUP];
struct	buf	uputab[NUP];
char upinit[NUP];

struct	uba_device *upip[NSC][UPIPUNITS]; /* fuji w/fixed head gives n,n+4 */
struct	buf	rupbuf[NUP];
struct 	buf	bupbuf[NUP];
struct	dkbad	upbad[NUP];
struct	pt	up_part[NUP];		/* 001 part tbl for each pack */

int	nNUP = NUP;
int	nNSC = NSC;

#endif

