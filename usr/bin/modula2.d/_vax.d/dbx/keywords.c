/*#@(#)keywords.c	4.1	Ultrix	7/17/90*/
/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)keywords.c 1.3 5/18/83";

static char rcsid[] = "$Header: keywords.c,v 1.3 84/03/27 10:21:05 linton Exp $";

/*
 * Keyword management.
 */

#include "defs.h"
#include "keywords.h"
#include "scanner.h"
#include "names.h"
#include "symbols.h"
#include "tree.h"
#include "y.tab.h"

#ifndef public
#include "scanner.h"
#endif

private String reserved[] ={
    "alias", "and", "assign", "at", "call", "catch", "cont",
    "debug", "delete", "div", "down", "dump", "edit", "file", "func",
    "gripe", "help", "if", "ignore", "in",
    "list", "mod", "next", "nexti", "nil", "not", "or",
    "print", "psym", "quit", "rerun", "return", "run",
    "sh", "skip", "source", "status", "step", "stepi",
    "stop", "stopi", "trace", "tracei", "up",
    "use", "whatis", "when", "where", "whereis", "which",
    "INT", "REAL", "NAME", "STRING",
    "LFORMER", "RFORMER", "#^", "->"
};

/*
 * The keyword table is a traditional hash table with collisions
 * resolved by chaining.
 */

#define HASHTABLESIZE 503

typedef struct Keyword {
    Name name;
    Token toknum : 16;
    Boolean isalias : 16;
    struct Keyword *chain;
} *Keyword;

typedef unsigned int Hashvalue;

private Keyword hashtab[HASHTABLESIZE];

#define hash(n) ((((unsigned) n) >> 2) mod HASHTABLESIZE)

/*
 * Enter all the reserved words into the keyword table.
 */

public enterkeywords()
{
    register Integer i;

    for (i = ALIAS; i <= WHICH; i++) {
	keyword(reserved[ord(i) - ord(ALIAS)], i, false);
    }
    keyword("set", ASSIGN, false);
    keyword("c", CONT, true);
    keyword("d", DELETE, true);
    keyword("h", HELP, true);
    keyword("e", EDIT, true);
    keyword("l", LIST, true);
    keyword("n", NEXT, true);
    keyword("p", PRINT, true);
    keyword("q", QUIT, true);
    keyword("r", RUN, true);
    keyword("s", STEP, true);
    keyword("st", STOP, true);
    keyword("j", STATUS, true);
    keyword("t", WHERE, true);
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
 * Enter a keyword into the name table.  It is assumed to not be there already.
 * The string is assumed to be statically allocated.
 */

private keyword(s, t, isalias)
String s;
Token t;
Boolean isalias;
{
    register Hashvalue h;
    register Keyword k;
    Name n;

    n = identname(s, true);
    h = hash(n);
    k = new(Keyword);
    k->name = n;
    k->toknum = t;
    k->isalias = isalias;
    k->chain = hashtab[h];
    hashtab[h] = k;
}

/*
 * Return the string associated with a token corresponding to a keyword.
 */

public String keywdstring(t)
Token t;
{
    return reserved[ord(t) - ord(ALIAS)];
}

/*
 * Find the keyword associated with the given string.
 */

private Keyword kwlookup (n)
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
 * Return the token associated with a given keyword string.
 * We assume that tokens cannot legitimately be nil (0).
 */

public Token findkeyword(n)
Name n;
{
    Keyword k;
    Token t;

    k = kwlookup(n);
    if (k == nil) {
	t = nil;
    } else {
	t = k->toknum;
    }
    return t;
}

/*
 * Create an alias.
 */

public enter_alias(newcmd, oldcmd)
Name newcmd;
Name oldcmd;
{
    Token t;
    Keyword k;

    t = findkeyword(oldcmd);
    if (t == nil) {
	error("\"%s\" is not a command", ident(oldcmd));
    } else {
	k = kwlookup(newcmd);
	if (k == nil) {
	    keyword(ident(newcmd), t, true);
	} else {
	    k->toknum = t;
	}
    }
}

/*
 * Print out an alias.
 */

public print_alias(cmd)
Name cmd;
{
    register Keyword k;
    register Integer i;
    Token t;

    if (cmd == nil) {
	for (i = 0; i < HASHTABLESIZE; i++) {
	    for (k = hashtab[i]; k != nil; k = k->chain) {
		if (k->isalias) {
		    if (isredirected()) {
			printf("alias ");
		    }
		    printf("%s\t%s\n", ident(k->name), keywdstring(k->toknum));
		}
	    }
	}
    } else {
	t = findkeyword(cmd);
	if (t == nil) {
	    printf("\n");
	} else {
	    printf("%s\n", keywdstring(t));
	}
    }
}
