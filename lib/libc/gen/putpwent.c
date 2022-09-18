#ifndef lint
static	char	*sccsid = "@(#)putpwent.c	4.2	(ULTRIX)	8/7/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1990 by			*
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
 *
 * 28-Jun-90	D. Long
 *	Print out UID and GID as signed decimal numbers instead of unsigned
 *	decimal numbers.
 *
 *	David L Ballenger, 30-Mar-1985					*
 * 0001	Modify to work in an ULTRIX environment				*
 *									*
 ************************************************************************/

/*	@(#)putpwent.c	1.2	*/
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * format a password file entry
 */
#include <stdio.h>
#include <pwd.h>

extern int fprintf();

int
putpwent(p, f)
register struct passwd *p;
register FILE *f;
{
	/* Note that the pw_quota/pw_age and pw_comment fields are not
	 * present in an ULTRIX-32/32m password file, so these fields
	 * are simply ignored when writing out the entry. The getpwent()
	 * routine clears them.
	 */
	(void) fprintf(f, "%s:%s:%d:%d:%s:%s:%s\n",
		       p->pw_name,
		       p->pw_passwd,
		       p->pw_uid,
		       p->pw_gid,
		       p->pw_gecos,
		       p->pw_dir,
		       p->pw_shell);
	return(ferror(f));
}
