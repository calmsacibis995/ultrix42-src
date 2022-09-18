/*
 *		@(#)assert.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History				*
 *									*
 *	David L Ballenger, 29-Mar-1985					*
 * 0001	Add defintions for System V compatibility.			*
 *									*
 *	Jon Reeves, 05-June-1989					*
 * 0002	Remove old Berkeley definitions; add ANSI def, protected by	*
 *	__STDC__ until we have compilers that do ((void) 0), #, and	*
 *	prototypes right; change name from _assert to __assert for	*
 *	ANSI compliance.						*
 *									*
 ************************************************************************/

/*
 * ANSI definitions of assert.
 */
#undef assert

#ifdef	__STDC__
#ifdef	NDEBUG
#define	assert(ignore)	((void) 0)
#else
extern	void	__assert(char *_Expr, char *_File, int _Line);
#define	assert(expr) \
	( (expr)? (void) 0 : __assert(#expr, __FILE__, __LINE__) )
#endif
#else	/* __STDC__ */
#ifndef NDEBUG
extern void __assert();
#define assert(EX) if (EX) ; else __assert("EX", __FILE__, __LINE__)
#else /* NDEBUG */
#define assert(EX)
#endif	/* NDEBUG */
#endif	/* __STDC__ */
