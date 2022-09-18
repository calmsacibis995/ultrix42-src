#ifndef lint
static	char	*sccsid = "@(#)expr.c	4.1	ULTRIX	7/2/90";
#endif lint

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
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"

MSG		BADSYM;
MSG		BADVAR;
MSG		BADKET;
MSG		BADSYN;
MSG		NOCFN;
MSG		NOADR;
MSG		BADLOC;

ADDR		kdb_lastframe;
ADDR		kdb_callpc;

CHAR		*kdb_lp;
extern		radix;
STRING		kdb_errflg;
L_INT		kdb_localval;
static CHAR	isymbol[1024];

CHAR		kdb_lastc,kdb_peekc;

L_INT		kdb_dot;
L_INT		kdb_ditto;
INT		kdb_dotinc;
L_INT		kdb_var[];
L_INT		kdb_expv;


expr(a)
{	/* term | term dyadic expr |  */
	INT		rc;
	L_INT		lhs;

	rdc(); 
	kdb_lp--; 
	rc=term(a);

	while( rc ){
		lhs = kdb_expv;

		switch ((int)readchar()) {

		case '+':
			term(a|1); 
			kdb_expv += lhs; 
			break;

		case '-':
			term(a|1); 
			kdb_expv = lhs - kdb_expv; 
			break;

		case '#':
			term(a|1); 
			kdb_expv = round(lhs,kdb_expv); 
			break;

		case '*':
			term(a|1); 
			kdb_expv *= lhs; 
			break;

		case '%':
			term(a|1); 
			kdb_expv = lhs/kdb_expv; 
			break;

		case '&':
			term(a|1); 
			kdb_expv &= lhs; 
			break;

		case '|':
			term(a|1); 
			kdb_expv |= lhs; 
			break;

		case ')':
			if( (a&2)==0 ){ 
				error(BADKET); 
				return (0);
			}

		default:
			kdb_lp--;
			return(rc);
		}
	}
	return(rc);
}

term(a)
{	/* item | monadic item | (expr) | */

	switch ((int)readchar()) {

	case '*':
		term(a|1); 
		kdb_expv=chkget(kdb_expv,DSP); 
		return(1);

	case '@':
		term(a|1); 
		kdb_expv=chkget(kdb_expv,ISP); 
		return(1);

	case '-':
		term(a|1); 
		kdb_expv = -kdb_expv; 
		return(1);

	case '~':
		term(a|1); 
		kdb_expv = ~kdb_expv; 
		return(1);

	case '#':
		term(a|1); 
		kdb_expv = !kdb_expv; 
		return(1);

	case '(':
		if( expr(2) ){
			if( *kdb_lp!=')' ){
				error(BADSYN);
			} 
			else {	
				kdb_lp++; 
				return(1);
			}
		}

	default:
		kdb_lp--;
		return(item(a));
	}
}

item(a)
{	/* name [ . local ] | number | . | ^ | <kdb_var | <register | 'x | | */
	INT		base, d;
	CHAR		savc;
	BOOL		hex;
	L_INT		frame;
	ADDR		savlastf;
	ADDR		savframe;
	ADDR		savpc;
	register struct nlist *symp;
	int regptr;

	hex=FALSE;

	readchar();
	if( symchar(0) ){
		readsym();
		if( kdb_lastc=='.' ){
			frame= uptr->u_pcb.pcb_fp; 
			kdb_lastframe=0;
			kdb_callpc= uptr->u_pcb.pcb_pc;
			while( kdb_errflg==0 ){
				savpc=kdb_callpc;
				findsym(kdb_callpc,ISYM);
				if(  eqsym(kdb_cursym->n_un.n_name,isymbol,'~') ){
					break;
				}
				kdb_callpc=get(frame+16, DSP);
				kdb_lastframe=frame;
				frame=get(frame+12,DSP)&EVEN;
				if( frame==0 ){
					error(NOCFN);
				}
			}
			savlastf=kdb_lastframe; 
			savframe=frame;
			readchar();
			if( symchar(0) ){
				chkloc(kdb_expv=frame);
			}
		} 
		else if ( (symp=lookup(isymbol))==0 ){ 
			error(BADSYM);
		} 
		else { 
			kdb_expv = symp->n_value;
		}
		kdb_lp--;

	} 
	else if ( getnum(readchar) ){
		;
	} 
	else if ( kdb_lastc=='.' ){
		readchar();
		if( symchar(0) ){
			kdb_lastframe=savlastf; 
			kdb_callpc=savpc;
			chkloc(savframe);
		} 
		else {	
			kdb_expv=kdb_dot;
		}
		kdb_lp--;

	} 
	else if ( kdb_lastc=='"' ){
		kdb_expv=kdb_ditto;

	} 
	else if ( kdb_lastc=='+' ){
		kdb_expv=inkdot(kdb_dotinc);

	} 
	else if ( kdb_lastc=='^' ){
		kdb_expv=inkdot(-kdb_dotinc);

	} 
	else if ( kdb_lastc=='<' ){
		savc=rdc();
		if( regptr=getreg(savc) ){
			kdb_expv = *(int *)regptr;
		} 
		else if ( (base=varchk(savc)) != -1 ){
			kdb_expv=kdb_var[base];
		} 
		else {	
			error(BADVAR);
		}

	} 
	else if ( kdb_lastc=='\'' ){
		d=4; 
		kdb_expv=0;
		while( quotchar() ){
			if( d-- ){
				kdb_expv = (kdb_expv << 8) | kdb_lastc;
			} 
			else { 
				error(BADSYN);
			}
		}

	} 
	else if ( a ){
		error(NOADR);
	} 
	else {	
		kdb_lp--; 
		return(0);
	}
	return(1);
}

