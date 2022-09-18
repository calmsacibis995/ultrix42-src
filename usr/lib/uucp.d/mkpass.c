#ifndef lint
static char sccsid[] = "@(#)mkpass.c	4.1 (Ultrix) 7/2/90";
#endif

/*
    mkpass

 ************************************************************************
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

#include <sys/file.h>

#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>

char	*strcpy();
char	*crypt();
char	*getpass();
char	*pw;
char	*gets();
char	pwbuf[256];
extern	int errno;

main(argc, argv)
	char *argv[];
{
	int i;
	char saltc[2];
	long salt;
	int c, pwlen, fd;
	int pid = getpid();

	
	while (gets(pwbuf) != NULL) {
		pwbuf[8] = '\0';
		time(&salt);
		salt = 9 * pid;
		saltc[0] = salt & 077;
		saltc[1] = (salt>>6) & 077;
		for (i = 0; i < 2; i++) {
			c = saltc[i] + '.';
			if (c > '9')
				c += 7;
			if (c > 'Z')
				c += 6;
			saltc[i] = c;
		}
		pw = crypt(pwbuf, saltc);
		puts(pw);
	}
}
