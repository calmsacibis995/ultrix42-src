/*
 @(#)scs.h	4.2  (ULTRIX)        10/16/90
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
 *		Systems Communication Services
 *
 *   Abstract:	This module contains Systems Communication Services( SCS )
 *		constants, data structure definitions, message definitions,
 *		and macros.
 *
 *   Creator:	Todd M. Katz	Creation Date:	March 23, 1985
 *
 *   Modification History:
 *
 *   16-Oct-1989	Pete Keilty
 *	Changed Port_failure macro PF_ERROR check to PF_PORTERROR.
 *	This was done so that on path failures due to none port errors 	
 *	connect resources would be returned.
 *
 *   14-Jun-1989	Pete Keilty
 *	Add locking around decrement of cbvte semaphore.
 *
 *   06-Apr-1989	Pete Keilty
 *	Added the new system smp locking.
 *
 *   06-Dec-1988	Todd M. Katz		TMK0006
 *	1. Added to structure definition PCCB( union pd ) MSI specific fields.
 *	2. Add the SCS informational events: NEW_LISTENER, NEW_CONN.
 *	3. Add the SCS warning events: TERM_LISTENER, TERM_CONN, FAIL_CONN,
 *	   TERM_FCONN, FAIL_FCONN, REJECT_FCONN, ABORT_FCONN.
 *	4. Add the event logging macros Elscscommon(), Elscsconn(),
 *	   Elscsrreason(), and Elscsldirid(), Scs_clmaxcode(), Scs_clftab(),
 *	   Scs_cltabmsg(), and Scs_cltabcode().
 *	5. Define constants for the types of events( CONN_EVENT, LSYSAP_EVENT).
 *	6. Add the console logging format code CF_NONE.
 *	7. Add the panic strings SCSPANIC_UNKCF and SCSPANIC_UNKCOD.
 *	8. Add union errlogopt to structure definition CB. 
 *
 *   28-Sep-1988	Todd M. Katz		TMK0005
 *	Eliminate the following unused definition: SCSPANIC_DISCON.
 *
 *   22-Aug-1988	Todd M. Katz		TMK0004
 *	Rename SE_BADMTYPE -> SE_BADSCSMTYPE to avoid name space conflict.
 *
 *   15-Aug-1988	Todd M. Katz		TMK0003
 *	1. Moved definition of SCSPC back to ../vaxscs/sca.h.
 *	2. Remove opt13 as an optional PD function for use by PPDs.
 *	3. SCA event codes have been completely revised.  SCS path crash codes
 *	   are now divided between error events( SYSAP, TIMEOUT ) and severe
 *	   error events( BADCONNID, BADCSTATE, NEGCREDITS, BADMTYPE,
 *	   NOTRANSFERS, NOCREDITS ).  The path crash attribute is applied by
 *	   individual port driver path crash routines.
 *	4. E_MAX_SCS defines the maximum SCS error event code and SE_MAX_SCS
 *	   defines the maximum SCS severe error event code.
 *	5. Update the macro Port_failure() to return true if the path failure
 *	   reason is PF_{ POWER, ERROR, FATALERROR }.
 *
 *   23-Mar-1988	Todd M. Katz		TMK0002
 *	Moved definition of SCSPC from ../vaxscs/sca.h to here.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, and added
 *	SMP support.
 */

/* SCS and SCS-PD Interface Constants.
 */
					/* Additional Buffer Disposal Codes  */
#define	RETURN_BUF		-1	/* Return Buffer		     */

					/* Buffer Presence/Absence Codes     */
#define	BUF			 1	/* Buffer present		     */
#define	NO_BUF			 2	/* Buffer absent		     */

					/* Console Logging Format Codes	     */
#define	CF_NONE			0	/* No optional information is logged */

					/* SCS Informational Event Codes     */
/* SCS informational events are either connection or local SYSAP specific.
 * There are currently no path specific SCS informational events.
 * scs_conlog(), scs_log_event(), scs_cli[], and scs_cltab[][] must be updated
 * to reflect new additions.
 *
 * The following SCS informational events are connection specific:
 *
 *	NEW_CONN
 *
 * Connection specific information is always displayed by default during
 * console logging of connection specific SCS informational events.
 *
 * The following SCS informational events are local SYSAP specific:
 *
 *	NEW_LISTENER
 *
 * Local SYSAP specific information is always displayed by default during
 * console logging of local SYSAP specific SCS informational events.
 *
 * NOTE: When updating scs_cli[] with a new SCS informational event bear in
 *	 mind the information displayed by default when choosing an appropriate
 *	 console formatting code( CF ).
 */
