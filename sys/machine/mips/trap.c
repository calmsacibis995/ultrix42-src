#ifndef lint
static char *sccsid = "@(#)trap.c	4.9    ULTRIX  3/6/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *	Modification History: trap.c
 *
 * 06-Mar-91 -- jaw
 *	optimize 3min spl changes.
 *
 * 20-Feb-91 -- jaw
 *	allow for secondary cpus to run softnet code.
 *
 * 19-Dec-90 -- jaw
 *  	fix up affinity on syscall exit...  takes care of any longjmps
 *	that might not have restored the affinity...
 *
 * 10-Aug-90 -- sekhar
 *	cleaned up softnet interrupt handler by removing duplication 
 *	of code. also changed softnet to call log_errinfo on a write
 *	timeout.
 *
 * 05-Jul-90 -- sekhar
 *	modified softnet interrupt handler for write buffer timeouts 
 *	on memory mapped accessed devices.
 *
 * 30-Apr-90 	Randall Brown
 *	Renamed intr() to kn01_intr() and whatspl() to kn01_whatspl().  This
 *	allows other systems to have different routines to handle the same
 *	thing.  These routines are now called through the cpu switch.
 *
 * 16-Apr-90 -- jaw
 *	performance fixes for single cpu.
 *
 * 29-Mar-90 -- gmm/jaw
 *	Call psignal() in softnet() if the process needs a signal. Result
 *	of changing splhigh() and spl6() to be same as splclock().
 *
 * 14-Feb-90 -- gmm
 *	replace pcpu with CURRENT_CPUDATA in trap() where the process could
 *	context switched and come back on a different processor.
 *
 * 30-Dec-89 -- bp
 *	Replaced useage of atintr_level with CPU specific in an interrupt
 *	service routine evaluation.
 *
 * 14-Nov-89 -- sekhar
 * 	Fixes to turn profiling off when scale set to 1.
 *
 * 13-Nov-89 -- afd
 *	Don't panic if we return from cpu specific bus error handler
 *
 * 19-Oct-89 -- jmartin
 *	Add TLBMISS_STUCK code to panic if retrying a page fault endlessly.
 *
 * 13-Oct-89 gmm
 *	smp changes. Access nofault, nofault_cause etc through cpudata.
 *	Changes for per processor tlbpids. Handle affinity correctly for
 *	system calls.
 *
 * 04-Oct-89 jaw
 *	release all locks held when "longjmp" out of a syscall occurs.  This
 *	is a tempory fix until code in "soclose" is fixed to handle 
 *	interrupted system calls.
 *
 * 09-Jun-89 -- scott
 *	added audit support
 *
 * 09-Jun-89 -- gmm
 *	Update cpudata structure for number of interrupts.
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 * 07-Apr-89 -- afd
 *	Moved all pmax specific code out to kn01.c.
 *	System specific routines now called thru the cpu switch.
 *	Cleaned out some old unused code.
 *
 * 10-Feb-89 -- Randall Brown
 *	Added the global variable 'atintr_level' that get incremented on
 *	every entrance to intr().  This is used by printf() during config
 *	to determine if we are at interrupt level.
 *
 * 19-Jan-89 -- Kong
 *	Made routines more generic so that they work for multiple
 *	machines.
 *
 * 29-Dec-88 -- Randall Brown
 *	In the 'pmaxconsprint' routine, changed the state of the variable 
 *	printstate so that printf (and cprintf) will use the prom
 *	putchar. (only if console is graphic device)
 *
 * 15-Dec-1988  afd
 *	Clear MEMERR bit in memintr() routine.
 *	Also fix address calculation in trap() & memintr();
 *	  when we compare a physical address to physmem the physical
 *	  address must be converted to pages (btop(pa)).
 *
 * 18-Nov-1988  afd (for rr)
 *	Added system call trace hooks.
 *
 * 17-Nov-1988  depp
 *	Added addition memory page protection to tlbmiss(), and tlbmod().
 *
 * 09-Nov-1988  jaa
 *	allow process to turn on/off unaligned access messages
 *
 * 09-Aug-1988  afd
 *	Added error logging support for PMAX:
 *	  - Added error log routines: pmaxlogesrpkt(), pmaxlogmempkt(),
 *	    pmaxconsprint(), chk_cpe().
 *	  - Added fault isolation for memory parity error (don't call buserror)
 *	  - Call the error log routines from trap() and memintr().
 *	  - Changed fixade() routine to give back failure status.
 *	  - Added whatspl() routine to give the current IPL level.
 *
 *************************************************************************/

#define CNT_TLBMISS_HACK 0

#include "../machine/cpu.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/syslog.h"
#include "../h/ptrace.h"
#include "../h/cpudata.h"
#include "../h/systrace.h"
#include "../h/errlog.h"
#include "../h/cmap.h"

#include "../net/net/netisr.h"

#include "../../machine/common/cpuconf.h"
#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/inst.h"
#include "../machine/fpu.h"

/*
 * Exception handling dispatch table. 
 */
extern VEC_syscall(), VEC_cpfault(), VEC_trap(), VEC_int(), VEC_tlbmod();
extern VEC_tlbmiss(), VEC_breakpoint(), VEC_addrerr(), VEC_ibe(), VEC_dbe();
extern VEC_unexp();
int  (*causevec[16])() = {
	/*  0: EXC_INT */		VEC_int,
	/*  1: EXC_MOD */		VEC_tlbmod,
	/*  2: EXC_RMISS */		VEC_tlbmiss,
	/*  3: EXC_WMISS */		VEC_tlbmiss,
	/*  4: EXC_RADE */		VEC_addrerr,
	/*  5: EXC_WADE */		VEC_addrerr,
	/*  6: EXC_IBE */		VEC_ibe,
	/*  7: EXC_DBE */		VEC_dbe,
	/*  8: EXC_SYSCALL */	 	VEC_syscall,
	/*  9: EXC_BREAK */		VEC_breakpoint,
	/* 10: EXC_II */		VEC_trap,
	/* 11: EXC_CPU */		VEC_cpfault,
	/* 12: EXC_OV */		VEC_trap,
	/* 13: undefined */		VEC_unexp,
	/* 14: undefined */		VEC_unexp,
	/* 15: undefined */		VEC_unexp
};

/*
 * Interrupt dispatch table
 */

extern softclock(), softnet(), biointr(), netintr(), ttyintr(); 
extern hardclock(), fpuintr(), memintr();

extern struct cpusw *cpup;	 /* Pointer to cpusw entry for this machine*/

extern int mmap_debug;

int  (*c0vec_tbl[8])();

int	c0vec_tblsize = {sizeof(c0vec_tbl)};	/* Size in bytes */

biointr(){
	/* this should be enhanced to determine which SII interrupted and
	   dispatch accordingly. */
	sii_intr(0);
}

netintr(){
	/* this should be enhanced to determine which LANCE interrupted and
	   dispatch accordingly. */
	lnintr(0);
}

/*
 * nofault dispatch table
 */
extern baerror(), cerror(), adderr(), uerror(), uaerror(), softfp_adderr();
extern reviderror(), cstrerror(), softfp_insterr(), fixade_error();
extern rdnf_error();
int  (*nofault_pc[NF_NENTRIES])() = {
	/* unused */		0,
	/* NF_BADADDR */	baerror,
	/* NF_COPYIO */		cerror,
	/* NF_ADDUPC */		adderr,
	/* NF_FSUMEM */		uerror,
	/* NF_USERACC */	uaerror,
	/* NF_SOFTFP */		softfp_adderr,
	/* NF_REVID */		reviderror,
	/* NF_COPYSTR */	cstrerror,
	/* NF_SOFTFPI */	softfp_insterr,
	/* NF_FIXADE */		fixade_error,
	/* NF_INTR */		rdnf_error
};

/*
 * Used for decoding break instructions.  There is an old standing bug in the
 * assembler which encoded the break code right justified to bit 16 not to
 * bit 6 (that's why the BRK_SHIFT is 16 not 6 as would be obvious).
 */
#define BRK_MASK	0xfc00003f
#define BRK_SHIFT	16
#define BRK_SUBCODE(x)	(((x) & ~BRK_MASK) >> BRK_SHIFT)

/*
 * This must be declared as an int array to keep the assembler from
 * making it gp relative.
 */
extern int sstepbp[];

extern struct	sysent	sysent[];
int	nsysent;

/* 
 *	The following two pointer are used in the exception handler as
 *	a pointer to the data that needs to be saved on the exception frame
 *	in the system specific entries.  If a particular processor does not
 *	need to use these they must be set to values that can be read
 *	and written.
 */

/*
 * rpbfix: put in comment
 */
u_int sr_usermask = (SR_IMASK0|SR_IEP|SR_KUP);	/* disable IEc, enable IEp, prev umode */

/*
 * TODO: Get parameters in agreement with locore.s
 */

#define USERFAULT 1

extern int runrun;
extern int k_puac;


/*
 * Called from locore.s (VEC_trap) to handle exception conditions
 */
trap(ep, code, sr, cause)
	register u_int *ep;		/* exception frame ptr */
	register u_int code;		/* trap code (trap type) */
	u_int sr, cause;		/* status and cause regs */
{
	register struct proc *p;
	register int i, vaddr;
	struct timeval syst;
	struct pte *pte;
	int nofault_save;
	u_int inst;
	int signo;
	struct cpudata *pcpu;

	pcpu = CURRENT_CPUDATA;
	syst = u.u_ru.ru_stime;
	p = u.u_procp;
	signo = 0;

	if (USERMODE(sr)) {
		code |= USERFAULT;
	}

	XPRINTF(XPR_TRAP, "trap %R cause %R\n", code, exccode_desc,
	    cause, cause_desc);
	XPRINTF(XPR_TRAP, "trap sr %r\n", sr, sr_desc, 0, 0);

	nofault_save = pcpu->cpu_nofault;
	pcpu->cpu_nofault = 0;

	switch(code) {

	case SEXC_PAGEIN:
	case SEXC_PAGEIN|USERFAULT:
		/* pagein. */
		i = u.u_error;	/* ??? the VAX does this ??? */
		vaddr = ep[EF_BADVADDR];
		pagein(vaddr, 0);
		u.u_error = i;
		pte = vtopte(p, btop(vaddr));
		tlbdropin(p->p_tlbpid, vaddr, *(int *)pte);
		if (code == SEXC_PAGEIN)
			goto done;
		break;


	case EXC_RMISS|USERFAULT:	/* from softfp nofault code */
	case EXC_WMISS|USERFAULT:	/* from softfp nofault code */
		code = SEXC_SEGV|USERFAULT;
		/* fall through */

	case SEXC_SEGV|USERFAULT:
		XPRINTF(XPR_TRAP, "trap USER SEGV badva 0x%x",
		    ep[EF_BADVADDR], 0, 0, 0);
		if (grow(ep[EF_BADVADDR]))
			break;
		XPRINTF(XPR_TRAP, "trap SEGV grow failed", 0, 0, 0, 0);
		u.u_code = code & ~USERFAULT;
		XPRINTF(XPR_SM,"SIGSEGV: faulting addr 0x%x",ep[EF_BADVADDR],
					0,0,0);
		signo = SIGSEGV;
		break;

	case SEXC_RESCHED|USERFAULT:
		astoff();
		if ((u.u_oweupc&SOWEUPC) && (u.u_prof.pr_scale > 1)) {
			addupc(ep[EF_EPC], &u.u_prof, 1);
			u.u_oweupc &= ~SOWEUPC;
		}
		break;

	case EXC_II|USERFAULT:
	case SEXC_CPU|USERFAULT:
		u.u_code = code & ~USERFAULT;
		signo = SIGILL;
		break;

	case EXC_OV|USERFAULT:
		u.u_code = code & ~USERFAULT;
		signo = SIGFPE;
		break;

	case EXC_DBE|USERFAULT:
		/*
		 * interpret instruction to calculate faulting address
		 */
		ep[EF_BADVADDR] = ldst_addr(ep);
		/* fall through */

	case EXC_IBE|USERFAULT:
		u.u_code = code & ~USERFAULT;
		/*
		 * Call processor specific routine to handle trap error.
		 * "signo" is passed as a return parameter: it gets set to
		 *     SIGBUS if we want to terminate the user process.
		 * If the system needs to panic, it does so in the processor
		 *     specific routine.
		 */ 
		if ((*(cpup->machcheck))(ep, code, sr, cause, &signo) < 0)
			panic("no trap error routine configured");
		break;

	case EXC_RADE|USERFAULT:
	case EXC_WADE|USERFAULT:
		if(u.u_procp->p_mips_flag & SFIXADE) {
			if((i = fixade(ep, cause)) == 0) {
				/* success */
				if( k_puac && p->p_puac) {  
					mprintf("Fixed up unaligned data access for pid %d (%s) at pc 0x%x\n",
					p->p_pid, u.u_comm, ep[EF_EPC]);
					uprintf("Fixed up unaligned data access for pid %d (%s) at pc 0x%x\n",
					p->p_pid, u.u_comm, ep[EF_EPC]);
				}
				break;
			} 
			uprintf("pid %d (%s) was killed on %s access, at pc 0x%x \n",
			p->p_pid, u.u_comm,
			(i == 1 ? "a kernel" : "an unaligned"), ep[EF_EPC]);
		}
		u.u_code = code & ~USERFAULT;
		signo = SIGBUS;
		break;

	case EXC_BREAK|USERFAULT:
		signo = SIGTRAP;
		vaddr = ep[EF_EPC];
		if (ep[EF_CAUSE] & CAUSE_BD)
			vaddr += 4;
		inst = fuiword((caddr_t)vaddr);
		u.u_trapcause = (inst == *sstepbp) ? CAUSESINGLE : CAUSEBREAK;
		u.u_code = BRK_SUBCODE(inst);
		break;

	case SEXC_SEGV:
		if (nofault_save && grow(ep[EF_BADVADDR]))
			goto done;
		goto chk_nofault;

	case EXC_DBE:
		/*
		 * interpret instruction to calculate faulting address
		 */
		ep[EF_BADVADDR] = ldst_addr(ep);
		/* fall through */

	case EXC_CPU:
	case EXC_IBE:
	case EXC_RADE:
	case EXC_WADE:
chk_nofault:
		/* nofault handler */
		if (nofault_save) {
			extern (*nofault_pc[])();

			CURRENT_CPUDATA->cpu_nofault_cause = cause;
			CURRENT_CPUDATA->cpu_nofault_badvaddr = ep[EF_BADVADDR];
			i = nofault_save;
			nofault_save = 0;
			if (i < 1 || i >= NF_NENTRIES)
				panic("bad nofault");
			ep[EF_EPC] = (u_int)nofault_pc[i];
			goto done;
		}
		/* fall through to default */
	default:
		/*
		 * Kernel mode errors.
		 * They all panic, its just a matter of what we log
		 *     and what panic message we issue.
		 * Call processor specific routine to log and panic.
		 * If we return to here then continue (this will happen
		 *     when doing a memory dump and we get a cpu read ECC
		 *     error on the dump).
		 */ 
		if ((*(cpup->machcheck))(ep, code, sr, cause, &signo) < 0)
			panic("no trap error routine configured");
		break;
	}
	CURRENT_CPUDATA->cpu_trap++;

	if (signo) {
		XPRINTF(XPR_TRAP, "trap sending signal %N", signo,
		    sig_values, 0, 0);
		psignal(p, signo);
		if (u.u_pcb.pcb_bd_ra) {
			ep[EF_EPC] = u.u_pcb.pcb_bd_epc;
			ep[EF_CAUSE] = u.u_pcb.pcb_bd_cause;
			u.u_pcb.pcb_bd_ra = 0;
		}
	}

	if (p->p_cursig || ISSIG(p,0))
		psig();
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

		(void) splclock();
		smp_lock(&lk_rq,LK_RETRY);
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		pcpu = CURRENT_CPUDATA;
		pcpu->cpu_hlock = pcpu->cpu_proc->p_hlock;
		pcpu->cpu_proc->p_hlock = 0;

	}
	/*
	 * if single stepping this process, install breakpoints before
	 * returning to user mode.  Do this here rather than in procxmt
	 * so single stepping will work when signals are delivered.
	 */
	if (u.u_pcb.pcb_sstep && USERMODE(sr))
		install_bp();
	if (u.u_prof.pr_scale > 1) {
		int ticks = times_to_ticks(&u.u_ru.ru_stime,&syst);
		if (ticks)
			addupc(ep[EF_EPC], &u.u_prof, ticks);
	}

done:
	CURRENT_CPUDATA->cpu_nofault = nofault_save;
}

