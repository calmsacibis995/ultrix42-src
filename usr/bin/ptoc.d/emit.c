#ifndef lint
static	char	*sccsid = "@(#)emit.c	4.1	(ULTRIX)	7/17/90";
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
 * File: 	emit.c
 *
 * Pascal to C translator - Emit C code
 */

/*	Modification History: emit.c
 *
 * 21-July-87 afd
 *	Added "case STRINGTY" to output of const values.
 *
 */

#include <stdio.h>
#include "ptoc.h"

#ifdef EMITDEBUG
int emitdebug = 1;
#define printd if (emitdebug) fprintf
#define printd10 if (emitdebug >= 10) fprintf
#endif

extern struct treenode *procindex[MAXLEV];
extern int lexlev;


/*
 * Print spaces on the current line.
 */
indent(lexlev)
    int lexlev;
{
    int i;
    for (i=0; i < lexlev; i++)
	printf("    ");
}


/* 
 * Emit C code.
 */

emitcode(tn)
    struct treenode *tn;
{
    register struct stentry *st;
    register struct cmtinfo *cptr;

    /* switch on treenode type */
    if (tn == NULL)
	return;
    switch (tn->type)
	{
	case PROGNODE:
	    emitcode(tn->firstc);
	    break;
	case ASSIGNNODE:
	    indent(lexlev);
	    printf("%s = %s;\n", tn->storewhere, tn->storewhat);
	    emitcode(tn->next);
	    break;
	case BEGINNODE:
	    emitbegin(tn);
	    break;
	case CASENODE:
	    emitcase(tn);
	    break;
	case COMMENTNODE:
	    indent(lexlev);
	    printf("/*");
	    for (cptr = tn->blkcmt; cptr != NULL; cptr = cptr->next)
		{
		if (cptr != tn->blkcmt)
		    printf("\n");
		printf("%s", cptr->cmt);
		}
	    printf("*/\n");
	    emitcode(tn->next);
	    break;
	case FDECLNODE:
	    emitproc(tn,1);
	    break;
	case FORNODE:
	    emitfor(tn);
	    break;
	case GOTONODE:
	    emitgoto(tn);
	    break;
	case IFNODE:
	    emitif(tn);
	    break;
	case LABELNODE:
	    printf("L%s:\n", tn->expression);	/* must start w/ a letter */
	    emitcode(tn->next);
	    break;
	case PCALLNODE:
	    indent(lexlev);
	    printf("%s (%s);\n", tn->stdecl->st_name, tn->expression);
	    emitcode(tn->next);
	    break;
	case PDECLNODE:
	    emitproc(tn,0);
	    break;
	case READNODE:
	    indent(lexlev);
	    printf("%s\n", tn->expression);
	    emitcode(tn->next);
	    break;
	case REPEATNODE:
	    emitrepeat(tn,0);
	    break;
	case SEMINODE:
	    indent(lexlev);
	    printf(";\n");
	    emitcode(tn->next);
	    break;
	case WHILENODE:
	    emitwhile(tn);
	    break;
	case WITHNODE:
	    emitcode(tn->firstc);
	    emitcode(tn->next);
	    break;
	case WRITENODE:
	    indent(lexlev);
	    printf("%s\n", tn->expression);
	    emitcode(tn->next);
	    break;
	default:
	    emitcode(tn->firstc);
	    emitcode(tn->next);
	}
}


emitbegin(tn)
    struct treenode *tn;
{
    struct stentry *st;		/* st entry to emit code for */
    struct stentry *stptr;	/* temp pointer */
    int i,j;
    struct cmtinfo *cptr;

