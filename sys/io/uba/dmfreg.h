
/*
 * 	@(#)dmfreg.h	4.1	(ULTRIX)	7/2/90
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
 * dmfreg.h	6.1	   7/29/83
 *
 * Modification history
 *
 * DMF32 registers/data structures and definitions
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: dmfreg.h	6.1	83/07/29.
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 * 29-Jan-87 - Tim Burke
 *
 *	Added definition of DMF_NODSR, which will represent the modem signals
 *	carrier detect and clear to send.
 *
 */

/* Register device structure
 *
 * "dmf" (unqualified) refers to the async portion of the dmf32,
 * "dmfc" to the combo portion,
 * "dmfs" to the sync portion,
 * "dmfl" to the lp portion, and
 * "dmfd" to the dr portion.
 */
struct dmfdevice {
	short	dmfccsr0;		/* Combo csr 0			*/
	short	dmfccsr1;		/* Combo csr 1			*/
	short	dmfs[4];
	short	dmfcsr; 		/* Control status register	*/
	short	dmflpr; 		/* Line parameter register	*/
	short	dmfrbuf;		/* Receiver buffer (ro) 	*/
	union {
		u_short dmfirw; 	/* Indirect register word	*/
		u_char	dmfirc[2];	/*    "         "    bytes	*/
	} dmfun;
	short	dmfl[2];
	short	dmfd[4];
};

#define dmfrsp	dmfrbuf 		/* Receive silo param. reg.(wo) */
#define dmftbuf dmfun.dmfirc[0] 	/* Transmit buffer		*/
#define dmftsc	dmfun.dmfirc[0] 	/* Transmit silo count		*/
#define dmfrms	dmfun.dmfirc[1] 	/* Receive modem status 	*/
#define dmflcr	dmfun.dmfirc[0] 	/* Line control register	*/
#define dmftms	dmfun.dmfirc[1] 	/* Transmit modem status	*/
#define dmftba	dmfun.dmfirw		/* Transmit buffer address	*/
#define dmftcc	dmfun.dmfirw		/* Transmit character count	*/

/* Control status register low definitions (dmfcsr) */
#define DMF_IAD 	0x1f		/* Indirect address register	*/
#define DMF_CLR 	0x20		/* Master reset 		*/
#define DMF_RIE 	0x40		/* Receiver interrupt enable	*/
#define DMF_RI		0x80		/* Receiver interrupt		*/
#define DMF_LIN 	0x700		/* Transmit line number 	*/
#define DMF_NXM 	0x2000		/* Non-existent memory		*/
#define DMF_TIE 	0x4000		/* Transmit interrupt enable	*/
#define DMF_TI		0x8000		/* Transmit interrupt		*/
#define DMF_IE		(DMF_TIE|DMF_RIE)	/* Interrupt enable	*/
#define DMF_SILOCNT	32		/* Size of DMF output per line	*/

/* Indirect register definitions (dmfun.dmfirw, dmfun.dmfirc) */
#define DMFIR_TBUF	0x00		/* Select tbuf indirect reg.	*/
#define DMFIR_LCR	0x08		/* Select lcr indirect register */
#define DMFIR_TBA	0x10		/* Select tba indirect register */
#define DMFIR_TCC	0x18		/* Select tcc indirect register */

/* Line parameter register definitions (dmflpr) */
#define BITS5		0x00
#define BITS6		(01<<3)
#define BITS7		(02<<3)
#define BITS8		(03<<3)
#define PENABLE 	0x20
#define EPAR		0x40
#define TWOSB		0x80

/* Receiver buffer register definitions (dmfrbuf) */
#define DMF_DSC 	0x800		/* Data set change		*/
#define DMF_PE		0x1000		/* Parity error 		*/
#define DMF_FE		0x2000		/* Framing error		*/
#define DMF_DO		0x4000		/* Data overrun 		*/

/* Receive modem status definitions (dmfrms) */
#define DMF_USRR	0x04		/* User modem signal (pin 25)	*/
#define DMF_SR		0x08		/* Secondary receive		*/
#define DMF_CTS 	0x10		/* Clear to send		*/
#define DMF_CAR 	0x20		/* Carrier detect		*/
#define DMF_RNG 	0x40		/* Ring 			*/
#define DMF_DSR 	0x80		/* Data set ready		*/
#define DMF_XMIT	(DMF_CTS|DMF_CAR|DMF_DSR)
#define DMF_NODSR 	(DMF_CAR|DMF_CTS)/* Instead of DMF_XMIT 	*/

