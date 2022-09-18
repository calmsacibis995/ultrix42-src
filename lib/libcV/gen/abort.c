/* @(#)abort.c	4.1	(ULTRIX) 7/3/90 */
/*	3.0 SID #	1.4	*/
/*LINTLIBRARY*/
/*
 *	abort() - terminate current process with dump via SIGIOT
 *
 * 001 - Linda Wilson 04-oct-1989
 *       Change SIGIOT to SIGABRT for standards conformance.
 *	 (Same signal, different name).
 * 002 - Jon Reeves, 1990-Jan-10
 *	Force exit if the signal handler returns.
 */

#include <signal.h>

extern int kill(), getpid();
static pass = 0;		/* counts how many times abort has been called*/

int
abort()
{
	extern void	_cleanup();	/* DAG -- added */

	/* increment first to avoid any hassle with interupts */
	if (++pass == 1)  {
		_cleanup();
	}
#ifdef _POSIX_SOURCE
	kill(getpid(), SIGABRT);
	signal(SIGABRT, SIG_DFL);
#endif
	return(kill(getpid(), SIGABRT));
}
