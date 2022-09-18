#ifndef	lint 
static char *sccsid = "@(#)mscp_tape.c	4.5    (ULTRIX)        12/20/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987 - 1990 by                    *
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
 *		Tape Class Driver
 *
 *   Abstract:	This module contains the 
 *		routines specific to the tape
 *		variant of MSCP.
 *
 *   Author:	Robin Lewis	3/2/1987
 *
 *   History:
 *
 *   08-Dec-1990	Tom Tierney
 *	Fixed recovery problem: removed checks for recovery state in
 *	tmscp_availcm.  In this way, available commands will be issued
 *	to units failing recovery and available end message processing
 *	will mark the unit appropriately and then continue recovery
 *	processing.  The mscp_restart_next routine should only be called
 *	after all previously online units had attempted recovery.
 *
 *	Also updated get unit status recovery table so that an online
 *	error (EV_ONLERROR) will cause available processing to occur and
 *	updated online end message processing to return EV_ONLERROR 
 *	when online fails during the unit recovery process. 
 * 
 *    4-Dec-1990	Brian Nadeau
 *	Add a return to tmscpioctl so lint will not complain.
 *
 *   26-Nov-1990	Brian Nadeau
 *	Ignore mscp_byte_cnt value when a serious exception occurs.
 *	Some controllers like the TQK50 fail to zero it after the
 *	exception.
 *
 *   05-Nov-1990	Tom Tierney
 *	Corrected transferem routine to copy byte count from the
 *	response packet's byte count field for all transfer commands
 *	except for "access" commands which get their byte count from
 *	the end packet's tape record field.
 *
 *   18-Dec-1989	Tim Burke
 *	Increase the timeout period of reposition commands from 20 minutes
 *	to 60 minutes.
 *
 *   06-Nov-1989	Tim Burke
 *	Set the exclusive access modifier in the setunitcm routine.
 *
 *   11-Sep-1989	Tim Burke
 *	Provide support for the following TA90 densities:
 *	MSCP_TF_NDCP, MSCP_TF_ENHD, and MSCP_TF_EDCP.
 *
 *   23-Aug-1989	Tim Burke
 *	Software workaround to a TU81+ microcode bug.  Do not allow the TU81
 *	to use caching because the command issued after the flush command will
 *	hang in the drive.
 *
 *   18-Aug-1989	Tim Burke
 *	In tmscp_setunitcm() only specify a tape format if positioned at
 *	BOT.  This prevents the command from being rejected with an invalid
 *	command, invalid format subcode.
 *
 *   01-Aug-1989	Tim Burke
 *	Fixed assignment of the mt_dsreg in mtiocget to properly contain the
 *	tms_flags field in the upper 8 bits of the field.
 *
 *   21-Jul-1989	David E. Eiche		DEE0070
 *	Changed various field names to conform to the T/MSCP preferred
 *	mnemonics, eliminate multiple levels of qualification, and
 *	eliminate multiple names for the same field.
 *	
 *   20-Jun-1989	Tim Burke
 *	Added TQK7L support.  This consists of loader present bit and 
 *	logging of loader errors.  Changed debugging printf's to be run off
 *	levels as specified by the tmscpdebug variable.  Added the kernel
 *	variable tmscptestcache which when set forces the drive to always run
 *	in caching mode, this is intended for debug and analysis usage only.
 *
 *   05-May-1989	Tim Burke
 *	Merged 3.1 deltas into pu and isis pools.
 *	Bug fix:  In transfercm if a non-nbuf read is being done and the 
 *	previous read hit a tape mark, clear the serious exception to allow
 *	the next read to be done on the first record of the next file.
 *
 *   14-Apr-1989	Tim Burke
 *	Added caching support to allow the use of controller read-ahead and 
 *	write-behind caching.  Changes involve adding MTFLUSH mtop, checking
 *	cache status in close routine, and keeping track of when writes could
 *	be pending in the controller's write back cache.
 *
 *   15-Mar-1989	Tim Burke
 *	Changed splx( IPL_SCS ) to Splscs();
 *
 *   07-Mar-1989	Todd M. Katz		TMK0002
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   28-Feb-1989	Tim Burke
 *	Prevent panics in recovery code.  Previously a panic would occur if
 *	more than one unit was online and needs recovery.  The fix involved
 *	recycling the rspid in tmscp_recovnext, and changing the recovery
 *	state table entries associated with the EV_ONLDONEXT from tmscp_invevent
 *	to tmscp_onlinecm.  This change was done on states 3 and 4.
 *
 *   27-Feb-1989	Tim Burke
 *	Prevent panics in recovery code by properly returning from the 
 *	tmscp_availcm routine if the reposition to last known position command
 *	fails.  Return to prevent sending a message when no resources are
 *	allocated to do so.
 *
 *   02-Feb-1989	Tim Burke
 *	Clear the up->Tflags.tms_wait flag in the tmscp_setunitem routine to
 *	prevent hanging in the tmscpopen routine.  Add a check in tmscpopen to
 *	see if the unit goes available unexpectadly.  Add missing subcode of
 *	MSCP_SC_EXUSE to the offline status code in check_return.
 *
 *   05-Jan-1988	Tim Burke
 *	In the transferem routine setup rp->p1 with the resid count so that it
 *	can be stuffed back into the buf struct in mscp_dealloc_reqb.  Also in
 *	this routine do not clear the BOM flag if you are still at BOT (if the
 *	operation fails).
 * 
 *	Added the Time_Check macro to log an error if the end message does not
 *	come back within the propper time interval.
 *
 *	Initilize the tms_status field in the unit block which will cause ioctls
 *	to fail if the device goes offline after it is opened.
 *
 *	Clear the serious exception state in the MTFSR and MTBSR ioctls if a
 *	tape mark has been encountered.
 *
 *	Added MSCP_ST_LOADR case to the check_return() routine. 
 *
 *   20-Dec-1988	Tim Burke
 *	Change MTFSR and MTBSR to move tape records and not tape objects.  This
 *	is done to be consistent with other drivers which don't traverse tape
 *	marks with record movement commands.  Change to return the propper
 *	resid field in MTIOCGET.
 *
 *   03-Oct-1988	Tim Burke
 *	Added state table and command routine for the set unit command.
 *	In tmscpopen() if the device is already online at position 0,
 *	set the density.  This is a software workaround to the TU81+
 *	failing to clear the online status via the avail command.
 *
 *   29-Sep-1988	Tim Burke
 *	Don't clear tms_category_flags in the onlineinit routine.  This would
 *	cause the tape density returned in DEVIOCGET to be wrong if the tape
 *	wasn't sitting at BOT.
 *
 *   14-Sep-1988	Tim Burke
 *	Clear tape written flag when a reposition command has changed the
 *	present position.  This prevents the close routine from improperly 
 * 	laying down tape marks in the wrong place.
 *
 *   17-Aug-1988	David E. Eiche		DEE0051
 *	Change state definitions to use ST_CMN_INITIAL so that
 *	recovery can rely on the initial state in any state table
 *	to be the same.  Also change unused events in the recovery
 *	state table to dispatch to the invalid event routine.
 *
 *   10-Aug-1988	David E. Eiche		DEE0050
 *	Change routines tmscp_recov_init and tmscp_recov_next
 *	to store the tape unit position only after finding a
 *	tape unit in the appropriate state.
 *
 *   05-Aug-1988	Tim Burke
 *	Maintain one copy of the online/offline status instead of using
 *	up->tms_flags with DEV_OFFLINE and up->flags.online; only keep 
 *	up->flags.online current and setup tms_flags in the ioctl routines
 *	as needed.
 *
 *   18-Jul-1988	Tim Burke
 *	In the open routine, derive the online status directly from the
 *	get unit status command.  Set inuse early in open routine to avoid
 *	race condition.  Allow for handling of non-EOT serious exceptions.
 *
 *   17-Jul-1988	David E. Eiche		DEE0045
 *	Change get device information ioctl to get model name from
 *	the connection block.
 *
 *   23-Jun-1988	Tim Burke
 *	Allow MTCLX and MTCLS ioctls by SU only.  Don't zero out tape
 *	attributes in the end message.
 *
 *   09-Jun-1988	Robin
 *	Added code that:stopped panic by setting a pointer value before it
 *	is used; returnes from ioctl calls with the correct status; sets up
 *	the hardware cache the correct way.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   20-Apr-1988	Tim Burke
 *	Setup residual count on partial writes in tmscp_transferem.
 *
 *   08-Apr-1988	Robin
 *	added some debug prints around tape density code.
 *
 *   04-Mar-1988	Robin
 *	In checking returns the write lock condition was set but it was only
 *	reset in close.  Now I reset the write lock flags and then reset them
 *	if the unit is still write locked.
 *
 *   01-Mar-1988	Robin
 *	We have a bug in the TU81+ which causes the path to crash if we
 *	issue two available commands and then do a get_command-status
 *	on the second.  The command status returned is zero (unknown command)
 *	and thats a problem.  To work around the problem I now must wait for
 *	the tape to rewind before letting close finish.
 *
 *   22-Feb-1988	Robin
 *	Changes the open routine to wait for the get unit status to finish.
 *	also changed the ioctl calls to get a rspid to check the return
 *	values.
 *
 *	The connection recovery code was added to bring a unit back online
 *	and reposition to a know position on the tape; then the outstanding
 *	I/O requests are re-issued.  Everything continues as before after
 *	the conection is recovered.
 *
 *
 *   02-Feb-1988	David E. Eiche		DEE0011
 *	Change tmscpstrategy to sleep if the request block is not
 *	available immediately.  Also remove code that initialized
 *	unused request block wait queue.
 *
 *   27-Jan-1988	Robin
 *	Removed the initialization routine calls in the open routine.  This
 *	will be done is start_sysap and mscp_systm_poll will break if we
 *	continue to do this.
 *
 *   15-Jan-1988	Todd M. Katz		TMK0001
 *	Include new header file ../vaxmsi/msisysap.h.
 *
 *   22-DEC-1987	Robin
 *	added B_error debug prints and fixed the DEVIOCGET ioctl
 *	DEV_CSE flag, it was set in the wrong word.
 */


/* Libraries and Include Files.
 */
#include	"../h/types.h"
#include	"../h/time.h"
#include	"../h/param.h"
#include	"../h/buf.h"
#include	"../h/errno.h"
#include	"../h/ioctl.h"
#include	"../h/devio.h"
#include	"../h/mtio.h"
#include	"../h/file.h"
#include	"../h/dump.h"
#include	"../h/systm.h"
#include	"../fs/ufs/fs.h"
#include	"../h/errlog.h"
#include	"../io/scs/sca.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include        "../io/msi/msisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/uba/ubavar.h"
#include	"../io/sysap/sysap.h"
#include	"../io/sysap/mscp_msg.h"
#include	"../io/sysap/mscp_defs.h"

/*
 * Caching control variable.  Setting this variable will always cause caching
 * to be used on this driver reguardless of calls to mtcache/mtnocache.  This
 * could prove useful at determining theoretical utility performance if they
 * utilized caching support.  Note that this should be used for debugging
 * and analysis because it opens a window of data integrity for utilities which
 * do not use mtflush; here there may be no indication of cache data loss to
 * the utility.
 */
int 	tmscptestcache = 0;
/* Debugging variables.  This value determines the debugging output level.
 * the larger the number, the more output.
 */
int 	tmscpdebug = 0;

#define TMSCPDEBUG
#ifdef TMSCPDEBUG

/*
 * Provide various levels of debugging printouts based on the value of
 * tmscpdebug.
 */
#define Dprint1 if ( tmscpdebug )      printf		/* Highest priority */
#define Dprint2 if ( tmscpdebug > 1 )  printf
#define Dprint3 if ( tmscpdebug > 2 )  printf
#define Dprint4 if ( tmscpdebug > 3 )  printf
#define Dprint5 if ( tmscpdebug > 4 )  printf
#define Dprint6 if ( tmscpdebug > 5 )  printf
#define Dprint7 if ( tmscpdebug > 6 )  printf
#define Dprint8 if ( tmscpdebug > 7 )  printf		/* Lowest priority */

/*
 * This macro is used to see if the timer has expired before the bit that
 * we are waiting for has cleared.  Typically the driver is waiting for an
 * end message for a particular command.  To prevent hanging you only wait
 * so long before giving up.
 */
#define Time_Check( Wait_For, Retry_Count, Error_String) { 		\
	if ((Wait_For) && ((Retry_Count) < 0)) { 			\
		mprintf("tmscp driver error: no end message for %s\n", 	\
			(Error_String));				\
	}}								

#else
#define Dprint1 ;
#define Dprint2 ;
#define Dprint3 ;
#define Dprint4 ;
#define Dprint5 ;
#define Dprint6 ;
#define Dprint7 ;
#define Dprint8 ;
#define Time_Check ;
#endif

/*
 * Evaluates to 1 if the unit in question is a TU81 drive.  This is being
 * done to provide software workarounds to hardware problems, hack, hack.
 */
#define IS_TU81_DRIVE( drive_model_name ) 				\
	(strncmp( drive_model_name, DEV_TU81, strlen(drive_model_name)) == 0)

/*
 * There have been cases where no end messages appear for outstanding commands.
 * As a result the utility program would end up hung in an unkillable state.
 * To avoid this problem (which shouldn't happen in the first place) impose
 * timers on how long to wait for the end message to return.
 *
 * Give reposition type commands a long time to complete.  On the TK70 a
 * forward skip command could take up to 57 minutes to complete (WOW).  This
 * is calculated as (600feet * 48tracks * 12inches) / (100ips * 60sec) = 57min.
 * Actual timing reveals that using 60 minutes is not enough to forward skip
 * file from the beginning to the last tape mark at the end of the tape.  So
 * use a value of 70 minutes to allow the needed time.
 */
#define RETRY_COUNT 		1200		/* 20 minutes */
#define RETRY_REPOS_COUNT 	4200		/* 70 minutes */

/* set up one variable to hold the position where we may have been is the
 * connection is lost.  This will allow us to move back to that position
 * and continue from there.
 */

/* External Variables and Routines.
 */
extern	void		mscp_datagram();
extern	void		mscp_dealloc_all();
extern	void		mscp_restart_next();
extern	REQB		*mscp_alloc_reqb();
extern	u_long		mscp_alloc_rspid();
extern	void		mscp_control();
extern	u_long		tmscp_invevent();
extern	u_long		mscp_alloc_msg();
extern	void		mscp_common_init();
extern	void		mscp_dealloc_reqb();
extern	void		mscp_message();
extern	u_long		mscp_map_buffer();
extern	u_long		mscp_onlgtuntem();
extern	u_long		mscp_recycle_rspid();
extern	u_long		mscp_send_msg();
extern  u_long		scs_restart();
extern  u_long		scs_crsh_path();
extern	void		mscp_system_poll();
extern	void		mscp_timer();
extern	CLASSB		tmscp_classb;
extern	RSPID_TBL	mscp_rspid_tbl[];
extern	LISTHD		mscp_rspid_lh;
extern	int		hz;
extern	int		lbolt;
extern	int		wakeup();
extern	UNITB		*tmscp_unit_tbl[];
	u_long		tmscp_onlineinit();
	u_long		tmscp_onlineem();
	u_long		tmscp_onlinecm();
	u_long		tmscp_onlgtuntem();
	u_long		tmscp_abortem();
	u_long		tmscp_abortcm();
	u_long		tmscp_availem();
	u_long		tmscp_availcm();
	u_long		tmscp_eraseem();
	u_long		tmscp_erasecm();
	u_long		tmscp_flushcm();
	u_long		tmscp_flushem();
	u_long		tmscp_gtuntcm();
	u_long		tmscp_gtuntem();
 	u_long		gtuntem_body();
	u_long		tmscp_invevent();
	u_long		tmscp_datagram();
 	u_long		tmscp_recovinit();
 	u_long		tmscp_recovnext();
	u_long		tmscp_recover_loc();
 	u_long		tmscp_reposem();
 	u_long		tmscp_reposcm();
 	u_long		tmscp_repreccm();
	u_long		tmscp_setunitcm();
	u_long		tmscp_setunitem();
	u_long		tmscp_transferem();
	u_long		tmscp_transfercm();
	u_long		tmscp_writetmem();
	u_long		tmscp_writetmcm();
	void		tmscp_init_driver();



/* ABORT COMMAND STATES */
#define ST_ABO_INITIAL		ST_CMN_INITIAL

/* UNIT AVAILABLE STATES. */
#define ST_AV_T_INITIAL		ST_CMN_INITIAL

/* UNIT DUMP STATES. */
#define ST_DU_T_INITIAL		ST_CMN_INITIAL

/* ERASE TAPE STATES */
#define ST_ERA_INITIAL		ST_CMN_INITIAL

/* FLUSH CACHE STATES */
#define ST_FLU_INITIAL		ST_CMN_INITIAL

/* TAPE DATAGRAM PROCESSING STATES */
#define TAPE_DATAGRAM		ST_CMN_INITIAL

/* GET UNIT STATUS COMMAND STATES */
#define ST_GTU_INITIAL		ST_CMN_INITIAL

/* TMSCP ON LINE STATES */
#define ST_ON_T_INITIAL		ST_CMN_INITIAL
#define ST_ON_T_GTUNT		1
#define ST_ON_T_STUNT		2
#define ST_ON_T_AVAIL		3

/* TMSCP RECOVERY STATES */
#define TAPE_RECOVERY_INITIAL	ST_CMN_INITIAL
#define TAPE_RECOVERY_GTUNT	1
#define TAPE_RECOVERY_STUNT	2
#define TAPE_RECOVERY_REPOS	3
#define TAPE_RECOVERY_AVAIL	4

/* REPOSITION COMMAND STATES */
#define ST_RPO_INITIAL		ST_CMN_INITIAL

/* WRITE TAPE MARK STATES */
#define ST_WTM_INITIAL		ST_CMN_INITIAL

/* Bring unit online states.
 */

