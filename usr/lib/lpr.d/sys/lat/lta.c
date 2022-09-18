
#ifndef lint
static char *sccsid = "@(#)lta.c	6.3	ULTRIX	3/16/88";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 ,1987 by		*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *			Modification History				*
 *									*
 *	Larry Cohen  -	09/16/85					*
 *		protosw and ETHERTYPE changes caused by subnet routing	*
 *									*
 *	Peter Harbo -	04/15/86					*
 *		Changes to ltaopen() and ltaclose() to handle command	*
 *		messages of LAT 5.1.					*
 *									*
 *	Ricky Palmer -	07/11/86					*
 *		added adpt and nexus for DEVIOCGET request		*
 *									*
 *	Ricky Palmer -	08/27/86					*
 *		cleaned up devioctl bcopy's				*
 *									*
 *									*
 *  	Tim Burke - 12/1/87						*
 *		Support for System V termio(7) and POSIX termios(7).	*
 *		Also changes for 8-bit canonical processing.		*
 *		Default settings now depend on mode of open.		*
 *		Look for terminal attributes and special characters	*
 *		in the POSIX termios data structure.			*
 *									*
 ************************************************************************/


/*
 * lta.c
 */

#include "lta.h"
#if NLTA > 0 || defined(BINARY)
/*
 * LAT terminal driver (service class 1)
 */

#include "../data/lta_data.c"
#include "../h/termio.h"

#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/kernel.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_var.h"
#include "../lat/lat_protocol.h"
#include "../h/ioctl.h"

int ltastart();
extern struct sclass class1;
extern u_short reqid;
extern struct ecb statable[];
extern int hz;
extern int wakeup();

/*
 * Default settings for an ULTRIX open.
 */
#define ISPEED	B9600
#define IFLAGS	(EVENP|ODDP|ECHO)

/* termio flags will be set to these default values in non-termio mode to
 * provide a backward compatible ULTRIX environment.
 */
#define IFLAG (BRKINT|IGNPAR|ISTRIP|IXON|IXANY)
#define OFLAG (OPOST)
#define CFLAG (PARENB|CREAD|CS7)
#define LFLAG (ISIG|ICANON|ECHO)

/*
 * Open a LAT terminal line. If there is no current LAT session on this
 * terminal (no carrier present) wait until a session is established.
 */
ltaopen(dev, flag)
dev_t dev;
int flag;
{
    register struct tty *tp;
    register int unit = minor(dev);
    struct ecb *ecbp;

    int error = 0;
    int slept = 0;

    if (unit >= nLAT1)
	return (ENXIO);
    ecbp = &(statable[unit]);
    tp = &lata[unit];
    tp->t_oproc = ltastart;

    /*
     * Look at the compatibility mode to specify correct default parameters
     * and to insure only standard specified functionality.
     */
    if ((u.u_procp->p_progenv == A_SYSV) || (u.u_procp->p_progenv == A_POSIX)) {
	    flag |= O_TERMIO;
	    tp->t_line = TERMIODISC;
    }
    /*
     * If this is the first open, initialise tty state to default.
     */
    if ((tp->t_state & TS_ISOPEN) == 0)
    {
	ttychars(tp);
	tp->t_cflag = ISPEED;		/* ispeed */
	tp->t_cflag_ext = ISPEED;	/* ospeed */
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
		tp->t_flags = IFLAGS;  /* ULTRIX - moved */
		tp->t_iflag = IFLAG;
		tp->t_oflag = OFLAG;
		tp->t_lflag = LFLAG;
		tp->t_cflag |= CFLAG;
	}
    }
    else
	if (tp->t_state & TS_XCLUDE && u.u_uid != 0)
	    return (EBUSY);

    if (flag & (O_NDELAY|O_NONBLOCK))
	tp->t_state |= TS_ONDELAY;
    else
    {
	/*
	 * Wait for carrier (session establishment) before completing the
	 * open request.
	 *
	 * ltaopen() is called either by a port waiting for any incoming
	 * LAT connection or after an ioctl() call has sent a Command message
	 * to a terminal server.  If the cmd message pointer of the unit's
	 * statable entry is null, then no such ioctl() was issued.  Otherwise
	 * call will time out in approximately 60 seconds if no status or
	 * other messages are received.
	 */
	while ((tp->t_state & TS_CARR_ON) == 0)
	{
	    tp->t_state |= TS_WOPEN;
	    if (error = ecbp->ecb_error)
		break;
	    else
	    {
		if ( slept && ecbp->ecb_statrecd == 0)
		{
		    if (ecbp->ecb_cmdmsg &&
			ecbp->ecb_cmdmsg->m_type != MT_FREE)
			m_freem(ecbp->ecb_cmdmsg);
		    ecbp->ecb_reqid = ecbp->ecb_entryid = ecbp->ecb_inuse =
			ecbp->ecb_error = ecbp->ecb_statrecd = 0;
		    ecbp->ecb_cmdmsg = (struct mbuf *)0;
		    return(ETIMEDOUT);
		}
	    }
	    if (ecbp->ecb_statrecd)
		slept++;
	    ecbp->ecb_statrecd = 0;
	    if ( ecbp->ecb_cmdmsg ) /* Soliciting a connection */
	    {
		if (slept)
		    timeout(wakeup,(caddr_t)&tp->t_rawq,
			(int)LAT_STATTIMEOUT * hz);
		else
		    timeout(wakeup,(caddr_t)&tp->t_rawq,
			(int)LAT_CMDTIMEOUT * hz);
	    }
	    sleep((caddr_t)&tp->t_rawq, TTIPRI);
	    untimeout(wakeup,(caddr_t)&tp->t_rawq);
	    slept++;
	}
    }

    /* We got a start slot.  Toss the cmd message and set cmd msg ptr to 0.
     */
    if (ecbp->ecb_cmdmsg && tp->t_addr)
    {
	if (ecbp->ecb_cmdmsg->m_type != MT_FREE)
	   m_freem(ecbp->ecb_cmdmsg);
	ecbp->ecb_cmdmsg = (struct mbuf *)0;
    }
    if (error)
    {
	tp->t_state = 0;
	ecbp->ecb_reqid = ecbp->ecb_entryid = ecbp->ecb_inuse =
	    ecbp->ecb_error = ecbp->ecb_statrecd = 0;
	ecbp->ecb_cmdmsg = (struct mbuf *)0;

	return (error);
    }
    else
    {
	return ((*linesw[tp->t_line].l_open)(dev, tp));
    }
}

