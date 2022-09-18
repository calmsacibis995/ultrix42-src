/*
 @(#)cippd.h	4.1  (ULTRIX)        7/2/90 
 */

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
 *		Driver(  CI PPD ) constants, data structure definitions,
 *		message definitions, and macros.
 *
 *   Creator:	Todd M. Katz	Creation Date:	April 22, 1985
 *
 *   Modification History:
 *
 *   04-Jan-1989	Todd M. Katz		TMK0006
 *	1. Add the message definition CIPPDH.
 *	2. Add padding when it is necessary to keep longword alignment.  While
 *	   some space is wasted such alignment is essential for ports of SCA to
 *	   hardware platforms which require field alignment and access type to
 *	   match( ie- only longword aligned entities may be longword accessed).
 *	3. Move the console logging format code CF_NONE to ../vaxscs/scs.h.
 *
 *   22-Sep-1988	Todd M. Katz		TMK0005
 *	1. Add SE_BADPPDMTYPE( Unsupported CI PPD message type ) and update
 *	   SE_MAX_PPD appropriately.
 *	2. Add RE_BADPPDMTYPE( Unsupported CI PPD message type ).
 *	3. Add CF_RPORT( Remote port station address logged ) as a console
 *	   formatting code.
 *
 *   17-Aug-1988	Todd M. Katz		TMK0004
 *	 1. SCA event codes have been completely revised.  CI PPD path crash
 *	    codes are now defined as error events( RHOST, PPDPROTOCOL ).  CI
 *	    PPD local port crash codes are now defined as severe error events(
 *	    NOPATH, BROKEN ).  The path and local port crash severity modifiers
 *	    are applied by cippd_crash_pb() and individual port driver local
 *	    port crash routines respectively.
 *	 2. Redefine the following informational events( ES_I ) as warning
 *	    events( ES_W ): TALKUP, BADSWTYPE.
 *	 3. Redefine the following informational events( ES_I ) as remote error
 *	    events( ES_RE ): DBCONFLICT, RERROR.
 * 	 4. Redefine the following informational events( ES_I ) as error
 *	    events( ES_E ): TALKDOWN, ALLOCFAIL, NORETRIES.
 *	 5. Rename E_PPDPROTOCOL -> E_RESTARTPATH and 
 *	    SE_BROKEN -> SE_PPDSANITY.
 *	 6. Define the informational event: NEW_PATH, the warning events:
 *	    ABORT_PATH and FAIL_PATH, the error event: PD, and the severe error
 *	    events: PD and PPDPROTOCOL.
 *	 7. E_MAX_PPD( formerly PC_MAX_PPDPC ) defines the maximum CI PPD error
 *	    event code and SE_MAX_PPD defines the maximum CI PPD severe error
 *	    code.
 *	 8. Delete IE_BADPORTADDR, LPC_POWER, LPC_POWERUP, CF_BADPORTNUM,
 *	    CF_PATH, and CF_SYSTEM.  Add CF_SYSAPNAME and CF_LPORTNAME.
 *	 9. Add Log_badportnum as a CI PPD mandatory PD function.  Also, delete
 *	    Log_packet and Map_reason as PD functions for the CI PPD.
 *	10. Add macros Elcippdnewpath() and Elcippdscommon() and delete macro
 *	    Elcippdrswtype().  Add shorthand notations Burst, Contact, and
 *	    Ppddgs; and, rename the shorthand notation Errorlogopt -> Elogopt.
 *	11. Define constants for the types of events( SYSTEM_EVENT, PATH_EVENT,
 *	    LPORT_EVENT, RPORT_EVENT ).
 *	12. Update macro Pinterval() to obtain port polling frequency and
 *	    burst sizes from CI PPD specific PCCB fields instead of from CI PPD
 *	    configuration variables.  This allows these parameters to vary on
 *	    a per local port basis( actually they vary on a per port driver
 *	    basis ).
 *	13. Moved definition of structures CLFTAB and CLSTAB to
 *	    ../vaxscs/sca.h.
 *
 *   17-May-1988	Todd M. Katz		TMK0003
 *	1. Add the definitions PPDPANIC_REQPC, PPDPANIC_UNKSLC, IE_ALLOCFAIL,
 *	   IE_NORETRIES, IE_BADSWTYPE, CF_PATH, and CF_SWTYPE; and, the
 *	   shorthand notations Aflogmap, Slib, and Tmologmap while removing the
 *	   shorthand notations Dbcoll, Lpstatus, Ptdlogmap, Pstatus, and
 *	   Rport_status.  Also add the Common System Level Logging Options.
 *	2. Create new macros Test_map(), Clear_map(), and Set_map(); rename the
 *	   old macros to Test_lpinfomap(), Clear_lpinfomap(), and
 *	   Set_lpinfomap() respectively; and, change Mapbyte() to be a more
 *	   generic macro.
 *	3. Update macros Elcippdprotocol(), and Elcippddbcoll(); remove macro
 *	   Elcippdrswtype(); rename macro Elcippdpacket() -> Elcippdspacket();
 *	   and add macros Elcippdppacket(), Elcippdpcommon(), Elcippdscommon(),
 *	   and Elcippdsysapnam() to reflect changes in CI PPD error logging
 *	   formats.
 *	4. Define the CI PPD data structures CIPPD_SLIB and cippd_slibq used
 *	   for keeping track of the logging of common system level conditions
 *	   on a per system basis.
 *
 *   11-Apr-1988	Todd M. Katz		TMK0002
 *	Add macros Pccb_fork(), Pccb_fork_done(), Pb_fork(), and 
 *	Pb_fork_done().
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made CI
 *	PPD and GVP completely independent from underlying port drivers, and
 *	added SMP support.
 */

