#ifndef lint
static char *sccsid ="@(#)optim.c	1.7	(ULTRIX)	9/12/86";
#endif

/************************************************************************
 *
 *			Modification History
 *
 *	David Metsky, 10-Sep-86
 * 004- Added several more cases to fix 003.  When optimizing, there
 *	were several other cases that cause the "constant expression
 *	to NOT" message to be written.  These have been placed in the
 *	renamed ISCALL routine that was added for 003 and is now called
 *	ISNOTCONST.
 *
 *	Victoria Holt, 18-Sep-85
 * 003- Fixed a bug in lint:  when hflag set, an expression of the form
 *	"!routine_name(args)" used to be flagged as "constant
 *	expression to NOT".  
 *
 *	Stephen Reilly, 27-Feb-84
 * 002 - If a division by 1 then optimize.
 *
 *	Stephen Reilly, 16-Dec-83
 * 001- This is a first pass at trying to do integer optimization when
 *	CASE are involved.
 *
 ***********************************************************************/

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

# define SWAP(p,q) {sp=p; p=q; q=sp;}
# define RCON(p) (p->in.right->in.op==ICON)
# define RO(p) p->in.right->in.op
# define RV(p) p->in.right->tn.lval
# define LCON(p) (p->in.left->in.op==ICON)
# define LO(p) p->in.left->in.op
# define LV(p) p->in.left->tn.lval

/* 004 - dnm
 * There may be more cases that cause this message to be generated. 
 */

# define ISNOTCONST(p) (p->in.left->in.op == CALL\
 		     || p->in.left->in.op == UNARY CALL\
	             || p->in.left->in.op == NAME)

int oflag = 0;

NODE *
fortarg( p ) NODE *p; {
	/* fortran function arguments */

	if( p->in.op == CM ){
		p->in.left = fortarg( p->in.left );
		p->in.right = fortarg( p->in.right );
		return(p);
		}

	while( ISPTR(p->in.type) ){
		p = buildtree( UNARY MUL, p, NIL );
		}
	return( optim(p) );
	}

	/* mapping relationals when the sides are reversed */
short revrel[] ={ EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };
NODE *
optim(p) register NODE *p; {
	/* local optimizations, most of which are probably machine independent */

	register o, ty;
	NODE *sp;
	int i;
	TWORD t;

	if( (t=BTYPE(p->in.type))==ENUMTY || t==MOETY ) econvert(p);
	if( oflag ) return(p);
	ty = optype( o=p->in.op);
	if( ty == LTYPE ) return(p);

	if( ty == BITYPE ) p->in.right = optim(p->in.right);
	p->in.left = optim(p->in.left);

	/* collect constants */

	switch(o){

	case SCONV:
	case PCONV:
		return( clocal(p) );

	case FORTCALL:
		p->in.right = fortarg( p->in.right );
		break;

	case UNARY AND:
		if( LO(p) != NAME ) cerror( "& error" );

		if( !andable(p->in.left) ) return(p);

		LO(p) = ICON;

		setuleft:
		/* paint over the type of the left hand side with the type of the top */
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case UNARY MUL:
		if( LO(p) != ICON ) break;
		LO(p) = NAME;
		goto setuleft;

	case MINUS:
		if( !nncon(p->in.right) ) break;
		RV(p) = -RV(p);
		o = p->in.op = PLUS;

	case MUL:
	case PLUS:
	case AND:
	case OR:
	case ER:
		/* commutative ops; for now, just collect constants */
		/* someday, do it right */
		if( nncon(p->in.left) || ( LCON(p) && !RCON(p) ) ) SWAP( p->in.left, p->in.right );
		/* make ops tower to the left, not the right */
		if( RO(p) == o ){
			NODE *t1, *t2, *t3;
			t1 = p->in.left;
			sp = p->in.right;
			t2 = sp->in.left;
			t3 = sp->in.right;
			/* now, put together again */
			p->in.left = sp;
			sp->in.left = t1;
			sp->in.right = t2;
			p->in.right = t3;
			}
		if(o == PLUS && LO(p) == MINUS && RCON(p) && RCON(p->in.left) &&
		  conval(p->in.right, MINUS, p->in.left->in.right)){
			zapleft:
			RO(p->in.left) = FREE;
			LO(p) = FREE;
			p->in.left = p->in.left->in.left;
		}
		if( RCON(p) && LO(p)==o && RCON(p->in.left) && conval( p->in.right, o, p->in.left->in.right ) ){
			goto zapleft;
			}
		else if( LCON(p) && RCON(p) && conval( p->in.left, o, p->in.right ) ){
			zapright:
			RO(p) = FREE;
			zapop:
			p->in.left = makety( p->in.left, p->in.type, p->fn.cdim, p->fn.csiz );
			p->in.op = FREE;
			return( clocal( p->in.left ) );
			}

		/* change muls to shifts */

		if( o==MUL && nncon(p->in.right) && (i=ispow2(RV(p)))>=0){
			if( i == 0 ){ /* multiplication by 1 */
				goto zapright;
				}
			o = p->in.op = LS;
			p->in.right->in.type = p->in.right->fn.csiz = INT;
			RV(p) = i;
			}

		/* change +'s of negative consts back to - */
		if( o==PLUS && nncon(p->in.right) && RV(p)<0 ){
			RV(p) = -RV(p);
			o = p->in.op = MINUS;
			}
		break;

	case DIV:
		if( nncon( p->in.right ) && p->in.right->tn.lval == 1 ) goto zapright;
/*		break;		slr002 continue the general cases */

	case LS:
	case RS:
	case MOD:
		/*
		 *		SLR001
		 *  If the left and right nodes are constants and we are
		 *  able to constant then we do it. If not then just 
		 *  continue.
		 */
		if( LCON(p) && RCON(p) && conval( p->in.left, o, p->in.right ) ) goto zapright;
		break;

		/*
		 *		SLR001
		 *  Try to optimize UTYPE nodes
		 */
	case NOT:
		if (hflag) {
		    if (! ISNOTCONST(p)) {
		        werror ( "constant argument to NOT" );
		    }
		}

	case UNARY MINUS:
	case COMPL:

		/*
		 *		SLR001
		 *  If left node is and integer constant try to constant
		 *  fold.  If we succeed then free up the node
		 */

		if( LCON(p) && conval( p->in.left, o, p->in.left ) )
	            goto zapop;
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
	case ULT:
	case ULE:
	case UGT:
	case UGE:
		if( !LCON(p) ) break;

		/* exchange operands */

		sp = p->in.left;
		p->in.left = p->in.right;
		p->in.right = sp;
		p->in.op = revrel[p->in.op - EQ ];
		break;

		}

	return(p);
	}

ispow2( c ) CONSZ c; {
	register i;
	if( c <= 0 || (c&(c-1)) ) return(-1);
	for( i=0; c>1; ++i) c >>= 1;
	return(i);
	}

nncon( p ) NODE *p; {
	/* is p a constant without a name */
	return( p->in.op == ICON && p->tn.rval == NONAME );
	}
