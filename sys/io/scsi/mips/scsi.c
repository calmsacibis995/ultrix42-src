#ifndef lint
static  char    *sccsid = "@(#)scsi.c	4.11  (ULTRIX)        2/14/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

/************************************************************************
 *
 * scsi.c	12/19/89
 *
 * CVAX/FIREFOXstar/PVAX/PMAX SCSI device driver (common code)
 *
 * Modification history:
 *
 *   29-Jan-91	Robin Miller
 *	o  Added support for CD-ROM audio commands.
 *	o  Moved pseudo SCSI commands to group 3 range (0x60-0x7f)
 *	   to avoid conflict with CD-ROM audio commands.
 *
 *   09-Jan-91	Robin Miller
 *	On fixed length block tapes (QIC), if the byte count isn't modulo
 *	the block size, return the error code EINVAL ("invalid argument")
 *	instead of ENXIO ("no such device or address") to avoid confusion.
 *	This error occurs frequently when using the 'dd' utility if the
 *	'sync=conv' option is omitted to pad output records.
 *
 *   27-Nov-90	Robin Miller
 *	Merged in Bill Dallas fix which resets the szflags field before
 *	retrying the command.  Previously if the device had disconnected
 *	with a check condition, the DMA disconnect flag (SZ_DMA_DISCON)
 *	was left set causing the wrong DMA count to be setup which later
 *	panic'ed the system.
 *
 *   26-Nov-90	Robin Miller
 *	o  Added function sz_cdb_length() to calculate and return the
 *	   Command Descriptor Block length.  The length is calculated
 *	   using the group code of the command.
 *	o  Removed '#ifdef FORMAT' conditionalization, since the special
 *	   interface is used to implement CD-ROM Audio commands.
 *	o  Added support for 10-byte read/write CDB's.  If the LBA is
 *	   too large for 6-byte CDB, setup and send a 10-byte CDB.
 *
 *   07-Nov-90	Robin Miller
 *	Changed logic associated with checking of End Of Media (EOM) and
 *	File Mark (FM) bits in the sense data for the TLZ04 (RDAT).  The
 *	RDAT sets both of these bits on reads past LEOT (early warning).
 *	Previously, we only checked and returned EOM status and ignored
 *	the FM bit.  This broke the restore utility since it expected an
 *	end of file indication (FM returns count of 0) to prompt for the
 *	next volume.
 *
 *   26-Oct-90	Robin Miller
 *	Modified scsi_logerr() function to pickup the proper controller
 *	base address out of the softc structure.  Previously, it was
 *	always using the base address of the first SCSI controller.
 *
 * 21-Sept-90	Bill Dallas
 *	Added fixes for correct handling of flags for tapes. This
 *	includes tpmark, dev_cse, and dev_shrtrec. All fixes are
 * 	in the state machine (state SZ_BEGIN). Please see the 
 *	comments in the code for a complete description.
 * 
 *   10-Sep-90  Charles Richmond IIS Corp
 *	Added declarations required for kzqsa board and driver.
 *
 *   24-Aug-90  Maria Vella
 *      - Set the sc_category_flags field with DEV_SHRTREC when ILI bit
 *        set in Request Sense data.  Used to be set in sc_flags which
 *        was interpreted as DEV_EOM in tape driver.
 *
 *   15-Aug-89	Robin Miller
 *	o  Added errlog cases for RRD42, RX26, and RZ25 devices.
 *
 *	o  Modified the szerror() routine so "Device Not Ready" errors
 *	   get sent to the error logger.  This error logging is only
 *	   done if the current command isn't a Test Unit Ready (SZ_TUR)
 *	   so checks in device open routines don't get logged as errors.
 *	   Previously ejecting a tape or a CD during an command didn't
 *	   get logged, only EIO was returned to the program.
 *
 *	o  Fixed problem logging errors for CD-ROM devices.  The
 *	   scsi_logerr() routine was not checking for CD-ROM devices
 *	   when setting up the class type field, so it was set to
 *	   undefined (EL_UNDEF).  The 'uerf' utility would then report
 *	   "OS EVENT TYPE 65535" and not display the error information.
 *	   The CD-ROM errors are now treated the same as disk errors.
 *
 *   10-Aug-90  Maria Vella
 *      - Cleared the SZ_NEED_SENSE bit in sc_szflags for: 
 *          - SP_START in the default case on return from sc->port_start()
 *          - SP_START_ERR after call to sz_restore_rqsns().
 *      - Added call to sz_restore_rqsns() in SZ_W_DMA in the
 *        default case on return from sc->port_start().
 *      
 *   30-Jul-90 	Bill Dallas   
 *	Added fixed block tape units tape mark handling.
 *	This included a new falg in sc_category_flags called
 *	TPMARK_PENDING
 *
 *   16-Jul-90  Janet Schank
 *	Added errlog cases for the RZ23L.
 *
 *   25-Jun-90	 Mitchell McConnell
 *	o  Fix for PMAX - port_start for RQSNS will return BUSBUSY if
 *	   unable to select due to reselection.  In this case, instead
 *	   of the event remaining the same, we need to set the event to
 *	   SZ_SELRETRY_SNS to work with state machine changes for 3max
 *	   (see explanation for 13-Jun-90).
 *
 *   13-Jun-90	 Mitchell McConnell
 *	o  Fix for RDAT "dropped packets" problem.  The port_start for 
 *	   RQSNS was returning SZ_IP, but we then lost arbitration (which
 *	   we didn't find out till interrupt time).  The problem is how to
 *	   recover state in a sufficiently general way without having
 *	   to create more sub-states, etc.  The proposed solution is to
 *	   use the pxstate field in the softc (this is how it is used by
 *	   the vax) which will be the state to return to if the select 
 *	   fails.  In the "normal" case, the oops state will be SZ_NEXT/BEGIN,
 *	   just as it has always been.  For the special RQSNS states,
 *	   the new state must revert "back" one state, i.e., SZ_RDMA_ERR
 *	   must go back to SZ_R_DMA.  The catch is that any flags that
 *	   may have been altered must be set back to their original values.
 *	   E.g., the routines save_rqsns and restore_rqsns must be called
 *	   so that on the next iteration through SZ_R_DMA, the NEED_SENSE
 *	   flag will be set, and the RQSNS may be retried.
 *
 *   07-Jun-90 	 Mitchell McConnell
 *	o  Reverted change of 20-Jan-90 back to original scheme to try
 *	   and isolate RDAT problem.
 *
 *   06-June-90	Bill Dallas
 *	Added support for the option table... new field in the devtab struct.
 *	This will allow easy additions of devices of a known type..tape/disk
 *
 *   17-May-90  John A. Gallant
 *	Added setting BERROR in the error leg of the SM.
 *
 *   16-May-90   Mitchell McConnell
 *	o  Removed ifdef'ed FORMAT stuff.
 *	o  Added new error states to handle SZ_IP return code from 
 *	   RQSNS.  All of the main states now have a box before their
 *	   case statement, to help in following the code.
 *	o  Because of asynchronous nature of RQSNS, must save relevant
 *	   softc fields on a per-target basis.  Created new structure
 *	   and routines to save/restore...
 *
 *   19-Mar-90  Janet Schank
 *      Added errlog cases for RZ24, RZ57, and TZ05.
 *
 *   26-FEB-90  Janet Schank
 *      Added errlog case for TLZ04 (RDAT).
 *
 * 23-Feb-90 -- sekhar
 *      Merged Joe Martin's fix for 3.1 cld. When copying user PTEs,
 *      check for page crossing and reevaluate vtopte.
 *
 *   20-Jan-90  Mitchell McConnell
 *	DEV_TPMARK and DEV_SHRTREC were previously set in category_flags
 *	to indicate filemark and over-read, respectively.  DEV_TPMARK was
 *	doing double duty, however, since it remained set until cleared by
 *	another tzopen or clear serious exception.  What is needed are 2
 *	flags which apply only to the current i/o to know how to handle
 *	residual bytes (in ST_R_DMA).  Since sc_flags is elsewhere used
 *	in this way, we now set TPMARK (when it occurs) in both sc_flags 
 *	and sc_category flags, with sc_flags being cleared as soon as 
 *	sc_resid is set.  Similarly, we now use sc_flags for SHRTREC 
 *	instead of category flags.
 *
 *   19-Dec-89	John A. Gallant
 *	Restored the error retry code in the szerror() routine, and in the SM
 *	now compare the retries aginst the SP/RW_RTCNT defines in scsireg.h
 *
 *   11-Dec-89  Mitchell McConnell
 *	Set sc_attached flag in szattach() to catch problem where devices
 *	which were physically present and alive but not configured were
 *	causing the system to hang.
 *
 *   11-Nov-89  Mitchell McConnell
 *      Added ASC support to scsi_logerr.  Added ASC logerr string 
 *      to sz_BusErr array entry #5. Made entry for 4C non-SII 
 *      specific, since I need to share it and the context
 *	should be clear from the controller id.
 *
 *   17-Oct-89  Janet L. Schank / JAG
 *      The RDAT drive was setting both the filemark and ili bits 
 *      in the request sense data when a filemark is read.  Changed
 *      the checks for these bits in szerror to only set one error
 *      flag depending on these bits.  Added the function get_scsiid.
 *
 *   17-Oct-89  Janet L. Schank / Art Zemon
 *      Added TZ05 support
 *
 *   04-Oct-89	Fred Canter
 *	Bug fix. Added newline to resetting SCSI bus message.
 *	Added sector number to error log packet for disk errors.
 *	Use sz_sbtol() to extract infobytes from sense data.
 *
 * 10/12/89     Janet L. Schank
 *      Moved the clearing of the rambuff from scsi.c to sii_scsistart
 *
 * 09/22/89     Janet L. Schank
 *      Changed some defines and ifdefs to include and use sii.h.
 *      Removed many "ifdef vax"'s.  Added default scsi_devtab
 *      entries:  szp_rz_udt, szp_tz_udt, szp_cz_udt.  Added sz_devname
 *      and sz_revlevel arrays.  Replaced the szattach and szslave
 *      functions with "auto-config" versions from the vax-side code.
 *
 *   14-Aug-89	Fred Canter
 *	Enabled binary error logging (sz_log_errors = 1).
 *	Bug fix: not setting registers valid flag for SII.
 *
 *   25-Jul-89	Fred Canter
 *	Separate SCSI device name defines from MSCP/TMSCP names.
 *
 *   23-Jul-89	Fred Canter
 *	Convert DBBR printfs to error log calls.
 *	RX33 support.
 *
 *   22-Jul-89	Fred Canter
 *	Always log "resetting bus" to console.
 *
 *   16-Jul-89	Fred Canter
 *	Do mode sense to get disk geometry.
 *
 *   15-Jul-89	Fred Canter
 *	Merged Dynamic BBR and error log changes.
 *	Add flag to control logging of DATAPROTECT errors (sz_log_we_errors).
 *
 *
 *   13-Jul-89	Fred Canter
 *	Special mode select/sense handling for the EXABYTE tape.
 *
 *   12-Jul-89	Fred Canter
 *	Added scsi_logerr() routine for SCSI binary error logging.
 *	Save current CDB and status byte for error log.
 *
 *	Buf gix (same as stc.c). Back/forward space thru a file mark
 *	should fail but wasn't.
 *
 *   27-Jun-89	John A. Gallant
 *	Moved the completion handling of a bp, from the statemachine to the
 *	completion handler for the drivers.  Included support for the read/
 *	write long commands.
 *
 *   14-Jun-89	Fred Canter
 *	Initialize sc_c_status in sz_start(), caused rzspecial commands
 *	to fail because of left over bad status.
 *
 *   11-Jun-89	Fred Canter
 *	Save additional sense code in sz_softc so we can tell when the
 *	floppy media has been changed. Hooks for softpc.
 *
 *   24-May-89	Fred Canter
 *	Changes to match the new, more general rzdisk utility.
 *	Changed the mode select data structures so an address and
 *	length are passed with the ioctl. This allows pages to be
 *	added to rzdisk without requiring a kernel rebuild.
 *
 *   22-May-89	Fred Canter
 *	Fixed a bug in the code which copies the data back to the user
 *	on a unrecoverable data error. The code was completely wrong and
 *	would cause a panic on a medium error (such as reading a unformatted
 *	SCSI floppy).
 *
 * 03/30/89	John A. Gallant
 *	Pulled more fixed from the VAX side for rxdisk:
 *	Fixed an "rzdisk" related bug. If an "rzdisk" command gets
 *	a check condition an the sense key is SZ_NOSENSE then return
 *	good status.
 *	Modified the state machine to copy data back to the user
 *	on UNRECOVERABLE data errors. The data copied back to the
 *	user consists of the data starting from the beginning of
 *	the disk transfer upto and including the bad block. This
 *	is needed mainly for "rzdisk" when REASSIGNING bad blocks.
 *
 * 03/22/89	John A. Gallant
 *	In sz_bldpkt() for mode select, if the tape is a TZxx don't send
 *	any of the vender unique bits.  Set the pll field to 12.
 *
 * 03/01/89	John A. Gallant
 *	Added the pseudo command codes for to allow the tape to unload.  I
 *	followed the same conventions as the firefox/pvax code.
 *
 * 01/16/89	John A. Gallant
 *	More modifications from the firefox/pvax code:
 *	Call szerror() for certain tzcommands so the reason they
 *	failed gets entered into the error log.
 *	If BBWAIT is set, the b_command/b_resid field is not copied to curcmd.
 *	I also found the "black hole".
 *
 * 12/14/88	John A. Gallant
 *	More additions from the firefox code:
 *	Set the "pf" bit when you issue a MODE SELECT to the CDROM.
 *	Needed so "rzdisk" will work.
 *	Added the printing of the sector number and partition to the
 *	error logger for RECOVERABLE and UNRECOVERABLE data errors.
 *	
 * 12/01/88	John A. Gallant
 *	Fixed the tape ILI, bug, an "over-read" is now flaged in category_flags
 *	for the state machine to check for.
 *
 * 11/23/88	John A. Gallant
 *	Revamped the debug printfs, to allow tracking of single targets.
 *
 * 11/09/88	John A. Gallant
 *	Started the merge with the V3.0 source level.  Due to time constraints
 *	only the changes for the new IOCTL support will be merged.  Others
 *	changes will hopefully occur as time permits.  Copied over the timeout
 *	table, however, it is not implemented.
 *   COMMENTS from V3.0:
 *   03-Nov-88	Alan Frechette
 *	Added in support for disk maintainence. Added the commands
 *	FORMAT UNIT, REASSIGN BLOCK, READ DEFECT DATA and VERIFY DATA.
 *	Made changes to MODE SENSE and MODE SELECT. Figured out a better
 *	way to determine length of a SCSI command in "sz_bldpkt()".
 *
 * 11/07/88	John A. Gallant
 *	All changes are removal of target 5 debug statements.  Added a debug
 *	statement for each interation of the statemachine.
 *
 *   10-Oct-88	 Ricky Palmer (rsp)
 *	One last pass through here for FT1.
 *
 *   16-Sep-88	 Ricky Palmer (rsp)
 *	Fixed rz23 support.
 *
 *   14-Sep-88	 Ricky Palmer (rsp)
 *	Added IS_KUSEG macro test to mapping code.
 *	Also put in panic for bad addr if mapping is to user space.
 *
 *    7-Sep-88	 Ricky Palmer (rsp)
 *	Added new code and support for PMAX. There are changes 
 *	in the probe and setup code as well as the start routine
 *	in order to support the scsi_sii.c routines. The changes
 *	are surrounded by ifdef's where necessary.
 *
 *   25-Aug-88   Ricky Palmer
 *      Ifdef'ed again for vax and mips. This time it is based on
 *      my August 1, 1988 ifdef's of the original scsi.c "glob" driver.
 *
 *   18-Aug-88 -- Fred Canter
 *	Pass -1 not 0 to sz_start() to start next I/O (fix target ID 0).
 *	Add sz_em_print to control extended message debug printout.
 *	Fix driver not always seeing MICROPOLIS B drive by asserting
 *	ATTN before droping ACK on last byte of extended message.
 *	Add SHMEM code to bcopy (pte mapping) to fix BAR 435.
 *	Changed select enable back to the original way.
 *	Try interrupt instead of spin on phase change (doesn't work yet).
 *	Fix driver bug which made it think targets were skipping
 *	the status command (sometimes), data in was eating status byte.
 *
 *	Merge PVAX and FIREFOX drivers.
 *
 *   08-Aug-88 -- Alan Frechette
 *	Merged in all of the SII portion of this driver. Modified
 *	the driver to execute the correct code paths whether using
 *	the SII chip or the NCR 5380 chip. Fixed a few bugs in the
 *	SII code, made changes to the SII code based on Fred's new
 *	driver and tried to improve performance.
 *
 *   08-Aug-88 -- Fred Canter
 *	Removed stray interrupt code (can recover from last delta).
 *	In szprobe, reset bus if BSY/SEL wedged after inquiry.
 *	Clean out old debug and hltcod code.
 *	Fix for parity errors caused by PARCK enabled during arbitration.
 *
 *   28-Jul-88 -- Fred Canter
 *	Removed old code which supported RRD40 as device 'cz' (now 'rz).
 *	Fixed background timer to work with 2nd SCSI controller.
 *	Program around TZ30 holding BSY true after command complete.
 *	Added extended message support so driver will work with
 *	yet another version of the RZ55.
 *	Removed ond sz_timer code.
 *	Clean up sz_flags for much improved reselect timeout handling.
 *	Removed old disconnect timeout code from background timer.
 *
 *   18-Jul-88 -- Fred Canter
 *	Removed all #ifdef CZ code (can recover it from last delta).
 *	Fix background timer for 2nd SCSI controller.
 *	Added work around for TZ30 holding bus after command complete.
 *	Fixed MICROPOLIS B RZ55 support (handle extended message).
 *
 *   16-Jul-88 -- Fred Canter
 *	Handle unit attn condition (media change).
 *	Convert sz_active from 0-7 to ID bit position.
 *	Improve reselect timeout handling (CDROM hacks).
 *	Enable parity checking at all times.
 *	Fixed a bug in rzioctl which prevented the driver from
 *	ever setting the default partitions once the disk had
 *	a partition table on it.
 *	Changed the RRD40 name from cz to rz (CDROM is a disk).
 *	Lots of code cleanup.
 *
 *   28-Jun-88 -- Fred Canter
 *	Several driver improvements and code cleanup:
 *		Restructure for better ??command() handling.
 *		Deal with unit attn (TZ cartridge change).
 *		Read system's (initiator) bus ID from NVR.
 *		Fix DEV_WRITTEN flag handling.
 *		Background timer to catch lost interrupts
 *		and reselect timeouts.
 *
 *   18-Jun-88 -- Fred Canter
 *	Added RZ55 support and fixed a bug in partition table code.
 *	Fixed a bug in request sense status which caused intalls to fail.
 *	Improved (but not fixed) lost interrupt and reselect timeout
 *	handling (massive code changes).
 *
 *   07-Jun-88 -- Fred Canter
 *	Bug fixes for finder opening every possible device in the world.
 *
 *   06-Jun-88 -- Fred Canter
 *	First submit to V2.4 pool (sccs create). Much cleanup done.
 *	Much more needed. Driver functioning well enough to support
 *	building a kit for testing.
 *
 *   23-Apr-88 -- Fred Canter
 *	A RED LETTER DAY for sure!
 *	Fixed "most" of the problems which were preventing the
 *	driver from running multiple devices concurrently.
 *
 *   22-Apr-88 -- Fred Canter
 *	Prototype driver now functioning reasonably.
 *	Much cleanup done, much more needed.
 *
 *    1-Mar-88 -- Fred Canter
 *	Created the prototype SCSI driver from the VAXstar
 *	TZK50 driver (stc.c).
 *
 ***********************************************************************/

#include "../data/scsi_data.c"

#ifdef vax
extern char cvqmsi[] [512*NBPG];
extern char cvqmsirb[];
#endif vax
extern char szbufmap[];

extern	int sz_retries[];	/* retry counter */
int sz_max_numof_fills = SZ_DEFAULT_FILLS; /* Maximum number of fills to write*/
					   /* in order to try to keep the tape*/
					   /* drive streaming.		      */

/*
 * This auto-configuration will be needed in both the mips and vax
 * cases.
 */
int     szslave(), szattach(), sz_start(), szerror();
int 	sii_probe(), sii_intr(), sii_scsistart(), sii_reset();
int 	kzq_probe(), kzq_intr(), kzq_scsistart(), kzq_reset();
int 	ascprobe(), ascintr(), asc_scsistart(), asc_reset();
u_short	szstd[] = { 0 };


struct	uba_driver siidriver = { sii_probe, szslave, szattach, sz_start,
				szstd, "rz", szdinfo, "sii", szminfo,
				0 };

struct	uba_driver kzqdriver = { kzq_probe, szslave, szattach, sz_start,
				szstd, "rz", szdinfo, "kzq", szminfo,
				0 };

#ifdef vax
int	sz_timer1();	/* TODO1: debug */
int	sz_timer2();	/* TODO1: debug */
int	szprobe(), szintr(), sz_scsistart(), sz_reset();

struct	uba_driver scsidriver = { szprobe, szslave, szattach, sz_start,
				szstd, "rz", szdinfo, "scsi", szminfo,
				0 };

extern struct nexus nexus[];
#endif vax

extern int sz_unit_rcvdiag[];	/* If zero, need unit's selftest status */

/*
 * Unit on line flag. Set to one if the
 * device is on-line. Set to zero on any unit
 * attention condition.
 */
extern int sz_unit_online[];
/*
 * The following table is the timeout value for each command assuming that
 * the timer is set to go off every 30 seconds.
 * NOTE: not used, but keep around for command timeout data.
 */
