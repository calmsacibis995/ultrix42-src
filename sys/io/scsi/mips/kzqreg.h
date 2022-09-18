/*
#ifndef lint
static char 	*sccsid = "@(#)kzqreg.h	4.1 (ULTRIX)		9/11/90";
#endif lint
/*

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
  * kzqreg.h	04/11/89
  *
  * Modification History
  *
  * 06-Jun-90   Charles Richmond - cloned siireg.h into kzqreg.h and
  *		added the kzq_ registers.
  *
  ***********************************************************************/

/*
 * SII and KZQSA registers
 */
struct kzq_regs
{
	u_short sii_sdb;	/* SCSI Data Bus and Parity		*/
	u_short sii_sc1;	/* SCSI Control Signals One		*/
	u_short sii_sc2;	/* SCSI Control Signals Two		*/
	u_short sii_csr;	/* Control/Status register		*/
	u_short sii_id;		/* Bus ID register			*/
	u_short sii_slcsr;	/* Select Control and Status Register	*/
	u_short sii_destat;	/* Selection Detector Status Register	*/
	u_short sii_dstmo;	/* DSSI Timeout Register		*/
	u_short sii_data;	/* Data Register			*/
	u_short sii_dmctrl;	/* DMA Control Register			*/
	u_short sii_dmlotc;	/* DMA Length of Transfer Counter	*/
	u_short sii_dmaddrl;	/* DMA Address Register Low		*/
	u_short sii_dmaddrh;	/* DMA Address Register High		*/
	u_short sii_dmabyte;	/* DMA Initial Byte Register		*/
	u_short sii_stlp;	/* DSSI Short Target List Pointer	*/
	u_short sii_ltlp;	/* DSSI Long Target List Pointer	*/
	u_short sii_ilp;	/* DSSI Initiator List Pointer		*/
	u_short sii_dsctrl;	/* DSSI Control Register		*/
	u_short sii_cstat;	/* Connection Status Register		*/
	u_short sii_dstat;	/* Data Transfer Status Register	*/
	u_short sii_comm;	/* Command Register			*/
	u_short sii_dictrl;	/* Diagnostic Control Register		*/
	u_short sii_clock;	/* Diagnostic Clock Register		*/
	u_short sii_bhdiag;	/* Bus Handler Diagnostic Register	*/
	u_short sii_sidiag;	/* SCSI IO Diagnostic Register		*/
	u_short sii_dmdiag;	/* Data Mover Diagnostic Register	*/
	u_short sii_mcdiag;	/* Main Control Diagnostic Register	*/
	u_short kzq_dmacsr;	/* DMA Control/Status Reg. for KZQSA	*/
	u_short kzq_qbar;	/* Qbus Address (high 5 bits in dmacsr) */
	u_short kzq_lbar;	/* Local Address for Qbus DMA.		*/
	u_short kzq_wc;		/* Word Count for DMA.			*/
	u_short kzq_vector;	/* Intr vector & 128k memory base.	*/
};
#define KZQ_REG                register volatile struct kzq_regs

#define KZQ_WAIT_COUNT		10000   /* Delay count used for the SII chip  */
#define KZQ_MAX_DMA_XFER_LENGTH	8192  	/* Max DMA transfer length for SII    */
#define QVEC_BCNT    0x80    /* The number of vectors possible       */
