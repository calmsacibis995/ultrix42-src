#ifndef lint
static	char	*sccsid = "@(#)param.c	4.4	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *   University    of   California,   Berkeley,   and   from   Bell	*
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
/*
 *
 * Modification History
 *
 * 28 Feb 91 prs
 *	Added max_nofile symbol to support a configurable
 *	number of open file descriptors.
 *
 * 27 Dec 90 prs
 *	Added ufs_blkpref_lookbehind symbol.
 *
 *  11 Dec 89 jaa
 *	removed vasslop/VASSLOP, they're not used anymore
 *
 *	09-Nov-1989	jaw
 *	add smp parameters.
 *
 *	16-AUG-1989	bp
 *	Conditionalized vasslop on SAS
 *
 *	12-Jun-89	bp
 *	Added kernel memory allocator high water mark tuning parameters.
 *
 * 12 Jun 89 -- gg
 *	added few dynamic swap configurable paramters(eg. maxdsiz, maxssiz
 *	vasslop, maxretry, swapfrag). Also removed the configurable parameters
 *	dmmin and dmmax.
 *
 *	5-May-89 -- Adrian Thoms
 *	Added maxprocptes parameter with default value
 *
 *	13-Jun-88 -- chet
 *	added configurable buffer cache support
 *
 *	3-Mar-88 -- jaa
 *	moved all shared memory, semaphore and dmmin/dmmax 
 *	data to data/vm_data.c
 *
 *	15-Jan-88 -- lp
 *	Removed NMBCLUSTER.
 *
 *	15-jan-87 -- koehler
 *	when calculating the number of gnodes, add the number of text
 *	slots needed
 *
 *	10-Jul-86 -- tresvik
 *	moved desfree, minfree, and lotsfree here from ../sys/vm_sched.c
 *	so that default paging limits could be overriden when running a
 *	SAS kernel.
 *
 *	29 Apr 86 -- depp
 *	Modified shared memory configuration data so that it's config file
 *	configurable.
 *
 *	09 Sep 85 -- reilly
 *	Modifed to handle the lockf call.
 *
 *	12 Aug 85 -- depp
 *	Changed sizing of "sem_undo" using NPROC rather than SEMUND since
 *	the addressing method is "sem_undo[u.u_procp - proc]".
 *
 *	12 Mar 85 -- depp
 *	Added kernel data structures for System V shared memory
 *
 *	27 Feb 85 -- depp
 *	Added kernel data structures for System V semaphores and Message queues
 *
 */


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/callout.h"
#include "../h/clist.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/quota.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/flock.h"
#include "../h/cpudata.h"
#include "../machine/pte.h"
#include "../machine/vmparam.h"
#include "../h/ipc.h"
#include "../h/msg.h"
#include "../h/sem.h"
#include "../h/shm.h"
#include "../h/vmmac.h"
#include "../h/kmalloc.h"

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 * Compiled with -DHZ=xx -DTIMEZONE=x -DDST=x -DMAXUSERS=xx
 */

#define	HZ 100
int	hz = HZ;
int	tick = 1000000 / HZ;
int     tickadj = 1;
struct	timezone tz = { TIMEZONE, DST };

/* must be multiple of 32 for bitmap */
#define	NPROC (((20 + 8 * MAXUSERS)+31)&(~0x1f))
int	nproc = NPROC;
int	max_proc_index = NPROC>>5;

#ifdef INET
#define	NETSLOP	20			/* for all the lousy servers*/
#else
#define	NETSLOP	0
#endif

#define NTEXT (24 + MAXUSERS + NETSLOP)
int	ntext = NTEXT;

#define	NGNODE (NPROC + MAXUSERS + NTEXT + 48)
int	ngnode = NGNODE;

int	nchsize = NGNODE * 11 / 10;

struct flckinfo flckinfo = {  4 * ( NGNODE / 10 ), NGNODE / 10 };

int	nfile = 16 * (NPROC + 16 + MAXUSERS) / 10 + 2 * NETSLOP + 32;

