/*	@(#)smp_lock.h	4.2	(ULTRIX)	9/4/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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

/* ---------------------------------------------------------------------
 * Modification History 
 *
 * 15-Jun-90 pmk
 *	Added lock_cidevice_d for CIXCD.
 *
 * 14-May-90 thomas
 *	DECwest ANSI 3.5.2.1 dls 029 1990 Jan 24
 *	Change struct lock_t field from unsigned long to unsigned int
 *	since ANSI does not allow unsigned long bitfields.
 *
 * 03-Mar-90 jaw
 *	primitive change to optimize mips.
 *
 * 22-Dec-89 scott
 *	X/Open changes
 *
 * 08-Dec-89 gmm
 *	Declare lock field in lock_t as volatile
 *
 * 09-Nov-89 jaw
 *	add global smp variable.
 *
 *  20-Jul-89 jaw
 *	rearrang lock structure so lock be and wanted field
 *	are in seperate longwords.  clean-up debug code.
 *
 *  16-Jun-89   Pete Keilty
 *	Added msi port smp locks.
 *
 *  12-Jun-89	dws
 *	Added entry for trusted path lock.
 *
 *  12-Jun-89   gg
 *	Add entry for dynamic swap lock.
 *
 *  9-Jun-89	Larry Scott
 *	Added entries for audit locks.
 *
 *  7-Jun-89	Randall Brown
 *	Added entry for pty select lock.
 *
 *  4-May-89	Giles Atkinson
 *	Add entry for LMF data lock.
 *
 * 24-Apr-89 -- jaw 
 *	fix race condition in process tracing between "ptrace" and child
 *	exiting.
 *
 * 06-Apr-89 - prs
 *	Added quota and accounting locks.
 *
 * 06-Apr-89 - Pete Keilty
 *	Added scs/gvp/ci locks LK_GVPBDDB, LK_PB, LK_PCCB, LK_CBVTE, 
 *	LK_SCADB. Also Larry's scsnet locks LK_SCSNETKNOWNSYS, LK_SCSNETSYS,
 *	LK_SCSNETRECVS	
 *
 * 31-Mar-89-us
 *	Added lk_tcpiss to control tcp_iss and tcp_slow_active.  Lowered
 *	heirarchy postitions of lk_tcb/lk_udb to below lk_socket.
 *
 * 27-Mar-89 - us
 *	Lowered ipl on lk_rtentry, lk_ifnet, lk_in_ifaddr as per
 *	lp changes made 3/16/89.  Includes lowering ipl on statistics
 *	locks to 0;
 *
 * 28-Feb-89 - jaw
 *	move SMP_DEBUG and SMP_STAT options into the h file so utilities
 *	can track the size of the lock_t structure.
 *
 * 12-Jan-89 - jaw
 *      lower values are now higher priority in hier postion.
 *
 * 05-Mar-86 -- bjg
 *	Added interlock for error logging
 *
 * 03-Mar-86 -- jrs
 *	Added interlock for cpu utilization table updating
 * 
 * 15 Jul 85 --jrs
 *	Created to hold resource lock related definitions
 * ---------------------------------------------------------------------
 */
/* Lock types, values for lock_t->l_lock */


#ifndef _SMP_LOCKS
#define _SMP_LOCKS

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#if !defined(_POSIX_SOURCE)

#define SMP_DEBUG 	1

#define LK_SPIN	0x1
#define LK_WAIT	0x2

/* flags sent to smp_lock */
#define		LK_RETRY	1
#define		LK_ONCE		0
#define	LK_TYPE_MASK 0x3

#define	LK_LOST		0
#define LK_WON		1
#define LK_FALSE	0
#define LK_TRUE		1
#define MAX_WAIT_COUNT	1000000
#define MAX_SPIN_COUNT	1000000

#define WAKE_ONE	1		/* wake only one process in wakeup */
#define WAKE_ALL	2		/* wake all processes waiting */
#endif /* !defined(_POSIX_SOURCE) */

#ifndef LOCORE

/* it is critical that the first longword is reserved for static data and
 * the lock bit.  Some machines only allow modifying under hardware interlock
 * a full long word. 
 */
