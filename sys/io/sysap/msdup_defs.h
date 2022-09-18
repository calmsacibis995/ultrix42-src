/*	@(#)msdup_defs.h	4.2	(ULTRIX)	2/21/91	*/

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
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
 ************************************************************************/

/*
 *
 *   Facility:	Systems Communication Architecture
 *		Diagnostic/Utilities Protocol (DUP) Class Driver
 *
 *   Abstract:	This module contains data type, macro, constant, structure,
 *		and message format definitions specific to DUP support.
 *
 *   Author:	Andrew J. Moskal	Creation Date:	dd-mmn-yyyy
 *
 *   Modification History:
 *
 *   $Log:	msdup_defs.h,v $
 *   Revision 1.2  90/05/24  13:41:17  prakash
 *   Added a field to the MSDUP_CONNB structure to keep track of the current
 *   protocol state of the DUP server, and another to mark if the state
 *   is about to change.
 *   
 *
 */

/*
 *   Conditional check to include this header file only once.
 */
#ifndef	MSDUP_defs.h
#define	MSDUP_defs.h	0		/* Disable subsequent includes */

/*
 *  Include prerequisite definitions
 */
#ifdef	KERNEL
#include "../io/sysap/msdup_pgm.h"	/* DUP program file/header defs      */
#include "../io/sysap/msdup_ioctl.h"	/* DUP control function definitions  */
#include "../io/sysap/msdup_msg.h"	/* DUP command/response message defs */
#include "../io/sysap/msdup_user.h"
#include "../io/sysap/msdup_errs.h"
/*#include "../io/sysap/msdup_error.h" */
#include "../io/sysap/msdup_proto.h"
#else	KERNEL
#include <io/sysap/msdup_pgm.h>		/* DUP program file/header defs      */
#include <io/sysap/msdup_ioctl.h>	/* DUP control function definitions  */
#include <io/sysap/msdup_msg.h>		/* DUP command/response message defs */
#include <io/sysap/msdup_user.h>
#include <io/sysap/msdup_errs.h>
/*#include <io/sysap/msdup_error.h> */
#include <io/sysap/msdup_proto.h>
#endif	KERNEL

/*
 *   Local Definitions.
 */

/*
 * Version Info.
 */
#define	DrvrVersion	"V 1.0"


/*
 *  Configuration parameters.
 */
#define	MSDUP_CLDRV_NAME	"U32_DUP_CL_DRVR "
#define	MSDUP_CLDRV_DATA	"                "


#define	MSDUP_NUNIT	16		/* Units per system.
					   Defined here and in msdup_defs.h. 
					   Change BOTH together.	*/
#define	MSDUP_NCONN	1		/* Simultaneous connections per unit */
#define	MSDUP_NREQ	8		/* The number of simultaneously 
					   active requests per connection -
					   1 for babble, 1 for a DUP command, 
					   1 for an abort command, and 1 timeout
					   get dust each for the two commands, plus 3 extra
					   for end messages of received SDI messages.*/
#define MSDUP_MAX_SLEEPERS  MSDUP_NREQ
#define MSDUP_MAX_TIMERS    MSDUP_NREQ
#define MSDUP_PNDG_SDI_MSGS 100		/* Maximum number of unprocessed SDI
					   messages that can be received 
					   without blocking.                  */
#define	MSDUP_PNDG_SDI_CMDS 100		/* maximum number of SDI commands that 
					   can be pending at one time before 
					   we decide to overflow and exit. */
#define MSDUP_CONNB_NO_IO_PND	0
#define MSDUP_CONNB_INPUT_PND	1
#define MSDUP_CONNB_OUTPUT_PND	2


/*
 *   Macros.
 */

/*
 *   DUP_index returns the logical unit index given the device number.
 */
#define MSDUP_index( Dev ) ( minor( (Dev) ) )

/*
 *   Dev_to_MSDUP_unit returns the unit control block pointer corresponding to
 *   the given device number.
 */
