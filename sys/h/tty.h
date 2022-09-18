/* @(#)tty.h	4.2	( ULTRIX )	9/4/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1983-1988 by			*
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

/*************************************************************************
 *			Modification History
 *
 *		April 4, 1985  -- Larry Cohen
 *	001	add TS_CLOSING state to signify that tty line is closing
 *		down and should not be opened until closed completely.
 *		Primarily to ensure DTR is kept down "long enough"
 *
 *		April 13, 1985 -- Larry Cohen
 *	002     add TS_INUSE to sigify that tty line is "in use" by
 *		another process.  Advisory in nature.  Always cleared
 *		when the process that set this flag exits or closes
 *		the file descriptor associated with the line.
 *
 *		10/1/85 -- Larry Cohen
 *	003	add t_winsiz to tty structure.
 *	
 *		1/22/85 -- Larry Cohen
 *		DEC standard 52 support.  MODEM definitions.
 *
 *		03/06/86 -- Miriam Amos
 *		Add t_iflag, t_oflag, t_cflag, and t_lflag for
 *		termio interface.
 *
 *		06/17/86 -- Tim Burke
 * 		Added two state definitions (TS_LRTO, TS_LTACT) which are used
 *		in termio min/time interaction to specify that a timeout is in
 *	    	progress and that the min/time has been examined.
 *
 *		08/14/86 -- Tim Burke
 *		Added one #define for VAXstar SLU driver dec standard 52.
 *
 *		08/26/86 -- Tim Burke
 *		Changed name of #define added on 8/14/86.
 *
 *		05/25/87 -- Tim Burke
 *		Added termio to list of include files
 *
 *		07/28/87 -- Tim Burke
 *		Add some defines from Andy Gadsby which are used for 8-bit
 *		processing in the terminal driver.
 *
 *		12/01/87 -- Tim Burke
 *		Modified tty structure to allow terminal attributes to be
 *		stored in the POSIX termios data structure instead of t_flags,
 *		tchars, and ltchars.
 *		Defined 1 new terminal state.
 *		TS_ISUSP	- input is suspended.
 *
 *		12/12/87 -- Tim Burke
 *		Added t_language field to provide a hook for local language
 *		extensions such as Japanese tty.  The intent is that each
 *		language will add their own local language structure to a union
 *		of languages.  For example it may look like:
 *
 *			union local_ext {
 *				struct jtty jtp;	-- Japanese --
 *				struct gtty gtp;	-- German --
 *				struct ftty ftp;	-- French --
 *			};
 *
 *		02/08/88 -- Tim Burke
 *		Added macros used for ease of locking tty data structures
 *		in SMP enviornment.  Added t_smp and t_lk_tty fields to the
 *		tty structure.
 *
 *		03/10/88 -- Tim Burke
 *		Added a new state TS_OABORT.  This state indicates that output
 *		has been suspended after seeing a stop character.  This state
 *		will notify the start routine that it only has to continue.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../h/ttychars.h"
#include "../h/ttydev.h"
#include "../h/termios.h"
#else
#include <ansi_compat.h>
#include <sys/ttychars.h>
#include <sys/ttydev.h>
#include <sys/termios.h>
#include <sys/smp_lock.h>
#endif
#ifndef _TTY_
#define _TTY_
#endif /* _TTY_ */

/*
 * A clist structure is the head of a linked list queue
 * of characters.  The characters are stored in blocks
 * containing a link and CBSIZE (param.h) characters. 
 * The routines in tty_subr.c manipulate these structures.
 */
struct clist {
	int	c_cc;		/* character count */
	char	*c_cf;		/* pointer to first char */
	char	*c_cl;		/* pointer to last char */
};

/*
 * Per-tty structure.
 *
 * Should be split in two, into device and tty drivers.
 * Glue could be masks of what to echo and circular buffer
 * (low, high, timeout).
 */
