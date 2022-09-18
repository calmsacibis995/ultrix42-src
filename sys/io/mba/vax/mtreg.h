
/*	@(#)mtreg.h	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1987 by		*
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
 * mtreg.h	6.1	07/29/83
 *
 * Modification history
 *
 * TM78/TU78 registers/data structures and definitions
 *
 *  7-Jan-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: mtreg.h	6.1	83/07/29.
 *	Converted definitions to hex values and cleaned up comments. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added "softcnt" and "hardcnt" to mu_softc structure. V2.0
 *	Added "category_flags" to mu_softc structure. V2.0
 *
 * 17-May-87 - ricky palmer
 *
 *	Added "firsttime", "savestate", and "changedstate" to mu_softc
 *	structure. V2.2
 *
 */

/* Register device structure */
struct	mtdevice {
	int	mtcs;			/* Control status register	*/
	int	mter;			/* Error register		*/
	int	mtca;			/* Cmd_addr, rec_cnt, skp_cnt r.*/
	int	mtmr1;			/* Maintenance register 	*/
	int	mtas;			/* Attention summary register	*/
	int	mtbc;			/* Byte count register		*/
	int	mtdt;			/* Drive type register		*/
	int	mtds;			/* Drive status register	*/
	int	mtsn;			/* Serial number register	*/
	int	mtmr2;			/* Maintenance register 	*/
	int	mtmr3;			/* Maintenance register 	*/
	int	mtner;			/* Non-data transfer error reg. */
	int	mtncs[4];		/* Non-data transfer cmd regs.	*/
	int	mtia;			/* Internal address		*/
	int	mtid;			/* Internal data		*/
};

/* Control status register definitions (mtcs) */
#define MT_GO		0x01		/* Go bit			*/
#define MT_NOOP 	0x02		/* No operation 		*/
#define MT_GCR		0x02		/* Make generic ops GCR ops	*/
#define MT_UNLOAD	0x04		/* Unload tape			*/
#define MT_REW		0x06		/* Rewind			*/
#define MT_SENSE	0x08		/* Eense			*/
#define MT_DSE		0x0a		/* Data security erase		*/
#define MT_WTMPE	0x0c		/* Write phase enc. tape mark	*/
#define MT_WTM		MT_WTMPE	/* Generic write tape mark	*/
#define MT_WTMGCR	0x0e		/* Write GCR tape mark		*/
#define MT_SFORW	0x10		/* Space forward record 	*/
#define MT_SREV 	0x12		/* Space reverse record 	*/
#define MT_SFORWF	0x14		/* Space forward file		*/
#define MT_SREVF	0x16		/* Space reverse file		*/
#define MT_SFORWE	0x18		/* Space forward either 	*/
#define MT_SREVE	0x1a		/* Space reverse either 	*/
#define MT_ERGPE	0x1c		/* Erase tape, set PE		*/
#define MT_ERASE	MT_ERGPE	/* Generic erase tape		*/
#define MT_ERGGCR	0x1e		/* Erase tape, set GCR		*/
#define MT_CLSPE	0x20		/* Close file PE		*/
#define MT_CLS		MT_CLSPE	/* Generic close file		*/
#define MT_CLSGCR	0x22		/* Close file GCR		*/
#define MT_SLEOT	0x24		/* Space to logical EOT 	*/
#define MT_SFLEOT	0x26		/* Space forward file onto LEOT */
#define MT_WCHFWD	0x28		/* Write check forward		*/
#define MT_WCHREV	0x2e		/* Write check reverse		*/
#define MT_WRITEPE	0x30		/* Write phase encoded		*/
#define MT_WRITE	MT_WRITEPE	/* Generic write		*/
#define MT_WRITEGCR	0x32		/* Write group coded		*/
#define MT_READ 	0x38		/* Read forward 		*/
#define MT_EXSNS	0x3a		/* Read extnd. sense error log. */
#define MT_READREV	0x3e		/* Read reverse 		*/

/* Drive type register definitions (mtdt)
 * Bits 0 - 8 are formatter/transport type
 * Bit 9 is spare
 */