/*
 * Close a LAT terminal line.
 */
/*ARGSUSED*/
ltaclose(dev, flag)
dev_t dev;
int flag;
{

    register struct tty *tp = &lata[minor(dev)];
    register int unit = minor(dev);
    struct lat_cmd *cmdp;
    struct ecb *ecbp;
    struct lat_vc *vcp;
    u_char lastmsg;
    int latch = 0;
    int t_exit = 0;

    /*
     * If slot is active, output remaining characters.
     */
    while (tp->t_addr)
    {
	if (tp->t_outq.c_cc)
	{
	    ltastart(tp);
	    sleep((caddr_t)&lbolt,TTOPRI);
	}
	else break;
	if (++t_exit == 120)
	    break;
    }

    /*
     * If slot and vc are active, get number of last message which must
     * be acknowledged before slot can be shut down.  latch for unsigned
     * comparison.
     */
    if (tp->t_addr && 
	(struct lat_vc *)((struct lat_slot *)(tp->t_addr))->lsl_vc)
    {
	vcp = (struct lat_vc *)(((struct lat_slot *)tp->t_addr)->lsl_vc);
	if ((vcp->lvc_nxmt - 1) < vcp->lvc_hxmt)
	    latch++;
	lastmsg = vcp->lvc_nxmt  - 1;
    }
    t_exit = 0;

    /*
     * Wait for acknowledgement of last message.
     */
    while (tp->t_addr && 
	(struct lat_vc *)((struct lat_slot *)(tp->t_addr))->lsl_vc)
    {
	vcp = (struct lat_vc *)(((struct lat_slot *)(tp->t_addr))->lsl_vc);
	if (vcp->lvc_rrf == 0)
	    break;
	sleep((caddr_t)&lbolt,TTOPRI);
	if (latch)
	{
	    if (vcp->lvc_hxmt < lastmsg)
		latch = 0;
	}
	else
	    if (vcp->lvc_hxmt > lastmsg)
		break;
	if (++t_exit == 60)
	    break;
    }

    (*linesw[tp->t_line].l_close)(tp);
    if (tp->t_addr)
    {
	terminateslot(tp->t_addr);
	tp->t_addr = 0;
    }
    ttyclose(tp);

    ecbp = &(statable[unit]);

    /*
     * If the pointer to the command message is non-zero, a start slot
     * was never received.  If the entry ID is non-zero, a status message
     * without error was returned, indicating the entry was queued.  Now
     * we must send message to dequeue this entry.
     */
    if (ecbp->ecb_cmdmsg && ecbp->ecb_entryid && (tp->t_addr == 0) )
    {
	cmdp = mtod(ecbp->ecb_cmdmsg,struct lat_cmd *);
	/*
	 * Cancel entry, and add entryid to message
	 */
	cmdp->lcm_cmdtype = (u_char)3;
	*(u_short *)cmdp->lcm_entryid = ecbp->ecb_entryid;
	(*(ecbp->ecb_if)->if_output)(ecbp->ecb_if,ecbp->ecb_cmdmsg,
	    &(ecbp->ecb_addr));
    }
    ecbp->ecb_reqid = ecbp->ecb_entryid = ecbp->ecb_inuse = ecbp->ecb_error =
	 ecbp->ecb_statrecd = 0;
    ecbp->ecb_cmdmsg = (struct mbuf *)0;
    tp->t_addr = 0;
    /* Remove termio flags that do not map */
	tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	tp->t_lflag &= ~TERMIO_ONLY_LFLAG;

}

