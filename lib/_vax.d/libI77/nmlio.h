/*
*	@(#)nmlio.h	4.1	(ULTRIX)	7/17/90
*/

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

/************************************************************************
*
*			Modification History
*
*	David Metsky		10-Jan-86
*
* 001	Added from BSD 4.3 version as part of upgrade.
*
*	Based on:	nmlio.h		5.1		7/30/85
*
*************************************************************************/

/*	header for namelist I/O */

#define ERRNM(x)	if(n=(x)) return(n);
#define VL		16	/* variable name length from f77pass1/defs.h */

extern char namelistkey_;

typedef struct
{
	char namelistname[VL+4];	/* 4 for padding */
	struct namelistentry
		{
		char varname[VL+4];	/* 4 for padding */
		char *varaddr;
		short int type;
		short int typelen;
		/*
		 * If dimp is not null, then the corner element of the array is
		 * at varaddr.  However,  the element with subscripts:
		 * (i1,...,in) is at
		 *
		 *	varaddr - dimp->baseoffset + sizeoftype *
		 *			((i1-1)+span[0]*((i2-1)+span[1]*...)
		 */
		int *dimp;	/* dimension info: (null means scalar)
					*dimp: numb. of dim.,
					dimp[0]: number of dimensions
					dimp[1]: total number of elements,
					dimp[2]: base offset,
					dimp[3]: span of 1st dimension
					dimp[4]: span of 2nd dimension
						...			 */
		} names[1];	/* actually one per name in the namelist */
} Namelist;

typedef struct
{	flag cierr;
	ftnint ciunit;
	flag ciend;
	Namelist *namelist;
} namelist_arglist;
