#ifndef lint
static char *sccsid = "@(#)tty_pcm.c	4.1      (ULTRIX)  8/13/90";
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
 
#include "pcm.h"
#if NPCM > 0

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
#include "../h/pcm.h"

static unsigned char button_device, dial_device;

ws_device buttondev = {
	BUTTONBOX_DEVICE,
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

ws_device dialdev = {
	KNOBBOX_DEVICE,
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

pcmopen(dev, tp)
dev_t dev;
register struct tty *tp;
{

	register int s, c;
	int i;
	int oldline;

	s = spltty();
	ttywflush(tp);

	oldline = tp->t_line;
	tp->t_line = PCMDISC;

	tp->t_flags |= LITOUT;
	tp->t_oflag_ext |= PLITOUT;

	/* initialize pcm and devices */
	if (putc(INIT_ALL, &tp->t_outq) < 0) {
	  cprintf("pcmopen:  putc INIT_ALL failed\n");
	  tp->t_line = oldline;
	  return -1;
	}

	i = ttstart(tp);

	/* we can probably get by w/o the while loop XXX */
	while (tp->t_rawq.c_cc <= 0) {
	  sleep((caddr_t) &tp->t_rawq, TTIPRI);
	}
	splx(s);

	c = getc(&tp->t_rawq);

	if (c != ACK) {
	  cprintf("pcmopen: INIT_ALL not acknowledged\n");
	}

	/* note:  the device is now enabled, and the line discipline set
	   to PCMDISC.  If any input events come, they get passed to
	   ws_enter_event() even though the event queue may not have
	   been initialized yet.  So ws_enter_event has to check that
	   the queue has been initialized before entering an event and
	   GET_AND_MAP_EVENT_QUEUE has to initialize the queue head
	   and tail pointers.
	 */

	/* query status and see which devices are present */
	if (putc(DEVICE_PRESENT, &tp->t_outq) < 0) {
	  cprintf("pcmopen:  putc DEVICE_PRESENT failed\n");
	  tp->t_line = oldline;
	  return -1;
	}

	ttstart(tp);

	s = spltty();
	while (tp->t_rawq.c_cc <= 0) {
	  sleep((caddr_t) &tp->t_rawq, TTIPRI);
	}
	splx(s);

	c = getc(&tp->t_rawq);

	if (c & 0x02)
	  button_device = ws_define_device(BUTTONBOX_DEVICE, &buttondev,
			sizeof(ws_event));

	if (c & 0x01)
	  dial_device = ws_define_device(KNOBBOX_DEVICE, &dialdev,
			sizeof(dial_event));

	return 0;
}

pcmclose(tp)
register struct tty *tp;
{

	register int s;

	/* XXX should disable devices or check w/ lemaire that hangup does */

	s = spl5();
	tp->t_cp = 0;
	tp->t_inbuf = 0;
	tp->t_rawq.c_cc = 0;		/* clear queues -- paranoid */
	tp->t_canq.c_cc = 0;
	tp->t_line = 0;			/* paranoid: avoid races */
	splx(s);
}

static struct dial_report {
	int bytecount;
	int dialnumber;
	int highbyte;
	int lowbyte;
} current_report = { 0, 0, 0, 0};

/*
 * if we are assembling a dial report, queue it
 * else see if we recognize it
 */
pcminput(c, tp)
	register int c;
	register struct tty *tp;
{

	struct dial_report *new_report = &current_report;

	/* implement flow control XXX
		say if rawq.c_cc > some number
		or if ws event queue full */

	if (new_report->bytecount > 0) {
	  switch (new_report->bytecount) {
	    case 1:
	      new_report->lowbyte = c;
	      ++new_report->bytecount;
	      break;
	    case 2:
	      new_report->highbyte = c;
	      ++new_report->bytecount;
	      break;
	    default:
	      new_report->bytecount = 0;
	      /* fall through */
	    case 0:
	      cprintf("pcminput: bad byte count %d\n", new_report->bytecount);
	      break;
	  }
	  if (new_report->bytecount == 3) {
	    send_dial_event(new_report);
	    new_report->bytecount = 0;
	  }
	} else if (c == ACK || c == NAK) {
	  if (putc(c, &tp->t_rawq) >= 0) {
	    wakeup((caddr_t) &tp->t_rawq);
	  } else
	    cprintf("pcminput: putc of ACK or NAK failed\n");
	} else if (c >= 0xC0 && c <= 0xFF) {
	  send_button_event(c);
	} else if (c >= 0x30 && c <= 0x37) {
	  new_report->bytecount = 1;
	  new_report->dialnumber = c - 0x30;
	} else if ((c & 0xF0) == 0x20) {
	  /* status byte */
	  if (putc(c, &tp->t_rawq) >= 0) {
	    wakeup((caddr_t) &tp->t_rawq);
	  } else
	    cprintf("pcminput: putc of status byte failed\n");
	} else {
	  /* discard it */
	  cprintf("pcminput: unknown character %x\n", c);
	}
}

send_dial_event(report)
register struct dial_report *report;
{

	dial_event d;
	extern ws_cursor_position *ws_get_cursor_position();
	ws_cursor_position *p = ws_get_cursor_position();

	d.w.screen = p->screen;
	d.w.device = dial_device;
	d.w.device_type = KNOBBOX_DEVICE;
	d.w.e.pointer.x = p->x;
	d.w.e.pointer.y = p->y;

	bzero(d.axis_data, sizeof d.axis_data);
	d.axis_data[report->dialnumber] =
				(report->highbyte << 8) | report->lowbyte;

	d.w.type = MOTION_TYPE;

	ws_enter_event(&d, sizeof(dial_event));

	/* wake up anyone waiting for graphics input on serial line */
	ws_wakeup_any_pending();

}

send_button_event(c)
register int c;
{

	ws_event w;
	extern ws_cursor_position *ws_get_cursor_position();
	ws_cursor_position *p = ws_get_cursor_position();

	w.screen = p->screen;
	w.device = button_device;
	w.device_type = BUTTONBOX_DEVICE;
	w.e.button.x = p->x;
	w.e.button.y = p->y;

	/* The button information encoding is convoluted. */
	/* We add one because X likes its buttons numbered from 1. */
	w.e.button.button = 24 - (c & 0x18) + (c & 0x07) + 1;
	if (c & 0x20)
	  w.type = BUTTON_UP_TYPE;
	else
	  w.type = BUTTON_DOWN_TYPE;

	ws_enter_event(&w, sizeof(ws_event));

	/* wake up anyone waiting for graphics input on serial line */
	ws_wakeup_any_pending();

}

pcmioctl(tp, cmd, data, flag)
struct tty *tp;
caddr_t data;
{

	int c;
	int s;

	switch (cmd) {
	  case PIOSETLED:
	    putc(LEDS,&tp->t_outq);
	    putc(data[0], &tp->t_outq);
	    putc(data[1], &tp->t_outq);
	    putc(data[2], &tp->t_outq);
	    putc(data[3], &tp->t_outq);

	    ttstart(tp);

	    /* wait for ack */
	    s = spltty();
	    while (tp->t_rawq.c_cc <= 0)
	      sleep((caddr_t) &tp->t_rawq, TTIPRI);
	    splx(s);
	    c = getc(&tp->t_rawq);

	    if (c == NAK)
	      return -1;
	    else if (c == ACK)
	      return 0;
	    else {
	      /* how did this happen */
	      cprintf("bad acknowledgement in PIOSETLED\n");
	      return -1;
	    }

	    break;
	  case PIOSETKNOB:
	    /* simply don't allow the user to do something stupid */
	    if ( ((int) data[0]) == 0 || ((int) data[1]) == 0 ) {
	      return -1;
	    }

	    putc(THRESHSMOOTH, &tp->t_outq);
	    putc(data[0], &tp->t_outq);
	    putc(data[1], &tp->t_outq);

	    ttstart(tp);

	    /* wait for ack */
	    s = spltty();
	    while (tp->t_rawq.c_cc <= 0)
	      sleep((caddr_t) &tp->t_rawq, TTIPRI);
	    splx(s);
	    c = getc(&tp->t_rawq);

	    if (c == NAK)
	      return -1;
	    else if (c == ACK)
	      return 0;
	    else {
	      /* how did this happen */
	      cprintf("bad acknowledgement in PIOSETLED\n");
	      return -1;
	    }

	    break;
	  default:
	    return -1;
	}
}

#endif
