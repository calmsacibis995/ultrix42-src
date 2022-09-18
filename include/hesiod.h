/*	@(#)hesiod.h	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1984-1989 by			*
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
 * Description:  This file contains definitions for use by the Hesiod
 * 		 name service and applications.
 */
/*
 * Modification History:
 *
 * 13-Jun-89	logcher
 *	Made DEF_RHS NULL.
 *
 * 26-May-89	logcher
 *	Added HES_BUFMAX
 *
 * 17-May-89	logcher
 *	Created.
 */

/* Buf size */

#define HES_BUFMAX	1024

/* Configuration information */

#define HESIOD_CONF	"/etc/hesiod.conf"	/* Configuration file. */
#define DEF_RHS		NULL		/* Defaults if HESIOD_CONF */
#define DEF_LHS		NULL		/*    file is not present. */

/* Error codes */

#define	HES_ER_UNINIT	-1	/* uninitialized */
#define	HES_ER_OK	0	/* no error */
#define	HES_ER_NOTFOUND	1	/* Hesiod name not found by server */
#define HES_ER_CONFIG	2	/* local problem (no config file?) */
#define HES_ER_NET	3	/* network problem */
#define HES_ER_AUTH	4	/* authentication error */

/* Declaration of routines */

char *hes_to_bind();
char **hes_resolve();
