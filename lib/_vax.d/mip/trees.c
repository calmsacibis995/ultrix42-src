#ifndef lint
static char *sccsid = "@(#)trees.c	4.3	ULTRIX	2/4/91";
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
 *			Modification History
 *
 *
 * 018	Jon Reeves, 10-May-1989
 *	Signal handler now returns void.
 *
 * 017	Jon Reeves, 03-May-1989 for Sid Maxwell
 *	Clean up handling of void * in : expressions.
 *
 * 016	Jon Reeves, 11-Apr-1988
 *	Add ifndef econvert/endif so lint builds cleanly.
 *
 * 015	David L Ballenger, 22-Mar-1987
 *	Fix problem with SUN fixes added in 010 that caused the following:
 *		if ((unsigned)char_variable <= 127)
 *	to generate a signed comparison and branch rather than unsigned.
 *
 * 014  Lu Anne Van de Pas, 14-Aug-86
 *	Fixed bug with asgop; ie.  if (int < (int= float * double / float)).
 *	The intermediate results of the compuations are used again for the 
 *	comparsion so we can't optimize it.      
 *
 * 013  Victoria Holt, 14-Apr-86
 *	Bug fix for constant pointers.
 *
 * 012	Lu Anne Van de Pas, 1-April-86
 * 	Added floating point exception handler from 4.3, to report 
 *	floating point overflow and underflow faults when folding constants
 *	together. 
 *
 *	David L Ballenger, 18-Mar-1986
 * 011	Add fix for casts of enumeration variables which was broken by
 *	the SUN fixes.
 *
 *	David L Ballenger, 16-Mar-1986
 * 010	Add bug fixes from SUN for NFS code.
 *
 * 	Lu Anne Van de Pas, 02-Mar-86
 * 009  Added support for single precision arithmetic to be done in 
 *	singleprec if fflag is set
 *
 *	Victoria Holt, 26-Feb-86
 * 008	Added support for const and volatile.
 *
 *	David L Ballenger, 28-May-1985
 * 007	Fix bug that prevented assignments between variables defined as
 *	void (*)(), i.e. pointer to function returning void.
 *
 *	Rich Phillips, 13-Sept-84
 * 006- Back out of 003, see local.c for reason (in mod history)
 *
 *	Rich Phillips, 21-Aug-84
 * 005- Issue an error when functions that do not return a value are
 *	used in a value required context.
 *
 *	Rich Phillips, 01-Aug-84
 * 004- Issue an error if a structure or union type was specified in a
 *	condition, like "if (a)" where "a" is a union or structure.
 *
 *	Rich Phillips, 20-July-84
 * 003-	Issue an error if unary & operator applies to a unary * with 
 *	NWASREG set.
 *
 *	Stephen Reilly, 13-Feb-84
 * 002- Fix a coding bug from ( 001 ).  The problem was I thought asgop
 *	only was true for assign type operations.  Not true the RETURN
 *	CAST are considered assign typea which causes conversion problems.
 *
 *	Stephen Reilly, 20-Jan-84
 * 001- Fix the problem with ( int op= real ). The bug was that the
 *	real was converted to the int rather than the int being 
 *	converted to real.  
 *
 ***********************************************************************/

# include "mfile1"
# include <setjmp.h>

	    /* corrections when in violation of lint */

/*	some special actions, used in finding the type of nodes */
# define NCVT 01
# define PUN 02
# define TYPL 04
# define TYPR 010
# define TYMATCH 040
# define LVAL 0100
# define CVTO 0200
# define CVTL 0400
# define CVTR 01000
# define PTMATCH 02000
# define OTHER 04000
# define NCVTR 010000

/* node conventions:

	NAME:	rval>0 is stab index for external
		rval<0 is -inlabel number
		lval is offset in bits
	ICON:	lval has the value
		rval has the STAB index, or - label number,
			if a name whose address is in the constant
		rval = NONAME means no name
	REG:	rval is reg. identification cookie

	*/

int bdebug = 0;
extern ddebug;
extern fflag; 	/*vdp009*/ 


extern int adebug; 
extern int eprint();


#ifndef BUG1
/* vdp009 - adebug flag. Print out actions or conversions that
 * need to be done to the tree
 */ 
printact(t, acts)	
	NODE *t;
	int acts;
{
	static struct actions {
		int	a_bit;
		char	*a_name;
	} actions[] = {
		{ PUN,		"PUN" },
		{ CVTL,		"CVTL" },
		{ CVTR,		"CVTR" },
		{ TYPL,		"TYPL" },
		{ TYPR,		"TYPR" },
		{ TYMATCH,	"TYMATCH" },
		{ PTMATCH,	"PTMATCH" },
		{ LVAL,		"LVAL" },
		{ CVTO,		"CVTO" },
		{ NCVT,		"NCVT" },
		{ OTHER,	"OTHER" },
		{ NCVTR,	"NCVTR" },
		{ 0 }
	};
	register struct actions *p;
	char *sep = " ";

	printf("actions");
	for (p = actions; p->a_name; p++)
		if (p->a_bit & acts) {
			printf("%s%s", sep, p->a_name);
			sep = "|";
		}
	if (!bdebug) {
		printf(" for:\n");
		fwalk(t, eprint, 0);
	} else
		putchar('\n');
}
#endif

