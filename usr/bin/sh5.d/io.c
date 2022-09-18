#ifndef lint
static CHTYPE *sccsid = "@(#)io.c	4.1      7/17/90";
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
 * 004 - Robert Dillberger Mon Apr  3 10:44:29 EDT 1989
 *		fixes quoted hereis bug by writing char to /tmp/sh$$
 *
 * 003 - Gary A. Gaudet, Wed Mar  8 15:29:37 EST 1989
 *		QAR #633
 *		fixes error message scrambling.
 *		fixes hereis bug by writing CHTYPE to /tmp/sh$$
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 	MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	i18n version of csh
 *
 */

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"dup.h"
#include	<fcntl.h>

short topfd;

/* ========	input output and file copying ======== */

initf(fd)
int	fd;
{
	register struct fileblk *f = standin;

	f->fdes = fd;
	f->fsiz = ((flags & oneflg) == 0 ? BUFSIZ : 1);
	f->fnxt = f->fend = f->fbuf;
	f->feval = 0;
	f->flin = 1;
	f->feof = FALSE;
}

estabf(s)
register CHTYPE *s;
{
	register struct fileblk *f;

	(f = standin)->fdes = -1;
	f->fend = length(s) + (f->fnxt = s);
	f->flin = 1;
	return(f->feof = (s == 0));
}

push(af)
struct fileblk *af;
{
	register struct fileblk *f;

	(f = af)->fstak = standin;
	f->feof = 0;
	f->feval = 0;
	standin = f;
}

pop()
{
	register struct fileblk *f;

	if ((f = standin)->fstak)
	{
		if (f && f->fdes >= 0)	/* 002 GAG - MIPS port */
			close(f->fdes);
		standin = f->fstak;
		return(TRUE);
	}
	else
		return(FALSE);
}

struct tempblk *tmpfptr;

pushtemp(fd,tb)
	int fd;
	struct tempblk *tb;
{
	tb->fdes = fd;
	tb->fstak = tmpfptr;
	tmpfptr = tb;
}

poptemp()
{
	if (tmpfptr)
	{
		close(tmpfptr->fdes);
		tmpfptr = tmpfptr->fstak;
		return(TRUE);
	}
	else
		return(FALSE);
}
	
chkpipe(pv)
int	*pv;
{
	if (pipe(pv) < 0 || pv[INPIPE] < 0 || pv[OTPIPE] < 0)
		error(piperr);
}

chkopen(idf)
CHTYPE *idf;
{
	register int	rc;

	if ((rc = open(wtoc(idf), 0)) < 0)
		failed(idf, badopen); /* 003 GAG */
		/*NOTREACHED*/
	else
		return(rc);
}

rename(f1, f2)
register int	f1, f2;
{
#ifdef RES
	if (f1 != f2)
	{
		dup(f1 | DUPFLG, f2);
		close(f1);
		if (f2 == 0)
			ioset |= 1;
	}
#else
	int	fs;

	if (f1 != f2)
	{
		fs = fcntl(f2, F_GETFD, 0);	/* DAG -- use defines */
		close(f2);
		fcntl(f1, F_DUPFD, f2);		/* DAG */
		close(f1);
		if (fs == 1)
			fcntl(f2, F_SETFD, 1);	/* DAG */
		if (f2 == 0)
			ioset |= 1;
	}
#endif
}

create(s)
CHTYPE *s;
{
	register int	rc;

	if ((rc = creat(wtoc(s), 0666)) < 0)
		failed(s, badcreate); /* 003 GAG */
		/*NOTREACHED*/
	else
		return(rc);
}

tmpfil(tb)
	struct tempblk *tb;
{
	int fd;

	itos(serial++);
	movstr(numbuf, tmpnam);
	fd = create(tmpout);
	pushtemp(fd,tb);
	return(fd);
}

/*
 * set by trim
 */
extern BOOL		nosubst;
#define			CPYSIZ		512

copy(ioparg)
struct ionod	*ioparg;
{
	register CHTYPE	*cline;
	register CHTYPE	*clinep;
	register struct ionod	*iop;
	CHTYPE	c;
	CHTYPE	*ends;
	CHTYPE	*start;
	int		fd;
	int		i;
	int		stripflg;
	

	if (iop = ioparg)
	{
		struct tempblk tb;

		copy(iop->iolst);
		ends = mactrim(iop->ioname);
		stripflg = iop->iofile & IOSTRIP;
		if (nosubst)
			iop->iofile &= ~IODOC;
		fd = tmpfil(&tb);

		if (fndef)
			iop->ioname = make(tmpout);
		else
			iop->ioname = cpystak(tmpout);

		iop->iolst = iotemp;
		iotemp = iop;

		cline = clinep = start = locstak();
		if (stripflg)
		{
			iop->iofile &= ~IOSTRIP;
			while (*ends == '\t')
				ends++;
		}
		for (;;)
		{
			chkpr();
			if (nosubst)
			{
				c = readc();
				if (stripflg)
					while (c == '\t')
						c = readc();

				while (!eolchar(c))
				{
					*clinep++ = c;
					c = readc();
				}
			}
			else
			{
				c = nextc(*ends);
				if (stripflg)
					while (c == '\t')
						c = nextc(*ends);
				
				while (!eolchar(c))
				{
					*clinep++ = c;
					c = nextc(*ends);
				}
			}

			*clinep = 0;
			if (eof || eq(cline, ends))
			{
				if ((i = cline - start) > 0)
				{
					if (nosubst) {	/* 004  RTD */
						start[i] = 0;
						write(fd, wtoc(start), i * sizeof(char)); /* 003 GAG */
					}
					else
						write(fd, start, i * sizeof(CHTYPE)); /* 003 GAG */
				}
				break;
			}
			else
				*clinep++ = NL;

			if ((i = clinep - start) < CPYSIZ)
				cline = clinep;
			else
			{
				if (nosubst) {	/* 004 RTD */
					start[i] = 0;
					write(fd, wtoc(start), i * sizeof(char)); /* 003 GAG */
				}
				else
					write(fd, start, i * sizeof(CHTYPE)); /* 003 GAG */
				cline = clinep = start;
			}
		}

		poptemp();		/* pushed in tmpfil -- bug fix for problem
					   deleting in-line scripts */
	}
}


link_iodocs(i)
	struct ionod	*i;
{	char *tmp;
	while(i)
	{
		free(i->iolink);

		itos(serial++);
		movstr(numbuf, tmpnam);
		i->iolink = make(tmpout);
		tmp = (char *)locstak();
		swtoc(tmp, i->ioname);
		link(tmp, wtoc(i->iolink));
		endstak(tmp);

		i = i->iolst;
	}
}


swap_iodoc_nm(i)
	struct ionod	*i;
{
	while(i)
	{
		free(i->ioname);
		i->ioname = i->iolink;
		i->iolink = 0;

		i = i->iolst;
	}
}


savefd(fd)
	int fd;
{
	register int	f;

	f = fcntl(fd, F_DUPFD, 10);
	return(f);
}


restore(last)
	register int	last;
{
	register int 	i;
	register int	dupfd;

	for (i = topfd - 1; i >= last; i--)
	{
		if ((dupfd = fdmap[i].dup_fd) > 0)
			rename(dupfd, fdmap[i].org_fd);
		else
			close(fdmap[i].org_fd);
	}
	topfd = last;
}
