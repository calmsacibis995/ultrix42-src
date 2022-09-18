#ifndef BUA_INCLUDE
#define BUA_INCLUDE 1

/* 
 * @(#)buareg.h	4.1 	Ultrix 7/2/90
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/* ------------------------------------------------------------------------
 * Modification History: /sys/vaxbi/buareg.h
 *
 * 	04-feb-86 -- jaw  get rid of biic.h.
 *
 *	03-Sep-85 -- jaw
 *		Changes for BI error interrupts.
 *
 * 	19-Jun-85 -- jaw
 *		VAX8200 name change.
 *
 *	20 Mar 85 -- jaw
 *		add support for VAX 8200.
 *
 * ------------------------------------------------------------------------
 */
#include "../io/bi/bireg.h"

/*
 * VAX BUA registers
 */

struct bua_regs
{
	struct 	biic_regs  bua_biic;	/* BIIC specific registers */
	long   	bua_pad[392];		
	long	bua_ctrl;		/* Control and status register */
	long	bua_vec;		/* vector offset register */
	long	bua_fubar;		/* Failed unibus address register */
	long  	bua_pad2;
	long	bua_micro[5];		/* micro diag registers */
	long	bua_pad3[3];
	long	bua_dpr[6];		/* data path control status */
	long	bua_pad4[10];		
	long	bua_bdps[20];		/* buffer data paths (5 octawords) */
	long	bua_pad5[8];
	struct	pte bua_map[512];	/* unibus to bi map registers */
};

#define BUAERR_BITS \
"\20\35BIF\34USSTO\33UIE\32IMR\31BADBDP"

/* bua control status register */
#define BUACR_MASK	0x1f000000 	/* mask for error bits */
#define	BUACR_ERR	0x80000000	/* logical or of all error bits */
#define BUACR_BIF	0x10000000	/* BI failure */
#define BUACR_USSTO	0x08000000	/* Slave sync timeout */
#define BUACR_UIE	0x04000000	/* unibus interlock error */
#define BUACR_IMR	0x02000000	/* invalid map register */
#define BUACR_BADBDP	0x01000000	/* bad buffered data patch select */
#define BUACR_BUAEIE	0x00100000	/* bua error interrupt bit */
#define BUACR_UPI	0x00020000	/* unibus power init */
#define BUACR_REGDMP	0x00010000	/* microdiag register dump */
#define BUACR_IEN	0x000000ff	/* internal error number */

/* vecter offset register */
#define BUAVEC		0x00003e00	

/* failed unibus address */
#define BUAFUBAR	0x0000ffff

/* micro diag */
#define BUAMICRO_ADDR	0xffff0000
#define BUAMICOR_STAT	0x0000ffff

/* data path control status */
#define BUADPR_DPSEL	0x00e00000
#define BUADPR_PURGE	0x00000001

/* map registers */
#define BUAMAP_VALID	0x80000000	/* map valid bit */
#define BUAMAP_PPIE	0x40000000	/* reserved control bit */
#define BUAMAP_LONG	0x04000000	/* longword access enable */
#define BUAMAP_BYTE	0x02000000	/* byte offset */
#define BUAMAP_DPS	0x00e00000	/* data path select */
#define BUAMAP_PFN	0x001fffff	/* page frame number */


#define NBDP_BUA	5

#endif
