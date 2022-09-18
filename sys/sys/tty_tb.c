#ifndef lint
static char *sccsid = "@(#)tty_tb.c	4.2      (ULTRIX)  8/13/90";
#endif
 
/************************************************************************
 *									*
 *			Copyright (c) 1986-1990 by			*
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
 

#include "tb.h"
#if NTB > 0

static unsigned char my_device;  	/* make this reentrant later - jmg */
static short old_status;
static short old_x, old_y;

/*
 * Line discipline for RS232 tablets;
 * supplies binary coordinate data.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/workstation.h"
#include "../h/inputdevice.h"
#include "../h/wsdevice.h"
#include "../h/tablet.h"

#define EVENT_T_LEFT_BUTTON	0x01
#define EVENT_T_FRONT_BUTTON	0x02
#define EVENT_T_RIGHT_BUTTON	0x03
#define EVENT_T_BACK_BUTTON	0x04

/* puck buttons */

#define T_LEFT_BUTTON		0x02
#define T_FRONT_BUTTON		0x04
#define T_RIGHT_BUTTON		0x08
#define T_BACK_BUTTON		0x10
#define T_PROXIMITY		0x01

/*
 * Tablet configuration table.
 */
struct	tbconf {
	short	tbc_recsize;	/* input record size in bytes */
	short	tbc_uiosize;	/* size of data record returned user */
	int	tbc_sync;	/* mask for finding sync byte/bit */
	int	(*tbc_decode)();/* decoding routine */
	char	*tbc_run;	/* enter run mode sequence */
	char	*tbc_point;	/* enter point mode sequence */
	char	*tbc_stop;	/* stop sequence */
	char	*tbc_start;	/* start/restart sequence */
	int	tbc_flags;
#define	TBF_POL	0x1	/* polhemus hack */
};

static	int decdecode(), tbdecode(), gtcodecode(), poldecode();
static	int tblresdecode(), tbhresdecode();

struct	tbconf tbconf[TBTYPE] = {
{ 5, sizeof (struct decpos), 0200, decdecode, "R", "P" },
{ 5, sizeof (struct tbpos), 0200, tbdecode, "6", "4" },
{ 5, sizeof (struct tbpos), 0200, tbdecode, "\1CN", "\1RT", "\2", "\4" },
{ 8, sizeof (struct gtcopos), 0200, gtcodecode },
{17, sizeof (struct polpos), 0200, poldecode, 0, 0, "\21", "\5\22\2\23",
 TBF_POL },
{ 5, sizeof (struct tbpos), 0100, tblresdecode, "\1CN", "\1PT", "\2", "\4"},
{ 6, sizeof (struct tbpos), 0200, tbhresdecode, "\1CN", "\1PT", "\2", "\4"},
};

/*
 * Tablet state
 */
struct tb {
	int	tbflags;		/* mode & type bits */
#define	TBMAXREC	17	/* max input record size */
	char	cbuf[TBMAXREC];		/* input buffer */
	union {
		struct	decpos decpos;
		struct	tbpos tbpos;
		struct	gtcopos gtcopos;
		struct	polpos polpos;
	} rets;				/* processed state */
#define NTBS	16
} tb[NTBS];

ws_device tdev = {
	STABLET_DEVICE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

/*
 * Open as tablet discipline; called on discipline change.
 */
/*ARGSUSED*/
tbopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct tb *tbp;

cprintf("tablet disc opened!!! \n");
	my_device = ws_define_device(STABLET_DEVICE, &tdev,
		sizeof(tablet_event));

	if (tp->t_line == TABLDISC)
		return (ENODEV);

	ttywflush(tp);
	for (tbp = tb; tbp < &tb[NTBS]; tbp++)
		if (tbp->tbflags == 0)
			break;
	if (tbp >= &tb[NTBS])
		return (EBUSY);
	tbp->tbflags = TBTIGER|TBPOINT;		/* default */
	tp->t_cp = tbp->cbuf;
	tp->t_inbuf = 0;
	bzero((caddr_t)&tbp->rets, sizeof (tbp->rets));
	tp->T_LINEP = (caddr_t)tbp;
	tp->t_flags |= LITOUT;
	return (0);
}

