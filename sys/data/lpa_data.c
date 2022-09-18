/*
 * @(#)lpa_data.c	4.1	(ULTRIX)	7/2/90
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

#include "lpa.h"

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/uio.h"

#include "../io/uba/ubavar.h"

#ifdef	BINARY
extern	struct	uba_device *lpadinfo[];

struct lpa_softc {
	int	sc_flag;	/* flags, as defined below */
	int	sc_device;	/* device: 0 = analog in, 1 = analog out */
	int	sc_channel;	/* device channel number */
	struct buf sc_ubuffer;	/* user buffer header */
	int	sc_ubabuf;	/* uba allocation pointer for buffer */
	int	sc_ubufn;	/* present buffer that user is accessing */
	int	sc_lbufn;	/* present buffer that lpa is accessing */
	int	sc_lbufnx;	/* next buffer for lpa (value in ustat) */
	int	sc_nbuf;	/* number of buffers */
	int	sc_count;	/* buffer size in words */
	short	sc_ustat;	/* user status word */
	struct buf sc_ustatbuf;	/* dummy user status word buffer for ubasetup */
	int	sc_ubaustat;	/* uba allocation pointer for ustat */
	struct buf *sc_buffer;	/* scratch buffer header */
	int	sc_start;	/* 0 if lpa operation has been started */
} lpa_softc[];

extern	int	nNLPA;

#else

struct	uba_device *lpadinfo[NLPA];

struct lpa_softc {
	int	sc_flag;	/* flags, as defined below */
	int	sc_device;	/* device: 0 = analog in, 1 = analog out */
	int	sc_channel;	/* device channel number */
	struct buf sc_ubuffer;	/* user buffer header */
	int	sc_ubabuf;	/* uba allocation pointer for buffer */
	int	sc_ubufn;	/* present buffer that user is accessing */
	int	sc_lbufn;	/* present buffer that lpa is accessing */
	int	sc_lbufnx;	/* next buffer for lpa (value in ustat) */
	int	sc_nbuf;	/* number of buffers */
	int	sc_count;	/* buffer size in words */
	short	sc_ustat;	/* user status word */
	struct buf sc_ustatbuf;	/* dummy user status word buffer for ubasetup */
	int	sc_ubaustat;	/* uba allocation pointer for ustat */
	struct buf *sc_buffer;	/* scratch buffer header */
	int	sc_start;	/* 0 if lpa operation has been started */
} lpa_softc[NLPA];

int	nNLPA = NLPA;

#endif

