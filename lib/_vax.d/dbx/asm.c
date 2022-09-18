/*	@(#)asm.c	4.2	ULTRIX	11/9/90	*/

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
 *  004	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	003 - Added a null L_MODINIT routine; otherwise, we blow up	*
 *	      if someone (like, say, VMS) decides to put debug info	*
 *	      in an assembler file.  (Jon Reeves, March 18, 1988)	*
 *									*
 *	002 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	001 - Modified asm_init() to use LanguageName constant in call	*
 *	      to language_define().					*
 *	      (Victoria Holt, June 22, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)asm.c	2.1	ULTRIX	4/24/89";
#endif not lint

/*
 * Assembly language dependent symbol routines.
 */

#include "defs.h"
#include "symbols.h"
#include "asm.h"
#include "languages.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"

#define isdouble(range) ( \
    range->symvalue.rangev.upper == 0 and range->symvalue.rangev.lower > 0 \
)

/*
 * Initialize assembly language information.
 */

public asm_init()
{
    Language lang;

    lang = language_define("assembler", ASSEMBLER);
    language_setop(lang, L_PRINTDECL, asm_printdecl);
    language_setop(lang, L_PRINTVAL, asm_printval);
    language_setop(lang, L_TYPEMATCH, asm_typematch);
    language_setop(lang, L_BUILDAREF, asm_buildaref);
    language_setop(lang, L_EVALAREF, asm_evalaref);
    language_setop(lang, L_MODINIT, asm_modinit);
    language_setop(lang, L_HASMODULES, asm_hasmodules);
    language_setop(lang, L_PASSADDR, asm_passaddr);
    language_setop(lang, L_PRINTF, asm_printf);
}

/*
 * Test if two types are compatible.
 */

public Boolean asm_typematch(type1, type2)
Symbol type1, type2;
{
    Boolean b;

    b = false;
    return b;
}

public asm_printdecl(s)
Symbol s;
{
    switch (s->class) {
	case CONST:
	    printf("%s = %d", symname(s), s->symvalue.constval->value.lcon);
	    break;

	case VAR:
	case REF:
	    printf("&%s = 0x%x", symname(s), s->symvalue.offset);
	    break;

	case PROC:
	case FUNC:
	    printf("%s (0x%x):", symname(s), codeloc(s));
	    break;

	case TYPE:
	    printf("%s", symname(s));
	    break;

	case ARRAY:
	    printf("$string");
	    break;

	default:
	    printf("[%s]", classname(s));
	    break;
    }
    putchar('\n');
}

/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 */

public asm_printval(s)
register Symbol s;
{
    register Symbol t;
    register Integer len;

    switch (s->class) {
	case ARRAY:
	    t = rtype(s->type);
	    if (t->class == RANGE and istypename(t->type, "$char")) {
		len = size(s);
		sp -= len;
		printf("\"%.*s\"", len, sp);
	    } else {
		printarray(s);
	    }
	    break;

	default:
	    printf("0x%x", pop(Integer));
	    break;
    }
}

/*
 * Treat subscripting as indirection through pointer to integer.
 */

public Node asm_buildaref(a, slist)
Node a, slist;
{
    register Symbol t;
    register Node p;
    Symbol etype, eltype;
    Node r, esub;

    t = rtype(a->nodetype);
    eltype = t->type;

    /* Start of vector support */
    if (t->class == ARRAY) {
      p = slist;
      esub = p->value.arg[0];
      etype = rtype(esub->nodetype);
      if (not compatible(etype, t_int)) {
	  beginerrmsg();
	  fprintf(stderr, "subscript must be integer-compatible");
	  enderrmsg();
      }
      if (p->value.arg[1] != nil) {
	  beginerrmsg();
	  fprintf(stderr, "too many subscripts for \"");
	  prtree(stderr, a);
	  fprintf(stderr, "\"");
	  enderrmsg();
      }
      r = build(O_INDEX, a, esub);
    /* End of vector support */
    } else {
        p = slist->value.arg[0];
	r = build(O_MUL, p, build(O_LCON, (long) size(eltype)));
	r = build(O_ADD, build(O_RVAL, a), r);
    }
    r->nodetype = eltype;
    return r;
}

/*
 * Evaluate a subscript index.  Assumes dimension is [0..n].
 */

public asm_evalaref(s, base, i)
Symbol s;
Address base;
long i;
{
    Symbol t;
    long lb, ub;

    t = rtype(s);
    /* Start of vector support */
    s = t->chain;
    lb = s->symvalue.rangev.lower;
    ub = s->symvalue.rangev.upper;
    if (i < lb or i > ub) {
	error("subscript out of range");
    }
    /* End of vector support */
    push(long, base + i * size(t->type));
}

public asm_modinit (typetable)
Symbol typetable[];
{
    /* nothing right now */
}

public boolean asm_hasmodules ()
{
    return false;
}

public boolean asm_passaddr (param, exprtype)
Symbol param, exprtype;
{
    return false;
}

Node c_printf();

public Node asm_printf(p, argv)
Node p;
char **argv;
{
	return c_printf(p, argv);
}
