#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifndef	SCSIREG_INCLUDE
#define	SCSIREG_INCLUDE	1

/*
 *	1/3/91	(ULTRIX)	@(#)scsireg.h	4.6
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
  * scsireg.h	6-Jun-88
  *
  * Modification History
  *
  *  11/20/90	Robin Miller
  *	Added defines for 10-byte READ/WRITE CDB's.
  *
  *  09/13/90	Robin Miller
  *	Added additional QIC density codes.
  *
  *  07/31/90	Robin Miller
  *	Added defines for RRD42 CDROM, RX26 diskette, & RZ25 disk.
  *
  * 07/16/90    Janet L. Schank
  *     Added a define for RZ23L.
  *
  * 06/07/90	Bill Dallas
  *	Added the generic option tables that are linked to devtab. These
  *	allow easy additions of devices. 
  * 
  *  23-Jan-90  Janet L. Schank
  *     Added define for TLZ04 (RDAT).
  *
  *  13-Nov-89  Janet L. Schank / Art Zemon
  *     Added TZ05, RZ24, and RZ57 support.
  *
  *  08-Oct-89	Fred Canter
  *	Document scsi_devtab flags with comments.
  *
  *  07-Oct-89	Fred Canter
  *	Save removable media bit in sz_softc structure.
  *
  *  04-Oct-89	Fred Canter
  *	Added sector number to error log packet for disk errors.
  *	Bump SCSI error log packet version number up to 2.
  *
  *  18-Aug-89  Janet L. Schank
  *     Added a define for the size of sz_timetable, SZ_TIMETBL_SZ
  *
  *  23-Jul-89	Fred Canter
  *	Convert DBBR printfs to error log calls.
  *	RX33 support.
  *
  *  22-Jul-89	Fred Canter
  *	Make SCSI error log version number first in error log packet.
  *
  *  18-Jul-89 	John A. Gallant
  *	Added the SZ_NODBBR flag definition for disabling DBBR.
  *
  *  14-Jul-89	Fred Canter
  *	Merged dynamic BBR and error log changes.
  *
  *  23-Jun-89 	John A. Gallant
  *	Added defines for read/write long
  *	Added variables for the BBR state machine.
  *	Added the function pointer for device driver completion routine.
  *
  *  22-Jun-89	Fred Canter
  *	Save a copy of current CDB for error log.
  *	Save a copy of the status byte for the current command for error log.
  *
  *  20-Jun-89	Fred Canter
  *	Added RZ56 support.
  *
  *  18-Jun-89	Fred Canter
  *	Convert to scsi_devtab.
  *
  *  17-Jun-89	Fred Canter
  *	Added data structures and definitions needed for SCSI error logging.
  *
  * 11-Jun-89	Fred Canter
  *	Added media changed counter to sz_softc. Softpc floppy hooks.
  *	Added additional sense code (sc_c_asc) to sz_softc.
  *
  *  24-Apr-89	Fred Canter
  *	Defined RX23s SCSI floppy.
  *
  *  06-Apr-89	Fred Canter
  *	Added sc_pxstate[] to sz_softc and #defines to support a
  *	target returning busy status (for TZxx).
  *
  *	Added b_comand to replace b_command for local command buffers.
  *	Use b_gid instead of b_resid to store command.
  *
  *	Added sc_tz_modsel to sz_softc for TZxx extended mode select.
  *
  *  08-Feb-89	Fred Canter
  *	Changed sc_sel_retry to array so select timeout retry count
  *	now on a per target basis.
  *
  *  03-Dec-88	Fred Canter
  *	Define number of retries for data xfer and other commands.
  *	Part of retry hardware error sense key modification.
  *
  *  03-Nov-88	Alan Frechette
  *	Added in support for disk maintainence. Added defines for 
  *	the commands FORMAT UNIT, REASSIGN BLOCK, READ DEFECT DATA
  *	and VERIFY DATA. Added fields to the "softc" structure for
  *	handling these special disk commands.
  *
  *	Added 5 additional flags to the "sc->sc_szflags" field for
  *	the SII.
  *
  *  16-Oct-88 -- Fred Canter
  *	Clean up comments. Moved SZDEBUG define from .c files to scsireg.h.
  *	Added RZxx for non-DEC disks. Added #ifdefs around spin stats
  *	variables in sz_softc structure.
  *
  *  28-Sep-88 -- Fred Canter
  *	Clean up comments.
  *
  *  29-Aug-88 -- Alan Frechette
  *	Added Asynchronous/Synchronous support to the SII. Added a 
  *	new field "sc_siireqack" to the softc structure. Also changed
  *	the name of some of the SII specific fields.
  *
  *  21-Aug-88 -- Fred Canter
  *	Improved device failed to select error handling.
  *	Added sc_rip (reset in progress) to sz_softc structure.
  *
  *  16-Aug-88 -- Fred Canter
  *	Merge 5380 and SII SCSI drivers.
  *	Moved 5380 chip specific register definitions to 5380reg.h.
  *
  *  08-Aug-88 -- Fred Canter
  *	Modify sz_softc for changes in scsi.c.
  *
  *  28-Jul-88 -- Fred Canter
  *	Clean up sz_flags definitions. Remove old background timer
  *	variables from sz_softc. Remove some debug variables from sz_softc.
  *
  *  18-Jul-88 -- Fred Canter
  *	Added controller alive to sz_softc.
  *
  *  16-Jul-88 -- Fred Canter
  *	Added disconnect timer, phase spin loop debug code,
  *	and save default partition table pointer in sz_softc.
  *
  * 28-Jun-88 -- Fred Canter
  *	Modified softc structure for improved ??command() error handling
  *	and background timer to catch resleect timeouts.
  *
  * 18-Jun-88 -- Fred Canter
  *	Added RZ55 support and a new flag for bus busy wait.
  *
  * 06-Jun-88 -- Fred Canter
  *	Created this header file for scsi.c from stcreg.h.
  *
  ***********************************************************************/


/*
 * Defining SZDEBUG enables a ton of debug printouts and
 * a debug variable (szdebug) to control which debug
 * messages get printed. This code is not of much use
 * in a running system because it floods the system with
 * printouts. Defining SZDEBUG also changes the timing
 * even if szdebug is set to zero. The SCSI bus analyzer
 * is a much better tool for debugging driver problems.
 */
/* #define	SZDEBUG */

/*
 * DMA Direction Register  (SCD_DIR)
 */
#define SZ_DMA_READ	0x0	/* A DMA read transfer is active */
#define SZ_DMA_WRITE	0x1	/* A DMA write transfer is active */

/*
 * The number of entries in sz_timetable.
 */
#define SZ_TIMETBL_SZ   0x40

/*
 * Flags  (sc_szflags)
 */
#define	SZ_NORMAL	0x000	/* Normal (no szflags set)		      */
#define SZ_NEED_SENSE	0x001	/* Need to do a Request Sense command	      */
#define SZ_REPOSITION	0x002	/* Need to reposition the tape	 (NOT USED)   */
#define SZ_ENCR_ERR	0x004	/* Encountered an error			      */
#define SZ_DID_DMA	0x008	/* A DMA operation has been done	      */
#define SZ_WAS_DISCON	0x010	/* Disconnect occured during this command     */
#define SZ_NODEVICE	0x020	/* No SCSI device present		      */
#define	SZ_BUSYBUS	0x040	/* Bus is busy, don't start next command      */
#define	SZ_RSTIMEOUT	0x080	/* Disconnected command to a disk timed out   */
				/* (force retry even if sense data is good).  */
