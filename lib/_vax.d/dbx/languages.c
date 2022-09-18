/*#@(#)languages.c	1.5	Ultrix	7/16/86*/

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
 *	004 - Added a simple and temporary hack for BLISS support.	*
 *	      (vjh, July 16, 1986)					*
 *									*
 *	003 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	002 - Fixed language_op() so that an error is generated when	*
 *	      trying to execute a language specific feature when the	*
 *	      language is UNKNOWN.					*
 *	      (vjh, Oct. 1, 1985)					*
 *									*
 *	001 - Modified several routines to make use of LanguageName	*
 *	      type.  Added routine getlangname(), which translates	*
 *	      a source file extension into a LanguageName.		*
 *	      (Victoria Holt, June 22, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)languages.c	4.2	ULTRIX	11/9/90";
#endif not lint

static char rcsid[] = "$Header: languages.c,v 1.5 84/12/26 10:39:49 linton Exp $";

/*
 * Language management.
 */

#include "defs.h"
#include "languages.h"
#include "c.h"
#include "pascal.h"
#include "modula-2.h"
#include "asm.h"

#ifndef public

typedef struct Language *Language;

typedef enum {
    L_PRINTDECL, L_PRINTVAL, L_TYPEMATCH, L_BUILDAREF, L_EVALAREF,
    L_MODINIT, L_HASMODULES, L_PASSADDR, L_PRINTF,
    L_ENDOP
} LanguageOp;

typedef enum {
    UNKNOWN, ASSEMBLER, C, FORTRAN, PASCAL, MODULA2
} LanguageName;

typedef LanguageOperation();

Language primlang;

#endif

struct Language {
    String name;
    LanguageName lname;
    LanguageOperation *op[20];
    Language next;
};

private Language head;

/*
 * Initialize language information.
 *
 * The last language initialized will be the default one
 * for otherwise indistinguised symbols.
 */

public language_init()
{
    primlang = language_define("$builtin symbols", UNKNOWN);
    c_init();
    fortran_init();
    pascal_init();
    modula2_init();
    asm_init();
}

public Language findlanguage(lname)
LanguageName lname;
{
    Language lang;

    lang = head;
    while (lang != nil and lang->lname != lname) {
	lang = lang->next;
    }
    if (lang == nil) {
        lang = head;
    }
    return lang;
}

public String language_name(lang)
Language lang;
{
    return (lang == nil) ? "(nil)" : lang->name;
}

public Language language_define(name, lname)
String name;
LanguageName lname;
{
    Language p;

    p = new(Language);
    p->name = name;
    p->lname = lname;
    p->next = head;
    head = p;
    return p;
}

public language_setop(lang, op, operation)
Language lang;
LanguageOp op;
LanguageOperation *operation;
{
    checkref(lang);
    assert(ord(op) < ord(L_ENDOP));
    lang->op[ord(op)] = operation;
}

public LanguageOperation *language_op(lang, op)
Language lang;
LanguageOp op;
{
    LanguageOperation *o;

    checkref(lang);
    if (lang->lname == UNKNOWN) {
        error("unknown language");
    }
    o = lang->op[ord(op)];
    checkref(o);
    return o;
}

/* Routine to translate a filename extension into a LanguageName.
 */
public LanguageName getlangname(suffix)
String suffix;
{
    LanguageName lname;

    if (suffix == nil) {
        return UNKNOWN;
    }
    switch (*suffix) {
        case 'f':
	case 'F':
	    lname = FORTRAN;
	    break;

	case 'b':	/* temporary hack for BLISS */
	case 'B':	/* temporary hack for BLISS */
	case 'c':
	case 'C':
	    lname = C;
	    break;

	case 's':
	case 'S':
	    lname = ASSEMBLER;
	    break;

	case 'p':
	case 'P':
	    lname = PASCAL;
	    break;

	case '\0':
	    lname = UNKNOWN;
	    break;

	default:
	    if (streq(suffix, "mod") or (streq(suffix, "MOD"))) {
	        lname = MODULA2;
	    } else {
	        lname = UNKNOWN;
	    }
	    break;
    }
    return lname;
}

public String language_suffix(lang)
Language lang;
{
    return ((String)"c");
}
