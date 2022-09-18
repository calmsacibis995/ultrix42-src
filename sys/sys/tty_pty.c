#ifndef lint
static char *sccsid = "@(#)tty_pty.c	4.7      (ULTRIX)  4/4/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-1991 by			*
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
/*	tty_pty.c	6.3	83/10/01	*/

/*
 * Pseudo-teletype Driver
 * (Actually two drivers, requiring two entries in 'cdevsw')
 */
/*
 *	Modification History
 *
 * 27-Mar-91 - Randall Brown
 *	Fix to ptcwrite().  Check for overflow of the slave's input queues
 *	even if the slave is in raw mode or cbreak mode.
 *
 * 07-Mar-91 - Tim Burke
 *	Security related fix.  Do not allow an open on the master if the
 *	slave is opened in the TS_WOPEN state.
 *
 * 10-Oct-90 - Kuo H. Hsieh
 *	Checked state flag for TS_ISOPEN on master end to make sure
 *	there is no activity going on in the slave terminal.
 *
 * 09-Sept-90 - U. Sinkewicz
 *	X.29 changes.
 *
 * 06-Mar-90 - jaw
 *	add missing splx.
 *
 * 22-Aug-89 - Randall Brown
 * 
 *	In ptcselect(), recheck conditions after attaining the lock.
 *
 * 15-Aug-89 - Randall Brown
 *
 *	Changed all references of TCSADFLUSH to TCSAFLUSH 
 *
 * 24-Jul-89 - Randall Brown
 * 
 *	No longer get lk_rq lock in ptyselect().
 *
 * 21-Jul-89 - Randall Brown
 *
 *	Added check for ISTRIP before sending each char to ttyinput().
 *
 *  2-Sep-88 - Tim Burke
 *
 *	Return EINVAL instead of ENOTTY for POSIX programs on invalid ioctls.
 *
 * 22-Mar-88 - Tim Burke
 *	Initial version of SMP support.  Use locks on pty and tty.
 *
 *  1-Dec-87 - Tim Burke
 *
 *	Added support for both System V termio(7) and POSIX termios(7).  These
 *	changes also include support for 8-bit canonical processing.  Changes
 *	involve:
 *
 *	- Default settings on first open depend on mode of open.  For termio
 *	  opens the defaults are "RAW" style, while non-termio opens default
 *	  to the traditional "cooked" style.
 *	- The driver now represents its terminal attributes and special 
 *	  characters in the POSIX termios data structure.  This contrasts the
 *	  original approach of storing attributes and special chars in the
 *	  t_flags, ltchars and tchars.
 *	- New termio ioctls: TCSANOW, TCSADRAIN, TCSADFLUSH, TCSETA, TESETAW,
 *	  TCSETAF.	
 *	- Addition of LPASS8 to local mode word for 8-bit canonical support.
 *
 * 8-Aug-87      Tim Burke
 *
 *      Bug fix in ptcwrite() to have master look at its own state flag to
 *      see if non-blocking I/O is to be done, rather than look at the slave's
 *      state flag.
 *
 * 29-Mar-87 - Tim Burke
 *
 *	Added full TERMIO functionality to terminal subsystem.
 *	Changes are isolated to the ptsopen routine in reguards to setting up
 *	propper default settings.
 *
 * 8-19-86	Tim Burke
 *
 *	Addition of TIOCMASTER code.  This allows the master to have complete
 *	control of pseudo terminals.  The slave side will sleep until the 
 *	master side relinquishes control.  Changes suggested by Peter Harbo
 *	of the DECNET group.
 */
#include "pty.h"

#if NPTY > 0 || defined(BINARY)
#include "../data/tty_pty_data.c"

#ifdef vax
#include "../machine/mtpr.h"
#endif vax

#ifdef mips
#include "../machine/cpu.h"
#endif mips

#ifdef DEBUG
int ptydebug = 0;
#endif DEBUG

#define BUFSIZ 		100		/* Chunk size iomoved from user */
#define	PF_RCOLL	0x01
#define	PF_WCOLL	0x02
#define	PF_NBIO		0x04
#define	PF_PKT		0x08		/* packet mode */
#define	PF_STOPPED	0x10		/* user told stopped */
#define	PF_REMOTE	0x20		/* remote and flow controlled input */
#define	PF_NOSTOP	0x40
#define PF_MASTER	0x80		/* Master controls flags */

/*
 * Default settings for a backward compatible Berkeley open.
 */
#define LFLAG (ISIG|ICANON)
#define IFLAG (IXON|IXANY)
#define OFLAG (OPOST)
#define CFLAG 0

/*
 * Markers to indicate that a wakeup call should be done when it is convenient.
 * Wakeups should be done after locks are released, or prior to sleeping.
 */
