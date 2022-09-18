#ifndef lint
static	char	*sccsid = "@(#)debug.c	4.2	(ULTRIX)	11/9/90";
#endif lint

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/cpu.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/quota.h"
#include "../h/syscall.h"

/*
 * 13-Oct-89  -- gmm
 *	SMP changes - removed nofault
 */
/*
 * ????
 * ADD DESCRIPTIONS FOR CMAPS
 */

struct reg_values pstat_values[] = {
	{ SSLEEP,		"SLEEP" },
	{ SWAIT,		"WAIT" },
	{ SRUN,			"RUN" },
	{ SIDL,			"IDL" },
	{ SZOMB,		"ZOMB" },
	{ SSTOP,		"STOP" },
	{ 0,			0 }
};

struct reg_values sig_values[] = {
	{ SIGHUP,		"SIGHUP" },
	{ SIGINT,		"SIGINT" },
	{ SIGQUIT,		"SIGQUIT" },
	{ SIGILL,		"SIGILL" },
	{ SIGTRAP,		"SIGTRAP" },
	{ SIGIOT,		"SIGIOT" },
	{ SIGEMT,		"SIGEMT" },
	{ SIGFPE,		"SIGFPE" },
	{ SIGKILL,		"SIGKILL" },
	{ SIGBUS,		"SIGBUS" },
	{ SIGSEGV,		"SIGSEGV" },
	{ SIGSYS,		"SIGSYS" },
	{ SIGPIPE,		"SIGPIPE" },
	{ SIGALRM,		"SIGALRM" },
	{ SIGTERM,		"SIGTERM" },
	{ SIGURG,		"SIGURG" },
	{ SIGSTOP,		"SIGSTOP" },
	{ SIGTSTP,		"SIGTSTP" },
	{ SIGCONT,		"SIGCONT" },
	{ SIGCHLD,		"SIGCHLD" },
	{ SIGTTIN,		"SIGTTIN" },
	{ SIGTTOU,		"SIGTTOU" },
	{ SIGIO,		"SIGIO" },
	{ SIGXCPU,		"SIGXCPU" },
	{ SIGXFSZ,		"SIGXFSZ" },
	{ SIGVTALRM,		"SIGVTALRM" },
	{ SIGPROF,		"SIGPROF" },
	{ 0,			0 },
};

struct reg_values imask_values[] = {
	{ SR_IMASK8,	"8" },
	{ SR_IMASK7,	"7" },
	{ SR_IMASK6,	"6" },
	{ SR_IMASK5,	"5" },
	{ SR_IMASK4,	"4" },
	{ SR_IMASK3,	"3" },
	{ SR_IMASK2,	"2" },
	{ SR_IMASK1,	"1" },
	{ SR_IMASK0,	"0" },
	{ 0,		NULL },
};

struct reg_desc sr_desc[] = {
	/* mask	     shift      name   format  values */
	{ SR_CU3,	0,	"CU3",	NULL,	NULL },
	{ SR_CU2,	0,	"CU2",	NULL,	NULL },
	{ SR_CU1,	0,	"CU1",	NULL,	NULL },
	{ SR_CU0,	0,	"CU0",	NULL,	NULL },
	{ SR_BEV,	0,	"BEV",	NULL,	NULL },
	{ SR_TS,	0,	"TS",	NULL,	NULL },
	{ SR_PE,	0,	"PE",	NULL,	NULL },
	{ SR_CM,	0,	"CM",	NULL,	NULL },
	{ SR_PZ,	0,	"PZ",	NULL,	NULL },
	{ SR_SWC,	0,	"SwC",	NULL,	NULL },
	{ SR_ISC,	0,	"IsC",	NULL,	NULL },
	{ SR_IBIT8,	0,	"IM8",	NULL,	NULL },
	{ SR_IBIT7,	0,	"IM7",	NULL,	NULL },
	{ SR_IBIT6,	0,	"IM6",	NULL,	NULL },
	{ SR_IBIT5,	0,	"IM5",	NULL,	NULL },
	{ SR_IBIT4,	0,	"IM4",	NULL,	NULL },
	{ SR_IBIT3,	0,	"IM3",	NULL,	NULL },
	{ SR_IBIT2,	0,	"IM2",	NULL,	NULL },
	{ SR_IBIT1,	0,	"IM1",	NULL,	NULL },
	{ SR_IMASK,	0,	"IPL",	NULL,	imask_values },
	{ SR_KUO,	0,	"KUo",	NULL,	NULL },
	{ SR_IEO,	0,	"IEo",	NULL,	NULL },
	{ SR_KUP,	0,	"KUp",	NULL,	NULL },
	{ SR_IEP,	0,	"IEp",	NULL,	NULL },
	{ SR_KUC,	0,	"KUc",	NULL,	NULL },
	{ SR_IEC,	0,	"IEc",	NULL,	NULL },
	{ 0,		0,	NULL,	NULL,	NULL },
};

