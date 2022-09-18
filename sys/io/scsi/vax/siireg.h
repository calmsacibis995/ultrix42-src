
/*
 *	7/2/90	(ULTRIX)	@(#)siireg.h	4.1
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

 /***********************************************************************
  * siireg.h	4-May-88
  *
  * Modification History
  *
  * 4-May-88	darrell - Creation of this file
  *
  ***********************************************************************/

/*
 * SII registers
 */
struct sii_regs
{
	u_short sii_sdb;	/* SCSI Data Bus and Parity		*/
	u_short pad0;
	u_short sii_sc1;	/* SCSI Control Signals One		*/
	u_short pad1;
	u_short sii_sc2;	/* SCSI Control Signals Two		*/
	u_short pad2;
	u_short sii_csr;	/* Control/Status register		*/
	u_short pad3;
	u_short sii_id;		/* Bus ID register			*/
	u_short pad4;
	u_short sii_slcsr;	/* Select Control and Status Register	*/
	u_short pad5;
	u_short sii_destat;	/* Selection Detector Status Register	*/
	u_short pad6;
	u_short sii_dstmo;	/* DSSI Timeout Register		*/
	u_short pad7;
	u_short sii_data;	/* Data Register			*/
	u_short pad8;
	u_short sii_dmctrl;	/* DMA Control Register			*/
	u_short pad9;
	u_short sii_dmlotc;	/* DMA Length of Transfer Counter	*/
	u_short pad10;
	u_short sii_dmaddrl;	/* DMA Address Register Low		*/
	u_short pad11;
	u_short sii_dmaddrh;	/* DMA Address Register High		*/
	u_short pad12;
	u_short sii_dmabyte;	/* DMA Initial Byte Register		*/
	u_short pad13;
	u_short sii_stlp;	/* DSSI Short Target List Pointer	*/
	u_short pad14;
	u_short sii_ltlp;	/* DSSI Long Target List Pointer	*/
	u_short pad15;
	u_short sii_ilp;	/* DSSI Initiator List Pointer		*/
	u_short pad16;
	u_short sii_dsctrl;	/* DSSI Control Register		*/
	u_short pad17;
	u_short sii_cstat;	/* Connection Status Register		*/
	u_short pad18;
	u_short sii_dstat;	/* Data Transfer Status Register	*/
	u_short pad19;
	u_short sii_comm;	/* Command Register			*/
	u_short pad20;
	u_short sii_dictrl;	/* Diagnostic Control Register		*/
	u_short pad21;
	u_short sii_clock;	/* Diagnostic Clock Register		*/
	u_short pad22;
	u_short sii_bhdiag;	/* Bus Handler Diagnostic Register	*/
	u_short pad23;
	u_short sii_sidiag;	/* SCSI IO Diagnostic Register		*/
	u_short pad24;
	u_short sii_dmdiag;	/* Data Mover Diagnostic Register	*/
	u_short pad25;
	u_short sii_mcdiag;	/* Main Control Diagnostic Register	*/
	u_short pad26;
};

/*
 * SC1 - SCSI Control Signals One
 */
#define SII_SC1_MSK	0x1ff		/* All possible signals on the bus    */
#define SII_SC1_SEL	0x80		/* SCSI SEL signal active on bus      */
#define SII_SC1_ATN	0x08		/* SCSI ATN signal active on bus      */

/*
 * SC2 - SCSI Control Signals Two
 */
#define SII_SC2_IGS	0x8		/* SCSI drivers for initiator mode    */

/*
 * CSR - Control/Status Register
 */
#define SII_HPM	0x10			/* SII in on an arbitrated SCSI bus   */
#define	SII_RSE	0x08			/* 1 = respond to reselections	      */
#define SII_SLE	0x04			/* 1 = respond to selections	      */
#define SII_PCE	0x02			/* 1 = report parity errors	      */
#define SII_IE	0x01			/* 1 = enable interrupts	      */

/*
 * ID - Bus ID Register
 */
#define SII_ID_IO	0x8000		/* I/O 				      */

/*
 * DESTAT - Selection Detector Status Register
 */
#define SII_IDMSK	0x7		/* ID of target reselected the SII    */

/*
 * DMCTRL - DMA Control Register
 */
