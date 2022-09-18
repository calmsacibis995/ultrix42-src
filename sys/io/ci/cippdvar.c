#ifndef	lint
static        char    *sccsid = "@(#)cippdvar.c	4.1  (ULTRIX)        7/2/90";
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
 *		Computer Interconnect Port-to-Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port-to-Port
 *		Driver( CI PPD ) internal data structures and variables.
 *
 *   Creator:	Todd M. Katz	Creation Date:	October 20, 1985
 *
 *   Modification History:
 *
 *   09-Dec-1989	Pete Keilty
 *	Added braces to array cippd_cltab[] & cippd_stab[] initiation
 *	so vcc would not complain.
 *
 *   06-Mar-1989	Todd M. Katz		TMK0006
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   22-Sep-1988	Todd M. Katz		TMK0005
 *	1. Update cippd_map_spc[], cippd_clre[], cippd_clse[], and
 *	   cippd_cltab[][] to reflect the creation of SE_BADPPDMTYPE and
 *	   RE_BADPPDMTYPE( BOTH: Unsupported CI PPD message type ).
 *	2. Update all console logging message strings.
 *
 *   18-Aug-1988	Todd M. Katz		TMK0004
 *	1. SCA event code formats have been complete redefined.  Both new
 *	   severity levels as well as severity level modifiers have been
 *	   created.  The GVP subclass( ESC_GVP ) has been eliminated.  Most
 *	   current CI PPD event codes have been reassigned to different
 *	   severity levels and new codes have been added.  Revise and add new
 *	   console logging format tables, update both cippd_cltab[][] and
 *	   cippd_map_pc[], and create cippd_map_spc[].
 *	2. Update the CI PPD finite state machine tables to make sure:
 *		1) Illegal event-state combinations are treated as such.
 *		2) The CI PPD interval timer is stoped whenever STOP datagrams
 *		   are received or a CI PPD protocol violation occurs.
 *		3) All CI PPD path establishment abortion processing is carried
 *		   out during the abortion and not prior to it.
 *		4) Reason code mapping is executed only for established paths.
 *	   Also add a new action set: op_str_rec for use only when a START CI
 *	   PPD datagram is received over a fully established path.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *	1. Update cippd_clie[] and cippd_cltab[][] to reflect the addition of
 *	   new CI PPD informational events: IE_ALLOCFAIL, IE_NORETRIES, and
 *	   IE_BADSWTYPE.
 *	2, Add cippd_test_trys() as the very first function executed within the
 *	   following action sets: str_snt_timeout[], sta_snt_timeout[],
 *	   str_rec_timeout[].
 *	3. Status is no longer returned by cippd_req_id(), cippd_snd_stack(),
 *	   cippd_snd_start().  Update the following action sets to reflect this
 *	   change: sta_snt_timeout[], str_snt_timeout[], and str_rec_timeout[].
 *	4. Eliminate all invocations of cippd_chk_ver().  The function of this
 *	   function has been subsummed into cippd_enter_db().
 *	5. Add the variable, cippd_sli_db( CI PPD system level logging
 *	   information database queue head ).
 *
 *   02-Jun-1988	Ricky S. Palmer
 *      Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   06-Apr-1988	Todd M. Katz		TMK0002
 *	The action set executed during processing of the CNFE_SCSMSG_REC -
 *	PS_PATH_FAILURE event - state combination was changed from
 *	ignore_event3 to a new set, ignore_event4.  This change is necessary
 *	because set ignore_event3 includes invocation of cippd_ignore_dg.  This
 *	routine must never be invoked during processing of CNFE_SCSMSG_REC
 *	events because of the following problems which would result:
 *
 *	1. cippd_ignore_dg returns datagram buffers to the appropriate local
 *	   port datagram free queues.  CNFE_SCSMSG_REC events involve
 *	   processing of messages and not datagram buffers.  Thus, invocation
 *	   of cippd_ignore_dg during processing of CNFE_SCSMSG_REC events
 *	   results in insertion of message buffers onto a local port datagram
 *	   free queues.
 *	2. cippd_ignore_dg disposes of buffers as explained above.
 *	   CNFE_SCSMSG_REC events have the strict requirement that their
 *	   associated buffers are always disposed of following and never
 *	   during event processing.  Thus, invocation of cippd_ignore_dg during
 *	   processing of CNFE_SCSMSG_REC events ultimately results in multiple
 *	   attempts to dispose of the same buffer.
 *
 *	The new action set ignore_event4 is derived directly from set
 *	ignore_event3.  It however lacks invocation of cippd_ignore_dg, and so
 *	lacks all problems associated with the former use of set ignore_event3 
 *	during processing of this specific event - state combination.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/ksched.h"
