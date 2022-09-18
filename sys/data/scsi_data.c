/*
 *	@(#)scsi_data.c	4.7	(ULTRIX)	2/21/91
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
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * scsi_data.c
 *
 * Modification history:
 *
 *    19-Feb-91  Bill Dallas
 *	Changed the string id for TSZ07 (CSS 9trk device).
 *
 *    12-Feb-91  Robin Miller
 *	Added reference to CD-ROM include file (h/cdrom.h).
 *	This support is only being added to the MIPS side at this time.
 *
 *    28-Jan-91  Robin Miller
 *	Modified default partition sizes for the RZ25 since another
 *	vendors' disk was chosen for this drive.
 *
 *    05-Nov-90  Robin Miller
 *	Changed block size for QIC-320 tapes from 1K to 512 bytes.
 *
 *    30-Oct-90  Robin Miller
 *	Added support for RZ25 disk drive.
 *
 *    10-Sep-90  Charles Richmond , IIS Corp
 *      Added includes and defines for kzqsa Qbus to SCSI
 *      Host adaptor.
 *
 *    07-Sept-90  Maria Vella
 * 	Added PDMA include file.
 *
 *    30-July-90  Robin Miller
 *	Added support for RRD42 and RX26 disk drives.
 *
 *    04-June-90  Bill Dallas 
 *	Added support for generic type devices. This is done
 * 	by a new table linked to the devtab. This allows for easy
 *	additions for QIC type units and new 9 track units with
 *	density selection support. This also allows for easy
 *	additions of disk drives.......... 
 *
 *   01-Mar-90  Janet L. Schank / Fred Canter
 *      Changed sz_wait_for_devices from 10 seconds to 5 seconds.
 *      Added sz_max_wait_for_devices (30 seconds).  This value
 *      is the maximum number of seconds to wait for a device that
 *      return busy status for the inquiry command.
 *
 *   23-Jan-90  Janet L. Schank
 *      Added an entry for TLZ04 (RDAT) to the scsi_devtab.  Changed
 *      sz_wait_for_devices to 10 seconds (the RDAT is slow).
 *
 *   05-Jan-90  Janet L. Schank
 *      Changed the size of sii_stray_intr from NSII to NSCSIBUS.
 *
 *   03-Jan-90  Janet L. Schank
 *      Changed the rzxx_sizes to use a 64Kb "b" partition.
 *
 *   21_Dec-89	John A. Gallant
 *	For the Exabyte, changed the VU value from 0x02 to 0x06, this will
 *	force even byte disconnects, and not hang the DS3100 systems.
 *
 *   20-Dec-89  Janet L. Schank
 *      Added the sii_stray_intr array to this file (moved from scsi_sii.c)
 *
 *   04-Dec-89  Janet L. Schank
 *      Modified the RZ24, RZ55, RZ56, and RZ57 default partition
 *      tables to use minimum 64Mb partition "b" size.
 *
 *	 10-Nov-89	Mitch McConnell
 *		Added include for ascreg.h.
 *
 *   17-Oct-89	Janet L. Schank / Art Zemon
 *	Added TZ05, RZ24, and RZ57 support.  Started a merge between
 *      the vax and mips sides.  This included the history list.
 *      Added the int default_scsiid which is used to hold the system's
 *      scsi bus id.  Added "How to add your own device."  Added an 'a'
 *      partition to the rrd40 partition table.
 *
 *   23-Jul-89	Fred Canter
 *	RX33 support.
 *
 *   17-Jul-89	Fred Canter
 *	Updated partition table sizes to correct values for base lever four.
 *
 *   14-Jul-89	Fred Canter
 *	Merged Dynamic BBR and error log changes.
 *
 *   23-Jun-89   John A. Gallant
 *	Added the buf structures for the bbr routine.
 *
 *   20-Jun-89	Fred Canter
 *	Added RZ56 support.
 *
 *   18-Jun-89	Fred Canter
 *	Convert to scsi_devtab.
 *
 *  17-Jun-89	Fred Canter
 *	Include errlog.h for binary error logging.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *  24-Apr-89	Fred Canter
 *	Added sizes table for RZ23s (SCSI floppy) disk.
 *
 *  31-Dec-88	Fred Canter
 *	 Include nexus.h for szreset() in scsi_5380.c.
 *
 *  18-Dec-88	Fred Canter
 *	Decided to keep save/restore data pointer message counters.
 *	So, moved them from scsi_5380.c to data file.
 *
 *  03-Nov-88	Alan Frechette
 *	Added in support for disk maintainence. Added data structures
 *	to handle disk maintainence.
 *
 *  20-Oct-88	Fred Canter
 *	Bug fix to last delta (lost the RZ55 H partition somehow).
 *
 *  16-Oct-88	Fred Canter
 *	Clean up comments. Add variable to control length of
 *	SCSI bus reset. Add partition table for non-DEC disks.
 *
 *  28-Sep-88	Fred Canter
 *	Clean up comments. Reduce delay after bus reset before 
 *	accessing devices from 7 seconds to 5 seconds.
 *
 *  14-Sep-88   Fred Canter
 *	Minor change to RZ55 partition table associated comments.
 *
 *  06-Sep-88   Stephen Reilly
 *	Modified RZ55 disk partition tables.
 *
 *  19-Aug-88	Fred Canter
 *	Merge 5380 and SII SCSI drivers (add include siireg.h).
 *	Added #include 5380reg.h (part of split driver source files).
 *
 *  18-Aug-88	Fred Canter
 *	Added #include shm.h and ipc.h as part of fix for BAR 435.
 *
 *  16-Jul-88	Fred Canter
 *	Finialize RZ23/RZ23/RZ55/RRD40 disk partition tables.
 *	Handle unit attn (offline/online).
 *
 *  28-Jun-88	Fred Canter
 *	Restructure ??command() handling. Save status and sense key.
 *
 *  18-Jun-88	Fred Canter
 *	Added RZ55 sizes table.
 *
 *  06-Jun-88	Fred Canter
 *	Created this data file for scsi.c from stc_data.c
 *
 */

