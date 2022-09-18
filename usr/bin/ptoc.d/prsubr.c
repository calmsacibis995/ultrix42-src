#ifndef lint
static char *sccsid = "@(#)prsubr.c	4.1	ULTRIX	7/17/90";
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
 * File: 	prsubr.c
 *
 * Pascal to C translator - Parser for:
 *			  - subroutines (procedures/functions)
 *			  - parameters 
 *			  - main program
 *			  - includes
 *			  - label declarations
 *			  - comments
 */

#include <stdio.h>
#include "ptoc.h"

#ifdef PRDEBUG
int prdebug = 1;
#define printd if (prdebug) fprintf
#define printd10 if (prdebug >= 10) fprintf
#endif

extern enum token nexttoken;	/* next Pascal token obtained as input */

extern struct scaninfo scandata;
extern int linecounter;
extern char tokenahead;
extern int linesize;				/* # of chars in inputline */
extern char ahead;				/* got 'nextchar' ahead flag */
extern char endofinput;
extern int charcounter;
extern FILE *fp;			/* file to scan from */
extern currfile[LINELENGTH];
extern char nextchar;
extern line inputline;
extern int doincl;			/* > 1 when processing include file */
extern struct treenode *procindex[MAXLEV];
extern struct stentry *stindex[MAXLEV];	/* sym table pts by lexic level */
extern int lexlev;				/* current lexical level */
extern struct fwdstmt *fwdhead;		/* head of list of fwd decls */
extern struct fwdstmt *fwdcurr;
extern int savecmt;			/* True when comment saved */

char gotprog;				/* True if program token scanned */
enum token holdtoken;
struct treenode *prevsub;		/* ptr to prev subroutine */

char *malloc();
struct stentry *getstentry();
char *getname();
struct pairs *getpairs();
struct stentry *findany();
struct treenode *gettn();
struct fwdstmt *getfwd();

/*
 * prog: Get a program stmt.  Symbol table head and tail are set up here.
 */

prog()
{
    gotprog = 0;
    lexlev = 0;
    stindex[0] = getstentry();	/* dummy header, not filled in */
    /*
     * The builtin symbols get defined before the program node, thus
     * they are not printed during code emiting.
     */
    builtins();
    scanner(0);
    if (nexttoken == PROGRAMT || nexttoken == MODULET)
	{
#       ifdef PRDEBUG
	printd(stderr,"prog: got program token\n");
#       endif
	if (nexttoken == PROGRAMT)
	    gotprog = 1;
	/*
	 * Scan program name, optional (input,output), up thru ";"
	 */
	for (; nexttoken != SEMICOLON ;)
	    scanner(0);
	savecmt = 1;
	scanner(0);
	}
    progseg(0);              /* get program segment */
    return(1);
}


/*
 * progseg:  Get a program segment (body).
 */

