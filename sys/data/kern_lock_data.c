#ifndef lint
static	char	*sccsid = "@(#)kern_lock_data.c	4.6	(ULTRIX)	2/21/91";
#endif 

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
/************************************************************************
 *
 *		Modification History
 *
 * 20-Feb-91 jaw
 *	fix mp locking for unix domain sockets.
 *
 * 18-Sep-90 Ursula Sinkewicz
 *	Added locks for X.25.
 *
 * 20-Aug-90 Matt Thomas
 *	Add locks for DECnet-ULTRIX Phase V.
 *
 * 15-Jun-90 pmk
 *	Added cidevice lock for CIXCD.
 *
 * 22-May-90 jaw
 *	move timeout above lk_rq.
 *
 * 03-Apr-90 gmm
 *	Added lk_kmtbsync
 *
 * 23-Mar-90 us
 *	Added LK_RAWCB to handle raw ip packets.
 * 04-Oct-89 jaw
 *	add include for systrace.h to pick up dependency for systrace option.
 *
 * 03-Aug-89 scott
 *	correct ipl level for audit locks
 *
 * 20-Jul-89 jaw
 *	make lock trace configurable and move lk_waitchk priority 
 * 	below procqs.
 *
 *  19-Jul-89   scott
 *	use wait lock for LK_AUDLOG
 *
 *  24-Jun-89   gg
 *	moved dynamic swap lock to fix "smp_lock: lock position messup
 *	panic".
 *
 *  16-Jun-89   Pete Keilty
 *	Added msi port smp locks.
 *
 *  12-Jun-89	dws
 *	Added entry for trusted path.
 *
 *  12-Jun-89   gg
 *	Added entry for dynamic swap lock.
 *
 *  9-Jun-89    Larry Scott
 *      Add entries for audit locks
 *
 *  10-May-89	Ed Ferris
 *	Change hierarchy of dli line entry lock.
 *
 *  8-May-89	Giles Atkinson
 *	Add entries for LMF data locks
 *
 * 24-Apr-89 -- jaw 
 *	fix race condition in process tracing between "ptrace" and child
 *	exiting.
 *
 *  06-Apr-89  Paul Shaughnessy
 *	Added quota and accounting locks.
 *
 *  06-Apr-89  Pete Keilty
 *	Added scs/gvp/ci locks, also Larry's scsnet locks
 *
 ************************************************************************/
#include "../h/types.h"
#include "../machine/cpu.h"
#include "../h/smp_lock.h"

#ifdef SMP_DEBUG
int smp_debug = 0;
#else	SMP_DEBUG
int smp_debug = 0;
#endif SMP_DEBUG
#include "sys_trace.h"

int smp_enable_trace=0;
struct lock_t lk_select;
struct lock_t lk_realtimer;
struct lock_t lk_rq;
struct lock_t lk_waitchk;
struct lock_t lk_procqs;
struct lock_t lk_pid;
struct lock_t lk_signal;
struct lock_t lk_llttab;
struct lock_t lk_descrqueue;
struct lock_t lk_dnet_intimeo;
struct lock_t lk_nsp_connect_error;
struct lock_t lk_rtentry;
struct lock_t lk_net_mgt;
struct lock_t lk_printf;
struct lock_t lk_audbuf;
struct lock_t lk_audlog;
struct lock_t lk_auditmask;
struct lock_t lk_ifnet;
struct lock_t lk_tcpiss; 
struct lock_t lk_tpath;



/* Below are the defines for the lock_data l_hierpos field.  When
   adding a lock, insert the new lock name in list in order
   of lock priority.  List is from HIGH to low priority lock. 
   Locks must be acquired in order of LOW to HIGH priority.   */

	/**** HIGH priority ****/

