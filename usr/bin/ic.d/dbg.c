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
static char Sccsid[] = "@(#)dbg.c	4.1	(ULTRIX)	7/17/90";
#endif

/*
 * @(#)dbg.c	1.1
 *
 *	functions used in debugging only
 *
 */

#define EBUG 1

#include "ic.h"

int yac_dbg = 0;		/* debug parser			*/
int lex_dbg = 0;		/* debug lexical analyser	*/
int sym_dbg = 0;		/* debug symbol entries		*/
int fct_dbg = 0;		/* debug functions		*/
int cod_dbg = 0;		/* debug code table		*/
int frm_dbg = 0;		/* debug format table		*/
int cnv_dbg = 0;		/* debug conversion tables	*/
int val_dbg = 0;		/* debug values			*/
int col_dbg = 0;		/* debug collation table	*/

FILE *dbgfp = stdout;

/*
 * sym_dmp -- dump contents of symbol for debugging purposes
 */
void
sym_dmp(sp)
sym *sp;
{
	if (sp)
	{
		dbg_prt(DBGIN, "SYMBOL: %4x %15.15s", sp,
			(sp->sym_nam != (char *)0) ? sp->sym_nam
						  : "?????????"
		       );

		dbg_prt(DBGNOID, ", next %4x", sp->sym_nxt);
		dbg_prt(DBGNOID, ", hashed %4x ", sp->sym_hshnxt);

		switch (sp->sym_typ)
		{
		case SYM_UDF:		/* name not yet defined */
			dbg_prt(DBGNOID, "SYM_UDF");
			break;

		case SYM_CDF:		/* name of a character */
			dbg_prt(DBGNOID, "SYM_CDF");
			val_dmp(sp->sym_val);
			break;
		
		case SYM_FDF:		/* name of a format */
			dbg_prt(DBGNOID, "SYM_FDF");
			val_dmp(sp->sym_val);
			break;
		
		case SYM_COD:		/* name for code set */
			dbg_prt(DBGNOID, "SYM_COD");
			break;

		case SYM_PRP:		/* property table name */
			dbg_prt(DBGNOID, "SYM_PRP");
			break;

		case SYM_COL:		/* collation name */
			dbg_prt(DBGNOID, "SYM_COL");
			col_dmp(sp->sym_col);
			break;

		case SYM_FRM:		/* name for format */
			dbg_prt(DBGNOID, "SYM_FRM");
			frm_dmp(sp->sym_frm);
			break;

		case SYM_CNV:		/* conversion name */
			dbg_prt(DBGNOID, "SYM_CNV");
			cnv_dmp(sp->sym_cnv);
			break;
		
		default:		/* ???? */
			dbg_prt(DBGNOID, "UNKNOWN TYPE '%x'", sp->sym_typ);
			break;
		}
		dbg_prt(DBGOUT|DBGNOID, "");
	}
	else
		dbg_prt(DBGTIN, "sym_dmp: ILLEGAL POINTER");
}

/*
 * val_dmp -- dump a value, following the chain.
 */
void
val_dmp(vp)
val *vp;
{
	if (vp == (val *)0)
	{
		dbg_prt(DBGTIN, "no value");
		return;
	}

	for (/*EMPTY*/; vp; vp = vp->val_nxt)
	{	
		dbg_prt(DBGIN, "value '%x' (%d byte(s)): ", vp, vp->val_len);

		if (vp->val_typ == VAL_COD)
			dbg_prt(DBGNOID, "VAL_COD (%x): %4x %4x", vp->val_cod,
				vp->val_cod->cod_rep, vp->val_cod->cod_prp);
		else if (vp->val_typ == VAL_STR)
			dbg_prt(DBGNOID, "VAL_STR (%x): \"%s\"", vp->val_str,
				vp->val_str);
		else if (vp->val_typ == VAL_SAM)
			dbg_prt(DBGNOID, "VAL_SAM");
		else if (vp->val_typ == VAL_VOI)
			dbg_prt(DBGNOID, "VAL_VOI");
		else
			dbg_prt(DBGNOID, "BUG: unknown value type");

		if (vp->val_nxt)
			dbg_prt(DBGNOID, ",");

		dbg_prt(DBGOUT|DBGNOID, "");
	}
}

/*
 * print indented debugging information
 */
/*VARARGS1*/
void
dbg_prt(inout, fmt, a1, a2, a3, a4, a5, a6, a7, a8)
int inout;
char *fmt;
int a1, a2, a3, a4, a5, a6, a7, a8;
{
	static indent = 0;
	register int i;

	if (inout & (DBGIN | DBGTIN))
		indent++;

	/*
	 * output indentation
	 */
	if (!(inout & DBGNOID))
	{	
		putc('\n', dbgfp);
		for (i = 0; i < indent; i++)
			fputs("    ", dbgfp);	/* vary string for offset */
	}
	
	fprintf(dbgfp, fmt, a1, a2, a3, a4, a5, a6, a7, a8);

	if (inout & (DBGOUT | DBGTIN))
		if (indent > 0)
			indent--;
}