#define WAKEUP_RAWQ	0x1	/* Wakeup is on address of raw queue */
#define WAKEUP_OUTQ	0x2	/* Wakeup is on address of output queue */

int ptc_poststart_wakeup();

/*
 * Initialize each lock to indicate that this driver is MP-safe.
 * Called once at boot time from init_main.c.
 */
ptyinit() 
{
	register int i;

	for(i=0; i<nNPTY; ++i ) {
		pt_tty[i].t_smp = (S_SMP|S_POST_WAKEUP|S_POST_START|S_PTY);
		pt_tty[i].t_poststart = ptc_poststart_wakeup;
		lockinit(&pt_tty[i].t_lk_tty, &lock_tty_d);
		lockinit(&pt_ioctl[i].pt_lk_pty_sel, &lock_pty_sel_d);
	}
}

/*ARGSUSED*/
ptsopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int saveipl;
	register int ret_val;

#ifdef DEBUG
	if(ptydebug) {
		printf("minor:%d  ",minor(dev));
		printf("NPTY: %d\n", nNPTY);
	}
#endif DEBUG
	if (minor(dev) >= nNPTY)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	TTY_LOCK(tp,saveipl);
	/*
	 * Look at the compatibility mode to specify correct default parameters
	 * and to insure only standard specified functionality.
	 */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);		/* Set up default chars */
		tp->t_flags = 0;
		tp->t_cflag = EXTB;
		tp->t_cflag_ext = EXTB;
		tp->t_iflag_ext = 0;
		tp->t_oflag_ext = 0;
		tp->t_lflag_ext = 0;
		/*
		 * Termio style opens go to TERMIODISC with a few different
		 * default special characters.
		 */
		if (flag & O_TERMIO) {
			tp->t_line = TERMIODISC;
			tp->t_cc[VERASE] = '#';
			tp->t_cc[VKILL] = '@';
			tp->t_cc[VINTR] = 0177;
 			tp->t_cc[VMIN] = 6;
 			tp->t_cc[VTIME] = 1;
			tp->t_iflag = 0;
			tp->t_oflag = 0;
			tp->t_lflag = 0;
			tp->t_cflag |= CFLAG;
		}
		/*
                 * Provide a backward compatible Berkeley
                 * environment.  
                 */
		else {
			tp->t_iflag = IFLAG;
			tp->t_oflag = OFLAG;
			tp->t_lflag = LFLAG;
			tp->t_cflag |= CFLAG;
		}
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0) {
		TTY_UNLOCK(tp,saveipl);
		return (EBUSY);
	}
	if (tp->t_oproc)			/* Ctrlr still around. */
		tp->t_state |= TS_CARR_ON;
	if (flag & (O_NDELAY|O_NONBLOCK)) 
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_rawq, TTIPRI);
		}
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
		tp->t_state |= TS_ONOCTTY;
        ret_val = (*linesw[tp->t_line].l_open)(dev, tp);
        TTY_UNLOCK(tp,saveipl);
        return (ret_val);
}

ptsclose(dev)
	dev_t dev;
{
	register struct tty *tp;
	register int saveipl;

	tp = &pt_tty[minor(dev)];
	TTY_LOCK(tp,saveipl);
	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);
	/* Remove termio flags that do not map */
	tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	tp->t_lflag &= ~TERMIO_ONLY_LFLAG;
	ptcwakeup(tp);
	TTY_UNLOCK(tp,saveipl);
}

