#ifndef lint
static char *sccsid = "@(#)sys_process.c	4.4	ULTRIX	2/12/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * Modification History: /sys/sys/sys_process.c
 *
 * 11-Feb-91	jaw
 *	fix for error being returned on a step or continue command when
 *	the process exits.  Also fix a kmalloc memory leak.
 *
 * 10-Oct-90	dlh
 *	correct test for vector process context state while debugging a 
 *	vector process
 *
 * 4-Sep-90	dlh
 *	added vector processor support code
 *
 * 06-Sep-89 -- gg
 *	Removed returning old values in case of requests 4,5 and 6. This 
 *	is done to conform to SVID. BSD and ultrix document these return
 *	values as "not useful"; the SVID documents the result to be the input
 *	values. Hence the change.
 *
 * 30-May-89 -- darrell
 *	Replaced the include of cpu.h with an include of cpuconf.h
 *
 * 24-Apr-89 -- jaw 
 *	fix race condition in process tracing between "ptrace" and child
 *	exiting.
 *
 * 09 Dec 87 -- jaw
 *	fix security hole.  A user can use ADB to modifiy the psl of the
 *      program being debugged and crash the system.  I disallow the user
 *	from setting the PSL_CM bit if compatibility mode is not implemented
 * 	on that type of VAX.  On VAX's that implement compatibility mode,
 *	I allow the PSL_CM bit to be set, but clear out the IV,FU,DV and
 *	FPD bits.
 *
 * 22 Nov 88 -- jaw 
 *	fix case where we were sending signal to our parent before we
 *	were REALLY stopped....  more magic.
 *
 * 22 Sep 88 -- jaw 
 *	Remove primary bop 
 *
 * 31 Aug 88 -- jmartin
 *	Use X_LOCK macro to gain exclusive, debugging access to text.
 *
 * 01-Jul-88 -- miche
 *	Make mpsafe, and allocate ipc structure on demand on a
 *	per process basis.
 *
 * 10-Jun-88 -- jaw 
 * 	add parameter to ISSIG for SMP.... this makes going to stop
 *	state atomic.
 *
 * 11 Sep 86 -- koehler
 *	gnode name change
 *
 * 25 Oct 84 -- jrs
 *	Add security fix to prohibit ptrace and
 *	binaries the user doesn't own
 *	Derived from 4.2BSD, labeled:
 *		sys_process.c 6.1	83/07/29
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/text.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/acct.h"
#include "../h/kmalloc.h"
#include "../h/ptrace.h"
#ifdef mips
#include "../machine/cachectl.h"
#endif mips

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#ifdef vax
#include "../machine/vectors.h"
#include "../machine/mtpr.h"
#endif vax
#include "../h/cpudata.h"

#ifdef mips
/*
 * This must be declared as an int array to fake the assembler
 * into not making this gp relative.
 */
extern int sstepbp[];
#endif mips

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct lock_t lk_debug;

struct debug {
	int		ip_req;
	int		*ip_addr;
	int		ip_data;
};

/*
 * sys-trace system call.
 */
