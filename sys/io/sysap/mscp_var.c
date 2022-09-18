#ifndef	lint
static char *sccsid = "@(#)mscp_var.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987 - 1989 by                    *
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
 *   07-Mar-1989	Todd M. Katz		TMK0002
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   20-May-1988	David E. Eiche		DEE0037
 *	Fix typo in DEE0036 which caused a state transition to CLOSED
 *	from RESTART upon receipt of an EV_INITIAL event.  Correct history
 *	comment for DEE0036.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *      removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   16-May-1988	David E. Eiche		DEE0036
 *	Make multiple changes to the connection management state table
 *	to:  handle an connection management end message arriving after
 *	connection restart; substitute a mscp_noaction for mscp_conreturn
 *	so that the latter may be removed;  ignore EV_EXRETRY in restart
 *	and other states in which it is not required to force connection
 *	recovery;  change event which causes transition from RESTART to
 *	CLOSED state:  was EV_INITIAL, is now EV_ERRECOV.
 *
 *   02-Apr-1988	David E. Eiche		DEE0020
 *	Fix connection management state table to correctly process
 *	a path failure event seen in connection restarting state.
 *
 *   07-Mar-1988	David E. Eiche		DEE0015
 *	Changed state tables to accomodate connection recovery.
 *
 *   22-Feb-1988	Robin
 *	Added entries for rd33 and ese20 disk partition sizes.
 *
 *   02-Feb-1988	Robin
 *	Changed the string constants for device types to use the defines
 *	found in devio.h.
 *
 *   26-Jan-1988	Robin Lewis
 *	Changed entries RV80 to be RV20 (the real name)
 *
 *   15-Jan-1988	Todd M. Katz		TMK0001
 *	Include new header file ../vaxmsi/msisysap.h.
 */
/**/

/* Libraries and Include Files.
 */
#include	"../h/types.h"
#include	"../h/time.h"
#include	"../h/param.h"
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
extern	u_long		mscp_alloc_msg(),
			mscp_alloc_rspid(),
			mscp_map_buffer(),
			mscp_availcm(),
			mscp_concleanup(),
			mscp_concomplete(),
			mscp_condisccmplt(),
			mscp_condealmsg(),
			mscp_conendmsg(),
			mscp_coninit(),
			mscp_conmarkopen(),
			mscp_conrestore(),
			mscp_conresynch(),
			mscp_conreturn(),
			mscp_consetretry(),
			mscp_constconcm(),
			mscp_constconem(),
			mscp_conwatchdog(),
			mscp_invevent(),
			mscp_markoffline(),
			mscp_markonline(),
			mscp_noaction(),
			mscp_onlgtuntem(),
			mscp_onlinecm(),
			mscp_onlineem(),
			mscp_onlineinit(),
			mscp_polinit(),
			mscp_polgtuntem(),
			mscp_recovinit(),
			mscp_recovnext(),
			mscp_transfercm(),
			mscp_transferem();

/**/

/* Connection management states.
 */
