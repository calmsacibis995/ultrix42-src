#ifndef lint
static	char	*sccsid = "@(#)input.c	4.1	ULTRIX	7/2/90";
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

CHAR		line[LINSIZ];
INT		infile;
CHAR		*kdb_lp;
CHAR		kdb_peekc,kdb_lastc = EOR;
INT		kdb_eof;

/* input routines */

eol(c)
CHAR	c;
{
	return(c==EOR || c==';');
}

rdc()
{	
	do{	
		readchar();
	} while( kdb_lastc==SP || kdb_lastc==TB );
	return(kdb_lastc);
}

readchar()
{
	extern kdb_input_debug;

	if( kdb_eof ){
		kdb_lastc=0;
	} 
	else {	
		if( kdb_lp==0 ){
			kdb_lp=line;
			do{ 
				kdb_eof = kdb_read(infile,kdb_lp,1)==0;
			} while( kdb_eof==0 && *kdb_lp++!=EOR );
			*kdb_lp=0; 
			kdb_lp=line;
		}
		if( kdb_lastc = kdb_peekc ){ 
			kdb_peekc=0;
		} 
		else if ( kdb_lastc = *kdb_lp ){ 
			kdb_lp++;
		}
	}
	if (kdb_input_debug)
		cprintf(">%o<", kdb_lastc);
	return(kdb_lastc);
}

nextchar()
{
	if( eol(rdc()) ){
		kdb_lp--; 
		return(0);
	} 
	else { 
		return(kdb_lastc);
	}
}

quotchar()
{
	if( readchar()=='\\' ){
		return(readchar());
	} 
	else if ( kdb_lastc=='\'' ){
		return(0);
	} 
	else {	
		return(kdb_lastc);
	}
}

getformat(deformat)
STRING		deformat;
{
	register STRING	fptr;
	register BOOL	quote;
	fptr=deformat; 
	quote=FALSE;
	while( (quote ? readchar()!=EOR : !eol(readchar())) ){
		if( (*fptr++ = kdb_lastc)=='"' ){
			quote = ~quote;
		}
	}
	kdb_lp--;
	if( fptr!=deformat ){ 
		*fptr++ = '\0'; 
	}
}
