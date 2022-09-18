/* @(#)guesser.h	4.1      ULTRIX 	10/16/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Modification History:
 *
 * 26-Sep-1990 - Adrian Thoms (thoms@wessex)
 *	Modified from code in old ln03of filter to export the interface
 *	of the guesser library module
 */

#define EMPTY_FILE		0
#define EXECUTABLE_FILE		1
#define ARCHIVE_FILE		2
#define DATA_FILE		3
#define TEXT_FILE		4
#define CTEXT_FILE		5
#define ATEXT_FILE		6
#define RTEXT_FILE		7
#define FTEXT_FILE		8
#define CAT_FILE		9
#define XIMAGE_FILE		10
#define POSTSCRIPT_FILE		11
#define ANSI_FILE               12


#define HOW_MUCH_TO_CHECK	4096		/* input buffer amount	*/

extern int determinefile();
extern char filestorage[];
extern int globi, in;
