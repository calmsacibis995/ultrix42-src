/*static	char	*sccsid = "@(#)cgram.y	4.1	(ULTRIX)	11/23/87";*/


/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/********************************************************************
 *
 *		Modification History
 *
 *	David Metsky. 2-Jul-87
 * 006	Changed the token numbers for const and volatile from 114 and 
 *	115 to 128 and 129 respectively.
 *
 *	Victoria Holt, 14-Apr-86
 * 005	Bug fix for constant pointers in mkty().
 *
 *	Lu Anne Van de Pas, 02-Mar-86
 * 004	Added support for f floating constants. 
 * 
 *	Victoria Holt, 26-Feb-86
 * 003	Added support for const and volatile.  Included changing the
 *	way types are handled, making the grammar recursive.  Types()
 *	was rewritten to reflect this.
 *
 * 	David L. Ballenger, 14-Nov-1984, 1.3
 * 002	Make asminfo processing conditional on ONEPASS being defined.
 *	This fixes problems with code shared with lint.
 *	
 *	Rich Phillips, 16-Aug-84
 * 001- Include asm processing in the syntax(see scan.c).
 *      
 *
 ********************************************************************/
%term NAME  2
%term STRING  3
%term ICON  4
%term FCON  5
%term DCON 113
%term PLUS   6
%term MINUS   8
%term MUL   11
%term AND   14
%term OR   17
%term ER   19
%term QUEST  21
%term COLON  22
%term ANDAND  23
%term OROR  24

/*	special interfaces for yacc alone */
/*	These serve as abbreviations of 2 or more ops:
	ASOP	=, = ops
	RELOP	LE,LT,GE,GT
	EQUOP	EQ,NE
	DIVOP	DIV,MOD
	SHIFTOP	LS,RS
	ICOP	ICR,DECR
	UNOP	NOT,COMPL
	STROP	DOT,STREF

	*/
%term ASOP  25
%term RELOP  26
%term EQUOP  27
%term DIVOP  28
%term SHIFTOP  29
%term INCOP  30
%term UNOP  31
%term STROP  32

/*	reserved words, etc */
%term TYPE  33
%term CLASS  34
%term STRUCT  35
%term RETURN  36
%term GOTO  37
%term IF  38
%term ELSE  39
%term SWITCH  40
%term BREAK  41
%term CONTINUE  42
%term WHILE  43
%term DO  44
%term FOR  45
%term DEFAULT  46
%term CASE  47
%term SIZEOF  48
%term ENUM 49


/*	little symbols, etc. */
/*	namely,

	LP	(
	RP	)

	LC	{
	RC	}

	LB	[
	RB	]

	CM	,
	SM	;

	*/

%term LP  50
%term RP  51
%term LC  52
%term RC  53
%term LB  54
%term RB  55
%term CM  56
%term SM  57
%term ASSIGN  58
%term ASM 59
/* 006 - dnm */
%term CONST 128
%term VOLATILE 129

/* at last count, there were 7 shift/reduce, 1 reduce/reduce conflicts
/* these involved:
	if/else
	recognizing functions in various contexts, including declarations
	error recovery
	*/

/* Are currently 11 shift/reduce, 0 reduce/reduce.  Additional shift/reduce
 * are in recognizing MUL declarations, where both left and right recursion
 * allow for a shift/reduce conflict.  See nfdeclarator and fdeclarator.
 */

%left CM
%right ASOP ASSIGN
%right QUEST COLON
%left OROR
%left ANDAND
%left OR
%left ER
%left AND
%left EQUOP
%left RELOP
%left SHIFTOP
%left PLUS MINUS
%left MUL DIVOP
%right UNOP
%right INCOP SIZEOF
%left LB LP STROP
%{
# include "mfile1"
%}

	/* define types */
%start ext_def_list

%type <intval> con_e ifelprefix ifprefix whprefix forprefix doprefix 
		switchpart multype enum_head str_head name_lp asmpart

%type <nodep>  e .e term attributes oattributes type typehead typelist 
		enum_dcl struct_dcl asm_idn cast_type null_decl
		funct_idn declarator fdeclarator nfdeclarator elist

%token <intval> CLASS NAME STRUCT RELOP CM DIVOP PLUS MINUS SHIFTOP MUL 
		AND OR ER ANDAND OROR ASSIGN STROP INCOP UNOP ICON ASM

