#ifndef lint
static char *sccsid = "@(#)trap.c	4.4	ULTRIX	4/11/91";
#endif lint

/************************************************************************
 *									*
 *		Copyright (c) 1985, 1986, 1988, 1989 by			*
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
 * Modification History: /sys/vax/trap.c
 *
 * 11-Apr-91	dlh
 *	syscall():
 *		- on return from a system call, p_affinity was being loaded with
 *		  ALLCPU.  This is wrong if the process is a vector process.
 *
 * 4-Sep-90 dlh
 *	- illegal instruction fault needs to look to see if cpu is vector
 *	  present and if not is the instruction a legal vector instruction
 *	- add vector disabled fault
 *
 * 14-Nov-89 sekhar
 *	Fixes to turn profiling off when scale set to 1.
 *
 * 09-Nov-89 jaw
 *	remove asymmetric syscall support.
 *
 * 04-Oct-89 jaw
 *	release all locks held when "longjmp" out of a syscall occurs.  This
 *	is a tempory fix until code in "soclose" is fixed to handle 
 *	interrupted system calls.
 *
 * 15-Aug-89 scott
 *	check for indirect syscall use for audit and sys_trace codepaths
 *	use callp->sy_narg instead of *(caddr)locr0[AP]
 *
 * 20-Jul-89 jaw
 *	cleanup debug for smp and optimize code in syscall path.
 *
 * 09-Jun-89 -- scott
 *	added audit support
 *
 * 23-Jan-89 -- jmartin
 *	Introduce a new trap type NOACCESS for protection fault on read.
 *
 * 10-Oct-88 -- jaw
 *	check for holding a lock after doing a switch but before restoring
 *	the old lock list.
 *
 * 10-Oct-88 -- jaw
 *	replace switch_to_master with general routine switch_affinity
 *
 * 19 Aug 88 -- miche
 *	Stay out of the idle loop for kernel debugger and pageout
 *	silencing of secondary processors from user mode.  Also,
 *	save the process context for easier debugging.
 *
 * 25 Jul 88 -- jmartin
 *	Allow pagein and grow to happen on any CPU.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 14 May 87 -- fred (Fred Canter)
 *	Turn off graphics console loopback of kernel printf's
 *	when the system panics (via trap).
 *
 * 21 Jan 87 -- jaw
 *	performance fixes to syscall.
 *
 * 02-Apr-86 -- jrs
 *	Clean up for better perfomance in single cpu case.
 *	Allow some syscalls to be mp.
 *
 * 18 Mar 86 -- jrs
 *	Change calls to cpuindex/cpuident
 *
 * 24 Feb 86 -- depp
 *	Added code to SIGBUS to pass virtual address to process
 *
 * 16 Jul 85 -- jrs
 *	Add run queue locking and multicpu sched mods
 *
 * 20 Jan 86 -- pmk
 *	Added binary error logging for traps
 *
 * 14 Oct 85 -- reilly
 *	Modified user.h
 *
 * 29 Oct 84 -- jrs
 *	Fix carry bit clear for compat mode problem
 *	Derived from 4.2BSD, labeled:
 *		trap.c 6.2	84/06/10
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/smp_lock.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "assym.s"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../vax/trap.h"
#include "../h/acct.h"
#include "../h/kernel.h"
#include "../h/errlog.h"
#include "../h/cpudata.h"
#include "../machine/mtpr.h"
#include "../h/systrace.h"
#include "../h/conf.h"
#include "../io/uba/qdioctl.h"	/* for QD_KERN_UNLOOP below */
#include "../h/vmmac.h"
#include "../machine/vmparam.h"
#include "../machine/vectors.h"
#include "../../machine/common/cpuconf.h"

int vm_debug = 1;

#ifdef KDEBUG
#include "../machine/kdb/kdb_trap.h"
#else  !KDEBUG
int in_kdb = 0;  
#endif !KDEBUG

#define	USER	040		/* user-mode flag added to type */

struct	sysent	sysent[];
int	nsysent;
#ifdef SYS_TRACE
int	traceopens = 0;
#endif SYS_TRACE

char	*trap_type[] = {
	"Reserved addressing mode",
	"Privileged instruction",
	"Reserved operand",
	"Breakpoint",
	"Xfc trap",
	"Syscall trap",
	"Arithmetic fault",
	"Ast trap",
	"Segmentation fault",
	"Protection fault",
	"Trace trap",
	"Compatibility mode trap",
#ifdef notdef
	"Page fault",
	"Page table fault",
#endif
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])


extern	int	ws_display_type;
/*
 * Called from the trap handler when a processor trap occurs.
 */
