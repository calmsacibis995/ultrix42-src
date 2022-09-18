#ifndef lint
static CHTYPE *sccsid = "@(#)main.c	4.1 (ULTRIX) 7/17/90";
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
 * 004 - David Lindner Thu Dec 14 13:08:01 EST 1989
 *	 Added LOGNAME for POSIX compliance
 *
 * 003 - David Lindner Wed Jul 26 10:24:54 EDT 1989
 *	 Fixed badfilename bug
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	 i18n version of csh
 */

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"
#include	"timeout.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include        "dup.h"
#include	<fcntl.h>	/* DAG -- for defines */
#include	<pwd.h>		/* DJL 004 */
#include	<strings.h>

#ifdef RES
#include	<sgtty.h>
#endif

static BOOL	beenhere = FALSE;
CHTYPE		tmpout[20];
struct fileblk	stdfile;
struct fileblk *standin = &stdfile;
int mailchk = 0;

static CHTYPE	*mailp;
static long	*mod_time = 0;

#ifdef pdp11
#include	<execargs.h>
#endif

extern int	exfile();
extern CHTYPE 	*simple();



main(c, v)
int	c;
char 	*v[];
{
	register int	rflag = ttyflg;
	int		rsflag = 1;	/* local restricted flag */
	struct namnod	*n;
	static struct	passwd *logrec;		/* DJL 004 */
	static char	logbuf[LOGNAME_MAX+1];
	static char	cbuf[LOGNAME_MAX+9]={'L','O','G','N','A','M','E','='};
	static CHTYPE	*wbuf;

	stdsigs();

	/*
	 * initialise storage allocation
	 */

	stakbot = 0;
	addblok((unsigned)0);

	/*
	 * set names from userenv
	 */

	setup_env();

	/*
	 * 'rsflag' is non-zero if SHELL variable is
	 *  set in environment and contains an'r' in
	 *  the simple file part of the value.
	 */
	if (n = findnam("SHELL"))
	{
		if (any('r', simple(n->namval)))
			rsflag = 0;
	}

	/*
	 * a shell is also restricted if argv(0) has
	 * an 'r' in its simple name
	 */

#ifndef RES

	if (c > 0 && any('r', simple(ctow(*v))))
		rflag = 0;

#endif

	hcreate();
	set_dotpath();

	/*
	 * look for options
	 * dolc is $#
	 */
	dolc = options(c, v, 1);

	if (dolc < 2)
	{
		flags |= stdflg;
		{
			register CHTYPE *flagc = flagadr;

			while (*flagc)
				flagc++;
			*flagc++ = STDFLG;
			*flagc = 0;
		}
	}
	if ((flags & stdflg) == 0)
		dolc--;
	dolv = (CHTYPE **)(v + c - dolc);
	dolc--;

	/*
	 * return here for shell file execution
	 * but not for parenthesis subshells
	 */
	setjmp(subshell);

	/*
	 * number of positional parameters
	 */
	replace(&cmdadr, dolv[0]);	/* cmdadr is $1 */
	replace(&calladr, v[0]);	/* DJL 003 */

	/*
	 * DJL 004
	 * set logname
	 */
	if (logrec = getpwuid(getuid()))
		strncpy(logbuf, logrec->pw_name, LOGNAME_MAX+1);
	else {
		prs("LOGNAME set to null\n");
		strcpy(logbuf, "");	
	}
	strcat(cbuf, logbuf);
	wbuf = ctow(cbuf);
	setname(wbuf, N_ENVNAM);

	/*
	 * set pidname '$$'
	 */
	assnum(&pidadr, getpid());

	/*
	 * set up temp file names
	 */
	settmp();

	/*
	 * default internal field separators - $IFS
	 */
	dfault(&ifsnod, sptbnl);

	dfault(&mchknod, MAILCHECK);
	mailchk = stoi(mchknod.namval);

	if ((beenhere++) == FALSE)	/* ? profile */
	{
		if (*(simple(cmdadr)) == '-')
		{			/* system profile */

#ifndef RES

			if ((input = pathopen(nullstr, sysprofile)) >= 0)
				exfile(rflag);		/* file exists */

#endif

			if ((input = pathopen(nullstr, profile)) >= 0)
			{
				exfile(rflag);
				flags &= ~ttyflg;
			}
		}
		if (rsflag == 0 || rflag == 0)
			flags |= rshflg;
		/*
		 * open input file if specified
		 */
		if (comdiv)
		{
			estabf(comdiv);
			input = -1;
		}
		else
		{
			input = ((flags & stdflg) ? 0 : chkopen(cmdadr));

#ifdef ACCT
			if (input != 0)
				preacct(cmdadr);
#endif
			comdiv--;
		}
	}
#ifdef pdp11
	else
		*execargs = (CHTYPE *)dolv;	/* for `ps' cmd */
#endif
		
	exfile(0);
	done();
}

