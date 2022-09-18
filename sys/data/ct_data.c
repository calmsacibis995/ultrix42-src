/*
 * @(#)ct_data.c	4.1	(ULTRIX)	7/2/90
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

#include "ct.h"
/*
 * GP DR11C driver used for C/A/T
 *
 * BUGS:
 *	This driver hasn't been tested in 4.1bsd or 4.2bsd
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

#ifdef BINARY

extern	struct ct_softc {
	int	sc_openf;
	struct	clist sc_oq;
} ct_softc[];
extern	struct	uba_device *ctdinfo[];

extern	int	nNCT;

#else

struct ct_softc {
	int	sc_openf;
	struct	clist sc_oq;
} ct_softc[NCT];
struct	uba_device *ctdinfo[NCT];
int	nNCT = NCT;

#endif
