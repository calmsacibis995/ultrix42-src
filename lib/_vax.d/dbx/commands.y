/*#@(#)commands.y	4.2	Ultrix	11/9/90*/
%{

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 *			Modification History				*
 *									*
 *  005	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	004 - Report an error when the user attempts to do anything	*
 *	      other than set/unset on debugger defined  variables.      *
 *	      This is a temporary fix to the problem.			*
 *	      (vjh, August 1, 1986)					*
 *									*
 *	003 - Merged in 4.3 changes.					*
 *	      (vjh, April 29, 1986)					*
 *									*
 *	002 - Update copyright.						*
 *	      (vjh, August 23, 1985)					*
 *									*
 *	001 - Added code to check for valid signals on CONT command.	*
 *	      Added 'delete *'.
 *	      (Victoria Holt, June 14, 1985)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)commands.y	5.3 (Berkeley) 5/31/85
 */

static char rcsid[] = "$Header: commands.y,v 1.5 84/12/26 10:38:41 linton Exp $";

/*
 * Yacc grammar for debugger commands.
 */

#include "defs.h"
#include "symbols.h"
#include "operators.h"
#include "tree.h"
#include "process.h"
#include "source.h"
#include "scanner.h"
#include "keywords.h"
#include "names.h"
#include "lists.h"
#include "history.h"

private String curformat = "X";

%}

%term
    ALIAS AND ASSIGN AT CALL CALLV CATCH CD CONT
	DEBUG DELETE DIV DOWN DUMP
    EDIT FILE F_MASK FUNC GRIPE GETENV HELP HISTORY IF IGNORE IN LIST
	MOD NEXT NEXTI NEXTV NIL NOT OR 
    PRINT PRINTF PSYM PWD QUIT 
	RECORD REREAD RERUN RETURN RUN SET SETENV SH SKIP
	SOURCE STATUS STEP STEPI STEPV STOP STOPI 
	T_MASK TRACE TRACEI UNALIAS
	UNSET UP USE 
    WHATIS WHEN WHERE WHEREIS WHICH

%term INT CHAR REAL NAME STRING
%term ARROW 

%right INT
%binary REDIRECT
%binary '<' '=' '>' '!' IN
%left '+' '-' OR
%left UNARYSIGN
%left '*' '/' DIV MOD AND
%left '\\'
%left NOT '(' '[' '.' '^' ARROW

%union {
    Name y_name;
    Symbol y_sym;
    Node y_node;
    Integer y_int;
    Operator y_op;
    long y_long;
    char y_char;
    double y_real;
    String y_string;
    Boolean y_bool;
    Cmdlist y_cmdlist;
    List y_list;
};

%type <y_op>	    trace stop
%type <y_long>	    INT count signal
%type <y_char>	    CHAR
%type <y_real>	    REAL
%type <y_string>    STRING redirectout filename opt_filename mode
%type <y_name>	    ALIAS AND ASSIGN AT CALL CALLV CD CATCH CONT
%type <y_name>	    DEBUG DELETE DIV DOWN DUMP
%type <y_name>	    EDIT FILE F_MASK FUNC GRIPE GETENV
%type <y_name>	    HELP HISTORY IF IGNORE IN LIST MOD
%type <y_name>	    NEXT NEXTI NEXTV NIL NOT OR 
%type <y_name>	    PRINT PRINTF PSYM PWD QUIT
%type <y_name>	    RECORD REREAD RERUN RETURN RUN
%type <y_name>	    SET SETENV SH SKIP SOURCE STATUS
%type <y_name>	    STEP STEPI STEPV STOP STOPI T_MASK TRACE TRACEI
%type <y_name>	    UNALIAS UNSET UP USE WHATIS WHEN
%type <y_name>	    WHERE WHEREIS WHICH
%type <y_name>	    name NAME keyword
%type <y_node>      opt_qual_symbol symbol module_symbol module
%type <y_node>	    command rcommand cmd step what where examine
%type <y_node>	    event opt_exp_list opt_cond env_command
%type <y_node>	    exp_list exp term boolean_exp constant address
%type <y_node>	    integer_list alias_command list_command line_number
%type <y_cmdlist>   actions
%type <y_list>      sourcepath name_list

%%

input:
    input command_nl
|
    /* empty */