#include		"../h/time.h"
#include		"../h/errlog.h"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"../io/ci/cippdsysap.h"
#include		"../io/ci/cisysap.h"
#include		"../io/msi/msisysap.h"
#include		"../io/bi/bvpsysap.h"
#include		"../io/gvp/gvpsysap.h"
#include		"../io/uba/uqsysap.h"
#include		"../io/sysap/sysap.h"
#include		"../io/ci/cippdscs.h"
#include		"../io/ci/ciscs.h"
#include		"../io/msi/msiscs.h"
#include		"../io/bi/bvpscs.h"
#include		"../io/gvp/gvpscs.h"
#include		"../io/uba/uqscs.h"
#include		"../io/scs/scs.h"
#include		"../io/gvp/gvp.h"
#include		"../io/ci/cippd.h"

/* External Variables and Routines.
 */
extern u_long		cippd_build_sb(), cippd_enab_path(), cippd_enter_db(),
			cippd_init_pb(), cippd_ppderror(), cippd_stop_dg(),
			cippd_test_trys();
extern void		cippd_ack_rec(), cippd_comp_trys(), cippd_disb_path(),
			cippd_error_dg(), cippd_ignore_dg(), cippd_inv_cache(),
			cippd_panic(), cippd_path_schd(), cippd_pcreason(),
			cippd_req_id(), cippd_rrestart(), cippd_snd_ack(),
			cippd_snd_stack(), cippd_snd_start(), cippd_snd_stop(),
			cippd_start_tmr(), cippd_stop_tmr(), cippd_upd_ptype(),
			cippd_update_sb();

/* Global Variables and Data Structures.
 */
