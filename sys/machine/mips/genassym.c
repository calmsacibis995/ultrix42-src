#ifndef lint
static	char	*sccsid = "@(#)genassym.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)genassym.c	7.1 (Berkeley) 6/5/86
 */

/*
 * genassym.c -- generate struct offset constants for assembler files
 *
 * NOTE: genassym should only be used for struct offsets, or other constants
 * that cannot be easily #defined.  Constants which appear as #defines and are
 * needed by assembler files should be obtained by including the appropriate
 * .h file (adding #ifndef LOCORE's as necessary).
 */

/*
 * Revision History
 *
 *
 * 16-Apr-90 -- jaw/gmm
 *	move kstackflg to per-cpu structure.
 *
 * 29-Mar-90 gmm/jaw
 *	Added cpu_fpe_event and cpu_fpe_sendsig
 *
 * 03-Mar-90 jaw
 *	primitive change to optimize mips.
 * 
 * 12-Dec-89 -- burns
 *	Moved SYSPTSIZE calculation here to get better control over
 *	the size of the SYSMAP.
 *
 * 09-Nov-89 -- jaw
 *	remove unused vmmeter words.
 *
 * 13-Oct-89  -- gmm
 *	smp support
 *
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/vmmeter.h"
#include "../h/vmparam.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/mbuf.h"
#include "../h/cpudata.h" 
#ifdef old
#include "../h/msgbuf.h"
#endif old

#ifdef PROFILING
#include "prof.h"
#endif PROFILING

#define CNT_TLBMISS_HACK 0

/*
 * Support for defineition of SYSPTSIZE based upon physmem,
 * and the buffer cache.
 */
#ifdef	PHYSMEM
int	physmem_est = PHYSMEM;
#else
int	physmem_est = MINMEM_MB;	/* The way the vax does it. */
#endif

#ifdef	BUFCACHE
int	bufcache = BUFCACHE;
#else
int	bufcache = 10;
#endif

