#ifndef lint
static char *sccsid = "@(#)init_main.c	4.7	(ULTRIX)	2/28/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *
 *			Modification History
 *
 * 28 Feb 91 -- prs
 *	Added support for a configurable number of
 *	open file descriptors.
 *
 * 14 Jan 91 -- Darrell Dunnuck
 *	Added call to pseudo_attach() for pseudo network devices.
 *
 * 20 Aug 90 -- Matt Thomas
 *	Add call to generate the node's UID.
 *
 * 10 Aug 90 -- jaa
 *	Turn on unaligned access fixups for proc 0
 *	Increase stack soft limit to 2 meg
 *
 * 12-Dec-1989  Alan Frechette
 *	Set the variable "dumpnetboot" to 1 if a network boot was
 *	performed. Needed in dumpsys() when checking for a network
 *	dump because forcing a dump via a RESET blows away "bootdev".
 *
 * 11 Dec 89 -- chet
 *	Change number of buffer cache hash lists computation.
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 *  14-Nov-89	gmm
 *	Change idle_swtch() call to idle() for mips.
 *
 *  1-Nov-89	Giles Atkinson
 *	Move call of set_lmf_smm() to point where the clock is running.
 *
 * 19-Oct-1989  Alan Frechette
 *	Moved bdevsw() call from gendump() to init_main(). Save the
 *	DEVIOCGET status and information globally.
 *
 * 13 Oct 89 -- gmm
 *	SMP changes for MIPS. Creates idle proc for the boot cpu. Copies
 *	name of kernel process into u_comm. Changed pid of tpathd to 4 for
 *	mips
 *
 * 25 Jul 89 -- chet
 *	Changes for new buffer cache organization
 *
 * 21-Jul-89 jaa
 *	remove delay from ufs_swapconf
 *	if swap device does not come online, mark it invalid in swdev
 *
 * 20-Jul-89 jaw
 *	clear noproc after set percpu pointer to proc structure.
 *
 * 12-Jun-89 -- dws
 *	Added trusted path system process.
 *
 * 12-Jun-89 -- gg
 *	Add call to dmapinit();
 *	Removed system panicking when no swap device is configured --
 *	A warning is issued instead. maxdsiz and maxssiz are made
 *	configurable. Also made dynamic swap changes in main().
 *
 *  8-May-89 -- Giles Atkinson
 *	Add call to set_lmf_smm().
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 * 06-Apr-89 -- prs
 *	SMP quota: Move lockinit of lk_gnode to finit()
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 07 Jun 88 -- miche
 *	SMP procqs:  modified to know about the bitmap at startup
 *
 * 20 May 88 -- condylis
 *	Added call to crinit to init smp cred lock
 *
 * 19 May 88 -- prs
 *	Moved some mounting of root logic to gfs_data.c.
 *
 * 19 May 88 -- cb
 *	Integrated gnode interface changes.
 *
 * 27 Apr 88 -- chet
 *	Add SMP buffer cache support.
 *
 * 17 Feb 88 -- Tim Burke
 *	Call ptyinit to initialize pseudo tty's as being MP-safe.
 *
 * 28 Jan 88 -- us
 *	Added change for malloced mbufs.
 *
 * 28 Jan 88 -- us
 *	Switched to new kernel memory allocator.
 *
 * 21 Jan 88 -- jmartin
 *	Replace calls to the (inline) functions clearseg and copyseg
 *	respectively with blkclr (or bzero) and blkcpy (or bcopy).
 *	Establish a window in process memory through which a parent
 *	can write to (and read from) the memory of the child.  This
 *	window is UPAGES*NBPG bytes located between the u-area and the
 *	user stack.  Remove the following entities: CMAP1, CADDR1,
 *	CMAP2, CADDR2, Vfmap, vfutl, clearseg, copyseg.  Redefine
 *	Forkmap and forkutl.  Change the computation for the location
 *	of USRSTACK and the size of the process page table.
 *
 ***************** SMP CHANGES above **************************
 *
 * 20-Oct-1988	Peter Keilty
 *	Change call to mscp_poll_wait to include a count parameter.
 *
 * 13-Sep-88	larry
 *	Do not wait infinitely for swap device to sucessfully open.
 *	Prevents system from hanging when config in bad secondary swap.
 *
 * 30-Aug-88	jaw
 *	add proc field p_master to fix ASMP bug.
 *
 * 27-Jun-88 larry
 *	flush print buffers after mscp polling.  Helps prevent printout
 *	scribbling during boot.
 *
 * 9-Jun-88 jaa
 *	fixed ufs_swapconf() so that if an open of a swap device
 *	fails, print an error message and retry until the open succeeds
 *
 * 4-Apr-88 jaa
 *	calculate and allocate swapmap in swapconf from now on
 *
 * 01-April-88	Fred L. Templin
 *	added cpu_subtype check to determine proper version of LANCE
 *	driver to use for diskless boots.
 *
 * 14-Mar-88 	Larry Cohen
 *	place configure after sysap startup
 *
 * 16-Jan-88	Larry Palmer
 *	Removed mbinit call.
 *
 * 28-Jan-88	jaw
 *	Set SMASTER flag on pager and swapper.
 *
 * 12-11-87	Robin L. and Larry C.
 *      Added portclass/kmalloc support to the system.
 *
 *
 * 23-Jul-87 -- cb
 *      Fixed the "bind()" bug by bzeroing out the ifreq struct allocated
 *      off the stack by getonnetwork().
 *
 * 25-Jun-87 -- Robin
 *	Added code to size the dump partition on the way up and
 *	save it away for later use by crash dump code in machdep.
 *
 * 15-Jun-87 -- logcher
 *	Print out different message at boot time if swapping to 
 *	local disk rather than to remote disk.
 *
 * 12-Jun-87 -- lea
 *	Initialized pointer so that the flags field does not collect
 *	garbage.
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added support for setting up an 
 *	NFS root filesystem
 *
 * 15-Dec-86 -- depp
 *	Fixed problem in swapconf() that caused crash when a MASSBUS
 *	disk is booted.
 *
 * 30-Sep-86 - Charlie Briggs
 *	Added nfs_swapconf and ufs_swapconf.
 *
 * 11-Sep-86 - koehler
 *	moved the mounting of the root filesystem to gfs_data.
 *
 * 16-Apr-86 - ricky palmer
 *	Replaced root device "case" code with devioctl code.
 *
 * 15-Apr-86 -- jf
 *	call system process initialization routine
 *
 * 12-Mar-86 -- bjg
 *	dumplo now decreased by one byte; after msgbuf removed, not
 *	enough space for dumps, so decremented dumplo by 1.
 *
 * 18-Feb-86 -- jrs
 *	Move unlock that releases slaves down a little to be extra safe
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 *	03-Oct-85	Stephen
 * 004- Dumplo will now be calculated on the physical memory size and 
 *	not on a predefined constant.
 *
 *	09-Sept-85	Stephen Reilly
 * 003- Modified to handle the new lockf code.
 *
 *	01-Jul-85	Stephen Reilly
 * 002- Dump's size was being based on the first entry of the
 *	swap table.  It is now fixed so that the size of the dump
 *	partition will be based on the dump device itself.
 *
 * 24 Oct 84 -- jrs
 *	Add initialization for nami cacheing and linked proc list
 *	Derived from 4.2BSD, labeled:
 *		init_main.c 6.3 84/05/22
 *
 *	01-Nov-84	Stephen Reilly
 * 001- The size of the swap device is now done here rather than in
 *	autoconf.c.  The reason is becuase of the new disk partitioning
 *	scheme.
 *	init_main.c	6.1	83/07/29
 *
 *	22 Feb 85	Greg Depp
 *	Add in calls to msginit() and seminit() for System V IPC
 *
 ***********************************************************************/
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/ioctl.h"
#include "../h/devio.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/mount.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/fs_types.h"
#include "../h/seg.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/types.h"
#include "../h/smp_lock.h"
#ifdef INET
#include "../h/protosw.h"
#endif
#include "../h/quota.h"
#include "../h/cpudata.h"
#include "../machine/reg.h"
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vmparam.h"
#include "../h/socket.h"
#include "../h/ioctl.h"
#include "../fs/ufs/fs.h"
#include "../net/netinet/in.h"
#include "../net/net/if.h"
#include "../net/rpc/types.h"
#include "../net/rpc/xdr.h"
#include "../net/rpc/auth.h"
#include "../net/rpc/clnt.h"
#include "../fs/nfs/nfs.h"
#include "../fs/nfs/nfs_gfs.h"
#include "../fs/nfs/nfs_clnt.h"
#include "../fs/nfs/vfs.h"
#include "../fs/nfs/vnode.h"
#ifdef vax
#include "../machine/sas/vmb.h"
#include "../machine/rpb.h"
#endif vax
#include "../sas/mop.h"
#include "../h/sys_tpath.h"

