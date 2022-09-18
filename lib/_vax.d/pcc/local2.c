# ifndef lint
static	char	*sccsid = "@(#)local2.c	4.2	(ULTRIX)	8/13/90";
# endif

# include "mfile2"
# include "ctype.h"

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

/************************************************************************
 *		Modification History
 *
 *	Jon Reeves, 90-Feb-01
 * 007	Add BSD4.3 code to zzzcode() to do SCONV's, including proper
 *	handling of unsigned to float and ZG; also replaces old conversion
 *	code.  Add support routine collapsible(), also from BSD4.3.
 *
 *	David L Ballenger, 16-Mar-1986
 * 006	Fix problem in zzzcode() where two cvt instructions were written
 *	on the same line with no intervening spaces or line feeds.
 *
 *	David L Ballenger, 16-Mar-1986
 * 005	Add bug fixes from SUN for NFS.
 *
 *	Lu Anne Van de Pas, 1-Mar-86:
 * 004- added support for f floating constants (FCON) and f floating 
 *      arithmetic to be done in f when -f is specified on the command 
 *	line (fflag).  
 *
 *	Rich Phillips, 30-July-84:
 * 003- Change fix 002 so it is not so retrictive. Allow pointer increments
 * 	in structure assignments of structures that are 1,2,4, or 8 bytes
 *	long to return STAREG. Continue to disallow cases when the object
 * 	referenced is not the same size as the increment of the register.
 *
 *	Stephen Reilly, 16-Feb-84:
 * 002- postfix of ++ or -- is incorrectly handled when the a pointer to a
 *	structure is 4 byte in size and is the expr of a if style 
 *	statement.
 *
 *	Stephen Reilly, 21-Oct-83:
 * 001- New -M flag is added that indicates that the compiler is to
 *	emit gfloat instructions, rather than dfloat instructions.
 *
 ***********************************************************************/

/*
 *	If Mflag when set it indicates that gfloat instructions are
 *	required by the user
 */

extern Mflag;					/* slr001 -M flag */
extern fflag; 					/* vdp004 -f flag */ 
# ifdef FORT
int ftlab1, ftlab2;
# endif
/* a lot of the machine dependent parts of the second pass */

# define putstr(s)	fputs((s), stdout)

# define BITMASK(n) ((1L<<n)-1)

where(c){
	fprintf( stderr, "%s, line %d: ", filename, lineno );
	}

lineid( l, fn ) char *fn; {
	/* identify line l and file fn */
	printf( "#	line %d, file %s\n", l, fn );
	}


eobl2(){
	OFFSZ spoff;	/* offset from stack pointer */
#ifdef FORT
	spoff = maxoff;
	if( spoff >= AUTOINIT ) spoff -= AUTOINIT;
	spoff /= SZCHAR;
	SETOFF(spoff,4);
#ifndef FLEXNAMES
	printf( "	.set	.F%d,%ld\n", ftnno, spoff );
#else
	/* SHOULD BE L%d ... ftnno but must change pc/f77 */
	printf( "	.set	LF%d,%ld\n", ftnno, spoff );
#endif
#else
	extern int ftlab1, ftlab2;

	spoff = maxoff;
	if( spoff >= AUTOINIT ) spoff -= AUTOINIT;
	spoff /= SZCHAR;
	SETOFF(spoff,4);
	printf( "L%d:\n", ftlab1);
	if( spoff!=0 )
		if( spoff < 64 )
			printf( "	subl2	$%ld,sp\n", spoff);
		else
			printf( "	movab	-%ld(sp),sp\n", spoff);
	printf( "	jbr 	L%d\n", ftlab2);
#endif
	maxargs = -1;
	}

struct hoptab { int opmask; char * opstring; } ioptab[] = {

	ASG PLUS, "add",
	ASG MINUS, "sub",
	ASG MUL, "mul",
	ASG DIV, "div",
	ASG OR, "bis",
	ASG ER,	"xor",
	ASG AND, "bic",
	PLUS,	"add",
	MINUS,	"sub",
	MUL,	"mul",
	DIV,	"div",
	OR,	"bis",
	ER,	"xor",
	AND,	"bic",
	-1, ""    };

hopcode( f, o ){
	/* output the appropriate string from the above table */

	register struct hoptab *q;

	for( q = ioptab;  q->opmask>=0; ++q ){
		if( q->opmask == o ){
			putstr( q->opstring );
/* tbl
			if( f == 'F' ) putchar( 'e' );
			else if( f == 'D' ) putchar( 'd' );
   tbl */
/* tbl */
		    /*
	             *  Put the appropriate data type
		     *  suffix on instruction
		     */
			switch( f ) {
				case 'L':
				case 'W':
				case 'B':
					putchar(tolower(f));	
					break;		/* slr001 */
				case 'F':
					/* If -f flag set then put the f
					 * suffix out for f floating
					 * operations.  Actually we
					 * should never have 'F' if
					 * fflag is not set (see FLOATFLG)
					 * in table.  I'm just making
					 * sure.	  vdp004 
					 */ 
					 if (fflag) {
					 	putchar('f');
						break; 
						}
				case 'D':
					/*
					 *  If -M flag set then put the
					 *  g suffix which indicates gfloat
					 */
					if ( Mflag )		/* slr001 */
					    putchar('g'); /* slr001 */
					else			/* slr001 */
					     putchar(tolower(f)); /* slr001 */
					break;

				}
/* tbl */
			return;
			}
		}
	cerror( "no hoptab for %s", opst[o] );
	}