#define MTDT_SPR	0x400		/* Slave present; always 1 ???	*/
#define MTDT_DRQ	0x800		/* Drive request required	*/
#define MTDT_7CH	0x1000		/* 7 channel; always 0		*/
#define MTDT_MOH	0x2000		/* Moving head; always 0	*/
#define MTDT_TAP	0x4000		/* Tape; always 1		*/
#define MTDT_NSA	0x8000		/* Not sector addrsd.; always 1 */

/* Drive status register definitions (mtds) */
#define MTDS_DSE	0x10		/* DSE in progress		*/
#define MTDS_MAINT	0x20		/* Maintenance mode		*/
#define MTDS_SHR	0x40		/* Unit is shared		*/
#define MTDS_AVAIL	0x80		/* Unit available		*/
#define MTDS_FPT	0x100		/* Write protected		*/
#define MTDS_EOT	0x200		/* Tape at EOT			*/
#define MTDS_BOT	0x400		/* Tape at BOT			*/
#define MTDS_PE 	0x800		/* Tape set for phase encoded	*/
#define MTDS_REW	0x1000		/* Tape rewinding		*/
#define MTDS_ONL	0x2000		/* Online			*/
#define MTDS_PRES	0x4000		/* Tape unit has power		*/
#define MTDS_RDY	0x8000		/* Tape ready			*/
#define MTDS_BITS	"\10\20RDY\17PRES\
			 \16ONL\15REW\14PE\
			 \13BOT\12EOT\11FPT\
			 \10AVAIL\7SHR\6MAINT\
			 \5DSE"                 /* Status bits          */

/* Internal data (mtid) */
#define MTID_CLR	0x4000		/* Controller clear */
#define MTID_RDY	0x8000		/* Controller ready */

/* Error register definitions (mter) */
#define MTER_DONE	0x01		/* Operation complete		*/
#define MTER_TM 	0x02		/* Unexpected tape mark 	*/
#define MTER_BOT	0x03		/* Unexpected BOT detected	*/
#define MTER_EOT	0x04		/* Tape positioned beyond EOT	*/
#define MTER_LEOT	0x05		/* Unexpected LEOT detected	*/
#define MTER_NOOP	0x06		/* No-op completed		*/
#define MTER_RWDING	0x07		/* Rewinding			*/
#define MTER_FPT	0x08		/* Write protect error		*/
#define MTER_NOTRDY	0x09		/* Not ready			*/
#define MTER_NOTAVL	0x0a		/* Not available		*/
#define MTER_OFFLINE	0x0b		/* Offline			*/
#define MTER_NONEX	0x0c		/* Unit does not exist		*/
#define MTER_NOTCAP	0x0d		/* Not capable			*/
#define MTER_ONLINE	0x0f		/* Tape came online		*/
#define MTER_LONGREC	0x10		/* Long tape record		*/
#define MTER_SHRTREC	0x11		/* Short tape record		*/
#define MTER_RETRY	0x12		/* Retry			*/
#define MTER_RDOPP	0x13		/* Read opposite		*/
#define MTER_UNREAD	0x14		/* Unreadable			*/
#define MTER_ERROR	0x15		/* Error			*/
#define MTER_EOTERR	0x16		/* EOT error			*/
#define MTER_BADTAPE	0x17		/* Tape position lost		*/
#define MTER_TMFLTA	0x18		/* TM fault A			*/
#define MTER_TUFLTA	0x19		/* TU fault A			*/
#define MTER_TMFLTB	0x1a		/* TM fault B			*/
#define MTER_MBFLT	0x1c		/* Massbus fault		*/
#define MTER_KEYFAIL	0x3f		/* Keypad entry error		*/
#define MTER_INTCODE	0x3f		/* Mask for interrupt code	*/

/* Driver and data specific structure */
struct	mu_softc {
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
	short	sc_firsttime;
	short	sc_savestate;
	short	sc_changedstate;
};

/* Driver and data specific definitions */
#define MTUNIT(dev)	(mutomt[UNIT(dev)]) /* Controller slave no.	*/