NODE *
buildtree( o, l, r ) register NODE *l, *r; {
	register NODE *p, *q;
	register actions;
	register opty;
	register struct symtab *sp;
	register NODE *lr, *ll;
	int i;

# ifndef BUG1
	if( bdebug ) printf( "buildtree( %s, %o, %o )\n", opst[o], l, r );
# endif
	opty = optype(o);

	/* check for constants */

	if( opty == UTYPE && l->in.op == ICON ){

		switch( o ){

		case NOT:
			if( hflag ) werror( "constant argument to NOT" );
		case UNARY MINUS:
		case COMPL:
			if( conval( l, o, l ) ) return(l);
			break;

			}
		}
			
	/* lvdp009 check for both Floating constant and double
	 * constant; combine unary minus node and constant node
	 */ 

	else if( o==UNARY MINUS && l->in.op==FCON ){

		l->fpn.fval = -l->fpn.fval;
		return(l);
		}

	else if( o==UNARY MINUS && l->in.op==DCON ){
		l->dpn.dval = -l->dpn.dval;
		return(l);
		}

	/* lvdp009 end */ 


	else if( o==QUEST && l->in.op==ICON ) {
		l->in.op = FREE;
		r->in.op = FREE;
		if( l->tn.lval ){
			tfree( r->in.right );
			return( r->in.left );
			}
		else {
			tfree( r->in.left );
			return( r->in.right );
			}
		}

	else if( (o==ANDAND || o==OROR) && (l->in.op==ICON||r->in.op==ICON) ) goto ccwarn;

	else if( opty == BITYPE && l->in.op == ICON && r->in.op == ICON ){

		switch( o ){

		case ULT:
		case UGT:
		case ULE:
		case UGE:
		case LT:
		case GT:
		case LE:
		case GE:
		case EQ:
		case NE:
		case ANDAND:
		case OROR:
		case CBRANCH:

		ccwarn:
			if( hflag ) werror( "constant in conditional context" );

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case MOD:
		case AND:
		case OR:
		case ER:
		case LS:
		case RS:
			if( conval( l, o, r ) ) {
				r->in.op = FREE;
				return(l);
				}
			break;
			}
		}


		/*vdp009 add test for double constant*/ 
	else if( opty == BITYPE && 
		(l->in.op==FCON || l->in.op==DCON || l->in.op==ICON) &&
		(r->in.op==FCON || r->in.op==DCON || r->in.op==ICON) ){
			
			if (o == PLUS || o == MINUS || o == MUL || o == DIV) {

				/* vdp012 set up to handle floating 
				 * point exception 
				 */
				extern int fpe_count;
				extern jmp_buf gotfpe;
				
				fpe_count = 0;
				if (setjmp(gotfpe))
					goto treatfpe;
					
				if( l->in.op == ICON )
					l->dpn.dval = l->tn.lval;
				else if (l->in.op == FCON ) 	/*vdp009 */ 
					l->dpn.dval = l->fpn.fval; 
			
				if( r->in.op == ICON )
					r->dpn.dval = r->tn.lval;
				else if (r->in.op == FCON ) 	/*vdp009*/ 
					r->dpn.dval = r->fpn.fval; 

				switch(o){  	
				/*vdp009 conbine constants together*/ 

				case PLUS:
					l->dpn.dval += r->dpn.dval;
					break;
				case MINUS:
					l->dpn.dval -= r->dpn.dval;
					break;
				case MUL:
					l->dpn.dval *= r->dpn.dval;
					break;
				case DIV:
					if( r->dpn.dval == 0 ) 
						uerror( "division by 0." );
					else 
						l->dpn.dval /= r->dpn.dval;
					break;

					}

			treatfpe:
				if (fpe_count > 0) {
					uerror("floating point exception in constant expression");
					l->dpn.dval = 1.0; /* Fairly harmless */
					}


				fpe_count = -1;
				l->in.op = DCON;
				l->in.type = l->fn.csiz = DOUBLE;
				r->in.op = FREE;
				return (l);
			}

		}


	if (asgop(o) && o != CAST) {
	    if (ISRODATA(l->in.typattr, l->in.type)) {
	        uerror("cannot assign to a const");
	    } else {
	    	if (ISPTR(l->in.type) && ISPTR(r->in.type) &&
		    ISCONST(r->in.typattr)) {
		   uerror("cannot assign const address to non-const pointer");
		}
	   }
	}
	if (o == INITASSIGN) o = ASSIGN;

	/* its real; we must make a new node */

	p = block( o, l, r, INT, 0, INT );

	actions = opact(p);

#ifndef	BUG1
	if (adebug)				/*vdp009*/
		printact(p, actions);
#endif

	if( actions&LVAL ){ /* check left descendent */
		if( notlval(p->in.left) ) {
			uerror( "illegal lhs of assignment operator" );
			}
		}

	if( actions & NCVTR ){
		p->in.left = pconvert( p->in.left );
		}
	else if( !(actions & NCVT ) ){
		switch( opty ){

		case BITYPE:
			p->in.right = pconvert( p->in.right );
		case UTYPE:
			p->in.left = pconvert( p->in.left );

			}
		}

	if( (actions&PUN) && (o!=CAST||cflag) ){
		chkpun(p);
		}

	if( actions & (TYPL|TYPR) ){
		
		/* make type equal to the left (TYPL) or
		   make type equal to the right (TYPR) */    

		q = (actions&TYPL) ? p->in.left : p->in.right;

		p->in.type = q->in.type;
		p->fn.cdim = q->fn.cdim;
		p->fn.csiz = q->fn.csiz;
		}

	if( actions & CVTL ) p = convert( p, CVTL );
	if( actions & CVTR ) p = convert( p, CVTR );
	if( actions & TYMATCH ) p = tymatch(p);
	if( actions & PTMATCH ) p = ptmatch(p);

	if( actions & OTHER ){
		l = p->in.left;
		r = p->in.right;

		switch(o){

		case NAME:
			sp = &stab[idname];
			if( sp->stype == UNDEF ){
#ifndef FLEXNAMES
				uerror( "%.8s undefined", sp->sname );
#else
				uerror( "%s undefined", sp->sname );
#endif
				/* make p look reasonable */
				p->in.type = p->fn.cdim = p->fn.csiz = INT;
				p->in.typattr = 0;
				p->tn.rval = idname;
				p->tn.lval = 0;
				defid( p, SNULL );
				break;
				}
			p->in.type = sp->stype;
			p->in.typattr = sp->stypattr;
			p->fn.cdim = sp->dimoff;
			p->fn.csiz = sp->sizoff;
			p->tn.lval = 0;
			p->tn.rval = idname;
			/* special case: MOETY is really an ICON... */
			if( p->in.type == MOETY ){
				p->tn.rval = NONAME;
				p->tn.lval = sp->offset;
				p->fn.cdim = 0;
				p->in.type = ENUMTY;
				p->in.op = ICON;
				}
			break;

		case ICON:
			p->in.type = INT;
			p->fn.cdim = 0;
			p->fn.csiz = INT;
			break;

		case STRING:
			p->in.op = NAME;
			p->in.type = CHAR+ARY;
			p->tn.lval = 0;
			p->tn.rval = NOLAB;
			p->fn.cdim = curdim;
			p->fn.csiz = CHAR;
			break;

		/* vdp009 - distinguish between Floating constant and
		 * double constant 
		 */  
		case FCON:
			p->tn.lval = 0;
			p->tn.rval = 0;
			p->in.type = FLOAT;
			p->fn.cdim = 0;
			p->fn.csiz = FLOAT;
			break;

		case DCON:
			p->tn.lval = 0;
			p->tn.rval = 0;
			p->in.type = DOUBLE;
			p->fn.cdim = 0;
			p->fn.csiz = DOUBLE;
			break;

		case STREF:
			/* p->x turned into *(p+offset) */
			/* rhs must be a name; check correctness */

			i = r->tn.rval;
			if( i<0 || ((sp= &stab[i])->sclass != MOS && sp->sclass != MOU && !(sp->sclass&FIELD)) ){
				uerror( "member of structure or union required" );
				}else
			/* if this name is non-unique, find right one */
			if( stab[i].sflags & SNONUNIQ &&
				(l->in.type==PTR+STRTY || l->in.type == PTR+UNIONTY) &&
				(l->fn.csiz +1) >= 0 ){
				/* nonunique name && structure defined */
				char * memnam, * tabnam;
				register k;
				int j;
				int memi;
				j=dimtab[l->fn.csiz+1];
				for( ; (memi=dimtab[j]) >= 0; ++j ){
					tabnam = stab[memi].sname;
					memnam = stab[i].sname;
# ifndef BUG1
					if( ddebug>1 ){
#ifndef FLEXNAMES
						printf("member %.8s==%.8s?\n",
#else
						printf("member %s==%s?\n",
#endif
							memnam, tabnam);
						}
# endif
					if( stab[memi].sflags & SNONUNIQ ){
#ifndef FLEXNAMES
						for( k=0; k<NCHNAM; ++k ){
							if(*memnam++!=*tabnam)
								goto next;
							if(!*tabnam++) break;
							}
#else
						if (memnam != tabnam)
							goto next;
#endif
						r->tn.rval = i = memi;
						break;
						}
					next: continue;
					}
				if( memi < 0 )
#ifndef FLEXNAMES
					uerror("illegal member use: %.8s",
#else
					uerror("illegal member use: %s",
#endif
						stab[i].sname);
				}
			else {
				register j;
				if( l->in.type != PTR+STRTY && l->in.type != PTR+UNIONTY ){
					if( stab[i].sflags & SNONUNIQ ){
						uerror( "nonunique name demands struct/union or struct/union pointer" );
						}
					else werror( "struct/union or struct/union pointer required" );
					}
				else if( (j=l->fn.csiz+1)<0 ) cerror( "undefined structure or union" );
				else if( !chkstr( i, dimtab[j], DECREF(l->in.type) ) ){
#ifndef FLEXNAMES
					werror( "illegal member use: %.8s", stab[i].sname );
#else
					werror( "illegal member use: %s", stab[i].sname );
#endif
					}
				}

			p = stref( p );
			p->in.typattr = stab[r->tn.rval].stypattr;
			break;

		case UNARY MUL:
			if( l->in.op == UNARY AND ){
				p->in.op = l->in.op = FREE;
				p = l->in.left;
				}
			if( !ISPTR(l->in.type))uerror("illegal indirection");
			p->in.type = DECREF(l->in.type);
 			p->in.typattr = DECATTR(l->in.typattr);
			p->fn.cdim = l->fn.cdim;
			p->fn.csiz = l->fn.csiz;
			break;

		case UNARY AND:
			switch( l->in.op ){

			case UNARY MUL:
				p->in.op = l->in.op = FREE;
				p = l->in.left;
			case NAME:
				p->in.type = INCREF( l->in.type );
  				p->in.typattr = INCATTR(l->in.typattr);
				p->fn.cdim = l->fn.cdim;
				p->fn.csiz = l->fn.csiz;
				break;

			case COMOP:
				lr = buildtree( UNARY AND, l->in.right, NIL );
				p->in.op = l->in.op = FREE;
				p = buildtree( COMOP, l->in.left, lr );
				break;

			case QUEST:
				lr = buildtree( UNARY AND, l->in.right->in.right, NIL );
				ll = buildtree( UNARY AND, l->in.right->in.left, NIL );
				p->in.op = l->in.op = l->in.right->in.op = FREE;
				p = buildtree( QUEST, l->in.left, buildtree( COLON, ll, lr ) );
				break;

# ifdef ADDROREG
			case OREG:
				/* OREG was built in clocal()
				 * for an auto or formal parameter
				 * now its address is being taken
				 * local code must unwind it
				 * back to PLUS/MINUS REG ICON
				 * according to local conventions
				 */
				{
				extern NODE * addroreg();
				p->in.op = FREE;
				p = addroreg( l );
				}
				break;

# endif
			default:
				uerror( "unacceptable operand of &" );
				break;
				}
			break;

		case LS:
		case RS:
		case ASG LS:
		case ASG RS:
			if(tsize(p->in.right->in.type, p->in.right->fn.cdim, p->in.right->fn.csiz) > SZINT)
				p->in.right = makety(p->in.right, INT, 0, INT );
			break;

		case RETURN:
		case ASSIGN:
		case CAST:
			/* structure assignment */
			/* take the addresses of the two sides; then make an
			/* operator using STASG and
			/* the addresses of left and right */

			{
				register TWORD t;
				register d, s;

				if( l->fn.csiz != r->fn.csiz ) uerror( "assignment of different structures" );

				r = buildtree( UNARY AND, r, NIL );
				t = r->in.type;
				d = r->fn.cdim;
				s = r->fn.csiz;

				l = block( STASG, l, r, t, d, s );

				if( o == RETURN ){
					p->in.op = FREE;
					p = l;
					break;
					}

				p->in.op = UNARY MUL;
				p->in.left = l;
				p->in.right = NIL;
				break;
				}
		case COLON:
			/* structure colon */

			if( l->fn.csiz != r->fn.csiz ) uerror( "type clash in conditional" );
			break;

		case CALL:
			p->in.right = r = strargs( p->in.right );
		case UNARY CALL:
			if( !ISPTR(l->in.type)) uerror("illegal function");
			p->in.type = DECREF(l->in.type);
			if( !ISFTN(p->in.type)) uerror("illegal function");
			p->in.type = DECREF( p->in.type );
			p->fn.cdim = l->fn.cdim;
			p->fn.csiz = l->fn.csiz;
			if( l->in.op == UNARY AND && l->in.left->in.op == NAME &&
				l->in.left->tn.rval >= 0 && l->in.left->tn.rval != NONAME &&
				( (i=stab[l->in.left->tn.rval].sclass) == FORTRAN || i==UFORTRAN ) ){
				p->in.op += (FORTCALL-CALL);
				}
			if( p->in.type == STRTY || p->in.type == UNIONTY ){
				/* function returning structure */
				/*  make function really return ptr to str., with * */

				p->in.op += STCALL-CALL;
				p->in.type = INCREF( p->in.type );
				p = buildtree( UNARY MUL, p, NIL );

				}
			break;

		default:
			cerror( "other code %d", o );
			}

		}

	if( actions & CVTO ) p = oconvert(p);
	p = clocal(p);

# ifndef BUG1
	if( bdebug ) fwalk( p, eprint, 0 );
# endif

	return(p);

	}

/* vdp012 report floating point exception.  
 * If fpe_count is not less then 0 then 
 * the floating point signal occured when 
 * we were combining constants in buildtree
 * (see call to setjmp)
 */
int fpe_count = -1;
jmp_buf gotfpe;

void
fpe() {
	if (fpe_count < 0)
		cerror("floating point exception");
	++fpe_count;
	longjmp(gotfpe, 1);
	}

NODE *
strargs( p ) register NODE *p;  { /* rewrite structure flavored arguments */

	if( p->in.op == CM ){
		p->in.left = strargs( p->in.left );
		p->in.right = strargs( p->in.right );
		return( p );
		}

	/*RAP005
		Catch use of functions that don't return values used as
		arguements.
	*/
	if ((p->in.op==UNARY CALL || p->in.op==CALL ) && p->in.type==UNDEF)
		uerror("void or undef used in value required context");

	if( p->in.type == STRTY || p->in.type == UNIONTY ){
		p = block( STARG, p, NIL, p->in.type, p->fn.cdim, p->fn.csiz );
		p->in.left = buildtree( UNARY AND, p->in.left, NIL );
		p = clocal(p);
		}

       /* vdp Single	(e.g. 34.56f )  
 	* precision floating point constants are
 	* cast to double (to eliminate convert code).
 	*/

	else if( p->in.op == FCON )
		p = makety(p, DOUBLE, 0, 0);

	return( p );
	}


chkstr( i, j, type ) TWORD type; {
	/* is the MOS or MOU at stab[i] OK for strict reference by a ptr */
	/* i has been checked to contain a MOS or MOU */
	/* j is the index in dimtab of the members... */
	int k, kk;

	extern int ddebug;

# ifndef BUG1
#ifndef FLEXNAMES
	if( ddebug > 1 ) printf( "chkstr( %.8s(%d), %d )\n", stab[i].sname, i, j );
#else
	if( ddebug > 1 ) printf( "chkstr( %s(%d), %d )\n", stab[i].sname, i, j );
#endif
# endif
	if( (k = j) < 0 ) uerror( "undefined structure or union" );
	else {
		for( ; (kk = dimtab[k] ) >= 0; ++k ){
			if( kk >= SYMTSZ ){
				cerror( "gummy structure" );
				return(1);
				}
			if( kk == i ) return( 1 );
			switch( stab[kk].stype ){

			case STRTY:
			case UNIONTY:
				if( type == STRTY ) continue;  /* no recursive looking for strs */
				if( hflag && chkstr( i, dimtab[stab[kk].sizoff+1], stab[kk].stype ) ){
					if( stab[kk].sname[0] == '$' ) return(0);  /* $FAKE */
					werror(
#ifndef FLEXNAMES
					"illegal member use: perhaps %.8s.%.8s?",
#else
					"illegal member use: perhaps %s.%s?",
#endif
					stab[kk].sname, stab[i].sname );
					return(1);
					}
				}
			}
		}
	return( 0 );
	}

conval( p, o, q ) register NODE *p, *q; {
	/* apply the op o to the lval part of p; if binary, rhs is val */
	int i, u;
	CONSZ val;

	val = q->tn.lval;
	u = ISUNSIGNED(p->in.type) || ISUNSIGNED(q->in.type);
	if( u && (o==LE||o==LT||o==GE||o==GT)) o += (UGE-GE);

	if( p->tn.rval != NONAME && q->tn.rval != NONAME ) return(0);
	if( q->tn.rval != NONAME && o!=PLUS ) return(0);
	if( p->tn.rval != NONAME && o!=PLUS && o!=MINUS ) return(0);

	switch( o ){

	case PLUS:
		p->tn.lval += val;
		if( p->tn.rval == NONAME ){
			p->tn.rval = q->tn.rval;
			p->in.type = q->in.type;
			}
		break;
	case MINUS:
		p->tn.lval -= val;
		break;
	case MUL:
		p->tn.lval *= val;
		break;
	case DIV:
		if( val == 0 )
			uerror( "division by 0" );
		else if (u)
			p->tn.lval /= (unsigned)val;
		else
			p->tn.lval /= val;
		break;
	case MOD:
		if( val == 0 )
			uerror( "division by 0" );
		else if (u)
			p->tn.lval %= (unsigned)val;
		else
			p->tn.lval %= val;
		break;
	case AND:
		p->tn.lval &= val;
		break;
	case OR:
		p->tn.lval |= val;
		break;
	case ER:
		p->tn.lval ^=  val;
		break;
	case LS:
		i = val;
		if (u)
			p->tn.lval = (unsigned)p->tn.lval << i;
		else
			p->tn.lval = p->tn.lval << i;
		break;
	case RS:
		i = val;
		if (u)
			p->tn.lval = (unsigned)p->tn.lval >> i;
		else
			p->tn.lval = p->tn.lval >> i;
		break;

	case UNARY MINUS:
		p->tn.lval = - p->tn.lval;
		break;
	case COMPL:
		p->tn.lval = ~p->tn.lval;
		break;
	case NOT:
		p->tn.lval = !p->tn.lval;
		break;
	case LT:
		p->tn.lval = p->tn.lval < val;
		break;
	case LE:
		p->tn.lval = p->tn.lval <= val;
		break;
	case GT:
		p->tn.lval = p->tn.lval > val;
		break;
	case GE:
		p->tn.lval = p->tn.lval >= val;
		break;
	case ULT:
		p->tn.lval = (p->tn.lval-val)<0;
		break;
	case ULE:
		p->tn.lval = (p->tn.lval-val)<=0;
		break;
	case UGE:
		p->tn.lval = (p->tn.lval-val)>=0;
		break;
	case UGT:
		p->tn.lval = (p->tn.lval-val)>0;
		break;
	case EQ:
		p->tn.lval = p->tn.lval == val;
		break;
	case NE:
		p->tn.lval = p->tn.lval != val;
		break;
	default:
		return(0);
		}
	return(1);
	}

chkpun(p) register NODE *p; {

	/* checks p for the existance of a pun */

	/* this is called when the op of p is ASSIGN, RETURN, CAST, COLON, or relational */

	/* one case is when enumerations are used: this applies only to lint */
	/* in the other case, one operand is a pointer, the other integer type */
	/* we check that this integer is in fact a constant zero... */

	/* in the case of ASSIGN, any assignment of pointer to integer is illegal */
	/* this falls out, because the LHS is never 0 */

	register NODE *q;
	register t1, t2;
	register d1, d2;

	t1  = p->in.left->in.type;
	t2  = p->in.right->in.type;

	if( t1==ENUMTY || t2==ENUMTY ) { /* check for enumerations */
		if( logop( p->in.op ) && p->in.op != EQ && p->in.op != NE ) {
			uerror( "illegal comparison of enums" );
			return;
			}
		if( t1==ENUMTY && t2==ENUMTY && p->in.left->fn.csiz==p->in.right->fn.csiz ) return;
		werror( "enumeration type clash, operator %s", opst[p->in.op] );
		return;
		}

	if( ISPTR(t1) || ISARY(t1) ) q = p->in.right;
	else q = p->in.left;

	if( !ISPTR(q->in.type) && !ISARY(q->in.type) ){
		if( q->in.op != ICON || q->tn.lval != 0 ){
			werror( "illegal combination of pointer and integer, op %s",
				opst[p->in.op] );
			}
		}
	else {
		d1 = p->in.left->fn.cdim;
		d2 = p->in.right->fn.cdim;
		for( ;; ){
			if( t1 == t2 ) {;
				if( p->in.left->fn.csiz != p->in.right->fn.csiz ) {
					werror( "illegal structure pointer combination" );
					}
				return;
				}
			if( ISARY(t1) || ISPTR(t1) ){
				if( !ISARY(t2) && !ISPTR(t2) ) break;
				if( ISARY(t1) && ISARY(t2) && dimtab[d1] != dimtab[d2] ){
					werror( "illegal array size combination" );
					return;
					}
				if( ISARY(t1) ) ++d1;
				if( ISARY(t2) ) ++d2;
				}
			else break;
			t1 = DECREF(t1);
			t2 = DECREF(t2);
			}
		werror( "illegal pointer combination" );
		}

	}

NODE *
stref( p ) register NODE *p; {

	TWORD t;
	int d, s, dsc, align;
	OFFSZ off;
	register struct symtab *q;

	/* make p->x */
	/* this is also used to reference automatic variables */

	q = &stab[p->in.right->tn.rval];
	p->in.right->in.op = FREE;
	p->in.op = FREE;
	p = pconvert( p->in.left );

	/* make p look like ptr to x */

	if( !ISPTR(p->in.type)){
		p->in.type = PTR+UNIONTY;
		}

	t = INCREF( q->stype );
	d = q->dimoff;
	s = q->sizoff;

	p = makety( p, t, d, s );
 	p->in.typattr = INCATTR(q->stypattr);

	/* compute the offset to be added */

	off = q->offset;
	dsc = q->sclass;

	if( dsc & FIELD ) {  /* normalize offset */
		align = ALINT;
		s = INT;
		off = (off/align)*align;
		}
	if( off != 0 ) p = clocal( block( PLUS, p, offcon( off, t, d, s ), t, d, s ) );

	p = buildtree( UNARY MUL, p, NIL );

	/* if field, build field info */

	if( dsc & FIELD ){
		p = block( FLD, p, NIL, q->stype, 0, q->sizoff );
		p->tn.rval = PKFIELD( dsc&FLDSIZ, q->offset%align );
		}

	return( clocal(p) );
	}

notlval(p) register NODE *p; {

	/* return 0 if p an lvalue, 1 otherwise */

	again:

	switch( p->in.op ){

	case FLD:
		p = p->in.left;
		goto again;

	case UNARY MUL:
		/* fix the &(a=b) bug, given that a and b are structures */
		if( p->in.left->in.op == STASG ) return( 1 );
		/* and the f().a bug, given that f returns a structure */
		if( p->in.left->in.op == UNARY STCALL ||
		    p->in.left->in.op == STCALL ) return( 1 );
	case NAME:
	case OREG:
		if (ISARY(p->in.type) || ISFTN(p->in.type))
		   return(1);
	case REG:
		return(0);

	default:
		return(1);

		}

	}

NODE *
bcon( i ){ /* make a constant node with value i */
	register NODE *p;

	p = block( ICON, NIL, NIL, INT, 0, INT );
	p->tn.lval = i;
	p->tn.rval = NONAME;
	return( clocal(p) );
	}

NODE *
bpsize(p) register NODE *p; {
	return( offcon( psize(p), p->in.type, p->fn.cdim, p->fn.csiz ) );
	}

OFFSZ
psize( p ) NODE *p; {
	/* p is a node of type pointer; psize returns the
	   size of the thing pointed to */

	if( !ISPTR(p->in.type) ){
		uerror( "pointer required");
		return( SZINT );
		}
	/* note: no pointers to fields */
	return( tsize( DECREF(p->in.type), p->fn.cdim, p->fn.csiz ) );
	}

NODE *
convert( p, f )  register NODE *p; {
	/*  convert an operand of p
	    f is either CVTL or CVTR
	    operand has type int, and is converted by the size of the other side
	    */

	register NODE *q, *r;

	q = (f==CVTL)?p->in.left:p->in.right;

	r = block( PMCONV,
		q, bpsize(f==CVTL?p->in.right:p->in.left), INT, 0, INT );
	r = clocal(r);
	if( f == CVTL )
		p->in.left = r;
	else
		p->in.right = r;
	return(p);

	}

#ifndef econvert
econvert( p ) register NODE *p; {

	/* change enums to ints, or appropriate types */

	register TWORD ty;

	if( (ty=BTYPE(p->in.type)) == ENUMTY || ty == MOETY ) {
		if( dimtab[ p->fn.csiz ] == SZCHAR ) ty = CHAR;
		else if( dimtab[ p->fn.csiz ] == SZINT ) ty = INT;
		else if( dimtab[ p->fn.csiz ] == SZSHORT ) ty = SHORT;
		else ty = LONG;
		ty = ctype( ty );
		p->fn.csiz = ty;
		MODTYPE(p->in.type,ty);
		if( p->in.op == ICON && ty != LONG )
			p->in.type = p->fn.csiz = INT;
		}
	}
#endif

NODE *
pconvert( p ) register NODE *p; {

	/* if p should be changed into a pointer, do so */

	if( ISARY( p->in.type) ){
		p->in.type = DECREF( p->in.type );
		++p->fn.cdim;
		return( buildtree( UNARY AND, p, NIL ) );
		}
	if( ISFTN( p->in.type) )
		return( buildtree( UNARY AND, p, NIL ) );

	return( p );
	}

NODE *
oconvert(p) register NODE *p; {
	/* convert the result itself: used for pointer and unsigned */

	switch(p->in.op) {

	case LE:
	case LT:
	case GE:
	case GT:
		if( ISUNSIGNED(p->in.left->in.type) || ISUNSIGNED(p->in.right->in.type) )  p->in.op += (ULE-LE);
	case EQ:
	case NE:
		return( p );

	case MINUS:
		return(  clocal( block( PVCONV,
			p, bpsize(p->in.left), INT, 0, INT ) ) );
		}

	cerror( "illegal oconvert: %d", p->in.op );

	return(p);
	}

NODE *
ptmatch(p)  register NODE *p; {

	/* makes the operands of p agree; they are
	   either pointers or integers, by this time */
	/* with MINUS, the sizes must be the same */
	/* with COLON, the types must be the same */

 	TWORD ta, t1, t2, t;
	int o, d2, d, s2, s;

	o = p->in.op;
	t = t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
 	ta = p->in.left->in.typattr;
	d = p->in.left->fn.cdim;
	d2 = p->in.right->fn.cdim;
	s = p->in.left->fn.csiz;
	s2 = p->in.right->fn.csiz;

	switch( o ){

	case ASSIGN:
	case RETURN:
	case CAST:
		{  break; }

	case MINUS:
		{  if( psize(p->in.left) != psize(p->in.right) ){
			uerror( "illegal pointer subtraction");
			}
		   break;
		   }
	case COLON:
		{  if( t1 != t2 ) uerror( "illegal types in :");
		   break;
		   }
	default:  /* must work harder: relationals or comparisons */

		if( !ISPTR(t1) ){
			t = t2;
			d = d2;
			s = s2;
			break;
			}
		if( !ISPTR(t2) ){
			break;
			}

		/* both are pointers */
		if( talign(t2,s2) < talign(t,s) ){
			t = t2;
			s = s2;
			}
		break;
		}

	p->in.left = makety( p->in.left, t, d, s );
	p->in.right = makety( p->in.right, t, d, s );
	if( o!=MINUS && !logop(o) ){

		p->in.type = t;
		p->fn.cdim = d;
		p->fn.csiz = s;
		}

 	if (o == CAST) p->in.right->in.typattr = ta;
	return(clocal(p));
	}

int tdebug = 0;

NODE *
tymatch(p)  register NODE *p; {

	/* satisfy the types of various arithmetic binary ops */

	/* rules are:
		if assignment, op, type of LHS
		if any float or doubles, make double
		if any longs, make long
		otherwise, make int
		if either operand is unsigned, the result is...
	*/

	register TWORD t1, t2, t, tu;
	register o, u;
	o = p->in.op;

	t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
	if( (t1==UNDEF || t2==UNDEF) && o!=CAST )
		uerror("void type illegal in expression");

	u = 0;
	if( ISUNSIGNED(t1) ){
		u = 1;
		t1 = DEUNSIGN(t1);
		}
	if( ISUNSIGNED(t2) ){
		u = 1;
		t2 = DEUNSIGN(t2);
		}

	if( ( t1 == CHAR || t1 == SHORT ) && o!= RETURN ) t1 = INT;
	if( t2 == CHAR || t2 == SHORT ) t2 = INT;

	/* vdp009 - if floating point is not to be promoted 
	 * up to double (fflag) we check for that here 
	 */
 
	if ((fflag) && (t1==DOUBLE || t2==DOUBLE)) 
		t = DOUBLE; 
	else if ((fflag) && (t1==FLOAT || t2==FLOAT)) 
		t = FLOAT;
	else if( t1==DOUBLE || t1==FLOAT || t2==DOUBLE || t2==FLOAT )
		 t = DOUBLE;
	else if( t1==LONG || t2==LONG ) 
		t = LONG;
	else t = INT;

	if( asgop(o) ){

		/*
		 *  We use the right hand side to determine which operand
		 *  must be converted.  This fixes the problem of
		 *  (int op= real) where the real is getting converted where
		 *  it should not. The ASSIGN is unchanged by this fix.  Only
		 *  the op= is effected.
		 */

		if ( (	      (dope[o]&ASGOPFLG) && 		/* vdp014*/
		      o != CAST && o != RETURN) && 
		     ( (t == DOUBLE || t==FLOAT) ) )		/* vdp009 slr002 slr001 */
		    {
		    tu = t;					/* slr001 */
  		    }
		else						/* slr001 */
		    {	
		    tu = p->in.left->in.type;			/* slr001 */
		    }

		t = t1;					/* slr001 */
	    	}
	else {
		tu = (u && UNSIGNABLE(t))?ENUNSIGN(t):t;
		}

	/* because expressions have values that are at least as wide
	   as INT or UNSIGNED, the only conversions needed
	   are those involving FLOAT/DOUBLE, and those
	   from LONG to INT and ULONG to UNSIGNED */

	if( t != t1 ) p->in.left = makety( p->in.left, tu, 0, (int)tu );

	if( t != t2 || o==CAST ) p->in.right = makety( p->in.right, tu, 0, (int)tu );

	if( asgop(o) ){

		/*
		 *   The mode of the operation is based on the right
		 *   side  if (int op= real )
		 */

		if ( ( (dope[o]&ASGOPFLG) && o != CAST && o != RETURN) &&
		        (tu == DOUBLE || tu == FLOAT) ) 	/* vdp009 slr002 */
		    {
		    p->in.type = p->in.right->in.type;		/* slr002 */
		    p->fn.cdim = p->in.right->fn.cdim;		/* slr002 */
		    p->fn.csiz = p->in.right->fn.csiz;		/* slr002 */

		    }
		else
		    {
		    p->in.type = p->in.left->in.type;		/* slr002 */
		    p->fn.cdim = p->in.left->fn.cdim;		/* slr002 */
		    p->fn.csiz = p->in.left->fn.csiz;		/* slr002 */
		    }

		}
	else if( !logop(o) ){
		p->in.type = tu;
		p->fn.cdim = 0;
		p->fn.csiz = t;
		}

# ifndef BUG1
	if( tdebug ) printf( "tymatch(%o): %o %s %o => %o\n",p,t1,opst[o],t2,tu );
# endif

	return(p);
	}

NODE *
makety( p, t, d, s ) register NODE *p; TWORD t; {
	/* make p into type t by inserting a conversion */

	if ((p->in.type == ENUMTY) && (p->in.op == ICON)){
		/*
 		 * Go ahead and convert to an integer type (CHAR, SHORT, etc).
		 * and then see if we can do any further conversion at compile
		 * time.
		 */
		econvert(p);
	}

	if( t == p->in.type ){
		p->fn.cdim = d;
		p->fn.csiz = s;
		return( p );
		}

	if( t & TMASK ){
		/* non-simple type */
		return( block( PCONV, p, NIL, t, d, s ) );
		}
	/* vdp009 Separate Floating constant from Double */ 
	if( p->in.op == ICON ){
		if( t==DOUBLE ){
			p->in.op = DCON;
			if( ISUNSIGNED(p->in.type) ){
				p->dpn.dval = /* (unsigned CONSZ) */ p->tn.lval;
				}
			else {
				p->dpn.dval = p->tn.lval;
				}

			p->in.type = p->fn.csiz = t;
			return( clocal(p) );
		}
		if (t==FLOAT) {
			p->in.op = FCON; 
			if ( ISUNSIGNED(p->in.type) ) {
				p->fpn.fval = /* (unsigned CONSZ) */ p->tn.lval;
				}
			else {
				p->fpn.fval = p->tn.lval ;
			     }
			p->in.type = p->fn.csiz = t; 
			return (clocal(p) ); 
		}
		/* dlb015
		 * Insert a node to do a scalar conversion, and call clocal
		 * to let it perform the conversion/cast of the integer 
		 * constant at compile time if possible.
		 */
		return( clocal(block( SCONV, p, NIL, t, d, s ) ) );

	} else if (p->in.op == FCON && t == DOUBLE ) {
		double db; 
		
		p->in.op = DCON;
		db = p->fpn.fval;
		p->dpn.dval = db; 
		p->in.type = p->fn.csiz = t; 
		return (clocal(p)); 
	} else if (p->in.op == DCON && t == FLOAT ) {
		if (fflag) {					/*vdp014*/
		float fl; 

		p->in.op = FCON;
		fl = p->dpn.dval; 

		if (fl != p->dpn.dval)
			werror("float conversion loses precision");
		
		p->fpn.fval=fl;
		p->in.type = p->fn.csiz = t; 
		return (clocal(p));
		}
	}	
	/* dlb015
	 * Insert a node to indicate that a scalar conversion/cast will need
	 * to be done at some point.  However, clocal can't be called now to
	 * do the conversion, since that might lose needed information.
	 */
	return( block( SCONV, p, NIL, t, d, s ) );

	}

NODE *
block( o, l, r, t, d, s ) register NODE *l, *r; TWORD t; {

	register NODE *p;

	p = talloc();
	p->in.op = o;
	p->in.left = l;
	p->in.right = r;
	p->in.type = t;
	p->fn.cdim = d;
	p->fn.csiz = s;

	if (l != NIL) {
	    p->in.typattr = l->in.typattr;
	} else {
	    p->in.typattr = UNDEF;
	}
	return(p);
	}

icons(p) register NODE *p; {
	/* if p is an integer constant, return its value */
	int val;

	if( p->in.op != ICON ){
		uerror( "constant expected");
		val = 1;
		}
	else {
		val = p->tn.lval;
		if( val != p->tn.lval ) uerror( "constant too big for cross-compiler" );
		}
	tfree( p );
	return(val);
	}

/* 	the intent of this table is to examine the
	operators, and to check them for
	correctness.

	The table is searched for the op and the
	modified type (where this is one of the
	types INT (includes char and short), LONG,
	DOUBLE (includes FLOAT), and POINTER

	The default action is to make the node type integer

	The actions taken include:
		PUN	  check for puns
		CVTL	  convert the left operand
		CVTR	  convert the right operand
		TYPL	  the type is determined by the left operand
		TYPR	  the type is determined by the right operand
		TYMATCH	  force type of left and right to match, by inserting conversions
		PTMATCH	  like TYMATCH, but for pointers
		LVAL	  left operand must be lval
		CVTO	  convert the op
		NCVT	  do not convert the operands
		OTHER	  handled by code
		NCVTR	  convert the left operand, not the right...

	*/

# define MINT 01  /* integer */
# define MDBI 02   /* integer or double */
# define MSTR 04  /* structure */
# define MPTR 010  /* pointer */
# define MPTI 020  /* pointer or integer */
# define MENU 040 /* enumeration variable or member */
# define MVOID 0100000 /* void type */

opact( p )  NODE *p; {

	register mt12, mt1, mt2, o;

	mt1 = mt2 = mt12 = 0;

	switch( optype(o=p->in.op) ){

	case BITYPE:
		mt2 = moditype( p->in.right->in.type );
	case UTYPE:
		mt1 = moditype( p->in.left->in.type );

		}

	if( ((mt1 | mt2) & MVOID) &&
	    o != COMOP &&
	    !(o == CAST && (mt1 & MVOID)) ){
		/* if lhs of RETURN is void, grammar will complain */
		if( o != RETURN )
			uerror( "value of void expression used" );
		return( NCVT );
		}
	mt1 &= ~MVOID;
	mt2 &= ~MVOID;
	mt12 = mt1 & mt2;

	switch( o ){

	case NAME :
	case STRING :
	case ICON :
	case FCON :
	case DCON : 		/*vdp009*/ 
	case CALL :
	case UNARY CALL:
	case UNARY MUL:
		{  return( OTHER ); }
	case UNARY MINUS:
		if( mt1 & MDBI ) return( TYPL );
		break;

	case COMPL:
		if( mt1 & MINT ) return( TYPL );
		break;

	case UNARY AND:
		{  return( NCVT+OTHER ); }
	case INIT:
	case CM:
		return( 0 );
	case NOT: 
	case CBRANCH:
	case ANDAND:
	case OROR:
		/*RAP004
			Don't allow expressions involving structures or unions.
		*/
		if (o==NOT || o==CBRANCH){
			/*Catch void type RAP005 */
			if (mt1 == 0){
				uerror("void or undef used in value required context");
				return(NCVT);
				}
			if (mt1 != MSTR)
				return(0);
			}
		else {
		       /* RAP005 */
		       if (mt1==0 || mt2 ==0){
				uerror("void or undef used in value required context");
				return(NCVT);
				}
		       if ( mt1 != MSTR && mt2 != MSTR)
				return( 0 );
		     }
		uerror("illegal use of union or structure in conditional expression");
		return( NCVT);

	case MUL:
	case DIV:
		if( mt12 & MDBI ) return( TYMATCH );
		/* jpm: spr 873 */
		if ( (mt1 & MPTI) && (mt2 & MENU)) return( TYMATCH );
		if ( (mt2 & MPTI) && (mt1 & MENU)) return( TYMATCH );
		break;

	case MOD:
	case AND:
	case OR:
	case ER:
		if( mt12 & MINT ) return( TYMATCH );
		break;

	case LS:
	case RS:
		if( mt12 & MINT ) return( TYMATCH+OTHER );
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
		if( (mt1&MENU)||(mt2&MENU) ) return( PTMATCH+PUN+NCVT );
		if( mt12 & MDBI ) return( TYMATCH+CVTO );
		else if( mt12 & MPTR ) return( PTMATCH+PUN );
		else if( mt12 & MPTI ) return( PTMATCH+PUN );
		else break;

	case QUEST:
	case COMOP:
		if (o==QUEST && mt1 == MSTR){ /*RAP004*/
			uerror("illegal use of union or structure in conditional expression");
			return( NCVT);
			}
		if( mt2&MENU ) return( TYPR+NCVTR );
		return( TYPR );

	case STREF:
		return( NCVTR+OTHER );

	case FORCE:
		return( TYPL );

	case COLON:
		if( mt12 & MENU ) return( NCVT+PUN+PTMATCH );
		else if( mt12 & MDBI ) return( TYMATCH );
		else if( mt12 & MPTR ) return( TYPL+PTMATCH+PUN );
		else if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+PUN );
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+PUN );
		else if( mt12 & MSTR ) return( NCVT+TYPL+OTHER );
		break;

	case ASSIGN:
	case RETURN:
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+OTHER );
	case CAST:
		if(o==CAST && mt1==0)return(TYPL+TYMATCH);
		if( mt12 & MDBI ) return( TYPL+LVAL+TYMATCH );
		else if( (mt1&MENU)||(mt2&MENU) ) return( LVAL+NCVT+TYPL+PTMATCH+PUN );
		else if(( mt2 == 0 ) && 	/* void (*func)() ? */
			(p->in.right->in.op == CALL || 
			 p->in.right->in.op == UNARY CALL)) break;
		else if( mt1 & MPTR ) return( LVAL+PTMATCH+PUN );
		else if( mt12 & MPTI ) return( TYPL+LVAL+TYMATCH+PUN );
		break;

	case ASG LS:
	case ASG RS:
		if( mt12 & MINT ) return( TYPL+LVAL+OTHER );
		break;

	case ASG MUL:
	case ASG DIV:
		if( mt12 & MDBI ) return( LVAL+TYMATCH );
		break;

	case ASG MOD:
	case ASG AND:
	case ASG OR:
	case ASG ER:
		if( mt12 & MINT ) return( LVAL+TYMATCH );
		break;

	case ASG PLUS:
	case ASG MINUS:
	case INCR:
	case DECR:
		if( mt12 & MDBI ) return( TYMATCH+LVAL );
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+LVAL+CVTR );
		break;

	case MINUS:
		if( mt12 & MPTR ) return( CVTO+PTMATCH+PUN );
		if( mt2 & MPTR ) break;
	case PLUS:
		if( mt12 & MDBI ) return( TYMATCH );