#define Dev_to_MSDUP_unit ( Dev ) \
    (( MSDUP_index( (Dev) ) >= MSDUP_NUNIT ) \
	? ( MSDUP_UNITB * ) NULL : msdup_unit_tbl[ MSDUP_index( (Dev) ) ] )

/*
 *  Init_MSDUP_msg clears the message buffer.
 */
#define Init_MSDUP_msg(Msg_ptr) (void)bzero((caddr_t)(Msg_ptr), sizeof(MSDUP_MSG))

/*
 * Avoid DUP IODONE panic. This can happen since there are multiple threads 
 * that might want to terminate the connection (on an error, for example). 
 * Under suitable circumstances, more than one thread might try to call
 * iodone, which causes it to panic. The follwoing macro avoids this problem
 * by ensuring that it does not get called more than once on the same buffer.
 */
#define	DUP_iodone(buf_ptr) {\
			      int ipl = Splscs();\
			      if (!((buf_ptr)->b_flags & B_DONE)) {\
			        (void)iodone((buf_ptr));\
			      }\
			      (void)splx(ipl);\
			      };


typedef struct _msdup_sdimsg {
  short                completed;
  short		       byte_cnt, offset;
  struct _msdup_sdimsg	*next;
  char                 msg[MAX_SDI_SIZE];
} MSDUP_SDI_MSG;

typedef struct {
  char			*aux;
  MSDUP_RSPID		cmd_ref;
} MSDUP_SDI_EM_INFO;

typedef struct _msdup_sleeper {
  short busy;
  int   status;
  caddr_t channel;
} MSDUP_SLEEPER;

typedef struct _msdup_timer {
  short busy;
  caddr_t arg;
  void (*rtn)();
  struct _msdup_connb *connb;
} MSDUP_TIMER;

typedef struct _msdup_waker {
  struct _msdup_connb *connb;
  caddr_t channel;
  int status;
} MSDUP_WAKER;

/*
 *  Per-request data structure
 */
typedef struct	_msdup_reqb {
    MSDUP_RSPID		    rspid;	/* Response ID                       */
    struct {
	u_short	perm	 	:  1;	/*  Request block is permanent	     */
	u_short	no_stall	:  1;	/*  Do not stall/queue request       */
	u_short	chk_progress	:  1;	/*  Check progress during timeouts   */
	u_short gotit           :  1;   /*  Set if end message received.     */
	u_short tmo_getdust     :  1;   /*  Set if this is a GET DUST msg.   */
	u_short firstGD         :  1;   /*  Set if this is the first GET DUST
					    from a command timeout.          */
	u_short	dustDone	:  1;   /*  Used by getdust.		     */
	u_short	error		:  1;	/*  Error indicator                  */
	u_short			:  8;	/*  Reserved                         */
    }			    flags;	/* Request block software flags	     */
    void		    (*rcv_rtn)();
      					/* Routine to be called when the end
					   message is received.              */
    void		    *rcv_rtn_arg;
    					/* Argument to be supplied to rcv_rtn*/
    struct _msdup_connb	    *connb;	/* Connection block back pointer     */
    struct _msdup_unitb	    *unitb;	/* Unit block back pointer	     */
    struct _msdup_classb    *classb;	/* Class block back pointer	     */
    u_long		    tmo_cnt;	/* Response timeout count (hz)       */
    caddr_t		    tmo_arg;	/* Argument passed to the tmo handler*/
    struct _msdup_reqb	    *tmo_reqb_ptr;
				        /* REQB for timeout command. 	     */
    u_long		    progress;   /* Progress Indicator 		     */
    struct buf		    *buf;	/* Buf structure pointer	     */
    char		    op_code : 7;
    char		    em      : 1;
    char			    : 0;/* Alignment.			     */
    MSDUP_MSG		    msgbuf;	/* End message received from the 
					   server.			     */
    void		    *msgbuf_ptr;/* Pointer to message to send to the 
					   server.			     */
    u_long		    msgsize;	/* Message size			     */
    u_long		    resid_byt_cnt;
					/* Residual data byte count          */
    u_char		    *resid_dat;	/* Residual data address             */
    MSDUP_DUST		    dust;	/* Dust status			     */
    int			    error;	/* Operation status                  */
    BHANDLE		    lbhandle;	/* Local buffer handle               */
} MSDUP_REQB;				/* DUP class driver request block    */

