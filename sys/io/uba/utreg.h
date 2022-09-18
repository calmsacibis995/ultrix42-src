
/* @(#)utreg.h	4.1	(ULTRIX)	7/2/90	*/

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
 * utreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * TU45 registers/data structures and definitions
 * System Industries Model 9700 Tape Drive,
 * emulates a TU45 on the UNIBUS.
 *
 *  9-Feb-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: utreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "softcnt" and "hardcnt" to tj_softc structure. V2.0
 *	Added "category_flags" to tj_softc structure. V2.0
 *
 */

/* Register device structure */
struct utdevice {
	u_short utcs1;			/* Control status register 1	*/
	short	utwc;			/* Word count register		*/
	u_short utba;			/* Low 16-bits of bus address	*/
	short	utfc;			/* Frame counter		*/
	u_short utcs2;			/* Control status register 2	*/
	u_short utds;			/* Drive status register	*/
	u_short uter;			/* Error register		*/
	u_short utas;			/* Attention status register	*/
	u_short utcc;			/* NRZI CRC char. for validat.	*/
	u_short utdb;			/* Data buffer reg. (not emul.) */
	u_short utmr;			/* Maintenance reg. (not emul.) */
	u_short utdt;			/* Drive type reg. (not emul.)	*/
	u_short utsn;			/* Serial number reg.(not emul.)*/
	u_short uttc;			/* Tape control register	*/
	u_short utbae;			/* Buffer addr. extension reg.	*/
	u_short utcs3;			/* Control and status reg. 3	*/
};

/* Control status 1 register definitions (utcs1)
 * Bits 1 - 5 are function code bits.
 * The remaining bits are control and status bits with
 * bit 10 unused, bit 12 unused, and  bit 13 (massbus control parity
 * error) unused and hence not emulated.
 */
#define UT_NOP		0x00		/* No operation 		*/
#define UT_GO		0x01		/* Do it			*/
#define UT_REWOFFL	0x02		/* Rewind offline		*/
#define UT_LOOP 	0x04		/* Loop read/write		*/
#define UT_REW		0x06		/* Rewind			*/
#define UT_CLEAR	0x08		/* Drive clear			*/
#define UT_SENSE	0x0a		/* Drive sense			*/
#define UT_PRESET	0x10		/* Read in preset		*/
#define UT_DIAGN	0x12		/* Diagnostic mode set		*/
#define UT_ERASE	0x14		/* Erase			*/
#define UT_WEOF 	0x16		/* Write tape mark		*/
#define UT_SFORW	0x18		/* Forward space block		*/
#define UT_SREV 	0x1a		/* Reverse space block		*/
#define UT_SFORWF	0x1c		/* Forward space file		*/
#define UT_SREVF	0x1e		/* Reverse space file		*/
#define UT_WCHFORW	0x28		/* Write check forward		*/
#define UT_WCHREV	0x2e		/* Write check reverse		*/
#define UT_WCOM 	0x30		/* Write forward		*/
#define UT_RCOM 	0x38		/* Read forward 		*/
#define UT_RREV 	0x3e		/* Read reverse 		*/
#define UT_IE		0x40		/* Interrupt-enable		*/
#define UT_RDY		0x80		/* Controller ready		*/
#define UT_EADDR	0x300		/* Extended address bits	*/
#define UT_DVA		0x800		/* Drive available		*/
#define UT_TRE		0x4000		/* Transfer error		*/
#define UT_SC		0x8000		/* Special condition		*/
#define UT_BITS 	"\10\20SC\17TRE\
			 \14DVA\10RDY\7IE\1GO"  /* Status 1 bits        */

/* Control status 2 register definitions (utcs2)
 * Bits 0 - 2 are unit select bits.
 */
#define UTCS2_BAI	0x08		/* UNIBUS address incr. inhibit */
#define UTCS2_PAT	0x10		/* Parity test			*/
#define UTCS2_CLR	0x20		/* Controller clear		*/
#define UTCS2_IR	0x40		/* Input ready (not emulated)	*/
#define UTCS2_OR	0x80		/* Output ready (not emulated)	*/
#define UTCS2_RPE	0x100		/* Rom parity error		*/
#define UTCS2_MXF	0x200		/* Missed transfer		*/
#define UTCS2_NEM	0x400		/* Non-existent memory		*/
#define UTCS2_PGE	0x800		/* Program error		*/
#define UTCS2_NED	0x1000		/* Non-existent drive		*/
#define UTCS2_PE	0x2000		/* Parity error 		*/
#define UTCS2_WCE	0x4000		/* Write check error		*/
#define UTCS2_DLT	0x8000		/* Data late			*/
#define UTCS2_BITS	"\10\20DLT\17WCE\
			 \16PE\15NED\14\NEM\
			 \13\PGE\12\MXF\11RPE\
			 \10OR\7IR\6CLR\5PAT\
			 \4\BAI"                /* Status 2 bits        */