/* CI PPD Constants.
 */
					/* CI PPD Configuration Events	     */
#define	CNFE_START_REC	        0	/* CI PPD START datagram received    */
#define	CNFE_STACK_REC	        1	/* CI PPD STACK datagram received    */
#define	CNFE_ACK_REC	        2	/* CI PPD ACK datagram received	     */
#define	CNFE_ID_REC	        3 	/* ID received			     */
#define	CNFE_SCSMSG_REC	        4	/* SCS CONN_REQ message received     */
#define	CNFE_ERROR_REC	        5	/* CI PPD error datagram received    */
#define	CNFE_STOP_REC	        6	/* CI PPD node stop datagram received*/
#define	CNFE_TIMEOUT	        7	/* CI PPD timeout occurred	     */
#define	CNFE_PATH_FAIL		8	/* CI PPD path failed		     */
#define	CNFE_PROC_ERROR		9	/* CI PPD FSM error detected	     */
#define	CNFE_MAX_EVENT	CNFE_PROC_ERROR	/* CI PPD maximum event number	     */

					/* CI PPD Panic Strings		     */
#define	PPDPANIC_BPATH	"ci ppd\t- removing unremovable path\n"
#define	PPDPANIC_EVENT	"ci ppd\t- unknown finite state machine event\n"
#define	PPDPANIC_FSM  "ci ppd\t- invalid state/event combination encountered\n"
#define	PPDPANIC_NOPATH	"ci ppd\t- unretrievable path\n"
#define	PPDPANIC_PBFB	"ci ppd\t- invalid pb fork block\n"
#define	PPDPANIC_PCCBFB	"ci ppd\t- invalid pccb fork block\n"
#define	PPDPANIC_PCR	"ci ppd\t- invalid/unknown path crash reason\n"
#define	PPDPANIC_POPEN	"ci ppd\t- path is already enabled\n"
#define	PPDPANIC_PSTATE	"ci ppd\t- invalid path state\n"
#define	PPDPANIC_REQPC	"ci ppd\t- panic requested on all path failures\n"
#define	PPDPANIC_TIMER	"ci ppd\t- broken traffic interval timer\n"
#define	PPDPANIC_UNKCF	"ci ppd\t- unknown console logging formatting code\n"
#define	PPDPANIC_UNKCOD	"ci ppd\t- unknown/invalid event code\n"
#define	PPDPANIC_UNKSLC	"ci ppd\t- unknown/invalid system level event\n"

					/* CI PPD Set Circuit Settings	     */
