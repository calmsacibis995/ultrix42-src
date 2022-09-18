
/*
 *	7/2/90	(ULTRIX)	@(#)stcreg.h	4.1
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988, 1989 by		*
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
  * stcreg.h	4-Jun-86
  *
  * Modification History
  *
  * 25-Apr-89	fred (Fred Canter)
  *	Added ST_COPY_DONE to sc_stflags as part of fix for Rev 45
  *	TZK50 firmware breaking the driver.
  *
  * 06-Jun-88	fred (Fred Canter)
  *	Changes for PVAX extended I/O mode.
  *
  * 12-Jan-88	darrell
  *	Added sc_ticks field to st_softc structure as a rewrite 
  *	of st_timer().
  *
  * 15-Sep-87	darrell
  *	Added ST_RETD_KEEP constant definition for sc_stflags.
  *
  * 20-Aug-87	darrell
  *	Added fields to st_softc to support st_timer().
  *
  * 03-Mar-87	darrell
  *	Corrected an incorrect comment
  *
  * 15-Jan-87	darrell
  *	Changed definitions and structures to support the rewrite of
  *	stc.c that is being submitted at this same time.
  *
  * 4-Sep-86	darrell
  *	Added a new structure (st_stateinfo) and more constants.
  *
  * 4-Sep-86	darrell
  *	First semi-working driver.
  *
  * 5-Aug-86	darrell
  *	Update for first pass of real driver (compiles but not debugged).
  *
  * 4-Jun-86	darrell - Creation of this file
  *
  ***********************************************************************/


/*
 * SCSI controller registers for VAXSTAR and TEAMMATE
 *
 * Shorten the register names.
 */
#define	scs_outdata	nb_scs_out_data		/* output data register	      */
#define scs_curdata	nb_scs_cur_data		/* current data register      */
#define scs_inicmd	nb_scs_ini_cmd		/* initiator cmd reg	      */
#define scs_mode	nb_scs_mode		/* mode register	      */
#define scs_tarcmd	nb_scs_tar_cmd		/* target command register    */
#define scs_selena	nb_cur_stat		/* select enable register     */
#define scs_curstat	nb_cur_stat		/* current status register    */
#define scs_dmasend	nb_scs_status		/* start DMA send action      */
#define scs_status	nb_scs_status		/* bus and status register    */
#define scs_dmatrcv	nb_scs_in_data		/* start DMA targ rec action  */
#define scs_indata	nb_scs_in_data		/* input data register	      */
#define scs_dmaircv	nb_scs_dma_ircv		/* start DMA initiator rcv act*/
#define scs_reset	nb_scs_dma_ircv		/* reset int/err action	      */
#define scd_adr		nb_scd_adr		/* DMA address register	      */
#define scd_cnt		nb_scd_cnt.w[0]		/* DMA byte count register    */
#define	scd_dir		nb_scd_dir		/* DMA transfer direction     */

/*
 * SCSI controller registers are on longword boundries,
 * but only the low byte is used. The high 24 bits must
 * be stripped off.
 */
#define SCS_REG_MASK	0x000000ff	/* strip off top 24 bits */
#define SCS_MASK(a)	(a &= SCSI_REG_MASK)

/*
 * SCSI bit for interrupt registers
 */
#define SCS_INT_TAPE	0x02	/* SCSI interrupt enable/clear bit */

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

/*
 * sc_timer
 */
#define ST_TIMERON	0x100	/* The watch dog timer is on		      */
/*
 * Flags  (sc_stflags)
 */
#define	ST_NORMAL	0x00	/* Normal				      */
#define ST_NEED_SENSE	0x01	/* Need to do a Request Sense command	      */
#define ST_REPOSITION	0x02	/* Need to reposition the tape		      */
#define ST_ENCR_ERR	0x04	/* Encountered an error			      */
#define ST_DID_DMA	0x08	/* A DMA operation has been done	      */
#define ST_WAS_DISCON	0x10	/* Disconnect occured during this command     */
#define ST_MAPPED	0x20	/* Virutal transfer buffer for read or write  */
#define ST_NODEVICE	0x40	/* No tape device present		      */
#define ST_RETD_KEEP	0x80	/* VS_KEEP was returned to vs_bufctl	      */
#define	ST_COPY_DONE	0x100	/* Data copy done by SDP don't need R_COPY    */

/*
 * Driver and data specific structure
 */
struct	st_softc {
	char	sc_openf;		/* Lock against multiple opens	      */
	short	sc_resid;		/* Copy of last bc		      */
	daddr_t sc_blkno;		/* Block number 		      */
	daddr_t sc_nxrec;		/* Position of end of tape	      */
	struct	st_softc *sc_ubaddr;	/* Bus address of ts_softc	      */
	u_short sc_uba; 		/* Bus addr. of cmd. pkt.(tsdb)       */
	short	sc_mapped;		/* Is ts_softc bus mapped ?	      */
	long	sc_flags;		/* Flags			      */
	long	sc_category_flags;	/* Category flags		      */
	u_long	sc_softcnt;		/* Soft error count total	      */
	u_long	sc_hardcnt;		/* Hard error count total	      */
	char	sc_device[DEV_SIZE];	/* Device type string		      */
	long	sc_stflags;		/* flags for reuesting other action   */
	char	sc_curcmd;		/* the current drive command	      */
	char	sc_prevpha;		/* the previous bus phase	      */
	char	sc_selstat;		/* Whether selected or disconnected   */
	char	sc_xstate;		/* State for st_start state machine   */
	char	sc_xevent;		/* Event for st_start state mancine   */
	char	sc_fstate;		/* State for st_fuzzy state machine   */
	char	sc_fevent;		/* Event for st_fuzzy state machine   */
	short	sc_bcount;		/* Byte count of last DMA transfer    */
	short	sc_savcnt;		/* Bytes remaining in transfer when a */
					/* disconnect occurs		      */
	long	sc_savstv;		/* begining address in the 16K buffer */
	long	sc_savbufp;		/* Saved virtual address of where to  */
					/* put the data read from the tape    */
	union {
	    struct st_cmdfmt st_cmd;	/* Complete command packet	      */
	    struct {
		char cmd[6];		/* Command portion of comand packet   */
		char dat[16];		/* Data portion of command packet     */
	    }altcmd;
	}sc_cmdpkt;			/* Command packet		      */
	struct	st_datfmt st_dat;	/* Return status data		      */
	u_char	sc_status;
	u_char	sc_message;
	int	sc_ticks;		/* number of times timer has gone off */
	int	sc_timer;		/* state of watch dog timer	      */
	int	sc_rate;		/* Rate the watch dog timer runs      */
	struct	timeval sc_progress;	/* last time progress occurred	      */
	struct	st_exsns_dt sc_sns;	/* extended sense data		      */
};
/*
 * st_softc names shortened
 */