STATE tmscp_onl_states[] = {

    /* Unit online initial state.
     */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_ON_T_INITIAL,	tmscp_onlineinit },		/* EV_INITIAL	      */
    { ST_ON_T_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_ON_T_INITIAL,	tmscp_onlinecm },		/* EV_MSGBUF	      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_T_GTUNT,	tmscp_onlineem },		/* EV_ENDMSG	      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/* 		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/* 		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/* 		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/* 		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ON_T_INITIAL,	tmscp_invevent },		/*		      */

    /* Unit online get unit status message proceesing
     */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/* EV_NULL	      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/* EV_INITIAL	      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/* EV_RSPID	      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/* EV_MSGBUF	      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_T_STUNT,	tmscp_onlgtuntem },		/* EV_ENDMSG	      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_GTUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_availcm },		/* EV_ONLERROR	      */

    /* Unit online set unit characteristics message proceesing
     */
    { ST_ON_T_STUNT,	tmscp_invevent },		/* EV_NULL	      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/* EV_INITIAL	      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/* EV_RSPID	      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/* EV_MSGBUF	      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_T_STUNT,	tmscp_setunitem },		/* EV_ENDMSG	      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_STUNT,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_availcm },		/* EV_ONLERROR	      */

    /* Unit online available end message processing.
     */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/* EV_INITIAL	      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/* EV_RSPID	      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/* EV_MSGBUF	      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_T_AVAIL,	tmscp_availem },		/* EV_ENDMSG	      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */
    { ST_ON_T_AVAIL,	tmscp_invevent },		/*		      */

};

/* unit recovery states.
 *
 * This state machine will only be entered by the connection
 * recovery code that is in common with the mscp disk driver.
 * The address of this state table will be loaded in the connection
 * block, and if the conection needs to be re-established; then this
 * is entered after the conection is re-made.  This will bring the
 * unit back online and reposition the tape at the point where the last
 * successful read/write was done.  When the state machine exits the
 * outstanding I/O (what ever) will be done as usual.  If the recovery
 * fails the I/O will then fail normally because the the unit will be offline.
 */

STATE tmscp_recovery_states[] = {

    /* Unit recovery initial state.
     */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/* EV_NULL	      */
    { TAPE_RECOVERY_INITIAL,	tmscp_recovinit },	/* EV_INITIAL	      */
    { TAPE_RECOVERY_INITIAL,	mscp_alloc_msg },	/* EV_RSPID	      */
    { TAPE_RECOVERY_INITIAL,	tmscp_onlinecm },	/* EV_MSGBUF	      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/* EV_MAPPING	      */
    { TAPE_RECOVERY_GTUNT,	tmscp_onlineem },	/* EV_ENDMSG	      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/* EV_TIMEOUT	      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/* 		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/* 		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/* 		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/* 		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_invevent },	/*		      */

    /* Unit recovery get unit status message processing
     */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/* EV_NULL	      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/* EV_INITIAL	      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/* EV_RSPID	      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/* EV_MSGBUF	      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/* EV_MAPPING	      */
    { TAPE_RECOVERY_STUNT,	tmscp_onlgtuntem },	/* EV_ENDMSG	      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/* EV_TIMEOUT	      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_GTUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_AVAIL,	tmscp_availcm },	/* EV_ONLERROR	      */

    /* Unit recovery set unit characteristics message processing
     */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/* EV_NULL	      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/* EV_INITIAL	      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/* EV_RSPID	      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/* EV_MSGBUF	      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/* EV_MAPPING	      */
    { TAPE_RECOVERY_REPOS,	tmscp_recover_loc },	/* EV_ENDMSG	      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/* EV_TIMEOUT	      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_STUNT,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_AVAIL,	tmscp_availcm },	/* EV_ONLERROR	      */

    /* Unit recovery reposition message processing
     */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/* EV_NULL	      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/* EV_INITIAL	      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/* EV_RSPID	      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/* EV_MSGBUF	      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/* EV_MAPPING	      */
    { TAPE_RECOVERY_REPOS,	tmscp_reposem },	/* EV_ENDMSG	      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/* EV_TIMEOUT	      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_onlinecm },	/* EV_ONLDONEXT	      */
    { TAPE_RECOVERY_REPOS,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_REPOS,	tmscp_recovnext },	/* EV_ONLCOMPLETE     */
    { TAPE_RECOVERY_AVAIL,	tmscp_availcm },	/* EV_ONLERROR	      */

    /* Unit recovery available end message processing.
     */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/* EV_NULL	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/* EV_INITIAL	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/* EV_RSPID	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/* EV_MSGBUF	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/* EV_MAPPING	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_availem },	/* EV_ENDMSG	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/* EV_TIMEOUT	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_INITIAL,	tmscp_onlinecm },	/* EV_ONLDONEXT	      */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/*		      */
    { TAPE_RECOVERY_AVAIL,	tmscp_recovnext },	/* EV_ONLCOMPLETE     */
    { TAPE_RECOVERY_AVAIL,	tmscp_invevent },	/* EV_ONLERROR	      */

};


/* Make unit available states.
 */
STATE tmscp_avl_states[] = {

    /* Unit available initial state.
     */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_AV_T_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_AV_T_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_AV_T_INITIAL,	tmscp_availcm },		/* EV_MSGBUF	      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_AV_T_INITIAL,	tmscp_availem },		/* EV_ENDMSG	      */
    { ST_AV_T_INITIAL,	tmscp_invevent},			/* EV_TIMEOUT	      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
    { ST_AV_T_INITIAL,	tmscp_invevent },		/*		      */
};

/* Abort running command states.
 */
STATE tmscp_abo_states[] = {

    /* Abort long running command initial state.
     */
    { ST_ABO_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_ABO_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_ABO_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_ABO_INITIAL,	tmscp_abortcm },		/* EV_MSGBUF	      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_ABO_INITIAL,	tmscp_abortem },		/* EV_ENDMSG	      */
    { ST_ABO_INITIAL,	tmscp_invevent},			/* EV_TIMEOUT	      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ABO_INITIAL,	tmscp_invevent },		/*		      */
};

/* Process a datagram message.
 */
STATE tmscp_datagram_states[] = {

    /* Process a datagram message.
     */
    { TAPE_DATAGRAM,	tmscp_invevent },		/* EV_NULL	      */
    { TAPE_DATAGRAM,	tmscp_datagram },		/* EV_INITIAL	      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/* 		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/* 		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/* 		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/* 		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/* 		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
    { TAPE_DATAGRAM,	tmscp_invevent },		/*		      */
};

/* Write tape mark command states.
 */
STATE tmscp_wtm_states[] = {

    /* Wriet tape mark initial state.
     */
    { ST_WTM_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_WTM_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_WTM_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_WTM_INITIAL,	tmscp_writetmcm },		/* EV_MSGBUF	      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_WTM_INITIAL,	tmscp_writetmem },		/* EV_ENDMSG	      */
    { ST_WTM_INITIAL,	tmscp_invevent},			/* EV_TIMEOUT	      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
    { ST_WTM_INITIAL,	tmscp_invevent },		/*		      */
};


/* Reposition tape command states.
 * Repositions files (tape marks), tape objects, nowhere.
 */
STATE tmscp_rpo_states[] = {

    /* Reposition tape initial state.
     */
    { ST_RPO_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_RPO_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_RPO_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_RPO_INITIAL,	tmscp_reposcm },		/* EV_MSGBUF	      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_RPO_INITIAL,	tmscp_reposem },		/* EV_ENDMSG	      */
    { ST_RPO_INITIAL,	tmscp_invevent},			/* EV_TIMEOUT	      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
};


/* Reposition tape records command states.
 * Reposition tape records only.
 */
STATE tmscp_rec_states[] = {

    /* Reposition tape initial state.
     */
    { ST_RPO_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_RPO_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_RPO_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_RPO_INITIAL,	tmscp_repreccm },		/* EV_MSGBUF	      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_RPO_INITIAL,	tmscp_reposem },		/* EV_ENDMSG	      */
    { ST_RPO_INITIAL,	tmscp_invevent},			/* EV_TIMEOUT	      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
    { ST_RPO_INITIAL,	tmscp_invevent },		/*		      */
};

/* Get unit status command states.
 */
STATE tmscp_gtu_states[] = {

    /* Reposition tape initial state.
     */
    { ST_GTU_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_GTU_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_GTU_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_GTU_INITIAL,	tmscp_gtuntcm },		/* EV_MSGBUF	      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_GTU_INITIAL,	tmscp_gtuntem },		/* EV_ENDMSG	      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/* EV_TIMEOUT	      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_GTU_INITIAL,	tmscp_invevent },		/*		      */
};


/* Set unit characteristics command states.
 */
STATE tmscp_stu_states[] = {

    /* Set characteristics initial state.
     */
    { ST_STU_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_STU_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_STU_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_STU_INITIAL,	tmscp_setunitcm },		/* EV_MSGBUF	      */
    { ST_STU_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_STU_INITIAL,	tmscp_setunitem },		/* EV_ENDMSG	      */
    { ST_STU_INITIAL,	tmscp_invevent },		/* EV_TIMEOUT	      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_STU_INITIAL,	tmscp_invevent },		/*		      */
};


/* Erase tape states.
 */
STATE tmscp_era_states[] = {

    /* Erase tape initial state.
     */
    { ST_ERA_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_ERA_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_ERA_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_ERA_INITIAL,	tmscp_erasecm },		/* EV_MSGBUF	      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_ERA_INITIAL,	tmscp_eraseem },		/* EV_ENDMSG	      */
    { ST_ERA_INITIAL,	tmscp_invevent},			/* EV_TIMEOUT	      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
    { ST_ERA_INITIAL,	tmscp_invevent },		/*		      */
};

/* Data transfer states.
 */
STATE tmscp_xfr_states[] = {

    /* Unit online initial state.
     */
    { ST_XF_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_XF_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_XF_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_XF_INITIAL,	mscp_map_buffer },		/* EV_MSGBUF	      */
    { ST_XF_INITIAL,	tmscp_transfercm },		/* EV_MAPPING	      */
    { ST_XF_INITIAL,	tmscp_transferem },		/* EV_ENDMSG	      */
    { ST_XF_INITIAL,	tmscp_invevent },		/* EV_TIMEOUT	      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
    { ST_XF_INITIAL,	tmscp_invevent },		/*		      */
};

/* Flush controller's write-back cache command state.
 */
STATE tmscp_flu_states[] = {

    /* flush command initial state.
     */
    { ST_FLU_INITIAL,	tmscp_invevent },		/* EV_NULL	      */
    { ST_FLU_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_FLU_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_FLU_INITIAL,	tmscp_flushcm },		/* EV_MSGBUF	      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/* EV_MAPPING	      */
    { ST_FLU_INITIAL,	tmscp_flushem },		/* EV_ENDMSG	      */
    { ST_FLU_INITIAL,	tmscp_invevent},		/* EV_TIMEOUT	      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
    { ST_FLU_INITIAL,	tmscp_invevent },		/*		      */
};
/*
 *
 *   Name:	tmscpopen	- Open Tape Unit
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

int 
tmscpopen(dev, flag)
    dev_t	dev;
    int		flag;

{

	UNITB	*up;
	REQB	*rp;
	int	retries;

	Dprint6("tmscpopen\n");

	/* If there is no known unit corresponding to the device
	 * number, return an error.
	 */
	if(( up = Dev_to_Tunitb( dev )) == NULL ) {
		Dprint8("open: return ENXIO unknown unit\n");
		return( ENXIO );
	}

	up->tms_category_flags &= ~DEV_RWDING;

	/* If the unit is in use fail as the drive is in use already.
	 */
	if(up->Tflags.tms_inuse){
		Dprint8("open: return ENXIO unit in use\n");
		return(ENXIO);
	}
	/*
	 * Mark the unit in use so opens will fail for the next user
	 * that wants the tape drive.  
	 */
	up->Tflags.tms_inuse = 1;

	/* Set drive type.
	 */
	up->sel = SEL( dev );

	/* Do a get unit status insure that the states have not changed.
	 * Clear online so that it's status will be set by the get unit status.
	 */
	up->flags.online = 0;
	up->Tflags.tms_wait = 1;
	rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_gtu_states,(u_long) 0, (u_long) 0 );

	retries = RETRY_COUNT;
	while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
		timeout( wakeup, ( caddr_t )rp, hz );
		( void )sleep(( caddr_t )rp, PSWP+1 );
		untimeout(wakeup,(caddr_t)rp);   /* to be sure*/
	}
	Time_Check(up->Tflags.tms_wait, retries, "gtu in open");
	up->Tflags.tms_wait = 0;
	/* If the unit is already online.  This would be the case for a 
	 * no-rewind device which is already out on the tape.
	 * Issue a reposition to clear outstanding serious exceptions which 
	 * would be set by encountering a tape mark when a previous open to
	 * the no-rewind device has read to the tape mark.  This allows for
	 * a transition into the next file.  If the device isn't online the
	 * serious exception would be explicitly cleared by the online states.
	 * return success.
	 *
	 * The tmscp_rpo_states clears flags and sets up up->tms_position
	 * to a known value.
	 */
	if( up->flags.online ) {
		Dprint8("OPEN unit already on line\n");
		up->Tflags.tms_wait = 1;
		rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_OBJCT, (u_long) 0 );
		retries = RETRY_COUNT;
		while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
			timeout( wakeup, ( caddr_t )rp, hz );
			( void )sleep(( caddr_t )rp, PSWP+1 );
			untimeout(wakeup, ( caddr_t )rp, hz );
		}
		Time_Check(up->Tflags.tms_wait, retries, "rpo in open");
		up->Tflags.tms_wait = 0;

		/* There have been cases on the TU81 where an availcm failed
		 * to bring the unit into an available state.  As a result the
		 * state is online which would prevent setting of the tape
		 * density which was otherwise done by the onlineinit.
		 * To work around this problem, check to see if we are at BOT.
		 * If so then set the tape density to the value determined by
		 * the previous get unit status.
		 */
		if (up->tms_position == 0) {
			Dprint8("OPEN, online at BOT, set attributes\n");
			up->Tflags.tms_wait = 1;
			rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_stu_states, (u_long) 0, (u_long) 0 );
			retries = RETRY_COUNT;
			while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
				timeout( wakeup, ( caddr_t )rp, hz );
				( void )sleep(( caddr_t )rp, PSWP+1 );
				untimeout(wakeup, ( caddr_t )rp, hz );
			}
			Time_Check(up->Tflags.tms_wait, retries, "stu in open");
			up->Tflags.tms_wait = 0;
		}
		/*
		 * This is a check to see if the hardware went out from under
		 * us.  What could happen is that the initial get unit status
		 * stated that the unit is online (not available).  Since that
		 * time the controller has changed its mind and became 
		 * available.  If this happens then don't return.  This will
		 * cause an online sequence to be performed to try to recover.
		 * This failure has been observed on the TU81+ and the TBK70.
		 */
	        if ( up->tms_status != MSCP_ST_AVLBL) { 
			return( 0 );
		}
		Dprint8("tmscpopen: unit went available.\n");
	}

	/* Zero saved cmd_ref used by ABORT process.
	 */
	up->cmd_ref = 0;

	/* Insure the tape flags are zero
	 * clear_Sflags is used as quick way to zero out all the bitfields.
	 * this introduces a small window (for SMP) where another open could 
	 * come in between these two instructions.
	 */
	up->clear_Sflags = 0;
	up->Tflags.tms_inuse = 1;
	up->tms_flags = 0;

	/* Start an online sequence
	 */
	rp = (REQB *) mscp_alloc_reqb( up,( struct buf * ) NULL, tmscp_onl_states, (u_long) 0, (u_long) 0 );

	retries = RETRY_COUNT;
	while( !up->flags.online && up->flags.online_ip && --retries >= 0 ) {
		timeout( wakeup, ( caddr_t )rp, hz );
		( void )sleep(( caddr_t )rp, PSWP+1 );
		untimeout(wakeup,(caddr_t)rp);   /* to be sure*/
	}
	/*
	 * If the end message wasn't received before the time expired then
	 * log this as a driver problem.
	 */
	if (!up->flags.online && up->flags.online_ip && (retries < 0)) {
		mprintf("tmscp driver error: online timed out\n");
	}

	if( !up->flags.online ){
		/* Unit did not come online.
		 * If the NO DELAY flag is not set return an error to the user.
		 * otherwise NO DELAY says return success even if it failed.
		 */
		if( !(flag & FNDELAY)) {
			Dprint8("open: ENXIO return, dev did not come online\n");
			up->Tflags.tms_inuse = 0;
			return( EIO );
		}
		/*
		 * In this case the unit was opened with the FNDELAY flag set
		 * and the unit failed to come online.  Hopefully no user
		 * programs really do anything with the tape when it is opened
		 * with the FNDELAY flag because they'd have no way of 
		 * knowing if the tape was brought online correctly.
		 */
		else {
			Dprint8("open: FNDELAY, dev did not come online\n");
		}
	}

	return( 0 );
}


