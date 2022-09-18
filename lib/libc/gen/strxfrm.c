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
static char	sccsid[] = "@(#)strxfrm.c	4.1	(ULTRIX)	7/3/90";
#endif /* lint */

/*
 *
 *   File name: 	stxfrm.c
 *
 *   Source file description:
 *	This file implements the ANSI strxfrm() function.
 *
 *   Functions:
 *	setlocale()
 *
 *   Modification history:
 *	 12-Mar-1987, ARG.
 *		Created.
 *
 *       WR001 1-9-90  Wendy Rannenberg
 *           Changed so it passes straight ASCII (C locale) though
 *           and corrected length of returned string.
 */

#include <i_defs.h>
#include <i_errno.h>
#include <sys/types.h>

/* 
 * strxfrm -- transform a string for comparison by strcmp() etc.
 *	
 * SYNOPSIS:
 *	size_t
 *	strxfrm(to, maxsize, from)
 *	char *to;
 *	size_t maxsize;
 *	char *from;
 *
 * DESCRIPTION:
 *	This is the ANSI defined function used to transform a string
 *	so that two transformed strings can be collated according to
 *	the programs locale using strcmp() or memcmp(). The length of
 *	the transformed string may be much longer than the original.
 *
 * RETURN:
 *	The length of the resultant string, if the string is too long to
 *	fit into the to buffer the length of buffer required is returned.
 *	The original contents of the to buffer are destroyed. 
 *	NOTE: that we NEVER count the nul terminator in the nul byte case
 */

/*
 * a useful definition to stop us overrunning the to buffer
 */
#define ADD(weight)	if (len++ < maxsize) *to++ = CONVWEIGHT(weight)
/*
 * a useful macro to output weights suitable for signed characters
 *
 * 0 => 0,   1 - 128 => -128 - -1,   129 - 255 => 1 - 127
 */
#define CONVWEIGHT(wgt) ((wgt > 128 ) ? wgt - 128 : ((wgt > 0) ? wgt - 129 : 0))

size_t
strxfrm(to, from, maxsize)
char  *to;			
size_t maxsize;
char  *from;
{	size_t	len = 0;
	char 	*fp;		/* from pointer				*/
	i_char	indx;		/* index into the collation table	*/
	coll    *ccp;		/* pointer to collation table		*/
	int	dipht = 0;	/* set and used by _getis		*/
	int	fdipht = 0;	/* set and used by _getis		*/
	i_char	dbl = 0;	/* set and used by _getis		*/
	int	skip;		/* number of padding sec weights	*/

	/*
	 * if no collation table set by setlocale() simply 
	 * do a move of the original data. Taking care to copy the
	 * nul terminator.
	 *
	 * WR001 just pass the C locale through here 
	 */
	if (_lc_cldflt == (col_tab *)0) {
		int c;
		
		len = strlen(from);    /* we shouldn't change the length */

		do {
			c = *from++ & 0xFF;
			*to++ = c;    /* do a simple copy not an ADD() */
		} while (c);
		return (len);
	}
	/*
	 * we have a collation table so we need to put in the
  	 * "to" string all the primary weights followed by
	 * the secondary weights.
	 */
	_isinit(_lc_cldflt);
	ccp = _lc_cldflt->col_col;

	/* 
	 * firstly output primary weights
	 */
	fp = from - 1;
	for (indx = _getis(&fp, &dipht, &fdipht, &dbl);
	     *fp || dipht;
	     indx = _getis(&fp, &dipht, &fdipht, &dbl)) {
		/* 
		 * check for bad character and skip zero primary weights
		 */
		if (indx == (i_char)I_ERROR)
			return 0;

		if (ccp[indx].cl_prim == 0)
			continue;

		/*
		 * don't overrun to string
		 */
		ADD(ccp[indx].cl_prim + 1);
	}
	/* 
	 * now output secondary weights again adding one 
	 * so that 0 secondary weights will not terminate strcmp.
	 * Skip is set to one to force a 1 to be output if we have
	 * any significant secondary weights. This is done so
	 * that strcmp will mismatch on different length strings
	 */
	skip = 1;

	fp = from - 1;
	for (indx = _getis(&fp, &dipht, &fdipht, &dbl);
	     *fp || dipht;
	     indx = _getis(&fp, &dipht, &fdipht, &dbl)) {
		i_char sec;

		/* 
		 * skip zero primary weights, this also catches the case
		 * of zero secondary weights
		 */
		if (ccp[indx].cl_prim == 0)
			continue;

		/*
		 * if lowest secondary weight don't output unless following
		 * characters have a higher weight
		 */
		if ((sec = ccp[indx].cl_sec) == 1)
			skip++;
		else {
			for ( ; skip > 0; skip--)
				ADD('\001');
			ADD(sec);
		}
	}
	/*
	 * finally add a nul byte to the end of the string.
	 */
	ADD(0);
	return len - 1;
}