short sz_timetable[] = {	 2,	/* SZ_TUR		0x00	*/
				 6,	/* SZ_REWIND		0x01	*/
				 0,	/* unused		0x02	*/
				 2,	/* SZ_RQSNS		0x03	*/
				 60,	/* SZ_FORMAT		0x04	*/
				 2,	/* SZ_RBL		0x05	*/
				 0,	/* unused		0x06	*/
				 2,	/* SZ_REASSIGN		0x07	*/
				 3,	/* SZ_READ		0x08	*/
				 0,	/* unused		0x09	*/
				 3,	/* SZ_WRITE		0x0a	*/
				 2,	/* SZ_TRKSEL		0x0b	*/
				 0,	/* unused		0x0c	*/
				 0,	/* unused		0x0d	*/
				 0,	/* unused		0x0e	*/
				 0,	/* unused		0x0f	*/
				 2,	/* SZ_WFM		0x10	*/
				 90,	/* SZ_SPACE		0x11	*/
				 2,	/* SZ_INQ		0x12	*/
				 90,	/* SZ_VFY		0x13	*/
				 2,	/* SZ_RBD		0x14	*/
				 2,	/* SZ_MODSEL		0x15	*/
				 2,	/* SZ_RESUNIT		0x16	*/
				 2,	/* SZ_RELUNIT		0x17	*/
				 0,	/* unused		0x18	*/
				 90,	/* SZ_ERASE		0x19	*/
				 2,	/* SZ_MODSNS		0x1a	*/
				 6,	/* SZ_LOAD/SZ_UNLOAD	0x1b	*/
				 6,	/* SZ_RCVDIAG		0x1c	*/
				 2,	/* SZ_SNDDIAG		0x1d	*/
				 2,	/* SZ_MEDREMOVAL	0x1e	*/
				 0,	/* unused		0x1f	*/
				 0,	/* unused		0x20	*/
				 0,	/* unused		0x21	*/
				 0,	/* unused		0x22	*/
				 0,	/* unused		0x23	*/
				 0,	/* unused		0x24	*/
				 2,	/* SZ_RDCAP		0x25	*/
				 0,	/* unused		0x26	*/
				 0,	/* unused		0x27	*/
				 3,	/* SZ_READ_10		0x28	*/
				 0,	/* unused		0x29	*/
				 3,	/* SZ_WRITE_10		0x2a	*/
				 0,	/* SZ_SEEK_10		0x2b	*/
				 0,	/* unused		0x2c	*/
				 0,	/* unused		0x2d	*/
				 0,	/* unused		0x2e	*/
				 10,	/* SZ_VFY_DATA		0x2f	*/
				 0,	/* unused		0x30	*/
				 0,	/* unused		0x31	*/
				 0,	/* unused		0x32	*/
				 0,	/* unused		0x33	*/
				 0,	/* unused		0x34	*/
				 0,	/* unused		0x35	*/
				 0,	/* unused		0x36	*/
				 2,	/* SZ_RDD		0x37	*/
				 0,	/* unused		0x38	*/
				 0,	/* unused		0x39	*/
				 0,	/* unused		0x3a	*/
				 0,	/* unused		0x3b	*/
				 0,	/* unused		0x3c	*/
				 0,	/* unused		0x3d	*/
				 0,	/* unused		0x3e	*/
				 0,	/* unused		0x3f	*/
				 2,	/* SZ_CHANGE_DEFINITION 0x40	*/
				 0,	/* unused		0x41	*/
				 2,	/* SZ_READ_SUBCHAN	0x42	*/
				 2,	/* SZ_READ_TOC		0x43	*/
				 2,	/* SZ_READ_HEADER	0x44	*/
				90,	/* SZ_PLAY_AUDIO	0x45	*/
				 0,	/* unused		0x46	*/
				90,	/* SZ_PLAY_AUDIO_MSF	0x47	*/
				90,	/* SZ_PLAY_AUDIO_TI	0x48	*/
				90,	/* SZ_PLAY_TRACK_REL	0x49	*/
				 0,	/* unused		0x4a	*/
				 2,	/* SZ_PAUSE_RESUME	0x4b	*/
				 0,	/* unused		0x4c	*/
				 0,	/* unused		0x4d	*/
				 0,	/* unused		0x4e	*/
				 0,	/* unused		0x4f	*/
				 0,	/* unused		0x50	*/
				 0,	/* unused		0x51	*/
				 0,	/* unused		0x52	*/
				 0,	/* unused		0x53	*/
				 0,	/* unused		0x54	*/
				 0,	/* unused		0x55	*/
				 0,	/* unused		0x56	*/
				 0,	/* unused		0x57	*/
				 0,	/* unused		0x58	*/
				 0,	/* unused		0x59	*/
				 0,	/* unused		0x5a	*/
				 0,	/* unused		0x5b	*/
				 0,	/* unused		0x5c	*/
				 0,	/* unused		0x5d	*/
				 0,	/* unused		0x5e	*/
				 0,	/* unused		0x5f	*/
				90,	/* SZ_P_FSPACER		0x60	*/
				90,	/* SZ_P_FSPACEF		0x61	*/
				90,	/* SZ_P_BSPACER		0x62	*/
				90,	/* SZ_P_BSPACEF		0x63	*/
				 2,	/* SZ_P_CACHE		0x64	*/
				 2,	/* SZ_P_NOCACHE		0x65	*/
				 0,	/* SZ_P_LOAD		0x66	*/
				 0,	/* SZ_P_UNLOAD		0x67	*/
				 0,	/* SZ_P_SSUNIT		0x68	*/
				 0,	/* SZ_P_RETENSION	0x69	*/
				 1,	/* SZ_P_EJECT		0x6a	*/
				 0,	/* unused		0x6b	*/
				 0,	/* unused		0x6c	*/
				 0,	/* unused		0x6d	*/
				 0,	/* unused		0x6e	*/
				 0,	/* unused		0x6f	*/
				 0,	/* unused		0x70	*/
				 0,	/* unused		0x71	*/
				 0,	/* unused		0x72	*/
				 0,	/* unused		0x73	*/
				 0,	/* unused		0x74	*/
				 0,	/* unused		0x75	*/
				 0,	/* unused		0x76	*/
				 0,	/* unused		0x77	*/
				 0,	/* unused		0x78	*/
				 0,	/* unused		0x79	*/
				 0,	/* unused		0x7a	*/
				 0,	/* unused		0x7b	*/
				 0,	/* unused		0x7c	*/
				 0,	/* unused		0x7d	*/
				 0,	/* unused		0x7e	*/
				 0,	/* unused		0x7f	*/
				 0,	/* unused		0x80	*/
				 0,	/* unused		0x81	*/
				 0,	/* unused		0x82	*/
				 0,	/* unused		0x83	*/
				 0,	/* unused		0x84	*/
				 0,	/* unused		0x85	*/
				 0,	/* unused		0x86	*/
				 0,	/* unused		0x87	*/
				 0,	/* unused		0x88	*/
				 0,	/* unused		0x89	*/
				 0,	/* unused		0x8a	*/
				 0,	/* unused		0x8b	*/
				 0,	/* unused		0x8c	*/
				 0,	/* unused		0x8d	*/
				 0,	/* unused		0x8e	*/
				 0,	/* unused		0x8f	*/
				 0,	/* unused		0x90	*/
				 0,	/* unused		0x91	*/
				 0,	/* unused		0x92	*/
				 0,	/* unused		0x93	*/
				 0,	/* unused		0x94	*/
				 0,	/* unused		0x95	*/
				 0,	/* unused		0x96	*/
				 0,	/* unused		0x97	*/
				 0,	/* unused		0x98	*/
				 0,	/* unused		0x99	*/
				 0,	/* unused		0x9a	*/
				 0,	/* unused		0x9b	*/
				 0,	/* unused		0x9c	*/
				 0,	/* unused		0x9d	*/
				 0,	/* unused		0x9e	*/
				 0,	/* unused		0x9f	*/
				 0,	/* unused		0xa0	*/
				 0,	/* unused		0xa1	*/
				 0,	/* unused		0xa2	*/
				 0,	/* unused		0xa3	*/
				 0,	/* unused		0xa4	*/
				90,	/* SZ_PLAY_AUDIO_12	0xa5	*/
				90,	/* SZ_PLAY_TRACK_REL_12	0xa6	*/
				 0,	/* unused		0xa7	*/
				 0,	/* unused		0xa8	*/
				 0,	/* unused		0xa9	*/
				 0,	/* unused		0xaa	*/
				 0,	/* unused		0xab	*/
				 0,	/* unused		0xac	*/
				 0,	/* unused		0xad	*/
				 0,	/* unused		0xae	*/
				 0,	/* unused		0xaf	*/
				 0,	/* unused		0xb0	*/
				 0,	/* unused		0xb1	*/
				 0,	/* unused		0xb2	*/
				 0,	/* unused		0xb3	*/
				 0,	/* unused		0xb4	*/
				 0,	/* unused		0xb5	*/
				 0,	/* unused		0xb6	*/
				 0,	/* unused		0xb7	*/
				 0,	/* unused		0xb8	*/
				 0,	/* unused		0xb9	*/
				 0,	/* unused		0xba	*/
				 0,	/* unused		0xbb	*/
				 0,	/* unused		0xbc	*/
				 0,	/* unused		0xbd	*/
				 0,	/* unused		0xbe	*/
				 0,	/* unused		0xbf	*/
				 2,	/* SZ_SET_ADDRESS_FORMAT 0xc0	*/
				 0,	/* unused		0xc1	*/
				 0,	/* unused		0xc2	*/
				 0,	/* unused		0xc3	*/
				 2,	/* SZ_PLAYBACK_STATUS	0xc4	*/
				 0,	/* unused		0xc5	*/
				90,	/* SZ_PLAY_TRACK	0xc6	*/
				90,	/* SZ_PLAY_MSF		0xc7	*/
				90,	/* SZ_PLAY_VAUDIO	0xc8	*/
				 2,	/* SZ_PLAYBACK_CONTROL	0xc9	*/
				 0,	/* unused		0xca	*/
				 0,	/* unused		0xcb	*/
				 0,	/* unused		0xcc	*/
				 0,	/* unused		0xcd	*/
				 0,	/* unused		0xce	*/
				 0,	/* unused		0xcf	*/
				 0,	/* unused		0xd0	*/
				 0,	/* unused		0xd1	*/
				 0,	/* unused		0xd2	*/
				 0,	/* unused		0xd3	*/
				 0,	/* unused		0xd4	*/
				 0,	/* unused		0xd5	*/
				 0,	/* unused		0xd6	*/
				 0,	/* unused		0xd7	*/
				 0,	/* unused		0xd8	*/
				 0,	/* unused		0xd9	*/
				 0,	/* unused		0xda	*/
				 0,	/* unused		0xdb	*/
				 0,	/* unused		0xdc	*/
				 0,	/* unused		0xdd	*/
				 0,	/* unused		0xde	*/
				 0,	/* unused		0xdf	*/
				 0,	/* unused		0xe0	*/
				 0,	/* unused		0xe1	*/
				 0,	/* unused		0xe2	*/
				 0,	/* unused		0xe3	*/
				 0,	/* unused		0xe4	*/
				 0,	/* unused		0xe5	*/
				 0,	/* unused		0xe6	*/
				 0,	/* unused		0xe7	*/
				 0,	/* unused		0xe8	*/
				 0,	/* unused		0xe9	*/
				 0,	/* unused		0xea	*/
				 0,	/* unused		0xeb	*/
				 0,	/* unused		0xec	*/
				 0,	/* unused		0xed	*/
				 0,	/* unused		0xee	*/
				 0,	/* unused		0xef	*/
				 0,	/* unused		0xf0	*/
				 0,	/* unused		0xf1	*/
				 0,	/* unused		0xf2	*/
				 0,	/* unused		0xf3	*/
				 0,	/* unused		0xf4	*/
				 0,	/* unused		0xf5	*/
				 0,	/* unused		0xf6	*/
				 0,	/* unused		0xf7	*/
				 0,	/* unused		0xf8	*/
				 0,	/* unused		0xf9	*/
				 0,	/* unused		0xfa	*/
				 0,	/* unused		0xfb	*/
				 0,	/* unused		0xfc	*/
				 0,	/* unused		0xfd	*/
				 0,	/* unused		0xfe	*/
				 0	/* unused		0xff	*/
};

#include "scsi_debug.h"

int rdatdebug = 0;

int scsidebug = 0;		/* general purpose debugging var */
int sz_direct_track_mode = 0;	/* for testing multivolume dump */

#ifdef vax
/* TODO2: addr of sz_softc for cntlr 1 - DEBUG */
struct sz_softc *sz_sc1_addr = (struct sz_softc *)sz_softc + 1;
#endif vax
/*
 * TODO: comment
 */
int	szp_firstcall = 1;
int	szp_ntz = 0;
int	szp_nrz = 0;
int	szp_ncz = 0;
int     szp_nrx = 0;

/*
 * These are default scsi_devtab entires for unknown devices.
 * There are here in case the user removes the "UNKNOWN"
 * entries from the scsi_devtab in scsi_data.c.
 */
struct	scsi_devtab	szp_rz_udt =
	{"UNKNOWN", 7, DEV_RZxx, RZxx, sz_rzxx_sizes, 0, 0,
	SCSI_STARTUNIT|SCSI_MODSEL_PF, NO_OPTTABLE };

struct	scsi_devtab	szp_tz_udt =
	{"UNKNOWN", 7, DEV_TZxx, TZxx, sz_null_sizes, 0, 0, SCSI_NODIAG, NO_OPTTABLE};

struct	scsi_devtab	szp_cz_udt =
	{"UNKNOWN", 7, DEV_RZxx, RZxx, sz_rrd40_sizes, 0, 0,
	SCSI_STARTUNIT|SCSI_MODSEL_PF, NO_OPTTABLE };

/* TODO: debug (segcnt size limits), remove later */
/*	 If set, limits segcnt to value of ??_max_xfer */
/*	 only takes effect after reboot! */
/*	 eg: 16384 for 16KB */
/* TODO: limit xfer size until scsi hardware gets fixed. */
/* TODO: stray interrupts interfere with DMA xfers. */
#ifdef vax
int	rz_max_xfer = 0;
int	cz_max_xfer = 0;
int	tz_max_xfer = 0;
#endif vax
#ifdef mips
int	rz_max_xfer = 8192;
int	cz_max_xfer = 8192;
int	tz_max_xfer = 8192;
#endif mips
/* TODO: end of debug */

extern int get_validbuf();	/* for array indexing */

/*
 * Compare strings (at most n bytes):  s1>s2: >0  s1==s2: 0  s1<s2: <0
 * NOTE: count must be accurate, because only one string is null terminated.
 */

sz_strncmp(s1, s2, n)
register char *s1, *s2;
register int n;
{

	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(n<0 ? 0 : *s1 - *--s2);
}

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

sz_strcmp(s1, s2)
register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return(0);
	return(*s1 - *--s2);
}

char	sz_devname[SZ_DNSIZE+1];
char	sz_revlevel[SZ_REV_LEN+2];

/*
 *
 * Name:		szattach	-Attach routine
 *
 * Abstract:		This routine attaches a slave to the controller
 *			by filling in the unit information structure
 *			(a good deal of which is not used in this driver).
 *			Also fills in the transfer rate for iostat if
 *			the slave is a disk. Prints the device type
 *			during autoconfigure.
 *
 * Inputs:
 *
 * ui			Unit information structure pointer.
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 * Side Effects:	None.
 *
 */

