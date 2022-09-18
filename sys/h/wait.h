/*	@(#)wait.h	4.2	(ULTRIX)	9/4/90	*/
/* ------------------------------------------------------------------------
 * Modification History: /sys/h/wait.h
 *
 * 20-Dec-89  scott
 *	added #ifdef's for xopen, posix compliance
 *
 */

#ifndef _WAIT_H_
#define _WAIT_H_

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * This file holds definitions relevent to the wait system call.
 * Some of the options here are available only through the ``wait3''
 * entry point; the old entry point with one argument has more fixed
 * semantics, never returning status of unstopped children, hanging until
 * a process terminates if any are outstanding, and never returns
 * detailed information about process resource utilization (<vtimes.h>).
 */

/*
 * Structure of the information in the first word returned by both
 * wait and wait3.  If w_stopval==WSTOPPED, then the second structure
 * describes the information returned, else the first.  See WUNTRACED below.
 */

#ifdef __mips
/*
 * The structures returned by wait() are defined by bit field names
 * in 4.2BSD, although not used consistently. In system V, the definition
 * is by byte and bit positions (gak!). We try to satisfy both by
 * conditionaly compiling the 4.2 bit fields to line up with the
 * system V position scheme.
 */
#endif /* __mips */

#if !defined(_POSIX_SOURCE)
union wait {
#else
union __wait	{
#endif /* !defined(_POSIX_SOURCE) */
#ifdef __vax
	int	w_status;		/* used in syscall */
#endif /* __vax */
#ifdef __mips
	unsigned int	w_status;		/* used in syscall */
#endif /* __mips */
	/*
	 * Terminated process status.
	 */
	struct {
#ifdef __vax
		unsigned short	w_Termsig:7;	/* termination signal */
		unsigned short	w_Coredump:1;	/* core dump indicator */
		unsigned short	w_Retcode:8;	/* exit code if w_termsig==0 */
#endif /* __vax */
#ifdef __mips
#ifdef __MIPSEL
		unsigned int	w_Termsig:7;	/* termination signal */
		unsigned int	w_Coredump:1;	/* core dump indicator */
		unsigned int	w_Retcode:8;	/* exit code if w_termsig==0 */
		unsigned int	w_Filler:16;	/* pad to word boundary */
#endif /* __MIPSEL */
#ifdef __MIPSEB
		unsigned int	w_Filler:16;	/* pad to word boundary */
		unsigned int	w_Retcode:8;	/* exit code if w_termsig==0 */
		unsigned int	w_Coredump:1;	/* core dump indicator */
		unsigned int	w_Termsig:7;	/* termination signal */
#endif /* __MIPSEB */
#endif /* __mips */
	} w_T;
	/*
	 * Stopped process status.  Returned
	 * only for traced children unless requested
	 * with the WUNTRACED option bit.
	 */
	struct {
#ifdef __vax
		unsigned short	w_Stopval:8;	/* == W_STOPPED if stopped */
		unsigned short	w_Stopsig:8;	/* signal that stopped us */
#endif /* __vax */
#ifdef __mips
#ifdef __MIPSEL
		unsigned int	w_Stopval:8;	/* == W_STOPPED if stopped */
		unsigned int	w_Stopsig:8;	/* signal that stopped us */
		unsigned int	w_Filler:16;	/* pad to word boundary */
#endif /* __MIPSEL */
#ifdef __MIPSEB
		unsigned int	w_Filler:16;	/* pad to word boundary */
		unsigned int	w_Stopsig:8;	/* signal that stopped us */
		unsigned int	w_Stopval:8;	/* == W_STOPPED if stopped */
#endif /* __MIPSEB */
#endif /* __mips */
	} w_S;
};

#if !defined(_POSIX_SOURCE)
#define	w_termsig	w_T.w_Termsig
#define w_coredump	w_T.w_Coredump
#define w_retcode	w_T.w_Retcode
#define w_stopval	w_S.w_Stopval
#define w_stopsig	w_S.w_Stopsig
#define	WSTOPPED	0177	/* value of s.stopval if process is stopped */
#endif /* !defined(_POSIX_SOURCE) */

#ifdef  WSTOPPED
#define _WSTOPPED	WSTOPPED
#else
#define _WSTOPPED	0177
#endif

/*
 * Option bits for the second argument of wait3.  WNOHANG causes the
 * wait to not hang if there are no stopped or terminated processes, rather
 * returning an error indication in this case (pid==0).  WUNTRACED
 * indicates that the caller should receive status about untraced children
 * which stop due to signals.  If children are stopped and a wait without
 * this option is done, it is as though they were still running... nothing
 * about them is returned.
 */
#define WNOHANG		1	/* dont hang in wait */
#define WUNTRACED	2	/* tell about stopped, untraced children */

/*
 * Must cast as union wait * because POSIX defines the input to these macros
 * as int.
 */

#ifdef _POSIX_SOURCE
#define WIFSTOPPED(x)	(((union __wait *)&(x))->w_S.w_Stopval == _WSTOPPED)
#define WIFSIGNALED(x)	(((union __wait *)&(x))->w_S.w_Stopval != _WSTOPPED && ((union __wait *)&(x))->w_T.w_Termsig != 0)
#define WIFEXITED(x)	(((union __wait *)&(x))->w_S.w_Stopval != _WSTOPPED && ((union __wait *)&(x))->w_T.w_Termsig == 0)
#define	WEXITSTATUS(x)	(((union __wait *)&(x))->w_T.w_Retcode)
#define	WTERMSIG(x)	(((union __wait *)&(x))->w_T.w_Termsig)
#define	WSTOPSIG(x)	(((union __wait *)&(x))->w_S.w_Stopsig)
#endif /* _POSIX_SOURCE */

#if !defined(_POSIX_SOURCE)
#define WIFSTOPPED(x)	(((union wait *)&(x))->w_stopval == WSTOPPED)
#define WIFSIGNALED(x)	(((union wait *)&(x))->w_stopval != WSTOPPED && ((union wait *)&(x))->w_termsig != 0)
#define WIFEXITED(x)	(((union wait *)&(x))->w_stopval != WSTOPPED && ((union wait *)&(x))->w_termsig == 0)
#define	WEXITSTATUS(x)	(((union wait *)&(x))->w_retcode)
#define	WTERMSIG(x)	(((union wait *)&(x))->w_termsig)
#define	WSTOPSIG(x)	(((union wait *)&(x))->w_stopsig)
#endif /* !defined(_POSIX_SOURCE) */

#ifndef	KERNEL
#include	<sys/types.h>	/* Old programs don't do this */
pid_t wait(), waitpid();
#endif	/* KERNEL */

#endif /* _WAIT_H_ */