char *
rnames[] = {  /* keyed to register number tokens */

	"r0", "r1",
	"r2", "r3", "r4", "r5",
	"r6", "r7", "r8", "r9", "r10", "r11",
	"ap", "fp", "sp", "pc",

	};

int rstatus[] = {
	SAREG|STAREG, SAREG|STAREG,
	SAREG|STAREG, SAREG|STAREG, SAREG|STAREG, SAREG|STAREG,
	SAREG, SAREG, SAREG, SAREG, SAREG, SAREG,
	SAREG, SAREG, SAREG, SAREG,

	};

tlen(p) NODE *p;
{
	switch(p->in.type) {
		case CHAR:
		case UCHAR:
			return(1);

		case SHORT:
		case USHORT:
			return(2);

		case DOUBLE:
			return(8);

		default:
			return(4);
		}
}

mixtypes(p, q) NODE *p, *q;
{
	register TWORD tp, tq;

	tp = p->in.type;
	tq = q->in.type;

	return( (tp==FLOAT || tp==DOUBLE) !=
		(tq==FLOAT || tq==DOUBLE) );
}

prtype(n) NODE *n;
{
	switch (n->in.type)
		{
		case DOUBLE:

			/*
			 *	If -M flag set put gfloat suffix
			 *      on instruction
			 */

			if  ( Mflag )				/* slr001 */
			    putchar('g');			/* slr001 */
			else					/* slr001 */
			    putchar('d');			/* slr001 */

			return;

		case FLOAT:
			putchar('f');
			return;

		case LONG:
		case ULONG:
		case INT:
		case UNSIGNED:
			putchar('l');
			return;

		case SHORT:
		case USHORT:
			putchar('w');
			return;

		case CHAR:
		case UCHAR:
			putchar('b');
			return;

		default:
			if ( !ISPTR( n->in.type ) ) cerror("zzzcode- bad type");
			else {
				putchar('l');
				return;
				}
		}
}

