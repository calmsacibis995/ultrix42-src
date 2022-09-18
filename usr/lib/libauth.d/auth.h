/* @(#)auth.h	4.2	(ULTRIX)	8/7/90 */
/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989, 1990 by		*
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
 * Description:
 *
 * Definitions for manipulating the Authorization Data-Base.
 */
#ifndef	_AUTH_H
#define	_AUTH_H

#ifndef	TRUSTED_MASK_LEN
#include <sys/audit.h>
#endif

#define	MAX_PASSWORD_LENGTH	16
#define	CRYPT_PASSWORD_LENGTH	26
#define	DEFAULT_PASS_EXP	(60*24*60*60)
#define	DEFAULT_ACCT_PRIVS	0
#define	INITIAL_PASSWORD	"Nologin"
#define	MAX_PRIVS		96
#define	MAX_AUDITS		(SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)*8
#define	DENY			0
#define	ALLOW			1

#define	AUTHORIZATION_DB	"/etc/auth"

typedef long UID;
typedef unsigned char PASSWORD[MAX_PASSWORD_LENGTH+1];
typedef char CRYPT_PASSWORD[CRYPT_PASSWORD_LENGTH+1];
typedef long TIME;
typedef unsigned short AUTH_MASK;
typedef unsigned char PRIVILEGE_MASK[(MAX_PRIVS+7)/8];
typedef int PRIVILEGE;
typedef unsigned short FAIL_COUNT;
typedef int AUDIT;
typedef unsigned long AUDIT_ID;
typedef unsigned char AUDIT_CONTROL;
typedef unsigned char AUDIT_MASK[(MAX_AUDITS+7)/8];

/*
 * The authorization record as it's stored in the ADB and passed in
 * the subroutine calls.
 */
struct authorization {
	UID		a_uid;
	char		a_password[CRYPT_PASSWORD_LENGTH];
	TIME		a_pass_mod;
	TIME		a_pw_minexp;
	TIME		a_pw_maxexp;
	AUTH_MASK	a_authmask;
	FAIL_COUNT	a_fail_count;
	struct audit_struct {
		AUDIT_ID	audit_id;
		AUDIT_CONTROL	audit_control;
		AUDIT_MASK	audit_mask;
	} a_audit;
	PRIVILEGE_MASK	a_privs;
};

typedef struct authorization AUTHORIZATION;
#define	a_audit_id	a_audit.audit_id
#define	a_audit_control	a_audit.audit_control
#define	a_audit_mask	a_audit.audit_mask

#define	A_LOGIN			1	/* Privilege to log in */
#define	A_CHANGE_PASSWORD	2	/* Privilege to change password */
#define	A_ENTER_PASSWORD	4	/* Non-auto-gen passwords accepted */

#define	A_EBADPASS		1001	/* Failed password authentication */
#define	A_ESOFTEXP		1002	/* Soft expired password */
#define	A_EHARDEXP		1003	/* Hard expired password */
#define	A_ENOLOGIN		1004	/* Account disabled */
#define	A_EOPENLINE		1005	/* Non secure communications line */

#if	__STDC__==1
AUTHORIZATION *getauthuid(int);
int endauthent(void), storeauthent(AUTHORIZATION *);
void setauthfile(char *);
#else
AUTHORIZATION *getauthuid();
int endauthent(), storeauthent();
void setauthfile();
#endif	/* __STDC__ */

#endif	/* _AUTH_H */