struct reg_values exc_values[] = {
	{ EXC_INT,	"INT" },
	{ EXC_MOD,	"MOD" },
	{ EXC_RMISS,	"RMISS" },
	{ EXC_WMISS,	"WMISS" },
	{ EXC_RADE,	"RADE" },
	{ EXC_WADE,	"WADE" },
	{ EXC_IBE,	"IBE" },
	{ EXC_DBE,	"DBE" },
	{ EXC_SYSCALL,	"SYSCALL" },
	{ EXC_BREAK,	"BREAK" },
	{ EXC_II,	"II" },
	{ EXC_CPU,	"CPU" },
	{ EXC_OV,	"OV" },
	{ SEXC_SEGV,	"SW_SEGV" },
	{ SEXC_RESCHED,	"SW_RESCHED" },
	{ SEXC_PAGEIN,	"SW_PAGEIN" },
	{ SEXC_CPU,	"SW_CP_UNUSABLE" },
	{ 0,		NULL },
};

struct reg_desc exccode_desc[] = {
	/* mask	     shift      name   format  values */
	{ 1,		0,	"USER",	NULL,	NULL },
	{ CAUSE_EXCMASK,0,	"EXC",	NULL,	exc_values },
	{ 0,		0,	NULL,	NULL,	NULL }
};

struct reg_desc cause_desc[] = {
	/* mask	     shift      name   format  values */
	{ CAUSE_BD,	0,	"BD",	NULL,	NULL },
	{ CAUSE_CEMASK,	-CAUSE_CESHIFT,	"CE",	"%d",	NULL },
	{ CAUSE_IP8,	0,	"IP8",	NULL,	NULL },
	{ CAUSE_IP7,	0,	"IP7",	NULL,	NULL },
	{ CAUSE_IP6,	0,	"IP6",	NULL,	NULL },
	{ CAUSE_IP5,	0,	"IP5",	NULL,	NULL },
	{ CAUSE_IP4,	0,	"IP4",	NULL,	NULL },
	{ CAUSE_IP3,	0,	"IP3",	NULL,	NULL },
	{ CAUSE_SW2,	0,	"SW2",	NULL,	NULL },
	{ CAUSE_SW1,	0,	"SW1",	NULL,	NULL },
	{ CAUSE_EXCMASK,0,	"EXC",	NULL,	exc_values },
	{ 0,		0,	NULL,	NULL,	NULL },
};

struct reg_desc tlbhi_desc[] = {
	/* mask	     shift      name   format  values */
	{ TLBHI_VPNMASK,0,	"VA",	"0x%x",	NULL },
	{ TLBHI_PIDMASK,-TLBHI_PIDSHIFT,"PID",	"%d",	NULL },
	{ 0,		0,	NULL,	NULL,	NULL },
};