static int
exfile(prof)
BOOL	prof;
{
	long	mailtime = 0;	/* Must not be a register variable */
	long 	curtime = 0;
	register int	userid;

	/*
	 * move input
	 */
	if (input > 0)
	{
		Ldup(input, INIO);
		input = INIO;
	}

	userid = geteuid();

	/*
	 * decide whether interactive
	 */
	if ((flags & intflg) ||
	    ((flags&oneflg) == 0 &&
	    isatty(output) &&
	    isatty(input)) )
	    
	{
		dfault(&ps1nod, (userid ? stdprompt : supprompt));
		dfault(&ps2nod, readmsg);
		flags |= ttyflg | prompt;
		ignsig(SIGTERM);
		if (mailpnod.namflg != N_DEFAULT)
			setmail(mailpnod.namval);
		else
			setmail(mailnod.namval);
	}
	else
	{
		flags |= prof;
		flags &= ~prompt;
	}

	if (setjmp(errshell) && prof)
	{
		close(input);
		return;
	}
	/*
	 * error return here
	 */

	loopcnt = peekc = peekn = 0;
	fndef = 0;
	nohash = 0;
	iopend = 0;

	if (input >= 0)
		initf(input);
	/*
	 * command loop
	 */
	for (;;)
	{
		tdystak(0);
		stakchk();	/* may reduce sbrk */
		exitset();

		if ((flags & prompt) && standin->fstak == 0 && !eof)
		{

			if (mailp)
			{
				time(&curtime);

				if ((curtime - mailtime) >= mailchk)
				{
					chkmail();
				        mailtime = curtime;
				}
			}

			prs(ps1nod.namval);

#ifdef TIME_OUT
			alarm(TIMEOUT);
#endif

			flags |= waiting;
		}

		trapnote = 0;
		peekc = readc();
		if (eof)
			return;

#ifdef TIME_OUT
		alarm(0);
#endif

		flags &= ~waiting;

		execute(cmd(NL, MTFLG), 0, eflag);
		eof |= (flags & oneflg);
	}
}

chkpr()
{
	if ((flags & prompt) && standin->fstak == 0)
		prs(ps2nod.namval);
}

settmp()
{
	itos(getpid());
	serial = 0;
	movstr("/tmp/sh-", tmpout);
	tmpnam = movstr(numbuf, &tmpout[TMPNAM]);
}

Ldup(fa, fb)
register int	fa, fb;
{
#ifdef RES

	dup(fa | DUPFLG, fb);
	close(fa);
	ioctl(fb, FIOCLEX, 0);

#else

	if (fa >= 0)
		{ close(fb);
		  fcntl(fa,F_DUPFD,fb);		/* normal dup */	/* DAG -- use defines */
		  close(fa);
		  fcntl(fb, F_SETFD, 1);	/* autoclose for fb */	/* DAG */
		}

#endif
}


chkmail()
{
	register CHTYPE 	*s = mailp;
	register CHTYPE	*save;

	long	*ptr = mod_time;
	CHTYPE	*start;
	BOOL	flg; 
	struct stat	statb;

	while (*s)
	{
		start = s;
		save = 0;
		flg = 0;

		while (*s)
		{
			if (*s != COLON)	
			{
				if (*s == '%' && save == 0)
					save = s;
			
				s++;
			}
			else
			{
				flg = 1;
				*s = 0;
			}
		}

		if (save)
			*save = 0;

		if (*start && stat(wtoc(start), &statb) >= 0)
		{
			if(statb.st_size && *ptr
				&& statb.st_mtime != *ptr)
			{
				if (save)
				{
					prs(save+1);
					newline();
				}
				else
					prs(mailmsg);
			}
			*ptr = statb.st_mtime;
		}
		else if (*ptr == 0)
			*ptr = 1;

		if (save)
			*save = '%';

		if (flg)
			*s++ = COLON;

		ptr++;
	}
}


setmail(mailpath)
	CHTYPE *mailpath;
{
	register CHTYPE	*s = mailpath;
	register int 	cnt = 1;

	long	*ptr;

	free(mod_time);
	if (mailp = mailpath)
	{
		while (*s)
		{
			if (*s == COLON)
				cnt += 1;

			s++;
		}

		ptr = mod_time = (long *)alloc(sizeof(long) * cnt);

		while (cnt)
		{
			*ptr = 0;
			ptr++;
			cnt--;
		}
	}
}
