#ifndef lint
static char *sccsid = "@(#)assert.c	4.1	ULTRIX	7/2/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  25/04/88 -- thoms
 * date and time created 88/04/25 18:59:38 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  10/07/88 -- thoms
 * added copyright notice and modification history
 *
 * SCCS history end
 */

#include <stdio.h>
_assert(cond, file, line)
     char *cond, *file;
     int line;
{
	fprintf(stderr, "Assertion failed (%s) in %s at line %d\n",
		cond, file, line);
	abort(1);
}
