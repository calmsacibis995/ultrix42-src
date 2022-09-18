#ifndef lint
static char *sccsid = "@(#)prstmt.c	4.1	ULTRIX	7/17/90";
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
 * File: 	prstmt.c
 *
 * Pascal to C translator - Parser for statements, Contains:
 *			  - statelist() - main routine for parsing stmts
 */

#include <stdio.h>
#include "ptoc.h"

extern enum token
    nexttoken,			/* next Pascal token obtained as input */
    holdtoken;			/* hold present token for look-ahead */
extern char tokenahead;
extern int lexlev;
extern struct scaninfo scandata;
extern int savecmt;			/* True when comment saved */

struct stentry *findany();
struct stentry *findlev();
struct stentry *getstentry();
char *getname();
char *malloc();
char *strchr();
struct treenode *gettn();
struct treenode *writestmt();

/* for "with" statements */
int withindex = -1;
char withflag[WITHNEST];	/* WITHPTR: use "id->";  WITHREC: use "id." */
struct stentry *withrecst[WITHNEST];
char withvar[WITHNEST][LINELENGTH];


/*
 * Begin block
 */

struct treenode *
beginblock(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;

    tn = gettn();
    tn->type = BEGINNODE;
    if (parent->type == WITHNODE)
	tn->blktype = WITHBLOCK;
    else
	tn->blktype = SEMIBLOCK;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    scanner(0);
    statelist(tn, BEGINLEVEL);
    return(tn);
}



/*
 * Assign stmt
 */

struct treenode *
assignstmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = ASSIGNNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    buf[0] = '\0';
    getexpr(buf,3);
    tn->storewhere = malloc(strlen(buf)+1);
    if (tn->storewhere == NULL)
	myexit(-1,"");
    strcpy(tn->storewhere,buf);
    scanner(0);				/* get start of RHS of expr */
    buf[0] = '\0';
    getexpr(buf,0);
    /*
     * If assigning a character string in Pascal, use "strcpy" in C
     */
    if (strchr(buf,'"') == buf)
	{
	tn->type = PCALLNODE;
	tn->stdecl = findany("strcpy");
	tn->expression = malloc(strlen(tn->storewhere) + strlen(buf) + 2);
	if (tn->expression == NULL)
	    myexit(-1,"");
	strcpy(tn->expression, tn->storewhere);
	strcat(tn->expression, ",");
	strcat(tn->expression, buf);
	}
    else
	{
	tn->storewhat = malloc(strlen(buf)+1);
	if (tn->storewhat == NULL)
	    myexit(-1,"");
	strcpy(tn->storewhat,buf);
	}
    if (nexttoken == SEMICOLON)
	{
	savecmt = 1;
	scanner(0);			/* get next token after SEMICOLEN */
	}
    return(tn);
}


/*
 * call stmt.
 */

struct treenode *
callstmt(parent,prev,st)
    struct treenode *parent, *prev;
    struct stentry *st;
{
    struct treenode * tn;
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = PCALLNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    tn->stdecl = st;
    scanner(0);
    if (nexttoken == LEFTPAREN)
	{
	strcpy(buf,scandata.si_name);	/* save proc name for getexpr() */
	scanner(0);			/* skip over '(' */
	getexpr(buf,2);
	tn->expression = malloc(strlen(buf)+1);
	if (tn->expression == NULL)
	    myexit(-1,"");
	strcpy(tn->expression,buf);
	scanner(0);			/* get ';' */
	}
    if (nexttoken == SEMICOLON)
	{
	savecmt = 1;
	scanner(0);				/* get nexttoken */
	}
    return(tn);
}


/*
 * case stmt
 */

