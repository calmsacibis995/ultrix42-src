/*
#ifndef lint
static  char    *sccsid = "@(#)diskdef.h	4.1  (ULTRIX)        7/17/90";
#endif lint
*/

#define	DISK_VAR_SIZE		9	/* length of _vm */
#define DISK_SIZE		(DISK_VAR_SIZE+1)
#define	DISK_INDEX		1	/* attribute tags */
#define	DISK_DEVDESCR		2
#define	DISK_MOUNTDESCR		3
#define	DISK_TOTALKBYTES	4
#define	DISK_USEDKBYTES		5
