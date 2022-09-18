#ifndef lint
static char Sccsid[] = "@(#)sym.c	4.1 (ULTRIX) 7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
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
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 001	David Lindner Tue Dec 19 10:26:37 EST 1989
 *	- Made sym_new(), and _sym_del() non local functions, also
 *	  fixed debug print statement that was incorrect.
 *
 */

#include "ic.h"
#include "y.tab.h"
#include <ctype.h>

#define HASHSIZE 127

/*
 * declaration of name validation routine
 */
static void name_valid();

/*
 * base array for hash
 */
static sym *symbase[HASHSIZE];
extern char *outname;			/* name of output file */

/*
 * DJL 001
 * sym_new -- create a new symbol table entry
 */
sym *
sym_new(name)
char *name;
{
	register sym *np;
	register int idx;

#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGIN, "sym_new(%s)", name);
#endif

	if ((np = new(sym, 1)) == (sym *)0)
		fatal("no room for symbol");

	/*
	 * initialize structure
	 */
	np->sym_typ = SYM_UDF;
	np->sym_nam = strsave(name);

	idx = hash(name);
	np->sym_hshnxt = symbase[idx];
	symbase[idx] = np;

#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGOUT, "-> %x", np);
#endif

	return np;
}

/*
 * sym_find -- locate a symbol table entry given the name of the symbol
 */
sym *
sym_find(name)
char *name;
{
	register sym *sp;

#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGIN, "sym_find(%s)", name);
#endif

	for (sp = symbase[hash(name)]; sp; sp = sp->sym_hshnxt)
		if (strcmp(sp->sym_nam, name) == 0)
			break;

#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGOUT, "-> %x", sp);
#endif

	return sp;
}

/*
 * sym_lookup -- symbol table lookup routine
 *	The return value is put in the global variable yylval
 */
int
sym_lookup(lexval)
int lexval;
{
	extern char yytext[];

#ifdef EBUG
	if (lex_dbg && fct_dbg)
		dbg_prt(DBGIN, "sym_lookup(%d = %s)", lexval, yytext);
#endif

	switch (lexval)
	{
	case Constant:			/* convert a constant to an integer */
		yylval.y_val = con_make(yytext);
		break;

	case Identifier:		/* remember an identifier */
		/*
		 * truncate to I_NAML characters if necessary
		 */
		if (strlen(yytext) >= I_NAML)
		{
			warning("Identifier %s truncated to %d characters",
				yytext, I_NAML);
			yytext[I_NAML - 1] = '\0';
		}
		if ((yylval.y_sym = sym_find(yytext)) != (sym *)0)
			break;
		yylval.y_sym = sym_new(yytext);
		break;
		
	case String:			/* remember a string constant */
		yylval.y_val = str_make(yytext);
		break;

	case Property:			/* make up a property */
		yylval.y_prp = prp_make(yytext);
		break;

	case IS:
		break;

	default:
		bug("sym_lookup1");
		break;
	}

#ifdef EBUG
	if (lex_dbg && fct_dbg)
		dbg_prt(DBGOUT, "-> %d (sets yylval to %x)", lexval, yylval.y_sym);
	else if (lex_dbg)
		dbg_prt(DBGTIN, "lex -> %s (%d)", yytext, lexval);
#endif

	return lexval;
}

/*
 * sym_set -- set the type of a symbol
 */
void
sym_set(sp, id)
sym *sp;
unsigned id;
{
#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGTIN, "sym_set(%x, %x)", sp, id);
#endif
	if (sp->sym_typ != SYM_UDF)
		error("redefinition of '%s'", sp->sym_nam);
	else
	{
		sp->sym_typ = id;

		/*
		 * handle the -o option:
		 */
		if (id == SYM_COD && outname != (char *)0)
		{
			int idx;

			/*
			 * place at correct place in hash
			 */
			idx = hash(sp->sym_nam);
			if (sp != symbase[idx] || sp->sym_hshnxt != (sym *)0)
				bug("cannot handle output reassign");
			else
			{
				symbase[idx] = (sym *)0;
				/*
				 * remember new output file name
				 */
				idx = hash(outname);
				symbase[idx] = sp;
				free(sp->sym_nam);
				sp->sym_nam = strsave(outname);
				outname = (char *)0;
			}
		}

		/*
		 * validate the identifier name
		 */
		name_valid(sp->sym_nam, id);
	}
}