#define SII_ASYNC	0x00		/* REQ/ACK Offset for async mode      */
#define SII_SYNC	0x03		/* REQ/ACK Offset for sync mode	      */

/*
 * DMLOTC - DMA Length Of Transfer Counter
 */
#define SII_TCMSK	0x1fff		/* transfer count mask		      */

/*
 * CSTAT - Connection Status Register
 */
#define	SII_CI		0x8000	/* composite interrupt bit for CSTAT	      */
#define SII_DI		0x4000	/* composite interrupt bit for DSTAT	      */
#define SII_RST_ONBUS	0x2000	/* 1 if reset is asserted on SCSI bus	      */
#define	SII_BER		0x1000	/* Bus error				      */
#define	SII_OBC		0x0800	/* Out_en Bit Cleared (DSSI mode)	      */
#define SII_TZ		0x0400	/* Target pointer Zero (STLP or LTLP is zero) */
#define	SII_BUF		0x0200	/* Buffer service - outbound pkt to non-DSSI  */
#define SII_LDN		0x0100	/* List element Done			      */
#define SII_SCH		0x0080	/* State Change				      */
#define SII_CON		0x0040	/* SII is Connected to another device	      */
#define SII_DST_ONBUS	0x0020	/* SII was Destination of current transfer    */
#define SII_TGT_ONBUS	0x0010	/* SII is operating as a Target		      */
#define SII_SWA		0x0008	/* Selected With Attention		      */
#define SII_SIP		0x0004	/* Selection In Progress		      */
#define SII_LST		0x0002	/* Lost arbitration			      */

/*
 * DSTAT - Data Transfer Status Register
 */
#define SII_DNE		0x2000	/* DMA transfer Done			      */
#define SII_TCZ		0x1000	/* Transfer Count register is Zero	      */
#define SII_TBE		0x0800	/* Transmit Buffer Empty		      */
#define SII_IBF		0x0400	/* Input Buffer Full			      */
#define SII_IPE		0x0200	/* Incoming Parity Error		      */
#define SII_OBB		0x0100	/* Odd Byte Boundry			      */
#define SII_MIS		0x0010	/* Phase Mismatch			      */
#define SII_ATN		0x0008	/* ATN set by initiator if in Target mode     */
#define SII_MSG		0x0004	/* current bus state of MSG		      */
#define SII_CD		0x0002	/* current bus state of C/D		      */
#define SII_IO		0x0001	/* current bus state of I/O		      */
#define SII_PHA_MSK	0x0007	/* Phase Mask				      */

/*
 * COMM - Command Register
 */
#define	SII_DMA		0x8000	/* DMA mode				      */
#define SII_RST		0x4000	/* Assert reset on SCSI bus for 25 usecs      */
#define SII_RSL		0x1000	/* 0 = select, 1 = reselect desired device    */

/* Commands 	I - Initiator, T - Target, D - Disconnected		      */
#define SII_INXFER	0x0800	/* Information Transfer command	(I,T)	      */
#define SII_SELECT	0x0400	/* Select command		(D)	      */
#define SII_REQDATA	0x0200	/* Request Data command		(T)	      */
#define	SII_DISCON	0x0100	/* Disconnect command		(I,T,D)	      */
#define SII_CHRESET	0x0080	/* Chip Reset command		(I,T,D)	      */
/* Chip state bits */
#define	SII_CON		0x0040	/* Connected				      */
#define SII_DST		0x0020	/* Destination				      */
#define SII_TGT		0x0010	/* Target				      */
#define SII_STATE_MSK  0x0070	/* State Mask				      */
/* SCSI control lines */
#define SII_ATN		0x0008	/* Assert the SCSI bus ATN signal	      */
#define	SII_MSG		0x0004	/* Assert the SCSI bus MSG signal	      */
#define	SII_CD		0x0002	/* Assert the SCSI bus C/D signal	      */
#define	SII_IO		0x0001	/* Assert the SCSI bus I/O signal	      */

/*
 * DICTRL - Diagnostic Control Register
 */
#define SII_PRE		0x4	/* Enable the SII to drive the SCSI bus	      */

#define SII_WAIT_COUNT		10000   /* Delay count used for the SII chip  */
#define SII_MAX_DMA_XFER_LENGTH	8192  	/* Max DMA transfer length for SII    */