;
command_nl:
    command_line '\n'
|
    command_line ';'
{
	chkalias = true;
}
|
    '!'  INT '\n'
{
	Iline str;

	del_Iline(0);
	historyevent--;
	str = strdup(get_Iline(historyevent - $2));
	if (str != NULL && strlen(str) > 0) {
	    Iline_alloc(str);
	    insertinput(str);
	    str[strlen(str)-1] = '\0';   /* strip off newline */
	    printf("(!%d = %s)\n", $2, str);
		chkalias = true;
	} else {
		error("Event not found.");
	}
}
|
    '!' '-' INT '\n'
{
	Iline str;

	del_Iline(0);
	historyevent--;
	str = strdup(get_Iline($3-1));
	if (str != NULL && strlen(str) > 0) {
	    Iline_alloc(str);
	    insertinput(str);
	    str[strlen(str)-1] = '\0';   /* strip off newline */
	    printf("(!-%d = %s)\n", $3, str);
		chkalias = true;
	} else {
		error("Event not found.");
	}
}
|
    '!' name '\n'
{
	char *str;

	del_Iline(0);
	historyevent--;
	str = strdup(srch_Iline(ident($2)));
	if (str != NULL && strlen(str) > 0) {
	    Iline_alloc(str);
	    insertinput(str);
	    str[strlen(str)-1] = '\0';   /* strip off newline */
	    printf("(!%s = %s)\n", ident($2), str);
		chkalias = true;
	} else {
		error("Event not found.");
	}
}
|
    '\n'
;

command_line:
    command
{
	if ($1 != nil) {
	    topeval($1);
	}
}
|
    rcommand redirectout
{
	if ($1 != nil) {
	    if ($2 != nil) {
		setout($2);
		topeval($1);
		unsetout();
	    } else {
		topeval($1);
	    }
	}
}
;
redirectout:
    '>' shellmode NAME
{
	$$ = ident($3);
}
|
    /* empty */
{
	$$ = nil;
}
;

/*
 * Non-redirectable commands.
 */
