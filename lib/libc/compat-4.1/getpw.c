/* @(#)getpw.c	4.1 (Berkeley) 12/21/80 */

/*
 * Modification History:
 *
 *  2/11/87 - Fred L. Templin
 *
 *           Changed getpw() to call getpwuid() in order
 *	     to access the Yellow Pages.
 *
 */

#include	<stdio.h>
#include	<pwd.h>

getpw(uid, buf)
int uid;
char buf[];
{
	struct passwd *getpwuid();
	struct passwd *ent;

	if ((ent = getpwuid (uid)) == (struct passwd *)NULL)
		return (1);
	(void) sprintf (buf, "%s:%s:%d:%d:%s:%s:%s",
						ent->pw_name, ent->pw_passwd,
						ent->pw_uid, ent->pw_gid,
						ent->pw_gecos, ent->pw_dir,
						ent->pw_shell);
	return (0);
}
