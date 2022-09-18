#ifndef	lint
static char *sccsid = "@(#)mscp_diskvar.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
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
 *   Author:	David E. Eiche	Creation Date:	March 11, 1988
 *
 *   History:
 *
 *   23-Oct-1989	Tim Burke
 *	Added state tables to allow for a set unit characteristics command.
 *
 *   07-Mar-1989	Todd M. Katz		TMK0001
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   08-Jul-1988	Pete Keilty
 *	Added accscan state table.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   03-Apr-1988	David E. Eiche		DEE0022
 *	Remove reference to mscp_onlineinit routine.  This is part of
 *	a larger fix to eliminate an open/close race condition.
 *
 *   11-Mar-1988	David E. Eiche
 *	Moved disk- specific MSCP modules from mscp_var.c into (this)
 *	separate module.  Audit trail information is retained in
 *	mscp_var.c.
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
extern  u_long		mscp_accscancm(),
			mscp_accscanem(),
			mscp_alloc_msg(),
			mscp_alloc_rspid(),
			mscp_map_buffer(),
			mscp_availcm(),
			mscp_forcecm(),
			mscp_forceem(),
			mscp_invevent(),
			mscp_markoffline(),
			mscp_markonline(),
			mscp_noaction(),
			mscp_onlgtuntem(),
			mscp_onlinecm(),
			mscp_onlineem(),
			mscp_recovinit(),
			mscp_recovnext(),
			mscp_setunitcm(),
			mscp_setunitem(),
			mscp_transfercm(),
			mscp_transferem();

/**/

/* Bring unit online states.
 */
STATE mscp_onl_states[] = {

    /* Unit online initial state.
     */
    { ST_ON_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_ON_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_ON_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_ON_INITIAL,	mscp_onlinecm },		/* EV_MSGBUF	      */
    { ST_ON_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_ONLIN,	mscp_onlineem },		/* EV_ENDMSG	      */
    { ST_ON_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_ON_INITIAL,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_ON_INITIAL,	mscp_invevent },		/* 		      */
    { ST_ON_INITIAL,	mscp_invevent },		/* 		      */
    { ST_ON_INITIAL,	mscp_invevent },		/*		      */
    { ST_ON_INITIAL,	mscp_invevent },		/*		      */
    { ST_ON_INITIAL,	mscp_invevent },		/*		      */
    { ST_ON_INITIAL,	mscp_invevent },		/* EV_ONLCOMPLETE     */
    { ST_ON_INITIAL,	mscp_invevent },		/* EV_ONLERROR	      */

    /* Unit online - online end message processing
     */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_NULL	      */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_RSPID	      */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_GTUNT,	mscp_onlgtuntem },		/* EV_ENDMSG	      */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_ON_ONLIN,	mscp_invevent },		/*		      */
    { ST_ON_ONLIN,	mscp_invevent },		/*		      */
    { ST_ON_ONLIN,	mscp_invevent },		/*		      */
    { ST_ON_ONLIN,	mscp_invevent },		/*		      */
    { ST_ON_AVAIL,	mscp_availcm },			/* EV_ONLERRAVAIL     */
    { ST_ON_ONLIN,	mscp_invevent },		/* EV_ONLCOMPLETE     */
    { ST_ON_AVAIL,	mscp_markoffline },		/* EV_ONLERROR	      */

    /* Unit online available end message processing.
     */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_NULL	      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_RSPID	      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_AVAIL,	mscp_markoffline },		/* EV_ENDMSG	      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_ON_AVAIL,	mscp_invevent },		/*		      */
    { ST_ON_AVAIL,	mscp_invevent },		/*		      */
    { ST_ON_AVAIL,	mscp_invevent },		/*		      */
    { ST_ON_AVAIL,	mscp_invevent },		/*		      */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_ONLERRAVAIL     */
    { ST_ON_AVAIL,	mscp_invevent },		/* EV_ONLCOMPLETE     */
    { ST_ON_AVAIL,	mscp_noaction },		/* EV_ONLERROR	      */

    /* Unit online get unit status end message processing.
     */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_NULL	      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_RSPID	      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_ON_GTUNT,	mscp_markonline },		/* EV_ERRECOV	      */
    { ST_ON_GTUNT,	mscp_invevent },		/*		      */
    { ST_ON_GTUNT,	mscp_invevent },		/*		      */
    { ST_ON_GTUNT,	mscp_invevent },		/*		      */
    { ST_ON_GTUNT,	mscp_invevent },		/*		      */
    { ST_ON_GTUNT,	mscp_invevent },		/* EV_ONLERRAVAIL     */
    { ST_ON_GTUNT,	mscp_markonline },		/* EV_ONLCOMPLETE     */
    { ST_ON_INITIAL,	mscp_onlinecm },		/* EV_ONLERROR	      */
};

