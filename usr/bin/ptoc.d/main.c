#ifndef lint
static	char	*sccsid = "@(#)main.c	4.1	(ULTRIX)	7/17/90";
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
 * File: 	main.c
 *
 * Pascal to C languge translator - main
 *				  - Also contains several misc routines
 *				  - gettn(), blkentry(),
 *				  - symbol table manipulation routines.
 */

#include <stdio.h>
#include "ptoc.h"

/* Global variables */

enum token
    restoken[NRESERVES+1],		/* reserved words */
    chartoken[127+1],			/* legal one char tokens */
    nexttoken;				/* next Pascal token obtained as input */

int resindex[SYMBOLMAX+1];		/* index into resword for [n]-char
					   Pascal reserved words */
symbol resword[NRESERVES+1];		/* Pascal reserved words */
int resnext[NRESERVES+1];		/* link to next reserved word of [n] chars */

struct treenode *procindex[MAXLEV];
struct stentry *stindex[MAXLEV];
int lexlev;

int doincl;				/* incremented when doing include file */
char tokenahead;			/* 'nexttoken' ahead flag */
char ahead;				/* got 'nextchar' ahead flag */
char endofinput;
int charcounter;
int linecounter;
int linesize;				/* # of chars in inputline */
int savecmt;				/* indicates when we will save a cmt */
FILE *fp;				/* fp to current file begin processed */
char currfile[LINELENGTH];		/* name of current file */
struct scaninfo scandata;
struct fwdstmt *fwdhead;
struct fwdstmt *fwdcurr;

struct fwdstmt *getfwd();
struct stentry *findlev();
struct stentry *findst();
char *malloc();
char *calloc();


/***************************************************************************/


main(argc, argv)
    int argc;
    char *argv[];
{
    if (argc == 2)
	{
	if ((fp = fopen(argv[1], "r")) == NULL)
	    {
	    fprintf(stderr, "File %s failed to open\n", argv[1]);
	    return(-1);
	    }
	else
	    strcpy(currfile, argv[1]);
	}
    else
	{
	fprintf(stderr, "usage is ptoc filename\n");
	return(-1);
	}
    /*
     * Init reserved word data structures, init global variables, then call
     * the prog() routine,  to parse a Pascal program.
     */
    init();
    doincl = 0;
    ahead = 0;
    tokenahead = 0;
    endofinput = 0;
    linecounter = 0;
    linesize = 0;
    charcounter = LINELENGTH;  /* trigger 1st call to getline */
    fwdhead = getfwd();
    fwdcurr = fwdhead;

    prog();
    lexlev = 0;
    emitcode(procindex[0]);

    fprintf(stderr,"\n\n%d lines processed.\n", linecounter);
    return(0);
}


/*
 * Gettn.  Allocate space for a tree node entry (treenode) and
 * initialize key fields in it.  Then return a pointer to it.
 */

struct treenode *
gettn()
{
    struct treenode *tn;

    tn = (struct treenode *) malloc (sizeof(struct treenode));
    if (tn == NULL)
	myexit(-1,"");
    tn->firstlocal = NULL;
    tn->parent = NULL;
    tn->prev = NULL;
    tn->next = NULL;
    tn->firstc = NULL;
    tn->lastc = NULL;
    tn->stdecl = NULL;
    tn->expression = NULL;
    tn->variable = NULL;
    tn->initvalue = NULL;
    tn->finalvalue = NULL;
    return(tn);
}

/*
 * Blkentry.  Allocate tree nodes for the beginning of a new
 * procedure/function.
 */


blkentry(parent)
    struct treenode *parent;		/* parent tree node */
{
    register struct treenode *tn, *prev;
    register int i;

    prev = NULL;
    for (i = 0; i<4; i++)
	{
	tn = gettn();
	tn->type = BEGINNODE;
	tn->parent = parent;
	switch (i)
	    {
	    case 0:
		tn->blktype = PARAMBLOCK;
		parent->firstc = tn;
		break;
	    case 1:
		tn->blktype = DECLBLOCK;
		tn->prev = prev;
		prev->next = tn;
		break;
	    case 2:
		tn->blktype = NOBLOCK;
		tn->prev = prev;
		prev->next = tn;
		break;
	    case 3:
		tn->blktype = SUBRBLOCK;
		parent->lastc = tn;
		tn->prev = prev;
		prev->next = tn;
		break;
	    }
	prev = tn;
    }
}


/*
 * Set up a "fwdstmt" structure.
 */

struct fwdstmt *
getfwd()
{
    struct fwdstmt *fwdptr;

    fwdptr = (struct fwdstmt *) malloc(sizeof(struct fwdstmt));
    if (fwdptr == NULL)
	myexit(-1,"");
    fwdptr->next = NULL;
    fwdptr->tree - NULL;
    return(fwdptr);
}

