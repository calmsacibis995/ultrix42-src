
/*
 * 	@(#)dhureg.h	4.1		(ULTRIX)	7/2/90	
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
 * dhureg.h
 *
 * Modification history
 *
 * DH(QUV)11/CX(ABY)(8,16) registers/data structures and definitions
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 * 10-Mar-87 - rsp (Ricky Palmer)
 *
 *	Added DHU_MDL (modem support) field.
 *
 */

/* Register device structure */
struct dhudevice {
	struct {
		char low;
		char high;
	} csr ; 			/* Control status register	*/
	union {
		short rbuf;		/* Receive buffer		*/
		char rxtimer;		/* Receive timer		*/
	} run;
	u_short 	lpr;		/* Line parameter register	*/
	union {
		short fifodata; 	/* Fifo data			*/
		struct	{
			char fifosize;	/* Fifo size			*/
			char stat;	/* Line status			*/
		} fs;
	} fun;
	u_short lnctrl; 		/* Line control 		*/
	u_short tbuffad1;		/* Xmit. buffer address 1	*/
	struct	{
		char low;
		char high;
	} tbuffad2;			/* Xmit. buffer address 2	*/
	u_short tbuffcnt;		/* Xmit. buffer count		*/
};

/* Control status register low definitions (csr.low) */
#define DHU_SKIP	0x10		/* Skip self test		*/
#define DHU_MRESET	0x20		/* Master reset 		*/
#define DHU_RIE 	0x40		/* Receiver interrupte enable	*/
#define DHU_RDATA	0x80		/* Received data available	*/

/* Control status register high definitions (csr.high) */
#define DHU_DMAERR	0x10		/* Transmit dma error		*/
#define DHU_DIAGFAIL	0x20		/* Diagnostic failure		*/
#define DHU_XIE 	0x40		/* Transmit interrupt enable	*/
#define DHU_TA		0x80		/* Transmitter action		*/

/* Run register receive buffer definitions (run.rbuf) */
#define DHU_DIAG	0x01		/* STAT && DIAG implies diag.in.*/
#define DHU_PERR	0x1000		/* Parity error 		*/
#define DHU_FERR	0x2000		/* Framing error		*/
#define DHU_OVERR	0x4000		/* Overrun error		*/
#define DHU_STAT	0x7000		/* Modem status or diag. info.	*/
#define DHU_VALID	0x8000		/* Data valid			*/

/* Line parameter register definitons (lpr) */
#define DHU_BITS5	0x00		/* 5 bit character		*/
#define DHU_BITS6	0x08		/* 6 bit character		*/
#define DHU_BITS7	0x10		/* 7 bit character		*/
#define DHU_BITS8	0x18		/* 8 bit character		*/
#define DHU_PENABLE	0x20		/* Parity enable		*/
#define DHU_EVENPAR	0x40		/* Even parity			*/
#define DHU_TWOSB	0x80		/* Stop bits: set = 2		*/

/* Line status register definitions (fs.stat) */
#define DHU11		0x01		/* On = DHU11, off = DHV11	*/
#define DHU_MDL		0x02		/* On = modem support, off=none */
#define DHU_CTS 	0x08		/* Clear to send from modem	*/
#define DHU_CD		0x10		/* Data carrier detected	*/
#define DHU_RING	0x20		/* Ring indicator		*/
#define DHU_DSR 	0x80		/* Data set ready		*/
#define DHU_XMIT	(DHU_DSR|DHU_CD|DHU_CTS)	/* Transmit	*/

/* Line control register definitions (lnctrl) */
#define DHU_XABORT	0x01		/* Transmitter abort		*/
#define DHU_RFLOW	0x02		/* Turn on XON/XOFF control	*/
#define DHU_REN 	0x04		/* Receive enable		*/
#define DHU_BREAK	0x08		/* Transmit break		*/
#define DHU_XFLOW	0x10		/* Respond to XON/XOFF		*/
#define DHU_FXOFF	0x20		/* Force XOFF to be sent	*/
#define DHU_MAINT	0x80		/* Local loopback mode		*/
#define DHU_MODEM	0x100		/* Modem link			*/
#define DHU_DTR 	0x200		/* Data terminal ready		*/
#define DHU_RTS 	0x1000		/* Request to send		*/

/* Transmit buffer address 2 register definitions (tbuffad2) */
#define DHU_START	0x80		/* Start dma transfer (low byte)*/
#define DHU_XEN 	0x80		/* Transmitter ena. (high byte) */

/* After the board self test it return codes decribing the state of the board */
#define DHU_NUM_ERR_CODES 8

/* Driver and data specific structure */
struct	dhu_softc {
	long	sc_flags[16];		/* Flags (one per line) 	*/
	long	sc_category_flags[16];	/* Category flags (one per line)*/
	u_long	sc_softcnt[16]; 	/* Soft error count total	*/
	u_long	sc_hardcnt[16]; 	/* Hard error count total	*/
	char	sc_device[DEV_SIZE][16];/* Device type string		*/
	long	sc_self_test[DHU_NUM_ERR_CODES]; /* self test codes	*/
};
