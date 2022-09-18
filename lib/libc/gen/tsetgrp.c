
#ifndef lint
static char *sccsid = "@(#)tsetgrp.c	4.1      ULTRIX  7/3/90";
#endif
 
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *									*
 *	Tim Burke  -	1/5/88						*
 * 		Created file to handle POSIX tcgetpgrp and tcsetpgrp.	*
 *									*
 ************************************************************************/

/*
 * POSIX termio.
 * This file is intended to be library routines to implement the 
 * tcsetpgrp and tcgetpgrp functions of POSIX.  They have to be library
 * routines because the Berkeley version uses integer pointers rather than
 * integer values.  
 */

#include <sys/types.h>
#include <sys/ioctl.h>

/*
 * tcgetpgrp - returns process group.
 */

pid_t tcgetpgrp(fildes)
	int fildes;
{
	int pgrp;
	if ((ioctl(fildes,TIOCGPGRP,&pgrp)) == -1)
		return(-1);
	return(pgrp);
}

/*
 * tcsetpgrp - sets process group.
 */
pid_t tcsetpgrp(fildes,pgrp_id)
	int fildes;
	int pgrp_id;
{
	if ((ioctl(fildes,TIOCSPGRP,&pgrp_id)) == -1)
		return(-1);
	return(0);
}
