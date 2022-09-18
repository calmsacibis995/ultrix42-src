#ifndef lint
static	char	*sccsid = "@(#)format.c	4.1	ULTRIX	7/2/90";
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
 *	UNIX debugger
 */

#include "defs.h"

MSG		BADMOD;
MSG		NOFORK;
MSG		ADWRAP;

STRING		kdb_errflg;
CHAR		kdb_lastc;
L_INT		kdb_dot;
INT		kdb_dotinc;
L_INT		kdb_var[];


scanform(icount,ifp,itype,ptype)
L_INT		icount;
STRING		ifp;
{
	STRING		fp;
	CHAR		modifier;
	INT		fcount, init=1;
	L_INT		savdot;
	BOOL exact;

	while( icount ){
		fp=ifp;
		savdot=kdb_dot; 
		init=0;

		if( init==0 && (exact=(findsym(kdb_dot,ptype)==0)) && kdb_maxoff ){
			cprintf("\n%s:\t",kdb_cursym->n_un.n_name);
		}

		/*now loop over format*/
		while( *fp && kdb_errflg==0 ){
			if( digit(modifier = *fp) ){
				fcount = 0;
				while( digit(modifier = *fp++) ){
					fcount *= 10;
					fcount += modifier-'0';
				}
				fp--;
			} 
			else { 
				fcount = 1;
			}

			if( *fp==0 ){ 
				break; 
			}
			if( exact && kdb_dot==savdot && itype==ISP && kdb_cursym->n_un.n_name[0]=='_' && *fp=='i' ){
				exform(1,"x",itype,ptype); 
				fp++; 
				cprintf("\n"); /* entry mask */
			} 
			else { 
				fp=exform(fcount,fp,itype,ptype);
			}
		}
		kdb_dotinc=kdb_dot-savdot;
		kdb_dot=savdot;

		if( kdb_errflg ){
			if( icount<0 ){
				kdb_errflg=0; 
				break;
			} 
			else { 
				error(kdb_errflg);
			}
		}
		if( --icount ){
			kdb_dot=inkdot(kdb_dotinc);
		}
	}
}