/* jpm: Fix spr 873; MINT|MENU Allow enums to index an array per K&R */
		else if( (mt1&MPTR) && (mt2&(MINT|MENU)) ) return( TYPL+CVTR );
		else if( (mt1&(MINT|MENU)) && (mt2&MPTR) ) return( TYPR+CVTL );

		}
	uerror( "operands of %s have incompatible types", opst[o] );
	return( NCVT );
	}

moditype( ty ) TWORD ty; {

	switch(ty){

	case TVOID:
		return( MPTR );
	case UNDEF:
		return( MVOID );
	case ENUMTY:
	case MOETY:
		return( MENU );

	case STRTY:
	case UNIONTY:
		return( MSTR );

	case CHAR:
	case SHORT:
	case UCHAR:
	case USHORT:
		return( MINT|MPTI|MDBI );
	case UNSIGNED:
	case ULONG:
	case INT:
	case LONG:
		return( MINT|MDBI|MPTI );
	case FLOAT:
	case DOUBLE:
		return( MDBI );
	default:
		return( MPTR|MPTI );

		}
	}

NODE *
doszof( p )  register NODE *p; {
	/* do sizeof p */
	int i;

	/* whatever is the meaning of this if it is a bitfield? */
	i = tsize( p->in.type, p->fn.cdim, p->fn.csiz )/SZCHAR;

	tfree(p);
	if( i <= 0 ) werror( "sizeof returns 0" );
	return( bcon( i ) );
	}

