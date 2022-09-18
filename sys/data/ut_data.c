
/*
 *	@(#)ut_data.c	1.6	(ULTRIX)	4/1/86
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
 * ut_data.c
 *
 * Modification history
 *
 * TU45 data file
 * System Industries Model 9700 Tape Drive,
 * emulates a TU45 on the UNIBUS.
 *
 *  9-Feb-86 - ricky palmer
 *
 *	Added "dis_eot_tj" character array for EOT code. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 */

#include "tj.h"

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
#include "../h/kernel.h"
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/utreg.h"

#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"

#ifdef	BINARY

extern	struct	buf	rutbuf[];	/* Bufs for raw i/o		*/
extern	struct	buf	cutbuf[];	/* Bufs for control operations	*/
extern	struct	buf	tjutab[];	/* Bufs for slave queue headers */
extern	struct	uba_ctlr *utminfo[];
extern	struct	uba_device *tjdinfo[];
extern	struct	tj_softc tj_softc[];
extern	short	tjtout[];
extern	int	nNUT;
extern	int	nNTJ;
extern	char	dis_eot_tj[];

#else

struct	buf	rutbuf[NUT];		/* Bufs for raw i/o		*/
struct	buf	cutbuf[NUT];		/* Bufs for control operations	*/
struct	buf	tjutab[NTJ];		/* Bufs for slave queue headers */
struct	uba_ctlr *utminfo[NUT];
struct	uba_device *tjdinfo[NTJ];
struct	tj_softc tj_softc[NTJ];
short	tjtout[NTJ];
int	nNUT = NUT;
int	nNTJ = NTJ;
char	dis_eot_tj[NTJ];

#endif	BINARY