%token <nodep> TYPE CONST VOLATILE

%%

%{

int		in_arg_dcl_list = 0;				/* qar 2349 - detect args declared
											 *  as static
											 */

	static int fake = 0;
#ifndef FLEXNAMES
	static char fakename[NCHNAM+1];
#else
	static char fakename[24];
#endif
%}

ext_def_list:	   ext_def_list external_def
		|
			=ftnend();
		;
external_def:	   data_def
			={ curclass = SNULL;  blevel = 0; }
		|  error
			={ curclass = SNULL;  blevel = 0; }
		;
data_def:
		   oattributes  SM
			={  $1->in.op = FREE; }
		|  oattributes init_dcl_list  SM
			={  $1->in.op = FREE; }
		|  oattributes fdeclarator {
				defid( tymerge($1,$2), curclass==STATIC?STATIC:EXTDEF );
#ifndef LINT
				pfstab(stab[$2->tn.rval].sname);
#endif
				}  function_body
			={  
			    if( blevel ) cerror( "function level error" );
			    if( reached ) retstat |= NRETVAL; 
			    $1->in.op = FREE;
			    ftnend();
			    }
		|  asmpartnow SM    /*RAP001  put out asm info now */
		;

function_body:	   arg_dcl_list
			 { in_arg_dcl_list = 0; }		/* qar 2349 */

             compoundstmt
		;
arg_dcl_list:	   arg_dcl_list 
			 { in_arg_dcl_list = 1;	}		/* qar 2349 */

			declaration

		| 	={  blevel = 1; }
		;

stmt_list:	   stmt_list statement
		|  /* empty */
			={  bccode();
			    locctr(PROG);
			    }
		;

r_dcl_stat_list	:  dcl_stat_list attributes SM
			={  $2->in.op = FREE; 
#ifndef LINT
			    plcstab(blevel);
#endif
			    }
		|  dcl_stat_list attributes init_dcl_list SM
			={  $2->in.op = FREE; 
#ifndef LINT
			    plcstab(blevel);
#endif
			    }
		;

dcl_stat_list	:  dcl_stat_list attributes SM
			={  $2->in.op = FREE; }
		|  dcl_stat_list attributes init_dcl_list SM
			={  $2->in.op = FREE; }
		|  /* empty */
		;
declaration:	   attributes declarator_list  SM
			={ curclass = SNULL;  $1->in.op = FREE; }
		|  attributes SM
			={ curclass = SNULL;  $1->in.op = FREE; }
		|  error  SM
			={  curclass = SNULL; }
		;
oattributes:	  attributes
		|  /* VOID */
			={  $$ = mkty(INT,0,INT);  curclass = SNULL; }
		;
attributes:	   class typehead
			={ $$ = $2; }
		|  typehead class
		|  class
			={  $$ = mkty(INT,0,INT); }
		|  typehead
			={ curclass = SNULL; }
		|  typehead class typehead
			={ types($1, $3, NIL); }
		;
class:		   CLASS
			={  curclass = $1; }
		;

typehead:	   type typelist
			={ $1->in.left = $2;
			   /* Call types to merge several types into
			      one; only call if there is more than one
			      type present in the declaration.
			   */
			   if ($2 != NIL) types($1);
			 }
		|  struct_dcl
		|  enum_dcl
		;

typelist:	   /* empty */
			={ $$ = NIL; }
		|  type typelist
			={ $1->in.left = $2; }
		;

type:		   TYPE
		|  CONST
		|  VOLATILE
		;

enum_dcl:	   enum_head LC moe_list optcomma RC
			={ $$ = dclstruct($1); }
		|  typelist ENUM NAME
			={ $$ = rstruct($3,0,cvcheck($1));  stwart = instruct;
			 }
		;

enum_head:	   typelist ENUM
			={ $$ = bstruct(-1,0,cvcheck($1)); stwart = SEENAME;
			 }
		|  typelist ENUM NAME
			={ $$ = bstruct($3,0,cvcheck($1)); stwart = SEENAME;
			 }
		;

moe_list:	   moe
		|  moe_list CM moe
		;

