#ifndef lint
static	char	*sccsid = "@(#)prdecl.c	4.1	(ULTRIX)	7/17/90";
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
* File: 	prdecl.c
*
* Pascal to C translator - Parser for declarations
*			 - Const, type & variable declarations
*/

/*
 *	TO DO
 *
 *  types of:  array of complex type, array bounds of
 *		enumeration in place: [(red,blue,green)]
 */

/*	Modification History
 *
 * 21-July-87 afd
 *	If a const value is more than 1 char, then call it a string (STRINGTY)
 *
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
extern char tokenahead;
extern struct stentry *stindex[MAXLEV];	/* sym table pts by lexic level */
extern int lexlev;			/* current lexical level */
extern int savecmt;			/* True when comment saved */
extern int linecounter;		/* for myexit() */
extern char currfile[LINELENGTH];	/* file name for myexit() */

enum token holdtoken;

char *malloc();
struct stentry *getstentry();
char *getname();
struct pairs *getpairs();
struct stentry *findany();
struct treenode *gettn();
struct cmtinfo *getcmtinfo();

/*
 * constseg:  Get a constant segment.
 */

constseg(tn)
    struct treenode *tn;		/* ptr to decl begin block */
{
    struct stentry *st;			/* current type symbol */
    char negative;

    if (nexttoken == CONSTT)
	{
#	ifdef PRDEBUG
	printd(stderr,"constseg: got CONST token\n");
#	endif
	savecmt = 1;
	scanner(0);		/* get const id */
	savecmt = 0;
	commentseg(tn);
	do
	    {
	    st = getstentry();
	    st->st_name = getname(scandata.si_idlen);
	    strcpy(st->st_name, scandata.si_name);
	    st->st_class = CONSTC;
	    addsymbol(st);
	    if (tn->firstsym == NULL)
		tn->firstsym = st;
	    tn->lastsym = st;

	    savecmt = 0;
	    scanner(0);		/* get = */
	    scanner(0);
	    negative = 0;
	    if (nexttoken == PLUS)
		{
		scanner(0);
		}
	    else
		if (nexttoken == MINUS)
		    {
		    negative = 1;
		    scanner(0);
		    }
	    if (nexttoken != IDENT)
		switch (nexttoken)
		    {
		    case NUMCONST:
			if (scandata.si_dflag == 2)
			    {
			    st->st_fcval = scandata.si_cvalue;
			    if (negative)
				st->st_fcval = -st->st_fcval;
			    st->st_tipe = REALTY;
			    }
			else
			    {
			    st->st_cval = scandata.si_cvalue;
			    if (negative)
				st->st_cval = -st->st_cval;
			    st->st_tipe = INTTY;
			    }
			break;
		    case CHARCONST:
			if (scandata.si_idlen != 1) { /* string */
				st->st_string = getname(scandata.si_idlen);
	    			strcpy(st->st_string, scandata.si_name);
				st->st_tipe = STRINGTY;
			} else {
				st->st_string = NULL;
				st->st_cval = scandata.si_name[0];
				st->st_tipe = CHARTY;
			}
			break;
		    case TRUET:
			st->st_cval = 1;
			st->st_tipe = INTTY;
			break;
		    case FALSET:
			st->st_cval = 0;
			st->st_tipe = INTTY;
			break;
		    case PERCENT:
			scanner(0);
			if (nexttoken != IDENT)
			    myexit(3,"bad constant");
			if ((strcmp(scandata.si_name,"o") == 0) ||
			    (strcmp(scandata.si_name,"O") == 0))
			    st->st_tipe = OCTALTY;
			else
			    if ((strcmp(scandata.si_name,"x") == 0) ||
				(strcmp(scandata.si_name,"X") == 0))
				st->st_tipe = HEXTY;
			    else myexit(3,"bad constant");
			/*
			 * Call scanner to get octal or hex const within
			 * apostrophes.  Use st_uptr to point to the
			 * constant string.
			 */
			scanner(0);
			if (nexttoken != CHARCONST)
			    myexit(3,"bad constant");
	    		st->st_uptr = (struct stentry *) getname(scandata.si_idlen);
	    		strcpy(st->st_uptr, scandata.si_name);
			st->st_cval = 0;
			break;
		    default:
			myexit(3,"bad constant");
		    }
	    else
		{
		st->st_tipe = USERTYPE;
		st->st_uptr = findany(scandata.si_name);
		}
	    scanner(0);		/* get SEMI */
	    if (nexttoken != SEMICOLON)
		myexit(2,";");
	    savecmt = 1;
	    scanner(0);		/* get next token */
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
	    }
	while (nexttoken == IDENT);
	return(1);
	}
    else
	return(0);
}


