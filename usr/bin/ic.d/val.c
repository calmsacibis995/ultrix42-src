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
static char Sccsid[] = "@(#)val.c	4.1	(ULTRIX)	7/17/90";
#endif

/*
 * WARNING: The code in this file depends heavily on a 8 bit byte and a
 * 'char' type beeing a byte as defined before.
 */

#include "ic.h"

static val *val_new();
static cod *cod_new();

/*
 * con_make -- extract a constant from string str.
 *	The following formats are recognized:
 *	'x'	-	quoted character(s) (maximal 2)
 *	n/n	-	character code in ISO notation
 *	0xn	-	character code in HEX notation
 *	0n	-	character code in OCT notation
 *	n	-	character code in decimal notation
 *
 * RETURNS:
 *	decimal value of constant truncated on error
 */
val *
con_make(str)
char *str;
{
	i_char res = 0;		/* result accumulator */
	i_char tmp;		/* to remember tmp values */
	int esc = 0;		/* to remember escaped chars */
	register char *cp;	/* to remember position of separation char */
	val *vp;
	extern char *index();

#ifdef EBUG
	if (lex_dbg && fct_dbg)
		dbg_prt(DBGIN, "con_make(%s)", str);
#endif
	vp = val_new();

	if (*str == '\'')
	{	/* character constant of form 'x' */
		for (cp = str + 1; *cp != '\'' || esc; cp++)
		{
			if (esc == 0)
			{
				if (*cp == '\\')
				{
					esc = 1;
					continue;
				}
				else
					tmp = ((i_char)*cp & 0377);
			}
			else
			{
				esc = 0;

				switch (*cp)
				{
				case '0': break;
				case 'n': tmp = '\n'; break;
				case 'b': tmp = '\b'; break;
				case 'r': tmp = '\r'; break;
				case 't': tmp = '\t'; break;
				case 'f': tmp = '\f'; break;
				case '\'': tmp = '\''; break;
				case '\\': tmp = '\\'; break;
				default :
					tmp = ((i_char)*cp & 0377);
					error("illegal escape in constant %s", str);
					break;
				}
			}
			vp->val_len++;
			res = ((res << 8) | tmp);
		}
		if (*++cp)
			error("illegal character constant %s", str);
	}
	else if ((cp = index(str, '/')) != (char *)0)
	{	/* character in ISO notation */
		tmp = atoi(str);
		res = atoi(++cp);
		if (tmp > 0xf || res > 0xf)
			error("illegal ISO type constant %s", str);
		res = ((tmp << 4) | res);
		vp->val_len++;
	}
	else if ((cp = index(str, 'x')) != (char *)0)
	{	/* character in hex notation */
		for (cp++, tmp = 0; *cp; cp++, tmp++)
		{
			if (*cp >= '0' && *cp <= '9')
				res = (res << 4) + *cp - '0';
			else if (*cp >= 'A' && *cp <= 'F')
				res = (res << 4) + *cp - 'A' + 10;
			else if (*cp >= 'a' && *cp <= 'f')
				res = (res << 4) + *cp - 'a' + 10;
			else
				bug("con_make1");
			
		}
		if (tmp > 2 * sizeof(i_char))
			error("hex constant %s too large", str);
		vp->val_len = (tmp + 1) / 2;
	}
	else if (*str == '0')
	{	/* character in octal notation */
		tmp = 0;
		for (cp = str + 1; *cp; cp++)
		{
			if (*cp >= '0' && *cp <= '7')
			{
				if ((res = (res << 3) + *cp - '0') != 0)
					tmp++;
			}
			else
				bug("con_make2");
		}
		if (tmp > 3 * sizeof(i_char)
		    || (tmp == 3 * sizeof(i_char) && *(str + 1) != '1'))
			error("octal constant %s too large", str);
		vp->val_len = (tmp + 2) / 3;
	}
	else if (*str == '-')
	{	/* pseudo made up code THIS IS ONLY DONE IN CASE OF ERROR! */
		res = (i_char)-1;
		vp->val_len = 0;
	}
	else
	{	/* decimal constant */
		for (cp = str, res = 0; *cp; cp++)
		{
			if (*cp >= '0' && *cp <= '9')
			{
				tmp = (res * 10) + *cp - '0';
				if (tmp < res)
					error("decimal constant %s too large",
						str);
				else
					res = tmp;
			}
			else
				bug("con_make3");
		}

		/*
		 * The following only works for sizeof(bit16) == 2 !
		 */
		if (res > 0xff)
			vp->val_len = 2;
		else
			vp->val_len = 1;
	}

	vp->val_cod = cod_new();
	vp->val_cod->cod_rep = res;
	vp->val_typ = VAL_COD;

#ifdef EBUG
	if (lex_dbg && fct_dbg)
	{
		val_dmp(vp);
		dbg_prt(DBGOUT, "-> %x", vp);
	}
#endif

	return vp;
}