#define	LK_HIGHEST_LOCK_PRIORITY  0
#define LK_PRINTF		LK_HIGHEST_LOCK_PRIORITY
#define LK_ERRLOG		LK_PRINTF+1
#define LK_TIMEOUT		LK_ERRLOG+1
#define LK_REALTIMER		LK_TIMEOUT+1
#define LK_RQ			LK_REALTIMER+1
#define LK_CMAP_BIO		LK_RQ+1
#define LK_RMAP			LK_CMAP_BIO+1
#define	LK_KMTBSYNC		LK_RMAP+1
#define LK_UTCTIME		LK_KMTBSYNC+1
#define LK_DNAUID		LK_UTCTIME+1
#define LK_BUCKETS		LK_DNAUID+1
#define LK_UNIBUS		LK_BUCKETS+1
#define LK_CFREELIST	        LK_UNIBUS+1
#define LK_SIGNAL		LK_CFREELIST+1
#define LK_TOTALSWAP		LK_SIGNAL+1
#define LK_P_VM			LK_TOTALSWAP+1
#define LK_TEXT			LK_P_VM+1
#define LK_SMEM			LK_TEXT+1
#define LK_CMAP			LK_SMEM+1
#define LK_IFQUEUE		LK_CMAP+1
#define LK_NET_MGT		LK_IFQUEUE+1
#define LK_PID			LK_NET_MGT+1
#define LK_PROCQS		LK_PID+1
#define LK_WAITCHK		LK_PROCQS+1
#define LK_SELECT		LK_WAITCHK+1
#define	LK_CKU			LK_SELECT+1
#define	LK_UDPDATA		LK_CKU+1
#define LK_ARPTAB		LK_UDPDATA+1
#define LK_PFILT		LK_ARPTAB+1
#define LK_BIO			LK_PFILT+1
#define LK_CIDEVICE		LK_BIO+1
#define LK_MSIMFREEQ		LK_CIDEVICE+1
#define LK_MSIDFREEQ		LK_MSIMFREEQ+1
#define LK_MSICOMQL		LK_MSIDFREEQ+1
#define LK_MSICOMQH		LK_MSICOMQL+1
#define LK_MSIXFP		LK_MSICOMQH+1
#define LK_MSIRFP		LK_MSIXFP+1
#define LK_GVPBDDB		LK_MSIRFP+1
#define LK_PB			LK_GVPBDDB+1
#define LK_PCCB			LK_PB+1
#define LK_CBVTE		LK_PCCB+1
#define LK_SCADB		LK_CBVTE+1
#define LK_SCSNETKNOWNSYS	LK_SCADB+1
#define LK_SCSNETSYS		LK_SCSNETKNOWNSYS+1
#define LK_SCSNETRECVS		LK_SCSNETSYS+1
#define LK_DEVICE15		LK_SCSNETRECVS+1
#define LK_DEVICE14		LK_DEVICE15+1
#define	LK_TPATH		LK_DEVICE14+1
#define LK_PTY_SEL		LK_TPATH+1
#define LK_TTY			LK_PTY_SEL+1
#define LK_IN_IFADDR		LK_TTY+1
#define LK_IFNET		LK_IN_IFADDR+1
#define LK_RTENTRY		LK_IFNET+1
#define	LK_GWSCREEN		LK_RTENTRY+1
#define LK_LAT			LK_GWSCREEN+1
#define LK_LATVC		LK_LAT+1
#define LK_DLI			LK_LATVC+1
#define LK_ROU			LK_DLI+1
#define LK_ROUCACHE		LK_ROU+1
#define LK_IPQ			LK_ROUCACHE+1
#define LK_NSP_CONNECT_ERROR	LK_IPQ+1
#define LK_DNASC		LK_NSP_CONNECT_ERROR+1
#define LK_DESCRQUEUE		LK_DNASC+1
#define LK_NSPRSP		LK_DESCRQUEUE+1
#define LK_X25			LK_NSPRSP+1
#define LK_TPOSIRSP		LK_X25+1
#define LK_UDPSTAT		LK_TPOSIRSP+1
#define LK_TCPISS		LK_UDPSTAT+1
#define LK_SOCKET		LK_TCPISS+1
#define LK_EVL			LK_SOCKET+1
#define LK_TPOSI		LK_EVL+1
#define LK_CLTS			LK_TPOSI+1
#define LK_CTF			LK_CLTS+1
#define LK_CML			LK_CTF+1
#define LK_SO_DISCONNECT	LK_CML+1
#define LK_TCB			LK_SO_DISCONNECT+1
#define LK_UDB			LK_TCB+1
#define LK_RAWCB		LK_UDB+1
#define LK_DLILINE		LK_RAWCB+1
#define LK_DNA_OBJECTS		LK_DLILINE+1
#define LK_NSP			LK_DNA_OBJECTS+1
#define LK_LLTTAB		LK_NSP+1
#define LK_SYSCALL_TRACE	LK_LLTTAB+1
#define LK_AUDBUF		LK_SYSCALL_TRACE+1
#define LK_AUDLOG		LK_AUDBUF+1
#define LK_AUDITMASK		LK_AUDLOG+1
#define LK_TCPSTAT		LK_AUDITMASK+1
#define LK_IPSTAT		LK_TCPSTAT+1
#define LK_NFSSTAT		LK_IPSTAT+1
#define	LK_FRLOCK		LK_NFSSTAT+1
#define	LK_CRED			LK_FRLOCK+1
#define	LK_RPCROUTE		LK_CRED+1
#define	LK_RPCXID		LK_RPCROUTE+1
#define	LK_RPCRQCRED		LK_RPCXID+1
#define	LK_RPCCALLOUT		LK_RPCRQCRED+1
#define	LK_RPCDUPREQ		LK_RPCCALLOUT+1
#define LK_NFSCHTABLE		LK_RPCDUPREQ+1
#define LK_NFSARGS		LK_NFSCHTABLE+1
#define LK_EACHFS		LK_NFSARGS+1
#define LK_GNODE_TABLE          LK_EACHFS+1
#define LK_NFSBIOD		LK_GNODE_TABLE+1
#define LK_NFSDNLC		LK_NFSBIOD+1
#define LK_NAMECACHE		LK_NFSDNLC+1
#define LK_FILE			LK_NAMECACHE+1
#define LK_DQUOTLOCKS           LK_FILE+1
#define LK_QUOTALOCKS           LK_DQUOTLOCKS+1
#define LK_DQUOT                LK_QUOTALOCKS+1
#define LK_QUOTA                LK_DQUOT+1
#define LK_EACHGNODE		LK_QUOTA+1
#define LK_EACHFILE		LK_EACHGNODE+1
#define LK_ACCT			LK_EACHFILE+1
#define LK_DEBUG		LK_ACCT+1
#define LK_MSGTXT		LK_DEBUG+1
#define LK_MSG			LK_MSGTXT+1
#define LK_MSGQ			LK_MSG+1
#define LK_SEMU			LK_MSGQ+1
#define LK_SEM			LK_SEMU+1
#define LK_SEMQ			LK_SEM+1
#define LK_LMF			LK_SEMQ+1

	/****  LOW priority ****/

