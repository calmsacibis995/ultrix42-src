#ifndef lint
static	char	*sccsid = "@(#)table.c	4.2	(ULTRIX)	8/13/90";
#endif lint

/************************************************************************
 *		Modification History
 *
 *	"@(#)table.c	1.1 (Berkeley) 12/15/82"
 * 	
 *	Jon Reeves, 90-Feb-01
 * 009-	Changed unsigned to float conversions to call ZA instead of
 *	trying to do it inline (which got it wrong before).
 *
 *	Lu Anne Van de Pas, 02-MAR-86
 * 008- Added table entries to support doing f floating arithmetic when
 *	the fflag is set.  The new need FLOATFLG says to use this production
 *	is the flag is set.
 *
 *	Rich Phillips, 17-Aug-84
 * 007- Add a special character to UNARY CALL so code for UNARY CALL to _asm
 *	will be put out using in.asminfo char ptr set up by cgram and scan.
 *	(code put out using outasm in match.c).
 *
 *	Rich Phillips, 17-July-84
 * 006- Do an ext when the result of a store into a field is required. The
 *	source used to be used as the result.
 *
 *      Rich Phillips, David Ballenger, 29-May-84
 * 005- Add two SCONV productions to handle signed and unsigned conversions
 *      from an integer constant pointer to a shorter type using a cast.
 *
 *	Stephen Reilly, 15-Jan-84
 * 004- Add two addition productions for the unsigned cases of error 003
 *
 *	Stephen Reilly, 20-Jan-84
 * 003- Add a two new productions for int op= real
 *
 *	Stephen Reilly, 06-Dec-83
 * 002- Fix the problem when floating point arithmetic is not done in
 *	double
 *
 *	Stephen Reilly, 23-Oct-83:
 * 001- New -M flag was added so we must add special zzzcode to 
 *	determine which of the float instructions will be used 
 *	( gfloat or dfloat )
 *
 ************************************************************************/

/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
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

# include "mfile2"

# define WPTR TPTRTO|TINT|TLONG|TFLOAT|TDOUBLE|TPOINT|TUNSIGNED|TULONG
# define AWD SNAME|SOREG|SCON|STARNM|STARREG
/* tbl */
# define ANYSIGNED TPOINT|TINT|TLONG|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TULONG|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TWORD TINT|TUNSIGNED|TPOINT|TLONG|TULONG
# define NIAWD SNAME|SCON|STARNM
/* tbl */

struct optab  table[] = {

PCONV,	INAREG|INTAREG,
	SAREG|AWD,	TCHAR|TSHORT,
	SANY,	TPOINT,
		NAREG|NASL,	RESC1,
		"	cvtZLl	AL,A1\n",

PCONV,	INAREG|INTAREG,
	SAREG|AWD,	TUCHAR|TUSHORT,
	SANY,	TPOINT,
		NAREG|NASL,	RESC1,
		"	movzZLl	AL,A1\n",

