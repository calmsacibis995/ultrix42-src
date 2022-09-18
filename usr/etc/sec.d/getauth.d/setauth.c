#ifndef lint
static	char	*sccsid = "@(#)setauth.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989, 1990 by			*
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
#include <sys/file.h>
#include <sys/audit.h>
#include <syscall.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <auth.h>

#define LEN (SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)

/*
  Program to set the ADB entry for a specific user.

Modification history.

02	02/07/90	D. Long
	Fixed audit message.
01	08/11/89	Bob Fontaine
	Added audit code and an exit routine.

*/

main(argc, argv)
int argc;
char *argv[];
{
	static char db[] = AUTHORIZATION_DB;
	struct passwd *pwd;
	AUTHORIZATION authorization;
	char buff[1000],buf[LEN],statustr[100];
	int i, status, c;

/* turn off auditing except for AUTH_EVENT and failed setgroups */

        if ( audcntl (SET_PROC_ACNTL,(char *)0,0,AUDIT_AND,0) == -1)
                perror ( "audcntl" );
        A_PROCMASK_SET ( buf, AUTH_EVENT, 1, 1 );
        if (audcntl(SET_PROC_AMASK,buf,LEN,0,0) == -1 )
                perror ( "audcntl" );

/*
  Check call line.
*/
	if(argc != 1)
		getout("usage: setauth",2);
/*
  Open the ADB.
*/
	if(open_auth(db, O_RDWR)) {
		sprintf(statustr,"Unable to open authorization data base %s.",db);
		getout(statustr,3);
	}
	status = 0;
/*
  Read ASCII auth lines from the standard input.
*/
	while(fgets(buff, sizeof buff, stdin)) {
		i = strlen(buff);
		if(i > 2048) {
			status = 1;
			strcpy(statustr,"record too long, ignored", stderr);
			if(buff[i-1] != '\n') {
				while((c=getchar()) != '\n' && c != EOF) ;
				if(c == EOF)
					break;
			}
		}
/*
  Set the individual fields of the ADB information.
*/
		if(binauth(buff, &authorization)) {
			sprintf(statustr,"Error in input record.");
			status = 1;
			continue;
		}
/*
  Store users entry into ADB, overwriting any existing entry for this
  UID.
*/
		if(set_auth(authorization.a_uid, authorization)) {
			sprintf(statustr,"Unable to set authorization entry for UID %d",
				authorization.a_uid);
			status = 1;
		}
		else {
			sprintf(statustr,"Set authorization entry for UID %d",
				authorization.a_uid);
			status = 0;
		}
	}
	close_auth();
	getout(statustr,status);
}
/*
   Leave the program. Generate an audit record.  Print an error message
  if necessary.
*/
getout(statustr,status)
char *statustr;
int status;
{
        char tmask[AUD_NPARAM];
        struct {
                char *a;
                int  b;
        } aud_arg;
        int i;

        /* build token mask */

        tmask[0] = T_CHARP;
	if(status == 0)
	        tmask[1] = T_RESULT;
	else
	        tmask[1] = T_ERROR;
        tmask[2] = '\0';

        /* fill in values to be recorded */

        aud_arg.a = statustr;
        aud_arg.b = status;

        /* generate audit record */
        if ( audgen ( AUTH_EVENT, tmask, &aud_arg ) == -1 ) perror ( "audgen" );
        if(status > 0)
	{
                fputs(statustr, stderr);
		fputs("\n",stderr);
	}
        exit(status);
}