static char *p_on = "\033[5i";
static char *p_off = "\033[4i";
turnon_printer()
{
	cprintf("%s",p_on);
}

turnoff_printer()
{
	cprintf("%s",p_off);
}

static int dumpstack_once = 0;

dumpstack(ep)
unsigned *ep;
{
	register int i;
	if (dumpstack_once)
		return;
	dumpstack_once++;
	while((unsigned)ep < (unsigned)(KERNELSTACK-4*4)) {
		printf("0x%x\t0x%x\t0x%x\t0x%x\t0x%x\n",
			ep,*ep,*(ep+1),*(ep+2),*(ep+3));
		ep += 4;
		DELAY(300000);
	}
	/* print the last few stack addresses (no more than 4 left) */
	if((unsigned)ep < (unsigned)(KERNELSTACK-4)) {
		printf("0x%x\t0x%x",ep,*ep);
prloop:
		ep++;
		if((unsigned)ep < (unsigned)(KERNELSTACK-4)) {
			printf("\t0x%x",*ep);
			goto prloop;
		}
		printf("\n");
	}
}

/*
 * install_bp -- install breakpoints to implement single stepping
 */
static
install_bp()
{
	unsigned inst;
	unsigned target_pc;

	if (u.u_pcb.pcb_ssi.ssi_cnt)
		panic("install_bp2");
	/*
	 * If user can't access where his pc points, we give up.
	 * He'll be getting a SIGSEGV shortly anyway!
	 */
	if (!useracc(USER_REG(EF_EPC), sizeof(int), B_READ))
		return;
	inst = fuiword(USER_REG(EF_EPC));
	if (is_branch(inst)) {
		target_pc = branch_target(inst, USER_REG(EF_EPC));
		/*
		 * Can't single step self-branches, so just wait
		 * until they fall through
		 */
		if (target_pc != USER_REG(EF_EPC))
			set_bp(target_pc);
		set_bp(USER_REG(EF_EPC)+8);
	} else
		set_bp(USER_REG(EF_EPC)+4);
	/*
	 * only install breakpoints once!
	 */
	u.u_pcb.pcb_sstep = 0;
}

