
/*
 *	7/2/90	(ULTRIX)	@(#)5380reg.h	4.1
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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

 /***********************************************************************
  * 5380reg.h	16-Aug-88
  *
  * Modification History
 *
 *  16-Oct-88 -- Fred Canter
 *	Clean up comments.
  *
  * 28-Sep-88 -- Fred Canter
  *	Clean up comments.
  *
  * 16-Aug-88 -- Fred Canter
  *	Created this header file for scsi.c from scsireg.h.
  *	Part of merging 5380 and SII SCSI drivers.
  *
  ***********************************************************************/


/*
 * SCSI controller (NCR 5380 chip) registers.
 * Must be padded to a length of 0x100 bytes
 * to allow access to the 2nd set of SCSI registers.
 */
struct	sz_regs {
	u_long	sz_pad0[32];		/* Page alignment */
	u_char	sz_scs_out_data;	/* (wo) Output data register */
#define	sz_scs_cur_data	sz_scs_out_data	/* (ro) Current data register */
	u_char	sz_pad1[3];
	u_char	sz_scs_ini_cmd;		/* (r/w) Initiator command register */
	u_char	sz_pad2[3];
	u_char	sz_scs_mode;		/* (r/w) Mode register */
	u_char	sz_pad3[3];
	u_char	sz_scs_tar_cmd;		/* (r/w) Target command register */
	u_char	sz_pad4[3];
	u_char	sz_cur_stat;		/* (ro) Current bus status register */
#define	sz_scs_sel_ena	sz_scs_cur_stat	/* (wo) Select enable register */
	u_char	sz_pad5[3];
	u_char	sz_scs_status;		/* (ro) Bus and status register */
#define	sz_scs_dma_send	sz_scs_status	/* (wo) Start DMA send action */
	u_char	sz_pad6[3];
	u_char	sz_scs_in_data;		/* (ro) Input data regsiter */
#define	sz_scs_data_trcv sz_scs_in_data	/* (wo) Start DMA target rcv action */
	u_char	sz_pad7[3];
	u_char	sz_scs_dma_ircv;	/* (wo) Start DMA initiator rcv actn */
#define	sz_scs_reset sz_scs_dma_ircv	/* (ro) Reset interrupt/error action */
	u_char	sz_pad8[3];
	u_long	sz_scd_adr;		/* (r/w) DMA address register */
	u_long	sz_pad9[7];
	u_long	sz_scd_cnt;		/* (r/w) DMA byte count register */
	u_char	sz_scd_dir;		/* (wo) DMA transfer direction */
	u_char	sz_pad10[3];
	u_long	sz_pad11[14];
};

/*
 * Shorten SCSI register names.
 */
#define	scs_outdata	sz_scs_out_data		/* output data register	      */
#define scs_curdata	sz_scs_cur_data		/* current data register      */
#define scs_inicmd	sz_scs_ini_cmd		/* initiator cmd reg	      */
#define scs_mode	sz_scs_mode		/* mode register	      */
#define scs_tarcmd	sz_scs_tar_cmd		/* target command register    */
#define scs_selena	sz_cur_stat		/* select enable register     */
#define scs_curstat	sz_cur_stat		/* current status register    */
#define scs_dmasend	sz_scs_status		/* start DMA send action      */
#define scs_status	sz_scs_status		/* bus and status register    */
#define scs_dmatrcv	sz_scs_in_data		/* start DMA targ rec action  */
#define scs_indata	sz_scs_in_data		/* input data register	      */
#define scs_dmaircv	sz_scs_dma_ircv		/* start DMA initiator rcv act*/
#define scs_reset	sz_scs_dma_ircv		/* reset int/err action	      */
#define scd_adr		sz_scd_adr		/* DMA address register	      */
#define scd_cnt		sz_scd_cnt		/* DMA byte count register    */
#define	scd_dir		sz_scd_dir		/* DMA transfer direction     */

/*
 * SCSI controller registers are on longword boundries,
 * but only the low byte is used. The high 24 bits must
 * be stripped off.
 * It appears this mask is not used.
 */
#define SCS_REG_MASK	0x000000ff	/* strip off top 24 bits */
#define SCS_MASK(a)	(a &= SCSI_REG_MASK)

