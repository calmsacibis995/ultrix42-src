#ifndef lint
static char *sccsid = "@(#)machdep.c	4.12    ULTRIX  3/6/91";
#endif	lint
/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/************************************************************************
 *
 *	Modification History: machdep.c
 *
 * 06-Mar-91	jaw
 *	3min ipl opt...
 *
 * 06-Nov-90 	Joe Szczypek
 *	Created char array which where bootpath will be saved.  This
 *	will be used by the network dump code during crashdumping.
 *	Also added check for tftp to getbootctlr().
 *
 * 13-Sep-90	Joe Szczypek
 *	Added support for 3max/3min TURBOchannel console callbacks.  Use
 *	rex_rex() to reboot, restart, and autoboot.  Use argv to get 
 *	bootdev if new console ROMs are present, and modified code getting
 *	"lnx" and "fzax" to use slot number instead of controller number to
 *	fetch appropriate entry from tc_slot structure.  Also added
 *	routine which will load up bootctlr for getsysinfo.
 *
 * 04-Sep-90	sekhar
 *	changes for memory mapped devices:
 *	modified useracc to disallow access to virtual addresses which 
 *	map I/O space.
 *
 * 31-Aug-90	Jim Paradis
 *	On sigreturn(), only restore the fp context if it had been
 * 	previously saved by sendsig().
 *
 * 04-Aug-90	Randall Brown
 *	Added getspl() and whatspl() routines that call the system specific
 *	routines through the cpu switch.  Added the spl_init() routine that
 *	loads the spl pointers with values from the cpu switch .
 *
 * 18-June-90 Fred L. Templin
 *	Added FZA for DS_5000. Code is running in compatibility mode.
 *
 * 15-June-90 Mark A. Parenti
 *      Moved stray() and passive_release here from vec_intr.c because
 *      vec_intr.c now gets built as Notbinary and we don't want to ship
 *      these sources.
 *
 * 6-June-90 Fred L. Templin
 *	Fixed "netbootchk()" for all LANCE and SGEC platforms
 *
 * 2-May-90 chc
 * 	Added the "ne" device for supporting the network boot 
 *
 * 24-Aprl-90 robin
 * fixed a bug in the R3000 that causes the cache isolate to fail if the 
 * write buffers are full is worked around via wbflush.  The fix is in the
 * clear_cache routines; we need to make sure the write buffers are NOT
 * full when we TRY to isolate the cache.  If the write buffer is
 * full the cache REALY is NOT isolated and every word get a zero low
 * order byte.... not good.
 *
 *
 * 29-Mar-90 gmm
 *	Changed some splhigh() to splextreme() since splhigh() now same
 *	as splclock()
 *
 * 06-Mar-90 gmm
 *	Added the routine alert_cpu() to update whichqs to ALLCPU under 
 *	lk_rq.
 *
 * 03-Mar-90 jaw
 *	primitive change to optimize mips.
 *
 * 14-Feb-90 -- gmm
 *	Avoid a race condition when starting more than one secondary 
 *	processor by switching affinity of the idle_proc to boot cpu.
 *
 * 05-Feb-90 -- dws
 *	Fixed handling of SIGCONT in sigreturn() for POSIX mode.
 *
 * 08-Dec-89 -- gmm
 *	Change the parent of the secondary's idle process to proc 0. When the
 *	idle process comes up on the secondary, it goes and removes itself 
 *	from the current parent's child queue and puts itself on proc 0's
 *	child queue.
 *
 * 14-Nov-89 -- gmm
 *	Changes to secondary_startup(), new routine init_idleproc() to start
 *	the idle process for the secondary, etc. Stop_secondary_cpu() now
 *	dumps the tlbs in core for debugging purposes.
 *
 * 09-Nov-89 -- jaw
 *	change references to maxcpu to smp.
 *
 * 26-Oct-89 -- afd
 *	for network boot, pick up network device unit number if its not 0.
 *
 * 19-Oct-89 -- jmartin
 *	Use PROT_{URKR|UW} instead of PG_{URKR|UW} as they are no longer
 *	the same thing.
 *
 * 13-Oct 89 -- gmm
 *	All MIPS specific smp changes. Changes to release_uarea_noreturn(),
 *	stop_secondary_cpu(), sig_parent_swtch(), new rotutines like intrcpu(),
 *	cpuident(), is_cpu(), secondary_startup() etc.
 *
 * 03-Oct-89 -- Joe Szczypek
 *	Changed set_lock, clear_lock, set_bit_atomic, and clear_bit_atomic
 *      to use interlocked operations if system is MP.
 *
 * 25 Jul 89 -- chet
 *	Change unmount and cache flushing code in boot()
 *
 * 24-July-89 -- Alan Frechette
 *	Moved all the crashdump code to a new file. The new file
 *	containing the crashdump code is "/sys/sys/crashdump.c".
 *
 * 18-July-89	kong
 *	Rewrote the routine useracc.  The original routine was
 *	located in usercopy.s and checks user access by reading
 *	and writing the page.  If the cache is incoherent at the
 *	time, or if DMA is going on, the contents of the buffer
 *	being probed gets corrupted.  The original routine also
 *	has the side-effect of causing paging in of pages.
 *	The new routine simply checks the protection field of the
 *	PTE to determine if access is allowed.  This is similar
 *	to the implementation on a VAX which uses the PROBEr/w instruction
 *	to check access.  
 *
 * 10-July-89	burns
 *	Added emulation for Vax crc instruction (from pmk).
 *	Added cpu switch routines for mips specific cache routines to
 *		clean_icache, clean_dcache, page_iflush and page_dflush.
 * 
 * 21-Jul-89 -- Fred Canter
 *	Conditionally include devio.h (errlog.h includes devio.h).
 *
 * 19-Jun-89 -- condylis
 *	Tightened up unmounting of file systems in boot().
 *
 * 15-Jun-1989  afd
 * 	Added cpu_initialize routine to call system specific initialization
 *	routine (for splm, intr vectors, hz, etc).
 *
 * 15-June-1989 kong
 *	Changed code in boot to work around (not fix) a panic hang,
 *	see the comments in "boot".
 *
 * 12-Jun-1989 -- gg
 *	In dumpsys() added a check for the presence dumpdev.
 *
 * 09-Jun-1989  gmm
 *	Call switch_to_idle_stack() in release_uarea_noreturn() before
 *	releasing the process's memory resources.
 *
 * 06-June-1989 robin
 *	Changed gendump to dump to the device specified in the config
 *	file as the bump device.  This applies to devices on non-boot
 *	controllers.  I also put in a routine to convert an integer to
 *	an ascii string (didn't know where to put it; its used here so 
 *	here it lives?).
 *
 * 20-Apr-1989	afd
 * 	In dumpsys(), if console is a graphics device, force printf messages
 *	directly to screen.  This is needed here for the case when we
 *	manually start the dump routine from console mode.
 *
 * 07-Apr-1989	afd
 *	Call wbflush, flush_cache, microdelay, & clock routines thru
 *	 cpu switch.
 *
 * 06-Apr-89 -- prs
 *      Added SMP accounting lock in boot().
 *
 * 28-Feb-1989  Kong
 *	Added routine "startrtclock", "ackrtclock", and "stopclocks".
 *	These routines are machine dependent and should eventually
 *	go through the  cpu switch table.
 *
 * 24-Feb-1989  Kong
 *	Added variable "Physmem" to keep track of the size of
 *	physical memory actually in the system.  This variable
 *	is used by "sizer" to generate the proper config file.
 *
 * 20-Feb-1989  Kong
 *	Added routine "badaddr".  As in VAXen, "badaddr" needs to
 *	be machine dependent because of the differences in I/O buses
 *	used.
 *
 * 13-Jan-1989	Kong
 *	Add the use of a system switch table.  From the variable "cpu"
 *	index into the system switch table to determine what system
 *	specific routine to call.
 *
 * 29-Dec-1988  afd
 *	In dumpsys(), set dumpsize BEFORE calling netdump().
 *	Otherwise, dumpsize is zero and savecore saves zero bytes.
 *
 *************************************************************************/


