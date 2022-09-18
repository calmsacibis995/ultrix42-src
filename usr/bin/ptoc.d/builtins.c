#ifndef lint
static	char	*sccsid = "@(#)builtins.c	4.1	(ULTRIX)	7/17/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * File: 	builtins.c
 *
 * Pascal to C translator - Builtin Pascal functions & procs setup here.
 */

#include "ptoc.h"

struct stentry *getstentry();
char *getname();

/*
 * Builtins:  Make symbol table entries for Pascal Builtin functions,
 * 		and for C "strcpy".
 */

builtins()
{
    struct stentry *st;
    struct stentry *procst;
    struct stentry *prevst;
    int i;

    for (i = 1; ; i++)
	{
	st = getstentry();
	procst = st;
	st->st_class = FUNCC;
	addsymbol(st);
	switch (i)
	    {
	    case 1:				/* new(ptr) */
		st->st_class = PROCC;
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "new");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "p");
		st->st_byref = 1;
		procst->st_lparam = st;
		break;
	    case 2:				/* dispose(ptr) */
		st->st_class = PROCC;
		st->st_nparams = 1;
		st->st_name = getname(7);
		strcpy(st->st_name, "dispose");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "p");
		st->st_byref = 1;
		procst->st_lparam = st;
		break;
	    case 3:				/* eoln(f) */
		st->st_nparams = 1;
		st->st_name = getname(4);
		strcpy(st->st_name, "eoln");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "f");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 4:				/* eof(f) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "eof");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "f");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 5:				/* ord(c) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "ord");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "c");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 6:				/* chr(n) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "chr");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "n");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 7:				/* put(f) */
		st->st_class = PROCC;
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "put");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "f");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 8:				/* get(f) */
		st->st_class = PROCC;
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "get");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "f");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 9:				/* reset(f) */
		st->st_class = PROCC;
		st->st_nparams = 1;
		st->st_name = getname(5);
		strcpy(st->st_name, "reset");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "f");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 10:				/* rewrite(f) */
		st->st_class = PROCC;
		st->st_nparams = 1;
		st->st_name = getname(7);
		strcpy(st->st_name, "rewrite");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "f");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 11:				/* abs(x) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "abs");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 12:				/* sqr(x) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "sqr");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 13:				/* sin(x) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "sin");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 14:				/* cos(x) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "cos");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 15:				/* arctan(x) */
		st->st_nparams = 1;
		st->st_name = getname(6);
		strcpy(st->st_name, "arctan");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 16:				/* exp(x) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "exp");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 17:				/* ln(x) */
		st->st_nparams = 1;
		st->st_name = getname(2);
		strcpy(st->st_name, "ln");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 18:				/* sqrt(x) */
		st->st_nparams = 1;
		st->st_name = getname(4);
		strcpy(st->st_name, "sqrt");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 19:				/* odd(x) */
		st->st_nparams = 1;
		st->st_name = getname(3);
		strcpy(st->st_name, "odd");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 20:				/* trunc(x) */
		st->st_nparams = 1;
		st->st_name = getname(5);
		strcpy(st->st_name, "trunc");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 21:				/* round(x) */
		st->st_nparams = 1;
		st->st_name = getname(5);
		strcpy(st->st_name, "round");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 22:				/* succ(x) */
		st->st_nparams = 1;
		st->st_name = getname(4);
		strcpy(st->st_name, "succ");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 23:				/* pred(x) */
		st->st_nparams = 1;
		st->st_name = getname(4);
		strcpy(st->st_name, "pred");
		/*
		 * Get param
		 */
		st = getstentry();
		procst->st_fparam = st;
		st->st_name = getname(1);
		strcpy(st->st_name, "x");
		st->st_byref = 0;
		procst->st_lparam = st;
		break;
	    case 24:				/* strcpy(str1,str2) */
		st->st_nparams = 2;
		st->st_name = getname(6);
		strcpy(st->st_name, "strcpy");
		break;
	    default:
		return;
	    }
	}
}
