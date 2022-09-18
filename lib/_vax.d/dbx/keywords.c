/*@(#)keywords.c	4.2	Ultrix	11/9/90*/

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
 *  002	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	001 - Merged in 4.3 changes.					*
 *	      (Victoria Holt, April 29, 1986)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)keywords.c	5.2 (Berkeley) 6/4/85";
#endif not lint

static char rcsid[] = "$Header: keywords.c,v 1.5 84/12/26 10:39:45 linton Exp $";

/*
 * Keywords, variables, and aliases (oh my!).
 */

#include "defs.h"
#include "keywords.h"
#include "scanner.h"
#include "names.h"
#include "symbols.h"
#include "tree.h"
#include "lists.h"
#include "main.h"
#include "y.tab.h"

#ifndef public

#include "scanner.h"
#include "tree.h"

#endif

private String reserved[] ={
    "alias", "and", "assign", "at",
	"call", "callv", "catch", "cd", "cont", 
    "debug", "delete", "div", "down", "dump", "edit", "file", "fmask",
	"func", "gripe", "getenv", "help", "history", "if", "ignore", "in",
    "list", "mod", "next", "nexti", "nextv", "nil", "not", "or", 
    "print", "printf", "psym", "pwd", "quit",
	"record", "reread", "rerun", "return", "run",
    "set", "setenv", "sh", "skip", "source", "status", "step", "stepi", 
	"stepv", "stop", "stopi", "tmask", "trace", "tracei",
	"unalias", "unset", "up", "use", 
    "whatis", "when", "where", "whereis", "which",
    "INT", "CHAR", "REAL", "NAME", "STRING", "->"
};

/*
 * The keyword table is a traditional hash table with collisions
 * resolved by chaining.
 */

#define HASHTABLESIZE 1007

typedef enum { ISKEYWORD, ISALIAS, ISVAR } KeywordType;

typedef struct Keyword {
    Name name;
    KeywordType class : 16;
    union {
	/* ISKEYWORD: */
	    Token toknum;

	/* ISALIAS: */
	    struct {
		unsigned char builtin;
		List paramlist;
		String expansion;
	    } alias;

	/* ISVAR: */
	    Node var;
    } value;
    struct Keyword *chain;
} *Keyword;

typedef unsigned int Hashvalue;

private Keyword hashtab[HASHTABLESIZE];

#define hash(n) ((((unsigned) n) >> 2) mod HASHTABLESIZE)

/*
 * Enter all the reserved words into the keyword table.
 *
 * If the vaddrs flag is set (through the -k command line option) then
 * set the special "$mapaddrs" variable.  This assumes that the
 * command line arguments are scanned before this routine is called.
 */

public enterkeywords()
{
    register integer i;

    for (i = ALIAS; i <= WHICH; i++) {
	keyword(reserved[ord(i) - ord(ALIAS)], i);
    }
    defalias("c", "cont");
    defalias("d", "delete");
    defalias("h", "help");
    defalias("hi", "history");
    defalias("e", "edit");
    defalias("l", "list");
    defalias("n", "next");
    defalias("p", "print");
    defalias("pd", "printf \"%d\\n\",");
    defalias("po", "printf \"0%o\\n\",");
    defalias("px", "printf \"0x%x\\n\",");
    defalias("q", "quit");
    defalias("r", "run");
    defalias("s", "step");
    defalias("st", "stop");
    defalias("j", "status");
    defalias("t", "where");
    if (vaddrs) {
	defvar(identname("$mapaddrs", true), nil);
    }
}

/*
 * Deallocate the keyword table.
 */

public keywords_free()
{
    register Integer i;
    register Keyword k, nextk;

    for (i = 0; i < HASHTABLESIZE; i++) {
	k = hashtab[i];
	while (k != nil) {
	    nextk = k->chain;
	    dispose(k);
	    k = nextk;
	}
	hashtab[i] = nil;
    }
}

/*
 * Insert a name into the keyword table and return the keyword for it.
 */

private Keyword keywords_insert (n)
Name n;
{
    Hashvalue h;
    Keyword k;

    h = hash(n);
    k = new(Keyword);
    k->name = n;
    k->chain = hashtab[h];
    hashtab[h] = k;
    return k;
}

/*
 * Find the keyword associated with the given name.
 */

private Keyword keywords_lookup (n)
Name n;
{
    Hashvalue h;
    register Keyword k;

    h = hash(n);
    k = hashtab[h];
    while (k != nil and k->name != n) {
	k = k->chain;
    }
    return k;
}

/*
 * Delete the given keyword of the given class.
 */

private boolean keywords_delete (n, class)
Name n;
KeywordType class;
{
    Hashvalue h;
    register Keyword k, prevk;
    boolean b;

    h = hash(n);
    k = hashtab[h];
    prevk = nil;
    while (k != nil and (k->name != n or k->class != class)) {
	prevk = k;
	k = k->chain;
    }
    if (k != nil) {
	b = true;
	if (prevk == nil) {
	    hashtab[h] = k->chain;
	} else {
	    prevk->chain = k->chain;
	}
	dispose(k);
    } else {
	b = false;
    }
    return b;
}

/*
 * Enter a keyword into the table.  It is assumed to not be there already.
 * The string is assumed to be statically allocated.
 */

private keyword (s, t)
String s;
Token t;
{
    Keyword k;
    Name n;

    n = identname(s, true);
    k = keywords_insert(n);
    k->class = ISKEYWORD;
    k->value.toknum = t;
}