#define	SZ_TIMERON	0x100	/* Disconnect being timed by timeout() call   */
#define	SZ_DID_STATUS	0x200	/* Status phase occurred (only used for debug)*/
#define SZ_DMA_DISCON	0x400	/* DMA was interrupted by a disconnect        */
#define	SZ_SELWAIT	0x800	/* Waiting for 250 MS select timeout	      */
#define SZ_ASSERT_ATN	0x1000	/* Need to assert the ATN signal 	      */
#define SZ_REJECT_MSG	0x2000	/* Need to reject a message 		      */
#define SZ_RESET_DEV	0x4000	/* Need to reset a scsi device                */
#define SZ_ABORT_CMD	0x8000	/* Need to abort the current command          */
#define SZ_RETRY_CMD	0x10000	/* Need to retry the current command          */
#define	SZ_BUSYTARG	0x20000	/* Target is busy, retry command later        */
#define	SZ_RECOVERED	0x40000	/* A recovered error occured, used w/BBR      */

/*
 * Maximum number of SCSI bus IDs (targets and initiator).
 * Also defines maximum number of logical units per SCSI controller.
 * Use to allocate/access all unit and target ID data structures.
 * MUST BE 8 - DO NOT CHANGE.
 */
#define	NDPS	8

/*
 * SCSI supported device class/type IDs
 */
#define	SZ_TAPE		0x80000000	/* TAPE device */
#define	SZ_DISK		0x40000000	/* DISK device */
#define	SZ_CDROM	0x20000000	/* CDROM device */
#define	SZ_UNKNOWN	0x10000000	/* UNKNOWN/UNSUPPORTED device */
#ifdef __mips
#define SZ_DEVMASK	0xf0000000	/* See if any of these */
#endif /* __mips */

#define	TZ30		0x80000001	/* TZ30 cartridge tape */
#define	TZK50		0x80000002	/* TZK50 cartridge tape */
#define	TZxx		0x80000004	/* TZxx non-DEC tape (may[not] work) */
#define TZ05		0x80001000	/* css tz05 */
#define TZ07		0x80001001	/* css tz07 */
#define	TZ9TRK		0x80001800	/* Generic non-DEC 9trk tape 
						(may[not] work) */
#define TLZ04           0x80002000      /* TLZ04 (RDAT) tape drive */
#define TZRDAT		0x80002800	/* Generic Rdat tape (may[not] work) */
#define TZK10		0x80004000	/* TZK10 (Qic) tape drive  */
#define	TZQIC		0x80004800	/* Generic non-DEC QIC tape 
						(may[not] work)	*/
#define TZK08		0x80008000	/* Exabyte TZK08 */	
#define TZ8MM		0x80008800	/* Generic non-DEC 8mm tape 
						(may[not] work)	*/

#define	RZ22		0x40000008	/* RZ22  40 MB winchester disk */
#define	RZ23		0x40000010	/* RZ23 100 MB winchester disk */
#define	RZ55		0x40000020	/* RZ55 300+ MB winchester disk */
#define	RZ56		0x40000040	/* RZ56 600+ MB winchester disk */
#define	RX23		0x40000080	/* RX23 3.5" 1.4MB SCSI floppy disk */
#define	RX33		0x40000100	/* RX33 5.25" 1.2MB SCSI floppy disk */
#define	RZxx		0x40000200	/* RZxx non-DEC disk (may[not] work) */
#define RZ24            0x40000400      /* RZ24 winchester disk */
#define RZ57            0x40000800      /* RZ57 winchester disk */
#define RZ23L           0x40001000      /* RZ23L 116Mb winchester disk */
#define RX26		0x40002000	/* RX26 3.5" 2.8MB SCSI floppy disk */
#define RZ25		0x40004000	/* RZ25 winchester disk */

#define	RRD40		0x20000400	/* RRD40 CDROM (optical disk) */
#define	CDxx		0x20000800	/* CDxx non-DEC CDROM (may[not] work)*/
#define RRD42		0x20001000	/* RRD42 CDROM (optical disk) */

/*
 * Driver and data specific structure
 *
 * The sz_softc structure is mostly longword alligned,
 * for readability when accessing it with ADB.
 */
#define	SZ_DNSIZE	SZ_VID_LEN+SZ_PID_LEN	/* ascii device name size */

/*
 * Turn on code which measures the amount of time spent
 * spinning in szintr and scsistart waiting for phase changes.
 */
/* #define	SPIN_STATS */

