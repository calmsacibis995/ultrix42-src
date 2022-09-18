#ifndef lint
static	char	*sccsid = "@(#)scan.c	4.3	(ULTRIX)	11/12/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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
 *
 *		Modification History
 *
 *	Jon Reeves, 10-May-89
 * 006	Signals now return void.
 *
 *	Lu Anne Van de Pas, 1-Apr-86
 * 005	Set up signal handler to catch floating point expection trap (from
 *	bsd4.3)
 * 
 *	Lu Anne Van de Pas, 02-Mar-86
 * 004  Added support for single precision floating point constants 
 *	(e.g.  3.45f, 3e1f, 1.0e-3f ) added flag for adebug (actions)
 *
 *	Victoria Holt, 26-Feb-86
 * 003	Added support for const and volatile.
 *
 *	David L Ballenger, 12-Sep-1985
 * 002	Add code to handle '-' as both input and output file specs so that
 *	ccom can be pipelined.
 *
 *	Rich Phillips, 16-Aug-84
 * 001 	Put asm handling code in an action routine to be called by
 *	the parser. This change causes asm's to be treated the same
 *	as a function call to a function that does not return a value.
 *	It also allows asm's to be used at the function declaration level.
 *
 *	Changes have also been applied to allo.c,cgram.y,manifest,match.c
 *	and table.c to change the asm handling.
 *
 *	Based on:  scan.c	1.3 (Berkeley) 12/24/82
 *
 *****************************************************************/

# include "mfile1"
# include <a.out.h>
# include <stab.h>
# include <ctype.h>
# include <signal.h>
	/* temporarily */

int asm_esc = 0; /* asm escaped used in file */
	/* lexical actions */

# define A_ERR 0		/* illegal character */
# define A_LET 1		/* saw a letter */
# define A_DIG 2		/* saw a digit */
# define A_1C 3			/* return a single character */
# define A_STR 4		/* string */
# define A_CC 5			/* character constant */
# define A_BCD 6		/* GCOS BCD constant */
# define A_SL 7			/* saw a / */
# define A_DOT 8		/* saw a . */
# define A_PL 9		/* + */
# define A_MI 10		/* - */
# define A_EQ 11		/* = */
# define A_NOT 12		/* ! */
# define A_LT 13		/* < */
# define A_GT 14		/* > */
# define A_AND 16		/* & */
# define A_OR 17		/* | */
# define A_WS 18		/* whitespace (not \n) */
# define A_NL 19		/* \n */

	/* character classes */

# define LEXLET 01
# define LEXDIG 02
# define LEXOCT 04
# define LEXHEX 010
# define LEXWS 020
# define LEXDOT 040

	/* reserved word actions */

# define AR_TY 0		/* type word */
# define AR_RW 1		/* simple reserved word */
# define AR_CL 2		/* storage class word */
# define AR_S 3		/* struct */
# define AR_U 4		/* union */
# define AR_E 5		/* enum */
# define AR_A 6		/* asm */
# define AR_TA 7	/* type attributes - const, volatile */

	/* text buffer */
#ifndef FLEXNAMES
# define LXTSZ 100
#else
#define	LXTSZ	BUFSIZ
#endif
char yytext[LXTSZ];
char * lxgcp;

int fflag=0;		/*vdp004 do single precision arithmetic in 'f' */ 
char * asmptr;		/*RAP001 pointer to asm contents between "'s*/
extern int proflg;
int adebug=0; 		/*vdp004 print actions from trees.c */ 
extern int idebug, bdebug, tdebug, edebug, ddebug, xdebug, gdebug;
int bswitch;							/* jpm: cld 153 */
extern unsigned int offsz;
int oldway;		/* allocate storage so lint will compile as well */

extern void fpe();	  /*vdp05 routine to handle floating point exception*/
struct sigvec fpe_sigvec;  /*vdp05*/

#ifndef LINT
extern int lastloc;
#endif

unsigned caloff();

