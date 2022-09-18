#ifndef lint
static  char    *sccsid = "@(#)pfilt.c	4.5  (ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988-1991 by			*
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
 * Common code for processing received ethernet packets

/*********************************************************************
 *  Ethernet packet filter layer,
 *  	formerly: Ethernet interface driver
 *
 *		- pfilt.c -
 **********************************************************************
 * HISTORY
 *
 * 11-Apr-91 jsd
 *	Fix several SMP lock/unlock holes; Do an untimeout when closing down
 *	Add lock owner checking when smp_debug is enabled
 *
 * 24-Feb-91 jsd
 *	Add COPYALL support for loopback packets
 *
 * 02-Jan-91 jsd/matt
 *	Move PROMSIC logic to pfilt_filter from net_read() and clean up.
 *
 * $Log:	pfilt.c,v $
 * Revision 1.3  91/02/07  17:31:24  mogul
 * "generic" device names have exactly one digit
 * Fixed bug involving multi-interface systems where pf0 is down.
 * 
 * Revision 1.2  91/01/28  15:20:37  mogul
 * CopyAllCount bug fix
 * 
 * Revision 1.1  91/01/28  13:06:15  mogul
 * Initial revision
 * 
 * Revision 1.3  90/06/05  13:20:00  mogul
 * Dynamic allocation of descriptors
 * 
 * Revision 1.2  90/05/22  15:36:51  mogul
 * Added support for EIOCALLOWCOPYALL ioctl and ENCOPYALL mode bit.
 * 
 * Revision 1.1  90/05/18  17:31:09  mogul
 * Initial revision
 * 
 * Revision 1.5  89/08/09  11:05:02  mogul
 * Conditionalized method used in enetopen on GNODE_CLONING
 * 
 * Revision 1.4  89/07/31  18:03:49  mogul
 * Moved definition of generic prefix from enet.c to enet.h
 * for use by outside programs.
 * 
 * Revision 1.3  89/07/31  17:58:38  mogul
 * Made EIOCALLOWPROMISC and EIOCMAXBACKLOG in-out; if the value
 * of the argument is negative, then return the current setting
 * instead of changing it.
 * 
 * EIOCSETIF accepts pseudo-names of the form pf0, pf1, etc., to
 * generically refer to the nth packet filter interface.
 * 
 * Revision 1.2  89/07/26  19:16:09  mogul
 * Added several new ioctls to make this easier to use:
 * 	EIOCALLOWPROMISC
 * 		Controls whether IFF_PROMISC is set/cleared automatically
 * 	EIOCMAXBACKLOG
 * 		Sets non-super-user input queue length limit
 * 	EIOCSRTIMEOUT
 * 	EIOCGRTIMEOUT
 * 		Set/Get read timeout in units of "struct timeval"
 * 	EIOCSETIF
 * 		Rebind minor device to a different interface
 * 
 * 09-Mar-89 Uttam S.N.
 * Combined pfilt.c and pfiltqueue.c into one file - pfilt.c
 * Rename enqueue, dequeue and initqueue routines to
 * pfiltenqueue, pfiltdequeue, pfiltinitqueue respectively.
 * These queueing routines were all converted to macros.
 *
 * 02-Mar-89 Uttam Shikarpur Nadig; Nashua, NH.
 * Changed names of en* and enet* subroutines to pfilt_*.
 *
 * Revision 1.4  88/11/23  18:05:15  mogul
 * Allocate per-unit structures dynamically rather than at compile
 * time.
 * 
 * Revision 1.3  88/11/18  13:49:40  mogul
 * Added pfilt_newaddress(), for use when an interface address
 * changes in midstream.
 * 
 * Revision 1.2  88/10/03  13:40:02  mogul
 * Port to Ultrix 3.0
 * 
 * Revision 1.19  87/07/06  12:54:03  chen
 * Updated for 2.0 Ultrix.  Changed call to MCLGET() for new macro,
 * also changed some #includes for rellocation of sources from sys/vaxif
 * to sys/net
 * 
 * Revision 1.18  87/04/09  15:22:44  mogul
 * Returns unwanted packets to caller instead of freeing them.  This
 * is so we can run DECnet along with the packet filter.
 * 
 * Revision 1.17  87/01/26  15:05:06  mogul
 * Added support for EIOCTRUNCATE ioctl for truncation of received packets.
 * 
 * Revision 1.16  86/12/08  13:44:27  mogul
 * This now cannot be compiled for a system without the "SUN"
 * openi code.
 * 
 * Revision 1.15  86/12/08  13:01:08  mogul
 * Self-profiling code.
 * 
 * Revision 1.14  86/09/30  13:21:52  mogul
 * Minor change to extent-map creation code (probably saves a few cycles.)
 * 
 * Revision 1.13  86/09/18  16:53:12  mogul
 * Don't transfer a partial stamp if there isn't room in the
 * user's buffer for a whole stamp.
 * Don't truncate a packet that won't fit unless it's the first
 * packet in a batch.
 * 
 * Revision 1.12  86/09/16  17:23:34  mogul
 * Automatic inclusion of RCS log messages in history comments.
 * 
 *
 * 13 July 1986		Jeff Mogul	Stanford
 *	Added Batch and TimeStamp support, with a little help
 *		from Greg Satz.
 *	Guarantee proper alignment between packets of a multi-packet batched
 *		read.
 *	Clears per-filter Drop count when input queue is flushed.
 *	Complain about bad lengths in pfiltrmove if uio_resid is > 0.
 *	Added ENNONEXCL mode.
 *	Added several new literal-push actions (not sure if these help).
 *	When scavenger removes packets from an input queue, remember
 *		how many previously dropped packets were recorded by the newly
 *		dropped ones.
 *	Added EIOCIFNAME to return interface name.
 *	Scavenger cleanup: fixed serious bug by making enScavLevel array
 *	of shorts. Init it only once.  Count packets dropped by scavenger.
 *	Rewrote pfilt_read batch delivery loop; scavenger attack after partial
 *		read might have blocked reader.
 *	Count interface overflows; records unwanted packets separately from
 *		drops.
 *	Allow super-user to set a much higher MAXWAITING.
 *	Mark packets according to how they were received (e.g.,
 *		promiscuous or not.)
 *	Eliminated the call to m_pullup for each received packet.  We now
 *		build a little map indicating which mbuf holds each byte in the
 *		packet; this should run a lot faster.  I haven't yet figured
 *		out what to do about filter operations on words that span two
 *		mbufs; in the current code, this "shouldn't happen" for
 *		reasonable filters.
 *	Counts # of packets dropped because a filter had too many
 *		packets queued.
 *	If filter length is zero, don't execute filter because we know the
 *		answer.
 *	Added NOMEM debug flag.
 *
 * 7 October 1985	Jeff Mogul	Stanford
 *	Removed ENMAXOPENS limitation; available minors are now
 *	dynamically allocated to interfaces, out of pool of NENETFILTER
 *	minors.
 *	Certain arrays formerly in the enState structure are now global.
 *	Depends on modified openi() function so that pfilt_open() need
 *	only be called once.
 *	Remove support for "kernel access", it won't ever be used again.
 *	Added EIOCMFREE ioctl.
 *
 * 17 October 1984	Jeff Mogul	Stanford
 *    More performance improvements:
 *	Added ENF_CAND, ENF_COR, ENF_CNAND, and ENF_CNOR, short-circuit
 *	operators, to make filters run faster.
 *	All evaluate "(*sp++ == *sp++)":
 *	ENF_CAND: returns false immediately if result is false, otherwise
 *		continue
 *	ENF_COR: returns true immediately if result is true, otherwise
 *		continue
 *	ENF_CNAND: returns true immediately if result is false, otherwise
 *		continue
 *	ENF_CNOR: returns false immediately if result is true, otherwise
 *		continue
 *	Also added ENF_NEQ to complement ENF_EQ
 *    - Maintain count of received packets per filter, dynamically
 *	re-organize filter queue to keep highly active filters in
 *	front of queue (but maintaining priority order), if they are
 *	"high priority" filters.
 *
 * 2 October 1984	Jeff Mogul	Stanford
 *	Made a few changes to pfiltdofilter() to speed it up, since profiling
 *	shows it to be rather popular:
 *	- precompute maximum word in packet and address of end of
 *	filters (thereby moving this code out of "inner loop").
 *	- minor re-arrangement to avoid re-evaluating a
 *	common subexpression.
 *	- changed #ifdef DEBUG in a few routines to #ifdef INNERDEBUG,
 *	so that code in inner loops isn't always testing the enDebug
 *	flag; this not only costs directly, but also breaks up some
 *	basic blocks that the optimizer could play with.
 *	- added enOneCopy flag; if true, then never deliver more than
 *	one copy of a packet.  This is equivalent to giving everyone
 *	a "high priority" device, and cuts down the number of superfluous
 *	calls to pfiltdofilter(). [Temporary hack, will remove this!]
 *
 * 24 August 1984	Jeff Mogul	Stanford
 *	YA bug with sleeping in pfilt_write(); straightened out handling
 *	of counts in pfiltkludgesleep so that they indicate the number
 *	of sleeps in progress. Maybe I've got this right, now?
 *	Also, don't sleep forever (since the net might be down).
 *
 * 17 July 1984	Jeff Mogul	Stanford
 *	Bug fix: in Pfilt_write(), several problems with sleeping on
 *	IF_QFULL:
 *	- don't do it for kernel mode writes.
 *	- count # of procs sleeping, to avoid lost wakeups.  Old
 *		scheme would only wake up the first sleeper.
 *	- using sleeper-count, avoid using more than one timeout
 *		table entry per device; old scheme caused timeout table panics
 *	- trap interrupted sleeps using setjmp, so that we can deallocate
 *		packet header and mbufs; otherwise we lost them and panicked.
 *
 * 5 July 1984	Jeff Mogul	Stanford
 *	Bug fix: in Pfilt_write() make sure enP_RefCount is zero before
 *	deallocating "packet".  Otherwise, "packets" get lost, and
 *	take mbufs (and ultimately, the system) with them.
 *
 * 8 December 1983	Jeffrey Mogul	Stanford
 *	Fixed bug in Pfilt_write() that eventually caused	allocator
 *	to run out of packets and panic.  If Pfilt_write() returns
 *	an error it should first deallocate any packets it has allocated.
 *
 * 10 November 1983	Jeffrey Mogul	Stanford
 *	Slight restructuring for support of 10mb ethers;
 *	- added the EIOCDEVP ioctl
 *	- removed the EIOCMTU ioctl (subsumed by EIOCDEVP)
 *	This requires an additional parameter to the pfilt_attach
 *	call so that the device driver can specify things.
 *
 *	Also, cleaned up the enDebug scheme by adding symbolic
 *	definitions for the bits.
 *
 * 25-Apr-83	Jeffrey Mogul	Stanford
 *	Began conversion to 4.2BSD.  This involves removing all
 *		references to the actual hardware.
 *	Changed read/write interface to use uio scheme.
 *	Changed ioctl interface to "new style"; this places a hard
 *		limit on the size of a filter (about 128 bytes).
 *	"Packets" now point to mbufs, not private buffers.
 *	Filter can only access data in first mbuf (about 50 words worst case);
 *		this is long enough for all Pup purposes.
 *	Added EIOCMTU ioctl to get MTU (max packet size).
 *	Added an Pfilt_select() routine and other select() support.
 *	Other stuff is (more or less) left intact.
 *	Most previous history comments removed.
 *	Changed some names from enXXXX to enetXXXX to avoid confusion(?)
 *
 * 10-Aug-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added new EIOCMBIS and EIOCMBIC ioctl calls to set and clear
 *	bits in mode word;  added mode bit ENHOLDSIG which suppresses
 *	the resetting of an enabled signal after it is sent (to be
 *	used in conjunction with the SIGHOLD mechanism);  changed
 *	EIOCGETP to zero pad word for future compatibility;  changed enwrite()
 *	to enforce correct source host address on output packets (V3.05e).
 *	(Stanford already uses long timeout value and has no pad word - JCM)
 *	[Last change before 4.2BSD conversion starts.]
 *
 * 01-Dec-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug in timeout handling caused by missing "break" in the
 *	"switch" state check within Pfilt_read().  This caused all reads
 *	to be preceeded by a bogus timeout.  In addition, fixed another
 *	bug in signal processing by also recording process ID of
 *	process to signal when an input packet is available.  This is
 *	necessary because it is possible for a process with an enabled
 *	signal to fork and exit with no guarantee that the child will
 *	reenable the signal.  Thus under appropriately bizarre race
 *	conditions, an incoming packet to the child can cause a signal
 *	to be sent to the unsuspecting process which inherited the
 *	process slot of the parent.  Of course, if the PID's wrap around
 *	AND the inheriting process has the same PID, well ... (V3.03d).
 *
 * 22-Feb-80  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Rewritten to provide multiple user access via user settable
 *	filters (V1.05).
 *
 * 18-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *      Created (V1.00).
 *
 **********************************************************************
 */

