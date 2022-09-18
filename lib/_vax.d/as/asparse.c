/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988 by			*
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
/******************************************************************
 *
 *		Modification History
 *
 * 006  Tanya Klinchina, 20-Nov-1989
 *      Added support for vector instructions.
 *
 * 005	jlr, 21-Jun-1988
 *	Catch .space <negative-number> and give a message instead of
 *	dumping core.
 *
 * 004	Jon Reeves, 13-Aug-1987
 *	Fix to change 002: a stab that referenced a defined variable
 *	that had previously been forward referenced was getting the
 *	variable's value doubled.
 *
 * 003	David L Ballenger, 24-Feb-1986
 *	Fix handling of .align statements so that fill expressions are
 *	correctly handled.  Also, move checking of the alignment
 *	expressions to yyparse() in this module from jalign() in
 *	asjxxx.c.
 *
 * 002- Victoria Holt, 07-May-85
 *	Fixed problem with N_LCSYM stabs.  The code that processed
 * 	them only worked when the stab was making a forward
 *	reference.  Also fixed the "hack" that masked and shifted an
 *	offset into the n_desc and n_other fields of the nlist to prevent
 * 	it from getting munged later.
 *
 *	Rich Phillips, 02-Aug-84
 * 001-	Update the location counter on pass2 when processing a .fill.
 *	It used to only update it for pass 1, causing relocation information
 *	problems.
 *
 *	Based on:  asparse.c 4.17 7/1/83
 *
 *******************************************************************/
#ifndef lint
static char sccsid[] = "@(#)asparse.c	4.4 ULTRIX 11/9/90";
#endif /* not lint */

#include <stdio.h>
#include "as.h"
#include "asscan.h"
#include "assyms.h"
#include "asexpr.h"


int	lgensym[10];
char	genref[10];

long	bitfield;
int	bitoff;
int	curlen;			/* current length of literals */

/*
 *	The following three variables are communication between various
 *	modules to special case a number of things.  They are properly
 *	categorized as hacks.
 */
extern	struct	symtab *lastnam;/*last name seen by the lexical analyzer*/
int	exprisname;		/*last factor in an expression was a name*/
int	droppedLP;		/*one is analyzing an expression beginning with*/
				/*a left parenthesis, which has already been*/
				/*shifted. (Used to parse (<expr>)(rn)*/

char	yytext[NCPName+2];	/*the lexical image*/
int	yylval;			/*the lexical value; sloppy typing*/
struct	Opcode		yyopcode;	/* lexical value for an opcode */
Bignum	yybignum;		/* lexical value for a big number */
/*
 *	Expression and argument managers
 */
struct	exp	*xp;		/*next free expression slot, used by expr.c*/
struct	exp	explist[NEXP];	/*max of 20 expressions in one opcode*/
struct	arg	arglist[NARG];	/*building up operands in instructions*/
struct	arg	varglist[NARG];	/*building up operands in vector instructions*/

readonly struct Vinst_fmt vinst_fmt[FMTMAX] = {
  { /* FMT0 entry is not used */
    0,
  },
  { /* FMT1 - VLD */
    (MOE | MTF | MI),                   /* modifier mask */
    0,                                  /* default modifiers */
    3,                                  /* # instr operands */
    {
      {VCTRL, 0, 0, VARG3},             /* cntrl.rw, vc */
      {VOPND, VARG1},                   /* base.ab */
      {VOPND, VARG2},                   /* stride.rl */
      },
  },
  { /* FMT2 - VST */
    (MOE | MTF),                        /* modifier mask */
    0,                                  /* default modifiers */
    3,                                  /* # instr opearands */
    {
      {VCTRL, 0, 0, VARG1},             /* cntrl.rw, vc */
      {VOPND, VARG2},                   /* base.ab */
      {VOPND, VARG3},                   /* stride.rl */
      },
  },
  { /* FMT3 - VGATH */
    (MOE | MTF | MI),                   /* modifier mask */
    0,                                  /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VCTRL, 0, VARG2, VARG3},         /* cntrl.rw, vb,vc */
      {VOPND, VARG1},                   /* base.ab */
      },
  },
  { /* FMT4 - VSCAT */
    (MOE | MTF),                        /* modifier mask */
    0,                                  /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VCTRL, 0, VARG3, VARG1},         /* cntrl.rw, vc, vb */
      {VOPND, VARG2},                   /* base.ab */
      },
  },
  { /* FMT5 - VVADD,VVSUB,VVMUL,VVDIV,VVSLLL */
    (MOE | MTF | EXC),                  /* modifier mask */
    0,                                  /* default modifiers */
    1,                                  /* # instr opearands */
    {
      {VCTRL, VARG1, VARG2, VARG3},     /* cntrl.rw, va, vb, vc */
      },
  },
  { /* FMT6 - VVSLLL, VVSRLL, VVBISL, VVXORL, VVBICL */
    (MOE | MTF),                        /* modifier mask */
    0,                                  /* default modifiers */
    1,                                  /* # instr opearands */
    {
      {VCTRL, VARG1, VARG2, VARG3},     /* cntrl.rw, va, vb, vc */
      },
  },
  { /* FMT7 - VVCMPx */
    (MOE | MTF),                        /* modifier mask */
    0,                                  /* default modifiers */
    1,                                  /* # instr opearands */
    {
      {VCTRL, VARG1, VARG2, VCODE},     /* cntrl.rw, va,vb,cmp */
      },
  },
  { /* FMT8 - VVCVT */
    (MOE | MTF | EXC),                  /* modifier mask */
    0,                                  /* default modifiers */
    1,                                  /* # instr opearands */
    {
      {VCTRL, VCODE, VARG1, VARG2},     /* cntrl.rw, cvt,vb,vc */
      },
  },
  { /* FMT9 - VVMERGE */
    (MTF),                              /* modifier mask */
    mtf_bit,                            /* default modifiers */
    1,                                  /* # instr opearands */
    {
      {VCTRL, VARG1, VARG2, VARG3},     /* cntrl.rw, va, vb, vc */
      },
  },
  { /* FMT10- VSADDx,VSSUBx,VSMULx,VSDIVx */
    (MOE | MTF | EXC),                  /* modifier mask */
    0,                                  /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VCTRL, 0, VARG2, VARG3},         /* cntrl.rw, vb,vc */
      {VOPND, VARG1},                   /* scalar.ry */
      },
  },
  { /* FMT11- VSSLLLx,VSSRLLx,VSBISLx,VSXORLx */
    (MOE | MTF),                        /* modifier mask */
    0,                                  /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VCTRL, 0, VARG2, VARG3},         /* cntrl.rw, vb,vc */
      {VOPND, VARG1},                   /* scalar.ry */
      },
  },
  { /* FMT12 - VSCMPx */
    (MOE | MTF),                        /* modifier mask */
    0,                                  /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VCTRL, 0, VARG2, VCODE},         /* cntrl.rw, vb,cmp */
      {VOPND, VARG1},                   /* src.ry */
      },
  },
  { /* FMT13 - VSMERGEx */
    (MTF),                              /* modifier mask */
    mtf_bit,                            /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VCTRL, 0, VARG2, VARG3},         /* cntrl.rw, vb,vc */
      {VOPND, VARG1},                   /* src.rq */
      },
  },
  { /* FMT14 - IOTA */
    (MTF),                              /* modifier mask */
    mtf_bit,                            /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VCTRL, 0, 0, VARG2},             /* cntrl.rw, vc */
      {VOPND, VARG1},                   /* stride.rl */
      },
  },
  { /* FMT15 - MTVP, MFVP */
    0,                                  /* modifier mask */
    0,                                  /* default modifiers */
    2,                                  /* # instr opearands */
    {
      {VLTRL, VCODE},                   /* #leteral */
      {VOPND, VARG1},                   /* src/dst.wl */
      },
  },
  { /* FMT16 - VSYNC */
    0,                                  /* modifier mask */
    0,                                  /* default modifiers */
    1,                                  /* # instr opearands */
    {
      {VLTRL, VCODE},                   /* #literal */
      },
  }
};
/*
 *	Sets to accelerate token discrimination
 */