extern	int	kdb_maxpos;
STRING
exform(fcount,ifp,itype,ptype)
INT		fcount;
STRING		ifp;
{
	/* execute single format item `fcount' times
		 * sets `kdb_dotinc' and moves `kdb_dot'
		 * returns address of next format item
		 */
	POS		w;
	L_INT		savdot, wx;
	STRING		fp;
	CHAR		c, modifier, longpr;
	L_REAL		fw;
	struct{
		L_INT	sa;
		INT	sb,sc;
	};
	int		print_count;

	while( fcount>0 ){
		fp = ifp; 
		c = *fp;
		longpr=(c>='A')&(c<='Z')|(c=='f')|(c=='4')|(c=='p');
		if( itype==NSP || *fp=='a' ){
			wx=kdb_dot; 
			w=kdb_dot;
		} 
		else { 
			w=get(kdb_dot,itype);
			if( longpr ){
				wx=itol(get(inkdot(2),itype),w);
			} 
			else { 
				wx=w;
			}
		}
		if( c=='F' ){
			fw.sb=get(inkdot(4),itype);
			fw.sc=get(inkdot(6),itype);
		}
		if( kdb_errflg ){ 
			return(fp); 
		}
		kdb_var[0]=wx;
		modifier = *fp++;
		kdb_dotinc=(longpr?4:2);
		;


		if( modifier!='a' ){ 
			cprintf("\t"); 
		}

		switch(modifier) {

		case SP: 
		case TB:
			break;

		case 't': 
		case 'T':
			while (fcount--) cprintf("%c", ' ');

		case 'r': 
		case 'R':
			while (fcount--)
				cprintf("%c", ' ');

		case 'a':
			psymoff(kdb_dot,ptype,":\t"); 
			kdb_dotinc=0; 
			break;

		case 'p':
			psymoff(kdb_var[0],ptype,"\t"); 
			break;

		case 'u':
			cprintf("%u",w); 
			break;

		case 'U':
			cprintf("%u",wx); 
			break;

		case 'c': 
		case 'C':
			if( modifier=='C' ){
				printesc(w&LOBYTE);
			} 
			else { 
				cprintf("%c",w&LOBYTE);
			}
			kdb_dotinc=1; 
			break;

		case 'b': 
		case 'B':
			cprintf("%o", w&LOBYTE); 
			kdb_dotinc=1; 
			break;

		case '1':
			cprintf("%r", w&LOBYTE); 
			kdb_dotinc=1; 
			break;

		case '2':
		case 'w':
			cprintf("%r", w); 
			break;

		case '4':
		case 'W':
			cprintf("%R", wx); 
			break;

		case 's': 
		case 'S':
			savdot=kdb_dot; 
			kdb_dotinc=1;
			while( (c=get(kdb_dot,itype)&LOBYTE) && kdb_errflg==0 ){
				kdb_dot=inkdot(1);
				if( modifier == 'S' ){
					printesc(c);
				} 
				else { 
					cprintf("%c", c);
				}
				if(kdb_maxpos <= print_count++) { /* GMM */
					cprintf("\n"); 
					print_count = 0;
				}
			}
			/*cprintf("\n"); GMM */
			kdb_dotinc=kdb_dot-savdot+1; 
			kdb_dot=savdot; 
			break;

		case 'x':
			cprintf("%x",w); 
			break;

		case 'X':
			cprintf("%X", wx); 
			break;

		case 'Y':
			printdate(wx); 
			break;

		case 'q':
			cprintf("%o", w); 
			break;

		case 'Q':
			cprintf("%O", wx); 
			break;

		case 'o':
			cprintf("%o", w); 
			break;

		case 'O':
			cprintf("%O", wx); 
			break;

		case 'i':
			printins(0,itype,w); 
			cprintf("\n"); 
			break;

		case 'd':
			cprintf("%d", w); 
			break;

		case 'D':
			cprintf("%D", wx); 
			break;

		case 'f':
			fw = 0;
			fw.sa = wx;

			if ((wx & ~0xFFFF00FF) == 0x8000)   /* 001 DM */
				cprintf("value is illegal floating point number\n");
			else
				cprintf("%x", fw);

			kdb_dotinc=4;
			break;

		case 'F':
			fw.sa = wx;

			if ((wx & ~0xFFFF00FF) == 0x8000)   /* 001 DM */
				cprintf("value is illegal floating point number\n");
			else
				cprintf("%X", fw);

			kdb_dotinc=8; 
			break;

		case 'n': 
		case 'N':
			cprintf("\n"); 
			kdb_dotinc=0; 
			break;

		case '"':
			kdb_dotinc=0;
			while( *fp != '"' && *fp ){
				cprintf("%c", *fp++); 
			}
			if( *fp ){ 
				fp++; 
			}
			break;

		case '^':
			kdb_dot=inkdot(-kdb_dotinc*fcount); 
			return(fp);

		case '+':
			kdb_dot=inkdot(fcount); 
			return(fp);

		case '-':
			kdb_dot=inkdot(-fcount); 
			return(fp);

		default: 
			error(BADMOD);
		}
		if( itype!=NSP ){
			kdb_dot=inkdot(kdb_dotinc);
		}
		fcount--; 
		cprintf("\n"); /* endline(); was uncomented - marc */
	}

	return(fp);
}

printesc(c)
{
	c &= STRIP;
	if( c==0177 ){ 
		cprintf("^?");
	} 
	else if ( c<SP ){
		cprintf("^%c", c + '@');
	} 
	else { 
		cprintf("%c", c);
	}
}

L_INT	inkdot(incr)
{
	L_INT		newdot;

	newdot=kdb_dot+incr;
	if( (kdb_dot ^ newdot) >> 24 ){ 
		error(ADWRAP); 
	}
	return(newdot);
}

digit(c)
{
	return c >= '0' && c <= '9';
}

printdate(tvec)
L_INT		tvec;
{
	/*
	 *	REG INT		i;
	 *	REG STRING	timeptr;
	 */
	char digits[36];
	register char *digitptr = digits;

	/*	timeptr = ctime(&tvec);
	 *	
	 *	FOR i=20; i<24; i++ DO *digitptr++ = *(timeptr+i); OD
	 *	FOR i=3; i<19; i++ DO *digitptr++ = *(timeptr+i); OD
	 *	cprintf("%s\n", digits);
	 */
	cprintf("%x\n", tvec);
} /*printdate*/