progseg(incl)
    char incl;			/* == 1 if processing an include file */
{
    struct treenode *tn;
    struct stentry *st;

    if (incl == 1)
	tn = procindex[0];
    else
	{
	tn = gettn();
	tn->type = PROGNODE;
	procindex[0] = tn;
	blkentry(tn);
	tn->firstc = tn->firstc->next;		/* no begin block for params */
	tn->firstc->next->next = NULL;		/* no begin block for stmts */
	lexlev = 1;
	/*
	 * Setup include of <stdio.h> and maxint symbol
	 */
	st = getstentry();
	st->st_name = getname(9);
	strcpy(st->st_name, "<stdio.h>");
	st->st_class = INCLUDEC;
	addsymbol(st);
	if (tn->firstc->firstsym == NULL)
	   tn->firstc->firstsym = st;
	tn->firstc->lastsym = st;

	st = getstentry();
	st->st_class = CONSTC;
	st->st_name = getname(6);
	strcpy(st->st_name, "maxint");
	st->st_cval = MAXINT;
	st->st_tipe = INTTY;
	addsymbol(st);
	if (tn->firstc->firstsym == NULL)
	   tn->firstc->firstsym = st;
	tn->firstc->lastsym = st;
	}
    /*
     * Pass ptr to decl_begin block
     */
	(void) commentseg(tn->firstc);
    (void) includeseg(tn->firstc);
    (void) labelseg();
	(void) commentseg(tn->firstc);
    (void) includeseg(tn->firstc);
    (void) constseg(tn->firstc);
	(void) commentseg(tn->firstc);
    (void) includeseg(tn->firstc);
    (void) typeseg(tn->firstc);
	(void) commentseg(tn->firstc);
    (void) includeseg(tn->firstc);
    /*
     * Add var "readln_dummy" to use for scanning thru "newline".
     * Needed for Pascal "readln".
     */
	st = getstentry();
	inittvf(st);
	st->st_name = getname(12);
	strcpy(st->st_name, "readln_dummy");
	st->st_lexlev = lexlev;
	st->st_dstruct = ARRS;
	st->st_tipe = CHARTY;
	st->st_class = VARC;
	st->st_numdims = 1;
	st->st_bounds = getpairs();
	st->st_bounds->pr_upper = 10;
	addsymbol(st);
	if (tn->firstc->firstsym == NULL)
	    tn->firstc->firstsym = st;
	tn->firstc->lastsym = st;
    (void) varseg(tn->firstc,tn);
	(void) commentseg(tn->firstc);
    (void) includeseg(tn->firstc);
    (void) fwdseg(tn->firstc->next);
	(void) commentseg(tn->firstc);
    (void) includeseg(tn->firstc);
    /*
     * Pass ptr to proc_begin block
     */
    prevsub = NULL;
    (void) subseg(tn->firstc->next, 0);
    if (incl == 0 && gotprog == 1)
	(void) mainstmts(tn->firstc->next);		/* create "main" proc */
}


/*
 * includeseg:  Get an include segment.
 */

includeseg(tn)
    struct treenode *tn;		/* ptr to decl begin block */
{
    struct stentry *st;			/* current var symbol */
    FILE *sfile;
    int slinesize;
    int slinecounter;
    int scharcounter;
    char sahead;
    char stokenahead;
    char snextchar;
    enum token snexttoken;
    line sinputline;
    char scurrfile[LINELENGTH];
    char unixform;			/* true if unix format include */

    /*
     * Accept Berkeley Pascal include syntax:  #include "file"
     *         and VMS Pascal include syntax:  %include 'file'
     */
    while (nexttoken == POUND || nexttoken == PERCENT)
	{
	if (nexttoken == POUND)
	    unixform = 1;
	else
	    unixform = 0;
	scanner(0);
	if (nexttoken != INCLUDET)
	    myexit(2,"include");
	if (unixform)
	    {
	    scanner(0);
	    if (nexttoken != QUOTE)
		myexit(2,"string");
	    }
	else
	    {
	    scanner(0);
	    if (nexttoken != CHARCONST)
		myexit(2,"string");
	    }
	st = getstentry();
	st->st_name = getname(strlen(scandata.si_name));
	strcpy(st->st_name, scandata.si_name);
	st->st_class = INCLUDEC;
	addsymbol(st);
	if (tn->firstsym == NULL)
	   tn->firstsym = st;
	tn->lastsym = st;
	savecmt = 1;
	scanner(0);
	savecmt = 0;
	if (nexttoken == COMMENT)
	    {
	    st->st_cmt = scandata.si_cmtptr;
	    /* comment loop here ? to get multiple comments in a row */
	    savecmt = 1;
	    scanner(0);
	    savecmt = 0;
	    commentseg(tn);
	    }
	/*
	 * Process include file
	 *
	 * Save scanner variables, to preserve state in present file.
	 */
	slinesize = linesize;
	slinecounter = linecounter;
	scharcounter = charcounter;
	sahead = ahead;
	stokenahead = tokenahead;
	snextchar = nextchar;
	snexttoken = nexttoken;
	strcpy(sinputline, inputline);
	strcpy(scurrfile, currfile);
	sfile = fp;
	/*
	 * See if include file can be opened.
	 */
	fp = fopen(st->st_name, "r");
	if (fp == NULL)
	    myexit(5,st->st_name);
	/*
	 * Re-initilize scanner variables for new file
	 */
	ahead = 0;
	tokenahead = 0;
	endofinput = 0;
	linecounter = 0;
	linesize = 0;
	charcounter = LINELENGTH;
	strcpy(currfile, st->st_name);

	doincl++;
	scanner(0);
	progseg(1);
	doincl--;
	fclose(fp);

	/*
	 * Restore scanner variables, to previous state.
	 */
	linesize = slinesize;
	linecounter = slinecounter;
	charcounter = scharcounter;
	ahead = sahead;
	tokenahead = stokenahead;
	endofinput = 0;
	nextchar = snextchar;
	nexttoken = snexttoken;
	strcpy(inputline, sinputline);
	strcpy(currfile, scurrfile);
	fp = sfile;
	}
}

