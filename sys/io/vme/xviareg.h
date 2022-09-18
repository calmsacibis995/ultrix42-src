/*
 *	@(#)xviareg.h	4.5	(ULTRIX)	2/21/91
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
 *	This module contains the definitions for the 3VIA module
 *
 * Revision History
 *
 *	19-Feb-1991	Mark Parenti (map)
 *		Add define for board revision level.
 *
 *	22-Jan-1991	Mark Parenti (map)
 *		Fix defines for error bit resets.
 *		Add defines for address space selection.
 *		Add define for DMA PMR address space enable.
 *
 *	18-Dec-1990	Mark Parenti (map)
 *		Add new registers.
 *		Remove MVIA support.
 *
 *	12-Oct-1990	Mark Parenti (map)
 *		Use native hardware register sizes when accessing the
 *		registers.
 *		Add some additional VIC registers needed for initialization.
 *
 *	08-Mar-1990	Mark Parenti (map)
 *		Add defines for mapping.
 *
 *      27-Feb-1990     Paul Grist
 *              Added some error masks needed by error handlers
 *
 *	14-Nov-1989	Mark Parenti (map)
 *		Original Version
 */

/*	3VIA Registers	*/

typedef struct _xvia_reg {
	volatile unsigned int	*csr;	/* device/configuration reg */
	volatile unsigned int	*vfadr; /* vme failing address register */
	volatile unsigned int	*cfadr; /* cpu failing address register */
	volatile unsigned int	*ior;	/* interrupt offset register	*/
	volatile unsigned char  *besr;  /* bus error status register */
	volatile unsigned char	*icr;	/* interrupt control registers 	*/
	volatile unsigned char	*errgi; /* error group int ctrl reg 	*/
	volatile unsigned char	*lvb;	/* local vector base register  	*/
	volatile unsigned char	*err;	/* error vector register    	*/
	volatile unsigned int	*viacsr; /*xVIA command/status register */
	volatile unsigned int	*viaclr; /*xVIA status/clear register */
	volatile unsigned short	*ivs;	/* interrupt vector source	*/
	volatile unsigned char	*arcr;	/* arbiter/requester config register */
	volatile unsigned char	*ttr;	/* transfer timeout register    */
	volatile unsigned char	*rcr;	/* release control register	*/
	volatile unsigned char	*licr;	/* local intr control registers	*/
	volatile unsigned char	*lbtr;	/* local bus timing register 	*/
	volatile unsigned char	*icfr;	/* interface config register	*/
	volatile unsigned char	*amsr;	/* address modifier source reg 	*/
	volatile unsigned char  *s0c0;  /* slave sel 0 control reg 0	*/
	volatile unsigned char  *s0c1;  /* slave sel 0 control reg 1	*/
	volatile unsigned char  *s1c0;  /* slave sel 1 control reg 0	*/
	volatile unsigned char  *s1c1;  /* slave sel 1 control reg 1	*/
	volatile unsigned int	(*pio_pmr)[];	/* PIO PMR's		    */
	volatile unsigned int	(*dma_pmr)[];	/* DMA PMR's		    */
}XVIAREGPTRS;

/* XVIA Register Offsets		*/

/* 3VIA Offsets				*/
#define	CSR_XVIA_OFF	0x00040000
#define	VFADR_XVIA_OFF	0x00040004
#define	CFADR_XVIA_OFF	0x00040008
#define	IOR_XVIA_OFF	0x0004000C
#define	BESR_XVIA_OFF	0x000440B8
#define	ICR_XVIA_OFF	0x00044004
#define	ERRGI_XVIA_OFF	0x00044048
#define	LVB_XVIA_OFF	0x00044054
#define	ERR_XVIA_OFF	0x00044058
#define	VIACSR_XVIA_OFF	0x00049000
#define VIACLR_XVIA_OFF 0x0004a000
#define	IVS_XVIA_OFF	0x00048004
#define	ARCR_XVIA_OFF	0x000440B0
#define	TTR_XVIA_OFF	0x000440A0
#define	RCR_XVIA_OFF	0x000440D0
#define	LICR_XVIA_OFF	0x00044024
#define	LBTR_XVIA_OFF	0x000440A4
#define	ICFR_XVIA_OFF	0x000440AC
#define	AMSR_XVIA_OFF	0x000440B4
#define S0C0_XVIA_OFF	0x000440C0
#define S0C1_XVIA_OFF	0x000440C4
#define S1C0_XVIA_OFF	0x000440C8
#define S1C1_XVIA_OFF	0x000440CC
#define	PIOPMR_XVIA_OFF	0x00050000
#define	DMAPMR_XVIA_OFF	0x00060000


/* XVIA Register Definitions		*/

/* CSR - Command/Status Register	*/

