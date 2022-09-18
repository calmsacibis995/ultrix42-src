/*
*	@(#)ndu.h	4.1	(ULTRIX)	7/17/90
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
*	David Metsky		15-Jan-86
*
* 001	Added from BSD 4.3 version as part of upgrade.
*
*	Based on:	ndu.h		4.1		85/03/19
*
*************************************************************************/

/*
 * This file defines the basic tree node data structure for the PCC.
 */

union ndu {
	struct {		/* interior node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
#ifndef FLEXNAMES
		char	name[NCHNAM];
#else
		char	*name;
		int	stalign;
#endif
		NODE	*left;
		NODE	*right;
	} in;
	struct {		/* terminal node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
#ifndef FLEXNAMES
		char	name[NCHNAM];
#else
		char	*name;
		int	stalign;
#endif
		CONSZ	lval;
		int	rval;
	} tn;
	struct {		/* branch node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
		int	label;		/* for use with branching */
	} bn;
	struct {		/* structure node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
		int	stsize;		/* sizes of structure objects */
		int	stalign;	/* alignment of structure objects */
	} stn;
	struct {		/* front node */
		int	op;
		int	cdim;
		TWORD	type;
		int	csiz;
	} fn;
	/*
	 * This structure is used when a double precision
	 * floating point constant is being computed
	 */
	struct {			/* DCON node */
		int	op;
		TWORD	type;
		int	cdim;
		int	csiz;
		double	dval;
	} dpn;
	/*
	 * This structure is used when a single precision
	 * floating point constant is being computed
	 */
	struct {			/* FCON node */
		int	op;
		TWORD	type;
		int	cdim;
		int	csiz;
		float	fval;
	} fpn;
};