/*
 * sym_chk -- check the type of the referenced identifier
 *	      returns 0 if not ok, 1 if ok
 */
int
sym_chk(sp, id)
sym *sp;
int id;
{
#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGIN, "sym_chk(%x, %x)", sp, id);
#endif

	if (sp == (sym *)0)
	{
#ifdef EBUG
		if (sym_dbg && fct_dbg)
			dbg_prt(DBGOUT, "-> 0");
#endif
		return 0;
	}

	if (sp->sym_typ != id)
	{
		if (sp->sym_typ == SYM_UDF && id != SYM_UDF)
		{
			error("reference to undefined Identifier '%s'", sp->sym_nam);
			/*
			 * make up a code if so requested
			 */
			if (id == SYM_CDF)
			{
				sp->sym_typ = SYM_CDF;
				sp->sym_val = con_make("-");
#ifdef EBUG
				if (sym_dbg && fct_dbg)
					dbg_prt(DBGOUT, "-> 1");
#endif

				return 1;
			}
		}
		else
			error("'%s' does not designate %s", sp->sym_nam,
				(id == SYM_UDF) ? "a free Identifier" :
				(id == SYM_CDF) ? "a code" :
				(id == SYM_FDF) ? "a format" :
				(id == SYM_COD) ? "a code table" :
				(id == SYM_COL) ? "a collation table" :
				(id == SYM_FRM) ? "a format table" :
				(id == SYM_CNV) ? "a conversion" :
				(id == SYM_PRP) ? "a property table" :
				"????????"
			);

#ifdef EBUG
		if (sym_dbg && fct_dbg)
			dbg_prt(DBGOUT, "-> 0");
#endif

		return 0;
	}

#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGOUT, "-> 1");
#endif

	return 1;
}

/*
 * sym_del -- delete a symbol chain
 */
void
sym_del(symbol)
sym *symbol;
{
	void _sym_del();
	register sym *sp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "sym_del(%x)", symbol);
#endif

	while (symbol != (sym *)0)
	{
		sp = symbol->sym_nxt;
		symbol->sym_nxt = (sym *)0;
		_sym_del(symbol);
		symbol = sp;
	}
}

/*
 * DJL 001
 * _sym_del -- delete a symbol preserving the definition chain
 */
void
_sym_del(symbol)
sym *symbol;
{
	void __sym_del();
	register sym *sp;
	int idx;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "_sym_del(%x)", symbol);
#endif

	/*
	 * remove from hash table:
	 */
	idx = hash(symbol->sym_nam);
	if (symbase[idx] == symbol)
		symbase[idx] = symbol->sym_hshnxt;
	else
	{
		for (sp = symbase[idx]; sp; sp = sp->sym_hshnxt)
			if (sp->sym_hshnxt == symbol)
			{
				sp->sym_hshnxt = sp->sym_hshnxt->sym_hshnxt;
				break;
			}
	}
	symbol->sym_hshnxt = (sym *)0;
	__sym_del(symbol);
}

/*
 * __sym_del -- delete the contents of a symbol entry
 */
static void
__sym_del(symbol)
sym *symbol;
{

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "__sym_del(%x)", symbol);
#endif

	if (symbol->sym_nam)
	{
		free(symbol->sym_nam);
		symbol->sym_nam = (char *)0;
	}

	if (symbol->sym_val != (val *)0)
	{
		if (symbol->sym_typ == SYM_CDF || symbol->sym_typ == SYM_FDF)
		{
			val_del(symbol->sym_val);
			symbol->sym_val = (val *)0;
		}
		else if (symbol->sym_typ == SYM_PRP)
		{
			if (symbol->sym_prp)
				free((char *)symbol->sym_prp);
			symbol->sym_prp = (pr_head *)0;
		}
		else if (symbol->sym_typ == SYM_COL)
		{
			if (symbol->sym_col)
				free((char *)symbol->sym_col);
			symbol->sym_col = (cl_head *)0;
		}
		else if (symbol->sym_typ == SYM_FRM)
		{
			if (symbol->sym_frm)
				free((char *)symbol->sym_frm);
			symbol->sym_frm = (st_head *)0;
		}
		else if (symbol->sym_typ == SYM_CNV)
		{
			if (symbol->sym_cnv)
				free((char *)symbol->sym_cnv);
			symbol->sym_cnv = (cv_head *)0;
		}
		else if (symbol->sym_typ != SYM_COD)
			bug("__sym_del1");
	}

	symbol->sym_typ = 0;
	free((char *)symbol);
}

