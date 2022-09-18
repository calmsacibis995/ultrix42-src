/*
 *	@(#)uqport.h	4.2	(ULTRIX)	10/16/90
 */


/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************
 *
 * Revision History:
 * 16-Oct-90		U. Sinkewicz
 *	Bug fixes required for DUP as a layered product.
 *
 * 20-Jul-1989		Mark A. Parenti
 *	Add burst rate to controller table.
 *	Sync controller table with SSP spec.
 *	Define UQ_CS bit for SSP scratchpad support.
 *	Add defines for new device register access method.
 *
 * 06-Jun-1988		Ricky S. Palmer
 *	Added support for KFQSA
 *
 * 19-Jan-1988          Todd M. Katz
 *	Delete LPC_SAFAT definition.
 */
/*^L*/


/*
 *	Defines for use with port rings
 */

#define	UQ_ERR	 0x00008000	/* error bit 				*/
#define	UQ_STEP4 0x00004000	/* step 4 has started 			*/
#define	UQ_STEP3 0x00002000	/* step 3 has started 			*/
#define	UQ_STEP2 0x00001000	/* step 2 has started 			*/
#define	UQ_STEP1 0x00000800	/* step 1 has started 			*/
#define	UQ_NV	 0x00000400	/* no host settable interrupt vector 	*/
#define	UQ_QB	 0x00000200	/* controller supports Q22 bus 		*/
#define	UQ_DI	 0x00000100	/* controller implements diagnostics 	*/
#define	UQ_IE	 0x00000080	/* interrupt enable 			*/
#define	UQ_PI	 0x00000001	/* host requests adapter purge interrupts */
#define	UQ_LF	 0x00000002 	/* request last fail packet		*/
#define	UQ_GO	 0x00000001	/* start operation, after init 		*/
#define	UQ_CS	 0x00000400	/* Controller scratchpad supported	*/
#define	UQ_OWN	 0x80000000	/* controller owns this descriptor	*/
#define	UQ_INT	 0x40000000	/* allow interrupt on ring transition 	*/
#define	UQ_MAP	 0x80000000	/* modifier for mapped buffer descriptors */

/*	UQSSP Message types						*/
#define	UQT_SEQ		0	/* Sequential Message			*/
#define	UQT_DG		1	/* Datagram Message			*/
#define	UQT_CRED	2	/* Credit Message			*/
#define	UQT_MAINT	15	/* Maintenance Message			*/

/*	UQSSP Connection IDs						*/
#define	UQC_DISK	0	/* Disk class connection		*/
#define	UQC_TAPE	1	/* Tape class connection		*/
#define	UQC_DUP		2	/* DUP class connection			*/
#define	UQC_MAINT	255	/* Maintenance connection		*/

/*	UQSSP Connection IDs - Bit Representation			*/
#define	UQCB_DISK	1<<UQC_DISK /* Disk class connection		*/
#define	UQCB_TAPE	1<<UQC_TAPE /* Tape class connection		*/
#define	UQCB_DUP	1<<UQC_DUP /* DUP class connection		*/

/*	UQ Controller Information Block
 */
struct	uq_cinfo {
	char	*name;		/* Controller name in ascii		    */
	int	servers;	/* Server bit vector			    */
	int	max_con;	/* Maximum number of concurrent connections */
	int	burst;		/* Bus burst rate (for step 4 write)        */
};	

/*	Controller Info Block
 */
/* The following table contains the burst rate for each uq controller type.
 * The meaning of this value varies for each controller. In general, 
 * this rate instructs the controller as to how many longwords it may
 * transfer on the bus before having to rearbitrate.
 * The value given is one LESS than the actual desired burst rate. The
 * range is 0 to 63. Values above 3 should only be used on dedicated buses
 * and values above 7 give only marginally better performance.
 * For XMI-based controllers, this value should be the optimal number
 * of longwords for DMA transactions.
 */

