#ifndef lint
static char *sccsid = "@(#)lk201.c	4.2      (ULTRIX)  12/6/90";
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
 ************************************************************************

/*
 * support for lk201 and successor keyboards.
 * written by J. Gettys, from cfb original
 */
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

/*
 * Keyboard translation and font tables
 */
#define CHAR_S	0xc7
#define CHAR_Q	0xc1

extern  char *q_special[],q_font[];
/* XXX should use keysm data for dumb keyboard, as it defaults to ascii*/
extern  u_short q_key[],q_shift_key[];

/* divisions that do not default correctly need to be fixed. */
struct lk_divdefaults {
	unsigned char division;
	unsigned char command;
} lk_divdefaults[] = {
	{ 5,  LK_AUTODOWN },
	{ 9,  LK_AUTODOWN },
	{ 10, LK_AUTODOWN },
	{ 11, LK_AUTODOWN },
	{ 12, LK_AUTODOWN },
	{ 13, LK_AUTODOWN },
};
	
#define KBD_INIT_DEFAULTS sizeof(lk_divdefaults)/sizeof(struct lk_divdefaults)

/* XXX this has to go into softc structure... */
extern struct lk201info lk201_softc[];

short lk201_kbdinitstring[] = {		/* reset any random keyboard stuff */
	LK_CL_ENABLE,			/* keyclick */
	0x84,				/* keyclick volume */
	LK_KBD_ENABLE,			/* the keyboard itself */
/*	LK_BELL_ENABLE,			/* keyboard bell */
/*	0x84,				/* bell volume */
	LK_LED_DISABLE,			/* keyboard leds */
	LED_ALL };
#define KBD_INIT_LENGTH	sizeof(lk201_kbdinitstring)/sizeof(short)

/*
 * Routine to get a character from LK201.
 */

lk201_getc(data)
	u_short	data;
{
	int	c;
	struct lk201info *lp = &lk201_softc[0];
	/*
         * Get a character from the keyboard,
         */

loop:
        /*
         * Check for various keyboard errors
         */

	if (data == LK_POWER_UP) {
		lk201_reset_keyboard(lp);
		return;
	}
	if (data == LK_POWER_ERROR || data == LK_INPUT_ERROR 
	 			   || data == LK_OUTPUT_ERROR) 	{
		mprintf(" Keyboard error, code = %x\n",data);
		return(0);
	}
	if (data < LK_LOWEST) return(0);
        /*
         * See if its a state change key
         */
	switch (data) {
	case LOCK:
		lp->lock ^= 0xffff;	/* toggle */
		if (lp->lock)
			lk201_putc(LK_LED_ENABLE);
		else
			lk201_putc(LK_LED_DISABLE);
		lk201_putc(LED_3);
		data = (*slu.kbd_getc)();
		goto loop;
	case SHIFT_RIGHT:
	case SHIFT:
		lp->shift ^= 0xffff;
		data = (*slu.kbd_getc)();
		goto loop;
	case CNTRL:
		lp->cntrl ^= 0xffff;
		data = (*slu.kbd_getc)();
		goto loop;
	case ALLUP:
		lp->cntrl = lp->shift = 0;
		data = (*slu.kbd_getc)();
		goto loop;
	case REPEAT:
		c = lp->last;
		break;
	default:
                /*
                 * Test for control characters. If set, see if the character
                 * is elligible to become a control character.
                 */
		if (lp->cntrl) {
		    c = q_key[data];
		    if (c >= ' ' && c <= '~')
			c &= 0x1f;
		} 
		else if (lp->lock || lp->shift)
			    c = q_shift_key[data];
		       else
			    c = q_key[data];
		break;	
	}
	lp->last = c;

        /*
         * Check for special function keys
         */
	if (c & 0x80)
	    return (0);
	else
	    return (c);
}

caddr_t lk201_init_closure (address)
	caddr_t address;
{
	return address;
}
/*
 *
 * Use the console line drivers putc routine
 * Should be in cfb module....
 */

lk201_putc( c )
char c;
{
    /* send the char to the keyboard using line 0 of the serial line driver */
    (*slu.kbd_putc)(c);
}

void lk201_up_down_mode(lp)
	struct lk201info *lp;
{
	register int i;
	if (lp->inkbdreset == 1) return;
	lp->inkbdreset = 1;
	(*lp->lk_putc)(LK_DEFAULTS);
	(*lp->lk_putc)(LK_ENABLE_401);
	for (i = 1; i < 15; i++) {
		DELAY(10000);
		(*lp->lk_putc) (LK_UPDOWN | (i << 3));
	}
	lp->inkbdreset = 0;
	lp->up_down_mode = 1;	/* now in up/down mode */
}

/*
 * reset an LK201.
 */
void lk201_reset_keyboard(lp)
	struct lk201info *lp;
{
	register int i;
	if (lp->inkbdreset == 1) return;
	lp->inkbdreset = 1;
	lp->up_down_mode = 0;	/* now in regular mode */
	(*lp->lk_putc)(LK_DEFAULTS);

	for (i = 0; i < KBD_INIT_DEFAULTS; i++) {
		(*lp->lk_putc) (lk_divdefaults[i].command | 
			(lk_divdefaults[i].division << 3));
		DELAY(1000);
	}
	for (i = 0; i < KBD_INIT_LENGTH; i++)
		(*lp->lk_putc) (lk201_kbdinitstring[i]);
	lp->inkbdreset = 0;
}