zzzcode( p, c ) register NODE *p; {
	register m;
	CONSZ val;
	switch( c ){

	case 'N':  /* logical ops, turned into 0-1 */
		/* use register given by register 1 */
		cbgen( 0, m=getlab(), 'I' );
		deflab( p->bn.label );
		printf( "	clrl	%s\n", rnames[getlr( p, '1' )->tn.rval] );
		deflab( m );
		return;

	case 'I':
	case 'P':
		cbgen( p->in.op, p->bn.label, c );
		return;

	case 'A':
		{
		register NODE *l, *r;

		if (xdebug) eprint(p, 0, &val, &val);
		r = getlr(p, 'R');
		if (optype(p->in.op) == LTYPE || p->in.op == UNARY MUL)
			{
			l = resc;
			if (fflag) /*vdp004-  check for floating arith */ 
				{  
				l->in.type = (r->in.type==FLOAT) ? FLOAT :
					     (r->in.type==DOUBLE) ? DOUBLE : 
					     INT;
				} 
			else 
				l->in.type = (r->in.type==FLOAT || 
					      r->in.type==DOUBLE ? DOUBLE : 
					      INT);
			}
		else if (p->in.op == SCONV) {
			l = resc;
			if (fflag)
				l->in.type = r->in.type;
			else
				l->in.type = 
					r->in.type==FLOAT ? DOUBLE : r->in.type;
			r = getlr(p, 'L');
		} else
			l = getlr(p, 'L');
		if (r->in.op == ICON)
			if (r->in.name[0] == '\0') {
				if (r->tn.lval == 0) {
					putstr("clr");
					prtype(l);
					putchar('\t');
					adrput(l);
					return;
					}
				if (r->tn.lval < 0 && r->tn.lval >= -63) {
					putstr("mneg");
					prtype(l);
					r->tn.lval = -r->tn.lval;
					goto ops;
					}
				if (r->tn.lval < 0)
					r->in.type = r->tn.lval >= -128 ? CHAR
						: (r->tn.lval >= -32768 ? SHORT
						: INT);
				else
					r->in.type = r->tn.lval <= 63 ? INT
						: (r->tn.lval <= 127 ? CHAR
						: (r->tn.lval <= 255 ? UCHAR
						: (r->tn.lval <= 32767 ? SHORT
						: (r->tn.lval <= 65535 ? USHORT
						: INT))));
				}
			else {
				putstr("moval");
				putchar('\t');
				acon(r);
				putchar(',');
				adrput(l);
				return;
				}

		if (p->in.op == SCONV &&
		    !(l->in.type == FLOAT || l->in.type == DOUBLE) &&
		    !mixtypes(l, r)) {
			/*
			 * Because registers must always contain objects
			 * of the same width as INTs, we may have to
			 * perform two conversions to get an INT.  Can
			 * the conversions be collapsed into one?
			 */
			if (m = collapsible(l, r))
				r->in.type = m;
			else {
				/*
				 * Two steps are required.
				 */
				NODE *x = &resc[1];

				*x = *l;
				if (tlen(x) > tlen(r) && ISUNSIGNED(r->in.type))
					putstr("movz");
				else
					putstr("cvt");
				prtype(r);
				prtype(x);
				putchar('\t');
				adrput(r);
				putchar(',');
				adrput(x);
				putchar('\n');
				putchar('\t');
				r = x;
				}
			l->in.type = (ISUNSIGNED(l->in.type) ? UNSIGNED : INT);
			}

		if ((r->in.type == UNSIGNED || r->in.type == ULONG) &&
		    mixtypes(l, r)) {
			int label1, label2;

			label1 = getlab();
			label2 = getlab();

			putstr("movl\t");
			adrput(r);
			putchar(',');
			adrput(l);
			putstr("\n\tjbsc\t$31,");
			adrput(l);
			printf(",L%d\n\tcvtl", label1);
			prtype(l);
			putchar('\t');
			adrput(l);
			putchar(',');
			adrput(l);
			printf("\n\tjbr\tL%d\nL%d:\n\tcvtl", label2, label1);
			prtype(l);
			putchar('\t');
			adrput(l);
			putchar(',');
			adrput(l);
			putstr("\n\tadd");
			prtype(l);
			putstr("2\t$0");
			prtype(l);
			putstr("2.147483648e9,");
			adrput(l);
			printf("\nL%d:", label2);

			return;
			}

		if (!mixtypes(l,r)) {
			if (tlen(l) == tlen(r)) {
				putstr("mov");
#ifdef FORT
				if (Oflag)
					prtype(l);
				else {
					if (l->in.type == DOUBLE)
						putchar('q');
					else if(l->in.type == FLOAT)
						putchar('l');
					else
						prtype(l);
					}
#else
				prtype(l);
#endif FORT
				goto ops;
				}
			else if (tlen(l) > tlen(r) && ISUNSIGNED(r->in.type))
				putstr("movz");
			else
				putstr("cvt");
			}
		else
			putstr("cvt");
		prtype(r);
		prtype(l);
	ops:
		putchar('\t');
		adrput(r);
		putchar(',');
		adrput(l);
		return;
		}

	case 'G':	/* i *= f; asgops with int lhs and float rhs */
		{
		register NODE *l, *r, *s;
		int rt;

		l = p->in.left;
		r = p->in.right;
		s = talloc();
		rt = r->in.type;

		s->in.op = SCONV;
		s->in.left = l;
		s->in.type = rt;
		zzzcode(s, 'A');
		putstr("\n\t");

		hopcode(rt == FLOAT ? 'F' : 'D', p->in.op);
		putstr("2\t");
		adrput(r);
		putchar(',');
		adrput(resc);
		putstr("\n\t");

		s->in.op = ASSIGN;
		s->in.left = l;
		s->in.right = resc;
		s->in.type = l->in.type;
		zzzcode(s, 'A');

		s->in.op = FREE;
		return;
		}

	case 'B':	/* get oreg value in temp register for left shift */
		{
		register NODE *r;
		if (xdebug) eprint(p, 0, &val, &val);
		r = p->in.right;
		if( tlen(r) == sizeof(int) && r->in.type != FLOAT )
			putstr("movl");
		else {
			putstr("cvt");
			prtype(r);
			putchar('l');
			}
		return;
		}

	case 'C':	/* num words pushed on arg stack */
		{
		extern int gc_numbytes;
		extern int xdebug;

		if (xdebug) printf("->%d<-",gc_numbytes);

		printf("$%d", gc_numbytes/(SZLONG/SZCHAR) );
		return;
		}

	case 'D':	/* INCR and DECR */
		zzzcode(p->in.left, 'A');
		putchar('\n');
		putchar('\t');

	case 'E':	/* INCR and DECR, FOREFF */
		if (p->in.right->tn.lval == 1)
			{
			putstr( p->in.op == INCR ? "inc" : "dec" );
			prtype(p->in.left);
			putchar('\t');
			adrput(p->in.left);
			return;
			}
		putstr( p->in.op == INCR ? "add" : "sub" );
		prtype(p->in.left);
		putchar('2');
		putchar('\t');
		adrput(p->in.right);
		putchar(',');
		adrput(p->in.left);
		return;

	case 'F':	/* register type of right operand */
		{
		register NODE *n;
		extern int xdebug;
		register int ty;

		n = getlr( p, 'R' );
		ty = n->in.type;

		if (xdebug) printf("->%d<-", ty);

		if ( ty==DOUBLE)
		    {

		    /*
		     * 	If -M flag is set then use gfloat suffix
		     *
		     */

		    if ( Mflag )			/* slr001 */
			putchar('g');			/* slr001 */
		    else				/* slr001 */
			 putchar('d');			/* slr001 */

		    }

		else if ( ty==FLOAT ) putchar('f');
		else putchar('l');
		return;
		}

	case 'L':	/* type of left operand */
	case 'R':	/* type of right operand */
		{
		register NODE *n;
		extern int xdebug;

		n = getlr( p, c );
		if (xdebug) printf("->%d<-", n->in.type);

		prtype(n);
		return;
		}

	case 'Z':	/* complement mask for bit instr */
		printf("$%ld", ~p->in.right->tn.lval);
		return;

	case 'U':	/* 32 - n, for unsigned right shifts */
		printf("$%d", 32 - p->in.right->tn.lval );
		return;

	case 'T':	/* rounded structure length for arguments */
		{
		int size;

		size = p->stn.stsize;
		SETOFF( size, 4);
		printf("$%d", size);
		return;
		}

	case 'S':  /* structure assignment */
		{
			register NODE *l, *r;
			register size;

			if( p->in.op == STASG ){
				l = p->in.left;
				r = p->in.right;

				}
			else if( p->in.op == STARG ){  /* store an arg into a temporary */
				l = getlr( p, '3' );
				r = p->in.left;
				}
			else cerror( "STASG bad" );

			if( r->in.op == ICON ) r->in.op = NAME;
			else if( r->in.op == REG ) r->in.op = OREG;
			else if( r->in.op != OREG ) cerror( "STASG-r" );

			size = p->stn.stsize;

			if( size <= 0 || size > 65535 )
				cerror("structure size <0=0 or >65535");

			switch(size) {
				case 1:
					putstr("	movb	");
					break;
				case 2:
					putstr("	movw	");
					break;
				case 4:
					putstr("	movl	");
					break;
				case 8:
					putstr("	movq	");
					break;
				default:
					printf("	movc3	$%d,", size);
					break;
			}
			adrput(r);
			putchar(',');
			adrput(l);
			putchar('\n');

			if( r->in.op == NAME ) r->in.op = ICON;
			else if( r->in.op == OREG ) r->in.op = REG;

			}
		break;

	case 'V':	/* slr001 Data type used for float instruction */

		/*
		 *	If Mflag is set it indicates that gfloat instruction
		 *	are wanted
		 *	
		 */

	        if ( Mflag )			/* slr001 */
		    putchar('g');			/* slr001 */
		else					/* slr001 */
		    putchar('d');			/* slr001 */

		break;					/* slr001 */

	case 'W':	
		/* 	vdp004 Data type used for float instruction  either
		 * 	f, d, or g. 
		 *	If Mflag is set it indicates that gfloat instruction
		 *	are wanted
		 *	If fflag is set it indicates that floating point 
		 *	arithmetic should be done in float.
		 *	
		 */

		if (fflag)
		  /* call prtype to give it a chance to print 
		   * out float vdp04 
		   */
		    prtype(p) ;				/* vdp004  */ 
	        else if ( Mflag )			/* vdp004 */
		    putchar('g');			/* vdp004  */
		else					/* vdp004  */
		    putchar('d');			/* vdp004  */

		break;					/* vdp004  */

	default:
		cerror( "illegal zzzcode" );
		}
	}

