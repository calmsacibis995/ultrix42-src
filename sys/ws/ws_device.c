#ifndef lint
static char *sccsid = "@(#)ws_device.c	4.6      (ULTRIX)  3/8/91";
#endif lint

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 * Modification History
 *
 * 08-Mar-91	Sam Hsu
 *	Fix 1 tablet bug related to 1 of the 2 mouse bugs below.
 *
 * 27-Feb-91	Joel Gringorten
 * 	Fix 2 mouse bugs.
 *
 * 06-Dec-90	Randall Brown
 *	On wsclose(), clear all screens. 
 *
 * 16-Jul-90	Randall Brown
 *	Fixed bug in keyboard interrupt routine that was hardwired to dc device.
 *
 ************************************************************************/

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/smp_lock.h"
#include "../h/tty.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/exec.h"
#include "../h/kmalloc.h"
#include "../h/file.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/conf.h"
#include "../h/devio.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/workstation.h"
#include "../h/inputdevice.h"
#include "../h/wsdevice.h"
#include "../h/lk201.h"
#include "../io/tc/slu.h"
#include "../io/tc/xcons.h"
#include "../io/ws/vsxxx.h"
#include "../io/ws/tablet.h"		/* XXX when stuff reorged, not needed*/
/* XXX bogus tablet code bug */
extern int consDev;
extern	u_short	pointer_id;	/* id of pointer device (mouse,tablet)-ss.c */
#define NULL 0

unsigned int cdata[16] = { 
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

ws_cursor_data default_cursor = {
	0, 16, 16, 0, 0, cdata, cdata
};

/*
 * Things left to do in this driver.
 * LINT it!!!
 *
 * reorganize into seperate files for input devices.
 * ws_data file.
 * finish to deal with multiple instances (maybe not really necessary)
 * bt459 bug workaround.
 * faster interface for loading colormap.
 * escape box implementation.
 * get rid of last vestages of serial line dependency
 * make able to use driver when console is not on screen.
 * input extension work (not me).
 * debug event queue problems.  should allow server to pass in address.
 * devget defines for installation.
 * should we worry about SMP of the driver?
 * see if console rom routine can be used rather than crock lk201 routine.
 */

/*
 * ULTRIX settings for first open.		  
 */
#define IFLAGS	(EVENP|ECHO|XTABS|CRMOD)

/*
 * Termio flags will be set to these default values in non-termio mode to
 * provide a backward compatible ULTRIX environment. 
 */
#define IFLAG (BRKINT|IGNPAR|ISTRIP|IXON|IXANY|ICRNL)
#define OFLAG (OPOST|TAB3|ONLCR)
#define LFLAG (ISIG|ICANON|ECHO)
#define CFLAG (PARENB|CREAD|CS7|CLOCAL)

#define CONS_DEV      0x01
#define GRAPHIC_DEV   0x02
#define TOY ((time.tv_sec * 1000) + (time.tv_usec / 1000))
#define EVENTQUEUESIZE 100

/*
 * Keyboard translation and font tables
 */
#define CHAR_S	0xc7
#define CHAR_Q	0xc1

extern  char *q_special[],q_font[];
extern  u_short q_key[],q_shift_key[];

/* this memory to be mapped is allocated early on. */
caddr_t mapped_area = NULL;

typedef struct {
	ws_descriptor ws;
	struct proc *rsel;		/* process waiting for select */
	ws_event_queue *queue;		/* where to find shared queue */
	ws_event *events;		/* where the driver puts the events*/
	ws_event_queue *user_queue_address;
	ws_motion_buffer *mb;
	ws_motion_history *motion;
	short open_flag;
	short dev_in_use;
	short mouse_on;
	short keybd_reset;
	short max_event_size;
	short max_axis_count;
/*	short kern_loop;*/
	struct mouse_report last_rep;
	char new_switch, old_switch;
	unsigned int cbuf[128], mbuf[128];
} ws_info;
ws_info ws_softc[1] = {  { 0, 0, 0, 0, 0, 0, 1 }, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sizeof(ws_event), 0, 0, { 0, 0, 0}, 0, 0};

extern struct mouse_report current_rep;	/* now in dc7085cons.c */
extern u_int printstate;

/* XXX this should be in a ws_device_data.c file, so customers can play */
#define NLK201 1
ws_keyboard keyboard;
/* private keyboard data */
struct lk201info lk201_softc[NLK201] = {
	lk201_getc, lk201_putc, 90, 9, &keyboard,
	0, 0, 0, 0, 0, 0, 0,
	{0,0,0,0,0,0,0,0},
	0, 0, 0
};
/* public keyboard data */
ws_keyboard keyboard = {
	LK_201,			/* by default */
	(caddr_t) lk201_softc,
	0, 			/* no axis data */
	lk201_init_closure,
	NULL,			/* lk201_ioctl */
	lk201_up_down_mode,	/* init keyboard */
	lk201_reset_keyboard,	/* reset to (close to default lk201) state */
	NULL,			/* always enabled */
	NULL,			/* can't disable */
	lk201_set_keyboard_control,
	NULL,
	lk201_ring_bell,
	{ 0, 0, 50, 50, 400, 100,
	1, 		/* autorepeat on, except for modifiers, return */
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	 0xffffffff, 0xdff807ff, 0xffffffff, 0xffffffff, },
	0},		/* leds */
	&lk201_definition,
	lk201_modifiers,
	lk201_keysyms,
	lk201_keycodes,
};

ws_pointer mouse = {
	VSXXX,
	(caddr_t)NULL,
	2, 	/* 2 axis */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	{ 1, 1, 1, 1 },	/* pointer rates */
	{ 0, 0, 0 },
	{ 0, 0, 0, { 0, 0, 0, 0 },},	/* no entries in q for motion	*/
					/* inside this box.		*/
	{ 0, 0, 0, { 864, 1024, 0, 0}},/* prevent cursor from leaving	*/
	0,
	0,
};


#define NUMINPUTDEVICES 4
#define NUMSCREENS 3

typedef struct {
	int device_type;
	union {
	  ws_device *dp;
	  ws_keyboard *kp;
	  ws_pointer *pp;
	} p;
} ws_devices;

ws_devices devices[NUMINPUTDEVICES]; /* = {
	{ KEYBOARD_DEVICE, (ws_device *)&keyboard },
	{ MOUSE_DEVICE, (ws_device *)&mouse },
	{ NULL_DEVICE, (ws_device *)NULL},
}	{ NULL_DEVICE, (ws_device *)NULL},
;
*/
/* a screen has a screen, a color map, and a cursor */
typedef struct {
	ws_screen_descriptor *sp;
	ws_visual_descriptor *vp;
	ws_depth_descriptor *dp;
	ws_screen_functions *f;
	ws_color_map_functions *cmf;
	ws_cursor_functions *cf;
	ws_screen_box adj_screens;
} ws_screens;
ws_screens screens[NUMSCREENS];
int wsstart();

extern int cpu;
/*
 * Open the graphic device.
 */