ptrace()
{
	register struct proc *p;
	register struct a {
		int	req;
		int	pid;
		int	*addr;
		int	data;
	} *uap;
	register struct debug *ipc;

	uap = (struct a *)u.u_ap;
	if (uap->req <= 0) {
		/*
		 * This is the child saying they want to be traced
		 * First allocate a debug structure.
		 */
		if (u.u_procp->p_debug)
			ipc = u.u_procp->p_debug;
		else KM_ALLOC(ipc, struct debug *, sizeof (struct debug),
			KM_DEBUG, KM_CLEAR);

		smp_lock(&lk_debug,LK_RETRY);
		u.u_procp->p_debug = (char *)ipc;
		u.u_procp->p_trace |= STRC;
		smp_unlock(&lk_debug);
		return;
	}

	/*
	 * Note that in the following we ensure that the process returned
	 * is both the one we want an din the stop state.  Since the only
	 * way out of a stop state is with a signal, and signals are always
	 * passed through the parent for a traced process, we are guaranteed
	 * that this process cannot go away without our knowing about it.
	 */
	p = pfind(uap->pid);

	smp_lock(&lk_debug, LK_RETRY);
	if (p == 0 || p->p_stat != SSTOP || p->p_ppid != u.u_procp->p_pid ||
	    !(p->p_trace & STRC) || p->p_debug==0 ) {
	    	smp_unlock(&lk_debug);
		u.u_error = ESRCH;
		return;
	}

	ipc = (struct debug *)p->p_debug;
	ipc->ip_data = uap->data;
	ipc->ip_addr = uap->addr;
	ipc->ip_req = uap->req;
	p->p_trace &= ~SWTED;

	while (ipc->ip_req > 0) {
		/*
		 * start child running, and wait for request
		 * value to be cleared:  child must wake us.
		 * We shouldn't ever be in this loop twice...
		 */
		if (p->p_stat == SSTOP) {
			(void)spl6();
			smp_lock(&lk_rq, LK_RETRY);
			if (p->p_stat == SSTOP) setrun(p);
			smp_unlock(&lk_rq);
			(void)spl0();
		}
		sleep_unlock(ipc, IPCPRI, &lk_debug);

		/*
		 * re-aquire lock.  If child exited then "p_debug" will be null
  		 * or another processes picked up the slot and is also being
		 * debugged.  If this is the case then return data passed in
 		 * and return EIO if the request was not for the child
		 * to exit.
		 *
		 */
		smp_lock(&lk_debug,LK_RETRY);
		if ( (p->p_ppid != u.u_procp->p_pid) || (p->p_debug == NULL)) {
			smp_unlock(&lk_debug);
			if ((uap->req != PT_KILL) && (uap->req != PT_CONTINUE)
				&& (uap->req != PT_STEP)) {
				/* not an exit */
				u.u_error = EIO;
			}
			u.u_r.r_val1 = uap->data;
			return;
		}
	}

	u.u_r.r_val1 = ipc->ip_data;
	if (ipc->ip_req < 0)
		u.u_error = EIO;
	smp_unlock(&lk_debug);
}

#ifdef vax
#define	NIPCREG 16
int ipcreg[NIPCREG] =
	{R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,AP,FP,SP,PC};

#define	PHYSOFF(p, o) \
	((physadr)(p)+((o)/sizeof(((physadr)0)->r[0])))
#endif

#ifdef mips
#define	ADDR(x)	((char *)&(x))
#define	PCB(x)	(u.u_pcb.x)

struct regmap {
	char *rm_addr;
	unsigned rm_width;
	unsigned rm_wrprot;
} regmap[NSPEC_REGS] = {
	{ ADDR(USER_REG(EF_EPC)),	sizeof(USER_REG(EF_EPC)),	0 },
	{ ADDR(USER_REG(EF_CAUSE)),	sizeof(USER_REG(EF_CAUSE)),	1 },
	{ ADDR(USER_REG(EF_MDHI)),	sizeof(USER_REG(EF_MDHI)),	0 },
	{ ADDR(USER_REG(EF_MDLO)),	sizeof(USER_REG(EF_MDLO)),	0 },
	{ ADDR(PCB(pcb_fpc_csr)),	sizeof(PCB(pcb_fpc_csr)),	0 },
	{ ADDR(PCB(pcb_fpc_eir)),	sizeof(PCB(pcb_fpc_eir)),	1 },
	{ ADDR(u.u_trapcause),		sizeof(u.u_trapcause),		1 },
	{ ADDR(u.u_trapinfo),		sizeof(u.u_trapinfo),		1 }
};

#define	ALIGNED(addr,size)	(((unsigned)(addr)&((size)-1))==0)

#endif mips
/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
#ifdef vax
	register *p;
#endif vax
	register struct text *xp;
	register int i;
#ifdef mips
	register char *p;
	register unsigned regno;
	int width;
#endif mips
	register struct debug *ipc;
	int	vreg_off;

	/* don't need to lock because the processes running is 
	   the only one show can free the debug structure. */
	ipc = (struct debug *)u.u_procp->p_debug;
	if (!ipc) {
		/* have set up no debugging facilties */
		uprintf("debugging witout setup\n");
		psignal(u.u_procp, SIGKILL);
		return(1);
	}

	u.u_procp->p_slptime = 0;
#ifdef mips
	if (u.u_pcb.pcb_ssi.ssi_cnt)
		remove_bp();