/*
 * How to add "your own" SCSI (disk or tape) device:
 *
 *    For those who would like to connect their own (non-DEC) devices to the
 *    SCSI bus the ULTRIX driver allows the addition and definition of new
 *    devices in the "scsi_data.c" file supplied with every binary or
 *    source license.
 *
 *    A good starting place would be the "UNKNOWN DISK" or "UNKNOWN TAPE"
 *    entries in scsi_devtab.  A new entry should be added to scsi_devtab
 *    describing the new device.  An example disk entry might look like
 *    the following:
 *
 *    struct scsi_devtab scsi_devtab[] = {
 *
 *         {"VID     DEVICEID", 16, DEV_RZxx, RZxx, sz_rzxx_sizes, 0, 0,
 *          SCSI_TRYSYNC | SCSI_STARTUNIT | SCSI_REQSNS | SCSI_TESTUNITREADY |
 *          SCSI_READCAPACITY, 0 },
 *
 *         { 0 },
 *
 *    };
 *
 *    The first field is the vendor returned string identifying the drive.
 *    This string is composed of the vendor id (first 8 chars) followed by
 *    the product id.
 *
 *    The second field is the length of the string.
 *
 *    The third field is the ULTRIX name for the device (DEV_RZxx for
 *    disks, DEV_TZxx for tapes, and DEV_CDxx for cd roms).
 *
 *    The fourth field is the class of SCSI device (RZxx for disks, TZxx for
 *    tapes, and CDxx for cd roms).
 *    
 *    The fifth field is the partition table entry.  Tape units should use
 *    sz_null_sizes.  Disk devices may use sz_rzxx_sizes.  The sz_rzxx_sizes
 *    assumes that the disk is at least 48Mb.  DO NOT MODIFY sz_rzxx_sizes!
 *    To create your own partition table, you must make an entry in the
 *    "size" struct below.
 *
 *    The sixth field is the probedelay.  The probedelay is the number of
 *    microseconds which will be delayed after the probe on this device.
 *
 *    The seventh field is an index into the sz_dk_mspw array.  This array
 *    is used by the iostat program (vax only).  Use '0' to specify no
 *    iostat.  If you would like to make an entry, go to the sz_dk_mspw
 *    array in this file and follow the directions.  If you make an addition
 *    be sure to put it at the end of the array.
 *    
 *    The eighth field is a mask of bits that defines what action(s) the
 *    driver's probe routine should take upon system boot. All the currently
 *    defined fields are:
 *
 *    SCSI_REQSNS         - send a REQUEST SENSE SCSI command
 *    SCSI_STARTUNIT      - send a START/STOP UNIT SCSI command
 *    SCSI_TRYSYNC        - set the device up as synchronous
 *    SCSI_TESTUNITREADY  - send a TEST UNIT READY SCSI command
 *    SCSI_READCAPACITY   - send a READ CAPACITY SCSI command
 *    SCSI_NODIAG         - don't issue RECEIVE DIAGNOSTIC command (tapes only)
 *    SCSI_MODSEL_PF	  - set the PF bit when sending a mode select command
 *    SCSI_REMOVABLE_DISK - the disk has removable media
 *    SCSI_NODBBR         - the device doesn't support bad block replacement
 *
 *    A detailed description of these flags may be found in "scsireg.h."
 *
 *	The last field is the address of the options table. This allows the
 *	addition and density selection of an array of various types of tape
 * 	drives. This option field is also used for disks.
 *	The field can either be NULL (0) for no options/density or
 *	the address of the option structure for this type tape unit.
 *	tzk10_opt or rz55_opt etc. 
 *	Please see the scsireg.h for a complete description. 
 *
 */

#include "sii.h"

#ifdef vax
#include "scsi.h"
#define NASC 0
#define NKZQ 0
#else 
#define NSCSI 0
#include "kzq.h"
#include "asc.h"
#endif vax

#define NSCSIBUS (NSII+NSCSI+NASC+NKZQ)

#ifndef FORMAT
#define FORMAT 1
#endif

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/conf.h"
#include "../h/kmalloc.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/dk.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../fs/ufs/fs.h"
#include "../h/mtio.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../../machine/common/cpuconf.h"
#include "../h/devio.h"
#include "../h/ipc.h"
#include "../h/shm.h"

#ifdef vax
#include "../machine/nexus.h"
#include "../machine/mtpr.h"
#endif vax
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../machine/cpu.h"
#ifdef mips
#include "../io/scsi/mips/scsivar.h"
#include "../io/scsi/mips/scsireg.h"
#include "../io/scsi/mips/siireg.h"
#include "../io/scsi/mips/pdma.h"
#include "../io/scsi/mips/kzqreg.h"
#include "../io/scsi/mips/ascreg.h"
#endif mips
#ifdef vax
#include "../io/scsi/vax/scsivar.h"
#include "../io/scsi/vax/scsireg.h"
#include "../io/scsi/vax/5380reg.h"
#include "../io/scsi/vax/siireg.h"
#endif vax

