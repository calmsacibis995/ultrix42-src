/*
 *	@(#)xbireg.h	4.2	(ULTRIX)	9/4/90
 */


/************************************************************************
 *									*
 *			Copyright (c) 1988, 90 by			*
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
 * 17-May-90    jas
 *      Removed pad after xbi_arvr.  Increased pad from 2 to 3 after
 *      xbi_abear.
 *
 * 01-May-90	jas
 *      Modified xbi_reg structure to support added xbi+ registers.  Also
 *      added additional definitions for xbi+.
 *
 * 05-Mar-1990    Paul Grist
 *     modified some XBIA regiter bit definitions, added some comments.
 *
 * 05-Mar-1990    Mark Parenti
 *	Add XVME definitions.
 */

struct xbi_reg {
	unsigned int  	xbi_dtype;	/* device type register */
	unsigned int  	xbi_xbe;	/* bus error status */
	unsigned int	xbi_fadr;	/* XMI fail address on error */
	unsigned int	xbi_arear; 	/* responder failed address*/
	unsigned int	xbi_aesr;	/* xbia error summary */
	unsigned int	xbi_aimr;	/* error interrupt mask */
 	unsigned int	xbi_aivintr; 	/* error interrupt distination */
	unsigned int	xbi_adg1;	/* diagnostic register */
	unsigned int	xbi_autlr;	/* xbia+ utility register */
	unsigned int	xbi_acsr;	/* xbia+ control and status reg */
	unsigned int	xbi_arvr;	/* xbia+ regturn vector register */
	unsigned int	xbi_xfaer;	/* XMI fail address extension    */
	unsigned int	xbi_abear;	/* BI Error Address register */
	unsigned int	xbi_pad1[3];
	unsigned int	xbi_bcsr; 	/* control and status */
	unsigned int	xbi_besr;	/* xbib error summary */
 	unsigned int	xbi_bidr;	/*  interrupt distination mask*/
	unsigned int 	xbi_btim; 	/* timeout address reg */
	unsigned int	xbi_bvor;       /* vector offset reg */
	unsigned int	xbi_bvr; 	/* xbib error intr vector */
	unsigned int	xbi_bdcr1;	/* diagnostic control reg */
	
};


/* XVME register names */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define	xbi_vdcr	xbi_bcsr

/* xbib control status register */
#define BCSR_ENABLE_INT	0x80000000
#define BCSR_INTERLOCK_MASK  0x4
#define BCSR_PARITY_ERROR 0x1


/* xbib diagnostic control register */
#define XBI_LOOPBACK 0x8

/* xbia error mask register bit definititions. */
#define XBIA_ENABLE_IVINTR	0x80000000
#define XBIA_ENABLE_CC		0x08000000
#define XBIA_ENABLE_PE		0x00800000
#define XBIA_ENABLE_WSE		0x00400000
#define XBIA_ENABLE_RDN		0x00200000
#define XBIA_ENABLE_WDN		0x00100000
#define XBIA_ENABLE_CRD		0x00080000
#define XBIA_ENABLE_NRR		0x00040000
#define XBIA_ENABLE_RSE		0x00020000
#define XBIA_ENABLE_RER		0x00010000
#define XBIA_ENABLE_NXM		0x00008000
#define XBIA_ENABLE_IBUS_APE	0x00000008
#define XBIA_ENABLE_IBUS_BPE	0x00000002
#define XBIA_ENABLE_IBUS_DPE	0x00000001
#define	XBIA_ENABLE_MASK	0x88F7800B

#define XBIAP_ENABLE_IPE        0x01000000
#define XBIAP_ENABLE_TTO        0x00002000
#define XBIAP_ENABLE_IPFN       0x00000800
#define XBIAP_ENABLE_CECCERR    0x00000400
#define XBIAP_ENABLE_UERRERR    0x00000200
#define XBIAP_ENABLE_IBIA       0x00000100
#define XBIAP_ENABLE_IE         0x00000080
#define XBIAP_ENABLE_IOWRTFAIL  0x00000040
#define XBIAP_ENABLE_BCIACLO    0x00000020
#define XBIAP_ENABLE_ADPE       0x00000010
#define XBIAP_ENABLE_ACPE       0x00000008
#define XBIAP_ENABLE_BDPE       0x00000004
#define XBIAP_ENABLE_BCPE       0x00000002
#define XBIAP_ENABLE_IORDPE     0x00000001

/* XBIA Error Summary Register	*/

#define XBIA_IB_OK		0x80000000   /* IBUS cables ok */
#define XBIA_RFID		0x03F00000   /* responder failing id */
#define XBIA_RFCMD		0x000F0000   /* responder failing command */
#define XBIA_MULT_ERRS		0x00004000   /* xbia+ multiple errors */
#define XBIA_CORR_PMR_ECC	0x00002000   /* correctable PMR ECC error */
#define XBIA_UNCORR_PMR_ECC	0x00001000   /* uncorrectable PMR ECC error */
#define XBIA_INV_PFN		0x00000800   /* invalid PFN */
#define XBIA_CORR_DMA_ECC	0x00000400   /* correctable DMA ECC error */
#define XBIA_UNCORR_DMA_ECC	0x00000200   /* uncorrectable DMA ECC error */
#define XBIA_INT_ERR		0x00000080   /* internal error */
#define XBIA_IOW_FAIL		0x00000040   /* I/O write failure */
#define XBIA_IBUS_A_DPE		0x00000010   /* IBUS DMAA data parity error */
#define XBIA_IBUS_A_CAPE	0x00000008   /* IBUS DMAA C/A parity error */
#define XBIA_IBUS_B_DPE		0x00000004   /* IBUS DMAB data parity error */
#define XBIA_IBUS_B_CAPE	0x00000002   /* IBUS DMAB C/A parity error */
#define XBIA_IBUS_DPE		0x00000001   /* IBUS I/O read data PE */


/* XBIA+ specific definitions		*/
#define	XBIAP_TYPE_MASK		0x0000000F
#define	XBIAP_XVME		0xE8
#define	XBIAP_DMA_VALID		0x80000000	/* PMR VALID bit	*/
#define	XBIAP_DMA_DISABLE	0x00000000	/* DWMBA mode (no pmr's) */
#define	XBIAP_DMA_512		0x00010000	/* 512 byte pages	*/
#define	XBIAP_DMA_4K		0x00020000	/* 4k byte pages	*/
#define	XBIAP_DMA_8K		0x00030000	/* 8k byte pages	*/

/* XBIB-specific definitions		*/
#define	XBIA_BCI_AC_LOW		0x00000020
#define	XBIA_INV_BI_ADD		0x00000100

/* XVME-specific definitions		*/
#define	XVME_VME_AC_LOW		0x00000020      /* VME power out-of-spec */
#define	XVME_PIO_SHIFT		19
#define	XVME_PIO_OFFSET		0x000FFFFF
#define	XVME_NDMAPMR	0x10000		/* Number of DMA PMR's	(64K)	  */
#ifdef __mips
#define	XVME_NBYTE_DMAPMR 0x2000	/* Page size for DMA PMR's (8KB)  */
#define	XVME_DMAPMR_SHIFT 0xD		/* Shift count for DMA page size  */
#endif /* __mips */
#ifdef __vax
#define	XVME_NBYTE_DMAPMR 0x200		/* Page size for DMA PMR's (512B)  */
#define	XVME_DMAPMR_SHIFT 0x9		/* Shift count for DMA page size  */
#endif /* __vax */
#define	XVME_NBYTE_PIOPMR 0x100000	/* Number of bytes per PIO PMR(1MB) */