#define	I_NEW_LISTENER	( SCSI | 0x01 )	/* SYSAP established new listener    */
#define	I_NEW_CONN	( SCSI | 0x02 )	/* SYSAP established new connection  */

					/* SCS Warning Event Codes	     */
/* SCS warning events are either connection or local SYSAP specific.  There are
 * currently no path specific SCS warning events.  scs_conlog(),
 * scs_log_event(), scs_clw[], and scs_cltab[][] must be updated to reflect new
 * additions.
 *
 * The following SCS warning events are connection specific:
 *
 *	TERM_CONN, FAIL_CONN, TERM_FCONN, FAIL_FCONN, REJECT_FCONN, ABORT_FCONN
 *
 * Connection specific information is always displayed by default during
 * console logging of connection specific SCS warning events.
 *
 * The following SCS warning events are local SYSAP specific:
 *
 *	TERM_LISTENER
 *
 * Local SYSAP specific information is always displayed by default during
 * console logging of local SYSAP specific SCS warning events.
 *
 * NOTE: When updating scs_clw[] with a new SCS warning event bear in mind the
 *	 information displayed by default when choosing an appropriate console
 *	 formatting code( CF ).
 */
#define	W_TERM_LISTENER	( SCSW | 0x01 )	/* SYSAP terminated listener	     */
#define	W_TERM_CONN	( SCSW | 0x02 )	/* SYSAP terminated connection	     */
#define	W_FAIL_CONN	( SCSW | 0x03 )	/* Path failure terminated connection*/
#define	W_TERM_FCONN	( SCSW | 0x04 )	/* Rem SCS rejected conn establish   */
#define	W_FAIL_FCONN	( SCSW | 0x05 )	/* Path fail aborted conn establish  */
#define	W_REJECT_FCONN	( SCSW | 0x06 )	/* SYSAP rejected connect establish  */
#define	W_ABORT_FCONN	( SCSW | 0x07 )	/* SYSAP aborted conn establishment  */

					/* SCS Remote Error Event Codes	     */
/* No codes are defined because SCS currently never defines any remote error
 * events.
 */

					/* SCS Error Event Codes	     */
/* SCS error events are always path specific.  There are currently no
 * connection or local SYSAP specific SCS error events.  scs_map_pc[] must be
 * updated to reflect new additions.  All PD logging routines and data
 * structures must also be updated when a new SCS path specific error event is
 * added.  This is because SCS path specific error events are never logged by
 * SCS.  They are always logged within the port driver appropriate to the
 * specific path.
 *
 * The following SCS error events are path specific and may have the path crash
 * severity modifier( ESM_PC ) applied by the appropriate port driver specific
 * routine:
 *
 *	SYSAP, TIMEOUT
 *
 * Path specific information is always displayed by default during console
 * logging of path specific SCS error events by the appropriate port drivers.
 *
 * NOTE: Path specific SCS error events are ( currently ) always candidates for
 *	 application of the path crash severity modifier( ESM_PC ).
 *
 * NOTE: SCS may NOT directly request PD logging of path specific SCS error
 *	 events.  It can only indirectly request their logging by attempting to
 *	 crash the specific path through invocation of the appropriate PD
 *	 specific path crashing routine( Crash_path ).
 */
#define	E_SYSAP       ( SCSE | 0x01 )	/* SYSAP requested failure	     */
#define	E_TIMEOUT     ( SCSE | 0x02 )	/* SCS request timeout occurred	     */
#define	E_MAX_SCS	E_TIMEOUT	/* Maximum SCS error event code	     */

					/* SCS Severe Error Event Codes	     */