/* Transmit modem status definitions (dmftms) */
#define DMF_OFF 	0x00		/* Modem control off		*/
#define DMF_USRW	0x01		/* User modem signal (pin 18)	*/
#define DMF_DTR 	0x02		/* Data terminal ready		*/
#define DMF_RATE	0x04		/* Data signal rate select	*/
#define DMF_ST		0x08		/* Secondary transmit		*/
#define DMF_RTS 	0x10		/* Request to send		*/
#define DMF_BRK 	0x20		/* Pseudo break bit		*/
#define DMF_PREEMPT	0x80		/* Pre-empt output		*/
#define DMF_ON		(DMF_DTR|DMF_RTS)	/* Modem control on	*/

/* Line control register definitions (dmflcr) */
#define DMF_TE		0x01		/* Transmit enable		*/
#define DMF_AUTOX	0x02		/* Auto XON/XOFF		*/
#define DMF_RE		0x04		/* Receive enable		*/
#define DMF_RBRK	0x08		/* Real break bit		*/
#define DMF_FLUSH	0x10		/* Flush transmit silo		*/
#define DMF_MIE 	0x20		/* Modem interrupt enable	*/
#define DMFLCR_ENA	(DMF_MIE|DMF_RE|DMF_TE) /* Line control enable	*/

/* DM lsr definitions */
#define DML_LE		0x01		/* Line enable			*/
#define DML_DTR 	0x02		/* Data terminal ready		*/
#define DML_RTS 	0x04		/* Request to send		*/
#define DML_ST		0x08		/* Secondary transmit		*/
#define DML_SR		0x10		/* Secondary receive		*/
#define DML_CTS 	0x20		/* Clear to send		*/
#define DML_CAR 	0x40		/* Carrier detect		*/
#define DML_RNG 	0x80		/* Ring 			*/
#define DML_DSR 	0x100		/* Data set ready, not DM bit	*/
#define DML_USR 	0x200		/* User modem sig., not DM bit	*/
#define SETLCR(pt, exp) \
	pt->dmfun.dmfirw = (((pt)->dmftms)<<8) | ((exp)&0xff)

/* Line printer csr definitions */
#define DMFL_FORMAT	0x00		/* No format control		*/
#define DMFL_PEN	(1<<0)		/* Print enable 		*/
#define DMFL_RESET	(1<<1)		/* Master reset 		*/
#define DMFL_UNUSED	(3<<3)
#define DMFL_MAINT	(1<<5)		/* Mainenance mode on		*/
#define DMFL_IE 	(1<<6)		/* Interrupt enable		*/
#define DMFL_PDONE	(1<<7)		/* Print done bit		*/
#define DMFL_INDIR	(7<<8)		/* Indirect reg.		*/
#define DMFL_UNUSED2	(1<<11)
#define DMFL_CONV	(1<<12) 	/* Connect verify		*/
#define DMFL_DAVRDY	(1<<13) 	/* Davfu ready			*/
#define DMFL_OFFLINE	(1<<14) 	/* Printer offline		*/
#define DMFL_DMAERR	(1<<15) 	/* Dma error bit		*/
#define DMFL_BUFSIZ	512		/* Max. chars. per dma		*/
#define DMFL_DEFCOLS	132		/* Default # of cols/line <=255 */
#define DMFL_DEFLINES	66		/* Default # of lines/page <=255*/

/* Line printer status structure */
struct dmfl_softc {
	unsigned dmfl_state;		/* Soft state bits		*/
	unsigned dmfl_info;		/* Uba info.			*/
	unsigned short dmfl_lines;	/* Lines per page (66 def.)	*/
	unsigned short dmfl_cols;	/* Cols per line (132 def.)	*/
	char dmfl_buf[DMFL_BUFSIZ];
};

/* Driver and data specific structure */
struct	dmf_softc {
	long	sc_flags[8];		/* Flags (one per line) 	*/
	long	sc_category_flags[8];	/* Category flags (one per line)*/
	u_long	sc_softcnt[8];		/* Soft error count total	*/
	u_long	sc_hardcnt[8];		/* Hard error count total	*/
	char	sc_device[DEV_SIZE][8]; /* Device type string		*/
};
