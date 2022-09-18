/*
 *	@(#)xvibreg.h	4.2	(ULTRIX)	9/4/90
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
 * Abstract:
 *	This module contains the definitions for the various VMEbus host
 *	modules as well as for the XVIB (the VMEbus card).
 *
 * Revision History
 *
 *      08-Mar-1990     Paul Grist
 *              modified bit definitions for VDCR and VESR registers, up
 *              to date with version 3 of reg spec.
 *
 *	14-Nov-1989	Mark Parenti (map)
 *		Original Version
 */

/*	XVIB Registers	*/

typedef struct _xvib_reg {
	volatile unsigned int	*vdcr;	/* device/configuration reg */
	volatile unsigned int	*vesr;	/* error summary register   */
	volatile unsigned int	*vfadr;	/* failing address register */
	volatile unsigned int	*vicr;	/* interrupt config register */
	volatile unsigned int	*vvor;	/* vector offset register   */
	volatile unsigned int	*vevr;	/* error vector register    */
	volatile unsigned int	*vbsr;	/* byte swap ram access register */
	volatile unsigned int	*vcar;	/* csr access register      */
	volatile unsigned int	(*dma_pmr)[];	/* DMA PMR's		    */

}XVIBREGPTRS;

struct xvib_off_reg {
	unsigned long  	rsvd:16;		/* reserved */
	unsigned long 	data_len:2;		/* VME Data length */
	unsigned long	addr_len:2;		/* VME Address space */
	unsigned long	vme_add:12;		/* VME Address */
};
struct xvib_vcar_reg {
	unsigned long  	reg:6;			/* register select */
	unsigned long 	rsvd:1;			/* reserved	   */
	unsigned long	write:1;		/* register write  */
	unsigned long	rsvd2:6;		/* reserved	   */
	unsigned long	data:18;		/* write data      */
};

typedef struct _xbia_info {
	struct xmidata	*xmidata;	/* Pointer to xmidata structure	*/
};

/*  XVIB Definitions */

/* XVIB Register Offsets		*/


/* XBIA Offsets				*/
#define	VDCR_XBIA_OFF	0x40
#define	VESR_XBIA_OFF	0x44
#define	VFADR_XBIA_OFF	0x48
#define	VICR_XBIA_OFF	0x4C
#define	VVOR_XBIA_OFF	0x50
#define	VEVR_XBIA_OFF	0x54
#define	VBSR_XBIA_OFF	0x58
#define	VCAR_XBIA_OFF	0x5C
#define	DMAPMR_XBIA_OFF	0x200

/* XVIB Register Definitions		*/

/* VDCR - Device/Configuration Register	*/

