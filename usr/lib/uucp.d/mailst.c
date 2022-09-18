#ifndef lint
static char sccsid[] = "@(#)mailst.c	4.1	(ULTRIX)	7/2/90";
#endif

#include "uucp.h"



/*******
 *	mailst(user, str, file, ferr)
 *
 *	mailst  -  this routine will fork and execute
 *	a mail command sending string (str) to user (user).
 *	If file is non-null, the file is also sent.
 *	(this is used for mail returned to sender.)
 */

/* decvax!larry - ifdef ULTRIX use "uucp" id on from line for mail */




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




mailst(user, str, file, ferr)
char *user, *str, *file, *ferr;
{
	FILE *fp, *fi;
	char cmd[BUFSIZ], buf[BUFSIZ];
	int nc;

#ifdef ULTRIX
	sprintf(cmd, "/bin/mail -r uucp %s", user);
#else
	sprintf(cmd, "mail %s", user);
#endif
	
	if ((fp = popen(cmd, "w")) == NULL)
		return;
	fprintf(fp, "%s\n", str);

/* 

	if (*ferr != '\0' && (fi = fopen(subfile(ferr), "r")) != NULL) {
		fprintf(fp, "\n\n%s\n\n",  "****** the following lines are the standard error output for the command *******");
		while ((nc = fread(buf, sizeof (char), BUFSIZ, fi)) > 0)
			fwrite(buf, sizeof (char), nc, fp);
		fclose(fi);
	}

*/
	if (*file != '\0' && (fi = fopen(subfile(file), "r")) != NULL) {
		fprintf(fp, "\n\n%s\n\n",  "****** the following is the original input file *******");
		while ((nc = fread(buf, sizeof (char), BUFSIZ, fi)) > 0)
			fwrite(buf, sizeof (char), nc, fp);
		fclose(fi);
	}

	pclose(fp);
	return;
}