moe:		   NAME
			={  moedef( $1 ); }
		|  NAME ASSIGN con_e
			={  strucoff = $3;  moedef( $1 ); }
		;

struct_dcl:	   str_head LC type_dcl_list optsemi RC
			={ $$ = dclstruct($1);  }
		|  typelist STRUCT NAME
			={ $$ = rstruct($3,$2,cvcheck($1)); }
		;

str_head:	   typelist STRUCT
			={ $$ = bstruct(-1,$2,cvcheck($1));  stwart=0;
			 }
		|  typelist STRUCT NAME
			={ $$ = bstruct($3,$2,cvcheck($1));  stwart=0;
			 }
		;

type_dcl_list:	   type_declaration
		|  type_dcl_list SM type_declaration
		;

type_declaration:  typehead declarator_list
			={ curclass = SNULL;  stwart=0; $1->in.op = FREE; }
		|  typehead
			={  if( curclass != MOU ){
				curclass = SNULL;
				}
			    else {
				sprintf( fakename, "$%dFAKE", fake++ );
#ifdef FLEXNAMES
				/* No need to hash this, we won't look it up */
				defid( tymerge($1, bdty(NAME,NIL,lookup( savestr(fakename), SMOS ))), curclass );
#else
				defid( tymerge($1, bdty(NAME,NIL,lookup( fakename, SMOS ))), curclass );
#endif
				werror("structure typed union member must be named");
				}
			    stwart = 0;
			    $1->in.op = FREE;
			    }
		;

declarator_list:   declarator
			={ defid( tymerge($<nodep>0,$1), curclass);  stwart = instruct; }
		|  declarator_list  CM {$<nodep>$=$<nodep>0;}  declarator
			={ defid( tymerge($<nodep>0,$4), curclass);  stwart = instruct; }
		;
declarator:	   fdeclarator
		|  nfdeclarator
		|  nfdeclarator COLON con_e
			%prec CM
			={  if( !(instruct&INSTRUCT) ) uerror( "field outside of structure" );
			    if( $3<0 || $3 >= FIELD ){
				uerror( "illegal field size" );
				$3 = 1;
				}
			    defid( tymerge($<nodep>0,$1), FIELD|$3 );
			    $$ = NIL;
			    }
		|  COLON con_e
			%prec CM
			={  if( !(instruct&INSTRUCT) ) uerror( "field outside of structure" );
			    falloc( stab, $2, -1, $<nodep>0 );  /* alignment or hole */
			    $$ = NIL;
			    }
		|  error
			={  $$ = NIL; }
		;

		/* int (a)();   is not a function --- sorry! */
nfdeclarator:	   multype nfdeclarator		
			={  umul:
				$$ = bdty( UNARY MUL, $2, $1 ); }
		|  nfdeclarator  LP   RP		
			={  uftn:
				$$ = bdty( UNARY CALL, $1, 0 );  }
		|  nfdeclarator LB RB		
			={  uary:
				$$ = bdty( LB, $1, 0 );  }
		|  nfdeclarator LB con_e RB	
			={  bary:
				if( (int)$3 <= 0 ) werror( "zero or negative subscript" );
				$$ = bdty( LB, $1, $3 );  }
		|  NAME  		
			/* Resolve qar 2349 - If static is specified in arg list
			 *    admonish the user.
			 */
			={  if (in_arg_dcl_list && (curclass == STATIC))
				{
					curclass = SNULL;
					werror("\"static\" is an invalid storage type in this declaration");
				}
				$$ = bdty( NAME, NIL, $1 );
			}
		|   LP  nfdeclarator  RP 		
			={ $$=$2; }
		;
fdeclarator:	   multype fdeclarator
			={  goto umul; }
		|  fdeclarator  LP   RP
			={  goto uftn; }
		|  fdeclarator LB RB
			={  goto uary; }
		|  fdeclarator LB con_e RB
			={  goto bary; }
		|   LP  fdeclarator  RP
			={ $$ = $2; }
		|  name_lp  name_list  RP
			={
				if( blevel!=0 ) uerror("function declaration in bad context");
				$$ = bdty( UNARY CALL, bdty(NAME,NIL,$1), 0 );
				stwart = 0;
				}
		|  name_lp RP
			={
				$$ = bdty( UNARY CALL, bdty(NAME,NIL,$1), 0 );
				stwart = 0;
				}
		;