/*
 * str_make -- make a string value
 */
val *
str_make(str)
register char *str;
{
	val *vp;
	char strbuf[STRMAX];
	register char *cp;
	int esc = 0;			/* to remember escaped chars */

#ifdef EBUG
	if (lex_dbg && fct_dbg)
		dbg_prt(DBGIN, "str_make(%s)", str);
#endif

	/*
	 * handle escaped characters
	 *	below ++str is to strip the leading quotes
	 */
	for (++str, cp = strbuf; *str; str++)
	{
		if (cp >= &strbuf[STRMAX])
		{
			error("string too long, max %d characters", STRMAX - 1);
			break;
		}

		if (esc == 0)
		{
			if (*str == '\\')
				esc = 1;
			else
				*cp++ = *str;
		}
		else
		{
			esc = 0;

			switch (*str)
			{
			case '0': break;
			case 'n': *cp++ = '\n'; break;
			case 'b': *cp++ = '\b'; break;
			case 'r': *cp++ = '\r'; break;
			case 't': *cp++ = '\t'; break;
			case 'f': *cp++ = '\f'; break;
			default : *cp++ = *str; break;
			}
		}
	}

	/*
	 * add null terminator to compiled string
	 */
	*cp = '\0';

	/*
	 * convert string to value
	 */
	vp = val_new();

	vp->val_typ = VAL_STR;

	/*
	 * below -1 is to strip the terminating quote
	 */
	vp->val_len = strlen(strbuf) - 1;
	strbuf[vp->val_len] = '\0';

	vp->val_str = strsave(strbuf);
	
#ifdef EBUG
	if (lex_dbg && fct_dbg)
	{
		val_dmp(vp);
		dbg_prt(DBGOUT, "-> %x", vp);
	}
#endif

	return vp;
}

/*
 * var_make -- make an identifier a value
 */
val *
var_make(sp)
sym *sp;
{
	register val *vp;

#ifdef EBUG
	if (lex_dbg && fct_dbg)
		dbg_prt(DBGIN, "var_make(%x)", sp);
#endif

	if (sym_chk(sp, SYM_CDF) == 0)
	{
#ifdef EBUG
		if (lex_dbg && fct_dbg)
			dbg_prt(DBGOUT, "-> 0");
#endif
		return (val *)0;
	}

	if (sp->sym_val == (val *)0)
	{
		error("recursive definition of %s", sp->sym_nam);
#ifdef EBUG
		if (lex_dbg && fct_dbg)
			dbg_prt(DBGOUT, "-> 0");
#endif
		return (val *)0;
	}

	vp = val_new();

	vp->val_typ = VAL_COD;
	vp->val_cod = cod_new();
	vp->val_len = sp->sym_val->val_len;
	vp->val_cod->cod_rep = sp->sym_val->val_cod->cod_rep;
	vp->val_cod->cod_prp = sp->sym_val->val_cod->cod_prp;

#ifdef EBUG
	if (lex_dbg && fct_dbg)
	{
		val_dmp(vp);
		dbg_prt(DBGOUT, "-> %x", vp);
	}
#endif

	return vp;
}

/*
 * val_add -- build a composite value
 */
val *
val_add(newval, startval)
val *newval;
val *startval;
{
	register val *vp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "val_add(%x, %x)", newval, startval);
#endif

	if (startval == (val *)0)
	{
#ifdef EBUG
		if (fct_dbg)
		{
			val_dmp(newval);
			dbg_prt(DBGOUT, "-> %x", newval);
		}
#endif
		return newval;
	}

	for (vp = startval; vp->val_nxt; vp = vp->val_nxt)
	;
	vp->val_nxt = newval;

#ifdef EBUG
	if (fct_dbg)
	{
		val_dmp(startval);
		dbg_prt(DBGOUT, "-> %x", startval);
	}
#endif

	return startval;
}

/*
 * val_chk -- check whether a value only contains the given types
 *
 *	RETURNS the bit pattern containing the OR of all values encountered
 */
bit16
val_chk(vp, types)
register val *vp;
bit16 types;
{
	bit16 retval = 0;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "val_chk(%x, %x)", vp, types);