/*
 * typeseg:  Get a type segment.
 */

typeseg(tn)
    struct treenode *tn;		/* ptr to decl begin block */
{
    struct stentry *st;			/* current type symbol */
    struct stentry *tmpst;		/* temporary stptr */

    if (nexttoken == TYPET)
	{
#	ifdef PRDEBUG
	printd(stderr,"typeseg: got TYPE token\n");
#	endif
	savecmt = 1;
	scanner(0);			/* get type id */
	savecmt = 0;
	commentseg(tn);
	do
	    {
	    st = getstentry();
	    inittvf(st);
	    st->st_name = getname(scandata.si_idlen);
	    strcpy(st->st_name, scandata.si_name);
	    st->st_class = TYPEC;
	    addsymbol(st);
	    if (tn->firstsym == NULL)
		tn->firstsym = st;
	    tn->lastsym = st;
	    savecmt = 0;
	    scanner(0);		/* get = */
	    scanner(0);
	    /*
	     * Check for VMS format of:  "id = [attributes] type;"
	     */
	    if (nexttoken == LEFTBRACKET)
		{
		while (nexttoken != RIGHTBRACKET)
		    scanner(0);
		scanner(0);
		}
	    if (!datatype(st))
		myexit(3,"data type not recognized");
	    /*
	     * If the type was an enumerated type we must update
	     * tn->lastsym to point to the last enumeration constant.
	     */
	    if (st->st_dstruct == UDEFS && st->st_tipe == ENUMTY)
		{
		for (tmpst = st->st_uptr; tmpst->st_link != NULL; tmpst = tmpst->st_link)
		    ;
		tn->lastsym = tmpst;
		}
	    scanner(0);		/* get SEMI */
	    if (nexttoken != SEMICOLON)
		myexit(2,";");
	    savecmt = 1;
	    scanner(0);		/* get next token */
	    savecmt = 0;
	    if (nexttoken == COMMENT)
		{
		st->st_cmt = scandata.si_cmtptr;
		/* comment loop here to get multiple comments in a row */
		savecmt = 1;
		scanner(0);
		savecmt = 0;
		commentseg(tn);
		}
	    }
	while (nexttoken == IDENT);
	return(1);
	}  /* if (nexttoken == TYPET) */
    else
	return(0);
}

/*
 * varseg:  Get a var segment.
 */

varseg(tn, functn)
    struct treenode *tn;		/* ptr to decl begin block */
    struct treenode *functn;		/* ptr to func/proc tree node */
{
    struct stentry *st;			/* current var symbol */
    struct stentry *secthead;		/* head of a section of var decls:
					   eg: v1,v2,v3: data-type */
    struct stentry *sect;		/* ptr to current stentry */
    struct stentry *funcst;		/* ptr to func/proc stentry */
    struct stentry *dupst;		/* to fill out dupvar st_entryies */
    char buf[LINELENGTH];		/* to hold init value */

