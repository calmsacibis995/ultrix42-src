
/*	@(#)mbareg.h	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * mbareg.h	6.1	07/29/83
 *
 * Modification History
 *
 * Massbus adapter register structures and definitions
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: mbareg.h	6.1	83/07/29.
 *	Added copyright notice and cleaned up file. V2.0
 *
 */

/* Register device structure */
struct mba_regs
{
	int	mba_csr;		/* Configuration register	*/
	int	mba_cr; 		/* Control register		*/
	int	mba_sr; 		/* Status register		*/
	int	mba_var;		/* Virtual address register	*/
	int	mba_bcr;		/* Byte count register		*/
	int	mba_dr;
	int	mba_pad1[250];
	struct mba_drv {		/* Per drive registers		*/
		int	mbd_cs1;	/* Control status register	*/
		int	mbd_ds; 	/* Drive status register	*/
		int	mbd_er1;	/* Error register		*/
		int	mbd_mr1;	/* Maintenance register 	*/
		int	mbd_as; 	/* Attention status register	*/
		int	mbd_da; 	/* Desired address (disks)	*/
#define 	mbd_fc	mbd_da		/* Frame count (tapes)		*/
		int	mbd_dt; 	/* Drive type			*/
		int	mbd_la; 	/* Look ahead (disks)		*/
#define 	mbd_ck	mbd_la		/* ??? (tapes)			*/
		int	mbd_sn; 	/* Serial number		*/
		int	mbd_of; 	/* ???				*/
#define 	mbd_tc	mbd_of		/* ???				*/
		int	mbd_fill[22];
	} mba_drv[8];
	struct	pte mba_map[256];	/* Io spc. virtual map of nexus */
	int	mba_pad2[256*5];
};

/* Control register definitions (mba_cr) */
#define MBCR_INIT	0x01		/* Init mba			*/
#define MBCR_IE 	0x04		/* Enable mba interrupts	*/

/* Status register definitions (mba_sr) */
#define MBSR_RDTIMO	0x01		/* Read data timeout		*/
#define MBSR_ISTIMO	0x02		/* Interface sequence timeout	*/
#define MBSR_RDS	0x04		/* Lead data substitute 	*/
#define MBSR_ERRCONF	0x08		/* Error confirmation		*/
#define MBSR_INVMAP	0x10		/* Invalid map			*/
#define MBSR_MAPPE	0x20		/* Page frame map parity error	*/
#define MBSR_MDPE	0x40		/* Massbus data parity error	*/
#define MBSR_MBEXC	0x80		/* Massbus exception		*/
#define MBSR_MXF	0x100		/* Miss transfer error		*/
#define MBSR_WCKLWR	0x200		/* Write check lower		*/
#define MBSR_WCKUP	0x400		/* Write check upper		*/
#define MBSR_DLT	0x800		/* Data late			*/
#define MBSR_DTABT	0x1000		/* Data transfer aborted	*/
#define MBSR_DTCMP	0x2000		/* Data transfer completed	*/
#define MBSR_SPE	0x4000		/* Silo parity error		*/
#define MBSR_ATTN	0x10000 	/* Attention from massbus	*/
#define MBSR_MCPE	0x20000 	/* Massbus control parity error */
#define MBSR_NED	0x40000 	/* Non-existent drive		*/
#define MBSR_PGE	0x80000 	/* Programming error		*/
#define MBSR_CBHUNG	0x800000	/* Control bus hung		*/
#define MBSR_CRD	0x20000000	/* Corrected read data		*/
#define MBSR_NRCONF	0x40000000	/* No response confirmation	*/
#define MBSR_DTBUSY	0x80000000	/* Data transfer busy		*/
#define MBSR_HARD	(MBSR_PGE|MBSR_ERRCONF|\
			 MBSR_ISTIMO|MBSR_RDTIMO)	/* Hard error	*/
#define MBSR_EBITS	(~(MBSR_DTBUSY|MBSR_CRD|\
			 MBSR_ATTN|MBSR_DTCMP)) 	/* Error field	*/
#define MBSR_BITS	"\20\40DTBUSY\37NRCONF\36CRD\
			 \30CBHUNG\24PGE\23NED\22MCPE\
			 \21ATTN\17SPE\16DTCMP\15DTABT\
			 \14DLT\13WCKUP\12WCKLWR\11MXF\
			 \10MBEXC\7MDPE\6MAPPE\5INVMAP\
			 \4ERRCONF\3RDS\2ISTIMO\1RDTIMO"/* Status bits  */

/* Control status register definitions (mba_cs1) */
#define MB_GO		0x01		/* Go command			*/
#define MB_WCOM 	0x30		/* Write command		*/
#define MB_RCOM 	0x38		/* Read command 		*/

/* Drive status register definitions (mba_ds) */
#define MBDS_DRY	0x80		/* Drive ready			*/
#define MBDS_DPR	0x100		/* Drive present		*/
#define MBDS_MOL	0x1000		/* Medium online		*/
#define MBDS_ERR	0x4000		/* Error in drive		*/
#define MBDS_DREADY	(MBDS_MOL|MBDS_DPR|MBDS_DRY)	/* All set	*/

/* Drive type register definitions (mba_dt) */
#define MBDT_RP04	0x10		/* RP04 disk			*/
#define MBDT_RP05	0x11		/* RP05 disk			*/
#define MBDT_RP06	0x12		/* RP06 disk			*/
#define MBDT_RM03	0x14		/* RM03 disk			*/
#define MBDT_RM02	0x15		/* RM02 disk			*/
#define MBDT_RM80	0x16		/* RM80 disk			*/
#define MBDT_RM05	0x17		/* RM05 disk			*/
#define MBDT_RP07	0x22		/* RP07 disk			*/
#define MBDT_TM03	0x28		/* TM03 tape formatter		*/
#define MBDT_TE16	0x29		/* TE16 tape drive		*/
#define MBDT_TU45	0x2a		/* TU45 tape drive		*/
#define MBDT_TU77	0x2c		/* TU77 tape drive		*/
#define MBDT_TU78	0x41		/* TU78 tape drive		*/
#define MBDT_ML11A	0x48		/* ML11A memory disk		*/
#define MBDT_ML11B	0x49		/* ML11B memory disk		*/
#define MBDT_SPR	0x400		/* Slave present		*/
#define MBDT_DRQ	0x800		/* Srive request required	*/
#define MBDT_7CH	0x1000		/* 7 channel			*/
#define MBDT_MOH	0x2000		/* Moving head			*/
#define MBDT_TAP	0x4000		/* Drive is a tape		*/
#define MBDT_NSA	0x8000		/* Not sector addressable	*/
#define MBDT_TYPE	0x1ff		/* Type mask			*/
#define MBDT_MASK	(MBDT_NSA|MBDT_TAP|MBDT_TYPE)	/* Full mask	*/

#ifdef KERNEL
extern	char	mbsr_bits[];
#endif /* KERNEL */

