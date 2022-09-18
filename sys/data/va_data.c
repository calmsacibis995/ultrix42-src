/*
 * @(#)va_data.c	4.1	(ULTRIX)	7/2/90
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

#include "va.h"
/*
 * Varian printer plotter
 */
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

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

struct	vadevice {
	u_short	vaba;			/* buffer address */
	short	vawc;			/* word count (2's complement) */
	union {
		short	Vacsw;		/* control status as word */
		struct {		/* control status as bytes */
			char Vacsl;
			char Vacsh;
		} vacsr;
	} vacs;
	short	vadata;			/* programmed i/o data buffer */
};

#ifdef	BINARY

extern	struct va_softc {
	u_char	sc_openf;		/* exclusive open flag */
	u_char	sc_iostate;		/* kind of I/O going on */
	short	sc_tocnt;		/* time out counter */
	short	sc_info;		/* csw passed from vaintr */
	int	sc_state;		/* print/plot state of device */
} va_softc[];

extern struct	buf rvabuf[];
extern struct	uba_device *vadinfo[];
extern struct	uba_ctlr *vaminfo[];
extern struct	buf vabhdr[];

extern	int	nNVA;

#else

struct va_softc {
	u_char	sc_openf;		/* exclusive open flag */
	u_char	sc_iostate;		/* kind of I/O going on */
	short	sc_tocnt;		/* time out counter */
	short	sc_info;		/* csw passed from vaintr */
	int	sc_state;		/* print/plot state of device */
} va_softc[NVA];

struct	buf rvabuf[NVA];
struct	uba_device *vadinfo[NVA];
struct	uba_ctlr *vaminfo[NVA];
struct	buf vabhdr[NVA];

int	nNVA = NVA;

#endif