    /*
     * If function (not proc or main) then set up a dummy var with
     * same name as the function name to take Pascal assignments to the
     * function.
     */
    if (functn->type == FDECLNODE)
	{
	st = getstentry();
	inittvf(st);
	funcst = functn->stdecl;
	st->st_name = getname(strlen(funcst->st_name));
	strcpy(st->st_name, funcst->st_name);
	funcst = functn->ftype;
	st->st_lexlev = lexlev;
	st->st_dstruct = funcst->st_dstruct;
	st->st_tipe = funcst->st_tipe;
	st->st_class = VARC;
	st->st_uptr = funcst->st_uptr;
	st->st_numdims = funcst->st_numdims;
	st->st_bounds = funcst->st_bounds;
	st->st_funcvar = 1;
	addsymbol(st);
	if (tn->firstsym == NULL)
	    tn->firstsym = st;
	tn->lastsym = st;
	}
    if (nexttoken == VART)
	{
#	ifdef PRDEBUG
	printd(stderr,"typeseg: got VAR token\n");
#	endif
	savecmt = 1;
	scanner(0);			/* get var id */
	savecmt = 0;
	commentseg(tn);
	do
	    {
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

		st->st_name = getname(scandata.si_idlen);
		strcpy(st->st_name, scandata.si_name);
		st->st_class = VARC;
		addsymbol(st);
		if (tn->firstsym == NULL)
		    tn->firstsym = st;
		tn->lastsym = st;
		savecmt = 0;
		scanner(0);		/* get next token */
		if (nexttoken == COMMA)
		    scanner(0);
		}
	    while (nexttoken != COLON);
	    scanner(0);

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
	     * If the type was an enumerated type we must update
	     * tn->lastsym to point to the last enumeration constant.
	     */
	    if (secthead->st_dstruct == UDEFS && secthead->st_tipe == ENUMTY)
		{
		for (dupst = secthead->st_uptr; dupst->st_link != NULL; dupst = dupst->st_link)
		    ;
		tn->lastsym = dupst;
		}
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

	    scanner(0);		/* get SEMI */
	    if (nexttoken != SEMICOLON)
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
		    myexit(2,";");
	    savecmt = 1;
	    scanner(0);		/* get next token */
	    savecmt = 0;
	    if (nexttoken == COMMENT)
		{
		secthead->st_cmt = scandata.si_cmtptr;
		/* comment loop here ? to get multiple comments in a row */
		savecmt = 1;
		scanner(0);
		savecmt = 0;
		commentseg(tn);
		}
	    }
	while (nexttoken == IDENT || nexttoken == PERCENT);
	return(1);
	}
    else
	return(0);
}


/*
 * datatype:  Get a datatype.
 * <data_type> ::= <simple_type> | <array_type> | ^<id> | file of <data_type> |
 *              set of <simple_type> | record <filedlist> end
 */

datatype(st)
    struct stentry *st;
{
    if (nexttoken == PACKED)
	scanner(0);
    if (!simpletype(st))
	if (!arraytype(st))
	    if (nexttoken == UPARROW)
		{
		st->st_dstruct = PTRS;
		scanner(0);		/* get id */
		if (!simpletype(st))
		    myexit(3,"bad pointer type");
		}
	    else
		if (nexttoken == FILET)
		    {
		    scanner(0);		/* get 'of' */
		    scanner(0);
		    if (!datatype(st))
			myexit(3,"data type not recognized");
		    st->st_dstruct = FILESTR;	/* MUST follow call for type
						   to set back struct type */
		    }
		else
		    if (nexttoken == SETT)
			{
			scanner(0);	/* get 'of' */
			if (nexttoken != OFT)
			    myexit(2,"of");
			scanner(0);
			if (!simpletype(st))	/* get type of the set */
			    myexit(3,"bad set type");
			st->st_dstruct = SETS;	/* MUST follow call for type
						   to set back struct type */
			}
		    else
			if (nexttoken == RECORDT)
			    {
			    st->st_dstruct = RECORDS;
			    lexlev++;
			    fieldlist(st,0);
			    stindex[lexlev] = NULL;	/* terminate inner level */
			    lexlev--;
			    }
			else
			    return(0);		/* no datatype */
    return(1);
}

