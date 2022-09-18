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

%{
#ifndef lint
static char ySccsid[] = "@(#)ic.y	4.1	(ULTRIX)	7/17/90";
#endif

#include "ic.h"
%}

/*
 * typedefinition for yacc value stack
 */
%union {
	sym		*y_sym;	/* Identifier */
	val		*y_val;	/* value: Constant or String or Identifier */
	i_char		 y_prp; /* property word */
}

/*
 * Terminal symbols
 */
%token ','
%token '-'
%token '/'
%token ':'
%token ';'
%token '='
%token CODESET
%token COLLATION
%token CONVERSION
%token DEFAULT
%token END
%token EXTENDED
%token STRINGTABLE
%token IS
%token PRIMARY
%token REST
%token SAME
%token EQUAL
%token VOID
%token CODE
%token STRING
%token PROPERTY
%token <y_sym>	Identifier
%token <y_val>	Constant
%token <y_val>	String
%token <y_prp>	Property

/*
 * typed non terminal symbols
 */
%type <y_sym>	code_set_intro code_def_intro code_definition
%type <y_sym>	code_definition_list property_definition
%type <y_sym>	format_list format conversion_head
%type <y_val>	format_value code code_or_string code_value
%type <y_val>	default_value conversion_value
%type <y_prp>	property_list

%%

/*
 * definition of the International UNIX data base
 */
intl_data_base
	:	{ ic_init(); prp_init(sym_fake(PRP_DEF, SYM_UDF)); }
	  codeset_table data_tables
		{ i_end(); }
	;

data_tables
	: data_table
	| data_tables data_table
		{ yyerrok; }
	| error
	| data_tables error
	;

data_table
	: property_table
	| collation_table
	| format_table
	| conversion_table
	;
/*
 * Definition of the codeset table
 */
codeset_table
	: code_set_intro code_definition_list end
		{ cod_set($1, $2); yyerrok; }
	| error
		{ fatal("Must have a codeset"); }
	;

code_set_intro
	: CODESET Identifier ':'
		{ sym_set(cod_anc = $2, SYM_COD); $$ = cod_anc; }
	| EXTENDED CODESET Identifier ':'
		{ sym_set(cod_anc = $3, SYM_COD); $$ = cod_anc;
		  i_hdr.i_flags |= I_16;
		}
	| error Identifier ':'
		{ sym_set(cod_anc = $2, SYM_COD); $$ = cod_anc; }
	| CODESET Identifier error
		{ sym_set(cod_anc = $2, SYM_COD); $$ = cod_anc; }
	| CODESET error ':'
		{ cod_anc = sym_fake("CODESET", SYM_COD); }
	;

end
	: END '.'
	| ';' END '.'
	| error '.'
	| END error
	| ';' error '.'
	| ';' END error
	;

code_definition_list
	: code_definition
		{ $$ = cod_add($1, (sym *)0); }
	| code_definition_list ';' code_definition
		{ $$ = cod_add($3, $1); yyerrok; }
	| error { $$ = (sym *)0; }
	| code_definition_list error code_definition
		{ $$ = cod_add($3, $1); yyerrok; }
	| code_definition_list ';' error
	;

code_definition
	: code_def_intro ':' property_list
		{ prp_set($1, $3); }
	| code_def_intro
		{ prp_set($1, I_ILLEGAL); }
	| property_definition
	;

code_def_intro
	: Identifier '='
		{ sym_set($1, SYM_CDF); }
	  code_value
		{ $$ = cod_make($1, $4); }
	;

code_value
	: code
		{ $$ = val_add($1, (val *)0); }
	| code_value ',' code
		{ $$ = val_add($3, $1); yyerrok; }
	| error { $$ = (val *)0; }
	| code_value error code
		{ $$ = val_add($3, $1); yyerrok; }
	| code_value ',' error
	;

code
	: Constant
	| Identifier
		{ $$ = var_make($1); }
	;

property_list
	: Property
		{ $$ = prp_add($1, I_ILLEGAL); yyerrok; }
	| property_list ',' Property
		{ $$ = prp_add($3, $1); yyerrok; }
	| error { $$ = I_ILLEGAL; }
	| property_list error Property
		{ $$ = prp_add($3, $1); yyerrok; }
	| property_list ',' error
	;

/*
 * Definition of property tables
 */
property_table
	: PROPERTY Identifier ':'
		{ prp_init($2); }
	  property_definition_list end 
		{ prp_end($2); }
	;

property_definition_list
	: property_definition
	| property_definition_list ';' property_definition
		{ yyerrok; }
	| error
	| property_definition_list error property_definition
	| property_definition_list ';' error
	;

