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

#ifndef lint
static char Sccsid[] = "@(#)cod.c	4.1	(ULTRIX)	7/17/90";
#endif

#include "ic.h"

sym **codeset;			/* ordered arry of code definitions   */
sym *cod_list;			/* pointer to unordered list of codes */

/*
 * cod_set -- assign the code list to the CODE identifier
 */
void
cod_set(code, list)
sym *code;			/* pointer to CODE symbol entry */
sym *list;			/* pointer to the head of the code table */
{
	register sym *sp;
	register int i;
	int compar();
	int fatalerr = 0;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "cod_set(%x, %x)", code, list);
#endif

	if (list == (sym *)0)
		fatal("empty codeset");

	/*
	 * allocate an array large enough to hold the whole code table
	 */
	if ((codeset = new(sym *, i_hdr.i_nblet)) == (sym **)0)
		fatal("no room for codeset table");

	/*
	 * fill table with codes
	 */
	for (sp = list, i = 0; sp; sp = sp->sym_nxt, i++)
		codeset[i] = sp;

	if (i != i_hdr.i_nblet)
		bug("cod_set");

	/*
	 * sort table in ascending machine collation order
	 */
	qsort(codeset, i_hdr.i_nblet, sizeof(sym *), compar);

	/*
	 * check for holes and duplicate entries
	 */
	for (i = 0; i < i_hdr.i_nbspl - 1; i++)
	{
		i_char code1 = codeset[i]->sym_val->val_cod->cod_rep;
		i_char code2 = codeset[i + 1]->sym_val->val_cod->cod_rep;

		if (code1 == code2)
		{
			error("duplicate code value for %s and %s",
				codeset[i]->sym_nam, codeset[i + 1]->sym_nam);
			/*
			 * NOT_YET: remove duplicates
			 */
			fatalerr = 1;
		}
		else if (code1 + 1 != code2)
		{
			error("missing code(s) between %x and %x", code1, code2);
			/*
			 * NOT_YET: fill these gaps
			 */
			fatalerr = 1;
		}
	}

#ifdef EBUG
	if (cod_dbg)
	{
		for (i = 0; i < i_hdr.i_nblet; i++)
			sym_dmp(codeset[i]);
	}
#endif
	/*
	 * temporary measure:
	 */
	if (fatalerr != 0)
		fatal("errors in code table");

	/*
	 * write the default property table to the temp file
	 */
	prp_end(prp_anc);
}

/*
 * cod_add -- add a code to the list of defined codes
 */
sym *
cod_add(newcod, oldcod)
sym *newcod;
sym *oldcod;
{

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "cod_add(%x, %x)", newcod, oldcod);
#endif

	if (newcod == (sym *)0)
	{

#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> %x", oldcod);
#endif

		return cod_list = oldcod;
	}

	newcod->sym_nxt = oldcod;

	/*
	 * update data base header
	 */
	i_hdr.i_nblet++;
	if (val_len(newcod->sym_val) == 1 || (i_hdr.i_flags & I_16))
		i_hdr.i_nbspl++;
	else
		i_hdr.i_nbdbl++;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", newcod);
#endif

	return cod_list = newcod;
}

sym *
cod_make(symbol, value)
sym *symbol;
val *value;
{

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "cod_make(%x, %x)", symbol, value);
#endif

	sym_chk(symbol, SYM_CDF);

	val_chk((value = valtocod(value, 0)), VAL_COD);

	/*
	 * check whether really a valid code definition:
	 */
	if (symbol->sym_typ != SYM_CDF || symbol->sym_val != (val *)0)
	{
		val_del(value);

#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> 0");
#endif

		return (sym *)0;
	}

	symbol->sym_val = value;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", symbol);
#endif

	return symbol;
}

/*
 * chrtoidx -- convert a code to the corresponding entry number
 */
int
chrtoidx(code)
register i_char code;
{
	register int i;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "chrtoidx(%x)", code);
#endif

	if (code < i_hdr.i_nbspl)
	{
#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> %x", code);
#endif
		return (int)code;
	}
	else
	{
		for (i = i_hdr.i_nbspl; i < i_hdr.i_nblet; i++)
			if (codeset[i]->sym_val->val_cod->cod_rep == code)
			{
#ifdef EBUG
				if (fct_dbg)
					dbg_prt(DBGOUT, "-> %x", i);
#endif
				return i;
			}
	}

	error("code '0x%4x' does not exist in code table", code);

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> 0");
#endif
	return 0;
}

int
compar(sympp1, sympp2)
sym **sympp1;
sym **sympp2;
{
	register unsigned val1 = (*sympp1)->sym_val->val_cod->cod_rep;
	register unsigned val2 = (*sympp2)->sym_val->val_cod->cod_rep;

	if (val1 > val2)
		return 1;
	if (val1 < val2)
		return -1;
	if (val1 == val2)
		return 0;

	bug("compar");

	/*NOTREACHED*/
}

/*
 * cod_first -- find first part of code
 */
cod *
cod_first(code)
i_char code;
{
	i_char first = ((code >> 8) & 0377);
	register sym *sp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "cod_first(%x)/%x", code, first);
#endif

	for (sp = cod_list; sp; sp = sp->sym_nxt)
		if (sp->sym_typ == SYM_CDF && sp->sym_val->val_typ == VAL_COD
		    && sp->sym_val->val_cod->cod_rep == first)
		{
#ifdef EBUG
			if (fct_dbg)
				dbg_prt(DBGOUT, "-> %x", sp->sym_val->val_cod);
#endif
			return sp->sym_val->val_cod;
		}

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> 0");
#endif

	return (cod *)0;
}