#define	MAXUNITS	16	/* Maximum number of interfaces supported */

#include "packetfilter.h"

#if (NPACKETFILTER > 0)

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/tty.h"
#include "../h/uio.h"
#include "../h/kmalloc.h"

#include "../h/protosw.h"
#include "../h/socket.h"
#include "../net/net/if.h"
#include "../net/netinet/in.h"
#include "../net/netinet/if_ether.h"
#include "../net/net/ether_driver.h"
#include "../h/types.h"
#include "../h/time.h"

#undef	queue
#undef	dequeue

#include "../net/net/pfilt.h" 
#include "../net/net/pfiltdefs.h"
#include "../h/smp_lock.h"

#undef	NPACKETFILTER
#define	NPACKETFILTER	256		/* maximum number of minor devices */

/* #define DEBUG	1	*/
/*#define INNERDEBUG 1*/	/* define only when debugging PfiltDoFilter()
					or PfiltInputDone()  */

#define pfiltenqueue(head, elt)\
{ (elt)->F = (head);(elt)->B = (head)->B;(head)->B = (elt);((elt)->B)->F = (elt)\
; };


#define pfiltinitqueue(head)\
    { (head)->F = (head); (head)->B = (head); };


#define pfiltdequeue(head, elt)\
{\
struct Queue *q_tmp1, *q_tmp2;\
q_tmp1 = q_tmp2  = (head)->F;\
q_tmp2->B->F = q_tmp2->F;\
q_tmp2->F->B = q_tmp2->B;\
(elt) = (q_tmp1 == (head)) ? ((struct enPacket *)0):\
(struct enPacket *)q_tmp2;\
};

int pfactive=0;		/* number of currently active filters */

#define	DEBUG_ER 1

#ifdef	DEBUG_ER
int er_debug;		/* er_debug: 1=cprintf, 2=mprintf */
#endif

#define	Pfiltprintf(flags)	if (enDebug&(flags)) printf

/*
 * Symbolic definitions for enDebug flag bits
 *	ENDBG_TRACE should be 1 because it is the most common
 *	use in the code, and the compiler generates faster code
 *	for testing the low bit in a word.
 */

#define	ENDBG_TRACE	1	/* trace most operations */
#define	ENDBG_DESQ	2	/* trace descriptor queues */
#define	ENDBG_INIT	4	/* initialization info */
#define	ENDBG_SCAV	8	/* scavenger operation */
#define	ENDBG_ABNORM	16	/* abnormal events */
#define	ENDBG_NOMEM	32	/* could not allocate an mbuf */
#define	ENDBG_MISC	64	/* sleep intrs, uiomov size mismatch, etc. */


#define min(a,b)        ( ((a)<=(b)) ? (a) : (b) )

#define	splenet	splimp	/* used to be spl6 but I'm paranoid */

#define PRINET  26			/* interruptible */

/*
 *  'enQueueElts' is the pool of packet headers used by the driver.
 *  'enPackets'   is the pool of packets used by the driver (these should
 *		  be allocated dynamically when this becomes possible).
 *  'enFreeq'     is the queue of available packets
 *  'enStatePs'   is the driver state table per logical unit number
 *			(a vector of pointers to state records)
 *  'enUnit'  	  is the physical unit number table per logical unit number;
 *		  the first "attach"ed ethernet is logical unit 0, etc.
 *  'enUnitMap'	  maps minor device numbers onto interface unit #s
 *  'enAllocMap'  indicates if minor device is allocated or free
 *  'enAllDescriptors' stores OpenDescriptors, indexed by minor device #
 *  'enFreeqMin'  is the minimum number of packets ever in the free queue
 *		  (for statistics purposes)
 *  'enScavenges' is the number of scavenges of the active input queues
 *		  (for statustics purposes)
 *  'enScavDrops' is the number of packets dropped during scavenging
 *		  (for statustics purposes)
 *  'enDebug'	  is a collection of debugging bits which enable trace and/or
 *		  diagnostic output as defined above (ENDBG_*)
 *  'enUnits'	  is the number of attached units
 */
struct enPacket	enQueueElts[ENPACKETS];
struct enQueue	enFreeq;
struct enState *enStatePs[MAXUNITS];
char		enUnitMap[NPACKETFILTER];
char		enAllocMap[NPACKETFILTER];
struct enOpenDescriptor
		*enAllDescriptors[NPACKETFILTER];
int		enFreeqMin = ENPACKETS;
int		enScavenges = 0;
int		enScavDrops = 0;
int		enDebug = ENDBG_ABNORM;
int		enUnits = 0;
int		enMaxMinors = NPACKETFILTER;
struct lock_t	lk_pfilt;      /* SMP: packetfilter lock */

/*
 *  Forward declarations for subroutines which return other
 *  than integer types.
 */
extern boolean PfiltDoFilter();


/*
 * Linkages to if_XXXXX.c
 */

struct enet_info {
	struct	ifnet *ifp;	/* which ifp for output */
};

struct enet_info enet_info[MAXUNITS];

struct sockaddr enetaf = { AF_IMPLINK };

/*
 * Used in filter processing to avoid having to do an m_pullup
 */
struct enpextent {
	long	afterEnd;	/* pkt offset of first byte after extent */
	long	offset;		/* pkt offset of first byte of extent */
					/* LONG SIGNED TO MAKE IT RUN FAST */
	char   *data;		/* memory address of first byte of extent */
};

#define	MAX_EXTENTS	16	/* enough for an ether pkt in mbufs */
#define	MAX_OFFSET	(2<<ENF_NBPA)
				/* guaranteed to be larger than any offset
				   that can be embedded in a filter */


/* #define	SELF_PROF	1 */
#ifdef	SELF_PROF
int	enSelfProf = 0;		/* set this non-zero for profiling */
struct	timeval enPerPktProf = {0, 0};
int	enPktCount = 0;
struct	timeval enPerFiltProf = {0, 0};
int	enFiltCount = 0;
struct	timeval enCalibrateProf = {0, 0};
int	enCalibrateCount = 0;
#endif	SELF_PROF


/****************************************************************
 *								*
 *		Various Macros & Routines			*
 *								*
 ****************************************************************/

/*
 *  forAllOpenDescriptors(p) -- a macro for iterating
 *  over all currently open devices.  Use it in place of
 *      "for ( ...; ... ; ... )"
 *  and supply your own loop body.  The loop variable is the
 *  parameter p which is set to point to the descriptor for
 *  each open device in turn.
 */

#define forAllOpenDescriptors(p)					\
	for ((p) = (struct enOpenDescriptor *)enDesq.enQ_F;		\
	      (struct Queue *)(&enDesq) != &((p)->enOD_Link);		\
	      (p) = (struct enOpenDescriptor *)(p)->enOD_Link.F)

/*
 *  enEnqueue - add an element to a queue
 */

#define	PfiltEnqueue(q, elt)						\
{									\
	pfiltenqueue((struct Queue *)(q), (struct Queue *)(elt));		\
	(q)->enQ_NumQueued++;						\
}

/*
 *  PfiltFlushQueue - release all packets from queue, freeing any
 *  whose reference counts drop to 0.  Assumes caller
 *  is at high IPL so that queue will not be modified while
 *  it is being flushed.
 */

PfiltFlushQueue(q)
register struct enQueue *q;
{

   register struct enPacket *qelt;

    smp_lock(&lk_pfilt, LK_RETRY);
    pfiltdequeue((struct Queue *)q, qelt);
    /* "qelt" is filled in at the end of the call pfiltdequeue */

    while(qelt != NULL)
    {
	if (0 == --(qelt->enP_RefCount))
	{
	    PfiltEnqueue(&enFreeq, qelt);
	}

	pfiltdequeue((struct Queue *)q, qelt);
    }
    q->enQ_NumQueued = 0;
    smp_unlock(&lk_pfilt);

}

/*
 *  PfiltInitWaitQueue - initialize an empty packet wait queue
 */

PfiltInitWaitQueue(wq)
register struct enWaitQueue *wq;
{			

    wq->enWQ_Head = 0;
    wq->enWQ_Tail = 0;
    wq->enWQ_NumQueued = 0;
    wq->enWQ_MaxWaiting = ENDEFWAITING;

}

/*
 *  PfiltEnWaitQueue - add a packet to a wait queue
 *  SMP: assumes lock held coming in
 */

PfiltEnWaitQueue(wq, p)
register struct enWaitQueue *wq;
struct enPacket *p;
{
    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltEnWaitQueue: not lock owner");
    }
    wq->enWQ_Packets[wq->enWQ_Tail] = p;
    wq->enWQ_NumQueued++;
    PfiltNextWaitQueueIndex(wq->enWQ_Tail);
}

/*
 *  PfiltDeWaitQueue - remove a packet from a wait queue
 *  SMP: assumes lock held coming in
 */

struct enPacket *
PfiltDeWaitQueue(wq)
register struct enWaitQueue *wq;
{

    struct enPacket *p;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltDeWaitQueue: not lock owner");
    }
    if (wq->enWQ_NumQueued < 1)
    	panic("PfiltDeWaitQueue");
    wq->enWQ_NumQueued--;
    p = wq->enWQ_Packets[wq->enWQ_Head];
    PfiltNextWaitQueueIndex(wq->enWQ_Head);
    return(p);
}

/*
 *  PfiltPutBack - undo the effect of PfiltDeWaitQueue; MUST be called
 *	immediately after corresponding PfiltDeWaitQueue with no
 *	intervening interrupts!
 *  SMP: assumes lock held coming in
 */
PfiltPutBack(wq, p)
register struct enWaitQueue *wq;
struct enPacket *p;
{
    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltPutBack: not lock owner");
    }
    PfiltPrevWaitQueueIndex(wq->enWQ_Head);
    wq->enWQ_Packets[wq->enWQ_Head] = p;
    wq->enWQ_NumQueued++;
}

/*
 *  PfiltTrimWaitQueue - cut a wait queue back to size
 *  SMP: assumes lock held coming in
 */
PfiltTrimWaitQueue(d, threshold)
register struct enOpenDescriptor *d;
int threshold;
{
    register struct enWaitQueue *wq = &(d->enOD_Waiting);
    register int Counter = (wq->enWQ_NumQueued - threshold);
    register struct enPacket *p;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltTrimWaitQueue: not lock owner");
    }
#ifdef	DEBUG
    Pfiltprintf(ENDBG_SCAV)
    		("PfiltTrimWaitQueue(%x, %d): %d\n", d, threshold, Counter);
#endif
    while (Counter-- > 0)
    {
	wq->enWQ_NumQueued--;
	PfiltPrevWaitQueueIndex(wq->enWQ_Tail);
	p = wq->enWQ_Packets[wq->enWQ_Tail];
	d->enOD_Drops += (1 + p->enP_Stamp.ens_dropped);
	    /* dropped this packet and it might have recorded other drops. */
	enScavDrops++;
	if (0 == --(p->enP_RefCount))
	{
	    m_freem(p->enP_mbuf);
	    PfiltEnqueue(&enFreeq, p);
	}
    }
}
/*
 *  PfiltFlushWaitQueue - remove all packets from wait queue
 */
