/* @(#)ka8800.h	4.1  (ULTRIX)        7/2/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 11-Jun-86 -- jaw add machinecheck recovery code.
 *
 * 29-Apr-86 -- jaw add error logging for nmi faults.
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * ------------------------------------------------------------------------
 */
#ifndef NBI_INCLUDE
#define NBI_INCLUDE 1
#include "../io/bi/bireg.h"

struct nmi_reg
{
	int memcsr0;
	int memcsr1;
	int memcsr2;
	int memcsr3;
	int memcsr4;
	int memcsr5;
};
#define NMI_C3_ENB_WRT	    	0x08000000
#define NMI_C3_ENB_INT		0xf8000000
#define NMI_C3_ENB_CRD	  	0x20000000
#define NMI_C3_INTERNAL_ERR	0x02000000

#define NMI_C2_ERR_BITS		0xf0000000
#define NMI_C2_RDS		0x40000000
#define NMI_C2_CRD		0x20000000
#define NMI_C2_BAD_DATA		0x10000000

#define NMI_C0_ERR		0xf8000000

struct nbia_regs
{
	int	nbi_csr0;
	int 	nbi_csr1;
	int 	nbi_stop0;
	int 	nbi_stop1;
	int	nbi_br4vec;
	int	nbi_br5vec;
	int	nbi_br6vec;
	int	nbi_br7vec;
	int	nbia_pad[120];
};

#define NBIC0_ERR	0xf8000000

#define NBIC1_ERR  	0x000000f8

#define NBIC0_INTREN 	0x00200000
#define NBIC0_LOOPBACK 	0x00010000


#define NBIC1_BI0	0x00000002
#define NBIC1_BI1	0x00000004
#define NBIC1_PBI0	0x00000008
#define NBIC1_PBI1	0x00000010

struct nbib_regs
{
	struct 	biic_regs nbi_biic;	/* BIIC specific registers */
	long	nbi_pad[64]; 
};


#define	MCSTS_ABORT	1
#define IBOX_ABORT	0x000007ff
#define CBOX_ABORT	0x00000004

#define EBOXA_PE	0x40
#define EBOXA_FILE	0x07
#define	AFILE		0x04
#define EBOXB_PE	0x80
#define	EBOXB_FILE	0x38
#define BFILE		0x20
#define	SDFILE 		0x00
#define MC8800_THRESH	1	/* seconds allowed between machinechecks */

#define VAX8800_ADPT_ADDR(numnbia)\
	((char *)(0x20080000 + (numnbia << 26)))
struct mc8800frame {
	int	mc8800_bcnt;
	int	mc8800_mcstat;
	int	mc8800_ipc;
	int	mc8800_va_viba;
	int	mc8800_iber;
	int	mc8800_cber;
	int	mc8800_eber;
	int	mc8800_nmifsr;
	int	mc8800_nmiear;
	int	mc8800_pc;
	int	mc8800_psl;
};
#endif

#define VAX8800_MAX_BI	4
#define VAX8820_MAX_BI	6

#define MAX_NNBIA	3  /* max number possible  of NBIA's in system */

#define V8820_STAR_PHYS_CSR01 0x38000000
#define V8820_STAR_PHYS_CSR23 0x38000800
#define V8820_STAR_PHYS_CSR4  0x38080000
#define V8820_STAR_PHYS_CSR67 0x38080800

#define V8820_NEMO_PHYS_CSR01 0x3a000000
#define V8820_NEMO_PHYS_CSR23 0x3a000800
#define V8820_NEMO_PHYS_CSR67 0x3a080800

/* NEMO/STAR timeout interrupt enable bit located in 
  CSR1 */
#define NBW_ENABLE_TIMEOUT_INT 0x400

#define NMIFSR_BUFID 	0x0c000000
#define NMIFSR_WRITETO 	0x04000000
#define NMIFSR_PIBATO	0x0c000000
