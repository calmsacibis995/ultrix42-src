/*
 *	@(#)mscp_defs.h	2.8	(ULTRIX)	1/19/90
 */

/************************************************************************
 *                                                                      *
 *                   Copyright (c) 1987, 1988, 1989 by			*
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Disk Class Driver
 *
 *   Abstract:	
 *
 *   Author:	David E. Eiche	Creation Date:	September 30, 1985
 *
 *   History:
 *
 *   03-July-1990	Matthew Sacks
 *	Add HTMO_OVERHEAD_KDB50 as a special case of the below mod.
 *
 *   28-Dec-1989	David E. Eiche		DEE0081
 *	Add HTMO_OVERHEAD to account for host processing overhead
 *	when calculating the host timeout.  Change the format of
 *	the controller model table to remove the host timeout,
 *	controller name and bus type fields whose values are now
 *	determined by live code.
 *
 *   23-Oct-1989	Tim Burke
 *	Added "excl_acc" to the unit block state flags.
 *
 *   21-Sep-1989        Tim Burke
 *	Changed the number of supported tapes to use the NTUNIT define instead
 *	of NUNIT since we support more disk than tape units.
 *
 *   19-May-1989	Tim Burke
 *	Added tms_cach_write bitfield to the Tflags field of the unit block
 *	structure for tmscp caching support.  This bit indicates that there
 *	is potential data in the controller's write back cache which may not
 *	have been flushed to media yet.
 *
 *   22-Mar-1989	Tim Burke
 *	Changed some structure element names to enable them to be used with
 * 	the Insert_entry and Remove_entry queue manipulation macros.  The 
 * 	following name changes were made:
 *	1) To the connection block:
 *		unit_fl 	to	unit.flink
 *		unit_bl		to 	unit.blink
 *	2) To the class block:
 *		connb_fl	to	flink
 *		connb_bl	to	blink
 *
 *   28-Mar-1989	Tim Burke
 *	Changed Ux (unit number calculation macro) to accomodate the usage
 *	of multiple major numbers to represent "ra" type devices.
 *
 *   20-Oct-1988	David E. Eiche		DEE0059
 *	Change CONNECT_TMO and CONNECT_RETRIES to retry more quickly.
 *
 *   28-Sep-1988	David E. Eiche		DEE0058
 *	Change host timeout definition for controllers that support
 *	dual port devices.
 *
 *   06-Sep-1988	David E. Eiche		DEE0054
 *	Add another unit flag to prevent the rwaitct to be incremented
 *	more than once when a second loss of connection occurred while
 *	recovering from the first.  Also change Incr_rwait and
 *	Decr_rwait macros to do nothing if the unit block address is
 *	zero.
 *
 *   06-Sep-1988	David E. Eiche		DEE0053
 *	Change definitions for the host timeout values in the controller
 *	model table.
 *
 *   19-Aug-1988	Pete Keilty
 *	Added 2 unit flags busy and close_ip for open and close
 *	synchronization.
 *
 *   17-Aug-1988	David E. Eiche		DEE0051
 *	Change state definitions to use ST_CMN_INITIAL so that
 *	recovery can rely on the initial state in any state table
 *	to be the same.  Also add a new connection flag to indicate
 *	that unit polling is in progress.
 *
 *   27-Jul-1988	Pete Keilty
 *	Add force flag to the flags word to id request as a force
 *	replacement, used in bbr code.
 *
 *   17-Jul-1988	David E. Eiche		DEE0047
 *	Add a connection block field to hold a pointer to the
 *	ubminit table entry for the controller.
 *
 *   17-Jul-1988	David E. Eiche		DEE0045
 *	Change model table structure definition and add fields to the
 *	connection block to hold host timeout, model name and bus type.
 *
 *   08-Jul-1988	Pete Keilty
 *	Added ST_ACC_INITIAL scan state and 4 access varibles to unitb.
 *	Changes flags.force_ip to flags.force_scan_ip.
 *
 *   08-Jun-1988	David E. Eiche		DEE0040
 *	Changed MAP_WAIT_TMO to be RSRC_WAIT_TMO and maptmo_intvl to be
 *	rsrctmo_intvl reflecting the revised purpose of the latter field
 *	in the connection block.
 *
 *   16-May-1988	Stephen L. Reilly
 *	Changed open_count field to part_mask, since the file system
 *	will only close us once now. 
 *
 *   16-May-1988	David E. Eiche		DEE0036
 *	Change old_cmd_sts field to be unsigned long.
 *
 *   13-Mar-1988	David E. Eiche		DEE00XX
 *	Removed unused trace macro definitions.
 *
 *   12-Feb-1988	David E. Eiche		DEE0014
 *	Remove Insq, Remq and media ID conversion macros which are
 *	no longer used.
 *
 *   02-Feb-1988	David E. Eiche		DEE0011
 *	Remove request block queue from class driver block definition.
 *
 *   26-Jan-1988	David E. Eiche		DEE0010
 *	Change timeout values used in bringing up a connection.
 */
