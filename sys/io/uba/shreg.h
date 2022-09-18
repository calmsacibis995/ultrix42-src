
/*
 * 	@(#)shreg.h	4.1	(ULTRIX)	7/2/90
 */

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
 * shreg.h
 *
 * Modification history
 *
 * MicroVAX 2000 serial line expander (8 line SLU)
 * registers, data structures, and defines
 *
 *   2-Jul-86  -- fred (Fred Canter)
 *	Created this header file for first pass MicroVAX 2000
 *	serial line expander (8 line SLU) driver.
 *	Derived from dhureg.h (delta 1.2).
 *
 *   4-Dec-86  -- fred (Fred Canter)
 *	Modified this header file for the real MicroVAX 2000
 *	serial line expander (8 line SLU) driver.
 *
 * 29-Jan-87 - Tim Burke
 *
 *	Added definition of SH_NODSR, which will represent the modem signals
 *	carrier detect and clear to send.
 *
 */

/*
 * VAXstar interrupt controller bit definitions
 */

#define	SINT_VF	010		/* Receive FIFO not empty */
#define	SINT_VS	04		/* Transmit FIFO empty */

/*
 * Device register structure
 *
 * DHU11 register names used to maintain consistency
 * with DHU11 driver (so changes in dhu.c port easily to sh.c).
 * MicroVAX 2000 SLU register names shown in comments.
 */

struct shdevice {
	struct {
		char low;
		char high;
	} csr;	 			/* 0 SOP_CSR  Cntrl status reg	*/
	union {
		short rbuf;		/* 2 SOP_RBUF Receive buffer	*/
		char rxtimer;		/* 2 SOP_RTIM Receive timer	*/
	} run;
	u_short lpr;			/* 4 SOP_LPR  Line param reg	*/
	union {				/* 6 SOP_DATA			*/
		short fifodata; 	/*	Fifo data		*/
		struct	{
			char fifosize;	/*	Fifo size		*/
			char stat;	/*	Line status		*/
		} fs;
	} fun;
	u_short lnctrl;			/* 8 SOP_CNTL Line control	*/
	u_short tbuffad1;		/* A NOT USED Xmit. bufr addr 1	*/
	struct	{
		char low;
		char high;
	} tbuffad2;			/* C SOP_TXA  Xmit. bufr addr 2	*/
	u_short tbuffcnt;		/* E NOT USED Xmit. bufr count	*/
};

/*
 * Many of the following definitions are not used,
 * but are maintained for possible future expansion.
 */

/* Control status register low definitions (csr.low) */
#define SH_SKIP		0x10		/* Skip self test		*/
#define SH_MRESET	0x20		/* Master reset 		*/
#define SH_RIE	 	0x40		/* Receiver interrupte enable	*/
#define SH_RDATA	0x80		/* Received data available	*/

/* Control status register high definitions (csr.high) */
#define SH_DMAERR	0x10		/* Transmit dma error		*/
#define SH_DIAGFAIL	0x20		/* Diagnostic failure		*/
#define SH_XIE	 	0x40		/* Transmit interrupt enable	*/
#define SH_TA		0x80		/* Transmitter action		*/

/* Run register receive buffer definitions (run.rbuf) */
#define SH_DIAG		0x01		/* STAT && DIAG implies diag.in.*/
#define SH_PERR		0x1000		/* Parity error 		*/
#define SH_FERR		0x2000		/* Framing error		*/
#define SH_OVERR	0x4000		/* Overrun error		*/
#define SH_STAT		0x7000		/* Modem status or diag. info.	*/
#define SH_VALID	0x8000		/* Data valid			*/

/* Line parameter register definitons (lpr) */
#define SH_BITS5	0x00		/* 5 bit character		*/
#define SH_BITS6	0x08		/* 6 bit character		*/
#define SH_BITS7	0x10		/* 7 bit character		*/
#define SH_BITS8	0x18		/* 8 bit character		*/
#define SH_PENABLE	0x20		/* Parity enable		*/
#define SH_EVENPAR	0x40		/* Even parity			*/
#define SH_TWOSB	0x80		/* Stop bits: set = 2		*/

/* Line status register definitions (fs.stat) */
#define	SH_MSTAT	0x02		/* Set = no modem control	*/
#define SH_CTS	 	0x08		/* Clear to send from modem	*/
#define SH_CD		0x10		/* Data carrier detected	*/
#define SH_RING		0x20		/* Ring indicator		*/
#define SH_DSR	 	0x80		/* Data set ready		*/
#define SH_XMIT	(SH_DSR|SH_CD|SH_CTS)	/* Transmit with CD|CTS|DSR	*/
#define SH_NODSR (SH_CD|SH_CTS)		/* Transmit with CD|CTS		*/

/* Line control register definitions (lnctrl) */
#define SH_XABORT	0x01		/* Transmitter abort		*/
#define SH_RFLOW	0x02		/* Turn on XON/XOFF control	*/
#define SH_REN	 	0x04		/* Receive enable		*/
#define SH_BREAK	0x08		/* Transmit break		*/
#define SH_XFLOW	0x10		/* Respond to XON/XOFF		*/
#define SH_FXOFF	0x20		/* Force XOFF to be sent	*/
#define SH_MAINT	0x80		/* Local loopback mode		*/
#define SH_MODEM	0x100		/* Modem link			*/
#define SH_DTR	 	0x200		/* Data terminal ready		*/
#define SH_RTS	 	0x1000		/* Request to send		*/

/* Transmit buffer address 2 register definitions (tbuffad2) */
#define SH_START	0x80		/* Start dma transfer (low byte)*/
#define SH_XEN	 	0x80		/* Transmitter ena. (high byte) */

/* Driver and data specific structure */
struct	sh_softc {
	long	sc_flags[8];		/* Flags (one per line) 	*/
	long	sc_category_flags[8];	/* Category flags (one per line)*/
	u_long	sc_softcnt[8];	 	/* Soft error count total	*/
	u_long	sc_hardcnt[8];	 	/* Hard error count total	*/
	char	sc_device[DEV_SIZE][8];	/* Device type string		*/
};
