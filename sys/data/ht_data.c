/*
 * static	char	*sccsid = "@(#)ht_data.c	4.1	(ULTRIX)	7/2/90"
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
 * ht_data.c
 *
 * Modification history
 *
 * TM03/TE16/TU45/TU77 data file
 *
 * 26-Jan-86 - ricky palmer
 *
 *	Added "dis_eot_tu" character array for EOT code. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "devio.h" to include list. V2.0
 *
 * 10-Feb-87 - pmk
 *	Added include ht.h and if NTU 0 init structures to 1.
 */

#include "tu.h"
#include "ht.h"

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
#include "../h/devio.h"

#include "../machine/cpu.h"
#include "../io/mba/vax/mbareg.h"
#include "../io/mba/vax/mbavar.h"
#include "../io/mba/vax/htreg.h"

#ifdef	BINARY

extern	struct	buf	rhtbuf[];
extern	struct	buf	chtbuf[];
extern	struct	mba_device *htinfo[];
extern	struct	tu_softc  tu_softc[];
extern	int	nNHT;
extern	int	nNTU;
extern	short	tutoht[];
extern	char	dis_eot_tu[];

#else

struct	buf	rhtbuf[NHT];
struct	buf	chtbuf[NHT];
struct	mba_device *htinfo[NHT];

#if NTU > 0
struct	tu_softc  tu_softc[NTU];
short	tutoht[NTU];
char	dis_eot_tu[NTU];
#else
struct	tu_softc  tu_softc[1];
short	tutoht[1];
char	dis_eot_tu[1];
#endif

int	nNHT = NHT;
int	nNTU = NTU;

#endif	BINARY
