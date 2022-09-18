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
static char Sccsid[] = "@(#)i_getstr.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * i_getstr -- get a string from the specified database
 *
 * SYNOPSIS:
 *	char *
 *	i_getstr(s, strtab)
 *	char *s;
 *	str_tab *strtab;
 *
 * DESCRIPTION:
 *	return a pointer to the string identified by s in strtab.
 *
 * RETURN:
 *	pointer to string if successful,
 *	(char *)0 otherwise, with i_errno set.
 */
char *
i_getstr(s, strtab)
char *s;
str_tab *strtab;
{
	str_tab *low;			/* lower limit for binary search */
	str_tab *high;			/* upper limit for binary search */
	register str_tab *mid;		/* current element considered	 */
	register int c;

	/*
	 * quick check for arguments
	 */
	if (strtab == (str_tab *)0)
	{
		i_errno = I_EISTI;
		return((char *)0);
	}

	/*
	 * use binary search to find string in sorted stringtable
	 */
	low = strtab;
	high = &strtab[(strtab->st_offst / sizeof(str_tab)) - 1];

	while (low <= high)
	{
		mid = low + (high - low) / 2;
		if ((c = strcmp(mid->st_name, s)) == 0)
		{
			return((char *)strtab + mid->st_offst);
		}
		else if (c < 0)
			low = &mid[1];
		else
			high = &mid[-1];
	}

	/* not found */
	i_errno = I_EISTR;
	return((char *)0);
}