struct tty {
	union {
		struct {
			struct	clist T_rawq;
			struct	clist T_canq;
		} t_t;
		struct {
			struct	buf *T_bufp;
			char	*T_cp;
			int	T_inbuf;
			int	T_rec;
		} t_n;
	} t_nu;
	struct {
		struct	buf *H_bufp;	/* used in tty_bk, tty_hc, tty_tb */
		char	*H_in;
		char	*H_out;
		char 	*H_base;
		char	*H_top;
		int	H_inbuf;
		int	H_read_cnt;
		} t_h;
	struct	clist t_outq;		/* device */
	int	(*t_oproc)();		/* device */
	int	(*t_poststart)();	/* routine to call after oproc to */
					/* do wakeups  */
	int	(*t_baudrate)();	/* routine to call to determine */
	                                /* if baudate is valid */
	struct	proc *t_rsel;		/* tty */
	struct	proc *t_wsel;
	caddr_t	T_LINEP;		/* used only in tty_tb.c */
	caddr_t	t_addr;			/* ??? */
	dev_t	t_dev;			/* device */
	unsigned int	t_flags;		/* some of both */
	unsigned short	t_iflag;	/* termio input modes */
	unsigned short	t_iflag_ext;	/* POSIX extensions to iflag */
	unsigned short	t_oflag;	/* termio output modes */
	unsigned short	t_oflag_ext;	/* POSIX extensions to oflag */
	unsigned short	t_cflag;	/* termio control modes */
	unsigned short	t_cflag_ext;	/* POSIX extensions to cflag */
	unsigned short	t_lflag;	/* termio line discipline modes */
	unsigned short	t_lflag_ext;	/* POSIX extensions to lflag */
	unsigned char	t_cc[NCCS];	/* termio control chars */
	unsigned int	t_state;	/* some of both */
	short	t_pgrp;			/* controling process number */
	char	t_delct;		/* tty */
	char	t_line;			/* line discipline */
	char	t_col;			/* tty */
	char	t_rocount, t_rocol;	/* tty */
	struct	winsize t_winsize;	/* window size */
	union	local_ext *t_language;	/* For local language extensions */
	int 	t_smp;			/* set if MP safe driver. */
	struct  lock_t  t_lk_tty;       /* smp lock */
	pid_t	t_sid;			/* POSIX session id */
	unsigned short	t_tpath;	/* Trusted path flags */
};


#define	t_rawq	t_nu.t_t.T_rawq		/* raw characters or partial line */
#define	t_canq	t_nu.t_t.T_canq		/* raw characters or partial line */

#define	t_bufp	t_nu.t_n.T_bufp		/* buffer allocated to protocol */
#define	t_cp	t_nu.t_n.T_cp		/* pointer into the ripped off buffer */
#define	t_inbuf	t_nu.t_n.T_inbuf	/* number chars in the buffer */
#define	t_rec	t_nu.t_n.T_rec		/* have a complete record */
#define	h_bufp	t_h.H_bufp		/* buffer allocated to protocol */
#define	h_in	t_h.H_in		/* next input position in buffer */
#define	h_out	t_h.H_out		/* next output position in buffer */
#define	h_base	t_h.H_base		/* pointer to base of buffer */
#define	h_top	t_h.H_top		/* pointer to top of buffer */
#define	h_inbuf	t_h.H_inbuf	/* number chars in the buffer */
#define	h_read	t_h.H_read_cnt	/* number chars to read in */

#define MODEM_CD   0x01
#define MODEM_DSR  0x02
#define MODEM_CTS  0x04
#define MODEM_DSR_START  0x08
#define MODEM_BADCALL 0x10

#define	TTIPRI	28
#define	TTOPRI	29

