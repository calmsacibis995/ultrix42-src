#ifndef lint
static char Sccsid[] = "@(#)col.c	4.1 (ULTRIX) 7/17/90";
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
 * 001	David Lindner Tue Dec 19 10:49:53 EST 1989
 *	- Added collation prefix to collation table name.
 *	- Check property table in col_prp().
 *
 */

#include "ic.h"

extern sym *sym_new();	/* DJL 001 */
extern void _sym_del();

int    equal;		/* true if equal weights required */
i_char prm_wgt;		/* cell for primary weight */
i_char sec_wgt;		/* cell for secondary weight */
static sym *curcol;	/* pointer to collation table header */
static coll *coltab;	/* collation table */
static coll colrest;	/* collation for remaining characters */

/*
 * col_init -- init a collation table
 */
void
col_init(hdr)
sym *hdr;
{
	char nambuf[BUFSIZE];

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "col_init(%x)", hdr);
#endif

	/*
	 * DJL 001
	 */
	if (strcmp(hdr->sym_nam, COL_DEF)) {
		sprintf(nambuf, "%s%s", COL_PRFX, hdr->sym_nam);
		_sym_del(hdr);
		hdr = sym_new(nambuf);
	}

	/*
	 * update the database header
	 */
	i_hdr.i_nbrcl++;

	/*
 	 * set the default value to zero:
	 */
	colrest.cl_prim = colrest.cl_sec = (i_char)0;

	if ((coltab = new(coll, i_hdr.i_nblet)) == (coll *)0)
		fatal("no room for collation table '%s'", hdr->sym_nam);

	sym_set(hdr, SYM_COL);

	if (hdr->sym_typ == SYM_COL && hdr->sym_col == (cl_head *)0)
	{
		if ((hdr->sym_col = new(cl_head, 1)) == (cl_head *)0)
			fatal("no room for collation header for '%s'",
				hdr->sym_nam);

		col_anc = sym_ins(hdr, col_anc);
		curcol = hdr;

		/*
		 * fill the collation header
		 */
		strcpy(hdr->sym_col->cl_name, hdr->sym_nam);
	}
	prm_wgt = sec_wgt = 0;
}

/*
 * col_set -- set collation value for a code
 */
void
col_set(code, prim, sec)
sym *code;		/* code to set weigths of */
i_char prim;		/* codes primary weight */
i_char sec;		/* codes secondary weight */
{
	coll *cp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "col_set(%x, %u, %u)", code, prim, sec);
#endif

	if (code->sym_typ != SYM_CDF || code->sym_val->val_typ != VAL_COD)
		error("not a code '%s'", code->sym_nam);
	else
	{
		cp = &coltab[chrtoidx(code->sym_val->val_cod->cod_rep)];

		if (cp->cl_prim != 0 || cp->cl_sec != 0)
			error("duplicate weight for '%s'", code->sym_nam);
		else
			cp->cl_prim = prim, cp->cl_sec = sec;
	}
        /*
         * test if primary and secondary weights are within limits
         */
        weight_tst(prim, sec);

}

void
col_end(colsym)
sym *colsym;
{
	extern int yynerrs;		/* yacc error count 		*/
	FILE *colfp;
	register int i;
	register coll *cp;
	register i_char sec = 0;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "col_end(%x)", colsym);
	if (col_dbg)
		sym_dmp(colsym);
#endif

	/*
	 * handle the rest collation
	 */
	for (i = 0; i < i_hdr.i_nblet; i++)
	{
		cp = &coltab[i];

		if (cp->cl_prim == 0 && cp->cl_sec == 0)
			cp->cl_prim = colrest.cl_prim, cp->cl_sec = ++sec;
	}

	/*
	 * remember the default property table for this collation
	 */
	if (*colsym->sym_col->cl_pnam == '\0')
		strcpy(colsym->sym_col->cl_pnam, PRP_DEF);
	
	if (yynerrs == 0)
	{
		/*
		 * open a tmp file for the collation table
		 *	the file name is remembered in the collation header !
		 */
		if ((colfp = tmp_make(colsym->sym_col->cl_name)) != (FILE *)0)
		{
			/*
			 * write collation table to file
			 */
			for (i = 0, cp = coltab; i < i_hdr.i_nblet; i++, cp++)
				if (fwrite(cp, sizeof(coll), 1, colfp) != 1)
					fatal("cannot write collation table '%s'",
					      colsym->sym_nam);

			/*
			 * close collation table file
			 */
			fclose(colfp);
		}
		else
		{
			error("cannot create temp file for collation %s",
			      colsym->sym_nam);
			strcpy(colsym->sym_col->cl_name, colsym->sym_nam);
		}
	}

	/*
	 * free unused space
	 */
	free((char *)coltab);
	coltab = (coll *)0;
	curcol = (sym *)0;
}

/*
 * col_range -- set the collation order of a series of values
 */