ptsread(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	register int error = 0;
	int ret_val;
	int saveflag,saveipl;
	char c;
	struct proc *save_pt_selw;

again:
	if (pti->pt_flags & PF_REMOTE) {
		register struct proc *p = u.u_procp;
		/*
		 * Send a SIGTTIN to background processes which attempt to 
		 * read if they aren't blocking or ignoring SIGTTIN.
		 * The background process must have this tty as controlling tty.
		 */
		while (tp == u.u_procp->p_ttyp && p->p_pgrp != tp->t_pgrp) {
			if ((p->p_sigignore & sigmask(SIGTTIN)) ||
			    (p->p_sigmask & sigmask(SIGTTIN)) ||
			    p->p_vm&SVFORK)
				return (EIO);
			gsignal(p->p_pgrp, SIGTTIN);
			sleep((caddr_t)&lbolt, TTIPRI);
		}
		TTY_LOCK(tp,saveipl);
		/*
		 * No input is available.  Sleep waiting for input unless
		 * NON-BLOCKING I/O is being done.
		 */
		if (tp->t_rawq.c_cc == 0) {
			if (tp->t_state & TS_NBIO) {
				TTY_UNLOCK(tp,saveipl);
				return (EWOULDBLOCK);
			}
			TTY_SLEEP(tp,(caddr_t)&tp->t_rawq, TTIPRI);
			splx(saveipl);
			goto again;
		}
		/*
		 * Any "real" input from the master pty will have an extra "0"
		 * character delimeting it.  For this reason we check that
		 * more than one char is on the queue.  
		 */
		while (tp->t_rawq.c_cc > 1 && uio->uio_resid > 0){
			c = getc(&tp->t_rawq);
			TTY_UNLOCK(tp,saveipl);
			ret_val = ureadc(c, uio);
			TTY_LOCK(tp,saveipl);
			if ( ret_val < 0) {
				error = EFAULT;
				break;
			}
		}
		/*
		 * Throw away the delimeting "0" character.
		 */
		if (tp->t_rawq.c_cc == 1)
			(void) getc(&tp->t_rawq);
		/* 
		 * If there are still characters on the queue then the "0"
		 * char was not the only thing on the queue.  Return here
		 * because wakeups shouldn't be done until all chars are read
		 * out of the rawq.
		 */
		if (tp->t_rawq.c_cc) {
			TTY_UNLOCK(tp,saveipl);
			return (error);
		}
		TTY_UNLOCK(tp,saveipl);
	} else
		if (tp->t_oproc)
			error = (*linesw[tp->t_line].l_read)(tp, uio);

	if (pti->pt_selw) {
		saveipl = spltty();
		smp_lock(&pti->pt_lk_pty_sel, LK_RETRY);
		if (pti->pt_selw == 0) {
			smp_unlock(&pti->pt_lk_pty_sel);
			splx(saveipl);
		} else {
			/*
			 * The process (if any) specified by pt_selw will
			 * be placed on the run queue if it is doing a 
			 * selwait.
			 */
			save_pt_selw = pti->pt_selw;
			pti->pt_selw = 0;
			
			/*
			 * If PF_WCOLL is set a wakeup will be done on
			 * selwait.
			 */
			saveflag = pti->pt_sel_flags & PF_WCOLL;
			pti->pt_sel_flags &= ~PF_WCOLL;

			smp_unlock(&pti->pt_lk_pty_sel);
			splx(saveipl);
			selwakeup(save_pt_selw,saveflag);
		}
	} 
	/*
	 * Reader's half of the PF_REMOTE flow control scheme.  Wake up the
	 * master pty to indicate that the rawq is now empty and ready to 
	 * accept more characters.
	 */
	wakeup((caddr_t)&tp->t_rawq.c_cf);
	return (error);
}

/*
 * Write to pseudo-tty.
 * Wakeups of controlling tty will happen
 * indirectly, when tty driver calls ptsstart.
 */
ptswrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = &pt_tty[minor(dev)];
	if (tp->t_oproc == 0)
		return (EIO);
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * Start output on pseudo-tty.
 * Wake up process selecting or sleeping for input from controlling tty.
 */
ptsstart(tp)
	struct tty *tp;
{
	register struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];

	TTY_ASSERT(tp);
	if (tp->t_state & TS_TTSTOP)
		return(NO_POST_START);
	if (pti->pt_flags & PF_STOPPED) {
		pti->pt_flags &= ~PF_STOPPED;
		pti->pt_send = TIOCPKT_START;
	}
	return(DO_POST_START);
}

/*
 * Wakeup the process (pt_selr) waiting for input.
 * This does not necessarily mean that input is available to read.  It 
 * could also indicate that the line is being dropped, stopped or had its
 * attributes changed by an ioctl.
 */
ptcwakeup(tp)
	struct tty *tp;
{
	register struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];

	TTY_ASSERT(tp);
	smp_lock(&pti->pt_lk_pty_sel, LK_RETRY);
	if (pti->pt_selr) {
		/*
		 * Wakeup the process pt_selr now that input is available.
		 * If there is more than one process selecting on this
		 * channel (RCOLL is set) then wakeup all of them.
		 */
		selwakeup(pti->pt_selr, pti->pt_sel_flags & PF_RCOLL);
		pti->pt_selr = 0;
		pti->pt_sel_flags &= ~PF_RCOLL;
	}
	smp_unlock(&pti->pt_lk_pty_sel);
	wakeup((caddr_t)&tp->t_outq.c_cf);
}



/*
 * Wakeup the process (pt_selr) waiting for input.
 * This does not necessarily mean that input is available to read.  It 
 * could also indicate that the line is being dropped, stopped or had its
 * attributes changed by an ioctl.
 */