/* ARGSUSED */
mainp1( argc, argv )
	int 	argc;
	char	*argv[];
{  /* control multiple files */
	register	i;
	register char 	*cp;
	int fdef = 0;
	char *release = "ULTRIX PCC version2.0";

	offsz = caloff();
	for( i=1; i<argc; ++i ) {
		cp = argv[i];
		if (*cp == '-') {
			cp++;
			if (*cp == 'X') while(*++cp) {
				switch( *cp ){

				case 'r':
					fprintf( stderr, "Release: %s\n",
						release );
					break;

				case 'd':
					++ddebug;
					break;
				case 'A':	/* actions --trees.c */ 
					++adebug;
					break;
				case 'i':
					++idebug;
					break;
				case 'b':
					++bdebug;
					break;
			    case 'S':					/* jpm: cld 153 added bswitch */
					++bswitch;	
					break;
				case 't':
					++tdebug;
					break;
				case 'e':
					++edebug;
					break;
				case 'x':
					++xdebug;
					break;
				case 'P':	/* profiling */
					++proflg;
					break;
				case 'g':
					++gdebug;
					break;
				case 'G':
					++gdebug;
					oldway = 1;
					break;
				}
			} else if (*cp == '\000') fdef++ ;
		} else switch( fdef++ ) {
			case 0:
			case 1: if(freopen(argv[i],
					   fdef==1 ? "r" : "w",
					   fdef==1 ? stdin : stdout)
				   == NULL) {
					fprintf(stderr,
						"ccom:can't open %s\n",
						argv[i]);
					exit(1);
				}
				break;
			default:break;
		}
}

# ifdef ONEPASS
	p2init( argc, argv );
# endif

	for( i=0; i<SYMTSZ; ++i ) stab[i].stype = TNULL;

	lxinit();
	tinit();
	mkdope();

	lineno = 1;

	/* dimension table initialization */

	dimtab[NULL] = 0;
	dimtab[CHAR] = SZCHAR;
	dimtab[INT] = SZINT;
	dimtab[FLOAT] = SZFLOAT;
	dimtab[DOUBLE] = SZDOUBLE;
	dimtab[LONG] = SZLONG;
	dimtab[SHORT] = SZSHORT;
	dimtab[UCHAR] = SZCHAR;
	dimtab[USHORT] = SZSHORT;
	dimtab[UNSIGNED] = SZINT;
	dimtab[ULONG] = SZLONG;
	/* starts past any of the above */
	curdim = 16;
	reached = 1;

	/* vdp05 assign routine fpe() to handle 
	 * floating point exception
	 */  
	fpe_sigvec.sv_handler = fpe;
	(void) sigvec(SIGFPE, &fpe_sigvec, (struct sigvec *) NULL);

	yyparse();
	yyaccpt();

	ejobcode( nerrors ? 1 : 0 );
	return(nerrors?1:0);

	}

# ifdef ibm

# define CSMASK 0377
# define CSSZ 256

# else

# define CSMASK 0177
# define CSSZ 128

# endif

short lxmask[CSSZ+1];

lxenter( s, m ) register char *s; register short m; {
	/* enter a mask into lxmask */
	register c;

	while( c= *s++ ) lxmask[c+1] |= m;

	}


# define lxget(c,m) (lxgcp=yytext,lxmore(c,m))

lxmore( c, m )  register c, m; {
	register char *cp;

	*(cp = lxgcp) = c;
	while( c=getchar(), lxmask[c+1]&m ){
		if( cp < &yytext[LXTSZ-1] ){
			*++cp = c;
			}
		}
	ungetc(c,stdin);
	*(lxgcp = cp+1) = '\0';
	}

struct lxdope {
	short lxch;	/* the character */
	short lxact;	/* the action to be performed */
	short lxtok;	/* the token number to be returned */
	short lxval;	/* the value to be returned */
	} lxdope[] = {

	'@',	A_ERR,	0,	0,	/* illegal characters go here... */
	'_',	A_LET,	0,	0,	/* letters point here */
	'0',	A_DIG,	0,	0,	/* digits point here */
	' ',	A_WS,	0,	0,	/* whitespace goes here */
	'\n',	A_NL,	0,	0,
	'"',	A_STR,	0,	0,	/* character string */
	'\'',	A_CC,	0,	0,	/* character constant */
	'`',	A_BCD,	0,	0,	/* GCOS BCD constant */
	'(',	A_1C,	LP,	0,
	')',	A_1C,	RP,	0,
	'{',	A_1C,	LC,	0,
	'}',	A_1C,	RC,	0,
	'[',	A_1C,	LB,	0,
	']',	A_1C,	RB,	0,
	'*',	A_1C,	MUL,	MUL,
	'?',	A_1C,	QUEST,	0,
	':',	A_1C,	COLON,	0,
	'+',	A_PL,	PLUS,	PLUS,
	'-',	A_MI,	MINUS,	MINUS,
	'/',	A_SL,	DIVOP,	DIV,
	'%',	A_1C,	DIVOP,	MOD,
	'&',	A_AND,	AND,	AND,
	'|',	A_OR,	OR,	OR,
	'^',	A_1C,	ER,	ER,
	'!',	A_NOT,	UNOP,	NOT,
	'~',	A_1C,	UNOP,	COMPL,
	',',	A_1C,	CM,	CM,
	';',	A_1C,	SM,	0,
	'.',	A_DOT,	STROP,	DOT,
	'<',	A_LT,	RELOP,	LT,
	'>',	A_GT,	RELOP,	GT,
	'=',	A_EQ,	ASSIGN,	ASSIGN,
	-1,	A_1C,	0,	0,
	};