void
col_range(fr, to, prim, equal)
sym *fr;
sym *to;
i_char prim;
int equal;
{
	register i_char sec = 0;	/* secondary weights */
	register coll *cp;		/* pointer into collation table */
	coll *cpfr;			/* first entry in collation table */
	coll *cpto;			/* last entry in collation table */

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "col_range(%x, %x, %u)", fr, to, prim);
#endif

	/*
	 * check start and end value:
	 */
	if (fr->sym_typ != SYM_CDF || to->sym_typ != SYM_CDF)
		error("illegal range specification");
	else
	{
		cpto = &coltab[chrtoidx(to->sym_val->val_cod->cod_rep)];
		cpfr = &coltab[chrtoidx(fr->sym_val->val_cod->cod_rep)];

		if (cpto < cpfr)
		{
			warning("inverted range");
			cp = cpto;
			cpto = cpfr;
			cpfr = cp;
		}

		for (cp = cpfr; cp != cpto; cp++)
			if (cp->cl_prim != 0 || cp->cl_sec != 0)
				error("duplicate weight for '%s'",
					codeset[cp - coltab]->sym_nam);
			else {
				cp->cl_prim = prim; 
				/* 
			 	 * if weights are equal assign lowest
				 * secondary weight otheerwise assign
				 * next secondary weight
				 */
				cp->cl_sec = equal ? 1 : ++sec;
			}
	}
        /*
         * test if primary and secondary weights are within limits
         */
        weight_tst(prim, sec);
}

/*
 * col_dipht -- collation for diphtongs
 */
void
col_dipht(dipht, first, second)
sym *dipht;
sym *first;
sym *second;
{
	register coll *cp;

	if (dipht->sym_typ != SYM_CDF
	    || first->sym_typ != SYM_CDF
	    || second->sym_typ != SYM_CDF
	    || val_len(first->sym_val) != 1
	    || val_len(second->sym_val) != 1
	    )
		error("illegal diphtong collation specification");
	else
	{
		cp = &coltab[chrtoidx(dipht->sym_val->val_cod->cod_rep)];

		if (cp->cl_prim != 0 || cp->cl_sec != 0)
			error("duplicate weight for '%s'",
				codeset[cp - coltab]->sym_nam);
		else
		{
			cp->cl_prim = first->sym_val->val_cod->cod_rep;
			cp->cl_sec = second->sym_val->val_cod->cod_rep;
		}
	}
}

/*
 * col_rest -- set collation value for unspecified rest
 */
void
col_rest(prim)
i_char prim;
{
#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "col_rest(%u)", prim);
#endif

	if (colrest.cl_prim != (i_char)0)
		error("duplicate rest collation");
	else
		colrest.cl_prim = prim;
}

/*
 * col_prp -- set name of property table for this collation
 */
void
col_prp(name)
sym *name;
{
	char nambuf[BUFSIZE];
	sym *tsp;

	/*
	 * DJL 001
	 */
	if (strcmp(name->sym_nam, PRP_DEF)) {
		sprintf(nambuf, "%s%s", PRP_PRFX, name->sym_nam);
		_sym_del(name);
		tsp = sym_find(nambuf);
		if (strcmp(tsp->sym_nam, nambuf) == 0)
			name = tsp;
	}

	if (sym_chk(name, SYM_PRP) == 0)
	{
		if (name->sym_typ == SYM_UDF)
			sym_del(name);
	}
	else if (*curcol->sym_col->cl_pnam == '\0')
	{
		strcpy(curcol->sym_col->cl_pnam, name->sym_nam);
		/*
		 * NOT_YET: could load property table here and then do
		 *	extensive checking.
		 */
	}
	else
	{
		error("duplicate property statement in collation %s",
			curcol->sym_col->cl_name);
		
	}
}

void
weight_tst(prim, sec)
i_char prim;
i_char sec;
{
        /*
         * print warning if primary weight >= WEIGHTMAX or secondary
         * weight >= WEIGHTMAX
         */
        if (prim >= WEIGHTMAX )
                warning("primary weight exceeds maximum value");

        if (sec >= WEIGHTMAX )
                warning("secondary weight exceeds maximum value");
}

#ifdef EBUG
/*
 * col_dmp -- dump a collation table
 */
void
col_dmp(colhdr)
cl_head *colhdr;
{
	register int i;
	register coll *cp;

	if (colhdr == (cl_head *)0)
	{
		dbg_prt(DBGTIN, "null cl_head pointer");
		return;
	}

	dbg_prt(DBGIN, "COLLATION %s :", colhdr->cl_name);
	dbg_prt(DBGTIN, "cl_offst: %ld", colhdr->cl_offst);

	for (cp = coltab, i = 0; cp && i < i_hdr.i_nblet; i++, cp++)
	{
		dbg_prt((i % 5) == 0 ? DBGTIN : DBGNOID, "(%04d/%04d) ",
			cp->cl_prim, cp->cl_sec);
	}

	dbg_prt(DBGOUT, "END.");
}
#endif