char	tokensets[(LASTTOKEN) - (FIRSTTOKEN) + 1];

static	char	UDotsname[64];	/*name of the assembly source*/

yyparse()
{
	reg	struct	exp	*locxp;
		/*
		 *	loc1xp and ptrloc1xp are used in the
		 * 	expression lookahead
		 */
		struct	exp	*loc1xp;	/*must be non register*/
		struct	exp	**ptrloc1xp = & loc1xp;
		struct	exp	*pval;		/*hacking expr:expr*/

	reg	struct	symtab	*np;
	reg	int		argcnt;

	reg	inttoktype	val;		/*what yylex gives*/
	reg	inttoktype	auxval;		/*saves val*/

	reg	struct 	arg	*ap;		/*first free argument*/

	reg	struct	symtab	*p;
	reg	struct	symtab	*stpt;

		struct	strdesc *stringp;	/*handles string lists*/

		int	regno;		/*handles arguments*/
		int	*ptrregno = &regno;
		int	sawmul;		/*saw * */
		int	sawindex;	/*saw [rn]*/
		int	sawsize;
		int	seg_type; 	/*the kind of segment: data or text*/
		int	seg_number;	/*the segment number*/
		int	space_value;	/*how much .space needs*/
		int	fill_rep;	/*how many reps for .fill */
		int	fill_size;	/*how many bytes for .fill */

		int	field_width;	/*how wide a field is to be*/
		int	field_value;	/*the value to stuff in a field*/
		char	*stabname;	/*name of stab dealing with*/
		ptrall	stabstart;	/*where the stab starts in the buffer*/
		int	reloc_how;	/* how to relocate expressions */
		int	toconv;		/* how to convert bignums */
		int	incasetable;	/* set if in a case table */
        union   Vinst_mod vmod;

	incasetable = 0;
	xp = explist;
	ap = arglist;

	val = yylex();

    while (val != PARSEEOF){	/* primary loop */

	while (INTOKSET(val, LINSTBEGIN)){
		if (val == INT) {
			int i = ((struct exp *)yylval)->e_xvalue;
			shift;
			if (val != COLON){
				yyerror("Local label %d is not followed by a ':' for a label definition",
					i);
				goto  errorfix;
			}
			if (i < 0 || i > 9) {
				yyerror("Local labels are 0-9");
				goto errorfix;
			}
			(void)sprintf(yytext, "L%d\001%d", i, lgensym[i]);
			lgensym[i]++;
			genref[i] = 0;
			yylval = (int)*lookup(passno == 1);
			val = NAME;
			np = (struct symtab *)yylval;
			goto restlab;
		}
		if (val == NL){
			lineno++;
			shift;
		} else
		if (val == SEMI) 
			shift;
		else {	/*its a name, so we have a label or def */
			if (val != NAME){
				ERROR("Name expected for a label");
			}
			np = (struct symtab *)yylval;
			shiftover(NAME);
			if (val != COLON) {
				yyerror("\"%s\" is not followed by a ':' for a label definition",
					FETCHNAME(np));
				goto  errorfix;
			}
restlab:
			shift;
			flushfield(NBPW/4);
			if ((np->s_type&XTYPE)!=XUNDEF) {
				if(  (np->s_type&XTYPE)!=dotp->e_xtype 
				   || np->s_value!=dotp->e_xvalue
				   || (  (passno==1)
				       &&(np->s_index != dotp->e_xloc)
				      )
				  ){
#ifndef DEBUG
					if (FETCHNAME(np)[0] != 'L')
#endif /* not DEBUG */
					{
						if (passno == 1)
						  yyerror("%s redefined",
							FETCHNAME(np));
						else
						  yyerror("%s redefined: PHASE ERROR, 1st: %d, 2nd: %d",
							FETCHNAME(np),
							np->s_value,
							dotp->e_xvalue);
					}
				}
			}
			np->s_type &= ~(XTYPE|XFORW);
			np->s_type |= dotp->e_xtype;
			np->s_value = dotp->e_xvalue;
			if (passno == 1){
				np->s_index = dotp-usedot;
				if (FETCHNAME(np)[0] == 'L'){
					nlabels++;
				}
				np->s_tag = LABELID;
			}
		}	/*end of this being a label*/
	}	/*end of to consuming all labels, NLs and SEMIS */ 

	xp = explist;
	ap = arglist;

	/*
	 *	process the INSTRUCTION body
	 */
	switch(val){

    default:
	ERROR("Unrecognized instruction or directive");

   case IABORT:
	shift;
	sawabort();
	/*NOTREACHED*/
	break;

   case PARSEEOF:
	tokptr -= sizeof(bytetoktype);
	*tokptr++ = VOID;
	tokptr[1] = VOID;
	tokptr[2] = PARSEEOF;
	break;

   case IFILE:
	shift;
	stringp = (struct strdesc *)yylval;
	shiftover(STRING);
	dotsname = &UDotsname[0];
	movestr(dotsname, stringp->sd_string,
		min(stringp->sd_strlen, sizeof(UDotsname)));
	break;

   case ILINENO:
	shift;		/*over the ILINENO*/
	expr(locxp, val);
	lineno = locxp->e_xvalue;
	break;

   case ISET: 	/* .set  <name> , <expr> */
	shift;
	np = (struct symtab *)yylval;
	shiftover(NAME);
	shiftover(CM);
	expr(locxp, val);
	np->s_type &= (XXTRN|XFORW);
	np->s_type |= locxp->e_xtype&(XTYPE|XFORW);
	np->s_value = locxp->e_xvalue;
	if (passno==1)
		np->s_index = locxp->e_xloc;
	if ((locxp->e_xtype&XTYPE) == XUNDEF)
		yyerror("Illegal set?");
	break;

   case ILSYM: 	/*.lsym name , expr */
	shift;
	np = (struct symtab *)yylval;
	shiftover(NAME);
	shiftover(CM);
	expr(locxp, val);
	/*
	 *	Build the unique occurance of the
	 *	symbol.
	 *	The character scanner will have
	 *	already entered it into the symbol
	 *	table, but we should remove it
	 */
	if (passno == 1){
		stpt = (struct symtab *)symalloc();
		stpt->s_name = np->s_name;
		np->s_tag = OBSOLETE;	/*invalidate original */
		nforgotten++;
		np = stpt;
		if ( (locxp->e_xtype & XTYPE) != XABS)
			yyerror("Illegal second argument to lsym");
		np->s_value = locxp->e_xvalue;
		np->s_type = XABS;
		np->s_tag = ILSYM;
	}
	break;

   case IGLOBAL: 	/*.globl <name> */
	shift;
	np = (struct symtab *)yylval;
	shiftover(NAME);
	np->s_type |= XXTRN;
	break;

   case IDATA: 	/*.data [ <expr> ] */
   case ITEXT: 	/*.text [ <expr> ] */
	seg_type = -val;
	shift;
	if (INTOKSET(val, EBEGOPS+YUKKYEXPRBEG+SAFEEXPRBEG)){
		expr(locxp, val);
		seg_type = -seg_type;   /*now, it is positive*/
	}

	if (seg_type < 0) {	/*there wasn't an associated expr*/
		seg_number = 0;
		seg_type = -seg_type;
	} else {
		if (   ((locxp->e_xtype & XTYPE) != XABS)	/* tekmdp */
		    || (seg_number = locxp->e_xvalue) >= NLOC) {
			yyerror("illegal location counter");
			seg_number = 0;
		}
	}
	if (seg_type == IDATA)
		seg_number += NLOC;
	flushfield(NBPW/4);
	dotp = &usedot[seg_number];
	if (passno==2) {	/* go salt away in pass 2*/
		txtfil = usefile[seg_number];
		relfil = rusefile[seg_number];
	}
	break;

	/*
	 *	Storage filler directives:
	 *
	 *	.byte	[<exprlist>]
	 *
	 *	exprlist:  empty | exprlist outexpr
	 *	outexpr:   <expr> | <expr> : <expr>
	 */
   case IBYTE:	curlen = NBPW/4; goto elist;
   case IWORD:	curlen = NBPW/2; goto elist;
   case IINT:	curlen = NBPW;   goto elist;
   case ILONG:	curlen = NBPW;   goto elist;

   elist:
	seg_type = val;
	shift;

	/*
	 *	Expression List processing
	 */
	if (INTOKSET(val, EBEGOPS+YUKKYEXPRBEG+SAFEEXPRBEG)){
	    do{
		/*
		 *	expression list consists of a list of :
		 *	<expr>
		 *	<expr> : <expr> 
		 *		(pack expr2 into expr1 bits
		 */
		expr(locxp, val);
		/*
		 *	now, pointing at the next token
		 */
		if (val == COLON){
			shiftover(COLON);
			expr(pval, val);
			if ((locxp->e_xtype & XTYPE) != XABS) /* tekmdp */
				yyerror("Width not absolute");
			field_width = locxp->e_xvalue;
			locxp = pval;
			if (bitoff + field_width > curlen)
				flushfield(curlen);
			if (field_width > curlen)
				yyerror("Expression crosses field boundary");
		} else {
			field_width = curlen;
			flushfield(curlen);
		}

		if ((locxp->e_xtype & XTYPE) != XABS) {
			if (bitoff)
				yyerror("Illegal relocation in field");
			switch(curlen){
				case NBPW/4:	reloc_how = TYPB; break;
				case NBPW/2:	reloc_how = TYPW; break;
				case NBPW:	reloc_how = TYPL; break;
			}
			if (passno == 1){
				dotp->e_xvalue += ty_nbyte[reloc_how];
			} else {
				outrel(locxp, reloc_how);
			}
		} else {
			/*
			 *	
			 *	See if we are doing a case instruction.
			 *	If so, then see if the branch distance,
			 *	stored as a word,
			 *	is going to loose sig bits.
			 */
			if (passno == 2 && incasetable){
				if (  (locxp->e_xvalue < -32768)
				    ||(locxp->e_xvalue > 32767)){
					yyerror("Case will branch too far: try -bswitch flag");
				}
			}
			field_value = locxp->e_xvalue & ( (1L << field_width)-1);
			bitfield |= field_value << bitoff;
			bitoff += field_width;
		}
		xp = explist;
		if (auxval = (val == CM))
			shift;
	    } while (auxval);
	}	/* there existed an expression at all */

	flushfield(curlen);
	if ( ( curlen == NBPW/4) && bitoff)
		dotp->e_xvalue ++;
	break;
	/*end of case IBYTE, IWORD, ILONG, IINT*/

   case ISPACE: 	/* .space <expr> */
	shift;
	expr(locxp, val);
	if ((locxp->e_xtype & XTYPE) != XABS)	/* tekmdp */
		yyerror("Space size not absolute");
	space_value = locxp->e_xvalue;
	if (space_value < 0)			/* jlr005 */
		yyerror("Space size negative");
  ospace:
	flushfield(NBPW/4);
	{
		static char spacebuf[128];
		while (space_value > sizeof(spacebuf)){
			outs(spacebuf, sizeof(spacebuf));
			space_value -= sizeof(spacebuf);
		}
		outs(spacebuf, space_value);
	}
	break;

	/*
	 *	.fill rep, size, value
	 *	repeat rep times: fill size bytes with (truncated) value
	 *	size must be between 1 and 8
	 */
   case	IFILL:
	shift;
	expr(locxp, val);
	if ( (locxp->e_xtype & XTYPE) != XABS)	/* tekmdp */
		yyerror("Fill repetition count not absolute");
	fill_rep = locxp->e_xvalue;
	shiftover(CM);
	expr(locxp, val);
	if ( (locxp->e_xtype & XTYPE) != XABS)	/* tekmdp */
		yyerror("Fill size not absolute");
	fill_size = locxp->e_xvalue;
	if (fill_size <= 0 || fill_size > 8)
		yyerror("Fill count not in in 1..8");
	shiftover(CM);
	expr(locxp, val);
	if (passno == 2 && (locxp->e_xtype & XTYPE) != XABS)	/* tekmdp */
		yyerror("Fill value not absolute");
	flushfield(NBPW/4);

	/*RAP001
		Update the location counter on both passes. Used to do it
		only for pass 1, causing a relocation problem. Whoever
		may have thought bwrite did update it (like Outb, outs, ..)
		but it doesn't, so it has to be done here.
	*/
	dotp->e_xvalue += fill_rep * fill_size;
	if (passno == 2) {
		while(fill_rep-- > 0)
			bwrite((char *)&locxp->e_xvalue, fill_size, txtfil);
	}
	break;

   case IASCII:		/* .ascii [ <stringlist> ] */
   case IASCIZ: 	/* .asciz [ <stringlist> ] */
	auxval = val;
	shift;
	/*
	 *	Code to consume a string list
	 *
	 *	stringlist: empty | STRING | stringlist STRING
	 */
	while (val == STRING){
		int	mystrlen;
		flushfield(NBPW/4);
		if (bitoff)
			dotp->e_xvalue++;
		stringp = (struct strdesc *)yylval;
		/*
		 *	utilize the string scanner cheat;
		 *	the scanner appended a null byte on the string,
		 *	but didn't charge it to sd_strlen
		 */
		mystrlen = stringp->sd_strlen;
		mystrlen += (auxval == IASCIZ) ? 1 : 0;
		if (passno == 2){
			if (stringp->sd_place & STR_CORE){
				outs(stringp->sd_string, mystrlen);
			} else {
				int	i, nread;
				fseek(strfile, stringp->sd_stroff, 0);
				for (i = 0; i < mystrlen;/*VOID*/){
					nread = fread(yytext, 1,
						min(mystrlen - i,
						  sizeof(yytext)), strfile);
					outs(yytext, nread);
					i += nread;
				}
			}
		} else {
			dotp->e_xvalue += mystrlen;
		}
		shift;		/*over the STRING*/
		if (val == CM)	/*could be a split string*/
			shift;
	}
	break;
	
   case IORG: 	/* .org <expr> */
	shift;
	expr(locxp, val);

	if ((locxp->e_xtype & XTYPE) == XABS)	/* tekmdp */
		orgwarn++;
	else if ((locxp->e_xtype & ~XXTRN) != dotp->e_xtype)
		yyerror("Illegal expression to set origin");
	space_value = locxp->e_xvalue - dotp->e_xvalue;
	if (space_value < 0)
		yyerror("Backwards 'org'");
	goto ospace;
	break;

/*
 *
 *	Process stabs.  Stabs are created only by the f77
 *	and the C compiler with the -g flag set.
 *	We only look at the stab ONCE, during pass 1, and
 *	virtually remove the stab from the intermediate file
 *	so it isn't seen during pass2.  This makes for some
 *	hairy processing to handle labels occuring in
 *	stab entries, but since most expressions in the
 *	stab are integral we save lots of time in the second
 *	pass by not looking at the stabs.
 *	A stab that is tagged floating will be bumped during
 *	the jxxx resolution phase.  A stab tagged fixed will
 *	not be be bumped.
 *
 *	.stab:	Old fashioned stabs
 *	.stabn: For stabs without names
 *	.stabs:	For stabs with string names
 *	.stabd: For stabs for line numbers or bracketing,
 *		without a string name, without
 *		a final expression.  The value of the
 *		final expression is taken to be  the current
 *		location counter, and is patched by the 2nd pass
 *
 *	.stab{<expr>,}*NCPName,<expr>, <expr>, <expr>, <expr>
 *	.stabn		 <expr>, <expr>, <expr>, <expr>
 *	.stabs   STRING, <expr>, <expr>, <expr>, <expr>
 *	.stabd		 <expr>, <expr>, <expr> # . 
 */
   case ISTAB: 
	yyerror(".stab directive no longer supported");
	goto errorfix;

  tailstab:
	expr(locxp, val);
	if (! (locxp->e_xvalue & STABTYPS)){
		yyerror("Invalid type in %s", stabname);
		goto errorfix;
	}
	stpt->s_ptype = locxp->e_xvalue;
	shiftover(CM);
	expr(locxp, val);
	stpt->s_other = locxp->e_xvalue;
	shiftover(CM);
	expr(locxp, val);
	stpt->s_desc = locxp->e_xvalue;
	shiftover(CM);
	exprisname = 0;
	expr(locxp, val);
	p = locxp->e_xname;
	if ((locxp->e_xtype&XTYPE)!=XUNDEF) {	/* 004 not forward reference */
		stpt->s_value = locxp->e_xvalue;
		stpt->s_index = dotp - usedot;
		if (exprisname){
			switch(stpt->s_ptype) {
				case N_GSYM:
				case N_FNAME:
				case N_RSYM:
				case N_SSYM:
				case N_LSYM:
				case N_PSYM:
				case N_BCOMM:
				case N_ECOMM:
				case N_LENG:
					stpt->s_tag = FIXEDSTAB;
					break;

				/* Special handling of LCSYM.  Although
				 * the named symbol representing the lcomm has
				 * been seen, the address (s_value field)
				 * will be changed in freezesymtab() once
				 * the start address of BSS is known.
				 * So treat this as a forward reference.
				 */
				case N_LCSYM:
					stpt->s_tag = FIXEDSTAB;
					stpt->s_dest = p;
					stpt->s_index = p->s_index;
					stpt->s_type = p->s_type | STABFLAG;
					stpt->s_value = 0;
					break;

				default:
					stpt->s_tag = FLOATINGSTAB;
					break;
			}
		} else
			stpt->s_tag = FIXEDSTAB;

	/* Forward reference. */
	/* Final address will be calculated in stabfix(), when */
	/* stpt->s_dest->s_value has been assigned its final */
	/* address. */
	} else {
		stpt->s_tag = FORWARDSTAB;
		stpt->s_dest = p;
		stpt->s_index = p->s_index;
		stpt->s_type = p->s_type | STABFLAG;

		/* e_xvalue may contain an offset, as in the case v.4+2; */
		/* although the address of v.4 is unknown here, the 2 */
		/* will be added in stabfix(). */
		stpt->s_value = locxp->e_xvalue;
	}
	/*
	 *	tokptr now points at one token beyond
	 *	the current token stored in val and yylval,
	 *	which are the next tokens after the end of
	 *	this .stab directive.  This next token must
	 *	be either a SEMI or NL, so is of width just
	 *	one.  Therefore, to point to the next token
	 *	after the end of this stab, just back up one..
	 */
	buildskip(stabstart, (bytetoktype *)tokptr - sizeof(bytetoktype));
	break;	/*end of the .stab*/

   case ISTABDOT:	
	stabname = ".stabd";
	stpt = (struct symtab *)yylval;
	/*
	 *	We clobber everything after the
	 *	.stabd and its pointer... we MUST
	 *	be able to get back to this .stabd
	 *	so that we can resolve its final value
	 */
	stabstart = tokptr;
	shift;		/*over the ISTABDOT*/
	if (passno == 1){
		expr(locxp, val);
		if (! (locxp->e_xvalue & STABTYPS)){
			yyerror("Invalid type in .stabd");
			goto errorfix;
		}
		stpt->s_ptype = locxp->e_xvalue;
		shiftover(CM);
		expr(locxp, val);
		stpt->s_other = locxp->e_xvalue;
		shiftover(CM);
		expr(locxp, val);
		stpt->s_desc = locxp->e_xvalue;
		/*
		 *
		 *	Now, clobber everything but the
		 *	.stabd pseudo and the pointer
		 *	to its symbol table entry
		 *	tokptr points to the next token,
		 *	build the skip up to this
		 */
		buildskip(stabstart, (bytetoktype *)tokptr - sizeof(bytetoktype));
	}
	/*
	 *	pass 1:	Assign a good guess for its position
	 *		(ensures they are sorted into right place)/
	 *	pass 2:	Fix the actual value
	 */
	stpt->s_value = dotp->e_xvalue;
	stpt->s_index = dotp - usedot;
	stpt->s_tag = FLOATINGSTAB;	/*although it has no effect in pass 2*/
	break;

   case ISTABNONE:
   case ISTABSTR:
        stabname = ((val == ISTABSTR) ? ".stabs" : ".stabn");
	auxval = val;
	if (passno == 2) goto errorfix;
	stpt = (struct symtab *)yylval;
	stabstart = tokptr;
	(bytetoktype *)stabstart -= sizeof(struct symtab *);
	(bytetoktype *)stabstart -= sizeof(bytetoktype);
	shift;
	if (auxval == ISTABSTR){
		stringp = (struct strdesc *)yylval;
		shiftover(STRING);
		stpt->s_name = (char *)stringp;
		/*
		 *	We want the trailing null included in this string.
		 *	We utilize the cheat the string scanner used,
		 *	and merely increment the string length
		 */
		stringp->sd_strlen += 1;
		shiftover(CM);
	} else {
		stpt->s_name = (char *)savestr("\0", 0, STR_BOTH);
	}
	goto tailstab;
	break;

   case ICOMM:		/* .comm  <name> , <expr> */
   case ILCOMM: 	/* .lcomm <name> , <expr> */
	auxval = val;
	shift;
	np = (struct symtab *)yylval;
	shiftover(NAME);
	shiftover(CM);
	expr(locxp, val);

	if ( (locxp->e_xtype & XTYPE) != XABS)	/* tekmdp */
		yyerror("comm size not absolute");
	if (passno == 1 && (np->s_type&XTYPE) != XUNDEF)
		yyerror("Redefinition of %s", FETCHNAME(np));
	if (passno==1) {
		np->s_value = locxp->e_xvalue;
		if (auxval == ICOMM)
			np->s_type |= XXTRN;
		else {
			np->s_type &= ~XTYPE;
			np->s_type |= XBSS;
		}
	}
	break;

   case IALIGN: 		/* .align <align_expr> [, <fill_expr> ]*/
   	{
		int	align_expr;
		int	fill_expr;

		/* Get the .align statement, and shift over .align.
		 */
		stpt = (struct symtab *)yylval;
		shift;

		/* Evaluate the alignment expression.
		 */
		expr(locxp, val);
		if ( (locxp->e_xtype & XTYPE) != XABS) {
			yyerror("Alignment expression not absolute");
			break ;
		} else if ( ! LEGAL_ALIGN_VAL(locxp->e_xvalue)) {
			yyerror("Illegal value for alignment expression");
			break ;
		} else if (locxp->e_xvalue > ALIGN_GUARANTEE) {
			/*
			 * If more alignment was specified, than is guaranteed
			 * by the loader, then issue a warning and convert
			 * the value to the maximum guaranteed by the loader.
			 */
			if (passno == 1){
				yywarning(
					".align %d is NOT preserved by the loader",
					locxp->e_xvalue);
				yywarning(".align %d converted to .align %d",
					locxp->e_xvalue,
					ALIGN_GUARANTEE);
			}
			align_expr = ALIGN_GUARANTEE;
		} else {
			/* Use the user specified alignment expression.
			 */
			align_expr = locxp->e_xvalue;
		}
		/* Evalutate the fill expression if there is one, else use 0.
		 */
		if (val == CM) {
			shiftover(CM);
			expr(locxp, val);
			if (passno == 2 && (locxp->e_xtype & XTYPE) != XABS) {
				yyerror("Fill expression not absolute");
				break;
			} else {
				fill_expr = locxp->e_xvalue;
			}
		} else {
			fill_expr = 0;	/* default to 0 */
		}
		/* Create the appropriate entry for the .align, so it
		 * can be used in calculating positions for symbols.
		 */
		jalign(align_expr, fill_expr, stpt);
	}
	break;

   case VINST0:		/* vector instructions w/o arguments*/
	np = (struct symtab *)yylval;
	incasetable = 0;
	shift;	
	vmod.imod = 0;
	if (val == VMODIFIER){
	  vmod.imod = yylval; /* save vector instr modifiers */
	  shift;              /* now bring in the first token for the arg list*/
	} /* end if VMODIFIER */
	format_vinst (np, &vmod, (struct arg *)0);
	break;

   case INST0: 		/* instructions w/o arguments*/
	incasetable = 0;
	insout(yyopcode, (struct arg *)0, 0);
	shift;	
	break;

   case INSTn:		/* instructions with arguments*/
   case IJXXX: 		/* UNIX style jump instructions */
   case VINSTn:		/* vector instructions with arguments*/

	if (val == VINSTn){
		np = (struct symtab *)yylval;
		auxval = val;
		shift;		/* bring in the first token for the arg list*/
	  	vmod.imod = 0;
	  	if (val == VMODIFIER){
	    	vmod.imod = yylval; /* save vector instr modifiers */
	    	shift;            /* now bring in the first token for the arg list*/
	  	} /* end if VMODIFIER */
		/*
	 	 *	Code to process an argument list
	 	 */
		ap = arglist;
		xp = explist;	

	} /* end if VINSTn */
	else {
		auxval = val;
		/*
	 	 *	Code to process an argument list
	 	 */
		ap = arglist;
		xp = explist;	

		shift;		/* bring in the first token for the arg list*/
	}
	
	for (argcnt = 1; argcnt <= 6; argcnt++, ap++){
		/*
		 *	code to process an argument proper
		 */
	    sawindex  = sawmul = sawsize = 0;
	    {
		switch(val) {

		   default:
		     disp:
			if( !(INTOKSET(val,
				 EBEGOPS
				+YUKKYEXPRBEG
				+SAFEEXPRBEG)) ) {
				ERROR("expression expected");
			}
			expr(ap->a_xp,val);
		     overdisp:
			if ( val == LP || sawsize){
				shiftover(LP);
				findreg(regno);
				shiftover(RP);
				ap->a_atype = ADISP;
				ap->a_areg1 = regno;
			} else {
				ap->a_atype = AEXP;
				ap->a_areg1 = 0;
			}
			goto index;

		   case SIZESPEC: 
		     sizespec:
			sawsize = yylval;
			shift;
			goto disp;

		   case REG:
		   case REGOP: 
			findreg(regno);
			ap->a_atype = AREG;
			ap->a_areg1 = regno;
			break;
		    
		   case VREG: 
			findvreg(regno);
			ap->a_atype = AVREG;
			ap->a_areg1 = regno;
			break;
		    
		   case MUL: 
			sawmul = 1;
			shift;
			if (val == LP) goto base;
			if (val == LITOP) goto imm;
			if (val == SIZESPEC) goto sizespec;
			if (INTOKSET(val,
				 EBEGOPS
				+YUKKYEXPRBEG
				+SAFEEXPRBEG)) goto disp;
			ERROR("expression, '(' or '$' expected");
			break;

		   case LP: 
		     base:
			shift;	/*consume the LP*/
			/*
			 *	hack the ambiguity of
			 *	movl (expr) (rn), ...
			 *	note that (expr) could also
			 *	be (rn) (by special hole in the
			 *	grammar), which we ensure
			 *	means register indirection, instead
			 *	of an expression with value n
			 */
			if (val != REG && val != REGOP){
				droppedLP = 1;
				val = exprparse(val, &(ap->a_xp));
				droppedLP = 0;
				goto overdisp;
			}
			findreg(regno);
			shiftover(RP);
			if (val == PLUS){
				shift;
				ap->a_atype = AINCR;
			} else
				ap->a_atype = ABASE;
			ap->a_areg1 = regno;
			goto index;

		   case LITOP: 
		      imm:
			shift;
			expr(locxp, val);
			ap->a_atype = AIMM;
			ap->a_areg1 = 0;
			ap->a_xp = locxp;
			goto index;

		   case MP: 
			shift;	/* -(reg) */
			findreg(regno);
			shiftover(RP);
			ap->a_atype = ADECR;
			ap->a_areg1 = regno;
	  index:			/*look for [reg] */
			if (val == LB){
				shift;
				findreg(regno);
				shiftover(RB);
				sawindex = 1;
				ap->a_areg2 = regno;
			}
			break;

		}	/*end of the switch to process an arg*/
	    }	/*end of processing an argument*/

	    if (sawmul){
			/*
			 * Make a concession for *(%r)
			 * meaning *0(%r) 
			 */
			if (ap->a_atype == ABASE) {
				ap->a_atype = ADISP;
				xp->e_xtype = XABS;
				xp->e_number = Znumber;
				xp->e_number.num_tag = TYPL;
				xp->e_xloc = 0;
				ap->a_xp = xp++;
			}
			ap->a_atype |= ASTAR;
			sawmul = 0;
	    }
	    if (sawindex){
		ap->a_atype |= AINDX;
		sawindex = 0;
	    }
	    ap->a_dispsize = sawsize == 0 ? d124 : sawsize;
		if (val != CM) break;
		shiftover(CM);
	}	/*processing all the arguments*/

	if (argcnt > 6){
		yyerror("More than 6 arguments");
		goto errorfix;
	}

 	if (auxval == VINSTn) {
 	  format_vinst (np, &vmod, arglist);
 	}
 	else {
		/*
	 	 *	See if this is a case instruction,
	 	 *	so we can set up tests on the following
	 	 *	vector of branch displacements
	 	 */
		if (yyopcode.Op_eopcode == CORE){
			switch(yyopcode.Op_popcode){
				case 0x8f:	/* caseb */
				case 0xaf:	/* casew */
				case 0xcf:	/* casel */
					incasetable++;
					break;
				default:
					incasetable = 0;
					break;
			}
		}

		insout(yyopcode, arglist,
					auxval == INSTn ? argcnt : - argcnt);
	}
	break;

   case IQUAD:		toconv = TYPQ;	goto bignumlist;
   case IOCTA:		toconv = TYPO;	goto bignumlist;

   case IFFLOAT:	toconv = TYPF;	goto bignumlist;
   case IDFLOAT:	toconv = TYPD;	goto bignumlist;
   case IGFLOAT:	toconv = TYPG;	goto bignumlist;
   case IHFLOAT:	toconv = TYPH;	goto bignumlist;
   bignumlist:	
	/*
	 *	eat a list of non 32 bit numbers.
	 *	IQUAD and IOCTA can, possibly, return
	 *	INT's, if the numbers are "small".
	 *
	 *	The value of the numbers is coming back
	 *	as an expression, NOT in yybignum.
	 */
	shift;	/* over the opener */
	if ((val == BIGNUM) || (val == INT)){
		do{
			if ((val != BIGNUM) && (val != INT)){
				ERROR(ty_float[toconv]
				   ? "floating number expected"
				   : "integer number expected" );
			}
			dotp->e_xvalue += ty_nbyte[toconv];
			if (passno == 2){
				bignumwrite(
					((struct exp *)yylval)->e_number,
					toconv);
			}
			xp = explist;
			shift;		/* over this number */
			if (auxval = (val == CM))
				shift;	/* over the comma */
		} while (auxval);	/* as long as there are commas */
	}
	break;
	/* end of the case for initialized big numbers */
    }	/*end of the switch for looking at each reserved word*/

	continue;

   errorfix: 
	/*
	 *	got here by either requesting to skip to the
	 *	end of this statement, or by erroring out and
	 *	wanting to apply panic mode recovery
	 */
	while (    (val != NL) 
		&& (val != SEMI) 
		&& (val != PARSEEOF)
	      ){
		shift;
	}
	if (val == NL)
		lineno++;
	shift;

    }	/*end of the loop to read the entire file, line by line*/

}	/*end of yyparse*/
	