#define	PfiltFlushWaitQueue(wq)	PfiltTrimWaitQueue(wq, 0)

/*
 *  scavenging thresholds:
 *
 *  index by number of active files;  for N open files, each queue may retain
 *  up to 1/Nth of the packets not guaranteed to be freed on scavenge.  The
 *  total number of available packets is computed less one for sending.
 *  (assumes high IPL)
 */
unsigned short enScavLevel[NPACKETFILTER+1];

/*
 *  PfiltInitScavenge -- set up ScavLevel table
 */
PfiltInitScavenge()
{
    register int PoolSize = (ENPACKETS-ENMINSCAVENGE);
    register int i = sizeof(enScavLevel)/sizeof(enScavLevel[0]);

    PoolSize--;		/* leave one for transmitter */
    while (--i>0)
	enScavLevel[i] = (PoolSize / i);
}

/*
 *  PfiltScavenge -- scan all OpenDescriptors for all ethernets, releasing
 *    any queued buffers beyond the prescribed limit and freeing any whose
 *    refcounts drop to 0.
 *    Assumes caller is at high IPL so that it is safe to modify the queues.
 *    SMP: assumes lock held coming in
 */
PfiltScavenge()
{

    register struct enOpenDescriptor *d;
    register int threshold = 0;
    register struct enState *enStatep;
    register int unit;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltScavenge: not lock owner");
    }
    for (unit = 0; unit < enUnits; unit++) {
	enStatep = enStatePs[unit];
	threshold += enCurOpens;
    }
    threshold = enScavLevel[threshold];

    enScavenges++;
#ifdef	DEBUG
    Pfiltprintf(ENDBG_SCAV)("PfiltScavenge: %d\n", threshold);
#endif
    for (unit = 0; unit < enUnits; unit++) {
	enStatep = enStatePs[unit];
	if (enDesq.enQ_F == 0)
	    continue;			/* never initialized */
	forAllOpenDescriptors(d)
	{
	    PfiltTrimWaitQueue(d, threshold);
	}
    }
}

/*
 *  PfiltAllocatePacket - allocate the next packet from the free list
 *  Assumes IPL is at high priority so that it is safe to touch the
 *  packet queue.  If the queue is currently empty, scavenge for
 *  more packets.
 *  SMP: assumes lock held coming in
 */

struct enPacket *
PfiltAllocatePacket()
{

    register struct enPacket *p;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltAllocatePacket: not lock owner");
    }
    if (0 == enFreeq.enQ_NumQueued)
	PfiltScavenge();
    pfiltdequeue((struct Queue *)&enFreeq, p);
   /* "p" is filled in at the end of the call, pfiltdequeue */
    if (p == NULL)
	panic("PfiltAllocatePacket");
    if (enFreeqMin > --enFreeq.enQ_NumQueued)
	enFreeqMin = enFreeq.enQ_NumQueued;

    p->enP_RefCount = 0;	/* just in case */

    return(p);

}

/*
 *  PfiltDeallocatePacket - place the packet back on the free packet queue
 *  (High IPL assumed).
 */

#define	PfiltDeallocatePacket(p)						\
{									\
	if (p->enP_RefCount) panic("PfiltDeallocatePacket: refcount != 0");\
	pfiltenqueue((struct Queue *)&enFreeq, (struct Queue *)(p));		\
	enFreeq.enQ_NumQueued++;					\
}

/****************************************************************
 *								*
 *	    Routines to move uio data to/from mbufs		*
 *								*
 ****************************************************************/

/*
 * These two routines were inspired by/stolen from ../sys/uipc_socket.c
 *	   Both return error code (or 0 if success).
 */

/*
 * read: return contents of mbufs to user.  DO NOT free them, since
 *	there may be multiple claims on the packet!
 */
Pfiltrmove(p, uio, flags, padneeded, truncation)
struct enPacket *p;
register struct uio *uio;
short flags;
int *padneeded;			/* by reference, so we can change it */
int truncation;
{
	register int len;
	register int count;
	register int error = 0;
	register struct mbuf *m = p->enP_mbuf;
	int total = 0;
	int pn = *padneeded;
	
	/* first, transfer header stamp */
	if (flags & (ENTSTAMP|ENBATCH)) {
	    if (uio->uio_resid >= (sizeof(struct enstamp) + pn)) {
		error = uiomove((caddr_t)&(p->enP_Stamp) - pn,
				    sizeof(struct enstamp) + pn,
				    UIO_READ, uio);
		if (error)
		    return(error);
	    }
	    else {	/* no room for stamp */
	    	uio->uio_resid = 0;	/* make sure Pfiltread() exits */
		return(EMSGSIZE);
	    }
	}

	count = min(p->enP_ByteCount, uio->uio_resid); /* # bytes to return */
	if (count > truncation)
	    count = truncation;
	while ((count > 0) && m && (error == 0)) {
	    len = min(count, m->m_len);	/* length of this transfer */
	    count -= len;
	    total += len;
	    error = uiomove(mtod(m, caddr_t), (int)len, UIO_READ, uio);
	    
	    m = m->m_next;
	}
#if	DEBUG
	if ((error == 0) && (total != p->enP_ByteCount) &&
		(truncation > p->enP_ByteCount) &&
		(uio->uio_resid != 0)) {
	    Pfiltprintf(ENDBG_MISC)("Pfiltrmove: %d != %d (resid %d)\n",
					total, p->enP_ByteCount,
					uio->uio_resid);
	}
#endif	DEBUG	    
	/*
	 * record how much padding is needed if another packet is batched
	 * following this one in the same read()
	 */
	if ((total &= ENALIGNMASK) == 0) {
	    *padneeded = 0;	/* already aligned */
	}
	else {
	    *padneeded = ENALIGNMENT - total;
	}

	return(error);
}

Pfiltwmove(uio, mbufp)
register struct uio *uio;
register struct mbuf **mbufp;	/* top mbuf is returned by reference */
{
	struct mbuf *mtop = 0;
	register struct mbuf *m;
	register struct mbuf **mp = &mtop;
	register struct iovec *iov;
	register int len;
	int error = 0;
	
	while ((uio->uio_resid > 0) && (error == 0)) {
	    iov = uio->uio_iov;
	    
	    if (iov->iov_len == 0) {
	    	uio->uio_iov++;
		uio->uio_iovcnt--;
		if (uio->uio_iovcnt < 0)
		    panic("Pfiltwmove: uio_iovcnt < 0 while uio_resid > 0");
	    }
	    MGET(m, M_WAIT, MT_DATA);
	    if (m == NULL) {
#ifdef	DEBUG
		Pfiltprintf(ENDBG_NOMEM)("Pfiltwmove: MGET\n");
#endif
	    	error = ENOBUFS;
		break;
	    }
	    if (iov->iov_len >= CLBYTES) {	/* big enough to use a page */
		register struct mbuf *p;
		MCLGET(m, p);
		if (p == 0)
		    goto nopages;
		len = CLBYTES;
	    }
	    else {
nopages:
		len = MIN(MLEN, iov->iov_len);
	    }
	    error = uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio);
	    m->m_len = len;
	    *mp = m;
	    mp = &(m->m_next);
	}

	if (error) {		/* probably uiomove fouled up */
	    if (mtop)
		m_freem(mtop);
	}
	else {
	    *mbufp = mtop;	/* return ptr to top mbuf */
	}
	return(error);
}

/*
 *  Pfilt_open - open ether net device
 *
 *  Errors:	ENXIO	- illegal minor device number
 *		EBUSY	- minor device already in use
 *		ENOBUFS	- KM_ALLOC failed
 */

/* ARGSUSED */
Pfilt_open(dev, flag, newmin)
dev_t dev;
int flag;
int *newmin;
{
    register int md;
    register int unit;
    register struct enState *enStatep;
    register int error;
    int s;

#ifdef	DEBUG
    Pfiltprintf(ENDBG_TRACE)("Pfilt_open(%o, %x):\n", dev, flag);
#endif

#ifdef	SELF_PROF
    if (enSelfProf) {
	struct timeval starttv, endtv;
	microtime(&starttv);
	microtime(&endtv);
	enCalibrateProf.tv_sec += (endtv.tv_sec - starttv.tv_sec); 
	enCalibrateProf.tv_usec += (endtv.tv_usec - starttv.tv_usec); 
	enCalibrateCount++;
    }
#endif	SELF_PROF

#ifdef	GNODE_CLONING
    /*
     * Each open enet file has a different minor device number.
     * When a user tries to open any of them, we actually open
     * any available minor device and associate it with the
     * corresponding unit.
     *
     * This is not elegant, but UNIX will call
     * open for each new open file using the same inode but calls
     * close only when the last open file referring to the inode 
     * is released. This means that we cannot know inside the
     * driver code when the resources associated with a particular
     * open of the same inode should be deallocated.  Thus, we have
     * to make up a temporary inode to represent each simultaneous
     * open of the ethernet.  Each inode has a different minor device number.
     */
    unit = minor(dev);

    /* check for illegal unit */
    if ( (unit >= enUnits)				/* bad unit */
	|| (enet_info[unit].ifp == 0)			/* ifp not known */
	|| ((enet_info[unit].ifp->if_flags & IFF_UP) == 0) )
							/* or if down */
    {
	return(ENXIO);
    }
#else
    /*
     * Without gnode cloning, we have to have a separate entry in
     * /dev/ for each minor device.  They all start out bound to
     * the first "up" interface.
     */
    for (unit = 0; unit < enUnits; unit++) {	/* check all units in order */
	if (enet_info[unit].ifp				/* interface known */
	    && (enet_info[unit].ifp->if_flags & IFF_UP) ) 	/* and up */
		break;	/* fall out of loop */
    }
    /* check for illegal unit (i.e., nothing "up") */
    if (unit >= enUnits)				/* bad unit */
	return(ENXIO);
#endif	GNODE_CLONING

#ifdef	GNODE_CLONING
    /* Allocate a minor device number */
    md = PfiltFindMinor();
#ifdef	DEBUG
    Pfiltprintf(ENDBG_TRACE)("Pfilt_open: md = %d\n", md);
#endif
    if (md < 0)
    {
	return(EBUSY);
    }
    *newmin = md;
#else
    /* Use the minor device number supplied */
    md = minor(dev);
    if (md >= NPACKETFILTER)
	return(ENXIO);		/* an illegal minor device number */
    if (enAllocMap[md])
	return(EBUSY);		/* already in use */
#endif	GNODE_CLONING

    enUnitMap[md] = unit;
    enAllocMap[md] = TRUE;

    enStatep = enStatePs[unit];
    Pfiltprintf(ENDBG_DESQ)
    	("Pfilt_open: Desq: %x, %x\n", enDesq.enQ_F, enDesq.enQ_B);
    s = splenet(); /* SMP */
    smp_lock(&lk_pfilt, LK_RETRY);
    pfactive++;

    /* Allocate memory for this descriptor */
    KM_ALLOC(enAllDescriptors[md], struct enOpenDescriptor *,
		sizeof(struct enOpenDescriptor), KM_PFILT, KM_NOW_CL);
    if (enAllDescriptors[md] == NULL)
	return(ENOBUFS);
    PfiltInitDescriptor(enAllDescriptors[md], ENHOLDSIG);
		/* ENHOLDSIG is set by default; historical accident */
    PfiltInsertDescriptor(&(enDesq), enAllDescriptors[md]);
    smp_unlock(&lk_pfilt);
    splx(s);

    return(0);
}

/*
 * PfiltFindMinor - find a free logical device on specified unit
 */
PfiltFindMinor()
{
	register int md;
	
	for (md = 0; md < enMaxMinors; md++) {
		if (enAllocMap[md] == FALSE)
			return(md);
	}
	return(-1);
}