/**/

/* Make unit available states.
 */
STATE mscp_avl_states[] = {

    /* Unit available initial state.
     */
    { ST_AV_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_AV_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_AV_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_AV_INITIAL,	mscp_availcm },			/* EV_MSGBUF	      */
    { ST_AV_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_AV_INITIAL,	mscp_markoffline },		/* EV_ENDMSG	      */
    { ST_AV_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_AV_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_AV_INITIAL,	mscp_invevent },		/*		      */
    { ST_AV_INITIAL,	mscp_invevent },		/*		      */
    { ST_AV_INITIAL,	mscp_invevent },		/*		      */
    { ST_AV_INITIAL,	mscp_invevent },		/*		      */
    { ST_AV_INITIAL,	mscp_invevent },		/*		      */
    { ST_AV_INITIAL,	mscp_invevent },		/*		      */
    { ST_AV_INITIAL,	mscp_invevent },		/* EV_AVLCOMPLETE     */
    { ST_AV_INITIAL,	mscp_noaction },		/* EV_AVLERROR	      */
};

/**/

/* Forced replacement states.
 */
STATE mscp_repl_states[] = {

    /* Forced replacement initial state.
     */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_RPL_INITIAL,	mscp_alloc_msg },		/* EV_INITIAL	      */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_RSPID	      */
    { ST_RPL_INITIAL,	mscp_forcecm },			/* EV_MSGBUF	      */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_RPL_INITIAL,	mscp_forceem },			/* EV_ERRECOV	      */
    { ST_RPL_INITIAL,	mscp_invevent },		/*		      */
    { ST_RPL_INITIAL,	mscp_invevent },		/*		      */
    { ST_RPL_INITIAL,	mscp_invevent },		/*		      */
    { ST_RPL_INITIAL,	mscp_invevent },		/*		      */
    { ST_RPL_INITIAL,	mscp_invevent },		/*		      */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_AVLCOMPLETE     */
    { ST_RPL_INITIAL,	mscp_invevent },		/* EV_AVLERROR	      */
};

/**/

/* Data transfer states.
 */
STATE mscp_xfr_states[] = {

    /* Unit online initial state.
     */
    { ST_XF_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_XF_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_XF_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_XF_INITIAL,	mscp_map_buffer },		/* EV_MSGBUF	      */
    { ST_XF_INITIAL,	mscp_transfercm },		/* EV_MAPPING	      */
    { ST_XF_INITIAL,	mscp_transferem },		/* EV_ENDMSG	      */
    { ST_XF_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_XF_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_XF_INITIAL,	mscp_transfercm },		/* EV_ERRECOV	      */
    { ST_XF_INITIAL,	mscp_invevent },		/*		      */
    { ST_XF_INITIAL,	mscp_invevent },		/*		      */
    { ST_XF_INITIAL,	mscp_invevent },		/*		      */
    { ST_XF_INITIAL,	mscp_invevent },		/*		      */
    { ST_XF_INITIAL,	mscp_invevent },		/*		      */
    { ST_XF_INITIAL,	mscp_invevent },		/*		      */
    { ST_XF_INITIAL,	mscp_invevent },		/*		      */
};

/**/

/* Set unit characteristics states.
 */
STATE mscp_stu_states[] = {

    /* Unit online initial state.
     */
    { ST_STU_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_STU_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_STU_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_STU_INITIAL,	mscp_setunitcm },		/* EV_MSGBUF	      */
    { ST_STU_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_STU_INITIAL,	mscp_setunitem },		/* EV_ENDMSG	      */
    { ST_STU_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_STU_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_STU_INITIAL,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_STU_INITIAL,	mscp_invevent },		/*		      */
    { ST_STU_INITIAL,	mscp_invevent },		/*		      */
    { ST_STU_INITIAL,	mscp_invevent },		/*		      */
    { ST_STU_INITIAL,	mscp_invevent },		/*		      */
    { ST_STU_INITIAL,	mscp_invevent },		/*		      */
    { ST_STU_INITIAL,	mscp_invevent },		/*		      */
    { ST_STU_INITIAL,	mscp_invevent },		/*		      */
};

/*^L*/

/* Access scan states.
 */