#include "../machine/pte.h"
#include "../machine/entrypt.h"

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
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/mount.h"
#include "../h/errlog.h"
#ifdef QUOTA
#include "../h/quota.h"
#endif QUOTA
#include "../h/exec.h"
#include "../h/signal.h"
#include "../h/fs_types.h"
#include "../h/dump.h"

#include "../io/uba/ubavar.h"
#include "../io/tc/tc.h"

#include "../machine/cpu.h"
#include "../machine/fpu.h"
#include "../machine/reg.h"
#include "../../machine/common/cpuconf.h"

#include "../net/net/netisr.h"
#include "../h/ioctl.h"
#ifndef	DEVIO_INCLUDE
#include "../h/devio.h"
#endif	DEVIO_INCLUDE
#include "../fs/ufs/fs.h"
#include "../sas/mop.h"
#include "../h/cpudata.h"
#include "../h/ipc.h"
#include "../h/shm.h"


extern int cpu;		/* System type, value defined in system.h */
extern struct cpusw *cpup;	/* Pointer to cpusw entry for this machine*/
extern int console_magic; /* magic number of console */
char netdevice[80];

#ifdef PGINPROF
/* NOT ported to MIPS yet! */
/*
 * Return the difference (in microseconds)
 * between the  current time and a previous
 * time as represented  by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
vmtime(otime, olbolt, oicr)
	register int otime, olbolt, oicr;
{

	if (mfpr(ICCS)&ICCS_INT)
		return(-1);
	else
		return(((time.tv_sec-otime)*60 + lbolt-olbolt)*16667 + mfpr(ICR)-oicr);
}
#endif

/*
 * Clear registers on exec
 */
setregs(entry)
	long entry;
{
	register int i;
	register struct proc *p = u.u_procp;
	register int *rp;

	for (rp = &u.u_ar0[EF_AT]; rp < &u.u_ar0[EF_GP];)
		*rp++ = 0;
	u.u_ar0[EF_S8] = 0;
	u.u_ar0[EF_RA] = 0;
	u.u_ar0[EF_MDLO] = 0;
	u.u_ar0[EF_MDHI] = 0;
	u.u_ar0[EF_EPC] = (int)entry;
	for (rp = u.u_pcb.pcb_fpregs; rp < &u.u_pcb.pcb_fpregs[32];)
		*rp++ = 0;
	u.u_pcb.pcb_fpc_csr = 0;
	u.u_pcb.pcb_fpc_eir = 0;
	u.u_pcb.pcb_ownedfp = 0;
	u.u_pcb.pcb_sstep = 0;
	u.u_pcb.pcb_ssi.ssi_cnt = 0;
}