struct reg_desc tlblo_desc[] = {
	/* mask	     shift      name   format  values */
	{ TLBLO_PFNMASK,0,	"PA",	"0x%x",	NULL },
	{ TLBLO_N,	0,	"N",	NULL,	NULL },
	{ TLBLO_D,	0,	"D",	NULL,	NULL },
	{ TLBLO_V,	0,	"V",	NULL,	NULL },
	{ TLBLO_G,	0,	"G",	NULL,	NULL },
	{ 0,		0,	NULL,	NULL,	NULL },
};

struct reg_desc tlbinx_desc[] = {
	/* mask	     shift      name   format  values */
	{ TLBINX_PROBE,	0,	"PFAIL",NULL,	NULL },
	{ TLBINX_INXMASK, -TLBINX_INXSHIFT, "INDEX", "%d", NULL },
	{ 0,		0,	NULL,	NULL,	NULL },
};

struct reg_desc tlbrand_desc[] = {
	/* mask	     shift      name   format  values */
	{ TLBRAND_RANDMASK, -TLBRAND_RANDSHIFT, "RANDOM", "%d", NULL },
	{ 0,		0,	NULL,	NULL,	NULL },
};

struct reg_desc tlbctxt_desc[] = {
	/* mask	     shift      name   format  values */
	{ TLBCTXT_BASEMASK, 0,	"PTEBASE", "0x%x", NULL },
	{ TLBCTXT_VPNMASK, 11,	"BADVAP", "0x%x", NULL},
	{ 0,		0,	NULL,	NULL,	NULL },
};

struct reg_values fileno_values[] = {
	/* value			name */
	{ PG_FZERO>>PTE_FILENOSHIFT,	"ZERO" },
	{ PG_FTEXT>>PTE_FILENOSHIFT,	"TEXT" },
	{ 0,				NULL }
};

struct reg_values prot_values[] = {
	/* value			name */
	{ PG_KR,			"KR" },
	{ PG_KW,			"KW" },
	{ PG_URKR,			"URKR" },
	{ PG_UW,			"UW" },
	{ 0,				NULL }
};

struct reg_desc pte_desc[] = {
	/* mask	     shift      name   format  values */
	{ PG_PFNUM,	0,	"PADDR", "0x%x", NULL },
	{ PG_N,		0,	"N",	NULL,	NULL },
	{ PG_M,		0,	"M",	NULL,	NULL },
	{ PG_V,		0,	"V",	NULL,	NULL },
	{ PG_G,		0,	"G",	NULL,	NULL },
	{ PG_FILENO,	PTE_FILENOSHIFT,"FILENO", "%d",	fileno_values },
	{ PG_SWAPM,	0,	"SWAPM",NULL,	NULL },
	{ PG_FOD,	0,	"FOD",	NULL,	NULL },
	{ PG_PROT,	0,	"PROT",	NULL,	prot_values },
	{ 0,		0,	NULL,	NULL,	NULL }
};

