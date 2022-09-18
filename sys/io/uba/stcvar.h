
/*
 *	7/2/90	(ULTRIX)	@(#)stcvar.h	4.1
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *                                                                      *
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 * stcvar.h	06-Jun-86
 *
 * Modification History
 *
 * 20-Aug-87	darrell
 *	Added the structure definition for the RECEIVE DIAGNOSTIC
 *	RESULT data.
 *
 * 03-Mar-87	darrell
 *	Fixed an oversight where the command and data structures where
 *	allocating space.
 *
 * 27-Jan-87	darrell
 *	Changed the st_modsel_cm structure to reflect the change in
 *	V32 TZK50 ucode that defines a vendor unique bit that disables
 *	reselect timeouts.
 *
 * 15-Jan-87	darrell
 *	Added definitions and structure changes to support the
 *	rewrite of stc.c that is being sbmitted at the same
 *	time.
 *
 * 26-Sep-86	darrell
 *	Added more constant definitions and a macro to timeout 
 *	spin loops.
 *
 *  5-Sep-86	darrell
 *	Added more constant definition.
 *
 *  4-Sep-86	darrell
 *	First semi-working driver.
 *
 *  6-Aug-86	darrell
 *	Updated for first pass of real driver (compiles but not debugged).
 *
 * 06-Jun-86	darrell 
 *	creation of this file.
 *
 ************************************************************************/


