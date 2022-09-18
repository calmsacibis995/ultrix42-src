/*
 * 	@(#)ssreg.h	4.1	(ULTRIX)	7/2/90
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
 *  8-May-89	Giles Atkinson
 *	Pull out major device numbers into cons_maj.h
 *
 * 29-Jan-87 - Tim Burke
 *
 *	Added definition of SS_NODSR, which will represent the modem signals
 *	carrier detect and clear to send.
 *
 * 30-Aug-86  -- fred (Fred Canter)
 *	General cleanup of comments and added #defines for speeds.
 *
 *  5-Aug-86  -- fred (Fred Canter)
 *	Many changes, mostly bug fixes and passing characters to
 *	the bitmap graphics driver (sm.c).
 *
 * 24-Jul-86  -- tim  (Tim Burke)
 *	Real Modem signal definitions as used by ssdtr and ssmsr.
 *
 *  2-Jul-86  -- fred (Fred Canter)
 *	Removed unused defines and general cleanup.
 *
 * 18-Jun-86  -- fred (Fred Canter)
 *	Created this header file for the VAXstar serial line unit driver.
 *	Derived from dzreg.h.
 *
 */

/*
 * Refer to sys/vaxuba/ubareg.h for VAXstar
 * SLU (ss) device register structure.
 */
#define	sscsr	nb_sercsr
#define	ssrbuf	nb_serrbuf_lpr
#define	sslpr	nb_serrbuf_lpr
#define	sstcr	nb_sertcr.c[0]
#define	ssmsr	nb_sermsr_tdr.c[1]
#define	ssdtr	nb_sertcr.c[1]
#define	sstbuf	nb_sermsr_tdr.c[0]
#define	ssbrk	ssmsr

/*
 * VAXstar interrupt controller register bits
 */
#define	SINT_SR	0200			/* Serial line receiver or silo full */
#define	SINT_ST	0100			/* Serial line transmitter done      */

/* Control status register definitions (sscsr) */
#define SS_OFF		0x00		/* Modem control off		*/
#define SS_CLR		0x10		/* Reset ss			*/
#define SS_MSE		0x20		/* Master Scan Enable		*/
#define	SS_RDONE	0x80		/* Receiver done		*/
#define SS_SAE		0x1000		/* Silo Alarm Enable		*/
#define	SS_SA		0x2000		/* Silo Alarm			*/
#define	SS_TRDY		0x8000		/* Transmit ready */
#define SS_ON		SS_DTR		/* Modem control on		*/

/* Line parameter register definitions (sslpr) */
#define BITS5		0x00		/* 5 bit char width		*/
#define BITS6		0x08		/* 6 bit char width		*/
#define BITS7		0x10		/* 7 bit char width		*/
#define BITS8		0x18		/* 8 bit char width		*/
#define TWOSB		0x20		/* two stop bits		*/
#define PENABLE 	0x40		/* parity enable		*/
#define OPAR		0x80		/* odd parity			*/
#define	SS_B4800	0xc00		/* 4800 BPS speed		*/
#define	SS_B9600	0xe00		/* 9600 BPS speed		*/
#define SS_RE		0x1000		/* Receive enable		*/

/* Receiver buffer register definitions (ssrbuf) */
#define SS_PE		0x1000		/* Parity error			*/
#define SS_FE		0x2000		/* Framing error		*/
#define SS_DO		0x4000		/* Data overrun error		*/
#define	SS_DVAL		0x8000		/* Receive buffer data valid	*/

/* Line control status definitions (sslcs) */
#define SS_SR		0x08		/* Secondary Receive		*/
#define SS_CTS		0x10		/* Clear To Send		*/
#define SS_CD		0x20		/* Carrier Detect		*/
#define SS_RI		0x40		/* Ring Indicate		*/
#define SS_DSR		0x80		/* Data Set Ready		*/
#define SS_LE		0x100		/* Line Enable			*/
#define SS_DTR		0x200		/* Data Terminal Ready		*/
#define SS_BRK		0x400		/* Break			*/
#define SS_ST		0x800		/* Secondary Transmit		*/
#define SS_RTS		0x1000		/* Request To Send		*/

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

/* ssdtr bits */
#define	SS_RRTS	0x1		/* REAL request to send bit */
#define SS_RDTR 0x4		/* REAL data terminal ready */

/* ssmsr bits */
#define	SS_RCTS	0x1		/* REAL clear to send bit */
#define	SS_RDSR	0x2		/* REAL data set ready bit */
#define SS_RCD	0x4		/* REAL carrier detect bit  */
#define SS_XMIT (SS_RDSR|SS_RCD|SS_RCTS)	/* Ready to transmit&rec. */
#define SS_NODSR (SS_RCD|SS_RCTS)	/* Instead of SS_XMIT */

/* Driver and data specific structure */
struct	ss_softc {
	long	sc_flags[4];		/* Flags (one per line) 	*/
	long	sc_category_flags[4];	/* Category flags (one per line)*/
	u_long	sc_softcnt[4];		/* Soft error count total	*/
	u_long	sc_hardcnt[4];		/* Hard error count total	*/
	char	sc_device[DEV_SIZE][4]; /* Device type string		*/
};