/*
 * collapsible(dest, src) -- if a conversion with a register destination
 *	can be accomplished in one instruction, return the type of src
 *	that will do the job correctly; otherwise return 0.  Note that
 *	a register must always end up having type INT or UNSIGNED.
 */
int
collapsible(dest, src)
NODE *dest, *src;
{
	int st = src->in.type;
	int dt = dest->in.type;
	int newt = 0;

	/*
	 * Are there side effects of evaluating src?
	 * If the derived type will not be the same size as src,
	 * we may have to use two steps.
	 */
	if (tlen(src) > tlen(dest)) {
		if (tshape(src, STARREG))
			return (0);
		if (src->in.op == OREG && R2TEST(src->tn.rval))
			return (0);
		}

	/*
	 * Can we get an object of dest's type by punning src?
	 * Praises be to great Cthulhu for little-endian machines...
	 */
	if (st == CHAR && dt == USHORT)
		/*
		 * Special case -- we must sign-extend to 16 bits.
		 */
		return (0);

	if (tlen(src) < tlen(dest))
		newt = st;
	else
		newt = dt;

	return (newt);
	}

rmove( rt, rs, t ) TWORD t; {
	printf( "	%s	%s,%s\n",
#ifdef FORT
		!Oflag ? (t==DOUBLE ? "movq" : "movl") :
#endif
		(t==FLOAT ? "movf" :			/* slr001 */
			(t==DOUBLE ? ( Mflag ? "movg" : "movd" ) : "movl")),
		rnames[rs], rnames[rt] );
	}