/* Drive status register definitions (utds) */
#define UTDS_SLA	0x01		/* Slave attention		*/
#define UTDS_BOT	0x02		/* Beginning-of-tape (BOT)	*/
#define UTDS_TM 	0x04		/* Tape mark			*/
#define UTDS_IDB	0x08		/* Identification burst 	*/
#define UTDS_SDWN	0x10		/* Slowing down 		*/
#define UTDS_PES	0x20		/* Phase encode status		*/
#define UTDS_SSC	0x40		/* Slave status change		*/
#define UTDS_DRY	0x80		/* Drive ready			*/
#define UTDS_DPR	0x100		/* Drive present (always 1)	*/
#define UTDS_GCR	0x200		/* GCR status			*/
#define UTDS_EOT	0x400		/* End-of-tape (EOT)		*/
#define UTDS_WRL	0x800		/* Write lock			*/
#define UTDS_MOL	0x1000		/* Medium on line		*/
#define UTDS_PIP	0x2000		/* Positioning in progress	*/
#define UTDS_ERR	0x4000		/* Composite error		*/
#define UTDS_ATA	0x8000		/* Attention active		*/
#define UTDS_BITS	"\10\20ATA\17ERR\
			 \16PIP\15MOL\14WRL\
			 \13EOT\12GCR\11DPR\
			 \10DRY\7SSC\6PES\
			 \5SDWN\4IDB\3TM\
			 \2BOT\1SLA"            /* Drive status bits    */

/* Drive error register definitions (uter) */
#define UTER_ILF	0x01		/* Illegal function		*/
#define UTER_ILR	0x02		/* Illegal register (always 0)	*/
#define UTER_RMR	0x04		/* Register modif. refused	*/
#define UTER_RPE	0x08		/* Read data parity error	*/
#define UTER_FMT	0x10		/* Format error 		*/
#define UTER_DPAR	0x20		/* Data bus parity error	*/
#define UTER_INC	0x40		/* Incorrectable data		*/
#define UTER_PEF	0x80		/* PE format error		*/
#define UTER_NSG	0x100		/* Non-standard gap		*/
#define UTER_FCE	0x200		/* Frame count error		*/
#define UTER_CS 	0x400		/* Correctable skew		*/
#define UTER_NEF	0x800		/* Non-executable function	*/
#define UTER_DTE	0x1000		/* Drive timing error		*/
#define UTER_OPI	0x2000		/* Operation incomplete 	*/
#define UTER_UNS	0x4000		/* Unsafe			*/
#define UTER_COR	0x8000		/* Correctable data error	*/
#define UTER_HARD	(UTER_UNS|UTER_NEF|\
			 UTER_DPAR|UTER_FMT|\
			 UTER_RMR|UTER_ILR|UTER_ILF)	/* Hard error	*/
#define UTER_BITS	"\10\20COR\17UNS\16OPI\
			 \15DTE\14NEF\13CS\12FCE\
			 \11NSG\10PEF\7INC\6DPAR\
			 \5FMT\4RPE\3RMR\2ILR\1ILF"     /* Error bits   */

/* Tape control register definitions (uttc)
 * Bits 0 - 2 are slave select.
 * Bit 11 is unused.
 */
#define UTTC_EVPAR	0x08		/* Even parity			*/
#define UTTC_PDP11FMT	0xc0		/* PDP-11 standard format	*/
#define UTTC_FMT	0xf0		/* Format select		*/
#define UTTC_NRZI	0x00		/* 800 bpi density select	*/
#define UTTC_PE 	0x400		/* 1600 bpi density select	*/
#define UTTC_GCR	0x500		/* 6250 bpi density select	*/
#define UTTC_DEN	0x700		/* Density select		*/
#define UTTC_EAODTE	0x1000		/* (not emulated)		*/
#define UTTC_TCW	0x2000		/* Tape control write		*/
#define UTTC_FCS	0x4000		/* Frame count status		*/
#define UTTC_ACCL	0x8000		/* Acceleration 		*/

/* Driver and data specific structure */
struct	tj_softc {
	char	sc_openf;		/* Exclusive open		*/
	daddr_t sc_blkno;		/* Next block to transfer	*/
	daddr_t sc_nxrec;		/* Next record on tape		*/
	u_short sc_erreg;		/* Copy of last uter		*/
	u_short sc_dsreg;		/* Copy of last utds		*/
	u_short sc_resid;		/* Residual from transfer	*/
	u_short sc_dens;		/* Copy of density info.	*/
	daddr_t sc_timo;		/* Time until timeout expires	*/
	short	sc_tact;		/* Timeout is active flag	*/
	long	sc_flags;		/* Flags			*/
	long	sc_category_flags;	/* Category flags		*/
	u_long	sc_softcnt;		/* Soft error count total	*/
	u_long	sc_hardcnt;		/* Hard error count total	*/
	char	sc_device[DEV_SIZE];	/* Device type string		*/
};

/* Driver and data specific definitions */
#define b_state 	b_active	/* Command state		*/
#define UTUNIT(dev)	(tjtout[UNIT(dev)])	/* Controller slave no. */
#define UTSTD		0772440 	/* Standard ut device csr	*/