#endif

	for (/*EMPTY*/; vp; vp = vp->val_nxt)
	{
#ifdef EBUG
		if (fct_dbg > DBGFUN1)
			switch (vp->val_typ)
			{
			case VAL_COD: dbg_prt(DBGNOID, " VAL_COD"); break;
			case VAL_STR: dbg_prt(DBGNOID, " VAL_STR"); break;
			case VAL_SAM: dbg_prt(DBGNOID, " VAL_SAM"); break;
			case VAL_VOI: dbg_prt(DBGNOID, " VAL_VOI"); break;
			default: dbg_prt(DBGNOID, " ????"); break;
			}
#endif
		retval |= vp->val_typ;

		if ((vp->val_typ & types) == 0)
		{
			switch (vp->val_typ)
			{
			case VAL_COD:
				error("Constant not legal in this context");
				break;
			
			case VAL_STR:
				error("String not legal in this context");
				break;

			case VAL_SAM:
				error("SAME not legal in this context");
				break;

			case VAL_VOI:
				error("VOID not legal in this context");
				break;

			default:
				bug("val_chk1");
			}
		}
	}

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", retval);
#endif

	return retval;
}

/*
 * def_make -- make a pseudo value for VOID and SAME
 */
val *
def_make(of)
int of;
{
	val *vp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "def_make(%x)", of);
#endif

	vp = val_new();

	if (of == VAL_VOI || of == VAL_SAM)
		vp->val_typ = of;
	else
		bug("def_make1");

#ifdef EBUG
	if (fct_dbg)
	{
		val_dmp(vp);
		dbg_prt(DBGOUT, "-> %x", vp);
	}
#endif

	return vp;
}

/*
 * valtocod -- try to coerce the value given into a single code
 *	       returns VAL_COD value.
 *
 * WARNING:
 *	for the time beeing this function is restricted to concatenating
 *	at most two values of type VAL_COD
 *
 */
val *
valtocod(value, dup)
val *value;
int dup;		/* if not zero -- duplicate value even if simple. */
{
	val *vp;

#ifdef EBUG
	if (fct_dbg)
	{
		dbg_prt(DBGIN, "valtocod(%x, %d)", value, dup);
		val_dmp(value);
	}
#endif

	/*
	 * if no value make up one
	 */
	if (value == (val *)0)
	{
		vp = val_new();
		vp->val_cod = cod_new();
		vp->val_len = 0;
		vp->val_typ = VAL_COD;
		vp->val_cod->cod_rep = (i_char)-1;
#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", vp);
#endif
		return vp;
	}

	if (value->val_nxt == (val *)0)
	{
		if (value->val_typ != VAL_COD)
			bug("valtocod: not VAL_COD");

		if (dup)
		{
			/*
			 * get room for new value
			 */
			vp = val_new();

			vp->val_cod = cod_new();
			vp->val_typ = value->val_typ;
			vp->val_len = value->val_len;
			vp->val_cod->cod_rep = value->val_cod->cod_rep;
			vp->val_cod->cod_prp = value->val_cod->cod_prp;
			value = vp;
		}

#ifdef EBUG
		if (fct_dbg)
		{
			val_dmp(value);
			dbg_prt(DBGOUT, "-> %x", value);
		}
#endif
		return value;
	}
	else if (value->val_nxt->val_nxt != (val *)0)
	{
		warning("very long code -- truncated");
		val_del(value->val_nxt->val_nxt);
		value->val_nxt->val_nxt = (val *)0;
	}

	vp = valtocod(value->val_nxt, dup);
	value->val_nxt = (val *)0;

	/*
	 * combine the code.
	 */
	value->val_len += vp->val_len;
	value->val_cod->cod_rep = ((value->val_cod->cod_rep << (vp->val_len * 8))
				  | vp->val_cod->cod_rep);
	/*
	 * new character has no properties as of yet
	 */
	value->val_cod->cod_prp = 0;

	if (value->val_len > sizeof(i_char))
		error("combined code is too long (%d bytes)", value->val_len);

	/*
	 * free second part of value as it is no longer needed
	 */
	free((char *)vp->val_cod);
	free((char *)vp);

#ifdef EBUG
	if (fct_dbg)
	{
		val_dmp(value);
		dbg_prt(DBGOUT, "-> %x", value);
	}
#endif

	return value;
}

/*
 * valtolist -- value to ichar value list
 */
