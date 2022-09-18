#ifndef lint
static char sccsid[] = "@(#)check.c	4.2	ULTRIX	11/9/90";
#endif not lint
/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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
 *  007	- Added support for vectors.							*
 *			fixed part of qar 1216								*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	006 - Complete 4.3 merge: $unsafeassign was missing.		*
 *	      (jlr, June 2, 1988)					*
 *									*
 *	005 - Correct 4.3 merge error (O_CALLPROC was missing).		*
 *	      (Jon Reeves, June 8, 1987)				*
 *									*
 *	004 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	003 - Added test for coredump when attempting to continue	*
 *	      (not start) process execution (eg. with step, cont, or	*
 *	      return).							*
 *	      (vjh, July 15, 1985)					*
 *									*
 *	002 - Added test for O_DOT op in check() under O_LIST.		*
 *	      Fixes internal-error-msg that occured if the user		*
 *	      attempted to "list" a structure/union member.		*
 *	      (vjh, June 13, 1985)					*
 *									*
 *	001 - Added a test in check() under the O_LIST case.  Check()   *
 *	      will call error() if the user is attempting to list a	*
 *	      routine that does not have source information.		*
 *	      (Victoria Holt, May 30, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */


/*
 * Check a tree for semantic correctness.
 */

#include "defs.h"
#include "tree.h"
#include "operators.h"
#include "events.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "main.h"
#include "process.h"
#include <signal.h>

#ifndef public
#endif

/*
 * Check that the nodes in a tree have the correct arguments
 * in order to be evaluated.  Basically the error checking here
 * frees the evaluation routines from worrying about anything
 * except dynamic errors, e.g. subscript out of range.
 */

public check(p)
register Node p;
{
    Node p1, p2;
    Address addr;
    Symbol f;

    checkref(p);
    switch (p->op) {
	case O_ASSIGN:
	    p1 = p->value.arg[0];
	    p2 = p->value.arg[1];
	    if (varIsSet("$unsafeassign")) {
			if (size(p1->nodetype) != size(p2->nodetype)) {
		    	error("incompatible sizes");
			}
	    } else {
			if (not compatible(p1->nodetype, p2->nodetype)) {
				if((streq(ident(p1->nodetype->name), "$vquad")) &&
														(p2->op == O_SCON)) {
					break;
				}
				else {
					error("incompatible types");
	    		}
	    	}
	    }
	    break;

	case O_CATCH:
	case O_IGNORE:
	    if (p->value.lcon < 0 or p->value.lcon > NSIG) {
		error("invalid signal number");
	    }
	    break;

	case O_CONT:
	    if (coredump) {
	        error("process is not active");
	    }
	    if (p->value.lcon != DEFSIG and (
		p->value.lcon < 0 or p->value.lcon > NSIG)
	    ) {
		error("invalid signal number");
	    }
	    break;

	case O_DUMP:
	    if (p->value.arg[0] != nil) {
		if (p->value.arg[0]->op == O_SYM) {
		    f = p->value.arg[0]->value.sym;
		    if (not isblock(f)) {
			error("\"%s\" is not a block", symname(f));
		    }
		} else {
		    beginerrmsg();
		    fprintf(stderr, "expected a symbol, found \"");
		    prtree(stderr, p->value.arg[0]);
		    fprintf(stderr, "\"");
		    enderrmsg();
		}
	    }
	    break;

	case O_LIST:
	    if (p->value.arg[0]->op == O_SYM) {
		f = p->value.arg[0]->value.sym;
		if (not isblock(f) ) {
		    error("\"%s\" is not a procedure or function", symname(f));
		}
		/*
		if (nosource(f)) {
 		    error("no source lines for \"%s\"", symname(f));
		}
		addr = firstline(f);
		if (addr == NOADDR) {
		    error("\"%s\" is empty", symname(f));
		}
		*/
	    } else {
	        if (p->value.arg[0]->op == O_DOT) {
		    error("cannot list a structure/union member");
		}
	    }
	    break;

	case O_TRACE:
	case O_TRACEI:
	    chktrace(p);
	    break;

	case O_STOP:
	case O_STOPI:
	    chkstop(p);
	    break;

	case O_STEP:
	case O_STEPV:
	case O_RETURN:
	    if (coredump) {
	        error("process is not active");
	    }
	    break;

	case O_CALLPROC:
	case O_CALL:
	    if (not isroutine(p->value.arg[0]->nodetype)) {
    		Symbol s, f;
	    	
			s = p->value.arg[0]->value.sym;
			find(f, s->name) where isroutine(f) endfind(f);
	    	if (f == nil) {
				beginerrmsg();
				fprintf(stderr, "\"");
				prtree(stderr, p->value.arg[0]);
				fprintf(stderr, "\" not call-able");
				enderrmsg();
	    	}
	    }
	    break;

	case O_WHEREIS:
	    if (p->value.arg[0]->op == O_SYM and
	      p->value.arg[0]->value.sym == nil) {
		error("symbol not defined");
	    }
	    break;

	default:
	    break;
    }
}

/*
 * Check arguments to a trace command.
 */

private chktrace(p)
Node p;
{
    Node exp, place, cond;

    exp = p->value.arg[0];
    place = p->value.arg[1];
    cond = p->value.arg[2];
    if (exp == nil) {
	chkblock(place);
    } else if (exp->op == O_LCON or exp->op == O_QLINE) {
	if (place != nil) {
	    error("unexpected \"at\" or \"in\"");
	}
	if (p->op == O_TRACE) {
	    chkline(exp);
	} else {
	    chkaddr(exp);
	}
    } else if (place != nil and (place->op == O_QLINE or place->op == O_LCON)) {
	if (p->op == O_TRACE) {
	    chkline(place);
	} else {
	    chkaddr(place);
	}
    } else {
	if (exp->op != O_RVAL and exp->op != O_SYM and exp->op != O_CALL) {
	    error("can't trace expressions");
	}
	chkblock(place);
    }
}

/*
 * Check arguments to a stop command.
 */

private chkstop(p)
Node p;
{
    Node exp, place, cond;

    exp = p->value.arg[0];
    place = p->value.arg[1];
    cond = p->value.arg[2];
    if (exp != nil) {
	if (exp->op != O_RVAL and exp->op != O_SYM and exp->op != O_LCON) {
	    beginerrmsg();
	    fprintf(stderr, "expected variable, found ");
	    prtree(stderr, exp);
	    enderrmsg();
	}
	chkblock(place);
    } else if (place != nil) {
	if (place->op == O_SYM) {
	    chkblock(place);
	} else {
	    if (p->op == O_STOP) {
		chkline(place);
	    } else {
		chkaddr(place);
	    }
	}
    }
}

/*
 * Check to see that the given node specifies some subprogram.
 * Nil is ok since that means the entire program.
 */

private chkblock(b)
Node b;
{
    Symbol p, outer;

    if (b != nil) {
	if (b->op != O_SYM) {
	    beginerrmsg();
	    fprintf(stderr, "expected subprogram, found ");
	    prtree(stderr, b);
	    enderrmsg();
	} else if (ismodule(b->value.sym)) {
	    outer = b->value.sym;
	    while (outer != nil) {
		find(p, outer->name) where p->block == outer endfind(p);
		if (p == nil) {
		    outer = nil;
		    error("\"%s\" is not a subprogram", symname(b->value.sym));
		} else if (ismodule(p)) {
		    outer = p;
		} else {
		    outer = nil;
		    b->value.sym = p;
		}
	    }
	} else if (
	    b->value.sym->class == VAR and
	    b->value.sym->name == b->value.sym->block->name and
	    b->value.sym->block->class == FUNC
	) {
	    b->value.sym = b->value.sym->block;
	} else if (not isblock(b->value.sym)) {
	    error("\"%s\" is not a subprogram", symname(b->value.sym));
	}
    }
}

/*
 * Check to make sure a node corresponds to a source line.
 */

private chkline(p)
Node p;
{
    if (p == nil) {
	error("missing line");
    } else if (p->op != O_QLINE and p->op != O_LCON) {
	error("expected source line number, found \"%t\"", p);
    }
}

/*
 * Check to make sure a node corresponds to an address.
 */

private chkaddr(p)
Node p;
{
    if (p == nil) {
	error("missing address");
    } else if (p->op != O_LCON and p->op != O_QLINE) {
	beginerrmsg();
	fprintf(stderr, "expected address, found \"");
	prtree(stderr, p);
	fprintf(stderr, "\"");
	enderrmsg();
    }
}