struct respref
respref[] = {
	INTAREG|INTBREG,	INTAREG|INTBREG,
	INAREG|INBREG,	INAREG|INBREG|SOREG|STARREG|STARNM|SNAME|SCON,
	INTEMP,	INTEMP,
	FORARG,	FORARG,
	INTEMP,	INTAREG|INAREG|INTBREG|INBREG|SOREG|STARREG|STARNM,
	0,	0 };

setregs(){ /* set up temporary registers */
	fregs = 6;	/* tbl- 6 free regs on VAX (0-5) */
	;
	}

szty(t){ 
	/* size, in registers, needed to hold thing of type t
	 *vdp004 for f floats with fflag only need 1 register
	 */

	if (fflag)					 
		return (  (t==DOUBLE) ? 2 : 1 ); 
	else
		return( (t==DOUBLE||t==FLOAT) ? 2 : 1 );
	}

rewfld( p ) NODE *p; {
	return(1);
	}

callreg(p) NODE *p; {
	return( R0 );
	}

base( p ) register NODE *p; {
	register int o = p->in.op;

	if( (o==ICON && p->in.name[0] != '\0')) return( 100 ); /* ie no base reg */
	if( o==REG ) return( p->tn.rval );
    if( (o==PLUS || o==MINUS) && p->in.left->in.op == REG && p->in.right->in.op==ICON)
		return( p->in.left->tn.rval );
    if( o==OREG && !R2TEST(p->tn.rval) && (p->in.type==INT || p->in.type==UNSIGNED || ISPTR(p->in.type)) )
		return( p->tn.rval + 0200*1 );
	if( o==INCR && p->in.left->in.op==REG ) return( p->in.left->tn.rval + 0200*2 );
	if( o==ASG MINUS && p->in.left->in.op==REG) return( p->in.left->tn.rval + 0200*4 );
	if( o==UNARY MUL && p->in.left->in.op==INCR && p->in.left->in.left->in.op==REG
	  && (p->in.type==INT || p->in.type==UNSIGNED || ISPTR(p->in.type)) )
		return( p->in.left->in.left->tn.rval + 0200*(1+2) );
	return( -1 );
	}

offset( p, tyl ) register NODE *p; int tyl; {

	if( tyl==1 &&
	    p->in.op==REG &&
	    (p->in.type==INT || p->in.type==UNSIGNED) )
		return( p->tn.rval );
	if( p->in.op==LS &&
	    p->in.left->in.op==REG &&
	    (p->in.left->in.type==INT || p->in.left->in.type==UNSIGNED) &&
	    p->in.right->in.op==ICON &&
	    p->in.right->in.name[0]=='\0' &&
	    (1<<p->in.right->tn.lval)==tyl)
		return( p->in.left->tn.rval );
	return( -1 );
	}

makeor2( p, q, b, o) register NODE *p, *q; register int b, o; {
	register NODE *t;
	register int i;
	NODE *f;

	p->in.op = OREG;
	f = p->in.left; 	/* have to free this subtree later */

	/* init base */
	switch (q->in.op) {
		case ICON:
		case REG:
		case OREG:
			t = q;
			break;

		case MINUS:
			q->in.right->tn.lval = -q->in.right->tn.lval;
		case PLUS:
			t = q->in.right;
			break;

		case INCR:
		case ASG MINUS:
			t = q->in.left;
			break;

		case UNARY MUL:
			t = q->in.left->in.left;
			break;

		default:
			cerror("illegal makeor2");
	}

	p->tn.lval = t->tn.lval;
#ifndef FLEXNAMES
	for(i=0; i<NCHNAM; ++i)
		p->in.name[i] = t->in.name[i];
#else
	p->in.name = t->in.name;
#endif

	/* init offset */
	p->tn.rval = R2PACK( (b & 0177), o, (b>>7) );

	tfree(f);
	return;
	}

canaddr( p ) NODE *p; {
	register int o = p->in.op;

	if( o==NAME || o==REG || o==ICON || o==OREG || (o==UNARY MUL && shumul(p->in.left)) ) return(1);
	return(0);
	}

shltype( o, p ) register NODE *p; {
	return( o== REG || o == NAME || o == ICON || o == OREG || ( o==UNARY MUL && shumul(p->in.left)) );
	}

flshape( p ) register NODE *p; {
	return( p->in.op == REG || p->in.op == NAME || p->in.op == ICON ||
		(p->in.op == OREG && (!R2TEST(p->tn.rval) || tlen(p) == 1)) );
	}

shtemp( p ) register NODE *p; {
	if( p->in.op == STARG ) p = p->in.left;
	return( p->in.op==NAME || p->in.op ==ICON || p->in.op == OREG || (p->in.op==UNARY MUL && shumul(p->in.left)) );
	}

