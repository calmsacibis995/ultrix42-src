/*	@(#)bimemreg.h	4.1	(ULTRIX)	7/2/90	*/

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/* ------------------------------------------------------------------------
 * Modification History: /sys/vaxbi/bimemreg.h
 *
 * 	04-feb-86 -- jaw  get rid of biic.h.
 *
 *	05-Jul-85 -- jaw  fix up crd handling.
 *
 * 	19-Jun-85 -- jaw  VAX8200 name change.
 *
 *	05 Jun 85 -- jaw  add support for VAX 8200.
 *
 * ------------------------------------------------------------------------
 */
#include "../io/bi/bireg.h"

/*
 * VAX BIMEM registers
 */

struct bimem {
	struct biic_regs bimem_biic;	/* BIIC specific registers */
	long bimem_csr1;
	long bimem_csr2;
};

/* bimem csr1 */
#define BI1_ICRD	0x00008000
#define BI1_MERR	0x00000400
#define BI1_BROKE	0x00001000
#define BI1_CNTLERR	0x00000200
#define BI1_ERRSUM	0x80000000

/* bimem csr2 */
#define BI1_SYN		0x000000ff
#define BI1_MSKADDR	0x0000ffff
#define BI1_CRDERR	0x20000000
#define BI1_RDS		0x80000000
#define MBI1_SYN(mcr) 	(mcr->bimem_csr2  & BI1_SYN) 
#define MBI1_ADDR(mcr) ((mcr->bimem_csr1 >> 8) & BI1_MSKADDR)
