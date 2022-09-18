#ifndef lint
static  char    *sccsid = "@(#)mscp_data.c	4.6	(ULTRIX)	2/12/91";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1985 - 1989 by                    *
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
 *   Abstract:	This module contains the disk class driver data
 *		structures.
 *
 *   Author:	David E. Eiche	Creation Date:	September 30, 1985
 *
 *   History:
 *
 *   11-Feb-1991	Pete Keilty
 *	The size of the RSPID table is now done here so it could be 
 *	adjusted on site if needed. 
 *
 *   14-Jan-91		Brian Nadeau
 *	Added support for the TA91
 *
 *   07-Jan-91		Brian Nadeau
 *	Change rah72 name to ra71.
 *
 *   02-Feb-90		Brian Nadeau
 *	Added default partition table entry for the ra72 and rah72.
 *
 *   Aug 1990		Matthew Sacks
 *	Correct the comment on the size of the ese20 "c" partition, so
 *	that it says 245757 instead of 245760.  The last 3 blocks are used
 *	for a fake RDT.
 *
 *   27-Dec-1989	David E. Eiche		DEE0081
 *	Change format of controller model table to eliminate unused
 *	config name, bus type and host timeout fields.  Also add entries
 *	for HSC60 and HSC90 controllers.
 *
 *   01-Dec-1989	Tim Burke
 *	Increased the size of the "b" partition of the ra70, rf31 and rf71
 *	to be 64MB.  Also changed the "g" partition of these disks.  Changed
 *	the ese20 partition table.  Added partition tables for the rf72 and
 *	ra92.  Added to the model table the rf73 and tf70l.  Added to the 
 *	mscp media table rfh31, rfh72, rf73, rfh73.
 *	
 *   16-Nov-1989	Tim Burke
 *	Added default partition table entry for the rf72.
 *
 *   03-Nov-1989	Tim Burke
 * 	Added an 'a' partition to the rrd40 to allow rsblk to get the partition
 *	table off the 'a' partition.
 *	Fixed the 'b' partition of the ra80.
 *
 *   10-Oct-1989	Tim Burke
 *	Changed the tmscp_unit_tbl to allocate NTUNIT unit block pointers.
 *
 *   23-Aug-1989	Tim Burke
 *	Modify the partition table for the RD53.  Enlarge the "b" partition
 *	from 33440 to 50160.  This causes the "g" partition to shrink so that
 *	it goes from 82928 to the end, not 66208 to the end.  Fixed the "b"
 *	partition table entry for the rf31 to start at 32678.
 *
 *   17-Aug-1989	David E. Eiche		DEE0074
 *	Change name of the HSX50 (previously called the BSA, HSB50, and
 *	KSB50) to be KDM70 to conform to the current MSCP specification.
 *	In addition, change the controller number of the KDM70 from
 *	128 to 27.  Add model table entries for the TM32; add media
 *	table entries for the TM32 and ESE25.
 *
 *   14-Jun-1989	Mark A. Parenti
 *	Modify partition tables for system disks. Changes are as follows:
 *		1. "A" partition is increased to 32768 blocks.
 *		2. "B" partition is a minimum of 50160 blocks.
 *
 *   07-Mar-1989	Todd M. Katz		TMK0002
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   03-Mar-1989	Tim Burke
 *	Add model table entries for TQL70 and TBL70.
 *
 *   22-Feb-1989	Tim Burke
 *	Change bus type specified in the model table for the DEV_TQK70.
 *	This is done because some older revs of the TBK70 returned a model
 *	number of 14 which corresponds to the TQK70 and consequently a
 *	uq and not bvpssp controller entry.  The software workaround to this
 *	problem is to specify an unknown bus type which forces the port driver
 *	to figure out the appropriate bus type.
 *
 *   28-Dec-1988	David E. Eiche		DEE0060
 *	Add model table entries for the HSC40, TF85 and RF72 controllers,
 *	and correct the entry for the TUK70 to be TUK50.  Add media table
 *	entries for the RF72, TF85 and TF70 units.  Change audit trail
 *	DEE0055 to fix references to fictional RA71 (really RF71) and
 *	RA72 (really RRD40!) devices.
 *
 *   06-Oct-1988	Stephen Reilly
 *	Correct comment that resulted in the elimination of the e partition
 *	of the rf71.
 *
 *   22-Sep-1988	Stephen Reilly
 *	Changed ra60 and ra81 partition tables back to their original
 *	default values.
 *	
 *   07-Sep-1988	Stephen Reilly
 *	Correct some partition table mistakes
 *
 *   06-Sep-1988	David E. Eiche		DEE0055
 *	Change partition tables for RA60, RA81, RA70 and RA71.
 *	Add a media table entry for the RA72.  Change controller
 *	name from RRD50 to KRQ50.
 *
 *   06-Sep-1988	David E. Eiche		DEE0053
 *	Change definitions for the host timeout values in the controller
 *	model table.
 *
 *   17-Aug-1988	David E. Eiche		DEE0052
 *	Change the partition table entry for the RRD40 to use
 *	the same partition layout as is used by the SCSI version
 *	of the device.
 *
 *   27-Jul-1988	David E. Eiche		DEE0049
 *	Add the TK50 to the group of devices with unknown bus type,
 *	eliminate the KFQSA entries, because MSCP doesn't ever get
 *	those values, and fix the bus type fields for the DEBNT and
 *	TBK50.
 *
 *   27-Jul-1988	David E. Eiche		DEE0048
 *	Change the model table to specify unknown bus type and config
 *	name for entries in which the information cannot be derived
 *	directly from the model code.
 *
 *   17-Jul-1988	David E. Eiche		DEE0045
 *	Change the model table format so that it can be searched
 *	by a simple linear search, add model number and config name
 *	fields to each entry.  Alphabetize the partition table entries.
 *	Add new devices to model, media and partition tables.
 *
 *   09-Jun-1988	Robin
 *	Fixed the media ID for rf30's
 *
 *   06-Jun-1988	Ricky S. Palmer
 *	Added support for KFQSA
 *
 *   02-Jun-1988     Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   17-Apr-1988	Ricky S. Palmer
 *	Added support for DSSI (MSI bus) controllers.
 *
 *   11-Mar-1988	David E. Eiche		DEE00YY
 *	Moved additional configuration dependent data from mscp_var.c
 *	into this module.  Audit trail information prior to the split
 *	has been retained in mscp_var.c
 *
 *   22-Feb-1988	Robin
 *	Added ese20 and rd33 partition data information.
 *
 *   09-Jan-1988	Todd M. Katz		TMK0001
 *	Included new header file ../vaxmsi/msisysap.h.
 */
