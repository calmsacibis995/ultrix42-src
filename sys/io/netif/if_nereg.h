#ifndef lint
/* static char *sccsid = "@(#)if_nereg.h	4.2 (ULTRIX)  9/4/90"; */
#endif /* lint */
/************************************************************************
 *									*
 *			Copyright (c) 1984 - 1989 by			*
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
 *   25-Sep-89  chc (Chran-Ham Chang)
 *	Created the if_nereg.h  module. This module is based upon
 *	a modified version of if_lnreg.h.
 * ---------------------------------------------------------------------
 */

/*
 * Digital SGEC NI Adapter. This module handles architectures which
 * implement Network Interfaces by a direct programmatic interface to
 * the DIGITAL SGEC chip set. 
 */

/*
 * SGEC ring descriptors
 */
struct ne_ring  {
        u_short   ne_flag;		/* rcv/xmt status flags */
	unsigned  ne_com:15;		/* receive length ; transmit TDR */
	unsigned  ne_own:1;		/* ownership bit */
	u_short	  ne_resv;		/* reserved */
	u_short   ne_info;		/* rcv/xmt info for SGEG */
	u_short	  ne_pg_off;		/* page offset */
	u_short   ne_bsize;		/* rcv/xmt buffer size */
	u_long    ne_bfaddr;		/* buffer address */
};
  		

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef __mips
#define NEDESC volatile struct ne_ring

#else /* vax */
#define NEDESC struct ne_ring
#endif

/*
 * SGEC receive status (ne_flag RDES0<00:15>)
 */
#define NE_OF			0x0001	/* Overflow			*/
#define NE_CE			0x0002	/* CRC error			*/
#define NE_DB			0x0004	/* Dribbling Bits		*/
#define NE_RTN			0x0008	/* Translation Not Valid        */
#define NE_FT			0x0020	/* Frame Type 			*/
#define NE_CS			0x0040	/* Collision Seen 		*/
#define NE_RTL			0x0080	/* Frame Too Long		*/
#define NE_RLS			0x0100	/* Last Segment			*/
#define NE_RFS			0x0200	/* First Segment		*/
#define NE_BO			0x0400	/* Buffer Overflow 		*/
#define NE_RF			0x0800	/* Runt Frame	 		*/
#define NE_RLE			0x4000	/* Length Error 		*/

/* 
 * SGEC Own bit (ne_own RDES0<31> & TDES0<31>)  
 */
#define NE_OWN			0x0001 /* Own bit, 1 own by SGEC, 0 own

/* SGEC info (ne_info RDES1<30:31> & TDES1<30:31>) */
#define NE_VA			0x4000	/* Virtual Address */
#define NE_CA			0x8000	/* Chain Address */

/* 
 * SGEC error summary 
 */
#define NE_ES			0x8000	/* Error Summary 		*/

/* 
 * SGEC packet Data type 
/*
#define NE_DT			0x3000   /* Mask for the Data type      */
/* Data Type */
#define NE_DT_TNOR		0x0000   /* Normal Transmit Frame Data  */
#define NE_DT_TSET		0x2000   /* Setup Frame	                */
#define NE_DT_TDIA		0x3000   /* Diagnostic Frame            */
#define NE_DT_RSRF		0x0000   /* Serial Received Frame       */
#define NE_DT_RILF		0x1000   /* Internally Looped back Frame*/
#define NE_DT_RELF		0x2000   /* External looped back Frame  */


/*
 * SGEC transmit status (ne_flag TDES0<00:15>)
 */
#define	NE_DE			0x0001	/* Deferred - network busy	*/
#define	NE_UF			0x0002	/* Underflow Error		*/
#define	NE_TTN			0x0004	/* Translation Not Valid	*/
#define	NE_CC			0x0078	/* Mask for Collision Count	*/
#define	NE_HF			0x0080	/* Heartbeat Fail		*/
#define	NE_EC			0x0100	/* Excessive Collisions		*/
#define	NE_LC			0x0200	/* Late Collision		*/
#define	NE_NC			0x0400	/* No Carrier			*/
#define	NE_LO			0x0800	/* Loss of Carrier 		*/
#define	NE_TLE			0x1000	/* Length Error			*/
#define	NE_TO			0x4000	/* Transmit Watchdog Timeout    */

/* transmit info (ne_info, TDES0<24:27>)
#define	NE_IC			0x0100	/* Interrupt on Completion	*/ 
#define	NE_TLS			0x0200	/* Last Segment			*/ 
#define	NE_TFS			0x0400	/* First Segment		*/ 
#define	NE_AC			0x0800	/* Add CRC disable		*/ 

/* Page Offset (ne_pg_off<7:0>) */
#define NE_PF			0x00ff	/* Page Offset		        */

/*
 * SGEC Vector Address, IPL, Sync/Asynch (CSR0)
 */
#define NE_CSR_IV	0x0000fffc	/* Interrupt Vector		*/
#define NE_CSR_SA	0x20000000	/* Sync/Asynch Mode		*/
#define NE_CSR_IP	0xc0000000	/* Interrupt Priority		*/
#define NE_CSR_INIT 	~(NE_CSR_IV | NE_CSR_SA | NE_CSR_IP)

/* 
 * SGEC Transmit Polling Demand (CSR1)
 */
#define NE_CSR1_PD	0x00000001	/* Tx Polling Demand            */ 

/* 
 * SGEC Receive Polling Demand (CSR2)
 */
#define NE_CSR2_PD	0x00000001	/* Rx Polling Demand 		*/

/* 
 * SGEC Status Register (CSR5)
 */
