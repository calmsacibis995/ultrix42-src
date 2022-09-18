/*
 *		@(#)ttold.h	4.2	(ULTRIX)	9/4/90
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
/************************************************************************
 *			Modification History				*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Get defininitions from <sgtty.h>				*
 *									*
 ************************************************************************/

/* @(#)ttold.h	6.1 */

/*
 * This header file should only be used by System V things.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/* 
 * Get definition of sgttyb structure, and other things.
 */

#ifdef KERNEL
#include "../h/sgtty.h"
#else
#include <sgtty.h>
#endif

#ifdef __SYSTEM_FIVE
/*
 * Old System V modes
 */
#define	O_HUPCL	  0000001
#define	O_XTABS	  0000002
#define	O_LCASE	  0000004
#define	O_ECHO	  0000010
#define	O_CRMOD	  0000020
#define	O_RAW	  0000040
#define	O_ODDP	  0000100
#define	O_EVENP	  0000200
#define	O_NLDELAY 0001400
#define	O_NL1	  0000400
#define	O_NL2	  0001000
#define	O_TBDELAY 0002000
#define	O_NOAL	  0004000
#define	O_CRDELAY 0030000
#define	O_CR1	  0010000
#define	O_CR2	  0020000
#define	O_VTDELAY 0040000
#define	O_BSDELAY 0100000
#endif /* __SYSTEM_FIVE */