struct reg_values syscall_values[] = {
	/* value			name */
	{ SYS_syscall,			"syscall" },
	{ SYS_exit,			"exit" },
	{ SYS_fork,			"fork" },
	{ SYS_read,			"read" },
	{ SYS_write,			"write" },
	{ SYS_open,			"open" },
	{ SYS_close,			"close" },
	{ 7,				"old_wait" },
	{ SYS_creat,			"creat" },
	{ SYS_link,			"link" },
	{ SYS_unlink,			"unlink" },
	{ SYS_execv,			"execv" },
	{ SYS_chdir,			"chdir" },
	{ 13,				"old_time" },
	{ SYS_mknod,			"mknod" },
	{ SYS_chmod,			"chmod" },
	{ SYS_chown,			"chown" },
	{ SYS_brk,			"brk" },
	{ 18,				"old_stat" },
	{ SYS_lseek,			"lseek" },
	{ SYS_getpid,			"getpid" },
	{ 21,				"old_mount" },
	{ 22,				"old_umount" },
	{ 23,				"old_setuid" },
	{ SYS_getuid,			"getuid" },
	{ 25,				"old_stime" },
	{ SYS_ptrace,			"ptrace" },
	{ 27,				"old_alarm" },
	{ 28,				"old_fstat" },
	{ 29,				"old_pause" },
	{ 30,				"old_utime" },
	{ 31,				"old_stty" },
	{ 32,				"old_gtty" },
	{ SYS_access,			"access" },
	{ 34,				"old_nice" },
	{ 35,				"old_ftime" },
	{ SYS_sync,			"sync" },
	{ SYS_kill,			"kill" },
	{ SYS_stat,			"stat" },
	{ 39,				"old_setpgrp" },
	{ SYS_lstat,			"lstat" },
	{ SYS_dup,			"dup" },
	{ SYS_pipe,			"pipe" },
	{ 43,				"old_times" },
	{ SYS_profil,			"profil" },
	{ 46,				"old_setgid" },
	{ SYS_getgid,			"getgid" },
	{ 48,				"old_sigsys" },
	{ SYS_acct,			"acct" },
	{ 52,				"old_phys" },
	{ 53,				"old_syslock" },
	{ SYS_ioctl,			"ioctl" },
	{ SYS_reboot,			"reboot" },
	{ 56,				"old_mpxchan" },
	{ SYS_symlink,			"symlink" },
	{ SYS_readlink,			"readlink" },
	{ SYS_execve,			"execve" },
	{ SYS_umask,			"umask" },
	{ SYS_chroot,			"chroot" },
	{ SYS_fstat,			"fstat" },
	{ SYS_getpagesize,		"getpagesize" },
	{ SYS_mremap,			"mremap" },
	{ SYS_vfork,			"vfork" },
	{ 67,				"old_vread" },
	{ 68,				"old_vwrite" },
	{ SYS_sbrk,			"sbrk" },
	{ SYS_sstk,			"sstk" },
	{ SYS_mmap,			"mmap" },
	{ SYS_vadvise,			"vadvise" },
	{ SYS_munmap,			"munmap" },
	{ SYS_mprotect,			"mprotect" },
	{ SYS_madvise,			"madvise" },
	{ SYS_vhangup,			"vhangup" },
	{ 77,				"old_vlimit" },
	{ SYS_mincore,			"mincore" },
	{ SYS_getgroups,		"getgroups" },
	{ SYS_setgroups,		"setgroups" },
	{ SYS_getpgrp,			"getpgrp" },
	{ SYS_setpgrp,			"setpgrp" },
	{ SYS_setitimer,		"setitimer" },
	{ SYS_wait3,			"wait3" },
	{ SYS_swapon,			"swapon" },
	{ SYS_getitimer,		"getitimer" },
	{ SYS_gethostname,		"gethostname" },
	{ SYS_sethostname,		"sethostname" },
	{ SYS_getdtablesize,		"getdtablesize" },
	{ SYS_dup2,			"dup2" },
	{ SYS_getdopt,			"getdopt" },
	{ SYS_fcntl,			"fcntl" },
	{ SYS_select,			"select" },
	{ SYS_setdopt,			"setdopt" },
	{ SYS_fsync,			"fsync" },
	{ SYS_setpriority,		"setpriority" },
	{ SYS_socket,			"socket" },
	{ SYS_connect,			"connect" },
	{ SYS_accept,			"accept" },
	{ SYS_getpriority,		"getpriority" },
	{ SYS_send,			"send" },
	{ SYS_recv,			"recv" },
	{ SYS_sigreturn,		"sigreturn" },
	{ SYS_bind,			"bind" },
	{ SYS_setsockopt,		"setsockopt" },
	{ SYS_listen,			"listen" },
	{ 107,				"was_vtimes" },
	{ SYS_sigvec,			"sigvec" },
	{ SYS_sigblock,			"sigblock" },
	{ SYS_sigsetmask,		"sigsetmask" },
	{ SYS_sigpause,			"sigpause" },
	{ SYS_sigstack,			"sigstack" },
	{ SYS_recvmsg,			"recvmsg" },
	{ SYS_sendmsg,			"sendmsg" },
	{ 115,				"old_vtrace" },
	{ SYS_gettimeofday,		"gettimeofday" },
	{ SYS_getrusage,		"getrusage" },
	{ SYS_getsockopt,		"getsockopt" },
	{ 119,				"old_resuba" },
	{ SYS_readv,			"readv" },
	{ SYS_writev,			"writev" },
	{ SYS_settimeofday,		"settimeofday" },
	{ SYS_fchown,			"fchown" },
	{ SYS_fchmod,			"fchmod" },
	{ SYS_recvfrom,			"recvfrom" },
	{ SYS_setreuid,			"setreuid" },
	{ SYS_setregid,			"setregid" },
	{ SYS_rename,			"rename" },
	{ SYS_truncate,			"truncate" },
	{ SYS_ftruncate,		"ftruncate" },
	{ SYS_flock,			"flock" },
	{ SYS_sendto,			"sendto" },
	{ SYS_shutdown,			"shutdown" },
	{ SYS_socketpair,		"socketpair" },
	{ SYS_mkdir,			"mkdir" },
	{ SYS_rmdir,			"rmdir" },
	{ SYS_utimes,			"utimes" },
	{ SYS_sigcleanup,		"old sigcleanup" },
	{ SYS_adjtime,			"adjtime" },
	{ SYS_getpeername,		"getpeername" },
	{ SYS_gethostid,		"gethostid" },
	{ SYS_sethostid,		"sethostid" },
	{ SYS_getrlimit,		"getrlimit" },
	{ SYS_setrlimit,		"setrlimit" },
	{ SYS_killpg,			"killpg" },
	{ 147,				"unused" },
	{ SYS_setquota,			"setquota" },
	{ SYS_quota,			"quota" },
	{ SYS_getsockname,		"getsockname" },
	{ SYS_sysmips,			"sysmips" },
	{ 152,				"cachectl" },
	{ 153,				"cacheflush" },
	{ 154,				"nfs debug" },
	{ 155,				"unused" },
	{ 156,				"unused" },
	{ 157,				"unused" },
	{ SYS_nfssvc,			"nfs_svc" },
	{ SYS_getdirentries,		"getdirentries" },
	{ 160,				"statfs" },
	{ 161,				"fstatfs" },
	{ 162,				"unmount" },
	{ SYS_async_daemon,		"async_daemon" },
	{ SYS_getfh,			"nfs_getfh" },
	{ SYS_getdomainname,		"getdomainname" },
	{ SYS_setdomainname,		"setdomainname" },
	{ 167,				"old pcfs_mount" },
	{ 168,				"quotactl" },
	{ SYS_exportfs,			"exportfs" },
	{ SYS_mount,			"mount" },
	{ 0,				NULL }
};

