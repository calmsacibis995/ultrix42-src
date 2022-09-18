/*
 * static  char    *sccsid = "@(#)pfiltdefs.h	4.2  (ULTRIX)	9/4/90";
 */
/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University	of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ---------------------------------------------------------------------
 * Common code for processing received Ethernet packets
 * derived from:
 *      enetdefs.h	Stanford	25 April 1983
 */

/* Notes:
 *	- check maxwaiting, etc.
 *	- Xdrops in enState may be meaningless
 */

/*
 *  Ethernet definitions NOT needed for user processes
 *
 **********************************************************************
 * HISTORY
 *
 * $Log:	pfiltdefs.h,v $
 * Revision 1.2  90/05/22  15:36:29  mogul
 * Added support for COPYALL stuff
 * 
 * Revision 1.1  90/05/18  17:31:16  mogul
 * Initial revision
 * 
 * Revision 1.2  89/07/26  19:19:43  mogul
 * Changed ENNOTSUWAITING from 8 to 32 (bigger memories nowadays)
 * Added several fields to struct enState to support EIOCALLOWPROMISC
 * 	and EIOCMAXBACKLOG
 * 
 * Revision 1.1  88/08/31  10:57:47  mogul
 * Initial revision
 * 
 * Revision 1.1  87/07/14  13:21:21  root
 * Initial revision
 * 
 * Revision 1.6  87/01/26  15:04:33  mogul
 * Added support for EIOCTRUNCATE ioctl for truncation of received packets.
 * 
 * Revision 1.5  86/09/16  17:16:15  mogul
 * Automatic inclusion of RCS log messages in history comments.
 * 
 *
 * 13 July 1986		Jeff Mogul	Stanford
 *	Superuser can set a much higher MAXWAITING (for use in network
 *		monitoring applications.)
 *	Record number of recently-dropped packets per minor device.
 *	Count unwanted packets (per interface) separately from dropped
 *		packets, and packets missed by interface because of overflow.
 *	Added Batch and TimeStamp support, with a little help
 *		from Greg Satz.
 * 7 October 1985	Jeff Mogul	Stanford
 *	Removed ENMAXOPENS limitation; available minors are now
 *	dynamically allocated to interfaces.
 *	Certain arrays in the enState structure are now indirected
 *	via pointers.
 * 17 October 1984	Jeff Mogul	Stanford
 *	Added RecvCount filed to enOpenDescriptor struct to store count
 *	of packets received on this filter.
 *	Also, made some changes to encourage use of "high-priority"
 *	devices:
 *	- changed ENHIPRIDEV from 12 to 0; allows anyone to use
 *		high priority
 *	- changed ENHIPRI from 128 to 2; no point in having more than
 *		one priority below ENHIPRI, since all would be equivalent.
 *
 * 2 October 1984	Jeff Mogul	Stanford
 *	Added field to enOpenDescriptor struct to store pre-computed
 *	address of short word AFTER last valid word in the filter;
 *	this saves a few instructions in some very busy code.
 *
 * 10 November 1983	Jeffrey Mogul	Stanford
 *	Minor changes to support EIOCDEVP ioctl,
 *	removed references to enUnit[]
 *
 * 25-Apr-83	Jeffrey Mogul	Stanford
 *	Began conversion to 4.2BSD.  This involves removing all
 *	references to the actual hardware.
 *	Most previous history comments removed.
 *	Split off from definitions of interest to users.
 *
 * 10-Aug-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added EIOCMBIS and EIOCMBIC definitions, and new ENHOLDSIG mode
 *	bit and ENPRIVMODES defintions (V3.05e). [Last change before
 *	4.2BSD conversion starts.]
 *
 * 22-Feb-80  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Rewritten for multiple simultaneous opens with filters (V1.05).
 *
 * 18-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *      Created (V1.00).
 *
 **********************************************************************
 */