    switch (tn->blktype)
	{
	case DECLBLOCK:
	    if (tn->parent->type != PROGNODE)
		printf("{\n");
	    for (st = tn->firstsym; st != NULL;  st = st->st_link)
		{
		/*
		 * Skip over st entries from include file
		 */
		if (st->st_emit == 0)
		    if (st == tn->lastsym)
			break;
		    else
			continue;
		switch (st->st_class)
		    {
		    case COMTC:
			printf("\n/*");
			for (cptr = st->st_cmt; cptr != NULL; cptr = cptr->next)
			    {
			    if (cptr != st->st_cmt)
				printf("\n");
			    printf("%s", cptr->cmt);
			    }
			printf("*/\n\n");
			break;
		    case INCLUDEC:
			if (strcmp(st->st_name, "<stdio.h>"))
			    printf("#include \"%s\"", st->st_name);
			else
			    printf("#include %s", st->st_name);
			if (st->st_cmt != NULL)
			    prstcmt(st->st_cmt);
			printf("\n");
			break;
		    case CONSTC:
			emitconst(st);
			break;
		    case TYPEC:
		    case VARC:
			emittype(st,0);
			/*
			 * Skip over enumeration constants and
			 * entries of duplicate vars w/ same type
			 */
			if (st->st_dstruct == UDEFS && st->st_tipe == ENUMTY)
			    for (i = 0, j = st->st_numdims; i < j; i++)
				{
				if (i == 0)
				    st = st->st_uptr;
				else
				    st = st->st_link;
				}
			for (; st->st_dupvar != NULL; )
			    st = st->st_dupvar;
			/*
			 * If there are any more symbols to print,
			 * skip over any union "defines" 
			 */
			if (st != tn->lastsym)
			    for (stptr = st->st_link; stptr != NULL; stptr = stptr->st_link)
				{
				if (stptr->st_class == DEFINEC)
				    st = stptr;
				else
				    break;
				}
			break;
		    case PROCC:		/* forward stmts */
		    case FUNCC:
			prtipe(st);
			printf("%s();\n",st->st_name);
			break;
		    default:
			fprintf(stderr, "emitcode: class #%d not implemented yet\n",
			     st->st_class);
		    }	/* end switch on "class" */
		if (st == tn->lastsym)
		    break;	/* out of for loop (so we don't fall into procs) */
		}
	    break;
	case ELSEBLOCK:
	    emitcode(tn->firstc);
	    break;
	case THENBLOCK:
	case SEMIBLOCK:
	    indent(lexlev);
	    printf("{\n");
	    emitcode(tn->firstc);
	    indent(lexlev);
	    printf("}\n");
	    break;
	case SUBRBLOCK:
	    emitcode(tn->firstc);
	    if (tn->parent->stdecl->st_class == FUNCC &&
		tn->parent->stdecl->st_funcpar == 0)
		printf("    return(v___%s);\n", tn->parent->stdecl->st_name);
	    printf("}\n\n");
	    break;
	case PARAMBLOCK:
	    emitparam(tn);
	    break;
	case NOBLOCK:		/* proc/funct block */
	case WITHBLOCK:
	    emitcode(tn->firstc);
	    break;
	}    /* end switch on block type */
    emitcode(tn->next);
}


/*
 * Emit case stmt.  Switch in C.
 */

emitcase(tn)
    struct treenode *tn;
{
    struct treenode *listtn, *labeltn;

    indent(lexlev);
    printf("switch (%s)\n", tn->expression);
    lexlev++;
    indent(lexlev);
    printf("{\n");
    /*
     * loop thru listhead nodes (for case labels)
     */
    for (listtn = tn->firstc; listtn != NULL; listtn = listtn->next)
	{
	/* loop thru individual labels for this case section. */
	for (labeltn = listtn->firstc; labeltn != NULL; labeltn = labeltn->next)
	    {
	    indent(lexlev);
	    if (strcmp(labeltn->expression,"default"))
		printf("case %s:\n", labeltn->expression);
	    else
		printf("default:\n");
	    }
	/*
	 * Emit stmts hung off last label node.
	 */
	lexlev++;
	emitcode(listtn->lastc->firstc);
	indent(lexlev);
	printf("break;\n");
	lexlev--;
	}
    indent(lexlev);
    printf("}\n");
    lexlev--;
    emitcode(tn->next);
}


/*
 * Emit for stmt.
 */

emitfor(tn)
    struct treenode *tn;
{
    indent(lexlev);
    printf("for (%s = %s; %s %s %s; %s%s)\n", tn->variable, tn->initvalue,
	tn->variable, (tn->to) ? "<=" : ">=", tn->finalvalue, tn->variable,
	(tn->to) ? "++" : "--");
    /*
     * Do body of for loop
     */
    lexlev++;
    emitcode(tn->firstc);
    lexlev--;
    emitcode(tn->next);
}