#define	VDCR_ENAB	0x80000000	/* Enable VMEbus arbitration	*/
#define	VDCR_ARB_MASK	0x60000000	/* Mask for arbitration type	*/
#define	VDCR_ARB_RRS	0x00000000	/* ARB - Round Robin		*/
#define VDCR_ARB_PRI	0x20000000	/* ARB - Priority		*/
#define	VDCR_ARB_PRS	0x40000000	/* ARB - Prioritized Round-Robin */
#define	VDCR_ARB_SGL	0x30000000	/* ARB - Single-Level		*/
#define	VDCR_ERR_SUM	0x10000000	/* Error summary bit */
#define VDCR_PSIZE_MASK	0x0C000000	/* Mask for Page size */
#define	VDCR_PSIZE_4K   0x00000000      /* 4K byte page size */	
#define	VDCR_PSIZE_8K	0x04000000	/* 8K byte page size */
#define VDCR_PSIZE_512  0x08000000	/* 512 byte page size */
#define VDCR_PSIZE_UDEF 0x0C000000      /* undefined page size */
#define	VDCR_BR_MASK	0x03000000	/* Bus Request Level Mask	*/
#define	VDCR_BR_BR0	0x00000000	/* BR - Bus Request Level 0	*/
#define	VDCR_BR_BR1	0x01000000	/* BR - Bus Request Level 1	*/
#define	VDCR_BR_BR2	0x02000000	/* BR - Bus Request Level 2	*/
#define	VDCR_BR_BR3	0x03000000	/* BR - Bus Request Level 3	*/
#define	VDCR_BR_SHIFT	24		/* Shift from vbadata structure	*/
#define	VDCR_CLR_SYSCLK	0x00800000	/* Clear SYSCLK signal  	*/
#define VDCR_RESET_XVIB 0x00400000      /* Reset XVIB			*/
#define	VDCR_ARBTO_SEL	0x00380000	/* Arbitration Timeout Select	*/
#define	VDCR_ARBTO_DIS	0x00000000	/* ARBTO - DISABLED		*/
#define	VDCR_ARBTO_xMS	0x00080000	/* ARBTO - xxx mS		*/
#define	VDCR_ARBTO_yMS	0x00100000	/* ARBTO - yyy mS		*/
#define	VDCR_ARBTO_zMS	0x00180000	/* ARBTO - zzz mS		*/
#define	VDCR_ARBTO_aUS	0x00200000	/* ARBTO - aaa uS		*/
#define	VDCR_ARBTO_bUS	0x00280000	/* ARBTO - bbb uS		*/
#define	VDCR_ARBTO_cUS	0x00300000	/* ARBTO - ccc uS		*/
#define	VDCR_ARBTO_dNS	0x00380000	/* ARBTO - ddd nS		*/
#define	VDCR_TRNTO_SEL 0x00070000	/* Transaction Timeout Select	*/
#define	VDCR_TRNTO_DIS	0x00000000	/* TRNTO - DISABLED		*/
#define	VDCR_TRNTO_xMS	0x00010000	/* TRNTO - xxx mS		*/
#define	VDCR_TRNTO_yMS	0x00020000	/* TRNTO - yyy mS		*/
#define	VDCR_TRNTO_zMS	0x00030000	/* TRNTO - zzz mS		*/
#define	VDCR_TRNTO_aUS	0x00040000	/* TRNTO - aaa uS		*/
#define	VDCR_TRNTO_bUS	0x00050000	/* TRNTO - bbb uS		*/
#define	VDCR_TRNTO_cUS	0x00060000	/* TRNTO - ccc uS		*/
#define	VDCR_TRNTO_dNS	0x00070000	/* TRNTO - ddd nS		*/
#define	VDCR_SERCLK_SEL	0x0000C000	/* SERCLK Period Select		*/
#define	VDCR_SERCLK_32	0x00000000	/* SERCLK - 32MHz		*/
#define	VDCR_SERCLK_16	0x00004000	/* SERCLK - 16MHz		*/
#define	VDCR_SERCLK_08	0x00008000	/* SERCLK - 8MHz		*/
#define	VDCR_SERCLK_04	0x0000C000	/* SERCLK - 4MHz		*/
#define	VDCR_IPL_SEL	0x00003000	/* Interrupt Priority Select	*/
#define	VDCR_IPL_14	0x00000000	/* IPL - ipl 14 (BR4)		*/
#define	VDCR_IPL_15	0x00001000	/* IPL - ipl 15 (BR5)		*/
#define	VDCR_IPL_16	0x00002000	/* IPL - ipl 16 (BR6)		*/
#define	VDCR_IPL_17	0x00003000	/* IPL - ipl 17 (BR7)		*/
#define	VDCR_DREV	0x00000F00	/* XVIB Device Revision		*/
#define	VDCR_DTYPE	0x000000FF	/* XVIB Device Type		*/
#define	VDCR_XVIB_DTYPE	0x000000E8	/* Device Type for XVIB		*/

/* VESR - Error Summary Register	*/

#define	VESR_SRAM_PE	0x80000000	/* Swap Ram Parity Error	*/
#define	VESR_BERR	0x40000000	/* VME *BERR signal		*/
#define	VESR_INTERLOCK	0x20000000	/* Interlock error		*/
#define	VESR_RMWII	0x10000000	/* Read Modify Write Error II	*/
#define	VESR_RMW	0x08000000	/* Read-Modify-Write Error 	*/
#define	VESR_TPE	0x04000000	/* Transmit Parity Error	*/
#define	VESR_ITPE	0x02000000	/* IBUS Tranmit Parity Error	*/
#define	VESR_IRPE	0x01000000	/* IBUS Receive Parity Error	*/
#define	VESR_TRNTO	0x00800000	/* VME Transaction Timeout	*/
#define	VESR_ARBTO	0x00400000	/* VME Arbitration Timeout	*/
#define	VESR_BGL_MASK	0x00300000	/* Bus Grant Level during timeout */
#define	VESR_AM_TO	0x000FC000	/* Address Modifiers during timeout */
#define	VESR_DS1_TO	0x00002000	/* VME DS1 signal during timeout */
#define	VESR_DS0_TO	0x00001000	/* VME DS0 signal during timeout */
#define VESR_WRITE      0x00000800      /* VME WRITE* signal value       */
#define	VESR_IRQ_MASK	0x000007F0	/* IRQn Interrupt Pending	 */
#define	VESR_BR_IS	0x0000000F	/* BRn Interrupt Sent		 */


/* VFADR - VME Failing Address Register	*/

#define	VFADR_LWORD	0x00000001	/* VME LWORD* signal during timeout */