multype:	  MUL
			={ $$ = 0; }
		| MUL CONST
			={ $2->in.op = FREE;
			   $$ = BCONST_PTR;	/* vjh005 */
			 }	
		| MUL VOLATILE
			={ $2->in.op = FREE;
			   $$ = BVOLATILE_PTR;	/* vjh005 */
			 }
		;

name_lp:	  NAME LP
			={
				/* turn off typedefs for argument names */
				stwart = SEENAME;
				if( stab[$1].sclass == SNULL )
				    stab[$1].stype = FTN;
				}
		;

name_list:	   NAME			
			={ ftnarg( $1 );  stwart = SEENAME; }
		|  name_list  CM  NAME 
			={ ftnarg( $3 );  stwart = SEENAME; }
		| error
		;
		/* always preceeded by attributes: thus the $<nodep>0's */
init_dcl_list:	   init_declarator
			%prec CM
		|  init_dcl_list  CM {$<nodep>$=$<nodep>0;}  init_declarator
		;
		/* always preceeded by attributes */
xnfdeclarator:	   nfdeclarator
			={  defid( $1 = tymerge($<nodep>0,$1), curclass);
			    beginit($1->tn.rval);
			    }
		|  error
		;
		/* always preceeded by attributes */
init_declarator:   nfdeclarator
			={  nidcl( tymerge($<nodep>0,$1) ); }
		|  fdeclarator
			={  defid( tymerge($<nodep>0,$1), uclass(curclass) );
			}
		|  xnfdeclarator optasgn e
			%prec CM
			={  doinit( $3 );
			    endinit(); }
		|  xnfdeclarator optasgn LC init_list optcomma RC
			={  endinit(); }
		| error
		;

init_list:	   initializer
			%prec CM
		|  init_list  CM  initializer
		;
initializer:	   e
			%prec CM
			={  doinit( $1 ); }
		|  ibrace init_list optcomma RC
			={  irbrace(); }
		;

optcomma	:	/* VOID */
		|  CM
		;

optsemi		:	/* VOID */
		|  SM
		;

optasgn		:	/* VOID */
			={  werror( "old-fashioned initialization: use =" ); }
		|  ASSIGN
		;

ibrace		: LC
			={  ilbrace(); }
		;

/*	STATEMENTS	*/

compoundstmt:	   dcmpstmt
		|  cmpstmt
		;

dcmpstmt:	   begin r_dcl_stat_list stmt_list RC
			={  
#ifndef LINT
			    prcstab(blevel);
#endif
			    --blevel;
			    if( blevel == 1 ) blevel = 0;
			    clearst( blevel );
			    checkst( blevel );
			    autooff = *--psavbc;
			    regvar = *--psavbc;
			    }
		;

cmpstmt:	   begin stmt_list RC
			={  --blevel;
			    if( blevel == 1 ) blevel = 0;
			    clearst( blevel );
			    checkst( blevel );
			    autooff = *--psavbc;
			    regvar = *--psavbc;
			    }
		;

begin:		  LC
			={  if( blevel == 1 ) dclargs();
			    ++blevel;
			    if( psavbc > &asavbc[BCSZ-2] ) cerror( "nesting too deep" );
			    *psavbc++ = regvar;
			    *psavbc++ = autooff;
			    }
		;