format_vinst(stp, vmodp, ap)
     struct symtab        *stp;
     union  Vinst_mod     *vmodp;
     struct arg           *ap;

/* This routine is called to reformat the vector instruction from the
 * assembler notation format into the vector instruction format as
 * specified in the instrs opcode table. It builds vector instruction
 * control word, sets the appropriate qualifier fields in the control 
 * word, sets the vector register fields and the compare/convert codes
 * in the control word, reshuffles the operands, and then calls the 
 * insout routine to output the instruction to the object file, in a
 * normal way. The routine is driven by the vinst_fmt table, whose entries
 * represent groups of like vector instructions.
 */
{
  reg int          i,j;
  int              arg_num;
  int              avalue;
  int              argtype;
  struct Opcode	   opcode;
  u_char           ctrlcode;
  struct arg       *vap;                /* first free vinst argument */
  struct Vinst_fmt fmt;
  struct Vinst_arg vi_arg;
  struct Ctrl_word ctrl_word, *ctrl_word_ptr = &ctrl_word;
  int              mod_length;
  char             ch;

  /* Pick up info from symbol table */
  opcode.Op_popcode = ((Iptr)stp)->i_popcode;
  opcode.Op_eopcode = ((Iptr)stp)->i_eopcode;
  ctrlcode = ((Iptr)stp)->i_ctrlcode;
  fmt = vinst_fmt[((Iptr)stp)->s_format];
  
  /* Following is a kluge to allow vector instructions use the
   * same mapping mechanism from opcode to the symbol table entry,
   * as regular instructions */
  itab[opcode.Op_eopcode][opcode.Op_popcode] = (Iptr) stp;

  vap = varglist;
  
  *(int *)ctrl_word_ptr = 0;

  for (i = 0; i < fmt.vi_nargs; i++) /* process args according to format */
    {
      vi_arg = fmt.vi_args[i];
      switch (vi_arg.va_tag) {

      default:
	break; /* argument type not defined */

      case VOPND:
	/* Move source argument into specified vector instruction operand */
	arg_num = vi_arg.va_args[0] - 1;
	varglist[i] = arglist[arg_num];
	argtype = fetcharg(stp, i);
        if ((argtype & TYPMASK) == TYPF4) { 
	  /* Special check for vmergef */
	  if ((varglist[i].a_atype & AMASK) != AIMM)
	    yyerror ("Illegal mode");
	}
        varglist[i].a_atype |= AVECT;
	break;

      case VLTRL:
	/* Generate literal as the instruction's first operand */
	xp->e_xvalue = ctrlcode;
	xp->e_number.num_tag = TYPL;
	xp->e_xloc = 0;
	xp->e_xtype = XABS; 

	vap->a_atype = AIMM;
	vap->a_areg1 = 0;
	if (xp >= &explist[NEXP])
	  yyerror("Too many expressions; try simplyfing");
	else
	  vap->a_xp = xp++;
	break;

      case VCTRL:
	/* Generate control word */
	for (j = 0; j < 3; j++) {
	  /* Set va, vb, vc registers in the control word */
	  if (vi_arg.va_args[j] != 0) {
	    arg_num = vi_arg.va_args[j];
	    if (arg_num == VCODE)
	      avalue = ctrlcode; /* set cmp/cvt code */
	    else {
	      arg_num--;
	      argtype = arglist[arg_num].a_atype;
	      if (argtype != AVREG)
		yyerror("arg %d, must specify a vector register", arg_num + 1);
	      else 
		avalue = arglist[arg_num].a_areg1;
	    }
	    switch (j) {
	    case VA:
	      ctrl_word.va_bits = avalue;
	      break;
	    case VB:
	      ctrl_word.vb_bits = avalue;
	      break;
	    case VC:
	      ctrl_word.vc_bits = avalue;
	      break;
	    } /* end switch */
	  } /* end if */
	} /* end for */
	break;

      } /* end switch */
    } /* end for */

  /* Process vector instruction modifiers into control word */
  mod_length = vmodp->mod_value.mod_length;

  if (fmt.vi_args[0].va_tag == VCTRL) { 
    ctrl_word.mod_bits |= fmt.vi_mod_dflt;
    if (mod_length > 0) {
      for (i = 1; i <= mod_length; i++) {
	switch (ch = vmodp->mod_value.mod_ch[i-1]) {
	case 'm':
	  if ((fmt.vi_mod_mask & MI) != 0)
	    ctrl_word.mod_bits |= mi_bit;
	  else
	    goto moderror;
	  break;
	case 'u':
	case 'v':
	  if ((fmt.vi_mod_mask & EXC) != 0)
	    ctrl_word.mod_bits |= exc_bit;
	  else
	    goto moderror;
	  break;
	case '0':
	  if ((fmt.vi_mod_mask & MTF) != 0) {
	    if ((fmt.vi_mod_mask & MOE) != 0)
	      ctrl_word.mod_bits |= moe_bit;
	  }
	  else
	    goto moderror;
	  break;
	case '1':
	  if ((fmt.vi_mod_mask & MTF) != 0) {
	    if ((fmt.vi_mod_mask & MOE) != 0)
	      ctrl_word.mod_bits |= moe_bit;
	  ctrl_word.mod_bits |= mtf_bit;
	  }
	  else
	    goto moderror;
	  break;
	default:
	  break;
	} /* end switch */

	continue;

      moderror:
	yyerror("vector opcode qualifier %c is not valid for this opcode", ch);
      } /* end for */
    } /* end if */

    /* Finally set up control word as instruction's first operand */
    xp->e_xtype = XABS; 
    xp->e_xvalue = *(int *)ctrl_word_ptr;
    xp->e_number.num_tag = TYPW;
    xp->e_xloc = 0;
    
    vap->a_atype = AVECT;
    vap->a_areg1 = 0;
    if (xp >= &explist[NEXP])
      yyerror("Too many expressions; try simplyfing");
    else
      vap->a_xp = xp++;
  } /* end if */

  else if (mod_length > 0)
    yyerror("Vector opcode qualifiers are not valid for this opcode");

  /* Finally output this instruction */
  insout(opcode, varglist, fmt.vi_nargs);
}
	