struct	uq_cinfo uq_cinfo[] =
{
/*  Ascii name	  Server vector	 Max connections   Burst	*/
   { "uda50",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 0 uda50 */
   { "rc25",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 1 rc25 */
   { "rux50",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 2 rux */
   { "tk50",	UQCB_TAPE | UQCB_DUP,	1,  	    3 },	/* 3 tk50 */
   { "uqssp",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 4 unk */
   { "tu81",	UQCB_TAPE | UQCB_DUP,	1,  	    3 },	/* 5 tu81 */
   { "uda50a",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 6 uda50a */
   { "rqdx",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 7 rqdx */
   { "uqssp",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 8 */
   { "uqssp",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 9 */
   { "uqssp",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 10 */
   { "uqssp",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 11 */
   { "uqssp",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 12 */
   { "kda50",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 13 kda50 */
   { "tk70",	UQCB_TAPE | UQCB_DUP,	1,  	    3 },	/* 14 tk70 */
   { "rv20",	UQCB_TAPE | UQCB_DISK | UQCB_DUP, 1, 3 },	/* 15 rv20 */
   { "krq50",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 16 krq50 */
   { "uqssp",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 17 uqssp */
   { "kdb50",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 18 kdb50 */
   { "rqdx3",	UQCB_DISK | UQCB_DUP,	1,  	    3 },	/* 19 rqdx3 */
   { "kdm70",	UQCB_DISK | UQCB_TAPE | UQCB_DUP, 3, 16 },	/* 20  kdm70 TEMP FOR NOW*/
   { "kfqsa",	UQCB_DISK | UQCB_DUP,	2,  	    3 },	/* 21 kfqsa disk */
   { "kfqsa",	UQCB_TAPE | UQCB_DUP,	2,  	    3 },	/* 22 kfqsa tape */
   { "kfqsa",	UQCB_TAPE | UQCB_DISK | UQCB_DUP, 3, 3 },	/* 23 kfqsa disk and tape */
   { "kfqsa",	UQCB_TAPE | UQCB_DISK | UQCB_DUP, 3, 3 },	/* 24 kfqsa other */
   { "uqssp",	UQCB_DISK | UQCB_DUP, 1,  	    3 },	/* 25 */
   { "kru50",	UQCB_DISK | UQCB_DUP, 1,  	    3 },	/* 26 kru50 */
   { "kdm70",	UQCB_DISK | UQCB_TAPE | UQCB_DUP, 3, 16 },	/* 27  kdm70 */
   { "tk70l",	UQCB_DISK | UQCB_TAPE | UQCB_DUP, 1, 3 },	/* 28  tk70l */
   { "tm32",	UQCB_DISK | UQCB_TAPE | UQCB_DUP, 1, 3 }	/* 29  tm32 */
};
	
/*	UQ port types
 */

#define		UDA_TYPE	0		
#define		RC25_TYPE	1
#define		RUX_TYPE	2		
#define		MAYA_TYPE	3		
#define		TU81_TYPE	5
#define		UDA50A_TYPE	6		
#define		RQDX_TYPE	7
#define		KDA50A_TYPE	13
#define		TK70_TYPE	14
#define		RV20_TYPE	15
#define		KRQ50_TYPE	16
#define		BDA_TYPE	18
#define		RQDX3_TYPE	19
#define		KFQSA_D_TYPE	21
#define		KFQSA_T_TYPE	22
#define		KFQSA_DT_TYPE	23
#define		KFQSA_OTHER	24
#define		KRU50_TYPE	26
#define		KDM_TYPE	27
#define		TQK7L_TYPE	28
#define		TM32_TYPE	29
#define		MAXUQNAME	29

#define UQ_IP		0x00	/* UQ offset of IP register		*/
#define	UQ_SA		0x02	/* UQ offset of SA register		*/

#define	UQB_IP		0xF2	/* BIIC offset of BDA IP register 	*/
#define UQB_SA		0xF4	/* BIIC offset of BDA read-only SA register */
#define UQB_SAW		0xF6	/* BIIC offset of BDA write-only SA register */
#define	UQB_IP_OFF	0x00	/* Offset of BDA IP register		*/
			        /* from port reg base			*/
#define UQB_SA_OFF	0x02	/* Offset of BDA read-only SA register	*/
                                /* from port register base 		*/
#define UQB_SAW_OFF	0x04	/* Offset of BDA write-only SA register */
                                /* from port register base		*/

#define UQX_IP		0x40	/* XMI offset of IP register		*/
#define	UQX_SA		0x44	/* XMI offset of SA register		*/
#define	UQX_PD		0x48	/* XMI offset of PD register		*/
#define UQX_IP_OFF	0x00	/* Offset of XMI IP register 		*/
                                /* from port register base		*/
#define	UQX_SA_OFF	0x04	/* Offset of XMI SA register		*/
                                /* from port register base		*/
#define	UQX_PD_OFF	0x08	/* Offset of XMI PD register		*/
                                /* from port register base		*/

#define	Uqip		uqregptrs.uqip
#define	Uqsa		uqregptrs.uqsa
#define	Uqsaw		uqregptrs.uqsaw
#define	Uqpd		uqregptrs.uqpd

/* 	Miscellaneous defines				*/
#define UQ_MAX_REINIT	3	/* Maximum number of init retries	*/
#define	UQ_BURST	0x7F	/* Burst rate in controller flags field	*/
#define S_IDLE  0               /* hasn't been initialized */
#define S_STEP1 1               /* doing step 1 init */
#define S_STEP2 2               /* doing step 2 init */
#define S_STEP3 3               /* doing step 3 init */
#define	S_STEP4 4		/* doing step 4 init */
#define S_RUN   5               /* running */

/*	UQ error log codes						*/
#define	UQ_SA_FATAL		1	/* Fatal error in SA register	*/
#define	UQ_RESET_FAIL		2	/* Initialization failed	*/


/*	SCS connection process names					*/
/*	NOTE: The following must be the same length as NAME_SIZE	*/
/*	defined in sysap.h						*/

#define	DISK_NAME	"MSCP$DISK       " /* Disk server name		*/
#define	TAPE_NAME	"MSCP$TAPE       " /* Tape server name		*/
#define	DUP_NAME	"DUP             " /* Dup server name		*/


/*	uq_flags definitions						*/

#define	UQ_PRB	0x00000001	/* in probe routine			*/
#define	UQ_TIM	0x00000002	/* timer started			*/