/* limits */
#define	NSPEEDS	16
#define	TTMASK	15
#define	OBUFSIZ	100
#define	TTYHOG	255
#ifdef KERNEL
short	tthiwat[NSPEEDS], ttlowat[NSPEEDS];
#define	TTHIWAT(tp)	tthiwat[((tp)->t_cflag_ext&CBAUD)&TTMASK]
#define	TTLOWAT(tp)	ttlowat[((tp)->t_cflag_ext&CBAUD)&TTMASK]
extern	struct ttychars ttydefaults;
#endif

/* smp state bits */
#define	S_SMP		0x1		/* driver is mp-safe */
#define	S_POST_WAKEUP	0x10		/* driver uses post-wakeups */
#define S_POST_START	0x20		/* driver has post_start routine */
#define S_DO_UNLOCK	0x40		/* unlock tty lock in post_start */
#define S_PTY		0x80		/* use pty specific locking */

/* deferred wakeup defines */
#define	NO_POST_START  	0x0		/* post start is not required */
#define DO_POST_START	0x1		/* post start is required */
#define NO_TTWAKEUP	0x0		/* ttwakeup does not need to be called */
#define DO_TTWAKEUP	0x1		/* ttwakeup does need to be called */

/* internal state bits */
#define	TS_TIMEOUT	0x000001	/* delay timeout in progress */
#define	TS_WOPEN	0x000002	/* waiting for open to complete */
#define	TS_ISOPEN	0x000004	/* device is open */
#define	TS_FLUSH	0x000008	/* outq has been flushed during DMA */
#define	TS_CARR_ON	0x000010	/* software copy of carrier-present */
#define	TS_BUSY		0x000020	/* output in progress */
#define	TS_ASLEEP	0x000040	/* wakeup when output done */
#define	TS_XCLUDE	0x000080	/* exclusive-use flag against open */
#define	TS_TTSTOP	0x000100	/* output stopped by ctl-s */
#define	TS_TBLOCK	0x000200	/* tandem queue blocked */
#define	TS_RCOLL	0x000400	/* collision in read select */
#define	TS_WCOLL	0x000800	/* collision in write select */
#define	TS_NBIO		0x001000	/* tty in non-blocking mode */
#define	TS_ASYNC	0x002000	/* tty in async i/o mode */
#define	TS_ONDELAY	0x004000	/* device is open; software copy of 
					 * carrier is not present */
/* state for intra-line fancy editing work */
#define	TS_BKSL		0x008000	/* state for lowercase \ work */
#define	TS_QUOT		0x010000	/* last character input was \ */
#define	TS_ERASE	0x020000	/* within a \.../ for PRTRUB */
#define	TS_LNCH		0x040000	/* next character is literal */
#define	TS_TYPEN	0x080000	/* retyping suspended input (PENDIN) */
#define	TS_CNTTB	0x100000	/* counting tab width; leave FLUSHO alone */


#define	TS_IGNCAR	0x200000	/* ignore software copy of carrier */
#define	TS_CLOSING	0x400000	/* closing down line */
#define TS_INUSE	0x800000	/* line is in use */
#define TS_LRTO		0x1000000	/* Raw timeout - used with min/time */
#define TS_LTACT	0x2000000	/* Timeout active, used with min/time */
#define TS_ISUSP	0x4000000	/* Suspend input. */
#define TS_OABORT	0x8000000      /* Output suspended on stop char. */
#define TS_ONOCTTY	0x10000000      /* Don't get controlling tty on open */
#define TS_NEED_PARAM   0x20000000      /* param() routine needs to be called */


#define	TS_LOCAL	(TS_BKSL|TS_QUOT|TS_ERASE|TS_LNCH|TS_TYPEN|TS_CNTTB)


/* define trusted path flags */
#define	TP_DOSAK	0x00000001	/* Process secure attention key */
#define	TP_DOLOGIN	0x00000002	/* Trusted login */
#define	TP_DOSHELL	0x00000004	/* Trusted shell */


