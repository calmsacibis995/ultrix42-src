#ifndef lint
static char *sccsid = "@(#)name.c	4.2 (ULTRIX) 8/13/90";
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
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 04	David Lindner Wed Jun 13 16:03:48 EDT 1990
 *	- Removed wasintr flag. With sigvec implementation of signal
 *	  handler this is no longer needed.
 *
 * 03	David Lindner Fri Jun  9 14:44:23 EDT 1989
 *	- Added check for wasintr flag. If set drop from read loop.
 *
 * 02	Greg Tarsa, 6-Jun-86
 *	- Added code to keep from importing IFS from the environment.
 *	  This closes many security holes that result from IFS being
 *	  set to turn paths into command sequences.
 *
 * 01	Greg Tarsa, 30-Jul-85
 *	- Added code so that environment variables that don't pass
 *	  the shell's test for validity still get passed to programs.
 *	- Code added to setupenv() to skip and count bad names, code
 *	  added to setenv to add bad names to subprogram environment.
 */

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 * Original Berkeley ID: "@(#)name.c	4.2 8/11/83"
 *
 */


#include	"defs.h"

PROC BOOL	chkid();


NAMNOD	ps2nod	= {	NIL,		NIL,		ps2name},
	fngnod	= {	NIL,		NIL,		fngname},
	pathnod = {	NIL,		NIL,		pathname},
	ifsnod	= {	NIL,		NIL,		ifsname},
	ps1nod	= {	&pathnod,	&ps2nod,	ps1name},
	homenod = {	&fngnod,	&ifsnod,	homename},
	mailnod = {	&homenod,	&ps1nod,	mailname};

NAMPTR		namep = &mailnod;


/* ========	variable and string handling	======== */

syslook(w,syswds)
	STRING		w;
	SYSTAB		syswds;
{
	REG CHAR	first;
	REG STRING	s;
	REG SYSPTR	syscan;

	syscan=syswds; first = *w;

	WHILE s=syscan->sysnam
	DO  IF first == *s
		ANDF eq(w,s)
	    THEN return(syscan->sysval);
	    FI
	    syscan++;
	OD
	return(0);
}

setlist(arg,xp)
	REG ARGPTR	arg;
	INT		xp;
{
	WHILE arg
	DO REG STRING	s=mactrim(arg->argval);
	   setname(s, xp);
	   arg=arg->argnxt;
	   IF flags&execpr
	   THEN prs(s);
		IF arg THEN blank(); ELSE newline(); FI
	   FI
	OD
}

VOID	setname(argi, xp)
	STRING		argi;
	INT		xp;
{
	REG STRING	argscan=argi;
	REG NAMPTR	n;

	IF letter(*argscan)
	THEN	WHILE alphanum(*argscan) DO argscan++ OD
		IF *argscan=='='
		THEN	*argscan = 0;
			n=lookup(argi);
			*argscan++ = '=';
			attrib(n, xp);
			IF xp&N_ENVNAM
			THEN	n->namenv = n->namval = argscan;
			ELSE	assign(n, argscan);
			FI
			return;
		FI
	FI
	failed(argi,notid);
}

replace(a, v)
	REG STRING	*a;
	STRING		v;
{
	free(*a); *a=make(v);
}

dfault(n,v)
	NAMPTR		n;
	STRING		v;
{
	IF n->namval==0
	THEN	assign(n,v)
	FI
}

assign(n,v)
	NAMPTR		n;
	STRING		v;
{
	IF n->namflg&N_RDONLY
	THEN	failed(n->namid,wtfailed);
	ELSE	replace(&n->namval,v);
	FI
}

INT	readvar(names)
	STRING		*names;
{
	FILEBLK		fb;
	REG FILE	f = &fb;
	REG CHAR	c;
	REG INT		rc=0;
	NAMPTR		n=lookup(*names++); /* done now to avoid storage mess */
	STKPTR		rel=relstak();

	push(f); initf(dup(0));
	IF lseek(0,0L,1)==-1
	THEN	f->fsiz=1;
	FI

	LOOP	c=nextc(0);
		IF (*names ANDF any(c, ifsnod.namval)) ORF eolchar(c)
		THEN	zerostak();
			assign(n,absstak(rel)); setstak(rel);
			IF *names
			THEN	n=lookup(*names++);
			ELSE	n=0;
			FI
			IF eolchar(c)
			THEN	break;
			FI
		ELSE	pushstak(c);
		FI
	POOL
	WHILE n
	DO assign(n, nullstr);
	   IF *names THEN n=lookup(*names++); ELSE n=0; FI
	OD

	IF eof THEN rc=1 FI
	lseek(0, (long)(f->fnxt-f->fend), 1);
	pop();
	return(rc);
}

assnum(p, i)
	STRING		*p;
	INT		i;
{
	itos(i); replace(p,numbuf);
}

STRING	make(v)
	STRING		v;
{
	REG STRING	p;

	IF v
	THEN	movstr(v,p=alloc(length(v)));
		return(p);
	ELSE	return(0);
	FI
}


