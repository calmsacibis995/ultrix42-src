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
static char Sccsid[] = "@(#)conv.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * toupper -- international version of toupper
 *
 * SYNOPSIS:
 *	int
 *	toupper(c)
 *	int c;
 *
 * DESCRIPTION:
 *	toupper is specified in the X/OPEN guide.
 *
 * RETURNS:
 *	converted code
 */
int
toupper(c)
int c;
{
	int result;

	if (_lc_toupper)
	{
		i_errno = 0;
		result = i_conv((i_char)c, _lc_toupper);
	}
	else
		i_errno = I_EBACC;

	if (i_errno)
		if (c >= 'a' && c <= 'z')
			result = c + 'A' - 'a';
		else
			result = c;

	return(result);
}

/*
 * tolower -- international version of tolower
 *
 * SYNOPSIS:
 *	int
 *	tolower(c)
 *	int c;
 *
 * DESCRIPTION:
 *	tolower is specified in the X/OPEN guide.
 *
 * RETURNS:
 *	converted code
 */
int
tolower(c)
int c;
{
	int result;

	if (_lc_tolower)
	{
		i_errno = 0;
		result = i_conv((i_char)c, _lc_tolower);
	}
	else
		i_errno = I_EBACC;

	if (i_errno)
		if(c >= 'A' && c <= 'Z')
			result = c - 'A' + 'a';
		else
			result = c;
	return(result);
}
