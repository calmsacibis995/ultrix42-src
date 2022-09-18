/*	ctrace - C program debugging tool
 *
 *	global type and data definitions
 *
 */

/* 	@(#)global.h	4.1	(ULTRIX)	7/17/90 	*/


/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *
 *   Modification History:
 *
 *
 */

#include <stdio.h>
#include "constants.h"

enum	bool {no, yes};

enum	trace_type {
	normal, assign, prefix, postfix, string, strres
};
struct symbol_struct {
	int	start, end;
	enum	symbol_type {
		constant, variable, repeatable, side_effect
	} type;
};

/* main.c global data */
extern	enum	bool suppress;	/* suppress redundant trace output (-s) */
extern	enum	bool pound_line;/* input preprocessed so suppress #line	*/
extern	int	tracemax;	/* maximum traced variables per statement (-t number) */
extern	char	*filename;	/* input file name */
extern	enum	bool trace;	/* indicates if this function should be traced */

/* parser.y global data */
extern	enum	bool fcn_body;	/* function body indicator */

/* scanner.l global data */
extern	char	indentation[];	/* left margin indentation (blanks and tabs ) */
extern	char	yytext[];	/* statement text */
extern	enum	bool too_long;  /* statement too long to fit in buffer */
extern	int	last_yychar;	/* used for parser error handling */
extern	int	token_start;	/* start of this token in the text */
extern	int	yyleng;		/* length of the text */
extern	int	yylineno;	/* number of current input line */
extern	FILE	*yyin;		/* input file descriptor */

/* lookup.c global data */
extern	enum	bool stdio_preprocessed;	/* stdio.h already preprocessed */
extern	enum	bool setjmp_preprocessed;	/* setjmp.h already preprocessed */