command:
    alias_command
{
	$$ = $1;
}
|
    ASSIGN exp '=' exp
{
	$$ = build(O_ASSIGN, unrval($2), $4);
}
|
    CATCH signal
{
	$$ = build(O_CATCH, $2);
}
|
    CATCH
{
	$$ = build(O_CATCH, 0);
}
|
    CONT
{
	$$ = build(O_CONT, (long) DEFSIG);
}
|
    CONT signal
{
	$$ = build(O_CONT, $2);
}
|
    DELETE '*'
{
	$$ = build(O_DELETE, nil);
}
|
    DELETE integer_list
{
	$$ = build(O_DELETE, $2);
}
|
    DOWN
{
	$$ = build(O_DOWN, build(O_LCON, (long) 1));
}
|
    DOWN INT
{
	$$ = build(O_DOWN, build(O_LCON, (long) $2));
}
|
    EDIT shellmode opt_filename
{
	$$ = build(O_EDIT, $3);
}
|
    RECORD  shellmode opt_filename
{
	$$ = build(O_RECORD, $3);
}
|
    env_command
{
	$$ = $1;
}
|
    FILE shellmode opt_filename
{
	$$ = build(O_CHFILE, $3);
}
|
    FUNC
{
	$$ = build(O_FUNC, nil);
}
|
    FUNC module_symbol
{
	$$ = build(O_FUNC, $2);
}
|
    GRIPE
{
	$$ = build(O_GRIPE);
}
|
    HELP
{
	$$ = build(O_HELP);
}
|
    HISTORY
{
	dump_Iline();
	$$ = nil;
}
|
    IGNORE signal
{
	$$ = build(O_IGNORE, $2);
}
|
    IGNORE
{
	$$ = build(O_IGNORE, 0);
}
|
    list_command
{
	$$ = $1;
}
|
    PWD
{
	$$ = build(O_PWD);
}
|
    PSYM exp
{
	$$ = build(O_PSYM, unrval($2));
}
|
    QUIT
{
	if (not popinput()) {
	    quit(0);
	} else {
	    $$ = nil;
	}
}
|
    RETURN
{
	$$ = build(O_RETURN, nil);
}
|
    RETURN opt_qual_symbol
{
	$$ = build(O_RETURN, $2);
}
|
    runcommand
{
	run();
	/* NOTREACHED */
}
|
    SET name '=' exp
{
	$$ = build(O_SET, build(O_NAME, $2), $4);
}
|
    SET name
{
	$$ = build(O_SET, build(O_NAME, $2), nil);
}
|
    SET
{
	$$ = build(O_SET, nil, nil);
}
|
    SH
{
	shellline();
	$$ = nil;
}
|
    SOURCE shellmode filename
{
	$$ = build(O_SOURCE, $3);
}
|
    step
{
	$$ = $1;
}
|
    stop where opt_cond
{
	$$ = build($1, nil, $2, $3);
}
|
    stop what opt_cond
{
	$$ = build($1, $2, nil, $3);
}
|
    stop IF boolean_exp
{
	$$ = build($1, nil, nil, $3);
}
|
    trace what where opt_cond
{
	$$ = build($1, $2, $3, $4);
}
|
    trace where opt_cond
{
	$$ = build($1, nil, $2, $3);
}
|
    trace what opt_cond
{
	$$ = build($1, $2, nil, $3);
}
|
    trace opt_cond
{
	$$ = build($1, nil, nil, $2);
}
|
    UNALIAS name
{
	$$ = build(O_UNALIAS, build(O_NAME, $2));
}
|
    UNSET name
{
	$$ = build(O_UNSET, build(O_NAME, $2));
}
|
    UP
{
	$$ = build(O_UP, build(O_LCON, (long) 1));
}
|
    UP INT
{
	$$ = build(O_UP, build(O_LCON, (long) $2));
}
|
    USE shellmode sourcepath
{
	String dir;

	$$ = nil;
	if (list_size($3) == 0) {
	    foreach (String, dir, sourcepath)
		printf("%s ", dir);
	    endfor
	    printf("\n");
	} else {
	    foreach (String, dir, sourcepath)
		list_delete(list_curitem(sourcepath), sourcepath);
	    endfor
	    sourcepath = $3;
	}
}
|
    WHATIS opt_qual_symbol
{
	$$ = build(O_WHATIS, $2);
}
|
    WHEN event '{' actions '}'
{
	$$ = build(O_ADDEVENT, $2, $4);
}
|
    WHEREIS name
{
	$$ = build(O_WHEREIS, build(O_SYM, lookup($2)));
}
|
    WHICH symbol
{
	$$ = build(O_WHICH, $2);
}
|
    '/'
{
	$$ = build(O_SEARCH,
	    build(O_LCON, (long) '/'),
	    build(O_SCON, strdup(scanner_linebuf))
	);
	gobble();
	insertinput("\n");
}
|
    '?'
{
	$$ = build(O_SEARCH,
	    build(O_LCON, (long) '?'),
	    build(O_SCON, strdup(scanner_linebuf))
	);
	gobble();
	insertinput("\n");
}
;
signal:
    INT
{
	$$ = $1;
}
|
    name
{
	$$ = siglookup(ident($1));
}
;
runcommand:
    run arglist
|
    run
;
run:
    RUN shellmode
{
	arginit();
	fflush(stdout);
}
|
    RERUN shellmode
{
	fflush(stdout);
}
|
    REREAD shellmode
{
	forcereread();
	fflush(stdout);
}
;
arglist:
    arglist arg
|
    arg
;
arg:
    NAME
{
	newarg(ident($1));
}
|
    STRING
{
	newarg($1);
}
|
    '<' NAME
{
	inarg(ident($2));
}
|
    '>' NAME
{
	outarg(ident($2));
}
;
step:
    STEP
{
	$$ = build(O_STEP, true, false);
}
|
    STEPI
{
	$$ = build(O_STEP, false, false);
}
|
    STEPV
{
	$$ = build(O_STEPV, false, false);
}
|
    NEXT
{
	$$ = build(O_STEP, true, true);
}
|
    NEXTI
{
	$$ = build(O_STEP, false, true);
}
|
    NEXTV
{
	$$ = build(O_STEPV, false, true);
}
;
shellmode:
    /* empty */
{
	beginshellmode();
}
;
sourcepath:
    sourcepath NAME
{
	$$ = $1;
	list_append(list_item(ident($2)), nil, $$);
}
|
    /* empty */
{
	$$ = list_alloc();
}
;
event:
    where