/*
 *  PfiltInit - initialize ethernet unit (called by pfilt_attach)
 */

PfiltInit(enStatep, unit)
register struct enState *enStatep;
register int unit;
{
    int s;

#ifdef	DEBUG
    Pfiltprintf(ENDBG_INIT)("PfiltInit(%x %d):\n", enStatep, unit);
#endif

    s = splenet(); /* SMP */
    smp_lock(&lk_pfilt, LK_RETRY);
    /*  initialize free queue if not already done  */
    if (enFreeq.enQ_F == 0)
    {
	register int i;

	pfiltinitqueue((struct Queue *)&enFreeq);
	for (i=0; i<ENPACKETS; i++)
	{
	    register struct enPacket *p;

	    p = &enQueueElts[i];
	    p->enP_RefCount = 0;
	    p->enP_Stamp.ens_stamplen = sizeof(struct enstamp);
	    PfiltDeallocatePacket(p);
	}

	/* calculate scavenger thresholds */
	PfiltInitScavenge();

	/* also a good time to init enAllocMap */
	for (i = 0; i < enMaxMinors; i++)
		enAllocMap[i] = FALSE;
    }
    pfiltinitqueue((struct Queue *)&enDesq);	/* init descriptor queue */
    enStatep->ens_UserMaxWaiting = ENNOTSUWAITING;
    enStatep->ens_AllowPromisc = false;
    smp_unlock(&lk_pfilt);
    splx(s);
}

/*
 *  Pfilt_close - ether net device close routine
 */

/* ARGSUSED */
Pfilt_close(dev, flag)
{
    register int md = ENINDEX(dev);
    register struct enState *enStatep = enStatePs[ENUNIT(dev)];
    register struct enOpenDescriptor *d = enAllDescriptors[md];
    struct enPacket *dummy;	/* a dummy pointer - used only for consistency
				 * in the macro pfiltdequeue
				 */
    int ipl;
    extern PfiltTimeout();

    enAllocMap[md] = FALSE;

#ifdef	DEBUG
    Pfiltprintf(ENDBG_TRACE)("Pfilt_close(%d, %x):\n", md, flag);
#endif

    /*
     *  insure that receiver doesn't try to queue something
     *  for the device as we are decommissioning it.
     *  (I don't think this is necessary, but I'm a coward.)
     */
    ipl = splenet();
    smp_lock(&lk_pfilt, LK_RETRY);
    pfactive--;

    /* if necessary, drop counter for promiscuous mode */
    if (d->enOD_Flag & ENPROMISC) {
	decPromiscCount(enStatep, dev);
    }

    /* if necessary, drop counter for copy-all mode */
    if (d->enOD_Flag & ENCOPYALL) {
	decCopyAllCount(enStatep, dev);
    }

    pfiltdequeue((struct Queue *)d->enOD_Link.B, dummy);
    enCurOpens--;
    Pfiltprintf(ENDBG_DESQ)
    		("Pfilt_close: Desq: %x, %x\n", enDesq.enQ_F, enDesq.enQ_B);
    PfiltFlushWaitQueue(d);
    untimeout(PfiltTimeout, (caddr_t)d);
    KM_FREE(d, KM_PFILT);
    enAllDescriptors[md] = (struct enOpenDescriptor *)0;
    smp_unlock(&lk_pfilt);
    splx(ipl);
}

/*
 *  Pfilt_read - read next packet from net
 */

/* VARARGS */
Pfilt_read(dev, uio)
dev_t dev;
register struct uio *uio;
{
    register struct enOpenDescriptor *d = enAllDescriptors[ENINDEX(dev)];
    register struct enPacket *p;
    int ipl;
    int error;
    int padneeded = 0;		/* modified by Pfiltrmove() */
    int first = 1;		/* is this the first packet in a batch? */
    extern PfiltTimeout();

#if	DEBUG
    Pfiltprintf(ENDBG_TRACE)("Pfilt_read(%x):", dev);
#endif

    ipl = splenet();
    smp_lock(&lk_pfilt, LK_RETRY);
    /*
     *  If nothing is on the queue of packets waiting for
     *  this open enet file, then set timer and sleep until
     *  either the timeout has occurred or a packet has
     *  arrived.
     */

    while (0 == d->enOD_Waiting.enWQ_NumQueued)
    {
	if (d->enOD_Timeout < 0)
	{
	    smp_unlock(&lk_pfilt);
	    splx(ipl);
	    return(0);
	}
	if (d->enOD_Timeout)
	{
	    /*
	     *  If there was a previous timeout pending for this file,
	     *  cancel it before setting another.  This is necessary since
	     *  a cancel after the sleep might never happen if the read is
	     *  interrupted by a signal.
	     */
	    if (d->enOD_RecvState == ENRECVTIMING)
		untimeout(PfiltTimeout, (caddr_t)d);
	    timeout(PfiltTimeout, (caddr_t)d, (int)(d->enOD_Timeout));
	    d->enOD_RecvState = ENRECVTIMING;
	}
	else
	    d->enOD_RecvState = ENRECVIDLE;

	sleep_unlock((caddr_t)d, PRINET, &lk_pfilt);
	smp_lock(&lk_pfilt, LK_RETRY);

	switch (d->enOD_RecvState)
	{
	    case ENRECVTIMING:
	    {
		untimeout(PfiltTimeout, (caddr_t)d);
		d->enOD_RecvState = ENRECVIDLE;
		break;
	    }
	    case ENRECVTIMEDOUT:
	    {
		smp_unlock(&lk_pfilt);
		splx(ipl);
		return(0);
	    }
	}
    }

    /* We believe there is something waiting for us in the queue */
    while (1) {
	if (d->enOD_Waiting.enWQ_NumQueued <= 0) {
	    /*
	     * Either we emptied the queue or someone else did while
	     * we weren't looking.
	     */
	    smp_unlock(&lk_pfilt);
	    splx(ipl);
	    return(0);
	}
	p = PfiltDeWaitQueue(&(d->enOD_Waiting));

	if (first) {
	    first = 0;
	}
	else {	/* this is not the first packet in a batch */
	    if (uio->uio_resid <
	    	(padneeded + sizeof(struct enstamp) + p->enP_ByteCount)) {
		/* no more room in user's buffer; don't truncate packet */
		PfiltPutBack(&(d->enOD_Waiting), p);
		smp_unlock(&lk_pfilt);
		splx(ipl);
		return(0);
	    }
	}

	/*  
	 * Move data from packet into user space.
	 */
	smp_unlock(&lk_pfilt);	/* unlock for uiomove */
	splx(ipl);
	error = Pfiltrmove(p, uio, d->enOD_Flag, &padneeded, d->enOD_Truncation);
	ipl = splenet();
	smp_lock(&lk_pfilt, LK_RETRY); /* relock after uiomove */
    
	if (--(p->enP_RefCount) <= 0)	/* if no more claims on this packet */
	{
	    m_freem(p->enP_mbuf);	/* release mbuf */
	    PfiltDeallocatePacket(p);	/* and packet */
	}
	if (d->enOD_Flag & ENBATCH && !error && uio->uio_resid > 0)
	    continue;
	else
	    break;
    }
    smp_unlock(&lk_pfilt);
    splx(ipl);

    return(error);
}



/*
 *  PfiltTimeout - process ethernet read timeout
 */

PfiltTimeout(d)
register struct enOpenDescriptor * d;
{
    register int ipl;
    register struct enOpenDescriptor *t;
    register struct enState *enStatep;
    register int unit;

#ifdef	DEBUG
    Pfiltprintf(ENDBG_TRACE)("PfiltTimeout(%x):\n", d);
#endif
    ipl = splenet();
    smp_lock(&lk_pfilt, LK_RETRY);
    if (smp) {
	/* don't call Pfilt_wakeup if the descriptor has gone away */
	for (unit = 0; unit < enUnits; unit++) {
	    enStatep = enStatePs[unit];
	    if (enDesq.enQ_F == 0)
		continue;			/* never initialized */
	    forAllOpenDescriptors(t) {
		if (d == t)
		    goto out;
	    }
	}
	/* if we fall through to here, then descriptor is no longer valid */
	smp_unlock(&lk_pfilt);
	splx(ipl);
	return(0);
    }
out:
    d->enOD_RecvState = ENRECVTIMEDOUT;
    Pfilt_wakeup(d);
    smp_unlock(&lk_pfilt);
    splx(ipl);
    wakeup((caddr_t)d);
}

/*
 *  Pfilt_write - write next packet to net
 */

int PfiltKludgeSleep[MAXUNITS];	/* Are we sleeping on IF_QFULL? */
				/*  really, # of procs sleeping on IF_QFULL */

/* VARARGS */
Pfilt_write(dev, uio)
dev_t dev;
register struct uio *uio;
{
    register int unit = ENUNIT(dev);
    register struct enState *enStatep = enStatePs[unit];
    struct mbuf *mp;
    register struct ifnet *ifp = enet_info[unit].ifp;
    int ipl;
    int error;
    int sleepcount;
    int PfiltKludgeTime();

#if	DEBUG
    Pfiltprintf(ENDBG_TRACE)("Pfilt_write(%x):\n", dev);
#endif

     if (uio->uio_resid == 0)
	 return(0);
     if (uio->uio_resid > ifp->if_mtu)	/* too large */
	 return(EMSGSIZE);
 
    /*
     * Copy user data into mbufs
     */
     if (error = Pfiltwmove(uio, &mp)) {
	 return(error);
     }

    ipl = splenet();
    smp_lock(&lk_pfilt, LK_RETRY);
    /*
     * if the queue is full,
     * hang around until there's room or until process is interrupted
     */
    sleepcount = 0;
    while (IF_QFULL(&(ifp->if_snd))) {
	if (sleepcount++ > 2) {	/* don't sleep too long */
	    smp_unlock(&lk_pfilt);
	    splx(ipl);
	    return(ETIMEDOUT);
	}
	/* if nobody else has a timeout pending for this unit, set one */
	if (PfiltKludgeSleep[unit] == 0)
	    timeout(PfiltKludgeTime, (caddr_t)unit, 2 * hz);
	PfiltKludgeSleep[unit]++;	/* record that we are sleeping */
	if (setjmp(&u.u_qsave)) {
	    /* sleep (following) was interrupted, clean up */
#if	DEBUG
	    Pfiltprintf(ENDBG_MISC)
	    	("Pfilt_write(%x): enet%d sleep %d interrupted\n", dev,
			unit, PfiltKludgeSleep[unit]);
#endif	DEBUG
	    PfiltKludgeSleep[unit]--;	/* we're no longer sleeping */
	    m_freem(mp);
	    splx(ipl);
	    return(EINTR);
	}
	sleep_unlock((caddr_t)&(PfiltKludgeSleep[unit]), PRINET, &lk_pfilt);
	smp_lock(&lk_pfilt, LK_RETRY);
	PfiltKludgeSleep[unit]--;	/* we are no longer sleeping */
    }
    
    smp_unlock(&lk_pfilt);
    /* place mbuf chain on outgoing queue & start if necessary */
    error = (*ifp->if_output)(ifp, mp, &enetaf);
			/* this always frees the mbuf chain */
    enXcnt++;
    splx(ipl);
    return(error);
}

PfiltKludgeTime(unit)
int unit;
{
	/* XXX perhaps we should always wakeup? */
	if (PfiltKludgeSleep[unit]) {
		wakeup((caddr_t)&(PfiltKludgeSleep[unit]));
		/* XXX should we restart transmitter? */
	}
}