struct treenode *
casestmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode *tn, *casetn;
    struct treenode *labelparent, *labelprev;
    char buf[EXPRLEN];
    char other = 0;		/* set true if "otherwise" found */
    char o_or_x[4];

    savecmt = 0;
    tn = gettn();
    casetn = tn;
    tn->type = CASENODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    /* get case expression */
    buf[0] = '\0';
    scanner(0);				/* get 1st char of expression */
    getexpr(buf,0);
    tn->expression = malloc(strlen(buf)+1);
    if (tn->expression == NULL)
	myexit(-1,"");
    strcpy(tn->expression,buf);
    scanner(0);				/* get nexttoken after 'OF' */
    prev = NULL;
    parent = tn;			/* case is parent to listheads */
    /*
     * Loop until 'end' of case stmt is found
     */
    for (; nexttoken != ENDT; )
	{
	tn = gettn();
	tn->type = LISTHEADNODE;
	if (parent->firstc == NULL)
	    parent->firstc = tn;
	parent->lastc = tn;
	if (prev != NULL)
	    prev->next = tn;
	prev = tn;
	/*
	 * get case labels for one case
	 */
	labelparent = tn;
	labelprev = NULL;
	for (; nexttoken != COLON; )
	    {
	    tn = gettn();
	    tn->type = LABELNODE;
	    if (labelparent->firstc == NULL)
		labelparent->firstc = tn;
	    labelparent->lastc = tn;
	    if (labelprev != NULL)
		labelprev->next = tn;
	    /*
	     * Get constant set up in scandata.si_name
	     */
	    switch (nexttoken)
		{
		case CHARCONST:
		    strcpy(buf,scandata.si_name);
		    strcpy(scandata.si_name,"'");
		    strcat(scandata.si_name,buf);
		    strcat(scandata.si_name,"'");
		    break;
		case MINUS:
		    scanner(0);
		    strcpy(buf,scandata.si_name);
		    strcpy(scandata.si_name, "-");
		    strcat(scandata.si_name, buf);
		    break;
		case PLUS:
		    scanner(0);
		    strcpy(buf,scandata.si_name);
		    strcpy(scandata.si_name, "+");
		    strcat(scandata.si_name, buf);
		    break;
		case IDENT:
		case NUMCONST:
		    break;		/* do nothing: already set up */
		case TRUET:
		    strcpy(scandata.si_name, "1");
		    break;
		case FALSET:
		    strcpy(scandata.si_name, "0");
		    break;
		case PERCENT:
		    scanner(0);
		    if (nexttoken != IDENT)
			myexit(2,"constant");
		    if ((strcmp(scandata.si_name,"o") == 0) ||
			(strcmp(scandata.si_name,"O") == 0))
			strcpy(o_or_x,"0");
		    else
			if ((strcmp(scandata.si_name,"x") == 0) ||
			    (strcmp(scandata.si_name,"X") == 0))
			    strcpy(o_or_x,"0x");
			else myexit(2,"constant");
		    /*
		     * Call scanner to get octal or hex const within
		     * apostrophes.
		     */
		    scanner(0);
		    if (nexttoken != CHARCONST)
			myexit(2,"constant");
		    strcpy(buf,scandata.si_name);
		    strcpy(scandata.si_name, o_or_x);
		    strcat(scandata.si_name, buf);
		    break;
		case OTHERWISE:
		    strcpy(scandata.si_name, "default");
		    other = 1;
		    break;
		default:
		    /*
		     * Assume its correct syntax (like +/- constant)
		     */
		    break;
		}   /* end switch on nexttoken */
	    tn->expression = malloc(strlen(scandata.si_name)+1);
	    if (tn->expression == NULL)
		myexit(-1,"");
	    strcpy(tn->expression,scandata.si_name);

	    labelprev = tn;
	    if (other)
		nexttoken = COLON;
	    else
		scanner(0);
	    if (nexttoken == COMMA)
		scanner(0);
	    if (nexttoken == SEMICOLON || nexttoken == ENDT)
		myexit(2,"case label");
	    }
	savecmt = 1;
	scanner(0);		/* get nexttoken after ":" @ end of case label */
	statelist(tn, SINGLESTMT);	/* tn = last label node in this group */
	}
    savecmt = 1;
    scanner(0);			/* get nexttoken after case 'end' */
    if (nexttoken == SEMICOLON)
	scanner(0);
    return(casetn);
}

/*
 * Cmt stmt. Create a tree node for it.
 */

struct treenode *
cmtstmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;

    tn = gettn();
    tn->type = COMMENTNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    tn->blkcmt = scandata.si_cmtptr;
    scanner(0);		/* get next token after comment */
    return(tn);
}


/*
 * for stmt
 */