/**/

/* Define MSCP_defs.H and test on it.  If it's already defined don't
 * include this file at all as it's been included once already.
 */
#ifndef MSCP_defs.H
#define MSCP_defs.H	0
/* Local Definitions.
 */

/* Common event codes
 */
#define EV_NULL		0		/* NULL event			     */
#define EV_INITIAL	1		/* Initialize state machine	     */
#define EV_RSPID	2		/* RSPID is available		     */
#define EV_MSGBUF	3		/* MSGBUF is available		     */
#define EV_MAPPING	4		/* Mapping resource is available     */
#define EV_ENDMSG	5		/* End message arrived		     */
#define EV_TIMEOUT	6		/* A timeout occurred		     */
#define EV_NOCREDITS	7		/* No credits available		     */
#define EV_ERRECOV	8		/* Error recovery complete	     */

/* Connection management specific event codes.
 */
#define EV_CONACTIVE	10		/* Connection is active		     */
#define EV_POLLCOMPLETE	11		/* Unit polling completed	     */
#define EV_EXRETRY	12		/* Exhausted retries		     */
#define EV_DISCOMPLETE	13		/* Disconnect completed		     */
#define EV_CONCOMPLETE	14		/* Connection completed		     */
#define EV_PATHFAILURE	15		/* Connection is broken		     */ 

/* Online specific event codes.  Used by online, available and unit recovery
 * state tables and routines.
 */
#define EV_ONLDONEXT	12		/* Process next unit (recovery )     */
#define EV_ONLERRAVAIL	13		/* Error during online- force AVAIL  */
#define EV_ONLCOMPLETE	14		/* Online sequence completed	     */
#define EV_ONLERROR	15		/* Error occurred during online	     */

/* Bad block replacement specific event codes.
 */
#define EV_BBRSUBSTEP	11		/* Substep completed		     */
#define EV_BBRRCTFULL	12		/* RCT full			     */
#define EV_BBRINVRCT	13		/* Invalid (corrupt) RCT	     */
#define EV_BBRSUCCESS	14		/* Step succeeded		     */
#define EV_BBRERROR	15		/* (Sub-)step encountered error	     */

/* Highest event number defined for any event table.
 */
#define EV_MAXEVENT	15		/* Maximum event code		     */

/* Common initial state.  Must be the same for all state tables for
 * recovery to work correctly.
 */
#define ST_CMN_INITIAL	0		/* Initial state		     */

/* Connection management states
 */
#define ST_CN_INITIAL	ST_CMN_INITIAL	/* Connection is uninitialized	     */
#define ST_CN_CLOSED	1		/* Connection is closed/broken	     */
#define ST_CN_RESOURCE	2		/* Resource wait		     */
#define ST_CN_STCON1	3		/* Processing 1st STCON message	     */
#define ST_CN_STCON2	4		/* Processing 2nd STCON / unit poll  */
#define ST_CN_OPEN	5		/* Connection is open		     */
#define ST_CN_RESTART	6		/* Connection restart after failure  */
#define ST_CN_DEAD	7		/* Persistent connection failure     */

/* Unit polling states
 */