#endif mips
#ifdef vax
	if ((u.u_procp->p_vpcontext) && 
	    ((u.u_procp->p_vpcontext->vpc_state == VPC_LOAD) || 
	     (u.u_procp->p_vpcontext->vpc_state == VPC_LIMBO))) {
		/*
		 * user proc pointer says this is a vector process, so 
		 * save the vector context
		 */
		CURRENT_CPUDATA->cpu_vpdata->vpd_in_kernel = VPD_IN_KERNEL;
		if (vp_contextsave (u.u_procp) == VP_FAILURE) {
			/*
			* the current implementation of vector support
			* panics if vp_contextsave() has any problem.
			* This signal is a place holder in case
			* vp_contextsave() gets smarter
			 */
			psignal(u.u_procp, SIGKILL);
			return(1);
		}
		/*
		 * disable the vector processor.
		 * vp_contextsave() leaves the vector processor enabled.  
		 * When a PT_WRITE_V is issued, it is not written 
		 * immediately to the vector processor.  It is written to 
		 * the struct vpcontext.  If the vector processor is 
		 * disabled when the next vector instruction is executed, 
		 * then the new value will be written to the vector 
		 * processor before the instruction is executed.
		 */
		mtpr (VPSR, 0);
		mfpr(VPSR);
		CURRENT_CPUDATA->cpu_vpdata->vpd_in_kernel = ~ VPD_IN_KERNEL;
		u.u_procp->p_vpcontext->vpc_state = VPC_SAVED;
		CURRENT_CPUDATA->cpu_vpdata->vpd_proc = NULL;
	}