ptc_poststart_wakeup(tp, poststart_arg)
	struct tty *tp;
        int poststart_arg;
{
	register struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];
	register struct proc *p = 0;
	register int flags;

	TTY_ASSERT(tp);
	smp_lock(&pti->pt_lk_pty_sel, LK_RETRY);
	if (pti->pt_selr) {
		/*
		 * Wakeup the process pt_selr now that input is available.
		 * If there is more than one process selecting on this
		 * channel (RCOLL is set) then wakeup all of them.
		 */
	        p = pti->pt_selr;
		flags = pti->pt_sel_flags & PF_RCOLL;
		pti->pt_selr = 0;
		pti->pt_sel_flags &= ~PF_RCOLL;
	}
	smp_unlock(&pti->pt_lk_pty_sel);
	if (tp->t_smp & S_DO_UNLOCK) {
	    smp_unlock(&tp->t_lk_tty);
	}
	if (p) {
	    selwakeup(p, flags);
	}
	wakeup((caddr_t)&tp->t_outq.c_cf);
}

/*ARGSUSED*/
ptcopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register struct pt_ioctl *pti;
	register int saveipl;

#ifdef DEBUG
	if (ptydebug)
		printf("ptcopen: NPTY = %d\n", nNPTY);
#endif DEBUG
	if (minor(dev) >= nNPTY)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	/*
	 * This insures that only one open can be done on the controller side.
	 * There might be some activities on the slave side.  Wait until
	 * the t_state is marked zero in ttyclose.
 	 * Don't use this pty if someone else is presently has the slave side
	 * open waiting for the associated master open.
         */
	TTY_LOCK(tp,saveipl);
	if (tp->t_oproc || (tp->t_state & (TS_ISOPEN|TS_WOPEN))) {
                TTY_UNLOCK(tp,saveipl);
		return (EIO);
	}
	tp->t_oproc = ptsstart;
	pti = &pt_ioctl[minor(dev)];
	pti->pt_flags = 0;
	pti->pt_sel_flags = 0;
	pti->pt_send = 0;
	bzero(pti->x29_mdata, X29DSIZE);
	bzero(pti->x29_sdata, X29DSIZE);
	tp->t_state |= TS_CARR_ON;
        if (tp->t_state & TS_WOPEN) {
                TTY_UNLOCK(tp,saveipl);
                wakeup((caddr_t)&tp->t_rawq);
        }
		else TTY_UNLOCK(tp,saveipl);
	return (0);
}

ptcclose(dev)
	dev_t dev;
{
	register struct tty *tp;
	register int saveipl;

	tp = &pt_tty[minor(dev)];
	TTY_LOCK(tp,saveipl);
	if (tp->t_state & TS_ISOPEN)
		gsignal(tp->t_pgrp, SIGHUP);
	tp->t_state &= ~TS_CARR_ON;	/* virtual carrier gone */
	ttyflush(tp, FREAD|FWRITE);
	tp->t_oproc = 0;		/* mark closed */
	TTY_UNLOCK(tp,saveipl);
}

ptcread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	char ptc_rdbuf[BUFSIZ];
	struct pt_ioctl *pti;
	int error = 0;
	register int saveipl;
	register int save_pt_send;
	int saveflag, wakeup_outq = 0;
	struct proc *save_t_wsel = 0;

	if ((tp->t_state&(TS_CARR_ON|TS_ISOPEN)) == 0)
		return (EIO);
	
	pti = &pt_ioctl[minor(dev)];
	if (pti->pt_flags & PF_PKT) {
		if (pti->pt_send) {
		    TTY_LOCK(tp,saveipl);
		    if (pti->pt_send) {	/* make sure pt_send has not changed */
			save_pt_send = pti->pt_send;
			TTY_UNLOCK(tp,saveipl);
			error = ureadc(save_pt_send, uio);
			if (error)
			    return (error);
			pti->pt_send = 0;
			return (0);
		    }
		    TTY_UNLOCK(tp,saveipl);
		}
		error = ureadc(0, uio);
	}
	TTY_LOCK(tp,saveipl);
	while (tp->t_outq.c_cc == 0 || (tp->t_state&TS_TTSTOP)) {
		if (((tp->t_state&TS_CARR_ON)==0)||(pti->pt_flags&PF_NBIO)){
			TTY_UNLOCK(tp,saveipl);
			return (EWOULDBLOCK);
		}
		TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_outq.c_cf, TTIPRI);
	}
	while (uio->uio_resid > 0 && error == 0) {
	        int cc;

		cc = q_to_b(&tp->t_outq, ptc_rdbuf, MIN(uio->uio_resid,BUFSIZ));
		if (tp->t_outq.c_cc == 0) {
		    if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
			if (tp->t_state&TS_ASLEEP) {
			    tp->t_state &= ~TS_ASLEEP;
			    wakeup_outq = 1;
			}
			if (tp->t_wsel) {
			    save_t_wsel = tp->t_wsel;
			    saveflag = tp->t_state & TS_WCOLL;
			    tp->t_wsel = 0;
			    tp->t_state &= ~TS_WCOLL;
			}
			TTY_UNLOCK(tp, saveipl);
			if (wakeup_outq) {
			    wakeup((caddr_t)&tp->t_outq);
			}
			if (save_t_wsel) {
			    selwakeup(save_t_wsel,saveflag );
			}
		    } else {
			TTY_UNLOCK(tp, saveipl);
		    }
		    wakeup((caddr_t)&tp->t_outq.c_cc);
		    error = uiomove(ptc_rdbuf, cc, UIO_READ, uio);
		    return(error);
		}
		if(cc == 0)
			break;
		/* Release lock because uiomove could fault. */
		TTY_UNLOCK(tp,saveipl);
		error = uiomove(ptc_rdbuf, cc, UIO_READ, uio);
		TTY_LOCK(tp,saveipl);
	}
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup_outq = 1;
		}
		if (tp->t_wsel) {
			save_t_wsel = tp->t_wsel;
			saveflag = tp->t_state & TS_WCOLL;
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
		TTY_UNLOCK(tp,saveipl);
		if (wakeup_outq) {
		    wakeup((caddr_t)&tp->t_outq);
		}
		if (save_t_wsel) {
		    selwakeup(save_t_wsel,saveflag );
		}	
		wakeup((caddr_t)&tp->t_outq.c_cc);
		return(error);
	}
	TTY_UNLOCK(tp,saveipl);
	wakeup((caddr_t)&tp->t_outq.c_cc);
	return (error);
}

