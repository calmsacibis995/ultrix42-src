#ifndef lint
static char *sccsid = "@(#)remove.c	4.1     ULTRIX  7/3/90";
#endif

/***********************************************************************
 *                                                                     *
 *                      Copyright (c) 1989 by                          *
 *              Digital Equipment Corporation, Maynard, MA             *
 *                      All rights reserved.                           *
 *                                                                     *
 *   This software is furnished under a license and may be used and    *
 *   copied  only  in accordance with the terms of such license and    *
 *   with the  inclusion  of  the  above  copyright  notice.   This    *
 *   software  or  any  other copies thereof may not be provided or    *
 *   otherwise made available to any other person.  No title to and    *
 *   ownership of the software is hereby transferred.                  *
 *                                                                     *
 *   This software is  derived  from  software  received  from  the    *
 *   University    of   California,   Berkeley,   and   from   Bell    *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to    *
 *   restrictions  under  license  agreements  with  University  of    *
 *   California and with AT&T.                                         *
 *                                                                     *
 *   The information in this software is subject to change  without    *
 *   notice  and should not be construed as a commitment by Digital    *
 *   Equipment Corporation.                                            *
 *                                                                     *
 *   Digital assumes no responsibility for the use  or  reliability    *
 *   of its software on equipment which is not supplied by Digital.    *
 *                                                                     *
 **********************************************************************/

/***********************************************************************
 * remove - removes a file or a directory, first it assumes that the
 *          path is a directory and calls rmdir. If it fails because
 *          part of the path is not a directory it tries calling
 *          unlink with the same path. 
 *
 *          This library function was created for conformance with XPG3
 ***********************************************************************/

#include <errno.h>

extern	int	errno;

int
remove (path)

char	*path;

{
    if (rmdir(path) != 0) {
	if (errno == ENOTDIR) {
	    /* Try Unlinking the file */
	    if (unlink(path) == 0)
		return (0); /* unlink succeeded */
	}
	return(-1); /* both failed */
    }
    return(0);  /* rmdir succeeded */
}