statement:	   e   SM
			={ ecomp( $1 ); }
		|  compoundstmt
		|  ifprefix statement
			={ deflab($1);
			   reached = 1;
			   }
		|  ifelprefix statement
			={  if( $1 != NOLAB ){
				deflab( $1 );
				reached = 1;
				}
			    }
		|  whprefix statement
			={  branch(  contlab );
			    deflab( brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP)) reached = 1;
			    else reached = 0;
			    resetbc(0);
			    }
		|  doprefix statement WHILE  LP  e  RP   SM
			={  deflab( contlab );
			    if( flostat & FCONT ) reached = 1;
			    ecomp( buildtree( CBRANCH, buildtree( NOT, $5, NIL ), bcon( $1 ) ) );
			    deflab( brklab );
			    reached = 1;
			    resetbc(0);
			    }
		|  forprefix .e RP statement
			={  deflab( contlab );
			    if( flostat&FCONT ) reached = 1;
			    if( $2 ) ecomp( $2 );
			    branch( $1 );
			    deflab( brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP) ) reached = 1;
			    else reached = 0;
			    resetbc(0);
			    }
		| switchpart statement
			={  if( reached ) branch( brklab );
			    deflab( $1 );
			   swend();
			    deflab(brklab);
			    if( (flostat&FBRK) || !(flostat&FDEF) ) reached = 1;
			    resetbc(FCONT);
			    }
		|  BREAK  SM
			={  if( brklab == NOLAB ) uerror( "illegal break");
			    else if(reached) branch( brklab );
			    flostat |= FBRK;
			    if( brkflag ) goto rch;
			    reached = 0;
			    }
		|  CONTINUE  SM
			={  if( contlab == NOLAB ) uerror( "illegal continue");
			    else branch( contlab );
			    flostat |= FCONT;
			    goto rch;
			    }
		|  RETURN  SM
			={  retstat |= NRETVAL;
			    branch( retlab );
			rch:
			    if( !reached ) werror( "statement not reached");
			    reached = 0;
			    }
		|  RETURN e  SM
			={  register NODE *temp;
			    idname = curftn;
			    temp = buildtree( NAME, NIL, NIL );
			    if(temp->in.type == TVOID)
				uerror("void function %s cannot return value",
					stab[idname].sname);
			    temp->in.type = DECREF( temp->in.type );
			    temp = buildtree( RETURN, temp, $2 );
			    /* now, we have the type of the RHS correct */
			    temp->in.left->in.op = FREE;
			    temp->in.op = FREE;
			    ecomp( buildtree( FORCE, temp->in.right, NIL ) );
			    retstat |= RETVAL;
			    branch( retlab );
			    reached = 0;
			    }
		|  GOTO NAME SM
			={  register NODE *q;
			    q = block( FREE, NIL, NIL, INT|ARY, 0, INT );
			    q->tn.rval = idname = $2;
			    defid( q, ULABEL );
			    stab[idname].suse = -lineno;
			    branch( stab[idname].offset );
			    goto rch;
			    }
		|   SM
		|  error  SM
		|  error RC
		|  label statement
		;
asmpartnow:	   ASM  
			={
			   procasm();		/* Read in the asm info */
			   outasm(asmptr);	/* Put it out now */
			 }  /* RAP001 */
		;
asmpart:	   ASM
			={ $$ = $1;
			   procasm(); 		/* Read in the asm info */
						/* (put out in match.c) */
			 }  /* RAP001 */
		;

label:		   NAME COLON
			={  register NODE *q;
			    q = block( FREE, NIL, NIL, INT|ARY, 0, LABEL );
			    q->tn.rval = $1;
			    defid( q, LABEL );
			    reached = 1;
			    }
		|  CASE e COLON
			={  addcase($2);
			    reached = 1;
			    }
		|  DEFAULT COLON
			={  reached = 1;
			    adddef();
			    flostat |= FDEF;
			    }
		;
doprefix:	DO
			={  savebc();
			    if( !reached ) werror( "loop not entered at top");
			    brklab = getlab();
			    contlab = getlab();
			    deflab( $$ = getlab() );
			    reached = 1;
			    }
		;
ifprefix:	IF LP e RP
			={  ecomp( buildtree( CBRANCH, $3, bcon( $$=getlab()) ) ) ;
			    reached = 1;
			    }
		;
ifelprefix:	  ifprefix statement ELSE
			={  if( reached ) branch( $$ = getlab() );
			    else $$ = NOLAB;
			    deflab( $1 );
			    reached = 1;
			    }
		;

whprefix:	  WHILE  LP  e  RP
			={  savebc();
			    if( !reached ) werror( "loop not entered at top");
			    if( $3->in.op == ICON && $3->tn.lval != 0 ) flostat = FLOOP;
			    deflab( contlab = getlab() );
			    reached = 1;
			    brklab = getlab();
			    if( flostat == FLOOP ) tfree( $3 );
			    else ecomp( buildtree( CBRANCH, $3, bcon( brklab) ) );
			    }
		;