struct	sz_softc {
	int	sc_sysid;		/* SCSI bus ID of system (initiator)  */
	int	sc_cntlr_alive;		/* Status of this cntlr (alive or not)*/
	int	sc_aipfts;		/* scsistart: # of AIP failed to set  */
	int	sc_lostarb;		/* # of times CPU lost arbitration    */
	int	sc_lastid;		/* ID of last target I/O started on   */
	int	sc_active;		/* Current selected target ID (0=none)*/
	int	sc_prevpha;		/* Previous bus phase		      */
	int	sc_fstate;		/* State for sz_fuzzy state machine   */
	int	(*port_start)();	/* pointer to the port start routine  */
	int	(*port_reset)();	/* pointer to the port reset routine  */
	char    *sc_rambuff;		/* Holds pointer to the RAM buffer    */
	int	sc_swcount;		/* Select timeout wait counter	      */
	int	sc_rip;			/* SCSI bus reset in progress	      */
	int	sc_scs_selena;		/* Soft copy, 5380 select enable reg  */
	int	sc_rmv_media;		/* Save removable media bit per target*/
	int	sc_dummy;		/* Align sz_softc for ADB	      */
	int	(*device_comp[NDPS])();	/* pointer to device completion func  */
	int	sc_sel_retry[NDPS];	/* Select failure retry count	      */
	struct scsi_devtab *sc_devtab[NDPS];  /* Target's scsi_devtab pointer */
#ifdef	SPIN_STATS
	int	sc_i_spin1[NDPS];	/* szintr: longest spin count	      */
	int	sc_i_spcmd[NDPS];	/*	   current command	      */
	int	sc_i_phase[NDPS];	/*	   phase entered after spin   */
	int	sc_ss_spin1[NDPS];	/* scsistart: longest spin count      */
	int	sc_ss_spcmd[NDPS];	/*	      current command	      */
	int	sc_ss_phase[NDPS];	/*	      phase entered after spin*/
#endif /*	SPIN_STATS */
#ifdef	DCT_STATS
	int	sc_dcstart[NDPS];	/* Time target disconnected	      */
	int	sc_dcend[NDPS];		/* Time target disconnect ended	      */
	int	sc_dcdiff[NDPS];	/* How long last disconnect lasted    */
	int	sc_dclongest[NDPS];	/* Longest time target disconnected   */
#endif /*	DCT_STATS */
	struct buf *sc_bp[NDPS];	/* Saved buffer pointer		      */
	int	sc_b_bcount[NDPS];	/* part of bp->b_bcount for this xfer */
	int	sc_bpcount[NDPS];	/* Xfer size, not always bp->b_bcount */
	int	sc_segcnt[NDPS];	/* Max byte count for xfer (segment)  */
	int	sc_xfercnt[NDPS];	/* Number of bytes transfered so far  */
	int	sc_resid[NDPS];		/* Copy of last bc		      */
	int	sc_savcnt[NDPS];	/* Bytes remaining in transfer when a */
	daddr_t	sc_blkno[NDPS];		/* Starting block number of xfer      */
	int	sc_openf[NDPS];		/* Lock against multiple opens	      */
	daddr_t	sc_disksize[NDPS];	/* DISK: number of LBNs on disk	      */
	struct size *sc_dstp[NDPS];	/* Pointer to default psrtition sizes */
	long	sc_flags[NDPS];		/* Flags			      */
	long	sc_category_flags[NDPS];/* Category flags		      */
	u_long	sc_softcnt[NDPS];	/* Soft error count total	      */
	u_long	sc_hardcnt[NDPS];	/* Hard error count total	      */
	int	sc_devtyp[NDPS];	/* Device class/type ID		      */
	int	sc_dkn[NDPS];		/* Saved DK number for iostat	      */
	int	sc_alive[NDPS];		/* Is a device at this SCSI target ID */
	int	sc_unit[NDPS];		/* Logical unit number for this ID    */
	char	sc_device[NDPS][DEV_SIZE]; /* Device type string,ADB 64 bytes */
	long	sc_szflags[NDPS];	/* Flags for reuesting other action   */
	int	sc_curcmd[NDPS];	/* Current cmd, eg: TUR, MODSEL, R/W  */
	int	sc_actcmd[NDPS];	/* Actual cmd, eg: RQSNS after R/W    */
	int	sc_selstat[NDPS];	/* Cntlr state: SEL RESEL DISCON IDLE */
	int	sc_xstate[NDPS];	/* State for sz_start state machine   */
	int	sc_xevent[NDPS];	/* Event for sz_start state machine   */
	int	sc_pxstate[NDPS];	/* Save state in case target busy     */
	int	sc_c_status[NDPS];	/* Status of last ??command()	      */
	int	sc_c_snskey[NDPS];	/* Sense Key for last ??command()     */
	int	sc_c_asc[NDPS];		/* Additional Sense code (disks only) */
	char	*sc_SZ_bufmap[NDPS];	/* Virtual address for buffer mapping */
	struct	pte *sc_szbufmap[NDPS];	/* PTEs from get_sys_ptes() call      */
	char	*sc_bufp[NDPS];		/* Pointer to user buffer	      */
	long	sc_dboff[NDPS];		/* Target's offset in 128K h/w buffer */
	u_char	sc_cmdlog[NDPS][12];	/* Copy of current command (cdb)      */
	union {				/* ADB: size is 22 bytes	      */
	    struct sz_cmdfmt sz_cmd;	/* Complete command packet	      */
	    struct {
		char cmd[6];		/* Command portion of comand packet   */
		char dat[16];		/* Data portion of command packet     */
	    } altcmd;
	} sc_cmdpkt[NDPS];		/* Command packet		      */
	struct	sz_datfmt sz_dat[NDPS];	/* Return status data, ADB size 44b   */
	u_char	sc_status[NDPS];	/* Status for current command	      */
	u_char	sc_statlog[NDPS];	/* Copy of status byte for error log  */
	u_char	sc_message[NDPS];	/* Current message		      */
	struct	sz_exsns_dt sc_sns[NDPS]; /* extended sense data,ADB size 44b */
	char	sc_devnam[NDPS][SZ_DNSIZE];	/* ASCII device name  (8*24b) */
	char	sc_revlvl[NDPS][SZ_REV_LEN];	/* ASCII dev rev level (8*4b) */
	u_char	sc_extmessg[NDPS][5];	/* For extended messages              */
	u_char	sc_siioddbyte[NDPS];	/* Used for SII to hold the ODD BYTE  */
	int	sc_siireqack[NDPS];    	/* The req/ack offset for each target */
	int	sc_siisentsync[NDPS];	/* Tells if Sync DataXfer Messg sent  */
	int	sc_siidmacount[NDPS];	/* Used for SII for I/O transfers >8K */
	long	sc_siidboff[NDPS];	/* Special RAM buffer offsets for SII */
	int	sc_rzspecial[NDPS];	/* Used for the "rzdisk" utility      */
	char	*sc_rzparams[NDPS];	/* Used for the "rzdisk" utility      */
	char	*sc_rzaddr[NDPS];	/* Used for the "rzdisk" utility      */
	struct  sz_exsns_dt sc_rzsns[NDPS];/* Used for the "rzdisk" utility   */
	u_short sc_mc_cnt[NDPS];	/* Floppy media changed counter	      */
	struct	timeval sc_progress;	/* last time progress occurred	      */
	long	targ_lun[NDPS];		/* target lun global (for ECRM)       */
					/* TODO: sc_progress set but not used */
    /* Parameters for the BBR code */

	int	sc_bbr_active[NDPS];	/* Active flag for BBR state machine  */
	int	sc_bbr_state[NDPS];	/* State for bbr state machine        */
	int	sc_bbr_oper[NDPS];	/* Current operation for bbr          */
	int	sc_bbr_read[NDPS];	/* Read counts                        */
	int	sc_bbr_rawr[NDPS];	/* Reassign/write counts              */
	int	sc_bbr_write[NDPS];	/* Write counts                       */
	char	*sc_bbraddr[NDPS];	/* Used for BBR data location         */
	char	*sc_bbrparams[NDPS];	/* Used for BBR REASSIGN parameters   */
};
/*
 * sz_softc names shortened
 */
#define sc_cmd		sc_cmdpkt[targid].altcmd.cmd	/* Cmd part of cmd pkt*/
#define sc_dat		sc_cmdpkt[targid].altcmd.dat	/* Dat part of cmd pkt*/
#define sz_command	sc_cmdpkt[targid].sz_cmd		/* Command Packet     */
#define sz_opcode	sc_cmdpkt[targid].sz_cmd.opcode	/* Command Opcode     */
#define	sz_tur		sc_cmdpkt[targid].sz_cmd.cmd.tur	/* TEST UNIT READY    */
#define sz_rwd		sc_cmdpkt[targid].sz_cmd.cmd.rwd	/* REWIND Comman      */
#define sz_rqsns	sc_cmdpkt[targid].sz_cmd.cmd.sense /* REQUEST SENSE     */
#define sz_rbl		sc_cmdpkt[targid].sz_cmd.cmd.rbl	/* REQUEST BLOCK LMTS */
							   /* DISK:           */
#define sz_rdcap	sc_cmdpkt[targid].sz_cmd.cmd.rdcap /* READ CAPACITY   */
#define sz_rwl		sc_cmdpkt[targid].sz_cmd.cmd.rwl  /* Read/Write long  */
							  /* TAPE:	      */
#define	sz_t_read	sc_cmdpkt[targid].sz_cmd.cmd.t_rw /*   READ Command   */
#define sz_t_write	sc_cmdpkt[targid].sz_cmd.cmd.t_rw /*   WRITE Command  */
							  /* DISK:	      */
#define	sz_d_read	sc_cmdpkt[targid].sz_cmd.cmd.d_rw /*   READ Command   */
#define sz_d_write	sc_cmdpkt[targid].sz_cmd.cmd.d_rw /*   WRITE Command  */
#define	sz_d_rw10	sc_cmdpkt[targid].sz_cmd.cmd.d_rw10 /* R/W 10-byte CDB  */
#define sz_d_fu		sc_cmdpkt[targid].sz_cmd.cmd.d_fu /*   FORMAT UNIT */
#define sz_d_rb		sc_cmdpkt[targid].sz_cmd.cmd.d_rb /*   REASSIGN BLOCK */
#define sz_d_rdd	sc_cmdpkt[targid].sz_cmd.cmd.d_rdd/*   READ DEFECT DATA */
#define sz_d_vd		sc_cmdpkt[targid].sz_cmd.cmd.d_vd /*   VERIFY DATA */