/*
 * Read from a LAT terminal.
 */
ltaread(dev, uio)
dev_t dev;
struct uio *uio;
{
    register struct tty *tp = &lata[minor(dev)];

    return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*
 * Write to a LAT terminal.
 */
ltawrite(dev, uio)
dev_t dev;
struct uio *uio;
{
    register struct tty *tp = &lata[minor(dev)];

    return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * Ioctl for LAT terminal.
 */
ltaioctl(dev, cmd, data, flag)
dev_t dev;
caddr_t data;
int cmd,flag;
{
    register struct tty *tp = &lata[minor(dev)];
    register int unit = minor(dev);
    struct lta_softc *sc = &lta_softc[unit];
    struct devget *devget;

    int error;

    error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
    if (error < 0)
    {
	error = ttioctl(tp, cmd, data, flag);
	if (error < 0)
	{
	   switch (cmd)
	   {
		  case DEVIOCGET:			/* device status */
			devget = (struct devget *)data;
			bzero(devget,sizeof(struct devget));
			devget->category = DEV_TERMINAL;
 /* "Bus" has no meaning as far as LAT goes, unless we should know what
     kind of "bus" the actual LAT hardware is. Bus should always refer
     to the first layer of connection between the device and interface */
			devget->bus = -1;
 /* I have used DEV_LAT to refer to all LAT hardware, we may want to revise
    this to be able to give the specific LAT hardware like, DECSERVER, PLUTO,
    etc. */
			bcopy(DEV_LAT,devget->interface,
			      strlen(DEV_LAT));
			bcopy(DEV_UNKNOWN,devget->device,
			      strlen(DEV_UNKNOWN));
			devget->adpt_num = -1;			/* n/a */
			devget->nexus_num = -1; 		/* n/a */
			devget->bus_num = -1;			/* n/a */
			devget->ctlr_num = unit/16;		/* which latbox  */
			devget->slave_num = unit;		/* which line	 */
			bcopy("lat",devget->dev_name,4);	/* Ultrix "lat"  */
			devget->unit_num = unit;		/* lat line?	 */
			devget->soft_count =
			      sc->sc_softcnt;			/* soft er. cnt. */
			devget->hard_count =
			      sc->sc_hardcnt;			/* hard er cnt.  */
			devget->stat = sc->sc_flags;		/* status	 */
			devget->category_stat =
			      sc->sc_category_flags;		/* cat. stat.	 */
			break;

		default:
		    return (ENOTTY);
	    }
	    error = 0;
	}
    }
    if (tp->t_state & TS_ISOPEN) {
 	switch(cmd) {
		case TCSANOW:			/* POSIX */
		case TCSADRAIN:			/* POSIX */
		case TCSADFLUSH:		/* POSIX */
 		case TCSETA:			/* SVID */
 		case TCSETAW:			/* SVID */
 		case TCSETAF:			/* SVID */
 		case TIOCSETP:			/* Berkeley */
 		case TIOCSETN:			/* Berkeley */
 		case TIOCSETC:			/* Berkeley */
			output_class1(tp, 1);
 			break;
 	}
    }
    return (error);
}

/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
ltastop(dev, flag)
dev_t dev;
int flag;
{
}

/*
 * Start (restart) output on the given LAT terminal.
 */
ltastart(tp)
register struct tty *tp;
{
    if ((tp->t_state & (TS_TIMEOUT|TS_TTSTOP)) == 0)
    {
	/*
	 * Check for sleepers and possibly wake them up.
	 */
	ltawakeup(tp);
	/*
	 * Now restart output unless the output queue is empty.
	 */
	if ((tp->t_state & TS_ISOPEN) && tp->t_outq.c_cc)
	{
	    output_class1(tp, 0);
	}
    }
}

ltawakeup(tp)
register struct tty *tp;
{
    /*
     * If there are sleepers, and output has drained below the low water
     * mark, wake up the sleepers.
     */
    if (tp->t_outq.c_cc <= TTLOWAT(tp))
    {
	if (tp->t_state & TS_ASLEEP)
	{
	    tp->t_state &= ~TS_ASLEEP;
	    wakeup((caddr_t)&tp->t_outq);
	}
	if (tp->t_wsel)
	{
	    selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
	    tp->t_wsel = 0;
	    tp->t_state &= ~TS_WCOLL;
	}
    }
}
#endif