#define ST_UP_INITIAL	ST_CMN_INITIAL	/* Polling initial state	     */

/* Set unit online states
 */
#define ST_ON_INITIAL	ST_CMN_INITIAL	/* Unit online initial state	     */
#define ST_ON_ONLIN	1		/* ONLIN end message state	     */
#define ST_ON_AVAIL	2		/* AVAIL end message state	     */
#define ST_ON_GTUNT	3		/* GTUNT end message state	     */

/* Set unit available states
 */
#define ST_AV_INITIAL	ST_CMN_INITIAL	/* Unit available initial state	     */

/* Force replacement states
 */
#define ST_RPL_INITIAL	ST_CMN_INITIAL	/* Unit available initial state	     */

/* Data transfer states
 */
#define ST_XF_INITIAL	ST_CMN_INITIAL	/* Data transfer initial state	     */

/* Access scan  states
 */
#define ST_ACC_INITIAL	ST_CMN_INITIAL	/* Access scan initial state	     */

/* Set unit characteristics  states
 */
#define ST_STU_INITIAL	ST_CMN_INITIAL	/* Set unit chars. initial state     */

/* Unit recovery states
 */
#define ST_RE_INITIAL	ST_CMN_INITIAL	/* Unit recovery initial state	     */
#define ST_RE_ONLIN	1		/* ONLIN end message state	     */
#define ST_RE_AVAIL	2		/* AVAIL end message state	     */
#define ST_RE_GTUNT	3		/* GTUNT end message state	     */

/* Bad Block Replacement states
 */
#define ST_BB_ONLINIT	ST_CMN_INITIAL	/* Online initialize		     */
#define ST_BB_REPINIT	1		/* Started replacement		     */
#define ST_BB_STEP0A	2		/* Continue online processing	     */
#define ST_BB_STEP0B	3		/* Continue online processing	     */
#define ST_BB_STEP0C	4		/* Continue online processing	     */
#define ST_BB_STEP1	5		/* Continue online processing	     */
#define ST_BB_STEP4	6		/* Read bad block		     */
#define ST_BB_STEP5	7		/* Save bad block data in RCT 1	     */
#define ST_BB_STEP6	8		/* Read RCT block 0 for update	     */
#define ST_BB_STEP6A	9		/* Update RCT block 0 		     */
#define ST_BB_STEP7	10		/* Start stress test of bad block    */
#define ST_BB_STEP7B	11		/* Continue stress test		     */
#define ST_BB_STEP7C	12		/* Continue stress test		     */
#define ST_BB_STEP8	13		/* Write original data back to block */
#define ST_BB_STEP9	14		/* Start search of RCT		     */
#define ST_BB_STEP10	15		/* Update RCT sector 0 for phase 2   */
#define ST_BB_STEP11	16		/* Update descriptor(s)		     */
#define ST_BB_STEP11A	17		/* Process 2nd descriptor block      */
#define ST_BB_STEP11B	18		/* Write out descriptor block	     */
#define ST_BB_STEP11C	19		/* Write out descriptor block	     */
#define ST_BB_STEP12	20		/* Start actual replacement	     */
#define ST_BB_STEP12A	21		/* Start actual replacement	     */
#define ST_BB_STEP12B	22		/* Start actual replacement	     */
#define ST_BB_STEP12C	23		/* Start actual replacement	     */
#define ST_BB_STEP12D	24		/* Start actual replacement	     */
#define ST_BB_STEP12E	25		/* Start actual replacement	     */
#define ST_BB_STEP13	26		/* Update RCT 0 for replacement done */
#define ST_BB_STEP15	27		/* Update RCT 0 for replacement done */
#define ST_BB_STEP15A	28		/* Update RCT 0 for replacement done */
#define ST_BB_STEP16	29		/* Write saved data to replacement   */
#define ST_BB_STEP17	30		/* Write saved data to replacement   */
#define ST_BB_STEP18	31		/* Write saved data to replacement   */
#define ST_BB_RCTSEARCH	32		/* Searching RCT		     */
#define ST_BB_RCTSEARCHA 33		/* Searching RCT		     */
#define ST_BB_RCTSEARCHB 34		/* Searching RCT		     */
#define ST_BB_MULTIREAD	35		/* In multi-read		     */
#define ST_BB_MULTIWRITE 36		/* In multi-write		     */
#define ST_BB_MULTIWRITE2 37		/* Second step of multi-write	     */
#define ST_BB_READ	38		/* Read in BBR mode		     */
#define ST_BB_WRITE 	39		/* Write in BBR mode		     */


