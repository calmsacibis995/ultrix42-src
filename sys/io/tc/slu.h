/*
 * @(#)slu.h	4.1	(ULTRIX)	8/9/90
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

struct slu {
    int	(*mouse_init)();
    int	(*mouse_putc)();
    int (*mouse_getc)();
    int	(*kbd_init)();
    int	(*kbd_putc)();
    int (*kbd_getc)();
    int (*slu_putc)();		/* put char out serial line unit */
    struct tty *slu_tty;
};

extern struct slu slu;