/**/

/* Libraries and Include Files.
 */
#include	"../h/types.h"
#include	"../h/time.h"
#include	"../h/param.h"
#include	"../h/kmalloc.h"
#include	"../h/buf.h"
#include	"../h/errno.h"
#include	"../h/ioctl.h"
#include	"../h/devio.h"
#include	"../h/file.h"
#include	"../fs/ufs/fs.h"
#include	"../h/errlog.h"
#include	"../machine/pte.h"
#include	"../h/vmmac.h"
#include	"../io/scs/sca.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/uba/ubavar.h"
#include	"../io/sysap/mscp_msg.h"
#include	"../io/sysap/mscp_defs.h"

/* External Variables and Routines.
 */

/* Minor device number to unit block correlation tables
 */
UNITB	*mscp_unit_tbl[NUNIT];
UNITB	*tmscp_unit_tbl[NTUNIT];

/* Response ID table (RSPID table) and listheads
 */
LISTHD mscp_rspid_lh = { &mscp_rspid_lh, &mscp_rspid_lh };
LISTHD mscp_rspid_wait_lh = { &mscp_rspid_wait_lh, &mscp_rspid_wait_lh };

/* The size of the RSPID table is the max number of HSC's and UQ 
 * controllers multiplied by the number of credits allowed per each.
 * But for now set rspid table size by default. 
 */
