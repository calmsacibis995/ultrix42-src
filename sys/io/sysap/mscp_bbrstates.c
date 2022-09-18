#ifndef	lint
static	char	*sccsid = "@(#)mscp_bbrstates.c	4.1	(ULTRIX)	7/2/90";
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
 *   Abstract:	This module contains the state tables associated with
 *		the bad block relacement portion of the disk class
 *		driver.
 *
 *   Author:	David E. Eiche	Creation Date:	October 15, 1987
 *
 *   History:
 *
 *   07-Mar-1989	Todd M. Katz		TMK0002
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   27-Jul-1988	Pete Keilty
 *	1. Changed state table states for step15, step15a now uses BBRSUCCESS
 *	   event.
 *	2. Changed step online & step0b on BBRERROR goto step12e
 *
 *   02-Jun-1988     Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   17-Apr-1988	Ricky S. Palmer
 *	Include header file "../vaxmsi/msisysap.h".
 *
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
#include	"../h/config.h"
#include	"../io/scs/sca.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/ci/ciadapter.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/uba/ubavar.h"
#include	"../io/sysap/mscp_msg.h"
#include	"../io/sysap/mscp_defs.h"
#include	"../io/sysap/mscp_bbrdefs.h"

/* External Variables and Routines.
 */
extern	u_long		mscp_alloc_msg();
extern	u_long		mscp_alloc_rspid();
extern	u_long		mscp_map_buffer();
extern	u_long		mscp_invevent();
extern	u_long		mscp_noaction();
extern	u_long		mscp_bbr_step0();
extern	u_long		mscp_bbr_step0a();
extern	u_long		mscp_bbr_step0b();
extern	u_long		mscp_bbr_step0c();
extern	u_long		mscp_bbr_step1();
extern	u_long		mscp_bbr_step4();
extern	u_long		mscp_bbr_step4a();
extern	u_long		mscp_bbr_step5();
extern	u_long		mscp_bbr_step6();
extern	u_long		mscp_bbr_step6a();
extern	u_long		mscp_bbr_step7();
extern	u_long		mscp_bbr_step7a();
extern	u_long		mscp_bbr_step7b();
extern	u_long		mscp_bbr_step7c();
extern	u_long		mscp_bbr_step8();
extern	u_long		mscp_bbr_step9();
extern	u_long		mscp_bbr_step10();
extern	u_long		mscp_bbr_step11();
extern	u_long		mscp_bbr_step11a();
extern	u_long		mscp_bbr_step11b();
extern	u_long		mscp_bbr_step11c();
extern	u_long		mscp_bbr_step12();
extern	u_long		mscp_bbr_step12a();
extern	u_long		mscp_bbr_step12b();
extern	u_long		mscp_bbr_step12c();
extern	u_long		mscp_bbr_step12d();
extern	u_long		mscp_bbr_step12e();
extern	u_long		mscp_bbr_step13();
extern	u_long		mscp_bbr_step14();
extern	u_long		mscp_bbr_step15();
extern	u_long		mscp_bbr_step15a();
extern	u_long		mscp_bbr_step16();
extern	u_long		mscp_bbr_step17();
extern	u_long		mscp_bbr_step18();
extern	u_long		mscp_bbr_step18a();
extern	u_long		mscp_rct_searcha();
extern	u_long		mscp_rct_searchb();
extern	u_long		mscp_rct_searchc();
extern	u_long		mscp_multi_read_cont();
extern	u_long		mscp_multi_write_cont();
extern	u_long		mscp_bbr_rwcont();
extern	u_long		mscp_bbr_rwfin();


/* Bad block replacement states
 */

/*
#define ST_BB_ONLINIT			0
#define ST_BB_REPINIT			1
*/
STATE mscp_bbr_states[] = {

    /* BBR Online processing initial state.
     */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_ONLINIT,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_BB_ONLINIT,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_BB_ONLINIT,	mscp_bbr_step0 },		/* EV_MSGBUF	      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_ONLINIT,	mscp_invevent },		/*		      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_ONLINIT,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP0A,	mscp_bbr_step0a },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP12E,	mscp_bbr_step12e },		/* EV_BBRERROR	      */


/*	BBR replacement started
 */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_REPINIT,	mscp_alloc_rspid },		/* EV_INITIAL	      */
    { ST_BB_REPINIT,	mscp_alloc_msg },		/* EV_RSPID	      */
    { ST_BB_STEP4,	mscp_bbr_step4 },		/* EV_MSGBUF	      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_REPINIT,	mscp_invevent },		/*		      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_REPINIT,	mscp_invevent },		/* EV_BBRERROR	      */

/*	Step 0 continuation
 */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP0A,	mscp_invevent },		/*		      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP0A,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP0B,	mscp_bbr_step0b },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP0B,	mscp_bbr_step0b },		/* EV_BBRERROR	      */

/*	Step 0 continuation
 */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP0B,	mscp_invevent },		/*		      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP0B,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP0C,	mscp_bbr_step0c },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP12E,	mscp_bbr_step12e },		/* EV_BBRERROR	      */

