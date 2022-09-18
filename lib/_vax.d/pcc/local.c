#ifndef lint
static char *sccsid = "@(#)local.c	4.1	ULTRIX	11/23/87";
#endif lint

/************************************************************************
 *		Modification History
 *	@(#)local.c	1.2 (Berkeley) 12/18/82
 *
 * 009	Lu Anne Van de Pas Aug-18-1986   
 *	Conditionalized changing Dcon to on Fcon; do only with -f flag
 *
 * 008	David L Ballenger, 03-Apr-1986
 *	Fix problem with casting unsigned values to ints.
 *
 *	David L Ballenger, 18-Mar-1986
 * 007	Add fix for casts of enumeration variables which was broken by
 *	the SUN fixes.
 *
 *	Lu Anne Van de Pas 23-Sept-86				
 * 006- Add code to return floating constant for floating instead of 
 *      returning a double.
 * 
 *	Rich Phillips, 13-Sept-84
 * 005- Remove code added for fix004. It wasn't catching every case and
 *	it would be too messy to make it. Lint catches taking the address
 *	of a register, even when the compiler won't use a register.
 *
 *	Rich Phillips, 20-July-84
 * 004- Transfer SWASREG to NWASREG when NAME nodes are converted to 
 * 	structure references.
 *
 *      Rich Phillips, Dave Ballenger, 29-May-84
 * 003- Don't remove SCONV when converting from an integer constant, that
 *      represents an address, to something smaller, like a SHORT or CHAR.
 *      The conversion should be left in and be handled by the tables.
 *
 *	Stephen Reilly, 19-Dec-83
 * 002- A fix was made so that fields that are of unsigned type and
 *	have the same length as the convert, clobbers the convert node
 *	rather than inheriting the convert node attributes.
 *
 *	Stephen Reilly, 21-Oct-83:
 * 001- New flag -M must be checked so that we will output the
 *	correct floating instructions.
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

# include "mfile1"

/*	this file contains code which is dependent on the target machine */

/*
 *	If set the Mflag indicates that the user want gfloat instructions
 */
extern int Mflag;					/* slr001 -M flag */
extern int fflag; 					/* vdp006 - f flag */ 
NODE *
cast( p, t ) register NODE *p; TWORD t; {
	/* cast node p to type t */

	p = buildtree( CAST, block( NAME, NIL, NIL, t, 0, (int)t ), p );
	p->in.left->in.op = FREE;
	p->in.op = FREE;
	return( p->in.right );
	}