int	ncallout = 16 + NPROC;

int	nclist = 75 + 16 * MIN(75,MAXUSERS);

int	nport = NPROC / 2;

int	nquota = (MAXUSERS * 9)/7 + 3;
int	ndquot = (MAXUSERS*NMOUNT)/4 + NPROC;

int 	maxusers = MAXUSERS;

/*
 * The bufcache value is the  % of memory to use for buffer cache data.
 * It is passed by the makefile (and thus, config) as a defined constant
 * on the compile line in machine specific directories. This constant
 * is not defined for BINARY.machine, so set it to a default value there.
 */
#ifndef BUFCACHE
#define BUFCACHE 10
#endif
int	bufcache = BUFCACHE;

int 	maxuprc	= MAXUPRC;
#ifndef MAXPROCPTES
#define MAXPROCPTES (btop(0x80000000))
#endif
unsigned maxprocptes = MAXPROCPTES;

/*
 * These are initialized at bootstrap time
 * to values dependent on memory size
 */
int	nbuf, nswbuf;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted.
 */
struct	proc *proc, *procNPROC;
struct	text *text, *textNTEXT;
struct	gnode *gnode, *gnodeNGNODE;
struct	file *file, *fileNFILE;
struct filock  	*flox;
struct flino   	*flinotab;
struct 	callout *callout;
struct	cblock *cfree;
struct	buf *buf, *swbuf;
short	*swsize;
int	*swpf;
char	*buffers;
struct	cmap *cmap, *ecmap;
#ifdef QUOTA
struct	quota *quota, *quotaNQUOTA;
struct	dquot *dquot, *dquotNDQUOT;
#endif

#define MAX_SECONDS_WAIT 40
int max_spin_count = 1000000;
int max_seconds_wait = 40;
int max_sleep_count = 2000;
int nisp = NISP;
#ifdef SMP
int smp=1;
#else
int smp=0;
#endif

/*
 * The next three variables are forced to 1 for a SAS kernel, to block
 * paging.  This overrides default paging limits.
 */
#ifdef SAS
int notsas = 0;
int desfree = 1;
int lotsfree = 1;
int minfree = 1;
#else SAS
int notsas = 1;
int desfree = 0;
int lotsfree = 0;
int minfree = 0;
#endif SAS


#ifndef MAXTSEG
#define	MAXTSEG	12            /* for 12 meg text (old NXDAD) */
#endif	MAXTSEG

/*
 *	SWAPFRAG is in terms of disk blocks
 */

#ifndef SWAPFRAG
#define SWAPFRAG 64
#endif SWAPFRAG

#ifndef MAXRETRY
/* retry count to get kernel space (KM_ALLOC) */
#define MAXRETRY 10
#endif MAXRETRY

int  swapfrag = SWAPFRAG;
int  maxretry = MAXRETRY;

int  maxdsiz  = btoc(MAXDSIZ);
int  maxssiz  = btoc(MAXSSIZ);

long usrptsize = 0;

int	maxtseg = MAXTSEG * 2;
int	maxtsiz = (MAXTSEG / 2) * 2048 - SLOP;

/* The following data structs are required for System V IPC */
char ipcmsgbuf[ MSGSEG * MSGSSZ];
struct map	msgmap[MSGMAP];
struct msqid_ds	msgque[MSGMNI];
struct msg	msgh[MSGTQL];
struct msginfo	msginfo = {
	MSGMAP,
	MSGMAX,
	MSGMNB,
	MSGMNI,
	MSGSSZ,
	MSGTQL,
	0,
	MSGSEG
};

struct	semid_ds	sema[SEMMNI];
struct	sem		sem[SEMMNS];
struct	map		semmap[SEMMAP];
struct	sem_undo	*sem_undo[NPROC];

#define	SEMUSZ	(sizeof(struct sem_undo)+sizeof(struct undo)*SEMUME)
int	semu[((SEMUSZ*SEMMNU)+NBPW-1)/NBPW];