#define	CSR_DMA		0x00001000	/* DMA Pagesize (1 = 4K, 0 = 512)*/
#define	CSR_SEL_UDMA	0x00000800	/* Enable Upper DMA space	*/
#define	CSR_SEL_LDMA	0x00000400	/* Enable Lower DMA space	*/
#define	CSR_INT_XACT	0x00000200	/* Perform interlocked xaction (RMW) */
#define CSR_DMA_PMFE	0x00000100	/* DMA Page Map Fault Error	*/
#define	CSR_REV		0x000F0000	/* Board revision		*/
#define	CSR_PIO_PMFE	0x00000080	/* PIO Page Map Fault Error	*/
#define	CSR_YAB_PE	0x00000040	/* YAbus Parity Error		*/
#define	CSR_YAB_XAFE	0x00000020	/* YAbus Transaction Fault Error */
#define	CSR_VME_MFE	0x00000010	/* VMEbus SYSFAIL asserted	*/
#define	CSR_VME_ACLOW	0x00000008	/* VME AC Power Fail		*/
#define	CSR_VME_SYSRST	0x00000004	/* VMEbus SYSRESET asserted	*/
#define	CSR_VME_RESET	0x00000002	/* Reset VMEbus			*/
#define	CSR_MVIB_RST	0x00000001	/* Reset MVIB			*/
#define	CSR_ERR_MSK	( CSR_DMA_PMFE |				\
                        CSR_PIO_PMFE |				\
                        CSR_YAB_PE |				\
                        CSR_YAB_XAFE |				\
                        CSR_VME_MFE )

#define CSR_RESET_ERROR_BITS 0xFFFFFE07     /* used to reset error bits */
#define	CSR_CLR_INT_XACT     0xFFFFFDFF	  /* clear interlocked xaction (RMW) */

/* VIACSR - xVIA Command/Status Register */

#define	VIACSR_ENAB_INT	0x00800000	/* Enable local interrupts	*/
#define	VIACSR_32MB_IO	0x00100000	/* Use 32MB I/O Window size	*/
#define	VIACSR_ENAB_ADP	0x00080000	/* Enable xVIA adapter		*/
#define	VIACSR_3VIA_RST	0x00040000	/* Reset 3VIA			*/
#define	VIACSR_MVIB_RST	0x00020000	/* Reset MVIB			*/
#define	VIACSR_MODEL100	0x00010000	/* Indicates DS5000_100		*/
#define	VIACSR_YAB_FRR	0x00008000	/* YAbus Fail_Request received	*/
#define	VIACSR_YAB_FAIL	0x00004000	/* YAbus Fail_Request sent	*/
#define	VIACSR_YAB_NACK	0x00002000	/* YAbus Nack received		*/
#define	VIACSR_YAB_NCLK	0x00001000	/* No YAbus Receive Clock	*/
#define	VIACSR_YAB_XACT	0x00000800	/* YAbus Xaction error		*/
#define	VIACSR_YAB_TAM	0x00000400	/* YAbus TAM error		*/
#define	VIACSR_YAB_DPE	0x00000200	/* YAbus data parity error	*/
#define	VIACSR_YAB_IDL	0x00000100	/* YAbus idle error		*/
#define	VIACSR_VEC_MASK	0x000000FF	/* xVIA interrupt vector	*/
#define VIACSR_ERR_MSK  0x0000DF00      /* error bits */

/* VIACLR - xVIA Status/Clear Register */

#define VIACLR_CLR_ALL 0x0000FF00	/* clear all error bits */

/* DRRF - Device Revision Register	*/

#define	DRRF_REV	0x0000000F	/* Device Hardware Revision	*/



/* BER - Bus Error Register (VIC)	*/

#define BER_LBERR	0x00000040	/* Local Bus Error		*/
#define BER_VMEBERR	0x00000020	/* VMEbus Error			*/
#define	BER_VMETO	0x00000010	/* VMEbus Time Out		*/
#define	BER_LBTO	0x00000008	/* Local Bus Time Out		*/
#define	BER_SLFACC0	0x00000004	/* Self Access while Master(SLSEL0)*/
#define	BER_SLFACC1	0x00000002	/* Self Access while Master(SLSEL1)*/
#define	BER_LBTO_VME	0x00000001	/* Local Bus timeout during VME acc */
#define	BER_ERR_MSK	( BER_LBERR |				\
                          BER_VMEBERR |				\
                          BER_VMETO |				\
                          BER_LBTO |				\
			  BER_SLFACC0 |                         \
			  BER_SLFACC1 |                         \
                          BER_LBTO_VME )

/* ICR -  Interrupt Control Register (VIC) */

#define	ICR_ENABLE	0x00000080
#define	ICR_IPL_2	0x00000002

/* LICR - Local Interrupt Control Register (VIC) */

#define LICR_EDGE	0x00000020
#define LICR_VECTOR	0x00000010
#define LICR_DISABLE  	0x00000080
/* ERRGI -  Interrupt Control Register (VIC) */

#define	ERRGI_ENABLE	0x000000F0

/* VIVS - Interrupt Vector Source	*/

#define	IVS_VEC_MASK	0x000000FF

/* PIOMR - PIO Map Register		*/