struct reg_values sym_values[] = {
	/* value			name */
	{ XPR_CLOCK,			"clock" },
	{ XPR_TLB,			"tlb" },
	{ XPR_INIT,			"init" },
	{ XPR_SCHED,			"sched" },
	{ XPR_PROCESS,			"process" },
	{ XPR_EXEC,			"exec" },
	{ XPR_SYSCALL,			"syscall" },
	{ XPR_TRAP,			"trap" },
	{ XPR_VM,			"vm" },
	{ XPR_SWAP,			"swap" },
	{ XPR_SWTCH,			"swtch" },
	{ XPR_DISK,			"disk" },
	{ XPR_TTY,			"tty" },
	{ XPR_TAPE,			"tape" },
	{ XPR_BIO,			"bio" },
	{ XPR_INTR,			"intr" },
	{ XPR_RMAP,			"rmap" },
	{ XPR_TEXT,			"text" },
	{ XPR_CACHE,			"cache" },
	{ XPR_NFS,			"nfs" },
	{ XPR_RPC,			"rpc" },
	{ XPR_RPC|XPR_NFS,		"nfsrpc" },
	{ XPR_SIGNAL,			"signal" },
	{ XPR_SM,                       "shared memory" },
	{ XPR_TLB|XPR_SCHED|XPR_PROCESS
	   |XPR_EXEC|XPR_SYSCALL|XPR_TRAP
	   |XPR_NOFAULT|XPR_VM|XPR_SWAP
	   |XPR_SWTCH|XPR_RMAP|XPR_TEXT
	   |XPR_SIGNAL,
					"kernel" },
	{ 0,				NULL }
};