	/* the following entry is to fix a problem with
	   the manner that the first pass handles the
	   type of a shift expression                 */
PCONV,	INAREG|INTAREG,
	SAREG|AWD,	TINT|TUNSIGNED,
	SANY,	TPOINT,
		NAREG|NASL,	RLEFT,
		"",

SCONV,	INTAREG|FORCC,
	SAREG,	TDOUBLE,
	SANY,	TDOUBLE,
		0,	RLEFT,
		"",

SCONV,	INTAREG|FORCC,		    /*vdp008 for f floating function returns*/
	SAREG|AWD,	ANYSIGNED,
	SANY,	TFLOAT,
	FLOATFLG|NAREG|NASL,	RESC1|RESCC,
		"	cvtZLf	AL,A1\n",		

#ifdef FORT
SCONV,	INTAREG|FORCC,
	SAREG|AWD,	TUCHAR|TUSHORT,
	SANY,	TFLOAT,
		NAREG|NASL,	RESC1|RESCC,
		"	movzZLl	AL,A1\n	cvtlf	A1,TA1\n",
#endif

SCONV,	INTAREG|FORCC,
	SAREG|AWD,	ANYSIGNED|TFLOAT,
	SANY,	TFLOAT|TDOUBLE,
		NAREG|NASL,	RESC1|RESCC,
		"	cvtZLZV	AL,A1\n",			/* slr001 */
				

SCONV,	INTAREG|FORCC,
	SAREG|AWD,	TUCHAR|TUSHORT,
	SANY,	TFLOAT|TDOUBLE,
		NAREG|NASL,	RESC1|RESCC,
		"	movzZLl	AL,A1\n	cvtlZW	A1,A1\n",    /* slr001 */
						/* vdp008 - ZW becomes 'f' if
						 * fflag otherwise 'd' or 'g'
						 */

SCONV,	INTAREG|FORCC,		/*vdp008 return 'f' floating if fflag*/
	SAREG|AWD,	TDOUBLE,
	SANY,	TFLOAT,
		FLOATFLG|NAREG|NASL,	RESC1|RESCC,
		"	cvtdf	AL,A1\n",

SCONV,	INTAREG|FORCC,
	SAREG|AWD,	TFLOAT|TDOUBLE,
	SANY,	ANYFIXED,
		NAREG|NASL,	RESC1|RESCC,
		"	cvtZLZF	AL,A1\n",

/* start of RAP005 */

SCONV,	INTAREG|FORCC,
	SCON,	TPOINT,
	SANY,	TUCHAR|TUSHORT,
		NAREG|NASL,	RESC1|RESCC,
		"	moval	CL,A1\n	movzZRl	A1,A1\n",

SCONV,	INTAREG|FORCC,
	SCON,	TPOINT,
	SANY,	TCHAR|TSHORT,
		NAREG|NASL,	RESC1|RESCC,
		"	moval	CL,A1\n	cvtZRl	A1,A1\n",

/* End of RAP005 */

SCONV,	INTAREG|FORCC,
	SAREG|SNAME|SCON|STARNM,	TANY,
	SANY,	ANYUSIGNED,
		NAREG|NASL,	RESC1|RESCC,
		"	movzZRl	AL,A1\n",

SCONV,	INTAREG|FORCC,
	SSOREG,	TANY,
	SANY,	ANYUSIGNED,
		NAREG|NASL,	RESC1|RESCC,
		"	movzZRl	AL,A1\n",

/* 009 */
SCONV,	INTAREG|FORCC,
	SAREG|AWD,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	ZA\n",


INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TWORD,
		0,	RNOP,
		"	.long	CL\n",

INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TSHORT|TUSHORT,
		0,	RNOP,
		"	.word	CL\n",

INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TCHAR|TUCHAR,
		0,	RNOP,
		"	.byte	CL\n",