/*
 * SCSI controller bit for VS3100
 * interrupt controller msk/req/clr registers.
 * Controller 0 uses the TZK50 controller's bit.
 * Controller 1 uses the ST506 disk controller's bit.
 * Algorithm is (SCS_IC_BIT >> cntlr)
 */
#define	SCS_IC_BIT	0x02

/*
 * Mode Register bits  (SCS_MODE)
 */
#define	SCS_BLOCK	0x80	/* DMA block mode - Must Be Zero	      */
#define SCS_TARG	0x40	/* target role */
#define SCS_PARCK	0x20	/* parity check enable */
#define SCS_INTPAR	0x10	/* interrupt on parity error */
#define SCS_INTEOP	0x08	/* interrupt on end of DMA */
#define SCS_MONBSY	0x04	/* monitor BSY */
#define SCS_DMA		0x02	/* enable DMA transfer */
#define SCS_ARB		0x01	/* start arbitration */

/*
 * Initiator Command Register  (SCS_INI_CMD)
 */
#define	SCS_INI_RST	0x80	/* assert/deassert RST on SCSI bus */
#define SCS_INI_AIP	0x40	/* arbitration in process - RO */
#define SCS_INI_TEST	0x40	/* test mode - WO, MBZ */
#define SCS_INI_LA	0x20	/* lost arbitration - RO */
#define SCS_INI_DIFF	0x20	/* differential enable - WO, MBZ */
#define SCS_INI_ACK	0x10	/* assert/deassert ACK on SCSI bus*/
#define SCS_INI_BSY	0x08	/* assert/deassert BSY on SCSI bus*/
#define SCS_INI_SEL	0x04	/* assert/deassert SEL on SCSI bus*/
#define SCS_INI_ATN	0x02	/* assert/deassert ATN on SCSI bus*/
#define SCS_INI_ENOUT	0x01	/* enable/disable output */

/*
 * Target Command Register  (SCS_TAR_CMD)
 */

/*
 * Bus and Status Register (SCS_STATUS)
 */
#define SCS_DMAEND	0x80	/* DMA end */
#define SCS_DMAREQ	0x40	/* DMA request */
#define SCS_PARERR	0x20	/* parity error */
#define SCS_INTREQ	0x10	/* interrupt request */
#define SCS_MATCH	0x08	/* phase match */
#define SCS_BSYERR	0x04	/* busy error */
#define SCS_ATN		0x02	/* attention condition on SCSI bus */
#define SCS_ACK		0x01	/* acknowledge - REQ/ACK transfer handshake */

/*
 * Current Bus Status Register  (SCS_CUR_STAT)
 */
#define SCS_RST		0x80	/* reset condition on SCSI bus		      */
#define SCS_BSY		0x40	/* busy -- SCSI bus being used		      */
#define SCS_REQ		0x20	/* request portion of REQ/ACK handshake	      */
#define SCS_MESSI	0x1c	/* Message In Phase			      */
#define SCS_MESSO	0x18	/* Message Out Phase			      */
#define SCS_STATUS	0x0c	/* Status Phase				      */
#define SCS_CMD		0x08	/* Command Phase			      */
#define SCS_DATAI	0x04	/* Data In Phase			      */
#define SCS_IO		0x04	/* I/O					      */
#define SCS_SEL		0x02	/* select a target or reselect an initiator   */
#define SCS_DBP		0x01	/* data parity				      */
#define SCS_DATAO	0x00	/* Data Out Phase			      */
#define SCS_PHA_MSK	0x1c	/* Mask out the Phase Bits		      */

/*
 * Select Enable  (SCS_SEL_ENA)
 * NOTE: these definitions are no longer used.
 */
#define SCS_ID7		0x80	/* SCSI ID 7 */
#define SCS_ID6		0x40	/* SCSI ID 6 */
#define SCS_ID5		0x20	/* SCSI ID 5 */
#define SCS_ID4		0x10	/* SCSI ID 4 */
#define SCS_ID3		0x08	/* SCSI ID 3 */
#define SCS_ID2		0x04	/* SCSI ID 2 */
#define SCS_ID1		0x02	/* SCSI ID 1 */
#define SCS_ID0		0x01	/* SCSI ID 0 */

/*
 * DMA Direction Register  (SCD_DIR)
 */
#define	SCD_DMA_IN	0x01	/* DMA in from device to 16K buffer */
#define SCD_DMA_OUT	0x00	/* DMA out from 16K buffer to device */