/* IOCTL subcodes used by radisk for BBR ancillary functions.
 */
#define ACC_SCAN	1
#define ACC_CLEAR	2
#define ACC_REVEC	3
#define ACC_UNPROTECT	4
#define ACC_PRINT	5

#define	NCONN		4		/* Simultaneous connections allowed  */
#define	NREQ		100		/* Maximum active requests allowed   */
#define	NRSPID	( NREQ + ( 2 * NCONN ))	/* Number of RSPIDs in system	     */

#define	NUNIT		256		/* Maximum units 8majors * 32minors  */
#define NTUNIT		32		/* Number of supported tmscp units   */
#define RA_C_BASE 	60		/* Major number of first ra char dev */
#define RA_B_BASE 	23		/* Major number of first ra block dev*/
#define RA_MINORS 	32		/* One minor number for 32 disks     */

/* Host timeout values
 */
#define HTMO_DUALPORT	30		/* Host timeout for dual ported and  */
					/*  multi-host controllers	     */
#define HTMO_NOTIMEOUT	0		/* Host timeout for dedicated	     */
					/*  controllers			     */
#define	HTMO_OVERHEAD	15		/* Host processing overhead	     */
#define HTMO_OVERHEAD_KDB50 60		/* Kdb50 has very slow init so we    */
					/* 	give the host more fudge     */

#define MAINT_TMO	60		/* Maintenance operation timeout     */
#define CONNECT_TMO	15		/* Connection establishment timeout  */
#define IMMEDIATE_TMO	60		/* Immediate command timeout	     */
#define DEAD_TMO	120		/* DEAD state timeout		     */
#define RSRC_WAIT_TMO	3		/* Resource wait timeout	     */

#define CONNECT_RETRIES	8		/* Connection retry count	     */
#define COMMAND_RETRIES	2		/* Command restart maximum retries   */

/**/

/* Macros.
 */

/* Incr_rwait increments the wait reasons count, causing
 * requests on the unit to be stalled.
 */
#define Incr_rwait( Rp ) { \
    if( ( Rp )->unitb ) \
	( Rp )->unitb->rwaitct++; }

/* Decr_rwait decrements the wait reasons count and unstalls
 * the unit if the count has reached zero.
 */
#define Decr_rwait( Rp ) { \
    if( ( Rp )->unitb ) { \
	--( Rp )->unitb->rwaitct; \
	if( ( Rp )->unitb->rwaitct == 0 ) \
	    mscp_unstall_unit( ( Rp )->unitb ); } }

/* Ux returns the logical unit index given the device number.
 *
 * mscp device unit number.  Since there can be more than one major number
 * representing mscp devices, part of the unit number is implicitly stored
 * in the major number.  The unit number is the difference between this major
 * number and the first mscp major number * 32.  This is because there are
 * 32 units per major number.
 *
 * The base major number is different for block and character devices.
 */
#define Ux(Dev) ((major(Dev) >= RA_C_BASE) ?				       \
		(((major(Dev) - RA_C_BASE) * RA_MINORS) + (minor(Dev) >> 3)) : \
		(((major(Dev) - RA_B_BASE) * RA_MINORS) + (minor(Dev) >> 3)))   

/* Px returns the partition index given the device number.
 */
#define Px( Dev ) ( minor( (Dev) ) & 0x7 )

/* Dev_to_unitb returns the unit block pointer corresponding to
 * the given device number.
 */
#define Dev_to_unitb( Dev ) \
    (( Ux( (Dev) ) >= NUNIT ) ? ( UNITB * )NULL : mscp_unit_tbl[ Ux( (Dev) ) ] )

