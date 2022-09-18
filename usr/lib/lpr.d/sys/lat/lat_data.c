#ifndef lint
static char *sccsid = "@(#)lat_data.c	4.1.1.3	2/29/88";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *			Modification History				*
 *									*
 *	Chung Wong - 1/7/88						*
 *		Add lat_service[MAXSERVICE].                            *
 *									*
 *	Chung Wong - 8/24/87						*
 *		Changed number of arrays from 128 to LAT_MAXSLOTS       *
 *		for statable.                                           *
 *									*
 *	Peter Harbo - 4/15/86						*
 *		Added table for lat connect solicitation (5.1), 	*
 *		solicit info queue timers.				*
 *									*
 *	Jeff Chase - 03/12/86						*
 *		Modified to handle the new MCLGET macro			*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 	    Update to new protsw format, ethernet addr struct change	*
 *									*
 ************************************************************************/

/*	lat_data.c	0.0	11/9/84	*/
/*	lat_data.c	1.0	4/15/86 */

#include "../data/lta_data.c"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"

extern struct sclass class1;

/*
 * LAT global data.
 */

struct lataddr LATmcast =
    {
	AF_LAT,
	{ 0x09, 0x00, 0x2b, 0x00, 0x00, 0x0f }
    };


struct ifnet *rcvif;			/* interface of last received msg */
struct lataddr rcvaddr = { AF_LAT };	/* source of last received msg */
struct lat_slot sl[LAT_MAXSLOTS];	/* slot data structures */
struct lat_vc *vc[LAT_MAXVC];		/* virtual circuit table */
u_char rand;				/* randomiser for circuit id */
u_char lattmoactive = 0;		/* timeout active flag */
u_char sockactive = 0;			/* write is active on control socket */
u_char slotsinuse = 0;			/* # of active slots */
struct ifqueue latintrq;		/* input packet queue */
struct lat_counters latctrs;		/* counters */
u_short mtimer = LAT_MTIMER;		/* multicast timer interval */
u_short soltimer;			/* solicit response timer */
u_short solxmit;			/* solicit info retry limit */
u_short reqid = 1;			/* request id counter */
struct sclass *sclass[MAXCLASS+1] =	/* service class table */
    { 0, &class1 };
struct ecb statable[LAT_MAXSLOTS];		/* status table for lat requests */

/*
 * Templates for the virtual circuit start message.
 */
struct vc_start startvc =
    {
	CLBYTES,			/* min datagram size */
	LAT_VER,	LAT_ECO,	/* version and ECO numbers */
	255,				/* max # of slots/circuit */
	0,				/* dedicated data link buffers */
	8,				/* server circuit timer */
	0,				/* keep alive timer */
	0,				/* facility code */
	LAT_ULTRIX | (2 << 8)		/* UNIX implementation */
    };

struct lat_service lat_service[MAXSERVICE] 
	= {0, "\0", 0, "\0", 0, "\0", 0, "\0",
	   0, "\0", 0, "\0", 0, "\0", 0, "\0"};

struct hic_entity lat_obj[LAT_MAXSLOTS];
