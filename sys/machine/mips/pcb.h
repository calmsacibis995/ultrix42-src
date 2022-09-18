/*	@(#)pcb.h	4.1	(ULTRIX)	7/2/90	*/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#ifdef KERNEL
#include "../h/cpudata.h"
#else
#include <sys/cpudata.h>
#endif /* KERNEL */

/*************************************************************************
 * Revision History
 *
 * 13-Oct-89 -- gmm
 *	smp support. added pointer to cpudata in pcb structure
 *
 *************************************************************************/

/*
 * MIPS process control block
 * MUST be first element of upage
 */

/*
 * pcb_regs indices
 */
#define	PCB_S0		0	/* callee saved regs.... */
#define	PCB_S1		1
#define	PCB_S2		2
#define	PCB_S3		3
#define	PCB_S4		4
#define	PCB_S5		5
#define	PCB_S6		6
#define	PCB_S7		7
#define	PCB_SP		8	/* stack pointer */
#define	PCB_S8		9	/* another callee save reg */
#define	PCB_PC		10	/* program counter */
#define	PCB_SR		11	/* C0 status register */
#define	NPCBREGS	12	/* number of regs saved at ctxt switch */

/*
 * jmp_buf offsets
 * WARNING:
 * if this changes, label_t in types.h must change
 */
#define	JB_S0		0	/* callee saved regs.... */
#define	JB_S1		1
#define	JB_S2		2
#define	JB_S3		3
#define	JB_S4		4
#define	JB_S5		5
#define	JB_S6		6
#define	JB_S7		7
#define	JB_SP		8	/* stack pointer */
#define	JB_S8		9	/* another callee saved reg */
#define	JB_PC		10	/* program counter */
#define	JB_SR		11	/* C0 status register */
#define	NJBREGS		12

#ifndef LOCORE
/*
 * single step information
 * used to hold instructions that have been replaced by break's when
 * single stepping
 */
struct ssi {
	int ssi_cnt;			/* number of bp's installed */
	struct ssi_bp {
		unsigned *bp_addr;	/* address of replaced instruction */
		unsigned bp_inst;	/* replaced instruction */
	} ssi_bp[2];
};

struct pcb
{
	/*
	 * General purpose registers saved at context switch time.
	 *
	 * NOTE: current switch assembler code assumes that pcb is first
	 * entry in u area, and that pcb_regs is the first thing in the
	 * pcb.
	 */
	int	pcb_regs[32];
	int	pcb_pc;
	int	pcb_sr;
	/*
	 * Misc.
	 */
	int	*pcb_sswap;	/* ptr for non-local resume */
	int	pcb_resched;	/* non-zero if time to resched */
	int	pcb_sstep;	/* non-zero if single stepping */
	struct	ssi pcb_ssi;	/* single step state info */
	/* These are use in branch delay instruction emulation */
	int	pcb_bd_epc;	/* epc register */
	int	pcb_bd_cause;	/* cause register */
	int	pcb_bd_ra;	/* address to return to if doing bd emulation */
	int	pcb_bd_instr;	/* the branch instr for the bd emulation */
	/* This is use in fp instruction emulation */
	int	pcb_softfp_pc;	/* resulting pc after fp emulation */
	/*
	 * Space for the state of all the potential coprocessors. WASTEFUL!
	 */
	int	pcb_fpregs[32];	/* floating point */
	int	pcb_fpc_csr;	/* floating point control and status reg */
	int	pcb_fpc_eir;	/* floating point exception instruction reg */
	int	pcb_ownedfp;	/* has owned fp at one time */
	int	pcb_c2regs[32];	/* TBD */
	int	pcb_c3regs[32];	/* TBD */
	struct	cpudata	*pcb_cpuptr;  /* pointer to the cpudata structure of the
					processor */
};

#define	aston() \
	{ \
		u.u_pcb.pcb_resched = 1; \
	}

#define	astoff() \
	{ \
		u.u_pcb.pcb_resched = 0; \
	}
#endif /* !LOCORE */