struct treenode *
forstmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = FORNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;

    scanner(0);				/* get 1st token of for loop var */
    buf[0] = '\0';
    getexpr(buf,0);
    tn->variable = malloc(strlen(buf)+1);
    if (tn->variable == NULL)
	myexit(-1,"");
    strcpy(tn->variable,buf);

    scanner(0);				/* get next token after ':=' */
    buf[0] = '\0';
    getexpr(buf,0);
    tn->initvalue = malloc(strlen(buf)+1);
    if (tn->initvalue == NULL)
	myexit(-1,"");
    strcpy(tn->initvalue,buf);

    if (nexttoken == TOT)
	tn->to = 1;
    else
	tn->to = 0;
    
    scanner(0);				/* get next token after 'to/downto' */
    buf[0] = '\0';
    getexpr(buf,0);
    tn->finalvalue = malloc(strlen(buf)+1);
    if (tn->finalvalue == NULL)
	myexit(-1,"");
    strcpy(tn->finalvalue,buf);

    if (nexttoken != LOOPDO)
       myexit(2,"do");
    savecmt = 1;
    scanner(0);				/* get next token after 'do' */
    statelist(tn, SINGLESTMT);
    return(tn);
}


/*
 * goto stmt
 */

struct treenode *
gotostmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = GOTONODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;

    scanner(0);				/* get 1st token of goto label */
    buf[0] = '\0';
    getexpr(buf,0);
    tn->expression = malloc(strlen(buf)+1);
    if (tn->expression == NULL)
	myexit(-1,"");
    strcpy(tn->expression,buf);

    if (nexttoken == SEMICOLON)
	{
	savecmt = 1;
	scanner(0);
	}
    return(tn);
}


/*
 * label stmt
 */

struct treenode *
labelstmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = LABELNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;

    tn->expression = malloc(strlen(scandata.si_name)+1);
    if (tn->expression == NULL)
	myexit(-1,"");
    strcpy(tn->expression,scandata.si_name);

    scanner(0);
    if (nexttoken != COLON)
	myexit(2,":");
    savecmt = 1;
    scanner(0);
    return(tn);
}



/*
 * if stmt.
 */

struct treenode *
ifstmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode *tn;		/* ptr to 'if' tree node */
    struct treenode *elsetn;		/* ptr to 'else block' tree node */
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = IFNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;

    scanner(0);				/* get 1st token of if expression */
    buf[0] = '\0';
    getexpr(buf,0);
    tn->expression = malloc(strlen(buf)+1);
    if (tn->expression == NULL)
	myexit(-1,"");
    strcpy(tn->expression,buf);

    if (nexttoken != THENT)
	myexit(2,"then");
    
    savecmt = 1;
    scanner(0);				/* get next token after 'then' */
    statelist(tn, SINGLESTMT);		/* 'then' part goes under firstc */
    if (nexttoken == ELSET)
	{
	/*
	 * Create an 'else-block' for the else clause.
	 * The 'if' tree node's lastchild points to the else-block.
	 */
	elsetn = gettn();
	elsetn->type = BEGINNODE;
	elsetn->blktype = ELSEBLOCK;
	tn->lastc = elsetn;
	savecmt = 1;
	scanner(0);			/* next token after 'else' */
	statelist(elsetn,SINGLESTMT);	/* 'else' part goes under elseblock */
	}
    else
	tn->lastc = NULL;		/* no else clause */
    return(tn);
}


/*
 * Read/Readln stmts
 */