/* SCS severe error events are always path specific.  There are currently no
 * connection or local SYSAP specific SCS severe error events.  scs_map_spc[]
 * must be updated to reflect new additions.  All PD logging routines and data
 * structures must also be updated when a new SCS path specific severe error
 * event is added.  This is because SCS path specific severe error events are
 * never logged by SCS.  They are always logged within the port driver
 * appropriate to the specific path.
 *
 * The following SCS severe error events are path specific and may have the
 * path crash severity modifier( ESM_PC ) applied by the appropriate port
 * driver specific routine:
 *
 *	BADCONNID, BADCSTATE, NEGCREDITS, BADSCSMTYPE, NOTRANSFERS, NOCREDITS
 *
 * Path specific information is always displayed by default during console
 * logging of path specific SCS severe error events by the appropriate port
 * drivers.
 *
 * NOTE: Path specific SCS severe error events are ( currently ) always
 *	 candidates for application of the path crash severity modifier(
 *	 ESM_PC ).
 *
 * NOTE: SCS may NOT directly request PD logging of path specific SCS severe
 *	 error events.  It can only indirectly request their logging by
 *	 attempting to crash the specific path through invocation of the
 *	 appropriate PD specific path crashing routine( Crash_path ).
 */
#define	SE_BADCONNID   ( SCSSE | 0x01 )	/* Illegal connection identification */
#define	SE_BADCSTATE   ( SCSSE | 0x02 )	/* Illegal connection state	     */
#define	SE_NEGCREDITS  ( SCSSE | 0x03 )	/* Unsupported withdrawal of credits */
#define	SE_BADSCSMTYPE ( SCSSE | 0x04 )	/* Unsupported SCS message type	     */
#define	SE_NOTRANSFERS ( SCSSE | 0x05 )	/* Unexpected block data transfer    */
#define	SE_NOCREDITS   ( SCSSE | 0x06 )	/* Message transmitted without credit*/
#define	SE_MAX_SCS	SE_NOCREDITS	/* Max SCS severe error event code   */

					/* SCS Fatal Error Event Codes	     */
/* No codes are defined because SCS currently never defines any fatal error
 * events.
 */

					/* PPD Message Types		     */
#define	SCSDG			 3	/* SCS datagram			     */
#define	SCSMSG			 4	/* SCS message			     */

					/* SCS Message Types		     */
#define	SCS_CONN_REQ		 0	/* SCS CONNECT request message       */
#define	SCS_CONN_RSP		 1	/* SCS CONNECT response message      */
#define	SCS_ACCEPT_REQ		 2	/* SCS ACCEPT request message	     */
#define	SCS_ACCEPT_RSP		 3	/* SCS ACCEPT response message       */
#define	SCS_REJECT_REQ		 4	/* SCS REJECT request message        */
#define	SCS_REJECT_RSP		 5	/* SCS REJECT response message       */
#define	SCS_DISCONN_REQ		 6	/* SCS DISCONNECT request message    */
#define	SCS_DISCONN_RSP		 7	/* SCS DISCONNECT response message   */
#define	SCS_CREDIT_REQ		 8	/* SCS CREDIT request message        */
#define	SCS_CREDIT_RSP		 9	/* SCS CREDIT response message       */
#define	SCS_APPL_MSG  		10	/* Application sequenced message     */
#define	SCS_APPL_DG    		11	/* Application datagram		     */
#define	SCS_MAX_SCSMSG	SCS_CREDIT_RSP	/* SCS maximum message type	     */

					/* SCS Panic Strings		     */
#define	SCSPANIC_ABORT	"scs\t- bad connid seen during connection abortion\n"
#define	SCSPANIC_CSTATE	"scs\t- invalid connection state\n"
#define	SCSPANIC_EVENT	"scs\t- invalid asynchronous event on connection\n"
#define	SCSPANIC_LQUEUE	"scs\t- corrupted listening sysap queue\n"
#define	SCSPANIC_NABORT	"scs\t- unexpected connection abortion occurred\n"
#define	SCSPANIC_SANITY	"scs\t- broken sanity timer\n"
#define	SCSPANIC_SCADB	"scs\t- corrupted sca configuration database\n"
#define	SCSPANIC_SCSMSG	"scs\t- unknown scs message type requested\n"
#define	SCSPANIC_UNKCF	"scs\t- unknown console logging formatting code\n"
#define	SCSPANIC_UNKCOD	"scs\t- unknown/invalid event code\n"

					/* Types of SCS Events		     */