ptsstop(tp, flush)
	register struct tty *tp;
	int flush;
{
	struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];
	int saveipl;
	
	TTY_ASSERT(tp);

	/* note: FLUSHREAD and FLUSHWRITE already ok */
	if (flush == 0) {
		flush = TIOCPKT_STOP;
		pti->pt_flags |= PF_STOPPED;
	} else {
		pti->pt_flags &= ~PF_STOPPED;
	}
	pti->pt_send |= flush;
	ptcwakeup(tp);
}

ptcselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	struct proc *p;
	register int s;
	register int error=0;


	if ((tp->t_state&(TS_CARR_ON|TS_ISOPEN)) == 0)
		return (1);

	switch (rw) {

	case FREAD:
		/*
		 * Input is available.  Return data available status.
		 */
		if (tp->t_outq.c_cc && (tp->t_state&TS_TTSTOP) == 0) 
			error = 1;
		/*
		 * Sleep in selwait until input is present.
		 */
		else {
			TTY_LOCK(tp, s);
			smp_lock(&pti->pt_lk_pty_sel, LK_RETRY);

			/*
			 * Recheck conditions now that we have lock.
			 */
			if (tp->t_outq.c_cc && (tp->t_state&TS_TTSTOP)==0) {
				error = 1;
				smp_unlock(&pti->pt_lk_pty_sel);
				TTY_UNLOCK(tp, s);
				break;
			}
			/*
		 	 * Here there is already somebody else who is waiting on
		 	 * this channel.  RCOLL is set to do a collective wakeup
		 	 * of all processes sleeping on this channel. 
			 */
			if ((p = pti->pt_selr) && p->p_wchan == (caddr_t)&selwait)
				pti->pt_sel_flags |= PF_RCOLL;
			/*
		 	 * This (active) process is the only one doing a select 
		 	 * on this channel.  Don't set RCOLL to avoid the 
		 	 * the overhead of waking up all select sleepers.
			 * This process will be placed on the run queue
		 	 * without the overhead of waking up other processes.
			 */
			else
				pti->pt_selr = u.u_procp;

			smp_unlock(&pti->pt_lk_pty_sel);
			TTY_UNLOCK(tp, s);

		}
		break;

	case FWRITE:
		/*
		 * Device ready to accept output.
		 */
		if ((pti->pt_flags & PF_REMOTE) == 0 || tp->t_rawq.c_cc == 0)
			error = 1;
		/*
		 * Wait for output to drain.
		 */
		else {

			TTY_LOCK(tp, s);
			smp_lock(&pti->pt_lk_pty_sel, LK_RETRY);

			/*
			 * Recheck conditions now that we have lock.
			 */
			if ((pti->pt_flags & PF_REMOTE) == 0 || tp->t_rawq.c_cc == 0) {
				error = 1;
				smp_unlock(&pti->pt_lk_pty_sel);
				TTY_UNLOCK(tp, s);
				break;
			}
			/*
		 	 * Here there is already somebody else who is waiting on
		 	 * this channel.  WCOLL is set to do a collective wakeup
		 	 * of all processes sleeping on this channel. 
			 */
			if ((p = pti->pt_selw) && p->p_wchan == (caddr_t)&selwait)
				pti->pt_sel_flags |= PF_WCOLL;
			/*
		 	 * This (active) process is the only one doing a select 
		 	 * on this channel.  Don't set WCOLL to avoid the 
		 	 * the overhead of waking up all select sleepers.
			 * This process will be placed on the run queue
		 	 * without the overhead of waking up other processes.
			 */
			else
				pti->pt_selw = u.u_procp;

			smp_unlock(&pti->pt_lk_pty_sel);
			TTY_UNLOCK(tp, s);
		}
	}
	return (error);
}

ptcwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register char *cp, *ce;
	register int cc;
	register int post_wakeup = 0, post_ttwakeup = NO_TTWAKEUP;
	char locbuf[BUFSIZ];
	int cnt = 0;
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int error = 0;
	int saveipl;

	if ((tp->t_state&(TS_CARR_ON|TS_ISOPEN)) == 0)
		return (EIO);
	TTY_LOCK(tp,saveipl);
	do {
		register struct iovec *iov;

		if (uio->uio_iovcnt == 0)
			break;
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			/*
			 * PF_REMOTE is used for flow control by not putting
			 * any more characters onto the rawq until all of the
			 * previous ones have been read off by the slave tty.
			 */
 			while((pti->pt_flags&PF_REMOTE)&&tp->t_rawq.c_cc != 0) {
				/*
				 * Process pending wakeups prior to sleeping.
				 */
				if (post_wakeup) {
					wakeup((caddr_t)&tp->t_rawq);
					post_wakeup = 0;
				}
 				TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_rawq.c_cf, TTIPRI);
			}
 			if(pti->pt_flags&PF_REMOTE){
				/*
				 * When PF_REMOTE is set the input chars on the
				 * slave tty side do not go through normal 
				 * canonical processing.  Here a "0" will be
				 * the only character on the queue used to 
				 * represent end of file.
				 */
 				(void) putc(0, &tp->t_rawq);
				/*
				 * Avoid wakeups while tty lock is held.
				 */
				post_wakeup = WAKEUP_RAWQ;
 			}
			uio->uio_iovcnt--;	
			uio->uio_iov++;
			if (uio->uio_iovcnt < 0)
				panic("ptcwrite");
			continue;
		}
		cc = MIN(iov->iov_len, BUFSIZ);
		cp = locbuf;
		/*
		 * Release lock because uiomove could fault.
		 * Process pending wakeups after unlock just in case someone
		 * else slithers in.
		 */
		TTY_UNLOCK(tp,saveipl); 
		if (post_wakeup) {
			wakeup((caddr_t)&tp->t_rawq);
			post_wakeup = 0;
		}
		error = uiomove(cp, cc, UIO_WRITE, uio);
		TTY_LOCK(tp,saveipl);
		if (error)
			break;
		ce = cp + cc;
again:
		/*
		 * PF_REMOTE is used for flow control by not putting
		 * any more characters onto the rawq until all of the
		 * previous ones have been read off by the slave tty.
		 */
		if (pti->pt_flags & PF_REMOTE) {
			if (tp->t_rawq.c_cc) {
				if (pti->pt_flags & PF_NBIO) {
					iov->iov_base -= ce - cp;
					iov->iov_len += ce - cp;
					uio->uio_resid += ce - cp;
					uio->uio_offset -= ce - cp;
					TTY_UNLOCK(tp,saveipl);
					if (post_wakeup) 
						wakeup((caddr_t)&tp->t_rawq);
					return (EWOULDBLOCK);
				}
				/*
				 * Process pending wakeups prior to sleeping.
				 */
				if (post_wakeup) {
					wakeup((caddr_t)&tp->t_rawq);
					post_wakeup = 0;
				}
				TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_rawq.c_cf, TTOPRI);
				goto again;
			}
			(void) b_to_q(cp, cc, &tp->t_rawq);
			/*
			 * A "0" on the queue here is used to delimit the end
			 * of this input.  It is also needed due to the way
			 * a "0" is stuffed onto the queue to specify EOF.
			 * For this reason the slave tty reader will be
			 * expecting an extra character which will not be
			 * returned up to the reading process.
			 */
			(void) putc(0, &tp->t_rawq);
			TTY_UNLOCK(tp,saveipl);
			wakeup((caddr_t)&tp->t_rawq);
			return (0);
		}
		while (cp < ce) {
			if (((tp->t_rawq.c_cc + tp->t_canq.c_cc) >= TTYHOG - 2) &&
			    ((tp->t_canq.c_cc > 0) ||
			     (tp->t_iflag_ext&PCBREAK) ||
			     ((tp->t_lflag&ICANON) == 0))) {
				/*
				 * Avoid wakeups while tty lock is held.
				 */
				post_wakeup = WAKEUP_RAWQ;
				if (pti->pt_flags & PF_NBIO) {
					iov->iov_base -= ce - cp;
					iov->iov_len += ce - cp;
					uio->uio_resid += ce - cp;
					uio->uio_offset -= ce - cp;
					if (post_ttwakeup) {
					    ttwakeup(tp);	/* ttwakeup will unlock tty lock */
					    splx(saveipl);	/* but does not lower IPL	 */
					} else {
					    TTY_UNLOCK(tp,saveipl);
					}
					if (post_wakeup) 
						wakeup((caddr_t)&tp->t_rawq);
					if (cnt == 0) 
						return (EWOULDBLOCK);
					
					return (0);
				}
				if (post_ttwakeup) {
				    ttwakeup(tp);	/* ttwakeup will unlock tty lock */
				    splx(saveipl);	/* but does not lower IPL	 */
				} else {
				    TTY_UNLOCK(tp,saveipl);
				}
				/*
				 * Wait for something to be read (as opposed to
				 * flushing the rawq).
				 * Process pending wakeups prior to sleeping.
				 */
				if (post_wakeup) {
					wakeup((caddr_t)&tp->t_rawq);
					post_wakeup = 0;
				}
				TTY_SLEEP_RELOCK(tp, (caddr_t)&tp->t_rawq.c_cf, TTOPRI);
				goto again;
			}
			if (tp->t_iflag & ISTRIP) {
			    *cp &= 0177;
			}
			post_ttwakeup += (*linesw[tp->t_line].l_rint)(*cp++, tp);
			cnt++;
		}
	} while (uio->uio_resid);
	if (post_ttwakeup) {
	    ttwakeup(tp);	/* ttwakeup will unlock tty lock */
	    splx(saveipl);	/* but does not lower IPL	 */
	} else {
	    TTY_UNLOCK(tp,saveipl);
	}
	/*
	 * Service any pending wakeups prior to return.
	 */
	if (post_wakeup) 
		wakeup((caddr_t)&tp->t_rawq);
	return (error);
}

