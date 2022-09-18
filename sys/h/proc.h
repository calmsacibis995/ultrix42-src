/* 	@(#)proc.h	4.4	(ULTRIX)	2/28/91 	*/

/************************************************************************
 *									*
 *		Copyright (c) 1984, 1986, 1987, 1989 by			*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/h/proc.h
 *
 * 27 Feb 91 jaa
 *	added SNFSPGN to p_vm.  used by pagein to let nfs know that this
 *	pagein cannot be interrupted
 *
 * 9/4/90 -- dlh
 *	added pointer to the proc structure to point to the vector processor
 *	context area (struct vpcontext *p_vpcontext)
 *
 * 11 Dec 89 --	sekhar
 *	Added a new bit SXCTDAT for mips. This bit when set inidicates
 *	that icache should  be flushed on pagein for this process.
 *
 * 13 Oct 89 -- gmm
 *	Changes for MIPS smp support. Added new field p_cpumask
 *
 * 14 Jun 89 -- jaa
 *	added p_smcount, sm_saddr and sm_eaddr to vax arch
 *
 * 12 Jun 80 -- gg
 *	added 4 new fields p_dmap,p_cdmap, p_smap,p_csmap for dynamic swap.
 *	removed p_swaddr from proc structure.
 *
 * 28 Feb 89 -- jaw
 *	seperate SLOAD and SSEL flags.
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 23 Aug 88 -- miche
 *	add support for ref'ing a process: p_exist, p_ref and P_DYING
 *
 * 25 Jul 88 -- jmartin
 *	Declare the SMP lock lk_p_vm.  Define the macros SET_P_VM and
 *	CLEAR_P_VM.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 07 Jun 88 -- miche
 *	SMP procqs:  Add FORALLPROC macro; remove allproc, freeproc, zombproc
 *************SMP changes above **************
 *
 *  1 Feb 89 -- jmartin
 *	Move machine-dependent mapping info to ../machine/vm/proc_map.h
 *
 * 30-Aug-88	jaw
 *	add proc field p_master to fix ASMP bug.
 *
 * 11 Jul 88 -- Mark Parenti
 *	Add SEXECDN flag to indicate process has been execed.
 *	Add p_sid field to hold session ID.
 *
 * 02 Jun 88 -- Mark Parenti
 *	Change SCLDSTP to SNOCLDSTP per change in POSIX
 *
 * 12 Jan 88 -- Fred Glover
 *	Add SLKDONE flag to indicate Sys-V file locking applied
 *
 * 28-Dec-87 -- Tim Burke
 *	Add p_ttyp a pointer to the controling terminal.  Formerly this field
 *	was in the "u" structure.
 *
 * 14-Oct-87 -- map
 *	Add p_progenv to hold mode(POSIX, BSD, SYSV) of current process.
 *	Add p_suid to hold effective uid. (Saved set uid).
 *	Add p_sgid to hold effective gid. (Saved set gid).
 *
 * 02-Apr-86 -- jrs
 *	Remove special master run queue as part of scheduler rewrite
 *
 * 25 Apr 85 -- depp
 *	Removed SHMEM ifdefs
 *
 * 01 Mar 85 -- depp
 *	Added shared memory definitions to proc structure\*
 *
 * 23 Oct 84 -- jrs
 *	Add definitions for linked proc list
 *	Derived from 4.2BSD, labeled:
 *		proc.h 6.2	84/06/06
 *
 * -----------------------------------------------------------------------
 */

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#if defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C))
#ifdef KERNEL
#include "../machine/vm/proc_map.h"
#else
#include <machine/vm/proc_map.h>
#include <sys/smp_lock.h>
#endif /* KERNEL */
struct	proc {
	struct	proc *p_link;	/* linked list of running processes */
	struct	proc *p_rlink;
	struct	proc *p_nxt;	/* linked list of allocated proc slots */
	struct	proc **p_prev;		/* also zombies, and free proc's */
	struct	pte *p_addr;	/* u-area kernel map address */
	struct	proc_map proc_map; /* machine-dependent VM info */
	s_char	p_usrpri;	/* user-priority based on p_cpu and p_nice */
	s_char	p_pri;		/* priority, negative is high */
	char	p_cpu;		/* cpu usage for scheduling */
	char	p_stat;
	char	p_time;		/* resident time for scheduling */
	s_char	p_nice;		/* nice for cpu usage */
	char	p_slptime;	/* time since last block */
	char	p_cursig;
	int	p_sig;		/* signals pending to this process */
	int	p_sigmask;	/* current signal mask */
	int	p_sigignore;	/* signals being ignored */
	int	p_sigcatch;	/* signals being caught by user */
	/*
	 int	p_flag;		Replaced with the following:
	*/
	int	p_sched;	/* contains old SLOAD */
	int	p_select;	/* contains old SSEL */
	int	p_vm;		/* contains old: SVFORK, SVFDONE, SNOVM,
				 * SKEEP, SPHYSIO, SPAGE, SULOCK, SSWAP,
				 * SPAGI, SUANOM, SSEQL, SLOCK, SPTECHG,
				 * SEXECDN */
	int	p_affinity;	/* MASK of CPUS on which can run */
#ifdef __mips
	int	p_cpumask;	/* last cpu on which the process ran */
#endif /* __mips */
	int	p_trace;	/* contains old STRC, SWTED */
	int	p_type;		/* contains old SSYS, SWEXIT and SLOGIN */
	int	p_file;		/* contains old SLKDONE */
	int	p_sigflag;	/* contains new SNOCLDSTP */
#ifdef __mips
	int	p_mips_flag;	/* contains SFIXADE */
#endif /* __mips */
	uid_t	p_uid;		/* eff. user id, used to direct tty signals */
	uid_t	p_suid;		/* saved set uid, used to direct tty signals */
	uid_t	p_sgid;		/* saved set group id, used by setgid */
	short	p_pgrp;		/* name of process group leader */
	short	p_pid;		/* unique process id */
	short	p_ppid;		/* process id of parent */
	u_short	p_xstat;	/* Exit status for wait */
	u_short	p_progenv;	/* process compatibility mode */
	short	p_poip;		/* page outs in progress */
	pid_t	p_sid;		/* session id (for POSIX job control) */
	struct	rusage *p_ru;	/* mbuf holding exit information */
	size_t	p_tsize;	/* size of text (clicks) */
	size_t	p_dsize;	/* size of data space (clicks) */
	size_t	p_ssize;	/* copy of stack size (clicks) */
	size_t 	p_rssize; 	/* current resident set size in clicks */
	size_t	p_maxrss;	/* copy of u.u_limit[MAXRSS] */
	size_t	p_swrss;	/* resident set size before last swap */
	caddr_t p_wchan;	/* event process is awaiting */
	int	p_exist;	/* if P_DYING, process will exit */
	int	p_ref;		/* count of processes relying on p */
	int	p_waitchk;	/* coordinates wait with setting ZOMB and STOP */
	struct	text *p_textp;	/* pointer to text structure */
	struct	proc *p_xlink;	/* linked list of procs sharing same text */
	short	p_cpticks;	/* ticks of cpu time */
#ifdef __vax
	float	p_pctcpu;	/* %cpu for this process during p_time */
#endif /* __vax */
#ifdef __mips
	int	p_pctcpu;	/* %cpu for this process during p_time */
	long	p_fp;		/* generate SIGFPE on all fp interrupts */
	u_short	p_puac;	/* print/don't print unaligned access msgs */
#endif /* __mips */
	short	p_ndx;		/* proc index for memall (because of vfork) */
	short	p_idhash;	/* hashed based on p_pid for kill+exit+... */
	struct	proc *p_pptr;	/* pointer to process structure of parent */
	struct	proc *p_cptr;	/* pointer to youngest living child */
	struct	proc *p_osptr;	/* pointer to older sibling processes */
	struct	proc *p_ysptr;	/* pointer to younger siblings */
	char	*p_debug;	/* for ptrace/procxmt communciation */
	struct	itimerval p_realtimer;
	struct	quota *p_quota;	/* quotas for this process */
	struct  tty *p_ttyp;    /* controlling tty pointer 	*/
	size_t	p_smsize;	/* size of SM space (clicks)	*/
	int	p_smbeg;	/* page table offset of first SMS */
	int	p_smend;	/* page table offset of end of	*/
				/*	last attached SMS.	*/
	int     p_smcount;      /* count of SMS attached to proc */
	struct p_sm {		/* shared memory related elements	*/
		struct	smem *sm_p;	/* pointer to SM struct	*/
				/* linked list of procs		*/
				/* sharing the same shared	*/
				/* memory segment.		*/
		struct	proc *sm_link;
		int     sm_saddr;		/* starting addr of SMS */
		int     sm_eaddr;		/* ending addr of SMS */
		int	sm_pflag;	/* R/W permissions	*/
		short	sm_lock;	/* this proc has locked SMS */
	} *p_sm;
	struct lock_t 	*p_hlock;
	struct 	dmap	*p_dmap;	/* disk map for data segment */
	struct 	dmap	*p_smap;	/* disk map for stack segment */
	struct 	dmap	*p_cdmap;	/* shadows data/stack swap */
	struct 	dmap	*p_csmap;	/* used during fork/exec*/