struct xprbuf *xprbase, *xprptr, *xprend;
int xprsize=0x10000, xprinitflag;
unsigned xpr_flags = 0;
int askme;
char *index();
unsigned clkticks;
extern struct kernargs kernargs[];


/*
 * xprinit: initialize xprintf buffer.  The minimum buffer size is one
 * page.  If debugging is not enabled, no buffers are allocated.  The
 * first physical page beyond the xprintf buffers is returned.  Xprbufs
 * are assumed to mapped virtual = physical.
 * Note that xpr buffers are uncached.
 */
xprinit(fpage)
{
	register int i, j;
	extern char xpr_addr;

	xprbase = (struct xprbuf *)PHYS_TO_K1((unsigned)ptob(fpage));
	xprptr = xprbase;
	if (!xprsize)
		xprsize = 1*NBPG;
	/* Clear out the buffer */
	fpage += btoc(xprsize);
	xprend = (struct xprbuf *)PHYS_TO_K1(ptob(fpage)) - 1;
#ifndef SABLE
	bzero(xprbase, (char *)xprend - (char *)xprbase);
#endif
	xprsize /= sizeof(struct xprbuf);
	xprinitflag = 1;
	return(fpage);
}

/*
 * Store a pointer (msg) and up to 3 arguments into the circular xpr buffer.
 * msg is assumed to be a pointer into the kernel text segment (which 
 * does not change after boot time) and all arguments are assumed to be 
 * non-pointers.  The printf string processing is delayed until the buffer
 * is dumped.
 */
xprintf(msg, arg1, arg2, arg3, arg4)
char *msg;
{
	register struct xprbuf *xp;
	register int s;
	extern int cur_pid;
	extern int cur_tlbpid;

	if (xprinitflag == 0)
		return;
	s = splhigh();
	xp = xprptr;
	xp->xp_msg = msg;
	xp->xp_arg1 = arg1;
	xp->xp_arg2 = arg2;
	xp->xp_arg3 = arg3;
	xp->xp_arg4 = arg4;
	xp->xp_timestamp = clkticks;
	xp->xp_pid = cur_pid;
	xp->xp_tlbpid = cur_tlbpid;
	if (++xprptr >= xprend)
		xprptr = xprbase;
	splx(s);
}

xprdump()
{
	xprtail(xprsize);
}

/*
 * Dump the current xprbuf.  This is generally called after the system has
 * crashed (from the monitor).  The string processing for xprintf calls
 * is done here via dprintf which doesn't call xprintf.
 */
xprtail(lines)
unsigned lines;
{
	register struct xprbuf *xp;
	register int i;

	if (!xprinitflag)
		dprintf("Buffer not initialized.\n");
	else {
		if (lines > xprsize)
			lines = xprsize;
		xp = xprptr - lines;
		if (xp < xprbase)
			xp += xprsize;
		dprintf("pid / tlb @ ticks: message\n");
		for(i = 0; i < lines; i++) {
			if (xp->xp_msg) {
				dprintf("%d / %x @ %d:\t",
				    xp->xp_pid, xp->xp_tlbpid,
				    xp->xp_timestamp);
				dprintf(xp->xp_msg, xp->xp_arg1, xp->xp_arg2,
					xp->xp_arg3, xp->xp_arg4);
				if (xp->xp_msg[strlen(xp->xp_msg)-1] != '\n')
					dprintf("\n");
			}
			if (++xp >= xprend)
				xp = xprbase;
		}
		dprintf("Done\n");
	}
}

