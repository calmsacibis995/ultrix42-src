/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#ifndef lint
static char Sccsid[] = "@(#)_index.c	4.1	(ULTRIX)	7/3/90";
#endif

#include <i_defs.h>
#include <i_errno.h>

/*
 * _isinit -- set the constants used by _getis
 *
 * SYNOPSIS
 * 	void
 *	_isinit(coltab)
 *	col_tab *coltab;
 *
 * DESCRIPTION
 *	Set the constants used bu _getis to save some work in this
 *	heavily called function. This MUST be called before _getis
 *	is used for EACH collation table
 */
static prp_tab *pp;
static i_char  *prptab;
static col_tab *coltab;
static int 	nbspl;

void
_isinit(l_coltab)
col_tab *l_coltab;
{
	pp     = l_coltab->col_prp;
	nbspl  = pp->prp_nbspl;
	prptab = (i_char *)pp->prp_tbl;
	coltab = l_coltab;
}

/*
 * _getis -- get the index into the collation table
 *
 * SYNOPSIS:
 *	i_char
 *	_getis(s, dipht, fdipht, sdbl)
 *	register char **s;
 *	int *dipht;
 *	int *fdipht;
 *	i_char *sdbl;
 *
 * DESCRIPTION:
 *	This function is used in the collation function _coll below to
 *	determine
 *		1) the index of the collation table entry
 *		2) whether the character is a diphtong
 *
 * RETURN:
 *	index of weight entry in collation table
 *	I_ERROR otherwise
 */
i_char
_getis(s, dipht, fdipht, sdbl)
register char **s;		/* string to get characters from	 */
int *dipht;			/* to remember diphtong status		 */
int *fdipht;			/* set to 1 if found a diphtong		 */
i_char *sdbl;			/* to remember second of a double letter */
{
	i_char	c;

	/*
	 * if last time we had a diphtong, return weights for second character
	 */
	if (*dipht)
	{
		*dipht = 0;
		*fdipht = 1;
		return(*sdbl);
	}

	/* 
	 * get the next character. Convenient for us to leave s at the
	 * current character so caller has predecremented
	 */
	(*s)++;

	/*
	 * quick check for argument
	 */
	if ((c = **s & 0377) >= nbspl)
		return((i_char)(I_ERROR));

	/*
	 * deal with the most common case now
	 */

	if ((prptab[c] & (I_DIPHT | I_FIRST)) == 0)
		return (c);


	if (prptab[c] & I_DIPHT)
	{
		/*
		 * c is a diphtong
		 */
		*dipht = 1;
		*sdbl = coltab->col_col[c].cl_sec;
		return(coltab->col_col[c].cl_prim);
	} else {
		/* 
		 * c has property I_FIRST
		 */

		register int i;
		register i_dblt	*ptr;
		i_char	 next;
		i_char	 dbl;

		/*
		 * c may be a double letter
		 * if it is not at the end of the string
		 * and its second part exists too.
		 */
		if ((next = (*s)[1] & 0377) == '\0')
			return(c);

		dbl = (c << 8) + next;

		for (ptr = pp->prp_dblt, i = pp->prp_nbdbl; i-- > 0; ptr++)
		{
			if (ptr->symb == dbl)
			{
				/*
				 * found a double letter
				 */
				(*s)++;

				/*
				 * the double letter could be a diphtong
				 */	
				if (prptab[ptr->indx] & I_DIPHT)
				{
					*dipht = 1;
					*sdbl = coltab->col_col[ptr->indx].cl_sec;
					return(coltab->col_col[ptr->indx].cl_prim);
				}
				return((i_char)ptr->indx);
			}
		}
	}

	/*
	 * c was a simple letter after all
	 */
	return(c);
}
