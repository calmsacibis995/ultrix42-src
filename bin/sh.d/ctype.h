/* ctype.h (Ultrix) 7/2/90 */

#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 ************************************************************************
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
 ************************************************************************
 ----------
  Modification History
  ~~~~~~~~~~~~~~~~~~~~
  01	15-Apr-84, Greg Tarsa.
	Added comments.
 */

/*
    Character classes for table 1
*/
#define T_SUB	01	/* command substitution character ($) */
#define T_MET	02	/* simple metacharacters (^()) */
#define	T_SPC	04	/* whitespace (sp \t) */
#define T_DIP	010	/* doubleable I/O pointers (| & ; < >)*/
#define T_EOF	020	/* end of file */
#define T_EOR	040	/* end of record (\n) */
#define T_QOT	0100	/* quotation (`") */
#define T_ESC	0200	/* text expansions (`$) */

/*
    Character classes for table 2
*/
#define T_BRC	01	/* brace ??? ({) */
#define T_DEF	02	/* var expansion default char (}=-+?) */
#define T_AST	04	/* All parameter subtitition char (@*) */
#define	T_DIG	010	/* digit (0123456789) */
#define T_FNG	020	/* file name glob char (*[?) */
#define T_SHN	040	/* special shell parameter (#-?$!) */
#define	T_IDC	0100	/* shell identifier char (a-z0-9) */
#define T_SET	0200	/* simple specail param set char (+) */

/* for single chars */
#define _TAB	(T_SPC)		/* '\t' */
#define _SPC	(T_SPC)		/* ' '  */
#define _UPC	(T_IDC)		/* uppercase letters */
#define _LPC	(T_IDC)		/* lowercase letters */
#define _DIG	(T_DIG)		/* digits */
#define _EOF	(T_EOF)		/* end of file */
#define _EOR	(T_EOR)		/* '\n' */
#define _BAR	(T_DIP)		/* '|' */
#define _HAT	(T_MET)		/* '^' */
#define _BRA	(T_MET)		/* '(' */
#define _KET	(T_MET)		/* ')' */
#define _SQB	(T_FNG)		/* '[' */
#define _AMP	(T_DIP)		/* '&' */
#define _SEM	(T_DIP)		/* ';' */
#define _LT	(T_DIP)		/* '<' */
#define _GT	(T_DIP)		/* '>' */
#define _LQU	(T_QOT|T_ESC)	/* '`' */
#define _BSL	(T_ESC)		/* '\' */
#define _DQU	(T_QOT)		/* '"' */
#define _DOL1	(T_SUB|T_ESC)	/* '$' */

#define _CBR	T_BRC		/* '{' */
#define _CKT	T_DEF		/* '}' */
#define _AST	(T_AST|T_FNG)	/* '*' */
#define _EQ	(T_DEF)		/* '=' */
#define _MIN	(T_DEF|T_SHN)	/* '-' */
#define _PCS	(T_SHN)		/* '!' */
#define _NUM	(T_SHN)		/* '#' */
#define _DOL2	(T_SHN)		/* '$' */
#define _PLS	(T_DEF|T_SET)	/* '+' */
#define _AT	(T_AST)		/* '@' */
#define _QU	(T_DEF|T_FNG|T_SHN)	/* '?' */

/*
    Abbreviations for tests
*/
#define _IDCH	(T_IDC|T_DIG)			/* I/O descriptor char */
#define _META	(T_SPC|T_DIP|T_MET|T_EOR)	/* metacharacter */

/*
    Character type macros based on the ctype1 array

    Note: these args are not call by value !!!!
*/
char    _ctype1[];

#define	space(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_SPC))
#define eofmeta(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(_META|T_EOF))
#define qotchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_QOT))
#define eolchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_EOR|T_EOF))
#define dipchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_DIP))
#define subchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_SUB|T_QOT))
#define escchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_ESC))

/*
    Character type macros based on the ctype2 array

    Note: these args are not call by value !!!!
*/
char    _ctype2[];

#define	digit(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_DIG))
#define fngchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_FNG))
#define dolchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_AST|T_BRC|T_DIG|T_IDC|T_SHN))
#define defchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_DEF))
#define setchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_SET))
#define digchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_AST|T_DIG))
#define	letter(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_IDC))
#define alphanum(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(_IDCH))
#define astchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_AST))