int	rspid_wq_cnt = 0;
#if defined( GENERIC ) || defined( SAS )
#define NUM_RSPID 96
#else
#define NUM_RSPID 192
#endif
int	nrspid = NUM_RSPID;
RSPID_TBL mscp_rspid_tbl[NUM_RSPID];

/* Partition entries for MSCP-talking disks
 */
PART_SIZE  ese20_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	50160,	32768,		/* B=blk 32768 thru 82927 */
	-1,	0,		/* C=blk 0 thru end 245757 */
	81416,	82928,		/* D=blk 82928 thru 164343 */
	-1,	164344,		/* E=blk 164344 thru end   */
	0,	0,
	-1,	82928,		/* G=blk 82928 thru end */
	0,	0
};
PART_SIZE ra60_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	50160,	32768,		/* B=blk 32768 thru 82927 */
	-1,	0,		/* C=blk 0 thru end (400175) */
	52416,	242928,	 	/* D=blk 242928 thru 295343 */
	52416,	295344, 	/* E=blk 295344 thru 347759 */
	-1,	347760, 	/* F=blk 347760 thru end */
	160000,	82928,		/* G=blk 82928 thru 242927 */
	-1,	242928		/* H=blk 242928 thru end */
};
PART_SIZE ra70_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	131072,	32768,		/* B=blk 32768 thru 163839 */
	-1,	0,		/* C=blk 0 thru end (547041)  */
	163840,	0,	 	/* D=blk 0 thru 163839 */
	471040,	0,	 	/* E=blk 0 thru 471039 */
	-1,	471040,		/* F=blk 471040 thru end */
	-1,	163840,		/* G=blk 163840 thru end */
	0,	0		
};
PART_SIZE ra71_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	131072,	32768,		/* B=blk 32768 thru 163839 */
	-1,	0,		/* C=blk 0 thru end (1367310)  */
	204800,	778240,	 	/* D=blk 778240 thru 983039 */
	204800, 983040,	 	/* E=blk 983040 thru 1187839 */
	-1,	1187840,	/* F=blk 1187840 thru end */
	614400,	163840,		/* G=blk 163840 thru 778239 */
	-1,	778240		/* H=blk 778240 thru end */
};
PART_SIZE ra72_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	190464,	32768,		/* B=blk 32768 thru 223231 */
	-1,	0,		/* C=blk 0 thru end (1953300)  */
	299008,	1144832, 	/* D=blk 1144832 thru 1443839 */
	299008,	1443840, 	/* E=blk 1443840 thru 1742847 */
	-1,	1742848,	/* F=blk 1742848 thru end */
	921600,	223232,		/* G=blk 223232 thru 1144831 */
	-1,	1144832		/* H=blk 1144832 thru end */
};
PART_SIZE ra80_sizes[8] = {
	32768,	0,		/* A=blk 0 thru 32767 */
	50160,	32768,		/* B=blk 32768 thru 82927 */
	-1,	0,		/* C=blk 0 thru end (237212)*/
	51428,	82928,	 	/* D=blk 82928 thru 134355 */
	51428,	134356, 	/* E=blk 134356 thru 185783 */
	-1,	185784, 	/* F=blk 185784 thru end */
	-1,	82928,		/* G=blk 82928 thru end (237212) */
	0,	0,
};
PART_SIZE ra81_sizes[8] ={
	32768,	0,		/* A=blk 0 thru 32767 */
	66690,	32768,		/* B=blk 32768 thru 99457 */
	-1,	0,		/* C=blk 0 thru end (891072)*/
	210538,	259458, 	/* D=blk 259458 thru 469995 */
	210538,	469996, 	/* E=blk 469996 thru 680533 */
	-1,	680534, 	/* F=blk 680534 thru end */
	160000,	99458,		/* G=blk 99458 thru 259457 */
	-1,	259458,		/* H=blk 259458 thru end */
};
PART_SIZE ra82_sizes[8] ={
	32768,	0,		/* A=blk 0 thru 32767 */
	66690,	32768,		/* B=blk 32768 thru 99457 */
	-1,	0,		/* C=blk 0 thru end (1216665) */
	220096,	99458,	 	/* D=blk 99458 thru 319553 */
	219735,	319554,	 	/* E=blk 319554 thru 539288 */
	437760,	539289, 	/* F=blk 539289 thru 977048 */
	877591, 99458,		/* G=blk 99458 thru 977048 */
	-1,	977049,		/* H=blk 977049 thru end */
};
PART_SIZE ra90_sizes[8] ={
	32768,	0,		/* A=blk 0 thru 32767 */
	127072,	32768,		/* B=blk 32768 thru 159839 */
	-1,	0,		/* C=blk 0 thru end (2376153) RA90 */
				/* C=blk 0 thru end (2940951) RA92 */
	420197,	159840,	 	/* D=blk 159840 thru 580036 */
	420197,	580037,	 	/* E=blk 580037 thru 1000233 */
	840393,	1000234, 	/* F=blk 1000234 thru 1840626 */
	1680787,159840,		/* G=blk 159840 thru 1840626 */
	-1,	1840627,	/* H=blk 1840627 thru end */
};
PART_SIZE rc25_sizes[8] = {
	15884,	0,		/* A=blk 0 thru 15883 */
	10032,	15884,		/* B=blk 15884 thru 25908 */
	-1,	0,		/* C=blk 0 thru end (50902) */
	0,	0,
	0,	0,
	0,	0,
	-1,	25916,		/* G=blk 25916 thru end (50902) */
	0,	0
};
PART_SIZE rd31_sizes[8] = {
	15884,	0,		/* A=blk 0 thru 15883 */
	10024,	15884,		/* B=blk 15884 thru 25908 */
	-1,	0,		/* C=blk 0 thru 41560 */
	0,	0,		
	0,	0,	
	0,	0,
	-1,	25908,		/* G=blk 25908 thru end */
	0,	0,
};
PART_SIZE rd32_sizes[8] = {
	15884,	0,		/* A=blk 0 thru 15883 */
	15625,	15884,		/* B=blk 15884 thru 31508 */
	-1,	0,		/* C=blk 0 thru 83236 */
	25863,	31509,		/* D=blk 31509 thru 57371 */
	-1,	57372,		/* E=blk 57372 thru end */
	0,	0,
	-1,	31509,		/* G=blk 31509 thru end */
	0,	0,
};
PART_SIZE rd33_sizes[8] = {
	15884,	0,		/* A=blk 0 thru 15883 */
	33440,	15884,		/* B=blk 15884 thru 49323 */
	-1,	0,		/* C=blk 0 thru end (138565) */
	0,	0,
	50714,	0,		/* E=blk 0 thru 50713 */
	-1,	50714,		/* F=blk 50714 thru end */
	-1,	49324,		/* G=blk 49324 thru end */
	-1,	15884,		/* H=blk 15884 thru end */
};
PART_SIZE rd51_sizes[8] = {
	15884,	0,		/* A=blk 0 thru 15883 */
	5716,	15884,		/* B=blk 15884 thru 21599 */
	-1,	0,		/* C=blk 0 thru 21599 */
	0,	0,		
	0,	0,	
	0,	0,
	0,	0,		
	0,	0,	
};
PART_SIZE rd52_sizes[8] ={
	15884,	0,		/* A=blk 0 thru 15883 */
	9766,	15884,		/* B=blk 15884 thru 25649 */
	-1,	0,		/* C=blk 0 thru end 60480 */
	0,	0,
	50714,	0,		/* E=blk 0 thru 50714 */
	-1,	50714,		/* F=blk 50714 thru end */
	-1,	25650,		/* G=blk 25650 thru end */
	-1,	15884,		/* H=blk 15884 thru end */
};
PART_SIZE rd53_sizes[8] ={
	32768,	0,		/* A=blk 0 thru 32767 */
	50160,	32768,		/* B=blk 32768 thru 82927 */
	-1,	0,		/* C=blk 0 thru end 138672 */
	0,	0,		
	0,	0,
	0,	0,
	-1,	82928,		/* G=blk 82928 thru end */
	-1,	32768,		/* H=blk 32768 thru end */
};
PART_SIZE rd54_sizes[8] ={
	32768,	0,		/* A=blk 0 thru 32767 */
	50160,	32768,		/* B=blk 32768 thru 82927 */
	-1,	0,		/* C=blk 0 thru end 311200 */
	130938,	82928,		/* D=blk 82928 thru 213865 */
	-1,	213866,		/* E=blk 213866 thru end   */
	0,	0,
	-1,	82928,		/* G=blk 82928 thru end */
	0,	0
};
PART_SIZE rf30_sizes[8] ={
	32768,	0,		/* A=blk 0 thru 32767 */
	50160,	32768,		/* B=blk 32768 thru 82927 */
	-1,	0,		/* C=blk 0 thru end (293040) */
	130938,	82928,		/* D=blk 82928 thru 213865 */
	-1,	213866,		/* E=blk 213866 thru end   */
	0,	0,
	-1,	82928,		/* G=blk 82928 thru end */
	0,	0
};
PART_SIZE rf31_sizes[8] ={
        32768,  0,              /* A=blk 0 thru 32767 */
        131072, 32768,          /* B=blk 32768 thru 163839 */
        -1,     0,              /* C=blk 0 thru end (744400) */
        163840, 0,              /* D=blk 0 thru 163839 */
        471040, 0,              /* E=blk 0 thru 471039 */
        -1, 	471040,         /* F=blk 471040 thru end (744400) */
        -1, 	163840,         /* G=blk 163840 thru end (744400) */
        0,      0         
};
PART_SIZE rf71_sizes[8] ={
        32768,  0,              /* A=blk 0 thru 32767 */
        131072, 32768,          /* B=blk 32768 thru 163839 */
        -1,     0,              /* C=blk 0 thru end (781440) */
        163840, 0,              /* D=blk 0 thru 163839 */
        471040, 0,              /* E=blk 0 thru 471039 */
        -1, 	471040,         /* F=blk 471040 thru end (781440) */
        -1, 	163840,         /* G=blk 163840 thru end (781440) */
        0,      0         
};
PART_SIZE rf72_sizes[8] ={
	32768,	0,		/* A=blk 0 thru 32767 		*/
	190608,	32768,		/* B=blk 32768 thru 223375 	*/
	-1,	0,		/* C=blk 0 thru end (2047100) 	*/
	300708,	1144976,  	/* D=blk 1144976 thru 1445683 	*/
	300708,	1445684,	/* E=blk 1445684 thru 1746391 	*/
	-1,	1746392, 	/* F=blk 1746392 thru end  	*/
	921600, 223376,		/* G=blk 223376 thru 1144975 	*/
	-1,	1144976,	/* H=blk 1144976 thru end 	*/
};
PART_SIZE rrd40_sizes[8] ={
	-1,	0,
	0,	0,
	-1,	0,		/* C=blk 0 thru end */
	0,	0,
	0,	0, 
	0,	0, 	
	0,	0,		
	0,	0		
};
PART_SIZE rrd50_sizes[8] ={		/* Need better values */
	15884,	0,		/* A=blk 0 thru 15883 */
	33440,	15884,		/* B=blk 15884 thru 49323 */
	-1,	0,		/* C=blk 0 thru end (1171875) */
	122993,	131404, 	/* D=blk 131404 thru 254396 */
	122993,	254397, 	/* E=blk 254397 thru 377389 */
	-1,	377390, 	/* F=blk 377390 thru end */
	82080,	49324,		/* G=blk 49324 thru 131403 */
	-1,	131404		/* H=blk 131404 thru end */
};
PART_SIZE rx33_sizes[8] = {
	-1,	0,		/* 0 thru 800 or 2400 handles both rx50 & rx33*/
	0,	0,
	-1,	0,		/* C=blk 0 thru end (2400) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0
};
PART_SIZE rx35_sizes[8] = {
	-1,	0,		/* 0 end	*/
	0,	0,
	-1,	0,		/* C=blk 0 thru end (4096) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0
};
PART_SIZE rx50_sizes[8] = {
	-1,	0,		/* 0 thru 800 */
	0,	0,
	-1,	0,		/* C=blk 0 thru end (800) */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0
};