/*
 * Line discipline change or last device close.
 */
tbclose(tp)
	register struct tty *tp;
{
	register int s;
	int modebits = TBPOINT|TBSTOP;

	tbioctl(tp, BIOSMODE, &modebits, 0);
	s = spl5();
	((struct tb *)tp->T_LINEP)->tbflags = 0;
	tp->t_cp = 0;
	tp->t_inbuf = 0;
	tp->t_rawq.c_cc = 0;		/* clear queues -- paranoid */
	tp->t_canq.c_cc = 0;
	tp->t_line = 0;			/* paranoid: avoid races */
	splx(s);
}

/*
 * Read from a tablet line.
 * Characters have been buffered in a buffer and decoded.
 */
tbread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	register struct tb *tbp = (struct tb *)tp->T_LINEP;
	register struct tbconf *tc = &tbconf[tbp->tbflags & TBTYPE];
	int ret;

	if ((tp->t_state&TS_CARR_ON) == 0)
		return (EIO);
	ret = uiomove(&tbp->rets, tc->tbc_uiosize, UIO_READ, uio);
	if (tc->tbc_flags&TBF_POL)
		tbp->rets.polpos.p_key = ' ';
	return (ret);
}

/*
 * Low level character input routine.
 * Stuff the character in the buffer, and decode
 * if all the chars are there.
 *
 * This routine could be expanded in-line in the receiver
 * interrupt routine to make it run as fast as possible.
 */
tbinput(c, tp)
	register int c;
	register struct tty *tp;
{
	register struct tb *tbp = (struct tb *)tp->T_LINEP;
	register struct tbconf *tc = &tbconf[tbp->tbflags & TBTYPE];
	if (tc->tbc_recsize == 0 || tc->tbc_decode == 0)	/* paranoid? */
		return;
	/*
	 * Locate sync bit/byte or reset input buffer.
	 */
	if (c&tc->tbc_sync || tp->t_inbuf == tc->tbc_recsize) {
		tp->t_cp = tbp->cbuf;
		tp->t_inbuf = 0;
	}
	*tp->t_cp++ = c /* &0177 */ ;
	/*
	 * Call decode routine only if a full record has been collected.
	 */
	if (++tp->t_inbuf == tc->tbc_recsize)
		(*tc->tbc_decode)(tbp->cbuf, &tbp->rets);
}

