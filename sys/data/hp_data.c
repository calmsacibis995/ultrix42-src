
/*
 * static	char	*sccsid = "@(#)hp_data.c	4.1	(ULTRIX)	7/2/90"
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
 * hp_data.c
 *
 * Modification history
 *
 * RH/RM03/RM05/RM80/RP05/RP06/RP07 data file
 *
 *  2-Oct-84 - Stephen Reilly
 *
 *	Added the pt structure to handle different partition per pack.
 *	-001
 *
 * 28-Mar-86 - Ricky Palmer
 *
 *	Added include of devio.h. V2.0
 *
 * 21-Jul-89 - Stephen Reilly
 *	Modified the RM05 partition table to accomodate the increase size
 *      of our installed images.
 */

#include "hp.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../machine/mtpr.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/dkbad.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../fs/ufs/fs.h"
#include "../h/dkio.h"
#include "../h/devio.h"

#include "../io/mba/vax/mbareg.h"
#include "../io/mba/vax/mbavar.h"
#include "../io/mba/vax/hpreg.h"

#ifdef BINARY

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
extern	struct	size {
	daddr_t nblocks;
	int	cyloff;
} rp06_sizes[8], rp05_sizes[8], rm03_sizes[8], rm05_sizes[8],
	rm80_sizes[8], rp07_sizes[8], cdc9775_sizes[8],
	cdc9730_sizes[8], capricorn_sizes[8],
	eagle_sizes[8], ampex_sizes[8];
extern	struct	mba_device *hpinfo[];
extern	struct	buf	rhpbuf[];
extern	struct	buf	bhpbuf[];
extern	struct	dkbad	hpbad[];
extern	struct	hp_softc hp_softc[];
struct	pt hp_part[];		 /* 001 Partition table for each pack */
extern	int	nNHP;

#else BINARY

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
struct	size {
	daddr_t nblocks;
	int	cyloff;
} rp06_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 37 */
	33440,	38,		/* B=cyl 38 thru 117 */
	340670, 0,		/* C=cyl 0 thru 814 */
	15884,	118,		/* D=cyl 118 thru 155 */
	55936,	156,		/* E=cyl 156 thru 289 */
	219384, 290,		/* F=cyl 290 thru 814 */
	291280, 118,		/* G=cyl 118 thru 814 */
	0,	0,
}, rp05_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 37 */
	33440,	38,		/* B=cyl 38 thru 117 */
	171798, 0,		/* C=cyl 0 thru 410 */
	15884,	118,		/* D=cyl 118 thru 155 */
	55936,	156,		/* E=cyl 156 thru 289 */
	50512,	290,		/* F=cyl 290 thru 410 */
	122408, 118,		/* G=cyl 118 thru 410 */
	0,	0,
}, rm03_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 99 */
	33440,	100,		/* B=cyl 100 thru 308 */
	131680, 0,		/* C=cyl 0 thru 822 */
	15884,	309,		/* D=cyl 309 thru 408 */
	55936,	409,		/* E=cyl 409 thru 758 */
	10144,	759,		/* F=cyl 759 thru 822 */
	82144,	309,		/* G=cyl 309 thru 822 */
	0,	0,
}, rm05_sizes[8] = {
	32768,	0,		/* A=cyl 0 thru 53 */
	66880,	54,		/* B=cyl 54 thru 163 */
	500384, 0,		/* C=cyl 0 thru 822 */
	15884,	562,		/* D=cyl 562 thru 588 */
	55936,	589,		/* E=cyl 589 thru 680 */
	86240,	681,		/* F=cyl 681 thru 822 */
	158592, 562,		/* G=cyl 562 thru 822 */
	241984, 164,		/* H=cyl 164 thru 561 */
}, rm80_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 36 */
	33440,	37,		/* B=cyl 37 thru 114 */
	242606, 0,		/* C=cyl 0 thru 558 */
	15884,	115,		/* D=cyl 115 thru 151 */
	55936,	152,		/* E=cyl 152 thru 280 */
	120559, 281,		/* F=cyl 281 thru 558 */
	192603, 115,		/* G=cyl 115 thru 558 */
	0,	0,
}, rp07_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 9 */
	66880,	10,		/* B=cyl 10 thru 51 */
	1008000, 0,		/* C=cyl 0 thru 629 */
	15884,	235,		/* D=cyl 235 thru 244 */
	307200, 245,		/* E=cyl 245 thru 436 */
	308650, 437,		/* F=cyl 437 thru 629 */
	631850, 235,		/* G=cyl 235 thru 629 */
	291346, 52,		/* H=cyl 52 thru 234 */
}, cdc9775_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 12 */
	66880,	13,		/* B=cyl 13 thru 65 */
	1077760, 0,		/* C=cyl 0 thru 841 */
	15884,	294,		/* D=cyl 294 thru 306 */
	307200, 307,		/* E=cyl 307 thru 546 */
	377440, 547,		/* F=cyl 547 thru 841 */
	701280, 294,		/* G=cyl 294 thru 841 */
	291346, 66,		/* H=cyl 66 thru 293 */
}, cdc9730_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 49 */
	33440,	50,		/* B=cyl 50 thru 154 */
	263360, 0,		/* C=cyl 0 thru 822 */
	15884,	155,		/* D=cyl 155 thru 204 */
	55936,	205,		/* E=cyl 205 thru 379 */
	141664, 380,		/* F=cyl 380 thru 822 */
	213664, 155,		/* G=cyl 155 thru 822 */
	0,	0,
}, capricorn_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 31 */
	33440,	32,		/* B=cyl 32 thru 97 */
	524288, 0,		/* C=cyl 0 thru 1023 */
	15884,	668,		/* D=cyl 668 thru 699 */
	55936,	700,		/* E=cyl 700 thru 809 */
	109472, 810,		/* F=cyl 810 thru 1023 */
	182176, 668,		/* G=cyl 668 thru 1023 */
	291346, 98,		/* H=cyl 98 thru 667 */
}, eagle_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 16 */
	66880,	17,		/* B=cyl 17 thru 86 */
	808320, 0,		/* C=cyl 0 thru 841 */
	15884,	391,		/* D=cyl 391 thru 407 */
	307200, 408,		/* E=cyl 408 thru 727 */
	109296, 728,		/* F=cyl 728 thru 841 */
	432816, 391,		/* G=cyl 391 thru 841 */
	291346, 87,		/* H=cyl 87 thru 390 */
}, ampex_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 26 */
	33440,	27,		/* B=cyl 27 thru 81 */
	495520, 0,		/* C=cyl 0 thru 814 */
	15884,	562,		/* D=cyl 562 thru 588 */
	55936,	589,		/* E=cyl 589 thru 680 */
	81312,	681,		/* F=cyl 681 thru 814 */
	153664, 562,		/* G=cyl 562 thru 814 */
	291346, 82,		/* H=cyl 82 thru 561 */
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */
struct	mba_device *hpinfo[NHP];
struct	buf	rhpbuf[NHP];
struct	buf	bhpbuf[NHP];
struct	dkbad	hpbad[NHP];
struct	hp_softc hp_softc[NHP];
struct	pt hp_part[NHP];	/* 001 Partition table for each pack */
int	nNHP = NHP;

#endif BINARY
