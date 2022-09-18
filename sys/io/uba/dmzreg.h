
/*
 * 	@(#)dmzreg.h	4.1	(ULTRIX)	7/2/90
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
 * dmzreg.h	6.1	   7/29/83
 *
 * Modification history
 *
 * DMZ32 registers/data structures and definitions
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: dmzreg.h	6.1	83/07/29.
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 * 29-Jan-87 - Tim Burke
 *
 *	Added definition of DMZ_NODSR, which will represent the modem signals
 *	carrier detect and clear to send.
 *
 */

/* Register device structure */
struct dmzdevice {
	short	dmzccsr0;		/* Combo csr 0			*/
	short	dmzccsr1;		/* Combo csr 1			*/
	struct octet {
		short	dmzcsr; 	/* Control status register	*/
		short	dmzlpr; 	/* Line parameter register	*/
		short	dmzrbuf;	/* Receiver buffer (ro) 	*/
		union {
			u_short dmzirw; 	/* Indirect reg. word	*/
			u_char	dmzirc[2];	/*    "      "	 bytes	*/
		} dmzun;
	} octets[3];
};

#define dmzrsp	dmzrbuf 		/* Receive silo param. reg.(wo) */
#define dmztbuf dmzun.dmzirc[0] 	/* Rransmit buffer		*/
#define dmztsc	dmzun.dmzirc[0] 	/* Transmit silo count		*/
#define dmzrms	dmzun.dmzirc[1] 	/* Receive modem status 	*/
#define dmzlcr	dmzun.dmzirc[0] 	/* Line control register	*/
#define dmztms	dmzun.dmzirc[1] 	/* Transmit modem status	*/
#define dmztba	dmzun.dmzirw		/* Transmit buffer address	*/
#define dmztcc	dmzun.dmzirw		/* Transmit character count	*/

/* Control status register low definitions (dmzcsr) */
#define DMZ_IAD 	0x1f		/* Indirect address register	*/
#define DMZ_CLR 	0x20		/* Master reset 		*/
#define DMZ_RIE 	0x40		/* Receiver interrupt enable	*/
#define DMZ_RI		0x80		/* Receiver interrupt		*/
#define DMZ_IDENT	0x300		/* dmzccsr0[11:8] == 3 on dmz32 */
#define DMZ_LIN 	0x700		/* Transmit line number 	*/
#define DMZ_NXM 	0x2000		/* Non-existent memory		*/
#define DMZ_TIE 	0x4000		/* Transmit interrupt enable	*/
#define DMZ_TI		0x8000		/* Transmit interrupt		*/
#define DMZ_IE		(DMZ_TIE|DMZ_RIE)	/* Interrupt enable	*/
#define DMZ_SILOCNT	32		/* Size of DMZ output per line	*/

/* Indirect register definitions (dmzun.dmzirw, dmzun.dmzirc) */
#define DMZIR_TBUF	0x00		/* Select tbuf indirect register*/
#define DMZIR_LCR	0x08		/* Select lcr indirect register */
#define DMZIR_TBA	0x10		/* Select tba indirect register */
#define DMZIR_TCC	0x18		/* Select tcc indirect register */

/* Line parameter register definitions (dmzlpr) */
#define BITS5		0x00
#define BITS6		(01<<3)
#define BITS7		(02<<3)
#define BITS8		(03<<3)
#define PENABLE 	0x20
#define EPAR		0x40
#define TWOSB		0x80

/* Receiver buffer register definitions (dmzrbuf) */
#define DMZ_DSC 	0x800		/* Data set change		*/
#define DMZ_PE		0x1000		/* Parity error 		*/
#define DMZ_FE		0x2000		/* Framing error		*/
#define DMZ_DO		0x4000		/* Data overrun 		*/

/* Receive modem status definitions (dmzrms) */
#define DMZ_USRR	0x04		/* User modem signal (pin 25)	*/
#define DMZ_CTS 	0x10		/* Clear to send		*/
#define DMZ_CAR 	0x20		/* Carrier detect		*/
#define DMZ_RNG 	0x40		/* Ring 			*/
#define DMZ_DSR 	0x80		/* Data set ready		*/
#define DMZ_XMIT	(DMZ_CAR|DMZ_CTS|DMZ_DSR)	/* ok to trans. */
#define DMZ_NODSR 	(DMZ_CAR|DMZ_CTS)/* Instead of DMZ_XMIT */

/* Transmit modem status definitions (dmztms) */
#define DMZ_OFF 	0x00		/* Modem control off		*/
#define DMZ_USRW	0x01		/* User modem signal (pin 18)	*/
#define DMZ_DTR 	0x02		/* Data terminal ready		*/
#define DMZ_RATE	0x04		/* Data signal rate select	*/
#define DMZ_RTS 	0x10		/* Request to send		*/
#define DMZ_BRK 	0x20		/* Pseudo break bit		*/
#define DMZ_PREEMPT	0x80		/* Preempt output		*/
#define DMZ_ON		(DMZ_DTR|DMZ_RTS)	/* Modem control on	*/

/* Line control register definitions (dmzlcr) */
#define DMZ_TE		0x01		/* Transmit enable		*/
#define DMZ_AUTOX	0x02		/* Auto XON/XOFF		*/
#define DMZ_RE		0x04		/* Receive enable		*/
#define DMZ_RBRK	0x08		/* Real break bit		*/
#define DMZ_FLUSH	0x10		/* Flush transmit silo		*/
#define DMZ_MIE 	0x20		/* Modem interrupt enable	*/
#define DMZLCR_ENA	(DMZ_MIE|DMZ_RE|DMZ_TE) /* Line control enable	*/

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
	pt->dmzun.dmzirw = (((pt)->dmztms)<<8) | ((exp)&0xff)

/* Driver and data specific structure */
struct	dmz_softc {
	long	sc_flags[24];		/* Flags (one per line) 	*/
	long	sc_category_flags[24];	/* Category flags (one per line)*/
	u_long	sc_softcnt[24]; 	/* Soft error count total	*/
	u_long	sc_hardcnt[24]; 	/* Hard error count total	*/
	char	sc_device[DEV_SIZE][24];/* Device type string		*/
};