#define Dev_to_Tunitb( Dev ) \
    (( UNIT( (Dev) ) >= NTUNIT ) ? ( UNITB * )NULL : tmscp_unit_tbl[ UNIT( (Dev) ) ] )

/* Init_msg clears the message buffer and then fills
 * in the RSPID and unit number.  If there is no unit
 * number associated with the command, NULL should be
 * supplied.
 */
#define Init_msg( Ptr, Rspid, Unit ) { \
    ( void )bzero(( caddr_t )(Ptr), sizeof( MSCP_CMDMSG )); \
    (Ptr)->mscp_cmd_ref = *( u_long * )&(Rspid); \
    (Ptr)->mscp_unit = ( u_short )(Unit); }

/* Pad_msg pads out a received end message to the maximum
 * MSCP message size.
 */
#define Pad_msg( Ptr, Size ) { \
    register int temp_size = sizeof( MSCP ) - (Size); \
    if( temp_size > 0 ) \
	( void )bzero(( ( caddr_t )(Ptr) + (Size) ), temp_size ); }
/**/

/* Double linked list entry and listhead
 */
typedef struct	_qe {			/* General queue entry descriptor    */
    struct	_qe	*flink;		/* Forward link			     */
    struct	_qe	*blink;		/* Backward link		     */
} QE, LISTHD;

/* SCA response identifier, a.k.a. command reference number
 */
typedef struct	_rspid {		/* Response ID			    */
    u_short		index;		/* RSPID table subscript	    */
    u_short		seq_no;		/* Seq No - distinguishes instances */
} RSPID;

/* Table of Response IDs (RSPIDs) used to manage allocation of RSPIDS
 * and to correlate a RSPID to a corresponding I/O request block (REQB).
 */
typedef struct _rspid_tbl {
    struct _rspid_tbl	*flink;
    struct _rspid_tbl	*blink;
    RSPID		rspid;
    struct _reqb	*reqb;
} RSPID_TBL;

/* Controller/Unit unique identifier
 */
typedef struct	_uniq_id {		/* Controller/unit identifier	     */
    u_char		device_no[6];	/* Device unique identifier	     */
    u_char		model;		/* Device model number		     */
    u_char		class;		/* Device class			     */
} UNIQ_ID;

/* Partition size field structure
 */
typedef	struct _part_size {		/* Partition size		    */
    daddr_t	    p_nblocks;		/* Number of blocks in partition    */
    daddr_t	    p_blkoff;		/* LBN of start of partition	    */
} PART_SIZE;

/* Disk media ID to name correlation table.
 */
typedef struct _dmscp_media {
    char	    *dev_name;		/* Device name string		     */
    int		    dev_index;
    int		    media_id;		/* MSCP disk medium identifier	     */
    PART_SIZE	    *part_sizes;	/* Default partition table pointer   */
} DMSCP_MEDIA; 

/* Tape media ID to name correlation table.
 */
typedef struct _tmscp_media {
    char	*dev_name;		/* Device name string		     */
    int		dev_index;
    int		media_id;		/* MSCP tape medium identifier	     */
} TMSCP_MEDIA;

/* Controller model to name correlation table structure. 
 */
typedef struct _model {
    char	*name;			/* Controller model name string	     */
    u_char	model;			/* Controller model number	     */
} MODEL;

/* State table entry structure.
 */
typedef	struct	_state {
    u_long	    new_state;		/* State after event occurence	     */
    u_long	    ( *action_rtn )();	/* Action routine address	     */
} STATE;

/* Per-request data structure
 *
 *	This structure has an adb macro associated with it in
 *	/usr/lib/adb.  Use it during debuggung and please
 *	update it if any changes are made in this file to this
 *	structure.
 *
 */