/*
 * paramseg:  Get a param segment.
 */

paramseg(tn, funcst, parent)
    struct treenode *tn;		/* ptr to param begin block */
    struct stentry *funcst;		/* st entry for proc/func */
    struct treenode *parent;		/* for getting func/proc as param.
					   parent level for func definition */
{
    struct stentry *st;			/* current var symbol */
    struct stentry *secthead;		/* head of a section of var decls:
					   eg: v1,v2,v3: data-type */
    struct stentry *sect;		/* ptr to current stentry */
    struct stentry *dupst;		/* to fill out dupvar st_entryies */
    char buf[LINELENGTH];		/* to hold init value */
    int num = 0;			/* nparam counter */
    char var;
    char funcpar;			/* == 1 if getting func param */
    					/* == 2 if getting proc param */

    if (nexttoken == LEFTPAREN)
	{
#	ifdef PRDEBUG
	printd(stderr,"paramseg: got LEFTPAREN token\n");
#	endif
	savecmt = 0;
	scanner(0);			/* get VAR or id */
	do
	    {
	    var = 0;
	    funcpar = 0;
	    if (nexttoken == VART)
		{
		var = 1;
		scanner(0);
		}
	    secthead = NULL;
	    do
		{
		/*
		 * Check for VMS/Pascal "mechanism-specifier"
		 */
		if (nexttoken == PERCENT)
		    {
		    scanner(0);
		    if (nexttoken == MECHT)
			scanner(0);
		    else
			myexit(2, "mechanism-specifier");
		    }
		st = getstentry();
		inittvf(st);
		/*
		 * String together constructs like v1, v2, v3: type;
		 */
		if (secthead == NULL)
		    {
		    secthead = st;
		    sect = st;
		    }
		else
		    {
		    sect->st_dupvar = st;
		    sect = st;
		    }
		/*
		 * If func/proc as a parameter call subseg to get
		 *   the function decl set up:  Pass funcpar to indicate
		 *   that we're getting a func/proc parameter.
		 * When we return "prevsub" points to the dummy function
		 *   decl that we set up.  So we can fill in the type.
		 */
		if (nexttoken == FUNCTIONT)
		    funcpar = 1;
		if (nexttoken == PROCEDURET)
		    funcpar = 2;
		if (funcpar)
		    {
		    lexlev--;
		    subseg(parent,funcpar);
		    strcpy(scandata.si_name, prevsub->stdecl->st_name);
		    scandata.si_idlen = strlen(prevsub->stdecl->st_name);
		    lexlev++;
		    }
		st->st_name = getname(scandata.si_idlen);
		strcpy(st->st_name, scandata.si_name);
		st->st_class = VARC;
		if (var)
		    st->st_byref = 1;
		addsymbol(st);
		num++;
		if (tn->firstsym == NULL)
		    tn->firstsym = st;
		tn->lastsym =st;
		if (funcst->st_fparam == NULL)
		    funcst->st_fparam = st;
		funcst->st_lparam = st;
		/*
		 * Only get next token if not a proc/func parameter
		 * If it is a procedure parameter skip getting the type
		 *    (nexttoken == ";" or ")"
		 */
		if (funcpar == 0)
		    scanner(0);		/* get next token: , or : */
		else if (funcpar == 2)
			 goto nextparam;	/* forgive me! */
		if (nexttoken == COMMA)
		    scanner(0);
		}
	    while (nexttoken != COLON);
	    scanner(0);		/* get type */

	    /*
	     * Check for VMS format of:  "id: [attributes] type;"
	     */
	    if (nexttoken == LEFTBRACKET)
		{
		while (nexttoken != RIGHTBRACKET)
		    scanner(0);
		scanner(0);
		}

	    /*
	     * Fill in data type for head variable in this `section':
	     *    eg. v1, v2, v3: data-type;
	     */
	    if (!datatype(secthead))
		myexit(3,"data type not recognized");
	    /* 
	     * Make dupvar's "st_next" field's point to same place as
	     * the secthead's st_next.  Needed to find subfields under the
	     * dupvar, if the dupvar is of type record and gets used in a
	     * "with" stmt.
	     *
	     * Also save other "type" fields for help in processing "with"
	     * statements.
	     */
	    for (dupst = secthead->st_dupvar; dupst != NULL; dupst = dupst->st_dupvar)
		{
		dupst->st_next = secthead->st_next;
		dupst->st_lexlev = secthead->st_lexlev;
		dupst->st_dstruct = secthead->st_dstruct;
		dupst->st_tipe = secthead->st_tipe;
		dupst->st_class = secthead->st_class;
		dupst->st_uptr = secthead->st_uptr;
		dupst->st_numdims = secthead->st_numdims;
		dupst->st_bounds = secthead->st_bounds;
		}
nextparam:
	    /*
	     * Now that we have the func/proc parameter type go back
	     * and fill in the dummy func/proc decl type.
	     */
	    if (funcpar)
		{
		secthead->st_funcpar = funcpar;
		prevsub->stdecl->st_dstruct = secthead->st_dstruct;
		prevsub->stdecl->st_tipe = secthead->st_tipe;
		prevsub->stdecl->st_uptr = secthead->st_uptr;
		prevsub->stdecl->st_funcpar = funcpar;

		if (funcpar == 1)
		    {	/* function so fill in type entry */
		    prevsub->ftype->st_dstruct = secthead->st_dstruct;
		    prevsub->ftype->st_tipe = secthead->st_tipe;
		    prevsub->ftype->st_uptr = secthead->st_uptr;
		    prevsub->ftype->st_funcpar = 1;
		    prevsub->ftype->st_class = FUNCC;  /* need for emit */
		    }
		else	/* proc so use type int for the type in C */
		    secthead->st_tipe = INTTY;
		}
	    if (funcpar != 2)
		scanner(0);		/* get SEMI or ")" */
	    if (nexttoken != SEMICOLON && nexttoken != RIGHTPAREN)
		if (nexttoken == ASSIGNOP)
		    {
		    scanner(0);
		    /*
		     * Check for VMS/Pascal "mechanism-specifier"
		     */
		    if (nexttoken == PERCENT)
			{
			scanner(0);
			if (nexttoken == MECHT)
			    scanner(0);
			else
			    myexit(2, "mechanism-specifier");
			}
		    buf[0] = '\0';
		    getexpr(buf,0);
		    secthead->st_value = malloc(strlen(buf) + 1);
		    if (secthead->st_value == NULL)
			myexit(-1,"");
		    if (buf[0] == '(')
			{
			buf[0] = '{';
			if (buf[strlen(buf)-1] == ')')
			    buf[strlen(buf)-1] = '}';
			}
		    strcpy(secthead->st_value,buf);
		    }
		else
		    myexit(2,"; or )");
	    scanner(0);		/* get next token */
	    }
	while (nexttoken == IDENT || nexttoken == VART || nexttoken == PERCENT
	    || nexttoken == FUNCTIONT || nexttoken == PROCEDURET);
	funcst->st_nparams = num;
	return(1);
	}
    else
	return(0);
}


