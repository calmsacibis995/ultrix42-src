#ifndef lint
static CHTYPE *sccsid = "@(#)macro.c	4.1      7/17/90";
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
 * 004 - Gary A. Gaudet, Mon Apr 17 10:34:15 EDT 1989
 *		Further fix to here doc bug.
 *
 * 003 - Gary A. Gaudet, Wed Mar  8 15:41:41 EST 1989
 *		Fixes here doc bug QAR #633
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 	MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	i18n version of csh
 */

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"

static CHTYPE	quote;	/* used locally */
static CHTYPE	quoted;	/* used locally */



static void	/* DAG -- bug fix (no value returned) */
copyto(endch)
register CHTYPE	endch;
{
	register CHTYPE	c;

	while ((c = getch(endch)) != endch && c)
		pushstak(c | quote);
	zerostak();
	if (c != endch)
		error(badsub);
}

static
skipto(endch)
register CHTYPE	endch;
{
	/*
	 * skip chars up to }
	 */
	register CHTYPE	c;

	while ((c = readc()) && c != endch)
	{
		switch (c)
		{
		case SQUOTE:
			skipto(SQUOTE);
			break;

		case DQUOTE:
			skipto(DQUOTE);
			break;

		case DOLLAR:
			if (readc() == BRACE)
				skipto('}');
		}
	}
	if (c != endch)
		error(badsub);
}

static
getch(endch)
CHTYPE	endch;
{
	register CHTYPE	d;

retry:
	d = readc();
	if (!subchar(d))
		return(d);
	if (d == DOLLAR)
	{
		register int	c;

		if ((c = readc(), dolchar(c)))
		{
			struct namnod *n = (struct namnod *)NIL;
			int		dolg = 0;
			BOOL		bra;
			BOOL		nulflg;
			register CHTYPE	*argp, *v;
			CHTYPE		idb[2];
			CHTYPE		*id = idb;

			if (bra = (c == BRACE))
				c = readc();
			if (letter(c))
			{
				argp = (CHTYPE *)relstak();
				while (alphanum(c))
				{
					pushstak(c);
					c = readc();
				}
				zerostak();
				n = lookup(absstak(argp));
				setstak(argp);
				if (n->namflg & N_FUNCTN)
					error(badsub);
				v = n->namval;
				id = n->namid;
				peekc = c | MARK;
			}
			else if (digchar(c))
			{
				*id = c;
				idb[1] = 0;
				if (astchar(c))
				{
					dolg = 1;
					c = '1';
				}
				c -= '0';
				v = ((c == 0) ? cmdadr : (c <= dolc) ? dolv[c] : (CHTYPE *)(dolg = 0));
			}
			else if (c == '$')
				v = pidadr;
			else if (c == '!')
				v = pcsadr;
			else if (c == '#')
			{
				itos(dolc);
				v = numbuf;
			}
			else if (c == '?')
			{
				itos(retval);
				v = numbuf;
			}
			else if (c == '-')
				v = flagadr;
			else if (bra)
				error(badsub);
			else
				goto retry;
			c = readc();
			if (c == ':' && bra)	/* null and unset fix */
			{
				nulflg = 1;
				c = readc();
			}
			else
				nulflg = 0;
			if (!defchar(c) && bra)
				error(badsub);
			argp = 0;
			if (bra)
			{
				if (c != '}')
				{
					argp = (CHTYPE *)relstak();
					if ((v == 0 || (nulflg && *v == 0)) ^ (setchar(c)))
						copyto('}');
					else
						skipto('}');
					argp = absstak(argp);
				}
			}
			else
			{
				peekc = c | MARK;
				c = 0;
			}
			if (v && (!nulflg || *v))
			{
				CHTYPE tmp = (*id == '*' ? SP | quote : SP);

				if (c != '+')
				{
					for (;;)
					{
						if (*v == 0 && quote)
							pushstak(QUOTE);
						else
						{
							while (c = *v++)
								pushstak(c | quote);
						}

						if (dolg == 0 || (++dolg > dolc))
							break;
						else
						{
							v = dolv[dolg];
							pushstak(tmp);
						}
					}
				}
			}
			else if (argp)
			{
				if (c == '?')
					failed(id, *argp ? argp : badparam);
				else if (c == '=')
				{
					if (n)
					{
						trim(argp);
						assign(n, argp);
					}
					else
						error(badsub);
				}
			}
			else if (flags & setflg)
				failed(id, unset);
			goto retry;
		}
		else
			peekc = c | MARK;
	}
	else if (d == endch)
		return(d);
	else if (d == SQUOTE)
	{
		comsubst();
		goto retry;
	}
	else if (d == DQUOTE)
	{
		quoted++;
		quote ^= QUOTE;
		goto retry;
	}
	return(d);
}