forprefix:	  FOR  LP  .e  SM .e  SM 
			={  if( $3 ) ecomp( $3 );
			    else if( !reached ) werror( "loop not entered at top");
			    savebc();
			    contlab = getlab();
			    brklab = getlab();
			    deflab( $$ = getlab() );
			    reached = 1;
			    if( $5 ) ecomp( buildtree( CBRANCH, $5, bcon( brklab) ) );
			    else flostat |= FLOOP;
			    }
		;
switchpart:	   SWITCH  LP  e  RP
			={  savebc();
			    brklab = getlab();
			    ecomp( buildtree( FORCE, $3, NIL ) );
			    branch( $$ = getlab() );
			    swstart();
			    reached = 0;
			    }
		;
/*	EXPRESSIONS	*/
con_e:		   { $<intval>$=instruct; stwart=instruct=0; } e
			%prec CM
			={  $$ = icons( $2 );  instruct=$<intval>1; }
		;
.e:		   e
		|
			={ $$=0; }
		;
elist:		   e
			%prec CM
		|  elist  CM  e
			={  goto bop; }
		;

e:		   e RELOP e
			={
			preconf:
			    if( yychar==RELOP||yychar==EQUOP||yychar==AND||yychar==OR||yychar==ER ){
			    precplaint:
				if( hflag ) werror( "precedence confusion possible: parenthesize!" );
				}
			bop:
			    $$ = buildtree( $2, $1, $3 );
			    }
		|  e CM e
			={  $2 = COMOP;
			    goto bop;
			    }
		|  e DIVOP e
			={  goto bop; }
		|  e PLUS e
			={  if(yychar==SHIFTOP) goto precplaint; else goto bop; }
		|  e MINUS e
			={  if(yychar==SHIFTOP ) goto precplaint; else goto bop; }
		|  e SHIFTOP e
			={						/* resolve qar 2001: */
	/* Detect cases of shifts larger than the object; eg:
	*  y = x << 2 + 40;
	*
	* yypvt[-2].nodep (aka 'l' in buldtree) would have l.in.op = 2 (NAME x)
	* 	and l.in.type = 5 (long if x is a long) or set to the proper value
    * similary yypvt[-3].nodep (aka 'r'), would have
	*       r.tn.type = 4 (ICON constant)
	*    and r.tn.lval = 42 (the value).
    */

 				NODE *l, * r;
				l = yypvt[-2].nodep;
				r = yypvt[-0].nodep;

				if  (r->tn.op == ICON)
				{
					if (r->tn.lval > dimtab[ l->in.type ])
					{
						werror("Shift of %d is > object size of %d",
							   r->tn.lval, dimtab[ l->in.type]);
					}
				}

				
				if(yychar==PLUS||yychar==MINUS) goto precplaint;
				else goto bop;
			}

		|  e MUL e
			={  goto bop; }
		|  e EQUOP  e
			={  goto preconf; }
		|  e AND e
			={  if( yychar==RELOP||yychar==EQUOP ) goto preconf;  else goto bop; }
		|  e OR e
			={  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; }
		|  e ER e
			={  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; }
		|  e ANDAND e
			={  goto bop; }
		|  e OROR e
			={  goto bop; }
		|  e MUL ASSIGN e
			={  abop:
				$$ = buildtree( ASG $2, $1, $4 );
				}
		|  e DIVOP ASSIGN e
			={  goto abop; }
		|  e PLUS ASSIGN e
			={  goto abop; }
		|  e MINUS ASSIGN e
			={  goto abop; }
		|  e SHIFTOP ASSIGN e
			={  goto abop; }
		|  e AND ASSIGN e
			={  goto abop; }
		|  e OR ASSIGN e
			={  goto abop; }
		|  e ER ASSIGN e
			={  goto abop; }
		|  e QUEST e COLON e
			={  $$=buildtree(QUEST, $1, buildtree( COLON, $3, $5 ) );
			    }
		|  e ASOP e
			={  werror( "old-fashioned assignment operator" );  goto bop; }
		|  e ASSIGN e
			={  goto bop; }
		|  term
		;
