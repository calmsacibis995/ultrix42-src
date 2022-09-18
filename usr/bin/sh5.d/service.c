#ifndef lint
static CHTYPE *sccsid = "@(#)service.c	4.1      7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   Modification History:
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 	MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	i18n version of csh
 *
 *
 *
 */

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<errno.h>

#define ARGMK	01

extern int	gsort();
extern CHTYPE	*sysmsg[];
extern short topfd;



/*
 * service routines for `execute'
 */
initio(iop, save)
	struct ionod	*iop;
	int		save;
{
	extern long	lseek();	/* DAG -- bug fix (was missing) */
	register CHTYPE	*ion;
	register int	iof, fd;
	int		ioufd;
	short	lastfd;

	lastfd = topfd;
	while (iop)
	{
		iof = iop->iofile;
		ion = mactrim(iop->ioname);
		ioufd = iof & IOUFD;

		if (*ion && (flags&noexec) == 0)
		{
			if (save)
			{
				fdmap[topfd].org_fd = ioufd;
				fdmap[topfd++].dup_fd = savefd(ioufd);
			}

			if (iof & IODOC)
			{
				struct tempblk tb;

				subst(chkopen(ion), (fd = tmpfil(&tb)));

				poptemp();	/* pushed in tmpfil() --
						   bug fix for problem with
						   in-line scripts
						*/

				fd = chkopen(tmpout);
				unlink(wtoc(tmpout));
			}
			else if (iof & IOMOV)
			{
				if (eq(minus, ion))
				{
					fd = -1;
					close(ioufd);
				}
				else if ((fd = stoi(ion)) >= USERIO)
					failed(ion, badfile);
				else
					fd = dup(fd);
			}
			else if (iof & IORDW)	/* DAG -- added (due to Brian Horn) */
			{
				if ((fd = open(ion, 2)) < 0)
					failed(ion, badopen);
					/*NOTREACHED*/
			}
			else if ((iof & IOPUT) == 0)
				fd = chkopen(ion);
			else if (flags & rshflg)
				failed(ion, restricted);
			else if (iof & IOAPP && (fd = open(wtoc(ion), 1)) >= 0)
				lseek(fd, 0L, 2);
			else
				fd = create(ion);
			if (fd >= 0)
				rename(fd, ioufd);
		}

		iop = iop->ionxt;
	}
	return(lastfd);
}

CHTYPE *
simple(s)
CHTYPE	*s;
{
	CHTYPE	*sname;

	sname = s;
	while (1)
	{
		if (any('/', sname))
			while (*sname++ != '/')
				;
		else
			return(sname);
	}
}

CHTYPE *
getpath(s)
	CHTYPE	*s;
{
	register CHTYPE	*path;

	if (any('/', s) || any(('/' | QUOTE), s))
	{
		if (flags & rshflg)
			failed(s, restricted);
			/*NOTREACHED*/		/* DAG -- keep lint happy */
		else
			return(nullstr);
	}
	else if ((path = pathnod.namval) == 0)
		return(defpath);
	else
		return(cpystak(path));
}

pathopen(path, name)
register CHTYPE *path, *name;
{
	register int	f;

	do
	{
		path = catpath(path, name);
	} while ((f = open(wtoc(curstak()), 0)) < 0 && path);
	return(f);
}

CHTYPE *
catpath(path, name)
register CHTYPE	*path;
CHTYPE	*name;
{
	/*
	 * leaves result on top of stack
	 */
	register CHTYPE	*scanp = path;
	register CHTYPE	*argp = locstak();

	while (*scanp && *scanp != COLON)
		*argp++ = *scanp++;
	if (scanp != path)
		*argp++ = '/';
	if (*scanp == COLON)
		scanp++;
	path = (*scanp ? scanp : 0);
	scanp = name;
	while ((*argp++ = *scanp++))
		;
	return(path);
}

CHTYPE *
nextpath(path)
	register CHTYPE	*path;
{
	register CHTYPE	*scanp = path;

	while (*scanp && *scanp != COLON)
		scanp++;

	if (*scanp == COLON)
		scanp++;

	return(*scanp ? scanp : 0);
}

static CHTYPE	*xecmsg;
static CHTYPE	**xecenv;

int	execa(at, pos)
	CHTYPE	*at[];
	short pos;
{
	register CHTYPE	*path;
	register CHTYPE	**t = at;
	int		cnt;

	if ((flags & noexec) == 0)
	{
		xecmsg = notfound;
		path = getpath(*t);
		xecenv = setenv();

		if (pos > 0)
		{
			cnt = 1;
			while (cnt != pos)
			{
				++cnt;
				path = nextpath(path);
			}
			execs(path, t);
			path = getpath(*t);
		}
		while (path = execs(path,t))
			;
		failed(*t, xecmsg);
	}
}

