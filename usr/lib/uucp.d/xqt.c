#ifndef lint
static char sccsid[] = "@(#)xqt.c	4.1 (decvax!larry) 7/2/90";
#endif

#include "uucp.h"
#include <signal.h>
#include <sys/ioctl.h>



/*******
 *	xuucico(rmtname)		start up uucico for rmtname
 *	char *rmtname;
 *
 *	return codes:  none
 */

xuucico(rmtname, addopts)
char *rmtname, *addopts;
{
	if (fork() == 0) {
		int tt;
		/*  start uucico for rmtname system  */
		char opt[100];
		close(0);
		close(1);
		close(2);
		open("/dev/null", 0);
		open("/dev/null", 1);
		open("/dev/null", 1);
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
		tt = open("/dev/tty", 2);
		if (tt)
		{
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
		if (rmtname[0] != '\0')
			sprintf(opt, "-s%.7s", rmtname);
		else
			opt[0] = '\0';
		execl(UUCICO, "UUCICO", "-r1",  opt, addopts, 0);
		exit(100);
	}
	return;
}


/*******
 *	xuuxqt()		start up uuxqt
 *
 *	return codes:  none
 */

xuuxqt()
{
	if (fork() == 0) {
		/*  start uuxqt  */
		close(0);
		close(1);
		close(2);
		open("/dev/null", 2);
		open("/dev/null", 2);
		open("/dev/null", 2);
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
		execl(UUXQT, "UUXQT",  0);
		exit(100);
	}
	return;
}
xuucp(str)
char *str;
{
	char text[300];
	if (fork() == 0) {
		/*  start uucp  */
		close(0);
		close(1);
		close(2);
		open("/dev/null", 0);
		open("/dev/null", 1);
		open("/dev/null", 1);
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
		sprintf(text, "%s -r %s", UUCP, str);
		execl(SHELL, "sh", "-c", text, 0);
		exit(100);
	}
	sleep(15);	/* Give uucp chance to finish */
	return;
}
