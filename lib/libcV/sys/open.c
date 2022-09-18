#ifndef lint
static char *sccsid = "@(#)open.c	4.1	ULTRIX	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987, 1988 by		*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *			Modification History
 *
 *	David L Ballenger, 28-May-1985
 * 001	Don't save and reset errno accross this call, so that user can
 *	see what error if any occurred.
 *
 * 	Tim Burke, 12-Feb, 1987
 * 002  Specify termio open so that if it is on a tty line the driver 
 *	will setup propper termio default line parameters.
 *
 * 	Mark Parenti, 14-Jan, 1988
 * 003	Use fcntl() instead of ioctl() to set NDELAY so that file 
 *	descriptor flag is set. fcntl() calls ioctl().
 *
 *	Jon Reeves, 29-Mar-1988
 * 004	Don't change caller's third parameter, since it may not be
 *	present; put the termio flag in the second parameter, where it
 *	belongs.
 *
 ************************************************************************/


/*
	open -- system call emulation for 4.2BSD

	last edit:	15-Dec-1983	D A Gwyn
*/
#include	<errno.h>
#include	<sys/file.h>
#include	<sys/ioctl.h>


extern int	_open(), _ioctl();
extern int	errno;

int
/*VARARGS2*/
open( path, oflag, mode )		/* returns fildes, or -1 */
	char		*path;		/* pathname of file */
	register int	oflag;		/* flag bits, see open(2) */
	int		mode;		/* O_CREAT protection mode */
{
	register int	fd;		/* file descriptor */

	/*
	 * Specify that this is a termio open so that the drivers will
	 * open a termio tty line under the correct default settings.
	 */
	fd = _open( path, oflag | O_TERMIO, mode );
	/*
	 * On System V, the O_NDELAY flag, affects not only the
	 * open, but also any subsequent read() and write() calls.
	 * This has to be emulated by calling fcntl().
	 */
	if ( (fd >= 0) && ((oflag & O_NDELAY) != 0) ) {
		static int	on = 1; 

/*
		if (_ioctl( fd, FIONBIO, (char *)&on ) == -1) {
*/
		if (_fcntl( fd, F_SETFL, FNDELAY) == -1) {
			/*
			 * If the ioctl fails, close the file and return
			 * ENXIO.
			 */
			(void)close(fd);
			errno = ENXIO ;
			return( -1 );
		}
	}
	return fd;
}
