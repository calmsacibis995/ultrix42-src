/*
 * 	@(#)smevent.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
*									*
*			Copyright (c) 1985 by				*
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

/*
 * Event queue entries
 */

# ifndef _SMEVENT_
# define _SMEVENT_

typedef struct  _event {
        short	        x;		/* x position */
        short 	        y;		/* y position */
        unsigned int    time;		/* 1 millisecond units */
        unsigned char   type;		/* button up/down/raw or motion */
        unsigned char   key;		/* the key (button only) */
        unsigned char   index;		/* which instance of device */
        unsigned char   device;		/* which device */
} smEvent;

/* type field */
#define BUTTON_UP_TYPE          0
#define BUTTON_DOWN_TYPE        1
#define BUTTON_RAW_TYPE         2
#define MOTION_TYPE             3

/* device field */
#define NULL_DEVICE	  0		/* NULL event (for QD_GETEVENT ret) */
#define MOUSE_DEVICE	  1		/* mouse */
#define KEYBOARD_DEVICE	  2		/* main keyboard */
#define TABLET_DEVICE	  3		/* graphics tablet */
#define AUX_DEVICE	  4		/* auxiliary */
#define CONSOLE_DEVICE	  5		/* console */
#define KNOB_DEVICE	  8
#define JOYSTICK_DEVICE	  9

typedef struct _timecoord {
	unsigned int	time;
	short		x, y;
} smTimeCoord;

/*
 * The event queue. This structure is normally included in the info
 * returned by the device driver.
 */

typedef struct _eventqueue {
	smEvent 	*events;
	unsigned int 	eSize;
        unsigned int    eHead;
        unsigned int    eTail;
	unsigned	long	timestamp_ms;
	smTimeCoord	*tcs;	/* history of pointer motions */
	unsigned int	tcSize;
	unsigned int	tcNext;	/* simple ring buffer, old events are tossed */
} smEventQueue;

/* mouse cursor position */

typedef struct _cursor {
        short x;
        short y;
} smCursor;

/* mouse motion rectangle */

typedef struct _box {
        short bottom;
        short right;
        short left;
        short top;
} smBox;

# endif _SMEVENT_