#define sc_cmd		sc_cmdpkt.altcmd.cmd		/* Cmd part of cmd pkt*/
#define sc_dat		sc_cmdpkt.altcmd.dat		/* Dat part of cmd pkt*/
#define st_command	sc_cmdpkt.st_cmd		/* Command Packet     */
#define st_opcode	sc_cmdpkt.st_cmd.opcode		/* Command Opcode     */
#define	st_tur		sc_cmdpkt.st_cmd.cmd.tur	/* TEST UNIT READY    */
#define st_rwd		sc_cmdpkt.st_cmd.cmd.rwd	/* REWIND Comman      */
#define st_rqsns	sc_cmdpkt.st_cmd.cmd.sense	/* REQUEST SENSE      */
#define st_rbl		sc_cmdpkt.st_cmd.cmd.rbl	/* REQUEST BLOCK LMTS */
#define	st_read		sc_cmdpkt.st_cmd.cmd.rw		/* READ Command	      */
#define st_write	sc_cmdpkt.st_cmd.cmd.rw		/* WRITE Command      */
#define st_trksel	sc_cmdpkt.st_cmd.cmd.trksel	/* TRACK SELECT       */
#define st_resunit	sc_cmdpkt.st_cmd.cmd.runit	/* RESERVE UNIT       */
#define st_wfm		sc_cmdpkt.st_cmd.cmd.wfm	/* WRITE FILEMARKS    */
#define st_space	sc_cmdpkt.st_cmd.cmd.space	/* SPACE Command      */
#define st_inq		sc_cmdpkt.st_cmd.cmd.inq	/* INQUIRY Command    */
#define st_vfy		sc_cmdpkt.st_cmd.cmd.vfy	/* VERIFY Command     */
#define st_rbd		sc_cmdpkt.st_cmd.cmd.rw		/* RCVR BUFFERED DATA */
#define st_modsel	sc_cmdpkt.st_cmd.cmd.modsel	/* MODE SELECT	      */
#define st_relunit	sc_cmdpkt.st_cmd.cmd.runit	/* RELEASE UNIT	      */
#define st_erase	sc_cmdpkt.st_cmd.cmd.erase	/* ERASE Command      */
#define st_modsns	sc_cmdpkt.st_cmd.cmd.sense	/* MODE SENSE Command */
#define st_load		sc_cmdpkt.st_cmd.cmd.ld		/* LOAD Command	      */
#define st_unload	sc_cmdpkt.st_cmd.cmd.ld		/* UNLOAD Command     */
#define st_recdiag	sc_cmdpkt.st_cmd.cmd.recdiag	/* RECEIVE DIAG RESULT*/
#define st_snddiag	sc_cmdpkt.st_cmd.cmd.diag	/* SEND DIAGNOSTIC    */
/*
 * Values for sc_selstat
 */
#define ST_IDLE		 0	/* The device is not selected (BUS Free)      */
#define ST_SELECT	 1	/* The device is selected		      */
#define	ST_DISCONN	 2	/* The device has disconnected		      */
#define ST_RESELECT	 3	/* The device is in the reselection process   */
/*
 * State Machine Events
 */
#define ST_CONT		 0	/* Continue wherever processing left off      */
#define	ST_BEGIN	 1	/* BEGIN processing requests from the queue   */
#define ST_DMA_DONE	 2	/* DMA count to zero interrupt		      */
#define ST_PAR_ERR	 3	/* Parity Error interrupt		      */
#define ST_PHA_MIS	 4	/* Phase Mismatch interrupt		      */
#define ST_RESET	 5	/* RST interrupt			      */
#define ST_CMD		 6	/* In Command Mode (Status/Positioning Cmd)   */
#define ST_DMA		 7	/* Data transfer using DMA		      */
#define ST_ABRT		 8	/* Abort the fuzzy transfer		      */
#define ST_ERROR	 9	/* An error event occured		      */
#define	ST_TIMEOUT	10	/* Timer expired			      */
#define ST_FREEB	11	/* Bus needs to be freed		      */
/*
 * Return Status from various routines 
 */
#define	ST_SUCCESS	0	/* Success				      */
#define ST_IP		1	/* In Progress				      */
#define ST_RET_ERR	2	/* Error condition occured		      */
#define ST_DISCONNECT	3	/* Phase Error				      */
#define ST_RETRY	4	/* The command failed, retries may succeed    */
#define ST_FATAL	5	/* The command failed, retries will fail      */