	/* for the use of fortran only */

GOTO,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jbr	CL\n",

GOTO,	FOREFF,
	AWD,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp	*AL\n",

GOTO,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp	(AL)\n",

STARG,	FORARG,
	SCON|SOREG,	TANY,
	SANY,	TANY,
		NTEMP+2*NAREG,	RESC3,
		"ZS",

STASG,	FORARG,
	SNAME|SOREG,	TANY,
	SCON|SAREG,	TANY,
		0,	RNULL,
		"	subl2	ZT,sp\nZS",

STASG,	FOREFF,
	SNAME|SOREG,	TANY,
	SCON|SAREG,	TANY,
		0,	RNOP,
		"ZS",

STASG,	INAREG,
	SNAME|SOREG,	TANY,
	SCON,	TANY,
		NAREG,	RESC1,
		"ZS	movl	AR,A1\n",

STASG,	INAREG,
	SNAME|SOREG,	TANY,
	SAREG,	TANY,
		0,	RRIGHT,
		"	pushl	AR\nZS	movl	(sp)+,AR\n",

FLD,	INAREG|INTAREG,
	SANY,	TANY,
	SFLD,	ANYSIGNED,
		NAREG|NASR,	RESC1,
		"	extv	$H,$S,AR,A1\n",

FLD,	INAREG|INTAREG,
	SANY,	TANY,
	SFLD,	ANYUSIGNED,
		NAREG|NASR,	RESC1,
		"	extzv	$H,$S,AR,A1\n",

FLD,	FORARG,
	SANY,	TANY,
	SFLD,	ANYSIGNED,
		0,	RNULL,
		"	extv	$H,$S,AR,-(sp)\n",

FLD,	FORARG,
	SANY,	TANY,
	SFLD,	ANYUSIGNED,
		0,	RNULL,
		"	extzv	$H,$S,AR,-(sp)\n",

OPLOG,	FORCC,
	SAREG|AWD,	TWORD,
	SAREG|AWD,	TWORD,
		0,	RESCC,
		"	cmpl	AL,AR\nZP",

OPLOG,	FORCC,
	SAREG|AWD,	TSHORT|TUSHORT,
	SAREG|AWD,	TSHORT|TUSHORT,
		0,	RESCC,
		"	cmpw	AL,AR\nZP",

OPLOG,	FORCC,
	SAREG|AWD,	TCHAR|TUCHAR,
	SAREG|AWD,	TCHAR|TUCHAR,
		0,	RESCC,
		"	cmpb	AL,AR\nZP",

OPLOG,	FORCC,
	SAREG|AWD,	TSHORT|TUSHORT,
	SSCON,	TANY,
		0,	RESCC,
		"	cmpw	AL,AR\nZP",

OPLOG,	FORCC,
	SAREG|AWD,	TCHAR|TUCHAR,
	SCCON,	TANY,
		0,	RESCC,
		"	cmpb	AL,AR\nZP",

OPLOG,	FORCC,
	SAREG|AWD,	TDOUBLE,
	SAREG|AWD,	TDOUBLE,
		0,	RESCC,
		"	cmpZV	AL,AR\nZP",			/* slr001 */

OPLOG,	FORCC,
	SAREG|AWD,	TDOUBLE,
	SAREG|AWD,	TFLOAT,
		NAREG|NASR,	RESCC,
		"	cvtfZV	AR,A1\n	cmpd	AL,A1\nZP",	/* slr001 */

OPLOG,	FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TDOUBLE,
		NAREG|NASL,	RESCC,
		"	cvtfZV	AL,A1\n	cmpZV	A1,AR\nZP",	/* slr001 */

OPLOG,	FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TFLOAT,
		0,	RESCC,
		"	cmpf	AL,AR\nZP",

CCODES,	INAREG|INTAREG,
	SANY,	TANY,
	SANY,	TANY,
		NAREG,	RESC1,
		"	movl	$1,A1\nZN",

UNARY CALL,	INTAREG,
	SCON,	TANY,
	SANY,	TWORD|TCHAR|TUCHAR|TSHORT|TUSHORT|TFLOAT|TDOUBLE,
		NAREG|NASL,	RESC1, /* should be register 0 */
		"R	calls	ZC,CL\n", /*RAP007 the R does it */

UNARY CALL,	INTAREG,
	SAREG,	TANY,
	SANY,	TWORD|TCHAR|TUCHAR|TSHORT|TUSHORT|TFLOAT|TDOUBLE,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	calls	ZC,(AL)\n",

UNARY CALL,	INAREG|INTAREG,
	SNAME,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* really reg 0 */
		"	calls	ZC,*AL\n",

UNARY CALL,	INAREG|INTAREG,
	SSOREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* really reg 0 */
		"	calls	ZC,*AL\n",

ASG RS,	INAREG|FOREFF|FORCC,
	SAREG,	TWORD,
	SCON,	TINT,
		0,	RLEFT|RESCC,
		"	extzv	AR,ZU,AL,AL\n",

ASG RS,	INAREG|FOREFF|FORCC,
	SAREG,	TWORD,
	SAREG,	ANYFIXED,
		NAREG,	RLEFT|RESCC,
		"	subl3	AR,$32,A1\n	extzv	AR,A1,AL,AL\n",

ASG RS,	INAREG|FOREFF|FORCC,
	SAREG,	TWORD,
	SAREG|AWD,	TWORD,
		NAREG,	RLEFT|RESCC,
		"	subl3	AR,$32,A1\n	extzv	AR,A1,AL,AL\n",

RS,	INAREG|INTAREG|FORCC,
	SAREG,	TWORD,
	SCON,	TINT,
		NAREG|NASL,	RESC1|RESCC,
		"	extzv	AR,ZU,AL,A1\n",

ASG LS,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SAREG|NIAWD,	ANYSIGNED|ANYUSIGNED,
		0,	RLEFT|RESCC,
		"	ashl	AR,AL,AL\n",

ASG LS,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SSOREG,	ANYSIGNED|ANYUSIGNED,
		0,	RLEFT|RESCC,
		"	ashl	AR,AL,AL\n",

ASG LS,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SOREG,	ANYSIGNED|ANYUSIGNED,
		NAREG,	RLEFT|RESCC,
		"	ZB	AR,A1\n	ashl	A1,AL,AL\n",

LS,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TWORD,
	SAREG|NIAWD,	ANYSIGNED|ANYUSIGNED,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	ashl	AR,AL,A1\n",

LS,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TWORD,
	SSOREG,	ANYSIGNED|ANYUSIGNED,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	ashl	AR,AL,A1\n",

LS,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TWORD,
	SOREG,	ANYSIGNED|ANYUSIGNED,
		NAREG|NASR,	RESC1|RESCC,
		"	ZB	AR,A1\n	ashl	A1,AL,A1\n",

INCR,	FOREFF,
	AWD,	TANY,
	SCON,	TANY,
		0,	RLEFT,
		"	ZE\n",

DECR,	FOREFF,
	AWD,	TANY,
	SCON,	TANY,
		0,	RLEFT,
		"	ZE\n",

INCR,	FOREFF,
	SAREG,	TWORD,
	SCON,	TANY,
		0,	RLEFT,
		"	ZE\n",

DECR,	FOREFF,
	SAREG,	TWORD,
	SCON,	TANY,
		0,	RLEFT,
		"	ZE\n",

/* jwf INCR and DECR for SAREG TCHAR|TSHORT matched by ASG PLUS etc */

INCR,	INAREG|INTAREG,
	AWD,	TANY,
	SCON,	TANY,
		NAREG,	RESC1,
		"	ZD\n",

DECR,	INAREG|INTAREG,
	AWD,	TANY,
	SCON,	TANY,
		NAREG,	RESC1,
		"	ZD\n",

INCR,	INAREG|INTAREG,
	SAREG,	TWORD,
	SCON,	TANY,
		NAREG,	RESC1,
		"	ZD\n",

DECR,	INAREG|INTAREG,
	SAREG,	TWORD,
	SCON,	TANY,
		NAREG,	RESC1,
		"	ZD\n",

ASSIGN,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TANY,
	SAREG|AWD,	TANY,
		0,	RLEFT|RRIGHT|RESCC,
		"	ZA\n",

/*	RAP006 Start 	*/		

ASSIGN,	FOREFF,
	SFLD,	TANY,
	SAREG|AWD,	TWORD,
		0,	RNULL,
		"	insv	AR,$H,$S,AL\n",

ASSIGN,	INAREG,
	SFLD,	ANYSIGNED,
	SAREG|AWD,	TWORD,
		NAREG,	RESC1,
		"	insv	AR,$H,$S,AL\n	extv	$H,$S,AL,A1\n",

ASSIGN,	INAREG,
	SFLD,	ANYUSIGNED,
	SAREG|AWD,	TWORD,
		NAREG,	RESC1,
		"	insv	AR,$H,$S,AL\n	extzv	$H,$S,AL,A1\n",


/*	RAP006 End	*/
ASSIGN,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SFLD,	ANYSIGNED,
		0,	RLEFT|RESCC,
		"	extv	$H,$S,AR,AL\n",

ASSIGN,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SFLD,	ANYUSIGNED,
		0,	RLEFT|RESCC,
		"	extzv	$H,$S,AR,AL\n",

/* dummy UNARY MUL entry to get U* to possibly match OPLTYPE */
UNARY MUL,	FOREFF,
	SCC,	TANY,
	SCC,	TANY,
		0,	RNULL,
		"	HELP HELP HELP\n",

REG,	INTEMP,
	SANY,	TANY,
	SAREG,	TDOUBLE,
		2*NTEMP,	RESC1,
		"	movZV	AR,A1\n",			/* slr001 */

REG,	INTEMP,
	SANY,	TANY,
	SAREG,	TANY,
		NTEMP,	RESC1,
		"	movZF	AR,A1\n",

#ifdef FORT
REG,	FORARG,
	SANY,	TANY,
	SAREG,	TFLOAT,
		0,	RNULL,
		"	cvtfd	AR,-(sp)\n",

REG,	FORARG,
	SANY,	TANY,
	SAREG,	TDOUBLE,
		0,	RNULL,
		"	movZR	AR,-(sp)\n",
#endif

OPLEAF,	FOREFF,
	SANY,	TANY,
	SAREG|AWD,	TANY,
		0,	RLEFT,
		"",

OPLTYPE,	INAREG|INTAREG,
	SANY,	TANY,
	SANY,	TFLOAT|TDOUBLE,
		2*NAREG|NASR,	RESC1,
		"	ZA\n",

OPLTYPE,	INAREG|INTAREG,
	SANY,	TANY,
	SANY,	TANY,
		NAREG|NASR,	RESC1,
		"	ZA\n",

OPLTYPE,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		0,	RESCC,
		"	tstZR	AR\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TWORD,
		0,	RNULL,
		"	pushl	AR\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TCHAR|TSHORT,
		0,	RNULL,
		"	cvtZRl	AR,-(sp)\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TUCHAR|TUSHORT,
		0,	RNULL,
		"	movzZRl	AR,-(sp)\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TDOUBLE,
		0,	RNULL,
		"	movZV	AR,-(sp)\n",			/* slr001 */

OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TFLOAT,
		0,	RNULL,
		"	cvtfZV	AR,-(sp)\n",			/* slr001 */

UNARY MINUS,	INTAREG|FORCC,					/* vdp008 */ 
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG|TDOUBLE|TFLOAT,
	SANY,	TANY,
		FLOATFLG|NAREG|NASL,	RESC1|RESCC,
		"	mnegZL	AL,A1\n",

UNARY MINUS,	INTAREG|FORCC,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG|TDOUBLE,
	SANY,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	mnegZL	AL,A1\n",

COMPL,	INTAREG|FORCC,
	SAREG|AWD,	TINT|TUNSIGNED,
	SANY,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	mcomZL	AL,A1\n",

COMPL,	INTAREG|FORCC,
	SAREG|AWD,	ANYSIGNED|ANYUSIGNED,
	SANY,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	cvtZLl	AL,A1\n	mcoml	A1,A1\n",

AND,	FORCC,
	SAREG|AWD,	TWORD,
	SCON,	TWORD,
		0,	RESCC,
		"	bitl	ZZ,AL\n",

AND,	FORCC,
	SAREG|AWD,	TSHORT|TUSHORT,
	SSCON,	TWORD,
		0,	RESCC,
		"	bitw	ZZ,AL\n",

AND,	FORCC,
	SAREG|AWD,	TCHAR|TUCHAR,
	SCCON,	TWORD,
		0,	RESCC,
		"	bitb	ZZ,AL\n",

ASG AND,	INAREG|FOREFF|FORCC,
	SAREG,	TWORD,
	SCON,	TWORD,
		0,	RLEFT|RESCC,
		"	bicl2	AR,AL\n",

ASG OPMUL,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
		0,	RLEFT|RESCC,
		"	OL2	AR,AL\n",

OPMUL,	INAREG|INTAREG|FORCC,
	STAREG,	TINT|TUNSIGNED|TLONG|TULONG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
		0,	RLEFT|RESCC,
		"	OL2	AR,AL\n",

OPMUL,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OL3	AR,AL,A1\n",

ASG MOD,	INAREG|INTAREG|FOREFF|FORCC,
	SAREG,	TINT|TUNSIGNED|TLONG|TULONG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
		NAREG,	RLEFT|RESCC,
		"	divl3	AR,AL,A1\n	mull2	AR,A1\n	subl2	A1,AL\n",

MOD,	INAREG|INTAREG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
		NAREG,	RESC1,
		"	divl3	AR,AL,A1\n	mull2	AR,A1\n	subl3	A1,AL,A1\n",

ASG PLUS,	INAREG|FOREFF|FORCC,
	SAREG,	TPOINT|TINT|TLONG|TUNSIGNED|TULONG,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	incZL	AL\n",

ASG PLUS,	INAREG|FOREFF|FORCC,
	AWD,	ANYSIGNED|ANYUSIGNED,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	incZL	AL\n",

ASG PLUS,	INAREG|FOREFF|FORCC,
	SAREG,	TSHORT|TCHAR,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	incZL	AL\n	cvtZLl	AL,AL\n",

ASG PLUS,	INAREG|FOREFF|FORCC,
	SAREG,	TUSHORT|TUCHAR,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	incZL	AL\n	movzZLl	AL,AL\n",

ASG MINUS,	INAREG|FOREFF|FORCC,
	SAREG,	TPOINT|TINT|TLONG|TUNSIGNED|TULONG,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	decZL	AL\n",

ASG MINUS,	INAREG|FOREFF|FORCC,
	AWD,	ANYSIGNED|ANYUSIGNED,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	decZL	AL\n",

ASG MINUS,	INAREG|FOREFF|FORCC,
	SAREG,	TSHORT|TCHAR,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	decZL	AL\n	cvtZLl	AL,AL\n",

ASG MINUS,	INAREG|FOREFF|FORCC,
	SAREG,	TUSHORT|TUCHAR,
	SONE,	TINT|TLONG,
		0,	RLEFT|RESCC,
		"	decZL	AL\n	movzZLl	AL,AL\n",

PLUS,	INAREG|INTAREG|FORCC,
	STAREG,	TWORD,
	SONE,	TWORD,
		0,	RLEFT|RESCC,
		"	incZL	AL\n",

PLUS,	INAREG|INTAREG|FORCC,
	STAREG,	TSHORT|TCHAR,
	SONE,	TWORD,
		0,	RLEFT|RESCC,
		"	incZL	AL\n	cvtZLl	AL,AL\n",

PLUS,	INAREG|INTAREG|FORCC,
	STAREG,	TUSHORT|TUCHAR,
	SONE,	TWORD,
		0,	RLEFT|RESCC,
		"	incZL	AL\n	movzZLl	AL,AL\n",

MINUS,	INAREG|INTAREG|FORCC,
	STAREG,	TWORD,
	SONE,	TWORD,
		0,	RLEFT|RESCC,
		"	decZL	AL\n",

MINUS,	INAREG|INTAREG|FORCC,
	STAREG,	TSHORT|TCHAR,
	SONE,	TWORD,
		0,	RLEFT|RESCC,
		"	decZL	AL\n	cvtZLl	AL,AL\n",

MINUS,	INAREG|INTAREG|FORCC,
	STAREG,	TUSHORT|TUCHAR,
	SONE,	TWORD,
		0,	RLEFT|RESCC,
		"	decZL	AL\n	movzZLl	AL,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SAREG|AWD,	TWORD,
		0,	RLEFT|RESCC,
		"	OL2	AR,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,	TWORD,
	SAREG,	TSHORT|TUSHORT|TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"	OL2	AR,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	AWD,	TSHORT|TUSHORT,
	SAREG|AWD,	TSHORT|TUSHORT,
		0,	RLEFT|RESCC,
		"	OW2	AR,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	AWD,	TSHORT|TUSHORT,
	SSCON,	TWORD,
		0,	RLEFT|RESCC,
		"	OW2	AR,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	AWD,	TCHAR|TUCHAR,
	SAREG|AWD,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"	OB2	AR,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	AWD,	TCHAR|TUCHAR,
	SCCON,	TWORD,
		0,	RLEFT|RESCC,
		"	OB2	AR,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,	TSHORT,
	SAREG|AWD,	ANYFIXED,
		0,	RLEFT|RESCC,
		"	OW2	AR,AL\n	cvtZLl	AL,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,	TUSHORT,
	SAREG|AWD,	ANYFIXED,
		0,	RLEFT|RESCC,
		"	OW2	AR,AL\n	movzZLl	AL,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,	TCHAR,
	SAREG|AWD,	ANYFIXED,
		0,	RLEFT|RESCC,
		"	OB2	AR,AL\n	cvtZLl	AL,AL\n",

ASG OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,	TUCHAR,
	SAREG|AWD,	ANYFIXED,
		0,	RLEFT|RESCC,
		"	OB2	AR,AL\n	movzZLl	AL,AL\n",

OPSIMP,	INAREG|INTAREG|FORCC,
	STAREG,	ANYFIXED,
	SAREG|AWD,	TWORD,
		0,	RLEFT|RESCC,
		"	OL2	AR,AL\n",

OPSIMP,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TWORD,
	SAREG|AWD,	TWORD,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OL3	AR,AL,A1\n",

ASG OPFLOAT,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TDOUBLE,
	SAREG|AWD,	TDOUBLE,
		0,	RLEFT|RESCC,
		"	OD2	AR,AL\n",

ASG OPFLOAT,	INAREG|FOREFF|FORCC,			/* SLR003 */
	SAREG|AWD,	ANYSIGNED,			/* SLR004 */
	SAREG|AWD,	TDOUBLE,
		NAREG,	RLEFT|RESC1|RESCC,
		"	cvtZLZV	AL,A1\n	OD2	AR,A1\n	cvtZVZL	A1,AL\n", 

ASG OPFLOAT,	INAREG|FOREFF|FORCC,			/* SLR004 */
	SAREG|AWD,	TUCHAR|TUSHORT,
	SAREG|AWD,	TDOUBLE,
		NAREG,	RLEFT|RESC1|RESCC,
		"	movzZLl	AL,A1\n	cvtlZV	A1,A1\n	OD2	AR,A1\n	cvtZVZL	A1,AL\n", 

ASG OPFLOAT,	INAREG|FOREFF|FORCC,			/* vdp008 */
	SAREG|AWD,	ANYSIGNED,			/* vdp008 */
	SAREG|AWD,	TFLOAT,
	FLOATFLG|NAREG,	RLEFT|RESC1|RESCC,
		"	cvtZLf	AL,A1\n	OF2	AR,A1\n	cvtfZL	A1,AL\n",

ASG OPFLOAT,	INAREG|FOREFF|FORCC,			/* SLR003 */
	SAREG|AWD,	ANYSIGNED|TUNSIGNED|TULONG,	/* SLR004 */
	SAREG|AWD,	TFLOAT,
	2*NAREG,	RLEFT|RESC1|RESCC,
		"	cvtZLZV	AL,A1\n	cvtZRZV	AR,A2\n	OD2	A2,A1\n	cvtZVZL	A1,AL\n",

/* 009 */
ASG OPFLOAT,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	ANYFIXED,
	SAREG|AWD,	TFLOAT|TDOUBLE,
		NAREG,	RLEFT|RESCC,	/* usable() knows we need a reg pair */
		"	ZG\n",

ASG OPFLOAT,	INAREG|FOREFF|FORCC,			/* vdp008 */
	SAREG|AWD,	TUSHORT|TUCHAR,			/* vdp008*/
	SAREG|AWD,	TFLOAT,
	FLOATFLG|NAREG,	RLEFT|RESC1|RESCC,		/* vdp008 no 2*nareg*/
		"	movzZLl	AL,A1\n	cvtlf	A1,A1\n	OF2	AR,A1\n	cvtfZL	A1,AL\n",

ASG OPFLOAT,	INAREG|FOREFF|FORCC,			/* SLR003 */
	SAREG|AWD,	TUSHORT|TUCHAR,			/* SLR004 */
	SAREG|AWD,	TFLOAT,
	2*NAREG,	RLEFT|RESC1|RESCC,
		"	movzZLl	AL,A1\n	cvtlZV	A1,A1\n	cvtZRZV	AR,A2\n	OD2	A2,A1\n	cvtZVZL	A1,AL\n",

/* 
 * #ifdef	FORT     
 *	SLR002	This production is not needed and is incorrect because
 *		it does floating arithmetic in float instead of double
 *	vdp008  Add this production back in but check FLOATFLG 
 */
ASG OPFLOAT,	INAREG|FOREFF|FORCC|INTAREG,	/*vdp008 also added intareg*/
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TFLOAT,
	FLOATFLG,	RLEFT|RESCC,
		"	OF2	AR,AL\n",	/* slr002 float arith must be
							  done in double */
/* vdp008 #endif  */ 

ASG OPFLOAT,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TDOUBLE,
	SAREG|AWD,	TFLOAT,
		NAREG|NASR,	RLEFT|RESCC,
		"	cvtfZV	AR,A1\n	OD2	A1,AL\n",	/* slr001 */

ASG OPFLOAT,	INAREG|INTAREG|FOREFF|FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TDOUBLE,
		NAREG,	RLEFT|RESC1|RESCC,
		"	cvtfZV	AL,A1\n	OD2	AR,A1\n	cvtZVf	A1,AL\n", /* slr001 */

OPFLOAT,	INAREG|INTAREG|FORCC,
	STAREG,	TDOUBLE,
	SAREG|AWD,	TDOUBLE,
		0,	RLEFT|RESCC,
		"	OD2	AR,AL\n",

OPFLOAT,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TDOUBLE,
	SAREG|AWD,	TDOUBLE,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OD3	AR,AL,A1\n",

OPFLOAT,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TDOUBLE,
		NAREG|NASL,	RESC1|RESCC,
		"	cvtfZV	AL,A1\n	OD2	AR,A1\n",	/* slr001 */

OPFLOAT,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TDOUBLE,
	SAREG|AWD,	TFLOAT,
		NAREG|NASR,	RESC1|RESCC,
		"	cvtfZV	AR,A1\n	OD3	A1,AL,A1\n",	/* slr001 */ 