typedef struct	_reqb {
    struct _reqb	*flink;		/* Request block		     */
    struct _reqb	*blink;		/*	queue pointers		     */
    struct _classb	*classb;	/* Class block back pointer	     */
    struct _connb	*connb;		/* Connection block back pointer     */
    struct _unitb	*unitb;		/* Unit block back pointer	     */
    struct buf		*bufptr;	/* Buf structure pointer	     */
    MSCP		*msgptr;	/* Message buffer pointer	     */
    u_long		msgsize;	/* Message size			     */
    u_long		p1;		/* Function dependent parameter 1    */
    u_long		p2;		/* Function dependent parameter 2    */
    u_char		*aux;		/* Auxiliary structure pointer	     */
    RSPID		rspid;		/* Response ID			     */
    u_long		op_seq_num;	/* Operation sequence number	     */
    u_short		*rwaitptr;	/* Resource wait counter pointer     */
    BHANDLE		lbhandle;	/* Local buffer handle		     */
    u_long		state;		/* Request state		     */
    STATE		*state_tbl;	/* State table used for request	     */
    struct {				/* Request block flags		     */
	u_short	perm_reqb 	 :1;	/* Request block is permanent	     */
	u_short nocreditw	 :1;	/* Don't wait for send credit	     */
	u_short online		 :1;	/* Request represents an online	     */
	u_short force		 :1;	/* Request reps a force replacement */
	u_short		   	:12;	/* Unused			     */
    } flags;
} REQB ;

/* Per-unit data structure
 *
 *	This structure has an adb macro associated with it in
 *	/usr/lib/adb.  Use it during debuggung and please
 *	update it if any changes are made in this file to this
 *	structure.
 *
 */
