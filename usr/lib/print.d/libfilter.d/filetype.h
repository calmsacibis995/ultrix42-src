/* @(#)filetype.h	4.1      ULTRIX 	10/16/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 * Modification History:
 *
 * 26-Sep-1990 - Adrian Thoms (thoms@wessex)
 *	Added addition stuff to export new fdtype() library interface
 */

#define PRINT		1
#define NOPRINT		0

#define MASH(x, y)	(((x)<<16) + (y))
#define MAJOR(l)	((unsigned)(l)>>16)
#define MINOR(l)	((l)&0xffff)

#define UNKNOWN		-1
#define SLINK		1
#define	DIRECTORY	2
#define	APPENDONLY		2
#define	STANDARD		1
#define	NAMEDPIPE	3
#define SOCKET		4
#define	SPECIAL		5
#define	BLOCK			1
#define	CHARACTER		2
#define	EMPTY		6
#define	ASCII		7
#define ASCIIwGARBAGE	8
#define	SCCS			1
#define	SHELL			2
#define	BSHELL			3
#define CSHELL			4
#define	CPROG			5
#define FORTPROG		6
#define	ASSEMBLER		7
#define NROFF			8
#define TEXT			9
#define CPIOARCHIVE		106 /* also used under DATA */
#define TROFFINT		10
#define POSTSCRIPT		11
#define COMMANDS		12
#define ENGLISH			100
#define	PRESS		9
#define DATA		11
#define DDIF			1
#define CAT_TROFF		2
#define X_IMAGE			3
#define COMPACTED		4
#define COMPRESSED		5
#define UUENCODED		6
#define PACKED			7
#define LN03			8
#define EXECUTABLE	12
#define PDP11SA			1
#define E411			2
#define E410			3
#define E413			4
#define PDP430			5
#define	PDP431			6
#define	PDP450			7
#define PDP451			8
#define ARCHIVE		14
#define VERYOLD			3
#define OLDARCH			2
#define STANDARD		1
#define RANLIB			4

extern long fdtype();
extern int binary_mktab();
