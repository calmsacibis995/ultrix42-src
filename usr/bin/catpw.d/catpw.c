#ifndef lint
static char *sccsid = "@(#)catpw.c	4.1 ULTRIX 7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *                                                                      *
 *   This software is furnished under a license nd may be used and	*
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
 * 000 - Gary A. Gaudet - Fri Aug 25 18:07:31 EDT 1989
 *	catpw will print all password file entries independent of
 *	YP, bind, /etc/password, or any other mechanism.
 */

#include <stdio.h>
#include <pwd.h>

main (argc, argv)
int argc;
char **argv;
{
	struct passwd *p;	/* current entry */

	if (argc != 1) {	/* no arguments */
		fprintf (stderr, "catpw: argument count.\nusage: catpw\n");
		exit (1);
	}

/*
 *	print all password file entries
 */
	setpwent ();
	while (p = getpwent())
		fprintf (stdout, "%s:%s:%d:%d:%s:%s:%s\n",
			p->pw_name, p->pw_passwd, p->pw_uid, p->pw_gid,
			p->pw_gecos, p->pw_dir, p->pw_shell);

	endpwent ();
	return (0);
}