#define sz_trksel	sc_cmdpkt[targid].sz_cmd.cmd.trksel /* TRACK SELECT     */
#define sz_resunit	sc_cmdpkt[targid].sz_cmd.cmd.runit /* RESERVE UNIT      */
#define sz_wfm		sc_cmdpkt[targid].sz_cmd.cmd.wfm	/* WRITE FILEMARKS    */
#define sz_space	sc_cmdpkt[targid].sz_cmd.cmd.space /* SPACE Command     */
#define sz_inq		sc_cmdpkt[targid].sz_cmd.cmd.inq	/* INQUIRY Command    */
#define sz_vfy		sc_cmdpkt[targid].sz_cmd.cmd.vfy	/* VERIFY Command     */
#define sz_rbd		sc_cmdpkt[targid].sz_cmd.cmd.rw	/* RCVR BUFFERED DATA */
#define sz_modsel	sc_cmdpkt[targid].sz_cmd.cmd.modsel /* MODE SELECT      */
#define sz_relunit	sc_cmdpkt[targid].sz_cmd.cmd.runit /* RELEASE UNIT      */
#define sz_erase	sc_cmdpkt[targid].sz_cmd.cmd.erase /* ERASE Command     */
#define sz_modsns	sc_cmdpkt[targid].sz_cmd.cmd.sense /* MODE SENSE Command*/
#define sz_load		sc_cmdpkt[targid].sz_cmd.cmd.ld	/* LOAD Command	      */
#define sz_unload	sc_cmdpkt[targid].sz_cmd.cmd.ld	/* UNLOAD Command     */
#define sz_recdiag	sc_cmdpkt[targid].sz_cmd.cmd.recdiag /* REC DIAG RESULT */
#define sz_snddiag	sc_cmdpkt[targid].sz_cmd.cmd.diag  /* SEND DIAGNOSTIC */
#define sz_copy	        sc_cmdpkt[targid].sz_cmd.cmd.copy  /* SEND COPY       */
/*
 * Values for sc_selstat
 */
#define SZ_IDLE		 0	/* The device is not selected (BUS Free)      */
#define SZ_SELECT	 1	/* The device is selected		      */
#define	SZ_DISCONN	 2	/* The device has disconnected		      */
#define SZ_RESELECT	 3	/* The device is in the reselection process   */
#define	SZ_BBWAIT	 4	/* Bus Busy Wait (wait for bus free)	      */
#define	SZ_SELTIMO	 5	/* Waiting for select (250 MS timeout)	      */

/*
 * State Machine Events
 */
#define SZ_CONT		 0	/* Continue wherever processing left off      */
#define	SZ_BEGIN	 1	/* BEGIN processing requests from the queue   */
#define SZ_DMA_DONE	 2	/* DMA count to zero interrupt		      */
#define SZ_PAR_ERR	 3	/* Parity Error interrupt		      */
#define SZ_PHA_MIS	 4	/* Phase Mismatch interrupt		      */
#define SZ_RESET	 5	/* RST interrupt			      */
#define SZ_CMD		 6	/* In Command Mode (Status/Positioning Cmd)   */
#define SZ_DMA		 7	/* Data transfer using DMA		      */
#define SZ_ABORT	 8	/* Abort the fuzzy transfer		      */
#define SZ_ERROR	 9	/* An error event occured		      */
#define	SZ_TIMEOUT	10	/* Timer expired			      */
#define SZ_FREEB	11	/* Bus needs to be freed		      */
#define	SZ_SELWAIT1	12	/* Select timeout wait events		      */
#define	SZ_SELWAIT2	13

/*
 * Number of retries for data transfer (RW)
 * and non data transfer (SP) commands.
 * Note: values are equal for now, but that could change.
 */

#define	SZ_SP_RTCNT	1
#define	SZ_RW_RTCNT	1

/*
 * Number of seconds to wait before retrying the command
 * when a target return busy status. Currently we wait
 * one second.
 * NOTE: used by SII code, not by NCR 5380 code.
 */
#define	SZ_BUSY_WAIT	1

/*
 * Return Status from various routines 
 */
#define	SZ_SUCCESS	0	/* Success				      */
#define SZ_IP		1	/* In Progress				      */
#define SZ_RET_ERR	2	/* Error condition occured		      */
#define	SZ_RET_ABORT	3	/* Command aborted in scsistart()	      */
#define SZ_DISCONNECT	4	/* Phase Error				      */
#define SZ_RETRY	5	/* The command failed, retries may succeed    */
#define SZ_FATAL	6	/* The command failed, retries will fail      */
#define	SZ_BUSBUSY	7	/* SCSI bus arbitration failed (put off cmd)  */
#define	SZ_SELBUSY	8	/* Wait for 250 MS select timeout	      */
#define	SZ_RET_RESET	9	/* Bus being reset (bail out, restart later)  */
#define	SZ_TARGBUSY	10	/* Target is busy, resend command later       */

#define	b_retry  b_bufsize	/* Local command buffer [see tzcommand()]     */
#define	b_comand b_gid		/* Local command buffer [see tzcommand()]     */

/*
 * General defines for Common values used with the option tables.
 * Ie generic densities etc.
*/
#define NO_OPTTABLE	0X0	/* make sure its null if no option table */

	/* Block sizes defined - some of the common ones */
#define SCSI_QIC_FIXED		512	/* for densities 24, 120, 150	*/
#define SCSI_QIC_320_FIXED	1024	/* for fixed 320 density	*/
#define SCSI_VARIABLE		0	/* varible block size		*/

	/* QIC SCSI density codes */
#define SCSI_QIC_UNKNOWN	0x0	/* QIC density is unknown.	*/
#define SCSI_QIC_24_DENS	0x5	/* QIC 24 density code		*/
#define SCSI_QIC_120_DENS_ECC	0xd	/* QIC 120 density with ECC.	*/
#define SCSI_QIC_150_DENS_ECC	0xe	/* QIC 150 density with ECC.	*/
#define SCSI_QIC_120_DENS	0xf	/* QIC 120 density code		*/
#define SCSI_QIC_150_DENS	0x10	/* QIC 150 density code		*/
#define SCSI_QIC_320_DENS	0x11	/* QIC 320 density code		*/

	/* SCSI 9 track density codes */
#define SCSI_DENS_DEFAULT	0	/* default with this density	*/
#define SCSI_800		0x1	/* 800 bpi			*/
#define SCSI_1600		0x2	/* 1600 bpi			*/
#define SCSI_6250		0x3	/* 6250 bpi			*/

#define SCSI_SPEED_MASK		0xf	/* Mask the char to 4 bits.	*/
#define SCSI_DENS_MASK		0xff	/* Mask off the int to a char	*/