#define	R_ZERO	0
#define	R_AT	1
#define	R_V0	2
#define	R_A0	4
#define	R_A1	5
#define	R_A2	6
#define	R_A3	7
#define	R_SP	29

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode in libc
 * to call user signal handling routine, followed by syscall
 * to sigreturn routine below.  After sigreturn
 * restores critcal registers, resets the signal mask and the
 * stack, it returns to user mode.
 */

#define	STACK_ALIGN(x)	((unsigned)(x) &~ ((4*sizeof(int))-1))

sendsig(p, sig, signalmask)
int (*p)(), sig, signalmask;
{
	register struct sigcontext *scp;
	register int *srp;
	register u_int *urp;
	register int *frp;
	int oonstack;

	XPRINTF(XPR_SIGNAL,"enter sendsig %d, 0x%x",sig,signalmask,0,0);
	oonstack = u.u_onstack;

	/*
	 * decide which stack signal is to be taken on, and make
	 * sure its aligned and accessable.
	 */
	if (!u.u_onstack && (u.u_sigonstack & sigmask(sig))) {
		scp = (struct sigcontext *)STACK_ALIGN(u.u_sigsp) - 1;
		u.u_onstack = 1;
	} else
		scp = (struct sigcontext *)STACK_ALIGN(USER_REG(EF_SP)) - 1;

	if (!u.u_onstack && (int)scp <= USRSTACK - ctob(u.u_ssize))
		grow((unsigned)scp);
	if (!useracc((caddr_t)scp, sizeof (struct sigcontext), B_WRITE))
		goto bad;

	/*
	 * Save into the sigcontext the signal state and that portion
	 * of process state that we trash in order to transfer control
	 * to the signal trampoline code.
	 * The remainder of the process state is preserved by
	 * signal trampoline code that runs in user mode.
	 */
	scp->sc_onstack = oonstack;
	scp->sc_mask = signalmask;
	scp->sc_pc = USER_REG(EF_EPC);
	scp->sc_regs[R_V0] = USER_REG(EF_V0);
	scp->sc_regs[R_A0] = USER_REG(EF_A0);
	scp->sc_regs[R_A1] = USER_REG(EF_A1);
	scp->sc_regs[R_A2] = USER_REG(EF_A2);
	scp->sc_regs[R_A3] = USER_REG(EF_A3);
	scp->sc_regs[R_SP] = USER_REG(EF_SP);

	/*
	 * if this process has ever used the fp coprocessor, save
	 * its state into the sigcontext
	 */
	scp->sc_ownedfp = u.u_pcb.pcb_ownedfp;
	if (u.u_pcb.pcb_ownedfp) {
		checkfp(u.u_procp, 0);	/* dump fp to pcb */
		for (srp = &scp->sc_fpregs[0], frp = u.u_pcb.pcb_fpregs;
		    frp < &u.u_pcb.pcb_fpregs[32]; srp++, frp++)
			*srp = *frp;
		scp->sc_fpc_csr = u.u_pcb.pcb_fpc_csr;
		scp->sc_fpc_eir = u.u_pcb.pcb_fpc_eir;
		u.u_pcb.pcb_fpc_csr &= ~FPCSR_EXCEPTIONS;
	}
	scp->sc_cause = USER_REG(EF_CAUSE);
	scp->sc_badvaddr = USER_REG(EF_BADVADDR);

	/*
	 * setup registers to enter signal handler
	 * when resuming user process
	 */
	USER_REG(EF_A0) = sig;
	if (sig == SIGFPE || sig == SIGSEGV || sig == SIGILL ||
	    sig == SIGBUS || sig == SIGTRAP) {
		USER_REG(EF_A1) = u.u_code;
		u.u_code = 0;
	} else
		USER_REG(EF_A1) = 0;
	USER_REG(EF_A2) = (unsigned)scp;
	USER_REG(EF_A3) = (unsigned)p;
	USER_REG(EF_SP) = STACK_ALIGN(scp);
	USER_REG(EF_EPC) = (unsigned)u.u_sigtramp;
	XPRINTF(XPR_SIGNAL,"exit sendsig %d, 0x%x",sig,signalmask,0,0);
	return;

bad:
	uprintf("sendsig: can't grow stack, pid %d, proc %s\n", 
		u.u_procp->p_pid, u.u_comm);
	/*
	 * Process has trashed its stack; give it an illegal
	 * instruction to halt it in its tracks.
	 */
	u.u_signal[SIGILL] = SIG_DFL;
	sig = sigmask(SIGILL);
	u.u_procp->p_sigignore &= ~sig;
	u.u_procp->p_sigcatch &= ~sig;
	u.u_procp->p_sigmask &= ~sig;
	psignal(u.u_procp, SIGILL);
}

/*
 * Routine to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 */