#define MAXSPECIAL	0xba	
#define BASEKEY		0x41
#define MINSPECIAL	0xb3
#define IsBitOn(ptr, bit) (((unsigned int *) (ptr))[(bit)>>5] & (1 << ((bit) & 0x1f)))
lk201_keyboard_event(closure, queue, ch, p)
	caddr_t closure;
	ws_event_queue *queue;
	register int ch;
	ws_pointer *p;
{
	struct lk201info *lp = (struct lk201info *)closure;
	register unsigned int key, bits, idx;
	int type;

	key = ch = ch & 0xff;
	if (key > MAXSPECIAL || (key >= BASEKEY && key < MINSPECIAL))  {
		idx = key >> 5;
		key &= 0x1f;
		key = 1 << key;
		    if ((lp->keys[idx] ^= key) & key) {
			register ws_keyboard_control *kc = &lp->kp->control;
			lp->last = ch;
			if (kc->auto_repeat && IsBitOn (kc->autorepeats, ch)) {
			    if (lp->repeating) {
				untimeout(lk201_autorepeat, lp);
			    }
			    timeout(lk201_autorepeat, lp, lp->timeout);
			    lp->repeating = 1;
			}
			lp->p = p;
			lp->queue = queue;
			type = BUTTON_DOWN_TYPE;
		    }
		    else {
			type = BUTTON_UP_TYPE;
			idx = 7;
			do {	/* last better be sensible */
			    if (bits = lp->keys[idx]) {
				lp->last = (idx << 5) | (ffs(bits) - 1);
				break;
			    }
			} while (--idx >= 0);
		    }
		    done:
		    ws_enter_keyboard_event(queue, ch, p, type);
	}
	else {
		untimeout(lk201_autorepeat, lp);
		lp->repeating = 0;
		switch (key)	{
		    case REPEAT: 
		  	mprintf("metronome error\n");
			break;
		    case ALLUP: 
			idx = 7;
			type = BUTTON_UP_TYPE;
			do {
			    if (bits = lp->keys[idx])   {
				lp->keys[idx] = 0;
				key = 0;
				do {
				    if (bits & 1)   {
				        ws_enter_keyboard_event
					   (queue, ((idx << 5)| key), p, type);
				    }
				    key++;
				} while (bits >>= 1);
			    }
			} while (--idx >= 0);
			break;
		    case LK_POWER_UP:
		        lk201_up_down_mode(lp);
			break;
		    case LK_INPUT_ERROR:
		    case LK_OUTPUT_ERROR:
		    case LK_POWER_ERROR: 
			    mprintf("\nlk201: keyboard error,code=%x",key);
		        return(0);
			break;
		}
	}
}

void lk201_autorepeat (closure)
	caddr_t closure;
{
	register struct lk201info *lp = (struct lk201info *)closure;
	register ws_keyboard_control *kc = &lp->kp->control;
	if (kc->auto_repeat  && IsBitOn (kc->autorepeats, lp->last)) {
			timeout(lk201_autorepeat, lp, lp->interval);
		(*lp->lk_putc)(LK_CL_SOUND);	
		/*
		 * DIX converts a single repeated down to a up/down pair, so
		 * why do twice as much work as needed in the kernel?
		 */
		ws_enter_keyboard_event(lp->queue, lp->last, lp->p, 
			BUTTON_DOWN_TYPE);
		ws_wakeup_any_pending(lp->queue);
	}
	else lp->repeating = 0;
}
void lk201_ring_bell (closure) 
	caddr_t closure;
{	
	register struct lk201info *lp = (struct lk201info *)closure;
	(*lp->lk_putc)(LK_RING_BELL);	
	return;
}

int lk201_set_keyboard_control (closure, kp, wskp)
	caddr_t closure;
	ws_keyboard *kp;
	ws_keyboard_control *wskp;
{
	register struct lk201info *lp = (struct lk201info *)closure;
	register int flags = wskp->flags;
	if (flags & WSKBKeyClickPercent) {
		kp->control.click = wskp->click;
		if (wskp->click == 0) 	(*lp->lk_putc)(LK_CL_DISABLE);	
		else {
			register int volume;
			volume = (7 - ((wskp->click / 14) & 7)) | 0x80;
			(*lp->lk_putc)(LK_CL_ENABLE);
			(*lp->lk_putc)(volume);
		}
	}
	/* can't deal with bell pitch or duration */
	if (flags & WSKBBellPercent) {
		register int loud;
		kp->control.bell = wskp->bell;
		if (kp->control.bell != 0) {
			loud = 7 - ((kp->control.bell / 14) & 7) | 0x80;
			(*lp->lk_putc)(LK_BELL_ENABLE);
			(*lp->lk_putc)(loud);
		}
		else (*lp->lk_putc)(LK_BELL_DISABLE);
	}
	if (flags & WSKBAutoRepeatMode) {
		kp->control.auto_repeat = wskp->auto_repeat;
	}
	if (flags & WSKBAutoRepeats) {
		bcopy(wskp->autorepeats, kp->control.autorepeats, 32);
	}
	if (flags & WSKBLed) {
		kp->control.leds = wskp->leds;
		(*lp->lk_putc)(LK_LED_DISABLE);
		(*lp->lk_putc)(LED_ALL);
		(*lp->lk_putc)(LK_LED_ENABLE);
		(*lp->lk_putc)((kp->control.leds & 0xf) | 0x80);
	}
	return 0;
}