szattach(ui)
	register struct uba_device *ui;
{
	register struct uba_ctlr *um;
	register struct sz_softc *sc;
	register struct scsi_devtab *sdp;
	int i;

	sc = &sz_softc[ui->ui_ctlr];
	sdp = (struct scsi_devtab *)sc->sc_devtab[ui->ui_slave];
	/*
	 * NOTE:
	 *	This cannot happen, because the slave number for
	 *	the initator is never alive. We check anyway to be sure.
	 */
	if ((1 << ui->ui_slave) == sc->sc_sysid)
	    return;
	ui->ui_flags = 0;
	um = ui->ui_mi;
	szutab[ui->ui_unit].b_actf = NULL;
	szutab[ui->ui_unit].b_actl = NULL;

	sc->sc_attached[ui->ui_slave] = 1;	

	/*
	 * Initialize iostat sec/word transfer rate
	 * for each disk/cdrom.
	 * RZ22  1.0 / (60 * 33 * 256) (max data per one revolution)
	 * RZ23  1.0 / (60 * 33 * 256) (max data per one revolution)
	 * RZ55  1.0 / (60 * 36 * 256) (max data per one revolution)
	 * RRD40 176.4 Kilobytes/second from spec
	 */
	if (ui->ui_dk >= 0) {
	    sc->sc_dkn[ui->ui_slave] = ui->ui_dk;
#ifdef NOWAY
	    if (sc->sc_devtyp[ui->ui_slave] == RZ22)
		    dk_mspw[ui->ui_dk] = 0.000002;
	    else if (sc->sc_devtyp[ui->ui_slave] == RZ23)
		    dk_mspw[ui->ui_dk] = 0.000002;
	    else if (sc->sc_devtyp[ui->ui_slave] == RZ55)
		    dk_mspw[ui->ui_dk] = 0.0000018;
	    else if (sc->sc_devtyp[ui->ui_slave] == RRD40)
		    dk_mspw[ui->ui_dk] = 0.0000113;
	    else {
		    sc->sc_dkn[ui->ui_slave] = -1;
		    dk_mspw[ui->ui_dk] = 0.0;
#endif NOWAY
        }
	else {
	    sc->sc_dkn[ui->ui_slave] = -1;
	}

#ifdef mips
	/*
	 * This flag means that the drive is alive.
	 */
	dk_mspw[ui->ui_dk] = 1;
#endif mips

	/* so device name appears after slave printout in autoconf */
	printf(" (%s)", sc->sc_device[ui->ui_slave]);
	/* print vendor/product ID & rev level if non-DEC disk */
	if ((sc->sc_devtyp[ui->ui_slave] == RZxx) ||
	    (sc->sc_devtyp[ui->ui_slave] == TZxx)) {
	    for (i = 0; i < SZ_DNSIZE; i++)
		sz_devname[i] = sc->sc_devnam[ui->ui_slave][i];
	    sz_devname[SZ_DNSIZE] = 0;
	    for (i = 0; i < SZ_REV_LEN; i++)
		sz_revlevel[i] = sc->sc_revlvl[ui->ui_slave][i];
	    sz_revlevel[SZ_REV_LEN] = 0;
	    printf(" [ %s %s ]", sz_devname, sz_revlevel);
	}
}

/*
 * Name:		szslave		-Slave routine
 *
 * Abstract:		This routines determines whether or not a slave
 *			device is alive during autoconfigure.
 *
 * Inputs:
 *
 * ui			Unit information structure pointer.
 *
 * Outputs:		None.
 *
 * Return Values:
 *
 * 0			Slave is not alive.
 *
 * 1			Slave is alive.
 *
 * Side Effects:
 *			IPL raised to 15.
 *
 */

szslave(ui)
	register struct uba_device *ui;
{
	register struct uba_ctlr *um = szminfo[ui->ui_ctlr];
	register struct sz_softc *sc;
	register int s = splbio();
	int alive;

	alive = 1;
	while(1) {
	    /* dead cntlr */
	    if (um->um_alive == 0) {
		alive = 0;
		break;
	    }
	    sc = &sz_softc[ui->ui_ctlr];
	    /* not target ID alive for this slave number */
	    if (sc->sc_alive[ui->ui_slave] == 0) {
		alive = 0;
		break;
	    }
	    /* see if correct device type, i.e., rz=disk or cdrom, tz=tape */
	    if ((strcmp("rz", ui->ui_devname) == 0) &&
		(sc->sc_devtyp[ui->ui_slave] & SZ_DISK)) {
		    break;
	    }
	    if ((strcmp("rz", ui->ui_devname) == 0) &&
		(sc->sc_devtyp[ui->ui_slave] & SZ_CDROM)) {
		    break;
	    }
	    if ((strcmp("tz", ui->ui_devname) == 0) &&
		(sc->sc_devtyp[ui->ui_slave] & SZ_TAPE)) {
		    break;
	    }
	    alive = 0;
	    break;
	}
	if (alive) {

	    sc->sc_unit[ui->ui_slave] = ui->ui_unit;

        }	/* end if alive */
		
	splx(s);
	return(alive);
}

int	sz_bp_pf = 0;

/******************************************************************
 *
 * Name:	sz_bldpkt
 *
 * Abstract:	Build a packet that is then passed one byte at a time
 * 		to the SCSI device.
 *
 * Inputs:
 * 	sc	Pointer to softc structure for controller.
 *	targid  Target.
 *	cmd	The SCSI command to build.
 *	addr	The logical block address.
 *	count	The number of blocks.
 *
 * Outputs:	None.
 *
 * Return values: None.
 *
 ******************************************************************/

sz_bldpkt(sc, targid, cmd, addr, count)

register struct sz_softc *sc;
int	targid;
long	cmd;
daddr_t	addr;
long	count;
{
	register u_char *byteptr;
	register int i;
	struct scsi_devtab *sdp;
	struct buf *bp = sc->sc_bp[targid]; /* get our bp filled in sz_start */
	struct format_params *fp;
	struct read_defect_params *rdp;
	struct verify_params *vp;
	struct reassign_params *rp;
	struct mode_sel_sns_params *msp;
	struct io_uxfer *iox;
	struct sz_rwl_cm *prwl;
	int len;
	struct tape_opt_tab *todp; /* our tape option table pointer */
	struct disk_opt_tab *dodp; /* our disk option table pointer */
	struct tape_info *ddp;  /* our density struct for tapes */

	sdp = (struct scsi_devtab *)sc->sc_devtab[targid];
	/*
	 * Save cmd in sc_actcmd, so we know what command
	 * we are really working on. For example, the current
	 * command could be read, but the actual command
	 * might be a request sense after the read.
	 */
	sc->sc_actcmd[targid] = cmd;

	/*
	 * Get a byte address of the begining of sc->sz_cmd space,
	 * and clear the data space
	 */
	byteptr = (u_char *)&sc->sz_command;
	for (i=0; i < SZ_MAX_CMD_LEN; i++)
		*byteptr++ = 0;

	PRINTD(targid, 0x20, ("sz_bldpkt: command = 0x%x\n", cmd));

	/* build the comand packet */

	bzero((char *)&sc->sz_dat[targid], sizeof(sc->sz_dat[targid]));

	switch (cmd) {
		case SZ_READ:
		case SZ_WRITE:
		case SZ_RBD:
		    if (sc->sc_devtyp[targid] & SZ_TAPE) {	/* TAPE r/w */
			/*
			 * Check to see if we have an option table
			 * if we do use those values in the table to set
			 * things up.......
			*/
			if( sdp->opt_tab != NULL ){
			    /*
			     * we have an option table use it.
			     * must cast the pointer
			    */
			    todp = (struct tape_opt_tab *)sdp->opt_tab;

			    /* 
			     * get our density struct pointer...
			    */
			    ddp = &todp->tape_info[DENS_IDX( bp )];

			    /* 
			     * since this is just a simple read/write
			     * all we have to do is determine if fixed
			     * block add convert to bytes/block..
			    */
			    /* 
			     * lets see if this density selection is valid..
			    */
			    if( ddp->tape_flags & DENS_VAL ){
				/*
				 * this is valid lets get our stuff
				*/
				if( ddp->blk_size != NULL ){  
				    /* 
				     * since blk size is non 0 
				     * this must be a fixed block tape dens
				    */
				    sc->sz_t_read.fixed = 1; /* fixed blocks */
				    /*
				     * since fixed the count is in blocks not
				     * bytes
				    */
				    i = count/ddp->blk_size;
				    sc->sz_t_read.xferlen2 = LTOB( i, 2);
				    sc->sz_t_read.xferlen1 = LTOB( i, 1);
				    sc->sz_t_read.xferlen0 = LTOB( i, 0);
				}
				else {
				    sc->sz_t_read.fixed = 0; 
				    /*
				     * we don't have to convert the btye
				     * count to blocks
				    */
				    sc->sz_t_read.xferlen2 = LTOB( count, 2);
				    sc->sz_t_read.xferlen1 = LTOB( count, 1);
				    sc->sz_t_read.xferlen0 = LTOB( count, 0);
				}
			    }
			    /*
			     * well we have a option table but this density
			     * struct is not valid.... We must do something
			     * but is it QIC 9trk 8mm etc... 
			    */
			    else {
				
				/* ERRLOG_MARK */
				cprintf("scsi OPTTABLE off id = %X\n", targid);
			        sc->sz_t_read.fixed = 0; 
			        sc->sz_t_read.xferlen2 = LTOB( count, 2);
			        sc->sz_t_read.xferlen1 = LTOB( count, 1);
			        sc->sz_t_read.xferlen0 = LTOB( count, 0);
			    }
			    
			/* end of if tape option table */
			} 
				
			/* 
			 * There is no option table... The default is
			 * a normal tape drive......
			*/
			else{
			    sc->sz_t_read.fixed = 0;	/* only records */
			    sc->sz_t_read.xferlen2 = LTOB(count,2);
			    sc->sz_t_read.xferlen1 = LTOB(count,1);
			    sc->sz_t_read.xferlen0 = LTOB(count,0);
			}
		    /* end of if tape */
		    }
		
		    else {		/* DISK r/w */
			sc->sz_d_read.lbaddr2 = LTOB(addr,2);
			sc->sz_d_read.lbaddr1 = LTOB(addr,1);
			sc->sz_d_read.lbaddr0 = LTOB(addr,0);
			sc->sz_d_read.xferlen = LTOB(count/512,0);
		    }
		    break;

		case SZ_READ_10:
		case SZ_WRITE_10: {
		    int blocks = (count / 512);

		    sc->sz_d_rw10.lbaddr3 = LTOB(addr, 3);
		    sc->sz_d_rw10.lbaddr2 = LTOB(addr, 2);
		    sc->sz_d_rw10.lbaddr1 = LTOB(addr, 1);
		    sc->sz_d_rw10.lbaddr0 = LTOB(addr, 0);
		    sc->sz_d_rw10.xferlen1 = LTOB(blocks, 1);
		    sc->sz_d_rw10.xferlen0 = LTOB(blocks, 0);
		    break;
		}
		case SZ_FORMAT:
		    fp = (struct format_params *)
					    sc->sc_rzparams[targid];
		    if(fp->fp_defects == VENDOR_DEFECTS) {
			    sc->sz_d_fu.fmtdat = 1;
			    sc->sz_d_fu.cmplst = 1;
		    }
		    else if(fp->fp_defects == KNOWN_DEFECTS) {
			    sc->sz_d_fu.fmtdat = 1;
			    sc->sz_d_fu.cmplst = 0;
		    }
		    else if(fp->fp_defects == NO_DEFECTS) {
			    sc->sz_d_fu.fmtdat = 0;
			    sc->sz_d_fu.cmplst = 0;
		    }
		    sc->sz_d_fu.dlf = fp->fp_format;
		    sc->sz_d_fu.pattern = fp->fp_pattern;
		    sc->sz_d_fu.interleave1 = LTOB(fp->fp_interleave,1);
		    sc->sz_d_fu.interleave0 = LTOB(fp->fp_interleave,0);
		    break;
		case SZ_REASSIGN:
		    break;
		case SZ_RDD:
		    rdp = (struct read_defect_params *)
					    sc->sc_rzparams[targid];
		    sc->sz_d_rdd.m = 1;
		    sc->sz_d_rdd.g = 1;
		    sc->sz_d_rdd.dlf = rdp->rdp_format;
		    sc->sz_d_rdd.alclen1 = LTOB(rdp->rdp_alclen,1);
		    sc->sz_d_rdd.alclen0 = LTOB(rdp->rdp_alclen,0);
		    break;
		case SZ_VFY_DATA:
		    vp = (struct verify_params *)
					    sc->sc_rzparams[targid];
		    sc->sz_d_vd.reladr = 0;
		    sc->sz_d_vd.bytchk = 0;
		    sc->sz_d_vd.lbaddr3 = LTOB(vp->vp_lbn,3);
		    sc->sz_d_vd.lbaddr2 = LTOB(vp->vp_lbn,2);
		    sc->sz_d_vd.lbaddr1 = LTOB(vp->vp_lbn,1);
		    sc->sz_d_vd.lbaddr0 = LTOB(vp->vp_lbn,0);
		    sc->sz_d_vd.verflen1 = LTOB(vp->vp_length,1);
		    sc->sz_d_vd.verflen0 = LTOB(vp->vp_length,0);
		    break;

		case SZ_READL:
		case SZ_WRITEL:
		    iox = (struct io_uxfer *) sc->sc_rzparams[targid];
		    prwl = (struct sz_rwl_cm *) &(iox->io_cdb[1]);

		    sc->sz_rwl.reladr = prwl->reladr;
		    sc->sz_rwl.lun = prwl->lun;
		    sc->sz_rwl.phad = prwl->phad;
		    sc->sz_rwl.lbaddr3 = prwl->lbaddr3;
		    sc->sz_rwl.lbaddr2 = prwl->lbaddr2;
		    sc->sz_rwl.lbaddr1 = prwl->lbaddr1;
		    sc->sz_rwl.lbaddr0 = prwl->lbaddr0;
		    sc->sz_rwl.dspec = prwl->dspec;
		    break;

		case SZ_RQSNS:
		    /*
		     * Check to see if we have an option table
		     * if we do use those values in the table to set
		     * things up.......
		    */
		    if( sdp->opt_tab != NULL ){
			/*
			* we have an option table use it.
			* must cast the pointer
			*/
			if (sc->sc_devtyp[targid] & SZ_TAPE) {	
			    todp = (struct tape_opt_tab *)sdp->opt_tab;
			    /* 
			     * lets see if the request sense is valid
			    */
			    if( todp->opt_flags & RSNS_ALLOCL_VAL){
			    	/* 
			       	 * is the request size larger then storage
			    	*/
			    	if( todp->rsns_allocl > 
					sizeof( struct sz_exsns_dt)){
				    /*
			 	     * truncate the size
				    */
				    sc->sz_rqsns.alclen = 
					sizeof(struct sz_exsns_dt);
			        }
			        else { 
				    sc->sz_rqsns.alclen = todp->rsns_allocl;
			        }
			    }
			    /* 
			     * we have an option table but they never validated
			     * the request sense size default it....
			    */
			    else {
			        sc->sz_rqsns.alclen = SZ_RQSNS_LEN;
			    }
			}
			else { /* must be disk */
			    dodp = (struct disk_opt_tab *)sdp->opt_tab;
			    /* 
			     * lets see if the request sense is valid
			    */
			    if( dodp->opt_flags & RSNS_ALLOCL_VAL){
			    	/* 
			       	 * is the request size larger then storage
			    	*/
			    	if( dodp->rsns_allocl > 
						sizeof( struct sz_exsns_dt)){
				    /*
			 	     * truncate the size
				    */
				    sc->sz_rqsns.alclen = 
						sizeof(struct sz_exsns_dt);
			        }
			        else { 
				    sc->sz_rqsns.alclen = dodp->rsns_allocl;
			        }
			    }
			    /* 
			     * we have an option table but they never validated
			     * the request sense size default it....
			    */
			    else {
			        sc->sz_rqsns.alclen = SZ_RQSNS_LEN;
			    }
			}

		    /*
		     * end of if option table
		    */
		    }
		    else{	/* default  No option table */
			sc->sz_rqsns.alclen = SZ_RQSNS_LEN;
		    }
		    break;
		case SZ_MODSNS:
		    /* 
		     * The tape section 
		    */
		    if (sc->sc_devtyp[targid] & SZ_TAPE) {	
	 		/*
			 * Check to see if we have an option table
			 * if we do use those values in the table to set
			 * things up.......
		   	*/
			if( sdp->opt_tab != NULL ){
			    /*
			     * we have an option table use it.
			     * must cast the pointer
			    */
			     todp = (struct tape_opt_tab *)sdp->opt_tab;

			    /* 
			     * lets see if the mode sense is valid
			    */
			    if( todp->opt_flags & MSNS_ALLOCL_VAL){
				/* 
				 * is the request size larger then storage
				 * The storage area is 44 bytes now....
				 * This large enougth for a head +block +page.
				 * I know that scsi_tape is not now requesting
				 * any pages but that can change.... 
				 * for tapes the msns_allocl field should
				 * just be the the header + block size and
				 * we do the addition for the page if requested.
				*/
				if( count < 0 ){ 
				    /* no pages .....*/
				    if( todp->msns_allocl > 
						sizeof( struct sz_datfmt)){
					/*
					 * truncate the size
					*/
					sc->sz_modsns.alclen = 
						sizeof(struct sz_datfmt);
					/* ERRLOG_MARK */
					cprintf("scsi OPTTABLE off id = %X\n", 
						targid);
				    }
				    else { 
					sc->sz_modsns.alclen = 
						todp->msns_allocl;
				    }
				}
				/* 
				 * Well count is => 0 must mean this is scsi 2
				 * and we are requesting a page along with this
				 * must check our size...
				*/
				else{ 
				    if( todp->opt_flags & PAGE_VAL) {
					if(( todp->msns_allocl + 
						todp->page_size) >
						sizeof(struct sz_datfmt)){

					    /* ERRLOG_MARK */
					    cprintf("scsi OPTTABLE off id = %X\n", targid);
					    sc->sz_modsns.alclen = 
						sizeof(struct sz_datfmt);
					}
					else { 
					    sc->sz_modsns.alclen = 
						( todp->msns_allocl +
							todp->page_size);
					}
				    }
				    else {
					/* whoever set up this tape unit 
					 * (dec/customer)
					 * said this was a scsi 2 implementation
					 * but  not everything is set up ... 
					 * lets notify
					 * and try to salvage
					*/

					/* ERRLOG_MARK */
					cprintf("scsi OPTTABLE off id = %X\n", targid);
					if( todp->msns_allocl > 
						sizeof(struct sz_datfmt)){
					    sc->sz_modsns.alclen = sizeof(struct sz_datfmt);
					}
					else { 
					    sc->sz_modsns.alclen = 
						todp->msns_allocl;
					}
				    }
				/*
				 * since count => 0 must set up the page control
				 * fields
				*/
				sc->sz_modsns.pgcode = count & 0x3f;
				sc->sz_modsns.pcf = (count >> 6 ) & 0x3;
				/* end of if count => 0 */
				}
			    /* end of if MSNS_ALLOCL_VAL */
			    }
			    /* 
			     * we have an option table but they never validated
			     * the mode sense size default it....
			    */
			    else {
				/* ERRLOG_MARK */
				cprintf("scsi OPTTABLE off id = %X\n", targid);
				sc->sz_rqsns.alclen = SZ_MODSNS_LEN;
			    }
			/*
			 * end of if option table
			*/
			}

			else{	/* default  No option table for all the old stuff */
			    /* EXABYTE tape has 2 more mode sense data bytes */

			    if (sdp->flags & SCSI_MODSEL_EXABYTE){
    				sc->sz_modsns.alclen = 16;
			    }
			    else{
				sc->sz_modsns.alclen = SZ_MODSNS_LEN;
			    }
		        }
		    /* End of if tape */
		    }
		    /* 
		     * Now we start with the disk 
		     * else if this for more scsi devices
		    */
		    else {					/* DISK/CDROM */
	 		/*
			 * Check to see if we have an option table
			 * if we do use those values in the table to set
			 * things up.......
		   	*/
			if( sdp->opt_tab  != NULL ){
			    /*
			     * we have an option table use it.
			     * must cast the pointer
			    */
			     dodp = (struct disk_opt_tab *)sdp->opt_tab;

			    /* 
			     * lets see if the mode sense is valid
			    */
			    if( dodp->opt_flags & MSNS_ALLOCL_VAL){
				/* 
				 * is the request size larger then storage
				 * The storage area is 44 bytes now....
				 * This large enougth for a head +block +page.
				 * the msns_allocl field should
				 * just be the the header + block size and
				 * we do the addition for the page if requested.
				*/
				if( count < 0 ){ 
				    /* no pages .....*/
				    if( dodp->msns_allocl > 
					     sizeof( struct sz_rzmodsns_dt)){
					/*
					 * truncate the size
					*/
					sc->sz_modsns.alclen = 
						sizeof(struct sz_rzmodsns_dt);
					/* ERRLOG_MARK */
					cprintf("scsi OPTTABLE off id = %X\n", 
						targid);
				    }
				    else { 
					sc->sz_modsns.alclen = dodp->msns_allocl;
				    }
				}
				/* 
				 * Well count is => 0 must mean this is scsi 2
				 * and we are requesting a page along with this
				 * must check our size...
				*/
				else{ 
				    if( dodp->opt_flags & PAGE_VAL) {
					if(( dodp->msns_allocl + 
						dodp->page_size) >
						sizeof(struct sz_rzmodsns_dt)){
					    sc->sz_modsns.alclen = 
						sizeof(struct sz_rzmodsns_dt);
					    /* ERRLOG_MARK */
					    cprintf("scsi OPTTABLE off id = %X\n", targid);
					}
					else { 
					    sc->sz_modsns.alclen = 
							(dodp->msns_allocl +
							dodp->page_size);
					}
				    }
				    else {
					/* whoever set up this disk unit 
					 * (dec/customer)
					 * said this was a scsi 2 implementation
					 * but not everything is set up ... 
					 * lets notify
					 * and try to salvage
					*/

					/* ERRLOG_MARK */
					cprintf("scsi OPTTABLE off id = %X\n", 
						targid);
					if( dodp->msns_allocl > 
						sizeof(struct sz_rzmodsns_dt)){

					    sc->sz_modsns.alclen = 
						  sizeof(struct sz_rzmodsns_dt);
					}
					else { 
					    sc->sz_modsns.alclen = 
						   todp->msns_allocl;
					}
				    }
				/*
				 * since count => 0 must set up the page control
				 * fields
				*/
				sc->sz_modsns.pgcode = count & 0x3f;
				sc->sz_modsns.pcf = (count >> 6 ) & 0x3;

				/* end of if count => 0 */
				}
			    /* end of if MSNS_ALLOCL_VAL */
			    }
			    /* 
			     * we have an option table but they never validated
			     * the mode sense size default it....
			    */
			    else {
				if( count < 0 ){
				    /* 
				     * scsi 2 ?? just header + block 
				    */
				    sc->sz_modsns.alclen = 12;
				    /* ERRLOG_MARK */
				    cprintf("scsi OPTTABLE off id = %X\n", 
						targid);
				}
				else { 
				    /*
				     * count says we want a page lets doit...
				    */
				    sc->sz_modsns.alclen =
						sizeof(struct sz_rzmodsns_dt);
				    sc->sz_modsns.pgcode = count & 0x3f;
				    sc->sz_modsns.pcf = (count >> 6) & 0x3;
				}

			    }
			/*
			 * end of if option table
			*/
			}
			/*
			 * no option table default old disks
			*/
			else{
			    if( count < 0){
				/* 
				 * size is 4 byte header plus 8 byte block desc.
				*/
				sc->sz_modsns.alclen = 12; 
			    }
			    else {
				/* 
				 * hdr + blk + 1 (32 byte page)
				*/
				sc->sz_modsns.alclen = 
					sizeof(struct sz_rzmodsns_dt);
				sc->sz_modsns.pgcode = count & 0x3f;
				sc->sz_modsns.pcf = (count >> 6) & 0x3;
			    }
			}
			/* 
			 * For special disk commands
			 * Like format and reassigning bad blocks... done
			 * thru a ioctl and all the data structs are set
			 * up by the utility that....
			 * don't mess with this.................the
			 * utility knows best.. I hope.
			*/
			if(sc->sc_rzspecial[targid]) {
			    msp = (struct mode_sel_sns_params *)
					sc->sc_rzparams[targid];
			    sc->sz_modsns.pgcode = msp->msp_pgcode;
			    sc->sz_modsns.pcf = msp->msp_pgctrl;
			    sc->sz_modsns.alclen = msp->msp_length;
			}
		    }
		    break;
		case SZ_INQ:
		    sc->sz_rqsns.alclen = SZ_INQ_MAXLEN;
		    break;
		case SZ_REWIND:
		case SZ_RBL:
		case SZ_TUR:
		    break;
		case SZ_RDCAP:			/* DISK: READ CAPACITY */
		    break;			/* All fields zero */
		case SZ_WFM:
		    sc->sz_wfm.numoffmks2 = 0; /* max of 65536 file marks */
		    sc->sz_wfm.numoffmks1 = LTOB(count,1);
		    sc->sz_wfm.numoffmks0 = LTOB(count,0);
		    break;
		case SZ_P_BSPACEF:
		    count = -count;
		case SZ_P_FSPACEF:
		    cmd = SZ_SPACE;
		    sc->sz_space.code = 1;
		    sc->sz_space.count2 = LTOB(count,2);
		    sc->sz_space.count1 = LTOB(count,1);
		    sc->sz_space.count0 = LTOB(count,0);
		    break;
		case SZ_P_BSPACER:
		    count = -count;
		case SZ_P_FSPACER:
		    cmd = SZ_SPACE;
		    sc->sz_space.code = 0;
		    sc->sz_space.count2 = LTOB(count,2);
		    sc->sz_space.count1 = LTOB(count,1);
		    sc->sz_space.count0 = LTOB(count,0);
		    break;
		case SZ_VFY:
		    sc->sz_vfy.fixed = 1;	/* only records */
		    sc->sz_vfy.bytcmp = 0;	/* CRC verify only */
		    sc->sz_vfy.verflen2 = LTOB(count,2);
		    sc->sz_vfy.verflen1 = LTOB(count,1);
		    sc->sz_vfy.verflen0 = LTOB(count,0);
		    break;
		case SZ_ERASE:
		    sc->sz_erase.longbit = 0; /* don't erase to end
						    of tape */
		    break;
		case SZ_P_LOAD:
		    cmd = SZ_SSLU;
		    sc->sz_load.load = 1;  /* load only for now */
		    sc->sz_load.reten = 0; /* not used on MAYA    */
		    break;
		case SZ_P_UNLOAD:
		    cmd = SZ_SSLU;
		    sc->sz_load.load = 0;  /* unload only for now */
		    sc->sz_load.reten = 0; /* not used on MAYA    */
		    break;
		case SZ_P_RETENSION:
		    cmd = SZ_SSLU;
		    sc->sz_load.load = 1;  /* make sure its loaded */
		    sc->sz_load.reten = 1; /* retension tape qic unit */
		    break;
		case SZ_P_SSUNIT: /* DISK: start/stop unit (count=1 to start) */
		    cmd = SZ_SSLU;
		    sc->sz_load.load = (count & 1);
		    sc->sz_load.immed = 1;
		    break;
		case SZ_RECDIAG: {
		    if (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) {
			struct diagnostic_params *dp;

			dp = (struct diagnostic_params *) sc->sc_rzparams[targid];
			sc->sz_recdiag.alloc_len1 = LTOB(dp->dp_length,1);
			sc->sz_recdiag.alloc_len0 = LTOB(dp->dp_length,0);
		    } else {
			sc->sz_recdiag.alloc_len1 = 0;
			sc->sz_recdiag.alloc_len0 = SZ_RECDIAG_LEN;
		    }
		    break;
		}
		case SZ_SNDDIAG: {
		    struct diagnostic_params *dp;

		    dp = (struct diagnostic_params *) sc->sc_rzparams[targid];
		    sc->sz_snddiag.control = dp->dp_control;
		    sc->sz_snddiag.param_len1 = LTOB(dp->dp_length,1);
		    sc->sz_snddiag.param_len0 = LTOB(dp->dp_length,0);
		    break;
		}
		case SZ_MODSEL:
		case SZ_P_CACHE:
		    cmd = SZ_MODSEL;
		    /* 
		     * set these here  will be changed 
		     * later if need be
		    */
		    sc->sz_modsel.pll = 12;
		    sc->sz_modsel.rdeclen = 8;

		    if (sc->sc_devtyp[targid] & SZ_TAPE) {
	 		/*
			 * Check to see if we have an option table
			 * if we do use those values in the table to set
			 * things up.......
		   	*/
			if( sdp->opt_tab != NULL ){
			    /*
			     * we have an option table use it.
			     * must cast the pointer
			    */
			     todp = (struct tape_opt_tab *)sdp->opt_tab;

			    /* 
			     * lets see if the mode select parameter length 
			     * is valid
			     * We must build it by hand...........
			    */
			    if( todp->opt_flags & MSEL_PLL_VAL){
				sc->sz_modsel.pll = todp->msel_pll;
				if( todp->opt_flags & MSEL_BLKDL_VAL){
				    sc->sz_modsel.rdeclen = todp->msel_blkdl;
				}
				if( todp->opt_flags & MSEL_VUL_VAL){
				    sc->sz_modsel.vulen = todp->msel_vul;
				}
			    /* end of if MSEL_PLL_VAL */
			    }
			    else{
				/* 
				 * no sense in testing anything else
				 * default to hdr and block ( 12 )
				*/
				sc->sz_modsel.pll = 12;
				sc->sz_modsel.rdeclen = 8;
				/* ERRLOG_MARK */
				cprintf("scsi OPTTABLE off id = %X\n", targid);
			    }
			    /* 
			     * get our density struct pointer...
			    */
			    ddp = &todp->tape_info[DENS_IDX( bp )];

			    /* 
			     * lets see if this density selection is valid..
			    */
			    if( ddp->tape_flags & DENS_VAL ){
				/*
				 * this is valid lets get our stuff
				*/
				sc->sz_modsel.density = 
					(char)(SCSI_DENS_MASK & ddp->dens);
				
				/*
				 * must set our block size......
				*/
				sc->sz_modsel.reclen2 = LTOB(ddp->blk_size,2);
				sc->sz_modsel.reclen1 = LTOB(ddp->blk_size,1);
				sc->sz_modsel.reclen0 = LTOB(ddp->blk_size,0);
			    }
			    /*
			     * well we have a option table but this density
			     * struct is not valid.... We must do something
			     * but is it QIC 9trk 8mm etc... 
			    */
			    else {
				/* ERRLOG_MARK */
				cprintf("scsi OPTTABLE off id = %X\n", targid);

				/* default it */
				sc->sz_modsel.density = 0;
			        /*
				 * since we zeroed the command struct before
				 * we got here we don't have to set zero's
				 * in the block size field
				*/

			    }
			    /* 
			     * check speed setting 
			    */
			    if( ddp->tape_flags & SPEED_VAL){
				sc->sz_modsel.speed = 
					(SCSI_SPEED_MASK & ddp->tape_speed);
			    } 
			    if( todp->opt_flags & BUF_MOD ){
				sc->sz_modsel.bufmode =  1;
			    }
			    /*
			     * is this scsi 2 if so must set pf.
			    */
			    if( sdp->flags & SCSI_MODSEL_PF ){
				sc->sz_modsel.pf = 1;
			    }
				
			/*
			 * end of if option table for tapes
			*/
			}

			/* 
			 * default the tape settings backwards compat
			*/

			else { 
			    sc->sz_modsel.bufmode = 1;
			    sc->sz_modsel.rdeclen = 8;
			    if ((sc->sc_devtyp[ targid ] == TZxx) ||
					(sc->sc_devtyp[ targid ] == TZ05) ||
					(sc->sc_devtyp[ targid ] == TLZ04)) {
				if ((sdp->flags & SCSI_MODSEL_EXABYTE) &&
						    (tz_exabyte_vu != -1)) {
				    /* Do extended mode select for EXABYTE */
				    sc->sz_modsel.pll = 16;
				    /* see scsi_data.c for values */
				    sc->sz_modsel.vulen = tz_exabyte_vu;
				    sc->sz_modsel.pad[0] = tz_exabyte_mt;
				    sc->sz_modsel.pad[1] = tz_exabyte_rt;
			        }
				else {
				    /* Don't send any VU bits. */
				    sc->sz_modsel.pll = 12;
				    sc->sz_modsel.rdeclen = 8;
				}
			    }
			    /* END of if TZXX TZ05 TLZ04 */
			    else {
				sc->sz_modsel.pll = 14;
				sc->sz_modsel.vulen = 1;
#ifdef SZDEBUG
				sc->sz_modsel.vu7 = sz_direct_track_mode;
#endif SZDEBUG
				sc->sz_modsel.notimo = 1;
				sc->sz_modsel.nof = sz_max_numof_fills;
			    }

			/* end of tape NO option table */
			}

		    /* 
		     * end of if tape ...... 
		    */
		    }

		    else {		/* DISK or CDROM */
	 		/*
			 * Check to see if we have an option table
			 * if we do use those values in the table to set
			 * things up.......
		   	*/
			if( sdp->opt_tab != NULL ){
			    /*
			     * we have an option table use it.
			     * must cast the pointer
			    */
			     dodp = (struct disk_opt_tab *)sdp->opt_tab;

			    /* 
			     * lets see if the mode select parameter length is valid
			     * We must build it build hand...........
			    */
			    if( dodp->opt_flags & MSEL_PLL_VAL){
				sc->sz_modsel.pll = dodp->msel_pll;
				if( dodp->opt_flags & MSEL_BLKDL_VAL){
				    sc->sz_modsel.rdeclen = dodp->msel_blkdl;
				}
				if( dodp->opt_flags & MSEL_VUL_VAL){
				    sc->sz_modsel.vulen = dodp->msel_vul;
				}
			    /* end of if MSEL_PLL_VAL */
			    }
			    else{
				/* 
				 * no sense in testing anything else
				 * default to hdr and block ( 12 )
				*/
				sc->sz_modsel.pll = 12;
				sc->sz_modsel.rdeclen = 8;
				/* ERRLOG_MARK */
				cprintf("scsi OPTTABLE off id = %X\n", targid);
			    }
				
			    /*
			     * must set our block size..512 for disks
			    */
			    i = 512;
			    sc->sz_modsel.reclen2 = LTOB(i,2);
			    sc->sz_modsel.reclen1 = LTOB(i,1);
			    sc->sz_modsel.reclen0 = LTOB(i,0);

			    /* 
			     * get our other flags and set up
			    */
			    if( dodp->opt_flags & BUF_MOD ){
				sc->sz_modsel.bufmode =  1;
			    }
			    /*
			     * is this scsi 2 if so must set pf.
			    */
			    if( sdp->flags & SCSI_MODSEL_PF ){
				sc->sz_modsel.pf = 1;
			    }
			
			/* end of if disk option table */
			}

			/* 
			 * we have no option table for this
			 * disk... This is here for backward
			 * compat.
			*/
			else { 
			    if(sdp->flags & SCSI_MODSEL_PF){
				sc->sz_modsel.pf = 1;
			    }
			    else{
				sc->sz_modsel.pf = 0;
			    }
			    sc->sz_modsel.pll = 12;
			    sc->sz_modsel.rdeclen = 8;
			    i = 512;	/* reclen really means LBN size */
			    sc->sz_modsel.reclen2 = LTOB(i,2);
			    sc->sz_modsel.reclen1 = LTOB(i,1);
			    sc->sz_modsel.reclen0 = LTOB(i,0);
			    if(sc->sc_rzspecial[targid]) {
				msp = (struct mode_sel_sns_params *)
					    sc->sc_rzparams[targid];
				sc->sz_modsel.pll = msp->msp_length;
				sc->sz_modsel.sp = msp->msp_setps;
			    }
			}
		    }
		    break;
		case SZ_P_NOCACHE:
		    /* 
		     * The following is only for tapes....
		    */
		    cmd = SZ_MODSEL;
		    if( sdp->opt_tab != NULL ){
			/*
			 * we have an option table use it.
			 * must cast the pointer
			*/
			todp = (struct tape_opt_tab *)sdp->opt_tab;

			/* 
			 * lets see if the mode select parameter length is valid
			 * We must build it build hand...........
			*/
			if( todp->opt_flags & MSEL_PLL_VAL){
			    sc->sz_modsel.pll = todp->msel_pll;
			    if( todp->opt_flags & MSEL_BLKDL_VAL){
				sc->sz_modsel.rdeclen = todp->msel_blkdl;
			    }
			    if( todp->opt_flags & MSEL_VUL_VAL){
				sc->sz_modsel.vulen = todp->msel_vul;
			    }
			/* end of if MSEL_PLL_VAL */
			 }
			 else{
			    /* 
			     * no sense in testing anything else
			     * default to hdr and block ( 12 )
			    */
			    sc->sz_modsel.pll = 12;
			    sc->sz_modsel.rdeclen = 8;
			    /* ERRLOG_MARK */
			    cprintf("scsi OPTTABLE off id = %X\n", targid);
			}
		        /* 
			 * get our density struct pointer...
			*/
			ddp = &todp->tape_info[DENS_IDX( bp )];

			/* 
			 * lets see if this density selection is valid..
			*/
			if( ddp->tape_flags & DENS_VAL ){
			    /*
		 	     * this is valid lets get our stuff
			    */
			    sc->sz_modsel.density = 
					(char)(SCSI_DENS_MASK & ddp->dens);
				
			    /*
			     * must set our block size......
			    */
			    sc->sz_modsel.reclen2 = LTOB(ddp->blk_size,2);
			    sc->sz_modsel.reclen1 = LTOB(ddp->blk_size,1);
			    sc->sz_modsel.reclen0 = LTOB(ddp->blk_size,0);
			}
			/*
			 * well we have a option table but this density
			 * struct is not valid.... We must do something
			 * but is it QIC 9trk 8mm etc... 
			*/
			else {
			    /* ERRLOG_MARK */
			    mprintf("scsi OPTTABLE off id = %X\n", targid);

			    /* default it */
			    sc->sz_modsel.density = 0;
			    /*
			     * since we zeroed the command struct before
			     * we got here we don't have to set zero's
			     * in the block size field
			    */

			}
			/* 
			 * check speed setting 
			*/
			if( ddp->tape_flags & SPEED_VAL){
			    sc->sz_modsel.speed = 
					(SCSI_SPEED_MASK & ddp->tape_speed);
			} 
			/*
			 * is this scsi 2 if so must set pf.
			*/
			if( sdp->flags & SCSI_MODSEL_PF ){
			    sc->sz_modsel.pf = 1;
			}
			
			/*
			 * end of if option table for tapes
			*/
		    }
		    /*
		     * For the old stuff backward compat
		    */
		    else {
			sc->sz_modsel.pll = 0x0e;
			sc->sz_modsel.bufmode = 0;
			sc->sz_modsel.rdeclen = 0x08;
			sc->sz_modsel.vulen = 1;
			sc->sz_modsel.nof = 0;
		    }
		    break;
		case SZ_TRKSEL:
		    return(SZ_RET_ERR);
		    break;
		case SZ_RESUNIT:
		case SZ_RELUNIT:
		    PRINTD(targid, 0x10,
			("sz_bldpkt: unimplemented command 0x%x\n", cmd));
		    return (SZ_RET_ERR);
		    break;
				
		case SZ_P_EJECT:
		    cmd = SZ_SSLU;
		    sc->sz_ssu.start = 0;	/* Stop the unit.	*/
		    sc->sz_ssu.loej = 1;	/* Eject the caddy.	*/
		    sc->sz_ssu.immed = 1;	/* Complete immediatly.	*/
		    break;

		case SZ_MEDREMOVAL:
		    sc->sz_mr.prevent = count;
		    break;

		case SZ_PAUSE_RESUME:
		    sc->sz_cd_pr.resume = count;
		    break;

		case SZ_PLAY_AUDIO:
		case SZ_PLAY_VAUDIO: {
		    struct cd_play_audio *pa;

		    pa = (struct cd_play_audio *) sc->sc_rzparams[targid];
		    sc->sz_cd_pa.lbaddr3 = LTOB(pa->pa_lba, 3);
		    sc->sz_cd_pa.lbaddr2 = LTOB(pa->pa_lba, 2);
		    sc->sz_cd_pa.lbaddr1 = LTOB(pa->pa_lba, 1);
		    sc->sz_cd_pa.lbaddr0 = LTOB(pa->pa_lba, 0);
		    sc->sz_cd_pa.xferlen1 = LTOB(pa->pa_length, 1);
		    sc->sz_cd_pa.xferlen0 = LTOB(pa->pa_length, 0);
		    break;
		}
		case SZ_PLAY_MSF:
		case SZ_PLAY_AUDIO_MSF: {
		    struct cd_play_audio_msf *msf;

		    msf = (struct cd_play_audio_msf *) sc->sc_rzparams[targid];
		    sc->sz_cd_msf.starting_M_unit = msf->msf_starting_M_unit;
		    sc->sz_cd_msf.starting_S_unit = msf->msf_starting_S_unit;
		    sc->sz_cd_msf.starting_F_unit = msf->msf_starting_F_unit;
		    sc->sz_cd_msf.ending_M_unit = msf->msf_ending_M_unit;
		    sc->sz_cd_msf.ending_S_unit = msf->msf_ending_S_unit;
		    sc->sz_cd_msf.ending_F_unit = msf->msf_ending_F_unit;
		    break;
		}
		case SZ_PLAY_AUDIO_TI: {
		    struct cd_play_audio_ti *ti;

		    ti = (struct cd_play_audio_ti *) sc->sc_rzparams[targid];
		    sc->sz_cd_ti.starting_track = ti->ti_starting_track;
		    sc->sz_cd_ti.starting_index = ti->ti_starting_index;
		    sc->sz_cd_ti.ending_track = ti->ti_ending_track;
		    sc->sz_cd_ti.ending_index = ti->ti_ending_index;
		    break;
		}
		case SZ_PLAY_TRACK_REL: {
		    struct cd_play_audio_tr *tr;

		    tr = (struct cd_play_audio_tr *) sc->sc_rzparams[targid];
		    sc->sz_cd_tr.lbaddr3 = LTOB(tr->tr_lba, 3);
		    sc->sz_cd_tr.lbaddr2 = LTOB(tr->tr_lba, 2);
		    sc->sz_cd_tr.lbaddr1 = LTOB(tr->tr_lba, 1);
		    sc->sz_cd_tr.lbaddr0 = LTOB(tr->tr_lba, 0);
		    sc->sz_cd_tr.starting_track = tr->tr_starting_track;
 		    sc->sz_cd_tr.xfer_len1 = LTOB(tr->tr_xfer_length,1);
		    sc->sz_cd_tr.xfer_len0 = LTOB(tr->tr_xfer_length,0);
		    break;
		}
		case SZ_PLAY_TRACK: {
		    struct cd_play_track *pt;

		    pt = (struct cd_play_track *) sc->sc_rzparams[targid];
		    sc->sz_cd_pt.starting_track = pt->pt_starting_track;
		    sc->sz_cd_pt.starting_index = pt->pt_starting_index;
		    sc->sz_cd_pt.number_indexes = pt->pt_number_indexes;
		    break;
		}
		case SZ_READ_TOC: {
		    register struct cd_toc *toc;

		    toc = (struct cd_toc *)sc->sc_rzparams[targid];
		    if (toc->toc_address_format == CDROM_MSF_FORMAT) {
			sc->sz_cd_toc.msf = 1;
		    }
		    sc->sz_cd_toc.starting_track = toc->toc_starting_track;
		    sc->sz_cd_toc.alloc_len1 = LTOB(toc->toc_alloc_length,1);
		    sc->sz_cd_toc.alloc_len0 = LTOB(toc->toc_alloc_length,0);
		    break;
		}
		case SZ_READ_SUBCHAN: {
		    register struct cd_sub_channel *sch;

		    sch = (struct cd_sub_channel *)sc->sc_rzparams[targid];
		    if (sch->sch_address_format == CDROM_MSF_FORMAT) {
			sc->sz_cd_sch.msf = 1;
		    }
		    sc->sz_cd_sch.subQ = 1;
		    sc->sz_cd_sch.data_format = sch->sch_data_format;
		    sc->sz_cd_sch.track_number = sch->sch_track_number;
		    sc->sz_cd_sch.alloc_len1 = LTOB(sch->sch_alloc_length,1);
		    sc->sz_cd_sch.alloc_len0 = LTOB(sch->sch_alloc_length,0);
		    break;
		}
		case SZ_READ_HEADER: {
		    register struct cd_read_header *rh;

		    rh = (struct cd_read_header *)sc->sc_rzparams[targid];
		    if (rh->rh_address_format == CDROM_MSF_FORMAT) {
			sc->sz_cd_rh.msf = 1;
		    }
		    sc->sz_cd_rh.lbaddr3 = LTOB(rh->rh_lba, 3);
		    sc->sz_cd_rh.lbaddr2 = LTOB(rh->rh_lba, 2);
		    sc->sz_cd_rh.lbaddr1 = LTOB(rh->rh_lba, 1);
		    sc->sz_cd_rh.lbaddr0 = LTOB(rh->rh_lba, 0);
		    sc->sz_cd_rh.alloc_len1 = LTOB(rh->rh_alloc_length,1);
		    sc->sz_cd_rh.alloc_len0 = LTOB(rh->rh_alloc_length,0);
		    break;
		}
		case SZ_PLAYBACK_CONTROL:
		case SZ_PLAYBACK_STATUS: {
		    register struct cd_playback *pb;

		    pb = (struct cd_playback *)sc->sc_rzparams[targid];
		    sc->sz_cd_pb.alloc_len1 = LTOB(pb->pb_alloc_length,1);
		    sc->sz_cd_pb.alloc_len0 = LTOB(pb->pb_alloc_length,0);
		    break;
		}
		case SZ_SET_ADDRESS_FORMAT:
		    sc->sz_cd_saf.lbamsf = count;
		    break;

		case SZ_SEEK_10:
		    sc->sz_d_seek.lbaddr3 = LTOB(count, 3);
		    sc->sz_d_seek.lbaddr2 = LTOB(count, 2);
		    sc->sz_d_seek.lbaddr1 = LTOB(count, 1);
		    sc->sz_d_seek.lbaddr0 = LTOB(count, 0);
		    break;

		default:
		    PRINTD(targid, 0x10,
			("sz_bldpkt: unknown command = 0x%x\n", cmd));
		    return (SZ_RET_ERR);
		    break;
	}
	/*
	 * TODO1:
	 * We assume each unit is a single SCSI target device, i.e.,
	 * no sub-units. So logical unit is always zero.
	 * 
	 * sz_read is used here to get to the fields in the 
	 * structure for all commands.
	 * TODO1: need better handling of 10 byte commands
	 */
	len = sz_cdb_length (cmd, targid);
	if (len == 6) {
	    sc->sz_t_read.lun = 0;
	    sc->sz_t_read.link = 0;
	    sc->sz_t_read.flag = 0;
	    sc->sz_t_read.mbz = 0;
	}
	sc->sz_opcode = cmd;
	/*
	 * Save the current command bytes for the error log.
	 * Unless we are doing a request sense after a failed command.
	 */
	byteptr = (u_char *)&sc->sz_command;
	if (sc->sc_curcmd[targid] == sc->sc_actcmd[targid]) {
	    for (i = 0; i < len; i++)
		sc->sc_cmdlog[targid][i] = *byteptr++;
	    for (; i < 12; i++)
		sc->sc_cmdlog[targid][i] = 0;
	}
	return(SZ_SUCCESS);
}

int	sz_s_cnt1 = 0;
int	sz_s_cnt2 = 0;

int	sz_szf_print = 0;
int	sz_sp_szflags = 0;
int	sz_rw_szflags = 0;

/******************************************************************
 *
 * Name:	sz_start
 *
 * Abstract:	Start or continue a transaction.  The "infamous" SCSI
 *		state machine....
 *
 * Inputs:
 * 	sc	Pointer to softc structure for controller.
 *	targid  Target.
 *
 * Outputs:	None.
 *
 * Return values: None.
 *
 ******************************************************************/

sz_start(sc, targid)
register struct sz_softc *sc;
int targid;
{
#ifdef vax
    int cntlr = sc - &sz_softc[0];
    register struct sz_regs *szaddr = (struct sz_regs *)szmem + cntlr;
    register struct nb_regs *sziaddr = (struct nb_regs *)nexus;
#endif vax
#ifdef mips
    int cntlr = sc->sc_siinum;
#endif mips
    register struct buf *bp;
    register struct buf *dp;
    volatile char *stv;		/* virtual address of st page tables	      */
    struct pte *pte, *mpte;	/* used for mapping page table entries	      */
    char *bufp;
    unsigned v;
    int o, npf;
    struct proc *rp;
    int ssrv;
    int i;
    int bcount;
    short count;
    u_char *byteptr;
#ifdef vax
    struct uba_ctlr *um = szminfo[cntlr];
    struct uba_device *ui;
#endif vax
    int unit;
    int part;
    daddr_t blkno;
    daddr_t badblk;
    struct scsi_devtab *sdp;
    struct tape_opt_tab *todp;
    struct tape_info *ddp;
    int	rqsns_rtn;		/* request sense return code from port_start */
    int szrtn;			/* rtn code from szerror */
	
    if (targid != -1) {		/* we know the target ID */
	/*
	 * If the target ID is not minus one, just enter
	 * the next state/event for that target.
	 */
	unit = sc->sc_unit[targid];
	dp = &szutab[unit];
	bp = dp->b_actf;
    }
    else {
	/*
	 * If the target ID is minus one, we were called from
	 * szintr() because the currently active target
	 * disconnected. Start I/O on the next target
	 * with a request pending, but not already active.
	 */
	targid = sc->sc_lastid;
	while (1) {
	    targid++;
	    if (targid >= NDPS)
		targid = 0;
	    if ((1 << targid) == sc->sc_sysid)
		continue;		/* skip initiator */
	    if (targid == sc->sc_lastid)
		return;			/* no target ready to start I/O */
	    /* TODO: need more checking (could be unknown target) */
	    if (sc->sc_alive[targid] == 0)
		continue;		/* non existent target */

	    if (sc->sc_attached[targid] == 0) {
		continue;		/* non attached target */
		}

	    unit = sc->sc_unit[targid];
	    dp = (struct buf *)&szutab[unit];

	    if (dp->b_active)
		continue;		/* target already active */

	    if (dp->b_actf == NULL)
		continue;		/* no request pending on this target */

	     /* found one, start it */
	    bp = dp->b_actf;
	    break;
	}
    }

    /*
     * The sz_start routine is a state machine for each
     * target device on each bus.
     * The states are kept in the sz_softc structure.
     * The state variables used in this state machine are:
     *
     *	sc->sc_xstate	used to dispatch to major states
     *	sc->sc_xevent	used to dispatch to minor (sub)states
     */
    for (;;) {	/* forever */

    PRINTD(targid, 0x8, ("target: %d state=%x, event=%x, flags=%x, unit=%x\n",
		targid, sc->sc_xstate[targid], sc->sc_xevent[targid],
			sc->sc_szflags[targid], unit));

    PRINTD(targid, 0x8000, ("t: %d st=%x, evt=%x, flgs=%x, unit=%x\n",
		targid, sc->sc_xstate[targid], sc->sc_xevent[targid],
			sc->sc_szflags[targid], unit));

    switch(sc->sc_xstate[targid]) {

	/*------------------------------*/
	/*				*/
	/*       S Z   N E X T		*/
	/*				*/
	/*------------------------------*/

	case SZ_NEXT:

	    switch(sc->sc_xevent[targid]) {

		case SZ_CONT:

		  /* Call the particular device completion routine. */

		      bp = dp->b_actf;

		      (*sc->device_comp[targid])(bp);

		    /*
		     * Find the next target with an I/O reuest pending
		     * and is not already busy, and start I/O on it.
		     */

		    if (sc->sc_szflags[targid] & SZ_BUSYBUS)
			return;	/* TODO: bus busy don't start next command */

		    targid = sc->sc_lastid;

		    while (1) {
			targid++;

			if (targid >= NDPS)
			    targid = 0;

			if ((1 << targid) == sc->sc_sysid)
			    continue;		/* skip initiator */

			/* TODO: need more checking, see comment above */
			if (sc->sc_alive[targid] == 0)
			    continue;		/* non existent target */

			if (sc->sc_attached[targid] == 0) {
			    continue;		/* non attached target */
			}

			unit = sc->sc_unit[targid];
			dp = (struct buf *)&szutab[unit];

			if (dp->b_actf == NULL) {
			    dp->b_active = 0;
			    if (targid == sc->sc_lastid) {
				/* TODO1: old code, selena, sztab.b_active */
				return;
			    }
			    else {
				continue;
			    }
			}
			else {
			    if (dp->b_active) {
				if (targid == sc->sc_lastid)
				    return;
				else
				    continue;
			    }
			}

			bp = dp->b_actf;	/* MUST set buffer pointer */
			dp->b_active = 1;	/* TODO1: also set in LATER */
			break;
		    }

		    continue;		/* takes us to SZ_BEGIN or SZ_RW_CONT */

		    break;		/* NOTREACHED */

		case SZ_BEGIN:

		    if ((bp = dp->b_actf) == NULL) {
			dp->b_active = 0;
			return;
		    }

		    sc->sc_szflags[targid] &= ~SZ_BUSYBUS;

		    if((sc->sc_flags[targid] & DEV_EOM) &&
			!((sc->sc_flags[targid] & DEV_CSE) ||
			(dis_eot_sz[unit] & DISEOT)))
		    {
			bp->b_resid = bp->b_bcount;
			bp->b_error = ENOSPC;
			bp->b_flags |= B_ERROR;
			sc->sc_xevent[targid] = SZ_CONT;
			break;
		    }
		    /* 
		     * For nbufio... If we are not at eot and DEV_CSE
		     * is set then clear the flag. In other words the
		     * dev_cse bit was set in response to a file mark
		     * detected during nbufio...The reasoning for the
		     * clear of the flag here is because of the previous
		     * check and all operations begin life at this
		     * state. We must notice the flag with the previous
		     * if because the user have set the flag to get
		     * past eot. This is allowed, else if not at 
		     * eot the flag was set for nbufio and if we
		     * what to notice eot we have to clear the flag.
		    */
		    if( !(sc->sc_flags[targid] & DEV_EOM) && 
				(sc->sc_flags[targid] & DEV_CSE)) {
			sc->sc_flags[targid] &= ~DEV_CSE;
		    }

		    if((bp->b_flags & B_READ) && (bp->b_flags & B_RAWASYNC) && 
			((sc->sc_category_flags[targid]&DEV_TPMARK) ||
			(sc->sc_flags[targid]&DEV_HARDERR))) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			sc->sc_xevent[targid] = SZ_CONT;
			break;
		    }
		    
		    /* 
		     * For fix block tape units check to see if tape mark
		     * is pending.... IF so this is the next read and post
		     * TPMARK AND clear the pending flag...
		    */
		    if((bp->b_flags & B_READ) && (bp->b_flags & B_RAWASYNC) && 
			     (sc->sc_category_flags[targid] & TPMARK_PENDING)){

			sc->sc_resid[targid] = bp->b_bcount;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			sc->sc_category_flags[targid] |= DEV_TPMARK;
			sc->sc_category_flags[targid] &= ~TPMARK_PENDING;

			break;
		    }

		    /* ok we have gotten to this point.... If 
		     * we had nbuf io and a tpmark then we can't
		     * get to here..... This is an operation whether
		     * control or read/write it does not matter we 
		     * must clear out the tpmark indicator...
		     * Ie straight reads will report the tpmark and
		     * the next operation should get the next record..
		     * tpmark cleared... for nbuf io we can't get here
		     * and the only way to clear it is with MTCSE ioctl.
		     * see mtio(4). Now there are side effects to this
		     * that mimicks the tmscp class driver.. on a tmscp
		     * class device if a control operation/write operation
		     * issused to the device the tpmark indicator is cleared
		     * and the operation is declared a success so if we
		     * clear DEV_TPMARK here we are golden...Give the man
		     * a beer.
		    */

		    sc->sc_category_flags[targid] &= ~DEV_TPMARK;
		    

		    /* 
		     * clear out the short record indicator left over
		     * from last operation...........
		     * This is done for reads/writes control and
		     * nbuf i/o. Reasoning is that we report the short 
		     * record status for the previous command. It
		     * it is up to the programmer to get status if they
		     * want it. If not any new operation must start
		     * with a clean slate so clear short rec indicator.
		    */

		     sc->sc_category_flags[targid] &= ~DEV_SHRTREC;

		    /* log progress */
		    sc->sc_progress = time;

		  /* Check to see what path is required for this command. */

		    if((bp == &cszbuf[unit]) ||
			((sc->sc_bbr_active[targid] == 1) &&
			(sc->sc_bbr_oper[targid] == SZ_SP_START)))
		    {
			/* 
			 * execute control operation with the specified count
			 */
			if (bp->b_comand == SZ_REWIND) {
			    /* TODO: appears not used, remove later? */
			    /*    um->um_tab.b_active = SREW;	*/
			    sc->sc_flags[targid] &= ~DEV_EOM;
			}
			else {
			    /* TODO: appears not used, remove later? */
			    /*    um->um_tab.b_active = SCOM;	*/
			}
	    	    	sc->sc_xstate[targid] = SZ_SP_START;
			sc->sc_xevent[targid] = SZ_CONT;
	    	    	break;
		    }
		    /*
		     * If it's not a control operation, it must be
		     * data.
		     */
		    sc->sc_xstate[targid] = SZ_RW_START;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		default:
			;

	    }		/* end switch(xevent) */

	break;		/* break from switch(xstate), case SZ_NEXT */

	  /*----------------------------*/
	  /*				*/
	  /*       S P   S T A R T      */
	  /*				*/
	  /*----------------------------*/

	/*
	 * Start a non data transfer command.
	 * TODO: insert fluff here.
	 * TODO: use sc_bp[] to save bp for safety?
	 */

	case SZ_SP_START:


	    /* remember where we are in case target returns busy status */

	    sc->sc_pxstate[targid] = SZ_SP_START;

	    /*
	     * Forgive the gotos, but.....
	     * The SELRETRY? flags tell us we must restart processing
	     * at the point where we call scsistart, because either
	     * a select timed out or completed during the timeout.
	     */

	    if (sc->sc_xevent[targid] == SZ_SELRETRY_CMD) {
		PRINTD(targid, 0x8000, 
	         ("SP_START: t=%x, evt = SELRETRY_CMD, going to sz_sp_cmd_err\n",
		      targid));
		goto sz_sp_cmd_err;
	    }
	    else if (sc->sc_xevent[targid] == SZ_SELRETRY_SNS) {
		PRINTD(targid, 0x8000, 
		 ("SP_START: t=%x, evt = SELRETRY_SNS, going to sz_sp_sns_err\n",
		      targid));
		goto sz_sp_sns_err;
	    }

	    /*
	     * Clear DEV_WRITTEN flag only for commands
	     * which would actually change tape position
	     * from the last (possible) write.
	     * TODO: deal with SZ_ERASE, SZ_UNLOAD, SZ_VFY
	     */

	    if (sc->sc_devtyp[targid] & SZ_TAPE) {

		switch (bp->b_comand) {

		    case SZ_REWIND:
		    case SZ_WFM:
		    case SZ_P_BSPACEF:
		    case SZ_P_FSPACEF:
		    case SZ_P_BSPACER:
		    case SZ_P_FSPACER:
		    case SZ_P_RETENSION:
			sc->sc_flags[targid] &= ~DEV_WRITTEN;
		    default:
			break;
		}	/* end switch b_comand */

	    }	/* end if devtype & TAPE */

	    /*
	    * The write to b_resid has to follow the
	    * read of b_command. This is because both
	    * b_command and b_resid are the same field
	    * (overloaded).  The write to b_resid destroys
	    * the data in b_command.  This is a black
	    * hole waiting to be fallen into!
	    *
	    * NOTE:
	    *
	    * We fell into the black hole described above!
	    * This code can be executed more than once for
	    * a command. This happens if the command has to
	    * be restarted after waiting for the bus to free up.
	    * To avoid the black hole, the read of b_command
	    * is only done the first time thru this code path.
	    *
	    * NOTE:
	    *
	    * The above bug is fixed. The command is now called
	    * b_comand and is stored in the b_gid buffer field.
	    */

	    if (sc->sc_selstat[targid] != SZ_BBWAIT) {
		sc->sc_bp[targid] = bp;
	    }

	    sc->sc_curcmd[targid] = bp->b_comand;
	    dp->b_active = 1;
	    sc->sc_lastid = targid;
	    bp->b_resid = 0;
	    sc->sc_resid[targid] = 0;	/* makes debug output look cleaner */
	    sc->sc_c_status[targid] = SZ_GOOD;

	    /* TODO2: debug - check for left over bits in szflags */

	    sz_sp_szflags = sc->sc_szflags[targid];

	    sc->sc_szflags[targid] = SZ_NORMAL;

	    sz_bldpkt(sc, targid, sc->sc_curcmd[targid], 0, bp->b_bcount);

sz_sp_cmd_err:

	    ssrv = (*sc->port_start)(sc, targid, bp);

	    PRINTD(targid, 0x8000, ("SP_START: t = %x, rtn fr port_start = %x\n",
				targid, ssrv));

	    /* TODO: lo probability, but must worry about BUSBUSY! */

	    switch (ssrv) {

		case SZ_BUSBUSY:	/* must wait for SCSI bus to free up */
		    sc->sc_selstat[targid] = SZ_BBWAIT;
		    dp->b_active = 0;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_BEGIN;
		    return;
		    break;	/* NOTREACHED */

		case SZ_IP:		/* In Progress (command disconnected) */
		    sc->sc_xstate[targid] = SZ_SP_CONT;
		    return;
		    break;	/* NOTREACHED */

		case SZ_SUCCESS:	/* Command completed ok in scsistart */
		    sc->sc_c_status[targid] = SZ_GOOD;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		case SZ_RET_ABORT:
			/* FALLTHROUGH */
		default:
		    sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;
		    sc->sc_c_status[targid] = SZ_BAD;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		case SZ_RET_ERR:
		    sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;
		/* 
		 * If command is request sense, just return
		 * a fatal error if it fails, i.e.,
		 * don't do RQSNS on RQSNS.
		 */

		if (sc->sc_curcmd[targid] == SZ_RQSNS) {
			sc->sc_c_status[targid] = SZ_BAD;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;
		}	/* end if curcmd == RQSNS */

		/*
		 * For all other commands,
		 * do a request sense and act on the sense data.
		 */

		PRINTD(targid, 0x8000, 
		     ("SP_START: calling save_rqsns for t = %x\n", targid));

		sz_save_rqsns(sc, targid);

		sz_bldpkt(sc, targid, SZ_RQSNS, 0, 0);

		/*
		 * TODO:
		 *	Fix "if (!sz_" in stc driver.
		 *	What to do if RQSNS fails?
		 *	Here and all other places.
		 */

sz_sp_sns_err:
		rqsns_rtn = (*sc->port_start)(sc, targid, bp);

		PRINTD(targid, 0x8000, 
		   ("SP_START: t = %x, rtn fr RQSNS port_start = %x\n", 
		      targid, rqsns_rtn));

		switch(rqsns_rtn) {

		  case SZ_SUCCESS:
			sc->sc_xstate[targid] = SZ_SP_START_ERR;
		        break;

		  case SZ_IP:	       
			sc->sc_xstate[targid] = SZ_SP_START_ERR;
			return;
			break;		/*NOTREACHED*/

		  default:

			PRINTD(targid, 0x8000,
		   ("SP_START: unknown rc from RQSNS port_start = %x, t = %x\n",
			    rqsns_rtn, targid));

			sz_restore_rqsns(sc, targid);

		        sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;

			sc->sc_c_status[targid] = SZ_BAD;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;

		}	/* end switch(rqsns_rtn) */

		    break;

		}	/* end switch(port_start()) */

	break;		/* break from  switch(xstate), case SZ_SP_START	*/

	/*------------------------------------------------------*/
	/*							*/
	/*            S P   S T A R T   E R R 			*/
	/*							*/
	/*	Only comes here when we know we have good 	*/
	/*	sense info.					*/
	/*------------------------------------------------------*/

	case SZ_SP_START_ERR:

	        PRINTD(targid, 0x8000, 
		   ("SP_START_ERR: t =%x, call restore_rqsns\n", targid));

	        sz_restore_rqsns(sc, targid);

		sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;

		if(sc->sc_rzspecial[targid]) {
		    bcopy((char *)&sc->sc_sns[targid],
				(char *)&sc->sc_rzsns[targid], 
				sizeof(sc->sc_sns[targid]));
		    if(sc->sc_sns[targid].snskey == SZ_NOSENSE){ 
			sc->sc_c_status[targid] = SZ_GOOD;
		    }
		    else{
			sc->sc_c_status[targid] = SZ_BAD;
		    }
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;
		    }

		if (bp->b_retry == 0) {
		    sc->sc_c_status[targid] = SZ_CHKCND;
		    sc->sc_c_snskey[targid] = sc->sc_sns[targid].snskey;

		if (sc->sc_devtyp[targid] & SZ_DISK) {
		    sc->sc_c_asc[targid] = 
			sc->sc_sns[targid].asb.rz_asb.asc;
		}
		else if (sc->sc_devtyp[targid] & SZ_CDROM) {
			sc->sc_c_asc[targid] = 
				sc->sc_sns[targid].asb.cd_asb.asc;
		}
		else
		    sc->sc_c_asc[targid] = 0xff;

		if (sc->sc_c_snskey[targid] == SZ_UNITATTEN) {
		    sz_unit_online[unit] = 0;

		    /* media changed (floppy softpc hooks) */

		    if (sc->sc_c_asc[targid] == 0x28)
			sc->sc_mc_cnt[targid]++;
		}

		/* log error for certain tape commands -- kludge! */

		if (sc->sc_devtyp[targid] & SZ_TAPE) {

		    switch(sc->sc_curcmd[targid]) {

		    case SZ_WFM:
		    case SZ_P_FSPACER:
		    case SZ_P_FSPACEF:
		    case SZ_P_BSPACER:
		    case SZ_P_BSPACEF:
			i = sc->sc_flags[targid];
			sz_retries[unit] = SZ_SP_RTCNT;	/* harderr */

			szerror(sc, targid);		/* log error */

			sc->sc_flags[targid] = i;	/* clean up */
			break;

		    default:
			break;

		    }	/* end switch(curcmd) */

		}	/* end if devtype & TAPE */

		sc->sc_xstate[targid] = SZ_NEXT;
		sc->sc_xevent[targid] = SZ_CONT;
		break;

		}	/* end if b_retry == 0 */
	    else {
		szrtn = szerror(sc, targid);

		switch (szrtn) {

		    case SZ_SUCCESS:
			sc->sc_c_status[targid] = SZ_GOOD;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;

		    case SZ_RETRY:
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;

			if (sz_retries[unit] < SZ_SP_RTCNT) {
			    sc->sc_xevent[targid] = SZ_BEGIN;
			    sz_retries[unit]++;
			}
			else {
			    sc->sc_c_status[targid] = SZ_BAD;
			}
			break;

		    case SZ_FATAL:
	    		if ((sc->sc_flags[targid] & DEV_EOM) &&
			    !((sc->sc_flags[targid] & DEV_CSE) ||
			    (dis_eot_sz[unit] & DISEOT))) {
			         bp->b_error = ENOSPC;
			}
			/* FALLTHROUGH */

		    default:
			sc->sc_c_status[targid] = SZ_BAD;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;

		    }	/* end switch(szerror()) */

		/*
		 * At this point the old code would go to
		 * SZ_CONT if the command was RQSNS or SPACE.
		 * It makes sense not to retry RQSNS/SPACE, but
		 * we now allow the caller to controll retries.
		 */
		break;

	    }	/* end else */

	    break;		/* end case: SP_START_ERR state */
		
	/*------------------------------------------------------*/
	/*							*/
	/*            	 S Z   S P   C O N T			*/
	/*							*/
	/*------------------------------------------------------*/

	/*
	 * Continue a non data transfer command
	 * after a disconnect.
	 * TODO: insert fluff here.
	 * If necessary, do a request sense and look at the
	 * sense data to determine what to do next.
	 * Caller specifies whether we retry command or not.
	 */

	case SZ_SP_CONT:

	    /* remember where we are in case target returns busy status */

	    sc->sc_pxstate[targid] = SZ_SP_CONT;

	    if (sc->sc_xevent[targid] == SZ_SELRETRY_SNS) {
		PRINTD(targid, 0x8000,
		  ("SP_CONT: t=%x, evt = SELRETRY_SNS, going to sz_sp_cont_err\n",
		       targid));
		goto sz_sp_cont_err;
	    }

	    /*
	     * Old code went to SZ_CONT if command was RQSNS.
	     * RQSNS can't get here because it doesn't disconnect.
	     * If this command is a SZ_RQSNS, look at the
	     * status bytes and determine what to do next,
	     * otherwise process as ususal.
	     */

	    if (sc->sc_szflags[targid] & SZ_NEED_SENSE) {

		PRINTD(targid, 0x8000, ("SP_CONT: call save_rqsns for t=%x\n",
					targid));

		sz_save_rqsns(sc, targid);

		sz_bldpkt(sc, targid, SZ_RQSNS, 0, 0);

sz_sp_cont_err:	
		ssrv = (*sc->port_start)(sc, targid, bp);

		PRINTD(targid, 0x8000,
		   ("SP_CONT: t = %x, rtn fr RQSNS port_start = %x\n", 
		        targid, ssrv));

		switch(ssrv) {

		  case SZ_SUCCESS:
		    sc->sc_xstate[targid] = SZ_SP_START_ERR;
		    break;

		  case SZ_IP:	     
		    sc->sc_xstate[targid] = SZ_SP_START_ERR;
		    return;
		    break;		/*NOTREACHED*/

                  case SZ_BUSBUSY:

		    sc->sc_selstat[targid] = SZ_BBWAIT;
		    dp->b_active = 0;

		    /* xstate ok, will send us back to SZ_SP_CONT: */
		    if (rdatdebug)
			cprintf("SP_CONT: BUSBUSY, xevent = SELRETRY_SNS\n");

		    sc->sc_xevent[targid] = SZ_SELRETRY_SNS;

		    return;
	            break;		/*NOTREACHED*/

		  default:

			PRINTD(targid, 0x8000,
			   ("SP_CONT: unknown rc from RQSNS port_start = %x\n",
			    ssrv));

			sz_restore_rqsns(sc, targid);

	                sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;
			sc->sc_c_status[targid] = SZ_BAD;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;

		}	/* end switch(ssrv) */

	    }		/* end if NEED_SENSE */
	    else {
		if (sc->sc_szflags[targid] & SZ_ENCR_ERR)
		    sc->sc_c_status[targid] = SZ_BAD;

	    	sc->sc_xstate[targid] = SZ_NEXT;
	    	sc->sc_xevent[targid] = SZ_CONT;

		break;

	    }	/* end else (!NEED_SENSE)       */

	    break;		/* end case SP_CONT */

	/*------------------------------------------------------*/
	/*							*/
	/*             S Z   S P   C O N T   E R R		*/
	/*							*/
	/*------------------------------------------------------*/

	  case SZ_SP_CONT_ERR:

	        PRINTD(targid, 0x8000, 
		   ("SP_CONT_ERR: t =%x, call restore_rqsns\n", targid));

	        sz_restore_rqsns(sc, targid);

		sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;

		if(sc->sc_rzspecial[targid]) {
		    bcopy((char *)&sc->sc_sns[targid],
				(char *)&sc->sc_rzsns[targid], 
				sizeof(sc->sc_sns[targid]));
		    if(sc->sc_sns[targid].snskey == SZ_NOSENSE){ 
			sc->sc_c_status[targid] = SZ_GOOD;
		    }
		    else{
			sc->sc_c_status[targid] = SZ_BAD;
		    }
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;
		    }

		if (bp->b_retry == 0) {
		    sc->sc_c_status[targid] = SZ_CHKCND;
		    sc->sc_c_snskey[targid] = sc->sc_sns[targid].snskey;

		    if (sc->sc_devtyp[targid] & SZ_DISK) {
			sc->sc_c_asc[targid] = 
			    sc->sc_sns[targid].asb.rz_asb.asc;
		    }
		    else if (sc->sc_devtyp[targid] & SZ_CDROM) {
			sc->sc_c_asc[targid] = 	
			    sc->sc_sns[targid].asb.cd_asb.asc;
		    }
		    else
			sc->sc_c_asc[targid] = 0xff;

		if (sc->sc_c_snskey[targid] == SZ_UNITATTEN) {
		    sz_unit_online[unit] = 0;

		    /* media changed (floppy softpc hooks) */

		    if (sc->sc_c_asc[targid] == 0x28)
			sc->sc_mc_cnt[targid]++;
		}	/* end if snskey == UNITATN */

		/* log error for certain tape commands -- kludge! */

		if (sc->sc_devtyp[targid] & SZ_TAPE) {

		    switch (sc->sc_curcmd[targid]) {

		    case SZ_WFM:
		    case SZ_P_FSPACER:
		    case SZ_P_FSPACEF:
		    case SZ_P_BSPACER:
		    case SZ_P_BSPACEF:

			i = sc->sc_flags[targid];

			sz_retries[unit] = SZ_SP_RTCNT;	/* harderr */

			szerror(sc, targid);		/* log error */

			sc->sc_flags[targid] = i;	/* clean up */
			break;

		    default:
			break;

		    }	/* end switch(curcmd) */

		}	/* end devtype & SZ_TAPE */

		sc->sc_xstate[targid] = SZ_NEXT;
		sc->sc_xevent[targid] = SZ_CONT;

		break;

	    }	/* end if b_retry == 0 */

	    else {
		szrtn = szerror(sc, targid);
		switch (szrtn) {

		case SZ_SUCCESS:
		    sc->sc_c_status[targid] = SZ_GOOD;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		case SZ_RETRY:
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;

		    if (sz_retries[unit] < SZ_SP_RTCNT) {
		    	sc->sc_xevent[targid] = SZ_BEGIN;
		    	sz_retries[unit]++;
		    }
		    else {
			sc->sc_c_status[targid] = SZ_BAD;
		    }
		    break;

		case SZ_FATAL:
		/* TODO: SP_START has code for DEV_EOM & DEV_CSE? */
		default:
		    sc->sc_c_status[targid] = SZ_BAD;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		}	/* end switch(szerror) */

		/*
		 * Old code went to SZ_CONT on RQSNS or SPACE.
		 * Should not retry RQSNS/SPACE, but we now
		 * allow caller to control retries.
		 */
		break;

	}	/* end else switch(szerror) */

	    break;	/* break from switch(xstate), case SZ_SP_CONT_ERR */

	/*------------------------------------------------------*/
	/*							*/
	/*               S Z   R W   S T A R T			*/
	/*							*/
	/*    Start a data transfer command (read/write).	*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_RW_START:

	    /* TODO2: debug - check for left over bits in szflags */

	    sz_rw_szflags = sc->sc_szflags[targid];
	    sc->sc_szflags[targid] = SZ_NORMAL;

	    /* 
	     * For fixed block units we must check to see if the
	     * byte count is a multiple of the block size.. for
	     * tape units only
	    */
	     if( sc->sc_devtyp[targid] & SZ_TAPE){
		if( sc->sc_devtab[targid]->opt_tab != NULL ){
		    /* 
		     * there is an table lets look at our block size
		    */
		    todp = (struct tape_opt_tab *)sc->sc_devtab[targid]->opt_tab;
		    /* 
		     * get our density struct pointer...
		    */
		    ddp = &todp->tape_info[DENS_IDX( bp )];

		    if(( ddp->tape_flags & DENS_VAL)  && 
				(ddp->blk_size != NULL)){
			/* 
			 * see if there is a remainder if
			 * there is blow the buffer away
			*/
			if( bp->b_bcount % ddp->blk_size){
			    bp->b_flags |= B_ERROR;
			    bp->b_error = EINVAL;
			    sc->sc_xstate[targid] = SZ_NEXT;
			    sc->sc_xevent[targid] = SZ_CONT;
			    break;
			}
		    }
		}
	    }

	    /*
	     * Map the users' page tables to my page tables (mpte),
	     * if the buffer is not in the buffer cache.
	     * This allows copying data between the 128K hardware
	     * data buffer and the users' buffer.
	     */
	    if (!(IS_KUSEG(bp->b_un.b_addr))) {
		bufp = (char *)bp->b_un.b_addr;
	    }
	    else {
		int user_addr = 0;
		/*
		 * Map to user space
		 */
		v = btop(bp->b_un.b_addr);
		o = (int)bp->b_un.b_addr & PGOFSET;
		npf = btoc(bp->b_bcount + o);
		if(npf <= 0)
			npf = 2;
		rp = (bp->b_flags & B_DIRTY) ? &proc[2] : bp->b_proc;
		if (bp->b_flags & B_UAREA) {
                    if(v)
                       	panic("szstart: v"); /* This should not ever happen */
		    pte = &rp->p_addr[v];
		}
		else if (bp->b_flags & B_PAGET) {
		    pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		}
		else if ((bp->b_flags & B_SMEM) && 
			((bp->b_flags & B_DIRTY) == 0)) {
		    pte = ((struct smem *)rp)->sm_ptaddr + v;	/* SHMEM */
		}
		else {
		    pte = (struct pte *)0;
		    user_addr++;
		}
		bufp = (char *)sc->sc_SZ_bufmap[targid] + o;
		mpte = (struct pte *)sc->sc_szbufmap[targid];
#ifdef mips
		/* Same mapping happens in nfs */
		for (i = 0; i < npf; i++, mpte++, pte++, v++) {
		    if (user_addr &&
			(((int)pte & PGOFSET) < CLSIZE*sizeof(struct pte)
			   || pte->pg_pfnum == 0))
				pte = vtopte(rp, v);
		    if (pte->pg_pfnum == 0)
				panic("scsi zero uentry");
		    *(int *)mpte = 0;
		    mpte->pg_pfnum = pte->pg_pfnum;
	    	}
		vmaccess(sc->sc_szbufmap[targid], bufp, npf, DO_CACHE);
#endif mips
	    }
	    sc->sc_bufp[targid] = bufp;

	    /*
	     * The bp->b_blkno always passed, but
	     * is only used by disks.
	     * TODO1: part table magic for disk blkno.
	     */
	    if (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) {
		part = minor(bp->b_dev) & 7;
		blkno = bp->b_blkno + sz_part[unit].pt_part[part].pi_blkoff;
	    }
	    else {
		blkno = bp->b_blkno;
	    }
	    sc->sc_blkno[targid] = blkno;

	    sc->sc_xfercnt[targid] = 0;

	    sc->sc_bp[targid] = bp;

	    sc->sc_xstate[targid] = SZ_RW_CONT;
	    sc->sc_xevent[targid] = SZ_CONT;
	    break;

	/*------------------------------------------------------*/
	/*							*/
	/*               S Z   R W   C O N T			*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_RW_CONT:

		bp = sc->sc_bp[targid];

	    /*
	     * See if transfer complete.
	     */

	    if (sc->sc_xfercnt[targid] >= bp->b_bcount) {
		sc->sc_xstate[targid] = SZ_NEXT;
		sc->sc_xevent[targid] = SZ_CONT;
		break;
	    }

	    /*
	     * Determine the block number and byte count
	     * for this segment of the transfer (which could
	     * be the entire transfer), then build the
	     * SCSI command packet.
	     *
	     * If the target device is a disk or cdrom,
	     * we may need to fiddle with the byte count
	     * to make it a multiple of 512 (lbn size).
	     * We talk 512 byte blocks to the disk, but only
	     * copy bp->b_bcount to/from the system.
	     * TODO: segment xfers > sc_segcnt bytes.
	     */

	    bcount = bp->b_bcount - sc->sc_xfercnt[targid];

	    if (bcount > sc->sc_segcnt[targid])
		bcount = sc->sc_segcnt[targid];

	    sc->sc_b_bcount[targid] = bcount;	/* TODO: usage? */

	    if (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) {
		sc->sc_bpcount[targid] = (((bcount + 511) / 512) * 512);
	    }
	    else {
		sc->sc_bpcount[targid] = bcount;
	    }

	    blkno = sc->sc_blkno[targid] + (sc->sc_xfercnt[targid] / 512);

	    if (bp->b_flags & B_READ) {
		int cmd = SZ_READ;

		if ( (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) &&
		     (blkno > SZ_MAX_LBA) ) {
		    cmd = SZ_READ_10;		/* Send a 10-byte CDB. */
		}
		sc->sc_curcmd[targid] = cmd;

		sz_bldpkt(sc, targid, cmd, blkno, sc->sc_bpcount[targid]);

		sc->sc_xstate[targid] = SZ_R_STDMA;
	    }
	    else {
		int cmd = SZ_WRITE;

		if ( (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) &&
		     (blkno > SZ_MAX_LBA) ) {
		    cmd = SZ_WRITE_10;		/* Send a 10-byte CDB. */
		}
		sc->sc_curcmd[targid] = cmd;

		sz_bldpkt(sc, targid, cmd, blkno, sc->sc_bpcount[targid]);

		sc->sc_xstate[targid] = SZ_W_STDMA;
	    }

	    dp->b_active = 1;
	    sc->sc_lastid = targid;

	    break;

	/*------------------------------------------------------*/
	/*							*/
	/*                 S Z   R   C O P Y			*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_R_COPY:

	    bp = sc->sc_bp[targid]; /* TODO1: */

	    /*
	     * Copy the data from the 128K buffer to memory
	     * (user space or kernel space).  Will have to
	     * set up own page table entries when copying to
	     * user space.
	     * TODO: page tables set up in RW_START now.
	     */

	    /*
	     * Map the user page tables to my page tables (mpte)
	     */
	    if (sc->sc_szflags[targid] & SZ_WAS_DISCON)
		sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;
	    /*
	     * Set up a pointer into the users' buffer.
	     */
	    bufp = sc->sc_bufp[targid];
	    bufp += sc->sc_xfercnt[targid];
/* TODO1: not sure this will work with partial records (resid != 0) */
	    /*
	     * Set up number of bytes to copy to users' buffer.
	     * For the last segment of the transfer, this could
	     * be less than the actual byte count read from the device.
	     * This is so we give the user the requested number of bytes.
	     */
/* TODO1
	    bcount = (sc->sc_bpcount[targid] - sc->sc_resid[targid]);
*/
	    bcount = sc->sc_bpcount[targid];
	    sc->sc_xfercnt[targid] += bcount;

	    if (sc->sc_xfercnt[targid] >= bp->b_bcount)
		sc->sc_resid[targid] = 0;
	    else
		sc->sc_resid[targid] = (bp->b_bcount - sc->sc_xfercnt[targid]);

	    sc->sc_xstate[targid] = SZ_RW_CONT;
	    sc->sc_xevent[targid] = SZ_CONT;

	    /* For UNRECOVERABLE data errors don't continue transfer */

	    if((sc->sc_devtyp[targid] & SZ_DISK) && 
				(bp->b_flags & B_ERROR)) {
	        sc->sc_xstate[targid] = SZ_NEXT;
	        sc->sc_xevent[targid] = SZ_CONT;
	    }

	    break;

	/*------------------------------------------------------*/
	/*							*/
	/*                 S Z   R   S T D M A			*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_R_STDMA:

	    /* remember where we are in case target returns busy status */

	    sc->sc_pxstate[targid] = SZ_R_STDMA;

	    bp = sc->sc_bp[targid];

	    /*
	     * Clear DEV_WRITTEN on read so we don't write
	     * filemarks (on close) in the wrong place.
	     */

	    if (sc->sc_devtyp[targid] & SZ_TAPE)
		sc->sc_flags[targid] &= ~DEV_WRITTEN;

	    /* TODO: why here? */
	    bp->b_resid = 0;

	    ssrv = (*sc->port_start)(sc, targid, bp);

	    PRINTD(targid, 0x8, ("SZ_R_STDMA return from port_start: %x\n",
		ssrv ));

	    switch( ssrv )
	    {
		case SZ_BUSBUSY:	/* must wait for SCSI bus to free up */
		    sc->sc_selstat[targid] = SZ_BBWAIT;
		    sc->sc_xstate[targid] = SZ_RW_CONT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    dp->b_active = 0;
		    return;
		    break;	/* NOTREACHED */

		/* TODO: verify this can't happen for data xfer command */
		case SZ_SUCCESS:
		    sc->sc_xstate[targid] = SZ_R_COPY;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		case SZ_IP:
		    sc->sc_xstate[targid] = SZ_R_DMA;
		    return;
		    break;	/* NOTREACHED */

		case SZ_RET_ERR:
		    sc->sc_xstate[targid] = SZ_R_DMA;
		    break;

		case SZ_RET_ABORT:
		default:
		    PRINTD(targid, 0x10,
                        ("SZ_R_STDMA return from port_start: SZ_RET_ABORT\n"));
		    bp->b_flags |= B_ERROR;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;
	    }
	    break;

	/*------------------------------------------------------*/
	/*							*/
	/*                 S Z   R   D M A			*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_R_DMA:

	    /* remember where we are in case target returns busy status */

	    sc->sc_pxstate[targid] = SZ_R_DMA;

	    bp = sc->sc_bp[targid];	/* TODO1 */

	    if (sc->sc_xevent[targid] == SZ_SELRETRY_SNS) {
		PRINTD(targid, 0x8000,
		   ("R_DMA: t=%x, evt = SELRETRY_SNS, going to sz_rdma_err\n",
		       targid));
		goto sz_rdma_err;
	    }

	    /*
	     * Determine if something went wrong that requires that
	     * a request sense command or a space command needs to be
	     * done here.  If a request sense needs to be done, send
	     * the command, and evaluate the result, and determine 
	     * if we need to return an error, return a short read,
	     * or retry the command.
	     */

	    if(sc->sc_szflags[targid] & SZ_NEED_SENSE) {
		/*
		 * TODO: 
		 * Determine what to do if the SZ_RQSNS fails
		 */

		sz_save_rqsns(sc, targid);

		sz_bldpkt(sc, targid, SZ_RQSNS, 0, 0);

sz_rdma_err:
		ssrv = (*sc->port_start)(sc, targid, bp);

		    PRINTD(targid, 0x8000,
			("R_DMA: t = %x, rtn fr RQSNS port_start = %x\n",
			   targid, ssrv));

		switch(ssrv) {

		    case SZ_BUSBUSY:

		    	sc->sc_selstat[targid] = SZ_BBWAIT;
			dp->b_active = 0;

			/* xstate ok as is, but set xevent... */

			if (rdatdebug)
			    cprintf("SP_CONT: BUSBUSY, xevent = SELRETRY_SNS\n");

			sc->sc_xevent[targid] = SZ_SELRETRY_SNS;

			return;
			break;		/*NOTREACHED*/

		      case SZ_SUCCESS:

			sc->sc_xstate[targid] = SZ_RDMA_ERR;
			break;

		      case SZ_IP:

			sc->sc_xstate[targid] = SZ_RDMA_ERR;
			return;
			break;		/*NOTREACHED*/

		      default:

			PRINTD(targid, 0x8000,
			  ("R_DMA: t = %x, UNKNOWN RTN CODE FROM port_start\n",
			       targid));

			sz_restore_rqsns(sc, targid);

			sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;
			bp->b_flags |= B_ERROR;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;
			
		}	/* end switch(ssrv) on RQSNS for R_DMA */

	    }		/* end if NEED_SENSE */
	    else {
		sc->sc_xstate[targid] = SZ_R_COPY;
		sc->sc_xevent[targid] = SZ_CONT;
	    }

    break;		/* end case SZ_R_DMA */

	/*------------------------------------------------------*/
	/*							*/
	/*              S Z   R   D M A   E R R			*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_RDMA_ERR:

	        PRINTD(targid, 0x8000, 
		   ("SP_RDMA_ERR: t =%x, call restore_rqsns\n", targid));

	    	    sz_restore_rqsns(sc, targid);

		    sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;

	            szrtn = szerror(sc, targid);

		    bp = sc->sc_bp[targid];
		    sdp = (struct scsi_devtab *)sc->sc_devtab[targid];

 PRINTD(targid, 0x8000, 
  ("RDMA_ERR: szrtn = %x, sc_szflags = %x, sc_flags = %x, sc_cat_flags = %x\n", 
	  szrtn, sc->sc_szflags[targid], sc->sc_flags[targid], 
	     sc->sc_category_flags[targid])); 

		    switch(szrtn) {

		    case SZ_SUCCESS:

			sc->sc_xevent[targid] = SZ_CONT;

 			if (sc->sc_category_flags[targid] & DEV_TPMARK) {	
			    sc->sc_resid[targid] = bp->b_bcount;
			    sc->sc_xstate[targid] = SZ_NEXT;

			  /* Clear the TPMARK flag, only applies to current io */
			 /*   sc->sc_flags[targid] &= ~DEV_TPMARK;  */
			}

			else if (sc->sc_category_flags[targid] & DEV_SHRTREC)
			{
			    /* 
			     * since we support fixed records the info
			     * contains a blk count not a byte count
			     * and we must take that into consideration.
			     * since it we only be set for fixed densities.
			    */
		 	    if(sdp->opt_tab != NULL){
			
				/* 
				 * get our pointer and use it to see
				 * if we are a fixed block unit.
				*/

				todp = (struct tape_opt_tab *)sdp->opt_tab;
				
				/*
				 * get our density struct for this setting
				*/
				ddp = &todp->tape_info[DENS_IDX( bp )];
				
				/* 
				 * lets see if this density selection is valid
				*/
				if( ddp->tape_flags & DENS_VAL){

				    /*
				     * Well we have a valid density
				     * let see if this is fixed or vari
				     * remember vari blk's are NULL 
				    */
				    if( ddp->blk_size != NULL){

					/* Grab the blk count from the sense data. */

					sc->sc_resid[targid] =
					(sz_sbtol( &(sc->sc_sns[targid].infobyte3))) *
							( ddp->blk_size );
					
					sc->sc_xstate[targid] = SZ_NEXT;

				    }	/* end if blk_size */
					
				    else {
					/*
					 * just a normal vari unit
					*/
					/* Grab the byte count from the sense data. */

					sc->sc_resid[targid] =
					sz_sbtol( &(sc->sc_sns[targid].infobyte3));
					
					sc->sc_xstate[targid] = SZ_NEXT;
				    }
			
				} /* end if dens valid */
				else {
				    /* ERRLOG_MARK */
				    cprintf("scsi OPTTABLE off id = %X\n", targid);
			
				    /*
				     * default it
				    */
				    /* Grab the blk count from the sense data. */

				    sc->sc_resid[targid] =
				    sz_sbtol( &(sc->sc_sns[targid].infobyte3));
				
				    sc->sc_xstate[targid] = SZ_NEXT;
				}

				/* end of if option table */
			    }

			    else  { /* old style */
				/* Grab the byte count from the sense data. */
				sc->sc_resid[targid] =
				sz_sbtol( &(sc->sc_sns[targid].infobyte3));
				
				sc->sc_xstate[targid] = SZ_NEXT;
			    }
			} /* end of if short record */
			else
			    sc->sc_xstate[targid] = SZ_R_COPY;

			break;

		    case SZ_RETRY:

			PRINTD(targid, 0x8000, 
			  ("RDMA_ERR: t = %x, SZ_RETRY from szerror\n", targid));

			if (sz_retries[unit] < SZ_RW_RTCNT) {
			    sz_retries[unit]++;
			    /*
			     * Since we are going to do the cmd again
			     * MUST start with clean slate in szflags
			     */
			    sc->sc_szflags[targid] = SZ_NORMAL;
			    sc->sc_xstate[targid] = SZ_RW_CONT;
			    sc->sc_xevent[targid] = SZ_CONT;
			}
			else {
			    sz_retries[unit] = 0;
			    bp->b_flags |= B_ERROR;
			    sc->sc_xstate[targid] = SZ_NEXT;
			    sc->sc_xevent[targid] = SZ_CONT;
			    /*
			     * Copy the data back to the user on 
			     * UNRECOVERABLE data errors. Adjust
 			     * the count in "sc_bpcount" to copy 
 			     * back the right amount of data.
			     */
		            if ((sc->sc_devtyp[targid] & SZ_DISK) &&
				(sc->sc_sns[targid].valid) &&
			        (sc->sc_sns[targid].snskey == SZ_MEDIUMERR)) {
	    			badblk = 
				    sz_sbtol( &(sc->sc_sns[targid].infobyte3));
				sc->sc_bpcount[targid] =
				    ((badblk - sc->sc_blkno[targid] + 1) * 512);
			        sc->sc_xstate[targid] = SZ_R_COPY;
			        sc->sc_xevent[targid] = SZ_CONT;
			    }
			}
			break;

		    case SZ_FATAL:

			if ((sc->sc_flags[targid] & DEV_EOM) &&
			    (sc->sc_flags[targid] & DEV_CSE) &&
			    (sc->sc_sns[targid].eom)) {
				sc->sc_xstate[targid] = SZ_R_COPY;
				sc->sc_xevent[targid] = SZ_CONT;
				break;
			}

			if ((sc->sc_flags[targid] & DEV_EOM) &&
			   !((sc->sc_flags[targid] & DEV_CSE) ||
			     (dis_eot_sz[unit] & DISEOT))) {
				bp->b_error = ENOSPC;
			}

			bp->b_flags |= B_ERROR;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;

			/*
			 * Copy the data back to the user on 
			 * UNRECOVERABLE data errors. Adjust
			 * the count in "sc_bpcount" to copy 
			 * back the right amount of data.
			 */

			if ((sc->sc_devtyp[targid] & SZ_DISK) &&
			    (sc->sc_sns[targid].valid) &&
			    (sc->sc_sns[targid].snskey == SZ_MEDIUMERR)) {
			    badblk = 
				sz_sbtol( &(sc->sc_sns[targid].infobyte3));
			    sc->sc_bpcount[targid] =
				((badblk - sc->sc_blkno[targid] + 1) * 512);
			    sc->sc_xstate[targid] = SZ_R_COPY;
			    sc->sc_xevent[targid] = SZ_CONT;
			}
			break;

		    default:
			bp->b_flags |= B_ERROR;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;

		    }	/* end switch(szerror()) */

	    break;

	/*------------------------------------------------------*/
	/*							*/
	/*                 S Z   W   S T D M A			*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_W_STDMA:

	    /* remember where we are in case target returns busy status */

	    sc->sc_pxstate[targid] = SZ_W_STDMA;

	    bp = sc->sc_bp[targid];	/* TODO1: */

	    /*
	     * Set DEV_WRITTEN so filemarks will be written on close.
	     */

	    if (sc->sc_devtyp[targid] & SZ_TAPE)
		sc->sc_flags[targid] |= DEV_WRITTEN;

	    bp->b_resid = 0;

	    ssrv = (*sc->port_start)(sc, targid, bp);

	    switch (ssrv) {

		case SZ_BUSBUSY:	/* must wait for SCSI bus to free up */
		    sc->sc_selstat[targid] = SZ_BBWAIT;
		    sc->sc_xstate[targid] = SZ_RW_CONT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    dp->b_active = 0;
		    return;
		    break;

		/* TODO: verify this can't happen for a data xfer command */

		case SZ_SUCCESS:
		    sc->sc_xfercnt[targid] += sc->sc_bpcount[targid];
		    if (sc->sc_xfercnt[targid] >= bp->b_bcount) {
			sc->sc_resid[targid] = 0;
		    }
		    else {
			sc->sc_resid[targid] = 
				(bp->b_bcount - sc->sc_xfercnt[targid]);
		    }
		    sc->sc_xstate[targid] = SZ_RW_CONT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		case SZ_IP:
		    sc->sc_xstate[targid] = SZ_W_DMA;
		    return;
		    break;

		case SZ_RET_ERR:
		    sc->sc_xstate[targid] = SZ_W_DMA;
		    break;

		case SZ_RET_ABORT:
		default:
		    PRINTD(targid, 0x10,
                        ("SZ_W_STDMA return from port_start: SZ_RET_ABORT\n"));
		    bp->b_flags |= B_ERROR;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

	    }
	    break;

	/*------------------------------------------------------*/
	/*							*/
	/*                    S Z   W   D M A			*/
	/*							*/
	/*------------------------------------------------------*/

	case SZ_W_DMA:

	        /* remember where we are in case target returns busy status */

	        sc->sc_pxstate[targid] = SZ_W_DMA;

		bp = sc->sc_bp[targid];	/* TODO1: */

	        if (sc->sc_xevent[targid] == SZ_SELRETRY_SNS) {
		    PRINTD(targid, 0x8000,
		       ("W_DMA: t=%x, evt = SELRETRY_SNS, going to sz_wdma_err\n",
			   targid));
		    goto sz_wdma_err;
	        }

	    	/*
	    	 * Determine if something went wrong that requires that
	    	 * a request sense command or a space command needs to be
	    	 * done here.  If a request sense needs to be done, send
	    	 * the command, and evaluate the result, and determine 
	    	 * if we need to return an error, return a short read,
	    	 * or retry the command.
	    	 */

	    if (sc->sc_szflags[targid] & SZ_NEED_SENSE) {
		/*
		 * TODO: 
		 * Determine what to do if the SZ_RQSNS fails
		 */

		sz_save_rqsns(sc, targid);

		sz_bldpkt(sc, targid, SZ_RQSNS, 0, 0);

sz_wdma_err:
		ssrv = (*sc->port_start)(sc, targid, bp);

		PRINTD(targid, 0x8000,
		   ("W_DMA: t = %x, rtn fr RQSNS port_start = %x\n",
		    targid, ssrv));

		switch(ssrv) {

		    case SZ_BUSBUSY:

		    	sc->sc_selstat[targid] = SZ_BBWAIT;
			dp->b_active = 0;

			/* xstate ok as is, but set xevent */

			if (rdatdebug)
			    cprintf("SP_CONT: BUSBUSY, xevent = SELRETRY_SNS\n");

			sc->sc_xevent[targid] = SZ_SELRETRY_SNS;

			return;

			break;		/*NOTREACHED*/

		      case SZ_SUCCESS:

			sc->sc_xstate[targid] = SZ_WDMA_ERR;
			break;

		      case SZ_IP:

			sc->sc_xstate[targid] = SZ_WDMA_ERR;
			return;
			break;		/*NOTREACHED*/

		      default:

			PRINTD(targid, 0x8000,
			   ("W_DMA: t = %x, unknown rc for RQSNS port_start\n",
			    targid));

			sz_restore_rqsns(sc, targid);

			sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;
			bp->b_flags |= B_ERROR;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;
			
		}	/* end switch(ssrv) on RQSNS for W_DMA */

	    }		/* end if NEED_SENSE */
	    else {
		if (sc->sc_szflags[targid] & SZ_WAS_DISCON)
		    sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;

		bcount = sc->sc_bpcount[targid];

		sc->sc_xfercnt[targid] += bcount;

		if (sc->sc_xfercnt[targid] >= bp->b_bcount)
		    sc->sc_resid[targid] = 0;
		else
		    sc->sc_resid[targid] = bp->b_bcount - sc->sc_xfercnt[targid];

		sc->sc_xstate[targid] = SZ_RW_CONT;
		sc->sc_xevent[targid] = SZ_CONT;

	    }	/* end else (no sense needed on W_DMA)  */

	break;


	/*------------------------------------------------------*/
	/*							*/
	/*             S Z   W   D M A   E R R			*/
	/*							*/
	/*------------------------------------------------------*/

        case SZ_WDMA_ERR:

	        PRINTD(targid, 0x8000, 
		   ("SP_WDMA_ERR: t =%x, call restore_rqsns\n", targid));

	            sz_restore_rqsns(sc, targid);

		    sc->sc_szflags[targid] &= ~SZ_NEED_SENSE;

		    szrtn = szerror(sc, targid);

 PRINTD(targid, 0x8000, 
  ("WDMA_ERR: szrtn = %x, sc_szflags = %x, sc_flags = %x, sc_cat_flags = %x\n", 
	  szrtn, sc->sc_szflags[targid], sc->sc_flags[targid], 
	     sc->sc_category_flags[targid])); 

		    switch(szrtn) {

		    case SZ_SUCCESS:

			sc->sc_xfercnt[targid] += sc->sc_bpcount[targid];

			if (sc->sc_xfercnt[targid] >= bp->b_bcount) {
			    sc->sc_resid[targid] = 0;
			}
			else {
			    sc->sc_resid[targid] = 
				(bp->b_bcount - sc->sc_xfercnt[targid]);
			}

			sc->sc_xstate[targid] = SZ_RW_CONT;
			sc->sc_xevent[targid] = SZ_CONT;

			break;

		    case SZ_RETRY:
			if (sz_retries[unit] < SZ_RW_RTCNT) {
			    sz_retries[unit]++;
			    /*
			     * Since we are going to do the cmd again
			     * MUST start with clean slate in szflags
			     */
			    sc->sc_szflags[targid] = SZ_NORMAL;
			    sc->sc_xstate[targid] = SZ_RW_CONT;
			    sc->sc_xevent[targid] = SZ_CONT;
			}
			else {
			    sz_retries[unit] = 0;
			    bp->b_flags |= B_ERROR;
			    sc->sc_xstate[targid] = SZ_NEXT;
			    sc->sc_xevent[targid] = SZ_CONT;
			}
			break;

		    case SZ_FATAL:
			/* 
			 * If DEV_CSE is set, the utility knows what
			 * it's doing, so just continue letting it
			 * write.
			 */
			if ((sc->sc_flags[targid] & DEV_EOM) &&
			   (sc->sc_flags[targid] & DEV_CSE) &&
			   (sc->sc_sns[targid].eom)) {

			    sc->sc_xfercnt[targid] += sc->sc_bpcount[targid];

			    if (sc->sc_xfercnt[targid] >= bp->b_bcount) {
				sc->sc_resid[targid] = 0;
			    }
			    else {
				sc->sc_resid[targid] = 
				  (bp->b_bcount - sc->sc_xfercnt[targid]);
			    }
			    sc->sc_xstate[targid] = SZ_RW_CONT;
			    sc->sc_xevent[targid] = SZ_CONT;

			    break;

			}	/* end if sns.eom */

			/*
			 * This case is really not a fatal error.
			 * The data is really written, but it is 
			 * easier to handle it here as an error
			 * TODO: not standing under this one.
			 */

			if ((sc->sc_flags[targid] & DEV_EOM) &&
			   !((sc->sc_flags[targid] & DEV_CSE) ||
			    (dis_eot_sz[unit] & DISEOT))) {

			    bp->b_error = ENOSPC;

			    sc->sc_xfercnt[targid] += sc->sc_bpcount[targid];

			    if (sc->sc_xfercnt[targid] >= bp->b_bcount) {
				sc->sc_resid[targid] = 0;
			    }
			    else {
				sc->sc_resid[targid] = 
				  (bp->b_bcount - sc->sc_xfercnt[targid]);
			    }

			    sc->sc_xstate[targid] = SZ_RW_CONT;
			    sc->sc_xevent[targid] = SZ_CONT;
			    break;
			}
			/* FALLTHROUGH */
		    default:
			bp->b_flags |= B_ERROR;
			sc->sc_xstate[targid] = SZ_NEXT;
			sc->sc_xevent[targid] = SZ_CONT;
			break;

		    }	/* end switch(szerror()) */

		    break;


	/*------------------------------------------------------*/
	/*							*/
	/*                    S Z   E R R			*/
	/*							*/
	/*------------------------------------------------------*/

	/*
	 * TODO:
	 *	Totally untested and may not even be used!
	 *	Currently only called from abort in szintr.
	 */
	case SZ_ERR:
	    switch (sc->sc_xevent[targid]) {
		case SZ_RESET:
		    /* 
		     * Reset the bus and then run down the request queue
		     * returning an error for all requests till the queue
		     * is empty.
		     */
		    while (dp->b_actf != NULL) {
			if (bp == &cszbuf[unit]) {
			    bp->b_flags |= B_ERROR;
			    bp->b_bcount = 0;
			    dp->b_actf = bp->av_forw;
			    biodone(bp);
			}
			else {
			    bp = dp->b_actf;
			    bp->b_resid = bp->b_bcount;
			    bp->b_error = EIO;
			    bp->b_flags |= B_ERROR;
			    dp->b_actf = bp->av_forw;
			    biodone(bp);
			}
		    }
/* TODO1: new reset scheme, don't reset bus! */
/*		    sz_reset(sc);	*/
		    return;
		    break;

		case SZ_ABORT:
		case SZ_PHA_MIS:
		case SZ_FREEB:
		    PRINTD(targid, 0x10,
			("sz_start: SZ_ERR: xevent = 0x%x\n",
			sc->sc_xevent[targid]));
#ifdef SZDEBUG
/* TODO: remove this and szdumpregs() routine? */
/*		    szdumpregs(cntlr);	*/
#endif SZDEBUG
		    PRINTD(targid, 0x10, ("sz_start: SZ_ERR: giving up!\n"));
		    /* TODO: set sc_resid? */
		    sc->sc_flags[targid] |= DEV_HARDERR;	
	
		    /* Make sure the error is reported back. */
		    bp->b_flags |= B_ERROR;
		    bp->b_error = EIO;
	
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;

		default:
		    PRINTD(targid, 0x10, ("sz_start: SZ_ERR: xevent = 0x%x\n",
			sc->sc_xevent[targid]));
		    bp->b_flags |= B_ERROR; 
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_CONT;
		    break;
	    }		/* end switch (xevent) */
	    break;

	/*------------------------------------------------------*/
	/*							*/
	/*     		     D E F A U L T			*/
	/*							*/
	/*------------------------------------------------------*/

	default:
	    PRINTD(targid, 0x8,
		("st0: sz_start: unknown xstate  = 0x%x\n",
		sc->sc_xstate[targid]));

	}	/* end main switch(xstate)      */

    }	/* end for (;;) */

}	/* end sz_start */

