#ifndef lint
static	char	*sccsid = "@(#)getauth.c	4.1	(ULTRIX)	%G";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
#include <auth.h>

#define LEN (SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)

/*
  Program to retrieve and display tersely, in ASCII, the ADB entry
  of a specific user.  If the username argument is omitted the entire ADB
  will be printed out, one line per user.

Modification History.

01	8/14/89	Bob Fontaine
	Added code to generate audit records.  Added an exit routine.

*/

/*
  Function to verify a string is a valid signed integer.
*/
int isnumber(number)
char *number;
{
	register char c;

	if(*number == '+' || *number == '-')
		number++;
	while(c = *number++)
		if(c < '0' || c > '9')
			return 0;
	return 1;
}

/*
  Function to print out an auth file entry in ASCII.
*/
int printit(uid, auth)
UID uid;
AUTHORIZATION auth;
{
	puts(asciiauth(&auth));
	return 0;
}

/*
  Main program of "getauth".
*/
main(argc, argv)
int argc;
char *argv[];
{
	static char db[] = AUTHORIZATION_DB;
	struct passwd *pwd;
	UID uid;
	AUTHORIZATION auth;
	int status, i, j;
	char buf[LEN],*name,statustr[100];

/* turn off auditing of all events except for LOGIN and failed setgroups */

        if ( audcntl (SET_PROC_ACNTL,(char *)0,0,AUDIT_AND,0) == -1)
                perror ( "audcntl" );
        A_PROCMASK_SET ( buf, AUTH_EVENT, 1, 1 );
        if (audcntl(SET_PROC_AMASK,buf,LEN,0,0) == -1 )
                perror ( "audcntl" );

/*
  Check call line.
*/
	if(argc > 1 && *argv[1] == '-') {
		sprintf(statustr,"usage: getauth [username]");
		getout(statustr,2);
	}
/*
  Get users ADB entry from their UID.
*/
	if(open_auth(db, O_RDONLY)) {
		sprintf(statustr,"Unable to open auth data base %s.",db);
		getout(statustr,3);
	}
	status = 0;
/*
  If arguements supplied treat each as a username or UID.  Else just
  print all records in auth file.
*/
	if(argc >= 2) {
		for(i=argc-1; i > 0; i--) {
			name = *++argv;
/*
  Get users passwd entry based on their username.
*/
			if(!(pwd=getpwnam(name))) {
/*
  If passwd entry not found see if argument is a UID.
*/
				if(!isnumber(name)) {
					sprintf(statustr,"User %s not found in passwd file.",name);
					status = 1;
					continue;
				}
				uid = atoi(name);
			} else
				uid = pwd->pw_uid;
/*
  Retrieve auth entry for UID.
*/
			if(get_auth(uid, &auth)) {
				sprintf(statustr,"User %s not found in auth database.",name);
				status = 1;
				continue;
			}
			printit(uid, auth);
		}
	} else {
/*
  Print out the entire auth data base.
*/
		run_auth(printit);
	}
	close_auth();
	if(status == 0)
	   sprintf(statustr,"Authorization database read by %s.",getlogin());
	getout(statustr,status);
}
/*
Print an error message (if necessary) and exit.
*/
getout(statustr,status)
int status;
char *statustr;
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


        aud_arg.a = statustr;
        aud_arg.b = status;

        /* generate audit record */
        if ( audgen ( AUTH_EVENT,tmask,&aud_arg) == -1 ) perror ( "audgen" );

        if(status > 0)
	{
                fputs(statustr, stderr);
		fputs("\n",stderr);
	}
        exit(status);
}
