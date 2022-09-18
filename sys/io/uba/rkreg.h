

/* @(#)rkreg.h	4.1	(ULTRIX)	7/2/90	*/

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
 * rkreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * RK711/RK07 registers/data structures and definitions
 *
 * 10-Feb-87 - pmk
 *	Changed rk_softc initialization of arrays from NRK to 7.
 *
 * 16-Apr-86 - ricky palmer
 *	Derived from 4.2BSD labeled: rkreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *	Moved rk_softc data structure here. V2.0
 *
 */

/* Register device structure */
struct rkdevice
{
	short	rkcs1;			/* Control status reg. 1	*/
	short	rkwc;			/* Word count			*/
	u_short rkba;			/* Bus address			*/
	short	rkda;			/* Disk address 		*/
	short	rkcs2;			/* Control status reg. 2	*/
	short	rkds;			/* Drive status 		*/
	short	rker;			/* Driver error register	*/
	short	rkatt;			/* Attention status/offset reg. */
	short	rkcyl;			/* Current cylinder register	*/
	short	rkxxx;
	short	rkdb;			/* Data buffer register 	*/
	short	rkmr1;			/* Maint. register 1		*/
	short	rkec1;			/* Burst error bit position	*/
	short	rkec2;			/* Burst error bit pattern	*/
	short	rkmr2;			/* Maint register 2		*/
	short	rkmr3;			/* Maint register 3		*/
};

/* Control status register 1 definitions (rkcs1)
 * Bits 1 - 4 are the function code
 * Bits 8 - 9 are the extended bus address
 */
#define RK_SELECT	0x00		/* Select drive 		*/
#define RK_GO		0x01		/* Do it			*/
#define RK_PACK 	0x02		/* Pack acknowledge		*/
#define RK_DCLR 	0x04		/* Drive clear			*/
#define RK_UNLOAD	0x06		/* Unload			*/
#define RK_START	0x08		/* Start spindle		*/
#define RK_RECAL	0x0a		/* Recalibrate			*/
#define RK_OFFSET	0x0c		/* Offset			*/
#define RK_SEEK 	0x0e		/* Seek 			*/
#define RK_READ 	0x10		/* Read data			*/
#define RK_WRITE	0x12		/* Write data			*/
#define RK_RHDR 	0x16		/* Read header			*/
#define RK_WHDR 	0x18		/* Write header 		*/
#define RK_IE		0x40		/* Interrupt enable		*/
#define RK_CRDY 	0x80		/* Controller ready		*/
#define RK_CDT		0x400		/* Drive type (rk07/rk06)	*/
#define RK_CTO		0x800		/* Controller timeout		*/
#define RK_CFMT 	0x1000		/* 18 bit word format		*/
#define RK_DTCPAR	0x2000		/* Drive to controller parity	*/
#define RK_DI		0x4000		/* Drive interrupt		*/
#define RK_CCLR 	0x8000		/* Controller clear (also error)*/
#define RK_CERR 	RK_CCLR

/* Control status register 2 definitions (rkcs2)
 * Bits 0 - 2 are drive select
 */
#define RKCS2_RLS	0x08		/* Release			*/
#define RKCS2_BAI	0x10		/* Bus address increment inhibit*/
#define RKCS2_SCLR	0x20		/* Subsystem clear		*/
#define RKCS2_IR	0x40		/* Input ready			*/
#define RKCS2_OR	0x80		/* Output ready 		*/
#define RKCS2_UFE	0x100		/* Unit field error		*/
#define RKCS2_MDS	0x200		/* Multiple drive select	*/
#define RKCS2_PGE	0x400		/* Programming error		*/
#define RKCS2_NEM	0x800		/* Non-existent memory		*/
#define RKCS2_NED	0x1000		/* Non-existent drive		*/
#define RKCS2_UPE	0x2000		/* Unibus parity		*/
#define RKCS2_WCE	0x4000		/* Write check			*/
#define RKCS2_DLT	0x8000		/* Data late			*/
#define RKCS2_BITS	"\10\20DLT\17WCE\
			 \16UPE\15NED\14NEM\
			 \13PGE\12MDS\11UFE\
			 \10OR\7IR\6SCLR\5BAI\4RLS"     /* Status 2 bits*/
#define RKCS2_HARD	(RKCS2_NED|RKCS2_PGE)		/* Hard error	*/