static
set_bp(addr)
unsigned *addr;
{
	register struct ssi_bp *ssibp;

	ssibp = &u.u_pcb.pcb_ssi.ssi_bp[u.u_pcb.pcb_ssi.ssi_cnt];
	ssibp->bp_addr = addr;
	/*
	 * Assume that if the fuiword fails, the write_utext will also
	 */
	ssibp->bp_inst = fuiword(addr);
	if (write_utext(addr, *sstepbp))
		u.u_pcb.pcb_ssi.ssi_cnt++;
}

/*
 * remove_bp -- remove single step breakpoints from current process
 */
remove_bp()
{
	register struct ssi_bp *ssibp;

	while (u.u_pcb.pcb_ssi.ssi_cnt > 0) {
		u.u_pcb.pcb_ssi.ssi_cnt--;
		ssibp = &u.u_pcb.pcb_ssi.ssi_bp[u.u_pcb.pcb_ssi.ssi_cnt];
		if (!write_utext(ssibp->bp_addr, ssibp->bp_inst)) {
			uprintf("couldn't remove breakpoint\n");
			continue;
		}
	}
}

static
is_branch(inst)
{
	union mips_instruction i;

	i.word = inst;
	switch (i.j_format.opcode) {
	case spec_op:
		switch (i.r_format.func) {
		case jr_op:
		case jalr_op:
			return(1);
		}
		return(0);

	case bcond_op:
	case j_op:	case jal_op:	case beq_op:
	case bne_op:	case blez_op:	case bgtz_op:
		return(1);

	case cop0_op:
	case cop1_op:
	case cop2_op:
	case cop3_op:
		switch (i.r_format.rs) {
		case bc_op:
			return(1);
		}
		return(0);
	}
	return(0);
}