/*	Step 0 continuation
 */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP0C,	mscp_invevent },		/*		      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP0C,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP1,	mscp_bbr_step1 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP1,	mscp_bbr_step1 },		/* EV_BBRERROR	      */

/*	Step 1 - Check if BBR in progress
 */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP1,	mscp_invevent },		/*		      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP1,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP18,	mscp_bbr_step18 },		/* EV_BBRERROR	      */

/*	Step 4 - Attempt to read original lbn
 */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP4,	mscp_bbr_step4a },		/* EV_ENDMSG	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP4,	mscp_invevent },		/*		      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP5,	mscp_bbr_step5 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP4,	mscp_invevent },		/* EV_BBRERROR	      */


/*	Step 5 - Save Bad Block data in RCT sector 1
 */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP5,	mscp_invevent },		/*		      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP5,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP6,	mscp_bbr_step6 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP18,	mscp_bbr_step18 },		/* EV_BBRERROR	      */


/*	Step 6 - Read RCT block 0 for update
 */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP6,	mscp_invevent },		/*		      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP6,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP6A,	mscp_bbr_step6a },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP18,	mscp_bbr_step18 },		/* EV_BBRERROR	      */


/*	Step 6a - Update RCT block 0 and write it out
 */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP6A,	mscp_invevent },		/*		      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP6A,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP7,	mscp_bbr_step7 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP17,	mscp_bbr_step17 },		/* EV_BBRERROR	      */


/*	Step 7 - Start stress test of suspected bad block
 */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP7,	mscp_bbr_step7 },		/* EV_INITIAL	      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP7,	mscp_bbr_step7a },		/* EV_ENDMSG	      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP7,	mscp_invevent },		/*		      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP7,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP7B,	mscp_bbr_step7b },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP8,	mscp_bbr_step8 },		/* EV_BBRERROR	      */


/*	Step 7b - Write saved data and reread up to 4 times
 */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP7B,	mscp_bbr_step7b },		/* EV_ENDMSG	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP7B,	mscp_invevent },		/*		      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP7B,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP7C,	mscp_bbr_step7c },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP8,	mscp_bbr_step8 },		/* EV_BBRERROR	      */


/*	Step 7c - write inverse data and reread it
 */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP7C,	mscp_bbr_step7c },		/* EV_ENDMSG	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP7C,	mscp_invevent },		/*		      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP7C,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP7B,	mscp_bbr_step7b },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP8,	mscp_bbr_step8 },		/* EV_BBRERROR	      */


/*	Step 8 - Write saved data back to original block
 */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP8,	mscp_bbr_step8 },		/* EV_ENDMSG	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP8,	mscp_invevent },		/*		      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP8,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP13,	mscp_bbr_step13 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP9,	mscp_bbr_step9 },		/* EV_BBRERROR	      */


/*	Step 9 - Start search of RCT for replacement block
 */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP9,	mscp_bbr_step9 },		/* EV_INITIAL	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP9,	mscp_invevent },		/*		      */
    { ST_BB_STEP9,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP10,	mscp_bbr_step10 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRERROR	      */


/*	Step 10 - Update RCT sector 0 to indicate phase 2
 */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP10,	mscp_invevent },		/*		      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP10,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP11,	mscp_bbr_step11 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRERROR	      */


/*	Step 11 - Update descriptors to record replacement
 */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP11,	mscp_bbr_step11 },		/* EV_INITIAL	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP11,	mscp_invevent },		/*		      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP11,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP11A,	mscp_bbr_step11a },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRERROR	      */


/*	Step 11a - Process second RCT descriptor block
 */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP11A,	mscp_invevent },		/*		      */
    { ST_BB_STEP11C,	mscp_bbr_step11c },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP11A,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP11B,	mscp_bbr_step11b },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRERROR	      */


/*	Step 11b - Write out RCT descriptor block
 */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP11B,	mscp_invevent },		/*		      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP11B,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP11C,	mscp_bbr_step11c },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP15,	mscp_bbr_step15 },		/* EV_BBRERROR	      */


/*	Step 11c - Write out RCT descriptor block
 */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP11C,	mscp_invevent },		/*		      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP11C,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP12,	mscp_bbr_step12 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP15,	mscp_bbr_step15 },		/* EV_BBRERROR	      */


/*	Step 12
 */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP12A,	mscp_bbr_step12a },		/* EV_ENDMSG	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP12,	mscp_invevent },		/*		      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP12,	mscp_invevent },		/* EV_BBRERROR	      */


/*	Step 12a
 */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP12B,	mscp_bbr_step12b },		/* EV_ENDMSG	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP12A,	mscp_invevent },		/*		      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP12A,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP12C,	mscp_bbr_step12c },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP12D,	mscp_bbr_step12d },		/* EV_BBRERROR	      */


/*	Step 12b
 */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP12B,	mscp_invevent },		/*		      */
    { ST_BB_STEP9,	mscp_bbr_step9 },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP12B,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP12C,	mscp_bbr_step12c },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP12D,	mscp_bbr_step12d },		/* EV_BBRERROR	      */


