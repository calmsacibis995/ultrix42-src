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
static char Sccsid[] = "@(#)cnv.c	4.1	(ULTRIX)	7/17/90";
#endif

#include "ic.h"

typedef union cnv cnv;

union cnv {
	i_char	cv_code;	/* 	when code conversion		*/
	val	*cnv_val;	/*	when string conversion		*/
};

sym *cnv_anc;			/* list of conversions			*/
static cnv *cv_tab;		/* conversion table			*/
static cnv def_cnv;		/* default conversion			*/
static int def_val_typ;		/* type of default conversion		*/
static int cv_type;		/* set to != 0 if a string conversion	*/
static int cnv_len;		/* maximal length if string conversion	*/

/*
 * cnv_init -- initialize a conversion table
 */
sym *
cnv_init(hdr, typ)
sym *hdr;
int typ;
{

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "cnv_init(%x, %x)", hdr, typ);
#endif

	/*
	 * update the database header
	 */
	i_hdr.i_nbrcv++;

	def_cnv.cv_code = 0;	/* no default conversion		*/
	cv_type = typ;		/* remember conversion type		*/
	cnv_len = 0;		/* no length as of yet			*/
	def_val_typ = 0;	/* don't know default conversion yet	*/

	/*
	 * allocate memory for conversions
	 */
	if ((cv_tab = new(cnv, i_hdr.i_nblet)) == (cnv *)0)
		fatal("no room for conversion '%s'", hdr->sym_nam);

	sym_set(hdr, SYM_CNV);

	if (hdr->sym_typ == SYM_CNV && hdr->sym_cnv == (cv_head *)0)
	{
		if ((hdr->sym_cnv = new(cv_head, 1)) == (cv_head *)0)
			fatal("no room for conversion header '%s'",
				hdr->sym_nam);

		cnv_anc = sym_ins(hdr, cnv_anc);

		/*
		 * fill in conversion header
		 */
		strcpy(hdr->sym_cnv->cv_name, hdr->sym_nam);
		hdr->sym_cnv->cv_type = typ;
	}

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", hdr);
#endif

	return hdr;
}

/*
 * def_set -- remeber value for default conversion
 */
void
def_set(value)
val *value;
{
#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "def_set(%x)", value);
#endif

	if (def_val_typ != 0)
	{
		error("multiple default conversion");
		val_del(value);
	}
	else if (value->val_typ == VAL_VOI)
	{
		def_val_typ = VAL_VOI;
		val_del(value);
	}
	else if (value->val_typ == VAL_SAM)
	{
		def_val_typ = VAL_SAM;
		val_del(value);
	}
	else if (cv_type == CNV_COD)
	{
		def_val_typ = VAL_COD;
		value = valtocod(value, 1);
		def_cnv.cv_code = value->val_cod->cod_rep;
		val_del(value);
	}
	else if (cv_type == CNV_STR)
	{
		def_val_typ = VAL_STR;
		def_cnv.cnv_val = valtolist(value);
		cnv_len = max(cnv_len, val_len(value));
	}
	else
		bug("def_set1");
}

/*
 * cnv_set -- make an entry for a conversion
 */
void
cnv_set(sp, value)
sym *sp;		/* character to convert */
val *value;		/* value to convert it to */
{
	register cnv *cnv_ptr;
	int idx;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "cnv_set(%x, %x)", sp, value);
#endif

	if (sym_chk(sp, SYM_CDF) == 0)
	{
		val_del(value);
		return;
	}

	idx = chrtoidx(sp->sym_val->val_cod->cod_rep);
	
	if ((cnv_ptr = &cv_tab[idx])->cv_code != 0)
	{
		error("duplicate conversion for '%s'", sp->sym_nam);
		return;
	}

	if (cv_type == CNV_COD)
	{
		value = valtocod(value, 1);
		cnv_ptr->cv_code = value->val_cod->cod_rep;
		val_del(value);
	}
	else if (cv_type == CNV_STR)
	{
		cnv_ptr->cnv_val = valtolist(value);
		cnv_len = max(cnv_len, val_len(cnv_ptr->cnv_val));
	}
	else
		bug("cnv_set2");
}