struct __lock_t {
	volatile unsigned int
			l_call:8,	/* non-zero if need to call */
			l_hierpos:8,	/* hierarchical position: 0 is high*/
			l_ipl:8,	/* minimum ipl to get lock */
			l_type:7,	/* type of lock */
			l_lock:1;	/* lock bit */
	struct __lock_t	*l_plock;	/* previous lock held */
	char *		l_pc;
	unsigned long 	l_lost; 	/* # of times getting lock failed */
	unsigned long 	l_spin;		/* # of times we spin */
	unsigned long 	l_won;		/* # of times lock was won	*/
	unsigned short 	l_wanted;	/* count of waiting processes */

};

#if !defined(_POSIX_SOURCE)
#define lock_t __lock_t

/* The lock trace structure is included in the per-cpu
   data stucture "cpudata".  The number of trace 
   entries is determined by the value of LK_TRACES */

struct lock_trace {
	long tr_pid;	/* pid of process locking */
	caddr_t tr_pc;	/* pc at which smp_lock was called */
	long tr_psl;	/* psl at getting of lock */
	struct lock_t *tr_lock;	/* address of lock */
};
#define CPU_LK_TRACES	4	/* number of LOCK trace entries for cpudata*/

#define LOCK_TRACE_ELEMENTS 200
struct debug_entry {
		unsigned int spinct;
		caddr_t calling_pc;
		caddr_t holding_pc;
	};
struct debug_lock_trace {
	struct lock_t *traced_lock;
	int current_lock_trace;	
	int debug_lock_entries;
	struct debug_entry element[LOCK_TRACE_ELEMENTS];
};

extern struct debug_lock_trace *debug_lock_trace;

/* data for locks */
struct lock_data {
	char	l_hierpos;
	char	l_type;
	char	l_ipl;
};

#ifdef KERNEL
extern	struct lock_data lock_data[];
extern int smp;
extern int smp_debug;
extern int smp_enable_trace;

/* priorities are now assigned in the file kern_lock_data.c.  Below
   are the externs to the data entry for each type of lock */

extern struct lock_data lock_printf_d;
extern struct lock_data lock_errlog_d;
extern struct lock_data lock_realtimer_d;
extern struct lock_data lock_rq_d;
extern struct lock_data lock_cmap_bio_d;
extern struct lock_data lock_rmap_d;
extern struct lock_data lock_timeout_d;
extern struct lock_data lock_buckets_d;
extern struct lock_data lock_ifqueue_d;
extern struct lock_data lock_unibus_d;
extern struct lock_data lock_cfreelist_d;
extern struct lock_data lock_signal_d;
extern struct lock_data lock_waitchk_d;
extern struct lock_data lock_p_vm_d;
extern struct lock_data lock_text_d;
extern struct lock_data lock_smem_d;
extern struct lock_data lock_cmap_d;
extern struct lock_data lock_net_mgt_d;
extern struct lock_data lock_pid_d;
extern struct lock_data lock_procqs_d;
extern struct lock_data lock_select_d;
extern struct lock_data lock_cku_d;
extern struct lock_data lock_udpdata_d;
extern struct lock_data lock_arptab_d;
extern struct lock_data lock_bio_d;
extern struct lock_data lock_cidevice_d;
extern struct lock_data lock_msimfreeq_d;
extern struct lock_data lock_msidfreeq_d;
extern struct lock_data lock_msicomql_d;
extern struct lock_data lock_msicomqh_d;
extern struct lock_data lock_msixfp_d;
extern struct lock_data lock_msirfp_d;
extern struct lock_data lock_gvpbddb_d;
extern struct lock_data lock_pb_d;
extern struct lock_data lock_pccb_d;
extern struct lock_data lock_cbvte_d;
extern struct lock_data lock_scadb_d;
extern struct lock_data lock_scsnetknownsys_d;
extern struct lock_data lock_scsnetsys_d;
extern struct lock_data lock_scsnetrecvs_d;
extern struct lock_data lock_device15_d;
extern struct lock_data lock_device14_d;
extern struct lock_data lock_pty_sel_d;
extern struct lock_data lock_tty_d;
extern struct lock_data lock_in_ifaddr_d;
extern struct lock_data lock_ifnet_d;
extern struct lock_data lock_rtentry_d;
extern struct lock_data lock_lat_d;
extern struct lock_data lock_latvc_d;
extern struct lock_data lock_evl_d;
extern struct lock_data lock_dli_d;
extern struct lock_data lock_dliline_d;
extern struct lock_data lock_rou_d;
extern struct lock_data lock_ipq_d;
extern struct lock_data lock_nsp_connect_error_d;
extern struct lock_data lock_descrqueue_d;
extern struct lock_data lock_udpstat_d;
extern struct lock_data lock_tcpiss_d;
extern struct lock_data lock_socket_d;
extern struct lock_data lock_tcb_d;
extern struct lock_data lock_udb_d;
extern struct lock_data lock_tcpstat_d;
extern struct lock_data lock_ipstat_d;
extern struct lock_data lock_dna_objects_d;
extern struct lock_data lock_llttab_d;
extern struct lock_data lock_nfsstat_d;
extern struct lock_data lock_frlock_d;
extern struct lock_data lock_cred_d;
extern struct lock_data lock_rpcroute_d;
extern struct lock_data lock_rpcxid_d;
extern struct lock_data lock_rpcrqcred_d;
extern struct lock_data lock_rpccallout_d;
extern struct lock_data lock_rpcdupreq_d;
extern struct lock_data lock_nfschtable_d;
extern struct lock_data lock_nfsargs_d;
extern struct lock_data lock_eachfs_d;
extern struct lock_data lock_gnode_table_d;
extern struct lock_data lock_nfs_biod_d;
extern struct lock_data lock_nfsdnlc_d;
extern struct lock_data lock_namecache_d;
extern struct lock_data lock_file_d;
extern struct lock_data lock_dquotlocks_d;
extern struct lock_data lock_quotalocks_d;
extern struct lock_data lock_dquot_d;
extern struct lock_data lock_quota_d;
extern struct lock_data lock_eachgnode_d;
extern struct lock_data lock_eachfile_d;
extern struct lock_data lock_acct_d;
extern struct lock_data lock_debug_d;
extern struct lock_data lock_msgtxt_d;
extern struct lock_data lock_msg_d;
extern struct lock_data lock_msgq_d;
extern struct lock_data lock_semu_d;
extern struct lock_data lock_sem_d;
extern struct lock_data lock_semq_d;
extern struct lock_data lock_totalswap_d;
extern struct lock_data lock_lmf_d;
extern struct lock_data lock_syscall_trace_d;
extern struct lock_data lock_audbuf_d;
extern struct lock_data lock_audlog_d;
extern struct lock_data lock_auditmask_d;

	/****  LOW priority ****/