#ifdef mips
#include "../h/cdrom.h"
#endif /* mips */
#include "../h/rzdisk.h"
#include "../h/errlog.h"

#ifdef	BINARY

/*
 * The system's SCSI bus id.
 */
extern int default_scsiid;

/*
 * Number of seconds to wait for SCSI devices
 * to come ready enough to respond to inquiry
 * after bus reset. Needs to be a variable so
 * users can change it.
 * Max is the maximum number of seconds to wait for a
 * device that returns busy status for the inquiry command.
 */
extern int sz_wait_for_devices;
extern int sz_max_wait_for_devices;

/*
 * Length of SCSI bus reset in Microseconds.
 * Factory default is 50 Usec.
 */
extern int sz_reset_width;

#ifdef vax
/*
 * Save/Restore Data Pointers messages counters
 * (see scsi_5380.c).
 */
extern	int	sz_sdp[];
extern	int	sz_rdp[];
extern	int	sz_isdp[];
#endif vax

extern	struct	pt sz_part[];		/* Partition table for each disk */
extern	struct size {
	daddr_t	nblocks;
	int	blkoffs;
} sz_rz22_sizes[], sz_rz23_sizes[], sz_rz24_sizes[],
sz_rz55_sizes[], sz_rz56_sizes[], sz_rz57_sizes[],
sz_rzxx_sizes[], sz_rrd40_sizes[], sz_rx23_sizes[],
sz_rz23l_sizes[], sz_null_sizes[], sz_rx33_sizes[],
sz_rx26_sizes[], sz_rz25_sizes[];

extern	struct	buf	rszbuf[];
extern	struct	buf	cszbuf[];
extern	struct	buf	bszbuf[];
extern	struct	uba_ctlr *szminfo[];
extern	struct	uba_device *szdinfo[];
extern	struct	buf szutab[];
extern	struct	sz_softc sz_softc[];
extern	int	sz_unit_rcvdiag[];
extern	int	sz_unit_online[];
extern	int	sz_retries[];
extern  int     sii_stray_intr[];
extern  int     kzq_stray_intr[];
extern  int     nNASC;
extern  int     nNKZQ;
extern  int     nNSII;
extern	int	nNSCSI;
extern  int     nNSCSIBUS;
extern	int	nNSZ;
extern	char	dis_eot_sz[];
extern	struct	scsi_devtab scsi_devtab[];
extern	int	tz_exabyte_vu;
extern	int	tz_exabyte_mt;
extern	int	tz_exabyte_rt;
extern	int	tz_exabyte_modsns;
extern struct 	tape_opt_tab tzk10_fixed_opt, tzk10_var_opt, viper_2525_opt,
		tz05_opt, tz07_opt, tzk08_opt;
extern struct 	disk_opt_tab rz55_opt;

#ifdef vax
extern	float	sz_dk_mspw[];
#endif vax

#else BINARY

/*
 * The system's SCSI bus id.
 */
int default_scsiid = DEFAULT_SCSIID;

/*
 * Number of seconds to wait for SCSI devices
 * to come ready enough to respond to inquiry
 * after bus reset. Needs to be a variable so
 * users can change it.
 * Factory default for sz_wait_for_devices is 5 seconds.
 * sz_max_wait_for_devices is the maximum number of seconds to wait
 * for a device that returns busy status for the inquiry command.
 * sz_max_wait_for_devices MUST be greater than sz_wait_for_devices.
 * Factory default is 30 seconds.
 */
int	sz_wait_for_devices = 5;
int	sz_max_wait_for_devices = 30;

/*
 * Length of SCSI bus reset in Microseconds.
 * Factory default is 50 Usec.
 */
int	sz_reset_width = 50;

#ifdef vax
/*
 * Save/Restore Data Pointers messages counters
 * (see scsi_5380.c).
 */
int	sz_sdp[NSCSIBUS*NDPS];
int	sz_rdp[NSCSIBUS*NDPS];
int	sz_isdp[NSCSIBUS*NDPS];
#endif vax

struct 	pt  sz_part[NSCSIBUS*NDPS];	/* Partition table for each disk */

/*
 * Default partition tables to be used of the
 * disk does not contain a parition table.
 */
