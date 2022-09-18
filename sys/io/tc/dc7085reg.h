
/*
 * @(#)dc7085reg.h	4.2	(ULTRIX)	8/9/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 ************************************************************************
 *
 * dc7085reg.h
 *
 * DC7085 SLU console driver
 *
 * Modification history
 *
 *  4-Jul-1990 - Randall P. Brown
 *	Removed definitions for LK201 keyboard.
 *
 *  8-Dec-1989 - Randall P. Brown
 *	Removed the definitions for the tablet buttons.
 *
 * 11-Jul-1988 - Randall P. Brown
 *	Removed the dc_tty struct from the softc struct.  The dc_tty struct
 *	is declared by itself in dc7085.c
 *
 * 16-Dec-1988 - Randall P. Brown
 *	Added the struct for pdma, and included the pdma struct in the
 * 	dc_softc struct.
 *
 * 17-Nov-1988 - Randall P. Brown
 *	Cleaned up file so that names are consistent with spec.
 *
 *  7-Jul-1988 - rsp (Ricky Palmer)
 *	Created file. Contents based on ssreg.h file.
 *
 */

/* Serial line registers */
struct dc_reg {
	u_short DZ_CSR;			/* SLU control status register	*/
	u_short pad1[3];
	union {				/* SLU read buffer/line param.	*/
		u_short DZ_RBUF;
		u_short DZ_LPR;
	} DZ_RBUF_LPR;
	u_short pad2[3];
	u_short DZ_TCR;			/* SLU transmitter control reg. */
	u_short pad3[3];
	union {				/* SLU modem status/txmt reg.	*/
	    u_long	l;		/* on 3max we need to only	*/
	    struct {			/* to full word writes to this	*/
		u_short	w;		/* register due to a R3000 bug	*/
		u_short pad4;
	    } DZ_W;
	} DZ_MSR_TDR;
	u_char 	pad5;
	u_char	DZ_BRK;			/* on 3max the upper 8 bits of 	*/
	                                /* the TDR are accessed at this */
	                                /* address			*/
};

#define dccsr		sc_regs->DZ_CSR
#define dcdtr		sc_regs->DZ_TCR
#define dcrbuf		sc_regs->DZ_RBUF_LPR.DZ_RBUF
#define dclpr		sc_regs->DZ_RBUF_LPR.DZ_LPR
#define dctcr		sc_regs->DZ_TCR
#define dcmsr		sc_regs->DZ_MSR_TDR.DZ_W.w
#define dcbrk_tbuf	sc_regs->DZ_MSR_TDR.l
#define dctbuf		sc_regs->DZ_MSR_TDR.l
#define dcbrk		sc_regs->DZ_BRK

#define CONSOLEMAJOR	0
#define NDCLINE 4


/* Control status register definitions (dccsr) */
#define DC_OFF		0x00		/* Modem control off		*/
#define DC_MAINT	0x08		/* Maintenance			*/
#define DC_CLR		0x10		/* Reset dc7085 chip		*/
#define DC_MSE		0x20		/* Master Scan Enable		*/
#define DC_RIE		0x40		/* Receive IE */
#define DC_RDONE	0x80		/* Receiver done		*/
#define DC_TIE		0x4000		/* Trasmit IE */
#define DC_TRDY		0x8000		/* Transmit ready		*/