/*ARGSUSED*/
ptyioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int error=0;
	register int saveipl;

	/* IF CONTROLLER STTY THEN MUST FLUSH TO PREVENT A HANG ??? */
	if (cdevsw[major(dev)].d_open == ptcopen) {

		TTY_LOCK(tp,saveipl);
		switch (cmd) {

		case TIOCPKT:
			if (*(int *)data)
				pti->pt_flags |= PF_PKT;
			else
				pti->pt_flags &= ~PF_PKT;
			TTY_UNLOCK(tp,saveipl);
			return(0);
 
		case TIOCREMOTE:
			if (*(int *)data)
				pti->pt_flags |= PF_REMOTE;
			else
				pti->pt_flags &= ~PF_REMOTE;
			ttyflush(tp, FREAD|FWRITE);
			TTY_UNLOCK(tp,saveipl);
			return(0);

		case FIONBIO:
			if (*(int *)data)
				pti->pt_flags |= PF_NBIO;
			else
				pti->pt_flags &= ~PF_NBIO;
			TTY_UNLOCK(tp,saveipl);
			return(0);

		case TIOCSETP:			/* Berkeley */
		case TCSANOW:			/* POSIX */
		case TCSADRAIN:			/* POSIX */
		case TCSAFLUSH:			/* POSIX */
 		case TCSETA:			/* SVID */
 		case TCSETAW:			/* SVID */
 		case TCSETAF:			/* SVID */
			while (getc(&tp->t_outq) >= 0)
				;
			break;
		case TIOCMASTER:
			if (*(int *)data)
				pti->pt_flags |= PF_MASTER;
			else
				pti->pt_flags &= ~PF_MASTER;
			TTY_UNLOCK(tp,saveipl);
			return (0);
		}
		TTY_UNLOCK(tp,saveipl);
	} else if (cdevsw[major(dev)].d_open == ptsopen) {
		while (pti->pt_flags & PF_MASTER)		
			sleep((caddr_t)&lbolt,TTIPRI);
	}


	/*
	 * Add a group of extended IOCTL calls relating to X.29
	 * This group is far more general and allows the master and the 
	 * slave side to exchange control information
	 */

	if ((cmd == TIOCX29SET) || (cmd == TIOCX29GET))
	  {
	    register char *x29ptr = (char *)data;

	    /*
	     * Ensure exclusive access to the tty
	     */
	    TTY_LOCK(tp, saveipl);

	    /*
	     * split processing based on master or slave device
	     */
	    if (cdevsw[major(dev)].d_open == ptcopen)
	      {
		/*
		 * Master side operation
		 */

		switch (cmd)
		  {
		  case TIOCX29SET:
		    /*
		     * Send control data to the slave side using the slave
		     * side block. Usual checks for operation in progress
		     * etc
		     */
		    while (pti->x29_sdata[0])
		      {

			/*
			 * If master side in non-blocking state just get
			 * out and leave as-is, otherwise sleep until
			 * slave side reads the data
			 */
			if (pti->pt_flags & PF_NBIO)
			  {
			    TTY_UNLOCK(tp, saveipl);
			    return (EWOULDBLOCK);
			  }
			else
			  TTY_SLEEP_RELOCK(tp,(caddr_t)pti->x29_sdata,TTIPRI);
		      }

		    /*
		     * Once we have the lock etc, copy the data across and
		     * return
		     */
		    bcopy(x29ptr, pti->x29_sdata, X29DSIZE); 
		    TTY_UNLOCK(tp, saveipl);
		    wakeup((caddr_t)pti->x29_sdata);
		    break;


		  case TIOCX29GET:
		    /*
		     * Fetch control block, use the master side block. This
		     * operation will never block as the master side is 
		     * assumed to issue this in response to IOCTL notification
		     */

		    bcopy(pti->x29_mdata, x29ptr, X29DSIZE);

		    /*
		     * Reset control word to zero to indicate all is well then
		     * wake up any process that may have been waiting
		     */
		    pti->x29_mdata[0] = 0;
		    TTY_UNLOCK(tp, saveipl);
		    wakeup((caddr_t)pti->x29_mdata);
		    break;

		  }

		return(0);
	      }
	    else 
	      {
		
		/* 
		 * Slave side operation, basically the same as the master
		 * but uses different non-blocking conventions.
		 */
		switch(cmd)
		  {
		    
		  case TIOCX29SET:
		    /*
		     * Set values to be transferred to the master side process
		     */
		    while (pti->x29_mdata[0])
		      {
			/*
			 * If slave side in non-blocking state just get
			 * out and leave as-is, otherwise sleep until
			 * master side reads the data
			 */
			if (tp->t_state & TS_NBIO)
			  {
			    TTY_UNLOCK(tp, saveipl);
			    return (EWOULDBLOCK);
			  }
			else
			  TTY_SLEEP_RELOCK(tp,(caddr_t)pti->x29_mdata,TTIPRI);
		      }

		    /*
		     * Once we have the lock etc, copy the data across and
		     * return
		     */
		    bcopy(x29ptr, pti->x29_mdata, X29DSIZE); 
		    
		    /*
		     * Wake up the master side to be told that it has 
		     * ioctl data
		     */
		    pti->pt_send |= TIOCPKT_IOCTL;
		    ptcwakeup(tp);
		    TTY_UNLOCK(tp, saveipl);
		    break;

		    

		  case TIOCX29GET:
		    /*
		     * Fetch control block, use the slave side block. This
		     * operation may block if no data is available and 
		     * non-blocking I/O is not in effect.
		     */

		    while (pti->x29_sdata[0] == 0)
		      {
			/*
			 * If slave side in non-blocking state just get
			 * out and leave as-is, otherwise sleep until
			 * master side sets the data
			 */
			if (tp->t_state & TS_NBIO)
			  {
			    TTY_UNLOCK(tp, saveipl);
			    return (EWOULDBLOCK);
			  }
			else
			  TTY_SLEEP_RELOCK(tp,(caddr_t)pti->x29_sdata,TTIPRI);
		      }


		    /*
		     * Now have data and we can copy it across
		     */
		    bcopy(pti->x29_sdata, x29ptr, X29DSIZE);

		    /*
		     * Reset control word to zero to indicate all is well then
		     * wake up any process that may have been waiting to write
		     */
		    pti->x29_sdata[0] = 0;
		    TTY_UNLOCK(tp, saveipl);
		    wakeup((caddr_t)pti->x29_sdata);
		    break;

		  }

		return 0;
	      }
	  }
	else
	  error = ttioctl(tp, cmd, data, dev);
	TTY_LOCK(tp,saveipl);
	if (cmd & _IOC_IN) {
		pti->pt_send |= TIOCPKT_IOCTL;
		ptcwakeup(tp);
	}
	if (error < 0) {
		if (u.u_procp->p_progenv == A_POSIX) 
			error = EINVAL;
		else
			error = ENOTTY;
	}
	{ int stop = (tp->t_cc[VSTOP] == ('s'&037) &&
		      tp->t_cc[VSTART] == ('q'&037));
	if (pti->pt_flags & PF_NOSTOP) {
		if (stop) {
			pti->pt_send &= TIOCPKT_NOSTOP;
			pti->pt_send |= TIOCPKT_DOSTOP;
			pti->pt_flags &= ~PF_NOSTOP;
			ptcwakeup(tp);
		}
	} else {
		if (stop == 0) {
			pti->pt_send &= ~TIOCPKT_DOSTOP;
			pti->pt_send |= TIOCPKT_NOSTOP;
			pti->pt_flags |= PF_NOSTOP;
			ptcwakeup(tp);
		}
	}
	}
	TTY_UNLOCK(tp,saveipl);
	return (error);
}
#endif