/*
 * <simple_type> ::= integer | boolean | char | real | <id> | ( <id> {,<id>}*) |
 *                  <constant> .. <constant> | <const-id> .. <const-id>
 */

simpletype(st)
    struct stentry *st;
{
    struct stentry *hold;	/* hold a ptr to the enum sym table entry */
    char holdname[LINELENGTH];	/* hold last symbol name for 'look-ahead' */
    int i;			/* number of enum constants */

    if (nexttoken == MINUS || nexttoken == PLUS)
	scanner(1);		/* could be negative subrange */
    switch (nexttoken) {
    case BOOLEANT:
	st->st_tipe = BOOLTY;
	break;
    case DOUBLE:
	st->st_tipe = DOUBLETY;
	break;
    case CHART:
	st->st_tipe = CHARTY;
	break;
    case INTEGERT:
	st->st_tipe = INTTY;
	break;
    case REALT:
	st->st_tipe = REALTY;
	break;
    case UNSIGNT:
	st->st_tipe = UNSIGNEDTY;
	break;
    case IDENT:
	holdtoken = nexttoken;
	strcpy(holdname,scandata.si_name);
	scanner(1);	/* 1 for no reals, despite '.' */
	if (nexttoken == DOTDOT)
	    {
	    st->st_dstruct = SUBRANGES;
	    nexttoken = holdtoken;
	    getrange(st,1);		/* 1=lower bound */
	    scanner(0);			/* get next const */
	    if (nexttoken == MINUS)
		scanner(1);
	    getrange(st,0);		/* 0=upper bound */
	    }
	else
	    {
	    tokenahead = 1;
	    strcpy(scandata.si_name,holdname);
	    st->st_tipe = USERTYPE;
	    if (st->st_dstruct == NOSTRUCT)
		st->st_dstruct = UDEFS;
	    st->st_uptr = findany(scandata.si_name);
	    /*
	     * If ptr is to an object that is not defined yet, then create
	     * a dummy stentry to store the objects name.
	     */
	    if (st->st_uptr == NULL)
		{
		if (st->st_dstruct == PTRS)
		    {
		    st->st_uptr = getstentry();
		    st->st_uptr->st_name = getname(scandata.si_idlen);
		    strcpy(st->st_uptr->st_name, scandata.si_name);
		    fprintf(stderr, "\"%s\", line %d: Warning: Pointer to an object that is not defined yet.\n", currfile, linecounter);
		    fprintf(stderr, "     This is illegal in C.\n");

		    }
		else
		    myexit(1,scandata.si_name);
		}
	    }
	break;
    case LEFTPAREN:		/* Enumerated type */
	st->st_dstruct = UDEFS;
	st->st_tipe = ENUMTY;
	scanner(0);		/* get id */
	if (nexttoken != IDENT)
	    myexit(2,"identifier");
	i = 1;
	hold = st;
	st = getstentry();
	st->st_name = getname(scandata.si_idlen);
	strcpy(st->st_name, scandata.si_name);
	st->st_class = CONSTC;
	st->st_tipe = ENUMTY;
	st->st_cval = i-1;
	st->st_enumptr = hold;	/* pt back to enum type */
	hold->st_uptr = st;	/* pt enum type to 1st const */
	addsymbol(st);
	scanner(0);
	for (; nexttoken == COMMA ;)
	    {
	    scanner(0);			/* get id */
	    if (nexttoken != IDENT)
		myexit(2,"identifier");
	    i++;
	    st = getstentry();
	    st->st_name = getname(scandata.si_idlen);
	    strcpy(st->st_name, scandata.si_name);
	    st->st_class = CONSTC;
	    st->st_tipe = ENUMTY;
	    st->st_cval = i-1;
	    st->st_enumptr = hold;
	    addsymbol(st);
	    scanner(0);			/* comma or left paren */
	    }
	if (nexttoken != RIGHTPAREN)
	    myexit(2,")");
	hold->st_numdims = i;
	break;
    case NUMCONST:		/* SUBRANGE */
    case CHARCONST:
	st->st_dstruct = SUBRANGES;
	getrange(st,1);		/* 1=lower bound */
	scanner(0);		/* get .. */
	scanner(0);		/* get next const */
	if (nexttoken == MINUS || nexttoken == PLUS)
	    scanner(1);
	getrange(st,0);		/* 0=upper bound */
	break;
    default:
	return(0);
    }
    return(1);
}