send_ws_event(tbpos)
	register struct decpos *tbpos;
{
	tablet_event tabev;
	short temp;
	extern ws_cursor_position *ws_get_cursor_position();
	ws_cursor_position *p = ws_get_cursor_position();

	tabev.ws.screen = p->screen;
	tabev.ws.device = my_device;
	tabev.ws.device_type = STABLET_DEVICE;
	
	/* process buttons */
	temp = old_status ^ tbpos->status;
	if(temp){
	    int j;

	    /* process proximity */
	    if(temp & T_PROXIMITY) {
		tabev.axis_data[0] = old_x = tbpos->xpos;
		tabev.axis_data[1] = old_y = tbpos->ypos;
		tabev.ws.e.pointer.x = p->x;
		tabev.ws.e.pointer.y = p->y;
		tabev.ws.e.pointer.buttons = tbpos->status;
		tabev.ws.type =
		    (tbpos->status & 1) ? PROXIMITY_OUT : PROXIMITY_IN;
		ws_enter_event(&tabev, sizeof(tablet_event));
	    }

	    /* define the changed button and if up or down */
	    for (j = 1; j <= 0x10; j <<= 1) {/* check each button */
		    if (!(j & temp))  /* did this button change? */
			continue;
		    switch (j) {
			case T_RIGHT_BUTTON:
			    tabev.ws.e.button.button = EVENT_T_RIGHT_BUTTON;
			    break;
			case T_FRONT_BUTTON:
			    tabev.ws.e.button.button = EVENT_T_FRONT_BUTTON;
			    break;
			case T_BACK_BUTTON:
			    tabev.ws.e.button.button = EVENT_T_BACK_BUTTON;
			    break;
			case T_LEFT_BUTTON:
			    tabev.ws.e.button.button = EVENT_T_LEFT_BUTTON;
			    break;
		    }
		    if (tbpos->status & j)
			tabev.ws.type = BUTTON_DOWN_TYPE;
		    else
			tabev.ws.type = BUTTON_UP_TYPE;
		    tabev.axis_data[0] = old_x = tbpos->xpos;
		    tabev.axis_data[1] = old_y = tbpos->ypos;
		    tabev.ws.e.button.x = p->x;
		    tabev.ws.e.button.y = p->y;
		    ws_enter_event(&tabev, sizeof(tablet_event));	    
	    }
            old_status = tbpos->status;
	}

	/* process motion */
	if(tbpos->xpos != old_x || tbpos->ypos != old_y) {
 	    tabev.ws.type = MOTION_TYPE;
	    tabev.axis_data[0] = old_x = tbpos->xpos;
	    tabev.axis_data[1] = old_y = tbpos->ypos;
	    tabev.ws.e.pointer.x = p->x;
	    tabev.ws.e.pointer.y = p->y;
	    tabev.ws.e.pointer.buttons = tbpos->status;
	    ws_enter_event(&tabev, sizeof(tablet_event));	    
	}

	/* wake up anyone waiting for graphics input on serial line */
	ws_wakeup_any_pending();

}

/*
 * Decode Digital 5 byte format
 */
static
decdecode(cp, tbpos)
	register char *cp;
	register struct decpos *tbpos;
{
cprintf("-> %x, %x, %x, %x, %x\n", (unsigned int) cp[0], (unsigned int) cp[1],
		(unsigned int) cp[2], (unsigned int) cp[3],
		(unsigned int) cp[4]);
	tbpos->status = *cp++ & 0x1F;
	tbpos->xpos = *cp++;
	tbpos->xpos |= *cp++ << 6;
	tbpos->ypos = *cp++;
	tbpos->ypos |= *cp << 6;
cprintf("decode xpos = %xx  %dd,   ypos = %xx  %dd\n", tbpos->xpos, 
	tbpos->xpos, tbpos->ypos,tbpos->ypos);
	/* XXX Do we want a sequence count? JH */
	send_ws_event(tbpos);
}

/*
 * Decode GTCO 8 byte format (high res, tilt, and pressure).
 */
static
gtcodecode(cp, tbpos)
	register char *cp;
	register struct gtcopos *tbpos;
{

	tbpos->pressure = *cp >> 2;
	tbpos->status = (tbpos->pressure > 16) | TBINPROX; /* half way down */
	tbpos->xpos = (*cp++ & 03) << 14;
	tbpos->xpos |= *cp++ << 7;
	tbpos->xpos |= *cp++;
	tbpos->ypos = (*cp++ & 03) << 14;
	tbpos->ypos |= *cp++ << 7;
	tbpos->ypos |= *cp++;
	tbpos->xtilt = *cp++;
	tbpos->ytilt = *cp++;
	tbpos->scount++;
}

/*
 * Decode old Hitachi 5 byte format (low res).
 */
static
tbdecode(cp, tbpos)
	register char *cp;
	register struct tbpos *tbpos;
{
	register char byte;

	byte = *cp++;
	tbpos->status = (byte&0100) ? TBINPROX : 0;
	byte &= ~0100;
	if (byte > 036)
		tbpos->status |= 1 << ((byte-040)/2);
	tbpos->xpos = *cp++ << 7;
	tbpos->xpos |= *cp++;
	if (tbpos->xpos < 256)			/* tablet wraps around at 256 */
		tbpos->status &= ~TBINPROX;	/* make it out of proximity */
	tbpos->ypos = *cp++ << 7;
	tbpos->ypos |= *cp++;
	tbpos->scount++;
}