/*ARGSUSED*/
wsopen(dev, flag)
	dev_t dev;
{
	register int i;
	register int unit = minor(dev);
	register struct tty *tp;
	register ws_info *wi = &ws_softc[0];
	register ws_screens *wsp = &screens[wi->ws.console_screen];
	register ws_devices *wsd = &devices[wi->ws.console_keyboard];
	if (wi->ws.num_screens_exist < 1) return(ENODEV);
/* XXX	if (consDev != GRAPHIC_DEV) return(ENODEV);*/
	/*
 	 * The graphics device can be open only by one person 
 	 */
	if (unit == 1) {
	    if (wi->open_flag != 0)
		return(EBUSY);
	    else
		wi->open_flag = 1;
            wi->dev_in_use |= GRAPHIC_DEV;  /* graphics dev is open */
	    for (i = 0; i < wi->ws.num_screens_exist; i++) {
		register ws_screens *wsp = &screens[i];
		(*(wsp->f->init_screen))(wsp->f->sc, wsp->sp);
		(*(wsp->cmf->init_color_map))(wsp->cmf->cmc);
		(*(wsp->cf->load_cursor))
			(wsp->cf->cc, wsp->sp, &default_cursor);
		ws_set_cursor_position (wsp->cf->cc, wsp, 0, 0);
	    }
	    (*(wsd->p.kp->init_keyboard))(wsd->p.kp->kc);

	    tp = &slu.slu_tty[unit];
	    ttychars(tp);
	    tp->t_state = TS_ISOPEN|TS_CARR_ON;
	    tp->t_cflag = tp->t_cflag_ext = B4800;
	    tp->t_iflag_ext = 0;
	    tp->t_oflag_ext = 0;
	    tp->t_lflag_ext = 0;

	    /*
 	     * set up event queue for later
 	     */
	    wi->mouse_on = 1;
	    wi->ws.cpu = cpu;
	    return(0);

	}

	wi->dev_in_use |= CONS_DEV;  /* mark console as open */

	tp = &slu.slu_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
	    return (EBUSY);
	tp->t_addr = (caddr_t)tp;
	tp->t_oproc = wsstart;

	/*
	 * Look at the compatibility mode to specify correct 
	 * default parameters and to insure only standard specified 
	 * functionality.
	 */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
#ifdef O_NOCTTY
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
		tp->t_state |= TS_ONOCTTY;
#endif O_NOCTTY
	if ((tp->t_state&TS_ISOPEN) == 0) {
	    ttychars(tp);
	    tp->t_state = TS_ISOPEN|TS_CARR_ON;
	    tp->t_cflag = tp->t_cflag_ext = B4800;
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
	    if (flag & O_TERMIO) {
		/*
		 * Provide a termio style environment.
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
		/*
		 * Provide a backward compatible ULTRIX 
		 * environment.  "COOKED" style.	
		 */
		tp->t_flags = IFLAGS;
		tp->t_iflag = IFLAG;
		tp->t_oflag = OFLAG;
		tp->t_lflag = LFLAG;
		tp->t_cflag |= CFLAG;
	    }
	}
        /*
 	 * Process line discipline specific open.
 	 */
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*
 * Close the graphic device.
 */

/*ARGSUSED*/
wsclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register ws_info *wi = &ws_softc[0];
	register ws_screens *wsp = &screens[wi->ws.console_screen];
	register ws_devices *wsd = &devices[wi->ws.console_keyboard];
	register int i;
	extern ws_update_event_queue_time();

	/*
 	 * If unit is not the mouse call the line disc. 
	 * otherwise clear the state
 	 * flag, and put the keyboard into down/up.
 	 */
	if (unit == 0) {
	    tp = &slu.slu_tty[unit];
	    (*linesw[tp->t_line].l_close)(tp);
	    ttyclose(tp);
	    wi->dev_in_use &= ~CONS_DEV;
/* XXX	    ws_keyboard.cntrl = ws_keyboard.shift = 0;*/
	    tp->t_state = 0;
	    
	    /* Remove termio flags that do not map */
	    tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	    tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	    tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	    tp->t_lflag &= ~TERMIO_ONLY_LFLAG;
	} else {
	    untimeout(ws_update_event_queue_time, wi);
	    wi->mouse_on = 0;
	    if (wi->open_flag != 1)
		return(EBUSY);
	    else
		wi->open_flag = 0; /* mark the graphics device available */
	    wi->dev_in_use &= ~GRAPHIC_DEV;
	    (*(wsd->p.kp->reset_keyboard))(wsd->p.kp->kc);

	    for (i = 0; i < wi->ws.num_screens_exist; i++) {
		register ws_screens *wsp = &screens[i];
		(*(wsp->f->clear_screen))(wsp->f->sc, wsp->sp);
		(*(wsp->cmf->init_color_map))(wsp->cmf->cmc);
		(*(wsp->cf->load_cursor))
			(wsp->cf->cc, wsp->sp, &default_cursor);
		/* if screen is console, leave cursor on */
		if (i == 0)
		    (*(wsp->cf->cursor_on_off))(wsp->cf->cc, CURSOR_ON);
		else
		    (*(wsp->cf->cursor_on_off))(wsp->cf->cc, CURSOR_OFF);
		ws_set_cursor_position
			(wsp->cf->cc, wsp, 
			wsp->sp->col * wsp->sp->f_width, 
			wsp->sp->row * wsp->sp->f_height);
		if (wsp->f->close) (*(wsp->f->close))(wsp->f->sc);
	    }
	    wi->user_queue_address = wi->queue = NULL;
	}
}

/*
 * workstation device select routine.  If graphics device is open, then
 * detect input queue.
 */

wsselect(dev, rw)
	dev_t dev;
{
	register ws_info *wi = &ws_softc[0];
	register int unit = minor(dev);
	register int s=spltty();
	register ws_event_queue *wsep = wi->queue;

	if (unit == 1) {
	    switch(rw) {
	    case FREAD:			/* if events okay */
			if (wsep->head != wsep->tail) {
			    splx(s);
			    return(1);
			}
			wi->rsel = u.u_procp;
			splx(s);
			return(0);
	    case FWRITE:		/* can never write */
			splx(s);
			return(EACCES);
	    }
	} else {
	    splx(s);
	    return(ttselect(dev, rw));
	}
}

/*
 * Graphic device ioctl routine.
 */