/*
 * <array_type> ::= 
 *       [ packed ] array [ <simple_type> {,<simple_type> *} ] of <simple_type>
 *                        ^                                  ^       ^
 *                                                                   |
 *					          should really be "datatype"
 *
 * RESTRICTIONS:
 *   1. We only deal with the upper bound since all C arrays start at 0 and go
 *	up.  Thus if negative Pascal Bounds are used, they are ignored!
 *   2. We only handle arrays of simple types.
 */

arraytype(st)
    struct stentry *st;
{
    struct pairs *pr, *newpr;
    struct stentry *stptr;
    int numdims = 1;

    if ((nexttoken != ARRAYT) && (nexttoken != VARYING))
	return(0);
    else {
	scanner(0); 	/* get [ */
	if (nexttoken != LEFTBRACKET)
	    myexit(2,"[");
	else {
	    st->st_dstruct = ARRS;
	    for (;;)
		{
		scanner(1);	/* 1st bound (1 for no reals, despite '.') */
		if (nexttoken == MINUS)
		    scanner(1);
		holdtoken = nexttoken;
		scanner(0);	/* .. or , or ] */
		switch (nexttoken) {
		    case DOTDOT:
			scanner(0);	/* upper bound */
			if (nexttoken == MINUS)
			    scanner(1);
			holdtoken = nexttoken;
			scanner(0);	/* , or ] */
			/* fall into */
		    case COMMA:
		    case RIGHTBRACKET:
			newpr = getpairs();
			if (numdims == 1)
			    st->st_bounds = newpr;
			else
			    pr->pr_next = newpr;
			pr = newpr;
			switch (holdtoken) {
			    case IDENT:
				if ((stptr = findany(scandata.si_name)) == NULL)
				    myexit(1,scandata.si_name);
				pr->pr_uuser = stptr;
				if (stptr->st_class == CONSTC)
				    pr->pr_upper = -1;  /* flag to use const name */
				else if (stptr->st_bounds != NULL)
					 {
				         if (stptr->st_bounds->pr_uuser != NULL && 
					     stptr->st_bounds->pr_uuser->st_class == CONSTC)
					     {
					     pr->pr_upper = -1;  /* flag to use const name */
					     pr->pr_uuser = stptr->st_bounds->pr_uuser;
					     }
					 else
					     pr->pr_upper = stptr->st_bounds->pr_upper + 1;
					 }
				else if (stptr->st_dstruct == UDEFS &&
				         stptr->st_tipe == ENUMTY)
					 pr->pr_upper = stptr->st_numdims;
				break;
			    case CHARCONST:
				pr->pr_upper = scandata.si_name[0] + 1;
				break;
			    case NUMCONST:
				pr->pr_upper = scandata.si_cvalue + 1;
				break;
			    case BOOLEANT:
				pr->pr_upper = 2;
				break;
			    default:
				myexit(3,"bad array dimension");
			    }	/* switch 2 */
			break;
		    default:
			myexit(3,"bad array dimension");
		    }	/* switch 1 */
		if (nexttoken == RIGHTBRACKET)
		    break;
		numdims++;
		}   /* for */
	    st->st_numdims = numdims;
	    scanner(0);
	    if (nexttoken != OFT)
		myexit(2,"of");
	    scanner(0);	/* type */
	    if (!simpletype(st))
		{
		st->st_tipe = INTTY;
		fprintf(stderr, "\"%s\", line %d: Warning: Array of complex type not supported, using integer\n", currfile, linecounter);
		while (nexttoken != SEMICOLON)
		    scanner(0);
		tokenahead = 1;
		}
	    }	/* nexttoken == LEFTBRACKET */
	}   /* nexttoken == ARRAYT */
}