/* VICR	- XVIB Interrupt Configuration Register */

#define	VICR_IRQ_MASK	0xF7000000	/* Interrupt Request Level Mask	*/
#define	VICR_IRQ_SHIFT	25		/* Shift from vbadata		*/
#define	VICR_ENAB_TRNPE	0x00800000	/* Enable VME Transmit Parity Error */
#define	VICR_ENAB_IBTPE	0x00400000	/* Enable IBUS Transmit Parity Error */
#define	VICR_ENAB_IBRPE	0x00200000	/* Enable IBUS Receive Parity Error */
#define	VICR_ENAB_ARBTO	0x00100000	/* Enable Arbitration Timeout Int   */
#define	VICR_ENAB_TRNTO	0x00080000	/* Enable VME Transaction Timeout Int */
#define	VICR_ENAB_INTLK	0x00040000	/* Enable Interlock Error Interrupt */
#define	VICR_ENAB_RMW	0x00020000	/* Enable RMW Error Interrupt	    */
#define	VICR_IRQ7_SEL	0x00018000	/* IRQ7 ipl translation (RORA)	    */
#define	VICR_IRQ7_14	0x00000000	/* IRQ7 == IPL 14 (Default)         */
#define	VICR_IRQ7_15	0x00008000	/* IRQ7 == IPL 15		    */
#define	VICR_IRQ7_16	0x00010000	/* IRQ7 == IPL 16		    */
#define	VICR_IRQ7_17	0x00018000	/* IRQ7 == IPL 17		    */
#define	VICR_IRQ6_SEL	0x00006000	/* IRQ6 ipl translation	(ROAK)	    */
#define	VICR_IRQ6_14	0x00000000	/* IRQ6 == IPL 14 (Default)         */
#define	VICR_IRQ6_15	0x00002000	/* IRQ6 == IPL 15		    */
#define	VICR_IRQ6_16	0x00004000	/* IRQ6 == IPL 16		    */
#define	VICR_IRQ6_17	0x00006000	/* IRQ6 == IPL 17		    */
#define	VICR_IRQ5_SEL	0x00001C00	/* IRQ5 ipl translation		    */
#define	VICR_IRQ5_14_AK	0x00000000	/* IRQ5 == IPL 14 ROAK (Default)    */
#define	VICR_IRQ5_14_RA	0x00000400	/* IRQ5 == IPL 14 RORA		    */
#define	VICR_IRQ5_15_AK 0x00000800	/* IRQ5 == IPL 15 ROAK   	    */
#define	VICR_IRQ5_15_RA	0x00000C00	/* IRQ5 == IPL 15 RORA		    */
#define	VICR_IRQ5_16_AK	0x00001000	/* IRQ5 == IPL 16 ROAK		    */
#define	VICR_IRQ5_16_RA	0x00001400	/* IRQ5 == IPL 16 RORA		    */
#define	VICR_IRQ5_17_AK	0x00001800	/* IRQ5 == IPL 17 ROAK		    */
#define	VICR_IRQ5_17_RA	0x00001C00	/* IRQ5 == IPL 17 RORA		    */
#define	VICR_IRQ4_SEL	0x00000380	/* IRQ4 ipl translation		    */
#define	VICR_IRQ4_14_AK	0x00000000	/* IRQ4 == IPL 14 ROAK (Default)    */
#define	VICR_IRQ4_14_RA	0x00000080	/* IRQ4 == IPL 14 RORA		    */
#define	VICR_IRQ4_15_AK	0x00000100	/* IRQ4 == IPL 15 ROAK   	    */
#define	VICR_IRQ4_15_RA	0x00000180	/* IRQ4 == IPL 15 RORA		    */
#define	VICR_IRQ4_16_AK	0x00000200	/* IRQ4 == IPL 16 ROAK		    */
#define	VICR_IRQ4_16_RA	0x00000280	/* IRQ4 == IPL 16 RORA		    */
#define	VICR_IRQ4_17_AK	0x00000300	/* IRQ4 == IPL 17 ROAK		    */
#define	VICR_IRQ4_17_RA	0x00000380	/* IRQ4 == IPL 17 RORA		    */
#define	VICR_IRQ3_SEL	0x00000070	/* IRQ3 ipl translation		    */
#define	VICR_IRQ3_14_AK	0x00000000	/* IRQ3 == IPL 14 ROAK (Default)    */
#define	VICR_IRQ3_14_RA	0x00000010	/* IRQ3 == IPL 14 RORA		    */
#define	VICR_IRQ3_15_AK	0x00000020	/* IRQ3 == IPL 15 ROAK   	    */
#define	VICR_IRQ3_15_RA	0x00000030	/* IRQ3 == IPL 15 RORA		    */
#define	VICR_IRQ3_16_AK	0x00000040	/* IRQ3 == IPL 16 ROAK		    */
#define	VICR_IRQ3_16_RA	0x00000050	/* IRQ3 == IPL 16 RORA		    */
#define	VICR_IRQ3_17_AK	0x00000060	/* IRQ3 == IPL 17 ROAK		    */
#define	VICR_IRQ3_17_RA	0x00000070	/* IRQ3 == IPL 17 RORA		    */
#define	VICR_IRQ2_SEL	0x0000000C	/* IRQ2 ipl translation (RORA)	    */
#define	VICR_IRQ2_14	0x00000000	/* IRQ2 == IPL 14 (Default)         */
#define	VICR_IRQ2_15	0x00000004	/* IRQ2 == IPL 15		    */
#define	VICR_IRQ2_16	0x00000008	/* IRQ2 == IPL 16		    */
#define	VICR_IRQ2_17	0x0000000C	/* IRQ2 == IPL 17		    */
#define	VICR_IRQ1_SEL	0x00000003	/* IRQ1 ipl translation	(ROAK)	    */
#define	VICR_IRQ1_14	0x00000000	/* IRQ1 == IPL 14 (Default)         */
#define	VICR_IRQ1_15	0x00000001      /* IRQ1 == IPL 15		    */
#define	VICR_IRQ1_16	0x00000002	/* IRQ1 == IPL 16		    */
#define	VICR_IRQ1_17	0x00000003	/* IRQ1 == IPL 17		    */