/**/

/* Controller and unit identifer "class" values - MSCP Table C-1
 *
 * This table is used to convert the controller or unit
 * class field found in a "unique identifier" into an
 * ASCII name string.
 */
struct _cu_class {
    char	*name;			/* Ctlr/unit class name string	     */
    char	id;			/* Controller/unit class ID	     */
} cu_class[] = {
    { "Reserved",	0 },		/* Reserved - do not use	     */
    { "Mass storage",	1 },		/* Mass storage controller	     */
    { "Disk",		2 },		/* DEC STD 166 disk class device     */
    { "Tape",		3 },		/* Tape class device		     */
    { "Disk",		4 },		/* DEC STD 144 disk class device     */
    { "Loader",		5 }		/* Media Loader service		     */
};

/* Mass storage controller values - MSCP table C-2
 *
 * This table is used to convert the controller model number
 * found in a "unique identifier" into an ASCII controller name
 * string.
 */
MODEL model_tbl[] = {
    { DEV_UNKNOWN,	0 },
    { DEV_HSC50,	1 },
    { DEV_UDA50,	2 },
    { DEV_RC25,		3 },
    { "VMS",		4 },
    { DEV_TU81,		5 },
    { DEV_UDA50A,	6 },
    { DEV_RQDX2,	7 },
    { "TOPS10/20",	8 },
    { DEV_TK50,		9 },
    { DEV_RUX50,	10 },
    { DEV_UNKNOWN,	11 },
    { DEV_KFBTA,	12 },
    { DEV_KDA50,	13 },
    { DEV_TQK70,	14 },
    { DEV_RV20,		15 },
    { DEV_KRQ50,	16 },
    { DEV_UNKNOWN,	17 },
    { DEV_KDB50,	18 },
    { DEV_RQDX3,	19 },
    { DEV_RQDX4,	20 },
    { DEV_TUK50,	25 },
    { DEV_KRU50,	26 },
    { DEV_KDM70,        27 },
    { DEV_TQL70,	28 },
    { DEV_TM32,		29 },
    { DEV_HSC70,	32 },
    { DEV_HSC40,	33 },
    { DEV_HSC60,	34 },
    { DEV_HSC90,	35 },
    { DEV_DEBNT,	65 },
    { DEV_TBK70,	66 },
    { DEV_TBL70,	68 },
    { DEV_RF30,		96 },
    { DEV_RF71,		97 },
    { DEV_TF85,		98 },
    { DEV_TF70,		99 },
    { DEV_RF31,		100 },
    { DEV_RF72,		101 },
    { DEV_RF73,		102 },
    { DEV_TF70L,	103 },
    { "ULTRIX",		248 },
    { "VAX/SVS",	249 },
};