struct treenode *
readstmt(parent, prev, ln)
    struct treenode *parent;
    struct treenode *prev;
    char ln;			/* true if its readLN */
{
    struct stentry *st;
    struct treenode *tn;
    char filevar[WORDLENGTH];
    char inputvar[LINELENGTH];
    char lit[EXPRLEN];
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = READNODE;
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
	    if (strcmp(buf,""))
		strcat(buf,", ");
    	    inputvar[0] = '\0';
	    getexpr(inputvar,4);
	    st = findany(scandata.si_name);
	    while (st->st_dstruct == UDEFS)
		st = st->st_uptr;
	    switch (st->st_dstruct)
		{
		case FILESTR:
		    strcpy(filevar, scandata.si_name);
		    break;
		case ARRS:
		    strcat(lit, "%s");
		    strcat(buf,inputvar);
		    break;
		default:
		    strcat(buf,"&");
		    strcat(buf,inputvar);
		    switch (st->st_tipe)
			{
			case CHARTY:
			    strcat(lit, "%c");
			    break;
			case REALTY:
			    strcat(lit, "%f");
			    break;
			default:
			    strcat(lit, "%d");
			    break;
			}   /* end switch st_tipe */
		}  /* end switch st_dstruct */
	    if (nexttoken == RIGHTPAREN)
		break;
	    scanner();
	    }  /* end for */
	}   /* end if (nexttoken == LEFTPAREN) */
    strcat(lit, "\"");
    if (ln)
	tn->expression = malloc(strlen(buf) + strlen(lit) + strlen(filevar) + 35);
    else
	tn->expression = malloc(strlen(buf) + strlen(lit) + strlen(filevar) + 15);
    if (tn->expression == NULL)
	myexit(-1,"");
    tn->expression[0] = '\0';
    if (strcmp(filevar, ""))
	{
	strcat(tn->expression, "fscanf(");			/* + 7 */
	strcat(tn->expression, filevar);
	strcat(tn->expression, ", ");		    		/* + 2 */
	}
    else
	strcat(tn->expression, "scanf(");
    strcat(tn->expression, "\"");				/* + 1 */
    strcat(tn->expression, lit);
    if (strcmp(buf,""))
	strcat(tn->expression, ", ");		    		/* + 2 */
    strcat(tn->expression, buf);
    strcat(tn->expression,");");		    		/* + 2 */
    if (ln)
	strcat(tn->expression," gets(readln_dummy);");		/* + 20 */
    scanner();
    if (nexttoken == SEMICOLON)
	{
	savecmt = 1;
	scanner();
	}
    return(tn);
}


/*
 * repeat stmt
 */

struct treenode *
repeatstmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;
    char buf[EXPRLEN];

    tn = gettn();
    tn->type = REPEATNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    savecmt = 1;
    scanner(0);
    statelist(tn, FUNCLEVEL);	/* get multiple stmts w/out begin..end */
    buf[0] = '\0';
    savecmt = 0;
    scanner(0);				/* get 1st token of expr */
    getexpr(buf,0);
    tn->expression = malloc(strlen(buf)+4);
    if (tn->expression == NULL)
	myexit(-1,"");
    /*
     * repeat-until (cond)   <translates to>   do-while (not cond)
     */
    strcpy(tn->expression,"!(");
    strcat(tn->expression,buf);
    strcat(tn->expression,")");
    if (nexttoken == SEMICOLON)
	{
	savecmt = 1;
	scanner(0);				/* get nexttoken */
	}
    return(tn);
}


/*
 * semistmt.
 */

struct treenode *
semistmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;

    tn = gettn();
    tn->type = SEMINODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;

    savecmt = 1;
    scanner(0);				/* get next token */
    return(tn);
}


/*
 * While stmt.
 */

struct treenode *
whilestmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode * tn;
    char buf[EXPRLEN];

    savecmt = 0;
    tn = gettn();
    tn->type = WHILENODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;

    scanner(0);				/* get 1st token of while expression */
    buf[0] = '\0';
    getexpr(buf,0);
    tn->expression = malloc(strlen(buf)+1);
    if (tn->expression == NULL)
	myexit(-1,"");
    strcpy(tn->expression,buf);

    if (nexttoken != LOOPDO)
	myexit(2,"do");
    savecmt = 1;
    scanner(0);				/* get next token after 'do' */
    statelist(tn, SINGLESTMT);
    return(tn);
}


/*
 * With stmt
 */

struct treenode *
withstmt(parent,prev)
    struct treenode *parent, *prev;
{
    struct treenode *tn;
    struct stentry *st, *wist;
    int i, j, wi;			/* loop indices */
    int numenums;			/* number of entries an enum type has */
    char buf[EXPRLEN];
    char first;

    savecmt = 0;
    tn = gettn();
    tn->type = WITHNODE;
    if (parent->firstc == NULL)
	parent->firstc = tn;
    parent->lastc = tn;
    if (prev != NULL)
	prev->next = tn;
    
