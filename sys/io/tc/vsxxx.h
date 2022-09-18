/*
 * @(#)vsxxx.h	4.1	(ULTRIX)	8/9/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988,89 by			*
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
 ************************************************************************
 *
 *
 * Modification history
 *
 *   4-Jul-90	Randall Brown
 *		Created file.
 */

/* Definitions for mouse buttons */
#define EVENT_LEFT_BUTTON	0x01
#define EVENT_MIDDLE_BUTTON	0x02
#define EVENT_RIGHT_BUTTON	0x03
#define RIGHT_BUTTON		0x01
#define MIDDLE_BUTTON		0x02
#define LEFT_BUTTON		0x04

/* Mouse definitions */
#define MOTION_BUFFER_SIZE 100
#define SELF_TEST	'T'
#define INCREMENTAL	'R'
#define PROMPT		'D'

#define MOUSE_ID	0x2
#define TABLET_ID	0x4

#define START_FRAME	0x80		/* start of report frame bit	*/
#define X_SIGN		0x10		/* sign bit for X		*/
#define Y_SIGN		0x08		/* sign bit for Y		*/

#define UPDATE_POS	0x01

/* Mouse report structure definition */
struct mouse_report {
	char state;			/* buttons and sign bits	*/
	short dx;			/* delta X since last change	*/
	short dy;			/* delta Y since last change	*/
	char bytcnt;			/* mouse report byte count	*/
};