/*
 *  Pfilt_ioctl - ether net control
 *
 *  EIOCGETP	 - get ethernet parameters
 *  EIOCSETP	 - set ethernet read timeout
 *  EIOCSETF	 - set ethernet read filter
 *  EIOCENBS	 - enable signal when read packet available
 *  EIOCINHS     - inhibit signal when read packet available
 *  FIONREAD	 - check for read packet available
 *  EIOCSETW	 - set maximum read packet waiting queue length
 *  EIOCFLUSH	 - flush read packet waiting queue
 *  EIOCMBIS	 - set mode bits
 *  EIOCMBIC	 - clear mode bits
 *  EICODEVP	 - get device parameters
 *  EIOCMFREE	 - number of free minors
 *  EIOCIFNAME	 - get name of interface for this minor
 *  EIOCTRUNCATE - set maximum number of bytes of packet to be returned
 *  EIOCALLOWPROMISC - allows non-super-users to set IFF_PROMISC
 *  EIOCALLOWCOPYALL - allows non-super-users to set IFF_PFCOPYALL
 *  EIOCMAXBACKLOG - sets non-super-user's maximum backlog
 *  EIOCSRTIMEOUT - set read timeout
 *  EIOCGRTIMEOUT - get read timeout
 *  EIOCSETIF	  - set interface for this minor
 */

/* ARGSUSED */
Pfilt_ioctl(dev, cmd, addr, flag)
caddr_t addr;
dev_t flag;
{

    register struct enState *enStatep = enStatePs[ENUNIT(dev)];
    register struct enOpenDescriptor * d = enAllDescriptors[ENINDEX(dev)];
    struct enPacket *dummy;	/* a dummy pointer - used only for consistency
				 * in the macro pfiltdequeue
				 */
    int ipl;

#if	DEBUG
    Pfiltprintf(ENDBG_TRACE)
	    	("Pfilt_ioctl(%x, %x, %x, %x):\n", dev, cmd, addr, flag);
#endif

    switch (cmd)
    {
	case EIOCGETP:
	{
	    struct eniocb t;

	    if (suser())
	    	t.en_maxwaiting = ENMAXWAITING;
	    else
	    	t.en_maxwaiting = ENNOTSUWAITING;
	    t.en_maxpriority = ENMAXPRI;
	    t.en_rtout = d->enOD_Timeout;
	    t.en_addr = -1;
	    t.en_maxfilters = ENMAXFILTERS;

	    bcopy((caddr_t)&t, addr, sizeof t);
	}
	endcase

	case EIOCSETP:
	{
	    struct eniocb t;

	    bcopy(addr, (caddr_t)&t, sizeof t);
	    d->enOD_Timeout = t.en_rtout;
	}
	endcase

	case EIOCSETF:
	{
	    struct enfilter f;
	    unsigned short *fp;

	    bcopy(addr, (caddr_t)&f, sizeof f);
	    if (f.enf_FilterLen > ENMAXFILTERS)
	    {
		return(EINVAL);
	    }
	    /* insure that filter is installed indivisibly */
	    ipl = splenet();
	    smp_lock(&lk_pfilt, LK_RETRY);
	    bcopy((caddr_t)&f, (caddr_t)&(d->enOD_OpenFilter), sizeof f);
	    /* pre-compute length of filter */
	    fp = &(d->enOD_OpenFilter.enf_Filter[0]);
	    d->enOD_FiltEnd = &(fp[d->enOD_OpenFilter.enf_FilterLen]);
	    d->enOD_RecvCount = 0;	/* reset counts when filter changes */
	    d->enOD_Drops = 0;
	    pfiltdequeue((struct Queue *)d->enOD_Link.B, dummy);
	    enDesq.enQ_NumQueued--;
	    PfiltInsertDescriptor(&(enDesq), d);
	    smp_unlock(&lk_pfilt);
	    splx(ipl);
	}
	endcase

	/*
	 *  Enable signal n on input packet
	 */
	case EIOCENBS:
	{
	    int snum;

	    bcopy(addr, (caddr_t)&snum, sizeof snum);
	    if (snum < NSIG) {
		    d->enOD_SigProc = u.u_procp;
		    d->enOD_SigPid  = u.u_procp->p_pid;
		    d->enOD_SigNumb = snum;	/* This must be set last */
	    } else {
		    goto bad;
	    }
	}
	endcase

	/*
	 *  Disable signal on input packet
	 */
	case EIOCINHS:
	{
		d->enOD_SigNumb = 0;
	}
	endcase

	/*
	 *  Check for packet waiting
	 */
	case FIONREAD:
	{
	    int n;
	    register struct enWaitQueue *wq;

	    ipl = splenet();
	    smp_lock(&lk_pfilt, LK_RETRY);
	    if ((wq = &(d->enOD_Waiting))->enWQ_NumQueued)
		n = wq->enWQ_Packets[wq->enWQ_Head]->enP_ByteCount;
	    else
		n = 0;
	    smp_unlock(&lk_pfilt);
	    splx(ipl);
	    bcopy((caddr_t)&n, addr, sizeof n);
	}
	endcase

	/*
	 *  Set maximum recv queue length for a device
	 */
	case EIOCSETW:
	{
	    unsigned un;

	    bcopy(addr, (caddr_t)&un, sizeof un);
	    /*
	     *  unsigned un         MaxQueued
             * ----------------    ------------
             *  0               ->  DEFWAITING
	     *  1..MAXWAITING   ->  un
	     *  MAXWAITING..-1  ->  MAXWAITING
	     *
	     * Non-superusers use ENNOTSUWAITING instead of ENMAXWAITING
             */
	    if (!suser())
		d->enOD_Waiting.enWQ_MaxWaiting = (un) ? min(un,ENNOTSUWAITING)
                                        : ENDEFWAITING;
	    else
		d->enOD_Waiting.enWQ_MaxWaiting = (un) ? min(un, ENMAXWAITING)
                                        : ENDEFWAITING;
	}
	endcase

	/*
	 *  Flush all packets queued for a device
	 */
	case EIOCFLUSH:
	{
	    ipl = splenet();
	    smp_lock(&lk_pfilt, LK_RETRY);
	    PfiltFlushWaitQueue(d);
	    d->enOD_Drops = 0;
	    smp_unlock(&lk_pfilt);
	    splx(ipl);
	}
	endcase

	/*
	 *  Set mode bits
	 */
	case EIOCMBIS:
	{
	    u_short mode;
	    short oldmode = d->enOD_Flag;

	    bcopy(addr, (caddr_t)&mode, sizeof mode);
	    if (mode&ENPRIVMODES)
		return(EINVAL);
	    else
		d->enOD_Flag |= mode;

	    /* if necessary, bump counter for promiscuous mode */
	    if ((mode & ENPROMISC) && ((oldmode & ENPROMISC) == 0)) {
		ipl = splenet();	/* lock PromiscCount */
		smp_lock(&lk_pfilt, LK_RETRY);
		incPromiscCount(enStatep, dev);
		smp_unlock(&lk_pfilt);
		splx(ipl);		/* unlock PromiscCount */
	    }

	    /* if necessary, bump counter for copy-all mode */
	    if ((mode & ENCOPYALL) && ((oldmode & ENCOPYALL) == 0)) {
		ipl = splenet();	/* lock CopyAllCount */
		smp_lock(&lk_pfilt, LK_RETRY);
		incCopyAllCount(enStatep, dev);
		smp_unlock(&lk_pfilt);
		splx(ipl);		/* unlock CopyAllCount */
	    }
	}
	endcase

	/*
	 *  Clear mode bits
	 */
	case EIOCMBIC:
	{
	    u_short mode;
	    short oldmode = d->enOD_Flag;

	    bcopy(addr, (caddr_t)&mode, sizeof mode);
	    if (mode&ENPRIVMODES)
		return(EINVAL);
	    else
		d->enOD_Flag &= ~mode;

	    /* if necessary, drop counter for promiscuous mode */
	    if ((oldmode & ENPROMISC) && (mode & ENPROMISC)) {
		ipl = splenet();	/* lock PromiscCount */
		smp_lock(&lk_pfilt, LK_RETRY);
		decPromiscCount(enStatep, dev);
		smp_unlock(&lk_pfilt);
		splx(ipl);		/* unlock PromiscCount */
	    }

	    /* if necessary, drop counter for copy-all mode */
	    if ((oldmode & ENCOPYALL) && (mode & ENCOPYALL)) {
		ipl = splenet();	/* lock CopyAllCount */
		smp_lock(&lk_pfilt, LK_RETRY);
		decCopyAllCount(enStatep, dev);
		smp_unlock(&lk_pfilt);
		splx(ipl);		/* unlock CopyAllCount */
	    }
	}
	endcase

	/*
	 * Return hardware-specific device parameters.
	 */
	case EIOCDEVP:
	{
	    bcopy((caddr_t)&(enDevParams), addr, sizeof(struct endevp));
	}
	endcase;

	/*
	 * Return # of free minor devices.
	 */
	case EIOCMFREE:
	{
	    register int md;
	    register int sum = 0;
	    
	    for (md = 0; md < enMaxMinors; md++)
	    	if (enAllocMap[md] == FALSE)
			sum++;
	    *(int *)addr = sum;
	}
	endcase;
	
	case EIOCIFNAME:
	{
		struct ifreq ifr;
		register char *cp, *ep;
		register struct ifnet *ifp = enet_info[ENUNIT(dev)].ifp;
		
		ep = ifr.ifr_name + sizeof(ifr.ifr_name) - 2;
		bcopy(ifp->if_name, ifr.ifr_name, (sizeof(ifr.ifr_name) - 2));
		for (cp = ifr.ifr_name; (cp < ep) && *cp; cp++)
			;
		*cp++ = '0' + ifp->if_unit;	/* unit better be < 10 */
		*cp = '\0';
		
		bcopy((caddr_t)&ifr, addr, sizeof(ifr));
	}
	endcase;

	/*
	 *  Set maximum packet length to return
	 */
	case EIOCTRUNCATE:
	{
	    int truncation;

	    bcopy(addr, (caddr_t)&truncation, sizeof truncation);
	    if (truncation < 0)
	    	d->enOD_Truncation = ENMAXINT;
	    else
	    	d->enOD_Truncation = truncation;
	}
	endcase

	/*
	 *  Allows non-super-users to set IFF_PROMISC.  This ioctl
	 * (super-user-only) sets/clears a per-interface flag; if
	 * the flag is set, then descriptors with ENPROMISC cause
	 * the interface to go into promiscuous mode.  We keep a
	 * reference count on this, and clear IFF_PROMISC when the
	 * the count goes to zero.
	 */
	case EIOCALLOWPROMISC:
	{
	    int allowpromisc;
	    register struct ifnet *ifp = enet_info[ENUNIT(dev)].ifp;
	    int wantpromisc;

	    bcopy(addr, (caddr_t)&allowpromisc, sizeof allowpromisc);

	    if (allowpromisc < 0) {	/* attempt to read current setting */
		allowpromisc = (enStatep->ens_AllowPromisc == true);
		bcopy((caddr_t)&allowpromisc, addr, sizeof allowpromisc);
		return(0);
	    }

	    if (!suser())
		return(EPERM);

	    ipl = splenet();	/* lock PromiscCount */
	    smp_lock(&lk_pfilt, LK_RETRY);
	    if ((enStatep->ens_AllowPromisc == true) && (allowpromisc == 0)) {
		/* Must disable IFF_PROMISC if we set it */
		if (enStatep->ens_PromiscCount > 0)
		    enetSetIfflags(ifp, (ifp->if_flags & ~IFF_PROMISC));
		enStatep->ens_PromiscCount = 0;
		enStatep->ens_AllowPromisc = false;
	    }
	    if ((enStatep->ens_AllowPromisc == false) && (allowpromisc != 0)) {
		/* Count the number of descriptors wanting promiscuous mode */
		wantpromisc = 0;
		forAllOpenDescriptors(d)
		    if (d->enOD_Flag & ENPROMISC)
			wantpromisc++;
		enStatep->ens_PromiscCount = wantpromisc;
		if (wantpromisc > 0)
		    enetSetIfflags(ifp, (ifp->if_flags | IFF_PROMISC));
		enStatep->ens_AllowPromisc = true;
	    }
	    smp_unlock(&lk_pfilt);
	    splx(ipl);		/* unlock PromiscCount */
	}
	endcase

	/*
	 *  Allows non-super-users to set IFF_PFCOPYALL.  This ioctl
	 * (super-user-only) sets/clears a per-interface flag; if
	 * the flag is set, then descriptors with ENCOPYALL cause
	 * ether_read() to go into copy-all mode.  We keep a
	 * reference count on this, and clear IFF_PFCOPYALL when the
	 * the count goes to zero.
	 */
	case EIOCALLOWCOPYALL:
	{
	    int allowcopyall;
	    register struct ifnet *ifp = enet_info[ENUNIT(dev)].ifp;
	    int wantcopyall;

	    bcopy(addr, (caddr_t)&allowcopyall, sizeof allowcopyall);

	    if (allowcopyall < 0) {	/* attempt to read current setting */
		allowcopyall = (enStatep->ens_AllowCopyAll == true);
		bcopy((caddr_t)&allowcopyall, addr, sizeof allowcopyall);
		return(0);
	    }

	    if (!suser())
		return(EPERM);

	    ipl = splenet();	/* lock CopyAllCount */
	    smp_lock(&lk_pfilt, LK_RETRY);
	    if ((enStatep->ens_AllowCopyAll == true) && (allowcopyall == 0)) {
		/* Must disable IFF_PFCOPYALL if we set it */
		if (enStatep->ens_CopyAllCount > 0)
		    enetSetIfflags(ifp, (ifp->if_flags & ~IFF_PFCOPYALL));
		enStatep->ens_CopyAllCount = 0;
		enStatep->ens_AllowCopyAll = false;
	    }
	    if ((enStatep->ens_AllowCopyAll == false) && (allowcopyall != 0)) {
		/* Count the number of descriptors wanting copy-all mode */
		wantcopyall = 0;
		forAllOpenDescriptors(d)
		    if (d->enOD_Flag & ENCOPYALL)
			wantcopyall++;
		enStatep->ens_CopyAllCount = wantcopyall;
		if (wantcopyall > 0)
		    enetSetIfflags(ifp, (ifp->if_flags | IFF_PFCOPYALL));
		enStatep->ens_AllowCopyAll = true;
	    }
	    smp_unlock(&lk_pfilt);
	    splx(ipl);		/* unlock CopyAllCount */
	}
	endcase

	/*
	 * Set the maximum backlog allowed for non-super-user descriptors.
	 * Make sure it is within the legal range.
	 */
	case EIOCMAXBACKLOG:
	{
	    int maxbacklog;

	    bcopy(addr, (caddr_t)&maxbacklog, sizeof maxbacklog);

	    if (maxbacklog < 0) {	/* attempt to read current setting */
		maxbacklog = enStatep->ens_UserMaxWaiting;
		bcopy((caddr_t)&maxbacklog, addr, sizeof maxbacklog);
		return(0);
	    }

	    if (!suser())
		return(EPERM);

	    if ((maxbacklog < 1) || (maxbacklog > ENMAXWAITING))
		return(EINVAL);
	    enStatep->ens_UserMaxWaiting = maxbacklog;
	}
	endcase

	/*
	 * Set the read timeout for this descriptor.  Converts
	 * from struct timeval to ticks.
	 */
	case EIOCSRTIMEOUT:
	{
	    struct timeval rtv;
	    long ticks;

	    bcopy(addr, (caddr_t)&rtv, sizeof rtv);

	    /* Check to make sure this is not too big */
	    if (rtv.tv_sec >= ((1<<((NBBY*sizeof(long))-1))/hz) )
	    if (rtv.tv_usec >= 1000000)
	    	return(EINVAL);

	    ticks = (rtv.tv_sec * hz) + (rtv.tv_usec / tick);
	    d->enOD_Timeout = ticks;
	}
	endcase

	/*
	 * Get the read timeout for this descriptor.  Converts
	 * from ticks to struct timeval.
	 */
	case EIOCGRTIMEOUT:
	{
	    struct timeval rtv;
	    long ticks = d->enOD_Timeout;
	    
	    rtv.tv_sec = ticks / hz;
	    ticks = ticks % hz;
	    rtv.tv_usec = ticks * tick;

	    bcopy((caddr_t)&rtv, addr, sizeof rtv);
	}
	endcase

	/*
	 * Change the interface associated with a minor device
	 */
	case EIOCSETIF:
	{
		struct ifreq ifr;
		register struct ifnet *oldifp = enet_info[ENUNIT(dev)].ifp;
		register struct ifnet *newifp;
		register struct enState *newStatep = (struct enState *)0;
		int unit;
		struct ifnet *ifunit();
		static struct ifnet *genericUnit();
		
		bcopy(addr, (caddr_t)&ifr, sizeof(ifr));
		if ((newifp = genericUnit(ifr.ifr_name)) == (struct ifnet *)0)
		    if ((newifp = ifunit(ifr.ifr_name)) == (struct ifnet *)0)
			return(EINVAL);

		if (newifp == oldifp)	/* no change */
		    return(0);

		/* find enState pointer for new interface */
		for (unit = 0; unit < enUnits; unit++) {
		    if (enet_info[unit].ifp == newifp) {
			newStatep = enStatePs[unit];
			break;
		    }
		}
		if (newStatep == (struct enState *)0)
		    return(EINVAL);	/* interface not known */
		
		ipl = splenet();
		smp_lock(&lk_pfilt, LK_RETRY);
		/* remove this from the old enState struct */
		pfiltdequeue((struct Queue *)d->enOD_Link.B, dummy);
		enCurOpens--;
		if (d->enOD_Flag & ENPROMISC)
		    decPromiscCount(enStatep, dev);
		if (d->enOD_Flag & ENCOPYALL)
		    decCopyAllCount(enStatep, dev);

		enUnitMap[ENINDEX(dev)] = unit;	/* change unit map */

		/* add this to the new enState struct */
		PfiltInsertDescriptor(&(newStatep->ens_Desq), d);
		PfiltFlushWaitQueue(d);
		if (d->enOD_Flag & ENPROMISC)
		    incPromiscCount(newStatep, dev);
		if (d->enOD_Flag & ENCOPYALL)
		    incCopyAllCount(newStatep, dev);
		smp_unlock(&lk_pfilt);
		splx(ipl);
	}
	endcase;

	default:
	{
	bad:
	    return(EINVAL);
	}
    }

    return(0);

}
				
