#ifndef lint
static char sccsid[] = "@(#)us_open.c	4.1 (decvax!larry) 7/2/90";
#endif

#include "uucp.h"
#ifdef UUSTAT
#define USOPEN
#endif

#ifdef UUSUB
#define USOPEN
#endif

#ifdef USOPEN
#include <sys/types.h>
#include "uust.h"
#include <sys/stat.h>
 
/*********
 *	us_open(file, mode, lock, atime, stime)	 open a file
 * 
 *	If the file "file" to be opened for writing, then wait for
 *	the "lock" file.
 *	If a lock already exists, the create time is checked for older
 *	than the age time (atime).
 *	If "lock" is old enough, then unlink it and create a new one.
 *	The "stime" is the sleep time while waiting.
 *	The file pointer is returned.
 */
 
FILE *
us_open(file, mode, lock, atime, stime)
char *file, *mode, *lock;
short stime, atime;
{
	long time();
	struct stat stbuf;	/* stat buffer : stat.h */
	FILE *fp;
	int fd;
 
	DEBUG(6, " Enter us_open, file: %s\n", file);
	if (strcmp(mode, "r") != SAME) {	/* wait lock */
		while ((fd=creat(lock,0)) == -1) {	/* lock exists */
			DEBUG(6,"in us_open, lock exists: %s", lock);
			DEBUG(6," sleep for %d seconds\n", stime);
			stat(lock, &stbuf);	/* get lock file status */
			sleep(stime);		/* sleep stime seconds */
			if ((time((long *)0)-stbuf.st_ctime)>(long)atime)
				unlink(lock);
		}
		close(fd);
	}
 
	if ((fp=fopen(file,mode)) == NULL) {
		fprintf(stderr, "File %s access denied\n", file);
		unlink(lock);
		DEBUG(5, "abnormal return from us_open, unlink %s\n", lock);
		return(fp);		/* return NULL */
	}
	DEBUG(6, " normal return from us_open.c\n %s", "");
	return(fp);
}
#else
static	int	us_open_here;	/* quiet 'ranlib' command' */
#endif