/*
 * Emit goto stmt.
 * Labels must start with a letter, so we stuff in the letter 'L' before
 * each label.
 */

emitgoto(tn)
    struct treenode *tn;
{
    indent(lexlev);
    printf("goto L%s;\n", tn->expression);
    emitcode(tn->next);
}


/*
 * Emit if stmt.
 */

emitif(tn)
    struct treenode *tn;
{
    indent(lexlev);
    printf("if (%s)\n", tn->expression);
    /*
     * Do 'then' part (true part)
     */
    lexlev++;
    emitcode(tn->firstc);
    /*
     * See if there is an 'else' part
     */
    if (tn->lastc != NULL)
	{
	indent(lexlev - 1);
	printf("else\n");
	emitcode(tn->lastc);
	}
    lexlev--;
    emitcode(tn->next);
}


/*
 * emit paramaters.  First the list of parameters, then their type
 * declarations.
 */

emitparam(tn)
    struct treenode *tn;
{
    struct stentry *st;

    printf("(");
    for (st=tn->firstsym; st != NULL; st = st->st_link)
	{
	printf("%s", st->st_name);
	if (st == tn->lastsym)
	    break;
	else
	    printf(", ");
	}
    printf(")\n");

    for (st=tn->firstsym; st != NULL; st = st->st_link)
	{
	emittype(st,0);
	/*
	 * Skip over entries of duplicate vars w/ same type.
	 */
	for (; st->st_dupvar != NULL; )
	    st = st->st_dupvar;
	if (st == tn->lastsym)
	    break;
	}
}


/*
 * Emit while stmt.
 */

emitwhile(tn)
    struct treenode *tn;
{
    indent(lexlev);
    printf("while (%s)\n", tn->expression);
    lexlev++;
    emitcode(tn->firstc);
    lexlev--;
    emitcode(tn->next);
}


/*
 * Emit repeat stmt.
 */

emitrepeat(tn)
    struct treenode *tn;
{
    indent(lexlev);
    printf("do {\n");
    lexlev++;
    emitcode(tn->firstc);
    lexlev--;
    indent(lexlev);
    printf("} while (%s);\n", tn->expression);
    emitcode(tn->next);
}


/* 
 * Emit constants.
 */

emitconst(st)
    struct stentry *st;
{
    printf("#define %s ",st->st_name);
    switch (st->st_tipe)
	{
	case INTTY:		/* also handles Pascal T/F as 1/0 */
	default:
	    printf("%d", st->st_cval);
	    break;
	case REALTY:
	    printf("%f", st->st_fcval);
	    break;
	case CHARTY:
	    printf("'%c'", st->st_cval);
	    break;
	case STRINGTY:
	    printf("\"%s\"", st->st_string);
	    break;
	case OCTALTY:
	    printf("0%s", st->st_uptr);
	    break;
	case HEXTY:
	    printf("0x%s", st->st_uptr);
	    break;
	case USERTYPE:
	    printf("%s", st->st_uptr->st_name);
	    break;
	}
    if (st->st_cmt != NULL)
	prstcmt(st->st_cmt);
    printf("\n");
}


/* 
 * Emit data types.
 * called by emitbegin, emitparam, emitproc
 */