/* service routines for expression reading */
getnum(rdf) int (*rdf)();
{
	INT base,d,frpt;
	BOOL hex;
	union{
		REAL r; 
		L_INT i;
	} 
	real;
	if( isdigit(kdb_lastc) || (hex=TRUE, kdb_lastc=='#' && isxdigit((*rdf)())) ){
		kdb_expv = 0;
		base = (hex ? 16 : radix);
		while( (base>10 ? isxdigit(kdb_lastc) : isdigit(kdb_lastc)) ){
			kdb_expv = (base==16 ? kdb_expv<<4 : kdb_expv*base);
			if( (d=convdig(kdb_lastc))>=base ){ 
				error(BADSYN); 
			}
			kdb_expv += d; 
			(*rdf)();
			if( kdb_expv==0 ){
				if( (kdb_lastc=='x' || kdb_lastc=='X') ){
					hex=TRUE; 
					base=16; 
					(*rdf)();
				} 
				else if ( (kdb_lastc=='t' || kdb_lastc=='T') ){
					hex=FALSE; 
					base=10; 
					(*rdf)();
				} 
				else if ( (kdb_lastc=='o' || kdb_lastc=='O') ){
					hex=FALSE; 
					base=8; 
					(*rdf)();
				}
			}
		}
		if( kdb_lastc=='.' && (base==10 || kdb_expv==0) && !hex ){
			real.r=kdb_expv; 
			frpt=0; 
			base=10;
			while( isdigit((*rdf)()) ){
				real.r *= base; 
				frpt++;
				real.r += kdb_lastc-'0';
			}
			while( frpt-- ){
				real.r /= base; 
			}
			kdb_expv = real.i;
		}
		kdb_peekc=kdb_lastc;
		/*		kdb_lp--; */
		return(1);
	} 
	else { 
		return(0);
	}
}

readsym()
{
	register char	*p;

	p = isymbol;
	do{ 
		if( p < &isymbol[sizeof(isymbol)-1] ){
			*p++ = kdb_lastc;
		}
		readchar();
	}
	while( symchar(1) );
	*p++ = 0;
}

convdig(c)
CHAR c;
{
	if( isdigit(c) ){
		return(c-'0');
	} 
	else if ( isxdigit(c) ){
		return(c - (isupper(c) ? 'A' : 'a') + 10);
	} 
	else {	
		return(17);
	}
}

symchar(dig)
{
	if( kdb_lastc=='\\' ){ 
		readchar(); 
		return(TRUE); 
	}
	return( isalpha(kdb_lastc) || kdb_lastc=='_' || dig && isdigit(kdb_lastc) );
}

varchk(name)
{
	if( isdigit(name) ){ 
		return(name-'0'); 
	}
	if( isalpha(name) ){ 
		return((name&037)-1+10); 
	}
	return(-1);
}

chkloc(frame)
L_INT		frame;
{
	readsym();
	do{ 
		if( localsym(frame)==0 ){ 
			error(BADLOC); 
		}
		kdb_expv=kdb_localval;
	}
	while( !eqsym(kdb_cursym->n_un.n_name,isymbol,'~') );
}

eqsym(s1, s2, c)
register char *s1, *s2;
{

	if (!strcmp(s1,s2))
		return (1);
	if (*s1 == c && !strcmp(s1+1, s2))
		return (1);
	return (0);
}