STATE mscp_accscan_states[] = {

    /* Unit online initial state.
     */
    { ST_ACC_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_ACC_INITIAL,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_ACC_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_ACC_INITIAL,	mscp_accscancm },		/* EV_MSGBUF	      */
    { ST_ACC_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_ACC_INITIAL,	mscp_accscanem },		/* EV_ENDMSG	      */
    { ST_ACC_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_ACC_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_ACC_INITIAL,	mscp_forceem },			/* EV_ERRECOV	      */
    { ST_ACC_INITIAL,	mscp_invevent },		/*		      */
    { ST_ACC_INITIAL,	mscp_invevent },		/*		      */
    { ST_ACC_INITIAL,	mscp_invevent },		/*		      */
    { ST_ACC_INITIAL,	mscp_invevent },		/*		      */
    { ST_ACC_INITIAL,	mscp_invevent },		/*		      */
    { ST_ACC_INITIAL,	mscp_invevent },		/*		      */
    { ST_ACC_INITIAL,	mscp_invevent },		/*		      */
};

/**/

/* Unit recovery states.
 */
STATE mscp_rec_states[] = {

    /* Unit online initial state.
     */
    { ST_RE_INITIAL,	mscp_invevent },		/* EV_NULL	      */
    { ST_RE_INITIAL,	mscp_recovinit },		/* EV_INITIAL	      */
    { ST_RE_INITIAL,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_RE_INITIAL,	mscp_onlinecm },		/* EV_MSGBUF	      */
    { ST_RE_INITIAL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_RE_ONLIN,	mscp_onlineem },		/* EV_ENDMSG	      */
    { ST_RE_INITIAL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_RE_INITIAL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_RE_INITIAL,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_RE_INITIAL,	mscp_invevent },		/* 		      */
    { ST_RE_INITIAL,	mscp_invevent },		/* 		      */
    { ST_RE_INITIAL,	mscp_invevent },		/*		      */
    { ST_RE_INITIAL,	mscp_invevent },		/*		      */
    { ST_RE_INITIAL,	mscp_invevent },		/*		      */
    { ST_RE_INITIAL,	mscp_invevent },		/* EV_ONLCOMPLETE     */
    { ST_RE_INITIAL,	mscp_invevent },		/* EV_ONLERROR	      */

    /* Unit recovery - online end message processing
     */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_NULL	      */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_RSPID	      */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_RE_GTUNT,	mscp_onlgtuntem },		/* EV_ENDMSG	      */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_RE_ONLIN,	mscp_invevent },		/*		      */
    { ST_RE_ONLIN,	mscp_invevent },		/*		      */
    { ST_RE_ONLIN,	mscp_invevent },		/*		      */
    { ST_RE_ONLIN,	mscp_invevent },		/*		      */
    { ST_RE_AVAIL,	mscp_availcm },			/* EV_ONLERRAVAIL     */
    { ST_RE_ONLIN,	mscp_invevent },		/* EV_ONLCOMPLETE     */
    { ST_RE_AVAIL,	mscp_markoffline },		/* EV_ONLERROR	      */

    /* Unit recovery available end message processing.
     */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_NULL	      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_RSPID	      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_RE_AVAIL,	mscp_markoffline },		/* EV_ENDMSG	      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_RE_AVAIL,	mscp_invevent },		/*		      */
    { ST_RE_AVAIL,	mscp_invevent },		/*		      */
    { ST_RE_AVAIL,	mscp_invevent },		/*		      */
    { ST_RE_AVAIL,	mscp_invevent },		/*		      */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_ONLERRAVAIL     */
    { ST_RE_AVAIL,	mscp_invevent },		/* EV_ONLCOMPLETE     */
    { ST_RE_GTUNT,	mscp_recovnext },		/* EV_ONLERROR	      */

    /* Unit recovery get unit status end message processing.
     */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_NULL	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_RSPID	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_RE_GTUNT,	mscp_recovnext },		/* EV_ERRECOV	      */
    { ST_RE_GTUNT,	mscp_invevent },		/*		      */
    { ST_RE_GTUNT,	mscp_invevent },		/*		      */
    { ST_RE_GTUNT,	mscp_invevent },		/*		      */
    { ST_RE_INITIAL,	mscp_onlinecm },		/* EV_ONLDONEXT	      */
    { ST_RE_GTUNT,	mscp_invevent },		/* EV_ONLERRAVAIL     */
    { ST_RE_GTUNT,	mscp_recovnext },		/* EV_ONLCOMPLETE     */
    { ST_RE_INITIAL,	mscp_onlinecm },		/* EV_ONLERROR	      */
};

