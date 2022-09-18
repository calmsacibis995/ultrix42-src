/*
 * 	@(#)dzreg.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
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
 * dzreg.h     6.2	  3/15/84
 *
 * Modification history
 *
 * DZ11/DZ32/DZV11/DZQ11 registers/data structures and definitions
 *
 * 30-Oct-84 - jrs
 *
 *	Derived from 4.2BSD labeled: dzreg.h   6.2     84/03/15.
 *	Update with defns for newest driver
 *
 * 15-Apr-86 - Tim Burke
 *
 *	Added three defines to dzlpr (BITS5, BITS6, DZ_RE - for termio).
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 */

/* Register device structure */
struct dzdevice {
	short dzcsr;
	short dzrbuf;
	union {
		struct {
			char	dztcr0;
			char	dzdtr0;
			char	dztbuf0;
			char	dzbrk0;
		} dz11;
		struct {
			short	dzlcs0;
			char	dztbuf0;
			char	dzlnen0;
		} dz32;
	} dzun;
};

#define dzlpr	dzrbuf
#define dzmsr	dzun.dz11.dzbrk0
#define dztcr	dzun.dz11.dztcr0
#define dzdtr	dzun.dz11.dzdtr0
#define dztbuf	dzun.dz11.dztbuf0
#define dzlcs	dzun.dz32.dzlcs0
#define dzbrk	dzmsr
#define dzlnen	dzun.dz32.dzlnen0
#define dzmtsr	dzun.dz32.dztbuf0

/* Control status register definitions (dzcsr) */
#define DZ_OFF		0x00		/* Modem control off		*/
#define DZ_32		0x01		/* DZ32 mode			*/
#define DZ_MIE		0x02		/* Modem Interrupt Enable	*/
#define DZ_CLR		0x10		/* Reset dz			*/
#define DZ_MSE		0x20		/* Master Scan Enable		*/
#define DZ_RIE		0x40		/* Receiver Interrupt Enable	*/
#define DZ_MSC		0x800		/* Modem Status Change		*/
#define DZ_SAE		0x1000		/* Silo Alarm Enable		*/
#define DZ_TIE		0x4000		/* Transmit Interrupt Enable	*/
#define DZ_IEN		(DZ_32|DZ_MIE|DZ_MSE|\
			 DZ_RIE|DZ_TIE) 	/* Interrupt enable	*/
#define DZ_ON		DZ_DTR		/* Modem control on		*/

/* Line parameter register definitions (dzlpr) */
#define BITS5		0x00
#define BITS6		0x08
#define BITS7		0x10
#define BITS8		0x18
#define TWOSB		0x20
#define PENABLE 	0x40
#define OPAR		0x80
#define DZ_RE		0x1000		/* Receive enable		*/

/* Receiver buffer register definitions (dzrbuf) */
#define DZ_PE		0x1000
#define DZ_FE		0x2000
#define DZ_DO		0x4000

/* Line control status definitions (dzlcs) */
#define DZ_SR		0x08		/* Secondary Receive		*/
#define DZ_CTS		0x10		/* Clear To Send		*/
#define DZ_CD		0x20		/* Carrier Detect		*/
#define DZ_RI		0x40		/* Ring Indicate		*/
#define DZ_DSR		0x80		/* Data Set Ready		*/
#define DZ_LE		0x100		/* Line Enable			*/
#define DZ_DTR		0x200		/* Data Terminal Ready		*/
#define DZ_BRK		0x400		/* Break			*/
#define DZ_ST		0x800		/* Secondary Transmit		*/
#define DZ_RTS		0x1000		/* Request To Send		*/
#define DZ_ACK		0x8000		/* ACK bit in dzlcs		*/

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

/* Driver and data specific structure */
struct	dz_softc {
	long	sc_flags[8];		/* Flags (one per line) 	*/
	long	sc_category_flags[8];	/* Category flags (one per line)*/
	u_long	sc_softcnt[8];		/* Soft error count total	*/
	u_long	sc_hardcnt[8];		/* Hard error count total	*/
	char	sc_device[DEV_SIZE][8]; /* Device type string		*/
};
