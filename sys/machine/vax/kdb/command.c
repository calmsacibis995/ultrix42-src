#ifndef lint
static	char	*sccsid = "@(#)command.c	4.1	ULTRIX	7/2/90";
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

MSG		BADEQ;
MSG		NOMATCH;
MSG		BADVAR;
MSG		BADCOM;

MAP		kdb_txtmap;
MAP		kdb_datmap;
INT		kdb_executing;
CHAR		*kdb_lp;
INT		kdb_fcor;
INT		kdb_fsym;
STRING		kdb_errflg;

CHAR		kdb_lastc;
static CHAR	eqformat[512] = "z";
static CHAR	stformat[512] = "X\"= \"^i";

L_INT		kdb_dot;
L_INT		kdb_ditto;
INT		kdb_dotinc;
INT		kdb_lastcom = '=';
L_INT		kdb_var[];
INT		kdb_pid;
L_INT		kdb_expv;
L_INT		kdb_adrval;
INT		kdb_adrflg;
L_INT		kdb_cntval;
INT		kdb_cntflg;


/* command decoding */

command(buf,defcom)
STRING		buf;
CHAR		defcom;
{
	INT		itype, ptype, modifier;
	BOOL		longpr, eqcom;
	CHAR		wformat[1];
	CHAR		savc;
	L_INT		locval;
	L_INT		locmsk;
	L_INT		w, savdot;
	L_INT		old_prot1, old_prot2;
	STRING		savlp=kdb_lp;
	int regptr;
	if (buf != 0) cprintf("who called command with buf != 0?\n");
	/*
	 *	IF buf
	 *	THEN IF *buf==EOR
	 *	     THEN return(FALSE);
	 *	     ELSE kdb_lp=buf;
	 *	     FI
	 *	FI
	 */

	do{
		if( kdb_adrflg=expr(0) ){
			kdb_dot=kdb_expv; 
			kdb_ditto=kdb_dot;
		}
		kdb_adrval=kdb_dot;
		if( rdc()==',' && expr(0) ){
			kdb_cntflg=TRUE; 
			kdb_cntval=kdb_expv;
		} 
		else { 
			kdb_cntflg=FALSE; 
			kdb_cntval=1; 
			kdb_lp--;
		}

		if( !eol(rdc()) ){
			kdb_lastcom=kdb_lastc;
		} 
		else { 
			if( kdb_adrflg==0 ){ 
				kdb_dot=inkdot(kdb_dotinc); 
			}
			kdb_lp--; 
			kdb_lastcom=defcom;
		}

		switch(kdb_lastcom&STRIP) {

		case '/':
			itype=DSP; 
			ptype=DSYM;
			goto trystar;

		case '=':
			itype=NSP; 
			ptype=0;
			goto trypr;

		case '?':
			itype=ISP; 
			ptype=ISYM;
			goto trystar;

trystar:
			if( rdc()=='*' ){ 
				kdb_lastcom |= QUOTE; 
			} 
			else { 
				kdb_lp--; 
			}
			if( kdb_lastcom&QUOTE ){
				itype |= STAR; 
				ptype = (DSYM+ISYM)-ptype;
			}

trypr:
			longpr=FALSE; 
			eqcom=kdb_lastcom=='=';
			switch (rdc()) {

			case 'm':
				{/*reset map data*/
					INT		fcount;
					MAP		*smap;
					union{
						MAP *m; 
						L_INT *mp;
					}
					amap;

					if( eqcom ){ 
						error(BADEQ); 
						return(FALSE); 
					}
					smap=(itype&DSP?&kdb_datmap:&kdb_txtmap);
					amap.m=smap; 
					fcount=3;
					if( itype&STAR ){
						amap.mp += 3;
					}
					while( fcount-- && expr(0) ){
						*(amap.mp)++ = kdb_expv; 
					}
					if( rdc()=='?' ){ 
						smap->ufd=kdb_fsym;
					} 
					else if ( kdb_lastc == '/' ){ 
						smap->ufd=kdb_fcor;
					} 
					else { 
						kdb_lp--;
					}
				}
				break;

			case 'L':
				longpr=TRUE;
			case 'l':
				/*search for exp*/
				if( eqcom ){ 
					error(BADEQ); 
					return(FALSE); 
				}
				kdb_dotinc=(longpr?4:2); 
				savdot=kdb_dot;
				expr(1); 
				locval=kdb_expv;
				if( expr(0) ){ 
					locmsk=kdb_expv; 
				} 
				else { 
					locmsk = -1L; 
				}
				if( !longpr ){ 
					locmsk &= 0xFFFF; 
					locval &= 0xFFFF; 
				}
				for(;;){ 
					w=get(kdb_dot,itype);
					if( kdb_errflg || (w&locmsk)==locval ){ 
						break; 
					}
					kdb_dot=inkdot(kdb_dotinc);
				}
				if( kdb_errflg ){
					kdb_dot=savdot; 
					kdb_errflg=NOMATCH;
				}
				psymoff(kdb_dot,ptype,"");
				break;

			case 'W':
				longpr=TRUE;
			case 'w':
				if( eqcom ){ 
					error(BADEQ); 
					return(FALSE); 
				}
				wformat[0]=kdb_lastc; 
				expr(1);
				do{  
					savdot=kdb_dot; 
					psymoff(kdb_dot,ptype,":\t");
					exform(1,wformat,itype,ptype);
					kdb_errflg=0; 
					kdb_dot=savdot;

					old_prot1 = kdb_chgprot(kdb_dot,PG_KW);
					old_prot2 = kdb_chgprot(kdb_dot+(sizeof(int)-1),PG_KW);
					if( longpr ){
						kdbput(kdb_dot,itype,kdb_expv);
					} 
					else { 
						kdbput(kdb_dot,itype,itol(get(kdb_dot+2,itype),kdb_expv));
					}
					kdb_chgprot(kdb_dot,old_prot1);
					kdb_chgprot(kdb_dot+(sizeof(int)-1),old_prot2);

					savdot=kdb_dot;
					cprintf("=\t"); 
					exform(1,wformat,itype,ptype);
					cprintf("\n");
				}
				while(  expr(0) && kdb_errflg==0 );
				kdb_dot=savdot;
				chkerr();
				break;

			default:
				kdb_lp--;
				getformat(eqcom ? eqformat : stformat);
				if( !eqcom ){
					psymoff(kdb_dot,ptype,":\t");
				}
				scanform(kdb_cntval,(eqcom?eqformat:stformat),itype,ptype);
			}
			break;

		case '>':
			kdb_lastcom=0; 
			savc=rdc();
			if( regptr=getreg(savc) ){
				*(int *)regptr = kdb_dot;
			} 
			else if ( (modifier=varchk(savc)) != -1 ){
				kdb_var[modifier]=kdb_dot;
			} 
			else {	
				error(BADVAR); 
				return(FALSE);
			}
			break;

		case '!':
			kdb_lastcom=0;
			cprintf("no shell in kdb\n");
			break;

		case '$':
			kdb_lastcom=0;
			printtrace(nextchar()); 
			break;

		case ':':
			if( !kdb_executing ){
				kdb_executing=TRUE;
				subpcs(nextchar());
				kdb_executing=FALSE;
				kdb_lastcom=0;
			}
			break;

		case 0:
			cprintf("%s", DBNAME);
			break;

		default: 
			error(BADCOM); 
			return(FALSE);
		}

	}
	while( rdc()==';' );
	if( buf ){ 
		kdb_lp=savlp; 
	} 
	else { 
		kdb_lp--; 
	}
	return(kdb_adrflg && kdb_dot!=0);
}