/*ARGSUSED*/
trap(sp, type, code, pc, psl)
	int sp, type;
	unsigned code;
	int pc, psl;
{
#ifdef KDEBUG
	register int *kdb_local_regptr;
#define REG_11 kdb_local_regptr
	extern int *kdb_regs_ptr;
#endif KDEBUG
	register int *locr0 = ((int *)&psl)-PS;
	register int i;
	register struct proc *p;
	register int s;
	struct el_rec *elrp;
	struct timeval syst;
	struct cpudata *pcpu;

	int	new_affinity;	/* new affinity mask of a process becoming a 
				 * vector process or of a process which needs
  				 * to switch it's affinity for some other 
				 * reason.
				 */
#ifdef KDEBUG
	asm("movl fp,r11");
	kdb_regs_ptr = kdb_local_regptr;
#endif KDEBUG

	syst = u.u_ru.ru_stime;
	if (USERMODE(locr0[PS])) {
		type |= USER;
		u.u_ar0 = locr0;
	} 

	switch (type) {

	default:
#ifdef KDEBUG
kdb_goto_label:
#endif KDEBUG
		elrp = ealloc(EL_EXPTFLTSIZE,EL_PRISEVERE);
		if (elrp != 0) {
		    LSUBID(elrp,ELCT_EXPTFLT,(type + 1),EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		    elrp->el_body.elexptflt.exptflt_va = code;
		    elrp->el_body.elexptflt.exptflt_pc = pc;
		    elrp->el_body.elexptflt.exptflt_psl = psl;
		    EVALID(elrp);
		}
		/*
		 * If console is a graphics device,
		 * force cprintf messages directly to screen.
		 */
		if (ws_display_type) {
		    i = ws_display_type << 8;
		    (*cdevsw[ws_display_type].d_ioctl)(i, QD_KERN_UNLOOP, 0, 0);
		}
		cprintf("trap type %d, code = %x, pc = %x\n", type, code, pc);
		type &= ~USER;
		if ((unsigned)type < TRAP_TYPES) {
			panic(trap_type[type]);
			
		}
		panic("trap");
#ifdef KDEBUG
	include_kdb();
#endif KDEBUG

	case T_NOACCESS+USER:
		if (
		    vm_debug &&
		    isatsv(u.u_procp, btop(code)))
			panic("no access to shared text");
		else	/* fall through to protection fault */	;
	case T_PROTFLT+USER:	/* protection fault */
		i = SIGBUS;
		u.u_code = code;
		break;

	case T_PRIVINFLT+USER:	/* privileged instruction fault */

                if (is_legal_vector_instruction(pc) == is_vect_inst) {
                    if (vptotal > 0) {
                            /*
                             * if this process is not already a vector
                             * process, then call vp_allocate to convert this
                             * process to vector process.  This is done by
                             * allocating the space to store the vector
                             * context.
                             */
                   
                            if (u.u_procp->p_vpcontext == NULL) {
			        if (vp_allocate (u.u_procp) == VP_FAILURE) {
				    goto out;
			        }
                            }
                   
                   
                            /*
                             * change the processes affinity
                             */
                   
                            new_affinity = u.u_procp->p_affinity & vpmask;
                            if (new_affinity == 0) {
                                    panic ("trap(): vector affinity problem");
                            } 
                            else {
                                    (void) switch_affinity (new_affinity);
                            }
			    goto out;
                    } else { /* there are no vector processors in the system */
                            u.u_code = ILL_VECINST_FAULT;
                            uprintf ("No vector processor available\n");
			    i = SIGILL;
                    }
                } else { /* this is not a vector instruction */
                    u.u_code = type &~ USER;
		    i = SIGILL;
                }

		break;

	case T_RESADFLT+USER:	/* reserved addressing fault */
	case T_RESOPFLT+USER:	/* resereved operand fault */
		u.u_code = type &~ USER;
		i = SIGILL;
		break;

	case T_ASTFLT+USER:
		astoff();
		if ((u.u_oweupc & SOWEUPC) && (u.u_prof.pr_scale > 1)) {
			addupc(pc, &u.u_prof, 1);
			u.u_oweupc &= ~SOWEUPC;
		}
		goto out;

	case T_ARITHTRAP+USER:
		u.u_code = code;
		i = SIGFPE;
		break;

	/*
	 * If the user SP is above the stack segment,
	 * grow the stack automatically.
	 */
	case T_SEGFLT+USER:
		if (grow((unsigned)locr0[SP]) || grow(code))
			goto out;
		u.u_code = code;
		i = SIGSEGV;
		break;

	case T_TABLEFLT:	/* allow page table faults in kernel mode */
	case T_TABLEFLT+USER:   /* page table fault */
		panic("ptable fault");

	case T_PAGEFLT:		/* allow page faults in kernel mode */
	case T_PAGEFLT+USER:	/* page fault */
		i = u.u_error;
		pagein(code, 0);
		u.u_error = i;
		if (type == T_PAGEFLT)
			return;
		goto out;

	case T_BPTFLT+USER:	/* bpt instruction fault */
	case T_TRCTRAP+USER:	/* trace trap */
		locr0[PS] &= ~PSL_T;
		i = SIGTRAP;
		break;

	case T_XFCFLT+USER:	/* xfc instruction fault */
		i = SIGEMT;
		break;

	case T_COMPATFLT+USER:	/* compatibility mode fault */
		u.u_acflag |= ACOMPAT;
		u.u_code = code;
		i = SIGILL;
		break;

				/* vector processor disabled fault */
	case T_VDISFLT:		/*	kernel mode                */
		vp_disabled_fault_handler (VP_DIS_KERN_MODE);
		goto out;
		break;
				/* vector processor disabled fault */
	case T_VDISFLT+USER:	/*	user mode                  */
		vp_disabled_fault_handler (VP_DIS_USER_MODE);
		goto out;
		break;
	}
	psignal(u.u_procp, i);
out:

	if (USERMODE(locr0[PS])) 
		if (CURRENT_CPUDATA->cpu_hlock) 
			panic("holding lock on trap exit");

	CURRENT_CPUDATA->cpu_trap++;
	p = u.u_procp;
	if (p->p_cursig || ISSIG(p,0))
		psig();
	/*
	 * take advantage of the fact that longword long-word
	 * aligned writes are atomic here: p_pri could change
	 * as soon as we released the lock anyway.
	 */
	p->p_pri = p->p_usrpri;

	if (CURRENT_CPUDATA->cpu_runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		pcpu = CURRENT_CPUDATA;
		pcpu->cpu_proc->p_hlock = pcpu->cpu_hlock;
		pcpu->cpu_hlock=0;

		(void)spl6();
		smp_lock(&lk_rq,LK_RETRY);
		p->p_pri = p->p_usrpri;	
		setrq(u.u_procp);
		u.u_ru.ru_nivcsw++;
		swtch();
		pcpu = CURRENT_CPUDATA;
		pcpu->cpu_hlock = pcpu->cpu_proc->p_hlock;
		pcpu->cpu_proc->p_hlock=0;
	}
	if (u.u_prof.pr_scale > 1) {
		int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(locr0[PC], &u.u_prof, ticks);
	}

}

/*
 * Called from the trap handler when a system call occurs
 */
/*ARGSUSED*/
syscall(sp, type, code, pc, psl)
	unsigned code;
{
	register int *locr0 = ((int *)&psl)-PS;
	register caddr_t params;		/* known to be r10 below */
	register int i;				/* known to be r9 below */
	register struct sysent *callp;
	register struct proc *p;
	register struct nameidata *ndp = &u.u_nd;
	struct cpudata *pcpu;
	unsigned int saveaffinity;
	struct timeval syst;
	unsigned ocode = 0;

	syst = u.u_ru.ru_stime;
	if (!USERMODE(locr0[PS]))
		panic("syscall");
	u.u_ar0 = locr0;
	if (code == 139) {	/* getdprop */	/* XXX */
		sigcleanup();			/* XXX */
		goto done;			/* XXX */
	}
	params = (caddr_t)locr0[AP] + NBPW;
	u.u_error = 0;

	callp = (code >= nsysent) ? &sysent[63] : &sysent[code];
	if (callp == sysent) {
		ocode = fuword(params);
		params += NBPW;
		callp = (ocode >= nsysent) ? &sysent[63] : &sysent[ocode];
	}
	if (i = callp->sy_narg * sizeof (int)) {
#ifndef lint
		asm("prober $3,r9,(r10)");		/* GROT */
		asm("bnequ ok");			/* GROT */
		u.u_error = EFAULT;			/* GROT */
		goto bad;				/* GROT */
ok:
asm("ok:");						/* GROT */
		asm("movc3 r9,(r10),_u+U_ARG");		/* GROT */
#else
		bcopy(params, (caddr_t)u.u_arg, (u_int)i);
#endif
	}
	u.u_ap = u.u_arg;
	ndp->ni_dirp = (caddr_t)u.u_arg[0];
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = locr0[R1];
	if (setjmp(&u.u_qsave)) {
		if (CURRENT_CPUDATA->cpu_hlock) {
			while(CURRENT_CPUDATA->cpu_hlock) {
				mprintf("lock held on syscall exit %x pc %x\n",
					CURRENT_CPUDATA->cpu_hlock,
					CURRENT_CPUDATA->cpu_hlock->l_pc);
				smp_unlock(CURRENT_CPUDATA->cpu_hlock);
			}
		}
		if (u.u_error == 0 && u.u_eosys == JUSTRETURN)
			u.u_error = EINTR;
	} else {
		u.u_eosys = JUSTRETURN;
		if ( audswitch 
#ifdef SYS_TRACE
			+traceopens
#endif SYS_TRACE
				) {
			u.u_gno_indx = 0;
			u.u_narg = callp->sy_narg;
			u.u_event = ocode == 0 ? code : ocode;
#ifdef SYS_TRACE
	 		/* trace it just before we do it! (only if open) */
	 		if (traceopens) {
				syscall_trace(u.u_event,callp->sy_narg,BEFORE);
				(*(callp->sy_call))();
				syscall_trace(u.u_event,callp->sy_narg,AFTER);
			} else 
#endif SYS_TRACE
				(*(callp->sy_call))();
			if ( aud_param[u.u_event][AUD_NPARAM-1] != 'X' )
			    AUDIT_CALL ( u.u_event, u.u_error, u.u_r.r_val1, AUD_GNO|AUD_HDR|AUD_PRM|AUD_RES, (int *)0, 0 );

		} else {
			(*(callp->sy_call))();

		}

	}
	if (u.u_eosys == RESTARTSYS) {
		pc = pc - 2;
		if (code > 63)
			pc -= 2;
	}
	else if (u.u_error) {
#ifndef lint
bad:
#endif
		locr0[R0] = u.u_error;
		locr0[PS] |= PSL_C;	/* carry bit */
	} else {
		locr0[R0] = u.u_r.r_val1;
		locr0[R1] = u.u_r.r_val2;
		locr0[PS] &= ~PSL_C;
	}
done:
	p = u.u_procp;
	if (p->p_cursig || ISSIG(p,0))
		psig();

	pcpu = CURRENT_CPUDATA;
	pcpu->cpu_syscall++;
	if (pcpu->cpu_hlock) 
		panic("holding lock on syscall exit");

	p->p_pri = p->p_usrpri;

	if (smp) {
		/* might need to fix-up affinity mask if we did a longjmp out 
		 * of an ASMP section of code
		 */
		p = u.u_procp;
		pcpu = CURRENT_CPUDATA;
		if (p->p_vpcontext) {
			/* this process is a vector process */
			if (p->p_vpcontext->vpc_state == VPC_SAVED) {
				/* this process's vector context has already 
				 * been saved, so it can run on any vector 
				 * processor.  set affinity to the pre-vector 
				 * affinity and'ed with the mask of processors 
				 * with vector units
				 */
				switch_affinity
				    (p->p_vpcontext->vpc_affinity & vpmask);
				pcpu = CURRENT_CPUDATA;
			} else {
				/* our vp context is loaded on this cpu 
				 * (vpc_state == VPC_LOAD or 
				 *  vpc_state == * VPC_LIMBO),
				 * so force the affinity to be this cpu.
				 * debug notes:
				 * - since this should always be true anyway, 
				 *   maybe we should panic if 
				 *   p_affinity != cpu_mask ???
				 * - vpc_state should never be
				 *   VPC_WAIT.  A state of wait means that
				 *   the execution of a vector instruction
				 *   has cause the allocation of a
				 *   vpcontext area.  The state remains
				 *   wait only as long a there is a search
				 *   being made for an available vector
				 *   processor.  Since no other user
				 *   instruction will be executed while the
				 *   vector instruction is pending, a
				 *   system call could not be made.
				 */
				p->p_affinity = pcpu->cpu_mask;
			}
		} else {
			/* This is not a vector process, so it can run 
			 * anywhere
			 */
			p->p_affinity = ALLCPU;
		}
	}

	if (pcpu->cpu_runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just
		 * change our priority without moving us from one
		 * queue to another (since the running process is
		 * not on * a queue.)  If that happened after we
		 * setrq ourselves but before we swtch'ed, we might
		 * not be on the queue indicated by our priority.
		 */
		(void) spl6();
		smp_lock(&lk_rq,LK_RETRY);
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale > 1) {
		int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(locr0[PC], &u.u_prof, ticks);
	}
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
nosys()
{
	if (u.u_signal[SIGSYS] == SIG_IGN || u.u_signal[SIGSYS] == SIG_HOLD)
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}
