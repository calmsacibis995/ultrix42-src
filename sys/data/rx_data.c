
/*
 * static	char	*sccsid = "@(#)rx_data.c	4.1	(ULTRIX)	7/2/90"
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

#include "urx.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/uio.h"
#include "../h/file.h"

#include "../machine/cpu.h"
#include "../machine/nexus.h"

#include "../io/uba/ubavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/rxreg.h"

#ifdef BINARY
 
/* per-controller data */
extern	struct	rx_ctlr {
	int	rxc_state;	/* controller state */
	u_short	rxc_rxcs;	/* extended error status */
	u_short	rxc_rxdb;
	u_short	rxc_rxxt[4];
	int	rxc_tocnt;	/* for watchdog routine */
} rx_ctlr[];

/* per-drive buffers */
extern	struct buf	rrxbuf[];	/* buffers for raw I/O */
extern	struct buf	erxbuf[];	/* buffers for reading error status */
extern	struct buf	rxutab[];	/* per drive buffers */

/* per-drive data */
extern	struct rx_softc {
	int	sc_flags;	/* drive status flags */
	int	sc_csbits;	/* constant bits for CS register */
	int	sc_open;	/* count number of opens */
	int	sc_offset;	/* raw mode kludge to avoid restricting */
				/* single sector transfers to start on */
				/* DEV_BSIZE boundaries */
	/*
	 * The rest of this structure is used to 
	 * store temporaries while simulating multi 
	 * sector transfers
	 */
	caddr_t	sc_uaddr;	/* unibus base address */
	long	sc_bcnt;	/* total transfer count */
	long	sc_resid;	/* no. of bytes left to transfer */
} rx_softc[];

extern	struct rxerr {
	short	rxcs;
	short	rxdb;
	short	rxxt[4];	/* error code dump from controller */
} rxerr[];
/* End of per-drive data */

extern	struct	uba_device *rxdinfo[];
extern	struct	uba_ctlr *rxminfo[];
extern	int	nNURX;
extern	int	nNFX;
#else

/* per-controller data */
struct	rx_ctlr {
	int	rxc_state;	/* controller state */
	u_short	rxc_rxcs;	/* extended error status */
	u_short	rxc_rxdb;
	u_short	rxc_rxxt[4];
	int	rxc_tocnt;	/* for watchdog routine */
} rx_ctlr[NFX];

/* per-drive buffers */
struct buf	rrxbuf[NURX];	/* buffers for raw I/O */
struct buf	erxbuf[NURX];	/* buffers for reading error status */
struct buf	rxutab[NURX];	/* per drive buffers */

/* per-drive data */
struct rx_softc {
	int	sc_flags;	/* drive status flags */
	int	sc_csbits;	/* constant bits for CS register */
	int	sc_open;	/* count number of opens */
	int	sc_offset;	/* raw mode kludge to avoid restricting */
				/* single sector transfers to start on */
				/* DEV_BSIZE boundaries */
	/*
	 * The rest of this structure is used to 
	 * store temporaries while simulating multi 
	 * sector transfers
	 */
	caddr_t	sc_uaddr;	/* unibus base address */
	long	sc_bcnt;	/* total transfer count */
	long	sc_resid;	/* no. of bytes left to transfer */
} rx_softc[NURX];

struct rxerr {
	short	rxcs;
	short	rxdb;
	short	rxxt[4];	/* error code dump from controller */
} rxerr[NURX];
/* End of per-drive data */

struct	uba_device *rxdinfo[NURX];
struct	uba_ctlr *rxminfo[NFX];

int	nNURX = NURX;
int	nNFX = NFX;

#endif
