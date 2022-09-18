#ifndef lint
static char Sccsid[] = "@(#)prp.c	4.1 (ULTRIX) 7/17/90";
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
 * Modificatin History
 * ~~~~~~~~~~~~~~~~~~~
 * 001	David Lindner Tue Dec 19 10:57:13 EST 1989
 *	- Added property prefix to property table name.
 *
 */

#include "ic.h"
#include <errno.h>

extern sym *sym_new();		/* DJL 001 */
extern void _sym_del();

/*
 * prp_init -- initialize a property list
 */
void
prp_init(hdr)
sym *hdr;
{
	register sym **spp;
	char nambuf[BUFSIZE];

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "prp_init(%x)", hdr);
#endif

	/*
	 * DJL 001
	 */
	if (strcmp(hdr->sym_nam, PRP_DEF)) {
		sprintf(nambuf, "%s%s", PRP_PRFX, hdr->sym_nam);
		_sym_del(hdr);
		hdr = sym_new(nambuf);
	}

	/*
	 * update the database header
	 */
	i_hdr.i_nbrpr++;

	sym_set(hdr, SYM_PRP);

	if (hdr->sym_typ == SYM_PRP && hdr->sym_prp == (pr_head *)0)
	{
		if ((hdr->sym_prp = new(pr_head, 1)) == (pr_head *)0)
			fatal("no room for property header for '%s'",
				hdr->sym_nam);

		prp_anc = sym_ins(hdr, prp_anc);

		/*
		 * fill the property header
		 */
		strcpy(hdr->sym_prp->pr_name, hdr->sym_nam);
	}

	/*
	 * reset all properties in the code set if code set is defined
	 */
	if (codeset != (sym **)0)
		for (spp = codeset; spp < &codeset[i_hdr.i_nblet]; spp++)
			(*spp)->sym_val->val_cod->cod_prp = I_ILLEGAL;
}

/*
 * prp_end -- end the property list handling
 */
void
prp_end(hdr)
sym *hdr;
{
	extern int yynerrs;		/* yacc error count 		*/
	register int i;
	FILE *prpfp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "prp_end(%x)", hdr);
#endif

	if (yynerrs == 0)
	{
		/*
		 * open a tmp file for the property table
		 *	the file name is remembered in the property header !
		 */
		if ((prpfp = tmp_make(hdr->sym_prp->pr_name)) != (FILE *)0)
		{
			/*
			 * write property table to file
			 */
			for (i = 0; i < i_hdr.i_nblet; i++)
				if (fwrite(&(codeset[i]->sym_val->val_cod->cod_prp), sizeof(i_char), 1, prpfp) != 1)
					fatal("cannot write property table '%s'",
					      hdr->sym_nam);

			/*
			 * close property table file
			 */
			fclose(prpfp);
		}
		else
		{
			error("cannot create temp file for property table %s",
			      hdr->sym_nam);
			strcpy(hdr->sym_prp->pr_name, hdr->sym_nam);
		}
	}
}

/*
 * prp_make -- convert a string to a property
 */
