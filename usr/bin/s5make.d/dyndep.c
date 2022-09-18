#ifndef lint
static	char	*sccsid = "@(#)dyndep.c	4.1	(ULTRIX)	8/17/88";
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
 *
 */

#include "defs"
/*
 *	Dynamicdep() checks each dependency by calling runtime().
 *	Runtime() determines if a dependent line contains "$@"
 *	or "$(@F)" or "$(@D)". If so, it makes a new dependent line
 *	and insert it into the dependency chain of the input name, p.
 *	Here, "$@" gets translated to p->namep. That is
 *	the current name on the left of the colon in the
 *	makefile.  Thus,
 *		xyz:	s.$@.c
 *	translates into
 *		xyz:	s.xyz.c
 *
 *	Also, "$(@F)" translates to the same thing without a prededing
 *	directory path (if one exists).
 *	Note, to enter "$@" on a dependency line in a makefile
 *	"$$@" must be typed. This is because `make' expands
 *	macros upon reading them.
 */

#define is_dyn(a)		(any( (a), DOLLAR) )


dynamicdep(p)
register NAMEBLOCK p;
{
	register LINEBLOCK lp, nlp;
	LINEBLOCK backlp=0;

	p->rundep = 1;

	for(lp = p->linep; lp != 0; lp = lp->nextline)
	{
		if( (nlp=runtime(p, lp)) != 0)
			if(backlp)
				backlp->nextline = nlp;
			else
				p->linep = nlp;

		backlp = (nlp == 0) ? lp : nlp;
	}
}


LINEBLOCK runtime(p, lp)
NAMEBLOCK p;
register LINEBLOCK lp;
{
	union
	{
		int u_i;
		NAMEBLOCK u_nam;
	} temp;
	register DEPBLOCK q, nq;
	LINEBLOCK nlp;
	NAMEBLOCK pc;
	CHARSTAR pc1;
	char c;
	CHARSTAR pbuf;
	char buf[128];

	temp.u_i = NO;
	for(q = lp->depp; q != 0; q = q->nextdep)
	{
		if((pc=q->depname) != 0)
		{
			if(is_dyn(pc->namep))
			{
				temp.u_i = YES;
				break;
			}
		}
	}

	if(temp.u_i == NO)
	{
		return(0);
	}

	nlp = ALLOC(lineblock);
	nq  = ALLOC(depblock);

	nlp->nextline = lp->nextline;
	nlp->shp   = lp->shp;
	nlp->depp  = nq;

	for(q = lp->depp; q != 0; q = q->nextdep)
	{
		pc1 = q->depname->namep;
		if(is_dyn(pc1))
		{
			subst(pc1, buf);
			temp.u_nam = srchname(buf);
			if(temp.u_nam == 0)
				temp.u_nam = makename(copys(buf));
			nq->depname = temp.u_nam;
		}
		else
		{
			nq->depname = q->depname;
		}

		if(q->nextdep == 0)
			nq->nextdep = 0;
		else
			nq->nextdep = ALLOC(depblock);

		nq = nq->nextdep;
	}
	return(nlp);
}
