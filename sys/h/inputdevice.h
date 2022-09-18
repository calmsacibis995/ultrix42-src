/*
 * @(#)inputdevice.h	4.1	(ULTRIX)	8/13/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987, 1989 by		*
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
 * ioctl assignments must not conflict with workstation.h
 */
/* from XI.h */  

#define WS_DvAccelNum		(1L << 0)
#define WS_DvAccelDenom		(1L << 1)
#define WS_DvThreshold		(1L << 2)

#define WS_DvKeyClickPercent 	(1L << 0)
#define WS_DvKey		(1L << 1)
#define WS_DvAutoRepeatMode	(1L << 2)

#define WS_DvLed     		(1L<<0)
#define WS_DvLedMode   		(1L<<1)


#define WS_AddToList		0
#define WS_DeleteFromList	1

#define WS_DvString		(1L << 0)

#define WS_DvInteger		(1L << 0)



#define WS_KeyClass		0
#define WS_ButtonClass		1
#define WS_ValuatorClass	2
#define WS_FeedbackClass	3
#define WS_ProximityClass	4
#define WS_FocusClass		5
#define WS_OtherClass		6

#define WS_KbdFeedbackClass  	0
#define WS_PtrFeedbackClass  	1
#define WS_StringFeedbackClass	2
#define WS_IntegerFeedbackClass	3
#define WS_LedFeedbackClass  	4
#define WS_BellFeedbackClass  	5

/*#define DEVICE_ON _IO(q, 20)		/* turn on the specified device	*/
/*#define DEVICE_OFF _IO(q, 21)		/* turn off the specified device*/


/* XXX the event stuff still needs work. */

/* type field */
#define BUTTON_UP_TYPE          0
#define BUTTON_DOWN_TYPE        1
#define BUTTON_RAW_TYPE         2
#define MOTION_TYPE             3
#define PROXIMITY_IN		4
#define PROXIMITY_OUT		5


/* device type field */
#define NULL_DEVICE		0	/* NULL event (for QD_GETEVENT ret) */
#define MOUSE_DEVICE		1	/* mouse */
#define KEYBOARD_DEVICE		2	/* main keyboard */
#define TABLET_DEVICE		3	/* graphics tablet */
#define TOUCHSCREEN_DEVICE	4	/* touchscreen */
#define CONSOLE_DEVICE		5	/* console */
#define BUTTONBOX_DEVICE	6
#define BARCODE_DEVICE		7
#define KNOBBOX_DEVICE		8
#define JOYSTICK_DEVICE		9
#define TRACKBALL_DEVICE	10
#define QUADRATURE_DEVICE	11
#define SPACEBALL_DEVICE	12
#define DATAGLOVE_DEVICE	13
#define EYETRACKER_DEVICE	14
#define CURSORKEYS_DEVICE	15
#define FOOTMOUSE_DEVICE	16
#define ID_MODULE_DEVICE	17
#define ONE_KNOB_DEVICE		18
#define NINE_KNOB_DEVICE	19
/* stand-alone graphics tablet, does not pretend to be mouse */
#define STABLET_DEVICE		20

/* all device related IOCTL's must have device_number as first element */
typedef struct {
	short device_number;
} ws_device_ioctl;

typedef struct {
	short screen;
	short device_number;
} ws_screen_and_device_ioctl;

typedef short axis_datum;

typedef struct {
        EQTime		time;		/* 1 millisecond units 		*/
	unsigned char   screen;		/* which screen the event was on*/
        unsigned char   device;		/* which device of workstation 	*/
	unsigned char	device_type;	/* which type of device		*/
        unsigned char   type;		/* button up/down/raw or motion */
	union {
	    struct { 
		unsigned char   key;	/* the key (type == button only)*/
		unsigned char	pad;	/* padding. 			*/
		short x;
		short y;
	     } key;
	    struct {
		unsigned char button;	/* which button was pressed	*/
		unsigned char	pad;	/* padding. 			*/
		short x;
		short y;
	     } button;
	    struct { 
		unsigned char buttons;
		unsigned char	pad;	/* padding. 			*/
		short x;
		short y;
	     } pointer;
	} e;
} ws_event; 

typedef struct {
	EQTime time;
	short device;			/* which device the motion is from */
	short screen;			/* which screen it is on 	*/
	short axis[2];			/* Must be multiple of two.	*/
} ws_motion_history;

typedef struct {
	ws_motion_history *motion;	/* history of pointer motions 	*/
	short	size;			/* number of entries		*/
	short	axis_count;		/* number of axis in an entry	*/
	short	entry_size;		/* size of entry in buffer	*/
	short	next;			/* older entries simply overwritten */
} ws_motion_buffer;

/* 
 * Event queue definition.
 * The queue is a circular list, with head and tail indicies.
 * since they are manipulated independently by the server and the driver,
 * atomicity of memory writes provides the appropriate synchronization.
 * In the new interface, the size of event is determined at autoconfiguration
 * time and is the maximum size required by any input device.  This will
 * allow new input devices to be added without requiring interface 
 * changes.
 *
 * So a program can tell if events are to be processed by the head and
 * tail indicies being non-equal, and process events until they are.
 */

