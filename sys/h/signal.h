/*	@(#)signal.h	2.8	(ULTRIX)	11/9/89	*/

/************************************************************************
 *									*
 *			Copyright (c) 1987-1989 by			*
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
 *	Debby Haeck 11/14/90						*
 *		added new Vector Arithmetic Exception handling codes	*
 *	Debby Haeck 9/4/90						*
 *		added new u_code for vector support			*
 *		ILL_VECOP_FAULT, ILL_VECINST_FAULT, TERM_VECT_HARD and	*
 *		TERM_VECT_TOOMANY					*
 *	Tak Yin Wong 3/390                                              *
 *		Add ifdef's for POSIX and XOPEN                         *
 *	Linda Wilson 9/12/89                                            *
 *		typedef sigset_t for X/OPEN			        *
 *	Linda Wilson 9/12/89                                            *
 *      	ifdef out sigmask for POSIX                             *
 *	Jon Reeves 7/14/89						*
 *		Add X/Open mandated function declarations.		*
 *	Jon Reeves 5/16/89						*
 *		Add new BRK_STACKOVERFLOW def from MIPS 2.0 cmplrs	*
 *	Jon Reeves 5/12/89						*
 *		Add raise() and sig_atomic_t for ANSI			*
 *	Mark Parenti 2/06/88						*
 *		Change SA_CLDSTOP to SA_NOCLDSTOP per POSIX change	*
 *	Fred Glover 1/12/88						*
 *		Add SIGLOST - server crash Sys-V lock notification      *
 *									*
 *	Larry Cohen 10/1/85						*
 *		Add SIGWINCH - window change signal			*
 *									*
 *	Greg Depp  25 Jun 85						*
 *	Moved SIGUSR1 and SIGUSR2 to 30 and 31 to conform with Berkeley *
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001 Add definitions for System V compatibility			*
 *									*
 ************************************************************************/


#ifndef	_SIGNAL_H_
#define _SIGNAL_H_

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#if !defined(_POSIX_SOURCE)
#define NSIG	32
#endif /* !defined(_POSIX_SOURCE) */

#define	SIGHUP     1	/* hangup */
#define	SIGINT     2	/* interrupt */
#define	SIGQUIT    3	/* quit */
#define	SIGILL     4	/* illegal instruction (not reset when caught) */

#if !defined(_POSIX_SOURCE)
#define	    ILL_RESAD_FAULT	0x0	/* reserved addressing fault */
#define	    ILL_PRIVIN_FAULT	0x1	/* privileged instruction fault */
#define	    ILL_RESOP_FAULT	0x2	/* reserved operand fault */
#define	    ILL_VECOP_FAULT	0x100	/* illegal vector opcode */
#define	    ILL_VECINST_FAULT	0x101	/* legal vector opcode on vector    */
					/* absent or non-vector capable sys */
/* CHME, CHMS, CHMU are not yet given back to users reasonably */
#endif /* !defined(_POSIX_SOURCE) */

#define	SIGTRAP    5	/* trace trap (not reset when caught) */
#define	SIGIOT     6	/* IOT instruction */
#define	SIGEMT     7	/* EMT instruction */
#define	SIGFPE     8	/* floating point exception */

#if !defined(_POSIX_SOURCE)
#define	    FPE_INTOVF_TRAP	0x1	/* integer overflow */
#define	    FPE_INTDIV_TRAP	0x2	/* integer divide by zero */
#define	    FPE_FLTOVF_TRAP	0x3	/* floating overflow */
#define	    FPE_FLTDIV_TRAP	0x4	/* floating/decimal divide by zero */
#define	    FPE_FLTUND_TRAP	0x5	/* floating underflow */
#define	    FPE_DECOVF_TRAP	0x6	/* decimal overflow */
#define	    FPE_SUBRNG_TRAP	0x7	/* subscript out of range */
#define	    FPE_FLTOVF_FAULT	0x8	/* floating overflow fault */
#define	    FPE_FLTDIV_FAULT	0x9	/* divide by zero floating fault */
#define	    FPE_FLTUND_FAULT	0xa	/* floating underflow fault */

/* the following are used to for Vector Arithmetic Exception handling */

#define	FPE_VECTOR           0x3e0	/* Vector Arithmetic Exception Mask */
#define	VFPE_FLTUND_TRAP      0x20	/* Floating Underflow */
#define	VFPE_FLTDIV_TRAP      0x40	/* Floating Divide by Zero */
#define	VFPE_FLTRSV_TRAP      0x80	/* Floating Reserved Operan */
#define	VPFE_FLTOVF_FAULT   0x0100	/* Floating Overflow */
#define	VFPE_INTOVF_TRAP    0x0200	/* Integer Overflow */
#endif /* !defined(_POSIX_SOURCE) */

