/*
 *	@(#)ssc.h	4.2	(ULTRIX)	9/4/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88,89 by			*
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
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:	ssc.h
 *
 *	02-Feb-89 Kong
 *	Only structures related to the System support chip (SSC) are
 *	included here.
 *	Created 2-24-89 by burns, by extracting from cvax.h all ssc
 *	definitions. 
 *
 **********************************************************************/

/*
 * VAX3600 (ka650) System Support Chip (SSC) registers
 * At 0x2014 0000 in Local Register space; mapped to "cvqssc".
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifndef	u_long
#define	u_long	unsigned long
#define u_char	unsigned char
#endif
struct ssc_regs
{
#ifdef __vax
	u_long ssc_sscbr;	/* SSC Base Addr Register	      */
	u_long ssc_pad1[3];	/* filler			      */
	u_long ssc_ssccr;	/* SSC Configuration Register	      */
	u_long ssc_pad2[3];	/* filler			      */
	u_long ssc_cbtcr;	/* CDAL Bus Timeout Control Register  */
	u_long ssc_pad3[3];	/* filler			      */
	u_long ssc_output;	/* Output Port			      */
	u_long ssc_pad4[14];	/* filler			      */
	u_long ssc_toy;		/* time of year clock reg	      */
	u_long ssc_csrs;	/* Console Storage Receiver Status    */
	u_long ssc_csrd;	/* Console Storage Receiver Data      */
	u_long ssc_csts;	/* Console Storage Transmitter Status */
	u_long ssc_cstd;	/* Console Storage Transmitter Data   */
	u_long ssc_crcs;	/* Console Receiver Control/Status    */
	u_long ssc_crdb;	/* Console Receiver Data Buffer	      */
	u_long ssc_ctcs;	/* Console Transmitter Control/Status */
	u_long ssc_ctdb;	/* Console Transmitter Data Buffer    */
	u_long ssc_pad5[19];	/* filler			      */
	u_long ssc_ioreset;	/* I/O System Reset Register	      */
	u_long ssc_pad6[4];	/* filler			      */
	u_long ssc_rdr;		/* Rom Data Register		      */
	u_long ssc_btc;		/* Bus Timeout Counter		      */
	u_long ssc_it;		/* Interval Timer		      */
	u_long ssc_pad7[1];	/* filler			      */
	u_long ssc_tcr0;	/* timer control reg 0		      */
	u_long ssc_tir0;	/* timer interval reg 0		      */
	u_long ssc_tnir0;	/* timer next interval reg 0	      */
	u_long ssc_tivr0;	/* timer interrupt vector reg 0	      */
	u_long ssc_tcr1;	/* timer control reg 1		      */
	u_long ssc_tir1;	/* timer interval reg 1		      */
	u_long ssc_tnir1;	/* timer next interval reg 1	      */
	u_long ssc_tivr1;	/* timer interrupt vector reg 1	      */
	u_long ssc_pad8[4];	/* filler		 	      */
	u_long ssc_adc0match;	/* address decode chan 0 match reg    */
	u_long ssc_adc0mask;	/* address decode chan 0 mask reg     */
	u_long ssc_pad9[2];	/* filler			      */
	u_long ssc_adc1match;	/* address decode chan 1 match reg    */
	u_long ssc_adc1mask;	/* address decode chan 1 mask reg     */
	u_long ssc_pad10[174];	/* pad to 0x20140400 for CPMBX	      */
	u_char ssc_cpmbx;	/* Console Program Mail Box: Lang & Hact   */
	u_char ssc_terminfo;	/* TTY info: Video Dev, MCS, CRT & ROM flags*/
	u_char ssc_keyboard;	/* Keyboard code		      */
	u_char :8;		/* filler			      */
	u_long ssc_pad11[67];	/* filler			      */
	u_long ssc_cca_addr;	/* Physical address of CCA	      */
	u_long ssc_ctsi_addr;	/* Physical address of CTSIA	      */
