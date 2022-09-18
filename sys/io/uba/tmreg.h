

/* @(#)tmreg.h	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1989 by		*
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
 * tmreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * TM11/TE10 registers/data structures and definitions
 *
 *  8-Feb-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: tmreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "softcnt" and "hardcnt" to te_softc structure. V2.0
 *	Added "category_flags" to te_softc structure. V2.0
 *
 * 26-Jun-89 - Tim Burke
 *
 *	Changed the tmdevice structure to reflect that the "tmer" is the
 *	first and not second register.
 *
 */

/* Register device structure */
struct tmdevice {
	u_short tmer;			/* Error register		*/
	u_short tmcs;			/* Control status register	*/
	short	tmbc;			/* Byte/frame count		*/
	u_short tmba;			/* Buffer address		*/
	short	tmdb;			/* Data buffer			*/
	short	tmrd;			/* Read lines			*/
	short	tmmr;			/* Maintenance register 	*/
};

/* Control status register definitions (tmcs) */
#define TM_OFFL 	0x00		/* Offline			*/
#define TM_GO		0x01		/* Do it			*/
#define TM_RCOM 	0x02		/* Read forward 		*/
#define TM_WCOM 	0x04		/* Write forward		*/
#define TM_WEOF 	0x06		/* Write tape mark		*/
#define TM_SFORW	0x08		/* Space forward		*/
#define TM_SREV 	0x0a		/* Space reverse		*/
#define TM_WIRG 	0x0c		/* Write with interrecord gap	*/
#define TM_REW		0x0e		/* Rewind			*/
#define TM_SENSE	TM_IE		/* Sense			*/
#define TM_IE		0x40		/* Interrupt enable		*/
#define TM_CUR		0x80		/* Control unit is ready	*/
#define TM_DCLR 	0x1000		/* Drive clear			*/
#define TM_D800 	0x6000		/* Select 800 bpi density	*/
#define TM_ERR		0x8000		/* Drive error summary		*/

/* Error register definitions (tmer) */
#define TMER_TUR	0x01		/* Tape unit ready		*/
#define TMER_RWS	0x02		/* Tape unit rewinding		*/
#define TMER_WRL	0x04		/* Tape unit write protected	*/
#define TMER_SDWN	0x08		/* Gap settling down		*/
#define TMER_CH7	0x10		/* 7 channel tape		*/
#define TMER_BOT	0x20		/* Beginning-of-tape (BOT)	*/
#define TMER_SELR	0x40		/* Tape unit properly selected	*/
#define TMER_NXM	0x80		/* Non-existent memory		*/
#define TMER_BTE	0x100		/* Bad tape error		*/
#define TMER_RLE	0x200		/* Record length error		*/
#define TMER_EOT	0x400		/* End-of-tape (EOT)		*/
#define TMER_BGL	0x800		/* Bus grant late		*/
#define TMER_PAE	0x1000		/* Parity error 		*/
#define TMER_CRE	0x2000		/* Cyclic redundancy error	*/
#define TMER_EOF	0x4000		/* End-of-file (EOF)		*/
#define TMER_ILC	0x8000		/* Illegal command		*/
#define TMER_HARD	(TMER_ILC|TMER_EOT)	/* Hard error		*/
#define TMER_SOFT	(TMER_CRE|TMER_PAE|\
			 TMER_BGL|TMER_RLE|\
			 TMER_BTE|TMER_NXM)	/* Soft error		*/
#define TMER_BITS	"\10\20ILC\17EOF\
			 \16CRE\15PAE\14BGL\
			 \13EOT\12RLE\11BTE\
			 \10NXM\7SELR\6BOT\
			 \5CH7\4SDWN\3WRL\
			 \2RWS\1TUR"            /* Error bits           */

/* Driver and data specific structure */
struct	te_softc {
	char	sc_openf;		/* Lock against multiple opens	*/
	short	sc_resid;		/* Copy of last bc		*/
	daddr_t sc_blkno;		/* Block number 		*/
	daddr_t sc_nxrec;		/* Position of end of tape	*/
	u_short sc_erreg;		/* Copy of last erreg		*/
	u_short sc_dsreg;		/* Copy of last dsreg		*/
	u_short sc_dens;		/* Copy of density info.	*/
	daddr_t sc_timo;		/* Time until timeout expires	*/
	short	sc_tact;		/* Timeout is active		*/
	long	sc_flags;		/* Flags			*/
	long	sc_category_flags;	/* Category flags		*/
	u_long	sc_softcnt;		/* Soft error count total	*/
	u_long	sc_hardcnt;		/* Hard error count total	*/
	char	sc_device[DEV_SIZE];	/* Device type string		*/
};

/* Driver and data specific definitions */
#define tmreverseop(cmd) ((cmd)==TM_SREV || (cmd)==TM_REW) /* Reverse	*/
#define TMUNIT(dev)	(tetotm[UNIT(dev)]) /* Controller slave no.	*/
#define TMSTD		0772520 	/* Standard tm device csr	*/