/*
 * Sense Key text string table.
 * Table indexed by the sense key.
 * NOTE: sense key 0x09 is vendor unique.
 *	 DEC devices don't use sense key 0x09,
 *	 but this could be a problem later.
 * NOTE: last code is "fake" for UNKNOWN codes.
 */
char	*sz_SenseKey[] = {
	"NO SENSE",
	"RECOVERED ERROR",
	"NOT READY",
	"MEDIUM ERROR",
	"HARDWARE ERROR",
	"ILLEGAL REQUEST",
	"UNIT ATTENTION",
	"DATA PROTECT",
	"BLANK CHECK",
	"VENDOR UNIQUE",
	"COPY ABORTED",
	"ABORTED COMMAND",
	"EQUAL",
	"VOLUME OVERFLOW",
	"MISCOMPARE",
	"RESERVED",
	"UNKNOWN",	/* not a real sense key */
	0
};

/*
 * SCSI bus protocol error message strings.
 */
char	*sz_BusErr[] = {
	"REQ failed to set",				/* 0 */
	"REQ failed to clear",				/* 1 */
	"Received < 5 data bytes",			/* 2 */
	"Unknown message",				/* 3 */
	"BSY hung after CMDCPT",			/* 4 */
	"asc_scsistart: ASC_INTP did not set",		/* 5 */
	"",						/* 6 */
	"sii_recvdata: SII_DNE/SII_MIS did not set",	/* 7 */
	"sii_senddata: SII_DNE/SII_MIS did not set",	/* 8 */
	"sii_recvdata: SII_IBF not set",		/* 9 */
	"sii_senddata: SII_TBE not set",		/* a */
	"sii_clear_discon_io_requests: clearing ID",	/* b */
	"Unknown phase (4/5) in phase change",		/* c */
	"DMA error in sii_restartdma",			/* d */
	"BSY not set on phase change",			/* e */
	"REQ not set on phase change",			/* f */
	0
};

