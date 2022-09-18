/*
 * 	@(#)smreg.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University	of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:
 *
 *
 * 14-Jul-88 -- vasudev (Vasudev K. Bhandarkar)
 *      Clean up irrelevant comments.  And match 4.1.1.12 version
 *      of sm.c.  No code changes.  Yes, really.
 *
 * 14-Aug-86  -- refiey (Ali Rafieymehr)
 *	Tablet support.
 *
 *  5-Aug-86  -- rafiey (Ali Rafieymehr)
 *	Changes for real VAXstar bitmap graphics driver.
 *
 * 18-Jun-86  -- rafiey (Ali Rafieymehr)
 *	Created this header file for the VAXstar monochrome display driver.
 *	Derived from qvreg.h.
 *
 **********************************************************************/

/*
 * VAXstar Monochrome definitions.
 */

#define	SELF_TEST	'T'
#define	INCREMENTAL	'R'
#define	PROMPT		'D'

#define	MOUSE_ID	0x2
#define TABLET_ID	0x4

#define START_FRAME	0x80		/* start of report frame bit */
#define X_SIGN		0x10		/* sign bit for X */
#define Y_SIGN		0x08		/* sign bit for Y */

#define	XOFFSET		216
#define	YOFFSET		 33

#define	UPDATE_POS	0x01

/*
 * VAXstar interrupt controller register bits
 *
 */

#define SINT_VF		010

/*
 * Cursor Command Register bits
 *
 */

#define	ENPA	0000001
#define	FOPA	0000002
#define	ENPB	0000004
#define	FOPB	0000010
#define	XHAIR 	0000020
#define	XHCLP	0000040
#define	XHCL1	0000100
#define	XHWID	0000200
#define	ENRG1	0000400
#define	FORG1	0001000
#define	ENRG2	0002000
#define	FORG2	0004000
#define	LODSA	0010000
#define	VBHI	0020000
#define	HSHI	0040000
#define	TEST	0100000


/*
 * Line Prameter Register bits
 *
 */

#define	SER_KBD      000000
#define	SER_POINTER  000001
#define	SER_COMLINE  000002
#define	SER_PRINTER  000003
#define	SER_CHARW    000030
#define	SER_STOP     000040
#define	SER_PARENB   000100
#define	SER_ODDPAR   000200
#define	SER_SPEED    006000
#define	SER_RXENAB   010000

#define EVENT_LEFT_BUTTON	1
#define EVENT_MIDDLE_BUTTON	2
#define EVENT_RIGHT_BUTTON	3

/* Mouse buttons */

#define RIGHT_BUTTON	0x01
#define MIDDLE_BUTTON	0x02
#define LEFT_BUTTON	0x04

/*
 * Mouse definitions
 *
 */

#define MOTION_BUFFER_SIZE 100
#define	SELF_TEST	'T'

/* mouse report structure definition */

	struct mouse_report {

	    char state;		/* buttons and sign bits */
	    short dx;		/* delta X since last change */
	    short dy;		/* delta Y since last change */
	    char bytcnt;	/* mouse report byte count */
	};


#define EVENT_T_LEFT_BUTTON	0x00
#define EVENT_T_FRONT_BUTTON	0x01
#define EVENT_T_RIGHT_BUTTON	0x02
#define EVENT_T_BACK_BUTTON	0x04

/* puck buttons */

#define T_LEFT_BUTTON		0x02
#define T_FRONT_BUTTON		0x04
#define T_RIGHT_BUTTON		0x08
#define T_BACK_BUTTON		0x10

/*
 * Lk201 keyboard 
 */

#define LK_UPDOWN 	0x86		/* bits for setting lk201 modes */
#define LK_AUTODOWN 	0x82
#define LK_DOWN 	0x80
#define LK_DEFAULTS 	0xd3		/* reset (some) default settings */
#define LK_AR_ENABLE 	0xe3		/* global auto repeat enable */
#define LK_CL_ENABLE 	0x1b		/* keyclick enable */
#define LK_KBD_ENABLE 	0x8b		/* keyboard enable */
#define LK_BELL_ENABLE 	0x23		/* the bell */
#define LK_LED_ENABLE 	0x13		/* light led */
#define LK_LED_DISABLE 	0x11		/* turn off led */
#define LK_RING_BELL 	0xa7		/* ring keyboard bell */
#define LED_1 		0x81		/* led bits */
#define LED_2 		0x82
#define LED_3 		0x84
#define LED_4 		0x88
#define LED_ALL 	0x8f
#define LK_KDOWN_ERROR	0x3d		/* key down on powerup error */
#define LK_POWER_ERROR 	0x3e		/* keyboard failure on powerup test */
#define LK_OUTPUT_ERROR	0xb5		/* keystrokes lost during inhibit */
#define LK_INPUT_ERROR 	0xb6		/* garbage command to keyboard */
#define LK_LOWEST	0x56		/* lowest significant keycode */
#define LK_DIV6_START	0xad		/* start of div 6 */
#define LK_DIV5_END	0xb2		/* end of div 5 */

/*
 * Keycodes for special keys and functions
 */

#define SHIFT	0xae
#define LOCK	0xb0
#define REPEAT	0xb4
#define CNTRL	0xaf
#define ALLUP	0xb3
#define	HOLD	0x56