extern	struct netblk *netblk_ptr;
extern  nfs_resolvfh();
#ifdef vax
extern	struct user u;		/* have to declare it somewhere! */
#endif vax
extern	gno_t rootino;		/* XXX look in ufs_mount.c */
extern int swapfrag, maxdsiz, maxssiz;
extern struct cpusw *cpup;

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
#ifdef mips
	     - process 1 to execute bootstrap
#endif mips
 *	     - process 2 to page out
#ifdef vax
 *	     - process 1 execute bootstrap
 *
 * loop at loc 13 (0xd) in user mode -- /etc/init
 *	cannot be executed.
#endif vax
 */

int roottype=GT_ULTRIX;	/* roottype set accorging to fs_types.h  */
int swaptype=SW_RAW;	/* swaptype set accorging to conf.h      */
long swapsize=0;	/* swap file size (if preconfigured)     */
int dumptype = -1;	/* dumptype set to full dump  */
struct gnode gpp;

extern struct timeval atime,btime, *timepick;
#ifdef mips
int k_puac = UAC_MSGON;
struct devget dumpdg;		/* DEVIOCGET info of dump device */
int dumpdeviocgetstatus=0;	/* DEVIOCGET status of dump device */
int dumpnetboot=0;		/* set if network boot performed */
#endif mips

