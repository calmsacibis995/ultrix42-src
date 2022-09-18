/*@(#)debug.c	4.2    Ultrix	%G$*/

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
 *									*
 *			Modification History				*
 *									*
 *  003	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	002 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	001 - Added O_PRINTENTRY to operator_str.			*
 *	      (Victoria Holt, July 18, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)debug.c	5.1 (Berkeley) 5/31/85";
#endif not lint

static char rcsid[] = "$Header: debug.c,v 1.5 84/12/26 10:39:01 linton Exp $";

/*
 *  Debug routines
 */

#include "defs.h"
#include "tree.h"
#include "operators.h"
#include "eval.h"
#include "events.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "object.h"
#include "main.h"
#include "mappings.h"
#include "process.h"
#include "machine.h"
#include "debug.h"
#include <signal.h>

public boolean tracetree;	/* trace building of parse trees */
public boolean traceeval;	/* trace tree evaluation */

/*
 * Dynamically turn on/off a debug flag, or display some information.
 */

public debug (p)
Node p;
{
   int code;

   code = p->value.lcon;
    switch (code) {
	case 0:
	    puts("debugging flags:");
	    puts("    1        trace scanner return values");
	    puts("    2        trace breakpoints");
	    puts("    3        trace execution");
	    puts("    4        trace tree building");
	    puts("    5        trace tree evaluation");
	    puts("   -[12345]  turns off corresponding flag");
	    puts("    6        dump function table");
	    break;

	case 1:
	case -1:
#           ifdef LEXDEBUG
		lexdebug = (boolean) (code > 0);
#           else
		error("can't debug scanner (not compiled with LEXDEBUG)");
#           endif
	          break;

	case 2:
	case -2:
	    tracebpts = (boolean) (code > 0);
	          break;

	case 3:
	case -3:
	    traceexec = (boolean) (code > 0);
	          break;

	case 4:
	case -4:
	    tracetree = (boolean) (code > 0);
	          break;

	case 5:
	case -5:
	    traceeval = (boolean) (code > 0);
	          break;

	case 6:
	    dumpfunctab();
                  break;

	default:
	    error("unknown debug flag");
	    break;
        }
}

private String leafname[] = {
    "nop", "name", "sym", "lcon", "ccon", "fcon", "scon", 
    "rval", "vreg","index"
};

public String opname (op)
Operator op;
{
    String s;
    static char buf[100];

	switch (op) {
	case O_ITOF:
	    s = "itof";
		break;

	case O_ENDX:
	    s = "endx";
		break;

	case O_QLINE:
	    s = "qline";
		break;

	default:
	    if (ord(op) <= ord(O_INDEX)) {
		s = leafname[ord(op)];
		} else {
		s = opinfo[ord(op)].opstring;
		if (s == nil) {
		    sprintf(buf, "[op %d]", op);
		    s = buf;
		}
		}
		break;
		}
    return s;
}
