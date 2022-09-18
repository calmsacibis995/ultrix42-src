#ifndef lint
static CHTYPE *sccsid = "@(#)expand.c	4.1      7/17/90";
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
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	i18n version of csh
 *
 *
 *
 */

/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>

#define MAXDIR	64
struct direct		*readdir();		/* getdir -> readdir */

static struct direct	dirbuf[MAXDIR];
static int		nxtdir = -1;
static int		maxdir = 0;
static CHTYPE		entry[MAXNAMLEN+1];

/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
extern int	addg();


expand(as, rcnt)
	CHTYPE	*as;
{
	int	count; 
	DIR	*dirf;
	BOOL	dir = 0;
	CHTYPE	*rescan = 0;
	register CHTYPE	*s, *cs;
	struct argnod	*schain = gchain;
	struct stat statb;
	BOOL	slash;

	if (trapnote & SIGSET)
		return(0);
	s = cs = as;

	/*
	 * check for meta chars
	 */
	{
		register BOOL open;

		slash = 0;
		open = 0;
		do
		{
			switch (*cs++)
			{
			case 0:
				if (rcnt && slash)
					break;
				else
					return(0);

			case '/':
				slash++;
				open = 0;
				continue;

			case '[':
				open++;
				continue;

			case ']':
				if (open == 0)
					continue;

			case '?':
			case '*':
				if (rcnt > slash)
					continue;
				else
					cs--;
				break;


			default:
				continue;
			}
			break;
		} while (TRUE);
	}

	for (;;)
	{
		if (cs == s)
		{
			s = nullstr;
			break;
		}
		else if (*--cs == '/')
		{
			*cs = 0;
			if (s == cs)
				s = "/";
			break;
		}
	}

#ifdef OBSOLETE 		/* system V 14-char directory name */
	if ((dirf = open(*s ? s : ".", 0)) > 0)
	{
		if (fstat(dirf, &statb) != -1 &&
		    (statb.st_mode & S_IFMT) == S_IFDIR)
			dir++;
		else
			close(dirf);
	}

#else
	if (stat(wtoc(s),&statb) >= 0
		&& (statb.st_mode&S_IFMT) == S_IFDIR
		&& (dirf = opendir(wtoc(s))) != NULL)
		dir++;
#endif

	count = 0;
	if (*cs == 0)
		*cs++ = QUOTE;
	if (dir)		/* check for rescan */
	{
		register CHTYPE *rs;
		struct direct *e;

		rs = cs;
		do
		{
			if (*rs == '/')
			{
				rescan = rs;
				*rs = 0;
				gchain = 0;
			}
		} while (*rs++);

		while ((e = readdir(dirf)) && (trapnote & SIGSET) == 0)
		{
			*(movstr(ctow(e->d_name), entry)) = 0;

			if (entry[0] == '.' && *cs != '.')
#ifndef BOURNE
				continue;
#else
			{
				if (entry[1] == 0)
					continue;
				if (entry[1] == '.' && entry[2] == 0)
					continue;
			}
#endif

			if (gmatch(entry, cs))
			{
				addg(s, entry, rescan);
				count++;
			}
		}
		closedir(dirf);

		if (rescan)
		{
			register struct argnod	*rchain;

			rchain = gchain;
			gchain = schain;
			if (count)
			{
				count = 0;
				while (rchain)
				{
					count += expand(rchain->argval, slash + 1);
					rchain = rchain->argnxt;
				}
			}
			*rescan = '/';
		}
	}

	{
		register CHTYPE	c;

		s = as;
		while (c = *s)
			*s++ = (c & STRIP ? c : '/');
	}
	return(count);
}

reset_dir()
{
	nxtdir = -1;
	maxdir = 0;
}

#ifdef OBSOLETE
/*
 * read next directory entry
 * and ignore inode == 0
 *
 */

struct direct *
getdir(dirf)
{
	for (;;)
	{
		if (++nxtdir == maxdir)
		{
			int r;

			r = read(dirf, dirbuf, sizeof(dirbuf)) / sizeof(struct direct);
			if (maxdir = r)
				nxtdir = 0;
			else
			{
				nxtdir = -1;
				return(0);
			}
		}

		/* nxtdir is next available entry */
		if (dirbuf[nxtdir].d_ino == 0)
			continue;


		return(&dirbuf[nxtdir]);
	}
}
#endif


gmatch(s, p)
register CHTYPE	*s, *p;
{
	register int	scc;
	CHTYPE		c;

	if (scc = *s++)
	{
		if ((scc &= STRIP) == 0)
			scc = QUOTE;
	}
	switch (c = *p++)
	{
	case '[':
		{
			BOOL ok;
			int lc;
			int notflag = 0;

			ok = 0;
			lc = 077777;
			if (*p == '!')
			{
				notflag = 1;
				p++;
			}
			while (c = *p++)
			{
				if (c == ']')
					return(ok ? gmatch(s, p) : 0);
				else if (c == MINUS)
				{
					if (notflag)
					{
						if (scc < lc || scc > *(p++))
							ok++;
						else
							return(0);
					}
					else
					{
						if (lc <= scc && scc <= (*p++))
							ok++;
					}
				}
				else
				{
					lc = c & STRIP;
					if (notflag)
					{
						if (scc && scc != lc)
							ok++;
						else
							return(0);
					}
					else
					{
						if (scc == lc)
							ok++;
					}
				}
			}
			return(0);
		}

	default:
		if ((c & STRIP) != scc)
			return(0);

	case '?':
		return(scc ? gmatch(s, p) : 0);

	case '*':
		while (*p == '*')
			p++;

		if (*p == 0)
			return(1);
		--s;
		while (*s)
		{
			if (gmatch(s++, p))
				return(1);
		}
		return(0);

	case 0:
		return(scc == 0);
	}
}

static int
addg(as1, as2, as3)
CHTYPE	*as1, *as2, *as3;
{
	register CHTYPE	*s1, *s2;
	register int	c;

	s2 = (CHTYPE *)((char *)locstak() + BYTESPERWORD);

	s1 = as1;
	while (c = *s1++)
	{
		if ((c &= STRIP) == 0)
		{
			*s2++ = '/';
			break;
		}
		*s2++ = c;
	}
	s1 = as2;
	while (*s2 = *s1++)
		s2++;
	if (s1 = as3)
	{
		*s2++ = '/';
		while (*s2++ = *++s1);
	}
	makearg(endstak(s2));
}

makearg(args)
	register struct argnod *args;
{
	args->argnxt = gchain;
	gchain = args;
}