# ifndef BUG2
eprint( p, down, a, b ) register NODE *p; int *a, *b; {
	register ty;

	*a = *b = down+1;
	while( down > 1 ){
		printf( "\t" );
		down -= 2;
		}
	if( down ) printf( "    " );

	ty = optype( p->in.op );

	printf("%o) %s, ", p, opst[p->in.op] );
	if( ty == LTYPE ){
		printf( CONFMT, p->tn.lval );
		printf( ", %d, ", p->tn.rval );
		}
	tprint( p->in.type );
	printf( ", %d, %d\n", p->fn.cdim, p->fn.csiz );
	}
# endif

prtdcon( p ) register NODE *p; {
	int i;
	int o = p->in.op; 
	
	/* print floating point constant, make a distingtion between 
	 * single precision and double precision floating point vdp009
	 */ 
	if( o == FCON || o ==DCON ){
		locctr( DATA );
		defalign( o == DCON ? ALDOUBLE : ALFLOAT );
		deflab( i = getlab() );
		if (o == FCON)
			fincode( p->fpn.fval, SZFLOAT ); 
		else 
			fincode( p->dpn.dval, SZDOUBLE ); 
		p->tn.lval = 0;
		p->tn.rval = -i;
		p->in.type = (o == DCON ? DOUBLE : FLOAT);
		p->in.op = NAME;
		}
	}


