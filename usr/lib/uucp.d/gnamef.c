#ifndef lint
static char sccsid[] = "@(#)gnamef.c	4.1 (decvax!larry) 7/2/90";
#endif

#include "uucp.h"
#include <sys/types.h>
#ifdef NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif



/*******
 *	gnamef(dirp, filename)	get next file name from directory
 *	DIR *dirp;
 *	char *filename;
 *
 *	return codes:
 *		0  -  end of directory read
 *		1  -  returned name
 */


gnamef(dirp, filename)
DIR *dirp;
char *filename;
{
	register struct direct *dentp;

	while (1) {
		if ((dentp = readdir(dirp)) == NULL)
			return(0);
		if (dentp->d_ino != 0)
			break;
	}

	/* Truncate filename.  This may become a problem someday. rti!trt */
	strncpy(filename, dentp->d_name, NAMESIZE-1);
	filename[NAMESIZE-1] = '\0';
	return(1);
}
