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
static char Sccsid[] = "@(#)message.c	4.1	(ULTRIX)	7/17/90";
#endif

#include <stdio.h>
#include "ic.h"

#define VARARG fmt, v1, v2, v3, v4, v5
#define VARPARAM (VARARG) char *fmt;

extern FILE *yyerfp;		/* defined in yyerror.c */

/*
 * message -- issue a message to yyerfp
 */
/*VARARGS1*/
void
message VARPARAM
{
	fprintf(yyerfp, VARARG);
	fputs(".\n", yyerfp);
}

/*
 * error -- issue an error message to yyerfp
 */
/*VARARGS1*/
void
error VARPARAM
{
	extern int yynerrs;

	yywhere();
	fprintf(yyerfp, "[error %2d] ", ++yynerrs);
	message(VARARG);
}

/*
 * warning -- issue a warning message to yyerfp
 */
/*VARARGS1*/
void
warning VARPARAM
{
	yywhere();
	fputs("[warning ] ", yyerfp);
	message(VARARG);
}

/*
 * fatal -- issue a fatal error message to yyerfp and exit
 */
/*VARARGS1*/
void
fatal VARPARAM
{
	yywhere();
	fputs("[fatal error] ", yyerfp);
	message(VARARG);
	/*
	 * remove temporary files:
	 */
	tmp_del();
	/*
	 * remove data base
	 */
	if (cod_anc != (sym *)0)
		unlink(cod_anc->sym_nam);
	exit(1);
}

/*
 * bug -- issue a "this cannot happen" message and exit
 */
/*VARARGS1*/
void
bug VARPARAM
{
	yywhere();
	fputs("[fatal bug] ", yyerfp);
	message(VARARG);
	exit(2);
}