/*
 *
 *   Name:	tmscpclose	- Close Tape Unit
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

int 
tmscpclose(dev, flag)

    dev_t	dev;
    int		flag;

{

    UNITB	*up;
    REQB	*rp;
    int		retries;


	Dprint6("tmscp_close\n");
    /* If there is no known unit corresponding to the device
     * number, return an error.
     */
	if(( up = Dev_to_Tunitb( dev )) == NULL ) {
		Dprint8("close: ENXIO return, unit unknown\n");
		return( ENXIO );
	}

	/*
	 * If caching is being used, the utility should have issued a flush
	 * command before attempting to close the device to insure that all
	 * records in the controller's write back cache have been placed onto
	 * the physical media.  
	 */
	if (up->Tflags.tms_cach_on && up->Tflags.tms_cach_write) {
		mprintf("tms warning - close without cache flush\n");
		Dprint2("tms - close without cache flush\n");
		/*
		 * An irresponsible utility is being used.  Force a cache
		 * flush operation to gain status of the write back cache.
		 */
		up->Tflags.tms_wait = 1;
		rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_flu_states, 
			(u_long) 0, (u_long) 0 );
		retries = RETRY_COUNT;
		while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
			timeout( wakeup, ( caddr_t )rp, hz );
			( void )sleep(( caddr_t )rp, PSWP+1 );
			untimeout(wakeup, ( caddr_t )rp, hz );
		}
		Time_Check(up->Tflags.tms_wait, retries, "close cache flush");
		up->Tflags.tms_wait = 0;
              
		/*
		 * Check status of flush on controller's write-back cache.
		 */
		if ( up->tms_status == MSCP_ST_SUCC) {
			Dprint3("close: successful cache flush\n");
		}
		else {
			/*
			 * This is a serious condition!  The controller's
			 * write-back cache has failed to flush.  This 
			 * implies that some records written to the tape may
			 * not have been placed on the physical media.  Not 
			 * only that, by the writes will have received a
			 * success end message if the records were successfuly
			 * placed in the controller's write back cache.
			 *
			 * The problem now is that there is really no way to
			 * notify the utility of this since it is not feisable
			 * to fail the close.  The only thing to do is to log
			 * this error.
			 *
			 * Any utility which enables caching is taking chances
			 * with data integrity if they don't do an explicit
			 * ioctl(MTFLUSH).
			 *
			 * At this point the unit should be in the serious 
			 * exception state.  Don't clear the serious exception
			 * because the unit is suspect and other tape operations
			 * should not be allowed until the exception is
			 * explicitly cleared.  If the "rewind device" is being
			 * used then the available command will clear the 
			 * serious exception.
			 *
			 * Clear tms_cach_write to allow the WTM's, REWIND and
			 * available commands to succeed.
			 */
			up->Tflags.tms_cach_write = 0;
			printf("tms - unit %d cache flush failure.\n",up->unit);
			printf("tms - unit %d potential data loss.\n",up->unit);
		}
	}

	/* If the tape unit is write enabled and the tape has been written
	 * on, write two (2) EOF's and then back space one (1) record.
	 * If the end of media (EOM) was detected, allow EOF's to be written
	 * but remember that tape is past EOM and the next command will fail.
	 */
	up->Tflags.tms_bufmark = 0;
	up->tms_flags &= ~DEV_CSE;
        if ( up->Tflags.tms_write == 1) {
        	up->Tflags.tms_write = 0;

		/* Write the first tape mark
		 */
		(void) mscp_alloc_reqb( up, NULL, tmscp_wtm_states, (u_long) 0, (u_long) 0 );

		/* Write the second tape mark
		 */
		(void) mscp_alloc_reqb( up, NULL, tmscp_wtm_states, (u_long) 0, (u_long) 0 );

		/* Now reposition back over the last tape mark 
		 */
		(void) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) (MSCP_MD_OBJCT | MSCP_MD_REVRS), (u_long) 1 );
	}
	up->Tflags.tms_write = 0;

	/* If the tape unit is a 
	 * no-rewind device then mark the unit available for other hosts.  The
	 * available mscp command will rewind the tape therefor the unit
	 * should not go available if the unit is not a rewind device. 
	 */
	if (up->sel & NO_REWIND) {
		/* Mark the unit NOT in use so opens will work for the next user
		 * that wants the tape drive.
		 */
		up->Tflags.tms_inuse = 0;
		return( 0);
	}
	

	/* Start the rewind.  The rewind is done
	 * this way because the availble command has caused the path to crash
	 * due to a hardware problem.  By doing the rewind this way we can wait
	 * for it to complete before allowing another process to get in the
	 * driver.
	*/
	up->Tflags.tms_wait = 1;
	rp = (REQB *)mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_REWND, (u_long) 0 );

	/* Here is where we wait for the rewind to finish.  We almost, if after
	 * 20 min. we give up, that way we will never hang the driver if a drive
	 * fails to go to reposition end message, paranoia I guess.
	 */
	retries = RETRY_COUNT;
	while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
		timeout( wakeup, ( caddr_t )rp, hz );
		( void )sleep(( caddr_t )rp, PSWP+1 );
		untimeout(wakeup, ( caddr_t )rp, hz );
	}
	Time_Check(up->Tflags.tms_wait, retries, "rew in close");
	up->Tflags.tms_wait = 0;


	/* Allocate a request block and make the unit available.  
	 */
	rp = (REQB *) mscp_alloc_reqb( up, ( struct buf * )NULL, tmscp_avl_states, (u_long) 0, (u_long) 0 );

	/* Wait for the unit to go available.  This may take a little time
	 * if it waits for a rewind.
	 */
	retries = RETRY_COUNT;
	while( up->Tflags.tms_inuse  && ( --retries >= 0 ) ) {
		timeout( wakeup, ( caddr_t )rp, hz );
		( void )sleep(( caddr_t )rp, PSWP+1 );
		untimeout(wakeup,(caddr_t)rp);   /* to be sure*/
	}
	Time_Check(up->Tflags.tms_inuse, retries, "avl in close");

	/* The unit went off line; clear error conditions to insure
	 * correct values when the unit comes back online.
	 */
	up->Tflags.tms_lost = 0;
	up->Tflags.tms_clserex = 0;
	up->Tflags.tms_serex = 0;
	up->Tflags.tms_inuse = 0;
	return( 0 );
}


/*
 *
 *   Name:	tmscpstrategy - rtn description
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

int 
tmscpstrategy( bp )

	struct buf 	*bp;

{

	register UNITB	*up;

	/* Check to make sure that the unit corresponding to the device number
	 * exists and is online.
	 */
	if(( up = Dev_to_Tunitb( bp->b_dev )) != ( UNITB * )NULL && 
	up->flags.online ) {
		/* In an effort to make tar work with nbuffered I/O and EOM
		 * this code fragment is placed here.  If the EOM handling 
		 * is enabled AND a clear serious exception has NOT 
		 * been done AND the EOM has been seen then return ENOSPC
		 * to the user.
		 */
		if ( (up->Tflags.tms_eom == 0) &&
		    !(up->tms_flags & DEV_CSE) &&
		     (up->tms_flags & DEV_EOM)) {
			bp->b_resid = bp->b_bcount;
			bp->b_error = ENOSPC;
			bp->b_flags |= B_ERROR;
			Dprint8("strategy: ENOSPC in buf\n");
			(void)iodone(bp);
			return;
		}

		/* Make sure that the request transfer size is within
		 * the limits of the controller.  This limit is known
		 * by "online" or "set unit char" commands.
		 */

		if ((bp->b_flags == B_WRITE) && 
		( up->tms_bcount < bp->b_bcount) ){
			Dprint8("strategy: I/O to big\n");
			Dprint8("strategy: OK count 0x%x request count 0x%x\n",up->tms_bcount,bp->b_bcount);
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			( void )iodone( bp );
			return;
		}
 
		/* If the action to be done is a READ and a tape mark
		 * has been seen return with an I/O error.  This is done
		 * because buffered I/O may have outstanding READS which 
		 * should fail.  If not n-buff I/O then allow the read to
		 * proceed to read in the first record of the next file.
		 * because buffered I/O may have outstanding READS which should fail.
		 */
		if( (up->Tflags.tms_bufmark) && 
		(bp->b_flags == B_READ) && 
		(bp->b_flags & B_RAWASYNC)) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			Dprint8("strategy: EIO 1 Berror\n");
			( void )iodone( bp );
			return;
		}
	
		/* Allocate a request block and start the data transfer.
		 */
		(void) mscp_alloc_reqb( up, bp, tmscp_xfr_states, (u_long) 0, (u_long) 0 );

		/* Unit number is out of range or unit is not online.
		 */ 
	} else {
		Dprint8("strategy:offline ENXIO in buf\n");
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		( void )iodone( bp );
	}

	return;
}


/*
 *
 *   Name:	tmscpread - 
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

int 
tmscpread( dev, uio )

	dev_t		dev;
	struct uio	*uio;

{

	register UNITB	*up;

	/* If there is no known unit corresponding to the device
	 * number, return an error.
	 */
	if(( up = Dev_to_Tunitb( dev )) == NULL ) {
		Dprint8("read: ENXIO return, unknown unit\n");
		return( ENXIO );
	}

	/* Invoke physio to fire off the strategy routine and return
	 * the resulting status.
	 */
	return( physio( tmscpstrategy, &up->rawbuf, dev, B_READ, minphys, uio ));
}


/*
 *
 *   Name:	tmscpwrite - 
 *
 *   Abstract:	Raw I/O read routine.
 *
 *   Inputs:	Device minor number and user i/o buffer
 *
 *   Outputs:	NONE
 *
 *
 *   Return	
 *   Values:
 */

int 
tmscpwrite( dev, uio )

	dev_t		dev;
	struct	uio 	*uio;

{

	register UNITB	*up;

	/* If there is no known unit corresponding to the device
	 * number, return an error.
	 */
	if(( up = Dev_to_Tunitb( dev )) == NULL ) {
		Dprint8("write: ENXIO return, unknown unit\n");
		return( ENXIO );
	}

	/* Invoke physio to fire off the strategy routine and return
	 * the resulting status.
	 */
	return( physio( tmscpstrategy, &up->rawbuf, dev, B_WRITE, minphys, uio ));
}


/*
 *
 *   Name:	tmscpioctl	- Process I/O Control Functions
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */


int
tmscpioctl(dev, cmd, data, flag)

    dev_t		dev;
    int			cmd;
    caddr_t		data;
    int			flag;

{

	register UNITB	*up;
	REQB		*rp;
	struct uba_device *ui;  /* ptr to the uba device structure */

	CONNB		*cp;
        struct mtop	*mtop;      /* mag tape cmd op to perform */
        struct mtget	*mtget;    /* mag tape struct to get info in */
        struct devget	*devget;  /* device struct for status info */
	int		unit;
	int		retries;
	int		s;

	Dprint6("tmscpioctl\n");

    /* If there is no known unit corresponding to the device
     * number, return an error.
     */
    if(( up = Dev_to_Tunitb( dev )) == NULL ) {
	Dprint8("ioctl: ENXIO return, unknown unit\n");
	return( ENXIO );
    }

    cp = up->connb;
    unit = up->unit;
    ui = up->ubdev;

    /* Set BOM if at position 0 for status ioctl's
     */
    if(up->tms_position == 0)
	up->tms_flags |= DEV_BOM;
    else
	up->tms_flags &= ~DEV_BOM;

    /* tms_status is used to determine the status of the requested ioctl
     * except for the case of MTIOCGET.  This means we need to init the field
     * in all cases except MTIOCGET.  For MTIOCGET we want to return the status
     * of the last command issued.
     */
    if (cmd != MTIOCGET)
                up->tms_status = ~(MSCP_ST_SUCC);

    switch( cmd ) {
        case MTIOCTOP:  /* tape operation */
	Dprint4("MTIOCTOP\n");
                mtop = (struct mtop *)data;
                switch (mtop->mt_op) {

/*		The ERASE and ABORT ioctls work but at this time there are
/*		no user level functions which call them
/*
/*		case MTERASE:	/* Issue tape erase.  It will erase from
/*				 * current tape position to physical
/*				 * end of tape.   The rewinds to BOT.
/*				 * Would use timeout of RETRY_REPOS_COUNT.
/*				 */
/*			Dprint4("MTERASE\n");
/*			/* Check to make sure that the unit 
/*			* is online.
/*			*/
/*			if( up->flags.online ) {
/*				(void) mscp_alloc_reqb( up, NULL, 
/*				    tmscp_era_states, (u_long) 0, (u_long) 0 );
/*			}
/*			break;
/*
/*
/*		case MTABORT:	/* Issue abort command.  It will abort the
/*				 * last long running tape command issued.
/*				 * NOTE: The present abort scheme is 
/*				 * insuficient if more than one command is
/*				 * outstanding or if the command intended to be
/*				 * aborted was not the most recent command.
/*				 */
/*			Dprint4("MTABORT\n");
/*			/* Check to make sure that the unit 
/*			* is online.
/*			*/
/*			if( up->flags.online ) {
/*
/*
/*
/*				/* Allocate a request block and start the data transfer.
/*				*/
/*				(void) mscp_alloc_reqb( up, NULL, tmscp_abo_states, (u_long) 0, (u_long) 0 );
/*			}
/*			break;
/*
 */
                case MTWEOF:	/* Write two EOF's and reposition back 1 
				 * this has the effect of writing a EOT
				 * but if a new file is put on the tape
				 * (assume no rewind) the second EOF is 
				 * over written making it a real EOF and
				 * not an EOT.
				 *
				 * Note: ts.c only writes one tape mark.  This
				 * makes behavior different if a reposition is
				 * done after this command.  If a close is done
				 * after MTWEOF then there will really be 3
				 * tape marks on the media.  This is a potential
				 * violation of ansi specs.  This routine never
				 * looks at the count parameter from the user.
				 */
			Dprint4("MTWEOF\n");
			/* Write the first tape mark
			 */
			up->Tflags.tms_wait = 1;
			rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_wtm_states, (u_long) 0, (u_long) 0 );
			retries = RETRY_COUNT;
			while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
				timeout( wakeup, ( caddr_t )rp, hz );
				( void )sleep(( caddr_t )rp, PSWP+1 );
				untimeout(wakeup, ( caddr_t )rp, hz );
			}
			Time_Check(up->Tflags.tms_wait, retries, "MTWEOF 1");
			up->Tflags.tms_wait = 0;
              
	                  /* Check the status for the command that was issued and return fail code
	                   * if the status is not success.
	                   */
	                  if ( up->tms_status != MSCP_ST_SUCC) 
	              		return(EIO);
	
			/* Write the second tape mark
			 * Initilize command status to prevent masking of error.
			 */
			up->Tflags.tms_wait = 1;
			up->tms_status = ~(MSCP_ST_SUCC);
			rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_wtm_states, (u_long) 0, (u_long) 0 );
			retries = RETRY_COUNT;
			while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
				timeout( wakeup, ( caddr_t )rp, hz );
				( void )sleep(( caddr_t )rp, PSWP+1 );
				untimeout(wakeup, ( caddr_t )rp, hz );
			}
			Time_Check(up->Tflags.tms_wait, retries, "MTWEOF 2");
			up->Tflags.tms_wait = 0;
              
	                /* Check the status for the command that was issued and return fail code
	                * if the status is not success.
	                */
	                if ( up->tms_status != MSCP_ST_SUCC) 
	              		return(EIO);
			up->Tflags.tms_write=0;
	
			/*
			 * Success of 2 consecutive wtm's will cause the
			 * controller to flush it's cache. Clear tms_cach_write
			 * to indicate that there are no write records pending
			 * in the controller's write cache.
			 */
			up->Tflags.tms_cach_write = 0;

			/* Now reposition back over the last tape mark 
			 * Initilize command status to prevent masking of error.
			 */
			up->Tflags.tms_wait = 1;
			up->tms_status = ~(MSCP_ST_SUCC);
			rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) (MSCP_MD_OBJCT | MSCP_MD_REVRS), (u_long) 1 );
			retries = RETRY_COUNT;
			while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
				timeout( wakeup, ( caddr_t )rp, hz );
				( void )sleep(( caddr_t )rp, PSWP+1 );
				untimeout(wakeup, ( caddr_t )rp, hz );
			}
			Time_Check(up->Tflags.tms_wait, retries, "MTWEOF 3");
			up->Tflags.tms_wait = 0;
              
	                /* Check the status for the command that was issued and return fail code
	                * if the status is not success.
	                */
	                if ( up->tms_status != MSCP_ST_SUCC) 
	              		return(EIO);
			else
				return(0);

                case MTFSF: 	/* Reposition command forward EOF count
			 	 */
			Dprint4("MTFSF\n");
			/* Check to make sure that the unit 
			 * is online.
			 */
			if( up->flags.online ) {
				/* Allocate a request block and start the data transfer.
				*/
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) 0, (u_long) mtop->mt_count );
				retries = RETRY_REPOS_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTFSF");
				up->Tflags.tms_wait = 0;
			}
              
	                /* Check the status for the command that was issued and return fail code
	                * if the status is not success.
	                */
	                if ( up->tms_status != MSCP_ST_SUCC) 
	              		return(EIO);
			else
				return(0);

		case MTBSF: 	/* Reposition command backward EOF count 
			 	 */
			Dprint4("MTBSF\n");

			/* Check to make sure that the unit 
			 * is online.
			 */
			if( up->flags.online ) {
				/* Allocate a request block and start the data transfer.
				*/
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_REVRS, (u_long) mtop->mt_count );
				retries = RETRY_REPOS_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTBSF");
				up->Tflags.tms_wait = 0;
			}
              
	                /* Check the status for the command that was issued and return fail code
	                * if the status is not success.
	                */
	                if ( up->tms_status != MSCP_ST_SUCC) 
	              		return(EIO);
			else
				return(0);

                case MTFSR:  	/* Reposition command forward count 
			 	 */
			Dprint4("MTFSR\n");
			/* Check to make sure that the unit 
			* is online.
			*/
			if( up->flags.online ) {
				/* Allocate a request block and start the data 
				 * transfer.
				 * To setup the reposition command to move
				 * RECORDS call the tmscp_rec_states instead
				 * of the tmscp_rpo_states.  This is the only
				 * way to tell in the "cm" routine that records
				 * and not tape OBJECTS or tape MARKS are to be
				 * skipped.  The previous driver moved tape
				 * objects; not true records.
				 */
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rec_states, (u_long) 0, (u_long) mtop->mt_count );
				retries = RETRY_REPOS_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTFSR 1");
				up->Tflags.tms_wait = 0;
			}
              
	                /* Check the status for the command that was issued 
			* and return fail code if the status is not success.
			* The status would not be success if a tape mark has 
			* been encountered (as an example).  Clear the 
			* associated serious exception if it has been raised.
			* Serious exception will be set if the reposition
			* record command hit a tape mark.
	                */
	                if ( up->tms_status != MSCP_ST_SUCC) {
				if (up->Tflags.tms_clserex) {
					/* Allocate a request block to
					* reposition nowhere to clear the 
					* serious exception.
					*/
					up->Tflags.tms_wait = 1;
					rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_OBJCT, (u_long) 0 );
					retries = RETRY_COUNT;
					while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					   timeout( wakeup, ( caddr_t )rp, hz );
					   ( void )sleep(( caddr_t )rp, PSWP+1);
					   untimeout(wakeup, ( caddr_t )rp, hz);
					}
					Time_Check(up->Tflags.tms_wait,retries,"MTFSR 2");
					up->Tflags.tms_wait = 0;
				}
	              		return(EIO);
			}
			else
				return(0);

		case MTBSR: 	/* Reposition command backward count 
			 	 */
			Dprint4("MTBSR\n");
			/* Check to make sure that the unit 
			* is online.
			*/
			if( up->flags.online ) {

				/* Allocate a request block and start the data 
				 * transfer.
				 * To setup the reposition command to move
				 * RECORDS call the tmscp_rec_states instead
				 * of the tmscp_rpo_states.  This is the only
				 * way to tell in the "cm" routine that records
				 * and not tape OBJECTS or tape MARKS are to be
				 * skipped.  The previous driver moved tape
				 * objects; not true records.
				 */
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rec_states, (u_long) (MSCP_MD_REVRS), (u_long) mtop->mt_count );
				retries = RETRY_REPOS_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTBSR 1");
				up->Tflags.tms_wait = 0;
			}

	                /* Check the status for the command that was issued 
			* and return fail code if the status is not success.
			* The status would not be success if a tape mark has 
			* been encountered (as an example).  Clear the 
			* associated serious exception if it has been raised.
			* Serious exception will be set if the reposition
			* record command hit a tape mark.
	                */
	                if ( up->tms_status != MSCP_ST_SUCC) {
				if (up->Tflags.tms_clserex) {
					/* Allocate a request block to
					* reposition nowhere to clear the 
					* serious exception.
					*/
					up->Tflags.tms_wait = 1;
					rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_OBJCT, (u_long) 0 );
					retries = RETRY_COUNT;
					while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					   timeout( wakeup, ( caddr_t )rp, hz );
					   ( void )sleep(( caddr_t )rp, PSWP+1);
					   untimeout(wakeup, ( caddr_t )rp, hz);
					}
					Time_Check(up->Tflags.tms_wait,retries,"MTBSR 2");
					up->Tflags.tms_wait = 0;
				}
	              		return(EIO);
			}
			else
				return(0);