struct size {
	daddr_t	nblocks;
	int	blkoffs;
} sz_rz22_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	-1,	32768,		/* B=blk 32768 thru end (102431) */
	-1,	0,		/* C=blk 0 thru end (102431) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
}, sz_rz23_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	66690,	32768,		/* B=blk 32768 thru 99457 */
	-1,	0,		/* C=blk 0 thru end (204863) */
	35135,	99458,		/* D=blk 99458 thru 134592 */
	35135,	134593,		/* E=blk 134593 thru 169727 */
	-1,	169728,		/* F=blk 169728 thru end (204863) */
	-1,	99458,		/* G=blk 99458 thru end (204863) */
	-1,	134593,		/* H=blk 134593 thru end (204863) */
}, sz_rz23l_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	66690,	32768,		/* B=blk 32768 thru 99457 */
	-1,	0,		/* C=blk 0 thru end (237588) */
	35135,	99458,		/* D=blk 99458 thru 134592 */
	35135,	134593,		/* E=blk 134593 thru 169727 */
	-1,	169728,		/* F=blk 169728 thru end (237588) */
	-1,	99458,		/* G=blk 99458 thru end (237588) */
	-1,	134593,		/* H=blk 134593 thru end (237587) */
}, sz_rz24_sizes[8] = {
	32768,  0,              /* A=blk 0 thru 32767 */
	131072, 32768,          /* B=blk 32768 thru 163839 */
	-1,	0,		/* C=blk 0 thru end (409791) */
	81984,	163840,         /* D=blk 163840 thru 245823 */
	81984,	245824,         /* E=blk 245824 thru 327807 */
	-1,	327808,         /* F=blk 327808 thru end (409791) */
	-1, 	163840,         /* G=blk 163840 thru end (409791) */
	0,	0,              /* H=zero for default */
}, sz_rz55_sizes[8] = {
	32768,  0,              /* A=blk 0 thru 32767 */
	131072, 32768,          /* B=blk 32768 thru 163839 */
	-1, 	0,	        /* C=blk 0 thru end (649039) */
	152446, 163840,         /* D=blk 163840 thru 316285 */
	152446, 316286,         /* E=blk 316286 thru 468731 */
	-1, 	468732,         /* F=blk 468732 thru end (649039) */
	-1, 	163840,         /* G=blk 163840 thru end (649039) */
	0, 	0,          	/* H=zero for default */
}, sz_rz56_sizes[8] = {
	32768,  0,              /* A=blk 0 thru 32767 */
	131072, 32768,          /* B=blk 32768 thru 163839 */
	-1, 	0,	        /* C=blk 0 thru end (1299173) */
	292530, 163840,         /* D=blk 163840 thru 456369 */
	292530, 456370,         /* E=blk 456370 thru 748899 */
	-1,	748900,         /* F=blk 748900 thru end (1299173) */
	567666,	163840,         /* G=blk 163840 thru 731505 */
	-1, 	731506,       	/* H=blk 731506 thru end (1299173) */
}, sz_rz57_sizes[8] = {
	32768,  0,              /* A=blk 0 thru 32767 (16Mb) */
	184320, 32768,          /* B=blk 32768 thru 217087 (90Mb) */
	-1,	0,		/* C=blk 0 thru 2025787 (989.15Mb) */
	299008,	831488,         /* D=blk 831488 thru 1130495 (146Mb) */
	299008,	1130496,        /* E=blk 1130469 thru 1429503 (146Mb) */
	-1,	1429504,        /* F=blk 1429504 thru 2025787 (291.15Mb) */
	614400, 217088,         /* G=blk 217088 thru 831487 (300Mb) */
	-1,	831488,         /* H=blk 831488 thru 2025787 (583.15Mb) */
}, sz_rzxx_sizes[8] = {
	32768,  0,              /* A=blk 0 thru 32767 */
	131072, 32768,          /* B=blk 32768 thru 163839 */
	-1,	0,		/* C=blk 0 thru end (?) */
	0,	0,
	0,	0,
	0,	0,
	-1, 	163840,         /* G=blk 163840 thru end (?) */
	0,	0,
}, sz_rrd40_sizes[8] = {
	-1,	0,              /* A=blk 0 thru end (CDROM size variable) */
	0,	0,
	-1,	0,		/* C=blk 0 thru end (CDROM size variable) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
}, sz_rx23_sizes[8] = {
	-1,	0,		/* A=blk 0 thru end (2879 for hi density) */
	0,	0,
	-1,	0,		/* C=blk 0 thru end (2879 for hi density) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
}, sz_rx33_sizes[8] = {
	-1,	0,		/* A=blk 0 thru end (2399 for hi density) */
	0,	0,
	-1,	0,		/* C=blk 0 thru end (2399 for hi density) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
}, sz_rx26_sizes[8] = {
	-1,	0,		/* A=blk 0 thru end (5759 for extra density) */
	0,	0,
	-1,	0,		/* C=blk 0 thru end (5759 for extra density) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
}, sz_rz25_sizes[8] = {
	32768,  0,              /* A=blk 0 thru 32767 */
	131072, 32768,          /* B=blk 32768 thru 163839 */
	-1, 	0,	        /* C=blk 0 thru end (832526) */
	222896, 163840,         /* D=blk 163840 thru 386735 */
	222896, 386736,         /* E=blk 370093 thru 609631 */
	-1, 	609632,         /* F=blk 576346 thru end (832526) */
	-1, 	163840,         /* G=blk 163840 thru end (832526) */
	0, 	0,          	/* H=zero for default */
}, sz_null_sizes[1] = {		/* Dummy for tapes */
	0,	0,
};

