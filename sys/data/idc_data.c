
/*
 * static	char	*sccsid = "@(#)idc_data.c	4.1	(ULTRIX)	7/2/90"
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
 * idc_data.c
 *
 * Modification history
 *
 * IDC/RL02/R80 data file
 *
 * 29-Oct-84 - Stephen Reilly
 *
 *	Added data structures for the disk partition scheme.
 *	-001
 *
 * 16-Apr-86 - Ricky Palmer
 *
 *	Added include of devio.h. V2.0
 *
 * 10-Feb-87 - pmk
 *	Added if NRB 0 init structures to 1.
 */

#include "idc.h"
#include "rb.h"

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
#include "../io/uba/idcreg.h"

#ifndef NRB
#define NRB 1
#endif	NRB

#ifdef	BINARY

extern	struct	buf idcutab[];
extern	struct	uba_ctlr *idcminfo[];
extern	struct	uba_device *idcdinfo[];

extern	union	idc_dar idccyl[];

extern	struct	buf ridcbuf[];
extern	struct	idc_softc idc_softc;
extern	struct	pt  idc_part[]; 		/* 001 part tbl per pack */

extern	int	nNIDC;
extern	int	nNRB;

extern	struct size {
	daddr_t nblocks;
	int	cyloff;
} rb02_sizes[], rb80_sizes[];

#else BINARY

struct	uba_ctlr *idcminfo[NIDC];
struct	idc_softc idc_softc;

#if NRB > 0
struct	buf idcutab[NRB];
struct	uba_device *idcdinfo[NRB];
union	idc_dar idccyl[NRB];
struct	buf ridcbuf[NRB];
struct	pt idc_part[NRB];		/* 001 part. tbl for each pack */
#else
struct	buf idcutab[1];
struct	uba_device *idcdinfo[1];
union	idc_dar idccyl[1];
struct	buf ridcbuf[1];
struct	pt idc_part[1];			/* 001 part. tbl for each pack */
#endif

int	nNIDC = NIDC;
int	nNRB = NRB;

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
struct size {
	daddr_t nblocks;
	int	cyloff;
} rb02_sizes[8] ={
	15884,	0,		/* A=cyl 0 thru 397 */
	4520,	398,		/* B=cyl 398 thru 510 */
	20480,	0,		/* C=cyl 0 thru 511 */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
}, rb80_sizes[8] ={
	15884,	0,		/* A=cyl 0 thru 36 */
	33440,	37,		/* B=cyl 37 thru 114 */
	242606, 0,		/* C=cyl 0 thru 558 */
	0,	0,
	0,	0,
	0,	0,
	82080,	115,		/* G=cyl 115 thru 304 */
	110143, 305,		/* H=cyl 305 thru 558 */
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */

#endif BINARY
