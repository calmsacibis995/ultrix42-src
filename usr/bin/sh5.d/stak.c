#ifndef lint
static CHTYPE *sccsid = "@(#)stak.c	4.1      7/17/90";
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
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	storage allocation	======== */

CHTYPE *
getstak(asize)			/* allocate requested stack */
int	asize;
{
	register CHTYPE	*oldstak;
	register int	size;

	size = round(asize, BYTESPERWORD);
	oldstak = stakbot;
	staktop = stakbot += size;
	return(oldstak);
}

/*
 * set up stack for local use
 * should be followed by `endstak'
 */
CHTYPE *
locstak()
{
	if (brkend - stakbot < BRKINCR)
	{
		if (setbrk(brkincr) == -1)
			error(nostack);
		if (brkincr < BRKMAX)
			brkincr += 256;
	}
	return(stakbot);
}

CHTYPE *
savstak()
{
	assert(staktop == stakbot);
	return(stakbot);
}

CHTYPE *
endstak(argp)		/* tidy up after `locstak' */
register CHTYPE	*argp;
{
	register CHTYPE	*oldstak;

	*argp++ = 0;
	oldstak = stakbot;
	stakbot = staktop = (CHTYPE *)round(argp, BYTESPERWORD);
	return(oldstak);
}

tdystak(x)		/* try to bring stack back to x */
register CHTYPE	*x;
{
	while ((CHTYPE *)(stakbsy) > (CHTYPE *)(x))
	{
		free(stakbsy);
		stakbsy = stakbsy->word;
	}
	staktop = stakbot = max((CHTYPE *)(x), (CHTYPE *)(stakbas));
	rmtemp(x);
}

stakchk()
{
	if ((brkend - stakbas) > BRKINCR + BRKINCR)
		setbrk(-BRKINCR);
}

CHTYPE *
cpystak(x)
CHTYPE	*x;
{
	return(endstak(movstr(x, locstak())));
}