/*
 * labelseg:  Get a label segment.  Just scan it and ignore it, since
 * its unnecessary in C.
 */

labelseg()
{
    if (nexttoken == LABELT)
	{
#	ifdef PRDEBUG
	printd(stderr,"labelseg: got label token\n");
#	endif
	savecmt = 0;
	scanner(0);		/* get label */
	do
	    {
	    if (nexttoken != NUMCONST)
		myexit(2, "numeric constant");
	    scanner(0);		/* get next token */
	    if (nexttoken == COMMA)
		scanner(0);		/* get next token */
	    }
	while (nexttoken != SEMICOLON);
	savecmt = 1;
	scanner(0);		/* get next token */
	savecmt = 0;
	return(1);
	}
    else
	return(0);
}

/*
 * commentseg:  Get a comment.
 */

commentseg(tn)
    struct treenode *tn;		/* ptr to decl begin block */
{
    struct stentry *st;			/* symbol table entry for comment */

    if (nexttoken == COMMENT)
	{
#	ifdef PRDEBUG
	printd(stderr,"commentseg: got COMMENT token\n");
#	endif
	st = getstentry();
	st->st_class = COMTC;
	st->st_cmt = scandata.si_cmtptr;
	addsymbol(st);
	if (tn->firstsym == NULL)
	    tn->firstsym = st;
	tn->lastsym = st;
	savecmt = 0;
	scanner(0);		/* next token after comment */
	return(1);
	}
    else
	return(0);
}


