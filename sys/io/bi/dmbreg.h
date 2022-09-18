
/*
 *	@(#)dmbreg.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986-1988 by			*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * dmbreg.h
 *
 * Modification history
 *
 * DMB32 registers/data structures and definitions
 *
 * 29-Dec-87 - Tim Burke
 *
 *	Added defines for the configuration register.
 *	Extend to 16 lines for DHB support.
 *
 * 29-Jan-87 - Tim Burke
 *
 *	Added definition of DMB_NODSR, which will represent the modem signals
 *	carrier detect and clear to send.
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 */

#include "../io/bi/bireg.h"

/* Register device structure
 *
 * In the "dmb_device" structure, registers which are specific to the
 * synchronous line on the DMB are not always named.  Where several sync
 * registers occur together they are padded as a group.
 *
 * In the bit definitions that follow, Sync bit fields are not defined.
 */
struct dmb_device {
	struct	biic_regs dmb_biic;	/* BI & BIIC specific registers */
	u_char	dmb_maintlow;		/* Maintenance register (R/W)	*/
	u_char	dmb_mainthigh;		/* Maintenance register (R/O)	*/
	short	dmb_maintunused;	/* Maintenance reg., unused	*/
	long	dmb_acsr;		/* Async CSR			*/
	long	dmb_scsr;		/* Sync CSR			*/
	short	dmb_pcsrlow;		/* Printer CSR: PRIE (R/W)	*/
	short	dmb_pcsrhigh;		/* Printer CSR: status (R/O)	*/
	long	dmb_pad1;		/* pad 1 word			*/
	long	dmb_config;		/* Device configuration 	*/
	long	dmb_acsr2;		/* 2nd async CSR		*/
	long	dmb_scsr2;		/* 2nd sync CSR 		*/
	long	dmb_pad2[11];		/* pad 11 words 		*/
	long	dmb_pcsr2;		/* 2nd printer CSR		*/
	long	dmb_spte;		/* System page table register	*/
	long	dmb_spts;		/* System page table size	*/
	long	dmb_gpte;		/* Global page table register	*/
	long	dmb_gpts;		/* Global page table size	*/
	long	dmb_pfix;		/* Printer prefix/suffix control*/
	long	dmb_pbufad;		/* Printer buffer address	*/
	long	dmb_pbufct;		/* Printer buffer count 	*/
	long	dmb_pctrl;		/* Printer control		*/
	long	dmb_pcar;		/* Printer carriage counter	*/
	long	dmb_psiz;		/* Printer page size descriptor */
	long	dmb_pad3[2];		/* Pad 2 words			*/
	long	dmb_syncpad[16];	/* Padding for Sync line regs.	*/
	long	dmb_preempt;		/* Transmission preempt buffer	*/
	long	dmb_tbuffadd;		/* Transmit buffer address	*/
	long	dmb_tbuffct;		/* Transmit buffer count/offset */
	long	dmb_lpr;		/* Line parameter register	*/
	union {
	    long  dmb_LNCTRL;
	    struct {
		char dmb_STARTDMA;
		char dmb_ABORT;
		short dmb_ERROR;
	    } dmb_lnctl;
	} dmb_lnctrl;		/* Line Control register	*/
	u_short dmb_lstatlow;		/* Line stat reg: modem ctrl R/O*/
	u_short dmb_lstathigh;		/* Line stat reg: xmit ena. R/W */
	long	dmb_flowc;		/* Flow control characters	*/
	long	dmb_pad4[10];		/* Padding for rsrvd.area/cons. */
	long	dmb_tbuf;		/* Transmit completion FIFO	*/
	long	dmb_sbuf;		/* Sync line completion FIFO	*/
	long	dmb_rbuf;		/* Receive buffer		*/
};