/*
 *	Process a register declaration of the form
 *	% <expr>
 *
 *	Note:
 *		The scanner has already processed funny registers of the form
 *	%dd[+-]*, where dd is a decimal number in the range 00 to 15 (optional
 *	preceding zero digit).  If there was any space between the % and
 *	the digit, the scanner wouldn't have recognized it, so we
 *	hack it out here.
	
/*
 *	Process a register declaration of the form
 *	% <expr>
 *
 *	Note:
 *		The scanner has already processed funny registers of the form
 *	%dd[+-]*, where dd is a decimal number in the range 00 to 15 (optional
 *	preceding zero digit).  If there was any space between the % and
 *	the digit, the scanner wouldn't have recognized it, so we
 *	hack it out here.
 */
inttoktype funnyreg(val, regnoback)	/*what the read head will sit on*/
	inttoktype	val;		/*what the read head is sitting on*/
	int	*regnoback;		/*call by return*/
{
	reg	struct	exp *locxp;
		struct	exp *loc1xp;
		struct	exp **ptrloc1xp = & loc1xp;

	expr(locxp, val);	/*and leave the current read head with value*/
	if ( (passno == 2) &&
	    (   (locxp->e_xtype & XTYPE) != XABS
	     || (locxp->e_xvalue < 0)
	     || (locxp->e_xvalue >= 16)
	    )
	  ){
		yyerror("Illegal register");
		return(0);
	}
	*regnoback = locxp->e_xvalue;
	return(val);
} 
/*
 *	Shift over error
 */
shiftoerror(token)
	int	token;
{
	char	*tok_to_name();
	yyerror("%s expected", tok_to_name(token));
}

/*VARARGS1*/
yyerror(s, a1, a2,a3,a4,a5)
	char	*s;
{

#define	sink stdout

	if (anyerrs == 0 && anywarnings == 0 && ! silent) 
		fprintf(sink, "Assembler:\n");
	anyerrs++;
	if (silent)
		return;
	fprintf(sink, "\"%s\", line %d: ", dotsname, lineno);
	fprintf(sink, s, a1, a2,a3,a4,a5);
	fprintf(sink, "\n");
#undef sink
}

/*VARARGS1*/
yywarning(s, a1, a2,a3,a4,a5)
	char	*s;
{
#define	sink stdout
	if (anyerrs == 0 && anywarnings == 0 && ! silent) 
		fprintf(sink, "Assembler:\n");
	anywarnings++;
	if (silent)
		return;
	fprintf(sink, "\"%s\", line %d: WARNING: ", dotsname, lineno);
	fprintf(sink, s, a1, a2,a3,a4,a5);
	fprintf(sink, "\n");
#undef sink
}
