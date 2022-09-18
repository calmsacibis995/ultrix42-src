/*
 * @(#)mtpr.h	4.4  (ULTRIX)        9/6/90    
 */
/************************************************************************
 *									*
 *			Copyright (c) 1983,86,88 by			*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/vax/mtpr.h
 *
 * 4-Sep-90	dlh
 *	added defines for vector processor support
 *
 * 31-Aug-90	paradis
 *	Added VAX9000 registers RXFCT and RXPRM
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added changes for VAX9000.
 *
 * 5-May-89 -- Adrian Thoms
 *    Added two new registers for VVAX [MEMSR, KCALL]
 *
 * 13-Sep-88 -- Tom Kong
 *	Added registers for Rigel VAX6400.
 *
 * 22-Feb-88 -- darrell
 *	Added changes for Firefox CVAX.
 *
 * 15-Feb-88 -- fred (Fred Canter)
 *	Also define MSER IPR for VAX420 (CVAXstar/PVAX).
 *
 * 19-Jan-88 -- jaw
 *	added changes for calypso cvax.
 *
 * 26-jun-26 -- jaw 
 *	removed ifdefs so cpu specific kernel source builds would work.
 *
 * 9-Jul-85 - jrs
 *	Changes for support of the VAX8800 were merged in.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 12-MAR-85 -JAW
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 19 Aug 84 -- rjl
 *	Added MicroVAX-II unique registers
 *
 * 29 Dec 83 -- jmcg
 *	Had to make a comment out of sccsid line.
 *
 * 29 Dec 83 --jmcg
 *	MicroVAX_1 Processor registers are similar enough to 750 and
 *	730 that they are just lumped together in the definitions.
 *
 * 28 Dec 83 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		mtpr.h	6.1	83/07/29
 *
 * ------------------------------------------------------------------------
 */

/*
 * VAX processor register numbers
 */

#define KSP	0x0		/* kernel stack pointer */
#define ESP	0x1		/* exec stack pointer */
#define SSP	0x2		/* supervisor stack pointer */
#define USP	0x3		/* user stack pointer */
#define ISP	0x4		/* interrupt stack pointer */
#define P0BR	0x8		/* p0 base register */
#define P0LR	0x9		/* p0 length register */
#define P1BR	0xa		/* p1 base register */
#define P1LR	0xb		/* p1 length register */
#define SBR	0xc		/* system segment base register */
#define SLR	0xd		/* system segment length register */
#define PCBB	0x10		/* process control block base */
#define SCBB	0x11		/* system control block base */
#define IPL	0x12		/* interrupt priority level */
#define ASTLVL	0x13		/* async. system trap level */
#define SIRR	0x14		/* software interrupt request */
#define SISR	0x15		/* software interrupt summary */
#define ICCS	0x18		/* interval clock control */
#define NICR	0x19		/* next interval count */
#define ICR	0x1a		/* interval count */
#define TODR	0x1b		/* time of year (day) */
#define RXCS	0x20		/* console receiver control and status */
#define RXDB	0x21		/* console receiver data buffer */
#define TXCS	0x22		/* console transmitter control and status */
#define TXDB	0x23		/* console transmitter data buffer */
#define MAPEN	0x38		/* memory management enable */
#define TBIA	0x39		/* translation buffer invalidate all */
#define TBIS	0x3a		/* translation buffer invalidate single */
#define PMR	0x3d		/* performance monitor enable */
#define SID	0x3e		/* system identification */
#define TBCHK	0x3f		/* Translation Buffer Check */

/* VAX780 and VAX8600 */
#define ACCS	0x28		/* accelerator control and status */
#define ACCR	0x29		/* accelerator maintenance */
#define WCSA	0x2c		/* WCS address */
#define WCSD	0x2d		/* WCS data */
#define SBIFS	0x30		/* SBI fault and status */
#define SBIS	0x31		/* SBI silo */
#define SBISC	0x32		/* SBI silo comparator */
#define SBIMT	0x33		/* SBI maintenance */
#define SBIER	0x34		/* SBI error register */
#define SBITA	0x35		/* SBI timeout address */
#define SBIQC	0x36		/* SBI quadword clear */
#define MBRK	0x3c		/* micro-program breakpoint */
/* end 780 and 8600 */

/* VAX9000 */
#define CPUCNF  0x6a            /* CPU configuration reg */
#define ICIR	0x6b            /* Inter CPU interrupt */
#define RXFCT	0x6c		/* console RX function request */
#define RXPRM	0x6d		/* console RX function parameter */
#define TXFCT	0x6e		/* console TX function request */
#define TXPRM	0x6f		/* console TX function parameter */