#define dmb_startdma dmb_lnctrl.dmb_lnctl.dmb_STARTDMA
#define dmb_abort dmb_lnctrl.dmb_lnctl.dmb_ABORT
#define dmb_error dmb_lnctrl.dmb_lnctl.dmb_ERROR

/*
 * bits in Configuration Register
 */
#define DMB_ALINES	0xff		/* Number of async lines	*/
#define DMB_SLINES	0xff00		/* Number of sync lines 	*/
#define DMB_PLINES	0xff0000	/* Number of printer ports	*/

/*
 * bits in Maintenance Register
 */
/* low byte (bits 0-7) (R/W) */
#define DMB_FFAIL	0x01		/* Force Failure		*/
#define DMB_RESET	0x02		/* Programmed reset		*/
#define DMB_PTEVALID	0x04		/* Page tables valid		*/
#define DMB_SST 	0x08		/* Skip self test		*/
#define DMB_MAINTLEV	0x30		/* Maintenance level		*/
/* 2nd byte (bits 8-15) (R/O) */
#define DMB_ALP 	0x02		/* Async lines present		*/
#define DMB_PP		0x04		/* Printer present		*/
#define DMB_DFAIL	0x08		/* Diagnostic error		*/
#define DMB_CABLE	0x20		/* Cables connected		*/

/*
 * bits in Async CSR
 */
#define DMB_AINDADD	0xff		/* Channel number indirect regs.*/
#define DMB_RIE 	0x100		/* Receive interrupt enable	*/
#define DMB_TIE 	0x200		/* Transmit interrupt enable	*/

#define DMB_IE (DMB_RIE|DMB_TIE)

/*
 * bits in 2nd Async CSR
 */
#define DMB_ARESET	0x400		/* Async port reset		*/
#define DMB_RTIMER	0xff0000	/* Receive interrupt delay timer*/

/*
 * bits in configuration register
 */
#define DMB_ASYNC_MASK	0xff		/* Number of asychronous lines	*/
#define DMB_8_LINES	0x08		/* DMB has 8 async lines	*/
#define DMB_16_LINES	0x10		/* DHB has 16 async lines	*/
#define DMB_SYNC_MASK	0xff00		/* Number of sychronous lines	*/
#define DMB_PRINT_MASK	0xff0000	/* Number of printer lines	*/

/*
 * bits in Async PREEMPT register
 */
#define DMB_PRECHAR	0xff		/* Char to xmit immediately	*/
#define DMB_PREGO	0x8000		/* Start preempt xmission	*/

/*
 * bits in Async TBUFFCT register
 */
#define DMB_TXCHARCT	0x10		/* Offset of Transmit DMA count */
#define DMB_TXBUFFOFF	0x1ff		/* Transmit buffer offset	*/

/*
 * bits in Async LPR
 */
#define DMB_ML		0x01		/* Modem loop			*/
#define DMB_DTR 	0x02		/* Data terminal ready		*/
#define DMB_DRS 	0x04		/* Data rate select		*/
#define DMB_RTS 	0x10		/* Request to send		*/
#define DMB_TXINTDELAY	0x200		/* Transmit interrupt control	*/
#define DMB_RXENA	0x400		/* Receiver enable		*/
#define DMB_BREAK	0x800		/* Break control		*/
#define DMB_MAINT	0x2000		/* Maint. mode: local loopback	*/
#define DMB_REPORT	0x4000		/* Report modem changes 	*/
#define DMB_DISCARD	0x8000		/* Discard flow control chars	*/
#define DMB_CHARLGTH	0x30000 	/* Character length		*/
#define DMB_BITS5	0x00		/* 5 bits per Character 	*/
#define DMB_BITS6	0x10000 	/* 6 bits per Character 	*/
#define DMB_BITS7	0x20000 	/* 7 bits per Character 	*/
#define DMB_BITS8	0x30000 	/* 8 bits per Character 	*/
#define DMB_PARITYENAB	0x40000 	/* Parity enable		*/
#define DMB_EVENPARITY	0x80000 	/* Even parity			*/
#define DMB_TWOSTOPB	0x100000	/* Stop code			*/
#define DMB_USECTS	0x200000	/* CTS controls output		*/
#define DMB_IAUTOFLOW	0x400000	/* Auto flow control of input	*/
#define DMB_OAUTOFLOW	0x800000	/* Auto flow control of output	*/
#define DMB_RXSPEED	0xf000000	/* Receive data rate		*/
#define DMB_TXSPEED	0xf0000000	/* Transmit data rate		*/