#define	XVIA_PIO_VALID	0x00000001	/* PIO Map Register Valid	    */
#define	XVIA_PIO_BSWAP	0x00000006	/* Byte swap mode mask		    */
#define	XVIA_PIO_ASIZE	0x00000030	/* Address size mask		    */
#define	XVIA_PIO_MASK	0xFFFFF000	/* PIO Map Register Entry Mask	    */
#define	XVIA_PIO_OFFSET 0x00000FFF	/* Offset into 4K page	            */
#define	XVIA_PIO_REGSHFT 12		/* Shift for PIO reg number	    */
#define	XVIA_PIO_ADD_SHIFT 12		/* Shift count for Address/PIO reg  */
#define	XVIA_PIO_FC_SHIFT 10		/* Shift count for Function/PIO reg */
#define	XVIA_PIO_AS_SHIFT 8		/* Shift count for Add Size/PIO reg */
#define	XVIA_PIO_BS_SHIFT 1		/* Shift count for Byte Swap/PIO reg */
#define	XVIA_PIO_DL_SHIFT 3		/* Shift count for Data Len/PIO reg */
#define	_3VIA_NPIOPMR 	0x400		/* Number of PIO PMR's 3VIA(1K)	    */
#define	_3VIA_PLUS_NPIOPMR	0x2000	/* Number of PIO PMR's 3VIA+(8K)    */
#define	XVIA_NBYTE_PIOPMR 0x1000	/* Number of bytes per PIO PMR(4K) */

/* PIO Function Codes			*/

#define	XVIA_PIO_UDAT	0x00		/* User Data			*/
#define	XVIA_PIO_UPROG	0x01		/* User Program			*/
#define	XVIA_PIO_SDAT	0x02		/* Supervisor Data		*/
#define	XVIA_PIO_SPROG	0x03		/* Supervisor Program		*/

/* PIO Address Size Codes		*/

#define	XVIA_PIO_A16	0x02		/* A16 Address Space		*/
#define	XVIA_PIO_A24	0x03		/* A24 Address Space		*/
#define	XVIA_PIO_A32	0x01		/* A32 Address Space		*/

/* DMA_PMR - DMA Map Register		*/

#define	XVIA_DMA_VALID	0x00000001	/* Map Register Valid		    */
#define	XVIA_DMA_SU	0x00000060	/* Allow both user and supervisor   */
#define	XVIA_DMA_ADD_SHIFT 12		/* Shift count for Address/DMA reg  */
#define	XVIA_DMA_BS_SHIFT 1		/* Shift count for Byte Swap/DMA reg */
#define	XVIA_DMA_2432	0x00000018	/* Enable 24 and 32-bit DMA access  */
#define	XVIA_DMA_UDMA	0x40000000	/* Select 2nd GB for PMR mapping    */
#define	XVIA_DMA_MASK	0xE0000000	/* Mask off upper bits		    */


/* IMR - Interrupt Mask Register	*/

#define	IMR_PMRPE	0x00100000	/* Interrupt on Page Map Read Parity */
                                        /* Error			     */
#define	IMR_PMBFE	0x00080000	/* Interrupt on Page Map Fault Error */
#define	IMR_PMFE	0x00040000	/* Interrupt on PIO Map Fault Error  */
#define	IMR_IB_PE	0x00000800	/* Interrupt on IBUS Parity Error    */
#define	IMR_IB_TRFE	0x00000400	/* Interrupt on IBUS Transaction     */
                                        /* Fault Error			     */
#define	IMR_VME_MF	0x00000400	/* Interrupt on VME Module Failure   */
#define	IMR_VME_SYSRST	0x00000200	/* Interrupt on VME System Reset     */
#define	IMR_VME_AC_LOW	0x00000100	/* Interrupt on VME AC Low	     */

/* ARCR	- Arbiter/Requester Configuration Register	*/

#define	ARCR_BR_SHIFT	5		/* Shift of BR level from vbadata    */
                                        /* structure to arcr register        */

/* TTR	- Transfer Timeout Register			*/

#define	TTR_VMETO_SHIFT	5      		/* Shift of VMEbus timeout value from*/
                                        /* vbadata structure to ttr register */
#define	TTR_LBTO_SHIFT	2      		/* Shift of localbus timeout value   */
                                        /* from vbadata structure to ttr     */
                                        /* register.			     */
/* Xvia general definitions	*/

#define	XVIA_NDMAPMR	0x8000		/* Number of DMA PMR's	(32K)	  */
#define	XVIA_NBYTE_DMAPMR 0x1000	/* Page size for DMA PMR's(4K)	  */
#define	XVIA_DMAPMR_SHIFT 0xC		/* Shift count for DMA page size  */
#define	XVIA_INT_VEC	0x0000FFFC	/* Mask for interrupt vector	  */
#define	XVIA_INT_IPL	0x000F0000	/* Mask for interrupt ipl	  */
#define	XVIA_PIO_BASE_OFF 0x00080000	/* Offset for base of PIO space	  */

