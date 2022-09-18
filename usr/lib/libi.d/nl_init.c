/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
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

#ifndef lint
static char Sccsid[] = "@(#)nl_init.c	4.1	(ULTRIX)	7/2/90";
#endif

#include "i_defs.h"
#include "locale.h"

/*
 * nl_init -- initialise internationalisation operation
 *
 * SYNOPSIS:
 *	nl_init(lang)
 *	char *lang;
 *
 * DESCRIPTION:
 *	This function is found in the X/OPEN guide, section on nl_init(3C).
 *	This is simply mapped onto the ANSI setlocale() call, taking care
 *	to map the return values.
 *
 * RETURN:
 *	0   if successful
 *	-1   otherwise
 */
nl_init(lang)
char *lang;
{
	if (lang == (char *)0 || *lang == '\0')
		return -1;
	return ((setlocale(LC_ALL, lang) == (char *)0) ? -1 : 0);
}