#ifdef mips
main()
#else vax
main(firstaddr)
	int firstaddr;
#endif vax
{
	register int i, m, tot;
	register struct proc *p;
	register struct mount *mp;
	register int s;
	extern	int cpu;
	struct ifnet *ifp;	/* 8.9.88.us */
	extern struct lock_t lk_gnode;
	extern int (*dna_uid_g_create_rtn)();
	extern int max_nofile;
#ifdef vax
        extern scs_start_sysaps();
	extern int appendflg;
#endif vax

	if (maxdsiz <= 0)
		maxdsiz = btoc(MAXDSIZ);
	if (maxssiz <= 0)
		maxssiz = btoc(MAXSSIZ);

	timepick = &atime;
	rqinit();
#include "loop.h"

#ifdef vax
	startup(firstaddr);
#endif vax

	/*
	 * set up system process 0 (swapper)
	 */
	p = &proc[0];

	CURRENT_CPUDATA->cpu_proc = p;
	CURRENT_CPUDATA->cpu_noproc = 0;

#ifdef vax
	p->p_p0br = u.u_pcb.pcb_p0br;
	p->p_szpt = 1;
	p->p_addr = uaddr(p);
	Sysmap[btop((long)(p->p_pcb)&0x7fffffff)] = *p->p_addr;
	setredzone(p->p_addr, (caddr_t)&u);
#endif vax
#ifdef mips
	p->p_textbr = 0;
	p->p_databr = 0;
	KM_ALLOC(p->p_stakbr, struct pte *, CLBYTES, KM_SYSPROC, KM_NOW_CL_CA);
	p->p_textpt = 0;
	p->p_datapt = 0;
	p->p_stakpt = CLSIZE;
	p->p_usrpri = PUSER;
	p->p_puac = UAC_MSGON;
	p->p_mips_flag |= SFIXADE;
	/*
	 * This may work for the vax
	 */
	u.u_ap = u.u_arg;
#endif mips

	p->p_stat = SRUN;
	p->p_sched = SLOAD;
	p->p_type = SSYS;
	p->p_affinity=boot_cpu_mask;
	p->p_nice = NZERO;
	u.u_procp = p;
	u.u_cmask = CMASK;
	/*
	 * Sanity check max number of file descriptors
	 * a process can open.
	 */
	if (max_nofile < NOFILE_IN_U)
		max_nofile = NOFILE_IN_U;
	else if (max_nofile > MAX_NOFILE)
		max_nofile = MAX_NOFILE;
	u.u_lastfile = -1;
	/*
	 * No need for the file descriptor overflow table;
	 * set to NULL.
	 */
	u.u_ofile_of = NULL;
	u.u_pofile_of = NULL;
	u.u_of_count = 0;

	crinit();		/* Initialize SMP credential lock  */
	u.u_cred = crget();

	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;
	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max =
		    RLIM_INFINITY;
	u.u_rlimit[RLIMIT_STACK].rlim_cur = 2*1024*1024;
	u.u_rlimit[RLIMIT_STACK].rlim_max = ctob(maxssiz);
	u.u_rlimit[RLIMIT_DATA].rlim_max =
	    u.u_rlimit[RLIMIT_DATA].rlim_cur = ctob(maxdsiz);
	p->p_maxrss = RLIM_INFINITY/NBPG;
	lockinit(&lk_gnode, &lock_gnode_table_d);
#ifdef QUOTA
	qtinit();
	p->p_quota = u.u_quota = getquota(0, 0, Q_NDQ);
	quota_unlock(u.u_quota)
#endif
	startrtclock();
#ifdef vax
#include "kg.h"
#if NKG > 0
	startkgclock();
#endif

/* start sca system appliations */
        scs_start_sysaps();

/*
 * Configure the system.
 */
	configure();
#endif vax

	/*
	 * Initialize tables, protocols, and set up well-known inodes.
	 */
	cinit();			/* needed by dmc-11 driver */
	ptyinit();			/* Declare pty's as MP safe */
#ifdef INET
#if NLOOP > 0
	loattach();			/* XXX */
#endif
	pseudo_attach();		/* attach pseudo network devices */
	/* affinity for net devices.  8.9.88.us */
	for (ifp=ifnet; ifp; ifp=ifp->if_next)
		if (ifp->d_affinity == 0)
			ifp->d_affinity = boot_cpu_mask;

	/*
	 * Block reception of incoming packets
	 * until protocols have been initialized.
	 */
	s = splimp();
	ifinit();
#endif
	domaininit();
#ifdef INET
	splx(s);
#endif
	pqinit();
	ghinit();
	bhinit();
	binit();
	netbootchk();	/* roottype will change if this is a network boot */
	bzero(mount, sizeof(struct mount) * NMOUNT);
	init_fs();
	if (roottype == GT_NFS) {
		getonnetwk();
#ifdef mips
		dumpnetboot = 1;
#endif mips
	}
	else
		setconf(); /* assumes that if not NFS then something else */
	/*
	 * Initialise the dmap_size array
	 */

	dmapinit();
	swapconf();
	textinit();
	bswinit();
	nchinit();
	flckinit();			/* 003 */
#ifdef GPROF
	kmstartup();
#endif
	msginit();
	seminit();

#ifdef mips
/* The Following Lines Moved From Just After hardclock Startup */
/* This done to avoid doing bad things before kmstartup() */

#if defined(STATCLOCK)
	startstatclock();
#endif defined(STATCLOCK)
#endif mips

	mp = &mount[0];

	KM_ALLOC(mp->m_fs_data, struct fs_data *, sizeof(struct fs_data), KM_MOUNT, KM_NOWAIT | KM_CLEAR);
	/* wait for mscp to finish polling for disks/tapes */
#ifdef mips
	if (cpup->flags & MSCP_POLL_WAIT)
	    mscp_poll_wait(5);
#endif mips
#ifdef vax
	mscp_poll_wait(5);
	spl6();
	appendflg = 0;
	printf("");
	spl0();
#endif vax
	mount_root(mp);
	nmount++;
	mp->m_gnodp = (struct gnode *) NULL;
	mp->m_flgs |= MTE_DONE;
	
	*timepick = time;
	boottime = time;
	if (dna_uid_g_create_rtn)
	    (*dna_uid_g_create_rtn)(-1, NULL);

/* This goes here to ensure that the clock is running on a DS 3100 */
	set_lmf_smm();

/* kick off timeout driven events by calling first time */
	schedcpu();
	schedpaging();

	/*
	 * set up the root file system
	 */
	if (roottype!=GT_NFS) {
		rootdir = gget(mp, rootino, 0, NULL);
                if (rootdir == NULL)
                        panic("init_main: rootdir == NULL");
		gfs_unlock(rootdir);
		u.u_cdir = gget(mp, rootino, 0, NULL);
                if (u.u_cdir == NULL)
                        panic("init_main: cdir == NULL");
		gfs_unlock(u.u_cdir);
	}
	else
		u.u_cdir=rootdir=mp->m_rootgp;

	u.u_rdir = NULL;

	bcopy(" swapper",(caddr_t)&u.u_comm[0], MAXCOMLEN);
	/*
	 * Set the scan rate and other parameters of the paging subsystem.
	 */
	setupclock();

#ifdef mips
	mpid = 1;	/* next pid to be used in nextpid() */
	proc[1].p_stat = SZOMB; 	/* force it to be in proc slot 2 */

	/*
	 * set the bits in the procmap which are used for
	 * slots 0, 1 and 2:  low order 3 bits are set
	 */
	proc_bitmap[0] = 7;

	/* 
	 * Make init process
	 */
	proc[0].p_stakpt = CLSIZE;
	proc[0].p_datapt = CLSIZE;
	proc[0].p_textpt = 0;
	p->p_datastart = USRDATA;
	p->p_textstart = USRTEXT;

	if (newproc(&proc[1], 0)) {
		unsigned szicode;
		int ptsz, err;

		ptsz = clrnd(ctopt(u.u_dsize)) + clrnd(UPAGES) + 
			clrnd(ctopt(1));
		ALLOC_VAS(ptsz, err);
		if(err)
			panic("init_main: can't alloc pt space");
		szicode = eicode - (char *)icode;
		expand(clrnd((int)btoc(szicode)), 0);
		if ((u.u_procp->p_dmap = dmalloc(u.u_dsize, CDATA)) == 0)
			panic("main: can't allocate dmap");
		/*
		 * Do a dummy allocation of smap space
		 */

		if ((u.u_procp->p_smap = dmalloc(1, CSTACK)) == 0)
			panic("main: can't allocate smap");
		(void) copyout((caddr_t)icode, (caddr_t)USRDATA, 
			(unsigned)szicode);
		/*
		 * Return goes to loc. 0 of user init
		 * code just copied out.
		 */
		u.u_procp->p_cdmap = (struct dmap *)NULL;
		u.u_procp->p_csmap = (struct dmap *)NULL;
		return;
	}
	/*
	 * make page-out daemon (process 2)
	 * the daemon has ctopt(nswbuf*CLSIZE*KLMAX) pages of page
	 * table so that it can map dirty pages into
	 * its address space during asychronous pushes.
	 */
	/*
	 * u block not mapped with user stack on mips machine
	 */
	proc[0].p_datapt = clrnd(ctopt(nswbuf*CLSIZE*KLMAX));
	mpid = 2;
	if (newproc(&proc[2],0)) {
		proc[2].p_affinity = boot_cpu_mask;  /* must be boot cpu */
		proc[2].p_type |= SSYS;
		proc[2].p_sched |= SLOAD;
		proc[2].p_dsize = u.u_dsize = nswbuf*CLSIZE*KLMAX;
		bcopy(" pagedaemon",(caddr_t)&u.u_comm[0], MAXCOMLEN);
		pageout();
		/*NOTREACHED*/
	}
	mpid = 3;
	proc_bitmap[0] = 15;
	if (newproc(&proc[3],0)) {
		proc[3].p_affinity = boot_cpu_mask;
		proc[3].p_type |= SSYS;
		proc[3].p_sched |= SLOAD;
		proc[3].p_mips_flag |= SIDLEP;
		bcopy(" idleproc",(caddr_t)&u.u_comm[0], MAXCOMLEN);
		CURRENT_CPUDATA->cpu_idleproc = &proc[3]; 
		idle();
	}

	mpid = 4;
	/*
	 * Make trusted path daemon (process 3)
	 *
	 */
	if (do_tpath) {
		proc[0].p_stakpt = proc[0].p_datapt = proc[0].p_textpt = CLSIZE;
		proc_bitmap[0] = 31;	/* mark slot 4 */
		if (newproc(&proc[4], 0)) {
			bcopy("tpathd", (caddr_t)&u.u_comm[0], MAXCOMLEN);
			proc[4].p_affinity = boot_cpu_mask;  /* for non-smp drivers */
			proc[4].p_type |= SSYS;
			proc[4].p_sched |= SLOAD;
			tpath_sysproc();
			/*NOTREACHED*/
		}
		mpid = 5;	/* next pid to be used in nextpid() */
	}
	proc[0].p_textpt = 0;
	proc[0].p_datapt = 0;
	proc[0].p_stakpt = 0;
#endif mips
#ifdef vax
	/*
	 * make page-out daemon (process 2)
	 * the daemon has ctopt(nswbuf*CLSIZE*KLMAX) pages of page
	 * table so that it can map dirty pages into
	 * its address space during asychronous pushes.
	 */
	mpid = 2;	/* next pid to be used in nextpid() */
	proc[0].p_szpt = clrnd(ctopt(nswbuf*CLSIZE*KLMAX + HIGHPAGES));
	proc[1].p_stat = SZOMB; 	/* force it to be in proc slot 2 */

	/*
	 * set the bits in the procmap which are used for
	 * slots 0, 1 and 2:  low order 3 bits are set
	 */
	proc_bitmap[0] = 7;

	if (newproc(&proc[2], 0)) {
		proc[2].p_affinity = boot_cpu_mask;  /* must be boot cpu */
		proc[2].p_type |= SSYS;
		proc[2].p_sched |= SLOAD;
		proc[2].p_dsize = u.u_dsize = nswbuf*CLSIZE*KLMAX;
		bcopy(" pagedaemon", (caddr_t)&u.u_comm[0], MAXCOMLEN);
		pageout();
		/*NOTREACHED*/
	}

	/*
	 * make init process and
	 * enter scheduling loop
	 */

	mpid = 1;	/* next pid to be used in nextpid() */
	proc[1].p_stat = 0;
	proc[0].p_szpt = CLSIZE;

	if (newproc(&proc[1], 0)) {
		int ptsz, err;

		ptsz = clrnd(clrnd(ctopt(u.u_dsize))) +
			clrnd(UPAGES + (clrnd(ctopt(u.u_ssize + HIGHPAGES))));
		ALLOC_VAS(ptsz, err);
		if(err)
			panic("init_main: can't alloc pt space");
		expand(clrnd((int)btoc(szicode)), 0);
		if ((u.u_procp->p_dmap = dmalloc(u.u_dsize, CDATA)) == 0)
			panic("main: can't allocate dmap");
		if (u.u_ssize)
			if ((u.u_procp->p_smap = dmalloc(u.u_ssize, CSTACK)) == 0)
				panic("main: can't allocate smap");
		(void) copyout((caddr_t)icode, (caddr_t)0, (unsigned)szicode);
		/*
		 * Return goes to loc. 0 of user init
		 * code just copied out.
		 */
		u.u_procp->p_cdmap = (struct dmap *)NULL;
		u.u_procp->p_csmap = (struct dmap *)NULL;
		return;
	}
	mpid = 3;	/* next pid to be used in nextpid() */

	/*
	 * Make trusted path daemon (process 3)
	 *
	 */
	if (do_tpath) {
		proc_bitmap[0] = 15;	/* mark slot 3 */
		if (newproc(&proc[3], 0)) {
			bcopy("tpathd", (caddr_t)&u.u_comm[0], MAXCOMLEN);
			proc[3].p_affinity = boot_cpu_mask;  /* for non-smp drivers */
			proc[3].p_type |= SSYS;
			proc[3].p_sched |= SLOAD;
			tpath_sysproc();
			/*NOTREACHED*/
		}
		mpid = 4;	/* next pid to be used in nextpid() */
	}

	proc[0].p_szpt = 1;
#endif vax

	sched();
}