#define LD	struct lock_data 

LD lock_printf_d 	={LK_PRINTF,	LK_SPIN,SPLEXTREME};
LD lock_errlog_d 	={LK_ERRLOG,	LK_SPIN,SPLEXTREME};
LD lock_timeout_d 	={LK_TIMEOUT,	LK_SPIN,SPLCLOCK};
LD lock_realtimer_d 	={LK_REALTIMER,	LK_SPIN,SPLCLOCK};
LD lock_rq_d 		={LK_RQ,	LK_SPIN,SPLCLOCK};
LD lock_cmap_bio_d 	={LK_CMAP_BIO,	LK_SPIN,SPLCLOCK};
LD lock_rmap_d 		={LK_RMAP,	LK_SPIN,SPLCLOCK};
LD lock_kmtbsync_d	={LK_KMTBSYNC,	LK_SPIN,SPLCLOCK};
LD lock_utctime_d 	={LK_UTCTIME,	LK_SPIN,SPLCLOCK};
LD lock_dnauid_d 	={LK_DNAUID,	LK_SPIN,SPLIMP};
LD lock_buckets_d 	={LK_BUCKETS,	LK_SPIN,SPLIMP};
LD lock_unibus_d 	={LK_UNIBUS,	LK_SPIN,SPLDEVHIGH};
LD lock_cfreelist_d 	={LK_CFREELIST,	LK_SPIN,SPLDEVHIGH};
LD lock_signal_d 	={LK_SIGNAL,	LK_SPIN,SPLDEVHIGH};
LD lock_totalswap_d 	={LK_TOTALSWAP,	LK_SPIN,SPLDEVHIGH};
LD lock_p_vm_d 		={LK_P_VM,	LK_SPIN,SPLDEVHIGH};
LD lock_text_d 		={LK_TEXT,	LK_SPIN,SPLDEVHIGH};
LD lock_smem_d 		={LK_SMEM,	LK_SPIN,SPLDEVHIGH};
LD lock_cmap_d 		={LK_CMAP,	LK_SPIN,SPLDEVHIGH};
LD lock_ifqueue_d 	={LK_IFQUEUE,	LK_SPIN,SPLIMP};
LD lock_net_mgt_d 	={LK_NET_MGT,	LK_SPIN,SPLIMP};
LD lock_pid_d 		={LK_PID,	LK_SPIN,SPLDEVHIGH};
LD lock_procqs_d 	={LK_PROCQS,	LK_SPIN,SPLDEVHIGH};
LD lock_waitchk_d 	={LK_WAITCHK,	LK_SPIN,SPLDEVHIGH};
LD lock_select_d 	={LK_SELECT,	LK_SPIN,SPLDEVHIGH};
LD lock_cku_d 		={LK_CKU,	LK_SPIN,SPLIMP};
LD lock_udpdata_d 	={LK_UDPDATA,	LK_SPIN,SPLIMP};
LD lock_arptab_d 	={LK_ARPTAB,	LK_SPIN,SPLIMP};
LD lock_pfilt_d 	={LK_PFILT,	LK_SPIN,SPLIMP};
LD lock_bio_d 		={LK_BIO,	LK_SPIN,SPLBIO};
LD lock_cidevice_d 	={LK_CIDEVICE,	LK_SPIN,SPLBIO};
LD lock_msimfreeq_d 	={LK_MSIMFREEQ,	LK_SPIN,SPLBIO};
LD lock_msidfreeq_d 	={LK_MSIDFREEQ,	LK_SPIN,SPLBIO};
LD lock_msicomql_d 	={LK_MSICOMQL,	LK_SPIN,SPLBIO};
LD lock_msicomqh_d 	={LK_MSICOMQH,	LK_SPIN,SPLBIO};
LD lock_msixfp_d  	={LK_MSIXFP,	LK_SPIN,SPLBIO};
LD lock_msirfp_d  	={LK_MSIRFP,	LK_SPIN,SPLBIO};
LD lock_gvpbddb_d 	={LK_GVPBDDB,	LK_SPIN,SPLBIO};
LD lock_pb_d 		={LK_PB,	LK_SPIN,SPLBIO};
LD lock_pccb_d 		={LK_PCCB,	LK_SPIN,SPLBIO};
LD lock_cbvte_d 	={LK_CBVTE,	LK_SPIN,SPLBIO};
LD lock_scadb_d 	={LK_SCADB,	LK_SPIN,SPLBIO};
LD lock_scsnetknownsys_d={LK_SCSNETKNOWNSYS,LK_SPIN,SPLBIO};
LD lock_scsnetsys_d 	={LK_SCSNETSYS,	LK_SPIN,SPLBIO};
LD lock_scsnetrecvs_d 	={LK_SCSNETRECVS,LK_SPIN,SPLBIO};
LD lock_device15_d 	={LK_DEVICE15,	LK_SPIN,SPLBIO};
LD lock_device14_d 	={LK_DEVICE14,	LK_SPIN,SPLBIO-1};
LD lock_tpath_d		={LK_TPATH,	LK_SPIN,SPLTTY};
LD lock_pty_sel_d	={LK_PTY_SEL,	LK_SPIN,SPLTTY};
LD lock_tty_d 		={LK_TTY,	LK_SPIN,SPLTTY};
LD lock_in_ifaddr_d 	={LK_IN_IFADDR,	LK_SPIN,SPLNET};
LD lock_ifnet_d 	={LK_IFNET,	LK_SPIN,SPLNET};
LD lock_rtentry_d 	={LK_RTENTRY,	LK_SPIN,SPLNET};
LD lock_gwscreen_d 	={LK_GWSCREEN,	LK_SPIN,SPLIMP};
LD lock_lat_d 		={LK_LAT,	LK_SPIN,SPLNET};
LD lock_latvc_d 	={LK_LATVC,	LK_SPIN,SPLNET};
LD lock_dli_d 		={LK_DLI,	LK_SPIN,SPLNET};
LD lock_rou_d 		={LK_ROU,	LK_SPIN,SPLNET};
LD lock_roucache_d	={LK_ROUCACHE,	LK_SPIN,SPLNET};
LD lock_ipq_d 		={LK_IPQ,	LK_SPIN,SPLNET};
LD lock_nsp_connect_error_d ={LK_NSP_CONNECT_ERROR,LK_SPIN,SPLNET};
LD lock_dnasc_d		={LK_DNASC,	LK_SPIN,SPLNET};
LD lock_descrqueue_d 	={LK_DESCRQUEUE,LK_SPIN,SPLNET};
LD lock_nsprsp_d 	={LK_NSPRSP,	LK_SPIN,SPLNET};
LD lock_tposirsp_d 	={LK_TPOSIRSP,	LK_SPIN,SPLNET};
LD lock_udpstat_d 	={LK_UDPSTAT,	LK_SPIN,SPLNET};
LD lock_tcpiss_d 	={LK_TCPISS,	LK_SPIN,SPLNET};
LD lock_socket_d 	={LK_SOCKET,	LK_SPIN,SPLNET};
LD lock_evl_d 		={LK_EVL,	LK_SPIN,SPLNET};
LD lock_tposi_d		={LK_TPOSI,	LK_SPIN,SPLNET};
LD lock_clts_d 		={LK_CLTS,	LK_SPIN,SPLNET};
LD lock_ctf_d 		={LK_CTF,	LK_SPIN,SPLNET};
LD lock_cml_d 		={LK_CML,	LK_SPIN,SPLNET};
LD lock_so_disconnect_d	={LK_SO_DISCONNECT,LK_SPIN,SPLNET};
LD lock_tcb_d 		={LK_TCB,	LK_SPIN,SPLNET};
LD lock_udb_d 		={LK_UDB,	LK_SPIN,SPLNET};
LD lock_rawcb_d		={LK_RAWCB,	LK_SPIN,SPLNET};
LD lock_dliline_d 	={LK_DLILINE,	LK_SPIN,SPLNET};
LD lock_dna_objects_d 	={LK_DNA_OBJECTS,LK_SPIN,SPLNET};
LD lock_llttab_d 	={LK_LLTTAB,	LK_SPIN,SPLNET};
LD lock_nsp_d	 	={LK_NSP,	LK_SPIN,SPLNET};
LD lock_x25_d	 	={LK_X25,	LK_SPIN,SPLNET};
LD lock_syscall_trace_d	={LK_SYSCALL_TRACE,LK_SPIN,SPLNONE};
LD lock_audbuf_d	={LK_AUDBUF,	LK_SPIN,SPLNONE};
LD lock_audlog_d	={LK_AUDLOG,	LK_WAIT,SPLNONE};
LD lock_auditmask_d	={LK_AUDITMASK, LK_SPIN,SPLNONE};
LD lock_tcpstat_d 	={LK_TCPSTAT,	LK_SPIN,SPLNONE};
LD lock_ipstat_d 	={LK_IPSTAT,	LK_SPIN,SPLNONE};
LD lock_nfsstat_d 	={LK_NFSSTAT,	LK_SPIN,SPLNONE};
LD lock_frlock_d 	={LK_FRLOCK,	LK_SPIN,SPLNONE};
LD lock_cred_d 		={LK_CRED,	LK_SPIN,SPLNONE};
LD lock_rpcroute_d 	={LK_RPCROUTE,	LK_SPIN,SPLNONE};
LD lock_rpcxid_d 	={LK_RPCXID,	LK_SPIN,SPLNONE};
LD lock_rpcrqcred_d 	={LK_RPCRQCRED,	LK_SPIN,SPLNONE};
LD lock_rpccallout_d 	={LK_RPCCALLOUT,LK_SPIN,SPLNONE};
LD lock_rpcdupreq_d 	={LK_RPCDUPREQ,	LK_SPIN,SPLNONE};
LD lock_nfschtable_d 	={LK_NFSCHTABLE,LK_SPIN,SPLNONE};
LD lock_nfsargs_d 	={LK_NFSARGS,	LK_SPIN,SPLNONE};
LD lock_eachfs_d 	={LK_EACHFS,	LK_SPIN,SPLNONE};
LD lock_gnode_table_d 	={LK_GNODE_TABLE,LK_SPIN,SPLNONE};
LD lock_nfs_biod_d 	={LK_NFSBIOD,	LK_SPIN,SPLNONE};
LD lock_nfsdnlc_d 	={LK_NFSDNLC,	LK_SPIN,SPLNONE};
LD lock_namecache_d 	={LK_NAMECACHE,	LK_SPIN,SPLNONE};
LD lock_file_d 		={LK_FILE,	LK_SPIN,SPLNONE};
LD lock_dquotlocks_d 	={LK_DQUOTLOCKS,LK_SPIN,SPLNONE};
LD lock_quotalocks_d 	={LK_QUOTALOCKS,LK_SPIN,SPLNONE};
LD lock_dquot_d 	={LK_DQUOT,	LK_SPIN,SPLNONE};
LD lock_quota_d 	={LK_QUOTA,	LK_SPIN,SPLNONE};
LD lock_eachgnode_d 	={LK_EACHGNODE,	LK_WAIT,SPLNONE};
LD lock_eachfile_d	={LK_EACHFILE,	LK_WAIT,SPLNONE};
LD lock_acct_d 		={LK_ACCT,	LK_WAIT,SPLNONE};
LD lock_debug_d 	={LK_DEBUG,	LK_SPIN,SPLNONE};
LD lock_msgtxt_d 	={LK_MSGTXT,	LK_SPIN,SPLNONE};
LD lock_msg_d 		={LK_MSG,	LK_WAIT,SPLNONE};
LD lock_msgq_d 		={LK_MSGQ,	LK_WAIT,SPLNONE};
LD lock_semu_d 		={LK_SEMU,	LK_WAIT,SPLNONE};
LD lock_sem_d 		={LK_SEM,	LK_WAIT,SPLNONE};
LD lock_semq_d 		={LK_SEMQ,	LK_WAIT,SPLNONE};
LD lock_lmf_d 		={LK_LMF,	LK_WAIT,SPLNONE};