#define	SET_VC_CLOSE		0	/* Close Set CI PPD virtual circuit  */
#define	SET_VC_OPEN		1	/* Open CI PPD virtual circuit	     */

					/* Types of CI PPD Messages	     */
#define	START		 	0	/* START virtual circuit datagram    */
#define	STACK			1	/* ACKnowledge START datagram	     */
#define	ACK			2	/* ACKnowledge STACK datagram	     */
					/* SCSDG( 3 ) is defined by SCS	     */
					/* SCSMSG( 4 ) is defined by SCS     */
#define	ERROR			5	/* ERROR datagram		     */
#define	STOP			6	/* STOP datagram		     */

					/* Console Logging Format Codes	     */
				 	/* 0 - No optional information logged*/
#define	CF_DBCONFLICT		1	/* Special db conflict info logged   */
#define	CF_SWTYPE		2	/* Special rem system sw type logged */
#define	CF_SYSAPNAME		3	/* Name of local SYSAP		     */
#define	CF_LPORTNAME		4	/* Name of local port		     */
#define	CF_RPORT		5	/* Remote port station address logged*/

					/* Types of CI PPD Events	     */
#define	SYSTEM_EVENT		0	/* System specific event	     */
#define	PATH_EVENT		1	/* Path specific event		     */
#define	RPORT_EVENT		2	/* Remote port specific event	     */
#define	LPORT_EVENT		3	/* Local port specific event	     */

					/* CI PPD Informational Event Codes  */
/* CI PPD informational events are always path specific.  There are currently
 * no remote port, local port, or system specific informational events.
 * cippd_conlog(), cippd_log_path(), cippd_cli[], and cippd_cltab[][] must be
 * updated to reflect new additions.
 *
 * The following CI PPD informational events are path specific but may NOT have
 * the path crash severity modifier( ESM_PC ) applied:
 *
 *	NEW_PATH
 *
 * Path specific information is always displayed by default during console
 * logging of path specific CI PPD informational events.
 *
 * NOTE: Path specific CI PPD informational events may never be candidates for
 *	 application of the path crash severity modifier( ESM_PC ).
 *
 * NOTE: When updating cippd_cli[] with a new CI PPD warning event bear in mind
 *	 the information displayed by default when choosing an appropriate
 *	 console formatting code( CF ).
 */
#define	I_NEW_PATH     ( PPDI | 0x01 )	/* PPD established new path	     */

					/* CI PPD Warning Event Codes	     */
/* CI PPD warning events are either path or system specific.  There are
 * currently no local or remote port specific CI PPD warning events.
 * cippd_conlog(), cippd_clw[], and cippd_cltab[][] must be updated to reflect
 * new additions.  cippd_log_path() must be updated when a new path specific CI
 * PPD warning event is added.  cippd_log_sys() must be updated when a new
 * system specific CI PPD warning event is added.  cippd_csyslev() also
 * requires updating when the new addition is also a common system level error
 * event.
 *
 * The following CI PPD warnings events are system specific and are also common
 * system level events:
 *
 * 	TALKUP, BADSWTYPE
 *
 * The following CI PPD wasrning events are path specific but may NOT have the
 * path crash severity modifier( ESM_PC ) applied:
 *
 *	ABORT_PATH, FAIL_PATH
 *
 * Path specific information is always displayed by default during console
 * logging of path specific CI PPD warning events.  The remote system name is
 * always displayed by default during console logging of system specific CI PPD
 * warning events.
 *
 * NOTE: Path specific CI PPD warning events may never be candidates for
 *	 application of the path crash severity modifier( ESM_PC ).  System
 *	 specific CI PPD warning events are ( currently ) always also common
 *	 system level events.
 *
 * NOTE: When updating cippd_clw[] with a new CI PPD warning event bear in mind
 *	 the information displayed by default when choosing an appropriate
 *	 console formatting code( CF ).
 */