val *
valtolist(value)
val *value;			/* pointer to value to convert		   */
{
	val *strtolist();

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "valtolist(%x)", value);
#endif

	if (value == (val *)0)
		bug("valtolist1");

	/*
	 * recursivly call ourself to get to the last value in the value chain
	 */
	if (value->val_nxt != (val *)0)
		value->val_nxt = valtolist(value->val_nxt);

	/*
	 * we are on our way back, convert the type we find to i_char list
	 */
	switch (value->val_typ)
	{
	case VAL_COD:
		/*
		 * nothing to do -- code was given as constant
		 */
		break;

	case VAL_STR:
		/*
		 * convert the string to an ichar list
		 */
		value = strtolist(value);
		break;

	default:
		bug("valtolist4");
		break;
	}

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", value);
#endif

	return value;
}

/*
 * strtolist -- convert string value to ichar value list
 */
val *
strtolist(value)
val *value;
{
	register char *cp;
	val *vp;
	val *tvp;
	val *val_anc = (val *)0;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "strtolist(%x)", value);
#endif

	if (value->val_typ != VAL_STR)
		bug("strtolist1");

	/*
	 * special measure to handle null strings
	 */
	tvp = value;

	for (cp = value->val_str; *cp; cp++)
	{
		vp = val_new();

		vp->val_typ = VAL_COD;
		vp->val_cod = cod_new();
		vp->val_cod->cod_rep = (i_char)*cp;
		vp->val_len = 1;

		/*
		 * insert the found value into the value list
		 */
		if (val_anc == (val *)0)
			val_anc = vp;
		else
			tvp->val_nxt = vp;

		tvp = vp;
	}

	/*
	 * make sure the value chain remains unbroken
	 */
	tvp->val_nxt = value->val_nxt;

	/*
	 * free no longer used space
	 */
	free(value->val_str);
	free((char *)value);
	value = val_anc;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", value);
#endif

	return value;
}

/*
 * val_del -- delete a value
 */
void
val_del(value)
val *value;
{
	register val *vp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "val_del(%x)", value);
#endif

	while (value)
	{
		vp = value->val_nxt;
		if (value->val_typ == VAL_COD || value->val_typ == VAL_STR)
		{
			if (value->val_str != (char *)0)
			{
				free(value->val_str);
				value->val_str = (char *)0;
			}
		}
		else if ( value->val_typ != VAL_VOI && value->val_typ != VAL_SAM)
			bug("val_del1");

		value->val_typ = 0;
		value->val_len = 0;
		value->val_nxt = (val *)0;

		free((char *)value);
		value = vp;
	}
}

/*
 * val_len -- evaluate length of a value
 */
int
val_len(value)
register val *value;
{
	register int len;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "val_len(%x)", value);
#endif

	for (len = 0; value; value = value->val_nxt)
		len += value->val_len;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %d", len);
#endif

	return len;
}

/*
 * chrtoval -- i_char to value conversion
 */
val *
chrtoval(c)
i_char c;
{
	cod *idxtocod();
	int len;
	cod *tip;
	cod *cp;
	val *vp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "chartoval(%x)", c);
#endif

	vp = val_new();
	cp = cod_new();

	if ((tip = idxtocod(c, &len)) != (cod *)0)
	{
		cp->cod_rep = tip->cod_rep;
		cp->cod_prp = tip->cod_prp;
		vp->val_len = len;
	}
	else
	{
		cp->cod_rep = c;
		cp->cod_prp = I_MISCEL;
		/*
		 * ATTENTION: The next statement assumes sizeof(bit16) == 2 !
		 */
		vp->val_len = (c > 0xff) ? 2 : 1;
	}

	vp->val_typ = VAL_COD;
	vp->val_cod = cp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", vp);
#endif

	return vp;
}

/*
 * idxtocod -- return the code with index idx
 */
cod *
idxtocod(idx, len)
i_char idx;
int *len;
{
	register sym *sp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGIN, "idxtocod(%x, %x)", idx, len);
#endif

	sp = codeset[idx];
	*len = sp->sym_val->val_len;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGOUT, "-> %x", sp->sym_val->val_cod);
#endif

	return sp->sym_val->val_cod;
}

/*
 * val_new -- allocate memory for a value
 */
static val *
val_new()
{
	register val *vp;

	if ((vp = new(val, 1)) == (val *)0)
		fatal("no room for value");
	return vp;
}

/*
 * cod_new -- allocate memory for a code
 */
static cod *
cod_new()
{
	register cod *cp;

	if ((cp = new(cod, 1)) == (cod *)0)
		fatal("no room for code");
	return cp;
}
