#ifndef lint
static char Sccsid[] = "@(#)frm.c	4.1 (ULTRIX) 7/17/90";
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
 * 002	David Lindner Tue Dec 19 10:53:12 EST 1989
 *	- Added form prefix form table name.
 *
 * 001	David Lindner Tue Dec 12 13:01:25 EST 1989
 *	- Set default for RADIXCHAR if it is set to "". The default
 *	  is currently "."
 *
 */

#include "ic.h"
#include <langinfo.h>

extern sym *sym_new();			/* DJL 002 */
extern void _sym_del();

sym *frm_anc;				/* head of format definition */
static sym *frm_tab = (sym *)0;
static int frm_cnt;			/* count of formats		*/

/*
 * table of required strings
 * NOT_YET: could be loaded from a file
 */
static char *frm_req[] = {
	AM_STR,			/* ante meridiem suffix			*/
	PM_STR,			/* post meridiem suffix			*/
	D_T_FMT,		/* standard string for date(1)		*/
	D_FMT,			/* date format string			*/
	T_FMT,			/* time format string			*/
	CRNCYSTR,		/* currency symbol			*/
	EXPL_STR,		/* lower case exponent character	*/
	EXPU_STR,		/* upper case exponent character	*/
	RADIXCHAR,		/* radix character			*/
	THOUSEP,		/* thousand separator			*/
	NOSTR,			/* negative answer			*/
	YESSTR,			/* affirmative answer			*/
	DAY_1,			/* days of the week (i.e. Sunday)	*/
	DAY_2,
	DAY_3,
	DAY_4,
	DAY_5,
	DAY_6,
	DAY_7,
	ABDAY_1,		/* days of the week abbreviated form	*/
	ABDAY_2,
	ABDAY_3,
	ABDAY_4,
	ABDAY_5,
	ABDAY_6,
	ABDAY_7,
	MON_1,			/* months of the year 			*/
	MON_2,
	MON_3,
	MON_4,
	MON_5,
	MON_6,
	MON_7,
	MON_8,
	MON_9,
	MON_10,
	MON_11,
	MON_12,
	ABMON_1,		/* months of the year abbreviated form	*/
	ABMON_2,
	ABMON_3,
	ABMON_4,
	ABMON_5,
	ABMON_6,
	ABMON_7,
	ABMON_8,
	ABMON_9,
	ABMON_10,
	ABMON_11,
	ABMON_12,
	(char *)0		/* THIS TERMINATES THE LIST		*/
};

void
frm_init(hdr)
sym *hdr;
{
	char nambuf[BUFSIZE];

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "frm_init(%x)", hdr);
#endif

	/*
	 * DJL 002
	 */
	if (strcmp(hdr->sym_nam, FRM_DEF)) {
		sprintf(nambuf, "%s%s", FRM_PRFX, hdr->sym_nam);
		_sym_del(hdr);
		hdr = sym_new(nambuf);
	}

	/*
	 * update the database header
	 */
	i_hdr.i_nbrst++;

	frm_cnt = 0;

	/*
	 * set type of symbol
	 */
	sym_set(hdr, SYM_FRM);

	if (hdr->sym_typ == SYM_FRM && hdr->sym_frm == (st_head *)0)
	{
		if ((hdr->sym_frm = new(st_head, 1)) == (st_head *)0)
			fatal("no room for format header '%s'", hdr->sym_nam);

		frm_anc = sym_ins(hdr, frm_anc);

		/*
		 * set format table header
		 */
		strcpy(hdr->sym_frm->st_name, hdr->sym_nam);
	}
}

/*
 * frm_set -- make a new format
 */
sym *
frm_set(ident, value)
sym *ident;
val *value;
{
	
	if (!strcmp(ident->sym_nam, "RADIXCHAR") && !strcmp(value->val_str, ""))
	{
		fprintf(stderr, "\nwarning: RADIXCHAR not set, defaulting to \".\"\n\n");
		strcpy(value->val_str, ".");
	}
#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "frm_set(%x, %x)", ident, value);
#endif

	if (sym_chk(ident, SYM_FDF) == 0)
	{
		val_del(value);
		sym_del(ident);
#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> 0");
#endif
		return (sym *)0;
	}

	ident->sym_val = valtolist(value);

	/*
	 * increment format counter
	 */
	frm_cnt++;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", ident);
#endif

	return ident;
}