/****************************************************************
 *								*
 *		Support for select() system call		*
 *								*
 *	Other hooks in:						*
 *		PfiltInitDescriptor()				*
 *		PfiltInputDone()					*
 *		PfiltTimeout()					*
 ****************************************************************/
/*
 * inspired by the code in tty.c for the same purpose.
 */

/*
 * Pfilt_select - returns true iff the specific operation
 *	will not block indefinitely.  Otherwise, return
 *	false but make a note that a selwakeup() must be done.
 */
Pfilt_select(dev, rw)
register dev_t dev;
int rw;
{
	register struct enOpenDescriptor *d;
	register struct enWaitQueue *wq;
	register int ipl;
	register int avail;
	
	switch (rw) {
	
	case FREAD:
		/*
		 * an imitation of the FIONREAD ioctl code
		 */
		d = (enAllDescriptors[ENINDEX(dev)]);
		
		ipl = splenet();
    		smp_lock(&lk_pfilt, LK_RETRY);
		wq = &(d->enOD_Waiting);
		if (wq->enWQ_NumQueued)
			avail = 1;	/* at least one packet queued */
		else {
			avail = 0;	/* sorry, nothing queued now */
			/*
			 * If there's already a select() waiting on this
			 * minor device then this is a collision.
			 * [This shouldn't happen because enet minors
			 * really should not be shared, but if a process
			 * forks while one of these is open, it is possible
			 * that both processes could select() us.]
			 */
			if (d->enOD_SelProc
			     && d->enOD_SelProc->p_wchan == (caddr_t)&selwait)
			     	d->enOD_SelColl = 1;
			else
				d->enOD_SelProc = u.u_procp;		
		}
		smp_unlock(&lk_pfilt);
		splx(ipl);	
		return(avail);

	case FWRITE:
		/*
		 * since the queueing for output is shared not just with
		 * the other enet devices but also with the IP system,
		 * we can't predict what would happen on a subsequent
		 * write.  However, since we presume that all writes
		 * complete eventually, and probably fairly fast, we
		 * pretend that select() is true.
		 */
		return(1);

	default:		/* hmmm. */
		return(1);		/* don't block in select() */
	}
}

Pfilt_wakeup(d)
register struct enOpenDescriptor *d;
{
	if (d->enOD_SelProc) {
		selwakeup(d->enOD_SelProc, d->enOD_SelColl);
		d->enOD_SelColl = 0;
		d->enOD_SelProc = 0;
	}
}

/*
 * pfilt_filter - incoming linkage from if_XXXXX.c
 */

struct mbuf *PfiltInputDone();