	struct	vpcontext	*p_vpcontext;
					/* ptr to proc's vp context area */
};
#endif /* defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C)) */

#define SIZEOF_PSM	(sminfo.smseg*sizeof(struct p_sm))

#define	PIDHSZ		64
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

#ifdef	__mips
#define	UAC_MSGON	1    /* print unaligned access messages, dflt */
#define	UAC_MSGOFF	0    /* don't print unaligned access messages */
#endif /* __mips */

#if defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C))
#ifdef KERNEL
short	pidhash[PIDHSZ];
struct	proc *pfind(), *proc_get();
struct	proc *proc, *procNPROC;	/* the proc table itself */
extern	int nproc, max_proc_index;
#define MAX_PROC_INDEX 512
unsigned proc_bitmap[MAX_PROC_INDEX];

#ifdef __vax
#define	NQS	32		/* 32 run queues */

struct	prochd {
	struct	proc *ph_link;	/* linked list of running processes */
	struct	proc *ph_rlink;
} qs[NQS];
#endif /* __vax */

#ifdef __mips
/*
 * Use single run queue. There is no ffs instruction and run queue's
 * are not typically very long.
 */
struct	prochd {
	struct	proc *ph_link;	/* linked list of running processes */
	struct	proc *ph_rlink;
} qs;
#endif /* __mips */