struct	buf	rszbuf[NSCSIBUS*NDPS];
struct	buf	cszbuf[NSCSIBUS*NDPS];
struct	buf	bszbuf[NSCSIBUS*NDPS];
struct	uba_ctlr *szminfo[NSCSIBUS];
struct	uba_device *szdinfo[NSCSIBUS*NDPS];
struct	buf szutab[NSCSIBUS*NDPS];
struct	sz_softc sz_softc[NSCSIBUS];
int	sz_unit_rcvdiag[NSCSIBUS*NDPS];
int	sz_unit_online[NSCSIBUS*NDPS];
int	sz_unit_attn[NSCSIBUS*NDPS];
int	sz_retries[NSCSIBUS*NDPS];
int     sii_stray_intr[NSCSIBUS];
int     kzq_stray_intr[NSCSIBUS];
int     nNASC = NASC;
int     nNSII = NSII;
int     nNKZQ = NKZQ;
int	nNSCSI = NSCSI;
int     nNSCSIBUS = NSCSIBUS;
int	nNSZ = NSCSIBUS*NDPS;
char	dis_eot_sz[NSCSIBUS*NDPS];

/* 
 * SCSI tape option table.
*/

struct tape_opt_tab

/* QIC UNITS KNOWN */

/* DEC TZK10 FIXED BLOCK SIZES */

tzk10_fixed_opt = {

/*****************************   opt_flags  ***********************************/
 MSEL_PLL_VAL | MSEL_BLKDL_VAL | MSNS_ALLOCL_VAL | RSNS_ALLOCL_VAL | SCSI_QIC
  | BUF_MOD, 

/* msel_pll	msel_blkdl	msel_vul	msns_allocl	rsns_allocl   */
   12,		8,		0,		12,		SZ_RQSNS_LEN,

/* page_size	rserv1		rserv2					      */
   0,		0,		0,

/* Density specs for rmt0l or bits 5 & 4 of minor number (00)		      */
/* tape_flags  								      */
{   DENS_VAL | ONE_FM ,

/* dens			blk_size		speed			      */
   SCSI_QIC_120_DENS, 	SCSI_QIC_FIXED, 	0},

/* Density specs for rmt0h or bits 5 & 4 of minor number (01)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM ,

/* dens			blk_size		speed			      */
   SCSI_QIC_320_DENS, 	SCSI_QIC_FIXED,		0},

/* Density specs for rmt0m or bits 5 & 4 of minor number (10)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM ,

/* dens			blk_size		speed			      */
   SCSI_QIC_150_DENS, 	SCSI_QIC_FIXED,		0},

/* Density specs for rmt0a or bits 5 & 4 of minor number (11)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM ,

/* dens			blk_size		speed			      */
   SCSI_QIC_24_DENS, 	SCSI_QIC_FIXED,		0}


/* END OF DEC TZK10 FIXED BLOCK SIZES */
},

/* DEC TZK10 Fixed blocks 24,120,150 QIC 320 is varible block */

tzk10_var_opt = {

/*****************************   opt_flags  ***********************************/
 MSEL_PLL_VAL | MSEL_BLKDL_VAL | MSNS_ALLOCL_VAL | RSNS_ALLOCL_VAL | SCSI_QIC 
  | BUF_MOD,

/* msel_pll	msel_blkdl	msel_vul	msns_allocl	rsns_allocl   */
   12,		8,		0,		12,		SZ_RQSNS_LEN,

/* page_size	rserv1		rserv2					      */
   0,		0,		0,

/* Density specs for rmt0l or bits 5 & 4 of minor number (00)		      */
/* tape_flags  								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_120_DENS, 	SCSI_QIC_FIXED,		0},

/* Density specs for rmt0h or bits 5 & 4 of minor number (01)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_320_DENS, 	SCSI_VARIABLE,		0},

/* Density specs for rmt0m or bits 5 & 4 of minor number (10)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_150_DENS, 	SCSI_QIC_FIXED,		0},

/* Density specs for rmt0a or bits 5 & 4 of minor number (11)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_24_DENS, 	SCSI_QIC_FIXED,		0}

/* END OF DEC TZK10 Varible block QIC 320 density */
},

/* VIPER 2525 FIXED BLOCK SIZES */

viper_2525_opt = {

/*****************************   opt_flags  ***********************************/
 MSEL_PLL_VAL | MSEL_BLKDL_VAL | MSEL_VUL_VAL | MSNS_ALLOCL_VAL | 
RSNS_ALLOCL_VAL | SCSI_QIC | BUF_MOD, 

/* msel_pll	msel_blkdl	msel_vul	msns_allocl	rsns_allocl   */
   15,		8,		2,		15,		SZ_RQSNS_LEN,

/* page_size	rserv1		rserv2					      */
   0,		0,		0,

/* Density specs for rmt0l or bits 5 & 4 of minor number (00)		      */
/* tape_flags  								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_120_DENS, 	SCSI_QIC_FIXED,		0},

/* Density specs for rmt0h or bits 5 & 4 of minor number (01)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_320_DENS, 	SCSI_QIC_320_FIXED,	0},

/* Density specs for rmt0m or bits 5 & 4 of minor number (10)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_150_DENS, 	SCSI_QIC_FIXED,		0},

/* Density specs for rmt0a or bits 5 & 4 of minor number (11)		      */
/* tape_flags								      */
{   DENS_VAL | ONE_FM,

/* dens			blk_size		speed			      */
   SCSI_QIC_24_DENS, 	SCSI_QIC_FIXED,		0}

/* END OF VIPER 2525 FIXED BLOCK SIZES */
},

/*    SOME 9 track drives	*/

/* DEC's TZ05 single density */