#define	REGVAL(x)	((x)?USER_REG((x)+EF_AT-1):0)
static
branch_target(inst, pc)
{
	union mips_instruction i;

	i.word = inst;
	switch (i.j_format.opcode) {
	case spec_op:
		switch (i.r_format.func) {
		case jr_op:
		case jalr_op:
			return(REGVAL(i.r_format.rs));
		}
		break;

	case j_op:
	case jal_op:
		return( ((pc+4)&~((1<<28)-1)) | (i.j_format.target<<2) );

	case bcond_op:
	case beq_op:
	case bne_op:
	case blez_op:
	case bgtz_op:
		return(pc+4+(i.i_format.simmediate<<2));

	case cop0_op:
	case cop1_op:
	case cop2_op:
	case cop3_op:
		switch (i.r_format.rs) {
		case bc_op:
			return(pc+4+(i.i_format.simmediate<<2));
		}
		break;
	}
	panic("branch_target");
}

ldst_addr(ep)
register u_int *ep;
{
	register u_int *pc;
	union mips_instruction i;
	int base;

	pc = (u_int *)ep[EF_EPC];
	if (ep[EF_CAUSE] & CAUSE_BD)
		pc++;
	i.word = *pc;	/* theoretically can't fault */
	if (i.i_format.opcode < lb_op) {
		panic("DBE not on load or store");
	}
	base = (i.i_format.rs == 0) ? 0 : ep[EF_AT + i.i_format.rs - 1];
	return (base + i.i_format.simmediate);
}

	

trap_nofault(ep, code, sr, cause)
register u_int *ep;
register u_int code;
u_int sr, cause;
{
	register int i;
	struct cpudata *pcpu;

	pcpu = CURRENT_CPUDATA;

	switch(code) {

	case EXC_DBE:
		/* nofault handler */
		if (pcpu->cpu_nofault) {
			extern (*nofault_pc[])();
			i = pcpu->cpu_nofault;
			pcpu->cpu_nofault = 0;
			if (i < 1 || i >= NF_NENTRIES)
				panic("bad nofault");
			ep[EF_EPC] = (u_int)nofault_pc[i];
			return;
		}
		/* fall through to default */

	default:
		splhigh();
		panic("trap_nofault");

	}
}

dumpregs(ep)
unsigned *ep;
{
	extern struct reg_desc cause_desc[], sr_desc[];
	/*
	 * Dump out other items of interest
	 */
	printf("Faulting PC: 0x%x\n", ep[EF_EPC]);
	printf("Cause register: %R\n", ep[EF_CAUSE], cause_desc);
	printf("Status register: %R\n", ep[EF_SR], sr_desc);
	printf("Bad Vaddress: 0x%x\n", ep[EF_BADVADDR]);
	printf("Stack Pointer: 0x%x\n", ep[EF_SP]);
}

#ifdef TLBMISS_STUCK
	/* must be power of 2 */
#define VADDR_RING_SIZE (1<<3)
u_int vaddr_ring[VADDR_RING_SIZE];
int vaddr_ring_index = 0;
#endif TLBMISS_STUCK