NODE *
clocal(p) NODE *p; {

	/* this is called to do local transformations on
	   an expression tree preparitory to its being
	   written out in intermediate code.
	*/

	/* the major essential job is rewriting the
	   automatic variables and arguments in terms of
	   REG and OREG nodes */
	/* conversion ops which are not necessary are also clobbered here */
	/* in addition, any special features (such as rewriting
	   exclusive or) are easily handled here as well */

	register struct symtab *q;
	register NODE *r;
	register o;
	register m, ml;

	switch( o = p->in.op ){

	case NAME:
		if( p->tn.rval < 0 ) { /* already processed; ignore... */
			return(p);
			}
		q = &stab[p->tn.rval];
		switch( q->sclass ){

		case AUTO:
		case PARAM:
			/* fake up a structure reference */
			r = block( REG, NIL, NIL, PTR+STRTY, 0, 0 );
			r->tn.lval = 0;
			r->tn.rval = (q->sclass==AUTO?STKREG:ARGREG);
			p = stref( block( STREF, r, p, 0, 0, 0 ) );
			break;

		case ULABEL:
		case LABEL:
		case STATIC:
			if( q->slevel == 0 ) break;
			p->tn.lval = 0;
			p->tn.rval = -q->offset;
			break;

		case REGISTER:
			p->in.op = REG;
			p->tn.lval = 0;
			p->tn.rval = q->offset;
			break;

			}
		break;

	case PCONV:
		/* do pointer conversions for char and longs */
		ml = p->in.left->in.type;
		if( ( ml==CHAR || ml==UCHAR || ml==SHORT || ml==USHORT ) && p->in.left->in.op != ICON ) break;

		/* pointers all have the same representation; the type is inherited */

	inherit:
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );


	case SCONV:
		m = (p->in.type == FLOAT || p->in.type == DOUBLE );
		ml = (p->in.left->in.type == FLOAT || p->in.left->in.type == DOUBLE );
		o = p->in.left->in.op;
		if ( !(fflag) && ( m != ml)) break; 		/*009*/
		if ((o == FCON || o == DCON) && ml && !m ) {
			/* floating type to an int type  - change to the 
			 * floating type to a ICON node and cast the value
			 * to an int  -lvdp006
		         */ 
			r= block (ICON, NIL, NIL, INT, 0, 0); 
			if (o == FCON) 
				r->tn.lval = (int) p->in.left->fpn.fval;
			else
				r->tn.lval = (int) p->in.left->dpn.dval;
			r->tn.rval = NONAME;
			p->in.left->in.op = FREE;
			p->in.left = r; 
			}
		else 
			if ( ((fflag) && (ml || m ))  || (m != ml)  )  
				/* vdp006 We are also done if floating flag
				 * and either p->in.op and p->in.left->in.type
				 * are FLOAT or DOUBLE 
				 */ 
				break;

		/* now, look for conversions downwards */

		m = p->in.type;
		ml = p->in.left->in.type;
		if( p->in.left->in.op == ICON ){ /* simulate the conversion here */
			CONSZ val;
			if (ISPTR(ml) && (m == CHAR || m == SHORT || m == UCHAR
				|| m == USHORT)) break; /* RAP003 */
			val = p->in.left->tn.lval;
			switch( m ){
			case CHAR:
				p->in.left->tn.lval = (char) val;
				break;
			case UCHAR:
				p->in.left->tn.lval = val & 0XFF;
				break;
			case USHORT:
				p->in.left->tn.lval = val & 0XFFFFL;
				break;
			case SHORT:
				p->in.left->tn.lval = (short)val;
				break;
			case UNSIGNED:
				p->in.left->tn.lval = val & 0xFFFFFFFFL;
				break;
			case INT:
				p->in.left->tn.lval = (int)val;
				break;
				}
			p->in.left->in.type = m;

		} else if ((m == ENUMTY) || (ml == ENUMTY)) {
			/* 
			 * If the conversion is to or from an enumeration,
			 * we need to hold onto the conversion operation
			 * a while longer so we can generate correct code
			 * and do type checking.
			 */
			return(p);

		} else if( m==CHAR || m==UCHAR ){
			/*
			 * Converting something larger to a "char"
			 * means we have to keep the conversion around
			 * awhile to generate the correct code and do
			 * type checking.
			 */
			if( ml!=CHAR && ml!= UCHAR )
				return(p);

		} else if( m==SHORT || m==USHORT ) {
			/*
			 * Converting something larger to a "short"
			 * means we have to keep the conversion around
			 * awhile to generate the correct code.
			 */
			if( ml!=CHAR && ml!=UCHAR && ml!=SHORT && ml!=USHORT )
				return(p);
		}

		/* clobber conversion */

		/*
		 *		SLR002
		 *  if we are trying to convert a field, whose type
		 *  is unsigned and of the same size, don't inhert
		 *  the convert attributes. Rather just clobber the
		 *  convert. 
		 *
		 *  The example below is the reason for the fix:
		 *  a = (long)(unsigned long)<unsigned field reference>
		 *  The code generated is a signed instruction rather
		 *  than an unsigned instruction
		 */
		if( tlen(p) == tlen(p->in.left) && (p->in.left->in.op != FLD))
		      goto inherit;

		/* Clobber the conversion, we don't really need to
		 * do it.
		 */
		p->in.op = FREE;
		return( p->in.left );

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) cerror( "bad conversion", 0);
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

	case RS:
	case ASG RS:
		/* convert >> to << with negative shift count */
		/* only if type of left operand is not unsigned */

		if( ISUNSIGNED(p->in.left->in.type) ) break;
		p->in.right = buildtree( UNARY MINUS, p->in.right, NIL );
		if( p->in.op == RS ) p->in.op = LS;
		else p->in.op = ASG LS;
		break;

	case FLD:
		/* make sure that the second pass does not make the
		   descendant of a FLD operator into a doubly indexed OREG */

		if( p->in.left->in.op == UNARY MUL
				&& (r=p->in.left->in.left)->in.op == PCONV)
			if( r->in.left->in.op == PLUS || r->in.left->in.op == MINUS ) 
				if( ISPTR(r->in.type) ) {
					if( ISUNSIGNED(p->in.left->in.type) )
						p->in.left->in.type = UCHAR;
					else
						p->in.left->in.type = CHAR;
				}
		break;
		}

	return(p);
	}

andable( p ) NODE *p; {
	return(1);  /* all names can have & taken on them */
	}

cendarg(){ /* at the end of the arguments of a ftn, set the automatic offset */
	autooff = AUTOINIT;
	}

cisreg( t ) TWORD t; { /* is an automatic variable of type t OK for a register variable */

#ifdef TRUST_REG_CHAR_AND_REG_SHORT
	if( t==INT || t==UNSIGNED || t==LONG || t==ULONG	/* tbl */
		|| t==CHAR || t==UCHAR || t==SHORT 		/* tbl */
		|| t==USHORT || ISPTR(t)) return(1);		/* tbl */
	else if (fflag && t==FLOAT) return (1); 		/* vdp006 */ 
#else
	if( t==INT || t==UNSIGNED || t==LONG || t==ULONG	/* wnj */
		|| ISPTR(t)) return (1);			/* wnj */
	else if (fflag && t==FLOAT) return (1); 		/* vdp006 */ 
#endif
	return(0);
	}

