
/*
 * 	@(#)lta_data.c	4.1	(ULTRIX)	7/2/90
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * lta_data.c
 */

/*
 * Modification History
 *
 * 11-Aug-87 - Tim Burke
 *
 *	Added exec.h to list of include files for compatibility mode check 
 *	stored in the upper 4 bits of the magic number.
 *
 */
#include "lta.h"
/*
 * LAT terminal driver data structures (service class 1)
 */

#ifdef vax
#include "bk.h"
#endif vax
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#ifdef vax
#include "../h/bk.h"
#endif vax
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/devio.h"
#include "../h/exec.h"

/* Lta driver and data specific structure */
struct	lta_softc {
	long	sc_flags;		 /* Flags (one per line)	 */
	long	sc_category_flags;	 /* Category flags (one per line)*/
	u_long	sc_softcnt;		 /* Soft error count total	 */
	u_long	sc_hardcnt;		 /* Hard error count total	 */
	char	sc_device[DEV_SIZE];	 /* Device type string		 */
};

#ifdef BINARY
extern struct tty lata[];
extern struct lta_softc lta_softc[];
extern int nLAT1;

#else BINARY

#if NLTA == 1
#undef NLTA
#define NLTA 16
#endif

struct tty lata[NLTA];
struct lta_softc lta_softc[NLTA];

int nLAT1 = NLTA;

#endif BINARY