/* Drive status register definitions (rkds)
 * Bits 9, 10, and 12 are spare
 */
#define RKDS_DRA	0x01		/* Drive available		*/
#define RKDS_OFF	0x04		/* Offset mode			*/
#define RKDS_ACLO	0x08		/* Ac low			*/
#define RKDS_SPLS	0x10		/* Speed loss			*/
#define RKDS_DROT	0x20		/* Drive off track		*/
#define RKDS_VV 	0x40		/* Volume valid 		*/
#define RKDS_DRDY	0x80		/* Drive ready			*/
#define RKDS_DDT	0x100		/* Disk drive type		*/
#define RKDS_WRL	0x800		/* Write lock			*/
#define RKDS_PIP	0x2000		/* Positioning in progress	*/
#define RKDS_CDA	0x4000		/* Current drive attention	*/
#define RKDS_SVAL	0x8000		/* Status valid 		*/
#define RKDS_DREADY	(RKDS_DRA|RKDS_VV|RKDS_DRDY)	/* Drive ready	*/
#define RKDS_BITS	"\10\20SVAL\17CDA\
			 \16PIP\14WRL\11DDT\
			 \10DRDY\7VV\6DROT\
			 \5SPLS\4ACLO\3OFF\1DRA"        /* Drive status */
#define RKDS_HARD	(RKDS_ACLO|RKDS_SPLS)		/* Hard error	*/

/* Drive error register definitions (rker) */
#define RKER_ILF	0x01		/* Illegal function		*/
#define RKER_SKI	0x02		/* Deek incomplete		*/
#define RKER_NXF	0x04		/* Non-executable function	*/
#define RKER_DRPAR	0x08		/* Control-to-drive parity error*/
#define RKER_FMTE	0x10		/* Format error 		*/
#define RKER_DTYE	0x20		/* Drive type error		*/
#define RKER_ECH	0x40		/* Hard ecc error		*/
#define RKER_BSE	0x80		/* Bad sector error		*/
#define RKER_HRVC	0x100		/* Header vertical redun. check */
#define RKER_COE	0x200		/* Cylinder overflow error	*/
#define RKER_IDAE	0x400		/* Invalid disk address error	*/
#define RKER_WLE	0x800		/* Write lock error		*/
#define RKER_DTE	0x1000		/* Drive timing error		*/
#define RKER_OPI	0x2000		/* Operation incomplete 	*/
#define RKER_UNS	0x4000		/* Drive unsafe 		*/
#define RKER_DCK	0x8000		/* Data check			*/
#define RKER_BITS	"\10\20DCK\17UNS\16OPI\
			 \15DTE\14WLE\13IDAE\
			 \12COE\11HRVC\10BSE\
			 \7ECH\6DTYE\5FMTE\
			 \4DRPAR\3NXF\2SKI\1ILF"        /* Error bits   */
#define RKER_HARD	(RKER_WLE|RKER_IDAE|\
			 RKER_COE|RKER_DTYE|\
			 RKER_FMTE|RKER_ILF)		/* Hard error	*/

/* Offset register definitions (rkas) */
#define RKAS_P400	0x10		/*  +400 RK06,	+200 RK07	*/
#define RKAS_M400	0x90		/*  -400 RK06,	-200 RK07	*/
#define RKAS_P800	0x20		/*  +800 RK06,	+400 RK07	*/
#define RKAS_M800	0xa0		/*  -800 RK06,	-400 RK07	*/
#define RKAS_P1200	0x30		/*  +800 RK06,	+400 RK07	*/
#define RKAS_M1200	0xb0		/* -1200 RK06, -1200 RK07	*/

/* Layout definitions */
#define NRK7CYL 	815
#define NRK6CYL 	411
#define NRKSECT 	22
#define NRKTRK		3

struct	rk_softc {
	int	sc_softas;
	int	sc_ndrive;
	int	sc_wticks;
	int	sc_recal;
	short	sc_offline[7];
	long	sc_flags[7];		   /* General device flags	*/
	long	sc_category_flags[7];      /* Category device flags	*/
	u_long	sc_softcnt[7];	     	   /* Soft error count		*/
	u_long	sc_hardcnt[7];	     	   /* Hard error count		*/
	char	sc_device[DEV_SIZE][7]	   /* Device type string	*/
};