/* Line parameter register definitions (dclpr) */
#define BITS5		0x00		/* 5 bit char width		*/
#define BITS6		0x08		/* 6 bit char width		*/
#define BITS7		0x10		/* 7 bit char width		*/
#define BITS8		0x18		/* 8 bit char width		*/
#define TWOSB		0x20		/* two stop bits		*/
#define PENABLE		0x40		/* parity enable		*/
#define OPAR		0x80		/* odd parity			*/
#define DC_B50		0x000		/* 50 BPS speed			*/
#define DC_B75		0x100		/* 75 BPS speed			*/
#define DC_B110		0x200		/* 110 BPS speed		*/
#define DC_B134_5	0x300		/* 134.5 BPS speed		*/
#define DC_B150		0x400		/* 150 BPS speed		*/
#define DC_B300		0x500		/* 300 BPS speed		*/
#define DC_B600		0x600		/* 600 BPS speed		*/
#define DC_B1200	0x700		/* 1200 BPS speed		*/
#define DC_B1800	0x800		/* 1800 BPS speed		*/
#define DC_B2000	0x900		/* 2000 BPS speed		*/
#define DC_B2400	0xa00		/* 2400 BPS speed		*/
#define DC_B3600	0xb00		/* 3600 BPS speed		*/
#define DC_B4800	0xc00		/* 4800 BPS speed		*/
#define DC_B7200	0xd00		/* 7200 BPS speed		*/
#define DC_B9600	0xe00		/* 9600 BPS speed		*/
#define DC_B19200	0xf00		/* 19200 BPS speed		*/
#define DC_B38400	0xf00		/* 38400 BPS speed - see LED2	*/
#define DC_RE		0x1000		/* Receive enable		*/

/* Transmit Control Register (dctcr) */
#define DC_TCR_EN_0	0x1		/* enable transmit on line 0	*/
#define DC_TCR_EN_1	0x2		/* enable transmit on line 1	*/
#define DC_TCR_EN_2	0x4		/* enable transmit on line 2	*/
#define DC_TCR_EN_3	0x8		/* enable transmit on line 3	*/

/* Receiver buffer register definitions (dcrbuf) */
#define DC_PE		0x1000		/* Parity error			*/
#define DC_FE		0x2000		/* Framing error		*/
#define DC_DO		0x4000		/* Data overrun error		*/
#define DC_DVAL		0x8000		/* Receive buffer data valid	*/

/* Line control status definitions (dclcs) */
#define DC_SR		0x08		/* Secondary Receive		*/
#define DC_CTS		0x10		/* Clear To Send		*/
#define DC_CD		0x20		/* Carrier Detect		*/
#define DC_RI		0x40		/* Ring Indicate		*/
#define DC_DSR		0x80		/* Data Set Ready		*/
#define DC_LE		0x100		/* Line Enable			*/
#define DC_DTR		0x200		/* Data Terminal Ready		*/
#define DC_BRK		0x400		/* Break			*/
#define DC_ST		0x800		/* Secondary Transmit		*/
#define DC_RTS		0x1000		/* Request To Send		*/

/* DM lsr definitions */
#define SML_LE		0x01		/* Line enable			*/
#define SML_DTR		0x02		/* Data terminal ready		*/
#define SML_RTS		0x04		/* Request to send		*/
#define SML_ST		0x08		/* Secondary transmit		*/
#define SML_SR		0x10		/* Secondary receive		*/
#define SML_CTS		0x20		/* Clear to send		*/
#define SML_CAR		0x40		/* Carrier detect		*/
#define SML_RNG		0x80		/* Ring				*/
#define SML_DSR		0x100		/* Data set ready, not DM bit	*/

/* Line Prameter Register bits */
#define SER_KBD      000000
#define SER_POINTER  000001
#define SER_COMLINE  000002
#define SER_PRINTER  000003
#define SER_CHARW    000030
#define SER_STOP     000040
#define SER_PARENB   000100
#define SER_ODDPAR   000200
#define SER_SPEED    006000
#define SER_RXENAB   010000

/* Pseudo DMA structure */
struct dcpdma {
    	char *p_mem;
	char *p_end;
};

/* Driver and data specific structure */
struct	dc_softc {
	struct	dcpdma dc_pdma[NDCLINE];/* peudo dma structure		*/
	volatile struct dc_reg *sc_regs; /* PMAX SLU registers		*/
	long	sc_flags[NDCLINE];	/* Flags (one per line)		*/
	long	sc_category_flags[NDCLINE]; /* Category flags (one per line)*/
	u_long	sc_softcnt[NDCLINE];	/* Soft error count total	*/
	u_long	sc_hardcnt[NDCLINE];	/* Hard error count total	*/
	char	sc_device[DEV_SIZE][NDCLINE]; /* Device type string	*/
};

/* Baud rate support status */
struct baud_support {
	u_short	baud_param;	/* How baud rate is represented by device */
	u_char  baud_support;	/* Set if baud rate is supported. */
};
