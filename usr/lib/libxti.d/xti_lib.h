/* "@(#)xti_lib.h	4.2 (ULTRIX)	11/14/90" */
/***********************************************************************
 *
 * Copyright (c) Digital Equipment Corporation, 1990
 * All Rights Reserved.  Unpublished - rights reserved
 * under the copyright laws of the United States.
 * 
 * The software contained on this media is proprietary
 * to and embodies the confidential technology of 
 * Digital Equipment Corporation.  Possession, use,
 * duplication or dissemination of the software and
 * media is authorized only pursuant to a valid written
 * license from Digital Equipment Corporation.
 *
 * RESTRICTED RIGHTS LEGEND   Use, duplication, or 
 * disclosure by the U.S. Government is subject to
 * restrictions as set forth in Subparagraph (c)(1)(ii)
 * of DFARS 252.227-7013, or in FAR 52.227-19, or in
 * FAR 52.227-14 ALT.III, as applicable.
 *
 ***********************************************************************/

/***********************************************************************
 *   			Modification History			       *
 *
 *  07/20/87	hu	Original code.
 *  12/02/87	mcmenemy Added additional library functionality
 *  01/19/88    mcmenemy Added connection-less support
 *  02/01/88    mcmenemy Add DECnet-ULTRIX support
 *  03/07/88    mcmenemy Update to Revision 2 (24-feb-88) at GRENOBLE
 *  06/27/88    mcmenemy Add xti_proto field to d_entry
 *  08/25/88    mcmenemy Update to Final Draft for XPG 3
 *  08/06/88    mcmenemy Get rid of unused user mode fields that are
 *			 now in kernel.
 *  02/22/89    mcmenemy Add evtinfo structure used in performance
 *                       changes
 *
 **********************************************************************/

/*
 * XTI Library: xti_lib.h
 *
 * This header file is included by xti.c and xti_lib.c modules to
 * provide the transport layer programming interface defined in the
 * X/OPEN Portability Guide: Networking Services. 
 */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef XTIOSI
#include <netosi/osi.h>
#endif
#ifdef XTINSP
#include <netdnet/dn.h>
#endif

#ifndef XTI_MAX_EVTS
#define XTI_MAX_EVTS 11  /* should match MAX_EVTS with socketvar.h */
#endif

struct xti_lookup {
  char *x_name;		/* t_provider identifier string */
  int x_af;			/* socket address format */
  int x_type;			/* socket type */
  int x_protocol;		/* protocol number for socket */
  struct t_info x_info;	/* t_open default info */
};

struct xti_evtinfo {
  int evtarray[XTI_MAX_EVTS];
};

/*
 *	The following are the states for the user
 */

#define XTI_UNINIT	0	/* unitialized */
#define XTI_UNBND       1	/* unbound */
#define XTI_IDLE	2	/* idle */
#define XTI_OUTCON	3	/* outgoing connection pending */
#define XTI_INCON    	4	/* incoming connection pending */
#define XTI_DATAXFER	5	/* data transfer */
#define XTI_OUTREL	6	/* outgoing release pending */
#define XTI_INREL      	7	/* incoming release pending */


/*
 *	The following are XTI user level events which cause state changes
 */


#define XTI_OPENED		0
#define XTI_BIND		1	
#define XTI_OPTMGMT     	2
#define XTI_UNBIND		3
#define XTI_CLOSED		4
#define XTI_SNDUDATA		5
#define XTI_RCVUDATA		6
#define XTI_RCVUDERR		7
#define XTI_CONNECT1		8
#define XTI_CONNECT2		9	
#define XTI_RCVCONNECT		10
#define XTI_LISTEN		11
#define XTI_ACCEPT1		12
#define XTI_ACCEPT2		13
#define XTI_ACCEPT3		14
#define XTI_SND			15
#define XTI_RCV			16
#define XTI_SNDDIS1		17
#define XTI_SNDDIS2		18
#define XTI_RCVDIS1		19
#define XTI_RCVDIS2		20
#define XTI_RCVDIS3		21
#define XTI_SNDREL		22
#define XTI_RCVREL		23
#define XTI_PASS_CONN		24
#define XTI_NOEVENTS		25


extern int action_1();
extern int action_2();
extern int action_3();
extern int action_3_4();

#define noaction ((unsigned int (*)()) 0)

struct d_entry {
  unsigned int sequence;  /* specific seq for this descr. */
  int qlen;	  	  /* queue length */
  int cnt_outs_con_ind;   /* count of outstanding connect indications */
  char active_flag;	  /* if 1 then entry is valid and else invalid */
  int state;		  /* state for this descriptor */
  int event;		  /* event for this descriptor */
  struct t_info info;     /* contains information retreived from t_open */
  int family;	          /* contains protocol family number */
  int xti_proto;          /* protocol number */
};

/* dcb[_NFILE]; */

struct descr_table {
  struct d_entry (*dcb)[];     /* pointer to array of d_entry struct of getdtablesize */
} d_table;	               /* descriptor table */

#define STATE_TABLE_RECORDS 43