/*
 * frm_add -- add a new format to the format list
 */
sym *
frm_add(newfrm, old)
sym *newfrm;		/* the new format */
sym *old;		/* the already existing format list */
{

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "frm_add(%x, %x)", newfrm, old);
#endif

	if (old == (sym *)0)
	{
#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> %x", newfrm);
#endif

		return newfrm;
	}

	if (newfrm == (sym *)0)
	{
#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> %x", old);
#endif

		return old;
	}

	/*
	 * add the format at the correct place in the list. saves the effort
	 * of sorting them for output
	 */
	old = sym_ins(newfrm, old);

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", old);
#endif

	return old;
}

/*
 * frm_end -- end of a format section
 */
void
frm_end(hdr, list)
sym *hdr;
sym *list;
{
	extern int yynerrs;		/* yacc error count		*/
	FILE *frmfp;
	sym *sp;
	str_tab idx_hdr;		/* index header in format table	*/
	char **cpp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "frm_end(%x, %x)", hdr, list);
#endif

	/*
	 * check on the availability of all required formats
	 */
	for (cpp = &frm_req[0]; *cpp; cpp++)
		if (sym_find(*cpp) == (sym *)0)
			error("mandatory string \"%s\" missing", *cpp);

	if (hdr->sym_typ != SYM_FRM)
	{
		sym_del(list);
		return;
	}

	frm_tab = list;
	hdr->sym_frm->st_siz = frm_cnt;

#ifdef EBUG
	if (frm_dbg)
		sym_dmp(hdr);
#endif

	if (yynerrs == 0)
	{
		/*
		 * write the format table to a file
		 *	the file name is remebered in the st_head
		 */
		if ((frmfp = tmp_make(hdr->sym_frm->st_name)) != (FILE *)0)
		{
			/*
			 * calculate offset to first string:
			 */
			idx_hdr.st_offst = (long)hdr->sym_frm->st_siz
					  *
					  (long)sizeof(str_tab);

			/*
			 * write header array to file
			 */
			for (sp = list; sp; sp = sp->sym_nxt)
			{
				zero(idx_hdr.st_name, I_NAML);
				strcpy(idx_hdr.st_name, sp->sym_nam);

				idx_hdr.st_siz = val_len(sp->sym_val);

				if (fwrite(&idx_hdr, sizeof(str_tab), 1, frmfp) != 1)
					fatal("cannot write format table '%s'",
					      hdr->sym_nam);
				/*
				 * offset to next string is current offset
				 * plus the length of this string plus one
				 * for the terminating null.
				 */
				idx_hdr.st_offst += (long)(idx_hdr.st_siz + 1);
			}

			/*
			 * write values to file
			 */
			for (sp = list; sp; sp = sp->sym_nxt)
			{
				register val *vp;

				for (vp = sp->sym_val; vp; vp = vp->val_nxt)
				{
					if (vp->val_typ != VAL_COD)
						bug("frm_end1");

					put_code(vp->val_cod->cod_rep, frmfp);
				}

				/*
				 * add the terminating \0
				 */
				put_code((i_char)0, frmfp);
			}

			/*
			 * misuse the hdr->sym_frm->st_offst field to remember
			 * the size of the format file.
			 */
			hdr->sym_frm->st_offst = idx_hdr.st_offst;

			/*
			 * close the format table file
			 */
			fclose(frmfp);
		}
		else
		{
			error("cannot create temp file for format %s",
			      hdr->sym_nam);
			strcpy(hdr->sym_frm->st_name, hdr->sym_nam);
		}
	}

	/*
	 * free no longer used space
	 */
	sym_del(frm_tab);
	frm_tab = (sym *)0;
}

#ifdef EBUG
/*
 * frm_dmp -- dump a format table
 */
void
frm_dmp(hdr)
st_head *hdr;
{
	register sym *sp;

	if (hdr == (st_head *)0)
	{
		dbg_prt(DBGTIN, "null st_head pointer");
		return;
	}

	dbg_prt(DBGIN, "FORMAT %s : ", hdr->st_name);
	dbg_prt(DBGTIN, "st_offst: %ld, st_siz = %d", hdr->st_offst, hdr->st_siz);

	for (sp = frm_tab; sp; sp = sp->sym_nxt)
		sym_dmp(sp);

	dbg_prt(DBGOUT, "END.");
}
#endif