emittype(st, nosemi)
    struct stentry *st;
    char nosemi;		/* true if no ';' wanted (function type) */
{
    register int i;
    register struct stentry *stptr;
    register struct pairs *pr;

    indent(lexlev);
    if (st->st_class == TYPEC && st->st_dstruct != RECORDS)
	printf("typedef ");
    switch (st->st_dstruct) {
	case NOSTRUCT:		/* symple type */
	case SUBRANGES: /* No subranges in C so make it just a scaler type */
	case SETS: 	/* No sets in C so make it just a scaler type */
	    prtipe(st);
	    if (st->st_funcpar && st->st_class == VARC)
		printf("(*");
	    printf("%s%s%s", (st->st_byref) ? "*" : "",
			(st->st_funcvar) ? "v___" : "", st->st_name);
	    if (st->st_funcpar && st->st_class == VARC)
		printf(")()");
	    for (stptr = st->st_dupvar; stptr != NULL; stptr = stptr->st_dupvar)
		printf(", %s%s", (st->st_byref) ? "*" : "", stptr->st_name);
	    if (st->st_value != NULL)
		printf(" = %s", st->st_value);
	    if (nosemi == 0)
		printf(";");
	    if (st->st_cmt != NULL)
		prstcmt(st->st_cmt);
	    printf("\n");
	    break;
	case ARRS:
	    prtipe(st);
	    printf("%s", st->st_name);
	    pr = st->st_bounds;
	    for (i=0; i < st->st_numdims; i++)
		{
		if (pr->pr_upper == -1)
		    printf("[%s+1]", pr->pr_uuser->st_name);
		else
		    printf("[%d]", pr->pr_upper);
		pr = pr->pr_next;
		}
	    for (stptr = st->st_dupvar; stptr != NULL; stptr = stptr->st_dupvar)
		{
		printf(", %s", stptr->st_name);
		pr = st->st_bounds;
		for (i=0; i < st->st_numdims; i++)
		    {
		    if (pr->pr_upper == -1)
			printf("[%s+1]", pr->pr_uuser->st_name);
		   else
		       printf("[%d]", pr->pr_upper);
		    pr = pr->pr_next;
		    }
		}
	    if (st->st_value != NULL)
		printf(" = %s", st->st_value);
	    if (nosemi == 0)
		printf(";");
	    if (st->st_cmt != NULL)
		prstcmt(st->st_cmt);
	    printf("\n");
	    break;
	case UDEFS:
	    if (st->st_tipe == ENUMTY)
		{
		if (st->st_class == TYPEC)
		    printf("enum %s {", st->st_name);
		else
		    printf("enum dummy {");
		/*
		 * Follow uptr to 1st enum const!  This is necessary for
		 * constructs like: v1, v2: (red,blue,green);
		 * v2's st_link does NOT point to red, but rather to the
		 * next variable.
		 */
		stptr = st;
		for (i=0; i < st->st_numdims; i++)
		    {
		    if (i == 0)
			stptr = stptr->st_uptr;
		    else
			stptr = stptr->st_link;
		    printf("%s", stptr->st_name);
		    if (i < st->st_numdims-1)
			printf(", ");
		    }
		if (st->st_class == TYPEC)
		    {
		    printf("}");
		    if (st->st_value != NULL)
			printf(" = %s", st->st_value);
		    printf(";");
		    if (st->st_cmt != NULL)
			prstcmt(st->st_cmt);
		    printf("\n");
		    }
		else
		    {
		    printf("} ");
		    printf("%s", st->st_name);
		    for (stptr = st->st_dupvar; stptr != NULL; stptr = stptr->st_dupvar)
			printf(", %s", stptr->st_name);
		    if (st->st_value != NULL)
			printf(" = %s", st->st_value);
		    printf(";");
		    if (st->st_cmt != NULL)
			prstcmt(st->st_cmt);
		    printf("\n");
		    }
		}   /* end ENUMTY */
	    else
		{
		if (st->st_tipe == USERTYPE)
		    {
		    prtipe(st);
		    if (st->st_funcpar && st->st_class == VARC)
			printf("(*");
		    printf("%s%s%s", (st->st_byref) ? "*" : "",
				(st->st_funcvar) ? "v___" : "", st->st_name);
		    if (st->st_funcpar && st->st_class == VARC)
			printf(")()");
		    for (stptr = st->st_dupvar; stptr != NULL; stptr = stptr->st_dupvar)
			printf(", %s%s", (st->st_byref) ? "*" : "", stptr->st_name);
		    if (st->st_value != NULL)
			printf(" = %s", st->st_value);
		    if (nosemi == 0)
			printf(";");
		    if (st->st_cmt != NULL)
			prstcmt(st->st_cmt);
		    printf("\n");
		    }
		}
	    break;
	case RECORDS:
	    if (st->st_tipe == UNIONTY)
		printf("union {\n");
	    else if (st->st_class == TYPEC)
		printf("struct %s {\n", st->st_name);
	    else
		printf("struct {\n");
	    lexlev++;
	    stptr = st;
	    for (stptr = stptr->st_next; stptr != NULL ; )
		{
		emittype(stptr,0);
		/*
		 * In case of dupvars as field list items, its only
		 * from the end of the dupvar chain that you can:
		 * 1) Follow the "st_link" pointer to go on after
		 *    a nested record.
		 * 2) Follow the "st_next" pointer to go on after
		 *    anything other than a nested record type.
		 */
		if (stptr->st_dstruct == RECORDS)
		    {
		    for (; stptr->st_dupvar != NULL; )
			stptr = stptr->st_dupvar;
		    stptr = stptr->st_link;
		    }
		else
		    {
		    for (; stptr->st_dupvar != NULL; )
			stptr = stptr->st_dupvar;
		    stptr = stptr->st_next;
		    }
		}

	    indent(lexlev);
	    if (st->st_class == TYPEC)
		{
		printf("}");
		if (st->st_value != NULL)
		    printf(" = %s", st->st_value);
		printf(";");
		if (st->st_cmt != NULL)
		    prstcmt(st->st_cmt);
		printf("\n");
		}
	    else
		{
		printf("} %s", st->st_name);
		for (stptr = st->st_dupvar; stptr != NULL; stptr = stptr->st_dupvar)
		    printf(", %s", stptr->st_name);
		if (st->st_value != NULL)
		    printf(" = %s", st->st_value);
		if (nosemi == 0)
		    printf(";");
		if (st->st_cmt != NULL)
		    prstcmt(st->st_cmt);
		printf("\n");
		}
	    lexlev--;
	    /*
	     * Print out any union field "defines"
	     */
	    for (stptr = st->st_link; stptr != NULL; stptr = stptr->st_link)
		{
		if (stptr->st_class == DEFINEC)
		    printf("#define %s %s\n", stptr->st_name, stptr->st_cmt->cmt);
		else
		    break;
		}
	    break;
	case FILESTR:
	    printf("FILE *%s", st->st_name);
	    if (st->st_value != NULL)
		printf(" = %s", st->st_value);
	    printf(";");
	    if (st->st_cmt != NULL)
		prstcmt(st->st_cmt);
	    printf("\n");
	    break;
	case PTRS:
	    if (st->st_tipe == USERTYPE)
		{
		if (st->st_uptr != NULL)
		    if (st->st_uptr->st_dstruct == RECORDS)
			printf("struct ");
		printf("%s ", st->st_uptr->st_name);
		}
	    else
		prtipe(st);
	    printf("*%s", st->st_name);
	    for (stptr = st->st_dupvar; stptr != NULL; stptr = stptr->st_dupvar)
		printf(", *%s", stptr->st_name);
	    if (st->st_value != NULL)
		printf(" = %s", st->st_value);
	    if (nosemi == 0)
		printf(";");
	    if (st->st_cmt != NULL)
		prstcmt(st->st_cmt);
	    printf("\n");
	    break;
	}   /* switch (st->st_dstruct) */
}

