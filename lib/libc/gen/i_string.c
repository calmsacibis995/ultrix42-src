#ifndef lint
static char Sccsid[] = "@(#)i_string.c	4.1 (ULTRIX) 7/3/90";
#endif

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

/*
 * _coll -- general collation algorithm
 *
 * SYNOPSIS:
 *	static int
 *	_coll(str1, str2, n, coltab)
 *	char *str1;
 *	char *str2;
 *	int n;
 *	col_tab *coltab;
 *
 * DESCRIPTION:
 *	_coll will collate the two strings "str1" and "str2" either for
 *	"n" characters or, if "n == -1" for all characters according to
 *	the given collation table "coltab".
 *
 *	This essentially implements a two pass collation algorithm which
 *	requires a one byte lookahead.
 *
 *	The letters count is the following:
 *	- if the symbol is a diphtong, it is counted as two letters
 *	- if the symbol is a double letter (i.e. collated as one single
 *	  letter), it is counted as one letter.
 *
 * RETURN:
 *	str1 < str2	< 0
 *	str1 = str2	= 0	(WARNING: also returned for errors!)
 *	str1 > str2	> 0
 *
 * RESTRICTIONS:
 *	This function will reset i_errno to zero, thus covering up any
 *	error that happend before. This is done because we have no way
 *	to signal successful completion otherwise.
 */

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 001	Wendy Rannenberg Fri Feb  2 11:31:48 EST 1990
 *	- Fixed so secondary weighting mechanisms do not sort dipthongs
 *	  last.
 *
 */

#include <i_defs.h>
#include <i_errno.h>

extern i_char _getis();

static int
_coll(str1, str2, n, coltab)
char *str1;		/* first argument to collate			*/
char *str2;		/* second argument to collate			*/
int n;			/* number of codes to compare, -1 if all	*/
col_tab *coltab;	/* collation table				*/
{
	i_char indx1;
	i_char indx2;
	char *s1 = str1 - 1;		/* predecrement for _getis	*/
	char *s2 = str2 - 1;
	i_char sdbl1;			/* for use by _getis only	*/
	i_char sdbl2;			/* for use by _getis only	*/
	int dipht1 = 0;			/* for use by _getis only	*/
	int dipht2 = 0;			/* for use by _getis only	*/
	int fdipht1 = 0;
	int fdipht2 = 0;
	coll *ccp = coltab->col_col;	/* pointer to col_col		*/
	int sec  = 0;			/* secondary weight value	*/

	/*
	 * reset i_errno
	 */
	i_errno = 0;

	/*
	 * if we compare zero characters all strings are equal...
	 */
	if (n == 0)
		return(0);

	/*
	 * Scan with primary weight.
	 * Also build up the secondary weights to save a second
 	 * pass if the strings have equal primary weights.
	 */
	_isinit(coltab);

	for (indx1 = _getis(&s1, &dipht1, &fdipht1, &sdbl1),
	     indx2 = _getis(&s2, &dipht2, &fdipht2, &sdbl2);

	     (*s1 || dipht1) && (*s2 || dipht2);

	     indx1 = _getis(&s1, &dipht1, &fdipht1, &sdbl1),
	     indx2 = _getis(&s2, &dipht2, &fdipht2, &sdbl2)
	)
	{
		/*
		 * skip codes with zero primary weight
		 */
		while (indx1 != (i_char)I_ERROR && *s1 && ccp[indx1].cl_prim == 0)
			indx1 = _getis(&s1, &dipht1, &fdipht1, &sdbl1);

		while (indx2 != (i_char)I_ERROR && *s2 && ccp[indx2].cl_prim == 0)
			indx2 = _getis(&s2, &dipht2, &fdipht2, &sdbl2);

		/*
		 * if one of the codes does not exist, terminate with an error
		 */
		if (indx1 == (i_char)I_ERROR || indx2 == (i_char)I_ERROR)
		{
			i_errno = I_EICOD;
			return(0);
		}

		/*
		 * if we have reached the end of one string while skipping
		 * codes that do not collate we are almost done.
		 */
		if (*s1 == '\0' || *s2 == '\0')
			break;

		/*
		 * the first codes with different primary weights break
		 * the tie.
		 */
		if (ccp[indx1].cl_prim != ccp[indx2].cl_prim)
			return (ccp[indx1].cl_prim - ccp[indx2].cl_prim);
		/* 
		 * primaries were equal if secondary weight haven't decided
		 * so far then try the secondary weights for this character
		 */
		if (sec == 0)
			sec = ccp[indx1].cl_sec - ccp[indx2].cl_sec;

		/*
		 * terminate if we have reached limit specified
		 * by n.
		 */
		if (n != -1 && --n == 0)
			break;

		/*
		 * reset dipthong markers
		 */
		fdipht1 = fdipht2 = 0;
		continue;
	}

	/*
	 * Now we have one more chance for simple determination of the
	 * result: if one string is longer than the other, the last
	 * codes breaks the tie (longer strings collate after shorter
	 * strings!)
	 */
	if ((*s1 == '\0' || *s2 == '\0') && (*s1 || *s2))
		return((*s1 == '\0') ? -1 : 1);

	/*
	 * If we are here then simply return the secondary weight built
	 * up in the previous pass.
	 */
	return (sec);
}

/*
 * i_coll -- collate two strings
 *
 * SYNOPSIS:
 *	int
 *	i_coll(str1, str2, coltab)
 *	char *str1;
 *	char *str2;
 *	col_tab *coltab;
 *
 * DESCRIPTION:
 *	collate two strings according to coltab from the database
 *
 * RETURN:
 *	str1 < str2	< 0
 *	str1 = str2	= 0	(WARNING: also returned for errors!)
 *	str1 > str2	> 0
 *
 * RESTRICTIONS:
 *	This function will reset i_errno to zero, thus covering up any
 *	error that happend before. This is done because we have no way
 *	to signal successful completion otherwise.
 */
int
i_coll(s1, s2, coltab)
char *s1;
char *s2;
col_tab *coltab;
{
	int result;

	if (coltab == (col_tab *)0)
	{
		i_errno = I_EICOL;
		result = strcmp(s1, s2);
	}
	else
		result = _coll(s1, s2, -1, coltab);
	return(result);
}

/*
 * i_ncoll -- collate two strings for n codes
 *
 * SYNOPSIS:
 *	int
 *	i_ncoll(str1, str2, n, coltab)
 *	char *str1;
 *	char *str2;
 *	int n;
 *	col_tab *coltab;
 *
 * DESCRIPTION:
 *	collate two strings according to coltab from the database for at most
 *	n codes
 *
 * RETURN:
 *	str1 < str2	< 0
 *	str1 = str2	= 0	(WARNING: also returned for errors!)
 *	str1 > str2	> 0
 *
 * RESTRICTIONS:
 *	This function will reset i_errno to zero, thus covering up any
 *	error that happend before. This is done because we have no way
 *	to signal successful completion otherwise.
 *	If n is negative, all strings will compare equal.
 */
int
i_ncoll(s1, s2, n, coltab)
char *s1;
char *s2;
int n;
col_tab *coltab;
{
	int result;

	if (n > 0)
	{
		if (coltab == (col_tab *)0)
		{
			i_errno = I_EICOL;
			result = strncmp(s1, s2, n);
		}
		else
			result = _coll(s1, s2, n, coltab);
		return(result);
	}
	i_errno = 0;
	return(0);
}