/*
 * Getrange.  Get subrange bounds.
 * Call with st = the stentry for the type id.  The pairs record will
 *   be set up here.
 */

getrange(st, lower)
    struct stentry *st;
    char lower;			/* 1 = getting lower, else upper */
{
    struct pairs *pr;

    if (lower)
	{
	pr = getpairs();
	st->st_bounds = pr;
	}
    else
	pr = st->st_bounds;
    switch (nexttoken) {
	case NUMCONST:
	    if (lower)
		{
		st->st_tipe = INTTY;		/* Base type of subrange */
		pr->pr_lower = scandata.si_cvalue;
		pr->pr_bound = INTTY;
		}
	    else
		pr->pr_upper = scandata.si_cvalue;
	    break;
	case CHARCONST:
	    if (lower)
		{
		st->st_tipe = CHARTY;		/* Base type of subrange */
		pr->pr_lower = scandata.si_name[0];
		pr->pr_bound = CHARTY;
		}
	    else
		pr->pr_upper = scandata.si_name[0];
	    break;
	case IDENT:
	    if (lower)
		{
		pr->pr_luser = findany(scandata.si_name);
		if (pr->pr_luser == NULL)
		    myexit(1,scandata.si_name);
		if (pr->pr_luser->st_tipe == ENUMTY &&
		    pr->pr_luser->st_class == CONSTC)
		    {
		    st->st_tipe = USERTYPE;
		    st->st_uptr = pr->pr_luser->st_enumptr;
		    }
		else
		    st->st_tipe = pr->pr_luser->st_tipe; /* Base type of subrange */
		pr->pr_bound = USERTYPE;
		}
	    else
		{
		pr->pr_uuser = findany(scandata.si_name);
		if (pr->pr_uuser == NULL)
		    myexit(1,scandata.si_name);
		pr->pr_upper = pr->pr_uuser->st_cval;	/* so value can be
							   obtained for
							   array bound*/
		}
	    break;
	default:
	    myexit(3,"bad subrange bound");
    };
}