union semtmp {
	short		semvals[SEMMSL];
	struct semid_ds	ds;
	struct sembuf	semops[SEMOPM];
};

int	semtmp_size = sizeof(union semtmp);

struct	seminfo seminfo = {
	SEMMAP,
	SEMMNI,
	SEMMNS,
	SEMMNU,
	SEMMSL,
	SEMOPM,
	SEMUME,
	SEMUSZ,
	SEMVMX,
	SEMAEM
};

struct smem	smem[SMMNI];	

struct	sminfo sminfo = {
	clrnd(SMMAX) * NBPG,
	(SMMIN * NBPG) & ~(CLBYTES - 1),
	SMMNI,
	SMSEG,
	clrnd(SMBRK),
	MAXSMAT,
};

/*
 * Size of wired allocator map.
 */

int km_wmapsize = KMEMWMAP;

/*
 * The following declarations are required for the tuneable
 * parameters of the kernel memory allocator for the high water marks.
 */

short kmbucket_hwm = -1;			/* Default value */
short kmemhwm[MAXBUCKETSAVE+1] = {
0,0,0,0,		/* Unused buckets */
-1,			/* Bucket with    16 byte elements  	*/
-1,			/* Bucket with    32 byte elements  	*/
-1,			/* Bucket with    64 byte elements	*/
KMBUCKET_HWM*4,		/* Bucket with   128 byte elements	*/
-1,			/* Bucket with   256 byte elements	*/
-1,			/* Bucket with   512 byte elements	*/
-1,			/* Bucket with  1024 byte elements	*/
-1,			/* Bucket with  2048 byte elements	*/
-1,			/* Bucket with 	4096 byte elements	*/
-1,			/* Bucket with 	8192 byte elements	*/
-1,			/* Bucket with 16384 byte elements	*/
-1,			/* Bucket with 32768 byte elements	*/
};

int     delay_wbuffers = 0;

int	ufs_blkpref_lookbehind = 8;

/*
 * The maximum number of file descriptors is now a configurable option
 * (max_nofile variable in /sys/conf/{mips|vax}/param.c).
 * The getdtablesize(2) system call should be used to obtain the
 * current limit. The value returned by getdtablesize() must be greater
 * than 64, and less than or equal to MAX_NOFILE in types.h . The
 * MAX_NOFILE define is needed for backward compatability with broken
 * programs that need a static sized array for selecting. These programs
 * should be modified to use the getdtablesize() interface for sizing.
 */

int	max_nofile = 64;

/*
 * The following three parameters control the effect that signal
 * activity has on process priority. In the default case shown
 * here, the sigpause system call and signal delivery via psignal
 * do effect process priority. This is historically compatible
 * with BSD versions of unix. Here are the defaults:
 *
 *	int sigpause_priority_mod = 0;
 *	int sigpause_priority_limit = PZERO + 1;
 *	int psignal_priority_mod = 1;
 *
 * These setting can be changed if it is perceived that the priority
 * adjustments are having detrimental effects on the system. Caution
 * is advised.
 *
 * The sigpause system call causes a process to sleep at priority
 * PSLEP. Setting the 'sigpause_priority_mod' to a non-zero value
 * causes the process to sleep at it's current priority, but no
 * lower than the value set in 'sigpause_priority_limit' (lower
 * numbered priorities have the highest priority). A processes
 * current priority should almost always be poorer that PSELP.
 *   Note: The sigpause_priority_limit should never be set lower
 *	than PZERO+1. Sleeping at a priority of PZERO or lower
 *	causes the process to be unable to be awakened by a signal.
 *	Since the design of the sigpause system call is to wait
 *	for a signal, allowing a process to sleep at PZERO or
 *	below will cause the process to sleep forever.
 * The psignal_priority_mod, when set to zero, causes the signal
 * delivery code to eliminate the normal setting of the process's
 * priority to PUSER, thus leaving the priority at it's current value.
 */
int sigpause_priority_mod = 0;
int sigpause_priority_limit = PZERO + 1;
int psignal_priority_mod = 1;