/*
 * for modem control
 */
#define DMB_OFF 0
#define DMB_ON (DMB_DTR|DMB_RTS)

/*
 * bits in Async LNCTRL
 */
#define DMB_TXDMASTART	0x01		/* Start DMA transfer		*/
#define DMB_TXDMAPTE	0x02		/* PTE address			*/
#define DMB_TXDMAPHYS	0x04		/* Physical address		*/
#define DMB_TXOUTABORT	0x01		/* Transmit output abort	*/
#define DMB_TXERROR	0xff		/* Transmitter error bits	*/

/*
 * bits in Async LSTAT
 */
/* low 16 bits (R/O) */
#define DMB_ML2 	0x400		/* Spare modem control lead	*/
#define DMB_CTS 	0x1000		/* Clear to send		*/
#define DMB_DCD 	0x2000		/* Data carrier detected	*/
#define DMB_RI		0x4000		/* Ring indicator		*/
#define DMB_DSR 	0x8000		/* Data set ready		*/
/* high 16 bits (R/W) */
#define DMB_SNDOFF	0x80		/* Send XOFF			*/
#define DMB_TXENA	0x8000		/* Transmitter enable		*/

#define DMB_XMIT (DMB_CTS|DMB_DCD|DMB_DSR)
#define DMB_NODSR (DMB_DCD | DMB_CTS)	/* Instead of DMB_XMIT 		*/

/*
 * bits in Async FLOWC
 */
#define DMB_SENTXOFF	0xff		/* Transmitted XOFF		*/
#define DMB_SENTXON	0xff00		/* Transmitted XON		*/
#define DMB_RECXOFF	0xff0000	/* Received XOFF		*/
#define DMB_RECXON	0xff000000	/* Received XON 		*/

/*
 * bits in Async TBUF
 */
#define DMB_TXLINE	0xff		/* Transmit line number 	*/
#define DMB_TXPREEMPT	0x100		/* Preempt completed		*/
#define DMB_ERRMASK	0xff0000	/* Error bits in tbuf & lnctrl	*/
#define DMB_ERMASK	0xf00000	/* Upper Error bits in tbuf 	*/
#define DMB_TXACT	0x80000000	/* Transmitter action		*/

/*
 * Interpretation of error byte in tbuf & lnctrl registers
 */
#define DMB_TXDMAERROR	0x100000	/* Transmit DMA error bit	*/
#define DMB_MSGERR	0x200000	/* Message error		*/
#define DMB_LASTCHERR	0x300000	/* Last char incomplete 	*/
#define DMB_BUFERR	0x400000	/* Buffer error 		*/
#define DMB_MODEMERR	0x500000	/* Modem error			*/
#define DMB_INTERNALERR 0x900000	/* Internal Error		*/

/*
 * bits in Async RBUF
 */
#define DMB_DIAG	0x01		/* Diagnostic indicator 	*/
#define DMB_RXCHAR	0xff		/* Received character		*/
#define DMB_PARITYERR	0x1000		/* Parity error 		*/
#define DMB_FRAMEERR	0x2000		/* Framing error		*/
#define DMB_OVERRUNERR	0x4000		/* Overrun error		*/
#define DMB_NONCHAR	0x8000		/* Non character data		*/
#define DMB_RXLINE	0xff0000	/* Receive line number		*/
#define DMB_DATAVALID	0x80000000	/* Data valid			*/