NODE *
offcon( off, t, d, s ) OFFSZ off; TWORD t; {

	/* return a node, for structure references, which is suitable for
	   being added to a pointer of type t, in order to be off bits offset
	   into a structure */

	register NODE *p;

	/* t, d, and s are the type, dimension offset, and sizeoffset */
	/* in general they  are necessary for offcon, but not on H'well */

	p = bcon(0);
	p->tn.lval = off/SZCHAR;
	return(p);

	}


static inwd	/* current bit offsed in word */;
static word	/* word being built from fields */;

incode( p, sz ) register NODE *p; {

	/* generate initialization code for assigning a constant c
		to a field of width sz */
	/* we assume that the proper alignment has been obtained */
	/* inoff is updated to have the proper final value */
	/* we also assume sz  < SZINT */

	if((sz+inwd) > SZINT) cerror("incode: field > int");
	word |= ((unsigned)(p->tn.lval<<(32-sz))) >> (32-sz-inwd);
	inwd += sz;
	inoff += sz;
	if(inoff%SZINT == 0) {
		printf( "	.long	0x%x\n", word);
		word = inwd = 0;
		}
	}

fincode( d, sz ) double d; {
	/* output code to initialize space of size sz to the value d */
	/* the proper alignment has been obtained */
	/* inoff is updated to have the proper final value */
	/* on the target machine, write it out in octal! */

	/*
	 *	If the Mflag is set make sure that real constants are
	 *	gfloat format.  Otherwise make it dfloat.
	 */

	printf("	%s	0%c%.20e\n", 
		sz == SZDOUBLE ? ( Mflag ? ".gfloat" : ".double" ) : ".float",

		sz == SZDOUBLE ? ( Mflag ? 'g' : 'd' ) : 'f', d);   /* slr001 */
	inoff += sz;
	}

cinit( p, sz ) NODE *p; {
	/* arrange for the initialization of p into a space of
	size sz */
	/* the proper alignment has been opbtained */
	/* inoff is updated to have the proper final value */
	ecode( p );
	inoff += sz;
	}

vfdzero( n ){ /* define n bits of zeros in a vfd */

	if( n <= 0 ) return;

	inwd += n;
	inoff += n;
	if( inoff%ALINT ==0 ) {
		printf( "	.long	0x%x\n", word );
		word = inwd = 0;
		}
	}

char *
exname( p ) char *p; {
	/* make a name look like an external name in the local machine */

#ifndef FLEXNAMES
	static char text[NCHNAM+1];
#else
	static char text[BUFSIZ+1];
#endif

	register i;

	text[0] = '_';
#ifndef FLEXNAMES
	for( i=1; *p&&i<NCHNAM; ++i ){
#else
	for( i=1; *p; ++i ){
#endif
		text[i] = *p++;
		}

	text[i] = '\0';
#ifndef FLEXNAMES
	text[NCHNAM] = '\0';  /* truncate */
#endif

	return( text );
	}

ctype( type ){ /* map types which are not defined on the local machine */
	switch( BTYPE(type) ){

	case LONG:
		MODTYPE(type,INT);
		break;

	case ULONG:
		MODTYPE(type,UNSIGNED);
		}
	return( type );
	}

noinit( t ) { /* curid is a variable which is defined but
	is not initialized (and not a function );
	This routine returns the stroage class for an uninitialized declaration */

	return(EXTERN);

	}

commdec( id ){ /* make a common declaration for id, if reasonable */
	register struct symtab *q;
	OFFSZ off, tsize();

	q = &stab[id];
	printf( "	.comm	%s,", exname( q->sname ) );
	off = tsize( q->stype, q->dimoff, q->sizoff );
	printf( CONFMT, off/SZCHAR );
	printf( "\n" );
	}

isitlong( cb, ce ){ /* is lastcon to be long or short */
	/* cb is the first character of the representation, ce the last */

	if( ce == 'l' || ce == 'L' ||
		lastcon >= (1L << (SZINT-1) ) ) return (1);
	return(0);
	}


/* Added both return cvtfloat and cvtdouble to replace 
 * function isitfloat.  Return FCON for floating constant and 
 * DCON for double constant. lvdp006 
 */ 

cvtfloat(s) 
char *s; 

{	double atof(); 

	fcon =  atof(s); 
	return (FCON);
}

cvtdouble(s) 
char *s; 

{
	double atof();

	dcon = atof(s); 
	return (DCON);
}


ecode( p ) NODE *p; {

	/* walk the tree and write out the nodes.. */

	if( nerrors ) return;
	p2tree( p );
	p2compile( p );
	}

#ifndef ONEPASS
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
#endif
