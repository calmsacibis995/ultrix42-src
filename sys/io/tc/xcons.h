/*
 * @(#)xcons.h	4.1	(ULTRIX)	8/9/90
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
 * xcons.h
 *
 * xcons alternate console driver
 *
 * Modification history
 *
 *   4-Jul-90	Randall Brown
 *		Created file.
 */

extern int xcons_kern_loop;

#define XCONSDEV	0

#define XCONS_CLOSED	1
#define XCONS_BLOCKED	2
#define XCONS_OK	3
