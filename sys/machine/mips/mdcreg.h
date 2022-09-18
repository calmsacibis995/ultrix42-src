/*
 *      @(#)mdcreg.h	4.2     (ULTRIX)        7/17/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * mdzreg.h
 *
 * Mipsmate DC7085 SLU console driver
 *
 *	This driver controls the 3 dz chips of the mipsmate.  Each DC7085 chip
 *	facilitates four asynchronous terminal lines.  The first dz (dz0)
 *	chip has the console on line 0 and has full modem control on line 2.
 *	The second and third dz chips (dz1 & dz2) are an expansion option.
 *	Line 2 of dz1 has full modem control.
 *
 * Modification history
 *
 *  Jul 6, 90	Kuo-Hsiung Hsieh
 * 	Added third field delay_set_break in buad_support struct to
 *	hold delay interval before setting a break condition in a
 *	transmission line.
 *
 *  May 30, 90 	Kuo-Hsiung Hsieh
 *	Corrected the address of break control due to spec's error.
 *
 *  March 22, 1990	Tim Burke
 *	Created file. Contents based on dc7085.h file.  Due to time constraints
 *	a separate copy of this driver is being developed instead of modifying
 *	the original source to accomodate all three device types.  The main
 *	changes from the dc7085 sources is to remove all references to graphic
 *	capabilities.  This driver can accomodate up to 3 DC7085 chips while
 *	the original driver could only have 1 such chip.
 *
 */

/* 
 * Serial line registers 
 * 	Each DC7085 is made up of the following registers.  Be careful not to
 *	access DZ_MSR in dz2 because is is non-existent!
 *
 *	This definition differs from the original version in that the transmit
 *	data register is split up to be byte accessable.  This is being done
 *	to allow setting and clearing of the break bits independently of the
 *	transmitted character.
 */
struct mdz_reg {
	u_short DZ_CSR;			/* SLU control status register	*/
	u_short pad1[3];
	union {				/* SLU read buffer/line param.	*/
		u_short DZ_RBUF;	/* Read only			*/
		u_short DZ_LPR;		/* Write only			*/
	} DZ_RBUF_LPR;
	u_short pad2[3];
	u_short DZ_TCR;			/* SLU transmitter control reg. */
	u_short pad3[3];
	union {				/* SLU modem status/txmt reg.	*/
		u_short	DZ_MSR;		/* Read only, modem status	*/
					/* TDR Transmit Data Register	*/
		struct {		/* Write Only			*/
		    u_char DZ_TBUF;	/* Bottom half of TDR		*/
		    u_char pad4[4];	/* padding 			*/
		    u_char DZ_BRK;	/* Top half of TDR, Due to the  */
					/* address multiplex, High byte */ 
		} DZ_TDR; 		/* is addressed 0x1c00001d	*/
	} DZ_MSR_TDR;
};

/*
 * These macros are used to make register access easier.  They point to
 * a particular device register and in some cases they represent the upper
 * or lower half of a device register.  This assumes that byte writes are
 * allowable.
 */
#define dccsr		DZ_CSR			  /* Chip Status 	   */
#define dcrbuf		DZ_RBUF_LPR.DZ_RBUF	  /* Received byte	   */
#define dclpr		DZ_RBUF_LPR.DZ_LPR	  /* Line attributes	   */
#define dctcr		DZ_TCR			  /* Transmitter control   */
#define dcmsr		DZ_MSR_TDR.DZ_MSR	  /* Modem status	   */
#define dctbuf		DZ_MSR_TDR.DZ_TDR.DZ_TBUF /* Transmit byte 	   */
#define dcbreak         DZ_MSR_TDR.DZ_TDR.DZ_BRK  /* Break bits            */

#define CONSOLEMAJOR	0
#define NDCLINE 4 			/* Each DC7085 supports 4 async lines */
					/* Tim - should this be configurable? */
