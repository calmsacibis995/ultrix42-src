#ifndef lint
static char	*sccsid = "@(#)logerr.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
*
*
* File Name: logerr.c
*
* Source file description: Allow a privileged user to write an error message
*	into the kernel error log buffer.  Intended for hardware errors
*	and important system related errors.  User process  and daemon
*	process related errors should be logged to syslog.
*
* Functions:
*
* Usage:
*
* Compile:
*
* Modification history:
*
*/

#include <sys/file.h>
#include <syslog.h>
#include <sys/param.h>
#define MAX_SIZE 256 - (sizeof(short))
#define ERROR -1
#define SUCCESS 0
/*
*
*
* Function: logerr(class, str)
*
* Function description: Privileged user process can log a hardware
*	error into the kernel error log buffer.
*
* Arguments:
*	class	class of error to be logged, as defined in <sys/errlog.h>
*			(e.g. ELMSGT_INFO)
*	str	pointer to string of text to be logged, if any
*
* Return value: SUCCESS or ERROR
*
* Side effects: None
*
*/


logerr(class,str)
short class;
char *str;
{
	/* log an error into kernel error log buffer */
	/* only executable by root */

	int fd;
	int i;
	char line[256];
	int cnt = 0;
	char *ptr;
	struct llog_msg {
		short class;
		char line[256];
	} llogmsg;

	if (geteuid()) {
		(void)sprintf(line, "\nlogerr: Must be super-user\n");
		syslog(LOG_SALERT, line);
		return(ERROR);
	}	

	fd = open("/dev/errlog", O_WRONLY, 0); 

	if (fd < 0){
		(void)sprintf(line, "\nlogerr: Can't open /dev/errlog\n");
		syslog(LOG_SALERT, line);
		return(ERROR);
	}

	if (str != NULL) {
  	    for(ptr=str; *ptr != NULL && cnt < MAX_SIZE;cnt++, ptr++) ;

	    if (cnt < MAX_SIZE && *ptr == NULL) {
		/* copy into structure */
		cnt++;	/* include null term */
		strcpy(llogmsg.line, str, cnt);
	    }
	    else {
		return(ERROR);
	    }
	}

	llogmsg.class = class;
	i = write(fd, &llogmsg, sizeof(short)+cnt);
	if (i < 0) {
		(void)sprintf(line,"\nlogerr: Can't write /dev/errlog\n");
		syslog(LOG_SALERT, line);
		return(ERROR);

	}
	(void)close(fd);
	return(SUCCESS);
}