/*
 * Print stentry comment
 */

prstcmt(chead)
    struct cmtinfo *chead;
{
    register struct cmtinfo *cptr;

    printf("\t/*");
    for (cptr = chead; cptr != NULL; cptr = cptr->next)
	{
	if (cptr != chead)
	    printf("\n");
	printf("%s", cptr->cmt);
	}
    printf("*/");
}


/*
 * Print base type
 */

prtipe(st)
    struct stentry *st;
{
    switch (st->st_tipe) {
	case BOOLTY:
	case CHARTY:
	    printf("char ");
	    break;
	case INTTY:
	    printf("int ");
	    break;
	case USERTYPE:
	    if (st->st_uptr->st_tipe == ENUMTY)
		printf("enum ");
	    else if (st->st_uptr->st_dstruct == RECORDS)
		printf("struct ");
	    printf("%s ",st->st_uptr->st_name);
	    break;
	case REALTY:
	    printf("float ");
	    break;
	case UNSIGNEDTY:
	    printf("unsigned ");
	    break;
	case DOUBLETY:
	    printf("double ");
	    break;
	default:
	    fprintf(stderr,"Internal error, routine prtipe, default case.\n");
	    fprintf(stderr,"    type = %d\n", st->st_tipe);
	    exit();
	    break;
	}   /* switch (st->st_tipe) */
}


emitproc(tn, func)
    struct treenode *tn;
    char func;
{
    if (func)
	emittype(tn->ftype,1);
    printf("%s ", tn->stdecl->st_name);
    lexlev++;
    emitcode(tn->firstc);
    lexlev--;
    emitcode(tn->next);
}