int	whichqs;		/* bit mask summarizing non-empty qs's */

#endif /* KERNEL */
#endif /* defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C)) */

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes */
/* These have been divied up into several fields:
 *	p_vm, p_sched, p_type, p_trace, p_file
 *	u_oweupc, u_sigflag.
 *	Note that STIMO isn't used anywhere
 * Both ps and pstat count on there being no overlap in bits,
 * so that these fields can be or'red together for printing
 */
#define	SLOAD	0x0000001	/* p_sched: in core */
#define	SSYS	0x0000002	/* p_type:  or pager process */
#define	SLOCK	0x0000004	/* p_vm: process being swapped out */
#define	SSWAP	0x0000008	/* p_vm: save area flag */
#define	STRC	0x0000010	/* p_trace: process is being traced */
#define	SWTED	0x0000020	/* p_trace: another tracing flag */
#define	SULOCK	0x0000040	/* p_vm: user settable lock in core */
#define	SPAGE	0x0000080	/* p_vm: process in page wait state */
#define	SKEEP	0x0000100	/* p_vm: another flag to prevent swap out */
#define	SOMASK	0x0000200	/* u_sigflag: restore old mask after taking signal */
#define	SWEXIT	0x0000400	/* p_type:  on exiting */
#define	SPHYSIO	0x0000800	/* p_vm: doing physical i/o (bio.c) */
#define	SVFORK	0x0001000	/* p_vm: process resulted from vfork() */
#define	SVFDONE	0x0002000	/* p_vm: another vfork flag */
#define	SNOVM	0x0004000	/* p_vm: no vm, parent in a vfork() */
#define	SPAGI	0x0008000	/* p_vm: init data space on demand, from inode */
#define	SSEQL	0x0010000	/* p_vm: user warned of sequential vm behavior */
#define	SUANOM	0x0020000	/* p_vm: user warned of random vm behavior */
#define	STIMO	0x0040000	/* NOT_USED: timing out during sleep */
/* was SDETACH */
#ifdef __mips
#define SXCTDAT	0x0080000	/* p_vm: an icache flush on pagein */
#endif
#define	SNOCLDSTP 0x0100000	/* p_sigflagPOSIX child stop flag */
/* was SOUSIG */
#define	SOUSIG	0x0100000	/* u_sigflag: using old signal mechanism */
#define	SOWEUPC	0x0200000	/* u_oweupc: owe process an addupc() call at next ast */
#define	SSEL	0x0400000	/* p_sched: selecting; wakeup/waiting danger */
#define	SLOGIN	0x0800000	/* p_type:  login process (legit child of init) */
#define	SPTECHG	0x1000000	/* p_vm: pte's for process have changed */
#define SNFSPGN 0x2000000	/* p_vm: uninterruptible pagin over nfs */
#define SLKDONE 0x4000000	/* p_file: Sys-V file locking applied */

