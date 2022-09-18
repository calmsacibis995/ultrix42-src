/************************************************************************
 *									*
 *	 	      Copyright (c) 1987, 1988 by			*
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

/* @(#)defs.h	4.1	(ULTRIX)	7/17/90 */

/*
 *
 *   File name: defs.h
 *
 *   Source file description:
 *	A useful header file for the string extraction/merge tools
 *
 *   Modification history:
 *	Andy Gadsby, 20-Jan-1987.
 *		Created.
 *
 */

/* some very useful standard definitions				*/

#define ERROR     (-1)		/* standard error return		*/
#ifdef OK
#undef OK
#endif OK
#define OK	  (0)		/* standard success return		*/
#ifdef TRUE
#undef TRUE
#endif TRUE
#define TRUE	  (1)
#ifdef FALSE
#undef FALSE
#endif FALSE
#define FALSE     (0)	

#ifndef NULL
# define NULL 	  0
#endif

#define STREXTRACT 	"strextract"

#define IGNFILE	   	"ignore"
#define PATTERN_FILE 	"patterns"

#define LIB_DIR		"/usr/lib/intln/"
#define HELPFILE   	"/usr/lib/intln/help"

#define MSGSUFFIX ".msg"	/* the message file suffix		*/
#define CATSUFFIX ".msf"	/* the catalogue file suffix		*/
#define INTPREFIX "nl_"		/* natural language prefix string	*/

#define LINESIZE  256		/* maximum size of a text line		*/
#define REWLEN   1024		/* maximum size of a rewrite string	*/

/* some general purpose function definitions				*/
char *fixsuffix();
char *fixprefix();

/* string ignore/duplicate matching definitions				*/

#define HASHMASK	63		/* a useful number		*/

struct element {			/* the information we save	*/
	struct element *next;		/* a flink			*/
	int    len;			/* the length of the string	*/
	long   linenum;			/* the first line number	*/
	long   msgnum;			/* message num in catalogue	*/
	int    flags;			/* useful flags			*/
	char   string[1];		/* start of string		*/
};

struct element *lookupstr();

					/* setting of the flags		*/
#define STR_IGNORE	1		/* ignore this string		*/
