#ifndef lint
static char	*sccsid = " @(#)f_errlist.c	1.2	(ULTRIX)	1/16/86";
#endif lint

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
*	David Metsky		10-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	f_errlist.c	5.2		7/30/85
*
*************************************************************************/

/*
 * f77 I/O error messages
 */

char *f_errlist[] =
{
/* 100 */	"error in format",
/* 101 */	"illegal unit number",
/* 102 */	"formatted i/o not allowed",
/* 103 */	"unformatted i/o not allowed",
/* 104 */	"direct i/o not allowed",
/* 105 */	"sequential i/o not allowed",
/* 106 */	"can't backspace file",
/* 107 */	"off beginning of record",
/* 108 */	"can't stat file",
/* 109 */	"no * after repeat count",
/* 110 */	"off end of record",
/* 111 */	"truncation failed",
/* 112 */	"incomprehensible list input",
/* 113 */	"out of free space",
/* 114 */	"unit not connected",
/* 115 */	"invalid data for integer format term",
/* 116 */	"invalid data for logical format term",
/* 117 */	"'new' file exists",
/* 118 */	"can't find 'old' file",
/* 119 */	"opening too many files or unknown system error",
/* 120 */	"requires seek ability",
/* 121 */	"illegal argument",
/* 122 */	"negative repeat count",
/* 123 */	"illegal operation for unit",
/* 124 */	"invalid data for d,e,f, or g format term",
/* 125 */	"illegal input for namelist",
};

int f_nerr = (sizeof(f_errlist)/sizeof(char *));