#ifdef 0
/*                case MTFSO:  	/* Reposition command forward count. 
/*				 * Reposition tape OBJECTS.  A tape object
/*				 * includes both tape records and tape marks.
/*				 * The differnce between this and MTFSR is that
/*				 * tape marks can be traversed in MTFSO.  The
/*				 * key aspect is setting the MSCP_MD_OBJCT
/*				 * modifier.
/*			 	 */
/*			Dprint4("MTFSO\n");
/*			/* Check to make sure that the unit 
/*			* is online.
/*			*/
/*			if( up->flags.online ) {
/*				/* Allocate a request block and start the data transfer.
/*				*/
/*				up->Tflags.tms_wait = 1;
/*				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_OBJCT, (u_long) mtop->mt_count );
/*				retries = RETRY_REPOS_COUNT;
/*				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
/*					timeout( wakeup, ( caddr_t )rp, hz );
/*					( void )sleep(( caddr_t )rp, PSWP+1 );
/*					untimeout(wakeup, ( caddr_t )rp, hz );
/*				}
/*				Time_Check(up->Tflags.tms_wait,retries,"MTFSO");
/*				up->Tflags.tms_wait = 0;
/*			}
/*              
/*	                /* Check the status for the command that was issued and return fail code
/*	                * if the status is not success.
/*	                */
/*	                if ( up->tms_status != MSCP_ST_SUCC) 
/*	              		return(EIO);
/*			else
/*				return(0);
/*
/*		case MTBSO: 	/* Reposition command backward count.
/*				 * Reposition tape OBJECTS.  A tape object
/*				 * includes both tape records and tape marks.
/*				 * The differnce between this and MTBSR is that
/*				 * tape marks can be traversed in MTBSO.  The
/*				 * key aspect is setting the MSCP_MD_OBJCT
/*				 * modifier.
/*			 	 */
/*			Dprint4("MTBSO\n");
/*			/* Check to make sure that the unit 
/*			* is online.
/*			*/
/*			if( up->flags.online ) {
/*				/* Allocate a request block and start moving.
/*				*/
/*
/*				up->Tflags.tms_wait = 1;
/*				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) (MSCP_MD_REVRS | MSCP_MD_OBJCT), (u_long) mtop->mt_count );
/*				retries = RETRY_REPOS_COUNT;
/*				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
/*					timeout( wakeup, ( caddr_t )rp, hz );
/*					( void )sleep(( caddr_t )rp, PSWP+1 );
/*					untimeout(wakeup, ( caddr_t )rp, hz );
/*				}
/*				Time_Check(up->Tflags.tms_wait,retries,"MTBSO");
/*				up->Tflags.tms_wait = 0;
/*
/*	                /* Check the status for the command that was issued and return fail code
/*	                * if the status is not success.
/*	                */
/*	                if ( up->tms_status != MSCP_ST_SUCC) 
/*	              		return(EIO);
/*			else
/*				return(0);
/*
/*			}
/*
 */
#endif 0

                case MTCACHE:	/* Set unit charicteristics for cache. 
			 	 */
			Dprint4("MTCACHE\n");
			if ((up->Tflags.tms_cach) && (up->flags.online) &&
				(IS_TU81_DRIVE(cp->model_name) == 0)) {
				/*
				 * Only turn on caching if the unit supports it.
				 * To do this, enable write-behind caching and
				 * disable the suppress caching to enable
				 * read ahead.
				 *
				 * Don't change the status if the unit is not
				 * online.
				 *
				 * Don't allow caching on TU81's due to 
				 * microcode bugs which cause the flush command
				 * to wedge the drive (hack).
				 */
				Dprint8("Unit allows caching\n");
				if (up->Tflags.tms_cach_on == 0) {
					up->Tflags.tms_cach_on = 1;
                                	/*
                                	 * Turn on controller caching.  This
                                	 * causes read-ahead and write-behind
                                	 * caching.
                                 	 */
                                	Dprint8("Enable cache.\n");
                                	up->unt_flgs &= ~MSCP_UF_SCCHH;
                                	up->unt_flgs |= MSCP_UF_WBKNV;
					/*
					 * Issue a set unit characteristics to
					 * toggle the cache status.
					 */
					up->Tflags.tms_wait = 1;
					rp = (REQB *) mscp_alloc_reqb( up, NULL,
					    tmscp_stu_states, (u_long) 0, 
					    (u_long) 0 );
					retries = RETRY_COUNT;
					while( up->Tflags.tms_wait  && 
						( --retries >= 0 ) ) {
						timeout(wakeup,(caddr_t)rp,hz);
						(void)sleep((caddr_t)rp,PSWP+1);
						untimeout(wakeup,(caddr_t)rp,hz);
					}
					Time_Check(up->Tflags.tms_wait, retries, "stu in mtcache");
					up->Tflags.tms_wait = 0;
				}
			}
			else {
				Dprint8("Warning: non-caching or offline\n");
				return(EIO);
			}	
			return(0);

                case MTNOCACHE:	/* Set unit charicteristics for nocache. 
				 * Only allow cache status changes if the 
				 * unit is online.  
			 	 */
			Dprint4("MTNOCACHE\n");
			if ((up->Tflags.tms_cach_on) && (up->flags.online)) {
			    up->Tflags.tms_cach_on = 0;
			    if ((up->Tflags.tms_cach) == 0) {
				Dprint8("Warning: invalid cache enabled\n");
			    }
			    else {
				Dprint8("Disable cache.\n");
				/*
				 * Turn off controller caching by clearing the
				 * write-behind bit and setting the suppress
				 * read-ahead caching bit.
				 */
				up->unt_flgs |= MSCP_UF_SCCHH;
				up->unt_flgs &= ~MSCP_UF_WBKNV;
				/*
				 * Issue a set unit characteristics to
				 * toggle the cache status.
				 */
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL,
				    tmscp_stu_states, (u_long) 0, 
				    (u_long) 0 );
				retries = RETRY_COUNT;
				while( up->Tflags.tms_wait  && 
					( --retries >= 0 ) ) {
					timeout(wakeup,(caddr_t)rp,hz);
					(void)sleep((caddr_t)rp,PSWP+1);
					untimeout(wakeup,(caddr_t)rp,hz);
				}
				Time_Check(up->Tflags.tms_wait, retries, "stu in mtnocache");
				up->Tflags.tms_wait = 0;
			    }
			}
			return(0);

                case MTREW: 	/* Reposition w/rewind modifier	
			 	 */
			Dprint4("MTREW\n");
			up->tms_category_flags |= DEV_RWDING;
			/* Check to make sure that the unit 
			* is online.
			*/
			if( up->flags.online ) {
				/* Allocate a request block and start the data transfer.
				*/
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_REWND, (u_long) 0 );
				retries = RETRY_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTREW");
				up->Tflags.tms_wait = 0;
			}

			up->tms_category_flags &= ~DEV_RWDING;
              
	                /* Check the status for the command that was issued and return fail code
	                * if the status is not success.
	                */
	                if ( up->tms_status != MSCP_ST_SUCC) 
	              		return(EIO);
			else
				return(0);

		case MTOFFL: 	/* Access w/offline modifier	
			 	 */
			/* Check to make sure that the unit
			* is online.
			*/
			Dprint4("MTOFFL\n");
			if( up->flags.online ) {
				/* Allocate a request block.
				*/
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_avl_states, (u_long) MSCP_MD_UNLOD, (u_long) 0 );
				retries = RETRY_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTOFFL");
				up->Tflags.tms_wait = 0;

			}

			/* Check the status for the command that was issued and return fail code
			 * if the status is not success.
			 */
			switch ( up->tms_status ) {
				case MSCP_ST_SUCC:
				case MSCP_ST_OFFLN:
				case MSCP_ST_AVLBL:
					return(0);
				default:
					return(EIO);
				}
			 
                        break;

                case MTNOP:
			return(0);

                case MTCSE:	/* Reposition w/clear serex and count of 0 
			 	 */
			Dprint4("MTCSE\n");
			up->tms_flags |= DEV_CSE;
			/* Check to make sure that the unit 
			* is online.
			*/
			if( up->flags.online ) {
				/* Allocate a request block and start the data transfer.
				*/
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_rpo_states, (u_long) MSCP_MD_OBJCT, (u_long) 0 );
				retries = RETRY_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTCSE");
				up->Tflags.tms_wait = 0;
              
	     	           /* Check the status for the command that was issued and return fail code
	       	           * if the status is not success.
	       	           */
	       	           if ( up->tms_status != MSCP_ST_SUCC) 
		       	       		return(EIO);
				else
					return(0);
			}
			else
				return(EIO);

                        break;

                case MTFLUSH:	/* Flush controller's write-back cache.  
				 * This should be utilized when caching is
				 * being used as a means of verifying that all
				 * records are on the physical media.
			 	 */
			Dprint4("MTFLUSH\n");
			/* Check to make sure that the unit  is online. */
			if( up->flags.online ) {
				if (up->Tflags.tms_cach_on == 0) {
					Dprint8("MTFLUSH: cache not enabled\n");
					return(0);
				}
				up->Tflags.tms_wait = 1;
				rp = (REQB *) mscp_alloc_reqb( up, NULL, 
					tmscp_flu_states, 
					(u_long) MSCP_MD_OBJCT, (u_long) 0 );
				retries = RETRY_COUNT;
				while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
					timeout( wakeup, ( caddr_t )rp, hz );
					( void )sleep(( caddr_t )rp, PSWP+1 );
					untimeout(wakeup, ( caddr_t )rp, hz );
				}
				Time_Check(up->Tflags.tms_wait,retries,"MTCSE");
				up->Tflags.tms_wait = 0;
              
	       	           	if ( up->tms_status != MSCP_ST_SUCC) {
					/*
					 * Return error on flush failure.
					 * The unit should be in serious
					 * exception state which requires
					 * explicit clearing.
					 */
		       	       		return(EIO);
				}
				else {
					return(0);
				}
			}
			else {	/* unit offline */
				return(EIO);
			}

                        break;

                case MTCLX: 
			Dprint4("MTCLX\n");
			/* Fall through to the next case, it does all the work.
			 * This is here only for backwards compatability.
			 */

		case MTCLS:
			Dprint4("MTCLS\n");
			if (!suser())
				return(EACCES);
    			s = Splscs();
			mscp_dispatch( EV_EXRETRY, &cp->timeout_reqb);
			( void )splx( s );
			return(0);

                case MTENAEOT:	/* Return error on EOT	
			 	 */
			up->Tflags.tms_eom = 0;
			Dprint4("MTENAEOT\n");
			return(0);

                case MTDISEOT:	/* Allow read/write after EOT.	
			 	 */
			up->Tflags.tms_eom = 1;
			Dprint4("MTDISEOT\n");
			return(0);

                default:
			Dprint4("default1\n");
                        return (ENXIO);
                }       /* end switch mtop->mt_op */
		break;

        case MTIOCGET:
		Dprint6("MTIOCGET\n");

		/*
		 * The DEV_OFFLINE bit of tms_flags is always the opposite of
		 * up->flags.online.  To avoid the need to keep 2 variables
		 * consistent, use just one of them.
		 */
		if( up->flags.online ) 
			up->tms_flags &= ~DEV_OFFLINE;
		else
			up->tms_flags |= DEV_OFFLINE;
		/* Save away the status of the last command before we change it.
		 */
                mtget = (struct mtget *)data;
                mtget->mt_erreg = up->tms_status;
		/*
		 * Note that tms_flags can extend beyond 8 bits.  At present 
		 * this means that DEV_HARDERR, DEV_DONE, DEV_RETRY, and
		 * DEV_ERASED are lost.  If this status is wanted by a 
		 * utility it should get it from the stat field of devio.
		 *
		 * The mt_dsreg field varies radically from tape driver to tape
		 * driver.  It probably shouldn't be trusted for much.
		 * 
		 * Returning tms_endcode in mt_dsreg probably isn't very
		 * useful because it is very driver specific.
		 */
                mtget->mt_dsreg = ((up->tms_flags & 0xff) << 8);
                mtget->mt_dsreg |= (up->tms_endcode & 0xff);

		/* Do a get unit status insure that the states have not changed.
		 */
		up->Tflags.tms_wait = 1;
		rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_gtu_states,(u_long) 0, (u_long) 0 );
		/* Wait for the information to come back in the end
		 * message before we return the data.
		 */
		retries = RETRY_COUNT;
		while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
			timeout( wakeup, ( caddr_t )rp, hz );
			( void )sleep(( caddr_t )rp, PSWP+1 );
			untimeout(wakeup,(caddr_t)rp);   /* to be sure*/
		}
		Time_Check(up->Tflags.tms_wait,retries,"MTIOCGET");
		up->Tflags.tms_wait = 0;

                mtget->mt_type = MT_ISTMSCP;
		/*
		 * Return the residual value from the last read or write.
		 * If debug is set return the present position in this field.
		 */
		if (tmscpdebug == 0) 
    			mtget->mt_resid = up->tms_resid;
		else
    			mtget->mt_resid = up->tms_position;
		return(0);

        case DEVIOCGET:                         /* device status */
		Dprint6("DEVIOCGET\n");

		/* Do a get unit status insure that the states have not changed.
		 */
		up->Tflags.tms_wait = 1;
		rp = (REQB *) mscp_alloc_reqb( up, NULL, tmscp_gtu_states,(u_long) 0, (u_long) 0 );
		/* Wait for the information to comme back in the end
		 * message before we return the data.
		 */
		retries = RETRY_COUNT;
		while( up->Tflags.tms_wait  && ( --retries >= 0 ) ) {
			timeout( wakeup, ( caddr_t )rp, hz );
			( void )sleep(( caddr_t )rp, PSWP+1 );
			untimeout(wakeup,(caddr_t)rp);   /* to be sure*/
		}
		Time_Check(up->Tflags.tms_wait,retries,"DEVIOCGET");
		up->Tflags.tms_wait = 0;

		/*
		 * The DEV_OFFLINE bit of tms_flags is always the opposite of
		 * up->flags.online.  To avoid the need to keep 2 variables
		 * consistent, use just one of them.
		 */
		if( up->flags.online ) 
			up->tms_flags &= ~DEV_OFFLINE;
		else
			up->tms_flags |= DEV_OFFLINE;
                devget = (struct devget *)data;
                bzero(devget,sizeof(struct devget));
                devget->category = DEV_TAPE;
                devget->adpt_num = ui->ui_adpt;         /* which adapter*/
                devget->nexus_num = ui->ui_nexus;       /* which nexus  */
	        if (ui->ui_hd == 0) {
			/*
			 * Native BI tape controllers such as the BVPSSP do
			 * not set ui_hd.
			 */
		        devget->bus = DEV_BI;
		        devget->bus_num = ui->ui_adpt;       /* which BI ? */
		} else if (ui->ui_hd->uba_type&(UBAUVI|UBAUVII)) {
		        devget->bus = DEV_QB;
		        devget->bus_num = ui->ui_ubanum;     /* which QB ? */
		} else {
		        devget->bus = DEV_UB;
		        devget->bus_num = ui->ui_ubanum;     /* which UBA ? */
		}

		bcopy( cp->model_name,
		       devget->interface,
		       strlen( cp->model_name ));

		mscp_media_to_ascii( up->media_id, devget->device );

                devget->ctlr_num = ui->ui_ctlr;         /* which interf.*/
                devget->slave_num = ui->ui_slave;       /* which plug   */
		bcopy( ui->ui_devname, devget->dev_name, strlen( ui->ui_devname ));
                devget->unit_num = unit;                /* which tms??  */
                devget->soft_count = up->tms_softcnt;  /* soft er. cnt.*/
                devget->hard_count = up->tms_hardcnt;  /* hard er. cnt.*/
                devget->stat = up->tms_flags;		/* status 	*/ 
                devget->category_stat = up->tms_category_flags;	/* category status */ 

		return(0);
    default:
	Dprint6("IOCTL DEFAULT\n");
	return (ENXIO);
    }

    Dprint6("IOCTL EXIT\n");  /* we should never get here */
    return (ENXIO);

}


/*
 *
 *   Name:	tmscp_init_driver	- Initialize class driver
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */


void
tmscp_init_driver()
{
	register int	i;
	int			s;
	register CLASSB	*clp = &tmscp_classb;
	register CMSB	*cmsp = &clp->cmsb;

	Dprint6("tmscp_init_driver\n");
	/* Do common disk and tape inits */
	mscp_common_init();

	/* Prevent re-entry during initialization
	 */
        s = Splscs();
	if ( clp->flags.init_ip ) {
	( void )splx( s );
	return;
	}
	clp->flags.init_ip = 1;
	clp->flags.disk = 0;

	/* Init class driver block listheads
	 */
	clp->flink = ( CONNB * )&clp->flink;
	clp->blink = ( CONNB * )&clp->flink;

	/* Zero and fill in the embedded connection management
	 * services block (CMSB).
	 */
	cmsp->control = mscp_control;
	cmsp->msg_event = mscp_message;
	cmsp->dg_event = mscp_datagram;
	cmsp->lport_name = 0;
	Zero_scaaddr( cmsp->rport_addr );
	cmsp->init_dg_credit = 2;
	cmsp->min_snd_credit = 2;
	cmsp->init_rec_credit = 10;
	( void )bcopy( "U32_TAPE_CL_DRVR",
		    cmsp->lproc_name,
		    NAME_SIZE );
	( void )bcopy( "MSCP$TAPE       ",
		    cmsp->rproc_name,
		    NAME_SIZE );
	( void )bcopy( "                ",
		    cmsp->conn_data,
		    DATA_SIZE );

	/* Initialize the unit table.
	 */
	for( i = 0; i < NUNIT; i++ )
	tmscp_unit_tbl[ i ] = ( UNITB * )NULL;
	clp->unit_tbl = tmscp_unit_tbl;

	/* Initialize the ULTRIX device tape device name string.
	 */
	clp->dev_name = "tms";

	/* Set up the state machine to call for connection recovery,
	 * if the connection path fails and recovery is needed.
	 */
	clp->recov_states = tmscp_recovery_states;

	/* Start up a 1 second timer for use by connection management and
	 * the resource allocation routines.
	 */
	( void )timeout( mscp_timer, ( caddr_t )clp, hz );

	/* Find all the currently known subsystems, start the
	 * connection process (which will complete asynchronously),
	 * restore the entry IPL and exit.
	 */
	mscp_system_poll( clp );
	( void )splx( s );
	return;
}


/*
 *
 *   Name:	tmscp_onlineinit - set a unit online.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_onlineinit( event, rp )

	u_long		event;
	register REQB	*rp;

{
	UNITB		*up = rp->unitb;
	Dprint6("tmscp_onlineinit\n");

	/* Zero ioctl modifers used by ioctl process
	 */
	rp->p1 = 0;
	rp->p2 = 0;

	/* Zero the media string
	 */
	up->tms_format &= 0xff00;

	/* Mark the unit offline with online-in-progress.
	 */
	up->flags.online = 0;
	up->flags.online_ip = 1;

	/* Attempt to allocate a RSPID and exit to redispatch when one
	 * becomes available.
	 */
	mscp_alloc_rspid( event, rp );
}



/*
 *
 *   Name:	tmscp_abortem
 *
 *   Abstract:	
 *	A ABORT mscp command has compleated. release any resources
 *	and return.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long 
tmscp_abortem( event, rp)
    u_long		event;
    register 	REQB	*rp;
{
	UNITB		*up = rp->unitb;
	/* The tape position after an ABORT is undefined (unknown)
	 * so the position lost flag is set.
	 */
	up->Tflags.tms_lost = 1;

	(void) check_return( rp );

	/* Deallocate the request and all the resources that it holds.
	 */
	mscp_dealloc_reqb( rp );
	return( EV_NULL );

}


/*
 *
 *   Name:	tmscp_abortcm
 *
 *   Abstract:	
 *	Issue a ABORT command.  This will abort the last command issued
 *	to the unit.  Get the last command refferance number out of the unit
 *	block.  The command ref. numbers for long running type (immediate) 
 *	commands are stored there.  If the command has compleated, the abort
 *	will not abort any other running commands.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long 
tmscp_abortcm( event, rp)
	u_long		event;
	register REQB	*rp;
{
	UNITB		*up = rp->unitb;
	register MSCP	*mp = rp->msgptr;

	/* Format the message buffer as a ABORT command.  Queue the message
	 * for transmission
	 *
	 * NOTE: The present support for aborting commands is insuficient.
	 * The up->cmd_ref can only store reference to a single command.  It
	 * may be the case that more than one command is outstanding.  This
	 * only aborts whatever happened to be the latest command (which may
	 * not be the one which is intended to be aborted).
	 */
	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_ABORT;
	mp->mscp_modifier = 0;
	mp->mscp_out_ref = up->cmd_ref;
	return( mscp_send_msg( rp ));
}



/*
 *
 *   Name:	mscp_availcm - bring a unit to the available state.
 *
 *   Abstract:	
 *
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_availcm( event, rp )

	u_long		event;
	register REQB	*rp;

{

	register MSCP	*mp = rp->msgptr;
	register UNITB	*up = rp->unitb;
	register CONNB	*cp = rp->connb;
	Dprint7("tmscp_availcm\n");
	up->tms_category_flags |= DEV_RWDING;

	/* Format the message buffer as a AVAILABLE command.  Queue the message
	 * for transmission
	 */
	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_AVAIL;
	mp->mscp_modifier = rp->p1;
	mp->mscp_modifier |= MSCP_MD_CLSEX;
	up->Tflags.tms_clserex = 0;
	/*
	 * In the event of cache data loss, clear the condition to allow the
	 * avail to succeed aviod leaving the unit stuck in this error state.
	 */
	if (up->Tflags.tms_cache_lost) {
		up->Tflags.tms_cache_lost = 0;
		up->Tflags.tms_cach_write = 0;
		mp->mscp_modifier |= MSCP_MD_CDATL; 
	}
	return( mscp_send_msg( rp ));
}
	

/*
 *
 *   Name:
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_availem( event, rp )
	u_long		event;
	register REQB	*rp;
{

	register UNITB	*up = rp->unitb;
	register CONNB	*cp = rp->connb;

	Dprint7("tmscp_availem\n");
	up->Tflags.tms_wait = 0;

	/* Mark the unit NOT in use so opens will work for the next user
	 * that wants the tape drive.
	 */
	up->Tflags.tms_inuse = 0;
	up->tms_category_flags &= ~DEV_RWDING;

	(void) check_return( rp );

	/* Turn off the software write-protected and online in progress
	 * bits.
	 */
	up->flags.online = 0;
	up->flags.wrtp = 0;
	up->flags.online_ip = 0;
	up->tms_position = 0;
	up->tms_flags |= DEV_BOM;

	/*
	 * Successful responses to the available will cause the controller
	 * to flush it's cache.  Clear tms_cach_write to indicate that there
	 * are no write records pending in the controller's write cache.
	 */
	if ( up->tms_status == MSCP_ST_SUCC) 
		up->Tflags.tms_cach_write = 0;

	/* Recovery in progress and this unit did not come online, return so
	 * the next unit (if any) can attempt unit recovery.
	 */
	if(( cp->flags.restart) && (rp->flags.perm_reqb)) {
             printf("\nTape unit #%d failed unit recovery.\n",up->unit);
		return (EV_ONLCOMPLETE);
	}
	/* Deallocate the request and all the resources that it holds.
	 */
	mscp_dealloc_reqb( rp );
	return( EV_NULL );

}


/*
 *
 *   Name:	mscp_datagram - Receive and process a tape datagram
 *
 *   Abstract:	
 *
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_datagram( event, rp )

	u_long		event;
	register REQB	*rp;

{
	Dprint6("tmscp_datagram\n");
}
	

/*
 *
 *   Name:	tmscp_eraseem
 *
 *   Abstract:	
 *	A tape erase mscp command has compleated. release any resources
 *	and return.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long 
tmscp_eraseem( event, rp)
	u_long		event;
	register REQB	*rp;
{
	register MSCP	*mp = rp->msgptr;
	UNITB		*up = rp->unitb;

	( void ) check_return( rp );

	/* If the tape was erased than mark it as erased for ioctl status.
	 * Being an "ancilary" type command, successful completion of the
	 * erase command will force a flush of the controller's write-back
	 * cache (not that it matters in the case where you've just erased
	 * the tape after flushing cache data to it).
	 */
	if( (mp->mscp_status & MSCP_ST_MASK) == MSCP_ST_SUCC) {
		up->tms_flags |= DEV_ERASED;
		up->Tflags.tms_cach_write = 0;
	}

	/* Deallocate the request and all the resources that it holds.
	 */
	mscp_dealloc_reqb( rp );
	return( EV_NULL );

}


/*
 *
 *   Name:	tmscp_erasecm
 *
 *   Abstract:	
 *	Issue a tape erase command.  This will erase the tape starting
 *	at the current position and stopping at the physical end of tape.
 *	The tape is then repositioned to BOT.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long 
tmscp_erasecm( event, rp)
	u_long		event;
	register REQB	*rp;
{
	UNITB		*up = rp->unitb;
	register MSCP	*mp = rp->msgptr;

	/* Format the message buffer as a ERASE command.  Queue the message
	 * for transmission.  Save cmd_ref in up->cmd_ref so it can be aborted
	 * if necessary.  
	 */
	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_ERASE;
	mp->mscp_modifier |= MSCP_MD_CLSEX;
	up->Tflags.tms_clserex = 0;
	mp->mscp_modifier |= MSCP_MD_IMMED;
	/*
	 * If the unit is in a cache data loss condition, clear that state if
	 * there are no writes waiting to be flushed to media.  Otherwise leave
	 * the unit in cache data loss state to cause future data transfer
	 * commands to fail.
	 */
	if (up->Tflags.tms_cache_lost) {
		if (up->Tflags.tms_cach_write == 0) {
			up->Tflags.tms_cache_lost = 0;
			/* clear cache data lost exception */
			mp->mscp_modifier |= MSCP_MD_CDATL; 
			Dprint2("Cache loss on empty write cache - 2\n");
		}
		else {
			Dprint2("Cache data loss condition - 2\n");
		}
	}
	up->cmd_ref = mp->mscp_cmd_ref;	
	return( mscp_send_msg( rp ));
}

/*
 *
 *   Name:	tmscp_flushcm
 *
 *   Abstract:	Flush controller's write back cache.  This is used to force
 *		all records in the cache down onto the physical media.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_flushcm( event, rp)
	u_long			event;
	register 	REQB	*rp;
{
	register MSCP		*mp = rp->msgptr;
	Dprint7("tmscp_flushcm\n");

	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_FLUSH;
	return( mscp_send_msg( rp ));
}

/*
 *
 *   Name:	tmscp_flushem
 *
 *   Abstract:	Returned status on a flush of the controller's write-back
 *		cache.  
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_flushem( event, rp)
	u_long		event;
	register REQB	*rp;
{
	register MSCP	*mp = rp->msgptr;
	UNITB		*up = rp->unitb;
	u_short		em_status = mp->mscp_status & MSCP_ST_MASK;

	Dprint7("tmscp_flushem\n");
	up->Tflags.tms_wait = 0;
	if( em_status == MSCP_ST_SUCC ) {
		Dprint8("tmscp_flushem: success\n");
		/*
		 * Clear tms_cach_write now that we know that the data is on
		 * the physical media.
		 */
		up->Tflags.tms_cach_write = 0;
	}
	else {
		Dprint8("tmscp_flushem: cache flush failure\n");
	}
	/*
	 * Save the position for use by recovery code.
	 * The position here reflects the last known good object count.
	 */
	up->tms_position = mp->mscp_position;
	(void) check_return( rp );

	/* Deallocate the request and all the resources that it holds.
	 */
	mscp_dealloc_reqb( rp );
	return( EV_NULL );

}

/*
 *
 *   Name:	tmscp_invevent - Process invalid event.
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return	NONE
 *   Values:
 */

u_long 
tmscp_invevent( event, rp )
    register u_long		event;
    REQB			*rp;

{
	register u_long		state = rp->state;

    	printf( "tmscp_invevent:  invalid event %d in state %d, reqb %x\n",
	    event,
	    state,
	    rp );
	panic( "tmscp_invevent: fatal tmscp error.\n" );
}



/*
 *
 *   Name:	tmscp_onlinecm - Format and send an MSCP ONLIN command.
 *
 *   Abstract:	Format and queue an MSCP online command to be sent.
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_onlinecm( event, rp )

	u_long			event;
	register REQB		*rp;

{

	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	Dprint7("tmscp_onlinecm\n");

	/* Format the message buffer as a ONLINE command.  Update the state
	 * and send the message
	 * come back.
	 */
	Init_msg( mp, rp->rspid, up->unit );
	mp->mscp_opcode = MSCP_OP_ONLIN;
	mp->mscp_unt_flgs = 0;  /* Watch this space */
	mp->mscp_modifier |= (MSCP_MD_CLSEX | MSCP_MD_EXCAC);
	up->Tflags.tms_clserex = 0;
	/*
	 * If the unit is in a cache data loss condition, clear that state.
	 * There is no need to check tms_cach_write first because that should
	 * Have been cleared in close or availcm.
	 */
	if (up->Tflags.tms_cache_lost) {
		up->Tflags.tms_cache_lost = 0;
		mp->mscp_modifier |= MSCP_MD_CDATL; 
	}
	return ( mscp_send_msg( rp ));
}


/*
 *
 *   Name:	tmscp_onlineem - process an online end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_onlineem( event, rp )
	u_long		event;
	register REQB	*rp;

{

	register MSCP	*mp = rp->msgptr;
	UNITB		*up = rp->unitb;
	u_short		em_status = mp->mscp_status & MSCP_ST_MASK;

	Dprint7("tmscp_onlineem \n");

	/* Check to see if the online succeeded.
	 */
	(void) check_return( rp );

	/*
	 * Successful responses to the online command will cause the controller
	 * to flush it's cache.  Clear tms_cach_write to indicate that there
	 * are no write records pending in the controller's write cache.
	 */
	if ( em_status ==  MSCP_ST_SUCC ) {
		up->Tflags.tms_cach_write = 0;
	}

	if(( em_status ==  MSCP_ST_SUCC ) || ( em_status == MSCP_ST_WRTPR )) {


		/* Store the contents of the online end message, recycle the
		 * RSPID, then format and send a get unit status message.
		 */
		up->unit_id = *( UNIQ_ID * )mp->mscp_unit_id;
		up->media_id = mp->mscp_media_id;
		up->unt_flgs = mp->mscp_unt_flgs;
		up->mult_unt = mp->mscp_mult_unt;
		up->tms_format = mp->mscp_format;
		up->tms_speed = mp->mscp_speed;
		up->tms_bcount = mp->mscp_maxwtrec;
		up->tms_noise = mp->mscp_noiserec;
		up->Tflags.tms_wait = 0;

		mscp_recycle_rspid( rp );
		rp->p1 = 0;
		rp->p2 = 0;
		Init_msg( mp, rp->rspid, up->unit );
		mp->mscp_opcode = MSCP_OP_GTUNT;
		return( mscp_send_msg( rp ));
			
	/* The online command did not succeed.  Check the end message status,
	 * reset the online in progress flag, and exit to allow open to time out
	 * the operation.
	 */
	} else { 

                up->flags.online_ip = 0;
                up->flags.online = 0;
                up->Tflags.tms_wait = 0;

		/* If this connection is being restarted and we have
		 * failed to bring this device online, exit with an
		 * online failure.  Available processing will be done
		 * on this unit and recovery for other units will
		 * continue.
		 */
		if ((rp->connb->flags.restart) && (rp->flags.perm_reqb))
		    return ( EV_ONLERROR );

		mscp_dealloc_reqb( rp );
		return( EV_NULL );
	}
}


/*
 *
 *   Name:	mscp_onlgtuntem - process GTUNT end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long 
tmscp_onlgtuntem( event, rp )
	u_long			event;
	register REQB		*rp;
{
	register MSCP		*mp = rp->msgptr;
	register UNITB		*up = rp->unitb;
	
	Dprint7("tmscp_onlgtuntem\n");

	if(( gtuntem_body( event, rp )) == MSCP_ST_SUCC) {
		/* The online sequence has completed successfully.  Mark the 
		 * unit online, turn off the online in progress flag.
		 * Do a set unit characteristics for density.
		 */
		up->flags.online = 1;
		up->flags.online_ip = 0;
		mscp_recycle_rspid( rp );
		rp->p1 = 0;
		rp->p2 = 0;
		Init_msg( mp, rp->rspid, rp->unitb->unit );
		mp->mscp_opcode =  MSCP_OP_STUNT;
		Dprint8("STUNT format = 0x%x\n",up->tms_format);
		mp->mscp_format = up->tms_format;
		mp->mscp_unt_flgs = up->unt_flgs;
		/*
		 * The unit should not be in a cache data loss state when
		 * brought online because the previous availcm should have
		 * cleared the condition.  This test is a sanity check.
		 */
		if (up->Tflags.tms_cache_lost) {
			up->Tflags.tms_cache_lost = 0;
			up->Tflags.tms_cach_write = 0;
			mp->mscp_modifier |= MSCP_MD_CDATL; 
			Dprint2("Cache loss condition in tmscp_onlgtuntem.\n");
		}
		return( mscp_send_msg( rp ));

	} else {
		/* The Get Unit Status failed.  Recycle the RSPID and then
		 * fabricate an event and status combination that will retry
		 * the ONLINE command.
		 */
		mscp_recycle_rspid( rp );
		rp->p1 = 0;
		rp->p2 = 0;
		return( EV_ONLERROR );
	}
}    


/*
 *
 *   Name:	tmscp_gtuntcm
 *
 *   Abstract:	The GET UNIT STATUS command returns the current
 *		connection state of a tape unit as well as unit
 *		characteristics.  Note that the validity of the
 *		unit characteristics is dependent on the current
 *		connection state.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_gtuntcm( event, rp)
	u_long			event;
	register 	REQB	*rp;
{
	register MSCP		*mp = rp->msgptr;
	Dprint7("tmscp_gtuntcm\n");

	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_GTUNT;
	return( mscp_send_msg( rp ));
}


/*
 *
 *   Name:	tmscp_gtuntem
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_gtuntem( event, rp)
	u_long		event;
	register REQB	*rp;
{
	UNITB			*up = rp->unitb;
	Dprint7("tmscp_gtuntem\n");

	( void ) gtuntem_body( event, rp );
	up->Tflags.tms_wait = 0;
	/* Deallocate the request and all the resources that it holds.
	 */
	mscp_dealloc_reqb( rp );
	return( EV_NULL );
}