shumul( p ) register NODE *p; {
	register o;
	extern int xdebug;

	if (xdebug) {
		 printf("\nshumul:op=%d,lop=%d,rop=%d", p->in.op, p->in.left->in.op, p->in.right->in.op);
		printf(" prname=%s,plty=%d, prlval=%D\n", p->in.right->in.name, p->in.left->in.type, p->in.right->tn.lval);
		}


	o = p->in.op;
	if( o == NAME || (o == OREG && !R2TEST(p->tn.rval)) || o == ICON ) return( STARNM );

	if( ( o == INCR || o == ASG MINUS ) &&
	    ( p->in.left->in.op == REG && p->in.right->in.op == ICON ) &&
	    p->in.right->in.name[0] == '\0' )
		{
		switch (p->in.type)
			{
			case CHAR|PTR:
			case UCHAR|PTR:
				o = 1;
				break;

			case SHORT|PTR:
			case USHORT|PTR:
				o = 2;
				break;

			case INT|PTR:
			case UNSIGNED|PTR:
			case LONG|PTR:
			case ULONG|PTR:
			case FLOAT|PTR:
				o = 4;
				break;

			case DOUBLE|PTR:
				o = 8;
				break;

			default:
				/*
				 * To fix the problem when the increment
				 * of a structure class is a register
				 * and is part of a condition test.
				 */

				if ( (ISPTR(p->in.left->in.type)) && /*RAP003*/
				     (p->in.type == p->in.left->in.type))
				    {
				    o = p->in.right->tn.lval;
				    if (o==1|o==2|o==4|o==8)
					return(STARREG);
				    }
				return(0);
			}
		return( p->in.right->tn.lval == o ? STARREG : 0);
		}

	return( 0 );
	}

adrcon( val ) CONSZ val; {
	putchar( '$' );
	printf( CONFMT, val );
	}

conput( p ) register NODE *p; {
	switch( p->in.op ){

	case ICON:
		acon( p );
		return;

	case REG:
		putstr( rnames[p->tn.rval] );
		return;

	default:
		cerror( "illegal conput" );
		}
	}

insput( p ) register NODE *p; {
	cerror( "insput" );
	}

upput( p ) register NODE *p; {
	cerror( "upput" );
	}

adrput( p ) register NODE *p; {
	register int r;
	/* output an address, with offsets, from p */

	if( p->in.op == FLD ){
		p = p->in.left;
		}
	switch( p->in.op ){

	case NAME:
		acon( p );
		return;

	case ICON:
		/* addressable value of the constant */
		putchar( '$' );
		acon( p );
		return;

	case REG:
		putstr( rnames[p->tn.rval] );
		return;

	case OREG:
		r = p->tn.rval;
		if( R2TEST(r) ){ /* double indexing */
			register int flags;

			flags = R2UPK3(r);
			if( flags & 1 ) putchar('*');
			if( flags & 4 ) putchar('-');
			if( p->tn.lval != 0 || p->in.name[0] != '\0' ) acon(p);
			if( R2UPK1(r) != 100) printf( "(%s)", rnames[R2UPK1(r)] );
			if( flags & 2 ) putchar('+');
			printf( "[%s]", rnames[R2UPK2(r)] );
			return;
			}
		if( r == AP ){  /* in the argument region */
			if( p->tn.lval <= 0 || p->in.name[0] != '\0' ) werror( "bad arg temp" );
			printf( CONFMT, p->tn.lval );
			putstr( "(ap)" );
			return;
			}
		if( p->tn.lval != 0 || p->in.name[0] != '\0') acon( p );
		printf( "(%s)", rnames[p->tn.rval] );
		return;

	case UNARY MUL:
		/* STARNM or STARREG found */
		if( tshape(p, STARNM) ) {
			putchar( '*' );
			adrput( p->in.left);
			}
		else {	/* STARREG - really auto inc or dec */
			register NODE *q;

/* tbl
			p = p->in.left;
			p->in.left->in.op = OREG;
			if( p->in.op == INCR ) {
				adrput( p->in.left );
				putchar( '+' );
				}
			else {
				putchar( '-' );
				adrput( p->in.left );
				}
   tbl */
			q = p->in.left;
			printf("%s(%s)%s", (q->in.op==INCR ? "" : "-"),
				rnames[q->in.left->tn.rval], 
				(q->in.op==INCR ? "+" : "") );
			p->in.op = OREG;
			p->tn.rval = q->in.left->tn.rval;
			p->tn.lval = (q->in.op == INCR ? -q->in.right->tn.lval : 0);
#ifndef FLEXNAMES
			p->in.name[0] = '\0';
#else
			p->in.name = "";
#endif
			tfree(q);
		}
		return;

	default:
		cerror( "illegal address" );
		return;

		}

	}

acon( p ) register NODE *p; { /* print out a constant */

	if( p->in.name[0] == '\0' ){
		printf( CONFMT, p->tn.lval);
		}
	else if( p->tn.lval == 0 ) {
#ifndef FLEXNAMES
		printf( "%.8s", p->in.name );
#else
		putstr( p->in.name );
#endif
		}
	else {
#ifndef FLEXNAMES
		printf( "%.8s+", p->in.name );
#else
		printf( "%s+", p->in.name );
#endif
		printf( CONFMT, p->tn.lval );
		}
	}