STATE mscp_con_states[] = {

    /* Connection uninitialized state.
     */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_CLOSED,	mscp_coninit },			/* EV_INITIAL	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_RSPID	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_CONACTIVE	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_EXRETRY	      */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_DISCOMPLETE     */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_CONCOMPLETE     */
    { ST_CN_INITIAL,	mscp_invevent },		/* EV_PATHFAILURE     */

    /* Connection closed state.
     */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_CLOSED,	mscp_noaction },		/* EV_INITIAL	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_RSPID	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_CN_CLOSED,	mscp_concomplete },		/* EV_TIMEOUT	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_RESOURCE,	mscp_alloc_rspid },		/* EV_CONACTIVE	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_CN_DEAD,	mscp_consetretry },		/* EV_EXRETRY	      */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_DISCOMPLETE     */
    { ST_CN_CLOSED,	mscp_concomplete },		/* EV_CONCOMPLETE     */
    { ST_CN_CLOSED,	mscp_invevent },		/* EV_PATHFAILURE     */

    /* Resource wait state.
     */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_RESOURCE,	mscp_noaction },		/* EV_INITIAL	      */
    { ST_CN_RESOURCE,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_CN_RESOURCE,	mscp_constconcm },		/* EV_MSGBUF	      */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_STCON1,	mscp_constconem },		/* EV_ENDMSG	      */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_TIMEOUT	      */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_CONACTIVE	      */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_EXRETRY	      */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_DISCOMPLETE     */
    { ST_CN_RESOURCE,	mscp_invevent },		/* EV_CONCOMPLETE     */
    { ST_CN_RESTART,	mscp_concleanup },		/* EV_PATHFAILURE     */

    /* First set controller characteristics wait state.
     */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_STCON1,	mscp_noaction },		/* EV_INITIAL	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_RSPID	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_STCON2,	mscp_constconem },		/* EV_ENDMSG	      */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_TIMEOUT	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_OPEN,	mscp_conmarkopen },		/* EV_CONACTIVE	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_EXRETRY	      */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_DISCOMPLETE     */
    { ST_CN_STCON1,	mscp_invevent },		/* EV_CONCOMPLETE     */
    { ST_CN_RESTART,	mscp_concleanup },		/* EV_PATHFAILURE     */

    /* Second set controller characteristics wait state.
     */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_STCON2,	mscp_noaction },		/* EV_INITIAL	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_RSPID	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_TIMEOUT	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_OPEN,	mscp_conmarkopen },		/* EV_CONACTIVE	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_EXRETRY	      */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_DISCOMPLETE     */
    { ST_CN_STCON2,	mscp_invevent },		/* EV_CONCOMPLETE     */
    { ST_CN_RESTART,	mscp_concleanup },		/* EV_PATHFAILURE     */

    /* Connection open state.
     */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_OPEN,	mscp_noaction },		/* EV_INITIAL	      */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_RSPID	      */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_OPEN,	mscp_conendmsg },		/* EV_ENDMSG	      */
    { ST_CN_OPEN,	mscp_conwatchdog },		/* EV_TIMEOUT	      */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_NOCREDITS	      */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_CONACTIVE	      */
    { ST_CN_OPEN,	mscp_conrestore },		/* EV_POLLCOMPLETE    */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_EXRETRY	      */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_DISCOMPLETE     */
    { ST_CN_OPEN,	mscp_invevent },		/* EV_CONCOMPLETE     */
    { ST_CN_RESTART,	mscp_concleanup },		/* EV_PATHFAILURE     */

    /* Connection restarting state.
     */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_RESTART,	mscp_noaction },		/* EV_INITIAL	      */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_RSPID	      */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_RESTART,	mscp_condealmsg },		/* EV_ENDMSG	      */
    { ST_CN_RESTART,	mscp_conresynch },		/* EV_TIMEOUT	      */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_CN_CLOSED,	mscp_coninit },			/* EV_ERRECOV	      */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_CONACTIVE	      */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_CN_RESTART,	mscp_noaction },		/* EV_EXRETRY	      */
    { ST_CN_RESTART,	mscp_condisccmplt },		/* EV_DISCOMPLETE     */
    { ST_CN_RESTART,	mscp_invevent },		/* EV_CONCOMPLETE     */
    { ST_CN_RESTART,	mscp_concleanup },		/* EV_PATHFAILURE     */

    /* Connection dead state.
     */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_NULL	      */
    { ST_CN_DEAD,	mscp_noaction },		/* EV_INITIAL	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_RSPID	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_CN_CLOSED,	mscp_coninit },			/* EV_TIMEOUT	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_CONACTIVE	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_CN_DEAD,	mscp_noaction },		/* EV_EXRETRY	      */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_DISCOMPLETE     */
    { ST_CN_DEAD,	mscp_noaction },		/* EV_CONCOMPLETE     */
    { ST_CN_DEAD,	mscp_invevent },		/* EV_PATHFAILURE     */
};

/**/

/* Unit polling states.
 */
STATE mscp_pol_states[] = {

    /* Unit polling initial state.
     */
    { ST_UP_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_UP_INITIAL,	mscp_alloc_rspid},		/* EV_INITIAL	      */
    { ST_UP_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_UP_INITIAL,	mscp_polinit },			/* EV_MSGBUF	      */
    { ST_UP_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_UP_INITIAL,	mscp_polgtuntem },		/* EV_ENDMSG	      */
    { ST_UP_INITIAL,	mscp_polinit },			/* EV_TIMEOUT	      */
    { ST_UP_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_UP_INITIAL,	mscp_invevent },		/*		      */
    { ST_UP_INITIAL,	mscp_invevent },		/* 		      */
    { ST_UP_INITIAL,	mscp_invevent },		/* 		      */
    { ST_UP_INITIAL,	mscp_invevent },		/* EV_POLLCOMPLETE    */
    { ST_UP_INITIAL,	mscp_invevent },		/* 		      */
    { ST_UP_INITIAL,	mscp_invevent },		/* 		      */
    { ST_UP_INITIAL,	mscp_invevent },		/*		      */
    { ST_UP_INITIAL,	mscp_invevent },		/*		      */
};

/**/

/* Class driver "master" control blocks
 */
CLASSB	mscp_classb = 0, tmscp_classb = 0;

/* Global flags common to disk and tape drivers.
 */
u_long	mscp_gbl_flags;