/*
 * Print sense data out of sc->sc_sns[unit], from SZ_RQSNS command
 */
szprintsense(sc, targid)
register struct sz_softc *sc;
int targid;
{
	u_char *byteptr;
	int i;

	byteptr = (u_char *)&sc->sc_sns[targid];
#ifdef vax
	mprintf("request sense data: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
		*byteptr, *byteptr + 1, *byteptr + 2, *byteptr + 3,
		*byteptr + 4, *byteptr + 5, *byteptr + 6, *byteptr + 7,
		*byteptr + 8, *byteptr + 9, *byteptr + 10, *byteptr + 11,
		*byteptr + 12, *byteptr + 13, *byteptr + 14, *byteptr + 15,
		*byteptr + 16, *byteptr + 17, *byteptr + 18, *byteptr + 19);
#endif vax
#ifdef mips
	cprintf("request sense data: ");
	for(i=0; i<SZ_RQSNS_LEN; i++)
	    cprintf("%x ",*(byteptr+i));
	cprintf("\n");
#endif mips
}

int	sz_err_rtcnt = 0;
int	sz_se_rstop = 1;

/*
 * Error logging control variables.
 */
int	sz_log_errors = 1;	/* If 0, log to console instead of error log */
int	sz_log_retries = 1;	/* If 0, do not log error retries	     */
int	sz_log_tape_softerr = 1;/* If 0, do not log soft tape errors         */
int	sz_log_wp_errors = 0;	/* If 0, do not log write protect errors     */