#define	SIGKILL    9	/* kill (cannot be caught or ignored) */
#define	SIGBUS    10	/* bus error */
#define	SIGSEGV   11	/* segmentation violation */
#define	SIGSYS    12	/* bad argument to system call */
#define	SIGPIPE   13	/* write on a pipe with no one to read it */
#define	SIGALRM   14	/* alarm clock */
#define	SIGTERM   15	/* software termination signal from kill */

#if !defined(_POSIX_SOURCE)
#define	    TERM_VECT_HARD	0x1	/* terminated - vector hardware err */
#define	    TERM_VECT_TOOMANY	0x2	/* terminated - too many vect procs */
#endif /* !defined(_POSIX_SOURCE) */

#define	SIGURG    16	/* urgent condition on IO channel */
#define	SIGSTOP   17	/* sendable stop signal not from tty */
#define	SIGTSTP   18	/* stop signal from tty */
#define	SIGCONT   19	/* continue a stopped process */
#define	SIGCHLD   20	/* to parent on child stop or exit */
#define	SIGTTIN   21	/* to readers pgrp upon background tty read */
#define	SIGTTOU   22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO     23	/* input/output possible signal */
#define	SIGXCPU   24	/* exceeded CPU time limit */
#define	SIGXFSZ   25	/* exceeded file size limit */
#define	SIGVTALRM 26	/* virtual time alarm */
#define	SIGPROF   27	/* profiling time alarm */
#define SIGWINCH  28	/* window size changes */
#define SIGLOST	  29	/* Sys-V rec lock: notify user upon server crash */
#define SIGUSR1   30	/* User signal 1 (from SysV) */
#define SIGUSR2   31	/* User signal 2 (from SysV) */

/* Add System V signal definitions (DLB001) */
#define SIGCLD	SIGCHLD	/* System V name for SIGCHLD	*/
#define SIGABRT	SIGIOT	/* Added from BRL package for /usr/group. Signal
			 * returned by abort(3C).  Map onto SIGIOT.
			 */

#if !defined(_POSIX_SOURCE)
#ifdef __mips
/*
 * Codes for the mips break instruction.
 */
#define	BRK_USERBP	0	/* user bp (used by debuggers) */
#define	BRK_KERNELBP	1	/* kernel bp (used by prom) */
#define	BRK_ABORT	2	/* no longer used */
#define	BRK_BD_TAKEN	3	/* for taken bd emulation */
#define	BRK_BD_NOTTAKEN	4	/* for not taken bd emulation */
#define	BRK_SSTEPBP	5	/* user bp (used by debuggers) */
#define	BRK_OVERFLOW	6	/* overflow check */
#define	BRK_DIVZERO	7	/* divide by zero check */
#define	BRK_RANGE	8	/* range error check */
#define	BRK_STACKOVERFLOW	9	/* used by Ada */
#endif /* __mips */
#endif /* !defined(_POSIX_SOURCE) */

#if defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C))
#ifndef	__SYSTEM_FIVE

/* Accesses to sig_atomic_t are atomic, even with async interrupts.
   Not an issue for us, but ANSI requires the definition. */
typedef long	sig_atomic_t;
typedef int sigset_t;	/* type used for sigsetops() functions  */

#if !defined(_POSIX_SOURCE)
/*
 * Signal vector "template" used in sigvec call.
 */
struct	sigvec {
	void	 (*sv_handler)();	/* signal handler */
	sigset_t sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};
#endif /* !defined(_POSIX_SOURCE) */

/*
 * The following structure must be exactly the same as the above structure
 * with the names changed for POSIX compliance.
 */
struct	sigaction {
	void	 (*sa_handler)();	/* signal handler */
	sigset_t sa_mask;		/* signal mask to apply */
	int	 sa_flags;		/* see signal options below */
};

#if !defined(_POSIX_SOURCE)
#define SV_ONSTACK      0x0001  /* take signal on signal stack */
#define SV_INTERRUPT    0x0002  /* do not restart system on signal return */
#endif /* !defined(_POSIX_SOURCE) */

#define	SA_NOCLDSTOP	0x0004	/* Don't generate SIGCLD when children stop */

#if !defined(_POSIX_SOURCE)
#define	SV_OLDSIG	0x1000	/* Emulate old signal() for POSIX */
#define sv_onstack sv_flags     /* isn't compatibility wonderful! */
#endif /* !defined(_POSIX_SOURCE) */