#ifdef STDEBUG
int stdebug = 1;
#define printd if (stdebug) printf
#define printd10 if (stdebug >= 10) printf
#endif

/*
 * Getstentry.  Allocate space for a symbol table entry (stentry) and
 * initialize key fields in it.  Then return a pointer to it.
 */

struct stentry *
getstentry()
{
    struct stentry *st;
    st = (struct stentry *)malloc(sizeof(struct stentry));
    if (st == NULL)
	myexit(-1,"");
    st->st_link = NULL;
    st->st_name = NULL;
    st->st_dstruct = NOSTRUCT;
    st->st_tipe = NOTYPE;
    st->st_class = NOCLASS;
    st->st_lexlev = lexlev;
    st->st_uptr = NULL;
    st->st_cmt = NULL;
    if (doincl)
	st->st_emit = 0;
    else
	st->st_emit = 1;
    st->st_funcpar = 0;
    return(st);
}


/*
 * Getpairs.  Allocate space for a pairs record (subrange),
 * initialize key fields in it.  Then return a pointer to it.
 */

struct pairs *
getpairs()
{
    struct pairs *pr;
    pr = (struct pairs *)malloc(sizeof(struct pairs));
    if (pr == NULL)
	myexit(-1,"");
    pr->pr_next = NULL;
    pr->pr_lower = 0;
    pr->pr_upper = 0;
    pr->pr_luser = NULL;
    pr->pr_uuser = NULL;
    pr->pr_bound = NOTYPE;
    return(pr);
}


/*
 * Initialize type, var & field class, symbol table entries
 */

inittvf(st)
    struct stentry *st;
{
    st->st_numdims = 0;
    st->st_bounds = NULL;
    st->st_byref = 0;
    st->st_next = NULL;
    st->st_uptr = NULL;
    st->st_dupvar = NULL;
    st->st_funcvar = 0;
    st->st_value = NULL;
}


/*
 * Getname.  Allocate space for a symbol name within a symbol table
 * entry (stentry) and return a pointer to it.
 */

char *
getname(length)
    int length;			/* length of symbol (in bytes) */
{
    char *name;
/*
    name = calloc(length+1, 1);
*/
    name = malloc(length+1);	/* +1 for NUL which terminates string */
    if (name == NULL)
	myexit(-1,"");
    return(name);
}

/*
 * Addsymbol.  Add a symbol to the end of the present lexic level.
 */

addsymbol(st)
    struct stentry *st;
{
    struct stentry *sttmp;

    sttmp = stindex[lexlev];
    if (sttmp == NULL)
	stindex[lexlev] = st;
    else
	{
	for (; sttmp->st_link != NULL; sttmp = sttmp->st_link)
	    ;
	sttmp->st_link = st;
	}
}

/*
 * Find stentry & return a ptr to it.
 * If symbol is not found then return NULL ptr.
 * Entire symbol table is searched, from present lexic level on out.
 */

struct stentry *
findany(name)
    char *name;
{
    struct stentry *st;
    int i;

    for (i = lexlev; i >= 0; i--)
	{
	st = findst(name, i);
	if (st != NULL)
	    break;
	}
    return(st);
}


/*
 * Find stentry & return a ptr to it.
 * If symbol is not found then return NULL ptr.
 * Entire symbol table is searched, from GIVEN lexic level on out.
 */

struct stentry *
findlev(name, level)
    char *name;
    int level;
{
    struct stentry *st;
    int i;

    for (i = level; i >= 0; i--)
	{
	st = findst(name, i);
	if (st != NULL)
	    break;
	}
    return(st);
}


/*
 * Find stentry & return a ptr to it.
 * If symbol is not found then return NULL ptr.
 */

struct stentry *
findst(name, level)
    char *name;
    int level;
{
    struct stentry *st;

    for (st = stindex[level]; st != NULL && strcmp(st->st_name, name); st=st->st_link)
	;
    return(st);
}

/*
 * Getcmtinfo.  Allocate space for a "cmtinfo" structure and
 * initialize key fields in it.  Then return a pointer to it.
 */

struct cmtinfo *
getcmtinfo()
{
    struct cmtinfo *ci;
    ci = (struct cmtinfo *)malloc(sizeof(struct cmtinfo));
    if (ci == NULL)
	myexit(-1,"");
    ci->cmt = NULL;
    ci->next = NULL;
    return(ci);
}

/*
 * Convert an integer to a string of ASCII characters
 */

itoa(n,s)
    int n;		/* the integer to convert */
    char s[];		/* the string to place the ASCII chars in */
{
    int c,i,j;

    i = 0;
    do {
	s[i++] = n % 10 + '0';
    } while (( n = n / 10) > 0);
    s[i] = '\0';
    /*
     * Reverse string (since its backwards)
     */
    for (i = 0, j = strlen(s)-1;  i < j;  i++, j--)
	{
	c = s[i];
	s[i] = s[j];
	s[j] = c;
	}
}