szerror(sc, targid)
register struct sz_softc *sc;
int targid;
{
	int sz_ret = 0;	/* the return flag to return if no error	      */
	int softerr=0;	/* a soft error has occured, report only	      */
	int harderr=0;	/* a hard error has occured, report, and terminate    */
	int retryerr=0;	/* a ? error has occured, report and retry	      */
	int snskey;
	int ifb;
	int cntlr;
	int unit;
	int rtcnt;
	struct buf *bp = sc->sc_bp[targid]; /* get our active buf struct.*/
	int part;
	int sn;
	int flags;
	struct scsi_devtab *sdp;
	struct tape_opt_tab *todp; /* our tape option table pointer */
	struct tape_info *ddp; /* our density struct for tapes */

#ifdef vax
	cntlr = sc - &sz_softc[0];
#endif vax
#ifdef mips
	cntlr = sc->sc_siinum;
#endif mips
	unit = sc->sc_unit[targid];
	PRINTD(targid, 0x1, ("szerror:\n"));

/*      PRINTD(targid, 0x10, ("szerror: targid = %x\n", targid)); */

/*	PRINTD(targid, 0x10, ("", szprintsense(sc, targid)));	*/

	sdp = (struct scsi_devtab *)sc->sc_devtab[targid];

	/*
	 * Set maximum retry count. Tells us
	 * harderr vs softerr.
	 */
  	if ((sc->sc_curcmd[targid] == SZ_READ) ||
	    (sc->sc_curcmd[targid] == SZ_WRITE) ||
	    (sc->sc_curcmd[targid] == SZ_READ_10) ||
	    (sc->sc_curcmd[targid] == SZ_WRITE_10)) {
		rtcnt = SZ_RW_RTCNT;
	}
	else {
		rtcnt = SZ_SP_RTCNT;
	}
	/*
	 * Sense data must be valid and be
	 * extended sense data format.
	 * All DEC devices use extended sense data.
	 * TODO: log error?
	 */

	/* TODO: debug */
	if (sz_se_rstop && (sc->sc_szflags[targid] & SZ_RSTIMEOUT))
	if (sc->sc_sns[targid].errclass != 7)
		return(SZ_FATAL);

	switch (sc->sc_sns[targid].snskey) {
		case SZ_NOSENSE:
		    sz_ret = SZ_SUCCESS;

		    if (sc->sc_devtyp[targid] & SZ_TAPE) {

			/*
			 * Do file mark handling before end of media check.
			 */
			if (sc->sc_sns[targid].filmrk) {
				
			    /* 
			     * For fixed block units.. Since record
			     * boundaries are not preserved we can 
			     * detect a file mark before the entire 
			     * read is satisfied... We check info byte
			     * 3 to see if any data was found....
			     * was still a byte count left..... Flag
			     * short record instead of file mark and
			     * set TPMARK_PENDING... On next read return 
			     * TPMARK.... This is done because the head 
			     * is positioned after the tape mark.
			     *
			     * Since the fixed unit support came with the
			     * option table we must be set up for this to 
			     * work correctly.
			    */ 

			    if(sdp->opt_tab != NULL){
			
				/* 
				 * get our pointer and use it to see
				 * if we are a fixed block unit.
				*/

				todp = (struct tape_opt_tab *)sdp->opt_tab;
				
				/*
				 * get our density struct for this setting
				*/
				ddp = &todp->tape_info[DENS_IDX( bp )];
				
				/* 
				 * lets see if this density selection is valid
				*/
				if( ddp->tape_flags & DENS_VAL){

				    /*
				     * Well we have a valid density
				     * let see if this is fixed or vari
				     * remember vari blk's are NULL 
				    */
				    if( ddp->blk_size != NULL){
					/* 
					 * ok folks lets see if any
					 * data went across
					*/
					if( sz_sbtol(&sc->sc_sns[targid].infobyte3) != 
						(bp->b_bcount/ddp->blk_size) ){

					    sc->sc_category_flags[targid] |= DEV_SHRTREC;
					    sc->sc_category_flags[targid] |= TPMARK_PENDING;
					}
					else {
					    /* no data went */
					    sc->sc_category_flags[targid] |= 
							DEV_TPMARK;
					}
				    }	/* end if blk_size */
					
				    else {
					/*
					 * just a normal vari unit
					*/
					sc->sc_category_flags[targid] |= DEV_TPMARK;
				    }
			
				} /* end if dens valid */
				else {
				    /* ERRLOG_MARK */
				    cprintf("scsi OPTTABLE off id = %X\n", targid);
			
				    /*
				     * default it
				    */
				    sc->sc_category_flags[targid] |= DEV_TPMARK;
				}

				/* end of if option table */
			    }
			    else { /* just a normal varible unit */

				sc->sc_category_flags[targid] |= DEV_TPMARK;
			    }

			PRINTD(targid, 0x10, ("szerror: read filemark\n"));

			}	/* end if sns.filmrk */

			else if (sc->sc_sns[targid].eom) {

			    PRINTD(targid, 0x10, ("szerror: EOM\n"));

			    if ((sc->sc_curcmd[targid] == SZ_READ) || 
				(sc->sc_curcmd[targid] == SZ_WRITE) ||
				(sc->sc_curcmd[targid] == SZ_READ_10) ||
	    			(sc->sc_curcmd[targid] == SZ_WRITE_10)) {

				    if (dis_eot_sz[unit] != DISEOT) {
					sc->sc_flags[targid] |= DEV_EOM;
					sz_ret = SZ_FATAL;

				    }	/* end if dis_eot != DISEOT */

			    }	/* end if curcmd == READ | WRITE */

			}   /* end if sns.eom */

			/*
			 * If we hit FILMRK, then the ili bit doesn't matter.
			 */

			else if (sc->sc_sns[targid].ili) {
			    PRINTD(targid, 0x10,
				("szerror: ili on sc_curcmd = 0x%x\n",
				sc->sc_curcmd[targid]));

			  /* Assume all other commands will generate SZ_FATAL */
			    sz_ret = SZ_FATAL;

			  /* Some fancy checking has to be done in the event of
			    a read.  There are 2 read cases "over-read" and
			    "under-read".  An "under-read" is not an error, you
			    got what you asked for.  An "over-read" needs to be
			    flaged so the state machine can update b_resid. */

			    if (sc->sc_curcmd[targid] == SZ_READ)
			    {
				sz_ret = SZ_SUCCESS;	/* it's ok */

			      /* The info bytes contain a two's compliment value
				of the difference.  For an "under-read" the
				value is negative. */

				if( (sc->sc_sns[targid].infobyte3 & 0x80) == 0 )
				{
				    /* signal over-read */
                                    sc->sc_category_flags[targid] |= DEV_SHRTREC;

				}	/* end if sns.infobyte3 & 0x80 */

			    }	/* end if curcmd == READ */

			}	/* end if sns.ili */

		    }	/* end if devtyp & SZ_TAPE */

		    /* TODO: debug */
		    if (sc->sc_szflags[targid] & SZ_RSTIMEOUT) {
			sz_ret = SZ_RETRY;
			sc->sc_szflags[targid] &= ~SZ_RSTIMEOUT;
			sz_err_rtcnt++;
		    }
		    break;

		case SZ_RECOVERR:
		  /* Allow the command to complete.  Signal a recovered error
		    in sc_flags for the BBR code. */

		    PRINTD(targid, 0x10, ("szerror: SZ_RECOVERR\n"));

		    sz_ret = SZ_SUCCESS;
		    sc->sc_szflags[targid] |= SZ_RECOVERED;
		    softerr++;
		    break;

		case SZ_NOTREADY:
		    PRINTD(targid, 0x10, ("szerror: SZ_NOTREADY\n"));
		    sz_ret = SZ_RETRY;
		    if (sc->sc_curcmd[targid] != SZ_TUR) {
			harderr++;
		        sz_ret = SZ_FATAL;
		    }
		    break;

		case SZ_MEDIUMERR:
		    /*
		     * Retry media errors on disk only.
		     */
		    PRINTD(targid, 0x10, ("szerror: SZ_MEDIUMERR\n"));

		    if (sc->sc_devtyp[targid] & SZ_DISK) {
			sz_ret = SZ_RETRY;
			if (sz_retries[unit] >= rtcnt)
			    harderr++;
			else
			    retryerr++;
			break;
		    }
		    /* FALLTHROUGH - tape and cdrom */

		    sz_ret = SZ_FATAL;

		    if (sc->sc_devtyp[targid] & SZ_TAPE) {
			if (sc->sc_sns[targid].eom)
			    /* can't DEV_UGH, called from szintr()! */
			    mprintf("szerror: %s unit %d: encountered EOT\n",
				sc->sc_device[targid], unit);
		    }
		    harderr++;
		    break;

		case SZ_HARDWARE:
		    PRINTD(targid, 0x10, ("szerror: SZ_HARDWARE\n"));

		    /*
		     * If disk reselect timeout, change to
		     * soft error and retry.
		     */
		    if ((sc->sc_devtyp[targid] & SZ_DISK) &&
			(sc->sc_sns[targid].asb.rz_asb.asc == 0x45)) {
			if (sc->sc_szflags[targid] & SZ_RSTIMEOUT)
			    sc->sc_szflags[targid] &= ~SZ_RSTIMEOUT;
		    }
		    /*
		     * We now retry all disk hardware errors. If this is the
		     * last retry report hard error, otherwise retry error.
		     */
		    if (sc->sc_devtyp[targid] & SZ_TAPE) {
			sz_ret = SZ_FATAL;
			harderr++;
			break;
		    }
		    sz_ret = SZ_RETRY;

		    if (sz_retries[unit] >= rtcnt)
			harderr++;
		    else
			retryerr++;
		    break;

		case SZ_ILLEGALREQ:
		    PRINTD(targid, 0x8010, ("szerror: SZ_ILLEGALREQ\n"));
		    sz_ret = SZ_FATAL;
		    harderr++;
		    /*
		     * If this is a CD-ROM and the error is "Illegal Mode
		     * for this Track" error (0x64, 0x00), then don't
		     * log the error.  This is caused by rzopen trying
		     * to read the partition table on an audio disk.
		     */
		    if ( (sc->sc_devtyp[targid] & SZ_CDROM) &&
			 (sc->sc_sns[targid].asb.cd_asb.asc == 0x64) &&
			 (sc->sc_sns[targid].asb.cd_asb.rb2 == 0x00) ) {
			/*
			 * Setting DEV_HARDERR completes all nbuf I/O
			 * reads with EIO.
			 */
			sc->sc_flags[targid] |= DEV_HARDERR;
			return (sz_ret);
		    }
		    break;

		case SZ_UNITATTEN:
		    PRINTD(targid, 0x8010, ("szerror: SZ_UNITATTEN\n"));

		    /* Count floppy media changes (softpc hooks) */
		    if ((sc->sc_devtyp[targid] & SZ_DISK) &&
			(sc->sc_sns[targid].asb.rz_asb.asc == 0x28))
			    sc->sc_mc_cnt[targid]++;
		    /*
		     * Retry the command if the target is a disk or cdrom
		     * and the unit attention was caused by a bus reset.
		     * TODO: risk - assumes default mode select parameters!
		     */
		    if (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) {
			if (sc->sc_sns[targid].asb.rz_asb.asc == 0x29) {
			    sz_ret = SZ_RETRY;
			    if (sz_retries[unit] >= rtcnt)
				harderr++;
			    else
				retryerr++;
			    break;
			}
		    }
		    sz_ret = SZ_FATAL;
		    sz_unit_online[unit] = 0;  /* tell the world were offline */
		    if (sc->sc_devtyp[targid] & SZ_TAPE)
			sc->sc_flags[targid] &= ~DEV_EOM;
		    break;
	
		case SZ_DATAPROTECT:
		    PRINTD(targid, 0x8010, ("szerror: SZ_DATAPROTECT\n"));
		    sz_ret = SZ_FATAL;
		    harderr++;
		    if (sc->sc_devtyp[targid] & SZ_CDROM)
			break;
		    sc->sc_flags[targid] |= DEV_WRTLCK;
		    /* can't DEV_UGH, called from szintr()! */
		    mprintf("szerror: %s unit %d: media write protected\n",
			sc->sc_device[targid], unit);
		    break;

		case SZ_BLANKCHK:
		    if (sc->sc_devtyp[targid] & SZ_TAPE) {
			PRINTD(targid, 0x10, ("szerror: BLANKCHK\n"));
			/* 
			 * nothing is on the tape.... return
			 * EOM back to user....
			*/
			    
			sc->sc_flags[targid] |= DEV_EOM;
			sz_ret = SZ_FATAL;
			}
		    else {
			sz_ret = SZ_FATAL;
			harderr++;
		    }
		    break;

		case SZ_ABORTEDCMD:
		    PRINTD(targid, 0x8010, ("szerror: SZ_ABORTEDCMD\n"));

		    /*
		     * Could be parity error. Cannot retry, position lost.
		     */
		    if (sc->sc_devtyp[targid] & SZ_TAPE) {
			sz_ret = SZ_FATAL;
			harderr++;
			break;
		    }
		    sz_ret = SZ_RETRY;
		    if (sz_retries[unit] >= rtcnt)
			harderr++;
		    else
			retryerr++;
		    break;

		case SZ_VOLUMEOVFL:
		    PRINTD(targid, 0x8010, ("szerror: SZ_VOLUMEOVFL\n"));

		    sz_ret = SZ_FATAL;
		    harderr++;
		    if (sc->sc_devtyp[targid] & SZ_TAPE)
			/* can't DEV_UGH, called from szintr()! */
			mprintf("szerror: %s unit %d: volume overflow\n",
			    sc->sc_device[targid], unit);
		    break;

		case SZ_MISCOMPARE:
		    PRINTD(targid, 0x8010, ("szerror: SZ_MISCOMPARE\n"));
		    sz_ret = SZ_FATAL;
		    harderr++;
		    break;

		case SZ_VNDRUNIQUE:
		case SZ_COPYABORTD:
		case SZ_EQUAL:
		case SZ_RESERVED:

		    PRINTD(targid, 0x8010, ("szerror: VU,CPYABT,EQ,RES\n"));

		    /*
		     * Not supported by DEC devices.
		     */
		    sz_ret = SZ_FATAL;
		    harderr++;
		    break;

		default:
		    cprintf("szerror: UNKNOWN SENSE KEY <<<<<=========\n");
		    sz_ret = SZ_FATAL;
		    harderr++;
		    break;
	}

	if (sz_ret != SZ_SUCCESS) {
	    PRINTD(targid, 0x8000, ("sz_error: t=%x, sz_ret = %x\n", 
			  targid, sz_ret));
        }

	/*
	 * If BBR in progress, don't report any errors.
	 * The BBR code logs its own errors.
	 */

	if (sc->sc_bbr_active[targid])
	    return(sz_ret);

	flags = 0;

	if (harderr) {
	    flags |= SZ_HARDERR;
	    sc->sc_flags[targid] |= DEV_HARDERR;
	    sc->sc_hardcnt[targid]++;

	    PRINTD(targid, 0x8000, 
	       ("szerror: t=%x, HARDERR\n", targid));
	}
	else if (softerr) {
	    flags |= SZ_SOFTERR;
	    sc->sc_softcnt[targid]++;
	}
	else if (retryerr) {
	    flags |= SZ_RETRYERR;
	    sc->sc_softcnt[targid]++;	/* not exactly accurrate, but... */
	}
	else {
	    return(sz_ret);
	}
	flags |= (SZ_LOGSNS|SZ_LOGREGS|SZ_LOGCMD);
	scsi_logerr(sc, sc->sc_bp[targid], targid, SZ_ET_DEVERR, 0, 0, flags);

	return(sz_ret);
}



