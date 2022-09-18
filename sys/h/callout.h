/*
 *	@(#)callout.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987, 1988 by		*
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
 *			Modification History
 *
 * 09-Nov-89 jaw
 *	change to timeout support in smp.
 *
 *  21-Sep-89   Pete keilty
 *	Removed ipl from callout structure. Ksched no longer needs this
 *	ipl saved and used in softclock. Calling routine raises ipl.
 *
 *  08-Feb-89	jaw
 *	fix race condition between timeout and untimeout 
 *
 *  26-Jan-89	jaw
 *	make call out simpler
 *
 * 22 Dec 88 -- miche
 *	Added copyright and this modification history
 *
 ************************************************************************/

/*
 * The callout structure is for
 * a routine arranging
 * to be called by the clock interrupt
 * (clock.c) with a specified argument,
 * in a specified amount of time.
 * Used, for example, to time tab
 * delays on typewriters.
 */

struct	callout {
	struct	callout *c_next;
	int	c_time; 	/* incremental time */
	caddr_t c_arg;		/* argument to routine */
	int	(*c_func)();	/* routine */
};

#define	CALLOUT_INPROGRESS	0x1
#define	CALLOUT_DONOT_REQUEUE	0x2


#ifdef KERNEL
struct	callout *callfree, *callout;
int ncallout;
#endif

struct chrout {
	int c_arg;
	int d_arg;
	int (*c_func)();
	struct chrout *c_next;
};
#ifdef KERNEL
struct chrout *chrfree, *chrlst, *chrcur, *chrout;
int nchrout;
#endif