property_definition
	: Identifier ':' property_list
		{ prp_set($1, $3); $$ = (sym *)0; }
	;

/*
 * Definition of collation sequences
 */
collation_table
	: COLLATION Identifier
		{ col_init($2); }
	  ':' collation_list end
		{ col_end($2); }
	| COLLATION ':'
		{ col_init($<y_sym>$ = sym_fake(COL_DEF, SYM_UDF)); }
	  collation_list end
		{ col_end($<y_sym>3); }
	;

collation_list
	: collation
	| collation_list ';' collation
		{ yyerrok; }
	| error
	| collation_list error collation
	| collation_list ';' error
	;

collation
	: collation_head code_value_list
	| collation_head Identifier '-' Identifier
		{ col_range($2, $4, prm_wgt, equal); }
	| collation_head REST
		{ col_rest(prm_wgt); }
	| Identifier '=' '('  Identifier ',' Identifier ')'
		{ col_dipht($1, $4, $6); }
	| PROPERTY ':' Identifier
		{ col_prp($3); }
	;

collation_head
	: PRIMARY ':'
		{ equal = 0; prm_wgt++; sec_wgt = 0; }
	|
	  EQUAL ':'
		{ equal = 1; prm_wgt++; sec_wgt = 0; }
	;

code_value_list
	: Identifier
		{ col_set($1, prm_wgt, equal ? 1 : ++sec_wgt); }
	| code_value_list ',' Identifier
		{ col_set($3, prm_wgt, equal ? 1 : ++sec_wgt); yyerrok; }
	| error
	| code_value_list error
	| code_value_list error Identifier
		{ col_set($3, prm_wgt, equal ? 1 : ++sec_wgt); yyerrok; }
	| code_value_list ',' error
	;

/*
 * Definition of the format tables
 */
format_table
	: STRINGTABLE Identifier
		{ frm_init($2); }
	  ':' format_list end
		{ frm_end($2, $5); yyerrok; }
	| STRINGTABLE ':'
		{ frm_init($<y_sym>$ = sym_fake(FRM_DEF, SYM_UDF)); }
	  format_list end
		{ frm_end($<y_sym>3, $4); yyerrok; }
	;

format_list
	: format
		{ $$ = frm_add($1, (sym *)0); }
	| format_list ';' format
		{ $$ = frm_add($3, $1); yyerrok; }
	| error { $$ = (sym *)0; }
	| format_list error format
		{ $$ = frm_add($3, $1); yyerrok; }
	| format_list ';' error
	;

format
	: Identifier
		{ sym_set($1, SYM_FDF); }
	  '=' format_value
		{ $$ = frm_set($1, $4); }
	;

format_value
	: code_or_string
		{ $$ = val_add($1, (val *)0); }
	| format_value ',' code_or_string
		{ $$ = val_add($3, $1); yyerrok; }
	| error { $$ = (val *)0; }
	| format_value error code_or_string
		{ $$ = val_add($3, $1); yyerrok; }
	| format_value ',' error
	;

code_or_string
	: code
	| String
	;

/*
 * Definition of a conversion table:
 */
conversion_table
	: conversion_head conversion_list end
		{ cnv_end($1); yyerrok; }
	;

conversion_head
	: CONVERSION Identifier ':'
		{ $$ = cnv_init($2, CNV_COD); }
	| CODE CONVERSION Identifier ':'
		{ $$ = cnv_init($3, CNV_COD); }
	| STRING CONVERSION Identifier ':'
		{ $$ = cnv_init($3, CNV_STR); }
	;

conversion_list
	: conversion
	| conversion_list ';' conversion
	| error
	| conversion_list error conversion
	| conversion_list ';' error
	;

conversion
	: DEFAULT IS default_value
		{ def_set($3); }
	| Identifier IS conversion_value
		{ cnv_set($1, $3); }
	| Identifier '-' Identifier IS Identifier '-' Identifier
		{ cnv_range($1, $3, $5, $7); }
	;

default_value
	: VOID { $$ = def_make(VAL_VOI); }
	| SAME { $$ = def_make(VAL_SAM); }
	| conversion_value
	;

conversion_value
	: code_or_string
		{ $$ = val_add($1, (val *)0); }
	| conversion_value ',' code_or_string
		{ $$ = val_add($3, $1); yyerrok; }
	| error { $$ = (val *)0; }
	| conversion_value error code_or_string
		{ $$ = val_add($3, $1); yyerrok; }
	| conversion_value ',' error
	;