/*
 *
 * Name:		scsi_logerr	-SCSI log error routine
 *
 * Abstract:		This routine logs the many and varied types of
 *			SCSI errors to the system error log.
 *			NOTE: controller errors indicated by targid = -1.
 *
 * Inputs:
 *
 * sc			Pointer to controller's sz_softc structure.
 * bp			Pointer to I/O buffer header, 0 if bp not valid.
 * targid		Target ID of device (0 - 7), -1 if to valid.
 * errtyp		Error type (parity, select/reselect, data, etc.)
 * subtyp		Error sub type. If an error can occur multiple
 *			places the subtyp IDs the code segment.
 * busdat		SCSI bus data (which IDs asserted on the bus).
 * flags		Bitwise flags, tells what to log and other info.
 * bit0			Log SCSI command packet.
 * bit1			Log SCSI status byte.
 * bit2			Log SCSI message byte.
 * bit3			Log SCSI entended sense data.
 * bit4			Log SCSI controller hardware registers.
 * bit5			Log SCSI bus data.
 * bit6			Log select status of each target.
 * bits 16 17 18	Error severity: HARD, SOFT, RETRY error.
 *
 * Outputs:
 *			Error log packet is built and logged.
 *
 *
 * Return Values:	(currently not used)
 *
 * 0			Logging of error packet succeeded.
 * 1			Error packet could not be logged.
 *
 * Side Effects:
 *			None.
 *
 */