term:		   term INCOP
			={  $$ = buildtree( $2, $1, bcon(1) ); }
		|  MUL term
			={ ubop:
			    $$ = buildtree( UNARY $1, $2, NIL );
			    }
		|  AND term
			={  if( ISFTN($2->in.type) || ISARY($2->in.type) ){
				werror( "& before array or function: ignored" );
				$$ = $2;
				}
			    else goto ubop;
			    }
		|  MINUS term
			={  goto ubop; }
		|  UNOP term
			={
			    $$ = buildtree( $1, $2, NIL );
			    }
		|  INCOP term
			={  $$ = buildtree( $1==INCR ? ASG PLUS : ASG MINUS,
						$2,
						bcon(1)  );
			    }
		|  SIZEOF term
			={  $$ = doszof( $2 ); }
		|  LP cast_type RP term  %prec INCOP
			={  $$ = buildtree( CAST, $2, $4 );
			    $$->in.left->in.op = FREE;
			    $$->in.op = FREE;
			    $$ = $$->in.right;
			    }
		|  SIZEOF LP cast_type RP  %prec SIZEOF
			={  $$ = doszof( $3 ); }
		|  term LB e RB
			={  $$ = buildtree( UNARY MUL, buildtree( PLUS, $1, $3 ), NIL ); }
		|  asm_idn	/* RAP001 */
			={  $$=buildtree(UNARY CALL,$1,NIL); }
		|  funct_idn  RP
			={  $$=buildtree(UNARY CALL,$1,NIL); }
		|  funct_idn elist  RP
			={  $$=buildtree(CALL,$1,$2); }
		|  term STROP NAME
			={  if( $2 == DOT ){
				if( notlval( $1 ) )uerror("structure reference must be addressable");
				$1 = buildtree( UNARY AND, $1, NIL );
				}
			    idname = $3;
			    $$ = buildtree( STREF, $1, buildtree( NAME, NIL, NIL ) );
			    }
		|  NAME
			={  idname = $1;
			    /* recognize identifiers in initializations */
			    if( blevel==0 && stab[idname].stype == UNDEF ) {
				register NODE *q;
#ifndef FLEXNAMES
				werror( "undeclared initializer name %.8s", stab[idname].sname );
#else
				werror( "undeclared initializer name %s", stab[idname].sname );
#endif
				q = block( FREE, NIL, NIL, INT, 0, INT );
				q->tn.rval = idname;
				defid( q, EXTERN );
				}
			    $$=buildtree(NAME,NIL,NIL);
			    stab[$1].suse = -lineno;
			}
		|  ICON
			={  $$=bcon(0);
			    $$->tn.lval = lastcon;
			    $$->tn.rval = NONAME;
			    if( $1 ) $$->fn.csiz = $$->in.type = ctype(LONG);
			    }
		|  FCON
			={  $$=buildtree(FCON,NIL,NIL);
			    $$->fpn.fval = fcon;
			    }
			/* vdp004 -maintain a distinction between floating 
			 * constants and double constants 
			 */ 
		|  DCON
			={  $$= buildtree(DCON,NIL,NIL);
			    $$->dpn.dval = dcon;
			    }
		|  STRING
			={  $$ = getstr(); /* get string contents */ }
		|   LP  e  RP
			={ $$=$2; }
		;

cast_type:	  typehead null_decl
			={
			$$ = tymerge( $1, $2 );
			$$->in.op = NAME;
			$1->in.op = FREE;
			}
		;

null_decl:	   /* empty */
			={ $$ = bdty( NAME, NIL, -1 ); }
		|  LP RP
			={ $$ = bdty( UNARY CALL, bdty(NAME,NIL,-1),0); }
		|  LP null_decl RP LP RP
			={  $$ = bdty( UNARY CALL, $2, 0 ); }
		|  MUL null_decl
			={  goto umul; }
		|  null_decl LB RB
			={  goto uary; }
		|  null_decl LB con_e RB
			={  goto bary;  }
		|  LP null_decl RP
			={ $$ = $2; }
		;

funct_idn:	   NAME  LP 
			={  
			    if( stab[$1].stype == UNDEF ){
				register NODE *q;
				q = block( FREE, NIL, NIL, FTN|INT, 0, INT );
				q->tn.rval = $1;
				defid( q, EXTERN );
				}
			    idname = $1;
			    $$=buildtree(NAME,NIL,NIL);
			    stab[idname].suse = -lineno;
			}
		|  term  LP 
		;
