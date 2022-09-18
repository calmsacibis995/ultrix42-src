/* @(#)hc.h	4.1  (ULTRIX)        7/2/90     */

/*
 * hc.h
 */


/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
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
 * Macro definition of hc.c/input().
 * This is used to replace a call to
 *		(*linesw[tp->t_line].l_rint)(c,tp);
 * with
 *
 *		if (tp->t_line == HCLDISC)
 *			HCINPUT(c, tp);
 *		else
 *			(*linesw[tp->t_line].l_rint)(c,tp);
 */

int hcdebug;

#define HCBUFSIZ	1024

#define	HCINPUT(c, tp) { \
	if ((tp)->h_inbuf < HCBUFSIZ ) { \
		if ((tp)->h_in >= tp->h_top) \
			(tp)->h_in = tp->h_base; \
		*(tp)->h_in++ = c & 0377; \
		if (++(tp)->h_inbuf >= HCBUFSIZ || (tp)->h_read <= (tp)->h_inbuf) { \
			wakeup((caddr_t)&(tp)->t_rawq); \
		} \
	} \
}