tz05_opt = {

/*****************************   opt_flags  ***********************************/
 MSEL_PLL_VAL | MSEL_BLKDL_VAL | MSNS_ALLOCL_VAL | RSNS_ALLOCL_VAL | SCSI_9TRK
  | BUF_MOD, 

/* msel_pll	msel_blkdl	msel_vul	msns_allocl	rsns_allocl   */
   12,		8,		0,		12,		SZ_RQSNS_LEN,

/* page_size	rserv1		rserv2					      */
   0,		0,		0,

/* Density specs for rmt0l or bits 5 & 4 of minor number (00)		      */
/* tape_flags  								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_1600, 		SCSI_VARIABLE,		2},

/* Density specs for rmt0h or bits 5 & 4 of minor number (01)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_1600,		SCSI_VARIABLE,		2},

/* Density specs for rmt0m or bits 5 & 4 of minor number (10)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_1600, 		SCSI_VARIABLE,		2},

/* Density specs for rmt0a or bits 5 & 4 of minor number (11)		      */
/* tape_flags								      */
{   DENS_VAL |SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_1600, 		SCSI_VARIABLE,		2}

/* END OF DEC's TZ05  */
},

/* DEC's TZ07 Dual density 1600 and 6250 */

tz07_opt = {

/*****************************   opt_flags  ***********************************/
 MSEL_PLL_VAL | MSEL_BLKDL_VAL  | MSNS_ALLOCL_VAL | RSNS_ALLOCL_VAL | SCSI_9TRK
  | BUF_MOD, 

/* msel_pll	msel_blkdl	msel_vul	msns_allocl	rsns_allocl   */
   12,		8,		0,		12,		SZ_RQSNS_LEN,

/* page_size	rserv1		rserv2					      */
   0,		0,		0,

/* Density specs for rmt0l or bits 5 & 4 of minor number (00)		      */
/* tape_flags  								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_1600,   	SCSI_VARIABLE,		2},

/* Density specs for rmt0h or bits 5 & 4 of minor number (01)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_6250,   	SCSI_VARIABLE,		2},

/* Density specs for rmt0m or bits 5 & 4 of minor number (10)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_6250, 		SCSI_VARIABLE,		2},

/* Density specs for rmt0a or bits 5 & 4 of minor number (11)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   SCSI_6250, 		SCSI_VARIABLE,		2}

/* END OF DEC's TZ07 9 Track tape */
},

/* EXABYTE - TZK08's */
/* Please note the msel_vul field ... since this device is using
 * the field as a bit patten  and not a length we need to set these
 * bits... the rest of the vu takes the defaults.
*/

tzk08_opt = {

/*****************************   opt_flags  ***********************************/
 MSEL_PLL_VAL | MSEL_BLKDL_VAL  | MSNS_ALLOCL_VAL | RSNS_ALLOCL_VAL |
 MSEL_VUL_VAL| SCSI_8MM | BUF_MOD, 

/* msel_pll	msel_blkdl	msel_vul	msns_allocl	rsns_allocl   */
   13,		8,		6,		16,		SZ_RQSNS_LEN,

/* page_size	rserv1		rserv2					      */
   0,		0,		0,

/* Density specs for rmt0l or bits 5 & 4 of minor number (00)		      */
/* tape_flags  								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   0,			SCSI_VARIABLE,		0},

/* Density specs for rmt0h or bits 5 & 4 of minor number (01)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   0,			SCSI_VARIABLE,		0},

/* Density specs for rmt0m or bits 5 & 4 of minor number (10)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   0,	 		SCSI_VARIABLE,		0},

/* Density specs for rmt0a or bits 5 & 4 of minor number (11)		      */
/* tape_flags								      */
{   DENS_VAL | SPEED_VAL ,

/* dens			blk_size		speed			      */
   0,			SCSI_VARIABLE,		0}

/* END OF Exabytes's TZK08 8MM tape */
}

/* add your own here */

/* end of tape_opt_table */
;

/*
 * Disk option table
*/


struct disk_opt_tab 

/* DEC's RZ55 */

rz55_opt = {

/*****************************   opt_flags  ***********************************/
 MSEL_PLL_VAL | MSEL_BLKDL_VAL | MSNS_ALLOCL_VAL | RSNS_ALLOCL_VAL | PAGE_VAL ,

/* msel_pll	msel_blkdl	msel_vul	msns_allocl	rsns_allocl   */
   12,		8,		0,		12,		SZ_RQSNS_LEN,

/* page_size	rserv1		rserv2					      */
   32,		0,		0

/* END OF DEC's RZ55  */
}

/* add your own here */

/* end of disk option table */
;


/*
 * SCSI device information table.
 */
struct  scsi_devtab scsi_devtab[] = {

/* DISKS: */

/* RZ55 */
{"DEC     RZ55", 12, DEV_RZ55, RZ55, sz_rz55_sizes, 0, 3,
#ifdef vax
	SCSI_STARTUNIT|SCSI_TRYSYNC, NO_OPTTABLE },
#endif vax
#ifdef mips
	SCSI_TRYSYNC|SCSI_STARTUNIT|SCSI_REQSNS|
		 SCSI_TESTUNITREADY|SCSI_READCAPACITY, NO_OPTTABLE },
#endif mips

/* RZ56 */
{"DEC     RZ56", 12, DEV_RZ56, RZ56, sz_rz56_sizes, 0, 6,
#ifdef vax
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_TRYSYNC, NO_OPTTABLE },
#endif vax
#ifdef mips
	SCSI_TRYSYNC|SCSI_STARTUNIT|SCSI_REQSNS|
		 SCSI_TESTUNITREADY|SCSI_READCAPACITY|SCSI_MODSEL_PF, NO_OPTTABLE },