typedef struct	_unitb {
    struct _unitb	*flink;		/* Unit block			     */
    struct _unitb	*blink;		/*	queue pointers		     */
    struct _connb	*connb;		/* Connection block back pointer     */
    struct uba_device	*ubdev;		/* Unibus device structure pointer   */
    struct	{
	REQB		*flink;		/* Request block		     */
	REQB		*blink;		/*	list head		     */
    } request;
    u_short		state;		/* Software unit state		     */
    struct	{			/* Software unit flags		     */
	u_short		alonl	   :1;	/*    Unit was already online	     */
	u_short		busy	   :1;	/*    Unit open/close busy flag	     */
	u_short		online	   :1;	/*    Unit is online		     */
	u_short		online_ip  :1;	/*    Unit online is in progress     */
	u_short		close_ip   :1;	/*    Unit close is in progress     */
	u_short		rct_pres   :1;	/*    Unit has an RCT		     */
	u_short		wrtp	   :1;	/*    Unit is write protected	     */
	u_short		force_scan_ip :1; /*  Forced replace/scan in progress*/
	u_short		wait_bump  :1;	/*    Reconnect has bumped rwaitct   */
	u_short		excl_acc   :1;	/*    Unit is exclusive access 	     */
	u_short		mscp_wait  :1;	/*    Waiting for a state to complete*/
	u_short			   :5;	/*    Unused			     */
    } flags;
    u_short		unit;		/* Unit number			     */
    u_short		rwaitct;	/* Resource wait reason counter	     */
    u_short		part_mask;	/* Open partition mask		     */
    dev_t		dev;		/* ?? DO I NEED THIS ??		     */
    u_short		mult_unt;	/* Multi-unit code		     */
    u_short		unt_flgs;	/* Unit flags			     */
    UNIQ_ID		unit_id;	/* Unit identifier		     */
    u_long		media_id;	/* Media identifier		     */
    u_short		shdw_unt;	/* Shadow unit			     */
    u_short		shdw_sts;	/* Shadow status		     */
    u_short		track;		/* Track size			     */
    u_short		group;		/* Group size			     */
    u_short		cylinder;	/* Cylinder size		     */
    u_char		unit_svr;	/* Unit software version	     */
    u_char		unit_hvr;	/* Unit hardware version	     */
    u_short		rct_size;	/* RCT size			     */
    u_char		rbns;		/* RBNs per track		     */
    u_char		rct_cpys;	/* RCT copies			     */
    u_long		unt_size;	/* Unit size (user accessible area)  */
    u_long		vol_ser;	/* Volume serial number		     */
    u_long		tot_size;	/* Total unit size including RCT     */
    u_long		acc_badlbn;     /* First bad lbn found by access     */
    u_long		acc_bytecnt;    /* access end packet byte count	     */
    u_short		acc_status;     /* access command status	     */
    u_short		acc_flags;      /* access end packet flags	     */
    u_long		tms_softcnt;	/* Soft error count		     */
    u_long		tms_hardcnt;	/* Hard error count		     */
    u_long		tms_category_flags; /* Category flags		     */
    u_long		tms_position;	/* LBN position on tape		     */
    u_long		tms_bcount;	/* Maximum byte count xfer on unit   */
    u_short		tms_format;	/* Current format and density	     */
    u_short		tms_speed;	/* Speed			     */
    u_short		tms_noise;	/* Noise level			     */
    u_short		format_menu;	/* Format/density menu		     */
    union {
	    struct Tflags{				/* Software tape unit flags	     */
		u_short		tms_serex   :1;	/* Serious exception		     */
		u_short		tms_clserex :1;	/* Set by serex, cleared by nop	     */
		u_short		tms_eom     :1;	/* End of media handling state	    */
		u_short		tms_eot     :1;	/* End of tape encountered	     */
		u_short		tms_tm	    :1;	/* Tape mark encountered	     */
		u_short		tms_write   :1; /* Tape was written on after opened  */
		u_short		tms_lost    :1; /* Tape position is unknown/lost     */
		u_short		tms_bufmark :1; /* Tape mark encountered on buffered i/o*/
		u_short		tms_cach    :1; /* Tape unit allows cacheing */
		u_short		tms_cach_on :1; /* Tape units caching in use */
		u_short		tms_cache_lost    :1; /* Cache data lost exception set */
		u_short		tms_inuse   :1; /* Tape unit open and in use */
		u_short		tms_wait   :1;  /* Tape unit wait on something*/
                u_short         tms_cach_write :1;  /* Cached write pending */
	    } Sflags;
	    u_short		clearflags;
	} state_flags;
#define Tflags state_flags.Sflags
#define clear_Sflags state_flags.clearflags
    int			sel;		/* No/rewind, etc.		     */
    u_char		tms_endcode;	/* Last mscp endcode for ioctl	     */
    u_short		tms_status;	/* Last mscp status for ioctl	     */
    u_char		tms_flags;	/* Last mscp flags for ioctl	     */
    u_long		tms_resid;	/* Last mscp resid for ioctl	     */
    u_long		cmd_ref;	/* Last mscp cmd_ref for ioctl ABORT */
    u_long		tms_recovery_location;	/* current recovery position on tape		     */
    char		mscp_device[DEV_SIZE];	/* Media type string 	     */
    struct pt		part_info;	/* Embedded partition structure	     */
    struct buf		rawbuf;		/* Embedded raw I/O buf structure    */
    struct buf		ioctlbuf;	/* Embedded ioctl buf structure	     */
} UNITB;

/* Per-connection data structure
 *
 *	This structure has an adb macro associated with it in
 *	/usr/lib/adb.  Use it during debuggung and please
 *	update it if any changes are made in this file to this
 *	structure.
 *
 */