/* 8600 */
#define PAMACC	0x40		/* PAMM access */
#define PAMLOC	0x41		/* PAMM location */
#define CSWP	0x42		/* Cache sweep (also VAX9000)*/
#define MDECC	0x43		/* MBOX data ecc register */
#define MENA	0x44		/* MBOX error enable register */
#define MDCTL	0x45		/* MBOX data control register */
#define MCCTL	0x46		/* MBOX mcc control register */
#define MERG	0x47		/* MBOX error generator register */
#define CRBT	0x48		/* Console reboot (also VAX9000) */
#define DFI	0x49		/* Diag fault insertion register */
#define EHSR	0x4a		/* Error handling status register */
#define STXCS	0x4c		/* Console block storage C/S */
#define STXDB	0x4d		/* Console block storage D/B */
#define ESPA	0x4e		/* EBOX scratchpad address */
#define ESPD	0x4f		/* EBOX sratchpad data */
/* end VAX8600 */

/* VAX8200 */
#define IPIR	0x16
#define WCSL	0x2e
#define RXCD	0x5c
#define CACHEX	0x5d
#define BINID	0x5e
#define BISTOP	0x5f
#define RXCS1	0x50
#define RXDB1	0x51
#define TXCS1	0x52
#define TXDB1	0x53
#define RXCS2	0x54
#define RXDB2	0x55
#define TXCS2	0x56
#define TXDB2	0x57
#define RXCS3	0x58
#define RXDB3	0x59
#define TXCS3	0x5a
#define TXDB3	0x5b
/* end VAX8200 */

/* VAX8800 */
#define	MCSTS	0x26
#define	NMION	0x80
#define	INOP	0x81
#define	NMIFSR	0x82
#define	NMISILO	0x83
#define	NMIEAR	0x84
#define	CCR	0x85
#define	REVR1	0x86
#define	REVR2	0x87
#define	CLRTOSTS 0x88
/* end 8800 */

/* VAX60 */
#define	CPUID	0xe		/* Unique Hardware ID Register (also VAX9000) */
#define WHAMI	0xf		/* Unique Software ID Register (also VAX9000) */
#define SAVGPR	0x29		/* Scratch register for halt code	      */
/* end VAX60 */

/* VAX750, VAX730, MVAX, VAX8200 and VAX8800 */
#define MCSR	0x17		/* machine check status register */
#define CSRS	0x1c		/* console storage receive status register */
#define CSRD	0x1d		/* console storage receive data register */
#define CSTS	0x1e		/* console storage transmit status register */
#define CSTD	0x1f		/* console storage transmit data register */
#define TBDR	0x24		/* translation buffer disable register */
#define CADR	0x25		/* cache disable register */
#define MCESR	0x26		/* machine check error summary register */
#define CAER	0x27		/* cache error */
#define SAVISP	0x29		/* Console saved stack pointer */
#define SAVPC	0x2a		/* Console saved pc */
#define SAVPSL	0x2b		/* Console saved psl */
#define IUR	0x37		/* init unibus register */
#define TB	0x3b		/* translation buffer */
/* end VAX750, VAX730, MVAX, VAX8200 and VAX8800 */

#define MSER 0x27		/* memory System Error */

/* begin Rigel VAX6400 */
#define	BCBTS	113		/* Backup cache tag store register	*/
#define	BCP1TS	114		/* Backup cache primary tag store reg	*/
#define	BCP2TS	115		/* Backup cache primary tag store reg	*/
#define	BCRFR	116		/* Backup cache refresh register	*/
#define	BCIDX	117		/* Backup cache index register		*/
#define	BCSTS	118		/* Backup cache status register		*/
#define	BCCTL	119		/* Backup cache control register	*/
#define	BCERR	120		/* Backup cache error address register	*/
#define	BCFBTS	121		/* Backup cache flush_bts register	*/
#define	BCFPTS	122		/* Backup cache flush_pts register	*/
#define	VINTSR	123		/* Vector interface error status reg	*/
#define	PCTAG	124		/* P-cache tag array register		*/
#define	PCIDX	125		/* P-cache index register		*/
#define	PCERR	126		/* P-cache error address register	*/
#define	PCSTS	127		/* P-cache status register		*/


/* begin vector processor IPRs */

#define	VPSR	144		/* Vector Processor Status Register	*/
#define	VAER	145		/* Vector Arithmetic Exception Register	*/
#define	VMAC	146		/* Vector Memory Activity Register	*/
#define	VTBIA	147		/* Vector Trans. Buffer Invalidate All	*/
#define	VSAR	148		/* Vector state address register */
#define	VIADR	157		/* Vector Indirect Address Register	*/
#define	VIDLO	158		/* Vector Indirect Data Low Register	*/
#define	VIDHI	159		/* Vector Indirect Data High Register	*/

/* end Rigel VAX6400 */
/* VVAX */
#define MEMSR   0x64            /* physical memory size */
#define KCALL   0x65            /* virtual kernel call */
/* end VVAX */
