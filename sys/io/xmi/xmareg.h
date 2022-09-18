/*
 *	@(#)xmareg.h	4.2	(ULTRIX)	9/4/90
 */

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
 * 06-Jun-1990	Pete Keilty
 *	Modified xma_reg to pad to 17k of node space like xmi_reg.
 *	CIKMF needs node space of 17k.
 *
 * 13-Apr-1990  Joe Szczypek
 *	Modified xma_reg with new registers to support xma2.
 *   
 * 08-Dec-1989	Pete Keilty
 *	Modified xma_reg to pad to 16k of node space like xmi_reg.
 *	CIXCD needed node space of 16k.
 *
 */
struct xma_reg {
	unsigned int xma_type;		/* XMI type field */
	unsigned int xma_xbe;		/* XMI bus error summary */
	unsigned int xma_pad1;
	unsigned int xma_pad2;
	unsigned int xma_seadr;		/* start/end address reg */
	unsigned int xma_mctl1;		/* control reg 1 */
	unsigned int xma_mecer;		/* ecc error status */
	unsigned int xma_mecea;		/* error address */
	unsigned int xma_intlk_flag0;	/* interlock flag 0 */
	unsigned int xma_intlk_flag1;	/* interlock flag 1 */
	unsigned int xma_intlk_flag2;	/* interlock flag 2 */
	unsigned int xma_intlk_flag3;	/* interlock flag 3 */
	unsigned int xma_mctl2;		/* control reg 2 */
	unsigned int xma_tcy;		/* manufacturing test register. */
	/* New XMA2 registers */
	unsigned int xma_becer;		/* Block ECC Error register */
	unsigned int xma_becea;		/* Block ECC Address register */
	unsigned int xma_pad3[4];
	unsigned int xma_stadr;		/* Starting Address Register */
	unsigned int xma_enadr;		/* Ending Address Register */
	unsigned int xma_intlv;		/* Segment/Interleave Register */
	unsigned int xma_mctl3;		/* Memory Control Register 3 */
	unsigned int xma_mctl4;		/* Memory Control Register 4 */
	unsigned int xma_pad4;	
	unsigned int xma_bsctl;		/* Block State Access Control */
	unsigned int xma_bsadr;		/* Block State Access Address */
	unsigned int xma_eectl;		/* EEPROM Access Control/Address */
	unsigned int xma_tmoer;		/* Time-out Control/Status Register */
	/* End XMA'x' registers */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef	__vax
	unsigned int pad[4322];		/* pad to 17k bytes */
#endif /*	__vax */
#ifdef	__mips
	unsigned int pad[131042];	/* pad to xmi node space size */
#endif /*	__mips */
};

/* defines for the XMA ECC error register */
#define XMA_ECC_RDS_ERROR	0x80000000  /* read data subsitute error */
#define XMA_ECC_HI_RATE		0x40000000  /* set if multiple errors */
#define XMA_ECC_CRD_ERROR	0x20000000  /* correctable read error */
#define XMA_ECC_BYTE_WRITE_ERR	0x08000000  /* RDS cause by previos parital
				  write.  Only valid if RDS_ERROR set */

#define XMA_ECC_ROW_PARITY_ERR	0x04000000  /* Internal error... only valid
				if RDS_ERROR is set */

#define XMA_ECC_COL_PARITY_ERR	0x02000000  /* Internal error ... only valid
				if RDS_ERROR is set */

#define XMA_ERR_SYNDROME	0x000000FF



/* defines for the XMA control register 1 */
#define XMA_CTL1_CRD_DISABLE	0x00008000
#define XMA_CTL1_LOCK_ERR	0x00001000
#define XMA_CTL1_UNLOCK_ERR 	0x00000800
#define XMA_CTL1_RDS_WRITE	0x00000400