/*
 * cnv_end -- finish up a conversion table
 */
void
cnv_end(hdr)
sym *hdr;			/* name of conversion	*/
{
	extern int yynerrs;	/* yacc error count	*/
	FILE *cnvfp;		/* conversion file	*/
	register int i;
	int dummy;

#ifdef EBUG
	if (fct_dbg)	
		dbg_prt(DBGTIN, "cnv_end(%x)", hdr);
#endif

	/*
	 * handle the default conversion:
	 */
	if (cv_type == CNV_COD)
	{
		if (def_val_typ == VAL_SAM)
		{
			for (i = 0; i < i_hdr.i_nblet; i++)
				if (cv_tab[i].cv_code == 0)
					cv_tab[i].cv_code = idxtocod((i_char)i, &dummy)->cod_rep;
		}
		else if (def_val_typ == VAL_COD)
		{
			for (i = 0; i < i_hdr.i_nblet; i++)
				if (cv_tab[i].cv_code == 0)
					cv_tab[i].cv_code = def_cnv.cv_code;
		}
		else if (def_val_typ != VAL_VOI && def_val_typ != 0)
			bug("cnv_end1");
	}
	else if (cv_type == CNV_STR)
	{
		if (def_val_typ == VAL_SAM)
		{
			for (i = 0; i < i_hdr.i_nblet; i++)
				if (cv_tab[i].cnv_val == (val *)0)
				{
					cv_tab[i].cnv_val = chrtoval((i_char)i);
					cnv_len = max(cnv_len,
						   val_len(cv_tab[i].cnv_val));
				}
		}
		else if (def_val_typ == VAL_STR)
		{
			for (i = 0; i < i_hdr.i_nblet; i++)
				if (cv_tab[i].cnv_val == (val *)0)
					cv_tab[i].cnv_val = def_cnv.cnv_val;
		}
		else if (def_val_typ != VAL_VOI && def_val_typ != 0)
			bug("cnv_end2");
	}
	else
		bug("cnv_end3");

	if (yynerrs == 0)
	{
		/*
		 * remember longest string conversion
		 * plus one is for the terminating \0
		 */
		if (cv_type == CNV_COD)
			hdr->sym_cnv->cv_size = i_hdr.i_nblet * sizeof(i_char);
		else
			hdr->sym_cnv->cv_size = ++cnv_len * i_hdr.i_nblet;

		/*
		 * write the conversion table to a file
		 *	the file name is remebered in the cv_head
		 */
		if ((cnvfp = tmp_make(hdr->sym_cnv->cv_name)) != (FILE *)0)
		{
			if (cv_type == CNV_COD)
			{
				for (i = 0; i < i_hdr.i_nblet; i++)
					if (fwrite(&cv_tab[i].cv_code, sizeof(i_char), 1, cnvfp) != 1)
						fatal("cannot write conversion table '%s'",
						      hdr->sym_nam);
			}
			else
			{
				for (i = 0; i < i_hdr.i_nblet; i++)
				{
					register val *vp;
					int len;

					for (vp = cv_tab[i].cnv_val; vp; vp = vp->val_nxt)
					{
						if (vp->val_typ != VAL_COD)
							bug("cnv_end4");

						put_code(vp->val_cod->cod_rep,
							cnvfp);
					}

					/*
					 * zero fill the conversion entry to
					 * maximal length
					 */
					for (len = val_len(cv_tab[i].cnv_val); len < cnv_len; len++)
						put_code((i_char)0, cnvfp);
				}
			}

			/*
			 * NOT_YET: perform check on correctness of table
			 *	could be done by stat and length compare
			 */

			/*
			 * close the conversion table file
			 */
			fclose(cnvfp);
		}
		else
		{
			error("cannot create temp file for conversion %s",
			      hdr->sym_nam);
			strcpy(hdr->sym_cnv->cv_name, hdr->sym_nam);
		}
	}

#ifdef EBUG
	if (cnv_dbg)
		sym_dmp(hdr);
#endif

	if (cv_type == CNV_STR)
		for (i = 0; i < i_hdr.i_nblet; i++)
			if (cv_tab[i].cnv_val != (val *)0
			    &&
			    cv_tab[i].cnv_val != def_cnv.cnv_val)
				val_del(cv_tab[i].cnv_val);

	free((char *)cv_tab);
	cv_tab = (cnv *)0;

	if (def_val_typ == VAL_STR && def_cnv.cnv_val != (val *)0)
		val_del(def_cnv.cnv_val);

	def_val_typ = 0;
	def_cnv.cv_code = (i_char)0;
}