#define NDC 3				/* Assume 3 DC7085's ???	*/
#define MAX_NDC 3			/* Assume 3 DC7085's at most	*/
#define NDCTTYS (NDCLINE * NDC)		/* 12 terminal lines total	*/
#define CONSOLE_UNIT 0          	/* The console is on DC0           */
#define CONSOLE_LINE 0          	/* The console is on line 0 of DC0 */


/* Control status register definitions (dccsr) */
#define DC_MAINT	0x08		/* Maintenance			*/
#define DC_CLR		0x10		/* Reset dc7085 chip		*/
#define DC_MSE		0x20		/* Master Scan Enable		*/
#define DC_RIE		0x40		/* Receive IE */
#define DC_RDONE	0x80		/* Receiver done		*/
#define DC_TIE		0x4000		/* Trasmit IE */
#define DC_TRDY		0x8000		/* Transmit ready		*/

/* Line parameter register definitions (dclpr) */
#define DC_LINE0	0x00		/* Params apply to line 0	*/
#define DC_LINE1	0x01		/* Params apply to line 1	*/
#define DC_LINE2	0x02		/* Params apply to line 2	*/
#define DC_LINE3	0x03		/* Params apply to line 3	*/
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

/* Receiver buffer register definitions (dcrbuf) */
#define DC_PE		0x1000		/* Parity error			*/
#define DC_FE		0x2000		/* Framing error		*/
#define DC_DO		0x4000		/* Data overrun error		*/
#define DC_DVAL		0x8000		/* Receive buffer data valid	*/

/* Modem status register definitions (dcmsr) */
#define DC_CTS		0x1		/* Clear To Send		*/
#define DC_DSR		0x2		/* Data Set Ready		*/
#define DC_CD		0x4		/* Carrier Detect		*/
#define DC_RI		0x8		/* Ring Indicate		*/
#define DC_SI		0x80		/* Speed Indicate		*/
#define DC_XMIT		(DC_CTS|DC_DSR|DC_CD)	/* All modem leads ready*/

/* dctcr bits */
#define DC_RSS  0x200			/* REAL speed select		*/
#define DC_RDTR 0x400			/* REAL data terminal ready	*/
#define DC_RRTS 0x800			/* REAL request to send		*/

/* DM lsr definitions, used for the dctodm nonsense */
#define SML_LE		0x01		/* Line enable			*/
#define SML_DTR		0x02		/* Data terminal ready		*/
#define SML_RTS		0x04		/* Request to send		*/
#define SML_ST		0x08		/* Secondary transmit		*/
#define SML_SR		0x10		/* Secondary receive		*/
#define SML_CTS		0x20		/* Clear to send		*/
#define SML_CAR		0x40		/* Carrier detect		*/
#define SML_RNG		0x80		/* Ring				*/
#define SML_DSR		0x100		/* Data set ready, not DM bit	*/

/* Pseudo DMA structure */
struct dcpdma {
    	char *p_mem;
	char *p_end;
};
struct	dcpdma mdc_pdma[NDC][NDCTTYS];

/* Driver and data specific structure */
struct	dc_softc {
	u_long	sc_softcnt[NDCLINE];	/* Soft error count total	*/
};
struct dc_softc mdc_softc[NDC];

/*
 * The following gives the base address of where the registers for each of
 * the DC chips reside.
 */
#define DC0_BASE_ADDR (struct mdz_reg *)(PHYS_TO_K1(0x1c000000))
#define DC1_BASE_ADDR (struct mdz_reg *)(PHYS_TO_K1(0x15000000))
#define DC2_BASE_ADDR (struct mdz_reg *)(PHYS_TO_K1(0x15200000))
/*
 * Access to the DC device registers will be of the form:
 * dcaddr = mdz_regs[unit];
 * dcaddr->csr |= IE;
 */
volatile struct mdz_reg *mdz_regs[] = {
		DC0_BASE_ADDR,
		DC1_BASE_ADDR,
		DC2_BASE_ADDR,
	};

/* Baud rate support status */
struct baud_support {
	u_short	baud_param;	/* How baud rate is represented by device */
	u_char  baud_support;	/* Set if baud rate is supported. */
	u_char	delay_set_break; /* the delay factor used in timeout */
};