/* Defines for sigprocmask() call. POSIX.
 */
#define	SIG_BLOCK	1		/* Add these signals to block mask	*/
#define	SIG_UNBLOCK	2		/* Remove these signals from block mask */
#define	SIG_SETMASK	3		/* Set block mask to this mask		*/


#if !defined(_POSIX_SOURCE)
/*
 * Structure used in sigstack call.
 */
struct	sigstack {
	char	*ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to properly restore state if
 * a non-standard exit is performed.
 *
#ifdef __vax
 *	XXX - sigcontext needs updating per 4.3BSD - rr
 *
#endif __vax
#ifdef __mips
 * WARNING: THE sigcontext MUST BE KEPT CONSISTENT WITH /usr/include/setjmp.h
 * AND THE LIBC ROUTINES setjmp() AND longjmp()
#endif __mips
 */
struct	sigcontext {
#ifdef __mips
	/*
	 * BEGIN REGION THAT MUST CORRESPOND WITH setjmp.h
	 * BEGIN REGION THAT MUST CORRESPOND WITH A jmp_buf
	 */
#endif /* __mips */
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore */
#ifdef __vax
	int	sc_sp;			/* sp to restore */
	int	sc_pc;			/* pc to retore */
	int	sc_ps;			/* psl to restore */
#endif /* __vax */
#ifdef __mips
	int	sc_pc;			/* pc at time of signal */
	/*
	 * General purpose registers
	 */
	int	sc_regs[32];	/* processor regs 0 to 31 */
	int	sc_mdlo;	/* mul/div low */
	int	sc_mdhi;	/* mul/div high */
	/*
	 * Floating point coprocessor state
	 */
	int	sc_ownedfp;	/* fp has been used */
	int	sc_fpregs[32];	/* fp regs 0 to 31 */
	int	sc_fpc_csr;	/* floating point control and status reg */
	int	sc_fpc_eir;	/* floating point exception instruction reg */
	/*
	 * END OF REGION THAT MUST AGREE WITH setjmp.h
	 * END OF jmp_buf REGION
	 */
	/*
	 * System coprocessor registers at time of signal
	 */
	int	sc_cause;	/* cp0 cause register */
	int	sc_badvaddr;	/* cp0 bad virtual address */
	int	sc_badpaddr;	/* cpu bd bad physical address */
#endif /* __mips */
};
#endif /* !defined(_POSIX_SOURCE) */
#endif	/* __SYSTEM_FIVE */

#if !defined(_POSIX_SOURCE)
#define	BADSIG		((void (*)())(-1))
#endif /* !defined(_POSIX_SOURCE) */

#define	SIG_ERR		((void (*)())(-1))
#define	SIG_DFL		((void (*)())( 0))
#define	SIG_IGN		((void (*)())( 1))


#ifdef KERNEL
#define	SIG_CATCH	((void (*)())( 2))
#define	SIG_HOLD	((void (*)())( 3))
#else /* KERNEL */

#ifdef __STDC__

#ifndef _PID_T_
#define _PID_T_
typedef int pid_t;
#endif /* _PID_T_ */
/*
 *  prototypes
 *
 */
extern void	(*signal(int __sig, void(*__func)(int)))(int);
int	raise( int __sig );
int 	kill( pid_t __pid, int __sig );
int 	sigemptyset( sigset_t *__set );
int 	sigfillset( sigset_t *__set );
int 	sigaddset( sigset_t *__set, int __signo );
int	sigdelset( sigset_t *__set, int __signo );
int	sigismember( const sigset_t *__set, int __signo );
int 	sigaction( int __sig, const struct sigaction *__act,
		struct sigaction *__oact );
int 	sigprocmask( int __how, const sigset_t *__set, sigset_t *__oset );
int	sigpending( sigset_t *__set );
int	sigsuspend( const sigset_t *__sigmask );

#else
extern void		(*signal())();
extern int		raise();
extern int		kill();
extern int		sigaction(), sigaddset(), sigdelset(), sigemptyset(), 				sigfillset(), sigismember(), sigpending(), 
			sigprocmask(), sigsuspend();
#endif /* __STDC__ */
#endif /* KERNEL */
#endif /* defined(__vax) || (defined(__mips) && defined(__LANGUAGE_C)) */

/*
 * Macro for converting signal number to a mask suitable for
 * sigblock().
 */


#if !defined(_POSIX_SOURCE)
#define sigmask(m)	(1 << ((m)-1))
#endif /* !defined(_POSIX_SOURCE) */
#endif /* _SIGNAL_H_ */