/*
 * Define a builtin command name alias.
 */

private defalias (s1, s2)
String s1, s2;
{
    alias(identname(s1, true), nil, s2, true);
}

/*
 * Look for a word of a particular class.
 */

private Keyword findword (n, class)
Name n;
KeywordType class;
{
    register Keyword k;

    k = keywords_lookup(n);
    while (k != nil and (k->name != n or k->class != class)) {
	k = k->chain;
    }
    return k;
}

/*
 * Return the token associated with a given keyword string.
 * If there is none, return the given default value.
 */

public Token findkeyword (n, def)
Name n;
Token def;
{
    Keyword k;
    Token t;

    k = findword(n, ISKEYWORD);
    if (k == nil) {
	t = def;
    } else {
	t = k->value.toknum;
    }
    return t;
}

/*
 * Return the associated string if there is an alias with the given name.
 */

public boolean findalias (n, pl, str)
Name n;
List *pl;
String *str;
{
    Keyword k;
    boolean b;

    k = findword(n, ISALIAS);
    if (k == nil) {
	b = false;
    } else {
	*pl = k->value.alias.paramlist;
	*str = k->value.alias.expansion;
	b = true;
    }
    return b;
}

/*
 * Return the string associated with a token corresponding to a keyword.
 */

public String keywdstring (t)
Token t;
{
    return reserved[ord(t) - ord(ALIAS)];
}

/*
 * Process an alias command, either entering a new alias or printing out
 * an existing one.
 */

public alias (newcmd, args, str, builtin_flag)
Name newcmd;
List args;
String str;
unsigned char builtin_flag;
{
    Keyword k;

    if (str == nil) {
	print_alias(newcmd);
    } else {
	k = findword(newcmd, ISALIAS);
	if (k == nil) {
	    k = keywords_insert(newcmd);
	}
	k->class = ISALIAS;
	k->value.alias.paramlist = args;
	k->value.alias.expansion = str;
	k->value.alias.builtin = builtin_flag;
    }
}

/*
 * Print out an alias.
 */

private print_alias (cmd)
Name cmd;
{
    register Keyword k;
    register Integer i;
    Name n;

    if (cmd == nil) {
	for (i = 0; i < HASHTABLESIZE; i++) {
	    for (k = hashtab[i]; k != nil; k = k->chain) {
		if (k->class == ISALIAS) {
		    if (isredirected()) {
				if(!k->value.alias.builtin) {
					printf("alias %s", ident(k->name));
					printparams(k->value.alias.paramlist);
					printf("\t\"%s\"\n", k->value.alias.expansion);
				}
		    } else {
			printf("%s", ident(k->name));
			printparams(k->value.alias.paramlist);
			printf("\t%s\n", k->value.alias.expansion);
		    }
		}
	    }
	}
    } else {
	k = findword(cmd, ISALIAS);
	if (k == nil) {
	    printf("\n");
	} else {
	    printparams(k->value.alias.paramlist);
	    printf("%s\n", k->value.alias.expansion);
	}
    }
}

private printparams (pl)
List pl;
{
    Name n;

    if (pl != nil) {
	printf("(");
	foreach(Name, n, pl)
	    printf("%s", ident(n));
	    if (not list_islast()) {
		printf(", ");
	    }
	endfor
	printf(")");
    }
}

/*
 * Remove an alias.
 */

public unalias (n)
Name n;
{
    if (not keywords_delete(n, ISALIAS)) {
	error("%s is not aliased", ident(n));
    }
}

/*
 * Define a variable.
 */

public defvar (n, val)
Name n;
Node val;
{
    Keyword k;

    if (n == nil) {
	print_vars();
    } else {
	if (lookup(n) != nil) {
	    error("\"%s\" is a program symbol -- use assign", ident(n));
	}
	k = findword(n, ISVAR);
	if (k == nil) {
	    k = keywords_insert(n);
	}
	k->class = ISVAR;
	k->value.var = val;
	if (n == identname("$mapaddrs", true)) {
	    vaddrs = true;
	}
    }
}

/*
 * Return the value associated with a variable.
 */

public Node findvar (n)
Name n;
{
    Keyword k;
    Node val;

    k = findword(n, ISVAR);
    if (k == nil) {
	val = nil;
    } else {
	val = k->value.var;
    }
    return val;
}

/*
 * Return whether or not a variable is set.
 */

public boolean varIsSet (s)
String s;
{
    return (boolean) (findword(identname(s, false), ISVAR) != nil);
}

/*
 * Delete a variable.
 */

public undefvar (n)
Name n;
{
    if (not keywords_delete(n, ISVAR)) {
	error("%s is not set", ident(n));
    }
    if (n == identname("$mapaddrs", true)) {
	vaddrs = false;
    }
}

/*
 * Print out all the values of set variables.
 */

private print_vars ()
{
    register integer i;
    register Keyword k;

    for (i = 0; i < HASHTABLESIZE; i++) {
	for (k = hashtab[i]; k != nil; k = k->chain) {
	    if (k->class == ISVAR) {
		if (isredirected()) {
		    printf("set ");
		}
		printf("%s", ident(k->name));
		if (k->value.var != nil) {
		    printf("\t");
		    prtree(stdout, k->value.var);
		}
		printf("\n");
	    }
	}
    }
}
