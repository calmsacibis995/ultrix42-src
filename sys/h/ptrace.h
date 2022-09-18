/*	@(#)ptrace.h	4.3	(ULTRIX)	9/6/90	*/
/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ptrace.h	7.1 (Berkeley) 6/4/86
 *
 * 9/4/90	dlh
 *	added two ptrace request numbers:
 *		PT_READ_V	read vector context or reg
 *		PT_WRITE_V	write vector context or reg
 *
 */

#ifndef _PTRACE_
#define _PTRACE_

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * ptrace request numbers
 */
#define PT_TRACE_ME	0	/* child declares it's being traced */
#define PT_READ_I	1	/* read word in child's I space */
#define PT_READ_D	2	/* read word in child's D space */
#define PT_READ_U	3	/* read registers in child's user structure */
#define PT_WRITE_I	4	/* write word in child's I space */
#define PT_WRITE_D	5	/* write word in child's D space */
#define PT_WRITE_U	6	/* write registers in child's user structure */
#define PT_CONTINUE	7	/* continue the child */
#define PT_KILL		8	/* kill the child process */
#define PT_STEP		9	/* single step the child */
#define PT_READ_V	10	/* read vector context or reg (32 bits) */
#define PT_WRITE_V	11	/* write vector context or reg (32 bits) */

#ifdef __mips

/*
 * register number definitions for PT_READ_U's and PT_WRITE_U's
 */
#define GPR_BASE	0			/* general purpose regs */
#define	NGP_REGS	32			/* number of gpr's */

#define FPR_BASE	(GPR_BASE+NGP_REGS)	/* fp regs */
#define	NFP_REGS	32			/* number of fp regs */

#define	SIG_BASE	(FPR_BASE+NFP_REGS)	/* sig handler addresses */
#define	NSIG_HNDLRS	32			/* number of signal handlers */

#define SPEC_BASE	(SIG_BASE+NSIG_HNDLRS)	/* base of spec purpose regs */
#define PC		SPEC_BASE+0		/* program counter */
#define	CAUSE		SPEC_BASE+1		/* cp0 cause register */
#define MMHI		SPEC_BASE+2		/* multiply high */
#define MMLO		SPEC_BASE+3		/* multiply low */
#define FPC_CSR		SPEC_BASE+4		/* fp csr register */
#define FPC_EIR		SPEC_BASE+5		/* fp eir register */
#define TRAPCAUSE	SPEC_BASE+6		/* multiplex SIGTRAP cause */
#define TRAPINFO	SPEC_BASE+7		/* associated info to SIGTRAP */
#define NSPEC_REGS	8			/* number of spec registers */
#define NPTRC_REGS	(SPEC_BASE + NSPEC_REGS)

/*
 * causes for SIGTRAP
 */
#define CAUSEEXEC	1		/* traced process exec'd */
#define CAUSEFORK	2		/* traced process fork'd */
#define CAUSEWATCH	3		/* traced process hit a watchpoint */
#define CAUSESINGLE	4		/* traced process executed 1 instr */
#define CAUSEBREAK	5		/* traced process hit a breakpoint */
#define	CAUSETRACEON	6		/* initial trap after PTRC_TRACEON */

#endif /* __mips */

#endif /* _PTRACE_ */