/*ARGSUSED*/
wsioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	register caddr_t data;
{
	register ws_info *wi = &ws_softc[0];
	register ws_screens *wsp;
	register ws_devices *wdp;
	register struct tty *tp;
	register int unit = minor(dev);
	register struct devget *devget;
	register ws_descriptor *wp = (ws_descriptor *)data;
	char *devname = "COLOR";
	int length = 6;
	int error, i;
	ws_cursor_position *wcp = (ws_cursor_position *)data;

	
	if (((cmd >> 8) & 0377) == 'w') {
	/* save a bit of code, by doing the check once */
	    if (cmd & IOC_S) {
		register int screen = ((ws_screen_ioctl *)data)->screen;
		if ((screen < 0) || (screen >= wi->ws.num_screens_exist)) 
			return (ENXIO);
		wsp = &screens[screen]; 
	    }
	    else if (cmd & IOC_D) {
		register int device;
		if (cmd & IOC_S) 
		  device = ((ws_screen_and_device_ioctl *)data)->device_number;
		else
		  device  = ((ws_device_ioctl *)data)->device_number;
		if ((device < 0) || (device >= wi->ws.num_devices_exist)) 
			return (ENXIO);
		wdp = &devices[device]; 
	    }
	    switch(cmd)  {
	    case GET_WORKSTATION_INFO:
		*wp = wi->ws;
		break;
	    case WRITE_COLOR_MAP:
	    {
		ws_color_map_data *cp = (ws_color_map_data *)data;
		ws_color_cell entry;
		/* XXX this could be faster than two procedure calls/entry */
		for (i = cp->start;  i < cp->start + cp->ncells; i++) {
			copyin(cp->cells + i, &entry, sizeof(entry));
			error = (*(wsp->cmf->load_color_map_entry))
				(wsp->cmf->cmc, cp->map, &entry);
			if (error < 0) return error;
		}
	        break;
	     }
	    case SET_CURSOR_POSITION:
		return(ws_set_cursor_position
			(wsp->cf->cc, wsp, wcp->x, wcp->y));
	    case SET_POINTER_POSITION:
	    {
		ws_pointer_position *wpp = (ws_pointer_position *)data;
		ws_pointer *p;
		if (wpp->device_number != wi->ws.console_pointer) 
			return EINVAL;
		p = (ws_pointer *)devices[wpp->device_number].p.pp;
		p->position.screen = wpp->screen;
		p->position.x = wpp->x;
		p->position.y = wpp->y;
		return(ws_set_cursor_position
			(wsp->cf->cc, wsp, wpp->x, wpp->y));
	    }
	    case LOAD_CURSOR:
	    {	/* do some work so that cursor load routines don't have to */
		register ws_cursor_data *cdp = (ws_cursor_data *)data;
		register nbytes = (((cdp->width + 31) >> 5)<< 2) * cdp->height;
		register int ret;
		if (nbytes > sizeof(wi->cbuf)) nbytes = sizeof(wi->cbuf);
		copyin(cdp->cursor, wi->cbuf, nbytes);
		copyin(cdp->mask,   wi->mbuf, nbytes);
		cdp->cursor = wi->cbuf;
		cdp->mask   = wi->mbuf;
		ret = (*(wsp->cf->load_cursor))
			(wsp->cf->cc, wsp->sp, cdp);
		return (ret);
	    }
	    case RECOLOR_CURSOR:
	    {
	        ws_cursor_color *ccp = (ws_cursor_color *) data;
		return (*(wsp->cf->recolor_cursor))
		    (wsp->cf->cc, wsp->sp, 
		     &ccp->foreground, &ccp->background);
	     }
	    case GET_SCREEN_INFO:
		*((ws_screen_descriptor *)data) = *(wsp->sp);
		break;
	    case GET_DEPTH_INFO: 
	    {	register ws_depth_descriptor *dp = 
			(ws_depth_descriptor *) data;
		if ((dp->which_depth) < 0 ||
			(dp->which_depth >= wsp->sp->allowed_depths))
			return EINVAL;
		*((ws_depth_descriptor *)data) = wsp->dp[dp->which_depth];
		break;
	    }
	    case MAP_SCREEN_AT_DEPTH:
	    {
		register ws_map_control *mp = (ws_map_control *) data;
		if ((mp->which_depth) < 0 ||
			(mp->which_depth >= wsp->sp->allowed_depths))
			return EINVAL;
		return ((*(wsp->f->map_unmap_screen))
			(wsp->f->sc, wsp->dp, wsp->sp, mp));
	    }
	    case GET_VISUAL_INFO:
	    {	register ws_visual_descriptor *vp = 
			(ws_visual_descriptor *) data;
		if ((vp->which_visual) < 0 ||
		    (vp->which_visual >= wsp->sp->nvisuals))
			return EINVAL;
		*((ws_visual_descriptor *)data) = wsp->vp[vp->which_visual];
		break;
	    }
	    case VIDEO_ON_OFF:
	    {
		register ws_video_control *vcp;
		vcp = (ws_video_control *) data;
		if (vcp->control == SCREEN_OFF) 
		     (*(wsp->cmf->video_off))(wsp->cmf->cmc);
		else (*(wsp->cmf->video_on))(wsp->cmf->cmc);
		break;
	    }
	    case SET_POINTER_BOX: /* XXX do anything about bogus box? */
	    {  	register ws_pointer_box *pbp = (ws_pointer_box *) data;
		register int newx, newy;
		ws_pointer *p;
/* XXX following line should go replaced when more general input stuff done */
		if (pbp->device_number != wi->ws.console_pointer) 
			return EINVAL;
		p = (ws_pointer *)devices[pbp->device_number].p.pp;
		/* if the cursor is outside, move it to nearest edge... */
		p->constrain = *pbp;
		newx = p->position.x;
		newy = p->position.y;
	 	if (newx < pbp->box.left)   newx = pbp->box.left;
		if (newx > pbp->box.right)  newx = pbp->box.right;
		if (newy < pbp->box.top)    newy = pbp->box.top;
		if (newy > pbp->box.bottom) newy = pbp->box.bottom;
		p->position.screen = pbp->screen;
		ws_set_cursor_position(wsp->cf->cc, wsp, newx, newy);
		break;
	    }
	    case SET_ESCAPE_BOX:
	    {   register ws_pointer_box *pbp = (ws_pointer_box *) data;
		ws_pointer *p;
/* XXX following line should go replaced when more general input stuff done */
		if (pbp->device_number != wi->ws.console_pointer) 
			return EINVAL;
		p = (ws_pointer *)devices[pbp->device_number].p.pp;
		p->suppress = *pbp;
		break;
	    }
	    case GET_AND_MAP_EVENT_QUEUE:
	    {	
	      /* XXX this needs some help, should take address of where the
	         queue should be put....*/
		ws_event_queue **weqp;
		extern ws_update_event_queue_time();
		int nbytes;
		short motion_size;
		weqp = (ws_event_queue **) data;
	        motion_size = sizeof(ws_motion_history) /*  +
		    ((wi->max_axis_count == 0 ) ? 0 :
		    (sizeof(short) * (wi->max_axis_count - 1))) */ ;
		nbytes = (int)wi->max_event_size * EVENTQUEUESIZE
  		  + sizeof(ws_event_queue) 
		  + sizeof(ws_motion_buffer)
		  + motion_size * MOTION_BUFFER_SIZE;
/*XXX why the hell can't I allocate 4096 and then map it? 
cprintf("nbytes = %d\n", nbytes);
		nbytes = (nbytes + (CLBYTES - 1)) & ~(CLBYTES - 1);
cprintf("nbytes = %d\n", nbytes);

		if (wi->queue == NULL) {*/
		if (mapped_area == NULL) {
			if (mapped_area == NULL)
			    mapped_area = (caddr_t) km_alloc((unsigned)nbytes,
				KM_DEVBUF, KM_NOARG);

			if (mapped_area == NULL) return ENOMEM;
		}
		wi->user_queue_address = (ws_event_queue *)
		    ws_map_region(mapped_area,   nbytes, 0600);
		wi->queue = (ws_event_queue *) mapped_area;
		wi->queue->events = (ws_event *)(wi->user_queue_address + 1);
		wi->events = (ws_event *) ((int)mapped_area + 
					sizeof(ws_event_queue));
		wi->queue->time = TOY;
		wi->queue->size = EVENTQUEUESIZE;
		wi->queue->event_size = wi->max_event_size;
		wi->queue->head = wi->queue->tail = 0;
		wi->mb = (ws_motion_buffer *)((unsigned int)wi->events +
			 (wi->max_event_size * EVENTQUEUESIZE));

		wi->motion = (ws_motion_history *) 
			((unsigned int)wi->mb + sizeof(ws_motion_buffer));
		wi->queue->mb = 
			(ws_motion_buffer *) ((unsigned int) wi->queue->events 
			+ (wi->max_event_size * EVENTQUEUESIZE));
		wi->mb->motion =  (ws_motion_history *) 
		    ((unsigned int)wi->queue->mb + sizeof(ws_motion_buffer));
		wi->mb->entry_size = motion_size;
		wi->mb->axis_count = wi->max_axis_count;
		wi->mb->size = MOTION_BUFFER_SIZE;
		wi->mb->next = 0;
		*weqp = wi->user_queue_address;
		timeout (ws_update_event_queue_time, wi, hz);
		break;
	    }
	    case SET_EDGE_CONNECTION:
	    {
		register ws_edge_connection *nep = (ws_edge_connection *)data;
		register ns = wi->ws.num_screens_exist;
		if ( nep->adj_screens.top   < -1  || 
		    nep->adj_screens.bottom < -1  || 
		    nep->adj_screens.right  < -1  || 
		    nep->adj_screens.left   < -1  || 
		    nep->adj_screens.top    >= ns ||
		    nep->adj_screens.bottom >= ns ||
		    nep->adj_screens.right  >= ns ||
		    nep->adj_screens.left   >= ns) return EINVAL;
		wsp->adj_screens = nep->adj_screens;
		break;
	    }		
	    case GET_EDGE_CONNECTION:
	    {
		register ws_edge_connection *nep = (ws_edge_connection *)data;
		nep->adj_screens = wsp->adj_screens;
		break;
	    }
	    case CURSOR_ON_OFF:
	    {
		register ws_cursor_control *ccp = (ws_cursor_control *)data;
		return (*(wsp->cf->cursor_on_off))
		    (wsp->cf->cc, ccp->control);
	     }
	    case SET_MONITOR_TYPE:
	    {
		register ws_monitor_type *wmp = (ws_monitor_type *) data;
		wsp->sp->monitor_type = wmp->monitor_type;
		break;
	    }
	    case SET_POINTER_CONTROL:
	    {
		ws_pointer_control *wpcp = (ws_pointer_control *) data;
		int type = devices[wpcp->device_number].device_type;

		if ((type != MOUSE_DEVICE) && (type != TABLET_DEVICE))
			return EINVAL;
		devices[wpcp->device_number].p.pp->pr = *wpcp;
		break;
	    }
	    case GET_POINTER_CONTROL:
	    {
		ws_pointer_control *wpcp = (ws_pointer_control *) data;
		int type = devices[wpcp->device_number].device_type;
		if ((type != MOUSE_DEVICE) && (type != TABLET_DEVICE))
			return EINVAL;
		*wpcp = devices[wpcp->device_number].p.pp->pr;
		break;
	    }
	    case GET_DEVICE_TYPE:
	    {
		ws_hardware_type *whtp = (ws_hardware_type *) data;
		whtp->hardware_type = 
			devices[whtp->device_number].p.dp->hardware_type;
		break;
	    }
	    case SET_KEYBOARD_CONTROL:
	    {
		ws_keyboard_control *wskp = (ws_keyboard_control *) data;
		int type = devices[wskp->device_number].device_type;
		if (type != KEYBOARD_DEVICE) return EINVAL;
		if (wdp->p.kp->set_keyboard_control) 
			return ((*(wdp->p.kp->set_keyboard_control))
				(wdp->p.kp->kc, wdp->p.kp, wskp));
		break;
	    }
	    case GET_KEYBOARD_CONTROL:
	    {
		ws_keyboard_control *wskp = (ws_keyboard_control *) data;
		int type = devices[wskp->device_number].device_type;
		if (type != KEYBOARD_DEVICE) return EINVAL;
		*wskp = wdp->p.kp->control;
		break;
	    }
	    case RING_KEYBOARD_BELL:
	    {
	        short *device_number = (short *) data;
		int type = devices[*device_number].device_type;
		if (type != KEYBOARD_DEVICE) return EINVAL;
		(*(wdp->p.kp->ring_bell)) (wdp->p.kp->kc);
		break;
	    }
	    case GET_KEYBOARD_DEFINITION:
	    {
	        ws_keyboard_definition *wkdp = (ws_keyboard_definition *) data;
		register int device_number = wkdp->device_number;
		int type = devices[device_number].device_type;
		if (type != KEYBOARD_DEVICE) return EINVAL;
		*wkdp = *devices[device_number].p.kp->definition;
		wkdp->device_number = device_number;
		break;
	    }
	    case GET_KEYSYMS_AND_MODIFIERS:
	    {
	        ws_keysyms_and_modifiers *wkmp = 
	    		(ws_keysyms_and_modifiers *) data;
		register int device_number = wkmp->device_number;
		register ws_keyboard *keyboard;
		int type = devices[device_number].device_type;
		if (type != KEYBOARD_DEVICE) return EINVAL;
		keyboard = devices[device_number].p.kp;
		copyout (keyboard->modifiers, wkmp->modifiers, 
			keyboard->definition->modifier_keycode_count 
			* sizeof(ws_keycode_modifiers));
		copyout (keyboard->keysyms, wkmp->keysyms,
			keyboard->definition->keysyms_present
			* sizeof (unsigned int));
		copyout (keyboard->keycodes, wkmp->keycodes,
			keyboard->definition->keysyms_present
			* sizeof (unsigned char));
		break;
	    }
	    default:
		if (cmd & IOC_S) {
			/* if a screen has an ioctl handler, then call it */
			if (wsp->f->ioctl == NULL) return EINVAL;
			else return(*(wsp->f->ioctl))
				(wsp->f->sc, cmd, data, flag);
		}
		if (cmd & IOC_D) {
			/* if a device has an ioctl handler, then call it */
			if (wdp->p.dp->ioctl == NULL) return EINVAL;
			else return((*(wdp->p.dp->ioctl))
				(wdp->p.dp->dc, cmd, data, flag));
		}
		return EINVAL;
	    }
	return (0);
	}
	switch (cmd) {
	    case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TERMINAL;	/* terminal cat.*/
		devget->bus = DEV_NB;			/* NO bus	*/
		bcopy(DEV_VS_SLU,devget->interface,
		      strlen(DEV_VS_SLU));		/* interface	*/
		if ((unit == 0) || (unit == 1)) {
		        bcopy(devname,devget->device, length);/* Ultrix "fb"*/
		}
		if (pointer_id == MOUSE_ID) {
		        bcopy(devname,devget->device, length);/* Ultrix "fb"*/
		}
		else if (pointer_id == TABLET_ID)
		    bcopy(DEV_TABLET,devget->device, strlen(DEV_TABLET));
		else
		    bcopy(DEV_UNKNOWN,devget->device, strlen(DEV_UNKNOWN));
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0	*/
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = 0;			/* cntlr number */
		devget->slave_num = unit;		/* line number 	*/
		    bcopy("COLOR",devget->dev_name, 6);/* Ultrix "fb"	*/
		devget->unit_num = unit;		/* dc line?	*/
		/* XXX TODO: should say not supported instead of zero!	*/
		devget->soft_count = 0;			/* soft err cnt */
		devget->hard_count = 0;			/* hard err cnt */
		devget->stat = 0;			/* status	*/
		devget->category_stat = 0;		/* cat. stat.	*/
/* XXX what to do about devget defines... */
		break;

	    default:					/* not ours ??  */
		tp = &slu.slu_tty[unit];
		error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
		if (error >= 0)
			return (error);
		error = ttioctl(tp, cmd, data, flag);
		if (error >= 0) 
			return (error);
	        /* if error = -1 then ioctl does not exist */
		if (u.u_procp->p_progenv == A_POSIX) 
			return (EINVAL);
		return (ENOTTY);
		break;
	}
	return (0);
}