/*
 * Look for a subroutine segment (procedures & functions)
 * If getting func/proc parameter we will just set up a function decl.
 *
 * NOTE: uses global "prevsub".  This is needed to handle nested Pascal
 *	procedures, and get them un-nested in C.
 */

subseg(parent, funcpar)
    struct treenode *parent;
    char funcpar;		/* 1 if getting func parameter */
    				/* 2 if getting proc parameter */
{
    struct stentry *st;
    struct treenode *tn;
    char proc;
    char fwd;			/* true if proc/funct found to be a fwd */
    struct fwdstmt *fptr;	/* ptr to forward list */
    char ext;

    /*
     * Check for VMS format of:  "[attributes] procedure/function name..."
     */
    if (nexttoken == LEFTBRACKET)
	{
	while (nexttoken != RIGHTBRACKET)
	    scanner(0);
	scanner(0);
	}
    for (; nexttoken == PROCEDURET || nexttoken == FUNCTIONT; )
	{
	if (nexttoken == PROCEDURET)
	    proc = 1;
	else
	    proc = 0;
	savecmt = 0;
	scanner(0);			/* get proc name */
	if (nexttoken != IDENT)
	    myexit(2,"identifier");
	/*
	 * See if the proc/funct was already declared with a forward
	 * statement.  If so point to the tree structure already built
	 * for the declaration.
	 */

	fwd = 0;
	if (funcpar == 0)
	    for (fptr = fwdhead->next; fptr != NULL; fptr = fptr->next)
		{
		if (strcmp(fptr->tree->stdecl->st_name, scandata.si_name) == 0)
		    {
		    fwd = 1;
		    break;
		    }
		}
	if (fwd)
	    tn = fptr->tree;
	else
	    {
	    tn = gettn();
	    if (proc)
		tn->type = PDECLNODE;
	    else
		tn->type = FDECLNODE;
	    }
	if (fwd)
	    {
	    for (;nexttoken != SEMICOLON;)
		scanner(0);
	    lexlev++;
	    /*
	     * Turn on "st_emit" flag in parameter entries that go
	     *    with the forward or external function.
	     * Also stuff a pointer to the parameters into the active
	     *    symbol table for this lexic level.
	     */
	    addsymbol(tn->firstc->firstsym);
	    for (st = tn->firstc->firstsym; st != NULL; st = st->st_link)
		{
		st->st_emit = 1;
		if (st == tn->firstc->lastsym)
		    break;
		}
	    }
	else
	    {
	    st = getstentry();
	    if (proc)
		st->st_class = PROCC;
	    else
		st->st_class = FUNCC;
	    st->st_nparams = 0;
	    st->st_name = getname(scandata.si_idlen);
	    strcpy(st->st_name, scandata.si_name);
	    tn->stdecl = st;
	    st->st_dstruct = NOSTRUCT;
	    st->st_tipe = INTTY;
	    addsymbol(st);
	    blkentry(tn);
	    lexlev++;
	    scanner(0);		/* get '(' or ';'  or ':' */
	    (void) paramseg(tn->firstc, st, parent);
	    if (!proc)
		{
		if (funcpar)
		    {
		    if (funcpar == 1)
			{
			st = getstentry();
			tn->ftype = st;
			/*
			 * We will fill in the function type back in
			 *   paramseg by using the prevsub pointer.
			 */
			 }
		    }
		else
		    {
		    if (nexttoken != COLON)
			myexit(2,":");
		    /*
		     * Get the function type.
		     * Also save the type info in the funct class stentry
		     * for use in forward/external function type decls, and
		     * for print formats from Pascal write stmts. 
		     */
		    scanner(0);		/* get funct type */
		    st = getstentry();
		    tn->ftype = st;
		    if (!datatype(st))
			myexit(3,"data type not recognized");
		    tn->stdecl->st_dstruct = st->st_dstruct;
		    tn->stdecl->st_tipe = st->st_tipe;
		    tn->stdecl->st_uptr = st->st_uptr;
		    scanner(0);		/* get ';' */
		    }
		}
	    }
	if (funcpar == 0)
	    {
	    savecmt = 1;
	    scanner(0);		/* get nexttoken AFTER ';' */
	    savecmt = 0;
		(void) commentseg(tn->firstc->next);
	    }
	/*
	 * Handle external declaration exactly as a forward declaration.
	 * This will create a function type declaration in C.
	 * Syntax: "function name (params): type; external;"
	 *   The reserved word table in sc.c returns the token EXTERNALT
	 *   for "extern" and "fortran" also.
	 */
	if (nexttoken == EXTERNALT || nexttoken == FORWARDT)
	    {
	    fwdcurr->next = getfwd();
	    fwdcurr = fwdcurr->next;
	    fwdcurr->tree = tn;
	    /*
	     * We want the "decl-begin" node (parent->prev->firstsym)
	     * to point to the proc id in the symbol table, so that
	     * a type definition will be produced for the proc id.
	     */
	    if (parent->prev->firstsym == NULL)
		parent->prev->firstsym = tn->stdecl;
	    parent->prev->lastsym = tn->stdecl;
	    scanner(0);
	    if (nexttoken == SEMICOLON)
		{
		savecmt = 1;
		scanner(0);
		savecmt = 0;
		(void) commentseg(tn->firstc->next);
		}
	    }
	else
	    {
	    if (parent->firstc == NULL)
		parent->firstc = tn;
	    parent->lastc = tn;
	    if (prevsub != NULL)
		prevsub->next = tn;
	    prevsub = tn;
	    /*
	     * If getting func/proc parameter, then we are done
	     * so return here.
	     */
	    if (funcpar)
		return;
	    (void) labelseg();
		(void) commentseg(tn->firstc->next);
	    (void) constseg(tn->firstc->next);
		(void) commentseg(tn->firstc->next);
	    (void) typeseg(tn->firstc->next);
		(void) commentseg(tn->firstc->next);
	    (void) varseg(tn->firstc->next,tn);
		(void) commentseg(tn->firstc->next);
	    /*
	     * Here's the clever part for un-nesting Pascal procedures.
	     * We want procs at same level, no nesting.  Global "prevsub"
	     * takes care of holding a pointer to the current proc tree
	     * node, so that it can be the next proc's prev.
	     */
	    (void) subseg(parent, 0);
	    savecmt = 1;
	    scanner(0);		/* skip over "begin" */
	    savecmt = 0;
	    (void) statelist(tn->lastc, FUNCLEVEL);
	    }
	stindex[lexlev] = NULL;
	lexlev--;
	/*
	 * Check for VMS format of:  "[attributes] procedure/function name..."
	 */
	if (nexttoken == LEFTBRACKET)
	    {
	    while (nexttoken != RIGHTBRACKET)
		scanner(0);
	    scanner(0);
	    }
	}
}