/*
 *
 *   Name:	gtuntem_body
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
gtuntem_body( event, rp)
	u_long			event;
	register 	REQB	*rp;
{
	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
	u_short			em_flags = mp->mscp_unt_flgs;
	Dprint7("gtuntem_body\n");


	/* If the Get Unit Status command succeeded, store the unit
	 * status in the unit block.
	 */
	(void) check_return( rp );
	if( em_status == MSCP_ST_SUCC ) {
		Pad_msg( mp, rp->msgsize );
		up->mult_unt = mp->mscp_mult_unt;
		up->unt_flgs = mp->mscp_unt_flgs;
		up->unit_id = *( UNIQ_ID * )mp->mscp_unit_id;
		up->media_id = mp->mscp_media_id;
		up->tms_format = mp->mscp_format;
		up->tms_speed = mp->mscp_speed;
		up->format_menu = mp->mscp_formenu;
		up->flags.online = 1;
		Dprint8("em_body, SUCC, set online\n");
	
	    /* Check on the flags and set the unit flags as needed.
	     */
		up->flags.wrtp = 0; 
		up->tms_flags &= ~DEV_WRTLCK;
		if (em_flags & MSCP_UF_WRTPH) {
			up->flags.wrtp = 1; /* set hardware write protect */
			up->tms_flags |= DEV_WRTLCK;
		}
		if (em_flags & MSCP_UF_WRTPS) {
			up->flags.wrtp = 1; /* set software write protect */
			up->tms_flags |= DEV_WRTLCK;
		}
		if (em_flags & MSCP_UF_WRTPD) {
			up->flags.wrtp = 1; /* set data safty write protect */
			up->tms_flags |= DEV_WRTLCK;
		}
		if (em_flags & MSCP_UF_CACH) {
			/*
			 * Unit supports caching.
			 */
			Dprint8("Unit does caching.\n");
			up->Tflags.tms_cach = 1; 
			if ((up->Tflags.tms_cach_on) || tmscptestcache) {
				/*
				 * Turn on controller caching.  This
				 * causes read-ahead and write-behind
				 * caching.
				 */
				Dprint8("Enable cache.\n");
				if (up->Tflags.tms_cach_on == 0)
					Dprint8("Forced cache enable.\n");
				up->unt_flgs &= ~MSCP_UF_SCCHH;
				up->unt_flgs |= MSCP_UF_WBKNV;
			}
			else {
				Dprint8("Disable cache.\n");
				up->unt_flgs |= MSCP_UF_SCCHH;
				up->unt_flgs &= ~MSCP_UF_WBKNV;
			}
		}

	
	    /* If the tape is at BOT then the density should be set.  Check the 
	     * device name and set the density to the highest allowable setting 
	     * if it's a high density name; otherwise set it to the lowest 
	     * allowable densith.  The allowable settings are in the UNIT BLOCK 
	     * format_menu.
	     *
	     * The order of the tests is important.  To find the highest density
	     * start at the largest known value and check if it's supported.  If
	     * not, check the next highest until one is found, thats the highest
	     *  supported.
	     */
		if( up->tms_position == 0 )
		{
		/* Set up default device params.
		 */
			up->tms_flags |= DEV_BOM;
			up->tms_category_flags = 0;
			up->tms_format &= 0xff00;
			/* Check if it's a HIGH density device */
			if ((up->sel & DENS_MASK) == HI_DENS)
			{
				Dprint8("HIGH DENSITY DEVICE FORMAT MENU = 0x%x\n",up->format_menu);
	                        switch (up->tms_format)
	                        {
	                        case MSCP_TC_9TRACK:
					Dprint8("9-TRACK\n");
	                                if((up->format_menu & MSCP_TF_GCR) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_6250BPI;
	                                        up->tms_format |= MSCP_TF_GCR;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_PE) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_1600BPI;
	                                        up->tms_format |= MSCP_TF_PE;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_800) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_800BPI;
	                                        up->tms_format |= MSCP_TF_800;
	                                }
	                                else
					{
	                                        up->tms_format = MSCP_TC_OLD;
	                                        up->tms_category_flags |= DEV_6250BPI;
					}
	                                break;
	                        case MSCP_TC_3480: /* TA90 */
					Dprint8("3480 Media\n");
	                                if((up->format_menu & MSCP_TF_EDCP) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_76000_CP;
	                                        up->tms_format |= MSCP_TF_EDCP;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_ENHD) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_76000BPI;
	                                        up->tms_format |= MSCP_TF_ENHD;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_NDCP) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_38000_CP;
	                                        up->tms_format |= MSCP_TF_NDCP;
	                                }
	                                else
					{
	                                        up->tms_format |= MSCP_TF_NORML;
	                                        up->tms_category_flags |= DEV_38000BPI;
					}
	                                break;
	                        case MSCP_TC_W1:   /* RV20, RV64 */
	                                up->tms_format |= MSCP_TF_NORML;
					/* No density was listed in the VAX
					 * products manual.  
					 */
	                                break;
	                        case MSCP_TC_CTP:
					Dprint8("CARTRIDGE\n");
	                                if((up->format_menu & MSCP_TF_BLKHD) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_10240BPI;
	                                        up->tms_format |= MSCP_TF_BLKHD;
	                                }
	                                else
	                                {
	                                        up->tms_category_flags |= DEV_6666BPI;
	                                        up->tms_format = MSCP_TC_OLD;
	                                }
	                                break;
	                        case MSCP_TC_OLD:
					Dprint8("OLD FORMAT\n");
	                                if((up->format_menu & MSCP_TF_BLOCK) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_6666BPI;
	                                        up->tms_format |= MSCP_TF_BLOCK;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_GCR) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_6250BPI;
	                                        up->tms_format |= MSCP_TF_GCR;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_PE) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_1600BPI;
	                                        up->tms_format |= MSCP_TF_PE;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_800) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_800BPI;
	                                        up->tms_format |= MSCP_TF_800;
	                                }
	                                else
	                                {
	                                        up->tms_category_flags |= DEV_6250BPI;
	                                        up->tms_format = MSCP_TC_OLD;
	                                }
	                                break;
	                        default:
	                                up->tms_format = MSCP_TC_OLD;
	                                up->tms_category_flags |= DEV_6250BPI;
	                                break;
	                        }
				Dprint8("format set to 0x%x\n",up->tms_format);
			}
			/* Check if it's a LOW density device */
			else if ((up->sel & DENS_MASK) == LOW_DENS)
			{
			Dprint8("LOW DENSITY DEVICE FORMAT MENU = 0x%x\n",up->format_menu);
	                        switch (up->tms_format)
	                        {
	                        case MSCP_TC_9TRACK:
					Dprint8("9-TRACK\n");
	                                if((up->format_menu & MSCP_TF_800) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_800BPI;
	                                        up->tms_format |= MSCP_TF_800;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_PE) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_1600BPI;
	                                        up->tms_format |= MSCP_TF_PE;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_GCR) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_6250BPI;
	                                        up->tms_format |= MSCP_TF_GCR;
	                                }
	                                else
	                                {
	                                        up->tms_category_flags |= DEV_6250BPI;
	                                        up->tms_format = MSCP_TC_OLD;
	                                }
	                                break;
	                        case MSCP_TC_3480: /* TA90 */
                                        Dprint8("3480 Media\n");
	                                if((up->format_menu & MSCP_TF_NORML) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_38000BPI;
	                                        up->tms_format |= MSCP_TF_NORML;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_NDCP) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_38000_CP;
	                                        up->tms_format |= MSCP_TF_NDCP;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_ENHD) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_76000BPI;
	                                        up->tms_format |= MSCP_TF_ENHD;
	                                }
	                                else
					if(up->format_menu & MSCP_TF_EDCP)
					{
	                                        up->tms_format |= MSCP_TF_EDCP;
	                                        up->tms_category_flags |= DEV_76000_CP;
					}
	                                break;
	                        case MSCP_TC_W1:   /* RV20, RV64 */
	                                up->tms_format |= MSCP_TF_NORML;
					/* No density was listed in the VAX
					 * products manual.  
					 */
	                                break;
	                        case MSCP_TC_CTP:
					Dprint8("CARTRIDGE\n");
	                                if((up->format_menu & MSCP_TF_NORML) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_6666BPI;
	                                        up->tms_format |= MSCP_TF_NORML;
	                                }
	                                else
	                                {
	                                        up->tms_format = MSCP_TC_OLD;
	                                        up->tms_category_flags |= DEV_6666BPI;
	                                }
	                                break;
	                        case MSCP_TC_OLD:
					Dprint8("OLD FORMAT\n");
	                                if((up->format_menu & MSCP_TF_800) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_800BPI;
	                                        up->tms_format |= MSCP_TF_800;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_PE) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_1600BPI;
	                                        up->tms_format |= MSCP_TF_PE;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_GCR) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_6250BPI;
	                                        up->tms_format |= MSCP_TF_GCR;
	                                }
	                                else
	                                if((up->format_menu & MSCP_TF_BLOCK) != 0)
	                                {
	                                        up->tms_category_flags |= DEV_6666BPI;
	                                        up->tms_format |= MSCP_TF_BLOCK;
	                                }
	                                else
	                                {
	                                        up->tms_category_flags |= DEV_6250BPI;
	                                        up->tms_format = MSCP_TC_OLD;
	                                }
	                                break;
	                        default:
	                                up->tms_format = MSCP_TC_OLD;
	                                up->tms_category_flags |= DEV_6250BPI;
	                        }
				Dprint8("format set to 0x%x\n",up->tms_format);
			}
			/* Check if it's a MEDIUM density device */
			else if ((up->sel & DENS_MASK) == MED_DENS) {
			    mprintf("tms - no support for medium density.\n");
			}
			/* Check if it's an AUXILIARY density device */
			else if ((up->sel & DENS_MASK) == AUX_DENS) {
			    mprintf("tms - no support for auxiliary density.\n");
			}
			else  {
			    mprintf("tms - unknown density. sel = 0x%x\n",up->sel);
			}
		}
		/*
		 * See if a media loader is present.  This check is done after
		 * the above clause because it zeroes out the whole field if
		 * at BOT.
		 */
		if (em_flags & MSCP_UF_LOADR) {
			up->tms_category_flags |= DEV_LOADER;
		}
		else {
			up->tms_category_flags &= ~DEV_LOADER;
		}
	}
	return(em_status);
}

/*
 *
 *   Name:	tmscp_recovinit 
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	
 *   Values:
 */

u_long
tmscp_recovinit( event, rp )

    u_long			event;
    register REQB		*rp;

{
	register CONNB		*cp = rp->connb;
	register UNITB		*up;
	register u_long		new_event = EV_NULL;

	Dprint6("tmscp_recovinit\n");

	/* Find the first unit that was online when the connection dropped.
	 */
	for( up = cp->unit.flink;
	    up != ( UNITB * )&cp->unit.flink && !up->flags.online;
	    up = up->flink ) {}

	
	/* If we found a previously online unit, mark it offline with
	 * online in progress, store the unit block pointer in the request
	 * block and allocate a RSPID to use in the online sequence.
	 */
	if( up != ( UNITB * )&cp->unit.flink ) {
		Dprint6("tmscp_recovinit: starting recovery on up %x, unit %d\n",
		up, up->unit);
		up->flags.online = 0;
		up->flags.online_ip = 1;
		up->tms_recovery_location = up->tms_position;
		rp->unitb = up;
		new_event = mscp_alloc_rspid( event, rp );

	/* No unit was online when the connection dropped; however, there
	 * may have been an online request enqueued, so attempt to restart
	 * the next request in the restart queue.
	 */
	} else {
		mscp_restart_next( cp );
		mscp_dealloc_all( rp );
	}

return( new_event );
}

/*
 *
 *   Name:	tmscp_recovnext - Mark current unit online and process next
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	
 *   Values:
 */

u_long 
tmscp_recovnext( event, rp )
    u_long			event;
    register REQB		*rp;
{
	register UNITB		*up = rp->unitb;
	register CONNB		*cp = rp->connb;
	register u_long		new_event = EV_NULL;
	
	Dprint6("tmscp_recovnext\n");

	/* Find the next unit that was online when the connection dropped.
	 */

	for( up = up->flink;
	    up != ( UNITB * )&cp->unit.flink && !up->flags.online;
	    up = up->flink ) {}
	
	/* If we found another previously online unit, mark it offline with
	 * online in progress, store the unit block pointer in the request
	 * block and redispatch to bring the unit online.
	 */
	if( up != ( UNITB * )&cp->unit.flink ) {
		Dprint6("tmscp_recovnext: continuing recovery on up %x, unit %d\n", up, up->unit);
		up->flags.online = 0;
		up->flags.online_ip = 1;
		up->tms_recovery_location = up->tms_position;
		rp->unitb = up;
		mscp_recycle_rspid(rp);
		new_event = EV_ONLDONEXT;
	
		/* No more units were online.  Attempt to start the first 
		 * request on the restart queue, deallocate the resources
		 * held by the request block and terminate the thread.
		 */
	} else {
		mscp_restart_next( cp );
		mscp_dealloc_all( rp );
	}
return( new_event );
}

/*
 *
 *   Name:	tmscp_recover_loc
 *
 *   Abstract:	This routine is used to position the tape from BOT to the last
 *		known location.  The conection was lost and the
 *		drive has been brought back online, now its time to 
 *		reposition back to the old position.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_recover_loc( event, rp )
	u_long			event;
	register	REQB	*rp;
{

	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	u_short			em_status = mp->mscp_status & MSCP_ST_MASK;

	Dprint6("tmscp_recover_loc\n");

	/* Check to see if the set_unit succeeded.
	 */

	(void) check_return( rp );
	switch( em_status ) {
		/* Store the contents of the end message. 
		 */
		case MSCP_ST_SUCC:
			up->unit_id = *( UNIQ_ID * )mp->mscp_unit_id;
			up->media_id = mp->mscp_media_id;
			up->unt_flgs = mp->mscp_unt_flgs;
			up->mult_unt = mp->mscp_mult_unt;
			up->tms_format = mp->mscp_format;
			up->tms_speed = mp->mscp_speed;
			up->tms_bcount = mp->mscp_maxwtrec;
			up->tms_noise = mp->mscp_noiserec;
			up->Tflags.tms_clserex = 0;

			/* Ok, its online, now reposition to the location on 
			 *the tape where we were when the connection was lost.
			 */
			mscp_recycle_rspid( rp );
			Init_msg( mp, rp->rspid, rp->unitb->unit );
			mp->mscp_opcode = MSCP_OP_REPOS;
			mp->mscp_modifier = 0;
			mp->mscp_modifier |= (MSCP_MD_CLSEX | MSCP_MD_OBJCT);

			/*
			 * If the unit is in a cache data loss condition, clear
			 * that state if there are no writes waiting to be 
			 * flushed to media. Otherwise leave the unit in cache 
			 * data loss state to cause future data transfer 
			 * commands to fail.
			 */
			if (up->Tflags.tms_cache_lost) {
				if (up->Tflags.tms_cach_write == 0) {
					up->Tflags.tms_cache_lost = 0;
					/* clear cache data lost exception */
					mp->mscp_modifier |= MSCP_MD_CDATL; 
					Dprint2("Cache loss on empty write cache-5");
				}
				else {
					Dprint2("Cache data loss condition-5");
				}
			}
			mp->mscp_rec_cnt = up->tms_position;
			mp->mscp_tmgp_cnt = 0;
			return( mscp_send_msg( rp ));
		
		/* The online did not succeed.   
		 * Let the unit go available.
		 */
		default:
			up->flags.online_ip = 0;
			up->flags.online = 0;
			mscp_recycle_rspid( rp );
			return( EV_ONLERROR );
			break;
	}

	/*
	 * Should never get here.
	 */
	mscp_recycle_rspid( rp );
	return( EV_ONLERROR );
}

/*
 *
 *   Name:	tmscp_reposcm
 *
 *   Abstract:	This command is used to position the tape from one
 *		location to another.  The manner in which the
 *		positioning operation is to be performed is dictated
 *		by the state of the following command modifiers:
 *			- Object Count
 *			- Rewind
 *			- Reverse
 *		and the values supplied in the "tape mark count" and
 *		"record count or object count" comand message fields.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_reposcm( event, rp)
	u_long			event;
	register 	REQB	*rp;
{
	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	Dprint7("tmscp_reposcm\n");

	up->cmd_ref = mp->mscp_cmd_ref;	/* Save cmd_ref so it can be aborted if need be */
	/* Format the message buffer as a REPOSITION command.  Queue the message
	 * for transmission
	 */
	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_REPOS;
	mp->mscp_modifier = rp->p1;
	mp->mscp_modifier |= MSCP_MD_CLSEX;
	up->Tflags.tms_clserex = 0;
	if (up->Tflags.tms_cache_lost) {
		/*
		 * A cache data loss condition has occured.  Clear this state
		 * if there are no pending writes which could be sitting in
		 * the controller's write back cache.  Also clear if DEV_CSE
		 * is set which would be the case if an MTCSE was issued in an
		 * attempt to recognize the cache data loss and clear it to 
		 * try error recovery.
		 */
		if ((up->Tflags.tms_cach_write == 0) || 
			(up->tms_flags & DEV_CSE)) {
			up->Tflags.tms_cache_lost = 0;
			up->Tflags.tms_cach_write = 0;
			mp->mscp_modifier |= MSCP_MD_CDATL; 
			Dprint2("Cache loss on empty write cache -6 \n");
		}
		else {
			Dprint2("Cache data loss condition -6 \n");
		}
	}
	if(mp->mscp_modifier & MSCP_MD_OBJCT) {
		mp->mscp_rec_cnt = rp->p2;
		mp->mscp_tmgp_cnt = 0;
	} else {
		mp->mscp_tmgp_cnt = rp->p2;
		mp->mscp_rec_cnt = 0;
	}
	return( mscp_send_msg( rp ));
}