i_char
prp_make(str)
char *str;
{
	register char *cp;
	int swval;
	i_char prop;

#ifdef EBUG
	if (lex_dbg && fct_dbg)
		dbg_prt(DBGIN, "prp_make(%s)", str);
#endif

	for (cp = str, swval = 0; *cp; cp++)
		swval += (int)*cp;

	switch (swval)
	{
	case 'D' + 'O' + 'U' + 'B' + 'L' + 'E':
		prop = I_FIRST;		/* misused to mark double letters */
		break;
	case 'A' + 'R' + 'I' + 'T' + 'H':
		prop = I_ARITH;
		break;
	case 'B' + 'L' + 'A' + 'N' + 'K':
		prop = I_BLANK;
		break;
	case 'N' + 'U' + 'M' + 'E' + 'R' + 'A' + 'L':
		prop = I_DIGIT;
		break;
	case 'H' + 'E' + 'X':
		prop = I_HEX;
		break;
	case 'U' + 'P' + 'P' + 'E' + 'R':
		prop = I_UPPER;
		break;
	case 'L' + 'O' + 'W' + 'E' + 'R':
		prop = I_LOWER;
		break;
	case 'I' + 'L' + 'L' + 'E' + 'G' + 'A' + 'L':
		prop = I_ILLEGAL;
		break;
	case 'D' + 'I' + 'A' + 'C' + 'R' + 'I' + 'T':
		prop = I_DIACR;
		break;
	case 'P' + 'U' + 'N' + 'C' + 'T':
		prop = I_PUNCT;
		break;
	case 'S' + 'U' + 'P' + 'S' + 'U' + 'B':
		prop = I_SUPSUB;
		break;
	case 'D' + 'I' + 'P' + 'H' + 'T' + 'O' + 'N' + 'G':
		prop = I_DIPHT;
		break;
	case 'F' + 'R' + 'A' + 'C' + 'T' + 'I' + 'O' + 'N':
		prop = I_FRACT;
		break;
	case 'S' + 'P' + 'A' + 'C' + 'E':
		prop = I_SPACE;
		break;
	case 'C' + 'T' + 'R' + 'L':
		prop = I_CTRL;
		break;
	case 'C' + 'U' + 'R' + 'E' + 'N' + 'C' + 'Y':
		prop = I_CURENCY;
		break;
	case 'M' + 'I' + 'S' + 'C' + 'E' + 'L':
		prop = I_MISCEL;
		break;
	default:
		bug("prp_make1");
	}

#ifdef EBUG
	if (lex_dbg && fct_dbg)
		dbg_prt(DBGOUT, "-> %x", prop);
#endif

	return prop;
}

/*
 * prp_add -- add a property to the list of properties already there
 */
i_char
prp_add(nprop, oprop)
i_char nprop;
i_char oprop;
{

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "prp_add(%x, %x)", nprop, oprop);
#endif

	if (oprop != I_ILLEGAL && nprop == I_ILLEGAL)
		error("character having properties cannot be illegal");
	else
	{
		if ((oprop & nprop) != 0)
			warning("duplicate property");
		oprop |= nprop;
	}

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", oprop);
#endif

	return oprop;
}

/*
 * prp_set -- set properties of a character doing a validity check
 */
i_char
prp_set(sp, props)
sym *sp;
register i_char props;
{

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "prp_set(%x, %x)", sp, props);
#endif

	/*
	 * if sp is 0 (can only happen in the error case)
	 * don't do anything and return 0.
	 */
	if (sp == (sym *)0)
	{
#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> 0");
#endif
		return (i_char)0;
	}

	/*
	 * if no value, make one up (can happen only in case of error)
	 */
	sym_chk(sp, SYM_CDF);

	/*
	 * check for space and an incompatible property
	 */
	if ((props & (I_SPACE|I_BLANK)) && (props & (I_DIGIT|I_UPPER|I_LOWER|
						    I_PUNCT|I_DIPHT|I_ARITH|
						    I_FRACT|I_SUPSUB|I_CURENCY)
				 )
	   )
		warning("%s a space character?", sp->sym_nam);

	/*
	 * check for invalid hex char
	 */
	if ((props & I_HEX) && !(props & (I_UPPER|I_LOWER|I_DIGIT)))
		warning("%s hex char but not letter or digit?", sp->sym_nam);

	/*
	 * if blank must be a space also
	 */
	if ((props & I_BLANK) && !(props & I_SPACE))
		warning("%s blank but not space?", sp->sym_nam);

	/*
	 * NOT_YET: are there more checks to do?
	 */

	/*
	 * if not a code definition make an error return here
	 */
	if (sp->sym_typ != SYM_CDF || sp->sym_val->val_typ != VAL_COD)
	{
#ifdef EBUG
		if (fct_dbg)
			dbg_prt(DBGOUT, "-> %x", props);
#endif
		return props;
	}


	/*
	 * special check for double letters
	 */
	if (props & I_FIRST)
	{
		/*
		 * clean up temp marker for double letter
		 */
		props &= ~I_FIRST;

		/*
		 * check if really two characters
		 */
		if (i_hdr.i_flags & I_16)
			error("double letters not allowed in 16 bit codsets");
		if (val_len(sp->sym_val) != 2)
			error("double letter not made up of two letters");
		else
		{
			/*
			 * flag first of double letter
			 */
			cod *cp = cod_first(sp->sym_val->val_cod->cod_rep);

			if (cp == (cod *)0)
				error("first letter does not exist");
			else
				cp->cod_prp |= I_FIRST;
		}
	}

	sp->sym_val->val_cod->cod_prp = props;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", props);
#endif

	return props;
}