#define	VICR_INIT_SET	0x00F70000	/* Bits to set upon reset	    */
/* VVOR - XVIB Vector Offset Register	*/

#define	VVOR_INT_DEST	0xFFFF0000	/* VME Interrupt Destination Mask   */
#define	VVOR_IV_OFFSET	0x0000FF00	/* Interrupt Vector Offset	    */

/* VEVR - XVIB Error Vector Register		*/

#define	VEVR_INT_DEST	0xFFFF0000	/* VME Error Int Destination Mask   */
#define	VEVR_INT_VECT	0x0000FFFC	/* VME Error Interrupt Vector	    */ 

/* VBSR - XVIB Byte Swap Ram Register		*/

#define	VBSR_WRITE	0x00000010	/* Enable WRITE to register	    */

/* VCAR - XVIB CSR Access Register		*/

#define	VCAR_WRT_DATA	0xFFFFC000	/* CSR Write Data		    */
#define	VCAR_WRITE	0x00000080	/* Enable WRITE to register	    */
#define	VCAR_REG_SEL	0x0000003F	/* Mask for register select	    */

/* VCAR Register Select Codes			*/
#define	VCAR_PMR_OFF	0x20		/* Offset to PMR[0]		    */
#define	VCAR_ADD_SEL	0x5		/* Address space select register   */

/* Address Space Select Register		*/
#define	ADSEL_A32	0x04000000	/* Enable A32 space		   */
#define	ADSEL_A24	0x02000000	/* Enable A24 space		   */
#define	ADSEL_A16	0x01000000	/* Enable A16 space		   */
#define	ADSEL_ADD_ENAB	(ADSEL_A32 | ADSEL_A24)	/* Enable A32 and A24      */
                                        /* spaces for host DMA		   */
/* Offset register definitions	*/
#define XVIB_PIO_A16		0x8000		/* A16 space	*/
#define XVIB_PIO_A24		0xC000		/* A24 space	*/
#define XVIB_PIO_A32		0x0000		/* A32 space	*/
#define	XVIB_PIO_DL_SHIFT	16		/* Shift Data Length Field */

/* PMR - PIO Page Map Register			*/
#define	XVIB_PIO_MASK	0xFFF00000	/* High order bits for VME address */
#define	XVIB_PIO_OFFSET	0x000FFFFF	/* Offset into 1MB PMR "page"	   */
#define	XVIB_PIO_REGSHFT 20		/* Shift count for PMR		   */

/* General Definitions	*/
#define	XVIB_NPIOPMR	32		/* Number of PIO PMR's		   */

/* Macros		*/

#define	XVIB_STORE_PMR(reg, value) \
	*Xvibregs.vcar = VCAR_WRITE | (value)  | \
	( ((reg) + VCAR_PMR_OFF) & VCAR_REG_SEL);

#define	XVIB_READ_PMR(reg, staddr) \
        *Xvibregs.vcar = VCAR_WRITE |  \
	( ((reg) + VCAR_PMR_OFF) & VCAR_REG_SEL); \
        (staddr) = *Xvibregs.vcar;