/*
 *
 *   Name:	tmscp_repreccm
 *
 *   Abstract:	This command is used to position the tape from one
 *		tape record to another.  An important distinction is that tape
 *		records do not include tape marks.  The manner in which the
 *		positioning operation is to be performed is dictated
 *		by the state of the following command modifiers:
 *			- Object Count
 *			- Reverse
 *		and the values supplied in the "tape mark count" and
 *		"record count or object count" comand message fields.
 *
 *		This routine is almost identical to the tmscp_reposcm
 *		routine above with the exception that RECORD movements
 *		are to be done.  The tmscp_reposcm routine causes file
 *		(tape mark) and object (both records and tape mark)
 *		movement.  The only means to differentiate records from
 *		objects was to create an additional routine and state table.
 *		The end message will still go to tmscp_reposem.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_repreccm( event, rp)
	u_long			event;
	register 	REQB	*rp;
{
	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	Dprint7("tmscp_rereccm\n");

	up->cmd_ref = mp->mscp_cmd_ref;	/* Save cmd_ref so it can be aborted if need be */
	/* Format the message buffer as a REPOSITION command.  Queue the message
	 * for transmission
	 */
	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_REPOS;
	mp->mscp_modifier = rp->p1;
	/*
	 * If the unit is in a cache data loss condition, clear that state if
	 * there are no writes waiting to be flushed to media.  Otherwise leave
	 * the unit in cache data loss state to cause future data transfer
	 * commands to fail.
	 */
	if (up->Tflags.tms_cache_lost) {
		if (up->Tflags.tms_cach_write == 0) {
			up->Tflags.tms_cache_lost = 0;
			/* clear cache data lost exception */
			mp->mscp_modifier |= MSCP_MD_CDATL; 
			Dprint2("Cache loss on empty write cache - 7\n");
		}
		else {
			Dprint2("Cache data loss condition - 7\n");
		}
	}
	mp->mscp_rec_cnt = rp->p2;	/* how many records */
	mp->mscp_tmgp_cnt = 0;		/* move no tape marks */
	return( mscp_send_msg( rp ));
}


/*
 *
 *   Name:	tmscp_reposem
 *
 *   Abstract:	Process the end message for a reposition command.  This command
 *		is used for lots of things, ioctl's, close, connection recovery.
 *		In the case of connection recovery, a few special things need to
 *		be done; turn off the in progress flag and test to make sure
 *		that the position is where it should be (the last known good
 *		location).
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_reposem( event, rp)
	u_long			event;
	register 	REQB	*rp;
{
	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	CONNB			*cp = rp->connb;
	u_short			em_status = mp->mscp_status & MSCP_ST_MASK;

	Dprint7("tmscp_reposem\n");

	(void) check_return( rp );
	
	/* Zero flag used to wait on the rewind to finish.
	*/
	up->Tflags.tms_wait = 0;

	if( em_status == MSCP_ST_SUCC ) {
		/*
		 * If the position has changed clear the written flag.  This is
		 * done to prevent tape marks to be put down in the close
		 * routine if a reposition has been done after the tape is
		 * written but before the close is issued.
		 */
		if (up->tms_position != mp->mscp_position) {
			up->Tflags.tms_write = 0;	
			Dprint8("tmscp_reposem: clear tms_write\n");
		}
		/*
		 * Save the position for use by recovery code.
		 */
		up->tms_position = mp->mscp_position;
		up->tms_flags &= ~DEV_BOM;
		if(up->tms_position == 0) {
			up->tms_flags |= DEV_BOM;
			up->Tflags.tms_lost = 0;
		}
		/*
		 * Successful responses to the reposition command will cause the
		 * controller to flush it's cache.  Clear tms_cach_write to 
		 * indicate that there are no write records pending in the 
		 * controller's write-back cache.
		 */
		up->Tflags.tms_cach_write = 0;
	}

	/* If a recovery was in progress, check that the position is where
	 * it should be.
	 */
	Dprint8("recovery in progress if its a 1; it is = %d\n", cp->flags.restart);
	if(( cp->flags.restart) && ( rp->flags.perm_reqb)) {
		up->Tflags.tms_lost = 0;
		if ( mp->mscp_position != up->tms_recovery_location) {
			Dprint2("tmscp_reposem recovery position mismatch\n");
			return( EV_ONLERROR );
		}else{
			return (EV_ONLCOMPLETE);
		}
	}

	/* Deallocate the request and all the resources that it holds.
	 */
	mscp_dealloc_reqb( rp );
	return( EV_NULL );

}



/*
 *
 *   Name:	tmscp_setunitcm - Format and send an MSCP STUNT command.
 *
 *   Abstract:	Set device characteristics.
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_setunitcm( event, rp )

	u_long			event;
	register REQB		*rp;

{

	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	Dprint7("tmscp_setunitcm\n");

	/* Format the message buffer as a STUNT command.  
	 * If cache info has been lost, set the clear cache data loss exception.
	 * Set the tape format and flags.  An assumption is made here that the
	 * format and flags have been previously setup, for example through
	 * a get unit status command.
	 */
	Init_msg( mp, rp->rspid, up->unit );
	mp->mscp_opcode = MSCP_OP_STUNT;
	mp->mscp_modifier = MSCP_MD_EXCAC; 
	/*
	 * If the unit is in a cache data loss condition, clear that state if
	 * there are no writes waiting to be flushed to media.  Otherwise leave
	 * the unit in cache data loss state to cause future data transfer
	 * commands to fail.
	 */
	if (up->Tflags.tms_cache_lost) {
		if (up->Tflags.tms_cach_write == 0) {
			up->Tflags.tms_cache_lost = 0;
			/* clear cache data lost exception */
			mp->mscp_modifier |= MSCP_MD_CDATL; 
			Dprint2("Cache loss on empty write cache - 8\n");
		}
		else {
			Dprint2("Cache data loss condition - 8\n");
		}
	}
	/*
	 * If the tape is not positioned at BOT, a zero value in the format
	 * field will not cause the format to change from the currently 
	 * selected density.  Specifying a non-zero format when not at
	 * BOT will cause the command to be rejected with an "Invalid
	 * Command" subcode "Invalid Format".
	 */
	if (up->tms_position != 0) {
		Dprint8("Set format to 0 since not at BOT.\n");
		mp->mscp_format = 0;
	}
	else {
        	Dprint8("At BOT: STUNT format = 0x%x, flags = 0x%x\n",
			up->tms_format,up->unt_flgs);
        	mp->mscp_format = up->tms_format;
	}
        mp->mscp_unt_flgs = up->unt_flgs;
	return ( mscp_send_msg( rp ));
}


/*
 *
 *   Name:	tmscp_setunitem - process a set unit characteristics end message
 *
 *   Abstract:	The SET UNIT CHARACTERISTICS Command is used to set
 *		host-settable unit characteristics and to obtain those
 *		unit characteristics essentail for proper class driver
 *		operation.
 *
 *		Successful execution of this command never alters the tape
 *		unit's host-relative connection state.  Unsuccessful
 *		execution leaves the tape unit in either the "unit-availabe"
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */

u_long
tmscp_setunitem( event, rp )
	u_long			event;
	register REQB		*rp;

{
	register MSCP		*mp = rp->msgptr;
	UNITB			*up = rp->unitb;
	u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
	Dprint7("tmscp_setunitem\n");

	/* Zero flag used to wait for the setunitcm to complete.
	*/
	up->Tflags.tms_wait = 0;

	/* Check to see if the transfer succeeded.
	 */

	(void) check_return( rp );

	/*
	 * Responses to the set unit char command will cause the controller
	 * to flush it's cache.  Clear tms_cach_write to indicate that there
	 * are no write records pending in the controller's write cache.
	 */
	up->Tflags.tms_cach_write = 0;

	switch( em_status ) {
		/* Store the contents of the end message. 
		 */
		case MSCP_ST_SUCC:
			up->unit_id = *( UNIQ_ID * )mp->mscp_unit_id;
			up->media_id = mp->mscp_media_id;
			up->unt_flgs = mp->mscp_unt_flgs;
			up->mult_unt = mp->mscp_mult_unt;
			up->tms_format = mp->mscp_format;
			up->tms_speed = mp->mscp_speed;
			up->tms_bcount = mp->mscp_maxwtrec;
			up->tms_noise = mp->mscp_noiserec;
			up->flags.online = 1;
	
			break;
		
	    /* The online command did not succeed.   Reset the online
	     * in progress flag, and exit to allow open to time out
	     * the operation.
	     */
		default:
			up->flags.online_ip = 0;
			break;
	}
	mscp_dealloc_reqb( rp );
	return( EV_NULL );
}


/*
 *
 *   Name:	mscp_transfercm - perform a data transfer operation.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */
u_long
tmscp_transfercm( event, rp )
	u_long			event;
	register REQB		*rp;
{
	register MSCP		*mp = rp->msgptr;
	register struct buf	*bp = rp->bufptr;
	register UNITB		*up = rp->unitb;


	/* Format the message buffer as a READ/WRITE command and fill in
	 * the transfer byte count, local buffer handle, and logical
	 * block number.  Update the state and send the message.
	 */

	Init_msg( mp, rp->rspid, rp->unitb->unit );
	/* Clear recoverable exception.
	 * If Re-Write error recovery is avalible: use it
	 */
	if((up->unt_flgs & MSCP_UF_EWRER) && ( bp->b_flags & B_WRITE ))
		mp->mscp_modifier |= MSCP_MD_ENRWR;

	/* If access past EOM is set then let I/O work past EOM by
	 * clearing any serious exception conditions which would
	 * normally stop the I/O. 
	 */
	if( (up->tms_flags & DEV_EOM) && up->Tflags.tms_eom &&
		up->Tflags.tms_clserex ) {
		mp->mscp_modifier |= MSCP_MD_CLSEX;
		up->Tflags.tms_clserex = 0;
	}

	mp->mscp_opcode = ( bp->b_flags & B_READ ) ?
			MSCP_OP_READ : MSCP_OP_WRITE;

	/*
	 * If write-back caching is enabled then set this status bit to
	 * indicate that records could be sitting in the cache.  This status
	 * bit will then be looked at when a cache data loss condition occurs
	 * so that the serious exception can be cleared if there are no
	 * records which could have been lost.  This bit gets cleared when
	 * an operation completes which guarantees that the cache has been
	 * flushed.  
	 * 
	 * In order for this approach to work, for any tmscp commands which
	 * clear this bit (signaling cache flush) must wait for completion
	 * where they are issued.  This must be done to prevent an end message
	 * associated with an earlier flush type operation from improperly
	 * clearing this bit when there are write commands which were issued
	 * after the flush type command.
	 *
	 * up->Tflags.tms_cach_write will be cleared upon successful completion
	 * of the following commands:
 	 *	flush, read, avail, online, repositions,  set unit char, 
	 *	2 consecutive WTM's.
	 */
	if ((up->Tflags.tms_cach_on) && (mp->mscp_opcode == MSCP_OP_WRITE)) {
		up->Tflags.tms_cach_write = 1;
	}

	/*
	 * If a tape mark has been encountered, clear the serious
	 * exception for reads which do not use n-buffered I/O.
	 * This allows the following functionality as described in mtio(4):
	 * "A zero byte count is returned when a tape mark is read, but 
	 * another read will fetch the first record of the next tape file."
	 */
	if ((up->Tflags.tms_bufmark) && (mp->mscp_opcode == MSCP_OP_READ) &&
	    ((bp->b_flags & B_RAWASYNC) == 0) && (up->Tflags.tms_clserex)) {
		up->Tflags.tms_bufmark = 0;
		up->Tflags.tms_clserex = 0;
		mp->mscp_modifier |= MSCP_MD_CLSEX;
	}

	mp->mscp_byte_cnt = bp->b_bcount;
	Move_bhandle( rp->lbhandle, mp->mscp_buffer[ 0 ] ); 
	return( mscp_send_msg( rp ));
	
}


/*
 *
 *   Name:	mscp_transferem - process a data transfer end message.
 *
 *   Abstract:	
 *
 *   Inputs:	rp			Request block pointer.
 *
 *   Outputs:	NONE
 *
 *
 *   Return	NONE.
 *   Values:
 */
u_long
tmscp_transferem( event, rp )
	u_long			event;
	register REQB		*rp;
{
	register MSCP		*mp = rp->msgptr;
	register struct buf	*bp = rp->bufptr;
	UNITB			*up = rp->unitb;
	u_short			em_status = mp->mscp_status & MSCP_ST_MASK;

	/* Default the written flag to a read
	 */
	up->tms_flags &= ~DEV_WRITTEN;

	/* Check to see if the transfer succeeded.
	 */
	(void) check_return(rp);
	switch (em_status) {
	case MSCP_ST_SUCC:
		/* Remember that the tape was written on and close will put down
		 * tape EOT/EOF
		 */
		if((up->tms_endcode & ~MSCP_OP_END) == MSCP_OP_WRITE) {
			up->Tflags.tms_write = 1;
			up->tms_flags |= DEV_WRITTEN;	/* for status ioctls */
		}
		/*
		 * Transitions from write to read commands will cause the 
		 * controller to flush it's cache.  Clear tms_cach_write to 
		 * indicat that there are no write records pending in the 
		 * controller's write-back cache.
		 */
		else {
			up->Tflags.tms_cach_write = 0;
		}
		break;
	/*
	 * TODO: The check_return routine does not handle all the possible
	 * data error cases.  As such the user application will see a return
	 * value of 0 instead of error.  To correct this problem a default case
	 * needs to be added to this switch statement to flag all non success
	 * status commands as errors.  This needs a little thought for the
	 * case of a partial transfer.  At this time it is too late in the
 	 * release cycle for a fix.
	 */
	}

	/* Calculate the bytes not transferred, complete the I/O,
	 * and deallocate the request block and all the resources
	 * that it holds.
	 * Save the residual value which is reterned via MTIOCGET.
	 * Put the residual count into rp->p1 to be used by mscp_dealloc_reqb
	 * to stuff into the resid field of the buf struct.
	 * Save the position for use by recovery code.
	 *
	 * If we encounter a serious exception the value in
	 * mscp_byte_cnt should be zero.  The TQK50 does not
	 * clear this field so we'll treat it as zero.
	 */

        if((up->tms_endcode & ~MSCP_OP_END) == MSCP_OP_ACCESS)
            up->tms_resid = bp->b_bcount - mp->mscp_taperec;
        else 
	    if(em_status != MSCP_ST_SEX)
	        up->tms_resid = bp->b_bcount - mp->mscp_byte_cnt;
	    else
	        up->tms_resid = bp->b_bcount;

	rp->p1 = up->tms_resid;
	up->tms_position = mp->mscp_position;
	if (up->tms_position != 0)
		up->tms_flags &= ~DEV_BOM;
	mscp_dealloc_reqb( rp );
	return( EV_NULL );
}



/*
 *
 *   Name:	tmscp_writetmcm
 *
 *   Abstract:	A tape mark is written to the specified unit beginning
 *		at the current position.
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_writetmcm( event, rp)
	u_long		event;
	register REQB	*rp;
{
	UNITB			*up = rp->unitb;
	register MSCP		*mp = rp->msgptr;
	Dprint7("tmscp_writetmcm\n");

	/* Format the message buffer as a WRITE TAPE MARK command.  
	 * Queue the message for transmission.
	 */
	Init_msg( mp, rp->rspid, rp->unitb->unit );
	mp->mscp_opcode = MSCP_OP_WRITM;
	mp->mscp_modifier |= MSCP_MD_CLSEX;
	up->Tflags.tms_clserex = 0;
	/*
	 * If the unit is in a cache data loss condition, clear that state if
	 * there are no writes waiting to be flushed to media.  Otherwise leave
	 * the unit in cache data loss state to cause future data transfer
	 * commands to fail.
	 */
	if (up->Tflags.tms_cache_lost) {
		if (up->Tflags.tms_cach_write == 0) {
			up->Tflags.tms_cache_lost = 0;
			/* clear cache data lost exception */
			mp->mscp_modifier |= MSCP_MD_CDATL; 
			Dprint2("Cache loss on empty write cache -9\n");
		}
		else {
			Dprint2("Cache data loss condition -9\n");
		}
	}
	return( mscp_send_msg( rp ));
}


/*
 *
 *   Name:	tmscp_writetmem
 *
 *   Abstract:	
 *
 *   Inputs:
 *
 *   Outputs:
 *
 *
 *   Return
 *   Values:
 */
u_long
tmscp_writetmem( event, rp)
	u_long		event;
	register REQB	*rp;
{
	register MSCP	*mp = rp->msgptr;
	UNITB		*up = rp->unitb;
	u_short		em_status = mp->mscp_status & MSCP_ST_MASK;

	Dprint7("tmscp_writetmem\n");
	up->Tflags.tms_wait = 0;
	if( em_status == MSCP_ST_SUCC ) {
		up->flags.wrtp = 0;
		/*
		 * Save the position for use by recovery code.
		 */
		up->tms_position = mp->mscp_position;
	}
	(void) check_return( rp );

	/* Deallocate the request and all the resources that it holds.
	 */
	mscp_dealloc_reqb( rp );
	return( EV_NULL );

}


/*
 *
 *   Name:	check_return
 *
 *   Abstract:	
 *	Check the status return code, sub-codes and flags.  Set information
 *	data elements in the control blocks to aid the handling of future
 *	commands.  For instance, if a tape marke is encountered set the 
 *	clserex flag so the next command will clear the exception before
 *	it runs or position lost flag which will cause all further transactions
 *	to fail.
 *
 *   Inputs:
 *	Request block
 *
 *   Outputs:
 *
 *
 *   Return
 */
#define Berror(Error) if(bp != NULL) { \
			bp->b_flags |= B_ERROR; \
			bp->b_error = Error;  \
			}