  do {
    scanner(0);		/* get 'with' var */
    if (nexttoken != IDENT)
	myexit(2,"identifier");
    st = findany(scandata.si_name);
    if (st == NULL && withindex < 0)
	myexit(1,scandata.si_name);

    for (wi = withindex; wi >= 0; wi--)
	{ /* get ptr to 1st field in record & see if id in record */
	wist = withrecst[wi]->st_next;
	for (i = 0; i < withrecst[wi]->st_numdims; i++)
	    {	/* see if ident matches a record field */
	    if (!strcmp(scandata.si_name, wist->st_name))
		break;
	    /*
	     * Skip over enumeration constants.
	     */
	    if (wist->st_dstruct == UDEFS && wist->st_tipe == ENUMTY)
		{
		numenums = wist->st_numdims;
		for (j = 0; j < numenums; j++)
		    wist = wist->st_link;
		}
	    wist = wist->st_link;
	    }
	if (i < withrecst[wi]->st_numdims)
	    {	/* ident is a record field of the 'with' var */
	    strcpy(withvar[withindex+1], withvar[wi]);
	    strcat(withvar[withindex+1],scandata.si_name);
	    st = wist;
	    break;
	    }
	}
    if (wi < 0)	/* id not in 'with' record */
	strcpy(withvar[withindex+1], scandata.si_name);

    /*
     * Loop to get all of "with" variable.
     */
    first = 1;
    for (; nexttoken == IDENT ;)
	{
	for (; st->st_dstruct == UDEFS ||
	    (st->st_dstruct == ARRS && st->st_tipe == USERTYPE) ; )
	    st = st->st_uptr;
	/*
	 * If id is an array id, call getexpr() to get array subscripts.
	 */
	scanner(0);
	if (nexttoken == LEFTBRACKET)
	    {
	    buf[0] = '\0';
	    getexpr(buf,1);
	    if (nexttoken != RIGHTBRACKET)
		myexit(2,"]");
	    strcat(withvar[withindex+1], buf);
	    scanner(0);		/* nexttoken after ']' */
	    }
	if (nexttoken == UPARROW)
	    if (st->st_dstruct != PTRS)
		myexit(3,"bad 'with' variable");
	    else
		{
withflag[withindex+1] = WITHPTR;
		scanner(0);
		if (nexttoken == DOT)
		    {
		    strcat(withvar[withindex+1], "->");
		    /*
		     * Can't trust st->uptr directly, since object pointed to
		     * could have been defined later.
		     */
		    st = findany(st->st_uptr->st_name);
		    if (st == NULL)
			myexit(3,"bad 'with' variable");
		    withrecst[withindex+1] = st;
		    scanner(0);
		    if (nexttoken != IDENT)
			myexit(3,"bad 'with' variable");
	    	    first = 0;
		    }
		}
	else if (nexttoken == DOT)
	    if (st->st_dstruct != RECORDS)
		myexit(3,"bad 'with' variable");
	    else
		{
withflag[withindex+1] = WITHREC;
		strcat(withvar[withindex+1], ".");
		withrecst[withindex+1] = st;
		scanner(0);
		if (nexttoken != IDENT)
		    myexit(3,"bad 'with' variable");
		first = 0;
		}
	else if (nexttoken != LOOPDO && nexttoken != COMMA)
		myexit(3,"bad 'with' variable");
	if (nexttoken == IDENT && (!first))
	    {
	    /*
	     * Get ptr to first field in record & find field name
	     */
	    wist = st->st_next;
	    for (i = 0; i < wist->st_numdims; i++)
		{	/* see if ident matches a record field */
		if (!strcmp(scandata.si_name, wist->st_name))
		    break;
		/*
		 * Skip over enumeration constants.
		 */
		if (wist->st_dstruct == UDEFS && wist->st_tipe == ENUMTY)
		    {
		    numenums = wist->st_numdims;
		    for (j = 0; j < numenums; j++)
			wist = wist->st_link;
		    }
		wist = wist->st_link;
		}
	    if (i < st->st_numdims)
		{	/* ident is a record field of the 'with' var */
		strcat(withvar[withindex+1],scandata.si_name);
		st = wist;
		}
	    else
		myexit(3,"bad 'with' variable");
	    }	/* if (nexttoken == IDENT && !first) */
	}	/* for (nexttoken == IDENT) */
    /*
     * Now update "withrecst" to point to last record structure
     * referenced by the last ident part of the "with" variable.
     */
    if (st->st_dstruct == PTRS)
	{
	/*
	 * Can't trust st->uptr directly, since object pointed to
	 * could have been defined later.
	 */
	st = findany(st->st_uptr->st_name);
	if (st == NULL)
	    myexit(3,"bad 'with' variable");
	withrecst[withindex+1] = st;
	strcat(withvar[withindex+1], "->");
	}
    else if (st->st_dstruct == RECORDS)
	{
	withrecst[withindex+1] = st;
	strcat(withvar[withindex+1], ".");
	}
    /*
     * Now done getting the "with" variable so
     * its safe to increment "withindex".
     */
    withindex++;
  } while (nexttoken == COMMA);
    savecmt = 1;
    scanner(0);		/* get nexttoken after 'do' */
    statelist(tn,SINGLESTMT);
    withflag[withindex] = 0;
    withindex--;
    return(tn);
}


