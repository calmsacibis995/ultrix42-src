/*	@(#)setjmp.h	4.2	(ULTRIX)	9/4/90	*/
#include <ansi_compat.h>
#ifndef	_JBLEN

#ifdef __vax
/************************************************************************
 *									*
 *			Copyright (c) 1985,1987,1988  by		*
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
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Add defintions for System V compatibility			*
 *									*
 ************************************************************************/


#define _JBLEN 10
typedef int jmp_buf[_JBLEN];

typedef int sigjmp_buf[_JBLEN];

#endif /* __vax */
#ifdef __mips
/*
 * jmp_buf offsets
 * This should really just be a struct sigcontext, but for historical
 * reasons ....
 * NOTE: THIS MUST MATCH the initial portion of struct sigcontext,
 *	sc_onsigstk, sc_sigmask, sc_pc, sc_regs, sc_mdlo, sc_mdhi,
 *	fpregs, and fpc_csr
 * must lie at offset equal to the corresponding entries in the jmp_buf
 * since longjmp performs a sigcleanup.
 * See libc routines setjmp/longjmp/sigvec, and kernel routines
 * sendsig/sigcleanup.
 */
#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
#define	JB_ONSIGSTK	0		/* onsigstack flag */
#define	JB_SIGMASK	1		/* signal mask */
#define	JB_PC		2		/* program counter */
#define	JB_REGS		3		/* registers */
#define	JB_ZERO		(JB_REGS+0)	/* register zero */
#define	JB_MAGIC	(JB_ZERO)	/* magic number saved at reg 0 */
#define	JB_AT		(JB_REGS+1)	/* AT */
#define	JB_V0		(JB_REGS+2)	/* function result regs */
#define	JB_V1		(JB_REGS+3)
#define	JB_A0		(JB_REGS+4)	/* argument regs */
#define	JB_A1		(JB_REGS+5)
#define	JB_A2		(JB_REGS+6)
#define	JB_A3		(JB_REGS+7)
#define	JB_T0		(JB_REGS+8)	/* caller saved regs */
#define	JB_T1		(JB_REGS+9)
#define	JB_T2		(JB_REGS+10)
#define	JB_T3		(JB_REGS+11)
#define	JB_T4		(JB_REGS+12)
#define	JB_T5		(JB_REGS+13)
#define	JB_T6		(JB_REGS+14)
#define	JB_T7		(JB_REGS+15)
#define	JB_S0		(JB_REGS+16)	/* callee saved regs */
#define	JB_S1		(JB_REGS+17)
#define	JB_S2		(JB_REGS+18)
#define	JB_S3		(JB_REGS+19)
#define	JB_S4		(JB_REGS+20)
#define	JB_S5		(JB_REGS+21)
#define	JB_S6		(JB_REGS+22)
#define	JB_S7		(JB_REGS+23)
#define	JB_T8		(JB_REGS+24)	/* temps */
#define	JB_T9		(JB_REGS+25)
#define	JB_K0		(JB_REGS+26)	/* kernel regs */
#define	JB_K1		(JB_REGS+27)
#define	JB_GP		(JB_REGS+28)	/* frame pointer */
#define	JB_SP		(JB_REGS+29)	/* stack pointer */
#define	JB_S8		(JB_REGS+30)	/* another callee saved */
#define	JB_RA		(JB_REGS+31)	/* return address */

#define	JB_FREGS	38		/* floating-point registers */
#define	JB_F0		(JB_FREGS+0)	/* function result regs */
#define	JB_F1		(JB_FREGS+1)
#define	JB_F2		(JB_FREGS+2)
#define	JB_F3		(JB_FREGS+3)
#define	JB_F4		(JB_FREGS+4)	/* caller save regs */
#define	JB_F5		(JB_FREGS+5)
#define	JB_F6		(JB_FREGS+6)
#define	JB_F7		(JB_FREGS+7)
#define	JB_F8		(JB_FREGS+8)
#define	JB_F9		(JB_FREGS+9)
#define	JB_F10		(JB_FREGS+10)
#define	JB_F11		(JB_FREGS+11)
#define	JB_F12		(JB_FREGS+12)	/* argument regs */
#define	JB_F13		(JB_FREGS+13)
#define	JB_F14		(JB_FREGS+14)
#define	JB_F15		(JB_FREGS+15)
#define	JB_F16		(JB_FREGS+16)	/* caller save regs */
#define	JB_F17		(JB_FREGS+17)
#define	JB_F18		(JB_FREGS+18)
#define	JB_F19		(JB_FREGS+19)
#define	JB_F20		(JB_FREGS+20)	/* callee save regs */
#define	JB_F21		(JB_FREGS+21)
#define	JB_F22		(JB_FREGS+22)
#define	JB_F23		(JB_FREGS+23)
#define	JB_F24		(JB_FREGS+24)
#define	JB_F25		(JB_FREGS+25)
#define	JB_F26		(JB_FREGS+26)
#define	JB_F27		(JB_FREGS+27)
#define	JB_F28		(JB_FREGS+28)
#define	JB_F29		(JB_FREGS+29)
#define	JB_F30		(JB_FREGS+30)
#define	JB_F31		(JB_FREGS+31)
#define JB_FPC_CSR	(JB_FREGS+32)	/* fp control and status register */

/*
 * These are not part of a jmpbuf, but are part of the sigcontext
 * and are referenced from the signal trampoline code in sigvec.s
 */
#define	SC_MDLO		(JB_REGS+32)
#define	SC_MDHI		(JB_REGS+33)

/*
 * offset for FLAGS
 *  LSB = savemask from sigsetjmp  (1 ->> restore signal mask)
 */

#define JB_FLAGS	72

#define	JBMAGIC		0xacedbade

/*
 * WARNING: a jmp_buf must be as large as a sigcontext since
 * longjmp uses one to perform a sigreturn
 */
#define	SIGCONTEXT_PAD	48
#define	NJBREGS		(JB_RA+1+SIGCONTEXT_PAD)
#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */
/* _JBLEN should be the same as NJBREGS */
#define _JBLEN	(34+1+48)

#ifdef __LANGUAGE_C
#ifndef LOCORE
typedef	int	jmp_buf[_JBLEN];

typedef int sigjmp_buf[_JBLEN];

#endif /* !LOCORE */
#endif /* __LANGUAGE_C */

#endif /* __mips */

#if defined(__mips) && defined(__LANGUAGE_C) || !defined(__mips)
#ifdef __STDC__
/*
 *  prototype
 *
 */

extern void	longjmp( jmp_buf __env, int __val );
extern int	setjmp( jmp_buf __env );
extern int	sigsetjmp(sigjmp_buf __env, int __savemask);
extern void 	siglongjmp(const sigjmp_buf __env, int __val);
#else

extern int setjmp();
extern void longjmp();
extern int sigsetjmp();
extern void siglongjmp();

#endif /* __STDC__ */
#endif /* defined(__mips) && defined(__LANGUAGE_C) || !defined(__mips) */

#endif /* _JBLEN */