/*
 * Look for a forward segment.  If forward keyword is found, make tree
 * structure entry for the func/proc now.  Get its params and save the
 * func/proc st entry & param info under this tree node in the special
 * forward list.  Then this special tree entry can be retrieved later
 * when the function body is encountered.
 *
 * syntax: "forward procedure (params);"
 */

fwdseg(parent)
    struct treenode *parent;		/* ptr to decl begin block */
{
    struct treenode *prev = NULL;
    struct stentry *st;			/* for saving fwd info */
    struct treenode *tn2;		/* for saving fwd info */
    char proc;

    for (; nexttoken == FORWARDT; )
	{
	savecmt = 0;
	scanner(0);
	fwdcurr->next = getfwd();
	fwdcurr = fwdcurr->next;
	tn2 = gettn();
	fwdcurr->tree = tn2;
	if (nexttoken == PROCEDURET)
	    {
	    proc = 1;
	    tn2->type = PDECLNODE;
	    }
	else if (nexttoken == FUNCTIONT)
	    {
	    proc = 0;
	    tn2->type = FDECLNODE;
	    }
	else myexit(2,"procedure or function");
	st = getstentry();
	scanner(0);			/* get proc name */
	if (nexttoken != IDENT)
	    myexit(2,"identifier");
	if (proc)
	    st->st_class = PROCC;
	else
	    st->st_class = FUNCC;
	st->st_nparams = 0;
	st->st_name = getname(scandata.si_idlen);
	strcpy(st->st_name, scandata.si_name);
	tn2->stdecl = st;
	st->st_dstruct = NOSTRUCT;
	st->st_tipe = INTTY;
/*
	if (parent->firstsym == NULL)
	    parent->firstsym = st;
	parent->lastsym = st;
*/
	/*
	 * We want the "decl-begin" node (parent->prev->firstsym)
	 * to point to the proc id in the symbol table, so that
	 * a type definition will be produced for the proc id.
	 */
	if (parent->prev->firstsym == NULL)
	    parent->prev->firstsym = tn2->stdecl;
	parent->prev->lastsym = tn2->stdecl;

	addsymbol(st);
	blkentry(tn2);
	lexlev++;
	scanner(0);		/* get '(' or ';'  or ':' */
	(void) paramseg(tn2->firstc, st, parent);
	if (!proc)
	    {
	    if (nexttoken != COLON)
		myexit(2,":");
	    /*
	     * Get the function type.
	     * Also save the type info in the funct class stentry
	     * for use in forward/external function type decls, and
	     * for print formats from Pascal write stmts. 
	     */
	    scanner(0);
	    st = getstentry();
	    tn2->ftype = st;
	    if (!datatype(st))
		myexit(3,"data type not recognized");
	    tn2->stdecl->st_dstruct = st->st_dstruct;
	    tn2->stdecl->st_tipe = st->st_tipe;
	    tn2->stdecl->st_uptr = st->st_uptr;
	    scanner(0);		/* get ';' */
	    }
	savecmt = 1;
	scanner(0);		/* get nexttoken AFTER ';' */
	savecmt = 0;
	(void) commentseg(tn2->firstc->next);
	lexlev--;
	}
}