#define	W_TALKUP       ( PPDW | 0x01 )	/* PPD attempting to talk up	     */
#define	W_BADSWTYPE   ( PPDAW | 0x02 )	/* Unsupported remote system sw type */
#define	W_ABORT_PATH   ( PPDW | 0x03 )	/* Lport fail aborted path estabment */
#define	W_FAIL_PATH    ( PPDW | 0x04 )	/* Local port failure terminated path*/

					/* CI PPD Remote Error Event Codes   */
/* CI PPD remote error events are either system or remote port specific.  There
 * are currently no local port or path specific CI PPD remote error events.
 * cippd_conlog(), cippd_clre[], and cippd_cltab[][] must be updated to reflect
 * new additions.  Currently, cippd_log_path() must be updated when a new
 * remote port specific CI PPD remote error event is added.  cippd_log_sys()
 * must be updated when a new system specific CI PPD remote error event is
 * added.  cippd_csyslev() also requires updating when the new addition is also
 * a common system level remote error event.
 *
 * The following CI PPD error events are remote port specific:
 *
 *	DBCONFLICT, BADPPDMTYPE
 *
 * The following CI PPD remote error events are system specific but are NOT
 * common system level events:
 *
 * 	RERROR
 *
 * The remote system name is always displayed by default during console logging
 * of system specific CI PPD remote error events.
 *
 * NOTE: System specific CI PPD remote error events are ( currently ) never
 *	 also common system level events.
 *
 * NOTE: When updating cippd_clre[] with a new CI PPD remote error event bear
 *	 in mind the information displayed by default when choosing an
 *	 appropriate console formatting code( CF ).
 */
#define	RE_DBCONFLICT  ( PPDRE | 0x01 )	/* Configuration db conflict occurred*/
#define	RE_RERROR      ( PPDRE | 0x02 )	/* Remote system reporting error     */
#define	RE_BADPPDMTYPE ( PPDRE | 0x03 )	/* Unsupported CI PPD message type   */

					/* CI PPD Error Event Codes	     */
/* CI PPD error events are either path or system specific.  There are currently
 * no local or remote port specific CI PPD error events.  cippd_conlog(),
 * cippd_cle[], cippd_cltab[][], and cippd_map_pc[] must be updated to reflect
 * new additions.  cippd_log_path() must be updated when a new path specific CI
 * PPD error event is added.  cippd_log_sys() must be updated when a new system
 * specific CI PPD error event is added.  cippd_csyslev() also requires
 * updating when the new addition is also a common system level error event.
 *
 * The following CI PPD error events are system specific and are also common
 * system level events:
 *
 *	TALKDOWN
 *
 * The following CI PPD error events are path specific and may have the path
 * crash severity modifier( ESM_PC ) applied:
 *
 *	PD, RHOST, RRESTARTPATH
 *
 * The following CI PPD error events are path specific but may NOT have the
 * path crash severity modifier( ESM_PC ) applied:
 *
 *	ALLOCFAIL, NORETRIES
 *
 * Path specific information is always displayed by default during console
 * logging of path specific CI PPD error events.  The remote system name is
 * always displayed by default during console logging of system specific CI PPD
 * error events.
 *
 * NOTE: Not all path specific CI PPD error events are candidates for
 *	 application of the path crash severity modifier( ESM_PC ).  System
 *	 specific CI PPD error events are ( currently ) always also common
 *	 system level events.
 *
 * NOTE: Update cippd_map_pc[] with a NULL entry whenever a new CI PPD error
 *	 event is system specific and can never have the path crash severity
 *	 modifier( ESM_PC ) applied; or, is path specific but is not a
 *	 candidate for application of this severity modifier.
 *
 * NOTE: When updating cippd_cle[] with a new CI PPD error event bear in mind
 *	 the information displayed by default when choosing an appropriate
 *	 console formatting code( CF ).
 */