#define	CONN_EVENT		0	/* Connection specific event	     */
#define	LSYSAP_EVENT		1	/* Local SYSAP specific event	     */

/* SCS Data Structure Definitions.
 */
typedef struct	_cb	{		/* Connection Block		     */
    struct _cbq	     *flink;		/* System-wide configuration 	     */
    struct _cbq	     *blink;		/*  database queue pointers 	     */
    u_short	     size;		/* Size of data structure	     */
    u_char	     type;		/* Structure type		     */
    u_char			:  8;
    struct _cbq	     scs_cb;		/* SCS waiting CB queue pointers     */
    void	     ( *control )();	/* SYSAP control routine	     */
    void	     ( *msg_event )();	/* Msg event notification routine    */
    void	     ( *dg_event )();	/* Dg event notification routine     */
    struct _pb	     *pb;		/* PB pointer			     */
    struct _pdt	     *pdt;		/* PDT pointer			     */
    struct _pccb     *pccb;		/* PCCB pointer			     */
    u_char	     *aux;		/* SYSAP auxiliary structure pointer */
    struct kschedblk forkb;		/* Fork block for connection abortion*/
    union		{		/* Optional error logging information*/
	u_long		rreason;	/*  SYSAP/SCS conn rejection reason  */
    } errlogopt;
    struct _cib	     cinfo;		/* Connection specific information   */
} CB;

typedef struct _cbvte	{		/* CB Vector Table Entry	     */
    union		{		/* First overlaid field		     */
	struct _cb	*cb;		/*  CB pointer			     */
	struct _cbvte	*cbvte;		/*  Next free CBVTE pointer	     */
    } ov1;
    struct _connid connid;		/* Identification number	     */
    u_short	   cbip;		/* Call back in progress semaphore   */
    u_short	   		: 16;	/*  ( must be longword aligned )     */
    struct lock_t  cbvte_lk;		/* Lock structure		     */
} CBVTE;

typedef struct	_cbvtdb	{		/* CB Vector Table Database	     */
    CBVTE	 *free_cbvte;		/* Free CBVTE list head		     */
    CBVTE	 *cbvt;			/* CB vector table pointer	     */
    u_short	 size;			/* Size of data structure	     */
    u_char	 type;			/* Structure type		     */
    u_char			:  8;
} CBVTDB;

typedef struct _pb	{		/* Path Block			     */
    struct _pbq	 *flink;		/* System-wide configuration	     */
    struct _pbq	 *blink;		/*  database queue pointers	     */
    u_short	 size;			/* Size of data structure	     */
    u_char	 type;			/* Structure type		     */
    u_char			:  8;
    struct _pbq	 timeout;		/* SCS protocol seq timeout q ptrs   */
    struct _cbq	 cbs;			/* CB queue head		     */
    struct _cbq	 scs_cb;		/* SCS waiting CB queue head	     */
    struct _sb	 *sb;			/* SB pointer			     */
    struct _pccb *pccb;			/* PCCB pointer			     */
    struct _pdt	 *pdt;			/* PDT pointer			     */
    struct _scsh *scs_msgbuf;		/* SCS send message buffer pointer   */
    struct lock_t pb_lk;		/* Lock structure		     */
    union		{		/* PD dependent fields		     */
	union	_gvppb	gvp;		/*  Generic Vaxport specific fields  */
    } pd;
    union		 {		/* PPD dependent fields		     */
	struct	_cippdpb cippd;		/*  CI PPD specific fields	     */
    } ppd;
    struct _pib	 pinfo;			/* Path specific information	     */
} PB;

typedef struct _sb	{		/* System Block			     */
    struct _sbq	*flink;			/* System-wide configuration	     */
    struct _sbq	*blink;			/*  database queue pointers	     */
    u_short	size;			/* Size of data structure	     */
    u_char	type;			/* Structure type		     */
    u_char			:  8;
    struct _pbq pbs;			/* PB queue head		     */
    struct _sib	sinfo;			/* System specific information	     */
} SB; 

/* Generic Port Driver( PD ) Data Structure Definitions.
 */
