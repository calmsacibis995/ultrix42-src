/*
 *	@(#)vbareg.h	4.2	(ULTRIX)	1/25/91
 */

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * Abstract:
 *	This module contains the definitions for the various VMEbus host
 *	modules as well as for the XVIB (the VMEbus card).
 *
 * Revision History
 *
 *	22-Jan-1990	Mark Parenti
 *		Add asc field to vbadata structure.  Selects if DMA PMRs
 *		are mapped to 1st or 2nd GB of VMEbus address space.
 *
 *	14-Nov-1989	Mark Parenti (map)
 *		Original Version
 */

/*	Definitions for flags parameter to vbasetup()	*/

#define	VME_A16		0x00000000	/* A16 Request			*/
#define	VME_A24		0x00000001	/* A24 Request			*/
#define	VME_A32		0x00000002	/* A32 Request			*/
#define	VME_D08		0x00000000	/* D08 Data Size		*/
#define	VME_D16		0x00000010	/* D16 Data Size		*/
#define	VME_D32		0x00000020	/* D32 Data Size		*/
#define	VME_BS_MASK	0x00030000	/* Byte Swap mask		*/
#define	VME_BS_SHIFT	16		/* Byte Swap shift count	*/
#define	VME_ASPACE_MASK	0x00000003	/* Address space mask		*/
#define	VME_ASIZE_MASK	0x00000030	/* Data space mask		*/
#define	VME_ASIZE_SHIFT	4		/* Shift for array index	*/
#define	VME_ADD_MASK	(VME_A16 | VME_A24 | VME_A32)

#define	VBA_ADPT_VEC	0x01	/* VME Interrupt vector for adapter use	*/
#define	NVME_VECS	0x100	/* Number of VME vectors		*/
#define	VME_VEC_SIZE	0x4	/* Size of a scb entry (32-bits)	*/
#define	VME_NBYTES_A16	0x10000 /* Size of A16 Memory Space 		*/
#define	VME_A24_VALID	0x800000 /* Lowest valid A24 csr		*/
#define	VME_A32_VALID	0x80000000 /* Lowest valid A32 csr		*/


struct	vbadata {
	int		vme_brl;
	unsigned int	arb_to;
	int		arb_type;
	int		intr_mask;
	int		syscon;
	int		release;
	int		asc;
};

typedef struct	intr_dispatch {
	int	(**intr_hdlr)();
};

#define SCB_VME_VEC_ADDR(vecpage, vector) \
	((vecpage)+(vector))

#define _3VIA_VEC_ADDR(vhp, vector) \
	((((struct vba_hd *)vhp)->intr_vec)+(vector))

#define SCB_VME_ADDR(vecpage) \
	((vecpage))



/************************************************************************/
/* The following values are used for the flags parameter of the		*/
/* vbasetup() and vballoc() routines. They may be combined by using the	*/
/* C-language bit-wise or ( | ).					*/
/************************************************************************/
#define	VMEA16D08	(VME_A16 | VME_D08)
#define	VMEA16D16	(VME_A16 | VME_D16)
#define	VMEA16D32	(VME_A16 | VME_D32)

#define	VMEA24D08	(VME_A24 | VME_D08)
#define	VMEA24D16	(VME_A24 | VME_D16)
#define	VMEA24D32	(VME_A24 | VME_D32)

#define	VMEA32D08	(VME_A32 | VME_D08)
#define	VMEA32D16	(VME_A32 | VME_D16)
#define	VMEA32D32	(VME_A32 | VME_D32)

#define	VME_DMA		0x02000000	/* Need DMA registers		*/
#define	VME_RESERV	0x04000000	/* Reserve VME Address space	*/
#define	VME_CANTWAIT	0x01000000	/* Must have it now		*/
#define	VME_BS_NOSWAP	0x00000000	/* No Byte Swap			*/
#define	VME_RMW_ENAB	0x00100000	/* Enable RMW (XVIB ONLY)	*/
#define	VME_BS_BYTE	0x00010000	/* Byte Swap Bytes		*/
#define	VME_BS_WORD	0x00020000	/* Byte Swap Words		*/
#define	VME_BS_LWORD	0x00030000	/* Byte Swap Longwords		*/





