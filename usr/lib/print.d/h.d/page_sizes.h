/*	@(#)page_sizes.h	4.1	ULTRIX	7/2/90 */

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
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  31/05/88 -- thoms
 * date and time created 88/05/31 19:55:02 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  19/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * SCCS history end
 */


typedef struct page_size_s {
	char *sz_name;		/* name returned by check_arg */
	short sz_width;		/* width in points */
	short sz_height;	/* height in points */
} PAGE_SIZE;

enum page_size_e {
        sz_10x13,
	sz_9x12,
	sz_a,
	sz_a3,
	sz_a4,
	sz_a5,
	sz_b,
	sz_b4,
	sz_b5,
	sz_bus,
	sz_c4,
	sz_c5,
	sz_executive,
        sz_legal,
};

extern PAGE_SIZE *page_size_lookup(/* char str */);
