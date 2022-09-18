#ifndef lint
static char sccsid[] = "@(#)expand.c	4.3 (ULTRIX) 1/21/91";
/* Original ID:  "@(#)expand.c	4.5 8/11/83" */
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 003	David Lindner Mon Jan 21 14:04:34 EST 1991
 *	- Modified argument passed to opendir command to be a "." instead
 *	  of a "". This is a berklyism that assumes "" to be the
 *	  current directory, that no longer holds.
 *
 * 002	David Lindner Tue Jun 12 14:15:46 EDT 1990
 *	- Removed setjmp that skipped by readdir upon faulting.
 *
 * 001	David Lindner Fri Nov 17 14:21:25 EST 1989
 *	- Modified argument passed to stat command to be a "." instead
 *	  of a "". This is a berklyism that assumes "" to be the
 *	  current directory, that no longer holds.
 *	- Added comment header.
 *
 */

#include	"defs.h"
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/dir.h>



/* globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */

PROC VOID	addg();


INT	expand(as,rflg)
	STRING		as;
{
	INT		count;
	DIR		*dirf;
	BOOL		dir=0;
	STRING		rescan = 0;
	REG STRING	s, cs;
	ARGPTR		schain = gchain;
	struct direct	*dp;
	STATBUF		statb;

	IF trapnote&SIGSET THEN return(0); FI

	s=cs=as;

	/* check for meta chars */
	BEGIN
	   REG BOOL slash; slash=0;
	   WHILE !fngchar(*cs)
	   DO	IF *cs++==0
		THEN	IF rflg ANDF slash THEN break; ELSE return(0) FI
		ELIF *cs=='/'
		THEN	slash++;
		FI
	   OD
	END

	LOOP	IF cs==s
		THEN	s=nullstr;
			break;
		ELIF *--cs == '/'
		THEN	*cs=0;
			IF s==cs THEN s="/" FI
			break;
		FI
	POOL
	IF stat( *s ? s:".",&statb)>=0		/* DJL 001 */
    	ANDF (statb.st_mode&S_IFMT)==S_IFDIR
    	ANDF (dirf=opendir(*s ? s:".")) != NULL /* DJL 003 */
	THEN	dir++;
	FI
	count=0;
	IF *cs==0 THEN *cs++=0200 FI
	IF dir
	THEN	/* check for rescan */
		REG STRING rs; rs=cs;

		REP	IF *rs=='/' THEN rescan=rs; *rs=0; gchain=0 FI
		PER	*rs++ DONE

		WHILE (trapnote&SIGSET) == 0 ANDF (dp = readdir(dirf)) != NULL
		DO	IF (*dp->d_name=='.' ANDF *cs!='.')
			THEN	continue;
			FI
			IF gmatch(dp->d_name, cs)
			THEN	addg(s,dp->d_name,rescan); count++;
			FI
		OD
		closedir(dirf);

		IF rescan
		THEN	REG ARGPTR	rchain;
			rchain=gchain; gchain=schain;
			IF count
			THEN	count=0;
				WHILE rchain
				DO	count += expand(rchain->argval,1);
					rchain=rchain->argnxt;
				OD
			FI
			*rescan='/';
		FI
	FI

	BEGIN
	   REG CHAR	c;
	   s=as;
	   WHILE c = *s
	   DO	*s++=(c&STRIP?c:'/') OD
	END
	return(count);
}

gmatch(s, p)
	REG STRING	s, p;
{
	REG INT		scc;
	CHAR		c;

	IF scc = *s++
	THEN	IF (scc &= STRIP)==0
		THEN	scc=0200;
		FI
	FI
	SWITCH c = *p++ IN

	    case '[':
		{BOOL ok; INT lc;
		ok=0; lc=077777;
		WHILE c = *p++
		DO	IF c==']'
			THEN	return(ok?gmatch(s,p):0);
			ELIF c==MINUS
			THEN	IF lc<=scc ANDF scc<=(*p++) THEN ok++ FI
			ELSE	IF scc==(lc=(c&STRIP)) THEN ok++ FI
			FI
		OD
		return(0);
		}

	    default:
		IF (c&STRIP)!=scc THEN return(0) FI

	    case '?':
		return(scc?gmatch(s,p):0);

	    case '*':
		IF *p==0 THEN return(1) FI
		--s;
		WHILE *s
		DO  IF gmatch(s++,p) THEN return(1) FI OD
		return(0);

	    case 0:
		return(scc==0);
	ENDSW
}

LOCAL VOID	addg(as1,as2,as3)
	STRING		as1, as2, as3;
{
	REG STRING	s1, s2;
	REG INT		c;

	s2 = locstak()+BYTESPERWORD;

	s1=as1;
	WHILE c = *s1++
	DO	IF (c &= STRIP)==0
		THEN	*s2++='/';
			break;
		FI
		*s2++=c;
	OD
	s1=as2;
	WHILE *s2 = *s1++ DO s2++ OD
	IF s1=as3
	THEN	*s2++='/';
		WHILE *s2++ = *++s1 DONE
	FI
	makearg(endstak(s2));
}

makearg(args)
	REG STRING	args;
{
	args->argnxt=gchain;
	gchain=args;
}