/* Instructions for setting up a tape_opt_tab structure.
 *
 * The tape option table structure allows for easy addition of a 
 * a SCSI bus tape drive. The table directs the SCSI driver to
 * format scsi command packets with certain lengths, density codes
 * number of file marks on close etc. The tape option table is
 * and array of structures each having the type of struct tape_opt_tab.
 * The struct devtab entry for the device has a pointer declared called
 * opt_tab. This pointer can either be null for no option table or can 
 * contain the address of the tape option table entry for this device.
 * There are some pre-defined tape option table entries for units already
 * known. If there is not an entry that describes your tape device you
 * can add an entry to the end of the table. 
 * 
 * Each tape option table structure has 2 parts. The first part is the
 * device specific part which describes what type of tape unit the 
 * device is, and some out/in going data sizes. The second part of
 * the struct is an array of structures that descibes the actions
 * for each of the possible densities. There can be only 4 densities
 * descibed by the major/minor pair for the device. Below is an
 * example of the rmt0 device in the /dev directory. Bits 5 and 4
 * of minor number are used for density selection.
 *
 *			Bit 5 | Bit 4
 *                     |-------------|
 *		rmt0l  |   0  |  0   | low density device
 *		rmt0h  |   0  |  1   | high density device
 *		rmt0m  |   1  |  0   | medium density device
 *		rmt0a  |   1  |  1   | auxilary density device
 *		       |-------------|
 * 
 * Structure flags and members explainations
 * 	
 *	opt_flags
 *	    MSEL_PLL_VAL
 *		This flag is used in conjuction with the struct member
 *		msel_pll. The flag tells the driver if the field msel_pll
 *		is valid and available for use. The msel_pll member is
 *		the mode select parameter list length. This field is
 *		used for the mode select command to specify the length
 *		of the parameter list. If this field is not valid a paramter
 * 		list length of 0 is used. This can cause problems with density
 *		selection and other options.  Please refer to your SCSI devices
 *		technical manual for the length of your devices parameter
 *		list.
 *	    MSEL_BLKDL_VAL
 *		This flag is used in conjuction with the struct member
 *		msel_blkdl. The flag tells the driver if the field msel_blkdl
 *		is valid and available for use. The msel_blkdl member is
 *		the mode select block descriptor list length. This field is
 *		used for the mode select command to specify the length
 *		of the block descriptor list.  If this field is not valid a 
 *		block descriptor list length of zero is used. This can cause 
 *		problems with density selection and other options.  
 *		Please refer to your SCSI devices technical manual for the 
 *		length of your devices block descriptor	list.
 *	    MSEL_VUL_VAL
 *		This flag is used in conjuction with the struct member
 *		msel_vul. The flag tells the driver if the field msel_vul
 *		is valid and available for use. The msel_vul member is
 *		the mode select vendor unique list length. This field is
 *		used for the mode select command to specify the length
 *		of the vendor unique list.  If this field is not valid  
 *		the vendor portion of the mode select commond is not
 *		described. Devices that implement SCSI 2 do not use 
 *		the vendor unique field of the mode select command,
 *		and some SCSI 1 implementations do not use the field also.
 *		Please refer to your SCSI devices technical manual to see
 *		if your device uses the vendor unique field in the mode select
 *		select command and for the length of your devices vendor 
 *		unique list.
 *	    MSNS_ALLOCL_VAL
 *		This flag is used in conjuction with the struct member
 *		msns_allocl. The flag tells the driver if the field msns_allocl
 *		is valid and available for use. The msns_allocl member is
 *		the mode sense allocation length. This field is
 *		used for the mode sense command to specify the length
 *		of the space allocated for data coming in from the device and 
 *		how much data the the device should transfer. If this field is 
 *		not valid a allocation length of zero is used. This can cause 
 *		problems with detecting density of the tape and other options.  
 *		Please refer to your SCSI devices technical manual for the 
 *		length of your devices mode sense allocation length.
 *	    RSNS_ALLOCL_VAL
 *		This flag is used in conjuction with the struct member
 *		rsns_allocl. The flag tells the driver if the field rsns_allocl
 *		is valid and available for use. The rsns_allocl member is
 *		the request sense allocation length. This field is
 *		used for the request sense command to specify the length
 *		of the space allocated for data coming in from the device and 
 *		how much data the the device should transfer. If this field is 
 *		not valid a allocation length of zero is used. This can cause 
 *		problems with detecting error conditions. If the field is valid
 *		and the size specified is greater then the size of the drivers
 *		storage area then the size will be trimmed to the storage size
 *		and a message will appear in the error log file.  
 *		Please refer to your SCSI devices technical manual for the 
 *		length of your devices mode sense allocation length.
 *	    PAGE_VAL
 *		This flag is used in conjuction with the struct member
 *		page_size. The flag tells the driver if the field page_size
 *		is valid and available for use. The page_size member is
 *		used in only SCSI 2 type devices. The page_size member is
 *		used in the driver for the transfer of device pages. The 
 *		value of this field can not excede 32 bytes. This field
 *		is the largest size of any page for the device. An exmaple
 *		of this is a SCSI 2 device which has 3 selectable pages.
 *		page 0 has a size of 11 bytes, page 1 has a size of 14
 *		bytes and page 2 has a size of 16 bytes. The flag PAGE_VAL
 *		should be set and the page_size member should be 16, which
 *		is the largest of the 3 pages.
 *		PLEASE note that currently the SCSI device driver for tapes
 *		has no need to manipulate any of the pages. 
 *	    BUF_MOD
 *		This flag is used to direct the driver to set the buffered
 *		mode bit in the modes select command packet. In buffered
 *		mode, write operations send a command complete message as 
 *		soon as the host (cpu) transfers the data specified in the
 *		command to the units buffer. Please refer to your units
 *		technical manual to see if the unit supports buffered mode.
 *		If the unit does support buffered mode it is strongly suggested
 *		that the flag is set. Failure to set the flag if buffered
 *		mode is supported will prevent the unit from streaming.
 *	    SCSI_QIC
 *		This flag tells the driver that the scsi unit is a QIC format
 *		tape drive.
 *	    SCSI_9TRK
 *		This flag tells the driver that the scsi unit is a 9 track
 *		format tape drive.
 *
 *	msel_pll
 *		Mode select parameter list length. Please refer to MSEL_PLL_VAL
 *		above.
 *	msel_blkdl
 *		Mode select block descriptor list length. Please refor to
 *		MSEL_BLKDL_VAL above.
 *	msel_vul
 *		Mode select vendor unique list length. Please refer to 
 *		MSEL_VUL_VAL above.
 *	msns_allocl
 *		Mode sense allocation length. Please refer to MSNS_ALLOCL_VAL
 *		above.
 *	rsns_allocl
 *		Request sense allocation length. Please refer to RSNS_ALLOCL_val
 *		above.
 *	page_size
 *		SCSI 2 device largest page size. Please refer to PAGE_VAL above.
 *	rserv1
 *	rserv2
 *		Reserved for future expansion and for longword boundaries.
 *	
 *	struct tape_info[NUM_DENS]   NUM_DENS = 4
 *		Each struct represents 1 of the possible densities selections.
 *		You should validate and define each density struct because each
 *		of the defaults taken are 0. Which can cause problems. 	   
 *	tape_flags
 *	    DENS_VAL
 *		This flag is used in conjuction with struct members dens,
 *		and blk_size. The flag tells the driver that these members 
 *		are valid.
 *		Please refer to the members decriptions below for usage.  
 *	    ONE_FM
 *		This flag is used to direct the driver to write only one file
 *		mark on close instead of the normal two. Used with mostly QIC
 *		format style tape drives. Due to the method of recording and
 *		and tape erase most QIC tape drives can not overwrite the 
 *		the second file mark when appending to a tape. 
 *	    SPEED_VAL
 *		This flag is used in conjuction with the struct member 
 *		tape_speed. The flag is used to tell the driver that 
 *		the struct member tape_speed is valid and available for
 *		use. The member contains the value used to specify the 
 *		units tape speed for this density. The value is obtained
 *		from the scsi units technical manual. An example of this
 *		is the TZ07 which has two tape speeds, 25 inches per second
 *		and 100 inches per second. A value of 0x2 specifies 100
 *		inches per second or 0x1 for 25 inches per second. The value of 
 *		member tape_speed should be 0x2 for 100 ips or 0x1 for 25 ips.
 *
 *	dens
 *		This is the actual density value that the mode select command
 *		uses to select the density for reading/writing of tapes.
 *		An example is a QIC unit having 4 density selections. If
 *		you want QIC 120 format for the rmt0m device then the
 *		member dens value would be 0xf. Further information can
 *		be obtained from your units technical manual for the possible
 *		density values that your unit supports.
 *	blk_size
 *		This is the actual block size supported for this density code.
 *		Since there are various blocking method with various style tape
 *		units, this field directs the drive to use a variable block
 *		or a fixed block. QIC style formats are a good example of this.
 *		QIC 150 formats deal with blocks of 512 bytes, QIC 320 format
 *		can be a fixed block of 1024 bytes or have a variable block .
 *		A zero in this field tells the driver that the block formatis
 *		variable. So if the density you want for this rmt device is 
 *		QIC 320 variable block then blk_size should be 0 and the dens
 *		member should be 0x11. If you want QIC 320 fixed then blk_size 
 *		should be 1024 and then dens member should be 0x11. If you want 
 *		QIC 150 then then blk_size should be 512 and the dens member
 *		0x10. Refer to your units technical manual for the correct
 *		values.
 *		
*/