|
    exp
;
actions:
    actions cmd ';'
{
	$$ = $1;
	cmdlist_append($2, $$);
}
|
    cmd ';'
{
	$$ = list_alloc();
	cmdlist_append($1, $$);
}
;
cmd:
    command
|
    rcommand
;

/*
 * Redirectable commands.
 */
rcommand:
    F_MASK exp_list
{
	$$ = build(O_FMASK, $2, nil);
}
|
    F_MASK '/' constant exp_list
{
	$$ = build(O_FMASK, $4, $3);
}
|
    T_MASK exp_list
{
	$$ = build(O_TMASK, $2, nil);
}
|
    T_MASK '/' constant exp_list
{
	$$ = build(O_TMASK, $4, $3);
}
|
    PRINT exp_list
{
	$$ = build(O_PRINT, $2);
}
|
    PRINTF opt_exp_list
{
	$$ = build(O_PRINTF, $2, findlanguage(C));
}
|
    WHERE
{
	$$ = build(O_WHERE);
}
|
    examine
{
	$$ = $1;
}
|
    CALL term  
{
	$$ = build(O_CALLPROC, $2, nil, nil);
}
|
    CALL term  '(' opt_exp_list ')'
{
	$$ = build(O_CALLPROC, $2, $4, nil);
}
|
    CALLV term  
{
	$$ = build(O_CALLPROC, $2, nil, $2);
}
|
    CALLV term  '(' opt_exp_list ')'
{
	$$ = build(O_CALLPROC, $2, $4, $2);
}
|
    DEBUG INT
{
 	$$ = build(O_DEBUG, $2);
}
|
    DEBUG '-' INT
{
	$$ = build(O_DEBUG, -$3);
}
|
    DUMP opt_qual_symbol
{
	$$ = build(O_DUMP, $2);
}
|
    DUMP '.'
{
	$$ = build(O_DUMP, nil);
}
|
    DUMP
{
	$$ = build(O_DUMP, build(O_SYM, curfunc));
}
|
    STATUS
{
	$$ = build(O_STATUS);
}
;
alias_command:
    ALIAS name name
{
	$$ = build(O_ALIAS, build(O_NAME, $2), build(O_NAME, $3));
}
|
    ALIAS name STRING
{
	$$ = build(O_ALIAS, build(O_NAME, $2), build(O_SCON, $3));
}
|
    ALIAS name '(' name_list ')' STRING
{
	$$ = build(O_ALIAS,
	    build(O_COMMA, build(O_NAME, $2), (Node) $4),
	    build(O_SCON, $6)
	);
}
|
    ALIAS name
{
	$$ = build(O_ALIAS, build(O_NAME, $2), nil);
}
|
    ALIAS
{
	$$ = build(O_ALIAS, nil, nil);
}
;
name_list:
    name_list ',' name
{
	$$ = $1;
	list_append(list_item($3), nil, $$);
}
|
    name
{
	$$ = list_alloc();
	list_append(list_item($1), nil, $$);
}
;
trace:
    TRACE
{
	$$ = O_TRACE;
}
|
    TRACEI
{
	$$ = O_TRACEI;
}
;
stop:
    STOP
{
	$$ = O_STOP;
}
|
    STOPI
{
	$$ = O_STOPI;
}
;
what:
    exp
{
	$$ = $1;
}
|
    STRING ':' line_number
{
	$$ = build(O_QLINE, build(O_SCON, $1), $3);
}
;
where:
    IN exp
{
	$$ = unrval($2);
}
|
    AT line_number
{
	$$ = build(O_QLINE, build(O_SCON, strdup(cursource)), $2);
}
|
    AT STRING ':' line_number
{
	$$ = build(O_QLINE, build(O_SCON, $2), $4);
}
;
filename:
    NAME
{
	$$ = ident($1);
}
;
opt_filename:
    /* empty */
{
	$$ = nil;
}
|
    filename
{
	$$ = $1;
}
;
opt_exp_list:
    exp_list
{
	$$ = $1;
}
|
    /* empty */
{
	$$ = nil;
}
;
env_command:
	SETENV name name 
{
	$$ = build(O_SETENV, build(O_NAME ,$2), build(O_NAME , $3));
}
|
	SETENV name STRING 
{
	$$ = build(O_SETENV, build(O_NAME ,$2), build(O_SCON , $3));
}
|
	GETENV name 
{
	$$ = build(O_GETENV, build(O_NAME ,$2));
}
|
    CD shellmode opt_filename
{
	$$ = build(O_CD, $3);
}
;
list_command:
    LIST
{
	$$ = build(O_LIST,
	    build(O_LCON, (long) cursrcline),
	    build(O_LCON, (long) cursrcline + 9)
	);
}
|
    LIST line_number
{
	$$ = build(O_LIST, $2,
			build (O_ADD, $2, build(O_LCON, 9)));
}
|
    LIST line_number ',' line_number
{
	$$ = build(O_LIST, $2, $4);
}
|
    LIST exp ':' exp
{
	$$ = build(O_LIST, $2, build (O_ADD, $2, build(O_SUB, $4, build(O_LCON,
		1))));
}
|
    LIST module_symbol
{
	$$ = build(O_LIST, $2);
}
;
integer_list:
    INT
{
	$$ = build(O_LCON, $1);
}
|
    INT integer_list
{
	$$ = build(O_COMMA, build(O_LCON, $1), $2);
}
|
    INT ',' integer_list
{
	$$ = build(O_COMMA, build(O_LCON, $1), $3);
}
;
line_number:
    INT
{
	$$ = build(O_LCON, $1);
}
|
    '$'
{
	$$ = build(O_LCON, (long) LASTLINE);
}
;
examine:
    where '/' count mode
{
	$$ = build(O_EXAMINE, $4, $1, nil, $3);
}
|
    address '/' count mode
{
	$$ = build(O_EXAMINE, $4, $1, nil, $3);
}
|
    address ',' address '/' mode
{
	$$ = build(O_EXAMINE, $5, $1, $3, 0);
}
|
    address '=' mode
{
	$$ = build(O_EXAMINE, $3, $1, nil, 0);
}
;
address:
    INT
{
	$$ = build(O_LCON, $1);
}
|
    '.'
{
	$$ = build(O_LCON, (long) prtaddr);
}
|
    '&' term
{
	$$ = amper($2);
}
|
    address '+' address
{
	$$ = build(O_ADD, $1, $3);
}
|
    address '-' address
{
	$$ = build(O_SUB, $1, $3);
}
|
    address '*' address
{
	$$ = build(O_MUL, $1, $3);
}
|
    '*' address %prec UNARYSIGN
{
	$$ = build(O_INDIR, $2);
}
|
    '-' address %prec UNARYSIGN
{
	$$ = build(O_NEG, $2);
}
|
    '(' exp ')'
{
	$$ = $2;
}
;
term:
    symbol
{
	$$ = $1;
}
|
    term '.' name
{
	$$ = unrval(dot($1, $3));
}
|
    term ARROW name
{
	$$ = unrval(dot($1, $3));
}
|
    term '[' exp_list ']'
{
	$$ = unrval(subscript($1, $3));
}
;
count:
    /* empty */
{
	$$ = 1;
}
|
    INT
{
	$$ = $1;
}
;
mode:
    name
{
	$$ = ident($1);
	curformat = $$;
}
|
    /* empty */
{
	$$ = curformat;
}
;
opt_cond:
    /* empty */
{
	$$ = nil;
}
|
    IF boolean_exp
{
	$$ = $2;
}
;
exp_list:
    exp
{
	$$ = build(O_COMMA, $1, nil);
}
|
    exp ',' exp_list
{
	$$ = build(O_COMMA, $1, $3);
}
;
exp:
    symbol
{
	$$ = build(O_RVAL, $1);
}
|
    exp '[' exp_list ']'
{
	$$ = subscript(unrval($1), $3);
}
|
    exp '.' name
{
	$$ = dot($1, $3);
}
|
    exp ARROW name
{
	$$ = dot($1, $3);
}
|
    '*' exp %prec UNARYSIGN
{
	$$ = build(O_INDIR, $2);
}
|
    exp '^' %prec UNARYSIGN
{
	$$ = build(O_INDIR, $1);
}
|
    exp '\\' opt_qual_symbol
{
	$$ = build(O_TYPERENAME, $1, $3);
}
|
    exp '\\' '&' opt_qual_symbol %prec '\\'
{
	$$ = renameptr($1, $4);
}
|
    exp '(' opt_exp_list ')'
{
	$$ = build(O_CALL, unrval($1), $3);
}
|
    constant
{
	$$ = $1;
}
|
    '+' exp %prec UNARYSIGN
{
	$$ = $2;
}
|
    '-' exp %prec UNARYSIGN
{
	$$ = build(O_NEG, $2);
}
|
    '&' exp %prec UNARYSIGN
{
	$$ = amper($2);
}
|
    exp '+' exp
{
	$$ = build(O_ADD, $1, $3);
}
|
    exp '-' exp
{
	$$ = build(O_SUB, $1, $3);
}
|
    exp '*' exp
{
	$$ = build(O_MUL, $1, $3);
}
|
    exp '/' exp
{
	$$ = build(O_DIVF, $1, $3);
}
|
    exp DIV exp
{
	$$ = build(O_DIV, $1, $3);
}
|
    exp MOD exp
{
	$$ = build(O_MOD, $1, $3);
}
|
    exp AND exp
{
	$$ = build(O_AND, $1, $3);
}
|
    exp OR exp
{
	$$ = build(O_OR, $1, $3);
}
|
    exp '<' exp
{
	$$ = build(O_LT, $1, $3);
}
|
    exp '<' '=' exp
{
	$$ = build(O_LE, $1, $4);
}
|
    exp '>' exp
{
	$$ = build(O_GT, $1, $3);
}
|
    exp '>' '=' exp
{
	$$ = build(O_GE, $1, $4);
}
|
    exp '=' exp
{
	$$ = build(O_EQ, $1, $3);
}
|
    exp '=' '=' exp
{
	$$ = build(O_EQ, $1, $4);
}
|
    exp '<' '>' exp
{
	$$ = build(O_NE, $1, $4);
}
|
    exp '!' '=' exp
{
	$$ = build(O_NE, $1, $4);
}
|
    '(' exp ')'
{
	$$ = $2;
}
;
boolean_exp:
    exp
{
	chkboolean($1);
	$$ = $1;
}
;
constant:
    INT
{
	$$ = build(O_LCON, $1);
}
|
    CHAR
{
	$$ = build(O_CCON, $1);
}
|
    REAL
{
	$$ = build(O_FCON, $1);
}
|
    STRING
{
	$$ = build(O_SCON, $1);
}
;
module_symbol:
    symbol
{
	$$ = $1;
}
|
    module '.' name
{
	$$ = dot($1, $3);
}
opt_qual_symbol:
    symbol
{
	$$ = $1;
}
|
    opt_qual_symbol '.' name
{
	$$ = dot($1, $3);
}
;
module:
    name
{
	$$ = findvar($1);
	if ($$ == nil) {
		$$ = (Node)findmodule($1);
		if ($$ == nil) {
			error("unable to locate module");
		} else {
	    	$$ = build(O_SYM, $$);
		}
	} else {
	    error("can only \"set/unset\" debugger variables");
	}
}
;

symbol:
    name
{
	$$ = findvar($1);
	if ($$ == nil) {
	    $$ = build(O_SYM, which($1));
	} else {
	    error("can only \"set/unset\" debugger variables");
	}
}
|
    '.' name
{
	$$ = dot(build(O_SYM, program), $2);
}
;
name:
    NAME
{
	$$ = $1;
}
|
    keyword
{
	$$ = $1;
}
keyword:
    ALIAS | AND | ASSIGN | AT | 
	CALL | CALLV | CATCH | CD | CONT |
	DEBUG | DELETE | DIV | 
    DOWN | DUMP | EDIT | FILE | F_MASK | FUNC |
	GRIPE | GETENV | HELP | HISTORY | IGNORE | IN |
	LIST | MOD | NEXT | NEXTI | NIL | NOT | OR |
	PRINT | PRINTF | PSYM | PWD | QUIT |
    RECORD | REREAD | RERUN | RETURN | RUN | SET | SETENV | SH | SKIP | 
	SOURCE | STATUS | STEP | STEPI |
    STOP | STOPI | T_MASK | TRACE | TRACEI | UNALIAS |
	UNSET | UP | USE | 
    WHATIS | WHEN | WHERE | WHEREIS | WHICH
;
