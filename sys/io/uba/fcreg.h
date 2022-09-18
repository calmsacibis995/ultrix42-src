/*
 * 	@(#)fcreg.h	4.1	(ULTRIX)	7/2/90
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
 *
 * Modification history:
 *
 *  12-15-87	darrell
 *	derrived from ssreg.h
 *
 */

/*
 * Shortened names for fields in the fc_regs structure
 */
#define fcrbuf  fcrbuf_lpr
#define fclpr   fcrbuf_lpr
#define fctcr   fcsertcr.c[0]
#define fcmsr   fcmsr_tdr.c[1]
#define fcdtr   fcsertcr.c[1]
#define fctbuf  fcmsr_tdr.c[0]
#define fcbrk   fcmsr

/*
 * Firefox SLU (fc) device register structure.
 */
struct fc_regs {
	u_short	fccsr;		/* serial line controller CSR */
	u_short	fcpad1;
	u_short	fcrbuf_lpr;		/* SLU read bufffer/line parameter */
	u_short	fcpad2;
	union {
		u_char	c[2];
		u_short	w;
	} fcsertcr;
	u_short	fcpad3;
	union	{
		u_char	c[2];
		u_short	w;
	} fcmsr_tdr;
};

#define	CONSOLEMAJOR	0
#define	FCMAJOR	58

/* Control status register definitions (fccsr) */
#define FC_OFF		0x00		/* Modem control off		*/
#define FC_CLR		0x10		/* Reset fc			*/
#define FC_MSE		0x20		/* Master Scan Enable		*/
#define FC_RIE		0x40		/* Reciever Interrupt Enagle	*/
#define	FC_RDONE	0x80		/* Receiver done		*/
#define FC_SAE		0x1000		/* Silo Alarm Enable		*/
#define	FC_SA		0x2000		/* Silo Alarm			*/
#define FC_TIE		0x4000		/* Transmit Interrupt Enable	*/
#define	FC_TRDY		0x8000		/* Transmit ready		*/
#define FC_ON		FC_DTR		/* Modem control on		*/

/* Line parameter register definitions (fclpr) */
#define BITS5		0x00		/* 5 bit char width		*/
#define BITS6		0x08		/* 6 bit char width		*/
#define BITS7		0x10		/* 7 bit char width		*/
#define BITS8		0x18		/* 8 bit char width		*/
#define TWOSB		0x20		/* two stop bits		*/
#define PENABLE 	0x40		/* parity enable		*/
#define OPAR		0x80		/* odd parity			*/
#define	FC_B4800	0xc00		/* 4800 BPS speed		*/
#define	FC_B9600	0xe00		/* 9600 BPS speed		*/
#define FC_RE		0x1000		/* Receive enable		*/

/* Receiver buffer register definitions (fcrbuf) */
#define FC_PE		0x1000		/* Parity error			*/
#define FC_FE		0x2000		/* Framing error		*/
#define FC_DO		0x4000		/* Data overrun error		*/
#define	FC_DVAL		0x8000		/* Receive buffer data valid	*/

/* Line control status definitions (fclcs) */
#define FC_SR		0x08		/* Secondary Receive		*/
#define FC_CTS		0x10		/* Clear To Send		*/
#define FC_CD		0x20		/* Carrier Detect		*/
#define FC_RI		0x40		/* Ring Indicate		*/
#define FC_DSR		0x80		/* Data Set Ready		*/
#define FC_LE		0x100		/* Line Enable			*/
#define FC_DTR		0x200		/* Data Terminal Ready		*/
#define FC_BRK		0x400		/* Break			*/
#define FC_ST		0x800		/* Secondary Transmit		*/
#define FC_RTS		0x1000		/* Request To Send		*/

/* DM lsr definitions */
#define SML_LE		0x01		/* Line enable			*/
#define SML_DTR 	0x02		/* Data terminal ready		*/
#define SML_RTS 	0x04		/* Request to send		*/
#define SML_ST		0x08		/* Secondary transmit		*/
#define SML_SR		0x10		/* Secondary receive		*/
#define SML_CTS 	0x20		/* Clear to send		*/
#define SML_CAR 	0x40		/* Carrier detect		*/
#define SML_RNG 	0x80		/* Ring 			*/
#define SML_DSR 	0x100		/* Data set ready, not DM bit	*/

/* fcdtr bits */
#define	FC_RRTS	0x1		/* REAL request to send bit */
#define FC_RDTR 0x4		/* REAL data terminal ready */

/* fcmsr bits */
#define	FC_RCTS	0x1		/* REAL clear to send bit */
#define	FC_RDSR	0x2		/* REAL data set ready bit */
#define FC_RCD	0x4		/* REAL carrier detect bit  */
#define FC_XMIT (FC_RDSR|FC_RCD|FC_RCTS)	/* Ready to transmit&rec. */
#define FC_NODSR (FC_RCD|FC_RCTS)	/* Instead of FC_XMIT */

/* Driver and data specific structure */
struct	fc_softc {
	long	sc_flags[4];		/* Flags (one per line) 	*/
	long	sc_category_flags[4];	/* Category flags (one per line)*/
	u_long	sc_softcnt[4];		/* Soft error count total	*/
	u_long	sc_hardcnt[4];		/* Hard error count total	*/
	char	sc_device[DEV_SIZE][4]; /* Device type string		*/
};
