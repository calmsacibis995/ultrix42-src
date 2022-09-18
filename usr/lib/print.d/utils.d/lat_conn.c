static char *sccsid ="@(#)lat_conn.c	4.1 ULTRIX 7/2/90"; 

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Modification History:
 *
 * 18-Aug-89 Giles Atkinson & Margaret Irish
 *	Merged LAT and PMAX fixes.
 *	Changed lat includes back to <>.
 *
 *  4-APR-89	VJW
 *	Add array solid_list so we can remember which solicit id was
 *	used on which interface.  Try to get responses up to 3 times.
 *	Don't do exit return if the LIOCRES ioctl fails - we still
 *	need to continue on & check the other devices.  All this to
 *	fix host initiated connects not working when you've got more
 *	than 1 ethernet controller.
 *
 * 11-may-89 -- Adrian Thoms
 *	Changed lat includes to "" from <>.
 *
 * 12-Dec-89 -- Matt Thomas
 *	Changed to use new ioctl LIOCCONN to do the work of 300+ lines of
 *	old mungy code.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/ltatty.h>
#include <sys/file.h>
#include <errno.h>

/*
 *		l a t _ c o n n
 *
 * Solicit a connection between a LAT terminal and a terminal server port.
 *
 * Returns:		0 if success
 *			error code if not
 *
 * Inputs:
 *	subjport	= Name of subject port (ULTRIX LAT terminal)
 *	objnam		= Name of object (terminal server node)
 *	objport		= Name of object port
 *	objsrvc		= Name of object service
 *	
 */
lat_conn(subjport,objnam,objport,objsrvc)
    char *subjport;
    char *objnam;
    char *objport;
    char *objsrvc;
{
    int fd;
    struct ltattyi ltattyi;

    fd = open(subjport, O_RDWR|O_NONBLOCK, 0);
    if (fd < 0)
	return -1;
    if (strlen(objnam) >= sizeof(ltattyi.lta_server_name)) {
	errno = EMSGSIZE;
	return -1;
    }
    if (strlen(objport) >= sizeof(ltattyi.lta_server_port)) {
	errno = EMSGSIZE;
	return -1;
    }
    if (strlen(objsrvc) >= sizeof(ltattyi.lta_dest_port)) {
	errno = EMSGSIZE;
	return -1;
    }
    strcpy(ltattyi.lta_server_name, objnam);
    strcpy(ltattyi.lta_server_port, objport);
    strcpy(ltattyi.lta_dest_port, objsrvc);
    if (ioctl(fd, LIOCCONN, &ltattyi) < 0) {
	int error = errno;
	(void) close(fd);
	errno = error;
        return -1;
    } 
    return fd;
}