sigreturn()
{
	register int *srp;
	register u_int *urp;
	register int *frp;
	struct a {
		struct sigcontext *sigcontextp;
	} *uap = (struct a *)u.u_ap;
	register struct sigcontext *scp;

	scp = uap->sigcontextp;
	if (!useracc((caddr_t)scp, sizeof (*scp), B_READ))
		return;
	u.u_eosys = FULLRESTORE;
	u.u_onstack = scp->sc_onstack & 01;
	if (u.u_procp->p_progenv == A_POSIX) u.u_procp->p_sigmask = 
		scp->sc_mask &~ (sigmask(SIGKILL)|sigmask(SIGSTOP));
	else	u.u_procp->p_sigmask = 
		scp->sc_mask &~ (sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP));
	/*
	 * copy entire user process state from sigcontext into
	 * exception frame, a special exit from syscall insures
	 * that the entire exception frame gets restored
	 */
	USER_REG(EF_EPC) = scp->sc_pc;
	USER_REG(EF_MDLO) = scp->sc_mdlo;
	USER_REG(EF_MDHI) = scp->sc_mdhi;
	for (urp = &USER_REG(EF_AT), srp = &scp->sc_regs[R_AT];
	    urp <= &USER_REG(EF_RA); urp++, srp++)
			*urp = *srp;
	if (scp->sc_ownedfp) {
		checkfp(u.u_procp, 1);	/* toss current fp contents */
		for (frp = u.u_pcb.pcb_fpregs, srp = &scp->sc_fpregs[0];
		    frp < &u.u_pcb.pcb_fpregs[32]; frp++, srp++)
			*frp = *srp;
		u.u_pcb.pcb_fpc_csr = scp->sc_fpc_csr & ~FPCSR_EXCEPTIONS;
	}
}

physstrat(bp, strat, prio)
	struct buf *bp;
	int (*strat)(), prio;
{
	int s;
	int count = bp->b_bcount;
	caddr_t addr = bp->b_un.b_addr;

	(*strat)(bp);
	/* pageout daemon doesn't wait for pushed pages */
	if (bp->b_flags & (B_DIRTY|B_RAWASYNC))
		return;
	s = splclock();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t)bp, prio);
	splx(s);
#ifdef oldmips
	if (bp->b_flags & B_READ)
		bufflush(bp);	/* do this if you do dma....rr */
#endif oldmips
}

int	waittime = -1;
int	shutting_down = 0;

boot(paniced, arghowto)
int	paniced, arghowto;
{
	register int	howto;			/* r11 == how to boot */
	register int	devtype;		/* r10 == major of root dev */
	register struct mount *mp;
	register struct gnode *gp;
	register struct pte *ptep;
	register int s;
	struct gnode *rgp;
	extern struct gnode *fref();
	extern struct gnode *acctp;
	extern struct gnode *savacctp;
	extern struct cred *acctcred;
	extern struct lock_t lk_acct;
#ifdef lint
	howto = 0;
	devtype = 0;
	printf ("howto %d, devtype %d\n", arghowto, devtype);
#endif
	(void) splextreme();
	howto = arghowto;
	rundown++;
	shutting_down++;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
		/*
		 * If accounting is on, turn it off. This allows the usr
		 * filesystem to be umounted cleanly.
		 */
		smp_lock(&lk_acct, LK_RETRY);
		if (savacctp) {
			acctp = savacctp;
			savacctp = NULL;
		}
		if (gp = acctp) {
	        	gfs_lock(gp);
			if (acctp != NULL) {
		        	acctp = NULL;
				GSYNCG(gp, acctcred);
				crfree(acctcred);
				acctcred = NULL;
				gput(gp);
			} else
			        gfs_unlock(gp);
		}
		smp_unlock(&lk_acct);

		(void) splnet();
		waittime = 0;

		gfs_gupdat(NODEV);
		if (paniced != RB_PANIC) {
			update(); /* flush dirty blocks */
			/* unmount all but root fs */
			/* include paranoid checks for active unmounts */
			/* and preclude new unmounts */
			for (mp = &mount[NMOUNT - 1]; mp > mount; mp--) { 
				smp_lock(&mp->m_lk, LK_RETRY);
				if ((mp->m_flgs & MTE_DONE) &&
				    !(mp->m_flgs & MTE_UMOUNT)) {
					mp->m_flgs |= MTE_UMOUNT;
					smp_unlock(&mp->m_lk);
					GUMOUNT(mp, 1);
				}
				else {
					smp_unlock(&mp->m_lk);
				}
			}
		}

		printf ("syncing disks... ");
		(void) splhigh();
		bflush (NODEV, (struct gnode *) 0, 1); 
		printf ("done\n");
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
	(void) splextreme();
	devtype = major (rootdev);
	(void) save();
	if (howto & RB_HALT) {
		printf("halting.... (transferring to monitor)\n\n");
		if(console_magic != REX_MAGIC)
			prom_restart();	/* always enters command mode */
		else {
			rex_rex('h');
		}
		/* doesn't return */
	} else {
		if (paniced == RB_PANIC) {
			dumpsys();
			if(console_magic != REX_MAGIC)
				prom_reboot();	/* follows $bootmode */
			else {
				rex_rex('r');
			}
			/* doesn't return */
		}
	}
	printf("rebooting.... (transferring to monitor)\n\n");
	if(console_magic != REX_MAGIC)
		prom_autoboot();	/* always reboots */
	else {
		rex_rex('b');
	}
	/* doesn't return */
	for (;;)	/* chicken */
		;
}

ovbcopy(from,to,len)
register char *from,*to;
register int len;
{
	if (from < to) {
		from += len;
		to += len;
		while(len--)
			*--to = *--from;
	} else {
		while(len--)
			*to++ = *from++;
	}
}

char boottype[10];
struct netblk netblk;

/*
 * called from init_main to see if a network boot has occurred.  If so,
 * available information is read into a local copy of the netblk
 * structure.  As per original design the changing of 'roottype' is what
 * triggers init_main to assume a diskless network environment.
 */