/*
aacon( p ) register NODE *p; { /* print out a constant */
/*

	if( p->in.name[0] == '\0' ){
		printf( CONFMT, p->tn.lval);
		return( 0 );
		}
	else if( p->tn.lval == 0 ) {
#ifndef FLEXNAMES
		printf( "$%.8s", p->in.name );
#else
		printf( "$%s", p->in.name );
#endif
		return( 1 );
		}
	else {
		printf( "$(" );
		printf( CONFMT, p->tn.lval );
		printf( "+" );
#ifndef FLEXNAMES
		printf( "%.8s)", p->in.name );
#else
		printf( "%s)", p->in.name );
#endif
		return(1);
		}
	}
 */

genscall( p, cookie ) register NODE *p; {
	/* structure valued call */
	return( gencall( p, cookie ) );
	}

/* tbl */
int gc_numbytes;
/* tbl */

gencall( p, cookie ) register NODE *p; {
	/* generate the call given by p */
	register NODE *p1, *ptemp;
	register temp, temp1;
	register m;

	if( p->in.right ) temp = argsize( p->in.right );
	else temp = 0;

	if( p->in.op == STCALL || p->in.op == UNARY STCALL ){
		/* set aside room for structure return */

		if( p->stn.stsize > temp ) temp1 = p->stn.stsize;
		else temp1 = temp;
		}

	if( temp > maxargs ) maxargs = temp;
	SETOFF(temp1,4);

	if( p->in.right ){ /* make temp node, put offset in, and generate args */
		ptemp = talloc();
		ptemp->in.op = OREG;
		ptemp->tn.lval = -1;
		ptemp->tn.rval = SP;
#ifndef FLEXNAMES
		ptemp->in.name[0] = '\0';
#else
		ptemp->in.name = "";
#endif
		ptemp->in.rall = NOPREF;
		ptemp->in.su = 0;
		genargs( p->in.right, ptemp );
		ptemp->in.op = FREE;
		}

	p1 = p->in.left;
	if( p1->in.op != ICON ){
		if( p1->in.op != REG ){
			if( p1->in.op != OREG || R2TEST(p1->tn.rval) ){
				if( p1->in.op != NAME ){
					order( p1, INAREG );
					}
				}
			}
		}

/*
	if( p1->in.op == REG && p->tn.rval == R5 ){
		cerror( "call register overwrite" );
		}
 */
/* tbl
	setup gc_numbytes so reference to ZC works */

	gc_numbytes = temp&(0x3ff);
/* tbl */

	p->in.op = UNARY CALL;
	m = match( p, INTAREG|INTBREG );

	/* compensate for deficiency in 'ret' instruction ... wah,kre */
	/* (plus in assignment to gc_numbytes above, for neatness only) */
	if (temp >= 1024)
		printf("	addl2	$%d,sp\n", (temp&(~0x3ff)));

/* tbl
	switch( temp ) {
	case 0:
		break;
	case 2:
		printf( "	tst	(sp)+\n" );
		break;
	case 4:
		printf( "	cmp	(sp)+,(sp)+\n" );
		break;
	default:
		printf( "	add	$%d,sp\n", temp);
		}
   tbl */
	return(m != MDONE);
	}

/* tbl */
char *
ccbranches[] = {
	"	jeql	L%d\n",
	"	jneq	L%d\n",
	"	jleq	L%d\n",
	"	jlss	L%d\n",
	"	jgeq	L%d\n",
	"	jgtr	L%d\n",
	"	jlequ	L%d\n",
	"	jlssu	L%d\n",
	"	jgequ	L%d\n",
	"	jgtru	L%d\n",
	};
/* tbl */

cbgen( o, lab, mode ) { /*   printf conditional and unconditional branches */

/* tbl */
	if( o == 0 ) printf( "	jbr	L%d\n", lab );
/* tbl */
	else {
		if( o > UGT ) cerror( "bad conditional branch: %s", opst[o] );
		printf( ccbranches[o-EQ], lab );
		}
	}

nextcook( p, cookie ) NODE *p; {
	/* we have failed to match p with cookie; try another */
	if( cookie == FORREW ) return( 0 );  /* hopeless! */
	if( !(cookie&(INTAREG|INTBREG)) ) return( INTAREG|INTBREG );
	if( !(cookie&INTEMP) && asgop(p->in.op) ) return( INTEMP|INAREG|INTAREG|INTBREG|INBREG );
	return( FORREW );
	}

lastchance( p, cook ) NODE *p; {
	/* forget it! */
	return(0);
	}