main()
{
	register struct proc *p = (struct proc *)0;
	register struct vmmeter *vm = (struct vmmeter *)0;
	register struct user *up = (struct user *)0;
	register struct rusage *rup = (struct rusage *)0;
	struct text *tp = (struct text *)0;
	struct pcb *pcb = (struct pcb *)0;
	struct cpudata *cpu = (struct cpudata *)0;
	struct lock_t *l = (struct lock_t *) 0;
	int nbufs;	/* number of buffer headers */
	int bufptes;	/* # of PTEs for buffer cache */
#if CNT_TLBMISS_HACK==1
	register struct user *aup = &u;
#endif CNT_TLBMISS_HACK
#ifdef PROFILING

#if PROFTYPE == 4
	struct tostruct *tos = (struct tostruct *)0;
#endif PROFTYPE == 4

#endif PROFILING

	printf("#ifdef LOCORE\n");
	printf("#define\tP_LINK %d\n", &p->p_link);
	printf("#define\tP_RLINK %d\n", &p->p_rlink);
	printf("#define\tP_XLINK %d\n", &p->p_xlink);
	printf("#define\tP_ADDR %d\n", &p->p_addr);
	printf("#define\tP_TLBPID %d\n", &p->p_tlbpid);
	printf("#define\tP_TLBINFO %d\n", &p->p_tlbinfo);
	printf("#define\tP_PRI %d\n", &p->p_pri);
	printf("#define\tP_STAT %d\n", &p->p_stat);
	printf("#define\tP_WCHAN %d\n", &p->p_wchan);
	printf("#define\tP_TSIZE %d\n", &p->p_tsize);
	printf("#define\tP_SSIZE %d\n", &p->p_ssize);
	printf("#define\tP_TEXTBR %d\n", &p->p_textbr);
	printf("#define\tP_DATABR %d\n", &p->p_databr);
	printf("#define\tP_STAKBR %d\n", &p->p_stakbr);
	printf("#define\tP_TEXTPT %d\n", &p->p_textpt);
	printf("#define\tP_DATAPT %d\n", &p->p_datapt);
	printf("#define\tP_STAKPT %d\n", &p->p_stakpt);
	printf("#define\tP_TEXTP %d\n", &p->p_textp);
	printf("#define\tP_SCHED %d\n", &p->p_sched);
	printf("#define\tP_FP %d\n", &p->p_fp);
	printf("#define\tP_AFFINITY %d\n", &p->p_affinity);
#if CNT_TLBMISS_HACK==1
	printf("#define\tCNT_UTLBMISS %d\n", &aup->u_ru.ru_inblock);
	printf("#define\tCNT_TLBMISS %d\n", &aup->u_ru.ru_oublock);
#endif CNT_TLBMISS_HACK
	printf("#define\tX_CADDR %d\n", &tp->x_caddr);
	printf("#define\tV_PDMA %d\n", &vm->v_pdma);
	printf("#define\tV_FAULTS %d\n", &vm->v_faults);
	printf("#define\tV_PGREC %d\n", &vm->v_pgrec);
	printf("#define\tV_FASTPGREC %d\n", &vm->v_fastpgrec);
#ifdef old
	printf("#define\tMSGBUFPTECNT %d\n", btoc(sizeof (struct msgbuf)));
#endif old
	printf("#define\tNBPW %d\n", NBPW);
	printf("#define\tNPTEPG %d\n", NPTEPG);
	printf("#define\tU_PROCP %d\n", &up->u_procp);
	printf("#define\tU_RU %d\n", &up->u_ru);
	printf("#define\tRU_MINFLT %d\n", &rup->ru_minflt);
	printf("#define\tPCB_REGS %d\n", pcb->pcb_regs);
	printf("#define\tPCB_SSWAP %d\n", &pcb->pcb_sswap);
	printf("#define\tPCB_RESCHED %d\n", &pcb->pcb_resched);
	printf("#define\tPCB_OWNEDFP %d\n", &pcb->pcb_ownedfp);

	printf("#define\tPCB_FPREGS %d\n", pcb->pcb_fpregs);
	printf("#define\tPCB_FPC_CSR %d\n", &pcb->pcb_fpc_csr);
	printf("#define\tPCB_FPC_EIR %d\n", &pcb->pcb_fpc_eir);

	printf("#define\tPCB_C2REGS %d\n", pcb->pcb_c2regs);
	printf("#define\tPCB_C3REGS %d\n", pcb->pcb_c3regs);

	printf("#define\tPCB_BD_EPC %d\n", &pcb->pcb_bd_epc);
	printf("#define\tPCB_BD_CAUSE %d\n", &pcb->pcb_bd_cause);
	printf("#define\tPCB_BD_RA %d\n", &pcb->pcb_bd_ra);
	printf("#define\tPCB_BD_INSTR %d\n", &pcb->pcb_bd_instr);

	printf("#define\tPCB_SOFTFP_PC %d\n", &pcb->pcb_softfp_pc);
	printf("#define\tPCB_CPUPTR %d\n", &pcb->pcb_cpuptr);
	printf("#define\tCPU_HLOCK %d\n", &cpu->cpu_hlock);
	printf("#define\tCPU_FPE_EVENT %d\n", &cpu->cpu_fpe_event);
	printf("#define\tCPU_FPE_SENDSIG %d\n", &cpu->cpu_fpe_sendsig);
	printf("#define\tCPU_KSTACK %d\n", &cpu->cpu_newpc);
	printf("#define\tL_PLOCK %d\n",&l->l_plock);
	printf("#define\tL_PC %d\n",&l->l_pc);
	printf("#define\tL_WON %d\n",&l->l_won);
	printf("#define\tL_LOST %d\n",&l->l_lost);

	printf("#define\tCPU_FPOWNER %d\n", &cpu->cpu_archdep.cp_fpowner);
	printf("#define\tCPU_NOFAULT %d\n", &cpu->cpu_archdep.cp_nofault);
	printf("#define\tCPU_NOFAULT_CAUSE %d\n", &cpu->cpu_archdep.cp_nofault_cause);
	printf("#define\tCPU_NOFAULT_BADVADDR %d\n", &cpu->cpu_archdep.cp_nofault_badvaddr);
	printf("#define\tCPU_MASK %d\n", &cpu->cpu_mask);

#ifdef PROFILING

#if PROFTYPE == 4
	printf("#define\tTOS_LINK %d\n", &tos->link);
	printf("#define\tTOS_COUNT %d\n", &tos->count);
	printf("#define\tTOS_SELFPC %d\n", &tos->selfpc);
	printf("#define\tTOS_SIZE %d\n", sizeof(*tos));
#endif PROFTYPE == 4

#endif PROFILING

	printf("#endif\n");

	/* Adjust physmem_est if necessary. */
	if (physmem_est < MINMEM_MB)
		physmem_est = MINMEM_MB;
	else if (physmem_est > MAXMEM_MB)
		physmem_est = MAXMEM_MB;


	/* Calculate amount of memory (in 1K units) used for buffer data. */
	/* Assume 1 buffer header for every 2K of buffer data. */
	nbufs = ((physmem_est * 1024) * ((float)bufcache / 100)) / 2;

	/* Calculate # of ptes needed for buffer data. */
	bufptes = ((nbufs * MAXBSIZE) /NBPG);
	/* the current mips assembler has a bug that causes lcomm directives of
	   odd amounts to mess up. This badly trashes the sysmap scheme. Thus this
	   hack */
	if (bufptes & 1)
		bufptes++;

	printf("/*physmem=%d bufcache=%d, SYSPTSIZE=%d*/ \n", physmem_est, bufcache, bufptes);
	printf("#ifdef SAS\n");
	printf("#include \"md.h\"\n");
	printf("#define\tSYSPTSIZE (%d+NMD)\n", bufptes);
	printf("#else SAS\n");
	printf("#define\tSYSPTSIZE %d\n", bufptes);
	printf("#endif SAS\n");

	exit (0);
}
