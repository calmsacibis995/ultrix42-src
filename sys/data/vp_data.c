/*
 * @(#)vp_data.c	4.1	(ULTRIX)	7/2/90
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
#include "vp.h"



#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/ioctl.h"
#include "../h/vcmd.h"
#include "../h/uio.h"
#include "../h/kernel.h"

#include "../io/uba/ubavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/vpreg.h"

unsigned minvpph();

#ifdef BINARY

extern	struct vp_softc {
	int	sc_state;
	int	sc_count;
	int	sc_bufp;
	struct	buf *sc_bp;
	int	sc_ubinfo;
} vp_softc[];

extern	struct	uba_device *vpdinfo[];

extern	struct	buf rvpbuf[];
extern	struct	uba_device *vpdinfo[];

extern	int	nNVP;

#else

struct vp_softc {
	int	sc_state;
	int	sc_count;
	int	sc_bufp;
	struct	buf *sc_bp;
	int	sc_ubinfo;
} vp_softc[NVP];

struct	uba_device *vpdinfo[NVP];

struct	buf rvpbuf[NVP];
struct	uba_device *vpdinfo[NVP];

int	nNVP = NVP;

#endif