#define NE_CSR5_IS	0x00000001	/* Interrupt Summary		*/
#define NE_CSR5_TI	0x00000002	/* Transmit Interrupt		*/
#define NE_CSR5_RI	0x00000004	/* Receive Interrupt            */
#define NE_CSR5_RU	0x00000008	/* Receive Buffer Unavailable   */
#define NE_CSR5_ME	0x00000010	/* Memory Error			*/
#define NE_CSR5_RW	0x00000020	/* Rx Watchdog Timer Interrupt  */
#define NE_CSR5_TW	0x00000040	/* Tx Watchdog Timer Interrupt  */
#define NE_CSR5_BO	0x00000080	/* Boot_Message			*/
#define NE_CSR5_DN	0x00010000	/* Done				*/
#define NE_CSR5_OM	0x00060000	/* Operating Mode		*/
/* Operation Mode */
#define NE_OM5_NOR	0x00000000	/* Normal Operation Mode        */
#define NE_OM5_INL	0x00020000	/* Internal Loopback            */
#define NE_OM5_EXL	0x00040000	/* External Loopback            */
#define NE_OM5_DIA	0x00060000	/* Diagnostic Mode	        */

#define NE_CSR5_RS	0x00c00000	/* Reception process State      */
/* Reception process State */
#define NE_RS_STP	0x00000000	/* Stoppped		        */
#define NE_RS_RUN	0x00400000	/* Running		        */
#define NE_RS_SUP	0x00800000	/* Suspended                    */

#define NE_CSR5_TS	0x03000000	/* Transmission process State   */
/* Reception process State */
#define NE_TS_STP	0x00000000	/* Stoppped		        */
#define NE_TS_RUN	0x01000000	/* Running		        */
#define NE_TS_SUP	0x02000000	/* Suspended                    */

#define NE_CSR5_SS	0x3c000000	/* Self Test Status		*/
/* Reception process State */
#define NE_SS_ROM	0x04000000	/* ROM Error 		        */
#define NE_SS_RAM	0x08000000	/* RAM Error 		        */
#define NE_SS_AFR	0x0c000000	/* Address filter RAM Error	*/
#define NE_SS_TFF	0x10000000	/* Transmit FIFO Error	        */
#define NE_SS_RFF	0x14000000	/* Receive FIFO Error	        */
#define NE_SS_SLE	0x18000000	/* Self test Loopback Error     */

#define NE_CSR5_SF	0x40000000	/* Self Test Failed 		*/
#define NE_CSR5_ID	0x80000000	/* Initialization Done		*/

/*
 * SGEC Command and Mode Register (CSR6)
 */
#define NE_CSR6_AF 	0x00000006   	/* Address Filtering Mode */
/* Address Filtering Mode */
#define NE_AF_NOR 	0x00000000 	/* Normal			 */
#define NE_AF_PRO 	0x00000002 	/* Promiscuous			 */
#define NE_AF_MUL 	0x00000004 	/* ALL Multicast  		 */  

#define NE_CSR6_PB	0x00000008	/* Pass Bad Frames Mode		*/
#define NE_CSR6_FC	0x00000040	/* Force Collision Mode		*/
#define NE_CSR6_DC	0x00000080	/* Disable Data Chaining Mode	*/
#define NE_CSR6_OM	0x00000300	/* Operating Mode 		*/
/* Operation Mode */
#define NE_OM6_NOR	0x00000000	/* Normal Operation Mode        */
#define NE_OM6_INL	0x00000100	/* Internal Loopback            */
#define NE_OM6_EXL	0x00000200	/* External Loopback            */
#define NE_OM6_DIA	0x00000300	/* Diagnostic Mode	        */

#define NE_CSR6_SR	0x00000400	/* Start/Stop Rx Command	*/
#define NE_CSR6_ST	0x00000800	/* Start/Stop Tx Command	*/
#define NE_CSR6_SE	0x00080000	/* Single Cycle Enable Mode	*/
#define NE_CSR6_BE	0x00100000	/* Boot Message Enable Mode     */
#define NE_CSR6_BL	0x1e000000	/* Burst Limit Mode	        */
#define NE_CSR6_IE	0x40000000	/* Interrupt Enable Mode        */
#define NE_CSR6_RE	0x80000000	/* Reset Command	        */

/*
 * SGEC Revision Number and Missed Frame Count (CSR10)
 */
#define NE_CSR10_MFC	0x0000ffff	/* Mask for Missed Frame Count  */
#define NE_CSR10_RN	0x000f0000	/* Chip Revision Number	        */


/* 
 * SGEC Boot Message (CSR11,12,13)
 */
#define NE_CSR13_PRC	0x0000007f	/* Boot Message Processor ID    */

/* 
 * SGEC setup frame error indicator
 */
#define NE_SETUP_IC	0x01000000	/* Interrup on Completion */
#define NE_SETUP_SE	0x00002000	/* Setup frame length error */
/*
 * Misc
 */
#define NE_CC_SHIFT 	3		/* Transmit Collision Count     */ 
#define NE_BL_SHIFT	24		/* Burst limit shift */
#define NE_RE_SHIFT	16		/* Revision number shift */
#define NE_BURST_SHIFT 	25		/* Burst limit shift */
#define NE_OCTAWORD     0x00000007      /* octaword alignment */
#define NE_LONGWORD     0x00000003      /* longword alignment */
#define NE_WORD     	0x00000001      /* word alignment */
#define NE_IPL_14       0x00000000      /* SGEC IPL 14  */
#define NE_IPL_15       0x40000000      /* SGEC IPL 15  */
#define NE_IPL_16       0x80000000      /* SGEC IPL 16  */
#define NE_IPL_17       0xc0000000      /* SGEC IPL 17  */
#define NE_BUS_ASYN     0x00000000      /* SGEC Asyn Mode  */
#define NE_BUS_SYN      0x20000000      /* SGEC Syn Mode   */