static CHTYPE *
execs(ap, t)
CHTYPE	*ap;
register CHTYPE	*t[];
{
	register CHTYPE *p, *prefix, *ptr;
	char **newep, **newap;
	char **convargs();

	prefix = catpath(ap, t[0]);
	trim(p = curstak());
	sigchk();
	
	/*
	 * convert argument and environment lists from 
	 * CHTYPE * to char * for exec. Unfortunatly catpath uses locstak
	 * but does not set staktop so we need to find the top here
	 */
	ptr = p;
	while (*ptr++)
		;
 	newap = (char **) (Rcheat(ptr) + 3 & ~3); /* GAG - byte align ptr */
	newep = convargs(t, newap);
	convargs(xecenv, newep);
	
	execve(wtoc(p), newap, newep);

	switch (errno)
	{
	case ENOEXEC:		/* could be a shell script */
		funcnt = 0;
		flags = 0;
		*flagadr = 0;
		comdiv = 0;
		ioset = 0;
		clearup();	/* remove open files and for loop junk */
		if (input)
			close(input);
		input = chkopen(p);
	
#ifdef ACCT
		preacct(p);	/* reset accounting */
#endif

		/*
		 * set up new args
		 */
		
		setargs(t);
		longjmp(subshell, 1);

	case ENOMEM:
		failed(p, toobig);

	case E2BIG:
		failed(p, arglist);

	case ETXTBSY:
		failed(p, txtbsy);

	default:
		xecmsg = badexec;
	case ENOENT:
		return(prefix);
	}
}


/*
 * for processes to be waited for
 */
#define MAXP 20
static int	pwlist[MAXP];
static int	pwc;

postclr()
{
	register int	*pw = pwlist;

	while (pw <= &pwlist[pwc])
		*pw++ = 0;
	pwc = 0;
}

post(pcsid)
int	pcsid;
{
	register int	*pw = pwlist;

	if (pcsid)
	{
		while (*pw)
			pw++;
		if (pwc >= MAXP - 1)
			pw--;
		else
			pwc++;
		*pw = pcsid;
	}
}

await(i, bckg)
int	i, bckg;
{
	int	rc = 0, wx = 0;
	int	w;
	int	ipwc = pwc;

	post(i);
	while (pwc)
	{
		register int	p;
		register int	sig;
		int		w_hi;
		int	found = 0;

		{
			register int	*pw = pwlist;

			p = wait(&w);
			if (wasintr)
			{
				wasintr = 0;
				if (bckg)
				{
					break;
				}
			}
			while (pw <= &pwlist[ipwc])
			{
				if (*pw == p)
				{
					*pw = 0;
					pwc--;
					found++;
				}
				else
					pw++;
			}
		}
		if (p == -1)
		{
			if (bckg)
			{
				register int *pw = pwlist;

				while (pw <= &pwlist[ipwc] && i != *pw)
					pw++;
				if (i == *pw)
				{
					*pw = 0;
					pwc--;
				}
			}
			continue;
		}
		w_hi = (w >> 8) & LOBYTE;
		if (sig = w & 0177)
		{
			if (sig == 0177)	/* ptrace! return */
			{
				prs("ptrace: ");
				sig = w_hi;
			}
			if (sysmsg[sig])
			{
				if (i != p || (flags & prompt) == 0)
				{
					prp();
					prn(p);
					blank();
				}
				prs(sysmsg[sig]);
				if (w & 0200)
					prs(coredump);
			}
			newline();
		}
		if (rc == 0 && found != 0)
			rc = (sig ? sig | SIGFLG : w_hi);
		wx |= w;
		if (p == i)
		{
			break;
		}
	}
	if (wx && flags & errflg)
		exitsh(rc);
	flags |= eflag;
	exitval = rc;
	exitset();
}

BOOL		nosubst;

trim(at)
CHTYPE	*at;
{
	register CHTYPE	*p;
	register CHTYPE 	*ptr;
	register CHTYPE	c;
	register CHTYPE	q = 0;

	if (p = at)
	{
		ptr = p;
		while (c = *p++)
		{
			if (*ptr = c & STRIP)
				++ptr;
			q |= c;
		}

		*ptr = 0;
	}
	nosubst = q & QUOTE;
}

CHTYPE *
mactrim(s)
CHTYPE	*s;
{
	register CHTYPE	*t = macro(s);

	trim(t);
	return(t);
}

