#ifndef lint
static char sccsid[] = "@(#)cpmv.c	4.1 (decvax!larry) 7/2/90";
#endif

#include "uucp.h"
#include <sys/types.h>
#include <sys/stat.h>


/***
 *	xcp(f1, f2)	copy f1 to f2
 *	char *f1, *f2;
 *
 *	return - 0 ok  |  FAIL failed
 *
 *
 *	decvax!larry -  copy modified path back to pointer
 */




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



xcp(f1, f2)
register char *f1, *f2;
{
	char buf[BUFSIZ];
	int len;
	register FILE *fp1, *fp2;
	char *lastpart();
	char full[100];
	struct stat s;

	if ((fp1 = fopen(subfile(f1), "r")) == NULL) {
		DEBUG(9, "can't open to read %s", subfile(f1));
		return(FAIL);
	}
	strcpy(full, f2);
	if (stat(subfile(f2), &s) == 0) {
		/* check for directory */
		if ((s.st_mode & S_IFMT) == S_IFDIR) {
			strcat(full, "/");
			strcat(full, lastpart(f1));
			/* calling program should know new name - decvax!larry*/
			strcpy(f2, full);
		}
	}
	DEBUG(4, "full %s\n", full);
	if ((fp2 = fopen(subfile(full), "w")) == NULL) {
		fclose(fp1);
		return(FAIL);
	}
	while((len = fread(buf, sizeof (char), BUFSIZ, fp1)) > 0)
		fwrite(buf, sizeof (char), len, fp2);
	fclose(fp1);
	fclose(fp2);
	return(0);
}


/*
 *	xmv(f1, f2)	move f1 to f2
 *	char * f1, *f2;
 *
 *	return  0 ok  |  FAIL failed
 */

xmv(f1, f2)
register char *f1, *f2;
{
	int ret;

	if (link(subfile(f1), subfile(f2)) < 0) {
		/*  copy file  */
		ret = xcp(f1, f2);
		if (ret == 0)
			unlink(subfile(f1));
		return(ret);
	}
	unlink(subfile(f1));
	return(0);
}
