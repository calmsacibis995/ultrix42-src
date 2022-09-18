#ifndef lint
static char *sccsid = "@(#)xma.c	4.2	ULTRIX	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
/*
 * Revision History
 *
 * 05-Jun-90 - jas
 * 	modified CRD enabling.  Now clear "Inhibit CRD" bit 
 *      rather than zeroing out entire register.
 * 01-May-90 - jas
 *	modified xmainit() to support xma2 module.
 * 08-Dec-89 - jaw
 *	remove printf.
 * 15-Jun-89 - darrell
 *	Removed cpup as function arg.
 *
 */

#include "../io/xmi/xmireg.h"
#include "../h/types.h"
#include "../h/time.h"
#include "../h/smp_lock.h"
#include "../h/errlog.h"
#ifdef vax
#include "../machine/ka6200.h"
#endif vax
#ifdef mips
#include "../machine/cpu.h"
#endif mips
#include "../io/xmi/xmareg.h"

xmainit(nxv,nxp,xminumber,xminode,xmidata) 
struct xma_reg	*nxv;	/* virtual pointer to XMI node */
struct xma_reg	*nxp;	/* physical pointer to XMI node */
int xminumber;
int xminode;
struct xmidata *xmidata;
{

	
#ifdef mips
	nxv = (struct xma_reg *)PHYS_TO_K1(nxp);
#endif mips

	/* clear out any pending errors */	
	nxv->xma_xbe = nxv->xma_xbe;
	nxv->xma_mctl1 = nxv->xma_mctl1;
	nxv->xma_mctl2 = nxv->xma_mctl2;
	nxv->xma_mecer = nxv->xma_mecer;
 
	/* clear out XMA2 specific errors */
	/* Note: Is XMA2 if bit 23 set. Bits 15:0 same as XMA */
	if ((nxv->xma_type & XMA2_MASK) == XMI_XMA2) {
		nxv->xma_becer = nxv->xma_becer;
		nxv->xma_mctl3 = nxv->xma_mctl3;
		nxv->xma_mctl4 = nxv->xma_mctl4;
		nxv->xma_tmoer = nxv->xma_tmoer;
	}

	/* enable CRD's */
	nxv->xma_mctl1 = nxv->xma_mctl1 & ~XMA_CTL1_CRD_DISABLE;
}



 