CHTYPE *
macro(as)
CHTYPE	*as;
{
	/*
	 * Strip "" and do $ substitution
	 * Leaves result on top of stack
	 */
	register BOOL	savqu = quoted;
	register CHTYPE	savq = quote;
	struct filehdr	fb;

	push(&fb);
	estabf(as);
	usestak();
	quote = 0;
	quoted = 0;
	copyto(0);
	pop();
	if (quoted && (stakbot == staktop))
		pushstak(QUOTE);
/*
 * above is the fix for *'.c' bug
 */
	quote = savq;
	quoted = savqu;
	return(fixstak());
}

static
comsubst()
{
	/*
	 * command substn
	 */
	struct fileblk	cb;
	register CHTYPE	d;
	register CHTYPE *savptr = fixstak();

	usestak();
	while ((d = readc()) != SQUOTE && d)
		pushstak(d);
	{
		register CHTYPE	*argc;

		trim(argc = fixstak());
		push(&cb);
		estabf(argc);
	}
	{
		register struct trenod *t = makefork(FPOU, cmd(EOFSYM, MTFLG | NLFLG));
		int		pv[2];

		/*
		 * this is done like this so that the pipe
		 * is open only when needed
		 */
		chkpipe(pv);
		initf(pv[INPIPE]);
		execute(t, 0, (int)(flags & errflg), 0, pv);
		close(pv[OTPIPE]);
	}
	tdystak(savptr);
	staktop = movstr(savptr, stakbot);
	while (d = readc()) {	/* 002 GAG - uncommented bug fix for MIPS port */
		if (staktop >= brkend) {
			setbrk(brkincr);
		}
		pushstak(d | quote);
	}
	await(0, 0);
	while (stakbot != staktop)
	{
		if ((*--staktop & STRIP) != NL)
		{
			++staktop;
			break;
		}
	}
	pop();
}

#define CPYSIZ	512

subst(in, ot)
int	in, ot;
{
	register CHTYPE	c;
	struct fileblk	fb;
	register int	count = CPYSIZ;

	push(&fb);
	initf(in);
	/*
	 * DQUOTE used to stop it from quoting
	 */
	while (c = (getchtype(DQUOTE) & STRIP))
	{
		pushstak(c);
		if (--count == 0)
		{
			flush(ot);
			count = CPYSIZ;
		}
	}
	flush(ot);
	pop();
}

static
flush(ot)
{
	write(ot, wtoc(stakbot), staktop - stakbot);
	if (flags & execpr)
		write(output, wtoc(stakbot), staktop - stakbot);
	staktop = stakbot;
}
/*
 * 003 GAG - the following subroutines read in a hereis document from
 *	/tmp/sh$$ as CHTYPE. They are copied from other subroutines
 *	that read in "char".
 */