struct mbuf *pfilt_filter(edp, m, eptr, istrailer)
struct ether_driver *edp;
register struct mbuf *m;
struct ether_header *eptr;
int istrailer;
{
    register struct enPacket *p;
    struct mbuf *m0;
    int s;
    int HowReceived = 0;
    short d_addr[4];
    register short *sb, *dap;
    struct protosw *pr;
    int mustcopy = 0;
    int pfcopyall = (edp->ess_if.if_flags & IFF_PFCOPYALL);
    register struct enpextent *exp;
    register int offset;
    register int extents_left;
    struct enpextent map[MAX_EXTENTS];
#ifdef	SELF_PROF
    struct timeval starttv, endtv;

    if (enSelfProf)
	microtime(&starttv);
#endif	SELF_PROF

#if	DEBUG
    Pfiltprintf(ENDBG_TRACE)("pfilt_filter(%d):\n", en);
#endif

    /* Copy the destination address into main memory */
    sb = (short *)eptr->ether_dhost;
    dap = d_addr;
    dap[0] = sb[0];
    dap[1] = sb[1];
    dap[2] = sb[2];

    /* Was it a MULTICAST (or BROADCAST) packet? */
    if (eptr->ether_dhost[0] & 1) {
	HowReceived = ENSF_MULTICAST;	/* set MULTICAST */
	/*
	 *  Was it a broadcast packet?
	 */
	if ((*((long *)dap) == 0xFFFFFFFF) &&
	    (*((short *)&(dap[2])) == (short)0xFFFF)) {
	    HowReceived = ENSF_BROADCAST; /* set BROADCAST */
	    mustcopy = 1;	/* save a copy of the packet */
	} else {
	    /*
	     * else we received a MULTICAST packet.
	     * If other protocols might be interested
	     * in the packet, save a copy of it.
	     */
	    pr = (struct protosw *) iftype_to_proto(eptr->ether_type);
	    if (pr && pr->pr_ifinput) {
		mustcopy = 1;
	    }
	}
    } else {
	/* else not MULTICAST or BROADCAST packet, check if it's ours */
	if (edp->ess_if.if_flags & IFF_PROMISC) {
	    /*
	     * All promiscuously received packets go to the filter
	     * This includes COPYALL packets that we sent since
	     * they appear to belong to someone else.
	     */
	    sb = (short *)edp->ess_addr;	/* our address */
	    if ((*((long *)dap) != *((long *)sb)) ||
		    (dap[2] != sb[2])) {
		HowReceived = ENSF_PROMISC;
	    } else {
		/*
		 * We are in PROMISC mode and this packet was
		 * addressed directly to us; need to make a copy
		 * if IFF_COPYALL mode is set.
		 */
		if (pfcopyall > 1) {
		    mustcopy = 1;
		}
	    }
	} else {
	    /* else not in PROMISC mode, check to see if our own
	     * output packet was looped back (ie. COPYALL mode is set).
	     * If so, insure we hand it to the packetfilter only.
	     * If a packet reaches here, it must have been either:
	     *	- directed at us, or
	     *	- a packet we sent which was looped back
	     */
	    if (pfcopyall > 1) {
		sb = (short *)edp->ess_addr;	/* our address */
		if ((*((long *)dap) != *((long *)sb)) ||
		    (dap[2] != sb[2])) {
		    /* The destination address does not match our own,
		     * so we let the packetfilter eat it.
		     * This works because mustcopy==0 and because
		     * we label the packet as PROMISC.
		     */
		    HowReceived = ENSF_PROMISC;
		} else {
		    /* else packet was directed at us (it came in normally)
		     * must save a copy for the protocols
		     */
		    mustcopy = 1;
		}
	    }
	}
    }

/* filter:	unused */

#ifdef	DEBUG_ER
    if (er_debug) {
	int cprintf(), mprintf();
	/* er_debug=1  print to console (cprintf)
	 * er_debug=2  print to error log (mprintf)
	 */
	(er_debug == 2 ? mprintf : cprintf)("%02x-%02x-%02x-%02x-%02x-%02x -> %02x-%02x-%02x-%02x-%02x-%02x %04x %x\n",
	eptr->ether_shost[0], eptr->ether_shost[1], eptr->ether_shost[2],
	eptr->ether_shost[3], eptr->ether_shost[4], eptr->ether_shost[5],
	eptr->ether_dhost[0], eptr->ether_dhost[1], eptr->ether_dhost[2],
	eptr->ether_dhost[3], eptr->ether_dhost[4], eptr->ether_dhost[5],
	eptr->ether_type, HowReceived);
    }
#endif	DEBUG_ER

    /*
     *  If we don't need to copy the packet (and we aren't
     *  promiscuous), then we can return here and now and
     *  not bother anymore with the packet filter.
     */
    if ((HowReceived != ENSF_PROMISC) && (mustcopy == 0))
	return m;

    /*
     * If there are no active filters, just return.
     * This can happen if we are in PROMISC mode
     * or COPYALL mode, but not actually doing any filtering.
     */
    if (pfactive == 0) {
	if (mustcopy) {
	    return m;
	} else {
	    m_freem(m);
	    return 0;
	}
    }

    /*
     * We need local net header after all.
     */
    MGET(m0, M_DONTWAIT, MT_DATA);
    if (m0 == 0) {	/* out of mbufs? */
	if (mustcopy) {
	    return m;
	} else {
	    m_freem(m);
	    return (struct mbuf *) 0;
	}
    }

    /*
     * If we must copy the mbuf, do it now.  If it
     * fails, screw the packet filter and process it normally.
     * If we don't have to copy, just append the mbuf.
     */
    if (mustcopy) {
	m0->m_next = m_copy(m, 0, M_COPYALL);
	/* m_copy returns NULL if it fails */
	if (m0->m_next == NULL) {
	    m_free(m0);
	    return m;
	}
    } else {
	m0->m_next = m;
    }

    /*
     *  Copy the ether header and swap the protocol
     *  type in "wire" format.
     */
    *(mtod(m0, struct ether_header *)) = *eptr;
    eptr = mtod(m0, struct ether_header *);
    eptr->ether_type = htons((u_short)eptr->ether_type);
    m0->m_len = sizeof(struct ether_header);

    s = splenet();
    smp_lock(&lk_pfilt, LK_RETRY);
    p = PfiltAllocatePacket();	/* panics if not possible */

    p->enP_ByteCount = m_length(m0);
    microtime(&(p->enP_Stamp.ens_tstamp));
    p->enP_mbuf = m0;
    p->enP_Stamp.ens_flags = HowReceived;
    p->enP_Stamp.ens_ifoverflows = edp->ess_missed;
    
    if (istrailer) { /* was trailer encapsulation */
	p->enP_ByteCount -= 2 * sizeof(u_short);
	/* we've dropped trailer type & length */
	p->enP_Stamp.ens_flags |= ENSF_TRAILER;
    }

    /* Build a map from packet byte offsets to mbufs */
    exp = map;
    /* ASSUMPTION: m is non-null upon entry to this procedure */
    exp->data = mtod(m0, char *);
    exp->offset = 0;
    offset = m0->m_len;
    exp->afterEnd = offset;
#if	INNERDEBUG
    Pfiltprintf(ENDBG_TRACE)("[o%d: aE:%d]",exp->offset,exp->afterEnd);
#endif
    exp++;

    extents_left = (MAX_EXTENTS - 2);	/* 1 just used, 1 left for sentinel */
    while ((m0 = m0->m_next) && (--extents_left >= 0)) {
	exp->data = mtod(m0, char *);
	exp->offset = offset;
	offset += m0->m_len;
	exp->afterEnd = offset;
#if	INNERDEBUG
	Pfiltprintf(ENDBG_TRACE)("[o%d: aE:%d]",exp->offset,exp->afterEnd);
#endif
	exp++;
    }
#if	INNERDEBUG
    Pfiltprintf(ENDBG_TRACE)("\n");
#endif
    /* The last extent always has data == 0; we reserved one for this */
    exp->afterEnd = MAX_OFFSET;
    exp->data = (char *)0;
    
    m0 = PfiltInputDone(enStatePs[edp->ess_enetunit], p, map);
    if (m0)
	m_freem(m0);

#ifdef	SELF_PROF
    if (enSelfProf) {
	microtime(&endtv);
	
	enPerPktProf.tv_sec += (endtv.tv_sec - starttv.tv_sec);
	enPerPktProf.tv_usec += (endtv.tv_usec - starttv.tv_usec);
	enPktCount++;
    }
#endif	SELF_PROF
    smp_unlock(&lk_pfilt);
    splx(s);
    if (mustcopy)
	return m;
    else
	return 0;
}

/*
 * PfiltInputDone - process correctly received packet
 * SMP: assumes lock held coming in
 */

struct mbuf *PfiltInputDone(enStatep, p, exp)
register struct enState *enStatep;
register struct enPacket *p;
struct enpextent *exp;
{
    register struct enOpenDescriptor *d;
    int queued = 0;
    int drops = 0;
    register unsigned long rcount;
    register struct enOpenDescriptor *prevd;
    int Exclusive;
    struct mbuf *notwanted = 0;
#ifdef	SELF_PROF
    struct timeval starttv, endtv;
    boolean accept;
#endif	SELF_PROF

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltInputDone: not lock owner");
    }
#if	INNERDEBUG
    Pfiltprintf(ENDBG_TRACE)("PfiltInputDone(%x): %x\n", enStatep, p);
#endif

    enRmissed = p->enP_Stamp.ens_ifoverflows;

    forAllOpenDescriptors(d)
    {
	/* restrict access to promiscuously received packets */
	if ( (p->enP_Stamp.ens_flags & ENSF_PROMISC) &&
		((d->enOD_Flag & ENPROMISC) == 0) )
	    continue;
#ifdef	SELF_PROF
	/* null filters always accept all packets */
	if (d->enOD_OpenFilter.enf_FilterLen == 0)
	    accept = true;
	else if (!enSelfProf) {
	    accept = PfiltDoFilter(p, d, exp);
	} else {
	    microtime(&starttv);
	    accept = PfiltDoFilter(p, d, exp);
	    microtime(&endtv);

	    enPerFiltProf.tv_sec += (endtv.tv_sec - starttv.tv_sec);
	    enPerFiltProf.tv_usec += (endtv.tv_usec - starttv.tv_usec);
	    enFiltCount++;
	}
	if (accept)
#else
	/* null filters always accept all packets */
	if ((d->enOD_OpenFilter.enf_FilterLen == 0)
		|| PfiltDoFilter(p, d, exp))
#endif	SELF_PROF
	{
	    if (d->enOD_Waiting.enWQ_NumQueued <
	    		d->enOD_Waiting.enWQ_MaxWaiting)
	    {
		PfiltEnWaitQueue(&(d->enOD_Waiting), p);
		p->enP_RefCount++;
		p->enP_Stamp.ens_dropped = d->enOD_Drops;
		d->enOD_Drops = 0;
		queued++;
		wakeup((caddr_t)d);
		Pfilt_wakeup(d);
#if	INNERDEBUG
		Pfiltprintf(ENDBG_TRACE)("PfiltInputDone: queued\n");
#endif
	    } else {
	    	d->enOD_Drops++;
		drops++;
	    }

	    /*  send notification when input packet received  */
	    if (d->enOD_SigNumb) {
		if (d->enOD_SigProc->p_pid == d->enOD_SigPid)
			psignal(d->enOD_SigProc, d->enOD_SigNumb);
		if ((d->enOD_Flag & ENHOLDSIG) == 0)
			d->enOD_SigNumb = 0;		/* disable signal */
	    }
	    rcount = ++(d->enOD_RecvCount);
	    
	    Exclusive = !(d->enOD_Flag & ENNONEXCL);

	    /* see if ordering of filters is wrong */
	    if (d->enOD_OpenFilter.enf_Priority >= ENHIPRI) {
	    	prevd = (struct enOpenDescriptor *)d->enOD_Link.B;
		/*
		 * If d is not the first element on the queue, and
		 * the previous element is at equal priority but has
		 * a lower count, then promote d to be in front of prevd.
		 */
		if (((struct Queue *)prevd != &(enDesq.enQ_Head)) &&
	    	    (d->enOD_OpenFilter.enf_Priority ==
				prevd->enOD_OpenFilter.enf_Priority)) {
		    /* threshold difference to avoid thrashing */
		    if ((100 + prevd->enOD_RecvCount) < rcount) {
			PfiltReorderQueue(&(prevd->enOD_Link), &(d->enOD_Link));
		    }
		}
	    }

	    if (Exclusive)
		break;	/* this filter wants exclusive delivery */
	}
    }
    if (queued == 0)			/* this buffer no longer in use */
    {
	/* m_freem(p->enP_mbuf);		/* free mbuf */
	notwanted = p->enP_mbuf;
	PfiltDeallocatePacket(p);			/*  and packet */
	if (drops == 0)
	    enRunwanted++;
    }
    else
	enRcnt++;

    enRdrops += drops;

    return(notwanted);		/* returns unwanted packet or NULL */
}

#define	opx(i)	(i>>ENF_NBPA)

/* SMP: assumes lock held coming in */
boolean
PfiltDoFilter(p, d, exp)
struct enPacket *p;
struct enOpenDescriptor *d;
struct enpextent *exp;
{

    register unsigned short *sp;
    register unsigned short *fp;
    register unsigned short *fpe;
    register unsigned op;
    register unsigned arg;
    register struct enpextent *exp1;
    unsigned short stack[ENMAXFILTERS+1];
    struct fw {unsigned short arg:ENF_NBPA, op:ENF_NBPO;};

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltDoFilter: not lock owner");
    }
#ifdef	INNERDEBUG
    Pfiltprintf(ENDBG_TRACE)("PfiltDoFilter(%x,%x):\n", p, d);