static struct state_table {
  int state;            /* state in XTI state machine */
  int event;            /* event in XTI state machine */
  unsigned int (*ptr_to_action)(); /* point to action routine */
  int next_state;       /* next valid state */
} sean[STATE_TABLE_RECORDS] = {
  { XTI_DATAXFER, XTI_SND, noaction, XTI_DATAXFER },
  { XTI_DATAXFER, XTI_RCV, noaction, XTI_DATAXFER },
  { XTI_UNINIT,   XTI_OPENED, noaction, XTI_UNBND },
  { XTI_UNBND,    XTI_BIND,(unsigned int (*)()) action_1, XTI_IDLE },
  { XTI_UNBND,    XTI_CLOSED, noaction, XTI_UNINIT },
  { XTI_IDLE,     XTI_OPTMGMT, noaction, XTI_IDLE },
  { XTI_IDLE,     XTI_UNBIND, noaction, XTI_UNBND },
  { XTI_IDLE,     XTI_CLOSED, noaction, XTI_UNINIT },
  { XTI_IDLE,     XTI_SNDUDATA, noaction, XTI_IDLE },
  { XTI_IDLE,     XTI_RCVUDATA, noaction, XTI_IDLE },
  { XTI_IDLE,     XTI_RCVUDERR, noaction, XTI_IDLE },
  { XTI_IDLE,     XTI_CONNECT1, noaction, XTI_DATAXFER },
  { XTI_IDLE,     XTI_CONNECT2, noaction, XTI_OUTCON },
  { XTI_IDLE,     XTI_LISTEN, (unsigned int (*)()) action_2, XTI_INCON },
  { XTI_IDLE,     XTI_PASS_CONN, noaction, XTI_DATAXFER },
  { XTI_OUTCON,   XTI_RCVCONNECT, noaction, XTI_DATAXFER },
  { XTI_OUTCON,	XTI_SNDDIS1, noaction, XTI_IDLE },
  { XTI_OUTCON,	XTI_RCVDIS1, noaction, XTI_IDLE },
  { XTI_OUTCON,	XTI_CLOSED, noaction, XTI_UNINIT },
  { XTI_INCON,	XTI_LISTEN, (unsigned int (*)()) action_2, XTI_INCON },
  { XTI_INCON,	XTI_ACCEPT1, (unsigned int (*)()) action_3, XTI_DATAXFER },
  { XTI_INCON,	XTI_ACCEPT2, (unsigned int (*)()) action_3_4, XTI_IDLE },
  { XTI_INCON,	XTI_ACCEPT3, (unsigned int (*)()) action_3_4, XTI_INCON },
  { XTI_INCON,	XTI_SNDDIS1, (unsigned int (*)()) action_3, XTI_IDLE },
  { XTI_INCON,	XTI_SNDDIS2, (unsigned int (*)()) action_3, XTI_INCON },
  { XTI_INCON,	XTI_RCVDIS2, (unsigned int (*)()) action_3, XTI_IDLE },
  { XTI_INCON,	XTI_RCVDIS3, (unsigned int (*)()) action_3, XTI_INCON },
  { XTI_INCON,  XTI_CLOSED, noaction, XTI_UNINIT },
  { XTI_DATAXFER, XTI_SNDDIS1, noaction, XTI_IDLE },
  { XTI_DATAXFER, XTI_RCVDIS1, noaction, XTI_IDLE },
  { XTI_DATAXFER, XTI_SNDREL, noaction, XTI_OUTREL },
  { XTI_DATAXFER, XTI_RCVREL, noaction, XTI_INREL },
  { XTI_DATAXFER, XTI_CLOSED, noaction, XTI_UNINIT },
  { XTI_OUTREL,	XTI_RCV, noaction, XTI_OUTREL },
  { XTI_OUTREL,	XTI_SNDDIS1, noaction, XTI_IDLE },
  { XTI_OUTREL,	XTI_RCVDIS1, noaction, XTI_IDLE },
  { XTI_OUTREL,	XTI_RCVREL, noaction, XTI_IDLE },
  { XTI_OUTREL, XTI_CLOSED, noaction, XTI_UNINIT },
  { XTI_INREL,	XTI_SND, noaction, XTI_INREL },
  { XTI_INREL,	XTI_SNDDIS1, noaction, XTI_IDLE },
  { XTI_INREL,	XTI_RCVDIS1, noaction, XTI_IDLE },
  { XTI_INREL,	XTI_SNDREL, noaction, XTI_IDLE },
  { XTI_INREL,  XTI_CLOSED, noaction, XTI_UNINIT },
};

static struct xti_debug_state {
	char *name;
	} xti_state_name[8] = {
	 "T_UNINIT","T_UNBND","T_IDLE","T_OUTCON","T_INCON",
	 "T_DATAXFER","T_OUTREL","T_INREL",
	};

static struct xti_debug_event {
	char *name;
	} xti_event_name[XTI_NOEVENTS] = {
 	"opened","bind","optmgmt","unbind","closed",
	"sndudata","rcvudata","rcvuderr","connect1",
	"connect2","rcvconnect","listen",
	"accept1","accept2","accept3",
	"snd","rcv","snddis1",
	"snddis2","rcvdis1","rcvdis2",
	"rcvdis3","sndrel","rcvrel",
	"pass_conn"};



