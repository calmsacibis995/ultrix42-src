/*   Modification Record
 *
 *   7/30/86	Aya Konishi (konishi@gully)
 * 	Modified the exiting procedure so that the 'learn files'
 * 	would not leave the terminal in funny line editing
 *	mode.
 */
#ifndef lint
static char *sccsid = "@(#)wrapup.c	4.1	ULTRIX	7/17/90";
#endif not lint

#include "signal.h"
#include "stdio.h"
#include "lrnref.h"

wrapup(n)
int n;
{
/* this routine does not use 'system' because it wants interrupts turned off */

	signal(SIGINT, SIG_IGN);
	chdir("..");
	if (fork() == 0) {
		signal(SIGHUP, SIG_IGN);
#if bsd4_2
		if (fork() == 0) {
			close(1);
			open("/dev/tty", 1);
			execl("/bin/stty", "stty", "new", 0);
		}
#endif
		execl("/bin/rm", "rm", "-rf", dir, 0);
		execl("/usr/bin/rm", "rm", "-rf", dir, 0);
		perror("bin/rm");
		fprintf(stderr, "Wrapup:  can't find 'rm' command.\n");
		exit(0);
	}
	if (!n && todo)
		printf("\nTo take up where you left off type \"learn %s %s\".\n", sname, todo);
	printf("Bye.\n");	/* not only does this reassure user but it
				stalls for time while deleting directory */
	fflush(stdout);
	wait(0);
	if ( strncmp(sname, "files",5) == 0) {    /* Fixed by Aya */
		system("stty erase  kill  start u stop u");
	}
	exit(n);
}