int edebug = 0;
ecomp( p ) register NODE *p; {
# ifndef BUG2
	if( edebug ) fwalk( p, eprint, 0 );
# endif
	if( !reached ){
		werror( "statement not reached" );
		reached = 1;
		}
	p = optim(p);
	walkf( p, prtdcon );
	locctr( PROG );
	ecode( p );
	tfree(p);
	}

# ifdef STDPRTREE
# ifndef ONEPASS

prtree(p) register NODE *p; {

	register struct symtab *q;
	register ty;

# ifdef MYPRTREE
	MYPRTREE(p);  /* local action can be taken here; then return... */
#endif

	ty = optype(p->in.op);

	printf( "%d\t", p->in.op );

	if( ty == LTYPE ) {
		printf( CONFMT, p->tn.lval );
		printf( "\t" );
		}
	if( ty != BITYPE ) {
		if( p->in.op == NAME || p->in.op == ICON ) printf( "0\t" );
		else printf( "%d\t", p->tn.rval );
		}

	printf( "%o\t", p->in.type );

	/* handle special cases */

	switch( p->in.op ){

	case NAME:
	case ICON:
		/* print external name */
		if( p->tn.rval == NONAME ) printf( "\n" );
		else if( p->tn.rval >= 0 ){
			q = &stab[p->tn.rval];
			printf(  "%s\n", exname(q->sname) );
			}
		else { /* label */
			printf( LABFMT, -p->tn.rval );
			}
		break;

	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* print out size */
		/* use lhs size, in order to avoid hassles with the structure `.' operator */

		/* note: p->in.left not a field... */
		printf( CONFMT, (CONSZ) tsize( STRTY, p->in.left->fn.cdim, p->in.left->fn.csiz ) );
		printf( "\t%d\t\n", talign( STRTY, p->in.left->fn.csiz ) );
		break;

	default:
		printf(  "\n" );
		}

	if( ty != LTYPE ) prtree( p->in.left );
	if( ty == BITYPE ) prtree( p->in.right );

	}