/*
 *  Per-connection data structure
 */
typedef struct  _msdup_connb {
    CONNID		    connid;	/* Connection ID                     */
    struct {
      u_short	    signal	:  1;	/*  Signal application on error.     */
	u_short	    error	:  1;	/*  If true, an error condition 
					    exists on this connection.	     */
	u_short	    babble_ena	:  1;	/*  Babbling is enabled              */
	u_short	    abort_pnd	:  1;	/*  Abort pgm in progress (pending)  */
	u_short	    io_pnd	:  2;	/*  I/O operation in progess. Value 
					    indicates whether input or output*/
	u_short	    disc_rcv	:  1;   /*  Disconnect received              */
	u_short	    disc_pnd	:  1;	/*  Disconnect in progress (pending) */
	u_short	    path_fail	:  1;	/*  Path failure                     */
	u_short	    vc_error	:  1;	/*  Virtual circuit error            */
	u_short			:  6;	/*  Reserved                         */
    }			    flags;	/* Connection block software flags   */
    MSDUP_ERROR		    error;	/* Last error on this connection.    */
    int			    signal;	/* Signal to send to process.	     */
    struct _msdup_unitb	    *unitb;	/* Unit block back pointer           */
    struct _msdup_classb    *classb;	/* Class block back pointer          */
    c_scaaddr		    sysid;	/* System ID                         */
    u_short			: 16;	/*  Alignment                        */
    c_scaaddr		    rport_addr;	/* Remote port address               */
    u_short			: 16;	/*  Alignment                        */
    u_long		    lport_name;	/* Local port name	             */
    u_char		    rproc_name[NAME_SIZE];
					/* Remote SYSAP name (blank filled)  */
    MSDUP_DUST		    dust;	/* Server status information         */
    u_char		    pgm_name[PGM_NAME_SIZE];
					/* Program name (blank filled)      */
    MSDUP_PGMFLG	    pgm_chr_flgs;
					/* Program characteristics flags     */
    short		    conn_status;/* Status of connection attempt  */
    MSDUP_STATES	    state;	/* The current state of the DUP 
					   server on this connection.        */
    int			    inTransit;  /* Non-zero means a message that would
					   cause state-change has been 
					   initiated.                        */
    MSDUP_STATES	    nextState;  /* If inTransit is non-zero, this is 
					   the state the server would be in if
					   the current state-changing event
					   completes successfully.	     */
    void		    *aux;       /* Auxiliary structure pointer.      */
    int			    newSDI;     /* Set if the next SDI block will
					   begin a new message.              */
    u_short		    exelcl_tmo;	/* EXECUTE LOCAL PROGAM cmd timeout  */
    u_short		    abort_tmo;	/* ABORT PROGRAM cmd timeout         */
    u_short		    data_tmo;	/* SEND/RECEIVE, SDI cmd timeout     */
    u_short		    babble_interval;
					/* Babble timer interval             */
    short		    rsrcTmo;	/* How long to wait for a resource.  */
    MSDUP_SLEEPER	    sleepers[MSDUP_MAX_SLEEPERS];
    MSDUP_TIMER		    timers[MSDUP_MAX_TIMERS];
    MSDUP_REQB		    reqb[MSDUP_NREQ];
				        /* Permanent request blocks used by
					   babble and command timeouts, and 
					   by the SDI end messages.          */
    int 		    reqbsInUse; /* Bit vector indicating which request
					   blocks above are in use.          */
    short		    active[MSDUP_NREQ];
    					/* active[i] set means reqb[i] is
					   active [message sent, but end 
					   message no in yet].                */
    short 		    credit_wait[MSDUP_NREQ];
				        /* credit_wait[i] set means reqb[i]
					   is waiting on SCS credits.        */
    short		    buffer_wait[MSDUP_NREQ];
    					/* buffer_wait[i] set means reqb[i]
					   is waiting for a buffer to become 
					   available. 			     */
    short		    map_wait[MSDUP_NREQ];
    					/* map_wait[i] set means reqb[i] is
					   waiting for SCS mapping resources
					   to become available. 	     */
    MSDUP_SDI_MSG	    *first_SDI_buf, *last_SDI_buf;
#if undefined
    MSDUP_SDI_CMD	    in_msg;	/* Incoming SDI message.	     */
    MSDUP_SDI_MSG	    sdi_bufs[MSDUP_PNDG_SDI_MSGS];
    					/* SDI Buffers for incoming SDI 
					   messages. 			     */
    short		    sdi_freelist[MSDUP_PNDG_SDI_MSGS];
    					/* sdi_freelist[i] set means sdi_bufs[i]
					   is in use. */
    short		    sdi_inlist[MSDUP_PNDG_SDI_MSGS];
    					/* sdi_inlist[i] set means sdi_bufs[i]
					   holds an as yet unread message.   */
    MSDUP_SDI_MSG	    *cur_sdi_msg;
					/* Buffer receiving current SDI msg. */
    MSDUP_SDI_CMD	    pndg_sdi_cmds[MSDUP_PNDG_SDI_CMDS];
    					/* Queue of incoming SDI commands. */
#endif
    struct buf		    *buf_ptr;	/* If IO in progress, pointer to the
					   corresponding buffer.	     */
    struct buf		    io1_buf;	/* buf structure for raw io          */
    struct buf		    io2_buf;	/* buf structure for raw io          */
    struct buf		    ini_buf;	/* Initial segment buf structure     */
    struct buf		    ovr_buf;	/* Overlay segment buf structure     */
    BHANDLE		    ini_lbhandle;
					/* Initial segment buffer descriptor */
    BHANDLE		    ovr_lbhandle;
					/* Overlay segment buffer descriptor */
} MSDUP_CONNB;				/* DUP connection data block         */