struct lxdope *lxcp[CSSZ+1];

lxinit(){
	register struct lxdope *p;
	register i;
	register char *cp;
	/* set up character classes */

	lxenter( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$", LEXLET );
	lxenter( "0123456789", LEXDIG );
	lxenter( "0123456789abcdefABCDEF", LEXHEX );
		/* \013 should become \v someday; \013 is OK for ASCII and EBCDIC */
	lxenter( " \t\r\b\f\013", LEXWS );
	lxenter( "01234567", LEXOCT );
	lxmask['.'+1] |= LEXDOT;

	/* make lxcp point to appropriate lxdope entry for each character */

	/* initialize error entries */

	for( i= 0; i<=CSSZ; ++i ) lxcp[i] = lxdope;

	/* make unique entries */

	for( p=lxdope; ; ++p ) {
		lxcp[p->lxch+1] = p;
		if( p->lxch < 0 ) break;
		}

	/* handle letters, digits, and whitespace */
	/* by convention, first, second, and third places */

	cp = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[1];
	cp = "123456789";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[2];
	cp = "\t\b\r\f\013";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[3];

	/* first line might have title */
	lxtitle();

	}

int lxmatch;  /* character to be matched in char or string constant */

lxstr(ct){
	/* match a string or character constant, up to lxmatch */

	register c;
	register val;
	register i;

	i=0;
	while( (c=getchar()) != lxmatch ){
		switch( c ) {

		case EOF:
			uerror( "unexpected EOF" );
			break;

		case '\n':
			uerror( "newline in string or char constant" );
			++lineno;
			break;

		case '\\':
			switch( c = getchar() ){

			case '\n':
				++lineno;
				continue;

			default:
				val = c;
				goto mkcc;

			case 'n':
				val = '\n';
				goto mkcc;

			case 'r':
				val = '\r';
				goto mkcc;

			case 'b':
				val = '\b';
				goto mkcc;

			case 't':
				val = '\t';
				goto mkcc;

			case 'f':
				val = '\f';
				goto mkcc;

			case 'v':
				val = '\013';
				goto mkcc;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c-'0';
				c=getchar();  /* try for 2 */
				if( lxmask[c+1] & LEXOCT ){
					val = (val<<3) | (c-'0');
					c = getchar();  /* try for 3 */
					if( lxmask[c+1] & LEXOCT ){
						val = (val<<3) | (c-'0');
						}
					else ungetc( c ,stdin);
					}
				else ungetc( c ,stdin);

				goto mkcc1;

				}
		default:
			val =c;
		mkcc:
			val = CCTRANS(val);
		mkcc1:
			if( lxmatch == '\'' ){
				val = CHARCAST(val);  /* it is, after all, a "character" constant */
				makecc( val, i );
				}
			else { /* stash the byte into the string */
				if( strflg ) {
					if( ct==0 || i<ct ) putbyte( val );
					else if( i == ct ) werror( "non-null byte ignored in string initializer" );
					}
				else bycode( val, i );
				}
			++i;
			continue;
			}
		break;
		}
	/* end of string or  char constant */

	if( lxmatch == '"' ){
		if( strflg ){ /* end the string */
			if( ct==0 || i<ct ) putbyte( 0 );  /* the null at the end */
			}
		else {  /* the initializer gets a null byte */
			bycode( 0, i++ );
			bycode( -1, i );
			dimtab[curdim] = i;  /* in case of later sizeof ... */
			}
		}
	else { /* end the character constant */
		if( i == 0 ) uerror( "empty character constant" );
		if( i>(SZINT/SZCHAR) || ( (pflag||hflag)&&i>1) )
			uerror( "too many characters in character constant" );
		}
	}

lxcom(){
	register c;
	/* saw a /*: process a comment */

	for(;;){

		switch( c = getchar() ){

		case EOF:
			uerror( "unexpected EOF" );
			return;

		case '\n':
			++lineno;

		default:
			continue;

		case '*':
			if( (c = getchar()) == '/' ) return;
			else ungetc( c ,stdin);
			continue;

# ifdef LINT
		case 'V':
			lxget( c, LEXLET|LEXDIG );
			{
				extern int vaflag;
				int i;
				i = yytext[7]?yytext[7]-'0':0;
				yytext[7] = '\0';
				if( strcmp( yytext, "VARARGS" ) ) continue;
				vaflag = i;
				continue;
				}
		case 'L':
			lxget( c, LEXLET );
			if( strcmp( yytext, "LINTLIBRARY" ) ) continue;
			{
				extern int libflag;
				libflag = 1;
				}
			continue;

		case 'A':
			lxget( c, LEXLET );
			if( strcmp( yytext, "ARGSUSED" ) ) continue;
			{
				extern int argflag, vflag;
				argflag = 1;
				vflag = 0;
				}
			continue;

		case 'N':
			lxget( c, LEXLET );
			if( strcmp( yytext, "NOTREACHED" ) ) continue;
			reached = 0;
			continue;
# endif
			}
		}
	}

yylex(){
	for(;;){

		register lxchar;
		register struct lxdope *p;
		register struct symtab *sp;
		int id;

		switch( (p=lxcp[(lxchar=getchar())+1])->lxact ){

		onechar:
			ungetc( lxchar ,stdin);

		case A_1C:
			/* eat up a single character, and return an opcode */

			yylval.intval = p->lxval;
			return( p->lxtok );

		case A_ERR:
			uerror( "illegal character: %03o (octal)", lxchar );
			break;

		case A_LET:
			/* collect an identifier, check for reserved word, and return */
			lxget( lxchar, LEXLET|LEXDIG );
			/*RAP001
				Set up asm to look like a name so a UNARY CALL
				to asm can be set up by the parser.
			*/
			if( (lxchar=lxres()) > 0 && lxchar != ASM) return( lxchar ); /* reserved word */
			if( lxchar== 0 ) continue;
#ifdef FLEXNAMES
			id = lookup( hash(yytext),
#else
			id = lookup( yytext,
#endif
				/* tag name for struct/union/enum */
				(stwart&TAGNAME)? STAG:
				/* member name for struct/union */
				(stwart&(INSTRUCT|INUNION|FUNNYNAME))?SMOS:0 );
			sp = &stab[id];
			if( sp->sclass == TYPEDEF && !stwart ){
				stwart = instruct;
				yylval.nodep = mkty( sp->stype, sp->dimoff, sp->sizoff );
				yylval.nodep->in.typattr = sp->stypattr;
				return( TYPE );
				}
			stwart = (stwart&SEENAME) ? instruct : 0;
			yylval.intval = id;
			if (lxchar == ASM) /* RAP001 */
				return ( ASM );
			else
				return( NAME );

		case A_DIG:
			/* collect a digit string, then look at last one... */
			lastcon = 0;
			lxget( lxchar, LEXDIG );
			switch( lxchar=getchar() ){

			case 'x':
			case 'X':
				if( yytext[0] != '0' && !yytext[1] ) uerror( "illegal hex constant" );
				lxmore( lxchar, LEXHEX );
				/* convert the value */
				{
					register char *cp;
					for( cp = yytext+2; *cp; ++cp ){
						/* this code won't work for all wild character sets,
						   but seems ok for ascii and ebcdic */
						lastcon <<= 4;
						if( isdigit( *cp ) ) lastcon += *cp-'0';
						else if( isupper( *cp ) ) lastcon += *cp - 'A'+ 10;
						else lastcon += *cp - 'a'+ 10;
						}
					}

			hexlong:
				/* criterion for longness for hex and octal constants is that it
				   fit within 0177777 */
				if( lastcon & ~0177777L ) yylval.intval = 1;
				else yylval.intval = 0;

				goto islong;

			case '.':
				lxmore( lxchar, LEXDIG );

			getfp:
				if((lxchar=getchar()) == 'f' || lxchar == 'F')
				{  /*vdp004 - floating point constant */ 
					goto flt; 
					} 
				if (lxchar == 'e' || lxchar == 'E' ){ /* exponent */
				
			case 'e':
			case 'E':
					if( (lxchar=getchar()) == '+' || lxchar == '-' ){
						*lxgcp++ = 'e';
						}
					else {
						ungetc(lxchar,stdin);
						lxchar = 'e';
						}
					lxmore( lxchar, LEXDIG );
					/* now have the whole thing... */
					if ((lxchar=getchar()) == 'f' ||
					    lxchar=='F') {
					    	goto flt; 
						}
					else {
						ungetc(lxchar,stdin);
						goto flt; 
						}
					    	
					}
				else {  /* no exponent */
					ungetc( lxchar ,stdin);
					goto flt; 
					}
			/* vdp004  recongize floating constants by those 
			 * that end with an 'f' or a 'F' 
			 */ 
			case 'f':
			case 'F': 
				/* Would get here only if we hadn't already
			 	 * run into a '.' or an 'e'. vdp004 
				 */
				uerror ("Illegal floating point constant.");
				continue; 
			flt:
				if (lxchar == 'f' || lxchar == 'F') {
					return ( cvtfloat( yytext) ); 
				}	
				else {
					return ( cvtdouble( yytext)); 
				}

			default:
				ungetc( lxchar ,stdin);
				if( yytext[0] == '0' ){
					/* convert in octal */
					register char *cp;
					for( cp = yytext+1; *cp; ++cp ){
						lastcon <<= 3;
						lastcon += *cp - '0';
						}
					goto hexlong;
					}
				else {
					/* convert in decimal */
					register char *cp;
					for( cp = yytext; *cp; ++cp ){
						lastcon = lastcon * 10 + *cp - '0';
						}
					}

				/* decide if it is long or not (decimal case) */

				/* if it is positive and fits in 15 bits, or negative and
				   and fits in 15 bits plus an extended sign, it is int; otherwise long */
				/* if there is an l or L following, all bets are off... */

				{	CONSZ v;
					v = lastcon & ~077777L;
					if( v == 0 || v == ~077777L ) yylval.intval = 0;
					else yylval.intval = 1;
					}

			islong:
				/* finally, look for trailing L or l */
				if( (lxchar = getchar()) == 'L' || lxchar == 'l' ) yylval.intval = 1;
				else ungetc( lxchar ,stdin);
				return( ICON );
				}

		case A_DOT:
			/* look for a dot: if followed by a digit, floating point */
			lxchar = getchar();
			if( lxmask[lxchar+1] & LEXDIG ){
				ungetc(lxchar,stdin);
				lxget( '.', LEXDIG );
				goto getfp;
				}
			stwart = FUNNYNAME;
			goto onechar;

		case A_STR:
			/* string constant */
			lxmatch = '"';
			return( STRING );

		case A_CC:
			/* character constant */
			lxmatch = '\'';
			lastcon = 0;
			lxstr(0);
			yylval.intval = 0;
			return( ICON );

		case A_BCD:
			{
				register i;
				int j;
				for( i=0; i<LXTSZ; ++i ){
					if( ( j = getchar() ) == '`' ) break;
					if( j == '\n' ){
						uerror( "newline in BCD constant" );
						break;
						}
					yytext[i] = j;
					}
				yytext[i] = '\0';
				if( i>6 ) uerror( "BCD constant exceeds 6 characters" );
# ifdef gcos
				else strtob( yytext, &lastcon, i );
				lastcon >>= 6*(6-i);
# else
				uerror( "gcos BCD constant illegal" );
# endif
				yylval.intval = 0;  /* not long */
				return( ICON );
				}

		case A_SL:
			/* / */
			if( (lxchar=getchar()) != '*' ) goto onechar;
			lxcom();
		case A_WS:
			continue;

		case A_NL:
			++lineno;
			lxtitle();
			continue;

		case A_NOT:
			/* ! */
			if( (lxchar=getchar()) != '=' ) goto onechar;
			yylval.intval = NE;
			return( EQUOP );

		case A_MI:
			/* - */
			if( (lxchar=getchar()) == '-' ){
				yylval.intval = DECR;
				return( INCOP );
				}
			if( lxchar != '>' ) goto onechar;
			stwart = FUNNYNAME;
			yylval.intval=STREF;
			return( STROP );

		case A_PL:
			/* + */
			if( (lxchar=getchar()) != '+' ) goto onechar;
			yylval.intval = INCR;
			return( INCOP );

		case A_AND:
			/* & */
			if( (lxchar=getchar()) != '&' ) goto onechar;
			return( yylval.intval = ANDAND );

		case A_OR:
			/* | */
			if( (lxchar=getchar()) != '|' ) goto onechar;
			return( yylval.intval = OROR );

		case A_LT:
			/* < */
			if( (lxchar=getchar()) == '<' ){
				yylval.intval = LS;
				return( SHIFTOP );
				}
			if( lxchar != '=' ) goto onechar;
			yylval.intval = LE;
			return( RELOP );

		case A_GT:
			/* > */
			if( (lxchar=getchar()) == '>' ){
				yylval.intval = RS;
				return(SHIFTOP );
				}
			if( lxchar != '=' ) goto onechar;
			yylval.intval = GE;
			return( RELOP );

		case A_EQ:
			/* = */
			switch( lxchar = getchar() ){

			case '=':
				yylval.intval = EQ;
				return( EQUOP );

			case '+':
				yylval.intval = ASG PLUS;
				break;

			case '-':
				yylval.intval = ASG MINUS;

			warn:
				if( lxmask[ (lxchar=getchar())+1] & (LEXLET|LEXDIG|LEXDOT) ){
					werror( "ambiguous assignment: assignment op taken" );
					}
				ungetc( lxchar ,stdin);
				break;

			case '*':
				yylval.intval = ASG MUL;
				goto warn;

			case '/':
				yylval.intval = ASG DIV;
				break;

			case '%':
				yylval.intval = ASG MOD;
				break;

			case '&':
				yylval.intval = ASG AND;
				break;

			case '|':
				yylval.intval = ASG OR;
				break;

			case '^':
				yylval.intval = ASG ER;
				break;

			case '<':
				if( (lxchar=getchar()) != '<' ){
					uerror( "=<%c illegal", lxchar );
					}
				yylval.intval = ASG LS;
				break;

			case '>':
				if( (lxchar=getchar()) != '>' ){
					uerror( "=>%c illegal", lxchar );
					}
				yylval.intval = ASG RS;
				break;

			default:
				goto onechar;

				}

			return( ASOP );

		default:
			cerror( "yylex error, character %03o (octal)", lxchar );

			}

		/* ordinarily, repeat here... */
		cerror( "out of switch in yylex" );

		}

	}

struct lxrdope {
	/* dope for reserved, in alphabetical order */

	char *lxrch;	/* name of reserved word */
	short lxract;	/* reserved word action */
	short lxrval;	/* value to be returned */
	} lxrdope[] = {

	"asm",		AR_A,	0,
	"auto",		AR_CL,	AUTO,
	"break",	AR_RW,	BREAK,
	"char",		AR_TY,	CHAR,
	"case",		AR_RW,	CASE,
	"const",	AR_TA,	CONST,
	"continue",	AR_RW,	CONTINUE,
	"double",	AR_TY,	DOUBLE,
	"default",	AR_RW,	DEFAULT,
	"do",		AR_RW,	DO,
	"extern",	AR_CL,	EXTERN,
	"else",		AR_RW,	ELSE,
	"enum",		AR_E,	ENUM,
	"for",		AR_RW,	FOR,
	"float",	AR_TY,	FLOAT,
	"fortran",	AR_CL,	FORTRAN,
	"goto",		AR_RW,	GOTO,
	"if",		AR_RW,	IF,
	"int",		AR_TY,	INT,
	"long",		AR_TY,	LONG,
	"return",	AR_RW,	RETURN,
	"register",	AR_CL,	REGISTER,
	"switch",	AR_RW,	SWITCH,
	"struct",	AR_S,	0,
	"sizeof",	AR_RW,	SIZEOF,
	"short",	AR_TY,	SHORT,
	"static",	AR_CL,	STATIC,
	"typedef",	AR_CL,	TYPEDEF,
	"unsigned",	AR_TY,	UNSIGNED,
	"union",	AR_U,	0,
	"void",		AR_TY,	UNDEF, /* tymerge adds FTN */
	"volatile",	AR_TA,	VOLATILE,
	"while",	AR_RW,	WHILE,
	"",		0,	0,	/* to stop the search */
	};

lxres() {
	/* check to see of yytext is reserved; if so,
	/* do the appropriate action and return */
	/* otherwise, return -1 */

	register c, ch;
	register struct lxrdope *p;

	ch = yytext[0];

	if( !islower(ch) ) return( -1 );

	switch( ch ){

	case 'a':
		c=0; break;
	case 'b':
		c=2; break;
	case 'c':
		c=3; break;
	case 'd':
		c=7; break;
	case 'e':
		c=10; break;
	case 'f':
		c=13; break;
	case 'g':
		c=16; break;
	case 'i':
		c=17; break;
	case 'l':
		c=19; break;
	case 'r':
		c=20; break;
	case 's':
		c=22; break;
	case 't':
		c=27; break;
	case 'u':
		c=28; break;
	case 'v':
		c=30; break;
	case 'w':
		c=32; break;

	default:
		return( -1 );
		}

	for( p= lxrdope+c; p->lxrch[0] == ch; ++p ){
		if( !strcmp( yytext, p->lxrch ) ){ /* match */
			switch( p->lxract ){

			case AR_TY:
			case AR_TA:
				/* type word or type attribute */
				stwart = instruct;
				yylval.nodep = mkty( (TWORD)p->lxrval, 0, p->lxrval );
				if (p->lxract == AR_TA)
				    return (p->lxrval);   /* CONST/VOLATILE */
				else
				    return( TYPE );

			case AR_RW:
				/* ordinary reserved word */
				return( yylval.intval = p->lxrval );

			case AR_CL:
				/* class word */
				yylval.intval = p->lxrval;
				return( CLASS );

			case AR_S:
				/* struct */
				stwart = INSTRUCT|SEENAME|TAGNAME;
				yylval.intval = INSTRUCT;
				return( STRUCT );

			case AR_U:
				/* union */
				stwart = INUNION|SEENAME|TAGNAME;
				yylval.intval = INUNION;
				return( STRUCT );

			case AR_E:
				/* enums */
				stwart = SEENAME|TAGNAME;
				return( yylval.intval = ENUM );

			case AR_A:
				/* asm */
				/*RAP001
					Let the parser handle it.
				*/
				return (ASM);
			default:
				cerror( "bad AR_?? action" );
				}
			}
		}
	return( -1 );
	}
/*RAP001
	Action routine to read the asm tail. The text between the "'s is
	stored in a buffer which will be put out later in match.c. It will
	be put out immediately if the asm was at the function definition
	level.

	The only real change to the code that used to handle asm's is to
	replace the putchar with code to save the information in a buffer.
*/
procasm()
{
	int c, cnt;
	char *tasmptr;

	cnt = 0;
	asm_esc = 1; /* warn the world! */
	lxget( ' ', LEXWS );
	if( getchar() != '(' ) goto badasm;
	lxget( ' ', LEXWS );
	if( getchar() != '"' ) goto badasm;
	asmptr  = tasmptr = (char *)malloc(40);
	*asmptr = '\0';	/* In case LINT is defined or theres an error */
	while( (c=getchar()) != '"' ){
		if( c=='\n' || c==EOF ) goto badasm;
# ifndef LINT
					
		/* Increase the buffer if its about to overflow.       */
		/* "+2" is used because of the CR and NIL to be added. */
		/* The buffer is always a multiple of 40 characters.   */

		if ( ( (cnt+2) % 40) == 0 ){
			asmptr = (char *)realloc( asmptr, cnt + 42 );
			tasmptr = asmptr + cnt;
			}
		*tasmptr++ = c;
		cnt++;
# endif
		}
	lxget( ' ', LEXWS );
	if( getchar() != ')' ) goto badasm;
# ifndef LINT
	/*
		Now terminate with CR and NIL
	*/
	*tasmptr++ = '\n';
	*tasmptr   = '\0';
# endif
	return( 0 );

badasm:
	uerror( "bad asm construction");
	free(asmptr);		/* Free the buffer */
	asmptr = (char *)NIL;	/* Ignore any partial information */
	return( 0 );		
}


/*RAP001
	Put out the asm information. This routine is called immediately
	for asm's at the function declaration level or in match.c for
	asm's in a function context in procedural code.

	All this  is done to force the code out at a time that will make
	the asm look semantically like a function call.
*/
outasm(cptr)
char *cptr;
{
	char *tptr=cptr;

	if ( tptr != (char *)NIL){
# ifndef ONEPASS
# ifndef LINT
		putchar(')');
# endif
# endif
		while(*tptr) putchar(*tptr++);
		free(cptr);
		}
}


extern int	labelno;

lxtitle(){
	/* called after a newline; set linenumber and file name */

	register c, val;
	register char *cp, *cq;

	for(;;){  /* might be several such lines in a row */
		if( (c=getchar()) != '#' ){
			if( c != EOF ) ungetc(c,stdin);
#ifndef LINT
			if ( lastloc != PROG) return;
			cp = ftitle;
			cq = ititle;
			while ( *cp ) if (*cp++ != *cq++) return;
			if ( *cq ) return;
			psline();
#endif
			return;
			}

		lxget( ' ', LEXWS );
		val = 0;
		for( c=getchar(); isdigit(c); c=getchar() ){
			val = val*10+ c - '0';
			}
		ungetc( c, stdin );
		lineno = val;
		lxget( ' ', LEXWS );
		if( (c=getchar()) != '\n' ){
			for( cp=ftitle; c!='\n'; c=getchar(),++cp ){
				*cp = c;
				}
			*cp = '\0';
#ifndef LINT
			if (ititle[0] == '\0') {
				cp = ftitle;
				cq = ititle;
				while ( *cp )  
					*cq++ = *cp++;
				*cq = '\0';
				*--cq = '\0';
#ifndef FLEXNAMES
				for ( cp = ititle+1; *(cp-1); cp += 8 ) {
					pstab(cp, N_SO);
					if (gdebug) printf("0,0,LL%d\n", labelno);
					}
#else
				pstab(ititle+1, N_SO);
				if (gdebug) printf("0,0,LL%d\n", labelno);
#endif

				*cq = '"';
				printf("LL%d:\n", labelno++);
				}
#endif
			}
		}
	}

#ifdef FLEXNAMES
#define	NSAVETAB	4096
char	*savetab;
int	saveleft;

char *
savestr(cp)
	register char *cp;
{
	register int len;

	len = strlen(cp) + 1;
	if (len > saveleft) {
		saveleft = NSAVETAB;
		if (len > saveleft)
			saveleft = len;
		savetab = (char *)malloc(saveleft);
		if (savetab == 0)
			cerror("Ran out of memory (savestr)");
	}
	strncpy(savetab, cp, len);
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return (cp);
}

/*
 * The definition for the segmented hash tables.
 */
#define	MAXHASH	20
#define	HASHINC	1013
struct ht {
	char	**ht_low;
	char	**ht_high;
	int	ht_used;
} htab[MAXHASH];

char *
hash(s)
	char *s;
{
	register char **h;
	register i;
	register char *cp;
	struct ht *htp;
	int sh;

	/*
	 * The hash function is a modular hash of
	 * the sum of the characters with the sum
	 * doubled before each successive character
	 * is added.
	 */
	cp = s;
	i = 0;
	while (*cp)
		i = i*2 + *cp++;
	sh = (i&077777) % HASHINC;
	cp = s;
	/*
	 * There are as many as MAXHASH active
	 * hash tables at any given point in time.
	 * The search starts with the first table
	 * and continues through the active tables
	 * as necessary.
	 */
	for (htp = htab; htp < &htab[MAXHASH]; htp++) {
		if (htp->ht_low == 0) {
			register char **hp =
			    (char **) calloc(sizeof (char **), HASHINC);
			if (hp == 0)
				cerror("ran out of memory (hash)");
			htp->ht_low = hp;
			htp->ht_high = htp->ht_low + HASHINC;
		}
		h = htp->ht_low + sh;
		/*
		 * quadratic rehash increment
		 * starts at 1 and incremented
		 * by two each rehash.
		 */
		i = 1;
		do {
			if (*h == 0) {
				if (htp->ht_used > (HASHINC * 3)/4)
					break;
				htp->ht_used++;
				*h = savestr(cp);
				return (*h);
			}
			if (**h == *cp && strcmp(*h, cp) == 0)
				return (*h);
			h += i;
			i += 2;
			if (h >= htp->ht_high)
				h -= HASHINC;
		} while (i < HASHINC);
	}
	cerror("ran out of hash tables");
}
#endif