typedef struct _pccb	{		/* Port Command and Control Block    */
    union		{		/* PD dependent fields		     */
					/*  ( MUST BE FIRST IN PCCB !! )     */
	struct _gvppccb	gvp;		/*  Generic Vaxport specific fields  */
	struct _msipccb	msi;		/*  MSI specific fields		     */
	struct _uqpccb	uq;		/*  UQ specific fields		     */
    } pd;
    struct _pccbq    *flink;		/* System-wide local port database   */
    struct _pccbq    *blink;		/*   queue pointers		     */
    u_short	     size;		/* Size of data structure	     */
    u_char	     type;		/* Structure type		     */
    u_char			:  8;
    struct _pdt	     *pdt;		/* PDT pointer			     */
    struct kschedblk forkb;		/* Fork block			     */
    struct lock_t    pccb_lk;		/* Lock structure		     */
    union		  {		/* PPD dependent fields		     */
	struct _cippdpccb cippd;	/*  CI PPD specific fields	     */
    } ppd;
    struct _lpib     lpinfo;		/* Local port information	     */
} PCCB;

typedef struct _pdt	{		/* Port Dispatch Table		     */
					/* Mandatory PD Functions	     */
    struct _scsh *( *alloc_dg )();	/* Allocate Datagram Buffer	     */
    void	  ( *dealloc_dg )();	/* Deallocate Datagram Buffer	     */
    void	  ( *add_dg )();	/* Add Datagram Buffer to Free Pool  */
    struct _scsh *( *remove_dg )();	/* Remove Dg Buffer from Free Pool   */
    struct _scsh *( *alloc_msg )();	/* Allocate Message Buffer	     */
    void	  ( *dealloc_msg )();	/* Deallocate Message Buffer	     */
    void	  ( *add_msg )();	/* Add Message Buffer to Free Pool   */
    struct _scsh *( *remove_msg )();	/* Remove Msg Buffer from Free Pool  */
    void	  ( *send_msg )();	/* Send Sequenced Message	     */
    u_long	  ( *map_buf )();	/* Map Buffer			     */
    void	  ( *unmap_buf )();	/* Unmap Buffer			     */
    u_long	  ( *open_path )();	/* Transition Formative Path to Open */
    void	  ( *crash_path )();	/* Crash Path			     */
    struct _pb	 *( *get_pb )();	/* Retrieve Path Block		     */
    void	  ( *remove_pb )();	/* Remove Path Block from Databases  */
    u_long        ( *remote_reset )();  /* Reset Remote Port and System      */
    u_long        ( *remote_start )();  /* Start Remote Port and System      */
					/* Optional PD Functions	     */
    void	  ( *send_dg )();	/* Send Datagram		     */
    u_long	  ( *send_data )();	/* Send Block Data		     */
    u_long	  ( *request_data )();	/* Request Block Data		     */
    void	  ( *crash_lport )();	/* Crash Local Port		     */
    void	  ( *shutdown )();	/* Inform Systems of Local Shutdown  */
					/* Optional PD Functions for PPD     */
    u_long	  ( *opt1 )();
    u_long	  ( *opt2 )();
    u_long	  ( *opt3 )();
    u_long	  ( *opt4 )();
    u_long	  ( *opt5 )();
    u_long	  ( *opt6 )();
    u_long	  ( *opt7 )();
    u_long	  ( *opt8 )();
    u_long	  ( *opt9 )();
    u_long	  ( *opt10 )();
    u_long	  ( *opt11 )();
    u_long	  ( *opt12 )();
} PDT;

typedef struct _tid	{		/* Transaction Identification Number */
    u_long	   blockid;		/* Block data transfer ID number     */
    struct _connid lconnid;		/* Local connection ID number	     */
} TID;

/* SCS Sequenced Message Definitions.
 */
					/* ACCEPT_REQ and CONN_REQ Sequenced */
