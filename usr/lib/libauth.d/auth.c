#ifndef lint
static	char	*sccsid = "@(#)auth.c	4.2	(ULTRIX)	11/15/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989 by			*
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
 * Modification history:
 *
 * 14-Nov-90 D. Long
 *	Added first_auth_user() and next_auth_user() functions for support
 *	of sequential reading.  Needed by getauthent() function.
 */

/*
 * Description:
 *
 * This module provides the most primitive interface into the auth data base.
 */
#include <pwd.h>
#include <ndbm.h>
#include "auth.h"
#define	ERROR		-1

DBM *_auth_db;

/*
 * Function to open the auth data base with the appropriate modes.
 */
int open_auth(data_base, flags)
char *data_base;
int flags;
{
	_auth_db = dbm_open(data_base, flags, 0600);
	return _auth_db == (DBM *) 0;
}

/*
 * Function to close the auth data base.
 */
int close_auth()
{
	dbm_close(_auth_db);
	return 0;
}

/*
 * Function to retrieve a users auth record from the auth data base.
 * Users are always indexed by their UID.
 */
int get_auth(user, auth)
UID user;
AUTHORIZATION *auth;
{
	datum key, content;
	int length;

	key.dptr = (char *) &user;
	key.dsize = sizeof user;
	content = dbm_fetch(_auth_db, key);
	length = content.dsize;
	if(length <= 0 || length > sizeof (AUTHORIZATION) || !content.dptr)
		return ERROR;
	bcopy(content.dptr, auth, length);
	return 0;
}

/*
 * Function to create or rewrite a users auth entry.
 */
int set_auth(user, auth)
UID user;
AUTHORIZATION auth;
{
	datum key, content;

	key.dptr = (char *) &user;
	key.dsize = sizeof user;
	content.dptr = (char *) &auth;
	content.dsize = sizeof auth;
	return dbm_store(_auth_db, key, content, DBM_REPLACE);
}

/*
 * Function to remove a users auth entry.  The record is first
 * zero filled to prevent disclosure of privileged information.
 */
int delete_auth(user)
UID user;
{
	AUTHORIZATION auth;
	datum key;

	bzero(&auth, sizeof auth);
	if(set_auth(user, auth) < 0)
		return ERROR;
	key.dptr = (char *) &user;
	key.dsize = sizeof user;
	return dbm_delete(_auth_db, key);
}

/*
 * Function to get the UID of the first auth entry in the database
 * in preparation for a sequential read.
 */
int first_auth_user(user)
UID *user;
{
	datum key;

	key = dbm_firstkey(_auth_db);
	if(key.dptr) {
		*user = *((UID *)(key.dptr));
		return 0;
	} else
		return -1;
}

/*
 * Function to sequentially retrieve UIDs from the database.  First_user
 * must have been called once before beginning calls to next_user.
 */
int next_auth_user(user)
UID *user;
{
	datum key;

	key = dbm_nextkey(_auth_db);
	if(key.dptr) {
		*user = *((UID *)(key.dptr));
		return 0;
	} else
		return -1;
}

/*
 * Function to retrieve all entries from the auth data base, calling
 * the specified function with each entry.
 */
int run_auth(func)
int (*func)();
{
	datum key, content;
	AUTHORIZATION auth;
	int i=0, uid, stat;

	for(key=dbm_firstkey(_auth_db); key.dptr != NULL; key=dbm_nextkey(_auth_db)) {
		uid = *((UID *)(key.dptr));
		if(get_auth(uid, &auth) < 0)
			return ERROR;
		else if((stat=(*func)(uid, auth)) != 0)
			return stat;
		i++;
	}
	return i;
}

/*
 * Function to fill in an auth record with acceptible default values.
 */
default_auth(auth)
AUTHORIZATION *auth;
{
	bzero(auth, sizeof *auth);
	strcpy(auth->a_password, INITIAL_PASSWORD);
	auth->a_pw_maxexp = DEFAULT_PASS_EXP;
	auth->a_pw_minexp = 0;
	return 0;
}
