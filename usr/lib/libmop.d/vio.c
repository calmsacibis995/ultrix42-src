#ifndef lint
static	char	*sccsid = "@(#)vio.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 */

#include "vio.h"			/* virtual file i/o routines */
#include <sys/stat.h>			/* stat block defintions */
#include <sys/file.h>			/* file i/o flag definitions */
#include <stdio.h>			/* standard i/o library definitions */
#include <sys/errno.h>			/* errno number definitions */

extern int errno;

/*
 * allocate space for VFILES
 */
VFILE _vfiles[MAX_VFILES];
int _nvfiles = MAX_VFILES;


/*
 *		_ v o p e n
 *
 * This routine opens a file, and reads it into memory for performing
 * "virtual i/o" to it.
 *
 * Returns:		pointer to VFILE or NULL if file not opened
 *
 * Inputs:
 *	name		= name of file to open
 *	mode		= "r" if file to be opened for reading
 *			= "w" if file to be opend for writing
 */
VFILE *_vopen(name, mode)
char *name;
char *mode;
{
	VFILE *vp;
	struct stat stat;
	char *cp;
	char *malloc();
	int fd;
	int oflags = O_RDONLY;
	if (*mode == 'w')
		oflags = O_RDWR;
	/*
	 * look for a free VFILE stucture,
	 * and try to open the file
	 */
	errno = EMFILE; /* assume no space left for opening file */
	for (vp = _vfiles; vp < &_vfiles[MAX_VFILES]; vp++)
	{
		if (vp->v_fd == 0)
		{
			if ((fd = open(name, oflags, 0777)) == -1)
				break;
			if ((flock(fd, ((*mode == 'r') ? LOCK_SH : LOCK_EX)) == -1) ||
			(fstat(fd, &stat) == -1) ||
			((cp = malloc(stat.st_size)) == NULL) ||
			(read(fd, cp, stat.st_size) == -1))
			{
				close(fd);
				if (cp)
					free(cp);
				break;
			}
			vp->v_fd = fd;
			vp->v_name = name;
			vp->v_mode = *mode;
			vp->v_base = cp;
			vp->v_size = stat.st_size;
			vp->v_flags = 0;
			return(vp);
		}
	}
	return(NULL);
}

/*
 *		_ v c l o s e
 *
 * This routine is called to terminate access to a file,
 * which has been read into memory. If the file has been modified,
 * it is written out before being closed.
 *
 * Returns:		0 on success,
 *			-1 on error
 *
 * Inputs:
 *	vp		= pointer to VFILE structure for file
 *
 */
_vclose(vp)
VFILE *vp;
{
	int status = 0;
	if (vp->v_fd != 0)
	{
		if (vp->v_flags & VF_MODIFIED)
		{
			if (write(vp->v_fd, vp->v_base, vp->v_size) == -1)
				status = -1;
		}
		free(vp->v_base);
		if (close(vp->v_fd) == -1)
			status = -1;
		vp->v_fd = 0;
	}
	return(status);
}


/*
 *		_ v r e l o a d
 *
 * This routine rereads the node data base into memory for performing
 * "virtual i/o" to it.
 *
 * Returns:		0 for success, -1 for failure
 *
 * Inputs:
 *	vp		= VFILE ptr to file to read
 */
_vreload(vp)
VFILE *vp;
{
	struct stat stat;
	char *cp;

	if ((lseek(vp->v_fd, 0, 0) == -1) ||
		(fstat(vp->v_fd, &stat) == -1) ||
		((cp = malloc(stat.st_size)) == NULL) ||
		(read(vp->v_fd, cp, stat.st_size) == -1))
	{
		if ( cp )
			free(cp);
		return(-1);
	}

	if ( vp->v_base )
		free(vp->v_base);

	vp->v_base = cp;
	vp->v_size = stat.st_size;

	return(NULL);
}