fieldlist(st,varec)
    struct stentry *st;
    int varec;			/* true if in varient record */
{
    int numdims = 0;		/* syntactically legal to have no fields */
    struct stentry *rechead;	/* hold ptr to fill in # of fields */
    struct stentry *secthead;	/* head of a sect of fields: f1,f2: type; */
    struct stentry *sect;	/* to build up this "section" */
    struct stentry *save_st;	/* save ptr to prior field & fill in "st_next" */
    struct stentry *dupst;	/* to fill out dupvar st_entryies */
    int undims = 0;		/* number of fields in varient record (union) */
    struct stentry *unhead;	/* hold ptr to union field */
    char holdname[LINELENGTH];	/* hold last symbol name for 'look-ahead' */
    char lookahead = 0;		/* set true for varient record look ahead */
    char union_name[LINELENGTH]; /* name of union to pass to 'dodefines' */

    rechead = st;
    scanner(0);			/* get 1st field id */
    do
	{
	secthead = NULL;
	do
	    {
	    if (nexttoken != IDENT)
		if ((nexttoken == ENDT) || (varec && nexttoken == RIGHTPAREN))
		    {
		    rechead->st_numdims = numdims;
		    return(1);
		    }
		else
		    if (nexttoken == CASET)
			goto docase;
		    else
			myexit(2,"end");
	    numdims++;
	    save_st = st;
	    st = getstentry();
	    inittvf(st);

	    addsymbol(st);	/* ASDEP did this in-line here */

	    /*
	     * If this is the first field name of a given field stmt,
	     *   then set up "secthead" and link "st_next".
	     * Else,
	     *   string together constructs like f1, f2, f3: type;
	     *   with "st_dupvar".  We don't want the "st_next" link
	     *   between "dupvar's" (because "st_next" is the way to
	     *   get from a section head to a nested record).
	     */
	    if (secthead == NULL)
		{
		secthead = st;
		sect = st;
		if (save_st->st_next == NULL)
		    save_st->st_next = st;
		}
	    else
		{
		sect->st_dupvar = st;
		sect = st;
		}

	    st->st_name = getname(scandata.si_idlen+1);	/* +1 for "u" in unions */
	    if (varec)
		strcpy(st->st_name, "u");
	    else
		strcpy(st->st_name, "");
	    strcat(st->st_name, scandata.si_name);
	    st->st_class = FIELDC;
	    savecmt = 0;
	    /*
	     * If we were a token ahead from varient record look-ahead (below)
	     * then we've already scanned the ":" so just set nexttoken.
	     */
	    if (lookahead)
		{
		nexttoken = COLON;
		lookahead = 0;
		}
	    else
		scanner(0);		/* get next token */
	    if (nexttoken == COMMA)
		scanner(0);
	    }
	while (nexttoken != COLON);
	
	scanner(0);
	/*
	 * Fill in data type for "head" variable in this `section':
	 *    eg. f1, f2, f3: data-type;
	 */
	if (!datatype(secthead))
	    myexit(3,"data type not recognized");
	/* 
	 * Make dupvar's "st_next" field's point to same place as
	 * the secthead's st_next.  Needed to find subfields under the
	 * dupvar, if the dupvar is of type record and get used in a
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

	scanner(0);		/* get SEMI (or "end" or ")" ) */
	if (nexttoken == SEMICOLON)
	    {
	    savecmt = 1;
	    scanner(0);		/* get next token (IDENT or END) */
	    savecmt = 0;
	    if (nexttoken == COMMENT)
		{
		secthead->st_cmt = scandata.si_cmtptr;
		savecmt = 0;
		scanner(0);
		}
	    }
docase:
	if (nexttoken == CASET)
	    {	/* handle varient field of record */
	    scanner(0);
	    /*
	     * Verify type of case variable
	     */
	    switch (nexttoken) {
	    case BOOLEANT:
	    case CHART:
	    case INTEGERT:
	    case IDENT:
		 break;
	    default:
		 myexit(2,"scaler type");
		 break;
	    }
	    /*
	     * Look ahead to distinguish between the 2 legal formats:
	     *     case var: type of
	     *     case scaler-type of
	     */
	    strcpy(holdname,scandata.si_name);
	    scanner(0);			/* look ahead */
	    if (nexttoken != OFT)
		if (nexttoken == COLON)
		    {
		    nexttoken = IDENT;
		    strcpy(scandata.si_name,holdname);
		    lookahead = 1;
		    }
		else
		    myexit(2,":");
	    }
	}
    while (nexttoken == IDENT);

    /*
     * Handle fields within varient part of a record.
     * In C we will have a union with each Pascal "tag" section
     * making a struct within the union.
     */
    if (nexttoken == OFT)
	{
	numdims++;
	save_st = st;
	/*
	 * Create C union, named "un"
	 */
	st = getstentry();
	inittvf(st);
	if (save_st->st_next == NULL)
	    save_st->st_next = st;
	st->st_name = getname(5);
	strcpy(st->st_name, "un_");
	itoa(varec,scandata.si_name);
	strcat(st->st_name, scandata.si_name);
	st->st_class = FIELDC;
	st->st_dstruct = RECORDS;
	st->st_tipe = UNIONTY;
	addsymbol(st);
	unhead = st;
	undims = 0;
	lexlev++;
	scanner(0);
	varec++;
	/*
	 * Loop until end of Pascal varient record.
	 * Each Pascal tag field section, will be a struct in C.
	 * These structs get dummy names: un1, un2, un3, etc.
	 */
	while ((nexttoken != ENDT) && (!(varec && nexttoken == RIGHTPAREN)))
	    {
	    while (nexttoken != LEFTPAREN)
		scanner(0);
	    undims++;
	    st = getstentry();
	    inittvf(st);
	    if (unhead->st_next == NULL)
		unhead->st_next = st;
	    st->st_name = getname(6);
	    strcpy(st->st_name, "un");
	    itoa(undims,scandata.si_name);
	    strcat(st->st_name, scandata.si_name);
	    st->st_class = FIELDC;
	    st->st_dstruct = RECORDS;
	    addsymbol(st);
	    lexlev++;
	    /*
	     * Recursive call to fieldlist to get the fields under
	     * this varient tag section.
	     */
	    fieldlist(st,varec);
	    stindex[lexlev] = NULL;	/* terminate inner level */
	    lexlev--;
	    scanner(0);		/* get ";" or "end" */
	    if (nexttoken == SEMICOLON)
		{
		savecmt = 1;
		scanner(0);
		savecmt = 0;
		if (nexttoken == COMMENT)
		    {
		    st->st_cmt = scandata.si_cmtptr;
		    scanner(0);
		    }
		}
	    }	/* end while nexttoken != ENDT */
	unhead->st_numdims = undims;
	stindex[lexlev] = NULL;  /* terminate union level */
	lexlev--;
	/*
	 * If we finished the highest lexic level varient (varec == 1)
	 * then make stentries for "defines".  This way the added C union &
	 * inner struct levels will be transparent to the program code.
	 */
	if (varec == 1)
	    {
	    for (st = rechead->st_next; st->st_tipe != UNIONTY; st = st->st_link)
		if (st == NULL)
		    myexit(3,"internal error in dodefines");
	    strcpy(union_name,"");
	    lexlev--;
	    dodefines(st,union_name);
	    lexlev++;
	    }
	varec--;
	}   /* end if nexttoken == OFT */

    if (nexttoken != ENDT && nexttoken != RIGHTPAREN)
	myexit(2,"end or )");
    rechead->st_numdims = numdims;
}