/* lock used for run queue */
extern struct lock_t lk_rq;
extern struct lock_t lk_waitchk;
extern struct lock_t lk_procqs;
extern struct lock_t lk_pid;
extern struct lock_t lk_printf;

/* lock for processor realtimer. */
extern struct lock_t lk_realtimer;

/* lock for controlling all p_sig* and p_cursig accesses */
extern struct lock_t lk_signal;

extern struct lock_t lk_timeout;
extern struct lock_t lk_select;
extern struct lock_t lk_debug;

/* Network locks.  */
extern struct lock_t lk_rtentry;
extern struct lock_t lk_net_mgt;
extern struct lock_t lk_ifnet;
extern struct lock_t lk_tcpiss;	/* Controls access to tcp_iss
				 * and tcp_slow_active.
				 * 3.31.89.us  */

/* DECnet locks */
extern struct lock_t lk_llttab;
extern struct lock_t lk_descrqueue;
extern struct lock_t lk_dna_objects;
extern struct lock_t lk_nsp_connect_error;
extern struct lock_t lk_rou;

/* audit locks */
extern struct lock_t lk_audbuf;
extern struct lock_t lk_audlog;
extern struct lock_t lk_auditmask;

/* Trusted path lock */
extern struct lock_t lk_tpath;

/*
 * below statement must be an expression, otherwise
 * the compiler barfs on:  if (smp_lock(lk, LK_ONCE)==1)
 *
if (option==LK_ONCE) smp_lock_once(lk);\
if (option==LK_RETRY) smp_lock_retry(lk); \
 */

#define smp_lock(lk,option) \
(((lk)->l_call) ? ((option == LK_ONCE) ? smp_lock_once(lk) : smp_lock_retry(lk)) : 1)

#ifdef __vax
#define smp_unlock(lk) \
if ((lk)->l_call) smp_unlock_short(lk)
#else 
#define smp_unlock(lk) \
if ((lk)->l_call) smp_unlock_long(lk)
#endif
		
#endif /* KERNEL */
#endif /* !defined(_POSIX_SOURCE) */
#endif /* LOCORE */
#endif /* _SMP_LOCKS */
