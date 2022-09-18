#ifndef lint
static	char	*sccsid = "@(#)umount.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987 by			*
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
 *			Modification History
 *
 *	From Doug Gwyn
 *
 * 	Mark A Parenti, 09-Oct-1987
 * 001	Rewrite for new Ultrix interface
 *
 ************************************************************************/

#include <sys/param.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mount.h>

char *malloc();

extern int	_umount(), getuid();
#define MSIZE (NMOUNT*sizeof(struct fs_data))

int
umount( spec )
	char	*spec;			/* special file to unmount */

	{
        register struct fs_data *mountbuffer,*fs_data;
	int	start = 0;
	int	ret;

        /* get the mounted file systems */
        if((mountbuffer = (struct fs_data *) malloc(MSIZE)) == NULL) {
                return(-1);
        }
       /* use this so that we don't hang if server's down with nfs file sys */
       if( (ret = getmountent(&start, mountbuffer, NMOUNT)) == -1)
		return(-1);
        if ((ret == 0) || ret == 1) {
		errno = ENOENT;
		return(-1);
        }
	/* search mount table for special device */
        for (fs_data = &mountbuffer[ret-1], ret=0; fs_data >= &mountbuffer[1];
                                                                fs_data--) {

	/* look for right special device */
		if(strcmp(fs_data->fd_devname, spec) == 0) 
		{

			if ( _umount( fs_data->fd_dev ) != 0 )
				{
				if ( errno == ENODEV )
					if ( getuid() != 0 )
						errno = EPERM;	/* not super-user */
					else
						errno = ENXIO;	/* spec nonexistent */
				/* other errno values are already okay */
			
				return -1;
				}

			return 0;
		}
	}
	/* Didn't find it	*/
	errno = ENOENT;
	return(-1);
	}
