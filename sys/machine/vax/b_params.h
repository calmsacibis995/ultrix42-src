/*
 *	@(#)b_params.h	4.2	(ULTRIX)	8/3/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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
 * Modification History:
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Changes for VAX9000 support.
 *
 * 4-Jul-85 -jrs
 *	Changes for support of the VAX8800 were merged in.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 14-Mar-85 -jaw
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 */
#ifdef UPGRADE
#define Upgrade	UPGRADE
#else
#define Upgrade 0
#endif
#define	LOGLIM9000	64
#define LOGLIM8800	32
#define LOGLIM8600	32
#define LOGLIM8200	32
#define LOGLIM780	32
#define LOGLIM750	32
#define LOGLIM730	16
#define LOGLIMMVAX	2