#endif mips

/* RZ57 */
{"DEC     RZ57", 12, DEV_RZ57, RZ57, sz_rz57_sizes, 0, 9,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_TRYSYNC, NO_OPTTABLE },

/* RZ23L */
/*
 * Note -- This entry must appear before RZ23, or else the RZ23L will
 *         match on the RZ23.
 */
{"DEC     RZ23L", 13, DEV_RZ23L, RZ23L, sz_rz23l_sizes, 0, 10,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_TRYSYNC },

/* RZ23 */
#ifdef vax
{"DEC     RZ23", 12, DEV_RZ23, RZ23, sz_rz23_sizes, 0, 2,
	SCSI_STARTUNIT, NO_OPTTABLE },
#endif vax
#ifdef mips
{"DEC     RZ23", 12, DEV_RZ23, RZ23, sz_rz23_sizes, 1000000, 2,
	SCSI_REQSNS|SCSI_STARTUNIT, NO_OPTTABLE }, 
#endif mips

/* RZ22 */
#ifdef vax
{"DEC     RZ22", 12, DEV_RZ22, RZ22, sz_rz22_sizes, 0, 1,
	SCSI_STARTUNIT, NO_OPTTABLE },
#endif vax
#ifdef mips
{"DEC     RZ22", 12, DEV_RZ22, RZ22, sz_rz22_sizes, 1000000, 1,
	SCSI_REQSNS|SCSI_STARTUNIT, NO_OPTTABLE }, 
#endif mips

/* RX23 */
#ifdef vax
{"DEC     RX23", 12, DEV_RX23, RX23, sz_rx23_sizes, 0, 4,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE },
#endif vax
#ifdef mips
/* RX23 */
{"DEC     RX23", 12, DEV_RX23, RX23, sz_rx23_sizes, 1000000, 4,
	SCSI_REQSNS|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE }, 
#endif mips

/* RZ24 */
{"DEC     RZ24", 12, DEV_RZ24, RZ24, sz_rz24_sizes, 0, 8,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_TRYSYNC, NO_OPTTABLE },

/* RZ25 */
{"DEC     RZ25", 12, DEV_RZ25, RZ25, sz_rz25_sizes, 0, 13,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_TRYSYNC, NO_OPTTABLE },

/* RX26 */
#ifdef vax
{"DEC     RX26", 12, DEV_RX26, RX26, sz_rx26_sizes, 0, 12,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE },
#endif vax
#ifdef mips
{"DEC     RX26", 12, DEV_RX26, RX26, sz_rx26_sizes, 1000000, 12,
	SCSI_REQSNS|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE }, 
#endif mips

/* RX33 */
#ifdef vax
{"DEC     RX33", 12, DEV_RX33, RX33, sz_rx33_sizes, 0, 7,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE },
#endif vax
#ifdef mips
{"DEC     RX33", 12, DEV_RX33, RX33, sz_rx33_sizes, 1000000, 7,
	SCSI_REQSNS|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE }, 
#endif mips

/* UNKNOWN DISK */
{"UNKNOWN", 7, DEV_RZxx, RZxx, sz_rzxx_sizes, 0, 14,
	SCSI_STARTUNIT|SCSI_REQSNS|SCSI_MODSEL_PF|SCSI_NODBBR, NO_OPTTABLE },

/* CDROMS: */

/* RRD40 */
{"DEC     RRD40", 13, DEV_RRD40, RRD40, sz_rrd40_sizes, 0, 5,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE },
{"DEC     RRD42", 13, DEV_RRD42, RRD42, sz_rrd40_sizes, 0, 11,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE },
{"SONY    CD-ROM", 14, DEV_RRD42, RRD42, sz_rrd40_sizes, 0, 11,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE },

/* UNKNOWN CDROM */
{"UNKNOWN", 7, DEV_RZxx, CDxx, sz_rrd40_sizes, 0, 11,
	SCSI_STARTUNIT|SCSI_MODSEL_PF|SCSI_REMOVABLE_DISK|SCSI_NODBBR, NO_OPTTABLE },

/* TAPES: */

/* TZK50 */
{ 0, 0x50, DEV_TK50, TZK50, sz_null_sizes, 0, 0, 0, NO_OPTTABLE },

/* TZK30 */
{ 0, 0x30, DEV_TZ30, TZ30, sz_null_sizes, 0, 0, 0, NO_OPTTABLE },

/* TLZ04 - RDAT drive */
{"DEC     TLZ04", 13, DEV_TLZ04, TLZ04, sz_null_sizes, 0, 0, 
SCSI_NODIAG, NO_OPTTABLE}, 

/* TZ05 - alias TZX0 */ 
{"NCR H621", 8, DEV_TZ05, TZ05, sz_null_sizes, 0, 0, SCSI_NODIAG,
 (caddr_t *)&tz05_opt}, 

/* TZ07 - Dual density 100ips 9track */
{"DEC     TSZ07", 13, DEV_TZ07, TZ07, sz_null_sizes, 0, 0, SCSI_NODIAG, 
(caddr_t *)&tz07_opt}, 

/* TZK08 8mm drive */