#endif
    sp = &stack[ENMAXFILTERS];
    fp = &d->enOD_OpenFilter.enf_Filter[0];
    fpe = d->enOD_FiltEnd;
	/* ^ is really: fpe = &fp[d->enOD_OpenFilter.enf_FilterLen]; */
    *sp = TRUE;

    for (; fp < fpe; )
    {
	op = ((struct fw *)fp)->op;
	arg = ((struct fw *)fp)->arg;
	fp++;
	switch (arg)
	{
	    default:
		if (arg < ENF_PUSHWORD) {
		    Pfiltprintf(ENDBG_TRACE)("bad arg %d\n", arg);
		    return(false);
		}
	    	arg -= ENF_PUSHWORD;
		arg *= 2;		/* convert to byte offset */
		/*
		 * We know that arg is between 0 and MAX_OFFSET
		 * and we know that the last extent has a sentinel
		 * value (afterEnd = MAX_OFFSET) so we don't have to check it.
		 *
		 * We are searching for the first extent that ends after
		 * arg.
		 */
		exp1 = exp;
		while (exp1->afterEnd <= arg) {
#ifdef	INNERDEBUG
		    Pfiltprintf(ENDBG_TRACE)("a:%d aE:%d ", exp1->afterEnd, arg);
#endif
		    exp1++;
		}
		if (exp1->data) {
		    *--sp = *(unsigned short *)&(exp1->data[arg-exp1->offset]);
#ifdef	INNERDEBUG
		    Pfiltprintf(ENDBG_TRACE)("o:%d %x\n", exp1->offset, *sp);
#endif
		}
		else
		{
#ifdef	INNERDEBUG
		    Pfiltprintf(ENDBG_TRACE)("a:%d aE:%d =>0(len)\n",
		    				exp1->afterEnd, arg);
#endif
		    return(false);
		}
		break;
	    case ENF_PUSHLIT:
		*--sp = *fp++;
		break;
	    case ENF_PUSHZERO:
		*--sp = 0;
		break;
	    case ENF_PUSHONE:
		*--sp = 1;
		break;
	    case ENF_PUSHFFFF:
		*--sp = 0xFFFF;
		break;
	    case ENF_PUSH00FF:
		*--sp = 0x00FF;
		break;
	    case ENF_PUSHFF00:
		*--sp = 0xFF00;
		break;
	    case ENF_NOPUSH:
		break;
	}
	if (sp < &stack[2])	/* check stack overflow: small yellow zone */
	{
	    Pfiltprintf(ENDBG_TRACE)("=>0(--sp)\n");
	    return(false);
	}
	if (op == ENF_NOP)
	    continue;
	/*
	 * all non-NOP operators binary, must have at least two operands
	 * on stack to evaluate.
	 */
	if (sp > &stack[ENMAXFILTERS-2])
	{
	    Pfiltprintf(ENDBG_TRACE)("=>0(sp++)\n");
	    return(false);
	}
	arg = *sp++;
	switch (op)
	{
	    default:
#ifdef	INNERDEBUG
		Pfiltprintf(ENDBG_TRACE)("=>0(def)\n");
#endif
		return(false);
	    case opx(ENF_AND):
		*sp &= arg;
		break;
	    case opx(ENF_OR):
		*sp |= arg;
		break;
	    case opx(ENF_XOR):
		*sp ^= arg;
		break;
	    case opx(ENF_EQ):
		*sp = (*sp == arg);
		break;
	    case opx(ENF_NEQ):
		*sp = (*sp != arg);
		break;
	    case opx(ENF_LT):
		*sp = (*sp < arg);
		break;
	    case opx(ENF_LE):
		*sp = (*sp <= arg);
		break;
	    case opx(ENF_GT):
		*sp = (*sp > arg);
		break;
	    case opx(ENF_GE):
		*sp = (*sp >= arg);
		break;

	    /* short-circuit operators */

	    case opx(ENF_COR):
	    	if (*sp++ == arg) {
#ifdef	INNERDEBUG
		    Pfiltprintf(ENDBG_TRACE)("=>COR %x\n", *sp);
#endif
		    return(true);
		}
		break;
	    case opx(ENF_CAND):
	    	if (*sp++ != arg) {
#ifdef	INNERDEBUG
		    Pfiltprintf(ENDBG_TRACE)("=>CAND %x\n", *sp);
#endif
		    return(false);
		}
		break;
	    case opx(ENF_CNOR):
	    	if (*sp++ == arg) {
#ifdef	INNERDEBUG
		    Pfiltprintf(ENDBG_TRACE)("=>COR %x\n", *sp);
#endif
		    return(false);
		}
		break;
	    case opx(ENF_CNAND):
	    	if (*sp++ != arg) {
#ifdef	INNERDEBUG
		    Pfiltprintf(ENDBG_TRACE)("=>CAND %x\n", *sp);
#endif
		    return(true);
		}
		break;
	}
    }
#ifdef	INNERDEBUG
    Pfiltprintf(ENDBG_TRACE)("=>%x\n", *sp);
#endif
    return(*sp ? true : false);

}

/* SMP: assumes lock held coming in */
PfiltInitDescriptor(d, flag)
register struct enOpenDescriptor *d;
{

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltInitDescriptor: not lock owner");
    }
#if	DEBUG
    Pfiltprintf(ENDBG_TRACE)("PfiltInitDescriptor(%x):\n", d);
#endif
    d->enOD_RecvState = ENRECVIDLE;
    d->enOD_OpenFilter.enf_FilterLen = 0;
    d->enOD_OpenFilter.enf_Priority = 0;
    d->enOD_FiltEnd = &(d->enOD_OpenFilter.enf_Filter[0]);
    d->enOD_RecvCount = 0;
    d->enOD_Truncation = ENMAXINT;
    d->enOD_Timeout = 0;
    d->enOD_SigNumb = 0;
    d->enOD_Flag = flag;
    d->enOD_SelColl = 0;
    d->enOD_SelProc = 0;		/* probably unnecessary */
    /*
     * Remember the PID that opened us, at least until some process
     * sets a signal for this minor device
     */
    d->enOD_SigPid = u.u_procp->p_pid;
    d->enOD_Drops = 0;

    PfiltInitWaitQueue(&(d->enOD_Waiting));
#if	DEBUG
    Pfiltprintf(ENDBG_TRACE)("=>eninitdescriptor\n");
#endif

}

/*
 *  PfiltInsertDescriptor - insert open descriptor in queue ordered by priority
 *  SMP: assumes lock held coming in
 */

PfiltInsertDescriptor(q, d)
register struct enQueue *q;
register struct enOpenDescriptor *d;
{
    struct enOpenDescriptor * nxt;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltInsertDescriptor: not lock owner");
    }
    nxt = (struct enOpenDescriptor *)q->enQ_F;
    while ((struct Queue *)q != &(nxt->enOD_Link))
    {
	if (d->enOD_OpenFilter.enf_Priority > nxt->enOD_OpenFilter.enf_Priority)
	    break;
	nxt = (struct enOpenDescriptor *)nxt->enOD_Link.F;
    }
    pfiltenqueue((struct Queue *)&(nxt->enOD_Link),(struct Queue *)&(d->enOD_Link));
    Pfiltprintf(ENDBG_DESQ)("enID: Desq: %x, %x\n", q->enQ_F, q->enQ_B);
    q->enQ_NumQueued++;

}

int enReorderCount = 0;		/* for external monitoring */

/*
 * PfiltReorderQueue - swap order of two elements in queue
 *	assumed to be called at splenet
 * SMP: assumes lock held coming in
 */
PfiltReorderQueue(first, last)
register struct Queue *first;
register struct Queue *last;
{
	register struct Queue *prev;
	register struct Queue *next;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("PfiltReorderQueue: not lock owner");
    }
	Pfiltprintf(ENDBG_DESQ)("enReorderQ: %x, %x\n", first, last);
	
	enReorderCount++;

	/* get pointers to other queue elements */
	prev = first->B;
	next = last->F;
	
	/*
	 * no more reading from queue elements; this ensures that
	 * the code works even if there are fewer than 4 elements
	 * in the queue.
	 */

	prev->F = last;
	next->B = first;
	
	last->B = prev;
	last->F = first;
	
	first->F = next;
	first->B = last;
}

pfilt_attach(ifp, devp)
struct ifnet *ifp;
struct endevp *devp;
{
    register struct enState *enStatep;

    KM_ALLOC(enStatep, struct enState *, sizeof(struct enState),
			KM_DEVBUF, KM_NOW_CL);
    if (enStatep == NULL)
	panic("pfilt_attach: not enough memory");
    enStatePs[enUnits] = enStatep;

#ifdef	DEBUG
    Pfiltprintf(ENDBG_INIT) ("pfilt_attach: type %d, addr ", devp->end_dev_type);
    if (enDebug&ENDBG_INIT) {
	register int i;
	for (i = 0; i < devp->end_addr_len; i++)
	    printf("%o ", devp->end_addr[i]);
	printf("\n");
    }
#endif	DEBUG

    enet_info[enUnits].ifp = ifp;

    bcopy((caddr_t)devp, (caddr_t)&(enDevParams), sizeof(struct endevp));

    PfiltInit(enStatep, enUnits);
    
    return(enUnits++);
}

/*
 * Called by interface driver when its hardware address changes
 * (either because of explicit change, or because it was set wrong initially)
 * Address length cannot change
 */
pfilt_newaddress(unit, hwaddr)
int unit;
u_char *hwaddr;
{
	struct endevp *devp;

	/* unit must already exist */
	if ((unit < 0) || (unit >= enUnits)) {
	    panic("pfilt_newaddress: bad unit number");
	}
	
	devp = &(enStatePs[unit]->ens_DevParams);
	bcopy((caddr_t)hwaddr, (caddr_t)(devp->end_addr),
			devp->end_addr_len);
}

/* SMP: assumes lock held coming in */
enetSetIfflags(ifp, newflags)
register struct ifnet *ifp;
register int newflags;
{
	struct ifreq ifr;
	int ret;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("enetSetIfflags: not lock owner");
    }
	/* code more or less copied from net/if.c */
	ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
					(newflags & ~IFF_CANTCHANGE);

	/* XXX I hope nobody looks at ifr_name here XXX */
	ifr.ifr_flags = newflags;
	/* unlock before calling the driver */
	smp_unlock(&lk_pfilt);
	ret=(*(ifp->if_ioctl))(ifp, SIOCSIFFLAGS, &ifr);
	smp_lock(&lk_pfilt, LK_RETRY);
	return(ret);
}

/* SMP: assumes lock held coming in */
decPromiscCount(enStatep, dev)
register struct enState *enStatep;
register dev_t dev;
{
	struct ifnet *ifp;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("decPromiscCount: not lock owner");
    }
	if (enStatep->ens_AllowPromisc &&
		(--(enStatep->ens_PromiscCount) == 0)) {
	    ifp = enet_info[ENUNIT(dev)].ifp;
	    enetSetIfflags(ifp, (ifp->if_flags & ~IFF_PROMISC));
	}
}

/* SMP: assumes lock held coming in */
incPromiscCount(enStatep, dev)
register struct enState *enStatep;
register dev_t dev;
{
	struct ifnet *ifp;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("incPromiscCount: not lock owner");
    }
	if (enStatep->ens_AllowPromisc &&
		(enStatep->ens_PromiscCount++ == 0)) {
	    ifp = enet_info[ENUNIT(dev)].ifp;
	    enetSetIfflags(ifp, (ifp->if_flags | IFF_PROMISC));
	}
}

/* SMP: assumes lock held coming in */
decCopyAllCount(enStatep, dev)
register struct enState *enStatep;
register dev_t dev;
{
	struct ifnet *ifp;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("decCopyAllCount: not lock owner");
    }
	if (enStatep->ens_AllowCopyAll &&
		(--(enStatep->ens_CopyAllCount) == 0)) {
	    ifp = enet_info[ENUNIT(dev)].ifp;
	    enetSetIfflags(ifp, (ifp->if_flags & ~IFF_PFCOPYALL));
	}
}

/* SMP: assumes lock held coming in */
incCopyAllCount(enStatep, dev)
register struct enState *enStatep;
register dev_t dev;
{
	struct ifnet *ifp;

    if (smp_debug) {
	if (!(smp_owner(&lk_pfilt)))
		panic("incCopyAllCount: not lock owner");
    }
	if (enStatep->ens_AllowCopyAll &&
		(enStatep->ens_CopyAllCount++ == 0)) {
	    ifp = enet_info[ENUNIT(dev)].ifp;
	    enetSetIfflags(ifp, (ifp->if_flags | IFF_PFCOPYALL));
	}
}

/* Convert generic name to a struct ifnet pointer */
static char genericPrefix[] = ENGENPREFIX;
static struct ifnet *genericUnit(iname)
register char *iname;
{
	register int i = 0;
	register int unit;
	
	/* Check for correct prefix */
	while (genericPrefix[i]) {
	    if (genericPrefix[i] != iname[i])
		return((struct ifnet *)0);
	    i++;
	}

	/* Check that suffix is a single digit and within range */
	unit = iname[i] - '0';
	if ((unit < 0) || (unit > 9) || (unit >= enUnits) || iname[i+1])
	    return((struct ifnet *)0);

	return(enet_info[unit].ifp);
}
#endif	(NPACKETFILTER > 0)