CHTYPE **
scan(argn)
int	argn;
{
	register struct argnod *argp = (struct argnod *)(Rcheat(gchain) & ~ARGMK);
	register CHTYPE **comargn, **comargm;

	comargn = (CHTYPE **)getstak(BYTESPERWORD * argn + BYTESPERWORD);
	comargm = comargn += argn;
	*comargn = ENDARGS;
	while (argp)
	{
		*--comargn = argp->argval;

		trim(*comargn);
		argp = argp->argnxt;

		if (argp == 0 || Rcheat(argp) & ARGMK)
		{
			gsort(comargn, comargm);
			comargm = comargn;
		}
		/* Lcheat(argp) &= ~ARGMK; */
		argp = (struct argnod *)(Rcheat(argp) & ~ARGMK);
	}
	return(comargn);
}

static int
gsort(from, to)
CHTYPE	*from[], *to[];
{
	int	k, m, n;
	register int	i, j;

	if ((n = to - from) <= 1)
		return;
	for (j = 1; j <= n; j *= 2)
		;
	for (m = 2 * j - 1; m /= 2; )
	{
		k = n - m;
		for (j = 0; j < k; j++)
		{
			for (i = j; i >= 0; i -= m)
			{
				register CHTYPE **fromi;

				fromi = &from[i];
				if (cf(fromi[m], fromi[0]) > 0)
				{
					break;
				}
				else
				{
					CHTYPE *s;

					s = fromi[m];
					fromi[m] = fromi[0];
					fromi[0] = s;
				}
			}
		}
	}
}

/*
 * Argument list generation
 */
getarg(ac)
struct comnod	*ac;
{
	register struct argnod	*argp;
	register int		count = 0;
	register struct comnod	*c;

	if (c = ac)
	{
		argp = c->comarg;
		while (argp)
		{
			count += split(macro(argp->argval));
			argp = argp->argnxt;
		}
	}
	return(count);
}

static int
split(s)		/* blank interpretation routine */
register CHTYPE	*s;
{
	register CHTYPE	*argp;
	register int	c;
	int		count = 0;

	for (;;)
	{
		sigchk();
		argp = (CHTYPE *)((char *)locstak() + BYTESPERWORD);
		while ((c = *s++, !any(c, ifsnod.namval) && c))
			*argp++ = c;
		if (argp == (CHTYPE *)((char *)staktop + BYTESPERWORD))
		{
			if (c)
			{
				continue;
			}
			else
			{
				return(count);
			}
		}
		else if (c == 0)
			s--;
		/*
		 * file name generation
		 */

		argp = endstak(argp);

		if ((flags & nofngflg) == 0 && 
			(c = expand(((struct argnod *)argp)->argval, 0)))
			count += c;
		else
		{
			makearg(argp);
			count++;
		}
		gchain = (struct argnod *)((int)gchain | ARGMK);
	}
}

#ifdef ACCT
#include	<sys/types.h>
#include	"acctdef.h"
#include	<sys/acct.h>
#include 	<sys/times.h>

struct acct sabuf;
struct tms buffer;
extern long times();
static long before;
static int shaccton;	/* 0 implies do not write record on exit
			   1 implies write acct record on exit
			*/


/*
 *	suspend accounting until turned on by preacct()
 */

suspacct()
{
	shaccton = 0;
}

preacct(cmdadr)
	CHTYPE *cmdadr;
{
	CHTYPE *simple();

	if (acctnod.namval && *acctnod.namval)
	{
		sabuf.ac_btime = time((long *)0);
		before = times(&buffer);
		sabuf.ac_uid = getuid();
		sabuf.ac_gid = getgid();
		strncpy(sabuf.ac_comm, wtoc(simple(cmdadr)), sizeof(sabuf.ac_comm));
		shaccton = 1;
	}
}

#include	<fcntl.h>

doacct()
{
	int fd;
	long int after;

	if (shaccton)
	{
		after = times(&buffer);
		sabuf.ac_utime = compress(buffer.tms_utime + buffer.tms_cutime);
		sabuf.ac_stime = compress(buffer.tms_stime + buffer.tms_cstime);
		sabuf.ac_etime = compress(after - before);

		if ((fd = open(wtoc(acctnod.namval), O_WRONLY | O_APPEND | O_CREAT, 0666)) != -1)
		{
			write(fd, &sabuf, sizeof(sabuf));
			close(fd);
		}
	}
}

/*
 *	Produce a pseudo-floating point representation
 *	with 3 bits base-8 exponent, 13 bits fraction
 */

compress(t)
	register time_t t;
{
	register exp = 0;
	register rund = 0;

	while (t >= 8192)
	{
		exp++;
		rund = t & 04;
		t >>= 3;
	}

	if (rund)
	{
		t++;
		if (t >= 8192)
		{
			t >>= 3;
			exp++;
		}
	}

	return((exp << 13) + t);
}
#endif