/* Instructions for setting up a disk_opt_tab structure.
 *
 * The disk option table structure allows for easy addition of a 
 * a SCSI bus disk drive. The table directs the SCSI driver to
 * format scsi command packets with certain lengths. The disk option table is
 * and array of structures each having the type of struct disk_opt_tab.
 * The struct devtab entry for the device has a pointer declared called
 * opt_tab. This pointer can either be null for no option table or can 
 * contain the address of the disk option table entry for this device.
 * There are some pre-defined tape option table entries for units already
 * known. If there is not an entry that describes your tape device you
 * can add an entry to the end of the table. 
 * 
 * 
 * Structure flags and members explainations
 * 	
 *	opt_flags
 *	    MSEL_PLL_VAL
 *		This flag is used in conjuction with the struct member
 *		msel_pll. The flag tells the driver if the field msel_pll
 *		is valid and available for use. The msel_pll member is
 *		the mode select parameter list length. This field is
 *		used for the mode select command to specify the length
 *		of the parameter list. If this field is not valid a paramter
 * 		list length of zero is used. This can cause problems with option
 *		selection. Please refer to your SCSI device technical manual for
 *		the length of your devices parameter list.
 *	    MSEL_BLKDL_VAL
 *		This flag is used in conjuction with the struct member
 *		msel_blkdl. The flag tells the driver if the field msel_blkdl
 *		is valid and available for use. The msel_blkdl member is
 *		the mode select block descriptor list length. This field is
 *		used for the mode select command to specify the length
 *		of the block descriptor list.  If this field is not valid a 
 *		block descriptor list length of zero is used. This can cause 
 *		problems with option selection. Refer to your SCSI devices 
 *		technical manual for the length of your devices descriptor list.
 *	    MSEL_VUL_VAL
 *		This flag is used in conjuction with the struct member
 *		msel_vul. The flag tells the driver if the field msel_vul
 *		is valid and available for use. The msel_vul member is
 *		the mode select vendor unique list length. This field is
 *		used for the mode select command to specify the length
 *		of the vendor unique list.  If this field is not valid  
 *		the vendor portion of the mode select commond is not
 *		described. Devices that implement SCSI 2 do not use 
 *		the vendor unique field of the mode select command,
 *		and some SCSI 1 implementations do not use the field also.
 *		Please refer to your SCSI devices technical manual to see
 *		if your device uses the vendor unique field in the mode select
 *		select command and for the length of your devices vendor unique
 *		list.
 *	    MSNS_ALLOCL_VAL
 *		This flag is used in conjuction with the struct member
 *		msns_allocl. The flag tells the driver if the field msns_allocl
 *		is valid and available for use. The msns_allocl member is
 *		the mode sense allocation length. This field is
 *		used for the mode sense command to specify the length
 *		of the space allocated for data coming in from the device and 
 *		how much data the the device should transfer. If this field is 
 *		not valid a allocation length of zero is used. This can cause 
 *		problems with detecting the current settings of the drive.  
 *		Please refer to your SCSI devices technical manual for the 
 *		length of your devices mode sense allocation length.
 *	    RSNS_ALLOCL_VAL
 *		This flag is used in conjuction with the struct member
 *		rsns_allocl. The flag tells the driver if the field rsns_allocl
 *		is valid and available for use. The rsns_allocl member is
 *		the request sense allocation length. This field is
 *		used for the request sense command to specify the length
 *		of the space allocated for data coming in from the device and 
 *		how much data the the device should transfer. If this field is 
 *		not valid a allocation length of zero is used. This can cause 
 *		problems with detecting error conditions. If the field is valid
 *		and the size specified is greater then the size of the drivers
 *		storage area then the size will be trimmed to the storage size
 *		and a message will appear in the error log file.  
 *		Please refer to your SCSI devices technical manual for the 
 *		length of your devices mode sense allocation length.
 *	    PAGE_VAL
 *		This flag is used in conjuction with the struct member
 *		page_size. The flag tells the driver if the field page_size
 *		is valid and available for use. The page_size member is
 *		used in only SCSI 2 type devices. The page_size member is
 *		used in the driver for the transfer of device pages. The 
 *		value of this field can not excede 32 bytes. This field
 *		is the largest size of any page for the device. An exmaple
 *		of this is a SCSI 2 device which has 3 selectable pages.
 *		page 0 has a size of 11 bytes, page 1 has a size of 14
 *		bytes and page 2 has a size of 16 bytes. The flag PAGE_VAL
 *		should be set and the page_size member should be 16, which
 *		is the largest of the 3 pages.
 *	    BUF_MOD
 *		This flag is used to direct the driver to set the buffered
 *		mode bit in the modes select command packet. In buffered
 *		mode, write operations send a command complete message as 
 *		soon as the host (cpu) transfers the data specified in the
 *		command to the units buffer. Please refer to your units
 *		technical manual to see if the unit supports buffered mode.
 *		If the unit does support buffered mode it is strongly suggested
 *		that the flag is set. Failure to set the flag if buffered
 *		mode is supported will hinder performance.
 *	    SCSI_REMOVAL
 *		This flag signifies to the driver that this unit is a 
 *		removable media type disk. This has NOT been implemented 
 *		in the driver as of yet.
 *
 *	msel_pll
 *		Mode select parameter list length. Please refer to MSEL_PLL_VAL
 *		above.
 *	msel_blkdl
 *		Mode select block descriptor list length. Please refor to
 *		MSEL_BLKDL_VAL above.
 *	msel_vul
 *		Mode select vendor unique list length. Please refer to 
 *		MSEL_VUL_VAL above.
 *	msns_allocl
 *		Mode sense allocation length. Please refer to MSNS_ALLOCL_VAL
 *		above.
 *	rsns_allocl
 *		Request sense allocation length. Please refer to RSNS_ALLOCL_val
 *		above.
 *	page_size
 *		SCSI 2 device largest page size. Please refer to PAGE_VAL above.
 *	rserv1
 *	rserv2
 *		Reserved for future expansion and for longword boundaries.
 *	
*/