optim2( p ) register NODE *p; {
	/* do local tree transformations and optimizations */

	register NODE *l, *r;

	switch( p->in.op ) {

	case AND:
		/* commute L and R to eliminate complements and constants */
		if( (l = p->in.left)->in.op == ICON && l->in.name[0] == 0 ||
		    l->in.op == COMPL ) {
			p->in.left = p->in.right;
			p->in.right = l;
			}
	case ASG AND:
		/* change meaning of AND to ~R&L - bic on pdp11 */
		r = p->in.right;
		if( r->in.op==ICON && r->in.name[0]==0 ) { /* complement constant */
			r->tn.lval = ~r->tn.lval;
			switch(p->in.type) {
			case CHAR:
			case UCHAR:
				r->tn.lval = (char)r->tn.lval;
				break;
			case SHORT:
			case USHORT:
				r->tn.lval = (short)r->tn.lval;
				break;
			default:
				break;
			}
		} else if( r->in.op==COMPL ) { /* ~~A => A */
			r->in.op = FREE;
			p->in.right = r->in.left;

		} else { /* insert complement node */
			p->in.right = talloc();
			p->in.right->in.op = COMPL;
			p->in.right->in.rall = NOPREF;
			p->in.right->in.type = r->in.type;
			p->in.right->in.left = r;
			p->in.right->in.right = NULL;
		}
		break;

		}
	}

NODE * addroreg(l) NODE *l;
				/* OREG was built in clocal()
				 * for an auto or formal parameter
				 * now its address is being taken
				 * local code must unwind it
				 * back to PLUS/MINUS REG ICON
				 * according to local conventions
				 */
{
	cerror("address of OREG taken");
	/*NOTREACHED*/
}



# ifndef ONEPASS
main( argc, argv ) char *argv[]; {
	return( mainp2( argc, argv ) );
	}
# endif


/* added by jwf */
struct functbl {
	int fop;
	TWORD ftype;
	char *func;
	} opfunc[] = {
	DIV,		TANY,	"udiv",
	MOD,		TANY,	"urem",
	ASG DIV,	TANY,	"udiv",
	ASG MOD,	TANY,	"urem",
	0,	0,	0 };

hardops(p)  register NODE *p; {
	/* change hard to do operators into function calls.  */
	register NODE *q;
	register struct functbl *f;
	register o;
	register TWORD t;

	o = p->in.op;
	t = p->in.type;
	if( t!=UNSIGNED && t!=ULONG ) return;

	for( f=opfunc; f->fop; f++ ) {
		if( o==f->fop ) goto convert;
		}
	return;

	/* need to rewrite tree for ASG OP */
	/* must change ASG OP to a simple OP */
	convert:
	if( asgop( o ) ) {
		q = talloc();
		switch( p->in.op ) {
			case ASG DIV:
				q->in.op = DIV;
				break;
			case ASG MOD:
				q->in.op = MOD;
				break;
		}
		q->in.rall = NOPREF;
		q->in.type = p->in.type;
		q->in.left = tcopy(p->in.left);
		q->in.right = p->in.right;
		p->in.op = ASSIGN;
		p->in.right = q;
		zappost(q->in.left); /* remove post-INCR(DECR) from new node */
		fixpre(q->in.left);	/* change pre-INCR(DECR) to +/-	*/
		p = q;

	}

	/* build comma op for args to function */
	q = talloc();
	q->in.op = CM;
	q->in.rall = NOPREF;
	q->in.type = INT;
	q->in.left = p->in.left;
	q->in.right = p->in.right;
	p->in.op = CALL;
	p->in.right = q;

	/* put function name in left node of call */
	p->in.left = q = talloc();
	q->in.op = ICON;
	q->in.rall = NOPREF;
	q->in.type = INCREF( FTN + p->in.type );
#ifndef FLEXNAMES
	strcpy( q->in.name, f->func );
#else
	q->in.name = f->func;
#endif
	q->tn.lval = 0;
	q->tn.rval = 0;

	return;

	}

zappost(p) NODE *p; {
	/* look for ++ and -- operators and remove them */

	register o, ty;
	register NODE *q;
	o = p->in.op;
	ty = optype( o );

	switch( o ){

	case INCR:
	case DECR:
			q = p->in.left;
			p->in.right->in.op = FREE;  /* zap constant */
			ncopy( p, q );
			q->in.op = FREE;
			return;

		}

	if( ty == BITYPE ) zappost( p->in.right );
	if( ty != LTYPE ) zappost( p->in.left );
}

fixpre(p) NODE *p; {

	register o, ty;
	o = p->in.op;
	ty = optype( o );

	switch( o ){

	case ASG PLUS:
			p->in.op = PLUS;
			break;
	case ASG MINUS:
			p->in.op = MINUS;
			break;
		}

	if( ty == BITYPE ) fixpre( p->in.right );
	if( ty != LTYPE ) fixpre( p->in.left );
}

myreader(p) register NODE *p; {
	walkf( p, hardops );	/* convert ops to function calls */
	canon( p );		/* expands r-vals for fields */
	walkf( p, optim2 );
	/* jwf toff = 0;  /* stack offset swindle */
	}
