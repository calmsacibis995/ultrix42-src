#ifndef lint
static	char	*sccsid = "@(#)prexpr.c	4.1	(ULTRIX)	7/17/90";
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
 * File: 	prexpr.c
 *
 * Pascal to C translator - Parser for expressions - getexpr()
 *			  - Also contains writestmt(), mainstmts(), myexit()
 */

/*	Modification History
 *
 * 21-July-87 afd
 *	Added case 6 to "myexit()", for line to long (over 132 chars)
 *
 */

#include <stdio.h>
#include "ptoc.h"

extern enum token
    nexttoken;			/* next Pascal token obtained as input */
extern int linecounter;		/* for myexit() */
extern int lexlev;
extern struct scaninfo scandata;
extern int savecmt;			/* True when comment saved */
extern char currfile[LINELENGTH];	/* file name for myexit() */
extern int withindex;
extern char withflag[WITHNEST];	/* WITHPTR: use "id->";  WITHREC: use "id." */
extern struct stentry *withrecst[WITHNEST];
extern char withvar[WITHNEST][LINELENGTH];

struct stentry *findany();
struct stentry *findlev();
struct stentry *getstentry();
char *getname();
char *malloc();
char *strchr();
struct treenode *gettn();

/*
 * getexpr.  Scan an expression, upto: a token that is not recognized
 * as part of an expression, or upto a ")" if not in a parenthesized sub expr.
 *
 * It calls itself recursively to pick up function parameters.  This is
 * so that the simple mechanism employed to handle `var' parameters will
 * work on nested function calls.
 */

char mathop;			/* set true in getexpr if a mathop is encountered
				 * This is used by writestmt() */

