/*	@(#)dk.h	4.2	(ULTRIX)	9/4/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,89 by			*
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
/*	dk.h	6.1	83/07/29	*/

/* Modification History
 *
 * 03-Mar-86 -- jrs
 *	Added cp_remaind array to hold ticks from each of the processors
 *	before averaging into cp_time.
 *
 * 29-Oct-84 tresvik
 *
 *	changed DK_NDRIVE from 4 to 16.  This allows a good number to be
 * 	plugged into uba_device->ui.dk during autoconf
 *
 * 21-Mar-86 Robin
 *
 *	changed DK_NDRIVE from 16 to 32.  This allows the SPD drive number to be
 * 	plugged into uba_device->ui.dk during autoconf
 *
 * 21-Aug-89 	Tim Burke
 *	Changed DK_NDRIVE from 32 to 256.
 *
 */

/*
 * Instrumentation
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define	CPUSTATES	4

#define	CP_USER		0
#define	CP_NICE		1
#define	CP_SYS		2
#define	CP_IDLE		3

#define	DK_NDRIVE	256

#ifdef KERNEL
long	dk_time[DK_NDRIVE];
long	dk_seek[DK_NDRIVE];
long	dk_xfer[DK_NDRIVE];
long	dk_wds[DK_NDRIVE];
int	dk_busy;
#ifdef __mips
int	dk_bps[DK_NDRIVE];
int	dk_mspw[DK_NDRIVE];
#endif /* __mips */
#ifdef __vax
float	dk_mspw[DK_NDRIVE];
#endif /* __vax */

long	tk_nin;
long	tk_nout;
#endif