tlbmiss(ep, code, vaddr, cause)
u_int *ep;
u_int code, vaddr, cause;
{
	register u_int vpn, uvpn;
	register struct pte *pte;
	register u_int kvpn;
	register struct tlbinfo *ti;
	struct pte tpte;
	struct proc *p;
	extern unsigned Syssize;

#if CNT_TLBMISS_HACK==1
	u.u_ru.ru_oublock++;
#endif CNT_TLBMISS_HACK

	/*
	 * Workaround for 3.0/4.0 chip bug.  If badvaddr has been trashed
	 * by chip, epc is correct vaddr
	 */
	if (vaddr == E_VEC)
		vaddr = ep[EF_EPC];

	vpn = btop(vaddr);
	p = u.u_procp;

	pte = (struct pte *)0;
	if (IS_KUSEG(vaddr)) {
		pte = vtopte(p, vpn);
#ifdef TLBMISS_STUCK
		{
		   int i;

		   vaddr_ring[vaddr_ring_index++ & (VADDR_RING_SIZE-1)] = vaddr;
		   for (i=0; i < VADDR_RING_SIZE-1; i++)
		       if (vaddr_ring[i] != vaddr_ring[i+1])
			   break;
		   if (i == VADDR_RING_SIZE-1) {
		       printf("vaddr==%8x\tproc_slot==%d\tpte==%8x\tpte/%8x\n",
			      vaddr, p - &proc[0], pte, *(int *)pte);
		       panic("tlbmiss: stuck on same vaddr");
		   }
	        }
#endif TLBMISS_STUCK
	} else if (IS_KPTESEG(vaddr)) {
		extern char utlbmiss[], eutlbmiss[];
		int is_utlbmiss = ep[EF_EPC] >= UT_VEC
			&& ep[EF_EPC] < UT_VEC + (eutlbmiss - utlbmiss);

		if (!is_utlbmiss && !IS_Forkmap(vaddr)) {
			printf("epc= 0x%x, vaddr= 0x%x\n", ep[EF_EPC], vaddr);
			panic("kpteseg miss outside utlbmiss");
		}

		uvpn = (struct pte *)vaddr - (struct pte *)KPTEBASE;
		if (uvpn >= btop(KUSIZE))
			panic("kpteseg");
		/*
		 * set-up the EPC and SR so that return to place that
		 * caused the original utlbmiss
		 */
		if (is_utlbmiss) {
			ep[EF_EPC] = ep[EF_K1];
			ep[EF_SR] &= ~(SR_KUP|SR_IEP);
			ep[EF_SR] |= (ep[EF_SR] & (SR_KUO|SR_IEO)) >> 2;
		}
		pte = vtopte(p, uvpn);

		if (!pte) {
			/*
			 * User referenced a bad page that caused a two
			 * level miss.  Drop in an invalid pte for the
			 * user and retry the reference so that BADVADDR
			 * can be fully determined for the signal handler.
			 */
			vaddr = (u_int)ptob(uvpn);
			tlbdropin(p->p_tlbpid, vaddr, 0);
			return(0);
		}

		/*
		 * Map kpte window
		 */
		kvpn = btop((unsigned)pte - K2BASE);
		tpte = Sysmap[kvpn];

		if (!tpte.pg_v)
			panic("tlbmiss page table not valid");
		tpte.pg_g = 0;
#if NOMEMCACHE==1
		tpte.pg_n = 1;
#endif
		ti = p->p_tlbinfo;
		if (p->p_tlbindx < NPAGEMAP) {
			ti += p->p_tlbindx;
			ti->lo.tl_word = *(unsigned *)&tpte;
			ti->hi.th_word = (unsigned)vaddr & TLBHI_VPNMASK;
			tlbwired(p->p_tlbindx+UPAGES, p->p_tlbpid, vaddr, tpte);
			p->p_tlbindx++;
		} else {
			tlbdropin(p->p_tlbpid, vaddr, tpte);
		}
		if (!is_utlbmiss)
			return(0);
		vaddr = (u_int)ptob(uvpn);
		ep[EF_BADVADDR] = vaddr;
	} else if (IS_KSEG2(vaddr)) {
		vpn -= btop(K2BASE);
		if (vpn < Syssize) {
			pte = &Sysmap[vpn];
			if (!pte->pg_v) {
				printf("vaddr= 0x%x, pte= 0x%x\n", vaddr, *pte);
				printf("epc = 0x%x\n", ep[EF_EPC]);
				panic("tlbmiss on invalid kernel page");
			}
#if NOMEMCACHE==1
			pte->pg_n = 1;
#endif
		}
	}
	if (pte) {
		XPRINTF(XPR_TLB, "tlbmiss pte va: 0x%x  pte: %r\n", vaddr,
		    *(int *)pte, pte_desc, 0);
		if (!pte->pg_v) {
			/* if "no access"  */
			if (pte->pg_prot < PROT_URKR)
				return(SEXC_SEGV);
			return(SEXC_PAGEIN);
		}
		if (pte->pg_g) {
			tlbdropin(0, vaddr, *(int *)pte);
		} else if ((p->p_tlbpid != -1) || (CURRENT_CPUDATA->cpu_tps[p->p_tlbpid].tps_procpid == p->p_pid)) {
			tlbdropin(p->p_tlbpid, vaddr, *(int *)pte);
		} else {
			panic("tlbmiss no tlbpid assigned");
		}
		return(0);
	}
	XPRINTF(XPR_TLB, "tlbmiss segv va 0x%x\n", vaddr, 0, 0, 0);
	return(SEXC_SEGV);
}

/*
 * Changes for ULTRIX include:
 *  1. if a shared memory segment, then account for some users having
 *     read only and some users having read/write
 */
tlbmod(ep, code, vaddr, cause)
u_int *ep;
{
	register unsigned vpn;
	register struct pte *pte;
	register struct proc *p;
	register int shm_page = 0;
	int smindex;

	vpn = btop(vaddr);
	p = u.u_procp;

	pte = vtopte(p, vpn);

	if (!pte)
		panic("tlbmod on invalid pte");

	XPRINTF(XPR_TLB, "tlbmod vaddr 0x%x pte %r\n", vaddr, *(int *)pte,
	    pte_desc, 0);
	if (pte->pg_prot != PROT_UW) {
		if (isasmsv(p, vpn, &smindex) &&   /* SHMEM */
		    (p->p_sm[smindex].sm_pflag) == PROT_UW) {
			shm_page++;
			goto good;
		}
		return(SEXC_SEGV);
	}
good:
	if (shm_page) {
		pte->pg_swapm = 1;
		tlbdropin(p->p_tlbpid, vaddr, (*(int *)pte | PG_M));
	} else {
		pte->pg_m = 1;
		tlbdropin(p->p_tlbpid, vaddr, *(int *)pte);
	}
		
	return(0);
}