netbootchk()
{
	extern struct netblk *netblk_ptr;
	extern int roottype;
	extern int swaptype;
	extern int dumptype;
	extern int swapsize;
	extern char *bootdev;
	extern int console_magic;
	extern char **ub_argv;
	extern int ub_argc;
	char *bootstring;
	int i;

	/*
	 * Note: network boot can be for diskless or Standalone 
	 * ULTRIX.  The caller (init_main.c) checks roottype to
	 * sense the difference.
	 */
	
	boottype[0] = '\0';
	if(console_magic != REX_MAGIC)
		bootdev = (char *)prom_getenv("boot");
	else {
		for(i=1;i<ub_argc;i++) {
			if((ub_argv[i][0] != '-') && (ub_argv[i][0] != NULL))
				if(ub_argv[i][1] == '/') 
					bootdev = (char *)ub_argv[i];
		}
	}

	i=0;
	while(bootdev[i] != NULL) {             /* Save for network crashing */
	        netdevice[i] = bootdev[i];
		i++;
	}
	netdevice[i] = bootdev[i];

	if ((!strncmp(bootdev, "mop", 3)) || 
	    (!strncmp(bootdev, "tftp", 4)) ||
	    (!strncmp(&bootdev[2], "mop", 3)) ||
	    (!strncmp(&bootdev[2], "tftp", 4))) { 
		int i; char *j;

		/*
		 * Take a picture of the netblk structure in case
		 * it gets tromped on by someone other than the
		 * kernel.
		 */
		bcopy((char *)NETBLK_LDADDR, (char *)&netblk,
			sizeof (struct netblk));
		/*
		 * Clear out that which was just copied so it isn't
		 * hanging around for a subsequent boot.  This will
		 * guarantee that NETBLK_LDADDR points to real data
		 * pertaining to this boot.
		 */
		j = (char *)NETBLK_LDADDR;
		for (i = 0; i < sizeof (struct netblk); i++)
			j[i] = 0;
		netblk_ptr = &netblk;
		switch (cpu) {
			case DS_5000:
			case DS_5000_100:
		            if (console_magic != REX_MAGIC) {
			    /*
			     * Walk the TURBOchannel looking for the nth
			     * instance of a LANCE or DEFZA.
			     */
			      if ((bootdev[4] > '0') && (bootdev[4] <= '3')) {
				int k, unit = bootdev[4] - '0';
				extern struct tc_slot tc_slot[];
				for (k = 0; k < 3; k++) {
				  if (!strcmp(tc_slot[k].devname,"ln"))
				    if (--unit == 0) {
				      bcopy("ln",boottype,2);
				      boottype[2] = tc_slot[k].unit+'0';
				      boottype[3] = '\0';
				      break;
				    }
				  if (!strcmp(tc_slot[k].devname,"fza"))
				    if (--unit == 0) {
				      bcopy("fza",boottype,3);
				      boottype[3] = tc_slot[k].unit+'0';
				      boottype[4] = '\0';
				      break;
				    }
				}

			      } else {
				/*
				 * For unit = 0 and default, use "ln0"
				 */
				bcopy("ln0",boottype,3);
				boottype[3] = '\0';
			      }
			    }

			    else { /* New TURBOchannel console */
			      int k,j;
			      extern struct tc_slot tc_slot[];
			      /*
			       * Walk the TURBOchannel looking for the nth
			       * instance of a LANCE or DEFZA.
			       */
			      j=bootdev[0]-0x30;
			      for(k = 0; k <= 6; k++) {
				if(tc_slot[k].slot == j) {
				  if (!strcmp(tc_slot[k].devname,"ln")) {
				    bcopy("ln",boottype,2);
				    boottype[2] = tc_slot[k].unit+'0';
				    boottype[3] = '\0';
				    break;
				  }
				  if (!strcmp(tc_slot[k].devname,"fza")) {
				    bcopy("fza",boottype,3);
				    boottype[3] = tc_slot[k].unit+'0';
				    boottype[4] = '\0';
				    break;
				  }
				}
			      } /* END FOR */
			    } /* END ELSE */
			    break;
			case DS_5500:
				bcopy("ne0",boottype,3);
				boottype[3] = '\0';
				break;
			case DS_3100:
			case DS_5100:
			case DS_5400:
			default:
				bcopy("ln0",boottype,3);
				boottype[3] = '\0';
				break;
		}
		if (netblk_ptr->rootfs == GT_NFS) {
			roottype= (int) netblk_ptr->rootfs;
			swaptype= (int) netblk_ptr->swapfs;
			swapsize= ((int) netblk_ptr->swapsz) * 1024;
			if (netblk_ptr->dmpflg != -1)
				dumptype= ((int) netblk_ptr->dmpflg) * 1024;
		}
	}
}

/* 
 * Call system specific configuration routine.
 */
configure()
{
	if ((*(cpup->config))() < 0)
		panic("No configuration routine configured\n");
}

/*
 * Delay for n microseconds
 * Call through the system switch to specific delay routine.
 */
microdelay(usecs)
	int usecs;
{
	(*(cpup->microdelay))(usecs);
}

read_todclk()
{
	extern int nocpu();
	if (cpup->readtodr == nocpu)
		panic("No read TOD routine configured\n");
	else
		return((*(cpup->readtodr))());
}
write_todclk()
{
	extern int nocpu();
	if (cpup->writetodr == nocpu)
		panic("No write TOD routine configured\n");
	else
		return((*(cpup->writetodr))());
}

/*
 * Wait until the write buffer of the processor we are on is empty.
 * In "critical path code", don't check return status.
 */
wbflush()
{
	(*(cpup->wbflush))();
}

/*
 * Machine dependent badaddr.
 *
 * On machines such as on the BI, access to NXM in BI does not
 * necessarily cause error interrupts or exceptions.  The only
 * error indication may be the setting of error bits in error registers.
 */