#define	E_TALKDOWN      ( PPDE | 0x01 )	/* PPD refuses to talk down	     */
#define	E_ALLOCFAIL     ( PPDE | 0x02 )	/* Insufficient mem to establish path*/
#define	E_NORETRIES     ( PPDE | 0x03 )	/* Transmission retries exhausted    */
#define	E_PD		( PPDE | 0x04 )	/* Port driver requested path failure*/
#define	E_RHOST         ( PPDE | 0x05 )	/* Remote host requested path failure*/
#define	E_RRESTARTPATH  ( PPDE | 0x06 )	/* Rem CI PPD req path re-establish  */
#define E_MAX_PPD     E_RRESTARTPATH	/* Maximum CI PPD error event code   */

					/* CI PPD Severe Error Event Codes   */
/* CI PPD severe error events are either local port or path specific.  There
 * are currently no remote port or system specific CI PPD severe error events.
 * cippd_conlog(), cippd_clse[], cippd_cltab[][], and cippd_map_spc[] must be
 * updated to reflect new additions.  cippd_log_path() must be updated when a
 * new path specific CI PPD severe error event is added.  PD logging data
 * structures may also require updating even though path specific CI PPD severe
 * error events are only logged by the CI PPD.  The need for such updating is
 * local port specific.  All PD logging routines and data structures must be
 * updated when a new local port specific CI PPD severe error event is added.
 * This is because CI PPD local port specific events are never logged by the CI
 * PPD.  They are always logged within the port driver appropriate to the
 * specific local port.  PD local port crashing routines may also require
 * updating when new local port specific CI PPD severe events are added.  The
 * need for such updating is local port specific.
 *
 * The following CI PPD severe error events are local port specific and may
 * have the local port crash severity modifier( ESM_LPC ) applied by the
 * appropriate port driver specific routine:
 *
 *	NOPATH, PPDSANITY
 *
 * The following CI PPD severe error events are path specific and may have the
 * path crash severity modifier( ESM_PC ) applied:
 *
 *	PD, BADPPDMTYPE
 *
 * The following CI PPD severe error events are path specific but may NOT have
 * the path crash severity modifier( ESM_PC ) applied:
 *
 *	PPDPROTOCOL
 *
 * Path specific information is always displayed by default during console of
 * logging of path specific CI PPD severe error events.  The local port station
 * address is always displayed by default during console logging of local port
 * specific CI PPD severe error events by the appropriate port drivers.
 *
 * NOTE: Not all path specific CI PPD severe error events are candidates for
 *	 application of the path crash severity modifier( ESM_PC ).  Local port
 *	 specific CI PPD severe error events are ( currently ) always
 *	 candidates for application of the local port crash severity modifier(
 *	 ESM_LPC ).
 *
 * NOTE: Update cippd_map_spc[] with a NULL entry whenever a new CI PPD severe
 *	 error event is local port specific and can never have the path crash
 *	 severity modifier( ESM_PC ) applied; or, is path specific but is not a
 *	 candidate for application of this severity modifier.
 *
 * NOTE: Update cippd_clse[] with a NULL entry whenever a new CI PPD severe
 *	 error event is local port specific.  Such events are only logged on
 *	 behalf of the CI PPD by port drivers.  They are never directly logged
 *	 by the CI PPD.  Likewise, update PD console logging format tables with
 *	 NULL entries whenever a new CI PPD severe error event is path
 *	 specific.  Such events are always directly logged by the CI PPD and
 *	 are never logged by individual port drivers.
 *
 * NOTE: The CI PPD may NOT directly request PD logging of local port specific
 *	 CI PPD severe error events.  It can only indirectly request their
 *	 logging by attempting to crash the specific local port through
 *	 invocation of the appropriate PD specific local port crashing routine(
 *	 Crash_lport ).
 *
 * NOTE: When updating cippd_clse[] with a new CI PPD severe error event bear
 *	 in mind the information displayed by default when choosing an
 *	 appropriate console formatting code( CF ).
 */
