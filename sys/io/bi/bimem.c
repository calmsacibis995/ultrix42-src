
#ifndef lint
static char *sccsid = "@(#)bimem.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86 by			*
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

/* ------------------------------------------------------------------------
 * Modification History:
 *
 *	16-Jun-89 -- darrell
 *		removed cpup from args passed.
 *
 *	13-Mar-86 -- darrell
 *		Passing cpup into bimeminit -- for consistency sake.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 *	12-Feb-86 -- darrell
 *		moved the routine bimemerr to ka8200.c
 *
 * 	04-feb-86 -- jaw  get rid of biic.h.
 *
 *	03-Feb-86 -- jaw   Scb changes.
 *
 *	16-Jan-85 -- darrell
 *		moved ka820.c and ka820.h to ../vax/8200.c and 
 *		../vax/8200.h had to change the include to find it.
 *
 *	20-Jan-86 -- pmk   add memory errlogging
 *
 *	03-Sep-85 -- jaw   bug fix.
 *
 *	05-Aug-85 -- lp    Move CRD clear in bimemerr to outside if.
 *
 *	05-Jul-85 -- jaw   Fix CRD handling.
 *
 * 	19-Jun-85 -- jaw   VAX8200 name change.
 *
 *	05 Jun 85 -- jaw   add CRD handling.
 *
 * 	20 Mar 85 -- jaw   add support for VAX 8200.
 *
 *
 * ------------------------------------------------------------------------
 */

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/errlog.h"
#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/nexus.h"
#include "../io/bi/bimemreg.h"
#include "../machine/ka8200.h"

bimeminit(nxv,nxp,binumber,binode)
char *nxp;
struct mcr *nxv;
int binumber;
int binode;
{
	mcrdata[nmcr].memtype = MEMTYPE_BI1;
	mcrdata[nmcr++].mcraddr = nxv;
}

bimemenable(mcr)
register struct bimem *mcr;
{
   	mcr->bimem_csr1 = (mcr->bimem_csr1 & 
			  (~(BI1_BROKE|BI1_ICRD|BI1_MERR|BI1_CNTLERR)));

	v8200port |= (V8200_CRDEN | V8200_CRDCLR ); 
}
