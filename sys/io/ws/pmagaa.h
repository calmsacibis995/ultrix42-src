/*
 * @(#)pmagaa.h	4.1	(ULTRIX)	8/10/90
 */

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

#define PMAGAA_FB_OFFSET		0x200000
#define PMAGAA_BT455_OFFSET		0x100000
#define PMAGAA_BT431_OFFSET		0x180000
#define PMAGAA_IREQ_OFFSET		0x080000
#define PMAGAA_DIAG_ROM_OFFSET		0x000000

int pmagaa_attach();
void pmagaa_interrupt();
void pmagaa_bt455_enable_interrupt();
void pmagaa_bt431_enable_interrupt();
caddr_t pmagaa_bt431_init_closure();
caddr_t pmagaa_bt455_init_closure();
int pmagaa_recolor_cursor();
int pmagaa_video_on();
int pmagaa_video_off();