/*
 *  Per-unit data structure
 */
typedef struct	_msdup_unitb {
    u_short		    unit;	/* Device number		     */
#ifdef 0
    struct {
	u_short	    online	:  1;	/*  SCS connection active -	     */
					/*    Online to DUP server           */
	u_short	    online_ip	:  1;	/*  SCS connection request initiated */
	u_short	    disc_pnd	:  1;	/*  SCS disconnect pending           */
	u_short	    disable	:  1;	/*  Disable further activity         */
					/*   CLOSE pending                   */
	u_short			: 12;	/*  Reserved                         */
    }			    flags;	/* Software unit flags               */
#endif
    struct _msdup_classb    *classb;	/* Class block back pointer          */
    struct proc		    *proc_ptr;  /* Pointer to the process structure  */
    u_short		    pid;	/* Process ID                        */
    u_short		    pgrp;	/* Group ID                          */
    MSDUP_CONNB		    connb;	/* Connection block                  */
} MSDUP_UNITB;				/* DUP unit data block               */

/*
 *  Driver-wide data structure
 */
typedef struct  _msdup_classb {
    char		    *dev_name;	/* ULTRIX device name string pointer */
    struct {
	u_short		init_done :  1;	/*  Initialization complete          */
	u_short		init_ip   :  1;	/*  Initialization in progress       */
	u_short	 		  : 14;	/*  Reserved (unused)                */
    }			    flags;	/* Driver software flags             */
    u_short		    unit_cnt;	/* Count of active units             */
    MSDUP_UNITB		    **unitb_tbl;/* Unit block hash table pointer     */
    CMSB		    cmsb;	/* Connection mgmt services block    */
} MSDUP_CLASSB;				/* DUP class driver data block       */

#endif	MSDUP_defs.