#define MINBUFHSZ 128	/* minimum # of buffer hash lists */

/*
 * Initialize hash lists for buffers.
 */
int nbufsperlist = 16;

bhinit()
{
	register int i;
	register struct bufhd *bp;
	extern int nbuf;

	/* determine number of buffer cache hash lists */
	/* make this dependent on number of buffer headers */
	bufhsz = MINBUFHSZ;
	while (1) {
		i = nbuf / bufhsz;
		if (i <= nbufsperlist)
			break;
		bufhsz *= 2;	/* yields, e.g., 128, 256, 512, ... */
	}
	bufhszminus1 = bufhsz - 1; /* used by BUFHASH function */

	KM_ALLOC(bufhash, struct bufhd *, (sizeof(struct bufhd) * bufhsz),
		 KM_BUFCACHE, KM_CLEAR | KM_NOWAIT);
	if (bufhash == (struct bufhd *) NULL) {
		panic("bhinit: km_alloc bufhash");
	}

	for (bp = bufhash, i = 0; i < bufhsz; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

int binit_flag; 	/* buffer cache has been initialized */
/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
binit()
{
	register struct buf *bp, *dp;
	register int i;
	int base, residual;

	lockinit(&lk_bio, &lock_bio_d);

	for (dp = bfreelist; dp < &bfreelist[BQUEUES]; dp++) {
		dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
		dp->b_flags = B_HEAD;
	}
	bbusylist.b_forw = bbusylist.b_back =
		bbusylist.av_forw = bbusylist.av_back = 
		bbusylist.busy_forw = bbusylist.busy_back = &bbusylist;
	bbusylist.b_flags = B_HEAD;

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		bp->b_gp = (struct gnode *) 0;
		bp->b_un.b_addr = buffers + i * MAXBSIZE;
		if (i < residual)
			bp->b_bufsize = (base + 1) * CLBYTES;
		else
			bp->b_bufsize = base * CLBYTES;
		binshash(bp, &bfreelist[BQ_CLEAN]);
		bp->state = B_BUSY|B_INVAL;
		bp->b_flags = B_BUSY;
		binstailbusy(bp);
		brelse(bp);
	}
	/*
	 *	Indicate that we have finished with initing the buffer
	 *	header's and are ready to determine the swap devices.
	 */
	binit_flag = -1;
}

/*
 * Initialize linked list of free swap
 * headers. These do not actually point
 * to buffers, but rather to pages that
 * are being swapped in and out.
 */
bswinit()
{
	register int i;
	register struct buf *sp = swbuf;

	bswlist.av_forw = sp;
	for (i=0; i<nswbuf-1; i++, sp++)
		sp->av_forw = sp+1;
	sp->av_forw = NULL;
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
cinit()
{
	register int ccp;
	register struct cblock *cp;

	lockinit(&cfreelist.c_lk_free_clist,&lock_cfreelist_d);

	ccp = (int)cfree;
	ccp = (ccp+CROUND) & ~CROUND;
	for (cp=(struct cblock *)ccp; cp < &cfree[nclist-1]; cp++) {
		cp->c_next = cfreelist.c_head;
		cfreelist.c_head = cp;
		cfreelist.c_count += CBSIZE;
	}
}

/*
 * Swapconf calls either nfs_swapconf or ufs_swapconf, depending
 * on the swaptype.  This routine is a combination of both autoconf.c
 * and binit code.  The reason for this was with the new disk
 * partitioning scheme the swap size can not be determined until the
 * buffer headers have been inittialized.
 */
int swapconf_flag = 0;
swapconf()
{
	register int nblks;				/* 001 */
	register struct swdevt *swp;			/* 001 */
	int status;
	extern struct pt part_tbl;              /* partition info for dumps */

	/*
	 *	Don't try to determine swap sizes until we have finished
	 *	with binit.
	 */
	if (!binit_flag)
		return;
	if (swaptype==SW_NFS) {
		nfs_swapconf();
	}
	else {
		/*
	 	*			002
	 	*	Before we can determine the size of the dump device we
	 	*	first open it.	This is now a requirement because the
	 	*	partition tables are not setup until an open is
	 	*	done on the drive.  This change is due to the new
	 	*	disk partitioning scheme.
	 	*/

                ufs_swapconf();
               /*  IF called by disk device, then ignore balance of routine */
                if (swapconf_flag)
                        return;
                swapconf_flag++;

		if (dumpdev==-1){
			printf("WARNING: DUMPDEV NOT CONFIGURED\n");
			goto chkswap;
		}
		bdevsw[major(dumpdev)].d_open(dumpdev);
		if (dumplo == 0)
#ifdef vax
			dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev)
					- (physmem*(NBPG/DEV_BSIZE)) - 1; /* 004 */
#endif vax
#ifdef mips
			dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) -
					ctod(physmem) - btodb(BLKDEV_IOSIZE);

#endif mips
		/*
		 * Perform ioctl call to get partition information.
		 */
	       	 status =  bdevsw[major(dumpdev)].d_ioctl
	       	    (dumpdev, DIOCGETPT, (struct pt *) &part_tbl);
	       	 if (status)
	       	         mprintf("dump device ioctl failed\n");
#ifdef mips
		/*
		 * Perform ioctl call to get DEVIOCGET information.
		 */
	       	 status =  bdevsw[major(dumpdev)].d_ioctl
	       	    (dumpdev, DEVIOCGET, (caddr_t) &dumpdg);
	       	 if (status)
	       	         dumpdeviocgetstatus = -1;
#endif mips
	
		/*
	 	 *	Don't forget to close it
	 	 */
		bdevsw[major(dumpdev)].d_close(dumpdev);
	}
chkswap:
	if (dumplo < 0)
		dumplo = 0;
	/*
	 * Count swap devices, and adjust total swap space available.
	 * Some of this space will not be available until a vswapon()
	 * system is issued, usually when the system goes multi-user.
	 */
	if ( nswdev == 0 ) {
		nswap = 0;
		for (swp = swdevt; swp->sw_dev; swp++) {
			if (swp->sw_dev == (dev_t)-1)
				continue;
			nswdev++;
			if (swp->sw_nblks > nswap)
				nswap = swp->sw_nblks;
		}
		if (nswdev == 0)
			panic("swapconf: SWAP DEVICE NOT CONFIGURED\n");
		
		if (nswdev > 1)
			nswap = ((nswap + swapfrag - 1)/swapfrag) * swapfrag;

		nswapmap = nswdev * ((nswap+swapfrag -1)/swapfrag);
		KM_ALLOC(swapmap, struct map *, (nswapmap*sizeof(struct map)),
			KM_RMAP, KM_CLEAR);
		nswap *= nswdev;
		maxpgio *= nswdev;
		swfree(0);
	}
}

