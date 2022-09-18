
/*	@(#)htreg.h	4.1	(ULTRIX)	7/2/90	*/
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
 * htreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * TM03/TE16/TU45/TU77 registers/data structures and definitions
 *
 * 26-Jan-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: htreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "softcnt" and "hardcnt" to tu_softc structure. V2.0
 *	Added "category_flags" to tu_softc structure. V2.0
 *
 */

/* Register device structure */
struct	htdevice {
	int	htcs1;			/* Control status register	*/
	int	htds;			/* Drive status register	*/
	int	hter;			/* Error register		*/
	int	htmr;			/* Maintenance register 	*/
	int	htas;			/* Attention status register	*/
	int	htfc;			/* Frame counter		*/
	int	htdt;			/* Drive type register		*/
	int	htck;			/* NRZI crc error character	*/
	int	htsn;			/* Serial number register	*/
	int	httc;			/* Tape control register	*/
};

/* Control status register definitions (htcs1) */
#define HT_SENSE	0x00		/* No operations (sense)	*/
#define HT_GO		0x01		/* Go bit			*/
#define HT_REWOFFL	0x02		/* Rewind offline		*/
#define HT_REW		0x06		/* Rewind			*/
#define HT_DCLR 	0x08		/* Drive clear			*/
#define HT_RIP		0x10		/* Read in preset		*/
#define HT_ERASE	0x14		/* Erase			*/
#define HT_WEOF 	0x16		/* Write tape mark		*/
#define HT_SFORW	0x18		/* Space forward		*/
#define HT_SREV 	0x1a		/* Space reverse		*/
#define HT_WCHFWD	0x28		/* Write check forward		*/
#define HT_WCHREV	0x2e		/* Write check reverse		*/
#define HT_WCOM 	0x30		/* Write forward		*/
#define HT_RCOM 	0x38		/* Read forward 		*/
#define HT_RREV 	0x3e		/* Read reverse 		*/

/* Drive status register definitions (htds) */
#define HTDS_SLA	0x01		/* Slave attention		*/
#define HTDS_BOT	0x02		/* Beginning-of-tape (BOT)	*/
#define HTDS_TM 	0x04		/* Tape mark			*/
#define HTDS_IDB	0x08		/* Identification burst 	*/
#define HTDS_SDWN	0x10		/* Settle down			*/
#define HTDS_PES	0x20		/* Phase-encoded status 	*/
#define HTDS_SSC	0x40		/* Slave status change		*/
#define HTDS_DRY	0x80		/* Drive ready			*/
#define HTDS_DPR	0x100		/* Drive present (always 1)	*/
#define HTDS_EOT	0x400		/* End-of-tape (EOT)		*/
#define HTDS_WRL	0x800		/* Write lock			*/
#define HTDS_MOL	0x1000		/* Medium on line		*/
#define HTDS_PIP	0x2000		/* Positioning in progress	*/
#define HTDS_ERR	0x4000		/* Composite error		*/
#define HTDS_ATA	0x8000		/* Attention active		*/
#define HTDS_BITS	"\10\20ATA\17ERR\16PIP\
			 \15MOL\14WRL\13EOT\
			 \11DPR\10DRY\7SSC\
			 \6PES\5SDWN\4IDB\3TM\
			 \2BOT\1SLA"            /* Drive status bits    */

/* Error register definitions (hter) */
#define HTER_ILF	0x01		/* Illegal function		*/
#define HTER_ILR	0x02		/* Illegal register		*/
#define HTER_RMR	0x04		/* Register modif. refused	*/
#define HTER_CPAR	0x08		/* Control bus parity error	*/
#define HTER_FMT	0x10		/* Format error 		*/
#define HTER_DPAR	0x20		/* Data parity error		*/
#define HTER_INCVPE	0x40		/* Incorrectable data/vpar.err. */
#define HTER_PEFLRC	0x80		/* Format error or lrc error	*/
#define HTER_NSG	0x100		/* Non-standard gap		*/
#define HTER_FCE	0x200		/* Frame count error		*/
#define HTER_CSITM	0x400		/* Correctable tape mark/skew	*/
#define HTER_NEF	0x800		/* Non-executable function	*/
#define HTER_DTE	0x1000		/* Drive timing error		*/
#define HTER_OPI	0x2000		/* Operation incomplete 	*/
#define HTER_UNS	0x4000		/* Unsafe			*/
#define HTER_CORCRC	0x8000		/* Correctable data or ecc	*/
#define HTER_HARD	(HTER_UNS|HTER_OPI|\
			 HTER_NEF|HTER_DPAR|\
			 HTER_FMT|HTER_CPAR| \
			 HTER_RMR|HTER_ILR|HTER_ILF)	/* Hard error	*/
#define HTER_BITS	"\10\20CORCRC\17UNS\
			 \16OPI\15DTE\14NEF\
			 \13CSITM\12FCE\11NSG\
			 \10PEFLRC\7INCVPE\6DPAR\
			 \5FMT\4CPAR\3RMR\2ILR\1ILF"    /* Error bits   */

/* Drive type register definitions (htdt)
 * Bits 0 - 8 are formatter/transport type
 * Bit 9 is spare
 */
#define HTDT_SPR	0x400		/* Slave present		*/
#define HTDT_DRQ	0x800		/* Drive requested; always 0	*/
#define HTDT_7CH	0x1000		/* 7 channel; always 0		*/
#define HTDT_MOH	0x2000		/* Moving head; always 0	*/
#define HTDT_TAP	0x4000		/* Tape; always 1		*/
#define HTDT_NSA	0x8000		/* Not sector addrsd.; always 1 */

/* Tape control register definitions (httc)
 * Bits 0 - 2 are slave select
 * Bits 4 - 7 are format select
 * Bits 8 - 10 are density select
 */
#define HTTC_EVEN	0x08		/* Select even parity		*/
#define HTTC_PDP11	0xc0		/* Pdp11 normal format		*/
#define HTTC_800BPI	0x300		/* 800 bpi			*/
#define HTTC_1600BPI	0x400		/* 1600 bpi			*/
#define HTTC_EAODTE	0x1000		/* Enable abort on xfer. errors */
#define HTTC_SAC	0x2000		/* Slave address change 	*/
#define HTTC_FCS	0x4000		/* Frame count status		*/
#define HTTC_ACCL	0x8000		/* Transport not read./writing	*/

/* Driver and data specific structure */
struct	tu_softc {
	char	sc_openf;
	long	sc_flags;
	long	sc_category_flags;
	daddr_t sc_blkno;
	daddr_t sc_nxrec;
	u_short sc_erreg;
	u_short sc_dsreg;
	short	sc_resid;
	short	sc_dens;
	struct	mba_device *sc_mi;
	int	sc_slave;
	u_long	sc_softcnt;
	u_long	sc_hardcnt;
	char	sc_device[DEV_SIZE];
};

/* Driver and data specific definitions */
#define HTUNIT(dev)	(tutoht[UNIT(dev)]) /* Controller slave no.	*/
