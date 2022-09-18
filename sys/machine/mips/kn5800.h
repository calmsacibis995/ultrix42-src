/* 	@(#)kn5800.h	4.2	(ULTRIX)	9/4/90 	*/
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
 * 24-Mar-89	Bill Burns
 *	Created the file.
 */


#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * ISIS interupt vector registers. A read of these gets us a vector.
 */

#define	KN5800_VEC_REG_BASE	0x40000000
extern	int	kn5800_vectors[];

#define	VEC_LEVEL0	0x50
#define	VEC_LEVEL1	0x54
#define	VEC_LEVEL2	0x58
#define	VEC_LEVEL3	0x5c



#define	XMI_START_PHYS 	0x11800000
#define KN5800_MAX_K1_IO_WINDOWS	4
#define XMI_MAPPED_WINDOW_BASE		0x36000000
#define	XMI_WINDOW_SIZE			0x02000000
#define XMI_MAPPED_WINDOWS		4

struct v5800csr {
	volatile long 	csr1;
};

/*
 * CSR1 bit definitions
 */
#define	XMI_NODE_ID	0x0000000f	/* XMI node id			*/
#define	BOOT_DISABLE	0x00000010	/* Boot disable			*/
#define EEPROMEN	0x00000020	/* Eeprom update enable		*/
#define XMI_ACLO	0x00000040	/* XMI ac low			*/
#define STLOOP		0x00000080	/* Self test loop		*/
#define EEPROMADDR	0x00000300	/* Eeprom write address bits	*/
#define DLCKOUTEN	0x00000400	/* Delayed lockout enable	*/
#define A_STPASS	0x00000800	/* X3PA Self test passed	*/
#define LED_D6		0x00001000	/* Led d6			*/
#define TIMOTE		0x00002000	/* Timeout enable		*/
#define B_STPASS	0x00004000	/* XP3B Self test passed	*/
#define EINTMR		0x00008000	/* Enable interval timer	*/
#define RINVAL		0x00010000	/* Reset invalidate fifo	*/
#define FHIT		0x00020000	/* Force hit			*/
#define FMISS		0x00040000	/* Force miss			*/
#define FBTP		0x00080000	/* Force bad tag parity		*/
#define FCI		0x00100000	/* Force cache invalidate	*/
#define CPUD		0x00200000	/* Force cache parity update disable */
#define FPSEL		0x00400000	/* Force parity select		*/
#define FCACHEEN	0x00800000	/* Force cache enable		*/
#define R3000E		0x01000000	/* R3000 enable			*/
#define CRS1_BIT_25	0x02000000	/* Reserved			*/
#define IFIFOFL		0x04000000	/* Invalidate fifo full		*/
#define TIMOT		0x08000000	/* X3PB timeout			*/
#define INTR1		0x10000000	/* Interrupt level 1		*/
#define INTMR		0x20000000	/* Interval timer interrupt	*/
#define LATHIT		0x40000000	/* 2nd level cache hit status 	*/
#define INSECURE	0x80000000	/* Console not secure		*/
#define	CSR1_SAVE_MASK	0x0001ffff


/*
 * X3PA Registers (XCP registers)
 */
struct xcp_reg {
	unsigned int xcp_dtype;
	unsigned int xcp_xbe;
	unsigned int xcp_fadr;
	unsigned int xcp_gpr;
	unsigned int xcp_csr2;
#ifdef __vax
	char	xcp_pad[1004];
#endif /* __vax */
#ifdef __mips
	char	xcp_pad[524268];
#endif /* __mips */
};


/* definitions for CSR2 bits */
#define	CSR2_VBPE	0x80000000
#define	CSR2_TPE 	0x40000000
#define	CSR2_IQO 	0x20000000
#define	CSR2_WDPE	0x10000000
#define	CSR2_CFE	0x08000000
#define	CSR2_DTPE	0x04000000
#define	CSR2_LOCKOUT	0x00600000
#define CSR2_ERRORS	0xff000000

/* definitions for XBE bits */
#define XBE_STF		0x400		/* self test fail */
#define XBE_ETF		0x800		/* extended test fail */
#define XBE_TTO		0x2000		/* transition timeout */
#define XBE_TE		0x4000		/* transmit error */
#define XBE_CNAK	0x8000		/* command noack */
#define XBE_RER		0x10000		/* read error response */
#define XBE_RSE		0x20000		/* read sequence error */
#define XBE_NRR		0x40000		/* no read response */
#define XBE_CRD		0x80000		/* corrected read data */
#define XBE_WDNAK	0x100000	/* write data noack */
#define XBE_RIDNAK	0x200000	/* read/ident data noack */
#define XBE_WSE		0x400000	/* write sequence error */
#define XBE_PE		0x800000	/* parity error */
#define XBE_IPE		0x1000000	/* inconsistent parity */
#define XBE_WEI		0x2000000	/* write error interrupt */
#define XBE_XFAULT	0x4000000	/* xmi fault */
#define XBE_CC		0x8000000	/* corrected confirmation */
#define XBE_XBAD	0x10000000	/* xmi bad */
#define XBE_NHALT	0x20000000	/* node halt */
#define XBE_NRST	0x40000000	/* node reset */
#define XBE_ES		0x80000000	/* error summary */
#define XBE_FATAL_BITS	(XBE_WDNAK | XBE_TE | XBE_CNAK | XBE_WEI | XBE_XFAULT)


/*
 * Structure to capture external processor registers
 * One per cpu gets allocated by master at startup.
 */
struct kn5800_regs {
	u_long	kn5800_csr1;
	u_long	kn5800_dtype;
	u_long	kn5800_xbe;
	u_long	kn5800_fadr;
	u_long	kn5800_gpr;
	u_long	kn5800_csr2;
};
extern struct kn5800_regs *kn5800_regp;

/*
 * Write buffer flush for DECsystem 58xx
 * Used by kn5800_wbflush and wherever else speed is of the utmost importance.
 * wbflush_dummy is used as the destination for the required i/o space read
 * to avoid the possability of the compiler optimizing it out.
 */
#define	KN5800_WBFLUSH()	(wbflush_dummy = *kn5800_wbflush_addr)
extern int	wbflush_dummy;	/* external -  to prevent compiler from
				   getting too smart */

#define CSR5800 	0xb0000000
#define	IAR_REGISTER	(volatile int *)0xb0130000
#define	IDR_REGISTER	(volatile int *)0xb0110000
char *kn5800_ip[16];

