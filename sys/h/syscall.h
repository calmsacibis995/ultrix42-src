/*	@(#)syscall.h	4.3	(ULTRIX)	9/4/90					*/
/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987 by			*
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
 *			Modification History				*
 *									*
 *	Matt Thomas	20 Aug 90					*
 *	Add utc_{get,adj}time using same syscall numbers as OSF/1	*
 *									*
 *      Joe Szczypek 10 Oct 89                                          *
 *      add atomic_op for mips                                          *
 *									*
 *	Joe Amato 21 Jul 89 						*
 *	turn on cachectl/cacheflush for mips				*
 *									*
 *      RR, LP, RSP --- first attempt at a merge....not the best but 	*
 *	workable for now						*
 *									*
 *	Larry Scott	9 Jun 89					*
 *	Added audcntl() and audgen()					*
 *									*
 *	Chet Juszczak	9 Mar 88					*
 *	Added getsysinfo() and setsysinfo()				*
 *									*
 *	Mark Parenti	28 Sep 87					*
 *	Added sigpending						*
 *									*
 *	Suzanne Logcher 18 Mar 87					*
 *	Added exportfs							*
 *									*
 *	Chet Juszczak 5 Aug 86						*
 *	Removed nfs_mount, renamed nfs_biod.				*
 *									*
 *	Jeff Chase 12 June 86						*
 *	Added getdomainname and setdomainname system calls		*
 *									*
 *	Greg Depp 25 Jun 85						*
 *	Added balance of System V system calls.  Moved "shmsys" to avoid*
 *	conflict with Berkeley.  The three new ones are place holders,	*
 *	as they do not currently exist.	 The numbers 163-170 are	*
 *	reserved by Berkeley for Ultrix-32.				*
 *									*
 *	David L Ballenger, 8-Mar-1985					*
 * 0001	Add IPC system calls from System V.				*
 *									*
 ************************************************************************/

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef __mips
#define SYS_syscall     0
#endif /* __mips */
#define	SYS_exit		1
#define	SYS_fork		2
#define	SYS_read		3
#define	SYS_write		4
#define	SYS_open		5
#define	SYS_close		6
							/*  7 is old: wait */
#define	SYS_creat		8
#define	SYS_link		9
#define	SYS_unlink		10
#define	SYS_execv		11
#define	SYS_chdir		12
							/* 13 is old: time */
#define	SYS_mknod		14
#define	SYS_chmod		15
#define	SYS_chown		16
#ifdef __mips
#define	SYS_brk			17			/* 17 is old: sbreak */
#else
							/* 17 is old: sbreak */
#endif /* __mips */
							/* 18 is old: stat */
#define	SYS_lseek		19
#define	SYS_getpid		20
#define	SYS_mount		21
#define	SYS_umount		22
							/* 23 is old: setuid */
#define	SYS_getuid		24
							/* 25 is old: stime */
#define	SYS_ptrace		26
							/* 27 is old: alarm */
							/* 28 is old: fstat */
							/* 29 is old: pause */
							/* 30 is old: utime */
							/* 31 is old: stty */
							/* 32 is old: gtty */
#define	SYS_access		33
							/* 34 is old: nice */
							/* 35 is old: ftime */
#define	SYS_sync		36
#define	SYS_kill		37
#define	SYS_stat		38
							/* 39 is old: setpgrp */
#define	SYS_lstat		40
#define	SYS_dup			41
#define	SYS_pipe		42
							/* 43 is old: times */
#define	SYS_profil		44
							/* 45 is unused */
							/* 46 is old: setgid */
#define	SYS_getgid		47
							/* 48 is old: sigsys */
							/* 49 is unused */
							/* 50 is unused */
#define	SYS_acct		51
							/* 52 is old: phys */
							/* 53 is old: syslock */
#define	SYS_ioctl		54
#define	SYS_reboot		55
							/* 56 is old: mpxchan */
#define	SYS_symlink		57
#define	SYS_readlink		58
#define	SYS_execve		59
#define	SYS_umask		60
#define	SYS_chroot		61
#define	SYS_fstat		62
							/* 63 is unused */
#define	SYS_getpagesize 64
#define	SYS_mremap		65
#ifdef __mips
#define SYS_vfork		66			/* 66 is old: vfork */
#else /* !mips */
							/* 66 is old: vfork */
#endif /* __mips */
							/* 67 is old: vread */
							/* 68 is old: vwrite */
#define	SYS_sbrk		69
#define	SYS_sstk		70
#define	SYS_mmap		71
#ifdef __mips
#define SYS_vadvise		72			/* 72 is old: vadvise */
#else /* !mips */
							/* 72 is old: vadvise */
#endif /* __mips */
#define	SYS_munmap		73
#define	SYS_mprotect		74
#define	SYS_madvise		75
#define	SYS_vhangup		76
							/* 77 is old: vlimit */
#define	SYS_mincore		78
#define	SYS_getgroups		79
#define	SYS_setgroups		80
#define	SYS_getpgrp		81
#define	SYS_setpgrp		82
#define	SYS_setitimer		83
#define	SYS_wait3		84
#define	SYS_wait		SYS_wait3
#define	SYS_swapon		85
#define	SYS_getitimer		86
#define	SYS_gethostname		87
#define	SYS_sethostname		88
#define	SYS_getdtablesize	89
#define	SYS_dup2		90
#define	SYS_getdopt		91
#define	SYS_fcntl		92
#define	SYS_select		93
#define	SYS_setdopt		94
#define	SYS_fsync		95
#define	SYS_setpriority		96
#define	SYS_socket		97
#define	SYS_connect		98
#define	SYS_accept		99
#define	SYS_getpriority		100
#define	SYS_send		101
#define	SYS_recv		102
#ifdef __mips
#define SYS_sigreturn		103			/* new sigreturn */
#else /* !mips */
							/* 103 was socketaddr */
