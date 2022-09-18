

/* @(#)tsreg.h	4.1	(ULTRIX)	7/2/90	*/
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
 * tsreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * TS11/TSU05/TSV05/TU80 registers/data structures and definitions
 *
 *  5-Feb-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: tsreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *
 *  8-Feb-86 - ricky palmer
 *
 *	Added streaming tape support for ts05/tsv05 subsystem. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "softcnt" and "hardcnt" to ts_softc structure. V2.0
 *	Added "category_flags" to ts_softc structure. V2.0
 *
 */

/* Register device structure */
struct	tsdevice {
	u_short tsdb;			/* Data buffer			*/
	u_short tssr;			/* Status register		*/
};

/* Status message structure */
struct	ts_sts {
	u_short s_sts;			/* Packet header		*/
	u_short s_len;			/* Packet length		*/
	u_short s_rbpcr;		/* Residual frame count 	*/
	u_short s_xs0;			/* Extended status 0		*/
	u_short s_xs1;			/* Extended status 1		*/
	u_short s_xs2;			/* Extended status 2		*/
	u_short s_xs3;			/* Extended status 3		*/
	u_short s_xs4;			/* Extended status 4 (ts05 only)*/
};

/* Command message structure */
struct ts_cmd {
	u_short c_cmd;			/* Command			*/
	u_short c_loba; 		/* Low order buffer address	*/
	u_short c_hiba; 		/* High order buffer address	*/
	u_short c_size; 		/* Byte count			*/
};

/* Characteristics data structure */
struct ts_char {
	long	char_addr;		/* Address of status packet	*/
	u_short char_size;		/* Size of status packet	*/
	u_short char_mode;		/* Characteristics mode 	*/
	u_short char_modext;		/* Char. mode extended (ts05)	*/
};

/* Status register definitions (tssr) */
#define TS_SUCC 	0x00		/* Successful termination	*/
#define TS_ATTN 	0x02		/* Attention			*/
#define TS_ALERT	0x04		/* Tape status alert		*/
#define TS_REJECT	0x06		/* Function reject		*/
#define TS_RECOV	0x08		/* Recoverable error		*/
#define TS_RECNM	0x0a		/* Recoverable error, no motion */
#define TS_UNREC	0x0c		/* Unrecoverable error		*/
#define TS_FATAL	0x0e		/* Fatal error			*/
#define TS_TC		0x0e		/* Termination class		*/
#define TS_FTC		0x30		/* Fatal termination class	*/
#define TS_OFL		0x40		/* Offline			*/
#define TS_SSR		0x80		/* Subsystem ready		*/
#define TS_XMEM 	0x300		/* Bus xmem bits		*/
#define TS_NBA		0x400		/* Need buffer address		*/
#define TS_NXM		0x800		/* Nonexistant memory		*/
#define TS_RMR		0x1000		/* Register modify refused	*/
#define TS_SPE		0x2000		/* Serial bus parity error	*/
#define TS_UPE		0x4000		/* Bus parity error		*/
#define TS_SC		0x8000		/* Special condition (error)	*/
#define TSSR_BITS	"\10\20SC\17UPE\16SPE\
			 \15RMR\14NXM\13NBA\
			 \12A17\11A16\10SSR\
			 \7OFL\6FC1\5FC0\4TC2\
			 \3TC1\2TC0\1-"         /* Status bits          */

/* Extended status 0 register definitions (s_xs0) */
#define TS_EOT		0x01		/* End-of-tape (EOT)		*/
#define TS_BOT		0x02		/* Beginning-of-tape (BOT)	*/
#define TS_WLK		0x04		/* Write locked 		*/
#define TS_PED		0x08		/* Phase-encoded drive		*/
#define TS_VCK		0x10		/* Volume check 		*/
#define TS_IES		0x20		/* Interrupt enable status	*/
#define TS_ONL		0x40		/* Online			*/
#define TS_MOT		0x80		/* Capstan is moving		*/
#define TS_ILA		0x100		/* Illegal address		*/
#define TS_ILC		0x200		/* Illegal command		*/
#define TS_NEF		0x400		/* Non-executable function	*/
#define TS_WLE		0x800		/* Write lock error		*/
#define TS_RLL		0x1000		/* Record length long		*/
#define TS_LET		0x2000		/* Logical end-of-tape (LEOT)	*/
#define TS_RLS		0x4000		/* Record length short		*/
#define TS_TMK		0x8000		/* Tape mark detected		*/
#define TSXS0_BITS	"\10\20TMK\17RLS\
			 \16LET\15RLL\14WLE\
			 \13NEF\12ILC\11ILA\
			 \10MOT\7ONL\6IES\
			 \5VCK\4PED\3WLK\
			 \2BOT\1EOT"            /* Status 0 bits        */

/* Extended status 1 register definitions (s_xs1) */
#define TS_MTE		0x01		/* Multitrack error		*/
#define TS_UNC		0x02		/* Uncorrectable data		*/
#define TS_POL		0x04		/* Postamble long		*/
#define TS_POS		0x08		/* Postamble short		*/
#define TS_IED		0x10		/* Invalid end of data		*/
#define TS_IPO		0x20		/* Invalid postamble		*/
#define TS_SYN		0x40		/* Synchronization failure	*/
#define TS_IPR		0x80		/* Invalid preamble		*/
#define TS_SCK		0x200		/* Speed check			*/
#define TS_DBF		0x400		/* Deskew buffer full		*/
#define TS_TIG		0x800		/* Trash in the gap		*/
#define TS_CRS		0x1000		/* Crease detected		*/
#define TS_COR		0x2000		/* Correctable data		*/
#define TS_DLT		0x8000		/* Data late			*/
#define TSXS1_BITS	"\10\20DLT\17-\16COR\
			 \15CRS\14TIG\13DBF\
			 \12SCK\11-\10IPR\
			 \7SYN\6IPO\5IED\
			 \4POS\3POL\2UNC\1MTE"  /* Status 1 bits        */

