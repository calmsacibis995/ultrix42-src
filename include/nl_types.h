/* @(#)nl_types.h	4.2	(ULTRIX)	12/6/90	*/

/************************************************************************
 *									*
 *         Copyright (c) Digital Equipment Corporation, 1990		*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 ************************************************************************/
/* Modification History: 
 *
 *     created 11/28/90
 */
#ifndef _STDIO_H
#include <stdio.h>
#endif


#ifndef _NL_TYPES_H_
#define _NL_TYPES_H_
#endif

/* definition of langinfo ITEM type */
typedef char *nl_item;

/*
 * nl_catd      used by the message catalogue functions catopen, catgetmsg,
 *              catgets and catclose to identify a catalogue descriptor.
 */
struct catalog_descriptor {
	char		*_mem;
	char		*_name;
	FILE 		*_fd;
	struct _header 	*_hd;
	struct _catset 	*_set;
	int		_setmax;
	int		_count;
	int		_pid;
};
typedef struct catalog_descriptor *nl_catd;

/* default set id used by gencat if missing $set directive */
#define NL_SETD 	1  				

extern nl_catd catopen();
extern char  *catgets();
extern int catclose();

