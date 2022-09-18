#ifndef lint
static	char	*sccsid = "@(#)rmauth.c	4.1	(ULTRIX)	7/2/90";
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
  Program to delete a users ADB entry.

Modification history.

01	08/11/89	Bob Fontaine
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
  Main program.
*/
main(argc, argv)
int argc;
char *argv[];
{
	static char db[] = AUTHORIZATION_DB;
	struct passwd *pwd;
	AUTHORIZATION auth;
	UID uid;
	int i;
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
	if(argc != 2)
		getout("usage: rmauth username",2);
/*
  Get users passwd entry based on their username.
*/
	name = argv[1];
	if(!(pwd=getpwnam(name))) {
/*
  If passwd entry not found see if argument is a UID.
*/
		if(!isnumber(name)) {
			sprintf(statustr,"User %s not found in passwd file.",name);
			getout(statustr,1);
		}
		uid = atoi(name);
	} else
		uid = pwd->pw_uid;
/*
  Get users ADB entry from their UID.
*/
	if(open_auth(db, O_RDWR)) {
		sprintf(statustr,"Unable to open auth data base %s.",db);
		getout(statustr,3);
	}
/*
  Verify they have an auth entry.
*/
	if(get_auth(uid, &auth)) {
		sprintf(statustr,"User %s not found in auth database.",name);
		getout(statustr,1);
	}
/*
  Delete their auth entry.
*/
	if(delete_auth(uid) < 0) {
		sprintf(statustr,"Unable to delete users %s auth entry.",name);
		getout(statustr,1);
	}
	close_auth();
	sprintf(statustr,"Removed user %s adb entry from %s.",name,db); 
	getout(statustr,0);
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
	if ( audgen ( AUTH_EVENT,tmask,&aud_arg) == -1 ) perror ( "audgen" );

	if(status > 0)
	{
		fputs(statustr, stderr);
		fputs("\n",stderr);
	}
	exit(status);
}
