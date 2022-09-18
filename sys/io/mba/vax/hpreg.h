
/*	@(#)hpreg.h	4.1	(ULTRIX)	7/2/90	*/
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
 * hpreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * RH/RM03/RM05/RM80/RP05/RP06/RP07 registers/data structures
 * and definitions
 *
 * 28-Mar-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: htreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *	Moved hp_softc data structure here. V2.0
 *
 */

/* Register device structure */
struct hpdevice
{
	int	hpcs1;			/* Control status register 1	*/
	int	hpds;			/* Drive status 		*/
	int	hper1;			/* Error register 1		*/
	int	hpmr;			/* Maintenance			*/
	int	hpas;			/* Attention summary		*/
	int	hpda;			/* Desired address register	*/
	int	hpdt;			/* Drive type			*/
	int	hpla;			/* Look ahead			*/
	int	hpsn;			/* Serial number		*/
	int	hpof;			/* Offset register		*/
	int	hpdc;			/* Desired cylinder register	*/
	int	hpcc;			/* Current cylinder		*/
#define hphr	hpcc			/* Holding register		*/
	int	hpmr2;			/* Maintenance register 2	*/
	int	hper2;			/* Error register 2		*/
	int	hpec1;			/* Burst error bit position	*/
	int	hpec2;			/* Burst error bit pattern	*/
};

/* Control status register definitions (hpcs1)
 * Bits 1 - 5 are the command
 */
#define HP_NOP		0x00		/* No operation 		*/
#define HP_GO		0x01		/* Go bit			*/
#define HP_UNLOAD	0x02		/* Offline drive		*/
#define HP_SEEK 	0x04		/* Seek 			*/
#define HP_RECAL	0x06		/* Recalibrate			*/
#define HP_DCLR 	0x08		/* Drive clear			*/
#define HP_RELEASE	0x0a		/* Release			*/
#define HP_OFFSET	0x0c		/* Offset			*/
#define HP_RTC		0x0e		/* Return to centerline 	*/
#define HP_PRESET	0x10		/* Read-in preset		*/
#define HP_PACK 	0x12		/* Pack acknowledge		*/
#define HP_SEARCH	0x18		/* Search			*/
#define HP_DIAGNOSE	0x1c		/* Diagnose drive		*/
#define HP_WCDATA	0x28		/* Write check data		*/
#define HP_WCHDR	0x2a		/* Write check header and data	*/
#define HP_WCOM 	0x30		/* Write data			*/
#define HP_WHDR 	0x32		/* Write header 		*/
#define HP_WTRACKD	0x34		/* Write track descriptor	*/
#define HP_RCOM 	0x38		/* Read data			*/
#define HP_RHDR 	0x3a		/* Read header and data 	*/
#define HP_RTRACKD	0x3c		/* Read track descriptor	*/
#define HP_IE		0x40		/* Interrupt enable		*/
#define HP_RDY		0x80		/* Controller ready		*/
#define HP_DVA		0x800		/* Drive available		*/
#define HP_TRE		0x4000		/* Transfer error		*/
#define HP_SC		0x8000		/* Special condition		*/

/* Drive status register definitions (hpds)
 * Bits 1 - 5 are spare
 */
#define HPDS_OM 	0x01		/* Offset mode			*/
#define HPDS_VV 	0x40		/* Volume valid 		*/
#define HPDS_DRY	0x80		/* Drive ready			*/
#define HPDS_DPR	0x100		/* Drive present		*/
#define HPDS_PGM	0x200		/* Programmable 		*/
#define HPDS_LST	0x400		/* Last sector transferred	*/
#define HPDS_WRL	0x800		/* Write locked 		*/
#define HPDS_MOL	0x1000		/* Medium online		*/
#define HPDS_PIP	0x2000		/* Positioning in progress	*/
#define HPDS_ERR	0x4000		/* Composite drive error	*/
#define HPDS_ATA	0x8000		/* Attention active		*/
#define HPDS_DREADY	(HPDS_DPR|HPDS_DRY|\
			 HPDS_MOL|HPDS_VV)	/* Drive ready bits	*/
#define HPDS_BITS	"\10\20ATA\17ERR\16PIP\
			 \15MOL\14WRL\13LST\
			 \12PGM\11DPR\10DRY\
			 \7VV\1OM"              /* Status bits          */

/* Error 1 register definitions (hper1) */
#define HPER1_ILF	0x01		/* Illegal function		*/
#define HPER1_ILR	0x02		/* Illegal register		*/
#define HPER1_RMR	0x04		/* Register mod. refused	*/
#define HPER1_PAR	0x08		/* Parity error 		*/
#define HPER1_FER	0x10		/* Format error 		*/
#define HPER1_WCF	0x20		/* Write clock fail		*/
#define HPER1_ECH	0x40		/* Ecc hard error		*/
#define HPER1_HCE	0x80		/* Header compare error 	*/
#define HPER1_HCRC	0x100		/* Header crc error		*/
#define HPER1_AOE	0x200		/* Address overflow error	*/
#define HPER1_IAE	0x400		/* Invalid address error	*/
#define HPER1_WLE	0x800		/* Write lock error		*/
#define HPER1_DTE	0x1000		/* Drive timing error		*/
#define HPER1_OPI	0x2000		/* Operation incomplete 	*/
#define HPER1_UNS	0x4000		/* Drive unsafe 		*/
#define HPER1_DCK	0x8000		/* Data check			*/
#define HPER1_BITS	"\10\20DCK\17UNS\
			 \16OPI\15DTE\14WLE\
			 \13IAE\12AOE\11HCRC\
			 \10HCE\7ECH\6WCF\5FER\
			 \4PAR\3RMR\2ILR\1ILF"          /* Error 1 bits */