badaddr(addr,len)
caddr_t addr;
int len;
{
	int status, s;
	extern int cold;	/* booting in progress */


	if (cold)
		status =  (*(cpup->badaddr))(addr,len);
	else {
		s = splextreme();	/* Disable interrupts */
		switch(cpu) {
		case DS_3100:
		case DS_5000:
		case DS_5000_100:
		case DS_5100:
		case DS_5400:
		case DS_5500:
		case DS_5800:
		default:
			status = (*(cpup->badaddr))(addr,len);
			break;
		}
		splx(s);	/* Restore interrupts */
	}
	return(status);
}

release_uarea_noreturn() 
{
	
	CURRENT_CPUDATA->cpu_exitproc = u.u_procp;
	(void)splclock();
	smp_lock(&lk_rq,LK_RETRY);
	swtch();
}

startrtclock()
{
	if ((*(cpup->startclocks))() < 0)
		panic("No start clock routine configured\n");
}

spl5() {
	return(splbio());
}


/*
 * Acknowledge clock interrupt.
 * In "critical path code", don't check return status.
 */
ackrtclock()
{
	(*(cpup->ackclock))();
}

stopclocks()
{
	if ((*(cpup->stopclocks))() < 0)
		panic("No stop clock routine configured\n");
}

/*
 * Flush the system caches.
 * In "critical path code", don't check return status.
 */
flush_cache()
{
/* A bug in the R3000 that causes the cache isolate to fail if the write buffers
 * are full is worked around via wbflush.
 */
	int s;
	s = splextreme();
	(*(cpup->wbflush))();
	(*(cpup->flush_cache))();
	splx(s);
}

clean_icache (addr,len)
{
/* A bug in the R3000 that causes the cache isolate to fail if the write buffers
 * are full is worked around via wbflush.
 */
	int s;
	s = splextreme();
	(*(cpup->wbflush))();
	(*(cpup->clean_icache))(addr,len);
	splx(s);
}

clean_dcache (addr,len)
{
/* A bug in the R3000 that causes the cache isolate to fail if the write buffers
 * are full is worked around via wbflush.
 */
	int s;
	s = splextreme();
	(*(cpup->wbflush))();
	(*(cpup->clean_dcache))(addr,len);
	splx(s);
}

page_iflush (addr)
{
/* A bug in the R3000 that causes the cache isolate to fail if the write buffers
 * are full is worked around via wbflush.
 */
	int s;
	s = splextreme();
	(*(cpup->wbflush))();
	(*(cpup->page_iflush))(addr);
	splx(s);
}

page_dflush (addr)
{
/* A bug in the R3000 that causes the cache isolate to fail if the write buffers
 * are full is worked around via wbflush.
 */
	int s;
	s = splextreme();
	(*(cpup->wbflush))();
	(*(cpup->page_dflush))(addr);
	splx(s);
}

/* 
 * Call system specific initialization routine (for splm, intr vectors, etc).
 */
cpu_initialize()
{
	if ((*(cpup->init))() < 0)
		panic("No initialization routine configured\n");
}

getspl()
{
        return((*(cpup->getspl))());
}

whatspl()
{
        return((*(cpup->whatspl))());
}

setlock(l) 
struct lock_t *l;
{
        if(!smp)                 /* If 1 CPU, don't need interlocked operations */

	  if (l->l_lock) return(0);

	  else {
	    l->l_lock=1;
	    return(1);
	  }
	
	else return(bbssi(31,l));        /* Set bit 31 with interlocked operation */
}

clearlock(l) 
struct lock_t *l;
{
        if(!smp)                 /* If 1 CPU, don't need interlocked operations */
	  l->l_lock = 0;

	else return(bbcci(31,l));        /* Clear bit 31 with interlocked operation */
	  
}

struct second_tlb {			/* dump tlb information */
	union tlb_hi tlb_high;
	union tlb_lo tlb_low;
} second_tlb[NTLBENTRIES];
stop_secondary_cpu()
{
	struct cpudata *pcpu;
	pcpu = CURRENT_CPUDATA;
	save();		/* save process's context in u pcb */
/* What to do to flush write buffers */
	save_tlb(second_tlb); /* for debugging purpose */
	pcpu->cpu_state &= ~CPU_RUN;
	pcpu->cpu_state |= CPU_STOP;
	for(;;)  /* Something else ?? */
	    ;
}

sig_parent_swtch()
{
	/* the caller to sig_parent_swtch() should have got lk_rq */
	CURRENT_CPUDATA->cpu_state |= CPU_SIGPARENT;
	swtch();
}

set_bit_atomic(bitpos,base)
unsigned long bitpos;
unsigned long *base;
{
        if(!smp) {                /* If 1 CPU, don't use interlocked instructions */
	  if ((*base) & (1<<bitpos)) return(0);
	  *(base) |= (1<<bitpos);
	  return(1);
	}
	else return(bbssi(bitpos,base));  /* MP, use interlocked operation */
}

clear_bit_atomic(bitpos,base)
unsigned long bitpos;
unsigned long *base;
{
        if(!smp)                  /* If 1 CPU, don't use interlocked instructions */
	  if ((*base) & (1<<bitpos)) {
	    *(base) &= ~(1<<bitpos);
	    return(1);
	  }
	  else return(0);
	else return(bbcci(bitpos,base));  /* MP, use interlocked operation */
}