#define	SE_PPDPROTOCOL ( PPDSE | 0x01 )	/* PPD protocol violation occurred   */
#define	SE_PD	       ( PPDSE | 0x02 )	/* Port driver requested path failure*/
#define	SE_NOPATH      ( PPDSE | 0x03 )	/* Attempted crash nonexistent path  */
#define	SE_PPDSANITY   ( PPDSE | 0x04 )	/* CI PPD sanity check failed	     */
#define	SE_BADPPDMTYPE ( PPDSE | 0x05 )	/* Unsupported CI PPD message type   */
#define SE_MAX_PPD     SE_BADPPDMTYPE	/* Max CI PPD severe error event code*/

					/* CI PPD Fatal Error Event Codes    */
/* No codes are defined because the CI PPD currently never defines any fatal
 * error events.
 */

					/* Common Sys Level Logging Options  */
#define	CLEAR			0	/* Clear system level logging bit    */
#define	SET			1	/* Test-Set system level logging bit */

/* CI PPD Data Structures.
 */
typedef struct _cippd_atab {		/* CI PPD Action Function Sets	     */
    u_long	flags;			/* Action function flags	     */
#define	CIPPD_NOSTATUS		0	/*  Action function NOT return value */
#define	CIPPD_STATUS		1	/*  Action function returns u_long   */
    u_long	( *action )();		/* Action function address	     */
} CIPPD_ATAB;

typedef struct _cippd_slibq {		/* CIPPD_SLIB Queue Pointers	     */
    struct _cippd_slibq	*flink;		/* Forward queue link		     */
    struct _cippd_slibq	*blink;		/* Backward queue link		     */
} cippd_slibq;

					/* CI PPD System Level Event Logging */
typedef struct _cippd_slib {		/*  Information Block		     */
    struct _cippd_slibq *flink;		/* CI PPD system level logging event */
    struct _cippd_slibq *blink;		/*  information database queue ptrs  */
    u_short		size;		/* Size of data structure	     */
    u_char		type;		/* Structure type		     */
    u_char			: 8;
    u_long		log_flags;	/* Event logging flags		     */
#define	SWTYPE		0x00000001	/*  Log unsupported rem sys sw type  */
#define	TALKUP		0x00000002	/*  Log attempt to talk up	     */
#define	TALKDOWN	0x00000004	/*  Log refusal to talk down	     */
    u_char		node_name[ NODENAME_SIZE ];/* SCA node name	     */
} CIPPD_SLIB;

					/* CI PPD State Transition Table     */
typedef struct _cippd_stab {		/*  Entry			     */
    struct _cippd_atab	*actions;	/* Action function set pointer	     */
    u_long		fstate;		/* New path state on success	     */
} CIPPD_STAB;

/* CI PPD Message Definitions.
 */
typedef struct _cippdh {		/* CI PPD Buffer Header		     */
    u_short	length;			/* Length of message/datagram	     */
    u_short	mtype;			/* PPD message type		     */
} CIPPDH;

typedef	struct	{			/* START and STACK Datagrams	     */
    c_scaaddr	sysid;			/* System identification number	     */
    u_short 	protocol;		/* Protocol version		     */
#define	CIPPD_VERSION	1		/*  CI PPD protocol version	     */
    u_short 	max_dg;			/* Maximum size of application dg    */
    u_short 	max_msg;		/* Maximum size of application msg   */
    u_long	swtype;			/* Software type 		     */
    u_long	swver;			/* Software version		     */
    u_quad	swincrn;		/* Software incarnation number	     */
    u_long	hwtype;			/* Hardware type		     */
    u_dodec	hwver;			/* Hardware version		     */
    u_char	nodename[ 8 ];		/* SCA node name		     */
    u_quad	cur_time;		/* Current system time		     */
} CIPPD_START, CIPPD_STACK;

/* CI PPD Macros.
 */
					/* Mandatory PD Functions for CI PPD */
#define	Get_port	pdt->opt1	/* Retrieve Port Number from Buffer  */
#define	Init_port	pdt->opt2	/* Initialize/Re-initialize a Port   */
#define	Log_badportnum	pdt->opt3	/* Log PD Originating Bad Port Num   */
#define	Send_reqid	pdt->opt4	/* Request Remote Port Identification*/
#define	Set_circuit	pdt->opt5	/* Set Virtual Circuit State - On/Off*/
					/* Optional PD Functions for CI PPD  */