int	model_ct =  sizeof( model_tbl ) / sizeof( model_tbl[0] );

/**/

/* Table to correlate a DEC STD 166 disk identifier value
 * (MSCP specification table C-3) to a device index or a
 * device identifier character string
 */

DMSCP_MEDIA dmscp_media[] = {
    {"AAA0",	0, 0x08421080,ra60_sizes},	/* Permanently reserved      */
    {DEV_RA80,  1, 0x25641050,ra80_sizes},
    {DEV_RC25,  2, 0x20643019,rc25_sizes},
    {DEV_RC25F, 3, 0x20643319,rc25_sizes},
    {DEV_RA60,  4, 0x22a4103c,ra60_sizes},
    {DEV_RA81,  5, 0x25641051,ra81_sizes},
    {DEV_RD51,  6, 0x25644033,rd51_sizes},
    {DEV_RX50,  7, 0x25658032,rx50_sizes},
    {DEV_RD52,  8, 0x25644034,rd52_sizes},
    {DEV_RD53,  9, 0x25644035,rd53_sizes},
    {DEV_RX33,  10, 0x25658021,rx33_sizes},
    {DEV_RA82,  11, 0x25641052,ra82_sizes},
    {DEV_RD31,  12, 0x2564401f,rd31_sizes},
    {DEV_RD54,  13, 0x25644036,rd54_sizes},
    {DEV_RRD50, 14, 0x25652232,rrd50_sizes},
    {DEV_RD32,  15, 0x25644020,rd32_sizes},
    {"AAA1",  	16, 0x08421081,ra60_sizes},	/* Reserved		     */
    {DEV_RX18,  17, 0x25658012,ra60_sizes},
    {DEV_RA70,  18, 0x25641046,ra70_sizes},
    {DEV_RA90,  19, 0x2564105a,ra90_sizes},
    {DEV_RX35,  20, 0x25658023,rx35_sizes},
    {DEV_RF30,  21, 0x2264601e,rf30_sizes},
    {DEV_RF71,  22, 0x22646047,rf71_sizes},
    {DEV_SVS00, 23, 0x21676980,ra60_sizes},
    {DEV_RD33,  24, 0x25644021,rd33_sizes},
    {DEV_ESE20, 25, 0x254b3294,ese20_sizes},
    {DEV_RRD40, 26, 0x25652228,rrd40_sizes},
    {DEV_RF31,  27, 0x2264601f,rf31_sizes},
    {DEV_RF72,  28, 0x22646048,rf72_sizes},
    {DEV_RA92,  29, 0x2564105c,ra90_sizes},
    {DEV_ESE25, 30, 0x254b3299,ese20_sizes},
    {DEV_RFH31, 33, 0x2264641f,ra60_sizes},
    {DEV_RFH72, 34, 0x22646448,ra60_sizes},
    {DEV_RF73, 	35, 0x22646049,rf72_sizes},
    {DEV_RFH73, 36, 0x22646449,ra60_sizes},
    {DEV_RA72,  37, 0x25641048,ra72_sizes},
    {DEV_RA71,  40, 0x25641047,ra71_sizes},
};
int dmscp_media_ct = sizeof( dmscp_media ) / sizeof( dmscp_media[0] );


/* Table to correlate a magnetic tape identifier value
 * (MSCP specification table C-4) to a device index or a
 * device identifier character string
 */


TMSCP_MEDIA tmscp_media[] = {
    {"BAA0",	0, 0x10421080},
    {DEV_TU78,  1, 0x69a9504e},
    {DEV_TU81,  2, 0x6d695051},
    {DEV_TK50,  3, 0x6d68b032},
    {DEV_TA81,  4, 0x6d681051},
    {DEV_TA79,  5, 0x6d68104f},
    {"BAA1",    6, 0x10421081},
    {DEV_TA90,  7, 0x6d68105a},
    {DEV_RV60,  8, 0x6d65603c},
    {DEV_SVS00, 9, 0x69676980},
    {DEV_TF85,  10, 0x6a686055},
    {DEV_TF70,  11, 0x6a686046},
    {DEV_TA91,  12, 0x6d68105b},
    {"BAA1",    13, 0x10421081},
    {DEV_TK70,  14, 0x6d68b046},
    {DEV_RV20,  15, 0x6d656014},
    {DEV_TM32,  16, 0x6d68d020},
};
int tmscp_media_ct = sizeof( tmscp_media ) / sizeof( tmscp_media[0] );