#ifdef notdef
#include "../h/msgbuf.h"
/*
 * dump the contents of the msgbuf from the monitor
 */
msgdump()
{
	register char *msgptr, *msgend;
	register int i, c;

	if (pmsgbuf->msg_magic != MSG_MAGIC) {
		dprintf("msgbuf not initialized\n");
		return;
	}
	dprintf("msgbuf buffer: pmsgbuf=0x%x\n", pmsgbuf);
	msgptr = &(pmsgbuf->msg_bufc[pmsgbuf->msg_bufx]);
	msgend = &(pmsgbuf->msg_bufc[MSG_BSIZE]);
	for(i=0; i < MSG_BSIZE; i++) {
		if (msgptr >= msgend) 
			msgptr = pmsgbuf->msg_bufc;
		if (c = *msgptr++)
			dprintf("%c", c);
	}
	dprintf("Done\n");
}
#endif notdef

assfail(ass, file, line)
char *ass, file;
int line;
{
	printf("ASSERTION %s FAILED in file %s at line %d\n",
	    ass, file, line);
	panic("ASSERTION");
}

/*
 * getargs(argc, argv)
 *
 *  1. loop through the arg list:
 *
 *  1a. if switch is "init="name, name is the file with the image of the
 *	  init process
 *
 *  1b. if switch is "-"x, copy the switch from the boot stack to kernel
 *	  space so it can be passed to init process
 *
 *  1c. if switch is sw"="n, set kernel flag sw to number n; if "="n is
 *	  omitted, set to 1
 *
 */
getargs(argc, argv, environ)
char *argv[];
char *environ[];
{
	register struct kernargs *kp;
	register int i;
	register char *cp;
	register char **argp;

#ifdef ultrix
	argp = 0;	/* environ is junk on ultrix */
	if ((u_int)argv <= (u_int)K1BASE ||
	    (u_int)argv >= (u_int)(K1BASE + 0x20000)) {
		cprintf("getargs: bad argv from prom 0x%x\n",argv);
		return;
	}
#else oldmips
	argp = environ;
#endif ultrix
loop:
	for (; argp && *argp; argp++) {
		if (strncmp("init=", *argp, 5) == 0) {
			extern char icode_file[];

			dprintf("Using %s for /etc/init\n", &(*argp)[5]);
			strcpy(icode_file, &(*argp)[5]);
			continue;
		}
		if (**argp == '-') {
			extern char *icode_args[];
			extern char *icode_argv[];
			extern int icode_argc[];

/*			dprintf("/etc/init arg: %s\n", *argp); */
			cp = *argp;
			/*
			 * Relocate in line here
			 */
			icode_argv[icode_argc[0]++] =
			    (char *)(icode_args[0] - (char *)icode + USRDATA);
			do {
				*icode_args[0]++ = *cp;
			} while (*cp++);
			icode_argv[icode_argc[0]] = 0;
			continue;
		}
		for(kp = kernargs; kp->name; kp++) {
			i = strlen(kp->name);
			if (strncmp(kp->name, *argp, i) == 0) {
				extern char *atob();

				cp = &((*argp)[i]);
				if (*cp == 0)
					*kp->ptr = 1;
				else if (*cp == '=') {
					cp = atob(++cp, kp->ptr);
					if (*cp)
		dprintf("WARNING: Badly formed kernel argument %s\n", *argp);
				} else if (*cp == ':') {
					extern char *symval();
					cp = symval(++cp, kp->ptr);
					if (*cp)
		dprintf("WARNING: Badly formed kernel argument %s\n", *argp);
				} else
					continue;
				dprintf("Kernel argument %s = 0x%x\n",
				    kp->name, *kp->ptr);
				break;
			}
		}
	}
	if (argv) {
		argp = argv+1;	/* skip boot device */
		argv = 0;
		goto loop;
	}
}