/* tunable constants */
#define	ENHIPRI		2		/* high priority priority */
			/*
			 * setting ENHIPRI to 2 makes sense because all
			 * priorities below ENHIPRI are equivalent; therefore,
			 * we only need one of them.
			 */
#define	ENMAXPRI	255		/* maximum high priority priority */

#define	ENUNIT(dev)	(enUnitMap[minor(dev)]) /* macro */
					/* to extract ethernet unit number */
					/* from device number (used as the */
					/* index into the enState[] table); */
#define	ENINDEX(dev)	(minor(dev)) /* macro to extract logical */
					/* device index from device number */
					/* (used as the index into the */
					/* AllDescriptors table for the */
					/* unit) */

#define ENMAXWAITING	255		/* maximum number of packets */
					/* which can be queued at one time */
					/* for an open enet file (max 2^15) */
#define	ENNOTSUWAITING	32		/* Ditto if requested by non-suser */
#define ENDEFWAITING	2		/* default number of packets */
					/* which can be queued at one */
					/* time for an open enet file */
#define ENPACKETS	256		/* number of preallocated packets */
					/* available for receiving (these
					/* should be allocated dynamically */
					/* once the system makes */
					/* this more feasible) */

#define ENMINSCAVENGE	4		/* minimum number of packets */
					/* guaranteed to be freed by */
					/* scavenging */

/*
 * Privileged mode bits
 */
#define	ENKERNEL	(0x8000)	/* flag bit to indicate kernel mode */
					/*  requests */

/*
 *  The following definitions are meant to clean up
 *  C language syntax a bit.
 */

typedef	enum {false, true}	boolean;
#define	TRUE	((int)true)
#define	FALSE	((int)false)

#define	ENMAXINT	(0x7FFFFFFF)	/* This is machine-dependent! */

#define endcase break;

/*
 * General purpose structure to define circular queues.
 *  Both the queue header and the queue elements have this
 *  structure.
 */

struct Queue
{
    struct Queue * F;
    struct Queue * B;
};

#ifdef notdef	/* these are no longer used */
/* Temporary used by queue macros. */
struct Queue * q_tmp;

#define enqueue(head,elt) \
{ (elt)->F = (head);(elt)->B = (head)->B;(head)->B = (elt);((elt)->B)->F = (elt); };

#define remqueue(head,elt) \
    { ((elt)->B)->F = (elt)->F; ((elt)->F)->B = (elt)->B; };

#define dequeue(head) 			  \
    (((q_tmp = (head)->F) == (head)) ? NULL : \
     (((q_tmp->B)->F = q_tmp->F),         \
      ((q_tmp->F)->B = q_tmp->B),	  \
      (q_tmp)))

#define queueempty(head) \
    ((head)->F == (head))

#define initqueue(head)  \
    { (head)->F = (head); (head)->B = (head); };
#endif /* notdef */

/*
 *  The ethernet packet structure.
 *
 *  NOTE!!: the padding code in Pfiltrmove() assumes that there are at
 *		least (ENALIGNMENT - 1) bytes of readable address space
 *		before enP_Stamp; this is likely to be true, but to be
 *		sure we put enP_Stamp last in this struct.
 */

struct enPacket
{
    struct Queue   enP_Link;		/* queue pointers */
    u_short	   enP_RefCount;	/* # of outstanding references to */
					/* this packet */
    struct mbuf	  *enP_mbuf;		/* first mbuf of packet */
    struct enstamp enP_Stamp;		/* various per-packet stats */
};
#define	enP_F	enP_Link.F
#define	enP_B	enP_Link.B
#define	enP_ByteCount enP_Stamp.ens_count

/*
 *  Ethernet queue header
 */
struct enQueue
{
    struct Queue enQ_Head;	/* queue header and trailer pointers */
    short	 enQ_NumQueued;	/* number of elements in queue */
};
#define	enQ_F	enQ_Head.F
#define	enQ_B	enQ_Head.B