extern struct mount_ops *nfs_mount_ops;
extern struct gnode_ops *nfs_gnode_ops;
extern nfs_strategy();
extern dev_t getpdev();
fhandle_t fhdl;
char	swpath[100];
struct mount mpp;
struct fs_data fsdatap;

/*
 * nfs_swapconf
 */
nfs_swapconf ()
{
        struct gnode *gp;
        struct mount *mp;
        struct fs_data *fsdata;
	int	i;

	gp = &gpp;
	mp = &mpp;
	fsdata = &fsdatap;


        gp->g_count = 1;
	gp->g_ops = nfs_gnode_ops;
	KM_ALLOC(vtor((struct vnode *) gp)->r_cred, struct ucred *, sizeof(struct ucred), KM_CRED, KM_CLEAR | KM_NOWAIT);

        vtor((struct vnode *) gp)->r_cred->cr_ref = 1;
        vtor((struct vnode *) gp)->r_cred->cr_uid = 0;
        vtor((struct vnode *) gp)->r_cred->cr_gid = 0;


        vtor((struct vnode *) gp)->r_cred->cr_groups[0] = 0;
        vtor((struct vnode *) gp)->r_cred->cr_ruid = 0;
        vtor((struct vnode *) gp)->r_cred->cr_rgid = 0;


	mp->m_ops = nfs_mount_ops;
	mp->iostrat = nfs_strategy;
        gp->g_mp = mp;
	lockinit(&gp->g_lk, &lock_eachgnode_d);
	gp->g_init = READY_GNODE;

        mp->m_fs_data = fsdata;

        fsdata->fd_fstype = GT_NFS;
        fsdata->fd_gtot = 1;
        fsdata->fd_mtsize = 8192;
        fsdata->fd_otsize = 8192;

        MP_TO_VFSP(mp)->vfs_data = (caddr_t) MP_TO_MIP(mp);
        ((struct vnode *) gp) -> v_vfsp = MP_TO_VFSP(mp);

        MP_TO_MIP(mp)->mi_addr.sin_family = AF_INET;
        MP_TO_MIP(mp)->mi_addr.sin_port = htons(NFS_PORT);
        MP_TO_MIP(mp)->mi_addr.sin_addr.s_addr =ntohl(netblk_ptr->srvipadr);
        MP_TO_MIP(mp)->mi_hard = 1;
        MP_TO_MIP(mp)->mi_refct = 0;
        MP_TO_MIP(mp)->mi_tsize = 8192;
        MP_TO_MIP(mp)->mi_stsize = 8192;
        MP_TO_MIP(mp)->mi_bsize = 8192;
        MP_TO_MIP(mp)->mi_timeo = 11;
        MP_TO_MIP(mp)->mi_retrans = 4;
        bcopy(netblk_ptr->swapdesc,swpath,
                strlen(netblk_ptr->swapdesc));
        bcopy(netblk_ptr->srvname,MP_TO_MIP(mp)->mi_hostname,
                strlen(netblk_ptr->srvname));
        mp->m_fs_data = fsdata;
        swdevt[0].sw_nblks = swapsize;
        swdevt[0].sw_gptr = (int *)gp;
        swdevt[0].sw_strat = nfs_strategy;
        swdevt[0].sw_type = swaptype;
	swdevt[0].sw_dev = getpdev();
	cprintf("swap size - %d blocks\n", swapsize);
	nfs_resolvfh(&fhdl,swpath);
	bcopy(&fhdl,&(vtor((struct vnode *) gp)->r_fh), sizeof(fhandle_t));
}