#ifdef SYS_TRACE
struct lock_t lk_systrace;
#endif SYS_TRACE


#define LOCK_TRACE
/* tracing mechnism for looking a spin counts for a single lock */
#ifdef LOCK_TRACE
struct debug_lock_trace lock_trace;
struct debug_lock_trace *debug_lock_trace = &lock_trace;
#else LOCK_TRACE
struct debug_lock_trace *debug_lock_trace = 0;
#endif LOCK_TRACE


options_lock_init() {
#include "inet.h"
#ifdef INET > 0
extern struct lock_t lk_in_ifaddr;
	lockinit(&lk_in_ifaddr, &lock_in_ifaddr_d);

#endif INET

#ifdef SYS_TRACE
lockinit(&lk_systrace,&lock_syscall_trace_d);
#endif SYS_TRACE

#ifdef LOCK_TRACE
	debug_lock_trace->traced_lock = &lk_rq;
	debug_lock_trace->debug_lock_entries = LOCK_TRACE_ELEMENTS;
#endif LOCK_TRACE

#ifdef PACKETFILTER
    {
	extern struct lock_t lk_pfilt;
	lockinit(&lk_pfilt, &lock_pfilt_d);
    }
#endif PACKETFILTER

#include "gwscreen.h"
#if	NGWSCREEN > 0
    {
	extern struct lock_t lk_gwscreen;
	lockinit(&lk_gwscreen, &lock_gwscreen_d);
    }
#endif	NGWSCREEN

}