#endif /* __mips */
#define	SYS_bind		104
#define	SYS_setsockopt		105
#define	SYS_listen		106
							/* 107 was vtimes */
#define	SYS_sigvec		108
#define	SYS_sigblock		109
#define	SYS_sigsetmask		110
#define	SYS_sigpause		111
#define	SYS_sigstack		112
#define	SYS_recvmsg		113
#define	SYS_sendmsg		114
							/* 115 is old vtrace */
#define	SYS_gettimeofday	116
#define	SYS_getrusage		117
#define	SYS_getsockopt		118
							/* 119 is old resuba */
#define	SYS_readv		120
#define	SYS_writev		121
#define	SYS_settimeofday	122
#define	SYS_fchown		123
#define	SYS_fchmod		124
#define	SYS_recvfrom		125
#define	SYS_setreuid		126
#define	SYS_setregid		127
#define	SYS_rename		128
#define	SYS_truncate		129
#define	SYS_ftruncate		130
#define	SYS_flock		131
							/* 132 is unused */
#define	SYS_sendto		133
#define	SYS_shutdown		134
#define	SYS_socketpair		135
#define	SYS_mkdir		136
#define	SYS_rmdir		137
#define	SYS_utimes		138
#ifdef __mips
#define SYS_sigcleanup  139     /* From 4.2 longjmp; same as SYS_sigreturn */
#else /* !mips */
							/* 139 is unused */
#endif /* __mips */
#define	SYS_adjtime		140
#define	SYS_getpeername		141
#define	SYS_gethostid		142
#define	SYS_sethostid		143
#define	SYS_getrlimit		144
#define	SYS_setrlimit		145
#define	SYS_killpg		146
							/* 147 is unused */
#define	SYS_setquota		148
#define	SYS_quota		149
#define	SYS_getsockname		150

#ifdef __mips
#define SYS_sysmips     151	/* for floating point control */

/*
 * mips local system calls
 */

#define SYS_cacheflush  152
#define SYS_cachectl    153
#define SYS_atomic_op   155

#ifdef notdef		/* __ULTRIX MAY OR MAY NOT DO THESE */
/*
 * nfs releated system calls
 */
#define SYS_debug       154

#define SYS_statfs      160
#define SYS_fstatfs     161
#define SYS_unmount     162

#define SYS_quotactl    168
#define SYS_mount       170

/*
 * mips local system calls (continued)
 */

#define SYS_hdwconf     171
#endif /* notdef */

/* try to keep binary compatibility with mips */

#define SYS_nfs_svc		158
#define SYS_nfssvc		158 /* cruft - delete when kernel fixed */
#define SYS_nfs_biod		163
#define SYS_async_daemon	163 /* cruft - delete when kernel fixed */
#define SYS_nfs_getfh		164
#define SYS_getfh		164 /* cruft - delete when kernel fixed */
#define SYS_getdirentries	159
#define SYS_getdomainname	165
#define SYS_setdomainname	166
#define SYS_exportfs		169

#else /* vax */
#define SYS_msgctl		151
#define SYS_msgget		152
#define SYS_msgrcv		153
#define SYS_msgsnd		154
#define SYS_semctl		155
#define SYS_semget		156
#define SYS_semop		157
#define SYS_uname		158
#define SYS_shmsys		159
#define SYS_plock		160
#define SYS_lockf		161
#define SYS_ustat		162
#define SYS_getmnt		163
#define SYS_getdirentries	164
#define SYS_nfs_biod		165	 /* 165 -- reserved for Ultrix */
#define SYS_nfs_getfh		166      /* 166 -- reserved for Ultrix */
#define SYS_nfs_svc		167      /* 167 -- reserved for Ultrix */
#define SYS_exportfs		168	 /* 168 -- reserved for Ultrix */
#define SYS_getdomainname	169	 /* 169 -- reserved for Ultrix */
#define SYS_setdomainname	170	 /* 170 -- reserved for Ultrix */
#define	SYS_sigpending		171	 /* 171 -- reserved for Ultrix */
#define	SYS_setsid		172
#define	SYS_waitpid		173
#endif /* __vax */

#ifdef __mips
#define SYS_msgctl		172
#define SYS_msgget		173
#define SYS_msgrcv		174
#define SYS_msgsnd		175
#define SYS_semctl		176
#define SYS_semget		177
#define SYS_semop		178
#define SYS_uname		179
#define SYS_shmsys		180
#define SYS_plock		181
#define SYS_lockf		182
#define SYS_ustat		183
#define SYS_getmnt		184
#ifdef notdef
#define SYS_mount		185
#define SYS_umount		186
#endif /* notdef */
#define	SYS_sigpending		187
#define	SYS_setsid		188
#define	SYS_waitpid		189
#endif /* __mips */

#define	SYS_utc_gettime		233	 /* 233 -- same as OSF/1 */
#define SYS_utc_adjtime		234	 /* 234 -- same as OSF/1 */
#define SYS_audcntl		252
#define SYS_audgen		253
#define SYS_startcpu		254	 /* 254 -- Ultrix Private */
#define SYS_stopcpu		255	 /* 255 -- Ultrix Private */
#define SYS_getsysinfo		256	 /* 256 -- Ultrix Private */
#define SYS_setsysinfo		257	 /* 257 -- Ultrix Private */
