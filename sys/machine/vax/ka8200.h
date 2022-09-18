/*	7/2/90 (ULTRIX-32) @(#)ka8200.h	4.1	*/	
/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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
 * Modification History: /sys/vax/ka8200.h
 *
 * 	04-feb-86 -- jaw  get rid of biic.h.
 *
 *	03-Feb-86 -- jaw added machine check defines.
 *
 *	15 Jan 86 -- darrell
 *		Moved this file from sys/vaxbi to sys/vax.
 *
 * 	19-Jun-85 -- jaw VAX8200 name change.
 *
 *	05 Jun 85 -- Jaw  cleanup.
 *
 * ------------------------------------------------------------------------
 */
#include "../io/bi/bireg.h"

struct ka820_regs {
 	struct 	biic_regs ka820_biic;	/* BIIC specific registers */
};


/*  VAX 8200 port Addresses */

#define NODESPV8200 	0x20080000
#define PORTV8200	0x20088000
#define	PCKV8200	0x20090000
#define EEV8200		0x20098000
#define RDPNIV8200	0x200a0000
#define RAPNIV8200	0x200a8000
#define RX50V8200	0x200b0000
#define WATCHV8200 	0x200b8000


/* VAX 820 portcontroller CSR bits */
#define V8200_LOGCONS	0x40000000
#define V8200_CRDEN 	0x00000004
#define V8200_CRDCLR 	0x00000002
#define V8200_RXEN  	0x00000080
#define V8200_CONSEN	0x00000400
#define V8200_CONSCLR	0x00000200

/* machine check frame bits */
#define VCR8200 0x40000000     /* vax can't retry bit */
#define	PFE8200 0x80	       /* mach check on prefetcher error */
#define MCHK_THRESHOLD  1	/* 1 sec */
#define MEMWRITE 0x00200000     /* machine check on write to memory */


long v8200port;
