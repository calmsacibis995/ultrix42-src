#ifndef	lint
static char *sccsid = "@(#)cons_sw.c	4.2      (ULTRIX)  8/9/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 * Modification History
 *
 * 29-Oct-89 - Randall Brown
 *      Added the call to cpu specific c_init() in cninit().
 *
 * 11-Jul-89 - Randall Brown
 *	Added the ttys entry to each switch entry.
 *
 * 20-May-89 - Randall Brown
 *
 *	created file.
 *
 */

#include "../machine/common/cpuconf.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/time.h"
#include "../h/errno.h"
#include "../h/systm.h"
#include "../h/types.h"
#include "../h/errlog.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/exec.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/tty.h"
#include "../machine/cons_sw.h"


#define DEFAULTSPEED B4800
#define IFLAGS	(EVENP|ECHO|XTABS|CRMOD)
#define IFLAG (BRKINT|IGNPAR|ISTRIP|IXON|IXANY|ICRNL)
#define OFLAG (OPOST|TAB3|ONLCR)
#define LFLAG (ISIG|ICANON|ECHO)
#define CFLAG (PARENB|CREAD|CS7|CLOCAL)


struct cons_sw *cons_swp; /* pointer to the current cons_sw entry */
int consDev = 0;        /* describes whether console is graphic device */

extern int ttselect();
extern int cpu;
extern struct cons_sw cons_sw[];

nocons()
{
	return(0);
}

/*
 *	This routine is called to configure the console.
 *	
 *	It loops through the entire cons_sw table until it finds
 * 	an entry whose ID matches the cpu ID.
 *
 *	If it reaches the end of the table, then something is wrong.
 */
cninit()
{
    int i = 0; /* Start at beginning of table */

    cons_swp = 0;	/* initialize cons_sw pointer */
    while (cons_sw[i].system_type != 0) { /* 0 == end of table */
	if (cpu == cons_sw[i].system_type) {
	    cons_swp = &cons_sw[i];
	    break;
	}
	i++;
    }
    if (cons_swp == 0) {
	panic("System does not have a console configured.");
    }
    cdevsw[0].d_ttys = cons_swp->ttys;
    (cons_swp->c_init)();		/* call device specific init routine */
}

cnopen(dev, flag)
    dev_t dev;
    int flag;
{
    return((cons_swp->c_open)(dev, flag));
}

cnclose(dev)
    dev_t dev;
{
    return((cons_swp->c_close)(dev));
}

cnread(dev, uio)
    dev_t dev;
    struct uio *uio;
{
    return((cons_swp->c_read)(dev, uio));
}

cnwrite(dev, uio)
    dev_t dev;
    struct uio *uio;
{
    return((cons_swp->c_write)(dev, uio));
}

cnioctl(dev, cmd, addr, flag)
    dev_t dev;
    unsigned int cmd;
    caddr_t addr;
    int flag;
{
    return((cons_swp->c_ioctl)(dev, cmd, addr, flag));
}

cnstop(tp, flag)
    struct tty *tp;
    int flag;
{
    return((cons_swp->c_stop)(tp, flag));
}

cnstart(tp)
    struct tty *tp;
{
    return((cons_swp->c_start)(tp));
}

cnselect(dev, rw)
    dev_t dev;
    int rw;
{
    return((cons_swp->c_select)(dev, rw));
}

cnputc(c)
    unsigned int c;
{
    return((cons_swp->c_putc)(c));
}

cngetc()
{
    return((cons_swp->c_getc)());
}


cnprobe(c)
    unsigned int c;
{
    return((cons_swp->c_probe)(c));
}

cnrint(c)	/* Console receive interrupt  */
int c;
{
    return((cons_swp->c_rint)(c));
}

cnxint(c)	/* Console transmit interrupt */
int c;
{
    return((cons_swp->c_xint)(c));
}


/*
 * These need to be moved into tty.c for all drivers to use
 */
/*
 * tty_def_open
 */
cons_def_open(tp, flag)
struct	tty *tp;
unsigned int	flag;
{

	/*
	 * Look at the compatibility mode to specify correct default parameters
	 * and to insure only standard specified functionality.
	 */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if (flag & O_NOCTTY) 
		tp->t_state |= TS_ONOCTTY;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		tp->t_cflag = tp->t_cflag_ext = DEFAULTSPEED;
		tp->t_iflag_ext = 0;
		tp->t_oflag_ext = 0;
		tp->t_lflag_ext = 0;
		/*
		 * Ultrix defaults to a "COOKED" mode on the first
		 * open, while termio defaults to a "RAW" style.
		 * Base this decision by a flag set in the termio
		 * emulation routine for open, or set by an explicit
		 * ioctl call.
		 */
		if ( flag & O_TERMIO ) {
			/* Provide a termio style environment.
			 * "RAW" style by default.
			 */
			tp->t_flags = RAW;   
			tp->t_iflag = 0;
			tp->t_oflag = 0;
			tp->t_cflag |= CS8|CREAD|HUPCL; 
			tp->t_lflag = 0;

			/*
			 * Change to System V line discipline.
			 */
			tp->t_line = TERMIODISC;
			/*
			 * The following three control chars have 
			 * different default values than ULTRIX.
			 */
 			tp->t_cc[VERASE] = '#';
 			tp->t_cc[VKILL] = '@';
 			tp->t_cc[VINTR] = 0177;
 			tp->t_cc[VMIN] = 6;
 			tp->t_cc[VTIME] = 1;
		} else {
			/* Provide a backward compatible ULTRIX 
			 * environment.  "COOKED" style.
			 */
			tp->t_flags = IFLAGS;
			tp->t_iflag = IFLAG;
			tp->t_oflag = OFLAG;
			tp->t_lflag = LFLAG;
			tp->t_cflag |= CFLAG;
		}
	}
}

/*
 * tty_def_close
 */
cons_def_close(tp)
struct	tty	*tp;
{
	/*
	 * Clear input speed so that the next open will have propper defaults.
	 */
	tp->t_cflag &= ~CBAUD;
	tp->t_cflag_ext &= ~CBAUD;
	/* Remove termio flags that do not map */
	tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	tp->t_lflag &= ~TERMIO_ONLY_LFLAG;

}