/*
 * Decode new Hitach 5-byte format (low res).
 */
static
tblresdecode(cp, tbpos)
	register char *cp;
	register struct tbpos *tbpos;
{

	*cp &= ~0100;		/* mask sync bit */
	tbpos->status = (*cp++ >> 2) | TBINPROX;
	tbpos->xpos = *cp++;
	tbpos->xpos |= *cp++ << 6;
	tbpos->ypos = *cp++;
	tbpos->ypos |= *cp++ << 6;
	tbpos->scount++;
}

/*
 * Decode new Hitach 6-byte format (high res).
 */
static
tbhresdecode(cp, tbpos)
	register char *cp;
	register struct tbpos *tbpos;
{
	char byte;

	byte = *cp++;
	tbpos->xpos = (byte & 03) << 14;
	tbpos->xpos |= *cp++ << 7;
	tbpos->xpos |= *cp++;
	tbpos->ypos = *cp++ << 14;
	tbpos->ypos |= *cp++ << 7;
	tbpos->ypos |= *cp++;
	tbpos->status = (byte >> 2) | TBINPROX;
	tbpos->scount++;
}

/*
 * Polhemus decode.
 */
static
poldecode(cp, polpos)
	register char *cp;
	register struct polpos *polpos;
{

	polpos->p_x = cp[4] | cp[3]<<7 | (cp[9] & 0x03) << 14;
	polpos->p_y = cp[6] | cp[5]<<7 | (cp[9] & 0x0c) << 12;
	polpos->p_z = cp[8] | cp[7]<<7 | (cp[9] & 0x30) << 10;
	polpos->p_azi = cp[11] | cp[10]<<7 | (cp[16] & 0x03) << 14;
	polpos->p_pit = cp[13] | cp[12]<<7 | (cp[16] & 0x0c) << 12;
	polpos->p_rol = cp[15] | cp[14]<<7 | (cp[16] & 0x30) << 10;
	polpos->p_stat = cp[1] | cp[0]<<7;
	if (cp[2] != ' ')
		polpos->p_key = cp[2];
}

/*ARGSUSED*/
tbioctl(tp, cmd, data, flag)
	struct tty *tp;
	caddr_t data;
{
	register struct tb *tbp = (struct tb *)tp->T_LINEP;

	switch (cmd) {

	case BIOGMODE:
		*(int *)data = tbp->tbflags & TBMODE;
		break;

	case BIOSTYPE:
		if (tbconf[*(int *)data & TBTYPE].tbc_recsize == 0 ||
		    tbconf[*(int *)data & TBTYPE].tbc_decode == 0)
			return (EINVAL);
		tbp->tbflags &= ~TBTYPE;
		tbp->tbflags |= *(int *)data & TBTYPE;
		/* fall thru... to set mode bits */

	case BIOSMODE: {
		register struct tbconf *tc;

		tbp->tbflags &= ~TBMODE;
		tbp->tbflags |= *(int *)data & TBMODE;
		tc = &tbconf[tbp->tbflags & TBTYPE];
		if (tbp->tbflags&TBSTOP) {
			if (tc->tbc_stop)
				ttyout(tc->tbc_stop, tp);
		} else if (tc->tbc_start)
			ttyout(tc->tbc_start, tp);
		if (tbp->tbflags&TBPOINT) {
			if (tc->tbc_point)
				ttyout(tc->tbc_point, tp);
		} else if (tc->tbc_run)
			ttyout(tc->tbc_run, tp);
		ttstart(tp);
		break;
	}

	case BIOGTYPE:
		*(int *)data = tbp->tbflags & TBTYPE;
		break;

	case TIOCSETD:
	case TIOCGETD:
	case TIOCGETP:
	case TIOCGETC:
		return (-1);		/* pass thru... */

	default:
		return (ENOTTY);
	}
	return (0);
}
#endif