cippd_slibq	cippd_sli_db = {	/* CI PPD System Level Logging	     */
		    &cippd_sli_db,	/*  Information Database Queue Head  */
		    &cippd_sli_db
};
u_short         cippd_map_pc[] = {	/* CI PPD Path Crash Mapping Table   */
	0,		/* Reserved for non-path crash severe error event    */
	0,		/* Reserved for non-path crash severe error event    */
	0,		/* Reserved for non-path crash severe error event    */
	PF_ERROR,	/* E_PD		 - Port driver requested failure     */
        PF_HOSTSHUTDWN, /* E_RHOST       - Remote system requested failure   */
        PF_PPDPROTOCOL, /* E_RESTARTPATH - Rem CI PPD requested path restart */
},
	         cippd_map_spc[] = {	/* CI PPD Severe Path Crash Map Table*/
	0,		/* Reserved for non-path crash severe error event    */
	PF_ERROR,	/* SE_PD	 - Port driver requested failure     */
	0,		/* Reserved for non-path crash severe error event    */
	0,		/* Reserved for non-path crash severe error event    */
        PF_PPDPROTOCOL, /* SE_BADPPDMTYPE- Unsupported CI PPD message type   */
};
static CLFTAB				/* Console Logging Formatting Tables */
/* NULL entries represent events which should never be logged by the CI PPD.
 */
		cippd_cli[] = {		/* CI PPD Informational Event Table  */
    { CF_NONE,       "new path established" }
},
		cippd_clw[] = {		/* CI PPD Warning Event Table	     */
    { CF_NONE,       "remote system: running newer ci ppd protocol version" },
    { CF_SWTYPE,     "unsupported path established" },
    { CF_NONE,       "path establishment aborted: local port failure" },
    { CF_NONE,       "path failed: local port failure" },
},
		cippd_clre[] = {	/* CI PPD Remote Error Event Table   */
    { CF_DBCONFLICT, "remote system: conflicts with known system" },
    { CF_NONE,       "received ci ppd error datagram" },
    { CF_RPORT,	     "received ci ppd datagram: contains unsupported type" }
},
		cippd_cle[] = {		/* CI PPD Error Event Table	     */
    { CF_NONE,       "path establishment aborted: protocol mismatch", },
    { CF_NONE,	     "path establishment aborted: insufficient memory" },
    { CF_NONE,	     "path establishment aborted: exhausted retries" },
    { CF_LPORTNAME,  "path termination: requested by port driver" },
    { CF_NONE,       "path termination: requested by remote system" },
    { CF_NONE,       "path re-synchronization: requested by remote ci ppd" }
},
		cippd_clse[] = {	/* CI PPD Severe Error Event Table   */
    { CF_NONE,	     "ci ppd protocol error" },
    { CF_LPORTNAME,  "path termination: requested by port driver" },
    { CF_NONE,	     NULL },
    { CF_NONE,	     NULL },
    { CF_NONE,	     "received ci ppd datagram: contains unsupported type" }
},
		cippd_clscse[] = {	/* SCS Error Event Table	     */
    { CF_SYSAPNAME,  "path termination: requested by local sysap" },
    { CF_NONE,       "scs protocol timeout" }
},
		cippd_clscsse[] = {	/* SCS Severe Error Event Table	     */
    { CF_NONE,       "scs connection: identifier illegal" },
    { CF_NONE,       "scs connection: illegal state" },
    { CF_NONE,       "scs_connection: illegal credit withdrawal" },
    { CF_NONE,       "received scs message: contains unsupported type" },
    { CF_NONE,       "scs connection: block data transfer unexpected" },
    { CF_NONE,       "received scs message: no matching credit" }
};
CLSTAB					/* CI PPD Console Logging Table	     */
	cippd_cltab[ ES_SE + 1 ][ EC_PD + 1 ] = {
{					/* Severity == ES_I		     */
    {  0,	NULL	      },	/*     Class == EC_SCS		     */
    {  1,	cippd_cli     },	/*     Class == EC_PD( CI PPD Only ) */
},{					/* Severity == ES_W		     */
    {  0,	NULL	      },	/*     Class == EC_SCS		     */
    {  4,	cippd_clw     },	/*     Class == EC_PD( CI PPD Only ) */
},{					/* Severity == ES_RE		     */
    {  0,	NULL	      },	/*     Class == EC_SCS		     */
    {  3,	cippd_clre    },	/*     Class == EC_PD( CI PPD Only ) */
},{					/* Severity == ES_E		     */
    {  2,	cippd_clscse  },	/*     Class == EC_SCS		     */
    {  6,	cippd_cle     },	/*     Class == EC_PD( CI PPD Only ) */
},{					/* Severity == ES_SE		     */
    {  6,	cippd_clscsse },	/*     Class == EC_SCS		     */
    {  5,	cippd_clse    },	/*     Class == EC_PD( CI PPD Only ) */
},
};
static CIPPD_ATAB
    error_dg_event[] = {		/* Error Datagram Event		     */
	Fn_nostatus( cippd_error_dg ),	/*	Process ERROR CI PPD Datagram*/
	Fn_nostatus( cippd_ignore_dg ),	/*	Discard Datagram  	     */
	End_fnset()
},	
    error_event1[] = {			/* Event Violates CI PPD Protocol    */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_status( cippd_ppderror ),	/*	Report CI PPD Protocol Error */
	End_fnset()
},
    error_event2[] = {			/* Event Violates CI PPD Protocol    */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_nostatus( cippd_disb_path ),	/*	Disable CI PPD Path	     */
	Fn_status( cippd_ppderror ),	/*	Report CI PPD Protocol Error */
	End_fnset()
},
    ignore_event1[] = {			/* Event has No Affect on State	     */
	End_fnset()
},
    ignore_event2[] = {			/* Event has No Affect on State	     */
	Fn_nostatus( cippd_ignore_dg ),	/*	Discard Datagram  	     */
	End_fnset()
},
    ignore_event3[] = {			/* Event has No Affect on State	     */
	Fn_nostatus( cippd_snd_stop ),	/*	Send STOP CI PPD Datagram    */
	Fn_nostatus( cippd_ignore_dg ),	/*	Discard Datagram  	     */
	End_fnset()
},
    ignore_event4[] = {			/* Event has No Affect on State	     */
	Fn_nostatus( cippd_snd_stop ),	/*	Send STOP CI PPD Datagram    */
	End_fnset()
},
    illegal_event[] = {			/* Illegal State-Event Combination   */
	Fn_nostatus( cippd_panic ),	/* 	Immediately Panic	     */
	End_fnset()
},
    path_failure1[] = {			/* Path Failure Event		     */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_nostatus( cippd_snd_stop ),	/*	Send STOP CI PPD Datagram    */
	Fn_nostatus( cippd_path_schd ),	/*	Schedule CI PPD Path Clean Up*/
	End_fnset()
},
    path_failure2[] = {			/* Path Failure Event		     */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_nostatus( cippd_disb_path ),	/*	Disable CI PPD Path	     */
	Fn_nostatus( cippd_snd_stop ),	/*	Send STOP CI PPD Datagram    */
	Fn_nostatus( cippd_path_schd ),	/*	Schedule CI PPD Path Clean Up*/
	End_fnset()
},
    process_error1[] = {		/* FSM Processing Failure Event	     */
	Fn_nostatus( cippd_snd_stop ),	/*	Send STOP CI PPD Datagram    */
	Fn_nostatus( cippd_path_schd ),	/*	Schedule CI PPD Path Clean Up*/
	End_fnset()
},
    process_error2[] = {		/* FSM Processing Failure Event	     */
	Fn_nostatus( cippd_disb_path ),	/*	Disable CI PPD Path	     */
	Fn_nostatus( cippd_snd_stop ),	/*	Send STOP CI PPD Datagram    */
	Fn_nostatus( cippd_path_schd ),	/*	Schedule CI PPD Path Clean Up*/
	End_fnset()
},
    stop_dg_event1[] = {		/* Stop Datagram Event		     */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_status( cippd_stop_dg ),	/*	Remote CI PPD Path Terminat'n*/
	End_fnset()
},
    stop_dg_event2[] = {		/* Stop Datagram Event		     */
	Fn_nostatus( cippd_stop_dg ),	/*	Remote CI PPD Path Terminat'n*/
	Fn_nostatus( cippd_ignore_dg ),	/*	Discard Datagram  	     */
	End_fnset()
},
					/*  STATE	    EVENT	     */
    cl_id_rec[] = {			/*  PS_CLOSED	    CNFE_ID_REC	     */
	Fn_status( cippd_init_pb ),	/*	Initialize Path Block	     */
	Fn_nostatus( cippd_comp_trys ),	/*	Compute Number of Tries	     */
	Fn_nostatus( cippd_snd_start ),	/*	Send START CI PPD Datagram   */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    cl_str_rec[] = {			/*  PS_CLOSED	    CNFE_START_REC   */
	Fn_status( cippd_build_sb ),	/*	Build Formative System Block */
	Fn_nostatus( cippd_comp_trys ),	/*	Compute Number of Tries	     */
	Fn_nostatus( cippd_req_id ),	/*	Request Report Port ID	     */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    str_snt_str_rec[] = {		/*  PS_START_SNT    CNFE_START_REC   */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_status( cippd_build_sb ),	/*	Build Formative System Block */
	Fn_nostatus( cippd_upd_ptype ),	/*	Update Remote Port Type	     */
	Fn_status( cippd_enab_path ),	/*	Enable CI PPD Path	     */
	Fn_nostatus( cippd_comp_trys ),	/*	Compute Number of Tries	     */
	Fn_nostatus( cippd_snd_stack ),	/*	Send STACK CI PPD Datagram   */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    str_snt_sta_rec[] = {		/*  PS_START_SNT    CNFE_STACK_REC   */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_status( cippd_build_sb ),	/*	Build Formative System Block */
	Fn_nostatus( cippd_upd_ptype ),	/*	Update Remote Port Type	     */
	Fn_status( cippd_enab_path ),	/*	Enable CI PPD Path	     */
	Fn_status( cippd_enter_db ),	/*	Enter PB in System Databases */
	Fn_nostatus( cippd_snd_ack ),	/*	Send ACK CI PPD Datagram     */
	End_fnset()
},
    str_snt_timeout[] = {		/*  PS_START_SNT    CNFE_TIMEOUT     */
	Fn_status( cippd_test_trys ),	/*	Test Transmission Attempts   */
	Fn_nostatus( cippd_snd_start ),	/*	Send START CI PPD Datagram   */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    str_rec_id_rec[] = {		/*  PS_START_REC    CNFE_ID_REC	     */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_status( cippd_init_pb ),	/*	Initialize Path Block	     */
	Fn_nostatus( cippd_upd_ptype ),	/*	Update Remote Port Type	     */
	Fn_status( cippd_enab_path ),	/*	Enable CI PPD Path	     */
	Fn_nostatus( cippd_comp_trys ),	/*	Compute Number of Tries	     */
	Fn_nostatus( cippd_snd_stack ),	/*	Send STACK CI PPD Datagram   */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    str_rec_str_rec[] = {		/*  PS_START_REC    CNFE_START_REC   */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_nostatus( cippd_update_sb ),	/*	Update Formative System Block*/
	Fn_nostatus( cippd_comp_trys ),	/*	Compute Number of Tries	     */
	Fn_nostatus( cippd_req_id ),	/*	Request Report Port ID	     */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    str_rec_timeout[] = {		/*  PS_START_REC    CNFE_TIMEOUT     */
	Fn_status( cippd_test_trys ),	/*	Test Transmission Attempts   */
	Fn_nostatus( cippd_req_id ),	/*	Request Report Port ID	     */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    sta_snt_str_rec[] = {		/*  PS_STACK_SNT    CNFE_START_REC   */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_nostatus( cippd_update_sb ),	/*	Update Formative System Block*/
	Fn_nostatus( cippd_comp_trys ),	/*	Compute Number of Tries	     */
	Fn_nostatus( cippd_snd_stack ),	/*	Send STACK CI PPD Datagram   */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    sta_snt_sta_rec[] = {		/*  PS_STACK_SNT    CNFE_STACK_REC   */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_nostatus( cippd_update_sb ),	/*	Update Formative System Block*/
	Fn_status( cippd_enter_db ),	/*	Enter PB in System Databases */
	Fn_nostatus( cippd_snd_ack ),	/*	Send ACK CI PPD Datagram     */
	End_fnset()
},
    sta_snt_ack_rec[] = {		/*  PS_STACK_SNT    CNFE_ACK_REC     */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_status( cippd_enter_db ),	/*	Enter PB in System Databases */
	Fn_nostatus( cippd_ack_rec ),	/*	Discard Received ACK Datagram*/
	End_fnset()
},
    sta_snt_scs_rec[] = {		/*  PS_STACK_SNT    CNFE_SCSMSG_REC  */
	Fn_nostatus( cippd_stop_tmr ),	/*	Stop CI PPD Interval Timer   */
	Fn_status( cippd_enter_db ),	/*	Enter PB in System Databases */
	End_fnset()
},
    sta_snt_timeout[] = {		/*  PS_STACK_SNT    CNFE_TIMEOUT     */
	Fn_status( cippd_test_trys ),	/*	Test Transmission Attempts   */
	Fn_nostatus( cippd_snd_stack ),	/*	Send STACK CI PPD Datagram   */
	Fn_nostatus( cippd_start_tmr ),	/*	Start CI PPD Interval Timer  */
	End_fnset()
},
    op_str_rec[] = {			/*  PS_OPEN	    CNFE_START_REC   */
	Fn_nostatus( cippd_rrestart ),	/*	Process Rem Path Restart Req */
	Fn_nostatus( cippd_ignore_dg ),	/*	Discard Datagram  	     */
	End_fnset()
},
    op_sta_rec[] = {			/*  PS_OPEN	    CNFE_STACK_REC   */
	Fn_nostatus( cippd_update_sb ),	/*	Update Formative System Block*/
	Fn_nostatus( cippd_snd_ack ),	/*	Send ACK CI PPD Datagram     */
	End_fnset()
},
    op_path_fail[] = {			/*  PS_OPEN	    CNFE_PATH_FAIL   */
	Fn_nostatus( cippd_pcreason ),	/*	Map Path Crash Reason	     */
	Fn_nostatus( cippd_disb_path ),	/*	Disable CI PPD Path	     */
	Fn_nostatus( cippd_snd_stop ),	/*	Send STOP CI PPD Datagram    */
	Fn_nostatus( cippd_inv_cache ),	/*	Invalidate Translation Cache */
	Fn_nostatus( cippd_path_schd ),	/*	Schedule CI PPD Path Clean Up*/
	End_fnset()
};
CIPPD_STAB				/* CI PPD State Transition Table     */
	cippd_stab[ PS_MAX_STATE + 1 ][ CNFE_MAX_EVENT + 1 ] = {
{					  /* State == PS_CLOSED		     */
    { cl_str_rec,      PS_START_REC    }, /*	Event == CNFE_START_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_STACK_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_ACK_REC	     */
    { cl_id_rec,       PS_START_SNT    }, /*	Event == CNFE_ID_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_SCSMSG_REC     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_ERROR_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_STOP_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_TIMEOUT	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_PATH_FAIL	     */
    { process_error1,  PS_PATH_FAILURE }, /*	Event == CNFE_PROC_ERROR     */
},{					  /* State == PS_START_SNT	     */
    { str_snt_str_rec, PS_STACK_SNT    }, /*	Event == CNFE_START_REC	     */
    { str_snt_sta_rec, PS_OPEN	       }, /*	Event == CNFE_STACK_REC	     */
    { error_event1,    PS_PATH_FAILURE }, /*	Event == CNFE_ACK_REC	     */
    { ignore_event2,   PS_START_SNT    }, /*	Event == CNFE_ID_REC	     */
    { error_event1,    PS_PATH_FAILURE }, /*	Event == CNFE_SCSMSG_REC     */
    { error_dg_event,  PS_START_SNT    }, /*	Event == CNFE_ERROR_REC	     */
    { stop_dg_event1,  PS_PATH_FAILURE }, /*	Event == CNFE_STOP_REC	     */
    { str_snt_timeout, PS_START_SNT    }, /*	Event == CNFE_TIMEOUT	     */
    { path_failure1,   PS_PATH_FAILURE }, /*	Event == CNFE_PATH_FAIL	     */
    { process_error2,  PS_PATH_FAILURE }, /*	Event == CNFE_PROC_ERROR     */
},{					  /* State == PS_START_REC	     */
    { str_rec_str_rec, PS_START_REC    }, /*	Event == CNFE_START_REC	     */
    { error_event1,    PS_PATH_FAILURE }, /*	Event == CNFE_STACK_REC	     */
    { error_event1,    PS_PATH_FAILURE }, /*	Event == CNFE_ACK_REC	     */
    { str_rec_id_rec,  PS_STACK_SNT    }, /*	Event == CNFE_ID_REC	     */
    { error_event2,    PS_PATH_FAILURE }, /*	Event == CNFE_SCSMSG_REC     */
    { error_dg_event,  PS_START_REC    }, /*	Event == CNFE_ERROR_REC	     */
    { stop_dg_event1,  PS_PATH_FAILURE }, /*	Event == CNFE_STOP_REC	     */
    { str_rec_timeout, PS_START_REC    }, /*	Event == CNFE_TIMEOUT	     */
    { path_failure1,   PS_PATH_FAILURE }, /*	Event == CNFE_PATH_FAIL	     */
    { process_error1,  PS_PATH_FAILURE }, /*	Event == CNFE_PROC_ERROR     */
},{					  /* State == PS_STACK_SNT	     */
    { sta_snt_str_rec, PS_STACK_SNT    }, /*	Event == CNFE_START_REC	     */
    { sta_snt_sta_rec, PS_OPEN	       }, /*	Event == CNFE_STACK_REC	     */
    { sta_snt_ack_rec, PS_OPEN	       }, /*	Event == CNFE_ACK_REC	     */
    { ignore_event2,   PS_STACK_SNT    }, /*	Event == CNFE_ID_REC	     */
    { sta_snt_scs_rec, PS_OPEN	       }, /*	Event == CNFE_SCSMSG_REC     */
    { error_dg_event,  PS_STACK_SNT    }, /*	Event == CNFE_ERROR_REC	     */
    { stop_dg_event1,  PS_PATH_FAILURE }, /*	Event == CNFE_STOP_REC	     */
    { sta_snt_timeout, PS_STACK_SNT    }, /*	Event == CNFE_TIMEOUT	     */
    { path_failure2,   PS_PATH_FAILURE }, /*	Event == CNFE_PATH_FAIL	     */
    { process_error2,  PS_PATH_FAILURE }, /*	Event == CNFE_PROC_ERROR     */
},{					  /* State == PS_OPEN		     */
    { op_str_rec,      PS_PATH_FAILURE }, /*	Event == CNFE_START_REC	     */
    { op_sta_rec,      PS_OPEN	       }, /*	Event == CNFE_STACK_REC	     */
    { ignore_event2,   PS_OPEN	       }, /*	Event == CNFE_ACK_REC	     */
    { ignore_event2,   PS_OPEN	       }, /*	Event == CNFE_ID_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_SCSMSG_REC     */
    { error_dg_event,  PS_OPEN	       }, /*	Event == CNFE_ERROR_REC	     */
    { stop_dg_event2,  PS_PATH_FAILURE }, /*	Event == CNFE_STOP_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_TIMEOUT	     */
    { op_path_fail,    PS_PATH_FAILURE }, /*	Event == CNFE_PATH_FAIL	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_PROC_ERROR     */
},{					  /* State == PS_PATH_FAILURE	     */
    { ignore_event3,   PS_PATH_FAILURE }, /*	Event == CNFE_START_REC	     */
    { ignore_event3,   PS_PATH_FAILURE }, /*	Event == CNFE_STACK_REC	     */
    { ignore_event3,   PS_PATH_FAILURE }, /*	Event == CNFE_ACK_REC	     */
    { ignore_event2,   PS_PATH_FAILURE }, /*	Event == CNFE_ID_REC	     */
    { ignore_event4,   PS_PATH_FAILURE }, /*	Event == CNFE_SCSMSG_REC     */
    { error_dg_event,  PS_PATH_FAILURE }, /*	Event == CNFE_ERROR_REC	     */
    { ignore_event2,   PS_PATH_FAILURE }, /*	Event == CNFE_STOP_REC	     */
    { illegal_event,   NULL	       }, /*	Event == CNFE_TIMEOUT	     */
    { ignore_event1,   PS_PATH_FAILURE }, /*	Event == CNFE_PATH_FAIL	     */
    { illegal_event,   NULL	       }  /*	Event == CNFE_PROC_ERROR     */
},
};