#ifdef __mips
#define SFIXADE 0x8000000	/* fixup unalligned address errors */
#define SIDLEP	0x20000000	/* idle process */

/* flags for p_fp */
#define	P_FP_SIGINTR1	1
#define	P_FP_SIGINTR2	2
#endif /* __mips */

#define SEXECDN 0x10000000	/* p_vm: exec() done - needed for POSIX pgrp test */

#ifndef __vax
# define P_VM_NO_OP	0
#else /* vax */
# define P_VM_NO_OP	SPTECHG
#endif /* __vax */

#ifdef KERNEL
#ifndef LOCORE
# define SET_P_VM(proc, flags)		\
if ((flags) & ~(P_VM_NO_OP))		\
  {					\
    int s;				\
    s = splimp();			\
    smp_lock(&lk_p_vm, LK_RETRY);	\
    (proc)->p_vm |= (flags);		\
    smp_unlock(&lk_p_vm);		\
    (void)splx(s);			\
  } else

# define CLEAR_P_VM(proc, flags)	\
if ((flags) & ~(P_VM_NO_OP))		\
  {					\
    int s;				\
    s = splimp();			\
    smp_lock(&lk_p_vm, LK_RETRY);	\
    (proc)->p_vm &= ~(flags);		\
    smp_unlock(&lk_p_vm);		\
    (void)splx(s);			\
  } else

struct lock_t lk_p_vm;
#endif /* LOCORE */
#endif /* KERNEL */

/* for affinity */
#define PRIMARY 	1
#define ALLCPU		-1

/* for p_exist */
#define P_ALIVE	0	/* process will stick around if ref'ed */
#define P_DYING	1	/* no longer legal to ref */
#define P_DEAD	2	/* done with exit: parent may now clear */

/*
 * There is one bit for every active process slot
 */

/*
 * This algorithm works out to 39 usec for any mask set.
 *  For fully populated masks, this is 1.22 usec/proc,
 *		for half full, this is 2.44 usec/proc
 *		for 1/4  full, this is 4.88 usec/proc
 *
 * If we assume a half populated process table of 1024 entries,
 * clustered entirely in the top 2/3s of the table, you get:
 *
 *	1024/32 = 32 masks
 *	32 * (2/3) = 22 used masks
 *	22 masks * 39 usec/mask = .858 microseconds/table
 */

/*
 * We have handcrafted while loops from If's and Goto's, so
 * that if a user of the macro issues a Break, it will break
 * out of the For loop, and not one of the inner While loops
 */

#define NEXTPROC	{ pp++; goto _a ; }

#define FORALLPROC(X) {						\
	register unsigned long *_bp;				\
	register struct proc *pp = proc;			\
	register unsigned long _mask;				\
								\
	/*							\
	 * for the whole index into the table			\
	 */							\
	for ( _bp = proc_bitmap;				\
		_bp < &proc_bitmap[max_proc_index] ; _bp++ ) {	\
		/*						\
		 * If any bits in this longword are used,	\
		 * find the associated structures		\
		 */						\
		if (_mask = *_bp) {				\
			_a: if (_mask) {			\
				_b: if ((_mask&1) == 0) {	\
					_mask = _mask >> 1;	\
					pp++;			\
					goto _b;		\
				}				\
				_mask = _mask >> 1;		\
				{ X }				\
				pp++;				\
				goto _a;			\
			} else {				\
				if (_mask = ((pp-proc)%32))	\
					pp += 32 - _mask;	\
			}					\
		} else pp += 32;				\
	}							\
}
