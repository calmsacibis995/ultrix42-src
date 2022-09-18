%{
#ifndef lint
static char lSccsid[] = "@(#)ic.l	4.1	(ULTRIX)	7/17/90";
#endif

/*
 * ic -- lexical analysis
 */

#include "ic.h"
#include "y.tab.h"
#define LAST(v)	(v - 1 + sizeof v / sizeof v[0])

%}

letter		[a-zA-Z_]
digit		[0-9]
letter_or_digit	[a-zA-Z_0-9]
let_or_dig_or_x	[a-zA-Z_0-9.]
hexdigit	[a-fA-F0-9]
white_space	[ \t\n]
blank		[ \t]
other		.

%%

^"#"{blank}*{digit}+({blank}+.*)?\n	yymark();
{letter}{letter_or_digit}*		return screen();
{letter}{let_or_dig_or_x}*{letter_or_digit}	return screen();
{digit}+				return sym_lookup(Constant);
{digit}+[/x]{hexdigit}*			return sym_lookup(Constant);
'([^\\'\n]|\\[\\'ntbrf0])*'		return sym_lookup(Constant);
\"([^"\n]|\\[\\"ntbrf0])*\"		return sym_lookup(String);
"->"					return sym_lookup(IS);
{white_space}+				;
{other}					{
#ifdef EBUG
					  if (lex_dbg)
						dbg_prt(DBGTIN, "lex -> %c",
							yytext[0]);
#endif
					  return yytext[0];
					}

%%

typedef struct rwtable {		/* reserved words table */
	char *rw_name;			/* representation */
	int   rw_yylex;			/* yylex value */
} rwtable;

rwtable rwtab[] = {			/* sorted table of reserved words */
	"ARITH",	Property,	/* arithmetic sign 	*/
	"BLANK",	Property,	/* blank character	*/
	"CODE",		CODE,		/* KEYWORD 	  	*/
	"CODESET",	CODESET,	/* KEYWORD		*/
	"COLLATION",	COLLATION,	/* KEYWORD		*/
	"CONVERSION",	CONVERSION,	/* KEYWORD		*/
	"CTRL",		Property,	/* control character	*/
	"CURENCY",	Property,	/* currency character	*/
	"DEFAULT",	DEFAULT,	/* KEYWORD		*/
	"DIACRIT",	Property,	/* diacritical sign	*/
	"DIPHTONG",	Property,	/* diphtong		*/
	"DOUBLE",	Property,	/* double letter	*/
	"END",		END,		/* KEYWORD		*/
	"EQUAL",	EQUAL,		/* KEYWORD		*/
	"EXTENDED",	EXTENDED,	/* KEYWORD		*/
	"FRACTION",	Property,	/* fraction character	*/
	"HEX",		Property,	/* hex character	*/
	"ILLEGAL",	Property,	/* illegal character	*/
	"LOWER",	Property,	/* lowercase letter	*/
	"MISCEL",	Property,	/* miscellaneous symbol */
	"NUMERAL",	Property,	/* numeral character	*/
	"PRIMARY",	PRIMARY,	/* KEYWORD		*/
	"PROPERTY",	PROPERTY,	/* KEYWORD		*/
	"PUNCT",	Property,	/* punctuation char	*/
	"REST",		REST,		/* KEYWORD		*/
	"SAME",		SAME,		/* KEYWORD		*/
	"SPACE",	Property,	/* space character	*/
	"STRING",	STRING,		/* intro for string conversion   */
	"STRINGTABLE",	STRINGTABLE,	/* KEYWORD		*/
	"SUPSUB",	Property,	/* super-/sub scripts	*/
	"UPPER",	Property,	/* upper case letter	*/
	"VOID",		VOID,		/* KEYWORD		*/

};

/*
 * screen() -- processor for reserved words
 */

static int
screen()
{
	rwtable *low = rwtab;
	rwtable *high = LAST(rwtab);
	rwtable *mid;
	int c;

	while (low <= high)
	{
		mid = low + (high - low) / 2;
		if ((c = strcmp(mid->rw_name, yytext)) == 0)
		{
			/*
			 * here a keyword or a property is returned
			 */
			if (mid->rw_yylex == Property)
				return sym_lookup(Property);
#ifdef EBUG
			if (lex_dbg)
				dbg_prt(DBGTIN, "lex: %s -> %x", yytext,
					mid->rw_yylex);
#endif
			return mid->rw_yylex;
		}
		else if (c < 0)
			low = mid + 1;
		else
			high = mid - 1;
	}

	/*
	 * not found in table of reserved words. must be Identifier
	 */
	return sym_lookup(Identifier);
}