getexpr(buf,cond)
    char *buf;
    int cond;			/* special conditions:
				 *	1: stop after `]' (for "with" array id)
				 *	2: in proc/func call (get var params)
				 *	3: getting LHS of assignment stmt.
				 *	   if funcvar occurs use the dummy
				 *	   funcvar (to assign to).
				 *	4: getting write stmt, stop on comma.
				 */
{
    char inarray = 0;		/* @ `[' increm;  @ `]' decrem */
    char inparen = 0;		/* @ `(' increm;  @ `)' decrem */
    char gotptr = 0;
    int i, j, wi;		/* loop indices */
    int numenums;		/* number of entries an enum type has */
    int nparams;		/* number of params a func/proc has */
    int paramnum;		/* number of params obtained thus far */
    struct stentry *st;
    struct stentry *stparam;	/* pointer to current formal parameter */
    char subbuf[EXPRLEN];	/* for recursize call to getexpr() */
    int saveindex;		/* save index into buf of last "ident" */
    char inset = 0;		/* set when processing "in" */
    char setvar[LINELENGTH];	/* to hold set variable */
    char setelement[LINELENGTH];/* to hold set element */
    int setindex;		/* index in "buf" of start of possible setvar
				       Reset on: and,or,!,(  */
    int varindex;		/* index in "buf" of 1st char of a variable
					Reset on: +,-,*,/,rel-ops,and,or,
						  [,(,!,in  and  , */
    int savemathop;		/* save mathop on recursive call for funct params */
    char nowith;	/* set if "with" var not needed ("." already used) */

    mathop = 0;
    setindex = 0;
    varindex = strlen(buf);
    if (cond == 2)
	{
	/* Getting proc/func params, set up for it here.
	 * Retrieve proc/func name from "buf": get st pointer to it, get
	 * number of params it has, get pointer to first param.
	 * (if we find a dummy function name variable used for returning
	 * funtion value, then we search at lexlev-1.)
	 */
	st = findany(buf);		/* proc name stored in "buf" */
	if (st != NULL)
	    if (st->st_class == VARC)
		st = findlev(buf,lexlev-1);
	if (st != NULL)
	    {
	    buf[0] = '\0';
	    nparams = st->st_nparams;
	    if (nparams > 0)
		{
		paramnum = 0;
		stparam = st->st_fparam;
		}
	    }
	else
	    myexit(1,buf);
	}

    for (; strlen(buf) < EXPRLEN - 10; )
	{
	if (gotptr)
	    if (nexttoken != DOT)
		if (inset)
		    {
		    strcpy(subbuf,setelement);
		    strcpy(setelement,"*");
		    strcat(setelement,subbuf);
		    gotptr = 0;
		    }
		else
		    {
		    strcpy(subbuf, buf + saveindex);
		    buf[saveindex] = '\0';
		    strcat(buf, "*");
		    strcat(buf, subbuf);
		    gotptr = 0;
		    }
	switch(nexttoken)
	    {
	    case ANDT:
		strcat(buf," && ");
    		setindex = strlen(buf);
		varindex = strlen(buf);
		break;
	    case CHARCONST:
		if (inset)
		    {
		    strcpy(setelement,"'");
		    strcat(setelement,scandata.si_name);
		    strcat(setelement,"'");
		    }
		else
		    {
		    if (scandata.si_idlen > 1)
			strcat(buf,"\"");
		    else
			strcat(buf,"'");
		    strcat(buf, scandata.si_name);
		    if (scandata.si_idlen > 1)
			strcat(buf,"\"");
		    else
			strcat(buf,"'");
		    }
		break;
	    case COLON:
		/*
		 * Legal in format of a write stmt
		 */
		strcat(buf,":");
		break;
	    case COMMA:
		if (inarray)
		    {
		    strcat(buf,"][");
		    varindex = strlen(buf);
		    }
		else
		    if (cond == 4)
			return;
		else
		    if (inset)
			{
			strcat(buf,setvar);
			if (inset == 2)
			    {
			    strcat(buf, " <= ");
			    inset = 1;
			    }
			else
			    strcat(buf, " == ");
			strcat(buf, setelement);
			strcat(buf, ") || (");
			}
		else
		    {
		    strcat(buf,",");
		    varindex = strlen(buf);
		    if (cond == 2 && paramnum < nparams)
			{
			paramnum++;
			stparam = stparam->st_link;
			}
		    }
		break;
	    case DOT:
		if (gotptr)
		    {
		    if (inset)
			strcat(setelement, "->");
		    else
			strcat(buf, "->");
		    gotptr = 0;
		    }
		else
		    if (inset)
			strcat(setelement, ".");
		    else
			strcat(buf,".");
		break;
	    case DIVT:
	    case INTDIVT:
		if (!inarray && cond != 2)
		    mathop = 1;
		strcat(buf," / ");
		varindex = strlen(buf);
		break;
	    case DOTDOT:
		strcat(buf, setvar);
		strcat(buf, " >= ");
		strcat(buf, setelement);
		strcat(buf, " && ");
		inset = 2;
		break;
	    case FALSET:
		strcat(buf,"0");
		break;
	    case IDENT:
		if (inset)
		    {
		    st = findany(scandata.si_name);
		    if (st != NULL && st->st_dstruct != SETS)
			/* var in [constid..constid] (or [expr..expr]) */
			strcpy(setelement,scandata.si_name);
		    else
			{  /* var in setid */
			inset = 0;
			strcat(buf, setvar);
			strcat(buf, " == ");
			strcat(buf,scandata.si_name);
			strcat(buf, ")");
			}
		    break;
		    }
		/*
		 * If a "." occured just before this ident, then we don't
		 * want to look for a "with" varible to "dot" in front of
		 * the ident.
		 */
		nowith = (char)strchr(buf + varindex,'.');
		for (wi = withindex; nowith == 0 && wi >= 0; wi--)
		    { /* get ptr to 1st field in record & see if id in record */
		    st = withrecst[wi]->st_next;
		    for (i = 0; i < withrecst[wi]->st_numdims; i++)
			{	/* see if ident matches a record field */
			if (!strcmp(scandata.si_name, st->st_name))
			    break;
			/*
			 * Skip over enumeration constants.
			 */
			if (st->st_dstruct == UDEFS && st->st_tipe == ENUMTY)
			    {
			    numenums = st->st_numdims;
			    for (j = 0; j < numenums; j++)
				st = st->st_link;
			    }
			st = st->st_link;
			}
		    if (nowith == 0 && i < withrecst[wi]->st_numdims)
			{	/* ident is a record field of the 'with' var */
			strcat(buf,withvar[wi]);
			strcat(buf,scandata.si_name);
			break;  /* from for loop */
			}
		    }
		if (nowith != 0 || wi < 0)	/* id not in 'with' record */
		    {
		    st = findany(scandata.si_name);
		    if (st == NULL)
			    {		/* had "record." got "field" */
			    strcat(buf,scandata.si_name);
			    break;
			    }
		    if (cond == 3 && st->st_funcvar)
			{	/* assignment to function use dummy name */
			strcpy(subbuf, scandata.si_name);
			strcpy(scandata.si_name, "v___");
			strcat(scandata.si_name, subbuf);
			}
		    /*
		     * if recursive function call find st entry for the func
		     */
		    if (cond != 3 && st->st_funcvar)
			st = findlev(scandata.si_name, lexlev-1);
		    /*
		     * if calling a function that was passed as a parameter
		     * find st entry for the func.
		     */
		    else if (cond != 3 && st->st_funcpar)
			    st = findlev(scandata.si_name, lexlev-1);
		    if (st != NULL)
			{
			/*
			 * If we encounter a function name in an expression
			 * we do not want to get its parameters if its
			 * a function name parameter.
			 */
			if (st->st_class == FUNCC && stparam->st_funcpar == 0)
			    {
			    nparams = st->st_nparams;
			    if (nparams > 0)
				{
				scanner(0);
				if (nexttoken == LEFTPAREN)
				    {
				    /* Save func name and call getexpr()
				     * recursively with it.  The recursive
				     * call is used so that the simple
				     * mechanism employed for handling `var'
				     * params will work on nested function
				     * calls.
				     */
				    strcpy(subbuf,scandata.si_name);
				    if (st->st_funcpar)
					strcat(buf, "(*");
				    strcat(buf,scandata.si_name);
				    if (st->st_funcpar)
					strcat(buf, ")");
				    strcat(buf, "(");
				    scanner(0);		/* skip over '(' */
				    savemathop = mathop;
				    getexpr(subbuf,2);
				    mathop = savemathop;
				    strcat(buf,subbuf);
				    strcat(buf,")");
				    }
				else
				    myexit(2,"(");
				break;
				}
			    else
				{ /* func w/ no params, need "funcname()" */
				if (st->st_funcpar)
				    strcat(buf, "(*");
				strcat(buf,scandata.si_name);
				if (st->st_funcpar)
				    strcat(buf, ")");
				strcat(buf, "()");
				break;
				}
			    }
			else if (cond == 2 && paramnum < nparams)
				{
				if (stparam->st_byref && !(inarray) &&
				    buf[strlen(buf)-1] != '.' &&
				    buf[strlen(buf)-1] != '>')
				    strcat(buf,"&");
				}
			else if (st->st_class == VARC && st->st_byref &&
				 st->st_dstruct != ARRS)
				/* accessing a parameter that was passed byref */
				strcat(buf, "*");
			saveindex = strlen(buf);
			}
		    strcat(buf,scandata.si_name);
		    }
		break;
	    case IN:
		/* Set "inset" flag & put "(" into "buf" in place of set id */
		inset = 1;
		strcpy(setvar, buf + setindex);
		buf[setindex] = '\0';
		strcat(buf,"(");
		varindex = strlen(buf);
		break;
	    case LEFTPAREN:
		/*
		 * Special case of call to an undefinded proc, when
		 * we were within a "with" stmt so we couldn't tell
		 * until now if it was an assignment stmt or bad proc call
		 */
		if (cond == 3 && inarray == 0)
		    myexit(1,scandata.si_name);
		inparen++;
		mathop = 1;
		strcat(buf,"(");
		if (!inarray)
		    setindex = strlen(buf);
		varindex = strlen(buf);
		break;
	    case LEFTBRACKET:
		if (!inset)
		    {
		    strcat(buf,"[");
		    inarray++;
		    }
		varindex = strlen(buf);
		break;
	    case MINUS:
		if (!inarray && cond != 2)
		    mathop = 1;
		strcat(buf," - ");
		varindex = strlen(buf);
		break;
	    case MODT:
		if (!inarray && cond != 2)
		    mathop = 1;
		strcat(buf," % ");
		varindex = strlen(buf);
		break;
	    case MULT:
		if (!inarray && cond != 2)
		    mathop = 1;
		strcat(buf," * ");
		varindex = strlen(buf);
		break;
	    case NOTT:
		strcat(buf," ! ");
    		setindex = strlen(buf);
		varindex = strlen(buf);
		mathop = 1;
		break;
	    case NUMCONST:
		if (inset)
		    strcpy(setelement,scandata.si_name);
		else
		    strcat(buf,scandata.si_name);
		break;
	    case ORT:
		strcat(buf," || ");
    		setindex = strlen(buf);
		varindex = strlen(buf);
		break;
	    case PERCENT:
		scanner(0);
		if (nexttoken != IDENT)
		    myexit(3,"bad constant");
		if ((strcmp(scandata.si_name,"o") == 0) ||
		    (strcmp(scandata.si_name,"O") == 0))
		    strcat(buf,"0");
		else
		    if ((strcmp(scandata.si_name,"x") == 0) ||
			(strcmp(scandata.si_name,"X") == 0))
			strcat(buf,"0x");
		    else
			myexit(3,"bad constant");
		/*
		 * Call scanner to get octal or hex const within apostrophes.
		 */
		scanner(0);
		if (nexttoken != CHARCONST)
		    myexit(3,"bad constant");
		strcat(buf,scandata.si_name);
		break;
	    case PLUS:
		if (!inarray && cond != 2)
		    mathop = 1;
		strcat(buf," + ");
		varindex = strlen(buf);
		break;
	    case RELATIONAL:
		switch(scandata.si_rel)
		    {
		    case eqrel:
			strcat(buf," == ");
			break;
		    case nerel:
			strcat(buf," != ");
			break;
		    case gtrel:
			strcat(buf," > ");
			break;
		    case gerel:
			strcat(buf," >= ");
			break;
		    case ltrel:
			strcat(buf," < ");
			break;
		    case lerel:
			strcat(buf," <= ");
			break;
		    }
		varindex = strlen(buf);
		mathop = 1;
		break;
	    case RIGHTPAREN:
		if (inparen)
		    {
		    strcat(buf,")");
		    inparen--;
		    if (cond == 2 && inparen == 0)
			cond = 0;
		    }
		else
		    return;	/* proc call & ?? */
		break;
	    case RIGHTBRACKET:
		if (inset)
		    {
		    strcat(buf,setvar);
		    if (inset == 2)
			strcat(buf, " <= ");
		    else
			strcat(buf, " == ");
		    strcat(buf, setelement);
		    strcat(buf, ")");
		    inset = 0;
		    }
		else
		    {
		    strcat(buf,"]");
		    inarray--;
		    varindex = strlen(buf);
		    /* if end of "with" array id, return */
		    if (cond == 1 && inarray == 0)
			return;
		    }
		break;
	    case TRUET:
		strcat(buf,"1");
		break;
	    case UPARROW:
		gotptr = 1;
		break;
	    default:
		return;
		break;
	    }   /* end switch nexttoken */
	if (inset)
	    scanner(1);		/* no "real" numbers allowed */
	else
	    scanner(0);
	}   /* end for */
    /*
     * skip over rest of expr > EXPRLEN
     */
    for (; nexttoken != ASSIGNOP && nexttoken != SEMICOLON;)
	scanner(0);
}