/* 
 * SCSI device option table for tapes - defines and structure declarations
*/

/*
 * Flags for tape_info.tape_flags
*/
#define	DENS_VAL	0x00000001	/* This density and blk size - valid */
#define ONE_FM		0x00000002	/* Write only 1 fm on close 	*/
#define SPEED_VAL	0x00000004	/* Tape speed field is valid */

/*
 * The rest of the fields for expansion... Ie no bsr/fsr etc..
*/

struct tape_info {
	int tape_flags;		/* Flags for opts are valid and direction */
	int dens;		/* SCSI density code ie 1600bpi. QIC_150 etc */
	int blk_size;		/* For QIC fixed unit 512. 9 trk 0 (variable) */
	char tape_speed;	/* tape speed this is per density */
};


/* 
 * Flags for tape_opt_tab.opt_flags or disk_opt_tab
 * This flags are used in both disk and tape option tables
*/

#define MSEL_PLL_VAL		0x00000001	/* msel_pll is valid	*/
#define MSEL_BLKDL_VAL		0x00000002	/* msel_blkdl is valid	*/
#define MSEL_VUL_VAL		0x00000004	/* msel_vul is valid	*/
#define MSNS_ALLOCL_VAL		0x00000008	/* msns_allocl is valid	*/
#define RSNS_ALLOCL_VAL		0x00000010	/* rsns_allocl is valid	*/
#define BUF_MOD			0x00000020	/* Buffered mode */
#define PAGE_VAL		0x00000040	/* page_size is valid */

/*
 * what kind of unit are we.......tape type /disk type.
 * These flags are used in the tape_opt_tab.opt_flags
 * or  disk_opt_tab
*/
		/* TAPES */
#define SCSI_QIC	0x00010000	/* This is a QIC tape unit	*/
#define SCSI_9TRK	0x00020000	/* This is a 9 Track unit	*/

#define SCSI_8MM	0x00040000	/* This is an 8 millemeter tape */
#define SCSI_RDAT	0x00080000	/* This is an rdat tape		*/

		/* DISKS */
#define SCSI_REMOVAL	0x00010000	/* This is a removable disk 
						Not implemented	*/



#define NUM_DENS	0x4	/* Number of densities possible		*/

struct tape_opt_tab {
	int opt_flags; 		/* Direction flags ie qic 9trk etc... 	*/
	char msel_pll;		/* Mode select Parameter list lenght	*/ 
	char msel_blkdl;	/* Mode select block descriptor lenght	*/
	char msel_vul;		/* Mode select vendor unique lenght	*/
	char msns_allocl;	/* mode sense alloc. lenght		*/
	char rsns_allocl;	/* request sense alloc. lenght		*/
	char page_size;		/* SCSI 2 page size			*/
	char rserv1;		/* int boundary and future expansion	*/
	char rserv2;
	struct tape_info tape_info[NUM_DENS];	/* one for each of the possible
						 * densities
						*/
};

struct disk_opt_tab {
	int opt_flags; 		/* Direction flags ie etc... 	*/
	char msel_pll;		/* Mode select Parameter list lenght	*/ 
	char msel_blkdl;	/* Mode select block descriptor lenght	*/
	char msel_vul;		/* Mode select vendor unique lenght	*/
	char msns_allocl;	/* mode sense alloc. lenght		*/
	char rsns_allocl;	/* request sense alloc. lenght		*/
	char page_size;		/* SCSI 2 page size			*/
	char rserv1;		/* int boundary and future expansion	*/
	char rserv2;		/* for further expansion		*/
};



/*
 * SCSI device information table data
 * structure and bit definitions.
 */
struct scsi_devtab {
	char *name;		/* Name we match on */
	int namelen;		/* Length on which we match */
#define tapetype namelen	/* Tapes dont say what they are??? */
	char *sysname;		/* What we call it when we see it */
	int devtype;		/* Mask for class and type of device */
	struct size *disksize;	/* Partition table for disks */
	int probedelay;		/* Time (Usec) to wait after all this */
	int mspw;		/* Milliseconds per word for iostat */
	int flags;		/* Flags to drive probe */
	caddr_t *opt_tab;	/* Pointer to our option table (tapes/disks) */
};

/*
 * There flags control how the driver handles
 * the device (mostly in the probe routine).
 */

/*
 * Send a request sense command to the target after the inquiry.
 * The command status and the sense data are not checked.
 */
#define SCSI_REQSNS		0x00000001

/*
 * Send a start unit command to the target after the inquiry.
 * Command status is not checked. Do not wait for the drive to spin up.
 */
#define SCSI_STARTUNIT		0x00000002

/*
 * Operate the target in synchronous data transfer mode if possible.
 * SII chip only. NCR 5380 does not support synchronous SCSI.
 */
#define SCSI_TRYSYNC		0x00000004

/*
 * Send a test unit ready command to the target after the inquiry.
 * The command status is not checked.
 */
#define SCSI_TESTUNITREADY	0x00000008

/*
 * Send a read capacity command to the target after the inquiry.
 * The command status and capacity data are not checked.
 */
#define SCSI_READCAPACITY	0x00000010

/*
 * For tapes only. Do not send the receive diagnostic results
 * command to the tape during the first open. This flag is for
 * SCSI tapes which do not support the receive diagnostic results
 * command. The TZ30 and TZK50 support receive diagnostic results,
 * so do not set the NODIAG flag for these tapes.
 */
#define SCSI_NODIAG		0x00000020

/*
 * For disks, including cdrom, only. Set the PF (page format)
 * bit when sending a mode select command to the target.
 */
#define	SCSI_MODSEL_PF		0x00000040

/*
 * For disks, not including cdrom, only. Set if the disk has
 * removable media, such as the RX23 floppy disk drive.
 */
#define	SCSI_REMOVABLE_DISK	0x00000080

/*
 * See sys/data/scsi_data.c.
 */
#define	SCSI_MODSEL_EXABYTE	0x00000100

/*
 * For hard disks. The driver reassigns the block if an ECC
 * correctable error occurs. Set this flag if the device does
 * not support the reassign block command. See sys/data/scsi_data.c
 * for examples, such as floppy disks and cdrom devices.
 */
#define	SCSI_NODBBR		0x00000200

/* DBBR state values and misc defines. */

#define BBR_READ	0	/* Read the bad block                        */
#define BBR_REASSIGN	1	/* Reassign the bad block                    */
#define BBR_WRITE	2	/* Write the bad block                       */

#define BBR_COUNT	3	/* number of retries for the BBR states      */

/*
 * SCSI error log data structures and definitions.
 */

#define	SZ_EL_VERS	2	/* SCSI error log packet version number */

/*
 * SCSI error log information flags.
 * These flags tell the scsi_logerr routine what type
 * of information to include in the error log packet.
 * The flags are included in the error log packet in the
 * info_flags field so UERF knows which fields are valid.
 */