struct st_cmdfmt {
    u_char opcode;			/* SCSI comand opcode		      */
    /*
     * Overlaid portion of commands
     */
    union {
	/* 
	 * TEST UNIT READY Command
	 * READ BLOCK LIMITS Command
	 */
	struct st_tur_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:4;		/* Flag				      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}tur;
	/*
	 * REWIND Command
	 */
	struct st_rewind_cm {
	    u_char	immed:1;	/* Immediate			      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}rwd;
	/*
	 * REQUEST SENSE Command
	 * MODE SENSE Command
	 * INQUIRY Command
	 */
	struct st_sense_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	alclen;		/* Allocation Length		      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}sense;
	/*
	 * READ Command
	 * WRITE Command
	 * RECOVER BUFFERED DATA Command
	 */
	struct st_rw_cm {
	    u_char	fixed:1;	/* Fixed			      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	xferlen2;	/* Transfer Length (MSB)	      */
	    u_char	xferlen1;	/* Transfer Length		      */
	    u_char	xferlen0;	/* Transfer Length (LSB)	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}rw;
	/*
	 * WRITE FILEMARKS Command
	 */
	struct st_wfm_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	numoffmks2;	/* Number of Filemarks (MSB)	      */
	    u_char	numoffmks1;	/* Number of Filemarks		      */
	    u_char	numoffmks0;	/* Number of Filemarks (LSB)	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}wfm;
	/*
	 * SPACE Command
	 */
	struct st_space_cm {
	    u_char	code:2;		/* Fixed			      */
	    u_char		:3;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	count2;		/* Count (MSB)			      */
	    u_char	count1;		/* Count			      */
	    u_char	count0;		/* Count (LSB)			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}space;
	/*
	 * VERIFY Command
	 */
	struct st_vfy_cm {
	    u_char	fixed:1;	/* Fixed			      */
	    u_char	bytcmp:1;	/* Byte Compare			      */
	    u_char		:3;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	verflen2;	/* Verification Length (MSB)	      */
	    u_char	verflen1;	/* Verification Length		      */
	    u_char	verflen0;	/* Verification Length (LSB)	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}vfy;
	/*
	 * ERASE Command
	 */
	struct st_era_cm {
	    u_char	longbit:1;	/* Long				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}erase;
	/*
	 * LOAD/UNLOAD Command
	 */
	struct st_ld_cm {
	    u_char	immed:1;	/* Immediate			      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	load:1;		/* Load				      */
	    u_char	reten:1;	/* Retention			      */
	    u_char		:6;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}ld;
	/*
	 * SEND DIAGNOSTIC Command
	 */
	struct st_snd_diag_cm {
	    u_char	unitofl:1;	/* Unit Offline			      */
	    u_char	devofl:1;	/* Device Offline		      */
	    u_char	selftst:1;	/* Self Test			      */
	    u_char		:2;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char	mbz1[2];	/* Must be Zero			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}diag;
	/*
	 * RECEIVE DIAGNOSTIC RESULT Command
	 */
	struct st_recdiag_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char	aloclen1;	/* Allocation Length (MSB)	      */
	    u_char	aloclen0;	/* Allocation Length (LSB)_	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}recdiag;
	/*
	 * MODE SELECT Command
	 */
	struct st_modsel_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	pll;		/* Parameter List Length	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	/* Parameter List Header	*/
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	mbz1:4;		/* Must be Zero			      */
	    u_char	bufmode:3;	/* Buffered Mode		      */
	    u_char		:1;	/* Reserved			      */
	    u_char	rdeclen;	/* Record Descriptor Length	      */
	/* Parameter List Block Descriptor */
	    u_char	mbz2;		/* Must be Zero			      */
	    u_char	numofrec2;	/* Number of Records (MSB)	      */
	    u_char	numofrec1;	/* Number of Records 		      */
	    u_char	numofrec0;	/* Number of Records (LSB)	      */
	    u_char		:8;	/* Reserved			      */
	    u_char	reclen2;	/* Record Length (MSB)		      */
	    u_char	reclen1;	/* Record Length 		      */
	    u_char	reclen0;	/* Record Length (LSB)		      */
	    u_char	vulen;		/* Vendor Unique Length		      */
	    u_char	nof:3;		/* Enable Fillers		      */
	    u_char	mbz3:3;		/* Must be Zero			      */
	    u_char	notimo:1;	/* Disable Reselect timeouts	      */
	    u_char	vu7:1;		/* Direct Track Access		      */
	    u_char	pad[2];		/* Pad				      */
	}modsel;
	/*
	 * Track SELECT Command
	 */
	struct st_trksel_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	trkval;		/* Track Value			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}trksel;
	/*
	 * RESERVE UNIT Command
	 * RELEASE UNIT Command
	 */
	 struct st_runit_cm {
	    u_char		:1;	/* Reserved			      */
	    u_char	thrdpdev:3;	/* Third Party Device		      */
	    u_char	thrdp:1;	/* Third Party			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}runit;
    }cmd;
};

/*
 * Data Format Structure
 */
struct st_datfmt {
    union {
	/*
	 * Extended Sense Data Format
	 */
	struct st_exsns_dt {
	    u_char	errcode:4;	/* Error Code			      */
	    u_char	errclass:3;	/* Error Class			      */
	    u_char	valid:1;	/* Valid			      */
	    u_char	segnum;		/* Segment Number		      */
	    u_char	snskey:4;	/* Sense Key			      */
	    u_char		:1;	/* Reserved			      */
	    u_char	ili:1;		/* Illegal Length Indicator	      */
	    u_char	eom:1;		/* End of Medium		      */
	    u_char	filmrk:1;	/* Filemark			      */
	    u_char	infobyte3;	/* Information Byte (MSB)	      */
	    u_char	infobyte2;	/* Information Byte 		      */
	    u_char	infobyte1;	/* Information Byte 		      */
	    u_char	infobyte0;	/* Information Byte (LSB)	      */
	    u_char	asl;		/* Additional Sense Length	      */
	    u_char	ctlr;		/* Controller internal error code     */
	    u_char	drv0;		/* Drive error byte		      */
	    u_char	drv1;		/* Drive error byte		      */
	    u_char	pad[9]		/* Pad				      */
	}exsns;
	/*
	 * READ BLOCK LIMITS Data
	 */
	struct st_rd_blklim_dt {
	    u_char		:8;	/* Reserved			      */
	    u_char	maxreclen2;	/* Maximum Record Length (MSB)	      */
	    u_char	maxreclen1;	/* Maximum Record Length 	      */
	    u_char	maxreclen0;	/* Maximum Record Length (LSB)	      */
	    u_char	minreclen1;	/* Minimum Record Length (MSB)	      */
	    u_char	minreclen0;	/* Minimum Record Length (LSB)	      */
	    u_char	pad[14]		/* Pad				      */
	}rdblim;
	/*
	 * INQUIRY Data
	 */
	struct st_inq_dt {
	    u_char	perfdt;		/* Peripheral Deice Type	      */
	    u_char	devtq:7;	/* Device Type Qualifier	      */
	    u_char	rmb:1;		/* Removable Media Bit		      */
	    u_char	version;	/* Version			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	addlen;		/* Additional Length		      */
	    u_char	pad[15]		/* Pad				      */
	}inq;
	/*
	 * MODE SENSE Data
	 */
	struct st_modsns_dt {
	/* Parameter List Header	*/
	    u_char	sdlen;		/* Sense Data Length		      */
	    u_char	mbz1:8;		/* Must be Zero			      */
	    u_char	mbz2:4;		/* Must be Zero			      */
	    u_char	bufmode:3;	/* Buffered Mode		      */
	    u_char	wp:1;		/* Write Protect		      */
	    u_char	bdeclen;	/* Block Descriptor Length	      */
	/* Parameter List Block Descriptor */
	    u_char	mbz3;		/* Must be Zero			      */
	    u_char	numofblk2;	/* Number of Blocks (MSB)	      */
	    u_char	numofblk1;	/* Number of Blocks 		      */
	    u_char	numofblk0;	/* Number of Blocks (LSB)	      */
	    u_char		:8;	/* Reserved			      */
	    u_char	blklen2;	/* Block Length (MSB)		      */
	    u_char	blklen1;	/* Block Length 		      */
	    u_char	blklen0;	/* Block Length (LSB)		      */
	    u_char	vulen;		/* Vendor Unique Length		      */
	    u_char	nof:2;		/* Enable Fillers		      */
	    u_char	mbz4:5;		/* Must be Zero			      */
	    u_char	vu7:1;		/* Direct Track Access		      */
	    u_char	pad[6];		/* Pad				      */
	}modsns;
	/*
	 * RECEIVE DIAGNOSTIC RESULT Data
	 */
	struct st_recdiag_dt {
	    u_char	ctlr_hd_rev;	/* Controller hardware revision level */
	    u_char	ctlr_fw_rev;	/* Controller firmware revision level */
	    u_char	ctlr_selftest;	/* Controller selftest  0 = Passed    */
					/*                      1 = Failed    */
	    u_char	drv_hw_rev;	/* Drive hardware revision level      */
	    u_char	drv_fw_rev;	/* Drive firmware revision level      */
	    u_char	drv_selftest;	/* Drive selftest  0 = Passed	      */
					/*		  xx = Failure Code   */
	}recdiag;
    }dat;
};

/*
 * TZK50 Command Opcodes
 */
#define	ST_TUR		0x00		/* TEST UNIT READY Command	      */
#define ST_REWIND	0x01		/* REWIND Command		      */
#define ST_RQSNS	0x03		/* REQUEST SENSE Command	      */
#define ST_RBL		0x05		/* READ BLOCK LIMITS Command	      */
#define	ST_READ		0x08		/* READ Command			      */
#define ST_WRITE	0x0a		/* WRITE Command		      */
#define ST_TRKSEL	0x0b		/* TRACK SELECT Comand		      */
#define ST_RESUNIT	0x16		/* RESERVE UNIT Command		      */
#define ST_WFM		0x10		/* WRITE FILEMARKS Command	      */
#define ST_SPACE	0x11		/* SPACE Command		      */
#define ST_INQ		0x12		/* INQUIRY Command		      */
#define ST_VFY		0x13		/* VERIFY Command		      */
#define ST_RBD		0x14		/* RECOVER BUFFERED DATA Command      */
#define ST_MODSEL	0x15		/* MODE SELECT Command		      */
#define ST_RELUNIT	0x17		/* RELEASE UNIT Command		      */
#define ST_ERASE	0x19		/* ERASE Command		      */
#define ST_MODSNS	0x1a		/* MODE SENSE Command		      */
#define ST_LOAD		0x1b		/* LOAD Command			      */
#define ST_UNLOAD	0x1b		/* UNLOAD Command		      */
#define ST_RECDIAG	0x1c		/* RECEIVE DIAGNOSTIC RESULT Command  */
#define ST_SNDDIAG	0x1d		/* SEND DIAGNOSTIC Command	      */
#define ST_P_FSPACER	0x1e		/* Psuedo opcode for space record     */
#define ST_P_FSPACEF	0x1f		/* Psuedo opcode for space file	      */
#define ST_P_BSPACER	0x2e		/* Psuedo opcode for backspace record */
#define ST_P_BSPACEF	0x2f		/* Psuedo opcode for backspace file   */
#define ST_P_CACHE	0x2c		/* Psuedo opcode for buffered mode    */
#define ST_P_NOCACHE	0x2d		/* Psuedo opcode for no buffered mode */

/*
 * SCSI Command Data structure names shortened
 */
#define stdt_exsns	st_datfmt.dat.exsns	/* EXTENDED SENSE Data	      */
#define stdt_rbl	st_datfmt.dat.rbl	/* READ BLOCK LIMITS Data     */
#define	stdt_inq	st_datfmt.dat.inq	/* INQUIRY Data		      */
#define stdt_modsel	st_datfmt.dat.modsel	/* MODE SELECT Data	      */
#define stdt_modsns	st_datfmt.dat.modsns	/* MODE SENSE Data	      */
/*
 * SCSI STATUS Byte
 */
#define	ST_GOOD		0x00	/* Good					      */
#define	ST_CHKCND	0x02	/* Check Condition			      */
#define ST_INTRM	0x10	/* Intermediate				      */
#define ST_RESCNF	0x18	/* Reservation Conflict			      */
/*
 * SCSI Message Protocols
 */
#define	ST_CMDCPT	0x00	/* Command Complete			      */
#define ST_SDP		0x02	/* Save Data Pointers			      */
#define ST_DISCON	0x04	/* Disconnect				      */
#define ST_ABT		0x06	/* Abort				      */
#define ST_MSGREJ	0x07	/* Message Reject			      */
#define ST_NOP		0x08	/* No Operation				      */
#define ST_MSGPE	0x09	/* Message Parity Error			      */
#define ST_DEVRST	0x0c	/* Bus Device Reset			      */
#define ST_LNKCMP	0x0a	/* Linked Command Complete		      */
#define ST_LNKCMPF	0x0b	/* Linked Command Complete with Flag	      */
#define ST_ID_NODIS	0x80	/* IDENTIFY wo/ disconnect capability,	      */
#define ST_ID_DIS	0xc0	/* IDENTIFY w/ disconnect capability,	      */
/*
 * Command Data allocation constants
 */
#define	ST_INQ_LEN	 5	/* number of bytes for INQUIRY data	      */
#define ST_RECDIAG_LEN	 6	/* number of bytes for RECEIVE DIAG data      */
#define ST_RQSNS_LEN	11	/* number of bytes for REQUEST SENSE data     */
#define ST_MODSNS_LEN	14	/* number of bytes for MODE SENSE data	      */
#define ST_MODSEL_LEN	14	/* number of bytes for MODE SELECT data	      */
#define ST_MAX_DT_LEN	20	/* Size of st_datfmt structure		      */

/*
 * Command Packet Length constants
 */
#define ST_MAX_CMD_LEN		22	/* maximum command length in bytes    */
#define ST_MODSEL_CMD_LEN	20	/* maximum length of modsel command   */
#define ST_CMD_LEN		 6	/* length of commands in bytes	      */

/*
 * Miscellaneous Constants
 */
#define ST_DEFAULT_FILLS	 7	/* Default number of fills that can be
					 * written to keep streaming	      */
#define	ST_MAX_ARB_TRIES	10	/* Max # of retries for bus arb	      */
#define ST_MAX_SEL_DLY	       250	/* Number of msec to wait for BSY     */
#define ST_MAX_STACK		 4	/* Maximun size of the state stack    */
#define ST_MAX_RETRIES		 5	/* Maximum number of retries	      */
#define ST_ONE_SEC	     10000	/* Produces a one second timeout when
					   calling STWAIT_XXXX macroes.	      */

/*
 * MACRO for converting a longword to a byte
 *	args - the longword
 *	     - which byte in the longword you want
 */
#define	LTOB(a,b)	((a>>(b*8))&0xff)

/* MACROS for timing out spin loops.
 *
 *	desc:	Waits while expression is true.
 *
 *	args:	expr 		- expression to spin on
 *		spincount 	- amount of time to wait in microseconds
 *		retval		- return value of this macro
 */
#define	STWAIT_WHILE(expr,spincount,retval) {				\
		for (retval = 0; ((retval < 100) && (expr)); retval++);	\
		while ((expr) && ( retval < spincount)) {		\
			DELAY(100);					\
			retval++;					\
		}							\
	}

/*
 * 	desc: Waits unitl expression is true.
 */
#define STWAIT_UNTIL(expr,spincount,retval) {				\
		STWAIT_WHILE((!expr),spincount,retval);			\
	}

/*
 * Request Sense Sense Key Codes
 */
#define ST_NOSENSE	0x00	/* Successful cmd or EOT, ILI, FILEMARK	      */
#define ST_RECOVERR	0x01	/* Successful cmd with controller recovery    */
#define ST_NOTREADY	0x02	/* Device present but not ready		      */
#define	ST_MEDIUMERR	0x03	/* Cmd terminated with media flaw	      */
#define ST_HARDWARE	0x04	/* Controller or drive hardware error	      */
#define ST_ILLEGALREQ	0x05	/* Illegal command or command parameters      */
#define ST_UNITATTEN	0x06	/* Unit Attention			      */
#define ST_DATAPROTECT	0x07	/* Write attempted to write protected media   */
#define ST_BLANKCHK	0x08	/* Read zero length record (EOD)	      */
#define ST_ABORTEDCMD	0x0b	/* Cmd aborted that may retry successfully    */
#define ST_VOLUMEOVFL	0x0d	/* Buffer data left over after EOM	      */
#define ST_MISCOMPARE	0x0e	/* Miscompare on Verify command		      */
/*
 * Top Level State Machine
 */
/* Status/Positioning States */
#define	ST_NEXT		 0	/* Process next request from queue	      */
#define ST_SP_START	 1	/* Start a Status or Positioning Operation    */
#define ST_SP_CONT	 2	/* Continue ST_SP_SETUP:		      */
/* Read/Write (DMA) States */
#define	ST_RW_START	 3	/* Start a Read or Write Operation	      */
#define ST_R_STDMA	 4	/* Start DMA WRITE			      */
#define ST_W_STDMA	 5	/* Copy Read data to memory		      */
#define ST_R_DMA	 6	/* Read DMA processing			      */
#define ST_W_DMA	 7	/* Write DMA processing			      */
#define ST_R_COPY	 8	/* Copy READ data to memory		      */
#define ST_R_DMACONT	 9	/* Read DMA Completion			      */
#define ST_W_DMACONT	10	/* WRITE DMA Completion			      */
#define ST_ERR		11	/* DMA Error				      */
/*
 * Bottom Level State Machine
 */
#define ST_SP_ARB	 0	/* Arbitrate for the Bus		      */
#define ST_SP_SEL	 1	/* Select the target device		      */
#define ST_CMD_PHA	 2	/* Command Phase			      */
#define ST_DATAO_PHA	 3	/* Data Out Phase			      */
#define ST_DATAI_PHA	 4	/* Data In Phase			      */
#define ST_STATUS_PHA	 5	/* Status Phase				      */
#define ST_MESSI_PHA	 6	/* Message In Phase			      */
#define ST_MESSO_PHA	 7	/* Message Out Phase			      */
#define ST_RELBUS	 8	/* Release the Bus			      */
#define ST_ABORT	 9	/* Abort the transaction		      */