/*	Step 12c
 */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP12C,	mscp_bbr_step12c },		/* EV_ENDMSG	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP12C,	mscp_invevent },		/*		      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP12C,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP13,	mscp_bbr_step13 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP9,	mscp_bbr_step9 },		/* EV_BBRERROR	      */


/*	Step 12d
 */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP12E,	mscp_bbr_step12e },		/* EV_ENDMSG	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP12D,	mscp_invevent },		/*		      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP12D,	mscp_invevent },		/* EV_BBRERROR	      */


/*	Step 12e
 */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP18,	mscp_bbr_step18 },		/* EV_ENDMSG	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP12E,	mscp_invevent },		/*		      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP12E,	mscp_invevent },		/* EV_BBRERROR	      */


/*	Step 13
 */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP13,	mscp_invevent },		/*		      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP13,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP13,	mscp_bbr_step14 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP17,	mscp_bbr_step17 },		/* EV_BBRERROR	      */


/*	Step 15
 */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP15,	mscp_invevent },		/*		      */
    { ST_BB_STEP15A,	mscp_bbr_step15a },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP15,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP15A,	mscp_bbr_step15a },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP15A,	mscp_bbr_step15a },		/* EV_BBRERROR	      */

/*	Step 15a - Write out descriptor block
 */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP15A,	mscp_invevent },		/*		      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP15A,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRERROR	      */

/*	Step 16
 */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP17,	mscp_bbr_step17 },		/* EV_ENDMSG	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP16,	mscp_invevent },		/*		      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP16,	mscp_invevent },		/* EV_BBRERROR	      */

/*	Step 17
 */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP17,	mscp_invevent },		/*		      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP17,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP18,	mscp_bbr_step18 },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP18,	mscp_bbr_step18 },		/* EV_BBRERROR	      */

/*	Step 18
 */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_STEP18,	mscp_invevent },		/*		      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_STEP18,	mscp_invevent },		/* EV_BBRERROR	      */

/*	RCT search state
 */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/*		      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_RCTSEARCH,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_RCTSEARCHA,	mscp_rct_searcha },		/* EV_BBRSUCCESS      */
    { ST_BB_RCTSEARCH,	mscp_rct_searchc },		/* EV_BBRERROR	      */


/*	RCT search state a
 */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/*		      */
    { ST_BB_RCTSEARCHB,	mscp_rct_searchb },		/* EV_BBRSUBSTEP      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_RCTSEARCHA,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_RCTSEARCHA,	mscp_rct_searchc },		/* EV_BBRERROR	      */


/*	RCT search state b
 */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_ENDMSG	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_RCTSEARCHB,	mscp_invevent },		/*		      */
    { ST_BB_RCTSEARCHB,	mscp_rct_searchb },		/* EV_BBRSUBSTEP      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRRCTFULL      */
    { ST_BB_STEP16,	mscp_bbr_step16 },		/* EV_BBRINVRCT	      */
    { ST_BB_RCTSEARCHB,	mscp_rct_searchb },		/* EV_BBRSUCCESS      */
    { ST_BB_RCTSEARCHB,	mscp_rct_searchc },		/* EV_BBRERROR	      */


/* 	Multi-read algorithm
 */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_MULTIREAD,	mscp_multi_read_cont },		/* EV_ENDMSG	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/*		      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_MULTIREAD,	mscp_invevent },		/* EV_BBRERROR	      */


/*	Multi-write algorithm
 */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_MULTIWRITE,	mscp_multi_write_cont },	/* EV_ENDMSG	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/*		      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_MULTIWRITE,	mscp_invevent },		/* EV_BBRERROR	      */


/*	Multi-write algorithm - forced error path
 */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_NULL	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_MAPPING	      */
    { ST_BB_MULTIWRITE2, mscp_multi_write_cont },	/* EV_ENDMSG	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/*		      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_MULTIWRITE2, mscp_invevent },		/* EV_BBRERROR	      */


/* 	Read in BBR mode
 */
    { ST_BB_READ,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_READ,	mscp_bbr_rwcont },		/* EV_MAPPING	      */
    { ST_BB_READ,	mscp_bbr_rwfin },		/* EV_ENDMSG	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_READ,	mscp_invevent },		/*		      */
    { ST_BB_READ,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_READ,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_READ,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_READ,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_READ,	mscp_invevent },		/* EV_BBRERROR	      */


/*	Write in BBR mode
 */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_NULL	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_INITIAL	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_RSPID	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_MSGBUF	      */
    { ST_BB_WRITE,	mscp_bbr_rwcont },		/* EV_MAPPING	      */
    { ST_BB_WRITE,	mscp_bbr_rwfin },		/* EV_ENDMSG	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_TIMEOUT	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_NOCREDITS	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_ERRECOV	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_SPARE2	      */
    { ST_BB_WRITE,	mscp_invevent },		/*		      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_BBRSUBSTEP      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_BBRRCTFULL      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_BBRINVRCT	      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_BBRSUCCESS      */
    { ST_BB_WRITE,	mscp_invevent },		/* EV_BBRERROR	      */

};