static
getchtype(endch) /* 003 GAG - copied from getc */
CHTYPE	endch;
{
	register CHTYPE	d;

retry:
	d = readchtype();
	if (!subchar(d))
		return(d);
	if (d == DOLLAR)
	{
		register int	c;

		if ((c = readchtype(), dolchar(c)))
		{
			struct namnod *n = (struct namnod *)NIL;
			int		dolg = 0;
			BOOL		bra;
			BOOL		nulflg;
			register CHTYPE	*argp, *v;
			CHTYPE		idb[2];
			CHTYPE		*id = idb;

			if (bra = (c == BRACE))
				c = readchtype();
			if (letter(c))
			{
				argp = (CHTYPE *)relstak();
				while (alphanum(c))
				{
					pushstak(c);
					c = readchtype();
				}
				zerostak();
				n = lookup(absstak(argp));
				setstak(argp);
				if (n->namflg & N_FUNCTN)
					error(badsub);
				v = n->namval;
				id = n->namid;
				peekc = c | MARK;
			}
			else if (digchar(c))
			{
				*id = c;
				idb[1] = 0;
				if (astchar(c))
				{
					dolg = 1;
					c = '1';
				}
				c -= '0';
				v = ((c == 0) ? cmdadr : (c <= dolc) ? dolv[c] : (CHTYPE *)(dolg = 0));
			}
			else if (c == '$')
				v = pidadr;
			else if (c == '!')
				v = pcsadr;
			else if (c == '#')
			{
				itos(dolc);
				v = numbuf;
			}
			else if (c == '?')
			{
				itos(retval);
				v = numbuf;
			}
			else if (c == '-')
				v = flagadr;
			else if (bra)
				error(badsub);
			else
				goto retry;
			c = readchtype();
			if (c == ':' && bra)	/* null and unset fix */
			{
				nulflg = 1;
				c = readchtype();
			}
			else
				nulflg = 0;
			if (!defchar(c) && bra)
				error(badsub);
			argp = 0;
			if (bra)
			{
				if (c != '}')
				{
					argp = (CHTYPE *)relstak();
					if ((v == 0 || (nulflg && *v == 0)) ^ (setchar(c)))
						copyto('}');
					else
						skipto('}');
					argp = absstak(argp);
				}
			}
			else
			{
				peekc = c | MARK;
				c = 0;
			}
			if (v && (!nulflg || *v))
			{
				CHTYPE tmp = (*id == '*' ? SP | quote : SP);

				if (c != '+')
				{
					for (;;)
					{
						if (*v == 0 && quote)
							pushstak(QUOTE);
						else
						{
							while (c = *v++)
								pushstak(c | quote);
						}

						if (dolg == 0 || (++dolg > dolc))
							break;
						else
						{
							v = dolv[dolg];
							pushstak(tmp);
						}
					}
				}
			}
			else if (argp)
			{
				if (c == '?')
					failed(id, *argp ? argp : badparam);
				else if (c == '=')
				{
					if (n)
					{
						trim(argp);
						assign(n, argp);
					}
					else
						error(badsub);
				}
			}
			else if (flags & setflg)
				failed(id, unset);
			goto retry;
		}
		else
			peekc = c | MARK;
	}
	else if (d == endch)
		return(d);
	else if (d == SQUOTE)
	{
		comsubst();
		goto retry;
	}
	else if (d == DQUOTE)
	{
		quoted++;
		quote ^= QUOTE;
		goto retry;
	}
	return(d);
}

readchtype() /* 003 GAG - copied from readc */
{
	register CHTYPE	c;
	register int	len;
	register struct fileblk *f;

	if (peekn)
	{
		peekc = peekn;
		peekn = 0;
	}
	if (peekc)
	{
		c = peekc;
		peekc = 0;
		return(c);
	}
	f = standin;
retry:
	if (f && f->fnxt != f->fend)	/* GAG - MIPS port */
	{
		if ((c = *f->fnxt++) == 0)
		{
			if (f->feval)
			{
				if (estabf(*f->feval++))
					c = EOF;
				else
					c = SP;
			}
			else
				goto retry;	/* = c = readc(); */
		}
		if (flags & readpr && standin->fstak == 0)
			prc(c);
		if (c == NL)
			f->flin++;
	}
	else if (f->feof || f->fdes < 0)
	{
		c = EOF;
		f->feof++;
	}
	else if ((len = readbchtype()) <= 0)
	{
		close(f->fdes);
		f->fdes = -1;
		c = EOF;
		f->feof++;
	}
	else
	{
		f->fend = (f->fnxt = f->fbuf) + len;
		goto retry;
	}
	return(c);
}

static
readbchtype() /* 003 GAG - copied from readb */
{
	register struct fileblk *f = standin;
	register int	len;

	do
	{
		if (trapnote & SIGSET)
		{
			newline();
			sigchk();
		}
		else if ((trapnote & TRAPSET) && (rwait > 0))
		{
			newline();
			chktrap();
			clearup();
		}
	} while ((len = readwchtype(f->fdes, f->fbuf, f->fsiz)) < 0 && trapnote);
	return(len);
}

readwchtype(fd, w, l) /* 003 GAG - copied from readw */
int fd;
register CHTYPE *w;
int l;
{	CHTYPE buf[1024];
	register i;
	register CHTYPE *cp = buf;
	int len;
					/* 004 - GAG */
	i = len = read(fd, cp, l*sizeof(CHTYPE))/sizeof(CHTYPE);
	while (i-- > 0)
		*w++ = *cp++;
	return len;
}
