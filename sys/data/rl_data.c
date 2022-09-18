
/*
static  char    *sccsid = "@(#)rl_data.c	4.1	(ULTRIX)	7/2/90";
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
 * rl_data.c
 *
 * Modification history
 *
 * RLU211/RLV211/RL02 data file
 *
 * 26-Oct-84 - Stephen Reilly
 *
 *	Added data structures for the new partition tables. -001
 *
 *  5-Dec-84 - Stephen Reilly
 *
 *	Don't allow -1 as a length. -002
 *
 * 16-Apr-86 - Ricky Palmer
 *
 *	Added include of devio.h. V2.0
 *
 * 10-Feb-87 - pmk
 *	Added include hl.h and if NRL 0 init structures to 1.
 */

#include "rl.h"
#include "hl.h"

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
#include "../h/ioctl.h"
#include "../fs/ufs/fs.h"
#include "../h/dkio.h"
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../machine/nexus.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/rlreg.h"

#ifdef BINARY

/*
 * State of controller from last transfer.
 * Since only one transfer can be done at a time per
 * controller, only allocate one for each controller.
 */
extern	struct	rl_stat {
	short	rl_cyl[4];	/* Current cylinder for each drive */
	short	rl_dn;		/* drive number currently transferring */
	short	rl_cylnhd;	/* current cylinder and head of transfer */
	u_short rl_bleft;	/* bytes left to transfer */
	u_short rl_bpart;	/* bytes transferred */
} rl_stat[];

extern	struct	uba_ctlr	*rlminfo[];
extern	struct	uba_device	*rldinfo[];
extern	struct	uba_device	*rlip[][4];
extern	struct	rl_softc	rl_softc[];
extern	struct	pt		rl_part[];	/* 001 part. tbl per pack */

/* User table per controller */
extern	struct	buf	rlutab[];

extern	struct	buf	rrlbuf[];

extern	int	nNHL;
extern	int	nNRL;

extern	struct	size {
	daddr_t nblocks;
	int	cyloff;
} rl02_sizes[];


#else

/*
 * State of controller from last transfer.
 * Since only one transfer can be done at a time per
 * controller, only allocate one for each controller.
 */
struct	rl_stat {
	short	rl_cyl[4];	/* Current cylinder for each drive */
	short	rl_dn;		/* drive number currently transferring */
	short	rl_cylnhd;	/* current cylinder and head of transfer */
	u_short rl_bleft;	/* bytes left to transfer */
	u_short rl_bpart;	/* bytes transferred */
} rl_stat[NHL];


struct	uba_ctlr	*rlminfo[NHL];
struct	uba_device	*rlip[NHL][4];
struct	rl_softc	rl_softc[NHL];

#if NRL > 0
struct	uba_device	*rldinfo[NRL];
struct	pt		rl_part[NRL];	/* 001 part. table per pack */
struct	buf	rlutab[NRL];
struct	buf	rrlbuf[NRL];
#else
struct	uba_device	*rldinfo[1];
struct	pt		rl_part[1];	/* 001 part. table per pack */
struct	buf	rlutab[1];
struct	buf	rrlbuf[1];
#endif

int	nNHL = NHL;
int	nNRL = NRL;

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
/* Last cylinder not used. Saved for Bad Sector File */
struct	size {
	daddr_t nblocks;
	int	cyloff;
} rl02_sizes[8] = {
	15884,		0,		/* A=cyl   0 thru 397 */
	 4520,		398,		/* B=cyl 398 thru 510 */
	20480,		0,		/* C=cyl   0 thru 511 */
	 4520,		398,		/* D=cyl 398 thru 510 */
	    0,		0,		/* E= Not Defined     */
	    0,		0,		/* F= Not Defined     */
	20440,		0,		/* G=cyl   0 thru 510 */
	    0,		0,		/* H= Not Defined     */
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */

#endif