# else

p2tree(p) register NODE *p; {
	register ty;

# ifdef MYP2TREE
	MYP2TREE(p);  /* local action can be taken here; then return... */
# endif

	ty = optype(p->in.op);

	switch( p->in.op ){

	case NAME:
	case ICON:
#ifndef FLEXNAMES
		if( p->tn.rval == NONAME ) p->in.name[0] = '\0';
#else
		if( p->tn.rval == NONAME ) p->in.name = "";
#endif
		else if( p->tn.rval >= 0 ){ /* copy name from exname */
			register char *cp;
			register i;
			cp = exname( stab[p->tn.rval].sname );
#ifndef FLEXNAMES
			for( i=0; i<NCHNAM; ++i ) p->in.name[i] = *cp++;
#else
			p->in.name = tstr(cp);
#endif
			}
#ifndef FLEXNAMES
		else sprintf( p->in.name, LABFMT, -p->tn.rval );
#else
		else {
			char temp[32];
			sprintf( temp, LABFMT, -p->tn.rval );
			p->in.name = tstr(temp);
		}
#endif
		break;

	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* set up size parameters */
		p->stn.stsize = (tsize(STRTY,p->in.left->fn.cdim,p->in.left->fn.csiz)+SZCHAR-1)/SZCHAR;
		p->stn.stalign = talign(STRTY,p->in.left->fn.csiz)/SZCHAR;
		break;

	case REG:
		rbusy( p->tn.rval, p->in.type );
	default:
#ifndef FLEXNAMES
		p->in.name[0] = '\0';
#else
		p->in.name = "";
#endif
		}

	p->in.rall = NOPREF;

	if( ty != LTYPE ) p2tree( p->in.left );
	if( ty == BITYPE ) p2tree( p->in.right );
	}

# endif
# endif
