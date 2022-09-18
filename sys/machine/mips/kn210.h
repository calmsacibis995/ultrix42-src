/*
 * @(#)kn210.h	4.2	(ULTRIX)	8/9/90
*/
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Revision History:
 * 16-Jan-89	Kong
 *	Created the file.
 */

/* Program Interval timer bit definition */
#define	TCR_RUN		0x00000001
#define	TCR_XFR		0x00000010
#define	TCR_INT		0x00000080
#define	TCR_ERR		0x80000000
#define	TCR_STP		0x00000004


/* MIPs Kseg 1 address of 10mS timer control/status register */
#define TCSR	PHYS_TO_K1(0x10084010)
#define	TCSR_IE		0x00000040	/* Enable timer interrupts */

/* MIPs Kseg 1 address of error interrupt status register */
#define	ISR	PHYS_TO_K1(0x10084000)
#define	ISR_FPU	0x00000008		/* FPU error interrupt 	*/
#define	ISR_PWF 0x00000004		/* Power fail interrupt	*/
#define	ISR_CERR 0x00000002		/* CQBIC or CMCTL error	*/
#define	ISR_WEAR 0x00000001		/* Write error interrupt*/

/* MIPs Kseg 1 address of interrupt vector read registers */
#define	VRR0	PHYS_TO_K1(0x16000050)	/* IRQ0	*/
#define	VRR1	PHYS_TO_K1(0x16000054)	/* IRQ1	*/
#define	VRR2	PHYS_TO_K1(0x16000058)	/* IRQ2	*/
#define	VRR3	PHYS_TO_K1(0x1600005c)	/* IRQ3	*/

#define	KN210LANCE_ADDR 0x10084400	/* physical addr of lance registers */
#define	KN210SSC_ADDR	0x10140000	/* physical addr of SSC reg set	    */
#define	LANCE_OFFSET	0xd4		/* SCB offset for network interrupts*/
#define	KN210QBUSREG	0x10087800	/* phys addr of Qbus map reg - 0x800*/
#define KN210MSIREG_ADDR 0x10084600	/* physical addr of MSI registers   */
#define	KN210SIIBUF_ADDR 0x10100000	/* physical addr of MSI buffer RAM  */
#define	KN210QMAPBASEREG 0x10080010	/* phys addr of QBus map base reg   */
#define KN210DSER	PHYS_TO_K1(0x10080004)
#define KN210WEAR	PHYS_TO_K1(0x17000000)
#define	KN210QBEAR	PHYS_TO_K1(0x10080008)
#define	KN210DEAR	PHYS_TO_K1(0x1008000c)
#define KN210CBTCR	PHYS_TO_K1(0x10140020)

/* Main memory csrs */
struct memcsr {
	/* memcsr0 through memcsr15 are memory configuration registers */
	unsigned	memcsr0;
	unsigned	memcsr1;
	unsigned	memcsr2;
	unsigned	memcsr3;
	unsigned	memcsr4;
	unsigned	memcsr5;
	unsigned	memcsr6;
	unsigned	memcsr7;
	unsigned	memcsr8;
	unsigned	memcsr9;
	unsigned	memcsr10;
	unsigned	memcsr11;
	unsigned	memcsr12;
	unsigned	memcsr13;
	unsigned	memcsr14;
	unsigned	memcsr15;
	/* memcsr16 is memory error status register */
	unsigned	memcsr16;
	/* memcsr17 is memory control and diagnostic status register */
	unsigned	memcsr17;
};
#define MEMCSR	PHYS_TO_K1(0x10080100)	/* Kseg 1 addr of memory csrs */

/*
 * Bits in memcsr0-15
 */
#define MEM_BNKENBLE	0x80000000	/* <31> Bank Enable */
#define	MEM_BNKUSAGE	0x00000003	/* <1:0> Bank Usage */
