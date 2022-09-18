/*
*	@(#)onepass.h	4.1	(ULTRIX)	7/17/90
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
*
*			Modification History
*
*	David Metsky		15-Jan-86
*
* 001	Added from BSD 4.3 version as part of upgrade.
*
*	Based on:	onepass.h	4.1		85/03/19
*
*************************************************************************/

#ifndef _ONEPASS_
#define	_ONEPASS_
/*
 * Definitions for creating a one-pass
 * version of the compiler.
 */

#ifdef _PASS2_
#define crslab crs2lab
#define where where2
#define xdebug x2debug
#define tdebug t2debug
#define deflab def2lab
#define edebug e2debug
#define eprint e2print
#define getlab get2lab
#define filename ftitle
#endif

/* NOPREF must be defined for use in first pass tree machine */
#define NOPREF	020000		/* no preference for register assignment */

#include "ndu.h"
#endif