/* define partab character types */
#define	ORDINARY	0
#define	CONTROL		1
#define	BACKSPACE	2
#define	NEWLINE		3
#define	TAB		4
#define	VTAB		5
#define	RETURN		6
#define	FORM_FEED	7

#ifdef KERNEL
/* define some portability values for the clist functions */
#define DELAY_FLAG	0400		/* indicates a delay character */
#define QUOTE_FLAG	01000		/* character has been quoted */
#define CHAR_MASK	0377		/* to get real character */
#define DEL_CHAR	0177		/* the delete character */

/*
 * SMP locking macros on tty structures.
 */
/*
 * Macro:          TTY_ASSERT(tp)
 *
 * Description:
 * If the driver is symmetric, check that
 * the TTY lock in the structure pointed
 * to by the parameter tp is held.  If
 * the driver is not symmetric, check
 * that the IPL is 15 or above.
 * This is a debugging macro mainly.  Note that smp_owner will only
 * function when smp_debug is set.  In an effort to avoid an unwanted
 * calls we test smp_debug first.
 */
#ifdef __vax
#define TTY_ASSERT(tp)	\
	{ \
	if(smp_debug) {\
		if (tp->t_smp) { \
			if ((smp_owner(&tp->t_lk_tty)) == LK_FALSE)\
				 panic("tty struct not locked"); \
		} else {\
		if (mfpr(IPL) < 0x14 ) panic("wrong ipl ");\
	} } }
#endif /* __vax */

#ifdef __mips
#define TTY_ASSERT(tp)	\
	{ \
	if(smp_debug) {\
		if (tp->t_smp) { \
			if ((smp_owner(&tp->t_lk_tty)) == LK_FALSE)\
				 panic("tty struct not locked"); \
		} else {\
		if (whatspl(getspl()) < SPLTTY) panic("wrong ipl ");\
	} } }
#endif /* __mips */

/*
 * Macro:  TTY_LOCK(tp,saveipl)
 *
 * Description:
 * Raise ipl to level 15 and save off old
 * value in "saveipl".  If the driver is
 * symmetric, lock the tty structure pointed
 * to by "tp".
 */
#define TTY_LOCK(tp,saveipl) {	\
	if (tp->t_smp) { \
		saveipl = spltty();\
		smp_lock(&tp->t_lk_tty, LK_RETRY);\
	} else saveipl = spltty();\
	}

/*
 * Macro:  TTY_UNLOCK
 *
 * Description:
 * If the driver is symmetric, unlock tty
 * structure pointed to by "tp".  Restore
 * IPL to "saveipl".
 */
#define TTY_UNLOCK(tp,saveipl) {\
	if (tp->t_smp){ \
		smp_unlock(&tp->t_lk_tty);\
	} \
	splx(saveipl);\
	}

/*
 * Macro:  TTY_SLEEP_RELOCK(tp,chan,pri)
 *
 * Description:
 * This is the same as "TTY_SLEEP" except when
 * the process is woken-up, the IPL is raised
 * to 15 and if symmetric, the tty structure
 * pointed to by "tp" is locked.
 */
#define TTY_SLEEP_RELOCK(tp,chan,pri) {\
	if (tp->t_smp) { \
		sleep_unlock(chan,pri,&tp->t_lk_tty);\
		(void) spltty();\
		smp_lock(&tp->t_lk_tty,LK_RETRY);\
	} else {\
		sleep(chan,pri);\
		(void) spltty();\
	}\
	}

/*
 * Macro:  TTY_SLEEP(tp,chan,pri)
 *
 * Description:
 * Put the current process onto the sleep queue
 * waiting on event "chan" at priority "pri".
 * If  the driver is symmetric, unlock the tty
 * structure pointed to by "tp".
 */
#define TTY_SLEEP(tp,chan,pri) {\
	if (tp->t_smp) { \
		sleep_unlock(chan,pri,&tp->t_lk_tty);\
	} else {\
		sleep(chan,pri);\
	}\
	}
#endif /* KERNEL */