	/* vdp008 added next two productions to match float op float
  	 * when fflag is set ... see FLOATFLG
	 */
OPFLOAT,	INAREG|INTAREG|FORCC,		
	STAREG,	TFLOAT,				
        SAREG|AWD,  TFLOAT,			
		FLOATFLG,	RLEFT|RESCC,
		"	OF2	TAR,AL\n",

OPFLOAT,	INAREG|INTAREG|FORCC,	
	SAREG|AWD,	TFLOAT,		
	SAREG|AWD,	TFLOAT,		
		FLOATFLG|NAREG|NASL|NASR,	RESC1|RESCC,
		"	OF3	AR,AL,TA1\n",

/* 
	SLR002	This production is not needed and is incorrect becuase
		it does floating arithmetic in float instead of double

OPFLOAT,	INAREG|INTAREG|FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TFLOAT,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OF3	AR,AL,A1\n	cvtfZV	A1,A1\n", slr001*/

	/* Default actions for hard trees ... */

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

UNARY MUL, DF( UNARY MUL ),

INCR, DF(INCR),

DECR, DF(INCR),

ASSIGN, DF(ASSIGN),

STASG, DF(STASG),

FLD, DF(FLD),

OPLEAF, DF(NAME),

OPLOG,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		REWRITE,	BITYPE,
		"",

OPLOG,	DF(NOT),

COMOP, DF(COMOP),

INIT, DF(INIT),

OPUNARY, DF(UNARY MINUS),


ASG OPANY, DF(ASG PLUS),

OPANY, DF(BITYPE),

FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" };