struct treenode *
writestmt(parent, prev, ln)
    struct treenode *parent;
    struct treenode *prev;
    char ln;			/* true if its writeLN */
{
    struct stentry *st;
    struct treenode *tn;
    char filevar[WORDLENGTH];
    char lit[EXPRLEN];
    char buf[EXPRLEN];
    int bi, ei;			/* beginning & ending buf index */
    char getfields;		/* set when getting record fields */
    char gotptr;		/* set when "->" found */
    char inrec;			/* set when record struct found */
    int ti;			/* temp buf index */
    int wi;			/* "with" var index */

    savecmt = 0;
    tn = gettn();
    tn->type = WRITENODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    filevar[0] = '\0';
    lit[0] = '\0';
    buf[0] = '\0';
    scanner();
    if (nexttoken == LEFTPAREN)
	{
	scanner();
	for (;;)
	    {
	    switch (nexttoken)
		{
		case CHARCONST:
		    strcat(lit, scandata.si_name);
		    scanner();		/* get nexttoken after char const */
		    break;
		default:
		    if (strcmp(buf,""))
			strcat(buf,", ");
		    bi = strlen(buf);
		    getexpr(buf,4);
		    if (mathop)
			{
			strcat(lit,"%d");
			/*
			 * Format spec could be after expr.
			 * If so, Remove it from buf
			 */
			ti = (int)strchr(buf + bi,':') - (int)buf;
			if (ti > bi)
			    buf[ti] = '\0';
			}
		    else
			{
			/*
			 * Special code to handle Pascal records (C structs).
			 * We need to find the proper field to get the
			 * correct type to format the output.
			 */
			inrec = 0;
			getfields = 0;
			do
			    {
			    gotptr = 0;
			    ei = (int)strchr(buf + bi,'-') - (int)buf;
			    ti = (int)strchr(buf + bi,'>') - (int)buf;
			    if (ei > bi && ei == ti-1)	/* found a '->' */
				{
				getfields = 1;
				ti = (int)strchr(buf + bi,'[') - (int)buf;
				if (ti > bi)
				    {	/* array of ptrs: "a[n]^." */
				    strncpy(scandata.si_name, buf + bi, ti-bi);
				    scandata.si_name[ti-bi] = '\0';
				    }
				else
				    {
				    strncpy(scandata.si_name, buf + bi, ei-bi);
				    scandata.si_name[ei-bi] = '\0';
				    }
				gotptr = 1;
				}
			    else
				{
				ei = (int)strchr(buf + bi,'.') - (int)buf;
				if (ei > bi)	/* found a '.' */
				    {
				    getfields = 1;
				    ti = (int)strchr(buf + bi,'[') - (int)buf;
				    if (ti > bi)
					{	/* array of recs: "a[n]." */
					strncpy(scandata.si_name, buf + bi, ti-bi);
					scandata.si_name[ti-bi] = '\0';
					}
				    else
					{
					strncpy(scandata.si_name, buf + bi, ei-bi);
					scandata.si_name[ei-bi] = '\0';
					}
				    }
				else
				    {
				    getfields = 0;
				    ti = (int)strchr(buf + bi,'[') - (int)buf;
				    if (ti > bi)
					{
					strncpy(scandata.si_name, buf + bi, ti-bi);
					scandata.si_name[ti-bi] = '\0';
					/*
					 * Format spec could be after array.
					 * If so, Remove it from buf
					 */
					ti = (int)strchr(buf + bi,':') - (int)buf;
					if (ti > bi)
					    buf[ti] = '\0';
					}
				    else
					{
					ti = (int)strchr(buf + bi,'(') - (int)buf;
					if (ti > bi)
					    { /* function name */
					    strncpy(scandata.si_name, buf + bi, ti-bi);
					    scandata.si_name[ti-bi] = '\0';
					    /*
					     * Format spec could be after array.
					     * If so, Remove it from buf
					     */
					    ti = (int)strchr(buf + bi,':') - (int)buf;
					    if (ti > bi)
						buf[ti] = '\0';
					    }
					else
					    {
					    ti = (int)strchr(buf + bi,':') - (int)buf;
					    if (ti > bi)
						{ /* format spec on an otherwise
						     plain var, Rem from buf */
						strncpy(scandata.si_name, buf + bi, ti-bi);
						scandata.si_name[ti-bi] = '\0';
						buf[ti] = '\0';
						}
					    else /* plain variable */
						strcpy(scandata.si_name, buf + bi);
					    ti = (int)strchr(buf + bi,'*') - (int)buf;
					    if (ti >= bi)
						{ /* ptr var pas:p^  C:*p */
						strcpy(scandata.si_name, buf + bi +1);
						}
					    }   /* : */
					}   /* ( */
				    }   /* . */
				}   /* - */
			    if (!inrec)
				{
				st = findany(scandata.si_name);
				if (st == NULL)
				    myexit(1,scandata.si_name);
				}
			    else
				{
				for (st = st->st_next; st != NULL; st = st->st_link)
				    if (!strcmp(scandata.si_name, st->st_name))
					break;
				if (st == NULL)
				    myexit(4,scandata.si_name);
				}
			    /*
			     * Chase user pointers to the real type
			     */
			    while (st->st_dstruct == UDEFS)
				st = st->st_uptr;
			    while ((st->st_dstruct == PTRS || 
				    st->st_dstruct == ARRS) &&
				    st->st_tipe == USERTYPE)
				st = st->st_uptr;
			    if (st->st_dstruct == RECORDS)
				inrec = 1;
			    if (ei > bi)
				if (gotptr)
				    bi = ei + 2;
				else
				    bi = ei + 1;
			    }
			while (getfields);

			switch (st->st_dstruct)
			    {
			    case FILESTR:
				strcpy(filevar, scandata.si_name);
				buf[0] = '\0';
				break;
			    case ARRS:
				while (st->st_tipe == USERTYPE)
				    st = st->st_uptr;
				switch (st->st_tipe)
				    {
				    case CHARTY:
					/* NOTE: string type */
					strcat(lit, "%s");
					break;
				    case REALTY:
					strcat(lit, "%f");
					break;
				    default:
					strcat(lit, "%d");
					break;
				    }   /* end switch st_tipe */
				break;
			    default:
				switch (st->st_tipe)
				    {
				    case CHARTY:
					strcat(lit, "%c");
					break;
				    case STRINGTY:
					strcat(lit, "%s");
					break;
				    case REALTY:
					strcat(lit, "%f");
					break;
				    case OCTALTY:
					strcat(lit, "0%o");
					break;
				    case HEXTY:
					strcat(lit, "0x%x");
					break;
				    default:
					strcat(lit, "%d");
					break;
				    }   /* end switch st_tipe */
			    }  /* end switch st_dstruct */
			}   /* end else */
		}  /* end switch nexttoken */
	    if (nexttoken == RIGHTPAREN)
		break;
	    scanner();
	    }  /* end for */
	}   /* end if (nexttoken == LEFTPAREN) */
    if (ln)
	strcat(lit, "\\n");
    strcat(lit, "\"");
    tn->expression = malloc(strlen(buf) + strlen(lit) + strlen(filevar) + 16);
    if (tn->expression == NULL)
	myexit(-1,"");
    tn->expression[0] = '\0';
    if (strcmp(filevar, ""))
	{
	strcat(tn->expression, "fprintf(");	    /* + 8 */
	strcat(tn->expression, filevar);
	strcat(tn->expression, ", ");		    /* + 2 */
	}
    else
	strcat(tn->expression, "printf(");
    strcat(tn->expression, "\"");		    /* + 1 */
    strcat(tn->expression, lit);
    if (strcmp(buf,""))
	strcat(tn->expression, ", ");		    /* + 2 */
    strcat(tn->expression, buf);
    strcat(tn->expression,");");		    /* + 2 */
    scanner();
    if (nexttoken == SEMICOLON)
	{
	savecmt = 1;
	scanner();
	}
    return(tn);
}