asm_idn:	asmpart /* RAP001 
				Put asm in the symbol table as a function that
				doesn't return a value (if not already there).
				Then build a NAME node with it and pass back
				a pointer to it.
				
				Store asmptr (set up by procasm in scan.c) in
				the NAME node. This is a pointer to a block
				containing the characters in the asm. The code
				will be put out later in match.c
			*/
			={  
			    if( stab[$1].stype == UNDEF ){
				register NODE *q;
				q = block( FREE, NIL, NIL, FTN|UNDEF, 0, INT );
				q->tn.rval = $1;
				defid( q, EXTERN );
				}
			    idname = $1;
			    $$=buildtree(NAME,NIL,NIL);
#ifdef ONEPASS
			    $$->in.asminfo = asmptr;
#endif
			    stab[idname].suse = -lineno;
			}
%%

NODE *
mkty( t, d, s ) unsigned t; {
	return( block( TYPE, NIL, NIL, t, d, s ) );
	}

NODE *
bdty( op, p, v ) NODE *p; {
	register NODE *q;

	q = block( op, p, NIL, INT, 0, INT );

	switch( op ){

	case UNARY MUL:
		if (v == BCONST_PTR || v == BVOLATILE_PTR) {  /* vjh005 */
		    q->in.typattr = v;
		} else {
		    q->in.typattr = 0;
		}
		break;

	case UNARY CALL:
		break;

	case LB:
		q->in.right = bcon(v);
		break;

	case NAME:
		q->tn.rval = v;
		break;

	default:
		cerror( "bad bdty" );
		}

	return( q );
	}

dstash( n ){ /* put n into the dimension table */
	if( curdim >= DIMTABSZ-1 ){
		cerror( "dimension table overflow");
		}
	dimtab[ curdim++ ] = n;
	}

savebc() {
	if( psavbc > & asavbc[BCSZ-4 ] ){
		cerror( "whiles, fors, etc. too deeply nested");
		}
	*psavbc++ = brklab;
	*psavbc++ = contlab;
	*psavbc++ = flostat;
	*psavbc++ = swx;
	flostat = 0;
	}

resetbc(mask){

	swx = *--psavbc;
	flostat = *--psavbc | (flostat&mask);
	contlab = *--psavbc;
	brklab = *--psavbc;

	}

addcase(p) NODE *p; { /* add case to switch */

	p = optim( p );  /* change enum to ints */
	if( p->in.op != ICON ){
		uerror( "non-constant case expression");
		return;
		}
	if( swp == swtab ){
		uerror( "case not in switch");
		return;
		}
	if( swp >= &swtab[SWITSZ] ){
		cerror( "switch table overflow");
		}
	swp->sval = p->tn.lval;
	deflab( swp->slab = getlab() );
	++swp;
	tfree(p);
	}

adddef(){ /* add default case to switch */
	if( swtab[swx].slab >= 0 ){
		uerror( "duplicate default in switch");
		return;
		}
	if( swp == swtab ){
		uerror( "default not inside switch");
		return;
		}
	deflab( swtab[swx].slab = getlab() );
	}

swstart(){
	/* begin a switch block */
	if( swp >= &swtab[SWITSZ] ){
		cerror( "switch table overflow");
		}
	swx = swp - swtab;
	swp->slab = -1;
	++swp;
	}

swend(){ /* end a switch block */

	register struct sw *swbeg, *p, *q, *r, *r1;
	CONSZ temp;
	int tempi;

	swbeg = &swtab[swx+1];

	/* sort */

	r1 = swbeg;
	r = swp-1;

	while( swbeg < r ){
		/* bubble largest to end */
		for( q=swbeg; q<r; ++q ){
			if( q->sval > (q+1)->sval ){
				/* swap */
				r1 = q+1;
				temp = q->sval;
				q->sval = r1->sval;
				r1->sval = temp;
				tempi = q->slab;
				q->slab = r1->slab;
				r1->slab = tempi;
				}
			}
		r = r1;
		r1 = swbeg;
		}

	/* it is now sorted */

	for( p = swbeg+1; p<swp; ++p ){
		if( p->sval == (p-1)->sval ){
			uerror( "duplicate case in switch, %d", tempi=p->sval );
			return;
			}
		}

	genswitch( swbeg-1, swp-swbeg );
	swp = swbeg-1;
	}