#define	Alloc_buf	pdt->opt6	/* Allocate Emergency Command Packets*/
#define	Dealloc_buf	pdt->opt7	/* Deallocate Emergency Command Pkts */
#define	Init_pb		pdt->opt8	/* Initialize a Path Block	     */
#define	Inv_cache	pdt->opt9	/* Invalidate Port Translation Cache */
#define	Notify_port	pdt->opt10	/* Notify Port of CI PPD Activity    */
#define	Test_lportconn	pdt->opt11	/* Test Local Port Connectivity	     */
#define	Update_ptype	pdt->opt12	/* Update Hardware Port Type	     */

					/* Action Function Sets Macros
					 */
#define	Fn_nostatus( fn )	{ CIPPD_NOSTATUS, ( u_long (*)() )fn   }
#define	Fn_status( fn )		{ CIPPD_STATUS,   ( u_long (*)() )fn   }
#define	End_fnset()		{ 0,		  ( u_long (*)() )NULL }

					/* Bitmap Manipulation Macros
					 */
/* SMP: The PCCB must be locked PRIOR to invocation.
 */
#define	Mapbyte( map, port )	( pccb->map[( port / 8 )])
#define	Mapbit( port )		( 1 << ( port % 8 ))
#define	Clear_lpinfomap( map, port ) {					\
    Mapbyte( lpinfo.map, ( long )port ) &= ~Mapbit(( long )port );	\
}
#define	Clear_map( map, port ) {					\
    Mapbyte( map, ( long )port ) &= ~Mapbit(( long )port );		\
}
#define	Set_lpinfomap( map, port ) {					\
    Mapbyte( lpinfo.map, ( long )port ) |= Mapbit(( long )port );	\
}
#define	Set_map( map, port ) {						\
    Mapbyte( map, ( long )port ) |= Mapbit(( long )port );		\
}
#define	Test_lpinfomap( map, port )					\
    ( Mapbyte( lpinfo.map, ( long )port ) & Mapbit(( long )port ))
#define	Test_map( map, port )						\
    ( Mapbyte( map, ( long )port ) & Mapbit(( long )port ))

					/* CI PPD Datagram Macros
					 */
#define	Cippd_stack( bp )	(( CIPPD_STACK * )bp )
#define	Cippd_start( bp )	(( CIPPD_START * )bp )

					/* CI PPD Event Logging Macros
					 */
#define	Cippd_clmaxcode( tab, event )					\
    ( tab[ Eseverity( event )][ Eclass( event )].max_code )
#define	Cippd_clftab( tab, event )					\
    ( tab[ Eseverity( event )][ Eclass( event )].ftable + Ecode( event ) - 1 )
#define	Cippd_cltabmsg( tab, event )	( Cippd_clftab( tab, event )->msg )
#define	Cippd_cltabcode( tab, event )	( Cippd_clftab( tab, event )->fcode )
#define	Elcippdcommon( elp )	( &elp->el_body.elcippd.cippdcommon )
#define	Elcippddbcoll( elp )						\
    ( &elp->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippddbcoll )
#define	Elcippdnewpath( elp )						\
    ( &elp->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippdnewpath )
#define	Elcippdpcommon( elp )						\
    ( &elp->el_body.elcippd.cippdtypes.cippdpath.cippdpcommon )
#define	Elcippdppacket( elp )						\
    ( &elp->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippdppacket )
#define	Elcippdprotocol( elp )						\
    (&elp->el_body.elcippd.cippdtypes.cippdsystem.cippdsystemopt.cippdprotocol)
#define	Elcippdscommon( elp )						\
    ( &elp->el_body.elcippd.cippdtypes.cippdsystem.cippdscommon )
#define	Elcippdspacket( elp )						\
    ( &elp->el_body.elcippd.cippdtypes.cippdsystem.cippdsystemopt.cippdspacket)
