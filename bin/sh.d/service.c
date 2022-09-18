#ifndef lint
static char *sccsid = "@(#)service.c	4.3 (ULTRIX) 11/9/90";
/* Original ID:  "@(#)service.c 4.3 9/9/83" */
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 * 04   David Lindner Wed Jun  6 11:46:06 EDT 1990
 *	- Added wasintr to see if wait interupted.
 *	- Added bckg flag to await to note whether the process was running
 *	  in the background or not.
 *	- Removed setjmps and longjmps to signal handling routines.
 *
 * 03	David Lindner Tue Oct 24 10:18:06 EDT 1989
 *	- Commented out call to newline, that was printing out newlines
 *	  for no reason.
 *
 * 02	David Lindner Mon Jul 31 17:53:57 EDT 1989
 *	- Fixed command variable substitution bug.
 *
 * 01	David Lindner Wed Jul 19 11:01:28 EDT 1989
 *	- Added test for boolflg, to insure sh doesn't exit on
 *	- boolean tests when -e flag is set. Also added comment
 *	- header.
 *
 */

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


PROC VOID	gsort();

#define ARGMK	01

INT		errno;
STRING		sysmsg[];
INT		num_sysmsg;
extern int	wasintr;

/* fault handling */
#define ENOMEM	12
#define ENOEXEC 8
#define E2BIG	7
#define ENOENT	2
#define ETXTBSY 26



/* service routines for `execute' */

VOID	initio(iop)
	IOPTR		iop;
{
	REG STRING	ion;
	REG INT		iof, fd;

	IF iop
	THEN	iof=iop->iofile;
		ion=mactrim(iop->ioname);
		IF *ion ANDF (flags&noexec)==0
		THEN	IF iof&IODOC
			THEN	subst(chkopen(ion),(fd=tmpfil()));
				close(fd); fd=chkopen(tmpout); unlink(tmpout);
			ELIF iof&IOMOV
			THEN	IF eq(minus,ion)
				THEN	fd = -1;
					close(iof&IOUFD);
				ELIF (fd=stoi(ion))>=USERIO
				THEN	failed(ion,badfile);
				ELSE	fd=dup(fd);
				FI
			ELIF (iof&IOPUT)==0
			THEN	fd=chkopen(ion);
			ELIF flags&rshflg
			THEN	failed(ion,restricted);
			ELIF iof&IOAPP ANDF (fd=open(ion,1))>=0
			THEN	lseek(fd, 0L, 2);
			ELSE	fd=create(ion);
			FI
			IF fd>=0
			THEN	rename(fd,iof&IOUFD);
			FI
		FI
		initio(iop->ionxt);
	FI
}

STRING	getpath(s)
	STRING		s;
{
	REG STRING	path;
	IF any('/',s)
	THEN	IF flags&rshflg
		THEN	failed(s, restricted);
		ELSE	return(nullstr);
		FI
	ELIF (path = pathnod.namval)==0
	THEN	return(defpath);
	ELSE	return(cpystak(path));
	FI
}

INT	pathopen(path, name)
	REG STRING	path, name;
{
	REG UFD		f;

	REP path=catpath(path,name);
	PER (f=open(curstak(),0))<0 ANDF path DONE
	return(f);
}

STRING	catpath(path,name)
	REG STRING	path;
	STRING		name;
{
	/* leaves result on top of stack */
	REG STRING	scanp = path,
			argp = locstak();

	WHILE *scanp ANDF *scanp!=COLON DO *argp++ = *scanp++ OD
	IF scanp!=path THEN *argp++='/' FI
	IF *scanp==COLON THEN scanp++ FI
	path=(*scanp ? scanp : 0); scanp=name;
	WHILE (*argp++ = *scanp++) DONE
	return(path);
}

LOCAL STRING	xecmsg;
LOCAL STRING	*xecenv;
LOCAL int	eflag;		/* DJL 02 */

VOID	execa(at)
	STRING		at[];
{
	REG STRING	path;
	REG STRING	*t = at;

	IF (flags&noexec)==0
	THEN	xecmsg=notfound; path=getpath(*t);
		namscan(exname);
		xecenv=setenv();
		eflag = 0;		/* DJL 02 */
		if (*t[0] == ('/'|~STRIP)) {
			eflag++;
			execs(path, t);
			eflag = 0;
			}
		WHILE path=execs(path,t) DONE
		failed(*t,xecmsg);
	FI
}

LOCAL STRING	execs(ap,t)
	STRING		ap;
	REG STRING	t[];
{
	REG STRING	p, prefix;

	if (eflag) {			/* DJL 02 */
		trim(t[0]);
		sigchk();
		execve(t[0], &t[0], xecenv);
		}
	else {
		prefix=catpath(ap,t[0]);
		trim(p=curstak());
		sigchk();
		execve(p, &t[0], xecenv);
		}

	SWITCH errno IN

	    case ENOEXEC:
		flags=0;
		comdiv=0; ioset=0;
		clearup(); /* remove open files and for loop junk */
		IF input THEN close(input) FI
		close(output); output=2;
		input=chkopen(p);

		/* band aid to get csh... 2/26/79 */
		{
			char c;
			if (!isatty(input)) {
				read(input, &c, 1);
				if (c == '#')
					gocsh(t, p, xecenv);
				lseek(input, (long) 0, 0);
			}
		}

		/* set up new args */
		setargs(t);
		longjmp(subshell,1);

	    case ENOMEM:
		failed(p,toobig);

	    case E2BIG:
		failed(p,arglist);

	    case ETXTBSY:
		failed(p,txtbsy);

	    default:
		xecmsg=badexec;
	    case ENOENT:
		return(prefix);
	ENDSW
}