#define Log_error if( !(em_flags & MSCP_EF_ERLOG) ) mprintf
check_return( rp )
	register 	REQB	*rp;
{
	register MSCP		*mp = rp->msgptr;
	register struct buf	*bp = rp->bufptr;
	UNITB			*up = rp->unitb;
	CONNB			*cp = rp->connb;
	u_short			em_status = mp->mscp_status & MSCP_ST_MASK;
	u_short			em_subcode = mp->mscp_status >> MSCP_ST_SBBIT;
	u_short			em_flags = mp->mscp_flags;
	u_short			em_endcode = mp->mscp_endcode & ~MSCP_OP_END;

	if(( em_status != MSCP_ST_SUCC) && (tmscpdebug))
		Dprint3("check_return\n\tstatus 0x%x\n\tsubcode 0x%x\n\tflags 0x%x\n\tendcode 0x%x\n",em_status
		,em_subcode
		,em_flags
		,em_endcode);
	
	/* remember opcode that compleated for ioctl status 
	 */
	up->tms_endcode = mp->mscp_endcode;


	/* Check end message flags and set unit flags for later use */
	if( em_flags & MSCP_EF_EOT ) {
		up->tms_flags |= DEV_EOM;	/* Turn on EOM flag */
	} else {
		up->tms_flags &= ~DEV_EOM;	/* Turn off EOM flag */
	}
	if( em_flags & MSCP_EF_DLS ) {
		up->Tflags.tms_cache_lost = 1;
		Dprint2("Detected cache data loss\n");
	}
	/*
	 * The unit can enter the position lost STATE if an error occurs
	 * which causes a command to return a status of MSCP_ST_PLOST; another
	 * way to get into this state is with a cache data loss condition.
	 * Once you are in a position lost state, the drive will remain in this
	 * state until:	1) Rewind to BOT, 2) unit goes offline 3) tape unloaded
	 * via available command.  While in position lost state, the position
	 * lost flag will be set in end messages.  This can be considered
	 * "noise" and does not necessarily preclude command failure.
	 *
	 * At this time, setting tms_lost doesn't do anything because it is
	 * never looked at.
	 */
	if( em_flags & MSCP_EF_PLS )
		up->Tflags.tms_lost = 1;
 
	/* Store away the status information for use by the MTIOGET ioctl
	 */
	up->tms_status = mp->mscp_status;

	switch (em_status) {
	case MSCP_ST_SUCC:
		switch(em_subcode) {
			case MSCP_SC_NORML:	/* Normal success state */
				break;

			case MSCP_SC_STCON:	/* Unit still connected, avail
						 * issued to a non-online drive
						 */
				Log_error( "tms - %d still connected \n on controller %d\n", up->unit ,cp->cnt_number);
				break;

			case MSCP_SC_DUPUN:	/* Duplicate unit number */
				Log_error( "tms - Duplicate unit %d detected\n on controller %d\n", up->unit ,cp->cnt_number);
				Berror( ENXIO);
				Dprint3("check_return: ENXIO 1 in buf\n");
				break;

			case MSCP_SC_ALONL:	/* Unit already online	*/
				up->flags.online = 1;
				Log_error( "tms - %d already on line\n on controller %d\n", up->unit ,cp->cnt_number);
				break;

			case MSCP_SC_STONL:	/* Unit still online	*/
				up->flags.online = 1;
				Log_error( "tms - %d still online \n on controller %d\n", up->unit ,cp->cnt_number);
				break;

			case MSCP_SC_UNIGN:	/* Unit still online, unload ignored */
				up->flags.online = 1;
				Log_error( "tms - %d still connected unload ignored\n on controller %d\n", up->unit ,cp->cnt_number);
				break;

			case MSCP_SC_EOT:	/* End of tape (EOT) encountered */
				up->tms_flags |= DEV_EOM;	/* Turn on EOM flag */
				/* If EOM detection is off set up is to let all
				 * transfers complete.
				 */
				if(up->Tflags.tms_eom)
					up->Tflags.tms_clserex = 1;
				break;

			case MSCP_SC_ROVOL:	/* Read-only Volume Format */
				up->flags.online = 1;
				Log_error( "tms - %d Read-only Volume Format \n on controller %d\n", up->unit ,cp->cnt_number);
				break;

			default:		/* Unknown subcode ???? */
				Log_error( "tms - unit %d unknown success subcode \nendcode 0x%x subcode 0x%x flags 0x%x\n controller %d\n", 
					up->unit , em_endcode, em_subcode, em_flags, cp->cnt_number);

				break;
		}
		break;

	/* An error of some kind occurred during the data transfer.
 	* Mark the buf structure accordingly.
	*/
	case MSCP_ST_OFFLN:		/* Unit is offline	*/
		up->flags.online = 0;
		up->tms_category_flags &= ~DEV_RWDING;
		/*
		 * If the controller's write back cache is being used and 
		 * there could still be outstanding records which have not been
		 * flushed to the media the assumption is made that a cache
		 * data loss  condition has occured since there is no way to 
		 * know otherwise.
		 *
		 * Since the unit is offline the MTFLUSH command will not 
		 * succeed.  This will cause the utility to assume that a
		 * cache data loss has occured.  If the utility doesn't do a
		 * MTFLUSH then it can't be assured that the data is on the 
		 * media.
		 */
		if (up->Tflags.tms_cach_on && up->Tflags.tms_cach_write) {
			Log_error("tms - potential offline cache data loss, unit %d, controller %d\n",up->unit ,cp->cnt_number);
		}
		switch(em_subcode) {
			case  MSCP_SC_DUPUN:	/* Duplicate unit number */
				Log_error( "tms - Duplicate unit %d detected\n on controller %d\n", up->unit ,cp->cnt_number);
				Berror( ENXIO);
				Dprint3("check_return: ENXIO in buf 2\n");
				break;

			case MSCP_SC_UNKNO:	/* Unknown unit or alive
						 * to another unit
						 */
				Log_error( "tms - unknown unit %d\n", up->unit );
				Berror( ENXIO);
				Dprint3("check_return: ENXIO in buf 3\n");
				break;

			case MSCP_SC_NOVOL:	/* No volume mounted or offln */
				/* if an open was issued then the online_ip fla
				 * should be set which means we expect to see
				 * tape mounted.  If the drive is powered off
				 * and we are polling we can flood the errorlog
				 * so no logs are issued.
				 *
				 * This used to call Log_error if (up->flags.
				 * online_ip == 1), this is not really an
				 * unusual event.
				 */
				Dprint3( "check return: tms - No volume mounted or drive unit %d powered off\n", up->unit );
				Berror( ENXIO);
				break;

			case MSCP_SC_INOPR:	/* Inoperative unit */
				Log_error( "tms - unit %d inoperative\n on controller %d\n", up->unit ,cp->cnt_number);
				Berror( ENXIO);
				Dprint3("check_return: ENXIO in buf 5\n");
				break;

			case MSCP_SC_UDSBL:	/* disabled by field service */
				Log_error( "tms - unit %d on controller %d\ndisabled by field service\n", up->unit ,cp->cnt_number);
				Berror( ENXIO);
				Dprint3("check_return: ENXIO in buf 6\n");
				break;

			case MSCP_SC_EXUSE:	/* unit claimed by other host */
				Log_error( "tms - unit %d on controller %d\nexclusive use set\n", up->unit ,cp->cnt_number);
				Berror( ENXIO);
				Dprint3("check_return: ENXIO in buf 6.5\n");
				break;

			case MSCP_SC_LOADE:	/* loader cycle error */
				Log_error( "tms - unit %d on controller %d\nLoader cycle error\n", up->unit ,cp->cnt_number);
				Berror( ENXIO);
				Dprint3("check_return: ENXIO in buf 6.6\n");
				break;

			default:		/* Unknown sub-code ??? */
				Log_error( "tms - unit %d unknown offline endcode 0x%x subcode 0x%x\n controller %d\n", 
					up->unit , em_endcode, em_subcode,cp->cnt_number);
				Berror( EIO);
				Dprint3("check_return: EIO 2 Berror\n");
				break;
		}
		break;

	case MSCP_ST_WRTPR:			/* Unit is write protected */
		up->tms_flags |= DEV_WRTLCK;
		switch (em_subcode) {
		case MSCP_SC_SOFTW:	/* Software write protected */
			up->flags.wrtp = 1; /* set write protect flag */
			break;

		case MSCP_SC_HARDW:	/* Hardware write protected */
			up->flags.wrtp = 1; /* set write protect flag */
			break;
		case MSCP_SC_DATL:	/* Data Safety write protected, for
					 * example a TK50 in a TK70 drive. */
			up->flags.wrtp = 1; /* set write protect flag */
			break;
		}
		Dprint2("check_return: EACCES in buf \n");
		Berror( EACCES);
		break;

	case MSCP_ST_ICMD:	/* Byte count greater than max allowed
				 */
		switch (em_subcode) {
		case MSCP_SC_INVML:	/* Invalid message length */
			Berror( EIO);
			Dprint2("check_return: EIO 3 Berror\n");
			break;
		}
		break;

	case MSCP_ST_ABRTD:		/* Command aborted	*/
		Log_error( "tms - unit %d command aborted (endcode 0x%x)\n on controller %d\n", up->unit ,em_endcode ,cp->cnt_number);
		Berror( EIO);
		Dprint2("check_return: EIO 4 Berror\n");
		break;

	case MSCP_ST_AVLBL:		/* Unit is available */
		/* The unit went off line; clear error conditions to insure
		 * correct values when the unit comes back online and mark the 
		 * unit off line.
		 */
		up->flags.online = 0;
		up->Tflags.tms_lost = 0;
		up->Tflags.tms_clserex = 0;
		up->Tflags.tms_serex = 0;
		up->tms_category_flags &= ~DEV_RWDING;
		break;

	case MSCP_ST_COMP:	/* Tape record data and host data buffer
				 * differ.
				 */
		Log_error( "tms - unit %d data compare error\n on controller %d\n", up->unit ,cp->cnt_number);
		Berror( EIO);
		Dprint2("check_return: EIO 5 Berror\n");
		break;	

	case MSCP_ST_DATA: 	/* Data error	*/
		switch(em_subcode) {
			case MSCP_SC_LGAP:	/* A long record gap was found
						 * on a ACCESS, COMPARE HOST DATA
						 * READ, or REPOSITION
						 */
				Log_error( "tms - unit %d long gap data error\n on controller %d\n", up->unit ,cp->cnt_number);
				Berror( EIO);
				Dprint2("check_return: EIO 6 Berror\n");
				break;

			case MSCP_SC_UREAD:	/* Unrecoverable read error,
						 * tape format unknown or non-
						 * recoverable data error obtained
						 * from tape unit
						 */
				Log_error( "tms - unit %d unrecoverable data error\n on controller %d\n", up->unit ,cp->cnt_number);
				Berror( EIO);
				Dprint2("check_return: EIO 7 Berror\n");
				break;
		}
		break;

	case MSCP_ST_HSTBF:	/* Host buffer access error	*/
		Log_error( "tms - unit %d host buffer access error (endcode 0x%x)\n on controller %d\nsubcode 0x%x\nbp 0x%x", up->unit ,em_endcode,cp->cnt_number,em_subcode, bp);
		Berror( EIO);
		Dprint2("check_return: EIO 8 Berror\n");
		break;

	case MSCP_ST_CNTLR:	/* Controller error	*/
		Log_error( "tms - unit %d controller error (endcode 0x%x)\n on controller %d\n", up->unit, em_endcode,cp->cnt_number);
		Berror( EIO);
		Dprint2("check_return: EIO 9 Berror\n");
		break;

	case MSCP_ST_FMTER:	/* Formatter error	*/
		Log_error( "tms - unit %d formatter error\n on controller %d\n", up->unit ,cp->cnt_number);
		Berror( EIO);
		Dprint2("check_return: EIO 10 Berror\n");
		break;

	case MSCP_ST_BOT:	/* Bottom of tape (BOT) encountered */
		up->tms_category_flags &= ~DEV_RWDING;
		up->tms_flags |= DEV_BOM;
		up->tms_position = 0;
		break;

	case MSCP_ST_TAPEM:		/* Serex from a tape mark, 
					 * can be cleared
					 */
		up->Tflags.tms_bufmark = 1;
		up->Tflags.tms_clserex = 1;
		break;

	case MSCP_ST_SEX:	/* A real serex occured and not buffered io */
		/* If EOM handling is enabled 
		 * return no space.
		 */
		if(( em_flags & MSCP_EF_EOT ) && (up->Tflags.tms_eom == 0)){
			Berror( ENOSPC);
			up->tms_flags |= DEV_EOM;	/* Turn on EOM flag */
			Dprint2("check_return MSCP_ST_SEX ENOSPC\n");
		}
		if( em_flags & MSCP_EF_EOT == 0){
			up->Tflags.tms_clserex = 0;
			Log_error( "tms - unit %d serious exception encountered\non controller %d\nendcode 0x%x subcode 0x%x\nflags 0x%x\n", 
			up->unit ,cp->cnt_number,em_endcode, em_subcode,em_flags);
			Dprint2("check_return: EIO Berror\n");
			Berror( EIO);
		}
		break;

	case MSCP_ST_PLOST:		
					/* The position on the tape is unknown
					 * the position lost condition is in
					 * effect until the tape is repositioned
					 * back to BOT
					 */
		Log_error( "tms - unit %d position lost\n on controller %d\n", 
			up->unit ,cp->cnt_number);
		up->Tflags.tms_lost = 1;
		up->Tflags.tms_serex = 1;
		up->Tflags.tms_clserex = 0;
		Berror( EIO);
		Dprint2("check_return: EIO 11 Berror\n");
		break;

	case MSCP_ST_LED:		
					/* 
					 * Logical end of tape detected as an 
					 * error.  Serios exception will be
					 * set.
					 */
		up->Tflags.tms_clserex = 1;
		Dprint2("check_return: EIO 12 Berror\n");
		break;

	case MSCP_ST_MFMTE:		
					/* 
					 * Media Format detected as an 
					 * error.  Serios exception will be
					 * set.
					 */
		Log_error( "tms - unit %d Media Format error detected\n on controller %d\n", 
			up->unit ,cp->cnt_number);
		up->Tflags.tms_serex = 1;
		up->Tflags.tms_clserex = 0;
		Berror( EIO);
		Dprint2("check_return: EIO 13.1 Berror\n");
		break;

	case MSCP_ST_DRIVE:		
					/* 
					 * Drive error detected 
					 * Serios exception will be
					 * set.
					 */
		Log_error( "tms - unit %d Drive error detected\n on controller %d\n", 
			up->unit ,cp->cnt_number);
		up->Tflags.tms_serex = 1;
		up->Tflags.tms_clserex = 0;
		Berror( EIO);
		Dprint2("check_return: EIO 13.2 Berror\n");
		break;

	case MSCP_ST_RDTRN:		
					/* 
					 * Record Data Truncated detected as an 
					 * error.  Serios exception will be
					 * set.
					 */
		Log_error( "tms - unit %d Record Data truncated \n on controller %d\n", 
			up->unit ,cp->cnt_number);
		up->Tflags.tms_serex = 1;
		up->Tflags.tms_clserex = 0;
		Berror( EIO);
		Dprint2("check_return: EIO 13.3 Berror\n");
		break;

	case MSCP_ST_DIAG:		
					/* 
					 * Internal Diagnostic detected 
					 */
		Log_error( "tms - unit %d Internal Diagnostic detected\n on controller %d\n", 
			up->unit ,cp->cnt_number);
		Berror( EIO);
		Dprint2("check_return: EIO 14 Berror\n");
		break;

	case MSCP_ST_LOADR: 	/* Media loader error */
		/*
		 * Unclear what to do with this error.  Perhaps retrun an
		 * error when loader support is added.
		 * Loader subcodes found in mscp spec table B-10
		 */
		Log_error( "tms - unit %d media loader error , controller %d\n",
			up->unit ,cp->cnt_number);
		Dprint2("check_return: media loader error\n");
		Berror( EIO);
		switch(em_subcode) {
			case MSCP_SC_LTIMO:	/* Loader command time out */
				Log_error( "tms - Loader command time out\n");
				break;
			case MSCP_SC_LTERR:	/* Loader controller
						 * transmission error */
				Log_error( "tms - Loader controller transmission error\n");
				break;
			case MSCP_SC_LPERR:	/* Loader controller 
						 * protocol error */
				Log_error( "tms - Loader controller protocol error\n");
				break;
			case MSCP_SC_LDERR:	/* Loader error */
				Log_error( "tms - Loader error\n");
				break;
			case MSCP_SC_LDONL:	/* Loader online */
				Log_error( "tms - Loader online\n");
				break;
			case MSCP_SC_INVSRC:	/* Invalid source slot ID */
				Log_error( "tms - Invalid source slot ID\n");
				break;
			case MSCP_SC_INVDST:	/* Invalid destination slot ID*/
				Log_error( "tms - Invalid destination slot ID\n");
				break;
			case MSCP_SC_SRCEMP:	/* Source slot is empty */
				Log_error( "tms - Source slot is empty \n");
				break;
			case MSCP_SC_DSTFUL:	/* Destination slot is full */
				Log_error( "tms - Destination slot is full \n");
				break;
			case MSCP_SC_LDMOT:	/* Loader motion error */
				Log_error( "tms - Loader motion error \n");
				break;
			case MSCP_SC_LDDRIE:	/* Loader/drive interface err */
				Log_error( "tms - Loader-drive interface error \n");
				break;
			case MSCP_SC_LDSLIE:	/* Loader/slot interface error*/
				Log_error( "tms - Loader-slot interface error \n");
				break;
			case MSCP_SC_LDMECH:	/* Loader mechanical error */
				Log_error( "tms - Loader mechanical error \n");
				break;
			case MSCP_SC_LDHARD:	/* Loader hardware error */
				Log_error( "tms - Loader hardware error \n");
				break;
			case MSCP_SC_LDCTLR:	/* Loader controller error */
				Log_error( "tms - Loader controller error \n");
				break;
			case MSCP_SC_LDUNRE:	/* Unrecognized loader command*/
				Log_error( "tms - Unrecognized loader command \n");
				break;
			case MSCP_SC_LDREXC:	/* Exception - recoverable by 
						 * RESET */
				Log_error( "tms - Exception - reset recoverable \n");
				break;
			case MSCP_SC_LDUEXC:	/* Exception - unrecoverable */
				Log_error( "tms - Unrecoverable exception \n");
				break;
			default:		/* Unknown sub-code ??? */
				Log_error( "tms - unit %d unknown loader subcode 0x%x\n",
					up->unit , em_subcode);
				Dprint2("check_return: unknown loader subcode 0x%x\n",em_subcode);
				break;
		}
		break;

	case MSCP_ST_IPARM:		
					/* 
					 * Invalid parameter
					 */
		Log_error( "tms - unit %d Invalid parameter on controller %d\n",
			up->unit ,cp->cnt_number);
		Berror( EIO);
		Dprint2("check_return: EIO 15 Berror\n");
		break;

	default:			/* Unknown status code ??? */
		Dprint2("check_return status fell through as DEFAULT value?\n");
		Berror( EIO);
		Dprint2("check_return: EIO 12 Berror\n");
		break;
	}
}

#undef Berror