typedef	struct	{			/*  Messages			     */
    u_short   	mtype;			/* SCS message type		     */
    short	credit;			/* Flow control credit		     */
    struct _connid rconnid;		/* Identification of receiving SYSAP */
					/* MBZ for CONN_REQ		     */
    struct _connid sconnid;		/* Identification of sending SYSAP   */
    u_short	min_credit;		/* Minimum credit requirement	     */
    u_short			: 16;	/* MBZ				     */
    u_char	rproc_name[ NAME_SIZE ];/* Receiving SYSAP name		     */
    u_char	sproc_name[ NAME_SIZE ];/* Sending SYSAP name		     */
    u_char	sproc_data[ DATA_SIZE ];/* Connect data from sending SYSAP   */
} ACCEPT_REQ, CONN_REQ;

					/* ACCEPT_RSP, CONN_RSP, DISCONN_REQ */
typedef struct	{			/*  and REJECT_REQ Sequenced Messages*/
    u_short	   mtype;		/* SCS message type		     */
    u_short			: 16;	/* MBZ				     */
    struct _connid rconnid;		/* Identification of receiving SYSAP */
    struct _connid sconnid;		/* Identification of sending SYSAP   */
    u_short			: 16;	/* MBZ				     */
    u_short	   reason;		/* Reason			     */
#define	Req_status reason		/*  Request status		     */
} ACCEPT_RSP, CONN_RSP, DISCONN_REQ, REJECT_REQ;

					/* CREDIT_REQ, CREDIT_RSP, 	     */
					/*  DISCONN_RSP, and REJECT_RSP      */
typedef struct _scsh	{		/*  Sequenced Messages and SCS Header*/
    u_short   	   mtype;		/* SCS message type		     */
    short	   credit;		/* Flow control credit		     */
					/* MBZ for DISCONN_RSP and REJECT_RSP*/
    struct _connid rconnid;		/* Identification of receiving SYSAP */
    struct _connid sconnid;		/* Identification of sending SYSAP   */
} CREDIT_REQ, CREDIT_RSP, DISCONN_RSP, REJECT_RSP, SCSH;

/* Macros.
 */
					/* CB Manipulation Macros
					 */
/* SMP: Both SCA database and CBVTE must be locked PRIOR to invocation.
 */
#define	Insert_cb( cb, pb ) {						\
    Insert_entry( cb->flink, pb->cbs )					\
    ++pb->pinfo.nconns;							\
}
/* SMP: Both SCA database and CBVTE must be locked PRIOR to invocation.
 */
#define	Remove_cb( cb, pb ) {						\
    Remove_entry( cb->flink )						\
    --pb->pinfo.nconns;							\
    ( void )scs_dealloc_cb( cb );					\
}
					/* Connection Manipulation Macros
					 */
/* SMP: The CBVTE is locked on successful invocation.
 */
#define	Check_connid( sourceid, cbvte, cb ) {				\
    if( sourceid.index <= ( lscs.max_conns - 1 )) {			\
	cbvte = Get_cbvte( sourceid );					\
	Lock_cbvte( cbvte )						\
	if( cbvte->connid.seq_num == sourceid.seq_num ) {		\
	    cb = Get_cb( cbvte );					\
	} else {							\
	    Unlock_cbvte( cbvte )					\
	    return( RET_INVCONNID );					\
	}								\
    } else {								\
	    return( RET_INVCONNID );					\
    }									\
}
/* SMP: Both the SCA database and CBVTE are locked on successful invocation.
 */
#define	Check_connid2( sourceid, cbvte, cb ) {				\
    if( sourceid.index <= ( lscs.max_conns - 1 )) {			\
	cbvte = Get_cbvte( sourceid );					\
	Lock_scadb()							\
	Lock_cbvte( cbvte )						\
	if( cbvte->connid.seq_num == sourceid.seq_num ) {		\
	    cb = Get_cb( cbvte );					\
	} else {							\
	    Unlock_cbvte( cbvte )					\
	    Unlock_scadb()						\
	    return( RET_INVCONNID );					\
	}								\
    } else {								\
	return( RET_INVCONNID );					\
    }									\
}
#define	Get_cbvte( sourceid )	( scs_cbvtdb->cbvt + sourceid.index )
#define	Get_cb( cbvte )		( cbvte->ov1.cb )
#define	Comp_connid( c1, c2 )	( U_long( c1 ) == U_long( c2 ))

					/* Counter Macros
					 */
/* SMP: The counter containing structure must be locked PRIOR to invocation.
 */