typedef struct  _connb {
    struct _connb	*flink;		/* Connection block queue	     */
    struct _connb	*blink;		/* 	pointers		     */
    struct _classb	*classb;	/* Class block back pointer	     */
    struct 		{		/* Unit queue list head		     */
	UNITB		*flink;		/*    forward link		     */
	UNITB		*blink;		/*    backward link		     */
    } unit;
    struct _bbrb	*bbrb;		/* BBR block address		     */
    u_short		state;		/* Driver's connection state	     */
    struct {				/* Connection management flags	     */
	u_short         restart    :1;	/* Reconnect in progress	     */
	u_short		sngl_strm  :1;	/* Single streaming after restart    */
	u_short		path_fail  :1;	/* Cleanup entered via path failure  */
	u_short		need_upoll :1;	/* Unit polling is needed	     */
	u_short		upoll_busy :1;	/* Polling request block is in use   */
	u_short		need_cr    :1;	/* Credit reservation is needed	     */
	u_short			  :10;	/* Unused			     */
    } flags;
    struct {
	REQB		*flink;		/* Queue of requests		     */
	REQB 		*blink;		/*	active in controller	     */
    } active;
    struct {
	REQB		*flink;		/* Queue of requests		     */
	REQB		*blink;		/*	in restart queue	     */
    } restart;
    struct {
	REQB		*flink;		/* Queue of requests		     */
	REQB		*blink;		/*	in credit wait queue	     */
    } credit_wq;
    struct {
	REQB		*flink;		/* Queue of requests		     */
	REQB		*blink;		/*	in buffer wait queue	     */
    } buffer_wq;
    struct {
	REQB		*flink;		/* Queue of requests		     */
	REQB		*blink;		/*	in map wait queue	     */
    } map_wq;
    u_short		cmdtmo_intvl;	/* Command timeout interval (s.)     */	
    u_short		rsrctmo_intvl;	/* Resource wait timeout (s.)	     */	
    u_short		retry_count;	/* Connection retry count	     */
    u_short		cur_unit;	/* Unit number used by poller	     */
    u_short		restart_count;	/* Restart command retry count	     */
    u_short		hst_tmo;	/* Host timeout period		     */
    c_scaaddr		sysid;		/* System ID			     */
    c_scaaddr		rport_addr;	/* Remote port address		     */
    u_long		lport_name;	/* Local port name		     */
    u_short		version;	/* MSCP version			     */
    u_short		cnt_flgs;	/* Controller flags		     */
    u_short		cnt_tmo;	/* Controller timeout period	     */
    u_char		cnt_svr;	/* Controller software version	     */
    u_char		cnt_hvr;	/* Controller hardware version	     */
    UNIQ_ID		cnt_id;		/* Controller identifier	     */
    u_long		max_bcnt;	/* Controller maximum byte count     */
    RSPID		old_rspid;	/* RSPID of oldest current command   */
    u_long		old_cmd_sts;	/* Status of oldest current command  */
    REQB		*restart_reqb;	/* Current request being restarted   */
    u_char		*model_name;	/* Controller model name	     */
    char		*cnt_name;	/* Config's controller name	     */
    short		cnt_number;	/* Config's controller number	     */
    u_short		bus_type;	/* I/O bus type			     */
    struct uba_ctlr	*ubctlr;	/* Ubminit structure pointer	     */
    CONNID		connid;		/* Connection ID		     */
    REQB		timeout_reqb;	/* Command timeout permanent REQB    */
    REQB		polling_reqb;	/* Polling/DAP permanent REQB	     */
} CONNB;

/* Driver-wide data structure
 *
 *	This structure has an adb macro associated with it in
 *	/usr/lib/adb.  Use it during debuggung and please
 *	update it if any changes are made in this file to this
 *	structure.
 *
 */

typedef struct  _classb {
    CONNB		*flink;		/* Connection block	 	     */
    CONNB		*blink;		/* 	list head		     */
    struct {
	REQB		*flink;		/* Response ID wait queue	     */
	REQB		*blink;		/* 	list head		     */
    } rspid_wq;
    u_long		operation_ct;	/* Request count since boot	     */
    UNITB		**unit_tbl;	/* Driver unit table pointer	     */
    char		*dev_name;	/* Device name string pointer	     */
    STATE		*recov_states;	/* Unit recovery state table pointer */
    u_short		system_ct;	/* Count of known systems	     */
    struct {
        u_short		disk	  :  1;	/* Disk CLASSB if true, else tape    */
        u_short		init_done :  1;	/* Driver initialization complete    */
        u_short		init_ip   :  1;	/* Driver init. in progress	     */
        u_short		need_poll :  1;	/* Need to re-poll for systems	     */
        u_short		poll_ip   :  1;	/* System poll in progress	     */
        u_short		listen    :  1;	/* SCS listen has been issued	     */
        u_short	 		  : 11;	/* Unused			     */
    } flags;
    CMSB		cmsb;		/* Connection mgmt services block    */
    MSB			msb;		/* Maintenance services block	     */
} CLASSB;


#endif