#define HPER1_HARD	(HPER1_WLE|HPER1_IAE|\
			 HPER1_AOE|HPER1_FER|\
			 HPER1_RMR|HPER1_ILR|\
			 HPER1_ILF)			/* Hard error 1 */

/* Error 2 register definitions (hper2) */
#define HPER2_DPE	0x08		/* Data parity error		*/
#define HPER2_SSE	0x20		/* Skip sector error (rm80 only)*/
#define HPER2_DVC	0x80		/* Device check 		*/
#define HPER2_LBC	0x400		/* Loss of bit check		*/
#define HPER2_LSC	0x800		/* Loss of system clock 	*/
#define HPER2_IVC	0x1000		/* Invalid command		*/
#define HPER2_OPE	0x2000		/* Operator plug error		*/
#define HPER2_SKI	0x4000		/* Seek incomplete		*/
#define HPER2_BSE	0x8000		/* Bad sector error		*/
#define HPER2_BITS	"\10\20BSE\17SKI\
			 \16OPE\15IVC\14LSC\
			 \13LBC\10DVC\6SSE\4DPE"        /* Error 2 bits */
#define HPER2_HARD	HPER2_OPE	/* Hard error 2 		*/

/* Offset register definitions (hpof) */
#define HPOF_P400	0x10		/*  +400 uinches		*/
#define HPOF_P800	0x20		/*  +800 uinches		*/
#define HPOF_P1200	0x30		/* +1200 uinches		*/
#define HPOF_M400	0x90		/*  -400 uinches		*/
#define HPOF_M800	0xa0		/*  -800 uinches		*/
#define HPOF_M1200	0xb0		/* -1200 uinches		*/
#define HPOF_SSEI	0x200		/* Skip sector inhibit		*/
#define HPOF_HCI	0x400		/* Header compare inhibit	*/
#define HPOF_ECI	0x800		/* Ecc inhibit			*/
#define HPOF_FMT22	0x1000		/* 16 bit format		*/
#define HPOF_MTD	0x4000		/* Move track descriptor	*/
#define HPOF_CMO	0x8000		/* Command modifier		*/

/* Current cylinder (holding) register (hpcc/hphr) */
#define HPHR_MAXCYL	0x8017		/* Maximum cylinder address	*/
#define HPHR_MAXTRAK	0x8018		/* Maximum track address	*/
#define HPHR_MAXSECT	0x8019		/* Maximum sector address	*/
#define HPHR_FMTENABLE	0xffff		/* Enable format command in cs1 */

/* Maintenance register (hpmr) */
#define HPMR_TRT	0x300		/* ML11 transfer rate		*/
#define HPMR_ARRTYP	0x400		/* ML11 array type		*/
#define HPMR_SZ 	0xf800		/* ML11 system size		*/

/*
 * Systems Industries kludge: use value in
 * the serial # register to figure out real drive type.
 */
#define SIRM05		0x0000		/* RM05 pseudo-indication	*/
#define SIMB_LU 	0x0007		/* Logical unit 		*/
#define SI9762		0x0100		/* 9762 			*/
#define SI9766		0x0300		/* 9766 			*/
#define SICAPN		0x0400		/* Capricorn mapped		*/
#define SICAPD		0x0500		/* Capricorn direct		*/
#define SI9775D 	0x0700		/* 9775 direct			*/
#define SI9730D 	0x0b00		/* 9730 direct			*/
#define SI9730M 	0x0d00		/* 9730 mapped			*/
#define SI9775M 	0x0e00		/* 9775 mapped			*/
#define SI9751D 	0x0f00		/* Eagle direct 		*/
#define SIMB_S6 	0x2000		/* Switch s6			*/
#define SIRM03		0x8000		/* RM03 indication		*/
#define SIMB_MB 	0xff00		/* Model byte value		*/

/* Driver and data specific structure */
struct	hp_softc {
	long	sc_flags;		/* General device flags 	*/
	long	sc_category_flags;	/* Category device flags	*/
	u_long	sc_softcnt;		/* Soft error count		*/
	u_long	sc_hardcnt;		/* Hard error count		*/
	u_char	sc_hpinit;		/* Drive initialized		*/
	u_char	sc_recal;		/* Recalibrate state		*/
	u_char	sc_hdr; 		/* Next i/o includes header	*/
	u_char	sc_doseeks;		/* Perform explicit seeks	*/
	daddr_t sc_mlsize;		/* ML11 size			*/
	char	sc_device[DEV_SIZE];	/* Device type string		*/
};