ufs_swapconf()
{
        register int nblks;                             /* 001 */
        register struct swdevt *swp;                    /* 001 */
	int mainswp;

        /*
         *                      001
         *      Most of the rest of the routine was taken from autoconf.c.
         *      The reason was that to get the correct sizing of a device
         *      with the new partitioning scheme, it requires the device to be
         *      open. This is the first time we can do this because of
         *      buf structure being init. here.
         */
        for (swp = swdevt; swp->sw_dev; swp++) {
                swp->sw_type = swaptype;

		/* 
		 * if an open fails 
		 *	print an error message, mark this device so 
		 *	it's not counted and continue
		 */
                if ((*bdevsw[major(swp->sw_dev)].d_open)(swp->sw_dev)) {
		   printf("ufs_swapconf: Can not open swap device (%d,%d)\n",  
			major(swp->sw_dev), minor(swp->sw_dev));
			swp->sw_dev = (dev_t)-1;
			swp->sw_nblks = 0;
			continue;
		}

                if (bdevsw[major(swp->sw_dev)].d_psize) {
                        nblks =
                          (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
                }
                bdevsw[major(swp->sw_dev)].d_close(swp->sw_dev);

                if (swp->sw_nblks == 0 || swp->sw_nblks > nblks)
                        swp->sw_nblks = nblks;
        }

}


struct ifnet *ifunit();
getonnetwk()
{

	struct ifreq ifr;
	struct ifnet *ifp;
	char name[30];
	short flags;		/* do I need it? */
	struct socket *so;	/* only used as calling parameter */
	int	s;
	extern char boottype[];

	cprintf("root on %s:%s\n", netblk_ptr->srvname, netblk_ptr->rootdesc);
	if (swaptype == SW_NFS)
		cprintf("swap on %s:%s\n", netblk_ptr->srvname, netblk_ptr->swapdesc);
	else
		cprintf("swap on local disk\n");
	bzero(&ifr, sizeof(struct ifreq));
	bcopy(netblk_ptr->cliname,hostname,sizeof(netblk_ptr->cliname));


	bcopy(boottype,name,sizeof(name));
	bcopy(name,ifr.ifr_name,sizeof(ifr.ifr_name));
	hostnamelen = sizeof(hostname);
	ifr.ifr_addr.sa_family= AF_INET;
	
	if (ifioctl(so,SIOCGIFFLAGS,(caddr_t)&ifr)){
		cprintf("error SIOCGIFFLAGS\n");
		/* halt ? */
	}

	bcopy(name,ifr.ifr_name,sizeof(ifr.ifr_name));
	flags=ifr.ifr_flags;		/* do I need this */
	ifp=ifunit(ifr.ifr_name);
	ifr.ifr_addr.sa_family= AF_INET;


	((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr= ntohl(netblk_ptr->netmsk);
	s = splnet();
	if (in_control(so,SIOCSIFNETMASK,(caddr_t) &ifr,ifp))
		cprintf("error SIOCSIFNETMASK\n");
	splx(s);
	ifp=ifunit(ifr.ifr_name);

	((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr=ntohl(netblk_ptr->cliipadr); 
	s = splnet();
	if (in_control(so,SIOCSIFADDR,(caddr_t)&ifr,ifp))
		cprintf("error SIOCSIFADDR\n");
	splx(s);
	ifp=ifunit(ifr.ifr_name);


	((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr=ntohl(netblk_ptr->brdcst); 
	s = splnet();
	if (in_control(so,SIOCSIFBRDADDR,(caddr_t)&ifr,ifp))
		cprintf("error SIOCSIFBRDADDR\n");
	splx(s);

}

#define UFS_SMALL	1
#define UFS_MEDIUM	2
#define UFS_LARGE	3
#define UFS_XLARGE	4

ufs_system_size()
{
	extern int maxusers;

	if (maxusers < 32)
		return (UFS_SMALL);

	if (maxusers < 128)
		return (UFS_MEDIUM);

	if (maxusers < 256)
		return (UFS_LARGE);

	return (UFS_XLARGE);
}
