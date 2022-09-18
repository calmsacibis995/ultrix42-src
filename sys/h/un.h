/* static	char	*sccsid = "@(#)un.h	4.1	(ULTRIX)	7/2/90"; */

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/h/un.h
 *
 * 15 Jan 88 -- lp
 *	Changed sun_path length to be based on small mbuf size. This 
 *	may not work if utility does not know MLEN.
 *
 * 23 Oct 84 -- jrs
 *	Changed structure size to conform with current Berkeley size
 *	Derived from 4.2BSD, labeled:
 *		un.h 6.2	84/05/07
 *
 * -----------------------------------------------------------------------
 */

/*
 * Definitions for UNIX IPC domain.
 */
struct	sockaddr_un {
	short	sun_family;		/* AF_UNIX */
#ifdef MLEN
	char	sun_path[MLEN-4];		/* path name (gag) */
#else
	char	sun_path[108-4];	/* Might not known what MLEN is */
#endif
};

#ifdef KERNEL
int	unp_discard();
#endif
