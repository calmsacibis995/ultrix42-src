/*
 * cpudata.c
 */

#ifndef lint
static char *sccsid = "@(#)cpudata.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,86 by			*
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


/* ------------------------------------------------------------------------
 * Modification History:
 *
 * 31-Jan-89 -- map
 *	Change include syntax for merged pool.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 27-Sep-85 -- tresvik
 *	Fixed reported bug - changed UMEMSIZE780 for 750 and 730
 *	to UMEMSIZE750 and UMEMSIZE730 respectively.  This broke builds
 *	for cpu specific kernels from source.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 14-Mar-85 -jaw
 *	Changes for support of the VAX8200.
 *
 * 12-Mar-85 -tresvik
 *	Reduced IOA count from 4 to 2 for the VAX8600
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 *  3 Nov 84 -- rjl
 *	Added support for MicroVAX-II
 *
 *  2 Jan 84 -- jmcg
 *	Added support for MicroVAX I.
 *
 *  2 Jan 84 --jmcg
 *	Derived from Ultrix-32 baseline sources 1.4, heritage is 4.2BSD
 *	labeled:
 *		cpudata.c	6.1	83/07/29
 *
 * ------------------------------------------------------------------------
 */

#include "../machine/pte.h"

#include "../h/param.h"

#include "../machine/cpu.h"
#include "../machine/ioa.h"
#include "../machine/nexus.h"
#include "../io/uba/ubareg.h"

/*
 * Initialization of per-cpu data structures.
 */

/*
 * These are the fixed addrs of bus spaces on the various machines. The
 * nexaddr entries are where the adapters live. The umaddr is the adapter
 * memory space and udevaddr is the adapter i/o space. These tables allow
 * noncontigous adapter spaces (vax/8600) and adapters whose memory and
 * device csr space are non-contigous (uVAXen). 
 */
short *ioaaddr8600[NIOA8600] = {
	IOA8600(0), IOA8600(1)
};
/*
 * Information to patch around the stupidity of configuration
 * registers not returning types on some of the processors.
 */
short	nexty750[NNEX750] = {
	NEX_MEM750,	NEX_MEM750,	NEX_MEM750,	NEX_MEM750,
	NEX_MBA,	NEX_MBA,	NEX_MBA,	NEX_MBA,
	NEX_UBA0,	NEX_UBA1,	NEX_ANY,	NEX_ANY,
	NEX_ANY,	NEX_ANY,	NEX_ANY,	NEX_ANY
};
short	nexty730[NNEX730] = {
	NEX_MEM730,	NEX_ANY,	NEX_ANY,	NEX_ANY,
	NEX_ANY,	NEX_ANY,	NEX_ANY,	NEX_ANY,
};
short	nextyUVI[NNEXUVI] = {
	NEX_Q22
};