#define	Data_counter( counter, bytes ) {				\
    if( counter != -1 ) {						\
	if(( long )counter < 0 ) {					\
	    if(( long )( counter += bytes ) > 0 ) {			\
		counter = -1;						\
	    }								\
	} else {							\
	    counter += bytes;						\
	}								\
    }									\
}
/* SMP: The counter containing structure must be locked PRIOR to invocation.
 */
#define	Event_counter( counter ) {					\
    if( ++counter == 0 ) {						\
	--counter;							\
    }									\
}

					/* Positioning and Message Macros
					 */
#define	Accept_req( bp )	(( ACCEPT_REQ * )bp )
#define	Accept_rsp( bp )	(( ACCEPT_RSP * )bp )
#define	Conn_req( bp )		(( CONN_REQ * )bp )
#define	Conn_rsp( bp )		(( CONN_RSP * )bp )
#define	Disconn_req( bp )	(( DISCONN_REQ * )bp )
#define	Reject_req( bp )	(( REJECT_REQ * )bp )
#define	Appl_to_scs( bp )	(( SCSH * )bp - 1 )
#define	Scs_to_appl( scsbp )	(( u_char * )( scsbp + 1 ))
#define	Cb_offset( ptr, offset )					\
    (( u_char * )&(( CB * )ptr )->offset - ( u_char * )ptr )
#define	Pb_offset( ptr, offset )					\
    (( u_char * )&(( PB * )ptr )->offset - ( u_char * )ptr )
#define	Pccb_offset( ptr, offset )					\
    (( u_char * )&(( PCCB * )ptr )->offset - ( u_char * )ptr )
#define	Pos_to_cb( ptr, offset )					\
    (( CB * )(( u_char * )ptr - Cb_offset( ptr, offset )))
#define	Pos_to_pb( ptr, offset )					\
    (( PB * )(( u_char * )ptr - Pb_offset( ptr, offset )))
#define	Pos_to_pccb( ptr, offset )					\
    (( PCCB * )(( u_char * )ptr - Pccb_offset( ptr, offset )))

					/* Shorthand Notations
					 */
#define	Cb			(( CB * )cb )
#define	Lcb			(( CB * )lcb )
#define	Sb			(( SB * )sb )
#define	Pb			(( PB * )pb )
#define	Alloc_dg		pdt->alloc_dg
#define	Dealloc_dg		pdt->dealloc_dg
#define	Add_dg			pdt->add_dg
#define	Remove_dg		pdt->remove_dg
#define	Alloc_msg		pdt->alloc_msg
#define	Dealloc_msg		pdt->dealloc_msg
#define	Add_msg			pdt->add_msg
#define	Remove_msg		pdt->remove_msg
#define	Send_msg		pdt->send_msg
#define	Map_buf			pdt->map_buf
#define	Unmap_buf		pdt->unmap_buf
#define	Open_path		pdt->open_path
#define	Crash_path		pdt->crash_path
#define Get_pb			pdt->get_pb
#define	Remove_pb		pdt->remove_pb
#define	Remote_reset		pdt->remote_reset
#define	Remote_start		pdt->remote_start
#define	Send_dg			pdt->send_dg
#define Send_data		pdt->send_data
#define Request_data		pdt->request_data
#define	Crash_lport		pdt->crash_lport
#define	Shutdown		pdt->shutdown

					/* SCS Event Logging Macros
					 */
#define	Elscscommon( elp )	( &elp->el_body.elscs.scscommon )
#define	Elscsconn( p )		(( struct scs_conn * )p )
#define	Elscsrreason( p )	(( u_long * )p )
#define	Elscsldirid( p )	(( u_short * )p )
#define	Scs_clmaxcode( tab, event )					\
    ( tab[ Eseverity( event )].max_code )
#define	Scs_clftab( tab, event )					\
    ( tab[ Eseverity( event )].ftable + Ecode( event ) - 1 )
#define	Scs_cltabmsg( tab, event )	( Scs_clftab( tab, event )->msg )
#define	Scs_cltabcode( tab, event )	( Scs_clftab( tab, event )->fcode )

				/* SCS SMP Locking Macros
				 */