/*
 *  Wait queue header
 */
struct enWaitQueue
{
    struct enPacket *enWQ_Packets[ENMAXWAITING];/* pointers to queued packets */
    short	     enWQ_Head;		/* index into Packets (for dequeue) */
    short	     enWQ_Tail;		/* index into Packets (for enqueue) */
    short	     enWQ_NumQueued;	/* current queue size */
    short	     enWQ_MaxWaiting;	/* threshold for additions */
};
#define	PfiltNextWaitQueueIndex(idx)					\
	if (++(idx) >= ENMAXWAITING) (idx) = 0
#define	PfiltPrevWaitQueueIndex(idx)					\
	if (--(idx) < 0) (idx) = (ENMAXWAITING-1)

/*
 *  States of receive side of open enet file.
 */
enum enStates {ENRECVIDLE, ENRECVTIMING, ENRECVTIMEDOUT};

struct enOpenDescriptor
{
    struct Queue       enOD_Link;	/* Linked list of OpenDescriptors */
    struct enWaitQueue enOD_Waiting;	/* fixed Queue of waiting packets */
    struct proc       *enOD_SigProc;	/* Process to signal (user mode) */
    short	       enOD_SigPid;	/* Process ID of process to signal */
    short    	       enOD_SigNumb;	/* Signal number for input packet */
					/* notification  */
    long               enOD_Timeout;	/* Length of time to wait for packet */
    struct enfilter    enOD_OpenFilter;	/* Packet filter */
    enum enStates      enOD_RecvState;	/* see enStates enumeration (above) */
    short	       enOD_Flag;	/* option bits */
    
    /* following are for enetselect() */
    short	       enOD_SelColl;	/* true if selects collide */
    struct proc       *enOD_SelProc;	/* process that last selected us */
    
    unsigned long      enOD_RecvCount;	/* number of packets received */
    int		       enOD_Truncation;	/* max. bytes to be returned */

    /* store precomputed address of end of filter */
    unsigned short    *enOD_FiltEnd;	/* addr of short AFTER end of filt */
    unsigned short     enOD_Drops;	/* # of recent drops for this filt */
};

/*
 *  State descriptor for each enet device
 */
struct enState
{
    long	ens_Rcnt;		/* input packets queued since */
					/* system startup */
    long	ens_Xcnt;		/* output packets sent since */
					/* system startup */
    long	ens_Rdrops;		/* input packets dropped since */
					/* system startup (queue overflows) */
    long	ens_Xdrops;		/* output packets dropped since */
					/* system startup */
    long	ens_Rmissed;		/* packets missed by interface */
    long	ens_Runwanted;		/* input packets not wanted by any */
					/* filter since system startup */
    struct enQueue			/* queue of active open */
		ens_Desq;		/* file descriptors */
    struct endevp	ens_DevParams;	/* device parameters, see enet.h */
    boolean	ens_AllowPromisc;	/* T => set IFF_PROMISC when needed */
    long	ens_PromiscCount;	/* count of times IFF_PROMISC needed */
    long	ens_UserMaxWaiting;	/* MAXWAITING for non-root users */
    boolean	ens_AllowCopyAll;	/* T => set IFF_PFCOPYALL if needed */
    long	ens_CopyAllCount;	/* # of times IFF_PFCOPYALL needed */
};
#define	enRcnt		(enStatep->ens_Rcnt)
#define	enXcnt		(enStatep->ens_Xcnt)
#define	enRdrops	(enStatep->ens_Rdrops)
#define	enXdrops	(enStatep->ens_Xdrops)
#define	enRmissed	(enStatep->ens_Rmissed)
#define	enRunwanted	(enStatep->ens_Runwanted)
#define	enDesq		(enStatep->ens_Desq)
#define	enCurOpens	(enStatep->ens_Desq.enQ_NumQueued)
#define	enDevParams	(enStatep->ens_DevParams)
