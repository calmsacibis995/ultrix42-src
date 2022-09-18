/*	@(#)cuserid.c	4.1	ULTRIX	7/3/90	*/
/*LINTLIBRARY*/
/*
 *	Modification History
 *
 * 001	M. Parenti, 1988 Jan 14
 *	Use effective userid for POSIX compliance.
 *
 * 002	M. Parenti, Aug. 9, 1988
 *	Don't call getlogin() if in POSIX mode.  POSIX requires the 
 *	returned name to be associated with the effective uid
 *	and getlogin returns a name associated with the login id which
 *	is not necessarily the current effective uid
 *
 * 003	M. Parenti, Aug 25, 1988
 *	Change progenv to short because kernel only copyout's a short.
 *
 * 004	L. Scott, Jan 04, 1990
 *	Changes in return values for POSIX compliance
 */
#include <stdio.h>
#include <pwd.h>
#include <sys/sysinfo.h>
#include <sys/exec.h>

extern char *strcpy(), *getlogin();
extern int geteuid();
extern struct passwd *getpwuid();
static char res[L_cuserid];

char *
cuserid(s)
char	*s;
{
	register struct passwd *pw;
	register char *p;
	short	progenv;

	if (s == NULL)
		s = res;
	if( getsysinfo(GSI_PROG_ENV, &progenv, sizeof(progenv), 0, 0, 0) < 1 )
		progenv = A_BSD;
	if( progenv != A_POSIX) {
		p = getlogin(); 
		if (p != NULL)
			return (strcpy(s, p));
	}
	pw = getpwuid(geteuid());
	endpwent();
	if (pw != NULL)
		return (strcpy(s, pw->pw_name));
	*s = '\0';
	if( progenv == A_POSIX && s != res) return(s);
	return (NULL);
}
