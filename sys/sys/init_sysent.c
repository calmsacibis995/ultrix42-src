#ifndef lint
static char *sccsid = "@(#)init_sysent.c	4.2	ULTRIX	9/4/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987, 1988 by		*
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
 *   Modification history:
 *
 * 13-Aug-90 -- Joe Comuzzi
 *      Added utc_gettime() and utc_adjtime() as 233 and 234, the values
 *      chosen for OSF/1
 *
 * 09-Nov-89 -- jaw
 *	remove support for asymmetric system calls.
 *
 * 10 Oct 89 -- jas
 *      Added atomic_op().
 *
 * 06 Apr 89 -- prs
 *	Marked accounting and quota system calls mpsafe.
 *
 * 17-Feb-89 -- map (Mark Parenti)
 *	The table does not match by number starting at 189 in mips
 *	mode.  Add comments for future reference.  Add places holders
 *	at 254 and 255 in VAX mode to get correct entries for
 *	getsysinfo() and setsysinfo.  Table is now synchronized 
 *	starting at entry 258.
 *	Fix incorrect #ifdef mips at 153. This should have encompassed
 *	all the mips-specific entries from 151-189.
 *
 * 2 Mar 88 -- chet
 *	Added getsysinfo(0 and setsysinfo().
 *
 * 28-Sep-87 -- map
 *	Added sigpending() system call for POSIX
 *
 * 12 May 87 -- chet
 *	Increased # of getmnt() args. from 3 to 5.
 *
 * 28-Apr-87 -- logcher
 *	Moved exportfs from 171 to 168
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added exportfs
 *
 * 15-Jul-86 -- rich
 *	added adjtime
 *
 * 11 June 86 -- Chase
 *	Added getdomainname and setdomainname system calls
 *
 * 02-Apr-86 -- jrs
 *	Added third param on whether call is mp safe
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 *	07 Oct 85 -- reilly
 *	Added the ustat syscall.
 *
 *	25 Jun 85 -- depp
 *	Moved shmsys due to conflict with Berkeley.  Reserved 160-170
 *	for future Ultrix use, with 160-162 for {plock, lockf, ustat}
 *
 *	 4 Jun 85 -- depp
 *	Added uname system call for System V
 *
 *	22 Feb 85 -- depp
 *	Added in new system calls for System V IPC
 *
 *
 */

/*
 * System call switch table.
 */

#include "../h/param.h"
#include "../h/systm.h"

int	nosys();

/* 1.1 processes and protection */
int	sethostid(),gethostid(),sethostname(),gethostname(),getpid();
int	getdomainname(), setdomainname();
int	fork(),rexit(),execv(),execve(),wait(),waitpid();
int	getuid(),setreuid(),getgid(),getgroups(),setregid(),setgroups();
int	getpgrp(),setpgrp(),setsid();

/* 1.2 memory management */
int	plock();
int	sbrk(),sstk();
int	getpagesize(),smmap(),mremap(),munmap(),mprotect(),madvise(),mincore();

/* 1.3 signals */
int	sigvec(),sigblock(),sigsetmask(),sigpause(),sigstack(),sigpending();
int	sigreturn();
int	kill(), killpg();

/* 1.4 timing and statistics */
int	gettimeofday(),settimeofday();
int	getitimer(),setitimer();

/* 1.5 descriptors */
int	getdtablesize(),dup(),dup2(),close();
int	select(),getdopt(),setdopt(),fcntl(),flock();
int	adjtime();

/* 1.6 resource controls */
int	getpriority(),setpriority(),getrusage(),getrlimit(),setrlimit();
int	setquota(),qquota();

/* 1.7 system operation support */
int	umount(),smount(),swapon();
int	sync(),reboot(),sysacct();

/* 1.11 audit subsystem */
int	audcntl(), audgen();

/* 2.1 generic operations */
int	read(),write(),readv(),writev(),ioctl();

/* 2.2 file system */
int	chdir(),chroot();
int	mkdir(),rmdir();
int	creat(),open(),mknod(),unlink(),stat(),fstat(),lstat();
int	chown(),fchown(),chmod(),fchmod(),utimes();
int	link(),symlink(),readlink(),rename();
int	lseek(),truncate(),ftruncate(),saccess(),fsync();

/* 2.3 communications */
int	socket(),bind(),listen(),accept(),connect();
int	socketpair(),sendto(),send(),recvfrom(),recv();
int	sendmsg(),recvmsg(),shutdown(),setsockopt(),getsockopt();
int	getsockname(),getpeername(),pipe();

int	umask();		/* XXX */

/* 2.4 processes */
int	ptrace();

/* 2.5 terminals */

#ifdef KDEBUG
int kdbenter();
#endif KDEBUG

#ifdef COMPAT
/* emulations for backwards compatibility */
#define	compat(n, name)	n, o/**/name

int	owait();		/* now receive message on channel */
int	otime();		/* now use gettimeofday */
int	ostime();		/* now use settimeofday */
int	oalarm();		/* now use setitimer */
int	outime();		/* now use utimes */
int	opause();		/* now use sigpause */
int	onice();		/* now use setpriority,getpriority */
int	oftime();		/* now use gettimeofday */
int	osetpgrp();		/* ??? */
int	otimes();		/* now use getrusage */
int	ossig();		/* now use sigvec, etc */
int	ovlimit();		/* now use setrlimit,getrlimit */
int	ovtimes();		/* now use getrusage */
int	osetuid();		/* now use setreuid */
int	osetgid();		/* now use setregid */
int	ostat();		/* now use stat */
int	ofstat();		/* now use fstat */
#else
#define	compat(n, name)	0, nosys
#endif

/* 2.6 System V IPC stuff */
int msgctl(),msgget(),msgrcv(),msgsnd();
int semctl(),semget(),semop();
int smsys();
int uname();
int ustat();

/* GFS items */
int getmnt(), getdirentries();

/* NFS items */
#ifdef NFS
int nfs_biod(), nfs_getfh(), nfs_svc();
int exportfs();
#endif

#ifdef mips
/* private mips calls */
int sysmips();
int cacheflush(), cachectl();
int atomic_op();
#endif mips

int utc_gettime();
int utc_adjtime();

/* private ULTRIX calls */
int getsysinfo(), setsysinfo();
int startcpu(), stopcpu();

/* BEGIN JUNK */
#ifdef vax
int	resuba();
#ifdef TRACE
int	vtrace();
#endif
#endif
int	profil();		/* 'cuz sys calls are interruptible */
int	vhangup();		/* should just do in exit() */
int	vfork();		/* awaiting fork w/ copy on write */
int	obreak();		/* awaiting new sbrk */
int	ovadvise();		/* awaiting new madvise */
/* END JUNK */

struct sysent sysent[] = {
	0, nosys, 			/*   0 = indir */
	1, rexit, 			/*   1 = exit */
	0, fork, 			/*   2 = fork */
	3, read, 			/*   3 = read */
	3, write, 			/*   4 = write */
	3, open, 			/*   5 = open */
	1, close, 			/*   6 = close */
#ifdef KDEBUG
	0, kdbenter, 			/*   7 = enter kernel debuger */
#else
	compat(0,wait),			/*   7 = old wait */
#endif
	2, creat, 			/*   8 = creat */
	2, link, 			/*   9 = link */
	1, unlink, 			/*  10 = unlink */
	2, execv, 			/*  11 = execv */
	1, chdir, 			/*  12 = chdir */
	compat(0,time),			/*  13 = old time */
	3, mknod, 			/*  14 = mknod */
	2, chmod, 			/*  15 = chmod */
	3, chown, 			/*  16 = chown; now 3 args */
	1, obreak, 			/*  17 = old break */
	compat(2,stat),			/*  18 = old stat */
	3, lseek, 			/*  19 = lseek */
	0, getpid, 			/*  20 = getpid */
	5, smount, 			/*  21 = mount */
	1, umount, 			/*  22 = umount */
	compat(1,setuid),		/*  23 = old setuid */
	0, getuid, 			/*  24 = getuid */
	compat(1,stime),		/*  25 = old stime */
	4, ptrace, 			/*  26 = ptrace */
	compat(1,alarm),		/*  27 = old alarm */
	compat(2,fstat),		/*  28 = old fstat */
	compat(0,pause),		/*  29 = opause */
	compat(2,utime),		/*  30 = old utime */
	0, nosys, 			/*  31 = was stty */
	0, nosys, 			/*  32 = was gtty */
	2, saccess, 			/*  33 = access */
	compat(1,nice),			/*  34 = old nice */
	compat(1,ftime),		/*  35 = old ftime */
	0, sync, 			/*  36 = sync */
	2, kill, 			/*  37 = kill */
	2, stat, 			/*  38 = stat */
	compat(2,setpgrp),		/*  39 = old setpgrp */
	2, lstat, 			/*  40 = lstat */
	2, dup, 			/*  41 = dup */
	0, pipe, 			/*  42 = pipe */
	compat(1,times),		/*  43 = old times */
	4, profil, 			/*  44 = profil */
	0, nosys, 			/*  45 = nosys */
	compat(1,setgid),		/*  46 = old setgid */
	0, getgid, 			/*  47 = getgid */
	compat(2,ssig),			/*  48 = old sig */
	0, nosys, 			/*  49 = reserved for USG */
	0, nosys, 			/*  50 = reserved for USG */
	1, sysacct, 			/*  51 = turn acct off/on */
	0, nosys, 			/*  52 = old set phys addr */
	0, nosys, 			/*  53 = old lock in core */
	3, ioctl, 			/*  54 = ioctl */
	1, reboot, 			/*  55 = reboot */
	0, nosys, 			/*  56 = old mpxchan */
	2, symlink, 			/*  57 = symlink */
	3, readlink, 			/*  58 = readlink */
	3, execve, 			/*  59 = execve */
	1, umask, 			/*  60 = umask */
	1, chroot, 			/*  61 = chroot */
	2, fstat, 			/*  62 = fstat */
	0, nosys, 			/*  63 = used internally */
	1, getpagesize, 		/*  64 = getpagesize */
	5, mremap, 			/*  65 = mremap */
	0, vfork, 			/*  66 = vfork */
	0, read, 			/*  67 = old vread */
	0, write, 			/*  68 = old vwrite */
	1, sbrk, 			/*  69 = sbrk */
	1, sstk, 			/*  70 = sstk */
	6, smmap, 			/*  71 = mmap */
	1, ovadvise, 			/*  72 = old vadvise */
	2, munmap, 			/*  73 = munmap */
	3, mprotect, 			/*  74 = mprotect */
	3, madvise, 			/*  75 = madvise */
	1, vhangup, 			/*  76 = vhangup */
	compat(2,vlimit),		/*  77 = old vlimit */
	3, mincore, 			/*  78 = mincore */
	2, getgroups, 			/*  79 = getgroups */
	2, setgroups, 			/*  80 = setgroups */
	1, getpgrp, 			/*  81 = getpgrp */
	2, setpgrp, 			/*  82 = setpgrp */
	3, setitimer, 			/*  83 = setitimer */
#ifdef vax
	0, wait, 			/*  84 = wait */
#endif vax
#ifdef mips
	3, wait, 			/*  84 = wait */
#endif mips
	1, swapon, 			/*  85 = swapon */
	2, getitimer, 			/*  86 = getitimer */
	2, gethostname, 		/*  87 = gethostname */
	2, sethostname, 		/*  88 = sethostname */
	0, getdtablesize, 		/*  89 = getdtablesize */
	2, dup2, 			/*  90 = dup2 */
	2, getdopt, 			/*  91 = getdopt */
	3, fcntl, 			/*  92 = fcntl */
	5, select, 			/*  93 = select */
	2, setdopt, 			/*  94 = setdopt */
	1, fsync, 			/*  95 = fsync */
	3, setpriority, 		/*  96 = setpriority */
	3, socket, 			/*  97 = socket */
	3, connect, 			/*  98 = connect */
	3, accept, 			/*  99 = accept */
	2, getpriority, 		/* 100 = getpriority */
	4, send, 			/* 101 = send */
	4, recv, 			/* 102 = recv */
#ifdef vax
	0, nosys, 			/* 103 = old socketaddr */
#endif vax
#ifdef mips
	1, sigreturn, 			/* 103 = sigreturn */
#endif mips
	3, bind, 			/* 104 = bind */
	5, setsockopt, 			/* 105 = setsockopt */
	2, listen, 			/* 106 = listen */
	compat(2,vtimes),		/* 107 = old vtimes */
#ifdef vax
	3, sigvec, 			/* 108 = sigvec */
#endif vax
#ifdef mips
	4, sigvec, 			/* 108 = sigvec */
#endif mips
	1, sigblock, 			/* 109 = sigblock */
	1, sigsetmask, 			/* 110 = sigsetmask */
	1, sigpause, 			/* 111 = sigpause */
	2, sigstack, 			/* 112 = sigstack */
	3, recvmsg, 			/* 113 = recvmsg */
	3, sendmsg, 			/* 114 = sendmsg */
#ifdef TRACE
	2, vtrace, 			/* 115 = vtrace */
#else
	0, nosys, 			/* 115 = nosys */
#endif
	2, gettimeofday, 		/* 116 = gettimeofday */
	2, getrusage, 			/* 117 = getrusage */
	5, getsockopt, 			/* 118 = getsockopt */
#ifdef vax
	1, resuba, 			/* 119 = resuba */
#else
	0, nosys, 			/* 119 = nosys */
#endif
	3, readv, 			/* 120 = readv */
	3, writev, 			/* 121 = writev */
	2, settimeofday, 		/* 122 = settimeofday */
	3, fchown, 			/* 123 = fchown */
	2, fchmod, 			/* 124 = fchmod */
	6, recvfrom, 			/* 125 = recvfrom */
	2, setreuid, 			/* 126 = setreuid */
	2, setregid, 			/* 127 = setregid */
	2, rename, 			/* 128 = rename */
	2, truncate, 			/* 129 = truncate */
	2, ftruncate, 			/* 130 = ftruncate */
	2, flock, 			/* 131 = flock */
	0, nosys, 			/* 132 = nosys */
	6, sendto, 			/* 133 = sendto */
	2, shutdown, 			/* 134 = shutdown */
	5, socketpair, 			/* 135 = socketpair */
	2, mkdir, 			/* 136 = mkdir */
	1, rmdir, 			/* 137 = rmdir */
	2, utimes,			/* 138 = utimes */
#ifdef vax
	0, nosys, 			/* 139 = used internally */
#endif vax
#ifdef mips
	1, sigreturn, 			/* 139 = sigreturn (4.2 longjumps)*/
#endif mips
	2, adjtime, 			/* 140 = adjtime */
	3, getpeername, 		/* 141 = getpeername */
	2, gethostid, 			/* 142 = gethostid */
	2, sethostid, 			/* 143 = sethostid */
	2, getrlimit, 			/* 144 = getrlimit */
	2, setrlimit, 			/* 145 = setrlimit */
	2, killpg, 			/* 146 = killpg */
	0, nosys, 			/* 147 = nosys */
	2, setquota, 			/* 148 = quota */
	4, qquota, 			/* 149 = qquota */
	3, getsockname, 		/* 150 = getsockname */
#ifdef vax
	3, msgctl, 			/* 151 = msgctl */
	2, msgget, 			/* 152 = msgget */
	5, msgrcv, 			/* 153 = msgrcv */
	4, msgsnd, 			/* 154 = msgsnd */
	4, semctl, 			/* 155 = semctl */
	3, semget, 			/* 156 = semget */
	3, semop, 			/* 157 = semop */
	1, uname, 			/* 158 = uname */
	4, smsys, 			/* 159 = shared memory */
	1, plock, 			/* 160 = plock */
	0, nosys, 			/* 161 = lockf (future) */
	2, ustat, 			/* 162 = ustat */
	5, getmnt, 			/* 163 = getmnt */
	4, getdirentries, 		/* 164 = getdirentries */
#ifdef NFS
	0, nfs_biod, 			/* 165 = NFS block I/O daemon */
	3, nfs_getfh, 			/* 166 = NFS get file handle */
	1, nfs_svc, 			/* 167 = NFS server daemon */
	3, exportfs, 			/* 168 = exportfs */
#else
	0, nosys, 			/* 165 */
	0, nosys, 			/* 166 */
	0, nosys, 			/* 167 */
	0, nosys, 			/* 168 */
#endif
	2, getdomainname, 		/* 169 = getdomainname */
	2, setdomainname, 		/* 170 = setdomainname */
	1, sigpending, 		/* 171 = sigpending */
	0, setsid, 			/* 172 = setsid */
	3, waitpid, 			/* 173 = waitpid */
#endif vax
#ifdef mips
	/*
	 * Syscalls 151-180 inclusive are reserved for vendor-specific
	 * system calls.  (This includes various calls added for compatibity
	 * with other Unix variants.)
	 */
	5, sysmips, 			/* 151 = sysmips */
	3, cacheflush,  		/* 152 = cacheflush */
	3, cachectl, 		 	/* 153 = cachectl */
#ifdef DEBUG
	1, debug,                     /* 154 = debug */
#else
	0, nosys,                     /* 154 = nosys */
#endif DEBUG
	2, atomic_op, 			/* 155 = atomic_op */
	0, nosys, 			/* 156 = nosys */
#ifdef NFS
	0, nosys, 			/* 157 = old nfs_mount */
	1, nfs_svc, 			/* 158 = nfs_svc */
#else
	0, nosys, 			/* 157 = nosys */
	0, nosys, 			/* 158 = nosys */
#endif NFS
	4, getdirentries, 		/* 159 = getdirentries */
	0, nosys, 			/* 160 = statfs */
	0, nosys, 			/* 161 = fstatfs */
	0, nosys, 			/* 162 = unmount */
#ifdef NFS
	0, nfs_biod, 			/* 163 = async_daemon */
	3, nfs_getfh, 			/* 164 = get file handle */
#else
	0, nosys, 			/* 163 = nosys */
	0, nosys, 			/* 164 = nosys */
#endif NFS
	2, getdomainname, 		/* 165 = getdomainname */
	2, setdomainname, 		/* 166 = setdomainname */
 	0, nosys, 			/* 167 = old pcfs_mount */
#ifdef QUOTA
 	0, nosys, 			/* 168 = quotactl */
#else
	0, errsys, 			/* 168 = not configured */
#endif QUOTA
#ifdef NFS
 	3, exportfs, 			/* 169 = exportfs */
#else
 	0, errsys, 			/* 169 = not configured */
#endif NFS
	0, nosys, 			/* 170 = mount */
#ifdef mips
	2, nosys, 			/* 171 = mipshwconf */
#endif mips
#ifdef mips			
/*
 * Ultrix system calls that mips doesn't have or are incompatible 
 */
	3, msgctl, 			/* 172 = msgctl */
	2, msgget, 			/* 173 = msgget */
	5, msgrcv, 			/* 174 = msgrcv */
	4, msgsnd, 			/* 175 = msgsnd */
	4, semctl, 			/* 176 = semctl */
	3, semget, 			/* 177 = semget */
	3, semop, 			/* 178 = semop */
	1, uname, 			/* 179 = uname */
	4, smsys, 			/* 180 = shared memory */
	1, plock, 			/* 181 = plock */
	0, nosys, 			/* 182 = lockf (future) */
	2, ustat, 			/* 183 = ustat */
	5, getmnt,			/* 184 = getmnt */
	5, smount,			/* 185 = mount */
	1, umount, 			/* 186 = umount */
	1, sigpending, 			/* 187 = sigpending */
	0, setsid, 			/* 188 = setsid */
	3, waitpid, 			/* 189 = waitpid */
#endif mips
#endif mips
#ifdef vax
	0, nosys, 			/* 174 */
	0, nosys, 			/* 175 */
	0, nosys, 			/* 176 */
	0, nosys, 			/* 177 */
	0, nosys, 			/* 178 */
	0, nosys, 			/* 179 */
	0, nosys, 			/* 180 */
	0, nosys, 			/* 181 */
	0, nosys, 			/* 182 */
	0, nosys, 			/* 183 */
	0, nosys, 			/* 184 */
	0, nosys, 			/* 185 */
	0, nosys, 			/* 186 */
	0, nosys, 			/* 187 */
	0, nosys, 			/* 188 */
	0, nosys, 			/* 189 */
#endif vax

	0, nosys, 			/* 190 */
	0, nosys, 			/* 191 */
	0, nosys, 			/* 192 */
	0, nosys, 			/* 193 */
	0, nosys, 			/* 194 */
	0, nosys, 			/* 195 */
	0, nosys, 			/* 196 */
	0, nosys, 			/* 197 */
	0, nosys, 			/* 198 */
	0, nosys, 			/* 199 */
	0, nosys, 			/* 200 */
	0, nosys, 			/* 201 */
	0, nosys, 			/* 202 */
	0, nosys, 			/* 203 */
	0, nosys, 			/* 204 */
	0, nosys, 			/* 205 */
	0, nosys, 			/* 206 */
	0, nosys, 			/* 207 */
	0, nosys, 			/* 208 */
	0, nosys, 			/* 209 */
	0, nosys, 			/* 210 */
	0, nosys, 			/* 211 */
	0, nosys, 			/* 212 */
	0, nosys, 			/* 213 */
	0, nosys, 			/* 214 */
	0, nosys, 			/* 215 */
	0, nosys, 			/* 216 */
	0, nosys, 			/* 217 */
	0, nosys, 			/* 218 */
	0, nosys, 			/* 219 */
	0, nosys, 			/* 220 */
	0, nosys, 			/* 221 */
	0, nosys, 			/* 222 */
	0, nosys, 			/* 223 */
	0, nosys, 			/* 224 */
	0, nosys, 			/* 225 */
	0, nosys, 			/* 226 */
	0, nosys, 			/* 227 */
	0, nosys, 			/* 228 */
	0, nosys, 			/* 229 */
	0, nosys, 			/* 230 */
	0, nosys, 			/* 231 */
	0, nosys, 			/* 232 */
	1, utc_gettime,			/* 233 = get utc time */
	2, utc_adjtime,			/* 234  = set/adjust utc */
	0, nosys, 			/* 235 */
	0, nosys, 			/* 236 */
	0, nosys, 			/* 237 */
	0, nosys, 			/* 238 */
	0, nosys, 			/* 239 */
	0, nosys, 			/* 240 */
	0, nosys, 			/* 241 */
	0, nosys, 			/* 242 */
	0, nosys, 			/* 243 */
	0, nosys, 			/* 244 */
	0, nosys, 			/* 245 */
	0, nosys, 			/* 246 */
	0, nosys, 			/* 247 */
	0, nosys, 			/* 248 */
	0, nosys, 			/* 249 */
	0, nosys, 			/* 250 */
	0, nosys, 			/* 251 */
	5, audcntl, 			/* 252 = audcntl */
	3, audgen, 			/* 253 = audgen */
	1, startcpu, 			/* 254 = startcpu */
	1, stopcpu, 			/* 255 = stopcpu */
	6, getsysinfo, 			/* 256 = getsysinfo */
	5, setsysinfo, 			/* 257 = setsysinfo */

};

int	nsysent = sizeof (sysent) / sizeof (sysent[0]);