/* TZK10 - DEC's QIC tape drive */ 
{"DEC     TZK10", 13, DEV_TZK10, TZK10, sz_null_sizes, 0, 0,
SCSI_MODSEL_PF | SCSI_NODIAG, (caddr_t *)&tzk10_fixed_opt}, 

/* TZK10 - DEC's QIC tape drive without the rom change */ 
{"TANDBERG TDC 3800", 17, DEV_TZK10, TZK10, sz_null_sizes, 0, 0,
SCSI_MODSEL_PF | SCSI_NODIAG, (caddr_t *)&tzk10_fixed_opt}, 

/* Viper 2525 - ARCHIVES Viper 2525 QIC tape drive */ 
{"ARCHIVE VIPER", 13, DEV_TZQIC, TZQIC, sz_null_sizes, 0, 0,
SCSI_NODIAG, (caddr_t *)&viper_2525_opt}, 


/* EXABYTE EXB-8200 */
{"EXABYTE EXB-8200", 16, DEV_TZK08, TZK08, sz_null_sizes, 0, 0,
	SCSI_NODIAG|SCSI_MODSEL_EXABYTE, (caddr_t *)&tzk08_opt},

/* GENERIC QIC __ FILL IN AND UNCOMMENT */
/*
{"VENDOR MODEL", 16 ??, DEV_TZQIC, TZQIC, sz_null_sizes, 0, 0, FLAGS,
ADDR of correct QIC tape_opt_tab entry },
*/

/* GENERIC 9 Track __ FILL IN AND UNCOMMENT */
/*
{"VENDOR MODEL", 16 ??, DEV_TZ9TRK, TZ9TRK, sz_null_sizes, 0, 0, FLAGS,
ADDR of correct 9 track tape_opt_tab entry },
*/

/* GENERIC 8 mm __ FILL IN AND UNCOMMENT */
/*
{"VENDOR MODEL", 16 ??, DEV_TZ8MM, TZ8MM, sz_null_sizes, 0, 0, FLAGS,
ADDR of correct 8 mm tape_opt_tab entry },
*/

/* GENERIC RDAT __ FILL IN AND UNCOMMENT */
/*
{"VENDOR MODEL", 16 ??, DEV_TZRDAT, TZRDAT, sz_null_sizes, 0, 0, FLAGS,
ADDR of correct RDAT tape_opt_tab entry },
*/
/* TZxx - UNKNOWN TAPE */
{"UNKNOWN", 7, DEV_TZxx, TZxx, sz_null_sizes, 0, 0, SCSI_NODIAG, NO_OPTTABLE },

{ 0 },
};

/*
 * Vendor unique mode select parameters for the EXABYTE EXB-8200
 * tape drive. Enabled with SCSI_MODSEL_EXABYTE flag.
 * NOTE: default thresholds are for revision 23 or 24 drive.
 * NOTE: optimum values depend on I/O speed of the system,
 *	 setting mt and rt to 0x20 works best for the VS3100).
 *	 The default settings work well on the DS3100.
 * NOTE: if tz_exabyte_modsns is set a mode sense will be done for each
 *	 tzopen(). The drive's current values for vu, mt, and rt will
 *	 be printed. You can set tz_exabyte_modsns with the debugger.
 */
int	tz_exabyte_vu = 0x06;	/* Vendor unique bits (even byte and parity)*/
int	tz_exabyte_mt = 0x080;	/* Default motion threshold = 0x080	    */
int	tz_exabyte_rt = 0x0a0;	/* Default reconnect threshold = 0x0a0	    */
int	tz_exabyte_modsns = 0;	/* If 1, do mode sense on each tzopen	    */

#ifdef vax
/*
 * Disk milliseconds per word transfer rate for iostat.
 * The rate calculation is based on words per one disk revolution.
 * The RZxx entry is for new or user disks. The size of the table
 * is not fixed, more entries may be added as needed.
 * NOTE: the driver changes the rate for RX23 9 sector diskettes.
 */
float	sz_dk_mspw[] = {           /* 1.0 / (60 * sectors/track * 256) */
	0.0,		/* 0  - no iostat (tapes and unknown devices)	    */
	0.000002,	/* 1  - RZ22  (1.0 / (60 * 33 * 256))		    */
	0.000002,	/* 2  - RZ23  (1.0 / (60 * 33 * 256))		    */
	0.0000018,	/* 3  - RZ55  (1.0 / (60 * 36 * 256))		    */
	0.0000434,	/* 4  - RX23  (1.0 / (5 * 18 * 256))		    */
	0.0000113,	/* 5  - RRD40 (176.4 KB/SEC)			    */
	0.0000012,	/* 6  - RZ56  (1.0 / (60 * 54 * 256))		    */
	0.0000434,	/* 7  - RX33  (1.0 / (6 * 15 * 256))		    */
	0.0000017,      /* 8  - RZ24  (1.0 / (60 * 38 * 256))               */
	0.0000009,      /* 9  - RZ57  (1.0 / (60 * 71 * 256))               */
	0.0000017,      /* 10 - RZ23L (1.0 / (60 * 39 * 256))               */
	0.0000133,	/* 11 - RRD42 (150.0 KB/SEC)			    */
	0.0000217,	/* 12 - RX26  (1.0 / ((300 / 60) * 36 * 256))	    */
	0.0000013,	/* 13 - RZ25  (1.0 / (60 * 48 * 256))		    */
	0.0,		/* 14 - RZxx					    */
};
#endif vax

#endif	BINARY