#endif vax

	switch (ipc->ip_req) {

	/* read user I */
	case PT_READ_I:
#ifdef mips
		if (!ALIGNED(ipc->ip_addr, sizeof(int)))
			goto error;
#endif mips
		if (!useracc((caddr_t)ipc->ip_addr, 4, B_READ))
			goto error;
		ipc->ip_data = fuiword((caddr_t)ipc->ip_addr);
		break;

	/* read user D */
	case PT_READ_D:
#ifdef mips
		if (!ALIGNED(ipc->ip_addr, sizeof(int)))
			goto error;
#endif mips
		if (!useracc((caddr_t)ipc->ip_addr, 4, B_READ))
			goto error;
		ipc->ip_data = fuword((caddr_t)ipc->ip_addr);
		break;

	/* read u */
	case PT_READ_U:
		i = (int)ipc->ip_addr;
#ifdef vax
		if (i<0 || i >= ctob(UPAGES))
			goto error;
		ipc->ip_data = *(int *)PHYSOFF(&u, i);
#endif vax
#ifdef mips
		if ((unsigned)(regno = i - GPR_BASE) < NGP_REGS)
			ipc->ip_data = (regno==0) ? 0 : USER_REG(EF_AT+regno-1);
		else if ((unsigned)(regno = i - FPR_BASE) < NFP_REGS) {
			checkfp(u.u_procp, 0);
			ipc->ip_data = u.u_pcb.pcb_fpregs[regno];
		} else if ((unsigned)(regno = i - SIG_BASE) < NSIG_HNDLRS)
			ipc->ip_data = (int) u.u_signal[regno];
		else if ((unsigned)(regno = i - SPEC_BASE) < NSPEC_REGS) {
			if(i == FPC_CSR || i == FPC_EIR)
				checkfp(u.u_procp, 0);
			switch (regmap[regno].rm_width) {
			case sizeof(char):
				ipc->ip_data = *(char *)(regmap[regno].rm_addr);
				break;
			case sizeof(short):
				ipc->ip_data = *(short *)(regmap[regno].rm_addr);
				break;
			case sizeof(int):
				ipc->ip_data = *(int *)(regmap[regno].rm_addr);
				break;
			default:
				panic("ptrace regmap botch");
			}
		} else
			goto error;
#endif mips
		break;

	/* write user I */
	/* Must set up to allow writing */
	case PT_WRITE_I:
#ifdef mips
		if (!ALIGNED(ipc->ip_addr, sizeof(int)))
				goto error;
#endif mips
		/*
		 * If text, must assure exclusive use
		 */
		xp = u.u_procp->p_textp;
		if (xp) {
			X_LOCK(xp);
			/* don't allow unless this is his */
			if ((u.u_uid && xp->x_gptr->g_uid != u.u_uid) ||
			    (xp->x_count!=1 || xp->x_gptr->g_mode&GSVTX)) {
				X_UNLOCK(xp);
				goto error;
			}
#ifdef notdef
/* We need to revisit this -- depp */
			xp->x_gptr->g_flag &= ~GTEXT;
#else  notdef
			xp->x_flag |= XTRC;
#endif notdef
		}
#ifdef mips
		if (!write_utext(ipc->ip_addr, ipc->ip_data))
			if (xp)
				xp->x_flag |= XWRIT;
		if(xp) X_UNLOCK(xp);
#endif mips
#ifdef vax
		i = -1;
		/* Clearly a punt:  may get PTE messups if this
		 * isn't protected.
		 */
		if ((i = suiword((caddr_t)ipc->ip_addr, ipc->ip_data)) < 0) {
			if (chgprot((caddr_t)ipc->ip_addr, RW) &&
			    chgprot((caddr_t)ipc->ip_addr+(sizeof(int)-1), RW))
				i = suiword((caddr_t)ipc->ip_addr, ipc->ip_data);
			(void) chgprot((caddr_t)ipc->ip_addr, RO);
			(void) chgprot((caddr_t)ipc->ip_addr+(sizeof(int)-1), RO);
		}
		if (xp) {
			if (i >= 0)
				xp->x_flag |= XWRIT;
			X_UNLOCK(xp);
		}
		if (i < 0)
			goto error;
#endif vax
		break;

	/* write user D */
	case PT_WRITE_D:
#ifdef mips
		if (!ALIGNED(ipc->ip_addr, sizeof(int)))
			goto error;
#endif mips
		if (suword((caddr_t)ipc->ip_addr, 0) < 0)
			goto error;
		(void) suword((caddr_t)ipc->ip_addr, ipc->ip_data);
#ifdef mips
		user_flush(ipc->ip_addr, sizeof(int), ICACHE);
#endif mips
		break;

	/* write u */
	case PT_WRITE_U:
		i = (int)ipc->ip_addr;
#ifdef vax
		p = (int *)PHYSOFF(&u, i);
		for (i=0; i<NIPCREG; i++)
			if (p == &u.u_ar0[ipcreg[i]])
				goto ok;
		if (p == &u.u_ar0[PS]) {
			/* force user mode */
			ipc->ip_data |= PSL_USERSET;
			/* clear MBZ bits in PSL */
			ipc->ip_data &=  ~PSL_USERCLR;
			if (ipc->ip_data & PSL_CM) {
				/* compatibility VAX's can have a PSL with
				  CM set, BUT  other PSL bit can not be set
				  or we will die on REI out of kernel */
				if ( cpu== VAX_730 || cpu == VAX_750 ||
				     cpu == VAX_780 || cpu == VAX_8600) {
				     ipc->ip_data &= ~(PSL_FPD|PSL_DV|PSL_FU|PSL_IV);
				} else {
					/* clear compatibilty mode if not 
					   implemented */
					ipc->ip_data &= ~PSL_CM;
				}
			}
			goto ok;
		}
		goto error;
ok:
		*p = ipc->ip_data;
#endif vax
#ifdef mips
		width = sizeof(int);
		if ((unsigned)(regno = i - GPR_BASE) < NGP_REGS) {
			if (regno == 0)
				goto error;
			p = (char *)&USER_REG(EF_AT+regno-1);
		} else if ((unsigned)(regno = i - FPR_BASE) < NFP_REGS) {
			checkfp(u.u_procp, 0);
			p = (char *)&u.u_pcb.pcb_fpregs[regno];
		} else if ((unsigned)(regno = i - SIG_BASE) < NSIG_HNDLRS)
			p = (char *)&u.u_signal[regno];
		else if ((unsigned)(regno = i - SPEC_BASE) < NSPEC_REGS) {
			if(i == FPC_CSR || i == FPC_EIR)
				checkfp(u.u_procp, 0);
			if (regmap[regno].rm_wrprot)
				goto error;
			/*
			 * complain if pc unaligned
			 */
			if (i == PC && (ipc->ip_data & sizeof(int)-1))
				goto error;
			p = (char *)(regmap[regno].rm_addr);
			width = regmap[regno].rm_width;
		} else
			goto error;
		switch (width) {
		case sizeof(char):
			/*
			 * User may have to and off sign extend
			 */
			*(char *)p = ipc->ip_data;
			break;
		case sizeof(short):
			*(short *)p = ipc->ip_data;
			break;
		case sizeof(int):
			*(int *)p = ipc->ip_data;
			break;
		default:
			panic("ptrace regmap botch");
		}
#endif mips
		break;

	/* set signal and continue */
	/* one version causes a trace-trap */
	case PT_CONTINUE:
	case PT_STEP:
		if ((int)ipc->ip_addr != 1) {
#ifdef vax
			u.u_ar0[PC] = (int)ipc->ip_addr;
#endif vax
#ifdef mips
			/*
			 * complain if pc unaligned
			 */
			if (!ALIGNED(ipc->ip_addr, sizeof(int)))
				goto error;
			u.u_ar0[EF_EPC] = (int)ipc->ip_addr;
#endif mips
		}

		if ((unsigned)ipc->ip_data > NSIG)
			goto error;
		u.u_procp->p_cursig = ipc->ip_data;	/* see issig */
#ifdef vax
		if (ipc->ip_req == PT_STEP) 
			u.u_ar0[PS] |= PSL_T;
#endif vax
#ifdef mips
		u.u_pcb.pcb_sstep = (ipc->ip_req == PT_STEP);
#endif mips
		smp_lock(&lk_debug,LK_RETRY);
		ipc->ip_req=0;
		smp_unlock(&lk_debug);
		wakeup((caddr_t)ipc);
		return (1);

	/* force exit */
	case PT_KILL:
		smp_lock(&lk_debug,LK_RETRY);
		ipc->ip_req=0;
		smp_unlock(&lk_debug);
		wakeup((caddr_t)ipc);
		exit(u.u_procp->p_cursig);
		/*NOTREACHED*/

#ifdef vax
	/* read a value from the vpcontext or a vector register */
	case PT_READ_V:
		if (u.u_acflag & AVP) {
			if (u.u_procp->p_vpcontext->vpc_state != VPC_SAVED) {
				uprintf ("bad vpcontext state\n");
				psignal(u.u_procp, SIGKILL);
				return(1);
			} 
			i = (int)ipc->ip_addr;
			vreg_off = sizeof (struct vpcontext) - sizeof (char *);
			if (i < vreg_off) {
				ipc->ip_data = * (int *)
				PHYSOFF(u.u_procp->p_vpcontext, i);
			} else {
				i -= vreg_off;
				ipc->ip_data = * (int *)
				  PHYSOFF(u.u_procp->p_vpcontext->vpc_vregs, i);
			}
		} else {
		    psignal(u.u_procp, SIGSYS);
		    u.u_r.r_val1 = -1;
		    goto error;
		}
		break;

	/* write a value to the vpcontext or a vector register */
	/* for the time being, will allow write of any entry in vpcontext,
	 * later should add bells and whistles to limit which entries can 
	 * be written.
	 */
	case PT_WRITE_V:
		if (u.u_acflag & AVP) {
			if (u.u_procp->p_vpcontext->vpc_state != VPC_SAVED) {
				uprintf ("bad vpcontext state\n");
				psignal(u.u_procp, SIGKILL);
				return(1);
			} 
			i = (int)ipc->ip_addr;
			vreg_off = sizeof (struct vpcontext) - sizeof (char *);
			if (i < vreg_off) {
				p = (int *) PHYSOFF(u.u_procp->p_vpcontext, i);
			} else {
				i -= vreg_off;
				p = (int *)
				  PHYSOFF(u.u_procp->p_vpcontext->vpc_vregs, i);
			}
			*p = ipc->ip_data;
		} else {
			psignal(u.u_procp, SIGSYS);
			u.u_r.r_val1 = -1;
			goto error;
		}
		break;
#endif vax

	default:
	error:
		ipc->ip_req = -1;
	}

	smp_lock(&lk_debug,LK_RETRY);
	/* if we didn't get an error then clear the 
	   ip_req field */
	if (ipc->ip_req != -1) 
		ipc->ip_req=0;
	smp_unlock(&lk_debug);
	return (0);
}
#ifdef mips
write_utext(addr, data)
caddr_t addr;
unsigned data;
{
	register struct text *xp;
	int i;
	struct gnode *gp;

	/*
	 * Must assure exclusive use
	 */
	if (xp = u.u_procp->p_textp) {
		gp = xp->x_gptr;
		if (xp->x_count != 1 || (gp->g_mode & GSVTX))
			return (0);
		gp->g_flag &= ~GTEXT;
		xp->x_flag |= XTRC;
	}
	i = -1;
	if ((i = suiword(addr, data)) < 0) {
		if (chgprot(addr, RW) &&
		    chgprot(addr+(sizeof(int)-1), RW))
			i = suiword(addr, data);
		(void) chgprot(addr, RO);
		(void) chgprot(addr+(sizeof(int)-1),RO);
	}
	if (i < 0)
		return (0);
	if (xp)
		xp->x_flag |= XWRIT;
	user_flush(addr, sizeof(int), ICACHE);
	return (1);
}
#endif mips