#endif
#ifdef __mips
volatile u_long ssc_sscbr;	/* SSC Base Addr Register	      */
volatile u_long ssc_pad1[3];	/* filler			      */
volatile u_long ssc_ssccr;	/* SSC Configuration Register	      */
volatile u_long ssc_pad2[3];	/* filler			      */
volatile u_long ssc_cbtcr;	/* CDAL Bus Timeout Control Register  */
volatile u_long ssc_pad3[3];	/* filler			      */
volatile u_long ssc_output;	/* Output Port			      */
volatile u_long ssc_pad4[14];	/* filler			      */
volatile u_long ssc_toy;	/* time of year clock reg	      */
volatile u_long ssc_csrs;	/* Console Storage Receiver Status    */
volatile u_long ssc_csrd;	/* Console Storage Receiver Data      */
volatile u_long ssc_csts;	/* Console Storage Transmitter Status */
volatile u_long ssc_cstd;	/* Console Storage Transmitter Data   */
volatile u_long ssc_crcs;	/* Console Receiver Control/Status    */
volatile u_long ssc_crdb;	/* Console Receiver Data Buffer	      */
volatile u_long ssc_ctcs;	/* Console Transmitter Control/Status */
volatile u_long ssc_ctdb;	/* Console Transmitter Data Buffer    */
volatile u_long ssc_pad5[19];	/* filler			      */
volatile u_long ssc_ioreset;	/* I/O System Reset Register	      */
volatile u_long ssc_pad6[4];	/* filler			      */
volatile u_long ssc_rdr;	/* Rom Data Register		      */
volatile u_long ssc_btc;	/* Bus Timeout Counter		      */
volatile u_long ssc_it;		/* Interval Timer		      */
volatile u_long ssc_pad7[1];	/* filler			      */
volatile u_long ssc_tcr0;	/* timer control reg 0		      */
volatile u_long ssc_tir0;	/* timer interval reg 0		      */
volatile u_long ssc_tnir0;	/* timer next interval reg 0	      */
volatile u_long ssc_tivr0;	/* timer interrupt vector reg 0	      */
volatile u_long ssc_tcr1;	/* timer control reg 1		      */
volatile u_long ssc_tir1;	/* timer interval reg 1		      */
volatile u_long ssc_tnir1;	/* timer next interval reg 1	      */
volatile u_long ssc_tivr1;	/* timer interrupt vector reg 1	      */
volatile u_long ssc_pad8[4];	/* filler		 	      */
volatile u_long ssc_adc0match;	/* address decode chan 0 match reg    */
volatile u_long ssc_adc0mask;	/* address decode chan 0 mask reg     */
volatile u_long ssc_pad9[2];	/* filler			      */
volatile u_long ssc_adc1match;	/* address decode chan 1 match reg    */
volatile u_long ssc_adc1mask;	/* address decode chan 1 mask reg     */
volatile u_long ssc_pad10[174];	/* pad to 0x20140400 for CPMBX	      */
volatile u_char ssc_cpmbx;	/* Console Program Mail Box: Lang & Hact   */
volatile u_char ssc_terminfo;	/* TTY info: Video Dev, MCS, CRT & ROM flags*/
volatile u_char ssc_keyboard;	/* Keyboard code		      */
volatile u_char :8;		/* filler			      */
volatile u_long ssc_pad11[67];	/* filler			      */
volatile u_long ssc_cca_addr;	/* Physical address of CCA	      */
volatile u_long ssc_ctsi_addr;	/* Physical address of CTSIA	      */
#endif /* __mips */
};

/*
 * Some ssc bit definitions.
 */
/* The SSC Configuration register */
#define	SSCCR_IPLMASK	0x03000000	/* IPL level mask		*/
#define	SSCCR_IPL14	0x00000000	/* IPL level 14			*/
#define	SSCCR_IPL15	0x01000000	/* IPL level 15			*/
#define	SSCCR_IPL16	0x02000000	/* IPL level 16			*/
#define	SSCCR_IPL17	0x03000000	/* IPL level 17			*/
#define	SSCCR_CPT	0x00008000	/* Control P detect		*/

#define CVQSSCADDR	((short *)(0x20140000))
#define CVQSSCSIZE	(512*3)

extern struct pte CVQSSCmap[];		/* maps to virtual ssc_regs	      */

/*
 * Stuff for mips system use of SSC chip.
 * These globals need to be set up at run time in the system (cpu) specific
 * initialization code.
 */
#ifdef __mips
extern struct ssc_regs *ssc_ptr;
extern int	ssc_console_timeout;
#define	DEFAULT_SSC_PHYS	0x10140000
#define	DEFAULT_SSC_TIMEOUT	30000
#endif /* __mips */