extern char *kn5800_ip[];
intrcpu(whichcpu)
int whichcpu;
{
	switch (cpu) {

	      case DS_5800:
		*kn5800_ip[whichcpu] = 0; /* this should  a byte-type instruction */
		break; 
		
	      default:
		break;
	}
}

cpuident()
{
	switch (cpu) {

	      case DS_5800:
	        return(kn5800_cpuid());

	     default:
		return(0);
       }
}

int start_stack_held = 0; /* indicates if any cpu is using the startup stack */
int current_secondary; /* secondary cpu currently being started up */
struct proc *idle_proc;    /* idle proc of the currently starting cpu*/
extern struct user *boot_idle_up;
extern unsigned cputype_word, fptype_word;

secondary_startup()
{
	int i;
	unsigned cputype,fptype;
	splextreme();
	clear_bev();
	for (i=0; i < NTLBENTRIES; i++)
		invaltlb(i);

	tlbwired(TLBWIREDBASE, 0, UADDR,
	 K0_TO_PHYS(boot_idle_up)>>(PGSHIFT-PTE_PFNSHIFT) | PG_M | PG_V | PG_G);
	u.u_pcb.pcb_cpuptr = cpudata[current_secondary];
	flush_cache();
	cputype = get_cpu_irr();
	if(cputype != cputype_word) {
		printf("WARNING: cpu %d version (0x%X) does not match with boot cpu version  (0x%X)\n",current_secondary,cputype,cputype_word);
	}
	fptype = get_fpc_irr();
	if( (fptype &= IRR_IMP_MASK) != fptype_word) {
		printf("WARNING: FPU version (0x%X) of cpu %d does not match with boot cpu's FPU version (0x%X)\n",fptype,current_secondary,fptype_word);
	}
	fp_init();
	secondary_init();  /* cpu specific secondary initialization */
	splclock();  /* somebody lowers the ipl */
	smp_lock(&lk_rq,LK_RETRY);  /* synchs with the swtch of idle proc */
	CURRENT_CPUDATA->cpu_proc = idle_proc;
	init_tlbpid();
	remrq(idle_proc);
	get_tlbpid(idle_proc);
	smp_unlock(&lk_rq);
	idle_proc->p_cpumask = CURRENT_CPUDATA->cpu_mask;
	CURRENT_CPUDATA->cpu_state = CPU_RUN;
	startrtclock();
	resume(pcbb(idle_proc));    /* resumes the idle proc on the secondary.
				       see init_idleproc() below */
}

secondary_init()
{
	/* should be through cpusw when more MP MIPS system become ready*/
	switch(cpu) {
	      case DS_5800:
		kn5800_init_secondary();
		return; 
		
	      default:
		return;
	}
}

init_idleproc(cpunum)
{
	int idlepid,found,s;
	register struct proc *cptr,*nq;

	while(start_stack_held)  /* if some other cpu 
			    using the startup stack */
		sleep((caddr_t)&start_stack_held,PZERO+1); 
	start_stack_held = 1;
	idlepid = get_proc_slot();
	if(idlepid == 0) {
		tablefull("proc");
		u.u_error = EAGAIN;
		start_stack_held = 0;
		wakeup((caddr_t)&start_stack_held);
		return(0);  /* get_proc_slot sets 
			    u.u_error to EAGAIN */
	}
	current_secondary = cpunum;
	idle_proc = &proc[idlepid];
	if(newproc(idle_proc,0)) {
		switch_affinity(boot_cpu_mask); /* Avoid a race 
				   condiation when more than 1 processor 
				   already running. Else the parent may be put
				   to sleep after the wakeup in the child 
				   below */
		bcopy("idleproc",(caddr_t)&u.u_comm[0], MAXCOMLEN);
		idle_proc->p_affinity = 1<<current_secondary;
		idle_proc->p_type |= SSYS;
		idle_proc->p_sched |= SLOAD;
		splclock();
		smp_lock(&lk_rq,LK_RETRY);
		setrq(idle_proc);
		idle_proc->p_mips_flag |= SIDLEP;
		/* wake up the parent process, when 
		   child proc is all set up */
		wakeup((caddr_t)idle_proc);
		if(save()) {
			if(CURRENT_CPUDATA->cpu_num != current_secondary) 
				panic("idle proc not back on the correct secondary");
			CURRENT_CPUDATA->cpu_idleproc = idle_proc;
			s = splclock();
			smp_lock(&lk_procqs,LK_RETRY);
			cptr = idle_proc->p_pptr->p_cptr;
			found = 0;
			/* make swapper the parent */
			while(cptr) {
			    if (cptr == idle_proc) {
				   idle_proc->p_pptr->p_cptr = cptr->p_osptr;
				   nq = idle_proc->p_osptr;
				   if (nq != NULL)
					nq->p_ysptr = NULL;
				   if(proc[0].p_cptr)
					proc[0].p_cptr->p_ysptr = idle_proc;
				   idle_proc->p_osptr = proc[0].p_cptr;
				   idle_proc->p_ysptr = NULL;
				   proc[0].p_cptr = idle_proc;
				   idle_proc->p_pptr = &proc[0];
				   idle_proc->p_ppid = 0;
				   found = 1;
				   break;
 			     }
			     cptr = cptr->p_osptr;
			}
			if(found == 0) 
				panic("init_idleproc: not found in child queue");
			smp_unlock(&lk_procqs);
			splx(s);
			start_stack_held = 0;
			wakeup((caddr_t)&start_stack_held);
			idle();
	        }
	        start_idleproc(); /* start the primary's idle process */
	}
	/* make the parent sleep till child sets up the
	   idle proc ready for the secondary cpu */
	sleep((caddr_t)idle_proc,PZERO-1);
	return(1); /* success */
}