/*
 * Make st entries for "defines".  This way the added C union &
 * inner struct levels will be transparent to the program code.
 */

dodefines(unionptr,union_name)
    struct stentry *unionptr;
    char *union_name;
{
    register struct stentry *st, *dst, *subrecst;
    register int j, subrec_num, i, union_num;
    char definestr[LINELENGTH];

    /*
     * Get the union name
     */
    strcat(union_name, unionptr->st_name);
    strcat(union_name, ".");
    union_num = unionptr->st_numdims;
    st = unionptr->st_next;

    /*
     * Get ptr to inner record
     */
    subrecst = st;

    for (i = 0; i < union_num; i++, subrecst = subrecst->st_link)
	{
	subrec_num = subrecst->st_numdims;
	if (subrec_num == 0)
	    continue;
	/*
	 * Get the inner record name
	 */
	strcpy(definestr, union_name);
	strcat(definestr, subrecst->st_name);
	strcat(definestr, ".");
	st = subrecst->st_next;
	for (j = 0; j < subrec_num; j++)
	    {
	    if (st->st_tipe == UNIONTY)
		{
		dodefines(st, definestr);
		continue;
		}
	    dst = getstentry();
	    inittvf(dst);
	    dst->st_class = DEFINEC;
	    dst->st_name = getname(strlen(st->st_name));
	    strcpy(dst->st_name, st->st_name+1);	/* skip over "u" */
	    dst->st_cmt = getcmtinfo();
	    dst->st_cmt->cmt = getname(strlen(definestr)+strlen(st->st_name));
	    strcpy(dst->st_cmt->cmt, definestr);
	    strcat(dst->st_cmt->cmt, st->st_name);
	    addsymbol(dst);

	    st = st->st_link;
	    /* skip over enum constants */
	    }
	}
}