scsi_logerr(sc, bp, targid, errtyp, subtyp, busdat, flags)
struct sz_softc *sc;
struct buf *bp;
int targid;
int errtyp;
int subtyp;
int busdat;
int flags;
{

	register struct	el_rec	*elp;
#ifdef	vax
	register struct sz_regs *szaddr;
	register struct sii_regs *siiaddr;
#endif vax
#ifdef mips
	SII_REG *siiaddr = (volatile struct sii_regs *)sc->sc_scsiaddr;
	ASC_REG *ascaddr = (volatile ASCREG *)sc->sc_scsiaddr;
#endif mips
	register int i;
	struct sz_exsns_dt *sdp;
	int	cntlr, unit;
	int	class, type, ctldevtyp, subidnum, unitnum, errcode;
	int	info_flags = 0;
	char	*p;
	int	snskey, ifb, part, sn;

#ifdef	vax
	cntlr = sc - &sz_softc[0];
#endif	vax
#ifdef	mips
	cntlr = sc->sc_siinum;
#endif	mips
	if (targid >= 0)
	    unit = sc->sc_unit[targid];
	else
	    unit = 0;

	if ((flags & SZ_RETRYERR) && (sz_log_retries == 0))
	    return(1);

	if ((targid >= 0) && (flags & (SZ_SOFTERR|SZ_RETRYERR)) &&
	    (sz_log_tape_softerr == 0) && (sc->sc_devtyp[targid] & SZ_TAPE))
		return(1);
	/*
	 * Set the port type flag so we and UERF
	 * can tell which SCSI port logged the error.
	 * If port is unknown all flas will be zero.
	 * NOTE: won't work on mips until the reset routine is added.
	 */
#ifdef	vax
	if (sc->port_reset == sz_reset)
	    info_flags |= SZ_NCR5380;
#endif	vax
	if (sc->port_reset == sii_reset)
	    info_flags |= SZ_DECSII;

	if (sc->port_reset == asc_reset)
	    info_flags |= SZ_NCRASC;

	/*
	 * Get pointer to a slot in the error log buffer.
	 * All fields except the header must be filled in because
	 * they are not zeroed by ealloc();
	 * Print the error information on the console if we cannot
	 * get a buffer slot or the error log daemon is not running.
	 */
	if ((elprocp == 0) || (sz_log_errors == 0) ||
	    ((elp = ealloc((sizeof(struct el_bdev)), EL_PRIHIGH)) == EL_FULL)) {
	    switch(errtyp) {
	    case SZ_ET_DEVERR:
		if ((targid < 0) || (bp == 0))
		    break;

		if (sc->sc_sns[targid].snskey > 0x0f)
		    snskey = 0x010;		/* cause "UNKNOWN" to print */
		else
		    snskey = sc->sc_sns[targid].snskey;

		if (flags & SZ_HARDERR)
		    p = "harderr";
		else if (flags & SZ_SOFTERR)
		    p = "softerr";
		else if (flags & SZ_RETRYERR)
		    p = "retryerr";
		else
		    p = "error";

		cprintf("%s unit %d: %s, SnsKey = %s",
		    sc->sc_device[targid], unit, p, sz_SenseKey[snskey]);

		if (sc->sc_devtyp[targid] & SZ_DISK) {
		    cprintf(", AddSnsCode = 0x%x",
			sc->sc_sns[targid].asb.rz_asb.asc);
		}
		if (sc->sc_devtyp[targid] & SZ_CDROM) {
		    cprintf(", AddSnsCode = 0x%x",
			sc->sc_sns[targid].asb.cd_asb.asc);
		}
		cprintf("\n");

		/*
		 * Combine the information bytes together.
		 * Has meaning for all DEC devices
		 * (if valid bit is set).
		 */
		if (sc->sc_sns[targid].valid) {
		    ifb = sz_sbtol( &(sc->sc_sns[targid].infobyte3));
		    if (sc->sc_devtyp[targid] & SZ_TAPE) {
			cprintf("XferLenDiff = %d, ", ifb);
			cprintf("CntlrIntErrCode = 0x%x, ",
			    sc->sc_sns[targid].asb.tz_asb.ctlr);
			cprintf("DrvErrBytes: 0 = 0x%x 1 = 0x%x\n",
			    sc->sc_sns[targid].asb.tz_asb.drv0,
			    sc->sc_sns[targid].asb.tz_asb.drv1);
		    }
		    else {
			cprintf("LogBlkAddr = %d, ", ifb);
			part = minor(bp->b_dev) & 07;
			sn = ifb - sz_part[unit].pt_part[part].pi_blkoff;
		        cprintf("Dev = rz%d%c, SectNum = %d\n", 
			    unit, ('a' + part), sn);
		    }
		}
#ifdef	NOTUSED
		if (sc->sc_devtyp[targid] & SZ_CDROM) {
		    if (sc->sc_sns[targid].asb.cd_asb.bpv) {
			mprintf("Bit Pointer (C/D = %d) = %d\n",
			    sc->sc_sns[targid].asb.cd_asb.cd,
			    sc->sc_sns[targid].asb.cd_asb.bitp);
		    }
		    if (sc->sc_sns[targid].asb.cd_asb.fpv) {
			mprintf("Field Pointer: MSB = 0x%x, LSB = 0x%x\n",
			    sc->sc_sns[targid].asb.cd_asb.fpmsb,
			    sc->sc_sns[targid].asb.cd_asb.fplsb);
		    }
		}
#endif	NOTUSED
		/* NOTE: no SZ_LOGREGS because registers not meaningful. */
		break;

	    case SZ_ET_PARITY:
		cprintf("SCSI bus %d: parity error (#%d)", cntlr, subtyp);
		if (targid >= 0)
		    cprintf(", ID = %d", targid);
		else
		    cprintf(", ID = ?");
		if (flags & SZ_LOGBUS)
		    cprintf(", busdata = 0x%x", busdat);
		cprintf("\n");
		if (flags & SZ_LOGREGS) {
		    if (info_flags & SZ_NCR5380)
			sz_dumpregs(cntlr, 2);		/* cprintf */
		    if (info_flags & SZ_DECSII)
			sii_dumpregs(cntlr, 1);		/* from scsi_logerr */
		    if (info_flags & SZ_NCRASC)
			asc_dumpregs(cntlr, 1);
		}
		break;

	    case SZ_ET_BUSRST:
		cprintf("SCSI bus %d: bus reset detected (#%d)\n",
		    cntlr, subtyp);
		break;

	    case SZ_ET_RSTBUS:
		cprintf("SCSI bus %d: resetting bus (#%d)\n", cntlr, subtyp);
		if (flags & SZ_LOGREGS) {
		    if (info_flags & SZ_NCR5380)
			sz_dumpregs(cntlr, 2);		/* cprintf */
		    if (info_flags & SZ_DECSII)
			sii_dumpregs(cntlr, 1);		/* from scsi_logerr */
		    if (info_flags & SZ_NCRASC)
			asc_dumpregs(cntlr, 1);
		}
		break;

	    case SZ_ET_RSTTARG:
		cprintf("SCSI bus %d: resetting target (#%d)", cntlr, subtyp);
		if (targid >= 0)
		    cprintf(", ID = %d", targid);
		else
		    cprintf(", ID = ?");
		break;

	    case SZ_ET_CMDABRTD:
		cprintf("SCSI bus %d: command aborted (#%d)", cntlr, subtyp);
		if (targid >= 0) {
		    cprintf(", ID = %d", targid);
		    /* actcmd more accurate, but we only ever log curcmd */
		    cprintf(", cmd = 0x%x", sc->sc_curcmd[targid]);
		}
		else {
		    cprintf(", ID = ?");
		    cprintf(", cmd = ?");
		}
		cprintf("\n");
		break;

	    case SZ_ET_RESELERR:
		cprintf("SCSI bus %d: reselect error (#%d)", cntlr, subtyp);
		if (targid >= 0)
		    cprintf(", ID = %d", targid);
		else
		    cprintf(", ID = ?");
		if (flags & SZ_LOGBUS)
		    cprintf(", busdata = 0x%x", busdat);
		if ((targid >= 0) && (flags & SZ_LOGSELST))
		    cprintf(", selstat = %d", sc->sc_selstat[targid]);
		cprintf("\n");
		if (flags & SZ_LOGREGS) {
		    if (info_flags & SZ_NCR5380)
			sz_dumpregs(cntlr, 2);		/* cprintf */
		    if (info_flags & SZ_DECSII)
			sii_dumpregs(cntlr, 1);		/* from scsi_logerr */
		    if (info_flags & SZ_NCRASC)
			asc_dumpregs(cntlr, 1);
		}
		break;

	    case SZ_ET_STRYINTR:
		cprintf("SCSI bus %d: stray interrupt (#%d)", cntlr, subtyp);
		if (flags & SZ_LOGSELST) {
		    cprintf(", selstat (ID 0-7):");
		    for (i = 0; i < NDPS; i++)
			cprintf(" %d", sc->sc_selstat[targid]);
		}
		cprintf("\n");
		if (flags & SZ_LOGREGS) {
		    if (info_flags & SZ_NCR5380)
			sz_dumpregs(cntlr, 2);		/* cprintf */
		    if (info_flags & SZ_DECSII)
			sii_dumpregs(cntlr, 1);		/* from scsi_logerr */
		    if (info_flags & SZ_NCRASC)
			asc_dumpregs(cntlr, 1);
		}
		break;

	    case SZ_ET_SELTIMO:
		cprintf("SCSI bus %d: selection timeout", cntlr);
		if (targid >= 0) {
		    cprintf(", ID = %d [%s unit %d]", targid,
			sc->sc_device[targid], unit);
		    cprintf(", retry = %d", sc->sc_sel_retry[targid]);
		}
		else
		    cprintf(", ID = ?");
		cprintf("\n");
		break;

	    case SZ_ET_DISTIMO:
		cprintf("SCSI bus %d: disconnect timeout", cntlr);
		if (targid >= 0) {
		    cprintf(", ID = %d [%s unit %d]", targid,
			sc->sc_device[targid], unit);
		    if (flags & SZ_LOGCMD)
			cprintf(", cmd=0x%x", sc->sc_curcmd[targid]);
		    if (flags & SZ_LOGSELST)
			cprintf(", selstat=%d", sc->sc_selstat[targid]);
		}
		else
		    cprintf(", ID = ?");
		cprintf("\n");
		break;

	    case SZ_ET_CMDTIMO:
		cprintf("SCSI bus %d: command timeout", cntlr);
		switch(subtyp) {
		case 0:
		    cprintf(" - TAPE");
		    break;

		case 1:
		    cprintf(" - DISCON");
		    break;

		case 2:
		    cprintf(" - ACTIVE");
		    break;

		default:
		    break;
		}
		if (targid >= 0) {
		    cprintf(", ID = %d [%s unit %d]", targid,
			sc->sc_device[targid], unit);
		    if (flags & SZ_LOGCMD)
			cprintf(", cmd=0x%x", sc->sc_curcmd[targid]);
		}
		else
		    cprintf(", ID = ?");
		cprintf("\n");
		if (flags & SZ_LOGREGS) {
		    if (info_flags & SZ_NCR5380)
			sz_dumpregs(cntlr, 2);		/* cprintf */
		    if (info_flags & SZ_DECSII)
			sii_dumpregs(cntlr, 1);		/* from scsi_logerr */
		    if (info_flags & SZ_NCRASC)
			asc_dumpregs(cntlr, 1);
		}
		break;

	    case SZ_ET_ACTSTAT:
		cprintf("SCSI bus %d: activity status error (#%d)",
		    cntlr, subtyp);
		if (targid >= 0) {
		    cprintf(", ID = %d", targid);
		    if (flags & SZ_LOGSELST)
			cprintf(", selstat = %d", sc->sc_selstat[targid]);
		}
		else
		    cprintf(", ID = ?");
		cprintf("\n");
		break;

	    case SZ_ET_BUSERR:
		cprintf("SCSI bus %d: ", cntlr);
		if (info_flags & SZ_NCR5380) {
		    if (subtyp & 0x80)
			cprintf("start ");
		    else
			cprintf("intr ");
		}
		switch((subtyp >> 4) & 0x7) {
		case 0:
		    cprintf("DATAO ");
		    break;
		case 1:
		    cprintf("DATAI ");
		    break;
		case 2:
		    cprintf("CMD ");
		    break;
		case 3:
		    cprintf("STATUS ");
		    break;
		case 6:
		    cprintf("MESSO ");
		    break;
		case 7:
		    cprintf("MESSI ");
		    break;
		default:
		    break;
		}
		cprintf("%s ", sz_BusErr[subtyp & 0x0f]);
		if (targid >= 0) {
		    if (flags & SZ_LOGMSG)
			cprintf("(0x%x)", sc->sc_message[targid]);
		    cprintf(", ID = %d [%s unit %d]", targid,
			sc->sc_device[targid], unit);
		}
		else
		    cprintf(", ID = ?");
		cprintf("\n");
		if (flags & SZ_LOGREGS) {
		    if (info_flags & SZ_NCR5380)
			sz_dumpregs(cntlr, 2);		/* cprintf */
		    if (info_flags & SZ_DECSII)
			sii_dumpregs(cntlr, 1);		/* from scsi_logerr */
		    if (info_flags & SZ_NCRASC)
			asc_dumpregs(cntlr, 1);
		}
		break;

	    case SZ_ET_DBBR:
		/* This should never happen! */
		if ((targid < 0) || (bp == 0))
		    break;
		cprintf("SCSI bus %d: LBN %d - ", cntlr, bp->b_blkno);
		switch(subtyp) {
		case 0:
		    cprintf("MEDIUM ERROR during BBR");
		    break;

		case 1:
		    cprintf("reassign block failed");
		    break;

		case 2:
		    cprintf("reassign block succeeded");
		    break;

		case 3:
		    cprintf("reassign/write failed");
		    break;

		default:
		    break;
		}
		cprintf(", ID = %d [%s unit %d]", targid,
			sc->sc_device[targid], unit);
		cprintf("\n");
		break;

	    default:
		cprintf("scsi_logerr: %s (errtyp = %d, subtyp = %d\n",
		    "unknown error type", errtyp, subtyp);
		break;
	    }		/* end of switch */
	    return(1);
	}

	/*
	 * If we are resetting the bus, then
	 * the error also goes to the console.
	 * So the user knows why the system went
	 * silent for 5 seconds.
	 */
	if (errtyp == SZ_ET_RSTBUS)
	    cprintf("SCSI bus %d: resetting bus\n", cntlr);

	bzero(&elp->el_body.elbdev.eldevdata.elscsi, sizeof(struct el_scsi));

	/*
	 * Fill in the subsystem ID packet.
	 */
	if (targid < 0)
	    class = ELCT_DCNTL;		/* Controller error */
	else if (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM))
	    class = ELCT_DISK;
	else if (sc->sc_devtyp[targid] & SZ_TAPE)
	    class = ELCT_TAPE;
	else
	    class = EL_UNDEF;			/* cannot happen */

	if (class == ELCT_DCNTL)
	    type = ELSCSI_CNTRL;
	else
	    type = ELDEV_SCSI;

	if (class == ELCT_DCNTL) {
	    if (info_flags & SZ_NCR5380)
		ctldevtyp = ELSCCT_5380;
	    else if (info_flags & SZ_DECSII)
		ctldevtyp = ELSCCT_SII;
	    else if (info_flags & SZ_NCRASC)
		ctldevtyp = ELSCCT_ASC;
	    else
		ctldevtyp = EL_UNDEF;
	    subidnum = EL_UNDEF;	/* TODO: adpater type = no bus? */
	    unitnum = cntlr;
	}
	else {
	    switch(sc->sc_devtyp[targid]) {
	    case RX23:
		ctldevtyp = ELSDT_RX23;
		break;
	    case RX26:
		ctldevtyp = ELSDT_RX26;
		break;
	    case RX33:
		ctldevtyp = ELSDT_RX33;
		break;
	    case RZ22:
		ctldevtyp = ELSDT_RZ22;
		break;
	    case RZ23:
		ctldevtyp = ELSDT_RZ23;
		break;
	    case RZ23L:
		ctldevtyp = ELSDT_RZ23L;
		break;
	    case RZ24:
		ctldevtyp = ELSDT_RZ24;
		break;
	    case RZ25:
		ctldevtyp = ELSDT_RZ25;
		break;
	    case RZ55:
		ctldevtyp = ELSDT_RZ55;
		break;
	    case RZ56:
		ctldevtyp = ELSDT_RZ56;
		break;
	    case RZ57:
		ctldevtyp = ELSDT_RZ57;
		break;
	    case RRD40:
		ctldevtyp = ELSDT_RRD40;
		break;
	    case RRD42:
		ctldevtyp = ELSDT_RRD42;
  		break;
	    case CDxx:
	    case RZxx:
		ctldevtyp = ELSDT_RZxx;
		break;
	    case TZ30:
		ctldevtyp = ELSTT_TZ30;
		break;
	    case TZK50:
		ctldevtyp = ELSTT_TZK50;
		break;
	    case TLZ04:
		ctldevtyp = ELSTT_TLZ04;
		break;
	    case TZ05:
		ctldevtyp = ELSTT_TZ05;
		break;
	    case TZ07:
		ctldevtyp = ELSTT_TZ07;
		break;
	    case TZK08:
		ctldevtyp = ELSTT_TZK08;
		break;
	    case TZK10:
		ctldevtyp = ELSTT_TZK10;
		break;
	    case TZxx:
		ctldevtyp = ELSTT_TZxx;
		break;
	    default:
		ctldevtyp = EL_UNDEF;
		break;
	    }
	    subidnum = cntlr;
	    unitnum = unit;
	}
	errcode = EL_UNDEF;		/* NOTE: this field not used */
	LSUBID(elp, class, type, ctldevtyp, subidnum, unitnum, errcode)

	/*
	 * Fill in the block device header information.
	 * The target ID must be valid. If the target ID is
	 * valid then the unit number is also valid.
	 * The buffer pointer must be valid. We only log
	 * b_dev if the bp is a command buffer (cszbuf).
	 * NOTE: why b_addr from bp not logged?
	 */
	elp->el_body.elbdev.eldevhdr.devhdr_dev = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_flags = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_bcount = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_blkno = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_retrycnt = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_herrcnt = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_serrcnt = EL_UNDEF;
	elp->el_body.elbdev.eldevhdr.devhdr_csr = EL_UNDEF;

	if (bp && (targid >= 0)) {
	    elp->el_body.elbdev.eldevhdr.devhdr_dev = bp->b_dev;
	    elp->el_body.elbdev.eldevhdr.devhdr_retrycnt =
							sz_retries[unit];
	    elp->el_body.elbdev.eldevhdr.devhdr_herrcnt =
							sc->sc_hardcnt[targid];
	    elp->el_body.elbdev.eldevhdr.devhdr_serrcnt =
							sc->sc_softcnt[targid];
	    if (bp != &cszbuf[unit]) {
		elp->el_body.elbdev.eldevhdr.devhdr_flags = bp->b_flags;
		elp->el_body.elbdev.eldevhdr.devhdr_bcount = bp->b_bcount;
		elp->el_body.elbdev.eldevhdr.devhdr_blkno = bp->b_blkno;
		/* NOTE: what, if anything, should go in devhdr_csr? */
	    }
	}

	/* Retry count from different source for selection timeout. */
	if ((errtyp == SZ_ET_SELTIMO) && (targid >= 0)) {
	    elp->el_body.elbdev.eldevhdr.devhdr_retrycnt =
						    sc->sc_sel_retry[targid];
	}

	/*
	 * Fill in error type, subtype, version, and severity.
	 */
	elp->el_body.elbdev.eldevdata.elscsi.error_typ = errtyp;
	elp->el_body.elbdev.eldevdata.elscsi.suberr_typ = subtyp;
	elp->el_body.elbdev.eldevdata.elscsi.scsi_elvers = SZ_EL_VERS;
	info_flags |= (flags & SZ_ESMASK);

	/*
	 * Fill in device's target ID (if known).
	 */
	if (targid >= 0)
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_id = targid;
	else
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_id = EL_UNDEF;

	/*
	 * Fill in SCSI bus data.
	 */
	if (flags & SZ_LOGBUS) {
	    elp->el_body.elbdev.eldevdata.elscsi.bus_data = busdat;
	    info_flags |= SZ_LOGBUS;
	}
	else
	    elp->el_body.elbdev.eldevdata.elscsi.bus_data = EL_UNDEF;

	/*
	 * Fill in select status for each target.
	 */
	if (flags & SZ_LOGSELST) {
	    for (i = 0; i < NDPS; i++) {
		elp->el_body.elbdev.eldevdata.elscsi.scsi_selst[i] =
							sc->sc_selstat[i];
	    }
	    info_flags |= SZ_LOGSELST;
	}

	/*
	 * Fill in SCSI message in byte.
	 */
	if ((targid >= 0) && (flags & SZ_LOGMSG)) {
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_msgin =
						sc->sc_message[targid];
	    info_flags |= SZ_LOGMSG;
	}
	else
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_msgin = EL_UNDEF;

	/*
	 * Fill in SCSI command packet (CDB) and status byte.
	 */
	if ((flags & SZ_LOGCMD) && (targid >= 0)) {
	    for (i = 0; i < 12; i++) {
		elp->el_body.elbdev.eldevdata.elscsi.scsi_cmd[i] =
						sc->sc_cmdlog[targid][i];
	    }
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_status =
						sc->sc_statlog[targid];
	    info_flags |= SZ_LOGCMD;
	}

	/*
	 * Fill in extended sense data from request sense.
	 */
	if ((flags & SZ_LOGSNS) && (targid >= 0)) {
	    sdp = (struct sz_exsns_dt *)&sc->sc_sns[targid];
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_esd = *(sdp);
	    info_flags |= SZ_LOGSNS;
	}

	/*
	 * Fill in NCR 5380 registers and DMA registers.
	 * WARNING: cannot log the NCR 5380 current data register.
	 *	    Reading this register causes a parity error,
	 *	    which causes the driver to loop resetting the bus.
	 */
#ifdef	vax
	if ((flags & SZ_LOGREGS) && (info_flags & SZ_NCR5380)) {
	    szaddr = (struct sz_regs *)szmem + cntlr;
	    info_flags |= SZ_LOGREGS;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.ini_cmd =
							szaddr->scs_inicmd;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.mode =
							szaddr->scs_mode;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.tar_cmd =
							szaddr->scs_tarcmd;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.cur_stat =
							szaddr->scs_curstat;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.sel_ena =
							sc->sc_scs_selena;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.status =
							szaddr->scs_status;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.adr =
							szaddr->scd_adr;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.cnt =
							szaddr->scd_cnt;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ncrregs.dir =
							szaddr->scd_dir;
	}
#endif	vax
	if ((flags & SZ_LOGREGS) && (info_flags & SZ_DECSII)) {
	/* siiaddr initialized at start of routine for mips */
#ifdef	vax
	    siiaddr = (struct sii_regs *)cvqmsi + cntlr;
#endif	vax
	    info_flags |= SZ_LOGREGS;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_sdb =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_sc1 =
							siiaddr->sii_sc1;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_sc2 =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_csr =
							siiaddr->sii_csr;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_id =
							siiaddr->sii_id;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_slcsr =
							siiaddr->sii_slcsr;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_destat =
							siiaddr->sii_destat;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dstmo =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_data =
							siiaddr->sii_data;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dmctrl =
							siiaddr->sii_dmctrl;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dmlotc =
							siiaddr->sii_dmlotc;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dmaddrl =
							siiaddr->sii_dmaddrl;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dmaddrh =
							siiaddr->sii_dmaddrh;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dmabyte =
							siiaddr->sii_dmabyte;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_stlp =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_ltlp =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_ilp =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dsctrl =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_cstat =
							siiaddr->sii_cstat;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dstat =
							siiaddr->sii_dstat;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_comm =
							siiaddr->sii_comm;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dictrl =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_clock =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_bhdiag =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_sidiag =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_dmdiag =
							EL_UNDEF;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.siiregs.sii_mcdiag =
							EL_UNDEF;
	}

	if ((flags & SZ_LOGREGS) && (info_flags & SZ_NCRASC)) {
	    info_flags |= SZ_LOGREGS;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.tclsb =
		ascaddr->asc_tclsb & 0xff;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.tcmsb =
		ascaddr->asc_tcmsb & 0xff;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.cmd =
		ascaddr->asc_cmd & 0xff;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.stat =
		sc->sc_asc_sr;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.ss =
		sc->sc_asc_ssr;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.intr =
		sc->sc_asc_isr;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.ffr =
		ascaddr->asc_ffss & ASC_FIFO_MSK;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.cnf1 =
		ascaddr->asc_cnf1 & 0xff;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.cnf2 =
		ascaddr->asc_cnf2 & 0xff;
	    elp->el_body.elbdev.eldevdata.elscsi.scsi_regs.ascregs.cnf3 =
		ascaddr->asc_cnf3 & 0xff;


	}	/* end if NCRASC */

	/*
	 * Fill in the sector number for disk errors.
	 * The BBR documentation needs both the LBN and sector.
	 */
	elp->el_body.elbdev.eldevdata.elscsi.sect_num = EL_UNDEF;
	if ((errtyp == SZ_ET_DEVERR) && (bp) && (targid >= 0)) {
	    if ((sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) &&
		(sc->sc_sns[targid].valid) &&
		(bp != &cszbuf[unit])) {
		    ifb = sz_sbtol( &(sc->sc_sns[targid].infobyte3));
		    part = minor(bp->b_dev) & 07;
		    sn = ifb - sz_part[unit].pt_part[part].pi_blkoff;
		    elp->el_body.elbdev.eldevdata.elscsi.sect_num = sn;
	    }
	}

	/*
	 * Tell UERF which error information fields are valid.
	 */
	elp->el_body.elbdev.eldevdata.elscsi.info_flags = info_flags;

	EVALID(elp);
	return(0);
}


/* ---------------------------------------------------------------------- */
/* A scsi bytes to long conversion routine.  The data values associated with
scsi packets have a Big Endian format.  Our system uses Little Endian.  This
routine will pack 4 BE bytes, the pointer argument, into a long and return
the long. */

/* Scsi bytes to long conversion. */
long
sz_sbtol( p )
    unsigned char *p;
{
    union le_type
    {
	unsigned char c[4];
	unsigned long l;
    } le_type;

  /* Assign the bytes to the union character array. */

    le_type.c[3] = *p++;
    le_type.c[2] = *p++;
    le_type.c[1] = *p++;
    le_type.c[0] = *p++;

    return( le_type.l );
}

#ifdef mips

/* This routine is responsible for returning the SCSI bus ID for the 
controller number passed.  It the ID is not defined or is out of bounds the
global value defined in scsi_data.c will be used. */

char *idstring = "scsiid?";	/* string to search for */
#define CNTLR_INDEX	6	/* loc of controller char in the string */
#define ASCII_0		0x30	/* add to binary # to get ACSII equivilent */

int 
get_scsiid( cntlr )
    int cntlr;			/* controller/bus number on the system */
{
    char *env;			/* ptr for the NVR string */
    int nvr_id;			/* converted ID # from NVR */

    /* Build an id string from our controller #, i.e., scsiid0, 1, etc.  The 
    ID string can be reused. */

    idstring[ CNTLR_INDEX ] = (char)((cntlr & 0xff) + ASCII_0);

    env = (char *)prom_getenv( idstring );
    if (env != NULL) {
	nvr_id = xtob(env);		/* convert ACSII hex to binary */

	/* Is the ID a valid #, ID's on the SCSI bus can only be [0-7]. */
	if ((nvr_id >= 0) && (nvr_id <= 7)) {
	    return( nvr_id );
	}
    }
    
    /* The SCSI bus ID conversion failed, return the default value to be used
    for this controller. */

    PRINTD(0xFF, 0x10,
	("get_scsiid: Failure on conversion of %s, using default = %d\n",
	idstring, default_scsiid));

    return( default_scsiid );		/* return the default */
}

/******************************************************************
 *
 * Name:	sz_save_rqsns
 *
 * Abstract:	Save the "state" of a transaction so that RQSNS can
 *		be transparently done by the port code.
 *
 * Inputs:
 *
 * 	sc	Pointer to softc structure for controller.
 *	targid  Target.
 *
 * Outputs:	None.
 *
 * Return values: None.
 *
 ******************************************************************/

sz_save_rqsns(sc, targid)

struct sz_softc *sc;
int	targid;
{

    if(sc->rqs[targid].valid == 1) {
	cprintf("sz_save_rqsns: valid set, t = %x, sc_szflags = %x\n", 
	    targid, sc->sc_szflags[targid]);

	panic("sz_save_rqsns: valid set\n");
    }
 
    PRINTD(targid, 0x8000, ("sz_save_rqsns: t = %x\n", targid));

    sc->rqs[targid].valid = 1;

    sc->rqs[targid].szflags = sc->sc_szflags[targid];
    sc->rqs[targid].xfercnt = sc->sc_xfercnt[targid];
    sc->rqs[targid].bpcount = sc->sc_bpcount[targid];
    sc->rqs[targid].b_bcount = sc->sc_b_bcount[targid];
    sc->rqs[targid].resid = sc->sc_resid[targid];
    sc->rqs[targid].bufp = sc->sc_bufp[targid];

    sc->sc_szflags[targid] = SZ_NORMAL;

}	/* end sz_save_rqsns */

/******************************************************************
 *
 * Name:	sz_restore_rqsns
 *
 * Abstract:	Restore the "state" of a transaction so that RQSNS can
 *		be transparently done by the port code.
 *
 * Inputs:
 *
 * 	sc	Pointer to softc structure for controller.
 *	targid  Target.
 *
 * Outputs:	None.
 *
 * Return values: None.
 *
 ******************************************************************/

sz_restore_rqsns(sc, targid)

struct sz_softc *sc;
int	targid;

{
    if (sc->rqs[targid].valid != 1) {
	cprintf("sz_restore_rqsns: valid not set, t = %x, sc_szflags = %x \n", 
	    targid, sc->sc_szflags[targid]);

	panic("sz_restore_rqsns: valid not set\n");

    }

    PRINTD(targid, 0x8000, ("sz_restore_rqsns: t = %x\n", targid));

    sc->rqs[targid].valid = 0;

    sc->sc_szflags[targid] = sc->rqs[targid].szflags;
    sc->sc_xfercnt[targid] = sc->rqs[targid].xfercnt;
    sc->sc_bpcount[targid] = sc->rqs[targid].bpcount;
    sc->sc_b_bcount[targid] = sc->rqs[targid].b_bcount;
    sc->sc_bufp[targid] = sc->rqs[targid].bufp;
    sc->sc_resid[targid] = sc->rqs[targid].resid;

}	/* end sz_restore_rqsns */


#endif mips

/************************************************************************
 *									*
 * sz_cdb_length() - Calculate the Command Descriptor Block length.	*
 *									*
 * Description:								*
 *	This function is used to determine the SCSI CDB length.  This	*
 * is done by checking the command group code.  The command sepcified	*
 * is expected to be the actual SCSI command byte, not a psuedo command	*
 * byte.  There should be tables for vendor specific commands, since	*
 * there is no way of determining the length of these commands.		*
 *									*
 * Inputs:	cmd = The SCSI command.					*
 *		targid = The SCSI Target ID.				*
 *									*
 * Return Value:							*
 *		Returns the CDB length.					*
 *									*
 ************************************************************************/
int
sz_cdb_length (cmd, targid)
register u_char cmd;
int targid;
{
	register int count = 0;

	/*
	 * Calculate the size of the SCSI command.
	 */
	switch (cmd & SCSI_GROUP_MASK) {

	    case SCSI_GROUP_0:
		count = 6;		/* 6 byte CDB. */
		break;

	    case SCSI_GROUP_1:
	    case SCSI_GROUP_2:
		count = 10;		/* 10 byte CDB. */
		break;

	    case SCSI_GROUP_5:
		count = 12;		/* 12 byte CDB. */
		break;

	    case SCSI_GROUP_3:
	    case SCSI_GROUP_4:
		PRINTD (targid, SCSID_CMD_EXP,
		("sz_cdb_length: Reserved group code for cmd 0x%x\n", cmd));
		count = 6;		/* Reserved group. */
		break;

	    case SCSI_GROUP_6:
	    case SCSI_GROUP_7:
		PRINTD (targid, SCSID_CMD_EXP,
	    ("sz_cdb_length: Vendor unique group code for cmd 0x%x\n", cmd));
		count = 10;		/* Vendor unique. */
		break;
	}
	PRINTD (targid, SCSID_CMD_EXP,
		("sz_cdb_length: Returning CDB length of %d\n", count));
	return (count);
}