/*
 * map a region of memory.  If the function succeeds, it returns the address
 * where it is visible to the user (the pages are double mapped, so that
 * physical memory can be accessed, for example framebuffers or I/O registers.)
 */

#define DWN(x) ((int)(x) & ~(CLBYTES-1))
caddr_t ws_map_region (addr, nbytes, how)
	caddr_t addr;
	register int nbytes;
	int how;
{
	register caddr_t result;
	register int tmp1;
	register int smid;
	register int start = DWN(addr);
	register int end = (int)addr + nbytes;
	register int size;
	char *smat();

	size = DWN(end + CLBYTES) - start;

	/* how was 0600 */
	if ((smid = vm_system_smget(start, size, how)) < 0)
		goto bad;

	if ((result = smat(smid, 0, 0)) == (caddr_t) -1)
		goto bad;
	if (smctl(smid, IPC_RMID, 0))
		goto bad;

	/* the following returns the low order bits to the mapped result. */
	tmp1 = (int)result;
	tmp1 |= (int) addr & (CLBYTES-1);
	return ((caddr_t) tmp1);
bad:	
	cprintf ("ws0: cannot map shared data structures\n");
	cprintf ("u_error = %d\n", u.u_error);
	return (NULL);
}

/*
 * Start transmission
 */
wsstart(tp)
	register struct tty *tp;
{
	register int unit, c;
	register ws_info *wi = &ws_softc[0];
	register ws_screens *wsp = &screens[wi->ws.console_screen];
	int s, xcons_status;

	unit = minor(tp->t_dev);
	s = spltty();

	/*
 	 * If it's currently active, or delaying, no need to do anything.
 	 */
	if (tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
	    goto out;
	/*
 	 * Display chars until the queue is empty, if the second subchannel 
	 * is open irect them there. Drop characters from any lines other 
	 * than 0 on the floor. TANDEM is set on second subchannel for flow 
	 * control.
 	 */
	while( tp->t_outq.c_cc ) {
	    if (unit == 0) {		/* console device */
		xcons_status = xcons_chkq();
		
		switch (xcons_status) {
		  case XCONS_CLOSED:
		    c = getc(&tp->t_outq);
		    ws_blitc(wsp, c & 0xff);
		    break;
		    
		  case XCONS_BLOCKED:
		    goto out;
		    break;
		    
		  case XCONS_OK:
		    c = getc(&tp->t_outq);
		    xconsrint(c);
		    break;
		}
	    } else
		c = getc(&tp->t_outq);
	}

	/*
 	 * Position the cursor to the next character location.
 	 */
	if (!(wi->dev_in_use & GRAPHIC_DEV))
	    ws_set_cursor_position
		(wsp->cf->cc, wsp, 
			wsp->sp->col * wsp->sp->f_width, 
			wsp->sp->row * wsp->sp->f_height);

	/*
 	 * If there are sleepers, and output has drained below low
 	 * water mark, wake up the sleepers.
 	 */
	tp->t_state &= ~TS_BUSY;
out:
	if (tp->t_outq.c_cc<=TTLOWAT(tp))
	    if (tp->t_state&TS_ASLEEP) {
		tp->t_state &= ~TS_ASLEEP;
		wakeup((caddr_t)&tp->t_outq);
	    }
	splx(s);

}

/*
 * Routine to stop output on the graphic device, e.g. for ^S/^Q
 * or output flush.
 */

/*ARGSUSED*/
wsstop(tp, flag)
	register struct tty *tp;
{
	register int s;

	/*
         * Block interrupts while modifying the state.
 	 */
	s = spltty();
	if (tp->t_state & TS_BUSY)
	    if ((tp->t_state&TS_TTSTOP)==0)
		tp->t_state |= TS_FLUSH;
	    else
		tp->t_state &= ~TS_BUSY;
	splx(s);
}

/*
 * Output a character to the screen
 */

ws_blitc(wsp, c)
register ws_screens *wsp;
register u_char c;
{
	register ws_screen_descriptor *wsdp = wsp->sp;
	register ws_info *wi = &ws_softc[0];
	register ws_devices *wsd = &devices[wi->ws.console_keyboard];
	register int i;

	c &= 0xff;
	switch (c) 
	{
	    case '\t':				/* tab		*/
		for (i = 8 - (wsdp->col & 0x7) ; i > 0 ; i--)
			ws_blitc(wsp, ' ');
		return(0);
	    case '\r':				/* return	*/
		wsdp->col = 0;
		return(0);
	    case '\b':				/* backspace	*/
		if (--wsdp->col < 0)
			wsdp->col = 0;
		return(0);
	    case '\n':				/* linefeed	*/
		if (wsdp->row+1 >= wsdp->max_row) {
			if (wi->mouse_on)	wsdp->row = 0;
			else	(*(wsp->f->scroll_screen))
					(wsp->f->sc, wsp->sp);
		}
		else
			wsdp->row++;
		    /*
 	 	     * Position the cursor to the next character location.
 	 	     */
	    	if (!(wi->dev_in_use & GRAPHIC_DEV))
		ws_set_cursor_position 
		    (wsp->cf->cc, wsp, 
			wsdp->col * wsdp->f_width, wsdp->row * wsdp->f_height);
		return(0);
	    case '\007':			/* bell		*/
		(*(wsd->p.kp->ring_bell))(wsd->p.kp->kc);
		return(0);
	    default:
		/*
		 * If the next character will wrap around then 
		 * increment row counter or scroll screen.
		 */
		if (wsdp->col >= wsdp->max_col) {
			wsdp->col = 0 ;
			if (wsdp->row+1 >= wsdp->max_row) {
				if (wi->mouse_on)	wsdp->row = 0;
				else (*(wsp->f->scroll_screen))
						(wsp->f->sc, wsp->sp);
			}
			else
			    wsdp->row++;
		}
		(*(wsp->f->blitc))
			(wsp->f->sc, wsp->sp, wsdp->row, wsdp->col, c);
		wsdp->col += 1;
		return(0);
	}
}

/*
 * set the cursor to the specified position.  Sets the system's current
 * belief about where the cursor is, constrained by the size of the
 * screen.   Calls out to the appropriate hardware routine; old cursor
 * position is still in screen structure at time of call.  The driver
 * only knows about hot spot coordinates.
 */
int ws_set_cursor_position(closure, wsp, x, y)
	register caddr_t closure;
	register ws_screens *wsp;
	register int x, y;
{
	register ws_screen_descriptor *screen = wsp->sp;
	register int value;
	if (y < 0) 		y = 0;
	if (y >= screen->height)y = screen->height - 1;
	if (x < 0)		x = 0;
	if (x >= screen->width)	x = screen->width - 1;
	value = (*(wsp->cf->set_cursor_position)) (closure, screen, x, y);
	screen->x = x;
	screen->y = y;
	return(value);
}

ws_cursor_position *
ws_get_cursor_position()
{
	return(&devices[ws_softc[0].ws.console_pointer].p.pp->position);
	/* whew! */
}

/*
 * Register a device with the generic driver.
 */

int ws_define_device(device_type, dp, event_size)
	register int device_type;
	register ws_device *dp;
	register int event_size;
{
	register ws_info *wi = &ws_softc[0];
	register ws_devices *wdp;
	register int i = wi->ws.num_devices_exist;
	register int j;
	int found = 0;

	/* run through and see if device type already defined */
	for (j=0; j<i; j++)
	  if (devices[j].device_type == device_type) {
	    i = j;
	    found = 1;
	    break;
	  }

	/* if new device type, increment num_devices_exist */
	if (!found)
	  if (wi->ws.num_devices_exist >= NUMINPUTDEVICES)
	    return -1;
	  else
	    wi->ws.num_devices_exist++;

	wdp = &devices[i];
	wdp->device_type = device_type;
	wdp->p.dp = dp;
	if (event_size > wi->max_event_size) wi->max_event_size = event_size;
	if(wdp->p.dp->axis_count > wi->max_axis_count) 
	    wi->max_axis_count = wdp->p.dp->axis_count;
	return i;
}

/*
 * Register a screen with the generic driver.
 */
int ws_define_screen(sp, vp, dp, f, cmf, cf)
	ws_screen_descriptor *sp;
	ws_visual_descriptor *vp;
	ws_depth_descriptor *dp;
	ws_screen_functions *f;
	ws_color_map_functions *cmf;
	ws_cursor_functions *cf;
{
	register ws_info *wi = &ws_softc[0];
	register ws_screens *wsp;
	register int i = wi->ws.num_screens_exist;
	if (i >= NUMSCREENS) return (-1);
	sp->screen = i;
	dp->screen = i;
	wsp = &screens[i];
	wsp->sp = sp;
	wsp->vp = vp;
	wsp->dp = dp;
	wsp->f  = f;
	wsp->cmf= cmf;
	wsp->cf = cf;
	wsp->adj_screens.top   = -1;
	wsp->adj_screens.bottom = -1;
	wsp->adj_screens.left = -1;
	wsp->adj_screens.right = -1;
	if (wi->ws.num_screens_exist > 0) {
		wsp->adj_screens.left   = i - 1;
		screens[i - 1].adj_screens.right = i;
	}
	wi->ws.num_screens_exist += 1;
	return (i);
}

int ws_clicks = 0;
ws_update_event_queue_time(wi)
	register ws_info *wi;
{
	timeout (ws_update_event_queue_time, wi, hz);
	wi->queue->time = TOY;
}

/*
 * take a keyboard event and put it in the event queue.
 */
#define EVROUND(q, x) ((x) % ((q)->size))
ws_enter_keyboard_event(queue, ch, p, type)
	register ws_event_queue *queue;
	int ch;
	ws_pointer *p;
        int type;
{
	register ws_info *wi = &ws_softc[0];
	register ws_event *ev = wi->events;
	int i;
	if ((i = EVROUND(queue, queue->tail + 1)) == queue->head)
		return;

	ev = (ws_event *)((int)(wi->events) + queue->tail * queue->event_size);
	ev->time = queue->time = TOY;
	ev->screen = p->position.screen;
	ev->device = wi->ws.console_keyboard;
	ev->device_type = KEYBOARD_DEVICE;
	ev->type = type;
	ev->e.key.key = ch & 0xff;
	ev->e.key.pad = 0;
	ev->e.key.x   = p->position.x;
	ev->e.key.y   = p->position.y;
	queue->tail = i;
}
/*
 * enters an event into the queue, and timestamps it, otherwise does not
 * affect the event.
 */
#ifdef NOTDEF
ws_enter_event(queue, event, size)
	register ws_event_queue *queue;
	ws_event *event;
	int size;
{
	register ws_info *wi = &ws_softc[0];
	register ws_event *ev = wi->events;
	int i;
	if ((i = EVROUND(queue, queue->tail + 1)) == queue->head)
		return;

	ev = (ws_event *)((int)(wi->events) + queue->tail * queue->event_size);
	bcopy ((char *) event, (char *) ev, size);
	ev->time = queue->time = TOY;
	queue->tail = i;
}
#endif NOTDEF

ws_enter_event(event, size)
	ws_event *event;
	int size;
{
	register ws_info *wi = &ws_softc[0];
	register ws_event *ev = wi->events;
	register ws_event_queue *queue = wi->queue;
	register short *ap;
	
	int i;
	if ((i = EVROUND(queue, queue->tail + 1)) == queue->head)
		return;

	ev = (ws_event *)((int)(wi->events) + queue->tail * queue->event_size);
	bcopy ((char *) event, (char *) ev, size);
	ev->time = queue->time = TOY;
	queue->tail = i;
        if(event->type == MOTION_TYPE){
	    ws_motion_history *mhp = wi->motion;
	    mhp = (ws_motion_history *)(((int)mhp) + 
				wi->mb->next * wi->mb->entry_size);
	    mhp->time = ev->time;
	    mhp->device = event->device;
	    mhp->screen = event->screen;
	    ap = (short *) ((char *)event + sizeof(event));
	    for(i = 0; i < devices[event->device].p.dp->axis_count; i++) 
		mhp->axis[i] = *ap++;
	    if (++wi->mb->next >= wi->mb->size) wi->mb->next = 0;
	}
}

#define TOP_EDGE 0x1
#define BOTTOM_EDGE 0x2
#define LEFT_EDGE 0x4
#define RIGHT_EDGE 0x8
ws_screen_descriptor *ws_do_edge_work (wi, sp, p, edge)
	register ws_info *wi;
	register ws_screen_descriptor *sp;
	register ws_pointer *p;
	register int edge;
{
	register int ns = wi->ws.num_screens_exist;
	register int current_screen = p->position.screen;
	register ws_screens *wsp = &screens[current_screen];
	register int next_screen = current_screen;
	if (ns > 1) {
		if (edge & TOP_EDGE) {
			next_screen = wsp->adj_screens.top;
			if (next_screen != -1) {
				sp = screens[next_screen].sp;
				p->position.y = p->position.y + sp->height;
			}
		}
		else if (edge & BOTTOM_EDGE) {
			next_screen = wsp->adj_screens.bottom;
			if (next_screen != -1) {
				sp = screens[next_screen].sp;
				p->position.y = p->position.y - sp->height;
			}
		}
		else if (edge & LEFT_EDGE) {
			next_screen = wsp->adj_screens.left;
			if (next_screen != -1) {
				sp = screens[next_screen].sp;
				p->position.x = p->position.x + sp->width;
			}
		}
		else if (edge & RIGHT_EDGE) {
			next_screen = wsp->adj_screens.right;
			if (next_screen != -1) {
				sp = screens[next_screen].sp;
				p->position.x = p->position.x - sp->width;
			}
		}
	}
	if (next_screen != -1) p->position.screen = next_screen;
	if (p->position.x < 0)		p->position.x = 0;
	if (p->position.x >= sp->width)	p->position.x = sp->width - 1;
	if (p->position.y < 0)		p->position.y = 0;
	if (p->position.y >= sp->height)p->position.y = sp->height - 1;
	return sp;
}
/*
 * current screen is in pointer position!
 */
vsxxx_mouse_event(wi, queue, wsp, p, last_rep, new_rep, open)
	register ws_info *wi;
	register ws_event_queue *queue;
	ws_screens *wsp;
	register ws_pointer *p;
	register struct mouse_report *new_rep;
	struct mouse_report *last_rep;
	int open;
{
	register ws_screen_descriptor *sp = screens[p->position.screen].sp; 
	ws_motion_history *mhp;
	unsigned int millis = TOY;
	register int i;
	register ws_event *ev;
	register char temp;
	register int j;
	register int edge = 0;
/*
 * see if mouse position has changed
 */
	if (new_rep->dx != 0 || new_rep->dy != 0) {
	    /*
 	     * Check to see if we have to accelerate the mouse
	     * I'm not willing to make this computation floating point...
 	     *
 	     */
	    if (p->pr.denominator > 0) {
		if (new_rep->dx >= p->pr.threshold)
		    new_rep->dx +=
			((new_rep->dx - p->pr.threshold)* p->pr.numerator) /
			  p->pr.denominator;
		if (new_rep->dy >= p->pr.threshold)
		    new_rep->dy +=
			((new_rep->dy - p->pr.threshold)* p->pr.numerator) /
			  p->pr.denominator;
	    }
	    /*
 	     * update mouse position
 	     */
	    if (new_rep->state & X_SIGN) {
		p->position.x += new_rep->dx;
		if (p->position.x >= sp->width)	edge |= RIGHT_EDGE;
	    }
	    else {
		p->position.x -= new_rep->dx;
		if (p->position.x < 0) 		edge |= LEFT_EDGE;
	    }
	    if (new_rep->state & Y_SIGN) {
		p->position.y -= new_rep->dy;
		if (p->position.y < 0) 		edge |= TOP_EDGE;
	    }
	    else {
		p->position.y += new_rep->dy;
		if (p->position.y >= sp->height) edge |= BOTTOM_EDGE;
	    }
	    if (edge)
		sp = ws_do_edge_work (wi, sp, p, edge);
	    if (open)
		ws_set_cursor_position(screens[p->position.screen].cf->cc, 
	          &screens[p->position.screen], p->position.x, p->position.y);
	    mhp = wi->motion;
	    mhp = (ws_motion_history *)(((int)mhp) + 
				wi->mb->next * wi->mb->entry_size);

	    mhp->time = queue->time = millis;
	    mhp->device = wi->ws.console_pointer;
	    mhp->screen = p->position.screen;
	    mhp->axis[0] = p->position.x;
	    mhp->axis[1] = p->position.y;
	    if (++wi->mb->next >= wi->mb->size) wi->mb->next = 0;
	    if (p->suppress.enable &&
		p->position.y < p->suppress.box.bottom &&
		p->position.y >=  p->suppress.box.top &&
		p->position.x < p->suppress.box.right &&
		p->position.x >=  p->suppress.box.left)	 goto mbuttons;
	    p->suppress.enable = 0;	/* trash box */
	    if (EVROUND(queue, queue->tail + 1) == queue->head)
			goto mbuttons;

	    i = EVROUND(queue, queue->tail - 1);
	    if ((queue->tail != queue->head) && (i != queue->head)) {
		ev = (ws_event *) (((int)wi->events) + queue->event_size * i);
	        if (ev->type == MOTION_TYPE
			&& ev->device_type ==
			devices[wi->ws.console_pointer].device_type) {
		    ev->screen = p->position.screen;
		    ev->time = queue->time = millis;
		    ev->e.pointer.x = p->position.x;
		    ev->e.pointer.y = p->position.y;
		    ev->device = wi->ws.console_pointer;
		    goto mbuttons;
		}
	    }
	    /*
 	     * Put event into queue and do select
 	     */
	    ev =(ws_event *)(((int)wi->events) + queue->tail * queue->event_size);
	    ev->type = MOTION_TYPE;
	    ev->screen = p->position.screen;
	    ev->time = queue->time = millis;
	    ev->device_type = devices[wi->ws.console_pointer].device_type;
	    ev->e.pointer.x = p->position.x;
	    ev->e.pointer.y = p->position.y;
	    ev->e.pointer.buttons = new_rep->state & 0x07;
            queue->tail = EVROUND(queue, queue->tail + 1);
	}

/*
 * See if mouse buttons have changed.
 */
mbuttons:

	wi->new_switch = new_rep->state & 0x07;
	wi->old_switch = last_rep->state & 0x07;

	temp = wi->old_switch ^ wi->new_switch;
	if (temp) {
	    for (j = 1; j < 8; j <<= 1)  {/* check each button */
		if (!(j & temp))  /* did this button change? */
		    continue;
		/*
 		 * Check for room in the queue
 		 */
                if ((i = EVROUND(queue, queue->tail + 1)) == queue->head) 
		    return(0);

		/* put event into queue and do select */
		ev =(ws_event *)
			(((int)wi->events) + queue->tail * queue->event_size);

		switch (j) {
			case RIGHT_BUTTON:
				ev->e.button.button = EVENT_RIGHT_BUTTON;
				break;

			case MIDDLE_BUTTON:
				ev->e.button.button = EVENT_MIDDLE_BUTTON;
				break;

			case LEFT_BUTTON:
				ev->e.button.button = EVENT_LEFT_BUTTON;
				break;
		}
		if (wi->new_switch & j)
			ev->type = BUTTON_DOWN_TYPE;
		else
			ev->type = BUTTON_UP_TYPE;
		ev->device_type = devices[wi->ws.console_pointer].device_type;
		ev->time = queue->time = millis;
		ev->e.button.x = p->position.x;
		ev->e.button.y = p->position.y;
		ev->screen = p->position.screen;
	        queue->tail = i;
	    }
	    /* update the last report */
	    *last_rep = current_rep;
	    p->mswitches = wi->new_switch;
	} /* Pick up mouse input */
}

vsxxx_tablet_event(wi, queue, p, last_rep, new_rep, screen, open)
	ws_info *wi;
	ws_event_queue *queue;
	ws_pointer *p;
	register struct mouse_report *new_rep;
	struct mouse_report *last_rep;
	int screen;
	int open;
{
    int cs = p->position.screen;	/* current screen */
    register ws_screen_descriptor *sp = screens[cs].sp;
    ws_motion_history *mhp;
    unsigned int millis = TOY;
    register ws_event *ev;
    register char temp;
    register int i, j;

    /* update cursor position coordinates */
    new_rep->dx =  (new_rep->dx * p->tablet_scale_x) / 1000;
    new_rep->dy =  ((2200 - new_rep->dy) * p->tablet_scale_y) / 1000;
    if (new_rep->dx > sp->width)
	new_rep->dx = sp->width;
    if (new_rep->dy > sp->height)
	new_rep->dy = sp->height;

    /*
     * see if the puck/stylus has moved
     */
    if (p->position.x != new_rep->dx || p->position.y != new_rep->dy)  {
	/*
	 * update cursor position
	 */
	p->position.x = new_rep->dx;
	p->position.y = new_rep->dy;

	if (open) 
	    ws_set_cursor_position 
		(screens[cs].cf->cc, &screens[cs], 
		 p->position.x, p->position.y);
	if (	p->suppress.enable &&
	    p->position.y < p->suppress.box.bottom &&
	    p->position.y >=  p->suppress.box.top &&
	    p->position.x < p->suppress.box.right &&
	    p->position.x >=  p->suppress.box.left) goto tbuttons;
	p->suppress.enable = 0;		/* trash box */
	if (EVROUND(queue, queue->tail + 1) == queue->head)
	    goto tbuttons;

	/*
	 * Put event into queue and do select
	 */
	ev =(ws_event *)
	    (((int)wi->events) + queue->tail * queue->event_size);
	ev->type = MOTION_TYPE;
	ev->time = queue->time = millis;
	ev->device_type = devices[wi->ws.console_pointer].device_type;
	ev->screen = p->position.screen;
	ev->device = wi->ws.console_pointer;
	ev->e.pointer.x = p->position.x;
	ev->e.pointer.y = p->position.y;
	ev->e.pointer.buttons = new_rep->state & 0x1e;
	queue->tail = EVROUND(queue, queue->tail + 1);
    }

    /*
     * See if tablet buttons have changed.
     */

 tbuttons:
    wi->new_switch = new_rep->state & 0x1e;
    wi->old_switch = last_rep->state & 0x1e;

    temp = wi->old_switch ^ wi->new_switch;
    if (temp) {	/* XXX this looked wrong; shouldn't it be inside loop? */
	/* it _was_ wrong; now it is right. */

	/* define the changed button and if up or down */
	for (j = 1; j <= 0x10; j <<= 1) { /* check each button */
	    if (!(j & temp))		/* did this button change? */
		continue;
	    /*
	     * Check for room in the queue
	     */
	    if ((i=EVROUND(queue, queue->tail + 1)) == queue->head) 
		return(0);

	    /* put event into queue and do select */
	    ev =(ws_event *)
		(((int)wi->events) + queue->tail * queue->event_size);

	    switch (j) {
	     case T_RIGHT_BUTTON:
		ev->e.button.button = EVENT_T_RIGHT_BUTTON;
		break;
	     case T_FRONT_BUTTON:
		ev->e.button.button = EVENT_T_FRONT_BUTTON;
		break;
	     case T_BACK_BUTTON:
		ev->e.button.button = EVENT_T_BACK_BUTTON;
		break;
	     case T_LEFT_BUTTON:
		ev->e.button.button = EVENT_T_LEFT_BUTTON;
		break;
	    }
	    if (wi->new_switch & j)
		ev->type = BUTTON_DOWN_TYPE;
	    else
		ev->type = BUTTON_UP_TYPE;
	    ev->device_type = 
		devices[wi->ws.console_pointer].device_type;
	    ev->time = queue->time = millis;
	    ev->e.button.x = p->position.x;
	    ev->e.button.y = p->position.y;
	    ev->screen = p->position.screen;
	    ev->device = wi->ws.console_pointer;
	    queue->tail =  i;
	}

	/* update the last report */
	*last_rep = current_rep;
	p->mswitches = wi->new_switch;
    }
}					/* Pick up tablet input */


/*
 * Direct kernel console output to display destination
 */

wsputc(c)
	register char c;
{
	register ws_info *wi = &ws_softc[0];
	register ws_screens *wsp = &screens[wi->ws.console_screen];
	register ws_screen_descriptor *wsdp = wsp->sp;
	register int xcons_status;
	static int ws_panic_init = 0;

	if ((printstate & PANICPRINT) && (ws_panic_init == 0)) {
	        ws_panic_init = 1;
	        wi->mouse_on = 0;
		wsdp->row = wsdp->max_row - 1;
		(*(wsp->cmf->init_color_map))(wsp->cmf->cmc);
		(*(wsp->cmf->video_on))(wsp->cmf->cmc);
	}

	xcons_status = xcons_chkq();
	
	switch (xcons_status) {
	  case XCONS_CLOSED:
	    /* real console output */
	    ws_blitc(wsp, c & 0xff);
	    break;
	    
	  case XCONS_BLOCKED:
	    break;
	    
	  case XCONS_OK:
	    /* not panic'ing, routing to alternate console */
	    xconsrint(c);
	    break;
	}
}




/*
 * Routine to initialize the mouse. 
 * XXX this needs to be moved to a separate file.
 * NOTE:
 *	This routine communicates with the mouse by directly
 *	manipulating the PMAX SLU registers. This is allowed
 *	ONLY because the mouse is initialized before the system
 *	is up far enough to need the SLU in interrupt mode.
 */

int vsxxx_mouse_init(getc, putc)
	int (*getc)();
	int (*putc)();
{
	register int	i;
	int	id_byte[4];
	int 	s;
	int pointer_id = 0;
        ws_screen_descriptor *sp = screens[0].sp;
	s = spltty();


	/*
 	 * Set SLU line parameters for mouse communication.
 	 */
	(*slu.mouse_init)();

	(*putc)(SELF_TEST);
	/*
 	 * Pick up the four bytes returned by mouse or tablet self test.
 	 */
	for (i= 0; i < 4; i++) {
		id_byte[i] = (*getc)();
		if (id_byte[i] < 0)  {
		    mprintf("\nws: Timeout on byte %d of self-test report\n",
				id_byte[i]);
		    goto OUT;
		}
	}

	/*
 	 * Set the operating mode
 	 *
 	 * We set the mode for both mouse and the tablet to 
	 * "Incremental stream mode".  XXX (some tablet use is absolute!!)
 	 */
	if ((id_byte[1] & 0x0f) == MOUSE_ID) pointer_id = MOUSE_ID;
	else pointer_id = TABLET_ID;
	(*putc)(INCREMENTAL);
	if(pointer_id == TABLET_ID) {
	    ws_info *wi = &ws_softc[0];
	    ws_pointer *p = (ws_pointer *)devices[wi->ws.console_pointer].p.pp;
	    p->hardware_type = TABLET_DEVICE; /* hack!!! */
	    p->tablet_scale_x = (sp->width * 1000) / 2200;
	    p->tablet_scale_y = (sp->height * 1000) / 2200;
        }

OUT:
	splx(s);
	return(pointer_id);
}

extern struct mouse_report current_rep;		 /* now in dc7085cons.c */

/*
 * Graphice device interrupt routine. 
 */
wskint(ch)
register int ch;
{
	register ws_info *wi = &ws_softc[0];
	register ws_devices *wsd = &devices[wi->ws.console_keyboard];
	ws_pointer *p = (ws_pointer *)devices[wi->ws.console_pointer].p.pp;
	register ws_screens *wsp = &screens[p->position.screen];
	struct mouse_report *new_rep;
	struct tty *tp;
	register int unit;
	register u_short c;
	register int i, j;
	u_short data;
	struct lk201info *kbd	= (struct lk201info *)wsd->p.kp->kc;

	/*
 	 * Mouse state info
 	 */
	unit = (ch>>8)&03;
	new_rep = &current_rep;
	tp = &slu.slu_tty[unit];
	/*
 	 * If graphic device is turned on
 	 */

   	if (wi->mouse_on == 1) 	{
	    if (wi->queue == NULL) return;
  	    /*
 	     * Pick up LK-201 input (if any)
 	     */
	    if (unit == 0) 
	        lk201_keyboard_event(&lk201_softc[0], wi->queue, ch, p);
	    /*
 	     * Pick up the mouse input (if any)
 	     */
	    if ((unit == 1) && (pointer_id == MOUSE_ID))
		vsxxx_mouse_event(wi, wi->queue, wsp,  p, 
				  &wi->last_rep, new_rep, 1);

	    else if ((unit == 1) && (pointer_id == TABLET_ID))
		vsxxx_tablet_event(wi, wi->queue, p,  &wi->last_rep, new_rep, 
			p->position.screen, 1, wsp->sp->x, wsp->sp->y);
	    /*
 	     * If we have proc waiting, and event has happened, wake him up
 	     */
	    if (wi->rsel && (wi->queue->head != wi->queue->tail)) {
		selwakeup (wi->rsel, 0);
		wi->rsel = 0;
	    } 
#if NOTDEF
	    else if (wi->queue->head != wi->queue->tail) ttwakeup(tp);
#endif
/* is this what he really means??? JH XXX
 *   or does he mean
 *
 *	    else if (tp->r_rsel && wi->queue->head != wi->queue->tail)
 *	    	ttwakeup(tp);
 *
 * if we want other serial lines to come through here, then we need the
 * ttwakeup call.  And if we do that we also needed to call the line disciple
 * input routine from here. We don't so I'll assume not. XXX
 */
	}
	else  { 
	/*
	 * If the graphic device is not turned on, this is console input
	 * Get a character from the keyboard.
	 */
	if (unit == 0) {
	    	data = ch & 0xff;

	    /*
 	     * Check for various keyboard errors
 	     */
	    if (data == LK_POWER_UP) {
 			(*(wsd->p.kp->reset_keyboard))(wsd->p.kp->kc);
			return;
	    }
	    if (data == LK_POWER_ERROR ||
		data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR)  {
			mprintf("ws: Keyboard error, code = %x\n",data);
			return;
	    }
	    if (data < LK_LOWEST || data == LK_MODE_CHANGE) return;

	    /*
 	     * See if its a state change key
 	     */

	    switch (data)  {
		    case LOCK:
			kbd->lock ^= 0xffff;	/* toggle */
			if (kbd->lock)
				(*(kbd->lk_putc)) (LK_LED_ENABLE);
			else
				(*(kbd->lk_putc)) (LK_LED_DISABLE);
			(*(kbd->lk_putc)) (LED_3);
			return;
		    case SHIFT_RIGHT:
		    case SHIFT:
			kbd->shift ^= 0xffff;
			return;	
		    case CNTRL:
			kbd->cntrl ^= 0xffff;
			return;
		    case ALLUP:
			kbd->cntrl = kbd->shift = 0;
			return;
		    case REPEAT:
			c = kbd->last;
			break;
		    case HOLD:
			/*
 			 * "Hold Screen" key was pressed, we treat it as 
			 *  if ^s or ^q was typed.  
			 */
			if (kbd->hold == 0) {
			    if ((tp->t_state & TS_TTSTOP) == 0)  {
			    	c = q_key[CHAR_S];
			    	(*(kbd->lk_putc)) (LK_LED_ENABLE);
			    	(*(kbd->lk_putc)) (LED_4);
				kbd->hold = 1;
			    } 
			    else c = q_key[CHAR_Q];
			}
			else {
			    c = q_key[CHAR_Q];
			    (*(kbd->lk_putc)) (LK_LED_DISABLE);
			    (*(kbd->lk_putc)) (LED_4);
			    kbd->hold = 0;
			}
			if (c >= ' ' && c <= '~')
			    c &= 0x1f;
			else if (c >= 0xA1 && c <= 0xFE)
			    c &= 0x9F;
		    	(*linesw[tp->t_line].l_rint)(c, tp);
			return;

		    default:

			/*
 			 * Test for control characters. If set, see if the 
			 * character is elligible to become a control 
			 * character.
 			 */
			if (kbd->cntrl) {
			    c = q_key[data];
			    if (c >= ' ' && c <= '~') c &= 0x1f;
			} 
			else if (kbd->lock || kbd->shift)
				    c = q_shift_key[data];
				else
				    c = q_key[data];
			break;	

	    }

	    kbd->last = c;

	    /*
 	     * Check for special function keys
 	     */
	    if (c & 0x100) {
		register char *string;

		string = q_special[c & 0x7f];
		while (*string)
		    (*linesw[tp->t_line].l_rint)(*string++, tp);
	    } 
	    else {
		if (tp->t_iflag & ISTRIP)	/* Strip to 7 bits. */
			c &= 0177;	
		else {			/* Take the full 8-bits */
			/*
			 * If ISTRIP is not set a valid character of 377
		 	 * is read as 0377,0377 to avoid ambiguity with
		 	 * the PARMARK sequence.
		 	 */ 
			if ((c == 0377) && (tp->t_line == TERMIODISC) &&
			    (tp->t_iflag & PARMRK))
				(*linesw[tp->t_line].l_rint)(0377,tp);
		
		}
	        (*linesw[tp->t_line].l_rint)(c, tp);
	    }
	    if (kbd->hold &&((tp->t_state & TS_TTSTOP) == 0))  {
		    (*(kbd->lk_putc)) (LK_LED_DISABLE);
		    (*(kbd->lk_putc)) (LED_4);
		    kbd->hold = 0;
	    }
	}
    }
    return;

} 

#ifdef NOTDEF
ws_wakeup_any_pending(queue)
	ws_event_queue *queue;
{
  	register ws_info *wi = &ws_softc[0];
	/*
 	 * If we have proc waiting, and event has happened, wake him up
 	 */
	if (wi->rsel && (wi->queue->head != wi->queue->tail)) {
		selwakeup (wi->rsel, 0);
		wi->rsel = 0;
	}
}
#endif

/* don't want to have to pass in queue,
	besides, we never used it!
*/
ws_wakeup_any_pending()
{
  	register ws_info *wi = &ws_softc[0];
	/*
 	 * If we have proc waiting, and event has happened, wake him up
 	 */
	if (wi->rsel && (wi->queue->head != wi->queue->tail)) {
		selwakeup (wi->rsel, 0);
		wi->rsel = 0;
	}
}

/*
 * v_consputc is the switch that is used to redirect the console dcputc to the
 * virtual console vputc.
 * v_consgetc is the switch that is used to redirect the console getchar to the
 * virtual console vgetc.
 *
 * Routine to initialize virtual console. This routine sets up the 
 * graphic device so that it can be used as the system console. It
 * is invoked before autoconfig and has to do everything necessary
 * to allow the device to serve as the system console.           
 *
 */

extern 	int (*v_consgetc)();
extern 	int (*v_consputc)();
extern	int (*vs_gdopen)();
extern	int (*vs_gdclose)();
extern	int (*vs_gdselect)();
extern	int (*vs_gdioctl)();
extern	int (*vs_gdstop)();
extern  int (*vs_gdkint)();

extern lk201_getc();

/*
 * Routine to do the board specific setup.
 *
 */

ws_cons_init()
{
	register ws_info *wi = &ws_softc[0];
	register ws_screens *wsp = &screens[wi->ws.console_screen];
	int mouse_getc(), mouse_putc();
	register ws_screen_descriptor *screen = wsp->sp;
	register ws_devices *wsd = &devices[wi->ws.console_keyboard];

	/*
 	 * Set the line parameters on SLU line for
 	 * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
 	 */
	(*slu.kbd_init)();

	(void) ws_define_device(KEYBOARD_DEVICE, &keyboard, sizeof(ws_event));
	(void) ws_define_device(MOUSE_DEVICE, &mouse, sizeof(ws_event));
	/*
 	 * Home the cursor.
	 * We want an LSI terminal emulation.  We want the graphics
	 * terminal to scroll from the bottom. So start at the bottom
 	 */
	screen->row = screen->max_row - 1;
	screen->col = 0;
	
	/* initialize the hardware */
	(*(wsp->f->init_screen))(wsp->f->sc, wsp->sp);
	(*(wsp->cmf->init_color_map))(wsp->cmf->cmc);
	(*(wsp->cf->load_cursor)) (wsp->cf->cc, wsp->sp, &default_cursor);
	(*(wsd->p.kp->reset_keyboard))(wsd->p.kp->kc);
	pointer_id = vsxxx_mouse_init(slu.mouse_getc, slu.mouse_putc);

	/* and take over the console */
	v_consputc = wsputc;
/* XXX should this be using rom console routine?*/
	v_consgetc = lk201_getc; 
	vs_gdopen = wsopen;
	vs_gdkint = wskint;
	vs_gdclose = wsclose;
	vs_gdselect = wsselect;
	vs_gdioctl = wsioctl;
	vs_gdstop = wsstop;
	return (1);
}

wsread() { return EIO; }
wswrite() { return EIO; }
