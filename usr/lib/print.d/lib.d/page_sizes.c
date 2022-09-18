#ifndef lint
static char *sccsid = "@(#)page_sizes.c	4.1      ULTRIX 7/2/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/*
 * page_sizes.h -- header for table of page sizes
 *
 * Description:
 *	This table of page sizes is used to generate the
 *	sizes in the item_list which is passed across the
 *	internal calling interface to the regis or tek4014
 *	translator.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  31/05/88 -- thoms
 * date and time created 88/05/31 19:54:59 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  19/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * SCCS history end
 */


#include "page_sizes.h"

static PAGE_SIZE page_size[] = {
	{ "a",		612,	792 },
	{ "a3",		842,	1191 },
	{ "a4",		595,	842 },
	{ "a5",		420,	595 },
	{ "b",		792,	1224 },
	{ "b4",		709,	1001 },
	{ "b5",		499,	709 },
	{ "executive",	540,	756 },
	{ "legal",	612,	1008 },
	{ 0 }
};

PAGE_SIZE  *page_size_lookup(str)
     char *str;
{
	register PAGE_SIZE *p;
	for (p = page_size; p->sz_name; p++) {
		if (!strcmp(p->sz_name, str)) {
			return p;
		}
	}
	return 0;
}