/* Extended status 2 register definitions (s_xs2) */
#define TS_DT		0xff		/* Dead tracks			*/
#define TS_DTP		0x100		/* Dead track parity		*/
#define TS_WCF		0x400		/* Write card fail		*/
#define TS_TU80 	0x800		/* Tape drive is a TU80 	*/
#define TS_CAF		0x1000		/* Capstan acceleration failure */
#define TS_BPE		0x2000		/* Serial bus parity error	*/
#define TS_SIP		0x4000		/* Silo parity error		*/
#define TS_OPM		0x8000		/* Operation in progress	*/
#define TSXS2_BITS	"\10\20OPM\17SIP\
			 \16BPE\15CAF\
			 \14TU80\13WCF\
			 \12-\11DTP"            /* Status 2 bits        */

/* Extended status 3 register definitions (s_xs3) */
#define TS_RIB		0x01		/* Reverse into BOT		*/
#define TS_LXS		0x02		/* Limit exceeded statically	*/
#define TS_NOI		0x04		/* Noise record 		*/
#define TS_DCK		0x08		/* Density check		*/
#define TS_CRF		0x10		/* Capstan response fail	*/
#define TS_REV		0x20		/* Reverse			*/
#define TS_OPI		0x40		/* Operation incomplete 	*/
#define TS_LMX		0x80		/* Limit exceeded		*/
#define TS_MEC		0xff00		/* Microdiagnostic error code	*/
#define TSXS3_BITS	"\10\10LMX\7OPI\6REV\
			 \5CRF\4DCK\3NOI\
			 \2LXS\1RIB"            /* Status 3 bits        */

/* Extended status 4 register definitions (s_xs4) (ts05 only) */
#define TS_RCX		0x4000		/* Retry count exceeded 	*/
#define TS_HSP		0x8000		/* High speed			*/
#define TSXS4_BITS	"\10\20HSP\17RCX"	/* Status 4 bits	*/

/* Command register definitions (c_cmd) */
#define TS_RCOM 	0x01		/* Read command 		*/
#define TS_SETCHR	0x04		/* Set characteristics		*/
#define TS_WCOM 	0x05		/* Write command		*/
#define TS_SFORW	0x08		/* Forward space record 	*/
#define TS_WEOF 	0x09		/* Write tape mark		*/
#define TS_SENSE	0x0f		/* Get status			*/
#define TS_IE		0x80		/* Interrupt enable		*/
#define TS_SREV 	0x108		/* Reverse space record 	*/
#define TS_OFFL 	0x10a		/* Unload			*/
#define TS_RETRY	0x200		/* Retry bit for read and write */
#define TS_REREAD	0x201		/* Read data retry		*/
#define TS_REWRITE	0x205		/* Write data retry		*/
#define TS_SFORWF	0x208		/* Forward space file		*/
#define TS_SREVF	0x308		/* Reverse space file		*/
#define TS_REW		0x408		/* Rewind			*/
#define TS_CVC		0x4000		/* Clear volume check		*/
#define TS_ACK		0x8000		/* Ack - release command packet */

/* Characteristics data register definitions (char_mode) */
#define TS_ERI		0x10		/* Enable mes. buf. rel. inter. */
#define TS_EAI		0x20		/* Enable attention interrupts	*/
#define TS_ENB		0x40		/* Enable tape marks stop (BOT) */
#define TS_ESS		0x80		/* Enable skip tape marks stop	*/

/* Char. extended data register definitions (char_modext) (ts05 only)	*/
#define TS_ENBUF	0x18		/* Enable read/write buffering	*/
#define TS_ENHSP	0x20		/* Enable highspeed streaming	*/

/* Driver and data specific structure */
struct	ts_softc {
	char	sc_openf;		/* Lock against multiple opens	*/
	short	sc_resid;		/* Copy of last bc		*/
	daddr_t sc_blkno;		/* Block number 		*/
	daddr_t sc_nxrec;		/* Position of end of tape	*/
	struct	ts_cmd sc_cmd;		/* Command packet		*/
	struct	ts_sts sc_sts;		/* Return status packet 	*/
	struct	ts_char sc_char;	/* Characteristics packet	*/
	struct	ts_softc *sc_ubaddr;	/* Bus address of ts_softc	*/
	u_short sc_uba; 		/* Bus addr. of cmd. pkt.(tsdb) */
	short	sc_mapped;		/* Is ts_softc bus mapped ?	*/
	long	sc_flags;		/* Flags			*/
	long	sc_category_flags;	/* Category flags		*/
	u_long	sc_softcnt;		/* Soft error count total	*/
	u_long	sc_hardcnt;		/* Hard error count total	*/
	char	sc_device[DEV_SIZE];	/* Device type string		*/
};

/* Driver and data specific definitions */
#define c_repcnt	c_loba		/* Character repeat count	*/
#define TSSTD		0772520 	/* Standard ts device csr	*/