/*
 * bits in Printer CSR
 */
/* low 16 bits (R/W) */
#define DMB_PRIE	0x800		/* Printer interrupt enable	*/
/* high 16 bits (R/O) */
#define DMB_PRDAVR	0x01		/* DAVFU ready			*/
#define DMB_PRCONNECT	0x02		/* Connect verify (connected)	*/
#define DMB_PROFFLINE	0x04		/* Printer offline		*/

/*
 * bits in 2nd Printer CSR
 */
#define DMB_PRESET	0x400		/* Printer port reset		*/

/*
 * bits in printer PFIX register
 */

#define DMB_PCOUNT	0xff		/* Prefix count 		*/
#define DMB_PCHAR	0xff00		/* Prefix character		*/
#define DMB_SCOUNT	0xff0000	/* Suffix count 		*/
#define DMB_SCHAR	0xff000000	/* Suffix char			*/

/*
 * bits in printer Buffer Count register
 */

#define DMB_PRCHARCT	0x10		/* Offset of Xmit DMA char count*/
#define DMB_PRBUFFOFF	0x1ff		/* Printer buffer offset	*/

/*
 * bits in printer Control register
 */

#define DMB_PRSTART	0x01		/* Printer DMA start		*/
#define DMB_PRPTE	0x02		/* Buffer address is addr of PTE*/
#define DMB_PRPHYS	0x04		/* Buffer address is physical	*/
#define DMB_PRABORT	0x100		/* Abort a DMA xfer		*/
#define DMB_PRFMT	0x200		/* Format control		*/
#define DMB_PRERROR	0xff0000	/* Error code			*/
#define DMB_PRTAB	0x1000000	/* Tab expansion		*/
#define DMB_PRTRUNC	0x2000000	/* Truncate long lines		*/
#define DMB_PRCR	0x4000000	/* Insert carriage returns	*/
#define DMB_PRFORM	0x8000000	/* Convert form to line feeds	*/
#define DMB_PRNONP	0x10000000	/* Accept non printing chars.	*/
#define DMB_PRDAVFU	0x20000000	/* Accept DAVFU control codes	*/
#define DMB_PRWRAP	0x40000000	/* Line wrap			*/
#define DMB_PRUPPER	0x80000000	/* Convert to upper case	*/

/*
 * bits in printer Carriage Counter register
 */

#define DMB_PRLINE	0xffff		/* Number of lines printed	*/
#define DMB_PRCHAR	0xffff0000	/* Number of chars printed	*/

/*
 * bits in printer Page Size register
 */

#define DMB_PRPAGE	0x10		/* Offset of Lines per Page	*/
#define DMB_PRWIDTH	0xffff		/* Columns per line		*/

/*
 * Line printer defaults
 */

#define DMBL_BUFSIZE	512		/* Max. chars. per dma		*/
#define DMBL_DEFCOLS	132		/* Default number of cols./line */
#define DMBL_DEFLINES	66		/* Default number of lines/page */

/*
 * Line printer software context
 */
struct dmbl_softc {
	unsigned dmbl_state;		/* Soft state bits		*/
	unsigned short dmbl_lines;	/* Lines per page (66 def.)	*/
	unsigned short dmbl_cols;	/* Cols per line (132 def.)	*/
	char dmbl_buf[DMBL_BUFSIZE];
};

#define DMBLINES 16			/* The DHB supports 16 lines.	*/

/* Driver and data specific structure */
struct	dmb_softc {
	long	sc_flags[DMBLINES];		/* Flags 		*/
	long	sc_category_flags[DMBLINES];	/* Category flags 	*/
	u_long	sc_softcnt[DMBLINES];		/* Soft error count total*/
	u_long	sc_hardcnt[DMBLINES];		/* Hard error count total*/
	char	sc_device[DEV_SIZE][DMBLINES]; /* Device type string	*/
};