sym *
sym_fake(name, typ)
char *name;
unsigned typ;
{
	sym *sp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "sym_fake(%s, %x)", name, typ);
#endif

	if ((sp = sym_find(name)) == (sym *)0)
	{
		sp = sym_new(name);
		sym_set(sp, typ);
	}

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", sp);
#endif

	return sp;
}

static int
hash(name)
char *name;
{
	register unsigned idx;
	register char *cp;

	for (cp = name, idx = 0; *cp; cp++)
		idx += (*cp & 0377);
	
	return (int)(idx % HASHSIZE);
}

/*
 * sym_ins -- insert a symbol into sorted list of symbols
 */
sym *
sym_ins(symbol, anc)
register sym *symbol;
sym *anc;
{
	register sym *sp;

	if (anc == (sym *)0)
		anc = symbol;
	else
	{
		if (strcmp(anc->sym_nam, symbol->sym_nam) > 0)
		{
			symbol->sym_nxt = anc;
			anc = symbol;
		}
		else
		{
			for (sp = anc; sp->sym_nxt; sp = sp->sym_nxt)
				if (strcmp(sp->sym_nxt->sym_nam, symbol->sym_nam) > 0)
					break;

			symbol->sym_nxt = sp->sym_nxt;
			sp->sym_nxt = symbol;
		}
	}
	return anc;
}

/*
 * sym_all_del -- delete all known symbols
 */
void
sym_all_del()
{
	sym **spp;

#ifdef EBUG
	if (sym_dbg && fct_dbg)
		dbg_prt(DBGTIN, "sym_all_del()");
	if (sym_dbg)
		dmp_all();
#endif

	for (spp = &symbase[0]; spp < &symbase[HASHSIZE - 1]; spp++)
		while (*spp)
		{	/*
			 * we follow the hash chain here to avoid
			 * problems in the order of definition
			 * and no longer care to preserve the sym_nxt 
			 * chain.
			 */
			(*spp)->sym_nxt = (sym *)0;
			_sym_del(*spp);
		}
}

/*
 * name_valid -- validate name of a symbol
 */
static void
name_valid(name, id)
char *name;
int id;
{
	register char *cp = name;

#ifdef EBUG
	/* DJL 001 */
	if (fct_dbg && lex_dbg)
		dbg_prt(DBGTIN, "name_valid(%x)", id);
#endif

	if (id == SYM_COD)
	{
		char *cpsav = name;
		int seenbar = 0;
		int seendot = 0;

		/*
		 * codeset name must adhere to X/OPEN syntax:
		 *	language[_territory[.codeset]]
		 * where all three members consist of alphanumeric chars.
		 * (This implementation requires the name to start with a
		 * letter!)
		 */

		for (;;)
		{
			while (isalnum(*cp))
				cp++;

			if (cp == cpsav)
				break;

			if (*cp == '\0')
				return;
			else if (*cp == '_')
				if (seenbar)
					break;
				else
				{
					seenbar++;
					cpsav = ++cp;
				}
			else if (*cp == '.')
				if (!seenbar || seendot)
					break;
				else
				{
					seendot++;
					cpsav = ++cp;
				}
			else
				break;
		}
		error("illegal codeset name '%s'", name);
	}
	else
	{
		/*
		 * ordinary identifier, should not contain '.'
		 */
		if (index(cp, '.') != (char *)0)
			warning("strange Identifier '%s'", cp);
	}
}

#ifdef EBUG
dmp_all()
{
	sym **spp;
	sym *sp;

	for (spp = &symbase[0]; spp < &symbase[HASHSIZE - 1]; spp++)
		for (sp = *spp; sp; sp = sp->sym_hshnxt)
			sym_dmp(sp);
}
#endif