NAMPTR		lookup(nam)
	REG STRING	nam;
{
	REG NAMPTR	nscan=namep;
	REG NAMPTR	*prev;
	INT		LR;

	IF !chkid(nam)
	THEN	failed(nam,notid);
	FI
	WHILE nscan
	DO	IF (LR=cf(nam,nscan->namid))==0
		THEN	return(nscan);
		ELIF LR<0
		THEN	prev = &(nscan->namlft);
		ELSE	prev = &(nscan->namrgt);
		FI
		nscan = *prev;
	OD

	/* add name node */
	nscan=alloc(sizeof *nscan);
	nscan->namlft=nscan->namrgt=NIL;
	nscan->namid=make(nam);
	nscan->namval=0; nscan->namflg=N_DEFAULT; nscan->namenv=0;
	return(*prev = nscan);
}

LOCAL BOOL	chkid(nam)
	STRING		nam;
{
	REG CHAR *	cp=nam;

	IF !letter(*cp)
	THEN	return(FALSE);
	ELSE	WHILE *++cp
		DO IF !alphanum(*cp)
		   THEN	return(FALSE);
		   FI
		OD
	FI
	return(TRUE);
}

LOCAL VOID (*namfn)();
namscan(fn)
	VOID		(*fn)();
{
	namfn=fn;
	namwalk(namep);
}

LOCAL VOID	namwalk(np)
	REG NAMPTR	np;
{
	IF np
	THEN	namwalk(np->namlft);
		(*namfn)(np);
		namwalk(np->namrgt);
	FI
}

VOID	printnam(n)
	NAMPTR		n;
{
	REG STRING	s;

	sigchk();
	IF s=n->namval
	THEN	prs(n->namid);
		prc('='); prs(s);
		newline();
	FI
}

LOCAL STRING	staknam(n)
	REG NAMPTR	n;
{
	REG STRING	p;

	p=movstr(n->namid,staktop);
	p=movstr("=",p);
	p=movstr(n->namval,p);
	return(getstak(p+1-ADR(stakbot)));
}

VOID	exname(n)
	REG NAMPTR	n;
{
	IF n->namflg&N_EXPORT
	THEN	free(n->namenv);
		n->namenv = make(n->namval);
	ELSE	free(n->namval);
		n->namval = make(n->namenv);
	FI
}

VOID	printflg(n)
	REG NAMPTR		n;
{
	IF n->namflg&N_EXPORT
	THEN	prs(export); blank();
	FI
	IF n->namflg&N_RDONLY
	THEN	prs(readonly); blank();
	FI
	IF n->namflg&(N_EXPORT|N_RDONLY)
	THEN	prs(n->namid); newline();
	FI
}

LOCAL INT	badnamec;	/* count of bad environment names */

VOID	setupenv()
{
	REG STRING	*e=environ;
	REG CHAR	*cp;

	/*
	 GT01:
	 Import all of the environment that fits 
	 proper identifier standards.  Ignore the
	 others, they stay in the environment, but
	 aren't brought into the shell's variable list.
	*/
	FOR badnamec = 0, cp = *e;*e ; cp = *(++e)
	DO
	    /* Find the delimiting equal sign */
	    WHILE *cp && *cp != '=' DO cp++ OD

	    /*
	     Check the id out if we have an equal sign
	     This is only a sanity check environment is
	     supposed to be of the form:
	     		 <name>=<value>\0
	    */
	    IF *cp
	    THEN
		/* terminate id at equal sign */
		*cp = '\0';

		/* Check it out, importing it if OK [GT02:] and not IFS */
		IF chkid(*e) && !eq(*e, "IFS")
		THEN
		    *cp = '=';		/* restore the equal sign */
		    setname(*e, N_ENVNAM);
		ELSE
		    *cp = '=';		/* restore the equal sign */
		    badnamec++;		/* count the bad names */
		FI
	    FI
	OD
}

LOCAL INT	namec;

VOID	countnam(n)
	NAMPTR		n;
{
	namec++;
}

LOCAL STRING 	*argnam;

VOID	pushnam(n)
	NAMPTR		n;
{
	IF n->namval
	THEN	*argnam++ = staknam(n);
	FI
}

STRING	*setenv()
{
	REG STRING	*er;
	REG STRING	*e=environ;
	REG CHAR	*cp;

	namec=0;
	namscan(countnam);
	argnam = er = getstak((namec+badnamec+1)*BYTESPERWORD);
	namscan(pushnam);

	/*
	 Now add the invalid names back into the environment
	*/
	FOR cp = *e;*e ; cp = *(++e)
	DO
	    /* Find the delimiting equal sign */
	    WHILE *cp && *cp != '=' DO cp++ OD

	    /* Check the id out if we have a value */
	    IF *cp
	    THEN
		/* terminate id at equal sign */
		*cp = '\0';

		/* Check it out, exporting it if bogus */
		IF chkid(*e)
		THEN
		    *cp = '=';
		ELSE
		    *cp = '=';
		    *argnam++ = *e;
		FI
	    FI
	OD

	*argnam++ = 0;	/* terminate the list */
	return(er);
}