gocsh(t, cp, xecenv)
	register char **t, *cp, **xecenv;
{
	char **newt[1000];
	register char **p;
	register int i;

	for (i = 0; t[i]; i++)
		newt[i+1] = t[i];
	newt[i+1] = 0;
	newt[0] = "/bin/csh";
	newt[1] = cp;
	execve("/bin/csh", newt, xecenv);
}

/* for processes to be waited for */
#define MAXP 20
LOCAL INT	pwlist[MAXP];
LOCAL INT	pwc;

postclr()
{
	REG INT		*pw = pwlist;

	WHILE pw <= &pwlist[pwc]
	DO *pw++ = 0 OD
	pwc=0;
}

VOID	post(pcsid)
	INT		pcsid;
{
	REG INT		*pw = pwlist;

	IF pcsid
	THEN	WHILE *pw DO pw++ OD
		IF pwc >= MAXP-1
		THEN	pw--;
		ELSE	pwc++;
		FI
		*pw = pcsid;
	FI
}

VOID	await(i, bckg)
	INT		i, bckg;
{
	INT		rc=0, wx=0;
	INT		w;
	INT		ipwc = pwc;

	post(i);
	WHILE pwc
	DO	REG INT		p;
		REG INT		sig;
		INT		w_hi;

		BEGIN
		   REG INT	*pw=pwlist;

 		   p=wait(&w);

		   if (wasintr) {
			wasintr = 0;
			if (bckg)
				break;
		   }

		   WHILE pw <= &pwlist[ipwc]
		   DO IF *pw==p
		      THEN *pw=0; pwc--;
		      ELSE pw++;
		      FI
		   OD
		END

		IF p == -1 THEN continue FI

		w_hi = (w>>8)&LOBYTE;

		IF sig = w&0177
		THEN	IF sig == 0177	/* ptrace! return */
			THEN	prs("ptrace: ");
				sig = w_hi;
			FI
			IF sig < num_sysmsg ANDF sysmsg[sig]
			THEN	IF i!=p ORF (flags&prompt)==0
				THEN prp(); prn(p); blank()
				FI
				prs(sysmsg[sig]);
				IF w&0200 THEN prs(coredump) FI
			FI
			/* DJL 003
			newline();
			*/
		FI

		IF rc==0
		THEN	rc = (sig ? sig|SIGFLG : w_hi);
		FI
		wx |= w;
	OD

	if ((wx) && (flags&errflg))	/* DJL 01 */
		if (!(flags&boolflg))
			exitsh(rc);
	exitval=rc; exitset();
}

BOOL		nosubst;

trim(at)
	STRING		at;
{
	REG STRING	p;
	REG CHAR	c;
	REG CHAR	q=0;

	IF p=at
	THEN	WHILE c = *p
		DO *p++=c&STRIP; q |= c OD
	FI
	nosubst=q&QUOTE;
}

STRING	mactrim(s)
	STRING		s;
{
	REG STRING	t=macro(s);
	trim(t);
	return(t);
}

STRING	*scan(argn)
	INT		argn;
{
	REG ARGPTR	argp = Rcheat(gchain)&~ARGMK;
	REG STRING	*comargn, *comargm;

	comargn=getstak(BYTESPERWORD*argn+BYTESPERWORD); comargm = comargn += argn; *comargn = ENDARGS;

	WHILE argp
	DO	*--comargn = argp->argval;
		IF argp = argp->argnxt
		THEN trim(*comargn);
		FI
		IF argp==0 ORF Rcheat(argp)&ARGMK
		THEN	gsort(comargn,comargm);
			comargm = comargn;
		FI
		/* Lcheat(argp) &= ~ARGMK; */
		argp = Rcheat(argp)&~ARGMK;
	OD
	return(comargn);
}

LOCAL VOID	gsort(from,to)
	STRING		from[], to[];
{
	INT		k, m, n;
	REG INT		i, j;

	IF (n=to-from)<=1 THEN return FI

	FOR j=1; j<=n; j*=2 DONE

	FOR m=2*j-1; m/=2;
	DO  k=n-m;
	    FOR j=0; j<k; j++
	    DO	FOR i=j; i>=0; i-=m
		DO  REG STRING *fromi; fromi = &from[i];
		    IF cf(fromi[m],fromi[0])>0
		    THEN break;
		    ELSE STRING s; s=fromi[m]; fromi[m]=fromi[0]; fromi[0]=s;
		    FI
		OD
	    OD
	OD
}

/* Argument list generation */

INT	getarg(ac)
	COMPTR		ac;
{
	REG ARGPTR	argp;
	REG INT		count=0;
	REG COMPTR	c;

	IF c=ac
	THEN	argp=c->comarg;
		WHILE argp
		DO	count += split(macro(argp->argval));
			argp=argp->argnxt;
		OD
	FI
	return(count);
}

LOCAL INT	split(s)
	REG STRING	s;
{
	REG STRING	argp;
	REG INT		c;
	INT		count=0;

	LOOP	sigchk(); argp=locstak()+BYTESPERWORD;
		WHILE (c = *s++, !any(c,ifsnod.namval) && c)
		DO *argp++ = c OD
		IF argp==staktop+BYTESPERWORD
		THEN	IF c
			THEN	continue;
			ELSE	return(count);
			FI
		ELIF c==0
		THEN	s--;
		FI
		IF c=expand((argp=endstak(argp))->argval,0)
		THEN	count += c;
		ELSE	/* assign(&fngnod, argp->argval); */
			makearg(argp); count++;
		FI
		Lcheat(gchain) |= ARGMK;
	POOL
}