/*
 * Statement list.
 * BNF:
 *	<statelist> ::= (<statement> | begin <statelist> end) ^+
 *	<statement> ::= (<label>: | SEMICOLON | <assignstmt> | <callstmt> |
 *			<casestmt> | <forstmt> | <ifstmt> | <whilestmt> |
 *			<repeatstmt> | <withstmt> | <gotostmt>)
 */

statelist(parent, state)
    struct treenode *parent;
    char state;		/* 0 = SINGLESTMT, 1 = FUNCLEVEL, 2 = BEGINLEVEL */
{
    struct stentry *st;
    struct treenode *tn, *prev;
    char stmt;			/* set to 0 when we don't recognize a stmt */
    char cont;			/* set to 1 when we get a comment or label */

    prev = NULL;
    stmt = 1;
    do {
	cont = 0;
	switch (nexttoken)
	    {
	    case BEGINT:
		prev = beginblock(parent, prev);
		break;
	    case ENDT:
		stmt = 0;
		/*
		 * This is neccessary so that the 'end' of a case stmt
		 * is not scanned here.
		 */
		if (state != SINGLESTMT)
		    {
		    scanner(0);
		    if (nexttoken == SEMICOLON)
			scanner(0);
		    }
		break;
	    case IDENT:
		st = findany(scandata.si_name);
		if (st != NULL && (st->st_funcpar == 2))
		    st = findlev(scandata.si_name,lexlev-1);
		if (st == NULL)
		    if (withindex < 0)
			myexit(1,scandata.si_name);
		if (st != NULL && (st->st_class == PROCC || st->st_class == FUNCC))
		    prev = callstmt(parent,prev,st);
		else
		    prev = assignstmt(parent,prev);
		break;
	    case CASET:
		prev = casestmt(parent, prev);
		break;
	    case COMMENT:
		prev = cmtstmt(parent, prev);
		/* can always have another stmt after a comment so continue */
		cont = 1;
		break;
	    case FORT:
		prev = forstmt(parent, prev);
		break;
	    case GOTOT:
		prev = gotostmt(parent, prev);
		break;
	    case IFT:
		prev = ifstmt(parent, prev);
		break;
	    case NUMCONST:
		prev = labelstmt(parent, prev);
		/* can always have another stmt after a label so continue */
		cont = 1;
		break;
	    case READT:
		prev = readstmt(parent, prev, 0);
		break;
	    case READLNT:
		prev = readstmt(parent, prev, 1);
		break;
	    case REPEATT:
		prev = repeatstmt(parent, prev);
		break;
	    case UNTILT:
		stmt = 0;	/* repeatstmt() will get the expression */
		break;
	    case WHILET:
		prev = whilestmt(parent, prev);
		break;
	    case WITHT:
		prev = withstmt(parent, prev);
		break;
	    case WRITET:
		prev = writestmt(parent, prev, 0);
		break;
	    case WRITELNT:
		prev = writestmt(parent, prev, 1);
		break;
	    case SEMICOLON:
		prev = semistmt(parent, prev);
		break;
	    default:
		stmt = 0;
		break;
	    }   /* end switch on nexttoken */
    } while (cont || (stmt && state));

    /*
     * Get any trailing comments and any between procs
     */

    while (nexttoken == COMMENT)
	prev = cmtstmt(parent, prev);
}
