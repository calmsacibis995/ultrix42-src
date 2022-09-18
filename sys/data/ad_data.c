/*
 * @(#)ad_data.c	4.1	(ULTRIX)	7/2/90
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
#include "ad.h"

#include "../machine/pte.h"

#include "../h/ioctl.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/map.h"

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/uba/adreg.h"

#ifdef BINARY

extern	struct uba_device *addinfo[];
extern	struct ad {
	char	ad_open;
	short int ad_uid;
	short int ad_state;
	short int ad_softcsr;
	short int ad_softdata;
	short int ad_chan;
	int	ad_icnt;
	int	ad_loop;
} ad[];

extern	int	nNAD;

#else

struct uba_device *addinfo[NAD];
struct ad {
	char	ad_open;
	short int ad_uid;
	short int ad_state;
	short int ad_softcsr;
	short int ad_softdata;
	short int ad_chan;
	int	ad_icnt;
	int	ad_loop;
} ad[NAD];

int	nNAD = NAD;
	
#endif	BINARY