u_long ci_ucode = 0;

alert_cpu()     /* inform every cpu that this process is available */
{
	int s;
	s = splclock();
	smp_lock(&lk_rq,LK_RETRY);
	whichqs = ALLCPU; 
	smp_unlock(&lk_rq);
	splx(s);
}

crc( ctp, inicrc, len, dp )
register char *ctp;
register u_long inicrc;
register u_long len;
register char *dp;
{
	register u_long index;

	while( len > 0 ) {

	    inicrc = (((char)inicrc ^ *dp++) & 0x0ff) | (inicrc & 0xffffff00);
	    index = 0x0f & inicrc;
	    inicrc = inicrc >> 4;
	    inicrc ^= *((u_long *)ctp + index);
	    index = 0x0f & inicrc;
	    inicrc = inicrc >> 4;
	    inicrc ^= *((u_long *)ctp + index);

	    --len;
	}
	
	return(inicrc);

}

/*
 * Routine to check if the process has read or write access
 * to a segment of user space given by its virtual address
 * and length in bytes.
 *
 * useracc must be called in process context.
 *
 * Parameters:
 *	va	Virtual address.  
 *	len	Number of bytes to check.  Must be greater than 0.
 *	rw 	= 1 for read access check, 0 for write access check.
 *
 * Returns:
 *	0	Access not allowed.
 *	1	Access is allowed.
 */
useracc(va,len,rw)
unsigned va;		/* Process virtual address */
int	len;		/* Length in bytes, must be > 0 */
int	rw;		/* mode of access check */
{
	register struct proc *p;
	register unsigned vpn;
	register struct pte *pte;
	register int prot;
	int smindex;

	if (!IS_KUSEG(va)) {
		/* Address not in user space, NO Access */
		return(0);
	}

	prot = rw ? PROT_URKR : PROT_UW;

	p = u.u_procp;

	/*
	 * Check access right for each page, exit
	 * the loop and return 0 if one of the pages
	 * cannot be accessed.
	 */
	for (vpn = btop(va); vpn <= btop(va+len-1); vpn++) {
		pte = vtopte(p,vpn);
		if (pte == 0)
			return(0);	/* No access */
		/*
		 * If I/O space is mapped to virtual addresses 
		 * then return 0. this prevents undesired access
		 * to address space created by the mmap system call.
		 */
		if (isasmsv(p,vpn,&smindex) && /* SHMEM */
		    (p->p_sm[smindex].sm_p->sm_perm.mode & IPC_MMAP))
			return(0);

		if (pte->pg_prot < prot) {
			/* Access is not allowed.  If SHMEM check
			 * elsewhere for true protection.
			 */
			if (isasmsv(p,vpn,&smindex) && /* SHMEM */
				(p->p_sm[smindex].sm_pflag) >= prot)
				continue;
			return(0);
		}
	}
	
	return(1);	/* Access allowed */
}
/*
 * Routine to handle stray interrupts.
 * Parameter:
 *	ep		Pointer to exception frame.
 *	vec		The vector obtained from reading the vector register.
 *
 */
stray(ep,vec)
int *ep;
int vec;
{
  extern int cpu;
  if (cold) {
	if ((cpu == DS_5400) || (cpu == DS_5500))
		cvec = (vec - 0x200) & 0xfffe;
	else
		cvec = vec;
  }
  else {
	cprintf("Stray intr, vec=0x%x\n",vec);
  }
}


/*
 * Passive release
 */
passive_release(ep,vec)
int *ep;
int vec;
{
}

int mips_spl_arch_type =0;
spl_init()
{
	if (cpu == DS_5000_100) 
		mips_spl_arch_type =1;
}

/*
 * Routine to convert TURBOchannel boot slot into logical controller number.
 */
getbootctlr()
{
  extern char **ub_argv;
  extern int rex_base;
  extern char bootctlr[];
  extern char consmagic[];
  extern struct tc_slot tc_slot[];
  char *cp;
  int i;
  
  bootctlr[0] = '\0';
  cp = (char *)&console_magic;
  for(i=0;i<4;i++)
	consmagic[i] = *cp++;
  consmagic[4]='\0';
  if(!rex_base) {
    return(0);
  }
  else {
    cp = (char *)&ub_argv[1][0];
    while(*cp != '/')
      cp++;
    cp--;
    if(((strncmp(cp+2,"rz",2)==0)) ||
       (strncmp(cp+2,"tz",2) == 0)) {
      for(i=0;i<=8;i++) {
	if((strcmp(tc_slot[i].devname,"asc")==0) &&
	   (tc_slot[i].slot == *cp - '0')) {
	  bootctlr[0] = tc_slot[i].unit + '0';
	  continue;
	}
      }
    } else {
    	if((strncmp(cp+2,"mop",3)==0) ||
	   (strncmp(cp+2,"tftp",4)==0)) { 
      		for(i=0;i<=8;i++) {
			if(((strcmp(tc_slot[i].devname,"ln")==0) ||
	   		   (strcmp(tc_slot[i].devname,"fza")==0)) &&
	   		   (tc_slot[i].slot == *cp - '0')) {
	  			bootctlr[0] = tc_slot[i].unit + '0';
	  			continue;
			}
      		}
    	}
    }	
 
    return(0);
  }
}

	