mainstmts(parent)
    struct treenode *parent;
{
    struct stentry *st;
    struct treenode *tn, *holdfc;

    /*
     * Insert "main" proc before the first procedure
     */

    holdfc = parent->firstc;
    tn = gettn();
    parent->firstc = tn;
    tn->next = holdfc;

    tn->type = PDECLNODE;
    if (parent->lastc == NULL)
	parent->lastc = tn;
    st = getstentry();
    st->st_class = PROCC;
    st->st_nparams = 0;
    st->st_name = getname(4);
    strcpy(st->st_name, "main");
    tn->stdecl = st;
    addsymbol(st);
    blkentry(tn);
    lexlev++;
    savecmt = 1;
    scanner(0);		/* skip over "begin" */
    (void) statelist(tn->firstc->next->next->next, FUNCLEVEL);
    return;
}



/*
 * Error exit.  Due to a compiler internal error or encountering illegal
 * Pascal syntax.
 */

myexit(n,s)
    int n;		/* error number */
    char *s;		/* possible string to go with error */
{
    if (n == -1)
	fprintf(stderr, "\n\"%s\", line %d: Malloc failed.  Too many Pascal symbols  \n", currfile, linecounter);
    else
	fprintf(stderr, "\"%s\", line %d: Illegal Pascal Syntax: ",
		currfile, linecounter);
    switch(n) {
    case 1:
	fprintf(stderr, "Undefined symbol: '%s'.\n", s);
	break;
    case 2:
	fprintf(stderr, "'%s' expected.\n", s);
	break;
    case 3:
	fprintf(stderr, "%s.\n", s);
	break;
    case 4:
	fprintf(stderr, "Undefined field: '%s'.\n", s);
	break;
    case 5:
	fprintf(stderr, "Can't open file: '%s'.\n", s);
	break;
    case 6:
	fprintf(stderr, "Line too long (over 132 chars).\n", s);
	break;
    default:
	break;
    }
    fprintf(stderr, "Exiting Pascal to C translator.\n");
    sleep(1);			/* Give messages time before exit */
    exit();
}