#define	SZ_LOGCMD	0x01	  /* Log SCSI command packet (CDB)	      */
#define	SZ_LOGSTAT	0x02	  /* Log SCSI status byte		      */
#define	SZ_LOGMSG	0x04	  /* Log SCSI message byte		      */
#define	SZ_LOGSNS	0x08	  /* Log SCSI extended sense data	      */
#define	SZ_LOGREGS	0x10	  /* Log SCSI controller and DMA registers    */
#define	SZ_LOGBUS	0x20	  /* Log SCSI bus data (which IDs on the bus) */
#define	SZ_LOGSELST	0x40	  /* LOG SCSI select status for each target   */
#define	SZ_HARDERR	0x10000	  /* HARD error				      */
#define	SZ_SOFTERR	0x20000	  /* SOFT error				      */
#define	SZ_RETRYERR	0x40000	  /* RETRY error			      */
#define	SZ_ESMASK	0x70000	  /* Error severity mask		      */
#define	SZ_NCR5380	0x100000  /* Port type is NCR 5380 chip		      */
#define	SZ_DECSII	0x200000  /* Port type is DEC SII chip		      */
#define SZ_NCRASC   	0x400000  /* Port type is NCR 53C94 (ASC)             */

/*
 * SCSI error log error type definitions.
 */
#define	SZ_ET_DEVERR	0	/* Device error reported from szerror()       */
#define	SZ_ET_PARITY	1	/* SCSI bus parity error		      */
#define	SZ_ET_BUSRST	2	/* SCSI bus reset detected		      */
#define	SZ_ET_RSTBUS	3	/* Controller resetting SCSI bus	      */
#define	SZ_ET_RSTTARG	4	/* Controller resetting target		      */
#define	SZ_ET_CMDABRTD	5	/* Command aborted			      */
#define	SZ_ET_RESELERR	6	/* Reselect error			      */
#define	SZ_ET_STRYINTR	7	/* Stray interrupt			      */
#define	SZ_ET_SELTIMO	8	/* Selection timeout			      */
#define	SZ_ET_DISTIMO	9	/* Disk disconnect timeout		      */
#define	SZ_ET_CMDTIMO	10	/* Command timeout			      */
#define	SZ_ET_ACTSTAT	11	/* Activity status error		      */
#define	SZ_ET_BUSERR	12	/* SCSI bus protocol error		      */
#define	SZ_ET_DBBR	13	/* Dynamic Bad Block Replacement reporting    */

/*
 * NCR 5380 SCSI chip registers.
 *
 * We only log the readable registers.
 * The select enable register is not readable
 * so we keep a copy in the sz_softc structure.
 */
struct	reg_5380 {
	u_char	ini_cmd;	/* (rw) Initiator command register    */
	u_char	mode;		/* (rw) Mode register		      */
	u_char	tar_cmd;	/* (rw) Target command register	      */
	u_char	cur_stat;	/* (ro) Current bus status register   */
	u_char	sel_ena;	/* (wo) Select enable (soft copy)     */
	u_char	status;		/* (ro) Bus and status register	      */
	u_char	pad[2];		/*      Alignment, not needed, but... */
	u_long	adr;		/* (rw) DMA address register	      */
	u_long	cnt;		/* (rw) DMA count register	      */
	u_long	dir;		/* (rw) DMA direction register	      */
};

/*
 * SII chip registers.
 *
 * Only the meaningful registers are logged.
 */

struct	reg_sii {
	u_short	sii_sdb;	/* SCSI Data Bus and Parity		*/
	u_short	sii_sc1;	/* SCSI Control Signals One		*/
	u_short	sii_sc2;	/* SCSI Control Signals Two		*/
	u_short	sii_csr;	/* Control/Status register		*/
	u_short	sii_id;		/* Bus ID register			*/
	u_short	sii_slcsr;	/* Select Control and Status Register	*/
	u_short	sii_destat;	/* Selection Detector Status Register	*/
	u_short	sii_dstmo;	/* DSSI Timeout Register		*/
	u_short	sii_data;	/* Data Register			*/
	u_short	sii_dmctrl;	/* DMA Control Register			*/
	u_short	sii_dmlotc;	/* DMA Length of Transfer Counter	*/
	u_short	sii_dmaddrl;	/* DMA Address Register Low		*/
	u_short	sii_dmaddrh;	/* DMA Address Register High		*/
	u_short	sii_dmabyte;	/* DMA Initial Byte Register		*/
	u_short	sii_stlp;	/* DSSI Short Target List Pointer	*/
	u_short	sii_ltlp;	/* DSSI Long Target List Pointer	*/
	u_short	sii_ilp;	/* DSSI Initiator List Pointer		*/
	u_short	sii_dsctrl;	/* DSSI Control Register		*/
	u_short	sii_cstat;	/* Connection Status Register		*/
	u_short	sii_dstat;	/* Data Transfer Status Register	*/
	u_short	sii_comm;	/* Command Register			*/
	u_short	sii_dictrl;	/* Diagnostic Control Register		*/
	u_short	sii_clock;	/* Diagnostic Clock Register		*/
	u_short	sii_bhdiag;	/* Bus Handler Diagnostic Register	*/
	u_short	sii_sidiag;	/* SCSI IO Diagnostic Register		*/
	u_short	sii_dmdiag;	/* Data Mover Diagnostic Register	*/
	u_short	sii_mcdiag;	/* Main Control Diagnostic Register	*/
};

/*
 *  ASC Chip registers (read only)
 */

struct  reg_asc {
    u_char  tclsb;  /* Transfer Count LSB           */
    u_char  tcmsb;  /* Transfer Count MSB           */
    u_char  cmd;    /* Command register         */
    u_char  stat;   /* Status register          */
    u_char  ss;     /* Sequence Step register       */
    u_char  intr;   /* Interrupt status register        */
    u_char  ffr;    /* FIFO flags register          */
    u_char  cnf1;   /* Configuration 1 register     */
    u_char  cnf2;   /* Configuration 2 register     */
    u_char  cnf3;   /* Configuration 3 register     */

};  /* end reg_asc */
   
/*
 * Data structure for the SCSI portion of the error log packet.
 *
 * TODO:
 * Goal not to exceed mscp el packet size of 120 bytes
 * SII reg size may have blown above goal out of the water!
 */
struct	el_scsi {
	u_char	scsi_elvers;		/* SCSI error log packet version      */
	u_char	error_typ;		/* Error type code		      */
	u_char	suberr_typ;		/* Error sub-type code		      */
	u_char	scsi_id;		/* SCSI bus ID of target	      */
	u_char	bus_data;		/* SCSI bus data (which IDs on bus)   */
	u_char	scsi_status;		/* SCSI status byte		      */
	u_char	scsi_msgin;		/* SCSI message in byte		      */
	u_char	scsi_pad;		/* Place holder			      */
	int	info_flags;		/* Info fields valid flags	      */
	u_char	scsi_selst[NDPS];	/* Select status for each target      */
	u_char	scsi_cmd[12];		/* SCSI command packet (CDB)	      */
	struct	sz_exsns_dt scsi_esd;	/* SCSI extended sense data	      */
	union {
	    struct reg_sii  siiregs;	/* SII port registers		      */
	    struct reg_5380 ncrregs;	/* NCR 5380 port registers	      */
       	    struct reg_asc  ascregs;    /* NCR 53C94 (ASC) port registers     */
	} scsi_regs;
	u_long	sect_num;		/* Sector number for disk errors      */
};

#endif /*	SCSIREG_INCLUDE */
