/*
 * @(#)if_lnreg.h	4.3	(ULTRIX)	9/4/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984, 85, 86 by			*
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

/* ---------------------------------------------------------------------
 *  Modification History:
 *
 *  23-Aug-90  Lea Gottfredsen
 *	added BIT16SET, LDPBITS for lance dma option
 *
 *  12-Jul-90  Lea Gottfredsen
 *	Added LN_OCTA_ALIGN.
 *
 *  7-Jun-89   Lea Gottfredsen						 
 *	Driver rewrite.
 *
 *  14-Dec-88  templin (Fred L. Templin)
 *	Changed LN_BUF_SIZE to 0x600 (1536 bytes)
 *
 *   5-Jan-88  templin (Fred L. Templin)
 *	Created the if_lnreg.h  module. This module is based upon
 *	a modified version of if_sereg.h.
 * ---------------------------------------------------------------------
 */

/*
 * Digital LANCE NI Adapter. This module handles architectures which
 * implement Network Interfaces by a direct programmatic interface to
 * the AMD LANCE chip set. 
 */

/*
 * Network INIT Block
 */
 struct ln_initb {
        u_short ln_mode;                /* NIB_MODE mode word */
        u_short ln_sta_addr[3];         /* NIB_PADR station address */
        u_short ln_multi_mask[4];       /* NIB_LADRF Multicast addr ma sk*/
        u_short ln_rcvlist_lo,          /* NIB_RDRP Rcv Dsc Ring Ptr */
                ln_rcvlist_hi:8,        /* Rcv list hi addr */
                ln_rcvresv:5,           /* reserved */
                ln_rcvlen:3;            /* RLEN Rcv ring length **/
        u_short ln_xmtlist_lo,          /* NIB_TDRP Xmt Dsc Ring Ptr */
                ln_xmtlist_hi:8,        /* Xmt list hi addr */
                ln_xmtresv:5,           /* reserved */
                ln_xmtlen:3;            /* TLEN Xmt ring length */
};
/*
 * LANCE ring descriptors
 */
struct ln_ring  {
        u_short ln_addr_lo;             /* Low order bits of buf addr */
        char ln_addr_hi;                /* Hi order bits of buf addr */
        char ln_flag;                   /* rcv/xmt status flag */
        short ln_buf_len; 		/* buffer length (2s comp)  */
	u_short ln_flag2;               /* transmit: status/TDR */
                                        /* receive: len of packet */
};

/*
 * LANCE initialization block mode word
 */
#define LN_DRX			0x0001	/* Disable receiver		*/
#define LN_DTX			0x0002	/* Disable transmitter		*/
#define LN_LOOP			0x0004	/* Loopback control		*/
#define LN_DTCR			0x0008	/* Disable transmit CRC		*/
#define LN_COLL			0x0010	/* Force collision		*/
#define LN_DIRTY		0x0020	/* Disable retry		*/
#define LN_INTL			0x0040	/* Internal loopback		*/
#define LN_PROM			0x8000	/* Promiscuous mode		*/
#define LN_INT_NP		0x0020  /* network primary, bit 5       */


/*
 * LANCE receive status (ln_flag)
 */
#define LN_ENP			0x0001	/* End of packet		*/
#define LN_STP			0x0002	/* Start of packet		*/
#define LN_RBUFF		0x0004	/* Receive Buffer error		*/
#define LN_CRC			0x0008	/* Checksum error		*/
#define LN_OFLO			0x0010	/* Overflow error		*/
#define LN_FRAM			0x0020	/* Framing error		*/
#define LN_RT_ERR		0x0040	/* Lance Error summary 		*/
#define LN_OWN			0x0080	/* Owned flag (1=Lance, 0=host)	*/

/* LANCE receive length (ln_flag2,11:0) */
#define LN_MCNT			0x0fff	/* low 12 bits of ln_flag2	*/

/*
 * LANCE transmit status (ln_flag)
 */
#define	LN_DEF			0x0004	/* Deferred - network busy	*/
#define	LN_ONE			0x0008	/* One retry was required	*/
#define	LN_MORE			0x0010	/* More retries were required	*/

/* transmit status (ln_flag2,15:10) */
#define	LN_TBUFF		0x8000	/* Transmit buffer error	*/
#define	LN_UFLO			0x4000	/* Underflow			*/
#define	LN_LCOL			0x1000	/* Late collision		*/
#define	LN_LCAR			0x0800	/* Loss of carrier		*/
#define	LN_RTRY			0x0400	/* Retries exhausted		*/

/* transmit TDR (ln_flag2,9:0) */
#define LN_TDR			0x03ff	/* Time Domain Reflectometer	*/

/*
 * LANCE command and status bits (CSR0)
 */
#define LN_INIT			0x0001	/* (Re)-Initialize		*/
#define LN_START		0x0002	/* Start operation		*/
#define LN_STOP			0x0004	/* Reset firmware (stop)	*/
#define LN_TDMD			0x0008	/* Transmit on demand		*/
#define LN_TXON			0x0010	/* Transmitter is enabled	*/
#define LN_RXON			0x0020	/* Receiver is enabled		*/
#define LN_INEA			0x0040	/* Interrupt enable		*/
#define LN_INTR			0x0080	/* Interrupt request		*/
#define LN_IDON 		0x0100	/* Initialization done		*/
#define LN_TINT			0x0200	/* Transmitter interrupt	*/
#define LN_RINT			0x0400	/* Receive interrupt		*/
#define LN_MERR			0x0800	/* Memory error			*/
#define LN_MISS			0x1000	/* Missed packet		*/
#define LN_CERR			0x2000	/* Collision error		*/
#define LN_BABL			0x4000	/* Transmit timeout err		*/
#define LN_ERR			0x8000	/* Error summary		*/

/*
 * LANCE CSR3 status word
 * (these should always be 0 for the LANCE hardware)
 */
#define LN_BCON			0x0001	/* Byte control 		*/
#define LN_ACON			0x0002	/* ALE control			*/
#define LN_BSWP			0x0004	/* Byte swap (for DMA)		*/

/*
 * LANCE CSR select
 */
#define LN_CSR0			0x0000	/* CSR 0			*/
#define LN_CSR1			0x0001	/* CSR 1			*/
#define LN_CSR2			0x0002	/* CSR 2			*/
#define LN_CSR3			0x0003	/* CSR 3			*/

/*
 * General constant definitions
 */
#define LNNOALLOC		0	/* No buffer allocation		*/
#define LNALLOC 		1	/* Allocate local RAM buffer	*/

#define	LN_WORD_ALIGN		0x01	/* Check for word alignment	*/
#define	LN_QUAD_ALIGN		0x07	/* Check for quad alignment	*/
#define	LN_OCTA_ALIGN		0x0F	/* Check for octaword alignment */
#define	LN_BUF_SIZE		0x0600	/* 1536 Byte buffers for rings	*/
#define	LN_LRB_SIZE		0x20000	/* 128K byte addresses */

#define LN_CRC_INIT		1	/* initialize CRC table		*/

#define	LN_NONDMA_RCV		0	/* Non-DMA architectures	*/
#define LN_DMA_RCV		1	/* LANCE receieve DMA		*/

#define LN_DMA_3MIN		1	/* LANCE DMA on the 3min option	*/

#define	BIT16SET		0x10000	/* ck if 16th bit set or set it */
#define LDPBITS			0x1ffe0000 /* LDP bit mask   	       */