softnet()
{
	extern int netisr;
	register struct isrent {
		int value;
		int (*function)();
	} *pisr;
	extern struct isrent netisr_tab[];	
	int s, signo;

	acksoftnet();

	/* see if any "extreme" priority signals posted by the 
	   fpu code */
	if (CURRENT_CPUDATA->cpu_fpe_event) {
		s = splfpu();	/* sync with fpu */
		/* send the signals...lowest numbered (highest
		   priority) first */
		while(CURRENT_CPUDATA->cpu_fpe_sendsig) {
			signo=ffs(CURRENT_CPUDATA->cpu_fpe_sendsig);
			/* note signo is 1-32 */
			CURRENT_CPUDATA->cpu_fpe_sendsig &= ~(1<<(signo-1));
			splx(s);
			psignal(CURRENT_CPUDATA->cpu_fpe_event,signo);
			s = splfpu();
		}
		CURRENT_CPUDATA->cpu_fpe_event = 0; /* no more */
		splx(s);
	}

	pisr = netisr_tab;

	while(pisr->value != -1) {
		s = splimp();
		if ((netisr & (1<<pisr->value)) &&
		    clear_bit_atomic(pisr->value, &netisr)) {
			splx(s);
			(*pisr->function)();
		} else
			splx(s);
		pisr++;
	}

	if (CURRENT_CPUDATA->cpu_wto_event) {
		if (mmap_debug || 
		   (sm_killp(CURRENT_CPUDATA->cpu_wto_pfn) == -1)){
			/*
			 *	log the information into error log buffer, 
			 *	print the error info on the console and then crash.
			 */
		        (*(cpup->log_errinfo))(CURRENT_CPUDATA->cpu_log_errinfo);
		        (*(cpup->print_consinfo))(CURRENT_CPUDATA->cpu_consinfo);
			CURRENT_CPUDATA->cpu_wto_event = 0;
                        panic("CPU write timeout");
		}
		else 
			CURRENT_CPUDATA->cpu_wto_event = 0;
	}
}

int traceopens = 0;