#define	Elcippdsysapnam( elp )						\
    ( &elp->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippd_sysap[ 0 ])


/* SMP: TEMP */				/* CI PPD SMP Locking Macros
					 */
#define	Fetch_time( to, from ) {					\
    to = from;								\
}
/* PBs must be locked whenever their semaphores are accessed.
 */
#define	Decr_pb_sem( pb ) {						\
    ++pb->Dbiip;							\
}
#define	Incr_pb_sem( pb ) {						\
    --pb->Dbiip;							\
}
#define	Test_pb_sem( pb )	( U_long( pb->Dbiip ) & 0xffff )

					/* Scheduling Macros
					 */
#define	Pb_fork( pb, routine, panstr ) {				\
    if( !Pb->Fsmpstatus.fkip ) {					\
	Pb->Fsmpstatus.fkip = 1;					\
	Kfork( &pb->Forkb, routine, pb )				\
    } else {								\
	( void )panic( panstr );					\
    }									\
}
#define	Pb_fork_done( pb, panstr ) {					\
    if( pb->Fsmpstatus.fkip ) {						\
	pb->Fsmpstatus.fkip = 0;					\
    } else {								\
	( void )panic( panstr );					\
    }									\
}
#define	Pccb_fork( pccb, routine, panstr ) {				\
    if( !pccb->Fsmstatus.fkip ) {					\
	pccb->Fsmstatus.fkip = 1;					\
	Kfork( &pccb->forkb, routine, pccb )				\
    } else {								\
	( void )panic( panstr );					\
    }									\
}
#define	Pccb_fork_done( pccb, panstr ) {				\
    if( pccb->Fsmstatus.fkip ) {					\
	pccb->Fsmstatus.fkip = 0;					\
    } else {								\
	( void )panic( panstr );					\
    }									\
}

					/* Shorthand Notations		     */
#define	Aflogmap	ppd.cippd.aflogmap
#define	Burst		ppd.cippd.burst
#define	Contact		ppd.cippd.contact
#define	Dbclogmap	ppd.cippd.dbclogmap
#define	Dbiip		ppd.cippd.dbiip
#define	Due_time	ppd.cippd.due_time
#define	Elogopt		ppd.cippd.elogopt
#define	Forkb		ppd.cippd.forkb
#define	Fsmpstatus	ppd.cippd.fsmpstatus
#define	Fsmstatus	ppd.cippd.fsmstatus
#define	Form_pb		ppd.cippd.form_pb
#define	Max_cables	ppd.cippd.max_cables
#define	Max_port	ppd.cippd.max_port
#define	Next_port	ppd.cippd.next_port
#define	Nform_paths	ppd.cippd.nform_paths
#define	Npaths		ppd.cippd.npaths
#define	Open_pb		ppd.cippd.open_pb
#define	Poll_due	ppd.cippd.poll_due
#define	Poll_interval	ppd.cippd.poll_interval
#define	Poll_cable	ppd.cippd.poll_cable
#define	Ppddgs		ppd.cippd.ppddgs
#define	Protocol	ppd.cippd.protocol
#define	Retry		ppd.cippd.retry
#define	Sanity_port	ppd.cippd.sanity_port
#define	Slib		(( CIPPD_SLIB * )slib )
#define	Timer_interval	ppd.cippd.timer_interval
#define	Tmologmap	ppd.cippd.tmologmap

					/* Miscellaneous Macros
					 */
#define	Maxport( pccb )							\
    (( cippd_max_port < pccb->lpinfo.Max_port )				\
	? cippd_max_port : pccb->lpinfo.Max_port )
#define	Pinterval( pccb )						\
    ((( pccb->Contact > cippd_itime ) ? pccb->Contact : cippd_itime ) * \
     (( pccb->Burst <= ( Maxport( pccb ) + 1 ))			    	\
	    ? pccb->Burst : ( Maxport( pccb ) + 1 ))		      / \
     ( Maxport( pccb ) + 1 ))