/*
 * System call which allows a user to read/write kernel variables specified
 * in the above table by symbolic name.  Only the super-user is allowed to
 * write non-readonly variables.
 */

int
mipskopt(argname, value, op)
	char *argname;
	int value;
	int op;
{
#define MAXKVARNAME 20
	char nbuf[MAXKVARNAME];
	register char *nbp;
	register struct kernargs *kp;
	int error = 0;

	nbp = nbuf;

	if ((error = copyinstr(argname, nbuf, MAXKVARNAME, 0)) != 0) {
		if (error == ENOENT)
			error = EINVAL; /* name too long */
		return(error);
	}

	for (kp = kernargs; kp->name; kp++)
		if (strcmp(kp->name, nbuf) == 0) {
			error = do_opt(kp, value, op);
			return(error);
		}
	
	return(EINVAL);
}

do_opt(kp, val, op)
struct kernargs *kp;
{
	u.u_r.r_val1 = *(kp->ptr);

	if (op != KOPT_GET && (!suser() || kp->readonly))
		return(EACCES);

	switch (op) {

	case KOPT_GET:
		break;

	case KOPT_SET:
		*(kp->ptr) = val;
		break;

	case KOPT_BIS:
		*(kp->ptr) |= val;
		break;

	case KOPT_BIC:
		*(kp->ptr) &= ~val;
		break;

	default:
		return(EINVAL);
	}

	if (kp->func && op != KOPT_GET)
		return((*kp->func)(*(kp->ptr)));
	return(0);
}

/*
 * ?????
 * MOVE SOME OF THIS STUFF TO libc.c and libasm.s
 */
/*
 * digit -- convert the ascii representation of a digit to its
 * binary representation
 */
unsigned
digit(c)
register char c;
{
	unsigned d;

	if (c >= '0' && c <= '9')
		d = c - '0';
	else if (c >= 'a' && c <= 'f')
		d = c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		d = c - 'A' + 10;
	else
		d = 999999; /* larger than any base to break callers loop */
	return(d);
}

char *
symval(cp, iptr)
register char *cp;
int *iptr;
{
	register struct reg_values *rv;
	register char *bp;
	char buf[32];

	*iptr = 0;

	while (cp && *cp) {
		bp = buf;
		while (*cp && *cp != '|' && bp < &buf[sizeof(buf)-1])
			*bp++ = *cp++;
		*bp = 0;
		for (rv = sym_values; rv->rv_name; rv++)
			if (strcmp(buf, rv->rv_name) == 0) {
				*iptr |= rv->rv_value;
				break;
			}
		if (rv->rv_name == NULL)
			printf("unknown symbol: %s\n", buf);
		while (*cp == '|')
			cp++;
	}
	return(cp);
}


/*
 * atob -- convert ascii to binary.  Accepts all C numeric formats.
 */
char *
atob(cp, iptr)
register char *cp;
int *iptr;
{
	int minus = 0;
	register int value = 0;
	unsigned base = 10;
	unsigned d;

	*iptr = 0;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	while (*cp == '-') {
		cp++;
		minus = !minus;
	}

	/*
	 * Determine base by looking at first 2 characters
	 */
	if (*cp == '0') {
		switch (*++cp) {
		case 'X':
		case 'x':
			base = 16;
			cp++;
			break;

		case 'B':	/* a frill: allow binary base */
		case 'b':
			base = 2;
			cp++;
			break;
		
		default:
			base = 8;
			break;
		}
	}

	while ((d = digit(*cp)) < base) {
		value *= base;
		value += d;
		cp++;
	}

	if (minus)
		value = -value;

	*iptr = value;
	return(cp);
}