#define	Init_cbvte_lock( cbvte ) {					\
    lockinit(&(( cbvte )->cbvte_lk ), &lock_cbvte_d );			\
}
#define	Init_pb_lock( pb ) {						\
    lockinit(&(( pb )->pb_lk ), &lock_pb_d );				\
}
#define	Init_pccb_lock( pccb ) {					\
    lockinit(&(( pccb )->pccb_lk ), &lock_pccb_d );			\
}
#define	Init_scadb_lock() {						\
    lockinit( &lk_scadb, &lock_scadb_d );				\
}
#define	Lock_cbvte( cbvte ) {						\
    smp_lock(&(( cbvte )->cbvte_lk ), LK_RETRY );			\
}
#define	Lock_pb( pb ) {							\
    smp_lock(&(( pb )->pb_lk ), LK_RETRY );				\
}
#define	Lock_pccb( pccb ) {						\
    smp_lock(&(( pccb )->pccb_lk ), LK_RETRY );				\
}
#define	Lock_scadb() {							\
    smp_lock( &lk_scadb, LK_RETRY );					\
}
#define	Test_pb_lock( pb )	(( pb )->pb_lk.l_lock )
#define	Test_pccb_lock( pccb )	(( pccb )->pccb_lk.l_lock )
#define	Test_scadb_lock()	( lk_scadb.l_lock )
#define	Unlock_cbvte( cbvte ) {						\
    smp_unlock(&(( cbvte )->cbvte_lk ));				\
}
#define	Unlock_pb( pb ) {						\
    smp_unlock(&(( pb )->pb_lk ));					\
}
#define	Unlock_pccb( pccb ) {						\
    smp_unlock(&(( pccb )->pccb_lk ));					\
}
#define	Unlock_scadb() {						\
    smp_unlock( &lk_scadb );						\
}

/* CBVTE Semaphore Macros: CBVTEs are locked when their semaphores are tested
 * and incremented but not when they are decremented.  This allows for
 * semaphores to be simultaneously tested and decremented in a SMP environment.
 * This is acceptable because SCS is interested in only whether semaphores are
 * zero or non-zero.  Semaphores may test non-zero when actually zero but may
 * never test zero when non-zero.  What is necessary to meet this semaphore
 * testing requirement is an atomic fetch of semaphore contents.  This is
 * achieved by longword-aligning the word-size CBVTE semaphores.  Once
 * atomically fetched the semaphore contents can be isolated and tested.
 *
 * This Decr_cbvte_sem should be an atomic operation, memory lock for 
 * duration of operation such as adawi operation.
 * Unable to do so at this time, So lock and unlock the cbvte.
 */
#define	Decr_cbvte_sem( cbvte )	{					\
    Lock_cbvte( cbvte ) 						\
    ++cbvte->cbip;							\
    Unlock_cbvte( cbvte ) 						\
}
#define	Incr_cbvte_sem( cbvte )	{					\
    --cbvte->cbip;							\
}
#define	Test_cbvte_sem( cbvte )	( U_long( cbvte->cbip ) & 0xffff )

					/* Miscellaneous Macros
					 */
/* SMP: The CBVTE must be locked PRIOR to invocation.
 */
#define	Init_csb( csb, cb, scsbp, size )	{			\
    csb->connid = cb->cinfo.lconnid;					\
    csb->Aux = cb->aux;							\
    csb->size = size - sizeof( SCSH );					\
    csb->buf = Scs_to_appl( scsbp );					\
}
#define	Port_failure( reason )					  	\
    (( reason == PF_POWER ||						\
       reason == PF_PORTERROR ||					\
       reason == PF_FATALERROR ) ? 1: 0 )

/* SMP: The CBVTE must be locked PRIOR to invocation.
 */
#define	Remove_pb_waitq( cb )	{					\
    if( cb->cinfo.cbstate != CB_NOT_WAIT ) {				\
	Lock_pb( cb->pb )						\
	Remove_entry( cb->scs_cb )					\
	Unlock_pb( cb->pb )						\
	cb->cinfo.cbstate = CB_NOT_WAIT;				\
    }									\
}
/* SMP: Both the SCA database and PB must be locked PRIOR to invocation.
 */
#define	Remove_scs_timeoutq( pb ) {					\
    pb->pinfo.status.sanity = 0;					\
    pb->pinfo.duetime = 0;						\
    Remove_entry( pb->timeout )						\
}