/*
 * cnv_range -- convert a range of values (ONLY for code conversions)
 */
void
cnv_range(frstart, frend, tostart, toend)
sym *frstart;
sym *frend;
sym *tostart;
sym *toend;
{
	register cnv *cp;
	cnv *cvfrs;		/* start of conversion area */
	cnv *cvfre;		/* end of conversion area */
	register int i;
	int tosidx;		/* index of start value */
	int toeidx;		/* index of end value */

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "cnv_range(%x, %x, %x, %x)", frstart, frend,
			tostart, toend);
#endif

	if (cv_type != CNV_COD)
		error("ranges only allowed in CODE conversions");
	else if (frstart->sym_typ != SYM_CDF || frend->sym_typ != SYM_CDF
		 ||
		 tostart->sym_typ != SYM_CDF || toend->sym_typ != SYM_CDF)
		error("illegal range in conversion");
	else
	{
		cvfrs = &cv_tab[chrtoidx(frstart->sym_val->val_cod->cod_rep)];
		cvfre = &cv_tab[chrtoidx(frend->sym_val->val_cod->cod_rep)];
		tosidx = chrtoidx(tostart->sym_val->val_cod->cod_rep);
		toeidx = chrtoidx(toend->sym_val->val_cod->cod_rep);

		if (cvfrs > cvfre)
		{
			warning("inverted source range");
			cp = cvfrs;
			cvfrs = cvfre;
			cvfre = cp;
		}

		if (tosidx > toeidx)
		{
			warning("inverted destination range");
			i = tosidx;
			tosidx = toeidx;
			toeidx = i;
		}

		if (toeidx - tosidx != cvfre - cvfrs)
			error("ranges do not match");
		else
		{
			/*
			 * do the conversion assignments
			 */
			for (cp = cvfrs, i = tosidx; i <= toeidx; cp++, i++)
			{
				if (cp->cv_code != 0)
					error("duplicate conversion for '%s'",
						codeset[cp - cv_tab]->sym_nam);
				else
					cp->cv_code = codeset[i]->sym_val->val_cod->cod_rep;
			}
		}
	}
}

#ifdef EBUG
/*
 * cnv_dmp -- dump a conversion table
 */
void
cnv_dmp(hdr)
cv_head *hdr;
{
	int i;

	if (hdr == (cv_head *)0)
	{
		dbg_prt(DBGTIN, "null cv_head pointer");
		return;
	}

	dbg_prt(DBGIN, "%s CONVERSION %s :",
		((hdr->cv_type == CNV_COD) ? "CODE" : "STRING"), hdr->cv_name);
	dbg_prt(DBGTIN, "cv_offst %ld, cv_size %d", hdr->cv_offst, hdr->cv_size);

	if (cv_tab != (cnv *)0)
	{
		if (hdr->cv_type == CNV_COD)
		{
			for (i = 0; i < i_hdr.i_nblet; i++)
				dbg_prt((i + 0) % 10 == 0 ? DBGTIN : DBGNOID,
					"%4d ", cv_tab[i].cv_code);
		}
		else
		{
			for (i = 0; i < i_hdr.i_nblet; i++)
				val_dmp(cv_tab[i].cnv_val);
		}
	}
	dbg_prt(DBGOUT, "END.");
}
#endif