syscall(ep, code, sr, cause)
register u_int *ep;
u_int code;
{
	register int regparams, nargs;
	register struct sysent *callp;
	register struct proc *p;
	register int *params;
	struct timeval syst;
	extern char *syscallnames[];
	struct cpudata *pcpu;

	syst = u.u_ru.ru_stime;
	u.u_error = 0;
	if (!USERMODE(sr))
		panic("syscall");

	ep[EF_EPC] += 4;		/* normal return executes next inst */

	regparams = EF_A0;
	if (code >= nsysent) {
		callp = &sysent[0];	/* indir (illegal) */
		ep[EF_EPC] -= 4;	/* just leave pc at the syscall inst */
	}
	else {
		callp = &sysent[code];
		if (callp == sysent) {
			/*
			 * indirect system call (syscall), first param is
			 * sys call number
			 */
			code = ep[EF_A0];
			regparams++;
			if (code >= nsysent)
				callp = &sysent[0];
			else
				callp = &sysent[code];
		}
	}
	u.u_eosys = NORMALRETURN;
	params = (int *)u.u_arg;
	for (nargs = callp->sy_narg; nargs && regparams <= EF_A3; nargs--)
		*params++ = ep[regparams++];
	if (nargs) {
		u.u_error = copyin(ep[EF_SP]+4*sizeof(int), params,
		    nargs*sizeof(int));
		if (u.u_error)
			goto done;
	}

	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = ep[EF_V1];

	if (setjmp(&u.u_qsave)) {
		if (CURRENT_CPUDATA->cpu_hlock) {
			while(CURRENT_CPUDATA->cpu_hlock) {
				mprintf("lock held on syscall exit %x pc %x\n",
					CURRENT_CPUDATA->cpu_hlock,
					CURRENT_CPUDATA->cpu_hlock->l_pc);
				smp_unlock(CURRENT_CPUDATA->cpu_hlock);
			}
		}

		if (u.u_error == 0 && u.u_eosys != RESTARTSYS)
			u.u_error = EINTR;
	} else {
		if ( audswitch == 0) {
#ifdef ultrix
#ifdef SYS_TRACE
	                /* trace it just before we do it! (only if open) */
	                if (traceopens) {
	                        syscall_trace(code,callp->sy_narg,BEFORE);
	                        (*(callp->sy_call))(u.u_arg);
	                        syscall_trace(code,callp->sy_narg,AFTER);
	                } else
#endif SYS_TRACE
#endif ultrix
			(*(callp->sy_call))(u.u_arg);
		}
		else {
			u.u_narg = callp->sy_narg;
			u.u_gno_indx = 0;
			u.u_event = code;
#ifdef ultrix
#ifdef SYS_TRACE
	                /* trace it just before we do it! (only if open) */
	                if (traceopens) {
	                        syscall_trace(code,callp->sy_narg,BEFORE);
	                        (*(callp->sy_call))(u.u_arg);
	                        syscall_trace(code,callp->sy_narg,AFTER);
	                } else
#endif SYS_TRACE
#endif ultrix
			(*(callp->sy_call))(u.u_arg);

			if ( aud_param[code][AUD_NPARAM-1] != 'X' )
				AUDIT_CALL ( code, u.u_error, u.u_r.r_val1, AUD_GNO|AUD_HDR|AUD_PRM|AUD_RES, (int *)0, 0 );
		}
	}

done:
	/*
	 * a3 is returned to user 0 if indicate no errors on syscall,
	 * non-zero otherwise
	 */
	if (u.u_eosys == NORMALRETURN) {
		if (u.u_error) {
			ep[EF_V0] = u.u_error;
			ep[EF_A3] = 1;
		} else {
			ep[EF_V0] = u.u_r.r_val1;
			ep[EF_V1] = u.u_r.r_val2;
			ep[EF_A3] = 0;
		}
	} else if (u.u_eosys == RESTARTSYS)
		ep[EF_EPC] -= 4;
	/* else if (u.u_eosys == FULLRESTORE) */
		/* returning from sigreturn, force full state restore */

	p = u.u_procp;
	if (p->p_cursig || ISSIG(p,0))
		psig();


	pcpu = CURRENT_CPUDATA;

	if (smp) {
		if (p->p_affinity != ALLCPU) {
			if (p != pcpu->cpu_fpowner) {
				p->p_affinity = ALLCPU;
			}
		}

	}
	pcpu->cpu_syscall++;
	p->p_pri = p->p_usrpri;
	if (pcpu->cpu_runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */

		(void) splclock();
		smp_lock(&lk_rq,LK_RETRY);
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	/*
	 * if single stepping this process, install breakpoints before
	 * returning to user mode.  Do this here rather than in procxmt
	 * so single stepping will work when signals are delivered.
	 */
	if (u.u_pcb.pcb_sstep)
		install_bp();
	if (u.u_prof.pr_scale > 1) {
		int ticks = times_to_ticks(&u.u_ru.ru_stime,&syst);
		if (ticks)
			addupc(ep[EF_EPC], &u.u_prof, ticks);
	}
	/*
	 * if u_eosys == FULLRESTORE, then force full state restore
	 */
	return (u.u_eosys == FULLRESTORE);
}

int iplmask[IPLSIZE];		/* afdfix: this can vary with cpu type */
int nstrays;


/*
 * TODO: pending interrupts now come for CAUSE reg which is passed as arg4
 */
kn01_intr(ep, code, sr, cause)
u_int *ep;
u_int code, sr, cause;
{
	register int req;
	register int prevmask;
	register int nintr = 0;	/* number of interrupts serviced */
	extern struct reg_desc cause_desc[], sr_desc[];

/*
	XPRINTF(XPR_INTR, "intr cause=%r  sr=%r\n", cause, cause_desc,
	    sr, sr_desc);
*/
	CURRENT_CPUDATA->cpu_inisr++;
	prevmask = (sr & SR_IMASK);
	cause &= prevmask;

	/*
	 * Should probably direct vector these
	 */
	if (req = ffintr(cause)) {
		nintr++;
		req--;
		splx(iplmask[req]);
/*		XPRINTF(XPR_INTR, "calling intr %d\n", req, 0, 0, 0);	*/
		(*c0vec_tbl[req])(ep);
	}
	if (nintr == 0) {
		nstrays++;
	}
	CURRENT_CPUDATA->cpu_intr += nintr;
	CURRENT_CPUDATA->cpu_inisr--;
}

/*
 * Note: stray interrupts cannot happen on the PMAX system
 */
stray_intr(ep)
u_int *ep;
{
	unsigned cause, get_cause();

	cause = get_cause();

	mprintf("intr: stray interrupt\n");
	mprintf("\tPC: 0x%x\n\tCause register: %R\n\tCause register: %R\n\tStatus register: %R\n",
		ep[EF_EPC], ep[EF_CAUSE], cause_desc, cause, cause_desc,
		ep[EF_SR], sr_desc);
}

/*
 * Interrupt for hard error comes here.
 * then we call system specific error handling routine.
 */
memintr(ep)
	u_int *ep;		/* exception frame ptr */
{
	if ((*(cpup->harderr_intr))(ep) < 0)
		panic("no hard error interrupt routine configured");
}

buserror(ep)
u_int *ep;
{
	/*
	 * attempt to scrub the error by writing it to zero
	 */
	wbadaddr(ep[EF_BADVADDR] &~ (sizeof(int)-1), sizeof(int));
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

/*
 * Masks and constants for the rs field of "coprocessor instructions" (25-21)
 * which are branch on coprocessor condition instructions.
 */
#define	COPz_BC_MASK	0x1a
#define COPz_BC		0x08

/*
 * Masks and constants for the rt field of "branch on coprocessor condition
 * instructions" (20-16).
 */
#define	COPz_BC_TF_MASK	0x01
#define	COPz_BC_TRUE	0x01
#define	COPz_BC_FALSE	0x00

#define	PC_JMP_MASK	0xf0000000

/*
 * emulate_branch is used by fpuintr() to calculate the resulting pc of a
 * branch instruction.  It is passed a pointer to the exception frame,
 * the branch instruction and the floating point control and status register.
 * The routine returns the resulting pc.  This routine will panic() if it
 * is called with a non-branch instruction or one it does not know how to
 * emulate.
 */
unsigned
emulate_branch(ep, instr, fpc_csr)
u_int *ep;
unsigned instr;
union fpc_csr fpc_csr;
{
    union mips_instruction cpu_instr;
    long condition;
    long rs, rt;

	cpu_instr.word = instr;

	/*
	 * The values for the rs and rt registers are taken from the exception
	 * frame and since there is space for the 4 argument save registers and
	 * doesn't save register zero this is accounted for (the +3).
	 */
	if(cpu_instr.r_format.rs == 0)
	    rs = 0;
	else
	    rs = ep[cpu_instr.r_format.rs + 3];
	if(cpu_instr.r_format.rt == 0)
	    rt = 0;
	else
	    rt = ep[cpu_instr.r_format.rt + 3];

	switch(cpu_instr.i_format.opcode){

	case spec_op:
	    switch(cpu_instr.r_format.func){
	    case jalr_op:
		/* r31 has already been updated by the hardware */
	    case jr_op:
		return(rs);
	    }
	    break;

	case jal_op:
	    /* r31 has already been updated by the hardware */
	case j_op:
	    return(((ep[EF_EPC] + 4) & PC_JMP_MASK) |
		   (cpu_instr.j_format.target << 2));

	case beq_op:
	    condition = rs == rt;
	    goto conditional;

	case bne_op:
	    condition = rs != rt;
	    goto conditional;

	case blez_op:
	    condition = rs <= 0;
	    goto conditional;

	case bgtz_op:
	    condition = rs > 0;
	    goto conditional;

	case bcond_op:
	    switch(cpu_instr.r_format.rt){
	    case bltzal_op:
		/* r31 has already been updated by the hardware */
	    case bltz_op:
		condition = rs < 0;
		goto conditional;

	    case bgezal_op:
		/* r31 has already been updated by the hardware */
	    case bgez_op:
		condition = rs >= 0;
		goto conditional;
	    }
	    break;

	case cop1_op:
	    if((cpu_instr.r_format.rs & COPz_BC_MASK) == COPz_BC){
		if((cpu_instr.r_format.rt & COPz_BC_TF_MASK) == COPz_BC_TRUE)
		    condition = fpc_csr.fc_struct.condition;
		else
		    condition = !(fpc_csr.fc_struct.condition);
		goto conditional;
	    }

	}
	/*
	 * For all other instructions (including branch on co-processor 2 & 3)
	 * we panic because this routine is only called when in the branch
	 * delay slot (as indicated by the hardware).
	 */

	if (CURRENT_CPUDATA->cpu_noproc) {
		printf("Unknown branch instruction = 0x%x\n", instr);
		panic("Unknown branch instruction");
	} else {
		
		CURRENT_CPUDATA->cpu_fpe_event = u.u_procp;
		CURRENT_CPUDATA->cpu_fpe_sendsig |= sigmask(SIGILL);
		setsoftnet();
				
	}

conditional:
	if(condition)
	    return(ep[EF_EPC] + 4 + (cpu_instr.i_format.simmediate << 2));
	else
	    return(ep[EF_EPC] + 8);
}

/*
 * Fixade() is called to fix unaligned loads and stores.  It returns a
 * zero value if can fix it and a non-zero error code if it can't fix it.
 * Error codes are:
 *	1 == its a kernel access from user space.
 *	2 == we couldn't fixup the alignment.
 *
 * Fixade() modifies the destination register (general or
 * floating-point) for loads or the destination memory location for
 * stores.  Also the epc is advanced past the instruction (possibly to the
 * target of a branch).
 */
fixade(ep, cause)
register u_int *ep;
u_int cause;
{
    union mips_instruction inst, branch_inst;
    u_int addr, new_epc, word;
    int error;


	if(cause & CAUSE_BD){
	    branch_inst.word = fuiword((caddr_t)ep[EF_EPC]);
	    inst.word = fuiword((caddr_t)(ep[EF_EPC] + 4));
	    if(branch_inst.i_format.opcode == cop1_op)
		checkfp(u.u_procp, 0);
	    new_epc = emulate_branch(ep, branch_inst.word, u.u_pcb.pcb_fpc_csr);
	}
	else{
	    inst.word = fuiword((caddr_t)ep[EF_EPC]);
	    new_epc = ep[EF_EPC] + 4;
	}

	addr = REGVAL(inst.i_format.rs) + inst.i_format.simmediate;

	/*
	 * The addresses of both the left and right parts of the reference
	 * have to be checked.  If either is a kernel address it is an
	 * illegal reference.
	 */
	if(addr >= K0BASE || addr+3 >= K0BASE)
	    return(1);

	error = 0;

	switch(inst.i_format.opcode){
	case lw_op:
	    error = uload_word(addr, &word);
	    if(inst.i_format.rt == 0)
		break;
	    else
	    	ep[inst.i_format.rt+3] = word;
	    break;
	case lh_op:
	    error = uload_half(addr, &word);
	    if(inst.i_format.rt == 0)
		break;
	    else
	    	ep[inst.i_format.rt+3] = word;
	    break;
	case lhu_op:
	    error = uload_uhalf(addr, &word);
	    if(inst.i_format.rt == 0)
		break;
	    else
	    	ep[inst.i_format.rt+3] = word;
	    break;
	case lwc1_op:
	    checkfp(u.u_procp, 0);
	    error = uload_word(addr, &word);
	    u.u_pcb.pcb_fpregs[inst.i_format.rt] = word;
	    break;

	case sw_op:
	    error = ustore_word(addr, REGVAL(inst.i_format.rt));
	    break;
	case sh_op:
	    error = ustore_half(addr, REGVAL(inst.i_format.rt));
	    break;
	case swc1_op:
	    checkfp(u.u_procp, 0);
	    error = ustore_word(addr, u.u_pcb.pcb_fpregs[inst.i_format.rt]);
	    break;

	default:
	    return(2);
	}
	
	if(error)
	    return(2);

	ep[EF_EPC] = new_epc;
	return(0);
}

/*
 * Convert a virtual address to a physical address.
 * The virtual address is NOT guaranteed to be good, so we must continually
 *     check values before referencing pointers.
 */
caddr_t
vatophys(va)
	register caddr_t va;	/* the virtual addr to convert to physical */	
{
	caddr_t pa;			/* the physical addr */	
	register struct pte *pte;	/* pointer to page table */
	register unsigned pf;		/* page frame number */

	if (IS_KSEG0(va))
		pa = (caddr_t)K0_TO_PHYS(va);
	else if (IS_KSEG1(va))
		pa = (caddr_t)K1_TO_PHYS(va);
	else if (IS_KSEG2(va)) {
		pte = (struct pte *)(&Sysmap[btop((int)(va) & ~SEG_BITS)]);
		/* 
		 * kseg2 can have a 0 pfnum, but can it be mapped to anything?
		 * if so, don't do the pfnum test here
		 */
		if (pte->pg_v && pte->pg_pfnum)
			pa = (caddr_t)((int)ptob(pte->pg_pfnum) | ((int) (va) & VA_BYTEOFFS));
		else
			pa = (caddr_t)-1;
	}
	else if (IS_KUSEG(va)) {
		if ((pte = vtopte(u.u_procp, btop(va))) == 0)
			pa = (caddr_t)-1;
		else if (pte->pg_v && pte->pg_pfnum)
			pa = (caddr_t)((int)ptob(pte->pg_pfnum) | ((int) (va) & VA_BYTEOFFS));
		else
			pa = (caddr_t)-1;
	}
	else pa = (caddr_t)-1;
	return(pa);
}

int     cpeintvl = CPEINTVL;	/* how often to log cache parity err count */
int     cpecount = 0;		/* current cache parity error count */
int     cpelogcount = 0;	/* count at last log */
/*
 * This routine is initially called by startup() (after config), thereafter
 * it is called by timeout.  Its purpose is to log cache parity errors.
 * The count of cache parity errors is kept by hardclock.
 */
chk_cpe()
{
	if (cpecount > cpelogcount) {
		mprintf("Cache parity error count is %d\n", cpecount);
		cpelogcount = cpecount;
	}
	if (cpeintvl > 0)
		timeout (chk_cpe, (caddr_t) 0, cpeintvl * hz);
}

/*
 * Determines what spl we are currently set to.
 * Returns a define const (see cpu.h) for the integer number corresponding
 *    to the interrupt bits set in the Status Register.
 *
 */
kn01_whatspl(sr)
unsigned sr;
{
	register int	index;		/* index into the splm array */
	register int	imask;		/* mask value we are looking for */
	register int	found = 0;	/* Gets us out of the loop */
	extern	 int	splm[];

	imask = (sr & SR_IMASK);
	index = (SPLMSIZE);
	while ((index >= 0) && !found) {
		index--;
		switch (index) {
		      case 4:	/* not used */
		      case 3:	/* not used */
			break;
		      default:
			if ((splm[index] & SR_IMASK) == imask) {
				found = 1;
			}
			break;
		}
		if (found)
			return(index);
	}
	return (0);
}