typedef struct {
	EQTime	time;			/* good enuf for screen saver	*/
	ws_event *events;		/* where the events are		*/
	int 	size;			/* number of entries in the queue */
        volatile int head;		/* head of circular list	*/
        volatile int tail;		/* tail of circular list	*/
	int	event_size;		/* size of each event in queue  */
	ws_motion_buffer *mb;		/* global motion history buffer */
} ws_event_queue;
#define GET_AND_MAP_EVENT_QUEUE _IOR('w', 20, ws_event_queue *)

#define MOTION_BUFFER_SIZE 100


typedef struct {
	short screen;		/* which screen 			*/
	short device_number;
	short enable;		/* non-zero, then enable 		*/
	ws_screen_box box;	/* the box itself			*/
} ws_pointer_box;

#define SET_ESCAPE_BOX _IOW('w',  (21|IOC_S|IOC_D), ws_pointer_box)
#define SET_POINTER_BOX _IOW('w', (22|IOC_S|IOC_D), ws_pointer_box)

typedef struct {
	short screen;		/* which screen to move pointer to */
	short device_number;
        short x;
        short y;
} ws_pointer_position;

#define SET_POINTER_POSITION	_IOW('w', (23|IOC_S|IOC_D), ws_pointer_position)

typedef struct {
	short	device_number;		/* which device to set		*/
	short	numerator;		/* accelerate cursor by this	*/
	short	denominator;		/* ratio, when greater than	*/
	short	threshold;		/* the threshold		*/
} ws_pointer_control;

#define SET_POINTER_CONTROL _IOW('w', (24 | IOC_D), ws_pointer_control)
#define GET_POINTER_CONTROL _IOWR('w', (25 | IOC_D), ws_pointer_control)
	
/* device reporting mode */
#define WS_Relative		0
#define WS_Absolute		1

typedef struct {
	short	device_number;		/* which device			*/
	short 	mode;			/* either absolute or relative  */
} ws_device_mode;

#define SET_DEVICE_MODE _IOW('w', (27|IOC_D), ws_device_mode)
#define GET_DEVICE_MODE _IOWR('w', (28|IOC_D), ws_device_mode)

#define LK_201 0	/* Current keyboard				*/
			/* LK301 bit the dust before customers saw it.	*/
#define LK_401 2	/* New keyboard, first seen on VS1000		*/

#define VSXXX  0	/* "hockey puck" mouse				*/
/* XXX what is tablet name? */
#define CSS_DEVICE_TYPE	128		/* reserved to CSS bit		*/
#define CUSTOMER_DEVICE_TYPE 256	/* reserved to customers bit	*/

typedef struct {
	short	device_number;		/* which device			*/
	short	hardware_type;
} ws_hardware_type;

#define GET_DEVICE_TYPE _IOWR('w', (29|IOC_D), ws_hardware_type)

/* masks for ChangeKeyboardControl */

#define WSKBKeyClickPercent	(1L<<0)
#define WSKBBellPercent		(1L<<1)
#define WSKBBellPitch		(1L<<2)
#define WSKBBellDuration	(1L<<3)
#define WSKBLed			(1L<<4)
#define WSKBLedMode		(1L<<5)
#define WSKBAutoRepeatMode	(1L<<7)
#define WSKBAutoRepeats		(1L<<8)


typedef struct {
	short device_number;		/* which device			*/
	short flags;			/* which things to set from list*/
	short click, bell, bell_pitch, bell_duration;
	short auto_repeat;
	unsigned int autorepeats[8];
	unsigned int leds;
} ws_keyboard_control;

#define SET_KEYBOARD_CONTROL _IOW('w', (30|IOC_D), ws_keyboard_control)
#define GET_KEYBOARD_CONTROL _IOWR('w', (31|IOC_D), ws_keyboard_control)
#define RING_KEYBOARD_BELL _IOW('w', (32|IOC_D), short)

/* 
 * min and max keycode can be devined from keysyms returned; more reliable
 * (less prone to operator error), so we'll do it this way.
 */
typedef struct {
	short device_number;		/* which keyboard to query	*/
	short keysyms_per_keycode;	/* how many keysyms on a single key */
	short keysyms_present;		/* number of keysyms on keyboard */
	short modifier_keycode_count;	/* number of keycodes with modifiers */
	short lock_key_led;		/* which LED should lock key be on */
} ws_keyboard_definition;

#define GET_KEYBOARD_DEFINITION _IOWR('w', (33|IOC_D), ws_keyboard_definition)

typedef struct {
	unsigned char keycode;
	unsigned char modbits;
} ws_keycode_modifiers;

typedef struct {
	short device_number;		/* which keyboard to query	*/
	ws_keycode_modifiers *modifiers;/* must be enough space!	*/
	unsigned int *keysyms;		/* must be enough space!	*/
	unsigned char *keycodes;	/* must be enough space!	*/
} ws_keysyms_and_modifiers;

#define GET_KEYSYMS_AND_MODIFIERS \
	_IOW('w', (34|IOC_D), ws_keysyms_and_modifiers)
